#include "vplex_http2.hpp"
#include "vplex_private.h"
#include "vplex_strings.h"
#include "scopeguard.hpp"

#include <string>
#include <ppltasks.h>
#include <thread>

using namespace std;
using namespace Concurrency;
using namespace Platform;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

#define MAX_BUFFER_LENGTH 10*1024

#define CHECK_CANCEL(mutex, cancel, send, ctx, size, hr) \
    BEGIN_MULTI_STATEMENT_MACRO \
    VPLMutex_Lock((mutex)); \
    if ((cancel) == true) { \
        hr = E_ABORT; \
    } else {\
        hr = send(ctx, size); \
    } \
    VPLMutex_Unlock((mutex)); \
    END_MULTI_STATEMENT_MACRO


#define   WINRT_INET_ERR_MAP(ERR_STATUS, VPL_ERR_CODE) \
    case ERR_STATUS: \
       rv = VPL_ERR_CODE; \
       VPL_REPORT_WARN("WinRT http2 error %s is mapped to %s", #ERR_STATUS, #VPL_ERR_CODE); \
    break;

int VPLError_XlatWinRTInetErrno(DWORD err_code)
{
    //reference : http://msdn.microsoft.com/en-us/library/ms775145(v=vs.85).aspx
    int rv = VPL_OK;
    switch(err_code) {
        case ERROR_SUCCESS:
            rv = VPL_OK;
        break;
        WINRT_INET_ERR_MAP(INET_E_INVALID_URL,                  VPL_ERR_INVALID);
        WINRT_INET_ERR_MAP(INET_E_NO_SESSION,                   VPL_ERR_INVALID_SESSION);
        WINRT_INET_ERR_MAP(INET_E_CANNOT_CONNECT,               VPL_ERR_CONNREFUSED); 
        WINRT_INET_ERR_MAP(INET_E_RESOURCE_NOT_FOUND,           VPL_ERR_UNREACH); //The server or proxy was not found.
        WINRT_INET_ERR_MAP(INET_E_OBJECT_NOT_FOUND,             VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_DATA_NOT_AVAILABLE,           VPL_ERR_IO);
        WINRT_INET_ERR_MAP(INET_E_DOWNLOAD_FAILURE,             VPL_ERR_IO);
        WINRT_INET_ERR_MAP(INET_E_AUTHENTICATION_REQUIRED,      VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_NO_VALID_MEDIA,               VPL_ERR_INVALID);
        WINRT_INET_ERR_MAP(INET_E_CONNECTION_TIMEOUT,           VPL_ERR_TIMEOUT);
        WINRT_INET_ERR_MAP(INET_E_INVALID_REQUEST,              VPL_ERR_INVALID);
        WINRT_INET_ERR_MAP(INET_E_UNKNOWN_PROTOCOL,             VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_SECURITY_PROBLEM,             VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_CANNOT_LOAD_DATA,             VPL_ERR_IO);
        WINRT_INET_ERR_MAP(INET_E_CANNOT_INSTANTIATE_OBJECT,    VPL_ERR_FAIL); //CoCreateInstance failed.
        WINRT_INET_ERR_MAP(INET_E_INVALID_CERTIFICATE,          VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_REDIRECT_FAILED,              VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_REDIRECT_TO_DIR,              VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_CANNOT_LOCK_REQUEST,          VPL_ERR_IO); //The requested resource could not be locked.
        WINRT_INET_ERR_MAP(INET_E_USE_EXTEND_BINDING,           VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_TERMINATED_BIND,              VPL_ERR_CONNABORT); //Binding was terminated.
        WINRT_INET_ERR_MAP(INET_E_BLOCKED_REDIRECT_XSECURITYID, VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_CODE_DOWNLOAD_DECLINED,       VPL_ERR_CANCELED); //The component download was declined by the user.
        WINRT_INET_ERR_MAP(INET_E_RESULT_DISPATCHED,            VPL_ERR_HTTP_ENGINE);
        WINRT_INET_ERR_MAP(INET_E_CANNOT_REPLACE_SFP_FILE,      VPL_ERR_IO); //Cannot replace a file that is protected by SFP.
        case E_ABORT:
            rv = VPL_ERR_CANCELED;
            VPL_REPORT_WARN("WinRT http2 error E_ABORT is mapped to VPL_ERR_CANCELED");
        break;
        default:
            rv = VPL_ERR_FAIL;
            VPL_REPORT_WARN("WinRT http2 error 0x%X is mapped to VPL_ERR_FAIL", err_code);
        break;
    }

    return rv;
}

static int VPLXlat_WinHttp_Err(DWORD err)
{
    int rv = VPL_OK;;
    rv = VPLError_XlatWinRTInetErrno(err);
    if (VPL_ERR_FAIL != rv) {
        return rv;
    }

    DWORD win32ErrCode = WIN32_FROM_HRESULT(err);
 
    if(0xFFFFFFFF != win32ErrCode){
        rv = VPLError_XlatWinErrno(win32ErrCode);
        if (rv != VPL_ERR_FAIL) {
            return rv;
        }
        rv = VPLError_XlatErrno(win32ErrCode);
    } else {
        VPLError_ReportUnknownErr(err, "VPL_ERR_HTTP_ENGINE");
        rv = VPL_ERR_HTTP_ENGINE;
    }
    return rv;
}

void CXMLHTTP2_Request2Callback::SetDebug(bool debug)
{
    debugOn = debug;
}

IFACEMETHODIMP CXMLHTTP2_Request2Callback::OnRedirect(IXMLHTTPRequest2 *pXHR, const wchar_t *pwszRedirectUrl) {
    return S_OK;
}

IFACEMETHODIMP CXMLHTTP2_Request2Callback::OnHeadersAvailable(IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const wchar_t *pwszStatus) {
    // get response code
    mHttpRespCode = (int)dwStatus;
    
    // try to get total length from http response header
    // ignore it if there is no Content-Length field in http response header
    wchar_t* wcvalue = NULL;
    char* value = NULL;
    // IXMLHTTPRequest2::GetResponseHeader promises to treat "Content-Length" as case-insensitive.
    HRESULT hr = pXHR->GetResponseHeader(L"Content-Length", &wcvalue);
    if (SUCCEEDED(hr)) {
        this->mTotalLen = _wcstoui64(wcvalue, NULL, 10);
        this->hasTotalLen = true;
        CoTaskMemFree(wcvalue);
    }
    else {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Error get content length from http response header = "FMT_HRESULT, hr);
    }

    return S_OK;
}

IFACEMETHODIMP CXMLHTTP2_Request2Callback::OnDataAvailable(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream) {
    // For application that needs to do a real-time chunk-by-chunk processing,
    // add the data reading in this callback. However the work must be done
    // as fast as possible, and must not block this thread, for example, waiting
    // on another event object.
    //
    HRESULT hr = S_OK;
    DWORD cbRead = 0;

    if (pResponseStream == NULL) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    while (true) {
        char buffer[MAX_BUFFER_LENGTH]={0};
        hr = pResponseStream->Read(buffer, MAX_BUFFER_LENGTH - 1, &cbRead);

        if (FAILED(hr) || cbRead == 0) {
            break;
        }
        // calculate
        mSofarLen += cbRead;

        if (debugOn && cbRead > 0) {
            VPL_LogHttpBuffer("receive", buffer, cbRead);
        }
        if (respBody != NULL) {
            // append to respBody if available
            respBody->append(buffer, cbRead);
        }
        if (hRespFile != NULL) {
            // write to file if file handle is available
            if (VPLFile_Write(hRespFile, buffer, cbRead) != cbRead) {
                mResult = VPL_ERR_FAIL;
                goto Exit;
            }
        }
        if (recvCb != NULL) {
            // call VPLHttp2_RecvCb if registered
            DWORD consumedBytes= ((VPLHttp2_RecvCb)recvCb)(intraHttp, recvCtx, buffer, cbRead);
            if (consumedBytes != cbRead) {
                mResult = VPL_ERR_IN_RECV_CALLBACK;
                hr = E_ABORT;

                VPL_LIB_LOG_INFO(VPL_SG_HTTP, 
                    "recvCb read only "FMTu32" of total "FMTu32" bytes, indicating to terminate the transfer."
                    ,consumedBytes, cbRead);
                goto Exit;
            }
        }
         
        if (recvProgCb != NULL) {
            // Report progress if registered VPLHttp2_ProgressCb
            ((VPLHttp2_ProgressCb)recvProgCb)(intraHttp, progCtx, mTotalLen, mSofarLen);
        }
    }

Exit:
    return hr;
}

IFACEMETHODIMP CXMLHTTP2_Request2Callback::OnResponseReceived(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream)
{
    // Data is all received
    if (debugOn) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "OnResponseReceived");
    }

    if (hRespFile != NULL) {
        VPLFile_Close(hRespFile);
        hRespFile = NULL;
    }
    mFinish.set();
    return S_OK;
}

