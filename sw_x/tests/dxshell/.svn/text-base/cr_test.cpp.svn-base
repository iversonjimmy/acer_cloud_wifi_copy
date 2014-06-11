//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "cr_test.hpp"

#include <vpl_plat.h>
#include <ccdi.hpp>
#include <vplex_file.h>
#include <vpl_conv.h>

#include <deque>
#include <sstream>
#include <iomanip>

#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include "gvm_file_utils.h"
#include "gvm_file_utils.hpp"
#include "TargetDevice.hpp"
#include <log.h>

#include "cJSON2.h"
#include "fs_test.hpp"
#include <queue>
#include <scopeguard.hpp>

const char* PICSTREAM_STR = "PicStream";

#define CR_TEST_PICSTREAM_DSET_NAME "PicStream"

static int deleteAllExceptSyncFolder(const std::string& folderToEmpty);

int cr_subscribe_up(bool android)
{
    int rv = 0;
    u64 userId;
    u64 datasetId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    rv = getDatasetId(userId, CR_TEST_UP_DSET_NAME, datasetId);
    if(rv != 0) {
        LOG_ERROR("getDatasetId("FMTx64",%s,"FMTx64"):%d",
                  userId, CR_TEST_UP_DSET_NAME, datasetId, rv);
        return rv;
    }
    std::string appDataPath;
    if(android) {
        appDataPath = "/sdcard/DCIM/Camera";
    }else{
        appDataPath ="$LOCALROOT/dxshell/picstream";
    }
    rv = subscribeDatasetCrUp(userId, datasetId,
                              true,
                              ccd::SUBSCRIPTION_TYPE_PRODUCER,
                              appDataPath,
                              CR_TEST_UP_DSET_NAME,
                              android);
    if(rv != 0) {
        LOG_ERROR("subscribe %s dataset failed("FMTx64","FMTx64", %s):%d",
                  CR_TEST_UP_DSET_NAME, userId, datasetId, appDataPath.c_str(), rv);
        return rv;
    }

    int rc;
    std::string cameraDirectory;
    rc = getDatasetRoot(userId, "CR Upload", cameraDirectory);
    if(rc != 0) {
        LOG_ERROR("getDatasetId failed for CR Upload:%d", rc);
        rv = rc;
    }

    rc = Util_CreatePath(cameraDirectory.c_str(), VPL_TRUE);
    if(rc != 0) {
        LOG_ERROR("Unable to create temp path:%s", cameraDirectory.c_str());
        return rc;
    }

    LOG_ALWAYS("Adding cameraDirectory (%s) to camera_roll_upload_dirs to retain"
               " backward compatibility with previously written tests.",
               cameraDirectory.c_str());
    std::vector<std::string> toAdd;
    toAdd.push_back(cameraDirectory);
    rc = cr_add_upload_dirs(toAdd);
    if(rc != 0) {
        LOG_ERROR("cr_add_upload_dirs:%d", rc);
        rv = rc;
    }

    return rv;
}

int cr_subscribe_down(const std::string& localDsetName,
                      u64 maxBytes, bool useMaxBytes,
                      u64 maxFiles, bool useMaxFiles)
{
    int rv = 0;
    u64 userId;
    u64 datasetId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    rv = getDatasetId(userId, CR_TEST_DOWN_DSET_NAME, datasetId);
    if(rv != 0) {
        LOG_ERROR("getDatasetId("FMTx64",%s,"FMTx64"):%d",
                  userId, CR_TEST_DOWN_DSET_NAME, datasetId, rv);
        return rv;
    }

    std::string appDataPath("$LOCALROOT/dxshell/picstream");
    rv = subscribeDatasetCrDown(userId, datasetId,
                                true,
                                ccd::SUBSCRIPTION_TYPE_CONSUMER,
                                appDataPath,
                                localDsetName.c_str(),
                                maxBytes, useMaxBytes,
                                maxFiles, useMaxFiles);
    if(rv != 0) {
        LOG_ERROR("subscribe %s dataset failed("FMTx64","FMTx64", %s):%d",
                  CR_TEST_DOWN_DSET_NAME, userId, datasetId, appDataPath.c_str(), rv);
        return rv;
    }
    return rv;
}

int cr_unsubscribe_down()
{
    u64 userId;
    u64 datasetId;
    int rv = 0;

    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    rv = getDatasetId(userId, CR_TEST_DOWN_DSET_NAME, datasetId);
    if(rv != 0) {
        LOG_ERROR("getDatasetId("FMTx64",%s,"FMTx64"):%d",
                  userId, CR_TEST_DOWN_DSET_NAME, datasetId, rv);
        return rv;
    }

    ccd::DeleteSyncSubscriptionsInput dssi;
    dssi.set_user_id(userId);
    dssi.add_dataset_ids(datasetId);
    rv = CCDIDeleteSyncSubscriptions(dssi);
    if(rv != 0) {
        LOG_ERROR("CCDIDeleteSyncSubscriptions:%d", rv);
    }
    return rv;
}


static int printExistingCameraDirs(ccd::GetSyncStateOutput& syncStateOut)
{
    int rc;

    ccd::GetSyncStateInput syncStateIn;
    syncStateIn.set_get_is_camera_roll_upload_enabled(true);
    syncStateIn.set_get_is_camera_roll_global_delete_enabled(true);
    syncStateIn.set_get_camera_roll_upload_dirs(true);
    syncStateIn.set_get_camera_roll_download_dirs(true);

    rc = CCDIGetSyncState(syncStateIn, syncStateOut);
    if(rc != 0)
    {
        LOG_ERROR("CCDIGetSyncState:%d, CRU_Enabled:%d, GlobalDeleteEnabled:%d",
                  rc,
                  syncStateOut.has_is_camera_roll_upload_enabled(),
                  syncStateOut.has_is_camera_roll_global_delete_enabled());
        return rc;
    }

    LOG_ALWAYS("SyncState CameraRoll Settings:"
               "\n%s",
               syncStateOut.DebugString().c_str());
    return 0;
}

