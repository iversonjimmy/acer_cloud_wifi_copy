//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_SCOPED_LOCKS_HPP__
#define __VPL_SCOPED_LOCKS_HPP__

//============================================================================
/// @file
/// Implements C++ scoped locking patterns.
//============================================================================

#include "vplex_plat.h"
#include "vplex_trace.h"
#include "vpl_th.h"

/// Lock the mutex in the constructor, then automatically unlocks the mutex when this object
/// goes out of scope.
/// You should only declare instances of this class on the stack and never pass them around!
class VPLScopedMutex {
 public:
    VPLScopedMutex(VPLMutex_t* mutex)
    {
        int rv = VPLMutex_Lock(mutex);
        if (rv < 0) {
            mutexToUnlock = NULL;
            VPLTRACE_LOG_ERR(VPLTRACE_GRP_VPL, VPL_SG_TH, "Failed to lock mutex: %d", rv);
        } else {
            mutexToUnlock = mutex;
        }
    }
    ~VPLScopedMutex()
    {
        if (mutexToUnlock != NULL) {
            int rv = VPLMutex_Unlock(mutexToUnlock);
            if (rv < 0) {
                VPLTRACE_LOG_ERR(VPLTRACE_GRP_VPL, VPL_SG_TH, "Failed to unlock mutex: %d", rv);
            }
        }
    }
 private:
    VPL_DISABLE_COPY_AND_ASSIGN(VPLScopedMutex);

    VPLMutex_t* mutexToUnlock;
};

#endif // include guard
