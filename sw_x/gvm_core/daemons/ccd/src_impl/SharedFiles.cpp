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
#include "SharedFiles.hpp"
#include "cache.h"
#include "ccdi.hpp"
#include "gvm_errors.h"
#include "vcs_defs.hpp"
#include "vcs_share_util.hpp"
#include "vcs_v1_util.hpp"
#include "virtual_device.hpp"
#include "vplex_http2.hpp"
#include "vpl_fs.h"
#include "vplu_format.h"
#include "log.h"

static bool s_printLog = true;

//class SharedFilesQuickShutdown {
//
//};

static int getVcsSessionInfo(u32 activationId,
                             bool getSbmDataset,
                             bool getSwmDataset,
                             VcsSession& vcsSession_out,
                             u64& sbmDatasetId_out,
                             u64& swmDatasetId_out)
{
    int rv;
    vcsSession_out.clear();
    sbmDatasetId_out = 0;
    swmDatasetId_out = 0;
    u64 updateDsetuserId;

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();  // Get the read lock to updateDatasets if needed
        if(rv != CCD_OK) {
           LOG_ERROR("Cannot obtain lock:%d", rv);
           return rv;
        }

        CachePlayer* updateDsetUser = cache_getUserByActivationId(activationId);
        if(updateDsetUser == NULL) {
           LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
           rv = CCD_ERROR_NOT_SIGNED_IN;
           return rv;
        }

        updateDsetuserId = updateDsetUser->user_id();
    }
    rv = CacheMonitor_UpdateDatasetsIfNeeded(true, updateDsetuserId);
    if (rv != 0) {
        LOG_ERROR("CacheMonitor_UpdateDatasetsIfNeeded("FMTu64"):%d, Contining",
                  updateDsetuserId, rv);
    }

    CacheAutoLock autoLock;
    rv = autoLock.LockForRead();
    if(rv != CCD_OK) {
       LOG_ERROR("Cannot obtain lock:%d", rv);
       return rv;
    }

    CachePlayer* user = cache_getUserByActivationId(activationId);
    if(user == NULL) {
       LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
       rv = CCD_ERROR_NOT_SIGNED_IN;
       return rv;
    }
    u64 userId = user->user_id();
    vcsSession_out.userId = userId;
    vcsSession_out.deviceId = VirtualDevice_GetDeviceId();
    {
        ServiceSessionInfo_t session;
        rv = Cache_GetSessionForVsdsByUser(userId, session);
        if (rv != 0){
            LOG_ERROR("Cache_GetSessionForVsdsByUser("FMTu64"):%d", userId, rv);
            return rv;
        }
        vcsSession_out.sessionHandle = session.sessionHandle;
        vcsSession_out.sessionServiceTicket = session.serviceTicket;
    }

    {
        ccd::GetInfraHttpInfoInput req;
        req.set_user_id(userId);
        req.set_service(ccd::INFRA_HTTP_SERVICE_VCS);
        req.set_secure(true);
        ccd::GetInfraHttpInfoOutput resp;
        rv = CCDIGetInfraHttpInfo(req, resp);
        if (rv < 0) {
            LOG_ERROR("CCDIGetInfraHttpInfo("FMTu64") failed: %d", userId, rv);
            return rv;
        }
        vcsSession_out.urlPrefix = resp.url_prefix();
    }

    if (getSbmDataset) {
        const vplex::vsDirectory::DatasetDetail* datasetDetail =
                Util_FindSbmDataset(user->_cachedData.details());
        if (datasetDetail != NULL) {
            sbmDatasetId_out = datasetDetail->datasetid();
        } else {
            rv = CCD_ERROR_DATASET_NOT_FOUND;
        }
    }

    if (getSwmDataset) {
        const vplex::vsDirectory::DatasetDetail* datasetDetail =
                Util_FindSwmDataset(user->_cachedData.details());
        if (datasetDetail != NULL) {
            swmDatasetId_out = datasetDetail->datasetid();
        } else {
            rv = CCD_ERROR_DATASET_NOT_FOUND;
        }
    }
    return rv;
}

