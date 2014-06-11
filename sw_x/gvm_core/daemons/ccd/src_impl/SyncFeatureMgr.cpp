//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "SyncFeatureMgr.hpp"

#include "ans_connection.hpp"  // For ANSConn_IsActive
#include "AsyncDatasetOps.hpp"
#include "cache.h"
#include "ccdi.hpp"
#include "ccd_storage.hpp"
#include "config.h"
#include "EventManagerPb.hpp"
#include "file_url_operation_ts.hpp"
#include "MediaMetadata.hpp"
#include "picstream.hpp"
#include "scopeguard.hpp"
#include "SyncConfigManager.hpp"
#if CCD_ENABLE_SYNCDOWN
#include "SyncDown.hpp"
#endif  // CCD_ENABLE_SYNCDOWN
#include "vcs_archive_access_ccd.hpp"
#include "vcs_file_url_access_basic_impl.hpp"
#include "vcs_file_url_operation_http.hpp"
#include "virtual_device.hpp"
#include "vplex_file.h"
#include "vpl_fs.h"
#include "vpl_lazy_init.h"
#include "vplex_strings.h"
#include "vplex_sync_agent_notifier.pb.h"
#include "vsds_query.hpp"

#include <algorithm>
#include <map>

using namespace ccd;

#define FMT_DatasetId FMTu64
#define FMT_SC_HANDLE "%p"
#define ARG_SC_HANDLE(h_) ((h_).x)

/// Holds the record that ties a SyncFeature to a SyncConfig instance.
struct FeatureSyncConfig_t {
    u64 userId;
    u64 datasetId;
    ccd::SyncFeature_t syncFeature;

    SyncConfigHandle scHandle; // Handle returned from registering syncConfig

    SyncType type;
    std::string localDir;
    std::string serverDir;
    u64 maxStorage;
    u64 maxFiles;

    FeatureSyncConfig_t(
            u64 userId,
            u64 datasetId,
            ccd::SyncFeature_t syncFeature,
            SyncConfigHandle scHandle,
            SyncType type,
            const std::string& localDir,
            const std::string& serverDir,
            u64 maxStorage,
            u64 maxFiles)
    :   userId(userId),
        datasetId(datasetId),
        syncFeature(syncFeature),
        scHandle(scHandle),
        type(type),
        localDir(localDir),
        serverDir(serverDir),
        maxStorage(maxStorage),
        maxFiles(maxFiles)
    {}
};


/// Protects the following fields.
static VPLLazyInitMutex_t s_mutex = VPLLAZYINITMUTEX_INIT;
/// Protected by #s_mutex.
//{
static SyncConfigThreadPool* s_oneWayUpThreadPool = NULL;
static SyncConfigThreadPool* s_oneWayDownThreadPool = NULL;
static SyncConfigManager* syncConfigMgr;
static std::vector<FeatureSyncConfig_t*> s_syncConfigs;
//}

/// Since we release s_mutex in the middle of SyncFeatureMgr_Add() and in all of the remove functions,
/// we use this mutex to protect against adding the same SyncFeature twice or starting a SyncFeature
/// while a previous instance of it is still shutting down.
static VPLLazyInitMutex_t s_addOrRemoveInProgressMutex = VPLLAZYINITMUTEX_INIT;

int SyncFeatureMgr_Start()
{
    int rv;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (syncConfigMgr != NULL) {
        return CCD_ERROR_ALREADY;
    }
    syncConfigMgr = CreateSyncConfigManager(rv);
    if (syncConfigMgr == NULL) {
        LOG_WARN("CreateSyncConfigManager failed: %d", rv);
    }
    return rv;
}

void SyncFeatureMgr_Stop()
{
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    if (s_syncConfigs.size() > 0) {
        LOG_ERROR("Expected all users to be removed before stopping!");
        SyncFeatureMgr_RemoveAll();
    }
    MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_oneWayUpThreadPool != NULL) {
        // There should be no active threads, so this MessyShutdown should not be
        // doing anything messy (it will return an error if there's still an active
        // task and prevent other tasks from being enqueued)
        int rc = s_oneWayUpThreadPool->CheckAndPrepareShutdown();
        if (rc != 0) {
            u32 unoccupied_threads;
            u32 total_threads;
            bool shuttingDown;
            s_oneWayUpThreadPool->GetInfoIncludeDedicatedThread(NULL,
                                                     /*OUT*/unoccupied_threads,
                                                     /*OUT*/total_threads,
                                                     /*OUT*/shuttingDown);
            LOG_ERROR("s_oneWayUpThreadPool.Shutdown:%d, unoccupied_threads(%d), "
                      "total_threads(%d), shuttingDown(%d).  THERE SHOULD BE NO TASKS!",
                      rc, unoccupied_threads, total_threads, shuttingDown);
        }
        delete s_oneWayUpThreadPool;
        s_oneWayUpThreadPool = NULL;
    }

    if (s_oneWayDownThreadPool != NULL) {
        // There should be no active threads, so this MessyShutdown should not be
        // doing anything messy (it will return an error if there's still an active
        // task and prevent other tasks from being enqueued)
        int rc = s_oneWayDownThreadPool->CheckAndPrepareShutdown();
        if (rc != 0) {
            u32 unoccupied_threads;
            u32 total_threads;
            bool shuttingDown;
            s_oneWayDownThreadPool->GetInfoIncludeDedicatedThread(NULL,
                                                     /*OUT*/unoccupied_threads,
                                                     /*OUT*/total_threads,
                                                     /*OUT*/shuttingDown);
            LOG_ERROR("s_oneWayDownThreadPool.Shutdown:%d, unoccupied_threads(%d), "
                      "total_threads(%d), shuttingDown(%d).  THERE SHOULD BE NO TASKS!",
                      rc, unoccupied_threads, total_threads, shuttingDown);
        }
        delete s_oneWayDownThreadPool;
        s_oneWayDownThreadPool = NULL;
    }

    DestroySyncConfigManager(syncConfigMgr);
    syncConfigMgr = NULL;
}

static ccd::SyncFeature_t callbackContextToFeature(void* cbContext)
{
    return (ccd::SyncFeature_t)(intptr_t)cbContext;
}

static FeatureSyncStateType_t syncConfigStatusToSyncFeatureState(
        bool has_error,
        SyncConfigStatus status)
{
    FeatureSyncStateType_t result;
    if (has_error) {
        result = CCD_FEATURE_STATE_OUT_OF_SYNC;
    } else if ((status == SYNC_CONFIG_STATUS_DONE) || (status == SYNC_CONFIG_STATUS_SCANNING)) {
        result = CCD_FEATURE_STATE_IN_SYNC;
    } else {
        result = CCD_FEATURE_STATE_SYNCING;
    }
    return result;
}

