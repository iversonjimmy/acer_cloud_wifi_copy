//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <gvm_file_utils.hpp>
#include <ccdi_client_tcp.hpp>
#include "scopeguard.hpp"

#include "autotest_common_utils.hpp"
#include "dx_common.h"
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "clouddochttp.hpp"
#include "TargetDevice.hpp"
#include "EventQueue.hpp"

#include "autotest_clouddoc.hpp"

#include "cJSON2.h"

#include <sstream>
#include <string>

const char *TEST_CLOUDDOC_STR = "SdkCloudDocRelease";
// each retry is 1 second
#define CLOUDDOC_TIMEOUT_RETRY_COUNT  20

#define CD_TEMPLATE_OP(op, num, skip, retry, arg1, arg2, arg3, arg4, arg5, resp, rc) \
    do { \
        const char *testArgs[] = { "CloudDocHttp", op, arg1, arg2, arg3, arg4, arg5 }; \
        resp.clear(); \
        rc = dispatch_clouddochttp_cmd_with_response(num, testArgs, resp); \
        if (rc < 0 && retry) { \
            LOG_INFO("Retry in 10 seconds!"); \
            VPLThread_Sleep(VPLTime_FromSec(10)); \
            rc = dispatch_clouddochttp_cmd_with_response(num, testArgs, resp); \
        } \
        if (!skip) { CHECK_AND_PRINT_RESULT("SdkCloudDocRelease", op, rv); } \
    } while (0);

#define CD_UPLOAD_BASIC(doc, preview, resp, rc)         CD_TEMPLATE_OP("UploadBasic",     4, false, false, doc, preview, NULL, NULL, NULL, resp, rc)
#define CD_UPLOAD_REV(cdoc, doc, prev, id, rev, resp, rc) CD_TEMPLATE_OP("UploadRevision",7, false, false, cdoc,     doc,   prev, id,  rev, resp, rc)
#define CD_LISTDOC(resp, rc)                            CD_TEMPLATE_OP("ListDocs",        2, false,  true, NULL,    NULL, NULL, NULL, NULL, resp, rc)
#define CD_DOWNLOAD(name, id, rev, out, resp, rc)       CD_TEMPLATE_OP("Download",        6, false,  true, name,      id,  rev,  out, NULL, resp, rc)
#define CD_DPREVIEW(name, id, rev, out, resp, rc)       CD_TEMPLATE_OP("DownloadPreview", 6, false,  true, name,      id,  rev,  out, NULL, resp, rc)
#define CD_DELETE(name, id, rev, resp, rc)              CD_TEMPLATE_OP("Delete",          6, false,  true, name,      id,  rev, NULL, NULL, resp, rc)
#define CD_MOVE(name, id, newname, resp, rc)            CD_TEMPLATE_OP("Move",            6, false,  true, name,      id,  newname, NULL, NULL, resp, rc)
#define CD_DELETE_ASYNC(name, id, rev, resp, rc)        CD_TEMPLATE_OP("DeleteAsync",     6, false,  true, name,      id,  rev, NULL, NULL, resp, rc)
#define CD_MOVE_ASYNC(name, id, newname, resp, rc)      CD_TEMPLATE_OP("MoveAsync",       6, false,  true, name,      id,  newname, NULL, NULL, resp, rc)
#define CD_CHECK_CONFLICT(name, id, resp, rc)           CD_TEMPLATE_OP("CheckConflict",   4, false,  true, name,      id,    NULL, NULL, NULL, resp, rc)
#define CD_CHECK_COPYBACK(name, id, resp, rc)           CD_TEMPLATE_OP("CheckCopyBack",   4, false,  true, name,      id,    NULL, NULL, NULL, resp, rc)

#define CD_UPLOAD_BASIC_SKIP(doc, preview, resp, rc)    CD_TEMPLATE_OP("UploadBasic",     4,  true, false,  doc, preview, NULL, NULL, NULL, resp, rc)
#define CD_DOWNLOAD_SKIP(name, id, rev, out, resp, rc)  CD_TEMPLATE_OP("Download",        6,  true,  true, name,      id,  rev,  out, NULL, resp, rc)
#define CD_DOWNLOAD_RANGE_SKIP(name, id, rev, out, range, resp, rc)  CD_TEMPLATE_OP("Download",        7,  true,  true, name,      id,  rev,  out, range, resp, rc)
#define CD_GET_METADATA_SKIP(name, id, rev, resp, rc)   CD_TEMPLATE_OP("GetMetadata",     5,  true,  true, name,      id,  rev, NULL, NULL, resp, rc)
#define CD_LISTDOC_SKIP(resp, rc)                       CD_TEMPLATE_OP("ListDocs",        2,  true,  true, NULL,    NULL, NULL, NULL, NULL, resp, rc)
#define CD_DPREVIEW_SKIP(name, id, rev, out, resp, rc)  CD_TEMPLATE_OP("DownloadPreview", 6,  true,  true, name,      id,  rev,  out, NULL, resp, rc)
#define CD_DELETE_SKIP(name, id, rev, resp, rc)         CD_TEMPLATE_OP("Delete",          6,  true,  true, name,      id,  rev, NULL, NULL, resp, rc)
#define CD_LISTREQ_SKIP(resp, rc)                       CD_TEMPLATE_OP("ListReq",         2,  true,  true, NULL,    NULL, NULL, NULL, NULL, resp, rc)
#define CD_GET_PROGRESS_SKIP(id, resp, rc)              CD_TEMPLATE_OP("GetProgress",     3,  true,  true,   id,    NULL, NULL, NULL, NULL, resp, rc)
#define CD_CANCEL_REQ(id, resp, rc)                     CD_TEMPLATE_OP("CancelReq",       3, false,  true,   id,    NULL, NULL, NULL, NULL, resp, rc)

