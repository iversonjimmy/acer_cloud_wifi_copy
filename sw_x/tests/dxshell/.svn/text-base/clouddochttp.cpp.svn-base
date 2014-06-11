//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.

#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv

#include <ccdi.hpp>
#include <vplex_file.h>
#include <vplex_strings.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vpl_conv.h>
#include "gvm_file_utils.hpp"
#include "gvm_misc_utils.h"

#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <fstream>
#include <cstdio>

#include <log.h>
#include <vpl_plat.h>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "clouddochttp.hpp"
#include "EventQueue.hpp"
#include "HttpAgent.hpp"
#include "TargetLocalDevice.hpp"
#include "cJSON2.h"
#include "scopeguard.hpp"

#define RF_HEADER_USERID         "x-ac-userId"
#define RF_HEADER_SESSION_HANDLE "x-ac-sessionHandle"
#define RF_HEADER_SERVICE_TICKET "x-ac-serviceTicket"

int clouddochttp_help(int argc, const char* argv[]);
int clouddochttp_help(int argc, const char* argv[])
{
    bool printAll = strcmp(argv[0], "CloudDocHttp") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll || strcmp(argv[0], "UploadBasic") == 0)
        std::cout << "CloudDocHttp UploadBasic Doc_abs_path [Doc Preview (abs path)]" << std::endl;
    if (printAll || strcmp(argv[0], "UploadRevision") == 0)
        std::cout << "CloudDocHttp UploadRevision Clouddoc_Name Doc_abs_path [Doc Preview (abs path)] Component_ID Doc_Base_Revision" << std::endl;
    if (printAll || strcmp(argv[0], "ListReq") == 0)
        std::cout << "CloudDocHttp ListReq" << std::endl;
    if (printAll || strcmp(argv[0], "GetProgress") == 0)
        std::cout << "CloudDocHttp GetProgress [Req_Handle]" << std::endl;
    if (printAll || strcmp(argv[0], "CancelReq") == 0)
        std::cout << "CloudDocHttp CancelReq [Req_Handle]" << std::endl;
    if (printAll || strcmp(argv[0], "ListDocs") == 0){
        std::cout << "CloudDocHttp ListDocs ['option[;option]...']" << std::endl
                  << " - Pagination parameters are optional and are given w/o spaces and quoted by '' " << std::endl
                  << "   > sortBy=<time | size | alpha>"  << std::endl
                  << "   > index=<value_start_from_1>"    << std::endl
                  << "   > max=<maximum_return_entries>"  << std::endl
                  << " - Ex. 'sortBy=time;index=1;max=5'" << std::endl;
    }
    if (printAll || strcmp(argv[0], "GetMetadata") == 0)
        std::cout << "CloudDocHttp GetMetadata Clouddoc_Name Component_ID [Revision]" << std::endl;
    if (printAll || strcmp(argv[0], "Download") == 0)
        std::cout << "CloudDocHttp Download Clouddoc_Name Component_ID Revision Out_File_abs_path [Range Header]" << std::endl;
    if (printAll || strcmp(argv[0], "DownloadPreview") == 0)
        std::cout << "CloudDocHttp DownloadPreview [Clouddoc Name] [Component ID] [Revision] [Out File (abs path)]" << std::endl;
    if (printAll || strcmp(argv[0], "Delete") == 0)
        std::cout << "CloudDocHttp Delete Clouddoc_Name Component_ID [Revision]" << std::endl;
    if (printAll || strcmp(argv[0], "Move") == 0)
        std::cout << "CloudDocHttp Move Clouddoc_Name Component_ID [Revision] New_Doc_Name_abs_path" << std::endl;
    if (printAll || strcmp(argv[0], "DeleteAsync") == 0)
        std::cout << "CloudDocHttp DeleteAsync Clouddoc_Name Component_ID [Revision]" << std::endl;
    if (printAll || strcmp(argv[0], "MoveAsync") == 0)
        std::cout << "CloudDocHttp MoveAsync Clouddoc_Name Component_ID [Revision] New_Doc_Name_abs_path" << std::endl;
    if (printAll || strcmp(argv[0], "CheckConflict") == 0)
        std::cout << "CloudDocHttp CheckConflict Clouddoc_Name Component_ID" << std::endl;
    if (printAll || strcmp(argv[0], "CheckCopyBack") == 0)
        std::cout << "CloudDocHttp CheckCopyBack Clouddoc_Name Component_ID" << std::endl;
    if (printAll || strcmp(argv[0], "DatasetChanges") == 0)
        std::cout << "CloudDocHttp DatasetChanges Dataset_ID ['option[;option]...']" << std::endl
                  << " - Pagination parameters are optional and are given w/o spaces and quoted by '' " << std::endl
                  << "   > changeSince=<version>"         << std::endl
                  << "   > max=<maximum_return_entries>"  << std::endl
                  << " - Ex. 'changeSince=3;max=5'"       << std::endl;

    return 0;
}

