/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

/// Virtual Storage (Save Data) Service (VSS)

#include "vss_server.hpp"
#include "gvm_rm_utils.hpp"

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "gvm_thread_utils.h"
#include "scopeguard.hpp"
#include "vplex_trace.h"
#include "vpl_lazy_init.h"
#include "vpl_fs.h"
#include "vpl_net.h"
#include "vpl_socket.h"
#include "vplex_powerman.h"

#include "vss_cmdproc.hpp"
#include "vss_query.hpp"
#include "managed_dataset.hpp"
#include "fs_dataset.hpp"
#include "vplex_socket.h"
#include "vss_comm.h"

#include "sn_features.h"

#define IS_TS
#include "ts_server.hpp"
#undef IS_TS
#include "vssts_srvr.hpp"
#include "ts_ext_server.hpp"
#include "HttpSvc_TsToHsAdapter.hpp"
#include "HttpSvc_Sn_Handler.hpp"
#include "HttpSvc_Sn_Handler_rf.hpp"
#include "echoSvc.hpp"

static TSServiceHandle_t echo_svc_handle;
static VPLLazyInitMutex_t http_handler_stop_mutex = VPLLAZYINITMUTEX_INIT;
static VPLCond_t http_handler_stop_cond;
static u32 http_handler_ref_cnt = 0;

#ifdef ENABLE_PHOTO_TRANSCODE
#include "image_transcode.h"
#endif

#include "rf_search_manager.hpp"

#ifndef CONFIG_ENABLE_TS_INIT_TS_IN_SN
#define CONFIG_ENABLE_TS_INIT_TS_IN_SN 4
#endif

using namespace std;

static TSServiceHandle_t  ts_http_handle;

static void http_handler(TSServiceRequest_t& request)
{
    HttpSvc::TsToHsAdapter *adapter = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Request from <"FMTu64","FMTu64"> started",
        request.client_user_id, request.client_device_id);

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&http_handler_stop_mutex));
    http_handler_ref_cnt++;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&http_handler_stop_mutex));
    
    adapter = new (std::nothrow) HttpSvc::TsToHsAdapter(&request);
    if (!adapter) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Insufficient memory");
        goto done;
    }

    adapter->Run();

 done:
    if (adapter)
        delete adapter;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Request from <"FMTu64","FMTu64"> done.",
        request.client_user_id, request.client_device_id);

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&http_handler_stop_mutex));
    http_handler_ref_cnt--;
    if (http_handler_ref_cnt == 0) {
        // Notify all http handlers are done
        VPLCond_Signal(&http_handler_stop_cond);
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&http_handler_stop_mutex));
}

vss_server::vss_server() :
    enableTs(0),
    servicePortRangeBegin(0),
    servicePortRangeEnd(0),
    clientSockInit(false),
    secureClearfiSockInit(false),
    curAddr(VPLNET_ADDR_INVALID),
    reportedAddr(VPLNET_ADDR_INVALID),
    lastReportAddrTime(VPLTIME_INVALID),
    msaGetObjectMetadataCb(NULL),
    nextPostponeSleepTime(VPLTIME_INVALID),
    sleepDeferred(false),
    mainStarted(false),
    self_session(NULL),
    stat_updates_in_progress(false),
    stat_disconnect_called(false),
    nextDatasetStatUpdateTime(VPLTIME_INVALID)
{    
#ifdef VPL_PIPE_AS_SOCKET_OK
    pipefds[0] = pipefds[1] = -1;
#endif
#ifdef PERF_STATS
    memset(&stats[0], 0, sizeof(stats));
#endif // PERF_STATS

    VPLMutex_Init(&mutex);
    VPLMutex_Init(&task_mutex);
    VPLCond_Init(&task_condvar);
    VPLMutex_Init(&stats_mutex);
    VPLCond_Init(&stats_condvar);
    VPLMutex_Init(&HttpSvc::Sn::Handler_rf_Helper::AccessControlLocker::mutex);

}

vss_server::~vss_server()
{
    VPLMutex_Lock(&mutex);

    // Stop all workers.
    // In reality, the storage node is always stopped first, which would have completed
    // the thread clean-up already, so the following if statement should never be true
    if(running) {
        running = false;
        VPLCond_Broadcast(&task_condvar);
        for(u32 i = 0; i < num_workers ; i++) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Joining worker %d/%d.",
                      i+1, num_workers);
            VPLThread_Join(&(workers[i].thread), NULL);
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Joined with worker %d/%d on shutdown.",
                              i+1, num_workers);
        }
        delete[] workers;

        VPLCond_Broadcast(&stats_condvar);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Joining stats thread.");
        VPLThread_Join(&(stats_thread), NULL);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Joined with stats thread on shutdown.");
    }

    if(clientSockInit) {
        VPLSocket_Close(clientSocket);
    }

    if(secureClearfiSockInit) {
        VPLSocket_Close(secureClearfiSocket);
    }

    while(!clients.empty()) {
        map<int, vss_client*>::iterator it = 
            clients.begin();
        delete it->second;
        clients.erase(it);
    }

    while(!clearfi_clients.empty()) {
        map<int,strm_http*>::iterator it =
            clearfi_clients.begin();
        delete it->second;
        clearfi_clients.erase(it);
    }

    while(!p2p_clients.empty()) {
        map<int, vss_p2p_client*>::iterator it = 
            p2p_clients.begin();
        vss_p2p_client* client = it->second;
        
        p2p_clients.erase(it);
        
        if((VPLSocket_Equal(client->punch_sockfd, VPLSOCKET_INVALID) ||
            p2p_clients.find(VPLSocket_AsFd(client->punch_sockfd)) == p2p_clients.end()) &&
           (VPLSocket_Equal(client->listen_sockfd, VPLSOCKET_INVALID) ||
            p2p_clients.find(VPLSocket_AsFd(client->listen_sockfd)) == p2p_clients.end()) &&
           (VPLSocket_Equal(client->sockfd, VPLSOCKET_INVALID) ||
            p2p_clients.find(VPLSocket_AsFd(client->sockfd)) == p2p_clients.end())) {
            // All references to this vss_p2p_client are gone.
            delete client;
        }
    }

    if (enableTs & CONFIG_ENABLE_TS_INIT_TS_IN_SN) {
        TSError_t err = TS_OK;
        string error_msg;
        
        err = TS::TS_DeregisterService(ts_http_handle, error_msg);
        if ( err != TS_OK ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "TS::TS_DeregisterService() - %d:%s", err, error_msg.c_str());
        }

        err = TS::TS_DeregisterService(echo_svc_handle, error_msg);
        if ( err != TS_OK ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "TS::TS_DeregisterService() - %d:%s", err, error_msg.c_str());
        }

#if defined(CLOUDNODE)
        vssts_srvr_stop();
#endif

        // Force shutdown all the tunnels so all the http handlers returns immediately
        TS::TS_ServerShutdown();

        // Wait for all handlers to be done
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&http_handler_stop_mutex));
        while (http_handler_ref_cnt > 0) {
            VPLCond_TimedWait(&http_handler_stop_cond, VPLLazyInitMutex_GetMutex(&http_handler_stop_mutex), VPL_TIMEOUT_NONE);
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&http_handler_stop_mutex));

        // Destroy http_handler_stop_cond no longer needed
        if (VPL_IS_INITIALIZED(&http_handler_stop_cond)) {
            VPLCond_Destroy(&http_handler_stop_cond);
        }

        HttpSvc::Sn::Handler::SetVssServer(NULL);
    }

    if(self_session) {
        delete self_session;
        self_session = NULL;
    }

    while(!datasets.empty()) {
        map<dataset_id, dataset*>::iterator it = 
            datasets.begin();
        delete it->second;
        datasets.erase(it);
    }

    VPLMutex_Unlock(&mutex);

#ifdef VPL_PIPE_AS_SOCKET_OK
    if(pipefds[0] != -1) close(pipefds[0]);
    if(pipefds[1] != -1) close(pipefds[1]);
#endif

    VPLMutex_Destroy(&mutex);
    VPLMutex_Destroy(&stats_mutex);
    VPLCond_Destroy(&stats_condvar);
    VPLMutex_Destroy(&task_mutex);
    VPLCond_Destroy(&task_condvar);
    VPLMutex_Destroy(&HttpSvc::Sn::Handler_rf_Helper::AccessControlLocker::mutex);

#if ENABLE_REMOTE_FILE_SEARCH
    if (HttpSvc::Sn::Handler_rf_Helper::rfSearchManager != NULL) {
        DestroyRemoteFileSearchManager(HttpSvc::Sn::Handler_rf_Helper::rfSearchManager);
        HttpSvc::Sn::Handler_rf_Helper::rfSearchManager = NULL;
    }
#endif // ENABLE_REMOTE_FILE_SEARCH
}

