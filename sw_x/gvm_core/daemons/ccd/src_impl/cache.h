//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CACHE_H__
#define __CACHE_H__
// TODO: rename to cache.hpp

#include "vpl_types.h"
#include "vpl_time.h"
#include "AutoLocks.hpp"
#include "ccd_util.hpp"
#include "ccd_features.h"
#include "SyncConfig.hpp"
#include <vplex_ias_service_types.pb.h>
#include <vplex_vs_directory_service_types.pb.h>
#include <vplex_sync_agent_notifier.pb.h>
#include <google/protobuf/repeated_field.h>
#include <ccdi_rpc.pb.h>
#if CCD_ENABLE_STORAGE_NODE
#include "media_metadata_errors.hpp"
#endif
#include <sstream>
#include "HttpService.hpp"
#include "ts_client.hpp"
#if CCD_USE_SHARED_CREDENTIALS
#include "vplex_shared_credential.hpp"
#endif

namespace Ts2 {
    class LocalInfo;
}

class vss_server;

/// This should be considered the client's best guess at what the server state is.

struct CachedUserDataFlags
{
    // The default copy and assign operations are okay for this object.
    bool datasets;
    bool subscriptions;
    bool linkedDevices;
    bool storageNodes;
    bool communityData;
    
    CachedUserDataFlags() :
        datasets(false),
        subscriptions(false),
        linkedDevices(false),
        storageNodes(false),
        communityData(false) {}

    CachedUserDataFlags(
            bool datasets,
            bool subscriptions,
            bool linkedDevices,
            bool storageNodes,
            bool communityData) :
        datasets(datasets),
        subscriptions(subscriptions),
        linkedDevices(linkedDevices),
        storageNodes(storageNodes),
        communityData(communityData) {}

    CachedUserDataFlags(const CachedUserDataFlags& other) :
        datasets(other.datasets),
        subscriptions(other.subscriptions),
        linkedDevices(other.linkedDevices),
        storageNodes(other.storageNodes),
        communityData(other.communityData)
    {}

    void set(const CachedUserDataFlags& flags) {
        datasets |= flags.datasets;
        subscriptions |= flags.subscriptions;
        linkedDevices |= flags.linkedDevices;
        storageNodes |= flags.storageNodes;
        communityData |= flags.communityData;
    }
    
    void unset(const CachedUserDataFlags& flags) {
        datasets &= !flags.datasets;
        subscriptions &= !flags.subscriptions;
        linkedDevices &= !flags.linkedDevices;
        storageNodes &= !flags.storageNodes;
        communityData &= !flags.communityData;
    }
    
    int numSet() const {
        return (datasets ? 1 : 0) +
                (subscriptions ? 1 : 0) +
                (linkedDevices ? 1 : 0) +
                (storageNodes ? 1 : 0) +
                (communityData ? 1 : 0);
    }

    void debugString(std::ostringstream& str) const {
        str << "{ " <<
                (datasets ? "dsets " : "") <<
                (subscriptions ? "subs " : "") <<
                (linkedDevices ? "devices " : "") <<
                (storageNodes ? "psns " : "") <<
                //(communityData ? "comm " : "") <<
                "}";
    }
    std::string debugString() const {
        std::ostringstream str;
        debugString(str);
        return str.str();
    }
};

static const CachedUserDataFlags CCD_USER_DATA_NONE(false, false, false, false, false);
static const CachedUserDataFlags CCD_USER_DATA_DATASETS_ONLY(true, false, false, false, false);
static const CachedUserDataFlags CCD_USER_DATA_SUBSCRIPTIONS_ONLY(false, true, false, false, false);
static const CachedUserDataFlags CCD_USER_DATA_UPDATE_SUBSCRIPTIONS(true, true, true, false, false);
static const CachedUserDataFlags CCD_USER_DATA_UPDATE_DEVICES(false, false, true, true, false);
static const CachedUserDataFlags CCD_USER_DATA_LINKED_DEVICES_ONLY(false, false, true, false, false);
static const CachedUserDataFlags CCD_USER_DATA_STORAGE_NODES_ONLY(false, false, false, true, false);
static const CachedUserDataFlags CCD_USER_DATA_VSDS(true, true, true, true, false);
static const CachedUserDataFlags CCD_USER_DATA_ALL(true, true, true, true, true);

