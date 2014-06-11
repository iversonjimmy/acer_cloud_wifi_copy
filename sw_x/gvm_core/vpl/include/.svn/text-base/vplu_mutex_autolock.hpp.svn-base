//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLU_MUTEX_AUTOLOCK_HPP__
#define __VPLU_MUTEX_AUTOLOCK_HPP__

//============================================================================
/// @file
/// VPL utility (VPLU) operations for scoped mutexes.
//============================================================================

#include "vpl_plat.h"
#include "vpl_th.h"
#include "vplu_common.h"
#include "vplu_debug.h"
#include <assert.h>

/// Locks the mutex in the constructor and then automatically unlocks the mutex when this
/// object goes out of scope.
/// You should only declare instances of this class on the stack and never pass them around!
/// For example:
/// <pre>
/// {
///     MutexAutoLock lock(&s_myMutex);
///     // s_myMutex is now locked
///     ... do stuff ...
///     // s_myMutex will be unlocked when leaving this scope, even if we're leaving via a
///     //   goto or return statement.
/// }
/// </pre>
class MutexAutoLock {
 public:
    MutexAutoLock(VPLMutex_t* mutex)
    {
        int rv = VPLMutex_Lock(mutex);
        if (rv < 0) {
            mutexToUnlock = NULL;
            VPL_REPORT_WARN("Failed to lock mutex: %d", rv);
        } else {
            mutexToUnlock = mutex;
        }
    }
    ~MutexAutoLock()
    {
        UnlockNow();
    }
    void UnlockNow()
    {
        if (mutexToUnlock != NULL) {
            int rv = VPLMutex_Unlock(mutexToUnlock);
            if (rv < 0) {
                VPL_REPORT_WARN("Failed to unlock mutex: %d", rv);
            }
            mutexToUnlock = NULL;
        }
    }
    void Relock(VPLMutex_t* mutex)
    {
        if (mutexToUnlock != NULL) {
            VPL_REPORT_FATAL("MutexAutoLock: must call UnlockNow before Relock!");
            assert(0);
            return;
        }
        int rv = VPLMutex_Lock(mutex);
        if (rv < 0) {
            mutexToUnlock = NULL;
            VPL_REPORT_WARN("Failed to lock mutex: %d", rv);
        } else {
            mutexToUnlock = mutex;
        }
    }
 private:
    VPL_DISABLE_COPY_AND_ASSIGN(MutexAutoLock);

    VPLMutex_t* mutexToUnlock;
};

#endif // include guard
