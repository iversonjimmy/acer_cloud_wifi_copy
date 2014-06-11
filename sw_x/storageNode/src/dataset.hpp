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

#ifndef STORAGE_NODE__DATASET_HPP__
#define STORAGE_NODE__DATASET_HPP__

/// @file
/// Generic Dataset class
/// Only identification info, synchronization objects, and timestamps.
/// In particular, actual file I/O functions are defined in the derived classes.

class dataset_id;
class dataset;

#include "vplu_types.h"
#include "vpl_th.h"
#include "vpl_fs.h"
#include "vpl_time.h"

#include <string>
#include <deque>
#include <climits>

#include "vssi_types.h"

#include "vss_server.hpp"
#include "vss_object.hpp"
#include "vss_file.hpp"

class dataset_id
{
public:
    dataset_id() {};
    dataset_id(u64 uid, u64 did) : uid(uid), did(did) {}
    ~dataset_id() {};

    bool operator<(const dataset_id& rhs) const
    {
        if(uid == rhs.uid) {
            return (did < rhs.did);
        }
        else return (uid < rhs.uid);
    };

    bool operator==(const dataset_id& rhs) const
    {
        if(uid == rhs.uid && did == rhs.did)
            return true;
        else
            return false;
    };

    u64 uid; // user ID
    u64 did; // dataset ID
};

/// Trash record properties
typedef struct {
    u64 version;      /// Record ID: version
    u32 index;        /// Record ID: index
    u64 ctime;        /// Trashed component creation time (us since epoch)
    u64 mtime;        /// Trashed component modification time (us since epoch)
    u64 dtime;        /// Trashed component deletion time (us since epoch)
    bool isDir;       /// Trashed component is a directory
    u64 size;         /// Trashed component size (bytes)
#ifdef _MSC_VER
#pragma warning(disable: 4200)  // disable zero-sized array warning in Visual Studio
#endif // _MSC_VER
    char component[]; /// Trashed component's original path and name. NULL terminated.
#ifdef _MSC_VER
#pragma warning(default: 4200)  // enable zero-sized array warning in Visual Studio
#endif // _MSC_VER
} trash_record;

class dataset
{
public:
    vss_server* server;

    // Create new dataset access object.
    // Will hold one reference for activation.
    dataset(dataset_id& id, int type, vss_server* server);
    virtual ~dataset();

    // Indicate dataset has an active object reference.
    void reserve(void);
    // Indicate an object released a reference.
    void release(void);
    // Return number of active dataset references.
    int num_references();

    bool is_invalid() {return invalid;}
    bool get_state_failed()  {return is_failed;}

    const dataset_id& get_id();

    bool is_case_insensitive();

    // Object opens this dataset.
    void open(vss_object* object);

    // Object closes this dataset, releases taken locks.
    void close(vss_object* object);

    virtual void close_unused_files(void) = 0;

    virtual u64 get_version(void) = 0;

    virtual s16 read_dir(const std::string& component,
                         std::string& data_out,
                         bool json = false,
                         bool pagination = false,
                         const std::string& sort="time",
                         u32 index = 1,
                         u32 max = UINT_MAX) = 0;

    virtual s16 read_dir2(const std::string& component,
                          std::string& data_out,
                          bool json = false,
                          bool pagination = false,
                          const std::string& sort="time",
                          u32 index = 1,
                          u32 max = UINT_MAX) = 0;

    // Stat a component file or directory.
    virtual int stat_component(const std::string& component,
                               std::string& data_out) = 0;
    virtual int stat_component(const std::string& component,
                               VPLFS_stat_t& stat_out) = 0;
    virtual int stat2_component(const std::string& component,
                                std::string& data_out) = 0;
    virtual int stat_shortcut(const std::string& component,
                              std::string& path,
                              std::string& type,
                              std::string& args) = 0;

    virtual s16 delete_all() = 0;

    virtual int get_total_used_size(u64& size) = 0;

    virtual s16 commit_iw(void) = 0;
    virtual s16 set_metadata_iw(const std::string& component,
                                u8 type,
                                u8 length,
                                const char* data) = 0;

    virtual s16 remove_iw(const std::string& component,
                          bool rename_target = false) = 0;
    virtual s16 rename_iw(const std::string& component,
                          const std::string& new_name,
                          u32 flags = 0) = 0;
    virtual s16 set_size_iw(const std::string& component, u64 size) = 0;
    virtual s16 set_times_iw(const std::string& component, u64 ctime,
                             u64 mtime) = 0;
    virtual s16 make_directory_iw(const std::string& component, u32 attrs) = 0;
    virtual s16 chmod_iw(const std::string& component, u32 attrs, u32 attrs_mask) = 0;

    s16 copy_file(const std::string& src, const std::string& dst);

    // file handle support
    virtual s16 open_file(const std::string& component,
                          u64 version,
                          u32 flags,
                          u32 attrs,
                          vss_file*& file_handle) = 0;
    virtual s16 read_file(vss_file* file,
                          vss_object* object,
                          u64 origin,
                          u64 offset,
                          u32& length,
                          char* data) = 0;
    virtual s16 write_file(vss_file* file,
                           vss_object* object,
                           u64 origin,
                           u64 offset,
                           u32& length,
                           const char* data) = 0;
    virtual s16 truncate_file(vss_file* file,
                              vss_object* object,
                              u64 origin,
                              u64 offset) = 0;
    virtual s16 chmod_file(vss_file* file,
                           vss_object* object,
                           u64 origin,
                           u32 attrs,
                           u32 attrs_mask) = 0;
    virtual s16 release_file(vss_file* file,
                             vss_object* object,
                             u64 origin) = 0;
    virtual s16 close_file(vss_file* file,
                           vss_object* object,
                           u64 origin) = 0;
    virtual vss_file* map_file_id(u32 file_id) = 0;

    virtual int get_space(const std::string& path, u64& disk_size, u64& avail_size) = 0;

    // for remote file to check the permission to enforce the user access control
    virtual int check_access_right(const std::string& path, u32 access_mask) = 0;

    virtual int get_sibling_components_count(const std::string& component, int& count) = 0;

    virtual s16 get_component_path(const std::string &component, std::string &path) = 0;
protected:
    VPL_DISABLE_COPY_AND_ASSIGN(dataset);

    VPLMutex_t mutex;

    int reserve_count;

    bool is_failed;
    bool invalid;

    dataset_id id;
    int ds_type;

    // Objects opened against this dataset (for debugging).
    std::set<vss_object*> objects;

    virtual void object_release(vss_object *object) = 0;

    void lock();
    void unlock();

private:

};

#endif // include guard
