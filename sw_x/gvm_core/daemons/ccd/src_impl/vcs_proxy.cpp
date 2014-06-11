/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include <vpl_types.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

#include <vplex_serialization.h>
#include <vpl_plat.h>
#include <vplu_types.h>
#include <vplex_http_util.hpp>
#include <log.h>

#include "vcs_util.hpp"
#include "vcs_utils.hpp"
#include "vcs_proxy.hpp"
#include "storage_proxy.hpp"

#include <vssi_error.h>
#include "virtual_device.hpp"
#include "cache.h"
#include "config.h"
#include "JsonHelper.hpp"
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
#include "cslsha.h"
#endif

#define CLOUDDOC_DATASET_NAME "Cloud Doc"

using namespace std;

static int translateHttpStatusCode(int httpStatus)
{
    int rv = vcs_translate_http_status_code(httpStatus);
    if (rv != 0) {
        LOG_ERROR("HTTP error: %d", httpStatus);
    }
    return rv;
}

size_t vcs_write_callback(const void *buf, size_t size, size_t nmemb, void *param);
size_t vcs_write_callback(const void *buf, size_t size, size_t nmemb, void *param)
{

    //cout << "vcs_write_callback: " <<(char*)buf << endl;
    std::string *strResponse = (std::string*)param;
    size_t numBytes = size * nmemb;
    if ((strResponse != NULL) && (buf != NULL)) {
        (*strResponse).append(static_cast<const char*>(buf), numBytes);
    } else {
        LOG_ERROR("VPLHttp_Request callback null param: buf 0x%x, param 0x%x, numBytes %d",
                  (int)buf, (int)param, (int)numBytes);
    }

    return nmemb;
}

static inline void generate_rf_response(stream_transaction* transaction, std::string& response, int response_code) {

    char dateStr[30];
    time_t curTime = time(NULL);
    strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT" , gmtime(&curTime));
    transaction->resp.add_header("Date", dateStr);

    transaction->resp.add_header("Ext", "");
    transaction->resp.add_header("Server","Acer Media Streaming Server");
    transaction->resp.add_header("realTimeInfo.dlna.org","DLNA.ORG_TLAG=*");
    transaction->resp.add_header("transferMode.dlna.org","Interactive");
    transaction->resp.add_header("Accept-Ranges", "bytes");
    transaction->resp.add_header("Connection", "Keep-Alive");
    if (response_code > 0) {
        transaction->resp.add_response(response_code);
    }
    char contentLengthValue[128];
    sprintf(contentLengthValue, FMTu_size_t, response.length());
    transaction->resp.add_header("Content-Length", contentLengthValue);
    transaction->resp.add_header("Content-Type", "application/json");
    transaction->resp.add_content(response);
    transaction->processing = false;
    transaction->sending = true;

}

int VCSGetCredentials(u64 uid, u64 &sessionHandle_out, string &serviceTicket_out)
{

    ServiceSessionInfo_t session;
    s32 rv = Cache_GetSessionForVsdsByUser(uid, session);
    if(rv != 0){
        LOG_ERROR("["FMTu64"]: Cache_GetSessionForVsdsByUser fail %d", uid, rv);
        return -1;
    }

    sessionHandle_out = session.sessionHandle;
    serviceTicket_out = session.serviceTicket;
    return 0;
}

static int getTransformedCredentials(u64 uid,
                                     std::string &sessionHandle_out,
                                     std::string &serviceTicketBase64Encoded_out)
{
    sessionHandle_out.clear();
    serviceTicketBase64Encoded_out.clear();
    u64 sessionHandle;
    std::string serviceTicket;
    int rv = VCSGetCredentials(uid, /*OUT*/ sessionHandle, /*OUT*/ serviceTicket);
    if (rv == 0) {
        stringstream ss;
        ss.str("");
        ss << static_cast<s64>(sessionHandle);
        sessionHandle_out = ss.str();
    }

    size_t encode_len = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(serviceTicket.length());
    //char secret_data_encoded[encode_len];
    char *secret_data_encoded = (char*)malloc(encode_len);
    if (secret_data_encoded == NULL) {
        return -1;
    }
    ON_BLOCK_EXIT(free, secret_data_encoded);

    VPL_EncodeBase64(serviceTicket.c_str(), serviceTicket.length(), secret_data_encoded, &encode_len, false, false);
    serviceTicketBase64Encoded_out = secret_data_encoded;

    return rv;
}

#define CHECK_AND_ADD_CREDENTIALS_HEADER(http, uid, query, check) \
    BEGIN_MULTI_STATEMENT_MACRO \
    std::string use_sessionHandle = sessionHandle; \
    std::string use_serviceTicket = serviceTicket; \
    if(query){ \
        if(check && !(sessionHandle.empty() && serviceTicket.empty()) ){ \
            /* If not empty, do nothing */ \
        \
        }else{ \
            { \
                int rc = getTransformedCredentials(uid, use_sessionHandle, use_serviceTicket); \
                if(rc < 0){ \
                    LOG_ERROR("getCredential failed!! ,%d", rc); \
                    return rc; \
                } \
            } \
        } \
    } \
    { \
        ostringstream oss; \
        oss.str(""); \
        oss << uid; \
        http.AddRequestHeader("X-ac-userId", oss.str()); \
        oss.str(""); \
        oss << use_sessionHandle; \
        http.AddRequestHeader("X-ac-sessionHandle", oss.str()); \
        oss.str(""); \
        oss << use_serviceTicket; \
        http.AddRequestHeader("X-ac-serviceTicket", oss.str()); \
        oss.str(""); \
        oss << VirtualDevice_GetDeviceId(); \
        http.AddRequestHeader("X-ac-deviceId", oss.str()); \
    } \
    END_MULTI_STATEMENT_MACRO

#define CHECK_AND_GET_USERREQ(_prefix) \
    BEGIN_MULTI_STATEMENT_MACRO \
    const std::string prefix = _prefix; \
    std::vector<std::string> uri_tokens; \
    VPLHttp_SplitUri(transaction->req->uri, uri_tokens); \
    if (uri_tokens.size() < 2) { \
        LOG_ERROR("uri_token less than 2: "FMTu_size_t, uri_tokens.size()); \
        return CCD_ERROR_PARAMETER; \
    } \
    if (uri_tokens[1] == prefix) { \
        size_t strfound; \
        strfound = transaction->req->uri.find(prefix); \
        if(transaction->req->uri.size() > strfound+prefix.size()+1) \
            userReq.assign(transaction->req->uri, strfound+prefix.size()+1, std::string::npos); \
        LOG_INFO("comppath and param======>%s", userReq.c_str()); \
    } else { \
        LOG_ERROR("uri_token mismatch: %s", uri_tokens[1].c_str()); \
        return CCD_ERROR_PARAMETER; \
    } \
    END_MULTI_STATEMENT_MACRO


