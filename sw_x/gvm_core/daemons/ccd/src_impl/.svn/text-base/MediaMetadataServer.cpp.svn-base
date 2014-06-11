//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

//============================================================================
/// @file
/// Implementation of MediaMetadataServer.hpp
//============================================================================

#include "MediaMetadataServer.hpp"
#include "MsaDb.hpp"

#include "ccd_features.h"
#include "ccd_storage.hpp"
#include "ccdi.hpp"
#include "gvm_file_utils.hpp"
#include "gvm_rm_utils.hpp"
#include "log.h"
#include "media_metadata_utils.hpp"
#include "MediaMetadataCache.hpp"
#include "protobuf_file_reader.hpp"
#include "SyncConfig.hpp"
#include "vpl_fs.h"
#include "vpl_lazy_init.h"
#include "vpl_th.h"
#include "vplex_file.h"
#include "vplu_mutex_autolock.hpp"
#include "gvm_misc_utils.h"
#include "scopeguard.hpp"

#include <set>

#if !CCD_ENABLE_MEDIA_SERVER_AGENT
# error "This platform/configuration does not support MSA; this source file should not be included in the build."
#endif

using namespace media_metadata;

static VPLLazyInitMutex_t s_msaMutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitMutex_t s_msaUpdateDBMutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitMutex_t s_cancelMutex = VPLLAZYINITMUTEX_INIT;
static VPLDetachableThreadHandle_t updateDBThread;
static bool s_isInit = false;
static bool s_isUpdateDBThread = false;

static std::string s_metadataSubscriptionPath;
static std::string s_tempPath;
static const char* MSA_TEMP_DELETE = "msatempdel";
static const char* MSA_STAGING_DIR = "msastage";
static const char* MSA_REPLAY_DIR = "msareplay";
static VPLUser_Id_t s_userId = VPLUSER_ID_NONE;

/// The local deviceId.
static u64 s_serverDeviceId = 0;

static bool s_inTransaction = false;
static ccd::BeginMetadataTransactionInput s_transactionArgs;
static bool s_inCatalog = false;
static media_metadata::CatalogType_t s_catType;
static MsaControlDB msaControlDB;
static bool s_cancel = false;

static std::map<int, u64> s_media_datasets;

#define ITUNES_DATASET_NAME     "MEDIA_ITUNES"
#define WMP_DATASET_NAME        "MEDIA_WMP"
#define GENERICLIB_DATASET_NAME "MEDIA_LIBRARY"
#define THUMBNAIL_CHUNK_TEMP_BUFFER   (16*1024)

//forward declaration
static MMError updatePhotoAblumRelationDBVersion4();
typedef struct {
    VPLUser_Id_t* userIdPtr;
    bool dbUpgradedFromV3ToV4;
} threadData;

static void SetCancel(bool cancel)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_cancelMutex));
    s_cancel = cancel;
}

// returns true if cancel has been set
static bool CheckCancel()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_cancelMutex));
    return s_cancel;
}

static char* s_thumbnail_temp_buffer = NULL;
const static u32 MAX_COLLECTION_ID_SIZE = 64;

static MediaMetadataCache s_cache;

static MMError copyThumbnailToTemp(const ContentDirectoryObject& metadata,
                                   u64 cloudDeviceId,
                                   const std::string& collectionId,
                                   u64 timestamp);
static MMError commitReplayLog();
static int cleanupReplayLog();

static MMError readCollection(const std::string collection_path, 
                              google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject>& cdos);

static std::string getTempDeleteDir(const std::string& tempDir) {
    return tempDir+"/"+MSA_TEMP_DELETE;
}

static std::string getMsaStagingDir(const std::string& tempDir) {
    return tempDir+"/"+MSA_STAGING_DIR;
}

static std::string getMsaReplayDir(const std::string& tempDir) {
    return tempDir+"/"+MSA_REPLAY_DIR;
}

static MMError stopUpdate();

static MMError updateMetadataDB(media_metadata::CatalogType_t catalogType)
{
    int rc = 0;
    int rv = 0;
    media_metadata::ListCollectionsOutput collections;
    std::string metadataCatDir;

    s_cache.formMetadataCatalogDir(s_metadataSubscriptionPath,
                                   catalogType,
                                   METADATA_FORMAT_INDEX,
                                   s_serverDeviceId,
                                   metadataCatDir);
    
    rv = s_cache.listCollections(s_metadataSubscriptionPath,
                                 catalogType,
                                 s_serverDeviceId,
                                 collections);
    if(rv != 0) {
        LOG_ERROR("list collections %d: %s,deviceId:"FMTx64,
                  rv, s_metadataSubscriptionPath.c_str(), s_serverDeviceId);
        goto out;
    }

    // Bug 10504: delete collection (including contents belongs to it)
    rc = msaControlDB.beginTransaction();
    if(rc != 0) {
        LOG_ERROR("beginTransaction to update server db: %d", rc);
        rv = rc;
        goto out;
    }
    for (int i=0;
         i < collections.collection_id_size() && rv == 0;
         i++)
    {
        //check if building DB thread is canceled and return MSA_CONTROLDB_DB_REBUILD_CANCELED
        if (CheckCancel()) {
            LOG_WARN("updateMetadataDB canceled");
            rv = MSA_CONTROLDB_DB_BUILD_CANCELED;
            goto canceled;
        }

        rc = msaControlDB.deleteCollection(collections.collection_id(i));
        if(rc != MSA_CONTROLDB_OK && rc != MSA_CONTROLDB_DB_NOT_PRESENT) {
            LOG_ERROR("deleteCollection: %s from DB %d",
                      collections.collection_id(i).c_str(), rc);
            rv = rc;
            break;
        }

        // Bug 10504: update collections (including contents belongs to it)
        rc = msaControlDB.updateCollection(collections.collection_id(i), catalogType);
        if (rc != MSA_CONTROLDB_OK) {
            LOG_ERROR("updateCollection: %s from DB %d",
                      collections.collection_id(i).c_str(), rc);
            rv = rc;
            break;
        }
        {
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject> cdos;
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject>::iterator it;
            std::string collFilename;
            std::string finalDataFile;
            s_cache.formMetadataCollectionFilename(collections.collection_id(i),
                                                   collections.collection_timestamp(i),
                                                   collFilename);
            finalDataFile = metadataCatDir + "/" + collFilename;
            rc = readCollection(finalDataFile, cdos);
            if(rc != 0) {
                LOG_ERROR("readCollection: %s from DB %d",
                          collections.collection_id(i).c_str(), rc);
                rv = rc;
                break;
            }

            for (it = cdos.begin(); it != cdos.end(); it++) {
                rc = msaControlDB.updateContentObject(collections.collection_id(i), *it);
                if(rc != 0) {
                    LOG_ERROR("updateContentObject: %s in collection:%s from DB, %d",
                              it->object_id().c_str(), collections.collection_id(i).c_str(), rc);
                    rv = rc;
                    break;
                }
            }
        }
    }
    rc = msaControlDB.commitTransaction();
    if(rc != 0) {
        LOG_ERROR("commitTransaction to update collections: %s from DB %d",
                  s_transactionArgs.collection_id().c_str(), rc);
        if (rv != MSA_CONTROLDB_DB_BUILD_CANCELED) {
            rv = rc;
        }
    }

out:
    return rv;

canceled:
    msaControlDB.rollbackTransaction();
    return rv;
}

static VPLTHREAD_FN_DECL callMSAUpdateThreadFn(void* arg)
{

    VPLUser_Id_t user_id;
    bool dbUpgradedFromV3toV4 = false;
    {
        threadData* thdData = (threadData*)arg;
        user_id = *(thdData->userIdPtr);
        dbUpgradedFromV3toV4 = thdData->dbUpgradedFromV3ToV4;
        delete thdData->userIdPtr;  // Allocated by the caller.
        delete thdData;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));
    int rv = 0;

    if (dbUpgradedFromV3toV4) {
        rv = updatePhotoAblumRelationDBVersion4();
        LOG_INFO("updatePhotoAblumRelationDBVersion4 DONE!");
        goto end;
    }
    for (int i = media_metadata::CatalogType_t_MIN;
         i <= media_metadata::CatalogType_t_MAX;
         i++)
    {
        // Using a switch statement to have the compiler warn us if a new
        // CatalogType_t is ever added.
        switch (i) {
        case media_metadata::MM_CATALOG_MUSIC:
        case media_metadata::MM_CATALOG_PHOTO:
        case media_metadata::MM_CATALOG_VIDEO:
            rv = updateMetadataDB((media_metadata::CatalogType_t)i);
            if (rv != MSA_CONTROLDB_OK) {
                if (rv == MSA_CONTROLDB_DB_BUILD_CANCELED) {
                    // MSA_CONTROLDB_DB_BUILD_CANCELED stands for DB building process canceled
                    LOG_WARN("Rebuild MSA DB with catalog %d interrupted.", i);
                    // stop update thread
                    i = media_metadata::CatalogType_t_MAX;
                }
                else
                    LOG_ERROR("Update MSA DB with catalog %d, error: %d", i, rv);
            }
            break;
        }
    }
end:
    // Currently, do not set DB updated if DB building process being interrupted
    // Do we build DB for any other errors?
    if (rv != MSA_CONTROLDB_DB_BUILD_CANCELED) {
        rv = msaControlDB.setDbUpdated(user_id,
                                       MSA_CONTROL_DB_UPDATED);
        if (rv != MSA_CONTROLDB_OK) {
            LOG_ERROR("Cannot set server db updated value to "FMTu64", err:%d",
                      MSA_CONTROL_DB_UPDATED, rv);
        }
        rv = msaControlDB.insertDbVersion(user_id,
                                       MSA_CONTROL_DB_SCHEMA_VERSION);
        if (rv != MSA_CONTROLDB_OK) {
            LOG_ERROR("Cannot set server db versione to "FMTu64", err:%d",
                      MSA_CONTROL_DB_SCHEMA_VERSION, rv);
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));
    return VPLTHREAD_RETURN_VALUE;
}

