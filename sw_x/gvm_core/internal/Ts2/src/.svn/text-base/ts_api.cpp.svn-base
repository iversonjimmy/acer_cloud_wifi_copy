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

///
/// ts_api.cpp
///
/// Tunnel Service Client APIs

#ifndef IS_TS
#define IS_TS
#endif

#include "ts_client.hpp"
#include "ts_internal.hpp"
#include "Pool.hpp"

using namespace std;

static u64 ts_user_id;
static u64 ts_device_id;
static u32 ts_instance_id;

enum TsState {
    TS_STATE_NULL = 0,
    TS_STATE_READY,
    TS_STATE_SHUTTING_DOWN,
};
static TsState ts_state = TS_STATE_NULL;

static Ts2::Transport::Pool* ts_pool = NULL;
static Ts2::LocalInfo *ts_localInfo = NULL;


namespace TS {

TSError_t TS_Open(const TSOpenParms_t& parms,
                  TSIOHandle_t& ret_io_handle,
                  string& error_msg)
{
    TSError_t err = TS_OK;
    u32 newVtId = 0;
    Ts2::Transport::Vtunnel* vt;
    ts_udi_t src_udi;
    ts_udi_t dst_udi;

    error_msg.clear();

    if (ts_state != TS_STATE_READY) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS not initialized";
        goto end;
    }

    src_udi.userId = ts_user_id;
    src_udi.deviceId = ts_device_id;
    src_udi.instanceId = ts_instance_id;
    dst_udi.userId = parms.user_id;
    dst_udi.deviceId = parms.device_id;
    dst_udi.instanceId = parms.instance_id;

    vt = new (std::nothrow) Ts2::Transport::Vtunnel(src_udi, dst_udi, parms.service_name, ts_localInfo);
    if (vt == NULL) {
        err = TS_ERR_NO_MEM;
        error_msg = "TS allocation failed";
        goto end;
    }
    err = ts_pool->VtunnelAdd(vt, newVtId);
    if (err != TS_OK) {
        error_msg = "Vtunnel creation failed";
        delete vt;
        goto end;
    }

    err = vt->Open(error_msg);

    LOG_INFO(TS_FMT_TARG": svc %s vtid: %d err %d: %s",
             parms.user_id, parms.device_id, parms.instance_id,
             parms.service_name.c_str(),
             newVtId, err, error_msg.c_str());

    // Release reference count
    ts_pool->VtunnelRelease(vt);

    if (err != TS_OK) {
        // VtunnelRemove deletes the vt object in addition to removing the vtId map entry.
        (void) ts_pool->VtunnelRemove(newVtId);
        goto end;
    }

    ret_io_handle = (TSIOHandle_t)newVtId;

end:
    return err;
}

TSError_t TS_Close(TSIOHandle_t& ioh,
                   string& error_msg)
{
    TSError_t err = TS_OK;
    u32 vtId;
    Ts2::Transport::Vtunnel* vt;

    error_msg.clear();

    if (ts_state != TS_STATE_READY) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS not initialized";
        goto end;
    }

    vtId = (u32)ioh;
    // Translate VT ID to pointer and take a reference
    err = ts_pool->VtunnelFind(vtId, vt);
    if (err != TS_OK) {
        error_msg = "Handle not found";
        goto end;
    }

    LOG_INFO("Closing vtId "FMTu32" vtunnel %p", vtId, vt);
    err = vt->Close(error_msg);
    
    // Release reference to Vtunnel
    ts_pool->VtunnelRelease(vt);

    // Even if the close failed, there's nothing useful to be done
    // with this tunnel.  Just delete it.
    (void) ts_pool->VtunnelRemove(vtId);

end:
    return err;
}

