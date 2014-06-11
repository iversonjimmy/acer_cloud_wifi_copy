#include "HttpSvc_Ccd_Handler_cmd.hpp"

#include "HttpSvc_HsToTsAdapter.hpp"
#include "HttpSvc_Utils.hpp"

#include <gvm_errors.h>
#include <gvm_misc_utils.h>
#include <HttpStream.hpp>
#include <log.h>

#include <vplex_http_util.hpp>

#include <string>

HttpSvc::Ccd::Handler_cmd::Handler_cmd(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_cmd[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_cmd::~Handler_cmd()
{
    LOG_INFO("Handler_cmd[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_cmd::_Run()
{
    int err = 0;

    const std::string &uri = hs->GetUri();
    std::vector<std::string> uri_tokens;
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 2) {
        LOG_ERROR("Handler_cmd[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    const std::string &deviceIdStr = uri_tokens[1];

    u64 deviceId = Util_ParseStrictDeviceId(deviceIdStr.c_str());
    if (deviceId == 0) {
        LOG_ERROR("Handler_cmd[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    hs->SetDeviceId(deviceId);

    hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
    hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);

    {
        HsToTsAdapter *adapter = new (std::nothrow) HsToTsAdapter(hs);
        if (!adapter) {
            LOG_ERROR("Handler_cmd[%p]: No memory to create HsToTsAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }

    return err;
}