int VCSpostUpload(const std::string &serviceName,
                  const std::string &comppath,
                  u64 modifyTime,
                  u64 userId,
                  u64 datasetId,
                  bool baserev_valid,
                  u64 baserev,
                  u64 compid,
                  // Out
                  string &vcsresponse,
                  string locationUri,
                  VPLFile_offset_t filesize)
{
    LOG_INFO("comppath ==========================> %s", comppath.c_str());

    int rc=0;
    VPLHttp2 h;
    int httpResponse = -1;
    //string response;

    char *out = NULL;
    
    stringstream ss;
    ss.str("");

    ss << "{"
       << "\"size\":" << filesize << ","
       << "\"updateDevice\":" << VirtualDevice_GetDeviceId() << ","
       << "\"accessUrl\":" << "\"" << locationUri << "\""; 

    if(modifyTime){
        if (compid == 0 && !baserev_valid) {
            ss << ",\"createDate\":" << modifyTime ;
        }
        ss << ",\"lastChanged\":" << modifyTime ;
    }
    ss << "}";
    
    out = strdup(ss.str().c_str());
    ON_BLOCK_EXIT(free, out);

    //Create JSON for VCS
    LOG_DEBUG("cJSON2 for VCS======>%s", out);

    string sessionHandle, serviceTicket;
    CHECK_AND_ADD_CREDENTIALS_HEADER(h, userId, true, false);

    string url;
    ss.str("");
    ss << "https://" << VCS_getServer(userId, datasetId) << "/" << serviceName << "/filemetadata/" << datasetId << "/" <<  comppath;
    if (baserev_valid) {
        ss << "?baseRevision=" << baserev;
    }
    if (compid) {
        ss << "&compId=" << compid;
    }
    url = ss.str();

    h.SetUri(url);
    h.SetTimeout(VPLTime_FromSec(30));
    
    if (__ccdConfig.debugVcsHttp)
        h.SetDebug(1);

    rc = h.Post(
            string(out),   // payload
            vcsresponse);  // arg to callback

    if (rc < 0){
        LOG_ERROR("VPLHttp_Request failed!, %d", rc);
        return rc;
    }

    httpResponse = h.GetStatusCode();

    rc = translateHttpStatusCode(httpResponse);
    if (rc < 0) {
        return rc;
    }

    cJSON2* json_response = cJSON2_Parse(vcsresponse.c_str());
    if (json_response==NULL) {
        LOG_ERROR("cJSON2_Parse error!");
        return -1;
    }

    cJSON2_Delete(json_response);

    return 0;
}

int VCSpostUpload(DocSNGTicket* ticket, string &vcsresponse, string locationUri, VPLFile_offset_t filesize)
{
    return VCSpostUpload("vcs",
                         ticket->docname,
                         ticket->modifyTime,
                         ticket->uid,
                         ticket->did,
                         ticket->baserev_valid,
                         ticket->baserev,
                         ticket->compid,
                         vcsresponse, locationUri, filesize);
}


int VCSuploadPreview(DocSNGTicket* ticket, string metadata)
{

    LOG_INFO("VCSuploadPreview ========================");

    cJSON2* json_response = cJSON2_Parse(metadata.c_str());
    if (json_response==NULL) {
        LOG_ERROR("cJSON2_Parse error!");
        return -1;
    }

    u64 compid = 0;
    if(!JSON_getInt64(json_response, "compId", compid)){
        //cout << "AWSAccessKeyId:" << AWSAccessKeyId << endl;
        LOG_INFO("compId: ================>"FMTu64, compid);
    }else{
        LOG_ERROR("Get compId error!");
        return -1;
    }

    cJSON2* json_revisionlist=NULL;
    if (JSON_getJSONObject(json_response, "revisionList", &json_revisionlist)) {
        LOG_ERROR("cJSON2_Parse error!");
        LOG_ERROR("ErrorPtr: %s", cJSON2_GetErrorPtr());
        return -1;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json_response);

    LOG_INFO("revisionList array size: ================>%d", cJSON2_GetArraySize(json_revisionlist));

    cJSON2* json_revisionlist1=NULL;
    u64 revision = 0;

    for(int i=0; i < cJSON2_GetArraySize(json_revisionlist); i++){

        u64 tmpRevision = 0;
        json_revisionlist1 = cJSON2_GetArrayItem(json_revisionlist, i);

        if(!JSON_getInt64(json_revisionlist1, "revision", tmpRevision)){
            //cout << "AWSAccessKeyId:" << AWSAccessKeyId << endl;
            LOG_INFO("tmpRevision: ================>"FMTu64, tmpRevision);
            if(tmpRevision > revision){
                revision = tmpRevision;
            }
        }else{
            LOG_ERROR("Get revision error!");
            return -1;
        }

    }

    LOG_INFO("revision: ================>"FMTu64, revision);

    //connect to server
    int rc=0;
    VPLHttp2 h;
    int httpResponse = -1;
    string response;
    
    stringstream ss;
    ss.str("");

    string sessionHandle, serviceTicket;
    CHECK_AND_ADD_CREDENTIALS_HEADER(h, ticket->uid, true, false);

    string url;
    ss.str("");
    ss << "https://" << VCS_getServer(ticket->uid, CLOUDDOC_DATASET_NAME) <<
          "/vcs/preview/" << ticket->did << "/" << ticket->docname <<
          "?compId=" << compid << "&revision=" << revision;
    url = ss.str();

    h.SetUri(url);
    h.SetTimeout(VPLTime_FromSec(30));
    
    if (__ccdConfig.debugVcsHttp)
        h.SetDebug(1);

    rc = h.Put(
            ticket->thumbPath.c_str(),
            NULL, //VPLHttp2_ProgressCb
            NULL, //VPLHttp2_ProgressCb context
            response);  // arg to callback
    if (rc < 0) {
        return rc;
    }

    httpResponse = h.GetStatusCode();
    rc = translateHttpStatusCode(httpResponse);

    return rc;
}

int VCSreadFile(stream_transaction *transaction)
{
    std::string response;

    std::string sessionHandle, serviceTicket;
    if(transaction->req->find_header("x-ac-sessionHandle")){
        sessionHandle = *transaction->req->find_header("x-ac-sessionHandle");
    }
    if(transaction->req->find_header("x-ac-serviceTicket")){
        serviceTicket = *transaction->req->find_header("x-ac-serviceTicket");
    }


    int rv = VCSreadFile_helper(transaction->uid,
                                sessionHandle,
                                serviceTicket,
                                transaction->deviceid,
                                transaction->req->uri,
                                response);
    if (rv != 0) {
        generate_rf_response(transaction, response, 400);
    }
    else {
        generate_rf_response(transaction, response, 200);
    }

    return rv;
}

