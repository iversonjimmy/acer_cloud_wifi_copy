#ifndef __VPLEX__HTTP2_HPP__
#define __VPLEX__HTTP2_HPP__

#include <vpl_th.h>
#include <vpl_time.h>
#include <vplu_types.h>

#include <vplex_file.h>
#include "vplex_http2_cb.hpp"
#include "vplex_util.hpp"

#include <string>
#include <map>

class VPLHttp2;
class VPLHttp2__Impl;

#ifdef ANDROID
#include <jni.h>
extern "C" {
    JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCSendCallback(
            JNIEnv * env,
            jclass clazz,
            jlong jSendCbPtr,
            jlong jHttp2ImplPtr,
            jbyteArray jBuffer);
    JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCRecvCallback(
            JNIEnv * env,
            jclass clazz,
            jlong jRecvCbPtr,
            jlong jHttp2ImplPtr,
            jbyteArray jBuffer,
            jlong jBytesInBuffer);
    JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCResponseStatusCallback(
            JNIEnv* env,
            jclass clazz,
            jlong jHttp2ImplPtr,
            int status);
    JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCResponseHeadersCallback(
            JNIEnv* env,
            jclass clazz,
            jlong jHttp2ImplPtr,
            jbyteArray jBuffer);
    JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCProgressCallback(
            JNIEnv* env,
            jclass clazz,
            jlong jHttp2ImplPtr,
            jlong dltotal,
            jlong dlnow,
            jlong ultotal,
            jlong ulnow);
} // extern "C"
/// @return number of bytes processed, or negative for error.  It is also considered an
///     error if this is not exactly equal to size.
typedef int (*VPLHttp_InternalRecvCallback_t)(VPLHttp2__Impl& http, const void* buf, size_t size);
/// @return number of bytes processed, or negative for error.
typedef int (*VPLHttp_InternalSendCallback_t)(VPLHttp2__Impl& http, void* buf, size_t size);
#else
#include <curl/curl.h>
#include <curl/easy.h>
#endif

class VPLHttp2__Impl {
public:
    VPLHttp2__Impl(VPLHttp2 *interface);
    ~VPLHttp2__Impl();

