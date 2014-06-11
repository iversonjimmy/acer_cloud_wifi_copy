#ifndef __HTTP_TS_STREAM_HPP__
#define __HTTP_TS_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <sstream>
#include <string>

#if defined(STREAM_USE_TS_WRAPPER) && STREAM_USE_TS_WRAPPER
#define IS_TS_WRAPPER
#else
#ifndef IS_TS
#define IS_TS
#endif // IS_TS
#endif // defined(STREAM_USE_TS_WRAPPER)
#include <ts_server.hpp>

#include "HttpStream.hpp"

class HttpTsStream : public HttpStream {
public:
    HttpTsStream(TSIOHandle_t tsio);
    ~HttpTsStream();

    // inherited interface
    void StopIo();
    bool IsIoStopped() const;

protected:
    ssize_t read_header(char *buf, size_t bufsize);
    ssize_t read_body(char *buf, size_t bufsize);
    ssize_t write_header(const char *data, size_t datasize);
    ssize_t write_body(const char *data, size_t datasize);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(HttpTsStream);

    bool ioStopped;

    const TSIOHandle_t tsio;
    ssize_t read(char *buf, size_t bufsize);
    ssize_t write(const char *data, size_t datasize);
};

#endif // incl guard
