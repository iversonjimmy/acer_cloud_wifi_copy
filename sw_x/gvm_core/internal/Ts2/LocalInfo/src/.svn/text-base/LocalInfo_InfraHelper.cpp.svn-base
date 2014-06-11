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

#include "LocalInfo_InfraHelper.hpp"

#include <log.h>

#include "vplex_vs_directory.h"

#include <cassert>

Ts2::LocalInfo_InfraHelper::LocalInfo_InfraHelper(const std::string &username, const std::string &password, const std::string &devicename)
    : username(username), password(password), devicename(devicename),
      userId(0), deviceId(0), instanceId(0), pxdName(""), ansName(""),
      ansNotifyCB(NULL), cbContext(NULL)
{
    infraHelper = new PxdTest::InfraHelper(username, password, devicename);
    if (!infraHelper) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: No memory to create PxdTest::InfraHelper obj", this);
        return;
    }

    pxdName = infraHelper->GetPxdSvrName();
    ansName = infraHelper->GetAnsSvrName();

    int err = infraHelper->ConnectInfra(userId, deviceId);
    if (err) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: ConnectInfra failed: err %d", this, err);
        delete infraHelper;
        infraHelper = NULL;
        return;
    }

    VPLMutex_Init(&mutex);
}

Ts2::LocalInfo_InfraHelper::~LocalInfo_InfraHelper()
{
    delete infraHelper;
    VPLMutex_Destroy(&mutex);
}

u64 Ts2::LocalInfo_InfraHelper::GetUserId() const
{
    return userId;
}

u64 Ts2::LocalInfo_InfraHelper::GetDeviceId() const
{
    return deviceId;
}

u32 Ts2::LocalInfo_InfraHelper::GetInstanceId() const
{
    return instanceId;
}

int Ts2::LocalInfo_InfraHelper::GetServerTcpDinAddrPort(u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
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

int Ts2::LocalInfo_InfraHelper::GetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                                 std::string &ccdSessionKey, std::string &ccdLoginBlob)
{
    if (!infraHelper) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: No InfraHelper", this);
        return -1;
    }

    // FIXME: Need to pass ans credential
    int err = infraHelper->GetCCDLoginBlob("0", userId,
                                           remoteDeviceId, /*remoteInstanceId*/"0",
                                           "",
                                           ccdSessionKey, ccdLoginBlob);
    if (err) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: GetCCDLoginBlob failed: err %d", this, err);
    }
    return err;
}

int Ts2::LocalInfo_InfraHelper::GetCcdServerKey(std::string &ccdServerKey)
{
    if (!infraHelper) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: No InfraHelper", this);
        return -1;
    }

    int err = infraHelper->GetCCDServerKey(/*instanceId*/"0", ccdServerKey);
    if (err) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: Failed to get CcdServerKey: err %d", this, err);
    }
    return err;
}

int Ts2::LocalInfo_InfraHelper::GetPxdSessionKey(std::string &pxdSessionKey, std::string &pxdLoginBlob)
{
    if (!infraHelper) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: No InfraHelper", this);
        return -1;
    }

    // FIXME: Need to pass ans credential
    int err = infraHelper->GetPxdLoginBlob("0", "", pxdSessionKey, pxdLoginBlob);
    if (err) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: GetPxdSessionKey failed: err %d", this, err);
    }
    return err;
}

int Ts2::LocalInfo_InfraHelper::ResetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId)
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: ResetCcdSessionKey not available", this);
    return -1;
}
int Ts2::LocalInfo_InfraHelper::ResetCcdServerKey()
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: ResetCcdServerKey not available", this);
    return -1;
}

int Ts2::LocalInfo_InfraHelper::ResetPxdSessionKey()
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: ResetPxdSessionKey not available", this);
    return -1;
}

const std::string &Ts2::LocalInfo_InfraHelper::GetPxdSvrName() const
{
    if (!infraHelper) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: No InfraHelper", this);
    }
    return pxdName;
}

const std::string &Ts2::LocalInfo_InfraHelper::GetAnsSvrName() const
{
    if (!infraHelper) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: No InfraHelper", this);
    }
    return ansName;
}


void Ts2::LocalInfo_InfraHelper::SetServerTcpDinAddrPort(u64 userId, u64 deviceId, u32 instanceId,
                                                         VPLNet_addr_t addr, VPLNet_port_t port)
{
    serverTcpDinAddrPort[deviceId].addr = addr;
    serverTcpDinAddrPort[deviceId].port = port;
}

