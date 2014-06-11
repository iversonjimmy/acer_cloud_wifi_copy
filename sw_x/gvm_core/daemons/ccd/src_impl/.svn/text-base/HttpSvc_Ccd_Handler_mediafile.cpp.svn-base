#include "HttpSvc_Ccd_Handler_mediafile.hpp"

#include "HttpSvc_Utils.hpp"
#include "HttpSvc_HsToTsAdapter.hpp"

#include <gvm_errors.h>
#include <log.h>
#include <HttpStream.hpp>

#include <vpl_conv.h>
#include <vplex_http_util.hpp>

#include <string>
#include <vector>

HttpSvc::Ccd::Handler_mediafile::Handler_mediafile(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_mediafile[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_mediafile::~Handler_mediafile()
{
    LOG_INFO("Handler_mediafile[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_mediafile::_Run()
{
    int err = 0;

    LOG_INFO("Handler_mediafile[%p]: Run", this);

    const std::string &uri = hs->GetUri();

    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // CheckReqHeaders() logged msg and set response.
        return err;
    }

    std::vector<std::string> uri_tokens;
    // Get device ID & UUID "/mediafile/tag/<deviceid>/<uuid>
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 4 || uri_tokens[1] != "tag" || uri_tokens[3].empty()) {
        LOG_ERROR("Handler_mediafile[%p]: Unexpected URI: %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;  // reset error
    }

    u64 deviceId = VPLConv_strToU64(uri_tokens[2].c_str(), NULL, 10);
    if (deviceId == 0) {
        LOG_ERROR("Handler_mediafile[%p]: Failed to parse deviceId from URI: %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;  // reset error
    }
    hs->SetDeviceId(deviceId);

    hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
    hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);

    {
        HsToTsAdapter *adapter = new (std::nothrow) HsToTsAdapter(hs);
        if (!adapter) {
            LOG_ERROR("Handler_mediafile[%p]: No memory to create HsToTsAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }

    return err;
}

