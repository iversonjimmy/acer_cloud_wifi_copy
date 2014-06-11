//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "SyncConfigThreadPool.hpp"
#include "gvm_errors.h"
#include "gvm_thread_utils.h"
#include "scopeguard.hpp"
#include "vplu_format.h"
#include "vplu_mutex_autolock.hpp"
#include "vplex_assert.h"

#include <new>

#include "log.h"

SyncConfigThreadPool::SyncConfigThreadPool()
:  m_initSuccess(false),
   m_threadPoolMutex(NULL),
   m_shutdown(false),
   m_generalThreads(NULL),
   m_dedicatedThreads(NULL)
{
    m_threadPoolMutex = new (std::nothrow) VPLMutex_t;
    if(m_threadPoolMutex == NULL) {
        LOG_ERROR("Out of memory");
        return;
    }
    int rc = VPLMutex_Init(m_threadPoolMutex);
    if(rc != VPL_OK) {
        LOG_ERROR("VPLMutex_Init:%d", rc);
        delete m_threadPoolMutex;
        m_threadPoolMutex = NULL;
        return;
    }

    m_dedicatedThreads = new (std::nothrow) DedicatedThreadPool();
    if(m_dedicatedThreads == NULL) {
        LOG_ERROR("Out of memory");
        return;
    }

    m_generalThreads = new (std::nothrow) GeneralThreadPool();
    if(m_generalThreads == NULL) {
        LOG_ERROR("Out of memory");
        return;
    }

    m_initSuccess = true;
}

bool SyncConfigThreadPool::CheckInitSuccess() { return m_initSuccess; }

SyncConfigThreadPool::~SyncConfigThreadPool()
{
    // If we try and destroy the SyncConfigThreadPool before the task is complete,
    // an error message will be printed, and the memory associated with any ongoing
    // detachable threads will leak.  A crash should never happen.
    bool genThreadsAllStopped = true;
    bool dediThreadsAllStopped = true;
    RefCountToDelete* refCountToDelete = NULL;
    if(m_initSuccess) {
        int rc = CheckAndPrepareShutdown();
        if(rc != 0) {
            LOG_ERROR("CheckAndPrepareShutdown:%d", rc);
        }
        VPLThread_Yield();

        MutexAutoLock lock(m_threadPoolMutex);
        if(m_generalThreads->size() > 0) {
            LOG_INFO("Destroying threadPool while there's still "FMTu_size_t" general threads. "
                     "Using refcounts to delete.",
                      m_generalThreads->size());
            genThreadsAllStopped = false;
            if (refCountToDelete == NULL) {
                refCountToDelete = new (std::nothrow) RefCountToDelete();
                if (refCountToDelete == NULL) {
                    LOG_ERROR("Allocation failed; leaking mutex and general threads.");
                } else {
                    refCountToDelete->threadPoolMutex = m_threadPoolMutex;
                }
            }
            if (refCountToDelete != NULL) {
                refCountToDelete->generalThreads = m_generalThreads;
            }
            for(GeneralThreadPool::iterator genIter = m_generalThreads->begin();
                genIter!=m_generalThreads->end();++genIter)
            {
                (*genIter)->threadCtx.threadPool = NULL;
                if (refCountToDelete != NULL) {
                    refCountToDelete->refCount++;
                }
                (*genIter)->threadCtx.refCountThreadPoolToDelete = refCountToDelete;
            }
        }
        if(m_dedicatedThreads->size() > 0) {
            LOG_INFO("Destroying threadPool while there's still "FMTu_size_t" dedicated threads. "
                     "Using refcounts to delete.",
                      m_dedicatedThreads->size());
            dediThreadsAllStopped = false;
            if (refCountToDelete == NULL) {
                refCountToDelete = new (std::nothrow) RefCountToDelete();
                if (refCountToDelete == NULL) {
                    LOG_ERROR("Allocation failed; leaking mutex and dedicated threads.");
                } else {
                    refCountToDelete->threadPoolMutex = m_threadPoolMutex;
                }
            }
            if (refCountToDelete != NULL) {
                refCountToDelete->dedicatedThreads = m_dedicatedThreads;
            }
            for(DedicatedThreadPool::iterator dediIter = m_dedicatedThreads->begin();
                dediIter!=m_dedicatedThreads->end();++dediIter)
            {
                (*dediIter)->threadCtx.threadPool = NULL;
                if (refCountToDelete != NULL) {
                    refCountToDelete->refCount++;
                }
                (*dediIter)->threadCtx.refCountThreadPoolToDelete = refCountToDelete;
            }
        }
    }

    if(genThreadsAllStopped && m_generalThreads) {
        delete m_generalThreads;
    }
    if(dediThreadsAllStopped && m_dedicatedThreads) {
        delete m_dedicatedThreads;
    }

    if(genThreadsAllStopped && dediThreadsAllStopped)
    {   // Can free mutex if threads are not going to be leaked.  If they are,
        // the mutex is still needed.
        if(m_threadPoolMutex) {
            int rc = VPLMutex_Destroy(m_threadPoolMutex);
            if(rc != VPL_OK) {
                LOG_ERROR("VPLMutex_Destroy:%d", rc);
            }
            delete m_threadPoolMutex;
        }
    }
}

