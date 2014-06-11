//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCDI_CLIENT_NAMED_SOCKET_HPP__
#define __CCDI_CLIENT_NAMED_SOCKET_HPP__

//============================================================================
/// @file
/// Extensions to ccdi.hpp for using the API as an IPC client to another process on the local host.
//============================================================================

#include "vplex_named_socket.h"

namespace ccdi {
namespace client {

/// @note Caller must call #VPLNamedSocket_CloseClient() to close \a sock_out when done with it.
/// @param sock_out Provide an uninitialized VPLNamedSocketClient_t struct.  Upon success, the
///     struct will be initialized.
int CCDIClient_OpenNamedSocket(VPLNamedSocketClient_t* sock_out);

} // namespace client
} // namespace ccdi

#endif // include guard
