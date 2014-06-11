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
/// ts_server.cpp
///
/// Tunnel Service Server API

#ifndef IS_TS
#define IS_TS
#endif

#include "ts_server.hpp"
#include "ts_internal.hpp"
#include "Packet.hpp"
#include "Pool.hpp"

using namespace std;

static Ts2::Transport::Pool* tss_pool = NULL;
static Ts2::LocalInfo *tss_localInfo = NULL;
static VPLMutex_t tss_mutex;
static VPLCond_t tss_cond_input;
static VPLCond_t tss_cond_threads;
static bool tss_is_init = false;
static bool tss_stopping = false;
static u64 tss_user_id;
static u64 tss_device_id;
static u32 tss_instance_id;

static map<string,TSServiceHandle_t> tss_services;
static int tss_service_threads = 0;

static const size_t threadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

#define TSS_SHUTDOWN_TIMEOUT    4

static VPLDetachableThreadHandle_t tssThread;
static VPLTHREAD_FN_DECL tssThreadMain(void *param);

static queue<Ts2::Packet *> tssInputQueue;

static void ts_queueConnRequest(Ts2::Packet *pkt);
static TSError_t ts_startService(TSServiceHandle_t svcHandle, u32 vtId, ts_udi_t *dst_udi);

namespace TS {

TSError_t TS_RegisterService(TSServiceParms_t& parms,
                             TSServiceHandle_t& ret_service_handle,
                             string& error_msg)
{
    TSError_t err = TS_OK;
    TSServiceHandle_t sh;
    map<string,TSServiceHandle_t>::iterator mit;
    list<string>::iterator lit;

    error_msg.clear();

    if (!tss_is_init) {
        error_msg = "TS layer not initialized";
        return TS_ERR_NOT_INIT;
    }

    if (parms.service_names.size() == 0) {
        error_msg = "No services specified.";
        return TS_ERR_INVALID;
    }

    VPLMutex_Lock(&tss_mutex);
    for (lit = parms.service_names.begin(); lit != parms.service_names.end(); lit++) {
        mit = tss_services.find(*lit);
        if (mit != tss_services.end()) {
            error_msg = "Service handler already exists";
            err = TS_ERR_EXISTS;
            LOG_ERROR("Service name %s already has svc handle %p", lit->c_str(), mit->second); 
            goto end;
        }
    }

    sh = new struct TSServiceHandle_s;
    sh->protocol_name = parms.protocol_name;
    sh->service_names = parms.service_names;
    sh->service_handler = parms.service_handler;

    for (lit = parms.service_names.begin(); lit != parms.service_names.end(); lit++) {
        tss_services[*lit] = sh;
        LOG_INFO("Service name %s -> svc handle %p", lit->c_str(), sh); 
    }
    ret_service_handle = sh;
end:
    VPLMutex_Unlock(&tss_mutex);

    return err;
}

//
// Remove all service name entries that map to a particular service handle.
// Note that this prevents new connections from being established, but does
// not stop any existing connections.
//
TSError_t TS_DeregisterService(TSServiceHandle_t& service_handle,
                               string& error_msg)
{
    TSError_t err = TS_OK;
    map<string,TSServiceHandle_t>::iterator mit;
    list<string>::iterator lit;

    if (!tss_is_init) {
        err = TS_ERR_NOT_INIT;
        error_msg = "TS layer not initialized";
        goto end;
    }

    error_msg.clear();

    VPLMutex_Lock(&tss_mutex);
    for (lit  = service_handle->service_names.begin();
         lit != service_handle->service_names.end();
         lit++) {

        mit = tss_services.find(*lit);
        if (mit == tss_services.end()) {
            continue;
        }
        // XXX Can it really happen that the service_handle doesn't match?
        if (service_handle != mit->second) {
            LOG_ALWAYS("Service name %s with mismatched svc handle %p:%p",
                lit->c_str(), service_handle, mit->second); 
            continue;
        }
        // Any active services will be stopped by TS_ServerShutdown
        LOG_INFO("Service name %s deleted for svc handle %p",
            lit->c_str(), service_handle); 

        tss_services.erase(mit);
    }

    delete service_handle;
    service_handle = NULL;

    VPLMutex_Unlock(&tss_mutex);

end:
    return err;
}

TSError_t TS_ServerInit(u64 userId,
                        u64 deviceId,
                        u32 instanceId,
                        Ts2::LocalInfo* local_info,
                        string& error_msg)
{
    TSError_t err = TS_OK;

    LOG_INFO("user id: "FMTu64", device id: "FMTu64", instance name: "FMTu32,
              userId, deviceId, instanceId);

    //
    // This is racey and kind of lame, but this should not be called by more than
    // one thread in the real world.
    //
    if (tss_is_init) {
        LOG_ERROR("already init");
        goto end;
    }

    if (VPLMutex_Init(&tss_mutex) != VPL_OK) {
        error_msg = "Failed to initialize tss_mutex";
        err = TS_ERR_NO_MEM;
        goto end;
    }

    if (VPLCond_Init(&tss_cond_input) != VPL_OK) {
        error_msg = "Failed to initialize tss condvar";
        err = TS_ERR_NO_MEM;
        goto end;
    }

    if (VPLCond_Init(&tss_cond_threads) != VPL_OK) {
        error_msg = "Failed to initialize tss condvar";
        err = TS_ERR_NO_MEM;
        goto end;
    }

    tss_user_id = userId;
    tss_device_id = deviceId;
    tss_instance_id = instanceId;
    tss_localInfo = local_info;

    tss_pool = new (std::nothrow) Ts2::Transport::Pool(/*isServer*/true, local_info);
    if (tss_pool == NULL) {
        err = TS_ERR_NO_MEM;
        error_msg = "TS allocation failed";
        goto end;
    }

    tss_pool->SetConnRequestHandler(ts_queueConnRequest);

    err = tss_pool->Start();
    if (err != TS_OK) {
        error_msg = "TS Pool Start failed";
        goto end;
    }

    tss_stopping = false;

    //
    // Start one main thread to field incoming connection requests
    //
    err = Util_SpawnThread(tssThreadMain, NULL, threadStackSize, /*isJoinable*/VPL_TRUE, &tssThread);
    if (err) {
        error_msg = "Failed to create server thread";
        err = TS_ERR_NO_MEM;
        goto end;
    }

    tss_is_init = true;
    tss_service_threads = 0;

end:
    if (err) {
        LOG_ERROR("%d: %s", err, error_msg.c_str()); 
    }
    return err;
}

void TS_ServerShutdown(void)
{
    if (!tss_is_init) {
        LOG_ERROR("Attempted to shutdown Server TS before init");
        goto end;
    }

    LOG_INFO("Shutting down Server TS");

    // Signal the main service thread to stop everything
    VPLMutex_Lock(&tss_mutex);
    tss_stopping = true;
    VPLCond_Signal(&tss_cond_input);
    VPLMutex_Unlock(&tss_mutex);

    // Then wait for it to complete the dirty work
    VPLDetachableThread_Join(&tssThread);

    LOG_INFO("Shutting down Server TS complete");

end:
    return;
}

TSError_t TSS_Read(TSIOHandle_t io_handle,
                  char* buffer,
                  size_t& buffer_len,
                  string& error_msg)
{
    TSError_t err = TS_OK;
    u32 vtId;
    Ts2::Transport::Vtunnel* vt;

    error_msg.clear();

    if (!tss_is_init) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS not initialized";
        goto end;
    }

