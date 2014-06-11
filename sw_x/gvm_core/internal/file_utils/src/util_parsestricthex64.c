//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "gvm_misc_utils.h"
#include "vpl_plat.h"

u64 Util_ParseStrictHex64(const char* rawId)
{
    u64 result = 0;
    int charsRead = 0;
    char c;
    while ((c = rawId[charsRead]) != '\0') {
        result = result << 4;
        if (c >= '0' && c <= '9') {
            result += (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result += (10 + (c - 'a'));
        } else if (c >= 'A' && c <= 'F') {
            result += (10 + (c - 'A'));
        } else {
            // Invalid!
            return 0;
        }
        charsRead++;
    }
    // Expecting 64 bits / 4 bits per hexadecimal char = 16 chars
    if (charsRead != 16) {
        return 0;
    }
    return result;
}
