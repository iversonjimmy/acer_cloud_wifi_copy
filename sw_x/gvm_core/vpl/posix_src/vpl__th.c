//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_th.h"

#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "vplu.h"

const VPLThread_t VPLTHREAD_INVALID = { .thread = 0 };

/// @brief A "native" POSIX thread priority: a scheduler policy plus
/// a params struct, which contains the actual POSIX thread priority level.
typedef struct {
    int policy;
    struct sched_param params;
} VPLThread__prio_mapping_t;

// Forward declarations of "helper" functions for handling thread-attributes..

// Map from VPL priorities to a "native" POSIX thread priority.
static int
VPLThread_MapPrioToNativePrio(VPLThread__prio_mapping_t* nativePrio, int vplPrio);

static void
VPLThread_MapAttrsToNativeAttrs(pthread_attr_t* nativeAttrs, const VPLThread_attr_t* vplAttrs);

#define VPLTHREAD__PRIO_INHERIT (-1)

/// Template for the default thread-attributes for #VPLThread_Create(),
/// in the case where @a attrs is NULL.
static const VPLThread_attr_t vplThread__default_attrs = {
    VPLTHREAD_STACKSIZE_DEFAULT,    // Documented VPL stack-size.
    VPL_FALSE,
    VPLTHREAD__PRIO_INHERIT,        // To be overwritten at runtime with the calling thread's priority.
    SCHED_OTHER,                    // Default nativeSchedPolicy.
    0,                              // Default nativeSchedPrio.
    NULL                            // Default stack address (unused).
};

int
VPLThread_Create(VPLThread_t* thread,
                 VPLThread_fn_t startRoutine,
                 void* startArg,
                 const VPLThread_attr_t* vplAttrs,
                 char const* threadName)
{
    int rc;
    int rv = VPL_OK;
    pthread_attr_t pthreadAttrs;
    pthread_t pthread;
    // Make a copy so we can modify it.
    VPLThread_attr_t myAttrs = vplThread__default_attrs;

    // unused parameters
    (void)threadName;

    // sanity checks
    if (thread == NULL) {
        return VPL_ERR_INVALID;
    }
    if (startRoutine == NULL) {
        return VPL_ERR_INVALID;
    }
    // start_arg may be NULL, so don't check it.

    // Map the VPLThread_attrs attributes onto pthread_attr_t attributes.
    if (vplAttrs != NULL) {
        myAttrs = *vplAttrs;
    }

    // Late-bind any inheritance of the creating thread's priority.
    if (myAttrs.vplPrio == VPLTHREAD__PRIO_INHERIT) {
        myAttrs.vplPrio = VPLThread_GetSchedPriority();
    }

    pthread_attr_init(&pthreadAttrs);
    VPLThread_MapAttrsToNativeAttrs(&pthreadAttrs, &myAttrs);
    // TODO: do we expose {,non}inheritance of priority?

    {
        size_t tempStackSize;
        pthread_attr_getstacksize(&pthreadAttrs, &tempStackSize);
        VPL_REPORT_INFO("Creating thread with stack size = "FMTu_size_t, tempStackSize);
    }
    rc = pthread_create(&pthread, &pthreadAttrs, startRoutine, startArg);
    thread->thread = pthread;
    if (myAttrs.createDetached) {
        thread->thread = 0;
    }

    if (rc == 0) {
        rv = VPL_OK;
    } else {
        rv = VPLError_XlatErrno(rc);
    }

    pthread_attr_destroy(&pthreadAttrs);

    return rv;
}

void
VPLThread_Exit(VPLThread_return_t retval)
{
    pthread_exit(retval);
}

int
VPLThread_Join(VPLThread_t* thread,
               VPLThread_return_t* retvalOut)
{
    int rc;
    int rv = VPL_OK;

    if (thread == NULL) {
        return VPL_ERR_INVALID;
    }
    if (thread->thread == pthread_self()) {
        return VPL_ERR_DEADLK;
    }

    // Caller may not care about exiting thread's retval
    rc = pthread_join((pthread_t)(thread->thread), retvalOut);
    if (rc != 0) {
        rv = VPLError_XlatErrno(rc);
    }

    return rv;
}

VPLThread_t
VPLThread_Self(void)
{
    VPLThread_t self;

    memset(&self, 0, sizeof(self));
    self.thread = pthread_self();
    // TODO: extract remaining thread parameters!
    return self;
}

int
VPLThread_Equal(VPLThread_t const* t1, VPLThread_t const* t2)
{
    if (t1 == NULL || t2 == NULL) {
        return VPL_ERR_INVALID;
    }
    return pthread_equal((pthread_t)(t1->thread), (pthread_t)(t2->thread));
}