    vtId = (u32)io_handle;
    // Translate VT ID to pointer and take a reference
    err = tss_pool->VtunnelFind(vtId, vt);
    if (err != TS_OK) {
        error_msg = "Handle not found";
        goto end;
    }

    LOG_DEBUG("Reading vtId "FMTu32" vtunnel %p len "FMTu_size_t, vtId, vt, buffer_len);
    err = vt->Read(buffer, buffer_len, error_msg);
    // Release reference to Vtunnel
    tss_pool->VtunnelRelease(vt);

    LOG_DEBUG("Read vtId "FMTu32" vtunnel %p len "FMTu_size_t, vtId, vt, buffer_len);

end:
    return err;
}

TSError_t TSS_Write(TSIOHandle_t io_handle,
                   const char* buffer,
                   size_t buffer_len,
                   string& error_msg)
{
    TSError_t err = TS_OK;
    u32 vtId;
    Ts2::Transport::Vtunnel* vt;

    error_msg.clear();

    if (!tss_is_init) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS not initialized";
        goto end;
    }

    vtId = (u32)io_handle;
    // Translate VT ID to pointer and take a reference
    err = tss_pool->VtunnelFind(vtId, vt);
    if (err != TS_OK) {
        error_msg = "Handle not found";
        goto end;
    }

    LOG_DEBUG("Writing vtId "FMTu32" vtunnel %p len "FMTu_size_t, vtId, vt, buffer_len);
    err = vt->Write(buffer, buffer_len, error_msg);

    // Release reference to Vtunnel
    tss_pool->VtunnelRelease(vt);

    LOG_INFO("Wrote vtId "FMTu32" vtunnel %p len "FMTu_size_t, vtId, vt, buffer_len);

