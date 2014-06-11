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
/// ts_pool.cpp
///
/// Tunnel Service Interface

#include "vplu_types.h"
#include "vpl_th.h"
#include "ts_pool.hpp"
#include "vssi_error.h"
#include <vplex_trace.h>
#include "vplex_vs_directory.h"
#include <vplu_mutex_autolock.hpp>
#include "ccdi.hpp"
#include <map>
#include <gvm_thread_utils.h>

using namespace std;

namespace TS_WRAPPER
{

// TODO for 2.7.
// See if we can lower the stack requirement 16KB (UTIL_DEFAULT_THREAD_STACK_SIZE) for non-Android.
static size_t Connector_StackSize = 
#ifdef ANDROID
    UTIL_DEFAULT_THREAD_STACK_SIZE
#else
    128 * 1024
#endif
    ;
static size_t Scrubber_StackSize = 
#ifdef ANDROID
    UTIL_DEFAULT_THREAD_STACK_SIZE
#else
    32 * 1024
#endif
    ;

static const char *conn_type_name[] = {
    "DIN",
    "DEX",
    "P2P",
    "PRX"
};

static int conn_types[] = {
    VSSI_SECURE_TUNNEL_DIRECT_INTERNAL,
    VSSI_SECURE_TUNNEL_DIRECT,
    VSSI_SECURE_TUNNEL_PROXY_P2P,
    VSSI_SECURE_TUNNEL_PROXY
};

static int conn_types_to_ind[] = {
    -1,
    1,
    3,
    2,
    0
};

typedef struct {
    VSSI_Result     result;
    VPLSem_t        *sem;
} TS_ioCtxt_t;

//----------------------------------------------------------------------

ts_tunnel::ts_tunnel(u64 device_id, int conn_type, VSSI_Session vssiSession,
                     const std::string &serverHostname, u16 serverPort,
                     ts_device_pool* devpool)
    : device_id(device_id),
      conn_type(conn_type),
      has_tunnel(false),
      is_failed(false),
      vssiSession(vssiSession),
      devpool(devpool),
      last_active(VPLTIME_INVALID),
      serverHostname(serverHostname),
      serverPort(serverPort),
      numThreadsInVssi(0)
{
    VPLMutex_Init(&mutex);

    // Defer initializing these sync primitives until connection is established.
    // See ts_tunnel::Connect() for initializations.
    VPL_SET_UNINITIALIZED(&read_sem);
    VPL_SET_UNINITIALIZED(&write_sem);
    VPL_SET_UNINITIALIZED(&noThreadsInVssi_cond);
}

ts_tunnel::~ts_tunnel()
{
    if (has_tunnel) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "ts_tunnel[%p] still has VSSI tunnel", this);
        Disconnect();
    }

    // Destroy the semaphores and mutexes
    VPLMutex_Destroy(&mutex);
    if ( VPL_IS_INITIALIZED(&read_sem) ) {
        VPLSem_Destroy(&read_sem);
    }
    if ( VPL_IS_INITIALIZED(&write_sem) ) {
        VPLSem_Destroy(&write_sem);
    }
    if ( VPL_IS_INITIALIZED(&noThreadsInVssi_cond) ) {
        VPLCond_Destroy(&noThreadsInVssi_cond);
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_tunnel[%p] destroyed", this);
}

void ts_tunnel::adjustNumThreadsInVssi(bool isEntering)
{
    MutexAutoLock lock(&mutex);
    if (isEntering) {
        numThreadsInVssi++;
    }
    else {
        numThreadsInVssi--;
        if (numThreadsInVssi == 0) {
            VPLCond_Signal(&noThreadsInVssi_cond);
        }
    }
    assert(numThreadsInVssi >= 0);
}

void ts_tunnel::disconnect()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    if ( has_tunnel ) {
        has_tunnel = false;  // indicate the tunnel is in the process of being disconnected
        while (numThreadsInVssi > 0) {
            int err = VPLCond_TimedWait(&noThreadsInVssi_cond, &mutex, VPL_TIMEOUT_NONE);
            if (err) {
                break;
            }
        }
        VSSI_SecureTunnelDisconnect(tunnel_handle);
    }
    if (!is_failed) {
        is_failed = true;
    }
}

