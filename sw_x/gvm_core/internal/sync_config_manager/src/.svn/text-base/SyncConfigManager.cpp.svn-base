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

#include "SyncConfigManager.hpp"
#include "gvm_errors.h"
#include "log.h"
#include "vplu_mutex_autolock.hpp"

#include <set>

using namespace std;

class SyncConfigManagerImpl : public SyncConfigManager
{
public:
    VPL_DISABLE_COPY_AND_ASSIGN(SyncConfigManagerImpl);

    template<class T> inline
    bool contains(const std::set<T>& container, const T& value)
    {
        return container.find(value) != container.end();
    }
    
    /// Protects the following fields.
    VPLMutex_t mutex;
    
    set<SyncConfig*> syncConfigs;

    SyncConfigManagerImpl()
    {
        VPL_SET_UNINITIALIZED(&mutex);
    }
    
    int init()
    {
        int rv;
        rv = VPLMutex_Init(&mutex);
        if (rv < 0) {
            LOG_WARN("VPLMutex_Init failed: %d", rv);
            return rv;
        }
        return rv;
    }
    
    static SyncConfig* handleToSyncConfig(const SyncConfigHandle& handle)
    {
        return static_cast<SyncConfig*>(handle.x);
    }
    
    int SyncConfig_Add(
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
            bool allow_create_db)
    {
        int errCode;
        SyncConfig* sc = CreateSyncConfig(user_id, dataset, type, sync_policy,
                local_dir, server_dir, dataset_access_info,
                thread_pool, make_dedicated_thread,
                event_cb, callback_context, errCode, allow_create_db);
        if (sc == NULL) {
            LOG_WARN("CreateSyncConfig failed: %d", errCode);
            return errCode;
        }
        {
            MutexAutoLock lock(&mutex);
            syncConfigs.insert(sc);
        }
        sync_config_handle__out->x = sc;
        return 0;
    }

    int SyncConfig_RequestClose(
            SyncConfigHandle sync_config_handle)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        int rc;
        LOG_INFO("RequestClose(%p)", sc);
        if ((rc = sc->RequestClose()) < 0) {
            LOG_WARN("RequestClose failed: %d", rc);
        }
        return rc;
    }

    int SyncConfig_Destroy(
            SyncConfigHandle sync_config_handle)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        {
            MutexAutoLock lock(&mutex);
            size_t count = syncConfigs.erase(sc);
            if (count < 1) {
                LOG_ERROR("Requested SyncConfig is not in our list!");
                return SYNC_CONFIG_ERR_NOT_FOUND;
            }
        }
        LOG_INFO("DestroySyncConfig(%p)", sc);
        DestroySyncConfig(sc);
        LOG_INFO("Done with SyncConfig %p", sc);
        return 0;
    }
    
    int SyncConfig_Enable(
            SyncConfigHandle sync_config_handle)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->Resume();
    }
    
    int SyncConfig_Disable(
            SyncConfigHandle sync_config_handle,
            bool blocking)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->Pause(blocking);
    }
    
    int SyncConfig_SetPriority(
            SyncConfigHandle sync_config_handle,
            int priority)
    {
        return SYNC_AGENT_ERR_NOT_IMPL;
    }
    
    int SyncConfig_GetStatus(
            SyncConfigHandle sync_config_handle,
            SyncConfigStatus& status__out,
            bool& has_error__out,
            bool& work_to_do__out,
            u32& uploads_remaining__out,
            u32& downloads_remaining__out,
            bool& remote_scan_pending__out)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        int rv = sc->GetSyncStatus(status__out, has_error__out,
             work_to_do__out, uploads_remaining__out, downloads_remaining__out,
             remote_scan_pending__out);
        return rv;
    }
    
    int SyncConfig_LookupComponentByPath(
            SyncConfigHandle sync_config_handle,
            const std::string& sync_config_relative_path,
            u64& component_id__out,
            u64& revision__out,
            bool& is_on_acs__out)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->LookupComponentByPath(sync_config_relative_path,
                                         component_id__out,
                                         revision__out,
                                         is_on_acs__out);
    }

    int SyncConfig_GetSyncStateForPath(
            SyncConfigHandle sync_config_handle,
            const std::string& abs_path,
            SyncConfigStateType_t& state__out,
            u64& dataset_id__out,
            bool& is_sync_folder_root__out)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->GetSyncStateForPath(abs_path,
                    state__out,
                    dataset_id__out,
                    is_sync_folder_root__out);

    }

    int SyncConfig_LookupAbsPath(
            SyncConfigHandle sync_config_handle,
            u64 component_id,
            u64 revision,
            std::string& absolute_path__out,
            u64& local_modify_time__out,
            std::string& hash__out)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->LookupAbsPath(component_id,
                                 revision,
                                 absolute_path__out,
                                 local_modify_time__out,
                                 hash__out);
    }

    void ReportRemoteChange(
            const vplex::syncagent::notifier::DatasetContentUpdate& notification)
    {
        MutexAutoLock lock(&mutex);
        for (std::set<SyncConfig*>::iterator it = syncConfigs.begin();
             it != syncConfigs.end();
             ++it)
        {
            SyncConfig* curr = *it;
            // Skip reporting the change if the dataset/userId don't match.
            if ((notification.dataset_id() == curr->GetDatasetId()) &&
                (notification.recipient_uid() == curr->GetUserId()))
            {
                curr->ReportRemoteChange();
            }
        }
    }
    
    int ReportPossibleRemoteChange(
            SyncConfigHandle sync_config_handle)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->ReportPossibleRemoteChange();
    }

    int ReportLocalChange(
            SyncConfigHandle sync_config_handle,
            const std::string& optional_path)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->ReportLocalChange(optional_path);
    }

    int ReportArchiveStorageDeviceAvailability(
            SyncConfigHandle sync_config_handle,
            bool is_online)
    {
        SyncConfig* sc = handleToSyncConfig(sync_config_handle);
        MutexAutoLock lock(&mutex);
        if (!contains(syncConfigs, sc)) {
            return SYNC_CONFIG_ERR_NOT_FOUND;
        }
        return sc->ReportArchiveStorageDeviceAvailability(is_online);
    }
};

SyncConfigManager* CreateSyncConfigManager(int& err_code__out)
{
    SyncConfigManagerImpl* result = new SyncConfigManagerImpl();
    err_code__out = result->init();
    if (err_code__out < 0) {
        delete result;
        result = NULL;
    }
    return result;
}

void DestroySyncConfigManager(SyncConfigManager* syncConfigManager)
{
    delete syncConfigManager;
}

int SyncConfigManager::SyncConfig_Join(
        SyncConfigHandle sync_config_handle)
{
    SyncConfig* sc = SyncConfigManagerImpl::handleToSyncConfig(sync_config_handle);
    int rc;
    LOG_INFO("Join(%p)", sc);
    if ((rc = sc->Join()) < 0) {
        LOG_WARN("Join failed: %d", rc);
    }
    return rc;
}