int VCSreadFile_helper(u64 uid,
                       std::string sessionHandle,
                       std::string serviceTicket,
                       u64 deviceid,
                       const std::string &uri,
                       /*OUT*/
                       std::string &response)
{
    u64 datasetId;

    const std::string clouddocPrefix = "/clouddoc/";
    const std::string vcsPrefix = "/vcs/";
    const char *serviceNameOverride = NULL;
    if (uri.compare(0, clouddocPrefix.length(), clouddocPrefix) == 0) {
        int err = VCS_getDatasetID(uid, CLOUDDOC_DATASET_NAME, datasetId);
        if(err < 0){
            LOG_ERROR("Failed to get datasetid: %d", err);
            return -1;  // FIXME
        }
        serviceNameOverride = "clouddoc";
    }
    else if (uri.compare(0, vcsPrefix.length(), vcsPrefix) == 0) {
        // datasetId is in the third part of the uri
        // Example: /vcs/filemetadata/12345/a/b/c
        //                            ^^^^^
        size_t pos = 0;
        for (int i = 0; i < 3; i++) {
            pos = uri.find('/', pos);
            if (pos == uri.npos) {
                LOG_ERROR("Malformed URI %s", uri.c_str());
                return -1;  // FIXME
            }
            pos++;
        }
        if (pos >= uri.length()) {
            LOG_ERROR("Malformed URI %s", uri.c_str());
            return -1;  // FIXME
        }
        datasetId = VPLConv_strToU64(uri.substr(pos).c_str(), NULL, 10);
    }
    else {
        LOG_ERROR("Unexpected service name in uri: %s", uri.c_str());
        return -1;  // FIXME
    }

    return VCSreadFile_helper(uid, datasetId, sessionHandle, serviceTicket, deviceid, uri, serviceNameOverride, response);
}

int VCSreadFile_helper(u64 userId,
                       u64 datasetId,
                       std::string sessionHandle,
                       std::string serviceTicket,
                       u64 deviceid,
                       const std::string &uri,
                       const char *serviceNameOverride,
                       /*OUT*/
                       std::string &response)
{

    VPLHttp2 http;

    LOG_INFO("VCSreadFile ====================================");

    LOG_INFO("transaction uid ======>"FMTu64, static_cast<s64>(userId));
    LOG_INFO("transaction device id======>"FMTu64, deviceid);

    LOG_INFO("request ======>%s", uri.c_str());

    /* URI rewrite rules
       "/clouddoc/aaa/bbb/ccc?ddd -> "/vcs/filemetadata/datasetid/bbb/ccc?ddd"
       "/vcs/aaa/nnn/bbb/ccc?ddd  -> "/vcs/filemetadata/nnn/bbb/ccc?ddd"
     */

    std::ostringstream url_oss;
    url_oss << "https://" << VCS_getServer(userId, datasetId) << "/vcs/filemetadata/";

    const std::string prefix_clouddoc = "/clouddoc/";
    const std::string prefix_vcs = "/vcs/";

    if (uri.compare(0, prefix_clouddoc.length(), prefix_clouddoc) == 0) {
        size_t pos = uri.find('/', prefix_clouddoc.length());
        if (pos == uri.npos) {
            LOG_ERROR("Unexpected format of URI %s", uri.c_str());
            return -1;  // FIXME
        }
        // assert: pos points to third slash
        url_oss << datasetId;
        url_oss << uri.substr(pos);
    }
    else if (uri.compare(0, prefix_vcs.length(), prefix_vcs) == 0) {
        size_t pos = 0;
        for (int i = 0; i < 3; i++) {
            pos = uri.find('/', pos);
            if (pos == uri.npos) {
                LOG_ERROR("Unexpected format of URI %s", uri.c_str());
                return -1;  // FIXME
            }
            pos++;
        }
        // assert: pos points to first digit of nnn (datasetId)
        url_oss << uri.substr(pos);
    }
    else {
        LOG_ERROR("Unexpected prefix in URI %s", uri.c_str());
        return -1;  // FIXME
    }

    if (serviceNameOverride) {
        if (uri.find('?') != uri.npos) {
            url_oss << "&prefixServiceName=" << serviceNameOverride;
        }
        else {
            url_oss << "?prefixServiceName=" << serviceNameOverride;
        }
    }
    std::string url = url_oss.str();

    LOG_DEBUG("rewritten uri %s", url.c_str());
    http.SetUri(url);
    http.SetTimeout(VPLTime_FromSec(30));

    LOG_INFO("VCSreadFile ====================================");

    LOG_INFO("transaction uid ======>"FMTu64, userId);
    LOG_INFO("transaction device id======>"FMTu64, deviceid);

    // assemble auth info in form of http headers
    CHECK_AND_ADD_CREDENTIALS_HEADER(http, userId, true, true);

    //VPLHttpHandle http;
    int httpResponse = -1;
    int err = -1;

    if (__ccdConfig.debugVcsHttp)
        http.SetDebug(1);

    err = http.Get(response);  // arg to callback
    if (err < 0) {
        return err;
    }

    httpResponse = http.GetStatusCode();

    err = translateHttpStatusCode(httpResponse);

    return err;
}

//copy VCSreadFile
int VCSreadFolder(stream_transaction *transaction)
{
    std::string response;

    std::string sessionHandle, serviceTicket;
    if(transaction->req->find_header("x-ac-sessionHandle")){
        sessionHandle = *transaction->req->find_header("x-ac-sessionHandle");
    }
    if(transaction->req->find_header("x-ac-serviceTicket")){
        serviceTicket = *transaction->req->find_header("x-ac-serviceTicket");
    }


    int rv = VCSreadFolder_helper(transaction->uid,
                                sessionHandle,
                                serviceTicket,
                                transaction->deviceid,
                                transaction->req->uri,
                                response);
    if (rv != 0) {
        generate_rf_response(transaction, response, 400);
    }
    else {
        generate_rf_response(transaction, response, 200);
    }

    return rv;
}

