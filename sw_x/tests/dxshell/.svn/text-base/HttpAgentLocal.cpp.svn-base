#include "HttpAgentLocal.hpp"
#include <vplex_file.h>
#include <vpl_th.h>
#include <log.h>

size_t HttpAgentLocal::write_callback(const void *buf, size_t size, size_t nmemb, void *param)
{
    HttpAgentLocal *agent = (HttpAgentLocal*)param;
    agent->stats.file.bytes += size * nmemb;
    if ((agent->maxfilebytes > 0) && (agent->stats.file.bytes >= agent->maxfilebytes)) {

        if (agent->maxbytes_delay > 0) {
            VPLThread_Sleep(agent->maxbytes_delay);
        }

        // pretend write failed, to interrupt download
        LOG_WARN("Interrupting download by returning error from callback function");
        nmemb = 0;
    }
    return nmemb;
}

s32 HttpAgentLocal::_write_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    HttpAgentLocal *agent = (HttpAgentLocal*)ctx;

    return agent->write_callback(buf, size, 1, ctx) * size;
}

HttpAgentLocal::HttpAgentLocal()
:  responseFileHandle(VPLFILE__INVALID_HANDLE)
{
}

HttpAgentLocal::~HttpAgentLocal()
{
}

#define SETUP_HEADERS() \
    BEGIN_MULTI_STATEMENT_MACRO \
        for(size_t i=0; i<numHeaders; i++){ \
        std::string header = headers[i]; \
        size_t pos = 0; \
        pos = header.find(":"); \
        if(pos != std::string::npos) \
            h.AddRequestHeader(header.substr(0,pos), header.substr(pos+1)); \
    } \
    END_MULTI_STATEMENT_MACRO \

int HttpAgentLocal::get(const std::string &url, size_t maxbytes, VPLTime_t maxbytes_delay, const char* headers[], size_t numHeaders)
{
    int rv = 0;
    VPLTime_t time_begin, time_end;
    int httpResponse = -1;
    VPLHttp2 h;
    
    this->maxfilebytes = maxbytes;
    this->maxbytes_delay = maxbytes_delay;
    memset(&stats.file, 0, sizeof(stats.file));

    SETUP_HEADERS();
    h.SetUri(url);
    h.SetTimeout(VPLTIME_FROM_SEC(30));

    time_begin = VPLTime_GetTimeStamp();
    rv = h.Get(_write_callback, (void*)this, NULL, NULL);
    time_end = VPLTime_GetTimeStamp();

    httpResponse = h.GetStatusCode();
    this->http_statuscode = httpResponse;

    if (maxfilebytes > 0) {  // interrupted download case
        // If we were able to read at least the specified number of bytes, mark the attempt a success.
        if (stats.file.bytes >= maxfilebytes) {
            rv = 0;
        }
    }
    else {
        if (rv < 0) {
            LOG_ERROR("VPLHttp_Request failed: %d", rv);
        }
        if ((httpResponse != -1) && (httpResponse < 200 || httpResponse >= 300)) {
            LOG_ERROR("HTTP response code %d unexpected", httpResponse);
            if (rv == 0)
                rv = -1;
        }
        if (rv < 0) {
            stats.total.failed++;
            goto end;
        }
    }

    stats.total.pass++;

end:
    stats.file.time = time_end - time_begin;
    stats.total.time += stats.file.time;
    stats.total.bytes += stats.file.bytes;
    stats.total.nfiles++;
    return rv;
}

size_t HttpAgentLocal::write_to_file_callback(const void *buf,
                                              size_t size,
                                              size_t nmemb,
                                              void *param)
{
    HttpAgentLocal *agent = (HttpAgentLocal*)param;
    agent->stats.file.bytes += size * nmemb;
    size_t bytesToWrite = size*nmemb;
    do {
        ssize_t bytesWritten = VPLFile_Write(agent->responseFileHandle,
                                             buf,
                                             bytesToWrite);
        if(bytesWritten < 0) {
            size_t totalBytesWritten = (size*nmemb) - bytesToWrite;
            LOG_ERROR("Error "FMTd_ssize_t" writing to file after writing "FMTu_size_t" bytes.",
                      bytesWritten, totalBytesWritten);
            nmemb = totalBytesWritten;
            break;
        }
        bytesToWrite -= bytesWritten;
    }while(bytesToWrite>0);

    if ((agent->maxfilebytes > 0) && (agent->stats.file.bytes >= agent->maxfilebytes)) {

        if (agent->maxbytes_delay > 0) {
            VPLThread_Sleep(agent->maxbytes_delay);
        }

        // pretend write failed, to interrupt download
        LOG_WARN("Interrupting download by returning error from callback function");
        nmemb = 0;
    }
    return nmemb;
}