void ts_tunnel::Disconnect()
{
    // mutex should *not* be taken yet
    assert(!VPLMutex_LockedSelf(&mutex));

    MutexAutoLock lock(&mutex);
    disconnect();
}

VPLTHREAD_FN_DECL ts_tunnel::connectorMain(void *param)
{
    ts_tunnel *tunnel = (ts_tunnel*)param;
    ts_device_pool *devpool = tunnel->get_devpool();
    std::string _error_msg;
    TSError_t tserr = tunnel->Connect(_error_msg);
    // tunnel->Connect() logs any error msgs
    if (tserr) {
        devpool->NotifyConnectionAttemptFinished(tunnel);
        tunnel->Disconnect();
        delete tunnel;
    }
    else {
        devpool->tunnel_put(tunnel, false, _error_msg);  // "false" = not in prior use
        devpool->NotifyConnectionAttemptFinished(tunnel);
    }

    return VPLTHREAD_RETURN_VALUE;
}

TSError_t ts_tunnel::ConnectAsync(std::string &error_msg)
{
    int err = Util_SpawnThread(connectorMain, VPL_AS_THREAD_FUNC_ARG(this), Connector_StackSize, /*isJoinable*/VPL_FALSE, NULL);
    if (err) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unable to spawn connection thread: err %d", err);
        error_msg = "Unable to spawn connection thread";

        // caller is responsible for destroying ts_tunnel obj.
    }
    return err;
}

class VssiConnectHelper {
public:
    VssiConnectHelper() : result(0) {
        VPLSem_Init(&sem, 1, 0);
    }
    ~VssiConnectHelper() {
        VPLSem_Destroy(&sem);
    }
    void Wait() {
        VPLSem_Wait(&sem);
    }
    void Post() {
        VPLSem_Post(&sem);
    }
    void SetResult(VSSI_Result result) {
        this->result = result;
    }
    VSSI_Result GetResult() {
        return result;
    }
private:
    VPLSem_t sem;
    VSSI_Result result;
};

static void vssi_connect_cb(void *_ctx, VSSI_Result result)
{
    VssiConnectHelper *vch = (VssiConnectHelper*)_ctx;
    vch->SetResult(result);
    vch->Post();
}

TSError_t ts_tunnel::Connect(std::string &error_msg)
{
    TSError_t err = TS_OK;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                      "Connecting %s tunnel %p to "FMTu64,
                      conn_type_name[conn_types_to_ind[get_conn_type()]],
                      this, device_id);

    {
        MutexAutoLock lock(&mutex);
        {
            VssiConnectHelper vch;
            VSSI_SecureTunnelConnect(vssiSession,
                                     serverHostname.c_str(),
                                     serverPort,
                                     (VSSI_SecureTunnelConnectType)conn_type,
                                     device_id,
                                     &tunnel_handle,
                                     &vch,
                                     (VSSI_Callback)vssi_connect_cb);
            vch.Wait();
            err = vch.GetResult();
        }
        if (err == VSSI_SUCCESS) {
            VPLSem_Init(&read_sem, VPLSEM_MAX_COUNT, 0);
            VPLSem_Init(&write_sem, VPLSEM_MAX_COUNT, 0);
            VPLCond_Init(&noThreadsInVssi_cond);

            has_tunnel = true;
        }
    }

    if ( err != TS_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to connect %s tunnel %p to "FMTu64": err %d:%s",
                         conn_type_name[conn_types_to_ind[get_conn_type()]], this, device_id, err, error_msg.c_str());
    }

    return err;
}

static void TS_io_cb(TS_ioCtxt_t* io_ctxt,
                     VSSI_Result result)
{
    io_ctxt->result = result;
    if (io_ctxt->sem) {
        VPLSem_Post(io_ctxt->sem);
    }
}

