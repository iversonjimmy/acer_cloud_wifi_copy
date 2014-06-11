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
#include <vplex_http_util.hpp>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vpl_conv.h>
#include <vpl_string.h>
#include "gvm_file_utils.hpp"
#include "gvm_misc_utils.h"
#include "log.h"
#include "scopeguard.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <fstream>
#include <cstdio>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "clouddochttp.hpp"
#include "fs_test.hpp"
#include "EventQueue.hpp"
#include "HttpAgent.hpp"
#include "TargetLocalDevice.hpp"
#include "cJSON2.h"

#define RF_HEADER_USERID         "x-ac-userId"
#define RF_HEADER_SESSION_HANDLE "x-ac-sessionHandle"
#define RF_HEADER_SERVICE_TICKET "x-ac-serviceTicket"
#define RF_HEADER_API_VERSION    "x-ac-rf-api-version"

//http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Dataset_Access

static std::string getNameSpace(bool is_media_rf) {
    if (is_media_rf) {
        return "/media_rf";
    } else {
        return "/rf";
    }
}

static int fs_test_help(int argc, const char* argv[])
{
    bool printAll = strcmp(argv[0], "RemoteFile") == 0 || strcmp(argv[0], "MediaRF") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll || strcmp(argv[0], "ReadDir") == 0) {
        std::cout << "RemoteFile/MediaRF ReadDir [DatasetId] [Path] '[sortBy=<value>;index=<value>;max=<value>]'" << std::endl
                  << " - Pagination parameters are optional and are given w/o spaces and quoted by ''" << std::endl
                  << "   > sortBy = [time | size | alpha]" << std::endl
                  << "   > index  = starts from 1" << std::endl
                  << "   > max    = maximum return entries" << std::endl;
    }
    if (printAll || strcmp(argv[0], "MakeDir") == 0)
        std::cout << "RemoteFile/MediaRF MakeDir [DatasetId] [Path]" << std::endl;
#if 0
    if (printAll || strcmp(argv[0], "CopyDir") == 0)
        std::cout << "RemoteFile/MediaRF CopyDir [DatasetId] [Path] [NewPath]" << std::endl;
    if (printAll || strcmp(argv[0], "CopyDirV2") == 0)
        std::cout << "RemoteFile/MediaRF CopyDirV2 [DatasetId] [Path] [NewPath]" << std::endl;
#endif
    if (printAll || strcmp(argv[0], "RenameDir") == 0)
        std::cout << "RemoteFile/MediaRF RenameDir [DatasetId] [Path] [NewPath]" << std::endl;
    if (printAll || strcmp(argv[0], "RenameDirV2") == 0)
        std::cout << "RemoteFile/MediaRF RenameDirV2 [DatasetId] [Path] [NewPath]" << std::endl;
    if (printAll || strcmp(argv[0], "DeleteDir") == 0)
        std::cout << "RemoteFile/MediaRF DeleteDir [DatasetId] [Path]" << std::endl;
    if (printAll || strcmp(argv[0], "ReadMetadata") == 0)
        std::cout << "RemoteFile/MediaRF ReadMetadata [DatasetId] [Path]" << std::endl;
    if (printAll || strcmp(argv[0], "SetPermission") == 0) {
        std::cout << "RemoteFile/MediaRF SetPermission [DatasetId] [Path] '[stat1=<value>;stat2=<value>;...]'" << std::endl
                  << " - Available stat: isReadOnly, isHidden, isSystem, isArchive" << std::endl
                  << " - Value: true or false" << std::endl;
    }
    if (printAll || strcmp(argv[0], "Download") == 0)
        std::cout << "RemoteFile/MediaRF Download [DatasetId] [File to download] [Output file path] [Range Header]" << std::endl;
    if (printAll || strcmp(argv[0], "Upload") == 0)
        std::cout << "RemoteFile/MediaRF Upload [DatasetId] [Save to path] [File to upload]" << std::endl;
    if (printAll || strcmp(argv[0], "CopyFile") == 0)
        std::cout << "RemoteFile/MediaRF CopyFile [DatasetId] [Path] [NewPath]" << std::endl;
    if (printAll || strcmp(argv[0], "CopyFileV2") == 0)
        std::cout << "RemoteFile/MediaRF CopyFileV2 [DatasetId] [Path] [NewPath]" << std::endl;
    if (printAll || strcmp(argv[0], "MoveFile") == 0)
        std::cout << "RemoteFile/MediaRF MoveFile [DatasetId] [Path] [NewPath]" << std::endl;
    if (printAll || strcmp(argv[0], "MoveFileV2") == 0)
        std::cout << "RemoteFile/MediaRF MoveFileV2 [DatasetId] [Path] [NewPath]" << std::endl;
    if (printAll || strcmp(argv[0], "DeleteFile") == 0)
        std::cout << "RemoteFile/MediaRF DeleteFile [DatasetId] [Path]" << std::endl;
    if (printAll || strcmp(argv[0], "AsyncUpload") == 0)
        std::cout << "RemoteFile/MediaRF AsyncUpload [DatasetId] [Save to path] [File to upload]" << std::endl;
    if (printAll || strcmp(argv[0], "CancelAsyncReq") == 0)
        std::cout << "RemoteFile/MediaRF CancelAsyncReq [Async handle] " << std::endl;
    if (printAll || strcmp(argv[0], "GetProgress") == 0)
        std::cout << "RemoteFile/MediaRF GetProgress [Async handle(optional)] " << std::endl;
    if (printAll || strcmp(argv[0], "EditTag") == 0)
        std::cout << "RemoteFile/MediaRF EditTag [deviceid] [objectid] [tagname=tagvalue;tagname=tagvalue;...]" << std::endl;
    if (printAll || strcmp(argv[0], "ListDatasets") == 0)
        std::cout << "RemoteFile/MediaRF ListDatasets" << std::endl;
    if (printAll || strcmp(argv[0], "ListDevices") == 0)
        std::cout << "RemoteFile/MediaRF ListDevices" << std::endl;
    if (printAll || strcmp(argv[0], "AccessControl") == 0){
        std::cout << "RemoteFile/MediaRF AccessControl [DatasetId] [Add/Remove/Get] [Allow/Deny] [User/System] [AbsPath/Path]" << std::endl
                  << " - Ex. RemoteFile AccessControl 1234567 Get" << std::endl
                  << " - Ex. RemoteFile AccessControl 1234567 Add Deny User c:\\" << std::endl;
    }
    // See http://wiki.ctbg.acer.com/wiki/index.php/Remote_File_Filename_Search_Design for RemoteFile Search API
    if (printAll || strcmp(argv[0], "SearchBegin") == 0) {
        std::cout << "RemoteFile/MediaRF SearchBegin <DatasetId> <AbsPath/Path> <searchString> [--disableIndex] [--disableRecursive]" << std::endl;
    }
    if (printAll || strcmp(argv[0], "SearchGet") == 0) {
        std::cout << "RemoteFile/MediaRF SearchGet <DatasetId> <searchQueueId> <startIndex> <maxPageSize>" << std::endl;
    }
    if (printAll || strcmp(argv[0], "SearchEnd") == 0) {
        std::cout << "RemoteFile/MediaRF SearchEnd <DatasetId> <searchQueueId>" << std::endl;
    }

    return 0;
}