int SyncConfigThreadPool::AddGeneralThreads(u32 numThreads)
{
    int rv = 0;

    LOG_INFO("Adding %d threads to general thread pool.", numThreads);

    MutexAutoLock lock(m_threadPoolMutex);
    if(m_shutdown) {
        LOG_ERROR("Already shutdown.");
        return SYNC_CONFIG_ERR_SHUTDOWN;
    }

    PoolThread* poolThread;
    for(;numThreads != 0;--numThreads)
    {
        int rc;

        // Initialize PoolThreadCtx
        poolThread = new (std::nothrow) PoolThread();
        if (poolThread == NULL) {
            rv = UTIL_ERR_NO_MEM;
            goto error_pool_thread;
        }
        poolThread->threadCtx.inUse = false;
        poolThread->threadCtx.dedicatedNotification = NULL;
        poolThread->threadCtx.threadPoolMutex = m_threadPoolMutex;
        poolThread->threadCtx.threadPool = this;
        poolThread->threadCtx.task = NULL;
        poolThread->threadCtx.taskCtx = NULL;
        rc = VPLSem_Init(&poolThread->threadCtx.startTaskSem,
                         1,
                         0);
        if(rc != 0) {
            LOG_ERROR("VPLSem_Init:%d for thread request:%d", rc, numThreads);
            rv = rc;
            goto error_sem_init;
        }
        poolThread->threadCtx.exitThread = false;
        poolThread->threadCtx.taskInfo = 0;

        // Initialize thread
        rc = Util_SpawnThread(threadRoutine,
                              &poolThread->threadCtx,
                              UTIL_DEFAULT_THREAD_STACK_SIZE,
                              VPL_FALSE,
                              &poolThread->thread);
        if(rc != 0) {
            LOG_ERROR("Util_SpawnThread:%d", rc);
            rv = rc;
            goto error_thread_create;
        }

        m_generalThreads->push_back(poolThread);
    }
    return 0;

 error_thread_create:
    {
        int rc = VPLSem_Destroy(&poolThread->threadCtx.startTaskSem);
        if(rc != 0) {
            LOG_ERROR("VPLSem_Destroy:%d", rc);
        }
    }
 error_sem_init:
    delete poolThread;
 error_pool_thread:
    return rv;
}

