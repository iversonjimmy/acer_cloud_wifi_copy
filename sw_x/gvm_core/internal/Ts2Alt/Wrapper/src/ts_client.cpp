/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

///
/// ts_client.cpp
///
/// Tunnel Service Client Interface

#ifndef IS_TS_WRAPPER
#define IS_TS_WRAPPER
#endif

#include <vplu_types.h>
#include <vpl_th.h>

#include "ts_client.hpp"
#include "ts_pool.hpp"

#include <gvm_errors.h>

#include <ccdi.hpp>

#include <vplex_trace.h>
#include <vplu_mutex_autolock.hpp>

namespace TS_WRAPPER {

class TsFrontEnd {
public:
    TsFrontEnd();
    ~TsFrontEnd();
    TSError_t Init(s32 (*getRouteInfoCb)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo),
                   VPLTime_t _idle_timeouts[MAX_CONN_TYPE],
                   int disabledRoutes,
                   std::string &error_msg);
    TSError_t Shutdown(std::string &error_msg);
    TSError_t RefreshServerInfo(u64 userId, bool forceDropConns, std::string &error_msg);
    TSError_t UpdateServerInfo(u64 userId, u64 deviceId, bool forceDropConns, std::string &error_msg);
    TSError_t RemoveServerInfo(u64 deviceId, std::string &error_msg);
    TSError_t Open(const TSOpenParms_t &parms,
                   TSIOHandle_t &ret_io_handle,
                   std::string &error_msg);
    TSError_t Close(TSIOHandle_t &ioh,
                    std::string &error_msg);
    TSError_t Read(TSIOHandle_t ioh,
                   char *buffer,
                   size_t &buffer_len,
                   std::string &error_msg);
    TSError_t Write(TSIOHandle_t ioh,
                    const char *buffer,
                    size_t buffer_len,
                    std::string &error_msg);
private:
    VPLMutex_t mutex;

    enum State {
        NOTINIT,      // Initial state.
                      // Call to Init() causes state to transition to INITIALIZING.
                      // All other calls result in an error.
        INITIALIZING, // Temporary state; when done, transitions to NORMAL.
                      // All calls result in an error.
        NORMAL,       // Call to *ServerInfo() causes state to transition to UPDATING.
                      // Call to Shutdown() causes state to transition to SHUTTINGDOWN.
                      // Call to Init() results in an error.
                      // Calls to Open(), Close(), Read(), Write() work as expected.
        UPDATING,     // Temporary state; when done, transitions to NORMAL.
                      // Calls to Init() result in an error.
                      // Calls to *ServerInfo() will block, waiting for NORMAL.
                      // Calls to Open(), Close(), Read(), Write(), Shutdown() will block, waiting for NORMAL.
        SHUTTINGDOWN, // Temporary state; when done, transitions to NOTINIT.
                      // All calls result in an error.
    };
    State state;
    VPLCond_t stateIsNormal_cond;

    int numThreads;  // Num of threads entering TsBackEnd (ts_pool.cpp).
    VPLCond_t noThreads_cond;

    std::map<u64, ts_device_pool*> devpools;

    int disabledRoutes;
    VPLTime_t idle_timeouts[MAX_CONN_TYPE];
    s32 (*getRouteInfoCb)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo);

    // mimics stream_service::streamServers, but without vssi session
    std::map<u64, TsRouteInfo> routeinfos;

    TSError_t waitStateNORMAL(std::string &error_msg);
    int updateServerInfo(u64 userId, u64 deviceId, bool forceDropConns);
    void removeServerInfo(u64 deviceId);
    void shutdownScrubbers();
    void waitNoThreads();
    TSError_t waitConnectingTunnels();
    TSError_t disconnectTunnels();
    void purgeTunnels();
    void purgeDevpools();
    void endVssiSessions();

    class TunnelDict {
    public:
        TunnelDict();
        ~TunnelDict();
        TSError_t Add(ts_tunnel *tunnel, TSIOHandle_t &handle, std::string &error_msg);
        TSError_t Find(TSIOHandle_t handle, ts_tunnel *&tunnel, std::string &error_msg);
        TSError_t Remove(TSIOHandle_t handle, std::string &error_msg);
        void Reset();
    private:
        VPLMutex_t mutex;
        int counter;  // used to generate unique number to associate with ts_tunnel obj; okay to wrap around
        std::map<int, ts_tunnel*> dict;
    };
    TunnelDict tunnel_wl;

};

