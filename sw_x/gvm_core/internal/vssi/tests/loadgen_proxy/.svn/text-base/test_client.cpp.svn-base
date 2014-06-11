/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD TECHNOLOGY INC.
 *
 */

#include "test_client.hpp"

#include "vpl_conv.h"
#include "vplex_trace.h"
#include "vplex_socket.h"

#include "vss_comm.h"
#include "vssi_error.h"
#include "loadgen_proxy.hpp"

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

#include <iostream>

using namespace std;

extern u64 testDeviceId;
extern string psnServiceTicket;
extern u64 psnSessionHandle;
extern string infra_domain;
extern char encryption_key[CSL_AES_KEYSIZE_BYTES];
extern char signing_key[CSL_SHA1_DIGESTSIZE];

test_client::test_client(const char* header, const char* request) :
    socket(VPLSOCKET_INVALID),
    fail_line(0),
    fail_code(0)
{
    // collect raw data for proxy/P2P attempt.
    
    version = vss_get_version(header);
    xid = vss_get_xid(header);
    device_id = vss_get_device_id(header);
    handle = vss_get_handle(header);
    proxy_cookie = vss_proxy_connect_get_cookie(request);
    client_ip = vss_proxy_connect_get_client_ip(request);
    client_port = vss_proxy_connect_get_client_port(request);
    server_port = vss_proxy_connect_get_server_port(request);
    server_addr.assign(vss_proxy_connect_get_server_addr(request),
                       vss_proxy_connect_get_addrlen(request));
    proxy_type = vss_proxy_connect_get_type(request);
}

test_client::~test_client()
{
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Destroyed test client for socket "FMT_VPLSocket_t" Bail at line %d, error %d.",
                        VAL_VPLSocket_t(socket),
                        fail_line, fail_code);
                        
    if(!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
    }
}

int test_client::start()
{
    VPLThread_attr_t attrs;
    VPLThread_AttrInit(&attrs);
    VPLThread_AttrSetDetachState(&attrs, true);
    VPLThread_AttrSetStackSize(&attrs, 1024*64);
    int rv = VPLDetachableThread_Create(NULL, test_client_main, (VPLThread_arg_t)(this), 
                                        &attrs, "test_client");
    VPLThread_AttrDestroy(&attrs);

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Start test client activity. result:%d",
                        rv);

    return rv;
}

VPLTHREAD_FN_DECL test_client_main(VPLThread_arg_t vpclient)
{
    test_client* client = (test_client*)(vpclient);

    client->launch();
    delete client;

    return VPLTHREAD_RETURN_VALUE;
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
    
    sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, false);
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

