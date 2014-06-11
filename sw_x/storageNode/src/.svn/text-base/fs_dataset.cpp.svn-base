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

#include "fs_dataset.hpp"

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <utime.h>
#endif

#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include <set>
#include <stack>
#include <algorithm>

#include "vpl_conv.h"
#include "vpl_fs.h"
#include "vplex_assert.h"
#include "vplex_file.h"
#include "vplex_trace.h"
#include "vplu_mutex_autolock.hpp"
#include "vssi_types.h"
#include "gvm_file_utils.h"
#include "gvm_file_utils.hpp"
#include "scopeguard.hpp"

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
#include "vpl__impersonate.hpp"
#endif

#include "vss_comm.h"

#include "HttpSvc_Sn_Handler_rf.hpp"

using namespace std;

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
static int get_shortcut_detail(const std::string& path, std::string& target_path, std::string& target_type, std::string& target_args)
{
    int rv = VSSI_SUCCESS;

    WIN32_IMPERSONATION;

    // Bug 10824: get shortcut detail if it is a file with .lnk extension
    rv = _VPLFS__GetShortcutDetail(path, target_path, target_type, target_args);
    if (rv != VPL_OK) {
        // shortcut cannot be parsed correctly, ignore it
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "unable to parse shortcut: %d", rv);
    }
    else if (target_path.find_first_of(':') == 1) {
        // converting <drive letter>:\... to Computer/<drive letter>/...
        target_path.erase(target_path.find_first_of(':'), 1);
        std::replace(target_path.begin(), target_path.end(), '\\', '/');
        target_path = "Computer/" + target_path;
        // Bug 13322: When the target path is Computer/D/, CCD needs to remove the last /
        if (target_path.find_last_of('/') == target_path.length() - 1) {
            target_path = target_path.substr(0, target_path.length() - 1);
        }
    }

    return rv;
}

void fs_dataset::refreshPrefixRewriteRulesHelper(
                  std::set<std::string>& winValidVirtualFolders_out,
                  std::map<std::string, std::string>& prefixRewriteRules_out,
                  std::map<std::string, _VPLFS__LibInfo>& winLibFolders_out)
{
    winValidVirtualFolders_out.clear();
    prefixRewriteRules_out.clear();
    winLibFolders_out.clear();

    winValidVirtualFolders_out.insert("");
    winValidVirtualFolders_out.insert("/");
    winValidVirtualFolders_out.insert("Libraries");
    winValidVirtualFolders_out.insert("Computer");
    winValidVirtualFolders_out.insert("Desktop");
    winValidVirtualFolders_out.insert("Public_Desktop");
    winValidVirtualFolders_out.insert("[LOCALAPPDATA]");

    std::map<std::string, _VPLFS__DriveType> driveMap;
    if (_VPLFS__GetComputerDrives(driveMap) == VPL_OK) {
        std::map<std::string, _VPLFS__DriveType>::const_iterator it;
        for(it = driveMap.begin(); it != driveMap.end(); it++) {
            prefixRewriteRules_out["Computer/" + it->first.substr(0, 1)] = it->first + "/";
        }
    }

    // replace the [LOCALAPPDATA] w/ the real one
    char *path = NULL;
    _VPLFS__GetLocalAppDataPath(&path);
    if (path != NULL) {
        prefixRewriteRules_out["[LOCALAPPDATA]"] = path;
        free(path);
        path = NULL;
    }

    char *librariesPath = NULL;

    if (_VPLFS__GetLibrariesPath(&librariesPath) == VPL_OK) {
        ON_BLOCK_EXIT(free, librariesPath);

        VPLFS_dir_t dir;
        VPLFS_dirent_t dirent;
        if (VPLFS_Opendir(librariesPath, &dir) == VPL_OK) {
            while (VPLFS_Readdir(&dir, &dirent) == VPL_OK) {

                char *p = strstr(dirent.filename, ".library-ms");
                if (p != NULL && p[strlen(".library-ms")] == '\0') {
                    // found library description file
                    std::string libDescFilePath;
                    libDescFilePath.assign(librariesPath).append("/").append(dirent.filename);

                    // grab both localized and non-localized name of the library folders
                    _VPLFS__LibInfo libinfo;
                    _VPLFS__GetLibraryFolders(libDescFilePath.c_str(), &libinfo);

                    // assigning the virtual folder and re-writing rules
                    std::map<std::string, _VPLFS__LibFolderInfo>::const_iterator it;
                    for (it = libinfo.m.begin(); it != libinfo.m.end(); it++) {
                        winValidVirtualFolders_out.insert("Libraries/"+libinfo.n_name);
                        winValidVirtualFolders_out.insert("Libraries/"+libinfo.l_name);
                        prefixRewriteRules_out["Libraries/"+libinfo.n_name+"/"+it->second.n_name] = it->second.path;
                        prefixRewriteRules_out["Libraries/"+libinfo.n_name+"/"+it->second.l_name] = it->second.path;
                        prefixRewriteRules_out["Libraries/"+libinfo.l_name+"/"+it->second.n_name] = it->second.path;
                        prefixRewriteRules_out["Libraries/"+libinfo.l_name+"/"+it->second.l_name] = it->second.path;
                    }
                    winLibFolders_out[libinfo.n_name] = libinfo;
                }
            }
            VPLFS_Closedir(&dir);
        }
    }

    char *desktopPath = NULL;
    
    _VPLFS__GetDesktopPath(&desktopPath);
    if (desktopPath != NULL) {
        prefixRewriteRules_out["Desktop"] = desktopPath;
        free(desktopPath);
        desktopPath = NULL;
    }

    char *publicDesktopPath = NULL;

    _VPLFS__GetPublicDesktopPath(&publicDesktopPath);
    if (publicDesktopPath != NULL) {
        prefixRewriteRules_out["Public_Desktop"] = publicDesktopPath;
        free(publicDesktopPath);
        publicDesktopPath = NULL;
    }
}
#endif // defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

void fs_dataset::refreshPrefixRewriteRules(VPLTime_t refreshThreshold)
{
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    ASSERT(VPLMutex_LockedSelf(&prefix_rewrite_mutex));
    // don't refresh if the last refresh was no earlier than refreshThreshold microsecs
    // allow first time (e.g. winLastRefreshTimeStamp is 0)
    if (VPLTime_GetTimeStamp() < winLastRefreshTimeStamp + refreshThreshold && winLastRefreshTimeStamp != 0)
        return;

    refreshPrefixRewriteRulesHelper(winValidVirtualFolders,
                                    prefixRewriteRules,
                                    winLibFolders);

    winLastRefreshTimeStamp = VPLTime_GetTimeStamp();
#else
    // No need to refresh.
    // The mapping is constant and defined in the constructor fs_dataset().
#endif
}

fs_dataset::fs_dataset(dataset_id& id, int type, vss_server* server) :
    dataset(id, type, server)
#ifdef WIN32
    , winLastRefreshTimeStamp(0)
#endif
{
    VPLMutex_Init(&fd_mutex);
    VPLMutex_Init(&prefix_rewrite_mutex);

#ifndef WIN32
    prefixRewriteRules[""] = "/";
#endif

}

fs_dataset::~fs_dataset()
{ 
    close_component_fds();

    VPLMutex_Destroy(&fd_mutex);
    VPLMutex_Destroy(&prefix_rewrite_mutex);
}


u64 fs_dataset::get_version()
{
    return 0;
}

bool fs_dataset::has_transactional_support()
{
    return false;
}

bool fs_dataset::component_is_directory(const std::string &name)
{
    if (name.empty()) {
        // root of the dataset is always a directory
        return true;
    }

    std::string path;
    get_component_path(name, path);
    if (path.empty())  // failed to get path
        return false;
    VPLFS_stat_t stat;
    if (VPLFS_Stat(path.c_str(), &stat) != VPL_OK)
        return false;
    return stat.type == VPLFS_TYPE_DIR;
}

// Open file key prefix strings.
static const char readRegularPrefix[] = "RREG";
static const char writeRegularPrefix[] = "WREG";

