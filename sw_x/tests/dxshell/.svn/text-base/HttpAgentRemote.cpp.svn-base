#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv

#include "HttpAgentRemote.hpp"
#include "RemoteAgent.hpp"
#include "dx_remote_agent.pb.h"

#include <vpl_net.h>
#include <log.h>
#include <vplex_file.h>
#include <vpl_fs.h>

#include <sstream>
#include "dx_common.h"
#include <iostream>

using namespace igware::dxshell;
const u32 READFILESIZE = 2048;
HttpAgentRemote::HttpAgentRemote()
{
}

HttpAgentRemote::~HttpAgentRemote()
{
}

int HttpAgentRemote::get(const std::string &url, size_t maxbytes, VPLTime_t maxbytes_delay, const char* headers[], size_t numHeaders)
{
    int rv = 0;
    VPLTime_t time_begin, time_end;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;

    memset(&stats.file, 0, sizeof(stats.file));

    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = DX_REMOTE_DEFAULT_PORT;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    httpInput.set_command_type(igware::dxshell::DX_HTTP_GET);
    httpInput.set_url(url);
    httpInput.set_max_bytes(maxbytes);
    httpInput.set_maxbytes_delay(maxbytes_delay);
    httpInput.set_use_media_player( ( (getenv("DX_REMOTE_USE_MEDIAPLAYER") != NULL || getenv("DX_REMOTE_USE_MEDIAPLAY") != NULL) ? true : false));

	if (numHeaders > 0) {
        for (unsigned int i = 0; i < numHeaders; ++i) {
            httpInput.add_headers(std::string(headers[i]));
        }
	}

    std::string params = httpInput.SerializeAsString();
    std::string rawResponse;

    time_begin = VPLTime_GetTimeStamp();
    rv = ragent.send(DX_REQUEST_HTTP_GET, params, rawResponse);
    time_end = VPLTime_GetTimeStamp();
    if (rv != 0) {
        goto end;
    }

    rv = httpOutput.httpagent_response();
    http_statuscode = httpOutput.httpagent_statuscode(); 
    stats.file.bytes += httpOutput.total_bytes();

    if (!httpOutput.ParseFromString(rawResponse)) {
        rv = -1;
		stats.total.failed++;
        goto end;
    }

    //if ( (rv = httpOutput.error_code()) != igware::dxshell::DX_SUCCESS) {
    //    goto end;
    //}

    if (httpOutput.error_code() != igware::dxshell::DX_SUCCESS) {
        rv = -1;
		stats.total.failed++;
        goto end;
    }

    stats.file.bytes += httpOutput.total_bytes();

    if (maxbytes > 0) {
        // Check if the minimum number of bytes were read.
        // If so, consider the attempt a success.
        // Otherwise, unless there's already an error code set, set an error code.
        if (stats.file.bytes >= maxbytes) {
            rv = 0;
		}
        else if (rv == 0) {
            rv = -1;  // FIXME: need a better error code
			stats.total.failed++;
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

int HttpAgentRemote::get_extended(const std::string &url,
                                  size_t maxbytes,
                                  VPLTime_t maxbytes_delay,
                                  const char* headers[],
                                  size_t numHeaders,
                                  const char* file_save_response)
{
    int rv = 0;
    VPLTime_t time_begin, time_end;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;

    memset(&stats.file, 0, sizeof(stats.file));

    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = DX_REMOTE_DEFAULT_PORT;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    httpInput.set_command_type(igware::dxshell::DX_HTTP_GET_EXTENDED);
    httpInput.set_url(url);
    httpInput.set_max_bytes(maxbytes);
    httpInput.set_maxbytes_delay(maxbytes_delay);
    for (unsigned int i = 0; i < numHeaders; ++i) {
        httpInput.add_headers(std::string(headers[i]));
    }

    httpInput.set_file_save_response(std::string(file_save_response));

    std::string params = httpInput.SerializeAsString();
    std::string rawResponse;

    time_begin = VPLTime_GetTimeStamp();
    rv = ragent.send(DX_REQUEST_HTTP_GET, params, rawResponse);
    time_end = VPLTime_GetTimeStamp();
    if (rv != 0) {
        goto end;
    }

    if (!httpOutput.ParseFromString(rawResponse)) {
        rv = -1;
        goto end;
    }

    rv = httpOutput.httpagent_response();
	http_statuscode = httpOutput.httpagent_statuscode(); 
	stats.file.bytes += httpOutput.total_bytes();

    if (httpOutput.error_code() != igware::dxshell::DX_SUCCESS) {
        rv = -1;
		stats.total.failed++;
        goto end;
    }

    stats.file.bytes += httpOutput.total_bytes();

    if (maxbytes > 0) {
        // Check if the minimum number of bytes were read.
        // If so, consider the attempt a success.
        // Otherwise, unless there's already an error code set, set an error code.
        if (stats.file.bytes >= maxbytes) {
            rv = 0;
		}
        else if (rv == 0) {
            rv = -1;  // FIXME: need a better error code
			 stats.total.failed++;
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

// currently we are using environment varible DX_REMOTE_IP, DX_REMOTE_PORT to save ip and port
// it means one set of ip and port is shared by multiple threads which will cause issue if multiple
// threads are calling get_extended function
// get_extended_by_ip_port allow user to pass in specific ip and port, instead of getting them from
// environment members
int HttpAgentRemote::get_extended_by_ip_port(const std::string &url,
                                  size_t maxbytes,
                                  VPLTime_t maxbytes_delay,
                                  const char* headers[],
                                  size_t numHeaders,
                                  const char* file_save_response,
                                  const VPLNet_addr_t& ip_addr,
                                  const VPLNet_port_t& port)
{
    int rv = 0;
    VPLTime_t time_begin, time_end;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;
    std::stringstream sslog;

    memset(&stats.file, 0, sizeof(stats.file));

    RemoteAgent ragent(ip_addr, port);
    sslog << " IP: " << ip_addr << ", Port: " << port;
    LOG_ALWAYS("%s", sslog.str().c_str());

    httpInput.set_command_type(igware::dxshell::DX_HTTP_GET_EXTENDED);
    httpInput.set_url(url);
    httpInput.set_max_bytes(maxbytes);
    httpInput.set_maxbytes_delay(maxbytes_delay);
    for (unsigned int i = 0; i < numHeaders; ++i) {
        httpInput.add_headers(std::string(headers[i]));
    }

    httpInput.set_file_save_response(std::string(file_save_response));

    std::string params = httpInput.SerializeAsString();
    std::string rawResponse;

    time_begin = VPLTime_GetTimeStamp();
    rv = ragent.send(DX_REQUEST_HTTP_GET, params, rawResponse);
    time_end = VPLTime_GetTimeStamp();
    if (rv != 0) {
        goto end;
    }

    if (!httpOutput.ParseFromString(rawResponse)) {
        rv = -1;
        goto end;
    }

    rv = httpOutput.httpagent_response();
	http_statuscode = httpOutput.httpagent_statuscode();
	stats.file.bytes += httpOutput.total_bytes();

    if (httpOutput.error_code() != igware::dxshell::DX_SUCCESS) {
        rv = -1;
		stats.total.failed++;
        goto end;
    }

    if (maxbytes > 0) {
        // Check if the minimum number of bytes were read.
        // If so, consider the attempt a success.
        // Otherwise, unless there's already an error code set, set an error code.
        if (stats.file.bytes >= maxbytes) {
            rv = 0;
		}
        else if (rv == 0) {
            rv = -1;  // FIXME: need a better error code
			stats.total.failed++;
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

int HttpAgentRemote::get_response_back(const std::string &url,
                                  size_t maxbytes,
                                  VPLTime_t maxbytes_delay,
                                  const char* headers[],
                                  size_t numHeaders,
                                  std::string &response)
{
    int rv = 0;
    VPLTime_t time_begin, time_end;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;

    memset(&stats.file, 0, sizeof(stats.file));

    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = DX_REMOTE_DEFAULT_PORT;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    httpInput.set_command_type(igware::dxshell::DX_HTTP_GET_RESPONSE_BACK);
    httpInput.set_url(url);
    httpInput.set_max_bytes(maxbytes);
    httpInput.set_maxbytes_delay(maxbytes_delay);
    for (unsigned int i = 0; i < numHeaders; ++i) {
        httpInput.add_headers(std::string(headers[i]));
    }

    std::string params = httpInput.SerializeAsString();
    std::string rawResponse;

    time_begin = VPLTime_GetTimeStamp();
    rv = ragent.send(DX_REQUEST_HTTP_GET, params, rawResponse);
    time_end = VPLTime_GetTimeStamp();
    if (rv != 0) {
        goto end;
    }

    if (!httpOutput.ParseFromString(rawResponse)) {
        rv = -1;
        goto end;
    }

    rv = httpOutput.httpagent_response();
	http_statuscode = httpOutput.httpagent_statuscode();
	stats.file.bytes += httpOutput.total_bytes();
	response = httpOutput.response();

    if (httpOutput.error_code() != igware::dxshell::DX_SUCCESS) {
        rv = -1;
		stats.total.failed++;
        goto end;
    }

    stats.file.bytes += httpOutput.total_bytes();
   
    if (maxbytes > 0) {
        // Check if the minimum number of bytes were read.
        // If so, consider the attempt a success.
        // Otherwise, unless there's already an error code set, set an error code.
        if (stats.file.bytes >= maxbytes) {
            rv = 0;
		}
        else if (rv == 0) {
            rv = -1;  // FIXME: need a better error code
			stats.total.failed++;
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

int HttpAgentRemote::post(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response)
{
    int rv = 0;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;

    memset(&stats.file, 0, sizeof(stats.file));

    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = DX_REMOTE_DEFAULT_PORT;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    httpInput.set_command_type(igware::dxshell::DX_HTTP_POST);
    httpInput.set_url(url);
    for (unsigned int i = 0; i < numHeaders; ++i) {
        httpInput.add_headers(std::string(headers[i]));
    }

    if (payloadLen == 0 || payload == NULL)
        httpInput.set_payload(std::string(""));
    else
        httpInput.set_payload(std::string(payload));
    
    if (file != NULL) {
        httpInput.set_file(std::string(file));
    }

    std::string params = httpInput.SerializeAsString();
    std::string rawResponse;

    rv = ragent.send(DX_REQUEST_HTTP_GET, params, rawResponse);

    if (rv != 0) {
        goto end;
    }

    if (!httpOutput.ParseFromString(rawResponse)) {
        rv = -1;
        goto end;
    }

    rv = httpOutput.httpagent_response();
	http_statuscode = httpOutput.httpagent_statuscode(); 
    response = httpOutput.response();

    if (httpOutput.error_code() != igware::dxshell::DX_SUCCESS) {
        rv = -1;
    }

end:
    return rv;
}

int HttpAgentRemote::del(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response)
{
    int rv = 0;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;

    memset(&stats.file, 0, sizeof(stats.file));

    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = DX_REMOTE_DEFAULT_PORT;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    httpInput.set_command_type(igware::dxshell::DX_HTTP_DELETE);
    httpInput.set_url(url);
    for (unsigned int i = 0; i < numHeaders; ++i) {
        httpInput.add_headers(std::string(headers[i]));
    }

    if (payloadLen == 0 || payload == NULL)
        httpInput.set_payload(std::string(""));
    else
        httpInput.set_payload(std::string(payload));
    
    if (file != NULL) {
        httpInput.set_file(std::string(file));
    }

    std::string params = httpInput.SerializeAsString();
    std::string rawResponse;

    rv = ragent.send(DX_REQUEST_HTTP_GET, params, rawResponse);

    if (rv != 0) {
        goto end;
    }

    if (!httpOutput.ParseFromString(rawResponse)) {
        rv = -1;
        goto end;
    }

    rv = httpOutput.httpagent_response();
	http_statuscode = httpOutput.httpagent_statuscode(); 
    response = httpOutput.response();

    if (httpOutput.error_code() != igware::dxshell::DX_SUCCESS) {
        rv = -1;
    }

end:
    return rv;
}

int HttpAgentRemote::put(const std::string &url,
                    const char* headers[],
                    size_t numHeaders,
                    const char* payload,
                    size_t payloadLen,
                    const char* file,
                    std::string &response)
{
    int rv = 0;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;

    memset(&stats.file, 0, sizeof(stats.file));

    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = DX_REMOTE_DEFAULT_PORT;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    httpInput.set_command_type(igware::dxshell::DX_HTTP_PUT);
    httpInput.set_url(url);
    for (unsigned int i = 0; i < numHeaders; ++i) {
        httpInput.add_headers(std::string(headers[i]));
    }

    if (payloadLen == 0 || payload == NULL)
        httpInput.set_payload(std::string(""));
    else
        httpInput.set_payload(std::string(payload));
    
    if (file != NULL) {
        httpInput.set_file(std::string(file));
    }

    std::string params = httpInput.SerializeAsString();
    std::string rawResponse;

    rv = ragent.send(DX_REQUEST_HTTP_GET, params, rawResponse);

    if (rv != 0) {
        goto end;
    }

    if (!httpOutput.ParseFromString(rawResponse)) {
        rv = -1;
        goto end;
    }

    rv = httpOutput.httpagent_response();
	http_statuscode = httpOutput.httpagent_statuscode(); 
    response = httpOutput.response();

    if (httpOutput.error_code() != igware::dxshell::DX_SUCCESS) {
        rv = -1;
    }

end:
    return rv;
}
