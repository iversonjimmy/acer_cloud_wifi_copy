#include <locale>
#include <vpl_time.h>
#include <vplex_file.h>
#include <vplex_private.h>
#include <vplex_strings.h>
#include <vplu_common.h>
#include <vplu_types.h>
#include <vplu_mutex_autolock.hpp>
#include "scopeguard.hpp"

#include "vplex__http2.hpp"

#include <Winhttp.h>

#pragma comment(lib, "winhttp.lib")

#define GET_HANDLE(mutex, handle) \
    BEGIN_MULTI_STATEMENT_MACRO \
    VPLMutex_Lock((mutex)); \
    if ((handle) == true) { \
        VPLMutex_Unlock((mutex)); \
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "handle is already in-use. abort"); \
        return -1; \
    } \
    (handle) = true; \
    VPLMutex_Unlock((mutex)); \
    END_MULTI_STATEMENT_MACRO

#define PUT_HANDLE(mutex, handle) \
    BEGIN_MULTI_STATEMENT_MACRO \
    VPLMutex_Lock((mutex)); \
    (handle) = false; \
    VPLMutex_Unlock((mutex)); \
    END_MULTI_STATEMENT_MACRO

//forward declaration
static void CALLBACK asyncHandlerCB(
        HINTERNET handle,
        DWORD_PTR context,
        DWORD status,
        LPVOID statusInfo,
        DWORD statusInfoLen);

#define WIN32_HTTP_ERR_MAP(ERR_STATUS, VPL_ERR_CODE) \
    case ERR_STATUS: \
       return VPL_ERR_CODE; \
    break;

static int VPLXlat_WinHttp_Err(DWORD err, DWORD sslErr)
{
    switch (err) {
        case ERROR_SUCCESS:
            return VPL_OK;
            break;
        case ERROR_WINHTTP_SECURE_FAILURE:
            if (sslErr == WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID) {
               return VPL_ERR_SSL_DATE_INVALID;
            } else {
               return VPL_ERR_SSL;
            }
            break;
        WIN32_HTTP_ERR_MAP(ERROR_WINHTTP_TIMEOUT,           VPL_ERR_TIMEOUT);
        WIN32_HTTP_ERR_MAP(ERROR_WINHTTP_NAME_NOT_RESOLVED, VPL_ERR_UNREACH);
        WIN32_HTTP_ERR_MAP(ERROR_WINHTTP_CANNOT_CONNECT,    VPL_ERR_CONNREFUSED);
        WIN32_HTTP_ERR_MAP(ERROR_WINHTTP_CONNECTION_ERROR,  VPL_ERR_NETRESET);
        WIN32_HTTP_ERR_MAP(ERROR_WINHTTP_INVALID_URL,       VPL_ERR_INVALID);
        WIN32_HTTP_ERR_MAP(ERROR_WINHTTP_INVALID_OPTION,    VPL_ERR_INVALID);
        WIN32_HTTP_ERR_MAP(ERROR_WINHTTP_OPERATION_CANCELLED, VPL_ERR_CANCELED);

        default:
            VPLError_ReportUnknownErr(err, "VPL_ERR_HTTP_ENGINE");
            return VPL_ERR_HTTP_ENGINE;
            break;
    }
}

// Some error information is only obtainable via this callback
static void CALLBACK statusCallback(
        HINTERNET handle,
        DWORD_PTR context,
        DWORD status,
        LPVOID statusInfo,
        DWORD statusInfoLen)
{
    switch (status) {
    case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
        if (context != 0) {
            *(DWORD*)context = *(DWORD*)statusInfo;
        }
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error retrieving SSL cert from server, error = 0x%x", *((DWORD*)statusInfo));
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            // BUG 1723: statusInfo is pointer to WINHTTP_ASYNC_RESULT struct
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384121(v=vs.85).aspx
            if (statusInfo == NULL) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                                "Error sending an HTTP request");
            }
            else {
                WINHTTP_ASYNC_RESULT *async_result = (WINHTTP_ASYNC_RESULT*)statusInfo;
                VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                                "Error sending an HTTP request, error = 0x%x, result = 0x%x",
                                async_result->dwError, async_result->dwResult);
            }
        }
        break;
    case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "CLOSING_CONNECTION");
        break;
    case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "CONNECTED_TO_SERVER");
        break;
    case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "CONNECTING_TO_SERVER");
        break;
    case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "CONNECTION_CLOSED");
        break;
    case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "HANDLE_CLOSING");
        break;
    case WINHTTP_CALLBACK_STATUS_HANDLE_CREATED:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "HANDLE_CREATED");
        break;
    case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "INTERMEDIATE_RESPONSE");
        break;
    case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "NAME_RESOLVED");
        break;
    case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "RECEIVING_RESPONSE");
        break;
    case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "RESPONSE_RECEIVED");
        break;
    case WINHTTP_CALLBACK_STATUS_REDIRECT:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "REDIRECT");
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "REQUEST_SENT");
        break;
    case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "RESOLVING_NAME");
        break;
    case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "SENDING_REQUEST");
        break;
    default:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Unknown Status %d Given", status);
        break;
    }
}

static inline void closeFile(VPLFile_handle_t &fileHandle) {
    if (VPLFile_IsValidHandle(fileHandle)) {
        VPLFile_Close(fileHandle);
        fileHandle = VPLFILE_INVALID_HANDLE;
    }
}

static int openReadFile(const char *file, VPLFile_handle_t &fileHandle, DWORD &size)
{
    int rv = VPL_OK;

    fileHandle = VPLFile_Open(file, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fileHandle)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Fail to open source file %s", file);
        rv = -1;
        goto out;
    }

    VPLFile_offset_t tmplSz = VPLFile_Seek(fileHandle, 0, VPLFILE_SEEK_END);
    VPLFile_Seek(fileHandle, 0, VPLFILE_SEEK_SET); // Reset
    if (tmplSz > UINT32_MAX) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "File size too large: "FMTu_VPLFile_offset_t" bytes", tmplSz);
        rv = VPL_ERR_NOSYS;
        goto out;
    }

    size = (DWORD)tmplSz;

    return rv;

