#ifndef __VPLEX__HTTP2_HPP__
#define __VPLEX__HTTP2_HPP__

#include <vplu_types.h>
#include <vpl_time.h>

#include "vplex_http2_cb.hpp"
#include "vplex_file.h"
#include "vplex_util.hpp"
#include "vplu_mutex_autolock.hpp"

#include <string>

#include <ppltasks.h>
#include <wrl.h>
#include <wrl\client.h>
#include <wrl\implements.h>
#include <MsXml6.h>

using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Details;

// Implementation of IXMLHTTPRequest2Callback for VPLHttp2
class CXMLHTTP2_Request2Callback : public Microsoft::WRL::RuntimeClass<
    RuntimeClassFlags<RuntimeClassType::ClassicCom>, IXMLHTTPRequest2Callback>
{
public:
    CXMLHTTP2_Request2Callback();
    ~CXMLHTTP2_Request2Callback();

    void SetDebug(bool debug);
    int WaitUntilComplete();
    int GetResponseCode();
    ULONGLONG GetRespContentLen();
    bool HasRespContentLen();
    ULONGLONG GetRecvContentLen();
    void Reset();
    void RegisterCallbacks(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb progCb, void *progCtx, VPLHttp2 *intraHttp);
    int SetRespBody(std::string &body);
    int SetRespFilePath(const std::string &filepath);

    IFACEMETHODIMP OnRedirect(IXMLHTTPRequest2 *pXHR, const wchar_t *pwszRedirectUrl);
    IFACEMETHODIMP OnHeadersAvailable(IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const wchar_t *pwszStatus);
    IFACEMETHODIMP OnDataAvailable(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream);
    IFACEMETHODIMP OnResponseReceived(IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream);
    IFACEMETHODIMP OnError(IXMLHTTPRequest2 *pXHR, HRESULT hrError);

private:
    int mHttpRespCode, mResult;
    ULONGLONG mSofarLen, mTotalLen;
    bool hasTotalLen;
    Concurrency::event mFinish;

    std::string *respBody;
    VPLFile_handle_t hRespFile;
    VPLHttp2_RecvCb recvCb;
    VPLHttp2_ProgressCb recvProgCb;
    void *recvCtx, *progCtx;
    VPLHttp2* intraHttp;
    bool debugOn;
};

// Implementation of ISequentialStream for VPLHttp2
class CXMLHttp2_RequestPostStream : 
    public Microsoft::WRL::RuntimeClass<RuntimeClassFlags<ClassicCom>, ISequentialStream>
{
private: 
    CXMLHttp2_RequestPostStream(); 
    ~CXMLHttp2_RequestPostStream();
    friend Microsoft::WRL::ComPtr<CXMLHttp2_RequestPostStream> Microsoft::WRL::Details::Make<CXMLHttp2_RequestPostStream>();

public:
    STDMETHODIMP 
    Open( 
        _In_opt_ PCWSTR pcwszFileName,
        _In_opt_ VPLHttp2_ProgressCb progCb,
        _In_opt_ void *progCtx,
        _In_opt_ VPLHttp2 *intraHttp
    );

    void CloseFile();

    STDMETHODIMP 
    Open( 
        _In_opt_ VPLHttp2_SendCb sendCb,
        _In_opt_ u64 sendSize,
        _In_opt_ void *sendCtx,
        _In_opt_ VPLHttp2_ProgressCb progCb,
        _In_opt_ void *progCtx,
        _In_opt_ VPLHttp2 *intraHttp
    );

    STDMETHODIMP 
    Read( 
        _Out_writes_bytes_to_(cb, *pcbRead)  void *pv, 
        ULONG cb, 
        _Out_opt_  ULONG *pcbRead 
    );

    STDMETHODIMP 
    Write( 
        _In_reads_bytes_(cb)  const void *pv, 
        ULONG cb, 
        _Out_opt_  ULONG *pcbWritten 
    );

    STDMETHODIMP 
    GetSize( 
        _Out_ ULONGLONG *pullSize 
    );

    void SetDebug(bool debug);

private:
    ULONGLONG total, sofar;
    VPLFile_handle_t m_hFile;
    VPLHttp2_SendCb sendCb;
    VPLHttp2_ProgressCb sendProgCb;
    void *sendCtx, *sendProgCtx;
    VPLHttp2* intraHttp;
    bool debugOn;
};

class VPLHttp2;

class VPLHttp2__Impl {
public:
    VPLHttp2__Impl(VPLHttp2 *intrHttp);
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
    // Request body is in a std::string object (reqBody).
    // Response body goes to a callback function (recvCb) as it is received.
    int Post(const std::string &reqBody, VPLHttp2_RecvCb recvCb, void *recvCtx, 
         VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);


    // Method to send a DELETE request.
    // Bug 7160: Implement return of response body.
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

private:
    int init();
    void cleanup();
    int setMethod(wchar_t *wcsMethod);
    int WaitFinish();

private:
    VPLHttp2 *intrHttp;
    VPLMutex_t sessionMutex;

    ComPtr<IXMLHTTPRequest2> sessionHandle;
    ComPtr<CXMLHTTP2_Request2Callback> spMyXhrCallback;
    ComPtr<IXMLHTTPRequest2Callback> spXhrCallback;

    VPLUtil_TimeOrderedMap<std::wstring, std::wstring> wcsHeaders;
    wchar_t *wcsUri;
    std::string stdRespHeader;
    VPLTime_t httpTimeout;
    bool isCancel;
    bool bTimeout;
    bool debugOn;
};


#endif // __VPLEX__HTTP2_HPP__