TSError_t ts_tunnel::read(char* buffer, size_t& buffer_len, string& error_msg)
{
    TSError_t err = TS_OK;
    VSSI_Result vss_err;
    size_t received = 0;
    TS_ioCtxt_t io_ctxt;

    {
        MutexAutoLock lock(&mutex);
        if ( !has_tunnel || is_failed ) {
            err = TS_ERR_COMM;
            error_msg = "Lost connection with the server.";
            goto done;
        }
    }

    for (;;) {
        adjustNumThreadsInVssi(/*isEntering*/true);
        vss_err = VSSI_SecureTunnelReceive(tunnel_handle, buffer, buffer_len);
        adjustNumThreadsInVssi(/*isEntering*/false);
        if ( vss_err >= 0 ) {  // Got some data; vss_err = num of bytes obtained.
            // We will return with whatever amount of data we got.
            received = vss_err;
            break;
        }

        if ( vss_err == VSSI_AGAIN ) {  // Nothing available right now.
            // Wait until there are more data ready.
            io_ctxt.sem = &read_sem;
            adjustNumThreadsInVssi(/*isEntering*/true);
            VSSI_SecureTunnelWaitToReceive(tunnel_handle, &io_ctxt, (VSSI_Callback)TS_io_cb);
            adjustNumThreadsInVssi(/*isEntering*/false);
            VPLSem_Wait(&read_sem);
            vss_err = io_ctxt.result;
            if ( vss_err == VSSI_SUCCESS ) {  // Data became available.
                continue;
            }
        }

        if ( vss_err == VSSI_ABORTED ) {  // Tunnel been reset.
            error_msg = "The VSSI command was aborted.";
            buffer_len = 0;
            err = TS_OK;
        } else {
            // Something went wrong, so bail out.
            {
                MutexAutoLock lock(&mutex);
                is_failed = true;
            }
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Lost connection to "FMTu64" vssi err %d",
                              device_id, vss_err);
            error_msg = "Lost connection with the server.";
            err = TS_ERR_COMM;
        }
        goto done;
    }

    buffer_len = received;

done:
    if ( err ) {
        buffer_len = 0;
    }

    return err;
}

TSError_t ts_tunnel::write(const char* buffer, size_t buffer_len, string& error_msg)
{
    TSError_t err = TS_OK;
    VSSI_Result vss_err;
    size_t sent = 0;
    TS_ioCtxt_t io_ctxt;

    {
        MutexAutoLock lock(&mutex);
        if ( !has_tunnel || is_failed ) {
            err = TS_ERR_COMM;
            error_msg = "Lost connection with the server.";
            goto done;
        }
    }

    for( ;; ) {
        adjustNumThreadsInVssi(/*isEntering*/true);
        vss_err = VSSI_SecureTunnelSend(tunnel_handle, &buffer[sent], buffer_len);
        adjustNumThreadsInVssi(/*isEntering*/false);
        if ( vss_err > 0 ) {  // Sent some data; vss_err = num of bytes sent.
            sent += vss_err;
            buffer_len -= vss_err;
            if ( buffer_len == 0 ) {  // All the data have been sent.
                break;
            }

            // We still have more data to send.
            // Pretend tunnel was not ready to send; this will trigger the code below.
            vss_err = VSSI_AGAIN;
        }

        if ( vss_err == VSSI_AGAIN ) {  // Tunnel not ready to send right now.
            // Wait until tunnel is ready to send.
            io_ctxt.sem = &write_sem;
            adjustNumThreadsInVssi(/*isEntering*/true);
            VSSI_SecureTunnelWaitToSend(tunnel_handle, &io_ctxt, (VSSI_Callback)TS_io_cb);
            adjustNumThreadsInVssi(/*isEntering*/false);
            VPLSem_Wait(&write_sem);
            vss_err = io_ctxt.result;
            if ( vss_err == VSSI_SUCCESS ) {  // Tunnel is now ready to send.
                continue;
            }
        }

        // Something went wrong, so bail out.
        {
            MutexAutoLock lock(&mutex);
            is_failed = true;
        }
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Lost connection to "FMTu64" vssi err %d",
                          device_id, vss_err);
        error_msg = "Lost connection with the server.";
        err = TS_ERR_COMM;
        goto done;
    }