IFACEMETHODIMP CXMLHTTP2_Request2Callback::OnError(IXMLHTTPRequest2 *pXHR, HRESULT hrError)
{
    mResult = VPLXlat_WinHttp_Err(hrError);
    // logs for http error
    if (debugOn) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error http action = %d", mResult);
    }
    if (hRespFile != NULL) {
        VPLFile_Close(hRespFile);
        hRespFile = NULL;
    }
    mFinish.set();
    return S_OK;
}

CXMLHTTP2_Request2Callback::CXMLHTTP2_Request2Callback()
{
    mResult = mHttpRespCode = 0;
    mTotalLen = mSofarLen = 0;
    hasTotalLen = false;
    
    this->respBody = NULL;
    this->hRespFile = NULL;
    this->recvCb = NULL;
    this->recvProgCb = NULL;
    this->intraHttp = NULL;
    this->recvCtx = NULL;
    this->progCtx = NULL;

    mFinish.reset();
}
CXMLHTTP2_Request2Callback::~CXMLHTTP2_Request2Callback()
{
    WaitUntilComplete();
    
    Reset();
}

int CXMLHTTP2_Request2Callback::WaitUntilComplete()
{
    mFinish.wait();

    return mResult;
}

int CXMLHTTP2_Request2Callback::GetResponseCode()
{
    return mHttpRespCode;
}

ULONGLONG CXMLHTTP2_Request2Callback::GetRespContentLen()
{
    return mTotalLen;
}

