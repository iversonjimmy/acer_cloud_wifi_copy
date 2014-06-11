/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF 
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

///
/// ts_shim.cpp
///
/// Tunnel Service Shim Interface

#include "vpl_th.h"
#include "vplex_trace.h"
#include "ts_client.hpp"

#include "LocalInfo_Ccd.hpp"

#undef __TS_CLIENT_HPP__
#define IS_TS
#include "ts_client.hpp" 
#undef IS_TS

#undef __TS_CLIENT_HPP__
#define IS_TS_WRAPPER
#include "ts_client.hpp"
#undef IS_TS_WRAPPER

#include <ccdi.hpp>

#include <map>

// TEMPORARY during 2.6-2.7
#define CONFIG_ENABLE_TS_USE_HTTPSVC_CCD 1
#define CONFIG_ENABLE_TS_ALWAYS_USE_VSSI 2
#define CONFIG_ENABLE_TS_INIT_TS_IN_SN   4
#define CONFIG_ENABLE_TS_ALWAYS_USE_TS   8

using namespace std;

typedef struct TS_Object_s {
    u64 user_id;
    u64 device_id;
    int ccd_protocol_version;
    bool use_ts;
} TS_Object_t;

static map<TSIOHandle_t,TS_Object_t*> ts_object_map;
static VPLMutex_t map_mutex;

typedef struct shim_table_s {
    TSError_t (*init)(u64 userId,
                      u64 deviceId,
                      u32 instanceId,
                      s32 (*getRouteInfoCb)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo),
                      VPLTime_t din_idle_timeout,
                      VPLTime_t p2p_idle_timeout,
                      VPLTime_t prx_idle_timeout,
                      int disabled_route_types,
                      Ts2::LocalInfo* localInfo,
                      string& error_msg);
    TSError_t (*getport)(int& port_out);
    void (*shutdown) (void);
    TSError_t (*open)(const TSOpenParms_t& parms,
                      TSIOHandle_t& ret_io_handle,
                      string& error_msg);
    TSError_t (*close)(TSIOHandle_t& io_handle,
                       string& error_msg);
    TSError_t (*read)(TSIOHandle_t io_handle,
                      char* buffer,
                      size_t& buffer_len,
                      string& error_msg);
    TSError_t (*write)(TSIOHandle_t io_handle,
                       const char* buffer,
                       size_t buffer_len,
                       string& error_msg);
    TSError_t (*getserverlocalport)(int& port_out);
} shim_table_t;

shim_table_t st_wrapper = {
    TS_WRAPPER::TS_Init,
    TS_WRAPPER::TS_GetPort,
    TS_WRAPPER::TS_Shutdown,
    TS_WRAPPER::TS_Open,
    TS_WRAPPER::TS_Close,
    TS_WRAPPER::TS_Read,
    TS_WRAPPER::TS_Write,
    TS_WRAPPER::TS_GetServerLocalPort,
};

shim_table_t st_ts = {
    TS::TS_Init,
    TS::TS_GetPort,
    TS::TS_Shutdown,
    TS::TS_Open,
    TS::TS_Close,
    TS::TS_Read,
    TS::TS_Write,
    TS::TS_GetServerLocalPort,
};

static int enableTs = 0;
void ts_shim_set_enableTs(int _enableTs);
void ts_shim_set_enableTs(int _enableTs)
{
    enableTs = _enableTs;
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "enableTs=%d", enableTs);
}

static Ts2::LocalInfo *g_localInfo = NULL;

TSError_t TS_Init(u64 userId,
                  u64 deviceId,
                  u32 instanceId,
                  s32 (*getRouteInfoCb)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo),
                  VPLTime_t din_idle_timeout,
                  VPLTime_t p2p_idle_timeout,
                  VPLTime_t prx_idle_timeout,
                  int disabled_route_types,
                  Ts2::LocalInfo *localInfo,
                  string& error_msg)
{
    TSError_t err = TS_OK;
    int rv = VPL_OK;

    VPL_Init();
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "Init...");

    VPL_SET_UNINITIALIZED(&map_mutex);
    rv = VPLMutex_Init(&map_mutex);
    if ( rv != VPL_OK ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:VPLMutex_init() %d", rv);
        err = TS_ERR_INVALID;
        error_msg = "failed to initialize mutex";
        goto done;
    }

    g_localInfo = localInfo;

    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI)) {
        err = TS::TS_Init(userId,
                          deviceId,
                          instanceId,
                          getRouteInfoCb,
                          din_idle_timeout,
                          p2p_idle_timeout,
                          prx_idle_timeout,
                          disabled_route_types,
                          localInfo,
                          error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to init tunnel service: %d",
                             err);
            goto done;
        }
    }

    err = TS_WRAPPER::TS_Init(userId,
                              deviceId,
                              instanceId,
                              getRouteInfoCb,
                              din_idle_timeout,
                              p2p_idle_timeout,
                              prx_idle_timeout,
                              disabled_route_types,
                              localInfo,
                              error_msg);
    if (err != TS_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to init tunnel service wrapper layer: %d",
            err);
        goto done;
    }

done:
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "Init end: %d", err);
    return err;
}

void TS_Shutdown(void)
{
    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI)) {
        TS::TS_Shutdown();
    }
    TS_WRAPPER::TS_Shutdown();

    VPLMutex_Lock(&map_mutex);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "cleanup ts_object_map...");
    ts_object_map.clear();
    VPLMutex_Unlock(&map_mutex);

    g_localInfo = NULL;

    if ( VPL_IS_INITIALIZED(&map_mutex) ) {
        VPLMutex_Destroy(&map_mutex);
    }
}