static int fs_test_help_response(int argc, const char* argv[], std::string& response, bool is_media_rf) {
    return fs_test_help(argc, argv);
}

static int check_json_format(std::string& json) {
    cJSON2 *jsonResponse = cJSON2_Parse(json.c_str());
    if (!jsonResponse) {
        LOG_ERROR("Invalid root json data.");
        return -1;
    }
    cJSON2_Delete(jsonResponse);
    jsonResponse = NULL;
    return 0;
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
        size_t end_key = input.find_first_of("=;", start);
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

static int fs_test_download(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(!argv[4]) {
        return fs_test_download(userId, argv[1], argv[2], argv[3], "/", response, is_media_rf);
    }
    else {
        return fs_test_download(userId, argv[1], argv[2], argv[3], argv[4], response, is_media_rf);
    }
}

static int fs_test_deletefile(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 3) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_deletefile(userId, argv[1], argv[2], response, is_media_rf);
}

static int fs_test_upload(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_upload(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_async_upload(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_async_upload(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_async_cancel(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 2) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }
    return fs_test_async_cancel(userId, argv[1], response, is_media_rf);
}

static int fs_test_async_progress(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 1) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }
    if (argc < 2) {
        std::string empty = "";
        return fs_test_async_progress(userId, empty, response, is_media_rf);
    } else {
        return fs_test_async_progress(userId, argv[1], response, is_media_rf);
    }
}

static int fs_test_readdir(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 3 || argc > 5) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if (argc == 3) {
        std::string empty;
        return fs_test_readdir(userId, argv[1], argv[2], empty, response, is_media_rf);
    }
    return fs_test_readdir(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_makedir(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 3) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_makedir(userId, argv[1], argv[2], response, is_media_rf);
}

