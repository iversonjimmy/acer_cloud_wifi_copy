//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __SYNC_CONFIG_HPP__
#define __SYNC_CONFIG_HPP__

#include "vplu_types.h"

#include "vcs_defs.hpp"
#include "vcs_archive_access.hpp"
#include "vcs_file_url_access.hpp"
#include "SyncConfigThreadPool.hpp"
#include "vplex_time.h"

#include "vplu_common.h"
#include <string>

static const VPLTime_t SYNC_CONFIG_DEFAULT_ERROR_RETRY_INTERVAL = VPLTime_FromMinutes(15);

// Forward declaration.
class SyncConfig;

/// Types of Sync.
enum SyncType {
    /// Normal Sync: File considered synced when content is on storage.
    SYNC_TYPE_TWO_WAY,
    SYNC_TYPE_ONE_WAY_UPLOAD,
    SYNC_TYPE_ONE_WAY_DOWNLOAD,

    /// Pure Virtual Sync: File considered synced when content metadata is recorded.
    SYNC_TYPE_ONE_WAY_UPLOAD_PURE_VIRTUAL_SYNC,
    SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC,

    /// Hybrid Virtual Sync: A mixture between normal and virtual.
    SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC,

    /// Two way mode for Syncbox.
    ///    Upload - Does not upload to infra. Simply reports to VCS that the file exists.
    ///    Download - If the download URL is from the local staging area, reports
    ///               to infra via PostURL that download has completed.
    SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE,

    // NOT IMPLEMENTED (placeholder for future):
    SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE_AND_USE_ACS,

  // Note:  Order of enums must be preserved for backwards compatibility.
};

/// Policies to specify the behavior of a Sync Config.
struct SyncPolicy {

    /// Policy to determine the "winner" of a conflict.  The "winner" is the device that has its copy
    /// of a particular file/directory become the latest revision on the server.
    /// @note Ignored if SyncType is not #SYNC_TYPE_TWO_WAY.
    enum SyncConflictWinnerPolicy {
        /// The first device to commit to the server wins.  If another device tries to commit a different
        /// change for the same base revision of a file/directory, that request will be rejected.
        /// If there is a conflict between a modification and a deletion, we will automatically resolve in
        /// favor of the modification.
        SYNC_CONFLICT_POLICY_FIRST_TO_SERVER_WINS,
        /// (Not yet supported) The file with the higher modification timestamp wins.
        /// The timestamp is recorded from the local filesystem on each device that modified the file.
        /// Each timestamp is also normalized to UTC.
        /// If there is a conflict between a modification and a deletion, we will automatically resolve in
        /// favor of the modification (since the filesystem doesn't leave a timestamp for a deleted entry).
        SYNC_CONFLICT_POLICY_LATEST_MODIFY_TIME_WINS,
    };
    SyncConflictWinnerPolicy how_to_determine_winner;

    /// Policy to determine what happens to the file or directory from the "loser" of a conflict.
    /// @note Ignored if SyncType is not #SYNC_TYPE_TWO_WAY.
    enum SyncConflictLoserPolicy {
        /// The losing file/dir will be deleted from all devices.
        SYNC_CONFLICT_POLICY_DELETE_LOSER,
        /// (Not yet supported) The losing file/dir will be renamed to a unique filename, but not uploaded to the server.
        SYNC_CONFLICT_POLICY_PRESERVE_LOSER,
        /// The losing file/dir will be renamed to a unique filename and then uploaded to the server
        /// and propagated to other devices.
        SYNC_CONFLICT_POLICY_PROPAGATE_LOSER,
    };
    SyncConflictLoserPolicy what_to_do_with_loser;

