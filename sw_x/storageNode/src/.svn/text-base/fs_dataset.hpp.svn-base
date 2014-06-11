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

#ifndef STORAGE_NODE__FS_DATASET_HPP__
#define STORAGE_NODE__FS_DATASET_HPP__

/// @file
/// FS Dataset class
/// FS dataset for personal cloud.

#include "vplu_types.h"
#include "vpl_fs.h"
#include "vpl_th.h"
#include "vpl_time.h"
#include "vplex_file.h"

#include <string>
#include <map>

#include "vssi_types.h"

#include "vss_server.hpp"

class vss_file;

class fs_dataset : public dataset
{
public:
    fs_dataset(dataset_id& id, int type, vss_server* server);
    virtual ~fs_dataset();
    
    typedef struct {
        VPLFS_stat_t stat;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        /// bug 10824: optional information for shortcut
        /// target path
        std::string target_path;  
        /// file or dir
        std::string target_type;
        /// arguments
        std::string target_args;
        /// bool for shortcut
        bool isShortCut;
#endif
    } file_stat_t;

    virtual bool has_transactional_support();

    virtual void close_unused_files(void);

    virtual u64 get_version(void);

    virtual s16 read_dir(const std::string& component,
                         std::string& data_out,
                         bool json = false,
                         bool pagination = false,
                         const std::string& sort="time",
                         u32 index = 1,
                         u32 max = UINT_MAX);

    virtual s16 read_dir2(const std::string& component,
                          std::string& data_out,
                          bool json = false,
                          bool pagination = false,
                          const std::string& sort="time",
                          u32 index = 1,
                          u32 max = UINT_MAX);

    virtual int stat_component(const std::string& component,
                               std::string& data_out);
    virtual int stat_component(const std::string& component,
                               VPLFS_stat_t& stat_out);
    virtual int stat2_component(const std::string& component,
                                std::string& data_out);
    virtual int stat_shortcut(const std::string& component,
                              std::string& path,
                              std::string& type,
                              std::string& args);
    virtual s16 delete_all();

    virtual int get_total_used_size(u64& size);

    virtual int get_space(const std::string& path, u64& disk_size, u64& avail_size);

    virtual int check_access_right(const std::string& path, u32 access_mask);

    virtual int get_sibling_components_count(const std::string& component, int& count);

    virtual s16 get_component_path(const std::string &component, std::string &path);

    static int rfPathToAbsPath(
                      const std::string&     pathIn,
                      const std::map<std::string, std::string>& prefixRewriteRules,
                      std::string& absPath_out);

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    static void fs_dataset::refreshPrefixRewriteRulesHelper(
                      std::set<std::string>& winValidVirtualFolders_out,
                      std::map<std::string, std::string>& prefixRewriteRules_out,
                      std::map<std::string, _VPLFS__LibInfo>& winLibFolders_out);
    static int checkAccessRightHelper(const std::string& upath,
                                      u32 access_mask);

#endif  // defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

protected:
    VPL_DISABLE_COPY_AND_ASSIGN(fs_dataset);

    struct fileinfo {
        VPLFile_handle_t h;
        int count;
        fileinfo() : h(VPLFILE_INVALID_HANDLE), count(0) {}
        ~fileinfo() {
            if (VPLFile_IsValidHandle(h)) { VPLFile_Close(h); }
        }
    };

    // File handles while reading, integrating changes
    std::map<std::string, fileinfo*> open_fds;
    static const unsigned int OPEN_FD_LIMIT = 10;
    VPLFile_handle_t get_component_fd(const std::string& component, bool writeable, s16& error);
    void release_component_fd(const std::string& component, bool writeable);
    void close_component_fd(const std::string& component);
    void close_component_fds();
    void close_unused_files_unlocked();

    VPLMutex_t fd_mutex;


    s16 check_existence(const std::string& component, bool& existed);

    /// Protects winValidVirtualFolders, winLibFolders, and prefixRewriteRules.
    VPLMutex_t prefix_rewrite_mutex;

#ifdef WIN32
    VPLTime_t winLastRefreshTimeStamp;
    std::set<std::string> winValidVirtualFolders;
    std::map<std::string, _VPLFS__LibInfo> winLibFolders;
#endif // WIN32

