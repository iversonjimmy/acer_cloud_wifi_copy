
#include <vplu_common.h>
#include <vplu_types.h>

#include <vpl_time.h>
#include <vplex_file.h>
#include <vplex_private.h>

#include "vplex__http2.hpp"

#include <locale>

#ifdef USE_VALGRIND_INSTRUMENTATION
#  include <valgrind/memcheck.h>
#else
#  define VALGRIND_MAKE_MEM_DEFINED(x,y)
#endif

#define SET_CURL_OPT(curl, option, val) \
    BEGIN_MULTI_STATEMENT_MACRO \
    CURLcode err = curl_easy_setopt(curl, option, val); \
    if (err != CURLE_OK) { \
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, \
                "curl_easy_setopt(\"%s\") returned "FMTenum": %s", \
                #option, err, curl_easy_strerror(err)); \
    } \
    END_MULTI_STATEMENT_MACRO

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

// doc and sample code provided in 'man pthread_key_create'
static pthread_key_t threadLocalCurlHandleKey;

static void setNetworkIdleTimeout(CURL* handle, long timeout_sec)
{
    // See Bug 10500 Comment 2
    // If we get less than 1 byte/sec for a period of timeout_s,
    // abort.
    SET_CURL_OPT(handle, CURLOPT_LOW_SPEED_LIMIT, 1);
    // convert to seconds
    SET_CURL_OPT(handle, CURLOPT_LOW_SPEED_TIME, timeout_sec);
}

VPLHttp2__Impl::VPLHttp2__Impl(VPLHttp2 *interface)
    : httpMethod(INVALID), handleInUse(false), debugOn(false),
      receive_state(RESPONSE_LINE),
      httpCode(-1), fileHandle(VPLFILE_INVALID_HANDLE),
      rxToStr(NULL), cancel(false)
{
    if(!curlStarted) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Curl not started, call VPLHttp2::Init");
    }
    this->pVPLHttp = interface;

    VPLMutex_Init(&mutex);

    // get the curl handle or initialize if not existing.
    handle = (CURL*)pthread_getspecific(threadLocalCurlHandleKey);
    if (handle == NULL) {
        handle = curl_easy_init();
        if (handle == NULL) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "unable to create session handle");
            return;
        }

        int rc = pthread_setspecific(threadLocalCurlHandleKey, handle);
        if (rc != 0) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "pthread_setspecific failed: %d", rc);
        }
    }
    curl_easy_reset(handle);
    SET_CURL_OPT(handle, CURLOPT_NOSIGNAL, 1);
    SET_CURL_OPT(handle, CURLOPT_SSL_VERIFYPEER, (long)1/*proxy->verifyPeer*/);
#if defined(__CLOUDNODE__)
    SET_CURL_OPT(handle, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");
#endif
    SET_CURL_OPT(handle, CURLOPT_SSL_VERIFYHOST, 2);

    const long DEFAULT_IDLE_TIMEOUT_SEC = 30;
    setNetworkIdleTimeout(handle, DEFAULT_IDLE_TIMEOUT_SEC);
}

static void destroyCurlHandle(void* curlHandle) {
    curl_easy_cleanup((CURL*)curlHandle);
}

VPLHttp2__Impl::~VPLHttp2__Impl()
{
    // destroy curl handle and everything
    if (VPLFile_IsValidHandle(this->fileHandle)) {
        VPLFile_Close(this->fileHandle);
    }

    VPLMutex_Destroy(&mutex);
}

int VPLHttp2__Impl::SetDebug(bool debug)
{
    int rv = 0;
    if (debug) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "SetDebug %d", debug);
        setupDebugCallback();
        debugOn = true;
    }
        
    return rv;
}

int VPLHttp2__Impl::SetTimeout(VPLTime_t timeout)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    // convert from microseconds to seconds.
    long timeout_s = VPLTime_ToSecRoundUp(timeout);
    SET_CURL_OPT(handle, CURLOPT_CONNECTTIMEOUT, timeout_s);
    // Set timeouts:
    // Don't use CURLOPT_TIMEOUT; it sets a time limit for the entire download.  For a
    // large-enough file, this will make it impossible to succeed.

    // TODO: Should not be here (see Bug  10528), but keeping this setNetworkIdle
    // call here to not change previous behavior (for 2.5.0) in case any app
    // inadvertently depended on this.  This should already be set by the
    // constructor and not exposed to the caller.
    setNetworkIdleTimeout(handle, timeout_s);

    return 0;
}