static int sendEventSyncHistoryDownload(SyncConfigEventEntryDownloaded& event)
{
    ccd::SyncEventType_t type;
    switch (event.detail_type) {
        case SYNC_CONFIG_EVENT_DETAIL_NEW_FILE:
            type = CCD_SYNC_EVENT_NEW_FILE_DOWNLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_MODIFIED_FILE:
            type = CCD_SYNC_EVENT_MODIFIED_FILE_DOWNLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_FILE_DELETE:
            type = CCD_SYNC_EVENT_FILE_DELETE_DOWNLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_FOLDER_CREATE:
            type = CCD_SYNC_EVENT_FOLDER_CREATE_DOWNLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_FOLDER_DELETE:
            type = CCD_SYNC_EVENT_FOLDER_DELETE_DOWNLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_CONFLICT_FILE_CREATED:
            type = CCD_SYNC_EVENT_CONFLICT_FILE_CREATED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_NONE:
        default:
            LOG_ERROR("Got unexpected syncHistory type: %d", (int)event.detail_type);
            return CCDI_ERROR_PARAMETER;
    }
    
    {
        ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
        ccdiEvent->mutable_sync_history()->set_type(type);
        ccdiEvent->mutable_sync_history()->set_path(event.abs_path);
        ccdiEvent->mutable_sync_history()->set_feature(SYNC_FEATURE_SYNCBOX);
        ccdiEvent->mutable_sync_history()->set_dataset_id(event.dataset_id);
        ccdiEvent->mutable_sync_history()->set_event_time(event.event_time);
        ccdiEvent->mutable_sync_history()->set_conflict_file_original_path(event.conflict_file_original_path);
        
        EventManagerPb_AddEvent(ccdiEvent);
        // ccdiEvent will be freed by EventManagerPb.
    }
    return 0;
}

static int sendEventSyncHistoryUpload(SyncConfigEventEntryUploaded& event)
{
    ccd::SyncEventType_t type;
    switch (event.detail_type) {
        case SYNC_CONFIG_EVENT_DETAIL_NEW_FILE:
            type = CCD_SYNC_EVENT_NEW_FILE_UPLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_MODIFIED_FILE:
            type = CCD_SYNC_EVENT_MODIFIED_FILE_UPLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_FILE_DELETE:
            type = CCD_SYNC_EVENT_FILE_DELETE_UPLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_FOLDER_CREATE:
            type = CCD_SYNC_EVENT_FOLDER_CREATE_UPLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_FOLDER_DELETE:
            type = CCD_SYNC_EVENT_FOLDER_DELETE_UPLOADED;
            break;
        case SYNC_CONFIG_EVENT_DETAIL_CONFLICT_FILE_CREATED:
            LOG_ERROR("Got Conflict + Upload syncHistory type for Syncbox. Should not happen.");
            return CCDI_ERROR_PARAMETER;
        case SYNC_CONFIG_EVENT_DETAIL_NONE:
        default:
            LOG_ERROR("Got unexpected syncHistory type: %d", (int)event.detail_type);
            return CCDI_ERROR_PARAMETER;
    }

    {
        ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
        ccdiEvent->mutable_sync_history()->set_type(type);
        ccdiEvent->mutable_sync_history()->set_path(event.abs_path);
        ccdiEvent->mutable_sync_history()->set_feature(SYNC_FEATURE_SYNCBOX);
        ccdiEvent->mutable_sync_history()->set_dataset_id(event.dataset_id);
        ccdiEvent->mutable_sync_history()->set_event_time(event.event_time);
        // conflict_file_original_path is only for download
        
        EventManagerPb_AddEvent(ccdiEvent);
        // ccdiEvent will be freed by EventManagerPb.
    }
    return 0;
}

static bool isScanInProgress(SyncConfigStatus status)
{
    return (status == SYNC_CONFIG_STATUS_SCANNING);
}

/// A #SyncConfigEventCallback.
static void eventCb(SyncConfigEvent& rawEvent)
{
    ccd::SyncFeature_t feature = callbackContextToFeature(rawEvent.callback_context);
    switch (rawEvent.type) {
    case SYNC_CONFIG_EVENT_STATUS_CHANGE:
        {
            SyncConfigEventStatusChange& event =
                    static_cast<SyncConfigEventStatusChange&>(rawEvent);
            FeatureSyncStateType_t prevState =
                    syncConfigStatusToSyncFeatureState(event.prev_has_error, event.prev_status);
            FeatureSyncStateType_t currState =
                    syncConfigStatusToSyncFeatureState(event.new_has_error, event.new_status);
            if (feature == SYNC_FEATURE_SYNCBOX) {
                // There are additional Syncbox-specific fields.
                if (currState != prevState ||
                        event.new_status != event.prev_status ||
                        event.new_uploads_remaining != event.prev_uploads_remaining ||
                        event.new_downloads_remaining != event.prev_downloads_remaining||
                        event.new_remote_scan_pending != event.prev_remote_scan_pending) {
                    ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
                    bool scan_in_progress = isScanInProgress(event.new_status);
                    ccdiEvent->mutable_sync_feature_status_change()->set_feature(feature);
                    ccd::FeatureSyncStateSummary* mutable_status =
                        ccdiEvent->mutable_sync_feature_status_change()->mutable_status();
                    mutable_status->set_status(currState);
                    mutable_status->set_uploads_remaining(event.new_uploads_remaining);
                    mutable_status->set_downloads_remaining(event.new_downloads_remaining);
                    mutable_status->set_remote_scan_pending(event.new_remote_scan_pending);
                    mutable_status->set_scan_in_progress(scan_in_progress);
                    EventManagerPb_AddEvent(ccdiEvent);
                    // ccdiEvent will be freed by EventManagerPb.
                }
            } else {
                if (currState != prevState) {
                    ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
                    ccdiEvent->mutable_sync_feature_status_change()->set_feature(feature);
                    ccdiEvent->mutable_sync_feature_status_change()->mutable_status()->set_status(currState);
                    EventManagerPb_AddEvent(ccdiEvent);
                    // ccdiEvent will be freed by EventManagerPb.
                }
            }
        }
        break;
    case SYNC_CONFIG_EVENT_ENTRY_DOWNLOADED:
        {
            SyncConfigEventEntryDownloaded& event =
                    static_cast<SyncConfigEventEntryDownloaded&>(rawEvent);
            media_metadata::CatalogType_t catalogType;
            switch (feature) {
                case SYNC_FEATURE_PHOTO_METADATA:
                    catalogType = media_metadata::MM_CATALOG_PHOTO;
                    break;
                case SYNC_FEATURE_MUSIC_METADATA:
                    catalogType = media_metadata::MM_CATALOG_MUSIC;
                    break;
                case SYNC_FEATURE_VIDEO_METADATA:
                    catalogType = media_metadata::MM_CATALOG_VIDEO;
                    break;
                case SYNC_FEATURE_SYNCBOX:
                    sendEventSyncHistoryDownload(event);
                    goto after_mca_update;
                case SYNC_FEATURE_PHOTO_THUMBNAILS:
                case SYNC_FEATURE_MUSIC_THUMBNAILS:
                case SYNC_FEATURE_VIDEO_THUMBNAILS:
                case SYNC_FEATURE_PLAYLISTS:
                case SYNC_FEATURE_NOTES:
                case SYNC_FEATURE_PICSTREAM_UPLOAD:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
                case SYNC_FEATURE_PICSTREAM_DELETION:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
                case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
                case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
                default:
                    goto after_mca_update;
            }
            {
                int rc = MCAUpdateMetadataDB(catalogType);
                if(rc != 0){
                    LOG_ERROR("MCAUpdateMetadataDB:%d", rc);
                }
            }
          after_mca_update:
            NO_OP();
        }
        break;
    case SYNC_CONFIG_EVENT_ENTRY_UPLOADED:
        {
            SyncConfigEventEntryUploaded& event =
                    static_cast<SyncConfigEventEntryUploaded&>(rawEvent);
            switch (feature) {
                case SYNC_FEATURE_SYNCBOX:
                    sendEventSyncHistoryUpload(event);
                    break;
                case SYNC_FEATURE_PHOTO_METADATA:
                case SYNC_FEATURE_MUSIC_METADATA:
                case SYNC_FEATURE_VIDEO_METADATA:
                case SYNC_FEATURE_PHOTO_THUMBNAILS:
                case SYNC_FEATURE_MUSIC_THUMBNAILS:
                case SYNC_FEATURE_VIDEO_THUMBNAILS:
                case SYNC_FEATURE_PLAYLISTS:
                case SYNC_FEATURE_NOTES:
                case SYNC_FEATURE_PICSTREAM_UPLOAD:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
                case SYNC_FEATURE_PICSTREAM_DELETION:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
                case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
                case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
                default:
                    break;
            }
        }
        break;
    default:
        break;
    }
}