TsFrontEnd::TsFrontEnd()
    : state(NOTINIT), numThreads(0), disabledRoutes(0)
{
    int err = VPL_OK;

    err = VPLMutex_Init(&mutex);
    if (err) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize mutex");
        VPL_SET_UNINITIALIZED(&mutex);
    }

    err = VPLCond_Init(&stateIsNormal_cond);
    if (err) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize condvar");
        VPL_SET_UNINITIALIZED(&stateIsNormal_cond);
    }

    err = VPLCond_Init(&noThreads_cond);
    if (err) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize condvar");
        VPL_SET_UNINITIALIZED(&noThreads_cond);
    }

    for (int i = 0; i < MAX_CONN_TYPE; i++) {
        idle_timeouts[i] = 0;
    }
}

TsFrontEnd::~TsFrontEnd()
{
    if (numThreads > 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Still has user threads into TsBackEnd");
    }
    if (!devpools.empty()) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Still has devpools");
    }

    if (VPL_IS_INITIALIZED(&mutex)) {
        VPLMutex_Destroy(&mutex);
    }
    if (VPL_IS_INITIALIZED(&stateIsNormal_cond)) {
        VPLCond_Destroy(&stateIsNormal_cond);
    }
    if (VPL_IS_INITIALIZED(&noThreads_cond)) {
        VPLCond_Destroy(&noThreads_cond);
    }
}

TSError_t TsFrontEnd::Init(s32 (*_getRouteInfoCb)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo),
                           VPLTime_t _idle_timeouts[MAX_CONN_TYPE],
                           int _disabledRoutes,
                           std::string &error_msg)
{
    TSError_t err = TS_OK;

    if (VPL_IS_UNINITIALIZED(&mutex) ||
        VPL_IS_UNINITIALIZED(&stateIsNormal_cond) ||
        VPL_IS_UNINITIALIZED(&noThreads_cond)) {
        error_msg = "Failed to initialize";
        return TS_ERR_NO_MEM;
    }

    MutexAutoLock lock(&mutex);

    // check state
    // NOTINIT -> proceed
    // INITIALIZING, NORMAL, UPDATING, SHUTTINGDOWN -> fail
    if (state == INITIALIZING) {
        error_msg = "Already initializing";
        return TS_ERR_INVALID;
    }
    if (state == NORMAL || state == UPDATING || state == SHUTTINGDOWN) {
        error_msg = "Already initialized";
        return TS_ERR_INVALID;
    }
    assert(state == NOTINIT);

    // Point of no return.

    state = INITIALIZING;

    disabledRoutes = _disabledRoutes;
    for (int i = 0; i < MAX_CONN_TYPE; i++) {
        idle_timeouts[i] = _idle_timeouts[i];
    }
    getRouteInfoCb = _getRouteInfoCb;

    state = NORMAL;
    VPLCond_Broadcast(&stateIsNormal_cond);

    return err;
}

