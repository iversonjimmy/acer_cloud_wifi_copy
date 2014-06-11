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


#include "vplex_trace.h"

#include "vss_comm.h" /// From client lib: command, reply definitions
#include "vssi_error.h" /// From client lib: error definitions
#include "vssi_types.h" /// From client lib: symbol definitions

#include "vss_server.hpp"
#include "vss_cmdproc.hpp"
#include "vss_object.hpp"
#include "vss_query.hpp"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <sstream>

using namespace std;

/// Error response generator
char* generate_error(s16 reason)
{
    char* rv = NULL;

    rv = (char*)calloc(VSS_HEADER_SIZE, 1);
    if(rv != NULL) {
        // Error responses won't be signed.
        vss_set_version(rv, 0);
        vss_set_command(rv, VSS_ERROR);
        vss_set_status(rv, reason);
        vss_set_data_length(rv, 0);
    }

    return rv;
}

static string assign_component_name(const char* data, size_t len)
{
    string rv;
    rv.assign(data, len);
    // Component names must not have leading or trailing slashes.
    while(rv.size() > 0 && rv[0] == '/') {
        rv.erase(0, 1);
    }
    while(rv.size() > 0 && rv[rv.size() - 1] == '/') {
        rv.erase(rv.size() - 1, 1);
    }
    // component names must not have trailing NULL characters.
    while(rv.size() > 0 && rv[rv.size() - 1] == '\0') {
        rv.erase(rv.size() - 1, 1);
    }

    return rv;
}

#ifdef PERF_STATS
struct vss_cmds_s {
    int         opcode;
    const char* name;
};
typedef struct vss_cmds_s vss_cmds_t;

vss_cmds_t cmd_list[] = {
    {VSS_NOOP, "VSS_NOOP"},
    {VSS_OPEN, "VSS_OPEN"},
    {VSS_CLOSE, "VSS_CLOSE"},
    {VSS_START_SET, "VSS_START_SET"},
    {VSS_COMMIT, "VSS_COMMIT"},
    {VSS_DELETE, "VSS_DELETE"},
    {VSS_ERASE, "VSS_ERASE"},
    {VSS_READ, "VSS_READ"},
    {VSS_READ_DIFF, "VSS_READ_DIFF"},
    {VSS_READ_DIR, "VSS_READ_DIR"},
    {VSS_STAT, "VSS_STAT"},
    {VSS_READ_TRASHCAN, "VSS_READ_TRASHCAN"},
    {VSS_READ_TRASH, "VSS_READ_TRASH"},
    {VSS_READ_TRASH_DIR, "VSS_READ_TRASH_DIR"},
    {VSS_STAT_TRASH, "VSS_STAT_TRASH"},
    {VSS_WRITE, "VSS_WRITE"},
    {VSS_MAKE_REF, "VSS_MAKE_REF"},
    {VSS_MAKE_DIR, "VSS_MAKE_DIR"},
    {VSS_REMOVE, "VSS_REMOVE"},
    {VSS_RENAME, "VSS_RENAME"},
    {VSS_RENAME2, "VSS_RENAME2"},
    {VSS_COPY, "VSS_COPY"},
    {VSS_COPY_MATCH, "VSS_COPY_MATCH="},
    {VSS_SET_SIZE, "VSS_SET_SIZE"},
    {VSS_SET_METADATA, "VSS_SET_METADATA"},
    {VSS_EMPTY_TRASH, "VSS_EMPTY_TRASH"},
    {VSS_DELETE_TRASH, "VSS_DELETE_TRASH"},
    {VSS_RESTORE_TRASH, "VSS_RESTORE_TRASH"},
    {VSS_GET_SPACE, "VSS_GET_SPACE"},
    {VSS_RELEASE_FILE, "VSS_RELEASE_FILE"},
    {VSS_SET_LOCK, "VSS_SET_LOCK"},
    {VSS_GET_LOCK, "VSS_GET_LOCK"},
    {VSS_SET_LOCK_RANGE, "VSS_SET_LOCK_RANGE"},
    {VSS_GET_NOTIFY, "VSS_GET_NOTIFY"},
    {VSS_SET_NOTIFY, "VSS_SET_NOTIFY"},
    {VSS_NOTIFICATION, "VSS_NOTIFICATION"},
    {VSS_OPEN_FILE, "VSS_OPEN_FILE"},
    {VSS_CLOSE_FILE, "VSS_CLOSE_FILE"},
    {VSS_READ_FILE, "VSS_READ_FILE"},
    {VSS_WRITE_FILE, "VSS_WRITE_FILE"},
    {VSS_TRUNCATE_FILE, "VSS_TRUNCATE_FILE"},
    {VSS_STAT_FILE, "VSS_STAT_FILE"},
    {VSS_CHMOD, "VSS_CHMOD"},
    {VSS_MAKE_DIR2, "VSS_MAKE_DIR2"},
    {VSS_READ_DIR2, "VSS_READ_DIR2"},
    {VSS_STAT2, "VSS_STAT2"},
    {VSS_NEGOTIATE, "VSS_NEGOTIATE"},
    {VSS_TUNNEL_RESET, "VSS_TUNNEL_RESET"},
    {VSS_AUTHENTICATE, "VSS_AUTHENTICATE"},
    {VSS_TUNNEL_DATA, "VSS_TUNNEL_DATA"},
    {VSS_PROXY_REQUEST, "VSS_PROXY_REQUEST"},
    {VSS_PROXY_CONNECT, "VSS_PROXY_CONNECT"},
    {VSS_SET_TIMES, "VSS_SET_TIMES"},
    {VSS_CHMOD_FILE, "VSS_CHMOD_FILE"},
};
#define _ARRAY_LEN(x)    (sizeof(x)/sizeof(x[0]))