MMError MSAInitForDiscovery(const u64 user_id, const u64 device_id)
{
    int rv = 0;
    int rc;
    ccd::ListLinkedDevicesOutput lldResponse;
    s_isInit = false;
    u64 localDeviceId = 0;

    LOG_INFO("Try to init MSA component...");
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    { 
        s_userId = user_id;
        if (s_userId == 0) {
            LOG_ERROR("Not signed-in!");
            rv = MM_ERR_NOT_SIGNED_IN;
            goto out;
        }

        localDeviceId = device_id;
        if (localDeviceId == 0) {
            LOG_ERROR("Bad device_id!");
            rv = MM_ERR_FAIL;
            goto out;
        }

        if (s_serverDeviceId != localDeviceId) {
            s_serverDeviceId = localDeviceId;
        }
        {
            char mediaMetaUpDir[CCD_PATH_MAX_LENGTH];
            DiskCache::getDirectoryForMediaMetadataUpload(s_userId, ARRAY_SIZE_IN_BYTES(mediaMetaUpDir), mediaMetaUpDir);
            s_metadataSubscriptionPath = mediaMetaUpDir;
            LOG_INFO("Metadata path set to \"%s\"", mediaMetaUpDir);
        }
        {
            // create/open msa db
            bool dbNeedsToBePopulated = false;
            bool dbNeedsToBeUpgradedFromV3ToV4 = false;
            char msaDbDir[CCD_PATH_MAX_LENGTH];
            DiskCache::getDirectoryForMcaDb(s_userId, ARRAY_SIZE_IN_BYTES(msaDbDir), msaDbDir);
            std::string appDataDirectory = msaDbDir;
            LOG_INFO("MSA DB path set to \"%s\"", msaDbDir);

            // interrupt prev update db thread (if exists)
            stopUpdate();

            rv = createOrOpenMediaServerDB(msaControlDB, appDataDirectory, s_userId,
                                           dbNeedsToBePopulated, dbNeedsToBeUpgradedFromV3ToV4);
            if(rv != MSA_CONTROLDB_OK) {
                LOG_ERROR("Opening server db: %s, user:"FMTx64,
                          appDataDirectory.c_str(), s_userId);
                goto out;
            }
            while (dbNeedsToBePopulated || dbNeedsToBeUpgradedFromV3ToV4) {
                // Updating MSA DB could be time consuming
                // Create thread to avoid blocking CCDStart
                //VPLUser_Id_t* userIdArg = new (std::nothrow) VPLUser_Id_t;  // userIdArg Ownership passed to spawned thread;
                threadData* thdData = new (std::nothrow) threadData;
                if (!thdData) {
                    LOG_ERROR("threadData OutOfMemory");
                    rv = CCD_ERROR_NOMEM;
                    break;
                }

                thdData->dbUpgradedFromV3ToV4 = dbNeedsToBeUpgradedFromV3ToV4;
                thdData->userIdPtr =  new (std::nothrow) VPLUser_Id_t;  // userIdArg Ownership passed to spawned thread;

                if (thdData->userIdPtr) {
                    *(thdData->userIdPtr) = s_userId;
                    int rc = Util_SpawnThread(callMSAUpdateThreadFn,
                                              (void*)thdData,
                                              UTIL_DEFAULT_THREAD_STACK_SIZE,
                                              VPL_TRUE,
                                              &updateDBThread);
                    if(rc != 0){
                        LOG_ERROR("Util_SpawnThread:%d", rc);
                        rv = rc;
                        delete thdData->userIdPtr;
                        delete thdData;
                    }
                    else {
                        s_isUpdateDBThread = true;
                    }
                } else {
                    LOG_ERROR("userIdArg OutOfMemory");
                    rv = CCD_ERROR_NOMEM;
                }
                break;
            }
        }
        char tempDir[CCD_PATH_MAX_LENGTH];
        DiskCache::getDirectoryForMediaMetadataUploadTemp(s_userId,
                                                          sizeof(tempDir),
                                                          tempDir);
        s_tempPath = Util_CleanupPath(tempDir);
        s_isInit = true;
    }

    s_inCatalog = false;

    {   // In case the app crashed while commit started, but has not ended.
        ccd::PrivateMsaDataCommitInput requestStart;
        requestStart.set_user_id(s_userId);
        requestStart.set_initialize(true);
        requestStart.set_commit_end(true);
        rc = CCDIPrivateMsaDataCommit(requestStart);
        if(rc != 0) {
            LOG_ERROR("CCDIPrivateMsaDataCommit init failed:%d, userId:"FMTx64,
                      rc, s_userId);
            if(rv==0) {rv = rc;}
            goto out;
        }
    }

 out:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    LOG_INFO("Init MSA component end: %d", rv);
    return rv;
}

