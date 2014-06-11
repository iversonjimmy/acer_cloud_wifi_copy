#include "HttpSvc_HsToVcsTranslator.hpp"

#include "HttpSvc_HsToHttpAdapter.hpp"
#include "HttpSvc_Utils.hpp"

#include "ccdi.hpp"
#include "cache.h"
#include "config.h"

#include "vcs_proxy.hpp"
#include "vcs_util.hpp"
#include "vcs_utils.hpp"
#include "virtual_device.hpp"

#include <HttpStream.hpp>
#include <HttpStringStream.hpp>
#include <InStringStream.hpp>
#include <OutStringStream.hpp>
#include <log.h>

#include <vplex_http_util.hpp>

HttpSvc::HsToVcsTranslator::HsToVcsTranslator(HttpStream *hs, u64 datasetId_in) : hs(hs), datasetId(datasetId_in)
{
    LOG_INFO("HsToVcsTranslator[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::HsToVcsTranslator::HsToVcsTranslator(HttpStream *hs, const std::string datasetId_in) : hs(hs)
{
    datasetId = VPLConv_strToU64(datasetId_in.c_str(), NULL, 10);
    LOG_INFO("HsToVcsTranslator[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::HsToVcsTranslator::~HsToVcsTranslator()
{
    LOG_INFO("HsToVcsTranslator[%p]: Destroyed", this);
}

int HttpSvc::HsToVcsTranslator::Run()
{
    int err = 0;
    
    LOG_INFO("HsToVcsTranslator[%p]: Run", this);
    
    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // errmsg logged and response set by CheckReqHeaders()
        return 0;  // reset error
    }
    
    const std::string &uri = hs->GetUri();
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 2) {
        LOG_ERROR("HsToVcsTranslator[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    const std::string &uri_service = uri_tokens[0];
    
    if (serviceJumpTable.handlers.find(uri_service) == serviceJumpTable.handlers.end()) {
        LOG_ERROR("HsToVcsTranslator[%p]: No handler: service %s", this, uri_service.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    
    std::string::size_type pos = hs->GetUri().find("?");
    if (pos == string::npos) {
        hasNoparameters = true;
    } else {
        hasNoparameters = false;
    }
    
    return err = (this->*serviceJumpTable.handlers[uri_service])();
    // response set by obj handler
}

static s32 sendCb(VPLHttp2 *http, void *ctx, char *buf, u32 size)
{
    HttpSvc::HsToVcsTranslator *translator = (HttpSvc::HsToVcsTranslator*)ctx;
    return translator->sendCb(http, buf, size);
}

s32 HttpSvc::HsToVcsTranslator::sendCb(VPLHttp2 *http, char *buf, u32 size)
{
    return (s32)hs->Read(buf, size);
}

int HttpSvc::HsToVcsTranslator::run_rf()
{
    // rf request format: /rf/<obj>/<datasetId>/<operation>
    int err = 0;
    
    const std::string &uri = hs->GetUri();
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 2) {
        LOG_ERROR("HsToVcsTranslator[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    const std::string &uri_objtype = uri_tokens[1];
    
    if (objJumpTable.handlers.find(uri_objtype) == objJumpTable.handlers.end()) {
        LOG_ERROR("HsToVcsTranslator[%p]: No handler: objtype %s", this, uri_objtype.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    
    // Prepare prefixPattern
    // rf prefix pattern: /rf/<obj>/<datasetId>
    std::ostringstream prefixPatternStream;
    prefixPatternStream << "/" << uri_tokens[0] << "/" << uri_tokens[1] << "/" << uri_tokens[2];
    std::string prefixPattern = prefixPatternStream.str();
    if (hs->GetUri().compare(0, prefixPattern.size(), prefixPattern) != 0) {
        LOG_ERROR("HsToVcsTranslator[%p]: Unexpected URI: uri %s", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    
    // dirPath looks like /a/b/c?compId=123
    std::ostringstream dirPath;
    dirPath.write(hs->GetUri().data() + prefixPattern.size(), hs->GetUri().size() - prefixPattern.size());
    
    std::string encodedOperationPattern;
    if (hasNoparameters) {
        encodedOperationPattern = dirPath.str();
    } else {
        std::string::size_type pos = dirPath.str().find("?");
        // operationPattern looks like /a/b/c
        encodedOperationPattern = dirPath.str().substr(0, pos);
        // parameterPattern looks like ?compId=123
        parameterPattern = dirPath.str().substr(pos, dirPath.str().size() - encodedOperationPattern.size());
    }
    
    // Decode the URI
    VPLHttp_DecodeUri(encodedOperationPattern, operationPattern);
    
    // Remove the double root slash
    if (operationPattern.size() > 0) {
        if (operationPattern[0] == '/' && operationPattern[1] == '/') {
            operationPattern = operationPattern.substr(1, operationPattern.size());
        }
    }
    
    // Service prefix parameter, like: "prefixServiceName=clouddoc"
    // rf don't have this prefix, ensure it's empty
    if (!servicePrefix.empty()) {
        servicePrefix.clear();
    }
    
    // Category name
    // Current rf VCS uses notes dataset
    categoryName = "notes";
    datasetCategory = DATASET_CATEGORY_NOTES;
    
    // Using protocol version 3
    std::string version;
    err = hs->GetReqHeader(Utils::HttpHeader_ac_version, version);
    if (err == 0) {
        hs->RemoveReqHeader(Utils::HttpHeader_ac_version);
    }
    hs->SetReqHeader(Utils::HttpHeader_ac_version, "3");
    
    return err = (this->*objJumpTable.handlers[uri_objtype])();
    // response set by obj handler
}

//--------------------------------------------------
//-------------------------------------------------- Dir operations
//--------------------------------------------------
int HttpSvc::HsToVcsTranslator::run_dir()
{
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[1].compare("dir") == 0);
    
    const std::string &method = hs->GetMethod();
    if (dirMethodJumpTable.handlers.find(method) == dirMethodJumpTable.handlers.end()) {
        LOG_ERROR("HsToVcsTranslator[%p]: No handler: method %s", this, method.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    
    return (this->*dirMethodJumpTable.handlers[method])();
    // response set by method handler
}

int HttpSvc::HsToVcsTranslator::run_dir_delete()
{
    assert(hs->GetMethod().compare("DELETE") == 0);
    
    int err = 0;
    
    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }
    
    // hs->GetUri() looks like /<service>/dir/(datasetId)/a/b/c?compId=123
    // we want to construct a url that looks like
    // https://host:port/vcs/(category)/dir/<datasetid>/a/b/c?compId=123
    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/";
        if (categoryName.size() > 0) {
            oss << categoryName << "/";
        }
        oss << "dir/" << datasetId;
        oss << VPLHttp_UrlEncoding(operationPattern, "/");
        oss << parameterPattern;
        
        // compId is required
        std::string value;
        err = hs->GetQuery("compId", value);
        if (err) {
            // Retrieve it from the server if it's a notes dataset
            if (datasetCategory == DATASET_CATEGORY_NOTES) {
                u64 compId;
                err = getCompId(operationPattern, &compId);
                if(err != 0) {
                    // errmsg logged and response set by getCompId()
                    return 0;
                }
                
                if (hasNoparameters) {
                    oss << "?";
                } else {
                    oss << "&";
                }
                oss << "compId=" << compId;
            }
        }
        
        newuri.assign(oss.str());
    }
    
    hs->SetUri(newuri);
    Utils::SetLocalDeviceIdInReq(hs);
    hs->RemoveReqHeader("Host");
    
    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hs);
        if (!adapter) {
            LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }
    
    return err;
}

int HttpSvc::HsToVcsTranslator::run_dir_get()
{
    assert(hs->GetMethod().compare("GET") == 0);
    
    int err = 0;
    
    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }
    
    // hs->GetUri() looks like /<service>/dir/(datasetId)/(path)?max=10
    // we want to construct a url that looks like
    // https://host:port/vcs/(category)/dir/<datasetid>/(path)?max=10&(servicePrefix)
    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/";
        if (categoryName.size() > 0) {
            oss << categoryName << "/";
        }
        oss << "dir/" << datasetId;
        oss << VPLHttp_UrlEncoding(operationPattern, "/");
        oss << parameterPattern;

        // compId is required for some datasets
        if (datasetCategory == DATASET_CATEGORY_NOTES) {
            // must get it with the current path
            std::string value;
            err = hs->GetQuery("compId", value);
            if (err) {
                u64 compId;
                err = getCompId(operationPattern, &compId);
                if(err != 0) {
                    // errmsg logged and response set by getCompId()
                    return 0;
                }
                
                if (hasNoparameters) {
                    oss << "?";
                } else {
                    oss << "&";
                }
                oss << "compId=" << compId;
            }
        }
        
        if (!servicePrefix.empty()) {
            oss << "&" << servicePrefix;
        }
        
        newuri.assign(oss.str());
    }
    
    hs->SetUri(newuri);
    Utils::SetLocalDeviceIdInReq(hs);
    hs->RemoveReqHeader("Host");
    
    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hs);
        if (!adapter) {
            LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }
    
    return err;
}

int HttpSvc::HsToVcsTranslator::run_dir_post()
{
    assert(hs->GetMethod().compare("POST") == 0);
    
    int err = 0;
    
    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }
    
    // hs->GetUri() looks like /<service>/dir/(datasetId)/<path>
    // we want to construct a url that looks like
    // https://host:port/vcs/(category)/dir/<datasetid>/<path>?parentCompId=
    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/";
        if (categoryName.size() > 0) {
            oss << categoryName << "/";
        }
        oss << "dir/" << datasetId;
        oss << VPLHttp_UrlEncoding(operationPattern, "/");
        oss << parameterPattern;
        
        // parentCompId is required
        std::string value;
        err = hs->GetQuery("parentCompId", value);
        if (err) {
            // Retrieve it from the server if it's a notes dataset
            if (datasetCategory == DATASET_CATEGORY_NOTES) {
                std::string parentPath;
                getParentPath(operationPattern, parentPath);
                
                u64 compId;
                err = getCompId(parentPath, &compId);
                if(err != 0) {
                    // errmsg logged and response set by getCompId()
                    return 0;
                }
                if (hasNoparameters) {
                    oss << "?";
                } else {
                    oss << "&";
                }
                oss << "parentCompId=" << compId;
            }
        }
        
        newuri.assign(oss.str());
    }
    
    // CCD needs to generate the request body.
    // Thus, it is not possible to forward the HttpStream object.
    // (It is only possible to modify the header.)
    // Plan: Create a new HttpStream obj and pass that to HsToHttpAdapter.
    std::string reqbody;
    {
        u64 currentTime_SecUtime = VPLTime_ToSec(VPLTime_GetTime());
        
        std::ostringstream oss;
        oss << "{";
        oss <<   "\"lastChanged\":"  << currentTime_SecUtime <<",";
        oss <<   "\"createDate\":"   << currentTime_SecUtime <<",";
        oss <<   "\"updateDevice\":" << VirtualDevice_GetDeviceId();
        oss << "}";
        reqbody.assign(oss.str());
    }
    
    std::string reqhdr;
    {
        std::ostringstream oss;
        oss << "POST " << newuri << " HTTP/1.1\r\n";
        {
            oss << Utils::HttpHeader_ContentType << ": application/json\r\n";
        }
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
        std::string x_ac_version;
        err = hs->GetReqHeader(Utils::HttpHeader_ac_version, x_ac_version);
        if (err == 0) {
            oss << Utils::HttpHeader_ac_version << ": " << x_ac_version << "\r\n";
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
        LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HttpStringStream obj", this);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }
    
    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hss);
        if (!adapter) {
            LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HsToHttpAdapter obj", this);
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

//--------------------------------------------------
//-------------------------------------------------- File operations
//--------------------------------------------------
int HttpSvc::HsToVcsTranslator::run_file()
{
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[1].compare("file") == 0);
    
    const std::string &method = hs->GetMethod();
    if (fileMethodJumpTable.handlers.find(method) == fileMethodJumpTable.handlers.end()) {
        LOG_ERROR("HsToVcsTranslator[%p]: No handler: method %s", this, method.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    
    return (this->*fileMethodJumpTable.handlers[method])();
    // response set by method handler
}

int HttpSvc::HsToVcsTranslator::run_file_delete()
{
    assert(hs->GetMethod().compare("DELETE") == 0);
    
    int err = 0;
    
    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }
    
    // hs->GetUri() looks like /<service>/file/(datasetId)/a/b/c?compId=123&revision=45
    // we want to construct a url that looks like
    // https://host:port/vcs/(category)/file/<datasetid>/a/b/c?compId=123&revision=45
    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/";
        if (categoryName.size() > 0) {
            oss << categoryName << "/";
        }
        oss << "file/" << datasetId;
        oss << VPLHttp_UrlEncoding(operationPattern, "/");
        oss << parameterPattern;
        
        // compId is required for some datasets
        if (datasetCategory == DATASET_CATEGORY_NOTES) {
            // must get it with the current path
            std::string value;
            err = hs->GetQuery("compId", value);
            if (err) {
                u64 compId;
                err = getCompId(operationPattern, &compId);
                if(err != 0) {
                    // errmsg logged and response set by getCompId()
                    return 0;
                }
                
                if (hasNoparameters) {
                    oss << "?";
                } else {
                    oss << "&";
                }
                oss << "compId=" << compId;
            }
        }
        
        newuri.assign(oss.str());
    }
    
    hs->SetUri(newuri);
    Utils::SetLocalDeviceIdInReq(hs);
    hs->RemoveReqHeader("Host");
    
    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hs);
        if (!adapter) {
            LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }
    
    return err;
}

int HttpSvc::HsToVcsTranslator::run_file_get()
{
    assert(hs->GetMethod().compare("GET") == 0);
    
    int err = 0;
    
    std::string sessionHandle, serviceTicket;
    {
        hs->GetReqHeader(Utils::HttpHeader_ac_sessionHandle, sessionHandle);
        hs->GetReqHeader(Utils::HttpHeader_ac_serviceTicket, serviceTicket);
    }
    
    // Retrieve protocol version
    std::string protocolVersion;
    hs->GetReqHeader(Utils::HttpHeader_ac_version, protocolVersion);
    
    // hs->GetUri() looks like /<service>/file/(datasetId)/a/b/c?compId=123&revision=45
    // operationPattern = /a/b/c
    // Also remove the root slash
    const std::string path_query = operationPattern.substr(1, operationPattern.size());
    // path_query = a/b/c
    
    std::ostringstream oss;
    oss << VPLHttp_UrlEncoding(path_query, "/");
    oss << parameterPattern;
    
    // compId is required
    std::string value;
    err = hs->GetQuery("compId", value);
    if (err) {
        // Retrieve it from the server if it's a notes dataset
        if (datasetCategory == DATASET_CATEGORY_NOTES) {
            u64 compId;
            err = getCompId(operationPattern, &compId);
            if(err != 0) {
                // errmsg logged and response set by getCompId()
                return 0;
            }
            
            if (hasNoparameters) {
                oss << "?";
            } else {
                oss << "&";
            }
            oss << "compId=" << compId;
        }
    }
    
    std::string accessUrl, locationName;
    std::vector<std::string> headerName, headerValue;
    err = VCSgetAccessInfo(hs->GetUserId(), datasetId, sessionHandle, serviceTicket, "GET", oss.str(), "", categoryName, protocolVersion, accessUrl, headerName, headerValue, locationName);
    if (err < 0) {
        LOG_ERROR("HsToVcsTranslator[%p]: VCSgetAccessInfo failed: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }
    
    LOG_INFO("HsToVcsTranslator[%p]: accessUrl %s", this, accessUrl.c_str());
    hs->SetUri(accessUrl);
    hs->RemoveReqHeader("Host");
    
    for (size_t i = 0; i < headerName.size(); i++) {
        hs->SetReqHeader(headerName[i], headerValue[i]);
    }
    
    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hs);
        if (!adapter) {
            LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }
    
    return err;
}

int HttpSvc::HsToVcsTranslator::run_file_post()
{
    assert(hs->GetMethod().compare("POST") == 0);
    
    int err = 0;
    
    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }
    
    // Retrieve the access URL
    std::string sessionHandle, serviceTicket;
    {
        hs->GetReqHeader(Utils::HttpHeader_ac_sessionHandle, sessionHandle);
        hs->GetReqHeader(Utils::HttpHeader_ac_serviceTicket, serviceTicket);
    }
    
    // Retrieve protocol version
    std::string protocolVersion;
    hs->GetReqHeader(Utils::HttpHeader_ac_version, protocolVersion);
    
    std::string accessUrl, locationName;
    std::vector<std::string> headerName, headerValue;
    err = VCSgetAccessInfo(hs->GetUserId(), datasetId, "", "", "PUT", "",
                           "", categoryName, protocolVersion, accessUrl, headerName, headerValue, locationName);
    if (err < 0) {
        LOG_ERROR("HsToVcsTranslator[%p]: VCSgetAccessInfo failed: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }
    
    LOG_INFO("HsToVcsTranslator[%p]: accessUrl %s", this, accessUrl.c_str());
    
    // hs->GetUri() looks like /<service>/file/(datasetId)/a/b/c?moveFrom=foo&compId=123&revision=45
    // We want to construct a url that looks like
    // https://host:port/vcs/(category)/filemetadata/<datasetid>/a/b/c?moveFrom=foo&compId=123&revision=45
    // Note the objtype needs to change from "file" to "filemetadata".
    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/";
        if (categoryName.size() > 0) {
            oss << categoryName << "/";
        }
        oss << "filemetadata/" << datasetId;
        oss << VPLHttp_UrlEncoding(operationPattern, "/");
        oss << parameterPattern;
        
        // folderCompId is required for some datasets
        if (datasetCategory == DATASET_CATEGORY_NOTES) {
            // folderCompId might not be present in the query string
            // must get it with the current path
            std::string value;
            err = hs->GetQuery("folderCompId", value);
            if (err) {
                
                std::string parentPath;
                getParentPath(operationPattern, parentPath);
                
                u64 compId;
                err = getCompId(parentPath, &compId);
                if(err != 0) {
                    // errmsg logged and response set by getCompId()
                    return 0;
                }
                if (hasNoparameters) {
                    oss << "?";
                } else {
                    oss << "&";
                }
                oss << "folderCompId=" << compId;
            }
            
            oss << "&uploadRevision=1";
            hasNoparameters = false;
        }
        
        if (!servicePrefix.empty()) {
            if (hasNoparameters) {
                oss << "?";
            } else {
                oss << "&";
            }
            oss << servicePrefix;
        }
        newuri.assign(oss.str());
    }
    
    u64 contentLength = 0;
    {
        std::string contentLengthStr;
        err = hs->GetReqHeader("Content-Length", contentLengthStr);
        if (err) {
            LOG_ERROR("HsToVcsTranslator[%p]: Content-Length missing: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
        contentLength = VPLConv_strToU64(contentLengthStr.c_str(), NULL, 10);
    }
    
    // Create a new HttpStream obj and pass that to HsToHttpAdapter.
    std::string reqbody;
    {
        u64 currentTime_SecUtime = VPLTime_ToSec(VPLTime_GetTime());
        
        std::ostringstream oss;
        oss << "{";
        oss << "\"lastChanged\":" << currentTime_SecUtime <<",";
        oss << "\"createDate\":" << currentTime_SecUtime <<",";
        oss << "\"size\":" << contentLength << ",";
        oss << "\"updateDevice\":" << VirtualDevice_GetDeviceId() << ",";
        oss << "\"accessUrl\":" << "\"" << accessUrl << "\"";
        oss << "}";
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
        if (protocolVersion.size() > 0) {
            oss << Utils::HttpHeader_ac_version << ": " << protocolVersion << "\r\n";
        }
        oss << Utils::HttpHeader_ac_deviceId << ": " << VirtualDevice_GetDeviceId() << "\r\n";
        oss << Utils::HttpHeader_ContentLength << ": " << reqbody.size() << "\r\n";
        oss << "\r\n";
        reqhdr.assign(oss.str());
    }
    
    // Upload the content
    VPLHttp2 httpHandler;
    if (__ccdConfig.debugVcsHttp){
        httpHandler.SetDebug(false);
    }
    
    for(size_t i=0; i<headerName.size(); i++){
        httpHandler.AddRequestHeader(headerName[i], headerValue[i]);
    }
    
    string response;
    httpHandler.SetUri(accessUrl);
    httpHandler.SetTimeout(VPLTime_FromSec(30));
    LOG_DEBUG("starting PUT");
    
    err = removeReqHeaderFromStream();
    if (err < 0) {
        // errmsg logged and response set by removeReqHeaderFromStream()
        return 0;
    }
    
    std::string respBody;
    err = httpHandler.Put(::sendCb, this, contentLength, /*sendProgCb*/NULL, /*sendProgCtx*/NULL, respBody);
    
    if (err) {
        hs->SetStatusCode(httpHandler.GetStatusCode());
        {
            std::ostringstream oss;
            oss << respBody.size();
            hs->SetRespHeader("Content-Length", oss.str());
        }
        if (respBody.size() > 0) {
            hs->Write(respBody.data(), respBody.size());
        }
        else {
            hs->Flush();
        }
        return 0;
    }
    
    if (httpHandler.GetStatusCode() < 200 || httpHandler.GetStatusCode() >= 300) {
        LOG_ERROR("HsToHttpAdapter[%p]: File upload failed", this);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }
    
    // Post file metadata
    InStringStream iss(reqhdr + reqbody);
    OutStringStream oss;
    HttpStringStream *hss = new (std::nothrow) HttpStringStream(&iss, &oss);
    if (!hss) {
        LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HttpStringStream obj", this);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }
    
    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hss);
        if (!adapter) {
            LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HsToHttpAdapter obj", this);
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

//--------------------------------------------------
//--------------------------------------------------  Filemetadata operations
//--------------------------------------------------
int HttpSvc::HsToVcsTranslator::run_filemetadata()
{
    int err = 0;
    
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[1].compare("filemetadata") == 0);
    
    // only acceptable method is GET
    if (hs->GetMethod().compare("GET") != 0) {
        LOG_ERROR("HsToVcsTranslator[%p]: Unexpected method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 405);
        return 0;
    }
    
    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }
    
    // hs->GetUri() looks like /<service>/filemetadata/(datasetid)/a/b/c?compId=123&revision=45
    // we want to construct a url that looks like
    // https://host:port/vcs/(category)/filemetadata/<datasetid>/a/b/c?compId=123&revision=45
    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/";
        if (categoryName.size() > 0) {
            oss << categoryName << "/";
        }
        oss << "filemetadata/" << datasetId;
        oss << VPLHttp_UrlEncoding(operationPattern, "/");
        oss << parameterPattern;
        
        // compId is required
        std::string value;
        err = hs->GetQuery("compId", value);
        if (err) {
            // Retrieve it from the server if it's a notes dataset
            if (datasetCategory == DATASET_CATEGORY_NOTES) {
                u64 compId;
                err = getCompId(operationPattern, &compId);
                if(err != 0) {
                    // errmsg logged and response set by getCompId()
                    return 0;
                }
                
                if (hasNoparameters) {
                    oss << "?";
                } else {
                    oss << "&";
                }
                oss << "compId=" << compId;
            }
        }
        
        if (!servicePrefix.empty()) {
            oss << "&" << servicePrefix;
        }
        
        newuri.assign(oss.str());
    }
    
    hs->SetUri(newuri);
    Utils::SetLocalDeviceIdInReq(hs);
    hs->RemoveReqHeader("Host");
    
    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hs);
        if (!adapter) {
            LOG_ERROR("HsToVcsTranslator[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }
    
    return err;
}

//--------------------------------------------------  Jump Tables and internal functions
HttpSvc::HsToVcsTranslator::ServiceJumpTable HttpSvc::HsToVcsTranslator::serviceJumpTable;

HttpSvc::HsToVcsTranslator::ServiceJumpTable::ServiceJumpTable()
{
    handlers["rf"] = &HsToVcsTranslator::run_rf;
}

HttpSvc::HsToVcsTranslator::ObjJumpTable HttpSvc::HsToVcsTranslator::objJumpTable;

HttpSvc::HsToVcsTranslator::ObjJumpTable::ObjJumpTable()
{
    handlers["dir"]          = &HsToVcsTranslator::run_dir;
    handlers["file"]         = &HsToVcsTranslator::run_file;
    handlers["filemetadata"] = &HsToVcsTranslator::run_filemetadata;
}

HttpSvc::HsToVcsTranslator::DirMethodJumpTable HttpSvc::HsToVcsTranslator::dirMethodJumpTable;

HttpSvc::HsToVcsTranslator::DirMethodJumpTable::DirMethodJumpTable()
{
    handlers["DELETE"] = &HsToVcsTranslator::run_dir_delete;
    handlers["GET"]    = &HsToVcsTranslator::run_dir_get;
    handlers["POST"]   = &HsToVcsTranslator::run_dir_post;
}

HttpSvc::HsToVcsTranslator::FileMethodJumpTable HttpSvc::HsToVcsTranslator::fileMethodJumpTable;

HttpSvc::HsToVcsTranslator::FileMethodJumpTable::FileMethodJumpTable()
{
    handlers["DELETE"] = &HsToVcsTranslator::run_file_delete;
    handlers["GET"]    = &HsToVcsTranslator::run_file_get;
    handlers["POST"]   = &HsToVcsTranslator::run_file_post;
}

int HttpSvc::HsToVcsTranslator::getServer(std::string &host_port)
{
    host_port = VCS_getServer(hs->GetUserId(), datasetId);
    if (host_port.empty()) {
        LOG_ERROR("HsToVcsTranslator[%p]: Failed to determine server host:port", this);
        Utils::SetCompleteResponse(hs, 500);
        return -1;
    }
    return 0;
}

int HttpSvc::HsToVcsTranslator::checkQueryPresent(const std::string &name)
{
    std::string value;
    int err = hs->GetQuery(name, value);
    if (err) {
        LOG_ERROR("HsToVcsTranslator[%p]: Query param %s missing: err %d", this, name.c_str(), err);
        Utils::SetCompleteResponse(hs, 400);
    }
    return err;
}

void HttpSvc::HsToVcsTranslator::getParentPath(const std::string &path, std::string &parentDir)
{
    size_t len = path.size();
    
    // skip slashes
    for (; len > 0 && path[len-1] == '/'; len--)
        ;
    // skip non-slashes
    for (; len > 0 && path[len-1] != '/'; len--)
        ;
    // skip slashes
    for (; len > 0 && path[len-1] == '/'; len--)
        ;
    
    if (len == 0) {
        if (path[0] == '/')
            parentDir.assign("/");
        else
            parentDir.clear();
    }
    else
        parentDir.assign(path, 0, len);
}

int HttpSvc::HsToVcsTranslator::removeReqHeaderFromStream()
{
    int err = 0;
    
    // Look for separation between message-header and message-body:
    //  Assuming all messages have at least 1 header, so look for CRLFCRLF sequence
    //  See http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4
    
    int state = 0;
    // state 0 is the initial state
    // state 1 means just saw \r
    // state 2 means just saw \r\n
    // state 3 means just saw \r\n\r
    // state 4 means just saw \r\n\r\n and is the final state
    
    while (state != 4) {
        char buf[1];
        ssize_t bytes = hs->Read(buf, sizeof(buf));
        if (bytes == 0) {  // EOF
            LOG_ERROR("HsToVcsTranslator[%p]: Premature EOF of request", this);
            Utils::SetCompleteResponse(hs, 400);
            err = -1;
            break;
        }
        if (state == 0) {
            state = buf[0] == '\r' ? 1 : 0;
        }
        else if (state == 1) {
            state = buf[0] == '\n' ? 2 : 0;
        }
        else if (state == 2) {
            state = buf[0] == '\r' ? 3 : 0;
        }
        else if (state == 3) {
            state = buf[0] == '\n' ? 4 : 0;
        }
        else {
            // unexpected state
            LOG_ERROR("HsToVcsTranslator[%p]: Internal error", this);
            Utils::SetCompleteResponse(hs, 500);
            err = -1;
            break;
        }
    }
    
    return err;
}

int HttpSvc::HsToVcsTranslator::getCompId(const std::string &pathString, u64 *compId_out)
{
    int err = 0;

    VcsSession vcsSession;
    {   // Populate VcsSession
        vcsSession.userId = hs->GetUserId();
        vcsSession.deviceId = VirtualDevice_GetDeviceId();
        ServiceSessionInfo_t session;
        err = Cache_GetSessionForVsdsByUser(vcsSession.userId, /*OUT*/session);
        if (err != 0) {
            LOG_ERROR("Cache_GetSessionForVsdsByUser("FMTu64"):%d",
                      vcsSession.userId, err);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        vcsSession.sessionHandle = session.sessionHandle;
        vcsSession.sessionServiceTicket = session.serviceTicket;
        
        {
            ccd::GetInfraHttpInfoInput req;
            req.set_user_id(vcsSession.userId);
            req.set_service(ccd::INFRA_HTTP_SERVICE_VCS);
            ccd::GetInfraHttpInfoOutput resp;
            err = CCDIGetInfraHttpInfo(req, resp);
            if (err != 0) {
                LOG_ERROR("CCDIGetInfraHttpInfo("FMTu64") failed: %d",
                          vcsSession.userId, err);
                Utils::SetCompleteResponse(hs, 500);
                return err;
            }
            vcsSession.urlPrefix = resp.url_prefix();
        }
    }
    
    if (datasetCategory == DATASET_CATEGORY_NOTES) {
        // TODO: We might need to change how VcsDataset being constructed when adding new service that requires a different category name being specified.
        VcsDataset vcsDataset(datasetId, VCS_CATEGORY_NOTES);
        
        // Send VCS request to get the compId
        VPLHttp2 httpHandle;
        err = vcs_get_comp_id(vcsSession,
                              httpHandle,
                              vcsDataset,
                              pathString,
                              true,
                              /*OUT*/ *compId_out);
        if(err != 0) {
            LOG_ERROR("vcs_get_comp_id("FMTu64",%s,"FMTu64",%s):%d",
                      vcsSession.userId, vcsSession.urlPrefix.c_str(),
                      datasetId, pathString.c_str(), err);
            if (err == VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT) {
                // Could not find any active component in target dataset.
                Utils::SetCompleteResponse(hs, 404);
            } else {
                Utils::SetCompleteResponse(hs, 500);
            }
            return err;
        }
    }

    return err;
}

