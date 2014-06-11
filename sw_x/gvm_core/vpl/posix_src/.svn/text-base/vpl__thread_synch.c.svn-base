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
#ifdef IOS
#include "vpl__mac_time.h"
#else
#include <time.h>
#endif
#include <sched.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "vpl_plat.h"
#include "vpl_error.h"

int
VPLSem_Init(VPLSem_t* sem, unsigned int maxCount, unsigned int value)
{
    int rv = VPL_OK;
    int rc;
    
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

    //base init
    sem->maxCount = maxCount;
    sem->count = value;
    sem->nWaiters = 0;

    rc = pthread_mutex_init((pthread_mutex_t *)(sem->mutex), (pthread_mutexattr_t *)NULL);
    if (rc != 0) {
        rv = VPL_ERR_NOBUFS;
        goto out;
    }

    //init cond var
    rc = pthread_cond_init((pthread_cond_t *)(sem->cond), NULL);
    if (rc != 0) {
        pthread_mutex_destroy((pthread_mutex_t *)(sem->mutex));
        rv = VPL_ERR_NOBUFS;
        goto out;
    }

out:
    if( rv == VPL_OK ) {
        VPL__SET_INITIALIZED(sem);
    }
    return rv;
}

int
VPLSem_Wait(VPLSem_t* sem)
{
    int rc;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    rc = pthread_mutex_lock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        return VPL_ERR_FAIL;
    }

    while (sem->count == 0) {
        sem->nWaiters++;
        rc = pthread_cond_wait((pthread_cond_t *)(sem->cond), (pthread_mutex_t *)(sem->mutex));
        sem->nWaiters--;
        if (rc != 0) {
            pthread_mutex_unlock((pthread_mutex_t *)(sem->mutex));
            return VPL_ERR_FAIL;
        }
    }
    sem->count--;

    rc = pthread_mutex_unlock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        return VPL_ERR_FAIL;
    }

    return VPL_OK;
}

static void
privTimeoutToTimespec(const struct timespec* in, VPLTime_t timeout, struct timespec* out)
{
    out->tv_sec = in->tv_sec + VPLTime_ToSec(timeout);
    out->tv_nsec = in->tv_nsec + VPLTime_ToNanosec(timeout%VPLTIME_MICROSEC_PER_SEC);
    out->tv_sec += out->tv_nsec/VPLTIME_NANOSEC_PER_SEC;
    out->tv_nsec %= VPLTIME_NANOSEC_PER_SEC;

    // If desired wait time is past the end of the epoch, wait until
    // the end of the epoch. This should only happen if the wait was
    // arbitrarily large, not because this code is in use near the end
    // of the epoch.
    if(out->tv_sec < in->tv_sec ||
       (out->tv_sec == in->tv_sec &&
        out->tv_nsec < in->tv_nsec)) {
        out->tv_sec = INT32_MAX;
        out->tv_nsec = 999999999;
    }
}

int
VPLSem_TimedWait(VPLSem_t* sem, VPLTime_t timeout)
{
    int rc;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    struct timespec now;
    // TODO: bug 521: use CLOCK_MONOTONIC
    if(clock_gettime(CLOCK_REALTIME, &now) != 0) {
        return VPL_ERR_FAIL;
    }
    struct timespec ts;
    privTimeoutToTimespec(&now, timeout, &ts);

    rc = pthread_mutex_lock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        return VPL_ERR_FAIL;
    }

    while (sem->count == 0) {
        sem->nWaiters++;
        rc = pthread_cond_timedwait((pthread_cond_t *)(sem->cond), (pthread_mutex_t *)(sem->mutex), &ts);
        sem->nWaiters--;
        // Upon return, the mutex should be locked again (even in failure cases).
        if (rc != 0) {
            pthread_mutex_unlock((pthread_mutex_t *)(sem->mutex));
            if (rc == ETIMEDOUT) {
                return VPL_ERR_TIMEOUT;
            }
            return VPL_ERR_FAIL;
        }
    }
    sem->count--;

    rc = pthread_mutex_unlock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        return VPL_ERR_FAIL;
    }

    return VPL_OK;
}

