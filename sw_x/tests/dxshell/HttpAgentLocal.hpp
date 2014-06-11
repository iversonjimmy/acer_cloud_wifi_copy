#ifndef _HTTP_AGENT_LOCAL_HPP_
#define _HTTP_AGENT_LOCAL_HPP_

#include "HttpAgent.hpp"

#include <vplex_http2.hpp>
#include <vplex_file.h>

#include <string>

class HttpAgentLocal : public HttpAgent {
public:
    HttpAgentLocal();
    ~HttpAgentLocal();
    virtual int get(const std::string &url,
                    size_t maxbytes,
                    VPLTime_t maxbytes_delay,
                    const char* headers[] = NULL,
                    size_t numHeaders = 0);
    virtual int get_extended(const std::string &url,
                             size_t maxbytes,
                             VPLTime_t maxbytes_delay,
                             const char* headers[],
                             size_t numHeaders,
                             const char* file_save_response);
    virtual int get_extended_by_ip_port(const std::string &url,
                             size_t maxbytes,
                             VPLTime_t maxbytes_delay,
                             const char* headers[],
                             size_t numHeaders,
                             const char* file_save_response,
                             const VPLNet_addr_t& ip_addr,
                             const VPLNet_port_t& port);
    int get_response_back(const std::string &url,
                             size_t maxbytes,
                             VPLTime_t maxbytes_delay,
                             const char* headers[],
                            size_t numHeaders,
                            std::string &response);
    int post(const std::string &url,
                            const char* headers[],
                            size_t numHeaders,
                            const char* payload,
                            size_t payloadLen,
                            const char* file,
                            std::string &response);
    int put(const std::string &url,
                            const char* headers[],
                            size_t numHeaders,
                            const char* payload,
                            size_t payloadLen,
                            const char* file,
                            std::string &response);
    int del(const std::string &url,
                            const char* headers[],
                            size_t numHeaders,
                            const char* payload,
                            size_t payloadLen,
                            const char* file,
                            std::string &response);

private:
    static size_t write_callback(const void *buf, size_t size, size_t nmemb, void *param);
    static s32 _write_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size);
    static size_t write_to_file_callback(const void *buf, size_t size, size_t nmemb, void *param);
    static s32 _write_to_file_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size);
    static size_t write_to_response_callback(const void *buf, size_t size, size_t nmemb, void *param);
    static s32 _write_to_response_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size);
    VPLFile_handle_t responseFileHandle;
};

#endif // _HTTP_AGENT_LOCAL_HPP_
