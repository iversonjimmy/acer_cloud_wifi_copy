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

#include "LocalInfo_FixedValues.hpp"

#include <log.h>

#include <vplu_format.h>

#include <cassert>

Ts2::LocalInfo_FixedValues::LocalInfo_FixedValues(u64 userId, u64 deviceId, u32 instanceId)
    : userId(userId), deviceId(deviceId), instanceId(instanceId), dummy("")
{
}

Ts2::LocalInfo_FixedValues::~LocalInfo_FixedValues()
{
}

u64 Ts2::LocalInfo_FixedValues::GetUserId() const
{
    return userId;
}

u64 Ts2::LocalInfo_FixedValues::GetDeviceId() const
{
    return deviceId;
}

u32 Ts2::LocalInfo_FixedValues::GetInstanceId() const
{
    return instanceId;
}

int Ts2::LocalInfo_FixedValues::GetServerTcpDinAddrPort(u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                                                        VPLNet_addr_t &addr, VPLNet_port_t &port)
{
    std::map<u64, AddrPort>::const_iterator it = serverTcpDinAddrPort.find(serverDeviceId);
    if (it == serverTcpDinAddrPort.end()) {
        return -1;
    }
    addr = it->second.addr;
    port = it->second.port;
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                                 std::string &ccdSessionKey, std::string &ccdLoginBlob)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: GetCcdSessionKey not available", this);
    return -1;
}

int Ts2::LocalInfo_FixedValues::GetCcdServerKey(std::string &ccdServerKey)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: GetCcdServerKey not available", this);
    return -1;
}

int Ts2::LocalInfo_FixedValues::GetPxdSessionKey(std::string &pxdSessionKey, std::string &pxdLoginBlob)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: GetPxdSessionKey not available", this);
    return -1;
}

int Ts2::LocalInfo_FixedValues::ResetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: ResetCcdSessionKey not available", this);
    return -1;
}
int Ts2::LocalInfo_FixedValues::ResetCcdServerKey()
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: ResetCcdServerKey not available", this);
    return -1;
}

int Ts2::LocalInfo_FixedValues::ResetPxdSessionKey()
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: ResetPxdSessionKey not available", this);
    return -1;
}

const std::string &Ts2::LocalInfo_FixedValues::GetPxdSvrName() const
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: GetPxdSvrName not available", this);
    return dummy;
}

const std::string &Ts2::LocalInfo_FixedValues::GetAnsSvrName() const
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: GetAnsSvrName not available", this);
    return dummy;
}

int Ts2::LocalInfo_FixedValues::GetDisabledRouteTypes() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetIdleDinTimeout() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetIdleDexTimeout() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetIdleP2pTimeout() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetIdlePrxTimeout() const
{
    return 0;
}

void Ts2::LocalInfo_FixedValues::SetServerTcpDinAddrPort(u64 userId, u64 deviceId, u32 instanceId,
                                                         VPLNet_addr_t addr, VPLNet_port_t port)
{
    serverTcpDinAddrPort[deviceId].addr = addr;
    serverTcpDinAddrPort[deviceId].port = port;
}

void Ts2::LocalInfo_FixedValues::SetAnsNotifyCB(AnsNotifyCB callback, void* context)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: SetAnsNotifyCB not available", this);
}

void Ts2::LocalInfo_FixedValues::AnsNotifyIncomingClient(const char* buffer, u32 bufferLength)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: AnsNotifyIncomingClient not available", this);
}

int Ts2::LocalInfo_FixedValues::CreateQueue(u64* handle_out)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: CreateQueue not available", this);
    return -1;
}

int Ts2::LocalInfo_FixedValues::GetEvents(u64 handle, u32 maxToGet, int timeoutMs,
                                          google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: GetEvents not available", this);
    return -1;
}

int Ts2::LocalInfo_FixedValues::DestroyQueue(u64 handle)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: DestroyQueue not available", this);
    return -1;
}

int Ts2::LocalInfo_FixedValues::GetMaxSegmentSize() const
{
    return 5120;
}

int Ts2::LocalInfo_FixedValues::GetMaxWindowSize() const
{
    return 3 * 5120;
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetRetransmitInterval() const
{
    return VPLTime_FromMillisec(5000);
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetMinRetransmitInterval() const
{
    return VPLTime_FromMillisec(1000);
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetMaxRetransmitRound() const
{
    return 8;
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetEventsTimeout() const
{
    return VPLTime_FromMillisec(60000);
}

int Ts2::LocalInfo_FixedValues::GetSendBufSize() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetDevAidParam() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetPktDropParam() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetAttemptPrxP2PDelay() const
{
    return 0;
}

int Ts2::LocalInfo_FixedValues::GetUsePrxDelay() const
{
    return 0;
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetTsOpenTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetTsReadTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetTsWriteTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetTsCloseTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetFinWaitTimeout() const
{
    return VPLTime_FromMillisec(4000);
}

VPLTime_t Ts2::LocalInfo_FixedValues::GetPacketSendTimeout() const
{
    return VPLTime_FromMillisec(18000);
}

Ts2::LocalInfo::PlatformType Ts2::LocalInfo_FixedValues::GetPlatformType() const
{
    Ts2::LocalInfo::PlatformType type = Ts2::LocalInfo::PLATFORM_TYPE_UNKNOWN;
#if defined(LINUX) && !defined(CLOUDNODE) && !defined(ANDROID)
    type = Ts2::LocalInfo::PLATFORM_TYPE_LINUX;
#elif defined(ANDROID)
    type = Ts2::LocalInfo::PLATFORM_TYPE_ANDROID;
#elif defined(CLOUDNODE)
    type = Ts2::LocalInfo::PLATFORM_TYPE_CLOUDNODE;
#elif defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    type = Ts2::LocalInfo::PLATFORM_TYPE_WINDOWS;
#elif defined(VPL_PLAT_IS_WINRT)
    type = Ts2::LocalInfo::PLATFORM_TYPE_WINRT;
#elif defined(IOS)
    type = Ts2::LocalInfo::PLATFORM_TYPE_IOS;
#endif
    return type;
}

int Ts2::LocalInfo_FixedValues::GetDeviceCcdProtocolVersion(u64 deviceId) const
{
    return 4;  // meaning TS-ready
}

bool Ts2::LocalInfo_FixedValues::IsAuthorizedClient(u64 deviceId) const
{
    return true;
}

void Ts2::LocalInfo_FixedValues::RegisterNetworkConnCB(void* instance, NetworkConnectedCB callback)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: RegisterNetworkConnCB not available", this);
}

void Ts2::LocalInfo_FixedValues::DeregisterNetworkConnCB(void* instance)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: DeregisterNetworkConnCB not available", this);
}

void Ts2::LocalInfo_FixedValues::ReportNetworkConnected()
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: ReportNetworkConnected not available", this);
}

int Ts2::LocalInfo_FixedValues::GetTestNetworkEnv() const
{
    return 0;
}

void Ts2::LocalInfo_FixedValues::SetAnsDeviceOnlineCB(void* instance, AnsDeviceOnlineCB callback)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: SetDeviceOnlineCB not available", this);
}

void Ts2::LocalInfo_FixedValues::AnsReportDeviceOnline(u64 deviceId)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: DeviceOnline not available", this);
}