int cr_clear_test(bool clearUploadFolder, bool clearDownloadFolder)
{
    u64 userId;
    int rc;
    int rv = 0;

    if(!clearUploadFolder && !clearDownloadFolder) {
        return 0;
    }

    rc = getUserId(userId);
    if(rc != 0) {
        LOG_ERROR("getUserId:%d", rc);
        return rc;
    }

    if(clearUploadFolder) {
        std::string crUpRoot;
        rc = getDatasetRoot(userId, "CR Upload", crUpRoot);
        if(rc != 0) {
            LOG_WARN("getDatasetId failed for CR Upload:%d, May not be error since subscribe is optional", rc);
        }else{
            rc = deleteAllExceptSyncFolder(crUpRoot);
            if(rc != 0) {
                LOG_ERROR("Problems clearing %s:%d", crUpRoot.c_str(), rc);
                rv = rc;
            }
        }
    }

    if(clearDownloadFolder) {
        std::string crDownRoot;
        rc = getDatasetRoot(userId, "CameraRoll", crDownRoot);
        if(rc != 0) {
            LOG_WARN("getDatasetId failed for CameraRoll:%d, May not be error since subscribe is optional", rc);
        }else{
            rc = deleteAllExceptSyncFolder(crDownRoot);
            if(rc != 0) {
                LOG_ERROR("Problems clearing %s:%d", crDownRoot.c_str(), rc);
                rv = rc;
            }
        }
    }

    ccd::GetSyncStateOutput response;
    rc = printExistingCameraDirs(response);
    if(rc != 0) {
        LOG_ERROR("printExistingCameraDirs:%d", rc);
        return rv;
    }

    if(clearUploadFolder) {
        for(int dirIndex=0;
            dirIndex < response.camera_roll_upload_dirs_size();
            ++dirIndex)
        {
            std::string folderToClear = response.camera_roll_upload_dirs(dirIndex);
            rc = deleteAllExceptSyncFolder(folderToClear);
            if(rc != 0) {
                LOG_ERROR("Problems clearing upload %s:%d",folderToClear.c_str(), rc);
                rv = rc;
            }
        }
    }

    if(clearDownloadFolder) {
        // Clear full res photos
        for(int dirIndex=0;
            dirIndex < response.camera_roll_full_res_download_dirs_size();
            ++dirIndex)
        {
            std::string folderToClear = response.camera_roll_full_res_download_dirs(dirIndex).dir();
            rc = deleteAllExceptSyncFolder(folderToClear);
            if(rc != 0) {
                LOG_ERROR("Problems clearing full-res %s:%d",folderToClear.c_str(), rc);
                rv = rc;
            }
        }

        // Clear low res photos
        for(int dirIndex=0;
            dirIndex < response.camera_roll_low_res_download_dirs_size();
            ++dirIndex)
        {
            std::string folderToClear = response.camera_roll_low_res_download_dirs(dirIndex).dir();
            rc = deleteAllExceptSyncFolder(folderToClear);
            if(rc != 0) {
                LOG_ERROR("Problems clearing low-res %s:%d",folderToClear.c_str(), rc);
                rv = rc;
            }
        }

        // Clear thumbnails
        for(int dirIndex=0;
            dirIndex < response.camera_roll_thumb_download_dirs_size();
            ++dirIndex)
        {
            std::string folderToClear = response.camera_roll_thumb_download_dirs(dirIndex).dir();
            rc = deleteAllExceptSyncFolder(folderToClear);
            if(rc != 0) {
                LOG_ERROR("Problems clearing thumbnail %s:%d",folderToClear.c_str(), rc);
                rv = rc;
            }
        }
    }

    return rv;
}

static void countPhotos(const std::string& cameraRollPath,
                        u32& numPhotos)
{
    numPhotos = 0;
    int rc;
    std::deque<std::string> localDirs;
    localDirs.push_back(cameraRollPath);
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if(target == NULL) {
        return;
    }

    while(!localDirs.empty()) {
        std::string dirPath(localDirs.front());
        localDirs.pop_front();

        rc = target->openDir(dirPath.c_str());
        if(rc == VPL_ERR_NOENT){
            // no photos
            continue;
        }else if(rc != VPL_OK) {
            LOG_ERROR("Unable to open %s:%d", dirPath.c_str(), rc);
            continue;
        }

        VPLFS_dirent_t folderDirent;
        //while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        while((rc = target->readDir(dirPath, folderDirent))==VPL_OK)
        {
            std::string dirent(folderDirent.filename);
            if(folderDirent.type != VPLFS_TYPE_FILE) {
                if(dirent=="." || dirent==".." || dirent==".sync_temp") {
                    // We want to skip these directories
                }else{
                    // save directory for later processing
                    std::string deeperDir = dirPath+"/"+dirent;
                    localDirs.push_back(deeperDir);
                }
                continue;
            }

            if(!isPhotoFile(dirent)) {
                continue;
            }
            numPhotos++;
        }

        rc = target->closeDir(dirPath);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing %s:%d", dirPath.c_str(), rc);
        }
    }

    if (target != NULL)
        delete target;
}