TSError_t TsFrontEnd::Shutdown(std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);

    // check state
    // NOTINIT, INITIALIZING, SHUTTINGDOWN -> fail
    // UPDATING -> wait
    // NORMAL -> proceed
    if (state == NOTINIT) {
        error_msg = "Not initialized";
        return TS_ERR_NOT_INIT;
    }
    if (state == INITIALIZING) {
        error_msg = "Initializing";
        return TS_ERR_INVALID;
    }
    if (state == SHUTTINGDOWN) {
        error_msg = "Already shutting down";
        return TS_ERR_INVALID;
    }
    assert(state == NORMAL || state == UPDATING);
    tserr = waitStateNORMAL(error_msg);
    if (tserr) {
        return tserr;
    }
    assert(state == NORMAL);

    // Point of no return.

    state = SHUTTINGDOWN;
    shutdownScrubbers();
    disconnectTunnels();  // Optional; trying to expedite user threads to leave early.
    waitNoThreads();
    disconnectTunnels();
    waitConnectingTunnels();
    endVssiSessions();
    if (numThreads == 0) {
        purgeTunnels();
        purgeDevpools();
    }
    devpools.clear();
    tunnel_wl.Reset();
    routeinfos.clear();

    state = NOTINIT;

    return tserr;
}

// replacement for stream_service::update_stream_servers()
TSError_t TsFrontEnd::RefreshServerInfo(u64 userId, bool forceDropConns, std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);

    // check state
    // NOTINIT, INITIALIZING, UPDATING, SHUTTINGDOWN -> fail
    // UPDATING -> wait
    // NORMAL -> proceed
    if (state == NOTINIT) {
        error_msg = "Not initialized";
        return TS_ERR_NOT_INIT;
    }
    if (state == INITIALIZING) {
        error_msg = "Initializing";
        return TS_ERR_INVALID;
    }
    if (state == SHUTTINGDOWN) {
        error_msg = "Shutting down";
        return TS_ERR_INVALID;
    }
    assert(state == NORMAL || state == UPDATING);
    tserr = waitStateNORMAL(error_msg);
    if (tserr) {
        return tserr;
    }
    assert(state == NORMAL);

    // Point of no return.

    state = UPDATING;

    // Create a set of linked CloudPCs.
    std::set<u64> linkedServers;
    {
        ccd::ListLinkedDevicesInput req;
        ccd::ListLinkedDevicesOutput resp;
        req.set_user_id(userId);
        req.set_storage_nodes_only(true);
        req.set_only_use_cache(true);
        int err = CCDIListLinkedDevices(req, resp);
        if (err) {
            tserr = err;
            goto end;
        }

        for (int i = 0; i < resp.devices_size(); i++) {
            linkedServers.insert(resp.devices(i).device_id());
        }
    }

    // Remove routeinfo of CloudPCs that are no longer linked.
    {
        std::map<u64, TsRouteInfo>::iterator it;
        for (it = routeinfos.begin(); it != routeinfos.end(); ) {
            std::map<u64, TsRouteInfo>::iterator tmp = it++;
            if (linkedServers.find(tmp->first) == linkedServers.end()) {
                routeinfos.erase(tmp);
            }
        }
    }

    // Disconnect connections to CloudPCs that are no longer linked.
    {
        std::map<u64, ts_device_pool*>::iterator it;
        for (it = devpools.begin(); it != devpools.end(); it++) {
            if (linkedServers.find(it->first) == linkedServers.end()) {
                it->second->DisconnectTunnels();
            }
        }
    }

    {
        std::set<u64>::iterator it;
        for (it = linkedServers.begin(); it != linkedServers.end(); it++) {
            updateServerInfo(userId, *it, forceDropConns);
        }
    }

 end:
    state = NORMAL;
    VPLCond_Broadcast(&stateIsNormal_cond);

    return tserr;
}