bool CXMLHTTP2_Request2Callback::HasRespContentLen()
{
    return hasTotalLen;
}

ULONGLONG CXMLHTTP2_Request2Callback::GetRecvContentLen()
{
    return mSofarLen;
}

void CXMLHTTP2_Request2Callback::Reset()
{
    mResult = mHttpRespCode = 0;
    mTotalLen = mSofarLen = 0;
    hasTotalLen = false;

    if (hRespFile != NULL) {
        VPLFile_Close(hRespFile);
        hRespFile = NULL;
    }
    // Cleanup given pointers
    this->respBody = NULL;
    this->recvCb = NULL;
    this->recvProgCb = NULL;
    this->intraHttp = NULL;
    this->recvCtx = NULL;
    this->progCtx = NULL;
}

void CXMLHTTP2_Request2Callback::RegisterCallbacks(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb progCb, void *progCtx, VPLHttp2 *intraHttp)
{
    this->recvCb = recvCb;
    this->recvProgCb = progCb;
    this->intraHttp = intraHttp;
    this->recvCtx = recvCtx;
    this->progCtx = progCtx;
}

int CXMLHTTP2_Request2Callback::SetRespBody(std::string &body)
{
    int rv = VPL_OK;

    if (&body == NULL)
        rv = VPL_ERR_INVALID;
    else
        respBody = &body;

    return rv;
}

int CXMLHTTP2_Request2Callback::SetRespFilePath(const std::string &filepath)
{
    int rv = VPL_OK;

    if (hRespFile != NULL) {
        VPLFile_Close(hRespFile);
        hRespFile = NULL;
    }
    
    hRespFile = VPLFile_Open(filepath.c_str(), 
                             VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_READWRITE,
                             0777);
    if (!VPLFile_IsValidHandle(hRespFile))
        rv = hRespFile;

    return rv;
}

CXMLHttp2_RequestPostStream::CXMLHttp2_RequestPostStream() 
{ 
    total = sofar = 0;
    m_hFile = VPLFILE_INVALID_HANDLE; 

    sendCb = NULL;
    sendCtx = NULL;
    sendProgCb = NULL;
    sendProgCtx = NULL;
    intraHttp = NULL;
}
 
CXMLHttp2_RequestPostStream::~CXMLHttp2_RequestPostStream() 
{ 
    total = sofar = 0;
    if (m_hFile != VPLFILE_INVALID_HANDLE) { 
        VPLFile_Close(m_hFile);
        m_hFile = VPLFILE_INVALID_HANDLE; 
    }

    sendCb = NULL;
    sendCtx = NULL;
    sendProgCb = NULL;
    sendProgCtx = NULL;
    intraHttp = NULL;
} 

void CXMLHttp2_RequestPostStream::SetDebug(bool debug)
{
    debugOn = debug;
}

void CXMLHttp2_RequestPostStream::CloseFile()
{
    if (m_hFile != VPLFILE_INVALID_HANDLE) {
        VPLFile_Close(m_hFile);
        m_hFile = VPLFILE_INVALID_HANDLE;
    }
}

STDMETHODIMP 
CXMLHttp2_RequestPostStream::Open( 
    _In_opt_ PCWSTR pcwszFileName,
    _In_opt_ VPLHttp2_ProgressCb progCb,
    _In_opt_ void *progCtx,
    _In_opt_ VPLHttp2 *intraHttp) 
{ 
    HRESULT hr = S_OK;
    VPLFile_handle_t hFile = VPLFILE_INVALID_HANDLE;

    if (pcwszFileName == NULL || *pcwszFileName == L'\0') {
        hr = E_INVALIDARG;
        goto out;
    }

    this->sendProgCb = progCb;
    this->sendProgCtx = progCtx;
    this->intraHttp = intraHttp;

    PSTR pstrFileName;
    _VPL__wstring_to_utf8_alloc(pcwszFileName, &pstrFileName);
    hFile = VPLFile_Open(pstrFileName, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(hFile)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Fail to open source file %s", pstrFileName);
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto out;
    }

    // Get file size
    ssize_t fileSz = (ssize_t) VPLFile_Seek(hFile, 0, VPLFILE_SEEK_END);
    if (fileSz < 0) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLFile_Seek failed: %d", (int)fileSz);
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto out;
    }
    VPLFile_Seek(hFile, 0, VPLFILE_SEEK_SET); // Reset

    total = fileSz;
    sofar = 0;
    m_hFile = hFile; 
    hFile = VPLFILE_INVALID_HANDLE;
    
out: 
    if (VPLFile_IsValidHandle(hFile)) {
        VPLFile_Close(hFile);
        hFile = VPLFILE_INVALID_HANDLE;
    }
    free(pstrFileName);
    return hr;
}

STDMETHODIMP 
CXMLHttp2_RequestPostStream::Open( 
    _In_opt_ VPLHttp2_SendCb sendCb,
    _In_opt_ u64 sendSize,
    _In_opt_ void *sendCtx,
    _In_opt_ VPLHttp2_ProgressCb progCb,
    _In_opt_ void *progCtx,
    _In_opt_ VPLHttp2 *intraHttp)
{
    HRESULT hr = S_OK;

    if (sendCb == NULL) {
        hr = E_INVALIDARG;
        goto out;
    }
    this->sendCb = sendCb;
    this->sendCtx = sendCtx;
    this->sendProgCb = progCb;
    this->sendProgCtx = progCtx;
    this->intraHttp = intraHttp;

    total = sendSize;
    sofar = 0;
out:
    return hr;
}