static int clouddochttp_help_with_response(int argc, const char* argv[], std::string &response) {
    return clouddochttp_help(argc, argv);
}

static void parse_arg(const std::string &input, std::map<std::string, std::string>& ret)
{
    ret.clear();

    if (input.empty())
        return;

    // strip spaces?
    size_t start = 0;
    size_t end = input.find_first_of(";", start);
    while (end != std::string::npos) {
        size_t end_key = input.find_first_of("=", start);
        if (end_key != std::string::npos && end_key != end
                && (end_key-start) > 0 && (end-end_key-1) > 0) {
            ret[input.substr(start, end_key-start)] = input.substr(end_key+1, end-end_key-1);
        }
        start = end + 1;
        end = input.find_first_of(";", start);
    }

    end = input.size();
    if (end-start > 0) {
        size_t end_key = input.find_first_of("=", start);
        if (end_key != std::string::npos
                && (end_key-start) > 0 && (end-end_key-1) > 0) {
            ret[input.substr(start, end_key-start)] = input.substr(end_key+1, end-end_key-1);
        }
    }
}

static int check_json_format(std::string& json, const std::string& attributeName = std::string()) {
    cJSON2 *jsonResponse = cJSON2_Parse(json.c_str());
    if (!jsonResponse) {
        LOG_ERROR("Invalid root json data.");
        return -1;
    }

    cJSON2 *jsonAttribute = NULL;

    if (attributeName != "") {
        int ret = JSON_getJSONObject(jsonResponse, attributeName.c_str(), &jsonAttribute);

        if(ret != 0){
            LOG_ERROR("Can not find %s!", attributeName.c_str());
            cJSON2_Delete(jsonResponse);
            return -1;
        }
    }


    cJSON2_Delete(jsonResponse);
    jsonResponse = NULL;
    return 0;
}

std::string urlEncodingLoose(const std::string &sIn)
{
    std::string sOut;
    for(u16 ix = 0; ix < sIn.size(); ix++) {
        u8 buf[4];
        memset( buf, 0, 4 );
        if (isalnum( (u8)sIn[ix]) ) {
            buf[0] = sIn[ix];
        } else if ( (u8)sIn[ix] == '/' || (u8)sIn[ix] == '\\' ) {
            buf[0] = sIn[ix];
        } else {
            buf[0] = '%';
            buf[1] = toHex( (u8)sIn[ix] >> 4 );
            buf[2] = toHex( (u8)sIn[ix] % 16);
        }
        sOut += (char *)buf;
    }
    return sOut;
}

static bool inline isWindows_ForDocs(const std::string &os)
{
    if (os.find(OS_WINDOWS) != std::string::npos ||
        os.find(OS_WINDOWS_RT) != std::string::npos) {
            return true;
    }
    return false;
}

static int clouddochttp_upload_basic(int argc, const char* argv[], std::string& response)
{
    if (argc < 2) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(argc == 2)
        return clouddochttp_upload(userId, "", argv[1], "", "", 0, response);

    return clouddochttp_upload(userId, "", argv[1], argv[2], "", 0, response);
}

static int clouddochttp_upload_revision(int argc, const char* argv[], std::string& response)
{
    if (argc < 5) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }
    
    if(argc == 5)
        return clouddochttp_upload(userId, argv[1], argv[2], "", argv[3], atoi(argv[4]), response);

    return clouddochttp_upload(userId, argv[1], argv[2], argv[3], argv[4], atoi(argv[5]), response);
}

static int clouddochttp_list_requests(int argc, const char* argv[], std::string& response)
{
    if (argc < 1) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return clouddochttp_list_requests(userId, response);
}

static int clouddochttp_get_progress(int argc, const char* argv[], std::string& response)
{
    if (argc < 2) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return clouddochttp_get_progress(userId, argv[1], response);
}

static int clouddochttp_cancel_request(int argc, const char* argv[], std::string& response)
{
    if (argc < 2) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return clouddochttp_cancel_request(userId, argv[1], response);
}

static int clouddochttp_list_docs(int argc, const char* argv[], std::string& response)
{
    if (argc < 1) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(argc == 1)
        return clouddochttp_list_docs(userId, "", response);

    return clouddochttp_list_docs(userId, argv[1], response);
}

static int clouddochttp_get_metadata(int argc, const char* argv[], std::string& response)
{
    if (argc < 3) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(!argv[3]) {
        return clouddochttp_get_metadata(userId, argv[1], argv[2], "", response);
    }else{
        return clouddochttp_get_metadata(userId, argv[1], argv[2], argv[3], response);
    }
}

