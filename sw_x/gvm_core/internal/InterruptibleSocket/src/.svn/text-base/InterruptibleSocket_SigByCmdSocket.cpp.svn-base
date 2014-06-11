//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

/*
 * An implementation of InterruptibleSocket.
 * This implementation uses a command socket to signal exit.
 */

#include "InterruptibleSocket_SigByCmdSocket.hpp"

#include <log.h>

#include <vpl_error.h>
#include <vplex_socket.h>

#include <cassert>

InterruptibleSocket_SigByCmdSocket::InterruptibleSocket_SigByCmdSocket(VPLSocket_t socket)
    : InterruptibleSocket(socket), ioState(INTR_SOCKET_IO_OK), cmdSock_client(VPLSOCKET_INVALID), cmdSock_server(VPLSOCKET_INVALID)
{
    VPLSocket_t listenSock = VPLSOCKET_INVALID;
    VPLSocket_t clientSock = VPLSOCKET_INVALID;
    VPLSocket_t serverSock = VPLSOCKET_INVALID;

    // Note that we are inside a constructor.  Thus, we cannot return an error.
    // If we fail to construct a command-socket pair, we will indicate this internally with invalid socket values
    // and run in a degraded mode of not having a command-socket pair (and thus a timeout must be reached to unblock).

    {
        VPLNet_addr_t addr = VPLNET_ADDR_LOOPBACK;
        VPLNet_port_t port = VPLNET_PORT_ANY;
        listenSock = VPLSocket_CreateTcp(addr, port);
        if (VPLSocket_Equal(listenSock, VPLSOCKET_INVALID)) {
            LOG_ERROR("Failed to create listen socket");
            goto end;
        }
    }

    {
        int err = VPLSocket_Listen(listenSock, 1);
        if (err) {
            LOG_ERROR("VPLSocket_Listen() failed on socket["FMT_VPLSocket_t"]: err %d", VAL_VPLSocket_t(listenSock), err);
            goto end;
        }
    }

    clientSock = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, /*nonblocking*/VPL_TRUE);
    if (VPLSocket_Equal(clientSock, VPLSOCKET_INVALID)) {
        LOG_ERROR("Failed to create client-end socket");
        goto end;
    }

    {
        VPLSocket_addr_t sin;
        sin.family = VPL_PF_INET;
        sin.addr = VPLNET_ADDR_LOOPBACK;
        sin.port = VPLSocket_GetPort(listenSock);
        int err = VPLSocket_Connect(clientSock, &sin, sizeof(sin));
        if (err) {
            LOG_ERROR("Failed to connect client socket to server: err %d", err);
            goto end;
        }
    }

    {
        VPLSocket_addr_t addr;
        int err = VPLSocket_Accept(listenSock, &addr, sizeof(addr), &serverSock);
        if (err) {
            LOG_ERROR("VPLSocket_Accept() failed: err %d", err);
            goto end;
        }
    }

    // Transfer ownership of sockets to obj data members.
    cmdSock_client = clientSock;
    clientSock = VPLSOCKET_INVALID;
    cmdSock_server = serverSock;
    serverSock = VPLSOCKET_INVALID;

 end:
    if (!VPLSocket_Equal(listenSock, VPLSOCKET_INVALID)) {
        VPLSocket_Close(listenSock);
    }
    if (!VPLSocket_Equal(clientSock, VPLSOCKET_INVALID)) {
        VPLSocket_Close(clientSock);
    }
    if (!VPLSocket_Equal(serverSock, VPLSOCKET_INVALID)) {
        VPLSocket_Close(serverSock);
    }

    if (VPLSocket_Equal(cmdSock_client, VPLSOCKET_INVALID) ||
        VPLSocket_Equal(cmdSock_server, VPLSOCKET_INVALID)) {
        LOG_WARN("No command socket pair");
        // This obj is in degraded mode - it cannot be stopped via command socket.
    }
}

InterruptibleSocket_SigByCmdSocket::~InterruptibleSocket_SigByCmdSocket()
{
    if (!VPLSocket_Equal(cmdSock_client, VPLSOCKET_INVALID)) {
        VPLSocket_Close(cmdSock_client);
    }
    if (!VPLSocket_Equal(cmdSock_server, VPLSOCKET_INVALID)) {
        VPLSocket_Close(cmdSock_server);
    }
}

