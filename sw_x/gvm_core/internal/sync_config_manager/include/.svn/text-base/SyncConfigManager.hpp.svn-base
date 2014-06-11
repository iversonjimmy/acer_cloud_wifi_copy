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

#ifndef __SYNC_CONFIG_MANAGER_HPP__
#define __SYNC_CONFIG_MANAGER_HPP__

#include "SyncConfig.hpp"
#include "vplex_sync_agent_notifier.pb.h"

/// An opaque handle to a Sync Config.  Do not directly modify its contents.
/// It is okay to test one SyncConfigHandle with another SyncConfigHandle for equality.
struct SyncConfigHandle { void* x; };

/// Optional wrapper layer to coordinate multiple #SyncConfig instances.
class SyncConfigManager
{
public:
    virtual ~SyncConfigManager() {}

    /// Register a SyncConfig.
    /// It will initially be disabled; you must call #SyncConfigManager::SyncConfig_Enable()
    /// to start the work.
    /// @param[out] sync_config_handle__out On success, returns the handle.
    /// @param user_id See #CreateSyncConfig().
    /// @param dataset See #CreateSyncConfig().
    /// @param type See #CreateSyncConfig().
    /// @param sync_policy See #CreateSyncConfig().
    /// @param local_dir See #CreateSyncConfig().
    /// @param server_dir See #CreateSyncConfig().
    /// @param dataset_access_info See #CreateSyncConfig().
    /// @param thread_pool See #CreateSyncConfig().
    /// @param make_dedicated_thread See #CreateSyncConfig().
    /// @param event_cb See #CreateSyncConfig().
    /// @param callback_context See #CreateSyncConfig().
    virtual int SyncConfig_Add(
            SyncConfigHandle* sync_config_handle__out,
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
            bool allow_create_db) = 0;

    /// Step 1/3 of "Removing a SyncConfig".
    /// Tells the SyncConfig to stop.
    virtual int SyncConfig_RequestClose(
            SyncConfigHandle sync_config_handle) = 0;

    /// Step 2/3 of "Removing a SyncConfig".
    /// This blocks until the SyncConfig is safely stopped.
    static int SyncConfig_Join(
            SyncConfigHandle sync_config_handle);

    /// Step 3/3 of "Removing a SyncConfig".
    /// Unregisters a previously registered SyncConfig and frees the resources.
    virtual int SyncConfig_Destroy(
            SyncConfigHandle sync_config_handle) = 0;

    /// Enable the SyncConfig, allowing its worker thread to perform network and disk I/O.
    virtual int SyncConfig_Enable(
            SyncConfigHandle sync_config_handle) = 0;

    /// Temporarily disable the SyncConfig, which causes its worker thread to be paused at the
    /// next reasonable opportunity.  Call #SyncConfig_Enable() to allow the SyncConfig to resume.
    /// @param blocking If true, the call will not return until the worker thread is actually
    ///     paused.  If false, the call returns immediately.
    virtual int SyncConfig_Disable(
            SyncConfigHandle sync_config_handle,
            bool blocking) = 0;

    /// @note Not supported yet.
    virtual int SyncConfig_SetPriority(
            SyncConfigHandle sync_config_handle,
            int priority) = 0;

    /// Get the status of the SyncConfig.
    /// @param status__out See #SyncConfigStatus.
    /// @param has_error_out If true, there was an error that needs to be retried.
    /// @param work_to_do__out True if there is scanning or syncing to be done now.  False if
    ///     the worker is waiting for a new scan request or for the error retry interval to expire.
    virtual int SyncConfig_GetStatus(
            SyncConfigHandle sync_config_handle,
            SyncConfigStatus& status__out,
            bool& has_error__out,
            bool& work_to_do__out,
            u32& uploads_remaining__out,
            u32& downloads_remaining__out,
            bool& remote_scan_pending__out) = 0;

    virtual int SyncConfig_LookupComponentByPath(
            SyncConfigHandle sync_config_handle,
            const std::string& sync_config_relative_path,
            u64& component_id__out,
            u64& revision__out,
            bool& is_on_acs__out) = 0;

    virtual int SyncConfig_GetSyncStateForPath(
            SyncConfigHandle sync_config_handle,
            const std::string& abs_path,
            SyncConfigStateType_t& state__out,
            u64& dataset_id__out,
            bool& is_sync_folder_root__out) = 0;

    virtual int SyncConfig_LookupAbsPath(
            SyncConfigHandle sync_config_handle,
            u64 component_id,
            u64 revision,
            std::string& absolute_path__out,
            u64& local_modify_time__out,
            std::string& hash__out) = 0;

    /// Call this when a DatasetContentUpdate is received.
    virtual void ReportRemoteChange(
            const vplex::syncagent::notifier::DatasetContentUpdate& notification) = 0;

    /// Call this to request a remote scan when you are not sure if there is actually a
    /// remote change to the dataset.
    virtual int ReportPossibleRemoteChange(
            SyncConfigHandle sync_config_handle) = 0;

    /// Called when there is a local change that possibly needs to be uploaded.
    /// This is only needed on platforms that don't support filesystem monitoring.
    /// On platforms that support filesystem monitoring, the monitoring will automatically
    /// be registered and hooked up within #SyncConfig_Add().
    virtual int ReportLocalChange(
            SyncConfigHandle sync_config_handle,
            const std::string& optional_path) = 0;

    virtual int ReportArchiveStorageDeviceAvailability(
            SyncConfigHandle sync_config_handle,
            bool is_online) = 0;

protected:
    SyncConfigManager() {};
private:
    VPL_DISABLE_COPY_AND_ASSIGN(SyncConfigManager);
};

/// Create a #SyncConfigManager object.
/// @return The newly created #SyncConfigManager object, or NULL if there was an error (check
///     \a err_code__out to find out the error code).
/// @note You must eventually call #DestroySyncConfigManager() to avoid leaking resources.
SyncConfigManager* CreateSyncConfigManager(int& err_code__out);

/// Destroy an object previously created by #CreateSyncConfigManager(), releasing its resources.
void DestroySyncConfigManager(SyncConfigManager* syncConfigManager);

#endif // include guard
