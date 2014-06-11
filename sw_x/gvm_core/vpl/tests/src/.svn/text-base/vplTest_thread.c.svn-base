//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplTest.h"

#include "vpl_th.h"
#include "vpl_time.h"

#ifndef _MSC_VER
#include <stdlib.h>
#include <stdbool.h>  // for bool, true, false
#endif

// Condition variable waits aren't guaranteed to complete
// within a tolerance of the requested time.  They can
// overrun by an arbitrary amount.
// In one Windows 7 case a 1s wait usually overran by ~12ms.
#define SLEEP_TIME_TOLERANCE  2000000 // 2 s
// TODO: should have a separate tolerance for over vs. under.

/// 3k is allegedly sufficient stack-space for these tests.
#define TEST_THREAD_STACK_SIZE 0x3000

#  define NUM_THREADS (32)
#  define NUM_MUTEXES (32)
#  define NUM_CONDVARS NUM_MUTEXES    // tests must have 1 test-condvar per test-mutex
#  define NUM_SEMAPHORES (32)         // need not be the same as NUM_MUTEXES/NUM_CONDVARS(?)

#define NUM_HELPER_THREADS (NUM_THREADS - 1)
VPLThread_t    test_thread[NUM_THREADS];

VPLThread_t    test_thread_id[NUM_THREADS];
VPLMutex_t     test_mutex[NUM_MUTEXES] = {VPLMUTEX_SET_UNDEF};
VPLCond_t      test_cond[NUM_CONDVARS] = {VPLCOND_SET_UNDEF};
VPLSem_t       test_sem[NUM_SEMAPHORES] = {VPLSEM_SET_UNDEF};
VPLThread_once_t once_control = VPLTHREAD_ONCE_INIT;

static int vpl_thread_test_dummy_init_val = 0;
static void vpl_thread_test_dummy_init(void) {
    vpl_thread_test_dummy_init_val++;
}

#if 0
/// Change thread priority, expecting success.
static void set_and_check_priority(int newPriority, const char *msg)
{
    int actualPriority = 0;
    int rc = 0;
    
    // Test Set priority back to what we got (no-op).
    VPLTEST_LOG("Set priority to %d.", newPriority);
    rc = VPLThread_SetSchedPriority(newPriority);
    VPLTEST_CHK_OK(rc, "VPLThread_SetSchedPriority");

    // Confirm actual priority is now what we set it to.
    actualPriority = VPLThread_GetSchedPriority();
    if (actualPriority != newPriority) {
        VPLTEST_FAIL("%s: after set priority to %d, priority now %d.",
                msg, newPriority, actualPriority);
    }
}

/// Change thread priority, expecting failure.
static void set_and_check_invalid_priority(int newPriority, const char *msg)
{
    int origPriority = VPLThread_GetSchedPriority();
    int actualPriority = 0;
    int rc = 0;

    // Test Set priority back to some caller-supplied illegal value.
    VPLTEST_LOG("Set priority to (invalid) %d.", newPriority);
    rc = VPLThread_SetSchedPriority(newPriority);
    if (rc == VPL_OK) {
        VPLTEST_NONFATAL_ERROR("%s to %d returned success code %d.",
                msg, newPriority, rc);
    }

    // Confirm actual priority has not changed after expected set-prio failure.
    actualPriority = VPLThread_GetSchedPriority();
    if (actualPriority != origPriority) {
        VPLTEST_FAIL("%s after set priority to invalid %d, priority now %d.",
                msg, newPriority, actualPriority);
    }
}
#endif

static VPLThread_return_t thread_test_helper(VPLThread_arg_t arg)
{
    VPLTime_t start_time;
    VPLTime_t cur_time;
    unsigned long timeout;
    VPLThread_t myself = VPLTHREAD_INVALID;
#   if !defined(ANDROID) && !defined(IOS)
    int affinity;
    int max_cpu;
#   endif
    int rc;
    int i;

    VPLTEST_LOG("Can call VPLThread_Once() successfully.");
    for (i = 0; i < 100; i++) {
        rc = VPLThread_Once(&once_control, vpl_thread_test_dummy_init);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("VPLThread_Once() returned %d.", rc);
        }
    }
    VPLTEST_LOG("Init function only ran once.");
    if (vpl_thread_test_dummy_init_val != 1) {
        VPLTEST_FAIL("Init function called %d times.",
                vpl_thread_test_dummy_init_val);
    }

#   if !defined(ANDROID) && !defined(IOS)
    VPLTEST_LOG("Can check and set thread affinity.");
    max_cpu = VPLThread_GetMaxValidCpuId();
    VPLTEST_LOG("Max_cpu = %d.", max_cpu);
    affinity = VPLThread_GetAffinity();
    // Initially, no affinity is set
    if (affinity != -1) {
        if (affinity == 0 && max_cpu == 0) {
            VPLTEST_LOG("Only one CPU. Affinity is fixed.");
        }
        else {
            VPLTEST_FAIL("Affinity is %d (max_cpu is %d). Expected -1 (no affinity set).",
                    affinity, max_cpu);
        }
    }
    // Set affinity to each of the possible CPUs.
    for (i = 0; i <= max_cpu; i++) {
        rc = VPLThread_SetAffinity(i);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Set Affinity for CPU %d returned error %d.", rc, i);
        }
        affinity = VPLThread_GetAffinity();
        // Should be as last set.
        if (affinity != i) {
            VPLTEST_FAIL("Affinity is %d. Expected %d (as last set).",
                    affinity, i);
        }
    }
    // Set affinity beyond max_cpu.
    rc = VPLThread_SetAffinity(max_cpu+1);
    if (rc == VPL_OK) {
        VPLTEST_FAIL("Set Affinity for CPU %d (beyond max_cpu %d) OK. Should have failed.",
                max_cpu+1, max_cpu);
    }