static int fs_test_copydir(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_copydir(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_copydir_v2(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_copydir_v2(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_copyfile(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_copyfile(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_copyfile_v2(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_copyfile_v2(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_renamedir(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_renamedir(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_renamedir_v2(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_renamedir_v2(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_movefile(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_movefile(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_movefile_v2(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_movefile_v2(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_deletedir(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 3) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_deletedir(userId, argv[1], argv[2], response, is_media_rf);
}

static int fs_test_edittag(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_edittag(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_listdatasets(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc > 1) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_listdatasets(userId, response, is_media_rf);
}

static int fs_test_listdevices(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc > 1) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_listdevices(userId, response, is_media_rf);
}

static int fs_test_readmetadata(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 3) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_readmetadata(userId, argv[1], argv[2], response, is_media_rf);
}

static int fs_test_setpermission(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return fs_test_setpermission(userId, argv[1], argv[2], argv[3], response, is_media_rf);
}

static int fs_test_accesscontrol(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc != 6 && argc != 3) {
        fs_test_help(argc, argv);
        return -1;
    }

    if(argc == 3){
        //AccessControl [DatasetId] Get
        std::string action = argv[2];
        if (action != "Get") {
            LOG_ERROR("Should use Get");
            fs_test_help(argc, argv);
            return -1;
        }
        u64 userId = 0;
        int rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Failed to get userId.");
            return -1;
        }

        return fs_test_accesscontrol(userId, argv[1], argv[2], response, is_media_rf);
    }else{

        std::string action = argv[2];
        if (action != "Add" && action != "Remove") {
            LOG_ERROR("Should use Add or Remove");
            fs_test_help(argc, argv);
            return -1;
        }

        std::string rule = argv[3];
        if (rule != "Allow" && rule != "Deny") {
            LOG_ERROR("Should use Allow or Deny");
            fs_test_help(argc, argv);
            return -1;
        }

        std::string user = argv[4];
        if (user != "User" && user != "System") {
            LOG_ERROR("Should use User or System");
            fs_test_help(argc, argv);
            return -1;
        }

        u64 userId = 0;
        int rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Failed to get userId.");
            return -1;
        }

        return fs_test_accesscontrol(userId, argv[2], argv[3], argv[4], argv[5], response, is_media_rf);
    }
}

static int fs_test_readdirmetadata(int argc, const char* argv[], std::string& response, bool is_media_rf)
{
    if (argc != 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    //ReadDirMetadata <DatasetId> <Path>
    return fs_test_readdirmetadata(userId, argv[1], argv[2], argv[3], response, is_media_rf);

}

static int fs_test_search_begin(int argc, const char* argv[],
                                std::string& response, bool is_media_rf)
{
    if (argc < 4) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }
    u64 datasetId = VPLConv_strToU64(argv[1], NULL, 10);
    bool disableIndex = false;
    bool recursive = true;
    for (int argIndex = 4; argIndex < argc; argIndex++)
    {
        if (std::string("--disableIndex") == argv[argIndex]) {
            disableIndex = true;
        } else if (std::string("--disableRecursive") == argv[argIndex]) {
            recursive = false;
        }
    }
    u64 searchQueueId;
    rv = fs_test_search_begin(userId, datasetId, argv[2], argv[3],
                              disableIndex, recursive, is_media_rf, true,
                              /*OUT*/ response, /*OUT*/ searchQueueId);
    if (rv != 0) {
        LOG_ERROR("fs_test_search_begin:%d", rv);
        return rv;
    }
    return 0;
}

static int fs_test_search_get(int argc, const char* argv[],
                              std::string& response, bool is_media_rf)
{
    if (argc != 5) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }
    u64 datasetId = VPLConv_strToU64(argv[1], NULL, 10);
    u64 searchQueueId = VPLConv_strToU64(argv[2], NULL, 10);
    u64 startIndex = VPLConv_strToU64(argv[3], NULL, 10);
    u64 maxPageSize = VPLConv_strToU64(argv[4], NULL, 10);

    std::vector<FSTestRFSearchResult> searchResults;
    u64 numberReturned;
    bool searchInProgress;

    rv = fs_test_search_get(userId,
                            datasetId,
                            searchQueueId,
                            startIndex,
                            maxPageSize,
                            is_media_rf,
                            true,  // printResponse
                            /*OUT*/ response,
                            /*OUT*/ searchResults,
                            /*OUT*/ numberReturned,
                            /*OUT*/ searchInProgress);
    if (rv != 0) {
        LOG_ERROR("fs_test_search_get:%d", rv);
        return rv;
    }
    return 0;
}

static int fs_test_search_end(int argc, const char* argv[],
                              std::string& response, bool is_media_rf)
{
    if (argc != 3) {
        fs_test_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }
    u64 datasetId = VPLConv_strToU64(argv[1], NULL, 10);
    u64 searchQueueId = VPLConv_strToU64(argv[2], NULL, 10);

    rv = fs_test_search_end(userId, datasetId, searchQueueId, is_media_rf, /*OUT*/response);
    if (rv != 0) {
        LOG_ERROR("fs_test_search_end:%d", rv);
        return rv;
    }
    return 0;
}

//========================================================
int fs_test_download(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &outputFile,
    const std::string &rangeHeader,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    std::string url;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::stringstream ss;
    std::string temp;
    std::map<std::string, std::string> parameters;
    HttpAgent *agent = NULL;

    LOG_INFO("Downloading \"%s\" to \"%s\"", path.c_str(), outputFile.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

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

        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->get_extended(url, 0, 0, headers, nr_headers, outputFile.c_str());
        LOG_ALWAYS("download rv = %d", rv);
        if (rv < 0) {
            VPLFile_handle_t fh = VPLFile_Open(outputFile.c_str(), VPLFILE_OPENFLAG_READONLY, 0666);
            if (!VPLFile_IsValidHandle(fh)) {
                LOG_ERROR("Error opening response file %s, %d", outputFile.c_str(), fh);
            } else {
                static const int BUF_SIZE = 1024;
                ssize_t len = 0;
                char buf[BUF_SIZE];
                response = "";
                do {
                    len = VPLFile_Read(fh, buf, 1024);
                    if (len < 0) {
                        LOG_ERROR("Error while reading response file %s, %d", outputFile.c_str(), fh);
                        break;
                    }
                    response.append(buf, len);
                } while (len == BUF_SIZE);

                VPLFile_Close(fh);

                if (VPLFile_Delete(outputFile.c_str()) != VPL_OK) {
                    LOG_ERROR("Fail to clean-up response file %s", outputFile.c_str());
                }
            }

            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Remote File download pass");

    }

end:
    return rv;
}

// currently we are using environment varible DX_REMOTE_IP, DX_REMOTE_PORT to save ip and port
// it means one set of ip and port is shared by multiple threads which will cause issue if multiple
// threads are calling remotefile download function
// we add another download function to allow user to pass in specific ip and port
// instead of getting them from environment members
int fs_test_download_by_ip_port(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &outputFile,
    const std::string &rangeHeader,
    const std::string &ip_addr,
    const std::string &port,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    std::string url;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::stringstream ss;
    std::string temp;
    std::map<std::string, std::string> parameters;
    HttpAgent *agent = NULL;
    VPLNet_addr_t ipaddr;
    VPLNet_port_t local_port;

    LOG_INFO("Downloading \"%s\" to \"%s\"", path.c_str(), outputFile.c_str());
    LOG_ALWAYS("Downloading \"%s\" to \"%s\"", path.c_str(), outputFile.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

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

        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        LOG_ALWAYS("url = %s", url.c_str());
        ipaddr = static_cast<uint32_t>(strtoul(ip_addr.c_str(), NULL, 0));
        local_port = static_cast<u16>(strtoul(port.c_str(), NULL, 0));
        rv = agent->get_extended_by_ip_port(url, 0, 0, headers, nr_headers, outputFile.c_str(), ipaddr, local_port);
        LOG_ALWAYS("download rv = %d", rv);
        if (rv < 0) {
            VPLFile_handle_t fh = VPLFile_Open(outputFile.c_str(), VPLFILE_OPENFLAG_READONLY, 0666);
            if (!VPLFile_IsValidHandle(fh)) {
                LOG_ERROR("Error opening response file %s, %d", outputFile.c_str(), fh);
            } else {
                static const int BUF_SIZE = 1024;
                ssize_t len = 0;
                char buf[BUF_SIZE];
                response = "";
                do {
                    len = VPLFile_Read(fh, buf, 1024);
                    if (len < 0) {
                        LOG_ERROR("Error while reading response file %s, %d", outputFile.c_str(), fh);
                        break;
                    }
                    response.append(buf, len);
                } while (len == BUF_SIZE);

                VPLFile_Close(fh);

                if (VPLFile_Delete(outputFile.c_str()) != VPL_OK) {
                    LOG_ERROR("Fail to clean-up response file %s", outputFile.c_str());
                }
            }

            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Remote File download pass");

    }

end:
    return rv;
}


int fs_test_deletefile(u64 userId,
    const std::string &dataset,
    const std::string &path,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("DeleteFile: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->del(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Delete file: %s", response.c_str());
    }

end:
    return rv;
}

int fs_test_upload(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &file,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    s64 tmplSz = 0;
    u64 contentLen = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    std::stringstream ss1;
    std::string tempPath = path;
    std::map<std::string, std::string> parameters;
    VPLFile_handle_t fHDst = VPLFILE_INVALID_HANDLE;
    HttpAgent *agent = NULL;
    u64 remoteFileSize = 0;
    TargetDevice *target = getTargetDevice();

    VPLFS_stat_t statBuf;
    rv = target->statFile(file, statBuf);
    if(rv != 0) {
        LOG_ERROR("VPLFSStat(%s):%d", file.c_str(), rv);
        rv = -1;
        goto end;
    }

    LOG_INFO("Uploading file \"%s\" to \"%s\". size("FMTu64")", file.c_str(), path.c_str(), (u64)statBuf.size);
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

    if (getenv("DX_REMOTE_IP") == NULL) {
        fHDst = VPLFile_Open(file.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(fHDst)) {
            LOG_ERROR("Fail to open source file %s", file.c_str());
            rv = -1;
            goto end;
        }

        tmplSz = (s64) VPLFile_Seek(fHDst, 0, VPLFILE_SEEK_END);
        if (tmplSz < 0) {
            LOG_ERROR("VPLFile_Seek failed: "FMTs64, tmplSz);
            rv = -1;
            goto end;
        }

        VPLFile_Seek(fHDst, 0, VPLFILE_SEEK_SET); // Reset

        if (VPLFile_IsValidHandle(fHDst)) {
            VPLFile_Close(fHDst);
            fHDst = VPLFILE_INVALID_HANDLE;
        }
    }
    else {
        rv = target->getFileSize(file, remoteFileSize);
        if (rv < 0) {
            LOG_ERROR("Fail to get file(%s) size from remote %d", file.c_str(), rv);
            goto end;
        }

        tmplSz = (s64) remoteFileSize;
    }

    contentLen += tmplSz;
    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqLength = VPLString_MallocAndSprintf("Content-Length: "FMTu64, contentLen);
        if (reqLength == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqLength);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqLength,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->post(url, headers, 4, NULL, 0, file.c_str(), response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Remote File upload pass. response: %s", response.c_str());

    }

end:
    return rv;
}

int fs_test_async_upload(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &file,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::stringstream ss;
    std::string payload;
    std::string url;
    std::map<std::string, std::string> parameters;
    HttpAgent *agent = NULL;

    LOG_INFO("Uploading file \"%s\" to \"%s\".", file.c_str(), path.c_str());
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

    ss.str("");
    ss << "{\"datasetId\":" << dataset
       <<",\"op\":\"" << "upload" << "\"" // XXX download?
       <<",\"path\":\"" << path << "\""
       <<",\"filepath\":\"" << file << "\""
       << "}";

    payload = ss.str();
    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqLength = VPLString_MallocAndSprintf("Content-Length: %d", payload.size());
        if (reqLength == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqLength);
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqLength,
            reqUserId,
            reqHandle,
            reqTicket,
            "Content-Type: application/json",
        };

        url = base_url + getNameSpace(is_media_rf) + "/async";

        rv = agent->post(url, headers, 5, payload.c_str(), payload.size(), NULL, response);
        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Remote File async upload pass. response: %s", response.c_str());
    }

end:
    return rv;

}

int fs_test_async_progress(u64 userId,
                            const std::string &async_handle,
                            std::string& response,
                            bool is_media_rf)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::stringstream ss;
    std::string url;
    std::map<std::string, std::string> parameters;
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
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/async";
        if (!async_handle.empty()) {
            url.append("/" + async_handle);
        }

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Response: %s", response.c_str());

        rv = check_json_format(response);
    }

end:
    return rv;
}

int fs_test_async_cancel(u64 userId,
                          const std::string &async_handle,
                          std::string& response,
                          bool is_media_rf)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::stringstream ss;
    std::string url;
    std::map<std::string, std::string> parameters;
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/async/" + async_handle;

        rv = agent->del(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Response: %s", response.c_str());
    }

end:
    return rv;
}

int fs_test_readdir(u64 userId,
                    const std::string &dataset,
                    const std::string &path,
                    const std::string &pagination,
                    std::string& response,
                    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::stringstream ss;
    std::map<std::string, std::string> parameters;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("ReadDir: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath) + "?" + createParameter(parameters);

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Read directory: %s", response.c_str());

        rv = check_json_format(response);
    }

end:
    return rv;
}

int fs_test_makedir(u64 userId,
                    const std::string &dataset,
                    const std::string &path,
                    std::string& response,
                    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("MakeDir: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->put(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Make directory: %s", response.c_str());
    }

end:
    return rv;
}

int fs_test_copydir(u64 userId,
                    const std::string &dataset,
                    const std::string &path,
                    const std::string &newpath,
                    std::string& response,
                    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("CopyDir: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        std::string tempNewpath = newpath;
        url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
              + "?copyFrom=" + VPLHttp_UrlEncoding(tempNewpath);

        rv = agent->post(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Copy directory pass.");
    }

end:
    return rv;
}

int fs_test_copydir_v2(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &newpath,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string api_version;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("CopyDir: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);
        api_version.assign("2");
        char *reqApiVersion = VPLString_MallocAndSprintf(RF_HEADER_API_VERSION": %s", api_version.c_str());
        if (reqApiVersion == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqApiVersion);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
            reqApiVersion,
        };

        std::string tempNewpath = newpath;
        //url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
        //    + "?copyFrom=" + VPLHttp_UrlEncoding(tempNewpath);
        url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempNewpath)
            + "?copyFrom=" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->post(url, headers, 4, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Copy directory pass.");
    }

end:
    return rv;
}

int fs_test_copyfile(u64 userId,
                     const std::string &dataset,
                     const std::string &path,
                     const std::string &newpath,
                     std::string& response,
                     bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("CopyFile: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };


        std::string tempNewpath = newpath;
        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
              + "?copyFrom=" + VPLHttp_UrlEncoding(tempNewpath);

        rv = agent->post(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Copy file pass.");
    }

end:
    return rv;
}

int fs_test_copyfile_v2(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &newpath,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string api_version;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("CopyFile: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);
        api_version.assign("2");
        char *reqApiVersion = VPLString_MallocAndSprintf(RF_HEADER_API_VERSION": %s", api_version.c_str());
        if (reqApiVersion == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqApiVersion);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
            reqApiVersion,
        };


        std::string tempNewpath = newpath;
        //url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
        //    + "?copyFrom=" + VPLHttp_UrlEncoding(tempNewpath);
        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempNewpath)
            + "?copyFrom=" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->post(url, headers, 4, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Copy file pass.");
    }

end:
    return rv;
}

int fs_test_renamedir(u64 userId,
                      const std::string &dataset,
                      const std::string &path,
                      const std::string &newpath,
                      std::string& response,
                      bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("RenameDir: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };


        std::string tempNewpath = newpath;
        url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
              + "?moveFrom=" + VPLHttp_UrlEncoding(tempNewpath);

        rv = agent->post(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Rename directory pass.");
    }

end:
    return rv;
}

int fs_test_renamedir_v2(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &newpath,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string api_version;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("RenameDir: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);
        api_version.assign("2");
        char *reqApiVersion = VPLString_MallocAndSprintf(RF_HEADER_API_VERSION": %s", api_version.c_str());
        if (reqApiVersion == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqApiVersion);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
            reqApiVersion,
        };

        std::string tempNewpath = newpath;
        //url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
        //    + "?moveFrom=" + VPLHttp_UrlEncoding(tempNewpath);
        url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempNewpath)
            + "?moveFrom=" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->post(url, headers, 4, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Rename directory pass.");
    }

end:
    return rv;
}

int fs_test_movefile(u64 userId,
                     const std::string &dataset,
                     const std::string &path,
                     const std::string &newpath,
                     std::string& response,
                     bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("MoveFile: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };


        std::string tempNewpath = newpath;
        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
              + "?moveFrom=" + VPLHttp_UrlEncoding(tempNewpath);

        rv = agent->post(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Rename directory pass.");
    }

end:
    return rv;
}

int fs_test_movefile_v2(u64 userId,
    const std::string &dataset,
    const std::string &path,
    const std::string &newpath,
    std::string& response,
    bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string api_version;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("MoveFile: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);
        api_version.assign("2");
        char *reqApiVersion = VPLString_MallocAndSprintf(RF_HEADER_API_VERSION": %s", api_version.c_str());
        if (reqApiVersion == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqApiVersion);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
            reqApiVersion,
        };

        std::string tempNewpath = newpath;
        //url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath)
        //    + "?moveFrom=" + VPLHttp_UrlEncoding(tempNewpath);
        url = base_url + getNameSpace(is_media_rf) + "/file/" + dataset + "/" + VPLHttp_UrlEncoding(tempNewpath)
            + "?moveFrom=" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->post(url, headers, 4, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Rename directory pass.");
    }

end:
    return rv;
}

int fs_test_deletedir(u64 userId,
                      const std::string &dataset,
                      const std::string &path,
                      std::string& response,
                      bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("DeleteDir: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/dir/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->del(url, headers, 3, NULL, 0, NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Delete directory: %s", response.c_str());
    }

end:
    return rv;
}

int fs_test_readmetadata(u64 userId,
                         const std::string &dataset,
                         const std::string &path,
                         std::string& response,
                         bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::map<std::string, std::string> parameters;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("ReadMetadata: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/filemetadata/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Read metadata: %s", response.c_str());

        rv = check_json_format(response);
    }

end:
    return rv;
}

int fs_test_setpermission(u64 userId,
                          const std::string &dataset,
                          const std::string &path,
                          const std::string &permissions,
                          std::string& response,
                          bool is_media_rf)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::string bodyPayload;
    std::map<std::string, std::string> payload;
    std::ostringstream oss;
    const char *tempPayload = NULL;
    std::string url;
    HttpAgent *agent = NULL;
    std::map<std::string, std::string>::iterator it;
    bool needComma = false;

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

    // get the tag parsed and write to the payload
    parse_arg(permissions, payload);

    for (it = payload.begin(); it != payload.end(); it++) {
        if (needComma) {
            oss << ",";
        }
        oss << "\"" << it->first << "\":" << it->second;
        needComma = true;
    }
    bodyPayload = "{" + oss.str() + "}";
    tempPayload = bodyPayload.c_str();

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqLength = VPLString_MallocAndSprintf("Content-Length: %d", strlen(tempPayload));
        if (reqLength == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqLength);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqLength,
            reqHandle,
            reqTicket,
            "Content-Type: application/json",
        };
        url = base_url + getNameSpace(is_media_rf) + "/filemetadata/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->put(url, headers, 5, tempPayload, strlen(tempPayload), NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
    }
    LOG_ALWAYS("Set permission pass. response: %s", response.c_str());

end:
    return rv;
}

int fs_test_edittag(u64 userId,
                    const std::string &deviceid,
                    const std::string &objectid,
                    const std::string &tagNameValueStr,
                    std::string& response,
                    bool is_media_rf)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string bodyPayload;
    std::map<std::string, std::string> payload;
    std::ostringstream oss;
    const char *tempPayload = NULL;
    std::string url;
    HttpAgent *agent = NULL;
    std::map<std::string, std::string>::iterator it;
    bool needComma = false;

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

    // get the tag parsed and write to the payload
    parse_arg(tagNameValueStr, payload);

    for (it = payload.begin(); it != payload.end(); it++) {
        if (needComma) {
            oss << ",";
        }
        oss << "\"" << it->first << "\":\"" << it->second << "\"";
        needComma = true;
    }
    bodyPayload = "{" + oss.str() + "}";
    tempPayload = bodyPayload.c_str();

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqLength = VPLString_MallocAndSprintf("Content-Length: %d", strlen(tempPayload));
        if (reqLength == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqLength);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqLength,
            reqHandle,
            reqTicket,
            "Content-Type: application/json",
        };
        url = base_url + "/mediafile/tag/" + deviceid + "/" + objectid;

        rv = agent->post(url, headers, 5, tempPayload, strlen(tempPayload), NULL, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
    }
    LOG_ALWAYS("Edit tag pass. response: %s", response.c_str());