// replacement for stream_service::add_or_change_stream_server()
TSError_t TsFrontEnd::UpdateServerInfo(u64 userId, u64 deviceId, bool forceDropConns, std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);

    // check state
    // NOTINIT, INITIALIZING, UPDATING, SHUTTINGDOWN -> fail
    // UPDATING -> wait
    // NORMAL -> proceed
    if (state == NOTINIT) {
        error_msg = "Not initialized";
        return TS_ERR_NOT_INIT;
    }
    if (state == INITIALIZING) {
        error_msg = "Initializing";
        return TS_ERR_INVALID;
    }
    if (state == SHUTTINGDOWN) {
        error_msg = "Shutting down";
        return TS_ERR_INVALID;
    }
    assert(state == NORMAL || state == UPDATING);
    tserr = waitStateNORMAL(error_msg);
    if (tserr) {
        return tserr;
    }
    assert(state == NORMAL);

    // Point of no return.

    state = UPDATING;

    updateServerInfo(userId, deviceId, forceDropConns);

    state = NORMAL;
    VPLCond_Broadcast(&stateIsNormal_cond);

    return tserr;
}

// replacement for stream_service::remove_stream_server()
TSError_t TsFrontEnd::RemoveServerInfo(u64 deviceId, std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);

    // check state
    // NOTINIT, INITIALIZING, UPDATING, SHUTTINGDOWN -> fail
    // UPDATING -> wait
    // NORMAL -> proceed
    if (state == NOTINIT) {
        error_msg = "Not initialized";
        return TS_ERR_NOT_INIT;
    }
    if (state == INITIALIZING) {
        error_msg = "Initializing";
        return TS_ERR_INVALID;
    }
    if (state == SHUTTINGDOWN) {
        error_msg = "Shutting down";
        return TS_ERR_INVALID;
    }
    assert(state == NORMAL || state == UPDATING);
    tserr = waitStateNORMAL(error_msg);
    if (tserr) {
        return tserr;
    }
    assert(state == NORMAL);

    // Point of no return.

    state = UPDATING;

    removeServerInfo(deviceId);

    state = NORMAL;
    VPLCond_Broadcast(&stateIsNormal_cond);

    return tserr;
}

TSError_t TsFrontEnd::Open(const TSOpenParms_t &parms,
                           TSIOHandle_t &ret_io_handle,
                           std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    {
        MutexAutoLock lock(&mutex);

        // check state
        // NOTINIT, INITIALIZING, SHUTTINGDOWN -> fail
        // UPDATING -> wait
        // NORMAL -> proceed
        if (state == NOTINIT) {
            error_msg = "Not initialized";
            return TS_ERR_NOT_INIT;
        }
        if (state == INITIALIZING) {
            error_msg = "Initializing";
            return TS_ERR_INVALID;
        }
        if (state == SHUTTINGDOWN) {
            error_msg = "Shutting down";
            return TS_ERR_INVALID;
        }
        assert(state == NORMAL || state == UPDATING);
        tserr = waitStateNORMAL(error_msg);
        if (tserr) {
            return tserr;
        }
        assert(state == NORMAL);

        numThreads++;
    }

    ts_device_pool *devpool = NULL;
    {
        MutexAutoLock lock(&mutex);
        std::map<u64, ts_device_pool*>::iterator it = devpools.find(parms.device_id);
        if (it != devpools.end()) {
            devpool = it->second;
        }
        else {
            std::map<u64, TsRouteInfo>::iterator ri_it = routeinfos.find(parms.device_id);
            if (ri_it == routeinfos.end()) {
                error_msg = "No routes to destination";
                tserr = TS_ERR_NO_ROUTE;
                goto out;
            }
            devpool = new (std::nothrow) ts_device_pool(parms.device_id, ri_it->second, idle_timeouts);
            if (!devpool) {
                error_msg = "Not enough memory";
                tserr = TS_ERR_NO_MEM;
                goto out;
            }
            tserr = devpool->StartScrubber(error_msg);
            if (tserr) {
                delete devpool;
                goto out;
            }
            devpools[parms.device_id] = devpool;
        }
    }
    assert(devpool);

    {
        ts_tunnel *tunnel = NULL;
        tserr = devpool->tunnel_get(parms.user_id, disabledRoutes, tunnel, error_msg);
        if (!tserr) {
            tserr = tunnel_wl.Add(tunnel, ret_io_handle, error_msg);
        }

        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          TS_FMT_TARG": ioh: %p, err %d: %s",
                          parms.user_id, parms.device_id,
                          tunnel, tserr, error_msg.c_str()); 
    }

 out:
    {
        MutexAutoLock lock(&mutex);
        numThreads--;
        if (numThreads == 0) {
            VPLCond_Broadcast(&noThreads_cond);
        }
    }

    return tserr;
}

