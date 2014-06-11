#include "HttpSvc_Sn_Handler_minidms.hpp"

#include <string>
#include <sstream>
#include <vector>

#include <ccdi.hpp>
#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>
#include <vplex_http_util.hpp>
#include <vpl_net.h>

HttpSvc::Sn::Handler_minidms::Handler_minidms(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_minidms[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Sn::Handler_minidms::~Handler_minidms()
{
    LOG_INFO("Handler_minidms[%p]: Destroyed", this);
}

int HttpSvc::Sn::Handler_minidms::process_deviceinfo()
{
    int rv = 0;
    VPLNet_port_t proxy_agent_port;
    VPLNet_addr_t local_addrs[VPLNET_MAX_INTERFACES];
    VPLNet_addr_t local_netmasks[VPLNET_MAX_INTERFACES];
    std::ostringstream oss;
    std::string json_response;

    rv = VPLNet_GetLocalAddrList(local_addrs, local_netmasks);
    if (rv < 0) {
        LOG_ERROR("Handler_minidms[%p]: VPLNet_GetLocalAddrList() failed: %d", this, rv);
        HttpStream_Helper::SetCompleteResponse(hs, 500, "{\"errMsg\":\"Failed to get local addresses.\"}", "application/json");\
        rv = 0;
        goto end;
    }

    oss << "{\"network_info_list\":[";
    for (int i = 0; i < rv; i++) {
        char c_addr[20];
        char c_mask[20];
        snprintf(c_addr, 20, FMT_VPLNet_addr_t, VAL_VPLNet_addr_t(local_addrs[i]));
        snprintf(c_mask, 20, FMT_VPLNet_addr_t, VAL_VPLNet_addr_t(local_netmasks[i]));
        if (i > 0) {
            oss << ",";
        }
        oss << "{";
        oss << "\"ip\":\"" << c_addr << "\",";
        oss << "\"mask\":\"" << c_mask << "\"";
        oss << "}";
    }
    oss << "],";

    {
        ccd::GetSystemStateInput request;
        ccd::GetSystemStateOutput response;
        request.set_get_network_info(true);
        rv = CCDIGetSystemState(request, response);
        if (rv != 0) {
            LOG_ERROR("Handler_minidms[%p]: CCDIGetSystemState() failed: %d", this, rv);
            HttpStream_Helper::SetCompleteResponse(hs, 500, "{\"errMsg\":\"Internal Server Error.\"}", "application/json");
            rv = 0;
            goto end;
        }
        proxy_agent_port = response.network_info().proxy_agent_port();
    }

    oss << "\"port\":" << proxy_agent_port << "}";

    json_response = oss.str();
    HttpStream_Helper::SetCompleteResponse(hs, 200, json_response.c_str(), "application/json");
end:
    return rv;
}

int HttpSvc::Sn::Handler_minidms::Run()
{
    int err = 0;

    LOG_INFO("Handler_minidms[%p]: Run", this);

    const std::string& uri = hs->GetUri();

    VPLHttp_SplitUri(uri, uri_tokens);

    if (uri_tokens.size() < 2) {
        LOG_ERROR("Handler_minidms[%p]: Unexpected URI: uri %s", this, uri.c_str());
        HttpStream_Helper::SetCompleteResponse(hs, 400, "{\"errMsg\":\"Invalid http request.\"}", "application/json");
        return 0;
    }

    const std::string& uri_objtype = uri_tokens[1];

    if (objJumpTable.handlers.find(uri_objtype) == objJumpTable.handlers.end()) {
        LOG_ERROR("Handler_minidms[%p]: No handler: objtype %s", this, uri_objtype.c_str());
        HttpStream_Helper::SetCompleteResponse(hs, 400, "{\"errMsg\":\"Invalid http request.\"}", "application/json");
        return 0;
    }

    err = (this->*objJumpTable.handlers[uri_objtype])();

    return err;
}

HttpSvc::Sn::Handler_minidms::ObjJumpTable HttpSvc::Sn::Handler_minidms::objJumpTable;

HttpSvc::Sn::Handler_minidms::ObjJumpTable::ObjJumpTable()
{
    handlers["deviceinfo"] = &Handler_minidms::process_deviceinfo;
}
