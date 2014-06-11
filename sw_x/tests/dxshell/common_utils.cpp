//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "gvm_file_utils.hpp"
#include "vpl_fs.h"
#include "vplex_file.h"
#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>
#include <set>
#include <deque>
#include <vplex_time.h>
#include <log.h>

#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include "media_metadata_errors.hpp"

LOGLevel g_defaultLogLevel = LOG_LEVEL_ERROR;

#define DX_VERBOSE_ENV          "DX_VERBOSE"

int setDebugLevel(int level)
{
    for (int i = 0; i < LOG_NUM_LEVELS; i++) {
        if (i >= level) { 
            LOG_ENABLE_LEVEL((LOGLevel)i);
        } else {
            LOG_DISABLE_LEVEL((LOGLevel)i);
        }
    }

    return 0;
}

void resetDebugLevel()
{
#if defined(VPL_PLAT_IS_WINRT)
    setDebugLevel(g_defaultLogLevel);
#else
    if (getenv(DX_VERBOSE_ENV) != NULL) {
        setDebugLevel(LOG_LEVEL_DEBUG);
    } else {
        setDebugLevel(g_defaultLogLevel);
    }
#endif
}

int waitCcd()
{
    LOG_INFO("start");
    VPLTime_t timeout = VPLTIME_FROM_SEC(CCD_TIMEOUT);
    VPLTime_t endTime = VPLTime_GetTimeStamp() + timeout;

    ccd::GetSystemStateInput request;
    request.set_get_players(true);
    ccd::GetSystemStateOutput response;
    int rv;
    while(1) {
        rv = CCDIGetSystemState(request, response);
        if (rv == CCD_OK) {
            LOG_INFO("CCD is ready!\n");
            break;
        }
#if 0
        if (rv != IPC_ERROR_SOCKET_CONNECT) {
            LOG_ERROR("Unexpected error: %d", rv);
            break;
        }
#endif
        LOG_INFO("Still waiting for CCD to be ready...\n");
        if (VPLTime_GetTimeStamp() >= endTime) {
            LOG_ERROR("Timed out waiting for CCD after "FMTu64"ms\n", VPLTIME_TO_MILLISEC(timeout));
            break;
        }
        VPLThread_Sleep(VPLTIME_FROM_SEC(1));
    }

    return (rv == 0) ? 0 : -1;
}

int dispatch(const std::map<std::string, subcmd_fn>& cmdmap, int argc, const char* argv[])
{
    int rv = 0;

    if (cmdmap.find(argv[0]) != cmdmap.end()) {
        subcmd_fn func = cmdmap.find(argv[0])->second; // C++11: cmdmap.at(argv[0]);
        rv = func(argc, &argv[0]);
    } else {
        LOG_ERROR("Command %s not supported", argv[0]);
        rv = -1;
    }
    return rv;
}

int dispatch2ndLevel(const std::map<std::string, subcmd_fn>& cmdMap, int argc, const char* argv[])
{
    int rv = 0;
    if(argc < 2 || checkHelp(argc, argv)) {
        printHelpFromCmdMap(cmdMap);
        return rv;
    }
    if(cmdMap.find(argv[1]) != cmdMap.end()) {
        subcmd_fn func = cmdMap.find(argv[1])->second; // C++11: cmdmap.at(argv[1]);
        rv = func(argc-1, &argv[1]);
    } else {
        LOG_ERROR("Command %s %s not supported", argv[0], argv[1]);
        rv = -1;
    }
    return rv;
}

void printHelpFromCmdMap(const std::map<std::string, subcmd_fn>& cmdMap)
{
    std::map<std::string, subcmd_fn>::const_iterator it;
    for (it = cmdMap.begin(); it != cmdMap.end(); ++it) {
        const char *argv[2];
        argv[0] = it->first.c_str();
        argv[1] = "Help";
        it->second(2, argv);
    }
}

