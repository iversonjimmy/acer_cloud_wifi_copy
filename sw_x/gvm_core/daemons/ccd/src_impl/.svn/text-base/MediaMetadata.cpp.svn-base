//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "MediaMetadata.hpp"
#include "ccdi.hpp"
#include "media_metadata_errors.hpp"
#include "McaDb.hpp"
#include "MediaMetadataCache.hpp"
#include "media_metadata_types.pb.h"
#include "gvm_file_utils.hpp"
#include "gvm_misc_utils.h"
#include "scopeguard.hpp"
#include "vpl_fs.h"
#include "vpl_lazy_init.h"
#include "vpl_th.h"
#include "vpl_user.h"
#include "vplu_mutex_autolock.hpp"
#include "ccd_storage.hpp"
#include "cache.h"

#include <string.h>
#include <log.h>
#include <set>
#include <sstream>

#if CCD_ENABLE_MEDIA_CLIENT

static bool s_isInit = false;
static VPLUser_Id_t s_userId = VPLUSER_ID_NONE;
static std::string s_mcaDownloadPath;
static std::string s_mcaDbDirectory;
static VPLLazyInitMutex_t s_mcaMutex = VPLLAZYINITMUTEX_INIT;

static McaControlDB mcaControlDB;

static void checkDeadlock()
{
    if (Cache_ThreadHasLock()) {
        GVM_ASSERT_FAILED("Possible deadlock condition!  Don't call MCA with cache lock held!");
    }
}

#endif

MMError MCAInit(VPLUser_Id_t userId)
{
#if !CCD_ENABLE_MEDIA_CLIENT
    return CCD_ERROR_FEATURE_DISABLED;
#else
    s32 rv = 0;
    bool dbNeedsToBePopulated = false;
    bool dbNeedsToBeUpgradedFromV10ToV11 = false;
    bool dbNeedsToBeUpgradedFromV11ToV12 = false;
    checkDeadlock();
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mcaMutex));
    if(s_isInit) {
        LOG_WARN("MCAInit already init with user:"FMT_VPLUser_Id_t
                 " mcaDownloadPath:%s mcaDbDirectory:%s",
                 s_userId,
                 s_mcaDownloadPath.c_str(), s_mcaDbDirectory.c_str());
        rv = MM_ERR_ALREADY;
        goto out;
    }

    // Assign the userId.
    s_userId = userId;

    {
        char mediaMetaDownDir[CCD_PATH_MAX_LENGTH];
        DiskCache::getDirectoryForMediaMetadataDownload(s_userId, ARRAY_SIZE_IN_BYTES(mediaMetaDownDir), mediaMetaDownDir);
        s_mcaDownloadPath = mediaMetaDownDir;
        LOG_INFO("Metadata path set to \"%s\"", mediaMetaDownDir);
    }
    {
        char mcaDbDir[CCD_PATH_MAX_LENGTH];
        DiskCache::getDirectoryForMcaDb(s_userId, ARRAY_SIZE_IN_BYTES(mcaDbDir), mcaDbDir);
        s_mcaDbDirectory = mcaDbDir;
        LOG_INFO("MCA DB path set to \"%s\"", mcaDbDir);
    }
    rv = McaControlDB::createOrOpenMetadataDB(mcaControlDB, s_mcaDbDirectory,
                                              s_userId,
                                              dbNeedsToBePopulated,
                                              dbNeedsToBeUpgradedFromV10ToV11,
                                              dbNeedsToBeUpgradedFromV11ToV12);
    if(rv != MCA_CONTROLDB_OK) {
        LOG_ERROR("Opening db: %s, user:"FMT_VPLUser_Id_t,
                  s_mcaDbDirectory.c_str(), s_userId);
        goto out;
    }
    s_isInit = true;
    if (dbNeedsToBePopulated) {
        LOG_INFO("Populating MCA categories");
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
                MCAUpdateMetadataDB((media_metadata::CatalogType_t)i);
                break;
            }
        }
        // setting mcaDbInitialized to true will cause the next call to have dbNeedsToBePopulated be false.
        int rc = mcaControlDB.setMcaDbInitialized(true);
        if (rc != 0) {
            LOG_ERROR("setMcaDbInitialized:%d", rc);
        }
    }

    if (dbNeedsToBeUpgradedFromV10ToV11 || dbNeedsToBeUpgradedFromV11ToV12) {

        MCAUpgradePhotoDBforVersion10to12(userId, dbNeedsToBeUpgradedFromV10ToV11, dbNeedsToBeUpgradedFromV11ToV12);

        int rc = mcaControlDB.insertMcaDbVersion(userId,MCA_CONTROL_DB_SCHEMA_VERSION);
        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("Cannot set DB file to version "FMTu64", err:%d",
                        MCA_CONTROL_DB_SCHEMA_VERSION, rc);
            rv = rc;
        }

        // setting mcaDbInitialized to true will cause the next call to have dbNeedsToBePopulated be false.
        rc = mcaControlDB.setMcaDbInitialized(true);
        if (rc != 0) {
            LOG_ERROR("setMcaDbInitialized:%d", rc);
        }
    }