    /// Select the technique to use when determining if a file upload or download can be skipped.
    enum FileComparePolicy {
        /// Default policy; any mismatch between the VCS listing and the localDB will be downloaded,
        /// and any mismatch between the local filesystem and localDB will be uploaded.
        FILE_COMPARE_POLICY_DEFAULT,
        /// Special case to avoid uploading/downloading unchanged files after migration from VSS to VCS.
        /// If the VCS fields lastChangedNanoSecs and createDateNanoSecs are both 0, then consider
        /// the local file to match the server file if they have the same file sizes.
        FILE_COMPARE_POLICY_USE_SIZE_FOR_MIGRATION,
        /// Locally compute the hash of each file and store the hash in VCS.
        /// Use the hash values returned from VCS to suppress unneeded uploads and downloads.
        /// This adds a bit of computational overhead, but the benefit is that we can save time
        /// and bandwidth when the file hasn't actually changed.
        FILE_COMPARE_POLICY_USE_HASH,
    };
    FileComparePolicy how_to_compare;

    /// Not supported yet; please use #UINT64_MAX.
    u64 max_storage_bytes;

    /// Not supported yet; please use #UINT64_MAX.
    u64 max_num_files;

    /// Maximum number of error retry attempts per task (only applicable to recoverable error cases).
    u64 max_error_retry_count;

    /// Error retry interval (only applicable to recoverable error cases).
    VPLTime_t error_retry_interval;

    /// If the SyncFolder is exposed to the end user, set this to true to enforce that
    /// we treat filenames that differ only by uppercase/lowercase as the same file.
    /// If we control the filenames, this can be false to improve performance and reduce local DB size.
    bool case_insensitive_compare;

    /// Set the defaults.
    SyncPolicy() :
        how_to_determine_winner(SYNC_CONFLICT_POLICY_FIRST_TO_SERVER_WINS),
        what_to_do_with_loser(SYNC_CONFLICT_POLICY_DELETE_LOSER),
        how_to_compare(FILE_COMPARE_POLICY_DEFAULT),
        max_storage_bytes(UINT64_MAX),
        max_num_files(UINT64_MAX),
        max_error_retry_count(500),
        error_retry_interval(SYNC_CONFIG_DEFAULT_ERROR_RETRY_INTERVAL),
        case_insensitive_compare(false)
    {}
};

/// The basic status of the synchronization.
enum SyncConfigStatus {
    /// There is no work to be done (all files are in-sync and there are no scan requests).
    /// From #SYNC_CONFIG_STATUS_DONE, can transition to:
    /// - #SYNC_CONFIG_STATUS_SCANNING when a scan is requested (due to a restart or a change notification).
    /// .
    SYNC_CONFIG_STATUS_DONE,
    /// There was a scan requested, but no actual changes have been found yet.
    /// From #SYNC_CONFIG_STATUS_SCANNING, can transition to:
    /// - #SYNC_CONFIG_STATUS_DONE if the scan completed and no changes were found.
    /// - #SYNC_CONFIG_STATUS_SYNCING when a local or remote change is found.
    /// .
    SYNC_CONFIG_STATUS_SCANNING,
    /// The scan has found at least one change to upload or download.
    /// From #SYNC_CONFIG_STATUS_SYNCING, can transition to:
    /// - #SYNC_CONFIG_STATUS_DONE if all downloads and uploads were completed successfully and
    ///   there are no pending scan requests.
    /// - #SYNC_CONFIG_STATUS_SCANNING if all downloads and uploads were completed successfully but
    ///   there is a new scan request.
    /// .
    SYNC_CONFIG_STATUS_SYNCING,
};

enum SyncConfigEventType {

    /// The overall sync status has changed.
    /// Cast the #SyncConfigEvent to #SyncConfigEventStatusChange to get details.
    SYNC_CONFIG_EVENT_STATUS_CHANGE,

    /// A remote change has been successfully pulled down to the local filesystem.
    /// Cast the #SyncConfigEvent to #SyncConfigEventEntryDownloaded to get details.
    SYNC_CONFIG_EVENT_ENTRY_DOWNLOADED,

    /// A local change has been successfully pushed up to the remote server.
    /// Cast the #SyncConfigEvent to #SyncConfigEventEntryUploaded to get details.
    SYNC_CONFIG_EVENT_ENTRY_UPLOADED,
};

