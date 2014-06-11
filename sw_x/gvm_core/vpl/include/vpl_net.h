//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_NET_H__
#define __VPL_NET_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for network-address operations.
/// Please see @ref VPLNet.
//============================================================================

#include "vpl_plat.h"
#include "vpl_types.h"
#include "vpl_conv.h"
#include "vpl__net.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLNet VPL Network Address/Port Lookup API
/// Types and tools for network addresses and ports used with @ref VPLSocket.
///
/// Defines address and port types and constants used with the VPL socket API.
/// Provides tools to find addresses, resolve hostnames, get network
/// information, and manipulate VPL network types.  Currently only IPv4
/// addresses are supported.
///@{

/// @name VPL Network-Address Constants
//@{

/// Internet address, always in network byte order.
/// Currently, only IPv4 is supported.
typedef uint32_t VPLNet_addr_t;

#define FMT_VPLNet_addr_t "%u.%u.%u.%u"
#define VAL_VPLNet_addr_t(ip) ((uint8_t*)&(ip))[0], ((uint8_t*)&(ip))[1],((uint8_t*)&(ip))[2],((uint8_t*)&(ip))[3]

/// Invalid #VPLNet_addr_t.
#define VPLNET_ADDR_INVALID  ((VPLNet_addr_t) 0xffffffff)

#define VPLNET_ADDRSTRLEN 16 // ###.###.###.###\0

/// Unspecified address.
#define VPLNET_ADDR_ANY             ((VPLNet_addr_t) 0x00000000) // 0.0.0.0

/// Local-net broadcast address.
#define VPLNET_ADDR_BROADCAST       ((VPLNet_addr_t) 0xffffffff) // 255.255.255.255

/// Local-loopback address.
#define VPLNET_ADDR_LOOPBACK        ((VPLNet_addr_t) VPLConv_hton_u32(0x7f000001)) // 127.0.0.1

/// Multicast address.
#define VPLNET_ADDR_UNSPEC_GROUP    ((VPLNet_addr_t) VPLConv_hton_u32(0xe0000000)) // 224.0.0.0

///  All multicast hosts on the local network.
#define VPLNET_ADDR_ALLHOSTS_GROUP  ((VPLNet_addr_t) VPLConv_hton_u32(0xe0000001)) // 224.0.0.1

///  All routers on the local network.
#define VPLNET_ADDR_ALLRTRS_GROUP   (VPLNet__addr_t) VPLConv_hton_u32(0xe0000002)) // 224.0.0.2

/// Last multicast-to-local-network-only address.
#define VPLNET_ADDR_MAX_LOCAL_GROUP ((VPLNet_addr_t) VPLConv_hton_u32(0xe00000ff)) // 224.0.0.255

/// Evaluates an IP address (VPLNet_addr_t) to determine if it is a routable address.
/// Routable addresses are addresses not on any of the following ranges:
/// * 10.0.0.0 - 10.255.255.255
/// * 172.16.0.0.- 172.31.255.255
/// * 192.168.0.0 - 192.168.255.255
/// * 169.254.0.0 - 169.254.255.255
#define VPLNet_IsRoutableAddress(addr)     \
    (((((addr) & 0xff) == 10) ||           \
      ((((addr) & 0xff) == 172) &&         \
       ((((addr) >> 8) & 0xff) >= 16 &&    \
        (((addr) >> 8) & 0xff) < 32)) ||   \
      ((((addr) & 0xff) == 192) &&         \
       ((((addr) >> 8) & 0xff) == 168)) || \
      ((((addr) & 0xff) == 169) &&         \
       ((((addr) >> 8) & 0xff) == 254)) || \
      ((addr) == VPLNET_ADDR_LOOPBACK))    \
     ? 0 : 1)

//@}

/// A TCP or UDP port number.
/// @note This can be in either host byte order or network byte order, depending on the context.
typedef uint16_t VPLNet_port_t;
#define VPLNET_PORT_ANY  0x0000

#define FMT_VPLNet_port_t "%u"

/// Invalid #VPLNet_port_t
#define VPLNET_PORT_INVALID  0

/// Convert #VPLNet_port_t value @a port from network byte-order to (local) host byte-order.
/// @param[in] port  Valid port number, in network byte order.
/// @return @a port, converted to host byte-order
static inline VPLNet_port_t VPLNet_port_ntoh(VPLNet_port_t port) { return VPLConv_ntoh_u16(port); }

/// Convert #VPLNet_port_t value @a port from (local) host byte-order to network byte-order.
/// @param[in] port  Valid port number, in host byte-order
/// @return @a port, converted to network byte order.
static inline VPLNet_port_t VPLNet_port_hton(VPLNet_port_t port) { return VPLConv_hton_u16(port); }

/// Maximum number network interfaces supported by this API.
#define VPLNET_MAX_INTERFACES  VPLNET__MAX_INTERFACES

/// Get the IP address of the local machine.
/// @return A valid VPLNet_addr_t if successful.
/// @return #VPLNET_ADDR_INVALID if failure.
VPLNet_addr_t VPLNet_GetLocalAddr(void);

#ifndef VPL_PLAT_IS_WINRT
/// Get the default gateway.
/// @note Not supported on WinRT!
/// @return The gateway address if successful.
/// @return #VPLNET_ADDR_INVALID if the call failed.
VPLNet_addr_t VPLNet_GetDefaultGateway(void);
#endif

/// Get the list of IP addresses of the local machine.
/// @param ppAddrs Pointer to list of return addresses
///     (should be an array #VPLNET_MAX_INTERFACES long).
/// @param ppNetmasks Pointer to (optional) list of corresponding netmasks
///     (should be an array #VPLNET_MAX_INTERFACES long).
/// @return On success, positive number of addresses returned.
/// @return #VPL_ERR_ACCESS No permission to create a socket with which to find the address list.
/// @return #VPL_ERR_NOBUFS Insufficient resources to perform the call.
/// @return #VPL_ERR_NOMEM Insufficient memory to perform the call.
/// @return #VPLNET_ADDR_INVALID @a ppAddrs is NULL or invalid.
/// @note Up to #VPLNET_MAX_INTERFACES will be returned.
int VPLNet_GetLocalAddrList(VPLNet_addr_t* ppAddrs,
                            VPLNet_addr_t* ppNetmasks);

/// Return IPv4 address based on hostname.
/// @note Not supported on WinRT!
/// @param[in] hostname Pointer to hostname string, NULL terminated.
/// @return IPv4 address if successful.
/// @return #VPLNET_ADDR_INVALID if call failed.
VPLNet_addr_t VPLNet_GetAddr(const char* hostname);

/// Converts IPv4 address to string.
/// @param[in] src Pointer to an Internet address
/// @param[out] dest Pointer to a buffer to put the string for the src address.
/// @param[in] count Size of the string buffer.
/// @return Pointer to the converted string if successful.
/// @return NULL if conversion failed,
///         or if @a src or @a dest are NULL
///         or if @a count == 0.
const char* VPLNet_Ntop(const VPLNet_addr_t* src, char* dest, size_t count);

//% end of defgroup
///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