#   endif
#   ifndef VPL_PLAT_IS_WINRT
    // Test VPL thread priorities.
    VPLTEST_LOG("Can check/set own priority level.");
    {
        int vpl_prio;
        
        int priority = VPLThread_GetSchedPriority();
        int origPriority = priority;  // to restore after priority test.

        UNUSED(vpl_prio); // temporarily

        if (priority < 0) {
            VPLTEST_NONFATAL_ERROR("Get priority returned error code %d.", priority);
            priority = VPL_PRIO_MIN; // so next call doesn't fail
        }

        // TODO these tests are making assumptions about the priorities the test
        // user is allowed to set.
        UNUSED(origPriority); // temporarily
#       if 0
        // Set priority to whatever we're currently at (or a sane value).
        // Confirm that priority is what we expected.
        set_and_check_priority(priority, "set prio to original value (no-op)");
        set_and_check_priority(VPL_PRIO_MIN, "set prio to lowest");
        set_and_check_priority(VPL_PRIO_MAX, "set prio to highest");

        // Negative testing.
        set_and_check_invalid_priority(VPL_PRIO_MAX+1, "illegal prio, above max");
        set_and_check_invalid_priority(VPL_PRIO_MIN-1, "illegal prio, below min");

        // Check every valid VPL priority.
        for (vpl_prio = VPL_PRIO_MIN; vpl_prio <= VPL_PRIO_MAX; vpl_prio++) {
            set_and_check_priority(vpl_prio, "iterating over prio");
        }

        // ... and restore priority to exact pre-test state.
        set_and_check_priority(origPriority, "restore original pre-test");
#       endif
    }
#   endif

    VPLTEST_LOG("Can sleep, and the right amount of time passes.");
    timeout = 1000000;    // ... 1s
 
    start_time = VPLTime_GetTimeStamp();
    VPLThread_Sleep(timeout);
    cur_time = VPLTime_GetTimeStamp();

    if (VPLTime_DiffAbs((start_time+timeout), cur_time) > SLEEP_TIME_TOLERANCE) {
        VPLTEST_FAIL("Sleep timer out of tolerance. Expected:"FMT_VPLTime_t" Got:"FMT_VPLTime_t" Tolerance:%d.",
                start_time+timeout, cur_time, SLEEP_TIME_TOLERANCE);
    }

    timeout = 100000;    // ... 100ms

    start_time = VPLTime_GetTimeStamp();
    VPLThread_Sleep(timeout);
    cur_time = VPLTime_GetTimeStamp();

    if (VPLTime_DiffAbs((start_time+timeout), cur_time) > SLEEP_TIME_TOLERANCE) {
        VPLTEST_FAIL("Sleep timer out of tolerance. Expected:"FMT_VPLTime_t" Got:"FMT_VPLTime_t" Tolerance:%d.",
                start_time+timeout, cur_time, SLEEP_TIME_TOLERANCE);
    }

    timeout = 10000;    // ... 10ms
    
    start_time = VPLTime_GetTimeStamp();
    VPLThread_Sleep(timeout);
    cur_time = VPLTime_GetTimeStamp();

    if (VPLTime_DiffAbs((start_time+timeout),cur_time) > SLEEP_TIME_TOLERANCE) {
        VPLTEST_FAIL("Sleep timer out of tolerance. Expected:"FMT_VPLTime_t" Got:"FMT_VPLTime_t" Tolerance:%d.",
                start_time+timeout, cur_time, SLEEP_TIME_TOLERANCE);
    }

    // thread can identify itself
    VPLTEST_LOG("Can get my own thread ID.");
    myself = VPLThread_Self();
    if (VPLThread_Equal(&myself, &VPLTHREAD_INVALID)) {
        VPLTEST_FAIL("Thread ID invalid.");
    }
    VPLTEST_LOG("Thread_self = "FMT_VPLThread_t".", VAL_VPLThread_t(myself));

    // thread exits
#ifndef VPL_PLAT_IS_WINRT
    VPLTEST_LOG("Exiting thread passes a return code back to thread joining.");
    VPLThread_Exit(arg);

    VPLTEST_FAIL("Running after call to VPLThread_Exit().");
#endif
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static VPLThread_return_t emptyFunc(VPLThread_arg_t arg)
{
#ifndef VPL_PLAT_IS_WINRT
    // Empty function that immediately exits the thread.
    VPLThread_Exit(arg);
#endif
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static void testThreadInvalidParameters(void)
{
    VPLThread_t thread1, thread2;
    int sanity_check = 0xcafebabe;
    int sanity_reply = 0;
    VPLThread_t myThread = VPLThread_Self();
    VPLThread_attr_t attrs;
    size_t size = 0;
    int prio = 0;
    memset(&attrs, 0, sizeof(attrs));

    VPLTEST_CALL_AND_CHK_RV(VPLThread_Create(NULL,
            emptyFunc,
            VPL_AS_THREAD_FUNC_ARG(sanity_check),
            NULL,
            "test"),
            VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_Create(&thread1,
            emptyFunc,
            NULL,
            NULL,
            "test"),
            VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_Create(&thread2,
            emptyFunc,
            VPL_AS_THREAD_FUNC_ARG(sanity_check),
            NULL,
            NULL),
            VPL_OK);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_Join(NULL,
            VPL_AS_THREAD_RETVAL_PTR(&sanity_reply)),
            VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_Join(&thread1, NULL), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_Join(&thread2, NULL), VPL_OK);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_Join(&myThread,
            VPL_AS_THREAD_RETVAL_PTR(&sanity_reply)),
            VPL_ERR_DEADLK);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_Equal(NULL, &myThread), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_Equal(&myThread, NULL), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrInit(NULL), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrDestroy(NULL), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrSetStackSize(NULL, 0), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrGetStackSize(NULL, &size), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrGetStackSize(&attrs, NULL), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrSetPriority(NULL, 0), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrGetPriority(NULL, &prio), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrGetPriority(&attrs, NULL), VPL_ERR_INVALID);
}

// TODO higher priorities requires special permission for the user running the test!
#define HELPER_THREAD_PRIO (0)

