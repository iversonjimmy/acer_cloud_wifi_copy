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

const char* VPL_strerror_errno(int errnum, char* buf, size_t buflen)
{
#if defined(ANDROID) || defined(IOS) || !defined(_GNU_SOURCE) || !_GNU_SOURCE
    // We have the XSI-compliant version of strerror_r; wrap it to imitate
    // the GNU-specific implementation.
    if (strerror_r(errnum, buf, buflen) != 0) {
        VPL_snprintf(buf, buflen, "Unknown error %d", errnum);
    }
    return buf;
#else
    // Use the GNU-specific implementation.
    return strerror_r(errnum, buf, buflen);
#endif
}
