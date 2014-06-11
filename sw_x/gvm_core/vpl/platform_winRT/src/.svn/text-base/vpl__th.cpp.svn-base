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
#include "vpl_lazy_init.h"
#include "vplu.h"
#include "log.h"

#include <windows.h>
#include <map>
#include <thread>

#include <assert.h>


//% TODO: should have a way to verify that HANDLE is void* and DWORD
//% is unsigned long

const VPLThread_t VPLTHREAD_INVALID = {-1, NULL, NULL, NULL};

//=============================================================
// STD Thread Pool
//=============================================================
struct _ThreadData
{
    int handle;
    std::thread* th;
    bool bFinished;
};

static std::map<int,_ThreadData*> s_ThreadPool;
static VPLLazyInitMutex_t s_poolmutex = VPLLAZYINITMUTEX_INIT;
//bug 2353: Change ThreadPool key generate rule to avoid random key duplicate, which cause new std thread failed to detach
static int s_poolkey = 0;
static int AddToThreadPool(_ThreadData* data)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
    int ret = 0;
    //gen polling key
    if( s_ThreadPool.size() > 0) {
        do {
            s_poolkey++;
            if(s_poolkey == INT_MAX)
                s_poolkey = 0;
            ret = s_poolkey;
        }
        while( s_ThreadPool.count(ret) > 0 );
    }
    //put to map
    s_ThreadPool.insert( std::pair<int,_ThreadData*>(ret,data));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
    return ret;
}

_ThreadData* GetThreadFromPool(int key)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
    _ThreadData *ret = NULL;
    if(s_ThreadPool.size() > 0) {
        if(s_ThreadPool.count(key) > 0) {
            ret = s_ThreadPool.find(key)->second;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
    return ret;
}

static void SetFinished(int key)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
    if(s_ThreadPool.size() > 0) {
        if(s_ThreadPool.count(key) > 0) {
            s_ThreadPool.find(key)->second->bFinished = true;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
}

static void RemoveFromThreadPool(int key)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
    if(s_ThreadPool.size() > 0) {
        if(s_ThreadPool.count(key) > 0) {
            _ThreadData *data = s_ThreadPool.find(key)->second;
            s_ThreadPool.erase(key);
            if( data->th != nullptr )
                if( data->th->joinable() )
                    data->th->detach();
            delete data;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_poolmutex));
}


//=============================================================
// STD Detachable Thread Pool
//=============================================================

static std::map<int,_ThreadData*> s_DetachableThreadPool;
static VPLLazyInitMutex_t s_detachablepoolmutex = VPLLAZYINITMUTEX_INIT;
//bug 2353: Change ThreadPool key generate rule to avoid random key duplicate, which cause new std thread failed to detach
static int s_detachabpoolkey = 0;
static int AddToDetachableThreadPool(_ThreadData* data)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
    int ret = 0;
    //gen polling key
    if( s_DetachableThreadPool.size() > 0) {
        do {
            s_detachabpoolkey++;
            if(s_detachabpoolkey == INT_MAX)
                s_detachabpoolkey = 0;
            ret = s_detachabpoolkey;
        }
        while( s_DetachableThreadPool.count(ret) > 0 );
    }
    //put to map
    s_DetachableThreadPool.insert( std::pair<int,_ThreadData*>(ret,data));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
    return ret;
}

_ThreadData* GetDetachableThreadFromPool(int key)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
    _ThreadData *ret = NULL;
    if(s_DetachableThreadPool.size() > 0) {
        if(s_DetachableThreadPool.count(key) > 0) {
            ret = s_DetachableThreadPool.find(key)->second;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
    return ret;
}

static void SetDetachableFinished(int key)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
    if(s_DetachableThreadPool.size() > 0) {
        if(s_DetachableThreadPool.count(key) > 0) {
            s_DetachableThreadPool.find(key)->second->bFinished = true;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
}

static void RemoveFromDetachableThreadPool(int key)
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
    if(s_DetachableThreadPool.size() > 0) {
        if(s_DetachableThreadPool.count(key) > 0) {
            _ThreadData *data = s_DetachableThreadPool.find(key)->second;
            s_DetachableThreadPool.erase(key);
            if( data->th != nullptr )
                if( data->th->joinable() )
                    data->th->detach();
            delete data;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_detachablepoolmutex));
}


//=============================================================

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

static inline VPLThread__win32Prio PriorityVplToWin(int vplPrio)
{
    if (vplPrio >= VPL_PRIO_MIN && vplPrio <= VPL_PRIO_MAX)
        return (VPLThread__priomap[vplPrio].win32Prio);

    // invalid priority
    return VPLThread__defaultPrio;
}

static const VPLThread_attr_t VPLThread_Default_Attrs = {
    (1024*1024),                     // our default stack-size
    VPL_FALSE,                       // our default for "create detached"
    VPL_PRIO_MIN,                    // our default priority
};

// Wrap our platform-independent VPLThread_fn_t for passing to std::thread.
static void VPLThread_StartWrapper(void* arg, bool createDetached)
{
    VPLThread_t* thread = (VPLThread_t*)arg;
    void* fn_arg = (thread->arg);
    VPLThread_fn_t fn = thread->fn;
    //execute really job function
    PtrToUint(fn(fn_arg));

    SetFinished(thread->handle);
    if(createDetached) {
        RemoveFromThreadPool(thread->handle);
    }
}

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

    //initial data
    _ThreadData *data = new _ThreadData();
    thread->handle = AddToThreadPool(data);
    data->handle = thread->handle;
    data->bFinished = false;

    //create std thread
    data->th = new std::thread( VPLThread_StartWrapper, thread, attrs->createDetached );
    if (data->th == NULL) {
        LOG_TRACE("std::thread create failed");
        rv = VPLError_XlatWinErrno(GetLastError());
        RemoveFromThreadPool(data->handle);
        goto out;
    }
    thread->id = data->th->get_id().hash();

    // TODO: set thread name
    UNUSED(thread_name);

    if (attrs->createDetached) {
        data->th->detach();
    }

out:
    return rv;
}