static int clouddochttp_getreq_info(const char *filename, u64& request_id) {
    int rv = 0;
    std::string response;
    cJSON2 *filelist = NULL;
    cJSON2 *jsonResponse = NULL;

    CD_LISTREQ_SKIP(response, rv);
    if (!rv) {
        jsonResponse = cJSON2_Parse(response.c_str());
        if (jsonResponse == NULL) {
            rv = -1;
        }
    }
    if (!rv) {
        // get the deviceList array object
        filelist = cJSON2_GetObjectItem(jsonResponse, "requestList");
        if (filelist == NULL) {
            rv = -1;
        }
    }
    if (!rv) {
        rv = -1;
        cJSON2 *node = NULL;
        cJSON2 *subNode = NULL;
        //cJSON2 *subNode2 = NULL;
        bool found = false;
        for (int i = 0; i < cJSON2_GetArraySize(filelist); i++) {
            node = cJSON2_GetArrayItem(filelist, i);
            if (node == NULL) {
                continue;
            }
            subNode = cJSON2_GetObjectItem(node, "name");
            if (subNode == NULL || subNode->type != cJSON2_String ||
                    strcmp(filename, subNode->valuestring)) {
                continue;
            }
#if 0
            // if more than one file found. abort and return error
            if (found) {
                LOG_ERROR("More than one file found!");
                rv = -1;
                goto exit;
            }
#endif
            subNode = cJSON2_GetObjectItem(node, "id");
            if (subNode == NULL || subNode->type != cJSON2_Number) {
                continue;
            }
            request_id = (u64)subNode->valueint;
#if 0
            subNode = cJSON2_GetObjectItem(node, "latestRevision");
            if (subNode == NULL) {
                continue;
            }
            subNode2 = cJSON2_GetObjectItem(subNode, "revision");
            if (subNode2 == NULL || subNode2->type != cJSON2_Number) {
                continue;
            }
            revision  = (u64)subNode2->valueint;
#endif
            if (!found) {
                found = true;
                rv = 0;
                break;
            }
        }
    }
exit:
    if (jsonResponse) {
        cJSON2_Delete(jsonResponse);
        jsonResponse = NULL;
    }

    return rv;
}

static int clouddochttp_getreq_info_wait(const char *filename, u64& request_id,
                                         unsigned int retry, bool expect_fail) {
    const int wait_ms = 1000;
    unsigned int count = 0;
    int rv = 0;
    while(count < retry) {
        rv = clouddochttp_getreq_info(filename, request_id);
        // expect not-exist
        if (expect_fail && rv) {
            // return 0 since it's expected failed
            rv = 0;
            break;
        }
        // expect exist and ignore revision
        if (!expect_fail && !rv) {
            break;
        }

        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(wait_ms));
        count++;
    };
    if (count > 0) {
        LOG_ALWAYS("waited for %d milliseconds", wait_ms * count);
    }
    return rv;
}

static int clouddochttp_get_progress_info_wait(const char *filename, const std::string& req_id,
                                               unsigned int retry) {
    const int wait_ms = 1000;
    unsigned int count = 0;
    int rv = 0;
    std::string response;

    while(count < retry) {
        cJSON2 *jsonResponse = NULL;

        CD_GET_PROGRESS_SKIP(req_id.c_str(), response, rv);

        // expect exist and ignore revision
        if (!rv) {
            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                rv = -1;
            }
        }

        if (!rv) {
            rv = -1;
            //Get totalSize and xferedSize
            cJSON2 *node = NULL;

            node = cJSON2_GetObjectItem(jsonResponse, "totalSize");
            if(node == NULL || node->type != cJSON2_Number) {
                rv = -1;
            }else{
                if(node->valueint > 0){
                    rv = 0;
                }
            }

            node = cJSON2_GetObjectItem(jsonResponse, "xferedSize");
            if(node == NULL || node->type != cJSON2_Number) {
                rv = -1;
            }else{
                if(node->valueint > 0){
                    rv = 0;
                }
            }
        }


        if (jsonResponse) {
            cJSON2_Delete(jsonResponse);
            jsonResponse = NULL;
        }

        if(rv == 0)
            break;

        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(wait_ms));
        count++;
    };

exit:
    if (count > 0) {
        LOG_ALWAYS("waited for %d milliseconds", wait_ms * count);
    }
    return rv;
}

static int clouddochttp_getdoc_info(const char *filename, u64& revision, u64& component_id) {
    int rv = 0;
    std::string response;
    cJSON2 *filelist = NULL;
    cJSON2 *jsonResponse = NULL;

    CD_LISTDOC_SKIP(response, rv);
    if (!rv) {
        jsonResponse = cJSON2_Parse(response.c_str());
        if (jsonResponse == NULL) {
            rv = -1;
        }
    }
    if (!rv) {
        // get the deviceList array object
        filelist = cJSON2_GetObjectItem(jsonResponse, "fileList");
        if (filelist == NULL) {
            rv = -1;
        }
    }
    if (!rv) {
        rv = -1;
        cJSON2 *node = NULL;
        cJSON2 *subNode = NULL;
        cJSON2 *subNode2 = NULL;
        bool found = false;
        for (int i = 0; i < cJSON2_GetArraySize(filelist); i++) {
            node = cJSON2_GetArrayItem(filelist, i);
            if (node == NULL) {
                continue;
            }
            subNode = cJSON2_GetObjectItem(node, "name");
            if (subNode == NULL || subNode->type != cJSON2_String ||
                strcmp(filename, subNode->valuestring)) {
                continue;
            }
            // if more than one file found. abort and return error
            if (found) {
                LOG_ERROR("More than one file found!");
                rv = -1;
                goto exit;
            }
            subNode = cJSON2_GetObjectItem(node, "compId");
            if (subNode == NULL || subNode->type != cJSON2_Number) {
                continue;
            }
            component_id = (u64)subNode->valueint;
            subNode = cJSON2_GetObjectItem(node, "latestRevision");
            if (subNode == NULL) {
                continue;
            }
            subNode2 = cJSON2_GetObjectItem(subNode, "revision");
            if (subNode2 == NULL || subNode2->type != cJSON2_Number) {
                continue;
            }
            revision  = (u64)subNode2->valueint;
            if (!found) {
                found = true;
                rv = 0;
            }
        }
    }
exit:
    if (jsonResponse) {
        cJSON2_Delete(jsonResponse);
        jsonResponse = NULL;
    }

    return rv;
}

static int clouddochttp_getdoc_info_wait(const char *filename, u64& revision, u64& component_id,
                                         unsigned int retry, bool expect_fail, u64 expect_rev=0) {
    const int wait_ms = 1000;
    unsigned int count = 0;
    int rv = 0;
    while(count < retry) {
        rv = clouddochttp_getdoc_info(filename, revision, component_id);
        // expect not-exist
        if (expect_fail && rv) {
            // return 0 since it's expected failed
            rv = 0;
            break;
        }
        // expect exist and ignore revision
        if (!expect_fail && !rv) {
            // ignore comparing revision
            if (expect_rev == 0) {
                break;
            } else if (expect_rev > 0) {
                // comparing revision
                if (expect_rev == revision) {
                    break;
                } else {
                    LOG_ERROR("Expected revision ("FMTu64") != current revision ("FMTu64")",
                               expect_rev, revision);
                    rv = -1;
                }
            }
        }

        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(wait_ms));
        count++;
    };
    if (count > 0) {
        LOG_ALWAYS("waited for %d milliseconds", wait_ms * count);
    }
    return rv;
}