TSError_t TsFrontEnd::Close(TSIOHandle_t &ioh,
                            std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    {
        MutexAutoLock lock(&mutex);

        // check state
        // NOTINIT, INITIALIZING, SHUTTINGDOWN -> fail
        // UPDATING -> wait
        // NORMAL -> proceed
        if (state == NOTINIT) {
            error_msg = "Not initialized";
            return TS_ERR_NOT_INIT;
        }
        if (state == INITIALIZING) {
            error_msg = "Initializing";
            return TS_ERR_INVALID;
        }
        if (state == SHUTTINGDOWN) {
            error_msg = "Shutting down";
            return TS_ERR_INVALID;
        }
        assert(state == NORMAL || state == UPDATING);
        tserr = waitStateNORMAL(error_msg);
        if (tserr) {
            return tserr;
        }
        assert(state == NORMAL);

        numThreads++;
    }

    ts_tunnel* tunnel = NULL;
    tserr = tunnel_wl.Find(ioh, tunnel, error_msg);
    if (tserr) {
        goto done;
    }

    {
        std::string _error_msg;
        tserr = tunnel->reset(_error_msg);
        if ( tserr != TS_OK ) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "tunnel->reset() %d:%s",
                              tserr, _error_msg.c_str());
            // The following put will delete it.
            tserr = TS_OK;
        }
    }

    tserr = tunnel->get_devpool()->tunnel_put(tunnel, /*is_assigned*/true, error_msg);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "ioh: %p, err %d: %s",
                      ioh, tserr, error_msg.c_str()); 

    if ( tserr == TS_OK ) {
        tserr = tunnel_wl.Remove(ioh, error_msg);
        ioh = NULL;
    }

 done:
    {
        MutexAutoLock lock(&mutex);
        numThreads--;
        if (numThreads == 0) {
            VPLCond_Broadcast(&noThreads_cond);
        }
    }

    return tserr;
}

TSError_t TsFrontEnd::Read(TSIOHandle_t ioh,
                           char *buffer,
                           size_t &buffer_len,
                           std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    {
        MutexAutoLock lock(&mutex);

        // check state
        // NOTINIT, INITIALIZING, SHUTTINGDOWN -> fail
        // UPDATING -> wait
        // NORMAL -> proceed
        if (state == NOTINIT) {
            error_msg = "Not initialized";
            return TS_ERR_NOT_INIT;
        }
        if (state == INITIALIZING) {
            error_msg = "Initializing";
            return TS_ERR_INVALID;
        }
        if (state == SHUTTINGDOWN) {
            error_msg = "Shutting down";
            return TS_ERR_INVALID;
        }
        assert(state == NORMAL || state == UPDATING);
        tserr = waitStateNORMAL(error_msg);
        if (tserr) {
            return tserr;
        }
        assert(state == NORMAL);

        numThreads++;
    }

    ts_tunnel* tunnel = NULL;
    tserr = tunnel_wl.Find(ioh, tunnel, error_msg);
    if (tserr) {
        goto done;
    }

    tserr = tunnel->read(buffer, buffer_len, error_msg);

 done:
    {
        MutexAutoLock lock(&mutex);
        numThreads--;
        if (numThreads == 0) {
            VPLCond_Broadcast(&noThreads_cond);
        }
    }

    return tserr;
}