bool checkHelp(int argc, const char* argv[])
{
    if (argc > 1) {
        if ((strncmp(argv[1], "Help", sizeof("Help")) == 0) ||
            (strncmp(argv[1], "help", sizeof("help")) == 0)) {
            return true;
        }
    }
    return false;
}

bool checkHelpAndExactArgc(int argc, const char* argv[], int expected_argc, int& rv_out)
{
    rv_out = 0;
    bool requestedHelp = checkHelp(argc, argv);
    if(!requestedHelp && (argc != expected_argc)) {
        LOG_ERROR("Wrong number of arguments");
        rv_out = -1;
    }
    if (requestedHelp || (argc != expected_argc)) {
        return true;
    }
    return false;
}

bool checkHelpAndMinArgc(int argc, const char* argv[], int expected_argc, int& rv_out)
{
    rv_out = 0;
    bool requestedHelp = checkHelp(argc, argv);
    if(!requestedHelp && (argc < expected_argc)) {
        LOG_ERROR("Too few arguments");
        rv_out = -1;
    }
    if (requestedHelp || (argc < expected_argc)) {
        return true;
    }
    return false;
}

// returns true iff char at "pos" is the first non-blank char on the line
bool isHeadOfLine(const std::string &lines, size_t pos)
{
    if (pos == 0)
        return true;
    pos--;
    while (1) {
        if (pos == 0)
            return true;
        if (lines[pos] == '\r' || lines[pos] == '\n')
            return true;
        if (lines[pos] != ' ' && lines[pos] != '\t')
            return false;
        pos--;
    }
    return true;
}

// returns true iff first non-blank char starting at "pos" is '='
bool isFollowedByEqual(const std::string &lines, size_t pos)
{
    while (pos < lines.length()) {
        if (lines[pos] == '=')
            return true;
        if (lines[pos] != ' ' && lines[pos] != '\t')
            return false;
        pos++;
    }
    return false;
}

// returns the position one char beyond the last char on the line
size_t findLineEnd(const std::string &lines, size_t pos)
{
    while (pos < lines.length()) {
        if (lines[pos] == '\r' || lines[pos] == '\n')
            return pos;
        pos++;
    }
    return pos;
}


void updateConfig(std::string &config, const std::string &key, const std::string &value)
{
    bool modified = false;
    size_t keypos = 0;
    size_t nextpos = 0;
    // TODO: need to make it case-insensitive comparison
    while ((keypos = config.find(key, nextpos)) != config.npos) {

        bool isHead = isHeadOfLine(config, keypos);
        if (!isHead) {
            nextpos = keypos + 1;
            continue;
        }

        bool isEqual = isFollowedByEqual(config, keypos + key.length());
        if (!isEqual) {
            nextpos = keypos + 1;
            continue;
        }

        std::string replacement = " = " + value;
        config.replace(keypos + key.length(), findLineEnd(config, keypos) - keypos - key.length(), replacement);
        modified = true;
        nextpos = keypos + 1;
    }

    if (!modified) {
        config += "\n" + key + " = " + value + "\n";
    }
}

