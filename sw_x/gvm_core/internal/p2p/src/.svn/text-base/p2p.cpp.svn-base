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

/// P2P Library public interface implementation
#include "p2p_internal.hpp"
#include "vplex_trace.h"
#include "vplex_socket.h"
#include "vpl_error.h"
#include "vpl_socket.h"
#include "vpl_thread.h"
#include "gvm_thread_utils.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"

#include <map>

extern VPLLazyInitMutex_t p2plib_mutex;
extern std::map<P2PHandle, VPLSocket_t> p2plib_handles;
extern u32 handle_id_next;

int p2p_connect(P2PHandle &handle,
                VPLSocket_t socket,
                const VPLSocket_addr_t* addr,
                size_t addr_size,
                void* ctx,
                p2p_callback callback)
{
    int rv = VPL_OK;
    int result = VPL_OK;
    VPLSocket_addr_t local_addr;
    p2p_connect_internal_args_t *p2p_args = NULL;

    local_addr.family = VPL_PF_UNSPEC;
    local_addr.addr = VPLNET_ADDR_INVALID;
    local_addr.port = VPLNET_PORT_INVALID;
    handle = 0;
#ifndef VPL_PLAT_IS_WINRT
    int opt = 0;

    // check if socket has address reuse set
    result = VPLSocket_GetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR, 
                                  (void *)&opt, sizeof(opt));
    if(result != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_GetSockOpt failed with %d.", result);
        goto end;
    }
    if(opt == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Socket is not set VPLSOCKET_SO_REUSEPORT.");
        result = VPL_ERR_INVALID; 
        goto end;
    }
    
    // check if socket is connected
    result = VPLSocket_GetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_ERROR, 
                         (void *)&opt, sizeof(opt));
    if(result != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_GetSockOpt failed with %d.", result);
        goto end;
    }
    if(opt != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Socket is not connected.");
        result = VPL_ERR_INVALID; 
        goto end;
    }

    // get local addr and port bound to socket
    local_addr.family = VPL_PF_INET;
    local_addr.addr = VPLSocket_GetAddr(socket);
    local_addr.port = VPLSocket_GetPort(socket);
    if(local_addr.addr == VPLNET_ADDR_INVALID ||
       local_addr.port == VPLNET_PORT_INVALID) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
        "Socket address or port is not valid.");
        result = VPL_ERR_INVALID;
        goto end;
    }
#else
    // TODO: sequential hole punching
    result = VPL_ERR_NOSYS;
    goto end;
#endif
 end:
    // prepare callback thread
    // pass result, active_tcp, passive_tcp, addr, context
    p2p_args = (p2p_connect_internal_args_t *)calloc(1, sizeof(p2p_connect_internal_args_t));
    if(p2p_args != NULL) {

        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));
        // create an handle id for this thing
        do {
            std::map<P2PHandle, VPLSocket_t>::iterator it;
            handle_id_next++;
            if ( handle_id_next == 0 ) {
                handle_id_next++;
            }
        } while (p2plib_handles.find(handle_id_next) != p2plib_handles.end());
        handle = handle_id_next;

        p2plib_handles[handle] = VPLSOCKET_INVALID;

        p2p_args->handle = handle;
        p2p_args->result = result;
        p2p_args->local_addr.family = local_addr.family; 
        p2p_args->local_addr.addr = local_addr.addr; 
        p2p_args->local_addr.port = local_addr.port; 
        p2p_args->remote_addr.family = addr->family; 
        p2p_args->remote_addr.addr = addr->addr; 
        p2p_args->remote_addr.port = addr->port; 
        p2p_args->ctx = ctx; 
        p2p_args->callback = callback;
       
        rv = Util_SpawnThread(p2p_connect_internal,
                              (void*)p2p_args,
                              UTIL_DEFAULT_THREAD_STACK_SIZE,
                              false,
                              NULL);
        if(rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "SpawnThread failed with %d.", rv);
            free(p2p_args);
            p2p_args = NULL;
            p2plib_handles.erase(handle);
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&p2plib_mutex));
    } else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "OOM while allocate memory.");
        rv = VPL_ERR_NOMEM;
    }
    return rv;
}

void p2p_stop(P2PHandle handle)
{
    p2p_stop_internal(handle);
}
