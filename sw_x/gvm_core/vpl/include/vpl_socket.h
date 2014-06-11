//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_SOCKET_H__
#define __VPL_SOCKET_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for network socket operations.
/// Please see @ref VPLSocket.
//============================================================================

#include "vpl_plat.h"
#include "vpl_types.h"
#include "vpl_net.h"
#include "vpl_time.h"

#include "vpl__socket.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLSocket VPL Sockets API
/// The VPL Sockets API provides an abstraction to the system network layer.
///
/// It very closely resembles the BSD sockets API.  VPL sockets also include
/// #VPLSocket_Init() and #VPLSocket_Quit(); the VPL socket
/// library should be initialized before using VPL socket methods.  Other
/// minor changes are intended to generalize or improve aspects of BSD
/// sockets, for example separating error codes from output parameters.
/// VPL sockets uses the types defined in @ref VPLNet.
///
/// VPL provides for up to 64 simultaneously-open sockets.
/// Attempts to open more than 64 sockets will result in a VPL error.
/// VPL applications should be written to bound the number of sockets, such
/// that the application never hits this limit.
///
/// VPL provides a pre-determined pool of network buffers.
/// Each socket returned by #VPLSocket_Create() and
/// #VPLSocket_Listen() will have per-socket buffer limits which most
/// applications will not need to change.
/// Applications which call #VPLSocket_SetSockOpt() to customize the
/// per-socket buffer limits are responsible for not exceeding the
/// system-wide limit of 16Mbytes for all socket buffers.
///@{

/// Opaque type representing a VPL Socket.
typedef VPLSocket__t VPLSocket_t;
#define FMT_VPLSocket_t    FMT_VPLSocket__t
#define VAL_VPLSocket_t    VAL_VPLSocket__t
/// @note Warning: #VPLSocket_AsFd may not be supported on all platforms.
#define VPLSocket_AsFd     VPLSocket__AsFd
#define VPLSOCKET_INVALID  VPLSOCKET__INVALID
/// Use this in places where you need a constant expression for an initializer.
#define VPLSOCKET_SET_INVALID VPLSOCKET__SET_INVALID
//% Not an actual prototype to avoid redundant redeclaration warning
#define VPLSocket_Equal VPLSocket__Equal

/// IPv4 Socket Address Type
typedef struct {
    int            family;
    VPLNet_addr_t  addr;
    /// Port number, in network byte order (use #VPLNet_port_ntoh() before printing it).
    VPLNet_port_t  port;
} VPLSocket_addr_t;

/// @name Socket Protocol Family 
/// Values for the @a family argument to #VPLSocket_Create().
//@{

/// An "unspecified" protocol family. If passed to #VPLSocket_Create(),
/// VPL will choose an appropriate family.  Currently the only supported family is
/// #VPL_PF_INET.
#define VPL_PF_UNSPEC  VPL__PF_UNSPEC /// unspecified protocol family

/// The Protocol Family (PF) and Address Family (AF) IPv4.
#define VPL_PF_INET    VPL__PF_INET   /// Internet protocol Family (TCP/UDP)
//@}

/// @name Socket Shutdown Values
/// Values for the @a how argument to #VPLSocket_Shutdown().
//@{
#define VPLSOCKET_SHUT_RD   VPLSOCKET__SHUT_RD
#define VPLSOCKET_SHUT_WR   VPLSOCKET__SHUT_WR
#define VPLSOCKET_SHUT_RDWR VPLSOCKET__SHUT_RDWR
//@}

/// @name SockOpt Levels
/// Values for the @a level argument to #VPLSocket_SetSockOpt() and #VPLSocket_GetSockOpt().
//@{

/// VPLSocket options which take effect at the socket level.
/// These options are applicable to all VPL Sockets.
#define VPLSOCKET_SOL_SOCKET      VPLSOCKET__SOL_SOCKET

/// VPLSocket options which apply to TCP sockets, created by #VPLSocket_Create()
/// as @a family #VPL_PF_INET, and @a type #VPLSOCKET_STREAM.
#define VPLSOCKET_IPPROTO_TCP     VPLSOCKET__IPPROTO_TCP
//@}

/// @name Socket Options
/// Values for the @a optionName to #VPLSocket_SetSockOpt() and #VPLSocket_GetSockOpt().
//@{