VPLFile_handle_t fs_dataset::get_component_fd(const std::string& component, bool writeable, s16& error)
{
    map<string, fileinfo*>::iterator fd_it;
    string key;
    error = VSSI_SUCCESS;
    VPLFile_handle_t fh = VPLFILE_INVALID_HANDLE;

    key = (writeable ? writeRegularPrefix : readRegularPrefix) + component;

    // Open file if needed. Remember file handle for later.
    VPLMutex_Lock(&fd_mutex);
    fd_it = open_fds.find(key);
    if(fd_it == open_fds.end()) {

        if(component_is_directory(component)) {
            error = VSSI_ISDIR;
            goto fail;
        }

        string path;
        error = get_component_path(component, path);
        if (error != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to map component %s to path: Error %d.",
                             component.c_str(), error);
            goto fail;
        }

        fh = VPLFile_Open(path.c_str(), 
                          writeable ? VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE : VPLFILE_OPENFLAG_READONLY, 
                          0777);
        if(VPLFile_IsValidHandle(fh)) {
            fileinfo *fi = new fileinfo;
            fi->h = fh;
            fi->count++;
            open_fds[key] = fi;
        }

        if(open_fds.size() > OPEN_FD_LIMIT) {
            // Keep total file handles down.
            close_unused_files_unlocked();
        }
    }
    else {
        fd_it->second->count++;
        fh = fd_it->second->h;
    }

 fail:
    VPLMutex_Unlock(&fd_mutex);

    return fh;
}

void fs_dataset::release_component_fd(const std::string& component, bool writeable)
{
    map<string, fileinfo*>::iterator fd_it;
    string key;

    key = (writeable ? writeRegularPrefix : readRegularPrefix) + component;

    // Reduce refcount no lower than 0 if fd exists.
    VPLMutex_Lock(&fd_mutex);
    fd_it = open_fds.find(key);
    if(fd_it != open_fds.end()) {
        fd_it->second->count--;
    }
    VPLMutex_Unlock(&fd_mutex);
}

// Only called while holding write lock. Refcounts may be safely ignored.
void fs_dataset::close_component_fd(const std::string& component)
{
    map<string, fileinfo*>::iterator fd_it;
    string key;

    // Close all open files for component, syncing those open for write.
    VPLMutex_Lock(&fd_mutex);

    key = writeRegularPrefix + component;
    fd_it = open_fds.find(key);
    if(fd_it != open_fds.end()) {
        delete fd_it->second;
        open_fds.erase(fd_it);
    }

    key = readRegularPrefix + component;
    fd_it = open_fds.find(key);
    if(fd_it != open_fds.end()) {
        delete fd_it->second;
        open_fds.erase(fd_it);
    }

    VPLMutex_Unlock(&fd_mutex);
}

// Only called while holding write lock. Refcounts may be safely ignored.
void fs_dataset::close_component_fds()
{
    map<string, fileinfo*>::iterator fd_it;

    // Close all files, syncing those open for write.
    VPLMutex_Lock(&fd_mutex);
    while((fd_it = open_fds.begin()) != open_fds.end()) {
        delete fd_it->second;
        open_fds.erase(fd_it);
    }
    VPLMutex_Unlock(&fd_mutex);
}

void fs_dataset::close_unused_files()
{
    VPLMutex_Lock(&fd_mutex);
    close_unused_files_unlocked();
    VPLMutex_Unlock(&fd_mutex);
}

void fs_dataset::close_unused_files_unlocked()
{
    map<string, fileinfo*>::iterator fd_it;

    fd_it = open_fds.begin();
    while(fd_it != open_fds.end()) {
        map<string, fileinfo*>::iterator temp = fd_it;
        temp++;
        if(fd_it->second->count == 0) {
            delete fd_it->second;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Closed fd for %s on over-limit condition.",
                              fd_it->first.c_str());
            open_fds.erase(fd_it);
        }
        fd_it = temp;
    }
}

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
void replaceAll(std::string &data, const std::string &A, const std::string &B)
{
    size_t r_index = 0;
    while(r_index < data.size() && (r_index=data.find(A, r_index)) != std::string::npos){
        data.replace(r_index, A.size(), B);
        r_index += B.size();
    }
}
#endif

s16 fs_dataset::append_dirent(const std::string& filename,
                              const file_stat_t& stat,
                              std::ostringstream& data_out,
                              bool json)
{
    if (json) {
        data_out << "{\"name\":\"" << filename << '"'
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
                 << ",\"type\":\"" << (stat.isShortCut ? "shortcut" : (stat.stat.type == VPLFS_TYPE_DIR? "dir" : "file")) << '"'
#else
                 << ",\"type\":\"" << (stat.stat.type == VPLFS_TYPE_DIR? "dir" : "file") << '"'
#endif
                 << ",\"lastChanged\":" << stat.stat.mtime
                 << ",\"size\":" << stat.stat.size
                 << ",\"isReadOnly\":" << (stat.stat.isReadOnly? "true" : "false")
                 << ",\"isHidden\":" << (stat.stat.isHidden? "true" : "false")
                 << ",\"isSystem\":" << (stat.stat.isSystem ? "true" : "false")
                 << ",\"isArchive\":" << (stat.stat.isArchive ? "true" : "false")
                //always return "isAllowed" as true
                 << ",\"isAllowed\":" << "true";
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        if (!stat.target_path.empty()) {
            data_out << ",\"target_path\":\"" << stat.target_path << "\"";
        }
        if (!stat.target_type.empty()) {
            data_out << ",\"target_type\":\"" << stat.target_type << "\"";
        }
        if (!stat.target_args.empty()) {
            // escape argument for json format
            // Only for arguments currently, since file/folder name cannot have /, \ and "
            std::string new_Args = stat.target_args;
            replaceAll(new_Args, "\\", "\\\\");
            replaceAll(new_Args, "/", "\\/");
            replaceAll(new_Args, "\"", "\\\"");
            data_out << ",\"target_args\":\"" << new_Args << "\"";
        }
#endif
        data_out << "}";
    }
    else {
        char vss_dirent[VSS_DIRENT_BASE_SIZE];

        memset(vss_dirent, 0, VSS_DIRENT_BASE_SIZE);

        vss_dirent_set_size(vss_dirent, stat.stat.size);
        vss_dirent_set_ctime(vss_dirent, VPLTime_FromSec(stat.stat.ctime));
        vss_dirent_set_mtime(vss_dirent, VPLTime_FromSec(stat.stat.mtime));
        if(stat.stat.type == VPLFS_TYPE_DIR) {
            vss_dirent_set_is_dir(vss_dirent, true);
        }
        else {
            vss_dirent_set_is_dir(vss_dirent, false);
        }

        vss_dirent_set_name_len(vss_dirent, filename.length() + 1); // account for NUL char at the end
        vss_dirent_set_reserved(vss_dirent);
        //vss_dirent_set_signature(vss_dirent, dummy_sig);

        // there are no metadata in FS datasets
        vss_dirent_set_meta_size(vss_dirent, 0);

        vss_dirent_set_change_ver(vss_dirent, 0);

        data_out.write(vss_dirent, sizeof(vss_dirent));
        data_out << filename;
        data_out.put('\0');  // NUL char at the end of name
    }

    return 1;  // 1 entry added
}

s16 fs_dataset::append_dirent(const std::string& folder_path,
                              const std::string& filename,
                              std::ostringstream& data_out,
                              bool json)
{
    s16 rv = 0;
    {
        file_stat_t stat;
        const std::string path = folder_path + "/" + filename;
        {
            int err = VPLFS_Stat(path.c_str(), &(stat.stat));
            if (err != VPL_OK) {
                // some system files are known to fail VPLFS_Stat(), so skip
                goto out;
            }
        }
        if (stat.stat.isHidden)
            goto out;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        // Bug 10824: get shortcut detail if it is a file with .lnk extension
        if (stat.stat.type == VPLFS_TYPE_FILE 
         && filename.find_last_of(".") != string::npos
         && filename.substr(filename.find_last_of(".")+1) == "lnk") {
             stat.isShortCut = true;
            get_shortcut_detail(path, stat.target_path, stat.target_type, stat.target_args);
        } else {
            stat.isShortCut = false;
        }
#endif
        rv = append_dirent(filename, stat, data_out, json);
    }
 out:
    return rv;
}

