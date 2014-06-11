//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef VPL__SRWLOCK_H__
#define VPL__SRWLOCK_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
#ifndef ANDROID
    pthread_rwlock_t rwlock;
#else
    // See src_android/vpl__srwlock.c for explanation
    pthread_mutex_t rwlock;
#endif
} VPLSlimRWLock__t;

#ifdef __cplusplus
}
#endif

#endif // include guard
