/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */
#include "vpl_types.h"
#include "vss_util.hpp"

#include "vplex_serialization.h"

#include "vplex_trace.h"

void decode64(std::string& data)
{
    char* decoded_data = NULL;
    size_t decoded_len;

    if(decode64(data.c_str(), data.size(),
                &decoded_data, &decoded_len, true) < 0) {
        // Failed decoding. return empty string.
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to decode {%s}.",
                         data.c_str());

        data.erase();
    }
    else {
        data.assign(decoded_data, decoded_len);
    }
    
    if(decoded_data) {
        free(decoded_data);
    }
}

int decode64(const char* in_buf, size_t in_len, char** out_buf, size_t* out_len, bool oneline)
{
    int rv = 0;

    *out_len = VPL_BASE64_DECODED_MAX_BUF_LEN(in_len);
    *out_buf = (char*)malloc(*out_len);
    if(*out_buf != NULL) {
        VPL_DecodeBase64(in_buf, in_len, *out_buf, out_len);
    }    
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed memory allocation for base64 decode.");
        rv = -1;
        *out_buf = NULL;
        *out_len = 0;
    }

    return rv;
}
 
