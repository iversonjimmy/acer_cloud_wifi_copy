//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "cache.h"

#include "vplex_protobuf_utils.hpp"

#include "ccdi.hpp"

#include "ans_connection.hpp"
#include "ccd_core.h"
#include "ccd_features.h"
#include "ccd_storage.hpp"
#include "config.h"
#include "EventManagerPb.hpp"
#include "gvm_file_utils.hpp"
#include "picstream.hpp"
#include "HttpService.hpp"
#include "DeviceStateCache.hpp"
#include "SyncFeatureMgr.hpp"
#include "virtual_device.hpp"
#include "vsds_query.hpp"
#include "query.h"
#include <map>
#include <list>
#include <set>
#include <algorithm>

#if CCD_ENABLE_STORAGE_NODE
#include "vss_server.hpp"
#include "media_metadata_errors.hpp"
#include "media_metadata_utils.hpp"
#include "MediaMetadataCache.hpp"
#include "MediaMetadataServer.hpp"
#include "scopeguard.hpp"
#include "vplex_serialization.h"
#endif

#include <vpl_fs.h>
#include "cslsha.h"
#include "protobuf_file_reader.hpp"
#include "pin_manager.hpp"

#ifdef IOS
#define PICSTREAM_PATH_REWRITE
#endif // IOS

// enable this for development/testing of Picstream path rewrite on local platform
#if 0
#define PICSTREAM_PATH_REWRITE
#define PICSTREAM_PATH_REWRITE_DEV_PREFIX "/home/fokushi/dxshell_test/cr_test_temp_folder"
#endif

using namespace std;
using namespace ccd;

CachePlayer::CachePlayer() :
    _elevatedPrivilegeEndTime(0),
#if CCD_ENABLE_SYNCDOWN
    syncdown_needs_catchup_notification(false),
#endif
    _upToDate(CCD_USER_DATA_NONE),
    _ansCredentialReplaceTime(0),
    _tempDisableMetadataUpload(false),
    _playerIndex(CACHE_PLAYER_INDEX_NOT_SIGNED_IN),
    _localActivationId(0)
{
}

CachePlayer::~CachePlayer()
{
    // Don't count on this being called; the player array is never destroyed while the Cache is in use.
}

CachedStatEvent* CachePlayer::getStatEvent(const std::string& event_id)
{
    for (int i = 0; i < _cachedData.details().stat_event_list_size(); i++) {
        ccd::CachedStatEvent* curr =
                _cachedData.mutable_details()->mutable_stat_event_list(i);
        if (curr->event_id() == event_id) {
            return curr;
        }
    }
    return NULL;
}

CachedStatEvent& CachePlayer::getOrCreateStatEvent(const std::string& event_id)
{
    CachedStatEvent* existing = getStatEvent(event_id);
 
    if (existing != NULL) {
        return *existing;
    } else {
        ccd::CachedStatEvent* newData = _cachedData.mutable_details()->add_stat_event_list();
        newData->set_app_id("");
        newData->set_event_id(event_id);
        newData->set_start_time_ms(0);
        newData->set_event_count(1);
        
        return *newData;
    }
}

void CachePlayer::initialize(VPLUser_Id_t userId, const char* username, const char* accountId)
{
    _cachedData.mutable_summary()->Clear();
    _cachedData.mutable_summary()->set_user_id((u64)userId);
    set_username(username);
    _cachedData.mutable_summary()->set_account_id(accountId);
}

bool CachePlayer::isLocalDeviceLinked() const
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    u64 thisDeviceId;
    rv = ESCore_GetDeviceGuid(&thisDeviceId);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
        return false;
    }
    return isDeviceLinked(thisDeviceId);
}

const std::string& CachePlayer::localDeviceName() const
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    u64 thisDeviceId;
    rv = ESCore_GetDeviceGuid(&thisDeviceId);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
        return google::protobuf::internal::kEmptyString; // any empty static string will do
    }
    const vplex::vsDirectory::DeviceInfo* deviceInfo =
            Util_FindInDeviceInfoList(thisDeviceId, _cachedData.details().cached_devices());
    if (deviceInfo == NULL) {
        return google::protobuf::internal::kEmptyString; // any empty static string will do
    } else {
        return deviceInfo->devicename();
    }
}

bool CachePlayer::isLocalDeviceStorageNode() const
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    u64 thisDeviceId;
    rv = ESCore_GetDeviceGuid(&thisDeviceId);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
        return false;
    }
    return isDeviceStorageNode(thisDeviceId);
}

void CachePlayer::unloadFromActivePlayerList()
{
    set_player_index(CACHE_PLAYER_INDEX_NOT_SIGNED_IN);
    set_local_activation_id(0);
    _cachedData.Clear();
}

#ifdef PICSTREAM_PATH_REWRITE

static std::string picstream_path_prefix_tag = "[PICSTREAM]";

static void get_picstream_collapsible_path_prefix(std::string &prefix)
{
#ifdef PICSTREAM_PATH_REWRITE_DEV_PREFIX
    prefix.assign(PICSTREAM_PATH_REWRITE_DEV_PREFIX);
    return;
#endif // PICSTREAM_PATH_REWRITE_DEV_PREFIX

#ifdef IOS
    char *homedir = NULL;
    int err = _VPLFS__GetHomeDirectory(&homedir);
    if (err != VPL_OK) {
        LOG_ERROR("Failed to get App home directory: %d", err);
        return;
    }
    prefix.assign(homedir);
#endif // IOS
}