/// MUST hold s_mutex when calling.
static int privRemoveSyncConfigStep1(size_t index, std::vector<FeatureSyncConfig_t*>& syncConfigsToRemove)
{
    ASSERT(VPLMutex_LockedSelf(VPLLazyInitMutex_GetMutex(&s_mutex)));
    LOG_INFO("Removing user="FMT_VPLUser_Id_t",dset="FMT_DatasetId",feat=%d,scHandle="FMT_SC_HANDLE,
             s_syncConfigs[index]->userId,
             s_syncConfigs[index]->datasetId,
             s_syncConfigs[index]->syncFeature,
             ARG_SC_HANDLE(s_syncConfigs[index]->scHandle));
    int rv = syncConfigMgr->SyncConfig_RequestClose(s_syncConfigs[index]->scHandle);
    if (rv != 0) {
        LOG_ERROR("SyncConfig_RequestClose:%d", rv);
    }
    syncConfigsToRemove.push_back(s_syncConfigs[index]);
    s_syncConfigs.erase(s_syncConfigs.begin()+index);
    return rv;
}

/// DO NOT hold s_mutex when calling.
static void privRemoveSyncConfigStep2(std::vector<FeatureSyncConfig_t*>& syncConfigsToRemove)
{
    ASSERT(!VPLMutex_LockedSelf(VPLLazyInitMutex_GetMutex(&s_mutex)));
    // Wait for all SyncConfig workers to finish.
    for (u32 index = 0; index < syncConfigsToRemove.size(); ++index) {
        SyncConfigManager::SyncConfig_Join(syncConfigsToRemove[index]->scHandle);
        // Any error will be logged within SyncConfig_Join().
    }

    // Free the records.
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    while (syncConfigsToRemove.size() > 0) {
        syncConfigMgr->SyncConfig_Destroy(syncConfigsToRemove[0]->scHandle);
        delete syncConfigsToRemove[0];
        syncConfigsToRemove.erase(syncConfigsToRemove.begin());
    }
}

// Safe for use by multiple threads.
static VCSArchiveAccessCcdMediaMetadataImpl mediaMetadataArchiveAccess;
static VCSFileUrlAccessBasicImpl myFileUrlAccess(&CreateVCSFileUrlOperation_HttpImpl, &CreateVCSFileUrlOperation_TsImpl);