#ifdef WIN32
s16 fs_dataset::read_dir_win_libraries(const std::string& component,
                                       std::string& data_out,
                                       bool json)
{
    // sanity check
    if (component != "Libraries" && component.find("Libraries/") != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "dispatched to wrong function for component %s", component.c_str());
        return VSSI_INVALID;
    }

    s16 rv = VSSI_INVALID;

    { // critical section for winLibFolders
        MutexAutoLock lock(&prefix_rewrite_mutex);

        refreshPrefixRewriteRules();

        if (component == "Libraries" || component == "Libraries/") {
            if (json) {
                std::ostringstream oss;
                int count = 0;
                oss << "{\"fileList\":[";
                std::map<std::string, _VPLFS__LibInfo>::const_iterator it;
                for (it = winLibFolders.begin(); it != winLibFolders.end(); it++) {
                    if (count > 0)
                        oss << ",";
                    oss << "{\"name\":\"" << it->first
                        << "\",\"displayName\":\"" << it->second.l_name
                        // XXX TBD: add the libraryType at 2.5-RC1
#if 0
                        << "\",\"libraryType\":\"" << it->second.folder_type
#endif
                        << "\",\"type\":\"dir\",\"lastChanged\":0,\"size\":0,\"isAllowed\":";
                    {
                        std::string path = "Libraries/";
                        path += it->first;
                        int err = HttpSvc::Sn::Handler_rf_Helper::checkAccessControlList(this, path);
                        if(err)
                            oss << "false";
                        else
                            oss << "true";
                    }
                    oss << "}";
                    count++;
                }
                oss << "],\"numOfFiles\":" << count << "}";
                data_out.assign(oss.str());
                rv = VSSI_SUCCESS;
            }
            else {
                std::ostringstream oss;
                file_stat_t stat;
                std::map<std::string, _VPLFS__LibInfo>::const_iterator it;
                for (it = winLibFolders.begin(); it != winLibFolders.end(); it++) {
                    memset(&stat.stat, 0, sizeof(stat.stat));
                    stat.stat.type = VPLFS_TYPE_DIR;
                    append_dirent(it->first, stat, oss, false);
                }
                data_out.assign(oss.str());
                rv = VSSI_SUCCESS;
            }
        }
        else {
            // component must be of form "Libraries/.+"
            size_t slash = component.find('/');
            if (slash == std::string::npos) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unables to parse path: %s",
                                 component.c_str());
                return rv;
            }
            std::map<std::string, _VPLFS__LibInfo>::const_iterator libit;
            std::string target = component.substr(slash+1);
            libit = winLibFolders.begin();
            for (; libit != winLibFolders.end(); libit++) {
                // match for both localized and non-localized folder name
                if (libit->second.n_name == target ||
                    libit->second.l_name == target) {
                    break;
                }
            }
            if (libit != winLibFolders.end()) {
                if (json) {
                    std::ostringstream oss;
                    int count = 0;
                    oss << "{\"fileList\":[";
                    std::map<std::string, _VPLFS__LibFolderInfo>::const_iterator locit;
                    for (locit = libit->second.m.begin(); locit != libit->second.m.end(); locit++) {
                        if (count > 0)
                            oss << ",";
                        oss << "{\"name\":\"" << locit->second.n_name
                            << "\",\"displayName\":\"" << locit->second.l_name
                            << "\",\"type\":\"dir\",\"lastChanged\":0,\"size\":0}";
                        count++;
                    }
                    oss << "],\"numOfFiles\":" << count << "}";
                    data_out.assign(oss.str());
                    rv = VSSI_SUCCESS;
                }
                else {
                    std::ostringstream oss;
                    file_stat_t stat;
                    std::map<std::string, _VPLFS__LibFolderInfo>::const_iterator locit;
                    for (locit = libit->second.m.begin(); locit != libit->second.m.end(); locit++) {
                        memset(&stat.stat, 0, sizeof(stat.stat));
                        stat.stat.type = VPLFS_TYPE_DIR;
                        rv = append_dirent(locit->second.l_name, stat, oss, false);
                    }
                    data_out.assign(oss.str());
                    rv = VSSI_SUCCESS;
                }
            }
        }
    }

    return rv;
}
#endif // WIN32

#ifdef WIN32
// Return directory listing for the root directory on windows
s16 fs_dataset::read_dir_win_root(const std::string& component,
                              std::string& data_out,
                              bool json)
{
    // sanity check
    if (component != "" && component != "/") {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "dispatched to wrong function for component %s", component.c_str());
        return VSSI_INVALID;
    }

    s16 rv = VSSI_SUCCESS;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "component path %s", component.c_str());
    std::ostringstream oss;
    std::string desktop_display_name;
    char *upath = NULL;
    bool bDesktop = false;

    rv = _VPLFS__GetDesktopPath(&upath);
    if (rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to query desktop path: %d", rv);
    }
    else {
        rv = _VPLFS__GetDisplayName(string(upath), desktop_display_name);
        if (rv != 0)
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to query desktop path for display name: %d", rv);
        else
            bDesktop = true;
    }

    oss << "{\"fileList\":[{\"name\": \"Computer\", \"type\" : \"dir\", \"isAllowed\" : true}, "
        << "{\"name\": \"Libraries\", \"type\" : \"dir\", \"isAllowed\" : true}, "
        << "{\"name\": \"Public_Desktop\", \"type\" : \"dir\", \"isAllowed\" : ";
    {
        int err = HttpSvc::Sn::Handler_rf_Helper::checkAccessControlList(this, "Public_Desktop");
        if(err)
            oss << "false";
        else
            oss << "true";
    }
    oss << "}, ";
    if ( bDesktop ) {
        oss << "{\"name\": \"Desktop\", \"displayName\" : \"" << desktop_display_name << "\", \"type\" : \"dir\", \"isAllowed\" : ";
    } else {
        oss << "{\"name\": \"Desktop\", \"type\" : \"dir\", \"isAllowed\" : ";
    }
    {
        int err = HttpSvc::Sn::Handler_rf_Helper::checkAccessControlList(this, "Desktop");
        if(err)
            oss << "false";
        else
            oss << "true";
    }
    oss << "}],";
    oss << "\"numOfFiles\":4}";
    data_out.assign(oss.str());

out:
    if (upath != NULL) {
        free (upath);
        upath = NULL;
    }
    return rv;
}
#endif // WIN32

#ifdef WIN32
// Return directory listing for the Computer folder on windows
s16 fs_dataset::read_dir_win_computer(const std::string& component,
                                      std::string& data_out,
                                      bool json)
{
    // sanity check
    if (component != "Computer" && component != "Computer/") {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "dispatched to wrong function for component %s", component.c_str());
        return VSSI_INVALID;
    }

    s16 rv = VSSI_SUCCESS;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "component path %s", component.c_str());
    int totalItems = 0;
    std::ostringstream oss;
    std::map<std::string, _VPLFS__DriveType> driveMap;
    std::map<std::string, _VPLFS__DriveType>::iterator it;

    rv = _VPLFS__GetComputerDrives(driveMap);
    if(rv == 0 && !driveMap.empty()) {
        oss << "{\"fileList\":[";
        for(it = driveMap.begin(); it != driveMap.end(); it++) {
            std::string strType;
            switch(it->second) {
            case WIN_DRIVE_REMOVABLE:
                strType = "DRIVE_REMOVABLE";
                break;
            case WIN_DRIVE_FIXED:
                strType = "DRIVE_FIXED";
                break;
            case WIN_DRIVE_CDROM:
                strType = "DRIVE_CDROM";
                break;
            case WIN_DRIVE_RAMDISK:
                strType = "DRIVE_RAMDISK";
                break;
            case WIN_DRIVE_UNKNOWN:
            case WIN_DRIVE_NO_ROOT_DIR:
            case WIN_DRIVE_REMOTE:
            default:
                break;
            }
            if (!strType.empty()) {
                totalItems++;
                if (json) {
                    if (totalItems > 1) {
                        oss << ",";
                    }
                    oss << "{\"name\": \"";
                    oss << it->first[0] << "\", \"driveType\" : \"";
                    oss << strType << "\", \"isAllowed\" : ";
                    {
                        std::string path = "Computer/";
                        path += it->first[0];
                        int err = HttpSvc::Sn::Handler_rf_Helper::checkAccessControlList(this, path);
                        if(err)
                            oss << "false";
                        else
                            oss << "true";
                    }
                    oss << "}";
                }
                else {
                    oss << it->first << strType;  // FIXME
                }
            }
        }
        if (json) {
            oss << "],\"numOfFiles\":" << totalItems << "}";
        }
    }
    data_out.assign(oss.str());

    return rv;
}
#endif // WIN32

// Return directory listing for folder on the local file system at "folder_path".
// The path to the folder relative to the root of the dataset is given in "component".
s16 fs_dataset::read_dir_fs(const std::string& folder_path,
                            const std::string& component,
                            std::string& data_out,
                            bool json)
{
    s16 rv = VSSI_SUCCESS;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "folder path %s", folder_path.c_str());

    VPLFS_dir_t dir;
    VPLFS_dirent_t dirent;
    bool need_to_close = false;
    int totalItems = 0;
    std::ostringstream oss;

    rv = VPLFS_Opendir(folder_path.c_str(), &dir);
    if (rv != VPL_OK) {
        goto out;
    }
    need_to_close = true;

    while ((rv = VPLFS_Readdir(&dir, &dirent)) == VPL_OK) {
        if ((strcmp(dirent.filename, ".") == 0) ||
            (strcmp(dirent.filename, "..") == 0))
            continue;
        if (json && totalItems > 0)
            oss << ",";
        rv = append_dirent(folder_path, dirent.filename, oss, json);
        totalItems += rv;
        if (rv < 0) {
            goto out;
        }
    }
    if (rv == VPL_ERR_MAX) {
        // no more entries - this is acceptable outcome
        rv = VSSI_SUCCESS;
    }
    if (json) {
        oss << "],\"numOfFiles\":" << totalItems << "}";
        data_out.assign("{\"fileList\":[" + oss.str());
    }
    else {
        data_out.assign(oss.str());
    }

 out:
    if (need_to_close) {
        VPLFS_Closedir(&dir);
        need_to_close = false;
    }

    return rv;
}