out:
    closeFile(fileHandle);
    return rv;
}

static int openWriteFile(const char *file, VPLFile_handle_t &fileHandle)
{
    int rv = VPL_OK;
    const int writePermissions = VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE;

    fileHandle = VPLFile_Open(file, writePermissions, 0666);
    if (!VPLFile_IsValidHandle(fileHandle)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Fail to open source file %s", file);
        rv = -1;
    }
    return rv;
}

VPLHttp2__Impl::VPLHttp2__Impl(VPLHttp2 *myInterface)
    : httpCode(-1), handleInUse(false), cancel(false), timeout(0), debugOn(false)
{
    // init file handle
    this->pVPLHttp = myInterface;

    VPLMutex_Init(&(this->mutex));

    this->sessionHandle = WinHttpOpen(NULL,   // User agent
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS,
                                      WINHTTP_FLAG_ASYNC);     // Flags

    if (this->sessionHandle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "unable to create session handle");
        return;
    }
    req.buffer = NULL;
}

VPLHttp2__Impl::~VPLHttp2__Impl()
{
    if (this->sessionHandle != NULL) {
        WinHttpCloseHandle(this->sessionHandle);
    }
    VPLMutex_Destroy(&(this->mutex));

    if (req.ReqDoneEvent != NULL) {
        CloseHandle(req.ReqDoneEvent);
        req.ReqDoneEvent = NULL;
    }
    if (req.buffer) {
        free(req.buffer);
        req.buffer = NULL;
    }

}

int VPLHttp2__Impl::Init(void)
{
    return VPL_OK;
}

void VPLHttp2__Impl::Shutdown(void)
{

}

int VPLHttp2__Impl::SetDebug(bool debug)
{
    int rv = 0;
    if (debug) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "SetDebug %d", debug);
        debugOn = true;
    }
    return rv;
}

int VPLHttp2__Impl::SetTimeout(VPLTime_t timeout)
{
    this->timeout = timeout;
    return 0;
}

int VPLHttp2__Impl::SetUri(const std::string &uri)
{
    this->requestUri = uri;

    return 0;
}

int VPLHttp2__Impl::AddRequestHeader(const std::string &field, const std::string &value)
{
    // Possibly consolidate request_headers
    this->request_headers[field] = value;

    return 0;
}