static void dump_stat(const char *name, vss_stat_t& stat)
{
    if ( stat.count == 0 ) {
        return;
    }
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
        "cmd %s: cnt: %d min: "FMT_VPLTime_t" max: "FMT_VPLTime_t" cum: "
        FMT_VPLTime_t" avg: "FMT_VPLTime_t,
        name, 
        stat.count,
        stat.time_min,
        stat.time_max,
        stat.time_cum,
        stat.count ?  stat.time_cum / stat.count : 0);
    memset(&stat, 0, sizeof(stat));
}

void vss_server::dump_stats(void)
{
    for( int i = 0 ; i < _ARRAY_LEN(cmd_list) ; i++ ) {
        dump_stat(cmd_list[i].name, stats[cmd_list[i].opcode]);
    }
}

void vss_server::update_stat(VPLTime_t time_start, 
                             vss_stat_t& stat)
{
    VPLTime_t time_dur;

    time_dur = VPLTime_DiffClamp(VPLTime_GetTimeStamp(), time_start);
    VPLMutex_Lock(&stats_mutex);
    stat.count++;
    stat.time_cum += time_dur;
    if ( (stat.time_min == 0) || (stat.time_min > time_dur)) {
        stat.time_min = time_dur;
    }
    if ( (stat.time_max < time_dur)) {
        stat.time_max = time_dur;
    }
    VPLMutex_Unlock(&stats_mutex);
}

static const char * cmd_id_to_str(int cmd)
{
    for( int i = 0 ; i < _ARRAY_LEN(cmd_list) ; i++ ) {
        if ( cmd_list[i].opcode == cmd ) {
            return cmd_list[i].name;
        }
    }

    return "Unknown";
}

#endif // PERF_STATS

bool vss_server::handle_command(vss_req_proc_ctx* context)
{
    int cmd_id = vss_get_command(context->header);
    char* response = NULL;
    bool rv = false;

#ifdef PERF_STATS
    context->time_start = VPLTime_GetTimeStamp();

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "received command %s[%d]",
        cmd_id_to_str(cmd_id), cmd_id);
#endif // PERF_STATS
    
    switch(cmd_id) {
    case VSS_NOOP:
        response = handle_noop(context); break;
    case VSS_OPEN:
        response = handle_open(context); break;
    case VSS_CLOSE:
        response = handle_close(context); break;
    case VSS_START_SET:
        response = handle_start_set(context); break;
    case VSS_COMMIT:
        response = handle_commit(context); break;
    case VSS_DELETE:
        response = handle_delete(context); break;
    case VSS_ERASE:
        response = handle_erase(context); break;
    case VSS_READ_DIR:
        response = handle_read_dir(context); break;
    case VSS_STAT:
        response = handle_stat(context); break;
    case VSS_MAKE_DIR:
        response = handle_make_dir(context); break;
    case VSS_REMOVE:
        response = handle_remove(context); break;
    case VSS_RENAME:
        response = handle_rename(context); break;
    case VSS_RENAME2:
        response = handle_rename2(context); break;
    case VSS_COPY:
        response = handle_copy(context); break;
    case VSS_SET_SIZE:
        response = handle_set_size(context); break;
    case VSS_SET_TIMES:
        response = handle_set_times(context); break;
    case VSS_SET_METADATA:
        response = handle_set_metadata(context); break;
    case VSS_GET_SPACE:
        response = handle_get_space(context); break;
    case VSS_SET_NOTIFY:
        response = handle_set_notify(context); break;
    case VSS_GET_NOTIFY:
        response = handle_get_notify(context); break;
    case VSS_OPEN_FILE:
        response = handle_open_file(context); break;
    case VSS_READ_FILE:
        response = handle_read_file(context); break;
    case VSS_WRITE_FILE:
        response = handle_write_file(context); break;
    case VSS_TRUNCATE_FILE:
        response = handle_truncate_file(context); break;
    case VSS_CHMOD_FILE:
        response = handle_chmod_file(context); break;
    case VSS_SET_LOCK:
        response = handle_set_lock(context); break;
    case VSS_GET_LOCK:
        response = handle_get_lock(context); break;
    case VSS_SET_LOCK_RANGE:
        response = handle_set_lock_range(context); break;
    case VSS_RELEASE_FILE:
        response = handle_release_file(context); break;
    case VSS_CLOSE_FILE:
        response = handle_close_file(context); break;
    case VSS_NEGOTIATE:
        response = handle_negotiate(context); break;
    case VSS_CHMOD:
        response = handle_chmod(context); break;
    case VSS_MAKE_DIR2:
        response = handle_make_dir2(context); break;
    case VSS_READ_DIR2:
        response = handle_read_dir2(context); break;
    case VSS_STAT2:
        response = handle_stat2(context); break;
    case VSS_AUTHENTICATE:
        response = handle_authenticate(context); break;
    default:
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "invalid cmd_id %u.", cmd_id);
        response = generate_error(VSSI_BADCMD); break;
    }

    if(response != NULL) {
#ifdef PERF_STATS
        update_stat(context->time_start, stats[cmd_id]);
#endif // PERF_STATS
        send_client_response(response, context);
        rv = true;
    }

#ifdef PERF_STATS
    if ( cmd_id == VSS_CLOSE ) {
        VPLMutex_Lock(&stats_mutex);
        dump_stats();
        VPLMutex_Unlock(&stats_mutex);
    }