static VPLThread_return_t testAttrHelperThread(VPLThread_arg_t arg)
{
    // Check that the thread priority is what we expect.
    VPLTEST_LOG("Verify that the helper thread's priority is %d.", HELPER_THREAD_PRIO);
#ifndef VPL_PLAT_IS_WINRT
    VPLTEST_CALL_AND_CHK_RV(VPLThread_GetSchedPriority(), HELPER_THREAD_PRIO);
    VPLThread_Exit(arg);
#endif
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static void testThreadAttributes(void)
{
    // Test basic functionality.

    VPLThread_attr_t attrs;
    size_t size = 0;
    int prio = 0;
    VPLThread_t helperThread;

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrInit(&attrs), VPL_OK);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrSetStackSize(&attrs, VPLTHREAD_STACKSIZE_MIN), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrGetStackSize(&attrs, &size), VPL_OK);
    if (size != VPLTHREAD_STACKSIZE_MIN) {
        VPLTEST_FAIL("VPLThread_AttrSetStackSize() set stack size to %d but VPLThread_AttrGetStackSize() returned "FMTu_size_t".",
                VPLTHREAD_STACKSIZE_MIN, size);
    }

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrSetPriority(&attrs, VPL_PRIO_MAX), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrGetPriority(&attrs, &prio), VPL_OK);
    if (prio != VPL_PRIO_MAX) {
        VPLTEST_FAIL("VPLThread_AttrSetPriority() set prio to %d but VPLThread_AttrGetPriority() returned %d.",
                VPL_PRIO_MAX, prio);
    }

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrDestroy(&attrs), VPL_OK);

    // Create a thread using the attributes.

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrInit(&attrs), VPL_OK);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrSetStackSize(&attrs, VPLTHREAD_STACKSIZE_MAX), VPL_OK);

    VPLTEST_LOG("Setting helper thread's priority to %d.", HELPER_THREAD_PRIO);
    VPLTEST_CALL_AND_CHK_RV(VPLThread_AttrSetPriority(&attrs, HELPER_THREAD_PRIO), VPL_OK);

    VPLTEST_CALL_AND_CHK_RV(VPLThread_Create(&helperThread,
            testAttrHelperThread,
            NULL,
            &attrs, // Set stack size and prio to the values specified above.
            "test"),
            VPL_OK);

    // Join the thread.

    VPLTEST_CALL_AND_CHK_RV(VPLThread_Join(&helperThread, NULL), VPL_OK);
}

void testVPLThread(void)
{
    int rc;
    int sanity_check = 0xcafebabe;
    int sanity_reply = 0;

#   if !defined(ANDROID) && !defined(IOS)
    VPLTEST_LOG("CPU affinity can be set to \"any of them\".");
    rc = VPLThread_SetAffinity(-1);
    VPLTEST_CHK_OK(rc, "VPLThread_SetAffinity");
#   endif

    VPLTEST_LOG("Thread can be created.");
    rc = VPLThread_Create(&test_thread[0],
                          thread_test_helper,
                          VPL_AS_THREAD_FUNC_ARG(sanity_check),
                          NULL, // default VPLThread thread-attributes: prio, stack-size, etc.
                          "test");
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Thread create failed. Code %d.", rc);
        goto out;
    }

    VPLTEST_LOG("Thread can be joined.");
    rc = VPLThread_Join(&test_thread[0], VPL_AS_THREAD_RETVAL_PTR(&sanity_reply));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Thread join failed. Code %d.", rc);
        goto out;
    }
//winRT doesn't support _endthreadex, thus cannot support return thread exit code
#ifndef VPL_PLAT_IS_WINRT
    VPLTEST_LOG("Thread return code found after join.");
    // thread return value found on join.
    if (sanity_check != sanity_reply) {
        VPLTEST_FAIL("Expected thread termination code not received. Expected: %x, got: %x.", sanity_check, sanity_reply);
    }
#endif
    VPLTEST_LOG("Testing invalid parameters for threads.");
    testThreadInvalidParameters();

    VPLTEST_LOG("Testing thread attributes.");
    testThreadAttributes();

 out:
    NO_OP();
}

struct TestMutexWithContention_Workspace {
    VPLMutex_t mutex;  // mutex that protects the counter below
    u64 counter;       // shared counter
    u64 limit;         // iteration limit
};

static void testMutexWithContention_Worker(struct TestMutexWithContention_Workspace *ws)
{
    u64 i;
    for (i = 0; i < ws->limit; i++) {
        VPLMutex_Lock(&ws->mutex);
        ws->counter++;
        VPLMutex_Unlock(&ws->mutex);
    }
}

static VPLTHREAD_FN_DECL testMutexWithContention_ThreadMain(void *arg)
{
    struct TestMutexWithContention_Workspace *ws = (struct TestMutexWithContention_Workspace*)arg;
    testMutexWithContention_Worker(ws);
    return VPLTHREAD_RETURN_VALUE;
}

