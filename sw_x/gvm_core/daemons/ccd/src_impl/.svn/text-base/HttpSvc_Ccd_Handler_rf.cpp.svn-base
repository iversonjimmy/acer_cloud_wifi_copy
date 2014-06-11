#include "HttpSvc_Ccd_Handler_rf.hpp"

// Need to include google/protobuf/descriptor.h before iOS' ConditionalMacros.h.
// For details, see http://code.google.com/p/protobuf/issues/detail?id=119 .
#include <google/protobuf/descriptor.h>

#include "HttpSvc_Ccd_AsyncAgent.hpp"
#include "HttpSvc_Ccd_Handler_Helper.hpp"
#include "HttpSvc_Ccd_Server.hpp"
#include "HttpSvc_Utils.hpp"
#include "HttpSvc_HsToVcsTranslator.hpp"

#include "cache.h"
#include "ccdi.hpp"
#include "JsonHelper.hpp"

#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <log.h>

#include <cJSON2.h>
#include <google/protobuf/repeated_field.h>

#include <vpl_conv.h>
#include <vpl_fs.h>
#include <vplex_http_util.hpp>
#include <vplu_format.h>

// RF interface is described at
// http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Dataset_Access

enum DatasetAccessProtocol {
    PROTOCOL_VSS = 1,
    PROTOCOL_VCS,
};

static u64 findDeviceHostingDataset(HttpStream *hs, u64 datasetId, DatasetAccessProtocol &accessProtocol_out)
{
    CacheAutoLock autoLock;
    int err = autoLock.LockForRead();
    if (err < 0) {
        LOG_ERROR("Failed to obtain lock: err %d", err);
        HttpSvc::Utils::SetCompleteResponse(hs, 500);
        return 0;  // error
    }

    CachePlayer* user = cache_getUserByUserId(hs->GetUserId());
    if (user == NULL) {
        LOG_ERROR("user "FMT_VPLUser_Id_t" not signed-in", hs->GetUserId());
        HttpSvc::Utils::SetCompleteResponse(hs, 401);
        return 0;  // error
    }

    const vplex::vsDirectory::DatasetDetail* dd = user->getDatasetDetail(datasetId);
    if (dd == NULL) {  // no such dataset
        LOG_ERROR("No dataset of id "FMTu64, datasetId);
        HttpSvc::Utils::SetCompleteResponse(hs, 400);
        return 0;  // error
    }
    if (dd->datasetlocation().find("<protocol>VCS</protocol>") != string::npos) {
        accessProtocol_out = PROTOCOL_VCS;
    } else {
        accessProtocol_out = PROTOCOL_VSS;
    };

    return dd->clusterid();
}

static u64 findDeviceHostingDataset(HttpStream *hs, const std::string &datasetIdStr, DatasetAccessProtocol &accessProtocol_out)
{
    u64 datasetId = VPLConv_strToU64(datasetIdStr.c_str(), NULL, 10);

    return findDeviceHostingDataset(hs, datasetId, accessProtocol_out);
}

