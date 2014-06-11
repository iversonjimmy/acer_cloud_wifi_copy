#include "vplex_http2.hpp"
#include "vplex__http2.hpp"
#include "vpl_time.h"
#include "vpl_java.h"
#include "vplex_private.h"
#include "scopeguard.hpp"

#include <locale>

static const char* CLASSNAME = "com/igware/vpl/android/HttpManager2";

#define CHECK_NOT_NULL(var, rv, errcode, label, ...) \
    do { \
        if ((var) == NULL) { \
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, __VA_ARGS__); \
            (rv) = (errcode); \
            goto label; \
        } \
    } while (0)

static const VPLTime_t DEFAULT_TIMEOUT = VPLTIME_FROM_SEC(30);

static int
getJavaClass(JNIEnv** env_out, jclass** class_out)
{
    int rv;
    *env_out = VPLJava_GetCurrentThreadEnv();
    CHECK_NOT_NULL(*env_out, rv, VPL_ERR_FAIL, out, "Failed to get JNI environment");
    rv = VPLJava_LoadClass(CLASSNAME, class_out);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Class %s not loaded", CLASSNAME);
        goto out;
    }
    CHECK_NOT_NULL(*class_out, rv, VPL_ERR_FAIL, out, "Class was NULL");
out:
    return rv;
}

VPLHttp2__Impl::VPLHttp2__Impl(VPLHttp2 *interface)
    : httpMethod(INVALID),
    timeOut(DEFAULT_TIMEOUT),
    initialized(false),
    requestHandle(0),
    debugLog(false),
    httpCode(-1),
    fileHandle(VPLFILE_INVALID_HANDLE),
    progCB(NULL),
    sendCB(NULL),
    recvCB(NULL),
    progCtx(NULL),
    recvCtx(NULL),
    sendCtx(NULL),
    rxToStr(NULL),
    cancel(false),
    pVPLHttp(interface)
{
    int rv = VPL_OK;
    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "init", "()V");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        env->CallStaticVoidMethod(*clazz, mid);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "createRequest", "()J");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        this->requestHandle = env->CallStaticLongMethod(*clazz, mid);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }
    initialized = true;

fail:
    return;
}

VPLHttp2__Impl::~VPLHttp2__Impl()
{
    closeFile();

    int rv;
    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "destroyRequest", "(J)V");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        env->CallStaticVoidMethod(*clazz, mid, (jlong)this->requestHandle);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }
fail:
    return;
}

void VPLHttp2__Impl::closeFile()
{
    if (VPLFile_IsValidHandle(this->fileHandle)) {
        VPLFile_Close(this->fileHandle);
        this->fileHandle = VPLFILE_INVALID_HANDLE;
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
    debugLog = debug;
    return VPL_OK;
}

int VPLHttp2__Impl::SetTimeout(VPLTime_t timeout)
{
    timeOut = timeout;
    return VPL_OK;
}

int VPLHttp2__Impl::SetUri(const std::string &uri)
{
    url = uri;
    return VPL_OK;
}

int VPLHttp2__Impl::AddRequestHeader(const std::string &field, const std::string &value)
{
    this->request_headers[field] = value;
    return VPL_OK;
}

int VPLHttp2__Impl::Get(std::string &respBody)
{
    int rv = setCommonParams(GET);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    this->progCB = recvProgCb;
    this->progCtx = recvProgCtx;
    int rv = setCommonParams(GET);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToFile(respBodyFilePath);
}

int VPLHttp2__Impl::Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    this->progCB = recvProgCb;
    this->progCtx = recvProgCtx;
    int rv = setCommonParams(GET);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToCallback(recvCb, recvCtx);
}