#endif // PERF_STATS

    return rv;
}

/// NOOP ver. 0
char* vss_server::handle_noop(vss_req_proc_ctx* context)
{
    // Reply with ERROR, but with no error code.
    return generate_error(VSSI_SUCCESS);
}

/// OPEN ver. 0
char* vss_server::handle_open(vss_req_proc_ctx* context)
{
    char* resp = NULL;
    s16 rc = VSSI_SUCCESS;
    char* req = context->header;
    char* data = context->body;
    dataset* dset = NULL;
    vss_object* object = NULL;
    u32 mode = vss_open_get_mode(data);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" "FMTu64":"FMTu64" open mode %u request.",
                      vss_get_device_id(req),
                      vss_open_get_uid(data), vss_open_get_did(data), mode);

    // Find the dataset.
    rc = getDataset(vss_open_get_uid(data),
                    vss_open_get_did(data),
                    dset);
    if(rc != VSSI_SUCCESS) {
        goto reply;
    }

    // Create object for dataset access (dataset reference passed).
    object = context->session->new_object(dset,
                                          vss_get_device_id(req),
                                          vss_open_get_uid(req),
                                          mode);
    if(object == NULL) {
        // Failed to open object.
        rc = VSSI_OLIMIT;
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMTu64":"FMTu64" open mode %u: object limit reached.",
                          vss_get_device_id(req),
                          vss_open_get_uid(data), vss_open_get_did(data),
                          mode);
        goto reply;
    }
    
    // Leave this reference to keep object in-scope.
    dset = NULL;

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" "FMTu64":"FMTu64" open mode %u result:%d (object %p).",
                      vss_get_device_id(req),
                      vss_open_get_uid(data), vss_open_get_did(data),
                      mode, rc, object);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_OPENR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;

        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_OPEN_REPLY);
        vss_set_data_length(resp, VSS_OPENR_SIZE);
        vss_set_status(resp, rc);
        vss_open_reply_set_handle(resp_data, (object) ? object->get_handle() : 0);
        vss_open_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
    }

    if(dset) {
        dset->release();
    }
    return resp;
}

/// CLOSE ver. 0
char* vss_server::handle_close(vss_req_proc_ctx* context)
{
    char* resp = NULL;
    const char* req = context->header;
    const char* data = context->body;
    u32 handle = vss_close_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    s16 rc = VSSI_SUCCESS;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u close request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    object->commands_processed++;
    object->close();
    if(rc >= VSSI_SUCCESS) {
        // once more to really delete the object
        context->session->release_object(handle);
    }

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u close response: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_CLOSER_SIZE, 1);
    if(resp) {
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_CLOSE_REPLY);
        vss_set_data_length(resp, VSS_CLOSER_SIZE);
        vss_set_status(resp, rc);
    }
    return resp;
}

/// START_SET ver. 0
char* vss_server::handle_start_set(vss_req_proc_ctx* context)
{
    char* resp = NULL;
    const char* req = context->header;
    const char* data = context->body;
    u32 handle = vss_start_set_get_handle(data);
    VSSI_Result rc;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u start set request.",
                      vss_get_device_id(req), handle);

    rc = VSSI_BADCMD;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%d start set response: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;

        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_START_SET_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_set_status(resp, rc);
        vss_version_reply_set_objver(resp_data, 0);
    }

    return resp;
}

/// COMMIT ver. 0
char* vss_server::handle_commit(vss_req_proc_ctx* context)
{
    char* resp = NULL;
    const char* req = context->header;
    const char* data = context->body;
    u32 handle = vss_commit_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    s16 rc = VSSI_SUCCESS;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u commit request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    rc = object->get_dataset()->commit_iw();

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u commit result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_COMMIT_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
    }
    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// ERASE ver. 0
char* vss_server::handle_erase(vss_req_proc_ctx* context)
{
    char* resp = NULL;
    const char* req = context->header;
    const char* data = context->body;
    VSSI_Result rc;
    u32 handle = vss_erase_get_handle(data);
    vss_object* object = context->session->find_object(handle);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u erase request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    rc = object->get_dataset()->remove_iw("/");

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%d erase result:%d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_ERASE_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_set_status(resp, rc);
        vss_version_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

char* vss_server::handle_delete(vss_req_proc_ctx* context)
{
    char* resp = NULL;
    const char* req = context->header;
    const char* data = context->body;
    VSSI_Result rc;
    dataset* dset = NULL;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" "FMTu64":"FMTu64" delete request.",
                      vss_get_device_id(req),
                      vss_delete_get_uid(data), vss_delete_get_did(data));

    rc = getDataset(vss_delete_get_uid(data), vss_delete_get_did(data), dset);
    if(rc != VSSI_SUCCESS) {
        goto reply;
    }

    rc = dset->delete_all();

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" "FMTu64":"FMTu64" delete result: %d.",
                      vss_get_device_id(req),
                      vss_delete_get_uid(data), vss_delete_get_did(data), rc);
    resp = (char*)calloc(VSS_HEADER_SIZE, 1);
    if(resp) {
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_DELETE_REPLY);
        vss_set_data_length(resp, 0);
        vss_set_status(resp, rc);
    }

    if(dset) {
        dset->release();
    }

    return resp;
}