int VPLHttp2__Impl::SetUri(const std::string &uri)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    // Must make a copy so that if the caller passed us a temporary string, we won't end up pointing
    // to freed memory.
    url = uri;
    SET_CURL_OPT(handle, CURLOPT_URL, url.c_str());
    SET_CURL_OPT(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    return 0;
}

int VPLHttp2__Impl::AddRequestHeader(const std::string &field, const std::string &value)
{
    this->request_headers[field] = value;
    return 0;
}

int VPLHttp2__Impl::Get(std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    setMethod(GET);
    rv = recvToString(respBody);
    if (rv == 0) {
        rv = connect();
    }

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    setMethod(GET);
    rv = recvToFile(respBodyFilePath);
    if (rv == 0) {
        rv = setupProgressCallback(recvProgCb, recvProgCtx);
    }
    if (rv == 0) {
        rv = connect();
    }

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    setMethod(GET);
    rv = recvToCallback(recvCb, recvCtx);
    if (rv == 0) {
        rv = setupProgressCallback(recvProgCb, recvProgCtx);
    }
    if (rv == 0) {
        rv = connect();
    }

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Put(const std::string &reqBody,
                        std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    setMethod(PUT);
    rv = sendFromString(reqBody);
    if (rv == 0) {
        rv = recvToString(respBody);
    }
    if (rv == 0) {
        rv = connect();
    }

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

    setMethod(PUT);
    rv = sendFromFile(reqBodyFilePath);
    if (rv == 0) {
        rv = setupProgressCallback(sendProgCb, sendProgCtx);
    }
    if (rv == 0) {
        rv = recvToString(respBody);
    }
    if (rv == 0) {
        rv = connect();
    }

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

    setMethod(PUT);
    rv = sendFromCallback(sendCb, sendCtx, sendSize);
    if (rv == 0) {
        rv = setupProgressCallback(sendProgCb, sendProgCtx);
    }
    if (rv == 0) {
        rv = recvToString(respBody);
    }
    if (rv == 0) {
        rv = connect();
    }

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Post(const std::string &reqBody,
                         std::string &respBody)
{
    int rv = 0;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    setMethod(POST);
    rv = sendFromString(reqBody);
    if (rv == 0) {
        rv = recvToString(respBody);
    }
    if (rv == 0) {
        rv = connect();
    }

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

    setMethod(POST);
    rv = sendFromFile(reqBodyFilePath);
    if (rv == 0) {
        rv = setupProgressCallback(sendProgCb, sendProgCtx);
    }
    if (rv == 0) {
        rv = recvToString(respBody);
    }
    if (rv == 0) {
        rv = connect();
    }
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

    setMethod(POST);
    rv = sendFromCallback(sendCb, sendCtx, sendSize);
    if (rv == 0) {
        rv = setupProgressCallback(sendProgCb, sendProgCtx);
    }
    if (rv == 0) {
        rv = recvToString(respBody);
    }
    if (rv == 0) {
        rv = connect();
    }

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

    setMethod(POST);
    rv = sendFromString(reqBody);
    if( rv == 0) {
        rv = recvToCallback(recvCb, recvCtx);
    }
    if (rv == 0) {
        rv = setupProgressCallback(recvProgCb, recvProgCtx);
    }
    if (rv == 0) {
        rv = connect();
    }

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::Delete(std::string& respBody)
{
    int rv;

    GET_HANDLE(&(this->mutex), this->handleInUse);

    setMethod(DELETE);

    rv = recvToString(respBody);
    if (rv == 0) {
        rv = connect();
    }

    PUT_HANDLE(&(this->mutex), this->handleInUse);

    return rv;
}

int VPLHttp2__Impl::GetStatusCode()
{
    return httpCode;
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
    VPLMutex_Lock(&(this->mutex));
    if ((this->handleInUse) == false) {
        VPLMutex_Unlock(&(this->mutex));
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "handle is not in-use. abort");
        return -1;
    }
    // set flag and return 0 at read/write callback to break the connection
    this->cancel = true;
    VPLMutex_Unlock(&(this->mutex));
    return 0;
}

// internal functions

static int VPLXlat_Curl_Err(CURLcode err)
{
    if (err == CURLE_OK) {
        return VPL_OK;
    } else if (err == CURLE_OPERATION_TIMEDOUT) {
        return VPL_ERR_TIMEOUT;
    } else if (err == CURLE_COULDNT_RESOLVE_PROXY
            || err == CURLE_COULDNT_RESOLVE_HOST) {
        return VPL_ERR_UNREACH;
    } else if (err == CURLE_COULDNT_CONNECT) {
        return VPL_ERR_CONNREFUSED;
    } else if (err == CURLE_SEND_ERROR
            || err == CURLE_RECV_ERROR) {
        return VPL_ERR_IO;
    } else if (err == CURLE_ABORTED_BY_CALLBACK) {
        return VPL_ERR_CANCELED;
    } else if (err == CURLE_PARTIAL_FILE) {
        return VPL_ERR_RESPONSE_TRUNCATED;
    } else {
        VPL_LIB_LOG_WARN(VPL_SG_HTTP, "Converting unknown CURL error code %d to %d.",
                         err, VPL_ERR_HTTP_ENGINE);
        return VPL_ERR_HTTP_ENGINE;
    }
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

int VPLHttp2__Impl::responseCB(void *buffer, size_t size, size_t nmemb, void *param)
{
    VPLHttp2__Impl *http = (VPLHttp2__Impl *)param;

    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl is NULL. Abort!");
        return 0;
    }
    if (buffer == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Input response buffer is NULL. Abort!");
        return 0;
    }

    size_t bytesToWrite = size*nmemb;

    char *buf = (char *)buffer;
    int rv = 0;
    size_t beginline = 0;

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

        http->line.append(&(buf[beginline]), linelen);
        beginline += linelen;
        rv += linelen;
        if(*(http->line.end() - 1) != '\n') {
            // not enough for a complete line
            break;
        }
        strip_newline(http->line);
        // Continue to receive next response line if the previous response status is HTTP 100
        if ((http->line.length() == 0) && (http->httpCode == 100))
        {
            http->receive_state = RESPONSE_LINE;
            http->httpCode = -1;
        } else {
            // Ignore the headers from HTTP 100
            if ((http->httpCode != 100))
            {
                // get the response line first and then header fields
                if (http->receive_state == RESPONSE_LINE) {
                    // Collect the command line
                    strip_newline(http->line);
                    if(http->line.length() > 0) {
                        // Find result code in the line
                        size_t pos = http->line.find_first_of(' ');
                        int tmp = -1;
                        http->line.erase(0, pos);
                        sscanf(http->line.c_str(), "%d", &tmp);

                        http->receive_state = RESPONSE_HEADER;
                        http->httpCode = tmp;
                        if (tmp == 100) {
                            if(http->debugOn) {
                                VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "RX 100 Continue. Expect next response header.");
                            }
                        }
                    }
                } else {
                    // Collect a header line
                    // Headers are of the form (name ':' field) with any amount of whitespace
                    // following the ':' before the field value. Whitespace after field is ignored.
                    std::string name = http->line.substr(0, http->line.find_first_of(':'));
                    std::string value = http->line.substr(http->line.find_first_of(':')+1);

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
                    if(http->response_headers.find(name) != http->response_headers.end()) {
                        VPL_LIB_LOG_WARN(VPL_SG_HTTP, "duplicate header found. name = %s\n - origin = %s\n - after = %s",
                                         name.c_str(), http->response_headers[name].c_str(), value.c_str());
                    }
                    http->response_headers[name] = value;
                }
            }
        }
        http->line.clear();
    }
    return rv;
}

