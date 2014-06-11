//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL__LAZY_INIT_H__
#define __VPL__LAZY_INIT_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER

typedef struct {
    INIT_ONCE o;
    VPLMutex_t m;
} _VPLLazyInitMutex__t;
#define _VPLLAZYINITMUTEX__INIT  {INIT_ONCE_STATIC_INIT, VPLMUTEX__SET_UNDEF}
VPLMutex_t* _VPLLazyInitMutex__GetMutex(_VPLLazyInitMutex__t* lazyInitMutex);
#define _VPLLAZYINITMUTEX__GET_MUTEX_IMPL  return _VPLLazyInitMutex__GetMutex(lazyInitMutex)

typedef struct {
    INIT_ONCE o;
    VPLCond_t c;
} _VPLLazyInitCond__t;
#define _VPLLAZYINITCOND__INIT  {INIT_ONCE_STATIC_INIT, VPLCOND__SET_UNDEF}
VPLCond_t* _VPLLazyInitCond__GetCond(_VPLLazyInitCond__t* lazyInitCond);
#define _VPLLAZYINITCOND__GET_COND_IMPL  return _VPLLazyInitCond__GetCond(lazyInitCond)

#else

#include <pthread.h>

typedef struct {
    pthread_once_t o;
    VPLMutex_t m;
} _VPLLazyInitMutex__t;
#define _VPLLAZYINITMUTEX__INIT  {PTHREAD_ONCE_INIT, VPLMUTEX__SET_UNDEF}
VPLMutex_t* _VPLLazyInitMutex__GetMutex(_VPLLazyInitMutex__t* lazyInitMutex);
#define _VPLLAZYINITMUTEX__GET_MUTEX_IMPL  return _VPLLazyInitMutex__GetMutex(lazyInitMutex)

typedef struct {
    pthread_once_t o;
    VPLCond_t c;
} _VPLLazyInitCond__t;
#define _VPLLAZYINITCOND__INIT  {PTHREAD_ONCE_INIT, VPLCOND__SET_UNDEF}
VPLCond_t* _VPLLazyInitCond__GetCond(_VPLLazyInitCond__t* lazyInitCond);
#define _VPLLAZYINITCOND__GET_COND_IMPL  return _VPLLazyInitCond__GetCond(lazyInitCond)

#endif

#ifdef  __cplusplus
}
#endif

#endif // include guard
