#include "DxRemoteHTTPAgent.hpp"
#include "../../dxshell/HttpAgent.hpp"
#include <sstream>
#include <vector>
#include "log.h"

DxRemoteHTTPAgent::DxRemoteHTTPAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) : IDxRemoteAgent(skt, buf, pktSize)
{
}

DxRemoteHTTPAgent::~DxRemoteHTTPAgent()
{
}

int DxRemoteHTTPAgent::doAction()
{
    igware::dxshell::ErrorCode errorCode = igware::dxshell::DX_ERR_UNEXPECTED_ERROR;
    igware::dxshell::HttpGetInput httpInput;
    igware::dxshell::HttpGetOutput httpOutput;
    httpOutput.set_error_code(igware::dxshell::DX_ERR_UNEXPECTED_ERROR);
    httpOutput.set_total_bytes(0);

    do
    {
        if (!httpInput.ParseFromArray(recvBuf, recvBufSize)) {
            errorCode = igware::dxshell::DX_ERR_BAD_REQUEST;
            break;
        }

        if (httpInput.command_type() == igware::dxshell::DX_HTTP_GET ||
            httpInput.command_type() == igware::dxshell::DX_HTTP_GET_EXTENDED ||
            httpInput.command_type() == igware::dxshell::DX_HTTP_GET_RESPONSE_BACK ||
            httpInput.command_type() == igware::dxshell::DX_HTTP_PUT ||
            httpInput.command_type() == igware::dxshell::DX_HTTP_POST ||
            httpInput.command_type() == igware::dxshell::DX_HTTP_DELETE) {
                errorCode = this->handler(httpInput, httpOutput);
        }
        else {
            errorCode = igware::dxshell::DX_ERR_UNKNOWN_REQUEST_TYPE;
        }
    } while (false);

    httpOutput.set_error_code(errorCode);

    LOG_INFO("Serialize Response for httpagent_statuscode: %d", httpOutput.httpagent_statuscode());
    LOG_INFO("Serialize Response for httpagent_response: %d", httpOutput.httpagent_response());
    LOG_INFO("Serialize Response for http error_code: %d", httpOutput.error_code());
    LOG_INFO("Serialize Response for response: %s", httpOutput.response().c_str());

    response = httpOutput.SerializeAsString();

    return 0;
}

igware::dxshell::ErrorCode DxRemoteHTTPAgent::handler(igware::dxshell::HttpGetInput &httpInput, igware::dxshell::HttpGetOutput &httpOutput)
{
    igware::dxshell::ErrorCode errorCode = igware::dxshell::DX_ERR_IO_HTTPAGENT_ERROR;
    int rc = -1;
    std::string url, payload, file, response, file_save_response;
    unsigned int maxbytes = 0;
    VPLTime_t maxbytes_delay = 0;
    HttpAgent *agent = NULL;
    std::vector<std::string> vHeaders;
    const char **headers = NULL;
    httpOutput.set_httpagent_response(rc);

    do {
        if (httpInput.headers_size() > 0) {
            headers = new (std::nothrow) const char *[httpInput.headers_size()];
            if ((headers == NULL) && (httpInput.command_type() != igware::dxshell::DX_HTTP_GET)) {
                errorCode = igware::dxshell::DX_ERR_IO_ERROR;
                LOG_ERROR("DxRemoteHTTPAgent: Empty headers, return error!");
                break;
            }

            for (int i = 0; i < httpInput.headers_size(); ++i) {
                vHeaders.push_back(std::string(httpInput.headers(i)));
            }

            for (int i = 0; i < httpInput.headers_size(); ++i) {
                headers[i] = vHeaders[i].c_str();
            }
        }

        url = httpInput.url();
        payload = httpInput.payload();
        file = httpInput.file();
        file_save_response = httpInput.file_save_response();
        maxbytes = httpInput.max_bytes();
        maxbytes_delay = httpInput.maxbytes_delay();

        agent = getHttpAgent();

        if (httpInput.command_type() == igware::dxshell::DX_HTTP_GET) {
            LOG_INFO("DX_HTTP_GET");
            if (httpInput.headers_size() > 0) {
                rc = agent->get(url, maxbytes, maxbytes_delay, headers, httpInput.headers_size());
            } else {
                rc = agent->get(url, maxbytes, maxbytes_delay);
            }
        }
        else if (httpInput.command_type() == igware::dxshell::DX_HTTP_GET_EXTENDED) {
            LOG_INFO("DX_HTTP_GET_EXTENDED");
            rc = agent->get_extended(url, maxbytes, maxbytes_delay, headers, httpInput.headers_size(), file_save_response.c_str());
        }
        else if (httpInput.command_type() == igware::dxshell::DX_HTTP_GET_RESPONSE_BACK) {
            LOG_INFO("DX_HTTP_GET_RESPONSE_BACK");
            rc = agent->get_response_back(url, maxbytes, maxbytes_delay, headers, httpInput.headers_size(), response);
        }
        else if (httpInput.command_type() == igware::dxshell::DX_HTTP_PUT) {
            LOG_INFO("DX_HTTP_PUT");
            rc = agent->put(url, headers, httpInput.headers_size(), (payload.empty() ? NULL: payload.c_str()), payload.size(), (file.empty() ? NULL : file.c_str()), response);
        }
        else if (httpInput.command_type() == igware::dxshell::DX_HTTP_POST) {
            LOG_INFO("DX_HTTP_POST");
            rc = agent->post(url, headers, httpInput.headers_size(), (payload.empty() ? NULL: payload.c_str()), payload.size(), (file.empty() ? NULL : file.c_str()), response);
        }
        else if (httpInput.command_type() == igware::dxshell::DX_HTTP_DELETE) {
            LOG_INFO("DX_HTTP_DELETE");
            rc = agent->del(url, headers, httpInput.headers_size(), (payload.empty() ? NULL: payload.c_str()), payload.size(), (file.empty() ? NULL : file.c_str()), response);
        }

        if (rc == 0) {
            errorCode = igware::dxshell::DX_SUCCESS;
            httpOutput.set_total_bytes(agent->getTotalBytes());
        }
        else {
            LOG_ERROR("DxRemoteHTTPAgent: http request failed, %s", url.c_str());
            httpOutput.set_total_bytes(0);
        }

        httpOutput.set_httpagent_statuscode(agent->getlastHttpStatusCode());
        httpOutput.set_response(response);
        httpOutput.set_httpagent_response(rc);
    } while (false);

    if (headers != NULL) {
        delete[] headers;
        headers = NULL;
    }

    if (agent != NULL) {
        delete agent;
        agent = NULL;
    }
    return errorCode;
}
