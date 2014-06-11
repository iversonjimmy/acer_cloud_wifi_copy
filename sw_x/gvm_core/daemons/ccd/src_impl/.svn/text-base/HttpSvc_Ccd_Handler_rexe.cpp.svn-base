#include "HttpSvc_Ccd_Handler_rexe.hpp"

#include "HttpSvc_Utils.hpp"
#include "HttpSvc_HsToTsAdapter.hpp"
#include "HttpSvc_Ccd_Handler_Helper.hpp"

#include <gvm_errors.h>
#include <log.h>
#include <HttpStream.hpp>

#include <vpl_conv.h>
#include <vplex_http_util.hpp>

#include <string>
#include <vector>

HttpSvc::Ccd::Handler_rexe::Handler_rexe(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_rexe[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_rexe::~Handler_rexe()
{
    LOG_INFO("Handler_rexe[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_rexe::_Run()
{
    int err = 0;

    LOG_INFO("Handler_rexe[%p]: Run", this);

    const std::string &uri = hs->GetUri();

    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // CheckReqHeaders() logged msg and set response.
        return err;
    }

    std::vector<std::string> uri_tokens;
    // Get device_id from URI. ("rexe/execute/<deviceid>/<executable name>/<app key>/<minimal version number>")
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 6 || uri_tokens[1] != "execute") {
        LOG_ERROR("Handler_rexe[%p]: Unexpected URI: %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;  // reset error
    }

    u64 deviceId = VPLConv_strToU64(uri_tokens[2].c_str(), NULL, 10);
    if (deviceId == 0) {
        LOG_ERROR("Handler_rexe[%p]: Failed to parse deviceId from URI: %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;  // reset error
    }
    hs->SetDeviceId(deviceId);

    LOG_ALWAYS("Handler_rexe[%p]: URI: %s", this, uri.c_str());
    LOG_ALWAYS("Handler_rexe[%p]: Target device_id: "FMTu64, this, deviceId);

    hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
    hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);

    err = Handler_Helper::ForwardToServerCcd(hs);
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: ForwardToServerCcd() failed, rv = %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        err = 0;
    }

    return err;
}
