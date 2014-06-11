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

// P2P Library internal routines

#include "p2p_internal.hpp"
#include "vplex_trace.h"
#include "vplex_socket.h"
#include "vpl_error.h"
#include "vpl_socket.h"
#include "vpl_thread.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"

#include <map>

#define POLL_SIZE 3

VPLLazyInitMutex_t p2plib_mutex = VPLLAZYINITMUTEX_INIT;
std::map<P2PHandle, VPLSocket_t> p2plib_handles;
u32 handle_id_next = 0;

VPLTHREAD_FN_DECL p2p_connect_internal(void* arg)
{
    p2p_connect_internal_args_t *p2p_args = (p2p_connect_internal_args_t *)arg;
    int result = VPL_OK;
    p2p_callback ap_callback = NULL;
    void* ctx = NULL;
    VPLSocket_poll_t poll_sockets[POLL_SIZE];
    VPLSocket_t active_tcp = VPLSOCKET_INVALID, passive_tcp = VPLSOCKET_INVALID, connected_socket = VPLSOCKET_INVALID,
            cmd_socket_listener = VPLSOCKET_INVALID, cmd_socket_client = VPLSOCKET_INVALID, cmd_socket_server = VPLSOCKET_INVALID;
    P2PHandle handle = 0;
    if(p2p_args == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Args is NULL.");
        goto end;
    }

    handle = p2p_args->handle;
    result = p2p_args->result;
    ap_callback = p2p_args->callback;
    ctx = p2p_args->ctx;
    if(result != VPL_OK) {
        // fail due to previous steps
        goto end;
    }

    // create two more sockets
    result = p2p_create_bind_tcp_reuseaddr_socket(&active_tcp, &(p2p_args->local_addr), sizeof(p2p_args->local_addr));
    if(result != VPL_OK) {
        goto end;
    }
    result = p2p_create_bind_tcp_reuseaddr_socket(&passive_tcp, &(p2p_args->local_addr), sizeof(p2p_args->local_addr));
    if(result != VPL_OK) {
        goto end;
    }

    // listen on passive_tcp socket for single connection attempted from other peer
    result = VPLSocket_Listen(passive_tcp, 1);
    if(result != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Listen failed with %d.", result);
        goto end;
    }

    // try to connect to other peer
    result  = VPLSocket_ConnectNowait(active_tcp, &(p2p_args->remote_addr), sizeof(p2p_args->remote_addr));
    if(result == VPL_OK) {
        // we got connected
        connected_socket = active_tcp;
        goto end;
    } else if(result != VPL_ERR_BUSY && result != VPL_ERR_AGAIN) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_ConnectNowait failed with %d.", result);
        goto end;
    }

    {
        VPLNet_addr_t addr = VPLNET_ADDR_LOOPBACK;
        VPLNet_port_t port = VPLNET_PORT_ANY;
        cmd_socket_listener = VPLSocket_CreateTcp(addr, port);
        if (VPLSocket_Equal(cmd_socket_listener, VPLSOCKET_INVALID)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to create listen socket");
            goto end;
        }
    }

    result = VPLSocket_Listen(cmd_socket_listener, 1);
    if (result) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Listen() failed on socket["FMT_VPLSocket_t"]: err %d", VAL_VPLSocket_t(cmd_socket_listener), result);
        goto end;
    }

    cmd_socket_client = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, /*nonblocking*/VPL_TRUE);
    if (VPLSocket_Equal(cmd_socket_client, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to create client-end socket");
        goto end;
    }

    {
        VPLSocket_addr_t sin;
        sin.family = VPL_PF_INET;
        sin.addr = VPLNET_ADDR_LOOPBACK;
        sin.port = VPLSocket_GetPort(cmd_socket_listener);
        result = VPLSocket_Connect(cmd_socket_client, &sin, sizeof(sin));
        if (result) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to connect client socket to server: err %d", result);
            goto end;
        }
    }

    {
        VPLSocket_addr_t addr;
        int err = VPLSocket_Accept(cmd_socket_listener, &addr, sizeof(addr), &cmd_socket_server);
        if (err) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Accept() failed: err %d", err);
            goto end;
        }
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));
    p2plib_handles[handle] = cmd_socket_client;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));

    // poll for active and passive
    poll_sockets[0].socket = passive_tcp;
    poll_sockets[0].events = VPLSOCKET_POLL_RDNORM;
    poll_sockets[1].socket = active_tcp;
    poll_sockets[1].events = VPLSOCKET_POLL_OUT;
    poll_sockets[2].socket = cmd_socket_server;
    poll_sockets[2].events = VPLSOCKET_POLL_RDNORM;

    result = VPLSocket_Poll(poll_sockets, POLL_SIZE, VPLTime_FromSec(P2P_TIMEOUT_SEC));
    if(result > 0) {
        for (int i = 0; i < POLL_SIZE; i++) {

            // check cmd socket, there is only shutdown case now
            if(VPLSocket_Equal(poll_sockets[i].socket, cmd_socket_server) &&
               poll_sockets[i].revents) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "cmd socket got message, force to shutdown");
                result = VPL_ERR_FAIL;
                goto end;
            }

            // check active socket
            if(VPLSocket_Equal(poll_sockets[i].socket, active_tcp) &&
               poll_sockets[i].revents) {
                int opt = 0;

                if(poll_sockets[i].revents != VPLSOCKET_POLL_OUT) {  // bad socket - handle as error
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "socket failed: revents %d", poll_sockets[i].revents);
                    result = VPL_ERR_IO;
                    goto end;
                }

                result = VPLSocket_GetSockOpt(active_tcp, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_ERROR, 
                                              (void *)&opt, sizeof(opt));
                if(result != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_GetSockOpt failed with %d.", result);
                    goto end;
                }
                if(opt == 0) {
                    // we got connected
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Active socket connected.");
                    connected_socket = active_tcp;
                    goto end;
                } else {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Active socket failed.");
                    result = opt;
                }
            }

            // check passive socket
            if(VPLSocket_Equal(poll_sockets[i].socket, passive_tcp) &&
               poll_sockets[i].revents) {

                if(poll_sockets[i].revents != VPLSOCKET_POLL_RDNORM) {  // bad socket - handle as error
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "socket failed: revents %d", poll_sockets[i].revents);
                    result = VPL_ERR_IO;
                    goto end;
                }

                result = VPLSocket_Accept(passive_tcp, NULL, 0, &connected_socket);
                if(result == VPL_OK) {
                    int opt = 1;
                    // we got connected
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Passive socket connected.");
                    // Set TCP_NO_DELAY to the new connected socket
                    result = VPLSocket_SetSockOpt(connected_socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &opt, sizeof(opt));
                    if(result != VPL_OK) {
                        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "VPLSocket_SetSockOpt set TCP_NODELAY failed with %d.", result);
                        result = VPL_OK;
                    }
                    goto end;
                }
            }
        }
    } else {
        result = VPL_ERR_TIMEOUT;
    }