static int clouddochttp_download(int argc, const char* argv[], std::string& response)
{
    if (argc < 5) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(!argv[5]) {
        return clouddochttp_download(userId, argv[1], argv[2], argv[3], argv[4], "/", response);
    } else {
        return clouddochttp_download(userId, argv[1], argv[2], argv[3], argv[4], argv[5], response);
    }
}

static int clouddochttp_download_preview(int argc, const char* argv[], std::string& response)
{
    if (argc < 5) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return clouddochttp_download_preview(userId, argv[1], argv[2], argv[3], argv[4], response);
}

static int clouddochttp_delete(int argc, const char* argv[], std::string& response)
{
    if (argc < 3) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(!argv[3]) {
        return clouddochttp_delete(userId, argv[1], argv[2], "", response);
    }else{
        return clouddochttp_delete(userId, argv[1], argv[2], argv[3], response);
    }
}

static int clouddochttp_delete_async(int argc, const char* argv[], std::string& response)
{
    if (argc < 3) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(!argv[3]) {
        return clouddochttp_delete_async(userId, argv[1], argv[2], "", response);
    }else{
        return clouddochttp_delete_async(userId, argv[1], argv[2], argv[3], response);
    }
}

static int clouddochttp_move(int argc, const char* argv[], std::string& response)
{
    if (argc < 4) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(!argv[4]) {
        return clouddochttp_move(userId, argv[1], argv[2], "", argv[3], response);
    }else{
        return clouddochttp_move(userId, argv[1], argv[2], argv[3], argv[4], response);
    }
}

static int clouddochttp_move_async(int argc, const char* argv[], std::string& response)
{
    if (argc < 4) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(!argv[4]) {
        return clouddochttp_move_async(userId, argv[1], argv[2], "", argv[3], response);
    }else{
        return clouddochttp_move_async(userId, argv[1], argv[2], argv[3], argv[4], response);
    }
}