s32 HttpAgentLocal::_write_to_file_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    HttpAgentLocal *agent = (HttpAgentLocal*)ctx;

    return agent->write_to_file_callback(buf, size, 1, ctx) * size;
}


size_t HttpAgentLocal::write_to_response_callback(const void *buf,
                                              size_t size,
                                              size_t nmemb,
                                              void *param)
{
    HttpAgentLocal *agent = (HttpAgentLocal*)param;
    agent->stats.file.bytes += size * nmemb;
    size_t bytesToWrite = size*nmemb;

    if (bytesToWrite>0) {
        agent->response.append(static_cast<const char*>(buf), bytesToWrite);
    }

    if ((agent->maxfilebytes > 0) && (agent->stats.file.bytes >= agent->maxfilebytes)) {

        if (agent->maxbytes_delay > 0) {
            VPLThread_Sleep(agent->maxbytes_delay);
        }

        // pretend write failed, to interrupt download
        LOG_WARN("Interrupting download by returning error from callback function");
        nmemb = 0;
    }
    return nmemb;
}

s32 HttpAgentLocal::_write_to_response_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    HttpAgentLocal *agent = (HttpAgentLocal*)ctx;

    return agent->write_to_response_callback(buf, size, 1, ctx) * size;
}



int HttpAgentLocal::get_extended(const std::string &url,
                                 size_t maxbytes,
                                 VPLTime_t maxbytes_delay,
                                 const char* headers[],
                                 size_t numHeaders,
                                 const char* file_save_response)
{
    int rv = 0;
    int rc;
    VPLTime_t time_begin, time_end;
    int httpResponse = -1;
    VPLHttp2 h;

    LOG_ALWAYS("get_extended");
    this->maxfilebytes = maxbytes;
    this->maxbytes_delay = maxbytes_delay;
    const int writePermissions = VPLFILE_OPENFLAG_CREATE |
                                 VPLFILE_OPENFLAG_WRITEONLY |
                                 VPLFILE_OPENFLAG_TRUNCATE;
    this->responseFileHandle = VPLFile_Open(file_save_response,
                                            writePermissions,
                                            0666);
    if(!VPLFile_IsValidHandle(this->responseFileHandle)) {
        LOG_ERROR("Error creating file %s, %d",
                  file_save_response, this->responseFileHandle);
        return this->responseFileHandle;
    }
    memset(&stats.file, 0, sizeof(stats.file));

    SETUP_HEADERS();
    h.SetUri(url);
    h.SetTimeout(VPLTIME_FROM_SEC(30));

    time_begin = VPLTime_GetTimeStamp();
    rv = h.Get(_write_to_file_callback, (void*)this, NULL, NULL);
    time_end = VPLTime_GetTimeStamp();

    rc = VPLFile_Close(this->responseFileHandle);
    this->responseFileHandle = VPLFILE__INVALID_HANDLE;
    if(rc != 0) {
        LOG_ERROR("Error closing file:%d", rc);
    }

    httpResponse = h.GetStatusCode();
    this->http_statuscode = httpResponse;
    if (maxfilebytes > 0) {  // interrupted download case
        // If we were able to read at least the specified number of bytes, mark the attempt a success.
        if (stats.file.bytes >= maxfilebytes) {
            rv = 0;
        }
    }
    else {
        if (rv < 0) {
            LOG_ERROR("VPLHttp_Request failed: %d", rv);
        }
        if ((httpResponse != -1) && (httpResponse < 200 || httpResponse >= 300)) {
            LOG_ERROR("HTTP response code %d unexpected", httpResponse);
            if (rv == 0)
                rv = -1;
        }
        if (rv < 0) {
            stats.total.failed++;
            goto end;
        }
    }

    stats.total.pass++;

end:
    stats.file.time = time_end - time_begin;
    stats.total.time += stats.file.time;
    stats.total.bytes += stats.file.bytes;
    stats.total.nfiles++;
    return rv;
}

int HttpAgentLocal::get_extended_by_ip_port(const std::string &url,
                                 size_t maxbytes,
                                 VPLTime_t maxbytes_delay,
                                 const char* headers[],
                                 size_t numHeaders,
                                 const char* file_save_response,
                                 const VPLNet_addr_t& ip_addr,
                                 const VPLNet_port_t& port)
{
    return 0;
}