done:

    return err;
}

TSError_t ts_tunnel::reset(string& error_msg)
{
    TSError_t err = TS_OK;
    VSSI_Result vss_err;
    TS_ioCtxt_t io_ctxt;

    {
        MutexAutoLock lock(&mutex);
        if ( !has_tunnel || is_failed ) {
            err = TS_ERR_COMM;
            error_msg = "Lost connection with the server.";
            goto done;
        }
    }

    // Treat a reset like a write call.
    io_ctxt.sem = &write_sem;
    adjustNumThreadsInVssi(/*isEntering*/true);
    VSSI_SecureTunnelReset(tunnel_handle, &io_ctxt, (VSSI_Callback)TS_io_cb);
    adjustNumThreadsInVssi(/*isEntering*/false);
    VPLSem_Wait(&write_sem);
    vss_err = io_ctxt.result;

    if ( vss_err != VSSI_SUCCESS ) {
        {
            MutexAutoLock lock(&mutex);
            is_failed = true;

            // If reset req times out, vssi will destroy the tunnel obj and return VSSI_COMM.
            // In this case, assume tunnel is already gone.
            if (vss_err == VSSI_COMM) {
                has_tunnel = false;
            }
        }
        err = TS_ERR_COMM;
        error_msg = "Reset of tunnel failed";
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VSSI_SecureTunnelReset() - %d (%d)", err, vss_err);
        goto done;
    }

    // All data in the tunnel should be gone at this point, as well.

done:

    return err;
}

//----------------------------------------------------------------------

ts_device_pool::ts_device_pool(u64 device_id, const TsRouteInfo &routeInfo, VPLTime_t timeout[4])
    : device_id(device_id),
      routeInfo(routeInfo),
      idle_tunnels_cnt(0),
      assigned_tunnels_cnt(0),
      scrubber_running(false),
      shutdown_stop(false),
      vssiSession(NULL)
{
    VPLMutex_Init(&mutex);
    VPLCond_Init(&have_idle_tunnels_cond);
    VPLCond_Init(&no_connecting_tunnels_cond);
    VPLCond_Init(&scrubber_cond);

    for (int i = 0; i < 4; i++) {
        conn_type_timeouts[i] = timeout[i];
    }
}

ts_device_pool::~ts_device_pool()
{
    if (!connecting_tunnels.empty()) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "ts_device_pool[%p] still has "FMTu_size_t" connecting tunnels", this, connecting_tunnels.size());
    }
    if (idle_tunnels_cnt != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "ts_device_pool[%p] still has %d idle tunnels", this, idle_tunnels_cnt);
    }
    if (assigned_tunnels_cnt != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "ts_device_pool[%p] still has %d assigned tunnels", this, assigned_tunnels_cnt);
    }
    if (scrubber_running) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "ts_device_pool[%p] still has scrubber running", this);
    }
    if (vssiSession) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "ts_device_pool[%p] still has VSSI session", this);
    }

    VPLMutex_Destroy(&mutex);
    VPLCond_Destroy(&have_idle_tunnels_cond);
    VPLCond_Destroy(&no_connecting_tunnels_cond);
    VPLCond_Destroy(&scrubber_cond);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_device_pool[%p] destroyed", this);
}

void ts_device_pool::UpdateRouteInfo(const TsRouteInfo &_routeInfo)
{
    routeInfo = _routeInfo;
}