int file_compare(const char* src, const char* dst) {

    const int BUF_SIZE = 16 * 1024;

    int rv = 0;
    VPLFS_stat_t stat_src, stat_dst;
    VPLFile_handle_t handle_src = 0, handle_dst = 0;
    char *src_buf = NULL;
    char *dst_buf = NULL;
    ssize_t src_read, dst_read;

    if (src == NULL || dst == NULL) {
        LOG_ERROR("file path is NULL");
        return -1;
    }
    LOG_ALWAYS("comparing %s with %s", src, dst);
    if (VPLFS_Stat(src, &stat_src)) {
        LOG_ERROR("fail to stat src file: %s", src);
        return -1;
    }
    if (VPLFS_Stat(dst, &stat_dst)) {
        LOG_ERROR("fail to stat dst file: %s", dst);
        return -1;
    }
    if (stat_src.type != VPLFS_TYPE_FILE || stat_dst.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("unable to compare non-file, src-type = %d, dst-type=%d",
                  stat_src.type, stat_dst.type);
        return -1;
    }
    // not equal
    if (stat_src.size != stat_dst.size) {
        LOG_ERROR("file size doesn't match: src = "FMTu64", dst = "FMTu64,
                  stat_src.size, stat_dst.size);
        return -1;
    }
    // open file
    handle_src = VPLFile_Open(src, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_src)) {
        LOG_ERROR("cannot open src file: %s", src);
        rv = -1;
        goto exit;
    }
    handle_dst = VPLFile_Open(dst, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_dst)) {
        LOG_ERROR("cannot open dst file: %s", dst);
        rv = -1;
        goto exit;
    }
    src_buf = new char[BUF_SIZE];
    if (src_buf == NULL) {
        LOG_ERROR("cannot allocate memory for src buffer");
        rv = -1;
        goto exit;
    }
    dst_buf = new char[BUF_SIZE];
    if (dst_buf == NULL) {
        LOG_ERROR("cannot allocate memory for dst buffer");
        rv = -1;
        goto exit;
    }
    do {
        src_read = VPLFile_Read(handle_src, src_buf, BUF_SIZE);
        dst_read = VPLFile_Read(handle_dst, dst_buf, BUF_SIZE);
        if (src_read < 0) {
            LOG_ERROR("error while reading src: %s, "FMTu_size_t"", src, src_read);
            rv = -1;
            break;
        }
        if (dst_read < 0) {
            LOG_ERROR("error while reading dst: %s, "FMTu_size_t"", dst, dst_read);
            rv = -1;
            break;
        }
        if (src_read != dst_read || memcmp(src_buf, dst_buf, src_read)) {
            LOG_ERROR("file is different");
            rv = -1;
            break;
        }
    } while (src_read == BUF_SIZE);

exit:
    if (VPLFile_IsValidHandle(handle_src)) {
        VPLFile_Close(handle_src);
    }
    if (VPLFile_IsValidHandle(handle_dst)) {
        VPLFile_Close(handle_dst);
    }
    if (src_buf != NULL) {
        delete src_buf;
        src_buf = NULL;
    }
    if (dst_buf != NULL) {
        delete dst_buf;
        dst_buf = NULL;
    }
    return rv;
}

int file_copy(const char* src, const char* dst)
{

    const int BUF_SIZE = 16 * 1024;

    int rv = 0;
    VPLFS_stat_t stat_src;
    VPLFile_handle_t handle_src = 0, handle_dst = 0;
    char *src_buf = NULL;
    ssize_t src_read, dst_write;
    VPLFile_offset_t offset;

    if (src == NULL || dst == NULL) {
        LOG_ERROR("file path is NULL");
        return -1;
    }
    if (strcmp(src, dst) == 0) {
        LOG_ALWAYS("source and target path are the same");
        return 0;
    }
    LOG_ALWAYS("Copy from %s to %s", src, dst);
    if (VPLFS_Stat(src, &stat_src)) {
        LOG_ERROR("fail to stat src file: %s", src);
        return -1;
    }
    if (stat_src.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("unable to copy non-file: %s, type = %d", src, stat_src.type);
        return -1;
    }
    if (strcmp(src, dst) == 0) {
        LOG_ALWAYS("source and target path are the same");
        return 0;
    }
    // open file
    handle_src = VPLFile_Open(src, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_src)) {
        LOG_ERROR("cannot open src file: %s", src);
        rv = -1;
        goto exit;
    }
    handle_dst = VPLFile_Open(dst, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
    if (!VPLFile_IsValidHandle(handle_dst)) {
        LOG_ERROR("cannot open dst file: %s", dst);
        rv = -1;
        goto exit;
    }
    src_buf = new char[BUF_SIZE];
    if (src_buf == NULL) {
        LOG_ERROR("cannot allocate memory for src buffer");
        rv = -1;
        goto exit;
    }
    offset = 0;
    do {
        src_read = VPLFile_Read(handle_src, src_buf, BUF_SIZE);
        if (src_read < 0) {
            LOG_ERROR("unable to read file: "FMTu_size_t"", src_read);
            rv = -1;
            break;
        }
        dst_write = VPLFile_Write(handle_dst, src_buf, src_read);
        if (dst_write < 0) {
            LOG_ERROR("unable to write file: "FMTu_size_t"", dst_write);
            rv = -1;
            break;
        }
        if (src_read != dst_write) {
            LOG_ERROR("src_read("FMTu_size_t") != dst_write("FMTu_size_t")", src_read, dst_write);
            rv = -1;
            break;
        }
        offset += src_read;
    } while (src_read == BUF_SIZE);

    LOG_DEBUG("Total length: "FMTu64, offset);
exit:
    if (VPLFile_IsValidHandle(handle_src)) {
        VPLFile_Close(handle_src);
    }
    if (VPLFile_IsValidHandle(handle_dst)) {
        VPLFile_Close(handle_dst);
        if (rv) {
            int srv;
            // error happens while copying files. remove it.
            srv = VPLFile_Delete(dst);
            if (srv) {
                LOG_ERROR("Failed to cleanup file %s, %d", dst, srv);
            }
        }
    }
    if (src_buf != NULL) {
        delete src_buf;
        src_buf = NULL;
    }
    return rv;
}

