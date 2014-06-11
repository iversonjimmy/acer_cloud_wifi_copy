//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __AUTOLOCKS_HPP__
#define __AUTOLOCKS_HPP__

// TODO: This file is not named accurately anymore.  The actual "autolocks" were refactored elsewhere,
//   and what's left here (RWLock) is not an autolock.

#include "vpl_srwlock.h"
#include "vplu_mutex_autolock.hpp"

#include "base.h"

/// Utility class that helps to build a recursive reader/writer lock using a
/// VPLSlimRWLock (which is non-recursive).
/// To avoid having a variably-sized data structure (for tracking all of the reader threads), each
/// thread is required to maintain its own thread-local read count and thread-local write count
/// for each RWLock.
class RWLock {
 public:
    RWLock()
    {
        int rv = VPLSlimRWLock_Init(&rwLock);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "VPLSlimRWLock_Init", rv);
        }
    }

    static VPL_BOOL threadHasLock(int& threadLocalReadCount, int& threadLocalWriteCount)
    {
        return (threadLocalReadCount + threadLocalWriteCount) > 0;
    }

    static VPL_BOOL threadHasWriteLock(int& threadLocalReadCount, int& threadLocalWriteCount)
    {
        return threadLocalWriteCount > 0;
    }

    s32 lock(int& threadLocalReadCount, int& threadLocalWriteCount);

    /// This will fail if the calling thread already has the reader lock.
    s32 lockWrite(int& threadLocalReadCount, int& threadLocalWriteCount);

    /// If the calling thread holds both reader and writer locks, we always decrement reader locks
    /// first, since it can never acquire any writer locks if it initially acquired a reader lock.
    s32 unlock(int& threadLocalReadCount, int& threadLocalWriteCount);

    ~RWLock()
    {
        VPLSlimRWLock_Destroy(&rwLock);
    }

 private:
    VPL_DISABLE_COPY_AND_ASSIGN(RWLock);
    VPLSlimRWLock_t rwLock;
};

#endif // include guard
