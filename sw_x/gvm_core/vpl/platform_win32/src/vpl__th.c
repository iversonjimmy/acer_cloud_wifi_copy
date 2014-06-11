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


//% TODO: should have a way to verify that HANDLE is void* and DWORD
//% is unsigned long

const VPLThread_t VPLTHREAD_INVALID = {0, 0, NULL, NULL};

//=============================================================
// Set a thread's name (for debugging)
//=============================================================

#if defined(_DEBUG) && defined(_MSC_VER)
    /// Modified version of code from msdn.microsoft.com
#   define MS_VC_EXCEPTION 0x406D1388

#   pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
       DWORD dwType; // Must be 0x1000.
       LPCSTR szName; // Pointer to name (in user addr space).
       DWORD dwThreadID; // Thread ID (-1=caller thread).
       DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#   pragma pack(pop)

    static void VPLThread_SetName(unsigned thread_id, char const* name)
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = thread_id;
        info.dwFlags = 0;
        //% seriously?
#ifdef VPL_PLAT_IS_WINRT
		VPLThread_Sleep(10);
#else
        Sleep(10);
#endif

        __try
            {
                RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
    }
#else
    static inline void VPLThread_SetName(unsigned thread_id, char const* name)
    {
        UNUSED(thread_id);
        UNUSED(name);
    }
#endif

// Wrap our platform-independent VPLThread_fn_t for passing to _beginthreadex.
static unsigned __stdcall VPLThread_StartWrapper(void* arg)
{
    void* fn_arg = (((VPLThread_t*)arg)->arg);
    VPLThread_fn_t fn = (((VPLThread_t*)arg)->fn);

    //% TODO: This cast discards bits on 64-bit Windows (MSVC uses LLP64).
    //%  To fix, we'll need to keep our own retval in VPLThread_t, just like on TWL.
    return PtrToUint(fn(fn_arg));
}

typedef struct VPLThread__win32Prio {
    //int sched_class;
    int schedPrio;
} VPLThread__win32Prio;

static const VPLThread__win32Prio VPLThread__defaultPrio = {
    VPL_PRIO_MIN
};

static struct VPLThread__win32Priority_map  {
     int vplPrio;
     VPLThread__win32Prio win32Prio;
} VPLThread__priomap[] = {
    {VPL_PRIO_MIN,              {0 } },
    {1,                         {1 } },
    {2,                         {2 } },
    {3,                         {3 } },
    {4,                         {4 } },
    {5,                         {5 } },
    {6,                         {6 } },
    {7,                         {7 } },
    {8,                         {8 } },
    {VPL_PRIO_MAX,              {9 } },
};

unsigned VPLThread__Num_Prio = (sizeof(VPLThread__priomap) /
                           sizeof(VPLThread__priomap[0]));

static inline int PriorityWinToVpl(VPLThread__win32Prio win32Prio)
{
    unsigned i;
    for (i = 0; i <= VPLThread__Num_Prio; i++) {
        struct VPLThread__win32Priority_map* pm = &VPLThread__priomap[i];
        if ( //(pm->win32Prio.sched_class == win32Prio.sched_class) &&
            (pm->win32Prio.schedPrio == win32Prio.schedPrio)) {
            return pm->vplPrio;
        }
    }

    // invalid priority
    return VPL_PRIO_MIN;
}

static inline VPLThread__win32Prio PriorityVplToWin(int vplPrio)
{
    if (vplPrio >= VPL_PRIO_MIN && vplPrio <= VPL_PRIO_MAX)
        return (VPLThread__priomap[vplPrio].win32Prio);

    // invalid priority
    return VPLThread__defaultPrio;
}

#ifndef VPL_PLAT_IS_WINRT
// Encapsulate setting of Win32 priority, so we can (later)
// choose to start frobbing the process' priority-class.
static int VPLThread__SetWin32Prio(void* thread, VPLThread__win32Prio win32Prio)
{
    int rv;

    rv = SetThreadPriority(thread, win32Prio.schedPrio);
#if 0
    rv = 
#endif
    return rv;
}
#endif