HttpSvc::Ccd::Handler_rf::Handler_rf(HttpStream *hs)
    : Handler(hs), asyncAgent(NULL)
{
    LOG_INFO("Handler_rf[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_rf::~Handler_rf()
{
    if (asyncAgent) {
        Utils::DestroyPtr(asyncAgent);  // REFCOUNT(AsyncAgent,RfInstanceCopy)
    }

    LOG_INFO("Handler_rf[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_rf::_Run()
{
    int err = 0;

    LOG_INFO("Handler_rf[%p]: Run", this);

    const std::string &uri = hs->GetUri();

    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // errmsg logged and response set by CheckReqHeaders()
        return 0;
    }

    // parse the uri for the namespaces
    // (1) rf (2) command (3) dataset_id (4) path
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 2) {
        LOG_ERROR("Handler_rf[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    const std::string &uri_objtype = uri_tokens[1];

    if (objJumpTable.handlers.find(uri_objtype) == objJumpTable.handlers.end()) {
        LOG_ERROR("Handler_rf[%p]: No handler: objtype %s", this, uri_objtype.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    err = (this->*objJumpTable.handlers[uri_objtype])();
    // response set by obj handler
    return err;
}

int HttpSvc::Ccd::Handler_rf::process_async()
{
    int err = 0;

    const std::string &method = hs->GetMethod();

    // Four kind of async. operations
    // 1. POST with request body (JSON format) -> to upload or download a file
    // 2. GET
    //    2a) w/o handle: list all the async op and returned in JSON format
    //    2b) w/ handle:  list specific async op and returned in JSON format
    // 3. DELETE with handle: cancel the async op
    // handle non-dataset required operation here (2)/(3)

    // status code defined in gvm_core/daemons/ccd/src_impl/AsyncUploadQueue.hpp
    // "enum UploadStatusType"

    {
        MutexAutoLock lock(&asyncLock.mutex);
        if (asyncAgent_shared) {
            asyncAgent = Utils::CopyPtr(asyncAgent_shared);  // REFCOUNT(AsyncAgent,RfInstanceCopy)
        }
    }

    if (asyncMethodJumpTable.handlers.find(method) != asyncMethodJumpTable.handlers.end()) {
        if (asyncAgent == NULL) {
            LOG_ERROR("No AsyncAgent obj");
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }

        err = (this->*asyncMethodJumpTable.handlers[method])();
        // response set by async handler
    }
    else {
        LOG_ERROR("Handler_rf[%p]: Unsupported method %s", this, method.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return err;
}

int HttpSvc::Ccd::Handler_rf::process_dataset()
{
    int err = 0;

    ccd::ListOwnedDatasetsOutput listOfDatasets;
    err = Cache_ListOwnedDatasets(hs->GetUserId(), listOfDatasets, true);
    if (err != 0) {
        LOG_ERROR("Could not get list of datasets: %d", err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    std::ostringstream oss;
    oss << "{\"datasetList\":[";
    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;
    bool needComma = false;
    for (it = listOfDatasets.dataset_details().begin(); it != listOfDatasets.dataset_details().end(); it++) {
        std::string datasetTypeName = vplex::vsDirectory::DatasetType_Name(it->datasettype());
        if (needComma) {
            oss << ",";
        }
        oss << "{"
            << "\"datasetId\":" << it->datasetid()
            << ",\"datasetName\":\"" << it->datasetname() << '"'
            << ",\"datasetType\":\"" << datasetTypeName << '"'
            << ",\"clusterId\":" << it->clusterid()
            << "}";
        needComma = true;
    }
    oss << "]}";
    std::string response = oss.str();
    Utils::SetCompleteResponse(hs, 200, response, Utils::Mime_ApplicationJson);

    return err;
}

int HttpSvc::Ccd::Handler_rf::process_device()
{
    int err = 0;

    ccd::ListLinkedDevicesInput req;
    ccd::ListLinkedDevicesOutput resp;

    LOG_DEBUG("Handle_rf[%p]: Call CCDIListLinkedDevices", this);
    req.set_user_id(hs->GetUserId());
    req.set_only_use_cache(true);

    err = CCDIListLinkedDevices(req, resp);
    if (err < 0) {
        LOG_ERROR("Handle_rf[%p]: CCDIListLinkedDevices failed: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    std::ostringstream oss;
    oss << "{\"deviceList\":[";
    bool needComma = false;

    for (int i = 0; i < resp.devices_size(); i++) {
        const ccd::LinkedDeviceInfo& curr = resp.devices(i);
        LOG_DEBUG("device[%d]: name(%s) id("FMTu64")",
                  i, curr.device_name().c_str(), curr.device_id());
        if (needComma) {
            oss << ",";
        }
        oss << "{"
            << "\"deviceClass\":\"" << curr.device_class() << '"'
            << ",\"deviceId\":" << curr.device_id()
            << ",\"deviceName\":\"" << curr.device_name() << '"'
            << ",\"isAcer\":" << (curr.is_acer() ? "true" : "false")
            << ",\"isPsn\":" << (curr.is_storage_node() ? "true" : "false")
            << ",\"osVersion\":\"" << curr.os_version() << '"'
            << "}";
        needComma = true;
    }
    oss << "]}";
    std::string response = oss.str();
    Utils::SetCompleteResponse(hs, 200, response, Utils::Mime_ApplicationJson);

    return err;
}

int HttpSvc::Ccd::Handler_rf::process_dir()
{
    int err = 0;

    // For /rf/dir or /rf/file the requested format are something looks like
    // /rf/dir/<dataset>/<file>
    // So the uri_tokens size should be at least 4. Filter out the non-sense
    // Don't bother to forward the request
    if (uri_tokens.size() < 4 || uri_tokens[2].empty()) {
        LOG_ERROR("Invalid request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    const std::string &datasetIdStr = uri_tokens[2];
    DatasetAccessProtocol accessProtocol;
    u64 deviceId = findDeviceHostingDataset(hs, datasetIdStr, accessProtocol);
    if (!deviceId) {  // error
        // errmsg logged and response set by findDeviceHostingDataset()
        return 0;
    }
    
    if (accessProtocol == PROTOCOL_VCS) {
        LOG_INFO("Handler_rf[%p]: Using VCS protocol", this);
        
        // copyFrom and moveFrom are not supported for VCS
        if ((hs->GetUri().find("copyFrom") != string::npos) ||
            (hs->GetUri().find("moveFrom") != string::npos)) {
            LOG_ERROR("Unsupported request, http_method=%s, uri=%s",
                      hs->GetMethod().c_str(),
                      hs->GetUri().c_str());
            std::string response = "{\"errMsg\": \"invalid request\"}";
            Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
            return 0;
        }
        
        // VCS dir uses POST instead of PUT
        if (hs->GetMethod().compare("PUT") == 0) {
            hs->SetMethod("POST");
        }
        
        // Translate to a VCS request by using HsToVcsTranslator
        {
            HsToVcsTranslator *translator = new (std::nothrow) HsToVcsTranslator(hs, datasetIdStr);
            if (!translator) {
                LOG_ERROR("Handler_rf[%p]: No memory to create HsToVcsTranslator obj", this);
                Utils::SetCompleteResponse(hs, 500);
                return 0;
            }
            err = translator->Run();
            delete translator;
        }

    } else {
        LOG_INFO("Handler_rf[%p]: Using VSS protocol", this);
        hs->SetDeviceId(deviceId);
        
        hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
        hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);
        
        err = Handler_Helper::ForwardToServerCcd(hs);
    }
    
    return err;
}

int HttpSvc::Ccd::Handler_rf::process_file()
{
    int err = 0;

    // For /rf/dir or /rf/file the requested format are something looks like
    // /rf/dir/<dataset>/<file>
    // So the uri_tokens size should be at least 4. Filter out the non-sense
    // Don't bother to forward the request
    if (uri_tokens.size() < 4 || uri_tokens[2].empty()) {
        LOG_ERROR("Invalid request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    const std::string &datasetIdStr = uri_tokens[2];
    DatasetAccessProtocol accessProtocol;
    u64 deviceId = findDeviceHostingDataset(hs, datasetIdStr, accessProtocol);
    if (!deviceId) {
        // errmsg logged and response set by findDeviceHostingDataset()
        return 0;
    }
    
    if (accessProtocol == PROTOCOL_VCS) {
        LOG_INFO("Handler_rf[%p]: Using VCS protocol", this);
        
        // copyFrom and moveFrom are not supported for VCS
        if ((hs->GetUri().find("copyFrom") != string::npos) ||
            (hs->GetUri().find("moveFrom") != string::npos)) {
            LOG_ERROR("Unsupported request, http_method=%s, uri=%s",
                      hs->GetMethod().c_str(),
                      hs->GetUri().c_str());
            std::string response = "{\"errMsg\": \"invalid request\"}";
            Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
            return 0;
        }
        
        // Translate to a VCS request by using HsToVcsTranslator
        {
            HsToVcsTranslator *translator = new (std::nothrow) HsToVcsTranslator(hs, datasetIdStr);
            if (!translator) {
                LOG_ERROR("Handler_rf[%p]: No memory to create HsToVcsTranslator obj", this);
                Utils::SetCompleteResponse(hs, 500);
                return 0;
            }
            err = translator->Run();
            delete translator;
        }
        
    } else {
        LOG_INFO("Handler_rf[%p]: Using VSS protocol", this);
        hs->SetDeviceId(deviceId);
        
        hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
        hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);
        
        err = Handler_Helper::ForwardToServerCcd(hs);
    }
    
    return err;
}

int HttpSvc::Ccd::Handler_rf::process_filemetadata()
{
    int err = 0;

    // For /rf/dir or /rf/file the requested format are something looks like
    // /rf/dir/<dataset>/<file>
    // So the uri_tokens size should be at least 4. Filter out the non-sense
    // Don't bother to forward the request
    if (uri_tokens.size() < 4 || uri_tokens[2].empty()) {
        LOG_ERROR("Invalid request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    const std::string &datasetIdStr = uri_tokens[2];
    DatasetAccessProtocol accessProtocol;
    u64 deviceId = findDeviceHostingDataset(hs, datasetIdStr, accessProtocol);
    if (!deviceId) {
        // errmsg logged and response set by findDeviceHostingDataset()
        return 0;
    }
    
    if (accessProtocol == PROTOCOL_VCS) {
        LOG_INFO("Handler_rf[%p]: Using VCS protocol", this);
        
        // Only "set file permissions" uses PUT, this request will not being supported for VCS.
        if (hs->GetMethod().compare("PUT") == 0) {
            LOG_ERROR("Unsupported request, http_method=%s, uri=%s",
                      hs->GetMethod().c_str(),
                      hs->GetUri().c_str());
            std::string response = "{\"errMsg\": \"invalid request\"}";
            Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
            return 0;
        }
        
        // Translate to a VCS request by using HsToVcsTranslator
        {
            HsToVcsTranslator *translator = new (std::nothrow) HsToVcsTranslator(hs, datasetIdStr);
            if (!translator) {
                LOG_ERROR("Handler_rf[%p]: No memory to create HsToVcsTranslator obj", this);
                Utils::SetCompleteResponse(hs, 500);
                return 0;
            }
            err = translator->Run();
            delete translator;
        }
        
    } else {
        LOG_INFO("Handler_rf[%p]: Using VSS protocol", this);
        hs->SetDeviceId(deviceId);
        
        hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
        hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);
        
        err = Handler_Helper::ForwardToServerCcd(hs);
    }

    return err;
}

int HttpSvc::Ccd::Handler_rf::process_whitelist()
{
    int err = 0;

    // For /rf/whitelist the requested format are something looks like
    // /rf/whitelist/<dataset>
    // So the uri_tokens size should be at least 3. Filter out the non-sense
    // Don't bother to forward the request
    if (uri_tokens.size() < 3 || uri_tokens[2].empty()) {
        LOG_ERROR("Invalid request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    const std::string &datasetIdStr = uri_tokens[2];
    DatasetAccessProtocol accessProtocol;
    u64 deviceId = findDeviceHostingDataset(hs, datasetIdStr, accessProtocol);
    if (!deviceId) {
        // errmsg logged and response set by findDeviceHostingDataset()
        return 0;
    }
    
    // Not supported for VCS
    if (accessProtocol == PROTOCOL_VCS) {
        LOG_ERROR("Unsupported request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }
    
    hs->SetDeviceId(deviceId);

    hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
    hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);

    err = Handler_Helper::ForwardToServerCcd(hs);

    return err;
}

int HttpSvc::Ccd::Handler_rf::process_search()
{
    int err = 0;
    // For /rf/search the requested format are something looks like
    // /rf/search/<dataset>
    // So the uri_tokens size should be at least 3. Filter out the non-sense
    // Don't bother to forward the request
    if (uri_tokens.size() < 3 || uri_tokens[2].empty()) {
        LOG_ERROR("Invalid request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    const std::string &datasetIdStr = uri_tokens[2];
    DatasetAccessProtocol accessProtocol;
    u64 deviceId = findDeviceHostingDataset(hs, datasetIdStr, accessProtocol);
    if (!deviceId) {
        // errmsg logged and response set by findDeviceHostingDataset()
        return 0;
    }
    
    // Not supported for VCS
    if (accessProtocol == PROTOCOL_VCS) {
        LOG_ERROR("Unsupported request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }
    
    hs->SetDeviceId(deviceId);

    hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
    hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);

    err = Handler_Helper::ForwardToServerCcd(hs);

    return err;
}

int HttpSvc::Ccd::Handler_rf::process_async_delete()
{
    int err = 0;

    // The only type of request is to cancel an async transfer request
    // http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Dataset_Access#cancel_async_transfer_request

    int http_status = 500;
    std::string response;

    // URI must have the format /rf/async/<requestId>
    if (uri_tokens.size() == 3 && !uri_tokens[2].empty()) {
        u64 requestId = VPLConv_strToS64(uri_tokens[2].c_str(), NULL, 10);
        err = asyncAgent->CancelRequest(requestId);
        if (err) {
            // Error msg already logged by CancelRequest().
            if (err == CCD_ERROR_NOT_FOUND) {
                http_status = 404;
                response = "{\"errMsg\":\"does not exist\"}";
            }
            else if (err == CCD_ERROR_WRONG_STATE) {
                http_status = 400;
                response = "{\"errMsg\":\"wrong state to cancel\"}";
            }
            else if (err == CCD_ERROR_PARAMETER) {
                http_status = 400;
                response = "{\"errMsg\":\"bad request\"}";
            }
            else {
                http_status = 400;
            }
        }
        else {
            http_status = 200;
        }
    } else {
        LOG_ERROR("Malformed URI: uri %s", hs->GetUri().c_str());
        http_status = 400;
        response = "{\"errMsg\": \"bad uri\"}";
        err = CCDI_ERROR_PARAMETER;
    }

    Utils::SetCompleteResponse(hs, http_status, response, Utils::Mime_ApplicationJson);

    return 0;
}

int HttpSvc::Ccd::Handler_rf::process_async_get()
{
    int err = 0;

    // There are two types of requests:
    // (1) get a list of known requests
    // http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Dataset_Access#list_async_transfer_requests
    // (2) get the status of a specific request
    // http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Dataset_Access#check_async_transfer_request_status

    int http_status = 500;
    std::string response;

    if (uri_tokens.size() == 2) {
        err = asyncAgent->GetJsonTaskStatus(response);
        if (!err) {
            http_status = 200;
        }
    } else if (uri_tokens.size() == 3 && !uri_tokens[2].empty()) {
        u64 requestId = VPLConv_strToS64(uri_tokens[2].c_str(), NULL, 10);
        AsyncAgent::RequestStatus status;
        err = asyncAgent->GetRequestStatus(requestId, status);
        if (err) {
            // Error msg already logged by CancelRequest().
            if (err == CCD_ERROR_NOT_FOUND) {
                http_status = 404;
                response = "{\"errMsg\":\"does not exist\"}";
            }
            else if (err == CCD_ERROR_PARAMETER) {
                http_status = 400;
                response = "{\"errMsg\":\"bad request\"}";
            }
            else {
                http_status = 400;
            }
        }
        else {  // !err
            http_status = 200;
            std::ostringstream oss;
            oss << "{\"id\":" << requestId;
            oss << ",\"status\":";
            switch (status.status) {
            case pending:     oss << "\"wait\""; break;
            case querying:
            case processing:  oss << "\"active\""; break;
            case complete:    oss << "\"done\""; break;
            case cancelling:  oss << "\"cancelling\""; break;
            case cancelled:
            case failed:
            default: oss << "\"error\""; break;
            };
            oss << ",\"totalSize\":" << status.size;
            oss << ",\"xferedSize\":" << status.sent_so_far;
            oss << "}";
            response = oss.str();
        }
    } else {
        LOG_ERROR("Malformed URI: uri %s", hs->GetUri().c_str());
        http_status = 400;
        response = "{\"errMsg\": \"bad uri\"}";
        err = CCDI_ERROR_PARAMETER;
    }

    Utils::SetCompleteResponse(hs, http_status, response, Utils::Mime_ApplicationJson);

    return 0;
}

int HttpSvc::Ccd::Handler_rf::process_async_post()
{
    int err = 0;

    // Only possible request is to POST an asynchronous upload or download request.
    // Note: download is a future feature.
    // Details at http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Dataset_Access#create_new_async_transfer_request

    // FIXME: the code below assumes a single wait is sufficient to get the whole content.
    //        this is generally not the case but usually sufficient.
    char buf[4096];
    char *reqBody = NULL;
    {
        ssize_t bytes = hs->Read(buf, sizeof(buf) - 1);  // subtract 1 for EOL
        if (bytes < 0) {
            LOG_ERROR("Handler_rf[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
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

    cJSON2 *json_async = cJSON2_Parse(reqBody);
    if (json_async == NULL) {
        LOG_ERROR("Handler_rf[%p]: Invalid JSON string %s", this, reqBody);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json_async);

    LOG_DEBUG("request = %s", reqBody);

    u64 asyncDatasetId = 0;
    std::string asyncOp;
    std::string asyncPath;
    std::string asyncFilepath;
    if (JSON_getInt64(json_async, "datasetId", asyncDatasetId) ||
        JSON_getString(json_async, "op", asyncOp) ||
        JSON_getString(json_async, "path", asyncPath) ||
        JSON_getString(json_async, "filepath", asyncFilepath)) {
        LOG_ERROR("Parameter(s) missing");
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    if (asyncOp == "upload") {
        // Nothing to do.
        // This is the only acceptable op at this time.
    }
    else if (asyncOp == "download") {
        Utils::SetCompleteResponse(hs, 501);
        return 0;
    }
    else {
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    LOG_DEBUG("ds="FMTu64", op=%s, path=%s, filepath=%s",
              asyncDatasetId, asyncOp.c_str(),
              asyncPath.c_str(), asyncFilepath.c_str());

    DatasetAccessProtocol accessProtocol;
    u64 deviceId = findDeviceHostingDataset(hs, asyncDatasetId, accessProtocol);
    if (!deviceId) {
        // errmsg logged and response set by findDeviceHostingDataset()
        return 0;
    }

    // Generate a new HTTP request header that looks like
    //   POST /rf/file/<datasetid>/<path>
    //   x-ac-srcfile: <localfilepath>
    std::string request_content;
    {
        std::ostringstream oss;
        oss << "POST /" << uri_tokens[0] << "/file/" << asyncDatasetId << "/"  << VPLHttp_UrlEncoding(asyncPath) << " HTTP/1.1\r\n"
            << Utils::HttpHeader_ac_srcfile << ": " << asyncFilepath << "\r\n";
        request_content = oss.str();
    }

    s64 size = 0;
    {
        VPLFS_stat_t stat;
        // asyncfilepath: path in the local filesystem
        err = VPLFS_Stat(asyncFilepath.c_str(), &stat);
        if (err != VPL_OK) {
            LOG_ERROR("Handler_rf[%p]: VPLFS_Stat failed: path %s, err %d", this, asyncFilepath.c_str(), err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        size = stat.size;
    }

    u64 handle = 0;
    err = asyncAgent->AddRequest(hs->GetUserId(), deviceId, size, request_content, handle);
    if (err) {
        LOG_WARN("Failed to add async request to queue: err %d", err);
        Utils::SetCompleteResponse(hs, 503);
        return 0;
    }

    {
        std::ostringstream oss;
        oss << handle;
        Utils::SetCompleteResponse(hs, 200, oss.str(), Utils::Mime_TextPlain);
        return 0;
    }
}

int HttpSvc::Ccd::Handler_rf::process_dirmetadata()
{
    int err = 0;

    // For /rf/dirmetadata the requested format are something looks like
    // /rf/dirmetadata/<dataset>/<path>
    // So the uri_tokens size should be at least 4. Filter out the non-sense
    // Don't bother to forward the request
    if (uri_tokens.size() < 4 || uri_tokens[2].empty()) {
        LOG_ERROR("Invalid request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    const std::string &datasetIdStr = uri_tokens[2];
    DatasetAccessProtocol accessProtocol;
    u64 deviceId = findDeviceHostingDataset(hs, datasetIdStr, accessProtocol);
    if (!deviceId) {
        // errmsg logged and response set by findDeviceHostingDataset()
        return 0;
    }
    
    // Not supported for VCS
    if (accessProtocol == PROTOCOL_VCS) {
        LOG_ERROR("Unsupported request, http_method=%s, uri=%s",
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"invalid request\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }
    
    hs->SetDeviceId(deviceId);

    hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
    hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);

    err = Handler_Helper::ForwardToServerCcd(hs);

    return err;
}

HttpSvc::Ccd::Handler_rf::ObjJumpTable HttpSvc::Ccd::Handler_rf::objJumpTable;

HttpSvc::Ccd::Handler_rf::ObjJumpTable::ObjJumpTable()
{
    handlers["async"]        = &Handler_rf::process_async;
    handlers["dataset"]      = &Handler_rf::process_dataset;
    handlers["device"]       = &Handler_rf::process_device;
    handlers["dir"]          = &Handler_rf::process_dir;
    handlers["file"]         = &Handler_rf::process_file;
    handlers["filemetadata"] = &Handler_rf::process_filemetadata;
    handlers["search"]       = &Handler_rf::process_search;
    handlers["whitelist"]    = &Handler_rf::process_whitelist;
    handlers["dirmetadata"]  = &Handler_rf::process_dirmetadata;
}

HttpSvc::Ccd::Handler_rf::AsyncMethodJumpTable HttpSvc::Ccd::Handler_rf::asyncMethodJumpTable;

HttpSvc::Ccd::Handler_rf::AsyncMethodJumpTable::AsyncMethodJumpTable()
{
    handlers["DELETE"] = &Handler_rf::process_async_delete;
    handlers["GET"]    = &Handler_rf::process_async_get;
    handlers["POST"]   = &Handler_rf::process_async_post;
}

HttpSvc::Ccd::AsyncAgent *HttpSvc::Ccd::Handler_rf::asyncAgent_shared = NULL;

void HttpSvc::Ccd::Handler_rf::SetAsyncAgent(AsyncAgent *_asyncAgent)
{
    MutexAutoLock lock(&asyncLock.mutex);

    if (asyncAgent_shared) {
        Utils::DestroyPtr(asyncAgent_shared);  // REFCOUNT(AsyncAgent,RfClassCopy)
        asyncAgent_shared = NULL;
    }
    if (_asyncAgent) {
        asyncAgent_shared = Utils::CopyPtr(_asyncAgent);  // REFCOUNT(AsyncAgent,RfClassCopy)
    }
}

HttpSvc::Ccd::Handler_rf::AsyncLock HttpSvc::Ccd::Handler_rf::asyncLock;

HttpSvc::Ccd::Handler_rf::AsyncLock::AsyncLock()
{
    VPLMutex_Init(&mutex);
}

HttpSvc::Ccd::Handler_rf::AsyncLock::~AsyncLock()
{
    VPLMutex_Destroy(&mutex);
}
