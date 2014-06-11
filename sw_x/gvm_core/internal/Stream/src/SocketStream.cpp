#include "SocketStream.hpp"

#include <gvm_errors.h>
#include <log.h>

#include <vplu_format.h>

SocketStream::SocketStream(VPLSocket_t socket)
    : ioStopped(false), socket(socket)
{
}

SocketStream::~SocketStream()
{
    // DO NOT CLOSE THE SOCKET
}

ssize_t SocketStream::Read(char *buf, size_t bufsize)
{
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }

    int numBytes = VPLSocket_Recv(socket, buf, bufsize);
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }
    if (numBytes <= 0) {
        if (numBytes < 0) {
            // Error detected - proceed to stop this stream.
            LOG_ERROR("SocketStream[%p]: Failed to read from socket["FMT_VPLSocket_t"]: err %d",
                      this, VAL_VPLSocket_t(socket), numBytes);
        }
        else {
            // This usually means disconnection - proceed to stop this stream.
            LOG_INFO("SocketStream[%p]: No bytes received from socket["FMT_VPLSocket_t"]",
                     this, VAL_VPLSocket_t(socket));
        }
        ioStopped = true;
    }
    return (ssize_t)numBytes;
}

ssize_t SocketStream::Write(const char *data, size_t datasize)
{
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }

    int numBytes = VPLSocket_Send(socket, data, datasize);
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }
    if (numBytes <= 0) {
        // Error detected - proceed to stop this stream.
        LOG_ERROR("SocketStream[%p]: Failed to send to socket["FMT_VPLSocket_t"]: err %d",
                  this, VAL_VPLSocket_t(socket), numBytes);
        ioStopped = true;
    }
    else if (static_cast<size_t>(numBytes) < datasize) {  // this cast is safe because the case of 0-or-less is already handled
        LOG_WARN("SocketStream[%p]: Partial write: %d/"FMT_size_t,
                 this, numBytes, datasize);
    }
    return (ssize_t)numBytes;
}

void SocketStream::StopIo()
{
    if (!ioStopped) {
        ioStopped = true;
    }
}

bool SocketStream::IsIoStopped() const
{
    return ioStopped;
}