void ts_device_pool::scrubIdleConnections(VPLTime_t timestamp, VPLTime_t *recall_in_out)
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    if (idle_tunnels_cnt == 0) {  // no idle connections
        if (recall_in_out) {
            *recall_in_out = VPL_TIMEOUT_NONE;
        }
        return;
    }

    VPLTime_t recall_in = VPL_TIMEOUT_NONE;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Scrubbing idle connections for device "FMTu64", timestamp "FMTu64, device_id, timestamp);

    bool destroy_all = false;

    std::list<ts_tunnel*> del_list;

    for( int i = 0 ; i < MAX_CONN_TYPE ; i++ ) {

        VPLTime_t timeout = conn_type_timeouts[i];
        if (timeout == 0) {
            // no timeout set for this connection type - skip
            continue;
        }

        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Found "FMTu_size_t" connections of type %d", idle_tunnels[i].size(), i);

        // walk each individual queue
        TSTunnelList_t::iterator tlit;
        TSTunnelList_t::iterator tmp;
        for( tlit = idle_tunnels[i].begin() ; tlit != idle_tunnels[i].end() ; tlit = tmp ) {
            tmp = tlit;
            tmp++;
            VPLTime_t diff = VPLTime_DiffClamp(timestamp, (*tlit)->get_last_active());
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "tunnel %p, last "FMTu64", diff "FMTu64, *tlit, (*tlit)->get_last_active(), diff);

            if ( !destroy_all && (diff < timeout) ) {
                if (recall_in_out) {
                    // convert to time remaining, so we can advise caller when to call back
                    diff = VPLTime_DiffClamp(timeout, diff);
                    if ( diff < recall_in ) {
                        recall_in = diff;
                    }
                }
                continue;
            }

            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Schedule idle %s tunnel %p to be destroyed", conn_type_name[conn_types_to_ind[(*tlit)->get_conn_type()]], *tlit);
            del_list.push_back(*tlit);
            idle_tunnels[i].erase(tlit);
            idle_tunnels_cnt--;
        }

        if (idle_tunnels[i].size() > 0) {
            // If we have at least one idle connection at this level,
            // we don't need to keep any idle connections of lower preferences.
            destroy_all = true;
        }
    }

    while (!del_list.empty()) {
        ts_tunnel *tt = del_list.front();
        del_list.pop_front();
        tt->Disconnect();
        delete tt;
    }

    if (recall_in_out) {
        *recall_in_out = recall_in;
    }
}