/// READ_DIR ver. 0
char* vss_server::handle_read_dir(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_read_dir_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    string read_data;
    string name;
    VSSI_Result rc;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u read directory request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_read_dir_get_name(data),
                                 vss_read_dir_get_name_length(data));

    rc = object->get_dataset()->read_dir(name, read_data);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u read dir(%s) result: %d.",
                      vss_get_device_id(req), handle, name.c_str(), rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_READ_DIRR_BASE_SIZE + read_data.size(), 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_READ_DIR_REPLY);
        vss_set_data_length(resp, VSS_READ_DIRR_BASE_SIZE + read_data.size());
        vss_set_status(resp, rc);
        vss_read_dir_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
        vss_read_dir_reply_set_length(resp_data, read_data.size());
        if(read_data.size() > 0) {
            memcpy(vss_read_dir_reply_get_data(resp_data), 
                   read_data.data(), read_data.size());
        }
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// READ_DIR2 ver. 0
char* vss_server::handle_read_dir2(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_read_dir_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    string read_data;
    string name;
    VSSI_Result rc;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u read directory request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_read_dir_get_name(data),
                                 vss_read_dir_get_name_length(data));

    rc = object->get_dataset()->read_dir2(name, read_data);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u read dir result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_READ_DIRR_BASE_SIZE + read_data.size(), 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_READ_DIR2_REPLY);
        vss_set_data_length(resp, VSS_READ_DIRR_BASE_SIZE + read_data.size());
        vss_set_status(resp, rc);
        vss_read_dir_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
        vss_read_dir_reply_set_length(resp_data, read_data.size());
        if(read_data.size() > 0) {
            memcpy(vss_read_dir_reply_get_data(resp_data), 
                   read_data.data(), read_data.size());
        }
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}


/// STAT ver. 0
char* vss_server::handle_stat(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_stat_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    string read_data;
    string name;
    VSSI_Result rc;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u stat request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_stat_get_name(data),
                                 vss_stat_get_name_length(data));

    rc = object->get_dataset()->stat_component(name, read_data);


 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u stat result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_READ_DIRR_BASE_SIZE + read_data.size(), 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_STAT_REPLY);
        vss_set_data_length(resp, VSS_READ_DIRR_BASE_SIZE + read_data.size());
        vss_set_status(resp, rc);
        vss_read_dir_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
        vss_read_dir_reply_set_length(resp_data, read_data.size());
        if(read_data.size() > 0) {
            memcpy(vss_read_dir_reply_get_data(resp_data), 
                   read_data.data(), read_data.size());
        }
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// STAT ver. 0
char* vss_server::handle_stat2(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_stat_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    string read_data;
    string name;
    VSSI_Result rc;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u stat2 request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_stat_get_name(data),
                                 vss_stat_get_name_length(data));

    rc = object->get_dataset()->stat2_component(name, read_data);


 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u stat2 result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_READ_DIRR_BASE_SIZE + read_data.size(), 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_STAT2_REPLY);
        vss_set_data_length(resp, VSS_READ_DIRR_BASE_SIZE + read_data.size());
        vss_set_status(resp, rc);
        vss_read_dir_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
        vss_read_dir_reply_set_length(resp_data, read_data.size());
        if(read_data.size() > 0) {
            memcpy(vss_read_dir_reply_get_data(resp_data), 
                   read_data.data(), read_data.size());
        }
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// MAKE_DIR ver. 0
char* vss_server::handle_make_dir(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_make_dir_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u make dir request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_make_dir_get_name(data), vss_make_dir_get_name_length(data));

    rc = object->get_dataset()->make_directory_iw(name, 0);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u make dir result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_MAKE_DIR_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                      (object) ? object->get_version() : 0);

    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// MAKE_DIR ver. 0