s16 fs_dataset::read_dir(const std::string& component,
                         std::string& data_out,
                         bool json,
                         bool pagination,
                         const std::string& sort,
                         u32 index,
                         u32 max)
{
#ifdef WIN32
    if (component == "" || component == "/") {
        return read_dir_win_root(component, data_out, json);
    }
    else if (component == "Computer" || component == "Computer/") {
        return read_dir_win_computer(component, data_out, json);
    }
    else if (component == "Libraries" ||
             (component.find("Libraries/") == 0 && count(component.begin(), component.end(), '/') <= 1)) {
        return read_dir_win_libraries(component, data_out, json);
    }
    else
#endif // WIN32
    {
        std::string path;
        get_component_path(component, path);
        if (pagination == true) {
            return read_dir_fs_pagination(path, component, data_out, json, sort, index, max);
        }
        return read_dir_fs(path, component, data_out, json);
    }
}

s16 fs_dataset::read_dir2(const std::string& component,
                          std::string& data_out,
                          bool json,
                          bool pagination,
                          const std::string& sort,
                          u32 index,
                          u32 max)
{
#ifdef WIN32
    if (component == "" || component == "/") {
        return read_dir_win_root(component, data_out, json);
    }
    else if (component == "Computer" || component == "Computer/") {
        return read_dir_win_computer(component, data_out, json);
    }
    else if (component == "Libraries" ||
             (component.find("Libraries/") == 0 && count(component.begin(), component.end(), '/') <= 1)) {
        return read_dir_win_libraries(component, data_out, json);
    }
    else
#endif // WIN32
    {
        std::string path;
        get_component_path(component, path);
        if (pagination == true) {
            return read_dir_fs_pagination(path, component, data_out, json, sort, index, max);
        }
        return read_dir_fs(path, component, data_out, json);
    }
}

s16 fs_dataset::create_component(const std::string& component)
{
    s16 rv = VSSI_SUCCESS;
    std::string path, new_component, new_dir;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" create component {%s}.",
                        id.uid, id.did, component.c_str());

    rv = get_component_path(component, path);
    if(rv == 0) {
        if(VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
            rv = delete_component(component);
            if(rv != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                    "Directory: %s exists; failed to delete it. Error:%d",
                                    path.c_str(), rv);
                rv = VSSI_ACCESS;
                goto exit;
            }
        }
        // Create the component directory, one level above path
        size_t slash = path.find_last_of('/');
        if(slash != string::npos) {
            string dir = path.substr(0, slash);
            if(create_directory(dir) != 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                FMTu64":"FMTu64" {%s} component file {%s} make directory failed.",
                                id.uid, id.did,
                                component.c_str(),
                                path.c_str());
                rv = VSSI_ACCESS;
                goto exit;
            }
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" make directory {%s}.",
                        id.uid, id.did, component.c_str());
    }

    // If directory not in place, create it.
    if(VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) != VPL_OK) {
        rv = VPLDir_Create(path.c_str(), 0777);
        if(rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                "Failed to create directory %s. Error:%d",
                                path.c_str(), rv);
            return VSSI_ACCESS;
        }
    }

 exit:
    return rv;
}

static bool sort_by_time(std::pair<std::string, fs_dataset::file_stat_t> i,
                         std::pair<std::string, fs_dataset::file_stat_t> j) {
    return i.second.stat.mtime > j.second.stat.mtime;
}

static bool sort_by_size(std::pair<std::string, fs_dataset::file_stat_t> i,
                         std::pair<std::string, fs_dataset::file_stat_t> j) {
    return i.second.stat.size > j.second.stat.size;
}

static bool sort_by_name(std::pair<std::string, fs_dataset::file_stat_t> i,
                         std::pair<std::string, fs_dataset::file_stat_t> j) {
    return i.first < j.first;
}

// Return directory listing for folder on the local file system at "folder_path".
// The path to the folder relative to the root of the dataset is given in "component".
s16 fs_dataset::read_dir_fs_pagination(const std::string& folder_path,
                                       const std::string& component,
                                       std::string& data_out,
                                       bool json,
                                       const std::string& sort,
                                       u32 index,
                                       u32 max)
{
    s16 rv = VSSI_SUCCESS;

    VPLFS_dir_t dir;
    VPLFS_dirent_t dirent;
    bool need_to_close = false;
    bool needComma = false;
    std::ostringstream oss;
    std::vector< std::pair<std::string, file_stat_t> > filelist;
    u32 total = 0;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "folder path %s", folder_path.c_str());
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "pagination: sort=%s, index=%d, max=%d",
                      sort.c_str(), index, max);

    if (max == 0) {
        // doesn't handle case w/ max entry 0
        goto out_for_nothing;
    }

    rv = VPLFS_Opendir(folder_path.c_str(), &dir);
    if (rv != VPL_OK) {
        goto out;
    }
    need_to_close = true;

    while ((rv = VPLFS_Readdir(&dir, &dirent)) == VPL_OK) {
        if ((strcmp(dirent.filename, ".") == 0) ||
            (strcmp(dirent.filename, "..") == 0))
            continue;

        // stat the file
        file_stat_t stat;
        const std::string path = folder_path + "/" + dirent.filename;
        if (VPLFS_Stat(path.c_str(), &(stat.stat)) != VPL_OK) {
            // some system files are known to fail VPLFS_Stat(), so skip
            continue;
        }
        if (stat.stat.isHidden) {
            continue;
        }
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        // Bug 10824: get shortcut detail if it is a file with .lnk extension
        if (stat.stat.type == VPLFS_TYPE_FILE 
         && path.find_last_of(".") != string::npos
         && path.substr(path.find_last_of(".")+1) == "lnk") {
             stat.isShortCut = true;
            get_shortcut_detail(path, stat.target_path, stat.target_type, stat.target_args);
        } else {
            stat.isShortCut = false;
        }
#endif
        filelist.push_back(std::make_pair(dirent.filename, stat));
    }

    if (rv == VPL_ERR_MAX) {
        // no more entries - this is acceptable outcome
        rv = VSSI_SUCCESS;
    }

    // index starts from 1
    if (filelist.empty() || filelist.size() < index) {
        goto out_for_nothing;
    }

    if (sort == "alpha") {
        std::sort(filelist.begin(), filelist.end(), sort_by_name);
    } else if (sort == "size") {
        std::sort(filelist.begin(), filelist.end(), sort_by_size);
    } else {
        // default sorting is "time"
        std::sort(filelist.begin(), filelist.end(), sort_by_time);
    }

    for (u32 i = index-1; i < filelist.size() && total < max; i++, total++) {
        if (needComma) {
            oss << ",";
        }
        append_dirent(filelist[i].first, filelist[i].second, oss, json);
        needComma = true;
    }

out_for_nothing:
    if (json) {
        oss << "],\"numOfFiles\":" << total << "}";
        data_out.assign("{\"fileList\":[" + oss.str());
    }
    else {
        data_out.assign(oss.str());
    }
out:

    if (need_to_close) {
        VPLFS_Closedir(&dir);
        need_to_close = false;
    }

    return rv;
}

