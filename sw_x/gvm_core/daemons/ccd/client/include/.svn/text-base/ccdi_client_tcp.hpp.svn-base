//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCDI_CLIENT_TCP_HPP__
#define __CCDI_CLIENT_TCP_HPP__

//============================================================================
/// @file
/// Extensions to ccdi.hpp for using the API as an IPC client over TCP.
//============================================================================

#include "vpl_socket.h"

namespace ccdi {
namespace client {

/// Override the CCD instance to connect to (for testing & debugging purposes).
/// @note No lock is used, so make sure that you don't use CCDI concurrently with calling this!
void CCDIClient_SetRemoteSocket(const VPLSocket_addr_t& sockAddr);

/// Override the CCD instance to connect to for the current thread only (for testing & debugging purposes).
void CCDIClient_SetRemoteSocketForThread(const VPLSocket_addr_t& sockAddr);

} // namespace client
} // namespace ccdi

#endif // include guard