struct UpdateContext
{
    ccd::UserSession session;
    VPLUser_Id_t userId;
    u64 deviceId;
    u32 localActivationId;
    bool wasLocalDeviceStorageNode;
#if CCD_ENABLE_STORAGE_NODE
    /// For use with #Cache_DidWeTryToUpdateSyncboxArchiveStorage().
    u32 updateSyncboxArchiveStorageSeqNum;
#endif

    vplex::vsDirectory::GetCloudInfoOutput cloudInfoOutput;
};

static const int CACHE_PLAYER_INDEX_NOT_SIGNED_IN = -1;
static const int ACTIVATION_ID_NONE = 0;

class CachePlayer
{
 public:
    CachePlayer();
    
    ~CachePlayer();
    
    const std::string& account_id() const { return _cachedData.summary().account_id(); }

    /// The infra cluster for the user, given to us by the login server.
    s64 cluster_id() const { return _cachedData.summary().cluster_id(); }

    bool datasets_up_to_date() const { return _upToDate.datasets; }

    /// Variables used to determine when syncing is allowed
    bool get_auto_sync() const
            { return !_cachedData.details().disable_auto_sync(); }
    bool get_background_data() const
            { return !_cachedData.details().disable_background_data(); }
    bool get_mobile_network_data() const
            { return !_cachedData.details().disable_mobile_network_data(); }
    bool get_enable_camera_roll_trigger() const
            { return _cachedData.details().enable_camera_roll_trigger(); }
    /// dir_out is "" if there is no record; otherwise the record is valid.
    int get_camera_upload_dir(const std::string& dir,
                              u32& index_out,
                              bool& neverInit_out) const;
    void get_camera_upload_dirs(
            google::protobuf::RepeatedPtrField<ccd::PicstreamDir>* mutableUploadDirs);
    
    void get_camera_download_full_res_dirs(google::protobuf::RepeatedPtrField<ccd::CameraRollDownloadDirSpecInternal>& downloadDirs);
    void get_camera_download_low_res_dirs(google::protobuf::RepeatedPtrField<ccd::CameraRollDownloadDirSpecInternal>& downloadDirs);
    void get_camera_download_thumb_dirs(google::protobuf::RepeatedPtrField<ccd::CameraRollDownloadDirSpecInternal>& downloadDirs);

    void clear_camera_download_full_res_dirs() { _cachedData.mutable_details()->clear_picstream_download_dirs_full_res(); _cachedData.mutable_details()->set_picstream_storage_conservation_dropping_mode(false);}
    void clear_camera_download_low_res_dirs() { _cachedData.mutable_details()->clear_picstream_download_dirs_low_res();   _cachedData.mutable_details()->set_picstream_storage_conservation_dropping_mode(false);}
    void clear_camera_download_thumb_dirs() { _cachedData.mutable_details()->clear_picstream_download_dirs_thumbnail();   _cachedData.mutable_details()->set_picstream_storage_conservation_dropping_mode(false);}

    void add_camera_download_full_res_dir(const std::string &dir, u32 max_size, u32 max_files, u32 preserve_free_disk_percentage, u64 preserve_free_disk_size_bytes);
    void add_camera_download_low_res_dir(const std::string &dir, u32 max_size, u32 max_files, u32 preserve_free_disk_percentage, u64 preserve_free_disk_size_bytes);
    void add_camera_download_thumb_dir(const std::string &dir, u32 max_size, u32 max_files, u32 preserve_free_disk_percentage, u64 preserve_free_disk_size_bytes);

    /// If not found, returns NULL.
    const vplex::vsDirectory::DatasetDetail* getDatasetDetail(u64 datasetId);

#if CCD_ENABLE_STORAGE_NODE
    int getLocalDeviceStorageDatasetId(u64& datasetId_out) const;
#endif

    /// Specifies if a user is occupying this player slot.  A user can
    /// be signed-in with or without a network connection.
    VPL_BOOL isSignedIn() const { return (_playerIndex != CACHE_PLAYER_INDEX_NOT_SIGNED_IN); }

    /// 
    void initialize(VPLUser_Id_t userId, const char* username, const char* accountId);

    bool isDeviceLinked(u64 deviceId) const { return Util_IsInDeviceInfoList(deviceId, _cachedData.details().cached_devices()); }

    bool isDeviceStorageNode(u64 deviceId) const { return Util_IsInUserStorageList(deviceId, _cachedData.details().cached_user_storage()); }

    bool isLocalDeviceLinked() const;

    bool isLocalDeviceStorageNode() const;
    
