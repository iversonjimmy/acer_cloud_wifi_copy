//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "vcs_v1_util.hpp"

#include "vcs_common.hpp"
#include "vcs_common_priv.hpp"

#include "cJSON2.h"
#include "gvm_file_utils.hpp"
#include "scopeguard.hpp"
#include "vpl_conv.h"
#include "vplex_http2.hpp"
#include "vplex_http_util.hpp"

#include <string>
#include <sstream>
#include <vector>

#include "log.h"

static int addSessionHeaders(const VcsSession& vcsSession,
                             VPLHttp2& httpHandle)
{   // See http://wiki.ctbg.acer.com/wiki/index.php/VCS_Design#Authentication
    int rc;
    // Vcs API at version 1
    rc = addSessionHeadersHelper(vcsSession,
                                 httpHandle,
                                 1);  // VCS_API_VERSION
    if (rc != 0) {
        LOG_ERROR("addSessionHeadersHelper:%d", rc);
    }
    return rc;
}

int VcsV1_getAccessInfo(const VcsSession& vcsSession,
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
    ss << vcsSession.urlPrefix << "/vcs/accessinfo/";
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
        LOG_ERROR("translateHttpStatusCode:%d, dset:"FMTu64,
                  rv, dataset.id);
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

static int parseRevision(cJSON2* fileEntryObject,
                         VcsV1_revisionListEntry& fileLatestRevision)
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
            }
        }else if(revisionAttrib->type==cJSON2_Number) {
            if(strcmp("revision", revisionAttrib->string)==0) {
                fileLatestRevision.revision = revisionAttrib->valueint;
            }else if(strcmp("size", revisionAttrib->string)==0) {
                fileLatestRevision.size = revisionAttrib->valueint;
            }else if(strcmp("lastChanged", revisionAttrib->string)==0) {
                fileLatestRevision.lastChanged = VPLTime_FromSec(revisionAttrib->valueint);
            }else if(strcmp("updateDevice", revisionAttrib->string)==0) {
                fileLatestRevision.updateDevice = revisionAttrib->valueint;
            }
        }
        revisionAttrib = revisionAttrib->next;
    }
    return 0;
}

