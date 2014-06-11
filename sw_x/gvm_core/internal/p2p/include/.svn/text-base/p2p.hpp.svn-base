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

/// P2P Library interface

#ifndef __P2P_H__
#define __P2P_H__

#include "vpl_socket.h"
#include "vplu_types.h"

/// Maximum timeout in seconds for a p2p attempt.
/// NOTE: the actually socket timeout value depends on platform TCP stack,
///       this define the maximum value we expect every platforms should have less timeout value than this one
#define P2P_TIMEOUT_SEC 60

// P2P Library handle
typedef u32 P2PHandle;

/// Callback interface for p2p_connect(), 
/// @param[in] result VPL_OK on success, or VPL error.
/// @param[in] VPLSocket_t socket handle that actually connects to remote peer,
///                        VPLSOCKET_INVALID on failure.
typedef void (*p2p_callback)(int result, VPLSocket_t, void* ctx);

/// Try to establish a p2p connection to remote peer via simultaneous TCP open. Callback function is getting called after attempting is done.
/// @param[in] handle p2p library handle id
/// @param[in] socket The socket should have established a connection to pxd and connection should not be closed before p2p is done.
///                   This is a TCP, address reuse, and non-blocking socket.
/// @param[in] addr address of remote peer
/// @param[in] addr_size size of addr structure
/// @param[in] ctx context pass into callback function
/// @param[in] callback callback function pointer
/// @return #VPL_OK Success
int p2p_connect(P2PHandle &handle,
                VPLSocket_t socket,
                const VPLSocket_addr_t* addr,
                size_t addr_size,
                void* ctx,
                p2p_callback callback);

/// Force stop any sockets attempts
/// A caller doesn't need to call this function if it already got a p2p callback returned
/// @param[in] handle p2p library handle id
void p2p_stop(P2PHandle handle);

#endif