void vss_server::set_server_config(const vplex::vsDirectory::SessionInfo& session,
                                   const std::string& sessionSecret,
                                   u64 userId,
                                   u64 clusterId,
                                   const char* rootDir,
                                   const char* serverServicePortRange,
                                   const char* infraDomain,
                                   const char* vsdsHostname,
                                   const char* tagEditProgramPath,
                                   const char* remotefileTempFolder,
                                   u16 vsdsPort,
                                   msaGetObjectMetadataFunc_t msaGetObjectMetadataCb,
                                   void* msaCallbackContext,
                                   int enableTs,
                                   Ts2::LocalInfo *localInfo)
{
    this->clusterId = clusterId;

    if (serverServicePortRange) {
        char *p = strchr(const_cast<char*>(serverServicePortRange), '-');
        if (p) {
            servicePortRangeBegin = atoi(serverServicePortRange);
            servicePortRangeEnd = atoi(p + 1);
            if ((servicePortRangeBegin < 1) || (servicePortRangeBegin > 65535) ||
                (servicePortRangeEnd < 1) || (servicePortRangeEnd > 65535) ||
                (servicePortRangeBegin > servicePortRangeEnd)) {
                servicePortRangeBegin = servicePortRangeEnd = 0;  // indicating invalid, so don't try
            }
        }
    }

    this->rootDir.assign(rootDir);
    // Add end slash if not present.
    if(this->rootDir[this->rootDir.size() - 1] != '/') {
        this->rootDir.append('/', 1);
    }
    
    this->msaGetObjectMetadataCb = msaGetObjectMetadataCb;
    this->msaCallbackContext = msaCallbackContext;

    this->infraDomain = infraDomain;

    this->tagEditProgramPath = tagEditProgramPath;

    this->remotefileTempFolder = remotefileTempFolder;

    this->sessionHandle = session.sessionhandle();
    this->sessionSecret = sessionSecret;

    this->enableTs = enableTs;
    this->localInfo = localInfo;

    this->userId = userId;

    // Initialize for query mechanism.
    query.init(vsdsHostname, vsdsPort, clusterId, userId, session);
    query.setSessionSecret(userId, sessionSecret);

    // Place own login session in vss_session table.
    self_session = new vss_session(session.sessionhandle(), userId,
                                                session.serviceticket(), 
                                                query);

    if (enableTs & CONFIG_ENABLE_TS_INIT_TS_IN_SN) {
        HttpSvc::Sn::Handler::SetVssServer(this);

        {
            // init TS and TS ext servers 
            string error_msg;
            u32 instanceId = localInfo->GetInstanceId();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                "TS_ServerInit for UDI <"FMTu64","FMTu64","FMTu32">", userId, clusterId, instanceId);
            TSError_t err = TS::TS_ServerInit(userId, clusterId, instanceId, localInfo, error_msg);
            if (err != TS_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "TS::TS_ServerInit() - %d:%s", err, error_msg.c_str());
            }

            TSServiceParms_t parms;
            parms.service_names.push_front("cmd");
            parms.service_names.push_front("mediafile");
            parms.service_names.push_front("media_rf");
            parms.service_names.push_front("minidms");
            parms.service_names.push_front("mm");
            parms.service_names.push_front("rexe");
            parms.service_names.push_front("rf");
            parms.service_names.push_front("tstest");
            parms.service_names.push_front("vcs_archive");
            parms.protocol_name = "http";
            parms.service_handler = http_handler;
            err = TS::TS_RegisterService(parms, ts_http_handle, error_msg);
            if (err != TS_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "TS::TS_RegisterService() - %d:%s", err, error_msg.c_str());
            }

#if defined(CLOUDNODE)
            err = vssts_srvr_start(this, self_session, error_msg);
            if ( err != TS_OK ) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                                 "vssts_srvr_start() - %d:%s", err, error_msg.c_str());
            }
#endif

            {
                TSServiceParms_t parms;
                parms.service_names.push_front("echo");
                parms.protocol_name = "echo";
                parms.service_handler = EchoSvc::handler;
                err = TS::TS_RegisterService(parms, echo_svc_handle, error_msg);
                if (err != TS_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "TS::TS_RegisterService() - %d:%s", err, error_msg.c_str());
                }
            }
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Server config:\n"
                        "\t\t\tService port range: %d-%d\n"
                        "\t\t\tCluster: %"PRIx64"\n"
                        "\t\t\tRoot: \"%s\"\n"
                        "\t\t\tinfraDomain: %s\n"
                        "\t\t\tVSDS server: %s:%d",
                        servicePortRangeBegin, servicePortRangeEnd,
                        clusterId, this->rootDir.c_str(),
                        infraDomain, vsdsHostname, vsdsPort);

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Self-Session handle: "FMTx64".",
                        session.sessionhandle());

}

const std::string& vss_server::getStorageRoot() const
{
    return rootDir;
}

// Open a socket for the server to listen on. Provide the port to bind to.
static int open_server_socket(int port, VPLSocket_t& sockfd)
{
    int rv = -1;
    VPLSocket_addr_t sin;
    VPLSocket_addr_t sin_tmp;
    int yes = 1;

    sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create socket. error:"FMT_VPLSocket_t,
                         VAL_VPLSocket_t(sockfd));
        goto exit;
    }

    rv = VPLSocket_SetSockOpt(sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to set TCP_NODELAY on socket. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLSocket_t(sockfd), rv);
        goto error;
    }

    sin.family = VPL_PF_INET;
    sin.port = VPLConv_hton_u16(port);
    sin.addr = VPLNET_ADDR_ANY;
    memcpy(&sin_tmp, &sin, sizeof(sin));

    rv = VPLSocket_Bind(sockfd, &sin_tmp, sizeof(sin));
    if (port != 0 && rv == VPL_ERR_ADDRINUSE) {
        // Reason: specific port was tried but it was already in use.
        goto error;
    }
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to bind socket. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLSocket_t(sockfd), rv);
        goto error;
    }

    rv = VPLSocket_Listen(sockfd, 10);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to listen socket. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLSocket_t(sockfd), rv);
        goto error;
    }

    rv = 0;

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Opened port %d for all addresses.",
                        VPLNet_port_ntoh(VPLSocket_GetPort(sockfd)));
    goto exit;
 error:
    VPLSocket_Close(sockfd);
 exit:
    return rv;
}

// Open a client socket to another server.
static void open_client_socket(const std::string& address,
                               u16 port, bool reuse_addr,
                               VPLSocket_t& sockfd)
{
    int yes = 1;
    int rc;
    VPLSocket_addr_t inaddr;
    VPLNet_addr_t origin_addr;
    VPLNet_port_t origin_port;

    // Determine address for connection
    inaddr.family = VPL_PF_INET;
    inaddr.addr = VPLNet_GetAddr(address.c_str());
    inaddr.port = VPLNet_port_hton(port);
    
    sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
    if(VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open socket to server "FMT_VPLNet_addr_t":%d.",
                         VAL_VPLNet_addr_t(inaddr.addr),
                         VPLNet_port_ntoh(inaddr.port));
        goto exit;
    }
    
    if(reuse_addr) {
        // Must use SO_REUSEADDR option. 
        rc = VPLSocket_SetSockOpt(sockfd, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                                  (void*)&yes, sizeof(yes));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Failed (%d) to set SO_REUSEADDR for socket.",
                              rc);
            goto fail;
        }
    }

    /* Set TCP no delay for performance reasons. */
    VPLSocket_SetSockOpt(sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                         (void*)&yes, sizeof(yes));
    
    if((rc = VPLSocket_Connect(sockfd, &inaddr,
                               sizeof(VPLSocket_addr_t))) != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed (%d) to connect to server "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                          rc,
                          VAL_VPLNet_addr_t(inaddr.addr),
                          VPLNet_port_ntoh(inaddr.port),
                          VAL_VPLSocket_t(sockfd));
        goto fail;
    }

    origin_addr = VPLSocket_GetAddr(sockfd);
    origin_port = VPLNet_port_ntoh(VPLSocket_GetPort(sockfd));
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Opened socket "FMT_VPLSocket_t" with address "FMT_VPLNet_addr_t":%u to server "FMT_VPLNet_addr_t":%u.",
                      VAL_VPLSocket_t(sockfd),
                      VAL_VPLNet_addr_t(origin_addr),
                      origin_port,
                      VAL_VPLNet_addr_t(inaddr.addr),
                      VPLNet_port_ntoh(inaddr.port));
    goto exit;
 fail:
    VPLSocket_Close(sockfd);
    sockfd = VPLSOCKET_INVALID;
 exit:
    return;
}

static void append_time(stringstream& stream, VPLTime_t time)
{
    if(VPLTime_ToSec(time) > 0) {
        stream << VPLTime_ToSec(time) << " s";
    }
    else if(VPLTime_ToMillisec(time) > 0) {
        stream << VPLTime_ToMillisec(time) << "ms";
    }
    else {
        stream << time << "us";
    }        
}

void vss_server::getServerStats(std::string& data_out)
{
    stringstream stats;
    VPLTime_t delta;

    stats << " Cluster " << clusterId << endl;

    stats << "Clients:" << clients.size() << endl;

    // Report the status of the worker threads.
    stats << "Worker threads active:" << workers_active << "/" << num_workers << endl;
    stats << "    Execute time min/avg/max/last:";
    append_time(stats, worker_active_min == VPLTIME_INVALID ? 0 : worker_active_min);
    stats << "/";    
    delta = tasks_finished ? worker_active_total / tasks_finished : 0;
    append_time(stats, delta);
    stats << "/";
    append_time(stats, worker_active_max);
    stats << "/";
    append_time(stats, worker_active_last);
    stats << " Processed:" << tasks_finished << endl;
    stats << "    Pending:" << taskQ.size() << " peak:" << task_waiting_max_count
          << " Wait min/avg/max/last:";
    append_time(stats, task_waiting_min == VPLTIME_INVALID ? 0 : task_waiting_min);
    stats << "/";    
    delta = tasks_finished ? task_waiting_total / tasks_finished : 0;
    append_time(stats, delta);
    stats << "/";
    append_time(stats, task_waiting_max);
    stats << "/";
    append_time(stats, task_waiting_last);
    stats << endl;

    data_out = stats.str();
}

#ifndef VPL_PIPE_AS_SOCKET_OK
/// Local socket that can be used to wake the net handler thread (client side).
static VPLSocket_t priv_socket_client;
/// Local socket that the net handler thread listens on (to allow other threads to wake it).
static VPLSocket_t priv_socket_server_listen;
static VPLSocket_t priv_socket_server_connected;
static int create_private_sockets(VPLSocket_t& priv_socket_server_listen, VPLSocket_t& priv_socket_client, VPLSocket_t& priv_socket_server_connected)
{
    int rc = VPL_OK;
    VPLSocket_addr_t privSockAddr;
    VPLNet_port_t privSockPort;
    int connect_retry_count;
    int accept_retry_count;
    int yes;

    priv_socket_server_listen = VPLSocket_CreateTcp(VPLNET_ADDR_LOOPBACK, VPLNET_PORT_ANY);
    if (VPLSocket_Equal(priv_socket_server_listen, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_CreateTcp (receiving-side) failed.");
        rc = VPL_ERR_SOCKET;
        goto fail_server_sock_create;
    }
    rc = VPLSocket_Listen(priv_socket_server_listen, 100);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Listen failed: %d.", rc);
        goto fail_server_sock_listen;
    }
    privSockPort = VPLSocket_GetPort(priv_socket_server_listen);
    if (privSockPort == VPLNET_PORT_INVALID) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_GetPort failed.");
        rc = VPL_ERR_SOCKET;
        goto fail_server_sock_get_port;
    }
    priv_socket_client = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);
    if (VPLSocket_Equal(priv_socket_client, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_CreateTcp (sending-side) failed.");
        rc = VPL_ERR_SOCKET;
        goto fail_client_sock_create;
    }
    yes = 1;
    rc = VPLSocket_SetSockOpt(priv_socket_client, VPLSOCKET_IPPROTO_TCP,
                              VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_SetSockOpt(TCP_NODELAY) failed: %d.", rc);
        goto fail_client_sock_set_nodelay;
    }
    privSockAddr.family = VPL_PF_INET;
    privSockAddr.addr = VPLNET_ADDR_LOOPBACK;
    privSockAddr.port = privSockPort;
    connect_retry_count = 0;
 retry_connect:
    rc = VPLSocket_Connect(priv_socket_client, &privSockAddr, sizeof(privSockAddr));
    if (rc != VPL_OK) {
        if (connect_retry_count < 16) {
            VPLThread_Sleep(VPLTime_FromMillisec(500));
            connect_retry_count++;
            goto retry_connect;
        }
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Connect failed: %d.", rc);
        goto fail_client_sock_connect;
    }
    accept_retry_count = 0;
 retry_accept:
    rc = VPLSocket_Accept(priv_socket_server_listen, NULL, 0, &priv_socket_server_connected);
    if (rc == VPL_ERR_AGAIN) {
        if (accept_retry_count < 200) {
            VPLThread_Sleep(VPLTime_FromMillisec(50));
            accept_retry_count++;
            goto retry_accept;
        }
    }
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Accept failed: %d.", rc);
        goto fail_server_sock_accept;
    }
    goto out;
 fail_server_sock_accept:
 fail_client_sock_connect:
 fail_client_sock_set_nodelay:
    VPLSocket_Close(priv_socket_client);
 fail_client_sock_create:
 fail_server_sock_get_port:
 fail_server_sock_listen:
    VPLSocket_Close(priv_socket_server_listen);
 fail_server_sock_create:
 out:
    return rc;
}
#endif

