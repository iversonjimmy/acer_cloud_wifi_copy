//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vpl_srwlock.h"
#include "vplu.h"

#ifndef _MSC_VER
#error "This file is unlikely to compile without Visual Studio"
#endif

int VPLSlimRWLock_Init(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    InitializeSRWLock(&rwlock->l); // returns VOID
    return VPL_OK;
}

int VPLSlimRWLock_Destroy(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    // There is nothing to cleanup for a SRWLOCK.
    return VPL_OK;
}

int VPLSlimRWLock_LockRead(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    AcquireSRWLockShared(&rwlock->l); // returns VOID
    return VPL_OK;
}

#if VPLRWLOCK_ENABLE_TRYREADLOCK
int VPLSlimRWLock_TryReadLock(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    int rv = VPL_ERR_NOOP;
    UNUSED(rwlock);
    return rv;
}
#endif

int VPLSlimRWLock_LockWrite(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    AcquireSRWLockExclusive(&rwlock->l); // returns VOID
    return VPL_OK;
}

#if VPLRWLOCK_ENABLE_TRYWRITELOCK
int VPLSlimRWLock_TryWriteLock(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    int rv = VPL_ERR_NOOP;
    UNUSED(rwlock);
    return rv;
}
#endif

int VPLSlimRWLock_UnlockRead(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    ReleaseSRWLockShared(&rwlock->l); // returns VOID
    return VPL_OK;
}

int VPLSlimRWLock_UnlockWrite(VPLSlimRWLock_t* rwlock)
{
    if (rwlock == NULL) {
        return VPL_ERR_INVALID;
    }
    ReleaseSRWLockExclusive(&rwlock->l); // returns VOID
    return VPL_OK;
}