void VPLHttp2__Impl::setMethod(const HttpMethod method)
{
    // since we are using enum. it can only be the defined value
    httpMethod = method;
    switch (method) {
    case POST:
        SET_CURL_OPT(handle, CURLOPT_POST, 1);
        SET_CURL_OPT(handle, CURLOPT_UPLOAD, 0);
        SET_CURL_OPT(handle, CURLOPT_CUSTOMREQUEST, NULL);
        break;
    case PUT:
        SET_CURL_OPT(handle, CURLOPT_POST, 0);
        SET_CURL_OPT(handle, CURLOPT_UPLOAD, 1);
        SET_CURL_OPT(handle, CURLOPT_CUSTOMREQUEST, NULL);
        break;
    case DELETE:
        SET_CURL_OPT(handle, CURLOPT_POST, 0);
        SET_CURL_OPT(handle, CURLOPT_UPLOAD, 0);
        SET_CURL_OPT(handle, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    case GET:
        SET_CURL_OPT(handle, CURLOPT_POST, 0);
        SET_CURL_OPT(handle, CURLOPT_UPLOAD, 0);
        SET_CURL_OPT(handle, CURLOPT_CUSTOMREQUEST, NULL);
        break;
    case INVALID:
        break;
    }
}

int VPLHttp2__Impl::connect()
{
    curl_slist* curl_request_headers = NULL;

    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized. Abort!");
        return -1;
    }

    // reset httpstatus and response status
    this->httpCode = -1;
    this->receive_state = RESPONSE_LINE;
    this->response_headers.clear();
    this->line.clear();

    {
        unordered_header_list::iterator it = request_headers.begin();
        for (; it != request_headers.end(); it++) {
            struct curl_slist* temp = curl_slist_append(curl_request_headers, (it->first+": "+it->second).c_str());
            if (temp == NULL) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl_slist_append failed; probably out-of-memory");
                return -1;
            }
            curl_request_headers = temp;
        }
    }

    // setup request headers
    SET_CURL_OPT(handle, CURLOPT_HTTPHEADER, curl_request_headers);
    // setup response header callback
    SET_CURL_OPT(handle, CURLOPT_HEADERFUNCTION, &responseCB);
    SET_CURL_OPT(handle, CURLOPT_WRITEHEADER, (void *) this);

    // issue the request to server
    CURLcode performCode = curl_easy_perform(handle);
    if (performCode != CURLE_OK) {
        VPL_LIB_LOG_WARN(VPL_SG_HTTP,
                         "curl_easy_perform returned "FMTenum" (%s)",
                         performCode, curl_easy_strerror(performCode));
    }

    if (curl_request_headers != NULL) {
        curl_slist_free_all(curl_request_headers);
    }

    return VPLXlat_Curl_Err(performCode);
}

