//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "vcs_share_util.hpp"
#include "vcs_common.hpp"
#include "vcs_common_priv.hpp"
#include "gvm_file_utils.hpp"
#include "scopeguard.hpp"
#include "vplex_http_util.hpp"

#include <string>
#include <sstream>
#include <vector>

#include "log.h"

static int addSessionHeaders(const VcsSession& vcsSession,
                             VPLHttp2& httpHandle)
{   // See http://wiki.ctbg.acer.com/wiki/index.php/VCS_Design#Authentication
    int rc;
    // Share API is currently at VCS_API_VERSION 3
    rc = addSessionHeadersHelper(vcsSession,
                                 httpHandle,
                                 3);  // VCS_API_VERSION
    if (rc != 0) {
        LOG_ERROR("addSessionHeadersHelper:%d", rc);
    }
    return rc;
}

int VcsShare_share(const VcsSession& vcsSession,
                   VPLHttp2& httpHandle,
                   const VcsDataset& dataset,
                   const std::string& componentName,
                   u64 compId,
                   const std::vector<std::string>& recipientEmail,
                   bool printLog)
{
    int rv = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string componentNameLeadingOrTrailingSlash;
    Util_trimSlashes(componentName, componentNameLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/sbm/share/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(componentNameLeadingOrTrailingSlash, "/");
    ss << "?compId=" << compId;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    ss.str("");
    ss << "{"
       <<     "\"recipients\":"
       <<         "[";
    bool notFirst = false;
    for (std::vector<std::string>::const_iterator emailIter = recipientEmail.begin();
         emailIter != recipientEmail.end(); ++emailIter)
    {
        if (notFirst) {
            ss << ",";
        }
        ss << "\"" << *emailIter << "\"";
        notFirst = true;
    }
    ss <<         "]"
       << "}";
    std::string jsonBody = ss.str();
    if(printLog){ LOG_ALWAYS("jsonBody:%s", jsonBody.c_str()); }

    std::string httpPostResponse;
    rv = httpHandle.Post(jsonBody, httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http Post:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64,
                  rv, dataset.id, componentName.c_str(), compId);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    // On success, there is nothing returned.

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}

int VcsShare_unshare(const VcsSession& vcsSession,
                     VPLHttp2& httpHandle,
                     const VcsDataset& dataset,
                     const std::string& componentName,
                     u64 compId,
                     const std::vector<std::string>& recipientEmail,
                     bool printLog)
{
    int rv = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string componentNameLeadingOrTrailingSlash;
    Util_trimSlashes(componentName, componentNameLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/sbm/unshare/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(componentNameLeadingOrTrailingSlash, "/");
    ss << "?compId=" << compId;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    ss.str("");
    ss << "{"
       <<     "\"recipients\":"
       <<         "[";
    bool notFirst = false;
    for (std::vector<std::string>::const_iterator emailIter = recipientEmail.begin();
         emailIter != recipientEmail.end(); ++emailIter)
    {
        if (notFirst) {
            ss << ",";
        }
        ss << "\"" << *emailIter << "\"";
        notFirst = true;
    }
    ss <<         "]"
       << "}";
    std::string jsonBody = ss.str();
    if(printLog){ LOG_ALWAYS("jsonBody:%s", jsonBody.c_str()); }

    std::string httpPostResponse;
    rv = httpHandle.Post(jsonBody, httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http Post:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64,
                  rv, dataset.id, componentName.c_str(), compId);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    // On success, there is nothing returned.

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}

int VcsShare_deleteFileSharedWithMe(const VcsSession& vcsSession,
                                    VPLHttp2& httpHandle,
                                    const VcsDataset& dataset,
                                    const std::string& path,
                                    u64 compId,
                                    bool printLog)
{
    int rv = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string pathLeadingOrTrailingSlash;
    Util_trimSlashes(path, pathLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/swm/unshare/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(pathLeadingOrTrailingSlash, "/");
    ss << "?compId=" << compId;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string emptyBody;
    std::string httpPostResponse;
    rv = httpHandle.Post(emptyBody, httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http Post:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64,
                  rv, dataset.id, path.c_str(), compId);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    // On success, there is nothing returned.

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}
