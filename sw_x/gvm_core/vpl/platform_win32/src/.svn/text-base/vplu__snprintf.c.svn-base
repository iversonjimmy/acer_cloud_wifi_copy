//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vplu_missing.h"
#include "vpl_string.h"

#ifdef _MSC_VER

int VPL_snprintf(char *str, size_t size, const char *format, ...)
{
    int rv;
    va_list ap;
    va_start(ap, format);
    rv = VPL_vsnprintf(str, size, format, ap);
    va_end(ap);
    return rv;
}

#endif