bool isWindows(const std::string &os)
{
    if (os.find(OS_WINDOWS) != std::string::npos &&
        os.find(OS_WINDOWS_RT) == std::string::npos) {
        return true;
    }
    return false;
}



u8 toHex(const u8 &x)
{
    return x > 9 ? x + 55: x + 48;
}

char toLower(char in)
{
  if(in<='Z' && in>='A')
    return in-('Z'-'z');
  return in;
}

// PRODUCT SPEC OF SUPPORTED FILES: 12-17-2012
// (Case insentive)
// JPEG (.jpg, .jpeg)
// TIFF (.tif, .tiff)
// PNG (.png)
// BMP (.bmp)
bool isPhotoFile(const std::string& dirent)
{
    std::string ext;
    std::string toCompare;
    static const char* supported_exts[] = {
        ".jpg",
        ".jpeg",
        ".png",
        ".tif",
        ".tiff",
        ".bmp"
    };

    for (unsigned int i = 0; i < ARRAY_ELEMENT_COUNT(supported_exts); i++) {
        ext.assign(supported_exts[i]);
        if (dirent.size() >= ext.size()) {
            toCompare = dirent.substr(dirent.size() - ext.size(), ext.size());
            for (unsigned int i = 0; i < toCompare.size(); i++) {
                toCompare[i] = toLower(toCompare[i]);
            }
            if (toCompare == ext) {
                return true;
            }
        }
    }

    return false;
}

bool isMusicFile(const std::string& dirent)
{
    std::string ext;
    std::string toCompare;
    static const char* supported_exts[] = {
        ".mp3",
        ".wma"
    };

    for (unsigned int i = 0; i < ARRAY_ELEMENT_COUNT(supported_exts); i++) {
        ext.assign(supported_exts[i]);
        if (dirent.size() >= ext.size()) {
            toCompare = dirent.substr(dirent.size() - ext.size(), ext.size());
            for (unsigned int i = 0; i < toCompare.size(); i++) {
                toCompare[i] = toLower(toCompare[i]);
            }
            if (toCompare == ext) {
                return true;
            }
        }
    }

    return false;
}

