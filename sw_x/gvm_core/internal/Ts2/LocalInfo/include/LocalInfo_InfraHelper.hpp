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

// Implementation of LocalInfo interface, for use with PxdTest::InfraHelper in standalone test programs.

#ifndef __TS2_LOCAL_INFO_INFRA_HELPER_HPP__
#define __TS2_LOCAL_INFO_INFRA_HELPER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "LocalInfo.hpp"
#include "PxdTestInfraHelper.hpp"

#include <string>

namespace Ts2 {
class LocalInfo_InfraHelper : public LocalInfo
{
public:
    LocalInfo_InfraHelper(const std::string &username, const std::string &password, const std::string &devicename);
    virtual ~LocalInfo_InfraHelper();

    virtual u64 GetUserId() const;
    virtual u64 GetDeviceId() const;
    virtual u32 GetInstanceId() const;
    virtual int GetServerTcpDinAddrPort(u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                                        VPLNet_addr_t &addr, VPLNet_port_t &port);
    virtual int GetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                 std::string &ccdSessionKey, std::string &ccdLoginBlob);
    virtual int GetCcdServerKey(std::string &ccdServerKey);
    virtual int GetPxdSessionKey(std::string &pxdSessionKey, std::string &pxdLoginBlob);
    virtual int ResetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId);
    virtual int ResetCcdServerKey();
    virtual int ResetPxdSessionKey();
    virtual const std::string &GetPxdSvrName() const;
    virtual const std::string &GetAnsSvrName() const;
    virtual int GetDisabledRouteTypes() const;
    virtual int GetIdleDinTimeout() const;
    virtual int GetIdleDexTimeout() const;
    virtual int GetIdleP2pTimeout() const;
    virtual int GetIdlePrxTimeout() const;

    // New interfaces.
    virtual void SetServerTcpDinAddrPort(u64 userId, u64 deviceId, u32 instanceId,
                                         VPLNet_addr_t addr, VPLNet_port_t port);

    int GetAnsSessionKey(std::string &ansSessionKey, std::string &ansLoginBlob);
    virtual void SetAnsNotifyCB(AnsNotifyCB callback, void* context);
    virtual void AnsNotifyIncomingClient(const char* buffer, u32 bufferLength);

    virtual int GetEvents(u64 handle, u32 maxToGet, int timeoutMs,
                          google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out);

    virtual int GetMaxSegmentSize() const;
    virtual int GetMaxWindowSize() const;
    virtual VPLTime_t GetRetransmitInterval() const;
    virtual VPLTime_t GetMinRetransmitInterval() const;
    virtual VPLTime_t GetMaxRetransmitRound() const;
    virtual VPLTime_t GetEventsTimeout() const;
    virtual int GetSendBufSize() const;
    virtual int GetDevAidParam() const;
    virtual int GetPktDropParam() const;

    virtual int GetAttemptPrxP2PDelay() const;
    virtual int GetUsePrxDelay() const;

    virtual VPLTime_t GetTsOpenTimeout() const;
    virtual VPLTime_t GetTsReadTimeout() const;
    virtual VPLTime_t GetTsWriteTimeout() const;
    virtual VPLTime_t GetTsCloseTimeout() const;

    virtual VPLTime_t GetFinWaitTimeout() const;
    virtual VPLTime_t GetPacketSendTimeout() const;

    virtual PlatformType GetPlatformType() const;

    virtual int GetDeviceCcdProtocolVersion(u64 deviceId) const;

    virtual bool IsAuthorizedClient(u64 deviceId) const;

    virtual void RegisterNetworkConnCB(void* instance, NetworkConnectedCB callback);
    virtual void DeregisterNetworkConnCB(void* instance);
    virtual void ReportNetworkConnected();

    virtual int GetTestNetworkEnv() const;

    virtual void SetAnsDeviceOnlineCB(void* instance, AnsDeviceOnlineCB callback);
    virtual void AnsReportDeviceOnline(u64 deviceId);

private:
    const std::string username;
    const std::string password;
    const std::string devicename;

    PxdTest::InfraHelper *infraHelper;

    u64 userId;
    u64 deviceId;
    u32 instanceId;

    struct AddrPort {
        AddrPort() : addr(0), port(0) {}
        VPLNet_addr_t addr;
        VPLNet_port_t port;
    };
    std::map<u64, AddrPort> serverTcpDinAddrPort;

    std::string pxdName;
    std::string ansName;

    mutable VPLMutex_t mutex;
    void (*ansNotifyCB)(const char* buffer, u32 bufferLength, void* context);
    void* cbContext;
};
}

#endif // incl guard