end:
    return rv;
}

int fs_test_listdatasets(u64 userId,
                         std::string& response,
                         bool is_media_rf)
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/dataset";

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
    }
    LOG_ALWAYS("List dataset pass. response: %s", response.c_str());

    rv = check_json_format(response);

end:
    return rv;
}

int fs_test_listdevices(u64 userId,
                        std::string& response,
                        bool is_media_rf)
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/device";

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }

    }
    LOG_ALWAYS("List device pass. response: %s", response.c_str());

    rv = check_json_format(response);

end:
    return rv;
}

int fs_test_accesscontrol(u64 userId,
                          const std::string &dataset,
                          const std::string &action,
                          std::string& response,
                          bool is_media_rf)
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + getNameSpace(is_media_rf) + "/whitelist/" + dataset;

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }

    }
    LOG_ALWAYS("Get whitelist pass. response: %s", response.c_str());

    rv = check_json_format(response);

end:
    return rv;
}

int fs_test_accesscontrol(u64 userId,
                          const std::string &action,
                          const std::string &rule,
                          const std::string &user,
                          const std::string &path,
                          std::string& response,
                          bool is_media_rf)
{
    int rv = 0;

    {
        ccd::UpdateStorageNodeInput request;
        request.set_user_id(userId);
        ccd::RemoteFileAccessControlDirSpec *dirSpec;

        if(action == "Add")
            dirSpec = request.mutable_add_remotefile_access_control_dir();
        else if(action == "Remove")
            dirSpec = request.mutable_remove_remotefile_access_control_dir();
        else{
            rv = -1;
            goto end;
        }
            
        if(rule == "Allow")
            dirSpec->set_is_allowed(true);
        else
            dirSpec->set_is_allowed(false);
        
        if(user == "User")
            dirSpec->set_is_user(true);
        else
            dirSpec->set_is_user(false);

        if(path[1] == ':')
            dirSpec->set_dir(path);
        else
            dirSpec->set_name(path);


        rv = CCDIUpdateStorageNode(request);
        if (rv != 0) {
            LOG_ERROR("CCDIUpdateStorageNode for user("FMTu64") failed %d", userId, rv);
            goto end;
        } else {
            LOG_INFO("CCDIUpdateStorageNode OK");
        }
    }

    LOG_ALWAYS("Set accesscontrol pass. response: %s", response.c_str());

end:
    return rv;
}

