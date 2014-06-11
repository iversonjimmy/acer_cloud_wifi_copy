#ifndef mdd_socket_platform_h
#define mdd_socket_platform_h

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#include <sys/socket.h>

typedef int MDDSocket_plarform_t;

#define MDDSocket_INVALID_PLATFORM -1

// Protocol type for socket
#define MDDSocket_SOCK_DGRAM_PLATFORM SOCK_DGRAM

// Levels for socket option
#define MDDSocket_IPPROTO_IP_PLATFORM IPPROTO_IP
#define MDDSocket_IPPROTO_IPV6_PLATFORM IPPROTO_IPV6
#define MDDSocket_SOL_SOCKET_PLATFORM SOL_SOCKET

// Options for socket option
#define MDDSocket_IP_ADD_MEMBERSHIP_PLATFORM IP_ADD_MEMBERSHIP
#define MDDSocket_IP_DROP_MEMBERSHIP_PLATFORM IP_DROP_MEMBERSHIP
#define MDDSocket_IP_MULTICAST_TTL_PLATFORM IP_MULTICAST_TTL
#define MDDSocket_IP_TTL_PLATFORM IP_TTL
#define MDDSocket_IP_PKTINFO_PLATFORM IP_PKTINFO

#define MDDSocket_IPV6_ADD_MEMBERSHIP_PLATFORM IPV6_ADD_MEMBERSHIP
#define MDDSocket_IPV6_DROP_MEMBERSHIP_PLATFORM IPV6_DROP_MEMBERSHIP
#define MDDSocket_IPV6_MULTICAST_HOPS_PLATFORM IPV6_MULTICAST_HOPS
#define MDDSocket_IPV6_UNICAST_HOPS_PLATFORM IPV6_UNICAST_HOPS
#define MDDSocket_IPV6_PKTINFO_PLATFORM IPV6_PKTINFO
#define MDDSocket_IPV6_RECVPKTINFO_PLATFORM IPV6_RECVPKTINFO

#define MDDSocket_SO_REUSEPORT_PLATFORM SO_REUSEPORT
#define MDDSocket_SO_REUSEADDR_PLATFORM SO_REUSEADDR

#ifdef __cplusplus
}
#endif

#endif //mdd_socket_platform_h
