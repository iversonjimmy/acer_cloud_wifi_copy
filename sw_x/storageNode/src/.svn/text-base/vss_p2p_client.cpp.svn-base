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

#include "vss_p2p_client.hpp"

#include <iostream>

#include "vpl_conv.h"
#include "vplex_trace.h"
#include "vpl_socket.h"
#include "vplex_socket.h"

#include "vss_comm.h"

#include "vss_cmdproc.hpp"

#include "vss_client.hpp"
#include "strm_http.hpp"

using namespace std;

vss_p2p_client::vss_p2p_client(vss_server& server,
                               vss_session* session,
                               u8 proxy_client_type,
                               u64 client_device_id,
                               VPLSocket_addr_t& origin_addr,
                               VPLSocket_addr_t& client_addr) :
    punch_sockfd(VPLSOCKET_INVALID), 
    listen_sockfd(VPLSOCKET_INVALID), 
    sockfd(VPLSOCKET_INVALID), 
    punching(false),
    listening(false),
    receiving(false),
    connected(false),
    server(server),
    session(session),
    proxy_client_type(proxy_client_type),
    client_device_id(client_device_id),
    origin_addr(origin_addr),
    client_addr(client_addr),
    incoming_req(NULL),
    reqlen(0),
    req_so_far(0),
    inactive_timeout(VPLTime_FromSec(5))
{
    last_active = VPLTime_GetTimeStamp();  
}

vss_p2p_client::~vss_p2p_client()
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "P2P Client with punch socket "FMT_VPLSocket_t" listening socket "FMT_VPLSocket_t" and possibly connected socket "FMT_VPLSocket_t" deleting... closing socket.",
                      VAL_VPLSocket_t(punch_sockfd),
                      VAL_VPLSocket_t(listen_sockfd),
                      VAL_VPLSocket_t(sockfd));

    if(!VPLSocket_Equal(punch_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(punch_sockfd);
    }

    if(!VPLSocket_Equal(listen_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(listen_sockfd);
    }

    if(!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(sockfd);
    }

    if(session) {
        server.release_session(session);
    }
    
    if(incoming_req) {
        free(incoming_req);
    }
}

bool vss_p2p_client::start()
{
    int yes = 1;
    int rc;

    // Attempt to make connection to client.
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Attempt to make P2P connection from "FMT_VPLNet_addr_t":%u to "FMTu64" at "FMT_VPLNet_addr_t":%u.",
                      VAL_VPLNet_addr_t(origin_addr.addr),
                      VPLNet_port_ntoh(origin_addr.port),
                      client_device_id, VAL_VPLNet_addr_t(client_addr.addr),
                      VPLNet_port_ntoh(client_addr.port));
    
    // Open two sockets: one to listen and one to connect (punch firewalls)
    punch_sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
    if(VPLSocket_Equal(punch_sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open P2P socket.");
        goto exit;
    }    
    listen_sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
    if(VPLSocket_Equal(listen_sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open P2P socket.");
        goto exit;
    }

    // Must use SO_REUSEADDR option for each. 
    rc = VPLSocket_SetSockOpt(punch_sockfd, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                              (void*)&yes, sizeof(yes));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed (%d) to set SO_REUSEADDR for socket.",
                          rc);
        goto exit;
    }
    rc = VPLSocket_SetSockOpt(listen_sockfd, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                              (void*)&yes, sizeof(yes));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed (%d) to set SO_REUSEADDR for socket.",
                          rc);
        goto exit;
    }

    /* Set TCP no delay for performance reasons. */
    VPLSocket_SetSockOpt(punch_sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                         (void*)&yes, sizeof(yes));
    VPLSocket_SetSockOpt(listen_sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                         (void*)&yes, sizeof(yes));
    
    // Bind each to origin address
    rc = VPLSocket_Bind(punch_sockfd, &origin_addr, sizeof(origin_addr));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to bind p2p socket to "FMT_VPLNet_addr_t":%u. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLNet_addr_t(origin_addr.addr),
                         VPLNet_port_ntoh(origin_addr.port),
                         VAL_VPLSocket_t(punch_sockfd), rc);
        goto exit;
    }
    rc = VPLSocket_Bind(listen_sockfd, &origin_addr, sizeof(origin_addr));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to bind p2p socket to "FMT_VPLNet_addr_t":%u. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLNet_addr_t(origin_addr.addr),
                         VPLNet_port_ntoh(origin_addr.port),
                         VAL_VPLSocket_t(listen_sockfd), rc);
        goto exit;
    }
    
    // "Connect" to the client, punching the local firewall.
    rc = VPLSocket_ConnectNowait(punch_sockfd, &client_addr, sizeof(client_addr));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Expected failure (%d) to connect to server "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                          rc,
                          VAL_VPLNet_addr_t(client_addr.addr),
                          VPLNet_port_ntoh(client_addr.port),
                          VAL_VPLSocket_t(punch_sockfd));
        if(rc == VPL_ERR_BUSY || rc == VPL_ERR_AGAIN) {
            punching = true;
        }
    }
    else {
        sockfd = punch_sockfd;        
        connected = true;
        receiving = true;
        listening = false; // will close listening socket when done.
        punch_sockfd = VPLSOCKET_INVALID;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Punch socket connected to client at "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                          VAL_VPLNet_addr_t(client_addr.addr),
                          VPLNet_port_ntoh(client_addr.port),
                          VAL_VPLSocket_t(sockfd));
        goto exit;
    }

    // Listen on the socket.
    // expecting only a single connection
    rc = VPLSocket_Listen(listen_sockfd, 1);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to listen on p2p socket: %d.", rc);
        goto exit;
    }
    
    listening = true;
    
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Now listening from "FMT_VPLNet_addr_t":%u on socket "FMT_VPLSocket_t" for P2P connection from client "FMTu64".",
                      VAL_VPLNet_addr_t(origin_addr.addr), VPLNet_port_ntoh(origin_addr.port),
                      VAL_VPLSocket_t(listen_sockfd), client_device_id);
    
 exit:
    last_active = VPLTime_GetTimeStamp();  
    return (listening || connected);
}


