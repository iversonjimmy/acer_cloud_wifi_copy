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
/// ts_pool.hpp
///
/// Tunnel Service Connection Pool

#ifndef __TS_POOL_HPP__
#define __TS_POOL_HPP__

#ifndef IS_TS_WRAPPER
#define IS_TS_WRAPPER
#endif

#include "vplu_types.h"
#include "vpl_time.h"
#include "ts_client.hpp"
#include "vssi.h"
#include <string>
#include <list>
#include <map>
#include <set>

using namespace std;

#define TS_FMT_TARG     "<"FMTu64","FMTu64">"

namespace TS_WRAPPER
{

class ts_tunnel;
class ts_device_pool;

/*
  Tunnel that is backed by a VSSI Secure Tunnel.
  It is visible to the application as TSIOHandle_t.
 */
class ts_tunnel {
public:
    ts_tunnel(u64 device_id, int conn_type, VSSI_Session vssiSession,
              const std::string &serverHostname, u16 serverPort,
              ts_device_pool* devpool);
    ~ts_tunnel();

    // Disconnect underlying VSSI secure tunnel.
    void Disconnect();

    TSError_t reset(string& error_msg);
    TSError_t read(char* buffer, size_t& buffer_len, string& error_msg);
    TSError_t write(const char* buffer, size_t buffer_len, string& error_msg);

    int get_conn_type(void) {return conn_type;};
    u64 get_device_id(void) {return device_id;};
    bool get_is_failed(void) {return is_failed;};
    VPLTime_t get_last_active(void) {return last_active;};
    void set_last_active(VPLTime_t cur_time) {last_active = cur_time;};
    ts_device_pool* get_devpool(void) {return devpool;};

    // Connect to the destination via VSSI Secure Tunnel.
    TSError_t Connect(std::string &error_msg);
    TSError_t ConnectAsync(std::string &error_msg);

private:
    u64                 device_id;
    int                 conn_type;  // contains one of VSSI_SECURE_TUNNEL_* values
    bool                has_tunnel; // True iff tunnel is available for use.
                                    // False doesn't necessary mean there is no tunnel;
                                    // it could mean the tunnel is present but no longer available for use
                                    // as it is in the process of getting disconnected.
    bool                is_failed;

    VSSI_Session vssiSession;
    ts_device_pool*     devpool;  // parent

    VPLMutex_t          mutex;

    VPLSem_t            write_sem;  // used to make VSSI calls essentially blocking calls
    VPLSem_t            read_sem;   // used to make VSSI calls essentially blocking calls

    VPLTime_t           last_active;
    std::string         serverHostname;
    u16                 serverPort;

    VSSI_SecureTunnel   tunnel_handle;

    int numThreadsInVssi;  // num of threads in VSSI_*() after tunnel is established
    void adjustNumThreadsInVssi(bool isEntering);  // helper function to adjust numThreadsInVssi
    VPLCond_t noThreadsInVssi_cond;

    static VPLTHREAD_FN_DECL connectorMain(void *param);

    void disconnect();  // this version does not take the mutex
};

#define MAX_CONN_TYPE       4

typedef list<ts_tunnel*> TSTunnelList_t;

/*
  Holds all tunnels to a specific destination.
 */
class ts_device_pool {
public:
    ts_device_pool(u64 device_id, const TsRouteInfo &routeInfo, VPLTime_t timeout[4]);

    // Caller must guarantee that no thread is inside TsBackEnd at the time of call,
    // and that no thread will enter TsBackEnd during the call.
    ~ts_device_pool();

    void UpdateRouteInfo(const TsRouteInfo &routeInfo);

    TSError_t StartScrubber(string& error_msg);
    void StopScrubber(void);

    TSError_t tunnel_get(u64 user_id,
                         int disabled_route_types,
                         ts_tunnel*& ret_tunnel,
                         string& error_msg);

    TSError_t tunnel_put(ts_tunnel* tunnel,
                         bool is_assigned,
                         string& error_msg);

    TSError_t WaitConnectingTunnels();

    TSError_t DisconnectTunnels();

    // Caller must guarantee that no thread is inside TsBackEnd at the time of call,
    // and that no thread will enter TsBackEnd during the call.
    TSError_t PurgeTunnels(string& error_msg);

    void NotifyConnectionAttemptFinished(ts_tunnel *tunnel);

    // Precondition: no connections using this VSSI session.
    void EndVssiSession();

private:
    VPLMutex_t mutex;

    u64         device_id;
    TsRouteInfo routeInfo;
    VPLTime_t   conn_type_timeouts[MAX_CONN_TYPE];

    std::set<ts_tunnel*> connecting_tunnels;
    TSTunnelList_t       idle_tunnels[MAX_CONN_TYPE];     // must not contain any "failed" tunnels
    TSTunnelList_t       assigned_tunnels[MAX_CONN_TYPE]; // may contain "failed" tunnels (waiting to be closed)
    int                  idle_tunnels_cnt;                // total num of tunnels in idle_tunnels[]
    int                  assigned_tunnels_cnt;            // total num of tunnels in assigned_tunnels[]
    VPLCond_t            have_idle_tunnels_cond;          // signaled when idle tunnel available
    VPLCond_t            no_connecting_tunnels_cond;      // signaled when no connecting tunnels

    VPLDetachableThreadHandle_t scrubber_thread;
    bool                        scrubber_running;
    bool                        shutdown_stop;
    VPLCond_t                   scrubber_cond;
    void scrubberMain(void);
    static VPLTHREAD_FN_DECL scrubberMain(void *param);
    void scrubIdleConnections(VPLTime_t timestamp, VPLTime_t *recall_in_out = NULL);

    VSSI_Session vssiSession;

    void failConnectingTunnels();
    void disconnectAssignedTunnels();
    void disconnectIdleTunnels();
    void purgeAssignedTunnels();
    void purgeIdleTunnels();
};

// Values for disabled_route_types bit mask
enum {
    // these values must agree with CONFIG_CLEARFI_MODE_DISABLE_* in gvm_core/daemons/ccd/src_impl/config.h
    DISABLE_DIN = 1,
    DISABLE_DEX = 2,
    DISABLE_P2P = 4,
    DISABLE_PRX = 8,
};

}  // namespace TS_WRAPPER guard
#endif // include guard
