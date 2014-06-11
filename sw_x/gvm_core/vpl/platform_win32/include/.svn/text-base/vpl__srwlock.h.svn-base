//
//  Copyright 2011 iGware Inc.
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

#include "vpl_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MSC_VER
   //% Size must be >= sizeof(pthread_rwlock_t)
#  define __VPL_SIZEOF_SYS_RWLOCK 72
#endif

typedef struct {
#ifdef _MSC_VER
    SRWLOCK l;
#else
    char rwlock[__VPL_SIZEOF_SYS_RWLOCK];
#endif
} VPLSlimRWLock__t;

#ifdef __cplusplus
}
#endif

#endif // include guard
