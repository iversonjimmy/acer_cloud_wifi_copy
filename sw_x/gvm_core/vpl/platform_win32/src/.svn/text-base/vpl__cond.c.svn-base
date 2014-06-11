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

// Condition variables using the Vista+ Win32 native condition variable API.

int VPLCond_Init(VPLCond_t* cond)
{
    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
#ifdef VPL_THREAD_SYNC_INIT_CHECK
    if (VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_IS_INIT;
    }
#endif
    InitializeConditionVariable(&cond->cond);
    VPL__SET_INITIALIZED(cond);

    return VPL_OK;
}

int VPLCond_Signal(VPLCond_t* cond)
{
    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
    
    if (!VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_INVALID;
    }

    WakeConditionVariable(&cond->cond);

    return VPL_OK;
}

int VPLCond_Broadcast(VPLCond_t* cond)
{
    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
    
    if (!VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_INVALID;
    }

    WakeAllConditionVariable(&cond->cond);

    return VPL_OK;
}

int VPLCond_TimedWait(VPLCond_t* cond, 
                      VPLMutex_t* mutex, 
                      VPLTime_t time)
{
    int count;
    int rv = VPL_OK;
    BOOL success;
    DWORD timeOutInMS;

    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_INVALID;
    }
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    count = mutex->lockCount;
    mutex->lockCount = 0;
    mutex->lockTid = 0;

    // VPLTime_t is in microseconds.  The call takes milliseconds.
    // Make sure to round up since we must wait *at least* @a time microseconds.
    timeOutInMS = (DWORD)((time == VPL_TIMEOUT_NONE) ? INFINITE : VPLTime_ToMillisecRoundUp(time));

    success = SleepConditionVariableCS(&cond->cond,
                                        &mutex->cs,
                                        timeOutInMS);
    if (!success) {
        rv = VPLError_XlatWinErrno(GetLastError());
    }

    mutex->lockCount = count;
    mutex->lockTid = GetCurrentThreadId(); 
    return rv; 
}

int VPLCond_Destroy(VPLCond_t* cond)
{
    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
    
    if (!VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_INVALID;
    }

    // There is no Win32 destroy function
    VPL_SET_UNINITIALIZED(cond);

    return VPL_OK;
}