    /// For a particular instance of CCD, we will count local "activate user" operations to avoid
    /// old threads from modifying a user after they log-out and log-in again.
    /// 0 indicates that the CachePlayer slot is not logged-in.
    u32 local_activation_id() const { return _localActivationId; }
    
    const std::string& localDeviceName() const;

    int player_index() const { return _playerIndex; }

    /// Update this user with the specified pieces of data from the network.
    /// @note Must hold cache lock for write when calling this.
    s32 processUpdates(bool writeNow,
            const UpdateContext& context,
            list<u64>& removedStorageNodeDeviceIds_out,
            list<u64>& changedStorageNodeDeviceIds_out);
    
    /// Populate the specified #CachedUserSummary (and optionally #CachedUserDetails) from
    /// \a userId's disk cache.
    /// @param details_out Can be NULL if not needed.
    static s32 readCachedUserData(VPLUser_Id_t userId,
            ccd::CachedUserSummary& summary_out,
            ccd::CachedUserDetails* details_out);
    
    /// Populate this CachePlayer object from @a userId's disk cache.
    s32 readCachedData(VPLUser_Id_t userId);
    
    u64 session_handle() const { return getSession().session_handle(); }

    /// @note Don't forget to write the changes to disk.
    void set_auto_sync(bool auto_sync)
            {_cachedData.mutable_details()->set_disable_auto_sync(!auto_sync);}

    /// @note Don't forget to write the changes to disk.
    void set_background_data(bool background_data)
            {_cachedData.mutable_details()->set_disable_background_data(!background_data);}

    /// @note Don't forget to write the changes to disk.
    void set_mobile_network_data(bool mobile_network_data)
            {_cachedData.mutable_details()->set_disable_mobile_network_data(!mobile_network_data);}

    /// @note Don't forget to write the changes to disk.
    void set_enable_camera_roll_trigger(bool enable)
            {_cachedData.mutable_details()->set_enable_camera_roll_trigger(enable);}
    /// @note Don't forget to write the changes to disk.
    int add_camera_upload_dir(const std::string& dir, u32& index_out);
    /// @note Don't forget to write the changes to disk.
    int remove_camera_upload_dir(const std::string& dir, u32& index_out);
    /// @note Don't forget to write the changes to disk.
    int clear_camera_upload_never_init(const std::string& dir, u32& index_out);

    void set_local_activation_id(s32 activationId) { _localActivationId = activationId; }

    void set_player_index(int playerIndex) { _playerIndex = playerIndex; }
    
    void set_username(const char* username) { _cachedData.mutable_summary()->set_username(username); }

    int setupStorageNode();

    bool storage_nodes_up_to_date() const { return _upToDate.storageNodes; }

    bool subscriptions_up_to_date() const { return _upToDate.subscriptions; }

    void unloadFromActivePlayerList();
    
    VPLUser_Id_t user_id() const { return (VPLUser_Id_t)_cachedData.summary().user_id(); }
    
    const std::string& username() const { return _cachedData.summary().username(); }
    
    /// Write this CachePlayer object to username()'s disk cache.
    s32 writeCachedData(bool isUserCredChange);
    
    ccd::CachedStatEvent* getStatEvent(const std::string& event_id);

    ccd::CachedStatEvent& getOrCreateStatEvent(const std::string& event_id);

    VPLTime_t _elevatedPrivilegeEndTime;

#if CCD_ENABLE_SYNCDOWN
    bool syncdown_needs_catchup_notification;
#endif

    //-----------------------------------
    // "CacheMonitor" stuff
    //-----------------------------------

    CachedUserDataFlags _upToDate;

    struct UpdateRecordWaiter {
        VPLSem_t semaphore;
        int errCode;
    };

    /// This is the data for a user's update-from-infra thread.
    struct UpdateRecord {

        /// Timestamp indicating when to perform the next update.  Can be extended up to
        /// maxNextUpdateTimestamp if there is more activity.
        /// #VPLTIME_INVALID indicates that no update is queued.
        VPLTime_t nextUpdateTimestamp;
        VPLTime_t maxNextUpdateTimestamp;

        VPLSem_t* doUpdateSem;

        /// If items are marked dirty while the updater thread is calling the infra, we will need
        /// to do another update before we can set _upToDate.
        CachedUserDataFlags willBeUpToDate;

        std::vector<UpdateRecordWaiter*> waitersInProgress;

        std::vector<UpdateRecordWaiter*> waitersNew;

        void wakeUpdaterThread();

        /// Called by a thread that wants to wait for an update.
        void registerWaiter(UpdateRecordWaiter* waiter);

