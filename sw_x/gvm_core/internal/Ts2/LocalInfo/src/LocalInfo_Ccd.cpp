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

// IMPORTANT: This file must be compiled as part of libcc.

#include "LocalInfo_Ccd.hpp"

#include "LanDeviceInfoCache.hpp"

#include "EventManagerPb.hpp"

#include <log.h>

#include <cache.h>
#include <ccdi.hpp>
#include <config.h>

#include <vplex_vs_directory.h>

#include <cassert>
#include <sstream>

Ts2::LocalInfo_Ccd::LocalInfo_Ccd(u64 userId, u64 deviceId, u32 instanceId)
    : userId(userId), deviceId(deviceId), instanceId(instanceId),
      clusterId(0), ansNotifyCB(NULL), cbContext(NULL),
      resetCcdSessionKeyCount(0), resetCcdServerKeyCount(0), resetPxdSessionKeyCount(0),
      ansDeviceOnlineCB(NULL), ansDeviceOnlineCBInstance(NULL)

{
    VPLMutex_Init(&mutex);
}

Ts2::LocalInfo_Ccd::~LocalInfo_Ccd()
{
    VPLMutex_Destroy(&mutex);
}

u64 Ts2::LocalInfo_Ccd::GetUserId() const
{
    // No need to take the mutex, because userId is const.
    return userId;
}

u64 Ts2::LocalInfo_Ccd::GetDeviceId() const
{
    // No need to take the mutex, because deviceId is const.
    return deviceId;
}

u32 Ts2::LocalInfo_Ccd::GetInstanceId() const
{
    // No need to take the mutex, because instanceId is const.
    return instanceId;
}

int Ts2::LocalInfo_Ccd::GetServerTcpDinAddrPort(u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                                                VPLNet_addr_t &addr, VPLNet_port_t &port)
{
    int err = 0;

    // Step 1: try to find info in CCD Cache.
    err = tryGetServerTcpDinAddrPortFromCcd(serverDeviceId, /*useCache*/true, addr, port);
    if (!err) {
        return err;
    }

    // Step 2: try contact infra for info.
    if (err == CCD_ERROR_NOT_FOUND) {
        LOG_INFO("Failed to find TcpDin addr:port of device "FMTu64" in Cache", serverDeviceId);
        err = tryGetServerTcpDinAddrPortFromCcd(serverDeviceId, /*useCache*/false, addr, port);
    }

    return err;
}

int Ts2::LocalInfo_Ccd::GetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                         std::string &ccdSessionKey, std::string &ccdLoginBlob)
{
    TSCredQuery_t query;
    query.user_id = userId;
    query.type = TS_CRED_QUERY_CCD_CRED;
    query.target_svr_user_id = remoteUserId;
    query.target_svr_device_id = remoteDeviceId;
    query.target_svr_instance_id = remoteInstanceId;
    query.resetCred = false;
    autoRetryTsCredQuery(&query, /*maxAttempts*/2, VPLTime_FromSec(3));
    if (query.result == CCD_OK) {
        ccdSessionKey = query.resp_key;
        ccdLoginBlob = query.resp_blob;
    }
    return query.result;
}

int Ts2::LocalInfo_Ccd::GetCcdServerKey(std::string &ccdServerKey)
{
    TSCredQuery_t query;
    query.user_id = userId;
    query.type = TS_CRED_QUERY_SVR_KEY;
    query.resetCred = false;
    autoRetryTsCredQuery(&query, /*maxAttempts*/2, VPLTime_FromSec(3));
    if (query.result == CCD_OK) {
        ccdServerKey = query.resp_key;
    }
    return query.result;
}

int Ts2::LocalInfo_Ccd::GetPxdSessionKey(std::string &pxdSessionKey, std::string &pxdLoginBlob)
{
    TSCredQuery_t query;
    query.user_id = userId;
    query.type = TS_CRED_QUERY_PXD_CRED;
    query.resetCred = false;
    autoRetryTsCredQuery(&query, /*maxAttempts*/2, VPLTime_FromSec(3));
    if (query.result == CCD_OK) {
        pxdSessionKey = query.resp_key;
        pxdLoginBlob = query.resp_blob;
    }
    return query.result;
}