int cr_check_status_test()
{
    int rc;
    int rv = 0;
    u64 userId;
    u64 datasetId = 0;
    bool crUploadSubscribed = true;
    bool crDownloadSubscribed = true;
    TargetDevice *target = NULL;

    rc = getUserId(userId);
    if(rc != 0) {
        LOG_ERROR("getUserId:%d", rc);
        return rc;
    }
    rc = getDatasetId(userId, CR_TEST_PICSTREAM_DSET_NAME, datasetId);
    if(rc != 0) {
        LOG_ERROR("PicStream Dataset does not exist:%d", rc);
        return rc;
    }

    target = getTargetDevice();
    if(target == NULL) {
        LOG_ERROR("target is NULL!");
        return -1;
    }

    std::string crUpRoot;
    rc = getDatasetRoot(userId, CR_TEST_UP_DSET_NAME, crUpRoot);
    if(rc != 0) {
        LOG_WARN("getDatasetId failed for %s:%d, ok -- upload subscription no longer required.", CR_TEST_UP_DSET_NAME, rc);
        crUploadSubscribed = false;
    }

    std::string crDownRoot;
    rc = getDatasetRoot(userId, CR_TEST_DOWN_DSET_NAME, crDownRoot);
    if(rc != 0) {
        LOG_WARN("getDatasetId failed for CameraRoll:%d, ok -- download subscription no longer required.", rc);
        crDownloadSubscribed = false;
    }

    u32 crUpNumPhotos = 0;
    u32 crDownNumPhotos = 0;

    countPhotos(crUpRoot, crUpNumPhotos);
    countPhotos(crDownRoot, crDownNumPhotos);

    LOG_ALWAYS("Client CameraRoll State: UserId:"FMTx64
               "\n %s subscribed (DEPRECATED - ERR IF SET): %d"
               "\n %s subscribed (DEPRECATED - ERR IF SET): %d",
               userId,
               CR_TEST_UP_DSET_NAME, crUploadSubscribed?1:0,
               CR_TEST_DOWN_DSET_NAME, crDownloadSubscribed?1:0);

    ccd::GetSyncStateInput gssIn;
    ccd::GetSyncStateOutput gssOut;
    gssIn.set_user_id(userId);
    gssIn.add_get_sync_states_for_datasets(datasetId);
    gssIn.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_UPLOAD);
    gssIn.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES);
    gssIn.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES);
    gssIn.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL);
    gssIn.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_DELETION);
    rc = CCDIGetSyncState(gssIn, gssOut);
    if(rc != 0) {
        LOG_ERROR("CCDIGetSyncState:%d", rc);
        rv = rc;
    }else if(gssOut.feature_sync_state_summary_size()<5) {
        LOG_ERROR("feature_sync_state_summary does not contain 5 entries:%d",
                  gssOut.feature_sync_state_summary_size());
        rv = -1;
    }else if(gssOut.dataset_sync_state_summary_size()==0) {
        LOG_ERROR("dataset_sync_state_summary does not contain 1 entry:%d",
                  gssOut.dataset_sync_state_summary_size());
        rv = -1;
    }else{
        // Normal operation
        LOG_ALWAYS("Feature Sync State Summaries:\n"
                   "  PicStream_Upload:     %s"
                   "  PicStream_FullRes:    %s"
                   "  Picstream_LowRes:     %s"
                   "  Picstream_Thumbnail:  %s"
                   "  Picstream_Deletion:   %s"
                   "  Picstream Dset Combo: %s",
                   gssOut.feature_sync_state_summary(0).DebugString().c_str(),
                   gssOut.feature_sync_state_summary(1).DebugString().c_str(),
                   gssOut.feature_sync_state_summary(2).DebugString().c_str(),
                   gssOut.feature_sync_state_summary(3).DebugString().c_str(),
                   gssOut.feature_sync_state_summary(4).DebugString().c_str(),
                   gssOut.dataset_sync_state_summary(0).DebugString().c_str());
    }

    ccd::GetSyncStateOutput syncStateOutput;
    rc = printExistingCameraDirs(syncStateOutput);
    if(rc!=0) {
        LOG_ERROR("printExistingCameraDirs:%d", rc);
        rv = rc;
    }
    std::string countStr;
    for(int dirIndex=0;
        dirIndex < syncStateOutput.camera_roll_upload_dirs_size();
        ++dirIndex)
    {
        u32 numPhotos = 0;
        char numberStr[32];
        countPhotos(syncStateOutput.camera_roll_upload_dirs(dirIndex),
                    numPhotos);
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr),
                "%d", numPhotos);

        countStr.append(std::string(" UploadPhotos:      "));
        countStr.append(std::string(numberStr));
        countStr.append(std::string("   ("));
        countStr.append(syncStateOutput.camera_roll_upload_dirs(dirIndex));
        countStr.append(std::string(")\n"));
    }

    if(crDownloadSubscribed){
        char numPhotoDownStr[32];
        snprintf(numPhotoDownStr, ARRAY_SIZE_IN_BYTES(numPhotoDownStr),
                 "%d", crDownNumPhotos);

        countStr.append(std::string(" DownloadDsetRoot:  "));
        countStr.append(std::string(numPhotoDownStr));
        countStr.append(std::string("   ("));
        countStr.append(crDownRoot);
        countStr.append(std::string(")\n"));
    }

    // Print out full res photos
    for(int dirIndex=0;
        dirIndex < syncStateOutput.camera_roll_full_res_download_dirs_size();
        ++dirIndex)
    {
        u32 numPhotos = 0;
        char numberStr[32];
        countPhotos(syncStateOutput.camera_roll_full_res_download_dirs(dirIndex).dir(),
                    numPhotos);
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr),
                "%d", numPhotos);

        countStr.append(std::string(" DownFullResPhotos: "));
        countStr.append(std::string(numberStr));
        countStr.append(std::string("   ("));
        countStr.append(syncStateOutput.camera_roll_full_res_download_dirs(dirIndex).dir());
        countStr.append(std::string(") maxFiles("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_full_res_download_dirs(dirIndex).max_files());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") maxSizeBytes("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_full_res_download_dirs(dirIndex).max_size());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") preserve_free_disk_percentage("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_full_res_download_dirs(dirIndex).preserve_free_disk_percentage());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") preserve_free_disk_size_bytes("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), FMTu64,
                 syncStateOutput.camera_roll_full_res_download_dirs(dirIndex).preserve_free_disk_size_bytes());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(")\n"));
    }

    // Print out low res photos
    for(int dirIndex=0;
        dirIndex < syncStateOutput.camera_roll_low_res_download_dirs_size();
        ++dirIndex)
    {
        u32 numPhotos = 0;
        char numberStr[32];
        countPhotos(syncStateOutput.camera_roll_low_res_download_dirs(dirIndex).dir(),
                    numPhotos);
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr),
                "%d", numPhotos);

        countStr.append(std::string(" DownLowResPhotos:  "));
        countStr.append(std::string(numberStr));
        countStr.append(std::string("   ("));
        countStr.append(syncStateOutput.camera_roll_low_res_download_dirs(dirIndex).dir());
        countStr.append(std::string(") maxFiles("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_low_res_download_dirs(dirIndex).max_files());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") maxSizeBytes("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_low_res_download_dirs(dirIndex).max_size());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") preserve_free_disk_percentage("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_low_res_download_dirs(dirIndex).preserve_free_disk_percentage());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") preserve_free_disk_size_bytes("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), FMTu64,
                 syncStateOutput.camera_roll_low_res_download_dirs(dirIndex).preserve_free_disk_size_bytes());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(")\n"));
    }

    // Print out thumbnails
    for(int dirIndex=0;
        dirIndex < syncStateOutput.camera_roll_thumb_download_dirs_size();
        ++dirIndex)
    {
        u32 numPhotos = 0;
        char numberStr[32];
        countPhotos(syncStateOutput.camera_roll_thumb_download_dirs(dirIndex).dir(),
                    numPhotos);
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr),
                "%d", numPhotos);

        countStr.append(std::string(" DownThumbnails:  "));
        countStr.append(std::string(numberStr));
        countStr.append(std::string("   ("));
        countStr.append(syncStateOutput.camera_roll_thumb_download_dirs(dirIndex).dir());
        countStr.append(std::string(") maxFiles("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_thumb_download_dirs(dirIndex).max_files());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") maxSizeBytes("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_thumb_download_dirs(dirIndex).max_size());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") preserve_free_disk_percentage("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), "%d",
                 syncStateOutput.camera_roll_thumb_download_dirs(dirIndex).preserve_free_disk_percentage());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(") preserve_free_disk_size_bytes("));
        snprintf(numberStr, ARRAY_SIZE_IN_BYTES(numberStr), FMTu64,
                 syncStateOutput.camera_roll_thumb_download_dirs(dirIndex).preserve_free_disk_size_bytes());
        countStr.append(std::string(numberStr));
        countStr.append(std::string(")\n"));
    }

    LOG_ALWAYS("Photo Counts:\n%s", countStr.c_str());

    if (target != NULL)
        delete target;
    return rv;
}

int cr_upload_enable_test(bool enable)
{
    u64 userId;
    int rv = 0;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_enable_camera_roll(enable);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set camera roll enable to %d for userId:"FMTu64" err:%d",
                  rv, enable, userId, syncSettingsOut.enable_camera_roll_err());
    }
    return rv;
}

static int deleteAllExceptSyncFolder(const std::string& folderToEmpty)
{
    int rc;
    int rv = 0;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("target is NULL!");
        return -1;
    }

    rc = target->openDir(folderToEmpty);
    if(rc == VPL_ERR_NOENT){
        // Nothing to delete, not an error
        goto end;
    }else if(rc != VPL_OK) {
        LOG_ERROR("Unable to open %s:%d", folderToEmpty.c_str(), rc);
        goto end;
    }

    VPLFS_dirent_t folderDirent;
    while((rc = target->readDir(folderToEmpty, folderDirent)==VPL_OK)) {
        std::string dirent(folderDirent.filename);
        if(dirent=="." || dirent==".." || dirent==".sync_temp") {
            // let's not delete these.
            continue;
        }
        std::string toDelete = folderToEmpty+"/"+dirent;
        rc = target->removeDirRecursive(toDelete);
        if(rc != 0) {
            LOG_ERROR("Problem deleting %s:%d", toDelete.c_str(), rc);
            rv = rc;
        }
    }

    rc = target->closeDir(folderToEmpty);
    if(rc != VPL_OK) {
        LOG_ERROR("Closing %s:%d", folderToEmpty.c_str(), rc);
    }

end:
    if (target != NULL)
        delete target;
    return rv;
}

static const char* TEST_PHOTO_PREFIX = "CRUploadTest";
static const int TEST_PHOTO_NUM_FIELD_WIDTH = 8;

static int parseTestPhotoName(const std::string& photoName, u32& photoNum)
{
    photoNum = 0;
    std::string beginsWith(TEST_PHOTO_PREFIX);
    if(photoName.compare(0, beginsWith.size(), beginsWith) != 0) {
        return -1;
    }
    std::string number = photoName.substr(beginsWith.size(),
                                          TEST_PHOTO_NUM_FIELD_WIDTH);
    int intPhotoNum = atoi(number.c_str());
    if(intPhotoNum < 0) {
        return -2;
    }
    photoNum = (u32)intPhotoNum;
    return 0;
}