//copy VCSreadFile_helper
int VCSreadFolder_helper(u64 uid,
                       std::string sessionHandle,
                       std::string serviceTicket,
                       u64 deviceid,
                       const std::string &uri,
                       /*OUT*/
                       std::string &response)
{
    u64 datasetId;

    const std::string clouddocPrefix = "/clouddoc/";
    const std::string vcsPrefix = "/vcs/";
    const char *serviceNameOverride = NULL;
    if (uri.compare(0, clouddocPrefix.length(), clouddocPrefix) == 0) {
        int err = VCS_getDatasetID(uid, CLOUDDOC_DATASET_NAME, datasetId);
        if(err < 0){
            LOG_ERROR("Failed to get datasetid: %d", err);
            return -1;  // FIXME
        }
        serviceNameOverride = "clouddoc";
    }
    else if (uri.compare(0, vcsPrefix.length(), vcsPrefix) == 0) {
        // datasetId is in the third part of the uri
        // Example: /vcs/filemetadata/12345/a/b/c
        //                            ^^^^^
        size_t pos = 0;
        for (int i = 0; i < 3; i++) {
            pos = uri.find('/', pos);
            if (pos == uri.npos) {
                LOG_ERROR("Malformed URI %s", uri.c_str());
                return -1;  // FIXME
            }
            pos++;
        }
        if (pos >= uri.length()) {
            LOG_ERROR("Malformed URI %s", uri.c_str());
            return -1;  // FIXME
        }
        datasetId = VPLConv_strToU64(uri.substr(pos).c_str(), NULL, 10);
    }
    else {
        LOG_ERROR("Unexpected service name in uri: %s", uri.c_str());
        return -1;  // FIXME
    }

    return VCSreadFolder_helper(uid, datasetId, sessionHandle, serviceTicket, deviceid, uri, serviceNameOverride, response);
}

int VCSreadFolder_helper(u64 userId,
                       u64 datasetId,
                       std::string sessionHandle,
                       std::string serviceTicket,
                       u64 deviceid,
                       const std::string &uri,
                       const char *serviceNameOverride,
                       /*OUT*/
                       std::string &response)
{

    LOG_INFO("VCSreadFolder ====================================");
   
    LOG_INFO("transaction uid ======>"FMTu64, static_cast<s64>(userId));
    LOG_INFO("transaction device id======>"FMTu64, deviceid);


    int rc=0;
    VPLHttp2 h;
    int httpResponse = -1;

    ostringstream ss;
    ss.str("");
    ss << "https://" << VCS_getServer(userId, datasetId) << "/vcs/dir/";

    //check if uri is valid, copy from VCSreadFile_helper
    {
        const std::string prefix_clouddoc = "/clouddoc/";
        const std::string prefix_vcs = "/vcs/";

        if (uri.compare(0, prefix_clouddoc.length(), prefix_clouddoc) == 0) {
            size_t pos = uri.find('/', prefix_clouddoc.length());
            if (pos == uri.npos) {
                LOG_ERROR("Unexpected format of URI %s", uri.c_str());
                return CCD_ERROR_PARAMETER;
            }
            // assert: pos points to third slash
            ss << datasetId;
            ss << uri.substr(pos);
        }
        else if (uri.compare(0, prefix_vcs.length(), prefix_vcs) == 0) {
            size_t pos = 0;
            for (int i = 0; i < 3; i++) {
                pos = uri.find('/', pos);
                if (pos == uri.npos) {
                    LOG_ERROR("Unexpected format of URI %s", uri.c_str());
                    return CCD_ERROR_PARAMETER;
                }
                pos++;
            }
            // assert: pos points to first digit of nnn (datasetId)
            ss << uri.substr(pos);
        }
        else {
            LOG_ERROR("Unexpected prefix in URI %s", uri.c_str());
            return CCD_ERROR_PARAMETER;
        }
    }

    if (serviceNameOverride) {
        if (uri.find('?') != uri.npos) {
            ss << "&prefixServiceName=" << serviceNameOverride;
        }
        else {
            ss << "?prefixServiceName=" << serviceNameOverride;
        }
    }

    string url = ss.str();

    h.SetUri(url);
    h.SetTimeout(VPLTime_FromSec(30));

    CHECK_AND_ADD_CREDENTIALS_HEADER(h, userId, true, true);

    if (__ccdConfig.debugVcsHttp)
        h.SetDebug(1);

    rc = h.Get(response);  // arg to callback
    if (rc < 0) {
        return rc;
    }

    httpResponse = h.GetStatusCode();
    // Remove any VCS error messages.
    if (httpResponse < 200 || httpResponse >= 300) {
        LOG_ERROR("HTTP error: %d", httpResponse);
        response.clear();
    }

    rc = translateHttpStatusCode(httpResponse);

    return rc;
}


int VCSdeleteFile(stream_transaction *transaction)
{
    int rv = -1;

    //Need to transcode the param for delete function
    int httpResponse = -1;
    string response;
    string userReq;

    CHECK_AND_GET_USERREQ("file");

    std::string sessionHandle, serviceTicket;
    if(transaction->req->find_header("x-ac-sessionHandle")){
        sessionHandle = *transaction->req->find_header("x-ac-sessionHandle");
    }
    if(transaction->req->find_header("x-ac-serviceTicket")){
        serviceTicket = *transaction->req->find_header("x-ac-serviceTicket");
    }

    rv = VCSdeleteFile(transaction->uid, sessionHandle, serviceTicket, userReq, response, httpResponse);

    generate_rf_response(transaction, response, httpResponse);

    return rv;
}


int VCSdeleteFile(u64 uid,
                  std::string sessionHandle,
                  std::string serviceTicket,
                  const string &userReq,
                  string &response,
                  int &httpResponse)
{

    LOG_INFO("VCSdeleteFile ====================================");
    LOG_INFO("Server ====================>%s", VCS_getServer(uid, CLOUDDOC_DATASET_NAME).c_str());

    LOG_INFO("transaction uid ===========>"FMTu64, uid);
    LOG_INFO("transaction device id======>"FMTu64, VirtualDevice_GetDeviceId());

    LOG_INFO("request ===================>%s", userReq.c_str());

    //Read datasetId
    u64 datasetId=0;

    int err = VCS_getDatasetID(uid, CLOUDDOC_DATASET_NAME, datasetId);

    if(err < 0){
        LOG_ERROR("Failed to get datasetid, %d", err);
        return err;
    }


    //Connect Server...

    int rc=0;
    VPLHttp2 h;

    stringstream ss;
    ss.str("");
    ss << "https://" << VCS_getServer(uid, CLOUDDOC_DATASET_NAME) << "/vcs/file/" << datasetId << "/" << userReq;
    string url = ss.str();

    h.SetUri(url);
    h.SetTimeout(VPLTime_FromSec(30));

    CHECK_AND_ADD_CREDENTIALS_HEADER(h, uid, true, true);

    if (__ccdConfig.debugVcsHttp)
        h.SetDebug(1);

    rc = h.Delete(
            response);  // arg to callback

    if (rc < 0){
        LOG_ERROR("VPLHttp_Request failed!, %d", rc);
        return rc;
    }

    httpResponse = h.GetStatusCode();

    rc = translateHttpStatusCode(httpResponse);

    return rc;
}

