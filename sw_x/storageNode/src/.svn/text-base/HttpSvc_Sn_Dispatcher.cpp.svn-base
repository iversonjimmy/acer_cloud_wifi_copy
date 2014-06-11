#include "HttpSvc_Sn_Dispatcher.hpp"

#include "HttpSvc_Sn_Handler_cmd.hpp"
#include "HttpSvc_Sn_Handler_mediafile.hpp"
#include "HttpSvc_Sn_Handler_minidms.hpp"
#include "HttpSvc_Sn_Handler_mm.hpp"
#include "HttpSvc_Sn_Handler_rexe.hpp"
#include "HttpSvc_Sn_Handler_rf.hpp"
#include "HttpSvc_Sn_Handler_vcs_archive.hpp"

#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>

#include <vplex_http_util.hpp>

#include <sstream>

typedef HttpSvc::Sn::Handler *(*handler_creator_fn)(HttpStream *hs);

template <class T>
HttpSvc::Sn::Handler *createHandler(HttpStream *hs)
{
    return new (std::nothrow) T(hs);
}

class SnHandlerCreatorJumpTable {
public:
    SnHandlerCreatorJumpTable() {
        creators["cmd"]       = &createHandler<HttpSvc::Sn::Handler_cmd>;
        creators["mediafile"] = &createHandler<HttpSvc::Sn::Handler_mediafile>;
        creators["media_rf"]  = &createHandler<HttpSvc::Sn::Handler_rf>;
        creators["minidms"]   = &createHandler<HttpSvc::Sn::Handler_minidms>;
        creators["mm"]        = &createHandler<HttpSvc::Sn::Handler_mm>;
        creators["rexe"]      = &createHandler<HttpSvc::Sn::Handler_rexe>;
        creators["rf"]        = &createHandler<HttpSvc::Sn::Handler_rf>;
        creators["vcs_archive"] = &createHandler<HttpSvc::Sn::Handler_vcs_archive>;
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
static SnHandlerCreatorJumpTable snHandlerCreatorJumpTable;

// class method
int HttpSvc::Sn::Dispatcher::Dispatch(HttpStream *hs)
{
    const std::string &uri = hs->GetUri();
    if (uri.empty()) {  // error
        return CCD_ERROR_PARSE_CONTENT;
    }

    std::vector<std::string> uri_parts;
    VPLHttp_SplitUri(hs->GetUri(), uri_parts);
    if (uri_parts.size() < 1 || uri_parts[0].empty()) {
        LOG_ERROR("Dispatcher: Malformed URI %s", hs->GetUri().c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Malformed URI " << hs->GetUri() << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    const std::string &serviceName = uri_parts[0];

    if (snHandlerCreatorJumpTable.find(serviceName) == snHandlerCreatorJumpTable.end()) {
        LOG_ERROR("Dispatcher: Unknown service %s", serviceName.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unknown service " << serviceName << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    Handler *handler = (snHandlerCreatorJumpTable[serviceName])(hs);
    if (handler == NULL) {
        LOG_ERROR("Dispatcher: Not enough memory");
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;
    }

    LOG_INFO("Dispatcher: Dispatching HttpStream[%p]: %s %s", hs, hs->GetMethod().c_str(), uri.c_str());


    // TODO: bug 16382: This only handles some cases.  It doesn't handle the case where the vssServer
    //   gets deallocated while handler is still within Run().
    //   We really need to ensure that it remains valid for the duration of the Run() call.
    if (handler->IsVssServerNull()) {
        LOG_ERROR("Handler[%p]: vssServer is NULL", handler);
        HttpStream_Helper::SetCompleteResponse(hs, 500,
                JSON_ERRMSG("Storage Node is not currently running."),
                "application/json");
        return 0;
    }

    int err = handler->Run();
    if (err) {
        LOG_ERROR("Dispatcher: Handler failed: err %d", err);
    }
    else {
        // response set by Handler::Run()
        LOG_INFO("Dispatcher: HttpStream[%p] outcome: status %d", hs, hs->GetStatusCode());
    }

    delete handler;

    return err;
}

