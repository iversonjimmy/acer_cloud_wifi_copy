//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplTest.h"

#include "vpl_error.h"
#include "vpl_srwlock.h"
#include "vpl_th.h"

#define NUM_LOCKS 1

#ifdef ANDROID
// Our Android implementation currently falls back to mutexes; this test will hang with more than 1 thread.
/// Number of helper threads (not counting main thread).
#  define NUM_THREADS 0
#else
/// Number of helper threads (not counting main thread).
#  define NUM_THREADS 10
#endif

enum {
    TEST_OP_RDLOCK,
    TEST_OP_WRLOCK
};

struct helper_args {
    VPLSlimRWLock_t* lock;
    VPLSem_t sem_locked;
    VPLSem_t sem_do_unlock;
    int operation;
    int threadNum;
};

static void testInvalidParameters(void)
{
    VPLTEST_CALL_AND_CHK_RV(VPLSlimRWLock_Init(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSlimRWLock_Destroy(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSlimRWLock_LockRead(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSlimRWLock_LockWrite(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSlimRWLock_UnlockRead(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLSlimRWLock_UnlockWrite(NULL), VPL_ERR_INVALID);
}

/// Helper will lock the specified lock as requested, post its "locked" semaphore, and then wait on
/// its "do_unlock" semaphore.
/// Once the "do_unlock" semaphore is posted, this will unlock the lock and exit.
static VPLThread_return_t rwlock_helper(VPLThread_arg_t arg)
{
    int rc;
    struct helper_args* args = (struct helper_args*)(arg);

    switch(args->operation) {
    case TEST_OP_RDLOCK:
        VPLTEST_DEBUG("Thread %d: acquiring reader lock", args->threadNum);
        rc = VPLSlimRWLock_LockRead(args->lock);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("RWLock helper failed to get read lock, code %d.", rc);
        }
        break;
    case TEST_OP_WRLOCK:
        VPLTEST_DEBUG("Thread %d: acquiring writer lock", args->threadNum);
        rc = VPLSlimRWLock_LockWrite(args->lock);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("RWLock helper failed to get write lock, code %d.", rc);
        }
        break;
    default:
        VPLTEST_FAIL("Unexpected: %d", args->operation);
        break;
    }
    VPLTEST_DEBUG("Thread %d: acquired lock", args->threadNum);

    rc = VPLSem_Post(&args->sem_locked);
    if (rc != 0) {
        VPLTEST_FAIL("%s failed: %d.", "VPLSem_Post", rc);
    }

    rc = VPLSem_Wait(&args->sem_do_unlock);
    if (rc != 0) {
        VPLTEST_FAIL("%s failed: %d.", "VPLSem_Wait", rc);
    }
    
    switch(args->operation) {
    case TEST_OP_RDLOCK:
        rc = VPLSlimRWLock_UnlockRead(args->lock);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("RWLock helper failed to unlock reader, code %d.", rc);
        }
        break;
    case TEST_OP_WRLOCK:
        rc = VPLSlimRWLock_UnlockWrite(args->lock);
        if (rc != VPL_OK) {
            VPLTEST_FAIL("RWLock helper failed to unlock writer, code %d.", rc);
        }
        break;
    default:
        VPLTEST_FAIL("Unexpected: %d", args->operation);
        break;
    }
    return VPL_AS_THREAD_RETVAL(0);
}

void testVPLSlimRWLock(void)
{
    VPLSlimRWLock_t test_lock[NUM_LOCKS];
    VPLThread_t helper_thread[NUM_THREADS];
    struct helper_args thread_args[NUM_THREADS];
    int rc;
    int i;

    VPLTEST_LOG("Testing invalid parameters for RW locks.");
    testInvalidParameters();

    // preliminaries: initialize the test semaphores.
    for (i = 0; i < NUM_THREADS; i++) {
        rc = VPLSem_Init(&(thread_args[i].sem_locked), 1, 0);
        if (rc != VPL_OK) {
            VPLTEST_NONFATAL_ERROR("%s failed: %d.", "VPLSem_Init", rc);
            goto Sem_Init_fail;
        }
        rc = VPLSem_Init(&(thread_args[i].sem_do_unlock), 1, 0);
        if (rc != VPL_OK) {
            VPLTEST_NONFATAL_ERROR("%s failed: %d.", "VPLSem_Init", rc);
            goto Sem_Init_fail;
        }
    }

    // Initialize locks
    for (i = 0; i < NUM_LOCKS; i++) {
        rc = VPLSlimRWLock_Init(&(test_lock[i]));
        if (rc != VPL_OK) {
            VPLTEST_NONFATAL_ERROR("%s failed: %d.", "VPLSlimRWLock_Init", rc);
            goto Lock_Init_fail;
        }
    }

    // Read lock tests:
    // * Able to acquire read lock
    // * Multiple threads can acquire read lock
    
    // Set-up helpers to acquire read lock.
    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i].lock = &(test_lock[0]);
        thread_args[i].operation = TEST_OP_RDLOCK;
        thread_args[i].threadNum = i;

        // Launch helper thread. It will take read lock and wait.
        rc = VPLThread_Create(&helper_thread[i],
                              rwlock_helper,
                              VPL_AS_THREAD_FUNC_ARG(&thread_args[i]),
                              0, // default VPLThread attributes
                              "test_reader");
        VPLTEST_ENSURE_OK(rc, "VPLThread_Create");
    }

    for (i = 0; i < NUM_THREADS; i++) {
        VPLTEST_DEBUG("Wait for semaphore from thread %d", i);
        rc = VPLSem_Wait(&thread_args[i].sem_locked);
        VPLTEST_ENSURE_OK(rc, "VPLSem_Wait");
    }

    rc = VPLSlimRWLock_LockRead(&test_lock[0]);
    VPLTEST_ENSURE_OK(rc, "VPLSlimRWLock_LockRead");

    for (i = 0; i < NUM_THREADS; i++) {
        rc = VPLSem_Post(&thread_args[i].sem_do_unlock);
        VPLTEST_ENSURE_OK(rc, "VPLSem_Post");
    }

    rc = VPLSlimRWLock_UnlockRead(&test_lock[0]);
    VPLTEST_ENSURE_OK(rc, "VPLSlimRWLock_UnlockRead");

    for (i = 0; i < NUM_THREADS; i++) {
        rc = VPLThread_Join(&helper_thread[i], NULL);
        VPLTEST_ENSURE_OK(rc, "VPLThread_Join");
    }

    // Write lock tests:
    // * Able to acquire write lock

    rc = VPLSlimRWLock_LockWrite(&test_lock[0]);
    VPLTEST_ENSURE_OK(rc, "VPLSlimRWLock_LockWrite");

    rc = VPLSlimRWLock_UnlockWrite(&test_lock[0]);
    VPLTEST_ENSURE_OK(rc, "VPLSlimRWLock_UnlockWrite");

 Lock_Init_fail:
    for (i = 0; i < NUM_LOCKS; i++) {
        rc = VPLSlimRWLock_Destroy(&(test_lock[i]));
        VPLTEST_CHK_OK(rc, "VPLSlimRWLock_Destroy");
    }
    
 Sem_Init_fail:
    for (i = 0; i < NUM_THREADS; i++) {
        rc = VPLSem_Destroy(&(thread_args[i].sem_locked));
        VPLTEST_CHK_OK(rc, "VPLSem_Destroy");
        rc = VPLSem_Destroy(&(thread_args[i].sem_do_unlock));
        VPLTEST_CHK_OK(rc, "VPLSem_Destroy");
    }
}