static void formTestPhotoName(u32 photoIndex,
                              const std::string& ext,
                              std::string& photoName)
{
    std::stringstream photoNameStream;
    std::stringstream testLength;
    testLength << photoIndex;
    if(testLength.str().size() > TEST_PHOTO_NUM_FIELD_WIDTH) {
        LOG_ERROR("photoIndex %d exceeded field size of %d, setting index to 0",
                  photoIndex, TEST_PHOTO_NUM_FIELD_WIDTH);
        photoIndex = 0;
    }
    photoNameStream << TEST_PHOTO_PREFIX <<
                       std::setfill('0') <<
                       std::setw(TEST_PHOTO_NUM_FIELD_WIDTH) <<
                       photoIndex <<
                       std::string(".") <<
                       ext;
    photoName = photoNameStream.str();
}

static void getHighestPhotoIndexHelper(const std::string& crUploadPath,
                                       u32& highestPhotoIndex)
{
    int rc;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (!target) {
        LOG_ERROR("target is NULL!");
        return;
    }

    rc = target->openDir(crUploadPath);
    if(rc == VPL_ERR_NOENT){
        // no photos
        goto end;
    }else if(rc != VPL_OK) {
        LOG_ERROR("Unable to open %s:%d", crUploadPath.c_str(), rc);
        goto end;
    }

    VPLFS_dirent_t folderDirent;
    while((rc = target->readDir(crUploadPath, folderDirent))==VPL_OK) {
        if(folderDirent.type != VPLFS_TYPE_FILE) {
            continue;
        }

        std::string dirent(folderDirent.filename);
        u32 photoNum;
        rc = parseTestPhotoName(dirent, photoNum);
        if(rc != 0) {
            continue;
        }
        if(photoNum > highestPhotoIndex) {
            highestPhotoIndex = photoNum;
        }
    }

    rc = target->closeDir(crUploadPath);
    if(rc != VPL_OK) {
        LOG_ERROR("Closing %s:%d", crUploadPath.c_str(), rc);
    }

end:
    if (target != NULL)
        delete target;
}

static void getHighestPhotoIndex(const std::string& crUploadPath,
                                 u32& highestPhotoIndex)
{
    highestPhotoIndex = 0;
    getHighestPhotoIndexHelper(crUploadPath, highestPhotoIndex);

    // Check for any other directories we should consider.
    int rc;
    ccd::GetSyncStateInput syncStateIn;
    ccd::GetSyncStateOutput syncStateOut;
    syncStateIn.set_get_camera_roll_upload_dirs(true);

    rc = CCDIGetSyncState(syncStateIn, syncStateOut);
    if(rc != 0)
    {
        LOG_WARN("CCDIGetSyncState:%d, not critical. continuing", rc);
        return;
    }

    for(int dirIndex=0;
        dirIndex < syncStateOut.camera_roll_upload_dirs_size();
        ++dirIndex)
    {
        if(syncStateOut.camera_roll_upload_dirs(dirIndex) == crUploadPath)
        {  //  Already done.
            continue;
        }
        getHighestPhotoIndexHelper(syncStateOut.camera_roll_upload_dirs(dirIndex),
                                   highestPhotoIndex);
    }
}

int cr_upload_photo_test(u32 numPhotos,
                         const std::string& testPhotoPath,
                         const std::string& testPhotoName,
                         const std::string& testPhotoExt,
                         const std::string& tempFolder,
                         const std::string& srcPhotoDir,
                         int deleteAfterMs)
{
    u64 userId;
    int rc;
    int rv = 0;
    TargetDevice *target = NULL;
    std::string finalPhotoNamePath;

    if(numPhotos == 0) {
        return 0;
    }

    // FIXME: this code will only work against local ccd
    std::string absPhotoPath;
    absPhotoPath.assign(testPhotoPath);
    absPhotoPath.append("/");
    absPhotoPath.append(testPhotoName);
    absPhotoPath.append(".");
    absPhotoPath.append(testPhotoExt);

    if(!isPhotoFile(absPhotoPath)) {
        LOG_ERROR("Test file has bad photo extension:%s",
                absPhotoPath.c_str());
        return -1;
    }
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(absPhotoPath.c_str(), &statBuf);
    if(rc != 0) {
        LOG_ERROR("Test file inaccessible%d:%s", rc, absPhotoPath.c_str());
        return rc;
    }

    rc = getUserId(userId);
    if(rc != 0) {
        LOG_ERROR("getUserId:%d", rc);
        return rc;
    }

    LOG_ALWAYS("srcPhotoDir:%s", srcPhotoDir.c_str());

    u32 highestPhotoIndex = 0;
    getHighestPhotoIndex(srcPhotoDir, highestPhotoIndex);

    target = getTargetDevice();
    if(target == NULL) {
        LOG_ERROR("target is NULL!");
        return -1;
    }

    rc = target->createDir(tempFolder, 0777);
    if(rc != 0) {
        LOG_ERROR("Unable to create temp path:%s", tempFolder.c_str());
        rv = rc;
        goto end;
    }

    highestPhotoIndex++;
    u32 photoNum;
    for(photoNum = 0; photoNum < numPhotos; photoNum++, highestPhotoIndex++)
    {
        std::string photoName;
        formTestPhotoName(highestPhotoIndex, testPhotoExt, photoName);

        std::string tempPhotoNamePath = tempFolder + "/" + photoName;

        rc = target->pushFile(absPhotoPath, tempPhotoNamePath);
        if(rc != 0) {
            LOG_ERROR("Copy of %s->%s failed:%d",
                      absPhotoPath.c_str(),
                      tempPhotoNamePath.c_str(),
                      rc);
            rv = rc;
            continue;
        }

        finalPhotoNamePath = srcPhotoDir+"/"+photoName;
        rc = target->renameFile(tempPhotoNamePath, finalPhotoNamePath);
        if(rc != 0) {
            LOG_ERROR("Moving file from %s->%s failed:%d",
                      tempPhotoNamePath.c_str(), finalPhotoNamePath.c_str(),
                      rc);
            rv = rc;
            continue;
        }
    }

    if(deleteAfterMs >= 0) {
        if(deleteAfterMs > 0) {
            VPLThread_Sleep(VPLTIME_FROM_MILLISEC(deleteAfterMs));
        }
        rc = target->deleteFile(finalPhotoNamePath);
        if(rc != 0) {
            LOG_ERROR("removing %s:%d", finalPhotoNamePath.c_str(), rc);
        }
    }

end:
    if (target != NULL)
        delete target;
    return rv;
}

static const char* TEST_LARGE_PHOTO_FILE_EXT = "jpg";

int cr_upload_big_photo_pulse(const std::string& testPhotoPath,
                              int timeMs,
                              int pulseSizeKb)
{
    int rc = 0;
    std::string crUpDir0;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("target is NULL!");
        return -1;
    }

    ccd::GetSyncStateOutput response;
    rc = printExistingCameraDirs(response);
    if(rc != 0) {
        LOG_ERROR("printExistingCameraDirs:%d", rc);
        return rc;
    }

    if(response.camera_roll_upload_dirs_size() == 0) {
        LOG_ERROR("No camera upload directory");
        return -1;
    }
    crUpDir0 = response.camera_roll_upload_dirs(0);
    LOG_ALWAYS("CR Up directory 0:%s", crUpDir0.c_str());

    u32 highestPhotoIndex = 0;
    getHighestPhotoIndex(crUpDir0, highestPhotoIndex);
    highestPhotoIndex++;

    std::string photoName;
    formTestPhotoName(highestPhotoIndex, TEST_LARGE_PHOTO_FILE_EXT, photoName);
    std::string finalPhotoNamePath = crUpDir0+"/"+photoName;

    // Create dst directory
    rc = target->createDir(finalPhotoNamePath, 0777, VPL_FALSE);
    if(rc != 0) {
        LOG_ERROR("Unable to create dst path:%s", finalPhotoNamePath.c_str());
        return rc;
    }

    rc = target->pushFileSlow(testPhotoPath, finalPhotoNamePath, timeMs, pulseSizeKb);
    if(rc != 0) {
        LOG_ERROR("Unable to push file: %s", finalPhotoNamePath.c_str());
        return rc;
    }

    return rc;
}