class service_port_opener {
public:
    service_port_opener(int port_begin, int port_end)
        : port_begin(port_begin), port_end(port_end), port_next(port_begin) {}
    int open(VPLSocket_t &sock) {
        while (port_next <= port_end) {
            int err = open_server_socket(port_next++, sock);
            if (err == VPL_OK) {
                return err;
            }
        }
        return open_server_socket(0, sock);  // "0" meaning any port
    }
private:
    const int port_begin;
    const int port_end;
    int port_next;  // next port number to try
};

static void destroySpo(service_port_opener *spo)
{
    delete spo;
}

int vss_server::start(u32 worker_threads)
{
    int rc;

    VPL_SET_UNINITIALIZED(&http_handler_stop_cond);
    if (VPLCond_Init(&http_handler_stop_cond) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unable to initialize cond for stopping http handlers");
        rc = VPL_ERR_NOMEM;
        goto fail_thread;
    }
    http_handler_ref_cnt = 0;

    VPLMutex_Lock(&mutex);

#ifdef ENABLE_PHOTO_TRANSCODE
    rc = ImageTranscode_Init();
    if (rc != 0) {
        if (rc == IMAGE_TRANSCODING_ALREADY_INITIALIZED) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Double init ImageTranscode, ignore.");
        } else {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Failed to init ImageTranscode, rc = %d", rc);
        }
    }
#endif

#if ENABLE_REMOTE_FILE_SEARCH
    {
        const u64 maxGeneratedResults = 2000;
        RemoteFileSearchManager* remoteFileSearchManager =
                CreateRemoteFileSearchManager(maxGeneratedResults,
                                              VPLTime_FromMinutes(15),
                                              /*OUT*/ rc);
        if (remoteFileSearchManager == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "CreateRemoteFileSearchManager:%d", rc);
        } else {
            if (HttpSvc::Sn::Handler_rf_Helper::rfSearchManager != NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rfSearchManager previously defined");
            }
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Initialized RemoteFileSearchManager");
            HttpSvc::Sn::Handler_rf_Helper::rfSearchManager = remoteFileSearchManager;
        }
    }
#endif // ENABLE_REMOTE_FILE_SEARCH

    {
        // Remove all contents of tmp dir and re-create.
        string tmp_dir = rootDir + ".tmp/";
        string tmp_delete_dir = rootDir+".to_delete/";
        (void)Util_rmRecursive(tmp_dir, tmp_delete_dir);
        VPLDir_Create(tmp_dir.c_str(), 0777);
    }
    {
        service_port_opener *spo = new (std::nothrow) service_port_opener(servicePortRangeBegin, servicePortRangeEnd);
        if (spo == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Not enough memory");
            return VPL_ERR_NOMEM;
        }
        ON_BLOCK_EXIT(destroySpo, spo);

        // Open client port.
        rc = spo->open(clientSocket);
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to open client port: error %d.", rc);
            return rc;
        }
        clientSockInit = true;
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Opened client port at port %d using socket "FMT_VPLSocket_t,
                            VPLNet_port_ntoh(VPLSocket_GetPort(clientSocket)), VAL_VPLSocket_t(clientSocket));

        // Open Secure CLEARFI port.
        rc = spo->open(secureClearfiSocket);
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to open Secure CLEARFI port: error %d.", rc);
            return rc;
        }
        secureClearfiSockInit = true;
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Opened Secure CLEARFI port at port %d using socket "FMT_VPLSocket_t,
                            VPLNet_port_ntoh(VPLSocket_GetPort(secureClearfiSocket)), VAL_VPLSocket_t(secureClearfiSocket));
    }

#ifdef VPL_PIPE_AS_SOCKET_OK
    {
        int i;
        int flags;
        rc = pipe(pipefds);
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to create notify pipe: %s(%d)",
                             strerror(errno), errno);
        }

        for(i = 0; i < 2; i++) {
            flags = fcntl(pipefds[i], F_GETFL);
            if (flags == -1) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to get flags for end %d of pipe: %s(%d)",
                                 i, strerror(errno), errno);
            }
            rc = fcntl(pipefds[i], F_SETFL, flags | O_NONBLOCK);
            if(rc == -1) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to set nonblocking flag for end %d of pipe: %s(%d)",
                                 i, strerror(errno), errno);
            }
        }
    }
#endif

    // Launch worker threads.
    VPLThread_attr_t thread_attr;
    VPLThread_AttrInit(&thread_attr);
    VPLThread_AttrSetStackSize(&thread_attr, WORKER_STACK_SIZE);
    VPLThread_AttrSetDetachState(&thread_attr, false);
    num_workers = 0;
    worker_active_max = 0;
    task_waiting_total = task_waiting_max = task_waiting_last = 0;
    task_waiting_min = VPLTIME_INVALID;
    task_waiting_max_count = 0;
    worker_active_total = worker_active_max = worker_active_last = 0;
    worker_active_min = VPLTIME_INVALID;
    task_waiting_total = task_waiting_max = task_waiting_last = 0;
    task_waiting_min = VPLTIME_INVALID;
    task_waiting_max_count = 0;
    tasks_finished = 0;

    workers = new worker_state[worker_threads];
    running = true;
    for(u32 i = 0; i < worker_threads; i++) {
        stringstream name;
        name << "worker_" << i;
        workers[i].name = name.str();
        VPLThread_Create(&(workers[i].thread), workerFunction, VPL_AS_THREAD_FUNC_ARG(this), &thread_attr, workers[i].name.c_str());
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Started worker %d/%d.",
                          i + 1, worker_threads);
    }
    VPLThread_AttrDestroy(&thread_attr);

    // Launch stats thread.
    VPLThread_attr_t stats_thread_attr;
    VPLThread_AttrInit(&stats_thread_attr);
    VPLThread_AttrSetStackSize(&stats_thread_attr, WORKER_STACK_SIZE);
    VPLThread_AttrSetDetachState(&stats_thread_attr, false);

    VPLThread_Create(&(stats_thread), statsFunction, VPL_AS_THREAD_FUNC_ARG(this), &stats_thread_attr, "stats");

    VPLThread_AttrDestroy(&stats_thread_attr);

#ifndef VPL_PIPE_AS_SOCKET_OK
    // Set up a socket for waking up the worker thread.
    rc = create_private_sockets(priv_socket_server_listen, priv_socket_client, priv_socket_server_connected);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                          "create priv socket failed: %d", rc);
        goto fail_create_priv_sockets;
    }
#endif

    // Begin server loop in a new thread.
    rc = Util_SpawnThread(main_loop_thread_fn,
                          this,
                          VSS_SERVER_STACK_SIZE,
                          VPL_TRUE,
                          &mainThreadHandle);
    if (rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "%s failed: %d", "Util_SpawnThread", rc);
        goto fail_thread;
    }
    mainStarted = true;

    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Storage node server thread started.");
    return 0;

fail_thread:
#ifndef VPL_PIPE_AS_SOCKET_OK
    VPLSocket_Close(priv_socket_server_connected);
    VPLSocket_Close(priv_socket_client);
    VPLSocket_Close(priv_socket_server_listen);
#endif
    return rc;
#ifndef VPL_PIPE_AS_SOCKET_OK
fail_create_priv_sockets:
    return rc;
#endif
}


void vss_server::addTask(void (*task)(void*), void* params)
{
    vss_task* entry = new vss_task;
    entry->params = params;
    entry->task = task;
    entry->wait_start = VPLTime_GetTimeStamp();
    VPLMutex_Lock(&task_mutex);
    taskQ.push_back(entry);
    if(taskQ.size() > task_waiting_max_count) {
        task_waiting_max_count = taskQ.size();
    }
    VPLCond_Signal(&task_condvar);
    VPLMutex_Unlock(&task_mutex);
}

VPLThread_return_t workerFunction(VPLThread_arg_t vpserver)
{
    vss_server* pserver = static_cast<vss_server*>(vpserver);
    pserver->doWork();
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

void vss_server::doWork()
{
    vss_task* task = NULL;
    VPLTime_t task_start, task_end, task_time;

    VPLMutex_Lock(&task_mutex);

    num_workers++;
    while(running) {
        if(taskQ.empty()) {
            VPLCond_TimedWait(&task_condvar, &task_mutex, VPL_TIMEOUT_NONE);
        }
        else {
            task = taskQ.front();
            taskQ.pop_front();

            // Track task waiting statistics
            task_time = VPLTime_GetTimeStamp() - task->wait_start;
            if(task_time < task_waiting_min) {
                task_waiting_min = task_time;
            }
            if(task_time > task_waiting_max) {
                task_waiting_max = task_time;
            }
            task_waiting_last = task_time;
            task_waiting_total += task_time;

            // Mark worker active for statistics
            workers_active++;
            
            VPLMutex_Unlock(&task_mutex);

            task_start = VPLTime_GetTimeStamp();
            (task->task)(task->params);
            delete task;
            task = NULL;
            task_end = VPLTime_GetTimeStamp();

            VPLMutex_Lock(&task_mutex);
            
            // Track worker statistics
            workers_active--;
            tasks_finished++;

            task_time = task_end - task_start;
            worker_active_last = task_time;
            worker_active_total += task_time;
            if(worker_active_min > task_time) {
                worker_active_min = task_time;
            }
            if(worker_active_max < task_time) {
                worker_active_max = task_time;
            }
        }
    }
    VPLMutex_Unlock(&task_mutex);
}

// static function
VPLTHREAD_FN_DECL vss_server::main_loop_thread_fn(void* param)
{
    vss_server* object = static_cast<vss_server*>(param);
    object->main_loop();
    return VPLTHREAD_RETURN_VALUE;
}

void vss_server::stop()
{
    // Issue quit signal to stop the server cold.
    const char msg = VSS_HALT;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                        "Storage node server thread stop requested.");

#ifdef ENABLE_PHOTO_TRANSCODE
    int rv = ImageTranscode_Shutdown();
    if (rv != 0) {
        if (rv == IMAGE_TRANSCODING_NOT_INITIALIZED) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Trying to shutdown uninitialized ImageTranscode, ignore.");
        } else {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Failed to shutdown ImageTranscode, rv = %d", rv);
        }
    }
