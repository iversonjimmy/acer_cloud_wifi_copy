/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#include "gvm_misc_utils.h"

#include "vpl_plat.h"
#include "log.h"
#include "gvm_errors.h"
#include "vplex_serialization.h"

int
Util_EncodeBase64(const void* src, size_t srcLen, char** dst_out, size_t* dstLen_out,
        VPL_BOOL addNewlines, VPL_BOOL urlSafe)
{
    size_t encodedBufLen;
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    
    if (dst_out == NULL) {
        return UTIL_ERR_INVALID;
    }
    if (addNewlines) {
        encodedBufLen = VPL_BASE64_ENCODED_BUF_LEN(srcLen);
    } else {
        encodedBufLen = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(srcLen);
    }
    *dst_out = malloc(encodedBufLen);
    if (*dst_out == NULL) {
        if (dstLen_out != NULL) {
            *dstLen_out = 0;
        }
        return UTIL_ERR_NO_MEM;
    }
    VPL_EncodeBase64(src, srcLen, *dst_out, &encodedBufLen, addNewlines, urlSafe);
    // Subtract one for the null-terminator.
    if (dstLen_out != NULL) {
        *dstLen_out = (encodedBufLen - 1);
    }
    return GVM_OK;
}
