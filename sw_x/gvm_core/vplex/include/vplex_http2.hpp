#ifndef __VPLEX_HTTP2_HPP__
#define __VPLEX_HTTP2_HPP__

#if defined(WIN32)
// This is to suppress warning C4355: 'this' : used in base member initializer list.
// If we ever convert the VPLHttp2 API to the proper pimpl idiom or make it
// an abstract base class, we should be able to remove this workaround.
#pragma warning(disable: 4355)
#endif

#include <vplu_types.h>
#include <vpl_time.h>

#include "vplex_http2_cb.hpp"
#include "vplex__http2.hpp"
#include "vplex_assert.h"

#include <string>

///
/// HTTP Client.
///
/// IMPORTANT NOTE:
///   Each VPLHttp2 instance is only designed to perform one operation (Get, Post, Put, Delete).
///   Attempting to reuse the same VPLHttp2 instance for another operation is a programmer error
///   and results in undefined behavior.
///
/// @see http://www.ctbg.acer.com/wiki/index.php/VPLHttp2
///
class VPLHttp2 {
public:
    VPLHttp2() : impl(this), useCount(0) { }
    ~VPLHttp2() { }

    ///
    /// This should be called from a single thread before using any of the other
    /// VPLHttp2 functions.
    ///
    static int Init(void)
    {
        if (VPL_IsInit() == VPL_FALSE) {
            // VPLInit() is required for VPLHttp2
            return VPL_ERR_NOT_INIT;
        }
        return VPLHttp2__Impl::Init();
    };

    ///
    /// For a clean shutdown, this should be called by the main thread before exiting.
    /// It is safe to call this even if #Init() hasn't been called.
    ///
    static void Shutdown(void) { return VPLHttp2__Impl::Shutdown(); };

    /// Enable/disable traffic debug log (disabled by default).
    /// When enabled, all incoming and outgoing bytes will be logged (but sensitive information
    /// such as passwords and keys will be replaced with "*").
    /// @note On Android, when sending from a file (reqBodyFilePath), the outgoing body will not
    ///   currently be logged, even if this is enabled.
    int SetDebug(bool debug) { return impl.SetDebug(debug); }

    /// Intention is to set network inactivity timeout.
    /// However, implementation (depending on what's available) may decide to ignore it.
    int SetTimeout(VPLTime_t timeout) { return impl.SetTimeout(timeout); }

    /// Set whole URI line.
    /// Example: SetUri("https://127.0.0.1:12345/clouddoc/dir?index=0&max=10");
    /// Error is returned if "uri" is malformed.
    /// Passed string is used as is; that is, no reencoding of any kind.
    int SetUri(const std::string &uri) { return impl.SetUri(uri); }

    /// Add a request header.
    /// Example: AddRequestHeader("x-ac-userId", "12345");
    /// Order of headers is preserved.
    /// If AddRequestHeader() is called twice for the same field,
    /// the order is determined by the first call, but the value is determined by the second call.
    int AddRequestHeader(const std::string &field, const std::string &value) { return impl.AddRequestHeader(field, value); }


    // Methods to send a GET request.
    // Three functions, depending on destination for response body.
    // Function does not return until the transfer is finished or canceled.
    // Returns VPL_OK on successful transport.
    // Returns VPL_ERR_CANCELED if operation was canceled by Cancel().

    /// Response body goes into a std::string object (respBody).
    int Get(std::string &respBody) {
        useOnce();
        return impl.Get(respBody);
    }
    /// Response body goes to a file (respBodyFilePath).
    /// @param recvProgCb Can be NULL if progress reporting is not desired.
    int Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx) {
        useOnce();
        return impl.Get(respBodyFilePath, recvProgCb, recvProgCtx);
    }
    /// Response body goes to a callback function (recvCB) as it is received.
    /// @param recvCb Cannot be NULL.
    /// @param recvProgCb Can be NULL if progress reporting is not desired.
    int Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx) {
        useOnce();
        return impl.Get(recvCb, recvCtx, recvProgCb, recvProgCtx);
    }


    // Methods to send a PUT request.
    // Three functions, depending on source of request body.
    // Any response body goes into a std::string object (respBody).  (Expectation is that response body is small.)
    // Function does not return until the transfer is finished or canceled.
    // Returns VPL_OK on successful transport.
    // Returns VPL_ERR_CANCELED if operation was canceled by Cancel().

    /// Request body is in a std::string object (reqBody).
    int Put(const std::string &reqBody,
        std::string &respBody) {
        useOnce();
        return impl.Put(reqBody, respBody);
    }
    /// Request body is in a file (reqBodyFilePath).
    int Put(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        std::string &respBody) {
        useOnce();
        return impl.Put(reqBodyFilePath, sendProgCb, sendProgCtx, respBody);
    }
    /// Request body obtained incrementally by repeatedly calling a callback function (sendCb).
    int Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        std::string &respBody) {
        useOnce();
        return impl.Put(sendCb, sendCtx, sendSize, sendProgCb, sendProgCtx, respBody);
    }
