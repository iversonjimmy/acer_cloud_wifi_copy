//
//  Copyright (C) 2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

/// BroadOn Virtual Storage Unit Test - client side libbvs poll loop

#include "vpl_th.h"
#include "vpl_socket.h"
#include "vplex_socket.h"
#include "vplex_trace.h"
#include "vssi.h"
#include "vssi_error.h"
#include "wrapper_glue.hpp"

#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <ccdi_client_tcp.hpp>

#include <errno.h>
#include <fcntl.h>

#include <map>

extern int __debug_disable_route_mask;
using namespace std;

namespace vssts_wrapper {

typedef struct wg_cb_ctxt_s {
        VSSI_RouteInfo          route_info;
        VSSI_Session            session;
        void*                   ctx;
        VSSI_Callback           callback;
} wg_cb_ctxt_t;

static u64 vssi_app_attrs = 0;
static VPLThread_t poll_thread;
static int poll_thread_should_run = 1;
static bool thread_created = false;
static bool vssi_is_init = false;
static bool poll_thread_is_init = false;
static map<u64,VSSI_Session> session_map;
static VPLMutex_t mutex;

void do_vssi_cleanup(void)
{
    VPLThread_return_t dontcare;

    if ( VPL_IS_INITIALIZED(&mutex) ) {
        VPLMutex_Destroy(&mutex);
    }

    if ( !poll_thread_is_init ) {
        return;
    }

    // Remove all known sessions.
    { 
        map<u64,VSSI_Session>::iterator it;
        for( it = session_map.begin(); it != session_map.end(); it++ ) {
            (void)VSSI_EndSession(it->second);
        }
        session_map.clear();
    }
   
    poll_thread_should_run = 0;
    if ( thread_created ) {
        wakeup_vssi_poll_thread();
        VPLThread_Join(&poll_thread, &dontcare);
        thread_created = false;
    }

    if ( vssi_is_init ) {
        ::VSSI_Cleanup();
        vssi_is_init = false;
    }
}

/// Lifted from the VSS server
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

struct poll_info {
    VPLSocket_poll_t* pollInfo;
    size_t infoSize; // size of allocated pollInfo array
    size_t infoCount; // Number of slots in-use for pollInfo array
};

/// Helper function to build select set from libbvs.
static void vstest_add_vplsock(VPLSocket_t vpl_sock, int recv, int send, void* ctx)
{
    struct poll_info* poll_info = (struct poll_info*)(ctx);

    if(send || recv) {
        // grow polInfo if full
        if(poll_info->infoSize == poll_info->infoCount) {
            VPLSocket_poll_t* tmp = (VPLSocket_poll_t*)realloc(poll_info->pollInfo, 
                                                               sizeof(VPLSocket_poll_t) * poll_info->infoSize * 2);
            if(tmp == NULL) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0,
                                 "FAILED TO REALLOC POLL INFO!");
                return;
            }
            poll_info->pollInfo = tmp;
            poll_info->infoSize *= 2;
        }

        poll_info->pollInfo[poll_info->infoCount].socket = vpl_sock;
        poll_info->pollInfo[poll_info->infoCount].events = 0;
        poll_info->pollInfo[poll_info->infoCount].revents = 0;
        if(recv) {
            poll_info->pollInfo[poll_info->infoCount].events |= VPLSOCKET_POLL_RDNORM;
        }
        if(send) {
            poll_info->pollInfo[poll_info->infoCount].events |= VPLSOCKET_POLL_OUT;
        }
        poll_info->infoCount++;
    }

    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Added socket %d for %s.",
                        VAL_VPLSocket_t(vpl_sock),
                        (recv && send) ? "send and receive" :
                        (recv) ? "receive only" :
                        (send) ? "send only" :
                        "neither send not receive");
}

/// Helper functions so libbvs can determine active sockets
static int vstest_vplsock_recv_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv = 0;
    unsigned int i;
    struct poll_info* poll_info = (struct poll_info*)(ctx);

    for(i = 0; i < poll_info->infoCount; i++) {
        if(VPLSocket_Equal(poll_info->pollInfo[i].socket, vpl_sock)) {
            if(poll_info->pollInfo[i].revents & 
               (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_RDNORM)) {
                rv = 1;
            }
            break;
        }
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for receive.", 
                        VAL_VPLSocket_t(vpl_sock), rv ? "ready" : "not ready");
    return rv;
}