int SyncFeatureMgr_Add(
        u64 user_id,
        u64 dataset_id,
        const VcsCategory& dataset_category,
        ccd::SyncFeature_t sync_feature,
        bool active,
        SyncType type,
        const SyncPolicy& sync_policy,
        const std::string& local_dir,
        const std::string& server_dir,
        bool makeDedicatedThread,
        u64 max_storage,
        u64 max_files,
        const LocalStagingArea* stagingAreaInfo,
        bool allow_create_db)
{
    int rv = 0;
    bool usesMediaMetadataArchiveAccess = false;
    DatasetAccessInfo datasetAccessInfo;

    LOG_INFO("SyncConfigAdd: user="FMT_VPLUser_Id_t",dset="FMT_DatasetId",feat=%d,"
             "type:%d,local_dir:%s,server_dir:%s,"
             "max_storage:"FMTu64",max_files:"FMTu64,
             user_id, dataset_id, sync_feature,
             (int)type, local_dir.c_str(), server_dir.c_str(),
             max_storage, max_files);

    if (stagingAreaInfo) {
        datasetAccessInfo.localStagingArea = *stagingAreaInfo;
    }

    datasetAccessInfo.deviceId = VirtualDevice_GetDeviceId();
    {
        ServiceSessionInfo_t session;
        rv = Cache_GetSessionForVsdsByUser(user_id, session);
        if (rv < 0){
            LOG_ERROR("Cache_GetSessionForVsdsByUser("FMTu64") failed: %d", user_id, rv);
            return rv;
        }
        datasetAccessInfo.sessionHandle = session.sessionHandle;
        datasetAccessInfo.serviceTicket = session.serviceTicket;

        rv = Cache_GetDeviceInfo(user_id, /*out*/ datasetAccessInfo.deviceName);
        if (rv < 0){
            LOG_ERROR("Cache_GetDeviceInfo("FMTu64") failed: %d", user_id, rv);
            return rv;
        }
    }
    {
        GetInfraHttpInfoInput req;
        req.set_user_id(user_id);
        req.set_service(INFRA_HTTP_SERVICE_VCS);
        req.set_secure(true);
        GetInfraHttpInfoOutput resp;
        rv = CCDIGetInfraHttpInfo(req, resp);
        if (rv < 0) {
            LOG_ERROR("CCDIGetInfraHttpInfo("FMTu64") failed: %d", user_id, rv);
            return rv;
        }
        datasetAccessInfo.urlPrefix = resp.url_prefix();
    }
    switch (sync_feature) {
    case SYNC_FEATURE_PHOTO_METADATA:
    case SYNC_FEATURE_PHOTO_THUMBNAILS:
    case SYNC_FEATURE_MUSIC_METADATA:
    case SYNC_FEATURE_MUSIC_THUMBNAILS:
    case SYNC_FEATURE_VIDEO_METADATA:
    case SYNC_FEATURE_VIDEO_THUMBNAILS:
        if (__ccdConfig.mediaMetadataSyncDownloadFromArchiveDevice) {
            usesMediaMetadataArchiveAccess = true;
            datasetAccessInfo.archiveAccess = &mediaMetadataArchiveAccess;
        } else {
            datasetAccessInfo.archiveAccess = NULL;
        }
        break;
    case SYNC_FEATURE_NOTES:
        datasetAccessInfo.archiveAccess = NULL;
        datasetAccessInfo.fileUrlAccess = &myFileUrlAccess;
        break;
    case SYNC_FEATURE_SYNCBOX: 
        datasetAccessInfo.archiveAccess = NULL;
        datasetAccessInfo.fileUrlAccess = &myFileUrlAccess;
        break;
    case SYNC_FEATURE_PLAYLISTS:
        datasetAccessInfo.archiveAccess = NULL;
        datasetAccessInfo.fileUrlAccess = &myFileUrlAccess;
        break;
    case SYNC_FEATURE_PICSTREAM_UPLOAD:
    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
    case SYNC_FEATURE_PICSTREAM_DELETION:
    case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
    case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
    case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
    case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
    case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
    case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
    case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
    case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
    case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
    default:
        datasetAccessInfo.archiveAccess = NULL;
        break;
    };

#ifdef ANDROID
    switch (sync_feature) {
    case SYNC_FEATURE_PHOTO_THUMBNAILS:
    case SYNC_FEATURE_MUSIC_THUMBNAILS:
    case SYNC_FEATURE_VIDEO_THUMBNAILS:
        { // Prevent Android from adding the thumbnails to its Gallery app.
            std::string noMediaFile = local_dir + "/.nomedia";
            VPLFS_stat_t statBuf;
            int rc = VPLFS_Stat(noMediaFile.c_str(), &statBuf);
            if (rc != 0) {
                // File does not exist.
                rc = Util_CreatePath(noMediaFile.c_str(), false);
                if (rc != 0) {
                    LOG_ERROR("Util_CreatePath(%s):%d", noMediaFile.c_str(), rc);
                    // Continue and try creating empty file anyways.
                }
                VPLFile_handle_t nmfHandle;
                nmfHandle = VPLFile_Open(noMediaFile.c_str(),
                                         VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE,
                                         0777);
                if (!VPLFile_IsValidHandle(nmfHandle)) {
                    LOG_ERROR("Unable to create .nomedia file for \"%s\": %d", local_dir.c_str(), nmfHandle);
                } else {
                    LOG_INFO("Created \"%s\"", noMediaFile.c_str());
                    rc = VPLFile_Close(nmfHandle);
                    if (rc != 0) {
                        LOG_ERROR("Failed to close \"%s\": %d", noMediaFile.c_str(), rc);
                    }
                }
            }
        }
        break;
    case SYNC_FEATURE_PHOTO_METADATA:
    case SYNC_FEATURE_MUSIC_METADATA:
    case SYNC_FEATURE_VIDEO_METADATA:
    case SYNC_FEATURE_PLAYLISTS:
    case SYNC_FEATURE_NOTES:
    case SYNC_FEATURE_SYNCBOX:
    case SYNC_FEATURE_PICSTREAM_UPLOAD:
    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
    case SYNC_FEATURE_PICSTREAM_DELETION:
    case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
    case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
    case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
    case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
    case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
    case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
    case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
    case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
    case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
    default:
        break;
    }
#endif

    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    std::vector<FeatureSyncConfig_t*> syncConfigsToRemove;
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));

        // Find if it is already in the list of sync configs.
        {
            size_t index;
            for(index=0; index<s_syncConfigs.size(); ++index)
            {
                if(s_syncConfigs[index]->userId == user_id &&
                   s_syncConfigs[index]->datasetId == dataset_id &&
                   s_syncConfigs[index]->syncFeature == sync_feature)
                {
                    break;
                }
            }

            if(index < s_syncConfigs.size())
            {  // Already exists, now check what changed.
                if(type != s_syncConfigs[index]->type ||
                   local_dir != s_syncConfigs[index]->localDir ||
                   server_dir != s_syncConfigs[index]->serverDir ||
                   max_storage != s_syncConfigs[index]->maxStorage ||
                   max_files != s_syncConfigs[index]->maxFiles)
                {
                    LOG_INFO("Parameters changed for user="FMT_VPLUser_Id_t",dset="FMT_DatasetId
                             ",feat=%d,scHandle="FMT_SC_HANDLE"; remove and recreate.",
                             user_id, dataset_id, sync_feature,
                             ARG_SC_HANDLE(s_syncConfigs[index]->scHandle));
                    privRemoveSyncConfigStep1(index, syncConfigsToRemove);
                } else { // no change
                    LOG_INFO("No change needed for user="FMT_VPLUser_Id_t",dset="FMT_DatasetId
                             ",feat=%d,scHandle="FMT_SC_HANDLE,
                             user_id, dataset_id, sync_feature,
                             ARG_SC_HANDLE(s_syncConfigs[index]->scHandle));
                    return 0;
                }
            }
        }
    } // Releases s_mutex.  We rely on s_addOrRemoveInProgressMutex to prevent another thread
      // from bypassing the "Already exists" check.

    privRemoveSyncConfigStep2(syncConfigsToRemove);

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        int newIndex;
        {
            SyncConfigThreadPool* threadPool = NULL;

            if((type == SYNC_TYPE_ONE_WAY_UPLOAD &&
                    __ccdConfig.uploadThreadPoolSize > 0) ||
               // OneWayDown sharing oneWayUpThreadPool
               (type == SYNC_TYPE_ONE_WAY_DOWNLOAD &&
                    __ccdConfig.uploadThreadPoolSize > 0 &&
                    __ccdConfig.downloadAndUploadUsesUploadThreadPool != 0))
            {   // OneWayUpThreadPool required.
                if (s_oneWayUpThreadPool == NULL) {
                    s_oneWayUpThreadPool = new SyncConfigThreadPool();
                    if (!s_oneWayUpThreadPool->CheckInitSuccess()) {
                        LOG_ERROR("CheckInitSuccess s_oneWayUpThreadPool");
                        delete s_oneWayUpThreadPool;
                        s_oneWayUpThreadPool = NULL;
                    } else {
                        LOG_INFO("Creating OneWayUp ThreadPool(%p) size(%d) shareWithDown(%d)",
                                 s_oneWayUpThreadPool,
                                 __ccdConfig.uploadThreadPoolSize,
                                 __ccdConfig.downloadAndUploadUsesUploadThreadPool);
                        int rc = s_oneWayUpThreadPool->AddGeneralThreads(
                                        __ccdConfig.uploadThreadPoolSize);
                        if(rc != 0) {
                            LOG_ERROR("AddGeneralThreadsUp(%d):%d",
                                      __ccdConfig.uploadThreadPoolSize, rc);
                        }
                    }
                }
                threadPool = s_oneWayUpThreadPool;
            }
            else if (type == SYNC_TYPE_ONE_WAY_DOWNLOAD &&
                    __ccdConfig.downloadAndUploadUsesUploadThreadPool == 0 &&
                    __ccdConfig.downloadThreadPoolSize > 0)
            {   // OneWayDownThreadPool required.
                if (s_oneWayDownThreadPool == NULL) {
                    s_oneWayDownThreadPool = new SyncConfigThreadPool();
                    if (!s_oneWayDownThreadPool->CheckInitSuccess()) {
                        LOG_ERROR("CheckInitSuccess s_oneWayDownThreadPool");
                        delete s_oneWayDownThreadPool;
                        s_oneWayDownThreadPool = NULL;
                    } else {
                        LOG_INFO("Creating OneWayDown ThreadPool(%p) size(%d)",
                                 s_oneWayDownThreadPool, __ccdConfig.downloadThreadPoolSize);
                        int rc = s_oneWayDownThreadPool->AddGeneralThreads(
                                        __ccdConfig.downloadThreadPoolSize);
                        if(rc != 0) {
                            LOG_ERROR("AddGeneralThreadsDown(%d):%d",
                                      __ccdConfig.downloadThreadPoolSize, rc);
                        }
                    }
                }
                threadPool = s_oneWayDownThreadPool;
            }

            VcsDataset dataset(dataset_id, dataset_category);
            SyncConfigHandle scHandle;
            int rc = syncConfigMgr->SyncConfig_Add(&scHandle, user_id, dataset,
                    type, sync_policy, local_dir, server_dir, datasetAccessInfo,
                    threadPool, false,
                    eventCb, (void*)(intptr_t)sync_feature, allow_create_db);
            if (rc != 0) {
                LOG_ERROR("SyncConfig_Add(user="FMT_VPLUser_Id_t",dset="FMT_DatasetId",threadPool=%p,feat=%d,allow_create_db=%s): %d",
                          user_id, dataset_id, threadPool, sync_feature, allow_create_db? "true":"false", rc);
                if (rc == SYNC_AGENT_DB_NOT_EXIST_TO_OPEN) {
                    return rc;
                }
                // TODO: This should not eat the error.  We should actually return this error code whenever
                //   someone calls SyncFeatureMgr_GetStatus for this SyncFeature.
                rv = CCDI_ERROR_FAIL;
                return rv;
            }
            bool shouldReportAvailability = false;
            bool archiveStorageCapable = false;
            bool archiveStorageDeviceIsOnline = false;
            if (usesMediaMetadataArchiveAccess)
            {   // Set initial state for mediaArchiveStorageDevice
                VPLUser_Id_t mediaMetadataUserId;
                u64 mediaServerDeviceId;
                u64 mediaMetadataDatasetId;

                rc = GetMediaServerArchiveStorage(/*OUT*/ mediaMetadataUserId,
                                                  /*OUT*/ mediaServerDeviceId,
                                                  /*OUT*/ mediaMetadataDatasetId,
                                                  /*OUT*/ archiveStorageCapable,
                                                  /*OUT*/ archiveStorageDeviceIsOnline);
                if (rc == 0 &&
                    user_id == mediaMetadataUserId &&
                    dataset_id == mediaMetadataDatasetId)
                {
                    shouldReportAvailability = true; // archive storage device found.
                }
            }

            bool availabilityWasReported = false;
            if (shouldReportAvailability) {
                rc = syncConfigMgr->ReportArchiveStorageDeviceAvailability(
                        scHandle,
                        (archiveStorageDeviceIsOnline && archiveStorageCapable));
                if (rc != 0) {
                    LOG_ERROR("ReportArchiveStorageDeviceAvailability(scHandle="FMT_SC_HANDLE
                              ",user="FMT_VPLUser_Id_t",dset="FMT_DatasetId"):%d. Continuing.",
                              ARG_SC_HANDLE(scHandle), user_id, dataset_id, rc);
                } else {
                    availabilityWasReported = true;
                }
            }

            LOG_INFO("Added: user="FMT_VPLUser_Id_t",dset="FMT_DatasetId
                     ",feat=%d,threadPool=%p,scHandle="FMT_SC_HANDLE",archiveStorageOnline:(%d,%d,%d)",
                     user_id, dataset_id, sync_feature, threadPool,
                     ARG_SC_HANDLE(scHandle), availabilityWasReported,
                     archiveStorageCapable,
                     archiveStorageDeviceIsOnline);

            newIndex = s_syncConfigs.size();
            FeatureSyncConfig_t* syncConfig = new FeatureSyncConfig_t(user_id, dataset_id,
                    sync_feature, scHandle, type, local_dir, server_dir, max_storage, max_files);
            s_syncConfigs.push_back(syncConfig);
        }

        if(active) {
            int rc = syncConfigMgr->SyncConfig_Enable(s_syncConfigs[newIndex]->scHandle);
            if (rc != 0) {
                LOG_ERROR("SyncConfig_Enable:%d", rc);
                rv = rc;
            }
        }
    }
    return rv;
}

