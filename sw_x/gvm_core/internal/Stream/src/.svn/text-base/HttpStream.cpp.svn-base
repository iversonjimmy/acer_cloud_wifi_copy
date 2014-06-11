#include "HttpStream.hpp"

#include <gvm_errors.h>

#include <vpl_conv.h>
#include <vplu_common.h>
#include <log.h>

#include <cassert>
#include <sstream>

HttpStream::HttpStream()
    : reqHdrAllRead(false), reqHdrReadBytes(0), reqBodyReadBytes(0), respHdrPushed(false), respBodyBytesWritten(0),
      userId(0), deviceId(0)
{
}

HttpStream::~HttpStream()
{
    LOG_INFO("HttpStream[%p]: in: "FMTu64" out: "FMTu64" status: %d",
             this,
             reqHdrReadBytes + reqBodyReadBytes,
             respHdrPushed + respBodyBytesWritten,
             respHdr.GetStatusCode());
}

ssize_t HttpStream::Read(char *buf, size_t bufsize)
{
    ssize_t bytes = 0;

    if (!reqHdr.IsComplete()) {
        loadReqHeader();
    }

    if (!reqHdrAllRead) {
        std::string reqheader;
        reqHdr.Serialize(reqheader);
        if (reqHdrReadBytes < reqheader.size()) {  // have bytes to send
            size_t avail = reqheader.size() - reqHdrReadBytes;
            size_t copybytes = avail;
            if (copybytes > bufsize)
                copybytes = bufsize;
            memcpy(buf, reqheader.data() + reqHdrReadBytes, copybytes);
            bytes = copybytes;
            reqHdrReadBytes += copybytes;
        }
        else {
            reqHdrAllRead = true;
        }
    }

    assert(bytes >= 0);
    if (static_cast<size_t>(bytes) < bufsize) {  // there's space left in "buf"
        std::string contentLength;
        int _err = reqHdr.GetHeader("Content-Length", contentLength);
        if (_err == CCD_ERROR_NOT_FOUND) {
            // assume no body
            goto end;
        }

        u64 bodySizeRemaining = VPLConv_strToU64(contentLength.c_str(), NULL, 10) - reqBodyReadBytes;
        if (bodySizeRemaining == 0) {
            goto end;
        }
        size_t copysize = (size_t)MIN((u64)bufsize - (u64)bytes, bodySizeRemaining);  // compare in u64 and downcast result to size_t; this is safe because first arg value is no larger than size_t
        ssize_t _bytes = read_body(buf + bytes, copysize);
        if (_bytes < 0) {  // error
            bytes = _bytes;
            goto end;
        }
        bytes += _bytes;
        reqBodyReadBytes += _bytes;
    }

 end:
    return bytes;
}

ssize_t HttpStream::Write(const char *data, size_t datasize)
{
    ssize_t bytes = 0;

    if (!respHdr.IsComplete()) {
        ssize_t used = respHdr.FeedData(data, datasize);
        if (used < 0) {
            bytes = used;
            goto end;
        }
        bytes += used;
        data += used;
        datasize -= used;
    }

    if (respHdr.IsComplete() && datasize > 0) {
        if (!respHdrPushed) {
            std::string respheader;
            respHdr.Serialize(respheader);
            ssize_t used = write_header(respheader.data(), respheader.size());
            if (used < 0) {  // error
                // msg logged by write_header()
                bytes = used;
                goto end;
            }
            respHdrPushed = true;
        }

        ssize_t used = write_body(data, datasize);
        if (used < 0) {  // error
            // msg logged by write_body()
            bytes = used;
            goto end;
        }
        bytes += used;
        respBodyBytesWritten += used;
    }

 end:
    return bytes;
}

int HttpStream::Flush()
{
    int err = 0;

    if (!respHdrPushed) {
        std::string respheader;
        respHdr.Serialize(respheader);
        ssize_t used = write_header(respheader.data(), respheader.size());
        if (used < 0) {  // error
            // msg logged by write_header()
            err = (int)used;
            goto end;
        }
        respHdrPushed = true;
    }

 end:
    return err;
}

const std::string &HttpStream::GetMethod()
{
    static std::string empty;

    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return empty;
    }
    return reqHdr.GetMethod();
}

int HttpStream::GetMethod(std::string &method)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.GetMethod(method);
}

int HttpStream::SetMethod(const std::string &method)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.SetMethod(method);
}

const std::string &HttpStream::GetUri()
{
    static std::string empty;

    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return empty;
    }
    return reqHdr.GetUri();
}

int HttpStream::GetUri(std::string &uri)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.GetUri(uri);
}

int HttpStream::SetUri(const std::string &uri)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.SetUri(uri);
}

int HttpStream::GetVersion(std::string &version)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.GetVersion(version);
}

int HttpStream::SetVersion(const std::string &version)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.SetVersion(version);
}

int HttpStream::GetQuery(const std::string &name, std::string &value)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.GetQuery(name, value);
}

int HttpStream::SetQuery(const std::string &name, const std::string &value)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.SetQuery(name, value);
}

int HttpStream::RemoveQuery(const std::string &name)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.RemoveQuery(name);
}

int HttpStream::GetReqHeader(const std::string &name, std::string &value)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.GetHeader(name, value);
}

int HttpStream::SetReqHeader(const std::string &name, const std::string &value)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.SetHeader(name, value);
}

int HttpStream::RemoveReqHeader(const std::string &name)
{
    if (!reqHdr.IsComplete())
        loadReqHeader();
    if (!reqHdr.IsComplete()) {
        return CCD_ERROR_PARSE_CONTENT;
    }
    return reqHdr.RemoveHeader(name);
}

int HttpStream::SetStatusCode(int code)
{
    return respHdr.SetStatusCode(code);
}

int HttpStream::GetStatusCode()
{
    return respHdr.GetStatusCode();
}

int HttpStream::GetStatusCode(int &code)
{
    return respHdr.GetStatusCode(code);
}

int HttpStream::GetRespHeader(const std::string &name, std::string &value)
{
    return respHdr.GetHeader(name, value);
}

int HttpStream::SetRespHeader(const std::string &name, const std::string &value)
{
    return respHdr.SetHeader(name, value);
}

int HttpStream::RemoveRespHeader(const std::string &name)
{
    return respHdr.RemoveHeader(name);
}

u64 HttpStream::GetRespBodyBytesWritten() const
{
    return respBodyBytesWritten;
}

int HttpStream::loadReqHeader()
{
    int err = 0;

    char buf[1];
    ssize_t bytes = 0;
    ssize_t used = 0;
    do {
        bytes = read_header(buf, sizeof(buf));
        if (bytes < 0) {
            err = bytes;
            goto end;
        }
        used = reqHdr.FeedData(buf, bytes);
        if (used < 0) {
            err = used;
            goto end;
        }
    } while (!reqHdr.IsComplete());

 end:
    return err;
}

void HttpStream::SetUserId(u64 userId)
{
    this->userId = userId;
}

u64 HttpStream::GetUserId() const
{
    return userId;
}

void HttpStream::SetDeviceId(u64 deviceId)
{
    this->deviceId = deviceId;
}

u64 HttpStream::GetDeviceId() const
{
    return deviceId;
}
