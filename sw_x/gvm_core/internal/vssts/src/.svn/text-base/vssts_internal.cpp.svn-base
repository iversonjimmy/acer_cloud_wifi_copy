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
/// vssts_internal.cpp
///
/// VSSTS internal support code

#include "vpl_th.h"
#include "vplex_assert.h"
#include "vplex_trace.h"

#include <stdlib.h>

#define IS_VSSTS        1
#include "vssts.hpp"
#undef IS_VSSTS
#include "vssts_internal.hpp"
#include "vssts_error.hpp"

using namespace std;

namespace vssts {

#define VSSTS_REAPER_STACK_SIZE     (32*1024)
#define VSSTS_READER_STACK_SIZE     (32*1024)
#define VSSTS_WRITER_STACK_SIZE     (32*1024)

#define VSSTS_OPEN_WAIT_TIMEOUT_IN_SEC  (60)

bool vssts_pool::lookup_device_id(u64 user_id, u64 dataset_id, u64& device_id)
{
    int rv;
    bool is_found = false;
    ccd::ListOwnedDatasetsInput listOdIn;
    ccd::ListOwnedDatasetsOutput listOdOut;

    VPLMutex_Lock(&mutex);
    {
        map<u64,u64>::iterator it;

        it = device_map.find(dataset_id);
        if ( it != device_map.end() ) {
            device_id = it->second;
            is_found = true;
        }
    }
    VPLMutex_Unlock(&mutex);

    if ( is_found ) {
        goto done;
    }

    listOdIn.set_user_id(user_id);
    // XXX - Is this correct?
    listOdIn.set_only_use_cache(true);

    rv = CCDIListOwnedDatasets(listOdIn, listOdOut);
    if ( rv != 0 ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "CCDIListOwnedDatasets() - %d", rv);
        goto done;
    }

    for( int i = 0 ; i < listOdOut.dataset_details_size() ; i++ ) {
        if ( listOdOut.dataset_details(i).datasetid() != dataset_id ) {
            continue;
        }
        is_found = true;
        device_id = listOdOut.dataset_details(i).clusterid();
    }

    if ( !is_found ) {
        rv = -1;
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "dataset "FMTu64 " not found",
            dataset_id);
        goto done;
    }

    VPLMutex_Lock(&mutex);
    device_map[dataset_id] = device_id;
    VPLMutex_Unlock(&mutex);

done:
    return is_found;
}

vssts_object::vssts_object(u32 object_id,
                           u64 user_id,
                           u64 dataset_id,
                           u64 device_id,
                           vssts_tunnel* tunnel,
                           vssts_pool* pool) :
    user_id(user_id),
    dataset_id(dataset_id),
    device_id(device_id),
    object_id(object_id),
    tunnel(tunnel),
    pool(pool),
    version(0),
    handle(0),
    mode(0),
    notify_callback(NULL),
    notify_ctx(NULL)
{
    VPL_SET_UNINITIALIZED(&mutex);
}

vssts_object::~vssts_object()
{
    // XXX - It's unsafe to release the files without checking the
    // reference counts.  Since the object cannot easily determine
    // when the tunnel is completely shut down, it's hard for the
    // destructor to de-allocate the files, which may be accessed
    // by the tunnel's reader thread.  It's better to just leave
    // things as is, and count on the caller to call VSSI_CloseFile()
    // to close the files appropriately.  Eventually, VSSI_Cleanup()
    // will also de-allocate all the outstanding files
    //VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Release files held by object");
    //pool->file_release_by_object_id(object_id);

    if ( tunnel != NULL ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Release tunnel held by object");
        pool->tunnel_put(tunnel);
    }

    if ( VPL_IS_INITIALIZED(&mutex) ) {
        VPLMutex_Destroy(&mutex);
    }
}

VSSI_Result vssts_object::init(void)
{
    VSSI_Result rv = VSSI_NOMEM;

    if ( VPLMutex_Init(&mutex) != VPL_OK ) {
        goto done;
    }

    rv = VSSI_SUCCESS;

done:
    return rv;
}

vssts_tunnel::vssts_tunnel(u32 tunnel_id, u64 user_id, u64 device_id, vssts_pool* pool) :
    user_id(user_id),
    device_id(device_id),
    tunnel_id(tunnel_id),
    is_basic_initialized(false),
    is_reader_thread_started(false),
    is_reader_thread_stop(false),
    is_writer_thread_started(false),
    is_writer_thread_stop(false),
    is_orphan_tunnel(false),
    is_shutdown(false),
    xid_next(0),
    pool(pool),
    ioh(NULL),
    is_open_done(false),
    tunnel_failed(TS_OK)
{
    VPL_SET_UNINITIALIZED(&mutex);
    VPL_SET_UNINITIALIZED(&cond);
    VPL_SET_UNINITIALIZED(&open_cond);
}

vssts_tunnel::~vssts_tunnel()
{
    // If basic stuff like mutexes are not initialized, do not attempt to do shutdown()
    if(is_basic_initialized && !is_shutdown) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "vssts_tunnel(%p) is getting destroyed before shutdown", this);
        shutdown();
    }

    if ( VPL_IS_INITIALIZED(&mutex) ) {
        VPLMutex_Destroy(&mutex);
    }

    if ( VPL_IS_INITIALIZED(&cond) ) {
        VPLCond_Destroy(&cond);
    }

    if ( VPL_IS_INITIALIZED(&open_cond) ) {
        VPLCond_Destroy(&open_cond);
    }
}

// Clean up routine
static VPLThread_return_t pool_tunnel_reaper_start(VPLThread_arg_t poolv)
{
    vssts_pool* pool = (vssts_pool*)poolv;

    pool->tunnel_reaper_start();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static VPLThread_return_t tunnel_reader_start(VPLThread_arg_t tunnelv)
{
    vssts_tunnel* tunnel = (vssts_tunnel*)tunnelv;

    tunnel->reader();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static VPLThread_return_t tunnel_writer_start(VPLThread_arg_t tunnelv)
{
    vssts_tunnel* tunnel = (vssts_tunnel*)tunnelv;

    tunnel->writer();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

VSSI_Result vssts_tunnel::init(void)
{
    VSSI_Result rv = VSSI_NOMEM;

    if ( VPLMutex_Init(&mutex) != VPL_OK ) {
        goto done;
    }

    if ( VPLCond_Init(&cond) != VPL_OK ) {
        goto done;
    }

    if ( VPLCond_Init(&open_cond) != VPL_OK ) {
        goto done;
    }

    is_basic_initialized = true;

    // Start up a thread to handle read accesses.
    {
        int rc;

        VPLThread_attr_t thread_attr;
        VPLThread_AttrInit(&thread_attr);
        VPLThread_AttrSetStackSize(&thread_attr, VSSTS_READER_STACK_SIZE);
        VPLThread_AttrSetDetachState(&thread_attr, false);

        is_reader_thread_stop = false;
        rc = VPLThread_Create(&reader_thread, tunnel_reader_start,
            VPL_AS_THREAD_FUNC_ARG(this), &thread_attr,
            "TS service handler");
        if ( rc != VPL_OK ) {
            rv = VSSI_NOMEM;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed start of tunnel reader");
            goto done;
        }
        is_reader_thread_started = true;
    }

    // Start up a thread to handle write accesses.
    {
        int rc;

        VPLThread_attr_t thread_attr;
        VPLThread_AttrInit(&thread_attr);
        VPLThread_AttrSetStackSize(&thread_attr, VSSTS_WRITER_STACK_SIZE);
        VPLThread_AttrSetDetachState(&thread_attr, false);

        is_writer_thread_stop = false;
        rc = VPLThread_Create(&writer_thread, tunnel_writer_start,
            VPL_AS_THREAD_FUNC_ARG(this), &thread_attr,
            "TS service handler");
        if ( rc != VPL_OK ) {
            rv = VSSI_NOMEM;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed start of tunnel reader");
            goto done;
        }
        is_writer_thread_started = true;
    }

    rv = VSSI_SUCCESS;

done:
    // In the case of an error, the destructor will join the threads since is_shutdown
    // would be false, as long as the basic initializations have completed successfully

    return rv;
}

TSError_t vssts_tunnel::tunnel_read(char* buffer,
                                    u32 buffer_len,
                                    string& error_msg)
{
    TSError_t ts_err = TS_OK;
    size_t tot_want = buffer_len;
    size_t tot_recvd = 0;

    while (tot_want > 0) {
        ts_err = TS_EXT::TS_Read(ioh, &buffer[tot_recvd], tot_want, error_msg);
        if ( ts_err != TS_OK ) {
            if ( ts_err == TS_ERR_TIMEOUT ) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS_EXT::TS_Read timeout detected, continue...");
                continue;
            }
            goto done;
        }

        tot_recvd += tot_want;
        tot_want = buffer_len - tot_recvd;
    }

done:
    return ts_err;
}

TSError_t vssts_tunnel::recv_msg(vssts_msg_t*& msg, string& error_msg)
{
    TSError_t ts_err;
    char hdr[VSS_HEADER_SIZE];
    u32 data_length;

    // init msg so it won't be deleted when tunnel_read is failed
    msg = NULL;

    // first pull out the header
    ts_err = tunnel_read(hdr, sizeof(hdr), error_msg);
    if ( ts_err != TS_OK ) {
        // Only logging it if we didn't shutdown explicitly
        if(!is_reader_thread_stop) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "tunnel_read(%p) %d:%s",
                ioh, ts_err, error_msg.c_str());
        }
        goto done;
    }

    // return once we have a complete packet
    msg = new (std::nothrow) vssts_msg_t(hdr);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate msg");
        ts_err = TS_ERR_NO_MEM;
        goto done;
    }

    // now, pull out the body
    data_length = msg->msg_len - VSS_HEADER_SIZE;
    
    // first pull out the header
    ts_err = tunnel_read(msg->msg_data, data_length, error_msg);
    if ( ts_err != TS_OK ) {
        // Only logging it if we didn't shutdown explicitly
        if(!is_reader_thread_stop) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "tunnel_read(%p) %d:%s",
                ioh, ts_err, error_msg.c_str());
        }
        goto done;
    }