    std::map<std::string, std::string> prefixRewriteRules;
    void refreshPrefixRewriteRules(VPLTime_t refreshThreshold = VPLTIME_MICROSEC_PER_SEC);

    // Append info about the file to data_out.
    // <0 return means error; >=0 return means number of entries added.
    s16 append_dirent(const std::string& filename,
                      const file_stat_t& stat,
                      std::ostringstream& data_out,
                      bool json);
    s16 append_dirent(const std::string& folder_path,
                      const std::string& filename,
                      std::ostringstream& data_out,
                      bool json);

#ifdef WIN32
    s16 read_dir_win_libraries(const std::string& component,
                               std::string& data_out,
                               bool json);
    s16 read_dir_win_root(const std::string& component,
                          std::string& data_out,
                          bool json);
    s16 read_dir_win_computer(const std::string& component,
                              std::string& data_out,
                              bool json);
#endif // WIN32
    s16 read_dir_fs(const std::string& folder_path,
                    const std::string& component,
                    std::string& data_out,
                    bool json);

    s16 read_dir_fs_pagination(const std::string& folder_path,
                               const std::string& component,
                               std::string& data_out,
                               bool json,
                               const std::string& sort,
                               u32 index,
                               u32 max);

    bool component_is_directory(const std::string &name);

    s16 create_component(const std::string& component);

    s16 create_directory(const std::string& path);

    s16 rename_component(const std::string& component, const std::string& new_component);

    s16 copy_component(const std::string& component, const std::string& new_component);

    s16 delete_component(const std::string& component);

    s16 delete_dir(const std::string& path);

    bool directory_exists(const std::string &name);

    int copy_file(const std::string& src,
                  const std::string& dst,
                  char* tempChunkBuf,
                  u32 tempChunkBufSize);

    // Make any necessary changes to the dataset (including trashing components) 
    // to accommodate the given component name as a file component.
    int accomodate_file_component(const std::string& name, bool writeable);

    virtual s16 commit_iw(void);
    virtual s16 set_metadata_iw(const std::string& component,
                                u8 type,
                                u8 length,
                                const char* data);

    virtual s16 remove_iw(const std::string& component,
                          bool rename_target);
    virtual s16 rename_iw(const std::string& component,
                          const std::string& new_name,
                          u32 flags);
    virtual s16 set_size_iw(const std::string& component, u64 size);
    virtual s16 set_times_iw(const std::string& component, u64 ctime,
                             u64 mtime);
    virtual s16 make_directory_iw(const std::string& component, u32 attrs);
    virtual s16 chmod_iw(const std::string& component, u32 attrs, u32 attrs_mask);

    //////////////////////////////////////////////////////
    ////////////////// FILE HANDLE API ///////////////////
    virtual s16 open_file(const std::string& component,
                          u64 version,
                          u32 flags,
                          u32 attrs,
                          vss_file*& fhandle_out);
    virtual s16 read_file(vss_file* fhandle,
                          vss_object* object,
                          u64 origin,
                          u64 offset,
                          u32& length_in_out,
                          char* data_out);
    virtual s16 write_file(vss_file* fhandle,
                           vss_object* object,
                           u64 origin,
                           u64 offset,
                           u32& length_in_out,
                           const char* data);
    virtual s16 truncate_file(vss_file* fhandle,
                              vss_object* object,
                              u64 origin,
                              u64 offset);
    virtual s16 chmod_file(vss_file* file,
                           vss_object* object,
                           u64 origin,
                           u32 attrs,
                           u32 attrs_mask);
    virtual s16 release_file(vss_file* fhandle,
                             vss_object* object,
                             u64 origin);
    virtual s16 close_file(vss_file* fhandle,
                           vss_object* object,
                           u64 origin);
    virtual vss_file* map_file_id(u32 file_id);
    ////////////////// FILE HANDLE API ///////////////////
    //////////////////////////////////////////////////////
    virtual void object_release(vss_object* object);
};

#endif // include guard
