#ifndef _HTTP_AGENT_HPP_
#define _HTTP_AGENT_HPP_

#include <vplu_types.h>
#include <vpl_time.h>
#include <vpl_socket.h>

#include <string>

class HttpAgent {
public:
    HttpAgent();
    virtual ~HttpAgent();
    virtual int get(const std::string &url,
                    size_t maxbytes,
                    VPLTime_t maxbytes_delay,
                    const char* headers[] = NULL,
                    size_t numHeaders = 0) = 0;
    virtual int get_extended(const std::string &url,
                             size_t maxbytes,
                             VPLTime_t maxbytes_delay,
                             const char* headers[],
                             size_t numHeaders,
                             const char* file_save_response) = 0;
    virtual int get_extended_by_ip_port(const std::string &url,
                             size_t maxbytes,
                             VPLTime_t maxbytes_delay,
                             const char* headers[],
                             size_t numHeaders,
                             const char* file_save_response,
                             const VPLNet_addr_t& ip_addr,
                             const VPLNet_port_t& port) = 0;
    virtual int get_response_back(const std::string &url,
                             size_t maxbytes,
                             VPLTime_t maxbytes_delay,
                            const char* headers[],
                            size_t numHeaders,
                            std::string &response) = 0;
    virtual int put(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response) = 0;
    virtual int post(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response) = 0;
    virtual int del(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response) = 0;
    int getlastHttpStatusCode() { return http_statuscode; } //Note: this function will keep last http response status code
    u32 getTotalFiles() { return stats.total.nfiles; }
	u32 getPassFiles(){ return stats.total.pass; }
    u32 getFailedFiles(){ return stats.total.failed; }
    u64 getFileBytes() { return stats.file.bytes; }
    u64 getTotalBytes() { return stats.total.bytes; }
    VPLTime_t getTotalTime() { return stats.total.time; }
    VPLTime_t getFileTime() { return stats.file.time; }
protected:
    struct {
        struct {
			u32 pass;
            u32 failed;
            u32 nfiles;
            u64 bytes;
            VPLTime_t time;
        } total;
        struct {
            u64 bytes;
            VPLTime_t time;
        } file;
    } stats;
    int http_statuscode;
    u64 maxfilebytes;
    VPLTime_t maxbytes_delay;
    std::string response;
};

HttpAgent *getHttpAgent();

#endif // _HTTP_AGENT_HPP_