// For SyncBox we can get SyncConfigEventType for the basic type
// (SYNC_CONFIG_EVENT_ENTRY_DOWNLOADED or SYNC_CONFIG_EVENT_ENTRY_UPLOADED) and
// SyncConfigEventDetailType for the detail, to combine into a SyncEventType_t
enum SyncConfigEventDetailType {

    /// This detail is useless. For some Impl of SyncConfig which
    ///  is not support detail type
    SYNC_CONFIG_EVENT_DETAIL_NONE,

    /// A new file was downloaded/uploaded
    SYNC_CONFIG_EVENT_DETAIL_NEW_FILE,

    /// A modification to an existing file was downloaded/uploaded
    SYNC_CONFIG_EVENT_DETAIL_MODIFIED_FILE,

    /// A local file was deleted due to remote/local
    SYNC_CONFIG_EVENT_DETAIL_FILE_DELETE,

    /// A local/remote folder was created
    SYNC_CONFIG_EVENT_DETAIL_FOLDER_CREATE,

    /// A folder was deleted due to remote/local
    SYNC_CONFIG_EVENT_DETAIL_FOLDER_DELETE,

    /// A local file was in conflict and was renamed.
    /// This type can be only found from SYNC_CONFIG_EVENT_ENTRY_DOWNLOADED
    SYNC_CONFIG_EVENT_DETAIL_CONFLICT_FILE_CREATED,
};

// This enum should be exactly one-to-one map to SyncStateType_t in ccdi_rpc.proto
enum SyncConfigStateType_t {

    /// The file/folder is not within a folder that is being tracked as a "SyncFolder".
    SYNC_CONFIG_STATE_NOT_IN_SYNC_FOLDER = 1,

    /// The file/folder exists locally and is the same as recorded in the server (VCS).
    SYNC_CONFIG_STATE_UP_TO_DATE = 2,

    /// There are remote changes to the file/folder that need to be downloaded.
    /// For folders, this recursively includes files in subdirectories.
    SYNC_CONFIG_STATE_NEED_TO_DOWNLOAD = 4,

    /// There are local changes to the file/folder that need to be uploaded.
    /// For folders, this recursively includes files in subdirectories.
    SYNC_CONFIG_STATE_NEED_TO_UPLOAD = 6,

    /// There are both files to upload and files to download under the specified folder.
    /// This state is only applicable to folders.
    /// This recursively includes files in subdirectories.
    SYNC_CONFIG_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD = 7,

    /// The file/folder is being filtered out because it has special meaning to CCD.
    /// For example ".sync_temp" will be filtered.
    SYNC_CONFIG_STATE_FILTERED = 8,

    /// The file/folder is within a "SyncFolder" and is not filtered out, but CCD doesn't
    /// know about it yet.
    /// This can happen when a file is newly created.
    /// It is probably safe to display this the same way as SYNC_STATE_NEED_TO_UPLOAD.
    SYNC_CONFIG_STATE_UNKNOWN = 9,
};

struct SyncConfigEvent {
    const SyncConfigEventType type;
    SyncConfig& sync_config;
    /// The pointer that was provided when the callback was registered.
    void* callback_context;
    u64 user_id;
    u64 dataset_id;

    SyncConfigEvent(
            SyncConfigEventType type,
            SyncConfig& sync_config,
            void* callback_context,
            u64 user_id,
            u64 dataset_id)
    :   type(type), sync_config(sync_config), callback_context(callback_context),
        user_id(user_id), dataset_id(dataset_id)
    {}
};

/// For #SYNC_CONFIG_EVENT_STATUS_CHANGE.
/// See #SyncConfigManager::SyncConfig_GetStatus() for more information about the status fields.
struct SyncConfigEventStatusChange : public SyncConfigEvent
{
    SyncConfigStatus new_status;
    bool new_has_error;
    bool new_work_to_do;
    SyncConfigStatus prev_status;
    bool prev_has_error;
    bool prev_work_to_do;

    // Following members are for SyncBox only
    u32 new_uploads_remaining;
    u32 new_downloads_remaining;
    bool new_remote_scan_pending;
    u32 prev_uploads_remaining;
    u32 prev_downloads_remaining;
    bool prev_remote_scan_pending;