        /// Called by the updater thread at the beginning of an update, to move all waitersNew to
        /// waitersInProgress.
        void moveWaiters();

        /// Called by the updater thread at the end of an update, to wake all waitersInProgress.
        void wakeupAndClearWaiters(int errCode);

        void init() {
            nextUpdateTimestamp = VPLTIME_INVALID;
            maxNextUpdateTimestamp = VPLTIME_INVALID;
            doUpdateSem = NULL;
        }

        void cleanup() {
            moveWaiters();
            wakeupAndClearWaiters(CCD_ERROR_NOT_SIGNED_IN);
            wakeUpdaterThread();
        }
    };

    UpdateRecord _vsdsUpdateThreadState;

    //-----------------------------------

    // These should be considered private, but for purposes of efficiency, they
    // are exposed and need to be refactored later.
 //private:
    /// @note Don't forget to write the changes to disk if you modify this.
    ccd::CachedUserData _cachedData;
    
    ccd::UserSession& getSession() { return *_cachedData.mutable_details()->mutable_session(); }
    
    const ccd::UserSession& getSession() const { return _cachedData.details().session(); }

    /// Timestamp after which we will allow asking IAS for replacement ANS credentials.
    /// This is to safe-guard against a flood of requests if the infra becomes misconfigured such
    /// that ANS always rejects the latest credentials from IAS.
    VPLTime_t _ansCredentialReplaceTime;

    bool _tempDisableMetadataUpload;

#if CCD_ENABLE_STORAGE_NODE
    static MMError MSAGetObjectMetadata(
            const media_metadata::GetObjectMetadataInput& input,
            const std::string &collectionId,
            media_metadata::GetObjectMetadataOutput& output,
            void* nullContext);
#endif

 private:
    // -1 if not signed in
    int _playerIndex;
    
    u32 _localActivationId;
};

s32 Cache_Clear(void);

s32 Cache_CreateNotesDatasetIfNeeded(u32 activationId);
s32 Cache_CreateSbmDatasetIfNeeded(u32 activationId);
s32 Cache_CreateSwmDatasetIfNeeded(u32 activationId);

#if CCD_ENABLE_STORAGE_NODE
s32 Cache_DeleteSyncboxDataset(u32 activationId, u64 dataset_id);
s32 Cache_CreateAndAssociateSyncboxDatasetIfNeeded(u32 activationId, u64& datasetId_out, bool ignore_cache);

s32 Cache_DissociateSyncboxDatasetIfNeeded(u32 activationId);
#endif

int Cache_GetActivationIdForSyncUser(u32* activationId_out, bool useUserId, VPLUser_Id_t userId);

/// Return the session for any locally logged-in user that is connected to the infrastructure.
s32 Cache_GetAnyActiveSession(ccd::UserSession& anySession_out);

s32 Cache_GetDeprecatedNetworkEnable(bool &networkEnable);

s32 Cache_GetMaxPlayers(void);

struct BasicSyncConfig {
    ccd::SyncFeature_t syncConfigId;
    SyncType type;
    const SyncPolicy& sync_policy;
    const char* pathSuffix;
};

void Cache_GetClientSyncConfig(const BasicSyncConfig*& clientSyncConfigs_out,
                               int& clientSyncConfigsSize_out);

/// Returns the smallest player index that a signed-in user is occupying.
/// If there is no such user, #CCD_ERROR_NOT_SIGNED_IN is returned.
s32 Cache_GetAnySignedInPlayer(void);

/// Returns boolean indicating whether only mobile network is available.
s32 Cache_GetOnlyMobileNetworkAvailable(bool &onlyMobileNetworkAvailable);

/// Returns boolean indicating whether stream mode is preventing NO_SYNC power state.
s32 Cache_GetStreamPowerMode(bool& streamPowerMode);

/// All data will be copied to the out-param(s), so no need to call #Cache_Lock().
s32 Cache_GetSessionByActivationId(
        u32 activationId,
        ccd::UserSession& session_out);

/// All data will be copied to the out-param(s), so no need to call #Cache_Lock().
/// DEPRECATED: Prefer to use Cache_GetSessionByActivationId.
s32 Cache_GetSessionByUser(
        VPLUser_Id_t userId,
        ccd::UserSession& session_out);

struct ServiceSessionInfo_t {
    u64 sessionHandle;
    std::string serviceTicket;
};

