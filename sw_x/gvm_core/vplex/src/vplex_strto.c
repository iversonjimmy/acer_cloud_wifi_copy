//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_plat.h"

#include <errno.h>

u32 VPL_strToU32(const char* str, char** endptr, int base)
{
    unsigned long long parsed;
    errno = 0;
    parsed = strtoull(str, endptr, base);
    if (errno == 0) {
        if (parsed > UINT32_MAX) {
            errno = ERANGE;
            parsed = UINT32_MAX;
        }
    }
    return (u32)parsed;
}

u64 VPL_strToU64(const char* str, char** endptr, int base)
{
    unsigned long long parsed;
    errno = 0;
    parsed = strtoull(str, endptr, base);
    if (errno == 0) {
        if (parsed > UINT64_MAX) {
            errno = ERANGE;
            parsed = UINT64_MAX;
        }
    }
    return (u64)parsed;
}
