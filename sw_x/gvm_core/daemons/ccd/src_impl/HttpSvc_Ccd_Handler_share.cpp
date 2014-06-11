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

#include "HttpSvc_Ccd_Handler_share.hpp"

#include "cache.h"
#include "ccdi.hpp"
#include "ccd_storage.hpp"
#include "ccd_util.hpp"
#include "HttpSvc_Ccd_Handler_Helper.hpp"
#include "HttpSvc_Utils.hpp"
#include "HttpStream.hpp"
#include "HttpStream_Helper.hpp"
#include "media_metadata_utils.hpp"
#include "MediaMetadataCache.hpp"
#include "SyncDown.hpp"
#include "vcs_util.hpp"
#include "virtual_device.hpp"
#include "util_mime.hpp"

#include "vpl_conv.h"
#include "vpl_fs.h"
#include "vplu_sstr.hpp"
#include "vplex_file.h"
#include "vplex_http_util.hpp"
#include "vplex_serialization.h"

#include <map>
#include <sstream>
#include <string>
#include <iomanip>

#include "log.h"

HttpSvc::Ccd::Handler_share::Handler_share(HttpStream *hs)
    : Handler(hs)
{
    LOG_TRACE("Handler_share[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_share::~Handler_share()
{
    LOG_TRACE("Handler_share[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_share::get_share_file()
{   // http://wiki.ctbg.acer.com/wiki/index.php/Photo_Sharing_Design#New_HTTP_API_to_fetch_shared_items
    const std::string& uri = hs->GetUri();

    // Required parameters:
    u64 feature = 0;
    u64 compId = 0;
    int type = -1;

    {   // Parse URI query parameters
        std::map<std::string, std::string> queryParams;
        VPLHttp_SplitUriQueryParams(uri, /*OUT*/ queryParams);

        std::map<std::string, std::string>::iterator mapResult;
        mapResult = queryParams.find("feature");
        if (mapResult == queryParams.end()) {
            LOG_ERROR("missing required query param:feature in (%s)", uri.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        } else {
            feature = VPLConv_strToU64(mapResult->second.c_str(), NULL, 10);
        }

        mapResult = queryParams.find("compId");
        if (mapResult == queryParams.end()) {
            LOG_ERROR("missing required query param:compId in (%s)", uri.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        } else {
            compId = VPLConv_strToU64(mapResult->second.c_str(), NULL, 10);
        }

        mapResult = queryParams.find("type");
        if (mapResult == queryParams.end()) {
            LOG_ERROR("missing required query param:type in (%s)", uri.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        } else {
            type = atoi(mapResult->second.c_str());
        }
    }

    {   // Verify arguments
        if (compId == 0) {
            LOG_ERROR("compId invalid:"FMTu64, compId);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        if (feature != ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME &&
            feature != ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME)
        {
            LOG_ERROR("feature invalid:"FMTu64, feature);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        if (type != 0 && type != 1) {
            LOG_ERROR("type invalid:%d", type);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    if (type == 1)
    {  // Thumbnail resolution requested.
        int rc = get_share_file_handleThumbnailRes(compId, feature);
        if (rc != 0) {
            LOG_ERROR("get_share_file_handleThumbnailRes("FMTu64","FMTu64"):%d. Closing connection",
                      compId, feature, rc);
            return rc;
        }
        // Http Response should be already set
    } else {
        // Shared resolution.  Need to download and serve from VCS.
        ASSERT(type == 0);  // Argument previously verified already.
        // TODO: Download the file from VCS
        int rc = get_share_file_handleSharedRes(compId, feature);
        if (rc != 0) {
            LOG_ERROR("get_share_file_handleSharedRes("FMTu64","FMTu64"):%d. Closing connection",
                      compId, feature, rc);
            return rc;
        }
        // Http Response should be already set

    }
    return 0;
}

int HttpSvc::Ccd::Handler_share::get_share_file_handleThumbnailRes(u64 compId,
                                                                   u64 feature)
{   // Thumbnail resolution should be pre-downloaded by SyncDown
    bool thumbnailDownloaded = false;
    std::string relThumbPath;

    // arguments should be already verified!
    ASSERT(compId != 0);
    ASSERT(feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME ||
           feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME);

    {
        u64 revision_unused;
        u64 datasetId_unused;
        std::string name_unused;
        int rc;
        rc = SyncDown_QuerySharedFilesEntryByCompId(compId,
                                                    feature,
                                                    /*OUT*/ revision_unused,
                                                    /*OUT*/ datasetId_unused,
                                                    /*OUT*/ name_unused,
                                                    /*OUT*/ thumbnailDownloaded,
                                                    /*OUT*/ relThumbPath);
        if (rc != 0) {
            LOG_ERROR("SyncDown_QuerySharedFilesGetThumbPathByCompId("FMTu64","FMTu64"):%d",
                      compId, feature, rc);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    if (!thumbnailDownloaded) {
        LOG_INFO("Thumbnail("FMTu64") not yet downloaded", compId);
        Utils::SetCompleteResponse(hs, 404);  // Resource not yet available
        return 0;
    }

    std::string baseAbsPath;
    if (feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME) {
        char sharedByMeDir[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForSharedByMe(hs->GetUserId(),
                                        sizeof(sharedByMeDir),
                                        sharedByMeDir);
        baseAbsPath.assign(sharedByMeDir);
    } else {
        // Should be already verified
        ASSERT(feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME);
        char sharedWithMeDir[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForSharedWithMe(hs->GetUserId(),
                                          sizeof(sharedWithMeDir),
                                          sharedWithMeDir);
        baseAbsPath.assign(sharedWithMeDir);
    }

    {   // Serve the file.
        std::string thumbnailPath;
        Util_appendToAbsPath(baseAbsPath, relThumbPath, /*OUT*/ thumbnailPath);
        std::string extension = Util_getFileExtension(Util_getChild(thumbnailPath));
        std::string mimeTypeHeader = Handler_Helper::photoMimeMap.GetMimeFromExt(extension);

        int toReturn = Handler_Helper::utilForwardLocalFile(hs,
                                                            thumbnailPath,
                                                            true,
                                                            mimeTypeHeader);
        if (toReturn != 0) {
            LOG_ERROR("utilForwardLocalFile(%s):%d. Aborting connection.",
                      thumbnailPath.c_str(), toReturn);
            return toReturn;
        }
    }

    return 0;
}

int HttpSvc::Ccd::Handler_share::get_share_file_handleSharedRes(u64 compId,
                                                                u64 feature)
{   // Shared resolution
    // arguments should be already verified!
    ASSERT(compId != 0);
    ASSERT(feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME ||
           feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME);

    std::string absBasePath;
    const VcsCategory* vcsCategory = NULL;
    if (feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME) {
        char tempBuf[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForSharedByMe(hs->GetUserId(),
                                        sizeof(tempBuf),
                                        tempBuf);
        absBasePath.assign(tempBuf);
        vcsCategory = &VCS_CATEGORY_SHARED_BY_ME;
    } else {
        // This should be already verified by here.
        ASSERT(feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME);
        char tempBuf[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForSharedWithMe(hs->GetUserId(),
                                          sizeof(tempBuf),
                                          tempBuf);
        absBasePath.assign(tempBuf);
        vcsCategory = &VCS_CATEGORY_SHARED_WITH_ME;
    }

    std::string absDestTempPath;
    Util_appendToAbsPath(absBasePath, "/temp", absDestTempPath);
    {
        int rc = Util_CreateDir(absDestTempPath.c_str());
        if (rc != 0) {
            LOG_ERROR("Util_CreateDir(%s):%d", absDestTempPath.c_str(), rc);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
    }

    u64 revision;
    u64 datasetId;
    std::string componentName;
    {
        bool thumbDownloaded_unused;
        std::string thumbRelPath_unused;
        int rc = SyncDown_QuerySharedFilesEntryByCompId(compId,
                                                        feature,
                                                        /*OUT*/ revision,
                                                        /*OUT*/ datasetId,
                                                        /*OUT*/ componentName,
                                                        /*OUT*/ thumbDownloaded_unused,
                                                        /*OUT*/ thumbRelPath_unused);
        if (rc != 0) {
            LOG_ERROR("SyncDown_QuerySharedFilesEntryByCompId("FMTu64","FMTu64"):%d",
                      compId, feature, rc);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    // TODO: Ideally there will be an adaptor to stream directly from the ACS
    //    server, but currently no modules does this and deadline is currently
    //    too tight to attempt this.  There may be complications with content-length, etc...
    //    Using tried and true download to file, then serve the file that mm and
    //    picstream handlers use.

    std::string destTempFilename;
    // Create unique filename
    destTempFilename = SSTR(compId << "_" << revision << "_" <<
            VPLDetachableThread_Self() << "_" << VPLTime_GetTimeStamp());
    std::string absDestTempFilenamePath;
    Util_appendToAbsPath(absDestTempPath, destTempFilename,
                         /*OUT*/ absDestTempFilenamePath);
    {
        int rc;
        rc = downloadFromVcsServer(compId,
                                   revision,
                                   datasetId,
                                   componentName,
                                   *vcsCategory,
                                   absDestTempFilenamePath);
        if (rc != 0) {
            LOG_ERROR("downloadFromVcsServer(%s,"FMTu64","FMTu64","FMTu64",%s):%d",
                      componentName.c_str(), compId, revision, datasetId,
                      absDestTempFilenamePath.c_str(), rc);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    // Serve temporary file that has just been downloaded.
    std::string extension = Util_getFileExtension(Util_getChild(componentName));
    std::string mimeTypeHeader = Handler_Helper::photoMimeMap.GetMimeFromExt(extension);
    int toReturn = Handler_Helper::utilForwardLocalFile(hs,
                                                        absDestTempFilenamePath,
                                                        true,
                                                        mimeTypeHeader);
    if (toReturn != 0) {
        LOG_ERROR("utilForwardLocalFile(%s):%d. Aborting connection.",
                  absDestTempFilenamePath.c_str(), toReturn);
        // Fall through to cleanup.  Need to return error code to abort connection
        // in case header was already sent.
    }

    {   // cleanup
        int rc = VPLFile_Delete(absDestTempFilenamePath.c_str());
        if (rc != 0) {
            LOG_ERROR("VPLFile_Delete(%s):%d", absDestTempFilenamePath.c_str(), rc);
        }
    }
    return toReturn;
}

static bool printLogTrue = true;

int HttpSvc::Ccd::Handler_share::downloadFromVcsServer(u64 compId,
                                                       u64 revision,
                                                       u64 datasetId,
                                                       const std::string& datasetRelPath,
                                                       const VcsCategory& category,
                                                       const std::string& absDownloadDestPath)
{
    int rv;
    VcsSession vcsSession;
    {   // Populate VcsSession
        vcsSession.userId = hs->GetUserId();
        vcsSession.deviceId = VirtualDevice_GetDeviceId();
        ServiceSessionInfo_t session;
        rv = Cache_GetSessionForVsdsByUser(vcsSession.userId, /*OUT*/session);
        if (rv != 0) {
            LOG_ERROR("Cache_GetSessionForVsdsByUser("FMTu64"):%d",
                      vcsSession.userId, rv);
            return rv;
        }
        vcsSession.sessionHandle = session.sessionHandle;
        vcsSession.sessionServiceTicket = session.serviceTicket;

        {
            ccd::GetInfraHttpInfoInput req;
            req.set_user_id(vcsSession.userId);
            req.set_service(ccd::INFRA_HTTP_SERVICE_VCS);
            ccd::GetInfraHttpInfoOutput resp;
            rv = CCDIGetInfraHttpInfo(req, resp);
            if (rv != 0) {
                LOG_ERROR("CCDIGetInfraHttpInfo("FMTu64") failed: %d",
                          vcsSession.userId, rv);
                return rv;
            }
            vcsSession.urlPrefix = resp.url_prefix();
        }
    }

    VcsDataset vcsDataset(datasetId, category);

    {
        VcsAccessInfo vcsAccessInfo;
        {   // Get AccessURL to download from storage
            VPLHttp2 httpHandle;
            // TODO: Ask Fumi how are handles cancelled on shutdown
            rv = vcs_access_info_for_file_get(vcsSession,
                                              httpHandle,
                                              vcsDataset,
                                              datasetRelPath,
                                              compId,
                                              revision,
                                              printLogTrue,
                                              /*OUT*/ vcsAccessInfo);
            if (rv != 0) {
                LOG_ERROR("vcs_access_info_for_file_get("FMTu64",%s,"FMTu64","FMTu64"):%d",
                          datasetId, datasetRelPath.c_str(), compId, revision, rv);
                return rv;
            }
        }

        {
            VPLHttp2 httpHandle;
            // TODO: Ask Fumi how are handles cancelled on shutdown
            rv = vcs_s3_getFileHelper(vcsAccessInfo,
                                      httpHandle,
                                      NULL, NULL,
                                      absDownloadDestPath,
                                      printLogTrue);
            if (rv != 0) {
                LOG_ERROR("vcs_s3_getFileHelper("FMTu64","FMTu64",%s,%s, %s, %s):%d",
                          compId, revision,
                          datasetRelPath.c_str(),
                          absDownloadDestPath.c_str(),
                          vcsAccessInfo.accessUrl.c_str(),
                          vcsAccessInfo.locationName.c_str(),
                          rv);
                return rv;
            }
        }
    }
    return rv;
}

int HttpSvc::Ccd::Handler_share::_Run()
{
    int err = 0;

    LOG_TRACE("Handler_share[%p]: Run", this);

    const std::string &uri = hs->GetUri();

    std::vector<std::string> uri_parts;
    VPLHttp_SplitUri(uri, uri_parts);

    if (uri_parts.size() < 2) {  // 2 is minimum dispatch level.
        LOG_ERROR("Incomplete URI:%s", uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    if (uri_parts[0] != "share") {
        LOG_ERROR("Bad dispatch, not share:%s", uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    bool unsupportedRequest = false;
    int unsupportedRequestLine = 0;
    if (hs->GetMethod() == "GET") {
        if (uri_parts[1] == "file") { // "/share/file"
            if (uri_parts.size() != 3)
            {   // This request has required query parameters. /share/file?queryParams
                int rc = get_share_file();
                if (rc != 0) {
                    LOG_ERROR("get_share_file(%s):%d. Closing connection",
                              uri.c_str(), rc);
                    return rc;
                }
                // Response code should already be set.
            } else {
                unsupportedRequest = true;
                unsupportedRequestLine = __LINE__;
            }
        } else {
            unsupportedRequest = true;
            unsupportedRequestLine = __LINE__;
        }
    } else {
        unsupportedRequest = true;
        unsupportedRequestLine = __LINE__;
    }

    if (unsupportedRequest) {
        LOG_ERROR("Unsupported HTTP url(%s) for Method(%s) at line(%d)",
                  uri.c_str(), hs->GetMethod().c_str(), unsupportedRequestLine);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return err;
}
