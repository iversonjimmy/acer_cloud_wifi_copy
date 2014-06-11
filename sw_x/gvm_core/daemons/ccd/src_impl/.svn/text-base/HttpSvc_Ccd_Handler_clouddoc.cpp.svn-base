#include "HttpSvc_Ccd_Handler_clouddoc.hpp"

#include "HttpSvc_HsToHttpAdapter.hpp"
#include "HttpSvc_Utils.hpp"

#include "cache.h"
#include "JsonHelper.hpp"
#include "vcs_proxy.hpp"
#include "vcs_utils.hpp"
#include "virtual_device.hpp"

#include <cJSON2.h>
#include <HttpStream.hpp>
#include <HttpStringStream.hpp>
#include <InStringStream.hpp>
#include <OutStringStream.hpp>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_conv.h>
#include <vpl_fs.h>
#include <vplex_http_util.hpp>

#include <cassert>
#include <new>
#include <sstream>
#include <string>

// http://www.ctbg.acer.com/wiki/index.php/CCD_Support_for_CloudDoc_v1.1

static const std::string CloudDoc_DatasetName = "Cloud Doc";

HttpSvc::Ccd::Handler_clouddoc::Handler_clouddoc(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_clouddoc[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_clouddoc::~Handler_clouddoc()
{
    LOG_INFO("Handler_clouddoc[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_clouddoc::_Run()
{
    int err = 0;

    LOG_INFO("Handler_clouddoc[%p]: Run", this);

    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // errmsg logged and response set by CheckReqHeaders()
        return 0;  // reset error
    }

    const std::string &uri = hs->GetUri();
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 2) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    assert(uri_tokens[0].compare("clouddoc") == 0);
    const std::string &uri_objtype = uri_tokens[1];

    if (objJumpTable.handlers.find(uri_objtype) == objJumpTable.handlers.end()) {
        LOG_ERROR("Handler_clouddoc[%p]: No handler: objtype %s", this, uri_objtype.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return err = (this->*objJumpTable.handlers[uri_objtype])();
    // response set by obj handler
}

int HttpSvc::Ccd::Handler_clouddoc::run_async()
{
    const std::string &method = hs->GetMethod();
    if (asyncMethodJumpTable.handlers.find(method) == asyncMethodJumpTable.handlers.end()) {
        LOG_ERROR("Handler_clouddoc[%p]: No handler: method %s", this, method.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return (this->*asyncMethodJumpTable.handlers[method])();
    // response set by method handler
}

int HttpSvc::Ccd::Handler_clouddoc::run_dir()
{
    int err = 0;

    // only acceptable method is GET
    if (hs->GetMethod().compare("GET") != 0) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 405);
        return 0;
    }

    std::string uri_query;
    {
        size_t qchar = hs->GetUri().find('?');
        if (qchar != std::string::npos) {
            uri_query.assign(hs->GetUri(), qchar + 1, std::string::npos);  // +1 to skip '?'
        }
    }

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        // errmsg logged and response set by getDatasetId()
        return 0;
    }

    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }

    // hs->GetUri() looks like /clouddoc/dir/?max=10
    // we want to construct a url that looks like
    // https://host:port/vcs/dir/<datasetid>/?max=10&prefixServiceName=clouddoc

    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/dir/" << datasetId << "/";
        if (uri_query.empty()) {
            oss << "?prefixServiceName=clouddoc";
        }
        else {
            oss << "?" << uri_query << "&prefixServiceName=clouddoc";
        }
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

int HttpSvc::Ccd::Handler_clouddoc::run_file()
{
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("file") == 0);

    const std::string &method = hs->GetMethod();
    if (fileMethodJumpTable.handlers.find(method) == fileMethodJumpTable.handlers.end()) {
        LOG_ERROR("Handler_clouddoc[%p]: No handler: method %s", this, method.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return (this->*fileMethodJumpTable.handlers[method])();
    // response set by method handler
}

int HttpSvc::Ccd::Handler_clouddoc::run_filemetadata()
{
    int err = 0;

    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("filemetadata") == 0);

    // only acceptable method is GET
    if (hs->GetMethod().compare("GET") != 0) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 405);
        return 0;
    }

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        // errmsg logged and response set by getDatasetId()
        return 0;
    }

    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }

    // hs->GetUri() looks like /clouddoc/filemetadata/a/b/c?compId=123&revision=45
    // we want to construct a url that looks like
    // https://host:port/vcs/filemetadata/<datasetid>/a/b/c?compId=123&revision=45

    static const std::string prefixPattern = "/clouddoc/filemetadata";

    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/filemetadata/" << datasetId;
        oss.write(hs->GetUri().data() + prefixPattern.size(), hs->GetUri().size() - prefixPattern.size());
        oss << "&prefixServiceName=clouddoc";
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

