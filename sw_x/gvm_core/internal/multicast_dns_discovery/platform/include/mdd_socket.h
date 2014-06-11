#ifndef mdd_socket_h
#define mdd_socket_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mdd_net.h"
#include "mdd_iface.h"
#include "mdd_socket_platform.h"

typedef MDDSocket_plarform_t MDDSocket_t;

#define MDDSocket_INVALID MDDSocket_INVALID_PLATFORM

typedef struct {
    MDDNetProto_t family;
    // port number in network byte order 
    unsigned short port;
    // address
    MDDNetAddr_t addr;
} MDDSocketAddr_t;

/// Protocol type for socket
#define MDDSocket_SOCK_DGRAM MDDSocket_SOCK_DGRAM_PLATFORM

/// Levels for socket option
#define MDDSocket_IPPROTO_IP MDDSocket_IPPROTO_IP_PLATFORM
#define MDDSocket_IPPROTO_IPV6 MDDSocket_IPPROTO_IPV6_PLATFORM
#define MDDSocket_SOL_SOCKET MDDSocket_SOL_SOCKET_PLATFORM

/// Options for socket option
#define MDDSocket_IP_ADD_MEMBERSHIP MDDSocket_IP_ADD_MEMBERSHIP_PLATFORM
#define MDDSocket_IP_DROP_MEMBERSHIP MDDSocket_IP_DROP_MEMBERSHIP_PLATFORM
#define MDDSocket_IP_MULTICAST_TTL MDDSocket_IP_MULTICAST_TTL_PLATFORM
#define MDDSocket_IP_TTL MDDSocket_IP_TTL_PLATFORM
#define MDDSocket_IP_PKTINFO MDDSocket_IP_PKTINFO_PLATFORM

#define MDDSocket_IPV6_ADD_MEMBERSHIP MDDSocket_IPV6_ADD_MEMBERSHIP_PLATFORM
#define MDDSocket_IPV6_DROP_MEMBERSHIP MDDSocket_IPV6_DROP_MEMBERSHIP_PLATFORM
#define MDDSocket_IPV6_MULTICAST_HOPS MDDSocket_IPV6_MULTICAST_HOPS_PLATFORM
#define MDDSocket_IPV6_UNICAST_HOPS MDDSocket_IPV6_UNICAST_HOPS_PLATFORM
#define MDDSocket_IPV6_PKTINFO MDDSocket_IPV6_PKTINFO_PLATFORM
#define MDDSocket_IPV6_RECVPKTINFO MDDSocket_IPV6_RECVPKTINFO_PLATFORM

#define MDDSocket_SO_REUSEPORT MDDSocket_SO_REUSEPORT_PLATFORM
#define MDDSocket_SO_REUSEADDR MDDSocket_SO_REUSEADDR_PLATFORM

/// Init the stuffs which related to socket operation
int MDDSocket_Init(void);
/// Release all stuffs which related to socket operation
int MDDSocket_Quit(void);
/// Create socket
MDDSocket_t MDDSocket_Create(MDDNetProto_t family, int type);
/// Set socket option
int MDDSocket_Setsockopt(MDDSocket_t socket, int level, int optname, const void *optval, int optlen);
/// Bind address to socket
int MDDSocket_Bind(MDDSocket_t socket, MDDSocketAddr_t *my_addr);
/// Close socket
int MDDSocket_Close(MDDSocket_t socket);
/// Send data to socket with specific address, and support to specific network interface (Require IP_PKTINFO or IPV6_PKTINFO)
int MDDSocket_Sendto(MDDSocket_t socket, unsigned char *buf, int buf_len, MDDSocketAddr_t *to, MDDIface_t *iface);
/// Receive data from socket, and support to specific network interface (Require IP_PKTINFO or IPV6_PKTINFO)
int MDDSocket_Recvfrom(MDDSocket_t socket, unsigned char *buf, int len, MDDSocketAddr_t *from, MDDIface_t *iface);
/// Set non block for the socket
int MDDSocket_Setnonblock(MDDSocket_t socket);
/// Join/Remove specific interface to multicast group
int MDDSocket_Jointomulticast(MDDSocket_t socket, int join, MDDNetAddr_t *multicast_addr, MDDIface_t *iface);
/// Returns the current address to which the socket is bound
int MDDSocket_Getsockname(MDDSocket_t socket, MDDSocketAddr_t *addr);

#ifdef __cplusplus
}
#endif

#endif //mdd_socket_h
