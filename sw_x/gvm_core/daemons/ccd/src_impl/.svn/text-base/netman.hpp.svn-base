/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef NETMAN_HPP__
#define NETMAN_HPP__

//============================================================================
/// @file
/// Network monitoring and management subsystem.
//============================================================================

#include "base.h"
#include "vpl_net.h"
#include <ccdi_rpc.pb.h>

s32 NetMan_SetGlobalAccessDataPath(const char* globalAccessDataPath);

s32 NetMan_Start();

s32 NetMan_Stop();

/// @param asyncId ans_device asyncId for the request that originated this.
/// @param wakeupKey Secret required to wakeup the machine.
/// @param wakeupKeyLen Length of \a wakeupKey, should be 6 bytes.
/// @param serverHostname Server host to send the keep-alive packet to.
/// @param serverPort Server port to send the keep-alive packet to.
/// @param packetIntervalSec How often to send the keep-alive packet.
/// @param payload Payload to be sent in the keep-alive packet.
/// @param payloadLen Number of bytes in \a payload.
s32 NetMan_SetWakeOnWlanData(
        u64 asyncId,
        const void* wakeupKey,
        size_t wakeupKeyLen,
        const char* serverHostname,
        VPLNet_port_t serverPort,
        u32 packetIntervalSec,
        const void* payload,
        size_t payloadLen);

s32 NetMan_ClearWakeOnWlanData();

void NetMan_CheckAndClearCache();

/// Performs any tasks that need to occur when switching networks or access points.
void NetMan_NetworkChange();

ccd::IoacOverallStatus NetMan_GetIoacOverallStatus();

void NetMan_CheckWakeReason();

s32 NetMan_EnableIOAC(u64 device_id, bool enable);

s32 NetMan_IsIOACAlreadyInUse(bool& in_use);

s32 NetMan_IsUserEnabledIOAC(u64 device_id, bool& enabled);

#endif // include guard