void test_client::launch()
{
    u64 query_len;
    u64 response_len;
    enum {
        RECV_START,
        RECV_END,
        RECV_WAIT_TOTAL,        
        SEND_START,
        SEND_END,
        SEND_WAIT_TOTAL,
        NUM_TIMESTAMPS
    };
    VPLTime_t times[NUM_TIMESTAMPS] = {0};
    u64 value;
    const int buflen = 16 * 1024;
    char* buffer = new char[buflen];
    u64 progress = 0;
    VPLSocket_poll_t pollInfo[2];
    VPLSocket_t p2p_sockets[2]; // listen and connect sockets
    int rc;

    // Connect to server
    string address = server_addr + "." + infra_domain;
    open_client_socket(address, server_port,
                       client_port == 0 ? false : true, socket);
    if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Connect-back to server %s:%u failed.",
                            address.c_str(), server_port);
        goto exit;
    }


    // Compose proxy connect response.
    vss_set_version(buffer, version);
    vss_set_command(buffer, VSS_PROXY_CONNECT_REPLY);
    vss_set_status(buffer, 0);
    vss_set_xid(buffer, xid);
    vss_set_device_id(buffer, device_id);
    vss_set_handle(buffer, handle);
    vss_set_data_length(buffer, VSS_PROXY_CONNECTR_SIZE);
    vss_proxy_connect_reply_set_cluster_id(buffer + VSS_HEADER_SIZE, testDeviceId);
    vss_proxy_connect_reply_set_cookie(buffer + VSS_HEADER_SIZE, proxy_cookie);
    vss_proxy_connect_reply_set_port(buffer + VSS_HEADER_SIZE, client_port);
    vss_proxy_connect_reply_set_type(buffer + VSS_HEADER_SIZE, proxy_type);
    // Sign, encrypt reply
    sign_vss_msg(buffer, encryption_key, signing_key);

    // Send response to server.
    rc = VPLSocket_Send(socket, buffer, VSS_HEADER_SIZE + vss_get_data_length(buffer));
    if(rc != VSS_HEADER_SIZE + vss_get_data_length(buffer)) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Send proxy connect msg failed:%d.",
                            rc);
        goto exit;
    }

    // If P2P requested, open P2P sockets and try to get P2P connection. Drop server connection if P2P made.
    if(client_port != 0) {
        VPLSocket_addr_t client_addr;
        VPLSocket_addr_t origin_addr;
        int yes = 1;
        client_addr.family = VPL_PF_INET;
        client_addr.addr = client_ip;
        client_addr.port = VPLNet_port_hton(client_port);
        origin_addr.family = VPL_PF_INET;
        origin_addr.addr = VPLSocket_GetAddr(socket);
        origin_addr.port = VPLSocket_GetPort(socket);

        p2p_sockets[0] = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
        if(VPLSocket_Equal(p2p_sockets[0], VPLSOCKET_INVALID)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to open P2P socket.");
            goto exit;
        }    
        p2p_sockets[1] = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
        if(VPLSocket_Equal(p2p_sockets[1], VPLSOCKET_INVALID)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to open P2P socket.");
            goto exit;
        }

        // Must use SO_REUSEADDR option for each. 
        rc = VPLSocket_SetSockOpt(p2p_sockets[0], VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                                  (void*)&yes, sizeof(yes));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Failed (%d) to set SO_REUSEADDR for socket.",
                              rc);
            goto exit;
        }
        rc = VPLSocket_SetSockOpt(p2p_sockets[1], VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                                  (void*)&yes, sizeof(yes));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Failed (%d) to set SO_REUSEADDR for socket.",
                              rc);
            goto exit;
        }

        /* Set TCP no delay for performance reasons. */
        VPLSocket_SetSockOpt(p2p_sockets[0], VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                             (void*)&yes, sizeof(yes));
        VPLSocket_SetSockOpt(p2p_sockets[1], VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                             (void*)&yes, sizeof(yes));

        // Bind each to origin address
        rc = VPLSocket_Bind(p2p_sockets[0], &origin_addr, sizeof(origin_addr));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to bind p2p socket to "FMT_VPLNet_addr_t":%u. socket:"FMT_VPLSocket_t", error:%d",
                             VAL_VPLNet_addr_t(origin_addr.addr),
                             VPLNet_port_ntoh(origin_addr.port),
                             VAL_VPLSocket_t(p2p_sockets[0]), rc);
            goto exit;
        }
        rc = VPLSocket_Bind(p2p_sockets[1], &origin_addr, sizeof(origin_addr));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to bind p2p socket to "FMT_VPLNet_addr_t":%u. socket:"FMT_VPLSocket_t", error:%d",
                             VAL_VPLNet_addr_t(origin_addr.addr),
                             VPLNet_port_ntoh(origin_addr.port),
                             VAL_VPLSocket_t(p2p_sockets[1]), rc);
            goto exit;
        }

        // "Connect" to the client, punching the local firewall.
        rc = VPLSocket_ConnectNowait(p2p_sockets[1], &client_addr, sizeof(client_addr));
        if(rc == VPL_ERR_BUSY || rc == VPL_ERR_AGAIN) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Awaiting connection to server "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                              VAL_VPLNet_addr_t(client_addr.addr),
                              VPLNet_port_ntoh(client_addr.port),
                              VAL_VPLSocket_t(p2p_sockets[1]));
            
        }
        else if(rc != VPL_OK) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Expected failure (%d) to connect to server "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                              rc,
                              VAL_VPLNet_addr_t(client_addr.addr),
                              VPLNet_port_ntoh(client_addr.port),
                              VAL_VPLSocket_t(p2p_sockets[1]));
        }
        else {
            VPLSocket_Close(socket);
            socket = p2p_sockets[1];
            VPLSocket_Close(p2p_sockets[0]);
            // P2P is done.
            goto authenticate;
        }
        
        // Listen on the socket.
        // expecting only a single connection
        rc = VPLSocket_Listen(p2p_sockets[0], 1);        
        if (rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to listen on p2p socket: %d.", rc);
            goto exit;
        }

        // Poll the listening and punching sockets until both are made invalid or one connects.
        pollInfo[0].socket = p2p_sockets[0];
        pollInfo[0].events = VPLSOCKET_POLL_RDNORM;
        pollInfo[1].socket = p2p_sockets[1];
        pollInfo[1].events = VPLSOCKET_POLL_OUT;
        
        do {
            rc = VPLSocket_Poll(pollInfo, 
                                VPLSocket_Equal(p2p_sockets[1], VPLSOCKET_INVALID) ? 1 : 2,
                                VPL_TIMEOUT_NONE);
            if(pollInfo[0].revents) {
                VPLSocket_addr_t addr;
                VPLSocket_Close(socket);
                rc = VPLSocket_Accept(p2p_sockets[0], &addr, sizeof(addr), &socket);
                VPLSocket_Close(p2p_sockets[0]);
                if(!VPLSocket_Equal(p2p_sockets[1], VPLSOCKET_INVALID)) {
                    VPLSocket_Close(p2p_sockets[1]);
                }

                if(rc != VPL_OK) {
                    // P2P connect failed.
                    goto exit;
                }
                else {
                    // Authenticate P2P now.
                    break;
                }
            }
            if(!VPLSocket_Equal(p2p_sockets[1], VPLSOCKET_INVALID) && pollInfo[1].revents) {
                int so_err = 0;
                rc = VPLSocket_GetSockOpt(p2p_sockets[1],
                                          VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_ERROR,
                                          &so_err, sizeof(so_err));
                if(rc != VPL_OK) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Get SO_ERROR for punch socket failed:%d.",
                                      rc); 
                    so_err = -1;
                }
                
                if(so_err == 0) {
                    // punch socket got through
                    VPLSocket_Close(socket);
                    VPLSocket_Close(p2p_sockets[0]);
                    socket = p2p_sockets[1];
                    p2p_sockets[0] = VPLSOCKET_INVALID;
                    // Authenticate P2P now.
                    break;
                }
                else {
                    // Punch has failed. Stop checking the socket.
                    VPLSocket_Close(p2p_sockets[1]);
                    p2p_sockets[1] = VPLSOCKET_INVALID;
                }
            }
        } while(1);

    authenticate:
        // authenticate p2p connection: receive authenticate request and send authenticate response
        {
            char auth_req[VSS_HEADER_SIZE];
            char tmp[32];
            char auth_resp[VSS_HEADER_SIZE];
            
            rc = VPLSocket_Recv(socket, auth_req, VSS_HEADER_SIZE);
            if(rc != VSS_HEADER_SIZE) {
                goto exit;
            }
            rc = VPLSocket_Recv(socket, tmp, vss_get_data_length(auth_req));
            if(rc != vss_get_data_length(auth_req)) {
                goto exit;
            }
            
            vss_set_version(auth_resp, vss_get_version(auth_req));
            vss_set_command(auth_resp, VSS_AUTHENTICATE_REPLY);
            vss_set_status(auth_resp, VSSI_SUCCESS);
            vss_set_xid(auth_resp, vss_get_xid(auth_req));
            vss_set_device_id(auth_resp, vss_get_device_id(auth_req));
            vss_set_handle(auth_resp, vss_get_handle(auth_req));
            vss_set_data_length(auth_resp, 0);
            sign_vss_msg(auth_resp, encryption_key, signing_key);

            rc = VPLSocket_Send(socket, auth_resp, VSS_HEADER_SIZE);
            if(rc != VSS_HEADER_SIZE) {
                goto exit;
            }
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "P2P connection established.");
        }
    }

    while(1) { // until client closes socket

        pollInfo[0].socket = socket;
        pollInfo[0].events = VPLSOCKET_POLL_RDNORM;
        
        // Receive activity query 
        // Wait for ready before each receive. Don't care about result.
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc;
            goto exit;
        }
        if(sizeof(value) != (rc = VPLSocket_Recv(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc;
            goto exit;
        }
        query_len = VPLConv_ntoh_u64(value);
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit; 
        }
        if(sizeof(value) != (rc = VPLSocket_Recv(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        response_len = VPLConv_ntoh_u64(value);
        
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Activity request on socket "FMT_VPLSocket_t" for "FMTu64" bytes receive, "FMTu64" bytes send.",
                          VAL_VPLSocket_t(socket), query_len, response_len);
        
        // Receive upload data
        times[RECV_START] = VPLTime_GetTime();
        times[RECV_WAIT_TOTAL] = 0;
        progress = 0;
        while(progress < query_len) {
            VPLTime_t wait_start = VPLTime_GetTime();
            rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE);
            times[RECV_WAIT_TOTAL] += (VPLTime_GetTime() - wait_start);
            
            if(pollInfo[0].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP)) {
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "FAIL: Socket error: %d.", pollInfo[0].revents);
                goto exit;
            }
            
            switch(rc) {
            case 1:
                rc = VPLSocket_Recv(socket, buffer, 
                                    (buflen < (query_len - progress)) ? 
                                    buflen : query_len - progress);
                if(rc > 0) {
                    progress += rc;
                }
                else if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
                    // Just try again.
                }
                else {
                    // Broken.
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "FAIL: Socket error on recv: %d.", rc);                    
                    fail_line = __LINE__; fail_code = rc; 
                    goto exit;
                }
                break;
            case 0: // Timeout. Huh?
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR:
                // Just try again.
                break;
            case VPL_ERR_INVALID:
            default:
                // Broken.
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "FAIL: Socket error on poll: %d.", rc);
                fail_line = __LINE__; fail_code = rc; 
                goto exit;
                break;
            }
        }
        times[RECV_END] = VPLTime_GetTime();
        
        // Send query receive measurements
        pollInfo[0].socket = socket;
        pollInfo[0].events = VPLSOCKET_POLL_OUT;
        
        value = VPLConv_hton_u64(times[RECV_START]);
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        if(sizeof(value) != (rc = VPLSocket_Send(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        value = VPLConv_hton_u64(times[RECV_END]);
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        if(sizeof(value) != (rc = VPLSocket_Send(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        value = VPLConv_hton_u64(times[RECV_WAIT_TOTAL]);
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        if(sizeof(value) != (rc = VPLSocket_Send(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        
        // Send response data
        times[SEND_START] = VPLTime_GetTime();
        times[SEND_WAIT_TOTAL] = 0;
        progress = 0;
        while(progress < response_len) {
            VPLTime_t wait_start = VPLTime_GetTime();
            rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE);
            times[SEND_WAIT_TOTAL] += (VPLTime_GetTime() - wait_start);
            
            switch(rc) {
            case 1:
                if(pollInfo[0].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP)) {
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "FAIL: Socket error: %d.", pollInfo[0].revents);
                    fail_line = __LINE__; fail_code = rc; 
                    goto exit;
                }
                
                rc = VPLSocket_Send(socket, buffer, 
                                    (buflen < (response_len - progress)) ? 
                                buflen : response_len - progress);
                if(rc > 0) {
                    progress += rc;
                }
                else if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
                // Just try again.
                }
                else {
                    // Broken.
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "FAIL: Socket error on send: %d.", rc);
                    fail_line = __LINE__; fail_code = rc;                     
                    goto exit;
                }
                break;
            case 0: // Timeout. Huh?
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR:
                // Just try again.
                break;
            case VPL_ERR_INVALID:
            default:
                // Broken.
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "FAIL: Socket error on poll: %d.", rc);
                fail_line = __LINE__; fail_code = rc; 
                goto exit;
                break;
            }
        }
        times[SEND_END] = VPLTime_GetTime();
        
        // Send response send measurements
        value = VPLConv_hton_u64(times[SEND_START]);
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        if(sizeof(value) != (rc = VPLSocket_Send(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        value = VPLConv_hton_u64(times[SEND_END]);
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        if(sizeof(value) != (rc = VPLSocket_Send(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        value = VPLConv_hton_u64(times[SEND_WAIT_TOTAL]);
        if(1 != (rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
        if(sizeof(value) != (rc = VPLSocket_Send(socket, &value, sizeof(value)))) { 
            fail_line = __LINE__; fail_code = rc; 
            goto exit;
        }
    }

 exit:
    delete [] buffer;
    
    return;
}
