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
#include "FileMonitorDelayQ.hpp"

#include "gvm_errors.h"
#include "gvm_thread_utils.h"
#include "vpl_fs.h"
#include "vpl_lazy_init.h"
#include "vpl_th.h"
#include "vplex_assert.h"
#include "vplu_mutex_autolock.hpp"

#include <string>
#include <vector>
#include <log.h>

// TODO: DelayQ is currently not bounded by size, can take
// a lot of memory if events happen too often.

// Necessary static because the FileMonitor callback requires a static function.
static VPLLazyInitMutex_t g_delayQueuesMutex = VPLLAZYINITMUTEX_INIT;
static std::vector<FileMonitorDelayQ*> g_delayQueues;

void FileMonitorDelayQ::fileMonitorCallback(VPLFS_MonitorHandle handle,
                                            void* eventBuffer,
                                            int eventBufferSize,
                                            int error)
{
    bool eventEnqueued = false;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&g_delayQueuesMutex));

    std::vector<FileMonitorDelayQ*>::iterator delayQIter;
    for(delayQIter=g_delayQueues.begin();
        delayQIter!=g_delayQueues.end();
        ++delayQIter)
    {
        FileMonitorHandleMap unused;
        if((*delayQIter)->GetHandleMap(handle, unused)) {
            (*delayQIter)->EnqueueEvent(handle,
                                        eventBuffer,
                                        eventBufferSize,
                                        error);
            eventEnqueued = true;
            break;
        }
    }

    if(!eventEnqueued) {
        LOG_WARN("Event dropped");
    }
}

void FileMonitorDelayQ::EnqueueEvent(VPLFS_MonitorHandle handle,
                                     void* eventBuffer,
                                     int eventBufferSize,
                                     int error)
{

    DelayQEntry entry;
    entry.handle = handle;
    entry.timeQueued = VPLTime_GetTimeStamp();
    entry.error = error;
    entry.eventBuffer = NULL;
    if(error==VPLFS_MonitorCB_OK && eventBufferSize != 0) {
        entry.eventBuffer = malloc(eventBufferSize);
        if(entry.eventBuffer == NULL) {
            entry.error = VPLFS_MonitorCB_OVERFLOW;
            entry.eventBufferSize = 0;
        } else {
            entry.eventBufferSize = eventBufferSize;
            memcpy(entry.eventBuffer, eventBuffer, eventBufferSize);
        }
    }else{
        entry.eventBuffer = NULL;
        entry.eventBufferSize = 0;
    }
    // This buffer needs to be separately allocated because we don't want any
    // std::deque resizes to move the std::string c_str() pointers.
    entry.stringPool = new std::list<std::string>();

    MutexAutoLock lock(&m_delayQ_mutex);
    m_delayQ.push_back(entry);    // eventBuffer MUST BE FREED on removal from m_delayQ
    LOG_DEBUG("Enqueued for "FMTu64, VPLTime_ToMillisec(entry.timeQueued));

    // Need to save the strings (for posix, and doing it for win32 for consistency);
    // otherwise they will fall out of scope after this call.
    if(error == VPLFS_MonitorCB_OK) {
        int bufferIndex = 0;
        void* currEventBuf = m_delayQ.back().eventBuffer;
        while(bufferIndex<m_delayQ.back().eventBufferSize) {
            VPLFS_MonitorEvent* monitorEvent = (VPLFS_MonitorEvent*)currEventBuf;

            if(monitorEvent->filename!=NULL) {
                m_delayQ.back().stringPool->push_back(monitorEvent->filename);
                monitorEvent->filename = m_delayQ.back().stringPool->back().c_str();
            }


            if(monitorEvent->moveTo!=NULL) {
                m_delayQ.back().stringPool->push_back(monitorEvent->moveTo);
                monitorEvent->moveTo = m_delayQ.back().stringPool->back().c_str();
            }

            if(monitorEvent->nextEntryOffsetBytes==0) {
                // No more entries
                break;
            }
            currEventBuf = (u8*)currEventBuf + monitorEvent->nextEntryOffsetBytes;
            bufferIndex +=  monitorEvent->nextEntryOffsetBytes;
        }
    }

    VPLCond_Signal(&m_delayQ_cond_var);
}

bool FileMonitorDelayQ::GetHandleMap(VPLFS_MonitorHandle handle,
                                     FileMonitorHandleMap& handleMap_out)
{
    MutexAutoLock lock(&m_delayQ_mutex);
    std::vector<FileMonitorHandleMap>::iterator iter = m_handleMap.begin();
    for(;iter!=m_handleMap.end();++iter) {
        if(iter->handle == handle) {
            handleMap_out = (*iter);
            return true;
        }
    }
    return false;
}

void FileMonitorDelayQ::AddFileMonitorDelayQueue(FileMonitorDelayQ* delayQ)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&g_delayQueuesMutex));
    g_delayQueues.push_back(delayQ);
}