static const VPLThread_attr_t VPLThread_Default_Attrs = {
    (1024*1024),                     // our default stack-size
    VPL_FALSE,                       // our default for "create detached"
    VPL_PRIO_MIN,                    // our default priority
};

int VPLThread_Create(VPLThread_t* thread,
                     VPLThread_fn_t start_routine,
                     void* start_arg,
                     const VPLThread_attr_t* attrs,
                     char const* thread_name)
{
    int rv = VPL_OK;
    VPLThread__win32Prio win_prio;

    // sanity checks
    if (thread == NULL) {
        return VPL_ERR_INVALID;
    }
    if (start_routine == NULL) {
        return VPL_ERR_INVALID;
    }
    // start_arg may be NULL, so don't check it.

    if (attrs == NULL) {
        attrs = &VPLThread_Default_Attrs;
    }

    win_prio = PriorityVplToWin(attrs->vplPrio);
    thread->fn = start_routine;
    thread->arg = start_arg;

    thread->handle = (HANDLE)_beginthreadex(NULL,
                                        (unsigned)attrs->stackSize,
                                        VPLThread_StartWrapper,
                                        thread,
                                        0 /* always start running */,
                                        (unsigned *)&(thread->id));

    if (thread->handle == NULL) {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
        goto out;
    }

    // TODO: set thread name
    UNUSED(thread_name);
#ifndef VPL_PLAT_IS_WINRT
    {
        int status = VPLThread__SetWin32Prio(thread->handle, win_prio);
        if (status == 0) {
            // Thread started, just had error on setting priority... ?
            //DWORD err = GetLastError();
            //rv = VPLError_XlatWinErrno(err);
        }
    }
#endif
    if (attrs->createDetached) {
        CloseHandle(thread->handle);
        thread->handle = NULL;
    }

out:
    return rv;
}

#ifndef VPL_PLAT_IS_WINRT
void VPLThread_Exit(VPLThread_return_t retval)
{
    //% TODO: This cast discards bits on 64-bit Windows (MSVC uses LLP64).
    //% To fix, we'll need to keep our own retval in VPLThread_t, just like on TWL.
    _endthreadex(PtrToUint(retval));
}
#endif

int VPLThread_Join(VPLThread_t* thread,
                   VPLThread_return_t* retval_out)
{
    DWORD rc;
    int rv = VPL_OK;
    BOOL ch;

    if (thread == NULL) {
        return VPL_ERR_INVALID;
    }
    if (thread->handle == NULL) {
        return VPL_ERR_THREAD_DETACHED;
    }
    // TODO: GetCurrentThread returns a special constant; I don't think this can ever evaluate to true.
    if (thread->handle == GetCurrentThread()) {
        return VPL_ERR_DEADLK;
    }

    // Caller may not care about exiting thread's retval
#ifdef VPL_PLAT_IS_WINRT
    rc = WaitForSingleObjectEx(thread->handle, INFINITE, TRUE);
#else
    rc = WaitForSingleObject(thread->handle, INFINITE);
#endif

    if (rc == WAIT_OBJECT_0) {
#ifndef VPL_PLAT_IS_WINRT
        if (retval_out != NULL) {
            /* Casting since this uses a different return type than we gave _beginthreadex */
            GetExitCodeThread(thread->handle, (DWORD*)retval_out);
        }
#endif
        ch = CloseHandle(thread->handle);
        // check return value "ch"?
        thread->handle = NULL;
    }
    else if (rc == WAIT_TIMEOUT) {
        /* threads don't do TIMEOUT, so treat like any error */
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
    }
    else {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
    }

    return rv;
}

VPLThread_t VPLThread_Self(void)
{
    VPLThread_t me;
    me.handle = GetCurrentThread();
    me.id = GetCurrentThreadId();
    me.fn = NULL;
    me.arg = NULL;
    return me;
}

int VPLThread_Equal(VPLThread_t const* t1, VPLThread_t const* t2)
{
    if (t1 == NULL || t2 == NULL) {
        return VPL_ERR_INVALID;
    }
    return t1->id == t2->id;
}

