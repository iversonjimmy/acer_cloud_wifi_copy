/*
 *                Copyright (C) 2005, BroadOn Communications Corp.
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of BroadOn Communications Corp.,
 *   and are protected by Federal copyright law. They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or in
 *   part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#include "vpl_th.h"
#include "vplu.h"

#include <windows.h>
#include <process.h>

#include <assert.h>

// Critical section implementation of VPL_Mutex__t

int VPLMutex_Init(VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }

#ifdef VPL_THREAD_SYNC_INIT_CHECK
    if (VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_IS_INIT;
    }
#endif

    mutex->lockTid = 0;
    mutex->lockCount = 0;

#ifdef VPL_PLAT_IS_WINRT
    InitializeCriticalSectionEx(&mutex->cs, 0, 0);
#else
    InitializeCriticalSection(&mutex->cs);
#endif
    VPL__SET_INITIALIZED(mutex);

    return VPL_OK;
}

int VPLMutex_Locked(const VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    return ((mutex)->lockTid != 0)? 1 : 0;
}


int VPLMutex_LockedSelf(const VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    return ((mutex)->lockTid == GetCurrentThreadId())? 1 : 0;
}

int VPLMutex_Lock(VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }

    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    if (VPLMutex_LockedSelf(mutex)) {
        mutex->lockCount++;
    }
    else
    {
        EnterCriticalSection(&mutex->cs);
        mutex->lockTid = GetCurrentThreadId();
        mutex->lockCount = 1;
    }
    
    return VPL_OK; 
}

int VPLMutex_TryLock(VPLMutex_t* mutex)
{
    BOOL success;
    int rv = VPL_OK;

    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }

    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    if (VPLMutex_LockedSelf(mutex)) {
        mutex->lockCount++;
    }
    else {
        success = TryEnterCriticalSection(&mutex->cs);
        if (success) {
            mutex->lockTid = GetCurrentThreadId();
            mutex->lockCount = 1;
        }
        else {
            rv = VPL_ERR_BUSY;
        }
    }

    return rv;
}

int VPLMutex_Unlock(VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }

    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    if (!VPLMutex_LockedSelf(mutex)) {
        return VPL_ERR_PERM;
    }
    
    if (--mutex->lockCount == 0) {
        mutex->lockTid = 0;
 
        // If the caller isn't the owner then things will go wrong
        // in a way unspecified by MSDN.  There is no return value.
        LeaveCriticalSection(&mutex->cs);
    }

    return VPL_OK; 
}

int VPLMutex_Destroy(VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }

    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    // This check is advisory only; program correctness should not rely upon this behavior.
    if (mutex->lockCount > 0) {
        return VPL_ERR_BUSY;
    }

    DeleteCriticalSection(&mutex->cs);
    VPL__SET_UNINITIALIZED(mutex);

    return VPL_OK;
}