int getUserIdAndClearFiDatasetId(u64& userId, u64& datasetId)
{
    int rv = 0;
    userId=0;
    datasetId=0;
    {
        ccd::GetSystemStateInput ccdiRequest;
        ccdiRequest.set_get_players(true);
        ccd::GetSystemStateOutput ccdiResponse;
        rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
            goto out;
        }
        userId = ccdiResponse.players().players(0).user_id();
        if (userId == 0) {
            LOG_ERROR("Not signed-in!");
            rv = MM_ERR_NOT_SIGNED_IN;
            goto out;
        }
    }
    {
        ccd::GetSyncStateInput ccdiRequest;
        ccdiRequest.set_user_id(userId);
        ccdiRequest.set_only_use_cache(true);
        ccd::GetSyncStateOutput ccdiResponse;
        rv = CCDIGetSyncState(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "CCDIGetSyncState", rv);
            goto out;
        }
        if (!ccdiResponse.is_device_linked()) {
            // TODO: autolink?
            LOG_ERROR("Device is not linked");
            rv = MM_ERR_FAIL;
            goto out;
        }
        if (!ccdiResponse.is_sync_agent_enabled()) {
            // TODO: enable it?
            LOG_WARN("Sync agent is not enabled");
        }
    }

    {
        ccd::ListOwnedDatasetsInput ccdiRequest;
        ccd::ListOwnedDatasetsOutput ccdiResponse;
        ccdiRequest.set_only_use_cache(true);
        ccdiRequest.set_user_id(userId);
        // Will possibly make an infra call if the dataset list is dirty.
        rv = CCDIListOwnedDatasets(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "CCDIListOwnedDatasets", rv);
            goto out;
        }
        rv = MM_ERR_NO_DATASET;
        for (int i = 0; i < ccdiResponse.dataset_details_size(); i++) {
            const vplex::vsDirectory::DatasetDetail& iterDset = ccdiResponse.dataset_details(i);
            if (iterDset.datasettype() == vplex::vsDirectory::USER_CONTENT_METADATA &&
                (iterDset.datasetname() == std::string("Media MetaData VCS")))
            {
                datasetId = iterDset.datasetid();
                rv = 0;
                break;
            }
        }
        if(rv != 0) {
            LOG_ERROR("Cannot find Media MetaData dataset.  Dxshell out-of-date?");
        }
    }
    LOG_INFO("MCA UserId:"FMTu64" Media MetaData Dataset:"FMTu64, userId, datasetId);
 out:
    return rv;
}

const std::string get_extension(const std::string &path)
{
    std::string ext;
    size_t pos = path.find_last_of("/.");
    if ((pos != path.npos) && (path[pos] == '.')) {
        ext.assign(path, pos + 1, path.size());
    }
    return ext;
}

void rmTrailingSlashes(const std::string& path_in, std::string& path_out)
{
    path_out = path_in;
    // Remove trailing
    while(path_out.size() > 0 && path_out.rfind('/') == path_out.size()-1) {
        path_out = path_out.substr(0, path_out.size()-1);
    }
    while(path_out.size() > 0 && path_out.rfind('\\') == path_out.size()-1) {
        path_out = path_out.substr(0, path_out.size()-1);
    }
}

void trimSlashes(const std::string& path_in, std::string& path_out)
{
    rmTrailingSlashes(path_in, path_out);
    // Remove leading
    while(path_out.find('/') == 0) {
        path_out = path_out.substr(1, path_out.size()-1);
    }
    while(path_out.find('\\') == 0) {
        path_out = path_out.substr(1, path_out.size()-1);
    }
}

void splitAbsPath(const std::string& absPath, std::string& path_out, std::string& name_out)
{
    path_out = "";
    name_out = "";

    std::string tempPath;
    trimSlashes(absPath, tempPath);

    ssize_t index;
    ssize_t indexLinux = tempPath.rfind('/');
    ssize_t indexWin32 = tempPath.rfind('\\');

    if(indexLinux!=std::string::npos && indexWin32!=std::string::npos) {
        if(indexLinux>=indexWin32) {
            index = indexLinux;
        }else{
            index = indexWin32;
        }
    }else if(indexLinux!=std::string::npos) {
        index = indexLinux;
    }else if(indexWin32!=std::string::npos) {
        index = indexWin32;
    }else {
        index = std::string::npos;
    }

    if(index != std::string::npos) {
        path_out = tempPath.substr(0, index);
        name_out = tempPath.substr(index+1, tempPath.size()-index);
    }else{
        name_out = tempPath;
    }
}