int VPLHttp2__Impl::makeHttpRequest(HttpMethod verb,
                                    VPLHttp2_SendCb txCB=NULL, void *txCtx=NULL,
                                    VPLHttp2_RecvCb rxCB=NULL, void *rxCtx=NULL,
                                    VPLHttp2_ProgressCb txProgCB=NULL, void *txProgCtx=NULL,
                                    VPLHttp2_ProgressCb rxProgCB=NULL, void *rxProgCtx=NULL,
                                    const char *txPayload=NULL, DWORD txSize=0)
{
    int rv = VPL_OK;

    LPWSTR wcURL = NULL, wcHost = NULL, wcPath = NULL, wcVerb = NULL;
    HINTERNET hConnectionHandle = NULL, hRequestHandle = NULL;
    unordered_header_list::iterator it;
    VPL_BOOL secure = VPLString_StartsWith(this->requestUri.c_str(), "https");
    VPL_BOOL rxHasContentLength = VPL_FALSE;
    bool carryData = (verb == POST || verb == PUT);
    bool isLock = false;

    // clean-up response header
    this->response_headers.clear();

    if (this->sessionHandle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "unable to create session handle");
        rv = -1;
        goto out;
    }
    if (verb == INVALID) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "invalid http method requested. shouldn't happen");
        rv = -1;
        goto out;
    }
    if ((verb == PUT || verb == POST) && txPayload == NULL && txCB == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "both send payload and callback are null. unable to send. abort!");
        rv = -1;
        goto out;
    }
    if ((verb == GET) && rxCB == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "receive callback is null. unable to receive. abort!");
        rv = -1;
        goto out;
    }

    // Convert the URL from UTF-8 to UTF-16
    rv = _VPL__utf8_to_wstring(this->requestUri.c_str(), &wcURL);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert URL (%s) from UTF8 to UTF16: %d", this->requestUri.c_str(), rv);
        rv = -1;
        goto out;
    }
    // NOTE: caller must free wcURL after use.
    //   which is done at the bottom of this function.

    if (WinHttpSetStatusCallback(this->sessionHandle,
                                 asyncHandlerCB,
                                 WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
                                 (DWORD_PTR)NULL) == WINHTTP_INVALID_STATUS_CALLBACK) {
        DWORD err = GetLastError(); 
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpSetStatusCallback", err);
        rv = VPLXlat_WinHttp_Err(err, 0);
        goto out;
    }

    // WinHttp requires the host and path to be passed in as separate
    // zero-terminated strings.
    // WinHttpCrackUrl gives us in-place pointers with lengths.
    URL_COMPONENTS urlComp;
    memset(&urlComp, 0, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    if (!WinHttpCrackUrl(wcURL, 0, 0, &urlComp)) {
        DWORD err = GetLastError();
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD", URL=%s", "WinHttpCrackUrl", err, this->requestUri.c_str());
        rv = VPLXlat_WinHttp_Err(err, 0);
        goto out;
    }
    wcHost = (LPWSTR)malloc(sizeof(WCHAR) * (urlComp.dwHostNameLength + 1));
    wcPath = (LPWSTR)malloc(sizeof(WCHAR) * (urlComp.dwUrlPathLength + 1));
    if (!wcHost || !wcPath) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "malloc failed");
        rv = VPL_ERR_NOMEM;
        goto out;
    }
    wcsncpy(wcHost, urlComp.lpszHostName, urlComp.dwHostNameLength);
    wcHost[urlComp.dwHostNameLength] = L'\0';
    wcsncpy(wcPath, urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    wcPath[urlComp.dwUrlPathLength] = L'\0';

    // Note: despite its name, WinHttpConnect() doesn't perform any network I/O yet.
    // See http://msdn.microsoft.com/en-us/library/windows/desktop/aa384270%28v=vs.85%29.aspx
    hConnectionHandle = WinHttpConnect(this->sessionHandle, wcHost, urlComp.nPort, 0);
    if (hConnectionHandle == NULL) {
        DWORD err = GetLastError();
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpConnect", err);
        rv = VPLXlat_WinHttp_Err(err, 0);
        goto out;
    }

    // Convert the verb from UTF-8 to UTF-16
    switch(verb) {
    case GET:
        rv = _VPL__utf8_to_wstring("GET", &wcVerb);
        break;
    case PUT:
        rv = _VPL__utf8_to_wstring("PUT", &wcVerb);
        break;
    case POST:
        rv = _VPL__utf8_to_wstring("POST", &wcVerb);
        break;
    case DELETE:
        rv = _VPL__utf8_to_wstring("DELETE", &wcVerb);
        break;
    case INVALID:
        rv = -1;
        break;
    }
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert http method (%d) from UTF8 to UTF16: %d", verb, rv);
        goto out;
    }
    // NOTE: caller must free wcVerb after use.
    //   which is done at the bottom of this function.

    /*
      BOOL WINAPI WinHttpSetTimeouts(
              _In_  HINTERNET hInternet,
              _In_  int dwResolveTimeout,
              _In_  int dwConnectTimeout,
              _In_  int dwSendTimeout,
              _In_  int dwReceiveTimeout
      );
    */
    if (this->timeout > 0) {
        // convert from microseconds to milliseconds for that best we get at WinHttp
        long timeout_ms = (long)((this->timeout + 999L) / 1000L);

        if (!WinHttpSetTimeouts(this->sessionHandle, timeout_ms, timeout_ms, timeout_ms, timeout_ms)) {
            DWORD err = GetLastError();
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpSetTimeouts", err);
            rv = VPLXlat_WinHttp_Err(err, 0);
            goto out;
        }
    }

    // Prepare and Start a request.
    VPLMutex_Lock(&mutex);
    isLock = true;

    // Must avoid calling WinHttpOpenRequest if the operation is already canceled.
    // Once we create the request handle, we must stay within this function until we
    // get the WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING callback.
    if (this->cancel) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "HTTP operation was canceled; don't call WinHttpOpenRequest.");
        rv = VPL_ERR_CANCELED;
        goto out;
    }

    // Note: despite its name, WinHttpOpenRequest() doesn't perform any network I/O yet.
    // See http://msdn.microsoft.com/en-us/library/windows/desktop/aa384270%28v=vs.85%29.aspx
    hRequestHandle = WinHttpOpenRequest(hConnectionHandle,
                                        wcVerb,
                                        wcPath,
                                        NULL,
                                        WINHTTP_NO_REFERER,
                                        WINHTTP_DEFAULT_ACCEPT_TYPES,
                                        secure ? WINHTTP_FLAG_SECURE : 0);
    if (hRequestHandle == NULL) {
        DWORD err = GetLastError();
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpOpenRequest", err);
        rv = VPLXlat_WinHttp_Err(err, 0);
        goto out;
    }

    if (debugOn) {
        char *uVerb = NULL;
        int rc = _VPL__wstring_to_utf8_alloc(wcVerb, &uVerb);
        if (rc != VPL_OK || uVerb == NULL) {
            VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "Failed to convert http method wstring to utf-8: %d", rc);
            VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "HTTP Request: (method = %d) %s", verb, this->requestUri.c_str());
        } else {
            VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "HTTP Request: %s %s", uVerb, this->requestUri.c_str());
            free(uVerb);
        }
    }

    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Http Request [%p] Starts.", &req);
    req.sslErr = 0;
    req.rxCB = rxCB;
    req.rxCtx = rxCtx;
    req.rxProgCB = rxProgCB;
    req.rxProgCtx = rxProgCtx;
    req.rxHasContentLength = VPL_FALSE;
    req.recvDone = VPL_FALSE;
    req.rxTotalSize = 0;
    req.rxRecievedSize = 0;
    req.txCB = txCB;
    req.txCtx = txCtx;
    req.txPayload = txPayload;
    req.txProgCB = txProgCB;
    req.txProgCtx = txProgCtx;
    req.txTotalSize = txSize;
    req.txRemaining = txSize;
    req.ReqHandle = hRequestHandle;
    req.vplError = VPL_OK;
    req.bufSize = CHUNK_SIZE;
    if (req.buffer)
        free(req.buffer);
    req.buffer = (char*)malloc(req.bufSize);
    if (req.buffer == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "malloc failed");
        rv = VPL_ERR_NOMEM;
        goto out;
    }

    req.ReqDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (req.ReqDoneEvent == NULL)
    {
        DWORD err = GetLastError();
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "CreateEvent Error(%d)", err);
        rv = -1;
        goto out;
    }

    // This may look a little strange, but WinHttpSetOption does indeed demand a pointer to
    // a buffer containing our pointer.  It does seem to copy the buffer, so it doesn't maintain
    // &pHttp2 (which would be bad since it's a stack address).
    VPLHttp2__Impl* pHttp2 = this;
    if (!WinHttpSetOption(req.ReqHandle,
                          WINHTTP_OPTION_CONTEXT_VALUE,
                          &pHttp2,
                          sizeof(VPLHttp2__Impl*)))
    {
        DWORD err = GetLastError();
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpSetOption", err);
        rv = VPLXlat_WinHttp_Err(err, req.sslErr);
        goto out;
    }

    // Add caller's headers.
    for (it = this->request_headers.begin(); it != this->request_headers.end(); it++) {
        std::string tmp = it->first+": "+it->second;
        // We have to convert each header to UTF16
        LPWSTR wcHeader;
        rv = _VPL__utf8_to_wstring(tmp.c_str(), &wcHeader);
        if (rv != VPL_OK) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert header (%s) from UTF8 to UTF16: %d", tmp.c_str(), rv);
            goto out;
        }
        // NOTE: caller must free wcHeader after use.

        BOOL bRv = WinHttpAddRequestHeaders(req.ReqHandle,
                                            wcHeader,
                                            -1,
                                            WINHTTP_ADDREQ_FLAG_ADD);
        free(wcHeader);
        if (!bRv) {
            DWORD err = GetLastError();
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpAddRequestHeaders", err);
            rv = VPLXlat_WinHttp_Err(err, req.sslErr);
            goto out;
        }
        if (debugOn) VPL_LogHttpBuffer("send header", tmp.c_str(), tmp.length());
    }

    // check if we need to call the send callback
    if (txPayload != NULL || !carryData) { // Payload is in memory.
        if (debugOn && (txSize > 0)) {
            VPL_LogHttpBuffer("send", txPayload, txSize);
        }
        if (!WinHttpSendRequest(req.ReqHandle,
                                WINHTTP_NO_ADDITIONAL_HEADERS,
                                0,  // Length of additional headers
                                carryData ? (LPVOID)txPayload : WINHTTP_NO_REQUEST_DATA,
                                carryData ? txSize : 0,
                                carryData ? txSize : 0,
                                0)) { // Callback context
            DWORD err = GetLastError();
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpSendRequest", err);
            rv = VPLXlat_WinHttp_Err(err, req.sslErr);
            goto out;
        }
        req.txRemaining = 0;
    } else {
        if (!WinHttpSendRequest(req.ReqHandle,
                                WINHTTP_NO_ADDITIONAL_HEADERS,
                                0,  // Length of additional headers
                                WINHTTP_NO_REQUEST_DATA,
                                0,
                                txSize,
                                0)) { // Callback context
            DWORD err = GetLastError();
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpSendRequest", err);
            rv = VPLXlat_WinHttp_Err(err, req.sslErr);
            goto out;
        }
    }

    VPLMutex_Unlock(&(this->mutex));
    isLock = false;

    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Waiting for Http Request[%p] to complete.", &req);
    if (WaitForSingleObject(req.ReqDoneEvent, INFINITE) == WAIT_FAILED)
    {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "WaitForSingleObject failed");
        rv = VPLError_XlatWinErrno(GetLastError());
        goto out;
    }

    VPLMutex_Lock(&(this->mutex));
    isLock = true;

    if (debugOn) {
        VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "Finished receiving: hasRxContentLength=%d, expect=%u, total=%u",
            req.rxHasContentLength, (unsigned int)req.rxTotalSize, (unsigned int)req.rxRecievedSize);
    }

    // if content-length is missing in the response header. skip the check
    if (req.rxHasContentLength == VPL_TRUE && req.rxTotalSize != req.rxRecievedSize) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Transferred a partial file for %s. expect=%u, actual=%u",
                        this->requestUri.c_str(), (unsigned int)req.rxTotalSize, (unsigned int)req.rxRecievedSize);
        rv = VPL_ERR_RESPONSE_TRUNCATED;
    }

