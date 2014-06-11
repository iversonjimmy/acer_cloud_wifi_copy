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

#ifndef __P2P_INTERNAL_H__
#define __P2P_INTERNAL_H__

#include "vpl_thread.h"
#include "vpl_socket.h"
#include "p2p.hpp"

// arg structure pass into p2p_connect_internal()
typedef struct {
    P2PHandle handle;
    int result;
    VPLSocket_addr_t local_addr;
    VPLSocket_addr_t remote_addr;
    void* ctx;
    p2p_callback callback;

} p2p_connect_internal_args_t;

/// Routine for internal connect to other remote peer and callback
VPLTHREAD_FN_DECL p2p_connect_internal(void* arg);

/// Routine for internal stop sockets
void p2p_stop_internal(P2PHandle handle);

/// Routine for create and bind tcp socket with reuse address
int p2p_create_bind_tcp_reuseaddr_socket(VPLSocket_t* socket, const VPLSocket_addr_t* addr, size_t addr_size);


#endif