#ifdef ENABLE_WIN32_DELETE_BUG_2977_CHECK
#include "vpl_th.h"
#include "vpl_time.h"
static int checkForWin32DeleteBug2977(int line, const std::string& path)
{
    VPLFS_stat_t statBuf;
    int rc = VPLFS_Stat(path.c_str(), &statBuf);
    if(rc == VPL_ERR_ACCESS) {
        LOG_WARN("[Win32SlowDeleteBug2977] potential line %d, path %s, Sleeping for 10 ms.",
                 line, path.c_str());
    } else if(rc != VPL_ERR_NOENT) {
        LOG_ERROR("Delete failed at line %d, path %s, %d", line, path.c_str(), rc);
        return rc;
    } else {
        return 0;
    }

    VPLThread_Sleep(VPLTime_FromMillisec(50));

    rc = VPLFS_Stat(path.c_str(), &statBuf);
    if(rc == VPL_ERR_NOENT) {
        LOG_WARN("[Win32SlowDeleteBug2977] Confirmed issue, line %d, path %s", line, path.c_str());
        return 0;
    } else if(rc == VPL_ERR_ACCESS) {
        LOG_WARN("[Win32SlowDeleteBug2977] Slept 50ms. Might be actual permission issue?  line:%d, path:%s",
                 line, path.c_str());
        return rc;
    } else {
        LOG_ERROR("Something has changed, line:%d, path:%s, %d",
                  line, path.c_str(), rc);
        return rc;
    }
}
#endif

int Util_rm_dash_rf(const std::string& path)
{
    int rv = 0;
    int rc = 0;
    VPLFS_stat_t statBufOne;
    rc = VPLFS_Stat(path.c_str(), &statBufOne);
    if(rc != VPL_OK) {
        LOG_WARN("removing path %s that does not exist.%d. Count as success.",
                 path.c_str(), rc);
        return 0;
    }

    std::vector<std::string> dirPaths;
    if(statBufOne.type == VPLFS_TYPE_FILE) {
       rc = VPLFile_Delete(path.c_str());
       if(rc != VPL_OK) {
           LOG_ERROR("File unlink unsuccessful: %s, (%d)",
                     path.c_str(), rc);
       }
       return rc;
    } else {
        dirPaths.push_back(path);
    }

    std::set<std::string> errorDirs;

    while(!dirPaths.empty()) {
        VPLFS_dir_t dp;
        VPLFS_dirent_t dirp;
        std::string currDir(dirPaths.back());

        if((rc = VPLFS_Opendir(currDir.c_str(), &dp)) != VPL_OK) {
            LOG_ERROR("Error(%d) opening %s",
                      rc, currDir.c_str());
            dirPaths.pop_back();
            continue;
        }

        int dirAdded = 0;
        while(VPLFS_Readdir(&dp, &dirp) == VPL_OK) {
            std::string dirent(dirp.filename);
            std::string absFile;
            Util_appendToAbsPath(currDir, dirent, absFile);
            VPLFS_stat_t statBuf;

            if(dirent == "." || dirent == "..") {
                continue;
            }

            if((rc = VPLFS_Stat(absFile.c_str(), &statBuf)) != VPL_OK) {
                LOG_ERROR("Error(%d) using stat on (%s,%s), type:%d",
                          rc, currDir.c_str(), dirent.c_str(), (int)dirp.type);
                continue;
            }
            if(statBuf.type == VPLFS_TYPE_FILE) {
                rc = VPLFile_Delete(absFile.c_str());
                if(rc != VPL_OK) {
                    LOG_ERROR("File unlink unsuccessful: %s, (%d)",
                              absFile.c_str(), rc);
                    rv = rc;
                    continue;
                }
#ifdef ENABLE_WIN32_DELETE_BUG_2977_CHECK
                checkForWin32DeleteBug2977(__LINE__, absFile.c_str());
#endif
            } else {
                std::string toPushBack(absFile);
                std::set<std::string>::iterator isError = errorDirs.find(toPushBack);
                if(isError == errorDirs.end()) {
                    // This directory did not have an error deleting, safe to traverse.
                    dirAdded++;
                    dirPaths.push_back(toPushBack);
                }
            }
        }

        VPLFS_Closedir(&dp);

        if(dirAdded == 0) {
            rc = VPLDir_Delete(currDir.c_str());
            if(rc != VPL_OK) {
                LOG_ERROR("rmdir unsuccessful: %s, (%d)",
                          currDir.c_str(), rc);
                errorDirs.insert(currDir);
                rv = rc;
            }
#ifdef ENABLE_WIN32_DELETE_BUG_2977_CHECK
            checkForWin32DeleteBug2977(__LINE__, currDir.c_str());
#endif
            dirPaths.pop_back();
        }
    }
    return rv;
}