out:
    return rv;
#endif // !CCD_ENABLE_MEDIA_CLIENT
}

void MCALogout()
{
#if !CCD_ENABLE_MEDIA_CLIENT
#else
    checkDeadlock();
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mcaMutex));
    int rc = mcaControlDB.closeDB();
    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("mcaControlDB.closeDB:%d", rc);
    }
    s_isInit = false;
    s_mcaDownloadPath.clear();
    s_mcaDbDirectory.clear();
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mcaMutex));
    LOG_INFO("MCALogout complete");
#endif
}


#if CCD_ENABLE_MEDIA_CLIENT

static MMError updateMetadataDB(u64 serverDeviceId,
                                media_metadata::CatalogType_t catType)
{
    media_metadata::ListCollectionsOutput output;
    int rv = MCA_CONTROLDB_OK;
    int rc;
    if (!s_isInit) {
        return MM_ERR_NOT_INIT;
    }

    rc = media_metadata::MediaMetadataCache::listCollections(s_mcaDownloadPath,
                                                             catType,
                                                             serverDeviceId,
                                                             output);
    if(rc != 0) {
        LOG_ERROR("MSAListCollections %d:%s,deviceId:"FMTx64,
                  rc, s_mcaDownloadPath.c_str(), serverDeviceId);
        return rc;
    }

    CollectionIdAndTimestampMap dbColIdTimestampMap;
    rc = mcaControlDB.getCollections(serverDeviceId, catType, dbColIdTimestampMap);
    if(rc != 0) {
        LOG_ERROR("getCollections:%d", rc);
        return rc;
    }

    // 1) DB should reflect the dataset and any changes in the dataset.
    for(int colIndex=0; colIndex < output.collection_id_size(); colIndex++) {
        std::string dsetCollectionId(output.collection_id(colIndex));
        u64 dsetTimestamp = output.collection_timestamp(colIndex);

        // 1a) look up in db map
        CollectionIdAndTimestampMap::iterator result =
                dbColIdTimestampMap.find(dsetCollectionId);


        if(result != dbColIdTimestampMap.end() && result->second == dsetTimestamp)
        {  // 1b) if in db map and timestamp is unchanged
            dbColIdTimestampMap.erase(result);
            continue;  // erase result from map or collection will be deleted
        } else
        {
            rc = mcaControlDB.beginTransaction();
            if(rc != MCA_CONTROLDB_OK) {
                LOG_ERROR("mcaControlDB begin transaction:%d, skipping %s",
                          rc, dsetCollectionId.c_str());
                if(result != dbColIdTimestampMap.end()) {
                    dbColIdTimestampMap.erase(result);
                }
                continue;  // erase result from map or collection will be deleted
            }

            if(result != dbColIdTimestampMap.end())
            {  // 1c) if in db map, delete because there's an updated version
                assert(result->second != dsetTimestamp);
                LOG_INFO("Collection Id %s@"FMTx64",type:%d, to update:"FMTx64"->"FMTx64,
                         result->first.c_str(), serverDeviceId,
                         (int)catType,
                         result->second, dsetTimestamp);
                rc = mcaControlDB.deleteCollection(serverDeviceId,
                                                   catType,
                                                   dsetCollectionId);
                if(rc != MCA_CONTROLDB_OK) {
                    LOG_ERROR("deleteCollection:%s@"FMTx64", %d",
                              dsetCollectionId.c_str(), serverDeviceId, rc);
                    rv = rc;
                }
            } else
            {  // 1d) else not in db map, no need to delete though
                LOG_INFO("Collection Id %s@"FMTx64" added:"FMTx64,
                         dsetCollectionId.c_str(), serverDeviceId, dsetTimestamp);
            }

            // Either timestamp is different or collection does not exist in db map.
            // add the collection
            rc = mcaControlDB.updateCollection(serverDeviceId,
                                               catType,
                                               dsetCollectionId,
                                               dsetTimestamp);
            if(rc != MCA_CONTROLDB_OK) {
                LOG_ERROR("updateCollection %s,"FMTx64",type:%d:%d",
                          dsetCollectionId.c_str(), dsetTimestamp,
                          (int)catType, rc);
                rv = rc;
            }

            media_metadata::MediaMetadataCache mediaMetadata;
            int rc = mediaMetadata.readCollection(s_mcaDownloadPath,
                                                  dsetCollectionId,
                                                  &dsetTimestamp,
                                                  catType,
                                                  serverDeviceId,
                                                  true);
            if(rc != 0) {
                LOG_ERROR("Error getCollection:%d,%s,type:%d from %s:"FMTx64,
                          rc, dsetCollectionId.c_str(),(int)catType,
                          s_mcaDownloadPath.c_str(),
                          serverDeviceId);
                // Must continue to commitTransaction, because beginTransaction
                // has already started.  Also continue updating other collections.
            } else {
                bool has_photo_album = false;
                bool photo_collection = false;
                std::string album_name;
                std::string thumbnail;
                media_metadata::MediaMetadataCache::ObjIterator metadataIter(mediaMetadata);
                while(true)
                {
                    const media_metadata::ContentDirectoryObject* contentObj = metadataIter.next();
                    if(contentObj == NULL) {
                        break;
                    }

                    has_photo_album = has_photo_album ? true : contentObj->has_photo_album();
                    photo_collection = photo_collection ? true :
                                    (contentObj->has_photo_album() || contentObj->has_photo_item());
                    if (contentObj->has_photo_item()) {
                        album_name = (*contentObj).photo_item().album_name();
                        if (thumbnail.empty())
                            thumbnail = (*contentObj).photo_item().thumbnail();
                    }

                    rc = mcaControlDB.updateContentDirectoryObject(serverDeviceId,
                                                                   dsetCollectionId,
                                                                   *contentObj);
                    if(rc != MCA_CONTROLDB_OK) {
                        LOG_ERROR("updateContentDirectoryObject for %s:%d",
                                  dsetCollectionId.c_str(), rc);
                        rv = rc;
                    }
                }

                if (rc == 0 && photo_collection && !has_photo_album) {
                    LOG_INFO("photo metadata file (v.11 and before): %s timestamp:"FMTu64, dsetCollectionId.c_str(), dsetTimestamp);
                    mcaControlDB.generatePhotoAlbum2Db(serverDeviceId, dsetCollectionId, album_name, thumbnail, dsetTimestamp);
                }

            }

            rc = mcaControlDB.commitTransaction();
            if(rc != MCA_CONTROLDB_OK) {
                LOG_ERROR("commit transaction failed %s:%d",
                          dsetCollectionId.c_str(), rc);
                rv = rc;
                //TODO: Rollback?
            }

            if(result != dbColIdTimestampMap.end())
            {   // erase result from map, or it (the result collection) will be
                // deleted in step 1e)
                dbColIdTimestampMap.erase(result);
            }
        }
    }

    // 1e) remaining in db map delete from db.
    if(dbColIdTimestampMap.size() > 0) {
        rc = mcaControlDB.beginTransaction();
        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("mcaControlDB for delete begin transaction:%d", rc);
            rv = rc;
        }

        CollectionIdAndTimestampMap::iterator iter = dbColIdTimestampMap.begin();
        for(;iter!=dbColIdTimestampMap.end(); ++iter) {
            rc = mcaControlDB.deleteCollection(serverDeviceId,
                                               catType,
                                               iter->first);
            if(rc != MCA_CONTROLDB_OK) {
                LOG_ERROR("deleteCollection:%s@"FMTx64",type:%d, %d",
                          iter->first.c_str(), serverDeviceId, (int)catType, rc);
                rv = rc;
            }
        }

        rc = mcaControlDB.commitTransaction();
        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("mcaControlDB commit transaction:%d", rc);
            rv = rc;
        }
    }

    return rv;
}