int Ts2::LocalInfo_Ccd::ResetCcdSessionKey(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId)
{
    if(++resetCcdSessionKeyCount > CRED_DROP_LIMIT) {
        LOG_WARN("Reach maximum drop limit for ccd session key.");
        return TS_ERR_INVALID;
    }

    TSCredQuery_t query;
    query.user_id = userId;
    query.type = TS_CRED_QUERY_CCD_CRED;
    query.target_svr_user_id = remoteUserId;
    query.target_svr_device_id = remoteDeviceId;
    query.target_svr_instance_id = remoteInstanceId;
    query.resetCred = true;
    Cache_TSCredQuery(&query);
    return query.result;
}

int Ts2::LocalInfo_Ccd::ResetCcdServerKey()
{
    if(++resetCcdServerKeyCount > CRED_DROP_LIMIT) {
        LOG_WARN("Reach maximum drop limit for ccd server key.");
        return TS_ERR_INVALID;
    }

    TSCredQuery_t query;
    query.user_id = userId;
    query.type = TS_CRED_QUERY_SVR_KEY;
    query.resetCred = true;
    Cache_TSCredQuery(&query);
    return query.result;
}

int Ts2::LocalInfo_Ccd::ResetPxdSessionKey()
{
    if(++resetPxdSessionKeyCount > CRED_DROP_LIMIT) {
        LOG_WARN("Reach maximum drop limit for pxd session key.");
        return TS_ERR_INVALID;
    }

    TSCredQuery_t query;
    query.user_id = userId;
    query.type = TS_CRED_QUERY_PXD_CRED;
    query.resetCred = true;
    Cache_TSCredQuery(&query);
    return query.result;
}

const std::string &Ts2::LocalInfo_Ccd::GetPxdSvrName() const
{
    MutexAutoLock lock(&mutex);

    if (pxdSvrName.empty()) {
        if (!clusterId) {
            fillClusterId();
        }
        if (!clusterId) {
            goto end;
        }

        std::ostringstream oss;
        oss << "pxd-c" << clusterId << "." << __ccdConfig.infraDomain;
        pxdSvrName.assign(oss.str());
    }

 end:
    return pxdSvrName;
}

const std::string &Ts2::LocalInfo_Ccd::GetAnsSvrName() const
{
    MutexAutoLock lock(&mutex);

    if (ansSvrName.empty()) {
        if (!clusterId) {
            fillClusterId();
        }
        if (!clusterId) {
            goto end;
        }

        std::ostringstream oss;
        oss << "ans-c" << clusterId << "." << __ccdConfig.infraDomain;
        ansSvrName.assign(oss.str());
    }

 end:
    return ansSvrName;
}

int Ts2::LocalInfo_Ccd::GetDisabledRouteTypes() const
{
    return __ccdConfig.clearfiMode;
}

int Ts2::LocalInfo_Ccd::GetIdleDinTimeout() const
{
    return __ccdConfig.dinConnIdleTimeout;
}

int Ts2::LocalInfo_Ccd::GetIdleDexTimeout() const
{
    return __ccdConfig.dexConnIdleTimeout;
}

int Ts2::LocalInfo_Ccd::GetIdleP2pTimeout() const
{
    return __ccdConfig.p2pConnIdleTimeout;
}

int Ts2::LocalInfo_Ccd::GetIdlePrxTimeout() const
{
    return __ccdConfig.prxConnIdleTimeout;
}

int Ts2::LocalInfo_Ccd::fillClusterId() const
{
    CacheAutoLock autoLock;
    int err = autoLock.LockForRead();
    if (err < 0) {
        LOG_ERROR("Failed to obtain lock");
        return err;
    }

    CachePlayer* user = cache_getUserByUserId(userId);
    if (user == NULL) {
        LOG_INFO("User is no longer logged in");
        return CCD_ERROR_NOT_SIGNED_IN;
    }

    assert(VPLMutex_LockedSelf(&mutex));
    clusterId = user->cluster_id();
    return 0;
}

void Ts2::LocalInfo_Ccd::SetAnsNotifyCB(AnsNotifyCB callback, void* context)
{
    VPLMutex_Lock(&mutex);
    ansNotifyCB = callback;
    cbContext = context;
    VPLMutex_Unlock(&mutex);
}

void Ts2::LocalInfo_Ccd::AnsNotifyIncomingClient(const char* buffer, u32 bufferLength)
{
    VPLMutex_Lock(&mutex);
    if(ansNotifyCB != NULL && cbContext != NULL) {
        ansNotifyCB(buffer, bufferLength, cbContext);
    } else {
        LOG_ERROR("ANS notification callback is NULL");
    }
    VPLMutex_Unlock(&mutex);
}

