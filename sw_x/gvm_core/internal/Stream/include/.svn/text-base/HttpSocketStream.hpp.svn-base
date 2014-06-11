#ifndef __HTTP_SOCKET_STREAM_HPP__
#define __HTTP_SOCKET_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_socket.h>

#include <sstream>
#include <string>

#include "HttpStream.hpp"

class SocketStream;

class HttpSocketStream : public HttpStream {
public:
    HttpSocketStream(SocketStream *ss);
    ~HttpSocketStream();

    // inherited interface
    void StopIo();
    bool IsIoStopped() const;

protected:
    ssize_t read_header(char *buf, size_t bufsize);
    ssize_t read_body(char *buf, size_t bufsize);
    ssize_t write_header(const char *data, size_t datasize);
    ssize_t write_body(const char *data, size_t datasize);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(HttpSocketStream);

    bool ioStopped;

    SocketStream *const ss;
    ssize_t read(char *buf, size_t bufsize);
    ssize_t write(const char *data, size_t datasize);
};

#endif // incl guard