STDMETHODIMP 
CXMLHttp2_RequestPostStream::Read(
    _Out_writes_bytes_to_(cb, *pcbRead)  void *pv,
    ULONG cb,
    _Out_opt_  ULONG *pcbRead)
{
    HRESULT hr = S_OK;
    DWORD dwError = ERROR_SUCCESS;
    ssize_t cbRead = 0;

    if (pcbRead != NULL)
        *pcbRead = 0;

    if (pv == NULL || cb == 0) {
        hr = E_INVALIDARG;
        goto out;
    }

    if (VPLFile_IsValidHandle(m_hFile)) {
        // read contents from file to send
        cbRead = VPLFile_Read(m_hFile, pv, cb);
        if(cbRead < 0) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Fail to read file");
            hr = S_FALSE;
            goto out;
        }
        if (debugOn && cbRead > 0) {
            VPL_LogHttpBuffer("send (from file)", pv, cbRead);
        }
    }    
    if (sendCb != NULL) {
        // retrieve contents to send by VPLHttp2_SendCb if registered
        cbRead = ((VPLHttp2_SendCb)sendCb)(intraHttp, sendCtx, (char*)pv, cb);
        if(cbRead < 0) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2_SendCb returned "FMT_ssize_t, cbRead);
            hr = S_FALSE;
            goto out;
        }
        else if (cbRead == 0) {
            hr = S_OK;
        }
        if (debugOn && cbRead > 0) {
            VPL_LogHttpBuffer("send (from callback)", pv, cbRead);
        }
    }

    if (pcbRead != NULL)
        *pcbRead = cbRead;
    
    sofar += cbRead;

    if (sendProgCb != NULL) {
        // Report progress if registered VPLHttp2_ProgressCb
        ((VPLHttp2_ProgressCb)sendProgCb)(intraHttp, sendProgCtx, total, sofar);
    }
out: 
    return hr; 
} 

STDMETHODIMP 
CXMLHttp2_RequestPostStream::Write( 
    _In_reads_bytes_(cb)  const void *pv, 
    ULONG cb, 
    _Out_opt_  ULONG *pcbWritten 
) 
{ 
    UNREFERENCED_PARAMETER(pv); 
    UNREFERENCED_PARAMETER(cb); 
    UNREFERENCED_PARAMETER(pcbWritten); 
    return STG_E_ACCESSDENIED; 
} 

STDMETHODIMP
CXMLHttp2_RequestPostStream::GetSize(
    _Out_ ULONGLONG *pullSize
)
{
    HRESULT hr = S_OK;

    if (pullSize == NULL) {
        hr = E_INVALIDARG;
        goto out;
    }

    *pullSize = total;

out:
    return hr;
}

VPLHttp2__Impl::VPLHttp2__Impl(VPLHttp2 *intrHttp) :
        intrHttp(intrHttp),
        wcsUri(NULL),
        httpTimeout(VPLTime_FromSec(0)),
        isCancel(false),
        bTimeout(false),
        debugOn(false)
{
    VPLMutex_Init(&sessionMutex);
}

VPLHttp2__Impl::~VPLHttp2__Impl()
{
    // cleanup sessionHandle and callbacks
    cleanup();

    if (wcsUri != NULL) {
        free(wcsUri);
        wcsUri = NULL;
    }

    wcsHeaders.clear(); 
}

int VPLHttp2__Impl::init()
{
    int rv = VPL_OK;
    try {
        HRESULT hr;
        hr = CoCreateInstance(CLSID_FreeThreadedXMLHTTP60,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&sessionHandle));
        if (hr != S_OK) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error open COM = %d", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto end;
        }
        //Create and initialize IXMLHTTPRequest2Callback
        hr = MakeAndInitialize<CXMLHTTP2_Request2Callback>(&spMyXhrCallback);
        if (hr != S_OK) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error initialize CXMLHTTP2_Request2Callback = %d", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto end;
        }
        hr = spMyXhrCallback.As(&spXhrCallback);
        if (hr != S_OK) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error assign IXMLHTTPRequest2Callback = %d", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto end;
        }
        // Set debug log after spMyXhrCallback is initialized
        spMyXhrCallback->SetDebug(this->debugOn);
    }
    catch (Exception^ ex) {
        char* errMsg;
        _VPL__wstring_to_utf8_alloc(ex->ToString()->Data(), &errMsg);
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                        "Error init state = %s", 
                        errMsg);
        rv = VPLXlat_WinHttp_Err(ex->HResult);
        free(errMsg);
    }
end:
    return rv;
}

void VPLHttp2__Impl::cleanup()
{
    if (sessionHandle != NULL)
        sessionHandle.Detach();
    if (spMyXhrCallback != NULL)
        spMyXhrCallback.Detach();
    if (spXhrCallback != NULL)
        spXhrCallback.Detach();
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
    debugOn = debug;
    return rv;
}

int VPLHttp2__Impl::SetTimeout(VPLTime_t timeout)
{
    HRESULT hr = 0;
    int rv = VPL_OK;

    bTimeout = true;
    httpTimeout = timeout;

    return rv;
}