void VPLThread_Sleep(VPLTime_t time)
{
    // Time is in microseconds. Convert to milliseconds
    DWORD sleep_time = (DWORD)(time/1000);
#ifdef VPL_PLAT_IS_WINRT
    WaitForSingleObjectEx(GetCurrentThread(), sleep_time, FALSE);
#else
    Sleep(sleep_time);
#endif
}

void VPLThread_Yield(void)
{
    VPLThread_Sleep(0);
}

// ------------------------------------------------------------------------

int
VPLThread_AttrInit(VPLThread_attr_t* attrs)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }

    memset(attrs, 0, sizeof(*attrs));
    attrs->stackSize = 0;
    attrs->vplPrio = -1; // marker to "inherit" priority.

    return VPL_OK;
}

int
VPLThread_AttrDestroy(VPLThread_attr_t* attrs)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }

    memset(attrs, 0, sizeof(*attrs));

    return VPL_OK;
}

int
VPLThread_AttrGetStackSize(const VPLThread_attr_t* attrs, size_t* stacksize)
{
    if (attrs == NULL || stacksize == NULL) {
        return VPL_ERR_INVALID;
    }

    (*stacksize) = attrs->stackSize;
    return VPL_OK;
}

int
VPLThread_AttrSetStackSize(VPLThread_attr_t* attrs, size_t stacksize)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }
    attrs->stackSize = stacksize;
    return  VPL_OK;
}

int
VPLThread_AttrGetPriority(const VPLThread_attr_t* attrs, int* vplPrio)
{
    if (attrs == NULL || vplPrio == NULL) {
        return VPL_ERR_INVALID;
    }
    (*vplPrio) = attrs->vplPrio;
    return VPL_OK;
}

int
VPLThread_AttrSetPriority(VPLThread_attr_t* attrs, int vplPrio)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }
    attrs->vplPrio = vplPrio;
    return VPL_OK;
}


int
VPLThread_AttrSetDetachState(VPLThread_attr_t* attrs, VPL_BOOL createDetached)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }
    attrs->createDetached = createDetached;
    return VPL_OK;
}

int
VPLThread_AttrGetDetachState(const VPLThread_attr_t* attrs, VPL_BOOL* createDetached_out)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }
    *createDetached_out = attrs->createDetached;
    return VPL_OK;
}

//% TODO: Implement set and get CPU affinity for win32.
//% For now, Windows acts as if there is only one CPU.
int VPLThread_SetAffinity(int cpu_id)
{
    if (cpu_id == 0 || cpu_id == -1)
        return VPL_OK;
    return VPL_ERR_INVALID;
}

int VPLThread_GetAffinity(void)
{
    return 0;
}

int VPLThread_GetMaxValidCpuId(void)
{
    return 0;
}

#ifndef VPL_PLAT_IS_WINRT
int VPLThread_SetSchedPriority(int vplPrio)
{
    int rv = VPL_OK;
    VPLThread__win32Prio win32Prio;
    
    if (vplPrio < VPL_PRIO_MIN || vplPrio > VPL_PRIO_MAX) {
        return VPL_ERR_INVALID;
    }

    win32Prio = PriorityVplToWin(vplPrio);
    if (VPLThread__SetWin32Prio(GetCurrentThread(), win32Prio) == 0) {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
    }
    return rv;
}
#endif

int VPLThread_GetSchedPriority(void)
{
#ifdef VPL_PLAT_IS_WINRT
    return VPL_ERR_FAIL;
#else
    VPLThread__win32Prio win32Prio;
    int vplPrio;
    int rv = GetThreadPriority(GetCurrentThread());
    if (rv == THREAD_PRIORITY_ERROR_RETURN) {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
        goto out;
    }

    win32Prio.schedPrio = rv;
    vplPrio = PriorityWinToVpl(win32Prio);
    rv = vplPrio;
 out:
    return rv;
#endif
}

//-------------------------