int VPLHttp2__Impl::Put(const std::string &reqBody,
                     std::string &respBody)
{
    int rv = setCommonParams(PUT);
    if (rv < 0) {
        return rv;
    }
    rv = setSendFromString(reqBody);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::Put(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
                     std::string &respBody)
{
    this->progCB = sendProgCb;
    this->progCtx = sendProgCtx;
    int rv = setCommonParams(PUT);
    if (rv < 0) {
        return rv;
    }
    rv = setSendFromFile(reqBodyFilePath);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
                     std::string &respBody)
{
    this->progCB = sendProgCb;
    this->progCtx = sendProgCtx;
    int rv = setCommonParams(PUT);
    if (rv < 0) {
        return rv;
    }
    rv = setSendFromCallback(sendCb, sendCtx, sendSize);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::Post(const std::string &reqBody,
                      std::string &respBody)
{
    int rv = setCommonParams(POST);
    if (rv < 0) {
        return rv;
    }
    rv = setSendFromString(reqBody);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::Post(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
                      std::string &respBody)
{
    this->progCB = sendProgCb;
    this->progCtx = sendProgCtx;
    int rv = setCommonParams(POST);
    if (rv < 0) {
        return rv;
    }
    rv = setSendFromFile(reqBodyFilePath);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
                      std::string &respBody)
{
    this->progCB = sendProgCb;
    this->progCtx = sendProgCtx;
    int rv = setCommonParams(POST);
    if (rv < 0) {
        return rv;
    }
    rv = setSendFromCallback(sendCb, sendCtx, sendSize);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::Post(const std::string &reqBody,
                         VPLHttp2_RecvCb recvCb,
                         void *recvCtx,
                         VPLHttp2_ProgressCb recvProgCb,
                         void *recvProgCtx)
{
    this->progCB = recvProgCb;
    this->progCtx = recvProgCtx;
    int rv = setCommonParams(POST);
    if (rv < 0) {
        return rv;
    }
    rv = setSendFromString(reqBody);
    if (rv < 0) {
        return rv;
    }

    return connectAndRecvToCallback(recvCb, recvCtx);
}

int VPLHttp2__Impl::Delete(std::string &respBody)
{
    int rv = setCommonParams(DELETE);
    if (rv < 0) {
        return rv;
    }
    return connectAndRecvToString(respBody);
}

int VPLHttp2__Impl::GetStatusCode()
{
    return httpCode;
}

const std::string *VPLHttp2__Impl::FindResponseHeader(const std::string &field)
{
    // This will do a case-insensitive search because response_headers is a map that uses
    // case_insensitive_less.
    if (response_headers.find(field) != response_headers.end()) {
        return &(response_headers[field]);
    }
    return NULL;
}

int VPLHttp2__Impl::Cancel()
{
    int rv = VPL_OK;

    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Cancel "FMTs64, this->requestHandle);
    this->cancel = true;

    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "cancel", "(J)I");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        rv = env->CallStaticIntMethod(*clazz, mid, (jlong)this->requestHandle);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }

fail:
    return rv;
}

// internal functions
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

int VPLHttp2__Impl::processResponseStatusCB(int status)
{
    httpCode = status;
    if (debugLog) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "HTTP status: %d", httpCode);
    }
    return 0;
}

int VPLHttp2__Impl::processResponseHeadersCB(const void* buffer, size_t bytesToWrite)
{
    if (buffer == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Input response buffer is NULL. Abort!");
        return -1;
    }
    const char* buf = (const char*)buffer;
    int rv = 0;
    size_t beginline = 0;

    while (beginline < bytesToWrite) {
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
        rv += linelen;
        if(*(line.end() - 1) != '\n') {
            // not enough for a complete line
            break;
        }
        strip_newline(line);

        // Collect a header line
        // Headers are of the form (name ':' field) with any amount of whitespace
        // following the ':' before the field value. Whitespace after field is ignored.
        std::string name = line.substr(0, line.find_first_of(':'));
        std::string value = line.substr(line.find_first_of(':')+1);

        // strip leading whitespace on value
        std::locale loc;
        while (value.length() > 0 && isspace(value[0], loc)) {
            value.erase(0, 1);
        }
        // strip trailing whitespace on value
        while (value.length() > 0 && isspace(value[value.length() - 1], loc)) {
            value.erase(value.length() - 1, 1);
        }

        // Possibly consolidate headers
        if (response_headers.find(name) != response_headers.end()) {
            VPL_LIB_LOG_WARN(VPL_SG_HTTP, "duplicate header found. name = %s\n - origin = %s\n - after = %s",
                    name.c_str(), response_headers[name].c_str(), value.c_str());
        }
        response_headers[name] = value;
        if (debugLog) {
            VPL_LogHttpBuffer("header in", line.c_str(), line.size());
        }
        line.clear();
    }
    return rv;
}

int VPLHttp2__Impl::progressCB(u64 dltotal, u64 dlnow, u64 ultotal, u64 ulnow)
{
    if (pVPLHttp == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 object is NULL. Abort!");
        return -1;
    }
    if (cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Abort due to cancellation is requested!");
        return -1;
    }
    if (progCB != NULL) {
        if (dltotal > 0) {
            if (httpMethod == GET) {
                progCB(pVPLHttp, progCtx, dltotal, dlnow);
            } else {
                VPL_LIB_LOG_WARN(VPL_SG_HTTP, "http method (%d) is not GET. dltotal="FMTu64", dlnow="FMTu64,
                        httpMethod, dltotal, dlnow);
            }
        } else if (ultotal > 0) {
            progCB(pVPLHttp, progCtx, ultotal, ulnow);
        } else {
            VPL_LIB_LOG_WARN(VPL_SG_HTTP, "both upload and download total size is zero!");
        }
    }
    return 0;
}