int get_cr_photo_count(const std::string& directory, u32& numPhotos_out)
{
    countPhotos(directory, numPhotos_out);
    LOG_ALWAYS("PhotoNum: %d", numPhotos_out);
    return 0;
}

int get_cr_photo_count_cloudnode(const std::string& directory, u32& numPhotos_out)
{
    numPhotos_out = 0;

    int rv = 0;
    u64 userId, deviceId;
    std::queue<std::string> dirs;
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id, %d", rv);
        return rv;
    }
    rv = getDeviceId(&deviceId);
    if (rv != 0) {
        LOG_ERROR("Fail to get device id, %d", rv);
        return rv;
    }
    if (!isCloudpc(userId, deviceId)) {

        u64 datasetId = 0;
        std::stringstream ssDatasetId;
        std::string response;
        rv = getDatasetId(userId, "Virt Drive", datasetId);
        if (rv != 0) {
            LOG_ERROR("Fail to get dataset id, %d", rv);
            return rv;
        }

        ssDatasetId << datasetId;
        dirs.push(directory);
        while(!dirs.empty()){
            std::string dirPath = dirs.front();
            dirs.pop();
            rv = fs_test_readdir(userId, ssDatasetId.str(), dirPath, "", response, false);
            if(rv != 0){
                LOG_ERROR("Fail to fs_test_readdir %d", rv);
                return rv;
            }

            cJSON2 *jsonResponse = NULL;
            cJSON2 *fileList  = NULL;
            cJSON2 *file = NULL;
            cJSON2 *fileName = NULL;
            cJSON2 *fileType = NULL;

            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                LOG_ERROR("Unable to parse JSON");
                rv = VPL_ERR_FAIL;
                return rv;
            }
            ON_BLOCK_EXIT(cJSON2_Delete, jsonResponse);

            fileList = cJSON2_GetObjectItem(jsonResponse, "fileList");        
            if (fileList == NULL) {
                LOG_ERROR("fileList not found");
                rv = VPL_ERR_FAIL;
                return rv;
            }

            for(int i=0; i < cJSON2_GetArraySize(fileList); i++){
                file = cJSON2_GetArrayItem(fileList, i);
                if(file == NULL)
                    continue;
                
                fileName = cJSON2_GetObjectItem(file, "name");
                if(fileName == NULL || fileName->type != cJSON2_String){
                    LOG_ERROR("fileName not found");
                    continue;
                }
                fileType = cJSON2_GetObjectItem(file, "type");
                if(fileType == NULL || fileType->type != cJSON2_String){
                    LOG_ERROR("fileType not found");
                    continue;
                }
                
                if(std::string(fileType->valuestring) == "dir"){
                    dirs.push(dirPath + "/" + fileName->valuestring);
                }else if(std::string(fileType->valuestring) == "file"){
                    numPhotos_out++;
                }
            }
        }
    }else{
        LOG_ERROR("CloudPC detected!");
    }
    return 0;
}

int cr_add_upload_dirs(std::vector<std::string>& uploadDirs)
{
    u64 userId;
    int rv;
    int rc;
    ccd::GetSyncStateOutput unused;
    rc = printExistingCameraDirs(unused);
    if(rc != 0) {
        LOG_ERROR("Fail querying current state:%d", rc);
    }

    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    request.set_user_id(userId);
    for(u32 uploadDirIndex = 0; uploadDirIndex < uploadDirs.size(); ++uploadDirIndex)
    {
        request.add_add_camera_roll_upload_dirs(uploadDirs[uploadDirIndex]);
        LOG_ALWAYS("Adding upload directory:(%s)", uploadDirs[uploadDirIndex].c_str());
    }

    rc = CCDIUpdateSyncSettings(request, response);
    if(rc != 0) {
        LOG_ERROR("CCDIUpateSyncSettings:%d, %d",
                  rc, response.add_camera_roll_upload_dirs_err());
        rv = rc;
    }

    return rv;
}

int cr_rm_upload_dirs(std::vector<std::string>& uploadDirs)
{
    u64 userId;
    int rv;
    int rc;
    ccd::GetSyncStateOutput unused;
    rc = printExistingCameraDirs(unused);
    if(rc != 0) {
        LOG_ERROR("Fail querying current state:%d", rc);
    }

    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    request.set_user_id(userId);

    for(u32 uploadDirIndex = 0; uploadDirIndex < uploadDirs.size(); ++uploadDirIndex)
    {
        request.add_remove_camera_roll_upload_dirs(uploadDirs[uploadDirIndex]);
        LOG_ALWAYS("Removing upload directory:(%s)", uploadDirs[uploadDirIndex].c_str());
    }

    rc = CCDIUpdateSyncSettings(request, response);
    if(rc != 0) {
        LOG_ERROR("CCDIUpateSyncSettings:%d, %d",
                  rc, response.remove_camera_roll_upload_dirs_err());
        rv = rc;
    }
    return rv;
}

int cr_get_upload_dir(int index, std::string& uploadDir)
{
    int rc;
    uploadDir.clear();

    ccd::GetSyncStateInput syncStateIn;
    ccd::GetSyncStateOutput syncStateOut;
    syncStateIn.set_get_camera_roll_upload_dirs(true);

    rc = CCDIGetSyncState(syncStateIn, syncStateOut);
    if(rc != 0)
    {
        LOG_ERROR("CCDIGetSyncState:%d", rc);
        return rc;
    }

    if(index < syncStateOut.camera_roll_upload_dirs_size()) {
        uploadDir = syncStateOut.camera_roll_upload_dirs(index);
    }else{
        LOG_ERROR("Cannot get index %d when there are only %d directories returned:"
                  "\n%s",
                  index,
                  syncStateOut.camera_roll_upload_dirs_size(),
                  syncStateOut.DebugString().c_str());
        return -1;
    }

    return 0;
}

int cmd_cr_check_status(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || argc != 1) {
        printf("%s %s - Displays status of PicStream\n", PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    int rc = cr_check_status_test();
    if(rc != 0) {
        LOG_ERROR("cr_check_status_test:%d", rc);
    }
    return rc;
}

int cmd_cr_upload_dirs_add(int argc, const char* argv[])
{
    int rv;
    if(checkHelp(argc, argv) || (argc==1) || (argc > 3)) {
        printf("%s %s <upload dir path> [another upload dir path]"
               " Add CameraRoll upload path(s)\n",
               PICSTREAM_STR, argv[0]);
        return 0;
    }

    std::vector<std::string> addDirs;
    if(argc > 1) {
        addDirs.push_back(std::string(argv[1]));
    }
    if(argc > 2) {
        addDirs.push_back(std::string(argv[2]));
    }
    rv = cr_add_upload_dirs(addDirs);
    if(rv != 0) {
        LOG_ERROR("cr_add_upload_dirs:%d", rv);
    }
    return rv;
}

int cmd_cr_upload_dirs_rm(int argc, const char* argv[])
{
    int rv;
    if(checkHelp(argc, argv) || (argc==1) || (argc > 3)) {
        printf("%s %s <upload dir path> [another upload dir path]"
               " Remove CameraRoll upload path(s)\n",
               PICSTREAM_STR, argv[0]);
        return 0;
    }

    std::vector<std::string> rmDirs;
    if(argc > 1) {
        rmDirs.push_back(std::string(argv[1]));
    }
    if(argc > 2) {
        rmDirs.push_back(std::string(argv[2]));
    }
    rv = cr_rm_upload_dirs(rmDirs);
    if(rv != 0) {
        LOG_ERROR("cr_rm_upload_dirs:%d", rv);
    }
    return rv;
}

int cmd_cr_send_file_to_stream(int argc, const char* argv[])
{
    if(checkHelp(argc, argv) || (argc != 2)) {
        printf("%s %s <absoluteFilePath>"
               " Sends a file to the camera roll stream\n",
               PICSTREAM_STR, argv[0]);
        return 0;
    }

    u64 userId;
    int rv;

    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    request.set_user_id(userId);

    request.set_send_file_to_camera_roll(argv[1]);
    rv = CCDIUpdateSyncSettings(request, response);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d, %d",
                  rv, response.send_file_to_camera_roll_err());
        return rv;
    }
    return rv;
}

