#include "HttpSocketStream.hpp"
#include "SocketStream.hpp"

#include <gvm_errors.h>

#include <cassert>

HttpSocketStream::HttpSocketStream(SocketStream *ss)
    : ioStopped(false), ss(ss)
{
}

HttpSocketStream::~HttpSocketStream()
{
}

void HttpSocketStream::StopIo()
{
    if (!ioStopped)
        ioStopped = true;
    if (ss && !ss->IsIoStopped()) {
        ss->StopIo();
    }

    assert(ioStopped);
    assert(!ss || ss->IsIoStopped());
}

bool HttpSocketStream::IsIoStopped() const
{
    return ioStopped;
}

ssize_t HttpSocketStream::read(char *buf, size_t bufsize)
{
    if (!ss)
        return (ssize_t)CCD_ERROR_NOT_INIT;
    return ss->Read(buf, bufsize);
}

ssize_t HttpSocketStream::read_header(char *buf, size_t bufsize)
{
    return read(buf, bufsize);
}

ssize_t HttpSocketStream::read_body(char *buf, size_t bufsize)
{
    return read(buf, bufsize);
}

ssize_t HttpSocketStream::write(const char *data, size_t datasize)
{
    if (!ss)
        return (ssize_t)CCD_ERROR_NOT_INIT;
    return ss->Write(data, datasize);
}

ssize_t HttpSocketStream::write_header(const char *data, size_t datasize)
{
    return write(data, datasize);
}

ssize_t HttpSocketStream::write_body(const char *data, size_t datasize)
{
    return write(data, datasize);
}