int VPLDetachableThread_Create(VPLDetachableThreadHandle_t* threadHandle_out,
        VPLDetachableThread_fn_t startRoutine,
        void* startArg,
        const VPLThread_attr_t* attrs,
        const char* threadName)
{
    int rv = VPL_OK;
    VPLThread__win32Prio win_prio;
    HANDLE newThread;
    unsigned threadId;

    // sanity checks
    if (startRoutine == NULL) {
        return VPL_ERR_INVALID;
    }
    // startArg may be NULL, so don't check it.
    // threadId_out may be NULL, so don't check it.

    if (attrs == NULL) {
        attrs = &VPLThread_Default_Attrs;
    }

    if (threadHandle_out == NULL) {
        if (!attrs->createDetached) {
            return VPL_ERR_INVALID;
        }
    } else {
        threadHandle_out->handle = NULL;
    }
    win_prio = PriorityVplToWin(attrs->vplPrio);

    // We use CRT functions, so we *must* use _beginthreadex instead of CreateThread.
    // From Austin Donnelly @ MSFT:
    // "
    // A quick browse of ...\VC\crt\src\tidtable.c shows that CRT (the C runtime library) keeps a per-thread data structure, pointed to from a TLS slot.
    // The per-thread data structure keeps thread-local copies of errno, pointers to some char buffers (eg for strerror(), asctime() etc), floating-point state and a smattering of other stuff.
    // This is dynamically allocated and initialised by _beginthread(), but obviously CreateThread can't do this since it knows nothing of the CRT.
    // All CRT routines which access this per-thread data structure will lazily create it if it doesn't yet exist, however there's always the risk that this dynamic allocation may fail.
    // This explains the comment "the CRT may terminate the process in low-memory conditions".
    // "
    newThread = (HANDLE)_beginthreadex(NULL,
                                        (unsigned)attrs->stackSize,
                                        startRoutine,
                                        startArg,
                                        0 /* always start running */,
                                        &threadId);

    if (newThread == NULL) {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
        goto out;
    }

    VPLThread_SetName(threadId, threadName);

#ifndef VPL_PLAT_IS_WINRT
    {
        int status = VPLThread__SetWin32Prio(newThread, win_prio);
        if (status == 0) {
            // Thread started, just had error on setting priority... ?
            //DWORD err = GetLastError();
            //rv = VPLError_XlatWinErrno(err);
            // TODO: log error
        }
    }
#endif

    if (attrs->createDetached) {
        CloseHandle(newThread);
    } else {
        threadHandle_out->handle = newThread;
    }
    //if (threadId_out != NULL) {
    //    *threadId_out = threadId;
    //}

out:
    return rv;
}

int VPLDetachableThread_Detach(VPLDetachableThreadHandle_t* handle)
{
    BOOL tempRes;
    if (handle == NULL) {
        return VPL_ERR_INVALID;
    }
    if (handle->handle == NULL) {
        return VPL_ERR_ALREADY;
    }
    tempRes = CloseHandle(handle->handle);
    if (!tempRes) {
        // TODO: unexpected: log error
    }
    handle->handle = NULL;
    return VPL_OK;
}

int VPLDetachableThread_Join(VPLDetachableThreadHandle_t* handle)
{
    DWORD rc;
    int rv = VPL_OK;

    if (handle == NULL) {
        return VPL_ERR_INVALID;
    }
    if (handle->handle == NULL) {
        return VPL_ERR_ALREADY;
    }
    // GetThreadId() doesn't exist until Vista.
    //if (GetThreadId(handle->handle) == GetCurrentThreadId()) {
    //    return VPL_ERR_DEADLK;
    //}
#ifdef VPL_PLAT_IS_WINRT
    rc = WaitForSingleObjectEx(handle->handle, INFINITE, TRUE);
#else
    rc = WaitForSingleObject(handle->handle, INFINITE);
#endif

    if (rc == WAIT_OBJECT_0) {
        BOOL tempRes = CloseHandle(handle->handle);
        if (!tempRes) {
            // TODO: log error
        }
        handle->handle = NULL;
    }
    else {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
    }
    return rv;
}

VPLThreadId_t VPLDetachableThread_Self(void)
{
    return GetCurrentThreadId();
}