    SyncConfigEventStatusChange(
            SyncConfig& sync_config,
            void* callback_context,
            u64 user_id,
            u64 dataset_id,
            SyncConfigStatus new_status,
            bool new_has_error,
            bool new_work_to_do,
            SyncConfigStatus prev_status,
            bool prev_has_error,
            bool prev_work_to_do,
            u32 new_uploads_remaining,
            u32 new_downloads_remaining,
            bool new_remote_scan_pending,
            u32 prev_uploads_remaining,
            u32 prev_downloads_remaining,
            bool prev_remote_scan_pending)
    :   SyncConfigEvent(SYNC_CONFIG_EVENT_STATUS_CHANGE, sync_config, callback_context, user_id, dataset_id),
        new_status(new_status), new_has_error(new_has_error), new_work_to_do(new_work_to_do),
        prev_status(prev_status), prev_has_error(prev_has_error), prev_work_to_do(prev_work_to_do),
        new_uploads_remaining(new_uploads_remaining), new_downloads_remaining(new_downloads_remaining),
        new_remote_scan_pending(new_remote_scan_pending), prev_uploads_remaining(prev_uploads_remaining),
        prev_downloads_remaining(prev_downloads_remaining), prev_remote_scan_pending(prev_remote_scan_pending)
    {}
};

/// For #SYNC_CONFIG_EVENT_ENTRY_DOWNLOADED.
/// This event supports both file and directory.
struct SyncConfigEventEntryDownloaded : public SyncConfigEvent
{
    /// If false, the file was downloaded.
    /// If true, the file was removed.
    bool is_deletion;
    std::string rel_path;
    std::string abs_path;

    // Following members are for SyncBox only
    SyncConfigEventDetailType detail_type;
    u64 event_time;
    std::string conflict_file_original_path;

    SyncConfigEventEntryDownloaded(
            SyncConfig& sync_config,
            void* callback_context,
            u64 user_id,
            u64 dataset_id,
            bool is_deletion,
            const std::string& rel_path,
            const std::string& abs_path,
            SyncConfigEventDetailType detail_type,
            u64 event_time,
            const std::string& conflict_file_original_path)
    :   SyncConfigEvent(SYNC_CONFIG_EVENT_ENTRY_DOWNLOADED, sync_config, callback_context, user_id, dataset_id),
        is_deletion(is_deletion), rel_path(rel_path), abs_path(abs_path), detail_type(detail_type),
        event_time(event_time), conflict_file_original_path(conflict_file_original_path)
    {}
};

/// For #SYNC_CONFIG_EVENT_ENTRY_UPLOADED.
/// This event supports both file and directory.
struct SyncConfigEventEntryUploaded : public SyncConfigEvent
{
    std::string rel_path;
    std::string abs_path;

    // Following members are for SyncBox only
    SyncConfigEventDetailType detail_type;
    u64 event_time;

    SyncConfigEventEntryUploaded(
            SyncConfig& sync_config,
            void* callback_context,
            u64 user_id,
            u64 dataset_id,
            const std::string& rel_path,
            const std::string& abs_path,
            SyncConfigEventDetailType detail_type,
            u64 event_time)
    :   SyncConfigEvent(SYNC_CONFIG_EVENT_ENTRY_UPLOADED, sync_config, callback_context, user_id, dataset_id),
        rel_path(rel_path), abs_path(abs_path), detail_type(detail_type), event_time(event_time)
    {}
};

/// Callback function that will be invoked whenever a SyncConfig changes status.
/// For convenience, both the new status and previous status are provided.
typedef void (*SyncConfigEventCallback)(SyncConfigEvent& event);

/// Parameters to allow cleaning up the staging area (private ACS).
// This is admittedly an over-simplified API for now.  It will need to be changed if we want to
// later support a "dumb" staging area (meaning it isn't part of CCD).
// A proper abstraction would probably include a callback interface to retrieve a paged directory
// listing of the staging area, including timestamps.
struct LocalStagingArea {
    /// Specify the staging area URL prefix (as provided by VCS).
    /// For Syncbox, this is expected to be
    /// acer-ts://<userId>/<deviceId>/<instanceId>/rf/file/<Device Storage datasetId>/[stagingArea:<syncbox datasetId>]/
    std::string urlPrefix;