char* vss_server::handle_make_dir2(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_make_dir2_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u make dir2 request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_make_dir2_get_name(data), vss_make_dir2_get_name_length(data));

    rc = object->get_dataset()->make_directory_iw(name,
        vss_make_dir2_get_attrs(data));

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u make dir2 result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_MAKE_DIR2_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                      (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// CHMOD ver. 0
char* vss_server::handle_chmod(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_chmod_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u chmod request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_chmod_get_name(data), vss_chmod_get_name_length(data));

    rc = object->get_dataset()->chmod_iw(name, vss_chmod_get_attrs(data),
        vss_chmod_get_attrs_mask(data)); 
 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u chmod result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_CHMOD_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                      (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}


char* vss_server::handle_copy(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_copy_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;
    string new_name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u copy request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }


    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_copy_get_source(data), vss_copy_get_source_length(data));
    new_name = assign_component_name(vss_copy_get_destination(data), vss_copy_get_destination_length(data));

    rc = object->get_dataset()->copy_file(name, new_name);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u copy result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_COPY_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                    (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// REMOVE ver. 0
char* vss_server::handle_remove(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_remove_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u remove request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_remove_get_name(data), vss_remove_get_name_length(data));
    rc = object->get_dataset()->remove_iw(name);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u remove result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_REMOVE_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                    object ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// RENAME ver. 0
char* vss_server::handle_rename(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_rename_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;
    string new_name;
    u32 flags = 0;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u rename request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_rename_get_name(data), vss_rename_get_name_length(data));
    new_name = assign_component_name(vss_rename_get_new_name(data), vss_rename_get_new_name_length(data));

    rc = object->get_dataset()->rename_iw(name, new_name, flags);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u rename result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_RENAME_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                    (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

char* vss_server::handle_rename2(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_rename2_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;
    string new_name;
    u32 flags;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u rename2 request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_rename2_get_name(data), vss_rename2_get_name_length(data));
    new_name = assign_component_name(vss_rename2_get_new_name(data), vss_rename2_get_new_name_length(data));
    flags = vss_rename2_get_flags(data);

    rc = object->get_dataset()->rename_iw(name, new_name, flags);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u rename2 result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_RENAME2_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                    (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// SET_TIMES ver. 0
char* vss_server::handle_set_times(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_set_times_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u set times request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_set_times_get_name(data), vss_set_times_get_name_length(data));

    rc = object->get_dataset()->set_times_iw(name, 
                                             vss_set_times_get_ctime(data),
                                             vss_set_times_get_mtime(data));

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u set size result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_SET_TIMES_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                     (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// SET_SIZE ver. 0
char* vss_server::handle_set_size(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_set_size_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u set size request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_set_size_get_name(data), vss_set_size_get_name_length(data));

    rc = object->get_dataset()->set_size_iw(name,
                                            vss_set_size_get_size(data));

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u set size result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_SET_SIZE_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                     (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

/// SET_METADATA ver. 0
char* vss_server::handle_set_metadata(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_set_metadata_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc;
    string name;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u set_metadata request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    if(!(object->get_flags() & VSS_OPEN_MODE_READWRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_set_metadata_get_name(data), vss_set_metadata_get_name_length(data));

    rc = object->get_dataset()->set_metadata_iw(name,
         vss_set_metadata_get_type(data),
         vss_set_metadata_get_length(data),
         vss_set_metadata_get_value(data));

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u set metadata result: %d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_status(resp, rc);
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_SET_METADATA_REPLY);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data,
                                     (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

char* vss_server::handle_get_space(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_get_space_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    u64 disk_size = 0, dataset_size = 0, avail_size = 0;
    VSSI_Result rc;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u get space request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // handle_get_space() must not hold off managed dataset db backup or fsck: Bug 9515

    rc = object->get_dataset()->get_total_used_size(dataset_size);
    if (rc != VSSI_SUCCESS) {
        goto reply;
    }

    // Get disk space information using the VSS server root path
    rc = object->get_dataset()->get_space(rootDir, disk_size, avail_size);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Error %d getting dataset space for path %s.", rc, rootDir.c_str());
        goto reply;
    }

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u get space result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_GET_SPACER_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_GET_SPACE_REPLY);
        vss_set_data_length(resp, VSS_GET_SPACER_SIZE);
        vss_set_status(resp, rc);
        vss_get_space_reply_set_disk_size(resp_data, disk_size);
        vss_get_space_reply_set_dataset_size(resp_data, dataset_size);
        vss_get_space_reply_set_avail_size(resp_data, avail_size);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

char* vss_server::handle_open_file(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    u32 handle = vss_open_file_get_handle(data);
    char* resp = NULL;
    string name;
    VSSI_Result rc = 0;
    vss_file* file_handle = NULL;
    u32 file_id = 0;
    vss_object* object = context->session->find_object(handle);
    u32 flags = vss_open_file_get_flags(data);
    u64 origin = vss_open_file_get_origin(data);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Make sure component name has no leading slashes.
    // Names are all relative to object root.
    name = assign_component_name(vss_open_file_get_name(data), vss_open_file_get_name_length(data));

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u open file request: %s.",
                      vss_get_device_id(req), handle, name.c_str());

    rc = object->get_dataset()->open_file(name,
                                          vss_open_file_get_objver(data),
                                          flags,
                                          vss_open_file_get_attrs(data),
                                          file_handle);
    //
    // Don't send the vss_file* back to the client, use the file_id.
    // Also need to connect the object to the vss_file for object teardown.
    //
    if(file_handle) {
        file_id = file_handle->get_file_id();
        file_handle->add_object(object);
        file_handle->add_origin(origin, flags);
        file_handle->add_open_origin(origin);
    }
 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u open file result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char *)calloc(VSS_HEADER_SIZE + VSS_OPEN_FILER_BASE_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_OPEN_FILE_REPLY);
        vss_set_data_length(resp, VSS_OPEN_FILER_BASE_SIZE);
        vss_set_status(resp, rc);
        vss_open_file_reply_set_handle(resp_data, file_id);
        vss_open_file_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

char* vss_server::handle_read_file(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* handle = NULL;
    u32 file_id = vss_read_file_get_handle(data);
    vss_object* object;
    u32 objhandle;
    char* resp = NULL;
    char* resp_data = NULL;
    VSSI_Result rc = 0;
    u32 length = vss_read_file_get_length(data);
    u64 offset = vss_read_file_get_offset(data);
    u64 origin = vss_read_file_get_origin(data);

    objhandle = (u32)(origin & 0xffffffffULL);
    object = context->session->find_object(objhandle);
    if (object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Translate client file ID to vss_file object
    handle = object->get_dataset()->map_file_id(file_id);
    if(handle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    if(!(handle->get_access_mode() & VSSI_FILE_OPEN_READ)) {
        // Not a readable object.
        rc = VSSI_PERM;
        goto reply;
    }

    // Limit read length to a reasonable maximum: 1MB.
    if(length > (1<<20)) {
        length = (1<<20);
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" #%p read file limited from %u to %u bytes.",
                          vss_get_device_id(req), handle,
                          length, (1<<20));
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p read file %s off "FMTu64" len %u from originator "FMTx64".",
                      vss_get_device_id(req), handle, handle->component.c_str(), offset, length, origin);

    //
    // Convert origin into (client, origin) pair to get global uniqueness
    //

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_READ_FILER_BASE_SIZE + length, 1);
    if (resp == NULL) {
        goto reply;
    }
    resp_data = resp + VSS_HEADER_SIZE;

    rc = handle->get_dataset()->read_file(handle, object, origin, offset, length, 
                                          vss_read_file_reply_get_data(resp_data));

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p read file result: %d.",
                      vss_get_device_id(req), handle, rc);
    // Handle error case only, e.g. VSSI_BADOBJ case
    if(!resp) {
        resp = (char*)calloc(VSS_HEADER_SIZE + VSS_READ_FILER_BASE_SIZE + length, 1);
        resp_data = resp + VSS_HEADER_SIZE;
        length = 0;
    }
    //
    // The request was blocked by an oplock and needs to be queued and retried later.
    // Note that actual reads and writes do not block on byte range locks, but return
    // an error synchronously.
    //
    if(rc == VSSI_RETRY) {
        handle->wait_oplock(context, object, false);
        free(resp);
        resp = NULL;
    }
    else if(resp) {
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_READ_FILE_REPLY);
        vss_set_data_length(resp, VSS_READ_FILER_BASE_SIZE + length);
        vss_set_status(resp, rc);
        vss_read_file_reply_set_length(resp_data, length);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }

    return resp;
}

char* vss_server::handle_write_file(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* handle = NULL;
    u32 file_id = vss_write_file_get_handle(data);
    vss_object* object;
    u32 objhandle;
    char* resp = NULL;
    VSSI_Result rc = 0;
    u32 length = vss_write_file_get_length(data);
    u64 offset = vss_write_file_get_offset(data);
    u64 origin = vss_write_file_get_origin(data);

    objhandle = (u32)(origin & 0xffffffffULL);
    object = context->session->find_object(objhandle);
    if (object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Translate client file ID to vss_file object
    handle = object->get_dataset()->map_file_id(file_id);
    if(handle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    if(!(handle->get_access_mode() & VSSI_FILE_OPEN_WRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p write name %s off "FMTu64" len %u from originator "FMTx64".",
                      vss_get_device_id(req), handle, handle->component.c_str(), offset, length, origin);

    rc = handle->get_dataset()->write_file(handle, object, origin, 
                                           offset, length, vss_write_file_get_data(data));
 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p write file result: %d.",
                      vss_get_device_id(req), handle, rc);

    //
    // The request was blocked by an oplock and needs to be queued and retried later.
    // Note that actual reads and writes do not block on byte range locks, but return
    // an error synchronously.
    //
    if(rc == VSSI_RETRY) {
        handle->wait_oplock(context, object, true);
        resp = NULL;
        goto done;
    }
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_WRITE_FILER_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_WRITE_FILE_REPLY);
        vss_set_data_length(resp, VSS_WRITE_FILER_SIZE);
        vss_set_status(resp, rc);
        vss_write_file_reply_set_length(resp_data, length);
    }
 done:
    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_truncate_file(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* handle = NULL;
    u32 file_id = vss_truncate_file_get_handle(data);
    vss_object* object;
    u32 objhandle;
    char* resp = NULL;
    VSSI_Result rc = 0;
    u64 offset = vss_truncate_file_get_offset(data);
    u64 origin = vss_truncate_file_get_origin(data);

    objhandle = (u32)(origin & 0xffffffffULL);
    object = context->session->find_object(objhandle);
    if (object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Translate client file ID to vss_file object
    handle = object->get_dataset()->map_file_id(file_id);
    if(handle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    if(!(handle->get_access_mode() & VSSI_FILE_OPEN_WRITE)) {
        // Not a writeable object.
        rc = VSSI_PERM;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p truncate name %s off "FMTu64" from originator "FMTx64".",
                      vss_get_device_id(req), handle, handle->component.c_str(), offset, origin);

    rc = handle->get_dataset()->truncate_file(handle, object, origin, offset);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p truncate file result: %d.",
                      vss_get_device_id(req), handle, rc);
    //
    // The request was blocked by an oplock and needs to be queued and retried later.
    // Note that actual io operations (R/W/T) do not block on byte range locks, but return
    // an error synchronously.
    //
    if(rc == VSSI_RETRY) {
        handle->wait_oplock(context, object, true);
        resp = NULL;
        goto done;
    }
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_TRUNCATE_FILER_SIZE, 1);
    if(resp) {
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_TRUNCATE_FILE_REPLY);
        vss_set_data_length(resp, VSS_TRUNCATE_FILER_SIZE);
        vss_set_status(resp, rc);
    }

 done:
    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_chmod_file(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* handle = NULL;
    u32 file_id = vss_chmod_file_get_handle(data);
    vss_object* object;
    u32 objhandle;
    char* resp = NULL;
    VSSI_Result rc = 0;
    u64 origin = vss_chmod_file_get_origin(data);
    u32 attrs = vss_chmod_file_get_attrs(data);
    u32 attrs_mask = vss_chmod_file_get_attrs_mask(data);

    objhandle = (u32)(origin & 0xffffffffULL);
    object = context->session->find_object(objhandle);
    if (object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Translate client file ID to vss_file object
    handle = object->get_dataset()->map_file_id(file_id);
    if(handle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p chmod name %s attrs:mask %08x:%08x from originator "FMTx64".",
                      vss_get_device_id(req), handle, handle->component.c_str(), attrs, attrs_mask, origin);

    rc = handle->get_dataset()->chmod_file(handle, object, origin, attrs, attrs_mask);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p chmod file result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_CHMOD_FILER_SIZE, 1);
    if(resp) {
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_CHMOD_FILE_REPLY);
        vss_set_data_length(resp, VSS_CHMOD_FILER_SIZE);
        vss_set_status(resp, rc);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_set_lock(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* fhandle = NULL;
    u32 file_id = vss_set_lock_get_fhandle(data);
    u32 objhandle = vss_set_lock_get_objhandle(data);
    u64 mode = vss_set_lock_get_mode(data);
    vss_object* object = context->session->find_object(objhandle);
    char* resp = NULL;
    VSSI_Result rc = 0;

    // Translate client file ID to vss_file object
    if (object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    fhandle = object->get_dataset()->map_file_id(file_id);
    if(fhandle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p set lock request.",
                      vss_get_device_id(req), fhandle);

    rc = fhandle->set_oplock(object, mode);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p set file lock result: %d.",
                      vss_get_device_id(req), fhandle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE, 1);
    if(resp) {
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_SET_LOCK_REPLY);
        vss_set_data_length(resp, 0);
        vss_set_status(resp, rc);
    }
    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_get_lock(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* fhandle = NULL;
    u32 file_id = vss_get_lock_get_fhandle(data);
    u32 objhandle = vss_get_lock_get_objhandle(data);
    u64 mode = 0ULL;
    vss_object* object = context->session->find_object(objhandle);
    char* resp = NULL;
    VSSI_Result rc = 0;

    // Translate client file ID to vss_file object
    if (object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    fhandle = object->get_dataset()->map_file_id(file_id);
    if(fhandle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p get lock request.",
                      vss_get_device_id(req), fhandle);

    rc = fhandle->get_oplock(object, mode);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p get file lock result: %d, mode "FMTx64".",
                      vss_get_device_id(req), fhandle, rc, mode);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_GET_LOCKR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_GET_LOCK_REPLY);
        vss_set_data_length(resp, VSS_GET_LOCKR_SIZE);
        vss_get_lock_reply_set_mode(resp_data, mode);
        vss_set_status(resp, rc);
    }
    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_set_lock_range(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_object* object = NULL;
    u32 objhandle;
    vss_file* handle = NULL;
    u32 file_id = vss_set_lock_range_get_handle(data);
    u64 start = vss_set_lock_range_get_start(data);
    u64 end = vss_set_lock_range_get_end(data);
    u64 origin = vss_set_lock_range_get_origin(data);
    u64 mode = vss_set_lock_range_get_mode(data);
    u32 flags = vss_set_lock_range_get_flags(data);
    char* resp = NULL;
    VSSI_Result rc = 0;

    objhandle = (u32)(origin & 0xffffffffULL);
    object = context->session->find_object(objhandle);
    if (object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Translate client file ID to vss_file object
    handle = object->get_dataset()->map_file_id(file_id);
    if(handle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p set lock range request for {%s}: origin "FMTx64
                      ", range "FMTu64":"FMTu64", M"FMTx64", F%08x.",
                      vss_get_device_id(req), handle, handle->component.c_str(),
                      origin, start, end, mode, flags);

    rc = handle->set_lock_range(object, origin, flags, mode, start, end);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p set lock range result: %d.",
                      vss_get_device_id(req), handle, rc);
    //
    // The request was blocked by an oplock and needs to be queued and retried later.
    // All oplocks must be released before the byte range lock can be set, so this
    // request needs to be retried after the oplock breaks are handled by the owners
    // of the oplocks.
    //
    if(rc == VSSI_RETRY) {
        handle->wait_oplock(context, object, false);
        free(resp);
        resp = NULL;
        goto done;
    }
    //
    // The request was blocked by another file lock is VSSI_RANGE_WAIT mode and 
    // needs to be queued and retried when the conflicting lock is released.
    //
    if(rc == VSSI_WAITLOCK) {
        handle->wait_brlock(context, origin, mode, start, end);
        resp = NULL;
        goto done;
    }

    resp = (char*)calloc(VSS_HEADER_SIZE, 1);
    if(resp) {
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_SET_LOCK_RANGE_REPLY);
        vss_set_data_length(resp, 0);
        vss_set_status(resp, rc);
    }
 done:
    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_release_file(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* handle = NULL;
    u32 file_id = vss_release_file_get_handle(data);
    vss_object* object;
    u32 objhandle;
    u64 origin;
    char* resp = NULL;
    VSSI_Result rc = 0;

    origin = vss_release_file_get_origin(data);

    objhandle = (u32)(origin & 0xffffffffULL);
    object = context->session->find_object(objhandle);
    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Translate client file ID to vss_file object
    handle = object->get_dataset()->map_file_id(file_id);
    if(handle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p release file request: %s.",
                      vss_get_device_id(req), handle, handle->component.c_str());

    rc = handle->get_dataset()->release_file(handle, object, origin);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p release file result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_RELEASE_FILE_REPLY);
        vss_set_data_length(resp, 0);
        vss_set_status(resp, rc);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_close_file(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    vss_file* handle = NULL;
    u32 file_id = vss_close_file_get_handle(data);
    vss_object* object;
    u32 objhandle;
    u64 origin;
    char* resp = NULL;
    VSSI_Result rc = 0;

    origin = vss_close_file_get_origin(data);

    objhandle = (u32)(origin & 0xffffffffULL);
    object = context->session->find_object(objhandle);
    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    // Translate client file ID to vss_file object
    handle = object->get_dataset()->map_file_id(file_id);
    if(handle == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p close file request: %s.",
                      vss_get_device_id(req), handle, handle->component.c_str());

    rc = handle->get_dataset()->close_file(handle, object, origin);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%p close file result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_VERSIONR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_CLOSE_FILE_REPLY);
        vss_set_data_length(resp, 0);
        vss_set_status(resp, rc);
        vss_set_data_length(resp, VSS_VERSIONR_SIZE);
        vss_version_reply_set_objver(resp_data, (object) ? object->get_version() : 0);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(objhandle);
    }
    return resp;
}

char* vss_server::handle_negotiate(vss_req_proc_ctx* context)
{
    VSSI_Result rc = VSSI_SUCCESS;
    char* resp = NULL;

    // Select security parameters that are at least what the client requested
    // and what the server requires for the client.
    // This type of server allows for non-secure communication.
    u8 signing_mode = VSS_NEGOTIATE_SIGNING_MODE_NONE;
    u8 sign_type = VSS_NEGOTIATE_SIGN_TYPE_NONE;
    u8 encrypt_type = VSS_NEGOTIATE_ENCRYPT_TYPE_NONE;
    u32 start_xid = 0;
    string challenge;

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Negotiating parameters...");

    if(vss_get_version(context->header) < 2) {
        // This command not supported until protocol version 2.
        rc = VSSI_BADVER;
        goto respond;
    }

    if(signing_mode < vss_negotiate_get_signing_mode(context->body)) {
        signing_mode = vss_negotiate_get_signing_mode(context->body);
    }
    if(sign_type < vss_negotiate_get_sign_type(context->body)) {
        sign_type = vss_negotiate_get_sign_type(context->body);
    }
    if(encrypt_type < vss_negotiate_get_encrypt_type(context->body)) {
        encrypt_type = vss_negotiate_get_encrypt_type(context->body);
    }
    if(vss_negotiate_get_chal_resp_len(context->body) > 0) {
        challenge.assign(vss_negotiate_get_chal_response(context->body),
                         vss_negotiate_get_chal_resp_len(context->body));
    }

    if(context->session->test_security_settings(vss_get_device_id(context->header),
                                                signing_mode, sign_type, encrypt_type, challenge)) {
        // Negotiation successful.
        // Get starting XID for response.
        start_xid = context->session->get_next_sub_session_xid(vss_get_device_id(context->header));
    }
    else {
        // Negotiation failed. Challenge the client.
        rc = VSSI_AGAIN;
    }

 respond:
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_NEGOTIATER_BASE_SIZE + challenge.size(), 1);
    if(resp) {
        char* resp_body = resp + VSS_HEADER_SIZE;
        
        vss_set_version(resp, vss_get_version(context->header));
        vss_set_command(resp, VSS_NEGOTIATE_REPLY);
        vss_set_data_length(resp, VSS_NEGOTIATER_BASE_SIZE + challenge.size());
        vss_set_status(resp, rc);
        vss_negotiate_reply_set_xid_start(resp_body, start_xid); 
        vss_negotiate_reply_set_signing_mode(resp_body, signing_mode);
        vss_negotiate_reply_set_sign_type(resp_body, sign_type);
        vss_negotiate_reply_set_encrypt_type(resp_body, encrypt_type);
        vss_negotiate_reply_set_challenge_len(resp_body, challenge.size());
        memcpy(vss_negotiate_reply_get_challenge(resp_body), 
               challenge.data(), challenge.size());
    }

    return resp;
}

char* vss_server::handle_authenticate(vss_req_proc_ctx* context)
{
    VSSI_Result rc = VSSI_SUCCESS;
    u64 req_cluster_id = vss_authenticate_get_cluster_id(context->body);

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Authenticate cluster "FMTu64" against this server "FMTu64".",
                        req_cluster_id, clusterId);

    // Accept only this cluster's ID.
    if(req_cluster_id != clusterId) {
        rc = VSSI_INVALID;
    }

    char* resp = (char*)calloc(VSS_HEADER_SIZE, 1);
    if(resp) {
        vss_set_version(resp, vss_get_version(context->header));
        vss_set_command(resp, VSS_AUTHENTICATE_REPLY);
        vss_set_data_length(resp, 0);
        vss_set_status(resp, rc);
    }

    return resp;
}

