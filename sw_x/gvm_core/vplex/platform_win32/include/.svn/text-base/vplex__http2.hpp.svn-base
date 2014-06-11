#ifndef __VPLEX__HTTP2_HPP__
#define __VPLEX__HTTP2_HPP__

#include <vpl_th.h>
#include <vpl_time.h>
#include <vplu_types.h>
#include <vplex_util.hpp>

#include <vplu__missing.h>  // for ssize_t

#include "vplex_http2_cb.hpp"

#include <string>
#include <map>

#include <Winhttp.h>

class VPLHttp2;

class VPLHttp2__Impl;

typedef struct _ASYNC_HTTP_REQ
{
    HINTERNET ReqHandle;
    HANDLE ReqDoneEvent;
    VPL_BOOL recvDone;
    DWORD vplError;  // translated from WinHttp_Err code
    char *buffer;
    DWORD bufSize;
    DWORD sslErr;

    DWORD rxTotalSize;
    DWORD rxRecievedSize;
    VPLHttp2_RecvCb rxCB;
    void *rxCtx;
    VPLHttp2_ProgressCb rxProgCB;
    void *rxProgCtx;
    VPL_BOOL rxHasContentLength;

    VPLHttp2_SendCb txCB;
    void *txProgCtx;
    void *txCtx;
    VPLHttp2_ProgressCb txProgCB;
    const char *txPayload;
    DWORD txTotalSize;
    DWORD txRemaining;
    DWORD txSize2Send;
} AsyncHttpReq;

class VPLHttp2__Impl {
public:
    VPLHttp2__Impl(VPLHttp2 *myInterface);
    ~VPLHttp2__Impl();

    ///
    /// This should be called from a single thread before using any of the other
    /// VPLHttp2 functions.
    ///
    static int Init(void);

    ///
    /// For a clean shutdown, this should be called by the main thread before exiting.
    /// It is safe to call this even if #Init() hasn't been called.
    ///
    static void Shutdown(void);

    // Enable debug
    int SetDebug(bool debug);

    // Intention is to set network inactivity timeout.
    // However, implementation (depending on what's available) may decide to ignore it.
    int SetTimeout(VPLTime_t timeout);


    // Set whole URI line.
    // Example: SetUri("https://127.0.0.1:12345/clouddoc/dir?index=0&max=10");
    // Error is returned if "uri" is malformed.
    // Passed string is used as is; that is, no reencoding of any kind.
    int SetUri(const std::string &uri);

    // Add a request header.
    // Example: AddRequestHeader("x-ac-userId", "12345");
    // Order of headers is preserved.
    int AddRequestHeader(const std::string &field, const std::string &value);


    // Methods to send a GET request.
    // Three functions, depending on destination for response body.

    // Response body goes into a std::string object (respBody).
    int Get(std::string &respBody);
    // Response body goes to a file (respBodyFilePath).
    int Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // Response body goes to a callback function (recvCB) as it is received.
    int Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);


    // Methods to send a PUT request.
    // Three functions, depending on source of request body.
    // Any response body goes into a std::string object (respBody).  (Exceptation is that response body is small.)

    // Request body is in a std::string object (reqBody).
    int Put(const std::string &reqBody,
        std::string &respBody);
    // Request body is in a file (reqBodyFilePath).
    int Put(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        std::string &respBody);
    // Request body obtained incrementally by repeatedly calling a callback function (sendCb).
    int Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        std::string &respBody);


    // Methods to send a POST request.
    // Three functions, depending on source of request body.
    // Any response body goes into a std::string object (respBody).  (Exceptation is that response body is small.)

    // Request body is in a std::string object (reqBody).
    int Post(const std::string &reqBody,
         std::string &respBody);
    // Request body is in a file (reqBodyFilePath).
    int Post(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
         std::string &respBody);
    // Request body obtained incrementally by repeatedly calling a callback function (sendCb).
    int Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
         std::string &respBody);

    // send from std::string, recv using callback
    int Post(const std::string &reqBody,
         VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);

    // Method to send a DELETE request.
    int Delete(std::string& respBody);

    // Get HTTP status code.
    int GetStatusCode();

    // Find response header value.
    // Note, field names are case insensitive.
    // Return NULL if not present.
    // Example: FindResponseHeaderValue("Content-Length") -> "345"
    const std::string *FindResponseHeader(const std::string &field);

    // Abort transfer.
    int Cancel();

    int receiveResponse(char *buf, size_t bytesToWrite);

    bool isCancelled() {return this->cancel;}

    VPLMutex_t mutex;

    AsyncHttpReq req;

    VPLHttp2 *pVPLHttp;

    bool debugOn;

    void setHttpCode(int code) { httpCode = code;}
private:

#if defined(DELETE)
#undef DELETE
#endif
    enum HttpMethod
    {
        INVALID,
        GET,
        PUT,
        POST,
        DELETE,
    };
    HttpMethod httpMethod;

    // The implementation of FindResponseHeader requires this to use case_insensitive_less.
    typedef std::map<std::string, std::string, case_insensitive_less> header_list;
    typedef VPLUtil_TimeOrderedMap<std::string, std::string, case_insensitive_less> unordered_header_list;

    // Note: This must remain above 8KB.
    // From http://msdn.microsoft.com/en-us/library/windows/desktop/aa384104%28v=vs.85%29.aspx:
    // "it is best to use a read buffer that is comparable in size, or larger than the internal read buffer used by WinHTTP, which is 8 KB."
    static const ssize_t CHUNK_SIZE = 64 * 1024;

    // store req headers by ourselves. setting headers for windows requires request handler.
    unordered_header_list request_headers;
    // response headers
    header_list response_headers;

    std::string requestUri;

    HINTERNET sessionHandle;

    int httpCode;

    bool handleInUse;

    bool cancel;

    VPLTime_t timeout;

    int makeHttpRequest(HttpMethod verb,
                        VPLHttp2_SendCb txCB, void *txCtx,
                        VPLHttp2_RecvCb rxCB, void *rxCtx,
                        VPLHttp2_ProgressCb txProgCB, void *txProgCtx,
                        VPLHttp2_ProgressCb rxProgCB, void *rxProgCtx,
                        const char *txPayload,
                        DWORD txSize);

    static s32 recvToStringCB(VPLHttp2 *http, void *ctx, const char *buf, u32 size);
    static s32 recvToFileCB(VPLHttp2 *http, void *ctx, const char *buf, u32 size);
    static s32 sendFromFileCB(VPLHttp2 *http, void *ctx, char *buf, u32 size);
};


#endif // __VPLEX__HTTP2_HPP__

