//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __SYNC_FEATURE_MGR_HPP__
#define __SYNC_FEATURE_MGR_HPP__

// It is currently safe to call SyncFeatureMgr APIs with the CCD cache lock held.
// (This means that the implementation of SyncFeatureMgr_* must be careful to
// avoid acquiring the cache lock while holding its own mutex.)

#include "SyncConfig.hpp"
#include "base.h"
#include "ccdi_rpc.pb.h"
#include "vplex_sync_agent_notifier.pb.h"

//**************
// TODO: Bug 9908: The SyncFeatureMgr API should evolve after we refactor chunks of cache.cpp into here.
//**************

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
        bool createDedicatedThread,
        u64 max_storage,
        u64 max_files,
        const LocalStagingArea* stagingAreaInfo,
        bool allow_create_db);

/// This can be used for any SyncConfig-based SyncFeature that is limited to one-per-user
/// (which is currently all of them).
int SyncFeatureMgr_Remove(
        u64 user_id,
        ccd::SyncFeature_t sync_feature);

/// @deprecated There should be no need to use this unless we reuse the same SyncFeature_t across
///   multiple datasets simultaneously.
int SyncFeatureMgr_Remove(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature);

int SyncFeatureMgr_SetActive(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature,
        bool active);

int SyncFeatureMgr_WaitForDisable(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature);

int SyncFeatureMgr_GetStatus(
        u64 user_id,
        ccd::SyncFeature_t sync_feature,
        ccd::FeatureSyncStateType_t& state__out,
        u32& uploads_remaining__out,
        u32& downloads_remaining__out,
        bool& remote_scan_pending__out,
        bool& scan_in_progress__out);

int SyncFeatureMgr_LookupComponentByPath(
        u64 user_id,
        u64 dataset_id,
        ccd::SyncFeature_t sync_feature,
        const std::string& sync_config_relative_path,
        u64& component_id__out,
        u64& revision__out,
        bool& is_on_ans__out);

int SyncFeatureMgr_GetSyncStateForPath(
        u64 user_id,
        const std::string& abs_path,
        SyncConfigStateType_t& state__out,
        u64& dataset_id__out,
        ccd::SyncFeature_t& sync_feature__out,
        bool& is_sync_folder_root__out);

int SyncFeatureMgr_LookupAbsPath(
        u64 user_id,
        u64 dataset_id,
        u64 component_id,
        u64 revision,
        const std::string& dataset_rel_path,
        std::string& absolute_path__out,
        u64& local_modify_time__out,
        std::string& hash__out);

int SyncFeatureMgr_RemoveByUser(u64 user_id);

int SyncFeatureMgr_RemoveByDatasetId(u64 user_id, u64 dataset_id);

int SyncFeatureMgr_RemoveAll();

/// Remove any of the user's SyncConfigs that are *not* listed in \a currentDatasetIds.
int SyncFeatureMgr_RemoveDeletedDatasets(u64 user_id, const std::vector<u64>& currentDatasetIds);

/// This will call #SyncConfig::ReportLocalChange() for all underlying SyncConfigs.
void SyncFeatureMgr_RequestLocalScansForUser(u64 userId);

/// This will call #SyncConfig::ReportRemoteChange() for all underlying SyncConfigs.
void SyncFeatureMgr_RequestRemoteScansForUser(u64 userId);

void SyncFeatureMgr_RequestAllRemoteScans();

void SyncFeatureMgr_ReportRemoteChange(const vplex::syncagent::notifier::DatasetContentUpdate& notification);

/// We need to call this when any of the following relevant information changes:
/// - Online status of a device.
/// - Protocol version of a device.
/// - UserStorage "enabled" flags.
void SyncFeatureMgr_ReportDeviceAvailability(u64 deviceId);

int SyncFeatureMgr_Start();

void SyncFeatureMgr_Stop();

int SyncFeatureMgr_BlockUntilSyncDone();

/// Temporarily disable or re-enable media metadata upload.
/// @param b_enable True to enable, false to disable.
int SyncFeatureMgr_DisableEnableMetadataUpload(u64 userId, bool b_enable);

#endif // include guard