void
VPLThread_Sleep(VPLTime_t time)
{
    struct timespec req;

    if (time == 0) {
        VPLThread_Yield();
    }
    else {
        req.tv_sec = (time/1000000);
        req.tv_nsec = (time%1000000)*1000;
        nanosleep(&req, NULL);
    }
}

/// Causes the calling thread to yield.
void
VPLThread_Yield(void)
{
    sched_yield();
}

// ------------------------------------------------------------------------
// CPU affinity ...
// ------------------------------------------------------------------------
#if !defined(ANDROID) && !defined(IOS)
/*
 * The CPU affinity system calls were introduced in Linux kernel 2.5.8.
 * The library interfaces were introduced in glibc 2.3. Initially, the 
 * glibc interfaces included a cpusetsize argument. In glibc 2.3.2, the
 * cpusetsize argument was removed, but this argument was restored in
 * glibc 2.3.4. 
 */
int
VPLThread_SetAffinity(int cpuId)
{
    int rv = VPL_ERR_DISABLED;
    cpu_set_t set;
    pid_t thread_pid = syscall(SYS_gettid);
    int max_cpu = VPLThread_GetMaxValidCpuId();

    CPU_ZERO(&set);

    if (cpuId == -1) { // Any CPU (no affinity)
        //% Reserved processors are already masked out.
        //% Map the rest to their platform IDs.
        int i = 0;
        for (i = 0; i <= max_cpu; i++) {
            CPU_SET(i, &set);
        }
    }
    else { // A specific CPU
        //% Map cpu_id to the appropriate platform CPU ID.
        //% If any CPU ID is reserved, mapping should filter it out.
        //% Should also verify the mapped CPU actually exists.
        CPU_SET(cpuId, &set);
    }

    rv = sched_setaffinity(thread_pid, sizeof(cpu_set_t), &set);
    if (rv == -1) {
        rv = VPLError_XlatErrno(errno);
    }
    else {
        rv = VPL_OK;
    }

    return rv;
}

int
VPLThread_GetAffinity(void)
{
    int rv = -1;
    cpu_set_t set;
    int i;
    int max_cpu = VPLThread_GetMaxValidCpuId();
    pid_t thread_pid = syscall(SYS_gettid);

    CPU_ZERO(&set);

    rv = sched_getaffinity(thread_pid, sizeof(cpu_set_t), &set);
    if (rv == -1) {
        rv = VPLError_XlatErrno(errno);
    }
    else {
        //% If more than one CPU is in the set, assume no affinity.
        rv = -1;
        for (i = 0; i <= max_cpu; i++) {
            if (CPU_ISSET(i, &set)) {
                if (rv == -1) {
                    // No affinity known. Set for this CPU.
                    rv = i;
                }
                else {
                    // More than one CPU in affinity set. No affinity.
                    rv = -1;
                    break;
                }
            }
        }
    }

    //% Map the platform CPU ID back to the API cpu_id to return.
    return rv;
}
#endif

#ifdef IOS
int
VPLThread_SetAffinity(int cpuId)
{
    return -1; //TODO: Porting for iOS.
}

int
VPLThread_GetAffinity(void)
{
    return -1; //TODO: Porting for iOS.
}
#endif

int
VPLThread_GetMaxValidCpuId(void)
{
    int rv = sysconf(_SC_NPROCESSORS_CONF) - 1;
    //% Reduce this by the number of reserved CPUs for the platform
    //% if they are present.
    return rv;
}

// ------------------------------------------------------------------------
// CPU thread priorities...
// ------------------------------------------------------------------------

/// Map VPL priorities to underlying GVM-RT (Linux/Unix) pthread priorities.
///
/// Linux only allows nonzero sched_priority when the policy is set to
/// some non-default value. In other words:
/// SCHED_OTHER and SCHED_BATCH require sched_priority == 0.
/// SCHED_FIFO and SCHED_RR require sched_priority != 0.

//% Map VPL thread priority to POSIX thread priority.
//% Assumes that input is between VPL_PRIO_MIN and VPL_PRIO_MAX.
static int
VPLThread_PrioMapHelper(int vplPrio)
{
    int pthreadMinPrio = 1;
    int pthreadMaxPrio = VPLTHREAD_PTHREAD_PRIO_MAX;
    int vplMinPrio = VPL_PRIO_MIN;
    int vplMaxPrio = VPL_PRIO_MAX;

    if (vplMaxPrio == vplMinPrio) {
        // Prevent division by zero later.
        return pthreadMinPrio;
    }

    // Priority is linearly mapped from the VPL thread priority to POSIX thread
    // priority.
    double relativePrio = (double)(vplPrio - vplMinPrio) /
            (double)(vplMaxPrio - vplMinPrio);
    int nativeSchedPrio = pthreadMinPrio +
            (int)(relativePrio * (pthreadMaxPrio - pthreadMinPrio));

    return nativeSchedPrio;
}