#endif

#ifdef VPL_PIPE_AS_SOCKET_OK
    int rc = write(pipefds[1], &msg, sizeof(msg));
    (void)rc;
#else
    commandQ.enqueue(msg);
    int temp_rv = VPLSocket_Write(priv_socket_client, " ", 1, VPL_TIMEOUT_NONE);
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "Wrote byte to priv socket: %d", temp_rv);
#endif

    // Best-effort to join with main thread when started.
    if(mainStarted) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                            "Joining storage node server thread.");
        VPLDetachableThread_Join(&mainThreadHandle);
        mainStarted = false;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                            "Storage node server thread stopped.");
    }

#ifndef VPL_PIPE_AS_SOCKET_OK
    // private socket.
    VPLSocket_Close(priv_socket_server_connected);
    VPLSocket_Close(priv_socket_client);
    VPLSocket_Close(priv_socket_server_listen);
#endif

    //cleanup access control list
    HttpSvc::Sn::Handler_rf_Helper::blackList.clear();
    HttpSvc::Sn::Handler_rf_Helper::whiteList.clear();
    HttpSvc::Sn::Handler_rf_Helper::userWhiteList.clear();
    HttpSvc::Sn::Handler_rf_Helper::acDirs.clear();

    if (HttpSvc::Sn::Handler_rf_Helper::rfSearchManager != NULL) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping rfSearchManager");
        int temp_rc = HttpSvc::Sn::Handler_rf_Helper::rfSearchManager->StopManager();
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping rfSearchManager returned");
        if (temp_rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rfSearchManager->StopManager():%d", temp_rc);
        }
    }
}

void vss_server::changeNotice()
{
    // Issue refresh signal to get main loop to refresh work.
    const char msg = VSS_REFRESH;
#ifdef VPL_PIPE_AS_SOCKET_OK
    int rc = write(pipefds[1], &msg, sizeof(msg));
    (void)rc;
#else
    commandQ.enqueue(msg);
    int temp_rv = VPLSocket_Write(priv_socket_client, " ", 1, VPL_TIMEOUT_NONE);
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Wrote byte to priv socket: %d", temp_rv);
#endif
}

void vss_server::notifyNetworkChange(VPLNet_addr_t localAddr)
{
    // Network connectivity has been established or changed.
    curAddr = localAddr;
    reportedAddr = VPLNET_ADDR_INVALID;
    lastReportAddrTime = VPLTIME_INVALID;
    changeNotice();
}

void vss_server::disconnectAllClients()
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Disconnecting all clients.");

    VPLMutex_Lock(&mutex);
    {
        std::map<int, vss_client*>::iterator it;
        for(it = clients.begin(); it != clients.end(); ++it) {
            it->second->disconnect();
        }
    }
    {
        std::map<int, strm_http*>::iterator it;
        for(it = clearfi_clients.begin(); it != clearfi_clients.end(); ++it) {
            it->second->disconnect();
        }
    }
    {
        std::map<int, vss_p2p_client*>::iterator it;
        for(it = p2p_clients.begin(); it != p2p_clients.end(); ++it) {
            it->second->disconnect();
        }
    }

    stat_disconnect_called = true;
    VPLMutex_Unlock(&mutex);

    changeNotice();
}

// Used by strm_http.cpp to verify the device is linked
bool vss_server::isDeviceLinked(u64 user_id, u64 device_id)
{
    u64 effective_device_id = device_id & 0xffffffffffull;

    return ((user_id != 0) && query.isDeviceLinked(user_id, effective_device_id));
}

bool vss_server::receiveNotification(const void* data, u32 size)
{
    char* req = (char*)data;
    u64 user_id;
    u64 device_id = vss_get_device_id(req);
    vss_session* session = get_session(vss_get_handle(req));
    bool rv = false;
    bool verify_device;
    int rc;

    // Verify the request as good.
    if(session == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to find session for notification.");
        goto exit;
    }
    user_id = session->get_uid();

    rc = session->verify_header(req, verify_device);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to authenticate notification header.");
        goto exit;
    }

    if ( verify_device && !isDeviceLinked(user_id, device_id) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Attempted access from unlinked"
            " device, uid: "FMTu64" did: "FMTu64, user_id, device_id);
        rc = VSSI_PERM;
        goto exit;
    }

    rc = session->validate_request_data(req);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to authenticate notification data.");
        goto exit;
    }
    
    // The only valid notification is make proxy connection.
    switch(vss_get_command(req)) {
    case VSS_PROXY_CONNECT:
        makeProxyConnection(req, session);
        rv = true;
        break;
    default:
        break;
    }

 exit:
    if(session) {
        release_session(session);
    }

    return rv;
}

void vss_server::makeProxyConnection(const char* req, vss_session* session)
{
    string address;
    const char* data = req + VSS_HEADER_SIZE;
    char* resp = NULL;
    char* resp_data;
    VPLSocket_t sockfd = VPLSOCKET_INVALID;
    u8 proxy_type = vss_proxy_connect_get_type(data);
    VPLNet_addr_t client_ip = vss_proxy_connect_get_client_ip(data);
    VPLNet_port_t client_port= vss_proxy_connect_get_client_port(data);

    address.assign(vss_proxy_connect_get_server_addr(data),
                   vss_proxy_connect_get_addrlen(data));
    address.append(".");
    address.append(infraDomain);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Received version %d request to make proxy connection via %s:%d, to "FMTu64" with P2P connect at "FMT_VPLNet_addr_t":%u, type %s.",
                      vss_get_version(req),
                      address.c_str(),                      
                      vss_proxy_connect_get_server_port(data),
                      vss_get_device_id(req),
                      VAL_VPLNet_addr_t(client_ip), client_port,
                      proxy_type == VSS_PROXY_CONNECT_TYPE_VSSP ? "VSSP" : 
                      proxy_type == VSS_PROXY_CONNECT_TYPE_STREAM ? "Stream" : 
                      proxy_type == VSS_PROXY_CONNECT_TYPE_SSTREAM ? "Secure Stream" : 
                      proxy_type == VSS_PROXY_CONNECT_TYPE_HTTP ? "HTTP" :
                      "UNKNOWN");

    // Open designated socket.
    open_client_socket(address, vss_proxy_connect_get_server_port(data), 
                       client_port == 0 ? false : true, sockfd);

    if(VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        goto exit;
    }

    // If P2P connection needed, start P2P connectivity.
    if(client_port != 0) {
        VPLSocket_addr_t client_addr;
        VPLSocket_addr_t origin_addr;
        client_addr.family = VPL_PF_INET;
        client_addr.addr = client_ip;
        client_addr.port = VPLNet_port_hton(client_port);
        origin_addr.family = VPL_PF_INET;
        origin_addr.addr = VPLSocket_GetAddr(sockfd);
        origin_addr.port = VPLSocket_GetPort(sockfd);

        vss_p2p_client* p2p = new vss_p2p_client(*this, 
                                                 get_session(vss_get_handle(req)),
                                                 proxy_type, 
                                                 vss_get_device_id(req),
                                                 origin_addr, client_addr);
        
        if(p2p->start()) {
            VPLMutex_Lock(&mutex);
            if(p2p->connected) {
                p2p_clients[VPLSocket_AsFd(p2p->sockfd)] = p2p;
            }
            else {
                p2p_clients[VPLSocket_AsFd(p2p->punch_sockfd)] = p2p;
                p2p_clients[VPLSocket_AsFd(p2p->listen_sockfd)] = p2p;
            }
            VPLMutex_Unlock(&mutex);
        }
        else {
            // Failed to begin P2P connection.
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Failed to create P2P connection.");
            delete p2p;
            client_port = 0; // Not performing P2P connection anymore.
        }
    }
        
    // Compose response message.
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_PROXY_CONNECTR_SIZE, 1);
    resp_data = resp + VSS_HEADER_SIZE;
    vss_set_version(resp, vss_get_version(req));
    vss_set_command(resp, VSS_PROXY_CONNECT_REPLY);
    vss_set_status(resp, 0);
    vss_set_xid(resp, vss_get_xid(req));
    vss_set_device_id(resp, vss_get_device_id(req));
    vss_set_handle(resp, vss_get_handle(req));
    vss_set_data_length(resp, VSS_PROXY_CONNECTR_SIZE);
    vss_proxy_connect_reply_set_cluster_id(resp_data, clusterId);
    vss_proxy_connect_reply_set_cookie(resp_data, vss_proxy_connect_get_cookie(data));
    vss_proxy_connect_reply_set_port(resp_data, client_port);
    vss_proxy_connect_reply_set_type(resp_data, proxy_type);
    
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Sending proxy connection init message with version:%d cluster_id:"FMTx64", port:%d, cookie:%d, type:%d.",   
                      vss_get_version(resp),
                      vss_proxy_connect_reply_get_cluster_id(resp_data),
                      vss_proxy_connect_reply_get_port(resp_data),
                      vss_proxy_connect_reply_get_cookie(resp_data),
                      vss_proxy_connect_reply_get_type(resp_data));
    
    session->sign_reply(resp);
    
    switch(proxy_type) {
    case VSS_PROXY_CONNECT_TYPE_VSSP: {
        vss_client* client = new vss_client(*this, sockfd, 
                                            client_ip, client_port);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Make proxy VSS client connection to %s:"FMT_VPLNet_port_t" on socket "FMT_VPLSocket_t".",
                          address.c_str(), vss_proxy_connect_get_server_port(data), VAL_VPLSocket_t(sockfd));
        
        if(client->start(VPLTIME_FROM_SEC(PROXY_CLIENT_INACTIVE_TIMEOUT_SEC)) == 0) {
            VPLMutex_Lock(&mutex);
            clients[VPLSocket_AsFd(sockfd)] = client;
            VPLMutex_Unlock(&mutex);
            
            // Send connection init message.
            client->put_response(resp, true);
            resp = NULL; // don't delete.
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "VSS Client fails to connect.");
            delete client;
            goto exit;
        }
    } break;
        
    case VSS_PROXY_CONNECT_TYPE_STREAM: 
    case VSS_PROXY_CONNECT_TYPE_SSTREAM: {
        strm_http* clearfi_client = new strm_http(*this, sockfd, STRM_PROXY,
                                                  (proxy_type == VSS_PROXY_CONNECT_TYPE_SSTREAM) ? get_session(vss_get_handle(req)) : NULL, 
                                                  vss_get_device_id(req));
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Make %sproxy stream (clear.fi) client connection to %s:"FMT_VPLNet_port_t" on socket "FMT_VPLSocket_t".",
                          (proxy_type == VSS_PROXY_CONNECT_TYPE_SSTREAM) ? "secure " : "",
                          address.c_str(), 
                          vss_proxy_connect_get_server_port(data), 
                          VAL_VPLSocket_t(sockfd));
        
        if(clearfi_client->start(VPLTIME_INVALID) == 0) {
            VPLMutex_Lock(&mutex);
            clearfi_clients[VPLSocket_AsFd(sockfd)] = clearfi_client;
            VPLMutex_Unlock(&mutex);
            // Send connection init message.
            clearfi_client->put_proxy_response(resp, VSS_HEADER_SIZE + VSS_PROXY_CONNECTR_SIZE);
            resp = NULL; // don't delete.
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream Client fails to connect.");
            delete clearfi_client;
            goto exit;
        }
    } break;
        
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Told to make proxy connection to %s:%d for unknown client type.",
                         address.c_str(), vss_proxy_connect_get_server_port(data));
        VPLSocket_Close(sockfd);
        goto exit;
    }
    
    // Keep system awake as proxy connection is made.
    if(!sleepDeferred) {
        sleepDeferred = true;
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "SLEEP: Keep system awake. Proxy connect in-progress.");
    }

    VPLPowerMan_PostponeSleep(VPL_POWERMAN_ACTIVITY_SERVING_DATA, NULL);
    nextPostponeSleepTime = VPLTime_GetTimeStamp() + VPLTime_FromSec(POSTPONE_SLEEP_INTERVAL_SEC);

 exit:
    if(resp) {
        free(resp);
    }
}

