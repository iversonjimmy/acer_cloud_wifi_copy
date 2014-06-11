//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __DEVICE_STATE_CACHE_HPP__
#define __DEVICE_STATE_CACHE_HPP__

#include "cache.h"

int DeviceStateCache_Init();

void DeviceStateCache_Shutdown();

void DeviceStateCache_SetLocalDeviceId(u64 deviceId);

void DeviceStateCache_SetLocalDeviceState(ccd::DeviceConnectionState_t state);

ccd::DeviceConnectionState_t DeviceStateCache_GetLocalDeviceState();

ccd::DeviceConnectionStatus DeviceStateCache_GetDeviceStatus(u64 deviceId);

/// @deprecated shouldn't need this anymore.
void DeviceStateCache_GetStorageNodes(
        VPLUser_Id_t userId,
        google::protobuf::RepeatedPtrField<ccd::StorageNodeInfo>& result);

void DeviceStateCache_MarkAllUpdating();

void DeviceStateCache_UpdateDeviceStatus(
        u64 deviceId,
        const ccd::DeviceConnectionStatus& status);

void DeviceStateCache_UpdateList(
        VPLUser_Id_t userId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>& devices,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>& storageNodes);

void DeviceStateCache_UserLoggedOut(VPLUser_Id_t userId);

#endif // include guard
