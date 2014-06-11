#include "HttpSvc_Sn_Handler_vcs_archive.hpp"

#include "HttpSvc_Sn_ASFileSender.hpp"
#include "HttpSvc_Sn_MediaFileSender.hpp"
#include "HttpSvc_Sn_MediaFsFile.hpp"

#include "dataset.hpp"
#include "vss_server.hpp"

#include <ccdi.hpp>
#include <ccdi_orig_types.h>

#include <cJSON2.h>
#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_conv.h>
#include <vplex_http_util.hpp>

#include <cassert>
#include <sstream>
#include <string>

// TODO: duplicated from sw_x/gvm_core/daemons/ccd/src_impl/HttpSvc_Utils.hpp (which we can't access from here).
static const std::string HttpHeader_ac_datasetRelPath = "X-ac-datasetRelPath";
static const std::string HttpHeader_ac_tolerateFileModification = "X-ac-tolerateFileMod";

//========================
// TODO: logic copied from HttpSvc_Sn_Handler_rf.cpp; refactor to a common location.
//========================
namespace {
struct EnabledFeatures {
    bool mediaServer;
    bool remoteFileAccess;

};
}

static int getEnabledFeatures(u64 userId, u64 deviceId, EnabledFeatures &enabledFeatures)
{
    enabledFeatures.mediaServer = false;
    enabledFeatures.remoteFileAccess = false;

    ccd::ListUserStorageInput request;
    request.set_user_id(userId);
    request.set_only_use_cache(true);
    ccd::ListUserStorageOutput listSnOut;
    int err = CCDIListUserStorage(request, listSnOut);
    if (err) {
        LOG_ERROR("CCDIListUserStorage failed: err %d", err);
        return err;
    }
    for (int i = 0; i < listSnOut.user_storage_size(); i++) {
        if (listSnOut.user_storage(i).storageclusterid() == deviceId) {
            enabledFeatures.mediaServer = listSnOut.user_storage(i).featuremediaserverenabled();
            enabledFeatures.remoteFileAccess = listSnOut.user_storage(i).featureremotefileaccessenabled();
            return 0;
        }
    }
    LOG_ERROR("Unknown storage device ID "FMTu64": userId "FMTu64, deviceId, userId);
    return err;
}
//========================