static void testMutexWithContention(void)
{
    struct TestMutexWithContention_Workspace ws;
    const u64 expLoopLimit = 10000;
    VPLTime_t startTime, endTime, elapsedTime;
    u64 testLoopLimit;
    int i;
    int rc;
    VPLThread_attr_t threadAttr;
    const size_t workerThread_stackSize = 16 * 1024;
    VPL_BOOL threadAttr_init = VPL_FALSE;
    VPLDetachableThreadHandle_t thread[NUM_THREADS];

    VPLMutex_Init(&ws.mutex);

    // Plan:
    // Spawn M threads and have each thread increment a shared counter N times.
    // At the end, the counter value must equal M * N.
    // (Otherwise, Mutex isn't working correctly.)

    // To guarantee that the threads will interleave,
    // we want each thread to run about 100 milliseconds.
    // To determine what N needs to be, we will run a simple small-scale experiment.
    ws.limit = expLoopLimit;
    ws.counter = 0;
    startTime = VPLTime_GetTimeStamp();
    testMutexWithContention_Worker(&ws);
    endTime = VPLTime_GetTimeStamp();
    elapsedTime = VPLTime_DiffAbs(startTime, endTime);
    VPLTEST_LOG("elapsedTime "FMT_VPLTime_t"us", elapsedTime);
    // On my Linux VM, it was 721us for 10000.

    // Determine N.
    testLoopLimit = VPLTime_FromMillisec(100) * expLoopLimit / elapsedTime;
    VPLTEST_LOG("testLoopLimit="FMTu64, testLoopLimit);
    ws.limit = testLoopLimit;
    ws.counter = 0;

    rc = VPLThread_AttrInit(&threadAttr);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Failed to init thread-attr obj. Code %d.", rc);
        goto end;
    }
    threadAttr_init = VPL_TRUE;
    rc = VPLThread_AttrSetStackSize(&threadAttr, workerThread_stackSize);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Failed to set stack size in thread-attr obj. Code %d.", rc);
        goto end;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        rc = VPLDetachableThread_Create(&thread[i], testMutexWithContention_ThreadMain, (void*)&ws, &threadAttr, "mutex test worker");
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thread create failed. Code %d.", rc);
            goto end;
        }
    }
    for (i = 0; i < NUM_THREADS; i++) {
        rc = VPLDetachableThread_Join(&thread[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thead join failed. Code %d.",rc);
            goto end;
        }
    }
    VPLTEST_LOG("ws.counter="FMTu64, ws.counter);
    if (ws.counter != NUM_THREADS * testLoopLimit) {
        VPLTEST_FAIL("Unexpected counter value; expected "FMTu64", got "FMTu64, NUM_THREADS * testLoopLimit, ws.counter);
    }

 end:
    if (threadAttr_init) {
        VPLThread_AttrDestroy(&threadAttr);
    }
    VPLMutex_Destroy(&ws.mutex);
}

static volatile bool mutex_test_in_critical_section = false;
static int mutex_test_shared_data = 0;

