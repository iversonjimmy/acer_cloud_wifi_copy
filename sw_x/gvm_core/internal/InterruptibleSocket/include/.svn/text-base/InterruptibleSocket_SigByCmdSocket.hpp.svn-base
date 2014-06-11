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

#ifndef __INTERRUPTIBLE_SOCKET_SIG_BY_CMD_SOCKET_HPP__
#define __INTERRUPTIBLE_SOCKET_SIG_BY_CMD_SOCKET_HPP__

#include "InterruptibleSocket.hpp"

#include <vplex_file.h>

class InterruptibleSocket_SigByCmdSocket : public InterruptibleSocket {
public:
    InterruptibleSocket_SigByCmdSocket(VPLSocket_t socket);
    ~InterruptibleSocket_SigByCmdSocket();

    ssize_t Read(u8 *buf, size_t bufsize, VPLTime_t timeout);
    ssize_t Write(const u8 *data, size_t datasize, VPLTime_t timeout);

    void StopIo();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(InterruptibleSocket_SigByCmdSocket);

    IoState ioState;
    VPLSocket_t cmdSock_client, cmdSock_server;
};

#endif // incl guard
