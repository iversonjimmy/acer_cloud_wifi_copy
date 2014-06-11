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

#include <vpl_plat.h>
#include "vcs_util.hpp"
#include "vcs_defs.hpp"
#include "vcs_common.hpp"
#include "vcs_common_priv.hpp"
#include "cJSON2.h"
#include "gvm_errors.h"
#include "gvm_file_utils.hpp"
#include "gvm_misc_utils.h"
#include "scopeguard.hpp"
#include "vpl_conv.h"
#include "vpl_fs.h"
#include "vplex_http2.hpp"
#include "vplex_http_util.hpp"
#include "vplex_serialization.h"
#include "vplu_sstr.hpp"

#include <cslsha.h>

#include <string>
#include <sstream>

#include "log.h"

static int parseRevision(cJSON2* fileEntryObject,
                         VcsFileRevision& fileLatestRevision)
{
    fileLatestRevision.clear();
    if(fileEntryObject == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(fileEntryObject->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", fileEntryObject->type);
        return -1;
    }

    cJSON2* revisionAttrib = fileEntryObject->child;

    while(revisionAttrib != NULL) {
        if(revisionAttrib->type==cJSON2_String){
            if(strcmp("previewUri", revisionAttrib->string)==0) {
                fileLatestRevision.previewUri = revisionAttrib->valuestring;
            }else if(strcmp("downloadUrl", revisionAttrib->string)==0) {
                fileLatestRevision.downloadUrl = revisionAttrib->valuestring;
            }
        }else if(revisionAttrib->type==cJSON2_Number) {
            if(strcmp("revision", revisionAttrib->string)==0) {
                fileLatestRevision.revision = revisionAttrib->valueint;
            }else if(strcmp("size", revisionAttrib->string)==0) {
                fileLatestRevision.size = revisionAttrib->valueint;
            }else if(strcmp("lastChanged", revisionAttrib->string)==0) {
                fileLatestRevision.lastChangedSecResolution = VPLTime_FromSec(revisionAttrib->valueint);
            }else if(strcmp("updateDevice", revisionAttrib->string)==0) {
                fileLatestRevision.updateDevice = revisionAttrib->valueint;
            }
        }else if(revisionAttrib->type==cJSON2_True ||
                 revisionAttrib->type==cJSON2_False)
        {
            if(strcmp("noACS", revisionAttrib->string)==0) {
                fileLatestRevision.noAcs = false;
                if(revisionAttrib->type==cJSON2_True) {
                    fileLatestRevision.noAcs = true;
                }
            }
        }
        revisionAttrib = revisionAttrib->next;
    }
    return 0;
}

static int parseFileEntry(cJSON2* fileEntryObject,
                          bool& isDir,
                          VcsFile& file,
                          VcsFolder& folder)
{
    file.clear();
    folder.clear();

    if(fileEntryObject == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(fileEntryObject->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", fileEntryObject->type);
        return -1;
    }

    cJSON2* entryAttrib = fileEntryObject->child;

    while(entryAttrib != NULL) {
        if(entryAttrib->type == cJSON2_String) {
            if(strcmp("name", entryAttrib->string)==0) {
                file.name = entryAttrib->valuestring;
                folder.name = entryAttrib->valuestring;
            }else if(strcmp("type", entryAttrib->string)==0) {
                if(strcmp("dir", entryAttrib->valuestring)==0) {
                    isDir = true;
                }else if(strcmp("file", entryAttrib->valuestring)==0) {
                    isDir = false;
                }else{
                    LOG_ERROR("Entry type of %s not recognized", entryAttrib->valuestring);
                    return -2;
                }
            }else if(strcmp("lastChangedNanoSecs", entryAttrib->string)==0) {
                // Bug 16586: lastChangedNanoSecs is actually in microseconds.
                file.lastChanged = VPLTime_FromMicrosec(
                        VPLConv_strToU64(entryAttrib->valuestring, NULL, 10));
            }else if(strcmp("createDateNanoSecs", entryAttrib->string)==0) {
                // Bug 16586: createDateNanoSecs is actually in microseconds.
                file.createDate = VPLTime_FromMicrosec(
                        VPLConv_strToU64(entryAttrib->valuestring, NULL, 10));
            }else if(strcmp("clientGeneratedHash", entryAttrib->string)==0) {
                file.hashValue = entryAttrib->valuestring;
            }
        }else if(entryAttrib->type == cJSON2_Number) {
            if(strcmp("compId", entryAttrib->string)==0) {
                file.compId = entryAttrib->valueint;
                folder.compId = entryAttrib->valueint;
            }else if(strcmp("version", entryAttrib->string)==0) {
                file.version = entryAttrib->valueint;
                folder.version = entryAttrib->valueint;
            }else if(strcmp("originDevice", entryAttrib->string)==0) {
                file.originDevice = entryAttrib->valueint;
            }else if(strcmp("numOfRevisions", entryAttrib->string)==0) {
                file.numRevisions = entryAttrib->valueint;
            }
        }else if(entryAttrib->type == cJSON2_Object) {
            if(strcmp("latestRevision", entryAttrib->string)==0) {
                int rc = parseRevision(entryAttrib, file.latestRevision);
                if(rc!=0) {
                    LOG_ERROR("Parsing latest revision:%d.  Continuing", rc);
                }
            }
        }
        entryAttrib = entryAttrib->next;
    }

    return 0;
}

static int parseFileListArray(cJSON2* fileListArray,
                              VcsGetDirResponse& getDirResponse)
{
    getDirResponse.dirs.clear();
    getDirResponse.files.clear();

    if(fileListArray == NULL) {
        LOG_ERROR("fileListArray is null (not zero elements)");
        return -1;
    }
    cJSON2* currEntry = fileListArray->child;

    while(currEntry != NULL) {
        bool isDir = false;
        VcsFile file;
        VcsFolder folder;

        int rc = parseFileEntry(currEntry, isDir, file, folder);
        if(rc!=0) {
            LOG_ERROR("File entry parsing");
        }else{
            if(isDir) {
                getDirResponse.dirs.push_back(folder);
            }else{
                getDirResponse.files.push_back(file);
            }
        }
        currEntry = currEntry->next;
    }

    return 0;
}

static int parseVcsGetDirResponse(cJSON2* cjsonGetDirResponse,
                                  VcsGetDirResponse& vcsResponse)
{
    int rv = 0;
    int rc;

    vcsResponse.clear();

    if(cjsonGetDirResponse == NULL) {
        LOG_ERROR("No response");
        return -1;
    }
    if(cjsonGetDirResponse->prev != NULL) {
        LOG_ERROR("Unexpected prev value. Continuing.");
    }
    if(cjsonGetDirResponse->next != NULL) {
        LOG_ERROR("Unexpected next value. Continuing.");
    }

    if(cjsonGetDirResponse->type == cJSON2_Object) {
        cJSON2* currObject = cjsonGetDirResponse->child;
        if(currObject == NULL) {
            LOG_ERROR("Object returned has no children.");
            return -2;
        }
        if(currObject->prev != NULL) {
            LOG_ERROR("Unexpected prev value. Continuing.");
        }

        while(currObject != NULL) {
            //LOG_ALWAYS("currObject "FMT_cJSON2_OBJ, VAL_cJSON2_OBJ(currObject));
            if(currObject->type == cJSON2_Array) {
                if(strcmp("fileList", currObject->string)==0) {
                    rc = parseFileListArray(currObject, vcsResponse);
                    if(rc != 0) {
                        LOG_ERROR("FileList parsing error:%d", rc);
                        rv = rc;
                    }
                }
            }else if(currObject->type == cJSON2_Number) {
                if(strcmp("numOfFiles", currObject->string)==0) {
                    vcsResponse.numOfFiles = currObject->valueint;
                }else if(strcmp("currentDirVersion", currObject->string)==0) {
                    vcsResponse.currentDirVersion = currObject->valueint;
                }else if(strcmp("datasetStatus", currObject->string)==0) {
                    // This field is deprecated. Ignore.
                }else{
                    // do nothing, other fields should be ignored.
                }
            }else{
                LOG_WARN("Unrecognized object element:"FMT_cJSON2_OBJ,
                         VAL_cJSON2_OBJ(currObject));
            }

            currObject = currObject->next;
        }

    }else{
        LOG_ERROR("Response returned was not a cjson object:%d",
                  cjsonGetDirResponse->type);
    }

    return 0;
}

static int addSessionHeaders(const VcsSession& vcsSession,
                             VPLHttp2& httpHandle)
{   // See http://wiki.ctbg.acer.com/wiki/index.php/VCS_Design#Authentication
    int rc;
    rc = addSessionHeadersHelper(vcsSession,
                                 httpHandle,
                                 3);  // VCS_API_VERSION
    if (rc != 0) {
        LOG_ERROR("addSessionHeadersHelper:%d", rc);
    }
    return rc;
}

// See http://wiki.ctbg.acer.com/wiki/index.php/VCS_Design#GET_dir for field
// descriptions.
static int vcs_read_folder_helper(const VcsSession& vcsSession,
                                  VPLHttp2& httpHandle,
                                  const VcsDataset& dataset,
                                  const std::string& folder,
                                  u64 compId,
                                  bool pageIndexExist,
                                  u64 pageIndex,
                                  bool pageMaxExist,
                                  u64 pageMax,
                                  bool sortByExist,
                                  std::string sortBy,
                                  bool printLog,
                                  VcsGetDirResponse& getDirResponse)
{
    int rv=0;
    getDirResponse.clear();

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }

    std::string folderNoLeadingOrTrailingSlash;
    Util_trimSlashes(folder, folderNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/dir/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(folderNoLeadingOrTrailingSlash, "/");
    ss <<"?compId=" << compId;
    if(pageIndexExist) {
        ss << "&index=" << pageIndex;
    }
    if(pageMaxExist) {
        ss << "&max=" << pageMax;
    }
    if(sortByExist) {
        ss << "&sortBy=" << VPLHttp_UrlEncoding(sortBy, "");
    }
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string httpGetResponse;
    rv = httpHandle.Get(/*out*/httpGetResponse);
    if(rv != 0) {
        LOG_ERROR("http GET returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64,
                  rv, dataset.id, folder.c_str(), compId);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpGetResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpGetResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpGetResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsGetDirResponse(json_response, getDirResponse);
        if(rv!=0) {
            LOG_ERROR("parseVcsGetDir:%d", rv);
            goto exit_with_server_response;
        }
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpGetResponse.c_str());
    }
    return rv;
}

// DEPRECATED: Infra would rather us not call get/dir without paging.
//int vcs_read_folder(const VcsSession& vcsSession,
//                    VPLHttp2& httpHandle,
//                    const VcsDataset& dataset,
//                    const std::string& folder,
//                    u64 compId,
//                    bool printLog,
//                    VcsGetDirResponse& getDirResponse)
//{
//    int rv;
//    getDirResponse.clear();
//    rv = vcs_read_folder_helper(vcsSession,
//                                httpHandle,
//                                dataset,
//                                folder,
//                                compId,
//                                false,
//                                0,
//                                false,
//                                0,
//                                false,
//                                std::string(""),
//                                printLog,
//                                getDirResponse);
//    if(rv != 0) {
//        LOG_ERROR("vcs_read_folder_helper:%d, %s,dset:"FMTu64,
//                  rv, folder.c_str(), dataset.id);
//    }
//    return rv;
//}

int vcs_read_folder_paged(const VcsSession& vcsSession,
                          VPLHttp2& httpHandle,
                          const VcsDataset& dataset,
                          const std::string& folder,
                          u64 compId,
                          u64 pageIndex,
                          u64 pageMax,
                          bool printLog,
                          VcsGetDirResponse& getDirResponse)
{
    int rv;
    rv = vcs_read_folder_helper(vcsSession,
                                httpHandle,
                                dataset,
                                folder,
                                compId,
                                true,
                                pageIndex,
                                true,
                                pageMax,
                                false,
                                std::string(""),
                                printLog,
                                getDirResponse);
    if(rv != 0) {
        LOG_ERROR("vcs_read_folder_helper:%d, %s,dset:"FMTu64,
                  rv, folder.c_str(), dataset.id);
    }
    return rv;
}

int vcs_delete_file(const VcsSession& vcsSession,
                    VPLHttp2& httpHandle,
                    const VcsDataset& dataset,
                    const std::string& filepath,
                    u64 compId,
                    u64 revision,
                    bool printLog)
{
    int rv = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string filepathNoLeadingOrTrailingSlash;
    Util_trimSlashes(filepath, filepathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/file/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(filepathNoLeadingOrTrailingSlash, "/");
    ss << "?compId=" << compId;
    ss << "&revision=" << revision;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string httpDeleteResponse;
    rv = httpHandle.Delete(/*out*/httpDeleteResponse);
    if(rv != 0) {
        LOG_ERROR("http DELETE returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64,
                  rv, dataset.id, filepath.c_str(), compId);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpDeleteResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }
    
exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpDeleteResponse.c_str());
    }
    return rv;
}

