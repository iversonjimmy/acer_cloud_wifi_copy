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

// TODO: temp hack; pthread_rwlock_t not defined in NDK version "android-9" and we want to support
//     that, so fall back to a simple mutex for now.

int VPLSlimRWLock_Init(VPLSlimRWLock_t* rwlock)
{
    int rv = VPL_OK;

    if(rwlock == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        int rc = pthread_mutex_init(&(rwlock->rwlock), NULL);
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
        int rc = pthread_mutex_destroy(&(rwlock->rwlock));
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
        int rc = pthread_mutex_lock(&(rwlock->rwlock));
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
        int rc = pthread_mutex_lock(&(rwlock->rwlock));
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
        int rc = pthread_mutex_unlock(&(rwlock->rwlock));
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