out:
    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Http Request[%p] Ended", &req);

    // Must hold the mutex to call the WinHttp APIs and check/set fields in "req".
    if (!isLock){
        VPLMutex_Lock(&(this->mutex));
    }

    // In the case when something failed, suppress the callback for the
    // WinHttpCloseHandle(req.ReqHandle) call we are about to make.
    WinHttpSetStatusCallback(this->sessionHandle,
                NULL,
                WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
                NULL);

    if (req.ReqHandle) {
        WinHttpCloseHandle(req.ReqHandle);
        req.ReqHandle = NULL;
    }

    if (rv == VPL_OK) {
        rv = req.vplError;
    }
    // This might not actually be needed, but it's better to be safe than sorry.
    // YL reported that it is possible to get the WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING without
    // WINHTTP_CALLBACK_STATUS_REQUEST_ERROR.  I'd expect that to happen only if we already
    // got WINHTTP_CALLBACK_STATUS_READ_COMPLETE with (statusInfoLen == 0).
    // This should detect if for some reason we didn't get the WINHTTP_CALLBACK_STATUS_READ_COMPLETE either.
    if (rv == VPL_OK && !req.recvDone) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "No error reported, but the response wasn't received either; assume canceled.");
        rv = VPL_ERR_CANCELED;
    }

    VPLMutex_Unlock(&(this->mutex));

    if (hConnectionHandle) {
        WinHttpCloseHandle(hConnectionHandle);
    }
    if (wcURL) {
        free(wcURL);
    }
    if (wcHost) {
        free(wcHost);
    }
    if (wcPath) {
        free(wcPath);
    }
    if (wcVerb) {
        free(wcVerb);
    }

    return rv;
}

int VPLHttp2__Impl::Get(std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    rv = makeHttpRequest(GET,
                         NULL, NULL,
                         recvToStringCB, (void *)&respBody);

    PUT_HANDLE(&(this->mutex), this->handleInUse);
    return rv;
}

int VPLHttp2__Impl::Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    VPLFile_handle_t fHandle = VPLFILE_INVALID_HANDLE;
    this->httpCode = -1;

    rv = openWriteFile(respBodyFilePath.c_str(), fHandle);
    if (rv) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unable to open file %s", respBodyFilePath.c_str());
        goto out;
    }
    rv = makeHttpRequest(GET,
                         NULL, NULL,
                         recvToFileCB, (void *)&fHandle,
                         NULL, NULL,
                         recvProgCb, recvProgCtx);
out:
    closeFile(fHandle);

    PUT_HANDLE(&(this->mutex), this->handleInUse);
    return rv;
}

int VPLHttp2__Impl::Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    rv = makeHttpRequest(GET,
                         NULL, NULL,
                         recvCb, recvCtx,
                         NULL, NULL,
                         recvProgCb, recvProgCtx);

    PUT_HANDLE(&(this->mutex), this->handleInUse);
    return rv;

}

int VPLHttp2__Impl::Put(const std::string &reqBody,
                        std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    rv = makeHttpRequest(PUT,
                         NULL, NULL,
                         recvToStringCB, (void *)&respBody,
                         NULL, NULL,
                         NULL, NULL,
                         reqBody.c_str(), static_cast<DWORD>(reqBody.size()));

    PUT_HANDLE(&(this->mutex), this->handleInUse);
    return rv;
}