    /// Specify the absolute path on the local filesystem.
    std::string absPath;

    LocalStagingArea() :
        urlPrefix(),
        absPath()
    {}
};

/// Information required to communicate with the servers that are hosting the dataset.
struct DatasetAccessInfo {
    /// Local deviceId.
    u64 deviceId;

    /// Session handle (assigned by IAS).
    u64 sessionHandle;

    /// Service Ticket for service "VSDS".
    std::string serviceTicket;

    /// URL prefix for accessing the regional cluster.
    /// For example "https://www-c100.cloud.acer.com:443". (CCD can use #Query_GetUrlPrefix()).
    std::string urlPrefix;

    /// Friendly name of the local device. Used for conflict copy file names.
    std::string deviceName;

    /// Provides the interface to an Archive Storage Device for OneWayDown sync.
    /// If NULL, SyncConfig will only use ACS when downloading files.
    VCSArchiveAccess* archiveAccess;

    /// Provides the interface to use VCS accessinfo URLs for TwoWay sync.
    /// Cannot be NULL for TwoWay sync.
    VCSFileUrlAccess* fileUrlAccess;

    /// Only relevant for #SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE; ignored otherwise.
    /// To enable this SyncConfig to clean up the dataset staging area, specify the staging
    /// area URL prefix and absolute path.
    LocalStagingArea localStagingArea;

    DatasetAccessInfo() :
        deviceId(0),
        sessionHandle(0),
        serviceTicket(),
        urlPrefix(),
        deviceName(),
        archiveAccess(NULL),
        fileUrlAccess(NULL)
    {}
};

/// Tracks state and exposes operations for a specific directory-hierarchy-to-sync.
/// Runs the sync algorithm when requested.
/// @see #CreateSyncConfig()
class SyncConfig
{
public:
    virtual ~SyncConfig() {};

    /// Stop the worker thread at the next reasonable opportunity and close the local DB.
    /// This returns immediately.  Use #SyncConfig::Join() to wait for the worker to stop.
    /// (Splitting the shutdown sequence into Close+Join makes it easy for a single coordinator
    /// thread to close multiple workers in parallel.)
    virtual int RequestClose() = 0;

    /// Wait for the worker thread to stop after #SyncConfig::RequestClose().
    virtual int Join() = 0;

    /// Pause the worker thread at the next reasonable opportunity.
    /// @param blocking If true, the call will not return until the worker thread is actually
    ///     paused.  If false, the call returns immediately.
    virtual int Pause(bool blocking) = 0;

    /// Resume the worker thread after #SyncConfig::Pause().
    /// This call returns immediately.
    ///
    /// @note If a thread has called the blocking version of Pause(true) and is
    /// blocked, Resume() will return #SYNC_AGENT_ERR_RESUME_CALLED_DURING_WAIT_FOR_PAUSE.
    /// It will be the responsibility of the caller waiting for Pause to resume the
    /// SyncConfig after pausing is no longer necessary.
    virtual int Resume() = 0;

    /// Call this to report a local filesystem change that possibly needs to be uploaded.
    /// This is only needed on platforms that don't support filesystem monitoring.
    /// On platforms that support filesystem monitoring, the monitoring will automatically
    /// be registered and hooked up within #CreateSyncConfig().
    virtual int ReportLocalChange(const std::string& path) = 0;

    /// Call this when there is definitely a remote change to the dataset.
    /// It is expected that this is called when the async notification service explicitly notifies
    /// us of a dataset content change.
    /// Since there is generally only one connection to the async notification service but
    /// multiple SyncConfig objects, the upper layer is responsible for dispatching those
    /// notifications to the appropriate SyncConfig.
    virtual int ReportRemoteChange() = 0;