TSError_t TS_Read(TSIOHandle_t ioh,
                  char* buffer,
                  size_t& buffer_len,
                  string& error_msg)
{
    TSError_t err = TS_OK;
    u32 vtId;
    Ts2::Transport::Vtunnel* vt;

    error_msg.clear();

    if (ts_state != TS_STATE_READY) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS not initialized";
        goto end;
    }

    vtId = (u32)ioh;
    // Translate VT ID to pointer and take a reference
    err = ts_pool->VtunnelFind(vtId, vt);
    if (err != TS_OK) {
        error_msg = "Handle not found";
        goto end;
    }

    LOG_DEBUG("Reading vtId "FMTu32" vtunnel %p len "FMTu_size_t, vtId, vt, buffer_len);
    err = vt->Read(buffer, buffer_len, error_msg);
    
    // Release reference to Vtunnel
    ts_pool->VtunnelRelease(vt);

    LOG_DEBUG("Read vtId "FMTu32" vtunnel %p len "FMTu_size_t" returns %d", vtId, vt, buffer_len, err);
end:
    return err;
}

TSError_t TS_Write(TSIOHandle_t ioh,
                   const char* buffer,
                   size_t buffer_len,
                   string& error_msg)
{
    TSError_t err = TS_OK;
    u32 vtId;
    Ts2::Transport::Vtunnel* vt;

    error_msg.clear();

    if (ts_state != TS_STATE_READY) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS not initialized";
        goto end;
    }

    vtId = (u32)ioh;
    // Translate VT ID to pointer and take a reference
    err = ts_pool->VtunnelFind(vtId, vt);
    if (err != TS_OK) {
        error_msg = "Handle not found";
        goto end;
    }

    LOG_DEBUG("Writing vtId "FMTu32" vtunnel %p len "FMTu_size_t, vtId, vt, buffer_len);
    err = vt->Write(buffer, buffer_len, error_msg);

    // Release reference to Vtunnel
    ts_pool->VtunnelRelease(vt);

    LOG_INFO("Wrote vtId "FMTu32" vtunnel %p len "FMTu_size_t" returns %d", vtId, vt, buffer_len, err);

end:
    return err;
}

TSError_t TS_Init(u64 userId,
                  u64 deviceId,
                  u32 instanceId,
                  s32 (*getRouteInfo)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo),
                  VPLTime_t din_idle_timeout,
                  VPLTime_t p2p_idle_timeout,
                  VPLTime_t prx_idle_timeout,
                  int disabled_route_types,
                  Ts2::LocalInfo* local_info,
                  string& error_msg)
{
    TSError_t err = TS_OK;

    if (ts_state != TS_STATE_NULL) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS already initialized";
        goto end;
    }

    ts_user_id = userId;
    ts_device_id = deviceId;
    ts_instance_id = instanceId;
    ts_localInfo = local_info;

    ts_pool = new (std::nothrow) Ts2::Transport::Pool(/*isServer*/false, local_info);
    if (ts_pool == NULL) {
        err = TS_ERR_NO_MEM;
        error_msg = "TS allocation failed";
        goto end;
    }
    err = ts_pool->Start();
    if (err != TS_OK) {
        error_msg = "TS Pool Start failed";
        goto end;
    }

    ts_state = TS_STATE_READY;

end:
    return err;
}

TSError_t TS_GetPort(int& port_out)
{
    TSError_t err = TS_GetServerLocalPort(port_out);

    return err;
}

void TS_Shutdown(void)
{
    int err = TS_OK;

    if (ts_state != TS_STATE_READY) {
        goto end;
    }

    LOG_INFO("Shutting down Client TS");

    // Prevent any new TS calls
    ts_state = TS_STATE_SHUTTING_DOWN;

    // Stop all active tunnels
    err = ts_pool->Stop();
    LOG_INFO("TS Pool Stop returns %d", err);

    //
    // Stop() will return error if client threads are still active.
    // It is too dangerous to delete the pool in that case.
    //
    if (err == TS_OK) {
        delete ts_pool;
        ts_pool = NULL;
    }
    ts_state = TS_STATE_NULL;

    LOG_INFO("Shutting down Client TS complete");

end:
    return;
}

void TS_RefreshServerInfo(u64 userId, bool forceDropConns)
{
    // This is a NOP for the real TS (only needed for VSSI)
}

void TS_UpdateServerInfo(u64 userId, u64 deviceId, bool forceDropConns)
{
    // This is a NOP for the real TS (only needed for VSSI)
}

void TS_RemoveServerInfo(u64 deviceId)
{
    // This is a NOP for the real TS (only needed for VSSI)
}

}  // namespace TS guard
