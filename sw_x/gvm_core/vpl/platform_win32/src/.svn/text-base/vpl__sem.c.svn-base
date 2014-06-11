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

//--------------------------------
// Semaphores
// -------------------------------

int VPLSem_Init(VPLSem_t* sem,
                unsigned int maxCount,
                unsigned int value)
{
    int rv = VPL_OK;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
#ifdef VPL_THREAD_SYNC_INIT_CHECK
    if (VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_IS_INIT;
    }
#endif
    if (maxCount > VPLSEM_MAX_COUNT) {
        return VPL_ERR_INVALID;
    }
    if (value > maxCount) {
        return VPL_ERR_INVALID;
    }
#ifdef VPL_PLAT_IS_WINRT
    sem->handle = CreateSemaphoreEx(NULL,      /* default security */
                                    value,     /* initial count    */
                                    maxCount,  /* max count        */
                                    NULL,      /* not named        */
                                    0,         /* reserved         */
                                    SEMAPHORE_ALL_ACCESS);     /* default security descriptor */
#else
    sem->handle = CreateSemaphore(NULL,      /* default security */
                                  value,     /* initial count    */
                                  maxCount,  /* max count        */
                                  NULL);     /* not named        */
#endif
    if (sem->handle == NULL) {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
    }
    else {
        VPL__SET_INITIALIZED(sem);
    }

    return rv;
}

int VPLSem_Wait(VPLSem_t* sem)
{
    int rv = VPL_OK;
    DWORD rc;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    do{
        rc = WaitForSingleObjectEx(sem->handle, INFINITE, VPL_TRUE);  // Note: Ex needed for alertable in vpl__monitor_dir.cpp
    } while(rc == WAIT_IO_COMPLETION);
    if (rc != WAIT_OBJECT_0) {
        rv = VPLError_XlatWinErrno(GetLastError());
    }

    return rv;
}

int VPLSem_TimedWait(VPLSem_t* sem, VPLTime_t timeout)
{
    int rv = VPL_OK;
    DWORD rc;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    {
        VPLTime_t timeoutTime = VPLTime_GetTimeStamp() + timeout;
        do {
            VPLTime_t now = VPLTime_GetTimeStamp();
            VPLTime_t remaining = ((now < timeoutTime) ? (timeoutTime - now) : 0);
            rc = WaitForSingleObjectEx(sem->handle, (DWORD)VPLTime_ToMillisec(remaining), VPL_TRUE);  // Note: Ex needed for alertable in vpl__monitor_dir.cpp
        } while(rc == WAIT_IO_COMPLETION);
        if (rc != WAIT_OBJECT_0) {
            if (rc == WAIT_TIMEOUT) {
                rv = VPL_ERR_TIMEOUT;
            } else {
                rv = VPLError_XlatWinErrno(GetLastError());
            }
        }
    }

    return rv;
}

int VPLSem_TryWait(VPLSem_t* sem)
{
    int rv = VPL_OK;
    DWORD rvWait;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    do{
        rvWait = WaitForSingleObjectEx(sem->handle, 0, VPL_TRUE);  // Note: Ex needed for alertable in vpl__monitor_dir.cpp
    }while(rvWait == WAIT_IO_COMPLETION);
    if (rvWait == WAIT_OBJECT_0) {
    }else if (rvWait == WAIT_TIMEOUT) {
        return VPL_ERR_AGAIN;
    }
    else {
        rv = VPLError_XlatWinErrno(GetLastError());
    }

    return rv;
}

int VPLSem_Post(VPLSem_t* sem)
{
    int rv = VPL_OK;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    if(!ReleaseSemaphore(sem->handle, 1, NULL)) {
        rv = VPLError_XlatWinErrno(GetLastError());
    }

    return rv;
}

int VPLSem_Destroy(VPLSem_t* sem)
{
    int rv = VPL_OK;
    BOOL rc;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    rc = CloseHandle(sem->handle);
    if(rc) {
        VPL_SET_UNINITIALIZED(sem);
    }
    else {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
        VPL_REPORT_FATAL("Close handle failed! err=%u rv=%d handle=%p", err, rv, sem->handle);
    }

    return rv;
}