VPLTHREAD_FN_DECL SyncConfigThreadPool::threadRoutine(void* ctx)
{
    PoolThreadCtx* threadCtx = (PoolThreadCtx*)ctx;
    RefCountToDelete* refCountFinalThreadCleanup = NULL;

    {
        MutexAutoLock lock(threadCtx->threadPoolMutex);
        LOG_INFO("Starting thread:%p", threadCtx);

        while (threadCtx->threadPool!=NULL && !threadCtx->exitThread)
        {
            int rc;

            {   // Most of time in while-loop should be spent here

                // Perform the wait and task without holding the mutex.
                lock.UnlockNow();
                // Be sure to always reacquire the mutex afterward.
                ON_BLOCK_EXIT_OBJ(lock, &MutexAutoLock::Relock, threadCtx->threadPoolMutex);

                rc = VPLSem_Wait(&threadCtx->startTaskSem);
                if(rc != 0) {
                    LOG_ERROR("VPLSem_Wait:%d", rc);
                    break;
                }

                if(threadCtx->task)
                {
                    // PERFORM TASK! Tasks, once accepted, can never be skipped
                    threadCtx->task(threadCtx->taskCtx);
                }
                else {
                    LOG_INFO("NULL task for thread %p, maybe just exiting", threadCtx);
                }

            } // ON_BLOCK_EXIT_OBJ relocks threadCtx->threadPoolMutex

            // Task is done, clear task state
            threadCtx->inUse = false;
            threadCtx->task = NULL;
            threadCtx->taskCtx = NULL;
            threadCtx->taskInfo = 0;

            if (threadCtx->threadPool==NULL || threadCtx->exitThread)
            {  // Skip notifications if this thread is going down.
                break;
            }

            // Notify thread pool that a thread is available
            if (threadCtx->dedicatedNotification)
            {   // Notifies only dedicated tasks, thread is reserved.
                rc = VPLSem_Post(threadCtx->dedicatedNotification);
                if (rc != 0 && rc != VPL_ERR_MAX) {
                    LOG_ERROR("VPLSem_Post:%d, handle:%p",
                              rc, threadCtx->dedicatedNotification);
                }
            } else
            {   // General thread pool
                // Notifies both dedicated tasks and general tasks because this
                // thread is available for both.
                // Dedicated threads get first priority even in the general pool.
                DedicatedThreadPool* dediThreads = threadCtx->threadPool->m_dedicatedThreads;
                DedicatedThreadPool::iterator dedicatedIter = dediThreads->begin();
                for(;dedicatedIter != dediThreads->end(); ++dedicatedIter)
                {
                    rc = VPLSem_Post((*dedicatedIter)->threadCtx.dedicatedNotification);
                    if(rc != 0 && rc != VPL_ERR_MAX) {
                        LOG_ERROR("Dedicated Thread Notification(%p):%d",
                                  (*dedicatedIter)->threadCtx.dedicatedNotification,
                                  rc);
                    }
                }

                // TODO: Round robin fairness?
                GeneralThreadPoolNotifier::iterator genIter =
                        threadCtx->threadPool->m_poolNotifications.begin();
                for(;genIter != threadCtx->threadPool->m_poolNotifications.end();
                    ++genIter)
                {
                    rc = VPLSem_Post(genIter->semaphore);
                    if(rc != 0 && rc != VPL_ERR_MAX) {
                        LOG_ERROR("General Thread Notification(%p):%d",
                                  genIter->semaphore, rc);
                    }
                }
            }
        } // while loop end

        ASSERT(VPLMutex_LockedSelf(threadCtx->threadPoolMutex));

        LOG_INFO("Exiting thread Begin:%p  Reasons(%p,%d), RefCount(%d)",
                 threadCtx, threadCtx->threadPool, threadCtx->exitThread,
                 threadCtx->refCountThreadPoolToDelete?threadCtx->refCountThreadPoolToDelete->refCount:0);

        if (threadCtx->refCountThreadPoolToDelete != NULL)
        {   // If refcount is enabled (once SyncConfigThreadPool is deleted),
            // check to see if this is the final instance.
            threadCtx->refCountThreadPoolToDelete->refCount--;
            if (threadCtx->refCountThreadPoolToDelete->refCount == 0) {
                // This is the final thread, remaining fields when left behind
                // when SyncConfigThreadPool was deleted.
                refCountFinalThreadCleanup = threadCtx->refCountThreadPoolToDelete;
            }
        } else {
            ASSERT(threadCtx->threadPool != NULL);
            // Remove dedicated thread state when it ends.  (threadCtx will become invalid)
            if (threadCtx->dedicatedNotification) {
                if(threadCtx->threadPool->m_dedicatedThreads)
                {
                    for(DedicatedThreadPool::iterator dediThreadIter =
                            threadCtx->threadPool->m_dedicatedThreads->begin();
                        dediThreadIter!=threadCtx->threadPool->m_dedicatedThreads->end();
                        ++dediThreadIter)
                    {
                        PoolThread* dediThread = *dediThreadIter;
                        if(threadCtx->dedicatedNotification ==
                                dediThread->threadCtx.dedicatedNotification)
                        {
                            int rc = VPLSem_Destroy(&threadCtx->startTaskSem);
                            if (rc != 0) {
                                LOG_ERROR("VPLSem_Destroy:%d", rc);
                            }

                            LOG_INFO("Thread state dedicated (%p) deleted", threadCtx);
                            threadCtx->threadPool->m_dedicatedThreads->erase(dediThreadIter);
                            delete dediThread;
                            break;
                        }
                    }
                }
            } else  // This thread is a general thread.
            {   // Clean up general thread state when it ends.
                if(threadCtx->threadPool->m_generalThreads)
                {
                    for(GeneralThreadPool::iterator generalThreadIter =
                            threadCtx->threadPool->m_generalThreads->begin();
                        generalThreadIter!=threadCtx->threadPool->m_generalThreads->end();
                        ++generalThreadIter)
                    {
                        PoolThread* genThread = *generalThreadIter;
                        if(threadCtx == &genThread->threadCtx)
                        {
                            int rc;
                            rc = VPLSem_Destroy(&threadCtx->startTaskSem);
                            if (rc != 0) {
                                LOG_ERROR("VPLSem_Destroy:%d", rc);
                            }

                            LOG_INFO("Thread state general (%p) delete", threadCtx);
                            threadCtx->threadPool->m_generalThreads->erase(generalThreadIter);
                            delete genThread;
                            break;
                        }
                    }
                }
            }
            // threadCtx is invalid starting here.
        }
    } // threadCtx->threadPoolMutex Lock end

    if (refCountFinalThreadCleanup != NULL)
    {  // This is the final thread, do some final cleanup.
        LOG_INFO("RefCount(0), final thread(%p) in ThreadPool cleanup",
                 refCountFinalThreadCleanup->generalThreads);
        if (refCountFinalThreadCleanup->generalThreads != NULL)
        {
            for(GeneralThreadPool::iterator generalThreadIter =
                    refCountFinalThreadCleanup->generalThreads->begin();
                generalThreadIter!=refCountFinalThreadCleanup->generalThreads->end();
                ++generalThreadIter)
            {
                PoolThread* genThread = *generalThreadIter;
                int rc;
                rc = VPLSem_Destroy(&genThread->threadCtx.startTaskSem);
                if (rc != 0) {
                    LOG_ERROR("VPLSem_Destroy:%d", rc);
                }
                LOG_INFO("Thread state general (%p) delete", &genThread->threadCtx);
                delete genThread;
            }
            delete refCountFinalThreadCleanup->generalThreads;
        }
        if (refCountFinalThreadCleanup->dedicatedThreads != NULL)
        {
            for(DedicatedThreadPool::iterator dediThreadIter =
                    refCountFinalThreadCleanup->dedicatedThreads->begin();
                dediThreadIter!=refCountFinalThreadCleanup->dedicatedThreads->end();
                ++dediThreadIter)
            {
                PoolThread* dediThread = *dediThreadIter;
                int rc = VPLSem_Destroy(&dediThread->threadCtx.startTaskSem);
                if (rc != 0) {
                    LOG_ERROR("VPLSem_Destroy:%d", rc);
                }

                LOG_INFO("Thread state dedicated (%p) deleted", &dediThread->threadCtx);
                delete dediThread;
            }
            delete refCountFinalThreadCleanup->dedicatedThreads;
        }
        int rc = VPLMutex_Destroy(refCountFinalThreadCleanup->threadPoolMutex);
        if (rc != 0) {
            LOG_ERROR("VPLMutex_Destroy:%d", rc);
        }
        delete refCountFinalThreadCleanup->threadPoolMutex;
        delete refCountFinalThreadCleanup;
    }

    // threadCtx ptr should be already freed, but can still print out the value.
    LOG_INFO("Exiting thread Done:%p", threadCtx);
    return VPLTHREAD_RETURN_VALUE;
}