end:
    return err;
}

TSError_t TSS_Close(TSIOHandle_t& io_handle,
                    string& error_msg)
{
    TSError_t err = TS_OK;
    u32 vtId;
    Ts2::Transport::Vtunnel* vt;

    error_msg.clear();

    if (!tss_is_init) {
        err = TS_ERR_WRONG_STATE;
        error_msg = "TS not initialized";
        goto end;
    }

    vtId = (u32)io_handle;
    // Translate VT ID to pointer and take a reference
    err = tss_pool->VtunnelFind(vtId, vt);
    if (err != TS_OK) {
        error_msg = "Handle not found";
        goto end;
    }

    LOG_INFO("Closing vtId "FMTu32" vtunnel %p", vtId, vt);
    err = vt->Close(error_msg);

    // Release reference to Vtunnel
    tss_pool->VtunnelRelease(vt);

    // Even if the close failed, there's nothing useful to be done
    // with this tunnel.  Just delete it.
    (void) tss_pool->VtunnelRemove(vtId);

end:
    return err;
}

TSError_t TS_GetServerLocalPort(int& port_out)
{
    TSError_t err = TS_ERR_INVALID;
    if (tss_pool != NULL) {
        port_out = tss_pool->GetListenPort();
        err = TS_OK;
    }
    return err;
}

}  // namespace TS guard


TSError_t TS_ServiceLookup(const string& service_name,
                           TSServiceHandle_t& service_handle,
                           string& error_msg)
{
    TSError_t err = TS_OK;
    map<string,TSServiceHandle_t>::iterator mit;

    error_msg.clear();

    if (!tss_is_init) {
        error_msg = "TS layer not initialized";
        return TS_ERR_NOT_INIT;
    }

    VPLMutex_Lock(&tss_mutex);
    mit = tss_services.find(service_name);
    if (mit == tss_services.end()) {
        error_msg = "Service not found";
        err = TS_ERR_NO_SERVICE;
        service_handle = NULL;
        goto end;
    }

    service_handle = mit->second;

end:
    VPLMutex_Unlock(&tss_mutex);

    return err;
}

static void
ts_queueConnRequest(Ts2::Packet *pkt)
{
    MutexAutoLock lock(&tss_mutex);
    tssInputQueue.push(pkt);
    VPLCond_Signal(&tss_cond_input);
}

struct ts_service_parms_s {
    TSServiceHandle_t svcHandle;
    u32 vtId;
    ts_udi_t *dst_udi;
};