bool vss_p2p_client::active()
{
    bool rv = false;

    // Consider active if connected or listening.
    if(connected || listening) {
        rv = true;
    }

    return rv;
}

bool vss_p2p_client::inactive()
{
    bool rv = false;
    
    // Inactive if timed out or neither connected nor listening.
    if(!connected && !listening) {
        rv = true;
    }
    else if(last_active + inactive_timeout < VPLTime_GetTimeStamp()) {
        rv = true;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "P2P Client socket "FMT_VPLSocket_t" timed out after "FMT_VPLTime_t"us inactivity.",
                          VAL_VPLSocket_t(sockfd), VPLTime_GetTimeStamp() - last_active);
    }

    return rv;
}

void vss_p2p_client::disconnect(void)
{
    connected = false;
    listening = false;
}

void vss_p2p_client::do_receive(VPLSocket_t socket)
{
    if(VPLSocket_Equal(socket, sockfd)) {
        // Authenticate connection.
        do_auth_receive();
    }
    else if(VPLSocket_Equal(socket, punch_sockfd)) {
        // Check status of punch socket. 
        // "Accept" connection if succeeded.
        // Retry connect if failed.
        int so_err = 0;
        int rc;

        rc = VPLSocket_GetSockOpt(punch_sockfd, 
                                  VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_ERROR,
                                  &so_err, sizeof(so_err));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Get SO_ERROR for punch_sockfd failed:%d.",
                              rc); 
            so_err = -1;
        }

        if(so_err == 0) {
            sockfd = punch_sockfd;        
            connected = true;
            receiving = true;
            listening = false; // will close listening socket when done
            punching = false; // will close punching socket when done
            server.noticeP2PConnected(this);
            punch_sockfd = VPLSOCKET_INVALID;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Punch socket connected to client at "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                              VAL_VPLNet_addr_t(client_addr.addr),
                              VPLNet_port_ntoh(client_addr.port),
                              VAL_VPLSocket_t(sockfd));

        }
        else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Punch socket connected ends with error %d.",
                              so_err); 
            punching = false;
        }
    }
    else if(VPLSocket_Equal(socket, listen_sockfd)) {
        // Check status of punch socket.
        do_connect();
    }
}

void vss_p2p_client::do_connect()
{
    // Connection made. Accept it and move to connected state.
    VPLSocket_addr_t addr;
    int rc;

    rc = VPLSocket_Accept(listen_sockfd, &addr, sizeof(addr),
                          &sockfd);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Client connection accept error. socket:"FMT_VPLSocket_t", error:%d", 
                         VAL_VPLSocket_t(listen_sockfd), rc);
    }
    else {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Got connection from "FMT_VPLNet_addr_t":%u (Expected from "FMT_VPLNet_addr_t":%u) on socket "FMT_VPLSocket_t".",
                          VAL_VPLNet_addr_t(addr.addr), VPLNet_port_ntoh(addr.port),
                          VAL_VPLNet_addr_t(client_addr.addr), VPLNet_port_ntoh(client_addr.port),
                          VAL_VPLSocket_t(sockfd));
        last_active = VPLTime_GetTimeStamp();
        inactive_timeout = VPLTime_FromSec(3);
        connected = true;
        receiving = true;
        listening = false; // will close listening socket when done
        punching = false; // will close punching socket when done
        server.noticeP2PConnected(this);
    }
}