int
VPLSem_TryWait(VPLSem_t* sem)
{
    int rv = VPL_OK;
    int rc;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    rc = pthread_mutex_lock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        return VPL_ERR_FAIL;
    }

    if (sem->count == 0) {
        rv = VPL_ERR_AGAIN;
    } else {
        sem->count--;
    }

    rc = pthread_mutex_unlock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        rv = VPL_ERR_FAIL;
    }

    return rv;
}

int
VPLSem_Post(VPLSem_t* sem)
{
    int rv = VPL_OK;
    int rc;

    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    rc = pthread_mutex_lock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        return VPL_ERR_FAIL;
    }

    if (sem->count == sem->maxCount) {
        rv = VPL_ERR_MAX;
        goto out;
    }

    if (sem->count++ == 0 && sem->nWaiters) {
        rv = pthread_cond_signal((pthread_cond_t *)(sem->cond));
        if (rv != 0) {
            rv = VPL_ERR_FAIL;
            goto out;
        }
    }

out:
    rc = pthread_mutex_unlock((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        rv = VPL_ERR_FAIL;
    }

    return rv;
}

int
VPLSem_Destroy(VPLSem_t* sem)
{
    int rv = VPL_OK;
    int rc;
    
    if (sem == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(sem)) {
        return VPL_ERR_INVALID;
    }

    //destroy our mutex
    rc = pthread_mutex_destroy((pthread_mutex_t *)(sem->mutex));
    if (rc != 0) {
        rv = VPL_ERR_FAIL;
    }

    //destroy our cond var 
    rc = pthread_cond_destroy((pthread_cond_t *)(sem->cond));
    if (rc != 0) {
        rv = VPL_ERR_FAIL;
    }

    VPL_SET_UNINITIALIZED(sem);
    return rv;
}

int
VPLMutex_Init(VPLMutex_t* mutex)
{
    int rv = VPL_OK;
    int rc;

    //    static const int mutex_type = PTHREAD_MUTEX_TIMED_NP;

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

    rc = pthread_mutex_init((pthread_mutex_t *)(mutex->pthMutex), (pthread_mutexattr_t *)NULL);

    if (rc != 0) {
        rv = VPLError_XlatErrno(rc);
    }
    else {
        VPL__SET_INITIALIZED(mutex);
    }

    return rv;
}

int
VPLMutex_Locked(const VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    // Note: this read of lockTid must be atomic.
    return ((mutex)->lockTid != 0)? 1 : 0;
}

int
VPLMutex_LockedSelf(const VPLMutex_t* mutex)
{
    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    // Note: this read of lockTid must be atomic.
    return ((pthread_t)((mutex)->lockTid) == pthread_self())? 1 : 0;
}

int
VPLMutex_Lock(VPLMutex_t* mutex)
{
    int rc;
    int rv = VPL_OK;
    pthread_t self = pthread_self();

    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    if(VPLMutex_LockedSelf(mutex)) {
        mutex->lockCount++;
    }
    else {
        rc = pthread_mutex_lock((pthread_mutex_t *)(mutex->pthMutex));
        
        if(rc == 0) {
            rv = VPL_OK;
            
            // Note: this assignment of lockTid must be atomic.
            mutex->lockTid = self;
            mutex->lockCount = 1;
        }
        else {
            rv = VPLError_XlatErrno(rc);
        }
    }

    return rv;
}

int
VPLMutex_TryLock(VPLMutex_t* mutex)
{
    int rv = VPL_OK;
    pthread_t self = pthread_self();
    int rc;

    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    if(VPLMutex_LockedSelf(mutex)) {
        mutex->lockCount++;
    }
    else {
        rc = pthread_mutex_trylock((pthread_mutex_t *)(mutex->pthMutex));
        
        if (rc == 0) {
            // Note: this assignment of lockTid must be atomic.
            mutex->lockTid = self;
            mutex->lockCount = 1;
            rv = VPL_OK;
        }
        else {
            rv = VPLError_XlatErrno(rc);
        }
    }

    return rv;
}