int VPLHttp2__Impl::setMethod(wchar_t *wcsMethod)
{
    MutexAutoLock lock(&sessionMutex);
    int rv = VPL_OK;
    HRESULT hr = 0;
    VPLUtil_TimeOrderedMap<std::wstring, std::wstring>::iterator it;

    if (wcsMethod == NULL || wcsUri == NULL) {
        return VPL_ERR_INVALID;
    }
    if (isCancel) {
        return VPL_ERR_CANCELED;
    }

    if (debugOn) {
        char *uVerb = NULL;
        char *uUri = NULL;
        int rc = VPL_OK;
        ON_BLOCK_EXIT(free, uVerb);
        ON_BLOCK_EXIT(free, uUri);

        rc = _VPL__wstring_to_utf8_alloc(wcsMethod, &uVerb);
        if (rc != VPL_OK || uVerb == NULL) {
            VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "Failed to convert http method wstring to utf-8: %d", rc);
        } 

        rc = _VPL__wstring_to_utf8_alloc(wcsUri, &uUri);
        if (rc != VPL_OK || uUri == NULL) {
            VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "Failed to convert request URI wstring to utf-8: %d", rc);
        } 

        if(uVerb != NULL && uUri != NULL){
            VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "HTTP Request: %s %s", uVerb, uUri);
        }
    }

    // cleanup prev sessionHandle and callbacks
    cleanup();

    // init sessionHandle and callbacks
    rv = this->init();
    if (rv < 0) {
        goto out;
    }

    // open http request
    hr = sessionHandle->Open(wcsMethod,      // Method.
                             wcsUri,         // Url.
                             spXhrCallback.Get(),    // Callback.
                             NULL,          // Username.
                             NULL,          // Password.
                             NULL,          // Proxy username.
                             NULL);         // Proxy password.
    if (FAILED(hr)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Open", hr);
        rv = VPLXlat_WinHttp_Err(hr);
        goto out;
    }

    for (it = wcsHeaders.begin(); it != wcsHeaders.end(); it++) {
        // NOTE: caller must free wcHeader after use.
        hr = sessionHandle->SetRequestHeader(it->first.c_str(), it->second.c_str()); 
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "SetRequestHeader", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }
    }

    // set HTTP request timeout
    if (bTimeout) {
        hr = sessionHandle->SetProperty(XHR_PROP_TIMEOUT, VPLTime_ToMillisec(httpTimeout));
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "SetProperty Timeout", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }
    }
out:
    return rv;
}

int VPLHttp2__Impl::SetUri(const std::string &uri)
{
    HRESULT hr = 0;
    int rv = VPL_OK;

    rv = _VPL__utf8_to_wstring(uri.c_str(), &wcsUri);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert url (%s) from UTF8 to UTF16: %d", uri.c_str(), rv);
        rv = VPLXlat_WinHttp_Err(hr);
        goto out;
    }
out:
    return rv;
}
int VPLHttp2__Impl::AddRequestHeader(const std::string &field, const std::string &value)
{
    LPWSTR wcField = NULL, wcValue = NULL;
    int rv = VPL_OK;
    HRESULT hr = 0;

    // skip empty headers
    if (field.empty())
        return VPL_ERR_INVALID;

    // We have to convert field and value to UTF16
    rv = _VPL__utf8_to_wstring(field.c_str(), &wcField);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert field (%s) from UTF8 to UTF16: %d", field.c_str(), rv);
        rv = VPLXlat_WinHttp_Err(hr);
        goto out;
    }
    rv = _VPL__utf8_to_wstring(value.c_str(), &wcValue);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert value (%s) from UTF8 to UTF16: %d", value.c_str(), rv);
        rv = VPLXlat_WinHttp_Err(hr);
        goto out;
    }
    
    wcsHeaders.insert(std::pair<std::wstring, std::wstring>(std::wstring(wcField), std::wstring(wcValue)));

out:
    if (wcField != NULL)
        free(wcField);
    if (wcValue != NULL)
        free(wcValue);
    return rv;
}

int VPLHttp2__Impl::Get(std::string &respBody)
{
    int rv = VPL_OK;

    rv = setMethod(L"GET");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }

    try {
        // an issue of IE engine.
        // If we don't add HTTP header "If-Modified-Since: <past date>", 
        // IXMLHttpRequest2 will response content in the cache.
        // Already tried "Cache-Control: no-cache" and "Pragma: no-cache" and doesn't work
        HRESULT hr = sessionHandle->SetRequestHeader(L"If-Modified-Since", L"Sat, 1 Jan 2000 00:00:00 GMT" ); 
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "SetRequestHeader", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }

        CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, NULL, 0, hr);
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
            rv = VPLXlat_WinHttp_Err(hr);
        }
        else
            rv = WaitFinish();
    }
    catch(Exception^ ex){
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
        rv = VPLXlat_WinHttp_Err(ex->HResult);
    }

out:
    return rv;
}