void vss_server::noticeP2PConnected(vss_p2p_client* p2p_client)
{
    VPLMutex_Lock(&mutex);

    // Change index for this client.
    p2p_clients.erase(VPLSocket_AsFd(p2p_client->punch_sockfd));
    p2p_clients.erase(VPLSocket_AsFd(p2p_client->listen_sockfd));
    p2p_clients[VPLSocket_AsFd(p2p_client->sockfd)] = p2p_client;

    VPLMutex_Unlock(&mutex);
}

void vss_server::makeP2PConnection(VPLSocket_t sockfd, VPLSocket_addr_t sockaddr, vss_session* session,
                                   u64 client_device_id, u8 client_type,
                                   char* resp)
{
    switch(client_type) {
    case VSS_PROXY_CONNECT_TYPE_VSSP: {
        vss_client* client = new vss_client(*this, sockfd, sockaddr.addr,VPLNet_port_ntoh(sockaddr.port));
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Make P2P VSS client connection to "FMTu64" on socket "FMT_VPLSocket_t".",
                          client_device_id, VAL_VPLSocket_t(sockfd));
        
        release_session(session); // Don't need this.

        if(client->start(VPLTIME_FROM_SEC(PROXY_CLIENT_INACTIVE_TIMEOUT_SEC)) == 0) {
            clients[VPLSocket_AsFd(sockfd)] = client;
            
            // Send auth response message.
            client->put_response(resp, true);
            resp = NULL; // don't delete.
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "VSS Client fails to connect.");
            delete client;
        }
    } break;

    case VSS_PROXY_CONNECT_TYPE_STREAM:  // Never used
    case VSS_PROXY_CONNECT_TYPE_SSTREAM: {
        strm_http* clearfi_client = new strm_http(*this, sockfd, STRM_P2P,
                                                  session, client_device_id);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Make secure P2P stream (clear.fi) client connection to "FMTu64" on socket "FMT_VPLSocket_t".",
                          client_device_id,
                          VAL_VPLSocket_t(sockfd));
        
        if(clearfi_client->start(VPLTIME_INVALID) == 0) {
            clearfi_clients[VPLSocket_AsFd(sockfd)] = clearfi_client;
            // Send connection init message.
            clearfi_client->put_proxy_response(resp, VSS_HEADER_SIZE + vss_get_data_length(resp));
            resp = NULL; // don't delete.
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream Client fails to connect.");
            delete clearfi_client;
        }
    } break;
        
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Told to make p2p connection to "FMTu64" for unknown client type.",
                         client_device_id);
        VPLSocket_Close(sockfd);
        release_session(session);
    }
    
    if(resp) {
        free(resp);
    }
}

void vss_server::close_unused_files()
{
    // For each dataset, close unused files.
    map<dataset_id, dataset*>::iterator cursor;
    for(cursor = datasets.begin(); cursor != datasets.end(); cursor++) {
        cursor->second->close_unused_files();
    }
}

void vss_server::main_loop()
{
    bool terminated = false;
    int rc;
    std::map<int, vss_client*>::iterator clientIt;
    std::map<int, strm_http*>::iterator clearfiIt;
    std::map<int, vss_p2p_client*>::iterator p2pIt;
    int i;
    VPLTime_t poll_timeout = 0;

    do {
        VPLMutex_Lock(&mutex);

        int pollspec_size =  2 +
#ifdef VPL_PIPE_AS_SOCKET_OK
             1 +// for pipe
#else
             1 +// for private socket
#endif
            // Add an extra pollspec for the private socket.
            clients.size() + clearfi_clients.size() + p2p_clients.size();
        VPLSocket_poll_t *pollspec = new VPLSocket_poll_t[pollspec_size];
        ON_BLOCK_EXIT(deleteArray<VPLSocket_poll_t>, pollspec);

        // Always receive for client sockets (accept connections)
        i = 0;
#ifdef VPL_PIPE_AS_SOCKET_OK
        pollspec[i].socket = *((VPLSocket_t*)(&(pipefds[0])));
        pollspec[i].events = VPLSOCKET_POLL_RDNORM;
        i++;
#else
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "add priv socket: %d", pollspec_size);
        pollspec[i].socket = priv_socket_server_connected;
        pollspec[i].events = VPLSOCKET_POLL_RDNORM;
        i++;
#endif
        pollspec[i].socket = clientSocket;
        pollspec[i].events = VPLSOCKET_POLL_RDNORM;
        i++;
        pollspec[i].socket = secureClearfiSocket;
        pollspec[i].events = VPLSOCKET_POLL_RDNORM;
        i++;

        for(clientIt = clients.begin(); clientIt != clients.end(); clientIt++) {
            if(clientIt->second->receiving || clientIt->second->sending) {
                pollspec[i].events = 0;
                pollspec[i].socket = clientIt->second->sockfd;
                
                if(clientIt->second->receiving) {
                    pollspec[i].events |= VPLSOCKET_POLL_RDNORM;
                }
                if(clientIt->second->sending) {
                    pollspec[i].events |= VPLSOCKET_POLL_OUT;
                }

                i++;
            }
        }        
        for(clearfiIt = clearfi_clients.begin(); clearfiIt != clearfi_clients.end(); clearfiIt++) {
            if(clearfiIt->second->receiving || clearfiIt->second->sending) {
                pollspec[i].events = 0;
                pollspec[i].socket = clearfiIt->second->sockfd;
                if(clearfiIt->second->receiving) {
                    pollspec[i].events |= VPLSOCKET_POLL_RDNORM;
                }
                if(clearfiIt->second->sending) {
                    pollspec[i].events |= VPLSOCKET_POLL_OUT;
                }
                
                i++;
            }
        }        
        for(p2pIt = p2p_clients.begin(); p2pIt != p2p_clients.end(); p2pIt++) {

            // P2P clients with a connection need to receive on the connected
            // socket.
            // Otherwise, receive on the listening socket.
            if(p2pIt->second->connected) {
                // P2P clients never send.
                if(p2pIt->second->receiving) {
                    pollspec[i].events = 0;
                    pollspec[i].socket = p2pIt->second->sockfd;
                    pollspec[i].events |= VPLSOCKET_POLL_RDNORM;
                    i++;
                }
            }
            else {
                if(p2pIt->second->listening &&
                   p2pIt->first == VPLSocket_AsFd(p2pIt->second->listen_sockfd)) {
                    pollspec[i].events = 0;
                    pollspec[i].socket = p2pIt->second->listen_sockfd;
                    pollspec[i].events |= VPLSOCKET_POLL_RDNORM;
                    i++;
                }
                if(p2pIt->second->punching &&
                   p2pIt->first == VPLSocket_AsFd(p2pIt->second->punch_sockfd)) {
                    pollspec[i].events = 0;
                    pollspec[i].socket = p2pIt->second->punch_sockfd;
                    pollspec[i].events |= VPLSOCKET_POLL_OUT;
                    i++;
                }
            }
        }        
        pollspec_size = i;
   
        VPLMutex_Unlock(&mutex);

        // Wait for something to do
#ifndef VPL_PIPE_AS_SOCKET_OK
        VPLTime_t timeout = VPL_TIMEOUT_NONE;
#endif

        rc = VPLSocket_Poll(pollspec, pollspec_size, 
#ifdef VPL_PIPE_AS_SOCKET_OK
                            poll_timeout
#else
                            timeout
#endif
                            );
        if(rc < 0) {
            switch(rc) {
            case VPL_ERR_AGAIN:
            case VPL_ERR_INVALID: // Sudden connection closure.
            case VPL_ERR_INTR: // Debugger interrupt.
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Poll() error: %d. Continuing.",
                                  rc);
                break;
            default: // Something unexpected. Crashing!
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Poll() error: %d.",
                                 rc);
                goto exit;
            }
        }
        
        // Handle commands