int fs_test_search_begin(u64 userId,
                         u64 datasetId,
                         const std::string& path,
                         const std::string& searchString,
                         bool disableIndex,
                         bool recursive,
                         bool is_media_rf,
                         bool printResponse,
                         std::string& response_out,
                         u64& searchQueueId_out)
{
    int rv = 0;
    std::string base_url;
    std::string datasetIdStr;
    std::string service_ticket;
    std::string session_handle;
    std::string bodyPayload;
    std::string url;
    searchQueueId_out = 0;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    {
        std::ostringstream oss;
        oss << datasetId;
        datasetIdStr = oss.str();
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        std::ostringstream oss;
        oss << "\"path\":\"" << path.c_str() << "\",";
        oss << "\"searchString\":\"" << searchString.c_str() << "\",";
        oss << "\"disableIndex\":" << (disableIndex?"true":"false") << ",";
        oss << "\"recursive\":" << (recursive?"true":"false");

        bodyPayload = "{" + oss.str() + "}";
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqLength = VPLString_MallocAndSprintf("Content-Length: %d", bodyPayload.size());
        if (reqLength == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqLength);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqLength,
            reqHandle,
            reqTicket,
            "Content-Type: application/json",
        };
        url = base_url + getNameSpace(is_media_rf) + "/search/" + datasetIdStr + "/begin";

        rv = agent->post(url, headers, 5,
                         bodyPayload.c_str(), bodyPayload.size(),
                         NULL, response_out);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s",
                      url.c_str(), rv, response_out.c_str());
            goto end;
        }
    }
    if (printResponse) {
        LOG_ALWAYS("SearchBegin. request:(%s),requestBody(%s),response(%s)",
                   url.c_str(), bodyPayload.c_str(), response_out.c_str());
    }
    {
        bool searchQueueIdExists = false;

        cJSON2* jsonResponse = cJSON2_Parse(response_out.c_str());
        if (jsonResponse == NULL) {
            LOG_ERROR("cJSON2_Parse error");
            if (rv==0) { rv = -1; }
            goto end;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, jsonResponse);

        cJSON2* jsonObject = jsonResponse->child;
        while (jsonObject != NULL) {
            if (jsonObject->type == cJSON2_Number) {
                if (strcmp("searchQueueId", jsonObject->string)==0) {
                    searchQueueId_out = jsonObject->valueint;
                    searchQueueIdExists = true;
                }
            }
            jsonObject = jsonObject->next;
        }

        if (!searchQueueIdExists) {
            if (rv == 0) {
                LOG_ERROR("SearchQueueId does not exist.");
                rv = -1;
                goto end;
            }
        }

        if (printResponse) {
            std::cout << "    searchQueueId: " << searchQueueId_out << std::endl;
        }
    }
 end:
    return rv;
}

