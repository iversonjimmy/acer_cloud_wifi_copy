/*
 *  Copyright 2012 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#ifndef STORAGE_NODE__VSS_FILE_HPP__
#define STORAGE_NODE__VSS_FILE_HPP__

/// @file
/// VSS File Handle class
/// File handle support for datasets for personal cloud.

#include <string>
#include <deque>
#include <map>
#include <vector>

#include "vpl_th.h"
#include "vplex_file.h"
#include "vplu_types.h"

#include "vssi_types.h"

// Forward decls ...
class dataset;
class vss_object;
class vss_req_proc_ctx;
class vss_file;

class vss_flock
{
private:
public:
    vss_object* object; // originating remote client object
    u64 origin;         // originating remote client file handle
    u64 mode;           // lock mode
    u64 start;          // start of byte range
    u64 end;            // end of locked byte range (next unlocked byte)

    vss_flock* next;    // singly linked list from vss_file

    vss_flock(vss_object* object, u64 origin, u64 mode, u64 start, u64 end);
    ~vss_flock();

    bool overlaps(u64 start, u64 end);
    bool matches_object(vss_object* object);
    bool matches_origin(u64 origin);
    bool matches(u64 origin, u64 mode, u64 start, u64 end);
    bool io_conflicts(u64 origin, u64 mode, u64 start, u64 end);
    bool lock_conflicts(u64 origin, u64 mode, u64 start, u64 end);
};

class vss_oplock
{
private:
    vss_object* object; // identifies originating remote client
    u64 mode;           // lock mode

    void notify(vss_file* file);  // send oplock break notification

public:
    vss_oplock(vss_object* object,
               u64 mode);
    ~vss_oplock();

    bool conflicts(vss_object* object,
                   u64 mode);
    bool matches(vss_object* object);
    void set_mode(u64 mode);
    void get_mode(u64& mode);
    bool need_to_break(vss_file* file,
                       vss_object* object,
                       bool write_request,
                       bool send_break);

    vss_oplock* next;   // singly linked list from vss_file
};

class vss_file
{
private:
    int refcount;
    u32 access_mode;      // open and sharing flags
    u32 attrs_orig;
    u32 attrs;
    u64 size_orig;
    u64 size;
    bool is_modified;
    bool new_modify_time;
    bool delete_pending;
    dataset* mydataset;   // backpointer to dataset
    u32 file_id;          // cookie passed back to client

    vss_flock* locks;     // chain of file lock structs
    int numlocks;         // count the lock chain

    vss_oplock* oplocks;  // chain of oplock structs
    int numoplocks;       // count the oplock chain

    VPLFile_handle_t fd;  // actual file descriptor

    struct object_reference {
        vss_object* object;
        int refcount;
    };
    std::map<vss_object*, object_reference*> object_refs;

    // Track the access modes individually per origin
    std::map<u64, u32> origin_flags;
    // Track every origin that has the file open 
    std::vector<u64> open_origins;

    struct oplock_wait {
        vss_req_proc_ctx* context;
        vss_object* object;
        bool write_request;
    };
    std::deque<oplock_wait*> oplock_queue;

    struct brlock_wait {
        vss_req_proc_ctx* context;
        u64 origin;
        u64 mode;
        u64 start;
        u64 end;
    };
    std::deque<brlock_wait*> brlock_queue;

    // Mutex to protect the lock chains and wait queues
    VPLMutex_t flock_mutex;

    bool add_range_lock(vss_object* object,
                        u64 origin,
                        u64 mode,
                        u64 start,
                        u64 end);
    bool remove_range_lock(u64 origin,
                           u64 mode,
                           u64 start,
                           u64 end);
    bool wake_range_lock(u64 origin,
                         u64 mode,
                         u64 start,
                         u64 end);
    bool remove_oplocks(vss_object *object);
    bool remove_brlocks(vss_object *object);

    bool release_brlocks(u64 origin);

    void wake_all();

    bool check_mode_conflict(u32 oldflags, u32 newflags);

public:
    vss_file(dataset* dset,
             VPLFile_handle_t fd,
             const std::string& component,
             const std::string& filename,
             u32 flags,
             u32 attrs,
             u64 size,
             bool is_modified = false);
    ~vss_file();

    VPLFile_handle_t get_fd();

    std::string component;
    std::string filename;   // Real content file name

    dataset* get_dataset();
    void set_file_id(u32 file_id);
    u32 get_file_id();
    u32 get_access_mode();
    void add_origin(u64 origin, u32 flags);
    void release_origin(u64 origin);
    bool validate_origin(u64 origin);
    void add_open_origin(u64 origin);
    void remove_open_origin(u64 origin);
    u32 get_attrs(void) {return attrs;};
    void set_attrs(u32 new_attrs);
    bool sync_attrs(u32& attrs);
    u64 get_size(void) {return size;}
    void set_size(u64 new_size);
    void set_size_if_larger(u64 new_size);
    bool sync_size(u64& attrs);
    bool get_new_modify_time(void) {return new_modify_time;};
    void set_new_modify_time(bool val) {new_modify_time = val;};
    bool get_is_modified(void) {return is_modified;};
    void set_is_modified(bool val) {is_modified = val;};
    bool delete_is_pending(void) {return delete_pending;};
    void set_delete_pending(void);
    s16 add_reference(u32 flags);
    s16 remove_reference(vss_object* object,
                         u64 origin);
    int get_refcount() {return refcount;};
    bool referenced();
    s16 add_object(vss_object* object);
    bool remove_object(vss_object* object);
    s16 set_lock_range(vss_object* object,
                       u64 origin,
                       u32 flags,
                       u64 mode,
                       u64 start,
                       u64 end);
    bool check_lock_conflict(u64 origin,
                             u64 mode,
                             u64 start,
                             u64 end);
    s16 set_oplock(vss_object* object,
                   u64 mode);
    s16 get_oplock(vss_object* object,
                   u64& mode);
    bool check_oplock_conflict(vss_object* object,
                               bool write);
    bool wait_oplock(vss_req_proc_ctx* context,
                     vss_object* object,
                     bool write);
    bool wait_brlock(vss_req_proc_ctx* context,
                     u64 origin,
                     u64 mode,
                     u64 start,
                     u64 end);
};

#endif // include guard