int HttpSvc::Ccd::Handler_clouddoc::run_preview()
{
    int err = 0;

    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("preview") == 0);

    // only acceptable method is GET
    if (hs->GetMethod().compare("GET") != 0) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 405);
        return 0;
    }

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        // errmsg logged and response set by getDatasetId()
        return 0;
    }

    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }

    // hs->GetUri() looks like
    // /clouddoc/preview/12748928/29271306316/a/b/c?compId=578173332&revision=1
    // we want to construct a url that looks like
    // https://host:port/vcs/preview/12748928/29271306316/a/b/c?compId=578173332&revision=1

    static const std::string prefixPattern = "/clouddoc";

    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs";
        oss.write(hs->GetUri().data() + prefixPattern.size(), hs->GetUri().size() - prefixPattern.size());
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

int HttpSvc::Ccd::Handler_clouddoc::run_conflict()
{
    int err = 0;

    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("conflict") == 0);

    // only acceptable method is GET
    if (hs->GetMethod().compare("GET") != 0) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 405);
        return 0;
    }

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    std::string response;
    std::string docname;
    u64 compid = 0;
    response.clear();
    // Process parameters
    {
        std::vector<std::string> uri_tokens;

        // Get docname
        VPLHttp_SplitUri(hs->GetUri(), uri_tokens);
        docname.clear();
        docname.assign(uri_tokens[2].c_str());
        for(u32 i = 3; i < uri_tokens.size(); i++) {
            docname += "/" + uri_tokens[i];
        }

        std::string compIdStr;
        err = hs->GetQuery("compId", compIdStr);
        if (err) {
            LOG_ERROR("Handler_clouddoc[%p]: Error when obtaining compId: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        else {
            std::string *compIdStrPtr = &compIdStr;
            LOG_INFO("compId=%s", compIdStrPtr->c_str());
            compid = (u64)VPLConv_strToU64(compIdStrPtr->c_str(), NULL, 10);
        }
    }

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    // Should check local DB here
    s32 rv = 0;
    u32 count = 0;
    rv = CountUnfinishedJobsinDB(docname, compid, count);

    if (count > 0) {
        // Found ongoing/pending upload/download jobs in DB
        // i.e. The version in VCS and the local version could be different
        response = std::string("{\"conflict\":true}");
    } else {
        // Request filemetadata to VCS to check if there are multiple revisions
        u64 datasetId = 0;
        err = getDatasetId(datasetId);
        if (err < 0) {
            // errmsg logged and response set by getDatasetId()
            return 0;
        }

        std::string host_port;
        err = getServer(host_port);
        if (err < 0) {
            // errmsg logged and response set by getServer()
            return 0;
        }

        // hs->GetUri() looks like /clouddoc/conflict/a/b/c?compId=123
        // we want to construct a url that looks like
        // https://host:port/vcs/filemetadata/<datasetid>/a/b/c?compId=123
        
        static const std::string prefixPattern = "/clouddoc/conflict";

        std::string newuri;
        {
            std::ostringstream oss;
            oss << "https://" << host_port << "/vcs/filemetadata/" << datasetId;
            oss.write(hs->GetUri().data() + prefixPattern.size(), hs->GetUri().size() - prefixPattern.size());
            oss << "&prefixServiceName=clouddoc";
            newuri.assign(oss.str());
        }

        // CCD needs to get the corresponding file metadata

        std::string reqhdr;
        {
            std::ostringstream oss;
            oss << "GET " << newuri << " HTTP/1.1\r\n";
            {
                std::string x_ac_userId;
                hs->GetReqHeader(Utils::HttpHeader_ac_userId, x_ac_userId);
                oss << Utils::HttpHeader_ac_userId << ": " << x_ac_userId << "\r\n";
            }
            {
                std::string x_ac_sessionHandle;
                hs->GetReqHeader(Utils::HttpHeader_ac_sessionHandle, x_ac_sessionHandle);
                oss << Utils::HttpHeader_ac_sessionHandle << ": " << x_ac_sessionHandle << "\r\n";
            }
            {
                std::string x_ac_serviceTicket;
                hs->GetReqHeader(Utils::HttpHeader_ac_serviceTicket, x_ac_serviceTicket);
                oss << Utils::HttpHeader_ac_serviceTicket << ": " << x_ac_serviceTicket << "\r\n";
            }
            oss << Utils::HttpHeader_ac_deviceId << ": " << VirtualDevice_GetDeviceId() << "\r\n";
            //oss << "Content-Length: " << 0 << "\r\n";
            oss << "\r\n";
            reqhdr.assign(oss.str());
        }

        InStringStream iss(reqhdr);
        OutStringStream oss;
        HttpStringStream *hss = new (std::nothrow) HttpStringStream(&iss, &oss);
        if (!hss) {
            LOG_ERROR("Handler_clouddoc[%p]: No memory to create HttpStringStream obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }

        {
            HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hss);
            if (!adapter) {
                LOG_ERROR("Handler_clouddoc[%p]: No memory to create HsToHttpAdapter obj", this);
                Utils::SetCompleteResponse(hs, 500);
                delete hss;
                return 0;
            }
            err = adapter->Run();
            delete adapter;
        }

        std::string output = oss.GetOutput();
        delete hss;
        // Sample response:
        //
        // HTTP/1.1 200 OK
        // Content-Length: 431
        // Content-Type: text/plain;charset=UTF-8
        // Date: Wed, 10 Jul 2013 22:05:52 GMT
        // Server: ATS/3.1.0-unstable
        //
        // {"name":"26813513394/C/Users/vincent/Desktop/QQQ.docx",
        //  "compId":2077351638,
        //  "numOfRevisions":1,
        //  "originDevice":26813513394,
        //  "revisionList":[{"revision":1,
        //                   "size":13263,
        //                   "lastChanged":1372782685,
        //                   "updateDevice":26813513394,
        //                   "previewUri":"/clouddoc/preview/13648446/26813513394/C/Users/vincent/Desktop/QQQ.docx?compId=2077351638&revision=1",
        //                   "downloadUrl":"https://acercloud-lab18-1351282429.s3.amazonaws.com/13648446/7526789162506097042"}]}

        u32 index = output.find("{\"");
        if (index != std::string::npos) {
            output.replace(0, index, "");
        } else {
            LOG_ERROR("filemetadata response is not JSON: %s", output.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        cJSON2 *json = cJSON2_Parse(output.c_str());
        if (json == NULL) {
            LOG_ERROR("error on parsing filemetadata request body as JSON: %s", output.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        } else {
            cJSON2 *json_revisionList = cJSON2_GetObjectItem(json, "revisionList");
            if (!json_revisionList) {
                Utils::SetCompleteResponse(hs, 400);
                cJSON2_Delete(json);
                return 0;
            }
            if (json_revisionList->type != cJSON2_Array) {
                Utils::SetCompleteResponse(hs, 400);
                cJSON2_Delete(json);
                return 0;
            }
            int revisionListSize = cJSON2_GetArraySize(json_revisionList);
            if (revisionListSize > 1) {
                response = std::string("{\"conflict\":true, \"revisionList\":");
                char *test = cJSON2_PrintUnformatted(json_revisionList);
                std::string revisionList;
                revisionList.assign(test);
                response = response + revisionList + "}";
            } else {
                response = std::string("{\"conflict\":false}");
            }
            // Parse complete
            cJSON2_Delete(json);
        }
    }
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (response.empty()) {
        response = std::string("{\"conflict\":false}");
        Utils::SetCompleteResponse(hs, 200, response, Utils::Mime_ApplicationJson);
    } else {
        Utils::SetCompleteResponse(hs, 200, response, Utils::Mime_ApplicationJson);
    }
    return err;
}

int HttpSvc::Ccd::Handler_clouddoc::run_copyback()
{
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("copyback") == 0);

    // only acceptable method is POST
    if (hs->GetMethod().compare("POST") != 0) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 405);
        return 0;
    }

    // Get DatasetId, datasetLocation
    s32 rv = -1;
    u64 datasetId = 0;
    std::string response;
    std::string datasetLocation;
    {
        ccd::ListOwnedDatasetsOutput listOfDatasets;
        rv = Cache_ListOwnedDatasets(hs->GetUserId(), listOfDatasets, true);
        if (rv != 0) {
            LOG_ERROR("Could not get list of datasets: %d", rv);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }

        google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;

        for (it = listOfDatasets.dataset_details().begin(); it != listOfDatasets.dataset_details().end(); it++) {
            //std::string datasetTypeName = vplex::vsDirectory::DatasetType_Name(it->datasettype());
            if(it->suspendedflag()) {
                LOG_ERROR("Cloud Doc dataset("FMTu64") suspended.", it->datasetid());
                continue;
            }
            if(it->datasetname() == "Cloud Doc"){
                LOG_INFO("datasetid   ======>"FMTu64, it->datasetid());
                LOG_INFO("datasetname ======>%s", it->datasetname().c_str());
                datasetId = it->datasetid();
                datasetLocation = it->datasetlocation();
                break;
            }
        }

        if(it == listOfDatasets.dataset_details().end()){
            LOG_ERROR("CloudDoc dataset not found!!");
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
    }
    DocSyncDown_NotifyDatasetContentChange(datasetId, false);
    response = std::string("{\"numOfRequests\":1}");
    Utils::SetCompleteResponse(hs, 200, response, Utils::Mime_ApplicationJson);
    return 0;
}

int HttpSvc::Ccd::Handler_clouddoc::run_async_delete()
{
    assert(hs->GetMethod().compare("DELETE") == 0);
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("async") == 0);

    // URI must have exactly 3 parts, e.g., /clouddoc/async/<requestId>
    if (uri_tokens.size() != 3) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected URI: uri %s", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    const std::string &requestIdStr = uri_tokens[2];

    u64 requestId = VPLConv_strToU64(requestIdStr.c_str(), NULL, 10);

    LOG_INFO("Delete Request: id "FMTu64, requestId);
    int err = DocSNGQueue_CancelRequest(requestId);
    if (err) {
        LOG_ERROR("Handler_clouddoc[%p]: Failed to delete request: id "FMTu64", err %d", this, requestId, err);
        Utils::SetCompleteResponse(hs, 400);
        return 0;  // reset error
    }

    Utils::SetCompleteResponse(hs, 200);
    return 0;
}

int HttpSvc::Ccd::Handler_clouddoc::run_async_get()
{
    assert(hs->GetMethod().compare("GET") == 0);
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("async") == 0);

    // URI must have exactly 2 or 3 parts, i.e.,
    // /clouddoc/async
    // /clouddoc/async/<requestId>
    if (uri_tokens.size() > 3) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected URI: uri %s", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    std::string respbody;
    if (uri_tokens.size() == 2) {
        char *output = NULL;
        int err = DocSNGQueue_GetRequestListInJson(&output);
        ON_BLOCK_EXIT(free, output);
        if (err) {
            LOG_ERROR("Handler_clouddoc[%p]: Failed to get list of requests: err %d", this, err);
            Utils::SetCompleteResponse(hs, 500);
            return 0;  // reset error
        }
        respbody.assign(output);
    }
    else {
        assert(uri_tokens.size() == 3);
        const std::string &requestIdStr = uri_tokens[2];
        u64 requestId = VPLConv_strToU64(requestIdStr.c_str(), NULL, 10);

        char *output = NULL;
        int err = DocSNGQueue_GetRequestStatusInJson(requestId, &output);
        ON_BLOCK_EXIT(free, output);
        if (err) {
            LOG_ERROR("Handler_clouddoc[%p]: Failed to find request: id "FMTu64", err %d", this, requestId, err);
            if (err == CCD_ERROR_NOT_FOUND) {
                Utils::SetCompleteResponse(hs, 404);
            }
            else {
                Utils::SetCompleteResponse(hs, 400);
            }
            return 0;  // reset error
        }

        respbody.assign(output);
    }

    Utils::SetCompleteResponse(hs, 200, respbody, Utils::Mime_ApplicationJson);
    return 0;
}