static int parseRFSearchGetSearchResults(cJSON2* cjsonFileArray,
                                         std::vector<FSTestRFSearchResult>& fileList_out)
{
    fileList_out.clear();
    int rv = 0;

    if(cjsonFileArray == NULL) {
        LOG_ERROR("cjsonFileArray is null (not zero elements)");
        return -1;
    }
    cJSON2* currEntry = cjsonFileArray->child;

    while(currEntry != NULL) {
        if (currEntry->type == cJSON2_Object) {
            cJSON2* fileEntry = currEntry->child;
            FSTestRFSearchResult entry;
            while (fileEntry != NULL) {
                if (fileEntry->type == cJSON2_String)
                {
                    if (strcmp("path", fileEntry->string)==0) {
                        entry.path.assign(fileEntry->valuestring);
                    } else if (strcmp("type", fileEntry->string)==0) {
                        if(strcmp("file", fileEntry->valuestring)==0) {
                            entry.isDir = false;
                        } else if(strcmp("dir", fileEntry->valuestring)==0) {
                            entry.isDir = true;
                        } else if(strcmp("shortcut", fileEntry->valuestring)==0) {
                            entry.isDir = false;
                            entry.isShortcut = true;
                        } else {
                            LOG_ERROR("Unrecognized entry type:%s", fileEntry->valuestring);
                        }
                    } else if (strcmp("target_path", fileEntry->string)==0) {
                        entry.shortcut.path.assign(fileEntry->valuestring);
                    } else if (strcmp("target_type", fileEntry->string)==0) {
                        entry.shortcut.type.assign(fileEntry->valuestring);
                    } else if (strcmp("target_args", fileEntry->string)==0) {
                        entry.shortcut.args.assign(fileEntry->valuestring);
                    }
                } else if (fileEntry->type == cJSON2_Number)
                {
                    if (strcmp("size", fileEntry->string)==0) {
                        entry.size = fileEntry->valueint;
                    } else if (strcmp("lastChanged", fileEntry->string)==0) {
                        entry.lastChanged = VPLTime_FromMillisec(fileEntry->valueint);
                    }
                } else if (fileEntry->type==cJSON2_True ||
                           fileEntry->type==cJSON2_False)
                {
                    if (strcmp("isAllowed", fileEntry->string)==0) {
                        entry.isAllowed = false;
                        if (fileEntry->type==cJSON2_True) {
                            entry.isAllowed = true;
                        }
                    } else if (strcmp("isArchive", fileEntry->string)==0) {
                        entry.isArchive = false;
                        if (fileEntry->type==cJSON2_True) {
                            entry.isArchive = true;
                        }
                    } else if (strcmp("isHidden", fileEntry->string)==0) {
                        entry.isHidden = false;
                        if (fileEntry->type==cJSON2_True) {
                            entry.isHidden = true;
                        }
                    } else if (strcmp("isReadOnly", fileEntry->string)==0) {
                        entry.isReadOnly = false;
                        if (fileEntry->type==cJSON2_True) {
                            entry.isReadOnly = true;
                        }
                    } else if (strcmp("isSystem", fileEntry->string)==0) {
                        entry.isSystem = false;
                        if (fileEntry->type==cJSON2_True) {
                            entry.isSystem = true;
                        }
                    }
                }
                fileEntry = fileEntry->next;
            }

            if (!entry.isShortcut &&
                (!entry.shortcut.path.empty() ||
                 !entry.shortcut.type.empty() ||
                 !entry.shortcut.args.empty()))
            {   // Verify that if this is not a shortcut, the shortcut fields are empty.
                LOG_ERROR("Not shortcut, shortcut fields should be empty (%s,%s,%s). Continuing.",
                          entry.shortcut.path.c_str(),
                          entry.shortcut.type.c_str(),
                          entry.shortcut.args.c_str());
            }

            fileList_out.push_back(entry);
        } else {
            LOG_ERROR("Array entry not an object. Continuing.");
            rv = -1;
        }
        currEntry = currEntry->next;
    }

    return rv;
}