static int parseRevisionListArray(cJSON2* fileListArray,
                                  std::vector<VcsV1_revisionListEntry>& revisionList)
{
    revisionList.clear();

    if(fileListArray == NULL) {
        LOG_ERROR("fileListArray is null (not zero elements)");
        return -1;
    }
    cJSON2* currEntry = fileListArray->child;

    while(currEntry != NULL) {
        VcsV1_revisionListEntry revision_out;

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

static int parseVcsPostFileMetadataResponse(cJSON2* fileEntryObject,
                                        VcsV1_postFileMetadataResponse& file)
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

int VcsV1_share_postFileMetadata(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 const std::string& path,
                                 VPLTime_t lastChanged,
                                 VPLTime_t createDate,
                                 u64 size,
                                 bool hasOpaqueMetadata,
                                 const std::string& opaqueMetadata,
                                 bool hasContentHash,
                                 const std::string& contentHash,
                                 u64 updateDevice,
                                 const std::string& accessUrl,
                                 bool printLog,
                                 VcsV1_postFileMetadataResponse& response_out)
{   // http://wiki.ctbg.acer.com/wiki/index.php/VCS_Design#Share_by_Me_API
    int rv = 0;
    int rc;
    std::string httpPostResponse;
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

    rc = httpHandle.SetDebug(printLog);
    if(rc != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rc);
    }
    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(path, pathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/filemetadata/";
    ss << dataset.id << "/"
       << updateDevice << "/"
       << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/");
    std::string url = ss.str();

    ss.str("");
    ss << "{"
       <<     "\"lastChanged\":" << lastChanged_SecUtimeServerEnforced
       <<     ",\"createDate\":" << createDate_SecUtimeServerEnforced
       <<     ",\"size\":" << size;
    if (hasOpaqueMetadata) {
        ss << ",\"opaqueMetadata\":" << "\"" << escapeJsonString(opaqueMetadata) << "\"";
    }
    if (hasContentHash) {
        ss << ",\"contentHash\":" << "\"" << escapeJsonString(contentHash) << "\"";
    }
    ss <<     ",\"updateDevice\":" << updateDevice
       <<     ",\"accessUrl\":" << "\"" << accessUrl << "\""
       << "}";

    std::string jsonBody = ss.str();

    rc = httpHandle.SetUri(url);
    if(rc != 0) {
        LOG_ERROR("SetUri:%d, %s", rc, url.c_str());
        return rc;
    }

    rc = addSessionHeaders(vcsSession, httpHandle);
    if(rc != 0) {
        LOG_ERROR("Error adding session headers:%d", rc);
        return rc;
    }

    if(printLog){
        LOG_ALWAYS("Url:%s", url.c_str());
        LOG_ALWAYS("Body:%s", jsonBody.c_str());
    }

    rc = httpHandle.Post(jsonBody, httpPostResponse);
    if(rc != 0) {
        LOG_ERROR("http POST returned:%d", rc);
        return rc;
    }

    if(printLog){ LOG_ALWAYS("DEBUG data:%s", httpPostResponse.c_str()); }

    // Http error code, should be 200 unless server can't be reached.
    rc = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rc != 0) {
        LOG_ERROR("translateHttpStatusCode:%d, dset:"FMTu64",path:%s",
                  rc, dataset.id, path.c_str());
        rv = rc;
    }
    if(rv != 0) {
        return rv;
    }

    do{
        cJSON2* json_response = cJSON2_Parse(httpPostResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("cJSON2_Parse error!%s", httpPostResponse.c_str());
            rv = -1;
            break;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        int rc = parseVcsPostFileMetadataResponse(json_response, response_out);
        if(rc!=0) {
            LOG_ERROR("parseVcsMakeDirResponse:%d, %s", rc, httpPostResponse.c_str());
            rv = rc;
            break;
        }
    }while(false);

    return rv;
}

static int parseGetRevision(cJSON2* fileEntryObject,
                            VcsV1_getRevisionListEntry& fileLatestRevision)
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
                fileLatestRevision.lastChanged = VPLTime_FromSec(revisionAttrib->valueint);
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

static int parseRecipientList(cJSON2* recipientListArray,
                              std::vector<std::string>& recipientList)
{
    recipientList.clear();
    if(recipientListArray == NULL) {
        LOG_ERROR("reciepintListArray is null (not zero elements)");
        return -1;
    }
    cJSON2* currEntry = recipientListArray->child;

    while(currEntry != NULL) {
        if(currEntry->type==cJSON2_String) {
            std::string recipient;
            recipient = currEntry->valuestring;
            recipientList.push_back(recipient);
        }
        currEntry = currEntry->next;
    }
    return 0;
}

static int parseGetRevisionListArray(cJSON2* fileListArray,
                                     std::vector<VcsV1_getRevisionListEntry>& revisionList)
{
    revisionList.clear();

    if(fileListArray == NULL) {
        LOG_ERROR("fileListArray is null (not zero elements)");
        return -1;
    }
    cJSON2* currEntry = fileListArray->child;

    while(currEntry != NULL) {
        VcsV1_getRevisionListEntry revision_out;

        int rc = parseGetRevision(currEntry, revision_out);
        if(rc!=0) {
            LOG_ERROR("Revision entry parsing:%d Skipping and continuing.", rc);
        }else{
            revisionList.push_back(revision_out);
        }
        currEntry = currEntry->next;
    }

    return 0;
}

static int parseVcsGetFileMetadataResponse(cJSON2* fileEntryObject,
                                           VcsV1_getFileMetadataResponse& file)
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
                file.lastChangedNano = VPLTime_FromMicrosec(
                        VPLConv_strToU64(entryAttrib->valuestring, NULL, 10));
            }else if(strcmp("createDateNanoSecs", entryAttrib->string)==0) {
                // Bug 16586: createDateNanoSecs is actually in microseconds.
                file.createDateNano = VPLTime_FromMicrosec(
                        VPLConv_strToU64(entryAttrib->valuestring, NULL, 10));
            }else if(strcmp("opaqueMetadata", entryAttrib->string)==0) {
                file.opaqueMetadata = entryAttrib->valuestring;
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
                int rc = parseGetRevisionListArray(entryAttrib, file.revisionList);
                if(rc!=0) {
                    LOG_ERROR("Parsing revision list:%d.  Continuing", rc);
                }
            } else if(strcmp("recipientList", entryAttrib->string)==0) {
                int rc = parseRecipientList(entryAttrib, file.recipientList);
                if(rc!=0) {
                    LOG_ERROR("parseRecipientList:%d.  Continuing", rc);
                }
            }
        }
        entryAttrib = entryAttrib->next;
    }

    return 0;
}

