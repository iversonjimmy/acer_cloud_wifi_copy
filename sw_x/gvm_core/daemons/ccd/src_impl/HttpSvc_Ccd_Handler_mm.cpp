#include "HttpSvc_Ccd_Handler_mm.hpp"

#include "HttpSvc_Ccd_Handler_Helper.hpp"
#include "HttpSvc_Ccd_MediaFile.hpp"
#include "HttpSvc_Ccd_MediaFileSender.hpp"
#include "HttpSvc_Utils.hpp"
#include "HttpStream_Helper.hpp"

#include "ccdi.hpp"
#include "cache.h"
#include "ccd_storage.hpp"
#include "ccd_util.hpp"
#include "config.h"
#include "vcs_util.hpp"
#include "pin_manager.hpp"
#include "SyncFeatureMgr.hpp"
#include "virtual_device.hpp"
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
#include "image_transcode.h"
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE

#include <log.h>
#include <media_metadata_utils.hpp>
#include <MediaMetadataCache.hpp>
#include <HttpStream.hpp>
#include <util_mime.hpp>

#include <vpl_fs.h>
#include <vplu_sstr.hpp>
#include <vplex_file.h>
#include <vplex_http_util.hpp>
#include <vplex_serialization.h>

#include <map>
#include <sstream>
#include <string>
#include <iomanip>

#define HEADER_ACT_XCODE_DIMENSION "act_xcode_dimension"
#define HEADER_ACT_XCODE_FMT     "act_xcode_fmt"
#define PARAMETER_WIDTH          "width"
#define PARAMETER_HEIGHT         "height"
#define PARAMETER_FMT            "fmt"

