//
//  Copyright 2010-2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_LAZY_INIT_H__
#define __VPL_LAZY_INIT_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for thread sync primitives that automatically
/// initialize upon first use.
//============================================================================

#include "vpl_plat.h"
#include "vpl_th.h"

#include "vpl__lazy_init.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------

/// You must initialize this with #VPLLAZYINITMUTEX_INIT.
/// You should only declare instances of #VPLLazyInitMutex_t as global (or static global) variables.
typedef _VPLLazyInitMutex__t  VPLLazyInitMutex_t;

/// You must assign this value to each #VPLLazyInitMutex_t.
/// @hideinitializer
#define VPLLAZYINITMUTEX_INIT  _VPLLAZYINITMUTEX__INIT

/// Get access to the underlying #VPLMutex_t.
static inline
VPLMutex_t* VPLLazyInitMutex_GetMutex(VPLLazyInitMutex_t* lazyInitMutex)
{ _VPLLAZYINITMUTEX__GET_MUTEX_IMPL; }

//-----------------------------------------------

/// You must initialize this with #VPLLAZYINITCOND_INIT.
/// You should only declare instances of #VPLLazyInitCond_t as global (or static global) variables.
typedef _VPLLazyInitCond__t  VPLLazyInitCond_t;

/// You must assign this value to each #VPLLazyInitCond_t.
/// @hideinitializer
#define VPLLAZYINITCOND_INIT  _VPLLAZYINITCOND__INIT

/// Get access to the underlying #VPLCond_t.
static inline
VPLCond_t* VPLLazyInitCond_GetCond(VPLLazyInitCond_t* lazyInitCond)
{ _VPLLAZYINITCOND__GET_COND_IMPL; }

//-----------------------------------------------

#ifdef  __cplusplus
}
#endif

#endif // include guard
