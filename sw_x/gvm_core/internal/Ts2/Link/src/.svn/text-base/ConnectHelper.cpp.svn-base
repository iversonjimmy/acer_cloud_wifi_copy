/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#include "ConnectHelper.hpp"

#include <cassert>

Ts2::Link::ConnectHelper::ConnectHelper(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                        ConnectedSocketHandler connectedSocketHandler, void *connectedSocketHandler_context,
                                        LocalInfo *localInfo)
    : remoteUserId(remoteUserId), remoteDeviceId(remoteDeviceId), remoteInstanceId(remoteInstanceId),
      connectedSocketHandler(connectedSocketHandler), connectedSocketHandler_context(connectedSocketHandler_context),
      localInfo(localInfo)
{
    assert(remoteUserId);
    assert(remoteDeviceId);
    assert(connectedSocketHandler);
    assert(localInfo);

    VPLMutex_Init(&mutex);
}

Ts2::Link::ConnectHelper::~ConnectHelper()
{
    VPLMutex_Destroy(&mutex);
}

u64 Ts2::Link::ConnectHelper::GetRemoteUserId() const
{
    // No need to hold a lock, as it is a const.
    return remoteUserId;
}

u64 Ts2::Link::ConnectHelper::GetRemoteDeviceId() const
{
    // No need to hold a lock, as it is a const.
    return remoteDeviceId;
}

u32 Ts2::Link::ConnectHelper::GetRemoteInstanceId() const
{
    // No need to hold a lock, as it is a const.
    return remoteInstanceId;
}