int Ts2::LocalInfo_Ccd::CreateQueue(u64* handle_out)
{
    return EventManagerPb_CreateQueue(handle_out);
}

int Ts2::LocalInfo_Ccd::GetEvents(u64 handle, u32 maxToGet, int timeoutMs,
                                  google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out)
{
    return EventManagerPb_GetEvents(handle, maxToGet, timeoutMs, events_out);
}

int Ts2::LocalInfo_Ccd::DestroyQueue(u64 handle)
{
    return EventManagerPb_DestroyQueue(handle);
}

int Ts2::LocalInfo_Ccd::GetMaxSegmentSize() const
{
    return __ccdConfig.ts2MaxSegmentSize;
}

int Ts2::LocalInfo_Ccd::GetMaxWindowSize() const
{
    return __ccdConfig.ts2MaxWindowSize;
}

VPLTime_t Ts2::LocalInfo_Ccd::GetRetransmitInterval() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2RetransmitIntervalMs);
}

VPLTime_t Ts2::LocalInfo_Ccd::GetMinRetransmitInterval() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2MinRetransmitIntervalMs);
}

VPLTime_t Ts2::LocalInfo_Ccd::GetMaxRetransmitRound() const
{
    return __ccdConfig.ts2MaxRetransmitRound;
}

VPLTime_t Ts2::LocalInfo_Ccd::GetEventsTimeout() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2EventsTimeoutMs);
}

int Ts2::LocalInfo_Ccd::GetSendBufSize() const
{
    return __ccdConfig.ts2SendBufSize;
}

int Ts2::LocalInfo_Ccd::GetDevAidParam() const
{
    return __ccdConfig.ts2DevAidParam;
}

int Ts2::LocalInfo_Ccd::GetPktDropParam() const
{
    return __ccdConfig.ts2PktDropParam;
}

int Ts2::LocalInfo_Ccd::GetAttemptPrxP2PDelay() const
{
    return __ccdConfig.ts2AttemptPrxP2PDelaySec;
}

int Ts2::LocalInfo_Ccd::GetUsePrxDelay() const
{
    return __ccdConfig.ts2UsePrxDelaySec;
}

VPLTime_t Ts2::LocalInfo_Ccd::GetTsOpenTimeout() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2TsOpenTimeoutMs);
}

VPLTime_t Ts2::LocalInfo_Ccd::GetTsReadTimeout() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2TsReadTimeoutMs);
}

VPLTime_t Ts2::LocalInfo_Ccd::GetTsWriteTimeout() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2TsWriteTimeoutMs);
}

VPLTime_t Ts2::LocalInfo_Ccd::GetTsCloseTimeout() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2TsCloseTimeoutMs);
}

VPLTime_t Ts2::LocalInfo_Ccd::GetFinWaitTimeout() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2FinWaitTimeoutMs);
}

VPLTime_t Ts2::LocalInfo_Ccd::GetPacketSendTimeout() const
{
    return VPLTime_FromMillisec(__ccdConfig.ts2PacketSendTimeoutMs);
}

void Ts2::LocalInfo_Ccd::autoRetryTsCredQuery(TSCredQuery_t *query, int maxAttempts, VPLTime_t interval)
{
    for (int i = 0; i < maxAttempts; i++) {
        Cache_TSCredQuery(query);
        if (query->result) {  // error
            VPLThread_Sleep(interval);
            continue;
        }
        break;
    }
}