size_t VPLHttp2__Impl::recvToStringCB(const void *buf, size_t size, size_t nmemb, void *param)
{
    VALGRIND_MAKE_MEM_DEFINED(buf, size * nmemb);

    VPLHttp2__Impl *http = (VPLHttp2__Impl *)param;

    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl is NULL. Abort!");
        return 0;
    }

    if (http->rxToStr == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "rxToStr is NULL. Abort!");
        return 0;
    }

    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Abort due to cancellation is requested!");
        http->cancel = false;
        return 0;
    }

    size_t bytesToWrite = size*nmemb;

    if (bytesToWrite > 0) {
        http->rxToStr->append(static_cast<const char*>(buf), bytesToWrite);
    }

    return bytesToWrite;
}

int VPLHttp2__Impl::recvToString(std::string &content)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }
    this->rxToStr = &content;

    // Register the function to receive the payload.
    SET_CURL_OPT(handle, CURLOPT_WRITEFUNCTION, recvToStringCB);
    // Serves as the param for the callback.
    SET_CURL_OPT(handle, CURLOPT_WRITEDATA, (void *)this);

    return 0;
}

size_t VPLHttp2__Impl::recvToFileCB(const void *buf, size_t size, size_t nmemb, void *param)
{
    VALGRIND_MAKE_MEM_DEFINED(buf, size * nmemb);

    VPLHttp2__Impl *http = (VPLHttp2__Impl *)param;

    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL");
        return 0;
    }

    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Abort due to cancellation is requested!");
        http->cancel = false;
        return 0;
    }

    size_t bytesToWrite = size*nmemb;

    // file write loop
    do {
        ssize_t bytesWritten = VPLFile_Write(http->fileHandle, buf, bytesToWrite);
        if (bytesWritten < 0) {
            int totalBytesWritten = (size*nmemb) - bytesToWrite;
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error %d writing to file after writing %d bytes.",
                            bytesWritten, totalBytesWritten);
            nmemb = totalBytesWritten;
            break;
        }
        bytesToWrite -= bytesWritten;
    } while (bytesToWrite >0);

    return size*nmemb-bytesToWrite;
}