int vcs_delete_dir(const VcsSession& vcsSession,
                   VPLHttp2& httpHandle,
                   const VcsDataset& dataset,
                   const std::string& dirpath,
                   u64 compId,
                   bool isRecursive,
                   bool hasDirDatasetVersion,
                   u64 dirDatasetVersion,
                   bool printLog)
{
    int rv = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string dirpathNoLeadingOrTrailingSlash;
    Util_trimSlashes(dirpath, dirpathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/dir/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(dirpathNoLeadingOrTrailingSlash, "/");
    ss << "?compId=" << compId;
    if (isRecursive) {
        ss << "&isRecursive=true";
    }
    if (hasDirDatasetVersion) {
        ss << "&dirDatasetVersion=" << dirDatasetVersion;
    }
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string httpDeleteResponse;
    rv = httpHandle.Delete(/*out*/httpDeleteResponse);
    if(rv != 0) {
        LOG_ERROR("http DELETE returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",dirPath:%s,"
                  "compId:"FMTu64,
                  rv, dataset.id, dirpath.c_str(), compId);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpDeleteResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpDeleteResponse.c_str());
    }
    return rv;
}

static int parseVcsMakeDirResponse(cJSON2* cjsonMakeDirResponse,
                                   VcsMakeDirResponse& vcsResponse)
{
    vcsResponse.clear();

    if(cjsonMakeDirResponse == NULL) {
        LOG_ERROR("No response");
        return -1;
    }
    if(cjsonMakeDirResponse->prev != NULL) {
        LOG_ERROR("Unexpected prev value. Continuing.");
    }
    if(cjsonMakeDirResponse->next != NULL) {
        LOG_ERROR("Unexpected next value. Continuing.");
    }

    if(cjsonMakeDirResponse->type == cJSON2_Object) {
        cJSON2* currObject = cjsonMakeDirResponse->child;
        if(currObject == NULL) {
            LOG_ERROR("Object returned has no children.");
            return -2;
        }
        if(currObject->prev != NULL) {
            LOG_ERROR("Unexpected prev value. Continuing.");
        }

        while(currObject != NULL) {
            //LOG_ALWAYS("currObject "FMT_cJSON2_OBJ, VAL_cJSON2_OBJ(currObject));
            if(currObject->type == cJSON2_Number) {
                if(strcmp("compId", currObject->string)==0) {
                    vcsResponse.compId = currObject->valueint;
                }else if(strcmp("originDevice", currObject->string)==0) {
                    vcsResponse.originDevice = currObject->valueint;
                }else if(strcmp("lastChanged", currObject->string)==0) {
                    vcsResponse.lastChangedSecResolution = VPLTime_FromSec(currObject->valueint);
                }else{
                    // do nothing, other fields should be ignored.
                }
            }else if(currObject->type == cJSON2_String) {
                if(strcmp("name", currObject->string)==0) {
                    vcsResponse.name = currObject->valuestring;
                }else{
                    // do nothing, other fields should be ignored.
                }
            }else{
                LOG_WARN("Unrecognized object element:"FMT_cJSON2_OBJ,
                         VAL_cJSON2_OBJ(currObject));
            }

            currObject = currObject->next;
        }

    }else{
        LOG_ERROR("Response returned was not a cjson object:%d",
                  cjsonMakeDirResponse->type);
    }

    return 0;
}