int Ts2::LocalInfo_InfraHelper::GetAnsSessionKey(std::string &ansSessionKey, std::string &ansLoginBlob)
{
    if (!infraHelper) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: No InfraHelper", this);
        return -1;
    }

    int err = infraHelper->GetAnsLoginBlob(ansSessionKey, ansLoginBlob);
    if (err) {
        LOG_ERROR("LocalInfo_InfraHelper[%p]: GetAnsLoginBlob failed: err %d", this, err);
    }
    return err;
}

int Ts2::LocalInfo_InfraHelper::GetDisabledRouteTypes() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetIdleDinTimeout() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetIdleDexTimeout() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetIdleP2pTimeout() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetIdlePrxTimeout() const
{
    return 0;
}

void Ts2::LocalInfo_InfraHelper::SetAnsNotifyCB(AnsNotifyCB callback, void* context)
{
    VPLMutex_Lock(&mutex);
    ansNotifyCB = callback;
    cbContext = context;
    VPLMutex_Unlock(&mutex);
}

void Ts2::LocalInfo_InfraHelper::AnsNotifyIncomingClient(const char* buffer, u32 bufferLength)
{
    VPLMutex_Lock(&mutex);
    if(ansNotifyCB != NULL && cbContext != NULL) {
        ansNotifyCB(buffer, bufferLength, cbContext);
    }
    VPLMutex_Unlock(&mutex);
}

int Ts2::LocalInfo_InfraHelper::CreateQueue(u64* handle_out)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: CreateQueue not available", this);
    return -1;
}

int Ts2::LocalInfo_InfraHelper::GetEvents(u64 handle, u32 maxToGet, int timeoutMs,
                                          google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out)
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: GetEvents not available", this);
    return -1;
}

int Ts2::LocalInfo_InfraHelper::DestroyQueue(u64 handle)
{
    LOG_ERROR("LocalInfo_FixedValues[%p]: DestroyQueue not available", this);
    return -1;
}

int Ts2::LocalInfo_InfraHelper::GetMaxSegmentSize() const
{
    return 5120;
}

int Ts2::LocalInfo_InfraHelper::GetMaxWindowSize() const
{
    return 3 * 5120;
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetRetransmitInterval() const
{
    return VPLTime_FromMillisec(5000);
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetMinRetransmitInterval() const
{
    return VPLTime_FromMillisec(1000);
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetMaxRetransmitRound() const
{
    return 8;
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetEventsTimeout() const
{
    return VPLTime_FromMillisec(60000);
}

int Ts2::LocalInfo_InfraHelper::GetSendBufSize() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetDevAidParam() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetPktDropParam() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetAttemptPrxP2PDelay() const
{
    return 0;
}

int Ts2::LocalInfo_InfraHelper::GetUsePrxDelay() const
{
    return 0;
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetTsOpenTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetTsReadTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetTsWriteTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetTsCloseTimeout() const
{
    return VPLTime_FromMillisec(30000);
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetFinWaitTimeout() const
{
    return VPLTime_FromMillisec(4000);
}

VPLTime_t Ts2::LocalInfo_InfraHelper::GetPacketSendTimeout() const
{
    return VPLTime_FromMillisec(18000);
}

Ts2::LocalInfo::PlatformType Ts2::LocalInfo_InfraHelper::GetPlatformType() const
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

int Ts2::LocalInfo_InfraHelper::GetDeviceCcdProtocolVersion(u64 deviceId) const
{
    return 4;  // meaning TS-ready
}

bool Ts2::LocalInfo_InfraHelper::IsAuthorizedClient(u64 deviceId) const
{
    return true;
}

void Ts2::LocalInfo_InfraHelper::RegisterNetworkConnCB(void* instance, NetworkConnectedCB callback)
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: RegisterNetworkConnCB not available", this);
}

void Ts2::LocalInfo_InfraHelper::DeregisterNetworkConnCB(void* instance)
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: DeregisterNetworkConnCB not available", this);
}

void Ts2::LocalInfo_InfraHelper::ReportNetworkConnected()
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: ReportNetworkConnected not available", this);
}

int Ts2::LocalInfo_InfraHelper::GetTestNetworkEnv() const
{
    return 0;
}

void Ts2::LocalInfo_InfraHelper::SetAnsDeviceOnlineCB(void* instance, AnsDeviceOnlineCB callback)
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: SetDeviceOnlineCB not available", this);
}

void Ts2::LocalInfo_InfraHelper::AnsReportDeviceOnline(u64 deviceId)
{
    LOG_ERROR("LocalInfo_InfraHelper[%p]: DeviceOnline not available", this);
}
