#include "HttpSvc_Sn_Handler_cmd.hpp"

#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>

#include <vplex_powerman.h>

#include <sstream>
#include <string>

HttpSvc::Sn::Handler_cmd::Handler_cmd(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_cmd[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Sn::Handler_cmd::~Handler_cmd()
{
    LOG_INFO("Handler_cmd[%p]: Destroyed", this);
}

int HttpSvc::Sn::Handler_cmd::Run()
{
    LOG_INFO("Handler_cmd[%p]: Run", this);

    static const std::string keepAwakeToken = "keepAwake";
    if (hs->GetUri().find(keepAwakeToken) != std::string::npos) {
        LOG_INFO("Handler_cmd[%p]: Received keep awake request", this);
        VPLPowerMan_PostponeSleep(VPL_POWERMAN_ACTIVITY_SERVING_DATA, NULL);
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return 0;
}