int VPLHttp2__Impl::Put(const std::string &reqBodyFilePath,
                        VPLHttp2_ProgressCb sendProgCb,
                        void *sendProgCtx,
                        std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    VPLFile_handle_t fHandle = VPLFILE_INVALID_HANDLE;
    this->httpCode = -1;

    DWORD txSize = 0;
    rv = openReadFile(reqBodyFilePath.c_str(), fHandle, txSize);
    if (rv) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unable to open file %s", reqBodyFilePath.c_str());
        goto out;
    }
    rv = makeHttpRequest(PUT,
                         sendFromFileCB, (void *)&fHandle,
                         recvToStringCB, (void *)&respBody,
                         sendProgCb, sendProgCtx,
                         NULL, NULL,
                         NULL, txSize);
out:
    closeFile(fHandle);

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Put(VPLHttp2_SendCb sendCb,
                        void *sendCtx,
                        u64 sendSize,
                        VPLHttp2_ProgressCb sendProgCb,
                        void *sendProgCtx,
                        std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    rv = makeHttpRequest(PUT,
                         sendCb, sendCtx,
                         recvToStringCB, (void *)&respBody,
                         sendProgCb, sendProgCtx,
                         NULL, NULL,
                         NULL, (DWORD)sendSize);

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Post(const std::string &reqBody,
                         std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    rv = makeHttpRequest(POST,
                         NULL, NULL,
                         recvToStringCB, (void *)&respBody,
                         NULL, NULL,
                         NULL, NULL,
                         reqBody.c_str(), static_cast<DWORD>(reqBody.size()));

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Post(const std::string &reqBodyFilePath,
                         VPLHttp2_ProgressCb sendProgCb,
                         void *sendProgCtx,
                         std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    VPLFile_handle_t fHandle = VPLFILE_INVALID_HANDLE;
    this->httpCode = -1;

    DWORD txSize = 0;
    rv = openReadFile(reqBodyFilePath.c_str(), fHandle, txSize);
    if (rv) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unable to open file %s", reqBodyFilePath.c_str());
        goto out;
    }
    rv = makeHttpRequest(POST,
                         sendFromFileCB, (void *)&fHandle,
                         recvToStringCB, (void *)&respBody,
                         sendProgCb, sendProgCtx,
                         NULL, NULL,
                         NULL, txSize);
out:
    closeFile(fHandle);

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Post(VPLHttp2_SendCb sendCb,
                         void *sendCtx, u64 sendSize,
                         VPLHttp2_ProgressCb sendProgCb,
                         void *sendProgCtx,
                         std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    return makeHttpRequest(POST,
                           sendCb, sendCtx,
                           recvToStringCB, (void *)&respBody,
                           sendProgCb, sendProgCtx,
                           NULL, NULL,
                           NULL, (DWORD)sendSize);

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Post(const std::string &reqBody,
                         VPLHttp2_RecvCb recvCb,
                         void *recvCtx,
                         VPLHttp2_ProgressCb recvProgCb,
                         void *recvProgCtx)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    return makeHttpRequest(POST,
                           NULL, NULL,
                           recvCb, recvCtx,
                           NULL, NULL,
                           recvProgCb, recvProgCtx,
                           reqBody.c_str(), static_cast<DWORD>(reqBody.size()));

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Delete(std::string& respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    this->httpCode = -1;
    rv = makeHttpRequest(DELETE,
                         NULL, NULL,
                         recvToStringCB, (void *)&respBody,
                         NULL, NULL,
                         NULL, NULL,
                         NULL, 0);

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::GetStatusCode()
{
    return this->httpCode;
}

const std::string *VPLHttp2__Impl::FindResponseHeader(const std::string &field)
{
    if (response_headers.find(field) != response_headers.end()) {
        return &(response_headers[field]);
    }
    return NULL;
}

int VPLHttp2__Impl::Cancel()
{
    VPLMutex_Lock(&mutex);
    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Cancel requested for Req[%p]", &req);
    cancel = true;

    if (req.ReqHandle) {
        WinHttpCloseHandle(req.ReqHandle);
        req.ReqHandle = NULL;
    }
    VPLMutex_Unlock(&mutex);
    return 0;
}

// internal functions

s32 VPLHttp2__Impl::sendFromFileCB(VPLHttp2 *http, void *ctx, char *buf, u32 size)
{
    VPLFile_handle_t *fileHandle = (VPLFile_handle_t *)ctx;
    if (fileHandle == NULL && VPLFile_IsValidHandle(*fileHandle)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Invalid file handle. Abort!");
        return -1;
    }

    char *curPos = buf;
    size_t bytesToSend = size;
    ssize_t bytesRead = 0;
    do {
        bytesRead = VPLFile_Read(*fileHandle, curPos, bytesToSend);
        if (bytesRead == 0) {
            // EOF
            break;
        } else if (bytesRead < 0) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error %d reading from file after reading %d bytes.",
                            bytesRead, (size-bytesToSend));
            break;
        }
        curPos += bytesRead;
        bytesToSend -= bytesRead;
    } while (bytesToSend > 0);

    return static_cast<s32>(size-bytesToSend);
}

s32 VPLHttp2__Impl::recvToFileCB(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    VPLFile_handle_t *fileHandle = (VPLFile_handle_t *)ctx;

    if (fileHandle == NULL && VPLFile_IsValidHandle(*fileHandle)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Invalid file handle. Abort!");
        return -1;
    }

    size_t bytesToWrite = size;

    // file write loop
    do {
        ssize_t bytesWritten = VPLFile_Write(*fileHandle, buf, bytesToWrite);
        if (bytesWritten < 0) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error %d writing to file after writing %d bytes.",
                            bytesWritten, size-bytesToWrite);
            break;
        }
        bytesToWrite -= bytesWritten;
    } while (bytesToWrite >0);

    return static_cast<s32>(size-bytesToWrite);
}

s32 VPLHttp2__Impl::recvToStringCB(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    std::string *rxToStr = (std::string *)ctx;

    if (rxToStr == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "rxToStr is NULL. Abort!");
        return 0;
    }

    if (size > 0) {
        rxToStr->append(static_cast<const char*>(buf), size);
    }
    return size;
}

