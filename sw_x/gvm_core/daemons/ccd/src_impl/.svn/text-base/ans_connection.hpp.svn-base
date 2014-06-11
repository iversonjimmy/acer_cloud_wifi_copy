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

#ifndef ANS_CONNECTION_HPP
#define ANS_CONNECTION_HPP

#include "base.h"

#if CCD_USE_ANS
#include "vplu_types.h"
#include "ans_device.h"
#include <string>
#include <vector>

s32 ANSConn_Start(
        VPLUser_Id_t userId,
        u64 deviceId,
        s64 clusterId,
        const std::string& ansSessionKey,
        const std::string& ansLoginBlob);

s32 ANSConn_Stop();

VPL_BOOL ANSConn_IsActive();

VPL_BOOL ANSConn_UpdateListOfDevicesToWatch(const std::vector<u64>& deviceIds);

VPL_BOOL ANSConn_RequestSleepSetup(int ioacDeviceType, u64 asyncId, const void* macAddr, int macAddrLen);

s32 ANSConn_WakeDevice(u64 deviceId);

u64 ANSConn_GetLocalConnId();

/// Get the IP address for the local interface that is being used to connect to ANS.
VPLNet_addr_t ANSConn_GetLocalAddr();

void ANSConn_GoingToSleepCb();

void ANSConn_ResumeFromSleepCb();

void ANSConn_ReportNetworkConnected();

/// 0 indicates foreground mode.  Positive value indicates background mode.
void ANSConn_SetForegroundMode(int backgroundModeIntervalSec);

void ANSConn_PerformBackgroundTasks();

int ANSConn_SendRemoteSwUpdateRequest(u64 userId, u64 deviceId);

#endif

#endif // include guard