s16 fs_dataset::create_directory(const std::string& path)
{
    int rc;
    string new_dir;
    VPLFS_stat_t stats;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" create directory {%s}.",
                        id.uid, id.did, path.c_str());

    // Create save directory path by creating any missing directories.
    // Any files in the way will be deleted.
    // This is a bit more than "mkdir -p <dir_name>"
    // Only directories followed by '/' will be created, so giving a file path
    // will create the file's parent directories.
    for(size_t pos = path.find('/', 1); pos != string::npos ; pos = path.find('/', pos + 1)) {
        new_dir.assign(path, 0, pos);

        // If a non-directory, remove it.
        rc = VPLFS_Stat(new_dir.c_str(), &stats);
        if(rc == VPL_OK) {
            if(stats.type != VPLFS_TYPE_DIR) {
                rc = VPLFile_Delete(new_dir.c_str());
                if(rc != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "Failed to delete file %s. Error:%d",
                        new_dir.c_str(), rc);
                }
            }
        }
        else if(rc != VPL_ERR_NOENT) { // expect/hope entry not found.
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                "stat(%s) failed: %d",
                                new_dir.c_str(), rc);
        }

        // If directory not in place, create it.
        if(VPLFile_CheckAccess(new_dir.c_str(), VPLFILE_CHECKACCESS_EXISTS) != VPL_OK) {
            rc = VPLDir_Create(new_dir.c_str(), 0777);
            if(rc != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                    "Failed to create directory %s. Error:%d",
                                    new_dir.c_str(), rc);
                return VSSI_ACCESS;
            }
        }
    }

    // If directory not in place, create it.
    if(VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) != VPL_OK) {
        rc = VPLDir_Create(path.c_str(), 0777);
        if(rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                "Failed to create directory %s. Error:%d",
                                path.c_str(), rc);
            return VSSI_ACCESS;
        }
    }

    return VSSI_SUCCESS;
}

s16 fs_dataset::rename_component(const std::string& component, const std::string& new_component)
{
    s16 rv = VSSI_SUCCESS;
    bool exist = false;
    std::string path;
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" make directory {%s}.",
                        id.uid, id.did, component.c_str());

    // Don't allow if source is a subpath of destination or vice versa.
    if((new_component.compare(0, component.size(), component) == 0 &&
        (component.size() == new_component.size() ||
         new_component[component.size()] == '/')) ||
       (component.compare(0, new_component.size(), new_component) == 0 &&
        (component.size() == new_component.size() ||
         component[new_component.size()] == '/'))) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} fail: intersecting directories.",
                          id.uid, id.did,
                          component.c_str(), new_component.c_str());
        // Impossible instruction. Treat as success.
        goto exit;
    }

    rv = check_existence(component, exist);
    if(rv == 0 && !exist) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} fail: source not found.",
                          id.uid, id.did,
                          component.c_str(), new_component.c_str());
        // Impossible instruction. Treat as success.
        goto exit;
    }

    rv = check_existence(new_component, exist);
    if(rv == 0 && exist) {
        // Something in the way. Delete it.
        goto exit;
    }
    else {
        // Create directory path for component.
        size_t slash = new_component.find_last_of('/');
        if(slash != string::npos) {
            string dir;
            bool exist = false;
            dir = new_component.substr(0, slash);
            rv = check_existence(dir, exist);
            if(rv == 0 && !exist) {
                rv = create_component(dir);
                if(rv != VSSI_SUCCESS) {
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} fail: create destination directory.",
                                      id.uid, id.did,
                                      component.c_str(), new_component.c_str());
                    goto exit;
                }
            }
        }
        rv = copy_component(component, new_component);
        if(rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "copy_component failed. source: %s, destination: %s:%d", component.c_str(),
                new_component.c_str(), rv);
        }
        else {
            rv = delete_component(component);
            if(rv != 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "delete_component failed. source: %s, rv: %d", component.c_str(), rv);
            }
        }
    }

 exit:
    return rv;
}

s16 fs_dataset::copy_component(const std::string& component, const std::string& new_component)
{
    s16 rv = VSSI_SUCCESS;
    std::string path, newpath;
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" copy component {%s}.",
                        id.uid, id.did, component.c_str());

    // Don't allow if source is a subpath of destination or vice versa.
    if((new_component.compare(0, component.size(), component) == 0 &&
        (component.size() == new_component.size() ||
         new_component[component.size()] == '/')) ||
       (component.compare(0, new_component.size(), new_component) == 0 &&
        (component.size() == new_component.size() ||
         component[new_component.size()] == '/'))) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" copy {%s} to {%s} fail: intersecting directories.",
                          id.uid, id.did,
                          component.c_str(), new_component.c_str());
        // Impossible instruction. Treat as success.
        goto exit;
    }

    rv = get_component_path(component, path);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "get_component_path failed. component: %s:%d", component.c_str(), rv);
        goto exit;
    }
    if(!directory_exists(path)) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" copy {%s} to {%s} fail: source not found.",
                          id.uid, id.did,
                          component.c_str(), new_component.c_str());
        // Impossible instruction. Treat as success.
        goto exit;
    }

    rv = get_component_path(new_component, newpath);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "get_component_path failed. new_component: %s:%d", new_component.c_str(), rv);
        goto exit;
    }
    if(directory_exists(newpath)) {
        // Something in the way. Delete it.
        goto exit;
    }
    else {
        // Create directory path for component.
        size_t slash = new_component.find_last_of('/');
        if (slash != string::npos) {
            string dir;
            bool exist = false;
            dir = new_component.substr(0, slash);
            rv = check_existence(dir, exist);
            if (rv == 0 && !exist) {
                rv = create_component(dir);
                if (rv != VSSI_SUCCESS) {
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Dataset "FMTu64":"FMTu64" copy {%s} to {%s} fail: create destination directory.",
                                      id.uid, id.did,
                                      component.c_str(), new_component.c_str());
                    goto exit;
                }
            }
        }
        // If component is directory
        if (component_is_directory(component)) {
            int rc = 0;
            VPLFS_dir_t dir_folder;
            rc = VPLFS_Opendir(path.c_str(), &dir_folder);
            if (rc == VPL_ERR_NOENT) {
                goto exit;
            }else if (rc != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unable to open %s:%d", path.c_str(), rc);
                goto exit;
            }
            ON_BLOCK_EXIT(VPLFS_Closedir, &dir_folder);
            VPLFS_dirent_t folderDirent;
            while ((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
            {
                std::string dirent(folderDirent.filename);
                std::string fullPath = path, new_path = newpath, component1 = component, new_component1 = new_component, response;
                fullPath += ("/" + dirent);
                new_path += ("/" + dirent);
                component1 += ("/" + dirent);
                new_component1 += ("/" + dirent);
                if(folderDirent.type != VPLFS_TYPE_FILE) {
                    if(dirent=="." || dirent=="..") {
                        // We want to skip these directories
                    }else{
                        // save directory for later processing
                        rc = copy_component(component1, new_component1);
                        if(rc != 0) {
                            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "copy_component failed. source: %s, destination: %s, rc: %d",
                                component1.c_str(), new_component1.c_str(), rc);
                        }
                    }
                    continue;
                }
                else {
                    u32 tempBufferSize = 16*1024;
                    char* tempBuffer = (char*)malloc(tempBufferSize);
                    if(tempBuffer == NULL) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Out of memory");
                        goto exit;
                    }
                    ON_BLOCK_EXIT(free, tempBuffer);
                    //copy_file
                    rc = copy_file(fullPath, new_path, tempBuffer, tempBufferSize);
                    if(rc != 0) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "copy_file failed. source: %s, destination: %s", fullPath.c_str(), new_path.c_str());
                        goto exit;
                    }
                }
            }
            if(!directory_exists(newpath)) {
                rv = create_directory(newpath);
                if(rv != 0) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "create_directory failed. source: %s, destination: %s", path.c_str(), newpath.c_str());
                    goto exit;
                }
            }
        }
        else {
            u32 tempBufferSize = 16*1024;
            char* tempBuffer = (char*)malloc(tempBufferSize);
            if(tempBuffer == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Out of memory");
                goto exit;
            }
            ON_BLOCK_EXIT(free, tempBuffer);
            //copy_file
            rv = copy_file(path, newpath, tempBuffer, tempBufferSize);
            if(rv != 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "copy_file failed. source: %s, destination: %s", path.c_str(), newpath.c_str());
                goto exit;
            }
        }
    }

 exit:
    return rv;
}

s16 fs_dataset::delete_component(const std::string& component)
{
    s16 rv = VSSI_SUCCESS;
    bool exist = false;
    std::string path;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" delete component {%s}.",
                        id.uid, id.did, component.c_str());

    rv = check_existence(component, exist);
    if(rv == 0) {
        if(exist == false) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "directory not exist: %s", component.c_str());
            goto exit;
        }
    }

    rv = get_component_path(component, path);
    if(rv == 0) {
        if(component_is_directory(component)) {
            rv = delete_dir(path);
            if (rv != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "delete directory %s failed: %d", path.c_str(), rv);
            }
        }
        else {
            int temp_rv = VPLFile_Delete(path.c_str());
            if (temp_rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLFile_Delete %s failed: %d", path.c_str(), temp_rv);
                rv = temp_rv;
                goto exit;
            }
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" get component path {%s}.",
                        id.uid, id.did, component.c_str());
    }

 exit:
    return rv;
}

