//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "media_metadata_utils.hpp"
#include "media_metadata_errors.hpp"

#include "gvm_utils.h"
#include "log.h"
#include "scopeguard.hpp"
#include "vplex_serialization.h"

#include <string>
#include <set>
#include <vector>

namespace media_metadata {

MMError MMGetDeviceFromUrl(const char* url, u64* deviceId_out)
{
    s32 rv;
    {
        char tempBuf[17];
        // NOTE: These all need to stay in sync:
        // - #MMGetDeviceFromUrl()
        // - #MMParseUrl()
        // - #MSAGetObjectMetadata()
        // .
        if (1 != sscanf(url, "/mm/%16s", tempBuf)) {
            LOG_ERROR("Failed to parse \"%s\"", url);
            rv = MM_INVALID_URL;
            goto out;
        }
        u64 deviceId = Util_ParseStrictDeviceId(tempBuf);
        if (deviceId == 0) {
            LOG_ERROR("Failed to parse \"%s\"", url);
            rv = MM_INVALID_URL;
            goto out;
        }
        if (deviceId_out != NULL) {
            *deviceId_out = deviceId;
        }
        rv = VPL_OK;
    }
out:
    return rv;
}

MMError MMParseUrl(const char* urlIn,
                    u64& deviceId,
                    std::string& urlType,
                    std::string& objectId,
                    std::string& collectionId,
                    std::string& extension,
                    bool preserveObjectIdBase64)
{
    const std::string marker("/mm/");
    std::string url;
    size_t pos, nextpos;
    std::string deviceIdStr;
    std::string objectIdBase64;
    std::string collectionIdBase64;
    char *decodeBuf;
    size_t decodeBufSize;
    int rv = 0;

    url.assign(urlIn);
    nextpos = url.find(marker);
    if (nextpos == std::string::npos) {
        LOG_ERROR("Invalid URL \"%s\"", urlIn);
        rv = MM_INVALID_URL;
        goto out;
    }
    pos = nextpos + marker.size();

    // extract deviceId
    nextpos = url.find('/', pos);
    if (nextpos == url.npos) {
        LOG_ERROR("Invalid URL \"%s\"", urlIn);
        rv = MM_INVALID_URL;
        goto out;
    }
    deviceIdStr = url.substr(pos, nextpos - pos);
    deviceId = Util_ParseStrictDeviceId(deviceIdStr.c_str());
    if (deviceId == 0) {
        LOG_ERROR("Invalid URL \"%s\"", urlIn);
        rv = MM_INVALID_URL;
        goto out;
    }
    pos = nextpos + 1;  // +1 to skip the '/' after the deviceId

    // extract urlType string
    nextpos = url.find('/', pos);
    if (nextpos == url.npos) {
        LOG_ERROR("Invalid URL \"%s\"", urlIn);
        rv = MM_INVALID_URL;
        goto out;
    }
    urlType.assign(url, pos, nextpos - pos);
    pos = nextpos + 1;  // +1 to skip the '/' after the deviceId

    // extract objectId string and decode it (if required)
    nextpos = url.find('/', pos);
    if (nextpos == url.npos) {
        objectIdBase64.assign(url, pos, url.size());  // url.size() = large enough to copy everything after "pos"
    }
    else {
        objectIdBase64.assign(url, pos, nextpos - pos);
    }
    // VCS needs the object ID in Base64, but everything else does not
    if(preserveObjectIdBase64) {
        objectId.assign(objectIdBase64);
    } else {
        decodeBufSize = objectIdBase64.size() * 2;
        decodeBuf = (char*)malloc(decodeBufSize);
        if (decodeBuf == NULL) {
            LOG_ERROR("Could not allocate memory");
            rv = MM_ERR_FAIL;
            goto out;
        }
        ON_BLOCK_EXIT(free, decodeBuf);
        VPL_DecodeBase64(objectIdBase64.c_str(), objectIdBase64.size(), decodeBuf, &decodeBufSize);
        if (decodeBufSize == 0) {
            LOG_ERROR("Invalid URL \"%s\"", urlIn);
            rv = MM_INVALID_URL;
            goto out;
        }
        objectId.assign(decodeBuf, decodeBufSize);
    }
    if (nextpos == url.npos) {
        goto out;
    }
    pos = nextpos + 1;  // +1 to skip the '/' after the objectId

    // extract (optional) collectionId string
    if (urlType == "c") {
        nextpos = url.find('.', pos);
        goto skip_collection_id; // Content URLs don't include the collectionId.
    } else if (urlType == "t") {
        nextpos = url.find('/', pos);
    } else {
        LOG_ERROR("Invalid URL \"%s\"", urlIn);
        rv = MM_INVALID_URL;
        goto out;
    }

    if (nextpos == url.npos) {
        collectionIdBase64.assign(url, pos, url.size());
    }
    else {
        collectionIdBase64.assign(url, pos, nextpos - pos);
    }
    {
        decodeBufSize = collectionIdBase64.size() * 2;
        decodeBuf = (char*)malloc(decodeBufSize);
        if (decodeBuf == NULL) {
            LOG_ERROR("Could not allocate memory");
            rv = MM_ERR_FAIL;
            goto out;
        }
        ON_BLOCK_EXIT(free, decodeBuf);
        VPL_DecodeBase64(collectionIdBase64.c_str(), collectionIdBase64.size(), decodeBuf, &decodeBufSize);
        if (decodeBufSize == 0) {
            LOG_ERROR("Invalid URL \"%s\"", urlIn);
            rv = MM_INVALID_URL;
            goto out;
        }
        collectionId.assign(decodeBuf, decodeBufSize);
    }
skip_collection_id:
    if (nextpos == url.npos) {
        goto out;
    }
    pos = nextpos + 1;  // +1 to skip the '/' after the collectionId (or the '.' after the 2nd objectId).

    // extract (optional) extension string
    nextpos = url.find('/', pos);
    if (nextpos == url.npos) {
        extension.assign(url, pos, url.size());
        goto out;
    }
    extension.assign(url, pos, nextpos - pos);

out:
    return rv;
}

MMError MMRemoveExtraInfoFromUrl(const char* urlIn, std::string& urlOut)
{
    const std::string token("/mm/");
    std::string url;
    size_t currPos;
    size_t cidPos;
    int rv = 0;

    url.assign(urlIn);
    currPos = url.find(token);
    if (currPos == std::string::npos) {
        LOG_ERROR("Invalid URL \"%s\"", urlIn);
        rv = MM_INVALID_URL;
        goto out;
    }
    currPos += token.size();
    // The next 16 characters are the deviceId followed by '/'
    if (((currPos + 16) >= url.size()) || (url[currPos+16] != '/')) {
        LOG_ERROR("Invalid URL \"%s\"", url.c_str());
        rv = MM_INVALID_URL;
        goto out;
    }
    // 16 (device ID) + 1 ('/') + 1 (content/thumbnail tag) + 1 ('/')
    currPos += 19;
    // Check if there are optional components at the end; if so, ignore.
    if ((cidPos = url.find('/', currPos)) != std::string::npos) {
        urlOut.assign(url, 0, cidPos);
    }
    else {
        urlOut.assign(url);
    }
out:
    return rv;
}

} // namespace media_metadata