static int parseRFSearchGetResponse(cJSON2* cjsonRFSearchGetObject,
                                    std::vector<FSTestRFSearchResult>& searchResults_out,
                                    u64& numberReturned_out,
                                    bool& searchInProgress_out)
{
    searchResults_out.clear();
    numberReturned_out = 0;
    searchInProgress_out = false;
    int rv = 0;
    int rc;
    bool searchResultsExist = false;
    bool numberReturnedExist = false;
    bool searchInProgressExist = false;

    if (cjsonRFSearchGetObject == NULL) {
        LOG_ERROR("No response");
        return -1;
    }
    if(cjsonRFSearchGetObject->prev != NULL) {
        LOG_ERROR("Unexpected prev value. Continuing.");
    }
    if(cjsonRFSearchGetObject->next != NULL) {
        LOG_ERROR("Unexpected next value. Continuing.");
    }

    if (cjsonRFSearchGetObject->type == cJSON2_Object) {
        cJSON2* currObject = cjsonRFSearchGetObject->child;
        if (currObject == NULL) {
            LOG_ERROR("Object returned has no children.");
            return -2;
        }
        if (currObject->prev != NULL) {
            LOG_ERROR("Unexpected prev value. Continuing.");
        }

        while (currObject != NULL) {
            if (currObject->type == cJSON2_Array) {
                if (strcmp("searchResults", currObject->string)==0) {
                    rc = parseRFSearchGetSearchResults(currObject, searchResults_out);
                    if (rc != 0) {
                        LOG_ERROR("parseRFSearchResults:%d", rc);
                        rv = rc;
                    }
                    searchResultsExist = true;
                }
            } else if (currObject->type == cJSON2_Number) {
                if (strcmp("numberReturned", currObject->string)==0) {
                    numberReturned_out = currObject->valueint;
                    numberReturnedExist = true;
                }
            } else if (currObject->type==cJSON2_True ||
                       currObject->type==cJSON2_False)
            {
                if (strcmp("searchInProgress", currObject->string)==0) {
                    searchInProgress_out = false;
                    if (currObject->type==cJSON2_True) {
                        searchInProgress_out = true;
                    }
                    searchInProgressExist = true;
                }
            }
            currObject = currObject->next;
        }
    }

    if (!searchResultsExist || !numberReturnedExist || !searchInProgressExist) {
        LOG_ERROR("Required arg missing (%d,%d,%d)", searchResultsExist,
                  numberReturnedExist, searchInProgressExist);
        rv = -1;
    }
    return rv;
}

int fs_test_search_get(u64 userId,
                       u64 datasetId,
                       u64 searchQueueId,
                       u64 startIndex,
                       u64 maxPageSize,
                       bool is_media_rf,
                       bool printResponse,
                       std::string& response_out,
                       std::vector<FSTestRFSearchResult>& searchResults_out,
                       u64& numberReturned_out,
                       bool& searchInProgress_out)
{
    int rv = 0;
    std::string datasetIdStr;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string queryString;
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
        std::ostringstream oss;
        oss << "?";
        oss << "searchQueueId=" << searchQueueId;
        oss << "&startIndex=" << startIndex;
        oss << "&maxPageSize=" << maxPageSize;

        queryString = oss.str();
    }

    {
        std::ostringstream oss;
        oss << datasetId;
        datasetIdStr = oss.str();
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqLength = VPLString_MallocAndSprintf("Content-Length: %d", 0);
        if (reqLength == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqLength);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqLength,
            reqHandle,
            reqTicket,
            "Content-Type: application/json",
        };
        url = base_url + getNameSpace(is_media_rf) + "/search/" + datasetIdStr + "/get" + queryString;

        rv = agent->post(url, headers, 5, NULL, 0, NULL, response_out);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s",
                      url.c_str(), rv, response_out.c_str());
            goto end;
        }
    }

    if (printResponse) {
        LOG_ALWAYS("SearchGet. request:(%s),queryString(%s),response(%s)",
                   url.c_str(), queryString.c_str(), response_out.c_str());
    }

    {
        int rc; // Error code rv already set, this is for display only.

        cJSON2* jsonResponse = cJSON2_Parse(response_out.c_str());
        if (jsonResponse == NULL) {
            LOG_ERROR("cJSON2_Parse error");
            if (rv==0) { rv = -1; }
            goto end;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, jsonResponse);

        rc = parseRFSearchGetResponse(jsonResponse,
                                      searchResults_out,
                                      numberReturned_out,
                                      searchInProgress_out);
        if (rc != 0) {
            LOG_ERROR("parseRFSearchGetResponse:%d", rc);
            if (rv==0) { rv = rc; }
            goto end;
        }

        if (printResponse) {
            int i = 0;
            for (std::vector<FSTestRFSearchResult>::iterator iter = searchResults_out.begin();
                 iter != searchResults_out.end(); ++iter)
            {
                std::cout << "   File" << ++i << ": " << iter->path.c_str()
                          << "     (" << (iter->isDir?"Dir":"File")
                          << ",modified:" << iter->lastChanged
                          << ",size:" << iter->size
                          << (iter->isReadOnly?",ReadOnly":"")
                          << (iter->isHidden?",Hidden":"")
                          << (iter->isSystem?",System":"")
                          << (iter->isArchive?",Archive":"")
                          << (iter->isAllowed?",Allowed":"")
                          << (iter->isShortcut?",Shortcut":"");
                if (iter->isShortcut) {
                    std::cout << "["  << iter->shortcut.path.c_str()
                              << ", " << iter->shortcut.type.c_str()
                              << ", " << iter->shortcut.args.c_str()
                              << "]";
                }
                std::cout << ")" << std::endl;
            }
            std::cout << "  numberReturned: " << numberReturned_out << std::endl;
            std::cout << "  searchInProgress: " << (searchInProgress_out?"true":"false") << std::endl;
        }
    }