void vss_p2p_client::do_auth_receive()
{
    int rc;
    void* buf;
    size_t bufsize;

    // Determine how many bytes to receive.
    bufsize = receive_buffer(&buf);
    if(buf == NULL || bufsize <= 0) {
        // nothing to receive now
        goto exit;
    }
    
    // Receive them. Handle any socket errors.
    rc = VPLSocket_Recv(sockfd, buf, bufsize);
    if(rc < 0) {
        if(rc == VPL_ERR_AGAIN) {
            // Temporary setback.
            rc = 0;
            goto exit;
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "recv error %d at sockfd %d, bufaddr %p, length %u",
                             rc, VPLSocket_AsFd(sockfd),
                             buf, bufsize);
            goto recv_error;
        }
    }
    else if(rc == 0) { // Socket shutdown. Treat as "error".
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Socket %d disconnected.",
                          VPLSocket_AsFd(sockfd));
        goto recv_error;
    }
    
    // Handle received bytes.
    if(rc > 0) {
        handle_received(rc);
    }
    
    last_active = VPLTime_GetTimeStamp();

 exit:
    return;
 recv_error:
    // Failed to receive data. Consider connection lost.
    receiving=false;
}

size_t vss_p2p_client::receive_buffer(void** buf_out)
{
    // Determine how much data to receive.
    // Allocate space to receive it as needed.
    size_t rv = 0;
    *buf_out = NULL;
    
    if(reqlen == 0) {
        // If no command yet in progress, start getting a command header.
        if(incoming_req == NULL) {
            incoming_req = (char*)calloc(VSS_HEADER_SIZE, 1);
            if(incoming_req == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed allocation for incoming req, client sockfd %d.",
                                 VPLSocket_AsFd(sockfd));
                goto exit;
            }
        }

        *buf_out = (void*)(incoming_req + req_so_far);
        rv = VSS_HEADER_SIZE - req_so_far;
    }
    else if(req_so_far == VSS_HEADER_SIZE) {
        // Increase req as needed
        if(reqlen > VSS_HEADER_SIZE) {
            void* tmp = realloc(incoming_req, reqlen);
            if(tmp == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to realloc req of size %u for req on socket %d.",
                                 reqlen, VPLSocket_AsFd(sockfd));
                goto exit;
            }
            incoming_req = (char*)tmp;
        }

        *buf_out = (void*)(incoming_req + req_so_far);
        rv = reqlen - req_so_far;
    }
    else {
        *buf_out = (void*)(incoming_req + req_so_far);
        rv = reqlen - req_so_far;
    }

 exit:
    return rv;
}

void vss_p2p_client::handle_received(int bufsize)
{
    // Accept the bytes received.
    req_so_far += bufsize;

    if(req_so_far < VSS_HEADER_SIZE) {
        // Do nothing yet.
    }
    else if(req_so_far == VSS_HEADER_SIZE) {
        // Verify the header as good. Use already-known session.

        int rc = session->verify_header(incoming_req);
        if(rc != 0) {
            // Drop this connection. P2P connect fails.
            receiving = false;
            connected = false;
        }
        else {
            reqlen = VSS_HEADER_SIZE + vss_get_data_length(incoming_req);
        }
    }

    if(reqlen > 0 && reqlen == req_so_far) {
        // Verify the data as good, if there's data.
        int rc = session->validate_request_data(incoming_req);
        if(rc != 0) {
            receiving = false;
            connected = false;
        }
        else {
            // Must be an AUTHENTICATE request. Make sure.
            char* resp = authenticate_connection();

            // Create appropriate type of client. Add to server.
            server.makeP2PConnection(sockfd, client_addr, session,
                                     client_device_id, proxy_client_type,
                                     resp);
            // New connected client now owns socket, session, and response.
            session = NULL;
            sockfd = VPLSOCKET_INVALID;
            receiving = false;
            // P2P client may now be deleted. Let server sweep-up on timeout.
            connected = false;
        }
    }

    return;
}

char* vss_p2p_client::authenticate_connection()
{
    s16 rc = VSSI_SUCCESS;
    char* data = incoming_req + VSS_HEADER_SIZE;
    char* resp;

    // Expect a VSS_AUTHENTICATE. Anything else is an error.
    if(vss_get_command(incoming_req) != VSS_AUTHENTICATE) {
        rc = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Received unexpected command code %d when authenticating proxy stream.",
                         vss_get_command(incoming_req));     
    }
    // Check cluster ID is the local device ID.
    else if(vss_authenticate_get_cluster_id(data) !=
            server.clusterId) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "P2P connection intended for device "FMTx64" arrived here at device "FMTx64". Rejecting auth.",
                         vss_authenticate_get_cluster_id(data),
                         server.clusterId);
        rc = VSSI_WRONG_CLUSTER;
    }
    // Successful authentication.
    else {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Successful authentication of secure stream.");
        rc = VSSI_SUCCESS;
    }

    // Prepare and send response with result code. No body data needed.
    resp = (char*)calloc(VSS_HEADER_SIZE, 1);
    vss_set_version(resp, vss_get_version(incoming_req));
    vss_set_command(resp, VSS_AUTHENTICATE_REPLY);
    vss_set_status(resp, rc);
    vss_set_xid(resp, vss_get_xid(incoming_req));
    vss_set_device_id(resp, vss_get_device_id(incoming_req));
    vss_set_handle(resp, vss_get_handle(incoming_req));
    vss_set_data_length(resp, 0);
    session->sign_reply(resp);
    
    return resp;
}