int
VPLMutex_Unlock(VPLMutex_t* mutex)
{
    int rv = VPL_OK;
    int rc;

    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    if(!VPLMutex_LockedSelf(mutex)) {
        rv = VPL_ERR_PERM;
    }
    else if(--mutex->lockCount == 0) {
        // Note: this assignment of lockTid must be atomic.
        mutex->lockTid = 0;
        rc = pthread_mutex_unlock((pthread_mutex_t *)(mutex->pthMutex));
        
        if(rc == 0) {
            rv = VPL_OK;
        }
        else {
            rv = VPLError_XlatErrno(rc);
        }
    }

    return rv;
}

int
VPLMutex_Destroy(VPLMutex_t* mutex)
{
    int rv;
    int rc;

    if (mutex == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(mutex)) {
        return VPL_ERR_INVALID;
    }

    rc = pthread_mutex_destroy((pthread_mutex_t *)(mutex->pthMutex));
    if (rc == 0) {
        VPL_SET_UNINITIALIZED(mutex);
        rv = VPL_OK;
    }
    else {
        rv = VPLError_XlatErrno(rc);
    }

    return rv;
}

/* Condition variable functions */

int
VPLCond_Init(VPLCond_t* cond)
{
    int rv;

    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
#ifdef VPL_THREAD_SYNC_INIT_CHECK
    if (VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_IS_INIT;
    }
#endif

    // TODO: bug 521: init with pthread_condattr_setclock as CLOCK_MONOTONIC.
    rv = pthread_cond_init((pthread_cond_t *)(cond->cond), NULL);
    if (rv == 0) {
        VPL__SET_INITIALIZED(cond);
        return VPL_OK;
    }
    else {
        return VPLError_XlatErrno(rv);
    }
}

int
VPLCond_Signal(VPLCond_t* cond)
{
    int rv;

    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_INVALID;
    }

    rv = pthread_cond_signal((pthread_cond_t *)(cond->cond));
    if (rv == 0) {
        return VPL_OK;
    } else {
        return VPLError_XlatErrno(rv);
    }
}

int
VPLCond_Broadcast(VPLCond_t* cond)
{
    int rv;

    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_INVALID;
    }

    rv = pthread_cond_broadcast((pthread_cond_t *)(cond->cond));
    if (rv == 0) {
        return VPL_OK;
    } else {
        return VPLError_XlatErrno(rv);
    }
}

int
VPLCond_TimedWait(VPLCond_t* cond, VPLMutex_t* mutex, VPLTime_t time)
{
    int count;
    int rv = VPL_OK;

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
    // Note: this assignment of lockTid must be atomic.
    mutex->lockTid = 0;

    if (time == VPL_TIMEOUT_NONE) {
        int rc = pthread_cond_wait((pthread_cond_t *)(cond->cond),
                (pthread_mutex_t *)(mutex->pthMutex));
        if (rc == 0) {
            rv = VPL_OK;
        }
        else {
            rv = VPLError_XlatErrno(rc);
        }
    }
    else {
        struct timespec now;
        // TODO: bug 521: use CLOCK_MONOTONIC
        if(clock_gettime(CLOCK_REALTIME, &now) == 0) {
            int rc;
            struct timespec ts;
            privTimeoutToTimespec(&now, time, &ts);
            rc = pthread_cond_timedwait((pthread_cond_t *)(cond->cond),
                    (pthread_mutex_t *)(mutex->pthMutex), &ts);
            // Upon return, the mutex should be locked again (even in failure cases).
            if (rc == 0) {
                rv = VPL_OK;
            }
            else {
                rv = VPLError_XlatErrno(rc);
            }
        }
        else {
            rv = VPL_ERR_FAIL;
        }
    }

    mutex->lockCount = count;
    // Note: this assignment of lockTid must be atomic.
    mutex->lockTid = pthread_self();
    return rv;
}

int
VPLCond_Destroy(VPLCond_t* cond)
{
    int rv = VPL_OK;
    int rc;

    if (cond == NULL) {
        return VPL_ERR_INVALID;
    }
    if (!VPL_IS_INITIALIZED(cond)) {
        return VPL_ERR_INVALID;
    }

    rc = pthread_cond_destroy((pthread_cond_t *)(cond->cond));
    if (rc != 0) {
        rv = VPLError_XlatErrno(rc);
    }

    VPL_SET_UNINITIALIZED(cond);
    return rv;
}

