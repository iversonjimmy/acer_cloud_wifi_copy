#include "HttpTsStream.hpp"

#include <gvm_errors.h>
#include <log.h>

#include <cassert>

#if defined(STREAM_USE_TS_WRAPPER) && STREAM_USE_TS_WRAPPER
using namespace TS_WRAPPER;
#else
using namespace TS;
#endif // defined(STREAM_USE_TS_WRAPPER)

HttpTsStream::HttpTsStream(TSIOHandle_t tsio)
    : ioStopped(false), tsio(tsio)
{
}

HttpTsStream::~HttpTsStream()
{
}

void HttpTsStream::StopIo()
{
    if (!ioStopped)
        ioStopped = true;

    assert(ioStopped);
}

bool HttpTsStream::IsIoStopped() const
{
    return ioStopped;
}

ssize_t HttpTsStream::read(char *buf, size_t bufsize)
{
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }

    TSError_t tserr = TS_OK;
    std::string errmsg;
    tserr = TSS_Read(tsio, buf, bufsize, errmsg);
    // NOTE: "bufsize" modified by TSS_Read to reflect size of data obtained.
    if (tserr != TS_OK) {
        LOG_ERROR("HttpTsStream[%p]: TSS_Read failed: err %d, msg %s", this, tserr, errmsg.c_str());
    }
    return tserr != TS_OK ? tserr : bufsize;
}

ssize_t HttpTsStream::read_header(char *buf, size_t bufsize)
{
    return read(buf, bufsize);
}

ssize_t HttpTsStream::read_body(char *buf, size_t bufsize)
{
    return read(buf, bufsize);
}

ssize_t HttpTsStream::write(const char *data, size_t datasize)
{
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }

    TSError_t tserr = TS_OK;
    std::string errmsg;
    tserr = TSS_Write(tsio, data, datasize, errmsg);
    if (tserr != TS_OK) {
        LOG_ERROR("HttpTsStream[%p]: TSS_Write failed: err %d, msg %s", this, tserr, errmsg.c_str());
    }
    return tserr != TS_OK ? tserr : datasize;
}

ssize_t HttpTsStream::write_header(const char *data, size_t datasize)
{
    return write(data, datasize);
}

ssize_t HttpTsStream::write_body(const char *data, size_t datasize)
{
    return write(data, datasize);
}