static int
VPLThread_NativeToVPLPrioMapHelper(int nativePrio)
{
    int pthreadMinPrio = 1;
    int pthreadMaxPrio = VPLTHREAD_PTHREAD_PRIO_MAX;
    int vplMinPrio = VPL_PRIO_MIN;
    int vplMaxPrio = VPL_PRIO_MAX;
    
    if (pthreadMaxPrio == pthreadMinPrio) {
        // Prevent division by zero later.
        return vplMinPrio;
    }
    // Check if it's out of range
    if ((nativePrio < pthreadMinPrio) || (nativePrio > pthreadMaxPrio)) {
        return VPL_ERR_INVALID;
    }
    
    // Priority is linearly mapped from the POSIX thread priority to VPL thread
    // priority.
    double relativePrio = (double)(nativePrio - pthreadMinPrio) /
            (double)(pthreadMaxPrio - pthreadMinPrio);
    int vplPrio = vplMinPrio + (int)(relativePrio * (vplMaxPrio - vplMinPrio));
    
    return vplPrio;
}

static int
VPLThread_MapPrioToNativePrio(VPLThread__prio_mapping_t* nativePrio, int vplPrio)
{
    // Special case: default POSIX thread priority.
    if (vplPrio == VPL_PRIO_MIN) {
        nativePrio->policy = SCHED_OTHER;
        nativePrio->params.sched_priority = 0;
        return VPL_OK;
    }

    // Otherwise, policy is fixed to SCHED_RR.
    nativePrio->policy = SCHED_RR;

    if (vplPrio < VPL_PRIO_MIN || vplPrio > VPL_PRIO_MAX) {
        // Can't do anything with an invalid VPL thread priority.
        nativePrio->params.sched_priority = VPLTHREAD__PRIO_INHERIT;
        return VPL_ERR_INVALID;
    }

    nativePrio->params.sched_priority = VPLThread_PrioMapHelper(vplPrio);

    return VPL_OK;
}

int
VPLThread_SetSchedPriority(int vplPrio)
{
    int rv = VPL_ERR_DISABLED;
    VPLThread__prio_mapping_t nativePrio;
    memset(&nativePrio, 0, sizeof(nativePrio));
    int status;

    // Check for out-of-bounds value for VPL priority.
    if (vplPrio < VPL_PRIO_MIN || vplPrio > VPL_PRIO_MAX) {
        return VPL_ERR_INVALID;
    }

    // Map VPL priority to a corresponding (scheduler policy, params) pair.
    // The mapping cannot fail.
    status = VPLThread_MapPrioToNativePrio(&nativePrio, vplPrio);
    if (status != VPL_OK) {
        // This is a VPL internal error.
        VPL_REPORT_FATAL("internal error, invalid priority %d", vplPrio);
        return VPL_ERR_INVALID;
    }

    rv = pthread_setschedparam(pthread_self(), nativePrio.policy, &(nativePrio.params));
    if (rv != 0) {
        VPL_REPORT_WARN("pthread_setschedparam(policy=%d, prio=%d) failed with %d: %s",
                nativePrio.policy, nativePrio.params.sched_priority, rv, strerror(errno));
        rv = VPLError_XlatErrno(rv);
    }

    return rv;
}

int
VPLThread_GetSchedPriority(void)
{
    int rv = -1;
    int policy;
    struct sched_param params;
    
    rv = pthread_getschedparam(pthread_self(), &policy, &params);
    if (rv != 0) {
        VPL_REPORT_WARN("pthread_getschedparam failed with %d", rv);
        rv = VPLError_XlatErrno(rv);
        goto out;
    }
    
    // Special case: default POSIX thread priority.
    if (policy == SCHED_OTHER && params.sched_priority == 0) {
        rv = VPL_PRIO_MIN;
        goto out;
    }
    
    // Map POSIX thread priority to VPL thread priority
    rv = VPLThread_NativeToVPLPrioMapHelper(params.sched_priority);
    
out:
    return rv;
}

// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
//
// VPLThread_attr_t functions and helpers.
//
// ------------------------------------------------------------------------