static void deleteRemovedCatalogs(const std::set<u64>& fsServerIds)
{
    int rc;
    std::vector<u64> dbCloudPcIds;
    rc = mcaControlDB.getCloudPcIds(dbCloudPcIds);
    if(rc != 0) {
        LOG_ERROR("getCloudPcIds:%d", rc);
        return;
    }
    bool hasDeletion = false;
    for(u32 idIter=0; idIter < dbCloudPcIds.size(); idIter++) {
        if(fsServerIds.find(dbCloudPcIds[idIter]) ==
                fsServerIds.end())
        {   // CloudPcId exists in DB but not in filesystem
            // Delete from DB
            if(!hasDeletion) {
                hasDeletion = true;
                rc = mcaControlDB.beginTransaction();
                if(rc != 0) {
                    LOG_ERROR("beginTransaction:%d", rc);
                }
            }
            LOG_INFO("Removing mediaMetadataCatalog device_id:"FMTu64,
                     dbCloudPcIds[idIter]);

            rc = mcaControlDB.deleteCatalog(dbCloudPcIds[idIter]);
            if(rc != 0) {
                LOG_ERROR("deleteCatalog("FMTu64"):%d", dbCloudPcIds[idIter], rc);
            }
        }
    }
    if(hasDeletion) {
        rc = mcaControlDB.commitTransaction();
        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("commitTransaction:%d", rc);
        }
    }
}