static VPLTHREAD_FN_DECL ts_serviceRunner(void* parm)
{
    struct ts_service_parms_s *parmsp = (struct ts_service_parms_s *) parm;
    TSServiceRequest_t sr;
    Ts2::Transport::Vtunnel* vt;
    TSError_t err = TS_OK;
    string errMsg;

    do {

        err = tss_pool->VtunnelFind(parmsp->vtId, vt);
        if (err != TS_OK) {
            LOG_ERROR("Bad tunnel ID %u", parmsp->vtId);
            break;
        }

        // First wait for the ACK to complete the open.
        // Note that this can fail if the operation times out or
        // it is aborted by a shutdown.
        err = vt->WaitForOpen();

        // Release reference to Vtunnel
        tss_pool->VtunnelRelease(vt);
        vt = NULL;

        sr.service_handle = parmsp->svcHandle;
        sr.client_user_id = parmsp->dst_udi->userId;
        sr.client_device_id = parmsp->dst_udi->deviceId;
        sr.client_credentials.erase();
        sr.io_handle = (TSIOHandle_t)parmsp->vtId;
        delete parmsp;

        if (err != TS_OK) {
            LOG_ERROR("Open failed to complete for tunnel ID %u, err %d", (u32)sr.io_handle, err);
            (void) tss_pool->VtunnelRemove((u32)sr.io_handle);
            break;
        }

        LOG_INFO("Service thread for tunnel ID %u starting", (u32)sr.io_handle);

        // Invoke the actual service handler function
        sr.service_handle->service_handler(sr);

        LOG_DEBUG("Service thread wait for close on tunnel ID %u, err %d msg %s", (u32)sr.io_handle, err, errMsg.c_str());

        //
        // Wait for data to flush and the close to come through from the client.
        // Note that shutdown could have closed the vtunnel out from under this thread,
        // so it is not safe to dereference the previous "vt" pointer.
        //
        err = tss_pool->VtunnelFind((u32)sr.io_handle, vt);
        if (err != TS_OK) {
            LOG_INFO("Tunnel ID %u already closed", (u32)sr.io_handle);
            break;
        }
        err = vt->WaitForClose();
        if (err != TS_OK) {
            LOG_INFO("Close failed for Tunnel ID %u, err %d", (u32)sr.io_handle, err);
            // Do not break here, need to clean up ...
        }

        // Release reference to Vtunnel
        tss_pool->VtunnelRelease(vt);
        vt = NULL;

        LOG_DEBUG("Service thread closed tunnel ID %u, err %d msg %s", (u32)sr.io_handle, err, errMsg.c_str());

        err = tss_pool->VtunnelRemove((u32)sr.io_handle);
        if (err != 0) {
            LOG_ERROR("Service thread remove tunnel ID %u, err %d", (u32)sr.io_handle, err);
        }

    } while (ALWAYS_FALSE_CONDITIONAL);

    // Thread exits
    {
        MutexAutoLock lock(&tss_mutex);
        tss_service_threads--;
        VPLCond_Broadcast(&tss_cond_threads);
        LOG_INFO("Service thread for tunnel ID %u exiting (%d threads still active)", (u32)sr.io_handle, tss_service_threads);
    }
    return 0;
}

static TSError_t
ts_startService(TSServiceHandle_t svcHandle, u32 vtId, ts_udi_t *dst_udi)
{
    TSError_t err = TS_OK;
    VPLDetachableThreadHandle_t svcThread;
    struct ts_service_parms_s *parms;

    parms = new struct ts_service_parms_s;

    parms->svcHandle = svcHandle;
    parms->vtId = vtId;
    parms->dst_udi = dst_udi;

    //
    // Start a service thread to handle a new Vtunnel
    //
    err = Util_SpawnThread(ts_serviceRunner, (void *)parms, threadStackSize, /*isJoinable*/VPL_FALSE, &svcThread);
    if (err) {
        err = TS_ERR_NO_MEM;
    }

    // The service threads run detached, so they cannot be reaped.  The code does track the number
    // of them outstanding, but does not "join" them.

    return err;
}

static u64 getUserIdFromDeviceId(u64 deviceId)
{
    // For now, it is always the same as the local userId.
    return tss_user_id;
}