/// All data will be copied to the out-param(s), so no need to call #Cache_Lock().
s32 Cache_GetSessionForVsdsCommon(
        const VPLUser_Id_t* userId,
        int playerIndex, // only used if userId is NULL
        ServiceSessionInfo_t& sessionInfo_out);

/// All data will be copied to the out-param(s), so no need to call #Cache_Lock().
static inline
s32 Cache_GetSessionForVsdsByIndex(
        int playerIndex,
        ServiceSessionInfo_t& sessionInfo_out)
{
    return Cache_GetSessionForVsdsCommon(NULL, playerIndex, sessionInfo_out);
}

/// All data will be copied to the out-param(s), so no need to call #Cache_Lock().
static inline
s32 Cache_GetSessionForVsdsByUser(
        VPLUser_Id_t userId,
        ServiceSessionInfo_t& sessionInfo_out)
{
    return Cache_GetSessionForVsdsCommon(&userId, -1, sessionInfo_out);
}

#if CCD_ENABLE_USER_SUMMARIES
s32 Cache_GetCachedUserSummaries(ccd::CachedUserSummaryList& summaries_out);
#endif

#if CCD_ENABLE_STORAGE_NODE
/// Some final actions before the PC goes to sleep
/// a) Close streaming connections.
void Cache_GoingToSuspend();
#endif

s32 Cache_IsSignedIn(int playerIndex, bool* out);

/// Initializes the cache and loads the #CachedUserSummaryList from the disk.
s32 Cache_Init();

/// Calling thread blocks until it can acquire a CCD Cache read lock.
s32 Cache_Lock(void);

/// Calling thread blocks until it can acquire the CCD Cache write lock.
s32 Cache_LockWrite(void);

void Cache_LogAllCaches(void);

void Cache_LogSummary(void);

void Cache_LogUserCache(void);

s32 Cache_Login(const ccd::LoginInput& request, ccd::LoginOutput& response);

#if CCD_USE_SHARED_CREDENTIALS
// Using user credentials which were retrieved from the shared location to login.
// This is for supporting SSO (Single Sign-On) on sandboxed devices.
s32 Cache_LoginWithSession(const vplex::sharedCredential::UserCredential& userCredential);
#endif

/// Specify either PlayerIndex (#CACHE_PLAYER_INDEX_NOT_SIGNED_IN for unused) or UserId (#VPLUSER_ID_NONE for unused)
s32 Cache_Logout(int optPlayerIndex, VPLUser_Id_t optUserId, VPL_BOOL warnIfNotSignedIn, ccd::LogoutReason_t reason);

/// Deactivate the local user, unlink the local device, and logout from IAS.
s32 Cache_Logout_Unlink(VPLUser_Id_t userId,
                        const ccd::UserSession& session,
                        u64 deviceId);

/// Notifies the cache that network connectivity may have changed.  This will update all running
/// storage nodes for logged-in users.  This call is called by ANS callback and
/// should eventually be replaced by Cache_OnDifferentNetwork.
void Cache_NetworkStatusChange(VPLNet_addr_t localAddr);

/// Notifies the cache that the network has changed and to tear down all existing
/// connections.
void Cache_OnDifferentNetwork();

/// Notifies the cache that ANS has accepted our credentials and considers us to be ONLINE.
void Cache_OnAnsConnected();

s32 Cache_PerformBackgroundTasks();

/// Handle a vplex::syncagent::notifier::SyncAgentNotification from ANS.
void Cache_ProcessSyncAgentNotification(
        const vplex::syncagent::notifier::SyncAgentNotification& notification, uint64_t asyncId);

void Cache_ProcessExpiredSession(u64 sessionHandle, ccd::LogoutReason_t reason);

s32 Cache_Quit(void);

s32 Cache_RemoveUser(VPLUser_Id_t userId);

s32 Cache_SetBackgroundModeInterval(int secs);

s32 Cache_SetDeprecatedNetworkEnable(bool networkEnable);

s32 Cache_SetForegroundMode(VPL_BOOL foregroundMode,
                            const std::string& appId,
                            ccd::CcdApp_t appType);

s32 Cache_GetForegroundApp(std::string& appId_out,
                           ccd::CcdApp_t& appType_out);

s32 Cache_SetOnlyMobileNetworkAvailable(bool onlyMobileNetworkAvailable);

s32 Cache_SetStreamPowerMode(bool streamPowerMode);

/// @return "Does the calling thread hold the CCD Cache lock (for either read or write)?"
VPL_BOOL Cache_ThreadHasLock();