static void getMediaServerDeviceIdFromMetadataFolders(std::set<u64>& mediaServerDeviceIds_out)
{
    // Goes through all the metadataFolders and puts all the mediaServerDeviceId's
    // into the set.
    mediaServerDeviceIds_out.clear();

    for (int i = media_metadata::CatalogType_t_MIN;
         i <= media_metadata::CatalogType_t_MAX;
         i++)
    {
        // Catalog types
        // media_metadata::MM_CATALOG_MUSIC:
        // media_metadata::MM_CATALOG_PHOTO:
        // media_metadata::MM_CATALOG_VIDEO:

        std::string formatDir;
        media_metadata::MediaMetadataCache::formMetadataFormatDir(
                              s_mcaDownloadPath,
                              (media_metadata::CatalogType_t)i,
                              media_metadata::METADATA_FORMAT_INDEX,
                              /*out*/ formatDir);
        VPLFS_dir_t dirHandle;
        VPLFS_dirent_t dirEnt;
        int rc;

        rc = VPLFS_Opendir(formatDir.c_str(), &dirHandle);
        if(rc == VPL_ERR_NOENT) {  // does not exist
            continue;
        }else if(rc != VPL_OK) {
            LOG_ERROR("VPLFS_Opendir(%s) err=%d", formatDir.c_str(), rc);
            continue;
        }

        while ((rc = VPLFS_Readdir(&dirHandle, &dirEnt)) == VPL_OK)
        {
            std::string dirent(dirEnt.filename);
            if(dirEnt.type != VPLFS_TYPE_DIR || dirent=="." || dirent=="..") {
                continue;
            }
            u64 serverDeviceId;
            rc = media_metadata::MediaMetadataCache::parseMetadataCatalogDirname(
                    dirent,
                    /*out*/ serverDeviceId);
            if(rc != 0) {
                continue;
            }
            std::pair<std::set<u64>::iterator,bool> result =
                    mediaServerDeviceIds_out.insert(serverDeviceId);
            if(result.second) {
                LOG_INFO("Found new mediaServer "FMTu64" at %s,%s",
                         serverDeviceId, formatDir.c_str(), dirent.c_str());
            } // else already in the set.
        }
        rc = VPLFS_Closedir(&dirHandle);
        if(rc != VPL_OK) {
            LOG_ERROR("VPLFS_Closedir(%s):%d", formatDir.c_str(), rc);
        }
    }
}

#endif