TSError_t ts_device_pool::tunnel_get(u64 user_id,
                                     int disabled_route_types,
                                     ts_tunnel*& ret_tunnel,
                                     string& error_msg)
{
    TSError_t err = TS_OK;
    ts_tunnel *tunnel = NULL;

    bool active_din_present = false;
    bool din_tried = false;

    {
        MutexAutoLock lock(&mutex);

        if (!vssiSession) {
            vssiSession = VSSI_RegisterSession(routeInfo.vssiSessionHandle, routeInfo.vssiServiceTicket.data());
            if (!vssiSession) {
                error_msg = "Failed to register VSSI session";
                err = TS_ERR_NO_MEM;
                goto found;
            }
        }

        // If there is an idle DIN, use that.
        if (!idle_tunnels[0].empty()) {
            tunnel = idle_tunnels[0].front();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Found idle DIN tunnel %p", tunnel);
            idle_tunnels[0].pop_front();
            idle_tunnels_cnt--;
            assigned_tunnels[0].push_back(tunnel);
            assigned_tunnels_cnt++;
            goto found;
        }

        active_din_present = !assigned_tunnels[0].empty();
    }

    // If there is an active DIN, try to open another DIN connection _in_the_background_.
    // Chance of getting this DIN in this call is slim, so this is more for the next call.
    if (active_din_present) {
        ts_tunnel *tt = new (std::nothrow) ts_tunnel(device_id, conn_types[0], vssiSession,
                                                     routeInfo.directInternalAddr, routeInfo.directPort,
                                                     this);
        if (!tt) {
            err = TS_ERR_NO_MEM;
            error_msg = "Not enough memory";
            goto found;
        }
        {
            // Increment this first to prevent race with the async connect finishing
            // and calling NotifyConnectionAttemptFinished() before we get to run again
            MutexAutoLock lock(&mutex);
            connecting_tunnels.insert(tt);
        }
        err = tt->ConnectAsync(error_msg);
        if ( err ) {  // failed to spawn a thread - need to clean-up ourselves
            // errmsg logged by ConnectAsync()
            NotifyConnectionAttemptFinished(tt);
            delete tt;
        }
        else {
            din_tried = true;
        }
    }

    // If we are trying to open a DIN connection in the background, yield and let others run.
    // We might get lucky and get the DIN connection.
    if (din_tried) {
        VPLThread_Yield();
    }

    // Use any idle connection.
    for (int i = 0; i < MAX_CONN_TYPE; i++) {
        MutexAutoLock lock(&mutex);
        if (!idle_tunnels[i].empty()) {
            tunnel = idle_tunnels[i].front();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Found idle %s tunnel %p", conn_type_name[i], tunnel);
            idle_tunnels[i].pop_front();
            idle_tunnels_cnt--;
            assigned_tunnels[i].push_back(tunnel);
            assigned_tunnels_cnt++;
            goto found;
        }
    }

    // Try to open connections in the background.
    for( int i = 0 ; i < MAX_CONN_TYPE ; i++ ) {
        // don't attempt route types that are mentioned in disabled_route_types
        if (((conn_types[i] == VSSI_SECURE_TUNNEL_DIRECT_INTERNAL) &&
             ((disabled_route_types & DISABLE_DIN) || din_tried)) ||
            ((conn_types[i] == VSSI_SECURE_TUNNEL_DIRECT) &&
             (disabled_route_types & DISABLE_DEX)) ||
            ((conn_types[i] == VSSI_SECURE_TUNNEL_PROXY_P2P) &&
             (disabled_route_types & DISABLE_P2P)) ||
            ((conn_types[i] == VSSI_SECURE_TUNNEL_PROXY) &&
             (disabled_route_types & DISABLE_PRX)))
            continue;

        ts_tunnel *tt = new (std::nothrow) ts_tunnel(device_id, conn_types[i], vssiSession,
                                                     i == 0 ? routeInfo.directInternalAddr :
                                                     i == 1 ? routeInfo.directExternalAddr :
                                                     routeInfo.proxyAddr,
                                                     i <= 1 ? routeInfo.directPort : routeInfo.proxyPort,
                                                     this);
        if (!tt) {
            err = TS_ERR_NO_MEM;
            error_msg = "Not enough memory";
            goto found;
        }
        {
            // Increment this first to prevent race with the async connect finishing
            // and calling NotifyConnectionAttemptFinished() before we get to run again
            MutexAutoLock lock(&mutex);
            connecting_tunnels.insert(tt);
        }
        err = tt->ConnectAsync(error_msg);
        if ( err ) {  // failed to spawn a thread - need to clean-up ourselves
            // errmsg logged by ConnectAsync()
            NotifyConnectionAttemptFinished(tt);
            delete tt;
        }
    }

    // Wait for the connection attempts running in the background.
    {
        MutexAutoLock lock(&mutex);
        VPLTime_t deadline = VPLTime_GetTimeStamp() + VPLTime_FromSec(30);  // match VSSI_PROXY_CONNECT_TIMEOUT
        while ((VPLTime_GetTimeStamp() < deadline) && (idle_tunnels_cnt == 0)) {
            VPLTime_t timeleft = VPLTime_DiffClamp(deadline, VPLTime_GetTimeStamp());
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Waiting for background connection attempt to complete: sleeping "FMTu64, timeleft);
            if (timeleft > 0) {
                int err = VPLCond_TimedWait(&have_idle_tunnels_cond, &mutex, timeleft);
                if (err && err != VPL_ERR_TIMEOUT) {
                    break;
                }
            }
        }

        if (idle_tunnels_cnt > 0) {
            for (int i = 0; i < MAX_CONN_TYPE; i++) {
                if (!idle_tunnels[i].empty()) {
                    tunnel = idle_tunnels[i].front();
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Found idle %s tunnel %p", conn_type_name[i], tunnel);
                    idle_tunnels[i].pop_front();
                    idle_tunnels_cnt--;
                    assigned_tunnels[i].push_back(tunnel);
                    assigned_tunnels_cnt++;
                    goto found;
                }
            }
        }
    }

    err = TS_ERR_NO_CONNECTION;
    error_msg = "Unable to reach desired device.";

found:
    ret_tunnel = tunnel;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, TS_FMT_TARG": type %d err %d:%s",
        user_id, device_id,
        tunnel ? tunnel->get_conn_type() : -1,
        err, error_msg.c_str());

    return err;
}

