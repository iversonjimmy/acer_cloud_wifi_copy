//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

/** @file
 * Linux APIs for socket operations.
 * The platform specific aspects of the APIs are in platform specific
 * headers and c files.
 */

#ifndef __VPL__SOCKET_H__
#define __VPL__SOCKET_H__

#ifdef IOS
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int fd;
} VPLSocket__t;
#define FMT_VPLSocket__t "%i"
#define VAL_VPLSocket__t(x) ((x).fd)
#define VPLSocket__AsFd(x) ((x).fd)

#define VPLSOCKET__SET_INVALID { -1 }
static const VPLSocket__t VPLSOCKET__INVALID = VPLSOCKET__SET_INVALID;

static inline int VPLSocket__Equal(VPLSocket__t a, VPLSocket__t b)
{
    return a.fd == b.fd;
}

#ifdef IOS

#define VPL__PF_UNSPEC AF_UNSPEC
#define VPL__PF_INET   PF_INET

#define VPLSOCKET__STREAM SOCK_STREAM
#define VPLSOCKET__DGRAM  SOCK_DGRAM

// should match the definition in sys/socket.h
#define VPLSOCKET__SHUT_RD   SHUT_RD
#define VPLSOCKET__SHUT_WR   SHUT_WR
#define VPLSOCKET__SHUT_RDWR SHUT_RDWR

/* level options for VPLSocket_SetSockOpt & VPLSocket_GetSockOpt */
/* should match the definitions in asm/socket.h and netinet/in.h */
#define VPLSOCKET__SOL_SOCKET   SOL_SOCKET
#define VPLSOCKET__IPPROTO_TCP  IPPROTO_TCP

/* should match the definitions in asm/socket.h */
#define VPLSOCKET__SO_BROADCAST    SO_BROADCAST
#define VPLSOCKET__SO_DEBUG        SO_DEBUG
#define VPLSOCKET__SO_ERROR        SO_ERROR
#define VPLSOCKET__SO_DONTROUTE    SO_DONTROUTE
#define VPLSOCKET__SO_KEEPALIVE    SO_KEEPALIVE
#define VPLSOCKET__SO_LINGER       SO_LINGER
#define VPLSOCKET__SO_OOBINLINE    SO_OOBINLINE
#define VPLSOCKET__SO_RCVBUF       SO_RCVBUF
#define VPLSOCKET__SO_REUSEADDR    SO_REUSEADDR
#define VPLSOCKET__SO_REUSEPORT    SO_REUSEPORT
#define VPLSOCKET__SO_SNDBUF       SO_SNDBUF
#define VPLSOCKET__TCP_NODELAY     TCP_NODELAY

//TODO: Can't find the following defination.
#define VPLSOCKET__IP_MTU_DISCOVER 10

#else // #ifdef IOS

#define VPL__PF_UNSPEC 0  // == PF_UNSPEC
#define VPL__PF_INET   2  // == PF_INET

#define VPLSOCKET__STREAM 1  // == SOCK_STREAM
#define VPLSOCKET__DGRAM  2  // == SOCK_DGRAM

// should match the definition in sys/socket.h
#define VPLSOCKET__SHUT_RD   0
#define VPLSOCKET__SHUT_WR   1
#define VPLSOCKET__SHUT_RDWR 2

/* level options for VPLSocket_SetSockOpt & VPLSocket_GetSockOpt */
/* should match the definitions in asm/socket.h and netinet/in.h */
#define VPLSOCKET__SOL_SOCKET	1
#define VPLSOCKET__IPPROTO_TCP	6

/* should match the definitions in asm/socket.h */
#define VPLSOCKET__SO_BROADCAST    6
#define VPLSOCKET__SO_DEBUG        1
#define VPLSOCKET__SO_ERROR        4
#define VPLSOCKET__SO_DONTROUTE    5
#define VPLSOCKET__SO_KEEPALIVE    9
#define VPLSOCKET__SO_LINGER       13
#define VPLSOCKET__SO_OOBINLINE    10
#define VPLSOCKET__SO_RCVBUF       8
#define VPLSOCKET__SO_REUSEADDR    2
#define VPLSOCKET__SO_SNDBUF       7
#define VPLSOCKET__TCP_NODELAY     1
#define VPLSOCKET__IP_MTU_DISCOVER 10

#endif // IOS

#ifdef __cplusplus
}
#endif

#endif // include guard