void SyncFeatureMgr_ReportDeviceAvailability(u64 deviceId)
{
    // Media Server
    if (__ccdConfig.mediaMetadataSyncDownloadFromArchiveDevice != 0) {
        VPLUser_Id_t userId;
        u64 mediaServerDeviceId;
        u64 datasetId;
        bool isArchiveProtocolVersion;
        bool isOnline;
        int rc = GetMediaServerArchiveStorage(/*OUT*/ userId,
                                              /*OUT*/ mediaServerDeviceId,
                                              /*OUT*/ datasetId,
                                              /*OUT*/ isArchiveProtocolVersion,
                                              /*OUT*/ isOnline);
        if (rc == 0 &&
            deviceId == mediaServerDeviceId &&
            isArchiveProtocolVersion)
        {
            MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
            for(u32 index = 0; index < s_syncConfigs.size(); ++index)
            {
                switch (s_syncConfigs[index]->syncFeature) {
                case SYNC_FEATURE_PHOTO_METADATA:
                case SYNC_FEATURE_PHOTO_THUMBNAILS:
                case SYNC_FEATURE_MUSIC_METADATA:
                case SYNC_FEATURE_MUSIC_THUMBNAILS:
                case SYNC_FEATURE_VIDEO_METADATA:
                case SYNC_FEATURE_VIDEO_THUMBNAILS:
                    if (s_syncConfigs[index]->userId == userId &&
                        s_syncConfigs[index]->datasetId == datasetId)
                    {
                        LOG_INFO("ReportArchiveStorageAvailability("FMT_VPLUser_Id_t
                                  ",feat=%d,dset="FMTu64",device="FMTu64",isOnline=%d).",
                                  userId, s_syncConfigs[index]->syncFeature,
                                  datasetId, deviceId, isOnline);
                        rc = syncConfigMgr->ReportArchiveStorageDeviceAvailability(
                                s_syncConfigs[index]->scHandle,
                                isOnline);
                        if (rc != 0) {
                            LOG_ERROR("ReportArchiveStorageDeviceAvailability:%d", rc);
                        }
                    }
                    break;
                case SYNC_FEATURE_PLAYLISTS:
                case SYNC_FEATURE_NOTES:
                case SYNC_FEATURE_SYNCBOX:
                case SYNC_FEATURE_PICSTREAM_UPLOAD:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
                case SYNC_FEATURE_PICSTREAM_DELETION:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
                case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
                case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
                default:
                    break;
                };
            }
        }
    }
    // Syncbox 
    {
        VPLUser_Id_t userId;
        u64 serverDeviceId;
        u64 datasetId;
        bool isOnline;
        int rc = GetSyncboxArchiveStorage(/*OUT*/ userId,
                                            /*OUT*/ serverDeviceId,
                                            /*OUT*/ datasetId,
                                            /*OUT*/ isOnline);
        if (rc == 0 &&
            deviceId == serverDeviceId)
        {
            MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
            for(u32 index = 0; index < s_syncConfigs.size(); ++index)
            {
                switch (s_syncConfigs[index]->syncFeature) {
                    case SYNC_FEATURE_SYNCBOX:
                        if (s_syncConfigs[index]->userId == userId &&
                            s_syncConfigs[index]->datasetId == datasetId)
                        {
                            LOG_INFO("ReportArchiveStorageAvailability("FMT_VPLUser_Id_t
                                    ",feat=%d,dset="FMTu64",device="FMTu64",isOnline=%d).",
                                    userId, s_syncConfigs[index]->syncFeature,
                                    datasetId, deviceId, isOnline);
                            rc = syncConfigMgr->ReportArchiveStorageDeviceAvailability(
                                    s_syncConfigs[index]->scHandle,
                                    isOnline);
                            if (rc != 0) {
                                LOG_ERROR("ReportArchiveStorageDeviceAvailability:%d", rc);
                            }
                        }
                        break;
                    case SYNC_FEATURE_NOTES:
                    case SYNC_FEATURE_PHOTO_METADATA:
                    case SYNC_FEATURE_PHOTO_THUMBNAILS:
                    case SYNC_FEATURE_MUSIC_METADATA:
                    case SYNC_FEATURE_MUSIC_THUMBNAILS:
                    case SYNC_FEATURE_VIDEO_METADATA:
                    case SYNC_FEATURE_VIDEO_THUMBNAILS:
                    case SYNC_FEATURE_PLAYLISTS:
                    case SYNC_FEATURE_PICSTREAM_UPLOAD:
                    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
                    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
                    case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
                    case SYNC_FEATURE_PICSTREAM_DELETION:
                    case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
                    case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
                    case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
                    case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
                    case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
                    case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
                    case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
                    case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
                    case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
                    default:
                        break;
                } // end switch
            }
        }
    }
}