int vcs_make_dir(const VcsSession& vcsSession,
                 VPLHttp2& httpHandle,
                 const VcsDataset& dataset,
                 const std::string& dirpath,
                 u64 parentCompId,
                 u64 infoLastChanged,
                 u64 infoCreateDate,
                 u64 infoUpdateDeviceId,
                 bool printLog,
                 VcsMakeDirResponse& makeDirResponse)
{
    int rv = 0;
    makeDirResponse.clear();

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string dirpathNoLeadingOrTrailingSlash;
    Util_trimSlashes(dirpath, dirpathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/dir/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(dirpathNoLeadingOrTrailingSlash, "/");
    ss << "?parentCompId=" << parentCompId;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    ss.str("");
    ss << "{";
    ss <<   "\"lastChanged\":"  << infoLastChanged    <<",";
    ss <<   "\"createDate\":"   << infoCreateDate     << ",";
    ss <<   "\"updateDevice\":" << infoUpdateDeviceId;
    ss << "}";
    std::string jsonBody = ss.str();
    if(printLog){ LOG_ALWAYS("jsonBody:%s", jsonBody.c_str()); }

    std::string httpPostResponse;
    rv = httpHandle.Post(jsonBody, /*out*/httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http POST returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",dirPath:%s,"
                  "parentCompId:"FMTu64,
                  rv, dataset.id, dirpath.c_str(), parentCompId);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpPostResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpPostResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsMakeDirResponse(json_response, makeDirResponse);
        if(rv!=0) {
            LOG_ERROR("parseVcsMakeDir:%d", rv);
            goto exit_with_server_response;
        }
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}

int vcs_access_info_for_file_put(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 bool printLog,
                                 VcsAccessInfo& accessInfo_out)
{
    int rv = 0;
    accessInfo_out.clear();

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/accessinfo/";
    ss << dataset.id;
    ss << "?method=PUT";
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string httpGetResponse;
    rv = httpHandle.Get(httpGetResponse);
    if(rv != 0) {
        LOG_ERROR("http GET returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64,
                  rv, dataset.id);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpGetResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpGetResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpGetResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsAccessInfoResponse(json_response, accessInfo_out);
        if(rv!=0) {
            LOG_ERROR("parseVcsMakeDir:%d", rv);
            goto exit_with_server_response;
        }
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpGetResponse.c_str());
    }
    return rv;
}