    /// Call this to request a remote scan when you are not sure if there is actually a remote
    /// change to the dataset.
    /// It is expected that this is called after reconnecting to the async notification service,
    /// since we are unsure if any notifications were missed.  This should also be called in
    /// response to a user-initiated sync request.
    virtual int ReportPossibleRemoteChange() = 0;

    /// Call this to report a change in the status of the archive storage device.
    /// If online, this will kick off a retry of previously failed interactions that require
    /// storage device.
    /// If offline, this will prevent spurious connection attempts to the archive
    /// device that will most likely fail.
    virtual int ReportArchiveStorageDeviceAvailability(bool is_online) = 0;

    /// Returns the current status of this worker.
    /// See #SyncConfigManager::SyncConfig_GetStatus() for more information about the status fields.
    virtual int GetSyncStatus(
            SyncConfigStatus& status__out,
            bool& has_error__out,
            bool& work_to_do__out,
            u32& uploads_remaining__out,
            u32& downloads_remaining__out,
            bool& remote_scan_pending__out) = 0;

    virtual u64 GetUserId() = 0;

    virtual u64 GetDatasetId() = 0;

    /// Returns the absolute path on the local filesystem for the SyncFolder.
    /// This is the same path that was passed in to #CreateSyncConfig (as local_dir), but
    /// with #Util_CleanupPath applied to it.
    virtual std::string GetLocalDir() = 0;

    /// @param abs_path Format is not strict; this method will normalize to forward slashes and
    ///     remove excess slashes before processing.
    virtual int GetSyncStateForPath(const std::string& abs_path,
                                    SyncConfigStateType_t& state__out,
                                    u64& dataset_id__out,
                                    bool& is_sync_folder_root__out) = 0;

    /// @param sync_config_relative_path should never start with a '/'.
    virtual int LookupComponentByPath(
            const std::string& sync_config_relative_path,
            u64& component_id__out,
            u64& revision__out,
            bool& is_on_acs__out) = 0;

    virtual int LookupAbsPath(
            u64 component_id,
            u64 revision,
            std::string& absolute_path__out,
            u64& local_modify_time__out,
            std::string& hash__out) = 0;

protected:
    SyncConfig() {};
private:
    VPL_DISABLE_COPY_AND_ASSIGN(SyncConfig);
};

/// Create a #SyncConfig object that can execute the sync algorithm.
/// This will also open the local DB for the sync config and spawn the worker thread.
/// The worker thread will initially be paused; you must call #SyncConfig::Resume() to
/// start it.
/// @param local_dir Absolute path on the local filesystem.
/// @param server_dir Relative path within the dataset.
/// @param event_cb Can be NULL if this callback is not desired.
/// @param thread_pool Separate SyncConfigs can share a thread pool.  Can also be NULL.
/// @param make_dedicated_thread Separate SyncConfigs can share a thread pool and if this
///     param is true, create its own dedicated thread to ensure progress is never blocked.
/// @param callback_context Optional pointer that will be passed to \a event_cb.
///     SyncConfig will not try to interpret its contents.
/// @return The newly created #SyncConfig object, or NULL if there was an error (check
///     \a err_code__out to find out the error code).
/// @note You must eventually call #DestroySyncConfig() to avoid leaking resources.
SyncConfig* CreateSyncConfig(
        u64 user_id,
        const VcsDataset& dataset,
        SyncType type,
        const SyncPolicy& sync_policy,
        const std::string& local_dir,
        const std::string& server_dir,
        const DatasetAccessInfo& dataset_access_info,
        SyncConfigThreadPool* thread_pool,
        bool make_dedicated_thread,
        SyncConfigEventCallback event_cb,
        void* callback_context,
        int& err_code__out,
        bool allow_create_db);


/// Destroy an object previously created by #CreateSyncConfig(), releasing its resources.
void DestroySyncConfig(SyncConfig* syncConfig);

// =========================
// ======= Utilities =======

#define SYNC_CONFIG_ID_STRING "1"
#define SYNC_TEMP_DIR ".sync_temp"
#define SYNC_CONFIG_DB_DIR SYNC_TEMP_DIR

#define DESKTOP_INI "desktop.ini"

#endif // include guard