static VPLThread_return_t mutex_test_helper(VPLThread_arg_t arg)
{
    // arg is the index of the mutex to take.
    unsigned int index = VPLTHREAD_FUNC_ARG_TO_UINT(arg);
    int rc;
    
    // Cannot unlock a mutex that we do not yet have a lock on.
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Unlock(&(test_mutex[0])), VPL_ERR_PERM);

    // Take own mutex, waiting for test to continue.
    VPLTEST_LOG("Mutex helper %d taking its mutex.", index);
    rc = VPLMutex_Lock(&(test_mutex[index]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
    }

    // Try to take test_mutex[0]
    VPLTEST_LOG("Mutex helper %d locking mutex 0.", index);
    rc = VPLMutex_Lock(&(test_mutex[0]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
    }
    else {
        VPLTEST_LOG("Mutex helper %d locked mutex 0.", index);
    }

    if (mutex_test_in_critical_section) {
        VPLTEST_FAIL("Another thread was already in the critical section!.");
    }
    mutex_test_in_critical_section = true;

    test_thread_id[index] = VPLThread_Self();

    mutex_test_shared_data++;

    // Make sure other threads have a chance to call VPLMutex_Lock before we leave the critical section.
    VPLThread_Sleep(900000);

    mutex_test_in_critical_section = false;

    VPLTEST_LOG("Mutex helper %d releasing mutex 0.", index);
    rc = VPLMutex_Unlock(&(test_mutex[0]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
    }
    else {
        VPLTEST_LOG("Mutex helper %d released mutex 0.", index);
    }

    VPLTEST_LOG("Mutex helper %d releasing its mutex.", index);
    rc = VPLMutex_Unlock(&(test_mutex[index]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
    }

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static void testMutexInvalidParameters(void)
{
    VPLMutex_t mutex, uninitMutex;
    memset(&mutex, 0, sizeof(mutex));
    memset(&uninitMutex, 0, sizeof(mutex));

    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Init(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Init(&mutex), VPL_OK);

    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Lock(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Lock(&mutex), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Lock(&uninitMutex), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLMutex_TryLock(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_TryLock(&uninitMutex), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Unlock(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Unlock(&uninitMutex), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Destroy(NULL), VPL_ERR_INVALID);
    // Cannot destroy a locked mutex.
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Destroy(&mutex), VPL_ERR_BUSY);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Unlock(&mutex), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Destroy(&mutex), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Destroy(&uninitMutex), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Locked(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Locked(&uninitMutex), 0);

    VPLTEST_CALL_AND_CHK_RV(VPLMutex_LockedSelf(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_LockedSelf(&uninitMutex), 0);
}

void testVPLMutex()
{
    int rc;
    int i = 0;

    VPLTEST_LOG("Mutex tests.");
    
    VPLTEST_LOG("Mutex can be initialized.");
    for (i = 0; i < NUM_MUTEXES; i++) {
        rc = VPLMutex_Init(&test_mutex[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex init failed. Code %d.", rc);
            goto out;
        }
        
#ifdef VPL_THREAD_SYNC_INIT_CHECK
        // Cannot init a mutex again. Should get VPL_ERR_IS_INIT.
        VPLTEST_CALL_AND_CHK_RV(VPLMutex_Init(&test_mutex[i]), VPL_ERR_IS_INIT);
#endif

        // confirm not locked on init
        if (VPLMutex_Locked(&test_mutex[i]) || VPLMutex_LockedSelf(&test_mutex[i])) {
            VPLTEST_FAIL("Mutex is locked before being locked.");
            goto mutex_cleanup;
        }
    }
    
    VPLTEST_LOG("Taking mutex[0] to hold helpers.");
    rc = VPLMutex_Lock(&(test_mutex[0]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
    }
    
    // Cannot destroy a locked mutex. Should get VPL_ERR_BUSY.
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Destroy(&(test_mutex[0])), VPL_ERR_BUSY,
            "This check is sketchy. Posix does not require implementations to do the EBUSY check");

    // Should still be locked by this thread.
    VPLTEST_LOG("Confirm mutex is locked.");
    if (!VPLMutex_Locked(&test_mutex[0]) || !VPLMutex_LockedSelf(&test_mutex[0])) {
        VPLTEST_FAIL("Mutex not locked after lock call.");
    }
        
    VPLTEST_LOG("Double-lock mutex.");
    rc = VPLMutex_Lock(&test_mutex[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex lock failed to lock mutex already held by this thread. Code %d.", rc);
        goto out;
    }
        
    // Release the recursive lock.
    VPLTEST_LOG("Undo double-lock.");
    rc = VPLMutex_Unlock(&(test_mutex[0]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
    }
    
    // Should still be locked by this thread.
    VPLTEST_LOG("Confirm mutex still locked.");
    if (!VPLMutex_LockedSelf(&test_mutex[0])) {
        VPLTEST_FAIL("Mutex not locked after two locks and one unlock.");
    }
    
    VPLTEST_LOG("Try to double-lock mutex.");
    rc = VPLMutex_TryLock(&test_mutex[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex trylock failed to lock mutex already held by this thread. Code %d.", rc);
        goto out;
    }
    
    // Release the recursive lock.
    VPLTEST_LOG("Undo double-lock.");
    rc = VPLMutex_Unlock(&(test_mutex[0]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
    }
    
    // Should still be locked by this thread.
    VPLTEST_LOG("Confirm mutex still locked.");
    if (!VPLMutex_LockedSelf(&test_mutex[0])) {
        VPLTEST_FAIL("Mutex not locked after two locks and one unlock.");
    }
        
    // Start mutex helpers and take all the mutexes.
    // Each will take a mutex, then wait on mutex 0 to be released.
    VPLTEST_LOG("Start helper threads.");
    for (i = 1; i < NUM_THREADS; i++) {
        rc = VPLMutex_Lock(&(test_mutex[i]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
        }
        test_thread_id[i] = VPLTHREAD_INVALID;
        rc = VPLThread_Create(&test_thread[i],
                              mutex_test_helper, VPL_AS_THREAD_FUNC_ARG(i),
                              NULL, // default VPLThread thread-attributes: prio, stack-size, etc.
                              "mutex helper");
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thread create failed. Code %d.", rc);
            goto out;
        }
    }
    
    // Wait for each mutex to be locked, then try to lock it.
    for (i = 1; i < NUM_THREADS; i++) {
        VPLTEST_LOG("Allow helper %d to continue to critical section.", i);
        rc = VPLMutex_Unlock(&(test_mutex[i]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
        }
        
        VPLTEST_LOG("Check that helper %d has its mutex.", i);
        while (!VPLMutex_Locked(&test_mutex[i])) {
            VPLThread_Sleep(10000);
        }
        
        VPLTEST_LOG("Try to lock mutex %d, held by helper thread.", i);
        rc = VPLMutex_TryLock(&test_mutex[i]);
        if (rc != VPL_ERR_BUSY) {
            VPLTEST_FAIL("Mutex trylock failed to report EBUSY when mutex was locked by another thread. Code %d.", rc);
            goto out;
        }
    }
    
    // Allow threads to finish.
    VPLTEST_LOG("Allow helpers to enter critical section, then exit.");
    rc = VPLMutex_Unlock(&(test_mutex[0]));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
    }
   
    // Join with the helper threads.
    for (i = 1; i < NUM_THREADS; i++) {
        rc = VPLThread_Join(&test_thread[i], NULL);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thread join failed. Code %d.", rc);
        }
        if (VPLThread_Equal(&test_thread_id[i], &VPLTHREAD_INVALID)) {
            VPLTEST_FAIL("Thread id for helper %d not set.", i);
        }
    }
    
    if (mutex_test_shared_data != NUM_HELPER_THREADS) {
        VPLTEST_FAIL("Counted %d threads (expected %d).", mutex_test_shared_data, NUM_HELPER_THREADS);
    }
    mutex_test_shared_data = 0;

    VPLTEST_LOG("Testing contended mutex case.");
    testMutexWithContention();

    VPLTEST_LOG("Testing mutex invalid parameters.");
    testMutexInvalidParameters();

 mutex_cleanup:
    VPLTEST_LOG("Mutex can be destroyed.");
    for (i = 0; i < NUM_MUTEXES; i++) {
        rc = VPLMutex_Destroy(&test_mutex[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex destroy failed. Code %d.", rc);
        }
    }
    
 out:
    NO_OP();
}

static VPLThread_return_t cond_signal_test_helper(VPLThread_arg_t arg)
{
    // arg is the index of the mutex to take.
    int index = VPLTHREAD_FUNC_ARG_TO_INT(arg);
    int rc;
    int i = 0;

    // Take own mutex, waiting for test to continue.
    VPLTEST_LOG("Cond helper %d taking its mutex %d times.", index, index);
    for (i = 0; i < index; i++) {
        rc = VPLMutex_Lock(&(test_mutex[index]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
        }
    }

    // Set thread ID, indicating thread has started.
    test_thread_id[index] = VPLThread_Self();

    // Wait forever on own cond variable
    VPLTEST_LOG("Helper %d: Test cond variable no timeout.", index);
    rc = VPLCond_TimedWait(&test_cond[index], &test_mutex[index], VPL_TIMEOUT_NONE);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Timedwait returned by other than signal. Code %d.", rc);
    }

    if (!VPLMutex_LockedSelf(&test_mutex[index])) {
        VPLTEST_FAIL("Cond variable released, but helper %d does not own its mutex.",
                index);
    }

    // Release mutex and exit.
    // Also tests that cond release re-locked mutex correctly.
    VPLTEST_LOG("cond helper %d releasing its mutex %d times.", index, index);
    for (i = 0; i < index; i++) {
        rc = VPLMutex_Unlock(&(test_mutex[index]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
        }
        if (i+1 < index) { // should still be locked
            if (!VPLMutex_LockedSelf(&test_mutex[index])) {
                VPLTEST_FAIL("Mutex released too early.");
            }
        }
        else { // should be unlocked
            if (VPLMutex_LockedSelf(&test_mutex[index])) {
                VPLTEST_FAIL("Mutex still locked when it should be released.");
            }
        }
    }

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

VPL_BOOL cond_mutex_test_running = VPL_FALSE;

static VPLThread_return_t cond_mutex_interaction_helper(VPLThread_arg_t arg)
{
    int rc;

    UNUSED(arg);

    while (cond_mutex_test_running) {
        rc = VPLMutex_Lock(&(test_mutex[0]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
        }

        // Pretend to do something critical
        VPLThread_Sleep(1000);

        rc = VPLMutex_Unlock(&(test_mutex[0]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
        }

        // Pretend to do something non-critical
        VPLThread_Sleep(3000);
    }

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static void cond_mutex_interaction_test(void)
{
    int rc;
    int i;

    cond_mutex_test_running = VPL_TRUE;
    rc = VPLThread_Create(&test_thread[0],
                          cond_mutex_interaction_helper,  
                          NULL,
                          NULL, // default VPLThread thread-attributes: prio, stack-size, etc.
                          "cond helper");

    // Mutex is locked initially.    
    rc = VPLMutex_Unlock(&test_mutex[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
    }

    for (i = 0; i < 100; i++) {
        rc = VPLMutex_Lock(&test_mutex[0]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
            break;
        }
        rc = VPLCond_TimedWait(&test_cond[0], &test_mutex[0], 10000);
        if (rc != VPL_ERR_TIMEOUT) {
            VPLTEST_FAIL("Timedwait returned without timeout. Code %d.", rc);
        }

        rc = VPLMutex_Unlock(&test_mutex[0]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
            break;
        }

        // Pretend to do something non-critical.
        VPLThread_Sleep(1000);
    }

    VPLTEST_LOG("Rejoin with the helper thread.");
    cond_mutex_test_running = VPL_FALSE;
    rc = VPLThread_Join(&test_thread[0], NULL);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Thread join failed. Code %d.", rc);
    }

    rc = VPLMutex_Lock(&test_mutex[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
    }
}

static void testCondInvalidParameters(void)
{
    uint32_t timeout = 1000000;
    VPLCond_t cond, uninitCond;
    VPLMutex_t mutex, uninitMutex;

    memset(&cond, 0, sizeof(cond));
    memset(&uninitCond, 0, sizeof(cond));
    memset(&mutex, 0, sizeof(mutex));
    memset(&uninitMutex, 0, sizeof(mutex));

    VPLTEST_CALL_AND_CHK_RV(VPLCond_Init(&cond), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Init(&mutex), VPL_OK);

    VPLTEST_CALL_AND_CHK_RV(VPLCond_Init(NULL), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLCond_TimedWait(&uninitCond, &mutex, timeout),
            VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLCond_TimedWait(&cond, &uninitMutex, timeout),
            VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLCond_TimedWait(NULL, &mutex, timeout),
            VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLCond_TimedWait(&cond, NULL, timeout),
            VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLCond_Signal(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLCond_Signal(&uninitCond), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLCond_Destroy(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLCond_Destroy(&uninitCond), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLCond_Destroy(&cond), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLMutex_Destroy(&mutex), VPL_OK);
}

static VPLThread_return_t cond_broadcast_test_helper(VPLThread_arg_t arg)
{
    int rc;
    int index = VPLTHREAD_FUNC_ARG_TO_INT(arg);

    rc = VPLMutex_Lock(&(test_mutex[0]));
    if(rc != VPL_OK) {
        VPLTEST_LOG("FAIL: mutex lock failed. Code %d", rc);
        vplTest_incrErrCount();
    }

    // Indicate thread is set to test receiving broadcast.
    test_thread_id[index] = VPLThread_Self();

    // Wait forever on cond variable
    VPLTEST_LOG("Helper %d: Test cond variable no timeout.", index);
    rc = VPLCond_TimedWait(&test_cond[0], &test_mutex[0], VPL_TIMEOUT_NONE);

    rc = VPLMutex_Unlock(&(test_mutex[0]));
    if(rc != VPL_OK) {
        VPLTEST_LOG("FAIL: mutex unlock failed. Code %d", rc);
        vplTest_incrErrCount();
    }

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static void cond_broadcast_test(void)
{
    int rc;
    int i;
    int ready = 0;

    VPLTEST_LOG("Start helper threads.");
    for(i = 1; i < 5; i++) {
        test_thread_id[i] = VPLTHREAD_INVALID;
        rc = VPLThread_Create(&test_thread[i],
                              cond_broadcast_test_helper,
                              VPL_AS_THREAD_FUNC_ARG(i),
                              NULL, // default VPLThread thread-attributes: prio, stack-size, etc.
                              "cond helper");
        if(rc != VPL_OK) {
            VPLTEST_LOG("FAIL: Thread create failed. Code %d", rc);
            vplTest_incrErrCount();
            return;
        }
    }

    // Mutex is locked initially.
    rc = VPLMutex_Unlock(&test_mutex[0]);
    if(rc != VPL_OK) {
        VPLTEST_LOG("FAIL: Mutex unlock failed. Code %d", rc);
        vplTest_incrErrCount();
    }

    // Make sure all helpers are waiting
    while(!ready) {
        VPLMutex_Lock(&test_mutex[0]);
        if(rc != VPL_OK) {
            VPLTEST_LOG("FAIL: Mutex lock failed. Code %d", rc);
            vplTest_incrErrCount();
            return;
        }

        for(i = 1; i < 5; i++) {
            if(VPLThread_Equal(&test_thread_id[i], &VPLTHREAD_INVALID)) {
                break;
            }
        }
        if(i == 5) {
            ready = 1;
        }

        VPLMutex_Unlock(&test_mutex[0]);
        if(rc != VPL_OK) {
            VPLTEST_LOG("FAIL: Mutex unlock failed. Code %d", rc);
            vplTest_incrErrCount();
            return;
        }
    }

    // Broadcast to all waiting helpers to wake up.
    rc = VPLCond_Broadcast(&test_cond[0]);
    if(rc != VPL_OK) {
        VPLTEST_LOG("FAIL: VPLCond_Broadcast() failed. Code %d", rc);
        vplTest_incrErrCount();
        return;
    }

    VPLTEST_LOG("Rejoin with the helper threads.");
    for(i = 1; i < 5; i++) {
        rc = VPLThread_Join(&test_thread[i], NULL);
        if(rc != VPL_OK) {
            VPLTEST_LOG("FAIL: Thread join failed. Code %d", rc);
            vplTest_incrErrCount();
        }
    }

    rc = VPLMutex_Lock(&test_mutex[0]);
    if(rc != VPL_OK) {
        VPLTEST_LOG("FAIL: Mutex lock failed. Code %d", rc);
        vplTest_incrErrCount();
    }
}

void testVPLCond(void)
{
    int i;
    VPLTime_t start_time;
    VPLTime_t cur_time;
    uint32_t timeout;
    int rc;

    VPLTEST_LOG("Initialize cond variables.");
    for (i = 0; i < NUM_CONDVARS; i++) {
        rc = VPLCond_Init(&test_cond[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Cond init failed. Code %d.", rc);
            goto cond_init_fail;
        }
#ifdef VPL_THREAD_SYNC_INIT_CHECK
        // Cannot init a cond var again. Should get VPL_ERR_IS_INIT.
        VPLTEST_CALL_AND_CHK_RV(VPLCond_Init(&test_cond[i]), VPL_ERR_IS_INIT);
#endif
    }

    VPLTEST_LOG("Initialize recursive mutex variables.");
    for (i = 0; i < NUM_MUTEXES; i++) {
        rc = VPLMutex_Init(&test_mutex[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Cond Mutex init failed. Code %d.", rc);
            goto mutex_init_fail;
        }
    }

    VPLTEST_LOG("Take Cond mutex for tests.");
    rc = VPLMutex_Lock(&test_mutex[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Cond Mutex lock failed. Code %d.", rc);
        goto mutex_lock_fail;
    }

    VPLTEST_LOG("Test cond variable timeout.");
    timeout = 0;
    start_time = VPLTime_GetTimeStamp();
    rc = VPLCond_TimedWait(&test_cond[0], &test_mutex[0], timeout);
    cur_time = VPLTime_GetTimeStamp();
    if (rc != VPL_ERR_TIMEOUT) {
        VPLTEST_FAIL("Condvar timedwait returned without timeout. Code %d.", rc);
    }
    if (VPLTime_DiffAbs((start_time+timeout), cur_time) > SLEEP_TIME_TOLERANCE) {
        VPLTEST_FAIL("Zero timeout took too long. Expected:"FMT_VPLTime_t" Got:"FMT_VPLTime_t" Tolerance:%d.",
                start_time+timeout, cur_time, SLEEP_TIME_TOLERANCE);
    }

    timeout = 1000000;
    start_time = VPLTime_GetTimeStamp();
    rc = VPLCond_TimedWait(&test_cond[0], &test_mutex[0], timeout);
    cur_time = VPLTime_GetTimeStamp();
    if (rc != VPL_ERR_TIMEOUT) {
        VPLTEST_FAIL("Timedwait returned without timeout. Code %d.", rc);
    }
    if (VPLTime_DiffAbs((start_time+timeout), cur_time) > SLEEP_TIME_TOLERANCE) {
        VPLTEST_FAIL("1.0s timeout took too long.  Expected:"FMT_VPLTime_t" Got:"FMT_VPLTime_t" Tolerance:%d.",
                start_time+timeout, cur_time, SLEEP_TIME_TOLERANCE);
    }

    VPLTEST_LOG("Start helper threads.");
    for (i = 1; i < NUM_THREADS; i++) {
        test_thread_id[i] = VPLTHREAD_INVALID;
        rc = VPLThread_Create(&test_thread[i],
                              cond_signal_test_helper, VPL_AS_THREAD_FUNC_ARG(i),
                              NULL, // default VPLThread thread-attributes: prio, stack-size, etc.
                              "cond helper");
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thread create failed. Code %d.", rc);
            goto helper_thread_fail;
        }
    }

    VPLTEST_LOG("Wait until helpers are blocking on cond variables.");
    // Also testing that mutex is free when waiting on cond.
    for (i = 1; i < NUM_MUTEXES; i++) {
        rc = VPLMutex_Lock(&test_mutex[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
        }

        while (VPLThread_Equal(&test_thread_id[i], &VPLTHREAD_INVALID)) {
            rc = VPLMutex_Unlock(&(test_mutex[i]));
            if (rc != VPL_OK) {
                VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
                vplTest_incrErrCount();
            }

            VPLThread_Sleep(100000);

            rc = VPLMutex_Lock(&test_mutex[i]);
            if (rc != VPL_OK) {
                VPLTEST_FAIL("Mutex lock failed. Code %d.", rc);
            }
        }
        VPLTEST_LOG("Helper %d now blocking on its cond variable.", i);

        rc = VPLMutex_Unlock(&(test_mutex[i]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Mutex unlock failed. Code %d.", rc);
            vplTest_incrErrCount();
        }

        rc = VPLCond_Signal(&test_cond[i]);
        VPLTEST_LOG("Helper %d signaled.", i);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Cond signal failed. Code %d.", rc);
        }
    }

    VPLTEST_LOG("Rejoin with the helper threads.");
    for (i = 1; i < NUM_THREADS; i++) {
        rc = VPLThread_Join(&test_thread[i], NULL);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thread join failed. Code %d.", rc);
        }
    }

    VPLTEST_LOG("Start cond broadcast test.");
    cond_broadcast_test();

    VPLTEST_LOG("Start cond/mutex interaction test.");
    cond_mutex_interaction_test();

    VPLTEST_LOG("Testing cond var invalid parameters.");
    testCondInvalidParameters();

 helper_thread_fail:
    VPLTEST_LOG("Unlock the mutex.");
    VPLMutex_Unlock(&test_mutex[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Cond var mutex unlock failed. Code %d.", rc);
    }
 mutex_lock_fail:
    VPLTEST_LOG("Destroy the Condvar mutexes.");
    rc = VPLMutex_Destroy(&test_mutex[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Cond var mutex destroy failed. Code %d.", rc);
    }
 mutex_init_fail:
    VPLTEST_LOG("Destroy the conds.");
    VPLCond_Destroy(&test_cond[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Cond destroy failed. Code %d.", rc);
    }
 cond_init_fail:
    NO_OP();
}

static int sem_test_helper_done = 0;
static VPLThread_return_t sem_test_helper(VPLThread_arg_t arg)
{
    // arg is the index of the sem to take, which gets then (arg+2) times.
    unsigned int index = VPLTHREAD_FUNC_ARG_TO_UINT(arg);
    int rc;
    unsigned int i;

    // Take own mutex, waiting for test to continue.
    VPLTEST_LOG("Sem helper %d taking its semaphore %d times.", index, index+2);
    for (i = 0; i < index+2 && !sem_test_helper_done; i++) {
        rc = VPLSem_Wait(&(test_sem[index]));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Sem wait failed. Code %d.", rc);
        }
    }
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static void testSemInvalidParameters(void)
{
    VPLSem_t uninitSem;
    memset(&uninitSem, 0, sizeof(uninitSem));

    VPLTEST_CALL_AND_CHK_RV(VPLSem_Init(NULL, 0, 0), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLSem_Wait(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSem_Wait(&uninitSem), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLSem_TryWait(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSem_TryWait(&uninitSem), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLSem_Post(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSem_Post(&uninitSem), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLSem_Destroy(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSem_Destroy(&uninitSem), VPL_ERR_INVALID);
}

void testVPLSem(void)
{
    const u32 NumSemaphores = NUM_SEMAPHORES;
    u32 i;
    int rc;

    VPLTEST_LOG("Initialize semaphores.");
    for (i = 0; i < NumSemaphores ; i++) {

        // Cannot init a semaphore with maxCount < value. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSem_Init(&test_sem[i], i, i+1), VPL_ERR_INVALID);

        rc = VPLSem_Init(&test_sem[i], i+1, i);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Semaphore init failed. Code %d.", rc);
            goto Sem_Init_fail;
        }

#ifdef VPL_THREAD_SYNC_INIT_CHECK
        // Cannot init a semaphore again. Should get VPL_ERR_IS_INIT.
        VPLTEST_CALL_AND_CHK_RV(VPLSem_Init(&test_sem[i], i+1, i), VPL_ERR_IS_INIT);
#endif
    }

    // Each semaphore has a max one more than its index, and a count equal
    // to its index. Trywait should return error for 0, but pass for the rest.
    VPLTEST_LOG("Attempt to take each semaphore.");
    for (i = 0; i < NumSemaphores; i++) {
        rc = VPLSem_TryWait(&test_sem[i]);
        if (i == 0) {
            if (rc != VPL_ERR_AGAIN) {
                VPLTEST_FAIL("Semaphore trywait didn't return ERR_AGAIN for empty semaphore. Code %d.", rc);
            }

        }
        else {
            if (rc != VPL_OK) {
                VPLTEST_FAIL("Semaphore trywait failed. Code %d.", rc);
            }
        }
    }

    // Each semaphore now has two less than its index, except 0 and 1.
    // Post to each sem twice. 0 should fail the second time, the rest should pass.
    VPLTEST_LOG("Attempt to post each semaphore.");
    for (i = 0; i < NumSemaphores; i++) {

        rc = VPLSem_Post(&test_sem[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Semaphore post failed. Code %d.", rc);
            continue;
        }

        if (i == 0) {
            rc = VPLSem_Post(&test_sem[i]);
            if (rc == VPL_OK) {
                VPLTEST_FAIL("Semaphore post over max happened. Code %d.", rc);
            } else if (rc != VPL_ERR_MAX) {
                VPLTEST_FAIL("Semaphore post failed. Code %d.", rc);
            }
        }
    }

    VPLTEST_LOG("Start helper threads.");
    for (i = 0; i < NUM_THREADS; i++) {
        test_thread_id[i] = VPLTHREAD_INVALID;
        rc = VPLThread_Create(&test_thread[i],
                              sem_test_helper,
                              VPL_AS_THREAD_FUNC_ARG(i),
                              NULL, // default VPLThread thread-attributes: prio, stack-size, etc.
                              "sem helper");
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thread create failed. Code %d.", rc);
        }
    }

    VPLTEST_LOG("Waiting for helpers to be blocked (semaphores empty).");
    for (i = 0; i < NumSemaphores; i++) {
        do {
            rc = VPLSem_TryWait(&test_sem[i]);
            if (rc == VPL_OK) {
                // give it back.
                rc = VPLSem_Post(&test_sem[i]);
                if (rc != VPL_OK) {
                    VPLTEST_FAIL("Semaphore post failed. Code %d.", rc);
                }
                VPLThread_Sleep(10000);
            }
            else if (rc == VPL_ERR_AGAIN) {
                break;
            }
            else {
                VPLTEST_FAIL("Semaphore trywait failed. Code %d.", rc);
            }
        } while(1);
    }

    sem_test_helper_done = 1;
    VPLTEST_LOG("Post each semaphore once to release helpers.");
    for (i = 0; i < NumSemaphores; i++) {
        rc = VPLSem_Post(&test_sem[i]);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Semaphore post failed. Code %d.", rc);
        }
    }

    VPLTEST_LOG("Rejoin with the helper threads.");
    for (i = 0; i < NUM_THREADS; i++) {
        rc = VPLThread_Join(&test_thread[i], NULL);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("Thread join failed. Code %d.", rc);
        }
    }

    VPLTEST_LOG("Destroy the semaphores.");
    rc = VPLSem_Destroy(&test_sem[0]);
    if (rc != VPL_OK) {
        VPLTEST_FAIL("Semaphore destroy failed. Code %d.", rc);
    }

    VPLTEST_LOG("Testing semaphore invalid parameters.");
    testSemInvalidParameters();

Sem_Init_fail:
    NO_OP();
}
