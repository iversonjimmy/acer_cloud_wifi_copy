//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_srwlock.h"

#include "vpl_assert.h"
#include "vpl_error.h"
#include "vplu_common.h"

#include <pthread.h>

int VPLSlimRWLock_Init(VPLSlimRWLock_t* rwlock)
{
    int rv = VPL_OK;

    if(rwlock == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {

        // TODO: wish we could do this at compile time
        VPLIMPL_ASSERT(ARRAY_SIZE_IN_BYTES(rwlock->rwlock) >= sizeof(pthread_rwlock_t));

        int rc = pthread_rwlock_init((pthread_rwlock_t*)&(rwlock->rwlock), NULL);
        if(rc != 0) {
            rv = VPLError_XlatErrno(rc);
        }
    }

    return rv;
}

int VPLSlimRWLock_Destroy(VPLSlimRWLock_t* rwlock)
{
    int rv = VPL_OK;

    if(rwlock == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        int rc = pthread_rwlock_destroy((pthread_rwlock_t*)&(rwlock->rwlock));
        if(rc != 0) {
            rv = VPLError_XlatErrno(rc);
        }
    }

    return rv;
}

int VPLSlimRWLock_LockRead(VPLSlimRWLock_t* rwlock)
{
    int rv = VPL_OK;

    if(rwlock == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        int rc = pthread_rwlock_rdlock((pthread_rwlock_t*)&(rwlock->rwlock));
        if(rc != 0) {
            rv = VPLError_XlatErrno(rc);
        }
    }

    return rv;
}

int VPLSlimRWLock_LockWrite(VPLSlimRWLock_t* rwlock)
{
    int rv = VPL_OK;

    if(rwlock == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        int rc = pthread_rwlock_wrlock((pthread_rwlock_t*)&(rwlock->rwlock));
        if(rc != 0) {
            rv = VPLError_XlatErrno(rc);
        }
    }
    return rv;
}

int VPLSlimRWLock_UnlockRead(VPLSlimRWLock_t* rwlock)
{
    int rv = VPL_OK;

    if(rwlock == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        int rc = pthread_rwlock_unlock((pthread_rwlock_t*)&(rwlock->rwlock));
        if(rc != 0) {
            rv = VPLError_XlatErrno(rc);
        }
    }
    return rv;
}

int VPLSlimRWLock_UnlockWrite(VPLSlimRWLock_t* rwlock)
{
    return VPLSlimRWLock_UnlockRead(rwlock);
}