int SharedFiles_StoreFile(u32 activationId,
                         const std::string& file_absPath,
                         const std::string& opaque_metadata,
                         const std::string& preview_absPath,
                         u64& compId_out,
                         std::string& name_out)
{
    int rv;
    VcsSession vcsSession;
    u64 sbmDatasetId = 0;
    u64 unused = 0;
    bool hasPreview = false;

    rv = getVcsSessionInfo(activationId,
                           true,
                           false,
                           /*OUT*/ vcsSession,
                           /*OUT*/ sbmDatasetId,
                           /*OUT*/ unused);
    if (rv != 0) {
        LOG_ERROR("getVcsSessionInfo("FMTu32",userId:"FMTu64"):%d",
                  activationId, vcsSession.userId, rv);
        return rv;
    }
    u64 userId = vcsSession.userId;
    VcsDataset vcsDataset(sbmDatasetId, VCS_CATEGORY_SHARED_BY_ME);

    LOG_INFO("StoreFile("FMTu64",file:%s,preview:%s,opaque_metadata_size:"FMT_size_t")",
             userId, file_absPath.c_str(), preview_absPath.c_str(), opaque_metadata.size());

    {
        VPLFS_stat_t photoFileStat;
        VPLFS_stat_t previewFileStat;
        rv = VPLFS_Stat(file_absPath.c_str(), &photoFileStat);
        if (rv != 0) {
            LOG_ERROR("VPLFS_Stat(%s):%d", file_absPath.c_str(), rv);
            return rv;
        }

        {
            int temp_rc = VPLFS_Stat(preview_absPath.c_str(), &previewFileStat);
            if (temp_rc != 0) {
                LOG_WARN("VPLFS_Stat(%s):%d, Continuing (%s) without preview",
                         preview_absPath.c_str(), temp_rc, file_absPath.c_str());
            } else {
                hasPreview = true;
            }
        }


        VcsAccessInfo accessInfo;
        {
            VPLHttp2 httpHandle;  // TODO: Cancellation on shutdown
            rv = VcsV1_getAccessInfo(vcsSession,
                                     httpHandle,
                                     vcsDataset,
                                     s_printLog,
                                     /*OUT*/ accessInfo);
            if (rv != 0) {
                LOG_ERROR("VcsV1_getAccessInfo:%d", rv);
                return rv;  // TODO: Retry strategy?
            }
        }

        {
            VPLHttp2 httpHandle;  // TODO: Cancellation on shutdown
            rv = vcs_s3_putFileHelper(accessInfo,
                                      httpHandle,
                                      NULL,
                                      NULL,
                                      file_absPath,
                                      s_printLog);
            if (rv != 0) {
                LOG_ERROR("vcs_s3_putFileHelper(%s):%d", file_absPath.c_str(), rv);
                return rv;  // TODO: Retry strategy?
            }
        }
        VcsV1_postFileMetadataResponse pfmResponse;
        {
            std::string child = Util_getChild(file_absPath);
            // TODO: What if child isn't unique?  Append date/time?
            // TODO: Need to escape opaque_metadata quotes

            VPLHttp2 httpHandle;  // TODO: Cancellation on shutdown
            rv = VcsV1_share_postFileMetadata(vcsSession,
                                              httpHandle,
                                              vcsDataset,
                                              child,
                                              photoFileStat.vpl_mtime,
                                              photoFileStat.vpl_ctime,
                                              (u64) photoFileStat.size,
                                              true,
                                              opaque_metadata,
                                              false,
                                              std::string("unusedContentHash"),
                                              VirtualDevice_GetDeviceId(),
                                              accessInfo.accessUrl,
                                              s_printLog,
                                              /*OUT*/ pfmResponse);
            if (rv != 0) {
                LOG_ERROR("VcsV1_postFileMetadata(%s):%d", file_absPath.c_str(), rv);
                return rv;
            }
            if ((u64)pfmResponse.numOfRevisions != (u64)pfmResponse.revisionList.size()) {
                LOG_WARN("Mismatch in numRevision("FMTu64") and vectorSize("FMT_size_t")",
                         pfmResponse.numOfRevisions, pfmResponse.revisionList.size());
            }
            compId_out = pfmResponse.compId;
            name_out = pfmResponse.name;
        }

        if (hasPreview) {
            u64 revisionId = pfmResponse.revisionList[pfmResponse.revisionList.size()-1].revision;
            VPLHttp2 httpHandle;  // TODO: Cancellation on shutdown
            rv = VcsV1_putPreview(vcsSession,
                                  httpHandle,
                                  vcsDataset,
                                  name_out,
                                  compId_out,
                                  revisionId,
                                  preview_absPath,
                                  s_printLog);
            if (rv != 0) {
                LOG_ERROR("VcsV1_putPreview(%s,%s,%s,"FMTu64"):%d",
                          preview_absPath.c_str(),
                          file_absPath.c_str(),
                          name_out.c_str(), compId_out, rv);
                return rv;
            }
        }
    }
    LOG_INFO("SharedFile(userId:"FMTu64",%s) preview(%d,%s) success.  ("FMTu64",name:%s)",
             userId, file_absPath.c_str(), hasPreview, preview_absPath.c_str(),
             compId_out, name_out.c_str());
    return 0;
}