static void strip_newline(std::string& line)
{
    size_t pos = line.length() - 1;

    if (line.length() != 0) {
        while(line[pos] == '\n' || line[pos] == '\r') {
            line.erase(pos, 1);
            pos--;
            if (line.length() == 0)
                break;
        }
    }
}

int VPLHttp2__Impl::receiveResponse(char *buf, size_t bytesToWrite)
{
    int rv = 0;
    size_t beginline = 0;

    enum RxState {
        RESPONSE_LINE,
        RESPONSE_HEADER
    };

    RxState receive_state = RESPONSE_LINE;
    std::string line;

    if (buf == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "buffer is NULL. Abort!");
        return -1;
    }

    while(beginline < bytesToWrite) {

        // Break input down into lines. Keep any partial line.
        // Lines end in "\n", with possible "\r\n" endings.
        // Lines may be empty.
        size_t linelen = 0;
        for(size_t i = beginline; i < bytesToWrite; i++) {
            linelen++;
            if (buf[i] == '\n') {
                break;
            }
        }

        line.append(&(buf[beginline]), linelen);
        beginline += linelen;
        rv += static_cast<int>(linelen);
        if(*(line.end() - 1) != '\n') {
            // not enough for a complete line
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "not enough for a complete line. shouldn't happen!");
            rv = -1;
            break;
        }

        // get the response line first and then header fields
        if (receive_state == RESPONSE_LINE) {
            // Collect the command line
            strip_newline(line);
            if(line.length() > 0) {
                if (debugOn) {
                    VPL_LogHttpBuffer("receive response", line.c_str(), line.size());
                }
                // Find result code in the line
                size_t pos = line.find_first_of(' ');
                int tmp = -1;
                line.erase(0, pos);
                sscanf(line.c_str(), "%d", &tmp);
                // XXX ?
                if (tmp != 100) {
                    receive_state = RESPONSE_HEADER;
                    this->httpCode = tmp;
                } else {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "RX 100 Continue. shouldn't happen!");
                    rv = -1;
                    break;
                }
            }
        } else {
            strip_newline(line);
            if (debugOn) {
                VPL_LogHttpBuffer("receive header", line.c_str(), line.size());
            }

            // Collect a header line
            // Headers are of the form (name ':' field) with any amount of whitespace
            // following the ':' before the field value. Whitespace after field is ignored.
            std::string name = line.substr(0, line.find_first_of(':'));
            std::string value = line.substr(line.find_first_of(':')+1);

            // strip leading whitespace on value
            std::locale loc;
            while(value.length() > 0 && isspace(value[0], loc)) {
                value.erase(0, 1);
            }
            // strip trailing whitespace on value
            while(value.length() > 0 && isspace(value[value.length() - 1], loc)) {
                value.erase(value.length() - 1, 1);
            }

            // Possibly consolidate headers
            if(this->response_headers.find(name) != this->response_headers.end()) {
                VPL_LIB_LOG_WARN(VPL_SG_HTTP, "duplicate header found. name = %s\n - origin = %s\n - after = %s",
                                 name.c_str(), this->response_headers[name].c_str(), value.c_str());
            }
            // strip empty field
            if (name.size() > 0) {
                this->response_headers[name] = value;
            }
        }
        line.clear();
    }
    return rv;
}


//
// Functions for Async Http Request
//

int asyncSendReqData(VPLHttp2__Impl& http2Impl)
{
    DWORD dwBytesWritten = 0;
    int rv = VPL_OK;

    AsyncHttpReq* pReq = &(http2Impl.req);

    if(pReq->txRemaining > 0) {
        // get read size from send callback
        ssize_t rdSz = pReq->txCB(http2Impl.pVPLHttp, pReq->txCtx, pReq->buffer, pReq->bufSize);

        if (rdSz <= 0) {
            // Negative is an error.
            // 0 is also unexpected, since we were told the size beforehand.
            VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                    "Req[%p] The VPLHttp2_SendCb callback returned "FMT_ssize_t", remaining="FMT_DWORD,
                    pReq, rdSz, pReq->txRemaining);
            rv = VPL_ERR_FAIL;
            goto out;
        } else { // (rdSz > 0)
            if (http2Impl.debugOn) {
                VPL_LogHttpBuffer("send (from callback)", pReq->buffer, rdSz);
            }
            pReq->txSize2Send = rdSz;
            /*&dwBytesWritten set to null in async mode and check later*/ 
            BOOL bwritten = WinHttpWriteData(pReq->ReqHandle , pReq->buffer, rdSz, NULL/*&dwBytesWritten*/ );
            if(!bwritten) {
                DWORD err = GetLastError();
                rv = VPLXlat_WinHttp_Err(err, pReq->sslErr);
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] %s failed: "FMT_DWORD", rv=%d",
                        pReq, "WinHttpWriteData", err, rv);
                goto out;
            }

            // The size of sent data will be received in asyncHandlerCB with status = WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE.
            // So, checking and progress will be handled there.

        }
    } else {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Send done. ("FMT_DWORD" bytes)", pReq, pReq->txTotalSize);
        if (!WinHttpReceiveResponse(pReq->ReqHandle, NULL)) {
            DWORD err = GetLastError();
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] %s failed: "FMT_DWORD, pReq, "WinHttpReceiveResponse", err);
            rv = VPLXlat_WinHttp_Err(err, pReq->sslErr);
            goto out;
        }
    }
out:
    return rv;
}

