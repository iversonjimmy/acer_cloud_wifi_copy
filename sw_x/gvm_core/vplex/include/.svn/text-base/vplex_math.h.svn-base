//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

//============================================================================
/// @file
/// APIs for math functions, including random number generation resources.
//============================================================================


#ifndef __VPLEX_MATH_H__
#define __VPLEX_MATH_H__

#include "vpl_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize the standard random number generator, seeding it with a value
/// based on the current time.
/// Returns VPL_OK on success, failure reason otherwise.
int VPLMath_InitRand(void);

/// Get a standard random number.
/// Returns a random number, but not necessarily one good enough for security.
uint32_t VPLMath_Rand(void);

/// Seed the random function with a specific value.
/// #VPLMath_InitRand() should usually provide a sufficiently random seed, but
/// this can be helpful during testing if you want to ensure that the same 
/// sequence of random numbers is produced each time (although multi-threading
/// can easily lead to different results).
void VPLMath_SRand(uint32_t seed);

#ifdef __cplusplus
}
#endif

#endif  // include guard
