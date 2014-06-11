/*
 *   Copyright (C) 2010, BroadOn Communications Corp.
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of BroadOn Communications Corp.,
 *   and are protected by Federal copyright law. They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or in
 *   part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#include "vpl_plat.h"

#ifdef VPL_PLAT_IS_WINRT
#include "vpl__socket_priv.h"
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "vplex_socket.h"
#include "vpl_th.h"

VPLSocket_t VPLSocket_CreateUdp(VPLNet_addr_t addr, VPLNet_port_t port)
{
    VPLSocket_t sockfd;
    int nonblock = 1;
    int rv = 0;

    sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_DGRAM, nonblock);

    if (!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_addr_t bind_addr;
        bind_addr.family = VPL_PF_INET;
        bind_addr.addr = addr;
        bind_addr.port = VPLNet_port_hton(port);

        rv = VPLSocket_Bind(sockfd, &bind_addr, sizeof(bind_addr));
        if (rv < 0) {
            VPLSocket_Close(sockfd);
            return VPLSOCKET_INVALID;
        }
    }

    return sockfd;
}

VPLSocket_t VPLSocket_CreateTcp(VPLNet_addr_t addr, VPLNet_port_t port)
{
    VPLSocket_t sockfd;
    int nonblock = 1;
    int nodelay = 1;

    sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, nonblock);

    if (!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        /* Set TCP no delay */
        VPLSocket_SetSockOpt(sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                (void*) &nodelay, sizeof(nodelay) );
#ifndef VPL_PLAT_IS_WINRT
        {
            /* Set socket reuse addr */
            int reuse = 1;
            VPLSocket_SetSockOpt(sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_SO_REUSEADDR,
                (void*) &reuse, sizeof(reuse) );
        }
#endif
        if ((addr != VPLNET_ADDR_INVALID) || (port != VPLNET_PORT_INVALID)) {
            int rv;
            VPLSocket_addr_t bind_addr;
            bind_addr.family = VPL_PF_INET;
            bind_addr.addr = addr;
            bind_addr.port = VPLNet_port_hton(port);

            rv = VPLSocket_Bind(sockfd, &bind_addr, sizeof(bind_addr));
            if (rv < 0) {
                VPLSocket_Close(sockfd);
                return VPLSOCKET_INVALID;
            }
        }
    }

    return sockfd;
}

int VPLSocket_ConnectNowait(VPLSocket_t sockfd, const VPLSocket_addr_t *in_addr, size_t in_addr_size)
{
#ifdef VPL_PLAT_IS_WINRT
    int rv = VPL_ERR_BUSY;

    _SocketData^ data = GetSocketData(sockfd);
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }
    if ( in_addr == NULL ) {
        return VPL_ERR_INVALID;
    }
    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        return VPL_ERR_INVALID;
    }
    if( data->IsBlocking() ) {
        return VPL_ERR_INVALID;
    }

    //regard as TCP client
    if( data->m_type == VPLSOCKET_STREAM ) {
        data->CreateTCPClient();
    }

    String^ addr = _SocketData::AddrToString(in_addr->addr);
    String^ port = in_addr->port.ToString();

    return data->DoConnectNowait(addr,port);
#else
    int rv = VPL_OK;
    int rc;
    struct sockaddr_in sin;
    socklen_t sin_size = sizeof(sin);

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if ( in_addr == NULL ) {
        return VPL_ERR_INVALID;
    }

    memset(&sin, 0, sin_size);
    sin.sin_family = AF_INET;
    sin.sin_port = in_addr->port;
    sin.sin_addr.s_addr = in_addr->addr;

    // Don't call VPLSocket_Connect() here; we need this to be non-blocking,
    // but VPLSocket_Connect() is always blocking.
    rc = connect(sockfd.s, (struct sockaddr*) &sin, sin_size);
    if(rc == SOCKET_ERROR) {
        rv = VPLError_XlatSockErrno(WSAGetLastError());
    }

    return rv;
#endif
}
