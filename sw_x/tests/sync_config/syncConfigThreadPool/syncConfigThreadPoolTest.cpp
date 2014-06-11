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
#include "vpl_th.h"
#include "vplu_format.h"
#include "vplu_mutex_autolock.hpp"
#include "vplu_types.h"
#include <list>
#include "log.h"

struct CountState
{
    u32 thread_id;
    u64 startingNum;
    u64 increment;
    VPLMutex_t* mutex;
    std::list<CountState>* countStates;
};

struct TestThreadState
{
    SyncConfigThreadPool * threadPool;
    VPLDetachableThreadHandle_t thread;
    bool dedicated;
    u32 thread_id;
    u64 startingNum;
    u64 max;

    TestThreadState()
    :  threadPool(NULL),
       dedicated(false),
       thread_id(0),
       startingNum(0),
       max(0)
    {}
};

static void myTaskFunc(void* ctx)
{
    CountState* countState = (CountState*) ctx;
    u64 origStarting = countState->startingNum;
    u64 currNum = countState->startingNum;
    u64 incUpTo = currNum + countState->increment;
    for(; currNum < incUpTo; currNum++)
    {
        LOG_INFO("ThreadId:%d  Num:"FMTu64, countState->thread_id, currNum);
    }

    MutexAutoLock lock(countState->mutex);
    for(std::list<CountState>::iterator iter = countState->countStates->begin();
        iter != countState->countStates->end(); ++iter)
    {
        if(iter->startingNum == origStarting) {
            LOG_INFO("Mini Task Done: (%d,"FMTu64"): Ongoing (including this task):%d",
                     countState->thread_id, currNum, countState->countStates->size());
            countState->countStates->erase(iter);
            break;
        }
    }
}

static VPLDetachableThread__returnType_t myThreadFunc(void* ctx)
{
    // Subdivde the count state into tasks of increment.
    TestThreadState* threadState = (TestThreadState*) ctx;
    VPLSem_t taskNotifier;
    VPLMutex_t countStateMutex;
    int rc;

    // 1. Initialization
    rc = VPLSem_Init(&taskNotifier, 1, 0);
    if(rc != 0) {
        LOG_ERROR("VPLSem_Init:%d, ThreadState:%d", rc, threadState->thread_id);
    }

    rc = VPLMutex_Init(&countStateMutex);
    if(rc != 0) {
        LOG_ERROR("VPLMutex_Init:%d, ThreadState:%d", rc, threadState->thread_id);
    }

    if(threadState->dedicated) {
        LOG_INFO("Thread:%d is dedicated", threadState->thread_id);
        threadState->threadPool->RegisterForTaskCompleteNotificationAndAddDedicatedThread(&taskNotifier);
    } else {  // general
        LOG_INFO("Thread:%d is in generalpool", threadState->thread_id);
        threadState->threadPool->RegisterForTaskCompleteNotification(&taskNotifier);
    }

    // 2. Doing work
    std::list<CountState> countStates;
    while(threadState->startingNum < threadState->max) {
        const u64 miniTaskInc = 100;
        for(; threadState->startingNum < threadState->max;
            threadState->startingNum += miniTaskInc)
        {
            CountState countState;
            countState.thread_id = threadState->thread_id;
            countState.startingNum = threadState->startingNum;
            countState.increment  = miniTaskInc;
            countState.mutex = &countStateMutex;
            countState.countStates = &countStates;

            MutexAutoLock lock(&countStateMutex);
            countStates.push_back(countState);
            if(threadState->dedicated) {
                rc = threadState->threadPool->AddTaskDedicatedThread(&taskNotifier,
                                                                     myTaskFunc,
                                                                     &countStates.back(),
                                                                     threadState->thread_id);
            }else{
                rc = threadState->threadPool->AddTask(myTaskFunc,
                                                      &countStates.back(),
                                                      threadState->thread_id);
            }
            if(rc != 0) {
                LOG_INFO("Try Task Later (%d,"FMTu64"): Already:%d",
                         threadState->thread_id, threadState->startingNum, countStates.size());
                countStates.pop_back();
                break;
            }

            // Task successfully added!
            LOG_INFO("Mini Task Start: (%d,"FMTu64"): Already:%d",
                     threadState->thread_id, threadState->startingNum, countStates.size());
        }

        VPLSem_Wait(&taskNotifier);
    }

    // 3. Let's not destroy state from underneath the tasks
    //    over-conservative: wait until all tasks are complete.
    LOG_INFO("Thread:%d  Waiting for %d tasks to complete.",
             threadState->thread_id, countStates.size());
    {
        u32 totalThreads = 0;
        u32 unOccupiedThreads = 0;
        bool shuttingDown = false;

        while(!shuttingDown ||
              (shuttingDown && unOccupiedThreads != totalThreads))
        {

            if(countStates.size() == 0) {
                LOG_INFO("Thread:%d  All tasks done.", threadState->thread_id);
                break;
            }
            LOG_INFO("Thread:%d  Still %d tasks",
                     threadState->thread_id, countStates.size());

            VPLSem_Wait(&taskNotifier);

            if(threadState->dedicated) {
                threadState->threadPool->
                    GetInfoIncludeDedicatedThread(&taskNotifier,
                                                  /*OUT*/ unOccupiedThreads,
                                                  /*OUT*/ totalThreads,
                                                  /*OUT*/ shuttingDown);
            }else{
                threadState->threadPool->GetInfo(/*OUT*/ unOccupiedThreads,
                                                 /*OUT*/ totalThreads,
                                                 /*OUT*/ shuttingDown);
            }
        }
    }

    // 4. Cleanup
    rc = threadState->threadPool->UnregisterForTaskCompleteNotification(&taskNotifier);
    if(rc != 0) {
        LOG_ERROR("Unregister:%d, threadState:%d", rc, threadState->thread_id);
    }

    rc = VPLMutex_Destroy(&countStateMutex);
    if(rc != 0) {
        LOG_ERROR("VPLMutex_Destroy:%d, threadState:%d", rc, threadState->thread_id);
    }

    rc = VPLSem_Destroy(&taskNotifier);
    if(rc != 0) {
        LOG_ERROR("VPLSem_Destroy:%d, threadState:%d", rc, threadState->thread_id);
    }
    LOG_INFO("Ending Thread:%d", threadState->thread_id);

    return 0;
}