int VPLHttp2__Impl::Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    int rv = VPL_OK;

    rv = setMethod(L"GET");
    if (rv != VPL_OK) {
        goto out;
    }
    spMyXhrCallback->RegisterCallbacks(NULL, NULL, recvProgCb, recvProgCtx, (VPLHttp2*)this);
    rv = spMyXhrCallback->SetRespFilePath(respBodyFilePath);
    if (rv != VPL_OK) {
        goto out;
    }

    try {
        // an issue of IE engine.
        // If we don't add HTTP header "If-Modified-Since: <past date>", 
        // IXMLHttpRequest2 will response content in the cache.
        // Already tried "Cache-Control: no-cache" and "Pragma: no-cache" and doesn't work
        HRESULT hr = sessionHandle->SetRequestHeader(L"If-Modified-Since", L"Sat, 1 Jan 2000 00:00:00 GMT" ); 
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "SetRequestHeader", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }

        CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, NULL, 0, hr);
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
            rv = VPLXlat_WinHttp_Err(hr);
        }
        else
            rv = WaitFinish();
    }
    catch(Exception^ ex){
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
        rv = VPLXlat_WinHttp_Err(ex->HResult);
    }

out:
    return rv;
}

int VPLHttp2__Impl::Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    int rv = VPL_OK;

    rv = setMethod(L"GET");
    if (rv != VPL_OK) {
        goto out;
    }

    spMyXhrCallback->RegisterCallbacks(recvCb, recvCtx, recvProgCb, recvProgCtx, (VPLHttp2*)this);

    try {
        // an issue of IE engine.
        // If we don't add HTTP header "If-Modified-Since: <past date>", 
        // IXMLHttpRequest2 will response content in the cache.
        // Already tried "Cache-Control: no-cache" and "Pragma: no-cache" and doesn't work
        HRESULT hr = sessionHandle->SetRequestHeader(L"If-Modified-Since", L"Sat, 1 Jan 2000 00:00:00 GMT" ); 
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "SetRequestHeader", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }

        CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, NULL, 0, hr);
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
            rv = VPLXlat_WinHttp_Err(hr);
        }
        else
            rv = WaitFinish();
    }
    catch(Exception^ ex){
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
        rv = VPLXlat_WinHttp_Err(ex->HResult);
    }

out:
    return rv;
}

int VPLHttp2__Impl::Put(const std::string &reqBody, std::string &respBody)
{
    int rv = VPL_OK;

    rv = setMethod(L"PUT");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }
    {
        // compose output buff for POST/PUT
        ComPtr<IStream> postStream;
        ULONG bytesWritten = 0;
        HRESULT hr = CreateStreamOnHGlobal(nullptr,
                                   true,
                                   &postStream);
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "CreateStreamOnHGlobal", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }
        if (reqBody.size() > 0) {
            if (debugOn) {
                VPL_LogHttpBuffer("send", reqBody.c_str(), reqBody.size());
            }
            hr = postStream->Write((void*)reqBody.c_str(),
                                   reqBody.size(),
                                   &bytesWritten);
            if (FAILED(hr)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Write", hr);
                rv = VPLXlat_WinHttp_Err(hr);
                goto out;
            }
            else if (bytesWritten == 0) {
                rv = VPL_ERR_FAIL;
                goto out;
            }
            try {
                CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, postStream.Get(), bytesWritten, hr);
                if (FAILED(hr)) {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                    rv = VPLXlat_WinHttp_Err(hr);
                }
                else
                    rv = WaitFinish();
            }
            catch(Exception^ ex){
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
                rv = VPLXlat_WinHttp_Err(ex->HResult);
            }
        }
        else {
            try {
                CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, NULL, 0, hr);
                if (FAILED(hr)) {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                    rv = VPLXlat_WinHttp_Err(hr);
                }
                else
                    rv = WaitFinish();
            }
            catch(Exception^ ex){
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
                rv = VPLXlat_WinHttp_Err(ex->HResult);
            }
        }
    }
out:
    return rv;
}
int VPLHttp2__Impl::Put(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    int rv = VPL_OK;

    rv = setMethod(L"PUT");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }
    {
        ULONGLONG ullFileSize = 0;
        wchar_t *pcwszFileName;

        ComPtr<CXMLHttp2_RequestPostStream> spMyXhrPostStream = Make<CXMLHttp2_RequestPostStream>();
        ComPtr<ISequentialStream> spXhrPostStream;
        HRESULT hr = spMyXhrPostStream.As(&spXhrPostStream);
        if (FAILED(hr)) {
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }

        spMyXhrPostStream->SetDebug(debugOn);
        _VPL__utf8_to_wstring(reqBodyFilePath.c_str(), &pcwszFileName);
        spMyXhrPostStream->Open(pcwszFileName, sendProgCb, sendProgCtx, (VPLHttp2*)this);
        spMyXhrPostStream->GetSize(&ullFileSize);

        try {
            CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, spXhrPostStream.Get(), ullFileSize, hr);
            if (FAILED(hr)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                rv = VPLXlat_WinHttp_Err(hr);
            }
            else
                rv = WaitFinish();
        }
        catch(Exception^ ex){
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
            rv = VPLXlat_WinHttp_Err(ex->HResult);
        }

        spMyXhrPostStream->CloseFile();
    }