bool SyncConfigThreadPool::handleExists(VPLSem_t* semaphore_in)
{
    for(GeneralThreadPoolNotifier::iterator genIter = m_poolNotifications.begin();
        genIter!=m_poolNotifications.end();++genIter)
    {
        if(genIter->semaphore == semaphore_in) {
            LOG_ERROR("Found %p in general pool", semaphore_in);
            return true;
        }
    }

    for(DedicatedThreadPool::iterator dediIter = m_dedicatedThreads->begin();
        dediIter!=m_dedicatedThreads->end();++dediIter)
    {
        if((*dediIter)->threadCtx.dedicatedNotification == semaphore_in) {
            LOG_ERROR("Found %p in dedicated thread pool", semaphore_in);
            return true;
        }
    }
    return false;
}

int SyncConfigThreadPool::RegisterForTaskCompleteNotification(VPLSem_t* semaphore_in)
{
    MutexAutoLock lock(m_threadPoolMutex);
    if(m_shutdown) {
        LOG_ERROR("Already shutdown.");
        return SYNC_CONFIG_ERR_SHUTDOWN;
    }
    if(handleExists(semaphore_in)) {
        LOG_ERROR("Handle already exists:%p", semaphore_in);
        return SYNC_CONFIG_ERR_ALREADY;
    }

    PoolNotification poolNotification;
    poolNotification.semaphore = semaphore_in;
    m_poolNotifications.push_back(poolNotification);
    return 0;
}