TSError_t ts_device_pool::tunnel_put(ts_tunnel* tunnel,
                                     bool is_assigned,
                                     string& error_msg)
{
    TSError_t err = TS_OK;

    int ci = conn_types_to_ind[tunnel->get_conn_type()];
    if ( ci == -1 ) {
        err = TS_ERR_INVALID;
        error_msg = "Unknown connection type";
        goto done;
    }

    {
        // Move ts_tunnel obj from assigned_tunnels[] to idle_tunnels[] atomically.
        MutexAutoLock lock(&mutex);

        if ( is_assigned ) {
            // ts_tunnel obj should be in assigned_tunnels[] - remove.

            TSTunnelList_t::iterator it;
            for( it = assigned_tunnels[ci].begin() ; it != assigned_tunnels[ci].end() ; it++ ) {
                if ( *it == tunnel ) {
                    break;
                }
            }
            if (it == assigned_tunnels[ci].end()) {  // not found
                err = TS_ERR_INVALID;
                error_msg = "Tunnel not found on assigned list";
                goto done;
            }

            assigned_tunnels[ci].erase(it);
            assigned_tunnels_cnt--;
        }

        if (!tunnel->get_is_failed()) {
            tunnel->set_last_active(VPLTime_GetTimeStamp());
            idle_tunnels[ci].push_back(tunnel);
            idle_tunnels_cnt++;
            VPLCond_Signal(&have_idle_tunnels_cond);
            VPLCond_Signal(&scrubber_cond);
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Idle %s tunnel %p", conn_type_name[ci], tunnel);
        }
        else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Destroying %s tunnel %p", conn_type_name[ci], tunnel);
            tunnel->Disconnect();
            delete tunnel;
        }
    }

    tunnel = NULL;

done:
    return err;
}

void ts_device_pool::NotifyConnectionAttemptFinished(ts_tunnel *tunnel)
{
    MutexAutoLock lock(&mutex);
    connecting_tunnels.erase(tunnel);
    if (connecting_tunnels.empty()) {
        VPLCond_Broadcast(&no_connecting_tunnels_cond);
    }
}

VPLTHREAD_FN_DECL ts_device_pool::scrubberMain(void *param)
{
    ts_device_pool* devpool = (ts_device_pool*)param;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_device_pool[%p]: Scrubber starting.", param);

    devpool->scrubberMain();

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_device_pool[%p]: Scrubber stopping.", param);

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

void ts_device_pool::scrubberMain(void)
{
    MutexAutoLock lock(&mutex);
    do {
        VPLTime_t delay;
        VPLTime_t cur_timestamp;

        delay = VPL_TIMEOUT_NONE;
        cur_timestamp = VPLTime_GetTimeStamp();

        VPLTime_t recall_in;
        scrubIdleConnections(cur_timestamp, &recall_in);
        delay = recall_in;

        // Delay until something of interest happens
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Sleeping for "FMTu64, delay);
        VPLCond_TimedWait(&scrubber_cond, &mutex, delay);
    } while (!shutdown_stop);
}

TSError_t ts_device_pool::StartScrubber(string& error_msg)
{
    TSError_t tserr = TS_OK;

    int err = Util_SpawnThread(scrubberMain, VPL_AS_THREAD_FUNC_ARG(this), Scrubber_StackSize, /*isJoinable*/VPL_TRUE, &scrubber_thread);
    if (err) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to start pool scrubber thread: err %d", err);
        error_msg = "Failed to start pool scrubber thread";
        tserr = TS_ERR_NO_MEM;
        goto done;
    }

    {
        MutexAutoLock lock(&mutex);
        scrubber_running = true;
    }

done:
    return tserr;
}