int vcs_access_info_for_file_get(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 const std::string& path,
                                 u64 compId,
                                 u64 revision,
                                 bool printLog,
                                 VcsAccessInfo& accessInfo_out)
{
    int rv = 0;
    accessInfo_out.clear();

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(path, pathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/accessinfo/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/");
    ss << "?method=GET";
    ss << "&compId=" << compId;
    ss << "&revision=" << revision;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string httpGetResponse;
    rv = httpHandle.Get(httpGetResponse);
    if(rv != 0) {
        LOG_ERROR("http GET returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64",revision:"FMTu64,
                  rv, dataset.id, path.c_str(), compId, revision);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpGetResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpGetResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpGetResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsAccessInfoResponse(json_response, accessInfo_out);
        if(rv!=0) {
            LOG_ERROR("parseVcsMakeDir:%d", rv);
            goto exit_with_server_response;
        }
    }
    
exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpGetResponse.c_str());
    }
    return rv;
}

static int parseVcsGetCompId(cJSON2* cjsonAccessInfo,
                             u64& compId_out)
{
    int rv = 0;
    compId_out=0;

    if(cjsonAccessInfo == NULL) {
        LOG_ERROR("No response");
        return -1;
    }
    if(cjsonAccessInfo->prev != NULL) {
        LOG_ERROR("Unexpected prev value. Continuing.");
    }
    if(cjsonAccessInfo->next != NULL) {
        LOG_ERROR("Unexpected next value. Continuing.");
    }

    if(cjsonAccessInfo->type == cJSON2_Object) {
        cJSON2* currObject = cjsonAccessInfo->child;
        if(currObject == NULL) {
            LOG_ERROR("Object returned has no children.");
            return -2;
        }
        if(currObject->prev != NULL) {
            LOG_ERROR("Unexpected prev value. Continuing.");
        }

        while(currObject != NULL) {
            //LOG_ALWAYS("currObject "FMT_cJSON2_OBJ, VAL_cJSON2_OBJ(currObject));
            if(currObject->type == cJSON2_Number) {
                if(strcmp("compId", currObject->string)==0) {
                    compId_out=currObject->valueint;
                }
            }else{
                LOG_WARN("Unrecognized object element:"FMT_cJSON2_OBJ,
                         VAL_cJSON2_OBJ(currObject));
            }
            currObject = currObject->next;
        }

    }else{
        LOG_ERROR("Response returned was not a cjson object:%d",
                  cjsonAccessInfo->type);
        rv = -3;
    }

    return rv;
}

int vcs_get_comp_id(const VcsSession& vcsSession,
                    VPLHttp2& httpHandle,
                    const VcsDataset& dataset,
                    const std::string& path,
                    bool printLog,
                    u64& compId_out)
{
    int rv = 0;
    compId_out = 0;

    {
        int temp_rc = httpHandle.SetDebug(printLog);
        if(temp_rc != 0) {
            LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", temp_rc);
        }
    }
    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(path, /*out*/ pathNoLeadingOrTrailingSlash);

    std::string url = SSTR(vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/getcompid/"
            << dataset.id << "/" << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/"));

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string emptyBody;
    std::string httpPostResponse;
    rv = httpHandle.Post(emptyBody, /*out*/ httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http POST returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s",
                  rv, dataset.id, path.c_str());
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), /*out*/ rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpPostResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpPostResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsGetCompId(json_response, compId_out);
        if(rv != 0) {
            LOG_ERROR("parseVcsGetCompId:%d", rv);
            goto exit_with_server_response;
        }
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}

static int parseVcsGetDatasetInfo(cJSON2* cjsonResponse, u64& currentVersion_out)
{
    int rv = 0;
    currentVersion_out=0;

    if(cjsonResponse == NULL) {
        LOG_ERROR("No response");
        return -1;
    }
    if(cjsonResponse->prev != NULL) {
        LOG_ERROR("Unexpected prev value. Continuing.");
    }
    if(cjsonResponse->next != NULL) {
        LOG_ERROR("Unexpected next value. Continuing.");
    }

    if(cjsonResponse->type == cJSON2_Object) {
        cJSON2* currObject = cjsonResponse->child;
        if(currObject == NULL) {
            LOG_ERROR("Object returned has no children.");
            return -2;
        }
        if(currObject->prev != NULL) {
            LOG_ERROR("Unexpected prev value. Continuing.");
        }

        while(currObject != NULL) {
            //LOG_ALWAYS("currObject "FMT_cJSON2_OBJ, VAL_cJSON2_OBJ(currObject));
            if(currObject->type == cJSON2_Number) {
                if(strcmp("currentVersion", currObject->string)==0) {
                    currentVersion_out = currObject->valueint;
                }
            }else{
                LOG_WARN("Unrecognized object element:"FMT_cJSON2_OBJ,
                         VAL_cJSON2_OBJ(currObject));
            }
            currObject = currObject->next;
        }

    }else{
        LOG_ERROR("Response returned was not a cjson object:%d",
                  cjsonResponse->type);
        rv = -3;
    }

    return rv;
}

int vcs_get_dataset_info(const VcsSession& vcsSession,
                         VPLHttp2& httpHandle,
                         const VcsDataset& dataset,
                         bool printLog,
                         u64& currentVersion_out)
{
    int rv;
    currentVersion_out = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/getdatasetinfo/" << dataset.id;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string emptyBody;
    std::string httpPostResponse;
    rv = httpHandle.Post(emptyBody, /*out*/httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http POST returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64,
                  rv, dataset.id);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpPostResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpPostResponse.c_str());
            // TODO: better error code?
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsGetDatasetInfo(json_response, currentVersion_out);
        if (rv != 0) {
            LOG_ERROR("parseVcsGetDatasetInfo:%d", rv);
            // TODO: better error code? (parseVcsGetDatasetInfo returns -1,-2,-3)
            goto exit_with_server_response;
        }
    }
    
exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return 0;
}