static int vstest_vplsock_send_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv = 0;
    struct poll_info* poll_info = (struct poll_info*)(ctx);
    unsigned int i;

    for(i = 0; i < poll_info->infoCount; i++) {
        if(VPLSocket_Equal(poll_info->pollInfo[i].socket, vpl_sock)) {
            if(poll_info->pollInfo[i].revents & 
               (VPLSOCKET_POLL_OUT)) {
                rv = 1;
            }
            break;
        }
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for send.", 
                        VAL_VPLSocket_t(vpl_sock), rv ? "ready" : "not ready");
    return rv;
}

static void reset_poll_info(struct poll_info& poll_info)
{
    if (poll_info.pollInfo != NULL && poll_info.infoSize > 0)
        free(poll_info.pollInfo);

    poll_info.pollInfo = (VPLSocket_poll_t*)calloc(sizeof(VPLSocket_poll_t), 10); // enough for normal operation?
    poll_info.infoSize = 10;
    poll_info.infoCount = 1;
    poll_info.pollInfo[0].socket = priv_socket_server_connected;
    poll_info.pollInfo[0].events = VPLSOCKET_POLL_RDNORM;
}

static VPLThread_return_t vssi_poll_thread(VPLThread_arg_t arg)
{
    struct poll_info poll_info = {NULL, 0, 0};

    int *e = (int *)arg;

    reset_poll_info(poll_info);

    while (*e) {
        int rv;
        char cmd = '\0';
        char wake_up = 0;
        int err = 0;

        // Determine how long to wait.
        VPLTime_t timeout = ::VSSI_HandleSocketsTimeout();

        VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                            "Waiting for activity "FMT_VPLTime_t"ms...",
                            timeout);

        rv = VPLSocket_Poll(poll_info.pollInfo,
                            poll_info.infoCount,
                            timeout);
        if (rv < 0) {
            switch(rv) {
            case VPL_ERR_AGAIN:
            case VPL_ERR_INVALID: // Sudden connection closure.
            case VPL_ERR_INTR: // Debugger interrupt.
            case VPL_ERR_NOTSOCK:
                // Bug 11879: cleanup poll_info.pollInfo except priv_socket_server_connected
                reset_poll_info(poll_info);
                break;
            default: // Something unexpected. Crashing!
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "Poll returns with error (%d):%s",
                    errno, strerror(errno));
                goto exit;
            }
        }

        // Let VSSI handle its sockets
        ::VSSI_HandleSockets(vstest_vplsock_recv_ready, 
                           vstest_vplsock_send_ready,
                           &poll_info);

        wake_up = 0;
        err = 0;
        if(poll_info.pollInfo[0].revents & VPLSOCKET_POLL_RDNORM) {
            cmd = '\0';
            int rc;
            do {
                VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                    "Reading net handle socket...");
                rc = VPLSocket_Recv(poll_info.pollInfo[0].socket, &cmd, 1);
                if(rc == -1) err = errno;
                VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                    "Read net handler socket. rc:%d(%s:%d), cmd:%c",
                                    rc, strerror(err), err, cmd);
                if(rc == 0) {
                    // no more messages at EOF.
                    break;
                }
                else if(rc == 1) {
                    // Got a message.
                    wake_up = 1;
                }
                else {
                    switch (rc) {
                    case VPL_ERR_AGAIN:
                    case VPL_ERR_INVALID: // Sudden connection closure.
                    case VPL_ERR_INTR: // Debugger interrupt.
                        break;
                    default: // Something unexpected. Crashing!
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Poll() error: %d.",
                                         rc);
                        break;
                    }

                }
            } while(rc == 1);
        }

        if(wake_up) {
            VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                "libbvs socket roster changed.");
            // Bug 11879: cleanup poll_info.pollInfo except priv_socket_server_connected
            reset_poll_info(poll_info);
            
            ::VSSI_ForEachSocket(vstest_add_vplsock, &poll_info);
        }
    }