int asyncGetResponseHeaders(VPLHttp2__Impl& http2Impl)
{

    DWORD dwSize = 0;
    LPVOID lpOutBuffer = NULL;
    BOOL  bResults = FALSE;
    char *utf8_header = NULL;
    DWORD rv = VPL_OK;

    AsyncHttpReq* pReq = &(http2Impl.req);

    // First, use WinHttpQueryHeaders to obtain the size of the buffer.
    WinHttpQueryHeaders(pReq->ReqHandle, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX, NULL,
                        &dwSize, WINHTTP_NO_HEADER_INDEX);

    // Allocate memory for the buffer.
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        lpOutBuffer = new WCHAR[dwSize/sizeof(WCHAR)];

        // Now, use WinHttpQueryHeaders to retrieve the header.
        bResults = WinHttpQueryHeaders(pReq->ReqHandle,
                                       WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                       WINHTTP_HEADER_NAME_BY_INDEX,
                                       lpOutBuffer, &dwSize,
                                       WINHTTP_NO_HEADER_INDEX);
    }

    // Print the header contents.
    if (bResults) {
        rv = _VPL__wstring_to_utf8_alloc((wchar_t*)lpOutBuffer, &utf8_header);
        if (rv) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] Error while preparing UTF8-header, rv=%d",pReq, rv);
            delete [] lpOutBuffer;
            goto out;
        }
        // TODO: shouldn't this be "strlen(utf8_header) + 1" instead of "dwSize/sizeof(WCHAR)"?
        rv = http2Impl.receiveResponse(utf8_header, dwSize/sizeof(WCHAR));
    } else {
        DWORD err = GetLastError();
        rv = VPLXlat_WinHttp_Err(err, pReq->sslErr);
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] %s failed: "FMT_DWORD", rv=%d",
                pReq, "WinHttpQueryHeaders", err, rv);
    }

    // Free the allocated memory.
    delete [] lpOutBuffer;
    free(utf8_header);

    if (rv >= 0) {
        rv = VPL_OK;
    } else {
        goto out;
    }

    DWORD StatusCode = 0;
    DWORD dwBufSize = sizeof(DWORD);

    if (!WinHttpQueryHeaders(pReq->ReqHandle,
                             WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
                             NULL,
                             &StatusCode,
                             &dwBufSize,
                             NULL)) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Obtain Status Code Failed", pReq);
    } else {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Status Code: %d", pReq, StatusCode);
    }

    pReq->rxTotalSize = 0;
    if (!WinHttpQueryHeaders(pReq->ReqHandle,
                             WINHTTP_QUERY_CONTENT_LENGTH |
                             WINHTTP_QUERY_FLAG_NUMBER,
                             NULL,
                             &pReq->rxTotalSize,
                             &dwBufSize,
                             NULL)) {
        // allowable
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] content-length not available in response header", pReq);
        pReq->rxHasContentLength = VPL_FALSE;

        /*
        //check if context exists.
        if (!WinHttpQueryDataAvailable(pReq->ReqHandle, &pReq->rxTotalSize)) {
            DWORD err = GetLastError();
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "WinHttpQueryDataAvailable", err);
            rv = VPLXlat_WinHttp_Err(err, 0);
            goto out;
        }
        //NO need in asynchronous mode because rxTotalSize remains zero!
        */
    } else {
        pReq->rxHasContentLength = VPL_TRUE;
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Response data length = "FMT_DWORD, pReq, pReq->rxTotalSize);
    }

out:
    pReq->rxRecievedSize = 0;
    return rv;
}

/// Requests WinHttp to fill in the buffer asynchronously.
/// We will get a WINHTTP_CALLBACK_STATUS_READ_COMPLETE when the buffer is ready for us to read.
static int asyncRecvResponseData(VPLHttp2__Impl& http2Impl)
{
    DWORD rv = VPL_OK;
    // Receive Data from Http, but we don't know how much data we can obtain in this stage.
    // The received data size will be available when next asyncHandlerCallback is triggered with WINHTTP_CALLBACK_STATUS_READ_COMPLETE.
    AsyncHttpReq* pReq = &(http2Impl.req);

    if (!WinHttpReadData(pReq->ReqHandle,
                         pReq->buffer,
                         pReq->bufSize,
                         NULL))
    {
        DWORD err = GetLastError();
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] WinHttpReadData Error: %d", pReq, err);
        rv = VPLXlat_WinHttp_Err(err, pReq->sslErr);
    }
    return rv;
}

int asyncPush2RecvCB(VPLHttp2__Impl& http2Impl, DWORD bytesRead)
{
    DWORD rv = VPL_OK;
    AsyncHttpReq* pReq = &(http2Impl.req);
    // call callback here : 1) rx to string 2) rx to file 3) rx to callback
    if (pReq->rxCB != NULL) {
        if (pReq->rxCB(http2Impl.pVPLHttp, pReq->rxCtx, pReq->buffer, bytesRead) != bytesRead) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] receive response error while calling callback", pReq);
            rv = VPL_ERR_IN_RECV_CALLBACK;
            goto out;
        }
    }
    if (http2Impl.debugOn) {
        VPL_LogHttpBuffer("recv", pReq->buffer, bytesRead);
    }

    // advance total bytes read after we actually wrote it
    pReq->rxRecievedSize += bytesRead;

    // call the progress callback if any
    if (pReq->rxProgCB != NULL) {
        pReq->rxProgCB(http2Impl.pVPLHttp, pReq->rxProgCtx, pReq->rxTotalSize, pReq->rxRecievedSize);
    }
out:
    return rv;
}
#define UNHANDLE_CASE(X) case WINHTTP_CALLBACK_STATUS_##X:                                                 \
                              if (http2Impl.debugOn) VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] "#X" (ignored)", pReq);   \
                         break;

