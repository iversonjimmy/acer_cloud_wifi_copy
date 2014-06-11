//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#include "HttpSvc_Ccd_Handler_iotsdkapi.hpp"

#include "cache.h"
#include "ccdi.hpp"
#include "ccd_features.h"
#include "ccd_storage.hpp"
#include "ccd_util.hpp"
#include "HttpSvc_Ccd_Handler_Helper.hpp"
#include "HttpSvc_Utils.hpp"
#include "HttpStream.hpp"
#include "HttpStream_Helper.hpp"
#include "virtual_device.hpp"
#include "util_mime.hpp"

#include "vpl_conv.h"
#include "vpl_fs.h"
#include "vplu_sstr.hpp"
#include "vplex_file.h"
#include "vplex_http_util.hpp"
#include "vplex_serialization.h"

#include <map>
#include <sstream>
#include <string>
#include <iomanip>

#include "log.h"

HttpSvc::Ccd::Handler_iotsdkapi::Handler_iotsdkapi(HttpStream *hs)
    : Handler(hs), isIoStream(false)
{
    LOG_TRACE("Handler_iotsdkapi[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_iotsdkapi::~Handler_iotsdkapi()
{
    LOG_TRACE("Handler_iotsdkapi[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_iotsdkapi::do_local()
{
    int rv = CCD_OK;

    LOG_INFO("/local API called");

    return rv;
}

int HttpSvc::Ccd::Handler_iotsdkapi::do_remote()
{
    int rv = CCD_OK;

    LOG_INFO("/remote API called");

    return rv;
}

int HttpSvc::Ccd::Handler_iotsdkapi::do_cloud()
{
    int rv = CCD_OK;

    LOG_INFO("/cloud API called");

    return rv;
}

HttpSvc::Ccd::Handler_iotsdkapi::ApiHandlerTable HttpSvc::Ccd::Handler_iotsdkapi::handlerTable;

HttpSvc::Ccd::Handler_iotsdkapi::ApiHandlerTable::ApiHandlerTable() 
{
    handlers["local"] = &Handler_iotsdkapi::do_local;
    handlers["remote"] = &Handler_iotsdkapi::do_remote;
    handlers["cloud"] = &Handler_iotsdkapi::do_cloud;
}

int HttpSvc::Ccd::Handler_iotsdkapi::_Run()
{
    int err = 0;

    LOG_TRACE("Handler_iotsdkapi[%p]: Run", this);

    const std::string &uri = hs->GetUri();

    std::vector<std::string> uri_parts;
    VPLHttp_SplitUri(uri, uri_parts);

    if (uri_parts.size() < 3) {  // 2 is minimum dispatch level.
        LOG_ERROR("Invalid URI:%s", uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    apiVer = uri_parts[0];
    const std::string &apidomain = uri_parts[1];
    if (handlerTable.handlers.find(apidomain) == handlerTable.handlers.end()) {
        LOG_ERROR("Handler_iotsdkapi[%p]: API domain not supported %s", this, uri_parts[1].c_str());
        Utils::SetCompleteResponse(hs, 400);
    }

    return err = (this->*handlerTable.handlers[uri_parts[1]])();
}
