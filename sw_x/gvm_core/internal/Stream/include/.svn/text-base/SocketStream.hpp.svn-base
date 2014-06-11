#ifndef __SOCKET_STREAM_HPP__
#define __SOCKET_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_socket.h>

#include "Stream.hpp"

class SocketStream : public Stream {
public:
    SocketStream(VPLSocket_t socket);
    ~SocketStream();

    // inherited interface
    ssize_t Read(char *buf, size_t bufsize);
    ssize_t Write(const char *data, size_t datasize);
    void StopIo();
    bool IsIoStopped() const;

protected:
    VPL_DISABLE_COPY_AND_ASSIGN(SocketStream);

    bool ioStopped;
    VPLSocket_t socket;
};

#endif // incl guard