static int clouddochttp_pushFile(const std::string &filename, const std::string &srcpath="") {

    std::string curDir;
    std::string remoteroot;
    std::string localPath, remotePath;
    TargetDevice *target = NULL;

    int rv = -1;

    rv = getCurDir(curDir);
    if (rv < 0) {
        LOG_ERROR("failed to get current dir. error = %d", rv);
        goto cleanup;
    }

    target = getTargetDevice();
    rv = target->getDxRemoteRoot(remoteroot);
    if (rv < 0) {
        LOG_ERROR("failed to get remote dir. error = %d", rv);
        goto cleanup;
    }

    if(srcpath.empty()){
        localPath = curDir + "/" + filename;
    }else{
        localPath = srcpath;
    }
    remotePath = remoteroot + "/" + filename;
 
    rv = target->pushFile(localPath.c_str(), remotePath.c_str());
    if (rv < 0) {
        LOG_ERROR("failed to get pushFile. error = %d", rv);
        goto cleanup;
    }

cleanup:
    delete target;
    return rv;
}

static int clouddochttp_pullFile(const std::string &filename) {

    std::string curDir;
    std::string remoteroot;
    std::string localPath, remotePath;
    TargetDevice *target = NULL;

    int rv = -1;

    rv = getCurDir(curDir);
    if (rv < 0) {
        LOG_ERROR("failed to get current dir. error = %d", rv);
        goto cleanup;
    }

    target = getTargetDevice();
    rv = target->getDxRemoteRoot(remoteroot);
    if (rv < 0) {
        LOG_ERROR("failed to get remote dir. error = %d", rv);
        goto cleanup;
    }

    localPath = curDir + "/" + filename;
    remotePath = remoteroot + "/" + filename;
 
    Util_rm_dash_rf(localPath);
    rv = target->pullFile(remotePath.c_str(), localPath.c_str());
    if (rv < 0) {
        LOG_ERROR("failed to get pushFile. error = %d", rv);
        goto cleanup;
    }

cleanup:
    delete target;
    return rv;
}

static int clouddochttp_compareFile(const std::string &localFile, const std::string &remoteFile, bool comparerange=false, VPLFile_offset_t from=0, VPLFS_file_size_t length=0) {

    std::string curDir;
    std::string remoteroot;
    std::string localPath1, localPath2;

    int rv = -1;
    
    rv = clouddochttp_pullFile(remoteFile);
    if (rv < 0) {
        LOG_ERROR("failed to get pullFile. error = %d", rv);
        goto cleanup;
    }

    rv = getCurDir(curDir);
    if (rv < 0) {
        LOG_ERROR("failed to get current dir. error = %d", rv);
        goto cleanup;
    }

    localPath1 = curDir + "/" + localFile;
    localPath2 = curDir + "/" + remoteFile;

    if(comparerange)
        rv = file_compare_range(localPath1.c_str(), localPath2.c_str(), from, length);
    else
        rv = file_compare(localPath1.c_str(), localPath2.c_str());
    if (rv < 0) {
        LOG_ERROR("failed to compare. error = %d", rv);
        goto cleanup;
    }

cleanup:
    return rv;
}

#define CLOUDDOC_DOCX_FILE_CLONE       "CloudDoc.docx.clone"
#define CLOUDDOC_DOCX_FILE_MOVE        "CloudDoc.docx.move"
#define CLOUDDOC_DOCX_FILE_TEMP        "tmp_CloudDoc.docx"
#define CLOUDDOC_DOCX_FILE_TEMP2       "tmp_CloudDoc2.docx"
#define CLOUDDOC_DOCX_FILE_TEMP3       "tmp_CloudDoc3.docx"
#define CLOUDDOC_DOCX_JPG_FILE_CLONE   "CloudDoc.docx.jpg.clone"

static int check_clouddoc_upload_visitor(const ccd::CcdiEvent &_event, void *_ctx)
{
    LOG_ALWAYS("\n== check_clouddoc_download_visitor: %s", _event.DebugString().c_str());
    check_event_visitor_ctx *ctx = (check_event_visitor_ctx*)_ctx;

    if (ctx->event.has_doc_save_and_go_completion()){

        if (!_event.has_doc_save_and_go_completion()) {
            LOG_ALWAYS("No has_doc_save_and_go_completion!");
            return 0;
        }
        if (!ctx->event.has_doc_save_and_go_completion()){
            LOG_ALWAYS("No has_doc_save_and_go_completion in ctx!");
            return 0;
        }

        const ccd::EventDocSaveAndGoCompletion *ctxevent = ctx->event.mutable_doc_save_and_go_completion();

        LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        LOG_ALWAYS("request completed: %d , file_path_and_name: %s", _event.doc_save_and_go_completion().change_type(), _event.doc_save_and_go_completion().docname().c_str());
        LOG_ALWAYS("ctx: %d , file_path_and_name: %s", ctxevent->change_type(), ctxevent->docname().c_str());
        if(_event.doc_save_and_go_completion().change_type() == ctxevent->change_type() &&
            _event.doc_save_and_go_completion().docname() == ctxevent->docname()) {
            LOG_ALWAYS("request completed: %d , file_path_and_name: %s", _event.doc_save_and_go_completion().change_type(),
                _event.doc_save_and_go_completion().docname().c_str());
            ctx->done = true;
        }
    }
    return 0;
}

static VPLThread_return_t listen_clouddoc_upload(VPLThread_arg_t arg)
{
    check_event_visitor_ctx *ctx = (check_event_visitor_ctx*)arg;
    set_target_machine("MD");

    int rv = -1;
    int checkTimeout = 1600;
    int secondsLeft = checkTimeout;
    EventQueue eq;

    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end = start + VPLTime_FromSec(secondsLeft);
    VPLTime_t now;

    ctx->count = 0;

    VPLSem_Post(&(ctx->sem));

    while ((now = VPLTime_GetTimeStamp()) < end && !ctx->done) {
        eq.visit(0, (s32)(VPLTime_ToMillisec(VPLTime_DiffClamp(end, now))), check_clouddoc_upload_visitor, (void*)ctx);
    }
    if (!ctx->done) {
        LOG_ERROR("task didn't complete within %d seconds", checkTimeout);
    }

    return (VPLThread_return_t)rv;
}

#define CREATE_CLOUDDOC_EVENT_VISIT_THREAD(testStr) \
    BEGIN_MULTI_STATEMENT_MACRO \
    VPLThread_t event_thread; \
    rv = VPLThread_Create(&event_thread, listen_clouddoc_upload, (VPLThread_arg_t)&ctx, NULL, "listen_clouddoc_upload"); \
    if (rv != VPL_OK) { \
        LOG_ERROR("Failed to spawn event listen thread: %d", rv); \
    } \
    VPLSem_Wait(&(ctx.sem)); \
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, testStr, rv); \
    END_MULTI_STATEMENT_MACRO