char* vss_server::handle_set_notify(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_set_notify_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc = VSSI_SUCCESS;
    u64 mask = 0x0;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u set notify request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    mask = vss_set_notify_get_mode(data);
    object->set_notify_mask(mask, context->client, context->vssts_id);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%d set notify result:%d.",
                      vss_get_device_id(req), handle, rc);
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_GET_NOTIFYR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_SET_NOTIFY_REPLY);
        vss_set_data_length(resp, VSS_GET_NOTIFYR_SIZE);
        vss_set_status(resp, rc);
        vss_get_notify_reply_set_mode(resp_data, mask);

    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}

char* vss_server::handle_get_notify(vss_req_proc_ctx* context)
{
    const char* req = context->header;
    const char* data = context->body;
    char* resp = NULL;
    u32 handle = vss_get_notify_get_handle(data);
    vss_object* object = context->session->find_object(handle);
    VSSI_Result rc = VSSI_SUCCESS;
    u64 mask = 0;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u get notify request.",
                      vss_get_device_id(req), handle);

    if(object == NULL) {
        rc = VSSI_BADOBJ;
        goto reply;
    }
    
    mask = object->get_notify_mask(context->client, context->vssts_id);

 reply:
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      FMTu64" #%u get notify result: %d.",
                      vss_get_device_id(req), handle, rc);

    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_GET_NOTIFYR_SIZE, 1);
    if(resp) {
        char* resp_data = resp + VSS_HEADER_SIZE;
        vss_set_version(resp, vss_get_version(req));
        vss_set_command(resp, VSS_GET_NOTIFY_REPLY);
        vss_set_data_length(resp, VSS_GET_NOTIFYR_SIZE);
        vss_set_status(resp, rc);
        vss_get_notify_reply_set_mode(resp_data, mask);
    }

    if(object) {
        object->commands_processed++;
        context->session->release_object(handle);
    }

    return resp;
}
