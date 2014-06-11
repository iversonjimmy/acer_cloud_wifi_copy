#include "HttpFileDownloadStream.hpp"
#include "InStream.hpp"
#include "OutStream.hpp"

#include <gvm_errors.h>

#include <cassert>

HttpFileDownloadStream::HttpFileDownloadStream(
        InStream *issReqHdr,
        InStream *issReqBody,
        VPLFile_handle_t fileRespBody,
        OutStream *ossResp)
    : ioStopped(false),
      issReqHdr(issReqHdr),
      issReqBody(issReqBody),
      fileRespBody(fileRespBody),
      ossResp(ossResp),
      bodyWriteCb(NULL),
      bodyWriteCbCtx(NULL)
{
}

HttpFileDownloadStream::~HttpFileDownloadStream()
{
}

void HttpFileDownloadStream::StopIo()
{
    ioStopped = true;
    if (issReqHdr && !issReqHdr->IsIoStopped())
        issReqHdr->StopIo();
    if (issReqBody && !issReqBody->IsIoStopped())
        issReqBody->StopIo();
    if (ossResp && !ossResp->IsIoStopped())
        ossResp->StopIo();

    assert(ioStopped);
    assert(!issReqHdr || issReqHdr->IsIoStopped());
    assert(!issReqBody || issReqBody->IsIoStopped());
    assert(!ossResp || ossResp->IsIoStopped());
}

bool HttpFileDownloadStream::IsIoStopped() const
{
    return ioStopped;
}

void HttpFileDownloadStream::SetBodyWriteCb(void (*cb)(void *ctx, size_t bytes), void *ctx)
{
    bodyWriteCb = cb;
    bodyWriteCbCtx = ctx;
}

ssize_t HttpFileDownloadStream::read_header(char *buf, size_t bufsize)
{
    return issReqHdr->Read(buf, bufsize);
}

ssize_t HttpFileDownloadStream::read_body(char *buf, size_t bufsize)
{
    return issReqBody->Read(buf, bufsize);
}

ssize_t HttpFileDownloadStream::write_header(const char *data, size_t datasize)
{
    return ossResp->Write(data, datasize);
}

ssize_t HttpFileDownloadStream::write_body(const char *data, size_t datasize)
{
    if (ioStopped) {
        return (ssize_t)VPL_ERR_CANCELED;
    }
    ssize_t bytes = VPLFile_Write(fileRespBody, data, datasize);
    if (bytes > 0 && bodyWriteCb) {
        (*bodyWriteCb)(bodyWriteCbCtx, static_cast<size_t>(bytes));
    }
    return bytes;
}