s16 fs_dataset::delete_dir(const std::string& path)
{
    s16 rv = VSSI_SUCCESS;
    VPLFS_dirent_t entry;
    VPLFS_dir_t directory;
    std::string filename, new_path;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" delete directory {%s}.",
                        id.uid, id.did, path.c_str());

    rv = VPLFS_Opendir(path.c_str(), &directory);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLFS_Opendir %s failed: %d", path.c_str(), rv);
        goto exit;
    }

    while ((rv = VPLFS_Readdir(&directory, &entry)) == VPL_OK) {
        if(entry.type != VPLFS_TYPE_FILE) {
            if (strcmp(entry.filename, "..") == 0
                || strcmp(entry.filename, ".") == 0) {
            continue;
            }
            new_path = path + "/" + entry.filename;
            {
                int temp_rv = delete_dir(new_path);
                if (temp_rv != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "delete_dir %s failed: %d", filename.c_str(), temp_rv);
                    rv = temp_rv;
                    goto exit;
                }
            }
        }
        else {
            filename = path + "/" + entry.filename;
            {
                int temp_rv = VPLFile_Delete(filename.c_str());
                if (temp_rv != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLFile_Delete %s failed: %d", filename.c_str(), temp_rv);
                    rv = temp_rv;
                    goto exit;
                }
            }
        }
    }
    rv = VPLFS_Rmdir(path.c_str());
    if (rv == VPL_ERR_MAX) {
        // No more directories
        rv = VPL_OK;
    }

 exit:
    int temp_rv = VPLFS_Closedir(&directory);
    if (temp_rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLFS_Closedir %s failed: %d", path.c_str(), temp_rv);
        rv = temp_rv;
    }
    return rv;
}

s16 fs_dataset::check_existence(const std::string& component, bool& existed)
{
    s16 rv = VSSI_SUCCESS;
    std::string path;
    rv = get_component_path(component, path);
    if(rv == 0) {
        if(directory_exists(path)) {
            existed = true;
        }
        else {
            existed = false;
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" check existence {%s}.",
                        id.uid, id.did, component.c_str());
    }
    return rv;
}

int fs_dataset::stat_component(const std::string& component,
                               std::string& data_out)
{
    // TODO: Actually get stat info?
    return VSSI_SUCCESS;
}

int fs_dataset::stat2_component(const std::string& component,
                               std::string& data_out)
{
    int rv = VSSI_SUCCESS;
    file_stat_t stat_out;
    std::ostringstream oss;
    std::string path;

    { // critical section for winValidVirtualFolders
        MutexAutoLock lock(&prefix_rewrite_mutex);

        refreshPrefixRewriteRules();

#ifdef WIN32
        if (winValidVirtualFolders.find(component) != winValidVirtualFolders.end()) {
            stat_out.stat.size = 0;
            stat_out.stat.type = VPLFS_TYPE_DIR;
            stat_out.stat.atime = 0;
            stat_out.stat.mtime = 0;
            stat_out.stat.ctime = 0;
            stat_out.stat.isHidden = VPL_FALSE;
            stat_out.stat.isSymLink = VPL_FALSE;
            stat_out.stat.isReadOnly = VPL_FALSE;
            stat_out.stat.isSystem = VPL_FALSE;
            stat_out.stat.isArchive = VPL_FALSE;
            return rv;
        }
#endif
    }
    rv = get_component_path(component, path);
    if (rv == VSSI_SUCCESS) {
        rv = VPLFS_Stat(path.c_str(), &(stat_out.stat));
    }

    rv = append_dirent(path, stat_out, oss, true);
    data_out = oss.str();
    return VSSI_SUCCESS;
}

int fs_dataset::stat_component(const std::string& component,
                               VPLFS_stat_t& stat_out)
{
    int rv = VPL_OK;

    { // critical section for winValidVirtualFolders
        MutexAutoLock lock(&prefix_rewrite_mutex);

        refreshPrefixRewriteRules();

#ifdef WIN32
        if (winValidVirtualFolders.find(component) != winValidVirtualFolders.end()) {
            stat_out.size = 0;
            stat_out.type = VPLFS_TYPE_DIR;
            stat_out.atime = 0;
            stat_out.mtime = 0;
            stat_out.ctime = 0;
            stat_out.isHidden = VPL_FALSE;
            stat_out.isSymLink = VPL_FALSE;
            return rv;
        }
#endif
    }

    std::string path;
    rv = get_component_path(component, path);
    if (rv == VSSI_SUCCESS) {
        rv = VPLFS_Stat(path.c_str(), &stat_out);
    }
    return rv;
}


int fs_dataset::stat_shortcut(const std::string& component,
                              std::string& target_path,
                              std::string& target_type,
                              std::string& target_args)
{
    int rv = VSSI_SUCCESS;

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    std::string path;

    rv = get_component_path(component, path);
    if (rv == VSSI_SUCCESS) {
        rv = get_shortcut_detail(path, target_path, target_type, target_args);
        // error msg already recorded in get_shortcut_detail();
    } else {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "fail to get component path, err = %d", rv);
    }
    
#endif

    return rv;
}

int fs_dataset::accomodate_file_component(const std::string& name, bool writeable)
{
    int rv = VSSI_SUCCESS;
    std::string path;

    // error case analysis:
    // component is new. no action needed.
    // make sure component is a file. otherwise, trash.
    // all other errors -> something went wrong. log and bail.

    rv = get_component_path(name, path);
    if(rv == VSSI_SUCCESS && VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
        VPLFS_stat_t stat;
        if (VPLFS_Stat(path.c_str(), &stat) != VPL_OK && stat.type != VPLFS_TYPE_FILE && writeable) {
            rv = delete_component(name);
        }
    }

    return rv;
}

s16 fs_dataset::delete_all()
{
    return 0;
}

int fs_dataset::rfPathToAbsPath(
        const std::string& component,
        const std::map<std::string, std::string>& prefixRewriteRules,
        std::string& absPath_out)
{
    int rv = VSSI_INVALID;
    std::map<std::string, std::string>::const_iterator it;
    for (it = prefixRewriteRules.begin(); it != prefixRewriteRules.end(); it++) {
        // it->first is the prefix pattern
        // it->second is the replacement prefix

        // really obvious case: prefix pattern is empty
        // => simply prepend replacement prefix to component
        if (it->first.empty()) {
            absPath_out.assign(it->second);
            if (!component.empty()) {
                absPath_out.append("/");
                absPath_out.append(component);
            }
            rv = VSSI_SUCCESS;
            break;
        }

        // another obvious case: component is the prefix pattern
        // => the path is simply the replacement prefix
        if (component == it->first) {
            absPath_out.assign(it->second);
            rv = VSSI_SUCCESS;
            break;
        }

        // more general case
        if (component.length() > it->first.length() &&
            component.compare(0, it->first.length(), it->first) == 0 &&
            component[it->first.length()] == '/') {
            absPath_out.assign(it->second);
            if (component[it->first.length()] == '/') {
                absPath_out.append(component,
                                   it->first.length(),
                                   component.length() - it->first.length());
            }
            rv = VSSI_SUCCESS;
            break;
        }
    }
    return rv;
}

s16 fs_dataset::get_component_path(const std::string &component,
                                   std::string &path)
{
    s16 rv = VSSI_INVALID;
    path.clear();

    { // critical section for prefixRewriteRules
        MutexAutoLock lock(&prefix_rewrite_mutex);
        refreshPrefixRewriteRules();
        rv = rfPathToAbsPath(component,
                             prefixRewriteRules,
                             path);
    }
    return rv;
}

bool fs_dataset::directory_exists(const std::string &path)
{
    if(VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
        return true;
    }

    return false;
}