int VCSmoveFile(stream_transaction *transaction)
{

    int rv = -1;
    //Need to transcode the param for move function
    int httpResponse = -1;
    string response;
    string userReq;

    CHECK_AND_GET_USERREQ("file");

    std::string sessionHandle, serviceTicket;
    if(transaction->req->find_header("x-ac-sessionHandle")){
        sessionHandle = *transaction->req->find_header("x-ac-sessionHandle");
    }
    if(transaction->req->find_header("x-ac-serviceTicket")){
        serviceTicket = *transaction->req->find_header("x-ac-serviceTicket");
    }

    rv = VCSmoveFile(transaction->uid, sessionHandle, serviceTicket, userReq, response, httpResponse);

    generate_rf_response(transaction, response, httpResponse);

    return rv;
}


int VCSmoveFile(u64 uid,
                std::string sessionHandle,
                std::string serviceTicket,
                const string &userReq,
                string &response,
                int &httpResponse)
{

    LOG_INFO("VCSmoveFile ====================================");

    LOG_INFO("transaction uid      ======>"FMTu64, static_cast<s64>(uid));
    LOG_INFO("transaction device id======>"FMTu64, VirtualDevice_GetDeviceId());

    LOG_INFO("request ======>%s", userReq.c_str());

    //Read datasetId
    u64 datasetId=0;

    int err = VCS_getDatasetID(uid, CLOUDDOC_DATASET_NAME, datasetId);

    if(err < 0){
        LOG_ERROR("Failed to get datasetid, %d", err);
        return err;
    }


    //Connect Server...

    int rc=0;
    VPLHttp2 h;

    stringstream ss;
    ss.str("");
    ss << "https://" << VCS_getServer(uid, CLOUDDOC_DATASET_NAME) << "/vcs/filemetadata/" << datasetId << "/" << userReq;
    if (userReq.empty()) {
        ss << "?prefixServiceName=clouddoc";
    }
    else {
        ss << "&prefixServiceName=clouddoc";
    }
    string url = ss.str();

    h.SetUri(url);
    h.SetTimeout(VPLTime_FromSec(30));

    char *out = NULL;
    ss.str("");

    ss << "{"
       << "\"updateDevice\":" << VirtualDevice_GetDeviceId() 
       << "}";
    
    out = strdup(ss.str().c_str());
    ON_BLOCK_EXIT(free, out);

    CHECK_AND_ADD_CREDENTIALS_HEADER(h, uid, true, true);

    if (__ccdConfig.debugVcsHttp)
        h.SetDebug(1);

    rc = h.Post(
            string(out),      // payload
            response);  // arg to callback

    if (rc < 0){
        LOG_ERROR("VPLHttp_Request failed!, %d", rc);
        return rc;
    }

    httpResponse = h.GetStatusCode();

    rc = translateHttpStatusCode(httpResponse);

    return rc;
}

int VCSdownloadPreview(stream_transaction *transaction)
{

    LOG_INFO("VCSdownloadPreview====================================");

    LOG_INFO("transaction uid ======>"FMTu64, static_cast<s64>(transaction->uid));
    LOG_INFO("transaction device id======>"FMTu64, transaction->deviceid);

    LOG_INFO("request ======>%s", transaction->req->uri.c_str());

    string userReq;

    CHECK_AND_GET_USERREQ("preview");

    //Read datasetId
    u64 datasetId=0;

    LOG_INFO("VCSreadFile ====================================");

    LOG_INFO("transaction uid ======>"FMTu64, transaction->uid);
    LOG_INFO("transaction device id======>"FMTu64, transaction->deviceid);

    int err = VCS_getDatasetID(transaction->uid, CLOUDDOC_DATASET_NAME, datasetId);

    if(err < 0){
        LOG_ERROR("Failed to get datasetid");
        return -1;
    }


    //Connect Server...

    int rc=0;
    VPLHttp2 h;
    int httpResponse = -1;
    string response;

    stringstream ss;
    ss.str("");
    ss << "https://" << VCS_getServer(transaction->uid, CLOUDDOC_DATASET_NAME) << "/vcs/preview/" << userReq;
    string url = ss.str();

    h.SetUri(url);
    h.SetTimeout(VPLTime_FromSec(30));

    string sessionHandle = *transaction->req->find_header("x-ac-sessionHandle");
    string serviceTicket = *transaction->req->find_header("x-ac-serviceTicket");
    CHECK_AND_ADD_CREDENTIALS_HEADER(h, transaction->uid, false, false);

    if (__ccdConfig.debugVcsHttp)
        h.SetDebug(1);

    rc = h.Get(response);  // arg to callback
    if (rc < 0) {
        return rc;
    }

    httpResponse = h.GetStatusCode();

    generate_rf_response(transaction, response, httpResponse);

    rc = translateHttpStatusCode(httpResponse);

    return rc;
}

int VCSgetDatasetChanges(u64 userid,
                         std::string sessionHandle,
                         std::string serviceTicket,
                         u64 datasetid,
                         u64 changeSince,
                         int max,
                         bool includeDeleted,
                         int &httpResponse,
                         std::string &response)
{
    int err = 0;
    VPLHttp2 http;

    // construct url
    std::string url;
    {
        std::ostringstream oss;
        oss << "https://" << VCS_getServer(userid, CLOUDDOC_DATASET_NAME) << "/vcs/datasetchanges/" << datasetid ;

        bool first = true;
        if(changeSince){
            oss << (first?"?":"&");
            oss << "changeSince=" << changeSince;
            first = false;
        }
        if(max) {
            oss << (first?"?":"&");
            oss << "max=" << max;
            first = false;
        }
        if(includeDeleted) { //default: false
            oss << (first?"?":"&");
            oss << "includeDeleted=true";
            first = false;
        }
        url = oss.str();
    }

    http.SetUri(url);
    http.SetTimeout(VPLTime_FromSec(30));

    // assemble auth info in form of http headers
    CHECK_AND_ADD_CREDENTIALS_HEADER(http, userid, true, true);

    //VPLHttpHandle http;
    httpResponse = -1;

    if (__ccdConfig.debugVcsHttp)
        http.SetDebug(1);

    err = http.Get(response);  // arg to callback
    if (err < 0) {
        return err;
    }

    httpResponse = http.GetStatusCode();
    err = translateHttpStatusCode(httpResponse);

    return err;
}