MMError MCAUpdateMetadataDB(media_metadata::CatalogType_t catalogType)
{
#if !CCD_ENABLE_MEDIA_CLIENT
    return CCD_ERROR_FEATURE_DISABLED;
#else
    int rv = 0;
    google::protobuf::RepeatedPtrField< media_metadata::MediaServerInfo > ccdCacheMediaServerList;
    std::set<u64> mediaServerDeviceIds;
    
    checkDeadlock();
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mcaMutex));
    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto out;
    }

    getMediaServerDeviceIdFromMetadataFolders(mediaServerDeviceIds);

    for(std::set<u64>::const_iterator setIter = mediaServerDeviceIds.begin();
        setIter != mediaServerDeviceIds.end(); ++setIter)
    {
        int rc = updateMetadataDB(*setIter,
                                  catalogType);
        if(rc != 0){
            LOG_ERROR("updateMetadataDB:cloudId:"FMTu64", type:%d, %d",
                      *setIter, (int)catalogType, rc);
            rv = rc;
        }
    }

    deleteRemovedCatalogs(mediaServerDeviceIds);

 out:
    return rv;
#endif
}

MMError MCAQueryMetadataObjects(u64 deviceId,
                                media_metadata::DBFilterType_t filter_type,
                                const std::string& selection,
                                const std::string& sort_order,
                                google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > &output)
{
#if !CCD_ENABLE_MEDIA_CLIENT
    return CCD_ERROR_FEATURE_DISABLED;
#else
    int rc;
    int rv = MCA_CONTROLDB_OK;
    checkDeadlock();
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mcaMutex));
    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto error;
    }

    switch(filter_type) {
    case media_metadata::MCA_MDQUERY_MUSICTRACK:
        rc = mcaControlDB.getMusicTracks(deviceId, selection, sort_order, output);
        break;
    case media_metadata::MCA_MDQUERY_MUSICALBUM:
        rc = mcaControlDB.getMusicAlbums(deviceId, selection, sort_order, output);
        break;
    case media_metadata::MCA_MDQUERY_MUSICARTIST:
        rc = mcaControlDB.getMusicArtist(deviceId, selection, sort_order, output);
        break;
    case media_metadata::MCA_MDQUERY_MUSICGENRE:
        rc = mcaControlDB.getMusicGenre(deviceId, selection, sort_order, output);
        break;
    case media_metadata::MCA_MDQUERY_PHOTOITEM:
        rc = mcaControlDB.getPhotos(deviceId, selection, sort_order, output);
        break;
    case media_metadata::MCA_MDQUERY_PHOTOALBUM:
        rc = mcaControlDB.getPhotoAlbums(deviceId, selection, sort_order, output);
        break;
    case media_metadata::MCA_MDQUERY_VIDEOITEM:
        rc = mcaControlDB.getVideos(deviceId, selection, sort_order, output);
        break;
    case media_metadata::MCA_MDQUERY_VIDEOALBUM:
        rc = mcaControlDB.getVideoAlbums(deviceId, selection, sort_order, output);
        break;
    default:
        rc = MCA_ERR_INVALID_FILTER_TYPE;
        break;
    }

    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("MCAQueryMetadataObjects:%d", rc);
        rv = rc;
    }

 error:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mcaMutex));
    return rv;
#endif
}

