#include "HttpSvc_Ccd_Dispatcher.hpp"

#include "HttpSvc_Ccd_Handler_clouddoc.hpp"
#include "HttpSvc_Ccd_Handler_cmd.hpp"
#include "HttpSvc_Ccd_Handler_ds.hpp"
#include "HttpSvc_Ccd_Handler_mediafile.hpp"
#include "HttpSvc_Ccd_Handler_minidms.hpp"
#include "HttpSvc_Ccd_Handler_mm.hpp"
#include "HttpSvc_Ccd_Handler_picstream.hpp"
#include "HttpSvc_Ccd_Handler_rexe.hpp"
#include "HttpSvc_Ccd_Handler_rf.hpp"
#include "HttpSvc_Ccd_Handler_share.hpp"
#include "HttpSvc_Ccd_Handler_iotsdkapi.hpp"
#include "HttpSvc_Utils.hpp"

#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <log.h>

#include <scopeguard.hpp>
#include <vplex_http_util.hpp>
#include "ccd_features.h"

typedef HttpSvc::Ccd::Handler *(*handler_creator_fn)(HttpStream *hs);

template <class T>
HttpSvc::Ccd::Handler *createHandler(HttpStream *hs)
{
    return new (std::nothrow) T(hs);
}

class HandlerCreatorJumpTable {
public:
    HandlerCreatorJumpTable() {
        creators["clouddoc"]  = &createHandler<HttpSvc::Ccd::Handler_clouddoc>;
        creators["cmd"]       = &createHandler<HttpSvc::Ccd::Handler_cmd>;
        creators["ds"]        = &createHandler<HttpSvc::Ccd::Handler_ds>;
        creators["mediafile"] = &createHandler<HttpSvc::Ccd::Handler_mediafile>;
        creators["media_rf"]  = &createHandler<HttpSvc::Ccd::Handler_rf>;
        creators["minidms"]   = &createHandler<HttpSvc::Ccd::Handler_minidms>;
        creators["mm"]        = &createHandler<HttpSvc::Ccd::Handler_mm>;
        creators["rexe"]      = &createHandler<HttpSvc::Ccd::Handler_rexe>;
        creators["rf"]        = &createHandler<HttpSvc::Ccd::Handler_rf>;
        creators["picstream"] = &createHandler<HttpSvc::Ccd::Handler_picstream>;
        creators["share"]     = &createHandler<HttpSvc::Ccd::Handler_share>;
#if CCD_ENABLE_IOT_SDK_HTTP_API
        creators["v1"]        = &createHandler<HttpSvc::Ccd::Handler_iotsdkapi>;
#endif
        // NOTE: If you add a new handler, please add documentation to
        //   http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface#Available_Services
    }
    std::map<std::string, handler_creator_fn>::const_iterator find(const std::string serviceName) const {
        return creators.find(serviceName);
    }
    std::map<std::string, handler_creator_fn>::const_iterator end() const {
        return creators.end();
    }
    handler_creator_fn &operator[](const std::string &serviceName) {
        return creators[serviceName];
    }
private:
    std::map<std::string, handler_creator_fn> creators;
};
static HandlerCreatorJumpTable handlerCreatorJumpTable;

// class method
int HttpSvc::Ccd::Dispatcher::Dispatch(HttpStream *hs)
{
    const std::string &uri = hs->GetUri();
    if (uri.empty()) {  // error
        return CCD_ERROR_PARSE_CONTENT;
    }

    std::vector<std::string> uri_parts;
    VPLHttp_SplitUri(hs->GetUri(), uri_parts);
    if (uri_parts.size() < 1 || uri_parts[0].empty() || *uri.rbegin() == '/') {
        LOG_ERROR("Dispatcher: Unexpected URI %s", hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    const std::string &serviceName = uri_parts[0];

    if (handlerCreatorJumpTable.find(serviceName) == handlerCreatorJumpTable.end()) {
        LOG_ERROR("Dispatcher: No handler for service %s", serviceName.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    Handler *handler = (handlerCreatorJumpTable[serviceName])(hs);
    if (handler == NULL) {
        LOG_ERROR("Dispatcher: No memory to create handler for service %s", serviceName.c_str());
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }
    ON_BLOCK_EXIT(deleteObj<HttpSvc::Ccd::Handler>, handler);

    LOG_INFO("Dispatcher: Dispatching HttpStream[%p]: %s %s", hs, hs->GetMethod().c_str(), uri.c_str());

    int err = handler->Run();
    if (err) {
        LOG_ERROR("Dispatcher: Handler failed: err %d", err);
    }
    else {
        // response set by Handler::Run()
        LOG_INFO("Dispatcher: HttpStream[%p] outcome: status %d", hs, hs->GetStatusCode());
    }
    return err;
}
