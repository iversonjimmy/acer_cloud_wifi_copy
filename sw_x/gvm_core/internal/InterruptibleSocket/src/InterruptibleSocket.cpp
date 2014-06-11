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

#include "InterruptibleSocket.hpp"

#include <log.h>

InterruptibleSocket::InterruptibleSocket(VPLSocket_t socket)
    : socket(socket)
{
}

InterruptibleSocket::~InterruptibleSocket()
{
    VPLSocket_Close(socket);
}

ssize_t InterruptibleSocket::recv(u8 *buf, size_t bufsize)
{
    int numBytesRcvd = VPLSocket_Recv(socket, buf, bufsize);
    if (numBytesRcvd < 0) {  // error
        LOG_ERROR("Failed to receive from socket["FMT_VPLSocket_t"]: err %d", VAL_VPLSocket_t(socket), numBytesRcvd);
    }
    else if (numBytesRcvd == 0) {  // socket disconnected
        LOG_INFO("No bytes received from socket["FMT_VPLSocket_t"]", VAL_VPLSocket_t(socket));
    }

    return (ssize_t)numBytesRcvd;
}

ssize_t InterruptibleSocket::send(const u8 *data, size_t datasize)
{
    int numBytesSent = VPLSocket_Send(socket, data, datasize);
    if (numBytesSent < 0) {  // error
        LOG_ERROR("Failed to send to socket["FMT_VPLSocket_t"]: err %d", VAL_VPLSocket_t(socket), numBytesSent);
    }

    return (ssize_t)numBytesSent;
}

int InterruptibleSocket::shutdown()
{
    int err = VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_RDWR);
    if (err) {
        LOG_ERROR("Failed to shutdown socket["FMT_VPLSocket_t"]: err %d", VAL_VPLSocket_t(socket), err);
    }
    return err;
}