static void collapse_picstream_path(std::string &dir)
{
    std::string prefix;
    get_picstream_collapsible_path_prefix(prefix);
    if (prefix.empty())
        return;

    if (dir.compare(0, prefix.size(), prefix) == 0) {
        dir.replace(0, prefix.size(), picstream_path_prefix_tag);
    }
}

static void expand_picstream_path(std::string &dir)
{
    std::string prefix;
    get_picstream_collapsible_path_prefix(prefix);
    // Even if prefix is empty, execute the rest of the function to remove the tag.

    if (dir.compare(0, picstream_path_prefix_tag.size(), picstream_path_prefix_tag) == 0) {
        dir.replace(0, picstream_path_prefix_tag.size(), prefix);
    }
}

#endif // PICSTREAM_PATH_REWRITE

int CachePlayer::add_camera_upload_dir(const std::string& dir, u32& index_out)
{
    ASSERT(Cache_ThreadHasWriteLock());
    index_out = -1;
    int rv = 0;
    if(dir == "") {
        LOG_ERROR("Cannot set empty directory");
        rv = CCD_ERROR_PARAMETER;
        return rv;
    }

    // 1) First see if the directory already exists
    const ccd::CachedUserDetails& const_details = _cachedData.details();
    for(int iter=0; iter<const_details.camera_upload_dirs_size(); ++iter) {
        std::string directory = const_details.camera_upload_dirs(iter).directory();
#ifdef PICSTREAM_PATH_REWRITE
        expand_picstream_path(directory);
#endif // PICSTREAM_PATH_REWRITE
        if(directory == dir) {
            LOG_WARN("Already exists:%s at index:%d",
                     directory.c_str(),
                     (int)const_details.camera_upload_dirs(iter).index());
            index_out = (u32)const_details.camera_upload_dirs(iter).index();
            rv = CCD_ERROR_ALREADY;
            return rv;
        }
    }
    // 2) Find the lowest index available
    char charMap[MAX_PICSTREAM_DIR] = {0};
    for(int iter = 0; iter<const_details.camera_upload_dirs_size(); ++iter){
        if(const_details.camera_upload_dirs(iter).index() < MAX_PICSTREAM_DIR) {
            charMap[const_details.camera_upload_dirs(iter).index()] = 1;
        }
    }
    int lowestIndex = 0;
    for(; lowestIndex < MAX_PICSTREAM_DIR; ++lowestIndex) {
        if(charMap[lowestIndex] == 0) {
            break;
        }
    }

    if(lowestIndex == MAX_PICSTREAM_DIR) {
        LOG_ERROR("Maximum picstream directories of %d reached, could not add %s",
                  MAX_PICSTREAM_DIR, dir.c_str());
        return CCD_ERROR_TOO_MANY_PATHS;
    }

    // 3) Set the directory to the lowest index, and return the index.
    ccd::CachedUserDetails* details = _cachedData.mutable_details();
    ccd::PicstreamDir* picstreamDir = details->add_camera_upload_dirs();
    index_out = lowestIndex;
    picstreamDir->set_index(lowestIndex);
    std::string dir2 = dir;
#ifdef PICSTREAM_PATH_REWRITE
    collapse_picstream_path(dir2);
#endif // PICSTREAM_PATH_REWRITE
    picstreamDir->set_directory(dir2);
    picstreamDir->set_never_init(true);

    LOG_DEBUG("added %s as %s", dir.c_str(), dir2.c_str());

    return 0;
}

int CachePlayer::remove_camera_upload_dir(const std::string& dir, u32& index_out)
{
    ASSERT(Cache_ThreadHasWriteLock());
    index_out = -1;
    const ccd::CachedUserDetails& const_details = _cachedData.details();
    ccd::CachedUserDetails* details = _cachedData.mutable_details();
    for(int iter=0; iter<const_details.camera_upload_dirs_size(); ++iter) {
        std::string directory = const_details.camera_upload_dirs(iter).directory();
#ifdef PICSTREAM_PATH_REWRITE
        expand_picstream_path(directory);
#endif // PICSTREAM_PATH_REWRITE
        if(directory == dir) {
            LOG_INFO("Removing camera directory %s with index:%d",
                     directory.c_str(),
                     (int)const_details.camera_upload_dirs(iter).index());
            index_out = (u32)const_details.camera_upload_dirs(iter).index();
            details->mutable_camera_upload_dirs()->
                                    SwapElements(iter, details->camera_upload_dirs_size()-1);
            details->mutable_camera_upload_dirs()->RemoveLast();
            return 0;
        }
    }

    LOG_WARN("Could not find %s to remove", dir.c_str());
    return CCD_ERROR_PATH_NOT_FOUND;
}

int CachePlayer::clear_camera_upload_never_init(const std::string& dir,
                                                u32& index_out)
{
    ASSERT(Cache_ThreadHasWriteLock());
    ccd::CachedUserDetails* details = _cachedData.mutable_details();
    int iter;
    for(iter = 0; iter < details->camera_upload_dirs_size(); ++iter)
    {
        std::string directory = details->camera_upload_dirs(iter).directory();
#ifdef PICSTREAM_PATH_REWRITE
        expand_picstream_path(directory);
#endif // PICSTREAM_PATH_REWRITE
        if(directory == dir) {
            details->mutable_camera_upload_dirs(iter)->clear_never_init();
            return 0;
        }
    }
    LOG_WARN("Clearing never_init of non-existent directory:%s", dir.c_str());
    return CCD_ERROR_PATH_NOT_FOUND;
}

