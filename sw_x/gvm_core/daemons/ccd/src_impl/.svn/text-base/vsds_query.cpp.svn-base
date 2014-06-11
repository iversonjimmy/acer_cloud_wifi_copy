//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vsds_query.hpp"

#include "vpl_th.h"
#include "vplex_vs_directory.h"
#include "vplex_vs_directory_service_types.pb.h"
#include "escore.h"

#include "cache.h"
#include "config.h"
#include "query.h"
#include "ccd_build_info.h"

using namespace std;
using namespace ccd;

static VPLVsDirectory_ProxyHandle_t __vsds;
static char __vsdsHostname[HOSTNAME_MAX_LENGTH];
static VPLMutex_t __vsdsMutex; // VSDS proxy is not currently threadsafe

s32 Query_InitVSDS(const char* vsdsHostname, u16 vsdsPort)
{
    int rv = 0;

    rv = VPLMutex_Init(&__vsdsMutex);
    if (rv != 0) {
        LOG_ERROR("VPLMutex_Init returned %d", rv);
        rv = CCD_ERROR_INTERNAL;
        goto out;
    }

    Query_UpdateVsdsHostname(vsdsHostname);

    rv = VPLVsDirectory_CreateProxy(__vsdsHostname, vsdsPort, &__vsds);
    if (rv != 0) {
        LOG_ERROR("VPLVsDirectory_CreateProxy returned %d.", rv);
        goto out;
    }

 out:
    return rv;
}

s32 Query_QuitVSDS(void)
{
    return VPLVsDirectory_DestroyProxy(__vsds);
}

void Query_UpdateVsdsHostname(const char* vsdsHostname)
{
    VPLMutex_Lock(&__vsdsMutex);
    strncpy(__vsdsHostname, vsdsHostname, ARRAY_SIZE_IN_BYTES(__vsdsHostname));
    VPLMutex_Unlock(&__vsdsMutex);
}

VPLVsDirectory_ProxyHandle_t Query_GetVsdsProxy(void)
{
    VPLMutex_Lock(&__vsdsMutex);
    return __vsds;
}

void Query_ReleaseVsdsProxy(VPLVsDirectory_ProxyHandle_t proxy)
{
    UNUSED(proxy); // vsds proxy is not currently threadsafe
    VPLMutex_Unlock(&__vsdsMutex);
}

/// @deprecated
s32 Query_AddSubscriptions(u64 userId,
                           const ccd::UserSession& session,
                           u64 deviceId,
                           const std::vector<vplex::vsDirectory::Subscription>& subscriptions,
                           vplex::vsDirectory::AddSubscriptionsOutput& response_out)
{
    int rv = 0;
    vplex::vsDirectory::AddSubscriptionsInput req;

    Query_FillInVsdsSession(req.mutable_session(), session);
    req.set_userid(userId);
    req.set_deviceid(deviceId);
    for(size_t index = 0; index < subscriptions.size(); index++) {
        *(req.add_subscriptions()) = subscriptions[index];
    }

    LOG_INFO("Query_AddSubscriptions userId:"FMTu64" deviceId:"FMTu64, userId, deviceId);
    rv = QUERY_VSDS(VPLVsDirectory_AddSubscriptions, req, response_out);
    return rv;
}

s32 Query_DeleteSubscriptions(u64 userId,
                              const ccd::UserSession& session,
                              u64 deviceId,
                              const std::vector<u64>& datasetIds,
                              vplex::vsDirectory::DeleteSubscriptionsOutput& response_out)
{
    int rv = 0;
    vplex::vsDirectory::DeleteSubscriptionsInput req;

    Query_FillInVsdsSession(req.mutable_session(), session);
    req.set_userid(userId);
    req.set_deviceid(deviceId);
    for(size_t index = 0; index < datasetIds.size(); index++) {
        req.add_datasetids(datasetIds[index]);
    }

    LOG_INFO("Query_DeleteSubscriptions userId:"FMTu64" deviceId:"FMTu64, userId, deviceId);
    rv = QUERY_VSDS(VPLVsDirectory_DeleteSubscriptions, req, response_out);
    return rv;
}