done:
    if (ts_err != TS_OK) {
        VPLMutex_Lock(&mutex);
        tunnel_failed = ts_err;
        VPLMutex_Unlock(&mutex);
    }

    if ( (ts_err != TS_OK) && (msg != NULL) ) {
        if ( msg->msg != NULL ) {
            // exit(1);
            delete [] msg->msg;
        }
        delete msg;
        msg = NULL;
    }
    return ts_err;
}

VSSI_Result vssts_tunnel::send_msg(vssts_msg_t*& msg)
{
    VSSI_Result rv = VSSI_SUCCESS;
    int rc;

    // Note that this function is called with object lock held, so it's important
    // for this function not to call any vssts_pool function to avoid deadlock.
    // Ideally, this function should be as simple as possible

    // Queue up message for sending
    VPLMutex_Lock(&mutex);
    if(!is_writer_thread_stop) {
        do {
            xid_next++;
            if ( xid_next == 0 ) {
                xid_next = 1;
            }
        } while ( (send_queue.find(xid_next) != send_queue.end()) ||
                  (msg_map.find(xid_next) != msg_map.end()) );

        msg->xid = xid_next;
        send_queue[msg->xid] = msg;

        rc = VPLCond_Signal(&cond);
        if (rc != VPL_OK) {
            send_queue.erase(msg->xid);

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rc);
            rv = VSSI_NOMEM;
        } else {
            msg = NULL;
        }
    }
    // Don't serve this request, this is closed or been closing.
    else {
        rv = VSSI_COMM;
    }
    VPLMutex_Unlock(&mutex);

    return rv;
}

void vssts_tunnel::handle_stat2_reply(VSSI_Dirent2** stats, vssts_msg_t* msg)
{
    char* entryData = vss_read_dir_reply_get_data(msg->msg_data);

    *stats = (VSSI_Dirent2*)malloc(sizeof(VSSI_Dirent2) +
                                  vss_dirent2_get_name_len(entryData));
    if(*stats == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        goto done;
    }
    (*stats)->name = (const char*)(*stats + 1);
    (*stats)->signature = NULL;
    (*stats)->metadata = NULL;

    (*stats)->size = vss_dirent2_get_size(entryData);
    (*stats)->ctime = vss_dirent2_get_ctime(entryData);
    (*stats)->mtime = vss_dirent2_get_mtime(entryData);
    (*stats)->changeVer = vss_dirent2_get_change_ver(entryData);
    (*stats)->isDir = vss_dirent2_get_is_dir(entryData);
    (*stats)->attrs = vss_dirent2_get_attrs(entryData);
    memcpy((char*)((*stats)->name), vss_dirent2_get_name(entryData),
           vss_dirent2_get_name_len(entryData));

done:
    return;
}

void vssts_object::clear_notify(void)
{
    VPLMutex_Lock(&mutex);
    notify_callback = NULL;
    notify_ctx = NULL;
    VPLMutex_Unlock(&mutex);
}

void vssts_object::release_tunnel_if_match(u32 tunnel_id, bool force_release)
{
    vssts_tunnel *tmp = NULL;

    VPLMutex_Lock(&mutex);
    if (tunnel != NULL) {
        if (force_release || (tunnel->get_tunnel_id() == tunnel_id)) {
            tmp = tunnel;
            tunnel = NULL;
            do_callback(VSSI_NOTIFY_DISCONNECTED_EVENT, NULL, 0);
        }
    }
    VPLMutex_Unlock(&mutex);

    if (tmp != NULL) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Releasing tunnel "FMTu32, tmp->get_tunnel_id());
        pool->tunnel_put(tmp);
    }
}

void vssts_object::repair_tunnel_if_needed(void)
{
    vssts_tunnel* new_tunnel = NULL;

    VPLMutex_Lock(&mutex);
    if (tunnel != NULL) {
        VPLMutex_Unlock(&mutex);
        goto done;
    }
    VPLMutex_Unlock(&mutex);

    new_tunnel = pool->tunnel_get(0, user_id, device_id);
    if ( new_tunnel == NULL ) {
        // Can't get a tunnel now, repair failed
        goto done;
    }

    // Because we have unlocked, need to check to make sure
    // repair is still needed
    VPLMutex_Lock(&mutex);
    if (tunnel != NULL) {
        VPLMutex_Unlock(&mutex);
        pool->tunnel_put(new_tunnel);
        goto done;
    }
    tunnel = new_tunnel;
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Repaired tunnel "FMTu32, tunnel->get_tunnel_id());
    VPLMutex_Unlock(&mutex);

done:
    return;
}

void vssts_object::do_callback(u64 mask, const char* buf, u32 length)
{
    VPLMutex_Lock(&mutex);
    if ( notify_callback != NULL ) {
        (notify_callback)(notify_ctx,  mask, buf, length);
    }
    VPLMutex_Unlock(&mutex);
}

void vssts_tunnel::handle_notification(vssts_msg_t* msg)
{
    vssts_object* object = NULL;

    object =
        pool->object_get_by_handle(vss_notification_get_handle(msg->msg_data));
    if ( object == NULL ) {
        goto done;
    }
    object->do_callback(vss_notification_get_mask(msg->msg_data),
                        vss_notification_get_data(msg->msg_data),
                        vss_notification_get_length(msg->msg_data));
    pool->object_put(object);
done:
    return;
}