int CachePlayer::get_camera_upload_dir(const std::string& dir,
                                       u32& index_out,
                                       bool& neverInit_out) const
{
    ASSERT(Cache_ThreadHasLock());
    index_out = -1;
    neverInit_out = false;
    std::string toReturn;
    const ccd::CachedUserDetails& details = _cachedData.details();
    int iter;
    for(iter = 0; iter < details.camera_upload_dirs_size(); ++iter)
    {
        std::string directory = details.camera_upload_dirs(iter).directory();
#ifdef PICSTREAM_PATH_REWRITE
        expand_picstream_path(directory);
#endif // PICSTREAM_PATH_REWRITE
        if(directory == dir) {
            index_out = (u32)details.camera_upload_dirs(iter).index();
            neverInit_out = details.camera_upload_dirs(iter).never_init();
            return 0;
        }
    }
    LOG_WARN("Camera upload dir does not exist:%s", dir.c_str());
    return CCD_ERROR_PATH_NOT_FOUND;
}

void CachePlayer::get_camera_upload_dirs(
        google::protobuf::RepeatedPtrField<ccd::PicstreamDir>* mutableUploadDirs_out)
{
    ASSERT(Cache_ThreadHasLock());
    const ccd::CachedUserDetails& details = _cachedData.details();
    *mutableUploadDirs_out = details.camera_upload_dirs();
#ifdef PICSTREAM_PATH_REWRITE
    for (int i = 0; i < mutableUploadDirs_out->size(); i++) {
        expand_picstream_path(*mutableUploadDirs_out->Mutable(i)->mutable_directory());
    }
#endif // PICSTREAM_PATH_REWRITE
}

void CachePlayer::get_camera_download_full_res_dirs(google::protobuf::RepeatedPtrField<ccd::CameraRollDownloadDirSpecInternal>& downloadDirs_out)
{
    ASSERT(Cache_ThreadHasLock());
    const ccd::CachedUserDetails& details = _cachedData.details();
    downloadDirs_out = details.picstream_download_dirs_full_res();
#ifdef PICSTREAM_PATH_REWRITE
    for (int i = 0; i < downloadDirs_out.size(); i++) {
        expand_picstream_path(*downloadDirs_out.Mutable(i)->mutable_dir());
    }
#endif // PICSTREAM_PATH_REWRITE
}

void CachePlayer::get_camera_download_low_res_dirs(google::protobuf::RepeatedPtrField<ccd::CameraRollDownloadDirSpecInternal>& downloadDirs_out)
{
    ASSERT(Cache_ThreadHasLock());
    const ccd::CachedUserDetails& details = _cachedData.details();
    downloadDirs_out = details.picstream_download_dirs_low_res();
#ifdef PICSTREAM_PATH_REWRITE
    for (int i = 0; i < downloadDirs_out.size(); i++) {
        expand_picstream_path(*downloadDirs_out.Mutable(i)->mutable_dir());
    }
#endif // PICSTREAM_PATH_REWRITE
}

void CachePlayer::get_camera_download_thumb_dirs(google::protobuf::RepeatedPtrField<ccd::CameraRollDownloadDirSpecInternal>& downloadDirs_out)
{
    ASSERT(Cache_ThreadHasLock());
    const ccd::CachedUserDetails& details = _cachedData.details();
    downloadDirs_out = details.picstream_download_dirs_thumbnail();
#ifdef PICSTREAM_PATH_REWRITE
    for (int i = 0; i < downloadDirs_out.size(); i++) {
        expand_picstream_path(*downloadDirs_out.Mutable(i)->mutable_dir());
    }
#endif // PICSTREAM_PATH_REWRITE
}

void CachePlayer::add_camera_download_full_res_dir(const std::string &dir, u32 max_size, u32 max_files, u32 preserve_free_disk_percentage, u64 preserve_free_disk_size_bytes)
{
    ccd::CameraRollDownloadDirSpecInternal* newEntry =
        _cachedData.mutable_details()->add_picstream_download_dirs_full_res();
    std::string dir2 = dir;
#ifdef PICSTREAM_PATH_REWRITE
    collapse_picstream_path(dir2);
#endif // PICSTREAM_PATH_REWRITE
    newEntry->set_dir(dir2);
    newEntry->set_max_size(max_size);
    newEntry->set_max_files(max_files);
    newEntry->set_preserve_free_disk_percentage(preserve_free_disk_percentage);
    newEntry->set_preserve_free_disk_size_bytes(preserve_free_disk_size_bytes);

    LOG_DEBUG("added %s as %s", dir.c_str(), dir2.c_str());
}

