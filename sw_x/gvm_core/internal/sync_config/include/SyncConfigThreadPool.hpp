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
#ifndef SYNCCONFIGTHREADPOOL_HPP_06_13_2013
#define SYNCCONFIGTHREADPOOL_HPP_06_13_2013

#include "vpl_plat.h"
#include "vpl_th.h"
#include "vplu_types.h"
#include <list>
#include <vector>

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_Sync_One_Way_Parallel_Tasks

typedef void SCThreadPoolTaskFunction(void* ctx);

class SyncConfigThreadPool {
 public:
    SyncConfigThreadPool();
    ~SyncConfigThreadPool();

    // Returns true if the initialization was successful; otherwise, destroy
    // this object and quit.  A consequence of not using exceptions when constructor
    // fails.
    bool CheckInitSuccess();

    // General thread pool.
    int AddGeneralThreads(u32 numThreads);

    // The semaphore is notified whenever there is potentially a free thread
    // available to do work.
    // semaphore_in - Semaphore is created and owned by the caller, and
    //                the semaphore must be unregistered before it is destroyed.
    //                Error is returned if handle already registered.
    int RegisterForTaskCompleteNotification(VPLSem_t* semaphore_in);

    // In some cases, to ensure that work is always being done, a dedicated
    // thread can be used.  When the dedicated thread completes a task, only
    // the semaphore associated with this dedicated thread is notified.
    // semaphore_in - Semaphore is created and owned by the caller, and the
    //                semaphore must be unregistered before it is destroyed.
    //                Becomes the handle used to refer to the dedicated thread.
    //                Error is returned if handle already registered.
    int RegisterForTaskCompleteNotificationAndAddDedicatedThread(VPLSem_t* semaphore_in);

    // Removes notification.  If semaphore is associated with a dedicated
    // thread, that dedicated thread will accept no further tasks and will
    // exit once the current task (if any) completes.
    int UnregisterForTaskCompleteNotification(VPLSem_t* semaphore_handle);

    // Returns SUCCESS if task is started, else returns STATUS_NO_FREE_THREADS.
    // Only looks in the general pool of threads.
    int AddTask(SCThreadPoolTaskFunction* taskFunction,
                void* taskCtx,
                u64 dbgTaskInfo);

    // First tries to add the task to the dedicated thread, if unsuccessful,
    // next tries to add the task to the general thread pool.  If still
    // unsuccessful, returns STATUS_NO_FREE_THREADS.
    // semaphore_in - must be a semaphore pointer registered for notification
    //                with a dedicated thread, otherwise ERR_UNRECOGNIZED_HANDLE
    //                will be returned.
    int AddTaskDedicatedThread(VPLSem_t* semaphore_handle,
                               SCThreadPoolTaskFunction* taskFunction,
                               void* taskCtx,
                               u64 dbgTaskInfo);

    // This should be called right before the thread state is destroyed.
    // Modules using SyncConfigThreadPool should be responsible for ensuring
    // there are no longer any tasks before destroying the thread pool.
    // Relying on this call to stop ongoing tasks is an error in design.
    // (SYNC_CONFIG_ERR_BUSY_TASK).
    //
    // All threads are asked to exit.  Stops any further tasks from being
    // performed.  Returns success if there are no threads remaining.
    // Otherwise, there are still threads that have not exited due to
    // performing a task.
    int CheckAndPrepareShutdown();

    // Get summary of threadPool state.  Unoccupied threads and total general threads.
    // Dedicated threads are not included in these counts.
    void GetInfo(u32& unoccupied_threads_out,
                 u32& total_threads_out,
                 bool& shuttingDown_out);

    // Same as GetInfo call except includes counts from the dedicated thread
    // indicated by the semaphore handle.
    // semaphore_in - must be a pointer registered for notification with a
    //                dedicated thread; otherwise, ERR_UNRECOGNIZED_HANDLE
    //                will be returned.  Can also pass NULL to include
    //                information about all dedicated threads.
    void GetInfoIncludeDedicatedThread(const VPLSem_t* semaphore_handle,
                                       u32& unoccupied_threads_out,
                                       u32& total_threads_out,
                                       bool& shuttingDown_out);
 private:
    bool m_initSuccess;
    VPLMutex_t* m_threadPoolMutex;
    volatile bool m_shutdown;
    struct RefCountToDelete;

    struct PoolThreadCtx
    {
        bool inUse;

        // Identifies if this is a dedicated thread, NULL if not.
        VPLSem_t* dedicatedNotification;

        // Need the threadPool state to properly do notifications once task is complete.
        VPLMutex_t* threadPoolMutex;  // Pointer to m_threadPoolMutex
        SyncConfigThreadPool* threadPool;

        SCThreadPoolTaskFunction* task;
        void* taskCtx;

        VPLSem_t startTaskSem;        // Notify worker thread there's a task to do.
        volatile bool exitThread;
        u64 taskInfo;  // Minimal information written by each task
                       // (easier to identify which thread is stuck)

        // When there are still running threads upon threadpool deletion, this
        // will point to the remaining objects to delete once the ref count reaches
        // 0.  threadPool should be NULL for this field to be populated.
        RefCountToDelete* refCountThreadPoolToDelete;

        PoolThreadCtx()
        :  inUse(false),
           dedicatedNotification(NULL),
           threadPoolMutex(NULL),
           threadPool(NULL),
           task(NULL),
           taskCtx(NULL),
           exitThread(false),
           taskInfo(0),
           refCountThreadPoolToDelete(NULL)
        {}
    };

    struct PoolThread
    {
        VPLDetachableThreadHandle_t thread;
        PoolThreadCtx threadCtx;
    };

    // Registered notifications.
    struct PoolNotification
    {  // Perhaps this will contain a callback in the future.
        VPLSem_t* semaphore;
    };

    typedef std::vector<PoolThread*> GeneralThreadPool;
    GeneralThreadPool* m_generalThreads;

    typedef std::vector<PoolThread*> DedicatedThreadPool;
    DedicatedThreadPool* m_dedicatedThreads;

    typedef std::vector<PoolNotification> GeneralThreadPoolNotifier;
    GeneralThreadPoolNotifier m_poolNotifications;
    
    struct RefCountToDelete {
        int refCount;  // Delete the following 3 fields when refcount drops to 0:
        GeneralThreadPool* generalThreads;
        DedicatedThreadPool* dedicatedThreads;
        VPLMutex_t* threadPoolMutex;
        
        RefCountToDelete()
        :   refCount(0),
            generalThreads(NULL),
            dedicatedThreads(NULL),
            threadPoolMutex(NULL)
        {}
    };

    static VPLTHREAD_FN_DECL threadRoutine(void* ctx);
    bool handleExists(VPLSem_t* semaphore_in);
    int addGeneralPoolTask(SCThreadPoolTaskFunction* taskFunction,
                           void* taskCtx,
                           u64 taskInfo);
    void countGeneralPool(u32& unoccupied_threads_out,
                          u32& total_threads_out);
};

#endif // SYNCCONFIGTHREADPOOL_HPP_06_13_2013