int SyncConfigThreadPool::RegisterForTaskCompleteNotificationAndAddDedicatedThread(
        VPLSem_t* semaphore_in)
{
    int rv = 0;

    MutexAutoLock lock(m_threadPoolMutex);
    if(m_shutdown) {
        LOG_ERROR("Already shutdown.");
        return SYNC_CONFIG_ERR_SHUTDOWN;
    }
    if(handleExists(semaphore_in)) {
        LOG_ERROR("Handle already exists:%p", semaphore_in);
        return SYNC_CONFIG_ERR_ALREADY;
    }

    LOG_INFO("Adding dedicated thread(%p).", semaphore_in);
    PoolThread* poolThread = new PoolThread();
    {
        int rc;

        // Initialize poolThread
        poolThread->threadCtx.inUse = false;
        poolThread->threadCtx.dedicatedNotification = semaphore_in;
        poolThread->threadCtx.threadPoolMutex = m_threadPoolMutex;
        poolThread->threadCtx.threadPool = this;
        poolThread->threadCtx.task = NULL;
        poolThread->threadCtx.taskCtx = NULL;
        rc = VPLSem_Init(&poolThread->threadCtx.startTaskSem,
                         1,
                         0);
        if(rc != 0) {
            LOG_ERROR("VPLSem_Init:%d for thread request(%p)", rc, semaphore_in);
            rv = rc;
            goto error_sem_init;
        }
        poolThread->threadCtx.exitThread = false;
        poolThread->threadCtx.taskInfo = 0;

        // Initialize thread
        rc = Util_SpawnThread(threadRoutine,
                              &poolThread->threadCtx,
                              UTIL_DEFAULT_THREAD_STACK_SIZE,
                              VPL_FALSE,
                              &poolThread->thread);
        if(rc != 0) {
            LOG_ERROR("Util_SpawnThread:%d", rc);
            rv = rc;
            goto error_thread_create;
        }

        m_dedicatedThreads->push_back(poolThread);
    }
    return 0;

 error_thread_create:
    {
        int rc = VPLSem_Destroy(&poolThread->threadCtx.startTaskSem);
        if(rc != 0) {
            LOG_ERROR("VPLSem_Destroy:%d", rc);
        }
    }
 error_sem_init:
    delete poolThread;
    return rv;
}

int SyncConfigThreadPool::UnregisterForTaskCompleteNotification(VPLSem_t* semaphore_handle)
{
    MutexAutoLock lock(m_threadPoolMutex);
    if(m_shutdown) {
        LOG_ERROR("Already shutdown.");
        return SYNC_CONFIG_ERR_SHUTDOWN;
    }

    for(GeneralThreadPoolNotifier::iterator genIter = m_poolNotifications.begin();
        genIter!=m_poolNotifications.end();++genIter)
    {
        if(genIter->semaphore == semaphore_handle) {
            int rc = VPLSem_Post(semaphore_handle);
            if(rc != 0 && rc != VPL_ERR_MAX) {
                LOG_ERROR("VPLSem_Post:%d", rc);
            }
            m_poolNotifications.erase(genIter);
            return 0;
        }
    }

    for(DedicatedThreadPool::iterator dediIter = m_dedicatedThreads->begin();
        dediIter!=m_dedicatedThreads->end();++dediIter)
    {
        if((*dediIter)->threadCtx.dedicatedNotification == semaphore_handle) {
            (*dediIter)->threadCtx.exitThread = true;
            int rc = VPLSem_Post(&(*dediIter)->threadCtx.startTaskSem);
            if(rc != 0 && rc != VPL_ERR_MAX) {
                LOG_ERROR("VPLSem_Post:%d", rc);
            }
            return 0;
        }
    }
    return SYNC_CONFIG_ERR_NOT_FOUND;
}