s32 Query_LinkDevice(u64 userId,
                     const ccd::UserSession& session,
                     const std::string& deviceName,
                     const std::string& deviceClass,
                     const std::string& osVersion,
                     bool isAcerDevice,
                     bool hasCamera,
                     vplex::vsDirectory::LinkDeviceOutput& response_out)
{
    int rv = 0;
    u64 deviceGuid;
    VPLVsDirectory_ProxyHandle_t proxy;
    vplex::vsDirectory::LinkDeviceInput req;
    char* osversion = NULL;
    

    ESCore_GetDeviceGuid(&deviceGuid);

    Query_FillInVsdsSession(req.mutable_session(), session);
    req.set_userid(userId);
    req.set_deviceid(deviceGuid);
    req.set_deviceclass(deviceClass);
    req.set_isacer(isAcerDevice);
    req.set_hascamera(hasCamera);

    rv = VPL_GetOSVersion(&osversion);
    if(rv == VPL_OK && osversion != NULL){
        req.set_osversion(osversion);
    }
    VPL_ReleaseOSVersion(osversion);

    req.set_protocolversion(CCD_PROTOCOL_VERSION);
    req.set_buildinfo(SW_CCD_BUILD_INFO);

    char* manufacturer = NULL;
    char* model = NULL;
    string modelNumber;
    rv = VPL_GetDeviceInfo(&manufacturer, &model);
    if(rv == VPL_OK && manufacturer != NULL && model != NULL){
        modelNumber = manufacturer;
        modelNumber += ":";
        modelNumber += model;
        req.set_modelnumber(modelNumber);

        VPL_ReleaseDeviceInfo(manufacturer, model);
    }

    if(deviceName != "") {
        req.set_devicename(deviceName);
    }

    proxy = Query_GetVsdsProxy();

    LOG_INFO("Linking Device: %s, "FMTu64" with user:"FMTu64, deviceClass.c_str(), deviceGuid, userId);
    rv = VPLVsDirectory_LinkDevice(proxy, VPLTIME_FROM_SEC(30),
                                   req, response_out);
    Query_ReleaseVsdsProxy(proxy);

    return rv;
}

s32 Query_UnlinkDevice(u64 userId,
                       const ccd::UserSession& session,
                       u64 deviceId,
                       vplex::vsDirectory::UnlinkDeviceOutput& response_out)
{
    int rv = 0;
    VPLVsDirectory_ProxyHandle_t proxy;
    vplex::vsDirectory::UnlinkDeviceInput req;

    Query_FillInVsdsSession(req.mutable_session(), session);
    req.set_userid(userId);
    req.set_deviceid(deviceId);

    proxy = Query_GetVsdsProxy();

    LOG_INFO("Unlinking Device: "FMTu64" with user:"FMTu64, deviceId, userId);
    rv = VPLVsDirectory_UnlinkDevice(proxy, VPLTIME_FROM_SEC(30),
                                     req, response_out);
    Query_ReleaseVsdsProxy(proxy);

    return rv;
}

s32 Query_SetDeviceName(u64 userId,
                        const ccd::UserSession& session,
                        const std::string& deviceName,
                        vplex::vsDirectory::SetDeviceNameOutput& response_out)
{
    int rv = 0;
    u64 deviceId;
    VPLVsDirectory_ProxyHandle_t proxy;
    vplex::vsDirectory::SetDeviceNameInput req;

    Query_FillInVsdsSession(req.mutable_session(), session);
    req.set_userid(userId);

    rv = ESCore_GetDeviceGuid(&deviceId);
    if(rv != 0) {
        LOG_ERROR("ESCore_GetDeviceGuid:%d", rv);
        goto exit;
    }
    req.set_deviceid(deviceId);
    req.set_devicename(deviceName);

    proxy = Query_GetVsdsProxy();

    LOG_INFO("Setting device name: "FMTu64" with user:"FMTu64" DeviceName:%s",
             deviceId, userId, deviceName.c_str());
    rv = VPLVsDirectory_SetDeviceName(proxy, VPLTIME_FROM_SEC(30),
                                      req, response_out);
    Query_ReleaseVsdsProxy(proxy);
 exit:
    return rv;
}


s32 Query_UpdateProtocolVersion(u64 userId,
                                const ccd::UserSession& session,
                                const std::string& version)
{
    int rv;
    u64 deviceGuid = 0;
    vplex::vsDirectory::UpdateDeviceInfoInput req;
    vplex::vsDirectory::UpdateDeviceInfoOutput resp;

    Query_FillInVsdsSession(req.mutable_session(), session);
    
    ESCore_GetDeviceGuid(&deviceGuid);
    req.set_userid(userId);
    req.set_deviceid(deviceGuid);
    req.set_protocolversion(version);
    
    rv = QUERY_VSDS(VPLVsDirectory_UpdateDeviceInfo, req, resp);
    return rv;
}