int Ts2::LocalInfo_Ccd::tryGetServerTcpDinAddrPortFromCcd(u64 deviceId, bool useCache, VPLNet_addr_t &addr, VPLNet_port_t &port)
{
    int err = 0;

    port = 0;  // also means "not found yet"

    // Step 1: try LAN discovery
    {
        ccd::LanDeviceInfo lanDeviceInfo;
        err = LanDeviceInfoCache::Instance().getLanDeviceByDeviceId(deviceId, &lanDeviceInfo);
        if (!err) {
            LOG_INFO("Device "FMTu64" found in LanDeviceInfoCache", deviceId);
            const ccd::LanDeviceRouteInfo &routeInfo = lanDeviceInfo.route_info();
            if (routeInfo.has_ip_v4_address() && routeInfo.has_tunnel_service_port() && routeInfo.tunnel_service_port() > 0) {
                // Found all info via LAN discovery.
                LOG_INFO("Found all needed info in LanDeviceInfoCache");
                addr = VPLNet_GetAddr(routeInfo.ip_v4_address().c_str());
                port = routeInfo.tunnel_service_port();
                goto end;
            }
            else {
                LOG_INFO("Not all needed info were found in LanDeviceInfoCache");
            }
        }
        // Info not found via LAN discovery - continue to Step 2.
    }

    // Step 2: try CCD Cache
    {
        ccd::ListUserStorageInput req;
        ccd::ListUserStorageOutput resp;
        req.set_user_id(userId);
        req.set_only_use_cache(useCache);
        err = CCDIListUserStorage(req, resp);
        if (err) {
            LOG_ERROR("CCDIListUserStorage failed: err %d", err);
            return err;
        }

        for (int i = 0; i < resp.user_storage_size(); i++) {
            const vplex::vsDirectory::UserStorage &us = resp.user_storage(i);
            if (us.storageclusterid() != deviceId) {
                continue;
            }
            for (int j = 0; j < us.storageaccess_size(); j++) {
                const vplex::vsDirectory::StorageAccess &sa = us.storageaccess(j);
                if (sa.routetype() != vplex::vsDirectory::DIRECT_INTERNAL &&
                    sa.routetype() != vplex::vsDirectory::DIRECT_EXTERNAL) {
                    continue;
                }
                for (int k = 0; k < sa.ports_size(); k++) {
                    const vplex::vsDirectory::StorageAccessPort &sap = sa.ports(k);
                    if (sap.porttype() != vplex::vsDirectory::PORT_CLEARFI) {
                        continue;
                    }
                    addr = VPLNet_GetAddr(sa.server().c_str());
                    port = sap.port();
                    if (sa.routetype() == vplex::vsDirectory::DIRECT_INTERNAL) {
                        goto end;
                    }
                    // ELSE continue to search for DIRECT_INTERNAL
                }
            }
        }
    }

 end:
    return port ? 0 : CCD_ERROR_NOT_FOUND;
}

Ts2::LocalInfo::PlatformType Ts2::LocalInfo_Ccd::GetPlatformType() const
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

int Ts2::LocalInfo_Ccd::GetDeviceCcdProtocolVersion(u64 deviceId) const
{
    return Cache_GetDeviceCcdProtocolVersion(userId, /*useOnlyCache*/true, deviceId);
}

bool Ts2::LocalInfo_Ccd::IsAuthorizedClient(u64 deviceId) const
{
    return GetDeviceCcdProtocolVersion(deviceId) >= 0;
}

void Ts2::LocalInfo_Ccd::RegisterNetworkConnCB(void* instance, NetworkConnectedCB callback)
{
    MutexAutoLock lock(&mutex);
    networkConnCBMap[instance] = callback;
}

void Ts2::LocalInfo_Ccd::DeregisterNetworkConnCB(void* instance)
{
    MutexAutoLock lock(&mutex);
    networkConnCBMap.erase(instance);
}

void Ts2::LocalInfo_Ccd::ReportNetworkConnected()
{
    MutexAutoLock lock(&mutex);
    for(std::map<void*, NetworkConnectedCB>::iterator it = networkConnCBMap.begin();
        it != networkConnCBMap.end();
        it++) {
        if(it->second != NULL) {
            it->second(it->first);
        } else {
            LOG_ERROR("Network connected callback is NULL, instance %p", it->first);
        }
    }
}

int Ts2::LocalInfo_Ccd::GetTestNetworkEnv() const
{
    return __ccdConfig.ts2TestNetworkEnv;
}

void Ts2::LocalInfo_Ccd::SetAnsDeviceOnlineCB(void* instance, AnsDeviceOnlineCB callback)
{
    VPLMutex_Lock(&mutex);
    ansDeviceOnlineCB = callback;
    ansDeviceOnlineCBInstance = instance;
    VPLMutex_Unlock(&mutex);
}

void Ts2::LocalInfo_Ccd::AnsReportDeviceOnline(u64 deviceId)
{
    VPLMutex_Lock(&mutex);
    if(ansDeviceOnlineCB != NULL && ansDeviceOnlineCBInstance != NULL) {
        ansDeviceOnlineCB(ansDeviceOnlineCBInstance, deviceId);
    }
    VPLMutex_Unlock(&mutex);
}
