//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "DeviceStateCache.hpp"

#include "vpl_th.h"

#include "ans_connection.hpp"
#include "EventManagerPb.hpp"

#include <vector>

static VPLMutex_t s_mutex;

static ccd::DeviceConnectionStatus s_defaultStatus;

static u64 s_localDeviceId = 0;

static ccd::DeviceConnectionStatus s_localDeviceStatus;

struct Record {
    u64 deviceId;
    ccd::DeviceConnectionStatus status;
    std::string name;
    bool isStorageNode;
};

static std::vector<Record> s_records;

int DeviceStateCache_Init()
{
    s_defaultStatus.set_state(ccd::DEVICE_CONNECTION_OFFLINE);
    s_defaultStatus.set_updating(true);
    s_localDeviceStatus.set_state(ccd::DEVICE_CONNECTION_OFFLINE);
    return VPLMutex_Init(&s_mutex);
}

void DeviceStateCache_Shutdown()
{
    VPLMutex_Destroy(&s_mutex);
}

void DeviceStateCache_SetLocalDeviceId(u64 deviceId)
{
    MutexAutoLock lock(&s_mutex);
    s_localDeviceId = deviceId;
}

void DeviceStateCache_SetLocalDeviceState(ccd::DeviceConnectionState_t state)
{
    MutexAutoLock lock(&s_mutex);
    s_localDeviceStatus.set_state(state);
    { // Post an event.
        ccd::CcdiEvent* event = new ccd::CcdiEvent();
        event->mutable_device_connection_change()->set_device_id(s_localDeviceId);
        *event->mutable_device_connection_change()->mutable_status() = s_localDeviceStatus;
        EventManagerPb_AddEvent(event);
    }
}

ccd::DeviceConnectionState_t DeviceStateCache_GetLocalDeviceState()
{
    return s_localDeviceStatus.state();
}

ccd::DeviceConnectionStatus DeviceStateCache_GetDeviceStatus(u64 deviceId)
{
    MutexAutoLock lock(&s_mutex);
    if (deviceId == s_localDeviceId) {
        return s_localDeviceStatus;
    }
    for (size_t i = 0; i < s_records.size(); i++) {
        if (s_records[i].deviceId == deviceId) {
            return s_records[i].status;
        }
    }
    return s_defaultStatus;
}

/// Returns true if the set of deviceIds changed (a new deviceId was added).
/// (Changing the status and/or name doesn't count.)
static bool privUpdateDeviceList(
        u64 deviceId,
        const std::string& deviceName,
        bool isStorageNode)
{
    ASSERT(VPLMutex_LockedSelf(&s_mutex));

    for (size_t i = 0; i < s_records.size(); i++) {
        if (s_records[i].deviceId == deviceId) {
            // Found it; update the name if it changed.
            if (deviceName.compare(s_records[i].name) != 0) {
                LOG_INFO("Changing device "FMT_DeviceId" name from \"%s\" to \"%s\"", deviceId,
                        s_records[i].name.c_str(),
                        deviceName.c_str());
                s_records[i].name = deviceName;
            }
            if (s_records[i].isStorageNode != isStorageNode) {
                LOG_INFO("Changing device "FMT_DeviceId" isStorageNode to %d", deviceId,
                        isStorageNode);
                s_records[i].isStorageNode = isStorageNode;
            }
            // We didn't change the set of deviceIds.
            return false;
        }
    }
    // Not found; add it now.
    {
        Record newRecord = { deviceId, s_defaultStatus, deviceName, isStorageNode };
        LOG_INFO("Adding device "FMT_DeviceId" (%s) (%s storageNode) to cache", deviceId, deviceName.c_str(),
                isStorageNode ? "is" : "not");
        s_records.push_back(newRecord);
        return true;
    }
}

