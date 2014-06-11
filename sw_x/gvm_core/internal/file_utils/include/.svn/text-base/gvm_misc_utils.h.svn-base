//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef _GVM_MISC_UTILS_H_
#define _GVM_MISC_UTILS_H_

#include "vplu_types.h"
#include "vpl_user.h"

// To support the static inlines.
#include <errno.h>
#include <stdlib.h>
#include "log.h"

#ifdef  __cplusplus
extern "C" {
#endif

// TODO: This probably belongs in its own "ES Core Utils" library.
#define FMT_ESContentId  FMTu32

/// Just like #VPL_EncodeBase64(), except this version mallocs the buffer for you.
/// @note On success, <code>*dst_out</code> will be malloc'ed by this function; the caller must free it.
/// @param dst_out String will be null-terminated. Must be freed by caller.
/// @param dstLen_out Count does not include null-terminator.
int Util_EncodeBase64(const void* src, size_t srcLen, char** dst_out, size_t* dstLen_out,
        VPL_BOOL addNewlines, VPL_BOOL urlSafe);

static inline
s64 Util_ParseContentId(const char* str)
{
    s64 result;
    errno = 0;
    result = strtoul(str, NULL, 16);
    if (errno != 0) {
        LOG_WARN("Failed to parse content id \"%s\"", str);
        return -1;
    }
    return result;
}

static inline
VPL_BOOL Util_ParseTitleId(const char* str, u64* titleId_out)
{
    errno = 0;
    *titleId_out = strtoull(str, NULL, 16);
    if (errno != 0) {
        LOG_WARN("Failed to parse title id \"%s\"", str);
        return VPL_FALSE;
    }
    return VPL_TRUE;
}

/// If @a rawId contains exactly 16 characters and all of them are hexadecimal digits, returns
/// the parsed value.  Otherwise, returns 0.
u64 Util_ParseStrictHex64(const char* rawId);

/// See #Util_ParseStrictHex64();
static inline
VPLUser_Id_t Util_ParseStrictUserId(const char* rawUserId) {
    return Util_ParseStrictHex64(rawUserId);
}

/// See #Util_ParseStrictHex64();
static inline
u64 Util_ParseStrictDeviceId(const char* rawDeviceId) {
    return Util_ParseStrictHex64(rawDeviceId);
}

/// See #Util_ParseStrictHex64();
static inline
u64 Util_ParseStrictDatasetId(const char* rawDatasetId) {
    return Util_ParseStrictHex64(rawDatasetId);
}

#ifdef  __cplusplus
}
#endif

#endif // include guard
