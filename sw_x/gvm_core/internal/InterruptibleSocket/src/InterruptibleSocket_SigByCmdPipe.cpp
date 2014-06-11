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
 * This implementation uses a command pipe to signal exit.
 */

#include "InterruptibleSocket_SigByCmdPipe.hpp"

#include <log.h>

#include <vpl_error.h>

#include <poll.h>
#include <cassert>

InterruptibleSocket_SigByCmdPipe::InterruptibleSocket_SigByCmdPipe(VPLSocket_t socket)
    : InterruptibleSocket(socket), ioState(INTR_SOCKET_IO_OK)
{
    int err = VPLPipe_Create(pipe);
    if (err) {
        LOG_ERROR("Failed to create command pipe: err %d", err);
        pipe[0] = pipe[1] = VPLFILE_INVALID_HANDLE;
    }
}

InterruptibleSocket_SigByCmdPipe::~InterruptibleSocket_SigByCmdPipe()
{
    for (int i = 0; i < 2; i++) {
        if (VPLFile_IsValidHandle(pipe[i])) {
            VPLFile_Close(pipe[i]);
            pipe[i] = VPLFILE_INVALID_HANDLE;
        }
    }
}

ssize_t InterruptibleSocket_SigByCmdPipe::Read(u8 *buf, size_t bufsize, VPLTime_t timeout)
{
    if (ioState != INTR_SOCKET_IO_OK) {
        return VPL_ERR_CANCELED;
    }

    {
        // VPL Poll() cannot fix file descriptors of different types, so go directly to POSIX layer.
        struct pollfd pollspec[2];
        pollspec[0].fd = VAL_VPLSocket_t(socket);
        pollspec[0].events = POLLIN;
        pollspec[1].fd = pipe[0];
        pollspec[1].events = POLLIN;
        int numSockets = poll(pollspec, ARRAY_ELEMENT_COUNT(pollspec), timeout);

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

        assert(pollspec[0].revents == POLLIN);
    }

    return recv(buf, bufsize);
}

ssize_t InterruptibleSocket_SigByCmdPipe::Write(const u8 *data, size_t datasize, VPLTime_t timeout)
{
    if (ioState != INTR_SOCKET_IO_OK) {
        return VPL_ERR_CANCELED;
    }

    {
        struct pollfd pollspec[2];
        pollspec[0].fd = VAL_VPLSocket_t(socket);
        pollspec[0].events = POLLOUT;
        pollspec[1].fd = pipe[0];
        pollspec[1].events = POLLIN;
        int numSockets = poll(pollspec, ARRAY_ELEMENT_COUNT(pollspec), timeout);

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

        assert(pollspec[0].revents == POLLOUT);
    }

    return send(data, datasize);
}

void InterruptibleSocket_SigByCmdPipe::StopIo()
{
    if (ioState == INTR_SOCKET_IO_OK) {
        shutdown();
        // Regardless of outcome of shutdown(), we will transition to IO_STOPPED state.
        ioState = INTR_SOCKET_IO_STOPPED;

        ssize_t bytesWritten = VPLFile_Write(pipe[1], "Q", 1);
        if (bytesWritten < 0) {
            LOG_ERROR("Failed to write into pipe: err %d", (int)bytesWritten);
        }
        else if (bytesWritten != 1) {
            LOG_ERROR("Wrong number of bytes written into pipe: nbytes %d", (int)bytesWritten);
        }
    }
}