int fs_dataset::copy_file(const std::string& src,
                    const std::string& dst,
                    char* tempChunkBuf,
                    u32 tempChunkBufSize)
{
    if(tempChunkBuf==NULL || tempChunkBufSize==0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "tempChunkBuf and tempChunkBufSize need to be defined:%d",
                  tempChunkBufSize);
        return -1;
    }

    // Create dst directory
    size_t slash = dst.find_last_of('/');
    if(slash != string::npos) {
        int rv = 0;
        string dir;
        bool exist = false;
        dir = dst.substr(0, slash);
        exist = directory_exists(dir);
        if(!directory_exists(dir)) {
            rv = create_directory(dir);
            if(rv != VSSI_SUCCESS) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                    "Dataset "FMTu64":"FMTu64" copy {%s} to {%s} fail: create destination directory.",
                                    id.uid, id.did,
                                    src.c_str(), dst.c_str());
                return rv;
            }
        }
    }

    // Copy Thumbnail to staging area
    VPLFile_handle_t fHSrc = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t fHDst = VPLFILE_INVALID_HANDLE;

    const int flagDst = VPLFILE_OPENFLAG_CREATE |
                        VPLFILE_OPENFLAG_WRITEONLY |
                        VPLFILE_OPENFLAG_TRUNCATE;

    int rv = 0;

    fHSrc = VPLFile_Open(src.c_str(), VPLFILE_OPENFLAG_READONLY, 0777);
    if (!VPLFile_IsValidHandle(fHSrc)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Fail to open source file %s", src.c_str());
        rv = -1;
        goto errorSrc;
    }

    fHDst = VPLFile_Open(dst.c_str(), flagDst, 0777);
    if (!VPLFile_IsValidHandle(fHDst)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Fail to create or open dst file %s", dst.c_str());
        rv = -1;
        goto errorDst;
    }

    {  // Perform the copy in chunks
        do {
            ssize_t bytesRead = VPLFile_Read(fHSrc,
                                             tempChunkBuf,
                                             tempChunkBufSize);
            if (bytesRead > 0) {
                ssize_t wrCnt = VPLFile_Write(fHDst, tempChunkBuf, bytesRead);
                if (wrCnt != bytesRead) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Fail to write to dst file %s, %d/%d",
                              dst.c_str(),
                              (int)bytesRead, (int)wrCnt);
                    rv = -1;
                    goto error;
                }
            } else {
                break;
            }
        } while(true);
    }
 error:
    if (VPLFile_IsValidHandle(fHDst)) {
        VPLFile_Close(fHDst);
    }
 errorDst:
    if (VPLFile_IsValidHandle(fHSrc)) {
        VPLFile_Close(fHSrc);
    }
 errorSrc:

    return rv;
}

int fs_dataset::get_total_used_size(u64& size)
{
    // TODO: How to get the total size for a FS dataset?  See bug 2360.
    size = 0;
    return VSSI_SUCCESS;
}

s16 fs_dataset::commit_iw(void)
{
    u64 size = 0;
    int rv = VSSI_SUCCESS;

    get_total_used_size(size);
    server->add_dataset_stat_update(id, size, get_version());

    return rv;
}

s16 fs_dataset::set_metadata_iw(const std::string& component,
                                u8 type,
                                u8 length,
                                const char* data)
{
    // FS dataset does not support metadata

    VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                     "Dataset "FMTu64":"FMTu64": inst write access to FS",
                     id.uid, id.did);
    return VSSI_BADCMD;
}

s16 fs_dataset::remove_iw(const std::string& component, bool rename_target)
{
    return delete_component(component);
}

s16 fs_dataset::set_size_iw(const std::string& component, u64 new_size)
{
    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                     "Deprecated: set_size_iw not implemented, use fileHandleAPI truncateFile: %s",
                      component.c_str());
    return VSSI_BADCMD;
}

s16 fs_dataset::rename_iw(const std::string& component,
                          const std::string& new_name,
                          u32 flags)
{
    return rename_component(component, new_name);
}

s16 fs_dataset::set_times_iw(const std::string& component,
                             u64 ctime, u64 mtime)
{
    // FS dataset does not support setting the ctime.
    (void)ctime;

    s16 vssi_err = VSSI_SUCCESS;
    int vpl_err;

    std::string path;
    vssi_err = get_component_path(component, path);
    if (vssi_err != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to map component to path: component {%s}, error %d.",
                         component.c_str(), vssi_err);
        goto out;
    }

    vpl_err = VPLFile_SetTime(path.c_str(), mtime);
    if (vpl_err != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to set mtime; component {%s}, path {%s}, error %d.",
                         component.c_str(), path.c_str(), vpl_err);
        vssi_err = VSSI_INVALID;
        goto out;
    }

 out:
    return vssi_err;
}

s16 fs_dataset::make_directory_iw(const std::string& component, u32 attrs)
{
    s16 rv = VSSI_SUCCESS;
    std::string path;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "make_directory component {%s}"
                      " attrs 0x%08x", component.c_str(), attrs);

    //
    // Check whether the containing directory path exists.  In order to avoid "mkdir -p"
    // behavior, the error needs to be detected at this level.  Calling create_directory
    // will create the parent tree, but that's not the desired behavior.  Changing the
    // lower level datasetDB code is too scary, so do it here.
    //
    size_t slash = component.find_last_of('/');
    if(slash != string::npos) {
        string dir = component.substr(0, slash);
        if(!component_is_directory(dir)) {
            rv = VSSI_NOTFOUND;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Parent directory not found for directory %s.",
                             component.c_str());
            goto out;
        }
    }

    rv = create_component(component);
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed mkdir record:%d.",
                         id.uid, id.did, rv);
        goto out;
    }

    rv = get_component_path(component, path);
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to map component to path: component {%s}, error %d.",
                         component.c_str(), rv);
        goto out;
    }

    rv = VPLFile_SetAttribute(path.c_str(), attrs, VPLFILE_ATTRIBUTE_MASK);
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to set attributes to path: component {%s}, error %d.",
                         path.c_str(), rv);
    }

out:
    return rv;
}

s16 fs_dataset::chmod_iw(const std::string& component, u32 attrs, u32 attrs_mask)
{
    s16 rv = VSSI_SUCCESS;
    std::string path;

    rv = get_component_path(component, path);
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to map component to path: component {%s}, error %d.",
                         component.c_str(), rv);
        goto out;
    }

    rv = VPLFile_SetAttribute(path.c_str(), attrs, attrs_mask);
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to set attributes to path: component {%s}, error %d.",
                         path.c_str(), rv);
    }

out:
    return rv;
}

s16 fs_dataset::open_file(const std::string& component,
                          u64 version,
                          u32 flags,
                          u32 attrs,
                          vss_file*& fhandle_out)
{
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                     "Dataset "FMTu64":"FMTu64": file handle access to FS %s - %s%s",
                     id.uid, id.did, component.c_str(),
                     (flags&VSSI_FILE_OPEN_READ)?"read":"",
                     (flags&VSSI_FILE_OPEN_WRITE)?"write":"");

    s16 rv = 0;
    int rc;
    fhandle_out = NULL;
    vss_file* handle = NULL;
    int vpl_flags;

    VPLFile_handle_t fileHandle;
    std::string filepath;

    if(component_is_directory(component)) {
        rv = VSSI_ISDIR;
        goto exit;
    }


    rv = get_component_path(component, filepath);
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to map component %s to path: Error %d.",
                         component.c_str(), rv);
        goto exit;
    }

    if(flags & VSSI_FILE_OPEN_WRITE){
        vpl_flags = VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE;
    }else{
        vpl_flags = VPLFILE_OPENFLAG_READONLY;
    }
    fileHandle = VPLFile_Open(filepath.c_str(),
                              vpl_flags,
                              0777);

    if(!VPLFile_IsValidHandle(fileHandle)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open component %s for %s%s.",
                         component.c_str(),
                         (flags&VSSI_FILE_OPEN_READ)?"read":"",
                         (flags&VSSI_FILE_OPEN_WRITE)?"write":"");
        rv = VSSI_NOTFOUND;
        goto exit;
    }

    handle = new vss_file(this, fileHandle, component, filepath, flags, 0, 0);
    if(handle == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Out of mem");
        rv = VSSI_NOMEM;
        goto fail_mem;
    }

    fhandle_out = handle;
    return rv;
 fail_mem:
    rc = VPLFile_Close(fileHandle);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLFile_Close:%d", rc);
    }
 exit:
    return rv;
}

s16 fs_dataset::read_file(vss_file* fhandle,
                          vss_object* object,
                          u64 origin,
                          u64 offset,
                          u32& length_in_out,
                          char* data_out)
{
    s16 rv = VSSI_SUCCESS;
    int rc;
    if(fhandle == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Bad Params");
        return VSSI_BADCMD;
    }
    VPLFile_handle_t fileHandle = fhandle->get_fd();
    if (!VPLFile_IsValidHandle(fileHandle)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to get handle for read: component{%s}, file{%s}, error %d.",
                         fhandle->component.c_str(), fhandle->filename.c_str(), rv);
        return VSSI_BADCMD;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Reading %s at offset %"PRIx64", length %d",
                        fhandle->component.c_str(), offset, length_in_out);

    rc = VPLFile_ReadAt(fileHandle, data_out, length_in_out, offset);
    if(rc < 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to read component %s, %s. Error:%d.",
                         fhandle->component.c_str(), fhandle->filename.c_str(), rc);
        rv = rc;
    }
    else {
        length_in_out = rc;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Read component %s, offset %"PRIx64", length %d",
                        fhandle->component.c_str(), offset, length_in_out);
    return rv;
}

