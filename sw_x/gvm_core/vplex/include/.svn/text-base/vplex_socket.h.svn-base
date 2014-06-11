//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_SOCKET_H__
#define __VPLEX_SOCKET_H__

#include <vpl_socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Opens a new non-blocking UDP socket and binds it.
/// @param addr Address to bind to.
/// @param port Port to bind to, in host byte order.
/// @return Socket ID or #VPLSOCKET_INVALID.
/// @note MTU discovery is disabled for the socket.
VPLSocket_t VPLSocket_CreateUdp(VPLNet_addr_t addr, VPLNet_port_t port);

/// Opens a new non-blocking TCP socket and binds it.
/// @param addr Address to bind to.
/// @param port Port to bind to, in host byte order.
/// @return Socket ID or #VPLSOCKET_INVALID
/// @note The following options are used for the socket:
///       Reuse the local address when binding if not in active use.
///       Send data as soon as possible, even for small amounts of data.
VPLSocket_t VPLSocket_CreateTcp(VPLNet_addr_t addr, VPLNet_port_t port);

/// Attempt to read up to @a len bytes from the socket (for TCP), or until EOF is received.
/// @param socket Socket to read.
/// @param buf Pointer to buffer to put read bytes.
/// @param len Size of the buffer in bytes.
/// @param timeout Abort the operation if this amount of time passes and we have not received
///     the specified number of bytes or EOF.
/// @return Positive number of bytes read on success.  Note that this can be less than @a len if
///     the socket was closed remotely and we received EOF.
/// @return #VPL_ERR_TIMEOUT The timeout was reached before receiving the specified number of bytes
///     or EOF.
/// @return #VPL_ERR_BADF @a socket is either an invalid socket or it is not valid for reading.
/// @return #VPL_ERR_IO A physical I/O error has occurred.
/// @return #VPL_ERR_NOMEM Not enough memory is available.
/// @return #VPL_ERR_NOTCONN @a socket is not connected.
/// @note This call is designed to block until the requested bytes have been
/// read or an error occurs, even with nonblocking sockets.
/// In order to read the requested number of bytes, this function may issue
/// more than one "read" call to the underlying socket stack.
/// Use #VPLSocket_Recv() or #VPLSocket_RecvFrom() to read only what is
/// available for non-blocking sockets, or to read from any #VPLSOCKET_DGRAM socket.
int VPLSocket_Read(VPLSocket_t socket, void* buf, int len, VPLTime_t timeout);

/// Attempt to write @a len bytes to the socket (for TCP).
/// @param socket Socket to use.
/// @param buf Pointer to the message body.
/// @param len Length of the message, in bytes.
/// @return Positive number of bytes written on success.
/// @return #VPL_ERR_AGAIN A nonblocking flag has been set for the underlying stream and the thread would be delayed waiting for #VPLSocket_Write() to return
/// @return #VPL_ERR_BADF @a socket is either an invalid socket or it was not opened for writing
/// @return #VPL_ERR_CONNRESET The connection used by @a socket was closed by its peer
/// @return #VPL_ERR_INTR The read was terminated due to a system call
/// @return #VPL_ERR_IO A physical I/O error has occurred
/// @return #VPL_ERR_NOMEM Not enough memory is available
/// @return #VPL_ERR_NETDOWN The local network interface is down
/// @return #VPL_ERR_UNREACH No route to the network is present
/// @note This call is designed to block until the requested bytes have been
/// written or an error occurs, even with nonblocking sockets.
/// Use #VPLSocket_Send() or #VPLSocket_SendTo() to write only as much as the
/// socket's outgoing buffer can send.
int VPLSocket_Write(VPLSocket_t socket, const void* buf, int len, VPLTime_t timeout);

/// Connect a socket to a given Internet address and port.
/// Returns immediately, even if connection is pending.
/// @param[in] socket Socket to connect.
/// @param[in] addr Address to connect to.
/// @param[in] addrSize sizeof(*addr)
/// @return #VPL_OK Success.
/// @return #VPL_ERR_BUSY connection in-progress. Poll socket for writability using #VPLSocket_Poll(). When write is ready for write, use #VPLSocket_GetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_ERROR, &value, sizeof(value)) to check connection status. value==0 means connected, otherwise one of the errors below is returned.
/// @return #VPL_ERR_ACCESS Write access to @a socket is denied.
/// @return #VPL_ERR_ADDRINUSE The specified addresses are already in use.
//% TODO: ?? ipaddr??
//% @return #VPL_ERR_ADDRNOTAVAIL @a ipaddr is not available from the local machine.
/// @return #VPL_ERR_BADF @a socket is not a valid socket identifier.
/// @return #VPL_ERR_CONNREFUSED The attempt to connect was ignored or explicitly refused.
/// @return #VPL_ERR_INTR The attempt to connect was interrupted by a system signal; the connection will be established asynchronously.
/// @return #VPL_ERR_INVALID @a addr is NULL.
/// @return #VPL_ERR_ISCONN @a socket is already connected.
/// @return #VPL_ERR_NETDOWN The local network interface is down.
/// @return #VPL_ERR_NETRESET Remote host has reset the connection request.
/// @return #VPL_ERR_NOBUFS No buffer space is available.
/// @return #VPL_ERR_NOTSOCK @a socket is not a socket identifier.
/// @return #VPL_ERR_OPNOTSUPPORTED @a socket is listening and cannot be connected.
/// @return #VPL_ERR_UNREACH No route to the network is present.
int VPLSocket_ConnectNowait(VPLSocket_t socket, const VPLSocket_addr_t* addr, size_t addrSize);

#ifdef __cplusplus
}
#endif

#endif // include guard