int HttpAgentLocal::get_response_back(const std::string &url,
                                 size_t maxbytes,
                                 VPLTime_t maxbytes_delay,
                                 const char* headers[],
                                 size_t numHeaders,
                                 std::string &response)
{
    int rv = 0;
    VPLTime_t time_begin, time_end;
    int httpResponse = -1;
    VPLHttp2 h;
    
    this->maxfilebytes = maxbytes;
    this->maxbytes_delay = maxbytes_delay;

    SETUP_HEADERS();
    h.SetUri(url);
    h.SetTimeout(VPLTIME_FROM_SEC(30));

    time_begin = VPLTime_GetTimeStamp();
    rv = h.Get(this->response);
    time_end = VPLTime_GetTimeStamp();

    httpResponse = h.GetStatusCode();
    this->http_statuscode = httpResponse;
    if (maxfilebytes > 0) {  // interrupted download case
        // If we were able to read at least the specified number of bytes, mark the attempt a success.
        if (stats.file.bytes >= maxfilebytes) {
            rv = 0;
        }
    }
    else {
        if (rv < 0) {
            LOG_ERROR("VPLHttp_Request failed: %d", rv);
        }
        if ((httpResponse != -1) && (httpResponse < 200 || httpResponse >= 300)) {
            LOG_ERROR("HTTP response code %d unexpected", httpResponse);
            if (rv == 0)
                rv = -1;
        }
        if (rv < 0) {
            stats.total.failed++;
            goto end;
        }
    }

    stats.total.pass++;

end:
    response = this->response;
    stats.file.time = time_end - time_begin;
    stats.total.time += stats.file.time;
    stats.total.bytes += stats.file.bytes;
    stats.total.nfiles++;
    return rv;
}

int HttpAgentLocal::post(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response)
{
    int rv = 0;
    int httpResponse = -1;
    VPLHttp2 h;
    
    SETUP_HEADERS();
    h.SetUri(url);
    h.SetTimeout(VPLTIME_FROM_SEC(30));

    if(payload)
        rv = h.Post(payload, this->response);
    else if(file)
        rv = h.Post(file, NULL, NULL, this->response);
    else
        rv = h.Post("", this->response);

    if (rv < 0) {
        LOG_ERROR("VPLHttp_Request failed: %d", rv);
    }

    httpResponse = h.GetStatusCode();
    this->http_statuscode = httpResponse;
    if ((httpResponse != -1) && (httpResponse < 200 || httpResponse >= 300)) {
        LOG_ERROR("HTTP response code %d unexpected", httpResponse);
        if (rv == 0)
            rv = -1;
    }
    response = this->response;

    return rv;
}

int HttpAgentLocal::put(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response)
{
    int rv = 0;
    int httpResponse = -1;
    VPLHttp2 h;
    
    SETUP_HEADERS();
    h.SetUri(url);
    h.SetTimeout(VPLTIME_FROM_SEC(30));

    if(payload)
        rv = h.Put(payload, this->response);
    else if(file)
        rv = h.Put(file, NULL, NULL, this->response);
    else
        rv = h.Put("", this->response);

    if (rv < 0) {
        LOG_ERROR("VPLHttp_Request failed: %d", rv);
    }

    httpResponse = h.GetStatusCode();
    this->http_statuscode = httpResponse;
    if ((httpResponse != -1) && (httpResponse < 200 || httpResponse >= 300)) {
        LOG_ERROR("HTTP response code %d unexpected", httpResponse);
        if (rv == 0)
            rv = -1;
    }
    response = this->response;

    return rv;
}

int HttpAgentLocal::del(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response)
{
    int rv = 0;
    int httpResponse = -1;
    VPLHttp2 h;
    
    SETUP_HEADERS();
    h.SetUri(url);
    h.SetTimeout(VPLTIME_FROM_SEC(30));

    rv = h.Delete(this->response);

    if (rv < 0) {
        LOG_ERROR("VPLHttp_Request failed: %d", rv);
    }

    httpResponse = h.GetStatusCode();
    this->http_statuscode = httpResponse;
    if ((httpResponse != -1) && (httpResponse < 200 || httpResponse >= 300)) {
        LOG_ERROR("HTTP response code %d unexpected", httpResponse);
        if (rv == 0)
            rv = -1;
    }
    response = this->response;

    return rv;
}
