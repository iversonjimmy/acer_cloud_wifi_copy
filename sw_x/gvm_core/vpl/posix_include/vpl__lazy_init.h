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

// On this platform, a regular VPLMutex can be initialized statically.
typedef struct {
    VPLMutex_t m;
}  _VPLLazyInitMutex__t;
#define _VPLLAZYINITMUTEX__INIT  {_VPLMUTEX__INIT}
#define _VPLLAZYINITMUTEX__GET_MUTEX_IMPL  return &(lazyInitMutex->m)

// On this platform, a regular VPLCond can be initialized statically.
typedef struct {
    VPLCond_t c;
}  _VPLLazyInitCond__t;
#define _VPLLAZYINITCOND__INIT  {_VPLCOND__INIT}
#define _VPLLAZYINITCOND__GET_COND_IMPL  return &(lazyInitCond->c)

#ifdef  __cplusplus
}
#endif

#endif // include guard