#ifndef ANDROID
static void
VPLThread_AttrsGetPrio(VPLThread_attr_t* attrs)
{
    VPLThread__prio_mapping_t nativePrio;
    int vplPrio;
    int status;
    memset(&nativePrio, 0, sizeof(nativePrio));

    /// Get the VPL priority, normalizing a possible non-VPL priority.
    vplPrio = VPLThread_GetSchedPriority();
    if (vplPrio < VPL_PRIO_MIN || vplPrio > VPL_PRIO_MAX) {
        VPL_REPORT_WARN("VPLThread internal error: impossible priority %d", vplPrio);
    }

    // Map that VPL priority back to a native (scheduler policy, params) pair.
    status = VPLThread_MapPrioToNativePrio(&nativePrio, vplPrio);
    UNUSED(status);

    // Now save all the translated priorities as our output...
    attrs->vplPrio = vplPrio;
    attrs->nativeSchedPolicy = nativePrio.policy; // native pthread sched-policy
    // TODO FIXME:
    // VPL cannot expose <pthread.h>!
    // But maybe just exposing nativeSchedPrio is okay?
#ifdef IOS
    attrs->nativeSchedPrio = nativePrio.params.sched_priority; // native pthread prio
#else
    attrs->nativeSchedPrio = nativePrio.params.__sched_priority; // native pthread prio
#endif
}

// Set native (pthreads) thread-attributes for priority to be "inherited"
// from the calling thread's priority.
static void
VPLThread_AttrsSetNativePrio(pthread_attr_t* nativeAttrs)
{
    VPLThread_attr_t vplAttrs;
    struct sched_param nativeSchedParams;

    VPLThread_AttrsGetPrio(&vplAttrs);

    memset(&nativeSchedParams, 0, sizeof(nativeSchedParams));

#ifdef IOS
    nativeSchedParams.sched_priority = vplAttrs.nativeSchedPrio;
#else
    nativeSchedParams.__sched_priority = vplAttrs.nativeSchedPrio;
#endif

    pthread_attr_setschedpolicy(nativeAttrs, vplAttrs.nativeSchedPolicy);
    pthread_attr_setschedparam(nativeAttrs, &nativeSchedParams);
}
#endif

static void
VPLThread_MapAttrsToNativeAttrs(pthread_attr_t* nativeAttrs, const VPLThread_attr_t* vplAttrs)
{
    // 1. Map stack-size argument.

    // Suggest stack-size for new thread, via pthread attr.
    if (vplAttrs->stackSize != 0) {
        pthread_attr_setstacksize(nativeAttrs, vplAttrs->stackSize);
    }
    else {
        // Use the default stack size defined by VPL.
        pthread_attr_setstacksize(nativeAttrs, VPLTHREAD_STACKSIZE_DEFAULT);
    }

    // 2. Map detached state
    if (vplAttrs->createDetached) {
        pthread_attr_setdetachstate(nativeAttrs, PTHREAD_CREATE_DETACHED);
    }

#ifndef ANDROID
    // 3. Map VPL priority onto native priority, which is a
    // (schedule policy, params) pair.

    if (vplAttrs->vplPrio == VPLTHREAD__PRIO_INHERIT) {
        // Just get the "native" pthread scheduler state, then
        // set that directly into the native attrs.
        VPLThread_AttrsSetNativePrio(nativeAttrs);
    }
    else {
        // We have an explicit VPl priority. Map that to native prio.
        VPLThread__prio_mapping_t nativePrio;
        memset(&nativePrio, 0, sizeof(nativePrio));
        // First, map the VPL priority to a (policy, params) pair.
        (void)VPLThread_MapPrioToNativePrio(&nativePrio, vplAttrs->vplPrio);
        // Then apply that (policy, params) pair to the native attrs.
        pthread_attr_setschedpolicy(nativeAttrs, nativePrio.policy);
        pthread_attr_setschedparam(nativeAttrs, &(nativePrio.params));
    }

    // Make sure that pthread_create() actually obeys the values in nativeAttrs.
    pthread_attr_setinheritsched(nativeAttrs, PTHREAD_EXPLICIT_SCHED);
#endif
    // All done.
}

// ------------------------------------------------------------------------
// End of VPLThread_Create() and helpers.
// ------------------------------------------------------------------------

int
VPLThread_AttrInit(VPLThread_attr_t* attrs)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }

    // Documented VPL default stack-size.
    attrs->stackSize = VPLTHREAD_STACKSIZE_DEFAULT;

    attrs->createDetached = VPL_FALSE;

    // Force us to get and use the calling thread's priority.
    attrs->vplPrio = VPLTHREAD__PRIO_INHERIT;

    // The other fields are currently not used, but we will set them anyway.
    attrs->nativeSchedPolicy = SCHED_OTHER;
    attrs->nativeSchedPrio = 0;
    attrs->stackAddr = NULL;

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
VPLThread_AttrGetStackSize(const VPLThread_attr_t* attrs, size_t* stackSize)
{
    if (attrs == NULL || stackSize == NULL) {
        return VPL_ERR_INVALID;
    }
    *stackSize = attrs->stackSize;
    return VPL_OK;
}

