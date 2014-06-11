#include "HttpSvc_Ccd_Handler_ds.hpp"

#include "HttpSvc_HsToHttpAdapter.hpp"
#include "HttpSvc_Utils.hpp"

#include "vcs_utils.hpp"

#include <HttpStream.hpp>
#include <log.h>

#include <vpl_conv.h>
#include <vplex_http_util.hpp>
#include <vplex_vs_directory_service_types.pb.h>

#include <cassert>
#include <sstream>

HttpSvc::Ccd::Handler_ds::Handler_ds(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_ds[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_ds::~Handler_ds()
{
    LOG_INFO("Handler_ds[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_ds::_Run()
{
    int err = 0;

    LOG_INFO("Handler_ds[%p]: Run", this);

    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // errmsg logged and response set by CheckReqHeaders()
        return 0;  // reset error;
    }

    const std::string &uri = hs->GetUri();
    std::vector<std::string> uri_tokens;
    VPLHttp_SplitUri(uri, uri_tokens);
    // URI should look like "/ds/datasetchanges/<datasetId>"
    if (uri_tokens.size() < 3 || uri_tokens[1].compare("datasetchanges") != 0 || uri_tokens[2].empty()) {
        LOG_ERROR("Handler_ds[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    u64 datasetId=0;
    {
        const std::string &datasetIdStr = uri_tokens[2];
        datasetId = VPLConv_strToU64(datasetIdStr.c_str(), NULL, 10);
    }

    vplex::vsDirectory::DatasetDetail datasetDetail;
    err = VCS_getDatasetDetail(hs->GetUserId(), datasetId, datasetDetail);
    if (err < 0) {
        LOG_ERROR("Handler_ds[%p]: Unknown dataset: did "FMTu64, this, datasetId);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    // hs->GetUri() looks like
    // /ds/datasetchanges/<datasetId>?changeSince=45&max=10
    // we want to construct a url that looks like
    // https://host:port/vcs/datasetchanges/<datasetId>?changeSince=45&max=10
    
    std::string newuri;
    {
        static const std::string prefixPattern = "/ds";

        std::ostringstream oss;
        oss << "https://"
            << datasetDetail.storageclusterhostname() << ":" << datasetDetail.storageclusterport()
            << "/vcs";
        oss.write(uri.data() + prefixPattern.size(), uri.size() - prefixPattern.size());
        newuri.assign(oss.str());
    }

    hs->SetUri(newuri);
    Utils::SetLocalDeviceIdInReq(hs);
    hs->RemoveReqHeader("Host");

    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hs);
        if (!adapter) {
            LOG_ERROR("Handler_clouddoc[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }

    return err;
}