int VPLHttp2__Impl::recvToFile(const std::string &filePath)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    const int writePermissions = VPLFILE_OPENFLAG_CREATE |
                                 VPLFILE_OPENFLAG_WRITEONLY |
                                 VPLFILE_OPENFLAG_TRUNCATE;

    this->fileHandle = VPLFile_Open(filePath.c_str(), writePermissions, 0666);

    if(!VPLFile_IsValidHandle(this->fileHandle)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error creating file %s, %d", filePath.c_str(), this->fileHandle);
        return this->fileHandle;
    }

    // Register the function to receive the payload.
    SET_CURL_OPT(handle, CURLOPT_WRITEFUNCTION, &recvToFileCB);
    // Serves as the param for the callback.
    SET_CURL_OPT(handle, CURLOPT_WRITEDATA, (void *)this);

    return 0;
}

size_t VPLHttp2__Impl::progressCB(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    VPLHttp2__Impl *http = (VPLHttp2__Impl *)clientp;

    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL. Abort!");
        return 1;
    }
    if (http->pVPLHttp == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 object is NULL. Abort!");
        return 1;
    }
    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Abort due to cancellation is requested!");
        http->cancel = false;
        return 1;
    }

    if (dltotal > 0) {
        if (http->progCB) {
            http->progCB(http->pVPLHttp, http->progCtx, (u64)dltotal, (u64)dlnow);
        }
    } else if (ultotal > 0) {
        if (http->progCB) {
            http->progCB(http->pVPLHttp, http->progCtx, (u64)ultotal, (u64)ulnow);
        }
    }

    return 0;
}

int VPLHttp2__Impl::setupProgressCallback(VPLHttp2_ProgressCb progCb, void *progCtx)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    this->progCB = progCb;
    this->progCtx = progCtx;

    // Register the function to receive the payload.
    SET_CURL_OPT(handle, CURLOPT_NOPROGRESS, false);
    SET_CURL_OPT(handle, CURLOPT_PROGRESSFUNCTION, progressCB);
    // Serves as the param for the callback.
    SET_CURL_OPT(handle, CURLOPT_PROGRESSDATA, (void *)this);

    return 0;
}

size_t VPLHttp2__Impl::recvToCallbackCB(const void *buf, size_t size, size_t nmemb, void *param)
{
    VALGRIND_MAKE_MEM_DEFINED(buf, size * nmemb);

    VPLHttp2__Impl *http = (VPLHttp2__Impl *)param;

    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL. Abort!");
        return 0;
    }
    if (http->pVPLHttp == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 object is NULL. Abort!");
        return 0;
    }
    if (http->recvCB == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 receive callback is NULL. Abort!");
        return 0;
    }
    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Abort due to cancellation is requested!");
        http->cancel = false;
        return 0;
    }
    return http->recvCB(http->pVPLHttp, http->recvCtx, (const char*)buf, size*nmemb);
}


int VPLHttp2__Impl::recvToCallback(VPLHttp2_RecvCb recvCb, void *recvCtx)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    this->recvCB = recvCb;
    this->recvCtx = recvCtx;

    // Register the function to receive the payload.
    SET_CURL_OPT(handle, CURLOPT_WRITEFUNCTION, &recvToCallbackCB);
    // Serves as the param for the callback.
    SET_CURL_OPT(handle, CURLOPT_WRITEDATA, (void *)this);

    return 0;
}