void CachePlayer::add_camera_download_low_res_dir(const std::string &dir, u32 max_size, u32 max_files, u32 preserve_free_disk_percentage, u64 preserve_free_disk_size_bytes)
{
    ccd::CameraRollDownloadDirSpecInternal* newEntry =
        _cachedData.mutable_details()->add_picstream_download_dirs_low_res();
    std::string dir2 = dir;
#ifdef PICSTREAM_PATH_REWRITE
    collapse_picstream_path(dir2);
#endif // PICSTREAM_PATH_REWRITE
    newEntry->set_dir(dir2);
    newEntry->set_max_size(max_size);
    newEntry->set_max_files(max_files);
    newEntry->set_preserve_free_disk_percentage(preserve_free_disk_percentage);
    newEntry->set_preserve_free_disk_size_bytes(preserve_free_disk_size_bytes);

    LOG_DEBUG("added %s as %s", dir.c_str(), dir2.c_str());
}

void CachePlayer::add_camera_download_thumb_dir(const std::string &dir, u32 max_size, u32 max_files, u32 preserve_free_disk_percentage, u64 preserve_free_disk_size_bytes)
{
    ccd::CameraRollDownloadDirSpecInternal* newEntry =
        _cachedData.mutable_details()->add_picstream_download_dirs_thumbnail();
    std::string dir2 = dir;
#ifdef PICSTREAM_PATH_REWRITE
    collapse_picstream_path(dir2);
#endif // PICSTREAM_PATH_REWRITE
    newEntry->set_dir(dir2);
    newEntry->set_max_size(max_size);
    newEntry->set_max_files(max_files);
    newEntry->set_preserve_free_disk_percentage(preserve_free_disk_percentage);
    newEntry->set_preserve_free_disk_size_bytes(preserve_free_disk_size_bytes);

    LOG_DEBUG("added %s as %s", dir.c_str(), dir2.c_str());
}

void CachePlayer::UpdateRecord::wakeupAndClearWaiters(int errCode)
{
    LOG_INFO("Waking "FMTu_size_t" threads with rv %d", waitersInProgress.size(), errCode);
    for (vector<UpdateRecordWaiter*>::iterator iter = waitersInProgress.begin();
         iter != waitersInProgress.end();
         iter++) {
        UpdateRecordWaiter* curr = *iter;
        curr->errCode = errCode;
        VPLSem_Post(&curr->semaphore);
    }
    waitersInProgress.clear();
}

void CachePlayer::UpdateRecord::moveWaiters()
{
    LOG_INFO("Moving "FMTu_size_t" records", waitersNew.size());
    for (vector<UpdateRecordWaiter*>::iterator iter = waitersNew.begin();
         iter != waitersNew.end();
         iter++) {
        UpdateRecordWaiter* curr = *iter;
        waitersInProgress.push_back(curr);
    }
    waitersNew.clear();
}

void CachePlayer::UpdateRecord::wakeUpdaterThread()
{
    // If doUpdateSem is NULL, the thread is just starting up and will figure out that it has
    // work to do instead of waiting, so it is safe to just drop this "post".
    if (doUpdateSem != NULL) {
        LOG_INFO("Waking updater thread");
        VPLSem_Post(doUpdateSem);
        LOG_INFO("Wake updater thread finished");
    }
}

void CachePlayer::UpdateRecord::registerWaiter(UpdateRecordWaiter* waiter)
{
    waitersNew.push_back(waiter);
    // Force it to update now.
    maxNextUpdateTimestamp = nextUpdateTimestamp = VPLTime_GetTimeStamp();
    wakeUpdaterThread();
}