int VPLHttp2__Impl::setCommonParams(HttpMethod method)
{
    int rv = VPL_OK;

    if (!initialized) {
        rv = VPL_ERR_FAIL;
        return rv;
    }

    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "setCommonParams", "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

        jstring jUrl = env->NewStringUTF(url.c_str());
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        ON_BLOCK_EXIT_OBJ(*env, &_JNIEnv::DeleteLocalRef, jUrl);

        std::string allHeaders;
        for (header_list::iterator it = request_headers.begin(); it != request_headers.end(); it++) {
            allHeaders += it->first + ": " + it->second + "\n";
        }

        jstring jHeaders = env->NewStringUTF(allHeaders.c_str());
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        ON_BLOCK_EXIT_OBJ(*env, &_JNIEnv::DeleteLocalRef, jHeaders);

        const char* http_method;
        httpMethod = method;
        switch(httpMethod) {
            case GET:
                http_method = "GET";
                break;
            case PUT:
                http_method = "PUT";
                break;
            case POST:
                http_method = "POST";
                break;
            case DELETE:
                http_method = "DELETE";
                break;
            case INVALID:
            default:
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Invalid httpMethod: %d", (int)httpMethod);
                rv = VPL_ERR_FAIL;
                goto fail;
        }
        if (debugLog) {
            VPL_LIB_LOG_INFO(VPL_SG_HTTP, "HTTP request: %s %s", http_method, url.c_str());
            VPL_LogHttpBuffer("headers out", allHeaders.c_str(), allHeaders.size());
        }
        jstring jHttpMethod = env->NewStringUTF(http_method);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        ON_BLOCK_EXIT_OBJ(*env, &_JNIEnv::DeleteLocalRef, jHttpMethod);
        jint jTimeoutMs = (jint)VPLTime_ToMillisec(timeOut);
        rv = env->CallStaticIntMethod(*clazz, mid, (jlong)this->requestHandle, jUrl, jHeaders, jHttpMethod, jTimeoutMs);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }
fail:
    return rv;
}

/// A #VPLHttp_InternalRecvCallback_t.
/* static */
int VPLHttp2__Impl::recvToStringCB(VPLHttp2__Impl& http, const void *buf, size_t size)
{
    if (http.rxToStr == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "rxToStr is NULL!");
        return VPL_ERR_FAIL;
    }
    if (size > 0) {
        http.rxToStr->append(static_cast<const char*>(buf), size);
    }
    return (int)size;
}

/// A #VPLHttp_InternalRecvCallback_t.
/* static */
int VPLHttp2__Impl::recvToFileCB(VPLHttp2__Impl& http, const void* buf, size_t size)
{
    size_t bytesRemaining = size;
    do {
        ssize_t numWritten = VPLFile_Write(http.fileHandle, buf, bytesRemaining);
        if (numWritten < 0) {
            int rv = (int)numWritten;
            int bytesSuccessfullyWritten = size - bytesRemaining;
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error %d writing to file after writing %d bytes.",
                            rv, bytesSuccessfullyWritten);
            return rv;
        }
        bytesRemaining -= numWritten;
    } while (bytesRemaining > 0);
    return (int)size;
}

/// A #VPLHttp_InternalRecvCallback_t.
/* static */
int VPLHttp2__Impl::recvToCallbackCB(VPLHttp2__Impl& http, const void *buf, size_t size)
{
    if (http.pVPLHttp == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 object is NULL. Abort!");
        return VPL_ERR_FAIL;
    }
    return http.recvCB(http.pVPLHttp, http.recvCtx, (const char*)buf, size);
}

// Common for all code paths.
int VPLHttp2__Impl::connectAndRecvHttpResponseCommon(VPLHttp_InternalRecvCallback_t internalRecvCb)
{
    int rv;
    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "connectAndRecvResponse", "(JJJ)I");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        jlong jHttp2ImplPtr = reinterpret_cast<jlong>(this);
        jlong jRecvCbPtr = reinterpret_cast<jlong>(internalRecvCb);
        int result = env->CallStaticIntMethod(*clazz, mid, (jlong)this->requestHandle, jHttp2ImplPtr, jRecvCbPtr);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        if (result < 0) {
            if (debugLog) {
                VPL_LIB_LOG_INFO(VPL_SG_HTTP, "HTTP error: %d", result);
            }
            rv = result;
        }
    }