/// @return "Does the calling thread hold the CCD Cache lock for write?"
VPL_BOOL Cache_ThreadHasWriteLock();

/// Calling thread releases a CCD Cache lock (either a read lock or write lock).
s32 Cache_Unlock(void);

s32 Cache_UpdatePowerMode(u32 activationId);

/// List datasets from the cache (without hitting the network).
s32 Cache_ListOwnedDatasets(VPLUser_Id_t userId,
                            ccd::ListOwnedDatasetsOutput& response,
                            bool onlyUseCache);

void Cache_TSCredQuery(TSCredQuery_t* query);

// Get the CCD Protocol Version of a specific device.
int Cache_GetDeviceCcdProtocolVersion(u64 userId, bool onlyUseCache, u64 deviceId);

int Cache_GetDeviceInfo(u64 userId, std::string& deviceName_out);

int Cache_GetSyncboxSettings(u64 userId, ccd::SyncBoxSettings& settings_out);

#if CCD_ENABLE_SYNCDOWN
// If datasetId == 0 - notify all sync down modules for the user,
//                     else notify only that dataset if it's a sync-down module
// overrideAnsCheck - if true, will attempt to perform syncDown even if ANS
//                    connection is down.
void Cache_NotifySyncDownModule(u64 userId,
                                u64 datasetId,
                                bool overrideAnsCheck);
#endif // CCD_ENABLE_SYNCDOWN

/// Called when a media metadata thumbnail migration reaches the delete phase.
int Cache_SetThumbnailMigrateDeletePhase(u32 activationId);

/// Called when a media metadata thumbnail migration is completed.
int Cache_ClearThumbnailMigrate(u32 activationId);

// -------------
#if CCD_ENABLE_STORAGE_NODE

/// Call this to retrieve a sequence number before calling VSDS GetCloudInfo.
/// Pass this to #Cache_DidWeTryToUpdateSyncboxArchiveStorage() after
/// GetCloudInfo returns to find out if the results from GetCloudInfo are
/// reliable (for determining if the local device is still the Syncbox Archive
/// Storage Device).
u32 Cache_GetUpdateSyncboxArchiveStorageSeqNum();

/// Returns true if the local CCD had any call to VSDS AddDatasetArchiveStorageDevice
/// in progress between when \a seqNum was retrieved and right now.
/// @param seqNum See #Cache_GetUpdateSyncboxArchiveStorageSeqNum().
bool Cache_DidWeTryToUpdateSyncboxArchiveStorage(u32 seqNum);

#endif
// -------------------------------------

/// Mark the specified data as dirty.
/// "Dirty" means that we are sure that it is out-of-date and we will automatically perform an
/// update from infra shortly, but only if ANS is connected.
/// Data that is not marked "dirty" is considered "up-to-date".
/// Note that when ANS is disconnected, #CacheMonitor_UpdateIfNeeded will always cause it to be
/// fetched from the infra.
/// (If ANS is not connected, the automatic update isn't as important, because we will hit the infra
/// anyway when anyone calls #CacheMonitor_UpdateIfNeeded, but we may still want to do the update
/// eventually, for the sake of the sync agent, but this is optional.)
void CacheMonitor_MarkDirty(VPLUser_Id_t userId, const CachedUserDataFlags& whichData);

//void CacheMonitor_MarkDirtyByActivationId(u32 activationId, const CachedUserDataFlags& whichData);

/// Same as #CacheMonitor_MarkDirty(), but for all users.
void CacheMonitor_MarkDirtyAll(const CachedUserDataFlags& whichData);

/// Mark the user's subscriptions dirty and return immediately.
/// After a short timeout, a background thread will update our cache from the infra.
#define CacheMonitor_MarkSubscriptionsDirty(userId) \
    BEGIN_MULTI_STATEMENT_MACRO \
    LOG_DEBUG("CacheMonitor_MarkSubscriptionsDirty"); \
    CacheMonitor_MarkDirty(userId, CCD_USER_DATA_SUBSCRIPTIONS_ONLY); \
    END_MULTI_STATEMENT_MACRO

/// Mark the user's datasets dirty and return immediately.
/// After a short timeout, a background thread will update our cache from the infra.
#define CacheMonitor_MarkDatasetsDirty(userId) \
    BEGIN_MULTI_STATEMENT_MACRO \
    LOG_DEBUG("CacheMonitor_MarkDatasetsDirty"); \
    CacheMonitor_MarkDirty(userId, CCD_USER_DATA_DATASETS_ONLY); \
    END_MULTI_STATEMENT_MACRO

