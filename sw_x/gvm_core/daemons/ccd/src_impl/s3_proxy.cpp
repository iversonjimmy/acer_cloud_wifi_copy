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

#include <iostream>
#include <string.h>
#include <sstream>
#include <time.h>


#include <vplex_serialization.h>
#include <vpl_plat.h>
#include <vplu_types.h>
#include <vplex_file.h>
#include <vplex_syslog.h>
#include <vpl_fs.h>
#include <vplu_mutex_autolock.hpp>

#include <cslsha.h>
#include <log.h>

#include "ccd_util.hpp"
#include "storage_proxy.hpp"
#include "vcs_util.hpp"
#include "vcs_utils.hpp"
#include "vcs_proxy.hpp"
#include "s3_proxy.hpp"
#include "virtual_device.hpp"
#include "cache.h"
#include "config.h"
#include "HttpStream.hpp"
#include "JsonHelper.hpp"

#include <CloudDocMgr.hpp>
#include <vssi_error.h>

#include <vplex_http2.hpp>


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

#if 0
static size_t s3_write_callback(const void *buf, size_t size, size_t nmemb, void *param)
{
    std::string *strResponse = (std::string*)param;
    size_t numBytes = size * nmemb;

    if ((strResponse != NULL) && (buf != NULL)) {
        (*strResponse).append(static_cast<const char*>(buf), numBytes);
        LOG_INFO("response: ================>%s", strResponse->c_str());
    } else {
        LOG_ERROR("VPLHttp_Request callback null param: buf 0x%x, param 0x%x, numBytes %d", (int)buf, (int)param, (int)numBytes);
    }

    return nmemb;
}
#endif


static size_t s3_write_file_callback(const void *buf, size_t size, size_t nmemb, void *param)
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


static size_t s3_write_file_and_httpstream_callback(const void *buf, size_t size, size_t nmemb, void *param)
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
                LOG_ERROR("Write to File Failed! (%d):%d, still write to HttpStream.", (int)bytes_remaining, (int)bytes_written);
                //invalid the handle, so the caller can verify if writing file is failed.
                VPLFile_Close(transaction->out_content_file);
                transaction->out_content_file = NULL;
                goto write_httpstream;
            }
            bytes_remaining -= bytes_written;
        }
        //return nmemb;
    } else {
        LOG_WARN("No valid file handle, write to HttpStream only!");
    }
    // END TEMPORARY CODE

write_httpstream:

    if(transaction->origHs) {

        if(!transaction->isSetOrigHsStatusCode) {
            transaction->origHs->SetStatusCode(200);
            transaction->isSetOrigHsStatusCode = true;
        }

        size_t bytes_total = size * nmemb;
        ssize_t bytes_written = transaction->origHs->Write((char*)buf, bytes_total);

        LOG_DEBUG("Write to HttpStream size:%d", (int)bytes_total);

        if (bytes_written != bytes_total) {
            LOG_ERROR("Write to HS Failed! (%d):%d", (int)bytes_total, (int)bytes_written);
            transaction->origHs->SetStatusCode(500);
            return 0;
        }
    }

    return nmemb;
}

static void s3_progress_callback(VPLHttp2 *http, void *ctx, u64 total, u64 sofar)
{
    LOG_DEBUG("S3 GET Progress CB: http("FMT0xPTR"), ctx("FMT0xPTR"), total("FMTu64"), sofar("FMTu64")", http, ctx, total, sofar);
    return ;
}

static void s3_put_progress_callback(VPLHttp2 *http, void *ctx, u64 total, u64 sofar)
{
    DocSNGTicket* ticket = (DocSNGTicket*)ctx;

    ticket->inProgress = true;  
    ticket->uploadFileSize = total;  
    ticket->uploadFileProgress = sofar;
    LOG_DEBUG("S3 PUT Progress CB: http("FMT0xPTR"), ctx("FMT0xPTR"), total("FMTu64"), sofar("FMTu64")", http, ctx, total, sofar);
    return ;
}

static s32 s3_write_file_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    LOG_DEBUG("http="FMT0xPTR", ctx="FMT0xPTR", buf="FMT0xPTR", size="FMTu32, http, ctx, buf, size);
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    return s3_write_file_callback(buf, size, 1, ctx) * size;

}