void ts_device_pool::StopScrubber()
{
    // Wake up the timeout thread and tell it to shut down.
    if (scrubber_running) {
        int err;

        // Signal the timeout thread to stop
        VPLMutex_Lock(&mutex);
        shutdown_stop = true;
        VPLCond_Signal(&scrubber_cond);
        VPLMutex_Unlock(&mutex);

        err = VPLDetachableThread_Join(&scrubber_thread);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "scrubber_thread join: %d", err);
        scrubber_running = false;
        shutdown_stop = false;
    }
}

void ts_device_pool::EndVssiSession()
{
    MutexAutoLock lock(&mutex);
    VSSI_EndSession(vssiSession);
    vssiSession = NULL;
}


TSError_t ts_device_pool::WaitConnectingTunnels()
{
    TSError_t tserr = TS_OK;

    MutexAutoLock lock(&mutex);

    // This may take up to 30 seconds (VSSI proxy connection timeout).
    while (!connecting_tunnels.empty()) {
        int err = VPLCond_TimedWait(&no_connecting_tunnels_cond, &mutex, VPL_TIMEOUT_NONE);
        if (err) {
            tserr = TS_ERR_NO_MEM;
            break;
        }
    }
    if (!connecting_tunnels.empty()) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "ts_device_pool[%p]: Failed to wait for num of connecting tunnels to go 0", this);
    }

    return tserr;
}

TSError_t ts_device_pool::DisconnectTunnels()
{
    TSError_t err = TS_OK;

    MutexAutoLock lock(&mutex);

    failConnectingTunnels();
    disconnectAssignedTunnels();
    disconnectIdleTunnels();

    return err;
}

TSError_t ts_device_pool::PurgeTunnels(string& error_msg)
{
    TSError_t err = TS_OK;

    MutexAutoLock lock(&mutex);

    if (!connecting_tunnels.empty()) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "ts_device_pool[%p] has "FMTu_size_t" connecting tunnels", this, connecting_tunnels.size());
    }

    purgeAssignedTunnels();
    purgeIdleTunnels();

    return err;
}

void ts_device_pool::failConnectingTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    std::set<ts_tunnel*>::iterator it;
    for (it = connecting_tunnels.begin(); it != connecting_tunnels.end(); it++) {
        (*it)->Disconnect();  // This has the side-effect of marking the tunnel "failed".
    }
}

void ts_device_pool::disconnectAssignedTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    for (int i = 0; i < MAX_CONN_TYPE; i++) {
        std::list<ts_tunnel*>::iterator it;
        for (it = assigned_tunnels[i].begin(); it != assigned_tunnels[i].end(); it++) {
            (*it)->Disconnect();
            // ts_tunnel obj will be destroyed when TS_Close() is called.
        }
    }
}

void ts_device_pool::disconnectIdleTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    for (int i = 0; i < MAX_CONN_TYPE; i++) {
        while (!idle_tunnels[i].empty()) {
            ts_tunnel *tunnel = idle_tunnels[i].front();
            idle_tunnels[i].pop_front();
            tunnel->Disconnect();
            delete tunnel;
            idle_tunnels_cnt--;
        }
    }
}

void ts_device_pool::purgeAssignedTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    for (int i = 0; i < MAX_CONN_TYPE; i++) {
        while (!assigned_tunnels[i].empty()) {
            ts_tunnel *tunnel = assigned_tunnels[i].front();
            assigned_tunnels[i].pop_front();
            tunnel->Disconnect();
            delete tunnel;
            assigned_tunnels_cnt--;
        }
    }

    assert(assigned_tunnels_cnt == 0);
}

void ts_device_pool::purgeIdleTunnels()
{
    // Caller is responsible for taking the lock.
    assert(VPLMutex_LockedSelf(&mutex));

    for (int i = 0; i < MAX_CONN_TYPE; i++) {
        while (!idle_tunnels[i].empty()) {
            ts_tunnel *tunnel = idle_tunnels[i].front();
            idle_tunnels[i].pop_front();
            tunnel->Disconnect();
            delete tunnel;
            idle_tunnels_cnt--;
        }
    }

    assert(idle_tunnels_cnt == 0);
}

}  // namespace TS_WRAPPER