void vssts_tunnel::clear_msg_on_fail(bool clear_send_queue)
{
    map<u32,vssts_msg_t*>* clear_map;

    VPLMutex_Lock(&mutex);
    if (clear_send_queue) {
        clear_map = &send_queue;
    } else {
        clear_map = &msg_map;
    }

    for (map<u32,vssts_msg_t*>::iterator it = clear_map->begin();
         it != clear_map->end();
         ) {
        vssts_msg_t* orig = NULL;
        vssts_object* object = NULL;
        vssts_file_t* file = NULL;

        orig = it->second;
        clear_map->erase(it++);

        VPLMutex_Unlock(&mutex);

        if ( orig->object_id ) {
            object = pool->object_get(orig->object_id);
            if ( object == NULL ) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Unknown object %d",
                    orig->object_id);
            } else {
                if ( orig->command == VSS_SET_NOTIFY ) {
                    object->clear_notify();
                } else if ( orig->command == VSS_OPEN ) {
                    vssts_object* tmp = object;
                    pool->object_put(tmp);
                } else if ( orig->command == VSS_CLOSE ) {
                    // The close command de-allocates the object
                    // even if it fails to reach the storage node.
                    // The same is NOT true for the closefile command,
                    // which only de-allocates the file object if it
                    // reaches the storage node
                    vssts_object* tmp = object;
                    pool->object_put(tmp);
                }
            }
        }

        if ( orig->file_id ) {
            file = pool->file_get(orig->file_id);
            if ( file == NULL ) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Unknown file "FMTu32,
                    orig->file_id);
            } else {
                if ( orig->command == VSS_OPEN_FILE ) {
                    vssts_file_t* tmp = file;
                    pool->file_put(tmp);
                }
            }
        }

        if ( orig->callback ) {
            (orig->callback)(orig->ctx, VSSI_COMM);
        }

        delete [] orig->msg;
        delete orig;

        if ( object != NULL ) {
            pool->object_put(object);
        }

        if ( file != NULL) {
            pool->file_put(file);
        }

        VPLMutex_Lock(&mutex);
    }
    VPLMutex_Unlock(&mutex);
}

void vssts_tunnel::handle_reply(vssts_msg_t*& msg)
{
    vssts_msg_t* orig = NULL;
    vssts_object* object = NULL;
    vssts_file_t* file = NULL;
    VSSI_Result rv;

    // This is an async message and handled differently from normal replies
    if ( msg->command == VSS_NOTIFICATION ) {
        handle_notification(msg);
        goto unknown_reply;
    }

    VPLMutex_Lock(&mutex);
    {
        map<u32,vssts_msg_t*>::iterator it;
        it = msg_map.find(msg->xid);
        if ( it != msg_map.end() ) {
            orig = it->second;
            msg_map.erase(it);
        }
    }
    VPLMutex_Unlock(&mutex);

    // This is an unknown reply
    if ( orig == NULL ) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Unexpected XID %d", msg->xid);
        goto unknown_reply;
    }

    if ( orig->object_id ) {
        object = pool->object_get(orig->object_id);
        if ( object == NULL ) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Unknown object %d",
                orig->object_id);
            rv = VSSI_INVALID;
            goto failed;
        }
    }

    if ( orig->file_id ) {
        file = pool->file_get(orig->file_id);
        if ( file == NULL ) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Unknown file "FMTu32,
                orig->file_id);
            rv = VSSI_INVALID;
            goto failed;
        }
    }

    rv = vss_get_status(msg->msg);

    // XXX - This is that really bad open_file special case. This causes
    // a serious amount of pain.
    if ( (rv != VSSI_SUCCESS) && 
         !((rv == VSSI_EXISTS) && (orig->command == VSS_OPEN_FILE)) &&
         // For close and closefile commands, we still want to close it
         // in error cases
         orig->command != VSS_CLOSE &&
         orig->command != VSS_CLOSE_FILE ) {
        if ( orig->command == VSS_SET_NOTIFY ) {
            // remove the installed handler if this failed.
            object->clear_notify();
        } else if ( orig->command == VSS_OPEN ) {
            // If open failed, release the object
            vssts_object* tmp = object;
            pool->object_put(tmp);
        } else if ( orig->command == VSS_OPEN_FILE ) {
            // If open file failed, release the file handle
            vssts_file_t* tmp = file;
            pool->file_put(tmp);
        }

        goto failed;
    }

    // If the message is the same as the orig then we failed
    // to send the request altogether
    switch (orig->command) {
    case VSS_DELETE:
        // Release this object so it can be reclaimed.
        {
            vssts_object* tmp = object;
            pool->object_put(tmp);
        }
        break;

    case VSS_OPEN:
        // return our handle to the user
        *(orig->ret_handle) = (VSSI_Object)orig->object_id;

        // Pull out the reply's handle and version number
        object->set_handle(vss_open_reply_get_handle(msg->msg_data));
        pool->object_add_handle(object);
        object->set_version(vss_open_reply_get_objver(msg->msg_data));
        break;

    case VSS_CLOSE:
        // release the initial reference to the object
        // so that the object can get reclaimed automatically.
        {
            vssts_object* tmp = object;
            pool->object_put(tmp);
        }

        // XXX - we could also mark the object as closed but
        // in reality, everything should fail out correctly, albeit a bit
        // more awkwardly, without it.
        
        break;

    case VSS_OPEN_FILE:
        // return our handle to the user
        *(orig->ret_file) = (VSSI_File)orig->file_id;

        // Pull out the reply's handle and version number
        file->server_handle = vss_open_file_reply_get_handle(msg->msg_data);
        object->set_version(vss_open_file_reply_get_objver(msg->msg_data));
        // XXX This appears to be a bug in the protocol
        // file->flags = vss_open_file_reply_get_flags(msg->msg_data);
        break;

    case VSS_CLOSE_FILE:
        object->set_version(vss_version_reply_get_objver(msg->msg_data));
        {
            // Release the initial reference so it can go away
            vssts_file_t* tmp = file;
            pool->file_put(tmp);
        }
        break;

    case VSS_READ_FILE:
        // pull out the length and the data
        *(orig->io_length) = vss_read_file_reply_get_length(msg->msg_data);
        memcpy(orig->read_buf, vss_read_file_reply_get_data(msg->msg_data),
               *(orig->io_length));
        break;

    case VSS_WRITE_FILE:
        // pull out the length written
        *(orig->io_length) = vss_write_file_reply_get_length(msg->msg_data);
        break;

    case VSS_ERASE:
    case VSS_MAKE_DIR2:
    case VSS_REMOVE:
    case VSS_RENAME2:
    case VSS_CHMOD:
    case VSS_SET_TIMES:
        object->set_version(vss_version_reply_get_objver(msg->msg_data));
        break;

    case VSS_GET_NOTIFY:
        *(orig->ret_mask_out) = vss_get_notify_reply_get_mode(msg->msg_data);
        break;

    case VSS_SET_NOTIFY:
        *(orig->ret_mask_out) = vss_get_notify_reply_get_mode(msg->msg_data);
        break;

    case VSS_GET_LOCK:
        *(orig->ret_lock_state) = vss_get_lock_reply_get_mode(msg->msg_data);
        break;

    case VSS_STAT2:
        handle_stat2_reply(orig->ret_dirent, msg);
        object->set_version(vss_read_dir_reply_get_objver(msg->msg_data));
        break;

    case VSS_GET_SPACE:
        *(orig->ret_disk_size) =
            vss_get_space_reply_get_disk_size(msg->msg_data);
        *(orig->ret_dataset_size) = 
            vss_get_space_reply_get_dataset_size(msg->msg_data);
        *(orig->ret_avail_size) =
            vss_get_space_reply_get_avail_size(msg->msg_data);
        break;

    case VSS_READ_DIR2: {
        VSSI_DirectoryState* dir = new (std::nothrow) VSSI_DirectoryState;
        if (dir == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
            rv = VSSI_NOMEM;
            break;
        }

        dir->data_len = vss_read_dir_reply_get_length(msg->msg_data);
        dir->raw_data = new char[dir->data_len];
        memcpy(dir->raw_data, vss_read_dir_reply_get_data(msg->msg_data),
               dir->data_len);
        dir->offset = 0;
        *(orig->ret_dir) = (VSSI_Dir2)dir;
        object->set_version(vss_read_dir_reply_get_objver(msg->msg_data));
        break;
    }

    case VSS_TRUNCATE_FILE:
    case VSS_CHMOD_FILE:
    case VSS_RELEASE_FILE:
    case VSS_SET_LOCK:
    case VSS_SET_LOCK_RANGE:
        break;

    default:
        break;
    }