int HttpSvc::Ccd::Handler_clouddoc::run_async_post()
{
    assert(hs->GetMethod().compare("POST") == 0);
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("clouddoc") == 0);
    assert(uri_tokens[1].compare("async") == 0);

    // NOTE:
    // The code below assumes that the whole request fits in the first 4096 bytes.
    // This is generally the case, but we could have anomalies..      
    char buf[4096];
    char *reqBody = NULL;
    {
        ssize_t bytes = hs->Read(buf, sizeof(buf)-1);  // subtract 1 for EOL
        if (bytes < 0) {
            LOG_ERROR("Handler_clouddoc[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        buf[bytes] = '\0';
        char *boundary = strstr(buf, "\r\n\r\n");  // find header-body boundary
        if (!boundary) {
            // header-body boundary not found - bad request
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        reqBody = boundary + 4;
    }

    LOG_DEBUG("request content: %s====================================", reqBody);

    std::string name, contentpath, previewpath, contentType;
    std::string moveFrom, op;
    bool isMove = false;
    bool isDelete = false;
    int change_type = ccd::DOC_SAVE_AND_GO_UPDATE;
    u64 revision = 0;
    u64 compid = 0, baserev = 0;
    u64 lastchanged = 0;
    cJSON2 *json_async = cJSON2_Parse(reqBody);
    if (json_async == NULL) {
        LOG_ERROR("Handler_clouddoc[%p]: Invalid JSON string %s", this, reqBody);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json_async);
    
    //
    // "op" is required for rename and delete, but optional
    // for upload to keep things backward compatible
    //
    if (!JSON_getString(json_async, "op", op)) {
        if (!op.compare("rename")) {
            if (!JSON_getString(json_async, "name", name) &&
                !JSON_getString(json_async, "moveFrom", moveFrom)) {
                    isMove = true;
                    change_type = ccd::DOC_SAVE_AND_GO_MOVE;
                    LOG_INFO("Move request: moveFrom=%s, moveTo=%s",
                        moveFrom.c_str(), name.c_str());
            } else {
                LOG_ERROR("Move parameter error");
                Utils::SetCompleteResponse(hs, 400);
                return 0;
            }
        } else if (!op.compare("delete")) {
            if (!JSON_getString(json_async, "name", name)) {
                isDelete = true;
                change_type = ccd::DOC_SAVE_AND_GO_DELETE;
                LOG_INFO("Delete request: deleteName=%s", name.c_str());
            } else {
                LOG_ERROR("Delete parameter error");
                Utils::SetCompleteResponse(hs, 400);
                return 0;
            }
        } else if (!op.compare("upload")) {
            LOG_INFO("Upload request");
        } else {
            LOG_ERROR("Unrecognized op %s", op.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    if (!isMove && !isDelete) {
        if (!JSON_getString(json_async, "name", name) &&
            !JSON_getString(json_async, "contentPath", contentpath)) {
            LOG_INFO("name=%s, contentPath=%s, previewPath=%s",
                     name.c_str(), contentpath.c_str(), previewpath.c_str());
        } else {
            LOG_ERROR("missing async upload parameter(s)");
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }
    
    // CompId is optional, but needs to be >= 1
    if (JSON_getInt64(json_async, "compId", compid) == 0) {
        LOG_INFO("compId="FMTu64, compid);
        if(!(compid >= 1)){
            LOG_ERROR("compId should to be at least 1");
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    // revision is optional on Move and Delete
    if (JSON_getInt64(json_async, "revision", revision) == 0) {
        LOG_INFO("revision="FMTu64, revision);
        if(!(revision >= 1)){
            LOG_ERROR("revision should to be at least 1");
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    if(!isMove && !isDelete) {
        LOG_INFO("Upload Case");
        if(contentpath.find("\\") != std::string::npos) {
            LOG_ERROR("Found backslash in contentPath: %s", contentpath.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    
        if (JSON_getString(json_async, "previewPath", previewpath) == 0) {
            LOG_INFO("previewPath=%s", previewpath.c_str());
        }
    
        if(previewpath.find("\\") != std::string::npos) {
            LOG_ERROR("Found backslash in previewPath: %s", previewpath.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    
        if (JSON_getString(json_async, "contentType", contentType) == 0) {
            LOG_INFO("contentType=%s", contentType.c_str());
        }
    
        if (JSON_getInt64(json_async, "baseRevision", baserev) == 0) {
            LOG_INFO("baseRevision="FMTu64, baserev);
            if(!(baserev >= 1)){
                LOG_ERROR("baseRevision should be at least 1");
                Utils::SetCompleteResponse(hs, 400);
                return 0;
            }
        }
    
        if (JSON_getInt64(json_async, "lastChanged", lastchanged) == 0) {
            LOG_INFO("lastChanged="FMTu64, lastchanged);
        }
    }

    // Get DatasetId, datasetLocation
    s32 rv = -1;
    u64 datasetId=0;
    std::string datasetLocation;
    {
        ccd::ListOwnedDatasetsOutput listOfDatasets;
        rv = Cache_ListOwnedDatasets(hs->GetUserId(), listOfDatasets, true);
        if (rv != 0) {
            LOG_ERROR("Could not get list of datasets: %d", rv);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        
        google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;
        
        for (it = listOfDatasets.dataset_details().begin(); it != listOfDatasets.dataset_details().end(); it++) {
            //std::string datasetTypeName = vplex::vsDirectory::DatasetType_Name(it->datasettype());
            if(it->datasetname() == CloudDoc_DatasetName){
                LOG_INFO("datasetid   ======>"FMTu64, it->datasetid());
                LOG_INFO("datasetname ======>%s", it->datasetname().c_str());
                datasetId = it->datasetid();
                datasetLocation = it->datasetlocation();
                break;
            }
        }
        
        if (it == listOfDatasets.dataset_details().end()) {
            LOG_ERROR("CloudDoc dataset not found!");
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
    }
    
    //Check file before we go
    if (!isMove && !isDelete) {
        VPLFS_stat_t stat;
        std::string errMsg;
        std::string response;
        
        rv = VPLFS_Stat(contentpath.c_str(), &stat);
        if (rv != VPL_OK) {
            LOG_ERROR("Failed stat on upload file %s", contentpath.c_str());
            errMsg += "Can not find upload file. ";
        }
        
        if (previewpath != "") {
            rv = VPLFS_Stat(previewpath.c_str(), &stat);
            if (rv != VPL_OK) {
                LOG_ERROR("Failed stat on preview file %s", previewpath.c_str());
                errMsg += "Can not find preview file. ";
            }
        }
        
        if (!errMsg.empty()) {
            response = "{\"errMsg\":\"" + errMsg + "\"}";
            Utils::SetCompleteResponse(hs, 404, response, Utils::Mime_ApplicationJson);
            return 0;
        }
    }
    
    u64 requestid = 0;
    rv = DocSNGQueue_NewRequest(hs->GetUserId(),
                                datasetId,
                                datasetLocation.c_str(),
                                change_type,
                                (isMove ? moveFrom.c_str() : name.c_str()),
                                contentpath.empty() ? NULL : contentpath.c_str(),
                                (isMove ? name.c_str() : NULL),
                                lastchanged,
                                (contentType.empty() ? NULL : contentType.c_str()),
                                compid,
                                ((revision != 0) ? revision : baserev),
                                ((previewpath != "") ? previewpath.c_str() : NULL),
                                NULL,        //request.has_thumbnail_image() ? request.thumbnail_image().data() : NULL,
                                0,           //request.has_thumbnail_image() ? request.thumbnail_image().size() : 0);
                                &requestid); // requestid OUT
    if (rv == 0) {
        std::ostringstream oss;
        oss << "/clouddoc/async/" << requestid;
        hs->SetRespHeader("Location", oss.str());
    }
    
    Utils::SetCompleteResponse(hs, rv == 0 ? 201 : 400);

    return 0;
}

int HttpSvc::Ccd::Handler_clouddoc::run_file_delete()
{
    assert(hs->GetMethod().compare("DELETE") == 0);

    int err = 0;

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    {
        std::string compid, rev, docname;
        u64 comp_id = 0, revision = 0;
        err = hs->GetQuery("compId", compid);

        if(compid.length() > 0) {
             comp_id = VPLConv_strToU64(compid.c_str(), NULL, 10);
        }

        err = hs->GetQuery("revision", rev);
        if(rev.length() > 0) {
            revision = VPLConv_strToU64(rev.c_str(), NULL, 10);
        }

        std::string uri;
        err = VPLHttp_DecodeUri(hs->GetUri(), uri);
        size_t pos_slash, pos;
        pos_slash = uri.find('/', std::string("/clouddoc/file").length());
        pos = uri.find_last_of('?',string::npos);
        if(pos - pos_slash > 1) {
            docname = uri.substr(pos_slash + 1, pos - pos_slash - 1);
        }

        if(docname.length() <= 0) {
            LOG_ERROR("Handler_picstream[%p]: Error when obtaining title: %s ", this, uri.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        if(0 == comp_id) {
            LOG_ERROR("Handler_clouddoc[%p]: Unexpected URI: uri %s", this, hs->GetUri().c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        //TODO: It would be better if checking local DB before querying. But, local DB is absent now.
    }

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        // errmsg logged and response set by getDatasetId()
        return 0;
    }

    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }

    // hs->GetUri() looks like /clouddoc/file/a/b/c?compId=123&revision=45
    // we want to construct a url that looks like
    // https://host:port/vcs/file/<datasetid>/a/b/c?compId=123&revision=45

    static const std::string prefixPattern = "/clouddoc/file";

    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/file/" << datasetId;
        oss.write(hs->GetUri().data() + prefixPattern.size(), hs->GetUri().size() - prefixPattern.size());
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

int HttpSvc::Ccd::Handler_clouddoc::run_file_get()
{
    assert(hs->GetMethod().compare("GET") == 0);

    int err = 0;

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }
    // revision must be present in the query string
    err = checkQueryPresent("revision");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    {
        std::string compid, rev, docname;
        u64 comp_id = 0, revision = 0;
        err = hs->GetQuery("compId", compid);

        if(compid.length() > 0) {
             comp_id = VPLConv_strToU64(compid.c_str(), NULL, 10);
        }
        err = hs->GetQuery("revision", rev);
        if(rev.length() > 0) {
            revision = VPLConv_strToU64(rev.c_str(), NULL, 10);
        }

        std::string uri;
        err = VPLHttp_DecodeUri(hs->GetUri(), uri);
        size_t pos_slash, pos;
        pos_slash = uri.find('/', std::string("/clouddoc/file").length());
        pos = uri.find_last_of('?',string::npos);
        if(pos - pos_slash > 1) {
            docname = uri.substr(pos_slash + 1, pos - pos_slash - 1);
        }

        if(docname.length() <= 0) {
            LOG_ERROR("Handler_picstream[%p]: Error when obtaining title: %s ", this, uri.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        if(0 == comp_id || 0 == revision) {
            LOG_ERROR("Handler_clouddoc[%p]: Unexpected URI: uri %s", this, hs->GetUri().c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        //TODO: It would be better if checking local DB before querying. But, local DB is absent now.

    }

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        // errmsg logged and response set by getDatasetId()
        return 0;
    }

    std::string sessionHandle, serviceTicket;
    {
        hs->GetReqHeader(Utils::HttpHeader_ac_sessionHandle, sessionHandle);
        hs->GetReqHeader(Utils::HttpHeader_ac_serviceTicket, serviceTicket);
    }

    // hs->GetUri() looks like /clouddoc/file/a/b/c?compId=123&revision=45

    static const std::string prefixPattern = "/clouddoc/file/";
    if (hs->GetUri().compare(0, prefixPattern.size(), prefixPattern) != 0) {
        LOG_ERROR("Handler_clouddoc[%p]: Unexpected URI: uri %s", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    const std::string path_query = hs->GetUri().substr(prefixPattern.size());
    // path_query = a/b/c?compId=123&revision=45

    std::string accessUrl, locationName;
    std::vector<std::string> headerName, headerValue;
    err = VCSgetAccessInfo(hs->GetUserId(), datasetId, sessionHandle, serviceTicket, "GET", path_query, "", accessUrl, headerName, headerValue, locationName);
    if (err < 0) {
        LOG_ERROR("Handler_clouddoc[%p]: VCSgetAccessInfo failed: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    LOG_INFO("Handler_clouddoc[%p]: accessUrl %s", this, accessUrl.c_str());
    hs->SetUri(accessUrl);
    hs->RemoveReqHeader("Host");

    for (size_t i = 0; i < headerName.size(); i++) {
        hs->SetReqHeader(headerName[i], headerValue[i]);
    }

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

int HttpSvc::Ccd::Handler_clouddoc::run_file_post()
{
    assert(hs->GetMethod().compare("POST") == 0);

    int err = 0;

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }
    // moveFrom must be present in the query string
    err = checkQueryPresent("moveFrom");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        // errmsg logged and response set by getDatasetId()
        return 0;
    }

    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }

    // hs->GetUri() looks like /clouddoc/file/a/b/c?moveFrom=foo&compId=123&revision=45
    // We want to construct a url that looks like
    // https://host:port/vcs/filemetadata/<datasetid>/a/b/c?moveFrom=foo&compId=123&revision=45
    // Note the objtype needs to change from "file" to "filemetadata".

    static const std::string prefixPattern = "/clouddoc/file";

    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/filemetadata/" << datasetId;
        oss.write(hs->GetUri().data() + prefixPattern.size(), hs->GetUri().size() - prefixPattern.size());
        oss << "&prefixServiceName=clouddoc";
        newuri.assign(oss.str());
    }

    // CCD needs to generate the request body.
    // Thus, it is not possible to forward the HttpStream object.
    // (It is only possible to modify the header.)
    // Plan: Create a new HttpStream obj and pass that to HsToHttpAdapter.

    std::string reqbody;
    {
        std::ostringstream oss;
        oss << "{\"updateDevice\":" << VirtualDevice_GetDeviceId() << "}";
        reqbody.assign(oss.str());
    }

    std::string reqhdr;
    {
        std::ostringstream oss;
        oss << "POST " << newuri << " HTTP/1.1\r\n";
        {
            std::string x_ac_userId;
            hs->GetReqHeader(Utils::HttpHeader_ac_userId, x_ac_userId);
            oss << Utils::HttpHeader_ac_userId << ": " << x_ac_userId << "\r\n";
        }
        {
            std::string x_ac_sessionHandle;
            hs->GetReqHeader(Utils::HttpHeader_ac_sessionHandle, x_ac_sessionHandle);
            oss << Utils::HttpHeader_ac_sessionHandle << ": " << x_ac_sessionHandle << "\r\n";
        }
        {
            std::string x_ac_serviceTicket;
            hs->GetReqHeader(Utils::HttpHeader_ac_serviceTicket, x_ac_serviceTicket);
            oss << Utils::HttpHeader_ac_serviceTicket << ": " << x_ac_serviceTicket << "\r\n";
        }
        oss << Utils::HttpHeader_ac_deviceId << ": " << VirtualDevice_GetDeviceId() << "\r\n";
        oss << Utils::HttpHeader_ContentLength << ": " << reqbody.size() << "\r\n";
        oss << "\r\n";
        reqhdr.assign(oss.str());
    }

    InStringStream iss(reqhdr + reqbody);
    OutStringStream oss;
    HttpStringStream *hss = new (std::nothrow) HttpStringStream(&iss, &oss);
    if (!hss) {
        LOG_ERROR("Handler_clouddoc[%p]: No memory to create HttpStringStream obj", this);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hss);
        if (!adapter) {
            LOG_ERROR("Handler_clouddoc[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            delete hss;
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }

    std::string output = oss.GetOutput();
    hs->Write(output.data(), output.size());
    delete hss;

    return err;
}

HttpSvc::Ccd::Handler_clouddoc::ObjJumpTable HttpSvc::Ccd::Handler_clouddoc::objJumpTable;

HttpSvc::Ccd::Handler_clouddoc::ObjJumpTable::ObjJumpTable()
{
    handlers["async"]        = &Handler_clouddoc::run_async;
    handlers["dir"]          = &Handler_clouddoc::run_dir;
    handlers["file"]         = &Handler_clouddoc::run_file;
    handlers["filemetadata"] = &Handler_clouddoc::run_filemetadata;
    handlers["preview"]      = &Handler_clouddoc::run_preview;
    handlers["conflict"]      = &Handler_clouddoc::run_conflict;
    handlers["copyback"]      = &Handler_clouddoc::run_copyback;
}

HttpSvc::Ccd::Handler_clouddoc::AsyncMethodJumpTable HttpSvc::Ccd::Handler_clouddoc::asyncMethodJumpTable;

HttpSvc::Ccd::Handler_clouddoc::AsyncMethodJumpTable::AsyncMethodJumpTable()
{
    handlers["DELETE"] = &Handler_clouddoc::run_async_delete;
    handlers["GET"]    = &Handler_clouddoc::run_async_get;
    handlers["POST"]   = &Handler_clouddoc::run_async_post;
}

HttpSvc::Ccd::Handler_clouddoc::FileMethodJumpTable HttpSvc::Ccd::Handler_clouddoc::fileMethodJumpTable;

HttpSvc::Ccd::Handler_clouddoc::FileMethodJumpTable::FileMethodJumpTable()
{
    handlers["DELETE"] = &Handler_clouddoc::run_file_delete;
    handlers["GET"]    = &Handler_clouddoc::run_file_get;
    handlers["POST"]   = &Handler_clouddoc::run_file_post;
}

int HttpSvc::Ccd::Handler_clouddoc::getDatasetId(u64 &datasetId)
{
    int err = VCS_getDatasetID(hs->GetUserId(), CloudDoc_DatasetName, datasetId);
    if (err < 0) {
        LOG_ERROR("Handler_clouddoc[%p]: Failed to find Cloud Doc dataset: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
    }
    return err;
}

int HttpSvc::Ccd::Handler_clouddoc::getServer(std::string &host_port)
{
    host_port = VCS_getServer(hs->GetUserId(), CloudDoc_DatasetName);
    if (host_port.empty()) {
        LOG_ERROR("Handler_clouddoc[%p]: Failed to determine server host:port", this);
        Utils::SetCompleteResponse(hs, 500);
        return -1;
    }
    return 0;
}

int HttpSvc::Ccd::Handler_clouddoc::checkQueryPresent(const std::string &name)
{
    std::string value;
    int err = hs->GetQuery(name, value);
    if (err) {
        LOG_ERROR("Handler_clouddoc[%p]: Query param %s missing: err %d", this, name.c_str(), err);
        Utils::SetCompleteResponse(hs, 400);
    }
    return err;
}