int SyncConfigThreadPool::addGeneralPoolTask(SCThreadPoolTaskFunction* taskFunction,
                                             void* taskCtx,
                                             u64 dbgTaskInfo)
{

    for(GeneralThreadPool::iterator genIter = m_generalThreads->begin();
        genIter!=m_generalThreads->end();++genIter)
    {
        PoolThread* poolThread = *genIter;
        if(!poolThread->threadCtx.inUse && !poolThread->threadCtx.exitThread)
        {   // Add the task
            poolThread->threadCtx.inUse = true;
            poolThread->threadCtx.task = taskFunction;
            poolThread->threadCtx.taskCtx = taskCtx;
            poolThread->threadCtx.taskInfo = dbgTaskInfo;

            int rc;
            rc = VPLSem_Post(&poolThread->threadCtx.startTaskSem);
            if(rc != 0 && rc != VPL_ERR_MAX) {
                LOG_ERROR("VPLSem_Post:%d, taskInfo="FMTu64". Continuing, pool thread stuck?",
                          rc, dbgTaskInfo);
            }
            return 0;
        }
    }

    return SYNC_CONFIG_STATUS_NO_AVAIL_THREAD;
}

int SyncConfigThreadPool::AddTask(SCThreadPoolTaskFunction* taskFunction,
                                  void* taskCtx,
                                  u64 dbgTaskInfo)
{
    int rc;
    MutexAutoLock lock(m_threadPoolMutex);
    if(m_shutdown) {
        LOG_ERROR("Already shutdown.");
        return SYNC_CONFIG_ERR_SHUTDOWN;
    }
    rc = addGeneralPoolTask(taskFunction,
                            taskCtx,
                            dbgTaskInfo);
    return rc;
}

int SyncConfigThreadPool::AddTaskDedicatedThread(VPLSem_t* semaphore_handle,
                                                 SCThreadPoolTaskFunction* taskFunction,
                                                 void* taskCtx,
                                                 u64 dbgTaskInfo)
{
    int rc;
    MutexAutoLock lock(m_threadPoolMutex);
    if(m_shutdown) {
        LOG_ERROR("Already shutdown.");
        return SYNC_CONFIG_ERR_SHUTDOWN;
    }

    // Look for dedicated thread
    for(DedicatedThreadPool::iterator dediIter = m_dedicatedThreads->begin();
        dediIter!=m_dedicatedThreads->end();++dediIter)
    {
        PoolThread* dediThread = *dediIter;
        if(dediThread->threadCtx.dedicatedNotification == semaphore_handle) {
            if(dediThread->threadCtx.exitThread) {
                LOG_ERROR("Dedicated thread being shut down:(%p, "FMTu64")",
                          semaphore_handle, dbgTaskInfo);
                return SYNC_CONFIG_ERR_NOT_FOUND;
            }
            if(dediThread->threadCtx.inUse) {
                // Dedicated thread not available
                break;
            }
            dediThread->threadCtx.inUse = true;
            dediThread->threadCtx.task = taskFunction;
            dediThread->threadCtx.taskCtx = taskCtx;
            dediThread->threadCtx.taskInfo = dbgTaskInfo;
            int rc = VPLSem_Post(&dediThread->threadCtx.startTaskSem);
            if(rc != 0 && rc != VPL_ERR_MAX) {
                LOG_ERROR("VPLSem_Post:%d", rc);
            }
            // Task successfully added on dedicated thread.
            return 0;
        }
    }

    // Adding task to general pool
    rc = addGeneralPoolTask(taskFunction,
                            taskCtx,
                            dbgTaskInfo);
    return rc;
}