/// Enable transmission and reception of broadcast packets on the socket.
#define VPLSOCKET_SO_BROADCAST    VPLSOCKET__SO_BROADCAST

/// Enable internal debugging on the socket.
#define VPLSOCKET_SO_DEBUG        VPLSOCKET__SO_DEBUG

/// Check socket error.
#define VPLSOCKET_SO_ERROR        VPLSOCKET__SO_ERROR

/// Instruct the OS to not perform routing for data written to the socket.
/// Data will be dropped if  the destination is not on the local network.
#define VPLSOCKET_SO_DONTROUTE    VPLSOCKET__SO_DONTROUTE

/// Send periodic "keepalive" messages.
/// Terminate the socket if "keepalives" are not received from the remote peer.
/// Applies only to sockets of type #VPLSOCKET_STREAM.
#define VPLSOCKET_SO_KEEPALIVE    VPLSOCKET__SO_KEEPALIVE

/// 
#define VPLSOCKET_SO_LINGER       VPLSOCKET__SO_LINGER
#define VPLSOCKET_SO_OOBINLINE    VPLSOCKET__SO_OOBINLINE

/// Set limit on receive-side buffering.
#define VPLSOCKET_SO_RCVBUF       VPLSOCKET__SO_RCVBUF
#define VPLSOCKET_SO_REUSEADDR    VPLSOCKET__SO_REUSEADDR
#define VPLSOCKET_SO_REUSEPORT    VPLSOCKET__SO_REUSEPORT
/// Set limit on send-side buffering.
#define VPLSOCKET_SO_SNDBUF       VPLSOCKET__SO_SNDBUF

/// Disable "Nagle" processing. Recommended for sockets used for
/// RPC protocols or other low-latency traffic. 
/// Applies only to sockets of type #VPLSOCKET_STREAM.
#define VPLSOCKET_TCP_NODELAY     VPLSOCKET__TCP_NODELAY

/// Enable MTU-discovery processing, as per Internet RFCs 1191, 1981, 4821.
#define VPLSOCKET_IP_MTU_DISCOVER VPLSOCKET__IP_MTU_DISCOVER
//@}

/// @name Socket types
/// Values for the @a type argument to #VPLSocket_Create().
//@{
/// Create a stream-oriented socket (TCP).
#define VPLSOCKET_STREAM     VPLSOCKET__STREAM

/// Create a datagram-oriented socket (UDP).
#define VPLSOCKET_DGRAM      VPLSOCKET__DGRAM
//@} 


/// Perform any platform-specific initialization for the use of sockets.
/// Correctly-written VPL applications must call #VPLSocket_Init() before
/// issuing any other VPLSocket calls.
///
/// @return #VPL_OK Success.
/// @return #VPL_ERR_FAIL Initialization failed.
int VPLSocket_Init(void);

/// Perform any platform-specific cleanup for the use of sockets.
/// @return #VPL_OK Success.
/// @return #VPL_ERR_FAIL Cleanup failed.
int VPLSocket_Quit(void);

/// Create a socket of a given type.
/// @param family Socket family type.
/// @param type Type of socket (#VPLSOCKET_STREAM or #VPLSOCKET_DGRAM).
/// @param nonblock If #VPL_TRUE, make the socket nonblocking.
/// @return Valid Socket identifier on success
/// @return #VPLSOCKET_INVALID on failure.
/// @note The created socket will typically be passed to #VPLSocket_Connect() to connect
///       to a server.  Or to create a local "server" socket, call  #VPLSocket_Listen().
//% VPLex note: If the socket is to be bound to a port to which other processes
//%       connect, use #VPLSocket_CreateTcp() or #VPLSocket_CreateUdp().
VPLSocket_t VPLSocket_Create(int family, int type, VPL_BOOL nonblock);

/// Close the given socket.
/// @param socket Socket to close.
/// @return #VPL_OK Socket closed successfully.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_INTR The function was interrupted by a system signal.
int VPLSocket_Close(VPLSocket_t socket);

/// Shutdown transmissions or receptions of a given socket.
/// @param[in] socket Socket to shutdown.
/// @param[in] how One of (#VPLSOCKET_SHUT_RD, #VPLSOCKET_SHUT_WR, #VPLSOCKET_SHUT_RDWR).
/// @return #VPL_OK Socket shutdown successfully.
/// @return #VPL_ERR_BADF @a socket is a bad socket identifier.
/// @return #VPL_ERR_NOTCONN @a socket is not connected.
/// @return #VPL_ERR_NOTSOCK @a socket does not identify a socket.
int VPLSocket_Shutdown(VPLSocket_t socket, int how);