static MMError stopUpdate()
{
    int rv = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    LOG_INFO("stop MSA updateDbThread, if any");
    if (s_isUpdateDBThread) {
        // set cancel flag
        SetCancel(true);
        LOG_INFO("Canceling updateDbThread, if any");
        // wait for DB building thread to exit
        rv = VPLDetachableThread_Join(&updateDBThread);
        if (rv != 0) {
            LOG_ERROR("Failed to join DB building thread: err %d", rv);
        }
        LOG_INFO("Canceling updateDbThread done");
        // reset flags
        SetCancel(false);
        s_isUpdateDBThread = false;
    }

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

// TODO: Need a in-catalog-transaction flag, but cannot have it now due for
// legacy support.  (In the new way, MSABeginCatalog should be called before
// BeginMetadataTransaction, but for legacy support, MSABeginCatalog won't
// even be called)
MMError MSABeginCatalog(const ccd::BeginCatalogInput& input)
{
    int rv = 0;
    int rc;
    if(!input.has_catalog_type()) {
        LOG_ERROR("Require catalog type.");
        rv = MM_ERR_BAD_PARAMS;
        return rv;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    if(!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }
    if(s_inCatalog) {
        rv = MM_ERR_IN_CATALOG;
        goto error;
    }

    s_catType = input.catalog_type();
    LOG_INFO("MSABeginCatalog of type:%d",(int)s_catType);

    // First best effort attempt without stopping sync Agent
    rc = commitReplayLog();
    if(rc == VPL_OK) {
        s_inCatalog = true;
        goto done;
    }

    {
        LOG_INFO("Begin catalog wait for sync stop");
        ccd::PrivateMsaDataCommitInput requestStart;
        requestStart.set_user_id(s_userId);
        requestStart.set_commit_start(true);
        rc = CCDIPrivateMsaDataCommit(requestStart);
        if(rc != 0) {
            LOG_ERROR("CCDIPrivateMsaDataCommit start failed:%d, userId:"FMTx64,
                      rc, s_userId);
            rv = rc;
            goto error;
        }
    }

    rc = commitReplayLog();
    if(rc != 0) {
        LOG_ERROR("Committing replay log:%d", rc);
        rv = rc;
        goto error_commit;
    }
    s_inCatalog = true;
 error_commit:
    {
        ccd::PrivateMsaDataCommitInput requestEnd;
        requestEnd.set_user_id(s_userId);
        requestEnd.set_commit_end(true);
        rc = CCDIPrivateMsaDataCommit(requestEnd);
        if(rc != 0) {
            LOG_ERROR("CCDIPrivateMsaDataCommit end failed:%d, userId:"FMTx64,
                      rc, s_userId);
        }
    }
 error:
 done:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

MMError MSACommitCatalog(const ccd::CommitCatalogInput& input)
{
    int rv = 0;
    int rc;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    if(!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }
    if(!s_inCatalog) {
        LOG_ERROR("MSABeginCatalog never succeeded.");
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }

    if(!input.has_catalog_type()) {
        LOG_ERROR("Missing catalog type");
        rv = MM_ERR_BAD_PARAMS;
        goto error;
    }

    if(s_catType != input.catalog_type()) {
        LOG_ERROR("Previously opened catalog type %d not matched by close %d",
                  (int)s_catType, (int)input.catalog_type());
        rv = MM_ERR_BAD_PARAMS;
        goto error;
    }

    {
        LOG_INFO("Commit catalog wait for sync stop");
        // Taking drastic measure to stop and start sync agent
        ccd::PrivateMsaDataCommitInput requestStart;
        requestStart.set_user_id(s_userId);
        requestStart.set_commit_start(true);
        rc = CCDIPrivateMsaDataCommit(requestStart);
        if(rc != 0) {
            LOG_ERROR("CCDIPrivateMsaDataCommit start failed:%d, userId:"FMTx64,
                      rc, s_userId);
            rv = rc;
            goto error;
        }

        rc = commitReplayLog();
        if(rc != 0) {
            LOG_ERROR("Committing replay log:%d", rc);
            rv = rc;
        }

        ccd::PrivateMsaDataCommitInput requestEnd;
        requestEnd.set_user_id(s_userId);
        requestEnd.set_commit_end(true);
        rc = CCDIPrivateMsaDataCommit(requestEnd);
        if(rc != 0) {
            LOG_ERROR("CCDIPrivateMsaDataCommit end failed:%d, userId:"FMTx64,
                      rc, s_userId);
        }
    }

    s_inCatalog = false;
 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

MMError MSAEndCatalog(const ccd::EndCatalogInput& input)
{
    int rv = 0;
    int rc;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    if(!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }
    if(!s_inCatalog) {
        LOG_ERROR("MSABeginCatalog never succeeded.");
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }

    if(!input.has_catalog_type()) {
        LOG_ERROR("Missing catalog type");
        rv = MM_ERR_BAD_PARAMS;
        goto error;
    }

    if(s_catType != input.catalog_type()) {
        LOG_ERROR("Previously opened catalog type %d not matched by close %d",
                  (int)s_catType, (int)input.catalog_type());
        rv = MM_ERR_BAD_PARAMS;
        goto error;
    }

    rc = cleanupReplayLog();
    if(rc != 0) {
        LOG_ERROR("cleanupReplayLog:%d", rc);
        rv = rc;
    }
    s_inCatalog = false;

 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;

}

MMError MSADestroy()
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    stopUpdate();

    if(s_thumbnail_temp_buffer) {
        free(s_thumbnail_temp_buffer);
        s_thumbnail_temp_buffer = NULL;
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return 0;
}

static bool isCollectionInReplayLog(const std::string& myCollectionId,
                                    media_metadata::CatalogType_t catType)
{
    std::string replayLogDir;
    MediaMetadataCache::formMetadataCatalogDir(getMsaReplayDir(s_tempPath),
                                               catType,
                                               METADATA_FORMAT_REPLAY,
                                               s_serverDeviceId,
                                               replayLogDir);
    VPLFS_dir_t dirHandleReplay;
    int rc = VPLFS_Opendir(replayLogDir.c_str(), &dirHandleReplay);
    if(rc == VPL_ERR_NOENT){
        // No replay log, collection is not in replay log.
        return false;
    }else if(rc != VPL_OK) {
        LOG_ERROR("Opening replay directory:%s, %d",
                  replayLogDir.c_str(), rc);
        return false;
    }

    bool toReturn = false;
    VPLFS_dirent_t replayDirent;
    while((rc = VPLFS_Readdir(&dirHandleReplay, &replayDirent))==VPL_OK) {
        if(replayDirent.type != VPLFS_TYPE_FILE) {
            continue;
        }
        std::string collectionId;
        bool deleted;
        u64 timestamp;
        rc = MediaMetadataCache::parseReplayFilename(replayDirent.filename,
                                                     collectionId,
                                                     deleted,
                                                     timestamp);
        if(myCollectionId == collectionId) {
            toReturn = true;
            break;
        }
    }

    rc = VPLFS_Closedir(&dirHandleReplay);
    if(rc != VPL_OK) {
        LOG_ERROR("Closing replay directory:%s, %d",
                  replayLogDir.c_str(), rc);
    }
    return toReturn;
}

static bool isCollectionIdInOtherCatalog(const std::string& collectionId,
                                         media_metadata::CatalogType_t catType,
                                         int catType_out)
{
    int catIndex;
    media_metadata::CatalogType_t catTypeIter;
    for(catIndex = 1; catIndex < 4; catIndex++)
    {
        if(catType == (media_metadata::CatalogType_t)catIndex) {
            continue;
        }

        switch((media_metadata::CatalogType_t)catIndex) {
        case MM_CATALOG_MUSIC:
            catTypeIter = MM_CATALOG_MUSIC;
            break;
        case MM_CATALOG_PHOTO:
            catTypeIter = MM_CATALOG_PHOTO;
            break;
        case MM_CATALOG_VIDEO:
            catTypeIter = MM_CATALOG_VIDEO;
            break;
        default:
            LOG_ERROR("Should never reach here:%d", (int)catIndex);
            return false;
        }
        std::string colIdDir_out;
        MediaMetadataCache::formMetadataCatalogDir(s_metadataSubscriptionPath,
                                                   catTypeIter,
                                                   METADATA_FORMAT_INDEX,
                                                   s_serverDeviceId,
                                                   colIdDir_out);
        // See http://intwww.routefree.com/wiki/index.php/MSA#Implementation_Details
        // for path specification.
        int rc;
        VPLFS_dir_t dirHandleReplay;
        rc = VPLFS_Opendir(colIdDir_out.c_str(), &dirHandleReplay);
        if(rc == VPL_ERR_NOENT){
            // No replay log, collection is not in replay log.
            continue;
        }else if(rc != VPL_OK) {
            LOG_ERROR("Opening collectionIndex directory:%s, %d",
                      colIdDir_out.c_str(), rc);
            continue;
        }

        VPLFS_dirent_t replayDirent;
        while((rc = VPLFS_Readdir(&dirHandleReplay, &replayDirent))==VPL_OK) {
            if(replayDirent.type != VPLFS_TYPE_FILE) {
                continue;
            }
            std::string parsedColId;
            u64 timestamp;
            rc = MediaMetadataCache::parseMetadataCollectionFilename(replayDirent.filename,
                                                                     parsedColId,
                                                                     timestamp);
            if(parsedColId == collectionId) {
                LOG_INFO("Found collection %s in catalog %d",
                         collectionId.c_str(), (int)catTypeIter);
                return true;
            }
        }

        rc = VPLFS_Closedir(&dirHandleReplay);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing replay directory:%s, %d",
                      colIdDir_out.c_str(), rc);
        }

        if(isCollectionInReplayLog(collectionId, catTypeIter)) {
            LOG_INFO("Found collection %s in replay log catalog %d",
                     collectionId.c_str(), (int)catTypeIter);
            return true;
        }
    }

    return false;
}

MMError MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& input)
{
    int rv = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }
    if (!s_inCatalog) {
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }
    if (s_inTransaction) {
        LOG_WARN("Already in Transaction for collection:%s.  Abandoning for:%s",
                 s_transactionArgs.collection_id().c_str(),
                 input.collection_id().c_str());
    }

    if(input.collection_id().find_first_of("|\\?*<\":>/_") != std::string::npos)
    {   // is using illegal chars, and in addition also cannot use '_'
        // Docs in Bug 9061:  http://msdn.microsoft.com/en-us/library/aa365247(v=vs.85).aspx
        LOG_ERROR("collection id cannot the characters '|\\?*<\":>/_': %s",
                  input.collection_id().c_str());
        rv = MM_ERR_BAD_COLLECTION_ID;
        goto error;
    }

    if(input.collection_id().empty()) {
        LOG_ERROR("Collection id not present.");
        rv = MM_ERR_BAD_COLLECTION_ID;
        goto error;
    }

    if(input.collection_id().size()>MAX_COLLECTION_ID_SIZE) {
        LOG_ERROR("Collection id %s exceeds max length of %d. length: "FMTu_size_t,
                  input.collection_id().c_str(),
                  MAX_COLLECTION_ID_SIZE,
                  input.collection_id().size());
        rv = MM_ERR_BAD_COLLECTION_ID;
        goto error;
    }

    // Make sure the collectionId is not already in the replay log.
    if(isCollectionInReplayLog(input.collection_id(), s_catType)) {
        LOG_ERROR("This collection %s has already been updated (committed). "
                  "Catalog must be committed before another update on the "
                  "same collection is possible.",
                  input.collection_id().c_str());
        rv = MM_ERR_ALREADY;
        goto error;
    }

    // Make sure the collectionId is not already in another catalog.
    {
        int catType_out = 0;
        if(isCollectionIdInOtherCatalog(input.collection_id(), s_catType, catType_out)) {
            LOG_ERROR("This collection %s is already in the catalog type:%d, cannot "
                      "add the identical collectionId to catalog type:%d",
                      input.collection_id().c_str(),
                      catType_out,
                      (int)s_catType);
            rv = MM_ERR_ALREADY;
            goto error;
        }
    }

    s_inTransaction = true;
    s_transactionArgs.Clear();
    s_transactionArgs = input;

    s_cache.reset();
    if(!input.reset_collection()) {
        // Load previous cache if it exists.
        (IGNORE_RESULT) s_cache.readCollection(s_metadataSubscriptionPath,
                                               input.collection_id(),
                                               NULL,
                                               s_catType,
                                               s_serverDeviceId,
                                               false);
    }
 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

static std::string getExtension(const std::string& thumbSource) {
    std::size_t slashIndex = thumbSource.find_last_of("/");
    if(slashIndex == std::string::npos) {
        slashIndex = 0;
    }else{
        slashIndex++;   // Skip over the slash.
    }
    std::string file = thumbSource.substr(slashIndex);
    std::size_t dotIndex = file.find_last_of(".");
    if(dotIndex == 0 || dotIndex == std::string::npos) {
        // No extension  (cannot be hidden file, or no period)
        return "";
    }
    dotIndex++; // Skip over the period.
    return file.substr(dotIndex);
}

static bool hasExtension(const std::string& thumb)
{
    std::string ext = getExtension(thumb);
    if(ext == "") {
        return false;
    }else{
        return true;
    }
}

MMError MSAUpdateMetadata(const ccd::UpdateMetadataInput& input)
{
    int rv = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }
    if (!s_inCatalog) {
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }
    if (!s_inTransaction) {
        rv = MM_ERR_NO_TRANSACTION;
        goto error;
    }

    // Ensure thumbnails have extensions
    if(input.metadata().music_album().has_album_thumbnail() &&
       !hasExtension(input.metadata().music_album().album_thumbnail()))
    {
        LOG_ERROR("Music album thumbnail has no ext:%s",
                  input.metadata().music_album().album_thumbnail().c_str());
        rv = MM_ERR_NO_THUMBNAIL_EXT;
        goto error;
    }
    if(input.metadata().photo_album().has_album_thumbnail() &&
       !hasExtension(input.metadata().photo_album().album_thumbnail()))
    {
        LOG_ERROR("Photo album thumbnail has no ext:%s",
                  input.metadata().photo_album().album_thumbnail().c_str());
        rv = MM_ERR_NO_THUMBNAIL_EXT;
        goto error;
    }
    if(input.metadata().photo_item().has_thumbnail() &&
       !hasExtension(input.metadata().photo_item().thumbnail()))
    {
        LOG_ERROR("Photo thumbnail has no ext:%s",
                  input.metadata().photo_item().thumbnail().c_str());
        rv = MM_ERR_NO_THUMBNAIL_EXT;
        goto error;
    }
    if(input.metadata().video_item().has_thumbnail() &&
       !hasExtension(input.metadata().video_item().thumbnail()))
    {
        LOG_ERROR("Video thumbnail has no ext:%s",
                  input.metadata().video_item().thumbnail().c_str());
        rv = MM_ERR_NO_THUMBNAIL_EXT;
        goto error;
    }

    s_cache.update(input.metadata());
    if(!input.metadata().music_album().has_album_thumbnail() &&
       !input.metadata().photo_album().has_album_thumbnail() &&
       !input.metadata().photo_item().has_thumbnail() &&
       !input.metadata().video_item().has_thumbnail())
    {  // No thumbnail specified, exit with success
        goto out;
    }

    rv = copyThumbnailToTemp(input.metadata(),
                             s_serverDeviceId,
                             s_transactionArgs.collection_id(),
                             s_transactionArgs.collection_timestamp());
    if(rv != 0) {
        LOG_ERROR("copyThumbnailToTemp:%d", rv);
        goto error;
    }
 error:
 out:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

MMError MSADeleteMetadata(const ccd::DeleteMetadataInput& input)
{
    int rv = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }
    if (!s_inCatalog) {
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }
    if (!s_inTransaction) {
        rv = MM_ERR_NO_TRANSACTION;
        goto error;
    }
    s_cache.remove(input.object_id());

 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

MMError MSADeleteCatalog(const ccd::DeleteCatalogInput& input)
{
    int rv = 0;
    int rc;
    media_metadata::CatalogType_t catType = MM_CATALOG_MUSIC;
    int catIndex = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }

    {
        LOG_INFO("Delete catalog wait for sync stop");
        ccd::PrivateMsaDataCommitInput requestStart;
        requestStart.set_user_id(s_userId);
        requestStart.set_commit_start(true);
        rc = CCDIPrivateMsaDataCommit(requestStart);
        if(rc != 0) {
            LOG_ERROR("CCDIPrivateMsaDataCommit start failed:%d, userId:"FMTx64,
                      rc, s_userId);
            rv = rc;
            goto error;
        }
    }

    // Bug 10504: delete object metata and collections by catalog type
    // MSADeleteCatalog deletes metadata files directly, and also need to delete items from db
    rc = msaControlDB.beginTransaction();
    if(rc != 0) {
        LOG_ERROR("beginTransaction to delete catalog metadata from db:%d, %s, %d, "FMTx64,
                  rc, s_metadataSubscriptionPath.c_str(), (int)catType, s_serverDeviceId);
        rv = rc;
        goto error_commit;
    }
    for(catIndex = 1; catIndex < 4; catIndex++)
    {
        switch((media_metadata::CatalogType_t)catIndex) {
        case MM_CATALOG_MUSIC:
            catType = MM_CATALOG_MUSIC;
            break;
        case MM_CATALOG_PHOTO:
            catType = MM_CATALOG_PHOTO;
            break;
        case MM_CATALOG_VIDEO:
            catType = MM_CATALOG_VIDEO;
            break;
        default:
            LOG_ERROR("Should never reach here.");
            // Initialize with arbitrary value to not have waring.
            catType = MM_CATALOG_MUSIC;
            break;
        }
        if(input.has_catalog_type()) {
            catType = input.catalog_type();
        }

        rc = MediaMetadataCache::deleteCollections(s_metadataSubscriptionPath,
                                                   catType,
                                                   s_serverDeviceId,
                                                   NULL, NULL);
        if(rc != 0) {
            LOG_ERROR("Delete catalog metadata:%d, %s, %d, "FMTx64,
                      rc, s_metadataSubscriptionPath.c_str(), (int)catType, s_serverDeviceId);
            rv = rc;
            break;
        }
        
        rc = msaControlDB.deleteCatalog(catType);
        if(rc != 0) {
            LOG_ERROR("Delete catalog metadata from db:%d, %s, %d, "FMTx64,
                      rc, s_metadataSubscriptionPath.c_str(), (int)catType, s_serverDeviceId);
            rv = rc;
            break;
        }

        {
            std::string catalogThumbDir_out;
            MediaMetadataCache::formMetadataCatalogDir(s_metadataSubscriptionPath,
                                                       catType,
                                                       METADATA_FORMAT_THUMBNAIL,
                                                       s_serverDeviceId,
                                                       catalogThumbDir_out);
            std::string tempDelPath = getTempDeleteDir(s_tempPath);
            rc = Util_rmRecursiveExcludeDir(catalogThumbDir_out,
                                            tempDelPath,
                                            SYNC_TEMP_DIR);
            if(rc != 0) {
                LOG_ERROR("Util_rmRecursiveExcludeDir(%s):%d, tempDelPath(%s), exclude(%s), %d",
                          catalogThumbDir_out.c_str(), rc, tempDelPath.c_str(),
                          SYNC_TEMP_DIR, (int)catType);
                rv = rc;
                break;
            }
        }
        if(input.has_catalog_type()) {
            break;
        }
    }
    rc = msaControlDB.commitTransaction();
    if(rc != 0) {
        LOG_ERROR("commitTransaction to delete catalog metadata from db:%d, %s, %d, "FMTx64,
                  rc, s_metadataSubscriptionPath.c_str(), (int)catType, s_serverDeviceId);
        rv = rc;
        goto error_commit;
    }

    // don't delete replay log directory and staging directory if anything failed
    if (rv != 0) {
        goto error_commit;
    }

    {
        // After deleting the catalog, there should not be any pending commits.  Delete
        // replay log directory and staging directory.
        rc = Util_rmRecursive(getMsaReplayDir(s_tempPath), getTempDeleteDir(s_tempPath));
        if(rc != 0) {
            LOG_ERROR("Deleting catalog replay dir:%d, (%s,%s)", rc,
                      getMsaReplayDir(s_tempPath).c_str(),
                      getTempDeleteDir(s_tempPath).c_str());
            rv = rc;
        }
        rc = Util_rmRecursive(getMsaStagingDir(s_tempPath), getTempDeleteDir(s_tempPath));
        if(rc != 0) {
            LOG_ERROR("Deleting catalog staging dir:%d, (%s,%s)", rc,
                      getMsaReplayDir(s_tempPath).c_str(),
                      getTempDeleteDir(s_tempPath).c_str());
            rv = rc;
        }
    }

 error_commit:
   {
       ccd::PrivateMsaDataCommitInput requestEnd;
       requestEnd.set_user_id(s_userId);
       requestEnd.set_commit_end(true);
       rc = CCDIPrivateMsaDataCommit(requestEnd);
       if(rc != 0) {
           LOG_ERROR("CCDIPrivateMsaDataCommit end failed:%d, userId:"FMTx64,
                     rc, s_userId);
       }
   }
error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

MMError MSADeleteCollection(const ccd::DeleteCollectionInput& input)
{
    int rv = 0;
    int rc;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }

    if(!s_inCatalog) {
        // Bug 10504: Get rid of backward capability usage.
        // MSABeginCatalog should always be called before MSADeleteCollection
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }

    {
        std::string replayLogDir;
        std::string replayLogFile;
        std::string replayLogPath;
        VPLFile_handle_t fileHandle;

        // Make sure the collection is not already in the replay log.
        if(isCollectionInReplayLog(input.collection_id(), s_catType)) {
            LOG_ERROR("This collection %s has already been updated (committed). "
                      "Catalog must be committed before another update on the "
                      "same collection is possible.",
                      input.collection_id().c_str());
            rv = MM_ERR_ALREADY;
            goto error;
        }

        // Create replay log with delete label.
        // Once the following replay file is created, remove is effectively "committed"
        MediaMetadataCache::formMetadataCatalogDir(getMsaReplayDir(s_tempPath),
                                                   s_catType,
                                                   METADATA_FORMAT_REPLAY,
                                                   s_serverDeviceId,
                                                   replayLogDir);
        rc = Util_CreatePath(replayLogDir.c_str(), VPL_TRUE);
        if(rc != 0) {
            LOG_ERROR("Unable to create temp path:%s", replayLogDir.c_str());
            rv = rc;
            goto error;
        }
        MediaMetadataCache::formReplayFilename(input.collection_id(),
                                               true,
                                               0,
                                               replayLogFile);
        replayLogPath = replayLogDir + "/" + replayLogFile;
        fileHandle = VPLFile_Open(replayLogPath.c_str(),
                                  VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_EXCLUSIVE,
                                  VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
        if(!VPLFile_IsValidHandle(fileHandle)) {
            LOG_ERROR("Unable to create replay file:%d, %s",
                      (int)fileHandle, replayLogPath.c_str());
            rv = (int)fileHandle;
            goto error;
        }
        rc = VPLFile_Close(fileHandle);
        if(rc != VPL_OK) {
            LOG_ERROR("VPLFile_Close:%d, %s", rc, replayLogPath.c_str());
        }
    }

 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    return rv;
}

MMError MSAListCollections(ListCollectionsOutput& output)
{
    int rv = 0;
    int rc;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }

    if(!s_inCatalog) {
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }

    rc = MediaMetadataCache::listCollections(s_metadataSubscriptionPath,
                                             s_catType,
                                             s_serverDeviceId,
                                             output);
    if(rc != 0) {
        LOG_ERROR("MSAListCollections %d:%s,deviceId:"FMTx64,
                  rc, s_metadataSubscriptionPath.c_str(), s_serverDeviceId);
        rv = rc;
        goto error;
    }

 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

MMError MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& input,
                                ccd::GetCollectionDetailsOutput& metadata)
{
    int rv = 0;
    int rc;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    MediaMetadataCache mediaMetadata;

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }

    if(!s_inCatalog) {
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }

    rc = mediaMetadata.readCollection(s_metadataSubscriptionPath,
                                      input.collection_id(),
                                      NULL,
                                      s_catType,
                                      s_serverDeviceId,
                                      true);
    if(rc != 0) {
        LOG_ERROR("Error getCollection:%d,%s from %s:"FMTx64,
                  rc, input.collection_id().c_str(),
                  s_metadataSubscriptionPath.c_str(),
                  s_serverDeviceId);
        rv = rc;
        goto error;
    }

    {
        MediaMetadataCache::ObjIterator metadataIter(mediaMetadata);
        while(true)
        {
            const ContentDirectoryObject* contentObj = metadataIter.next();
            if(contentObj == NULL) {
                break;
            }
            *(metadata.add_metadata()) = *contentObj;
        }
    }

 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