s32 CachePlayer::processUpdates(bool writeNow, const UpdateContext& context,
        list<u64>& removedStorageNodeDeviceIds, list<u64>& changedStorageNodeDeviceIds)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv = 0;
    ASSERT(Cache_ThreadHasWriteLock());
    {
        bool changed = false;

        // Linked Devices
        {
            // Check our existing cached list of devices.
            google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>* cachedList =
                    _cachedData.mutable_details()->mutable_cached_devices();
            for (int i = 0; i < cachedList->size(); i++) {
                vplex::vsDirectory::DeviceInfo& currCache = *cachedList->Mutable(i);
                const vplex::vsDirectory::DeviceInfo* currResponse =
                        Util_FindInDeviceInfoList(currCache.deviceid(), context.cloudInfoOutput.devices());
                if (currResponse == NULL) {
                    // Device was unlinked.  If this was the local device, we will log out the user
                    // after updating their local cache (see #Cache_UpdateCachedUserData()).
                    changed = true;
                    { // Post an event.
                        ccd::CcdiEvent* event = new ccd::CcdiEvent();
                        event->mutable_device_info_change()->set_device_id(currCache.deviceid());
                        event->mutable_device_info_change()->set_change_type(DEVICE_INFO_CHANGE_TYPE_UNLINK);
                        event->mutable_device_info_change()->set_device_name(currCache.devicename());
                        EventManagerPb_AddEvent(event);
                        // event will be freed by EventManagerPb.
                    }
                    { // Remove from the list by replacing it with the last element in the list.
                        cachedList->SwapElements(i, cachedList->size() - 1);
                        cachedList->RemoveLast();
                        i--; // Need to process this index again.
                    }
                } else {
                    // Device still exists.
                    if ((currCache.devicename().compare(currResponse->devicename()) != 0) || 
                        (currResponse->has_featuremediaservercapable() && (currCache.featuremediaservercapable() != currResponse->featuremediaservercapable())) ||
                        (currResponse->has_featurevirtdrivecapable() && (currCache.featurevirtdrivecapable() != currResponse->featurevirtdrivecapable())) ||
                        (currResponse->has_featureremotefileaccesscapable() && (currCache.featureremotefileaccesscapable() != currResponse->featureremotefileaccesscapable())) ||
                        (currResponse->has_featurefsdatasettypecapable() && (currCache.featurefsdatasettypecapable() != currResponse->featurefsdatasettypecapable())) ||
                        (currResponse->has_buildinfo() && (!currCache.has_buildinfo() || currCache.buildinfo().compare(currResponse->buildinfo()) != 0)) ||
                        (currResponse->has_modelnumber() && (!currCache.has_modelnumber() || currCache.modelnumber().compare(currResponse->modelnumber()) != 0)) ||
                        (currResponse->has_osversion() && (!currCache.has_osversion() || currCache.osversion().compare(currResponse->osversion()) != 0)) ||
                        (currCache.protocolversion().compare(currResponse->protocolversion()) != 0)) {
                        // Device name changed.
                        changed = true;
                        currCache.set_devicename(currResponse->devicename());
                        currCache.set_protocolversion(currResponse->protocolversion());
                        if (currResponse->has_featuremediaservercapable()) {
                            currCache.set_featuremediaservercapable(currResponse->featuremediaservercapable());
                        }
                        if (currResponse->has_featurevirtdrivecapable()) {
                            currCache.set_featurevirtdrivecapable(currResponse->featurevirtdrivecapable());
                        }
                        if (currResponse->has_featureremotefileaccesscapable()) {
                            currCache.set_featureremotefileaccesscapable(currResponse->featureremotefileaccesscapable());
                        }
                        if (currResponse->has_featurefsdatasettypecapable()) {
                            currCache.set_featurefsdatasettypecapable(currResponse->featurefsdatasettypecapable());
                        }
                        if (currResponse->has_buildinfo()) {
                            currCache.set_buildinfo(currResponse->buildinfo());
                        }
                        if (currResponse->has_modelnumber()) {
                            currCache.set_modelnumber(currResponse->modelnumber());
                        }
                        if (currResponse->has_osversion()) {
                            currCache.set_osversion(currResponse->osversion());
                        }
                        { // Post an event.
                            ccd::CcdiEvent* event = new ccd::CcdiEvent();
                            event->mutable_device_info_change()->set_device_id(currCache.deviceid());
                            event->mutable_device_info_change()->set_change_type(DEVICE_INFO_CHANGE_TYPE_UPDATE);
                            event->mutable_device_info_change()->set_device_name(currResponse->devicename());
                            event->mutable_device_info_change()->set_protocol_version(currResponse->protocolversion());
                            EventManagerPb_AddEvent(event);
                            // event will be freed by EventManagerPb.
                        }
                        // Also need to tell the SyncConfigs, since a change in protocol version will not
                        // necessarily cause a change to the corresponding UserStorage.
                        SyncFeatureMgr_ReportDeviceAvailability(currCache.deviceid());
                    } else {
                        // No change.
                        LOG_DEBUG("No change for linked device: %s", currCache.ShortDebugString().c_str());
                    }
                }
            }
            // Check if there are any new devices to add to our cache.
            for (int i = 0; i < context.cloudInfoOutput.devices_size(); i++) {
                const vplex::vsDirectory::DeviceInfo& currResponse = context.cloudInfoOutput.devices(i);
                if (!Util_IsInDeviceInfoList(currResponse.deviceid(), _cachedData.details().cached_devices())) {
                    // New device was linked.
                    changed = true;
                    vplex::vsDirectory::DeviceInfo* newDevice = _cachedData.mutable_details()->add_cached_devices();
                    *newDevice = currResponse;
                    { // Post an event.
                        ccd::CcdiEvent* event = new ccd::CcdiEvent();
                        event->mutable_device_info_change()->set_device_id(currResponse.deviceid());
                        event->mutable_device_info_change()->set_change_type(DEVICE_INFO_CHANGE_TYPE_LINK);
                        event->mutable_device_info_change()->set_device_name(currResponse.devicename());
                        event->mutable_device_info_change()->set_protocol_version(currResponse.protocolversion());
                        EventManagerPb_AddEvent(event);
                        // event will be freed by EventManagerPb.
                    }
                }
            }
        }

        // User Storage
        {
            // Update the CachedUserStorage list.
            google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>* cachedUserStorage =
                    _cachedData.mutable_details()->mutable_cached_user_storage();

            // Check if there is any change in CachedUserStorage.
            list<u64> changedUserStorageIds;
            for (int i = 0; i < context.cloudInfoOutput.storageassignments_size(); i++) {
                const vplex::vsDirectory::UserStorage& currResponse = context.cloudInfoOutput.storageassignments(i);
                const vplex::vsDirectory::UserStorage* currCachedUserStorage =
                        Util_FindInUserStorageList(currResponse.storageclusterid(), *cachedUserStorage);
                // When upgrading from 2.3.6 to 2.4.0, currCachedUserStorage will be missing, even if nothing changed on the
                // infra; we need to make sure that we call streamService.add_or_change_stream_server().
                if ((currCachedUserStorage == NULL) || (!Util_IsUserStorageEqual(&currResponse, currCachedUserStorage))) {
                    changedUserStorageIds.push_front(currResponse.storageclusterid());
                }
            }
            // Check our existing cached list.
            // Make appropriate calls when things have changed.
            for (int i = 0; i < cachedUserStorage->size(); i++) {
                const vplex::vsDirectory::UserStorage& currCache = cachedUserStorage->Get(i);
                const vplex::vsDirectory::UserStorage* currResponse =
                        Util_FindInUserStorageList(currCache.storageclusterid(), context.cloudInfoOutput.storageassignments());
                if (currResponse == NULL) {
                    // A StorageNode was deleted.
                    changed = true;
                    { // Post an event.
                        ccd::CcdiEvent* event = new ccd::CcdiEvent();
                        event->mutable_storage_node_change()->set_device_id(currCache.storageclusterid());
                        event->mutable_storage_node_change()->set_change_type(STORAGE_NODE_CHANGE_TYPE_DELETED);
                        EventManagerPb_AddEvent(event);
                        // event will be freed by EventManagerPb.
                    }
                    
                    // Store the device id and process it after the cache lock has been released.
                    removedStorageNodeDeviceIds.push_front(currCache.storageclusterid());
                } else {
                    // The StorageNode still exists.
                    LOG_DEBUG("No change for StorageNode: %s", currCache.ShortDebugString().c_str());
                }
            }
            // Check if there are any new items to add to our cache.
            for (int i = 0; i < context.cloudInfoOutput.storageassignments_size(); i++) {
                const vplex::vsDirectory::UserStorage& currResponse = context.cloudInfoOutput.storageassignments(i);
                if (currResponse.storagetype() == 1) { // Type of storage cluster: 0=datacenter, 1=personal storage node
                    bool snJustAdded = false;

                    // Find mutable cached user storage
                    const vplex::vsDirectory::UserStorage* currCachedUserStorage = Util_FindInUserStorageList(currResponse.storageclusterid(), *cachedUserStorage);

                    if (currCachedUserStorage == NULL) { // currResponse is a new StorageNode
                        changed = true;
                        snJustAdded = true;
                    }

                    if(snJustAdded ||
                       find(changedUserStorageIds.begin(), changedUserStorageIds.end(), currResponse.storageclusterid()) != changedUserStorageIds.end())
                    {   // If just added or storage address changed, update it.
                        changed = true;

                        // Post an event.
                        ccd::CcdiEvent* event = new ccd::CcdiEvent();
                        event->mutable_storage_node_change()->set_device_id(currResponse.storageclusterid());
                        if(snJustAdded) {
                            event->mutable_storage_node_change()->set_change_type(STORAGE_NODE_CHANGE_TYPE_CREATED);
                        }else{
                            event->mutable_storage_node_change()->set_change_type(STORAGE_NODE_CHANGE_TYPE_UPDATED);
                        }
                        EventManagerPb_AddEvent(event);
                        // event will be freed by EventManagerPb.

                        // Store the device id and process it after the cache lock has been released.
                        changedStorageNodeDeviceIds.push_front(currResponse.storageclusterid());
                    }
                }
            }
            
            // Directly overwrite the CachedUserStorage.
            *cachedUserStorage = context.cloudInfoOutput.storageassignments();
        }

        // Field no longer used. Clean up old data
         _cachedData.mutable_details()->clear_subscriptions();

        // Datasets
        std::set<std::string> datasetHash;
        {
            bool datasetChanged = false;
            for(int i=0; i < _cachedData.details().datasets_size(); i++){
                if(_cachedData.details().datasets(i).has_details_hash())
                    datasetHash.insert(_cachedData.details().datasets(i).details_hash());
            }
            if(_cachedData.details().datasets_size() != context.cloudInfoOutput.datasets_size()){
                LOG_INFO("dataset size changed,%d to %d", _cachedData.details().datasets_size(),
                                                          context.cloudInfoOutput.datasets_size());
                datasetChanged = true;
                changed = true;
            }
            _cachedData.mutable_details()->clear_datasets();

            ccd::CcdiEvent *ep = new ccd::CcdiEvent();
            ccd::EventDatasetChange *cp = ep->mutable_dataset_change();

            for (int i = 0; i < context.cloudInfoOutput.datasets_size(); i++) {
                CachedDataset* newDataset = _cachedData.mutable_details()->add_datasets();
                *(newDataset->mutable_details()) = context.cloudInfoOutput.datasets(i);
                std::string dataset;
                std::string hashcode;
                context.cloudInfoOutput.datasets(i).SerializeToString(&dataset);
                int hash_rc = Util_CalcHash(dataset.data(), dataset.size(), /*out*/ hashcode);
                if (hash_rc < 0) {
                    LOG_ERROR("Util_CalcHash failed: %d", hash_rc);
                    // Continue anyway.
                }

                newDataset->set_details_hash(hashcode);

                cp->add_dataset_id(context.cloudInfoOutput.datasets(i).datasetid());

                if(datasetHash.find(newDataset->details_hash()) == datasetHash.end()){
                    LOG_INFO("Dataset added or modified: %s", newDataset->details().DebugString().c_str());
                    datasetChanged = true;
                    changed = true;
#if CCD_ENABLE_STORAGE_NODE
                    if (_cachedData.details().syncbox_sync_settings().is_archive_storage()) {
                        if (newDataset->details().datasetname() == GetLocalArchiveStorageSyncboxDatasetName()) {
                            // This is the Syncbox dataset that was created by the local device.
                            if (_cachedData.details().has_local_syncbox_archive_storage_dataset_id() == false ||
                                _cachedData.details().local_syncbox_archive_storage_dataset_id() != newDataset->details().datasetid()) {
                                // We only want to allowArchiveStorageSyncboxCreateDb when the dataset is first created.
                                LOG_INFO("SyncBox dataset is just created, now allow creating DB of SyncBox.");
                                _cachedData.mutable_details()->set_local_syncbox_archive_storage_dataset_id(newDataset->details().datasetid());
                                _cachedData.mutable_details()->set_allow_syncbox_archive_storage_create_db(true);
                            }
                        }
                    }
#endif
                }
            }

            if(datasetChanged){
                EventManagerPb_AddEvent(ep);
                // EventManager will delete ep.
            }else{
                delete ep;
            }
        }

#if CCD_ENABLE_STORAGE_NODE
        // Update Syncbox settings coherent flag. If local setting is different from cached remote setting,
        // coherent flag is set to false.
        if (_cachedData.details().syncbox_sync_settings().is_archive_storage()) {
            // Need to skip this block if we tried to update the Archive
            // Storage Device association since the beginning of the GetCloudInfo call.
            if (!Cache_DidWeTryToUpdateSyncboxArchiveStorage(context.updateSyncboxArchiveStorageSeqNum)) {
                const vplex::vsDirectory::DatasetDetail* syncboxDataset = Util_FindSyncboxArchiveStorageDataset(_cachedData.details());
                if (syncboxDataset != NULL) {
                    if (Util_IsLocalDeviceArchiveStorage(*syncboxDataset)) {
                        LOG_INFO("Local device is still Syncbox Archive Storage.");
                        _cachedData.mutable_details()->mutable_syncbox_sync_settings()->set_coherent(true);
                    } else {
                        // This condition can be caused by another device activating itself as a new Syncbox server.
                        LOG_INFO("We are no longer Syncbox Archive Storage, dataset="FMTu64, syncboxDataset->datasetid());
                        _cachedData.mutable_details()->mutable_syncbox_sync_settings()->set_coherent(false);
                    }
                } else {
                    LOG_WARN("Syncbox dataset not found, we are no longer Syncbox Archive Storage.");
                    _cachedData.mutable_details()->mutable_syncbox_sync_settings()->set_coherent(false);
                }
            }
        }
#endif

        // Write to disk (only if needed).
        if (changed && writeNow) {
            rv = writeCachedData(false);
            if (rv != 0) {
                LOG_ERROR("%s failed: %d", "writeCachedData", rv);
                // Failed to write, but we already updated the user in memory, so we should continue anyway.
                rv = 0;
            }
        }
    }
    return rv;
}

