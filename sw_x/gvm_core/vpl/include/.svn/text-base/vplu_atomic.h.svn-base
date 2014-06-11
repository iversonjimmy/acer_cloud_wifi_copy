//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLU_ATOMIC_H__
#define __VPLU_ATOMIC_H__

//============================================================================
/// @file
/// VPL utility (VPLU) operations for atomic addition/subtraction.
//============================================================================

#include "vpl_plat.h"
#include "vpl__atomic.h"

#define VPL_ATOMIC_INC32(operand) VPLAtomic_add(&(operand), 1)
#define VPL_ATOMIC_DEC32(operand) VPLAtomic_add(&(operand), -1)

#define VPL_ATOMIC_INC_UNSIGNED32(operand) VPLAtomic_add((volatile int32_t*)&(operand), 1)
#define VPL_ATOMIC_DEC_UNSIGNED32(operand) VPLAtomic_add((volatile int32_t*)&(operand), -1)

#endif // include guard
