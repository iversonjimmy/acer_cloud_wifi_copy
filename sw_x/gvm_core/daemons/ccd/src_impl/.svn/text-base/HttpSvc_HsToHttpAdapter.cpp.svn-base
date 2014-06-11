#include "HttpSvc_HsToHttpAdapter.hpp"

#include "HttpSvc_Utils.hpp"

#include "config.h"

#include <log.h>
#include <HttpStream.hpp>

#include <vpl_conv.h>
#include <vplex_http2.hpp>
#include <vplu_format.h>

#include <sstream>

HttpSvc::HsToHttpAdapter::HsToHttpAdapter(HttpStream *hs)
    : hs(hs), runCalled(false), bailout(false), send_resp_in_chunks(false)
{
    LOG_INFO("HsToHttpAdapter[%p]: Created for HttpStream %p", this, hs);
}

HttpSvc::HsToHttpAdapter::~HsToHttpAdapter()
{
    LOG_INFO("HsToHttpAdapter[%p]: Destroyed", this);
}

int HttpSvc::HsToHttpAdapter::Run()
{
    if (runCalled) {
        LOG_ERROR("HsToHttpAdapter[%p]: Run called more than once", this);
        return -1;  // FIXME
    }
    runCalled = true;

    LOG_INFO("HsToHttpAdapter[%p]: Run", this);

    int err = 0;

    err = http.SetUri(hs->GetUri());
    if (err) {
        LOG_ERROR("HsToHttpAdapter[%p]: Bad URI: uri %s", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    err = copyReqHeader();
    if (err) {
        LOG_ERROR("HsToHttpAdapter[%p]: Failed to copy req header from Hs To HsToHttpAdapter: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    if (__ccdConfig.debugVcsHttp)
        http.SetDebug(1);

    if (methodJumpTable.handlers.find(hs->GetMethod()) == methodJumpTable.handlers.end()) {
        LOG_ERROR("HsToHttpAdapter[%p]: No handler: method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return (this->*methodJumpTable.handlers[hs->GetMethod()])();
}

static s32 recvCb(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    HttpSvc::HsToHttpAdapter *adapter = (HttpSvc::HsToHttpAdapter*)ctx;
    return adapter->recvCb(http, buf, size);
}

s32 HttpSvc::HsToHttpAdapter::recvCb(VPLHttp2 *http, const char *buf, u32 size)
{
    if (recvCb_firsttime) {
        copyRespHeader();
        recvCb_firsttime = false;
    }

    if (send_resp_in_chunks) {
        // send "chunk-size CRLF"; cf. http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.6.1
        std::ostringstream oss;
        oss << std::hex << size << "\r\n";
        hs->Write(oss.str().data(), oss.str().size());
    }
    s32 nwritten = (s32)hs->Write(buf, size);
    if (send_resp_in_chunks) {
        // send "chunk-data CRLF"; cf. http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.6.1
        hs->Write("\r\n", 2);
    }
    return nwritten;
}

static s32 sendCb(VPLHttp2 *http, void *ctx, char *buf, u32 size)
{
    HttpSvc::HsToHttpAdapter *adapter = (HttpSvc::HsToHttpAdapter*)ctx;
    return adapter->sendCb(http, buf, size);
}

s32 HttpSvc::HsToHttpAdapter::sendCb(VPLHttp2 *http, char *buf, u32 size)
{
    return (s32)hs->Read(buf, size);
}

int HttpSvc::HsToHttpAdapter::run_delete()
{
    int err = 0;

    std::string respBody;
    err = http.Delete(respBody);
    if (!err) {
        hs->SetStatusCode(http.GetStatusCode());
        {
            std::ostringstream oss;
            oss << respBody.size();
            hs->SetRespHeader("Content-Length", oss.str());
        }
        if (respBody.size() > 0) {
            hs->Write(respBody.data(), respBody.size());
        }
        else {
            hs->Flush();
        }
    }
    return err;
}

int HttpSvc::HsToHttpAdapter::run_get()
{
    int err = 0;
    recvCb_firsttime = true;
    err = http.Get(::recvCb, this, /*recvProgCb*/NULL, /*recvProgCtx*/NULL);
    LOG_INFO("err %d", err);
    if (send_resp_in_chunks) {
        // send "last-chunk CRLF"; cf. http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.6.1
        hs->Write("0\r\n\r\n", 5);
    }
    hs->Flush();
    return err;
}

int HttpSvc::HsToHttpAdapter::run_post()
{
    int err = 0;

    u64 contentLength = 0;
    {
        std::string contentLengthStr;
        err = hs->GetReqHeader("Content-Length", contentLengthStr);
        if (err) {
            LOG_ERROR("HsToHttpAdapter[%p]: Content-Length missing: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        contentLength = VPLConv_strToU64(contentLengthStr.c_str(), NULL, 10);
    }

    err = removeReqHeaderFromStream();
    if (err < 0) {
        // errmsg logged and response set by removeReqHeaderFromStream()
        return 0;
    }

    std::string respBody;
    err = http.Post(::sendCb, this, contentLength, /*sendProgCb*/NULL, /*sendProgCtx*/NULL, respBody);
    if (!err) {
        hs->SetStatusCode(http.GetStatusCode());
        {
            std::ostringstream oss;
            oss << respBody.size();
            hs->SetRespHeader("Content-Length", oss.str());
        }
        if (respBody.size() > 0) {
            hs->Write(respBody.data(), respBody.size());
        }
        else {
            hs->Flush();
        }
    }
    return err;
}

int HttpSvc::HsToHttpAdapter::run_put()
{
    int err = 0;

    u64 contentLength = 0;
    {
        std::string contentLengthStr;
        err = hs->GetReqHeader("Content-Length", contentLengthStr);
        if (err) {
            LOG_ERROR("HsToHttpAdapter[%p]: Content-Length missing: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        contentLength = VPLConv_strToU64(contentLengthStr.c_str(), NULL, 10);
    }

    err = removeReqHeaderFromStream();
    if (err < 0) {
        // errmsg logged and response set by removeReqHeaderFromStream()
        return 0;
    }

    std::string respBody;
    err = http.Put(::sendCb, this, contentLength, /*sendProgCb*/NULL, /*sendProgCtx*/NULL, respBody);
    if (!err) {
        hs->SetStatusCode(http.GetStatusCode());
        {
            std::ostringstream oss;
            oss << respBody.size();
            hs->SetRespHeader("Content-Length", oss.str());
        }
        if (respBody.size() > 0) {
            hs->Write(respBody.data(), respBody.size());
        }
        else {
            hs->Flush();
        }
    }
    return err;
}

HttpSvc::HsToHttpAdapter::MethodJumpTable HttpSvc::HsToHttpAdapter::methodJumpTable;

HttpSvc::HsToHttpAdapter::MethodJumpTable::MethodJumpTable()
{
    handlers["DELETE"] = &HsToHttpAdapter::run_delete;
    handlers["GET"]    = &HsToHttpAdapter::run_get;
    handlers["POST"]   = &HsToHttpAdapter::run_post;
    handlers["PUT"]    = &HsToHttpAdapter::run_put;
}

int HttpSvc::HsToHttpAdapter::copyReqHeader()
{
    // Currently, HttpStream has no method to enumerate or iterate over request headers.
    // Thus, we will blindly try a few known header names.
    tryCopyReqHeader(Utils::HttpHeader_ac_userId);
    tryCopyReqHeader(Utils::HttpHeader_ac_sessionHandle);
    tryCopyReqHeader(Utils::HttpHeader_ac_serviceTicket);
    tryCopyReqHeader(Utils::HttpHeader_ac_deviceId);
    tryCopyReqHeader(Utils::HttpHeader_ac_version);
    tryCopyReqHeader(Utils::HttpHeader_Host);
    tryCopyReqHeader(Utils::HttpHeader_Date);
    tryCopyReqHeader(Utils::HttpHeader_ContentLength);
    tryCopyReqHeader(Utils::HttpHeader_ContentType);
    tryCopyReqHeader(Utils::HttpHeader_Range);

    return 0;
}

int HttpSvc::HsToHttpAdapter::copyRespHeader()
{
    LOG_INFO("status %d", http.GetStatusCode());
    hs->SetStatusCode(http.GetStatusCode());

    // Currently, VPLHttp2 has no method to enumerate or iterate over response headers.
    // Thus, we will blindly try a few known header names.
    tryCopyRespHeader("Date");
    tryCopyRespHeader("Server");
    tryCopyRespHeader("Content-Length");
    tryCopyRespHeader("Content-Type");

    return 0;
}

int HttpSvc::HsToHttpAdapter::tryCopyReqHeader(const std::string &name)
{
    std::string value;
    int err = hs->GetReqHeader(name, value);
    if (!err) {
        http.AddRequestHeader(name, value);
    }
    return 0;
}

int HttpSvc::HsToHttpAdapter::tryCopyRespHeader(const std::string &name)
{
    const std::string *value = http.FindResponseHeader(name);
    if (value) {
        hs->SetRespHeader(name, *value);
    }
    else if (name == "Content-Length") {
        hs->SetRespHeader("Transfer-Encoding", "chunked");
        send_resp_in_chunks = true;
    }
    return 0;
}

int HttpSvc::HsToHttpAdapter::removeReqHeaderFromStream()
{
    int err = 0;

    // Look for separation between message-header and message-body:
    //  Assuming all messages have at least 1 header, so look for CRLFCRLF sequence
    //  See http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4

    int state = 0;
    // state 0 is the initial state
    // state 1 means just saw \r
    // state 2 means just saw \r\n
    // state 3 means just saw \r\n\r
    // state 4 means just saw \r\n\r\n and is the final state

    while (state != 4) {
        char buf[1];
        ssize_t bytes = hs->Read(buf, sizeof(buf));
        if (bytes == 0) {  // EOF
            LOG_ERROR("HsToHttpAdapter[%p]: Premature EOF of request", this);
            Utils::SetCompleteResponse(hs, 400);
            err = -1;
            break;
        }
        if (state == 0) {
            state = buf[0] == '\r' ? 1 : 0;
        }
        else if (state == 1) {
            state = buf[0] == '\n' ? 2 : 0;
        }
        else if (state == 2) {
            state = buf[0] == '\r' ? 3 : 0;
        }
        else if (state == 3) {
            state = buf[0] == '\n' ? 4 : 0;
        }
        else {
            // unexpected state
            LOG_ERROR("HsToHttpAdapter[%p]: Internal error", this);
            Utils::SetCompleteResponse(hs, 500);
            err = -1;
            break;
        }
    }

    return err;
}