failed:
    if ( orig->callback ) {
        (orig->callback)(orig->ctx, rv);
    }

    // Delete the original msg
    delete [] orig->msg;
    delete orig;

unknown_reply:
    // Now delete the reply, if it's a real reply
    if ( msg != orig ) {
        delete [] msg->msg;
        delete msg;
    }
    if ( object != NULL ) {
        pool->object_put(object);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
}

void vssts_tunnel::reader(void)
{
    int rv;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Starting. Dev "FMTu64, device_id);

    // Open the tunnel
    {
        TSOpenParms_t parms;
        TSError_t err;
        string error_msg;

        parms.user_id = user_id;
        parms.device_id = device_id;
        parms.service_name = "VSSI";
        parms.credentials.erase();
        parms.flags = 0;
        parms.timeout = VPLTIME_INVALID;

        err = TS_EXT::TS_Open(parms, ioh, error_msg);
        if ( err != TS_OK ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TS_Open() %d:%s",
                             err, error_msg.c_str());

            VPLMutex_Lock(&mutex);
            tunnel_failed = err;
            is_open_done = true;
            rv = VPLCond_Signal(&open_cond);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
            }
            VPLMutex_Unlock(&mutex);

            // Releasing the tunnel will cause all pending messages
            // to be replied with VSSI_COMM during shutdown()
            pool->tunnel_release(tunnel_id);
            goto skip_read;
        }
    }

    VPLMutex_Lock(&mutex);
    is_open_done = true;

    rv = VPLCond_Signal(&open_cond);
    if (rv != VPL_OK) {
        tunnel_failed = TS_ERR_INTERNAL;
        VPLMutex_Unlock(&mutex);

        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
        pool->tunnel_release(tunnel_id);
        goto skip_read;
    }
    VPLMutex_Unlock(&mutex);

    while (!is_reader_thread_stop) {
        TSError_t err;
        string error_msg;
        vssts_msg_t* msg;

        err = recv_msg(msg, error_msg);
        if ( err != TS_OK) {
            if (!is_reader_thread_stop) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "recv_msg() %d:%s",
                                 err, error_msg.c_str());

                // Releasing the tunnel will cause all pending messages
                // to be replied with VSSI_COMM during shutdown()
                pool->tunnel_release(tunnel_id);
            }
            break;
        } else {
            handle_reply(msg);
        }
    }

skip_read:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Ending. Dev "FMTu64, device_id);
}

void vssts_tunnel::writer(void)
{
    TSError_t err = TS_OK;
    int rv;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Starting. Dev "FMTu64, device_id);

    // Wait for the reader to open the tunnel
    VPLMutex_Lock(&mutex);
    while (!is_open_done) {
        rv = VPLCond_TimedWait(&open_cond, &mutex, VPLTIME_FROM_SEC(VSSTS_OPEN_WAIT_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            tunnel_failed = TS_ERR_COMM;
            VPLMutex_Unlock(&mutex);

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TS_Open() timed wait failed, rv=%d", rv);
            pool->tunnel_release(tunnel_id);
            goto skip_write;
        }
    }

    while (!is_writer_thread_stop) {
        if (send_queue.empty() && (tunnel_failed == TS_OK)) {
            rv = VPLCond_TimedWait(&cond, &mutex, VPL_TIMEOUT_NONE);
            if (rv != VPL_OK) {
                tunnel_failed = TS_ERR_COMM;
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "writer() timed wait failed, rv=%d", rv);
                pool->tunnel_release(tunnel_id);
                goto skip_write;
            }
        }

        if (tunnel_failed != TS_OK) {
            VPLMutex_Unlock(&mutex);

            // I don't think this check for is_writer_thread_stop is absolutely necessary,
            // but it reduces unnecessary work
            if (!is_writer_thread_stop) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Detected tunnel failure %d", tunnel_failed);

                // Releasing the tunnel will cause all pending messages
                // to be replied with VSSI_COMM during shutdown()
                pool->tunnel_release(tunnel_id);
            }
            goto skip_write;
        }

        for (map<u32,vssts_msg_t*>::iterator it = send_queue.begin();
             it != send_queue.end();
             ) {
            vssts_msg_t* msg = NULL;
            string error_msg;

            msg = it->second;
            send_queue.erase(it++);
            msg_map[msg->xid] = msg;
            VPLMutex_Unlock(&mutex);

            // If the write failed, the tunnel will be released, causing
            // the read to fail too, which will then trigger the msg_map
            // to be cleared.

            vss_set_xid(msg->msg, msg->xid);
            if (tunnel_failed != TS_OK) {
                if(!is_writer_thread_stop) {
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Detected tunnel failure %d", tunnel_failed);

                    // Releasing the tunnel will cause all pending messages
                    // to be replied with VSSI_COMM during shutdown()
                    pool->tunnel_release(tunnel_id);
                }
                goto skip_write;
            } else {
                err = TS_EXT::TS_Write(ioh, msg->msg, msg->msg_len, error_msg);
                if (err != TS_OK && !is_writer_thread_stop) {
                    VPLMutex_Lock(&mutex);
                    tunnel_failed = err;
                    VPLMutex_Unlock(&mutex);

                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TS_Write("FMTu64") %d:%s",
                                     device_id, err, error_msg.c_str());

                    // Releasing the tunnel will cause all pending messages
                    // to be replied with VSSI_COMM during shutdown()
                    pool->tunnel_release(tunnel_id);
                    goto skip_write;
                }
            }
            VPLMutex_Lock(&mutex);
        }
    }
    VPLMutex_Unlock(&mutex);

skip_write:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Ending. Dev "FMTu64, device_id);
}

void vssts_tunnel::set_orphan(void)
{
    VPLMutex_Lock(&mutex);
    is_orphan_tunnel = true;
    VPLMutex_Unlock(&mutex);
    pool->tunnel_reaper_signal();
}

void vssts_tunnel::shutdown(void)
{
    int rv;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutting down tunnel "FMTu32, tunnel_id);

    VPLMutex_Lock(&mutex);
    if(!is_orphan_tunnel) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "This is not a orphan tunnel(%p)", this);
    }
    is_reader_thread_stop = true;
    is_writer_thread_stop = true;
    rv = VPLCond_Signal(&cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }
    VPLMutex_Unlock(&mutex);

    if ( ioh != NULL) {
        TSError_t err;
        string error_msg;

        err = TS_EXT::TS_Close(ioh, error_msg);
        if ( err != TS_OK ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TS_Close(%p) %d:%s",
                ioh, err, error_msg.c_str());
            // ignore this error
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Joining reader and writer threads");
    if ( is_reader_thread_started ) {
        rv = VPLThread_Join(&reader_thread, NULL);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join reader thread, rv=%d", rv);
        }
    }
    if ( is_writer_thread_started ) {
        rv = VPLThread_Join(&writer_thread, NULL);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join writer thread, rv=%d", rv);
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Clearing out remaining messages");
    clear_msg_on_fail(false);
    clear_msg_on_fail(true);

    VPLMutex_Lock(&mutex);
    is_shutdown = true;
    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Tunnel "FMTu32" shutdown done", tunnel_id);
}

bool vssts_tunnel::is_orphan(void)
{
    return is_orphan_tunnel;
}

vssts_pool::vssts_pool() :
    object_id_next(0),
    tunnel_id_next(0),
    file_id_next(0),
    tunnel_reaper_stop(false),
    is_shutdown(false)
{
    VPL_SET_UNINITIALIZED(&mutex);
    VPL_SET_UNINITIALIZED(&cond);
}