#define CHECK_CLOUDDOC_EVENT_VISIT_RESULT(testStr, timeout) \
    BEGIN_MULTI_STATEMENT_MACRO \
    int retry = 0; \
    while(!ctx.done && retry++ < timeout) \
        VPLThread_Sleep(VPLTIME_FROM_SEC(1)); \
    if (!ctx.done) { \
        LOG_ERROR("couddoc_upload didn't complete within %d seconds", timeout); \
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, testStr, rv); \
    } \
    END_MULTI_STATEMENT_MACRO

static int clouddochttp_uploadDir(const std::string &localDir,
                                  const std::string &docx_preview,
                                  const std::string &remoteroot_path,
                                  bool full)
{

#define CONSTRUCT_DOCNAME_MAP(name) \
    BEGIN_MULTI_STATEMENT_MACRO \
        std::string path = name; \
        ss.str(""); \
        ss << deviceId; \
        if(path[0] != '/') { \
            /* Assume this is win32 path, ex: c:/test/123.txt */ \
            int idx = 0; \
            idx = path.find_first_of(":"); \
            if (idx != std::string::npos) { \
                path.erase(idx, 1); \
            } \
            \
            ss << "/"; \
        } \
        ss << path; \
        doc_upload[name] = ss.str(); \
        /* docname always use forward slash */ \
        std::replace(doc_upload[name].begin(), doc_upload[name].end(), '\\', '/'); \
    END_MULTI_STATEMENT_MACRO

    int rv = -1;
    u64 deviceId;
    std::stringstream ss;
    std::string response;
    std::map<std::string, std::string> doc_upload;
    std::map<std::string, std::string>::iterator it;

    rv = getDeviceId(&deviceId);
    if(rv != 0) {
        LOG_ERROR("Failed to get deviceId %d", rv);
        goto exit;
    }

    {
        std::string docSetPath;
        std::string dxroot;
        //construct golden data path
        rv = getDxRootPath(dxroot);
        if (rv < 0) {
            LOG_ERROR("failed to get dxroot dir. error = %d", rv);
            goto exit;
        }
        docSetPath = dxroot + localDir;
        //enumerate files in golden test data folder
        VPLFS_dir_t dir;
        VPLFS_dirent_t dirent;
        rv = VPLFS_Opendir(docSetPath.c_str(), &dir);
        if (rv < 0) {
            LOG_ERROR("failed to open dir(%s). error = %d", docSetPath.c_str(), rv);
            goto exit;
        }
        ON_BLOCK_EXIT(VPLFS_Closedir, &dir);

        while(VPLFS_Readdir(&dir, &dirent) == VPL_OK){
            if(dirent.type != VPLFS_TYPE_FILE)
                continue;

            std::string doc_golden = docSetPath+"/"+dirent.filename;
            rv = clouddochttp_pushFile(dirent.filename, doc_golden);
            if(rv != 0){
                LOG_ERROR("clouddochttp_pushFile failed, src: %s", doc_golden.c_str());
                break;
            }
            std::string doc_local = remoteroot_path + "/" + dirent.filename;
            CONSTRUCT_DOCNAME_MAP(doc_local);
            LOG_ALWAYS("doc_local :%s", doc_local.c_str());
            LOG_ALWAYS("doc_upload:%s", doc_upload[doc_local].c_str());
        }
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForUpload", rv);

    }

    // 11.1 Upload Docs
    for (it = doc_upload.begin(); it != doc_upload.end(); ++it) {
        CD_UPLOAD_BASIC_SKIP(it->first.c_str(), docx_preview.c_str(), response, rv);
        if(rv != 0){
            LOG_ERROR("Fail to upload :%s, %d", it->first.c_str(), rv);
            goto exit;
        }
    }

    // 11.2 Check Docs uploaded
    for (it = doc_upload.begin(); it != doc_upload.end(); ++it) {
        u64 revision;
        u64 component_id;
        std::string revStr, compStr;
        LOG_ALWAYS("Checking: %s", it->second.c_str());
        rv = clouddochttp_getdoc_info_wait(it->second.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        if(rv != 0){
            LOG_ERROR("Fail to find :%s, %d", it->second.c_str(), rv);
            goto exit;
        }
        if(!full){
            //delete
            ss.str("");
            ss << component_id;
            compStr = ss.str();
            ss.str("");
            ss << revision;
            revStr = ss.str();
            CD_DELETE_SKIP(it->second.c_str(), compStr.c_str(), revStr.c_str(), response, rv);
            if(rv != 0){
                LOG_ERROR("Fail to delete :%s, %d", it->second.c_str(), rv);
                goto exit;
            }
        }
    }

exit:
    return rv;
}

int do_autotest_sdk_release_clouddochttp(int argc, const char* argv[]) {


    int cloudPCId = 1;
    int clientPCId = 2;
    int clientPCId2 = 3;
    u64 userId = 0;
    u64 deviceId = 0;
    u64 datasetId = 0;

    u64 revision  = 0;
    u64 component_id = 0;
    u64 request_id = 0;
    std::string comp_id;
    std::string rev_id;
    std::string req_id;

    std::string response;
    int rv = 0;
    int count = 0;

    cJSON2 *jsonResponse = NULL;

    std::stringstream ss;
    std::string clientPCOSVersion;
    std::string tmp_docx,
                tmp_docx2,
                tmp_docx3;
    std::string docx;
    std::string docx_clone;
    std::string docx_move;
    std::string docx_preview;
    std::string docx_preview_clone;
    std::string docx_preview_move;
    std::string docx2;
    std::string docx_preview2;
    std::string tmp_docx_upload,
                tmp_docx2_upload,
                tmp_docx3_upload;
    std::string docx_upload;
    std::string docx_clone_upload;
    std::string docx_move_upload;
    std::string docx_dummy;
    std::string docx_dummy_upload;
    std::string docx_dummy_100M;
    std::string docx_dummy_100M_upload;

    std::string remoteroot_path;

    bool full = false;

    if (argc == 5 && (strcmp(argv[4], "-f") == 0 || strcmp(argv[4], "--fulltest") == 0) ) {
        full = true;
    }

    if (checkHelp(argc, argv) || (argc < 4) || (argc == 5 && !full)) {
        printf("AutoTest %s <domain> <username> <password> [<fulltest>(-f/--fulltest)]\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Testing AutoTest SDK Release CloudDoc: Domain(%s) User(%s) Password(%s)", argv[1], argv[2], argv[3]);

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string alias = "CloudPC";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_CLOUDDOC_STR, rv);
    }

    START_CCD(TEST_CLOUDDOC_STR, rv);

    START_CLOUDPC(argv[2], argv[3], TEST_CLOUDDOC_STR, true, rv);

    if (full) {
        TargetDevice *target = getTargetDevice();
        target->getDxRemoteRoot(remoteroot_path);
        delete target;

        std::string curDir;
        curDir = remoteroot_path;

        std::replace(curDir.begin(), curDir.end(), '\\', '/');
        curDir.append("/");
        tmp_docx2 = curDir + "tmp_CloudDoc2.docx";
    }

    LOG_ALWAYS("\n\n==== Launching MD CCD ====");
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        std::string alias = "MD";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_CLOUDDOC_STR, rv);
    }

    {
        QUERY_TARGET_OSVERSION(clientPCOSVersion, TEST_CLOUDDOC_STR, rv);
    }

    START_CCD(TEST_CLOUDDOC_STR, rv);

    UPDATE_APP_STATE(TEST_CLOUDDOC_STR, rv);

    START_CLIENT(argv[2], argv[3], TEST_CLOUDDOC_STR, true, rv);

    LOG_ALWAYS("\n\n==== Launching Client CCD 2 ====");
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "Client", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        std::string alias = "Client";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_CLOUDDOC_STR, rv);
    }

    START_CCD(TEST_CLOUDDOC_STR, rv);
    START_CLIENT(argv[2], argv[3], TEST_CLOUDDOC_STR, true, rv);

    if (full) {
        TargetDevice *target = getTargetDevice();
        target->getDxRemoteRoot(remoteroot_path);
        delete target;

        std::string curDir;
        curDir = remoteroot_path;

        std::replace(curDir.begin(), curDir.end(), '\\', '/');
        curDir.append("/");
        tmp_docx3 = curDir + "tmp_CloudDoc3.docx";
    }

    LOG_ALWAYS("\n\n==== Testing CloudDoc - TC 1 ====");

    //setCcdTestInstanceNum(clientPCId);
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        TargetDevice *target = getTargetDevice();
        target->getDxRemoteRoot(remoteroot_path);
        delete target;
    }

    if(getUserIdBasic(&userId) ||
       getDeviceId(&deviceId) ||
       getDatasetId(userId, "Cloud Doc", datasetId)) {
        LOG_ERROR("Failed to get userId = "FMTu64", deviceId = "FMTu64", datasetId = "FMTu64,
                  userId, deviceId, datasetId);
        rv = -1;
    }
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "CheckCloudDocEnabled", rv);

    LOG_ALWAYS("userId = "FMTu64", deviceId = "FMTu64", datasetId = "FMTu64, userId, deviceId, datasetId);

    // TC1 CloudDocs 1.1 commands verifications

    // get testing file path figured out
    {
        std::string curDir;

        // FIXME: following code will only work against local ccd

        // MSA path
        rv = getCurDir(curDir);
        if (rv < 0) {
            LOG_ERROR("failed to get current dir. error = %d", rv);
            goto exit;
        }

        curDir = remoteroot_path;

        std::replace(curDir.begin(), curDir.end(), '\\', '/');
        curDir.append("/");

        // if we use the docx to upload the file directly, the file will be overwritten
        // by the syncback whenever there's a latest revision.
        // to avoid the issue, we add a tmp_docx as a clone of the original file
        // and do the upload
        tmp_docx  = curDir + "tmp_" + CLOUDDOC_DOCX_FILE;

        docx = curDir + CLOUDDOC_DOCX_FILE;
        docx_clone = docx + ".clone";
        docx_move = docx + ".move";
        docx_preview = curDir + CLOUDDOC_DOCX_JPG_FILE;
        docx_preview_clone = docx_preview + ".clone";
        docx_preview_move = docx_preview + ".move";
        docx2 = curDir + CLOUDDOC_DOCX_FILE2;
        docx_preview2 = curDir + CLOUDDOC_DOCX_JPG_FILE2;
        docx_dummy = curDir + CLOUDDOC_DOCX_DUMMY;
        docx_dummy_100M = curDir + CLOUDDOC_DOCX_100M_DUMMY;

#define CONSTRUCT_DOCNAME(name) \
    BEGIN_MULTI_STATEMENT_MACRO \
        std::string path = name; \
        ss.str(""); \
        ss << deviceId; \
        if(path[0] != '/') { \
            /* Assume this is win32 path, ex: c:/test/123.txt */ \
            int idx = 0; \
            idx = path.find_first_of(":"); \
            if (idx != std::string::npos) { \
                path.erase(idx, 1); \
            } \
            \
            ss << "/"; \
        } \
        ss << path; \
        name##_upload = ss.str(); \
    END_MULTI_STATEMENT_MACRO 


        CONSTRUCT_DOCNAME(tmp_docx);
        docx_upload = tmp_docx_upload;

        CONSTRUCT_DOCNAME(docx_clone);
        CONSTRUCT_DOCNAME(docx_move);
        CONSTRUCT_DOCNAME(docx_dummy);
        CONSTRUCT_DOCNAME(docx_dummy_100M);

        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "CloudPC", rv);
        if(getDeviceId(&deviceId)) {
            LOG_ERROR("Failed to get deviceId = "FMTu64, deviceId);
            rv = -1;
        }
        CONSTRUCT_DOCNAME(tmp_docx2);

        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "Client", rv);
        if(getDeviceId(&deviceId)) {
            LOG_ERROR("Failed to get deviceId = "FMTu64, deviceId);
            rv = -1;
        }
        CONSTRUCT_DOCNAME(tmp_docx3);
        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
    }

    // clean-up before testing
    {
        TargetDevice *target = getTargetDevice();
        target->deleteFile(docx_clone);
        target->deleteFile(docx_preview_clone);
    }
    LOG_ALWAYS("tmp_DOCX = %s", tmp_docx.c_str());

    LOG_ALWAYS("DOCX = %s", docx.c_str());
    LOG_ALWAYS("DOCX.p = %s", docx_preview.c_str());
    LOG_ALWAYS("DOCX.u = %s", docx_upload.c_str());

    LOG_ALWAYS("DOCX.c = %s", docx_clone.c_str());
    LOG_ALWAYS("DOCX.c.u = %s", docx_clone_upload.c_str());
    LOG_ALWAYS("DOCX.p.c = %s", docx_preview_clone.c_str());

    LOG_ALWAYS("DOCX.m = %s", docx_move.c_str());
    LOG_ALWAYS("DOCX.m.u = %s", docx_move_upload.c_str());
    LOG_ALWAYS("DOCX.p.m = %s", docx_preview_move.c_str());

    LOG_ALWAYS("DOCX2 = %s", docx2.c_str());
    LOG_ALWAYS("DOCX.p2 = %s", docx_preview2.c_str());

    LOG_ALWAYS("tmp_docx2 = %s", tmp_docx2.c_str());
    LOG_ALWAYS("tmp_docx2_upload = %s", tmp_docx2_upload.c_str());

    LOG_ALWAYS("tmp_docx3 = %s", tmp_docx3.c_str());
    LOG_ALWAYS("tmp_docx3_upload = %s", tmp_docx3_upload.c_str());

    // clone docx to tmp_docx
    {
        std::string curDir;
        std::string dxshell_docx;
        std::string dxshell_tmp_docx;
        rv = getCurDir(curDir);
        if (rv < 0) {
            LOG_ERROR("failed to get current dir. error = %d", rv);
            goto exit;
        }
        dxshell_docx = curDir + "/" + CLOUDDOC_DOCX_FILE;
        dxshell_tmp_docx = curDir + "/" + "tmp_" + CLOUDDOC_DOCX_FILE;

        rv = file_copy(dxshell_docx.c_str(), dxshell_tmp_docx.c_str());
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1", rv);
        if (full) {
            LOG_ALWAYS("\n\n== [full]Upload document from all devices and list all from them ==");
            std::string dxshell_tmp_docx2, dxshell_tmp_docx3;
            dxshell_tmp_docx2 = curDir + "/" + "tmp_CloudDoc2.docx";
            dxshell_tmp_docx3 = curDir + "/" + "tmp_CloudDoc3.docx";
            rv = file_copy(dxshell_docx.c_str(), dxshell_tmp_docx2.c_str());
            CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1_full", rv);
            rv = file_copy(dxshell_docx.c_str(), dxshell_tmp_docx3.c_str());
            CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1_full", rv);
        }
    }
    rv = clouddochttp_pushFile(CLOUDDOC_DOCX_FILE_TEMP);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1", rv);
    rv = clouddochttp_pushFile(CLOUDDOC_DOCX_JPG_FILE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1", rv);
    if (full) {
        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        rv = clouddochttp_pushFile(CLOUDDOC_DOCX_FILE_TEMP2);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1", rv);
        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "Client", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }
        rv = clouddochttp_pushFile(CLOUDDOC_DOCX_FILE_TEMP3);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1", rv);
        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
    }

    // 1. async upload request
    LOG_ALWAYS("\n\n== Upload doc from MD ==");
    CD_UPLOAD_BASIC(tmp_docx.c_str(), docx_preview.c_str(), response, rv);

    // 2. list doc
    LOG_ALWAYS("\n\n== List doc from MD ==");
    rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded", rv);

    ss.str("");
    ss << component_id;
    comp_id = ss.str();

    ss.str("");
    ss << revision;
    rev_id = ss.str();

    LOG_ALWAYS("name = %s, rev = "FMTu64", comp_id = "FMTu64, docx_upload.c_str(), revision, component_id);

    if (full) {
        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        LOG_ALWAYS("\n\n== [full]List doc from CloudPC ==");
        rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded_CloudPC", rv);
        LOG_ALWAYS("\n\n== [full]Upload doc2 from CloudPC ==");
        CD_UPLOAD_BASIC(tmp_docx2.c_str(), "", response, rv);
        LOG_ALWAYS("\n\n== [full]List doc2 from CloudPC ==");
        rv = clouddochttp_getdoc_info_wait(tmp_docx2_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded_CloudPC", rv);

        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "Client", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }
        LOG_ALWAYS("\n\n== [full]List doc from Client ==");
        rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded_CloudPC", rv);
        LOG_ALWAYS("\n\n== [full]List doc2 from Client ==");
        rv = clouddochttp_getdoc_info_wait(tmp_docx2_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded_Client", rv);
        LOG_ALWAYS("\n\n== [full]Upload doc3 from Client ==");
        CD_UPLOAD_BASIC(tmp_docx3.c_str(), "", response, rv);
        LOG_ALWAYS("\n\n== [full]List doc3 from Client ==");
        rv = clouddochttp_getdoc_info_wait(tmp_docx3_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded_Client", rv);

        SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        LOG_ALWAYS("\n\n== [full]List doc2 from MD ==");
        rv = clouddochttp_getdoc_info_wait(tmp_docx2_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded_MD", rv);
        LOG_ALWAYS("\n\n== [full]List doc3 from MD ==");
        rv = clouddochttp_getdoc_info_wait(tmp_docx3_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded_MD", rv);
    }

    // 3. get document metadata
    {
        cJSON2 *metadata = NULL;

        CD_GET_METADATA_SKIP(docx_upload.c_str(), comp_id.c_str(), rev_id.c_str(), response, rv);
        if (!rv) {
            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                rv = -1;
            }
        }
        if (!rv) {
            metadata = cJSON2_GetObjectItem(jsonResponse, "name");
            if (metadata == NULL || metadata->type != cJSON2_String ||
                strcmp(docx_upload.c_str(), metadata->valuestring)) {
                LOG_ERROR("error while parsing metadata");
                rv = -1;
            }
        }
        if (jsonResponse) {
            cJSON2_Delete(jsonResponse);
            jsonResponse = NULL;
        }
    }
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "GetMetadata", rv);

    // 4. download content
    CD_DOWNLOAD_SKIP(docx_upload.c_str(), comp_id.c_str(), rev_id.c_str(), docx_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX

    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_FILE, CLOUDDOC_DOCX_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadFile", rv);

    // 4.1 download content in range
    CD_DOWNLOAD_RANGE_SKIP(docx_upload.c_str(), comp_id.c_str(), rev_id.c_str(), docx_clone.c_str(), "1000-2000", response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX

    //rv = file_compare(docx_range.c_str(), docx_clone.c_str());
    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_FILE, CLOUDDOC_DOCX_FILE_CLONE, true, 1000, 2000-1000+1);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadFileRange", rv);

    // 5. download preview
    CD_DPREVIEW_SKIP(docx_upload.c_str(), comp_id.c_str(), rev_id.c_str(), docx_preview_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX

    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_JPG_FILE, CLOUDDOC_DOCX_JPG_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadPreviewFile", rv);

    // 5.1 check copyback
    CD_CHECK_COPYBACK(docx_upload.c_str(), comp_id.c_str(), response, rv);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "CheckCopyBack", rv);

    // 6. delete document
    CD_DELETE(docx_upload.c_str(), comp_id.c_str(), rev_id.c_str(), response, rv);
    rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, true);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileDeleted", rv);

    // 7. Verify multiple revision
    // 7.1

    // clone docx to tmp_docx
    rv = clouddochttp_pushFile(CLOUDDOC_DOCX_FILE_TEMP);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1RevisionTest", rv);
    rv = clouddochttp_pushFile(CLOUDDOC_DOCX_JPG_FILE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1RevisionTest", rv);


    CD_UPLOAD_BASIC(tmp_docx.c_str(), docx_preview.c_str(), response, rv);
    // expected revision 1
    rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false, 1);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "ExpectRev1Uploaded", rv);

    // 7.2 bump up to revision 2
    CD_UPLOAD_BASIC(tmp_docx.c_str(), docx_preview.c_str(), response, rv);

    // get comp_id here
    // grab the component id and rev id, also expecting revision 2
    rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false, 2);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "ExpectRev2UploadedAndGetComponentId", rv);

    ss.str("");
    ss << component_id;
    comp_id = ss.str();

    LOG_ALWAYS("name = %s, rev = "FMTu64", comp_id = "FMTu64, docx_upload.c_str(), revision, component_id);

    // 7.3 switch to another device & upload from revision 1
    //setCcdTestInstanceNum(clientPCId2);
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "Client", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId2);
    }

    // clone docx to tmp_docx
    rv = clouddochttp_pushFile(CLOUDDOC_DOCX_FILE2);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1RevisionTest", rv);
    rv = clouddochttp_pushFile(CLOUDDOC_DOCX_JPG_FILE2);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC1RevisionTest", rv);
    {
        std::string remoteroot_path;
        TargetDevice *target = getTargetDevice();
        int rv = target->getDxRemoteRoot(remoteroot_path);
        if(rv < 0){
            LOG_ERROR("failed to getDxRemoteRoot. error = %d", rv);
            goto exit;
        }
        docx2 = remoteroot_path + "/" +CLOUDDOC_DOCX_FILE2;
        docx_preview2 = remoteroot_path + "/" + CLOUDDOC_DOCX_JPG_FILE2;
    }

    CD_UPLOAD_REV(docx_upload.c_str(), docx2.c_str(), docx_preview2.c_str(), comp_id.c_str(), "1", response, rv);
    rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false, 3);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "ExpectRev3Uploaded", rv);

    // 7.4 check the # of revisions
    do {
        cJSON2 *metadata = NULL;

        CD_GET_METADATA_SKIP(docx_upload.c_str(), comp_id.c_str(), NULL, response, rv);
        if (!rv) {
            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                rv = -1;
            }
        }
        // verify filename
        if (!rv) {
            metadata = cJSON2_GetObjectItem(jsonResponse, "name");
            if (metadata == NULL || metadata->type != cJSON2_String ||
                    strcmp(docx_upload.c_str(), metadata->valuestring)) {
                LOG_ERROR("error while parsing metadata for filename");
                rv = -1;
            }
        }
        if (!rv) {
            // assumption, only 2 revisions exists. since we've deleted the file at step 6
            // this should be correct
            metadata = cJSON2_GetObjectItem(jsonResponse, "revisionList");
            if (metadata == NULL || cJSON2_GetArraySize(metadata) != 2) {
                LOG_ERROR("error while parsing metadata for revision list (expecting 2 revisions). metadata = %p", metadata);
                rv = -1;
            }
        }
        // verify revision
        if (!rv) {
            cJSON2 *node = NULL;
            cJSON2 *subNode = NULL;
            for (int i = 0; i < cJSON2_GetArraySize(metadata); i++) {
                node = cJSON2_GetArrayItem(metadata, i);
                if (node == NULL) {
                    LOG_ERROR("error while parsing metadata for revision array");
                    rv = -1;
                }
                subNode = cJSON2_GetObjectItem(node, "revision");
                if (subNode == NULL || subNode->type != cJSON2_Number) {
                    LOG_ERROR("revision type is incorrect. type = %d", subNode->type);
                    rv = -1;
                    break;
                }
                if ((subNode->valueint != 2) && (subNode->valueint != 3)) {
                    LOG_ERROR("unexpected revision # = "FMTs64, subNode->valueint);
                    rv = -1;
                    break;
                }
            }
        }
        if (jsonResponse) {
            cJSON2_Delete(jsonResponse);
            jsonResponse = NULL;
        }
        // check if succeed
        if (rv) {
            VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));
        } else {
            // get the expected revisions from metadata
            if (count > 0) LOG_ALWAYS("wait %d ms for expected revisions", count*1000);
            break;
        }
    } while (++count < CLOUDDOC_TIMEOUT_RETRY_COUNT);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyUploadedRevisionNumber", rv);

    //setCcdTestInstanceNum(clientPCId);
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    // 7.5 download rev 2 document
    // 7.6 download rev 2 preview
    {
        TargetDevice *target = getTargetDevice();
        target->deleteFile(docx_clone);
        target->deleteFile(docx_preview_clone);
    }
 
    CD_DOWNLOAD_SKIP(docx_upload.c_str(), comp_id.c_str(), "2", docx_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX
    // compare w/ the original docx file instead of the tmp_docx (bcz it might
    // be overwritten by the rev 3 by now (syncback)
    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_FILE, CLOUDDOC_DOCX_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadFileRev2", rv);

    CD_DPREVIEW_SKIP(docx_upload.c_str(), comp_id.c_str(), "2", docx_preview_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX
    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_JPG_FILE, CLOUDDOC_DOCX_JPG_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadPreviewFileRev2", rv);

    // 7.7 download rev 3 document
    // 7.8 download rev 3 preview
    Util_rm_dash_rf(docx_clone);
    Util_rm_dash_rf(docx_preview_clone);

    CD_DOWNLOAD_SKIP(docx_upload.c_str(), comp_id.c_str(), "3", docx_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX
    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_FILE2, CLOUDDOC_DOCX_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadFileRev3", rv);

    CD_DPREVIEW_SKIP(docx_upload.c_str(), comp_id.c_str(), "3", docx_preview_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX
    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_JPG_FILE2, CLOUDDOC_DOCX_JPG_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadPreviewFileRev3", rv);

    // 7.9 check conflict
    CD_CHECK_CONFLICT(docx_upload.c_str(), comp_id.c_str(), response, rv);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "CheckConflict", rv);

    // 8. Move

    // 8.1 Test Move command
    //setCcdTestInstanceNum(clientPCId);
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
    CD_MOVE(docx_upload.c_str(), comp_id.c_str(), docx_move.c_str(), response, rv);
    rv = clouddochttp_getdoc_info_wait(docx_move_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyMoveFile", rv);

    // 8.1.1 Test Move Async command
    // Rename to original
    CD_MOVE_ASYNC(docx_move_upload.c_str(), comp_id.c_str(), tmp_docx.c_str(), response, rv);
    rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyMoveFileAsync_1", rv);
    // Rename to new name
    CD_MOVE_ASYNC(docx_upload.c_str(), comp_id.c_str(), docx_move.c_str(), response, rv);
    rv = clouddochttp_getdoc_info_wait(docx_move_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, false);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyMoveFileAsync_2", rv);

    // 8.2 Download moved file
    {
        TargetDevice *target = getTargetDevice();
        target->deleteFile(docx_clone);
        target->deleteFile(docx_preview_clone);
    }

    CD_DOWNLOAD_SKIP(docx_move_upload.c_str(), comp_id.c_str(), "3", docx_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX
    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_FILE2, CLOUDDOC_DOCX_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadFileMoved", rv);

    CD_DPREVIEW_SKIP(docx_move_upload.c_str(), comp_id.c_str(), "3", docx_preview_clone.c_str(), response, rv);
    VPLThread_Sleep(VPLTIME_FROM_SEC(2)); // XXX
    rv = clouddochttp_compareFile(CLOUDDOC_DOCX_JPG_FILE2, CLOUDDOC_DOCX_JPG_FILE_CLONE);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDownloadPreviewFileMoved", rv);

   
    // cleanup. Test delete_async
    CD_DELETE_ASYNC(docx_move_upload.c_str(), comp_id.c_str(), NULL, response, rv);
    rv = clouddochttp_getdoc_info_wait(docx_upload.c_str(), revision, component_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, true);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyDeleteFileAsync", rv);

    
    //setCcdTestInstanceNum(clientPCId);
    SET_TARGET_MACHINE(TEST_CLOUDDOC_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
    // 9. Check upload request
    // Create File for testing
    
    {
        std::string curDir;
        std::string dxshell_docx_dummy;
        rv = getCurDir(curDir);
        if (rv < 0) {
            LOG_ERROR("failed to get current dir. error = %d", rv);
            goto exit;
        }
        dxshell_docx_dummy = curDir + "/" + CLOUDDOC_DOCX_DUMMY;

        int filesize = 50;
        {
            TargetDevice *myRemoteDevice = getTargetDevice();
            if(myRemoteDevice->getDeviceClass() == "Windows RT Machine"
                || myRemoteDevice->getOsVersion() == OS_ANDROID
                || isIOS(myRemoteDevice->getOsVersion())) {
                //For mobile device, use smaller file
                filesize = 5;
            }
            if (myRemoteDevice != NULL) {
                delete myRemoteDevice;
                myRemoteDevice = NULL;
            }
        }
        rv = create_dummy_file(dxshell_docx_dummy.c_str(), filesize*1024*1024);

        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC8", rv);
        rv = clouddochttp_pushFile(CLOUDDOC_DOCX_DUMMY);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForTC8", rv);
    }

    
    // 9.1 Check Requst ID
    CD_UPLOAD_BASIC(docx_dummy.c_str(), docx_preview.c_str(), response, rv);
    
    docx_dummy_upload = urlEncodingLoose(docx_dummy_upload);
    rv = clouddochttp_getreq_info(docx_dummy_upload.c_str(), request_id);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyRequestID", rv);

    // 9.2 Test GetProgress
    ss.str("");
    ss << request_id;
    req_id = ss.str();

    clouddochttp_get_progress_info_wait(docx_dummy_upload.c_str(), req_id, CLOUDDOC_TIMEOUT_RETRY_COUNT);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "GetProgress", rv);

    // 9.3 Cancel the request

    CD_CANCEL_REQ(req_id.c_str(), response, rv);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "CancelReq", rv);

    // 9.4 Make sure request is not in list after cancel
    rv = clouddochttp_getreq_info_wait(docx_dummy_upload.c_str(), request_id, CLOUDDOC_TIMEOUT_RETRY_COUNT, true);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyCancelReq", rv);

    // 10. Upload large file (100M)
    if (full) {
        {
            std::string curDir;
            std::string dxshell_docx_dummy;
            rv = getCurDir(curDir);
            if (rv < 0) {
                LOG_ERROR("failed to get current dir. error = %d", rv);
                goto exit;
            }
            dxshell_docx_dummy = curDir + "/" + CLOUDDOC_DOCX_100M_DUMMY;

            int filesize = 100;
            rv = create_dummy_file(dxshell_docx_dummy.c_str(), filesize*1024*1024);

            CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForUploadLargeFile", rv);
            rv = clouddochttp_pushFile(CLOUDDOC_DOCX_100M_DUMMY);
            CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "PrepareDataForUploadLargeFile", rv);
        }
        // 10.1 Check Requst ID
        CD_UPLOAD_BASIC(docx_dummy_100M.c_str(), docx_preview.c_str(), response, rv);
        std::string docx_dummy_100M_upload_encoding = urlEncodingLoose(docx_dummy_100M_upload);
        rv = clouddochttp_getreq_info(docx_dummy_100M_upload_encoding.c_str(), request_id);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyRequestID", rv);

        // 10.2 Test GetProgress
        ss.str("");
        ss << request_id;
        req_id = ss.str();

        clouddochttp_get_progress_info_wait(docx_dummy_100M_upload_encoding.c_str(), req_id, CLOUDDOC_TIMEOUT_RETRY_COUNT);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "GetProgress", rv);
        LOG_ALWAYS("name: %s, encoding_name: %s", docx_dummy_100M_upload.c_str(), docx_dummy_100M_upload_encoding.c_str());

        check_event_visitor_ctx ctx;
        {
            const char *testStr = "CheckCloudDocUploadCompleteEventThread";

            ctx.done        = false;
            ctx.userid      = userId;
            ctx.event.mutable_doc_save_and_go_completion()->set_change_type(ccd::DOC_SAVE_AND_GO_UPDATE);
            ctx.event.mutable_doc_save_and_go_completion()->set_docname(docx_dummy_100M_upload_encoding);
            CREATE_CLOUDDOC_EVENT_VISIT_THREAD(testStr);
        }

        CHECK_CLOUDDOC_EVENT_VISIT_RESULT("CloudDocUploadCompleteEvent", 10000000);

        rv = clouddochttp_getdoc_info_wait(docx_dummy_100M_upload.c_str(), revision, component_id, 300, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "VerifyFileUploaded", rv);

        ss.str("");
        ss << component_id;
        comp_id = ss.str();

        ss.str("");
        ss << revision;
        rev_id = ss.str();

        LOG_ALWAYS("name = %s, rev = "FMTu64", comp_id = "FMTu64, docx_upload.c_str(), revision, component_id);
    }

    // 11. Upload doc/docx/ppt/pptx/xls/xlsx
    rv = clouddochttp_uploadDir("/GoldenTest/TestDocData/DocTestSet1", docx_preview, remoteroot_path, full);
    CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "DocTestSet1", rv);

    // 12. Upload 101 files including chinese and european
    if(full){
        rv = clouddochttp_uploadDir("/GoldenTest/TestDocData/DocTestSet2", docx_preview, remoteroot_path, false);
        CHECK_AND_PRINT_RESULT(TEST_CLOUDDOC_STR, "DocTestSet2", rv);
    }

exit:
    if (jsonResponse) {
        cJSON2_Delete(jsonResponse);
        jsonResponse = NULL;
    }

    LOG_ALWAYS("\n\n== Freeing cloud PC ==");
    if(set_target_machine("CloudPC") < 0)
        setCcdTestInstanceNum(cloudPCId);

    {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client ==");
    if(set_target_machine("MD") < 0)
        setCcdTestInstanceNum(clientPCId);
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client 2 ==");
    //setCcdTestInstanceNum(clientPCId2);
    if(set_target_machine("Client") < 0)
        setCcdTestInstanceNum(clientPCId2);
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    return rv;
}