static MMError copyThumbnailToTemp(const media_metadata::ContentDirectoryObject& metadata,
                                   u64 cloudPcDeviceId,
                                   const std::string& collectionId,
                                   u64 timestamp)
{
    int rc;
    VPLFS_stat_t statSource;
    std::string thumbSource;

    // For the cloud node, the thumbnail path is prefixed with "/native"

    if(metadata.has_video_item() && metadata.video_item().has_thumbnail()) {
        thumbSource = metadata.video_item().thumbnail().c_str();
#if defined(CLOUDNODE)
        // Removing the "/native" prefix from the path
        if (thumbSource.compare(0, 7, "/native") == 0) {
            thumbSource.erase(0, 7);
        } else {
            LOG_ERROR("Invalid video thumbnail source:%s, %s",
                      metadata.video_item().thumbnail().c_str(),
                      metadata.object_id().c_str());
            return MM_ERR_INVALID;
        }
#endif
        
        rc = VPLFS_Stat(thumbSource.c_str(), &statSource);
        if(rc == VPL_OK && statSource.type == VPLFS_TYPE_FILE) {
        } else {
            LOG_WARN("Non-existent video thumbnail:%s, %s",
                     metadata.video_item().thumbnail().c_str(),
                     metadata.object_id().c_str());
            return MM_ERR_NOT_FOUND;
        }
    } else if(metadata.has_photo_item() && metadata.photo_item().has_thumbnail()) {
        thumbSource = metadata.photo_item().thumbnail().c_str();
#if defined(CLOUDNODE)
        // Removing the "/native" prefix from the path
        if (thumbSource.compare(0, 7, "/native") == 0) {
            thumbSource.erase(0, 7);
        } else {
            LOG_ERROR("Invalid photo thumbnail source:%s, %s",
                      metadata.photo_item().thumbnail().c_str(),
                      metadata.object_id().c_str());
            return MM_ERR_INVALID;
        }
#endif

        rc = VPLFS_Stat(thumbSource.c_str(), &statSource);
        if(rc == VPL_OK && statSource.type == VPLFS_TYPE_FILE) {
        } else {
            LOG_WARN("Non-existent photo thumbnail:%s, %s",
                     metadata.photo_item().thumbnail().c_str(),
                     metadata.object_id().c_str());
            return MM_ERR_NOT_FOUND;
        }
    } else if(metadata.has_music_album() && metadata.music_album().has_album_thumbnail()) {
        thumbSource = metadata.music_album().album_thumbnail().c_str();
#if defined(CLOUDNODE)
        // Removing the "/native" prefix from the path
        if (thumbSource.compare(0, 7, "/native") == 0) {
            thumbSource.erase(0, 7);
        } else {
            LOG_ERROR("Invalid album thumbnail source:%s, %s",
                      metadata.music_album().album_thumbnail().c_str(),
                      metadata.object_id().c_str());
            return MM_ERR_INVALID;
        }
#endif

        rc = VPLFS_Stat(thumbSource.c_str(), &statSource);
        if(rc == VPL_OK && statSource.type == VPLFS_TYPE_FILE) {
        } else {
            LOG_WARN("Non-existent album thumbnail:%s, %s, %d",
                     metadata.music_album().album_thumbnail().c_str(),
                     metadata.object_id().c_str(), rc);
            return MM_ERR_NOT_FOUND;
        }
    } else if(metadata.has_photo_album() && metadata.photo_album().has_album_thumbnail()) {
        thumbSource = metadata.photo_album().album_thumbnail().c_str();
#if defined(CLOUDNODE)
        // Removing the "/native" prefix from the path
        if (thumbSource.compare(0, 7, "/native") == 0) {
            thumbSource.erase(0, 7);
        } else {
            LOG_ERROR("Invalid album thumbnail source:%s, %s",
                      metadata.photo_album().album_thumbnail().c_str(),
                      metadata.object_id().c_str());
            return MM_ERR_INVALID;
        }
#endif

        rc = VPLFS_Stat(thumbSource.c_str(), &statSource);
        if(rc == VPL_OK && statSource.type == VPLFS_TYPE_FILE) {
        } else {
            LOG_WARN("Non-existent album thumbnail:%s, %s, %d",
                     metadata.photo_album().album_thumbnail().c_str(),
                     metadata.object_id().c_str(), rc);
            return MM_ERR_NOT_FOUND;
        }
    } else {
        if(metadata.has_video_item() ||
           metadata.has_photo_item() ||
           metadata.has_music_album() ||
           metadata.has_photo_album())
        {
            LOG_WARN("Thumbnail expected to exist for %s, %s but does not:"
                     "video:%d, photo:%d, music_album:%d, photo_album:%d, ",
                     metadata.object_id().c_str(),
                     collectionId.c_str(),
                     metadata.has_video_item(),
                     metadata.has_photo_item(),
                     metadata.has_music_album(),
                     metadata.has_photo_album());
            // Log the error, but can do nothing about
        }
        // Thumbnail does not exist, nothing to do
        return 0;
    }

    // Create parallel staging directory.
    std::string thumbnailDir;
    MediaMetadataCache::formThumbnailCollectionDir(getMsaStagingDir(s_tempPath),
                                                   s_catType,
                                                   METADATA_FORMAT_THUMBNAIL,
                                                   s_serverDeviceId,
                                                   collectionId,
                                                   true,
                                                   timestamp,
                                                   thumbnailDir);

    // Make sure staging thumbnail directory exists.
    rc = Util_CreatePath(thumbnailDir.c_str(), VPL_TRUE);
    if(rc != 0) {
        LOG_ERROR("Unable to create temp path:%s", thumbnailDir.c_str());
        return rc;
    }

    // Copy Thumbnail to staging area
    VPLFile_handle_t fHDst = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t fHSrc = VPLFILE_INVALID_HANDLE;
    const int flagDst = VPLFILE_OPENFLAG_CREATE |
                        VPLFILE_OPENFLAG_WRITEONLY |
                        VPLFILE_OPENFLAG_TRUNCATE;

    if(!s_thumbnail_temp_buffer) {
        s_thumbnail_temp_buffer = (char*) malloc(THUMBNAIL_CHUNK_TEMP_BUFFER);
        if(!s_thumbnail_temp_buffer) {
            LOG_ERROR("Out of memory for: %d", THUMBNAIL_CHUNK_TEMP_BUFFER);
            return CCD_ERROR_NOMEM;
        }
    }

    int rv = 0;
    std::string thumbnailFilename;
    std::string ext = getExtension(thumbSource);
    MediaMetadataCache::formThumbnailFilename(metadata.object_id(),
                                              ext,
                                              thumbnailFilename);
    std::string thumbnailTempPath = thumbnailDir+"/"+thumbnailFilename;

    fHDst = VPLFile_Open(thumbnailTempPath.c_str(), flagDst, 0666);
    if (!VPLFile_IsValidHandle(fHDst)) {
        LOG_ERROR("Fail to create or open thumbnail cache file %s",
                  thumbnailTempPath.c_str());
        rv = -1;
        goto exit;
    }

    fHSrc = VPLFile_Open(thumbSource.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fHSrc)) {
        LOG_ERROR("Fail to open thumbnail source %s", thumbSource.c_str());
        rv = -1;
        goto exit;
    }

    {  // Perform the copy in chunks
        for (ssize_t bytesTransfered = 0; bytesTransfered < statSource.size;) {
            ssize_t bytesRead = VPLFile_Read(fHSrc,
                                       s_thumbnail_temp_buffer,
                                       THUMBNAIL_CHUNK_TEMP_BUFFER);
            if (bytesRead > 0) {
                ssize_t wrCnt = VPLFile_Write(fHDst, s_thumbnail_temp_buffer, bytesRead);
                if (wrCnt != bytesRead) {
                    LOG_ERROR("Fail to write to thumbnail cache file %s, %d/%d",
                              thumbnailTempPath.c_str(),
                              (int)bytesRead, (int)wrCnt);
                    rv = -1;
                    goto exit;
                }
                bytesTransfered += bytesRead;
            } else {
                break;
            }
        }
    }
 exit:
    if (VPLFile_IsValidHandle(fHDst)) {
        VPLFile_Close(fHDst);
    }
    if (VPLFile_IsValidHandle(fHSrc)) {
        VPLFile_Close(fHSrc);
    }
    return rv;
}