vssts_pool::~vssts_pool()
{
    if ( VPL_IS_INITIALIZED(&mutex) ) {
        VPLMutex_Destroy(&mutex);
    }

    if ( VPL_IS_INITIALIZED(&cond) ) {
        VPLCond_Destroy(&cond);
    }
}

VSSI_Result vssts_pool::init(void)
{
    VSSI_Result rv = VSSI_NOMEM;

    if ( VPLMutex_Init(&mutex) != VPL_OK ) {
        goto done;
    }

    if ( VPLCond_Init(&cond) != VPL_OK ) {
        goto done;
    }

    // Start up a reaper thread to clean up tunnel 
    {
        int rc;

        VPLThread_attr_t thread_attr;
        VPLThread_AttrInit(&thread_attr);
        VPLThread_AttrSetStackSize(&thread_attr, VSSTS_REAPER_STACK_SIZE);
        VPLThread_AttrSetDetachState(&thread_attr, false);

        tunnel_reaper_stop = false;
        rc = VPLThread_Create(&tunnel_reaper_thr, pool_tunnel_reaper_start,
            VPL_AS_THREAD_FUNC_ARG(this), &thread_attr,
            "TS tunnel reaper");
        if ( rc != VPL_OK ) {
            rv = VSSI_NOMEM;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed start of tunnel reader");
            goto done;
        }
    }

    rv = VSSI_SUCCESS;

done:
    return rv;
}

void vssts_pool::shutdown(void)
{
    // Stop tunnel and threads first, prevent any unexpected incoming packet
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Releasing all tunnels");
    tunnel_release_all();

    // Stop the tunnel reaper, and reap all tunnels
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping the tunnel reaper thread");
    tunnel_reaper_shutdown();

    // Close and release remaining objects
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Releasing all objects");
    object_release_all();

    // Close and release remaining files
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Releasing all files");
    file_release_all();

    VPLMutex_Lock(&mutex);
    is_shutdown = true;
    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Pool shutdown done");
}

VSSI_Result vssts_pool::object_get(u64 user_id,
                                   u64 dataset_id,
                                   vssts_object*& object)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_tunnel* tunnel = NULL;
    vssts_oref_t* oref = NULL;
    u32 object_id;
    u64 device_id;

    object = NULL;

    if ( !lookup_device_id(user_id, dataset_id, device_id) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "dataset "FMTu64" not found",
            dataset_id);
        rv = VSSI_NOTFOUND;
        goto done_no_unlock;
    }

    // tunnel_id of 0 means create a new tunnel since it's an invalid
    // tunnel id
    // 
    // Different objects going to the same device will use a different
    // tunnel object, although the underlying TS implementation may
    // multi-plex them onto the same connection
    // 
    // It's possible to re-use the same tunnel if it goes to the same
    // device, but it seems like an unnecessary optimization at this
    // point
    //
    // Need bigger lock around the entire setup process to avoid tunnels
    // being released before the object gets a reference to it
    VPLMutex_Lock(&mutex);
    tunnel = tunnel_get(0, user_id, device_id);
    if ( tunnel == NULL ) {
        rv = VSSI_COMM;
        goto done;
    }

    // create an object_id for this thing
    do {
        map<u32,vssts_oref_t*>::iterator it;

        object_id_next++;
        if ( object_id_next == 0 ) {
            object_id_next++;
        }
    } while (obj_map.find(object_id_next) != obj_map.end());
    object_id = object_id_next;

    object = new (std::nothrow) vssts_object(object_id,
                                             user_id,
                                             dataset_id,
                                             device_id,
                                             tunnel,
                                             this);
    if ( object == NULL ) {
        tunnel_put(tunnel);

        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rv = VSSI_NOMEM;
        goto done;
    }

    rv = object->init();
    if ( rv != VSSI_SUCCESS ) {
        delete object;
        object = NULL;
        goto done;
    }

    oref = new (std::nothrow) vssts_oref_t;
    if ( oref == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        delete object;  // This will trigger the necessary tunnel_put()
        object = NULL;
        rv = VSSI_NOMEM;
        goto done;
    }

    oref->ref_cnt = 1;
    oref->object = object;

    // This could go inside the start() routine...
    obj_map[object_id] = oref;

done:
    VPLMutex_Unlock(&mutex);

done_no_unlock:
    return rv;
}

vssts_object* vssts_pool::object_get(u32 object_id, bool do_tunnel_repair)
{
    vssts_object* obj = NULL;
    map<u32,vssts_oref_t*>::iterator it;

    VPLMutex_Lock(&mutex);
    it = obj_map.find(object_id);
    if ( it == obj_map.end() ) {
        goto done;
    }
    it->second->ref_cnt++;
    obj = it->second->object;

    // Note that it is necessary to maintain the lock while trying to repair
    // the tunnel to avoid the new tunnel being released before the object
    // gets a reference to it
    if (do_tunnel_repair) {
        obj->repair_tunnel_if_needed();
    }

done:
    VPLMutex_Unlock(&mutex);

    return obj;
}

vssts_object* vssts_pool::object_get_by_handle(u32 object_handle)
{
    vssts_object* obj = NULL;
    map<u32,u32>::iterator it;
    u32 object_id = 0;

    VPLMutex_Lock(&mutex);
    it = obj_handle_map.find(object_handle);
    if ( it == obj_handle_map.end() ) {
        goto done;
    }
    object_id = it->second;

done:
    VPLMutex_Unlock(&mutex);

    if ( object_id != 0 ) {
        obj = object_get(object_id);
    }
    return obj;
}

void vssts_pool::object_put(vssts_object*& object)
{
    map<u32,vssts_oref_t*>::iterator it;
    vssts_oref_t* oref = NULL;

    // This could go inside the start() routine...
    // XXX We may need ref counts on this thing... Might want an ID lookup
    // like we have in TS at some point.
    VPLMutex_Lock(&mutex);
    it = obj_map.find(object->get_object_id());
    if ( it == obj_map.end() ) {
        goto done;
    }
    it->second->ref_cnt--;
    if ( it->second->ref_cnt != 0 ) {
        goto done;
    }
    oref = it->second;
    obj_map.erase(it);

    // remove from the object handle map
    { 
        map<u32,u32>::iterator hit;
        hit = obj_handle_map.find(object->get_handle());
        if ( hit != obj_handle_map.end() ) {
            obj_handle_map.erase(hit);
        }
    }

    // It is necessary to de-allocate the object before releasing
    // the pool mutex to prevent a race condition with the pool
    // shutdown logic.  The pool shutdown logic would attempt to
    // make sure all tunnels are reaped first.  However, if we
    // remove the object from the object map without actually
    // deleting it, then it's possible that this object's tunnel
    // would not get reaped by the pool shutdown logic
    if (oref != NULL) {
        delete oref;
    }
    delete object;

done:
    VPLMutex_Unlock(&mutex);
    object = NULL;
}

void vssts_pool::object_add_handle(vssts_object* obj)
{
    VPLMutex_Lock(&mutex);
    obj_handle_map[obj->get_handle()] = obj->get_object_id();
    VPLMutex_Unlock(&mutex);
}

void vssts_pool::object_release_all(void)
{
    vssts_oref_t* oref = NULL;
    vssts_object* obj = NULL;
    map<u32,vssts_oref_t*>::iterator it;

    VPLMutex_Lock(&mutex);
    for (it = obj_map.begin(); it != obj_map.end(); obj_map.erase(it++)) {
        map<u32,u32>::iterator hit;

        obj = it->second->object;
        oref = it->second;

        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Remaining object(%d) found", obj->get_object_id());
        hit = obj_handle_map.find(obj->get_handle());
        if ( hit != obj_handle_map.end() ) {
            obj_handle_map.erase(hit);
        }

        // Destructor of object will put tunnel.
        delete obj;
        delete oref;
    }
    VPLMutex_Unlock(&mutex);
}