/// Bind a socket to an address and port.
/// @param[in] socket Socket to bind.
/// @param[in] addr Address to bind to. 
///            The only supported value for @a addr.family is #VPL_PF_INET.
///              @a addr.addr should be one of: 
///            - An address of this host, as returned by #VPLNet_GetLocalAddrList()
///            - #VPLNET_ADDR_ANY,
///            - #VPLNET_ADDR_LOOPBACK,
///            - A multicast address in the range
///              #VPLNET_ADDR_ALLHOSTS_GROUP...#VPLNET_ADDR_MAX_LOCAL_GROUP
/// @param[in] addrSize sizeof(addr)
/// @return #VPL_OK Socket bound to address successfully.
/// @return #VPL_ERR_ACCESS the @a addr is protected.
/// @return #VPL_ERR_ADDRINUSE @a addr is already in use.
/// @return #VPL_ERR_ADDRNOTAVAIL A nonexistent interface was requested or @a addr is not local.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_INVALID @a addr is NULL,
///                          or @a socket is already bound to an address.
/// @return #VPL_ERR_NOTSOCK @a socket is not a socket.
int VPLSocket_Bind(VPLSocket_t socket, const VPLSocket_addr_t* addr, size_t addrSize);

/// Change socket options.
/// @param[in] socket Socket to modify.
/// @param[in] level The level at which the option is defined (for example, #VPLSOCKET_SOL_SOCKET).
/// @param[in] optionName The socket option to be modified.
/// @param[in] optionValue A pointer to a buffer in which the new option value is defined.
/// @param[in] optionLen The size in bytes of the @a optionValue buffer.
/// @return #VPL_OK The option was set successfully.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_INVALID @a optionName is invalid at the specified @a level or @a socket has been shutdown.
/// @return #VPL_ERR_ISCONN @a socket is already connected and @a optionName cannot be set while @a socket is connected.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
/// @return #VPL_ERR_NOMEM There is not enough memory available to complete the operation.
/// @return #VPL_ERR_NOBUFS There are not enough system resources available to complete the operation.
int VPLSocket_SetSockOpt(VPLSocket_t socket, int level, int optionName,
                         const void* optionValue, unsigned int optionLen);

/// Query socket option values.
/// @param[in] socket Socket to query.
/// @param[in] level The level at which the option is defined (for example, #VPLSOCKET_SOL_SOCKET).
/// @param[in] optionName The socket option to retrieve.
/// @param[out] optionValue Location to store the value of the specified option.
/// @param[in] optionLen The size in bytes of the @a optionValue buffer.
/// @return #VPL_OK The option was retrieved successfully.
/// @return #VPL_ERR_ACCESS The calling process does not have the correct privileges.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_INVALID @a socket has been shutdown.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
/// @return #VPL_ERR_NOBUFS There are not enough system resources available to complete the operation.
int VPLSocket_GetSockOpt(VPLSocket_t socket, int level, int optionName,
                         void* optionValue, unsigned int optionLen);

/// Set the TCP keep-alive parameters for a TCP stream socket.
/// @param[in] socket Socket to set for keep-alive.
/// @param[in] enable When nonzero, enable keep-alive.
/// @param[in] waitSec Idle time (seconds) to wait until sending first keep-alive probe.
/// @param[in] intervalSec Interval (seconds) between keep-alive probes.
/// @param[in] count Number of failed probes before declaring the socket closed.
/// @note @a count is not settable on Windows Vista or later; the value is fixed at 10, regardless
///   of what you specify.
/// @return #VPL_OK The option was retrieved successfully.
/// @return #VPL_ERR_ACCESS The calling process does not have the correct privileges.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_INVALID @a socket has been shutdown.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
/// @return #VPL_ERR_NOBUFS There are not enough system resources available to complete the operation.
int VPLSocket_SetKeepAlive(VPLSocket_t socket, int enable, int waitSec,
                           int intervalSec, int count);