out:
    return rv;
}
int VPLHttp2__Impl::Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    int rv = VPL_OK;

    rv = setMethod(L"PUT");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }
    {
        ULONGLONG ullFileSize = 0;

        ComPtr<CXMLHttp2_RequestPostStream> spMyXhrPostStream = Make<CXMLHttp2_RequestPostStream>();
        ComPtr<ISequentialStream> spXhrPostStream;
        HRESULT hr = spMyXhrPostStream.As(&spXhrPostStream);
        if (FAILED(hr)) {
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }

        spMyXhrPostStream->SetDebug(debugOn);
        spMyXhrPostStream->Open(sendCb, sendSize, sendCtx, sendProgCb, sendProgCtx, (VPLHttp2*)this);
        spMyXhrPostStream->GetSize(&ullFileSize);

        try {
            CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, spXhrPostStream.Get(), ullFileSize, hr);
            if (FAILED(hr)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                rv = VPLXlat_WinHttp_Err(hr);
            }
            else
                rv = WaitFinish();
        }
        catch(Exception^ ex){
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
            rv = VPLXlat_WinHttp_Err(ex->HResult);
        }
    }
out:
    return rv;
}
int VPLHttp2__Impl::Post(const std::string &reqBody, std::string &respBody)
{
    int rv = VPL_OK;

    rv = setMethod(L"POST");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }
    {
        // compose output buff for POST/PUT
        ComPtr<IStream> postStream;
        ULONG bytesWritten = 0;
        HRESULT hr = CreateStreamOnHGlobal(nullptr,
                                   true,
                                   &postStream);
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "CreateStreamOnHGlobal", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }
        if (reqBody.size() > 0) {
            if (debugOn) {
                VPL_LogHttpBuffer("send", reqBody.c_str(), reqBody.size());
            }
            hr = postStream->Write((void*)reqBody.c_str(),
                                   reqBody.size(),
                                   &bytesWritten);
            if (FAILED(hr)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Write", hr);
                rv = VPLXlat_WinHttp_Err(hr);
                goto out;
            }
            else if (bytesWritten == 0) {
                rv = VPL_ERR_FAIL;
                goto out;
            }
            try {
                CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, postStream.Get(), bytesWritten, hr);
                if (FAILED(hr)) {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                    rv = VPLXlat_WinHttp_Err(hr);
                }
                else
                    rv = WaitFinish();
            }
            catch(Exception^ ex){
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
                rv = VPLXlat_WinHttp_Err(ex->HResult);
            }
        }
        else {
            try {
                CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, NULL, 0, hr);
                if (FAILED(hr)) {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                    rv = VPLXlat_WinHttp_Err(hr);
                }
                else
                    rv = WaitFinish();
            }
            catch(Exception^ ex){
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
                rv = VPLXlat_WinHttp_Err(ex->HResult);
            }
        }
    }
out:
    return rv;
}
int VPLHttp2__Impl::Post(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    int rv = VPL_OK;

    rv = setMethod(L"POST");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }
    {
        ULONGLONG ullFileSize = 0;
        wchar_t *pcwszFileName;

        ComPtr<CXMLHttp2_RequestPostStream> spMyXhrPostStream = Make<CXMLHttp2_RequestPostStream>();
        ComPtr<ISequentialStream> spXhrPostStream;
        HRESULT hr = spMyXhrPostStream.As(&spXhrPostStream);
        if (FAILED(hr)) {
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }

        spMyXhrPostStream->SetDebug(debugOn);
        _VPL__utf8_to_wstring(reqBodyFilePath.c_str(), &pcwszFileName);
        spMyXhrPostStream->Open(pcwszFileName, sendProgCb, sendProgCtx, (VPLHttp2*)this);
        spMyXhrPostStream->GetSize(&ullFileSize);

        try {
            CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, spXhrPostStream.Get(), ullFileSize, hr);
            if (FAILED(hr)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                rv = VPLXlat_WinHttp_Err(hr);
            }
            else
                rv = WaitFinish();
        }
        catch(Exception^ ex){
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
            rv = VPLXlat_WinHttp_Err(ex->HResult);
        }

        spMyXhrPostStream->CloseFile();
    }
out:
    return rv;
}
int VPLHttp2__Impl::Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    int rv = VPL_OK;

    rv = setMethod(L"POST");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }
    {
        ULONGLONG ullFileSize = 0;

        ComPtr<CXMLHttp2_RequestPostStream> spMyXhrPostStream = Make<CXMLHttp2_RequestPostStream>();
        ComPtr<ISequentialStream> spXhrPostStream;
        HRESULT hr = spMyXhrPostStream.As(&spXhrPostStream);
        if (FAILED(hr)) {
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }
        
        spMyXhrPostStream->SetDebug(debugOn);
        spMyXhrPostStream->Open(sendCb, sendSize, sendCtx, sendProgCb, sendProgCtx, (VPLHttp2*)this);
        spMyXhrPostStream->GetSize(&ullFileSize);

        try {
            CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, spXhrPostStream.Get(), ullFileSize, hr);
            if (FAILED(hr)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                rv = VPLXlat_WinHttp_Err(hr);
            }
            else
                rv = WaitFinish();
        }
        catch(Exception^ ex){
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
            rv = VPLXlat_WinHttp_Err(ex->HResult);
        }
    }
