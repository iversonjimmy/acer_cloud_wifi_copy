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

#ifndef STORAGE_NODE__MANAGED_DATASET_HPP__
#define STORAGE_NODE__MANAGED_DATASET_HPP__

/// @file
/// Managed Dataset class
/// Managed dataset for personal cloud.

#include "vplu_types.h"
#include "vpl_th.h"
#include "vpl_time.h"

#include <string>
#include <utility> // for pair
#include <queue>
#include <map>
#include <sstream>

#include "vssi_types.h"
#include "vss_file.hpp"

#include "vss_server.hpp"

#include "DatasetDB.hpp"

#define RESERVED_PERCENT_OF_SPACE 0.01
#define RESERVED_TIMES_OF_DBSIZE 3

// Maximum number of the child components for a component
#define MAXIMUM_COMPONENTS (64*1024)

enum {
    DB_ERR_CLASS_USER,
    DB_ERR_CLASS_SWHW,
    DB_ERR_CLASS_SCHEMA,
    DB_ERR_CLASS_DATABASE,
    DB_ERR_CLASS_SYSTEM
};

typedef struct {
    std::string     component;
    u64             size;
} md_filesize;

class managed_dataset : public dataset
{
public:
    managed_dataset(dataset_id& id, int type, std::string& storage_base, vss_server* server);
    virtual ~managed_dataset();

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

    void file_periodic_sync(void);
    void dbase_backup(void);
    void dataset_check();


    virtual int get_space(const std::string& path, u64& disk_size, u64& avail_size);

    virtual s16 get_component_path(const std::string &component, std::string &path);

protected:
    VPL_DISABLE_COPY_AND_ASSIGN(managed_dataset);

    // Dataset base directory
    std::string dir_name;
    std::string tmp_to_delete_dir;

    u64 version;

    VPLMutex_t fd_mutex;

    DatasetDB datasetDB;

    // dataset options
    u32 datasetdb_options;

    s16 commit_iw(void);
    s16 set_metadata_iw(const std::string& component,
                        u8 type,
                        u8 length,
                        const char* data);

    s16 remove_iw(const std::string& component,
                  bool rename_target);
    s16 rename_iw(const std::string& component,
                  const std::string& new_name,
                  u32 flags);
    s16 set_size_iw(const std::string& component, u64 size);
    s16 set_times_iw(const std::string& component, u64 ctime,
                     u64 mtime);
    s16 make_directory_iw(const std::string& component, u32 attrs);
    s16 chmod_iw(const std::string& component, u32 attrs, u32 attrs_mask);

    // Support for open file handles and locking

    s16 open_file(const std::string& component,
                  u64 version,
                  u32 flags,
                  u32 attrs,
                  vss_file*& file_handle);
    s16 read_file(vss_file* file,
                  vss_object* object,
                  u64 origin,
                  u64 offset,
                  u32& length,
                  char* data);
    s16 write_file(vss_file* file,
                   vss_object* object,
                   u64 origin,
                   u64 offset,
                   u32& length,
                   const char* data);
    s16 truncate_file(vss_file* file,
                      vss_object* object,
                      u64 origin,
                      u64 offset);
    s16 chmod_file(vss_file* file,
                   vss_object* object,
                   u64 origin,
                   u32 attrs,
                   u32 attrs_mask);
    s16 release_file(vss_file* file,
                     vss_object* object,
                     u64 origin);
    s16 close_file(vss_file* file,
                   vss_object* object,
                   u64 origin);
    vss_file* map_file_id(u32 file_id);

    void object_release(vss_object* object);
 
    virtual int check_access_right(const std::string& path, u32 access_mask);

    virtual int get_sibling_components_count(const std::string& component, int& count);
private:
    s16 elaborate_unknown_component_error(const std::string &name);
    bool component_exists(const std::string &name);
    int delete_component(const std::string& component, VPLTime_t time);
    bool remove_allowed(const std::string& component,
                        vss_file*& fp,
                        bool rename_target);
    // Rename a file or directory.
    int rename_component(const std::string& source,
                         const std::string& destination,
                         VPLTime_t time,
                         u32 flags);
    bool check_open_conflict(const std::string& component);
    int make_dataset_directory();
    int merge_timestamp(const std::string& component,
                        u64 ctime, u64 mtime,
                        u64 version, u32 index,
                        u64 origin_device, VPLTime_t time);                       
    s16 create_directory(const std::string& component,
                         VPLTime_t time=0,
                         u32 attrs=0);
    DatasetDBError append_dirent(const ComponentInfo &info,
                                 std::ostringstream& data_out,
                                 bool json);
    DatasetDBError append_dirent(const std::string& component,
                                 std::ostringstream& data_out,
                                 bool json);
    DatasetDBError append_dirent2(const ComponentInfo &info,
                                  std::ostringstream& data_out,
                                  bool json);
    DatasetDBError append_dirent2(const std::string& component,
                                  std::ostringstream& data_out,
                                  bool json);
    void append_dirent2_pagination(const ComponentInfo &info,
                                   std::vector< std::pair<std::string, ComponentInfo> >& filelist);
    void update_component_info_to_cache(ComponentInfo &info);
    bool component_is_directory(const std::string &name);


    void mini_trans_start(const char *str);
    void mini_trans_end(const char *str, bool trigger_commit = false);
    void file_update_database(vss_file* file);
    void period_thread_start(void);
    void period_thread_stop(void);
    void tran_commit(void);
    void tran_rollback(void);
    int dbase_backup_start(void);
    void dbase_backup_end(bool force_end);
    int dbase_fsck_start(void);
    void dbase_fsck_end(void);
    int db_err_to_class(DatasetDBError dbrv);
    int db_err_classify(DatasetDBError dbrv);
    int get_remaining_space(const std::string& path, u64& disk_size, u64& avail_size);
    void set_marker(const std::string& path);
    bool test_marker(const std::string& path);
    void clear_marker(const std::string& path);
    void mark_backup_err(void);
    void mark_fsck_err(void);
    void clear_dbase_err(void);
    void stop_ccd(void);
    void hold_off_fsck(void);

    void dbase_activate(void);

    bool mini_tran_open;
    bool tran_is_open;
    bool tran_do_commit;
    bool tran_needs_commit;
    bool dbase_needs_backup;
    bool backup_done;
    bool fsck_done;
    bool fsck_needed;
    bool shutdown_sync;

    VPLTime_t last_backup;
    VPLTime_t last_action;
    VPLTime_t last_commit;
    VPLThread_t per_sync_thread;
    VPLThread_t backup_thread;
    VPLThread_t fsck_thread;

    std::map<std::string, vss_file*> open_handles;
    std::map<u32, vss_file*> file_id_to_handle;

    VPLMutex_t sync_mutex;
    VPLMutex_t access_mutex;
    VPLCond_t per_sync_cond;
    u64 remaining_space_size;
};

#endif // include guard