fail:
    return rv;
}

int VPLHttp2__Impl::connectAndRecvToString(std::string& content_out)
{
    int rv = VPL_OK;
    {
        this->rxToStr = &content_out;
        rv = connectAndRecvHttpResponseCommon(recvToStringCB);
    }
    return rv;
}

int VPLHttp2__Impl::connectAndRecvToFile(const std::string& filePath)
{
    int rv = VPL_OK;
    {
        const int writePermissions = VPLFILE_OPENFLAG_CREATE |
                                     VPLFILE_OPENFLAG_WRITEONLY |
                                     VPLFILE_OPENFLAG_TRUNCATE;
        VPLFile_handle_t handle = VPLFile_Open(filePath.c_str(), writePermissions, 0666);
        if(!VPLFile_IsValidHandle(handle)) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error creating file %s, %d", filePath.c_str(), handle);
            return handle;
        }
        this->fileHandle = handle;
        ON_BLOCK_EXIT_OBJ(*this, &VPLHttp2__Impl::closeFile);
        rv = connectAndRecvHttpResponseCommon(recvToFileCB);
    }
    return rv;
}

int VPLHttp2__Impl::connectAndRecvToCallback(VPLHttp2_RecvCb recvCb, void* recvCtx)
{
    int rv = VPL_OK;
    {
        this->recvCB = recvCb;
        this->recvCtx = recvCtx;
        rv = connectAndRecvHttpResponseCommon(recvToCallbackCB);
    }
    return rv;
}

int VPLHttp2__Impl::setSendFromString(const std::string& content)
{
    int rv = VPL_OK;
    JNIEnv* env;
    jclass* clazz;
    if (debugLog) {
        VPL_LogHttpBuffer("send", content.c_str(), content.size());
    }
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "sendHttpContentFromString",
                "(JLjava/lang/String;)I");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

        jstring jContent = env->NewStringUTF(content.c_str());
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        ON_BLOCK_EXIT_OBJ(*env, &_JNIEnv::DeleteLocalRef, jContent);

        rv = env->CallStaticIntMethod(*clazz, mid, (jlong)this->requestHandle, jContent);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }
fail:
    return rv;
}

int VPLHttp2__Impl::setSendFromFile(const std::string& filePath)
{
    int rv = VPL_OK;
    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "sendHttpContentFromFile", "(JJLjava/lang/String;)I");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

        jstring jFilepath = env->NewStringUTF(filePath.c_str());
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        ON_BLOCK_EXIT_OBJ(*env, &_JNIEnv::DeleteLocalRef, jFilepath);

        jlong jHttp2ImplPtr = reinterpret_cast<jlong>(this);
        rv = env->CallStaticIntMethod(*clazz, mid, (jlong)this->requestHandle, jHttp2ImplPtr, jFilepath);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }
fail:
    return rv;
}

/// A #VPLHttp_InternalSendCallback_t.
/* static */
int VPLHttp2__Impl::sendFromCallbackCB(VPLHttp2__Impl& http, void* buf, size_t size)
{
    if (http.pVPLHttp == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 object is NULL!");
        return VPL_ERR_FAIL;
    }
    return http.sendCB(http.pVPLHttp, http.sendCtx, (char*)buf, size);
}

int VPLHttp2__Impl::setSendFromCallback(VPLHttp2_SendCb sendCb, void* sendCtx, u64 sendSize)
{
    int rv = VPL_OK;
    jlong jSendCbPtr = reinterpret_cast<jlong>(sendFromCallbackCB);
    jlong jHttp2ImplPtr = reinterpret_cast<jlong>(this);

    this->sendCB = sendCb;
    this->sendCtx = sendCtx;

    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "getJavaClass failed: %d", rv);
        goto fail;
    }
    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "sendHttpContentFromCallback", "(JJJJ)I");
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        rv = env->CallStaticIntMethod(*clazz, mid, (jlong)this->requestHandle, jHttp2ImplPtr, jSendCbPtr, (jlong)sendSize);
        VPLJAVA_CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
    }
fail:
    return rv;
}

