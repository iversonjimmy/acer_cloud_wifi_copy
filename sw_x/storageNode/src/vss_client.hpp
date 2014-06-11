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

#ifndef STORAGE_NODE__VSS_CLIENT_HPP__
#define STORAGE_NODE__VSS_CLIENT_HPP__

/// @file
/// VSS client class
/// A client is a console requesting a VSS operation.

class vss_client;

#include "vplu_types.h"
#include "vpl_socket.h"
#include "vpl_net.h"
#include "vpl_th.h"
#include "vpl_time.h"

#include <queue>
#include <utility>

#include "vssi_error.h"

#include "vss_session.hpp"
#include "vss_server.hpp"

class vss_client
{
public:
    // Communication socket for the client
    VPLSocket_t sockfd;
    bool sending;
    bool receiving;
    bool disconnected;
    bool new_connection;

    int tasks_pending;

    // Client's origin address.    
    VPLNet_addr_t addr;
    VPLNet_port_t port;
    // Client device ID (last valid device ID received)
    u64 device_id;

    // Parent server
    vss_server& server;

    vss_client(vss_server& server,
               VPLSocket_t sockfd,
               VPLNet_addr_t addr,
               VPLNet_port_t port);
    ~vss_client();

    // Start a client. Use when client first connects.
    int start(VPLTime_t inactive_timeout);

    void do_send(void);

    void do_receive(void);

    // Further receive processing by worker threads.
    void verify_incoming_header();
    void verify_request(vss_req_proc_ctx*& context);

    // Put a response for eventual sending to client.
    void put_response(const char* resp, bool proxy_reply = false, bool async_reply = false);

    // Clients have three states: 
    // * active (working on a request)
    // * idle (connected, but no request receiving or in progress)
    // * inactive (timed-out or disconnected and delete-able)
    // Is client actively processing a request?
    bool active();
    // Check if client has gone inactive.
    // Inactive if no requests in-progress and disconnected or timed-out.
    bool inactive();

    // Disconnect the vss_client request handler. Will cause to be cleaned up on next
    // timeout.  Also disconnects on comm error.
    void disconnect();

    // add object to associated list, should be called by vss_object only.
    void associate_object(vss_object * object);

    // remove object from associated list, should be called by vss_object only.
    void disassociate_object(vss_object * object);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_client);

    u64 recv_cnt;
    u64 send_cnt;
    u64 cmd_cnt;
    VPLTime_t start_time;

    vss_req_proc_ctx* incoming_req;
    size_t reqlen;
    size_t req_so_far;
    bool recv_error; // If set, no more receive activity allowed.

    // Queues of requests and responses pending
    std::queue<std::pair<size_t, const char*> > send_queue;
    size_t sent_so_far; // for head reply in-progress

    VPLMutex_t mutex;

    // Count of received commands not yet replied.
    int pending_cmds;

    // Time client was last active.
    VPLTime_t last_active;
    // Time until client deemed inactive.
    VPLTime_t inactive_timeout;

    size_t receive_buffer(void** buf_out);
    void handle_received(int bufsize);

    //associated clients
    std::set<vss_object*> objects;
};

// Worker thread task entry points
void verify_incoming_header_helper(void* vpclient);

#endif // include guard