HttpSvc::Ccd::Handler_mm::Handler_mm(HttpStream *hs)
    : Handler(hs)
{
    LOG_TRACE("Handler_mm[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_mm::~Handler_mm()
{
    LOG_TRACE("Handler_mm[%p]: Destroyed", this);
}

bool HttpSvc::Ccd::Handler_mm::trySatisfyFromCache(HttpStream *hs)
{
    const std::string &uri = hs->GetUri();
    if (uri.find("/t/") == uri.npos) {
        // Not a thumbnail. Only thumbnails are cached.
        return false;
    }
    LOG_INFO("Check cache for thumbnail uri %s", uri.c_str());

    u64 deviceId;
    std::string urlType, objectId, collectionId, extension;
    MMError mmerr = media_metadata::MMParseUrl(uri.c_str(),
                                                        deviceId,
                                                        urlType,
                                                        objectId,
                                                        collectionId,
                                                        extension,
                                                        false);
    if(mmerr != 0 || collectionId.empty() || extension.empty()) {
        // if either of these is missing, it's not possible to construct the local path to the thumbnail file
        return false;
    }

    char defaultMediaPath[CCD_PATH_MAX_LENGTH];
    DiskCache::getDirectoryForMediaMetadataDownload(hs->GetUserId(), sizeof(defaultMediaPath), defaultMediaPath);
    std::string mediaPath(defaultMediaPath);
    {
        int rc;
        CacheAutoLock lock;
        rc = lock.LockForRead();
        if(rc != 0) {
            LOG_WARN("Unable to obtain cache lock:%d", rc);
            return false;
        }
        u64 userId = hs->GetUserId();
        CachePlayer* user = cache_getSyncUser(true, userId);
        if(user == NULL) {
            LOG_ERROR("Not Logged In:"FMTu64, userId);
            return false;
        }
        if(user->_cachedData.details().has_mm_thumb_download_path()) {
            mediaPath = user->_cachedData.details().mm_thumb_download_path();
        }
    }

    std::string thumbnail_dir, thumbnail_file;
    VPLFS_stat_t stat;
    int catTypeIndex;
    static const int MAX_CAT_TYPE = 4;
    VPLFile_handle_t file = VPLFILE_INVALID_HANDLE;
    for(catTypeIndex = 1; catTypeIndex < MAX_CAT_TYPE; catTypeIndex++) {
        media_metadata::CatalogType_t catType;
        switch((media_metadata::CatalogType_t)catTypeIndex) {
        case media_metadata::MM_CATALOG_MUSIC:
            catType = media_metadata::MM_CATALOG_MUSIC;
            break;
        case media_metadata::MM_CATALOG_PHOTO:
            catType = media_metadata::MM_CATALOG_PHOTO;
            break;
        case media_metadata::MM_CATALOG_VIDEO:
            catType = media_metadata::MM_CATALOG_VIDEO;
            break;
        default:
            LOG_ERROR("Could not find thumbnail, type:%d", catTypeIndex);
            return false;
        }
        media_metadata::MediaMetadataCache::formThumbnailCollectionDir(
                std::string(mediaPath),
                catType,
                media_metadata::METADATA_FORMAT_THUMBNAIL,
                deviceId,
                collectionId,
                false,
                0,
                thumbnail_dir);
        media_metadata::MediaMetadataCache::formThumbnailFilename(objectId, extension, thumbnail_file);
        std::string thumbnail_path = thumbnail_dir + "/" + thumbnail_file;

        LOG_DEBUG("Checking thumbnail_path %s", thumbnail_path.c_str());

        int rc;
        if ((rc=VPLFS_Stat(thumbnail_path.c_str(), &stat)) != VPL_OK) {
            LOG_DEBUG("Failed stat on thumbnail file %s, type:%d, %d",
                     thumbnail_path.c_str(), catTypeIndex, rc);
            continue;
        }

        if ((rc=VPLFile_CheckAccess(thumbnail_path.c_str(), VPLFILE_CHECKACCESS_READ)) != VPL_OK) {
            LOG_DEBUG("Cannot read thumbnail %s, type:%d, %d",
                     thumbnail_path.c_str(), catTypeIndex, rc);
            continue;
        }

        // Open file for read.
        file =  VPLFile_Open(thumbnail_path.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(file)) {
            LOG_DEBUG("Failed to open thumbnail file %s, type:%d", thumbnail_path.c_str(), catTypeIndex);
            continue;
        }else{
            // Success, no errors in accessing the file for streaming, we can break out.
            LOG_INFO("Found thumbnail at %s", thumbnail_path.c_str());
            break;
        }
    }
    if(catTypeIndex == MAX_CAT_TYPE) {
        LOG_INFO("Could not find thumbnail in cache.");
        return false;
    }

    Utils::AddStdRespHeaders(hs);

    // Check range request.
    u64 start, end;
    u64 total;
    char contentLengthValue[128];
    char contentRangeValue[128];

    std::string reqRange;
    int err = hs->GetReqHeader(Utils::HttpHeader_Range, reqRange);
    if (err == 0) {
        LOG_INFO("Range request found: %s", reqRange.c_str());
        int ret = -1;
        ret = Util_ParseRange(&reqRange, stat.size, start, end);
        if(ret < 0){
            goto invalidRange;
        }
    }
    else {
        // Get whole entity.
        start = 0;
        end = stat.size - 1;
    }
    total = end - start + 1;
    
    // Set-up response
    // Indicate if whole file or only part being returned.
    if(start == 0 && end == stat.size - 1) {
        hs->SetStatusCode(200);
    }
    else {
        hs->SetStatusCode(206);
        sprintf(contentRangeValue, "bytes "FMTu64"-"FMTu64"/"FMTu_VPLFS_file_size_t, 
                start, end, stat.size);
        hs->SetRespHeader(Utils::HttpHeader_ContentRange, contentRangeValue); 
    }

    sprintf(contentLengthValue, FMTu64, total);
    hs->SetRespHeader(Utils::HttpHeader_ContentLength, contentLengthValue); 
    {
        std::string mimeType = Handler_Helper::photoMimeMap.GetMimeFromExt(extension);
        hs->SetRespHeader(Utils::HttpHeader_ContentType, mimeType);
    }

    if (VPLFile_IsValidHandle(file)) {
        VPLFile_offset_t offset = VPLFile_Seek(file, start, VPLFILE_SEEK_SET);
        if (offset < 0) {
            LOG_ERROR("VPLFile_Seek:%d", (int)offset);
            hs->SetStatusCode(500);
            goto end;
        }

        do {
            char buffer[32 * 1024];
            ssize_t bytesWritten;
            size_t chunksize = (size_t)total > ARRAY_SIZE_IN_BYTES(buffer) ? ARRAY_SIZE_IN_BYTES(buffer) : (size_t)total;
            ssize_t nbytes = VPLFile_Read(file, buffer, chunksize);
            if (nbytes < 0) {
                LOG_ERROR("Failed to read from file: err "FMTd_ssize_t, nbytes);
                hs->SetStatusCode(500);
                goto end;
            }
            bytesWritten = hs->Write(buffer, nbytes);
            if (bytesWritten != nbytes) {
                LOG_ERROR("Write(%d):%d", (int)nbytes, (int)bytesWritten);
                hs->SetStatusCode(500);
                goto end;
            }
            total -= chunksize;
        } while (total > 0);
    }

    goto end;

 invalidRange:
    if(!reqRange.empty()) {
        LOG_ERROR("Range \"%s\" invalid for size "FMTu_VPLFS_file_size_t".",
                  reqRange.c_str(), stat.size);
    }
    sprintf(contentRangeValue, "bytes */"FMTu_VPLFile_offset_t, 
            stat.size);
    hs->SetRespHeader(Utils::HttpHeader_ContentRange, contentRangeValue); 
    hs->SetStatusCode(416);
    hs->Flush();

 end:
    if (VPLFile_IsValidHandle(file)) {
        int rc = VPLFile_Close(file);
        if (rc != 0) {
            LOG_ERROR("VPLFile_Close:%d", rc);
        }
    }
    return true;
}

static bool cloudPcIsAccessible(u64 userId, const std::string& uri)
{
    bool useCloudPc = false;

    // One constraint is only go to VCS for thumbnails
    if (uri.find("/t/") == uri.npos) {
        // Not a thumbnail. Don't use VCS in that case.
        // If the cloudPc is not accessible, the higher level will
        // get an error later.
        return true;
    }

    int err;
    u64 deviceId;

    // Extract the device ID in order to find the CloudPC
    err = media_metadata::MMGetDeviceFromUrl(uri.c_str(), &deviceId);
    if (err) {
        LOG_ERROR("MMGetDeviceFromUrl failed: err %d", err);
        return true;
    }

    do {
        int rc;
        CacheAutoLock lock;
        rc = lock.LockForRead();
        if(rc != 0) {
            LOG_WARN("Unable to obtain cache lock:%d", rc);
            break;
        }
        CachePlayer* user = cache_getSyncUser(true, userId);
        if(user == NULL) {
            LOG_ERROR("Not Logged In:"FMTu64, userId);
            break;
        }

        // Note: still hold the cache lock for read at this point
        {
            ccd::ListUserStorageInput request;
            ccd::ListUserStorageOutput listSnOut;
            request.set_user_id(userId);
            request.set_only_use_cache(true);
            rc = CCDIListUserStorage(request, listSnOut);
            if (rc != 0) {
                LOG_ERROR("CCDIListUserStorage for user("FMTu64") failed %d", userId, rc);
                break;
            }
            for (int i = 0; i < listSnOut.user_storage_size(); i++) {
                //
                // Find SN and check whether it is accessible
                //
                if (listSnOut.user_storage(i).storageclusterid() == deviceId) {
                    LOG_DEBUG("CloudPC deviceId is "FMTu64, deviceId);
                    for (int j = 0; j < listSnOut.user_storage(i).storageaccess_size(); j++) {
                        // If there is a routetype, check it.
                        if (!listSnOut.user_storage(i).storageaccess(j).has_routetype()) continue;
                        // If a PROXY or DIN route is found, that means it should be reachable.
                        vplex::vsDirectory::RouteType rtype = listSnOut.user_storage(i).storageaccess(j).routetype();
                        if (rtype == vplex::vsDirectory::PROXY ||
                            rtype == vplex::vsDirectory::DIRECT_INTERNAL) {
                            LOG_DEBUG("Found usable route to CloudPC of type %d", rtype);
                            useCloudPc = true;
                            break;
                        }
                    }
                }
            }
        }
        // Bail if not found
        if (!useCloudPc) {
            break;
        }

        {
            //
            // SN found, now check that the device status is ONLINE
            //
            useCloudPc = false;
            ccd::ListLinkedDevicesInput request;
            ccd::ListLinkedDevicesOutput listDevOut;
            request.set_user_id(userId);
            request.set_only_use_cache(true);
            rc = CCDIListLinkedDevices(request, listDevOut);
            if (rc != 0) {
                LOG_ERROR("CCDIListLinkedDevices for user("FMTu64") failed %d", userId, rc);
                break;
            }
            for (int i = 0; i < listDevOut.devices_size(); i++) {
                if (listDevOut.devices(i).device_id() == deviceId) {
                    if (listDevOut.devices(i).connection_status().state() == ccd::DEVICE_CONNECTION_ONLINE) {
                        LOG_DEBUG("CloudPC "FMTu64" status is ONLINE", deviceId);
                        useCloudPc = true;
                    }
                    break;
                }
            }
        }
    } while(ALWAYS_FALSE_CONDITIONAL);

    if (useCloudPc) {
        LOG_INFO("CloudPC is ONLINE, fetch thumbnail from "FMTu64, deviceId);
    } else {
        LOG_INFO("CloudPC not accessible, fetch thumbnail from VCS");
    }

    return useCloudPc;
}

static const std::string get_extension(const std::string& path)
{
    std::string ext;
    size_t pos = path.find_last_of("/.");
    if ((pos != path.npos) && (path[pos] == '.')) {
        ext.assign(path, pos + 1, path.size());
    }
    return ext;
}

int HttpSvc::Ccd::Handler_mm::_Run()
{
    int err = 0;

    LOG_TRACE("Handler_mm[%p]: Run", this);

    if(__ccdConfig.useThumbnailCache &&
        trySatisfyFromCache(hs))
    {
        LOG_DEBUG("Returned from trySatisfyFromCache");
        return 0;
    }

    const std::string &uri = hs->GetUri();

    if (isLocalReservedContent(uri)) {
        LOG_INFO("Handler_mm[%p]: Is local reserved content", this);
        if (hs->GetMethod() == "HEAD" || hs->GetMethod() == "GET") {
            bool is_replied = false;
            bool is_head = hs->GetMethod() == "HEAD";
            err = tryToSendLocalReservedContent(hs, is_head, is_replied);
            if (err == 0 && is_replied) {
                LOG_DEBUG("Handler_mm[%p]: Returned from tryToGetFromLocalFileSystem", this);
                return 0;
            } else if (err && !is_replied) {
                LOG_WARN("Handler_mm[%p]: Something failed, forward to this request to media server. (rv = %d)", this, err);
            } else if (err && err == VPL_ERR_CONNRESET) {
                LOG_WARN("Handler_mm[%p]: Connection was closed by peer. (rv = %d)", this, err);
                return err;
            } else if (err) {
                LOG_ERROR("Handler_mm[%p]: Error happened in tryToGetFromLocalFileSystem(). (rv = %d)", this, err);
                return err;
            } else {
                // err == 0 && is_replied == false, this case should never happen.
                LOG_ERROR("Handler_mm[%p]: This case should never happen. (rv = %d)", this, err);
            }
        } else {
            LOG_ERROR("Handler_mm[%p]: Unsupported http method: %s", this, hs->GetMethod().c_str());
            Utils::SetCompleteResponse(hs, 400);
            return -1;
        }
    } else {
        LOG_INFO("Handler_mm[%p]: Is not local reserved content.", this);
    }

    bool preferCloudPcOverVcs = cloudPcIsAccessible(hs->GetUserId(), uri);

    if (preferCloudPcOverVcs) {
        err = prepareRequest(hs);
        if (err) {
            // errmsg logged and response set by prepareRequest()
            return 0;  // reset error
        }
        err = getFromStorageServer(hs);
        if (err == 0) {
            return 0;
        }
    }
    else
    {   //
        // Bug 12330: Use either CloudPC or VCS.  Can't try one then the other.
        // The problem is that once a proxy to StorageServer has started above,
        // there's no way to "recover" if the proxy failed.  For example, if the proxy
        // failed mid-stream and returned a 400 status code for the request, there's
        // nothing further that can be done after that, since the failure will already
        // have been reported all the way back to the client.
        //
        std::string destPath;
        {
            char tempBuf[CCD_PATH_MAX_LENGTH];
            DiskCache::getPathForUserHttpThumbTemp(hs->GetUserId(),
                                                   CCD_PATH_MAX_LENGTH,
                                                   tempBuf);
            destPath = tempBuf;
            int rc = Util_CreateDir(destPath.c_str());
            if (rc != 0) {
                LOG_ERROR("Util_CreateDir(%s):%d", destPath.c_str(), rc);
                return rc;
            }
        }
        std::string filename_out;
        u64 compId = 0;
        u64 revisionId = 0;

        err = getFromVcsServer(hs,
                               destPath,
                               /*OUT*/ filename_out,
                               /*OUT*/ compId,
                               /*OUT*/ revisionId);
        if (err != 0) {
            LOG_ERROR("getFromVcsServer(%s,"FMTu64","FMTu64",%s):%d",
                      destPath.c_str(), compId, revisionId, filename_out.c_str(), err);
            Utils::SetCompleteResponse(hs, 400);
            return err;
        }
        err = forwardDownloadedVcsFile(hs, destPath + std::string("/") + filename_out);
        // forwardDownloadedVcsFile always sets hs->SetStatusCode.
        if (err != 0) {
            LOG_ERROR("forwardDownloadedVcsFile(%s,%s):%d",
                destPath.c_str(), filename_out.c_str(), err);
            goto error;
        }

        // In case of an error, we simply propagate the error to the caller.
        // This is because we don't know where the error happened.
        // Case to consider: what if http response status 200 was already returned,
        //    but then something went wrong in the middle of transferring the body..
     error:
        {
            std::string completePath = destPath + std::string("/") + filename_out;
            int rc = VPLFile_Delete(completePath.c_str());
            if (rc != 0) {
                LOG_ERROR("VPLFile_Delete(%s):%d", completePath.c_str(), rc);
            }
        }
    }
    return err;
}

static
int lookupThumbnailComponent(u64 userId, u64 datasetId, const std::string& syncConfigRelPath,
        u64& compId_out, u64& revision_out, bool& isOnAcs_out, std::string& datasetRelPath_out)
{
    int rv;
    rv = SyncFeatureMgr_LookupComponentByPath(userId, datasetId,
            ccd::SYNC_FEATURE_PHOTO_THUMBNAILS, syncConfigRelPath,
            /*out*/compId_out, /*out*/revision_out, /*out*/isOnAcs_out);
    if (rv == 0) {
        datasetRelPath_out = SSTR("/ph/t/" << syncConfigRelPath);
        return 0;
    }
    rv = SyncFeatureMgr_LookupComponentByPath(userId, datasetId,
            ccd::SYNC_FEATURE_MUSIC_THUMBNAILS, syncConfigRelPath,
            /*out*/compId_out, /*out*/revision_out, /*out*/isOnAcs_out);
    if (rv == 0) {
        datasetRelPath_out = SSTR("/mu/t/" << syncConfigRelPath);
        return 0;
    }
    rv = SyncFeatureMgr_LookupComponentByPath(userId, datasetId,
            ccd::SYNC_FEATURE_VIDEO_THUMBNAILS, syncConfigRelPath,
            /*out*/compId_out, /*out*/revision_out, /*out*/isOnAcs_out);
    if (rv == 0) {
        datasetRelPath_out = SSTR("/vi/t/" << syncConfigRelPath);
        return 0;
    }
    return CCD_ERROR_NOT_FOUND;
}

int HttpSvc::Ccd::Handler_mm::getFromVcsServer(HttpStream *hs,
                                               const std::string& destPath,
                                               std::string& destFilename_out,
                                               u64& compId_out,
                                               u64& revision_out)
{
    int rv = 0;
    destFilename_out.clear();
    compId_out = 0;
    revision_out = 0;
    const bool printLogTrue = true;

    const std::string &uri = hs->GetUri();

    LOG_DEBUG("uri %s", uri.c_str());
    if (uri.find("/t/") == uri.npos) {
        // Not a thumbnail. This logic is specific to thumbnails.
        LOG_DEBUG("Not a thumbnail request %s", uri.c_str());
        return -1;
    }

    u64 datasetId;
    std::string datasetRelPath;
    {
        u64 deviceId;
        std::string urlType;
        std::string objectId;
        std::string collectionId;
        std::string extension;

        rv = media_metadata::MMParseUrl(uri.c_str(),
                                                 deviceId,
                                                 urlType,
                                                 objectId,
                                                 collectionId,
                                                 extension,
                                                 true);
        if (rv) {
            LOG_WARN("Handler_mm[%p]: MMParseUrl failed: err %d", this, rv);
            return rv;
        }
        rv = GetMediaMetadataDatasetByUserId(hs->GetUserId(), datasetId);
        if (rv != 0) {
            LOG_WARN("GetMediaMetadataDatasetByUserId("FMTu64"):%d",
                    hs->GetUserId(), rv);
            return rv;
        }
        if (__ccdConfig.mediaMetadataThumbVirtDownload) {
            std::string syncConfigRelPath = SSTR(
                    "ca" << std::setfill('0') << std::setw(16) << std::setbase(16) << deviceId <<
                    "/co" << collectionId <<
                    "/" << objectId << "." << extension);
            bool isOnAcs;
            rv = lookupThumbnailComponent(hs->GetUserId(), datasetId, syncConfigRelPath,
                        /*out*/compId_out, /*out*/revision_out, /*out*/isOnAcs,
                        datasetRelPath);
            if (rv != 0) {
                LOG_INFO("lookupThumbnailComponent(%s) returned: %d", syncConfigRelPath.c_str(), rv);
                return rv;
            }
            if (!isOnAcs) {
                LOG_INFO("Thumbnail for \"%s\" is not on ACS", syncConfigRelPath.c_str());
                return CCD_ERROR_NOT_FOUND;
            }
        } else {
            // Note: This old implementation assumes photo thumbnails only:
            datasetRelPath = SSTR("/ph/t/ca" <<
                    std::setfill('0') << std::setw(16) << std::setbase(16) << deviceId <<
                    "/co" << collectionId <<
                    "/" << objectId << "." << extension);
            LOG_INFO("VCS uri %s", datasetRelPath.c_str());
        }
    }

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

    VcsDataset vcsDataset(datasetId, VCS_CATEGORY_METADATA);
    if (__ccdConfig.mediaMetadataThumbVirtDownload) {
        // compId_out and revisionId_out were already populated above.
    } else {
        {   // Get the compId of the object to download
            VPLHttp2 httpHandle;
            // TODO: Ask Fumi how are handles cancelled on shutdown
            rv = vcs_get_comp_id(vcsSession,
                                 httpHandle,
                                 vcsDataset,
                                 datasetRelPath,
                                 printLogTrue,
                                 /*OUT*/ compId_out);
            if(rv != 0) {
                LOG_ERROR("vcs_get_comp_id("FMTu64",%s,"FMTu64",%s):%d",
                          vcsSession.userId, vcsSession.urlPrefix.c_str(),
                          datasetId, datasetRelPath.c_str(), rv);
                return rv;
            }
        }

        {   // Get the revisionId;
            VcsFileMetadataResponse fileMetadataResp;
            VPLHttp2 httpHandle;

            // TODO: Ask Fumi how are handles cancelled on shutdown
            rv = vcs_get_file_metadata(vcsSession,
                                       httpHandle,
                                       vcsDataset,
                                       datasetRelPath,
                                       compId_out,
                                       printLogTrue,
                                       /*OUT*/ fileMetadataResp);
            if(rv != 0) {
                LOG_ERROR("vcs_get_file_metadata("FMTu64",%s,"FMTu64"):%d",
                          datasetId, datasetRelPath.c_str(), compId_out, rv);
                return rv;
            }
            if(fileMetadataResp.revisionList.size() == 1 &&
               fileMetadataResp.numOfRevisions == 1)
            {
                revision_out = fileMetadataResp.revisionList[0].revision;
            } else {
                LOG_ERROR("Server Error, more than one revision??? "
                          "vcs_get_file_metadata("FMTu64",%s,"FMTu64"): numRevision:("FMTu64","FMTu_size_t")",
                          datasetId, datasetRelPath.c_str(), compId_out,
                          fileMetadataResp.numOfRevisions,
                          fileMetadataResp.revisionList.size());
                return CCD_ERROR_INTERNAL;
            }
        }
    }

    // Create unique filename
    destFilename_out = SSTR(compId_out << "_" << revision_out << "_" <<
            VPLDetachableThread_Self() << "_" << VPLTime_GetTimeStamp());

    {
        VcsAccessInfo vcsAccessInfo;
        {   // Get AccessURL to download from storage
            VPLHttp2 httpHandle;
            // TODO: Ask Fumi how are handles cancelled on shutdown
            rv = vcs_access_info_for_file_get(vcsSession,
                                              httpHandle,
                                              vcsDataset,
                                              datasetRelPath,
                                              compId_out,
                                              revision_out,
                                              printLogTrue,
                                              /*OUT*/ vcsAccessInfo);
            if (rv != 0) {
                LOG_ERROR("vcs_access_info_for_file_get("FMTu64",%s,"FMTu64","FMTu64"):%d",
                          datasetId, datasetRelPath.c_str(), compId_out, revision_out, rv);
                return rv;
            }
        }

        {
            std::string absPath = SSTR(destPath << "/" << destFilename_out);
            VPLHttp2 httpHandle;
            // TODO: Ask Fumi how are handles cancelled on shutdown
            rv = vcs_s3_getFileHelper(vcsAccessInfo,
                                      httpHandle,
                                      NULL, NULL,
                                      absPath,
                                      printLogTrue);
            if (rv != 0) {
                LOG_ERROR("vcs_s3_getFileHelper(%s(%s,%s), %s, %s):%d",
                          absPath.c_str(),
                          destPath.c_str(),
                          destFilename_out.c_str(),
                          vcsAccessInfo.accessUrl.c_str(),
                          vcsAccessInfo.locationName.c_str(),
                          rv);
                return rv;
            }
        }
    }
    return rv;
}

int HttpSvc::Ccd::Handler_mm::prepareRequest(HttpStream *hs)
{
    int err = 0;

    const std::string &uri = hs->GetUri();

    {
        std::string serverUrl;
        err = media_metadata::MMRemoveExtraInfoFromUrl(uri.c_str(), serverUrl);
        if (err) {
            LOG_ERROR("Handler_mm[%p]: MCARemoveOptionsFromUrl failed: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return err;
        }
        hs->SetUri(serverUrl);
    }

    // If there is parameter - 'fmt', add Http Header - 'act_xcode_fmt' for photo transcoding.
    {
        std::string str_fmt;
        int rc = hs->GetQuery(PARAMETER_FMT, str_fmt);
        if (!rc) {
            hs->SetReqHeader(HEADER_ACT_XCODE_FMT, str_fmt);
        }
    }

    // If there is parameter - 'width' and 'height', add Http Header - 'act_xcode_dimension' for photo transcoding.
    {
        std::string str_width, str_height;
        int rc = hs->GetQuery(PARAMETER_WIDTH, str_width);
        if (!rc) {
            rc = hs->GetQuery(PARAMETER_HEIGHT, str_height);
            if (!rc) {
                std::ostringstream oss;
                oss << str_width << "," << str_height;
                hs->SetReqHeader(HEADER_ACT_XCODE_DIMENSION, oss.str());
            }
        }
    }

    u64 deviceId;
    err = media_metadata::MMGetDeviceFromUrl(uri.c_str(), &deviceId);
    if (err) {
        LOG_ERROR("Handler_mm[%p]: MMGetDeviceFromUrl failed: err %d", this, err);
        Utils::SetCompleteResponse(hs, 400);
        return 0;  // reset error
    }
    hs->SetDeviceId(deviceId);

    hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
    hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);

    // NOTE: If err != 0, the response code must be set
    return err;
}

int HttpSvc::Ccd::Handler_mm::getFromStorageServer(HttpStream *hs)
{
    return Handler_Helper::ForwardToServerCcd(hs);
}

int HttpSvc::Ccd::Handler_mm::forwardDownloadedVcsFile(HttpStream *hs,
                                                       const std::string& pathToForward)
{   // Similar to HttpSvc::Ccd::Handler_share::utilForwardLocalFile.
    // This function must always set hs->SetStatusCode before returning.
    
    // TODO: Find a way to abstract this function, the main issue is setting http headers.
    
    const std::string &uri = hs->GetUri();

    u64 deviceId;
    std::string urlType, objectId, collectionId, extension;
    MMError mmerr = media_metadata::MMParseUrl(uri.c_str(),
                                                        deviceId,
                                                        urlType,
                                                        objectId,
                                                        collectionId,
                                                        extension,
                                                        true);
    if(mmerr != 0 || collectionId.empty() || extension.empty()) {
        // if either of these is missing, it's not possible to construct the local path to the thumbnail file
        LOG_ERROR("MMParseUrl(%s):(%s,%s,%s,%s,%d)", uri.c_str(),
                  collectionId.c_str(), objectId.c_str(),
                  collectionId.c_str(), extension.c_str(), mmerr);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    {
        std::string mimeType = Handler_Helper::photoMimeMap.GetMimeFromExt(extension);
        int toReturn = Handler_Helper::utilForwardLocalFile(hs,
                                                            pathToForward,
                                                            true,
                                                            mimeType);
        if (toReturn != 0) {
            LOG_ERROR("utilForwardLocalFile(%s):%d. Aborting connection.",
                      pathToForward.c_str(), toReturn);
            return toReturn;
        }
    }
    return 0;
}

bool HttpSvc::Ccd::Handler_mm::isLocalReservedContent(const std::string& uri)
{
    int rv;
    u64 device_id;
    std::vector<std::string> uri_tokens;
    std::string url_type;
    std::string object_id;
    std::string collection_id;
    std::string extension;
    PinnedMediaItem pinned_media_item;

    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 4 || uri_tokens[3].empty()) {
        LOG_WARN("Handler_mm[%p]: Not local reserved content URL: %s", this, uri.c_str());
        return false;
    }

    rv = media_metadata::MMParseUrl(uri.c_str(),
                                             device_id,
                                             url_type,
                                             object_id,
                                             collection_id,
                                             extension,
                                             false);
    if (rv ||
        device_id == 0 ||
        object_id.empty()) {
        LOG_WARN("Handler_mm[%p]: Can not get device_id from URL, uri = %s, rv = %d", this, uri.c_str(), rv);
        return false;
    }

    rv = PinnedMediaManager_GetPinnedMediaItem(object_id, device_id, pinned_media_item);
    if (rv) {
        LOG_DEBUG("Handler_mm[%p]: Not in the pinned media database. (object_id=%s, device_id="FMTu64", rv=%d)", this, object_id.c_str(), device_id, rv);
        return false;
    }

    return true;
}

int HttpSvc::Ccd::Handler_mm::tryToSendLocalReservedContent(HttpStream *hs,
                                                            bool isHead,
                                                            bool& is_replied_out)
{
    int rv = 0;
    const std::string &uri = hs->GetUri();
    std::string uri_without_query;
    std::vector<std::string> uri_tokens;
    u64 device_id;
    std::string url_type;
    std::string object_id;
    std::string collection_id;
    std::string extension;
    PinnedMediaItem pinned_media_item;
    MediaFile* mediafile = NULL;
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
    bool do_transcode;
    std::string fmt;
    size_t width;
    size_t height;
    ImageTranscode_ImageType image_type;
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE

    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 4 || uri_tokens[3].empty()) {
        LOG_WARN("Handler_mm[%p]: Not local reserved content URL: %s", this, uri.c_str());
        is_replied_out = false;
        rv = -1;
        goto end;
    }

    if (uri.find("?") != std::string::npos) {
        uri_without_query = uri.substr(0, uri.find("?"));
    } else {
        uri_without_query = uri;
    }

    rv = media_metadata::MMParseUrl(uri_without_query.c_str(),
                                    device_id,
                                    url_type,
                                    object_id,
                                    collection_id,
                                    extension,
                                    false);
    if (rv ||
        device_id == 0 ||
        object_id.empty()) {
        LOG_WARN("Handler_mm[%p]: Can not get device_id from URL, uri = %s, rv = %d", this, uri.c_str(), rv);
        is_replied_out = false;
        goto end;
    }

    rv = PinnedMediaManager_GetPinnedMediaItem(object_id, device_id, pinned_media_item);
    if (rv) {
        LOG_WARN("Handler_mm[%p]: Not in the pinned media database. (object_id=%s, device_id="FMTu64", rv=%d)", this, object_id.c_str(), device_id, rv);
        is_replied_out = false;
        goto end;
    }

    // If file extension can not get from URL parsing, get the file extension from file path.
    if (extension.empty()) {
        extension = get_extension(pinned_media_item.path);
        LOG_INFO("Handler_mm[%p]: Get file extension from file path, ext = %s", this, extension.c_str());
    }

    LOG_DEBUG("Handler_mm[%p]: file_path: %s, extension: %s", this, pinned_media_item.path.c_str(), extension.c_str());

    mediafile = MediaFile::Create(pinned_media_item.path);
    if (!mediafile) {
        LOG_ERROR("Handler_mm[%p]: Not enough memory", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        goto end;
    }

#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
    image_type = ImageType_Original;
    do_transcode = false;
    width = 0;
    height = 0;
    {
        int rc = hs->GetQuery(PARAMETER_FMT, fmt);
        if (!rc) {
            VPLHttp_Trim(fmt);
            std::transform(fmt.begin(), fmt.end(), fmt.begin(), (int(*)(int))toupper);
            if (fmt == "JPG") {
                image_type = ImageType_JPG;
            } else if (fmt == "PNG") {
                image_type = ImageType_PNG;
            } else if (fmt == "TIFF") {
                image_type = ImageType_TIFF;
            } else if (fmt == "BMP") {
                image_type = ImageType_BMP;
            } else {
                LOG_ERROR("Handler_mm[%p]: Unsupported image type %s", this, fmt.c_str());
                std::ostringstream oss;
                oss << "{\"errMsg\":\"Unsupported image type " << fmt << ".\"}";
                HttpStream_Helper::SetCompleteResponse(hs, 415, oss.str(), "application/json");
                rc = 0;
                goto end;
            }
        }
    }
    {
        std::string str_width, str_height;
        int rc = hs->GetQuery(PARAMETER_WIDTH, str_width);
        if (!rc) {
            rc = hs->GetQuery(PARAMETER_HEIGHT, str_height);
            if (!rc) {
                std::ostringstream oss;
                oss << str_width << "," << str_height;
                width = atoi(str_width.c_str());
                height = atoi(str_height.c_str());
                do_transcode = true;
            }
        }
    }

    if (do_transcode) {
        rv = mediafile->Transcode(width, height, image_type);
        if (rv) {
            LOG_ERROR("Handler_mm[%p]: Failed to transcode; err %d", this, rv);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to transcode\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 415, oss.str(), "application/json");
            goto end;
        }
    }
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE

    {
        std::string mediaType;
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
        if (do_transcode && !fmt.empty()) {
            mediaType = Util_FindPhotoMediaTypeFromExtension(fmt);
            if (mediaType.empty()) {
                mediaType.assign("image/unknown");
            }
        } else {
            mediaType = Util_FindPhotoMediaTypeFromExtension(extension);
        }
#else
        mediaType = Util_FindPhotoMediaTypeFromExtension(extension);
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE

        MediaFileSender* mfs = new (std::nothrow) MediaFileSender(mediafile, mediaType, hs);
        if (!mfs) {
            LOG_ERROR("Handler_mm[%p]: Not enough memory", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Not enough memory\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
            return CCD_ERROR_NOMEM;
        }

        rv = mfs->Send();

        delete mfs;
    }

end:
    if (mediafile) {
        delete mediafile;
        mediafile = NULL;
    }

    return rv;
}