static int parseRevisionListArray(cJSON2* fileListArray,
                                  std::vector<VcsFileRevision>& revisionList)
{
    revisionList.clear();

    if(fileListArray == NULL) {
        LOG_ERROR("fileListArray is null (not zero elements)");
        return -1;
    }
    cJSON2* currEntry = fileListArray->child;

    while(currEntry != NULL) {
        VcsFileRevision revision_out;

        int rc = parseRevision(currEntry, revision_out);
        if(rc!=0) {
            LOG_ERROR("Revision entry parsing:%d Skipping and continuing.", rc);
        }else{
            revisionList.push_back(revision_out);
        }
        currEntry = currEntry->next;
    }

    return 0;
}

static int parseVcsFileMetadataResponse(cJSON2* fileEntryObject,
                                        VcsFileMetadataResponse& file)
{
    file.clear();

    if(fileEntryObject == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(fileEntryObject->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", fileEntryObject->type);
        return -1;
    }

    cJSON2* entryAttrib = fileEntryObject->child;

    while(entryAttrib != NULL) {
        if(entryAttrib->type == cJSON2_String) {
            if(strcmp("name", entryAttrib->string)==0) {
                file.name = entryAttrib->valuestring;
            }else if(strcmp("lastChangedNanoSecs", entryAttrib->string)==0) {
                // Bug 16586: lastChangedNanoSecs is actually in microseconds.
                file.lastChanged = VPLTime_FromMicrosec(
                        VPLConv_strToU64(entryAttrib->valuestring, NULL, 10));
            }else if(strcmp("createDateNanoSecs", entryAttrib->string)==0) {
                // Bug 16586: createDateNanoSecs is actually in microseconds.
                file.createDate = VPLTime_FromMicrosec(
                        VPLConv_strToU64(entryAttrib->valuestring, NULL, 10));
            }
        }else if(entryAttrib->type == cJSON2_Number) {
            if(strcmp("compId", entryAttrib->string)==0) {
                file.compId = entryAttrib->valueint;
            }else if(strcmp("originDevice", entryAttrib->string)==0) {
                file.originDevice = entryAttrib->valueint;
            }else if(strcmp("numOfRevisions", entryAttrib->string)==0) {
                file.numOfRevisions = entryAttrib->valueint;
            }
        }else if(entryAttrib->type == cJSON2_Array) {
            if(strcmp("revisionList", entryAttrib->string)==0) {
                int rc = parseRevisionListArray(entryAttrib, file.revisionList);
                if(rc!=0) {
                    LOG_ERROR("Parsing revision list:%d.  Continuing", rc);
                }
            }
        }
        entryAttrib = entryAttrib->next;
    }

    return 0;
}

int vcs_post_file_metadata(const VcsSession& vcsSession,
                           VPLHttp2& httpHandle,
                           const VcsDataset& dataset,
                           const std::string& path,
                           u64 parentCompId,
                           bool hasCompId,      // compId is unknown for new files
                           u64 compId,          // Only valid when hasCompId==true
                           u64 uploadRevision,  // lastKnownRevision+1, (for example, 1 for new files)
                           VPLTime_t lastChanged,
                           VPLTime_t createDate,
                           u64 fileSize,
                           const std::string& contentHash,
                           const std::string& clientGeneratedHash,
                           u64 infoUpdateDeviceId,
                           bool accessUrlExists,
                           const std::string& accessUrl,
                           bool printLog,
                           VcsFileMetadataResponse& response_out)
{
    int rv = 0;
    response_out.clear();
    u64 lastChanged_SecUtimeServerEnforced;
    u64 createDate_SecUtimeServerEnforced;

    lastChanged_SecUtimeServerEnforced = VPLTime_ToSec(lastChanged);
    createDate_SecUtimeServerEnforced = VPLTime_ToSec(createDate);
    if (lastChanged_SecUtimeServerEnforced == 0 ||
        createDate_SecUtimeServerEnforced == 0)
    {
        FAILED_ASSERT("Cannot be 0,:("FMTu64"->"FMTu64"),("FMTu64"->"FMTu64")",
                      lastChanged,
                      lastChanged_SecUtimeServerEnforced,
                      createDate,
                      createDate_SecUtimeServerEnforced);
    }

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(path, pathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/filemetadata/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/");
    ss << "?folderCompId=" << parentCompId;
    if(hasCompId) {
        ss << "&compId=" << compId;
    }
    ss << "&uploadRevision=" << uploadRevision;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    ss.str("");
    ss << "{";
    ss <<   "\"lastChanged\":"  << lastChanged_SecUtimeServerEnforced  <<",";
    ss <<   "\"createDate\":"   << createDate_SecUtimeServerEnforced   <<",";
    ss <<   "\"lastChangedNanoSecs\":" << lastChanged                  <<",";
    ss <<   "\"createDateNanoSecs\":" << createDate                    <<",";
    ss <<   "\"size\":"         << fileSize                            <<",";
    ss <<   "\"contentHash\":\""  << contentHash                       <<"\",";  // string-type
    ss <<   "\"clientGeneratedHash\":\""  << clientGeneratedHash       <<"\",";  // string-type
    ss <<   "\"updateDevice\":" << infoUpdateDeviceId;
    if (accessUrlExists) {
        ss <<",";
        ss <<   "\"accessUrl\":\""    << accessUrl <<"\""; // string-type
    }
    ss << "}";
    std::string jsonBody = ss.str();
    if(printLog){ LOG_ALWAYS("jsonBody:%s", jsonBody.c_str()); }

    std::string httpPostResponse;
    rv = httpHandle.Post(jsonBody, httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http POST returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64",uploadRevision:"FMTu64,
                  rv, dataset.id, path.c_str(), compId, uploadRevision);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpPostResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpPostResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsFileMetadataResponse(json_response, response_out);
        if(rv!=0) {
            LOG_ERROR("parseVcsMakeDir:%d", rv);
            goto exit_with_server_response;
        }
    }

exit_with_server_response:
    if (rv != 0) {
        // TODO: log jsonBody?
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}

int vcs_post_archive_url(const VcsSession& vcsSession,
                         VPLHttp2& httpHandle,
                         const VcsDataset& dataset,
                         const std::string& path,
                         u64 compId,     
                         u64 revision,  
                         bool printLog)
{
    int rv = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }

    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(path, pathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/url/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/");
    ss << "?compId=" << compId;
    ss << "&revision=" << revision;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    ss.str("");
    ss << "{";
    // The body is intentionally empty; VCS will interpret this as meaning that the file is now available
    // via the "vcs_archive" service hosted on the archive storage device.
    ss << "}";
    std::string jsonBody = ss.str();
    if(printLog){ LOG_ALWAYS("jsonBody:%s", jsonBody.c_str()); }

    std::string httpPostResponse;
    rv = httpHandle.Post(jsonBody, httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http POST returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s,"
                  "compId:"FMTu64",revision:"FMTu64,
                  rv, dataset.id, path.c_str(), compId, revision);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}

static int parseVcsBatchPostFileMetadata_RevisionList(
        cJSON2* revisionList,
        VcsBatchFileMetadataResponse_RevisionList& revisionList_out)
{
    revisionList_out.clear();
    if(revisionList == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(revisionList->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", revisionList->type);
        return -1;
    }

    cJSON2* revisionAttrib = revisionList->child;

    while(revisionAttrib != NULL) {
        if(revisionAttrib->type==cJSON2_String){
            if(strcmp("previewUri", revisionAttrib->string)==0) {
                revisionList_out.previewUri = revisionAttrib->valuestring;
            }
        }else if(revisionAttrib->type==cJSON2_Number) {
            if(strcmp("revision", revisionAttrib->string)==0) {
                revisionList_out.revision = revisionAttrib->valueint;
            }else if(strcmp("size", revisionAttrib->string)==0) {
                revisionList_out.size = revisionAttrib->valueint;
            }else if(strcmp("lastChanged", revisionAttrib->string)==0) {
                revisionList_out.lastChangedSecResolution = VPLTime_FromSec(revisionAttrib->valueint);
            }else if(strcmp("updateDevice", revisionAttrib->string)==0) {
                revisionList_out.updateDevice = revisionAttrib->valueint;
            }
        }else if(revisionAttrib->type==cJSON2_True ||
                 revisionAttrib->type==cJSON2_False)
        {
            if(strcmp("noACS", revisionAttrib->string)==0) {
                revisionList_out.noACS = false;
                if(revisionAttrib->type==cJSON2_True) {
                    revisionList_out.noACS = true;
                }
            }
        }
        revisionAttrib = revisionAttrib->next;
    }
    return 0;
}

static int parseVcsBatchPostFileMetadata_RevisionListArray(
        cJSON2* revisionListArray,
        std::vector<VcsBatchFileMetadataResponse_RevisionList>& revisionListArray_out)
{
    revisionListArray_out.clear();

    if(revisionListArray == NULL) {
        LOG_ERROR("revisionListArray is null (not zero elements)");
        return -1;
    }

    cJSON2* currEntry = revisionListArray->child;

    while(currEntry != NULL) {
        VcsBatchFileMetadataResponse_RevisionList revision;
        int rc = parseVcsBatchPostFileMetadata_RevisionList(currEntry, /*OUT*/ revision);
        if(rc!=0) {
            LOG_ERROR("folder entry parsing:%d. Skipping and continuing.", rc);
        }else{
            revisionListArray_out.push_back(revision);
        }
        currEntry = currEntry->next;
    }

    return 0;
}

static int parseVcsBatchPostFileMetadataResponse_File(
                cJSON2* file,
                VcsBatchFileMetadataResponse_File& file_out)
{
    file_out.clear();
    if(file == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(file->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", file->type);
        return -1;
    }

    cJSON2* fileAttrib = file->child;

    while(fileAttrib != NULL) {
        if(fileAttrib->type==cJSON2_String){
            if(strcmp("name", fileAttrib->string)==0) {
                file_out.name = fileAttrib->valuestring;
            }else if(strcmp("errorMsg", fileAttrib->string)==0) {
                int tempErrorCode = 0;
                std::string errorMsg = fileAttrib->valuestring;
                if(isVcsErrorTemplate(errorMsg,
                                      /*OUT*/tempErrorCode))
                {
                    file_out.errCode = tempErrorCode;
                } else {
                    LOG_ERROR("Cannot parse errorMsg:%s", fileAttrib->valuestring);
                }
            }else if(strcmp("lastChangedNanoSecs", fileAttrib->string)==0) {
                // Bug 16586: lastChangedNanoSecs is actually in microseconds.
                file_out.lastChanged = VPLTime_FromMicrosec(
                        VPLConv_strToU64(fileAttrib->valuestring, NULL, 10));
            }else if(strcmp("createDateNanoSecs", fileAttrib->string)==0) {
                // Bug 16586: createDateNanoSecs is actually in microseconds.
                file_out.createDate = VPLTime_FromMicrosec(
                        VPLConv_strToU64(fileAttrib->valuestring, NULL, 10));
            }
        }else if(fileAttrib->type==cJSON2_Number) {
            if(strcmp("compId", fileAttrib->string)==0) {
                file_out.compId = fileAttrib->valueint;
            }else if(strcmp("numOfRevisions", fileAttrib->string)==0) {
                file_out.numOfRevisions = fileAttrib->valueint;
            }
        }else if(fileAttrib->type==cJSON2_True ||
                 fileAttrib->type==cJSON2_False)
        {
            if(strcmp("success", fileAttrib->string)==0) {
                file_out.success = false;
                if(fileAttrib->type==cJSON2_True) {
                    file_out.success = true;
                }
            }
        }else if(fileAttrib->type==cJSON2_Array) {
            if(strcmp("revisionList", fileAttrib->string)==0) {
                int rc = parseVcsBatchPostFileMetadata_RevisionListArray(
                        fileAttrib,
                        file_out.revisionList);
                if (rc != 0) {
                    LOG_ERROR("parseVcsBatchPostFileMetadata_RevisionListArray:%d, Continuing", rc);
                }
            }
        }

        fileAttrib = fileAttrib->next;
    }
    return 0;
}

static int parseVcsBatchPostFileMetadataResponse_FilesArray(
                cJSON2* filesArray,
                std::vector<VcsBatchFileMetadataResponse_File>& files_out)
{
    files_out.clear();

    if(filesArray == NULL) {
        LOG_ERROR("filesArray is null (not zero elements)");
        return -1;
    }

    cJSON2* currEntry = filesArray->child;

    while(currEntry != NULL) {
        VcsBatchFileMetadataResponse_File file;
        int rc = parseVcsBatchPostFileMetadataResponse_File(currEntry, /*OUT*/ file);
        if(rc!=0) {
            LOG_ERROR("file entry parsing:%d. Skipping and continuing.", rc);
        }else{
            files_out.push_back(file);
        }
        currEntry = currEntry->next;
    }

    return 0;
}

static int parseVcsBatchPostFileMetadataResponse_Folder(
                cJSON2* folder,
                VcsBatchFileMetadataResponse_Folder& folder_out)
{
    folder_out.clear();
    if(folder == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(folder->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", folder->type);
        return -1;
    }

    cJSON2* folderAttrib = folder->child;

    while(folderAttrib != NULL) {
        if(folderAttrib->type==cJSON2_Array){
            if(strcmp("files", folderAttrib->string)==0) {
                int rc = parseVcsBatchPostFileMetadataResponse_FilesArray(
                                            folderAttrib,
                                            folder_out.files);
                if(rc!=0) {
                    LOG_ERROR("Parsing fileArray:%d.  Continuing", rc);
                }
            }
        }else if(folderAttrib->type==cJSON2_Number) {
            if(strcmp("folderCompId", folderAttrib->string)==0) {
                folder_out.folderCompId = folderAttrib->valueint;
            }
        }
        folderAttrib = folderAttrib->next;
    }
    return 0;
}

static int parseVcsBatchPostFileMetadataResponse_FoldersArray(
                cJSON2* foldersArray,
                std::vector<VcsBatchFileMetadataResponse_Folder>& folders_out)
{
    folders_out.clear();

    if(foldersArray == NULL) {
        LOG_ERROR("foldersArray is null (not zero elements)");
        return -1;
    }

    cJSON2* currEntry = foldersArray->child;

    while(currEntry != NULL) {
        VcsBatchFileMetadataResponse_Folder folder;
        int rc = parseVcsBatchPostFileMetadataResponse_Folder(currEntry, /*OUT*/ folder);
        if(rc!=0) {
            LOG_ERROR("folder entry parsing:%d. Skipping and continuing.", rc);
        }else{
            folders_out.push_back(folder);
        }
        currEntry = currEntry->next;
    }

    return 0;
}

static int parseVcsBatchPostFileMetadataResponse(
                cJSON2* json_response,
                VcsBatchFileMetadataResponse& response_out)
{
    response_out.clear();

    if(json_response == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(json_response->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", json_response->type);
        return -1;
    }

    cJSON2* entryAttrib = json_response->child;

    while(entryAttrib != NULL) {
        if(entryAttrib->type == cJSON2_Number) {
            if(strcmp("entriesReceived", entryAttrib->string)==0) {
                response_out.entriesReceived = entryAttrib->valueint;
            }else if(strcmp("entriesFailed", entryAttrib->string)==0) {
                response_out.entriesFailed = entryAttrib->valueint;
            }
        }else if(entryAttrib->type == cJSON2_Array) {
            if(strcmp("folders", entryAttrib->string)==0) {
                int rc = parseVcsBatchPostFileMetadataResponse_FoldersArray(
                                            entryAttrib,
                                            response_out.folders);
                if(rc!=0) {
                    LOG_ERROR("Parsing folderArray:%d.  Continuing", rc);
                }
            }
        }
        entryAttrib = entryAttrib->next;
    }

    return 0;
}

int vcs_batch_post_file_metadata(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 bool printLog,
                                 const std::vector<VcsBatchFileMetadataRequest>& request,
                                 VcsBatchFileMetadataResponse& response_out)
{
    int rv = 0;
    response_out.clear();

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }

    std::string url;
    {
        std::stringstream ss;
        ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/batchfilemetadata/";
        ss << dataset.id << "/";

        url = ss.str();
    }

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    // Request format:
    // http://wiki.ctbg.acer.com/wiki/index.php/VCS_Batch_APIs#For_MMD_.26_Notes
    // { "folders" : [ { "folderCompId" : <folderCompId>,
    //                   "folderPath" : <folderPath>,
    //                   "files" :
    //                    [   // an array of files under this folder
    //                      { "name" : <fileName>,
    //                        "size" : <size>,
    //                        "updateDevice" : <updateDevice>,
    //                        "accessUrl" : <accessUrl>,
    //                        "lastChanged" : <utime>,
    //                        "createDate" : <utime>,
    //                        "lastChangedNanoSecs" : <timestamp in nanosecond>,
    //                        "createDateNanoSecs" : <timestamp in nanosecond>,
    //                        "contentHash": <contentHash>,
    //                        "uploadRevision" : <uploadRevision>,
    //                        "baseRevision" : <baseRevision>,
    //                        "compId" : <compId>
    //                      } ...
    //                    ]
    //                 } ... ]
    // }

    std::string jsonBody;
    {
        std::stringstream ss;
        ss << "{\"folders\":";  // Start JSON_REQUEST
        ss <<   "[";  // Start FOLDERS_ARRAY

        u64 prevFolderCompId = 0;
        for(std::vector<VcsBatchFileMetadataRequest>::const_iterator reqIter = request.begin();
            reqIter != request.end(); ++reqIter)
        {
            if (reqIter->folderCompId == 0) {
                FAILED_ASSERT("Invalid folderCompId(%s,%s).",
                              reqIter->folderPath.c_str(), reqIter->fileName.c_str());
            }
            std::string folderPathNoLeadingOrTrailingSlash;
            Util_trimSlashes(reqIter->folderPath, folderPathNoLeadingOrTrailingSlash);

            u64 lastChanged_SecUtimeServerEnforced;
            u64 createDate_SecUtimeServerEnforced;
            lastChanged_SecUtimeServerEnforced = VPLTime_ToSec(reqIter->lastChanged);
            createDate_SecUtimeServerEnforced = VPLTime_ToSec(reqIter->createDate);
            if (lastChanged_SecUtimeServerEnforced == 0 ||
                createDate_SecUtimeServerEnforced == 0)
            {
                FAILED_ASSERT("(%s,%s) Times cannot be 0,:"
                              "("FMTu64"->"FMTu64"),("FMTu64"->"FMTu64")",
                              reqIter->folderPath.c_str(), reqIter->fileName.c_str(),
                              reqIter->lastChanged,
                              lastChanged_SecUtimeServerEnforced,
                              reqIter->createDate,
                              createDate_SecUtimeServerEnforced);
            }

            if (prevFolderCompId != reqIter->folderCompId) {
                if (prevFolderCompId != 0)
                {   // Close the previous entry
                    ss <<   "]";    // End FILES_ARRAY
                    ss << "},";     // End FOLDER_OBJECT
                }
                // Start new folder object
                ss << "{";  // Start FOLDER_OBJECT
                ss <<   "\"folderCompId\":" << reqIter->folderCompId;
                ss <<   ",\"folderPath\":\""   << folderPathNoLeadingOrTrailingSlash.c_str() << "\"";
                ss <<   ",\"files\":[";  // Start FILES_ARRAY
            } else {
                ss <<     ",";
            }
            prevFolderCompId = reqIter->folderCompId;

            // start file object within file array
            ss << "{";  // Start FILE_OBJECT
            ss <<   "\"name\":\"" << reqIter->fileName.c_str() << "\"";
            ss <<   ",\"size\":" << reqIter->size;
            ss <<   ",\"updateDevice\":" << reqIter->updateDevice;
            if (reqIter->hasAccessUrl) {
                ss <<   ",\"accessUrl\":\"" << reqIter->accessUrl.c_str() << "\"";
            }

            ss << ",\"lastChanged\":" << lastChanged_SecUtimeServerEnforced;
            ss << ",\"createDate\":" << createDate_SecUtimeServerEnforced;
            // Bug 16586: lastChangedNanoSecs is actually in microseconds.
            ss << ",\"lastChangedNanoSecs\":" << VPLTime_ToMicrosec(reqIter->lastChanged);
            // Bug 16586: createDateNanoSecs is actually in microseconds.
            ss << ",\"createDateNanoSecs\":" << VPLTime_ToMicrosec(reqIter->createDate);

            if (reqIter->hasContentHash) {
                ss <<   ",\"contentHash\":\"" << reqIter->contentHash.c_str() << "\"";
            }
            ss << ",\"uploadRevision\":" << reqIter->uploadRevision;
            if (reqIter->hasBaseRevision) {
                ss << ",\"baseRevision\":" << reqIter->baseRevision;
            }
            if (reqIter->hasCompId) {
                ss << ",\"compId\":" << reqIter->compId;
            }
            ss << "}";  // End FILE_OBJECT
        }

        if (prevFolderCompId != 0)
        {   // Check that an entry was written out
            ss <<   "]";    // End FILES_ARRAY
            ss << "}";     // End FOLDER_OBJECT
        }

        ss <<   "]";  // End FOLDERS_ARRAY
        ss << "}";    // End JSON_REQUEST

        jsonBody = ss.str();
    }
    if(printLog){ LOG_ALWAYS("jsonBody:%s", jsonBody.c_str()); }

    std::string httpPostResponse;
    rv = httpHandle.Post(jsonBody, httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http POST returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64,
                  rv, dataset.id);
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpPostResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpPostResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsBatchPostFileMetadataResponse(json_response,
                                                       /*OUT*/ response_out);
        if (rv != 0) {
            LOG_ERROR("parseVcsBatchPostFileMetadata:%d",
                      rv);
            goto exit_with_server_response;
        }
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpPostResponse.c_str());
    }
    return rv;
}

int vcs_get_file_metadata(const VcsSession& vcsSession,
                          VPLHttp2& httpHandle,
                          const VcsDataset& dataset,
                          const std::string& filepath,
                          u64 compId,
                          bool printLog,
                          VcsFileMetadataResponse& response_out)
{
    int rv = 0;
    response_out.clear();

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(filepath, pathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/filemetadata/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/");
    ss << "?compId=" << compId;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    std::string httpGetResponse;
    rv = httpHandle.Get(/*out*/httpGetResponse);
    if(rv != 0) {
        LOG_ERROR("http GET returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s",
                  rv, dataset.id, filepath.c_str());
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpGetResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

    {
        cJSON2* json_response = cJSON2_Parse(httpGetResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpGetResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsFileMetadataResponse(json_response, response_out);
        if(rv!=0) {
            LOG_ERROR("parseVcsGetCompId:%d", rv);
            goto exit_with_server_response;
        }
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, url.c_str(), httpGetResponse.c_str());
    }
    return rv;
}

int vcs_post_acs_access_url_virtual(const VcsSession& vcsSession,
                                    VPLHttp2& httpHandle,
                                    const VcsDataset& dataset,
                                    const std::string& path,
                                    u64 compId,
                                    u64 revisionId,
                                    const std::string& acs_access_url,
                                    bool printLog)
{
    int rv = 0;

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }
    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(path, pathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/" << dataset.category.str << "/url/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/");
    ss <<"?compId=" << compId;
    ss <<"&revision=" << revisionId;
    std::string url = ss.str();

    rv = httpHandle.SetUri(url);
    if(rv != 0) {
        LOG_ERROR("SetUri:%d, %s", rv, url.c_str());
        return rv;
    }

    rv = addSessionHeaders(vcsSession, httpHandle);
    if(rv != 0) {
        LOG_ERROR("Error adding session headers:%d", rv);
        return rv;
    }

    ss.str("");
    ss << "{";
    ss <<   "\"accessUrl\":\""  << acs_access_url <<"\"";  // string-type
    ss << "}";
    std::string body = ss.str();

    std::string httpPostResponse;
    rv = httpHandle.Post(body, /*out*/httpPostResponse);
    if(rv != 0) {
        LOG_ERROR("http POST returned:%d", rv);
        return rv;
    }

    // Http error code, should be 200 unless server can't be reached.
    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("vcs_translate_http_status_code:%d, dset:"FMTu64",path:%s",
                  rv, dataset.id, path.c_str());
        goto exit_with_server_response;
    }

    // Parse VCS error code, if any.
    if(isVcsErrorTemplate(httpPostResponse.c_str(), rv)) {
        LOG_WARN("VCS_ERRCODE(%d)", rv);
        goto exit_with_server_response;
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", body=\"%s\", response: %s",
                rv, url.c_str(), body.c_str(), httpPostResponse.c_str());
    }
    return rv;
}
