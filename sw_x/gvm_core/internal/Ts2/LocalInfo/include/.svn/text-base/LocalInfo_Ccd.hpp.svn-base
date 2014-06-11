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

// Implementation of LocalInfo interface, for use with Ccd.

#ifndef __TS2_LOCAL_INFO_CCD_HPP__
#define __TS2_LOCAL_INFO_CCD_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_th.h>

#include <string>
#include <map>

#include "LocalInfo.hpp"

#define CRED_DROP_LIMIT 3

namespace Ts2 {
class LocalInfo_Ccd : public LocalInfo
{
public:
    LocalInfo_Ccd(u64 userId, u64 deviceId, u32 instanceId);
    virtual ~LocalInfo_Ccd();

    // Inherited interfaces.
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

    virtual void SetAnsNotifyCB(AnsNotifyCB callback, void* context);
    virtual void AnsNotifyIncomingClient(const char* buffer, u32 bufferLength);

    virtual int CreateQueue(u64* handle_out);
    virtual int GetEvents(u64 handle, u32 maxToGet, int timeoutMs,
                          google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out);
    virtual int DestroyQueue(u64 handle);
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
    const u64 userId;
    const u64 deviceId;
    const u32 instanceId;

    mutable u64 clusterId;
    mutable std::string pxdSvrName;
    mutable std::string ansSvrName;
    int fillClusterId() const;

    mutable VPLMutex_t mutex;
    void (*ansNotifyCB)(const char* buffer, u32 bufferLength, void* context);
    void* cbContext;

    std::map<void*, NetworkConnectedCB> networkConnCBMap;

    u32 resetCcdSessionKeyCount;
    u32 resetCcdServerKeyCount;
    u32 resetPxdSessionKeyCount;

    void autoRetryTsCredQuery(TSCredQuery_t *query, int maxAttempts, VPLTime_t interval);
    int tryGetServerTcpDinAddrPortFromCcd(u64 deviceId, bool useCache, VPLNet_addr_t &addr, VPLNet_port_t &port);

    void (*ansDeviceOnlineCB)(void* instance, u64 deviceId);
    void* ansDeviceOnlineCBInstance;
};
}

#endif // incl guard