//% NOTE: not currently implemented for Windows
/// Get the TCP keep-alive parameters for a TCP stream socket.
/// @param[in] socket Socket to set for keep-alive.
/// @param[out] enable When nonzero, keep-alive is enabled.
/// @param[out] waitSec Idle time (seconds) to wait until sending first keep-alive probe.
/// @param[out] intervalSec Interval (seconds) between keep-alive probes.
/// @param[out] count Number of failed probes before declaring the socket closed.
/// @return #VPL_OK The option was retrieved successfully.
/// @return #VPL_ERR_ACCESS The calling process does not have the correct privileges.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_INVALID @a socket has been shutdown.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
/// @return #VPL_ERR_NOBUFS There are not enough system resources available to complete the operation.
int VPLSocket_GetKeepAlive(VPLSocket_t socket, int* enable, int* waitSec,
                           int* intervalSec, int* count);

/// Connect a socket to a given Internet address and port.
/// Blocks until connected or an error occurs, even for non-blocking sockets.
/// @param[in] socket Socket to connect.
/// @param[in] addr Address to connect to.
/// @param[in] addrSize sizeof(*addr)
/// @return #VPL_OK Success.
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
int VPLSocket_Connect(VPLSocket_t socket, const VPLSocket_addr_t* addr, size_t addrSize);

/// Same as VPLSocket_Connect() except that this function takes a specific timeout
int VPLSocket_ConnectWithTimeout(VPLSocket_t socket, const VPLSocket_addr_t* addr, size_t addrSize, VPLTime_t timeout);

/// Same as VPLSocket_Connect() except that this function takes two specific timeouts, one for nonroutable addresses and another for routable addresses.
int VPLSocket_ConnectWithTimeouts(VPLSocket_t socket, const VPLSocket_addr_t* addr, size_t addrSize, VPLTime_t timeout_nonroutable, VPLTime_t timeout_routable);

/// Listen for incoming connections on a socket.
/// @param socket Socket to listen on.
/// @param backlog Maximum number of connections to allow pending 
///               (not accepted) at a time.
/// @return #VPL_OK if listening
/// @return #VPL_ERR_ACCESS The calling process does not have the appropriate privileges
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier
/// @return #VPL_ERR_DESTADDRREQ @a socket is not bound to a local address and the protocol does not support listening on an unbound socket
/// @return #VPL_ERR_INVALID @a socket is already connected or has been shutdown
/// @return #VPL_ERR_NOBUFS There are insufficient system resources to complete the call
/// @return #VPL_ERR_NOTSOCK @a socket is not a socket
/// @return #VPL_ERR_OPNOTSUPPORTED The socket protocol does not support listen
int VPLSocket_Listen(VPLSocket_t socket, int backlog);

/// Accept an incoming connection.
/// @param socket Socket from which to accept.
/// @param addr Pointer for return of sender address, if desired.
/// @param addrSize sizeof(*addr) if addr was passed non-NULL.
/// @param[out] connectedSocket On success, initialized to refer to the accepted socket.
/// @return #VPL_OK Success; @a connectedSocket will be initialized to refer to the accepted socket.
/// @return #VPL_ERR_AGAIN @a socket is non-blocking and no connections are present to be accepted.
/// @return #VPL_ERR_BADF @a socket is not a valid socket.
/// @return #VPL_ERR_MAX  The requesting process has reached its
///                         limit of #VPLSOCKET_MAX_SOCKETS concurrently-active sockets.
/// @return #VPL_ERR_FAULT @a addr is not a validly formatted address.
/// @return #VPL_ERR_INTR A system-signal interrupted the call.
/// @return #VPL_ERR_INVALID #VPLSocket_Listen() was not called on @a socket prior to this call,
///                          or @a connectedSocket was NULL
/// @return #VPL_ERR_NETDOWN The network system has failed
/// @return #VPL_ERR_NOBUFS Not enough system-resources to complete this call
/// @return #VPL_ERR_NOTSOCK @a socket is not a socket
/// @return #VPL_ERR_OPNOTSUPPORTED @a socket is not a type that supports connections
/// @note The accepted connection socket will have the same nonblocking
///       property as the parent socket.
int VPLSocket_Accept(VPLSocket_t socket,
                     VPLSocket_addr_t* addr, size_t addrSize,
                     VPLSocket_t* connectedSocket);

