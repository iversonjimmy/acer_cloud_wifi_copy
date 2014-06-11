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

#ifndef __INTERRUPTIBLE_SOCKET_SIG_BY_CMD_PIPE_HPP__
#define __INTERRUPTIBLE_SOCKET_SIG_BY_CMD_PIPE_HPP__

#include "InterruptibleSocket.hpp"

#include <vplex_file.h>

class InterruptibleSocket_SigByCmdPipe : public InterruptibleSocket {
public:
    InterruptibleSocket_SigByCmdPipe(VPLSocket_t socket);
    ~InterruptibleSocket_SigByCmdPipe();

    ssize_t Read(u8 *buf, size_t bufsize, VPLTime_t timeout);
    ssize_t Write(const u8 *data, size_t datasize, VPLTime_t timeout);

    void StopIo();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(InterruptibleSocket_SigByCmdPipe);

    IoState ioState;
    VPLFile_handle_t pipe[2];
};

#endif // incl guard
