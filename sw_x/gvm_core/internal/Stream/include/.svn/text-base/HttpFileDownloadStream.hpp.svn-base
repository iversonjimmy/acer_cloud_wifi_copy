//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __HTTP_FILE_DOWNLOAD_STREAM_HPP__
#define __HTTP_FILE_DOWNLOAD_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_file.h>

#include "HttpStream.hpp"

class InStream;
class OutStream;

class HttpFileDownloadStream : public HttpStream {
public:
    HttpFileDownloadStream(
            InStream *issReqHdr,
            InStream *issReqBody,
            VPLFile_handle_t fileRespBody,
            OutStream *ossResp);
    ~HttpFileDownloadStream();

    // inherited interface
    void StopIo();
    bool IsIoStopped() const;

    // Set a callback function that will be called every time the file is written to.
    void SetBodyWriteCb(void (*cb)(void *ctx, size_t bytes), void *ctx);

protected:
    ssize_t read_header(char *buf, size_t bufsize);
    ssize_t read_body(char *buf, size_t bufsize);
    ssize_t write_header(const char *data, size_t datasize);
    ssize_t write_body(const char *data, size_t datasize);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(HttpFileDownloadStream);

    bool ioStopped;

    InStream *issReqHdr;
    InStream *issReqBody;
    VPLFile_handle_t fileRespBody;
    OutStream *ossResp;

    void (*bodyWriteCb)(void *ctx, size_t bytes);
    void *bodyWriteCbCtx;
};

#endif // incl guard
