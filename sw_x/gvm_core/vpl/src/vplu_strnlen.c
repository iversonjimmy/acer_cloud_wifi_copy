//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vplu_missing.h"

#ifdef VPL_PLAT_NEEDS_STRNLEN
size_t strnlen(const char *s, size_t maxlen)
{
    const char *p = s;
    const char *e = s + maxlen;
    while (p != e && *p)
        ++p;
    
    return p - s;
}
#endif