static MMError readCollection(const std::string collection_path, 
                              google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject>& cdos)
{
    int rv = 0;
    int rc;
    ProtobufFileReader reader;
    cdos.Clear();

    rc = reader.open(collection_path.c_str(), true);
    if(rc != 0) {
        LOG_ERROR("Can't open %s: %d", collection_path.c_str(), rc);
        rv = rc;
        goto end;
    }
    {
        u32 version;
        u64 numEntries;
        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());
        if (!tempStream.ReadVarint32(&version)) {
            LOG_ERROR("Failed to read version in %s", collection_path.c_str());
            rv = rc;
            goto end;
        }

        if(version != METADATA_COLLECTION_FORMAT_VERSION) {
            LOG_INFO("Metadata collection %s format version %d is not %d.  Ignoring.",
                     collection_path.c_str(), version, METADATA_COLLECTION_FORMAT_VERSION);
            rv = 0;
            goto end;
        }

        if (!tempStream.ReadVarint64(&numEntries)) {
            LOG_ERROR("Failed to read numEntries in %s", collection_path.c_str());
            rv = rc;
            goto end;
        }
        int count = 0;
        for (u64 i = 0; i < numEntries; i++) {
            u32 currMsgLen;
            if (!tempStream.ReadVarint32(&currMsgLen)) {
                LOG_ERROR("Failed to read size of entry["FMTu64"] (of "FMTu64"), %s",
                          i, numEntries, collection_path.c_str());
                rv = rc;
                goto end;
            }
            google::protobuf::io::CodedInputStream::Limit limit =
                    tempStream.PushLimit(static_cast<int>(currMsgLen));
            media_metadata::ContentDirectoryObject* currObj = cdos.Add();
            if (!currObj->ParseFromCodedStream(&tempStream)) {
                LOG_ERROR("Failed to read entry["FMTu64"] (of "FMTu64"), %s",
                            i, numEntries, collection_path.c_str());
                rv = rc;
                goto end;
            }
            tempStream.PopLimit(limit);
            LOG_DEBUG("Read [%d]: %s", count++, currObj->DebugString().c_str());
        }
    }
end:
    return rv;
}

