//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_IPC_SOCKET_H__
#define __VPLEX_IPC_SOCKET_H__

#include "vplex_plat.h"

//============================================================================
/// @file
// TODO: Bug 10159
/// This is primarily to continue supporting GVM code.
/// If we still care about the simulated send and receive delays, this file should
/// probably be rewritten to use #VPLSocket_t as the underlying type.
/// If we don't care about the simulated delays, consumers of this API should
/// probably switch to vpl_socket.h and this file can be removed.
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

typedef s32 IPCError;

#if !defined(WIN32)

typedef enum {
    IPC_SOCKET_TCP,
    IPC_SOCKET_UDP,
    IPC_SOCKET_UNIX, ///< Unix Domain Socket or Windows Named Pipe
} IPCSocketType;

typedef struct {
    u8              nonBlock;
    //s8              address[IPC_SOCKET_ADDRESS_LENGTH];
    u16             port;
    s32             fd;
    u32             sendDelay; // microseconds
    u32             recvDelay; // microseconds
    IPCSocketType   type;
} IPCSocket;

#define IPCSocket_CloseAndLog(sock) \
    BEGIN_MULTI_STATEMENT_MACRO \
    int temp_rv_ipcsocket_closeandlog; \
    if ((temp_rv_ipcsocket_closeandlog = IPCSocket_Close(sock)) != 0) { \
        VPL_LIB_LOG_ERR(VPL_SG_SOCKET, "IPCSocket_Close failed: %d", temp_rv_ipcsocket_closeandlog); \
    } \
    END_MULTI_STATEMENT_MACRO

///
/// @defgroup IpcSockets TCP, UDP, UNIX sockets
///@{
///
IPCError IPCSocket_Accept(IPCSocket *sock, IPCSocket *client, u8 nonBlock);

IPCError IPCSocket_Bind(IPCSocket *sock, const char *address, u16 port);

/// Close the socket and mark it as closed.  Additional calls to this function will immediately
/// return success.
IPCError IPCSocket_Close(IPCSocket *sock);

IPCError IPCSocket_Connect(IPCSocket *sock, const s8 *address, u16 port);

s8*      IPCSocket_GetAddress(const s8* hostName);

/// Set a newly-created IPC socket into listening mode, where the socket
/// acts as the "passive" side of a connection.  On success, the socket
/// is ready for a call to IPSocket_Accept(), to accept incoming
/// connection requests from a peer socket of the same protocol-family and type.
///
/// @return IPC_ERROR_OK:  success,
/// @return IPC_ERROR_PARAMETER: \a sock is null.
/// @return IPC_ERROR_PARAMETER: \a socket type does not support connections
/// @return IPC_ERROR_PARAMETER: \a backlog is less than the minimum supported value of 2.
IPCError IPCSocket_Listen(IPCSocket *sock, s32 backlog);

IPCError IPCSocket_Open(IPCSocket *sock, IPCSocketType type, u8 nonBlock);

/// Receive data on \a sock, into buffer \a buf, up to \a length bytes.
///
/// @return On success, a non-negative value, the length of data read into \a buf
/// @return IPC_ERROR_SOCKET_PARAMETER : invalid argument
/// @return IPC_ERROR_SOCKET_NONBLOCK : operation would block on non-blocking socket
/// @return IPC_ERROR_SOCKET_RECEIVE : other error, unspecified. (Connection closed, ...)
s32      IPCSocket_Receive(IPCSocket *sock, void *buffer, u32 length, s32 flags);

/// Receive data on \a sock,  remote peer \a address: \a port,  into buffer \a buf,
/// up to \a length bytes.
/// @return On success, a non-negative value, the length of data read into \a buf
/// @return IPC_ERROR_SOCKET_PARAMETER : invalid argument
/// @return IPC_ERROR_SOCKET_NONBLOCK : operation would block on non-blocking socket
/// @return IPC_ERROR_SOCKET_RECEIVE_FROM : other error, unspecified. (Connection closed, ...)
s32      IPCSocket_ReceiveFrom(IPCSocket *sock, const s8* address, u16 port, void *buffer, u32 length, s32 flags);

/// Send data on \a sock, from buffer \a buf, sending (up to) \a length bytes.
///
/// @return On success,a non-negative value,  the  length of data written to \a sock
/// @return IPC_ERROR_SOCKET_PARAMETER : invalid argument
/// @return IPC_ERROR_SOCKET_NONBLOCK : operation would block on non-blocking socket
/// @return IPC_ERROR_SOCKET_SEND : other error, unspecified. (Connection closed, ...)
s32      IPCSocket_Send(IPCSocket *sock, const void *buffer, u32 length, s32 flags);
s32      IPCSocket_SendFile(IPCSocket *sock, s32 fd, off_t *offset, size_t length);

/// Send data on \a sock, to  remote peer \a address: \a port,  from buffer \a buf,
/// up to \a length bytes.
/// @return On success,a non-negative value,  the  length of data written to \a sock
/// @return IPC_ERROR_SOCKET_PARAMETER : invalid argument
/// @return IPC_ERROR_SOCKET_NONBLOCK : operation would block on non-blocking socket
/// @return IPC_ERROR_SOCKET_SENDTO : other error, unspecified. (Connection closed, ...)
s32      IPCSocket_SendTo(IPCSocket *sock, const s8* address, u16 port, const void *buffer, u32 length, s32 flags);

IPCError IPCSocket_SetDelay(IPCSocket *sock, u32 send, u32 receive); // microseconds
///@}

#endif

#ifdef __cplusplus
}
#endif

#endif // include guard