/// Mark the user's list of linked device dirty and return immediately.
/// After a short timeout, a background thread will update our cache from the infra.
#define CacheMonitor_MarkLinkedDevicesDirty(userId) \
    BEGIN_MULTI_STATEMENT_MACRO \
    LOG_DEBUG("CacheMonitor_MarkLinkedDevicesDirty"); \
    CacheMonitor_MarkDirty(userId, CCD_USER_DATA_LINKED_DEVICES_ONLY); \
    END_MULTI_STATEMENT_MACRO

/// Mark the user's list of storage nodes dirty and return immediately.
/// After a short timeout, a background thread will update our cache from the infra.
#define CacheMonitor_MarkStorageNodesDirty(userId) \
    BEGIN_MULTI_STATEMENT_MACRO \
    LOG_DEBUG("CacheMonitor_MarkStorageNodesDirty"); \
    CacheMonitor_MarkDirty(userId, CCD_USER_DATA_STORAGE_NODES_ONLY); \
    END_MULTI_STATEMENT_MACRO

// -------------------------------------

/// If any of the requested items are marked "dirty" or ANS is not connected, requests an update
/// and blocks until the update occurs.  Otherwise, immediately returns #VPL_OK.
//@{
int CacheMonitor_UpdateIfNeeded(u32 activationId, const CachedUserDataFlags& whichData);

int CacheMonitor_UpdateIfNeeded(bool useUserId, VPLUser_Id_t userId, const CachedUserDataFlags& whichData);

static inline int CacheMonitor_UpdateIfNeededSyncUser(const CachedUserDataFlags& whichData) {
    return CacheMonitor_UpdateIfNeeded(false, 0, whichData);
}

static inline int CacheMonitor_UpdateSubscriptionsIfNeeded(bool useUserId, VPLUser_Id_t userId) {
    return CacheMonitor_UpdateIfNeeded(useUserId, userId, CCD_USER_DATA_UPDATE_SUBSCRIPTIONS);
}
static inline int CacheMonitor_UpdateDatasetsIfNeeded(bool useUserId, VPLUser_Id_t userId) {
    return CacheMonitor_UpdateIfNeeded(useUserId, userId, CCD_USER_DATA_DATASETS_ONLY);
}
static inline int CacheMonitor_UpdateLinkedDevicesIfNeeded(bool useUserId, VPLUser_Id_t userId) {
    return CacheMonitor_UpdateIfNeeded(useUserId, userId, CCD_USER_DATA_LINKED_DEVICES_ONLY);
}

static inline int CacheMonitor_UpdateSubscriptionsIfNeeded(VPLUser_Id_t userId) {
    return CacheMonitor_UpdateSubscriptionsIfNeeded(true, userId);
}
static inline int CacheMonitor_UpdateDatasetsIfNeeded(VPLUser_Id_t userId) {
    return CacheMonitor_UpdateDatasetsIfNeeded(true, userId);
}
static inline int CacheMonitor_UpdateLinkedDevicesIfNeeded(VPLUser_Id_t userId) {
    return CacheMonitor_UpdateLinkedDevicesIfNeeded(true, userId);
}

//@}

// -------------------------------------

/// Must lock before calling and unlock *after* you are done with the result.
CachePlayer* cache_getTheUser(); // call lock/unlock

/// Must lock before calling and unlock *after* you are done with the result.
CachePlayer* cache_getUserByPlayerIndex(int playerIndex); // call lock/unlock

/// Must lock before calling and unlock *after* you are done with the result.
CachePlayer* cache_getUserByUserId(VPLUser_Id_t userId); // call lock/unlock

/// Must lock before calling and unlock *after* you are done with the result.
CachePlayer* cache_getUserByActivationId(u32 activationId); // call lock/unlock

/// Gets the user at @a playerIndex, unless it is #CACHE_PLAYER_INDEX_NOT_SIGNED_IN, in which
/// case gets the logged-in user with id @a userId.
/// Must lock before calling and unlock *after* you are done with the result.
s32 cache_getUser(int playerIndex, VPLUser_Id_t userId, VPL_BOOL warnIfNotSignedIn, CachePlayer** user_out); // call lock/unlock

/// When using certain CCDI APIs related to Sync Agent, we allow omitting the userId to
/// indicate player 0.
/// Must lock before calling and unlock *after* you are done with the result.
CachePlayer* cache_getSyncUser(bool useUserId, VPLUser_Id_t userId); // call lock/unlock

