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

/// P2P Library simple server for exchange address between two test program
#include <cstdio>
#include "vpl_th.h"
#include "vplex_trace.h"
#include "vpl_error.h"
#include "vpl_socket.h"
#include "vpl_thread.h"
#include <signal.h>
#include <map>
#include <string>
#include <sstream>

#define MAX_CLIENTS 20
#define BUF_SIZE 64
void int_handler(int);
VPLTHREAD_FN_DECL p2p_client_handler(void* arg);
static bool running;
VPLMutex_t mutex; 

typedef struct {
    VPLSocket_addr_t addr;
    VPLSocket_t socket;
} p2p_client;

static std::map<std::string, p2p_client*> clients;

void int_handler(int dummy=0)
{
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "shutdowning...\n");
    running = false;
}

int main(int argc, char **argv)
{
    int result;
    VPLSocket_t socket;
    running = true;
    VPLSocket_addr_t addr;
    VPLSocket_poll_t poll_sockets[1];
    if(argc < 2) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "usage: p2p_test_server server_port");
        goto end;
    }

    VPL_Init();
    VPLMutex_Init(&mutex); 
    // Create socket
    socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);

    addr.addr = VPLNET_ADDR_ANY;
    addr.port = VPLNet_port_hton((u16)atoi(argv[1]));
    result = VPLSocket_Bind(socket, &addr, sizeof(addr));
    
    result = VPLSocket_Listen(socket, MAX_CLIENTS);
    signal (SIGINT, int_handler);
    
    poll_sockets[0].socket = socket;
    poll_sockets[0].events = VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_OUT;

    // Listeing
    while(running) {
        result = VPLSocket_Poll(poll_sockets, 1, VPLTime_FromSec(5));
        if(result > 0 ) {
            p2p_client* client = (p2p_client*)calloc(1, sizeof(p2p_client));
            result = VPLSocket_Accept(socket, &(client->addr), sizeof(client->addr), &(client->socket));
            if(result == VPL_OK) {
                VPLDetachableThreadHandle_t thr;
                VPLThread_attr_t thr_attr;
                VPLThread_AttrInit(&thr_attr);
                VPLThread_AttrSetStackSize(&thr_attr, 32*1024);
                VPLThread_AttrSetDetachState(&thr_attr, false);
                VPLDetachableThread_Create(&thr,
                                           p2p_client_handler,
                                           client,
                                           &thr_attr,
                                           "p2p_server_client_handler");
            } else {
                free(client);
            }
        }
    }

    VPLMutex_Lock(&mutex);
    VPLSocket_Close(socket);
    for(std::map<std::string, p2p_client*>::iterator it = clients.begin();
        it != clients.end(); ++it) {
        VPLSocket_Close(it->second->socket);
        free(it->second);
    }
    VPLMutex_Unlock(&mutex);
    VPLMutex_Destroy(&mutex); 

end:
    VPL_Quit();
    return 0;
}

VPLTHREAD_FN_DECL p2p_client_handler(void* arg)
{
    // handle client request
    // record client id, and try to find requested addr info for client
    // recv "CLIENT_ID,REQUEST_CLIENT_ID"
    // send "REQ_CLIENT_IP:REQ_CLIENT_PORT"
    p2p_client* client = (p2p_client*)arg;
    char buf[BUF_SIZE] = {0};
    bool request_done = false;
    std::string client_id;
    std::string request_client_id;
    std::stringstream buf_stream;
    std::string buf_string;
    char ip_addr[16] = {0};
    u16 port = 0;
    int bytes_done = 0;

    VPLTRACE_LOG_INFO(TRACE_APP, 0, "client connected.");
    while(bytes_done < BUF_SIZE) {
        int rc = 0;
        rc = VPLSocket_Recv(client->socket, buf, BUF_SIZE - bytes_done);
        if(rc >= 0) {
            bytes_done += rc;
        }
    }
    buf_string = buf;
    client_id = buf_string.substr(0,buf_string.find_first_of(','));
    request_client_id = buf_string.substr(buf_string.find_first_of(',')+1);
    VPLMutex_Lock(&mutex);
    clients[client_id] = client;
    VPLMutex_Unlock(&mutex);
    while(!request_done && running) {
        VPLMutex_Lock(&mutex);
        if(clients[request_client_id] != NULL) {
            VPLNet_Ntop(&(clients[request_client_id]->addr.addr), ip_addr, 16);
            port = VPLNet_port_ntoh(clients[request_client_id]->addr.port);
            buf_stream << ip_addr << ":" << port;
            memset(buf, 0, BUF_SIZE);
            memcpy(buf, buf_stream.str().data(), buf_stream.str().size());
            bytes_done = 0;
            while(bytes_done < BUF_SIZE) {
                int rc = 0;
                rc = VPLSocket_Send(client->socket, buf + bytes_done, BUF_SIZE - bytes_done);
                if(rc >= 0) {
                    bytes_done += rc;
                }
            }
            VPLTRACE_LOG_INFO(TRACE_APP, 0, "client_id %s request id: %s request id addr: %s", 
                              client_id.c_str(), request_client_id.c_str(), buf);
            request_done = true;
            VPLMutex_Unlock(&mutex);
        } else {
            VPLMutex_Unlock(&mutex);
            VPLThread_Sleep(VPLTime_FromSec(1));
        }
    }
    return VPLTHREAD_RETURN_VALUE;
}
