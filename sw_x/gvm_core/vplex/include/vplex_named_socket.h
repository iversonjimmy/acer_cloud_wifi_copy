//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLEX_NAMED_SOCKET_H__
#define __VPLEX_NAMED_SOCKET_H__

//============================================================================
/// @file
/// Provides a minimal abstraction so that Unix Domain Sockets and Windows
/// Named Pipes have the same interface for our code.
/// Note that this is only intended for IPC between processes running on the
/// same local host.
//============================================================================

#include "vplex_plat.h"

#include "vplex__named_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque type representing a VPL Named Socket.
typedef VPLNamedSocket__t  VPLNamedSocket_t;
#define FMT_VPLNamedSocket_t  FMT_VPLNamedSocket__t
#define VAL_VPLNamedSocket_t(sock)  VAL_VPLNamedSocket__t(sock)

typedef VPLNamedSocketClient__t  VPLNamedSocketClient_t;
#define FMT_VPLNamedSocketClient_t  FMT_VPLNamedSocketClient__t
#define VAL_VPLNamedSocketClient_t(sock)  VAL_VPLNamedSocketClient__t(sock)

//-----------------------
// Server-side calls
//-----------------------

/// Create the named socket and prepare it to listen for incoming connection requests.
s32 VPLNamedSocket_OpenAndActivate(VPLNamedSocket_t* socket, const char* uniqueName, const char* clientOsUserId);

/// Accept a client connection.  The connection can be serviced via @a connectedSocket_out.
/// You must call #VPLNamedSocket_Reactivate() before using @a listeningSocket again.
/// This will block until a connection arrives.
s32 VPLNamedSocket_Accept(VPLNamedSocket_t* listeningSocket, VPLNamedSocket_t* connectedSocket_out);

/// Prepare the named socket to listen for incoming connection requests again (after
/// #VPLNamedSocket_Accept() has been called).
/// @param uniqueName must be the same as the value passed to #VPLNamedSocket_OpenAndActivate()
///     when @a socket was created, otherwise behavior is undefined.
s32 VPLNamedSocket_Reactivate(VPLNamedSocket_t* socket, const char* uniqueName, const char* clientOsUserId);

/// Destroy the named socket (server-side).
s32 VPLNamedSocket_Close(VPLNamedSocket_t* socket);

/// Get the file descriptor than can be used to read from or write to the named socket.
/// @note When using the file descriptor, you should call read()/write() instead of recv()/send().
///     read()/write() will work on all platforms, whereas recv()/send() will fail on Windows.
/// @note On Windows, the file descriptor will not work with select().
int VPLNamedSocket_GetFd(const VPLNamedSocket_t* socket);

//-----------------------
// Client-side calls
//-----------------------

/// Attempt to connect to a named socket.
/// When finished with the socket, you must call #VPLNamedSocket_CloseClient().
s32 VPLNamedSocketClient_Open(const char* uniqueName, VPLNamedSocketClient_t* connectedSocket_out);

/// Release resources associated with @a socket.
s32 VPLNamedSocketClient_Close(VPLNamedSocketClient_t* socket);

/// Get the file descriptor than can be used to read from or write to the named socket.
/// @note When using the file descriptor, you should call read()/write() instead of recv()/send().
///     read()/write() will work on all platforms, whereas recv()/send() will fail on Windows.
/// @note On Windows, the file descriptor will not work with select().
int VPLNamedSocketClient_GetFd(const VPLNamedSocketClient_t* socket);

#ifdef __cplusplus
}
#endif

#endif // include guard