static s32 s3_write_file_and_httpstream_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    LOG_DEBUG("http="FMT0xPTR", ctx="FMT0xPTR", buf="FMT0xPTR", size="FMTu32, http, ctx, buf, size);
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    return s3_write_file_and_httpstream_callback(buf, size, 1, ctx) * size;

}

int S3_getFile(stream_transaction* transaction, string bucket, string filename)
{
    int rc=0;

    //We need to get meta from VCS first...
    std::string response;
    std::string sessionHandle, serviceTicket;
    if(transaction->req->find_header("x-ac-sessionHandle")){
        sessionHandle = *transaction->req->find_header("x-ac-sessionHandle");
    }
    if(transaction->req->find_header("x-ac-serviceTicket")){
        serviceTicket = *transaction->req->find_header("x-ac-serviceTicket");
    }

    rc = VCSreadFile_helper(transaction->uid, sessionHandle, serviceTicket, transaction->deviceid, transaction->req->uri, response);
    if (rc < 0) {
        return rc;
    }

    LOG_INFO("response: ================>%s", response.c_str());

    //We need to get meta from 
    cJSON2* json_response = cJSON2_Parse(response.c_str());
    if (json_response==NULL) {
        LOG_ERROR("cJSON2_Parse error!");
        LOG_ERROR("ErrorPtr: %s", cJSON2_GetErrorPtr());
        return -1;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json_response);


    u64 filesize = 0;  // FIXME: u64 - but JSON needs to be fixed first.


    cJSON2* revisionlist=NULL;
    if (JSON_getJSONObject(json_response, "revisionList", &revisionlist)) {
        LOG_ERROR("cJSON2_Parse error!");
        LOG_ERROR("ErrorPtr: %s", cJSON2_GetErrorPtr());
        return -1;
    }

    LOG_INFO("revisionList array size: ================>%d", cJSON2_GetArraySize(revisionlist));

    cJSON2* revisionlist1=NULL;
    revisionlist1 = cJSON2_GetArrayItem(revisionlist, 0);

    if(!JSON_getInt64(revisionlist1, "size", filesize)){
        LOG_INFO("size: ================>"FMTu64, filesize);
    }else{
        LOG_ERROR("Can not find size, error!");
        return -1;
    }


    /*
      Split URI into datasetid (if present) and else.
      "/clouddoc/file/aaa/bbb?ccc" -> (null,"aaa/bbb?ccc")
      "/vcs/file/nnn/aaa/bbb?ccc"  -> (nnn,"aaa/bbb?ccc")
     */
    const std::string &req_uri = transaction->req->uri;
    const std::string prefix_clouddoc_file = "/clouddoc/file/";
    const std::string prefix_vcs_file = "/vcs/file/";
    u64 datasetId = 0;
    std::string path_query;
    if (req_uri.compare(0, prefix_clouddoc_file.length(), prefix_clouddoc_file) == 0) {
        path_query.assign(req_uri.substr(prefix_clouddoc_file.length()));
        int err = VCS_getDatasetID(transaction->uid, "Cloud Doc", datasetId);
        if (err) {
            LOG_ERROR("Failed to find \"Cloud Doc\" dataset: %d", err);
            return err;
        }
    }
    else if (req_uri.compare(0, prefix_vcs_file.length(), prefix_vcs_file) == 0) {
        size_t pos = req_uri.find('/', prefix_vcs_file.length());
        if (pos == req_uri.npos) {
            LOG_ERROR("Unexpected URI %s", req_uri.c_str());
            return -1;  // FIXME
        }
        path_query.assign(req_uri, pos + 1, req_uri.npos);
        datasetId = VPLConv_strToU64(req_uri.substr(prefix_vcs_file.length()).c_str(), NULL, 10);
    }
    else {
        LOG_ERROR("Unexpected URI %s", req_uri.c_str());
        return -1;  // FIXME
    }
    LOG_DEBUG("datasetId="FMTu64, datasetId);
    LOG_DEBUG("path_query=%s", path_query.c_str());

    string accessUrl, locationName;
    int err = 0;

    vector<string> headerName, headerValue;
    err = VCSgetAccessInfo(transaction->uid, datasetId, sessionHandle, serviceTicket, "GET", path_query, "", accessUrl, headerName, headerValue, locationName);

    if(err != 0){
        LOG_ERROR("VCSgetAccessInfo Failed");
        return -1;
    }


    VPLHttp2 _h;
    VPLHttp2 &h = transaction->http2 ? *transaction->http2 : _h;

    if (__ccdConfig.debugAcsHttp)
        h.SetDebug(1);

    for(size_t i=0; i<headerName.size(); i++){
        h.AddRequestHeader(headerName[i], headerValue[i]);
    }


    std::string range;
    const string* reqRange = NULL;
    reqRange = transaction->req->find_header("Range");
    if(reqRange){
        LOG_INFO("Range request found: %s", reqRange->c_str());
        int ret = -1;
        u64 start=0, end=0;
        ret = Util_ParseRange(transaction->req->find_header("Range"), filesize, start, end);
        if(ret == 0){
            h.AddRequestHeader("Range", *reqRange);
        }else{
            LOG_ERROR("Parse fail, range: %s", transaction->req->find_header("Range")->c_str());
            LOG_ERROR("Skip range header!!");
        }
    }

    {
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
        char contentLengthValue[128];
        sprintf(contentLengthValue, FMTu64, filesize);
        if(transaction->req->find_header("Range")){
            int ret=-1;
            u64 start=0, end=0;
            ret = Util_ParseRange(transaction->req->find_header("Range"), filesize, start, end);
            if(ret == 0){
                //over ride contentLengthValue
                sprintf(contentLengthValue, FMTu64, end-start+1);
                LOG_INFO("Range:================>%s", transaction->req->find_header("Range")->c_str());
                LOG_INFO("FileSize:=============>"FMTu64, filesize);
                LOG_INFO("Start:================>"FMTu64, start);
                LOG_INFO("End:  ================>"FMTu64, end);
                LOG_INFO("contentLengthValue: ================>%s", contentLengthValue);
            }else{
                LOG_ERROR("Parse fail, range: %s", transaction->req->find_header("Range")->c_str());
            }
        }
        transaction->resp.add_header("Content-Length", contentLengthValue); 
    }

    h.SetUri(accessUrl);
    h.SetTimeout(VPLTime_FromSec(30));
    // Blocking call, can take a while.
    rc = h.Get(s3_write_file_callback, transaction, s3_progress_callback, NULL);

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

//
// This function use s3_write_file_and_httpstream_callback.
// If transaction->out_content_file is valid, the result will write to file
// If transaction->origHs is valid, the result will write to the original httpstream
//
int S3_getFile2HttpStreamAndFile(stream_transaction* transaction, std::string bucket, std::string filename)
{//copy from S3_getFile
    int rc=0;

    //We need to get meta from VCS first...
    std::string response;
    std::string sessionHandle, serviceTicket;
    if(transaction->req->find_header("x-ac-sessionHandle")){
        sessionHandle = *transaction->req->find_header("x-ac-sessionHandle");
    }
    if(transaction->req->find_header("x-ac-serviceTicket")){
        serviceTicket = *transaction->req->find_header("x-ac-serviceTicket");
    }

    rc = VCSreadFile_helper(transaction->uid, sessionHandle, serviceTicket, transaction->deviceid, transaction->req->uri, response);
    if (rc < 0) {
        return rc;
    }

    LOG_INFO("response: ================>%s", response.c_str());

    //We need to get meta from 
    cJSON2* json_response = cJSON2_Parse(response.c_str());
    if (json_response==NULL) {
        LOG_ERROR("cJSON2_Parse error!");
        LOG_ERROR("ErrorPtr: %s", cJSON2_GetErrorPtr());
        return -1;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json_response);


    u64 filesize = 0;  // FIXME: u64 - but JSON needs to be fixed first.


    cJSON2* revisionlist=NULL;
    if (JSON_getJSONObject(json_response, "revisionList", &revisionlist)) {
        LOG_ERROR("cJSON2_Parse error!");
        LOG_ERROR("ErrorPtr: %s", cJSON2_GetErrorPtr());
        return -1;
    }

    LOG_INFO("revisionList array size: ================>%d", cJSON2_GetArraySize(revisionlist));

    cJSON2* revisionlist1=NULL;
    revisionlist1 = cJSON2_GetArrayItem(revisionlist, 0);

    if(!JSON_getInt64(revisionlist1, "size", filesize)){
        LOG_INFO("size: ================>"FMTu64, filesize);
    }else{
        LOG_ERROR("Can not find size, error!");
        return -1;
    }


    /*
      Split URI into datasetid (if present) and else.
      "/clouddoc/file/aaa/bbb?ccc" -> (null,"aaa/bbb?ccc")
      "/vcs/file/nnn/aaa/bbb?ccc"  -> (nnn,"aaa/bbb?ccc")
     */
    const std::string &req_uri = transaction->req->uri;
    const std::string prefix_clouddoc_file = "/clouddoc/file/";
    const std::string prefix_vcs_file = "/vcs/file/";
    u64 datasetId = 0;
    std::string path_query;
    if (req_uri.compare(0, prefix_clouddoc_file.length(), prefix_clouddoc_file) == 0) {
        path_query.assign(req_uri.substr(prefix_clouddoc_file.length()));
        int err = VCS_getDatasetID(transaction->uid, "Cloud Doc", datasetId);
        if (err) {
            LOG_ERROR("Failed to find \"Cloud Doc\" dataset: %d", err);
            return err;
        }
    }
    else if (req_uri.compare(0, prefix_vcs_file.length(), prefix_vcs_file) == 0) {
        size_t pos = req_uri.find('/', prefix_vcs_file.length());
        if (pos == req_uri.npos) {
            LOG_ERROR("Unexpected URI %s", req_uri.c_str());
            return -1;  // FIXME
        }
        path_query.assign(req_uri, pos + 1, req_uri.npos);
        datasetId = VPLConv_strToU64(req_uri.substr(prefix_vcs_file.length()).c_str(), NULL, 10);
    }
    else {
        LOG_ERROR("Unexpected URI %s", req_uri.c_str());
        return -1;  // FIXME
    }
    LOG_DEBUG("datasetId="FMTu64, datasetId);
    LOG_DEBUG("path_query=%s", path_query.c_str());

    string accessUrl, locationName;
    int err = 0;

    vector<string> headerName, headerValue;
    err = VCSgetAccessInfo(transaction->uid, datasetId, sessionHandle, serviceTicket, "GET", path_query, "", accessUrl, headerName, headerValue, locationName);

    if(err != 0){
        LOG_ERROR("VCSgetAccessInfo Failed");
        return -1;
    }


    VPLHttp2 _h;
    VPLHttp2 &h = transaction->http2 ? *transaction->http2 : _h;

    if (__ccdConfig.debugAcsHttp)
        h.SetDebug(1);

    for(size_t i=0; i<headerName.size(); i++){
        h.AddRequestHeader(headerName[i], headerValue[i]);
    }


    std::string range;
    const string* reqRange = NULL;
    reqRange = transaction->req->find_header("Range");
    if(reqRange){
        LOG_INFO("Range request found: %s", reqRange->c_str());
        int ret = -1;
        u64 start=0, end=0;
        ret = Util_ParseRange(transaction->req->find_header("Range"), filesize, start, end);
        if(ret == 0){
            h.AddRequestHeader("Range", *reqRange);
        }else{
            LOG_ERROR("Parse fail, range: %s", transaction->req->find_header("Range")->c_str());
            LOG_ERROR("Skip range header!!");
        }
    }

    {
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
        char contentLengthValue[128];
        sprintf(contentLengthValue, FMTu64, filesize);
        if(transaction->req->find_header("Range")){
            int ret=-1;
            u64 start=0, end=0;
            ret = Util_ParseRange(transaction->req->find_header("Range"), filesize, start, end);
            if(ret == 0){
                //over ride contentLengthValue
                sprintf(contentLengthValue, FMTu64, end-start+1);
                LOG_INFO("Range:================>%s", transaction->req->find_header("Range")->c_str());
                LOG_INFO("FileSize:=============>"FMTu64, filesize);
                LOG_INFO("Start:================>"FMTu64, start);
                LOG_INFO("End:  ================>"FMTu64, end);
                LOG_INFO("contentLengthValue: ================>%s", contentLengthValue);
            }else{
                LOG_ERROR("Parse fail, range: %s", transaction->req->find_header("Range")->c_str());
            }
        }
        transaction->resp.add_header("Content-Length", contentLengthValue); 
    }
    transaction->isSetOrigHsStatusCode = false;
    h.SetUri(accessUrl);
    h.SetTimeout(VPLTime_FromSec(30));
    // Blocking call, can take a while.
    rc = h.Get(s3_write_file_and_httpstream_callback, transaction, s3_progress_callback, NULL);

    VPLMutex_Lock(&transaction->mutex);
    transaction->processing = false;
    transaction->sending = true;
    VPLMutex_Unlock(&transaction->mutex);

    if (rc < 0){
        LOG_ERROR("VPLHttp_Request failed!");
        if(transaction->origHs) {
            transaction->origHs->SetStatusCode(h.GetStatusCode());
        }
        return rc;
    }

    rc = translateHttpStatusCode(h.GetStatusCode());

    return rc;
}

static bool isDropJob(int rc) {
    if(rc == CCD_ERROR_HTTP_STATUS ||
        rc == CCD_ERROR_BAD_SERVER_RESPONSE)
    {
        // http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_API_Client_Side_Error_Handling#CCD_calling_VCS-v1-API_error_handling
        return true;
    }
    return false;
}

static int S3_putFile(DocSNGTicket* ticket, string mimetype)
{

    string accessUrl, locationName;
    int rc = 0;

    u64 datasetId = 0;
    rc = VCS_getDatasetID(ticket->uid, "Cloud Doc", datasetId);
    if (rc) {
        LOG_ERROR("Failed to find \"Cloud Doc\" dataset: %d", rc);
        return rc;
    }

    vector<string> headerName;
    vector<string> headerValue;
    rc = VCSgetAccessInfo(ticket->uid, datasetId,
                          "", "", "PUT", "",
                          ticket->contentType,
                          accessUrl,
                          headerName,
                          headerValue,
                          locationName);
    if (rc != 0) {
        LOG_ERROR("VCSgetAccessInfo Failed, %d", rc);

        if (isDropJob(rc)) {
            LOG_ERROR("VCSgetAccessInfo dropping job(%s):%d", ticket->docname.c_str(), rc);
            return 1;  // job dropped when positive.
        }
        return rc;
    }

    VPLFS_stat_t filestat;
    rc = VPLFS_Stat(ticket->srcPath.c_str(), &filestat);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to stat(%s):%d, dropping job", ticket->srcPath.c_str(), rc);
        return 2;  // job dropped when positive.
    }

    VPLHttp2 h;

    ticket->http_handle = &h;

    if (__ccdConfig.debugAcsHttp) {
        h.SetDebug(1);
    }

    for(size_t i=0; i<headerName.size(); i++) {
        h.AddRequestHeader(headerName[i], headerValue[i]);
    }

    string response;
    h.SetUri(accessUrl);
    h.SetTimeout(VPLTime_FromSec(30));
    // Blocking call, can take a while.
    rc = h.Put(ticket->srcPath, s3_put_progress_callback, ticket, response);

    {
        MutexAutoLock lock(&(ticket->http_handle_mutex));
        ticket->http_handle = NULL;
    }

    if (rc < 0) {
        LOG_ERROR("VPLHttp_Request failed!, %d", rc);
        return rc;
    }

    rc = translateHttpStatusCode(h.GetStatusCode());
    if (rc < 0) {
        return rc;
    }

    string vcsresponse;
    rc = VCSpostUpload(ticket,
                       vcsresponse,
                       accessUrl,
                       (VPLFile_offset_t)filestat.size);
    if (rc != 0) {
        if(isDropJob(rc)) {
            LOG_ERROR("VCSpostUpload dropping job(%s):%d", ticket->docname.c_str(), rc);
            return 3;  // job dropped when positive.
        }
        LOG_ERROR("VCSpostUpload failed!, %d", rc);
        return rc;
    }

    // parse vcsresponse to get compid and latest revision number
    cJSON2 *json_resp = cJSON2_Parse(vcsresponse.c_str());
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
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC
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
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC
            ticket->modifyTime = mtime;
        }
        cJSON2_Delete(json_resp);
    }

    if(ticket->thumbPath != "") {
        rc = VCSuploadPreview(ticket, vcsresponse);
        if (rc != 0) {
            if(isDropJob(rc)) {
                LOG_ERROR("VCSuploadPreview dropping job(%s):%d", ticket->docname.c_str(), rc);
                return 3;  // job dropped when positive.
            }
            LOG_ERROR("VCSuploadPreview failed!, %d", rc);
            return rc;
        }
    }

    return 0;
}