int SyncFeatureMgr_Remove(
        u64 user_id,
        ccd::SyncFeature_t sync_feature)
{
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
    for(u32 index = 0; index < s_syncConfigs.size(); ++index)
    {
        if(s_syncConfigs[index]->userId == user_id &&
           s_syncConfigs[index]->syncFeature == sync_feature)
        {
            std::vector<FeatureSyncConfig_t*> syncConfigsToRemove;
            int rv = privRemoveSyncConfigStep1(index, syncConfigsToRemove);
            lock2.UnlockNow();
            privRemoveSyncConfigStep2(syncConfigsToRemove);
            return rv;
        }
    }
    LOG_INFO("No record found for user="FMT_VPLUser_Id_t",feat=%d",
             user_id, sync_feature);
    return CCD_ERROR_NOT_FOUND;
}

int SyncFeatureMgr_Remove(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature)
{
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
    for(u32 index = 0; index < s_syncConfigs.size(); ++index)
    {
        if(s_syncConfigs[index]->userId == user_id &&
           s_syncConfigs[index]->datasetId == dataset_id &&
           s_syncConfigs[index]->syncFeature == sync_feature)
        {
            std::vector<FeatureSyncConfig_t*> syncConfigsToRemove;
            int rv = privRemoveSyncConfigStep1(index, syncConfigsToRemove);
            lock2.UnlockNow();
            privRemoveSyncConfigStep2(syncConfigsToRemove);
            return rv;
        }
    }
    LOG_INFO("No record found for user="FMT_VPLUser_Id_t",dset="FMT_DatasetId",feat=%d",
             user_id, dataset_id, sync_feature);
    return CCD_ERROR_NOT_FOUND;
}

int SyncFeatureMgr_SetActive(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature,
        bool active)
{
    LOG_INFO("SyncFeatureMgr_SetActive user="FMT_VPLUser_Id_t",dset="FMT_DatasetId",feat=%d,act:%d",
             user_id, dataset_id, sync_feature, (int)active);
    {
        u32 index;
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (index=0; index<s_syncConfigs.size();++index)
        {
            if (s_syncConfigs[index]->userId == user_id &&
               s_syncConfigs[index]->datasetId == dataset_id &&
               s_syncConfigs[index]->syncFeature == sync_feature)
            {
                if (active) {
                    syncConfigMgr->SyncConfig_Enable(s_syncConfigs[index]->scHandle);
                } else {
                    syncConfigMgr->SyncConfig_Disable(s_syncConfigs[index]->scHandle, false);
                }
                return 0;
            }
        }
    }
    LOG_WARN("Requested SetActive not found("FMTu64","FMTu64",%d)",
             user_id, dataset_id, sync_feature);
    return CCD_ERROR_NOT_FOUND;
}

int SyncFeatureMgr_WaitForDisable(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature)
{
    LOG_INFO("SyncFeatureMgr_WaitForDisable user="FMT_VPLUser_Id_t",dset="FMT_DatasetId",feat=%d",
             user_id, dataset_id, sync_feature);
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    {
        u32 index;
        MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (index=0; index<s_syncConfigs.size();++index)
        {
            if (s_syncConfigs[index]->userId == user_id &&
               s_syncConfigs[index]->datasetId == dataset_id &&
               s_syncConfigs[index]->syncFeature == sync_feature)
            {
                // Must do this without holding s_mutex.
                lock2.UnlockNow();
                syncConfigMgr->SyncConfig_Disable(s_syncConfigs[index]->scHandle, true);
                return 0;
            }
        }
    }
    LOG_WARN("Requested WaitForDisable not found("FMTu64","FMTu64",%d)",
             user_id, dataset_id, sync_feature);
    return CCD_ERROR_NOT_FOUND;
}

