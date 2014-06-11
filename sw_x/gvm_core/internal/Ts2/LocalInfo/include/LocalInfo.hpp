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

// Interface declaration of LocalInfo.
// LocalInfo is a provider of info maintained outside Ts2.
// (E.g., local userId, deviceId, instanceId, session keys, secrets, ...)

#ifndef __TS2_LOCAL_INFO_HPP__
#define __TS2_LOCAL_INFO_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>

#include <string>
#include <ccdi_rpc.pb.h>
#include "ts_types.hpp"

namespace Ts2 {
class LocalInfo
{
public:
    LocalInfo() {}
    virtual ~LocalInfo() {}

    virtual u64 GetUserId() const = 0;
    virtual u64 GetDeviceId() const = 0;
    virtual u32 GetInstanceId() const = 0;
    virtual int GetServerTcpDinAddrPort(u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                                        VPLNet_addr_t &addr, VPLNet_port_t &port) = 0;
    virtual int GetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                 std::string &ccdSessionKey, std::string &ccdLoginBlob) = 0;
    virtual int GetCcdServerKey(std::string &ccdServerKey) = 0;
    virtual int GetPxdSessionKey(std::string &pxdSessionKey, std::string &pxdLoginBlob) = 0;
    virtual int ResetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId) = 0;
    virtual int ResetCcdServerKey() = 0;
    virtual int ResetPxdSessionKey() = 0;
    virtual const std::string &GetPxdSvrName() const = 0;
    virtual const std::string &GetAnsSvrName() const = 0;

    // [Testing Option]
    // Bitmask indicating which route types should not be attempted.
    // Possible values are some bit-OR of DISABLED_ROUTE_TYPE_*.
    virtual int GetDisabledRouteTypes() const = 0;
    enum DisabledRouteType {
        DISABLED_ROUTE_TYPE_DIN = 1,
        DISABLED_ROUTE_TYPE_DEX = 2,
        DISABLED_ROUTE_TYPE_P2P = 4,
        DISABLED_ROUTE_TYPE_PRX = 8,
    };

    // Number of seconds after which an idle connection should be closed.
    // 0 means no timeout.
    virtual int GetIdleDinTimeout() const = 0;
    virtual int GetIdleDexTimeout() const = 0;
    virtual int GetIdleP2pTimeout() const = 0;
    virtual int GetIdlePrxTimeout() const = 0;

    typedef void (*AnsNotifyCB)(const char* buffer, u32 bufferLength, void* context);
    virtual void SetAnsNotifyCB(AnsNotifyCB callback, void* context) = 0;
    virtual void AnsNotifyIncomingClient(const char* buffer, u32 bufferLength) = 0;

    // Interfaces to get CcdiEvents
    virtual int CreateQueue(u64* handle_out) = 0;
    virtual int GetEvents(u64 handle, u32 maxToGet, int timeoutMs, 
                          google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out) = 0;
    virtual int DestroyQueue(u64 handle) = 0;

    virtual int GetMaxSegmentSize() const = 0;
    virtual int GetMaxWindowSize() const = 0;
    virtual VPLTime_t GetRetransmitInterval() const = 0;
    virtual VPLTime_t GetMinRetransmitInterval() const = 0;
    virtual VPLTime_t GetMaxRetransmitRound() const = 0;
    virtual VPLTime_t GetEventsTimeout() const = 0;
    virtual int GetSendBufSize() const = 0;
    virtual int GetDevAidParam() const = 0;
    virtual int GetPktDropParam() const = 0;

    // In Seconds
    virtual int GetAttemptPrxP2PDelay() const = 0;
    // In Seconds
    virtual int GetUsePrxDelay() const = 0;

    // Timeout to use for each TS_* calls.
    virtual VPLTime_t GetTsOpenTimeout() const = 0;
    virtual VPLTime_t GetTsReadTimeout() const = 0;
    virtual VPLTime_t GetTsWriteTimeout() const = 0;
    virtual VPLTime_t GetTsCloseTimeout() const = 0;

    virtual VPLTime_t GetFinWaitTimeout() const = 0;
    virtual VPLTime_t GetPacketSendTimeout() const = 0;

    enum PlatformType {
        // 0x0 for unknown
        PLATFORM_TYPE_UNKNOWN     = 0x0,

        // 0x100 ~ 0x2ff for Linux Family
        PLATFORM_TYPE_LINUX       = 0x101,
        PLATFORM_TYPE_ANDROID     = 0x102,
        PLATFORM_TYPE_CLOUDNODE   = 0x103,

        // 0x200 ~ 0x2FF for Windows Family
        PLATFORM_TYPE_WINDOWS     = 0x201,
        PLATFORM_TYPE_WINRT       = 0x202,
        PLATFORM_TYPE_WOA         = 0x203,

        // 0x300 ~ 0x3FF for Mac Family
        PLATFORM_TYPE_IOS         = 0x301,
    };
    virtual PlatformType GetPlatformType() const = 0;

    virtual int GetDeviceCcdProtocolVersion(u64 deviceId) const = 0;

    virtual bool IsAuthorizedClient(u64 deviceId) const = 0;

    typedef void (*NetworkConnectedCB)(void* instance);
    // Register network connected callback
    virtual void RegisterNetworkConnCB(void* instance, NetworkConnectedCB callback) = 0;
    virtual void DeregisterNetworkConnCB(void* instance) = 0;
    virtual void ReportNetworkConnected() = 0;

    // For test and simulate network environment in ccd
    virtual int GetTestNetworkEnv() const = 0;

    // Callback for handling device online event
    typedef void (*AnsDeviceOnlineCB)(void* instance, u64 deviceId);
    virtual void SetAnsDeviceOnlineCB(void* instance, AnsDeviceOnlineCB callback) = 0;
    virtual void AnsReportDeviceOnline(u64 deviceId) = 0;
};
}

#endif // incl guard