end:
    return rv;
}

int fs_test_search_end(u64 userId,
                       u64 datasetId,
                       u64 searchQueueId,
                       bool is_media_rf,
                       std::string& response_out)
{
    int rv = 0;
    std::string base_url;
    std::string datasetIdStr;
    std::string service_ticket;
    std::string session_handle;
    std::string queryString;
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
        std::ostringstream oss;
        oss << datasetId;
        datasetIdStr = oss.str();
    }

    {
        std::ostringstream oss;
        oss << "?";
        oss << "searchQueueId=" << searchQueueId;

        queryString = oss.str();
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqLength = VPLString_MallocAndSprintf("Content-Length: %d", 0);
        if (reqLength == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqLength);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqLength,
            reqHandle,
            reqTicket,
            "Content-Type: application/json",
        };
        url = base_url + getNameSpace(is_media_rf) +
              "/search/" + datasetIdStr + "/end" + queryString;

        rv = agent->post(url, headers, 5, NULL, 0, NULL, response_out);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s",
                      url.c_str(), rv, response_out.c_str());
            goto end;
        }
    }
    LOG_ALWAYS("SearchEnd. request:(%s),queryString(%s),response(%s)",
               url.c_str(), queryString.c_str(), response_out.c_str());

end:
    return rv;
}

int fs_test_readdirmetadata(u64 userId,
                            const std::string &dataset,
                            const std::string &syncboxdataset,
                            const std::string &path,
                            std::string& response,
                            bool is_media_rf)
{
    int rv = 0;
    InfraHttpInfo infraHttpInfo;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string tempPath = path;
    std::map<std::string, std::string> parameters;
    std::string url;
    HttpAgent *agent = NULL;

    LOG_INFO("ReadDirMetadata: \"%s\" ", path.c_str());
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
        ON_BLOCK_EXIT(free, reqUserId);
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqHandle);
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        ON_BLOCK_EXIT(free, reqTicket);

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        tempPath = "[syncbox:"+syncboxdataset+"]/"+tempPath;

        url = base_url + getNameSpace(is_media_rf) + "/dirmetadata/" + dataset + "/" + VPLHttp_UrlEncoding(tempPath);

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        if (rv < 0) {
            LOG_ERROR("Request failed(%s). rv = %d. Response msg = %s", url.c_str(), rv, response.c_str());
            goto end;
        }
        LOG_ALWAYS("Read dirmetadata: %s", response.c_str());

        rv = check_json_format(response);
    }

end:
    return rv;
}

//========================================================
class NewFsTestDispatchTable {
public:
    NewFsTestDispatchTable() {
        cmds["AsyncUpload"]     = fs_test_async_upload;
        cmds["CancelAsyncReq"]  = fs_test_async_cancel;
        cmds["CopyDir"]         = fs_test_copydir;
        cmds["CopyFile"]        = fs_test_copyfile;
        cmds["CopyDirV2"]       = fs_test_copydir_v2;
        cmds["CopyFileV2"]      = fs_test_copyfile_v2;
        cmds["Download"]        = fs_test_download;
        cmds["DeleteDir"]       = fs_test_deletedir;
        cmds["DeleteFile"]      = fs_test_deletefile;
        cmds["EditTag"]         = fs_test_edittag;
        cmds["GetProgress"]     = fs_test_async_progress;
        cmds["Help"]            = fs_test_help_response;
        cmds["ListDatasets"]    = fs_test_listdatasets;
        cmds["ListDevices"]     = fs_test_listdevices;
        cmds["MakeDir"]         = fs_test_makedir;
        cmds["MoveFile"]        = fs_test_movefile;
        cmds["MoveFileV2"]      = fs_test_movefile_v2;
        cmds["ReadDir"]         = fs_test_readdir;
        cmds["ReadMetadata"]    = fs_test_readmetadata;
        cmds["RenameDir"]       = fs_test_renamedir;
        cmds["RenameDirV2"]     = fs_test_renamedir_v2;
        cmds["SetPermission"]   = fs_test_setpermission;
        cmds["Upload"]          = fs_test_upload;
        cmds["AccessControl"]   = fs_test_accesscontrol;
        cmds["ReadDirMetadata"] = fs_test_readdirmetadata;
        cmds["SearchBegin"]     = fs_test_search_begin;
        cmds["SearchGet"]       = fs_test_search_get;
        cmds["SearchEnd"]       = fs_test_search_end;
    }
    std::map<std::string, rf_subcmd_fn> cmds;
};

static NewFsTestDispatchTable newFsTestDispatchTable;

static int rf_dispatch(std::map<std::string, rf_subcmd_fn> &cmdmap, int argc, const char* argv[], std::string &response)
{
    int rv = 0;
    bool is_media_rf = false;

    if (argc <= 1) {
        return fs_test_help(argc, argv);
    }

    if (strcmp(argv[0], "MediaRF") == 0) {
        is_media_rf = true;
    }

    if (cmdmap.find(argv[1]) != cmdmap.end()) {
        rv = cmdmap[argv[1]](argc - 1, &argv[1], response, is_media_rf);
    } else {
        LOG_ERROR("Command %s not supported", argv[1]);
        rv = -1;
    }
    return rv;
}

int dispatch_fstest_cmd(int argc, const char* argv[])
{
    std::string response;
    return rf_dispatch(newFsTestDispatchTable.cmds, argc, &argv[0], response);
}

int dispatch_fstest_cmd_with_response(int argc, const char* argv[], std::string& response)
{
    return rf_dispatch(newFsTestDispatchTable.cmds, argc, &argv[0], response);
}