int VPLThread_Join(VPLThread_t* thread,
                   VPLThread_return_t* retval_out)
{
    DWORD rc;
    int rv = VPL_OK;

    if (thread == NULL) {
        return VPL_ERR_INVALID;
    }

    _ThreadData *data = GetThreadFromPool(thread->handle);
    //if the thread job finished before join called, we do not wait but just return VPL_OK 
    if( data != NULL && data->bFinished ) { 
        goto end;
    }
    if (thread->id == std::this_thread::get_id().hash()) {
        return VPL_ERR_DEADLK;
    }
    if(data == NULL || !data->th->joinable()) {
        return VPL_ERR_THREAD_DETACHED;
    }
    

    WaitForSingleObjectEx(data->th->native_handle() ,INFINITE, false);

end:
    //detach the thread & remove from pool
    RemoveFromThreadPool(thread->handle);

    return rv;
}

VPLThread_t VPLThread_Self(void)
{
    VPLThread_t me;

    me.handle = 0;
    me.id = std::this_thread::get_id().hash();
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
    std::chrono::duration<int,std::milli> std_sleep_time(sleep_time);
    std::this_thread::sleep_for(std_sleep_time);
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

int VPLThread_GetSchedPriority(void)
{
    return VPL_ERR_NOOP;
}

//-------------------------

// Wrap our platform-independent VPLThread_fn_t for passing to std::thread.
static void VPLThread_StartDetachableWrapper(int handle, VPLDetachableThread_fn_t fn, void* fn_arg, bool createDetached)
{
    //execute really job function
    PtrToUint(fn(fn_arg));

    SetDetachableFinished(handle);
    if(createDetached) {
        RemoveFromDetachableThreadPool(handle);
    }
}

int VPLDetachableThread_Create(VPLDetachableThreadHandle_t* threadHandle_out,
        VPLDetachableThread_fn_t startRoutine,
        void* startArg,
        const VPLThread_attr_t* attrs,
        const char* threadName)
{
    int rv = VPL_OK;
    VPLThread__win32Prio win_prio;

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

    //initial data
    _ThreadData *data = new _ThreadData();
    data->handle = AddToDetachableThreadPool(data);
    data->bFinished = false;

    //create std thread
    data->th = new std::thread( VPLThread_StartDetachableWrapper, data->handle, startRoutine, startArg, attrs->createDetached);
    if (data->th == NULL) {
        LOG_TRACE("std::thread create failed");
        rv = VPLError_XlatWinErrno(GetLastError());
        RemoveFromDetachableThreadPool(data->handle);
        goto out;
    }

    if (threadHandle_out != NULL) {
        threadHandle_out->handle = data->handle;
        threadHandle_out->id = data->th->get_id().hash();
    }

    if (attrs->createDetached) {
        data->th->detach();
    } 

out:
    return rv;
}

int VPLDetachableThread_Detach(VPLDetachableThreadHandle_t* handle)
{
    BOOL tempRes;
    if (handle == NULL) {
        return VPL_ERR_INVALID;
    }

    _ThreadData *data = GetDetachableThreadFromPool(handle->handle);
    if ( data == NULL || !data->th->joinable() ) {
        return VPL_ERR_ALREADY;
    }

    data->th->detach();
    return VPL_OK;
}

int VPLDetachableThread_Join(VPLDetachableThreadHandle_t* handle)
{
    DWORD rc;
    int rv = VPL_OK;

    if (handle == NULL) {
        return VPL_ERR_INVALID;
    }
    
    _ThreadData *data = GetDetachableThreadFromPool(handle->handle);
    //if the thread job finished before join called, we do not wait but just return VPL_OK 
    if( data != NULL && data->bFinished ) { 
        goto end;
    }
    if ( data == NULL || !data->th->joinable() ) {
        return VPL_ERR_ALREADY;
    }

    if (handle->id == std::this_thread::get_id().hash()) {
        return VPL_ERR_DEADLK;
    }

    WaitForSingleObjectEx(data->th->native_handle() ,INFINITE, false);

end:
    //detach the thread & remove from pool
    RemoveFromDetachableThreadPool(handle->handle);

    return rv;
}

VPLThreadId_t VPLDetachableThread_Self(void)
{
    return GetCurrentThreadId();
}