#if CCD_ENABLE_MEDIA_CLIENT
#if 0 //MCA_CONTROL_DB_SCHEMA_VERSION == 11
// DEPRECATED
// replace by upgradePhotoDBforVersion10to12
static MMError upgradePhotoDBforVersion10to11(u64 serverDeviceId)
{
    LOG_INFO("Update comp_id and special_format_flag to photoDB, serviceDeviceId: "FMTu64, serverDeviceId);
    media_metadata::CatalogType_t catType = media_metadata::MM_CATALOG_PHOTO;
    media_metadata::ListCollectionsOutput output;
    int rv = MCA_CONTROLDB_OK;
    int rc;
    if (!s_isInit) {
        return MM_ERR_NOT_INIT;
    }

    rc = media_metadata::MediaMetadataCache::listCollections(s_mcaDownloadPath,
                                                             catType,
                                                             serverDeviceId,
                                                             output);
    if(rc != 0) {
        LOG_ERROR("MSAListCollections %d:%s,deviceId:"FMTx64,
                  rc, s_mcaDownloadPath.c_str(), serverDeviceId);
        return rc;
    }

    for(int colIndex=0; colIndex < output.collection_id_size(); colIndex++) {

        std::string dsetCollectionId(output.collection_id(colIndex));
        u64 dsetTimestamp = output.collection_timestamp(colIndex);

        rc = mcaControlDB.beginTransaction();

        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("mcaControlDB begin transaction:%d, skipping %s",
                      rc, dsetCollectionId.c_str());
            continue;
        }


        media_metadata::MediaMetadataCache mediaMetadata;
        int rc = mediaMetadata.readCollection(s_mcaDownloadPath,
                                              dsetCollectionId,
                                              &dsetTimestamp,
                                              catType,
                                              serverDeviceId,
                                              true);
        if(rc != 0) {
            LOG_ERROR("Error getCollection:%d,%s,type:%d from %s:"FMTx64,
                      rc, dsetCollectionId.c_str(),(int)catType,
                      s_mcaDownloadPath.c_str(),
                      serverDeviceId);
            // Must continue to commitTransaction, because beginTransaction
            // has already started.  Also continue updating other collections.
        } else {
            media_metadata::MediaMetadataCache::ObjIterator metadataIter(mediaMetadata);
            while(true){

                const media_metadata::ContentDirectoryObject* contentObj = metadataIter.next();
                if(contentObj == NULL) {
                    break;
                }
                if(contentObj->has_photo_item() ) {

                    if (contentObj->photo_item().has_special_format_flag() || 
                        contentObj->photo_item().has_comp_id() ) {

                        rc = mcaControlDB.updateContentDirectoryObject(serverDeviceId,
                                                                       dsetCollectionId,
                                                                       *contentObj);
                        if(rc != MCA_CONTROLDB_OK) {
                            LOG_ERROR("updateContentDirectoryObject for %s:%d",
                                      dsetCollectionId.c_str(), rc);
                            rv = rc;
                        } else {
                            LOG_INFO("Update ObjId: %s compId:%s special_format_flag: "FMTu32,
                                        contentObj->object_id().c_str(),
                                        contentObj->photo_item().has_comp_id() ? contentObj->photo_item().comp_id().c_str() : "N/A",
                                        contentObj->photo_item().has_special_format_flag() ? contentObj->photo_item().special_format_flag() : 0
                                        );
                        }
                    }
                } else {
                    LOG_WARN("Not photo item! GetCollection:%d,%s,type:%d from %s:"FMTx64" object id:%s",
                      rc, dsetCollectionId.c_str(),(int)catType,
                      s_mcaDownloadPath.c_str(),
                      serverDeviceId,
                      contentObj->has_object_id() ? contentObj->object_id().c_str() : "N/A");
                }
            }
        }

        rc = mcaControlDB.commitTransaction();
        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("commit transaction failed %s:%d",
                      dsetCollectionId.c_str(), rc);
            rv = rc;
            //TODO: Rollback?
        }
    }

    return rv;
}
#endif
#endif