int VCSgetAccessInfo(u64 userid,
                     const std::string &sessionHandle,
                     const std::string &serviceTicket,
                     const std::string &method,
                     const std::string &path_query,
                     const std::string &contentType,
                     /*OUT*/
                     std::string &accessurl,
                     vector<std::string> &header,
                     std::string &locationname)
{

    vector<string> headerName;
    vector<string> headerValue;

    int ret = -1;
    
    ret = VCSgetAccessInfo(userid,
                           sessionHandle,
                           serviceTicket,
                           method,
                           path_query,
                           contentType,
                           accessurl,
                           headerName,
                           headerValue,
                           locationname);
    if(ret < 0){
        return ret;
    }

    for(size_t i=0; i<headerName.size(); i++){

        header.push_back(headerName[i]+":"+headerValue[i]);

    }

    return 0;
}

int VCSgetAccessInfo(u64 userid,
                     u64 datasetid,
                     const std::string &sessionHandle,
                     const std::string &serviceTicket,
                     const std::string &method,
                     const std::string &path_query,
                     const std::string &contentType,
                     /*OUT*/
                     std::string &accessurl,
                     vector<std::string> &headerName,
                     vector<std::string> &headerValue,
                     std::string &locationname)
{
    return VCSgetAccessInfo(userid,
                           datasetid,
                           sessionHandle,
                           serviceTicket,
                           method,
                           path_query,
                           contentType,
                           "",
                           "",
                           accessurl,
                           headerName,
                           headerValue,
                           locationname);
}

int VCSgetAccessInfo(u64 userid,
                     u64 datasetid,
                     const std::string &sessionHandle,
                     const std::string &serviceTicket,
                     const std::string &method,
                     const std::string &path_query,
                     const std::string &contentType,
                     const std::string &categoryName,
                     const std::string &protocolVersion,
                     /*OUT*/
                     std::string &accessurl,
                     vector<std::string> &headerName,
                     vector<std::string> &headerValue,
                     std::string &locationname)
{
    int err = 0;
    VPLHttp2 http;

    // construct url
    std::string url;
    {
        std::ostringstream oss;
        oss << "https://" << VCS_getServer(userid, datasetid) << "/vcs/";
        if (categoryName.size() > 0) {
            oss << categoryName << "/";
            http.AddRequestHeader("X-ac-version", protocolVersion);
        }
        oss << "accessinfo/" << datasetid << "/";

        if(path_query.empty()){
            oss << "?method=" << method;
        }else{
            // The path_query might not contains any parameter
            oss << path_query;
            if (path_query.find("?") == string::npos) {
                oss << "?method=" << method;
            } else {
                oss << "&method=" << method;
            }
        }
        url = oss.str();
    }

    http.SetUri(url);
    http.SetTimeout(VPLTime_FromSec(30));

    // assemble auth info in form of http headers
    CHECK_AND_ADD_CREDENTIALS_HEADER(http, userid, true, true);
    size_t nr_headers = 0;

    std::string http_header_contentType;
    if(!contentType.empty()){
        std::ostringstream oss;
        oss << contentType;
        http_header_contentType.assign(oss.str());
        http.AddRequestHeader("contentType", http_header_contentType);
        nr_headers++;
    }


    //VPLHttpHandle http;
    int httpResponse = -1;
    std::string response;

    if (__ccdConfig.debugVcsHttp)
        http.SetDebug(1);

    err = http.Get(response);  // arg to callback

    if (err < 0){
        LOG_ERROR("VPLHttp_Request failed!, %d", err);
        return err;
    }

    httpResponse = http.GetStatusCode();

    err = translateHttpStatusCode(httpResponse);
    if (err < 0) {
        return err;
    }

    cJSON2* json_response = cJSON2_Parse(response.c_str());
    if (json_response == NULL) {
        LOG_ERROR("cJSON2_Parse error!");
        return -1;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json_response);


    cJSON2* json_header;
    if (!JSON_getString(json_response, "accessUrl", accessurl) &&
        !JSON_getJSONObject(json_response, "header", &json_header)) {

        LOG_DEBUG("accessUrl=%s, header=%s, locationName=%s",
                accessurl.c_str(), json_header->valuestring, locationname.c_str());

        for(int i=0; i < cJSON2_GetArraySize(json_header); i++){
            LOG_INFO("%s:%s", cJSON2_GetArrayItem(json_header, i)->string, cJSON2_GetArrayItem(json_header, i)->valuestring);
            headerName.push_back(cJSON2_GetArrayItem(json_header, i)->string);
            headerValue.push_back(cJSON2_GetArrayItem(json_header, i)->valuestring);
            
            //header.push_back(tmpHeader);
        }
    } else {
        LOG_ERROR("missing parameter(s)");
        return -1;
    }

    return err;
}

// DEPRECATED
int VCSgetAccessInfo(u64 userid,
                     const std::string &sessionHandle,
                     const std::string &serviceTicket,
                     const std::string &method,
                     const std::string &path_query,
                     const std::string &contentType,
                     /*OUT*/
                     std::string &accessurl,
                     vector<std::string> &headerName,
                     vector<std::string> &headerValue,
                     std::string &locationname)
{
    int err = 0;

    u64 datasetid = 0;

    err = VCS_getDatasetID(userid, CLOUDDOC_DATASET_NAME, datasetid);

    if(err < 0){
        LOG_ERROR("Failed to get datasetid");
        return -1;  
    }
    return VCSgetAccessInfo(userid,
                            datasetid,
                            sessionHandle,
                            serviceTicket,
                            method,
                            path_query,
                            contentType,
                            "",
                            "",
                            accessurl,
                            headerName,
                            headerValue,
                            locationname);
}

int VCSdummy()
{
    return 0;
}


extern struct StorageProxy s3_proxy;