int
VPLThread_AttrSetStackSize(VPLThread_attr_t* attrs, size_t stackSize)
{
    if (attrs == NULL) {
        return VPL_ERR_INVALID;
    }

    attrs->stackSize = stackSize;
    return VPL_OK;
}

int
VPLThread_AttrGetPriority(const VPLThread_attr_t* attrs, int* vplPrio)
{
    if (attrs == NULL || vplPrio == NULL) {
        return VPL_ERR_INVALID;
    }
    *vplPrio = attrs->vplPrio;
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

//-------------------------

int VPLDetachableThread_Create(VPLDetachableThreadHandle_t* threadHandle_out,
        VPLDetachableThread_fn_t startRoutine,
        void* startArg,
        const VPLThread_attr_t* attrs,
        const char* threadName)
{
    int rc;
    int rv = VPL_OK;
    pthread_attr_t pthreadAttrs;
    pthread_t pthread;
    // Make a copy so we can modify it.
    VPLThread_attr_t myAttrs = vplThread__default_attrs;

    // unused parameters
    (void)threadName;

    // sanity checks
    if (startRoutine == NULL) {
        return VPL_ERR_INVALID;
    }
    // startArg may be NULL, so don't check it.
    // threadId_out may be NULL, so don't check it.

    // Map the VPLThread_attrs attributes onto pthread_attr_t attributes.
    if (attrs != NULL) {
        myAttrs = *attrs;
    }

    if (threadHandle_out == NULL) {
        if (!myAttrs.createDetached) {
            return VPL_ERR_INVALID;
        }
    } else {
        threadHandle_out->handle = 0;
    }

    // Late-bind any inheritance of the creating thread's priority.
    if (myAttrs.vplPrio == VPLTHREAD__PRIO_INHERIT) {
        myAttrs.vplPrio = VPLThread_GetSchedPriority();
    }

    pthread_attr_init(&pthreadAttrs);
    VPLThread_MapAttrsToNativeAttrs(&pthreadAttrs, &myAttrs);
    // TODO: do we expose {,non}inheritance of priority?

    {
        size_t tempStackSize;
        pthread_attr_getstacksize(&pthreadAttrs, &tempStackSize);
        VPL_REPORT_INFO("Creating thread with stack size = "FMTu_size_t, tempStackSize);
    }
    rc = pthread_create(&pthread, &pthreadAttrs, startRoutine, startArg);
    if (rc != 0) {
        rv = VPLError_XlatErrno(rc);
        goto attr_destroy;
    }

    rv = VPL_OK;
    if (myAttrs.createDetached) {
        // Nothing else to do.
    } else {
        threadHandle_out->handle = pthread;
    }
    //if (threadId_out != NULL) {
    //    *threadId_out = pthread;
    //}

attr_destroy:
    pthread_attr_destroy(&pthreadAttrs);
    return rv;
}

int VPLDetachableThread_Detach(VPLDetachableThreadHandle_t* handle)
{
    if (handle == NULL) {
        return VPL_ERR_INVALID;
    }
    if (handle->handle == 0) {
        return VPL_ERR_ALREADY;
    }
    int rv = pthread_detach(handle->handle);
    if (rv != 0) {
        rv = VPLError_XlatErrno(rv);
        // TODO: unexpected: log error
    } else {
        handle->handle = 0;
    }
    return rv;
}

int VPLDetachableThread_Join(VPLDetachableThreadHandle_t* handle)
{
    int rc;
    int rv = VPL_OK;

    if (handle == NULL) {
        return VPL_ERR_INVALID;
    }
    if (handle->handle == 0) {
        return VPL_ERR_ALREADY;
    }
    if (handle->handle == pthread_self()) {
        return VPL_ERR_DEADLK;
    }

    rc = pthread_join(handle->handle, NULL);
    if (rc != 0) {
        rv = VPLError_XlatErrno(rc);
    } else {
        handle->handle = 0;
    }
    return rv;
}

VPLThreadId_t VPLDetachableThread_Self(void)
{
#ifdef ANDROID
    return pthread_self();
#elif defined(IOS)
    return (VPLThreadId_t)pthread_mach_thread_np(pthread_self());
#else
    return syscall(SYS_gettid);
#endif
}