void FileMonitorDelayQ::RemoveFileMonitorDelayQueue(FileMonitorDelayQ* delayQ)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&g_delayQueuesMutex));
    std::vector<FileMonitorDelayQ*>::iterator delayQIter;
    for(delayQIter=g_delayQueues.begin();
        delayQIter!=g_delayQueues.end();)
    {
        if((*delayQIter)==delayQ) {
            delayQIter = g_delayQueues.erase(delayQIter);
        } else {
            ++delayQIter;
        }
    }
}


FileMonitorDelayQ::FileMonitorDelayQ(VPLTime_t delayAmount)
:  m_initCount(0),
   m_threadRunning(false),
   m_delayAmount(delayAmount)
{
    int rc;
    rc = VPLMutex_Init(&m_api_mutex);
    if (rc != 0) {
        LOG_ERROR("VPLMutex_Init failed: %d", rc);
    }
    rc = VPLCond_Init(&m_delayQ_cond_var);
    if (rc != 0) {
        LOG_ERROR("VPLCond_Init failed: %d", rc);
    }
    rc = VPLMutex_Init(&m_delayQ_mutex);
    if (rc != 0) {
        LOG_ERROR("VPLMutex_Init failed: %d", rc);
    }
}

FileMonitorDelayQ::~FileMonitorDelayQ()
{
    if (VPL_IS_INITIALIZED(&m_delayQ_cond_var)) {
        VPLCond_Destroy(&m_delayQ_cond_var);
    }
    if (VPL_IS_INITIALIZED(&m_api_mutex)) {
        VPLMutex_Destroy(&m_api_mutex);
    }
    if (VPL_IS_INITIALIZED(&m_delayQ_mutex)) {
        VPLMutex_Destroy(&m_delayQ_mutex);
    }
}

int FileMonitorDelayQ::Init()
{
    int rv;

    MutexAutoLock lock(&m_api_mutex);
    if(m_initCount>0) {
        ++m_initCount;
        LOG_INFO("Already init, refcount:%d", m_initCount);
        return 0;
    }
    m_threadRunning = true;
    rv = Util_SpawnThread(FileMonitorDelayQ_ThreadFn, this,
            UTIL_DEFAULT_THREAD_STACK_SIZE, VPL_TRUE, &m_delayQ_thread);
    if (rv != 0) {
        LOG_WARN("Util_SpawnThread failed: %d", rv);
        goto thread_create_fail;
    }
    AddFileMonitorDelayQueue(this);
    rv = VPLFS_MonitorInit();
    if(rv != 0) {
        LOG_ERROR("VPLFS_MonitorInit:%d", rv);
        goto monitor_init_fail;
    }
    ++m_initCount;
    LOG_INFO("Init success");
    return 0;

 monitor_init_fail:
    {
        MutexAutoLock lock(&m_delayQ_mutex);
        m_threadRunning = false;
        VPLCond_Signal(&m_delayQ_cond_var);
    }
 thread_create_fail:
    m_threadRunning = false;
    return rv;
}

int FileMonitorDelayQ::Shutdown()
{
    int rc;
    MutexAutoLock lock(&m_api_mutex);
    if(m_initCount==0) {
        LOG_ERROR("Already shutdown");
        return CCD_ERROR_NOT_INIT;
    }
    --m_initCount;
    if(m_initCount > 0) {
        LOG_INFO("Not ready to shutdown, refcount:%d", m_initCount);
        return 0;
    }

    ASSERT(m_initCount==0);
    RemoveFileMonitorDelayQueue(this);
    {
        MutexAutoLock lock(&m_delayQ_mutex);
        m_threadRunning = false;
        for(std::vector<FileMonitorHandleMap>::iterator iter = m_handleMap.begin();
            iter != m_handleMap.end();)
        {
            rc = VPLFS_MonitorDirStop(iter->handle);
            if(rc != 0) {
                LOG_ERROR("VPLFS_MonitorDirStop:%d", rc);
            }
            iter = m_handleMap.erase(iter);
        }
        VPLCond_Signal(&m_delayQ_cond_var);
    }

    m_handleMap.clear();

    rc = VPLDetachableThread_Join(&m_delayQ_thread);
    if(rc != 0) {
        LOG_ERROR("VPLDetachableThread_Join:%d", rc);
    }

    std::deque<DelayQEntry>::iterator dIter = m_delayQ.begin();
    while(dIter != m_delayQ.end())
    {
        free(dIter->eventBuffer);
        dIter = m_delayQ.erase(dIter);
    }

    rc = VPLFS_MonitorDestroy();
    if(rc != 0) {
        LOG_ERROR("VPLFS_MonitorDestroy:%d", rc);
    }
    LOG_INFO("FileMonitorDelayQ Shutdown Complete");
    return 0;
}