#ifdef VPLHTTP2_FUTURE
    // send from std::string, recv in file
    int Put(const std::string &reqBody,
        const std::string *respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from file, recv in file
    int Put(const std:string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        const std::string *respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from callback, recv in file
    int Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        const std::string *respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from std::string, recv using callback
    int Put(const std::string &reqBody,
        VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from file, recv using callback
    int Put(const std:string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from callback, recv using callback
    int Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
        VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
#endif // VPLHTTP2_FUTURE


    // Methods to send a POST request.
    // Three functions, depending on source of request body.
    // Any response body goes into a std::string object (respBody).  (Expectation is that response body is small.)
    // Function does not return until the transfer is finished or canceled.
    // Returns VPL_OK on successful transport.
    // Returns VPL_ERR_CANCELED if operation was canceled by Cancel().

    /// Request body is in a std::string object (reqBody).
    int Post(const std::string &reqBody,
         std::string &respBody) {
        useOnce();
        return impl.Post(reqBody, respBody);
    }
    /// Request body is in a file (reqBodyFilePath).
    int Post(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
         std::string &respBody) {
        useOnce();
        return impl.Post(reqBodyFilePath, sendProgCb, sendProgCtx, respBody);
    }
    /// Request body obtained incrementally by repeatedly calling a callback function (sendCb).
    int Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
         std::string &respBody) {
        useOnce();
        return impl.Post(sendCb, sendCtx, sendSize, sendProgCb, sendProgCtx, respBody);
    }
    // send from std::string, recv using callback
    int Post(const std::string &reqBody,
            VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx) {
        useOnce();
        return impl.Post(reqBody, recvCb, recvCtx, recvProgCb, recvProgCtx);
    }
#ifdef VPLHTTP2_FUTURE
    // send from std::string, recv in file
    int Post(const std::string &reqBody,
             const std::string *respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from file, recv in file
    int Post(const std:string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             const std::string *respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from callback, recv in file
    int Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             const std::string *respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from std::string, recv using callback
    int Post(const std::string &reqBody,
             VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from file, recv using callback
    int Post(const std:string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    // send from callback, recv using callback
    int Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
#endif // VPLHTTP2_FUTURE


    /// Method to send a DELETE request.
    /// Function does not return until the transfer is finished or canceled.
    /// Returns VPL_OK on successful transport.
    /// Returns VPL_ERR_CANCELED if operation was canceled by Cancel().
    int Delete(std::string &respBody) {
        useOnce();
        return impl.Delete(respBody);
    }


    /// Get HTTP status code.
    /// This value is guaranteed to be valid when Get()/Put()/Post()/Delete() returns success
    /// and (if applicable) before the first #VPLHttp2_RecvCb callback is invoked.
    int GetStatusCode() { return impl.GetStatusCode(); }

    /// Find response header value.
    /// This value is guaranteed to be valid before before Get()/Put()/Post()/Delete() returns
    /// and (if applicable) before the first #VPLHttp2_RecvCb callback is invoked.
    /// Note, field names are case insensitive.
    /// Return NULL if not present.
    /// Example: FindResponseHeaderValue("Content-Length") -> "345"
    const std::string *FindResponseHeader(const std::string &field) { return impl.FindResponseHeader(field); }

    /// Abort data transfer.
    /// It is acceptable for this function to be called anytime while the object is valid.
    /// In particular, if it is called before any data transfer has started, no data transfer should happen.
    int Cancel() { return impl.Cancel(); }


private:
    VPLHttp2__Impl impl;
    int useCount;

    void useOnce() {
        if (useCount != 0) {
            FAILED_ASSERT("vplex_http2 handle used more than once:%d", useCount);
        }
        useCount++;
    }
};

#endif // include guard