int VcsV1_getFileMetadata(const VcsSession& vcsSession,
                                VPLHttp2& httpHandle,
                                const VcsDataset& dataset,
                                const std::string& filepath,
                                u64 compId,
                                bool printLog,
                                VcsV1_getFileMetadataResponse& response_out)
{   // http://wiki.ctbg.acer.com/wiki/index.php/Photo_Sharing_Design#SWM_GET_filemetadata
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
    ss << vcsSession.urlPrefix << "/vcs/filemetadata/";
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

    {
        cJSON2* json_response = cJSON2_Parse(httpGetResponse.c_str());
        if (json_response == NULL) {
            LOG_ERROR("Failed cJSON2_Parse(%s)", httpGetResponse.c_str());
            rv = -1;
            goto exit_with_server_response;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        rv = parseVcsGetFileMetadataResponse(json_response, response_out);
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

int VcsV1_putPreview(const VcsSession& vcsSession,
                     VPLHttp2& httpHandle,
                     const VcsDataset& dataset,
                     const std::string& path,
                     u64 compId,
                     u64 revisionId, // For PicStream, the revision should be always 1
                     const std::string& destLocalFilepath,
                     bool printLog)
{
    int rv = 0;
    int rc;
    std::string httpPostResponse;

    rc = httpHandle.SetDebug(printLog);
    if(rc != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rc);
    }
    std::string pathNoLeadingOrTrailingSlash;
    Util_trimSlashes(path, pathNoLeadingOrTrailingSlash);

    std::stringstream ss;
    ss.str("");
    ss << vcsSession.urlPrefix << "/vcs/preview/";
    ss << dataset.id << "/"
       << VPLHttp_UrlEncoding(pathNoLeadingOrTrailingSlash, "/")
       << "?compId=" << compId
       << "&revision=" << revisionId;
    std::string url = ss.str();

    rc = httpHandle.SetUri(url);
    if(rc != 0) {
        LOG_ERROR("SetUri:%d, %s", rc, url.c_str());
        return rc;
    }

    rc = addSessionHeaders(vcsSession, httpHandle);
    if(rc != 0) {
        LOG_ERROR("Error adding session headers:%d", rc);
        return rc;
    }

    if(printLog){ LOG_ALWAYS("Url:%s", url.c_str()); }

    rc = httpHandle.Put(destLocalFilepath, NULL, NULL, httpPostResponse);
    if(rc != 0) {
        LOG_ERROR("http POST returned:%d", rc);
        return rc;
    }

    // Http error code, should be 200 unless server can't be reached.
    rc = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rc != 0) {
        LOG_ERROR("translateHttpStatusCode:%d, dset:"FMTu64",path:%s, status:%d",
                  rc, dataset.id, path.c_str(), httpHandle.GetStatusCode());
        rv = rc;
    }

    return rv;
}

