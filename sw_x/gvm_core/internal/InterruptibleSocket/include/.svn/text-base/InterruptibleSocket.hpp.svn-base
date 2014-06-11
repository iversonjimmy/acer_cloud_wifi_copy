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
  A wrapper around a VPL Socket
  that would make it possible to wake up threads blocked on the socket.
  Read() and Write() are available for I/O.
  StopIo() will mark the socket for no further I/O,
  and will cause any threads blocked calling Read() and Write() to return.

  Multiple implementation are planned, accomodating different socket behaviors.
 */

#ifndef __INTERRUPTIBLE_SOCKET_HPP__
#define __INTERRUPTIBLE_SOCKET_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_time.h>

class InterruptibleSocket {
public:
    // Assumption is that a socket is connected.
    InterruptibleSocket(VPLSocket_t socket);

    // Destroying the obj has the side effect of closing the socket.
    virtual ~InterruptibleSocket();

    // Read at most "bufsize" bytes into "buf", waiting upto time specified in "timeout".
    // It is a user error to have multiple threads call Read() at the same time.
    // An implementation may or may not actively detect such a condition.
    virtual ssize_t Read(u8 *buf, size_t bufsize, VPLTime_t timeout) = 0;

    // Write at most "datasize" bytes from "data" into the socket, waiting upto time specified in "timeout".
    // It is a user error to have multiple threads call Write() at the same time.
    // An implementation may or may not actively detect such a condition.
    virtual ssize_t Write(const u8 *data, size_t datasize, VPLTime_t timeout) = 0;

    // Note that one thread calling Read() and another thread calling Write() at the same time will be supported.

    // Request the obj to stop I/O at its earliest possible time.
    // Actual work is done asynchronously.
    // Return from this function simply means the request has been received.
    // It does not mean that all I/O has ceased.
    virtual void StopIo() = 0;

    enum IoState {
        INTR_SOCKET_IO_OK = 0,
        INTR_SOCKET_IO_STOPPING,
        INTR_SOCKET_IO_STOPPED,
    };

protected:
    VPL_DISABLE_COPY_AND_ASSIGN(InterruptibleSocket);

    const VPLSocket_t socket;

    ssize_t recv(u8 *buf, size_t bufsize);
    ssize_t send(const u8 *data, size_t datasize);
    int shutdown();
};

#endif // incl guard