#ifdef VPL_PIPE_AS_SOCKET_OK
        do {
            char cmd;
            rc = read(pipefds[0], &cmd, 1);
            if(rc == -1) {
                if(errno == EAGAIN || errno == EINTR) {
                    // no big deal
                    break;
                }
                else {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                                     "Got error %s(%d) reading notify pipe.",
                                     strerror(errno), errno);
                    break;
                }
            }
            else if(rc == 1) {
                switch(cmd) {
                case VSS_HALT:
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "Server halting...");
                    goto exit;
                default:
                    // Just eat the request. No special handling.
                    break;
                }
            }
        } while(rc == 1);
#else
        while (!commandQ.empty()) {
            char cmd = commandQ.dequeue();
            switch(cmd) {
            case VSS_HALT:
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "Server halting...");
                goto exit;
                break;
            default:
                // Just eat the request. No special handling.
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "bad cmd byte %02x", cmd);
                break;
            }
        }
#endif

        // Handle send/receive
        for (int i = 0; i < pollspec_size; i++) {
        #ifdef VPL_PIPE_AS_SOCKET_OK
            if(VPLSocket_Equal(pollspec[i].socket, clientSocket)) {
        #else
            if(VPLSocket_Equal(pollspec[i].socket, priv_socket_server_connected) && (pollspec[i].revents & VPLSOCKET_POLL_RDNORM)) {
                // throw away the byte in the socket
                {
                    char tmp;
                    rc = VPLSocket_Recv(pollspec[i].socket, &tmp, 1);
                }
            }
            // Handle new clients (accept)
            else if(VPLSocket_Equal(pollspec[i].socket, clientSocket)) {
        #endif
                if (pollspec[i].revents & 
                    (VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_SO_INVAL | VPLSOCKET_POLL_EV_INVAL)) {
                    VPLSocket_t newClientFd;
                    VPLSocket_addr_t addr;
                    
                    rc = VPLSocket_Accept(clientSocket, &addr, sizeof(addr),
                                          &newClientFd);
                    
                    if (rc != VPL_OK) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Client connection accept error. socket:"FMT_VPLSocket_t", error:%d", 
                                         VAL_VPLSocket_t(clientSocket), rc);
                    }
                    else {
                        vss_client* client = new vss_client(*this, newClientFd,
                                                            addr.addr, VPLNet_port_ntoh(addr.port));
                        
                        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                          "Accept VSS client connection from "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t" on socket "FMT_VPLSocket_t".",
                                          VAL_VPLNet_addr_t(addr.addr), VPLNet_port_ntoh(addr.port), 
                                          VAL_VPLSocket_t(newClientFd));
                        
                        if(client->start(VPLTIME_FROM_SEC(DIRECT_CLIENT_INACTIVE_TIMEOUT_SEC)) == 0) {
                            VPLMutex_Lock(&mutex);
                            clients[VPLSocket_AsFd(newClientFd)] = client;
                            VPLMutex_Unlock(&mutex);
                        }
                        else {
                            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                             "VSS Client fails to connect.");
                            delete client;
                        }
                    }
                }
            }
            else if(VPLSocket_Equal(pollspec[i].socket, secureClearfiSocket)) {
                if (pollspec[i].revents & 
                    (VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_SO_INVAL | VPLSOCKET_POLL_EV_INVAL)) {
                    VPLSocket_t newClientFd;
                    VPLSocket_addr_t addr;
                    
                    rc = VPLSocket_Accept(secureClearfiSocket, &addr, sizeof(addr),
                                          &newClientFd);
                    
                    if (rc != VPL_OK) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Secure clear.fi Client connection accept error. socket:"FMT_VPLSocket_t", error:%d", 
                                         VAL_VPLSocket_t(secureClearfiSocket), rc);
                    }
                    else {
                        strm_http* clearfi_client = 
                            new strm_http(*this, newClientFd, STRM_DIRECT,
                                          NULL, 0, true);
                        
                        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                          "Accept Secure clear.fi client connection (need auth) from "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t" on socket "FMT_VPLSocket_t".",
                                          VAL_VPLNet_addr_t(addr.addr), addr.port, 
                                          VAL_VPLSocket_t(newClientFd));
                        
                        if(clearfi_client->start(VPLTIME_INVALID) == 0) {
                            VPLMutex_Lock(&mutex);
                            clearfi_clients[VPLSocket_AsFd(newClientFd)] = clearfi_client;
                            VPLMutex_Unlock(&mutex);
                        }
                        else {
                            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                             "STRM-HTTP clearfi client fails to connect.");
                            delete clearfi_client;
                        }
                    }
                }
            }
            else if(clients.find(VPLSocket_AsFd(pollspec[i].socket)) != clients.end()) {
                if (pollspec[i].revents & 
                    (VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_SO_INVAL | VPLSOCKET_POLL_EV_INVAL)) {
                    clients[VPLSocket_AsFd(pollspec[i].socket)]->do_receive();
                }
                if (pollspec[i].revents & VPLSOCKET_POLL_OUT) {
                    clients[VPLSocket_AsFd(pollspec[i].socket)]->do_send();
                }
            }
            else if(clearfi_clients.find(VPLSocket_AsFd(pollspec[i].socket)) != clearfi_clients.end()) {
                if (pollspec[i].revents & 
                    (VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_SO_INVAL | VPLSOCKET_POLL_EV_INVAL)) {
                    clearfi_clients[VPLSocket_AsFd(pollspec[i].socket)]->do_receive();
                }
                if (pollspec[i].revents & VPLSOCKET_POLL_OUT) {
                    clearfi_clients[VPLSocket_AsFd(pollspec[i].socket)]->do_send();
                }
            }
            else if(p2p_clients.find(VPLSocket_AsFd(pollspec[i].socket)) != p2p_clients.end()) {
                if (pollspec[i].revents & 
                    (VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_OUT | VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_SO_INVAL | VPLSOCKET_POLL_EV_INVAL)) {
                    p2p_clients[VPLSocket_AsFd(pollspec[i].socket)]->do_receive(pollspec[i].socket);
                }
            }
        }

        // Handle timeouts
        poll_timeout = timeout_handler();

    } while(!terminated);
 exit:
    // Stop all workers.
    VPLMutex_Lock(&task_mutex);
    VPLMutex_Lock(&stats_mutex);

    running = false;
    VPLCond_Broadcast(&task_condvar);
    VPLCond_Broadcast(&stats_condvar);

    VPLMutex_Unlock(&stats_mutex);
    VPLMutex_Unlock(&task_mutex);

    for(u32 i = 0; i < num_workers ; i++) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Joining worker %d/%d.",
                          i+1, num_workers);
        VPLThread_Join(&(workers[i].thread), NULL);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Joined with worker %d/%d on shutdown.",
                          i+1, num_workers);
    }
    delete[] workers;
    workers = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Joining stats thread.");
    VPLThread_Join(&(stats_thread), NULL);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Joined with stats thread on shutdown.");

    return;
}