static void privUpdateDeviceStatus(
        u64 deviceId,
        const ccd::DeviceConnectionStatus& status)
{
    ASSERT(VPLMutex_LockedSelf(&s_mutex));

    for (size_t i = 0; i < s_records.size(); i++) {
        if (s_records[i].deviceId == deviceId) {
            LOG_INFO("Updating device "FMT_DeviceId" status {%s} -> {%s}", deviceId,
                    s_records[i].status.ShortDebugString().c_str(),
                    status.ShortDebugString().c_str());
            s_records[i].status = status;
            return;
        }
    }
    LOG_WARN("Device "FMT_DeviceId" not found in cache; dropping update %s", deviceId,
            status.ShortDebugString().c_str());
}

void DeviceStateCache_UpdateDeviceStatus(
        u64 deviceId,
        const ccd::DeviceConnectionStatus& status)
{
    MutexAutoLock lock(&s_mutex);
    privUpdateDeviceStatus(deviceId, status);

    if (deviceId == s_localDeviceId) {
        LOG_WARN("Got a normal device status update for the local device");
    } else {
        // Post an event.
        ccd::CcdiEvent* event = new ccd::CcdiEvent();
        event->mutable_device_connection_change()->set_device_id(deviceId);
        *event->mutable_device_connection_change()->mutable_status() = status;
        EventManagerPb_AddEvent(event);
        // event will be freed by EventManagerPb.
    }
}

void DeviceStateCache_UpdateList(
        VPLUser_Id_t userId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>& devices,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>& storageNodes)
{
    bool updateAns = false;
    UNUSED(userId); // for now, we only support 1 user.
    // Update the cache.
    {
        MutexAutoLock lock(&s_mutex);

        // First remove any existing records that aren't in the new list.
        for (size_t i = 0; i < s_records.size(); i++) {
            u64 curr = s_records[i].deviceId;
            if (!Util_IsInDeviceInfoList(curr, devices)) {
                LOG_INFO("Removing device "FMT_DeviceId" from cache", curr);
                s_records[i] = s_records[s_records.size() - 1];
                s_records.pop_back();
                updateAns = true;
                i--;
            }
        }
        // Add/update new records.
        for (int i = 0; i < devices.size(); i++) {
            const vplex::vsDirectory::DeviceInfo& curr = devices.Get(i);
            bool isStorageNode = Util_IsInUserStorageList(curr.deviceid(), storageNodes);
            updateAns |= privUpdateDeviceList(curr.deviceid(), curr.devicename(), isStorageNode);
        }
    } // release mutex

    // If there was a change, also update the ANS client.
    if (updateAns) {
        std::vector<u64> deviceIds;
        for (size_t i = 0; i < s_records.size(); i++) {
            deviceIds.push_back(s_records[i].deviceId);
        }
        // Important: must not be holding s_mutex when calling ANSConn.
        if (!ANSConn_UpdateListOfDevicesToWatch(deviceIds)) {
            LOG_ERROR("Failed to update list of devices to watch!");
        }
    }
}

void DeviceStateCache_GetStorageNodes(
        VPLUser_Id_t userId,
        google::protobuf::RepeatedPtrField<ccd::StorageNodeInfo>& result)
{
    MutexAutoLock lock(&s_mutex);
    UNUSED(userId); // for now, we only support 1 user.
    for (size_t i = 0; i < s_records.size(); i++) {
        if (s_records[i].isStorageNode) {
            ccd::StorageNodeInfo* newInfo = result.Add();
            newInfo->set_device_id(s_records[i].deviceId);
            *newInfo->mutable_status() = (s_records[i].deviceId == s_localDeviceId) ? s_localDeviceStatus : s_records[i].status;
            if (s_records[i].name.size() > 0) {
                newInfo->set_storage_name(s_records[i].name);
            }
        }
    }
}

void DeviceStateCache_MarkAllUpdating()
{
    MutexAutoLock lock(&s_mutex);
    for (size_t i = 0; i < s_records.size(); i++) {
        s_records[i].status.set_updating(true);
    }
}

void DeviceStateCache_UserLoggedOut(VPLUser_Id_t userId)
{
    UNUSED(userId); // for now, we only support 1 user.
    s_records.clear();
    DeviceStateCache_SetLocalDeviceState(ccd::DEVICE_CONNECTION_OFFLINE);
}
