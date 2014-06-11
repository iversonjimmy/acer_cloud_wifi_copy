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
 * APIs for socket operations.
 * The platform specific aspects of the APIs are in platform specific
 * headers and c files.
 */

#ifndef __VPL__SOCKET_H__
#define __VPL__SOCKET_H__

#include "vpl_plat.h"

#ifndef VPL_PLAT_IS_WINRT
#include "winsock2.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VPL_PLAT_IS_WINRT
typedef struct {
    int s;
} VPLSocket__t;
#define FMT_VPLSocket__t "%d"
#define VAL_VPLSocket__t(x) (((x).s))
#else
typedef struct {
    SOCKET s;
} VPLSocket__t;
#define FMT_VPLSocket__t "%p"
#define VAL_VPLSocket__t(x) ((void*)((x).s))
#endif

#ifdef _WIN64
// According to http://msdn.microsoft.com/en-us/library/ms724485%28VS.85%29.aspx
// , it is ok to convert SOCKET to int for win64
// because "The per-process limit on kernel handles is 2^24"
# define VPLSocket__AsFd(sock)  ((int)((sock).s))
#else
# define VPLSocket__AsFd(sock)  ((int)((sock).s))
#endif

#ifdef VPL_PLAT_IS_WINRT
#define VPLSOCKET__SET_INVALID { -1 }  // == INVALID_SOCKET
#else
#define VPLSOCKET__SET_INVALID { ~((SOCKET)0) }  // == INVALID_SOCKET
#endif

static const VPLSocket__t VPLSOCKET__INVALID = VPLSOCKET__SET_INVALID;

#define VPL__PF_UNSPEC 0  // == PF_UNSPEC
#define VPL__PF_INET   2  // == PF_INET

#define VPLSOCKET__STREAM 1  // == SOCK_STREAM
#define VPLSOCKET__DGRAM  2  // == SOCK_DGRAM
#ifdef VPL_PLAT_IS_WINRT
#define VPLSOCKET__STREAMLISTENER 3 // == StreamSocketListener
#define VPLSOCKET_STREAMLISTENER VPLSOCKET__STREAMLISTENER
#endif

static inline int VPLSocket__Equal(VPLSocket__t a, VPLSocket__t b)
{
    return a.s == b.s;
}

// should match the definition in Winsock2.h
#define VPLSOCKET__SHUT_RD   1
#define VPLSOCKET__SHUT_WR   0
#define VPLSOCKET__SHUT_RDWR 2

/* level options for VPLSocket_SetSockOpt & VPLSocket_GetSockOpt */
/* should match the definitions in ws2def.h */
#define VPLSOCKET__SOL_SOCKET	0xffff
#define VPLSOCKET__IPPROTO_TCP	6
#ifdef VPL_PLAT_IS_WINRT
/* option names for VPLSocket_SetSockOpt & VPLSocket_GetSockOpt */
#define VPLSOCKET__TCP_NODELAY     0x0001
#endif

// should match the definition in Ws2def.h
#define VPLSOCKET__SO_BROADCAST    0x0020
#define VPLSOCKET__SO_DEBUG        0x0001
#define VPLSOCKET__SO_ERROR        0x1007
#define VPLSOCKET__SO_DONTROUTE    0x0010
#define VPLSOCKET__SO_KEEPALIVE    0x0008
#define VPLSOCKET__SO_LINGER       0x0080
#define VPLSOCKET__SO_OOBINLINE    0x0100
#define VPLSOCKET__SO_RCVBUF       0x1002
// TODO: Bug 1442: SO_REUSEADDR in Windows is not the same as in Unix!
#define VPLSOCKET__SO_REUSEADDR    0x0004
#define VPLSOCKET__SO_SNDBUF       0x1001
#define VPLSOCKET__TCP_NODELAY     0x0001
#define VPLSOCKET__IP_DONTFRAGMENT 14 //from ws2ipdef.h  (also appears in WinSock.h with value == 9

#ifdef __cplusplus
}
#endif

#endif // include guard
