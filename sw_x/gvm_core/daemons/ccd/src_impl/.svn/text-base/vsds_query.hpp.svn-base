//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VSDS_QUERY_HPP__
#define __VSDS_QUERY_HPP__

#include "cache.h"
#include "vplex_vs_directory.h"
#include <vector>
#include <string>

/// @param vsdsHostname The buffer will be copied.
s32 Query_InitVSDS(const char* vsdsHostname, u16 vsdsPort);

s32 Query_QuitVSDS(void);

/// @param vsdsHostname The buffer will be copied.
void Query_UpdateVsdsHostname(const char* vsdsHostname);

VPLVsDirectory_ProxyHandle_t Query_GetVsdsProxy(void);

void Query_ReleaseVsdsProxy(VPLVsDirectory_ProxyHandle_t proxy);

static const VPLTime_t VSDS_TIMEOUT = VPLTIME_FROM_SEC(30);

static inline
void Query_FillInVsdsSession(vplex::vsDirectory::SessionInfo* dest, const ccd::UserSession& src)
{
    dest->set_sessionhandle(src.session_handle());
    dest->set_serviceticket(src.vs_ticket());
}

static inline
void Query_FillInVsdsSession(vplex::vsDirectory::SessionInfo* dest, const ServiceSessionInfo_t& src)
{
    dest->set_sessionhandle(src.sessionHandle);
    dest->set_serviceticket(src.serviceTicket);
}

///
/// Call a function from vplex_vs_directory.h.
/// Example usage (from IAS):
///  <pre>
///    vplex::ias::RegisterVirtualDeviceRequestType registerRequest;
///    registerRequest.mutable__inherited()->set_version("2.0");
///    registerRequest.set_username(username);
///    ... <set other request fields> ...
///    vplex::ias::RegisterVirtualDeviceResponseType registerResponse;
///    rv = QUERY_IAS(VPLIas_RegisterVirtualDevice, registerRequest, registerResponse);
///  </pre>
#define QUERY_VSDS(vplFunc_, req_in_, resp_out_) \
        Query_Vsds(vplFunc_, #vplFunc_, req_in_, resp_out_)
template<typename InT, typename OutT>
static s32 Query_Vsds(
        int (*vplex_func)(VPLVsDirectory_ProxyHandle_t, VPLTime_t, const InT&, OutT&),
        const char* funcName,
        const InT& in,
        OutT& out)
{
    LOG_DEBUG("Calling %s", funcName);
    s32 rv;
    VPLVsDirectory_ProxyHandle_t proxy = Query_GetVsdsProxy();
    rv = vplex_func(proxy, VSDS_TIMEOUT, in, out);
    Query_ReleaseVsdsProxy(proxy);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", funcName, rv);
    }
    return rv;
}

//------------------------------------------

/// @deprecated
s32 Query_AddSubscriptions(u64 userId,
                           const ccd::UserSession& session,
                           u64 deviceId,
                           const std::vector<vplex::vsDirectory::Subscription>& subscriptions,
                           vplex::vsDirectory::AddSubscriptionsOutput& response_out);

s32 Query_DeleteSubscriptions(u64 userId,
                              const ccd::UserSession& session,
                              u64 deviceId,
                              const std::vector<u64>& datasetIds,
                              vplex::vsDirectory::DeleteSubscriptionsOutput& response_out);


s32 Query_LinkDevice(u64 userId,
                     const ccd::UserSession& session,
                     const std::string& deviceName,
                     const std::string& deviceClass,
                     const std::string& osVersion,
                     bool isAcerDevice,
                     bool hasCamera,
                     vplex::vsDirectory::LinkDeviceOutput& response_out);

s32 Query_UnlinkDevice(u64 userId,
                       const ccd::UserSession& session,
                       u64 deviceId,
                       vplex::vsDirectory::UnlinkDeviceOutput& response_out);

s32 Query_SetDeviceName(u64 userId,
                        const ccd::UserSession& session,
                        const std::string& deviceName,
                        vplex::vsDirectory::SetDeviceNameOutput& response_out);

s32 Query_UpdateProtocolVersion(u64 userId, const ccd::UserSession& session, const std::string& version);

s32 Query_UpdateBuildInfo(u64 userId, const ccd::UserSession& session, const std::string& buildInfo);

s32 Query_UpdateModelNumber(u64 userId, const ccd::UserSession& session, const std::string& modelNumber);

s32 Query_UpdateOsVersion(u64 userId, const ccd::UserSession& session, const std::string& version);

s32 Query_UpdateDeviceName(u64 userId, const ccd::UserSession& session, const std::string& name);

s32 Query_UpdateDeviceInfo(u64 userId,
                           const ccd::UserSession& session,
                           const std::string& deviceName,
                           const std::string& osVersion,
                           const std::string& protocolVersion,
                           const std::string& modelNumber,
                           const std::string& buildInfo);

/// Get combined results of:
/// GetLinkedDevices, GetSubscriptionDetailsForDevice, ListOwnedDataSets, ListUserStorage, GetUserStorageAddress
s32 Query_GetCloudInfo(u64 userId, const ccd::UserSession& session, u64 deviceId,
                       const std::string& version, vplex::vsDirectory::GetCloudInfoOutput& response_out);

#endif // include guard
