//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.

#ifndef __FS_TEST_H__
#define __FS_TEST_H__

#include <vplu_types.h>

typedef int (*rf_subcmd_fn)(int argc, const char *argv[], std::string& response, bool is_media_rf);

int dispatch_fstest_cmd(int argc, const char* argv[]);

// Defined in dx_common.h
//int dispatch_fstest_cmd_with_response(int argc, const char* argv[], std::string& response);

int fs_test_download(u64 userId, const std::string &dataset, const std::string &path, const std::string &outputFile, const std::string &rangeHeader, std::string& response, bool is_media_rf=false);

int fs_test_download_by_ip_port(u64 userId, const std::string &dataset, const std::string &path, const std::string &outputFile, const std::string &rangeHeader, const std::string& ip_addr, const std::string& port, std::string& response, bool is_media_rf=false);

int fs_test_upload(u64 userId, const std::string &dataset, const std::string &path, const std::string &file, std::string& response, bool is_media_rf=false);

int fs_test_async_upload(u64 userId, const std::string &dataset, const std::string &path, const std::string &file, std::string& response, bool is_media_rf=false);

int fs_test_async_cancel(u64 userId, const std::string &async_handle, std::string& response, bool is_media_rf=false);

int fs_test_async_progress(u64 userId, const std::string &async_handle, std::string& response, bool is_media_rf=false);

int fs_test_readdir(u64 userId, const std::string &dataset, const std::string &path, const std::string &pagination, std::string& response, bool is_media_rf=false);

int fs_test_makedir(u64 userId, const std::string &dataset, const std::string &path, std::string& response, bool is_media_rf=false);

int fs_test_copydir(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_copyfile(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_renamedir(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_movefile(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_copydir_v2(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_copyfile_v2(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_renamedir_v2(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_movefile_v2(u64 userId, const std::string &dataset, const std::string &path, const std::string &newpath, std::string& response, bool is_media_rf=false);

int fs_test_deletedir(u64 userId, const std::string &dataset, const std::string &path, std::string& response, bool is_media_rf=false);

int fs_test_deletefile(u64 userId, const std::string &dataset, const std::string &path, std::string& response, bool is_media_rf=false);

int fs_test_readmetadata(u64 userId, const std::string &dataset, const std::string &path, std::string& response, bool is_media_rf=false);

int fs_test_setpermission(u64 userId, const std::string &dataset, const std::string &path, const std::string &permissions, std::string& response, bool is_media_rf=false);

int fs_test_edittag(u64 userId, const std::string &objectid, const std::string &deviceid, const std::string &tagNameValueStr, std::string& response, bool is_media_rf=false);

int fs_test_listdatasets(u64 userId, std::string& response, bool is_media_rf=false);

int fs_test_listdevices(u64 userId, std::string& response, bool is_media_rf=false);

int fs_test_accesscontrol(u64 userId, const std::string &action, const std::string &rule, const std::string &user, const std::string &path, std::string& response, bool is_media_rf=false);

int fs_test_accesscontrol(u64 userId, const std::string &dataset, const std::string &action, std::string& response, bool is_media_rf=false);

int fs_test_readdirmetadata(u64 userId,
                            const std::string &dataset,
                            const std::string &path,
                            const std::string &syncboxdataset,
                            std::string& response,
                            bool is_media_rf);

struct FSTestRFSearchShortcutDetails {
    std::string path;
    std::string type;
    std::string args;
};

struct FSTestRFSearchResult {
    std::string path;
    bool isDir;
    VPLTime_t lastChanged;
    u64 size;
    bool isReadOnly;
    bool isHidden;
    bool isSystem;
    bool isArchive;
    bool isAllowed;

    bool isShortcut;
    FSTestRFSearchShortcutDetails shortcut;

    FSTestRFSearchResult()
    :   isDir(false),
        lastChanged(VPLTIME_INVALID),
        size(0),
        isReadOnly(false),
        isHidden(false),
        isSystem(false),
        isArchive(false),
        isAllowed(false),
        isShortcut(false)
    {}
};

int fs_test_search_begin(u64 userId,
                         u64 datasetId,
                         const std::string& path,
                         const std::string& searchString,
                         bool disableIndex,
                         bool recursive,
                         bool is_media_rf,
                         bool printResponse,
                         std::string& response_out,
                         u64& searchQueueId_out);

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
                       bool& searchInProgress_out);

int fs_test_search_end(u64 userId,
                       u64 datasetId,
                       u64 searchQueueId,
                       bool is_media_rf,
                       std::string& response_out);

#endif // __FS_TEST_H__
