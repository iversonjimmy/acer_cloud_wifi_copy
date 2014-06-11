#include "ConnectedSocketPair.hpp"

#include <log.h>

#include <vpl_net.h>
#include <vplex_socket.h>

ConnectedSocketPair::ConnectedSocketPair()
{
    for (int i = 0; i < ARRAY_ELEMENT_COUNT(sock); i++) {
        sock[i] = VPLSOCKET_INVALID;
    }
}

ConnectedSocketPair::~ConnectedSocketPair()
{
    for (int i = 0; i < ARRAY_ELEMENT_COUNT(sock); i++) {
        if (!VPLSocket_Equal(sock[i],  VPLSOCKET_INVALID)) {
            VPLSocket_Close(sock[i]);
            sock[i] = VPLSOCKET_INVALID;
        }
    }
}

int ConnectedSocketPair::GetSockets(VPLSocket_t _sock[2])
{
    if (VPLSocket_Equal(sock[0], VPLSOCKET_INVALID) ||
        VPLSocket_Equal(sock[1], VPLSOCKET_INVALID)) {
        create();
    }
    if (VPLSocket_Equal(sock[0], VPLSOCKET_INVALID) ||
        VPLSocket_Equal(sock[1], VPLSOCKET_INVALID)) {
        return VPL_ERR_FAIL;
    }
    for (int i = 0; i < ARRAY_ELEMENT_COUNT(sock); i++) {
        _sock[i] = sock[i];
    }

    return 0;
}

int ConnectedSocketPair::create()
{
    VPLSocket_t listenSock = VPLSOCKET_INVALID;
    {
        VPLNet_addr_t addr = VPLNET_ADDR_LOOPBACK;
        VPLNet_port_t port = VPLNET_PORT_ANY;
        listenSock = VPLSocket_CreateTcp(addr, port);
        if (VPLSocket_Equal(listenSock, VPLSOCKET_INVALID)) {
            LOG_ERROR("Failed to create listen socket");
            return VPL_ERR_SOCKET;
        }
    }

    {
        int err = VPLSocket_Listen(listenSock, 10);
        if (err) {
            LOG_ERROR("VPLSocket_Listen() failed on socket["FMT_VPLSocket_t"]: err %d", VAL_VPLSocket_t(listenSock), err);
            return err;
        }
    }

    VPLSocket_t clientSock = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, /*nonblocking*/VPL_TRUE);
    if (VPLSocket_Equal(clientSock, VPLSOCKET_INVALID)) {
        LOG_ERROR("Failed to create client-end socket");
        return VPL_ERR_SOCKET;
    }

    {
        VPLSocket_addr_t sin;
        sin.family = VPL_PF_INET;
        sin.addr = VPLNET_ADDR_LOOPBACK;
        sin.port = VPLSocket_GetPort(listenSock);
        int err = VPLSocket_Connect(clientSock, &sin, sizeof(sin));
        if (err) {
            LOG_ERROR("Failed to connect client socket to server: err %d", err);
            return err;
        }
    }

    VPLSocket_t serverSock = VPLSOCKET_INVALID;
    {
        VPLSocket_addr_t addr;
        int err = VPLSocket_Accept(listenSock, &addr, sizeof(addr), &serverSock);
        if (err) {
            LOG_ERROR("VPLSocket_Accept() failed: err %d", err);
            return err;
        }
    }

    sock[0] = clientSock;
    sock[1] = serverSock;
    return 0;
}