MMError MSACommitMetadataTransaction()
{
    int rv = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }
    if (!s_inCatalog) {
        rv = MM_ERR_NO_BEG_CATALOG;
        goto error;
    }
    if (!s_inTransaction) {
        rv = MM_ERR_NO_TRANSACTION;
        goto error;
    }

    {
        int rc;
        MediaServerInfo serverInfo;
        serverInfo.set_cloud_device_id(s_serverDeviceId);
        serverInfo.set_device_name("");

        std::string replayLogDir;
        std::string replayLogFile;
        std::string replayLogPath;
        VPLFile_handle_t fileHandle;

        rv = s_cache.writeCollectionToTemp(getMsaStagingDir(s_tempPath),
                                           s_transactionArgs.collection_id(),
                                           s_transactionArgs.collection_timestamp(),
                                           s_catType,
                                           serverInfo);
        if (rv != 0) {
            LOG_ERROR("writing failed: %d", rv);
            goto error;
        }

        // Once the following replay file is created, data is effectively "committed"
        MediaMetadataCache::formMetadataCatalogDir(getMsaReplayDir(s_tempPath),
                                                   s_catType,
                                                   METADATA_FORMAT_REPLAY,
                                                   s_serverDeviceId,
                                                   replayLogDir);
        // Make sure replay directory exists.
        rc = Util_CreatePath(replayLogDir.c_str(), VPL_TRUE);
        if(rc != 0) {
            LOG_ERROR("Unable to create temp path:%s", replayLogDir.c_str());
            rv = rc;
            goto error;
        }
        MediaMetadataCache::formReplayFilename(
                s_transactionArgs.collection_id(),
                false,
                s_transactionArgs.collection_timestamp(),
                replayLogFile);
        replayLogPath = replayLogDir + "/" + replayLogFile;
        fileHandle = VPLFile_Open(replayLogPath.c_str(),
                                  VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_EXCLUSIVE,
                                  VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
        if(!VPLFile_IsValidHandle(fileHandle)) {
            LOG_ERROR("Unable to create replay file:%d, %s",
                      (int)fileHandle, replayLogPath.c_str());
            rv = (int)fileHandle;
            goto error;
        }
        rc = VPLFile_Close(fileHandle);
        if(rc != VPL_OK) {
            LOG_ERROR("VPLFile_Close:%d, %s", rc, replayLogPath.c_str());
        }

        // Optimization: Best effort copy of metadata file to syncFolder
        // May not succeed because sync agent is not disabled, in which
        // it will just be moved in the commitReplayLog() step.
        rc = s_cache.moveCollectionTempToDataset(s_metadataSubscriptionPath,
                                                 getMsaStagingDir(s_tempPath),
                                                 s_transactionArgs.collection_id(),
                                                 s_transactionArgs.collection_timestamp(),
                                                 s_catType,
                                                 serverInfo);
        if(rc != VPL_OK) {
            LOG_WARN("Not able to move collection:%d, %s. Expected error, can continue.",
                     rc, s_transactionArgs.collection_id().c_str());
        }

        // Bug 10504: delete collection (including contents belongs to it)
        rc = msaControlDB.beginTransaction();
        if(rc != 0) {
            LOG_ERROR("beginTransaction to deleteCollection: %s from DB",
                        s_transactionArgs.collection_id().c_str());
            rv = rc;
            goto error;
        }
        rc = msaControlDB.deleteCollection(s_transactionArgs.collection_id());
        if(rc != MSA_CONTROLDB_OK && rc != MSA_CONTROLDB_DB_NOT_PRESENT) {
            LOG_ERROR("deleteCollection: %s from DB",
                        s_transactionArgs.collection_id().c_str());
            rv = rc;
        }
        {
            // Bug 10504: update collections (including contents belongs to it)
            msaControlDB.updateCollection(s_transactionArgs.collection_id(), s_catType);
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject> cdos;
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject>::iterator it;
            std::string collFilename;
            std::string finalDataDir;
            std::string finalDataFile;
            s_cache.formMetadataCollectionFilename(s_transactionArgs.collection_id(),
                                                   s_transactionArgs.collection_timestamp(),
                                                   collFilename);
            s_cache.formMetadataCatalogDir(s_metadataSubscriptionPath,
                                           s_catType,
                                           METADATA_FORMAT_INDEX,
                                           s_serverDeviceId,
                                           finalDataDir);
            finalDataFile = finalDataDir + "/" + collFilename;
            rc = readCollection(finalDataFile, cdos);
            if(rc != 0) {
                LOG_ERROR("readCollection: %s from DB",
                            s_transactionArgs.collection_id().c_str());
                rv = rc;
            }

            for (it = cdos.begin(); it != cdos.end(); it++) {
                rc = msaControlDB.updateContentObject(s_transactionArgs.collection_id(), *it);
                if(rc != 0) {
                    LOG_ERROR("updateContentObject: %s in collection:%s from DB",
                        it->object_id().c_str(), s_transactionArgs.collection_id().c_str());
                    rv = rc;
                    break;
                }
            }
        }
        rc = msaControlDB.commitTransaction();
        if(rc != 0) {
            LOG_ERROR("commitTransaction to update collections: %s from DB",
                        s_transactionArgs.collection_id().c_str());
            rv = rc;
            goto error;
        }

        s_inTransaction = false;
    }
error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

MMError MSAGetMetadataSyncState(media_metadata::GetMetadataSyncStateOutput& output)
{
    int rv = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    u64 dataset_id = 0;
    output.Clear();

    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }

    {
        ccd::ListSyncSubscriptionsInput ccdiRequest;
        ccdiRequest.set_user_id(s_userId);
        ccdiRequest.set_only_use_cache(true);
        ccd::ListSyncSubscriptionsOutput ccdiResponse;
        rv = CCDIListSyncSubscriptions(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "CCDIListSyncSubscriptions", rv);
            goto error;
        }

        for (int i = 0; i < ccdiResponse.subs_size(); i++) {
            const ccd::SyncSubscriptionDetail& currSub = ccdiResponse.subs(i);
            if (currSub.dataset_details().datasettype() == vplex::vsDirectory::CLEAR_FI) {
                dataset_id = currSub.dataset_details().datasetid(); // Found it.
            }
        }

        if (dataset_id == 0) {
            rv = MM_ERR_NO_DATASET;
            goto error;
        }
        ccd::GetSyncStateInput request;
        request.set_user_id(s_userId);
        request.add_get_sync_states_for_datasets(dataset_id);
        ccd::GetSyncStateOutput response;
        output.set_sync_state(media_metadata::MSA_SYNC_STATE_OUT_OF_SYNC);
        rv = CCDIGetSyncState(request, response);
        if (rv != 0) {
            return rv;
        } else {
            if (response.dataset_sync_state_summary(0).has_status()) {
                switch (response.dataset_sync_state_summary(0).status()) {
                    case ccd::CCD_SYNC_STATE_IN_SYNC:
                        output.set_sync_state(media_metadata::MSA_SYNC_STATE_IN_SYNC);
                        break;
                    case ccd::CCD_SYNC_STATE_SYNCING:
                        output.set_sync_state(media_metadata::MSA_SYNC_STATE_SYNCING);
                        break;
                    case ccd::CCD_SYNC_STATE_OUT_OF_SYNC:
                        output.set_sync_state(media_metadata::MSA_SYNC_STATE_OUT_OF_SYNC);
                        break;
                }
            }
        }
    }
 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

static MMError ConvertCdoToObjectMetadata(const media_metadata::ContentDirectoryObject& obj,
                                                media_metadata::GetObjectMetadataOutput& output)
{
    int rv = 0;

    if (obj.has_music_album()) {
        output.set_media_type(media_metadata::MEDIA_MUSIC_ALBUM);
        if (obj.music_album().has_album_thumbnail())
            output.set_thumbnail(obj.music_album().album_thumbnail());
    } else if (obj.has_photo_album()) {
        output.set_media_type(media_metadata::MEDIA_PHOTO_ALBUM);
        if (obj.photo_album().has_album_thumbnail())
            output.set_thumbnail(obj.photo_album().album_thumbnail());
    } else if (obj.has_music_track()) {
        output.set_media_type(media_metadata::MEDIA_MUSIC_TRACK);
        if (obj.music_track().has_absolute_path())
            output.set_absolute_path(obj.music_track().absolute_path());
        if (obj.music_track().has_file_format())
            output.set_file_format(obj.music_track().file_format());
    } else if (obj.has_photo_item()) {
        output.set_media_type(media_metadata::MEDIA_PHOTO);
        if (obj.photo_item().has_absolute_path())
            output.set_absolute_path(obj.photo_item().absolute_path());
        if (obj.photo_item().has_thumbnail())
            output.set_thumbnail(obj.photo_item().thumbnail());
        if (obj.photo_item().has_file_format())
            output.set_file_format(obj.photo_item().file_format());
        if (obj.photo_item().has_comp_id())
            output.set_comp_id(obj.photo_item().comp_id());
        if (obj.photo_item().has_special_format_flag())
            output.set_special_format_flag(obj.photo_item().special_format_flag());
    } else if (obj.has_video_item()) {
        output.set_media_type(media_metadata::MEDIA_VIDEO);
        if (obj.video_item().has_absolute_path())
            output.set_absolute_path(obj.video_item().absolute_path());
        if (obj.video_item().has_thumbnail())
            output.set_thumbnail(obj.video_item().thumbnail());
        if (obj.video_item().has_file_format())
            output.set_file_format(obj.video_item().file_format());
    } else {
        rv = MM_ERR_BAD_PARAMS;
        LOG_ERROR("Invalid ContentDirectoryObject: %s, %d", obj.DebugString().c_str(), rv);
    }

    return rv;
}

MMError MSAGetContentObjectMetadata(const std::string& objectId,
                                    const std::string& collectionId,
                                    const media_metadata::CatalogType_t& catalogType,
                                    media_metadata::GetObjectMetadataOutput& output)
{
    int rv = 0;
    MediaMetadataCache cache;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaMutex));

    // Bug 10504: MSAGetMetadataObject to query metadata object from DB
    // Still query metadata files when query failed or db corrupts
    rv = msaControlDB.getObjectMetadata(objectId, collectionId, output);
    if (rv != 0) {
        LOG_WARN("Object \"%s\" was not found from DB",
                    objectId.c_str());
        media_metadata::CatalogType_t catTypeIter;
        int catIndex;
        for(catIndex = 1; catIndex < 4; catIndex++)
        {
            MediaMetadataCache cache;
            switch((media_metadata::CatalogType_t)catIndex) {
            case MM_CATALOG_MUSIC:
                catTypeIter = MM_CATALOG_MUSIC;
                break;
            case MM_CATALOG_PHOTO:
                catTypeIter = MM_CATALOG_PHOTO;
                break;
            case MM_CATALOG_VIDEO:
                catTypeIter = MM_CATALOG_VIDEO;
                break;
            default:
                LOG_ERROR("Should never reach here:%d", (int)catIndex);
                rv = MM_ERR_FAIL;
                goto out;
            }
            if(catalogType != 0) {
                catTypeIter = catalogType;
            }
            if (collectionId.empty()) {
                rv = cache.read(s_metadataSubscriptionPath,
                                catTypeIter,
                                s_serverDeviceId,
                                true);
            }
            else {
                rv = cache.readCollection(s_metadataSubscriptionPath,
                                          collectionId,
                                          NULL,  // collectionTimestamp
                                          catTypeIter,
                                          s_serverDeviceId,
                                          true);
            }
            if (rv != 0) {
                LOG_WARN("Failed to read cache of catalog %d: %d!", (int)catTypeIter, rv);
                if(catalogType != 0) {
                    rv = MM_ERR_NOT_FOUND;
                    goto out;
                }else{
                    continue;
                }
            }
            const ContentDirectoryObject* obj = cache.get(objectId);
            if (obj == NULL) {
                LOG_WARN("Object \"%s\", collection(%s), cat(%d,%d), path(%s), serverId("FMTu64") "
                         "was not found by MSAGetContentObjectMetadata:%d",
                         objectId.c_str(), collectionId.c_str(),
                         (int)catalogType, (int)catTypeIter,
                         s_metadataSubscriptionPath.c_str(), s_serverDeviceId,
                         rv);
                rv = MM_ERR_NOT_FOUND;
                if(catalogType != 0) {
                    goto out;
                }else{
                    continue;
                }
            }else{  // Found, success!
                LOG_DEBUG("ContentDirectoryObject found: %s", obj->DebugString().c_str());
                rv = ConvertCdoToObjectMetadata(*obj, output);
                if (rv == 0)
                    LOG_DEBUG("Convert to ObjectMetadata: %s", output.DebugString().c_str());
                break;
            }
        }
    }
    
out:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaMutex));
    return rv;
}

static MMError deleteCollectionMetadata(const std::string& collectionId,
                                        const std::string& metadataDir)
{
    int rv = 0;
    VPLFS_dir_t dirHandleColl;
    int rc = VPLFS_Opendir(metadataDir.c_str(), &dirHandleColl);
    if(rc == VPL_ERR_NOENT) {
        return 0;
    } else if(rc != VPL_OK) {
        LOG_ERROR("Opening metadataDir %d, %s", rc, metadataDir.c_str());
        return rc;
    }

    VPLFS_dirent_t collDirent;
    while((rc = VPLFS_Readdir(&dirHandleColl, &collDirent))==VPL_OK) {
        if(collDirent.type != VPLFS_TYPE_FILE) {
            continue;
        }
        std::string fileCollId;
        u64 timestamp;
        rc = MediaMetadataCache::parseMetadataCollectionFilename(collDirent.filename,
                                                                 fileCollId,
                                                                 timestamp);
        if(rc != 0) {
            LOG_DEBUG("Not a collection metadata file:%d, %s",
                      rc, collDirent.filename);
            continue;
        }
        if(collectionId != fileCollId) {
            continue;
        }

        std::string pathToFile = metadataDir+"/"+collDirent.filename;
        rc = Util_rmRecursive(pathToFile, getTempDeleteDir(s_tempPath));
        if(rc != 0) {
            LOG_ERROR("Could not remove:%d, (%s,%s)", rc, pathToFile.c_str(),
                      getTempDeleteDir(s_tempPath).c_str());
            rv = rc;
            continue;
        }
    }

    rc = VPLFS_Closedir(&dirHandleColl);
    if(rc != VPL_OK) {
        LOG_ERROR("Closing replay directory:%s, %d",
                  metadataDir.c_str(), rc);
    }
    return rv;
}


// Returns true if thumbnail exist.
static bool thumbnailObjExist(const media_metadata::MediaMetadataCache& cache,
                              const std::string& objectId,
                              const std::string& extension)
{
    const ContentDirectoryObject* cdo = cache.get(objectId);
    if(cdo == NULL) {
        return false;
    }

    std::string ext;
    if(cdo->has_video_item() && cdo->video_item().has_thumbnail()) {
        ext = getExtension(cdo->video_item().thumbnail());
    } else if(cdo->has_photo_item() && cdo->photo_item().has_thumbnail()) {
        ext = getExtension(cdo->photo_item().thumbnail());
    } else if(cdo->has_music_album() && cdo->music_album().has_album_thumbnail()) {
        ext = getExtension(cdo->music_album().album_thumbnail());
    } else if(cdo->has_photo_album() && cdo->photo_album().has_album_thumbnail()) {
        ext = getExtension(cdo->photo_album().album_thumbnail());
    } else {
        LOG_ERROR("No thumbnail for object:%s, %s", objectId.c_str(), extension.c_str());
        return false;
    }

    if(ext == extension) {
        return true;
    } else {
        LOG_WARN("Mismatch extension: %s, extension from meta:%s not %s",
                 objectId.c_str(), ext.c_str(), extension.c_str());
        return false;
    }
}

static MMError commitCollection(const std::string& collectionId,
                                u64 timestamp)
{
    int rv = 0;
    int rc;
    // 1. Move metadata from temp metadata to syncfolder
    std::string metadataTempDir;
    std::string metadataSyncFile;

    MediaMetadataCache::formMetadataCatalogDir(getMsaStagingDir(s_tempPath),
                                               s_catType,
                                               METADATA_FORMAT_INDEX,
                                               s_serverDeviceId,
                                               metadataTempDir);
    MediaMetadataCache::formMetadataCollectionFilename(collectionId,
                                                       timestamp,
                                                       metadataSyncFile);
    std::string tempMetadataPath = metadataTempDir + "/" + metadataSyncFile;
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(tempMetadataPath.c_str(), &statBuf);
    if(rc==VPL_OK && statBuf.type == VPLFS_TYPE_FILE) {
        std::string metadataSyncDir;
        MediaMetadataCache::formMetadataCatalogDir(s_metadataSubscriptionPath,
                                                   s_catType,
                                                   METADATA_FORMAT_INDEX,
                                                   s_serverDeviceId,
                                                   metadataSyncDir);
        rc = Util_CreatePath(metadataSyncDir.c_str(), VPL_TRUE);
        if(rc != 0) {
            LOG_ERROR("Unable to create sync path:%s", metadataSyncDir.c_str());
            return rc;
        }
        std::string syncMetadataPath = metadataSyncDir + "/" + metadataSyncFile;
        rc = VPLFile_Rename(tempMetadataPath.c_str(), syncMetadataPath.c_str());
        if(rc != 0) {
            LOG_ERROR("Renaming metadata file %d, %s->%s",
                      rc, tempMetadataPath.c_str(), syncMetadataPath.c_str());
            rv = rc;
            return rv;
        }

        // Bug 10504: delete collection (including contents belongs to it)
        rc = msaControlDB.beginTransaction();
        if(rc != 0) {
            LOG_ERROR("beginTransaction to deleteCollection: %s from DB",
                        collectionId.c_str());
            rv = rc;
            return rv;
        }
        rc = msaControlDB.deleteCollection(collectionId);
        if(rc != MSA_CONTROLDB_OK && rc != MSA_CONTROLDB_DB_NOT_PRESENT) {
            LOG_ERROR("deleteCollection: %s from DB",
                collectionId.c_str());
            rv = rc;
        }
        else {
            // Bug 10504: update collections (including contents belongs to it)
            msaControlDB.updateCollection(collectionId, s_catType);
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject> cdos;
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject>::iterator it;
            rc = readCollection(syncMetadataPath, cdos);
            if(rc != 0) {
                LOG_ERROR("readCollection: %s from DB",
                            s_transactionArgs.collection_id().c_str());
                rv = rc;
            }
            else {
                for (it = cdos.begin(); it != cdos.end(); it++) {
                    rc = msaControlDB.updateContentObject(collectionId, *it);
                    if(rc != 0) {
                        LOG_ERROR("updateContentObject: %s in collection:%s from DB",
                            it->object_id().c_str(), collectionId.c_str());
                        rv = rc;
                        break;
                    }
                }
            }
        }
        rc = msaControlDB.commitTransaction();
        if(rc != 0) {
            LOG_ERROR("commitTransaction to update collections: %s from DB",
                        collectionId.c_str());
            rv = rc;
            return rv;
        }

    }else if(rc == VPL_ERR_NOENT) {
        // If doesn't exist, it's ok because it may have already been moved.
    }else{
        LOG_ERROR("VPLFS_Stat metadata:%d, %s", rc, tempMetadataPath.c_str());
        rv = rc;
        return rv;
    }

    // 2. Move thumbnails from temp to syncfolder
    std::string tempThumbDir;
    std::string syncThumbDir;

    MediaMetadataCache::formThumbnailCollectionDir(getMsaStagingDir(s_tempPath),
                                                   s_catType,
                                                   METADATA_FORMAT_THUMBNAIL,
                                                   s_serverDeviceId,
                                                   collectionId,
                                                   true,
                                                   timestamp,
                                                   tempThumbDir);
    MediaMetadataCache::formThumbnailCollectionDir(s_metadataSubscriptionPath,
                                                   s_catType,
                                                   METADATA_FORMAT_THUMBNAIL,
                                                   s_serverDeviceId,
                                                   collectionId,
                                                   false,
                                                   timestamp,
                                                   syncThumbDir);
    VPLFS_dir_t dirHandleTempThumbnail;
    rc = VPLFS_Opendir(tempThumbDir.c_str(), &dirHandleTempThumbnail);
    if(rc == VPL_ERR_NOENT){
        // Directory does not exist, nothing to commit, not an error
    }else if(rc != VPL_OK) {
        LOG_ERROR("Opening tempThumbnailDir directory:%d, %s",
                  rc, tempThumbDir.c_str());
        return rc;
    }else{
        // Move thumbnail files
        rc = Util_CreatePath(syncThumbDir.c_str(), VPL_TRUE);
        if(rc != 0) {
            LOG_ERROR("Unable to create sync path:%s", syncThumbDir.c_str());
            return rc;
        }

        VPLFS_dirent_t thumbnailDirent;
        while((rc = VPLFS_Readdir(&dirHandleTempThumbnail, &thumbnailDirent))==VPL_OK) {
            if(thumbnailDirent.type != VPLFS_TYPE_FILE) {
                continue;
            }
            std::string sourceThumbPath;
            std::string destThumbPath;
            sourceThumbPath = tempThumbDir+"/"+thumbnailDirent.filename;
            destThumbPath = syncThumbDir+"/"+thumbnailDirent.filename;
            rc = VPLFile_Rename(sourceThumbPath.c_str(), destThumbPath.c_str());
            if(rc != 0) {
                LOG_ERROR("Renaming thumbnail file %d, %s->%s",
                          rc, sourceThumbPath.c_str(), destThumbPath.c_str());
                rv = rc;
                continue;
            }
        }

        rc = VPLFS_Closedir(&dirHandleTempThumbnail);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing replay directory:%s, %d",
                      tempThumbDir.c_str(), rc);
        }
    }

    // 3. Figure out which thumbnails are safe to delete and delete them.
    MediaMetadataCache collectionCache;
    rc = collectionCache.readCollection(s_metadataSubscriptionPath,
                                        collectionId,
                                        &timestamp,
                                        s_catType,
                                        s_serverDeviceId,
                                        false);
    if(rc != 0) {
        LOG_ERROR("Reading collection:%d, %s, %s, "FMTx64,
                  rc, s_metadataSubscriptionPath.c_str(),
                  collectionId.c_str(), s_serverDeviceId);
        return rc;
    }

    VPLFS_dir_t dirHandleSyncThumbnail;
    rc = VPLFS_Opendir(syncThumbDir.c_str(), &dirHandleSyncThumbnail);
    if(rc == VPL_ERR_NOENT){
        // Directory does not exist, nothing to commit, not an error
    }else if(rc != VPL_OK) {
        LOG_ERROR("Opening syncThumbDir directory:%d, %s",
                  rc, syncThumbDir.c_str());
        return rc;
    }else{
        VPLFS_dirent_t thumbnailDirent;
        while((rc = VPLFS_Readdir(&dirHandleSyncThumbnail, &thumbnailDirent))==VPL_OK) {
            if(thumbnailDirent.type != VPLFS_TYPE_FILE) {
                continue;
            }
            std::string objectId;
            std::string ext;
            rc = MediaMetadataCache::parseThumbnailFilename(thumbnailDirent.filename,
                                                            objectId,
                                                            ext);
            if(rc != 0) {
                continue;
            }

            if(!thumbnailObjExist(collectionCache, objectId, ext))
            {   // Thumbnail does not exist in the metadata file and should not
                // exist as a file.  Remove.
                std::string fileToRemove = syncThumbDir + "/" + thumbnailDirent.filename;
                rc = VPLFile_Delete(fileToRemove.c_str());
                if(rc != VPL_OK) {
                    LOG_ERROR("Delete file:%d, %s", rc, fileToRemove.c_str());
                    rv = rc;
                    continue;
                }
            }
        }

        rc = VPLFS_Closedir(&dirHandleSyncThumbnail);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing sync thumb directory:%s, %d",
                      tempThumbDir.c_str(), rc);
        }
    }

    return rv;
}

