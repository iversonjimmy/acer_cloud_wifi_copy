#include "HttpStringStream.hpp"
#include "InStringStream.hpp"
#include "OutStringStream.hpp"

#include <gvm_errors.h>

HttpStringStream::HttpStringStream(InStringStream *iss, OutStringStream *oss)
    : ioStopped(false), iss(iss), oss(oss)
{
}

HttpStringStream::~HttpStringStream()
{
}

void HttpStringStream::StopIo()
{
    if (!ioStopped)
        ioStopped = true;
    if (iss && !iss->IsIoStopped())
        iss->StopIo();
    if (oss && !oss->IsIoStopped())
        oss->StopIo();
}

bool HttpStringStream::IsIoStopped() const
{
    return ioStopped;
}

ssize_t HttpStringStream::read(char *buf, size_t bufsize)
{
    if (!iss)
        return CCD_ERROR_NOT_INIT;
    return iss->Read(buf, bufsize);
}

ssize_t HttpStringStream::read_header(char *buf, size_t bufsize)
{
    return read(buf, bufsize);
}

ssize_t HttpStringStream::read_body(char *buf, size_t bufsize)
{
    return read(buf, bufsize);
}

ssize_t HttpStringStream::write(const char *data, size_t datasize)
{
    if (!oss)
        return CCD_ERROR_NOT_INIT;
    return oss->Write(data, datasize);
}

ssize_t HttpStringStream::write_header(const char *data, size_t datasize)
{
    return write(data, datasize);
}

ssize_t HttpStringStream::write_body(const char *data, size_t datasize)
{
    return write(data, datasize);
}