int FileMonitorDelayQ::AddMonitor(const std::string& directory,
                                  int num_events_internal,
                                  FileMonitorDelayQCallback cb,
                                  void* context,
                                  VPLFS_MonitorHandle* handle_out)
{
    MutexAutoLock lock(&m_api_mutex);
    if(m_initCount==0) {
        return CCD_ERROR_NOT_INIT;
    }
    MutexAutoLock lock2(&m_delayQ_mutex);
    int rc;
    rc = VPLFS_MonitorDir(directory.c_str(),
                          num_events_internal,
                          fileMonitorCallback,
                          handle_out);
    if(rc != 0) {
        LOG_ERROR("VPLFS_MonitorDir:%d, %s", rc, directory.c_str());
        return rc;
    }

    FileMonitorHandleMap handleMap;
    handleMap.handle = *handle_out;
    handleMap.cb = cb;
    handleMap.cb_ctx = context;
    m_handleMap.push_back(handleMap);
    return 0;
}

int FileMonitorDelayQ::RemoveMonitor(VPLFS_MonitorHandle handle)
{
    MutexAutoLock lock(&m_api_mutex);
    if(m_initCount==0) {
        return CCD_ERROR_NOT_INIT;
    }
    int rc = removeMonitor(handle);
    if(rc != 0) {
        LOG_ERROR("removeMonitor:%d", rc);
    }
    return rc;
}

int FileMonitorDelayQ::removeMonitor(VPLFS_MonitorHandle handle)
{
    MutexAutoLock lock(&m_delayQ_mutex);
    int rc = VPLFS_MonitorDirStop(handle);
    if(rc != 0) {
        LOG_ERROR("VPLFS_MonitorDirStop:%d", rc);
    }
    for(std::vector<FileMonitorHandleMap>::iterator iter = m_handleMap.begin();
        iter != m_handleMap.end();)
    {
        if(iter->handle == handle) {
            iter = m_handleMap.erase(iter);
        }else{
            ++iter;
        }
    }
    return rc;
}

VPLTHREAD_FN_DECL FileMonitorDelayQ::FileMonitorDelayQ_ThreadFn(void* arg)
{
    FileMonitorDelayQ* self = (FileMonitorDelayQ*) arg;
    self->delayQLoop();
    LOG_INFO("FileMonitorDelayQ thread exiting");
    return VPLTHREAD_RETURN_VALUE;
}

static void cleanupDelayQEntry(DelayQEntry& entry)
{
    free(entry.eventBuffer);
    entry.eventBuffer = NULL;
    delete entry.stringPool;
    entry.stringPool = NULL;
}

void FileMonitorDelayQ::delayQLoop()
{
    VPLMutex_Lock(&m_delayQ_mutex);
    VPLTime_t currTime = VPLTime_GetTimeStamp();

    while (true) {

        ASSERT(!VPLMutex_LockedSelf(&m_api_mutex));
        // call callbacks
        while(hasWork(currTime)) {
            ASSERT(!m_delayQ.empty());

            FileMonitorDelayQCallback callback = NULL;
            void* context = NULL;
            FileMonitorHandleMap handleMap_out;

            DelayQEntry & entry = m_delayQ.front();
            if(GetHandleMap(m_delayQ.front().handle, handleMap_out)) {
                callback = handleMap_out.cb;
                context = handleMap_out.cb_ctx;
            } // else, callback will remain null and callback call skipped.
            VPLMutex_Unlock(&m_delayQ_mutex);

            if(callback) {
                callback(entry.handle,
                         entry.eventBuffer,
                         entry.eventBufferSize,
                         entry.error,
                         context);
            }

            VPLMutex_Lock(&m_delayQ_mutex);
            if(!m_delayQ.empty()) {
                cleanupDelayQEntry(m_delayQ.front());
                m_delayQ.pop_front();
            }
            if(!m_threadRunning) {
                goto exit;
            }
        }

        currTime = VPLTime_GetTimeStamp();
        // Wait for work to do.
        while (!hasWork(currTime))
        {
            VPLTime_t timeout = VPL_TIMEOUT_NONE;
            if(!m_delayQ.empty()) {
                VPLTime_t timeAlreadyWaited = VPLTime_DiffClamp(currTime, m_delayQ.front().timeQueued);
                timeout = VPLTime_DiffClamp(m_delayAmount, timeAlreadyWaited);
            }
            LOG_DEBUG("Sleeping for "FMTu64"ms", VPLTime_ToMillisec(timeout));
            int rc = VPLCond_TimedWait(&m_delayQ_cond_var, &m_delayQ_mutex, timeout);
            if ((rc != 0) && (rc != VPL_ERR_TIMEOUT)) {
                LOG_WARN("VPLCond_TimedWait failed: %d", rc);
            }
            if(!m_threadRunning) {
                goto exit;
            }
            currTime = VPLTime_GetTimeStamp();
        }
    } // while (1)
 exit:
    {
        // Could have exited with events still in the queue.
        for(std::deque<DelayQEntry>::iterator iter = m_delayQ.begin();
            iter != m_delayQ.end(); ++iter)
        {
            cleanupDelayQEntry(*iter);
        }
        m_delayQ.clear();
    }
    VPLMutex_Unlock(&m_delayQ_mutex);
}

bool FileMonitorDelayQ::hasWork(VPLTime_t currTime)
{
    if(m_delayQ.empty()) {
        return false;
    }
    if(VPLTime_DiffClamp(currTime, m_delayQ.front().timeQueued) >= m_delayAmount)
    {
        return true;
    }
    return false;
}