exit:
    VPLSocket_Close(priv_socket_server_connected);
    VPLSocket_Close(priv_socket_client);
    VPLSocket_Close(priv_socket_server_listen);

    free(poll_info.pollInfo);

    return NULL;
}

void wakeup_vssi_poll_thread(void)
{
    int temp_rv = VPLSocket_Write(priv_socket_client, " ", 1, VPL_TIMEOUT_NONE);
    (void)temp_rv;
}

int do_vssi_setup(u64 app_id)
{
    int rv = 0;
    u64 device_id = 0;
    u64 vssi_app_id;
    
    if (vssi_is_init) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "VSSI wrapper already initialized");
        return rv;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "VSSI_Init() app_id is "FMTu64, app_id);

    vssi_app_attrs = (app_id & VSSI_APP_ATTRIBUTE_MASK) >> VSSI_APP_ATTRIBUTE_SHIFT;
    vssi_app_id = (app_id & ~VSSI_APP_ATTRIBUTE_MASK) << 40;
    VPL_SET_UNINITIALIZED(&mutex);

    {
        ccd::GetSystemStateInput gssInput;
        ccd::GetSystemStateOutput gssOutput;

        gssInput.set_get_device_id(true);
        rv = CCDIGetSystemState(gssInput, gssOutput);
        if ( rv != CCD_OK ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:VPLMutex_init() %d", rv);
            rv = VSSI_NOMEM;
            goto fail;
        }
        device_id = gssOutput.device_id() | vssi_app_id;
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "device id: "FMTu64", app id: "FMTu64, device_id, vssi_app_id);
    }

    rv = VPLMutex_Init(&mutex);
    if ( rv != VPL_OK ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:VPLMutex_init() %d", rv);
        rv = VSSI_NOMEM;
        goto fail;
    }

    // Create the local socket connection for waking up the thread
    rv = create_private_sockets(priv_socket_server_listen,
                                priv_socket_client,
                                priv_socket_server_connected);
    if(rv != 0) {
        rv = VSSI_NOMEM;
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Initialization of libvssi poll thread.");
        goto fail;
    }
    poll_thread_is_init = true;
    poll_thread_should_run = 1;

    rv = VPLThread_Create(&poll_thread,
                          vssi_poll_thread,
                          &poll_thread_should_run,
                          0, // default VPLThread thread-attributes: priority, stack-size, etc.
                          "libvsstsi_wrapper_poll_thread");
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "creating libvssi_poll_thread failed!");
        rv = VSSI_NOMEM;
        goto fail;
    }
    thread_created = true;

    rv = ::VSSI_Init(device_id, wakeup_vssi_poll_thread);
    if( rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "VSSI_init() failed! - %d", rv);
        goto fail;
    }
    vssi_is_init = true;

fail:
    if ( rv != VSSI_SUCCESS ) {
        do_vssi_cleanup();

        VPLSocket_Close(priv_socket_server_connected);
        VPLSocket_Close(priv_socket_client);
        VPLSocket_Close(priv_socket_server_listen);
    }

    return rv;
}

static bool lookup_device_id(u64 user_id, u64 dataset_id, u64& device_id)
{
    int rv;
    bool is_found = false;
    ccd::ListOwnedDatasetsInput listOdIn;
    ccd::ListOwnedDatasetsOutput listOdOut;

    listOdIn.set_user_id(user_id);
    // XXX - Is this correct?
    listOdIn.set_only_use_cache(true);

    rv = CCDIListOwnedDatasets(listOdIn, listOdOut);
    if ( rv != 0 ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "CCDIListOwnedDatasets() - %d", rv);
        goto exit;
    }

    for( int i = 0 ; i < listOdOut.dataset_details_size() ; i++ ) {
        if ( listOdOut.dataset_details(i).datasetid() != dataset_id ) {
            continue;
        }
        is_found = true;
        device_id = listOdOut.dataset_details(i).clusterid();
    }

    if ( !is_found ) {
        rv = -1;
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "dataset "FMTu64 " not found",
            dataset_id);
        goto exit;
    }

exit:
    return is_found;
}