static void printThreadStats(SyncConfigThreadPool& myThreadPool)
{
    u32 unoccupiedThreads = 0;
    u32 totalThreads = 0;
    bool shuttingDown = false;
    myThreadPool.GetInfo(/*OUT*/ unoccupiedThreads,
                         /*OUT*/ totalThreads,
                         /*OUT*/ shuttingDown);
    LOG_INFO("General: unoccupied:"FMTu32" total:"FMTu32", %d",
             unoccupiedThreads, totalThreads, shuttingDown);
    myThreadPool.GetInfoIncludeDedicatedThread(NULL,
                                               /*OUT*/ unoccupiedThreads,
                                               /*OUT*/ totalThreads,
                                               /*OUT*/ shuttingDown);
    LOG_INFO("Dedicated: unoccupied:"FMTu32" total:"FMTu32", %d",
             unoccupiedThreads, totalThreads, shuttingDown);
}

// If TEST_EARLY_SHUTDOWN enabled, should leak memory, not crash.
#define TEST_EARLY_SHUTDOWN 0

int main(int argc, char** argv)
{
    int rc;
    int rv = 0;
    std::vector<TestThreadState*> threadStates;
    {
        SyncConfigThreadPool myThreadPool;
        if(!myThreadPool.CheckInitSuccess()) {
            LOG_ERROR("CheckInitSuccess");
            return -1;
        }
        const int NUM_GENERAL_THREADS = 3;
        LOG_INFO("Starting ThreadPool with %d general threads", NUM_GENERAL_THREADS);
        rc = myThreadPool.AddGeneralThreads(3);
        if(rc != 0) {
            LOG_ERROR("AddGeneralThreads:%d", rc);
            return rc;
        }

        const int NUM_TEST_THREADS = 8;
        for(int i = 0; i<NUM_TEST_THREADS; i++)
        {
            TestThreadState* threadState = new TestThreadState();
            threadState->startingNum = (i+1)*100000;
            threadState->thread_id = (i+1);
            threadState->max = threadState->startingNum + 1000;  // have each thread count 10k
            threadState->threadPool = &myThreadPool;
            threadState->dedicated = false;

            if(threadState->thread_id==7 || threadState->thread_id==6)
            {
                threadState->dedicated = true;
                LOG_INFO("Setting thread_id:%d to be dedicated thread",
                         threadState->thread_id);
            }

            // Initialize thread
            rc = VPLDetachableThread_Create(&threadState->thread,
                                            myThreadFunc,
                                            threadState,
                                            NULL,
                                            "testth");
            if(rc != 0) {
                LOG_ERROR("VPLDetachableThread_Create:%d", rc);
                rv = rc;
            }

            rc = VPLDetachableThread_Detach(&threadState->thread);
            if(rc != 0) {
                LOG_ERROR("VPLDetachableThread_Detach:%d", rc);
                // TODO: What to do?
                rv = rc;
            }

            threadStates.push_back(threadState);
        }

        printThreadStats(myThreadPool);

#if TEST_EARLY_SHUTDOWN
        VPLThread_Sleep(VPLTime_FromMillisec(300));
        rc = myThreadPool.CheckAndPrepareShutdown();
        if(rc != 0) {
            LOG_WARN("Early shutdown:%d, expecting thread leaks", rc);
        }
        // ThreadPool is deleted here
    }
#else // !TEST_EARLY_SHUTDOWN

        const int WAIT_FOR_WORK_SECS = 10;
        LOG_INFO("Waiting for work to be done 0 out of %d secs.", WAIT_FOR_WORK_SECS);
        const int secInc = 2;
        for(int secs=0; secs<WAIT_FOR_WORK_SECS; secs+=secInc)
        {
            VPLThread_Sleep(VPLTime_FromSec(secInc));
            LOG_INFO("Waiting for work to be done %d out of %d secs.",
                     secs, WAIT_FOR_WORK_SECS);
            printThreadStats(myThreadPool);
        }
        LOG_INFO("Test done waiting. Shutdown.");

        rc = myThreadPool.CheckAndPrepareShutdown();
        if(rc != 0) {
            LOG_ERROR("threadPool Shutdown:%d", rc);
        }

        const int WAIT_FOR_SHUTDOWN_SECS = 10;
        LOG_INFO("Waiting for shutdown. (perhaps not needed) 0 out of %d secs",
                 WAIT_FOR_SHUTDOWN_SECS);
        for(int secs=0; secs<WAIT_FOR_SHUTDOWN_SECS; secs+=secInc)
        {
            VPLThread_Sleep(VPLTime_FromSec(secInc));
            LOG_INFO("Waiting for shutdown. (perhaps not needed) %d out of %d secs",
                     secs, WAIT_FOR_SHUTDOWN_SECS);
            printThreadStats(myThreadPool);
        }
    }
#endif // TEST_EARLY_SHUTDOWN

    while(threadStates.size()>0) {
        delete threadStates.back();
        threadStates.pop_back();
    }

    LOG_INFO("Deleted thread pools.  Exiting.");

    return rv;
}