int cmd_cr_set_full_res_dl_dir(int argc, const char* argv[])
{
    bool rmDirectory = false;
    std::string directory;
    u32 maxFiles = 0;
    u32 maxBytes = 0;
    u32 preserve_free_disk_percentage = 0;
    u64 preserve_free_disk_size_bytes = 0;
    int currArg = 1;
    if(!checkHelp(argc, argv)) {
        if((argc > currArg+1) && (strcmp(argv[currArg], "-c")==0)) {
            rmDirectory = true;;
            currArg++;
        }
        if(argc > currArg) {
            directory = argv[currArg];
            currArg++;
        }
        if(argc > currArg) {
            maxFiles = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            maxBytes = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            preserve_free_disk_percentage = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            preserve_free_disk_size_bytes = VPLConv_strToU64(argv[currArg++], NULL, 10);
        }
    }

    if(checkHelp(argc, argv) || argc != currArg) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command or wrong number of arguments."
                      " (also, args must appear in the same order)");
        }
        printf("%s %s [-c (rm directory)] <download path> [maxFiles (default 0) [maxBytes (default 0) [preserve_free_disk_percentage (default 0) [preserve_free_disk_size_bytes (default 0)]]]]"
               "  Sets/clears full res download path\n",
               PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    u64 userId;
    int rv;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    request.set_user_id(userId);
    if(rmDirectory) {
        request.set_remove_camera_roll_full_res_download_dir(directory);
    }else{
        ccd::CameraRollDownloadDirSpec* crdds = request.mutable_add_camera_roll_full_res_download_dir();
        crdds->set_dir(directory);
        crdds->set_max_files(maxFiles);
        crdds->set_max_size(maxBytes);
        crdds->set_preserve_free_disk_percentage(preserve_free_disk_percentage);
        crdds->set_preserve_free_disk_size_bytes(preserve_free_disk_size_bytes);
    }

    rv = CCDIUpdateSyncSettings(request, response);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d, add_err:%d, rm_err:%d",
                  rv,
                  response.add_camera_roll_full_res_download_dir_err(),
                  response.remove_camera_roll_full_res_download_dir_err());
        return rv;
    }
    return rv;
}

int cmd_cr_set_low_res_dl_dir(int argc, const char* argv[])
{
    bool rmDirectory = false;
    std::string directory;
    u32 maxFiles = 0;
    u32 maxBytes = 0;
    u32 preserve_free_disk_percentage = 0;
    u64 preserve_free_disk_size_bytes = 0;
    int currArg = 1;
    if(!checkHelp(argc, argv)) {
        if((argc > currArg+1) && (strcmp(argv[currArg], "-c")==0)) {
            rmDirectory = true;;
            currArg++;
        }
        if(argc > currArg) {
            directory = argv[currArg];
            currArg++;
        }
        if(argc > currArg) {
            maxFiles = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            maxBytes = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            preserve_free_disk_percentage = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            preserve_free_disk_size_bytes = VPLConv_strToU64(argv[currArg++], NULL, 10);
        }
    }

    if(checkHelp(argc, argv) || argc != currArg) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command or wrong number of arguments."
                      " (also, args must appear in the same order)");
        }
        printf("%s %s [-c (rm directory)] <download path> [maxFiles (default 0) [maxBytes (default 0) [preserve_free_disk_percentage (default 0) [preserve_free_disk_size_bytes (default 0)]]]]"
               "  Sets/clears low res download path\n",
               PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    u64 userId;
    int rv;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    request.set_user_id(userId);
    if(rmDirectory) {
        request.set_remove_camera_roll_low_res_download_dir(directory);
    }else{
        ccd::CameraRollDownloadDirSpec* crdds = request.mutable_add_camera_roll_low_res_download_dir();
        crdds->set_dir(directory);
        crdds->set_max_files(maxFiles);
        crdds->set_max_size(maxBytes);
        crdds->set_preserve_free_disk_percentage(preserve_free_disk_percentage);
        crdds->set_preserve_free_disk_size_bytes(preserve_free_disk_size_bytes);
    }
    rv = CCDIUpdateSyncSettings(request, response);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d, add_err:%d, rm_err:%d",
                  rv,
                  response.add_camera_roll_low_res_download_dir_err(),
                  response.remove_camera_roll_low_res_download_dir_err());
        return rv;
    }
    return rv;
}