static VPLTHREAD_FN_DECL tssThreadMain(void *param)
{
    int err = TS_OK;
    int stopErr = TS_OK;
    Ts2::Packet *pkt;
    u32 newVtId = 0;
    Ts2::Transport::Vtunnel* vt;
    ts_udi_t src_udi;
    ts_udi_t dst_udi;
    string errMsg;
    string svcName;
    TSServiceHandle_t svcHandle;

    LOG_INFO("TSS service thread start");

    while (1) {
        {
            MutexAutoLock lock(&tss_mutex);

            while (!tss_stopping && tssInputQueue.empty()) {
                err = VPLCond_TimedWait(&tss_cond_input, &tss_mutex, VPL_TIMEOUT_NONE);
                if (err) {
                    LOG_ERROR("CondVar failed: err %d", err);
                    goto end;
                }
            }
            if (tss_stopping) {
                break;
            }

            pkt = tssInputQueue.front();
            tssInputQueue.pop();
        } // AutoLock scope

        {
            if (pkt->GetPktType() != Ts2::TS_PKT_SYN) {
                LOG_ERROR("Drop Packet %p: not a SYN (type %u)", pkt, pkt->GetPktType());
                delete pkt;
                continue;
            }

            ((Ts2::PacketSyn*)pkt)->GetSvcName(svcName);
            LOG_DEBUG("Packet SYN with svc name: %s", svcName.c_str());
            
            // Check that the requested service is available
            err = TS_ServiceLookup(svcName, svcHandle, errMsg);
            if (err != TS_OK) {
                LOG_ERROR("Drop Packet %p: svc name %s not found", pkt, svcName.c_str());
                delete pkt;
                continue;
            }
            // Check that the dstUdi in the packet == our identity?
            if (pkt->GetDstDeviceId() != tss_device_id ||
                pkt->GetDstInstId() != tss_instance_id) {
                LOG_ERROR("Drop Packet %p: incorrect destination device", pkt);
                delete pkt;
                continue;
            }
            src_udi.userId = tss_user_id;
            src_udi.deviceId = tss_device_id;
            src_udi.instanceId = tss_instance_id;
            // The orientation of the tunnel is opposite to the packet in terms of src and dest
            u32 dst_vt_id = pkt->GetSrcVtId();
            dst_udi.deviceId = pkt->GetSrcDeviceId();
            dst_udi.userId = getUserIdFromDeviceId(dst_udi.deviceId);
            dst_udi.instanceId = pkt->GetSrcInstId();

#ifndef TS2_PKT_RETRY_ENABLE
            {
#else
            // check sequence number of SYN packet to verify it is redundent or not
            // consider the situation packet might be dropped during network change (even TCP packet)
            // ts_server needs to reply SYN ACK no matter it is redundent or not
            // 
            Ts2::Transport::dstInfo_t dst = {dst_udi.deviceId, dst_udi.instanceId, dst_vt_id, pkt->GetSeqNum()};
            err = tss_pool->VtunnelFind(dst, vt);
            if (err != TS_OK) {
#endif
                // Create new vtunnel on the server end
                vt = new (std::nothrow) Ts2::Transport::Vtunnel(src_udi, dst_udi, pkt->GetSeqNum(), dst_vt_id, svcName, tss_localInfo);
                if (vt == NULL) {
                    LOG_ERROR("Drop Packet %p: cannot alloc vtunnel", pkt);
                    delete pkt;
                    // Probably can't continue in this case
                    goto end;
                }
                err = tss_pool->VtunnelAdd(vt, newVtId);
                if (err == TS_ERR_INVALID_DEVICE) {
                    // Client device is unauthorized
                    LOG_ERROR("Drop Packet %p: unauthorized device", pkt);
                    delete pkt;
                    delete vt;
                    vt = NULL;
                    continue;
                }
                else if (err != TS_OK) {
                    // This failure should only happen if shutdown in progress
                    LOG_ERROR("Drop Packet %p: cannot assign vtunnel ID", pkt);
                    delete pkt;
                    delete vt;
                    goto end;
                }

                LOG_INFO("New tunnel open to "TS_FMT_TARG": svc %s local vtid: %u, remote vtid: %u",
                    dst_udi.userId, dst_udi.deviceId, dst_udi.instanceId, svcName.c_str(), newVtId, dst_vt_id);
            }

            // handle the SYN packet
            vt->HandleSyn(pkt, TS_OK);

            delete pkt;

            // Release reference count from VtunnelAdd/VtunnelFind
            (void) tss_pool->VtunnelRelease(vt);
            vt = NULL;

            // Start a service thread to handle the new Vtunnel
            LOG_DEBUG("Start service handler thread");
            err = ts_startService(svcHandle, newVtId, &dst_udi);
            if (err != TS_OK) {
                LOG_ERROR("Unable to start requested service %s", svcName.c_str());
                (void) tss_pool->VtunnelRemove(newVtId);
                // This is some sort of resource problem, so bail
                break;
            }
            {
                MutexAutoLock lock(&tss_mutex);
                tss_service_threads++;
            }
        }
    }

    // Server Stop requested
 
    VPLMutex_Lock(&tss_mutex);

    {
        // XXX: numVtunnels should be the same as tss_service_threads, right?
        size_t numVtunnels = tss_pool->GetPoolSize();
        LOG_INFO("tssThreadMain: shutdown started with %d active threads and "FMTu_size_t" vtunnels",
            tss_service_threads, numVtunnels);
    }

    //
    // TS_DeregisterService should have been done before calling TS_ServerShutdown,
    // but delete any remaining service entries.
    //
    if (tss_services.size() != 0) {
        LOG_WARN("Service map is not empty ("FMTu_size_t" entries).", tss_services.size());
        map<string,TSServiceHandle_t>::iterator mit;
        string errmsg;

        mit = tss_services.begin();
        while (mit != tss_services.end()) {
            LOG_WARN("Delete svc %s", mit->first.c_str());
            map<string,TSServiceHandle_t>::iterator mit_cur = mit++;
            tss_services.erase(mit_cur);
            // XXX: can't delete the service handle, since there is a many-to-one
            //      mapping of service names to handles. Or we have to keep track
            //      of the unique occurrences.
        }
    }

    VPLMutex_Unlock(&tss_mutex);

    // Stop any running services
    stopErr = tss_pool->Stop();
    LOG_INFO("TSS Pool Stop returns %d", stopErr);

    // Wait for all the service threads to exit
    while (1) {
        MutexAutoLock lock(&tss_mutex);
        if (tss_service_threads <= 0) {
            break;
        }
        LOG_INFO("TSS shutdown: wait for %d threads to exit", tss_service_threads);
        err = VPLCond_TimedWait(&tss_cond_threads, &tss_mutex, VPLTime_FromSec(TSS_SHUTDOWN_TIMEOUT));
        if (err == VPL_ERR_TIMEOUT) {
            // Don't hang up the shutdown
            LOG_ERROR("CondWait for exit of service threads times out after %d secs", TSS_SHUTDOWN_TIMEOUT);
            LOG_ERROR("Remaining service threads: %d", tss_service_threads);
            break;
        } else if (err != 0) {
            LOG_ERROR("CondWait fails %d", err);
            // Not much we can do, but better to bail than loop
            break;
        }
    }

    //
    // If the Pool->Stop returned an error or the thread count is non-zero, then
    // it is too risky to delete the tss_pool.
    //
    if (stopErr == TS_OK && err == TS_OK) {
        VPLCond_Destroy(&tss_cond_input);
        VPLCond_Destroy(&tss_cond_threads);
        VPLMutex_Destroy(&tss_mutex);

        LOG_INFO("TSS service thread delete Pool %p", tss_pool);
        delete tss_pool;
        tss_pool = NULL;
    }
    tss_is_init = false;

end:
    LOG_INFO("TSS service thread exit");
    return VPLTHREAD_RETURN_VALUE;
}