TSError_t TS_Open(const TSOpenParms_t& parms,
                  TSIOHandle_t& ret_io_handle,
                  string& error_msg)
{
    TSError_t err = TS_OK;

    if (!g_localInfo) {
        err = TS_ERR_NOT_INIT;
        error_msg = "TsShim is not initialized";
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TsShim is not initialized");
        return err;
    }

    int ccd_protocol_version = g_localInfo->GetDeviceCcdProtocolVersion(parms.device_id);
    bool use_ts = (enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_TS) || (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI) && (ccd_protocol_version >= 4));

    shim_table_t* st = use_ts ? &st_ts : &st_wrapper;
    err = (st->open)(parms, ret_io_handle, error_msg);
    if (err == TS_OK) {
        VPLMutex_Lock(&map_mutex);
        TS_Object_t* obj = new TS_Object_t;
        obj->user_id = parms.user_id;
        obj->device_id = parms.device_id;
        obj->ccd_protocol_version = ccd_protocol_version;
        obj->use_ts = use_ts;
        ts_object_map[ret_io_handle] = obj;
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "add obj to ts_object_map[%p]", ret_io_handle);
        VPLMutex_Unlock(&map_mutex);
    }
    return err;
}

TSError_t TS_Close(TSIOHandle_t& io_handle,
                   string& error_msg)
{
    TS_Object_t* obj = NULL;
    map<TSIOHandle_t, TS_Object_t*>::iterator it;
    shim_table_t* st = NULL;
    TSError_t err = TS_OK;

    VPLMutex_Lock(&map_mutex);
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "looking for obj %p in ts_object_map", io_handle);
    it = ts_object_map.find(io_handle);
    if (it != ts_object_map.end()) {
        obj = it->second;
        ts_object_map.erase(it);
    }
    VPLMutex_Unlock(&map_mutex);

    if (obj == NULL) {
        error_msg = "cannot find io handle";
        return TS_ERR_INVALID;
    }

    st = obj->use_ts ? &st_ts : &st_wrapper;
    err = (st->close)(io_handle, error_msg);

    delete obj;

    return err;
}

TSError_t TS_Read(TSIOHandle_t io_handle,
                  char* buffer,
                  size_t& buffer_len,
                  string& error_msg)
{
    TS_Object_t* obj = NULL;
    map<TSIOHandle_t, TS_Object_t*>::iterator it;
    shim_table_t* st = NULL;

    VPLMutex_Lock(&map_mutex);
    it = ts_object_map.find(io_handle);
    if (it != ts_object_map.end()) {
        obj = it->second;
    }
    VPLMutex_Unlock(&map_mutex);

    if (obj == NULL) {
        error_msg = "cannot find io handle";
        return TS_ERR_INVALID;
    }

    st = obj->use_ts ? &st_ts : &st_wrapper;
    return (st->read)(io_handle, buffer, buffer_len, error_msg);
}

TSError_t TS_Write(TSIOHandle_t io_handle,
                   const char* buffer,
                   size_t buffer_len,
                   string& error_msg)
{
    TS_Object_t* obj = NULL;
    map<TSIOHandle_t, TS_Object_t*>::iterator it;
    shim_table_t* st = NULL;

    VPLMutex_Lock(&map_mutex);
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "looking for obj %p in ts_object_map", io_handle);
    it = ts_object_map.find(io_handle);
    if (it != ts_object_map.end()) {
        obj = it->second;
    }
    VPLMutex_Unlock(&map_mutex);

    if (obj == NULL) {
        error_msg = "cannot find io handle";
        return TS_ERR_INVALID;
    }
    st = obj->use_ts ? &st_ts : &st_wrapper;
    return (st->write)(io_handle, buffer, buffer_len, error_msg);
}

TSError_t TS_GetPort(int& port_out)
{
    TSError_t tserr = TS_OK;
    if (enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI) {
        port_out = 0;
    }
    else {
        tserr = TS::TS_GetPort(port_out);
    }
    return tserr;
}

TSError_t TS_GetServerLocalPort(int& port_out)
{
    TSError_t tserr = TS_OK;
    if (enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI) {
        return tserr;
    }
    else {
        tserr = TS::TS_GetServerLocalPort(port_out);
    }
    return tserr;
}

void TS_RefreshServerInfo(u64 userId, bool forceDropConns)
{
    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI)) {
        TS::TS_RefreshServerInfo(userId, forceDropConns);
    }
    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_TS)) {
        TS_WRAPPER::TS_RefreshServerInfo(userId, forceDropConns);
    }
}

void TS_UpdateServerInfo(u64 userId, u64 deviceId, bool forceDropConns)
{
    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI)) {
        TS::TS_UpdateServerInfo(userId, deviceId, forceDropConns);
    }
    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_TS)) {
        TS_WRAPPER::TS_UpdateServerInfo(userId, deviceId, forceDropConns);
    }
}

void TS_RemoveServerInfo(u64 deviceId)
{
    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_VSSI)) {
        TS::TS_RemoveServerInfo(deviceId);
    }
    if (!(enableTs & CONFIG_ENABLE_TS_ALWAYS_USE_TS)) {
        TS_WRAPPER::TS_RemoveServerInfo(deviceId);
    }
}