vssts_file_t* vssts_pool::file_get(u64 handle, u32 object_id, u32 flags)
{
    vssts_file_t* file = NULL;

    VPLMutex_Lock(&mutex);

    // ref count is 1 per object. With the last object it can go away.
    {
        vssts_fref_t* fref = NULL;

        file = new (std::nothrow) vssts_file_t;
        if ( file == NULL ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
            goto done;
        }

        file->object_id = object_id;
        file->flags = flags;
        {
            u64 tmp = (u64)file;
            tmp <<= 32;
            file->origin = (u64)handle | tmp;
        }

        fref = new (std::nothrow) vssts_fref_t;
        if ( fref == NULL ) {
            delete file;
            file = NULL;

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
            goto done;
        }

        fref->file = file;
        fref->ref_cnt = 1;

        // generate a file ID. Need something 32-bit so can't use origin.
        do {
            file_id_next++;
            if ( file_id_next == 0 ) {
                file_id_next = 1;
            }
        } while (file_map.find(file_id_next) != file_map.end());
        file->file_id = file_id_next;
        file_map[file->file_id] = fref;
    }

done:
    VPLMutex_Unlock(&mutex);

    return file;
}

vssts_file_t* vssts_pool::file_get(u32 file_id)
{
    map<u32,vssts_fref_t*>::iterator it;
    vssts_file_t* file = NULL;

    VPLMutex_Lock(&mutex);

    it = file_map.find(file_id);
    if ( it == file_map.end() ) {
        goto done;
    }

    it->second->ref_cnt++;
    file = it->second->file;

done:
    VPLMutex_Unlock(&mutex);

    return file;
}

void vssts_pool::file_put(vssts_file_t*& file)
{
    map<u32,vssts_fref_t*>::iterator it;
    bool do_delete = false;
    vssts_fref_t* fref = NULL;

    // This could go inside the start() routine...
    // XXX We may need ref counts on this thing... Might want an ID lookup
    // like we have in TS at some point.
    VPLMutex_Lock(&mutex);
    it = file_map.find(file->file_id);
    if ( it == file_map.end() ) {
        goto done;
    }
    it->second->ref_cnt--;
    if ( it->second->ref_cnt != 0 ) {
        goto done;
    }
    fref = it->second;
    file_map.erase(it);
    do_delete = true;
done:
    VPLMutex_Unlock(&mutex);
    if ( do_delete ) {
        if ( fref != NULL ) {
            delete fref;
        }
        delete file;
    }
    file = NULL;
}

void vssts_pool::file_release_by_object_id(u32 object_id)
{
    vssts_file_t* file = NULL;
    map<u32,vssts_fref_t*>::iterator it;
    vssts_fref_t* fref;

    VPLMutex_Lock(&mutex);
    for (it = file_map.begin(); it != file_map.end();) {
        file = it->second->file;
        if(file->object_id == object_id) {
            file = it->second->file;
            fref = it->second;
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Orphan file(%d) found", file->file_id);
            delete fref;
            delete file;
            file_map.erase(it++);
        } else {
            it++;
        }
    }
    VPLMutex_Unlock(&mutex);
}

void vssts_pool::file_release_all(void)
{
    vssts_file_t* file = NULL;
    map<u32,vssts_fref_t*>::iterator it;
    vssts_fref_t* fref;

    VPLMutex_Lock(&mutex);
    for (it = file_map.begin(); it != file_map.end(); file_map.erase(it++)) {
        file = it->second->file;
        fref = it->second;
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Remaining file(%d) found", file->file_id);
        delete fref;
        delete file;
    }
    VPLMutex_Unlock(&mutex);
}

vssts_tunnel* vssts_pool::tunnel_get(u32 tunnel_id, u64 user_id, u64 device_id)
{
    vssts_tunnel* tunnel = NULL;
    u32 new_tunnel_id;

    VPLMutex_Lock(&mutex);

    // If we are getting destroyed
    if(tunnel_reaper_stop) {
        goto done;
    }

    // see if we can use an existing tunnel
    {
        map<u32,vssts_tref_t*>::iterator it;

        it = tunnel_map.find(tunnel_id);
        if ( it != tunnel_map.end() ) {
            tunnel = it->second->tunnel;
            it->second->ref_cnt++;
            goto done;
        }
    }

    // create a tunnel_id for this thing
    do {
        map<u32,vssts_tref_t*>::iterator it;

        tunnel_id_next++;
        if ( tunnel_id_next == 0 ) {
            tunnel_id_next++;
        }
    } while (tunnel_map.find(tunnel_id_next) != tunnel_map.end());
    new_tunnel_id = tunnel_id_next;

    // ref count is 1 per object. With the last object it can go away.
    {
        vssts_tref_t* tref;
        VSSI_Result rv;

        tunnel = new (std::nothrow) vssts_tunnel(new_tunnel_id, user_id, device_id, this);
        if ( tunnel == NULL ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
            goto done;
        }

        rv = tunnel->init();
        if ( rv != VSSI_SUCCESS ) {
            delete tunnel;
            tunnel = NULL;
            goto done;
        }

        tref = new (std::nothrow) vssts_tref_t;
        if ( tref == NULL ) {
            tunnel->shutdown();
            delete tunnel;
            tunnel = NULL;

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
            goto done;
        }

        tunnel_reaper_add(tunnel);

        tref->tunnel = tunnel;
        tref->ref_cnt = 1;

        tunnel_map[new_tunnel_id] = tref;
    }

done:
    VPLMutex_Unlock(&mutex);

    return tunnel;
}

void vssts_pool::tunnel_put(vssts_tunnel*& tunnel)
{
    map<u32,vssts_tref_t*>::iterator it;
    vssts_tref_t* tref = NULL;

    VPLMutex_Lock(&mutex);
    it = tunnel_map.find(tunnel->get_tunnel_id());
    if ( it == tunnel_map.end() ) {
        goto done;
    }
    tref = it->second;

    tref->ref_cnt--;
    if ( tref->ref_cnt != 0 ) {
        goto done;
    }

    tunnel_map.erase(it);

    // We don't perform vssts_tunnel clean up here, just mark it as ready to be shutdown.
    // instead, we use tunnel reaper in vssts_pool to do this.
    tref->tunnel->set_orphan();

    delete tref;
done:
    VPLMutex_Unlock(&mutex);
    tunnel = NULL;
}

void vssts_pool::tunnel_release(u32 tunnel_id)
{
    vssts_object* obj = NULL;
    map<u32,vssts_oref_t*>::iterator it;

    // If the tunnel is broken, then it's necessary to release the tunnel's
    // reference from the vssts_object, so that the tunnel can die, making
    // way for a new tunnel

    // Scan through each object with reference to this tunnel, and call the 
    // object's tunnel release function
    VPLMutex_Lock(&mutex);
    for (it = obj_map.begin(); it != obj_map.end(); it++) {
        obj = it->second->object;
        obj->release_tunnel_if_match(tunnel_id);
    }
    VPLMutex_Unlock(&mutex);
}

void vssts_pool::tunnel_release_all(void)
{
    vssts_object* obj = NULL;
    map<u32,vssts_oref_t*>::iterator it;

    VPLMutex_Lock(&mutex);
    for (it = obj_map.begin(); it != obj_map.end(); it++) {
        obj = it->second->object;
        obj->release_tunnel_if_match(0, true);
    }
    VPLMutex_Unlock(&mutex);
}