TSError_t TsFrontEnd::Write(TSIOHandle_t ioh,
                            const char *buffer,
                            size_t buffer_len,
                            string &error_msg)
{
    TSError_t tserr = TS_OK;

    {
        MutexAutoLock lock(&mutex);

        // check state
        // NOTINIT, INITIALIZING, SHUTTINGDOWN -> fail
        // UPDATING -> wait
        // NORMAL -> proceed
        if (state == NOTINIT) {
            error_msg = "Not initialized";
            return TS_ERR_NOT_INIT;
        }
        if (state == INITIALIZING) {
            error_msg = "Initializing";
            return TS_ERR_INVALID;
        }
        if (state == SHUTTINGDOWN) {
            error_msg = "Shutting down";
            return TS_ERR_INVALID;
        }
        assert(state == NORMAL || state == UPDATING);
        tserr = waitStateNORMAL(error_msg);
        if (tserr) {
            return tserr;
        }
        assert(state == NORMAL);

        numThreads++;
    }

    ts_tunnel* tunnel = NULL;
    tserr = tunnel_wl.Find(ioh, tunnel, error_msg);
    if (tserr) {
        goto done;
    }

    tserr = tunnel->write(buffer, buffer_len, error_msg);

 done:
    {
        MutexAutoLock lock(&mutex);
        numThreads--;
        if (numThreads == 0) {
            VPLCond_Broadcast(&noThreads_cond);
        }
    }

    return tserr;
}

TSError_t TsFrontEnd::waitStateNORMAL(std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    while (state != NORMAL) {
        int err = VPLCond_TimedWait(&stateIsNormal_cond, &mutex, VPL_TIMEOUT_NONE);
        if (err) {
            error_msg = "Could not enter correct state";
            return TS_ERR_NO_SERVICE;
        }
    }
    assert(state == NORMAL);

    return tserr;
}

int TsFrontEnd::updateServerInfo(u64 userId, u64 deviceId, bool forceDropConns)
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    TsRouteInfo routeInfo;
    {
        s32 err = getRouteInfoCb(userId, deviceId, &routeInfo);
        if (err) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to get route info; err %d", err);
            return err;
        }
    }

    bool endVssiSession = false;
    {
        std::map<u64, TsRouteInfo>::iterator it = routeinfos.find(deviceId);
        if (it != routeinfos.end()) {
            if (routeInfo.directInternalAddr != it->second.directInternalAddr) {
                it->second.directInternalAddr = routeInfo.directInternalAddr;
                forceDropConns = true;
            }
            if (routeInfo.directExternalAddr != it->second.directExternalAddr) {
                it->second.directExternalAddr = routeInfo.directExternalAddr;
                forceDropConns = true;
            }
            if (routeInfo.directPort != it->second.directPort) {
                it->second.directPort = routeInfo.directPort;
                forceDropConns = true;
            }
            if (routeInfo.proxyAddr != it->second.proxyAddr) {
                it->second.proxyAddr = routeInfo.proxyAddr;
                forceDropConns = true;
            }
            if (routeInfo.proxyPort != it->second.proxyPort) {
                it->second.proxyPort = routeInfo.proxyPort;
                forceDropConns = true;
            }
        }
        else {
            routeinfos[deviceId].directInternalAddr = routeInfo.directInternalAddr;
            routeinfos[deviceId].directExternalAddr = routeInfo.directExternalAddr;
            routeinfos[deviceId].directPort = routeInfo.directPort;
            routeinfos[deviceId].proxyAddr = routeInfo.proxyAddr;
            routeinfos[deviceId].proxyPort = routeInfo.proxyPort;
        }

        if (routeInfo.vssiSessionHandle && !routeInfo.vssiServiceTicket.empty()) {
            if (routeInfo.vssiSessionHandle != routeinfos[deviceId].vssiSessionHandle) {
                forceDropConns = true;
                endVssiSession = true;
            }
            routeinfos[deviceId].vssiSessionHandle = routeInfo.vssiSessionHandle;
            routeinfos[deviceId].vssiServiceTicket = routeInfo.vssiServiceTicket;
        }
    }

    if (forceDropConns) {
        std::map<u64, ts_device_pool*>::iterator it = devpools.find(deviceId);
        if (it != devpools.end()) {
            it->second->DisconnectTunnels();  // Optional; trying to possibly expedite threads leaving early
            waitNoThreads();
            it->second->DisconnectTunnels();
            if (endVssiSession) {
                it->second->WaitConnectingTunnels();
                it->second->EndVssiSession();
            }
            it->second->UpdateRouteInfo(routeinfos[deviceId]);
        }
    }

    return 0;
}

