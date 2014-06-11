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
 * This implementation uses VPLSocket_Shutdown to signal exit.
 */

#include "InterruptibleSocket_SigBySockShutdown.hpp"

#include <log.h>

#include <vpl_error.h>

#include <cassert>

InterruptibleSocket_SigBySockShutdown::InterruptibleSocket_SigBySockShutdown(VPLSocket_t socket)
    : InterruptibleSocket(socket), ioState(INTR_SOCKET_IO_OK)
{
}

InterruptibleSocket_SigBySockShutdown::~InterruptibleSocket_SigBySockShutdown()
{
}

ssize_t InterruptibleSocket_SigBySockShutdown::Read(u8 *buf, size_t bufsize, VPLTime_t timeout)
{
    if (ioState != INTR_SOCKET_IO_OK) {
        return VPL_ERR_CANCELED;
    }

    {
        VPLSocket_poll_t pollspec[1];
        pollspec[0].socket = socket;
        pollspec[0].events = VPLSOCKET_POLL_RDNORM;
        int numSockets = VPLSocket_Poll(pollspec, ARRAY_ELEMENT_COUNT(pollspec), timeout);

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
        if (pollspec[0].revents != VPLSOCKET_POLL_RDNORM) {  // bad socket - handle as EOF
            LOG_WARN("Socket failed: revents %d", pollspec[0].revents);
            return 0;
        }

        assert(pollspec[0].revents == VPLSOCKET_POLL_RDNORM);
    }

    return recv(buf, bufsize);
}

ssize_t InterruptibleSocket_SigBySockShutdown::Write(const u8 *data, size_t datasize, VPLTime_t timeout)
{
    if (ioState != INTR_SOCKET_IO_OK) {
        return VPL_ERR_CANCELED;
    }

    {
        VPLSocket_poll_t pollspec[1];
        pollspec[0].socket = socket;
        pollspec[0].events = VPLSOCKET_POLL_OUT;
        int numSockets = VPLSocket_Poll(pollspec, ARRAY_ELEMENT_COUNT(pollspec), timeout);

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
        if (pollspec[0].revents != VPLSOCKET_POLL_OUT) {  // bad socket - handle as error
            LOG_WARN("socket failed: revents %d", pollspec[0].revents);
            return VPL_ERR_IO;
        }

        assert(pollspec[0].revents == VPLSOCKET_POLL_OUT);
    }

    return send(data, datasize);
}

void InterruptibleSocket_SigBySockShutdown::StopIo()
{
    if (ioState == INTR_SOCKET_IO_OK) {
        shutdown();
        // Regardless of outcome of shutdown(), we will transition to IO_STOPPED state.
        ioState = INTR_SOCKET_IO_STOPPED;
    }
}