end:
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));
    p2plib_handles.erase(handle);
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));

    if(VPLSocket_Equal(VPLSOCKET_INVALID, connected_socket)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "P2P failed with %d.", result);
    } else {
        result = VPL_OK;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "P2P succeed.");
    }
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, passive_tcp) &&
       !VPLSocket_Equal(connected_socket, passive_tcp)) {
        VPLSocket_Close(passive_tcp);
    }
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, active_tcp) &&
       !VPLSocket_Equal(connected_socket, active_tcp)) {
        VPLSocket_Close(active_tcp);
    }
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, cmd_socket_client)) {
        VPLSocket_Close(cmd_socket_client);
    }
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, cmd_socket_server)) {
        VPLSocket_Close(cmd_socket_server);
    }
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, cmd_socket_listener)) {
        VPLSocket_Close(cmd_socket_listener);
    }
    if(p2p_args != NULL) {
        free(p2p_args);
        p2p_args = NULL;
    }
    if(ap_callback != NULL) {
        ap_callback(result, connected_socket, ctx);
    }
    // Must always return VPLTHREAD_RETURN_VALUE.
    return VPLTHREAD_RETURN_VALUE;
}

int p2p_create_bind_tcp_reuseaddr_socket(VPLSocket_t* socket, const VPLSocket_addr_t* addr, size_t addr_size)
{
    int result = VPL_OK;
    int opt = 1;
    *socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);
    if(VPLSocket_Equal(VPLSOCKET_INVALID, *socket)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to create socket.");
        result = VPL_ERR_SOCKET; 
        goto end;
    }

    // set reuseaddr
    result = VPLSocket_SetSockOpt(*socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR, 
                         (void *)&opt, sizeof(opt));
    
    if(result != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_SetSockOpt failed with %d.", result);
        goto end;
    }
    
#ifdef IOS
    // set reuseport for iOS
    result = VPLSocket_SetSockOpt(*socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEPORT,
                         (void *)&opt, sizeof(opt));

    if(result != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_SetSockOpt failed with %d.", result);
        goto end;
    }
#endif

    // bind socket
    result = VPLSocket_Bind(*socket, addr, addr_size);
    if(result != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Bind failed with %d.", result);
        goto end;
    }

    result = VPLSocket_SetSockOpt(*socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &opt, sizeof(opt));
    if(result != VPL_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "VPLSocket_SetSockOpt set TCP_NODELAY failed with %d.", result);
        result = VPL_OK;
    }

end:
    return result;
}

void p2p_stop_internal(P2PHandle handle)
{
    VPLSocket_t cmd_socket = VPLSOCKET_INVALID;
    std::map<P2PHandle, VPLSocket_t>::iterator it;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));
    if( (it = p2plib_handles.find(handle)) != p2plib_handles.end() ) {
        cmd_socket = it->second;
        if( !VPLSocket_Equal(VPLSOCKET_INVALID, cmd_socket)) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Force to shutdown");
            int bytesWritten = VPLSocket_Send(cmd_socket, "Q", 1);
            if (bytesWritten < 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to write into pipe: err %d", (int)bytesWritten);
            }
            else if (bytesWritten != 1) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Wrong number of bytes written into pipe: nbytes %d", (int)bytesWritten);
            }
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));
}