void TsFrontEnd::removeServerInfo(u64 deviceId)
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    {
        std::map<u64, TsRouteInfo>::iterator it = routeinfos.find(deviceId);
        if (it != routeinfos.end()) {
            routeinfos.erase(it);
        }
    }

    {
        std::map<u64, ts_device_pool*>::iterator it = devpools.find(deviceId);
        if (it != devpools.end()) {
            it->second->StopScrubber();
            it->second->DisconnectTunnels();  // Optional; trying to possibly expedite threads leaving early
            waitNoThreads();
            it->second->DisconnectTunnels();
            it->second->WaitConnectingTunnels();
            it->second->EndVssiSession();
            delete it->second;
            devpools.erase(it);
        }
    }
}

void TsFrontEnd::shutdownScrubbers()
{
    MutexAutoLock lock(&mutex);
    std::map<u64, ts_device_pool*>::iterator it;
    for (it = devpools.begin(); it != devpools.end(); it++) {
        ts_device_pool *devpool = it->second;
        devpool->StopScrubber();
    }
}

void TsFrontEnd::waitNoThreads()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    while (numThreads > 0) {
        int err = VPLCond_TimedWait(&noThreads_cond, &mutex, VPL_TIMEOUT_NONE);
        if (err) {
            break;
        }
    }
    if (numThreads > 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to wait for num of threads to go 0");
    }
}

TSError_t TsFrontEnd::waitConnectingTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    TSError_t tserr = TS_OK;

    std::map<u64, ts_device_pool*>::iterator it;
    for (it = devpools.begin(); it != devpools.end(); it++) {
        TSError_t tserr2 = it->second->WaitConnectingTunnels();
        if (tserr2 && !tserr) {  // return the first error
            tserr = tserr2;
        }
    }
    if (tserr) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to wait for num of connecting tunnels to go 0");
    }

    return tserr;
}

TSError_t TsFrontEnd::disconnectTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    TSError_t tserr = TS_OK;

    std::map<u64, ts_device_pool*>::iterator it;
    for (it = devpools.begin(); it != devpools.end(); it++) {
        TSError_t tserr2 = it->second->DisconnectTunnels();
        if (tserr2 && !tserr) {  // return first error
            tserr = tserr2;
        }
    }
    if (tserr) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to disconnect all tunnels");
    }

    return tserr;
}

void TsFrontEnd::purgeTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    std::map<u64, ts_device_pool*>::iterator it;
    for (it = devpools.begin(); it != devpools.end(); it++) {
        std::string _error_msg;
        it->second->PurgeTunnels(_error_msg);
    }
}

void TsFrontEnd::purgeDevpools()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    std::map<u64, ts_device_pool*>::iterator it;
    for (it = devpools.begin(); it != devpools.end(); it++) {
        delete it->second;
    }
    devpools.clear();
}

void TsFrontEnd::endVssiSessions()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    std::map<u64, ts_device_pool*>::iterator it;
    for (it = devpools.begin(); it != devpools.end(); it++) {
        it->second->EndVssiSession();
    }
}

TsFrontEnd::TunnelDict::TunnelDict()
    : counter(0)
{
    VPLMutex_Init(&mutex);
}

TsFrontEnd::TunnelDict::~TunnelDict()
{
    VPLMutex_Destroy(&mutex);
}

