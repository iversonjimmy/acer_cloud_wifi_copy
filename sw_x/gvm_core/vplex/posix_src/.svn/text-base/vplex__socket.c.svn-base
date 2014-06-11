/*
 *                Copyright (C) 2008, BroadOn Communications Corp.
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of BroadOn Communications Corp.,
 *   and are protected by Federal copyright law. They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or in
 *   part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#ifdef IOS

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <vpl_th.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "vplex_socket.h"
#include "vplex_mem_utils.h"

VPLSocket_t VPLSocket_CreateUdp(VPLNet_addr_t addr, VPLNet_port_t port)
{
    VPLSocket_t sockfd;
    return sockfd; //TODO: Porting for iOS.
}

#else

#include "vplex_socket.h"
#include "vplex_mem_utils.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <vpl_th.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

VPLSocket_t VPLSocket_CreateUdp(VPLNet_addr_t addr, VPLNet_port_t port)
{
    VPLSocket_t sockfd;
    VPLSocket_addr_t sock_addr;
    int nonblock = 1;
    int rv = 0;
    sockfd = VPLSocket_Create(PF_INET, SOCK_DGRAM, nonblock);

    if (!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        /* Don't do MTU discovery. Prevents Linux from doing it for this UDP
         * port and fouling up our sends and retransmits.
         */
        int mtu_discover = 0;
        setsockopt(sockfd.fd, SOL_SOCKET, IP_MTU_DISCOVER,
                   (void*)&mtu_discover, sizeof(mtu_discover));

        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = addr;
        sock_addr.port = VPLNet_port_hton(port);
        rv = VPLSocket_Bind(sockfd, &sock_addr, sizeof(sock_addr));
        if (rv < 0) {
            VPLSocket_Close(sockfd);
            return VPLSOCKET_INVALID;
        }
    }

    return sockfd;
}
#endif

VPLSocket_t VPLSocket_CreateTcp(VPLNet_addr_t addr, VPLNet_port_t port)
{
    VPLSocket_t sockfd;
    VPLSocket_addr_t sock_addr;
    int nonblock = 1;
    int reuse = 1;
    int nodelay = 1;

    sockfd = VPLSocket_Create(PF_INET, SOCK_STREAM, nonblock);

    if (!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        /* Set socket reuse addr */
        setsockopt(sockfd.fd, SOL_SOCKET, SO_REUSEADDR,
                   (void*) &reuse, sizeof(reuse));

        /* Set TCP no delay */
        setsockopt(sockfd.fd, IPPROTO_TCP, TCP_NODELAY,
                (void*) &nodelay, sizeof(nodelay));
        if ((addr != VPLNET_ADDR_INVALID) || (port != VPLNET_PORT_INVALID)) {
            sock_addr.family = VPL_PF_INET;
#ifdef IOS
            // see Bug 2419 for why
            if (addr == VPLNET_ADDR_LOOPBACK) {
                sock_addr.addr = VPLNET_ADDR_ANY;
            } else {
                sock_addr.addr = addr;
            }
#else
            sock_addr.addr = addr;
#endif
            sock_addr.port = VPLNet_port_hton(port);
            int rv = VPLSocket_Bind(sockfd, &sock_addr, sizeof(sock_addr));
            if (rv < 0) {
                VPLSocket_Close(sockfd);
                return VPLSOCKET_INVALID;
            }
        }
    }

    return sockfd;
}

int 
VPLSocket_ConnectNowait(VPLSocket_t sockfd, const VPLSocket_addr_t* addr, size_t addrSize)
{
    int rv = VPL_OK;
    struct sockaddr_in sin;
    struct sockaddr sa;
    socklen_t sin_size = sizeof(sin);
    int err;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if (addr == NULL) {
        return VPL_ERR_INVALID;
    }
    UNUSED(addrSize);
    
    memset(&sin, 0, sin_size);
    sin.sin_family = AF_INET;
    sin.sin_port = addr->port;
    sin.sin_addr.s_addr = addr->addr;
    memcpy(&sa, &sin, sin_size);
    
    rv = connect(sockfd.fd, (struct sockaddr*) &sa, sin_size);
    if(rv != 0) {
        err = errno;

        if (err) {        
            rv = VPLError_XlatErrno(err);
        }
    }
    
    return rv;
}
