#ifndef __HTTP_FILE_UPLOAD_STREAM_HPP__
#define __HTTP_FILE_UPLOAD_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_file.h>

#include "HttpStream.hpp"

class InStringStream;
class OutStringStream;

class HttpFileUploadStream : public HttpStream {
public:
    HttpFileUploadStream(InStringStream *iss_reqhdr, VPLFile_handle_t file_reqbody, OutStringStream *oss_resp);
    ~HttpFileUploadStream();

    // inherited interface
    void StopIo();
    bool IsIoStopped() const;

    // Set a callback function that will be called every time the file is read.
    void SetBodyReadCb(void (*cb)(void *ctx, size_t bytes, const char* buf), void *ctx);

protected:
    ssize_t read_header(char *buf, size_t bufsize);
    ssize_t read_body(char *buf, size_t bufsize);
    ssize_t write_header(const char *data, size_t datasize);
    ssize_t write_body(const char *data, size_t datasize);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(HttpFileUploadStream);

    bool ioStopped;

    InStringStream *iss_reqhdr;
    VPLFile_handle_t file_reqbody;
    OutStringStream *oss_resp;

    ssize_t write(const char *data, size_t datasize);

    void (*bodyReadCb)(void *ctx, size_t bytes, const char* buf);
    void *bodyReadCbCtx;
};

#endif // incl guard