int SyncConfigThreadPool::CheckAndPrepareShutdown()
{
    int remainingTaskGeneral = 0;
    int remainingTaskDedicated = 0;

    MutexAutoLock lock(m_threadPoolMutex);
    LOG_INFO("Shutting down SyncConfigThreadPool");
    m_shutdown = true;

    // Tell all general threads to exit.  Count if any are still in use.
    for(GeneralThreadPool::iterator genIter = m_generalThreads->begin();
        genIter!=m_generalThreads->end(); ++genIter)
    {
        PoolThread* genThread = *genIter;
        genThread->threadCtx.exitThread = true;
        int rc = VPLSem_Post(&genThread->threadCtx.startTaskSem);
        if(rc != 0 && rc != VPL_ERR_MAX) {
            LOG_ERROR("VPLSem_Post:%d", rc);
        }
        if(genThread->threadCtx.inUse) {
            remainingTaskGeneral++;
        }
    }

    // Tell all dedicated threads to exit.  Count if any are still in use.
    for(DedicatedThreadPool::iterator dediIter = m_dedicatedThreads->begin();
        dediIter!=m_dedicatedThreads->end(); ++dediIter)
    {
        PoolThread* dediThread = *dediIter;
        dediThread->threadCtx.exitThread = true;
        int rc = VPLSem_Post(&dediThread->threadCtx.startTaskSem);
        if(rc != 0 && rc != VPL_ERR_MAX) {
            LOG_ERROR("VPLSem_Post:%d", rc);
        }
        if(dediThread->threadCtx.inUse) {
            remainingTaskDedicated++;
        }
    }

    if ((remainingTaskGeneral > 0) || (remainingTaskDedicated > 0)) {
        LOG_ERROR("Task remaining general:%d dedicated:%d)",
                  remainingTaskGeneral, remainingTaskDedicated);
        return SYNC_CONFIG_ERR_BUSY_TASK;
    }

    return 0;
}

void SyncConfigThreadPool::countGeneralPool(u32& unoccupied_threads_out,
                                            u32& total_threads_out)
{
    unoccupied_threads_out = 0;
    total_threads_out = m_generalThreads->size();

    GeneralThreadPool::iterator genIter = m_generalThreads->begin();
    for(;genIter!=m_generalThreads->end(); ++genIter)
    {
        PoolThread* genThread = *genIter;
        if(!genThread->threadCtx.inUse && !genThread->threadCtx.exitThread) {
            unoccupied_threads_out++;
        }
    }
}

void SyncConfigThreadPool::GetInfo(u32& unoccupied_threads_out,
                                   u32& total_threads_out,
                                   bool& shuttingDown_out)
{
    unoccupied_threads_out = 0;
    total_threads_out = 0;
    MutexAutoLock lock(m_threadPoolMutex);

    shuttingDown_out = m_shutdown;
    countGeneralPool(unoccupied_threads_out, total_threads_out);
}

void SyncConfigThreadPool::GetInfoIncludeDedicatedThread(const VPLSem_t* semaphore_handle,
                                                         u32& unoccupied_threads_out,
                                                         u32& total_threads_out,
                                                         bool& shuttingDown_out)
{
    unoccupied_threads_out = 0;
    total_threads_out = 0;

    MutexAutoLock lock(m_threadPoolMutex);

    shuttingDown_out = m_shutdown;
    for(DedicatedThreadPool::iterator dediIter = m_dedicatedThreads->begin();
        dediIter!=m_dedicatedThreads->end();++dediIter)
    {
        PoolThread* dediThread = *dediIter;
        if(semaphore_handle == NULL ||
           semaphore_handle == dediThread->threadCtx.dedicatedNotification)
        {
            total_threads_out++;
            if(!dediThread->threadCtx.inUse && !dediThread->threadCtx.exitThread) {
                unoccupied_threads_out++;
            }

            if(semaphore_handle != NULL) {
                break;
            }
        }
    }

    u32 generalCountUnoccupied = 0;
    u32 generalCountTotal = 0;
    countGeneralPool(/* OUT */generalCountUnoccupied,
                     /* OUT */generalCountTotal);

    unoccupied_threads_out += generalCountUnoccupied;
    total_threads_out += generalCountTotal;
}