static void strip_route_info(VSSI_RouteInfo& route_info, int mask)
{
    // strip all particular routing type
    int num_routes = route_info.num_routes;
    for( int i = 0, j = 0 ; i < num_routes ; i++ ) {
        int type = route_info.routes[i].type;
        if ( ((mask & 1) && type == VSSI_ROUTE_DIRECT_INTERNAL) ||
             ((mask & (1 << 1)) && type == VSSI_ROUTE_DIRECT_EXTERNAL) ||
             ((mask & (1 << 2)) && type == VSSI_ROUTE_PROXY)
                ) {
            // bad, reap
            route_info.num_routes--;
            delete[] route_info.routes[i].server;
            route_info.routes[i].server = NULL;
        } else {
            // good, copy
            if( i != j ) {
                memcpy(&route_info.routes[j], &route_info.routes[i],
                        sizeof(VSSI_Route));
            }
            j++;
        }
    }
}

static VSSI_Result get_routes(u64 user_id, u64 dataset_id, wg_cb_ctxt_t* ctxt)
{
    int rv = 0;
    u64 device_id;
    VSSI_RouteInfo* route_info;

    route_info = &ctxt->route_info;
    route_info->num_routes = 0;
    route_info->routes = NULL;

    // figure out what device this dataset is on.
    if ( lookup_device_id(user_id, dataset_id, device_id) == false ) {
        rv = VSSI_NOTFOUND;
        goto exit;
    }

    // pull out route / session info this device
    {
        bool is_found = false;
        int i;
        ccd::ListUserStorageOutput listSnOut;
        ccd::ListUserStorageInput listSnIn;

        listSnIn.set_user_id(user_id);
        listSnIn.set_only_use_cache(true);
        rv = CCDIListUserStorage(listSnIn, listSnOut);
        if(rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "CCDIListUserStorage() - %d", rv);
            rv = VSSI_NOTFOUND;
            goto exit;
        }

        rv = -1;
        for (i = 0; i < listSnOut.user_storage_size(); i++) {
            if ( listSnOut.user_storage(i).storageclusterid() == device_id ) {
                is_found = true;
                break;
            }
        }

        if ( !is_found ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "device "FMTu64 " not found",
                device_id);
            rv = VSSI_NOTFOUND;
            goto exit;
        }

        // Find or create a VSSI_Session to this storage
        {
            u64 handle = listSnOut.user_storage(i).accesshandle();
            map<u64,VSSI_Session>::iterator it;

            VPLMutex_Lock(&mutex);
            it = session_map.find(handle);
            if ( it != session_map.end() ) {
                ctxt->session = it->second;
            }
            else {
                ctxt->session = ::VSSI_RegisterSession(handle,
                    listSnOut.user_storage(i).devspecaccessticket().data());
                session_map[handle] = ctxt->session;
            }
            VPLMutex_Unlock(&mutex);
        }

        // Now, piece together the routes for this beast.
        if (vssi_app_attrs & VSSI_APP_ATTRIBUTE_LOOPBACK_ONLY) {
            // These are local Orbe apps, and therefore should only connect to the Orbe
            // using the loopback IP address
            route_info->num_routes = 1;
            route_info->routes = new VSSI_Route;
            memset(route_info->routes, 0, sizeof(VSSI_Route));

            // Find the internal direct route from VSDS
            bool route_found = false;
            for( int j = 0 ; j < listSnOut.user_storage(i).storageaccess_size() ; j++ ) {
                if ( listSnOut.user_storage(i).storageaccess(j).routetype() !=
                        vplex::vsDirectory::DIRECT_INTERNAL ) {
                    continue;
                }

                bool port_found = false;
                for( int k = 0 ; k < listSnOut.user_storage(i).storageaccess(j).ports_size() ; k++ ) {
                    if ( listSnOut.user_storage(i).storageaccess(j).ports(k).porttype() != 
                            vplex::vsDirectory::PORT_VSSI ) {
                        continue;
                    }
                    if ( listSnOut.user_storage(i).storageaccess(j).ports(k).port() == 0 ) {
                        continue;
                    }
                    route_info->routes[0].port =
                        listSnOut.user_storage(i).storageaccess(j).ports(k).port();
                    port_found = true;
                    break;
                }
                if ( !port_found ) {
                    continue;
                }

                size_t len = sizeof("127.0.0.1");
                route_info->routes[0].server = new char[len];
                memcpy(route_info->routes[0].server, "127.0.0.1", len);
                route_info->routes[0].type = vplex::vsDirectory::DIRECT_INTERNAL;
                route_info->routes[0].proto = listSnOut.user_storage(i).storageaccess(j).protocol();
                route_info->routes[0].cluster_id = listSnOut.user_storage(i).storageclusterid();
                route_found = true;
                break;
            }
            if ( !route_found ) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "No VS routes found.");
                rv = VSSI_NOTFOUND;
                goto exit;
            }
            rv = VSSI_SUCCESS;
        } else {
            int route_ind = 0;
            int num_routes;
            //local route info
            ccd::ListLanDevicesInput listLanDevIn;
            ccd::ListLanDevicesOutput listLanDevOut;
            listLanDevIn.set_user_id(user_id);
            // XXX - Why are we requesting these strange states?
            listLanDevIn.set_include_unregistered(true);
            listLanDevIn.set_include_registered_but_not_linked(true);
            listLanDevIn.set_include_linked(true);
            rv = CCDIListLanDevices(listLanDevIn, listLanDevOut);
            if(rv != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "List Lan Devices result %d", rv);
                rv = VSSI_NOTFOUND;
                goto exit;
            }

            num_routes = listSnOut.user_storage(i).storageaccess_size() +
                listLanDevOut.infos_size();

            route_info->num_routes = 0;
            route_info->routes = new VSSI_Route[num_routes];
            memset(route_info->routes, 0, sizeof(VSSI_Route)*num_routes);

            // build up the list from the locally found devices
            for (int j = 0; j < listLanDevOut.infos_size(); j++) {
                size_t len;

                if ( listLanDevOut.infos(j).device_id() != device_id ) {
                    continue;
                }

                len = listLanDevOut.infos(j).route_info().ip_v4_address().size() + 1;
                route_info->routes[route_ind].server = new char[len];
                memcpy(route_info->routes[route_ind].server,
                       listLanDevOut.infos(j).route_info().ip_v4_address().c_str(),
                       len);
                route_info->routes[route_ind].port =
                    listLanDevOut.infos(j).route_info().virtual_drive_port();
                route_info->routes[route_ind].type =
                    vplex::vsDirectory::DIRECT_INTERNAL;
                route_info->routes[route_ind].proto =  vplex::vsDirectory::VS;
                route_info->routes[route_ind].cluster_id = device_id;
                route_ind++;
            }

            // now build up the list with info from VSDS.
            for( int j = 0 ; j < listSnOut.user_storage(i).storageaccess_size() ; j++ ) {
                bool port_found = false;
                for( int k = 0 ; k < listSnOut.user_storage(i).storageaccess(j).ports_size() ; k++ ) {
                    if ( listSnOut.user_storage(i).storageaccess(j).ports(k).porttype() != 
                            vplex::vsDirectory::PORT_VSSI ) {
                        continue;
                    }
                    if ( listSnOut.user_storage(i).storageaccess(j).ports(k).port() == 0 ) {
                        continue;
                    }
                    route_info->routes[route_ind].port =
                        listSnOut.user_storage(i).storageaccess(j).ports(k).port();
                    port_found = true;
                    break;
                }
                if ( !port_found ) {
                    continue;
                }
                size_t len = listSnOut.user_storage(i).storageaccess(j).server().size() + 1;
                route_info->routes[route_ind].server = new char[len];
                memcpy(route_info->routes[route_ind].server,
                       listSnOut.user_storage(i).storageaccess(j).server().c_str(),
                       len);
                route_info->routes[route_ind].type = listSnOut.user_storage(i).storageaccess(j).routetype();
                route_info->routes[route_ind].proto = listSnOut.user_storage(i).storageaccess(j).protocol();
                route_info->routes[route_ind].cluster_id = listSnOut.user_storage(i).storageclusterid();
                route_ind++;
            }
            if ( route_ind == 0 ) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "No VS routes found.");
                rv = VSSI_NOTFOUND;
                goto exit;
            }
            route_info->num_routes = route_ind;

            // DEBUG: Strip out particular routing type
            if(__debug_disable_route_mask) {
                strip_route_info(*route_info, __debug_disable_route_mask);
            }
        }

        for (int ind = 0; ind < route_info->num_routes; ind++) {
            VPLTRACE_LOG_INFO(TRACE_APP, 0, "route %d: server - %s, port - %hd, type - %d, proto - %d, cluster_id - "FMTu64,
                              ind,
                              route_info->routes[ind].server,
                              route_info->routes[ind].port,
                              route_info->routes[ind].type,
                              route_info->routes[ind].proto,
                              route_info->routes[ind].cluster_id);
        }
    }