VPLTHREAD_FN_DECL VcsCloudDoc_HandleTicket(void *vpTicket)
{
    int rv = -1;
    DocSNGTicket* ticket = (DocSNGTicket*)(vpTicket);

    LOG_INFO("Ticket uid ======>"FMTu64, ticket->uid);
    LOG_INFO("Ticket did ======>"FMTu64, ticket->did);
    LOG_INFO("Ticket dloc ======>%s", (ticket->dloc).c_str());
    LOG_INFO("Ticket srcPath ======>%s", (ticket->srcPath).c_str());
    LOG_INFO("Ticket device_id ======>"FMTu64, VirtualDevice_GetDeviceId());
    if(ticket->operation == DOC_SNG_UPLOAD){
        LOG_INFO("Ticket operation ======> is upload");
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        // Create a computeSHA1 thread to compute SHA1 of the file
        // Thus, the vcs_handler and computeSHA1 can do at the same time.
        // Theoretically, computeSHA1 should be done before upload complete.
        // However, still put a thread_join before calling the completeTicket callback
        VPLThread_t computeFileSHA1;
        void* rval;
        rv = VPLThread_Create(&computeFileSHA1,
            computeSHA1,
            vpTicket,
            0, // default VPLThread thread-attributes: prio, stack-size, etc.
            "Compute the SHA1 of the File");
        if(rv != VPL_OK)
        {
            LOG_ERROR("Cannot create computeSHA1 thread, error = %d", rv);
        }
#endif
        rv = s3_proxy.uploadFile(ticket, "image/jpeg");
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        VPLThread_Join(&computeFileSHA1, &rval);
#endif
    }else if(ticket->operation == DOC_SNG_DELETE){
        
        int httpResponse = -1;
        string response;
        string userReq;
        
        stringstream ss;
        ss.str("");
        ss << ticket->docname;
        if (ticket->compid != 0) {
            ss << "?compId=" << ticket->compid;
        }
        if (ticket->baserev_valid) {
            ss << "&revision=" << ticket->baserev;
        }
        userReq = ss.str();
        LOG_INFO("userReq ===============> %s", userReq.c_str());
       
        rv = VCSdeleteFile(ticket->uid, "", "", userReq, response, httpResponse);

    }else if(ticket->operation == DOC_SNG_RENAME){

        int httpResponse = -1;
        string response;
        string userReq;

        LOG_INFO("origName ===============> %s", ticket->origName.c_str());
        LOG_INFO("newName  ===============> %s", ticket->newName.c_str());

        //// Clean up the path
        std::string doc2 = ticket->newName;         // The fullpath replacing all backslash with forward slash
        // Replace backslash with forward slash
        std::replace(doc2.begin(), doc2.end(), '\\', '/');

        // Reject the (file) name ended with slash
        if(*doc2.rbegin() == '/'){
            LOG_ERROR("bad path");
            rv = -1;
            goto end;
        }

        // check path are absolute
#ifdef WIN32
        if (doc2.length() < 3 || doc2[1] != ':' || doc2[2] != '/'){
            LOG_ERROR("bad path");
            rv = -1;
            goto end;
        }
#else
        if (doc2.empty() || doc2[0] != '/'){
            LOG_ERROR("bad path: %s", doc2.c_str());
            rv = -1;
            goto end;
        }
#endif

        std::string doc3 = doc2;        // for docname
#ifdef WIN32
        // remove colon after drive letter
        // e.g., C:/Users/fokushi/test.docx -> C/Users/fokushi/test.docx
        doc3.erase(1, 1);
#else
        // strip initial slash
        // e.g., /home/fokushi/test.txt -> home/fokushi/test.txt
        doc3.erase(0, 1);
#endif
        stringstream ss;
        ss.str("");

        string deviceId;
        deviceId.assign(ticket->docname);
        u32 index = deviceId.find("/");
        if (index != std::string::npos) {
            deviceId.resize(index);
            ss << deviceId;
        } else {
            ss << VirtualDevice_GetDeviceId();
        }
        
        ss << "/";
        ss << VPLHttp_UrlEncoding(doc3, "/");
        ss << "?moveFrom=" << ticket->docname;
        if(ticket->compid != 0) {
            ss << "&compId=" << ticket->compid;
        }
        if (ticket->baserev_valid !=0) {
            ss << "&revision=" << ticket->baserev;
        }
        userReq = ss.str();
        LOG_INFO("userReq ===============> %s", userReq.c_str());
       
        rv = VCSmoveFile(ticket->uid, "", "", userReq, response, httpResponse);

        if(rv == 0)
        {
            // parse vcsresponse to get compid and latest revision number
            cJSON2 *json_resp = cJSON2_Parse(response.c_str());
            if (json_resp) {
                cJSON2 *json_compid = cJSON2_GetObjectItem(json_resp, "compId");
                if (json_compid) {
                    ticket->compid = (u64)json_compid->valueint;
                }
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
                cJSON2 *json_origin_device = cJSON2_GetObjectItem(json_resp, "originDevice");
                if (json_origin_device) {
                    ticket->origin_device = (u64)json_origin_device->valueint;
                }
                cJSON2 *json_name = cJSON2_GetObjectItem(json_resp, "name");
                if(json_name) {
                    ticket->docname = json_name->valuestring;
                }
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC
                u64 revnum = 0;
                u64 size = 0;
                u64 mtime = 0;
                u64 update_device = 0;
                cJSON2 *json_revarray = cJSON2_GetObjectItem(json_resp, "revisionList");
                if (json_revarray) {
                    int revarray_size = cJSON2_GetArraySize(json_revarray);
                    for (int i = 0; i < revarray_size; i++) {
                        cJSON2 *json_rev = cJSON2_GetArrayItem(json_revarray, i);
                        if (json_rev) {
                            cJSON2 *json_revnum = cJSON2_GetObjectItem(json_rev, "revision");
                            if (json_revnum) {
                                if ((u64)json_revnum->valueint > revnum) {
                                    revnum = (u64)json_revnum->valueint;
                                }
                            }
                            cJSON2 *json_size = cJSON2_GetObjectItem(json_rev, "size");
                            if (json_size) {
                                size = (u64)json_size->valueint;
                            }
                            cJSON2 *json_mtime = cJSON2_GetObjectItem(json_rev, "lastChanged");
                            if (json_mtime) {
                                mtime = (u64)json_mtime->valueint;
                            }
                            cJSON2 *json_update_device = cJSON2_GetObjectItem(json_rev, "updateDevice");
                            if (json_update_device) {
                                update_device = (u64)json_update_device->valueint;
                            }
                        }
                    }
                    if (revnum != 0) {
                        ticket->baserev = revnum;
                    }
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
                    ticket->size = size;
                    ticket->update_device = update_device;
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC
                    ticket->modifyTime = mtime;
                }
                cJSON2_Delete(json_resp);
            }
        }
    } else {
        LOG_INFO("Ticket operation ======> is NOT upload/rename/delete");
        rv = -1;  // redundant but play it safe
    }

end:
    // Determine return value and call either completeTicket or resetTicket
    if (rv == 0) {
        ticket->result = VSSI_SUCCESS;
        ticket->parentQ->completeTicket(ticket);
    } else if (rv == VPL_ERR_CANCELED) {
        // Drop the ticket if the operation was cancelled by request
        LOG_INFO("VPL_ERR_CANCELED!");
        ticket->result = VSSI_ABORTED;
        ticket->parentQ->completeTicket(ticket);
    } else if (rv > 0
            || rv == CCD_ERROR_HTTP_STATUS
            || rv == CCD_ERROR_BAD_SERVER_RESPONSE) {
        //
        // Positive return or other unretryable errors ==> drop the job/ticket
        //
        ticket->result = CCD_ERROR_HTTP_STATUS;
        ticket->parentQ->completeTicket(ticket);
    } else if (rv == CCD_ERROR_TRANSIENT) {
        LOG_ERROR("Retry right away for transient server error");
        ticket->result = CCD_ERROR_TRANSIENT;
        ticket->parentQ->resetTicket(ticket);
    } else {
#if 0
            rv == VPL_ERR_UNREACH
            || rv == VPL_ERR_CONNREFUSED
            || rv == VPL_ERR_NETDOWN
            || rv == VPL_ERR_TIMEOUT
            || rv == VPL_ERR_SSL
            || rv == VPL_ERR_SSL_DATE_INVALID
            || rv == VPL_ERR_IO
            || rv == VSSI_NOSPACE
            || rv == VSSI_COMM
            || rv == VSSI_NOMEM
            || rv == VSSI_HLIMIT
            || rv == VSSI_INIT
            || rv == VSSI_CMDLIMIT
            || rv == VSSI_ABORTED
            || rv == VSSI_TIMEOUT
            || rv == VSSI_NET_DOWN
            || rv == VSSI_AGAIN
            || rv == VSCORE_ERR_AGAIN
#endif
        //
        // All other errors are retried after temporarily suspend processing
        //
        LOG_ERROR("Retry later for network error");
        ticket->result = VSSI_AGAIN;
        ticket->parentQ->resetTicket(ticket);
    }

    return VPLTHREAD_RETURN_VALUE;
}

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
static VPLThread_return_t computeSHA1(void* arg)
{
    DocSNGTicket* ticket = (DocSNGTicket*)(arg);
    CSL_ShaContext hashCtx;
    u8 hashVal[CSL_SHA1_DIGESTSIZE];   // Really SHA1, but be on the safe side
    std::string fileHash;

    VPLFile_handle_t fh;
    size_t n;
    unsigned char buf[1024];

    fh = VPLFile_Open(ticket->srcPath.c_str(), VPLFILE_OPENFLAG_READONLY, 0666);
    if (!VPLFile_IsValidHandle(fh)) {
        LOG_ERROR("Error opening file: %s", ticket->srcPath.c_str());
        return VPLTHREAD_RETURN_VALUE_UNUSED;
    } else {
        CSL_ResetSha(&hashCtx);
        while((n = VPLFile_Read(fh, buf, sizeof(buf))) > 0)
            CSL_InputSha(&hashCtx, buf, n);
        CSL_ResultSha(&hashCtx, hashVal);
    }
    
    ticket->hashval.clear();
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        char byteStr[4];
        snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, hashVal[hashIndex]);
    ticket->hashval.append(byteStr);
    }

    VPLFile_Close(fh);
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}
#endif

