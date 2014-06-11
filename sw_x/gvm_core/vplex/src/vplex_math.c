//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_math.h"
#include "vplex_plat.h"
#include "vplu.h"
#include "vpl_time.h"

#include <stdio.h>
#include <stdlib.h>

// TODO:
// Use random(), instead of rand().
// While both have the same range, random() has a longer period (due to its larger state).

int VPLMath_InitRand(void) {
    /// Just use native random number generator.
    /// Seed with current time.
    int rv = VPL_OK;
    unsigned int seed = TRUNCATE_TO_U32(VPLTime_GetTime());
    srand(seed);
    return rv;
}

void VPLMath_SRand(uint32_t seed)
{
    srand(seed);
}

uint32_t VPLMath_Rand(void) {
    /// Just use native random number generator.
    // We will assume that it has at least 16 bits of entropy in the low-end.
    // Thus, call twice and stitch the results together.
    return (rand() << 16) | (rand() & 0xffff);
}