HttpSvc::Sn::Handler_vcs_archive::Handler_vcs_archive(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_vcs_archive[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Sn::Handler_vcs_archive::~Handler_vcs_archive()
{
    LOG_INFO("Handler_vcs_archive[%p]: Destroyed", this);
}

int HttpSvc::Sn::Handler_vcs_archive::Run()
{
    LOG_INFO("Handler_vcs_archive[%p]: Run", this);

    const std::string &uri = hs->GetUri();
    // URI has the format: /vcs_archive/<deviceId>/<datasetId>/<compId>/<revision>

    int err = 0;

    std::vector<std::string> uri_tokens;
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() == 5) {
        // Good to go!
    } else if ((uri_tokens.size() == 6) && (uri_tokens[5].size() == 0)) {
        // Special case to ignore a trailing slash.
        // This shouldn't have happened, but VCS didn't follow the spec originally.
    } else {
        LOG_WARN("Handler_vcs_archive[%p]: Unexpected number (%d) of segments; uri %s", this, (int)uri_tokens.size(), uri.c_str());
        HttpStream_Helper::SetCompleteResponse(hs, 400,
                JSON_ERRMSG("Unexpected number of segments; uri " << uri),
                "application/json");
        return 0;
    }
    assert(uri_tokens[0] == "vcs_archive");

    // Confirm that the deviceId is the local device.
    u64 requestedDeviceId = VPLConv_strToU64(uri_tokens[1].c_str(), NULL, 10);
    if (requestedDeviceId != vssServer->clusterId) {
        LOG_WARN("Handler_vcs_archive[%p]: Requested deviceId is not the local device; uri %s", this, uri.c_str());
        HttpStream_Helper::SetCompleteResponse(hs, 404,
                JSON_ERRMSG("Wrong deviceId: " << requestedDeviceId),
                "application/json");
        return 0;
    }

    // Confirm that "Archive Storage Device File Serving" is enabled for the local device.
    {
        EnabledFeatures enabledFeatures;
        err = getEnabledFeatures(hs->GetUserId(), vssServer->clusterId, enabledFeatures);
        if (err) {
            LOG_ERROR("Handler_vcs_archive[%p]: Failed to determine enabled features; err %d", this, err);
            HttpStream_Helper::SetCompleteResponse(hs, 500,
                    JSON_ERRMSG("Failed to determine enabled features"),
                    "application/json");
            return 0;  // reset error
        }
        
        // TODO: ideally, we should base this on which dataset is being requested, but I don't
        //   think this can do much harm the way it is now.
        if (!enabledFeatures.mediaServer && !vssServer->isSyncboxArchiveStorage()) {
            LOG_WARN("Handler_vcs_archive[%p]: Local device does not have Archive Storage Device File Serving enabled; uri %s", this, uri.c_str());
            HttpStream_Helper::SetCompleteResponse(hs, 403,
                    JSON_ERRMSG("Archive Storage Device File Serving not enabled"),
                    "application/json");
            return 0;
        }
    }

    bool tolerateFileModification = false;
    {
        std::string tolerateFileModificationStr;
        err = hs->GetReqHeader(HttpHeader_ac_tolerateFileModification, tolerateFileModificationStr);
        if (err != 0) {
            if (err == CCD_ERROR_NOT_FOUND) {
                // This is okay.
                err = 0;
            } else {
                LOG_WARN("Handler_vcs_archive[%p]: Failed to get req header: %d", this, err);
                HttpStream_Helper::SetCompleteResponse(hs, 500,
                        JSON_ERRMSG("Failed to parse headers"),
                        "application/json");
                return 0;
            }
        } else {
            if (tolerateFileModificationStr == "1") {
                tolerateFileModification = true;
            }
        }
    }

    if (!tolerateFileModification) {
        LOG_WARN("Handler_vcs_archive[%p]: (tolerateFileModification == false) is not supported yet", this);
        HttpStream_Helper::SetCompleteResponse(hs, 400,
                JSON_ERRMSG("Caller must tolerate file modification"),
                "application/json");
        return 0;
    }

    u64 datasetId = VPLConv_strToU64(uri_tokens[2].c_str(), NULL, 10);
    u64 componentId = VPLConv_strToU64(uri_tokens[3].c_str(), NULL, 10);
    u64 revision = VPLConv_strToU64(uri_tokens[4].c_str(), NULL, 10);

    // Get the absolute path of the file to serve.
    ccd::GetSyncStateOutput ccdiResponse;
    {
        std::string datasetRelPath;
        {
            std::string encodedDatasetRelPath;
            err = hs->GetReqHeader(HttpHeader_ac_datasetRelPath, encodedDatasetRelPath);
            if (err == 0) {
                err = VPLHttp_DecodeUri(encodedDatasetRelPath, datasetRelPath);
                if (err != 0) {
                    LOG_WARN("Handler_vcs_archive[%p]: Invalid encoding for datasetRelPath: %s",
                            this, encodedDatasetRelPath.c_str());
                    HttpStream_Helper::SetCompleteResponse(hs, 400,
                            JSON_ERRMSG("Invalid encoding for " << HttpHeader_ac_datasetRelPath),
                            "application/json");
                    return 0;
                }
            } else if (err == CCD_ERROR_NOT_FOUND) {
                // This is okay, as long as datasetId isn't the media metadata dataset.
                // If it is the media metadata dataset, the lookup call to CCDIGetSyncState will
                // return an error in ccdiResponse.lookup_abs_path().err_code().
                err = 0;
            } else {
                LOG_WARN("Handler_vcs_archive[%p]: Failed to get req header: %d", this, err);
                HttpStream_Helper::SetCompleteResponse(hs, 500,
                        JSON_ERRMSG("Failed to parse headers"),
                        "application/json");
                return 0;
            }
        }

        LOG_INFO("Handler_vcs_archive[%p]: request is for: dset"FMTu64",comp"FMTu64",rev"FMTu64",%s",
                this, datasetId, componentId, revision, datasetRelPath.c_str());

        {
            ccd::GetSyncStateInput ccdiRequest;
            ccdiRequest.set_only_use_cache(true);
            ccdiRequest.mutable_lookup_abs_path()->set_dataset_id(datasetId);
            ccdiRequest.mutable_lookup_abs_path()->set_component_id(componentId);
            ccdiRequest.mutable_lookup_abs_path()->set_revision(revision);
            ccdiRequest.mutable_lookup_abs_path()->set_dataset_rel_path(datasetRelPath);
            err = CCDIGetSyncState(ccdiRequest, ccdiResponse);
            if (err != 0) {
                LOG_WARN("Handler_vcs_archive[%p]: CCDIGetSyncState failed: %d", this, err);
                HttpStream_Helper::SetCompleteResponse(hs, 500,
                        JSON_ERRMSG("Failed to perform lookup (" << err << ")."),
                        "application/json");
                return 0;
            }
            if (ccdiResponse.lookup_abs_path().err_code() != 0) {
                LOG_WARN("Handler_vcs_archive[%p]: lookup_abs_path failed: %d",
                        this, ccdiResponse.lookup_abs_path().err_code());
                if (ccdiResponse.lookup_abs_path().err_code() == SYNC_CONFIG_ERR_REVISION_IS_OBSOLETE) {
                    HttpStream_Helper::SetCompleteResponse(hs, 404,
                            JSON_ERRMSG("Obsolete component/revision (" << componentId << "/" << revision << ")."),
                            "application/json");
                }
                else if (ccdiResponse.lookup_abs_path().err_code() == SYNC_CONFIG_ERR_REVISION_NOT_READY) {
                    HttpStream_Helper::SetCompleteResponse(hs, 500,
                            JSON_ERRMSG("Not ready to serve component/revision (" << componentId << "/" << revision << ")."),
                            "application/json");
                } else {
                    HttpStream_Helper::SetCompleteResponse(hs, 500,
                            JSON_ERRMSG("Error " << ccdiResponse.lookup_abs_path().err_code() <<
                                    " serving component/revision (" << componentId << "/" << revision << ")"),
                            "application/json");
                }
                return 0;
            }
        }
    }
    const std::string& absPathToServe = ccdiResponse.lookup_abs_path().absolute_path();

    LOG_INFO("Handler_vcs_archive[%p]: serving %s", this, absPathToServe.c_str());
    {
        MediaFsFile mf(absPathToServe);
        u64 localModifyTime = ccdiResponse.lookup_abs_path().local_modify_time();
        std::string hash = ccdiResponse.lookup_abs_path().hash();
        ASFileSender asfs(&mf, localModifyTime, hash, hs);
        err = asfs.Send();
        // We rely on our caller (HttpSvc::Sn::Dispatcher::Dispatch) to log the result.
    }
    return err;
}