VPLTime_t vss_server::timeout_handler()
{
    VPLTime_t rv = VPLTime_FromSec(DATASET_STAT_UPDATE_INTERVAL_SEC);
    VPLTime_t cur_time = VPLTime_GetTimeStamp();
    bool clients_active = false;
    string hostName;
    char hostNameBuf[VPLNET_ADDRSTRLEN];
    VPLNet_port_t clientPortUsed, clearfiPortUsed, tsPortUsed;

    if(curAddr != reportedAddr) {
        if(curAddr == VPLNET_ADDR_INVALID) {
            // Will report on the next valid address change.
            reportedAddr = curAddr;
            lastReportAddrTime = VPLTIME_INVALID;
        }
        else {
            if(lastReportAddrTime == VPLTIME_INVALID ||
               cur_time > lastReportAddrTime + VPLTIME_FROM_SEC(ADDRESS_REPORT_INTERVAL_SEC)) {
                lastReportAddrTime = cur_time;
                
                if(VPLNet_Ntop(&curAddr, hostNameBuf, VPLNET_ADDRSTRLEN) == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Failed to convert address "FMT_VPLNet_addr_t" into a %d byte or less string.",
                                     VAL_VPLNet_addr_t(curAddr), VPLNET_ADDRSTRLEN);
                    goto skip_update;
                }

                hostName.assign(hostNameBuf, strlen(hostNameBuf));
                
                clientPortUsed = VPLNet_port_ntoh(VPLSocket_GetPort(clientSocket));
                clearfiPortUsed = VPLNet_port_ntoh(VPLSocket_GetPort(secureClearfiSocket));
                tsPortUsed = 0;
                {
                    int port = 0;
                    if (TS::TS_GetPort(port) == 0) {
                        tsPortUsed = port;
                    }
                }
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "Reporting PSN IP address change to VSDS (was "FMT_VPLNet_addr_t", now "FMT_VPLNet_addr_t") VSSI port:%d clearfi port:%d TS port:%d.",
                                    VAL_VPLNet_addr_t(reportedAddr),
                                    VAL_VPLNet_addr_t(curAddr),
                                    clientPortUsed, clearfiPortUsed, tsPortUsed);
                if(query.updateConnection(hostName, clientPortUsed, clearfiPortUsed, tsPortUsed) == 0) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Reported current address %s info.",
                                      hostName.c_str());
                    reportedAddr = curAddr;
                }
                else {
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Failed to report current address info. Will retry in %us.",
                                      ADDRESS_REPORT_INTERVAL_SEC);
                    rv = VPLTime_FromSec(ADDRESS_REPORT_INTERVAL_SEC);
                }
            }
            else {
                rv = VPLTime_DiffClamp(lastReportAddrTime + VPLTIME_FROM_SEC(ADDRESS_REPORT_INTERVAL_SEC),
                                       cur_time);
            }
        }
    }

 skip_update:
    VPLMutex_Lock(&mutex);

    // For each client type, reap inactive clients and look for active clients.
    {
        std::map<int, vss_client*>::iterator it;
        for(it = clients.begin(); it != clients.end();) {
            std::map<int, vss_client*>::iterator tmp = it;
            it++;
            if(tmp->second->inactive()) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Clean-up client socket %d.",
                                  tmp->first);
                delete tmp->second;
                clients.erase(tmp);
            }
            else if(tmp->second->active()) {
                clients_active = true;
            }
        }    
    }
    {
        std::map<int, strm_http*>::iterator it;
        for(it = clearfi_clients.begin(); it != clearfi_clients.end();) {
            std::map<int, strm_http*>::iterator tmp = it;
            it++;
            if(tmp->second->inactive()) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Timeout or disconnect for CLEARFI client socket %d: "FMT_VPLTime_t"us have passed since last active.",
                                  tmp->first,
                                  VPLTime_DiffClamp(cur_time, tmp->second->last_active));
                delete tmp->second;
                clearfi_clients.erase(tmp);
            }
            else if(tmp->second->active()) {
                clients_active = true;
            }
        }    
    }
    {
        std::map<int, vss_p2p_client*>::iterator it;
        for(it = p2p_clients.begin(); it != p2p_clients.end();) {
            std::map<int, vss_p2p_client*>::iterator tmp = it;
            it++;
            if(tmp->second->inactive()) {
                vss_p2p_client* client = tmp->second;
                
                p2p_clients.erase(tmp);
                
                if((VPLSocket_Equal(client->punch_sockfd, VPLSOCKET_INVALID) ||
                    p2p_clients.find(VPLSocket_AsFd(client->punch_sockfd)) == p2p_clients.end()) &&
                   (VPLSocket_Equal(client->listen_sockfd, VPLSOCKET_INVALID) ||
                    p2p_clients.find(VPLSocket_AsFd(client->listen_sockfd)) == p2p_clients.end()) &&
                   (VPLSocket_Equal(client->sockfd, VPLSOCKET_INVALID) ||
                    p2p_clients.find(VPLSocket_AsFd(client->sockfd)) == p2p_clients.end())) {
                    // All references to this vss_p2p_client are gone.
                    delete client;
                }
            }
            else if(tmp->second->active()) {
                clients_active = true;
            }
        }    
    }

    // Look for timed-out sessions
    if(!self_session->try_clear_timeout_objects()) {     
        if(rv > VPLTime_FromSec(OBJECT_INACTIVE_TIMEOUT_RETRY_SEC)) {  
            rv = VPLTime_FromSec(OBJECT_INACTIVE_TIMEOUT_RETRY_SEC);  
        }    
    }
    VPLMutex_Unlock(&mutex);

    // Check if clients are actively communicating.
    // If so, make sure PC stays awake when clients are active and also for a default amount of time (now 10 minutes)
    // after the client connection becomes inactive 
    if(clients_active) {
        if(!sleepDeferred) {
            sleepDeferred = true;
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "SLEEP: Keep system awake. Clients are active.");
        }
        if((nextPostponeSleepTime == VPLTIME_INVALID ||        
            cur_time > nextPostponeSleepTime)) {
            VPLPowerMan_PostponeSleep(VPL_POWERMAN_ACTIVITY_SERVING_DATA, NULL);
            nextPostponeSleepTime = cur_time + VPLTime_FromSec(POSTPONE_SLEEP_INTERVAL_SEC);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "SLEEP: Sleep postponed. Next PostponeSleep will be called at "FMT_VPLTime_t".", 
                              nextPostponeSleepTime);
        }

        if(rv > VPLTime_DiffClamp(nextPostponeSleepTime, cur_time)) {
            rv = VPLTime_DiffClamp(nextPostponeSleepTime, cur_time);
        }
    }
    else {
        if(sleepDeferred) {
            sleepDeferred = false;
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "SLEEP: Allowing system to sleep. No active clients.");
        }
        if(stat_disconnect_called) {
            stat_disconnect_called = false;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                             "Disconnect successful: No active clients.");
        }
    }

    // If stats updates pending and not processing, start processing them.
    VPLMutex_Lock(&stats_mutex);

    if((nextDatasetStatUpdateTime == VPLTIME_INVALID ||
       cur_time > nextDatasetStatUpdateTime) &&
       !stat_updates_in_progress && !dataset_stat_updates.empty()) {
        nextDatasetStatUpdateTime = cur_time + VPLTime_FromSec(DATASET_STAT_UPDATE_INTERVAL_SEC);
        VPLCond_Signal(&stats_condvar);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                          "DATASET_STAT_UPDATE: Add dataset stat update task. "\
                          "Next add dataset stat update task will be called at "FMT_VPLTime_t".", 
                          nextDatasetStatUpdateTime);
    }
    
    if(!dataset_stat_updates.empty() && !stat_updates_in_progress &&
            rv > VPLTime_DiffClamp(nextDatasetStatUpdateTime, cur_time)) {
        rv = VPLTime_DiffClamp(nextDatasetStatUpdateTime, cur_time);
    }

    VPLMutex_Unlock(&stats_mutex);

    return rv;
}

int vss_server::getDataset(u64 uid, u64 did, dataset*& dataset_out)
{
    int rv = VSSI_SUCCESS;
    map<dataset_id, dataset*>::iterator it;
    dataset_id id(uid, did);
    
    dataset_out = NULL;

    // Check that the dataset is actually supposed to be here.
    u64 real_cluster_id = 0;
    rv = query.findDatasetStorage(uid, did, real_cluster_id);
    if (rv) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to determine storage host of dataset "FMTu64, did);
        goto fail;
    }
    if(clusterId != real_cluster_id) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" exists on cluster "FMTx64". This is cluster "FMTx64". Access denied.",
                          uid, did, real_cluster_id, clusterId);
        rv = VSSI_ACCESS;
        goto fail;
    }

    // Find or create the dataset requested.
    VPLMutex_Lock(&mutex);

    it = datasets.find(id);

    if(it != datasets.end()) {
        // Dataset found. Get reference.
        dataset_out = it->second;
    }

    if(dataset_out == NULL) {
        // Dataset doesn't exist. Make it.
        vplex::vsDirectory::DatasetDetail detail;
        rv = query.findDatasetDetail(uid, did, detail);
        if (rv) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to get details of dataset "FMTu64, did);
            goto exit;
        }
        if(detail.datasettype() == vplex::vsDirectory::FS) {
            dataset_out = new (std::nothrow) fs_dataset(id, detail.datasettype(), this);
        }
        else {
            dataset_out = new (std::nothrow) managed_dataset(id, detail.datasettype(), rootDir, this);
        }
        if (dataset_out == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "No memory for dataset obj");
            rv = VSSI_NOMEM;
            goto exit;
        }

        // grab a reference to the dataset so that it doesn't get torn
        // down when the client exits. Keep it around until shutdown to
        // reduce the overhead of activating the database, etc.
        dataset_out->reserve();

        // Dataset has been created and activated
        datasets[id] = dataset_out;
    }

    // If the dataset has an unrecoverable error return VSSI_FAILED
    // to indicate that the device needs attention of some sort.
    // in the case of the cloudnode this is likely customer support.
    if( dataset_out->get_state_failed() ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "dataset <"FMTu64":"FMTu64"> failed",
            uid, did);
        rv = VSSI_FAILED;
        dataset_out = NULL;
        goto exit;
    }
    else if ( dataset_out->is_invalid() ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "dataset <"FMTu64":"FMTu64"> invalid",
            uid, did);
        rv = VSSI_ACCESS;
        dataset_out = NULL;
        goto exit;
    }
    
    // Take requested reference. Can't fail.
    dataset_out->reserve();
 
 exit:
    VPLMutex_Unlock(&mutex);
 fail:
    return rv;
}

void vss_server::removeDataset(dataset_id id, dataset* old_dataset)
{
    map<dataset_id, dataset*>::iterator it;

    VPLMutex_Lock(&mutex);

    it = datasets.find(id);
    if(it->second == old_dataset && old_dataset->num_references() == 0) {
        delete it->second;
        datasets.erase(it);
    }

    VPLMutex_Unlock(&mutex);
}

vss_session* vss_server::get_session(u64 handle)
{
    vss_session* rv = NULL;

    VPLMutex_Lock(&mutex);
    if ( handle != sessionHandle ) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Not a self-session");
        rv = NULL;
    }
    else {
        rv = self_session;
        // This appears to get used for randomizing xid's.
        rv->last_updated = VPLTime_GetTimeStamp();
    }
    VPLMutex_Unlock(&mutex);

    return rv;
}

void vss_server::release_session(vss_session* session)
{
    // We only support self_sessions and we don't delete them.
}

// TODO: Server workers start here.

void requeueRequestHelper(void* vpcontext)
{
    vss_req_proc_ctx* context = (vss_req_proc_ctx*)(vpcontext);
    if ( context->server != NULL ) {
        context->server->processRequest(context, false);
    }
    else {
        context->client->server.processRequest(context, false);
    }
}

void vss_server::requeueRequest(vss_req_proc_ctx* context)
{
    addTask(requeueRequestHelper, context);
}

void vss_server::processRequest(vss_req_proc_ctx* context, bool do_verify)
{
    // Complete request verification
    if(do_verify) {
        context->client->verify_request(context);
    }
    if(context) {
        // Request verified. process it.
        handle_command(context);
    }
    // Else, request not verified.
}

void vss_server::notifyReqReceived(vss_req_proc_ctx* context)
{
    addTask(processRequestHelper, context);
}

void processRequestHelper(void* vpcontext)
{
    vss_req_proc_ctx* context = (vss_req_proc_ctx*)(vpcontext);
    if ( context->server != NULL ) {
        context->server->processRequest(context, false);
    }
    else {
        context->client->server.processRequest(context, true);
    }
}

void vss_server::send_client_response(char* resp, vss_req_proc_ctx* context)
{
    if(resp == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "NULL response!");
        goto cleanup;
    }

    vss_set_version(resp, vss_get_version(context->header));
    vss_set_xid(resp, vss_get_xid(context->header));
    vss_set_device_id(resp, vss_get_device_id(context->header));
    vss_set_handle(resp, vss_get_handle(context->header));

    if ( context->client ) {
        context->session->sign_reply(resp);
        context->client->put_response(resp);
    }
    else {
        vssts_srvr_put_response(context->vssts_id, resp);
    }

 cleanup:
    delete context;
}

void vss_server::get_test_media_file(std::string& testFile, int *type)
{
    testFile = this->rootDir;
    testFile.append(DEFAULT_TEST_FILENAME);
    *type = DEFAULT_MEDIA_TYPE;
}

void vss_server::getPortNumbers(VPLNet_port_t *clientPort, VPLNet_port_t *secureClearfiPort)
{
    VPLNet_port_t port;

    port = VPLSocket_GetPort(clientSocket);
    if (port == VPLNET_PORT_INVALID) {
        *clientPort = 0;
    } else {
        *clientPort = VPLNet_port_ntoh(port);
    }

    port = VPLSocket_GetPort(secureClearfiSocket);
    if (port == VPLNET_PORT_INVALID) {
        *secureClearfiPort = 0;
    } else {
        *secureClearfiPort = VPLNet_port_ntoh(port);
    }
}

void vss_server::add_dataset_stat_update(const dataset_id& id,
                                         u64 size, u64 version)
{
    VPLMutex_Lock(&stats_mutex);

    dataset_stat_updates[id] = make_pair(size, version);

    VPLMutex_Unlock(&stats_mutex);
}