int SyncFeatureMgr_GetStatus(
        u64 user_id,
        ccd::SyncFeature_t sync_feature,
        ccd::FeatureSyncStateType_t& state__out,
        u32& uploads_remaining__out,
        u32& downloads_remaining__out,
        bool& remote_scan_pending__out,
        bool& scan_in_progress__out)
{
    {
        u32 index;
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (index = 0; index < s_syncConfigs.size(); ++index)
        {
            if (s_syncConfigs[index]->userId == user_id &&
                s_syncConfigs[index]->syncFeature == sync_feature)
            {
                SyncConfigStatus status;
                bool has_error;
                bool work_to_do;
                int rv = syncConfigMgr->SyncConfig_GetStatus(s_syncConfigs[index]->scHandle,
                        status, has_error, work_to_do, uploads_remaining__out, downloads_remaining__out,
                        remote_scan_pending__out);
                if (rv < 0) {
                    LOG_WARN("SyncConfig_GetStatus failed: %d", rv);
                    state__out = CCD_FEATURE_STATE_OUT_OF_SYNC;
                    return rv;
                }
                state__out = syncConfigStatusToSyncFeatureState(has_error, status);
                scan_in_progress__out = isScanInProgress(status);
                return 0;
            }
        }
    }
    LOG_WARN("Requested sync not found");
    return CCD_ERROR_NOT_FOUND;
}

int SyncFeatureMgr_LookupComponentByPath(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature,
        const std::string& sync_config_relative_path,
        u64& component_id__out,
        u64& revision__out,
        bool& is_on_ans__out)
{
    {
        u32 index;
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (index = 0; index < s_syncConfigs.size(); ++index)
        {
            if (s_syncConfigs[index]->userId == user_id &&
                s_syncConfigs[index]->datasetId == dataset_id &&
                s_syncConfigs[index]->syncFeature == sync_feature)
            {
                int rv = syncConfigMgr->SyncConfig_LookupComponentByPath(
                        s_syncConfigs[index]->scHandle,
                        sync_config_relative_path,
                        component_id__out,
                        revision__out,
                        is_on_ans__out);
                if (rv != 0) {
                    LOG_WARN("SyncConfig_LookupComponentByPath(%s,"FMTu64",%d):%d",
                             sync_config_relative_path.c_str(),
                             dataset_id, (int)sync_feature,
                             rv);
                }
                return rv;
            }
        }
    }
    LOG_WARN("Requested sync not found");
    return CCD_ERROR_NOT_FOUND;
}

int SyncFeatureMgr_GetSyncStateForPath(
    u64 user_id,
    const std::string& abs_path,
    SyncConfigStateType_t& state__out,
    u64& dataset_id__out,
    SyncFeature_t& sync_feature__out,
    bool& is_sync_folder_root__out)
{
    state__out = SYNC_CONFIG_STATE_NOT_IN_SYNC_FOLDER;
    {
        u32 index;
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (index = 0; index < s_syncConfigs.size(); ++index)
        {
            if (s_syncConfigs[index]->userId == user_id) {
                VPL_BOOL match = false;
                ccd::SyncFeature_t syncFeature;
                switch (s_syncConfigs[index]->syncFeature) {
                case SYNC_FEATURE_SYNCBOX:
                    match = true;
                    syncFeature = SYNC_FEATURE_SYNCBOX;
                    break;
                case SYNC_FEATURE_NOTES:
                case SYNC_FEATURE_PLAYLISTS:
                case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
                case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
                case SYNC_FEATURE_PHOTO_METADATA:
                case SYNC_FEATURE_PHOTO_THUMBNAILS:
                case SYNC_FEATURE_MUSIC_METADATA:
                case SYNC_FEATURE_MUSIC_THUMBNAILS:
                case SYNC_FEATURE_VIDEO_METADATA:
                case SYNC_FEATURE_VIDEO_THUMBNAILS:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
                case SYNC_FEATURE_PICSTREAM_UPLOAD:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
                case SYNC_FEATURE_PICSTREAM_DELETION:
                case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
                default:
                    break;
                }
                if (match) {
                    sync_feature__out = syncFeature;
                    int rv = syncConfigMgr->SyncConfig_GetSyncStateForPath(
                            s_syncConfigs[index]->scHandle,
                            abs_path,
                            state__out,
                            dataset_id__out,
                            is_sync_folder_root__out);
                    if (rv < 0) {
                        LOG_WARN("SyncConfig_GetSyncStateForPath(%s,%d):%d",
                                 abs_path.c_str(), (int)s_syncConfigs[index]->syncFeature, rv);
                    }
                    return rv;
                }
            }
        }
    }

    return 0;
}

int SyncFeatureMgr_LookupAbsPath(
        u64 user_id,
        u64 dataset_id,
        u64 component_id,
        u64 revision,
        const std::string& dataset_rel_path,
        std::string& absolute_path__out,
        u64& local_modify_time__out,
        std::string& hash__out)
{
    {
        u32 index;
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (index = 0; index < s_syncConfigs.size(); ++index)
        {
            if (s_syncConfigs[index]->userId == user_id &&
                s_syncConfigs[index]->datasetId == dataset_id)
            {
                // Calling this function is only meaningful for upload SyncConfigs.
                VPL_BOOL match;
                // For media metadata, we need to use dataset_rel_path to dispatch to the
                // correct syncFeature (since they all share the same dataset).
                switch (s_syncConfigs[index]->syncFeature) {
                case SYNC_FEATURE_SYNCBOX:
                case SYNC_FEATURE_NOTES:
                    match = true;
                    break;
                case SYNC_FEATURE_PLAYLISTS:
                    match = VPLString_StartsWith(dataset_rel_path.c_str(), "playlists/");
                    break;
                case SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD:
                    match = VPLString_StartsWith(dataset_rel_path.c_str(), "ph/i/");
                    break;
                case SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD:
                    match = VPLString_StartsWith(dataset_rel_path.c_str(), "ph/t/");
                    break;
                case SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD:
                    match = VPLString_StartsWith(dataset_rel_path.c_str(), "mu/i/");
                    break;
                case SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD:
                    match = VPLString_StartsWith(dataset_rel_path.c_str(), "mu/t/");
                    break;
                case SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD:
                    match = VPLString_StartsWith(dataset_rel_path.c_str(), "vi/i/");
                    break;
                case SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD:
                    match = VPLString_StartsWith(dataset_rel_path.c_str(), "vi/t/");
                    break;
                case SYNC_FEATURE_PHOTO_METADATA:
                case SYNC_FEATURE_PHOTO_THUMBNAILS:
                case SYNC_FEATURE_MUSIC_METADATA:
                case SYNC_FEATURE_MUSIC_THUMBNAILS:
                case SYNC_FEATURE_VIDEO_METADATA:
                case SYNC_FEATURE_VIDEO_THUMBNAILS:
                case SYNC_FEATURE_PICSTREAM_UPLOAD:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES:
                case SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL:
                case SYNC_FEATURE_PICSTREAM_DELETION:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME:
                case SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME:
                case SYNC_FEATURE_MEDIA_METADATA_UPLOAD:
                default:
                    match = false;
                    break;
                }
                if (match) {
                    int rv = syncConfigMgr->SyncConfig_LookupAbsPath(
                            s_syncConfigs[index]->scHandle,
                            component_id,
                            revision,
                            absolute_path__out,
                            local_modify_time__out,
                            hash__out);
                    if (rv < 0) {
                        LOG_WARN("SyncConfig_LookupAbsPath("FMTu64",%d,"FMTu64","FMTu64"):%d",
                                 dataset_id, (int)s_syncConfigs[index]->syncFeature,
                                 component_id, revision, rv);
                    }
                    return rv;
                }
            }
        }
    }
    LOG_WARN("Requested sync not found for "FMTu64","FMTu64",%s",
            user_id, dataset_id, dataset_rel_path.c_str());
    return CCD_ERROR_NOT_FOUND;
}