int cmd_cr_list_items(int argc, const char* argv[])
{

    if (checkHelp(argc, argv) || argc != 1) {
        printf("%s %s - Displays All PicStream Items\n", PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    int err;
    std::string criteria;
    std::queue<std::string> albums;
    std::queue<u32> itemCount;
    std::queue<u64> totalSize;
    ccd::CCDIQueryPicStreamObjectsInput req;
    ccd::CCDIQueryPicStreamObjectsOutput resp;

    req.set_filter_field(ccd::PICSTREAM_QUERY_ALBUM);
    err = CCDIQueryPicStreamObjects(req, resp);
    setDebugLevel(LOG_LEVEL_INFO);
    if(resp.mutable_content_objects()->size() > 0) {
        google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject >::iterator it;
        for (it = resp.mutable_content_objects()->begin(); it != resp.mutable_content_objects()->end(); ++it) {

            LOG_DEBUG("Album: %s, Items:"FMTu32" total file size:"FMTu64"\n", 
                                    it->picstream_album().album_name().c_str(),
                                    it->picstream_album().item_count(),
                                    it->picstream_album().item_total_size());

            albums.push(it->picstream_album().album_name());
            itemCount.push(it->picstream_album().item_count());
            totalSize.push(it->picstream_album().item_total_size());
        }
    }

    int albumCnt = 0;
    while(albums.size() > 0) {

        LOG_INFO("Photo Album #%d: name(%s) item_count("FMTu32") item_total_size("FMTu64")", albumCnt++, albums.front().c_str(), itemCount.front(), totalSize.front());

        std::string album = albums.front();
        criteria = "album_name=\"" + album + "\"";
        req.set_filter_field(ccd::PICSTREAM_QUERY_ITEM);
        req.set_search_field(criteria);
        err = CCDIQueryPicStreamObjects(req, resp);

        if(resp.mutable_content_objects()->size() > 0) {

            google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject >::iterator it;
            int idx=0;

            for (it = resp.mutable_content_objects()->begin(); it != resp.mutable_content_objects()->end(); ++it) {

                LOG_INFO("Photo Item #%d:compId(%s) albumName(%s) identifier(%s) title(%s) dateTime("FMTu64") ori_deviceid("FMTu64") "
                         " full_url(%s) low_res_url(%s) thumb_url(%s) file_size("FMTu64")",
                        idx++,
                        it->mutable_pcdo()->comp_id().c_str(),
                        it->mutable_pcdo()->mutable_picstream_item()->album_name().c_str(),
                        it->mutable_pcdo()->mutable_picstream_item()->identifier().c_str(),
                        it->mutable_pcdo()->mutable_picstream_item()->title().c_str(),
                        it->mutable_pcdo()->mutable_picstream_item()->date_time(),
                        it->mutable_pcdo()->mutable_picstream_item()->ori_deviceid(),
                        it->full_res_url().c_str(),
                        it->low_res_url().c_str(),
                        it->thumbnail_url().c_str(),
                        it->mutable_pcdo()->mutable_picstream_item()->file_size()
                );

            }//iterator current album

        }

        albums.pop();
        totalSize.pop();
        itemCount.pop();
    }//albums
    setDebugLevel(LOG_LEVEL_ERROR);
    return 0;
}

int cmd_cr_set_thumb_dl_dir(int argc, const char* argv[])
{
    bool rmDirectory = false;
    std::string directory;
    u32 maxFiles = 0;
    u32 maxBytes = 0;
    u32 preserve_free_disk_percentage = 0;
    u64 preserve_free_disk_size_bytes = 0;
    int currArg = 1;
    if(!checkHelp(argc, argv)) {
        if((argc > currArg+1) && (strcmp(argv[currArg], "-c")==0)) {
            rmDirectory = true;;
            currArg++;
        }
        if(argc > currArg) {
            directory = argv[currArg];
            currArg++;
        }
        if(argc > currArg) {
            maxFiles = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            maxBytes = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            preserve_free_disk_percentage = static_cast<u32>(atoi(argv[currArg++]));
        }
        if(argc > currArg) {
            preserve_free_disk_size_bytes = VPLConv_strToU64(argv[currArg++], NULL, 10);
        }
    }

    if(checkHelp(argc, argv) || argc != currArg) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command or wrong number of arguments."
                      " (also, args must appear in the same order)");
        }
        printf("%s %s [-c (rm directory)] <download path> [maxFiles (default 0) [maxBytes (default 0) [preserve_free_disk_percentage (default 0) [preserve_free_disk_size_bytes (default 0)]]]]"
               "  Sets/clears thumbnail download path\n",
               PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    u64 userId;
    int rv;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    request.set_user_id(userId);
    if(rmDirectory) {
        request.set_remove_camera_roll_thumb_download_dir(directory);
    }else{
        ccd::CameraRollDownloadDirSpec* crdds = request.mutable_add_camera_roll_thumb_download_dir();
        crdds->set_dir(directory);
        crdds->set_max_files(maxFiles);
        crdds->set_max_size(maxBytes);
        crdds->set_preserve_free_disk_percentage(preserve_free_disk_percentage);
        crdds->set_preserve_free_disk_size_bytes(preserve_free_disk_size_bytes);
    }
    rv = CCDIUpdateSyncSettings(request, response);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d, add_err:%d, rm_err:%d",
                  rv,
                  response.add_camera_roll_thumb_download_dir_err(),
                  response.remove_camera_roll_thumb_download_dir_err()
                  );
        return rv;
    }
    return rv;
}

int cmd_start_cr_up(int argc, const char* argv[])
{
    int rv;
    if(checkHelp(argc, argv) || (argc != 1 && argc != 2)) {
        printf("%s %s [-A (uses Android path /sdcard)] (DEPRECATED) Subscribes to CameraRoll upload\n", PICSTREAM_STR, argv[0]);
        return 0;
    }
    bool android = false;
    std::string dashA("-A");
    if(argc==2 && dashA == argv[1]) {
        android = true;
    }

    rv = cr_subscribe_up(android);
    if(rv != 0) {
        LOG_ERROR("cr_subscribe_up:%d", rv);
    }
    return rv;
}

int cmd_start_cr_down(int argc, const char* argv[])
{
    int rv;
    int currArg = 1;
    bool useMaxBytes = false;
    u32 maxBytes = 0;
    bool useMaxFiles = false;
    u32 maxFiles = 0;
    if(!checkHelp(argc, argv)) {
        if(argc > currArg && (strcmp(argv[currArg], "-b")==0)) {
            useMaxBytes = true;
            currArg++;
            maxBytes = static_cast<u32>(atoi(argv[currArg++]));
        }

        if(argc > currArg && (strcmp(argv[currArg], "-f")==0)) {
            useMaxFiles = true;
            currArg++;
            maxFiles = static_cast<u32>(atoi(argv[currArg++]));
        }
    }

    if(argc != currArg || checkHelp(argc, argv)) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
        }
        printf("%s %s [-b <max_bytes>] [-f <max_files>]- (DEPRECATED) Subscribes to CameraRoll download\n",
                       PICSTREAM_STR, argv[0]);
        return 0;
    }

    rv = cr_subscribe_down(std::string(CR_TEST_DOWN_DSET_NAME),
                           maxBytes,
                           useMaxBytes,
                           maxFiles,
                           useMaxFiles);
    if(rv != 0) {
        LOG_ERROR("cr_sbuscribe_down:%d", rv);
    }
    return rv;
}

int cmd_cr_upload_enable(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || argc != 2) {
        printf("%s %s [true|false]\n", PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument %s.  Try 'dxshell %s help' for more info.",
                  argv[1], argv[0]);
        return 0;
    }

    int rv = cr_upload_enable_test(enable);
    if(rv != 0) {
        LOG_ERROR("cr_upload_enable_test(%d):%d", enable, rv);
    }
    return rv;
}

int cmd_cr_global_delete_enable(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || argc != 2) {
        printf("%s %s [true|false] (Global delete default is disabled.)\n", PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument %s.  Try 'dxshell %s help' for more info.",
                  argv[1], argv[0]);
        return 0;
    }


    u64 userId;
    int rv = 0;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_enable_global_delete(enable);

    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set global delete enable to %d for userId:"FMTu64" err:%d",
                  rv, enable, userId, syncSettingsOut.enable_global_delete_err());
    }

    return rv;
}

int cmd_cr_clear(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || argc != 2) {
        printf("%s %s [crDown|crUp|both]\n", PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }
    std::string crDown("crDown");
    std::string crUp("crUp");
    std::string all("both");

    bool clearUploadFolder = false;
    bool clearDownloadFolder = false;

    if(crDown == argv[1]) {
        clearDownloadFolder = true;
    }else if(crUp == argv[1]) {
        clearUploadFolder = true;
    }else if(all == argv[1]) {
        clearDownloadFolder = true;
        clearUploadFolder = true;
    }else {
        LOG_ERROR("Bad argument %s.  Try 'dxshell %s help' for more info.",
                  argv[1], argv[0]);
        return 0;
    }

    int rv = cr_clear_test(clearUploadFolder, clearDownloadFolder);
    if(rv != 0) {
        LOG_ERROR("cr_clear_test(%d,%d):%d",
                  clearUploadFolder, clearDownloadFolder, rv);
    }
    return rv;
}

// choose a folder where renaming from this folder to the final destination
// folder will not result in a cross-volume error.
static std::string getCrTempFolder()
{
    int rc = 0;
    TargetDevice *target = NULL;
    std::string tempFolder;
    std::string separator;

    target = getTargetDevice();
    if (target != NULL) {
        rc = target->getWorkDir(tempFolder);
        if (rc != 0) {
            LOG_ERROR("Failed to get workdir on target device: %d", rc);
            goto exit;
        }
        rc = target->getDirectorySeparator(separator);
        if (rc != 0) {
            LOG_ERROR("Failed to get directory separator on target device: %d", rc);
            goto exit;
        }
        tempFolder.append(separator).append("cr_test_temp_folder");
    }

exit:
    if (target != NULL)
        delete target;
    return tempFolder;
}

