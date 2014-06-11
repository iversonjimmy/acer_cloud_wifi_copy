//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

// this #define is required, per http://msdn.microsoft.com/en-us/library/sxtz2fa8%28v=vs.90%29.aspx
#define _CRT_RAND_S

#include "vplex_math.h"
#include "vplex_plat.h"
#include "vplu.h"
#include "vpl_time.h"

#include <stdio.h>

// Our preference is to use rand_s().
// (cf. http://msdn.microsoft.com/en-us/library/sxtz2fa8%28v=vs.90%29.aspx)
// However, since it can fail, we will initialize rand(), to use as a fall-back.

int VPLMath_InitRand(void)
{
    int rv = VPL_OK;
    unsigned int seed = TRUNCATE_TO_U32(VPLTime_GetTime());
    srand(seed);
    return rv;
}

void VPLMath_SRand(uint32_t seed)
{
    srand(seed);
}

uint32_t VPLMath_Rand(void)
{
    unsigned int num;
    errno_t err = rand_s(&num);
    if (err) {
        // rand_s() failed; use rand() instead.
        // Since rand() only has 15 bits of entropy in the low-end,
        // (cf. http://msdn.microsoft.com/en-us/library/398ax69y%28v=vs.90%29.aspx)
        // call it multiple times and stitch the results together.
        num = (rand() << 30) | (rand() << 15) | rand();
    }
    return num;
}