s32 Query_UpdateBuildInfo(u64 userId,
                                const ccd::UserSession& session,
                                const std::string& buildInfo)
{
    int rv;
    u64 deviceGuid = 0;
    vplex::vsDirectory::UpdateDeviceInfoInput req;
    vplex::vsDirectory::UpdateDeviceInfoOutput resp;

    Query_FillInVsdsSession(req.mutable_session(), session);
    
    ESCore_GetDeviceGuid(&deviceGuid);
    req.set_userid(userId);
    req.set_deviceid(deviceGuid);
    req.set_buildinfo(buildInfo);
    
    rv = QUERY_VSDS(VPLVsDirectory_UpdateDeviceInfo, req, resp);
    return rv;
}

s32 Query_UpdateModelNumber(u64 userId,
                                const ccd::UserSession& session,
                                const std::string& modelNumber)
{
    int rv;
    u64 deviceGuid = 0;
    vplex::vsDirectory::UpdateDeviceInfoInput req;
    vplex::vsDirectory::UpdateDeviceInfoOutput resp;

    Query_FillInVsdsSession(req.mutable_session(), session);
    
    ESCore_GetDeviceGuid(&deviceGuid);
    req.set_userid(userId);
    req.set_deviceid(deviceGuid);
    req.set_modelnumber(modelNumber);
    
    rv = QUERY_VSDS(VPLVsDirectory_UpdateDeviceInfo, req, resp);
    return rv;
}


s32 Query_UpdateOsVersion(u64 userId,
                          const ccd::UserSession& session,
                          const std::string& version)
{
    int rv;
    u64 deviceGuid = 0;
    vplex::vsDirectory::UpdateDeviceInfoInput req;
    vplex::vsDirectory::UpdateDeviceInfoOutput resp;

    Query_FillInVsdsSession(req.mutable_session(), session);
    
    ESCore_GetDeviceGuid(&deviceGuid);
    req.set_userid(userId);
    req.set_deviceid(deviceGuid);
    req.set_osversion(version);
    
    rv = QUERY_VSDS(VPLVsDirectory_UpdateDeviceInfo, req, resp);
    return rv;
}

s32 Query_UpdateDeviceName(u64 userId,
                           const ccd::UserSession& session,
                           const std::string& name)
{
    int rv;
    u64 deviceGuid = 0;
    vplex::vsDirectory::UpdateDeviceInfoInput req;
    vplex::vsDirectory::UpdateDeviceInfoOutput resp;

    Query_FillInVsdsSession(req.mutable_session(), session);
    
    ESCore_GetDeviceGuid(&deviceGuid);
    req.set_userid(userId);
    req.set_deviceid(deviceGuid);
    req.set_devicename(name);
    
    rv = QUERY_VSDS(VPLVsDirectory_UpdateDeviceInfo, req, resp);
    return rv;
}

s32 Query_UpdateDeviceInfo(u64 userId,
                           const ccd::UserSession& session,
                           const std::string& deviceName,
                           const std::string& osVersion,
                           const std::string& protocolVersion,
                           const std::string& modelNumber,
                           const std::string& buildInfo)
{
    int rv;
    u64 deviceGuid = 0;
    vplex::vsDirectory::UpdateDeviceInfoInput req;
    vplex::vsDirectory::UpdateDeviceInfoOutput resp;

    Query_FillInVsdsSession(req.mutable_session(), session);
    
    ESCore_GetDeviceGuid(&deviceGuid);
    req.set_userid(userId);
    req.set_deviceid(deviceGuid);

    if(!deviceName.empty()){
        req.set_devicename(deviceName);
    }
    if(!osVersion.empty()){
        req.set_osversion(osVersion);
    }
    if(!protocolVersion.empty()){
        req.set_protocolversion(protocolVersion);
    }
    if(!modelNumber.empty()){
        req.set_modelnumber(modelNumber);
    }
    if(!buildInfo.empty()){
        req.set_buildinfo(buildInfo);
    }
    
    rv = QUERY_VSDS(VPLVsDirectory_UpdateDeviceInfo, req, resp);
    return rv;
}

s32 Query_GetCloudInfo(u64 userId,
                       const ccd::UserSession& session,
                       u64 deviceId,
                       const std::string& version,
                       vplex::vsDirectory::GetCloudInfoOutput& response_out)
{
    int rv;
    vplex::vsDirectory::GetCloudInfoInput req;
    Query_FillInVsdsSession(req.mutable_session(), session);
    req.set_userid(userId);
    req.set_deviceid(deviceId);
    req.set_version(version);
	rv = QUERY_VSDS(VPLVsDirectory_GetCloudInfo, req, response_out);
    return rv;
}