    // See the same named methods in #VPLHttp2.
    //{
    static int Init(void);
    static void Shutdown(void);
    int SetDebug(bool debug);
    int SetTimeout(VPLTime_t timeout);
    int SetUri(const std::string &uri);
    int AddRequestHeader(const std::string &field, const std::string &value);
    int Get(std::string &respBody);
    int Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    int Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    int Put(const std::string &reqBody,
            std::string &respBody);
    int Put(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
            std::string &respBody);
    int Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
            std::string &respBody);
    int Post(const std::string &reqBody,
             std::string &respBody);
    int Post(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             std::string &respBody);
    int Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             std::string &respBody);
    int Post(const std::string &reqBody,
             VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    int Delete(std::string& respBody);
    int GetStatusCode();
    const std::string *FindResponseHeader(const std::string &field);
    int Cancel();
    //}

private:

    VPLMutex_t mutex;

    enum HttpMethod {
        INVALID,
        GET,
        PUT,
        POST,
        DELETE,
    };
    HttpMethod httpMethod;

    // The implementation of FindResponseHeader requires this to use case_insensitive_less.
    typedef std::map<std::string, std::string, case_insensitive_less> header_list;

    // response headers
    header_list response_headers;

    std::string line;

#ifdef ANDROID
    VPLTime_t timeOut;
    bool initialized;
    std::string url;
    header_list request_headers;
    s64 requestHandle;
    bool debugLog;

    int processResponseStatusCB(int status);
    int processResponseHeadersCB(const void* buffer, size_t size);
    int progressCB(u64 dltotal, u64 dlnow, u64 ultotal, u64 ulnow);

    void closeFile();

    // Note: these only set up the local state; they do not actually perform network I/O.
    //{
    int setCommonParams(HttpMethod method);
    int setSendFromString(const std::string& content);
    int setSendFromFile(const std::string& filePath);
    int setSendFromCallback(VPLHttp2_SendCb sendCb, void* sendCtx, u64 sendSize);
    //}

    // These actually perform the network I/O.
    //{
    int connectAndRecvToString(std::string& content_out);
    int connectAndRecvToFile(const std::string& filePath);
    int connectAndRecvToCallback(VPLHttp2_RecvCb recvCb, void* recvCtx);

    int connectAndRecvHttpResponseCommon(VPLHttp_InternalRecvCallback_t internalRecvCb);
    //}

    /// A #VPLHttp_InternalRecvCallback_t.
    static int recvToStringCB(VPLHttp2__Impl& http, const void* buf, size_t size);
    /// A #VPLHttp_InternalRecvCallback_t.
    static int recvToFileCB(VPLHttp2__Impl& http, const void* buf, size_t size);
    /// A #VPLHttp_InternalRecvCallback_t.
    static int recvToCallbackCB(VPLHttp2__Impl& http, const void* buf, size_t size);

    /// A #VPLHttp_InternalSendCallback_t.
    static int sendFromCallbackCB(VPLHttp2__Impl& http, void* buf, size_t size);

    friend JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCSendCallback(
            JNIEnv * env,
            jclass clazz,
            jlong jSendCbPtr,
            jlong jHttp2ImplPtr,
            jbyteArray jBuffer);
    
    friend JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCRecvCallback(
            JNIEnv * env,
            jclass clazz,
            jlong jRecvCbPtr,
            jlong jHttp2ImplPtr,
            jbyteArray jBuffer,
            jlong jBytesInBuffer);
    
    friend JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCResponseStatusCallback(
            JNIEnv* env,
            jclass clazz,
            jlong jHttp2ImplPtr,
            int status);
    
    friend JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCResponseHeadersCallback(
            JNIEnv* env,
            jclass clazz,
            jlong jHttp2ImplPtr,
            jbyteArray jBuffer);
    
    friend JNIEXPORT jint JNICALL Java_com_igware_vpl_android_HttpManager2_callCProgressCallback(
            JNIEnv* env,
            jclass clazz,
            jlong jHttp2ImplPtr,
            jlong dltotal,
            jlong dlnow,
            jlong ultotal,
            jlong ulnow);
    
#else
    typedef VPLUtil_TimeOrderedMap<std::string, std::string, case_insensitive_less> unordered_header_list;

    // request headers
    unordered_header_list request_headers;

    std::string url;

    CURL* handle;
    bool handleInUse;

    bool debugOn;

    // response header parsing state
    enum ReceiveState {
        RESPONSE_LINE,
        RESPONSE_HEADER,
    };
    ReceiveState receive_state;

    /// Intended for CURLOPT_WRITEFUNCTION.
    static size_t recvToStringCB(const void *buf, size_t size, size_t nmemb, void *param);
    /// Intended for CURLOPT_WRITEFUNCTION.
    static size_t recvToFileCB(const void *buf, size_t size, size_t nmemb, void *param);
    /// Intended for CURLOPT_WRITEFUNCTION.
    static size_t recvToCallbackCB(const void *buf, size_t size, size_t nmemb, void *param);

    /// Intended for CURLOPT_READFUNCTION.
    static size_t sendFromFileCB(void *buf, size_t size, size_t nmemb, void *param);
    /// Intended for CURLOPT_READFUNCTION.
    static size_t sendFromCallbackCB(void *buf, size_t size, size_t nmemb, void *param);

    /// Intended for CURLOPT_PROGRESSFUNCTION.
    static size_t progressCB(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

    static int responseCB(void *buffer, size_t size, size_t nmemb, void *param);
    int setupProgressCallback(VPLHttp2_ProgressCb progCb, void *progCtx);
    void setMethod(const HttpMethod method);
    int connect();
    int recvToString(std::string &content);
    int recvToFile(const std::string &filePath);
    int recvToCallback(VPLHttp2_RecvCb recvCb, void *recvCtx);
    int sendFromString(const std::string &content);
    int sendFromFile(const std::string &filePath);
    int sendFromCallback(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize);
    int setupDebugCallback();
#endif

    int httpCode;

    VPLFile_handle_t fileHandle;
    VPLHttp2_ProgressCb progCB;
    /// Only set when using a Put(), Post(), etc. that takes a client-provided send callback.
    VPLHttp2_SendCb sendCB;
    /// Only set when using a Get(), Post(), etc. that takes a client-provided receive callback.
    VPLHttp2_RecvCb recvCB;
    void *progCtx;
    void *recvCtx;
    void *sendCtx;
    std::string *rxToStr;

    bool cancel;

    VPLHttp2 *pVPLHttp;
    static bool curlStarted;
};


#endif // __VPLEX__HTTP2_HPP__