int cmd_cr_upload_photo(int argc, const char* argv[])
{
    int rv = 0;
    int currArg = 1;
    int numPhotos = 1;
    int dirIndex = 0;
    std::string fileExtension = "jpg";

    if(!checkHelp(argc, argv)) {
        if((argc > currArg+1) && (strcmp(argv[currArg], "-i")==0)) {
            dirIndex = atoi(argv[++currArg]);
            currArg++;
        }
        if((argc > currArg+1) && (strcmp(argv[currArg], "-e")==0)) {
            fileExtension = argv[++currArg];
            currArg++;
        }
    }

    if(!checkHelp(argc, argv) && (argc == currArg)){
        // do nothing, default numPhotos already set
    }else if(!checkHelp(argc, argv) && (argc == currArg+1)) {
        numPhotos = atoi(argv[currArg]);
    }else{
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command or wrong number of arguments."
                      " (also, args must appear in the same order)");
        }
        printf("%s %s [-i index (default 0)] [-e ext (default jpg)] [numPhotos (default 1)]\n", PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    // FIXME: this code will only work against local ccd
    std::string test_clip_file_name(PICSTREAM_TEST_CLIP_FILE_NAME);
    std::string currDir;
    rv = getCurDir(currDir);
    if (rv < 0) {
        LOG_ERROR("Cannot get current directory");
        return -1;
    }

    std::string tempFolder = getCrTempFolder();

    std::string srcPhotoDir;
    int rc = cr_get_upload_dir(dirIndex, srcPhotoDir);
    if(rc != 0) {
        LOG_ERROR("cr_get_upload_dir:%d, %d", rc, dirIndex);
        return rc;
    }
    LOG_ALWAYS("Uploading %d photos, photoDir:%s, index:%d",
               numPhotos, srcPhotoDir.c_str(), dirIndex);

    rc = cr_upload_photo_test(numPhotos,
                              currDir,
                              test_clip_file_name,
                              fileExtension,
                              tempFolder,
                              srcPhotoDir,
                              -1);
    if(rc != 0) {
        LOG_ERROR("cr_upload_photo_test(%d,%s,%s, %s.%s):%d",
                  numPhotos,
                  currDir.c_str(),
                  test_clip_file_name.c_str(),
                  fileExtension.c_str(),
                  tempFolder.c_str(), rc);
    }
    return rc;
}

int cmd_cr_upload_big_photo_delete(int argc, const char* argv[])
{
    int currArg = 1;
    int timeMs = 100;
    int dirIndex = 0;
    std::string test_large_file_name(PICSTREAM_TEST_CLIP_LARGE_FILE_NAME);
    std::string test_large_file_ext(PICSTREAM_TEST_CLIP_LARGE_FILE_EXT);

    if(!checkHelp(argc, argv)) {
        if((argc > currArg+1) && (strcmp(argv[currArg], "-i")==0)) {
            dirIndex = atoi(argv[++currArg]);
            currArg++;
        }
    }

    if(!checkHelp(argc, argv) && (argc==currArg)){
        // default timeMs already set.
    }else if(!checkHelp(argc, argv) && (argc == currArg+1)) {
        timeMs = atoi(argv[currArg]);
    }else{
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command or wrong number of arguments."
                      " (also, args must appear in the same order)");
        }
        printf("%s %s [-i index (default 0)] [timeMillisec (default 100ms)]  "
               "Photo is deleted after time specified\n",
               PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    std::string tempFolder = getCrTempFolder();

    std::string srcPhotoDir;

    int rc = cr_get_upload_dir(dirIndex, srcPhotoDir);
    if(rc != 0) {
        LOG_ERROR("cr_get_upload_dir:%d, %d", rc, dirIndex);
        return rc;
    }
    LOG_ALWAYS("UploadPhotoDelete, delete after %d ms, srcFolder:%s, index:%d",
               timeMs, srcPhotoDir.c_str(), dirIndex);

    rc = cr_upload_photo_test(1,
                              dxtool_root,
                              test_large_file_name,
                              test_large_file_ext,
                              tempFolder,
                              srcPhotoDir,
                              timeMs);
    if(rc != 0) {
        LOG_ERROR("cr_upload_photo_test(%d,[%s,%s.%s],%s,%d,%d):%d",
                  1,
                  dxtool_root.c_str(),
                  test_large_file_name.c_str(),
                  test_large_file_ext.c_str(),
                  tempFolder.c_str(), true, timeMs, rc);
    }
    return rc;
}

int cmd_cr_upload_big_photo_slow(int argc, const char* argv[])
{
    int rc = 0;
    int currArg = 1;
    int timeMs = 50;
    int pulseSizeKb = 500;
    std::string test_clip_file;
    std::string currDir;

    if(!checkHelp(argc, argv)) {
        if(argc > currArg+1 && (strcmp(argv[currArg], "-t")==0)) {
            currArg++;
            timeMs = atoi(argv[currArg++]);
        }

        if(argc > currArg+1 && (strcmp(argv[currArg], "-p")==0)) {
            currArg++;
            pulseSizeKb = atoi(argv[currArg++]);
        }

        if(argc != currArg) {
            LOG_ERROR("Bad arguments.  Check that arguments are in same order.");
        }
    }

    if (checkHelp(argc, argv) || (argc != currArg)) {
        printf("%s %s [-t timeMillisec (default 50)] [-p pulse size KB (default 500)]\n",
               PICSTREAM_STR, argv[0]);
        return 0;   // No arguments needed
    }

    // FIXME: this code will only work against local ccd

    rc = getCurDir(currDir);
    if (rc < 0) {
        LOG_ERROR("Cannot get current directory");
        return -1;
    }
    test_clip_file.assign(currDir.c_str());
    test_clip_file.append("/");
    test_clip_file.append(PICSTREAM_TEST_CLIP_LARGE_FILE_NAME);
    test_clip_file.append(".");
    test_clip_file.append(PICSTREAM_TEST_CLIP_LARGE_FILE_EXT);

    LOG_INFO("Executing slow photo upload, timeMs:%d, pulseSizeKb:%d",
             timeMs, pulseSizeKb);
    rc = cr_upload_big_photo_pulse(test_clip_file,
                                       timeMs,
                                       pulseSizeKb);
    if(rc != 0) {
        LOG_ERROR("cr_upload_big_photo_pulse(%s,%d,%d):%d",
                  test_clip_file.c_str(),
                  timeMs,
                  pulseSizeKb,
                  rc);
    }
    return rc;
}

int cmd_cr_trigger_upload_dir(int argc, const char* argv[])
{
    if(checkHelp(argc, argv) || (argc != 2)) {
        printf("%s %s <UploadDir>"
               " Trigger picstream upload\n",
               PICSTREAM_STR, argv[0]);
        return 0;
    }

    u64 userId;
    int rv = 0;

    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    LOG_ALWAYS("uploaddir: %s", argv[1]);
    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    request.set_user_id(userId);
    request.set_trigger_camera_roll_upload_dir(argv[1]);
    rv = CCDIUpdateSyncSettings(request, response);
    if (rv != 0) {
        LOG_ERROR("IDxRemoteDeviceProxy::trigger_upload_dir() CCDIUpdateSyncSettings failed: rv = %d, userId ="FMTx64, rv, userId);
    }

    return rv;
}