void vssts_pool::tunnel_reaper_start(void)
{
    int rv;

    VPLMutex_Lock(&mutex);
    while (!tunnel_reaper_stop) {
        rv = VPLCond_TimedWait(&cond, &mutex, VPL_TIMEOUT_NONE);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Reaper failed timed wait, rv=%d\n", rv);
        }

        for(list<vssts_tunnel*>::iterator it = tunnel_reap_list.begin();
            it != tunnel_reap_list.end();
            ) {
            vssts_tunnel* tunnel = *it;
            if(tunnel->is_orphan()) {
                tunnel_reap_list.erase(it);
                VPLMutex_Unlock(&mutex);
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Delete tunnel "FMTu32, tunnel->get_tunnel_id());
                tunnel->shutdown();
                delete tunnel;
                VPLMutex_Lock(&mutex);
                /// Since we released the lock, state of tunnels might be changed, we start over from beginning and check again.
                /// A infinite loop wouldn't happen because it always reduces the list size by 1 in this scope,
                /// and assign the iterator to begin, if it keeps hitting into this scope, the size is going to be zero (begin = end).
                it = tunnel_reap_list.begin();
            } else {
                it++;
            }
        }
    }
    VPLMutex_Unlock(&mutex);
}

void vssts_pool::tunnel_reaper_shutdown(void)
{
    int rv;

    VPLMutex_Lock(&mutex);
    tunnel_reaper_stop = true;
    rv = VPLCond_Signal(&cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }
    VPLMutex_Unlock(&mutex);

    rv = VPLThread_Join(&tunnel_reaper_thr, NULL);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join reaper thread, rv=%d", rv);
    }
}

void vssts_pool::tunnel_reaper_add(vssts_tunnel* tunnel)
{
    int rv;

    VPLMutex_Lock(&mutex);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Push tunnel "FMTu32" back to cleanup queue.", tunnel->get_tunnel_id());
    tunnel_reap_list.push_back(tunnel);
    rv = VPLCond_Signal(&cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }
    VPLMutex_Unlock(&mutex);
}

void vssts_pool::tunnel_reaper_signal(void)
{
    int rv;

    VPLMutex_Lock(&mutex);
    rv = VPLCond_Signal(&cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }
    VPLMutex_Unlock(&mutex);
}

#define USE_VSSI_VERSION        2

void vssts_object::init_hdr(char* hdr,
                            u8 command,
                            u32 data_length)
{
    memset(hdr, 0, VSS_HEADER_SIZE);
    vss_set_version(hdr, USE_VSSI_VERSION);
    vss_set_command(hdr, command);
    vss_set_device_id(hdr, device_id);
    vss_set_data_length(hdr, data_length);
}

void vssts_object::delete2( void* ctx, VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_DELETE_BASE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_DELETE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_delete_set_reserved(msg->msg_data);
    vss_delete_set_uid(msg->msg_data, user_id);
    vss_delete_set_did(msg->msg_data, dataset_id);

    // xid assigned by send code.

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

VSSI_Result vssts_object::open(u8 mode,
                               VSSI_Object* ret_handle,
                               void* ctx,
                               VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_OPEN_BASE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_OPEN, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_open_set_reserved(msg->msg_data);
    vss_open_set_mode(msg->msg_data, mode);
    vss_open_set_uid(msg->msg_data, user_id);
    vss_open_set_did(msg->msg_data, dataset_id);

    // xid assigned by send code.
    msg->ret_handle = ret_handle;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
    }

    // This is a special case where the caller of this function actually checks its
    // return value and do the callback in the case of a failure.  It's needed
    // because the caller needs to do certain error recovery if this function fails

    return rc;
}

VSSI_Result vssts_object::close(void* ctx, VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_CLOSE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_CLOSE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_close_set_handle(msg->msg_data, handle);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
    }

    // This is a special case where the caller of this function actually checks its
    // return value and do the callback in the case of a failure.  It's needed
    // because the caller needs to do certain error recovery if this function fails

    return rc;
}

