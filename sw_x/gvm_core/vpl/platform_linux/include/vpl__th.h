//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL__TH_H__
#define __VPL__TH_H__

#include "vpl_limits.h"
#include <inttypes.h>

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
// Threads
//============================================================================

#define VPLTHREAD__MAX_THREADS    VPLLIMIT_NPROC
#define VPLTHREAD__MAX_SEMAPHORES (64)
#define VPLTHREAD__MAX_MUTEXES    (64)
#define VPLTHREAD__MAX_CONDVARS   (64)

/* Refer to this platform's definition of pthread_t */
typedef struct {
    pthread_t thread;
} VPLThread__t;
#ifdef IOS
#define PRI_VPLThread__t  "p"
#else
#define PRI_VPLThread__t  "lu"
#endif
#define VAL_VPLThread__t(thrd) ((thrd).thread)

//============================================================================

typedef struct {
    pthread_t handle;
} VPLDetachableThreadHandle__t;

#define VPLTHREAD__FN_ATTRS
typedef void* VPLDetachableThread__returnType_t;
#define VPLTHREAD__RETURN_VALUE  NULL

//============================================================================
// Thread-Creation Attributes
//============================================================================

// Stack limits...

// platform-specific stack-size limits.
#define VPLTHREAD__STACKSIZE_MAX      VPLLIMIT_STACK
#define VPLTHREAD__STACKSIZE_MIN      ((VPLLIMIT_STACK > (32*1024))? (32*1024) : VPLLIMIT_STACK)
#define VPLTHREAD__STACKSIZE_DEFAULT  ((VPLLIMIT_STACK > (2*1024*1024))? (2*1024*1024) : VPLLIMIT_STACK)

// platform-specific thread priority limits.
#define VPLTHREAD__PTHREAD_PRIO_MAX   VPLLIMIT_RTPRIO

typedef struct  {
    size_t stackSize;
    VPL_BOOL createDetached;
    int vplPrio;
    int nativeSchedPolicy;
    int nativeSchedPrio;
    uint32_t* stackAddr;  //% reserved for future use
} VPLThread__attr_t;

//============================================================================
// Dynamic initialization
//============================================================================
#define VPLTHREAD__ONCE_INIT { PTHREAD_ONCE_INIT }
//% val must match the size of a pthread_once_t, which happens to be an int.
typedef struct {
    pthread_once_t val;
} VPLThread__once_t;

//============================================================================
// Thread synchronization
//============================================================================

// Magic numbers to reveal the state of a thread synchronization object.
#define VPL_TH_UNDEF   0xdeadc0de
#define VPL_TH_INIT    0x476f6f64 // Good
#define VPL_TH_UNINIT  0xdeadbeef

// Tests to determine thread synchronization object state.
// Note: VPL_TH_IS UNDEF works best if the corresponding SET_UNDEF macro was
//       used at object declaration (see platform specific headers),
//       but should work most of the time anyway.
#define VPL__IS_INITIALIZED(obj) ((obj)->magic == VPL_TH_INIT)
#define VPL__IS_UNDEF(obj) ((obj)->magic != VPL_TH_INIT && \
                               (obj)->magic != VPL_TH_UNINIT)
#define VPL__IS_UNINITIALIZED(obj) ((obj)->magic == VPL_TH_UNINIT)

// Macros to set the initialization state of a thread synchronization object.
#define VPL__SET_INITIALIZED(obj) (obj)->magic = VPL_TH_INIT
#define VPL__SET_UNINITIALIZED(obj) (obj)->magic = VPL_TH_UNINIT

#define __VPL_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define __VPL_COND_INITIALIZER PTHREAD_COND_INITIALIZER

//============================================================================
// Semaphore functions
//============================================================================

typedef struct {
    unsigned int count;
    unsigned int maxCount;
    unsigned int nWaiters;
    pthread_mutex_t mutex[1];
    pthread_cond_t cond[1];
    unsigned int magic;
} VPLSem__t;
#ifdef ANDROID
#define VPLSEM__SET_UNDEF {0,0,0,{},{},VPL_TH_UNDEF}
#else
#define VPLSEM__SET_UNDEF {.magic = VPL_TH_UNDEF}
#endif

//============================================================================
// Mutex functions
//============================================================================

typedef struct {
    pthread_mutex_t  pthMutex[1];
    pthread_t        lockTid;
    int              lockCount;
    unsigned int     magic;
} VPLMutex__t;
#define VPLMUTEX__SET_UNDEF {.magic = VPL_TH_UNDEF}

#define _VPLMUTEX__INIT { {__VPL_MUTEX_INITIALIZER}, 0, 0, VPL_TH_INIT }

//============================================================================
// Condition Variables
//============================================================================

typedef struct {
    pthread_cond_t cond[1];
    unsigned int magic;
} VPLCond__t;
#define VPLCOND__SET_UNDEF {{}, VPL_TH_UNDEF}

#define _VPLCOND__INIT { {__VPL_COND_INITIALIZER}, VPL_TH_INIT }

//============================================================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