int SyncFeatureMgr_RemoveByUser(u64 user_id)
{
    int rv = 0;
    bool itemRemoved = false;
    std::vector<FeatureSyncConfig_t*> syncConfigsToRemove;
    LOG_INFO("SyncConfig_RemoveByUser user="FMT_VPLUser_Id_t, user_id);
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    {
        MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (size_t index = 0; index < s_syncConfigs.size(); ++index)
        {
            if (s_syncConfigs[index]->userId == user_id)
            {
                itemRemoved = true;
                int rc = privRemoveSyncConfigStep1(index, syncConfigsToRemove);
                if (rc != 0) {
                    // Error already logged.
                    rv = rc;
                }
                index--; // Remain at the current index, forloop will increment
            }
        }
    } // Releases s_mutex.

    if (syncConfigsToRemove.size() == 0) {
        LOG_WARN("No syncConfigs removed for user="FMT_VPLUser_Id_t, user_id);
    } else {
        privRemoveSyncConfigStep2(syncConfigsToRemove);
    }

    return rv;
}

int SyncFeatureMgr_RemoveByDatasetId(
        u64 user_id,
        u64 dataset_id)
{
    int rv = 0;
    std::vector<FeatureSyncConfig_t*> syncConfigsToRemove;
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    {
        MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
        for(u32 index = 0; index < s_syncConfigs.size(); ++index)
        {
            if(s_syncConfigs[index]->userId == user_id &&
               s_syncConfigs[index]->datasetId == dataset_id)
            {
                LOG_INFO("Removing syncConfig "FMT_VPLUser_Id_t",dset="FMT_DatasetId",feat=%d",
                             user_id, dataset_id, s_syncConfigs[index]->syncFeature);
                int rc = privRemoveSyncConfigStep1(index, syncConfigsToRemove);
                if (rc != 0) {
                    // Error already logged.
                    rv = rc;
                }
                index--; // Remain at the current index, forloop will increment
            }
        }
    } // Releases s_mutex.

    privRemoveSyncConfigStep2(syncConfigsToRemove);

    return rv;
}

int SyncFeatureMgr_RemoveAll()
{
    int rv = 0;
    LOG_INFO("Removing all sync configs.");
    std::vector<FeatureSyncConfig_t*> syncConfigsToRemove;
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    {
        MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
        while (s_syncConfigs.size() > 0) {
            int rc = privRemoveSyncConfigStep1(0, syncConfigsToRemove);
            if (rc != 0) {
                // Error already logged.
                rv = rc;
            }
        }
    } // Releases s_mutex.

    privRemoveSyncConfigStep2(syncConfigsToRemove);

    return rv;
}

static inline
bool isDatasetIdInList(u64 datasetId, const std::vector<u64>& datasetList)
{
    return std::find(datasetList.begin(), datasetList.end(), datasetId) != datasetList.end();
}

/// Remove any of the user's SyncConfigs that are *not* listed in \a currentDatasetIds.
int SyncFeatureMgr_RemoveDeletedDatasets(
        u64 user_id,
        const std::vector<u64>& currentDatasetIds)
{
    int rv = 0;
    std::vector<FeatureSyncConfig_t*> syncConfigsToRemove;
    MutexAutoLock lock1(VPLLazyInitMutex_GetMutex(&s_addOrRemoveInProgressMutex));
    {
        MutexAutoLock lock2(VPLLazyInitMutex_GetMutex(&s_mutex));
        for (u32 index = 0; index < s_syncConfigs.size(); ++index)
        {
            if (s_syncConfigs[index]->userId == user_id &&
                !isDatasetIdInList(s_syncConfigs[index]->datasetId, currentDatasetIds))
            {
                LOG_INFO("Removing syncConfig for deleted dataset: "FMT_VPLUser_Id_t",dset="FMT_DatasetId",feat=%d",
                             user_id, s_syncConfigs[index]->datasetId, s_syncConfigs[index]->syncFeature);
                int rc = privRemoveSyncConfigStep1(index, syncConfigsToRemove);
                if (rc != 0) {
                    // Error already logged.
                    rv = rc;
                }
                index--; // Remain at the current index, forloop will increment
            }
        }
    } // Releases s_mutex.

    privRemoveSyncConfigStep2(syncConfigsToRemove);

    return rv;
}

void SyncFeatureMgr_RequestRemoteScansForUser(u64 userId)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    for (size_t index = 0; index < s_syncConfigs.size(); ++index) {
        if (s_syncConfigs[index]->userId == userId) {
            syncConfigMgr->ReportPossibleRemoteChange(s_syncConfigs[index]->scHandle);
        }
    }
}

void SyncFeatureMgr_RequestLocalScansForUser(u64 userId)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    for (size_t index = 0; index < s_syncConfigs.size(); ++index) {
        if (s_syncConfigs[index]->userId == userId) {
            syncConfigMgr->ReportLocalChange(s_syncConfigs[index]->scHandle, "");
        }
    }
}

void SyncFeatureMgr_RequestAllRemoteScans()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    for (size_t index = 0; index < s_syncConfigs.size(); ++index) {
        syncConfigMgr->ReportPossibleRemoteChange(s_syncConfigs[index]->scHandle);
    }
}

void SyncFeatureMgr_ReportRemoteChange(
        const vplex::syncagent::notifier::DatasetContentUpdate& notification)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    syncConfigMgr->ReportRemoteChange(notification);
}

int SyncFeatureMgr_BlockUntilSyncDone()
{
    // TODO: impl if needed
    return CCD_ERROR_NOT_IMPLEMENTED;
}