/// *CAUTION*: It is possible for additional callbacks to occur in this
/// same thread for this same handle before the current callback completes.
/// See https://bugs.ctbg.acer.com/show_bug.cgi?id=17969 for details.
static void CALLBACK  asyncHandlerCB(
        HINTERNET handle,
        DWORD_PTR context,
        DWORD status,
        LPVOID statusInfo,
        DWORD statusInfoLen)
{
    //http://msdn.microsoft.com/en-us/library/windows/desktop/aa383917(v=vs.85).aspx
    int rv = VPL_OK;
    if(0 == context) {
        // No Request, do nothing.  Not expected.
        return;
    }
    VPLHttp2__Impl& http2Impl = *(reinterpret_cast<VPLHttp2__Impl*>(context));

    MutexAutoLock lock(&(http2Impl.mutex));

    AsyncHttpReq* pReq = &(http2Impl.req);

    switch (status) {
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        if (http2Impl.debugOn) VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE", pReq);
        // Must ensure that the handle is still valid before proceeding.
        if (pReq->ReqHandle == NULL) {
            rv = VPL_ERR_CANCELED;
            goto out;
        }
        rv = asyncSendReqData(http2Impl);
        break;

    case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
        if (http2Impl.debugOn) VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE", pReq);
        // Must ensure that the handle is still valid before proceeding.
        if (pReq->ReqHandle == NULL) {
            rv = VPL_ERR_CANCELED;
            goto out;
        }
        {
            // http://msdn.microsoft.com/en-us/library/windows/desktop/hh707337(v=vs.85).aspx
            DWORD txSize = *((DWORD*) statusInfo);
            //VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Send %d bytes data.", pReq, txSize);

            if(txSize != pReq->txSize2Send) {
                rv = VPL_ERR_HTTP_ENGINE;
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "WinHttpWriteData content missing, data_size_to_write[%d], dwBytesWritten[%d], Request[%p]", pReq->txSize2Send, txSize, pReq);
                goto out;
            }

            pReq->txRemaining -= txSize;

            // call txProgCB to update the send progress here
            if (pReq->txProgCB != NULL) {
                pReq->txProgCB(http2Impl.pVPLHttp, pReq->txProgCtx, pReq->txTotalSize, pReq->txTotalSize - pReq->txRemaining);
            }

            rv = asyncSendReqData(http2Impl);
        }
        break;

    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        if (http2Impl.debugOn) VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE", pReq);
        // Must ensure that the handle is still valid before proceeding.
        if (pReq->ReqHandle == NULL) {
            rv = VPL_ERR_CANCELED;
            goto out;
        }
        rv = asyncGetResponseHeaders(http2Impl);
        if(VPL_OK != rv) break;
        rv = asyncRecvResponseData(http2Impl);
        break;

    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        if (http2Impl.debugOn) VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] WINHTTP_CALLBACK_STATUS_READ_COMPLETE", pReq);

        if ( 0 == statusInfoLen) { // Previous WinHttpReadData obtained no data, indicating end of response.
            VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Recv done. (%d bytes)", pReq, pReq->rxRecievedSize);
            pReq->recvDone = VPL_TRUE;
            if (pReq->ReqHandle != NULL) {
                WinHttpCloseHandle(pReq->ReqHandle);
                pReq->ReqHandle = NULL;
            }
        } else {
            //VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Recv %d bytes data.", pReq, statusInfoLen);
            if (pReq->ReqHandle == NULL) {
                rv = VPL_ERR_CANCELED;
                goto out;
            }
            rv = asyncPush2RecvCB(http2Impl, statusInfoLen); //push recv data from buffer to recvCB
            if (VPL_OK != rv) break;
            rv = asyncRecvResponseData(http2Impl); // read new data
        }
        break;

    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        if (http2Impl.debugOn) VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] WINHTTP_CALLBACK_STATUS_REQUEST_ERROR", pReq);
        {
            // BUG 1723: statusInfo is pointer to WINHTTP_ASYNC_RESULT struct
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa384121(v=vs.85).aspx
            if (statusInfo == NULL) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                                "Req[%p] Error sending an HTTP request", pReq);
            }
            else {
                WINHTTP_ASYNC_RESULT *async_result = (WINHTTP_ASYNC_RESULT*)statusInfo;
                if (ERROR_WINHTTP_OPERATION_CANCELLED == async_result->dwError) {
                    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] Operation is Canceled!", pReq);
                    rv = VPL_ERR_CANCELED;
                } else {
                    rv = VPLXlat_WinHttp_Err(async_result->dwError, pReq->sslErr);
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                                "Req[%p] Error sending an HTTP request, error = 0x%x, result = 0x%x, VPL_ERR=%d",
                                pReq, async_result->dwError, async_result->dwResult, rv);
                    http2Impl.setHttpCode(500);
                }
            }
        }
        break;

    case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
        if (http2Impl.debugOn) VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] WINHTTP_CALLBACK_STATUS_SECURE_FAILURE", pReq);
        {
            pReq->sslErr = *((DWORD*)statusInfo);
            rv = VPLXlat_WinHttp_Err(ERROR_WINHTTP_SECURE_FAILURE, pReq->sslErr);
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] Error retrieving SSL cert from server, error = 0x%x", pReq, *((DWORD*)statusInfo));
        }
        break;
    case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Req[%p] WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING", pReq);
        SetEvent(pReq->ReqDoneEvent);
        break;
    UNHANDLE_CASE(CONNECTING_TO_SERVER);
    UNHANDLE_CASE(CLOSING_CONNECTION);
    UNHANDLE_CASE(CONNECTED_TO_SERVER);
    UNHANDLE_CASE(CONNECTION_CLOSED);
    UNHANDLE_CASE(HANDLE_CREATED);
    UNHANDLE_CASE(INTERMEDIATE_RESPONSE);
    UNHANDLE_CASE(NAME_RESOLVED);
    UNHANDLE_CASE(RECEIVING_RESPONSE);
    UNHANDLE_CASE(RESPONSE_RECEIVED);
    UNHANDLE_CASE(REQUEST_SENT);
    UNHANDLE_CASE(RESOLVING_NAME);
    UNHANDLE_CASE(SENDING_REQUEST);
    default:
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] Unknown Status: "FMT_DWORD, pReq, status);
        break;
    }

out:

    if ( VPL_OK != rv ) {

        // prevent pReq->vplError is overwritten by VPL_ERR_CANCELED if any
        // other error happens before and then closes ReqHandle.
        if (pReq->vplError == VPL_OK) {
            pReq->vplError = rv;
        }

        if (VPL_ERR_CANCELED != rv) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Req[%p] Request was terminated abnormally.", pReq);
            if (pReq->ReqHandle != NULL) {
                WinHttpCloseHandle(pReq->ReqHandle);
                pReq->ReqHandle = NULL;
            }
        }
    }
}

