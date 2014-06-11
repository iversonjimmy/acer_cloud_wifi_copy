//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL__TH_H__
#define __VPL__TH_H__

#include "vpl_plat.h"
#include "vpl_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
// Threads
//============================================================================

#define VPLTHREAD__MAX_THREADS      VPLLIMIT_NPROC
#define VPLTHREAD__MAX_SEMAPHORES   (64)
#define VPLTHREAD__MAX_MUTEXES      (64)
#define VPLTHREAD__MAX_CONDVARS     (64)

typedef struct {
#   ifndef VPL_PLAT_IS_WINRT
    void* handle;  // == HANDLE
    // GetThreadId is undefined on XP, so we need to also store the ID.
    unsigned long id;  // == DWORD
#   else
    int handle;
    unsigned int id;  // == DWORD
#   endif
    // Note: The following are here as safe temporary storage for
    // VPLThread_Create to pass data to VPLThread_StartWrapper, to avoid heap
    // allocation.
    VPLThread_fn_t fn;
    VPLThread_arg_t arg;
} VPLThread__t;
#define PRI_VPLThread__t  "lu"
#define VAL_VPLThread__t(thread) ((thread).id)

//============================================================================

typedef struct {
#   ifndef VPL_PLAT_IS_WINRT
    //% Type is actually "HANDLE".
    void* handle;
#   else
    int handle;
    unsigned int id;  // == DWORD
#   endif
} VPLDetachableThreadHandle__t;

#define VPLTHREAD__FN_ATTRS __stdcall
typedef unsigned int VPLDetachableThread__returnType_t;
#define VPLTHREAD__RETURN_VALUE  0

//============================================================================
// Thread-Creation Attributes
//============================================================================

//% TODO: these are just copied verbatim from GVM/Linux.
//% are the values appropriate for Win32?

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
} VPLThread__attr_t;

//============================================================================
// Dynamic initialization
//============================================================================
#ifdef _MSC_VER
#define VPLTHREAD__ONCE_INIT {INIT_ONCE_STATIC_INIT}
typedef struct {
    INIT_ONCE o;
} VPLThread__once_t;
#else
#define VPLTHREAD__ONCE_INIT {0}
typedef struct {
    //% val must match the size of a pthread_once_t, which happens to be an int.
    int val;
} VPLThread__once_t;
#endif

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

//============================================================================
// Semaphore functions
//============================================================================
typedef struct {
    void* handle;
    unsigned int magic;
} VPLSem__t;
#define VPLSEM__SET_UNDEF {0, VPL_TH_UNDEF}

//============================================================================
// Mutex functions
//============================================================================

// We implement mutexes using critical sections because they're more
// efficient and work with condition variables.
typedef struct {
    unsigned int     magic;
    int              lockCount;
    //% NOTE: The code assumes that lockTid can be read atomically and written atomically.
    //%   Undefined behavior can result if a read and write are interleaved.
    unsigned long    lockTid;
    CRITICAL_SECTION cs;
} VPLMutex__t;
#ifdef _MSC_VER
#define VPLMUTEX__SET_UNDEF {VPL_TH_UNDEF}
#else
#define VPLMUTEX__SET_UNDEF {VPL_TH_UNDEF,0,0,{0,0,0,0,0,0}}
#endif

//============================================================================
// Condition Variables
//============================================================================

#ifndef _MSC_VER
// The Windows definition, not present in the mingw headers,
// boils down to a void* pointer.
typedef void* CONDITION_VARIABLE;
typedef void** PCONDITION_VARIABLE;

// These aren't in the w32api headers, even though the functions are in recent
// kernel32.a versions.
WINBASEAPI void WINAPI InitializeConditionVariable(PCONDITION_VARIABLE);
WINBASEAPI BOOL WINAPI SleepConditionVariableCS(PCONDITION_VARIABLE,
                                                PCRITICAL_SECTION,
                                                DWORD);
WINBASEAPI VOID WINAPI WakeConditionVariable(PCONDITION_VARIABLE);
WINBASEAPI VOID WINAPI WakeAllConditionVariable(PCONDITION_VARIABLE);
#endif

typedef struct {
    CONDITION_VARIABLE cond;
    unsigned int magic;
} VPLCond__t;

#define VPLCOND__SET_UNDEF {0, VPL_TH_UNDEF}

//============================================================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
