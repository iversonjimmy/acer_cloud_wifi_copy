#include "HttpFileUploadStream.hpp"
#include "InStringStream.hpp"
#include "OutStringStream.hpp"

#include <gvm_errors.h>

#include <cassert>

HttpFileUploadStream::HttpFileUploadStream(InStringStream *iss_reqhdr,
                                           VPLFile_handle_t file_reqbody,
                                           OutStringStream *oss_resp)
    : ioStopped(false), iss_reqhdr(iss_reqhdr), file_reqbody(file_reqbody), oss_resp(oss_resp), bodyReadCb(NULL), bodyReadCbCtx(NULL)
{
}

HttpFileUploadStream::~HttpFileUploadStream()
{
}

void HttpFileUploadStream::StopIo()
{
    if (!ioStopped)
        ioStopped = true;
    if (iss_reqhdr && !iss_reqhdr->IsIoStopped())
        iss_reqhdr->StopIo();
    if (oss_resp && !oss_resp->IsIoStopped())
        oss_resp->StopIo();

    assert(ioStopped);
    assert(!iss_reqhdr || iss_reqhdr->IsIoStopped());
    assert(!oss_resp || oss_resp->IsIoStopped());
}

bool HttpFileUploadStream::IsIoStopped() const
{
    return ioStopped;
}

void HttpFileUploadStream::SetBodyReadCb(void (*cb)(void *ctx, size_t bytes, const char* buf), void *ctx)
{
    bodyReadCb = cb;
    bodyReadCbCtx = ctx;
}

ssize_t HttpFileUploadStream::read_header(char *buf, size_t bufsize)
{
    return iss_reqhdr->Read(buf, bufsize);
}

ssize_t HttpFileUploadStream::read_body(char *buf, size_t bufsize)
{
    if (ioStopped)
        return (ssize_t)VPL_ERR_CANCELED;
    ssize_t bytes = VPLFile_Read(file_reqbody, buf, bufsize);
    if (bytes > 0 && bodyReadCb) {
        (*bodyReadCb)(bodyReadCbCtx, bytes, buf);
    }
    return bytes;
}

ssize_t HttpFileUploadStream::write(const char *data, size_t datasize)
{
    return oss_resp->Write(data, datasize);
}

ssize_t HttpFileUploadStream::write_header(const char *data, size_t datasize)
{
    return write(data, datasize);
}

ssize_t HttpFileUploadStream::write_body(const char *data, size_t datasize)
{
    return write(data, datasize);
}