VPLThread_return_t statsFunction(VPLThread_arg_t vpserver)
{
    vss_server* pserver = static_cast<vss_server*>(vpserver);
    pserver->process_stat_updates();
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

void vss_server::process_stat_updates()
{
    VPLMutex_Lock(&stats_mutex);

    while (running) {
        if (dataset_stat_updates.empty()) {
            VPLCond_TimedWait(&stats_condvar, &stats_mutex, VPL_TIMEOUT_NONE);
        } else {
            stat_updates_in_progress = true;

            while(!dataset_stat_updates.empty()) {
                map<dataset_id, pair<u64, u64> >::iterator it = dataset_stat_updates.begin();
                u64 uid, did, size, version;
                uid = it->first.uid;
                did = it->first.did;
                size = it->second.first;
                version = it->second.second;
                dataset_stat_updates.erase(it);

                VPLMutex_Unlock(&stats_mutex);

                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "Send out dataset update, datasetid "FMTu64", size "FMTu64", version "FMTu64, did, size, version);
                query.updateDatasetStats(uid, did, size, version);

                //scan all sessions
                VPLMutex_Lock(&mutex);
                {
                    std::vector<u32> handle_list;
                    vss_object * object = NULL;
                    self_session->find_object_handles(uid, did, handle_list);

                    for(int handle_idx = 0; handle_idx < handle_list.size(); handle_idx++) {
                        object = self_session->find_object(handle_list[handle_idx], false);
                        if(object != NULL && (object->get_notify_mask(NULL, 0) & VSSI_NOTIFY_DATASET_CHANGE_EVENT)) {
                            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "Send out notification, datasetid "FMTu64", size "FMTu64", version "FMTu64, did, size, version);
                            send_notify_event(object, VSSI_NOTIFY_DATASET_CHANGE_EVENT);
                        }
                        if(object != NULL)
                            self_session->release_object(object->get_handle());
                    }
                } 
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" update to size:"FMTu64" version:"FMTu64".",
                          uid, did, size, version);
        
                VPLMutex_Lock(&stats_mutex);
            }

            stat_updates_in_progress = false;
        }
    }

    VPLMutex_Unlock(&stats_mutex);
}

void vss_server::send_notify_event(vss_object * object, 
                                   VSSI_NotifyMask event_type, 
                                   char* data, 
                                   size_t data_length)
{
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "send notify event.");
    VPLMutex_Lock(&mutex);
    char* resp = NULL;
    vss_session * parent_session = NULL;
    char* resp_data = NULL;
    vss_client * client = NULL;
    u32 vssts_id = 0;

    if(object == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "object should not be null.");
        goto fail;
    }

    if(!(object->get_notify_mask(NULL, 0) & event_type)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "object doesn't set this event mask.");
        goto fail;
    }

    //check if client is still connected.
    client = object->get_client();
    vssts_id = object->get_vssts_id();
    if((client == NULL) && (vssts_id == 0)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "fail to find client");
        goto fail;
    }

    if ( client != NULL ) {
        parent_session = get_session(object->get_session_handle());
        if(parent_session == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "fail to find session.");
            goto fail;
        }
    }

    resp = (char*)calloc(VSS_HEADER_SIZE + 
                         VSS_NOTIFICATION_BASE_SIZE +
                         data_length, 1);
    resp_data = resp + VSS_HEADER_SIZE;
    if(resp) {
        vss_set_version(resp, 2);
        vss_set_command(resp, VSS_NOTIFICATION);
        vss_set_data_length(resp, VSS_NOTIFICATION_BASE_SIZE + data_length);
        vss_notification_set_handle(resp_data, object->get_handle());
        vss_notification_set_length(resp_data, data_length);
        vss_notification_set_mask(resp_data, event_type);
        vss_notification_set_data(resp_data, data);
        
        vss_set_xid(resp, 0);
        if ( client ) {
            vss_set_device_id(resp, client->device_id);
            vss_set_handle(resp, parent_session->get_session_handle());
            parent_session->sign_reply(resp);
            client->put_response(resp, false, true);
        }
        else {
            vssts_srvr_put_response(vssts_id, resp);
        }
        //delete resp;
        //resp = NULL;
    }

fail:
    VPLMutex_Unlock(&mutex);
    return;
}

bool vss_server::isTrustedNetwork()
{
    return query.isTrustedNetwork();
}

int vss_server::updateRemoteFileAccessControlDir(const u64& user_id,
                                                const u64& dataset_id,
                                                const ccd::RemoteFileAccessControlDirSpec &dir, 
                                                bool add)
{
using namespace HttpSvc::Sn::Handler_rf_Helper;

    int rv = 0;

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    AccessControlLocker acLocker;
    std::string target;
    if(dir.has_name())
        target = dir.name();
    else
        target = dir.dir();

    bool found = false;
    std::multimap<std::string,ccd::RemoteFileAccessControlDirSpec>::iterator it;
    {
        std::pair<std::multimap<std::string,ccd::RemoteFileAccessControlDirSpec>::iterator,std::multimap<std::string,ccd::RemoteFileAccessControlDirSpec>::iterator> range;
        range = acDirs.equal_range(target);
        for(it=range.first; it!=range.second; it++){
            if((it->second).dir() == dir.dir() &&
                    (it->second).name() == dir.name() &&
                    (it->second).is_allowed() == dir.is_allowed() &&
                    (it->second).is_user() == dir.is_user()){
                found = true;
                break;
            }
        }
    }

    if(!found){
        if(add)
            acDirs.insert(std::pair<std::string, ccd::RemoteFileAccessControlDirSpec>(target, dir));
        else{
            //remove not-exist item
            rv = CCD_ERROR_NOT_FOUND;
            goto end;
        }
    }else{
        if(!add)
            acDirs.erase(it);
        else{
            //add exist item
            rv = CCD_ERROR_ALREADY;
            goto end;
        }
    }

    dataset *ds;

    rv = getDataset(user_id, dataset_id, ds);
    if (rv) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to find dataset; dataset "FMTu64", rv %d", dataset_id, rv);
        goto end;
    }

    rv = HttpSvc::Sn::Handler_rf_Helper::updateRemoteFileAccessControlDir(ds, dir, add);
end:
#endif
    return rv;
}

int vss_server::getRemoteFileAccessControlDir(ccd::RemoteFileAccessControlDirs &rfaclDirs)
{
using namespace HttpSvc::Sn::Handler_rf_Helper;

    int rv = VPL_OK;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    AccessControlLocker acLocker;


    std::multimap<std::string,ccd::RemoteFileAccessControlDirSpec>::const_iterator it;

    for(it=acDirs.begin(); it!=acDirs.end(); it++){
        ccd::RemoteFileAccessControlDirSpec* newDir = rfaclDirs.add_dirs();
        *newDir = it->second;
    }
#endif
    return rv;
}

int vss_server::updateSyncboxArchiveStorageDataset(u64 datasetId)
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "updateSyncboxArchiveStorageDataset: dsetId("FMTu64")",
             datasetId);
    int rv = VPL_OK;
    if (!syncboxArchiveStorageEnabled) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "SyncboxArchiveStorage is not enabled yet."
                " Not permit to set dataset id:"FMTu64, datasetId);
        rv = VPL_ERR_PERM;
    } else {
        syncboxArchiveStorageDatasetId = datasetId;
    }
    return rv;
}

int vss_server::setSyncboxArchiveStorageParam(u64 datasetId, bool enable, const std::string& syncFolderPath)
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "setSyncboxArchiveStorageParam: dsetId("FMTu64"), %s, folder(%s)",
             datasetId, enable? "enable":"disable", syncFolderPath.c_str());
    if (enable) {
        syncboxArchiveStorageDatasetId = datasetId;
        syncboxSyncFeaturePath = syncFolderPath;
    } 
    syncboxArchiveStorageEnabled = enable;
    return 0;
} 

vss_req_proc_ctx::vss_req_proc_ctx(vss_client* client) :
    client(client),
    session(NULL),
    vssts_id(0),
    server(NULL),
    body(NULL)
{
}

vss_req_proc_ctx::~vss_req_proc_ctx()
{
    if(body) { free(body); }
    if(session && (client != NULL)) { client->server.release_session(session); }
}

int StorageNode_ListDatasetInfos(const char *root_path, std::list<vss_dataset_info_t>& dataset_infos)
{
    int rv = 0;
    VPLFS_dir_t dir;
    VPLFS_dirent_t entry;

    // Directory hierarchy is $root_path/<user_id>/<dataset_id>   

    rv = VPLFS_Opendir(root_path, &dir);
    if (rv != VPL_OK) {
        if (rv == VPL_ERR_NOENT) {
            // Top-level directory does not exist, no datasets
            rv = VPL_OK;
            goto exit;
        }
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to open root dir %s, rv=%d", root_path, rv);
        goto exit;
    }

    while ((rv = VPLFS_Readdir(&dir, &entry)) != VPL_ERR_MAX) {
        std::string user_path;
        VPLFS_dir_t user_dir;
        VPLFS_dirent_t user_entry;

        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to read root dir %s, rv=%d", root_path, rv);
            goto exit;
        }

        if (entry.type != VPLFS_TYPE_DIR) {
            // Unexpected, but just continue
            continue;
        }

        if (strncmp(entry.filename, ".", 1) == 0) {
            // Anything starting with a period is not an user ID
            continue;
        }

        user_path = root_path;
        user_path += "/";
        user_path += entry.filename;

        rv = VPLFS_Opendir(user_path.c_str(), &user_dir);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to open user dir %s, rv=%d", user_path.c_str(), rv);
            goto exit;
        }

        while ((rv = VPLFS_Readdir(&user_dir, &user_entry)) != VPL_ERR_MAX) {
            vss_dataset_info_t info;

            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to read user dir %s, rv=%d", user_path.c_str(), rv);
                goto exit;
            }

            if (user_entry.type != VPLFS_TYPE_DIR) {
                // Unexpected, but just continue
                continue;
            }

            if (strncmp(user_entry.filename, ".", 1) == 0) {
                // Anything starting with a period is not a dataset ID
                continue;
            }

            info.user_id = VPLConv_strToU64(entry.filename, NULL, 16);
            info.dataset_id = VPLConv_strToU64(user_entry.filename, NULL, 16);
            dataset_infos.push_back(info);
        }

        VPLFS_Closedir(&user_dir);
    }
    rv = VPL_OK;

    VPLFS_Closedir(&dir);

 exit:
    return rv;
}