int VPLHttp2__Impl::sendFromString(const std::string &content)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    // Use custom request to make sure the content-length is appended by the CURL
    if (this->httpMethod == PUT) {
        SET_CURL_OPT(handle, CURLOPT_POST, 0);
        SET_CURL_OPT(handle, CURLOPT_UPLOAD, 0);
        SET_CURL_OPT(handle, CURLOPT_CUSTOMREQUEST, "PUT");
    }

    // Need to set even when empty; otherwise, curl will do a get rather than a
    // post when these options are not set.
    SET_CURL_OPT(handle, CURLOPT_POSTFIELDS, content.c_str());
    // Must *not* include the null-terminator.
    SET_CURL_OPT(handle, CURLOPT_POSTFIELDSIZE, content.size());

    return 0;
}

size_t VPLHttp2__Impl::sendFromFileCB(void *buf, size_t size, size_t nmemb, void *param)
{
    VPLHttp2__Impl *http = (VPLHttp2__Impl *)param;
    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL. Abort!");
        return 0;
    }

    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Abort due to cancellation is requested!");
        http->cancel = false;
        return CURL_READFUNC_ABORT;
    }

    size_t bytesToSend = size * nmemb;
    ssize_t byteRead = 0;

    char *cur_pos = (char *)buf;
    do {
        byteRead = VPLFile_Read(http->fileHandle, cur_pos, bytesToSend);
        if (byteRead == 0) {
            // EOF
            break;
        } else if (byteRead < 0) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error %d reading to file after reading %d bytes.",
                            byteRead, (cur_pos-(char *)buf));
            break;
        }
        cur_pos += byteRead;
        bytesToSend -= byteRead;
    } while (bytesToSend > 0);

    return cur_pos-(char *)buf;
}

int VPLHttp2__Impl::sendFromFile(const std::string &filePath)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    int rv = 0;
    curl_off_t filesize;

    this->fileHandle = VPLFile_Open(filePath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(this->fileHandle)) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Fail to open source file:%d, %s",
                        (int)this->fileHandle, filePath.c_str());
        rv = -1;
        goto out;
    }

    filesize = (curl_off_t) VPLFile_Seek(this->fileHandle, 0, VPLFILE_SEEK_END);
    if (filesize < 0) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "File size invalid "FMTu64, filesize);
        rv = -1;
        goto out;
    }
    VPLFile_Seek(this->fileHandle, 0, VPLFILE_SEEK_SET); // Reset

    if (this->httpMethod == PUT) {
        if (filesize == 0) {
            // If nothing goes to server with PUT method, need to set this to 0
            // otherwise cURL will wait for something to read
            SET_CURL_OPT(handle, CURLOPT_INFILESIZE, 0);
        } else {
            SET_CURL_OPT(handle, CURLOPT_INFILESIZE_LARGE, filesize);
        }
    } else if (this->httpMethod == POST) {
        // We are going to use the CURLOPT_READFUNCTION, the CURLOPT_POSTFILEDS can only be NULL
        SET_CURL_OPT(handle, CURLOPT_POSTFIELDS, NULL);
        SET_CURL_OPT(handle, CURLOPT_POSTFIELDSIZE_LARGE, filesize);
    } else {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unhandled http method = %d. Abort!", this->httpMethod);
        rv = -1;
        goto out;
    }

    SET_CURL_OPT(handle, CURLOPT_READFUNCTION, &sendFromFileCB);
    SET_CURL_OPT(handle, CURLOPT_READDATA, (void *)this);

    return rv;

out:
    if (VPLFile_IsValidHandle(this->fileHandle)) {
        VPLFile_Close(this->fileHandle);
        this->fileHandle = VPLFILE_INVALID_HANDLE;
    }
    return rv;
}

size_t VPLHttp2__Impl::sendFromCallbackCB(void *buf, size_t size, size_t nmemb, void *param)
{
    VPLHttp2__Impl *http = (VPLHttp2__Impl *)param;

    if (http == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2__Impl object is NULL. Abort!");
        return 0;
    }
    if (http->pVPLHttp == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 object is NULL. Abort!");
        return 0;
    }
    if (http->sendCB == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "VPLHttp2 send callback is NULL. Abort!");
        return 0;
    }
    if (http->cancel == true) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Abort due to cancellation is requested!");
        http->cancel = false;
        return 0;
    }
    // set to chunk size if bigger than
    size_t bytesToSend = size*nmemb;

    return http->sendCB(http->pVPLHttp, http->sendCtx, (char*)buf, bytesToSend);
}