exit:
    return rv;
}

static void wg_callback(void* ctx, VSSI_Result result)
{
    wg_cb_ctxt_t* ctxt = (wg_cb_ctxt_t*)ctx;

    (ctxt->callback)(ctxt->ctx, result);

    // Now, tear down route info, etc.
    if ( ctxt->route_info.routes != NULL ) {
        for(int i = 0; i < ctxt->route_info.num_routes; i++) {
            delete [] ctxt->route_info.routes[i].server;
        }
        delete [] ctxt->route_info.routes;
    }

    // free up the context
    delete ctxt;
}

void do_vssi_delete2(u64 user_id,
                     u64 dataset_id,
                     void* ctx,
                     VSSI_Callback callback)
{
    int rv = 0; // pass until failed.
    wg_cb_ctxt_t* ctxt = new wg_cb_ctxt_t;

    ctxt->ctx = ctx;
    ctxt->callback = callback;

    rv = get_routes(user_id, dataset_id, ctxt);
    if ( rv != VSSI_SUCCESS ) {
        goto exit;
    }

    VSSI_Delete2(ctxt->session, user_id, dataset_id, &ctxt->route_info,
                     ctxt, wg_callback);

exit:
    if ( rv != 0 ) {
        // We're just making up the error code...
        wg_callback(ctxt, VSSI_NOMEM);
    }

}