/// Must lock before calling.
s32 cache_getBackgroundModeIntervalSecs(); // call lock/unlock

/// Must lock before calling.
ccd::PowerMode_t cache_getPowerMode(); // call lock/unlock

/// Must lock before calling.
void cache_processUpdatedDatasetList(CachePlayer& user); // call lock/unlock

/// Must lock before calling.
void cache_clearMMCacheByType(const CachePlayer& user, int syncType); // call lock/unlock

/// Must lock before calling.
void cache_convertCacheState(CachePlayer& user); // call lock/unlock

/// Update OwnershipSync with the current sync configs, based on the cache state.
/// Call this in response to a change in the list of datasets or change
/// in PicStream (camera roll) download directories.
/// Must lock before calling.
void cache_registerSyncConfigs(CachePlayer& user); // call lock/unlock

/// Must lock before calling.
void cache_getLoggedOutUsers(google::protobuf::RepeatedPtrField<ccd::LoggedOutUser>& users_out); // call lock/unlock

int GetMediaMetadataDatasetByUserId(VPLUser_Id_t userId,
                                    u64& datasetId_out);

std::string GetLocalArchiveStorageSyncboxDatasetName();

struct SyncFeature {
    u32 feature; ///< Numeric value is based on SyncFeature_t.
    bool canUpload;

    SyncFeature(u32 argFeature, bool argCanUpload)
    :  feature(argFeature),
       canUpload(argCanUpload)
    {}
};

void GetSyncFeaturesFromAppType(ccd::CcdApp_t appType,
                                std::vector<SyncFeature>& features_out);

/// Automatically releases the lock when this object goes out of scope.
/// You should only declare instances of this class on the stack and never pass them around!
class CacheAutoLock {
 public:
    CacheAutoLock() : needsToUnlock(false) {}

    s32 LockForRead();

    s32 LockForWrite();

    void UnlockNow();

    ~CacheAutoLock() { UnlockNow(); }

 private:
    VPL_DISABLE_COPY_AND_ASSIGN(CacheAutoLock);
    bool needsToUnlock;
};

// -------------------------------------
// Local Servers APIs
// -------------------------------------

/// Get the HttpService instance (used for handling HTTP service requests).
HttpService& LocalServers_GetHttpService();

int LocalServers_CreateLocalInfoObj(VPLUser_Id_t userId, u32 instanceId);

/// You should call #LocalServers_CreateLocalInfoObj() before calling this.
int LocalServers_StartHttpService(VPLUser_Id_t userId);

/// Stop any local servers that have been started for this user (HttpService and/or StorageNode).
void LocalServers_StopServers(bool userLogout);

/// You must lock this mutex when using #LocalServers_getStorageNode() or
/// #LocalServers_getLocalInfo().
/// @note DO NOT lock this while already holding the cache lock!
VPLMutex_t* LocalServers_GetMutex();

/// You must lock the LocalServers mutex before calling this and only unlock *after* you are done
/// with the Ts2::LocalInfo.
Ts2::LocalInfo* LocalServers_getLocalInfo();

/// Report to TS layer that the OS confirmed network connectivity.
void LocalServers_ReportNetworkConnected();

/// Report to TS layer that the local device is online (according to ANS).
void LocalServers_ReportDeviceOnline();

#if CCD_ENABLE_STORAGE_NODE

/// You must lock the LocalServers mutex before calling this and only unlock *after* you are done
/// with the vss_server.
vss_server* LocalServers_getStorageNode();

struct StartStorageNodeContext
{
    VPLUser_Id_t userId;
    u32 activationId;
    s64 clusterId;
    u64 deviceStorageDatasetId;
    ccd::UserSession session;
    bool isSyncboxArchiveStorage;
    /// When isSyncboxArchiveStorage is true, this is the specific Syncbox dataset that
    /// is associated with the local Archive Storage Device.
    /// (Remember that the user may have more than one Syncbox dataset; make sure that
    /// this is the correct one associated with this device).
    u64 syncboxArchiveStorageDatasetId;
    std::string syncboxSyncFeaturePath;
};

void LocalServers_PopulateStartStorageNodeContext(CachePlayer& user, StartStorageNodeContext& context_out);

int LocalServers_StartStorageNode(const StartStorageNodeContext& context);

void LocalServers_StopStorageNode();

#endif // #if CCD_ENABLE_STORAGE_NODE

#endif // include guard