/// @name Socket-Event types
///  Values for the #VPLSocket_poll_t#revents field in the @a socketList argument of #VPLSocket_Poll().
//@{
/// Normal data is waiting to be read.
#define VPLSOCKET_POLL_RDNORM   0x0001
/// OOB data is waiting to be read.
#define VPLSOCKET_POLL_RDPRI    0x0002
/// Data can be written without blocking.
#define VPLSOCKET_POLL_OUT      0x0004
/// An error has occurred (output only).
#define VPLSOCKET_POLL_ERR      0x0100
/// A stream-oriented connection went away (output only).
#define VPLSOCKET_POLL_HUP      0x0200
/// An invalid socket was specified (output only).
#define VPLSOCKET_POLL_SO_INVAL 0x0400
/// Invalid event flags were specified (output only).
#define VPLSOCKET_POLL_EV_INVAL 0x0800
//@}

/// Structure to specify socket activity to watch for, in #VPLSocket_Poll.
typedef struct {
    /// The socket of interest.  The #VPLSocket_poll_t is ignored if this is set to #VPLSOCKET_INVALID.
    VPLSocket_t socket;
    /// Flags specifying events of interest.
    uint16_t events;
    /// Flags which will contain the socket status after #VPLSocket_Poll.
    uint16_t revents;
} VPLSocket_poll_t;

/// Blocking call to wait for an actionable status on a set of sockets.
/// @param socketList An array of @a numSockets #VPLSocket_poll_t structures to
///        specify the activity to wait for on each socket.
/// @param numSockets The length of the @a socketList array.
///        Due to Windows platform limitations, lengths larger than 64 may be truncated to 64.
/// @param timeout Maximum time (microseconds) to wait for actionable status.
///        - If @a timeout is zero, #VPLSocket_Poll() will fill in the @a revents members
///          of @a socketList and return immediately, without blocking.
///        - If @a timeout is #VPL_TIMEOUT_NONE, #VPLSocket_Poll() will block.
/// @return On success: a positive number indicating the total number of sockets
///         that have been selected (i.e. descriptors for which the "revents" member
///         is non-zero). A value of zero indicates a timeout with no sockets selected.
/// @return #VPL_ERR_AGAIN The allocation of internal data structures failed but a subsequent
///         call to #VPLSocket_Poll() may succeed.
/// @return #VPL_ERR_INTR A system signal interrupted the call.
/// @return #VPL_ERR_INVALID @a numSockets is too big, or one of the @a socketList members
///         refers to a stream of multiplexer that is linked downstream from a multiplexer.
//% TODO: underlying platforms' poll implementations may be inconsistent about
//% closed connections.  See http://www.greenend.org.uk/rjk/2001/06/poll.html
//% We should define and enforce a strict API here.
// Caution: According to the man page:
// Under Linux, select() may report a socket file descriptor as "ready for reading", while
// nevertheless a subsequent read blocks. This could for example happen when data has arrived but
// upon examination has wrong checksum and is discarded. There may be other circumstances in which
// a file descriptor is spuriously reported as ready. Thus it may be safer to use O_NONBLOCK on
// sockets that should not block.
int VPLSocket_Poll(VPLSocket_poll_t* socketList, int numSockets, VPLTime_t timeout);

/// Send a buffer on a given connected socket.
/// @param socket Socket to send on.
/// @param msg Pointer to the message body.
/// @param len Length of the message, in bytes.
/// @return Number of bytes sent on success.
/// @return #VPL_ERR_AGAIN @a socket is marked non-blocking and the operation would block.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_CONNRESET The connection was reset by its peer.
/// @return #VPL_ERR_INTR A system signal interrupted the call before any data was sent.
/// @return #VPL_ERR_IO An I/O error occurred.
/// @return #VPL_ERR_MSGSIZE The message is too large to be sent all at once.
/// @return #VPL_ERR_NETDOWN The local network interface is down.
/// @return #VPL_ERR_NOBUFS There are not enough system resources available to complete the call.
/// @return #VPL_ERR_NOTCONN @a socket does not refer to a connected socket.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
/// @return #VPL_ERR_UNREACH No route to the network is present.
/// @note This call may not send all bytes. Check return value to see how
///       much got sent, and re-call when ready to send more if necessary.
int VPLSocket_Send(VPLSocket_t socket, const void* msg, int len);