#if CCD_ENABLE_MEDIA_CLIENT
static MMError upgradePhotoDBforVersion10to12(u64 serverDeviceId, const bool& v10to11, const bool& v11to12)
{
    LOG_INFO("Update MCA Database, serviceDeviceId: "FMTu64, serverDeviceId);
    media_metadata::CatalogType_t catType = media_metadata::MM_CATALOG_PHOTO;
    media_metadata::ListCollectionsOutput output;
    int rv = MCA_CONTROLDB_OK;
    int rc;
    if (!s_isInit) {
        return MM_ERR_NOT_INIT;
    }

    rc = media_metadata::MediaMetadataCache::listCollections(s_mcaDownloadPath,
                                                             catType,
                                                             serverDeviceId,
                                                             output);
    if(rc != 0) {
        LOG_ERROR("MSAListCollections %d:%s,deviceId:"FMTx64,
                  rc, s_mcaDownloadPath.c_str(), serverDeviceId);
        return rc;
    }

    rc = mcaControlDB.beginTransaction();

    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("mcaControlDB begin transaction:%d",rc);
        return rc;
    }

    for(int colIndex=0; colIndex < output.collection_id_size(); colIndex++) {

        std::string dsetCollectionId(output.collection_id(colIndex));
        u64 dsetTimestamp = output.collection_timestamp(colIndex);

        media_metadata::MediaMetadataCache mediaMetadata;
        int rc = mediaMetadata.readCollection(s_mcaDownloadPath,
                                              dsetCollectionId,
                                              &dsetTimestamp,
                                              catType,
                                              serverDeviceId,
                                              true);
        if(rc != 0) {
            LOG_ERROR("Error getCollection:%d,%s,type:%d from %s:"FMTx64,
                      rc, dsetCollectionId.c_str(),(int)catType,
                      s_mcaDownloadPath.c_str(),
                      serverDeviceId);
            // Must continue to commitTransaction, because beginTransaction
            // has already started.  Also continue updating other collections.
        } else {
            bool albumUpdated = false;
            std::string albumName, thumbnail;

            media_metadata::MediaMetadataCache::ObjIterator metadataIter(mediaMetadata);
            while(true){
                const media_metadata::ContentDirectoryObject* contentObj = metadataIter.next();
                if(contentObj == NULL) {
                    break;
                }
                if(contentObj->has_photo_item() ) {

                    if (v10to11 && (contentObj->photo_item().has_special_format_flag() || 
                                    contentObj->photo_item().has_comp_id()
                                   )
                        ) {

                        rc = mcaControlDB.updateContentDirectoryObject(serverDeviceId,
                                                                       dsetCollectionId,
                                                                       *contentObj);
                        if(rc != MCA_CONTROLDB_OK) {
                            LOG_ERROR("updateContentDirectoryObject for %s:%d",
                                      dsetCollectionId.c_str(), rc);
                            rv = rc;
                        } else {
                            LOG_INFO("Update ObjId: %s compId:%s special_format_flag: "FMTu32,
                                        contentObj->object_id().c_str(),
                                        contentObj->photo_item().has_comp_id() ? contentObj->photo_item().comp_id().c_str() : "N/A",
                                        contentObj->photo_item().has_special_format_flag() ? contentObj->photo_item().special_format_flag() : 0
                                        );
                        }
                    }

                    if (v11to12) {
                        if(contentObj->photo_item().has_album_ref()) {
                            mcaControlDB.updatePhotoAlbumRelation(serverDeviceId, contentObj->object_id(), contentObj->photo_item().album_ref());
                        } else {
                            // mm file format is version 11, no photo album info and album_id_ref
                            // use collectionId as albumId
                            mcaControlDB.updatePhotoAlbumRelation(serverDeviceId, contentObj->object_id(), dsetCollectionId);
                            albumName = contentObj->photo_item().album_name();
                            thumbnail = contentObj->photo_item().thumbnail();
                        }
                    }
                } else if (v11to12 && contentObj->has_photo_album()) {
                    mcaControlDB.updatePhotoAlbum(serverDeviceId, dsetCollectionId, *contentObj);
                    albumUpdated = true;
                } else {
                    LOG_WARN("Not photo item! GetCollection:%d,%s,type:%d from %s:"FMTx64" object id:%s",
                      rc, dsetCollectionId.c_str(),(int)catType,
                      s_mcaDownloadPath.c_str(),
                      serverDeviceId,
                      contentObj->has_object_id() ? contentObj->object_id().c_str() : "N/A");
                }
            }

            if (!albumUpdated) {
                // mm file format is version 11, no photo album info and album_id_ref
                mcaControlDB.generatePhotoAlbum2Db(serverDeviceId, dsetCollectionId, albumName,thumbnail, dsetTimestamp);
            }

        }
    }

    rc = mcaControlDB.commitTransaction();
    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("commit transaction failed :%d", rc);
        return rc;
        //TODO: Rollback?
    }

    return rv;
}
#endif

MMError MCAUpgradePhotoDBforVersion10to12(VPLUser_Id_t userId, const bool &v10to11, const bool& v11to12)
{
#if !CCD_ENABLE_MEDIA_CLIENT
    return CCD_ERROR_FEATURE_DISABLED;
#else
    int rv = 0;
    int rc;
    google::protobuf::RepeatedPtrField< media_metadata::MediaServerInfo > ccdCacheMediaServerList;
    std::set<u64> mediaServerDeviceIds;

    checkDeadlock();
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mcaMutex));
    if (!s_isInit) {
        rv = MM_ERR_NOT_INIT;
        goto out;
    }

    getMediaServerDeviceIdFromMetadataFolders(mediaServerDeviceIds);

    for(std::set<u64>::const_iterator setIter = mediaServerDeviceIds.begin();
        setIter != mediaServerDeviceIds.end(); ++setIter)
    {
        //rc = upgradePhotoDBforVersion10to11(*setIter);
        rc = upgradePhotoDBforVersion10to12(*setIter, v10to11, v11to12);
        if(rc != 0){
            LOG_ERROR("updateMetadataDB:cloudId:"FMTu64", error: %d",
                      *setIter, rc);
            rv = rc;
        }
    }

 out:
    return rv;
#endif
}

