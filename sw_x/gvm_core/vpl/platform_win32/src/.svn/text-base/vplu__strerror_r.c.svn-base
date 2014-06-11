//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_string.h"

#ifndef _MSC_VER
// Not provided with mingw, so you get this lame implementation instead:
static inline int strerror_s(
        char* buffer,
        size_t numberOfElements,
        int errnum)
{
    VPL_snprintf(buffer, numberOfElements, "(error %d)", errnum);
    return 0;
}
#endif

const char* VPL_strerror_errno(int errnum, char* buf, size_t buflen)
{
    // Wrap strerror_s to imitate the GNU-specific implementation of strerror_r.
    if (strerror_s(buf, buflen, errnum) != 0) {
        VPL_snprintf(buf, buflen, "Unknown error %d", errnum);
    }
    return buf;
}