// For Photo 3.0, PicStream downloads thumbnail from VCS server.
//copy from s3_write_file_callback
static size_t vcs_write_file_callback(const void *buf, size_t size, size_t nmemb, void *param)
{
    stream_transaction *transaction = (stream_transaction*)param;

    // BEGIN TEMPORARY CODE: DO NOT DEPEND ON THIS CODE TO BE HERE FOR LONG
    // temporarily added to support SyncBack
    if (VPLFile_IsValidHandle(transaction->out_content_file)) {
        size_t bytes_total = size * nmemb;
        size_t bytes_remaining = bytes_total;
        while (bytes_remaining > 0) {
            ssize_t bytes_written = VPLFile_Write(transaction->out_content_file,
                                                  (char*)buf + bytes_total - bytes_remaining,
                                                  bytes_remaining);
            if (bytes_written < 0) {  // file i/o error - fail
                return 0;
            }
            bytes_remaining -= bytes_written;
        }
        return nmemb;
    }
    // END TEMPORARY CODE

    VPLMutex_Lock(&transaction->mutex);

    while (transaction->resp.content.size() > 32 * 1024) {
        int rv = VPLCond_TimedWait(&transaction->cond_resp, &transaction->mutex, VPLTime_FromSec(30));
        if (rv != VPL_OK) { // includes VPL_ERR_TIMEOUT
            // give up
            nmemb = 0;
            goto out;
        }
    }

    transaction->resp.add_content((const char*)buf, size * nmemb);

    if (transaction->resp.response == 500) {  // default value, meaning response has not been set yet
        transaction->resp.add_response(transaction->resp.find_header("Range") ? 206 : 200);

        // start sending response
        transaction->sending = true;
    }

 out:
    VPLMutex_Unlock(&transaction->mutex);

    return nmemb;
}

//copy from s3_write_file_callback
static s32 vcs_write_file_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    LOG_DEBUG("http="FMT0xPTR", ctx="FMT0xPTR", buf="FMT0xPTR", size="FMTu32, http, ctx, buf, size);
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    return vcs_write_file_callback(buf, size, 1, ctx) * size;
}

//copy from s3_progress_callback
static void vcs_progress_callback(VPLHttp2 *http, void *ctx, u64 total, u64 sofar)
{
    LOG_DEBUG("vcs GET Progress CB: http("FMT0xPTR"), ctx("FMT0xPTR"), total("FMTu64"), sofar("FMTu64")", http, ctx, total, sofar);
    return ;
}

int VCSgetPreview(stream_transaction *transaction, u64 datasetId)
{

    LOG_INFO("request ======>%s", transaction->req->uri.c_str());

    string userReq;

    CHECK_AND_GET_USERREQ("preview");

    LOG_INFO("VCSgetPreview ====================================");

    LOG_INFO("transaction uid ======>"FMTu64, transaction->uid);
    LOG_INFO("transaction device id======>"FMTu64, transaction->deviceid);
    LOG_INFO("dataset id======>"FMTu64, datasetId);

    int rc=0;
    VPLHttp2 _h;
    VPLHttp2 &h = transaction->http2 ? *transaction->http2 : _h;

    string response;

    stringstream ss;
    ss.str("");
    ss << "https://" << VCS_getServer(transaction->uid, datasetId) << "/vcs/preview/" << userReq;
    string url = ss.str();

    LOG_INFO("url=%s", url.c_str());

    std::string sessionHandle, serviceTicket;
    CHECK_AND_ADD_CREDENTIALS_HEADER(h, transaction->uid, true, true);

    if (__ccdConfig.debugAcsHttp)
        h.SetDebug(1);

    h.SetUri(url);
    h.SetTimeout(VPLTime_FromSec(30));

    rc = h.Get(vcs_write_file_callback, transaction, vcs_progress_callback, NULL);
    if (rc < 0) {
        return rc;
    }

    VPLMutex_Lock(&transaction->mutex);
    transaction->processing = false;
    transaction->sending = true;
    VPLMutex_Unlock(&transaction->mutex);

    if (rc < 0){
        LOG_ERROR("VPLHttp_Request failed!");
        return rc;
    }

    rc = translateHttpStatusCode(h.GetStatusCode());

    return rc;
}
