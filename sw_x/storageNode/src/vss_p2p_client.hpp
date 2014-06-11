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

#ifndef STORAGE_NODE__VSS_P2P_CLIENT_HPP__
#define STORAGE_NODE__VSS_P2P_CLIENT_HPP__

/// @file
/// VSS P2P client class
/// A P2P client is an attempt to make a P2P connection for another client type.

class vss_p2p_client;

#include "vplu_types.h"
#include "vpl_socket.h"
#include "vpl_th.h"
#include "vpl_time.h"

#include <queue>
#include <utility>

#include "vssi_error.h"

#include "vss_session.hpp"
#include "vss_server.hpp"

class vss_p2p_client
{
public:
    // Communication socket for the client
    VPLSocket_t punch_sockfd;
    VPLSocket_t listen_sockfd;
    VPLSocket_t sockfd;
    bool punching;
    bool listening;
    bool receiving;
    bool connected;

    vss_p2p_client(vss_server& server,
                   vss_session* session,
                   u8 proxy_client_type,
                   u64 client_device_id,
                   VPLSocket_addr_t& origin_addr,
                   VPLSocket_addr_t& client_addr);

    ~vss_p2p_client();

    // Start P2P process - attempt to connect to client
    bool start();

    void do_receive(VPLSocket_t socket);

    // Clients have three states: 
    // * active (working on a request)
    // * idle (connected, but no request receiving or in progress)
    // * inactive (timed-out or disconnected and delete-able)
    // Is client actively processing a request?
    bool active();
    // Is client inactive (disconnected or timed-out)?
    bool inactive();

    // Disconnect the p2p connection handler.  Will cause to be cleaned up on next
    // timeout.
    void disconnect(void);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_p2p_client);

    vss_server& server;
    vss_session* session;
    u8 proxy_client_type;
    u64 client_device_id;
    VPLSocket_addr_t origin_addr;
    VPLSocket_addr_t client_addr;

    char* incoming_req;
    size_t reqlen;
    size_t req_so_far;

    VPLTime_t last_active;
    VPLTime_t inactive_timeout;

    void do_connect();
    void do_auth_receive();
    size_t receive_buffer(void** buf_out);
    void handle_received(int bufsize);
    char* authenticate_connection();
};

#endif // include guard