/// Currently, this is only called when we are sending from a client-provided #VPLHttp2_SendCb.
JNIEXPORT jint JNICALL
Java_com_igware_vpl_android_HttpManager2_callCSendCallback(
        JNIEnv * env,
        jclass clazz,
        jlong jSendCbPtr,
        jlong jHttp2ImplPtr,
        jbyteArray jBuffer)
{
    VPLHttp2__Impl* http = reinterpret_cast<VPLHttp2__Impl*>(jHttp2ImplPtr);
    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL!");
        return -1;
    }
    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Request has been canceled.");
        return -1;
    }
    VPLHttp_InternalSendCallback_t internalSendCb = reinterpret_cast<VPLHttp_InternalSendCallback_t>(jSendCbPtr);
    jint rv = 0;
    jsize len = env->GetArrayLength(jBuffer);
    jbyte* bytes = env->GetByteArrayElements(jBuffer, NULL);
    CHECK_NOT_NULL(bytes, rv, 0, out, "Failed to get JNI array elements");
    rv = (*internalSendCb)(*http, bytes, len);
    if (http->debugLog && (rv > 0)) {
        VPL_LogHttpBuffer("send (from callback)", bytes, (size_t)rv);
    }
    // Using mode 0 to "copy back the content and free the elems buffer".
    env->ReleaseByteArrayElements(jBuffer, bytes, 0);
out:
    return rv;
}

/// This is called whenever we read a buffer of payload from the HTTP connection.
/// @param jBytesInBuffer The number of valid bytes in jBuffer (which is possibly less than full length of jBuffer).
JNIEXPORT jint JNICALL
Java_com_igware_vpl_android_HttpManager2_callCRecvCallback(
        JNIEnv * env,
        jclass clazz,
        jlong jRecvCbPtr,
        jlong jHttp2ImplPtr,
        jbyteArray jBuffer,
        jlong jBytesInBuffer)
{
    VPLHttp2__Impl* http = reinterpret_cast<VPLHttp2__Impl*>(jHttp2ImplPtr);
    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL!");
        return -1;
    }
    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Request has been canceled.");
        return -1;
    }
    VPLHttp_InternalRecvCallback_t internalRecvCb = reinterpret_cast<VPLHttp_InternalRecvCallback_t>(jRecvCbPtr);
    jint rv = 0;
    jbyte* bytes = env->GetByteArrayElements(jBuffer, NULL);
    CHECK_NOT_NULL(bytes, rv, 0, out, "Failed to get JNI array elements");
    if (http->debugLog) {
        VPL_LogHttpBuffer("receive", bytes, (size_t)jBytesInBuffer);
    }
    rv = (*internalRecvCb)(*http, bytes, (size_t)jBytesInBuffer);
    // Using JNI_ABORT since we promise that the buffer was not modified in native code.
    env->ReleaseByteArrayElements(jBuffer, bytes, JNI_ABORT);
out:
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_igware_vpl_android_HttpManager2_callCResponseStatusCallback(
        JNIEnv* env,
        jclass clazz,
        jlong jHttp2ImplPtr,
        jint status)
{
    VPLHttp2__Impl* http = reinterpret_cast<VPLHttp2__Impl*>(jHttp2ImplPtr);
    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL!");
        return -1;
    }
    return http->processResponseStatusCB(status);
}

JNIEXPORT jint JNICALL
Java_com_igware_vpl_android_HttpManager2_callCResponseHeadersCallback(
        JNIEnv* env,
        jclass clazz,
        jlong jHttp2ImplPtr,
        jbyteArray jBuffer)
{
    VPLHttp2__Impl* http = reinterpret_cast<VPLHttp2__Impl*>(jHttp2ImplPtr);
    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL!");
        return -1;
    }
    jint rv = 0;
    jsize len = env->GetArrayLength(jBuffer);
    jbyte* bytes = env->GetByteArrayElements(jBuffer, NULL);
    CHECK_NOT_NULL(bytes, rv, 0, out, "Failed to get JNI array elements");
    rv = http->processResponseHeadersCB(bytes, (size_t)len);
    // Using JNI_ABORT since we promise that the buffer was not modified in native code.
    env->ReleaseByteArrayElements(jBuffer, bytes, JNI_ABORT);
out:
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_igware_vpl_android_HttpManager2_callCProgressCallback(
        JNIEnv* env,
        jclass clazz,
        jlong jHttp2ImplPtr,
        jlong dltotal,
        jlong dlnow,
        jlong ultotal,
        jlong ulnow)
{
    VPLHttp2__Impl* http = reinterpret_cast<VPLHttp2__Impl*>(jHttp2ImplPtr);
    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL!");
        return -1;
    }
    return http->progressCB((u64)dltotal, (u64)dlnow, (u64)ultotal, (u64)ulnow);
}