void do_vssi_open_object2(u64 user_id,
                          u64 dataset_id,
                          u8 mode,
                          VSSI_Object* handle,
                          void* ctx,
                          VSSI_Callback callback)
{
    int rv = 0; // pass until failed.
    wg_cb_ctxt_t* ctxt = new wg_cb_ctxt_t; // freed at end of wg_callback

    ctxt->ctx = ctx;
    ctxt->callback = callback;

    rv = get_routes(user_id, dataset_id, ctxt);
    if ( rv != VSSI_SUCCESS ) {
        goto exit;
    }

    VSSI_OpenObject2(ctxt->session, user_id, dataset_id, &ctxt->route_info,
                     VSSI_READWRITE | VSSI_FORCE, handle,
                     ctxt, wg_callback);

exit:
    if ( rv != 0 ) {
        // We're just making up the error code...
        wg_callback(ctxt, VSSI_NOMEM);
    }
}

bool dataset_is_new_vssi(u64 user_id, u64 dataset_id)
{
    bool is_new = false;
    u64 device_id;

    // figure out what device this dataset is on.
    if ( lookup_device_id(user_id, dataset_id, device_id) == false ) {
        goto done;
    }

    // Now, find out the protocol for this device.
    {
        int rv;
        ccd::ListLinkedDevicesInput listLdIn;
        ccd::ListLinkedDevicesOutput listLdOut;

        listLdIn.set_user_id(user_id);
        listLdIn.set_only_use_cache(true);

        rv = CCDIListLinkedDevices(listLdIn, listLdOut);
        if ( rv != 0 ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "CCDIListLinkedDevices() - %d", rv);
            goto done;
        }

        for( int i = 0 ; i < listLdOut.devices_size() ; i++ ) {
            if ( listLdOut.devices(i).device_id() != device_id ) {
                continue;
            }

            // It is not sufficient that TS2 is supported by the target CCD (i.e. protocol_version 4)
            // The vssts server on the target CCD also needs to be supported.
            if ( listLdOut.devices(i).has_protocol_version() &&
                 atoi(listLdOut.devices(i).protocol_version().c_str()) >= 5 ) {
                is_new = true;
            }
            break;
        }
    }
    
done:
    return is_new;
}

}