static int cleanupReplayLog()
{
    int rc;
    int rv = 0;
    std::string replaySubDir;
    MediaMetadataCache::formMetadataTypeDir(getMsaReplayDir(s_tempPath),
                                            s_catType,
                                            replaySubDir);
    rc = Util_rmRecursive(replaySubDir, getTempDeleteDir(s_tempPath));
    if(rc != VPL_OK) {
        LOG_ERROR("Deleting replay directory:%d, (%s,%s)", rc,
                  replaySubDir.c_str(), getTempDeleteDir(s_tempPath).c_str());
        rv = rc;
    }
    rc = Util_rmRecursive(getMsaStagingDir(s_tempPath), getTempDeleteDir(s_tempPath));
    if(rc != VPL_OK) {
        LOG_ERROR("Deleting temp stage directory:%d, (%s,%s)",
                  rc, getMsaStagingDir(s_tempPath).c_str(),
                  getTempDeleteDir(s_tempPath).c_str());
        rv = rc;
    }
    return rv;
}

static MMError commitReplayLog()
{
    int rv = 0;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));

    // Specify final destination
    std::string syncMetadataDir;
    MediaMetadataCache::formMetadataCatalogDir(s_metadataSubscriptionPath,
                                               s_catType,
                                               METADATA_FORMAT_INDEX,
                                               s_serverDeviceId,
                                               syncMetadataDir);

    // 1. Go through replay log
    std::string replayLogDir;
    MediaMetadataCache::formMetadataCatalogDir(getMsaReplayDir(s_tempPath),
                                               s_catType,
                                               METADATA_FORMAT_REPLAY,
                                               s_serverDeviceId,
                                               replayLogDir);

    VPLFS_dir_t dirHandleReplay;
    int rc = VPLFS_Opendir(replayLogDir.c_str(), &dirHandleReplay);
    if(rc == VPL_ERR_NOENT){
        // Nothing to commit, not an error
        rv = 0;
        goto end;
    }else if(rc != VPL_OK) {
        LOG_ERROR("Opening replay directory:%s, %d",
                  replayLogDir.c_str(), rc);
        rv = rc;
        goto end;
    }

    VPLFS_dirent_t replayDirent;
    while((rc = VPLFS_Readdir(&dirHandleReplay, &replayDirent))==VPL_OK) {
        if(replayDirent.type != VPLFS_TYPE_FILE) {
            continue;
        }
        std::string collectionId;
        bool deleted;
        u64 timestamp;
        rc = MediaMetadataCache::parseReplayFilename(replayDirent.filename,
                                                     collectionId,
                                                     deleted,
                                                     timestamp);

        std::string syncThumbnailDir;
        MediaMetadataCache::formThumbnailCollectionDir(s_metadataSubscriptionPath,
                                                       s_catType,
                                                       METADATA_FORMAT_THUMBNAIL,
                                                       s_serverDeviceId,
                                                       collectionId,
                                                       false,
                                                       timestamp,
                                                       syncThumbnailDir);

        if(deleted) {
            rc = deleteCollectionMetadata(collectionId,
                                          syncMetadataDir);
            if(rc != 0) {
                LOG_ERROR("deleteCollectionMetadata: %s from %s",
                          collectionId.c_str(), syncMetadataDir.c_str());
                rv = rc;
                continue;
            }

            // Bug 10504: delete collection (including contents belongs to it)
            rc = msaControlDB.beginTransaction();
            if (rc != 0) {
                LOG_ERROR("beginTransaction to deleteCollection from db: %d", rc);
                rv = rc;
                continue;
            }
            rc = msaControlDB.deleteCollection(collectionId);
            if(rc != MSA_CONTROLDB_OK && rc != MSA_CONTROLDB_DB_NOT_PRESENT) {
                LOG_ERROR("deleteCollection: %s from DB",
                    collectionId.c_str());
                rv = rc;
            }
            rc = msaControlDB.commitTransaction();
            if (rc != 0) {
                LOG_ERROR("beginTransaction to deleteCollection from db: %d", rc);
                rv = rc;
                continue;
            }

            rc = Util_rmRecursive(syncThumbnailDir, getTempDeleteDir(s_tempPath));
            if(rc != 0 && rc != VPL_ERR_NOENT) {
                LOG_ERROR("Could not delete (%s,%s), %d",
                          syncThumbnailDir.c_str(),
                          getTempDeleteDir(s_tempPath).c_str(), rc);
                rv = rc;
                continue;
            }

        } else {
            rc = commitCollection(collectionId, timestamp);
            if(rc != 0) {
                LOG_ERROR("commitCollection:%d, %s, "FMTu64,
                          rc, collectionId.c_str(), timestamp);
                rv = rc;
                continue;
            }

            // Commit done, delete replay log.
            std::string replayPath = replayLogDir+"/"+replayDirent.filename;
            rc = VPLFile_Delete(replayPath.c_str());
            if(rc != 0) {
                LOG_ERROR("Delete replayLog failed:%d, %s", rc, replayPath.c_str());
                rv = rc;
            }
        }
    }

    rc = VPLFS_Closedir(&dirHandleReplay);
    if(rc != VPL_OK) {
        LOG_ERROR("Closing replay directory:%s, %d",
                  replayLogDir.c_str(), rc);
    }

    // 4. If successful delete temp and replay directories
    if(rv == VPL_OK) {
        rc = cleanupReplayLog();
        if(rc != 0) {
            LOG_ERROR("cleanupReplayLog:%d", rc);
            rv = rc;
        }
    } else if(rv == VPL_ERR_ACCESS) {  // Remap error to MM error
        rv = MM_ERR_NO_ACCESS;
    }

end:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_msaUpdateDBMutex));
    return rv;
}

static MMError updatePhotoAblumRelationDBVersion4()
{
    int rc = 0;
    int rv = 0;
    media_metadata::ListCollectionsOutput collections;
    std::string metadataCatDir;
    media_metadata::CatalogType_t catalogType = media_metadata::MM_CATALOG_PHOTO;
    s_cache.formMetadataCatalogDir(s_metadataSubscriptionPath,
                                   catalogType,
                                   METADATA_FORMAT_INDEX,
                                   s_serverDeviceId,
                                   metadataCatDir);
    
    rv = s_cache.listCollections(s_metadataSubscriptionPath,
                                 catalogType,
                                 s_serverDeviceId,
                                 collections);
    if(rv != 0) {
        LOG_ERROR("list collections %d: %s,deviceId:"FMTx64,
                  rv, s_metadataSubscriptionPath.c_str(), s_serverDeviceId);
        goto out;
    }

    // Bug 10504: delete collection (including contents belongs to it)
    rc = msaControlDB.beginTransaction();
    if(rc != 0) {
        LOG_ERROR("beginTransaction to update server db: %d", rc);
        rv = rc;
        goto out;
    }
    for (int i=0;
         i < collections.collection_id_size() && rv == 0;
         i++)
    {
        //check if building DB thread is canceled and return MSA_CONTROLDB_DB_REBUILD_CANCELED
        if (CheckCancel()) {
            LOG_WARN("updateMetadataDB canceled");
            rv = MSA_CONTROLDB_DB_BUILD_CANCELED;
            goto canceled;
        }

        // Bug 10504: update collections (including contents belongs to it)
        rc = msaControlDB.updateCollection(collections.collection_id(i), catalogType);
        if (rc != MSA_CONTROLDB_OK) {
            LOG_ERROR("updateCollection: %s from DB %d",
                      collections.collection_id(i).c_str(), rc);
            rv = rc;
            break;
        }
        {
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject> cdos;
            google::protobuf::RepeatedPtrField<media_metadata::ContentDirectoryObject>::iterator it;
            std::string collFilename;
            std::string finalDataFile;
            s_cache.formMetadataCollectionFilename(collections.collection_id(i),
                                                   collections.collection_timestamp(i),
                                                   collFilename);
            finalDataFile = metadataCatDir + "/" + collFilename;
            rc = readCollection(finalDataFile, cdos);
            if(rc != 0) {
                LOG_ERROR("readCollection: %s from DB %d",
                          collections.collection_id(i).c_str(), rc);
                rv = rc;
                break;
            }

            for (it = cdos.begin(); it != cdos.end(); it++) {

                const media_metadata::ContentDirectoryObject& cdo = *it;

                if (cdo.has_photo_item()) {
                    if (cdo.photo_item().has_album_ref()){
                        rc = msaControlDB.updatePhotoAlbumRelation(cdo.object_id(), cdo.photo_item().album_ref(), collections.collection_id(i));
                    } else {
                        //old style of mm files
                        rc = msaControlDB.updatePhotoAlbumRelation(cdo.object_id(), collections.collection_id(i), collections.collection_id(i));
                    }
                }

                if(rc != 0) {
                    LOG_ERROR("updatePhotoAblumRelationDBVersion4: %s in collection:%s from DB, %d",
                              it->object_id().c_str(), collections.collection_id(i).c_str(), rc);
                    rv = rc;
                    break;
                }
            }
        }
    }
    rc = msaControlDB.commitTransaction();
    if(rc != 0) {
        LOG_ERROR("commitTransaction to update collections: %s from DB %d",
                  s_transactionArgs.collection_id().c_str(), rc);
        if (rv != MSA_CONTROLDB_DB_BUILD_CANCELED) {
            rv = rc;
        }
    }

out:
    return rv;

canceled:
    msaControlDB.rollbackTransaction();
    return rv;
}