ssize_t InterruptibleSocket_SigByCmdSocket::Read(u8 *buf, size_t bufsize, VPLTime_t timeout)
{
    if (ioState != INTR_SOCKET_IO_OK) {
        return VPL_ERR_CANCELED;
    }

    {
        VPLSocket_poll_t pollspec[2];
        int numInSocks = 0;
        pollspec[numInSocks].socket = socket;
        pollspec[numInSocks].events = VPLSOCKET_POLL_RDNORM;
        numInSocks++;
        if (!VPLSocket_Equal(cmdSock_server, VPLSOCKET_INVALID)) {
            pollspec[numInSocks].socket = cmdSock_server;
            pollspec[numInSocks].events = VPLSOCKET_POLL_RDNORM;
            numInSocks++;
        }
        int numSockets = VPLSocket_Poll(pollspec, numInSocks, timeout);

        if (ioState != INTR_SOCKET_IO_OK) {
            return VPL_ERR_CANCELED;
        }

        if (numSockets < 0) {  // poll error
            LOG_ERROR("Poll() failed: err %d", numSockets);
            return numSockets;
        }
        if (numSockets == 0) {  // timeout
            // Note that it is not necessarily an error for Recv to timeout.
            LOG_INFO("Socket not ready for Recv after "FMT_VPLTime_t" secs", VPLTime_ToSec(timeout));
            return VPL_ERR_TIMEOUT;
        }
        if (pollspec[0].revents && 
            pollspec[0].revents != VPLSOCKET_POLL_RDNORM) {  // bad socket - handle as EOF
            LOG_WARN("Socket failed: revents %d", pollspec[0].revents);
            return 0;
        }

        assert(pollspec[0].revents == VPLSOCKET_POLL_RDNORM);
    }

    return recv(buf, bufsize);
}

ssize_t InterruptibleSocket_SigByCmdSocket::Write(const u8 *data, size_t datasize, VPLTime_t timeout)
{
    if (ioState != INTR_SOCKET_IO_OK) {
        return VPL_ERR_CANCELED;
    }

    {
        VPLSocket_poll_t pollspec[2];
        int numInSocks = 0;
        pollspec[numInSocks].socket = socket;
        pollspec[numInSocks].events = VPLSOCKET_POLL_OUT;
        numInSocks++;
        if (!VPLSocket_Equal(cmdSock_server, VPLSOCKET_INVALID)) {
            pollspec[numInSocks].socket = cmdSock_server;
            pollspec[numInSocks].events = VPLSOCKET_POLL_RDNORM;
            numInSocks++;
        }
        int numSockets = VPLSocket_Poll(pollspec, numInSocks, timeout);

        if (ioState != INTR_SOCKET_IO_OK) {
            return VPL_ERR_CANCELED;
        }

        if (numSockets < 0) {  // poll error
            LOG_ERROR("Poll() failed: err %d", numSockets);
            return numSockets;
        }
        if (numSockets == 0) {  // timeout
            LOG_ERROR("Socket not ready for send after "FMT_VPLTime_t" secs", VPLTime_ToSec(timeout));
            return VPL_ERR_TIMEOUT;
        }
        if (pollspec[0].revents &&
            pollspec[0].revents != VPLSOCKET_POLL_OUT) {  // bad socket - handle as error
            LOG_WARN("socket failed: revents %d", pollspec[0].revents);
            return VPL_ERR_IO;
        }

        assert(pollspec[0].revents == VPLSOCKET_POLL_OUT);
    }

    return send(data, datasize);
}

void InterruptibleSocket_SigByCmdSocket::StopIo()
{
    if (ioState == INTR_SOCKET_IO_OK) {
        shutdown();
        // Regardless of outcome of shutdown(), we will transition to IO_STOPPED state.
        ioState = INTR_SOCKET_IO_STOPPED;

        int bytesWritten = VPLSocket_Send(cmdSock_client, "Q", 1);
        if (bytesWritten < 0) {
            LOG_ERROR("Failed to write into pipe: err %d", (int)bytesWritten);
        }
        else if (bytesWritten != 1) {
            LOG_ERROR("Wrong number of bytes written into pipe: nbytes %d", (int)bytesWritten);
        }
    }
}