int SharedFiles_ShareFile(u32 activationId,
                          const std::string& datasetRelPath,
                          u64 compId,
                          const std::vector<std::string>& recipientEmails)
{
    int rv;
    VcsSession vcsSession;
    u64 sbmDatasetId = 0;
    u64 unused = 0;

    rv = getVcsSessionInfo(activationId,
                           true,
                           false,
                           /*OUT*/ vcsSession,
                           /*OUT*/ sbmDatasetId,
                           /*OUT*/ unused);
    if (rv != 0) {
        LOG_ERROR("getVcsSessionInfo("FMTu32",userId:"FMTu64"):%d",
                  activationId, vcsSession.userId, rv);
        return rv;
    }
    u64 userId = vcsSession.userId;
    VcsDataset vcsDataset(sbmDatasetId, VCS_CATEGORY_SHARED_BY_ME);

    {
        VPLHttp2 httpHandle;  // TODO: Cancellation on shutdown
        rv = VcsShare_share(vcsSession,
                            httpHandle,
                            vcsDataset,
                            datasetRelPath,
                            compId,
                            recipientEmails,
                            s_printLog);
        if (rv != 0) {
            LOG_ERROR("VcsShare_share(userId:"FMTu64",compId:"FMTu64",file:%s):%d",
                      userId, compId, datasetRelPath.c_str(), rv);
            return rv;
        }
    }

    LOG_INFO("ShareFile(userId:"FMTu64",compId:"FMTu64",file:%s)",
             userId, compId, datasetRelPath.c_str());
    return 0;
}

int SharedFiles_UnshareFile(u32 activationId,
                            const std::string& datasetRelPath,
                            u64 compId,
                            const std::vector<std::string>& recipientEmails)
{
    int rv;
    VcsSession vcsSession;
    u64 sbmDatasetId = 0;
    u64 unused = 0;

    rv = getVcsSessionInfo(activationId,
                           true,
                           false,
                           /*OUT*/ vcsSession,
                           /*OUT*/ sbmDatasetId,
                           /*OUT*/ unused);
    if (rv != 0) {
        LOG_ERROR("getVcsSessionInfo("FMTu32",userId:"FMTu64"):%d",
                  activationId, vcsSession.userId, rv);
        return rv;
    }
    u64 userId = vcsSession.userId;
    VcsDataset vcsDataset(sbmDatasetId, VCS_CATEGORY_SHARED_BY_ME);

    {
        VPLHttp2 httpHandle;  // TODO: Cancellation on shutdown
        rv = VcsShare_unshare(vcsSession,
                              httpHandle,
                              vcsDataset,
                              datasetRelPath,
                              compId,
                              recipientEmails,
                              s_printLog);
        if (rv != 0) {
            LOG_ERROR("VcsShare_unshare(userId:"FMTu64",compId:"FMTu64",file:%s):%d",
                      userId, compId, datasetRelPath.c_str(), rv);
            return rv;
        }
    }

    LOG_INFO("UnshareFile(userId:"FMTu64",compId:"FMTu64",file:%s)",
             userId, compId, datasetRelPath.c_str());
    return 0;
}

int SharedFiles_DeleteFile(u32 activationId,
                           const std::string& datasetRelPath,
                           u64 compId)
{
    int rv;
    VcsSession vcsSession;
    u64 swmDatasetId = 0;
    u64 unused = 0;

    rv = getVcsSessionInfo(activationId,
                           false,
                           true,
                           /*OUT*/ vcsSession,
                           /*OUT*/ unused,
                           /*OUT*/ swmDatasetId);
    if (rv != 0) {
        LOG_ERROR("getVcsSessionInfo("FMTu32",userId:"FMTu64"):%d",
                  activationId, vcsSession.userId, rv);
        return rv;
    }
    u64 userId = vcsSession.userId;
    VcsDataset vcsDataset(swmDatasetId, VCS_CATEGORY_SHARED_WITH_ME);

    {
        VPLHttp2 httpHandle;  // TODO: Cancellation on shutdown
        rv = VcsShare_deleteFileSharedWithMe(vcsSession,
                                             httpHandle,
                                             vcsDataset,
                                             datasetRelPath,
                                             compId,
                                             s_printLog);
        if (rv != 0) {
            LOG_ERROR("VcsShare_deleteFileSharedWithMe(userId:"FMTu64",compId:"FMTu64",file:%s):%d",
                      userId, compId, datasetRelPath.c_str(), rv);
            return rv;
        }
    }

    LOG_INFO("VcsShare_deleteFileSharedWithMe(userId:"FMTu64",compId:"FMTu64",file:%s)",
             userId, compId, datasetRelPath.c_str());
    return 0;
}
