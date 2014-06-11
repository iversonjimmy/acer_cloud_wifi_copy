//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "AutoLocks.hpp"
#include "gvm_assert.h"

//------------------------------------------------------------------------------

s32 RWLock::lock(int& threadLocalReadCount, int& threadLocalWriteCount)
{
    LOG_DEBUG("Waiting to acquire lock "FMT0xPTR" (tlr:%d, tlw:%d) as %s",
            &rwLock, threadLocalReadCount, threadLocalWriteCount, "reader");
    s32 rv = CCD_OK;

    // We can be more efficient by not making the underlying call
    // if we already have a read lock; just update our thread-local count.
    // Therefore, we should only call the underlying function if we don't
    // hold the lock.
    // VPLSlimRWLock_t is not recursive anyway, and the pthreads spec says
    // "Results are undefined if the calling thread holds a write
    // lock on rwlock at the time the call is made."
    if (!threadHasLock(threadLocalReadCount, threadLocalWriteCount)) {
        int temp_rc = VPLSlimRWLock_LockRead(&rwLock);
        if (temp_rc != 0) {
            LOG_ERROR("%s failed: %d", "VPLSlimRWLock_LockRead", temp_rc);
            rv = CCD_ERROR_LOCK_FAILED;
            goto out;
        }
    }
    threadLocalReadCount++;
out:
    LOG_DEBUG("Lock "FMT0xPTR" updated (tlr:%d, tlw:%d)",
            &rwLock, threadLocalReadCount, threadLocalWriteCount);
    return rv;
}

s32 RWLock::lockWrite(int& threadLocalReadCount, int& threadLocalWriteCount)
{
    LOG_DEBUG("Waiting to acquire lock "FMT0xPTR" (tlr:%d, tlw:%d) as %s",
            &rwLock, threadLocalReadCount, threadLocalWriteCount, "writer");
    s32 rv = CCD_OK;

    // If we already have the write lock, just update our thread-local count.
    // We must not call the underlying function.
    // VPLSlimRWLock_t is not recursive.
    // Also, the pthreads spec says "Results are undefined if the calling thread holds the
    // read-write lock (whether a read or write lock) at the time the call is made."
    if (!threadHasWriteLock(threadLocalReadCount, threadLocalWriteCount)) {

        // It is illegal to upgrade from a read lock to a write lock (it would be easy to create
        // a deadlock if two threads both wanted to do that).
        if (threadLocalReadCount > 0) {
            GVM_ASSERT_FAILED("Cannot upgrade a read lock to a write lock!");
            rv = CCD_ERROR_LOCK_USAGE;
            goto out;
        }
        int temp_rc = VPLSlimRWLock_LockWrite(&rwLock);
        if (temp_rc != 0) {
            LOG_ERROR("%s failed: %d", "VPLSlimRWLock_LockWrite", temp_rc);
            rv = CCD_ERROR_LOCK_FAILED;
            goto out;
        }
    }
    threadLocalWriteCount++;
out:
    LOG_DEBUG("Lock "FMT0xPTR" updated (tlr:%d, tlw:%d)",
        &rwLock, threadLocalReadCount, threadLocalWriteCount);
    return rv;
}

s32 RWLock::unlock(int& threadLocalReadCount, int& threadLocalWriteCount)
{
    LOG_DEBUG("Unlocking "FMT0xPTR" (tlr:%d, tlw:%d)",
                &rwLock, threadLocalReadCount, threadLocalWriteCount);
    s32 rv = CCD_OK;

    // Spec says "Results are undefined if the read-write lock rwlock is not
    // held by the calling thread."
    if (!threadHasLock(threadLocalReadCount, threadLocalWriteCount)) {
        GVM_ASSERT_FAILED("Thread did not have a lock!");
        rv = CCD_ERROR_LOCK_USAGE;
        goto out;
    }

    // Only call the underlying unlock function when releasing the final lock,
    // since we only called the underlying lock function when obtaining the
    // initial lock.
    // This works just fine because you can't upgrade a read lock to a write
    // lock, and write locks include all the privileges of a read lock.
    // Unfortunately, this precludes downgrading a write lock to a read lock, but
    // POSIX pthreads doesn't seem to allow that anyway.
    if ((threadLocalReadCount + threadLocalWriteCount) == 1) {
        if (threadLocalReadCount > 0) {
            int temp_rc = VPLSlimRWLock_UnlockRead(&rwLock);
            if (temp_rc != 0) {
                LOG_ERROR("%s failed: %d", "VPLSlimRWLock_UnlockRead", temp_rc);
                rv = CCD_ERROR_LOCK_FAILED;
                goto out;
            }
        } else {
            int temp_rc = VPLSlimRWLock_UnlockWrite(&rwLock);
            if (temp_rc != 0) {
                LOG_ERROR("%s failed: %d", "VPLSlimRWLock_UnlockWrite", temp_rc);
                rv = CCD_ERROR_LOCK_FAILED;
                goto out;
            }
        }
    }
    if (threadLocalReadCount > 0) {
        threadLocalReadCount--;
    } else {
        threadLocalWriteCount--;
    }
out:
    LOG_DEBUG("Lock "FMT0xPTR" updated (tlr:%d, tlw:%d)",
        &rwLock, threadLocalReadCount, threadLocalWriteCount);
    return rv;
}
