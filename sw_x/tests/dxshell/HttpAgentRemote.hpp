#ifndef _HTTP_AGENT_REMOTE_HPP_
#define _HTTP_AGENT_REMOTE_HPP_

#include "HttpAgent.hpp"

#include <string>

class HttpAgentRemote : public HttpAgent {
public:
    HttpAgentRemote();
    virtual ~HttpAgentRemote();
    int get(const std::string &url,
            size_t maxbytes,
            VPLTime_t maxbytes_delay,
            const char* headers[] = NULL,
            size_t numHeaders = 0);
    int get_extended(const std::string &url,
                     size_t maxbytes,
                     VPLTime_t maxbytes_delay,
                     const char* headers[],
                     size_t numHeaders,
                     const char* file_save_response);
    int get_extended_by_ip_port(const std::string &url,
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
};

#endif // _HTTP_AGENT_REMOTE_HPP_