TSError_t TsFrontEnd::TunnelDict::Add(ts_tunnel *tunnel, TSIOHandle_t &handle, std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);
    handle = (TSIOHandle_t)counter++;
    dict[(int)handle] = tunnel;

    return tserr;
}

TSError_t TsFrontEnd::TunnelDict::Find(TSIOHandle_t handle, ts_tunnel *&tunnel, std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);
    std::map<int, ts_tunnel*>::iterator it = dict.find((int)handle);
    if (it == dict.end()) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Invalid handle %p", handle);
        error_msg = "Invalid handle";
        tserr = TS_ERR_INVALID;
    }
    else {
        tunnel = it->second;
    }

    return tserr;
}

TSError_t TsFrontEnd::TunnelDict::Remove(TSIOHandle_t handle, std::string &error_msg)
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);
    std::map<int, ts_tunnel*>::iterator it = dict.find((int)handle);
    if (it == dict.end()) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Invalid handle %p", handle);
        error_msg = "Invalid handle";
        tserr = TS_ERR_INVALID;
    }
    else {
        dict.erase(it);
    }

    return tserr;
}

void TsFrontEnd::TunnelDict::Reset()
{
    dict.clear();
}

static TsFrontEnd tsFrontEnd;

TSError_t TS_Init(u64 userId,
                  u64 deviceId,
                  u32 instanceId,
                  s32 (*getRouteInfoCb)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo),
                  VPLTime_t din_idle_timeout,
                  VPLTime_t p2p_idle_timeout,
                  VPLTime_t prx_idle_timeout,
                  int disabled_route_types,
                  Ts2::LocalInfo* localInfo,
                  std::string &error_msg)
{
    VPLTime_t idle_timeout[] = {
        din_idle_timeout,
        0,
        p2p_idle_timeout,
        prx_idle_timeout,
    };

    return tsFrontEnd.Init(getRouteInfoCb, idle_timeout, disabled_route_types, error_msg);
}

void TS_Shutdown()
{
    std::string error_msg;
    tsFrontEnd.Shutdown(error_msg);
}

void TS_RefreshServerInfo(u64 userId, bool forceDropConns)
{
    std::string error_msg;
    tsFrontEnd.RefreshServerInfo(userId, forceDropConns, error_msg);
}

void TS_UpdateServerInfo(u64 userId, u64 deviceId, bool forceDropConns)
{
    std::string error_msg;
    tsFrontEnd.UpdateServerInfo(userId, deviceId, forceDropConns, error_msg);
}

void TS_RemoveServerInfo(u64 deviceId)
{
    std::string error_msg;
    tsFrontEnd.RemoveServerInfo(deviceId, error_msg);
}

TSError_t TS_Open(const TSOpenParms_t &parms,
                  TSIOHandle_t &ret_io_handle,
                  std::string &error_msg)
{
    return tsFrontEnd.Open(parms, ret_io_handle, error_msg);
}

TSError_t TS_Close(TSIOHandle_t &ioh,
                   std::string &error_msg)
{
    return tsFrontEnd.Close(ioh, error_msg);
}

TSError_t TS_Read(TSIOHandle_t ioh,
                  char *buffer,
                  size_t &buffer_len,
                  std::string &error_msg)
{
    return tsFrontEnd.Read(ioh, buffer, buffer_len, error_msg);
}

TSError_t TS_Write(TSIOHandle_t ioh,
                   const char *buffer,
                   size_t buffer_len,
                   std::string &error_msg)
{
    return tsFrontEnd.Write(ioh, buffer, buffer_len, error_msg);
}

TSError_t TS_GetPort(int& port_out)
{
    // TODO: get port from VSSI
    port_out = 0;

    return TS_OK;
}

TSError_t TS_GetServerLocalPort(int& port_out)
{
    // Not implemented
    return TS_OK;
}

}  // namespace TS_WRAPPER guard
