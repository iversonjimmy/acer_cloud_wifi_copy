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

/// P2P Library test program
#include <cstdio>
#include "p2p.hpp"
#include "vpl_th.h"
#include "vplex_trace.h"
#include "vpl_error.h"
#include "vpl_socket.h"
#include <sstream>
#include <string>

#define BUF_SIZE 64

void p2p_done(int result, VPLSocket_t socket, void* ctx);
typedef struct {
    VPLSem_t sem;
    int result;
} p2p_context_t;

int main(int argc, char **argv)
{
    int result = VPL_OK;
    int opt = 1;
    std::string client_id;
    std::string request_client_id;
    VPLSocket_t socket;
    VPLSocket_addr_t addr_server;
    VPLSocket_addr_t addr;
    p2p_context_t context;
    std::stringstream buf_stream;
    std::string buf_string;
    char buf[BUF_SIZE] = {0};
    int bytes_done = 0;
    // TODO: Print TC_RESULT
    // TODO: clean up
    // TODO: usage
    if(argc < 5) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "usage: p2p_test server_addr server_port your_client_id remote_peer_client_id");
        goto end;
    }
    
    VPL_Init();
    VPL_SET_UNINITIALIZED(&(context.sem));

    if(VPLSem_Init(&(context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "Failed to create semaphore");
        goto end;
    }

    // Set up connnection to server first
    addr_server.addr = VPLNet_GetAddr(argv[1]);
    addr_server.port = VPLNet_port_hton((u16)atoi(argv[2]));

    client_id = argv[3];
    request_client_id = argv[4];

    // Create socket
    socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_FALSE);

    // Set reuseaddr
    result = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR, 
                         (void *)&opt, sizeof(opt));
    
    // Connect
    result = VPLSocket_Connect(socket, &addr_server, sizeof(addr_server));

    // Get the other peer addr
    // send "CLIENT_ID,REQUEST_CLIENT_ID"
    // recv "REQ_CLIENT_IP:REQ_CLIENT_PORT"
    buf_stream << client_id << "," << request_client_id;
    memcpy(buf, buf_stream.str().data(), buf_stream.str().size());
    while(bytes_done < BUF_SIZE) {
        int rc = 0;
        rc = VPLSocket_Send(socket, buf + bytes_done, BUF_SIZE - bytes_done);
        if(rc >= 0) {
            bytes_done += rc;
        }
    }
    memset(buf, 0, BUF_SIZE);
    bytes_done = 0;
    while(bytes_done < BUF_SIZE) {
        int rc = 0;
        rc = VPLSocket_Recv(socket, buf + bytes_done, BUF_SIZE - bytes_done);
        if(rc >= 0) {
            bytes_done += rc;
        }
    }
    buf_string = buf;
    addr.addr = VPLNet_GetAddr(buf_string.substr(0,buf_string.find_first_of(':')).c_str());
    addr.port = VPLNet_port_hton((u16)atoi(buf_string.substr(buf_string.find_first_of(':')+1).c_str()));

    VPLTRACE_LOG_INFO(TRACE_APP, 0, "client_id %s request id: %s request id addr: %s", client_id.c_str(), request_client_id.c_str(), buf_string.c_str());

    // Do p2p
    p2p_connect(socket, &addr, sizeof(addr), &context, p2p_done);

    // We are waiting here for p2p algorithm finished
    VPLSem_Wait(&(context.sem));
    result = context.result;
    if(result != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "p2p_connect() with %d", result);
        goto end;
    }

end:
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, socket)) {
        VPLSocket_Close(socket);
    }
    VPLSem_Destroy(&(context.sem));
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "Test done\n");
    VPL_Quit();
    return result; 
}

// P2p callback funtion is getting called after p2p attemping is done.
void p2p_done(int result, VPLSocket_t socket, void* ctx)
{
    p2p_context_t *context = (p2p_context_t *)ctx;
    context->result = result;
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, socket)) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "P2P succeed\n");
        VPLSocket_Close(socket);
    } else {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "P2P failed on %d \n", result);
    }
    VPLSem_Post(&(context->sem));
}
