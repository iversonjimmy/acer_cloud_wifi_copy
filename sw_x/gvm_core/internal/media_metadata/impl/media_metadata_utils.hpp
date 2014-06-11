//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//


#ifndef __MEDIA_METADATA_UTILS_HPP__
#define __MEDIA_METADATA_UTILS_HPP__

//============================================================================
/// @file
/// Media Metadata utilities.
//============================================================================

#include <vplu.h>
#include <vpl_user.h>
#include <string>

#include "media_metadata_errors.hpp"

namespace media_metadata {

/// Extract the device ID from the streaming URL.
MMError MMGetDeviceFromUrl(const char* url, u64* deviceId_out);

/// Parse the Media Streaming URL.
/// See http://www.ctbg.acer.com/wiki/index.php/Personal_Cloud:_Media_Streaming
MMError MMParseUrl(const char* urlIn,
                    u64& deviceId,
                    std::string& urlType,
                    std::string& objectId,
                    std::string& collectionId,
                    std::string& extension,
                    bool base64ObjectId);

MMError MMRemoveExtraInfoFromUrl(const char* urlIn, std::string& urlOut);

static inline
std::string MM_u64ToString(u64 value)
{
    char tempBuf[17];
    snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), "%016"PRIx64, value);
    return std::string(tempBuf);
}

static inline
std::string MMDeviceIdToString(u64 deviceId)
{
    return MM_u64ToString(deviceId);
}

static inline
std::string MMTimestampToString(u64 timestamp)
{
    return MM_u64ToString(timestamp);
}

} // namespace media_metadata

#endif // include guard