out:
    return rv;
}
int VPLHttp2__Impl::Post(const std::string &reqBody, VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    int rv = VPL_OK;

    rv = setMethod(L"POST");
    if (rv != VPL_OK) {
        goto out;
    }

    spMyXhrCallback->RegisterCallbacks(recvCb, recvCtx, recvProgCb, recvProgCtx, (VPLHttp2*)this);

    {
        // compose output buff for POST/PUT
        ComPtr<IStream> postStream;
        ULONG bytesWritten = 0;
        HRESULT hr = CreateStreamOnHGlobal(nullptr,
                                   true,
                                   &postStream);
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "CreateStreamOnHGlobal", hr);
            rv = VPLXlat_WinHttp_Err(hr);
            goto out;
        }
        if (reqBody.size() > 0) {
            if (debugOn) {
                VPL_LogHttpBuffer("send", reqBody.c_str(), reqBody.size());
            }
            hr = postStream->Write((void*)reqBody.c_str(),
                                   reqBody.size(),
                                   &bytesWritten);
            if (FAILED(hr)) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Write", hr);
                rv = VPLXlat_WinHttp_Err(hr);
                goto out;
            }
            else if (bytesWritten == 0) {
                rv = VPL_ERR_FAIL;
                goto out;
            }
            try {
                CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, postStream.Get(), bytesWritten, hr);
                if (FAILED(hr)) {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                    rv = VPLXlat_WinHttp_Err(hr);
                }
                else
                    rv = WaitFinish();
            }
            catch(Exception^ ex){
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
                rv = VPLXlat_WinHttp_Err(ex->HResult);
            }
        }
        else {
            try {
                CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, NULL, 0, hr);
                if (FAILED(hr)) {
                    VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
                    rv = VPLXlat_WinHttp_Err(hr);
                }
                else
                    rv = WaitFinish();
            }
            catch(Exception^ ex){
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
                rv = VPLXlat_WinHttp_Err(ex->HResult);
            }
        }
    }
out:
    return rv;
}
int VPLHttp2__Impl::Delete(std::string& respBody)
{
    int rv = VPL_OK;
    rv = setMethod(L"DELETE");
    if (rv != VPL_OK) {
        goto out;
    }
    rv = spMyXhrCallback->SetRespBody(respBody);
    if (rv != VPL_OK) {
        goto out;
    }
    try {
        HRESULT hr = 0;
        CHECK_CANCEL(&(this->sessionMutex), this->isCancel, sessionHandle->Send, NULL, 0, hr);
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "%s failed: "FMT_DWORD, "Send", hr);
            rv = VPLXlat_WinHttp_Err(hr);
        }
        else
            rv = WaitFinish();
    }
    catch(Exception^ ex){
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Exception: 0x%X", ex->HResult);
        rv = VPLXlat_WinHttp_Err(ex->HResult);
    }
out:
    return rv;
}
int VPLHttp2__Impl::GetStatusCode()
{
    return spMyXhrCallback->GetResponseCode();
}
const std::string* VPLHttp2__Impl::FindResponseHeader(const std::string &field)
{
    HRESULT hr = S_OK;
    int rv = VPL_OK;
    wchar_t *wcfield = NULL, *wcvalue = NULL;
    char *value = NULL;

    stdRespHeader.clear();

     rv = _VPL__utf8_to_wstring(field.c_str(), &wcfield);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert http header (%s) from UTF8 to UTF16: %d", field.c_str(), rv);
        goto failed;
    }

    // IXMLHTTPRequest2::GetResponseHeader promises to treat wcfield as case-insensitive.
    hr = sessionHandle->GetResponseHeader(wcfield, &wcvalue);
    if (FAILED(hr)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error get http header = %d", hr);
        rv = VPLXlat_WinHttp_Err(hr);
        goto failed;
    }

    rv = _VPL__wstring_to_utf8_alloc(wcvalue, &value);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Failed to convert value from UTF16 to UTF8: %d", rv);
        goto failed;
    }
    
    stdRespHeader.assign(value);

out:
    return (const std::string*)&stdRespHeader;
failed:
    return NULL;
}
int VPLHttp2__Impl::Cancel()
{
    MutexAutoLock lock(&sessionMutex);
    int rv = VPL_OK;
    if (sessionHandle != NULL) {
        HRESULT hr = sessionHandle->Abort();
        if (FAILED(hr)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Abort failed: "FMT_HRESULT, hr);
            rv = VPLXlat_WinHttp_Err(hr);
        }
    }
    this->isCancel = true;
    return rv;
}

int VPLHttp2__Impl::WaitFinish()
{
    if(debugOn)
        VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "Wait for response...");
        
    int rv = spMyXhrCallback->WaitUntilComplete();
    int httpResponse = spMyXhrCallback->GetResponseCode();

    if(debugOn)
        VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "httpResponse: %d", httpResponse);

    // At minimum, check all 2xx response code
    if (httpResponse >= 200 && httpResponse < 300) {
        // if content-length is missing in the response header. skip the check
        if (spMyXhrCallback->HasRespContentLen()) {
            if (spMyXhrCallback->GetRespContentLen() != spMyXhrCallback->GetRecvContentLen()) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Transferred a partial file, expect="FMTu64", actual="FMTu64,
                                spMyXhrCallback->GetRespContentLen(), spMyXhrCallback->GetRecvContentLen());
                if (rv == 0)
                    rv = VPL_ERR_RESPONSE_TRUNCATED;
            }
        }
    }

    return rv;
}
