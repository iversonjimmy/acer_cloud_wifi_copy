/*
 *  Copyright 2014 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF 
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

///
/// vsstsi_internal.hpp
///
/// VSSTS Internal support

#ifndef VSSTS_INTERNAL_HPP_
#define VSSTS_INTERNAL_HPP_

#include "vpl_th.h"
#include "ts_ext_client.hpp"

#define IS_VSSTS        1
#include "vssts_types.hpp"
#undef IS_VSSTS

#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <ccdi_client_tcp.hpp>

#include "vss_comm.h"

#include <map>

namespace vssts {

/// A directory struct, for open/read/close/etc directory.
typedef struct {
    char* raw_data; // raw data from server
    size_t data_len; // length of data from server
    size_t offset; // offset to start of next entry
    VSSI_Dirent2 cur_entry2; // entry that would be shown to client
} VSSI_DirectoryState;

class vssts_pool;

typedef struct vssts_msg_s vssts_msg_t;

class vssts_tunnel {
public:
    vssts_tunnel(u32 tunnel_id, u64 user_id, u64 device_id, vssts_pool* pool);
    ~vssts_tunnel();

    VSSI_Result init(void);
    u32 get_tunnel_id(void) {return tunnel_id;}

    // Send a message. Return VSSI_SUCCESS if the queue op succeed.
    // If a tunnel is closed, or been closing, return VSSI_COMM error.
    VSSI_Result send_msg(vssts_msg_t*& msg);
    void do_callback(vssts_msg_t*& msg);

    void reader(void);
    void writer(void);

    // Call this function to mark a tunnel as no object referring.
    void set_orphan(void);
    bool is_orphan(void);

    // Call this function to trigger TS_EXT::TS_Close(), do actual shutdown.
    void shutdown(void);

private:
    TSError_t recv_msg(vssts_msg_t*& msg, string& error_msg);
    void handle_stat2_reply(VSSI_Dirent2** stats, vssts_msg_t* msg);
    void handle_reply(vssts_msg_t*& msg);
    TSError_t tunnel_read(char* buffer, u32 buffer_len, string& error_msg);
    void handle_notification(vssts_msg_t* msg);

    // Clear msg_map if anything goes wrong.
    void clear_msg_on_fail(bool clear_send_queue = false);

    u64                     user_id;
    u64                     device_id;
    u32                     tunnel_id;
    volatile bool           is_basic_initialized;
    volatile bool           is_reader_thread_started;
    volatile bool           is_reader_thread_stop;
    volatile bool           is_writer_thread_started;
    volatile bool           is_writer_thread_stop;
    volatile bool           is_orphan_tunnel;
    volatile bool           is_shutdown;
    u32                     xid_next;

    vssts_pool*             pool;

    TSIOHandle_t            ioh;
    volatile bool           is_open_done;
    volatile TSError_t      tunnel_failed;  /// Mark this tunnel had failed with some reason
    VPLMutex_t              mutex;
    VPLCond_t               cond;
    VPLCond_t               open_cond;
    VPLThread_t             reader_thread;
    VPLThread_t             writer_thread;
    map<u32,vssts_msg_t*>   send_queue;
    map<u32,vssts_msg_t*>   msg_map;
};

typedef struct vssts_file_s {
    u32             object_id;      /// pointer back to object
    u32             file_id;
    u32             flags;          /// File open mode bits
    u64             origin;         /// Unique origin ID
    u32             server_handle;  /// File handle returned by the server
} vssts_file_t;

/// An instantiated VSSI object
class vssts_object {
public:
    vssts_object(u32 object_id,
                 u64 user_id,
                 u64 dataset_id,
                 u64 device_id,
                 vssts_tunnel* tunnel,
                 vssts_pool* pool);
    ~vssts_object();

    VSSI_Result init(void);
    u32 get_object_id(void) {return object_id;}

    u64 get_version(void) {return version;}
    void set_version(u64 new_version) {version = new_version;}
    u32 get_handle(void) {return handle;}
    void set_handle(u32 new_handle) {handle = new_handle;}

    void delete2(void* ctx, VSSI_Callback callback);
    VSSI_Result open(u8 mode, VSSI_Object* handle, void* ctx, VSSI_Callback callback);
    VSSI_Result close(void* ctx, VSSI_Callback callback);
    void erase(void* ctx, VSSI_Callback callback);
    void mkdir2(const char* name, u32 attrs, void* ctx, VSSI_Callback callback);
    void dir_read(const char* name,
                  VSSI_Dir2* dir,
                  void* ctx,
                  VSSI_Callback callback);
    void stat2(const char* name,
               VSSI_Dirent2** stats,
               void* ctx,
               VSSI_Callback callback);
    void chmod(const char* name,
               u32 attrs,
               u32 attrs_mask,
               void* ctx,
               VSSI_Callback callback);
    void remove(const char* name, void* ctx, VSSI_Callback callback);
    void rename2(const char* name,
                 const char* new_name,
                 u32 flags,
                 void* ctx,
                 VSSI_Callback callback);
    void get_notify_events(VSSI_NotifyMask* mask_out,
                           void* ctx,
                           VSSI_Callback callback);
    void set_notify_events(VSSI_NotifyMask* mask_out,
                           void* notify_ctx,
                           VSSI_NotifyCallback notify_callback,
                           void* ctx,
                           VSSI_Callback callback);
    void set_times(const char* name,
                   VPLTime_t ctime,
                   VPLTime_t mtime,
                   void* ctx,
                   VSSI_Callback callback);
    void get_space(u64* disk_size,
                   u64* dataset_size,
                   u64* avail_size,
                   void* ctx,
                   VSSI_Callback callback);

    void file_open(const char* name,
                   u32 flags,
                   u32 attrs,
                   VSSI_File* file,
                   void* ctx,
                   VSSI_Callback callback);

    VSSI_Result file_close(vssts_file_t* file,
                           void* ctx,
                           VSSI_Callback callback);

    void file_read(vssts_file_t* file,
                   u64 offset,
                   u32* length,
                   char* buf,
                   void* ctx,
                   VSSI_Callback callback);

    void file_write(vssts_file_t* file,
                    u64 offset,
                    u32* length,
                    const char* buf,
                    void* ctx,
                    VSSI_Callback callback);

    void file_truncate(vssts_file_t* file,
                       u64 offset,
                       void* ctx,
                       VSSI_Callback callback);

    void file_chmod(vssts_file_t* file,
                    u32 attrs,
                    u32 attrs_mask,
                    void* ctx,
                    VSSI_Callback callback);

    void file_release(vssts_file_t* file, void* ctx, VSSI_Callback callback);

    void file_set_lock_state(vssts_file_t* file,
                             VSSI_FileLockState lock_state,
                             void* ctx,
                             VSSI_Callback callback);

    void file_get_lock_state(vssts_file_t* file,
                             VSSI_FileLockState* lock_state,
                             void* ctx,
                             VSSI_Callback callback);

    void file_set_byte_range_lock(vssts_file_t* file,
                                  VSSI_ByteRangeLock* br_lock,
                                  u32 flags,
                                  void* ctx,
                                  VSSI_Callback callback);

    void init_hdr(char* hdr, u8 command, u32 data_length);

    void do_callback(u64 mask, const char* buf, u32 length);

    void clear_notify(void);

    void release_tunnel_if_match(u32 tunnel_id, bool force_release = false);
    void repair_tunnel_if_needed(void);

private:

    u64                     user_id;
    u64                     dataset_id;
    u64                     device_id;
    u32                     object_id;
    vssts_tunnel*           tunnel;
    vssts_pool*             pool;
    VPLMutex_t              mutex;

    u64                     version;        /// Current object version
    u32                     handle;         /// object handle
    int                     mode;           /// Object open mode

    /// Callback and context for async notifications received for this object.
    VSSI_NotifyCallback     notify_callback;
    void*                   notify_ctx;
};

struct vssts_msg_s {
    vssts_msg_s(vssts_object* object,
                u8 command,
                char* msg,
                u32 msg_len,
                void *ctx,
                VSSI_Callback callback) :
        xid(0),
        command(command),
        msg(msg),
        msg_data(&msg[VSS_HEADER_SIZE]),
        msg_len(msg_len),
        object_id(0),
        file_id(0),
        io_length(NULL),
        read_buf(NULL),
        ctx(ctx),
        callback(callback),
        ret_handle(NULL),
        ret_file(NULL),
        ret_dir(NULL),
        ret_dirent(NULL),
        ret_disk_size(NULL),
        ret_dataset_size(NULL),
        ret_avail_size(NULL),
        ret_mask_out(NULL),
        ret_lock_state(NULL) {
            object->init_hdr(msg, command, msg_len - VSS_HEADER_SIZE);
            object_id = object->get_object_id();
        };
    vssts_msg_s(char* hdr) :
        xid(vss_get_xid(hdr)),
        command(vss_get_command(hdr)),
        msg(NULL),
        msg_data(NULL),
        msg_len(vss_get_data_length(hdr) + VSS_HEADER_SIZE),
        object_id(0),
        file_id(0),
        io_length(NULL),
        read_buf(NULL),
        ctx(NULL),
        callback(NULL),
        ret_handle(NULL),
        ret_file(NULL),
        ret_dir(NULL),
        ret_dirent(NULL),
        ret_disk_size(NULL),
        ret_dataset_size(NULL),
        ret_avail_size(NULL),
        ret_mask_out(NULL),
        ret_lock_state(NULL) {
            msg = new char[msg_len];
            msg_data = &msg[VSS_HEADER_SIZE];
            memcpy(msg, hdr, VSS_HEADER_SIZE);
        };
    u32             xid;
    u8              command;
    char*           msg;
    char*           msg_data;
    int             msg_len;
    u32             object_id;
    u32             file_id;
    u32*            io_length;
    char*           read_buf;
    void*           ctx;
    VSSI_Callback   callback;
    VSSI_Object*    ret_handle;
    VSSI_File*      ret_file;
    VSSI_Dir2*      ret_dir;
    VSSI_Dirent2**  ret_dirent;
    u64*            ret_disk_size;
    u64*            ret_dataset_size;
    u64*            ret_avail_size;
    VSSI_NotifyMask* ret_mask_out;
    VSSI_FileLockState* ret_lock_state;
};

typedef struct vssts_oref_s {
    vssts_object*       object;
    int                 ref_cnt;
} vssts_oref_t;

typedef struct vssts_tref_s {
    vssts_tunnel*       tunnel;
    int                 ref_cnt;
} vssts_tref_t;

typedef struct vssts_fref_s {
    vssts_file_t*       file;
    int                 ref_cnt;
} vssts_fref_t;

class vssts_pool {
public:
    vssts_pool();
    ~vssts_pool();

    VSSI_Result init(void);

    VSSI_Result object_get(u64 user_id, u64 dataset_id, vssts_object*& object);
    void object_put(vssts_object*& obj);
    vssts_object* object_get(u32 object_id, bool do_tunnel_repair = false);
    vssts_object* object_get_by_handle(u32 object_handle);
    void object_add_handle(vssts_object* obj);

    void tunnel_put(vssts_tunnel*& tunnel);
    vssts_tunnel* tunnel_get(u32 tunnel_id, u64 user_id, u64 device_id);
    void tunnel_release(u32 tunnel_id);
    void tunnel_release_all(void);

    vssts_file_t* file_get(u64 handle, u32 object_id, u32 flags);
    vssts_file_t* file_get(u32 file_id);
    void file_put(vssts_file_t*& file);

    // Clean up files when object is gone
    void file_release_by_object_id(u32 object_id);

    // Tunnel reaper routines
    void tunnel_reaper_add(vssts_tunnel* tunnel);
    void tunnel_reaper_shutdown(void);
    void tunnel_reaper_start(void);
    void tunnel_reaper_signal(void);

    // Shutdown this pool
    void shutdown(void);

private:
    bool lookup_device_id(u64 user_id, u64 dataset_id, u64& device_id);
    void file_release_all(void);
    void object_release_all(void);

    u32                     object_id_next;
    u32                     tunnel_id_next;
    u32                     file_id_next;

    map<u32,vssts_oref_t*>  obj_map;
    map<u32,u32>            obj_handle_map;
    VPLMutex_t              mutex;
    VPLCond_t               cond;
    map<u64,u64>            device_map;     // dataset to device id xlation
    map<u32,vssts_tref_t*>  tunnel_map;
    map<u32,vssts_fref_t*>  file_map;

    // Class members for tunnel reaper thread
    list<vssts_tunnel*>     tunnel_reap_list;
    bool                    tunnel_reaper_stop;
    VPLThread_t             tunnel_reaper_thr;

    volatile bool           is_shutdown;
};

VSSI_Result VSSI_InternalInit(void);
void VSSI_InternalCleanup(void);

}

#endif // include guard