static int clouddochttp_dataset_changes(int argc, const char* argv[], std::string& response)
{
    if (argc < 2) {
        clouddochttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(argc == 2)
        return clouddochttp_dataset_changes(userId, argv[1], "", response);

    return clouddochttp_dataset_changes(userId, argv[1], argv[2], response);
}

static int clouddochttp_check_conflict(int argc, const char* argv[], std::string& response)
{
    if (argc != 3) {
        clouddochttp_help(argc, argv);
        LOG_ERROR("argc %d, argv[0] = \"%s\"", argc, argv[0]);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return clouddochttp_check_conflict(userId, argv[1], argv[2], response);
}

static int clouddochttp_check_copyback(int argc, const char* argv[], std::string& response)
{
    if (argc != 3) {
        clouddochttp_help(argc, argv);
        LOG_ERROR("argc %d, argv[0] = \"%s\"", argc, argv[0]);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return clouddochttp_check_copyback(userId, argv[1], argv[2], response);
}


///////////////////////////// End of Functions for Command Dispatcher /////////////////////////////////////

int clouddochttp_upload(u64 userId,
    const std::string &docname,
    const std::string &doc,
    const std::string &preview,
    const std::string &component_id,
    const int base_revision,
    std::string& response)
{
#ifndef WIN32
    char *dir = get_current_dir_name();

    LOG_INFO("The path is %s", dir);
    //std::cout << "The path is" << dir << std::endl;
#endif

    u64 deviceId = 0;
    getDeviceId(&deviceId);
    std::cout << "The deviceid is " <<  deviceId << std::endl;

    int rv = 0;

    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    //std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;
    TargetDevice *target = NULL;

    if((base_revision == 0) ^ (component_id == "")){
        LOG_ERROR("either base_revision or component_id required!");
        rv = -1;
        goto end;
    }

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("failed to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        //// Clean up the path 
        std::string doc2 = doc;         // The fullpath replacing all backslash with forward slash
        std::string preview2 = preview; // The fullpath replacing all backslash with forward slash 
        // Replace backslash with forward slash
        std::replace(doc2.begin(), doc2.end(), '\\', '/');

        if(preview != ""){
            std::replace(preview2.begin(), preview2.end(), '\\', '/');
        }

        // check paths are absolute
        target = getTargetDevice();
        if (isWindows_ForDocs(target->getOsVersion())) {
            if (doc2.length() < 3 || doc2[1] != ':' || doc2[2] != '/'){
                LOG_ERROR("bad path");
                rv = -1;
                goto end;
            }
            if((preview != "") && (preview2.length() < 3 || preview2[1] != ':' || preview2[2] != '/')){
                LOG_ERROR("bad path");
                rv = -1;
                goto end;
            }

        }else{
            if (doc2.empty() || doc2[0] != '/'){
                LOG_ERROR("bad path: %s", doc2.c_str());
                rv = -1;
                goto end;
            }
            if((preview != "") && (preview2.empty() || preview2[0] != '/')){
                LOG_ERROR("bad path");
                rv = -1;
                goto end;
            }
        }

        std::string doc3 = doc2;        // for docname
        if (isWindows_ForDocs(target->getOsVersion())) {
            // remove colon after drive letter
            // e.g., C:/Users/fokushi/test.docx -> C/Users/fokushi/test.docx
            doc3.erase(1, 1);
        }else{
            // strip initial slash
            // e.g., /home/fokushi/test.txt -> home/fokushi/test.txt
            doc3.erase(0, 1);
        }

        std::string payload;
        std::stringstream ss;
        ss << "{";

        if(docname != ""){
           // This is UploadRevision, the input should be docname
           ss << "\"name\":\"" << urlEncodingLoose(docname) << "\",";
        }else{
           ss << "\"name\":\"" << deviceId << "/" << urlEncodingLoose(doc3) << "\",";
        }          
 
        ss << "\"contentPath\":\"" << doc2    << "\","
           << "\"lastChanged\":"   << time(0) ;

        if(preview != "") {
            ss << ",\"previewPath\":\"" << preview2 << "\"";
        }
        if(component_id != "") {
            ss << ",\"compId\":" << component_id;
        }
        if (base_revision != 0) {
            ss << ",\"baseRevision\":" << base_revision;
        }
        ss << "}";
        
        payload = ss.str();
        std::cout << "payload: " << payload << std::endl;

        url = base_url + "/clouddoc/async";

        rv = agent->post(url, headers, 3, payload.c_str(), payload.size(), NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Upload pass. response: %s", response.c_str());

    // TODO
    // On success, response body is empty.
    // Get requestid from Location header in response.

end:
    if(target != NULL){
        delete target;
        target = NULL;
    }
    return rv;
}


int clouddochttp_list_requests(u64 userId, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/clouddoc/async";

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("List requests pass. response: %s", response.c_str());

    rv = check_json_format(response);
    if(rv == 0) rv = check_json_format(response, "requestList");
    if(rv == 0) rv = check_json_format(response, "numOfRequests");

end:
    return rv;
}

int clouddochttp_get_progress(u64 userId,
    const std::string &requesthandle,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/clouddoc/async/" + requesthandle;

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Get progress pass. response: %s", response.c_str());

    rv = check_json_format(response);
    if(rv == 0) rv = check_json_format(response, "id");
    if(rv == 0) rv = check_json_format(response, "status");
    if(rv == 0) rv = check_json_format(response, "totalSize");
    if(rv == 0) rv = check_json_format(response, "xferedSize");

end:
    return rv;
}

int clouddochttp_cancel_request(u64 userId,
    const std::string &requesthandle,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/clouddoc/async/" + requesthandle;

        rv = agent->del(url, headers, 3, NULL, 0, NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Cancel request pass. response: %s", response.c_str());

    //rv = check_json_format(response);

end:
    return rv;
}



int clouddochttp_list_docs(u64 userId, const std::string &pagination, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::map<std::string, std::string> parameters;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    // parsed and split the pagination parameters
    parse_arg(pagination, parameters);


    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        if(pagination == ""){
            url = base_url + "/clouddoc/dir";
        }else{
            url = base_url + "/clouddoc/dir/" + "?" + createParameter(parameters);
        }

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }

    LOG_ALWAYS("List docs pass. response: %s", response.c_str());

    rv = check_json_format(response);
    if(rv == 0) rv = check_json_format(response, "currentDirVersion");
    if(rv == 0) rv = check_json_format(response, "numOfFiles");

end:
    return rv;
}

int clouddochttp_get_metadata(u64 userId,
    const std::string &docname,
    const std::string &component_id,
    const std::string &revision,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/clouddoc/filemetadata/" + urlEncodingLoose(docname) +"?compId="+component_id;

        if(revision != "")
            url += ("&revision="+revision);

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Get metadata pass. response: %s", response.c_str());

    rv = check_json_format(response);
    if(rv == 0) rv = check_json_format(response, "name");
    if(rv == 0) rv = check_json_format(response, "numOfRevisions");
    if(rv == 0) rv = check_json_format(response, "originDevice");
    if(rv == 0) rv = check_json_format(response, "revisionList");

end:
    return rv;
}

int clouddochttp_download(u64 userId,
    const std::string &docname,
    const std::string &component_id,
    const std::string &revision,
    const std::string &outfilename,
    const std::string &rangeHeader,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::stringstream ss;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        size_t nr_headers = 3;
        if(!rangeHeader.empty() && rangeHeader != "/") {
            ss.str("");
            ss << "Range: bytes=";
            ss << rangeHeader;
            temp = ss.str();
            nr_headers++;
        }
        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
            temp.c_str(),
        };

        url = base_url + "/clouddoc/file/" + urlEncodingLoose(docname) + "?compId="+component_id+"&revision="+revision;

        rv = agent->get_extended(url, 0, 0, headers, nr_headers, outfilename.c_str());

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_extended(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Download pass.");

end:
    return rv;
}

int clouddochttp_download_preview(u64 userId,
    const std::string &docname,
    const std::string &component_id,
    const std::string &revision,
    const std::string &outfilename,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;
    u64 datasetId = 0;
    

    //Get previewUri
    std::string previewUri;
    {
        std::string metadata;
        clouddochttp_get_metadata(userId, docname, component_id, revision, metadata);

        cJSON2* json_response = cJSON2_Parse(metadata.c_str());
        if (json_response==NULL) {
            LOG_ERROR("cJSON2_Parse error!");
            return -1;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json_response);

        cJSON2* revisionlist=NULL;
        if (JSON_getJSONObject(json_response, "revisionList", &revisionlist)) {
            LOG_ERROR("Get revisionList error!");
            return -1;
        }

        cJSON2* revisionlist1=NULL;
        revisionlist1 = cJSON2_GetArrayItem(revisionlist, 0);

        if(JSON_getString(revisionlist1, "previewUri", previewUri)){
            LOG_ERROR("Get previewUri error!");
            return -1;
        }

    }

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    rv = getDatasetId(userId, "Cloud Doc", datasetId);
    if (rv != 0) {
        LOG_ERROR("Fail to get dataset id");
        return -1;
    }


    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        std::string datasetIdStr;
        std::stringstream ss;
        ss << datasetId ;
        datasetIdStr = ss.str();

        url = base_url + previewUri;

        rv = agent->get_extended(url, 0, 0, headers, 3, outfilename.c_str());
        // FIXME:
        // Response is preview image data.
        // Direct to a local file.

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_extended(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }

end:
    return rv;
}


int clouddochttp_delete(u64 userId,
    const std::string &docname,
    const std::string &component_id,
    const std::string &revision,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/clouddoc/file/" + urlEncodingLoose(docname) + "?compId="+component_id;

        if(revision != "")
            url += ("&revision="+revision);

        rv = agent->del(url, headers, 3, NULL, 0, NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Delete pass. response: %s", response.c_str());

    //not expect any json output
    //rv = check_json_format(response);

end:
    return rv;
}


int clouddochttp_delete_async(u64 userId,
    const std::string &docname,
    const std::string &component_id,
    const std::string &revision,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        std::string payload;
        std::stringstream ss;
        ss << "{ \"op\":\"delete\", ";

        if(docname != ""){
            ss << "\"name\":\"" << urlEncodingLoose(docname) << "\"";
        }else{
            LOG_ERROR("async delete requires docname");
            free(reqUserId);
            free(reqHandle);
            free(reqTicket);
            rv = -1;
            goto end;
        }          
 
        if(component_id != "") {
            ss << ",\"compId\":" << component_id;
        }
        if (revision != "") {
            ss << ",\"revision\":" << revision;
        }
        ss << "}";
        
        payload = ss.str();
        std::cout << "payload: " << payload << std::endl;

        url = base_url + "/clouddoc/async";

        rv = agent->post(url, headers, 3, payload.c_str(), payload.size(), NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) { LOG_ERROR("agent->post(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("DeleteAsync pass. response: %s", response.c_str());

    // TODO
    // On success, response body is empty.
    // Get requestid from Location header in response.

end:
    return rv;
}


int clouddochttp_move(u64 userId,
    const std::string &docname,
    const std::string &component_id,
    const std::string &revision,
    const std::string &newdocname,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;
    TargetDevice *target = NULL;


    std::stringstream ss;
    u64 deviceId = 0;
    std::string deviceIdstr;
    getDeviceId(&deviceId);
    ss << deviceId;
    deviceIdstr = ss.str();
    std::cout << "The deviceid is " <<  deviceIdstr << std::endl;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        //// Clean up the path
        std::string doc2 = newdocname;         // The fullpath replacing all backslash with forward slash
        // Replace backslash with forward slash
        std::replace(doc2.begin(), doc2.end(), '\\', '/');


        // check path are absolute
        target = getTargetDevice();
        if (isWindows_ForDocs(target->getOsVersion())) {
            if (doc2.length() < 3 || doc2[1] != ':' || doc2[2] != '/'){
                LOG_ERROR("bad path");
                rv = -1;
                goto end;
            }
        }else{
            if (doc2.empty() || doc2[0] != '/'){
                LOG_ERROR("bad path: %s", doc2.c_str());
                rv = -1;
                goto end;
            }
        }

        std::string doc3 = doc2;        // for docname
        if (isWindows_ForDocs(target->getOsVersion())) {
            // remove colon afer drive letter
            // e.g., C:/Users/fokushi/test.docx -> C/Users/fokushi/test.docx
            doc3.erase(1, 1);
        }else{
            // strip initial slash
            // e.g., /home/fokushi/test.txt -> home/fokushi/test.txt
            doc3.erase(0, 1);
        }


        url = base_url + "/clouddoc/file/" + deviceIdstr + "/" + urlEncodingLoose(doc3) +"?compId="+component_id+"&moveFrom="+urlEncodingLoose(docname);
        if(revision != "")
            url += ("&revision="+revision);

        rv = agent->post(url, headers, 3, NULL, 0, NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Move pass. response: %s", response.c_str());

    rv = check_json_format(response);
    if(rv == 0) rv = check_json_format(response, "name");
    if(rv == 0) rv = check_json_format(response, "numOfRevisions");
    if(rv == 0) rv = check_json_format(response, "originDevice");
    if(rv == 0) rv = check_json_format(response, "revisionList");

end:
    if(target != NULL){
        delete target;
        target = NULL;
    }
    return rv;
}


int clouddochttp_move_async(u64 userId,
    const std::string &docname,
    const std::string &component_id,
    const std::string &revision,
    const std::string &newdocname,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    HttpAgent *agent = NULL;
    TargetDevice *target = NULL;

    std::stringstream ss;
    u64 deviceId = 0;
    std::string deviceIdstr;
    getDeviceId(&deviceId);
    ss << deviceId;
    deviceIdstr = ss.str();
    std::cout << "The deviceid is " <<  deviceIdstr << std::endl;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        //// Clean up the path
        std::string doc2 = newdocname;         // The fullpath replacing all backslash with forward slash
        // Replace backslash with forward slash
        std::replace(doc2.begin(), doc2.end(), '\\', '/');

        // check path are absolute
        target = getTargetDevice();
        if (isWindows_ForDocs(target->getOsVersion())) {
            if (doc2.length() < 3 || doc2[1] != ':' || doc2[2] != '/'){
                LOG_ERROR("bad path");
                free(reqUserId);
                free(reqHandle);
                free(reqTicket);
                rv = -1;
                goto end;
            }
        }else{
            if (doc2.empty() || doc2[0] != '/'){
                LOG_ERROR("bad path: %s", doc2.c_str());
                free(reqUserId);
                free(reqHandle);
                free(reqTicket);
                rv = -1;
                goto end;
            }
        }

        //
        // Note that the pathname formats here are different than in the
        // synchronous HTTP case:  here it does the same thing that the
        // older CCDIDocSaveAndGoNotifyChange API does.
        //
        std::string payload;
        ss.str("");

        ss << "{ \"op\":\"rename\", ";

        if(docname != ""){
            ss << "\"moveFrom\":\"" << urlEncodingLoose(docname) << "\"";
        }else{
            LOG_ERROR("async move requires docname");
            free(reqUserId);
            free(reqHandle);
            free(reqTicket);
            rv = -1;
            goto end;
        }          
        //ss << ",\"name\":\"" << urlEncodingLoose(doc2) << "\"";
        ss << ",\"name\":\"" << doc2 << "\"";
 
        if(component_id != "") {
            ss << ",\"compId\":" << component_id;
        }
        if (revision != "") {
            ss << ",\"revision\":" << revision;
        }
        ss << "}";
        
        payload = ss.str();
        std::cout << "payload: " << payload << std::endl;

        url = base_url + "/clouddoc/async";

        rv = agent->post(url, headers, 3, payload.c_str(), payload.size(), NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("MoveAsync pass. response: %s", response.c_str());

    // TODO
    // On success, response body is empty.
    // Get requestid from Location header in response.

end:
    if(target != NULL){
        delete target;
        target = NULL;
    }
    return rv;
}


int clouddochttp_dataset_changes(u64 userId, const std::string &datasetId, const std::string &pagination, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::map<std::string, std::string> parameters;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    // parsed and split the pagination parameters
    parse_arg(pagination, parameters);

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        if(pagination == ""){
            url = base_url + "/ds/datasetchanges/" + datasetId;
        }else{
            url = base_url + "/ds/datasetchanges/" + datasetId + "?" + createParameter(parameters);
        }

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("List dataset changes pass. response: %s", response.c_str());

    rv = check_json_format(response);
    if(rv == 0) rv = check_json_format(response, "currentVersion");
    if(rv == 0) rv = check_json_format(response, "changeList");

end:
    return rv;
}


int clouddochttp_check_conflict(u64 userId, 
    const std::string &docname,
    const std::string &component_id,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::map<std::string, std::string> parameters;
    std::string url;
    HttpAgent *agent = NULL;

    do {
        agent = getHttpAgent();
        if (!agent) {
            LOG_ERROR("Fail to get http agent");
            rv = -1;
            break;
        }

        rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
        if (rv < 0) {
            LOG_ERROR("getLocalHttpInfo returned %d.", rv);
            break;
        }

        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            break;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            break;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            break;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/clouddoc/conflict/" + urlEncodingLoose(docname) +"?compId="+component_id;

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            break;
        }

        LOG_ALWAYS("Check conflict passed. response: %s", response.c_str());

        // parse JSON response to get results
        cJSON2 *json_resp = cJSON2_Parse(response.c_str());
        if (json_resp) {
            do {
                cJSON2 *json_bool = cJSON2_GetObjectItem(json_resp, "conflict");
                if (json_bool == NULL) {
                    LOG_ERROR("Invalid response format:  conflict not found");
                    rv = -1;
                    break;
                }
                if (json_bool->type == cJSON2_False) {
                    LOG_ALWAYS("No conflict");
                    break;
                }
                if (json_bool->type != cJSON2_True) {
                    LOG_ERROR("Invalid response format:  conflict value invalid");
                    rv = -1;
                    break;
                }
                cJSON2 *json_revarray = cJSON2_GetObjectItem(json_resp, "revisionList");
                if (json_revarray == NULL) {
                    LOG_ERROR("Invalid response format:  revisionList not found");
                    rv = -1;
                    break;
                }
                int revarray_size = cJSON2_GetArraySize(json_revarray);
                LOG_ALWAYS("revisionList has %d entries", revarray_size);
                if (revarray_size < 2) {
                    LOG_ERROR("Invalid response:  revisionList should have two or more entries");
                    rv = -1;
                    break;
                }
                u64 revision = 0;
                u64 updateDevice = 0;
                u64 lastChanged = 0;
                u64 baseRevision = 0;
                for (int i = 0; i < revarray_size; i++) {
                    cJSON2 *json_rev = cJSON2_GetArrayItem(json_revarray, i);
                    if (json_rev) {
                        if (JSON_getInt64(json_rev, "revision", revision) != 0) {
                            LOG_ERROR("Revision not found");
                        }
                        if (JSON_getInt64(json_rev, "updateDevice", updateDevice) != 0) {
                            LOG_ERROR("updateDevice not found");
                        }
                        if (JSON_getInt64(json_rev, "lastChanged", lastChanged) != 0) {
                            LOG_ERROR("lastChanged not found");
                        }
                        if (JSON_getInt64(json_rev, "baseRevision", baseRevision) != 0) {
                            LOG_ERROR("baseRevision not found");
                        }
                        LOG_ALWAYS("revision "FMTu64", updateDevice "FMTu64", lastChanged "FMTu64", baseRevision "FMTu64,
                            revision, updateDevice, lastChanged, baseRevision);
                    }
                }
            } while (0);
            cJSON2_Delete(json_resp);
        } else {
            LOG_ERROR("Invalid response:  json parse fails");
            rv = -1;
            break;
        }
    } while (0);

    return rv;
}


int clouddochttp_check_copyback(u64 userId, 
    const std::string &docname,
    const std::string &component_id,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::map<std::string, std::string> parameters;
    std::string url;
    HttpAgent *agent = NULL;

    do {
        agent = getHttpAgent();
        if (!agent) {
            LOG_ERROR("Fail to get http agent");
            rv = -1;
            break;
        }

        rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
        if (rv < 0) {
            LOG_ERROR("getLocalHttpInfo returned %d.", rv);
            break;
        }

        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            break;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            break;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            break;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };
       
        url = base_url + "/clouddoc/copyback/" + urlEncodingLoose(docname) +"?compId="+component_id;

        rv = agent->post(url, headers, 3, NULL, 0, NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->post(\"%s\") returned %d", url.c_str(), rv);
            break;
        }

        LOG_ALWAYS("Check copyback passed. response: %s", response.c_str());

        // parse JSON response to get results
        cJSON2 *json_resp = cJSON2_Parse(response.c_str());
        if (json_resp) {
            do {
                u64 numRequests = 0;
                if (JSON_getInt64(json_resp, "numOfRequests", numRequests) != 0) {
                    LOG_ERROR("Invalid response format:  numOfRequests not found");
                    rv = -1;
                    break;
                }
                LOG_ALWAYS("numOfRequests "FMTu64, numRequests);
            } while (0);
            cJSON2_Delete(json_resp);
        } else {
            LOG_ERROR("Invalid response:  json parse fails");
            rv = -1;
            break;
        }
    } while(0);

    return rv;
}


class CloudDocHttpDispatchTable {
public:
    CloudDocHttpDispatchTable() {
        cmds["UploadBasic"]     = clouddochttp_upload_basic;
        cmds["UploadRevision"]  = clouddochttp_upload_revision;
        cmds["ListReq"]         = clouddochttp_list_requests;
        cmds["GetProgress"]     = clouddochttp_get_progress;
        cmds["CancelReq"]       = clouddochttp_cancel_request;
        cmds["ListDocs"]        = clouddochttp_list_docs;
        cmds["GetMetadata"]     = clouddochttp_get_metadata;
        cmds["Download"]        = clouddochttp_download;
        cmds["DownloadPreview"] = clouddochttp_download_preview;
        cmds["Delete"]          = clouddochttp_delete;
        cmds["Move"]            = clouddochttp_move;
        cmds["DeleteAsync"]     = clouddochttp_delete_async;
        cmds["MoveAsync"]       = clouddochttp_move_async;
        cmds["DatasetChanges"]  = clouddochttp_dataset_changes;
        cmds["CheckConflict"]   = clouddochttp_check_conflict;
        cmds["CheckCopyBack"]   = clouddochttp_check_copyback;
        cmds["Help"]            = clouddochttp_help_with_response;
    }
    std::map<std::string, clouddoc_subcmd_fn> cmds;
};

static CloudDocHttpDispatchTable cloudDocHttpDispatchTable;

static int clouddoc_dispatch(std::map<std::string, clouddoc_subcmd_fn> &cmdmap, int argc, const char* argv[], std::string &response)
{
    int rv = 0;

    if (cmdmap.find(argv[0]) != cmdmap.end()) {
        rv = cmdmap[argv[0]](argc, &argv[0], response);
    } else {
        LOG_ERROR("Command %s not supported", argv[0]);
        rv = -1;
    }
    return rv;
}

int dispatch_clouddochttp_cmd(int argc, const char* argv[])
{
    std::string response;

    if (argc <= 1)
        return clouddochttp_help(argc, argv);
    else
        return clouddoc_dispatch(cloudDocHttpDispatchTable.cmds, argc - 1, &argv[1], response);
}

int dispatch_clouddochttp_cmd_with_response(int argc, const char* argv[], std::string& response)
{
    if (argc <= 1)
        return clouddochttp_help(argc, argv);
    else
        return clouddoc_dispatch(cloudDocHttpDispatchTable.cmds, argc - 1, &argv[1], response);
}

int JSON_getInt64(cJSON2* node, const char* attributeName, u64& value)
{
    int rv = 0;

    if (node==NULL) {
        LOG_ERROR("JSON_getInt64: Node is null");
        return -1;
    }

    cJSON2* json_value;
    if (JSON_getJSONObject(node, attributeName, &json_value)) {
        LOG_INFO("JSON_getInt64, Can\' find - %s", attributeName);
        return -1;
    }

    value = (u64)json_value->valueint;
    return rv;
}

int JSON_getString(cJSON2* node, const char* attributeName, std::string& value)
{
    int rv = 0;

    if (node==NULL) {
        LOG_ERROR("JSON_getString, Node is null");
        return -1;
    }

    cJSON2* json_value;
    if (JSON_getJSONObject(node, attributeName, &json_value)) {
        LOG_INFO("JSON_getString, Can\' find - %s", attributeName);
        return -1; 
    }

    value = json_value->valuestring;
    return rv;
}

int JSON_getJSONObject(cJSON2* node, const char* attributeName, cJSON2** value)
{
    int rv = 0;
    if (node==NULL) {
        LOG_ERROR("JSON_getJSONObject: Node is null");
        return -1;
    }

    cJSON2* json_value;
    json_value = cJSON2_GetObjectItem(node, attributeName);
    if (json_value==NULL) {
        return -1;
    }
    *value = json_value;
    return rv;
}

std::string urlEncoding(std::string &sIn)
{
    std::string sOut;
    for(u16 ix = 0; ix < sIn.size(); ix++) {
        u8 buf[4];
        memset( buf, 0, 4 );
        if (isalnum( (u8)sIn[ix]) ) {
            buf[0] = sIn[ix];
        } else if ( isspace( (u8)sIn[ix] ) ) {
            buf[0] = '+';
        } else {
            buf[0] = '%';
            buf[1] = toHex( (u8)sIn[ix] >> 4 );
            buf[2] = toHex( (u8)sIn[ix] % 16);
        }
        sOut += (char *)buf;
    }
    return sOut;
}

std::string createParameter(std::map<std::string, std::string> parameters)
{
    std::string str_parameter = "";
    std::map<std::string, std::string>::iterator it;

    if (parameters.size() > 0) {
        std::map<std::string, std::string>::iterator it = parameters.begin();
        str_parameter += (*it).first + "=" + urlEncoding((*it).second);
        it++;
        for ( ; it != parameters.end(); it++ ) {
            str_parameter += "&" + (*it).first + "=" + urlEncoding((*it).second);
        }
    }

    return str_parameter;
}