void countPhotoNum(const std::string& cameraRollPath, u32& numPhotos)
{
    numPhotos = 0;
    int rc;
    VPLFS_dir_t dir_folder;
    std::deque<std::string> localDirs;
    localDirs.push_back(cameraRollPath);

    while(!localDirs.empty()) {
        std::string dirPath(localDirs.front());
        localDirs.pop_front();

        rc = VPLFS_Opendir(dirPath.c_str(), &dir_folder);
        if(rc == VPL_ERR_NOENT){
            // no photos
            continue;
        }else if(rc != VPL_OK) {
            LOG_ERROR("Unable to open %s:%d", dirPath.c_str(), rc);
            continue;
        }

        VPLFS_dirent_t folderDirent;
        while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        {
            std::string dirent(folderDirent.filename);
            if(folderDirent.type != VPLFS_TYPE_FILE) {
                if(dirent=="." || dirent==".." || dirent==".sync_temp" || dirent=="thumb") {
                    // We want to skip these directories
                }else{
                    // save directory for later processing
                    std::string deeperDir = dirPath+"/"+dirent;
                    localDirs.push_back(deeperDir);
                }
                continue;
            }

            if(!isPhotoFile(dirent)) {
                continue;
            }
            numPhotos++;
        }

        rc = VPLFS_Closedir(&dir_folder);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing %s:%d", dirPath.c_str(), rc);
        }
    }
}

void countMusicNum(const std::string& albumPath, u32& numMusic)
{
    numMusic = 0;
    int rc;
    VPLFS_dir_t dir_folder;
    std::deque<std::string> localDirs;
    localDirs.push_back(albumPath);

    while(!localDirs.empty()) {
        std::string dirPath(localDirs.front());
        localDirs.pop_front();

        rc = VPLFS_Opendir(dirPath.c_str(), &dir_folder);
        if(rc == VPL_ERR_NOENT){
            // no photos
            continue;
        }else if(rc != VPL_OK) {
            LOG_ERROR("Unable to open %s:%d", dirPath.c_str(), rc);
            continue;
        }

        VPLFS_dirent_t folderDirent;
        while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        {
            std::string dirent(folderDirent.filename);
            if(folderDirent.type != VPLFS_TYPE_FILE) {
                if(dirent=="." || dirent==".." || dirent==".sync_temp") {
                    // We want to skip these directories
                }else{
                    // save directory for later processing
                    std::string deeperDir = dirPath+"/"+dirent;
                    localDirs.push_back(deeperDir);
                }
                continue;
            }

            if(!isMusicFile(dirent)) {
                continue;
            }
            numMusic++;
        }

        rc = VPLFS_Closedir(&dir_folder);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing %s:%d", dirPath.c_str(), rc);
        }
    }
}