// We don't want this to get mixed up with other putFile functions
// At the very least, error handling convention is different.
// For SyncUp, a positive error means drop the job.
int S3_putFileSyncUp(const std::string &serviceName,
                     const std::string &comppath,
                     u64 modifyTime,
                     u64 userId,
                     u64 datasetId,
                     bool baserev_valid,
                     u64 baserev,
                     u64 compid,
                     const std::string &contentType,
                     const std::string &srcPath,
                     VPLHttp2 *http2,
                     /*OUT*/ std::string &vcsresponse)
{
    string accessUrl, locationName;
    int rc;

    vector<string> headerName;
    vector<string> headerValue;
    rc = VCSgetAccessInfo(userId, datasetId,
                          "", "", "PUT", "",
                          contentType,
                          accessUrl,
                          headerName,
                          headerValue,
                          locationName);
    if(rc != 0){
        LOG_ERROR("VCSgetAccessInfo Failed, %d", rc);

        if(isDropJob(rc))
        {
            LOG_ERROR("VCSgetAccessInfo dropping job(%s):%d", comppath.c_str(), rc);
            return 2;  // job dropped when positive.
        }
        return rc;
    }


    VPLFS_stat_t filestat;
    rc = VPLFS_Stat(srcPath.c_str(), &filestat);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to stat(%s):%d, dropping job", srcPath.c_str(), rc);
        return 1;  // job dropped when positive.
    }

    VPLHttp2 _h;
    VPLHttp2 &h = http2 ? *http2 : _h;

    if (__ccdConfig.debugAcsHttp)
        h.SetDebug(1);

    for(size_t i=0; i<headerName.size(); i++){
        h.AddRequestHeader(headerName[i], headerValue[i]);
    }

    string response;
    h.SetUri(accessUrl);
    h.SetTimeout(VPLTime_FromSec(30));
    LOG_DEBUG("starting PUT");
    // Blocking call, can take a while.  Regardless of what is returned from ACS,
    // we do not want to drop the job here.  Only VCS API should drop job.
    rc = h.Put(srcPath, NULL, NULL, response);
    LOG_DEBUG("returned from PUT: rc %d", rc);

    if (rc < 0){
        LOG_ERROR("VPLHttp_Request failed (%d)", rc);
        return rc;
    }

    rc = translateHttpStatusCode(h.GetStatusCode());
    if (rc < 0) {
        LOG_ERROR("Put file error(%s):%d, %d",
                  comppath.c_str(), rc, h.GetStatusCode());
        return rc;
    }

    rc = VCSpostUpload(serviceName,
                       comppath,
                       modifyTime,
                       userId,
                       datasetId,
                       baserev_valid,
                       baserev,
                       compid,
                       vcsresponse,
                       accessUrl,
                       (VPLFile_offset_t)filestat.size);
    if(rc != 0) {
        LOG_ERROR("VCSpostUpload failed(%s):%d", comppath.c_str(), rc);
        if(isDropJob(rc)) {
            LOG_ERROR("VCSpostUpload dropping job(%s):%d", comppath.c_str(), rc);
            return 3;  // job dropped when positive.
        }
        return rc;
    }
    return rc;
}


struct StorageProxy s3_proxy = {

               100,
               VCSdummy,  
               VCSdummy,
               VCSreadFolder,  //readFolder
               VCSdummy,
               VCSdummy,
               VCSdummy,
               VCSdummy,
               VCSreadFile,
               S3_putFile,
               S3_getFile,
               VCSdownloadPreview,
               VCSdeleteFile,
               VCSdummy,
               VCSmoveFile,

};