/// Send a buffer to a given address and port.
/// @param socket Socket to use.
/// @param addr Destination address.
/// @param addrSize sizeof(*addr).
/// @param buf Pointer to buffer containing bytes to send.
/// @param len Size of the buffer, in bytes.
/// @return Number of bytes sent on success.
/// @return #VPL_ERR_AGAIN @a socket is marked non-blocking and the operation would block.
/// @return #VPL_ERR_BADF @a socket is an invalid socket identifier.
/// @return #VPL_ERR_CONNRESET The connection was reset by its peer.
/// @return #VPL_ERR_DESTADDRREQ The socket is not in connected mode and no peer address is set.
/// @return #VPL_ERR_INTR A system signal interrupted the call before any data was sent.
/// @return #VPL_ERR_INVALID @a addr was NULL.
/// @return #VPL_ERR_IO An I/O error occurred.
/// @return #VPL_ERR_MSGSIZE The message is too large to be sent all at once.
/// @return #VPL_ERR_NETDOWN The local network interface is down.
/// @return #VPL_ERR_NOBUFS There are not enough system resources available to complete the call.
/// @return #VPL_ERR_NOMEM Not enough memory available to complete the call.
/// @return #VPL_ERR_NOTCONN @a socket does not refer to a connected socket.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
/// @return #VPL_ERR_UNREACH No route to @a addr is present.
/// @note This call may not send all bytes. Check return value to see how
///       much got sent, and re-call when ready to send more if necessary.
int VPLSocket_SendTo(VPLSocket_t socket, const void* buf, int len,
                     const VPLSocket_addr_t* addr, size_t addrSize);

/// Receive a buffer on a (connected) socket.
/// @param socket Socket to read.
/// @param buf Pointer to buffer to put read bytes.
/// @param len Size of the buffer, in bytes.
/// @return Number of bytes received on success.
/// @return #VPL_ERR_AGAIN @a socket is marked non-blocking and the call would block.
/// @return #VPL_ERR_BADF @a socket is not a valid socket identifier.
/// @return #VPL_ERR_CONNRESET The connection was reset by its peer.
/// @return #VPL_ERR_INTR The call was interrupted by a system signal.
/// @return #VPL_ERR_NOTCONN @a socket does not refer to a connected socket.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
int VPLSocket_Recv(VPLSocket_t socket, void* buf, int len);

/// Receive a buffer on a socket and report where the bytes came from.
/// @param socket Socket to read.
/// @param buf Pointer to buffer to put read bytes.
/// @param len Size of the buffer, in bytes.
/// @param senderAddr Pointer for return of sender address, if desired.
/// @param senderAddrSize sizeof(*senderAddr) if @a senderAddr is non-NULL.
/// @return Number of bytes read on success.
/// @return #VPL_ERR_AGAIN @a socket is marked non-blocking and the call would block.
/// @return #VPL_ERR_BADF @a socket is not a valid socket identifier.
/// @return #VPL_ERR_CONNRESET The connection was reset by its peer.
/// @return #VPL_ERR_INTR The call was interrupted by a system signal.
/// @return #VPL_ERR_NOBUFS There are not enough system resources available to complete this call.
/// @return #VPL_ERR_NOMEM There is insufficient memory to complete the call.
/// @return #VPL_ERR_NOTCONN @a socket does not refer to a connected socket.
/// @return #VPL_ERR_NOTSOCK @a socket does not refer to a socket.
int VPLSocket_RecvFrom(VPLSocket_t socket, void* buf, int len,
                       VPLSocket_addr_t* senderAddr, size_t senderAddrSize);

/// Looks up the local port corresponding to a given socket.
/// @param socket The socket.
/// @return A valid port on success. Port is in network byte order.
/// @return #VPLNET_PORT_INVALID if no port.
VPLNet_port_t VPLSocket_GetPort(VPLSocket_t socket);

/// Looks up the local address corresponding to a given socket.
/// @param socket The socket.
/// @return A valid address on success. Address is in network byte order.
/// @return #VPLNET_ADDR_INVALID if no port.
VPLNet_addr_t VPLSocket_GetAddr(VPLSocket_t socket);

//% end of VPL-Sockets Doxygen group
///@}

/*
 *  Get the address and port of the peer socket that is
 *  connected to the given socket, similar to the POSIX
 *  getpeername interface.
 */
VPLNet_port_t VPLSocket_GetPeerPort(VPLSocket_t socket);
VPLNet_addr_t VPLSocket_GetPeerAddr(VPLSocket_t socket);

#ifdef __cplusplus
}
#endif

#endif // include guard