s16 fs_dataset::write_file(vss_file* fhandle,
                           vss_object* object,
                           u64 origin,
                           u64 offset,
                           u32& length_in_out,
                           const char* data)
{
    s16 rv = VSSI_SUCCESS;
    if(fhandle == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Bad Params");
        return VSSI_BADCMD;
    }
    u32 bytes_written_so_far = 0;

    VPLFile_handle_t fileHandle = fhandle->get_fd();
    if (!VPLFile_IsValidHandle(fileHandle)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to get handle for write: component{%s}, file{%s}, error %d.",
                         fhandle->component.c_str(), fhandle->filename.c_str(), rv);
        return VSSI_BADCMD;
    }

    while (bytes_written_so_far < length_in_out) {
        ssize_t bytes_written = VPLFile_WriteAt(fileHandle,
                                                data + bytes_written_so_far,
                                                length_in_out - bytes_written_so_far,
                                                offset + bytes_written_so_far);
        if (bytes_written < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             FMTu64":"FMTu64" {%s} {%s} fail to write new data, error "FMT_ssize_t".",
                             id.uid, id.did,
                             fhandle->component.c_str(),
                             fhandle->filename.c_str(),
                             bytes_written);
            rv = VSSI_INVALID;
            goto exit;
        }
        bytes_written_so_far += bytes_written;
    }

 exit:
    length_in_out = bytes_written_so_far;
    return rv;
}

s16 fs_dataset::truncate_file(vss_file* fhandle,
                              vss_object* object,
                              u64 origin,
                              u64 offset)
{
    s16 rv = VSSI_SUCCESS;
    int rc;
    if(fhandle == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Bad Params");
        return VSSI_BADCMD;
    }

    VPLFile_handle_t fileHandle = fhandle->get_fd();
    if (!VPLFile_IsValidHandle(fileHandle)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to get handle for write: component{%s}, file{%s}, error %d.",
                         fhandle->component.c_str(), fhandle->filename.c_str(), rv);
        return VSSI_BADCMD;
    }

    rc = VPLFile_TruncateAt(fileHandle, offset);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         FMTu64":"FMTu64" {%s} {%s} componentfd %d truncate to "FMTu64" bytes fail:%d.",
                         id.uid, id.did,
                         fhandle->component.c_str(),
                         fhandle->filename.c_str(),
                         fileHandle, offset, rc);
        rv = VSSI_INVALID;
        goto out;
    }

 out:
    return rv;
}

s16 fs_dataset::chmod_file(vss_file* fhandle,
                           vss_object* object,
                           u64 origin,
                           u32 attrs,
                           u32 attrs_mask)
{
    s16 rv = VSSI_SUCCESS;
    if(fhandle == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Bad Params");
        return VSSI_BADCMD;
    }

    VPLFile_handle_t fileHandle = fhandle->get_fd();
    if (!VPLFile_IsValidHandle(fileHandle)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to get handle for write: component{%s}, file{%s}, error %d.",
                         fhandle->component.c_str(), fhandle->filename.c_str(), rv);
        return VSSI_BADCMD;
    }

    rv = VPLFile_SetAttribute(fhandle->filename.c_str(), attrs, attrs_mask);
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to set attributes to path: component {%s}, error %d.",
                         fhandle->filename.c_str(), rv);
        rv = VSSI_INVALID;
        goto out;
    }

 out:
    return rv;
}

//
// release_file is a NOP for fs dataset.  The intent is to
// implement "CleanUpIRP" semantics for Windows, which means
// the descriptor is no longer usable by the application, but
// there may still be IO triggered by the VM system for mapped
// files.  close_file means all the mapped pages are gone.
// FS dataset does not support file locks or other state that
// would need to be released short of a real close_file.
//
s16 fs_dataset::release_file(vss_file* fhandle,
                             vss_object* object,
                             u64 origin)
{
    s16 rv = VSSI_SUCCESS;
    return rv;
}

s16 fs_dataset::close_file(vss_file* fhandle,
                           vss_object* object,
                           u64 origin)
{
    s16 rv = VSSI_SUCCESS;
    if(fhandle == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Bad Params");
        return VSSI_BADCMD;
    }

    //  No need to close file handle within fhandle.  It is done in the
    //  destructor when fhandle is deleted.
    //  rv = VPLFile_Close(fhandle->get_fd());  // Not needed.

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Dataset "FMTu64":"FMTu64": Close file handle access to FS %s,%s",
                      id.uid, id.did,
                      fhandle->component.c_str(),
                      fhandle->filename.c_str());

    // Update PSN status if file was open for write, create or delete
    if (fhandle->get_access_mode() & (VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE|VSSI_FILE_OPEN_DELETE)) {
        commit_iw();
    }

    delete fhandle;
    return rv;
}

vss_file* fs_dataset::map_file_id(u32 file_id)
{
    return NULL;
}

void fs_dataset::object_release(vss_object* object)
{
}

int fs_dataset::get_space(const std::string& path, u64& disk_size, u64& avail_size)
{
    int rv;
    rv = VPLFS_GetSpace(path.c_str(), &disk_size, &avail_size);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Error %d getting disk space %s.", rv, path.c_str());
    }

    return rv;
}

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
int fs_dataset::checkAccessRightHelper(const std::string& upath,
                                       u32 access_mask)
{
    ACCESS_MASK mask = 0;
    wchar_t *wpath = NULL;
    wchar_t *epath = NULL;
    int rv = 0;

    rv = _VPL__utf8_to_wstring(upath.c_str(), &wpath);
    if (rv != VPL_OK) {
        goto out;
    }

    /* Note:
     * _stat() cannot handle trailing slashes, so we need to remove them
     * cf. http://msdn.microsoft.com/en-us/library/14h5k7ff%28v=VS.100%29.aspx
     * unless the path is to the root directory (e.g., C:/)
     */
    if (wcslen(wpath) > 3) {
        int i = (int)(wcslen(wpath) - 1);
        while ((i >= 0) && (wpath[i] == L'/' || wpath[i] == L'\\')) {
            wpath[i] = L'\0';
            i--;
        }
    }

    rv = _VPLFS__GetExtendedPath(wpath, &epath, 0);
    if (rv < 0) {
        goto out;
    }

    // 3. Mapping the VPL Mask to requested one
    if (access_mask & VPLFILE_CHECK_PERMISSION_WRITE) {
        mask |= FILE_GENERIC_WRITE;
    }
    if (access_mask & VPLFILE_CHECK_PERMISSION_READ) {
        mask |= FILE_GENERIC_READ;
    }
    if (access_mask & VPLFILE_CHECK_PERMISSION_DELETE) {
        mask |= DELETE;
    }
    if (access_mask & VPLFILE_CHECK_PERMISSION_EXECUTE) {
        mask |= FILE_GENERIC_EXECUTE;
    }

    // 4. pass to the _VPL_CheckAccessRight() for the result
    rv = _VPLFS__CheckAccessRight(epath, mask, false /* is_checkancestor */);
    if (rv != VPL_OK && rv != VPL_ERR_ACCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unknown permission error: %s, %d",
                         upath.c_str(), rv);
    }

 out:
    if (wpath != NULL) {
        free(wpath);
    }
    if (epath != NULL) {
        free(epath);
    }
    return rv;
}
#endif // defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

int fs_dataset::check_access_right(const std::string& path, u32 access_mask)
{
    int rv = VPL_OK;

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    std::string upath;

    // 1. expands the aliases to the real path

    // return VPL_OK if path = "/", "Computer", "Libraries" & "Libraries/xxx"
    if (path == "" || path == "/") {
        return VPL_OK;
    } else if (path == "Computer" || path == "Computer/") {
        return VPL_OK;
    } else if (path == "Libraries" || (path.find("Libraries/") == 0 &&
               count(path.begin(), path.end(), '/') <= 1)) {
        return VPL_OK;
    }

    // convert to the real path
    rv = get_component_path(path, upath);
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to map component to path: component {%s}, error %d.",
                         path.c_str(), rv);
        goto out;
    }

    // 2. convert to the windows format (backward slashes, wchar_t)
    std::replace(upath.begin(), upath.end(), '/', '\\');

    rv = checkAccessRightHelper(upath,
                                access_mask);
    if (rv != VPL_OK && rv != VPL_ERR_ACCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unknown permission error: %s, %d",
                         path.c_str(), rv);
    }

 out:

#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    return rv;
}

int fs_dataset::get_sibling_components_count(const std::string& component, int& count)
{
    int rv = 0;

    // FIXME (Bug 12999)
    // This implementation (always return count=0) is sufficient for now,
    // as there is currently no need for this function in fs_dataset.

    count = 0;

    return rv;
}


