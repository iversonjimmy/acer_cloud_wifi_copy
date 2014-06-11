//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __VCS_COMMON_HPP_4_28_2014__
#define __VCS_COMMON_HPP_4_28_2014__

#include "vpl_plat.h"
#include "vplex_http2.hpp"

#include <string>
#include <vector>

// This file contains definitions and functions that are required to access
// VCS and are shared between multiple VCS versions.

struct VcsCategory {
    const std::string str;
    VcsCategory(const char* cstring) : str(cstring) {}
};

struct VcsSession {
    u64 userId;

    u64 deviceId;

    // urlPrefix includes protocol (scheme name), domain, and port
    // For example, https://www-c100.pc-int.igware.net:443
    std::string urlPrefix;

    u64 sessionHandle;

    // sessionServiceTicket - binary byte array (no encoding) representing the
    // serviceTicket.
    std::string sessionServiceTicket;

    void clear() {
        userId = 0;
        deviceId = 0;
        urlPrefix.clear();
        sessionHandle = 0;
        sessionServiceTicket.clear();
    }
    VcsSession(){ clear(); }
    VcsSession(u64 userId, u64 deviceId, const std::string& urlPrefix, u64 sessionHandle, const std::string& sessionServiceTicket) :
        userId(userId), deviceId(deviceId), urlPrefix(urlPrefix), sessionHandle(sessionHandle), sessionServiceTicket(sessionServiceTicket) {}
};

struct VcsHeaderPair {
    std::string headerName;
    std::string headerValue;
};

struct VcsAccessInfo {
    std::string accessUrl;
    std::vector<VcsHeaderPair> header;
    std::string locationName;
    void clear() {
        accessUrl.clear();
        header.clear();
        locationName.clear();
    }
    VcsAccessInfo(){ clear(); }
};

// TODO: There is probably a better name for this
int vcs_translate_http_status_code(int httpStatus);

/// Upload a file to an HTTP or HTTPS location, as instructed by VCS.
// TODO: rename this to http_put_file_using_vcs_accessinfo().
int vcs_s3_putFileHelper(const VcsAccessInfo& accessInfo,
                         VPLHttp2& httpHandle,
                         VPLHttp2_ProgressCb progressCb,
                         void* progressCbCtx,
                         const std::string& srcLocalFilepath,
                         bool printLog);
int vcs_s3_putFileHelper(const VcsAccessInfo& accessInfo,
                         VPLHttp2& httpHandle,
                         VPLHttp2_ProgressCb progressCb,
                         void* progressCbCtx,
                         const std::string& srcLocalFilepath,
                         bool printLog,
                         std::string& hash__out);
//TODO: Expected callback version of putFileHelper, to make sure a consistent
// file is uploaded.

/// Download a file from an HTTP or HTTPS location, as instructed by VCS.
// TODO: rename this to http_get_file_using_vcs_accessinfo().
int vcs_s3_getFileHelper(const VcsAccessInfo& accessInfo,
                         VPLHttp2& httpHandle,
                         VPLHttp2_ProgressCb progressCb,
                         void* progressCbCtx,
                         const std::string& destLocalFilepath,
                         bool printLog);

#endif // include guard