void vssts_object::mkdir2(const char* name,
                          u32 attrs,
                          void* ctx,
                          VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    int msg_len = VSS_HEADER_SIZE + VSS_MAKE_DIR2_BASE_SIZE + name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_MAKE_DIR2, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_make_dir2_set_handle(msg->msg_data, handle);
    vss_make_dir2_set_objver(msg->msg_data, version);
    vss_make_dir2_set_attrs(msg->msg_data, attrs);
    vss_make_dir2_set_name_length(msg->msg_data, name_len);
    vss_make_dir2_set_name(msg->msg_data, name);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::dir_read(const char* name,
                            VSSI_Dir2* dir,
                            void* ctx,
                            VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    int msg_len = VSS_HEADER_SIZE + VSS_READ_DIR_BASE_SIZE + name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_READ_DIR2, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg->ret_dir = dir;

    // now the command specific hdr
    vss_read_dir_set_handle(msg->msg_data, handle);
    vss_read_dir_set_objver(msg->msg_data, version);
    vss_read_dir_set_name_length(msg->msg_data, name_len);
    vss_read_dir_set_name(msg->msg_data, name);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::stat2(const char* name,
                         VSSI_Dirent2** stats,
                         void* ctx,
                         VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    int msg_len = VSS_HEADER_SIZE + VSS_STAT_BASE_SIZE + name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_STAT2, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg->ret_dirent = stats;

    // now the command specific hdr
    vss_stat_set_handle(msg->msg_data, handle);
    vss_stat_set_objver(msg->msg_data, version);
    vss_stat_set_name_length(msg->msg_data, name_len);
    vss_stat_set_name(msg->msg_data, name);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::chmod(const char* name,
                         u32 attrs,
                         u32 attrs_mask,
                         void* ctx,
                         VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    int msg_len = VSS_HEADER_SIZE + VSS_CHMOD_BASE_SIZE + name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_CHMOD, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_chmod_set_handle(msg->msg_data, handle);
    vss_chmod_set_objver(msg->msg_data, version);
    vss_chmod_set_attrs(msg->msg_data, attrs);
    vss_chmod_set_attrs_mask(msg->msg_data, attrs_mask);
    vss_chmod_set_name_length(msg->msg_data, name_len);
    vss_chmod_set_name(msg->msg_data, name);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::remove(const char* name, void* ctx, VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    int msg_len = VSS_HEADER_SIZE + VSS_REMOVE_BASE_SIZE + name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_REMOVE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_remove_set_handle(msg->msg_data, handle);
    vss_remove_set_objver(msg->msg_data, version);
    vss_remove_set_name_length(msg->msg_data, name_len);
    vss_remove_set_name(msg->msg_data, name);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::rename2(const char* name,
                           const char* new_name,
                           u32 flags,
                           void* ctx,
                           VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    int new_name_len = strlen(new_name);
    int msg_len = VSS_HEADER_SIZE + VSS_RENAME2_BASE_SIZE + name_len +
                  new_name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_RENAME2, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_rename2_set_handle(msg->msg_data, handle);
    vss_rename2_set_flags(msg->msg_data, flags);
    vss_rename2_set_objver(msg->msg_data, version);
    vss_rename2_set_name_length(msg->msg_data, name_len);
    vss_rename2_set_new_name_length(msg->msg_data, new_name_len);
    vss_rename2_set_name(msg->msg_data, name);
    vss_rename2_set_new_name(msg->msg_data, new_name);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::get_notify_events(VSSI_NotifyMask* mask_out,
                                     void* ctx,
                                     VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_GET_NOTIFY_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_GET_NOTIFY, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_get_notify_set_handle(msg->msg_data, handle);

    msg->ret_mask_out = mask_out;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::set_notify_events(VSSI_NotifyMask* mask_in_out,
                                     void* new_notify_ctx,
                                     VSSI_NotifyCallback new_notify_callback,
                                     void* ctx,
                                     VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_SET_NOTIFY_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_SET_NOTIFY, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_set_notify_set_handle(msg->msg_data, handle);
    vss_set_notify_set_mode(msg->msg_data, *mask_in_out);

    msg->ret_mask_out = mask_in_out;

    VPLMutex_Lock(&mutex);
    notify_callback = new_notify_callback;
    notify_ctx = new_notify_ctx;

    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }

        VPLMutex_Lock(&mutex);
        notify_callback = NULL;
        notify_ctx = NULL;
        VPLMutex_Unlock(&mutex);

        callback(ctx, rc);
    }
}

void vssts_object::set_times(const char* name,
                             VPLTime_t ctime,
                             VPLTime_t mtime,
                             void* ctx,
                             VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    int msg_len = VSS_HEADER_SIZE + VSS_SET_TIMES_BASE_SIZE + name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_SET_TIMES, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_set_times_set_handle(msg->msg_data, handle);
    vss_set_times_set_objver(msg->msg_data, version);
    vss_set_times_set_ctime(msg->msg_data, ctime);
    vss_set_times_set_mtime(msg->msg_data, mtime);
    vss_set_times_set_name_length(msg->msg_data, name_len);
    vss_set_times_set_name(msg->msg_data, name);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::get_space(u64* disk_size,
                             u64* dataset_size,
                             u64* avail_size,
                             void* ctx,
                             VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_GET_SPACE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_GET_SPACE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg->ret_disk_size = disk_size;
    msg->ret_dataset_size = dataset_size;
    msg->ret_avail_size = avail_size;

    // now the command specific hdr
    vss_get_space_set_handle(msg->msg_data, handle);

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_open(const char* name,
                             u32 flags,
                             u32 attrs,
                             VSSI_File* ret_file,
                             void* ctx,
                             VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int name_len = strlen(name);
    vssts_file_t* file = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_OPEN_FILE_BASE_SIZE + name_len;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    file = pool->file_get(handle, object_id, flags);
    if (file == NULL) {
        rc = VSSI_NOMEM;
        goto done;
    }

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_OPEN_FILE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_open_file_set_handle(msg->msg_data, handle);
    vss_open_file_set_objver(msg->msg_data, version);
    vss_open_file_set_origin(msg->msg_data, file->origin);
    vss_open_file_set_flags(msg->msg_data, flags);
    vss_open_file_set_attrs(msg->msg_data, attrs);
    vss_open_file_set_name_length(msg->msg_data, name_len);
    vss_open_file_set_name(msg->msg_data, name);

    msg->file_id = file->file_id;
    msg->ret_file = ret_file;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }

    if (rc != VSSI_SUCCESS && file != NULL) {
        pool->file_put(file);
    }
}

VSSI_Result vssts_object::file_close(vssts_file_t* file,
                                     void* ctx,
                                     VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_CLOSE_FILE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_CLOSE_FILE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_close_file_set_origin(msg->msg_data, file->origin);
    vss_close_file_set_handle(msg->msg_data, file->server_handle);

    msg->file_id = file->file_id;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
    }

    // This is a special case where the caller of this function actually checks its
    // return value and do the callback in the case of a failure.  It's needed
    // because the caller needs to do certain error recovery if this function fails
    return rc;
}

void vssts_object::file_read(vssts_file_t* file,
                             u64 offset,
                             u32* length,
                             char* buf,
                             void* ctx,
                             VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_READ_FILE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_READ_FILE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_read_file_set_origin(msg->msg_data, file->origin);
    vss_read_file_set_offset(msg->msg_data, offset);
    vss_read_file_set_length(msg->msg_data, *length);
    vss_read_file_set_handle(msg->msg_data, file->server_handle);

    msg->file_id = file->file_id;
    msg->read_buf = buf;
    msg->io_length = length;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_write(vssts_file_t* file,
                              u64 offset,
                              u32* length,
                              const char* buf,
                              void* ctx,
                              VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_WRITE_FILE_BASE_SIZE + *length;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_WRITE_FILE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_write_file_set_origin(msg->msg_data, file->origin);
    vss_write_file_set_offset(msg->msg_data, offset);
    vss_write_file_set_length(msg->msg_data, *length);
    vss_write_file_set_handle(msg->msg_data, file->server_handle);
    memcpy(vss_write_file_get_data(msg->msg_data), buf, *length);

    msg->file_id = file->file_id;
    msg->io_length = length;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_truncate(vssts_file_t* file,
                                 u64 offset,
                                 void* ctx,
                                 VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_TRUNCATE_FILE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_TRUNCATE_FILE, hdr, msg_len, ctx, callback);
    if (msg  == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_truncate_file_set_origin(msg->msg_data, file->origin);
    vss_truncate_file_set_offset(msg->msg_data, offset);
    vss_truncate_file_set_handle(msg->msg_data, file->server_handle);

    msg->file_id = file->file_id;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_chmod(vssts_file_t* file,
                              u32 attrs,
                              u32 attrs_mask,
                              void* ctx,
                              VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_CHMOD_FILE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_CHMOD_FILE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_chmod_file_set_origin(msg->msg_data, file->origin);
    vss_chmod_file_set_attrs(msg->msg_data, attrs);
    vss_chmod_file_set_attrs_mask(msg->msg_data, attrs_mask);
    vss_chmod_file_set_handle(msg->msg_data, file->server_handle);

    msg->file_id = file->file_id;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_release(vssts_file_t* file,
                              void* ctx,
                              VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_RELEASE_FILE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_RELEASE_FILE, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_release_file_set_origin(msg->msg_data, file->origin);
    vss_release_file_set_handle(msg->msg_data, file->server_handle);

    msg->file_id = file->file_id;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_set_lock_state(vssts_file_t* file,
                                            VSSI_FileLockState lock_state,
                                            void* ctx,
                                            VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_SET_LOCK_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_SET_LOCK, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_set_lock_set_mode(msg->msg_data, lock_state);
    vss_set_lock_set_objhandle(msg->msg_data, handle);
    vss_set_lock_set_fhandle(msg->msg_data, file->server_handle);

    msg->file_id = file->file_id;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_get_lock_state(vssts_file_t* file,
                                            VSSI_FileLockState* lock_state,
                                            void* ctx,
                                            VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_GET_LOCK_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this, VSS_GET_LOCK, hdr, msg_len, ctx, callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_get_lock_set_objhandle(msg->msg_data, handle);
    vss_get_lock_set_fhandle(msg->msg_data, file->server_handle);

    msg->file_id = file->file_id;
    msg->ret_lock_state = lock_state;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

void vssts_object::file_set_byte_range_lock(vssts_file_t* file,
                                            VSSI_ByteRangeLock* br_lock,
                                            u32 flags,
                                            void* ctx,
                                            VSSI_Callback callback)
{
    vssts_msg_t* msg = NULL;
    int msg_len = VSS_HEADER_SIZE + VSS_SET_LOCK_RANGE_SIZE;
    char* hdr = NULL;
    VSSI_Result rc = VSSI_COMM;

    // Here's where we craft a message to send.
    hdr = new (std::nothrow) char[msg_len];
    if (hdr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    msg = new (std::nothrow) vssts_msg_t(this,
                                         VSS_SET_LOCK_RANGE,
                                         hdr,
                                         msg_len,
                                         ctx,
                                         callback);
    if (msg == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        rc = VSSI_NOMEM;
        goto done;
    }

    // now the command specific hdr
    vss_set_lock_range_set_origin(msg->msg_data, file->origin);
    vss_set_lock_range_set_start(msg->msg_data, br_lock->offset);
    vss_set_lock_range_set_end(msg->msg_data,
                               br_lock->offset + br_lock->length - 1);
    vss_set_lock_range_set_mode(msg->msg_data, br_lock->lock_mask);
    vss_set_lock_range_set_handle(msg->msg_data, file->server_handle);
    vss_set_lock_range_set_flags(msg->msg_data, flags);

    msg->file_id = file->file_id;

    VPLMutex_Lock(&mutex);
    if (tunnel) {
        rc = tunnel->send_msg(msg);
    }
    VPLMutex_Unlock(&mutex);

done:
    if (rc != VSSI_SUCCESS) {
        if (msg != NULL) {
            delete msg;
        }
        if (hdr != NULL) {
            delete[] hdr;
        }
        callback(ctx, rc);
    }
}

VSSI_Result VSSI_InternalInit(void)
{
    return VSSI_SUCCESS;
}

void VSSI_InternalCleanup(void)
{
}

}