const vplex::vsDirectory::DatasetDetail* CachePlayer::getDatasetDetail(u64 datasetId)
{
    const ccd::CachedUserDetails& userDetails = _cachedData.details();
    for (int i = 0; i < userDetails.datasets_size(); i++) {
        if (userDetails.datasets(i).details().datasetid() == datasetId) {
            return &(userDetails.datasets(i).details());
        }
    }
    return NULL;
}

#if CCD_ENABLE_STORAGE_NODE
int CachePlayer::getLocalDeviceStorageDatasetId(u64& datasetId_out) const
{
    // Util_GetDeviceStorageDatasetId will always assign datasetId_out.
    return Util_GetDeviceStorageDatasetId(_cachedData.details(),
            VirtualDevice_GetDeviceId(), /*out*/datasetId_out);
}
#endif

s32 CachePlayer::readCachedData(VPLUser_Id_t userId)
{
    s32 rv = readCachedUserData(userId, *(_cachedData.mutable_summary()), _cachedData.mutable_details());
    if (rv < 0) {
        _cachedData.Clear();
    }
    return rv;
}

#if CCD_ENABLE_STORAGE_NODE

/// A #vss_server::msaGetObjectMetadataFunc_t
MMError CachePlayer::MSAGetObjectMetadata(const media_metadata::GetObjectMetadataInput& input,
        const std::string &collectionId,
        media_metadata::GetObjectMetadataOutput& output, void* nullContext)
{
    s32 rv;
    output.Clear();
    {
        u64 thisDeviceId;
        string objectId;
        rv = ESCore_GetDeviceGuid(&thisDeviceId);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
            goto out;
        }
        if (input.has_url()) {
            // NOTE: These all need to stay in sync:
            // - #MMGetDeviceFromUrl()
            // - #MMParseUrl()
            // - #MSAGetObjectMetadata()
            // .
            const string token("/mm/");
            size_t currPos = input.url().find(token);
            if (currPos == string::npos) {
                LOG_ERROR("Invalid URL \"%s\"", input.url().c_str());
                rv = MM_INVALID_URL;
                goto out;
            }
            currPos += token.size();
            // Confirm that the next 16 characters are the correct deviceId and '/' comes after
            // that.
            if (((currPos + 16) >= input.url().size()) || (input.url()[currPos+16] != '/')) {
                LOG_ERROR("Invalid URL \"%s\"", input.url().c_str());
                rv = MM_INVALID_URL;
                goto out;
            }
            string expectedDeviceId = media_metadata::MMDeviceIdToString(thisDeviceId);
            if (input.url().compare(currPos, 16, expectedDeviceId) != 0) {
                LOG_ERROR("Wrong device in \"%s\" (expected %s)",
                        input.url().c_str(), expectedDeviceId.c_str());
                rv = MM_INVALID_URL;
                goto out;
            }
            // 16 (device ID) + 1 ('/') + 1 (content/thumbnail tag) + 1 ('/')
            currPos += 19;
            size_t encodedLen = input.url().size() - currPos;
            size_t decodedLen = VPL_BASE64_DECODED_MAX_BUF_LEN(encodedLen);
            char* decoded = (char*)malloc(decodedLen);
            ON_BLOCK_EXIT(free, decoded);
            VPL_DecodeBase64((input.url().c_str() + currPos), encodedLen, decoded, &decodedLen);
            objectId.assign(decoded, decodedLen);
        } else if (input.has_object_id()) {
            objectId = input.object_id();
        } else {
            LOG_ERROR("Must specify either url or object_id");
            rv = MM_ERR_INVALID;
            goto out;
        }

        {
            // Bug 10504: Query the metadata object from DB first.
            // The API will still fallback to parsing the metadata files if the query fails for any reason.
            int rc = 0;
            if (input.has_catalog_type()) {
                rc = MSAGetContentObjectMetadata(objectId, collectionId, input.catalog_type(), output);
            }else {
                rc = MSAGetContentObjectMetadata(objectId, collectionId, (media_metadata::CatalogType_t)0, output);
            }
            if (rc != 0) {
                LOG_WARN("Object \"%s\", collection:%s, cat:(%d,%d) "
                         "was not found by MSAGetContentObjectMetadata:%d",
                         objectId.c_str(), collectionId.c_str(),
                         input.has_catalog_type(), (int)input.catalog_type(), rc);
                rv = rc;
            }
        }
    }
 out:
    return rv;
}