int VPLHttp2__Impl::sendFromCallback(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize)
{
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    this->sendCB = sendCb;
    this->sendCtx = sendCtx;

    if (this->httpMethod == PUT) {
        if (sendSize == 0) {
            // If nothing goes to server with PUT method, need to set this to 0
            // otherwise cURL will wait for something to read
            SET_CURL_OPT(handle, CURLOPT_INFILESIZE, 0);
        } else {
            SET_CURL_OPT(handle, CURLOPT_INFILESIZE_LARGE, sendSize);
        }
    } else if (this->httpMethod == POST) {
        SET_CURL_OPT(handle, CURLOPT_POSTFIELDS, NULL);
        SET_CURL_OPT(handle, CURLOPT_POSTFIELDSIZE_LARGE, sendSize);
    } else {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unhandled http method = %d. Abort!", this->httpMethod);
        return -1;
    }

    SET_CURL_OPT(handle, CURLOPT_READFUNCTION, &sendFromCallbackCB);
    SET_CURL_OPT(handle, CURLOPT_READDATA, (void *)this);

    return 0;
}

static int curlDebugCallback(CURL* curl, curl_infotype infoType, char* text, size_t len, void* cbParam)
{
    VALGRIND_MAKE_MEM_DEFINED(text, len);
    const char* logType;
    switch(infoType) {
    case CURLINFO_TEXT:
        logType = "text";
        break;
    case CURLINFO_HEADER_IN:
        logType = "header in";
        break;
    case CURLINFO_DATA_IN:
        logType = "data in";
        break;
    case CURLINFO_HEADER_OUT:
        logType = "header out";
        break;
    case CURLINFO_DATA_OUT:
        logType = "data out";
        break;
    case CURLINFO_SSL_DATA_IN:
    case CURLINFO_SSL_DATA_OUT:
        // Nothing interesting to log; this will be called again with the decrypted data.
        return 0;
    case CURLINFO_END:
    default:
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Unexpected infotype %d", (int)infoType);
        return 0;
    }
    VPL_LogHttpBuffer(logType, text, len);
    return 0;
}

int VPLHttp2__Impl::setupDebugCallback() {
    if (this->handle == NULL) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "curl session handle is not initialized!");
        return -1;
    }

    SET_CURL_OPT(handle, CURLOPT_VERBOSE, 1);
    SET_CURL_OPT(handle, CURLOPT_DEBUGFUNCTION, curlDebugCallback);

    return 0;
}

bool VPLHttp2__Impl::curlStarted = false;

int VPLHttp2__Impl::Init(void)
{
    if(curlStarted) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Curl already started");
        return VPL_ERR_ALREADY;
    }

    CURLcode curlCode = curl_global_init(CURL_GLOBAL_ALL);
    if (curlCode == CURLE_OK) {
        int rc = pthread_key_create(&threadLocalCurlHandleKey, destroyCurlHandle);
        if(rc != 0) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                            "pthread_key_create failed: %d", rc);
            curl_global_cleanup();
            return VPL_ERR_MAX;  // I'm guessing some resource ran out.
        }
        curlStarted = true;
        return VPL_OK;
    } else {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,
                "curl_global_init returned "FMTenum": %s",
                curlCode, curl_easy_strerror(curlCode));
        return VPLXlat_Curl_Err(curlCode);
    }
}

void VPLHttp2__Impl::Shutdown(void)
{
    if (curlStarted) {
        VPL_LIB_LOG_ALWAYS(VPL_SG_HTTP, "Calling curl_global_cleanup()");
        curl_global_cleanup();
        // Not calling pthread_key_delete on purpose.  After such a call,
        // the curl handle will no longer be cleaned up when each thread ends.
       
        // Not setting curlStarted to false.  Once vplex_http2 shuts down, it is not exepcted to be initialized again.
    }
}