int CachePlayer::setupStorageNode()
{
    int rv;
    LOG_INFO("Setting up storage node for user "FMT_VPLUser_Id_t, user_id());
    {
        u64 deviceId;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
            goto out;
        }

        char vsdsHostname[HOSTNAME_MAX_LENGTH];
        CCD_GET_INFRA_CLUSTER_HOSTNAME(vsdsHostname, cluster_id());

        vplex::vsDirectory::SessionInfo vsdsSession;
        Query_FillInVsdsSession(&vsdsSession, getSession());

        vss_query queryContext;
        queryContext.init(vsdsHostname, __ccdConfig.vsdsContentPort,
                deviceId, user_id(), vsdsSession);

        // Automatically register this storage cluster.
        // This should only be done once on storageNode installation.
        {
            bool virtDriveCapable;
            bool mediaServerCapable;
            bool remoteFileAccessCapable;
            bool fsDatasetTypeCapable;
            bool myStorageServerCapable;
#if defined(CLOUDNODE)
            virtDriveCapable = true;
            mediaServerCapable = true;
            remoteFileAccessCapable = true;
            fsDatasetTypeCapable = false;
            myStorageServerCapable = true;
#else // CLOUDPC
            virtDriveCapable = false;
            mediaServerCapable = true;
            remoteFileAccessCapable = true;
            fsDatasetTypeCapable = true;
            myStorageServerCapable = true;
#endif // CLOUDPC
            rv = queryContext.registerStorageNode(virtDriveCapable,
                mediaServerCapable, remoteFileAccessCapable, fsDatasetTypeCapable,
                myStorageServerCapable);
        }
        if(rv != 0 && rv != VPL_VS_DIRECTORY_ERR_DUPLICATE_CLUSTERID)  {
            LOG_ERROR("Failed to register storage cluster for user "FMT_VPLUser_Id_t, user_id());
            goto out;
        }

        // Automatically add this storage for the owning user.
        // This should only be done when a valid user requests the action.
        rv = queryContext.addUserStorageNode();
        if(rv != 0)  {
            LOG_ERROR("Failed to add storage node for user "FMT_VPLUser_Id_t, user_id());
            goto out;
        }

        // Update the enabled features.
        {
            bool setVirtDrive;
            bool enableVirtDrive;
            bool setMediaServer;
            bool enableMediaServer;
            bool setRemoteFileAccess;
            bool enableRemoteFileAccess;
            bool setFsDatasetTypeSupport;
            bool enableFsDatasetTypeSupport;
#if defined(CLOUDNODE)
            setVirtDrive = true;
            enableVirtDrive = true;
            setMediaServer = true;
            enableMediaServer = true;
            setRemoteFileAccess = true;
            enableRemoteFileAccess = true;
            setFsDatasetTypeSupport = true;
            enableFsDatasetTypeSupport = false;
#else // CLOUDPC
            setVirtDrive = false;
            enableVirtDrive = false;
            setMediaServer = true;
            enableMediaServer = false;
            setRemoteFileAccess = true;
            enableRemoteFileAccess = false;
            setFsDatasetTypeSupport = true;
            enableFsDatasetTypeSupport = true;
#endif // CLOUDPC
            // NOTE: We could call the function up above but we've
            // already got the queryContext set up...
            rv = queryContext.updateFeatures(setMediaServer, enableMediaServer,
                setVirtDrive, enableVirtDrive,
                setRemoteFileAccess, enableRemoteFileAccess,
                setFsDatasetTypeSupport, enableFsDatasetTypeSupport);
        }
        if (rv != 0) {
            LOG_ERROR("Failed to update storage node features for user "FMT_VPLUser_Id_t, user_id());
            goto out;
        }
    }
out:
   return rv;
}
#endif
