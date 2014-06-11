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

#include "vpl_socket.h"
#include "vplex_trace.h"
#include "vplex_assert.h"

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

#include "stdlib.h"

#include "vssi.h"
#include "vssi_vss.h"
#include "vssi_error.h"
#include "vss_comm.h"

extern u64 vssi_device_id;
extern VSSI_SessionNode* user_session_list_head;
extern int vssi_max_server_connections;
extern void (*VSSI_NotifySocketChangeFn)(void);
extern VPLMutex_t vssi_mutex;

VSSI_Result VSSI_SendProxyRequest(VSSI_UserSession* session,
                                  u64 destination_device,
                                  u8 traffic_type,
                                  VPLNet_port_t p2p_port,
                                  VPLSocket_t connection)
{
    VSSI_Result rv = VSSI_SUCCESS;
    int rc;

    char header[VSS_HEADER_SIZE] = {0};
    char body[VSS_PROXY_REQUEST_SIZE] = {0};

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Sending proxy request preamble for proxy connection to cluster "FMTu64".",
                      destination_device);

    vss_set_command(header, VSS_PROXY_REQUEST);
    vss_set_version(header, 0); // Always use version 0.
    vss_set_xid(header, 0);
    vss_set_handle(header, session->handle);
    vss_set_device_id(header, vssi_device_id);
    vss_set_data_length(header, VSS_PROXY_REQUEST_SIZE);
    vss_proxy_request_set_cluster_id(body, destination_device);
    vss_proxy_request_set_port(body, p2p_port);
    vss_proxy_request_set_type(body, traffic_type);

    /// Sign the outgoing data and header
    compute_hmac(body, VSS_PROXY_REQUEST_SIZE,
                 session->secret, vss_get_data_hmac(header),
                 VSS_HMAC_SIZE);

    memset(vss_get_header_hmac(header), 0, VSS_HMAC_SIZE);
    compute_hmac(header, VSS_HEADER_SIZE,
                 session->secret,  vss_get_header_hmac(header),
                 VSS_HMAC_SIZE);

    // Send all parts of the message consecutively.
    rc = VSSI_Sendall(connection, header, VSS_HEADER_SIZE);
    if(rc != VSS_HEADER_SIZE) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unable to write proxy request header on connection "FMT_VPLSocket_t", error %d.",
                         VAL_VPLSocket_t(connection), rc);
        rv = VSSI_COMM;
        goto exit;
    }

    rc = VSSI_Sendall(connection, body, VSS_PROXY_REQUEST_SIZE);
    if(rc != VSS_PROXY_REQUEST_SIZE) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unable to write proxy request body on connection "FMT_VPLSocket_t", error %d.",
                         VAL_VPLSocket_t(connection), rc);
        rv = VSSI_COMM;
        goto exit;
    }

 exit:
    return rv;
}

void VSSI_SendOpenCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_OPEN_BASE_SIZE] = {0};

    vss_set_command(header, VSS_OPEN);
    // XID to be filled in immediately before send.
    vss_open_set_mode(request, object->mode);
    vss_open_set_reserved(request);
    vss_open_set_uid(request, object->access.vs.user_id);
    vss_open_set_did(request, object->access.vs.dataset_id);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_OPEN_BASE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendCloseCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_CLOSE_SIZE] = {0};

    vss_set_command(header, VSS_CLOSE);
    // XID to be filled in immediately before send.
    vss_close_set_handle(request, object->access.vs.proto_handle);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_CLOSE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendStartSetCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_START_SET_SIZE] = {0};

    vss_set_command(header, VSS_START_SET);
    // XID to be filled in immediately before send.
    vss_start_set_set_handle(request, object->access.vs.proto_handle);
    vss_start_set_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_START_SET_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendCommitCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_COMMIT_SIZE] = {0};

    vss_set_command(header, VSS_COMMIT);
    // XID to be filled in immediately before send.
    vss_commit_set_handle(request, object->access.vs.proto_handle);
    vss_commit_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_COMMIT_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendDeleteCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_DELETE_BASE_SIZE] = {0};

    vss_set_command(header, VSS_DELETE);
    // XID to be filled in immediately before send.
    vss_delete_set_uid(request, object->access.vs.user_id);
    vss_delete_set_did(request, object->access.vs.dataset_id);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_DELETE_BASE_SIZE,
                 NULL, 0, NULL, 0);
}

void VSSI_SendEraseCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_ERASE_SIZE] = {0};

    vss_set_command(header, VSS_ERASE);
    // XID to be filled in immediately before send.
    vss_erase_set_handle(request, object->access.vs.proto_handle);
    vss_erase_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_ERASE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendReadCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info,
                          const char* name,
                          u64 offset,
                          u32 length)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_READ_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_READ);
    // XID to be filled in immediately before send.
    vss_read_set_offset(request, offset);
    vss_read_set_length(request, length);
    vss_read_set_handle(request, object->access.vs.proto_handle);
    vss_read_set_objver(request, object->version);
    vss_read_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_READ_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendReadTrashCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               VSSI_TrashId id,
                               const char* name,
                               u64 offset,
                               u32 length)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_READ_TRASH_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_READ_TRASH);
    // XID to be filled in immediately before send.
    vss_read_trash_set_recver(request, id.version);
    vss_read_trash_set_recidx(request, id.index);
    vss_read_trash_set_offset(request, offset);
    vss_read_trash_set_length(request, length);
    vss_read_trash_set_handle(request, object->access.vs.proto_handle);
    vss_read_trash_set_objver(request, object->version);
    vss_read_trash_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_READ_TRASH_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendWriteCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info,
                           const char* name,
                           u64 offset,
                           u32 length,
                           const char* buf)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_WRITE_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_WRITE);
    // XID to be filled in immediately before send.
    vss_write_set_offset(request, offset);
    vss_write_set_length(request, length);
    vss_write_set_handle(request, object->access.vs.proto_handle);
    vss_write_set_objver(request, object->version);
    vss_write_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_WRITE_BASE_SIZE,
                 name, name_len,
                 buf, length);
}

void VSSI_SendSetTimesCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              const char* name,
                              VPLTime_t ctime,
                              VPLTime_t mtime)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_SET_TIMES_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_SET_TIMES);
    // XID to be filled in immediately before send.
    vss_set_times_set_ctime(request, ctime);
    vss_set_times_set_mtime(request, mtime);
    vss_set_times_set_handle(request, object->access.vs.proto_handle);
    vss_set_times_set_name_length(request, name_len);
    vss_set_times_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_SET_TIMES_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendSetSizeCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             const char* name,
                             u64 length)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_SET_SIZE_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_SET_SIZE);
    // XID to be filled in immediately before send.
    vss_set_size_set_size(request, length);
    vss_set_size_set_handle(request, object->access.vs.proto_handle);
    vss_set_size_set_name_length(request, name_len);
    vss_set_size_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_SET_SIZE_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendRemoveCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info,
                            const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_REMOVE_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_REMOVE);
    // XID to be filled in immediately before send.
    vss_remove_set_handle(request, object->access.vs.proto_handle);
    vss_remove_set_name_length(request, name_len);
    vss_remove_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_REMOVE_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendRenameCommand(VSSI_ObjectState* object,
                            VSSI_PendInfo* info,
                            const char* name,
                            const char* new_name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_RENAME_BASE_SIZE] = {0};
    size_t name_len = strlen(name);
    size_t new_name_len = strlen(new_name);

    vss_set_command(header, VSS_RENAME);
    // XID to be filled in immediately before send.
    vss_rename_set_handle(request, object->access.vs.proto_handle);
    vss_rename_set_name_length(request, name_len);
    vss_rename_set_new_name_length(request, new_name_len);
    vss_rename_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_RENAME_BASE_SIZE,
                 name, name_len,
                 new_name, new_name_len);
}

void VSSI_SendRename2Command(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             const char* name,
                             const char* new_name,
                             u32 flags)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_RENAME2_BASE_SIZE] = {0};
    size_t name_len = strlen(name);
    size_t new_name_len = strlen(new_name);

    vss_set_command(header, VSS_RENAME2);
    // XID to be filled in immediately before send.
    vss_rename2_set_handle(request, object->access.vs.proto_handle);
    vss_rename2_set_flags(request, flags);
    vss_rename2_set_name_length(request, name_len);
    vss_rename2_set_new_name_length(request, new_name_len);
    vss_rename2_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_RENAME2_BASE_SIZE,
                 name, name_len,
                 new_name, new_name_len);
}

void VSSI_SendCopyCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info,
                          const char* source,
                          const char* destination)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_COPY_BASE_SIZE] = {0};
    size_t source_len = strlen(source);
    size_t destination_len = strlen(destination);

    vss_set_command(header, VSS_COPY);
    // XID to be filled in immediately before send.
    vss_copy_set_handle(request, object->access.vs.proto_handle);
    vss_copy_set_source_length(request, source_len);
    vss_copy_set_destination_length(request, destination_len);
    vss_copy_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_COPY_BASE_SIZE,
                 source, source_len,
                 destination, destination_len);
}

void VSSI_SendCopyMatchCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u64 size,
                               const char* signature,
                               const char* destination)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_COPY_MATCH_BASE_SIZE] = {0};
    size_t destination_len = strlen(destination);

    vss_set_command(header, VSS_COPY_MATCH);
    // XID to be filled in immediately before send.
    vss_copy_match_set_handle(request, object->access.vs.proto_handle);
    vss_copy_match_set_objver(request, object->version);
    vss_copy_match_set_size(request, size);
    vss_copy_match_set_destination_length(request, destination_len);
    vss_copy_match_set_signature(request, signature);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_COPY_MATCH_BASE_SIZE,
                 destination, destination_len,
                 NULL, 0);
}


void VSSI_SendReadDirCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_READ_DIR_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_READ_DIR);
    // XID to be filled in immediately before send.
    vss_read_dir_set_handle(request, object->access.vs.proto_handle);
    vss_read_dir_set_objver(request, object->version);
    vss_read_dir_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_READ_DIR_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendReadDir2Command(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_READ_DIR_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_READ_DIR2);
    // XID to be filled in immediately before send.
    vss_read_dir_set_handle(request, object->access.vs.proto_handle);
    vss_read_dir_set_objver(request, object->version);
    vss_read_dir_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_READ_DIR_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendReadTrashDirCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* info,
                                  VSSI_TrashId id,
                                  const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_READ_TRASH_DIR_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_READ_TRASH_DIR);
    // XID to be filled in immediately before send.
    vss_read_trash_dir_set_handle(request, object->access.vs.proto_handle);
    vss_read_trash_dir_set_objver(request, object->version);
    vss_read_trash_dir_set_recver(request, id.version);
    vss_read_trash_dir_set_recidx(request, id.index);
    vss_read_trash_dir_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_READ_TRASH_DIR_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendStatCommand(VSSI_ObjectState* object,
                          VSSI_PendInfo* info,
                          const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_STAT_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_STAT);
    // XID to be filled in immediately before send.
    vss_stat_set_handle(request, object->access.vs.proto_handle);
    vss_stat_set_objver(request, object->version);
    vss_stat_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_STAT_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendStat2Command(VSSI_ObjectState* object,
                           VSSI_PendInfo* info,
                           const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_STAT_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_STAT2);
    // XID to be filled in immediately before send.
    vss_stat_set_handle(request, object->access.vs.proto_handle);
    vss_stat_set_objver(request, object->version);
    vss_stat_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_STAT_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}


void VSSI_SendStatTrashCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               VSSI_TrashId id,
                               const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_STAT_TRASH_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_STAT_TRASH);
    // XID to be filled in immediately before send.
    vss_stat_trash_set_handle(request, object->access.vs.proto_handle);
    vss_stat_trash_set_objver(request, object->version);
    vss_stat_trash_set_recver(request, id.version);
    vss_stat_trash_set_recidx(request, id.index);
    vss_stat_trash_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_STAT_TRASH_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendReadTrashcanCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_READ_TRASHCAN_BASE_SIZE] = {0};

    vss_set_command(header, VSS_READ_TRASHCAN);
    // XID to be filled in immediately before send.
    vss_read_trashcan_set_handle(request, object->access.vs.proto_handle);
    vss_read_trashcan_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_READ_TRASHCAN_BASE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendMakeDirCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* info,
                             const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_MAKE_DIR_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_MAKE_DIR);
    // XID to be filled in immediately before send.
    vss_make_dir_set_handle(request, object->access.vs.proto_handle);
    vss_make_dir_set_name_length(request, name_len);
    vss_make_dir_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_MAKE_DIR_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendChmodCommand(VSSI_ObjectState* object,
                           VSSI_PendInfo* info,
                           const char* name,
                           u32 attrs,
                           u32 attrs_mask)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_CHMOD_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_CHMOD);
    // XID to be filled in immediately before send.
    vss_chmod_set_handle(request, object->access.vs.proto_handle);
    vss_chmod_set_name_length(request, name_len);
    vss_chmod_set_objver(request, object->version);
    vss_chmod_set_attrs(request, attrs);
    vss_chmod_set_attrs_mask(request, attrs_mask);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_CHMOD_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendMakeDir2Command(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              const char* name,
                              u32 attrs)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_MAKE_DIR2_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_MAKE_DIR2);
    // XID to be filled in immediately before send.
    vss_make_dir2_set_handle(request, object->access.vs.proto_handle);
    vss_make_dir2_set_name_length(request, name_len);
    vss_make_dir2_set_objver(request, object->version);
    vss_make_dir2_set_attrs(request, attrs);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_MAKE_DIR2_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendSetMetadataCommand(VSSI_ObjectState* object,
                                 VSSI_PendInfo* info,
                                 const char* name,
                                 u8 type,
                                 u8 length,
                                 const char* data)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_SET_METADATA_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_SET_METADATA);
    // XID to be filled in immediately before send.
    vss_set_metadata_set_handle(request, object->access.vs.proto_handle);
    vss_set_metadata_set_objver(request, object->version);
    vss_set_metadata_set_type(request, type);
    vss_set_metadata_set_length(request, length);
    vss_set_metadata_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_SET_METADATA_BASE_SIZE,
                 name, name_len,
                 data, length);
}


void VSSI_SendEmptyTrashCommand(VSSI_ObjectState* object,
                                VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_EMPTY_TRASH_BASE_SIZE] = {0};

    vss_set_command(header, VSS_EMPTY_TRASH);
    // XID to be filled in immediately before send.
    vss_empty_trash_set_handle(request, object->access.vs.proto_handle);
    vss_empty_trash_set_objver(request, object->version);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_EMPTY_TRASH_BASE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendDeleteTrashCommand(VSSI_ObjectState* object,
                                 VSSI_PendInfo* info,
                                 VSSI_TrashId id)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_DELETE_TRASH_BASE_SIZE] = {0};

    vss_set_command(header, VSS_DELETE_TRASH);
    // XID to be filled in immediately before send.
    vss_delete_trash_set_handle(request, object->access.vs.proto_handle);
    vss_delete_trash_set_objver(request, object->version);
    vss_delete_trash_set_recver(request, id.version);
    vss_delete_trash_set_recidx(request, id.index);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_DELETE_TRASH_BASE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendRestoreTrashCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* info,
                                  VSSI_TrashId id,
                                  const char* name)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_RESTORE_TRASH_BASE_SIZE] = {0};
    const char* destination = NULL;
    char dummy_name[] = "\0";
    size_t name_len = 0;

    if(name) {
        destination = name;
        name_len = strlen(name);
        if(name_len == 0) {
            // Need a dummy name to send,
            destination = dummy_name;
            name_len = 1;
        }
    }

    vss_set_command(header, VSS_RESTORE_TRASH);
    // XID to be filled in immediately before send.
    vss_restore_trash_set_handle(request, object->access.vs.proto_handle);
    vss_restore_trash_set_objver(request, object->version);
    vss_restore_trash_set_recver(request, id.version);
    vss_restore_trash_set_recidx(request, id.index);
    vss_restore_trash_set_name_length(request, name_len);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_RESTORE_TRASH_BASE_SIZE,
                 destination, name_len,
                 NULL, 0);
}

void VSSI_SendGetSpaceCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_GET_SPACE_SIZE] = {0};

    vss_set_command(header, VSS_GET_SPACE);
    // XID to be filled in immediately before send.
    vss_get_space_set_handle(request, object->access.vs.proto_handle);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_GET_SPACE_SIZE,
                 NULL, 0, NULL, 0);
}

void VSSI_SendGetNotifyCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* cmd_info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_GET_NOTIFY_SIZE] = {0};

    vss_set_command(header, VSS_GET_NOTIFY);
    // XID to be filled in immediately before send.
    vss_get_notify_set_handle(request, object->access.vs.proto_handle);

    VSSI_SendVss(object, cmd_info,
                 header,
                 request, VSS_GET_NOTIFY_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendSetNotifyCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* cmd_info,
                               VSSI_NotifyMask mask)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_SET_NOTIFY_SIZE] = {0};

    vss_set_command(header, VSS_SET_NOTIFY);
    // XID to be filled in immediately before send.
    vss_set_notify_set_handle(request, object->access.vs.proto_handle);
    vss_set_notify_set_mode(request, mask);

    VSSI_SendVss(object, cmd_info,
                 header,
                 request, VSS_SET_NOTIFY_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendOpenFileCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* cmd_info,
                              const char* name,
                              u32 flags,
                              u32 attrs)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_OPEN_FILE_BASE_SIZE] = {0};
    size_t name_len = strlen(name);

    vss_set_command(header, VSS_OPEN_FILE);
    // XID to be filled in immediately before send.
    vss_open_file_set_origin(request, cmd_info->file_state->origin);
    vss_open_file_set_handle(request, object->access.vs.proto_handle);
    vss_open_file_set_flags(request, flags);
    vss_open_file_set_attrs(request, attrs);
    vss_open_file_set_name_length(request, name_len);

    VSSI_SendVss(object, cmd_info,
                 header,
                 request, VSS_OPEN_FILE_BASE_SIZE,
                 name, name_len,
                 NULL, 0);
}

void VSSI_SendReleaseFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_RELEASE_FILE_SIZE] = {0};

    vss_set_command(header, VSS_RELEASE_FILE);
    // XID to be filled in immediately before send.
    vss_release_file_set_handle(request, info->file_state->server_handle);
    vss_release_file_set_origin(request, info->file_state->origin);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_RELEASE_FILE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendCloseFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_CLOSE_FILE_SIZE] = {0};

    vss_set_command(header, VSS_CLOSE_FILE);
    // XID to be filled in immediately before send.
    vss_close_file_set_handle(request, info->file_state->server_handle);
    vss_close_file_set_origin(request, info->file_state->origin);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_CLOSE_FILE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendReadFileCommand(VSSI_ObjectState* object,
                              VSSI_PendInfo* info,
                              u64 offset,
                              u32 length)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_READ_FILE_SIZE] = {0};

    vss_set_command(header, VSS_READ_FILE);
    // XID to be filled in immediately before send.
    vss_read_file_set_handle(request, info->file_state->server_handle);
    vss_read_file_set_origin(request, info->file_state->origin);
    vss_read_file_set_offset(request, offset);
    vss_read_file_set_length(request, length);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_READ_FILE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendWriteFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u64 offset,
                               u32 length,
                               const char* buf)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_WRITE_FILE_BASE_SIZE] = {0};

    vss_set_command(header, VSS_WRITE_FILE);
    // XID to be filled in immediately before send.
    vss_write_file_set_handle(request, info->file_state->server_handle);
    vss_write_file_set_origin(request, info->file_state->origin);
    vss_write_file_set_offset(request, offset);
    vss_write_file_set_length(request, length);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_WRITE_FILE_BASE_SIZE,
                 buf, length,
                 NULL, 0);
}

void VSSI_SendTruncateFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u64 offset)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_TRUNCATE_FILE_SIZE] = {0};

    vss_set_command(header, VSS_TRUNCATE_FILE);
    // XID to be filled in immediately before send.
    vss_truncate_file_set_handle(request, info->file_state->server_handle);
    vss_truncate_file_set_origin(request, info->file_state->origin);
    vss_truncate_file_set_offset(request, offset);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_TRUNCATE_FILE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendChmodFileCommand(VSSI_ObjectState* object,
                               VSSI_PendInfo* info,
                               u32 attrs,
                               u32 attrs_mask)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_CHMOD_FILE_SIZE] = {0};

    vss_set_command(header, VSS_CHMOD_FILE);
    // XID to be filled in immediately before send.
    vss_chmod_file_set_handle(request, info->file_state->server_handle);
    vss_chmod_file_set_origin(request, info->file_state->origin);
    vss_chmod_file_set_attrs(request, attrs);
    vss_chmod_file_set_attrs_mask(request, attrs_mask);

    VSSI_SendVss(object, info,
                 header,
                 request, VSS_CHMOD_FILE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendSetLockCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* cmd_info,
                             u64 lock_mask)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_SET_LOCK_SIZE] = {0};

    vss_set_command(header, VSS_SET_LOCK);
    // XID to be filled in immediately before send.
    vss_set_lock_set_fhandle(request, cmd_info->file_state->server_handle);
    vss_set_lock_set_objhandle(request, object->access.vs.proto_handle);
    vss_set_lock_set_mode(request, lock_mask);

    VSSI_SendVss(object, cmd_info,
                 header,
                 request, VSS_SET_LOCK_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendGetLockCommand(VSSI_ObjectState* object,
                             VSSI_PendInfo* cmd_info)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_GET_LOCK_SIZE] = {0};

    vss_set_command(header, VSS_GET_LOCK);
    // XID to be filled in immediately before send.
    vss_get_lock_set_fhandle(request, cmd_info->file_state->server_handle);
    vss_get_lock_set_objhandle(request, object->access.vs.proto_handle);

    VSSI_SendVss(object, cmd_info,
                 header,
                 request, VSS_GET_LOCK_SIZE,
                 NULL, 0,
                 NULL, 0);
}

void VSSI_SendSetLockRangeCommand(VSSI_ObjectState* object,
                                  VSSI_PendInfo* cmd_info,
                                  VSSI_ByteRangeLock *br_lock,
                                  u32 flags)
{
    char header[VSS_HEADER_SIZE] = {0};
    char request[VSS_SET_LOCK_RANGE_SIZE] = {0};

    vss_set_command(header, VSS_SET_LOCK_RANGE);
    // XID to be filled in immediately before send.
    vss_set_lock_range_set_handle(request, cmd_info->file_state->server_handle);
    vss_set_lock_range_set_origin(request, cmd_info->file_state->origin);
    vss_set_lock_range_set_start(request, br_lock->offset);
    vss_set_lock_range_set_end(request, br_lock->offset + br_lock->length);
    vss_set_lock_range_set_mode(request, br_lock->lock_mask);
    vss_set_lock_range_set_flags(request, flags);

    VSSI_SendVss(object, cmd_info,
                 header,
                 request, VSS_SET_LOCK_RANGE_SIZE,
                 NULL, 0,
                 NULL, 0);
}

/// Handle command APIs
/// These functions handle replies to the named commands, parsing the reply
/// and returning information in the provided arguments.
/// All of them return the reply return code.

/// Verify an incoming header's signature.
/// If the header has a bad signature, the data length cannot be trusted.
VSSI_Result VSSI_VerifyHeader(char* header,
                              VSSI_UserSession* session,
                              u8 sign_mode, u8 sign_type)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(vss_get_version(header) == 0 ||
       (sign_mode != VSS_NEGOTIATE_SIGNING_MODE_NONE &&
        sign_type == VSS_NEGOTIATE_SIGN_TYPE_SHA1)) {
        char orig_hmac[VSS_HMAC_SIZE] = {0};
        char calc_hmac[VSS_HMAC_SIZE] = {0};

        // Verify header signature.
        memcpy(orig_hmac, vss_get_header_hmac(header), VSS_HMAC_SIZE);
        memset(vss_get_header_hmac(header), 0, VSS_HMAC_SIZE);

        switch(vss_get_version(header)) {
        case 0:
            compute_hmac(header, VSS_HEADER_SIZE,
                         session->secret, calc_hmac, VSS_HMAC_SIZE);
            break;
        case 1:
        case 2:
            compute_hmac(header, VSS_HEADER_SIZE,
                         session->signingKey, calc_hmac, VSS_HMAC_SIZE);
            break;
        default:
            rv = VSSI_BADVER;
            goto exit;
            break;
        }

        if(memcmp(orig_hmac, calc_hmac, VSS_HMAC_SIZE) != 0) {
            rv = VSSI_BADSIG;
        }
    }

 exit:
    return rv;
}

/// Verify an incoming command's data.
/// If the data has a bad signature, the command cannot be trusted.
VSSI_Result VSSI_VerifyData(char* command, size_t cmdlen,
                            VSSI_UserSession* session,
                            u8 sign_mode, u8 sign_type, u8 enc_type)
{
    VSSI_Result rv = VSSI_SUCCESS;
    char calc_hmac[VSS_HMAC_SIZE] = {0};

    // no data requires no hash check.
    if(cmdlen - VSS_HEADER_SIZE == 0) {
        goto exit;
    }

    // Verify data signature.
    switch(vss_get_version(command)) {
    case 0:
        compute_hmac(command + VSS_HEADER_SIZE, cmdlen - VSS_HEADER_SIZE,
                     session->secret, calc_hmac, VSS_HMAC_SIZE);
        
        if(memcmp(vss_get_data_hmac(command), calc_hmac, VSS_HMAC_SIZE) != 0) {
            rv = VSSI_BADSIG;
        }
        break;
    case 1:
    case 2:
        if(enc_type == VSS_NEGOTIATE_ENCRYPT_TYPE_AES128) {
            // Decrypt the data first. Use temp buffer.
            char* tmp = (char*)malloc(cmdlen - VSS_HEADER_SIZE);
            if(tmp == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to alloc temp buffer to decrypt reply.");
                return VSSI_NOMEM;
            }
            decrypt_data(tmp, command + VSS_HEADER_SIZE,
                         cmdlen - VSS_HEADER_SIZE, command,
                         session->encryptingKey);
            memcpy(command + VSS_HEADER_SIZE, tmp, cmdlen - VSS_HEADER_SIZE);
            free(tmp);
        }

        if(sign_mode == VSS_NEGOTIATE_SIGNING_MODE_FULL &&
           sign_type == VSS_NEGOTIATE_SIGN_TYPE_SHA1) {
            compute_hmac(command + VSS_HEADER_SIZE, cmdlen - VSS_HEADER_SIZE,
                         session->signingKey, calc_hmac, VSS_HMAC_SIZE);
            if(memcmp(vss_get_data_hmac(command), calc_hmac, VSS_HMAC_SIZE) != 0) {
                rv = VSSI_BADSIG;
            }
        }
        break;
    default:
        rv =  VSSI_BADVER;
        break;
    }

 exit:
    return rv;
}

/// Error - Just return the command result.
VSSI_Result VSSI_HandleErrorReply(char* reply)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
    }

    return rv;
}

/// Open - Fill in object with information from reply on success.
/// @param object - Object being opened
VSSI_Result VSSI_HandleOpenReply(char* reply,
                                 VSSI_ObjectState* object)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            char* data = reply + VSS_HEADER_SIZE;
            object->new_version =
                object->version = vss_open_reply_get_objver(data);
            object->access.vs.proto_handle = vss_open_reply_get_handle(data);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Opened object "FMTu64":"FMTu64" version "FMTu64" with handle "FMTu32".",
                              object->access.vs.user_id,
                              object->access.vs.dataset_id,
                              object->version,
                              object->access.vs.proto_handle);
        }
    }

    return rv;
}

/// Close - Just return the command result.
VSSI_Result VSSI_HandleCloseReply(char* reply,
                                  VSSI_ObjectState* object)
{
    VSSI_Result rv = VSSI_SUCCESS;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Closed object "FMTu64":"FMTu64" version "FMTu64" with handle "FMTu32".",
                      object->access.vs.user_id,
                      object->access.vs.dataset_id,
                      object->version,
                      object->access.vs.proto_handle);

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
    }

    return rv;
}

/// Read - Fill in buffer, length read, object version on success.
/// @param [in] object - Object pointer
/// @param [out] length - Length of read performed on success
VSSI_Result VSSI_HandleReadReply(char* reply,
                                 VSSI_ObjectState* object,
                                 u32* length,
                                 char* buf)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;
        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            *length = vss_read_reply_get_length(data);
            memcpy(buf, vss_read_reply_get_data(data), *length);
        }
        object->new_version = vss_read_reply_get_objver(data);
    }

    return rv;
}

/// Write - Fill in length read, object version on success.
/// @param [in] object - Object pointer
/// @param [out] length - Length of read performed on success
VSSI_Result VSSI_HandleWriteReply(char* reply,
                                  VSSI_ObjectState* object,
                                  u32* length)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;
        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            *length = vss_write_reply_get_length(data);
        }
        object->new_version = vss_write_reply_get_objver(data);
        //VPLTRACE_LOG_INFO(TRACE_BVS, 0, "new_version "FMTu64".", object->new_version);
    }

    return rv;
}

VSSI_Result VSSI_HandleVersionReply(char* reply,
                                    VSSI_ObjectState* object)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;
        rv = vss_get_status(reply);
        object->new_version = vss_version_reply_get_objver(data);
        if(rv == VSSI_SUCCESS) {
            object->version = object->new_version;
        }
    }

    return rv;
}

/// Delete - Just return the command result.
VSSI_Result VSSI_HandleDeleteReply(char* reply)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
    }

    return rv;
}

VSSI_Result VSSI_HandleReadDirReply(char* reply,
                                    VSSI_ObjectState* object,
                                    VSSI_Dir* directory,
                                    int version)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;

        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            VSSI_DirectoryState* dir = malloc(sizeof(VSSI_DirectoryState));
            if(dir == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to allocate memory for directory state.");
                rv = VSSI_NOMEM;
                goto exit;
            }

            dir->version = version;
            dir->data_len = vss_read_dir_reply_get_length(data);
            dir->raw_data = malloc(dir->data_len);
            if(dir->raw_data == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to allocate memory for directory state data.");
                rv = VSSI_NOMEM;
                free(dir);
                goto exit;
            }

            memcpy(dir->raw_data, vss_read_dir_reply_get_data(data),
                   dir->data_len);
            dir->offset = 0;
            *directory = (VSSI_Dir)(dir);
        }
        object->new_version = vss_read_dir_reply_get_objver(data);
    }

 exit:
    return rv;
}

VSSI_Result VSSI_HandleStatReply(char* reply,
                                 VSSI_ObjectState* object,
                                 VSSI_Dirent** stats)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;

        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            // Allocate new VSSI_Dirent and metadata space.
            // Layout: VSSI_Dirent | VSSI_DirentMetadataState | name | metadata
            VSSI_DirentMetadataState* metadata;
            char* entryData = vss_read_dir_reply_get_data(data);
            *stats = (VSSI_Dirent*)malloc(sizeof(VSSI_Dirent) +
                                          VSS_DIRENT_SIGNATURE_SIZE +
                                          sizeof(VSSI_DirentMetadataState) +
                                          vss_dirent_get_name_len(entryData) +
                                          vss_dirent_get_meta_size(entryData));
            if(*stats == NULL) {
                rv = VSSI_NOMEM;
                goto exit;
            }
            (*stats)->signature = (const char*)(*stats + 1);
            metadata = (VSSI_DirentMetadataState*)((*stats)->signature + VSS_DIRENT_SIGNATURE_SIZE);
            (*stats)->name = (const char*)(metadata + 1);
            metadata->data = (const char*)((*stats)->name + vss_dirent_get_name_len(entryData));

            (*stats)->size = vss_dirent_get_size(entryData);
            (*stats)->ctime = vss_dirent_get_ctime(entryData);
            (*stats)->mtime = vss_dirent_get_mtime(entryData);
            (*stats)->changeVer = vss_dirent_get_change_ver(entryData);
            (*stats)->isDir = vss_dirent_get_is_dir(entryData);
            memcpy((char*)((*stats)->signature), vss_dirent_get_signature(entryData),
                   VSS_DIRENT_SIGNATURE_SIZE);
            (*stats)->metadata = metadata;
            memcpy((char*)((*stats)->name), vss_dirent_get_name(entryData),
                   vss_dirent_get_name_len(entryData));
            metadata->dataSize = vss_dirent_get_meta_size(entryData);
            metadata->curOffset = 0;
            memcpy((char*)(metadata->data), vss_dirent_get_metadata(entryData),
                   metadata->dataSize);
        }
        object->new_version = vss_read_dir_reply_get_objver(data);
    }

 exit:
    return rv;
}

VSSI_Result VSSI_HandleStat2Reply(char* reply,
                                  VSSI_ObjectState* object,
                                  VSSI_Dirent2** stats)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;

        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            // Allocate new VSSI_Dirent and metadata space.
            // Layout: VSSI_Dirent | VSSI_DirentMetadataState | name | metadata
            VSSI_DirentMetadataState* metadata;
            char* entryData = vss_read_dir_reply_get_data(data);
            *stats = (VSSI_Dirent2*)malloc(sizeof(VSSI_Dirent2) +
                                          VSS_DIRENT2_SIGNATURE_SIZE +
                                          sizeof(VSSI_DirentMetadataState) +
                                          vss_dirent2_get_name_len(entryData) +
                                          vss_dirent2_get_meta_size(entryData));
            if(*stats == NULL) {
                rv = VSSI_NOMEM;
                goto exit;
            }
            (*stats)->signature = (const char*)(*stats + 1);
            metadata = (VSSI_DirentMetadataState*)((*stats)->signature + VSS_DIRENT2_SIGNATURE_SIZE);
            (*stats)->name = (const char*)(metadata + 1);
            metadata->data = (const char*)((*stats)->name + vss_dirent2_get_name_len(entryData));

            (*stats)->size = vss_dirent2_get_size(entryData);
            (*stats)->ctime = vss_dirent2_get_ctime(entryData);
            (*stats)->mtime = vss_dirent2_get_mtime(entryData);
            (*stats)->changeVer = vss_dirent2_get_change_ver(entryData);
            (*stats)->isDir = vss_dirent2_get_is_dir(entryData);
            (*stats)->attrs = vss_dirent2_get_attrs(entryData);
            memcpy((char*)((*stats)->signature), vss_dirent2_get_signature(entryData),
                   VSS_DIRENT2_SIGNATURE_SIZE);
            (*stats)->metadata = metadata;
            memcpy((char*)((*stats)->name), vss_dirent2_get_name(entryData),
                   vss_dirent2_get_name_len(entryData));
            metadata->dataSize = vss_dirent2_get_meta_size(entryData);
            metadata->curOffset = 0;
            memcpy((char*)(metadata->data), vss_dirent2_get_metadata(entryData),
                   metadata->dataSize);
        }
        object->new_version = vss_read_dir_reply_get_objver(data);
    }

 exit:
    return rv;
}

VSSI_Result VSSI_HandleReadTrashcanReply(char* reply,
                                         VSSI_ObjectState* object,
                                         VSSI_Trashcan* trashcan)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;

        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            VSSI_TrashcanState* trash = malloc(sizeof(VSSI_TrashcanState));
            if(trash == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to allocate memory for trashcan state.");
                rv = VSSI_NOMEM;
                goto exit;
            }

            trash->data_len = vss_read_dir_reply_get_length(data);
            trash->raw_data = malloc(trash->data_len);
            if(trash->raw_data == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to allocate memory for trashcan state data.");
                rv = VSSI_NOMEM;
                free(trash);
                goto exit;
            }

            memcpy(trash->raw_data, vss_read_dir_reply_get_data(data),
                   trash->data_len);
            trash->offset = 0;
            *trashcan = (VSSI_Trashcan)(trash);
        }
        object->new_version = vss_read_dir_reply_get_objver(data);
    }

 exit:
    return rv;
}

VSSI_Result VSSI_HandleGetSpaceReply(char* reply,
                                     VSSI_ObjectState* object,
                                     u64* disk_size,
                                     u64* dataset_size,
                                     u64* avail_size)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;

        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            *disk_size = vss_get_space_reply_get_disk_size(data);
            *dataset_size = vss_get_space_reply_get_dataset_size(data);
            *avail_size = vss_get_space_reply_get_avail_size(data);
        }
    }

    return rv;
}

/// OpenFile - Fill in information from reply on success.
/// @param file - file being opened
VSSI_Result VSSI_HandleOpenFileReply(char* reply,
                                     VSSI_FileState* file_state)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_ObjectState* object;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS || rv == VSSI_EXISTS) {
            char* data = reply + VSS_HEADER_SIZE;
            object = file_state->object;
            if(object == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                  "Open File Reply file_state with null object pointer.");
                rv = VSSI_INVALID;
                goto exit;
            }
            object->new_version =
                object->version = vss_open_file_reply_get_objver(data);
            file_state->server_handle = vss_open_file_reply_get_handle(data);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Opened file in object "FMTu64":"FMTu64" version "FMTu64" returning handle %p",
                              object->access.vs.user_id,
                              object->access.vs.dataset_id,
                              object->version,
                              (void*)file_state->server_handle);
        }
    }
  exit:
    return rv;
}

VSSI_Result VSSI_HandleCloseFileReply(char* reply,
                                    VSSI_ObjectState* object,
                                    int* delete_filestate_when)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;
        rv = vss_get_status(reply);
        object->new_version = vss_version_reply_get_objver(data);
        if(rv == VSSI_SUCCESS) {
            object->version = object->new_version;
        }
        // If there is an interruption in the network connection and VSSI_CloseFile never reaches the server,
        // then VSSI_HandleLostConnection will bypass this handler and call VSSI_ResolveCommand.
        // We need to preserve the filestate in this case to allow the client to retry,
        // so we only set the filestate up for deletion on hearing back from the server.
        // There is no point in needing a retry after it gets to the server, either it succeeds or it doesn't. 
        *delete_filestate_when = DELETE_ALWAYS;
    }

    return rv;
}

VSSI_Result VSSI_HandleReadFileReply(char* reply,
                                     u32* length,
                                     char* buf)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            char* data = reply + VSS_HEADER_SIZE;
            *length = vss_read_file_reply_get_length(data);
            memcpy(buf, vss_read_file_reply_get_data(data), *length);
        }
    }
    return rv;
}

VSSI_Result VSSI_HandleWriteFileReply(char* reply,
                                      u32* length)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            char* data = reply + VSS_HEADER_SIZE;
            *length = vss_write_file_reply_get_length(data);
        }
    }
    return rv;
}

VSSI_Result VSSI_HandleTruncateFileReply(char* reply)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
    }
    return rv;
}

VSSI_Result VSSI_HandleChmodFileReply(char* reply)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
    }
    return rv;
}

VSSI_Result VSSI_HandleSetFileLockReply(char* reply)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
    }
    return rv;
}

VSSI_Result VSSI_HandleGetFileLockReply(char* reply,
                                        VSSI_FileLockState* lock_state)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_FileLockState mode = 0ULL;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;

        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            mode = vss_get_lock_reply_get_mode(data);
            if(lock_state != NULL) {
                *lock_state = mode;
            }
        }
    }

    return rv;
}

VSSI_Result VSSI_HandleSetLockRangeReply(char* reply)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        rv = vss_get_status(reply);
    }
    return rv;
}

VSSI_Result VSSI_HandleGetNotifyReply(char* reply,
                                      VSSI_NotifyMask* mask_out)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_NotifyMask mask = 0;

    if(reply == NULL) {
        rv = VSSI_INVALID;
    }
    else {
        char* data = reply + VSS_HEADER_SIZE;

        rv = vss_get_status(reply);
        if(rv == VSSI_SUCCESS) {
            mask = vss_get_notify_reply_get_mode(data);
        }
    }

    if(mask_out) {
        *mask_out = mask;
    }
    
    return rv;
}

/// Send a command using given session.
/// Command may have a request part and up to 2 data parts.
/// Fill-in the command signature and queue to send.
void VSSI_SendVss(VSSI_ObjectState* object,
                  VSSI_PendInfo* cmd_info,
                  char* header,
                  const char* fixed_data, size_t fixed_len,
                  const char* var_data1, size_t var_len1,
                  const char* var_data2, size_t var_len2)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_MsgBuffer* msg = NULL;
    VSSI_SendState* send_state = NULL;

    VPLMutex_Lock(&vssi_mutex);

    // Find the send state for object session and destination server.
    send_state = VSSI_GetSendState(object->user_session,
                                   object->access.vs.cluster_id);
    if(send_state == NULL) {
        rv = VSSI_NOMEM;
        goto reply;
    }

    // Prepare message to be sent.
    // This allocates a single buffer for the whole message with padding for possible encryption.
    msg = VSSI_PrepareMessage(header,
                              fixed_data, fixed_len,
                              var_data1, var_len1,
                              var_data2, var_len2);
    if(msg == NULL) {
        rv = VSSI_NOMEM;
        goto reply;
    }

    VPLMutex_Lock(&(send_state->send_mutex));

    // If send state has negotiated session with server successfully,
    // compose message and queue to server.
    if(send_state->negotiated == NEGOTIATE_DONE) {
        // Reserve XID for message and put cmd_info into send_state pending commands table.
        u32 xid;
        rv = VSSI_AddPendingCommand(send_state, cmd_info, &xid);
        VPLMutex_Unlock(&(send_state->send_mutex));
        if(rv != VSSI_SUCCESS) {
            goto reply;
        }

        // Put XID to message.
        vss_set_xid(msg->msg, xid);

        // Compose message, applying crypto as needed.
        msg = VSSI_ComposeMessage(send_state->session,
                                  send_state->sign_mode,
                                  send_state->sign_type,
                                  send_state->enc_type,
                                  msg);

        cmd_info = VSSI_GetVSCommandSlot(send_state, xid);
        cmd_info->msg = msg;

        // Send to the server.
        VSSI_SendToServer(send_state->server_connection, cmd_info);
    }

    // If session not negotiated, prepare message and queue to send state.
    // If negotiation not underway, start negotiation, too.
    else {
        int do_negotiate = 0;

        rv = VSSI_AddWaitingCommand(send_state, cmd_info);
        if(rv != VSSI_SUCCESS) {
            goto unlock_reply;
        }

        send_state->wait_tail->msg = msg;

        if(send_state->negotiated == NEGOTIATE_NEEDED) {
            send_state->negotiated = NEGOTIATE_PENDING;
            do_negotiate = 1;
        }

        VPLMutex_Unlock(&(send_state->send_mutex));

        if(do_negotiate) {
            // Try to negotiate connection. If not connected, this will occur on connection made.
            rv = VSSI_NegotiateSession(object, send_state, NULL, 0);
            if(rv != VSSI_SUCCESS) {
                goto reply;
            }
        }
    }

    VPLMutex_Unlock(&vssi_mutex);
    return;
 unlock_reply:
    VPLMutex_Unlock(&(send_state->send_mutex));
 reply:
    VSSI_ResolveCommand(cmd_info, rv);
    VPLMutex_Unlock(&vssi_mutex);
}

void VSSI_SendToServer(VSSI_VSConnectionNode* server_connection,
                       VSSI_PendInfo* cmd_info)
{
    VPLMutex_Lock(&(server_connection->send_mutex));

    VSSI_SendToServerUnlocked(server_connection, cmd_info);

    VPLMutex_Unlock(&(server_connection->send_mutex));
}

void VSSI_SendToServerUnlocked(VSSI_VSConnectionNode* server_connection,
                               VSSI_PendInfo* cmd_info)
{
    VSSI_VSConnection* connection;
    int num_connections = 0;
    int busy_connections = 0;


    // Put command message to first connection not already sending.
    for(connection = server_connection->connections; connection != NULL; connection = connection->next) {
        num_connections++;
        if(!VPLSocket_Equal(connection->conn_id, VPLSOCKET_INVALID) &&
           connection->send_msg == NULL &&
           cmd_info != NULL) {
            cmd_info->connection = connection;
            connection->send_msg = cmd_info->msg;
            cmd_info->msg = NULL;
            cmd_info = NULL;
            connection->enable_send = 1;
            connection->cmds_outstanding++;
            VSSI_NotifySocketChangeFn();
        }
        if(connection->cmds_outstanding > 0) {
            busy_connections++;
        }
    }

    if(cmd_info != NULL) {
        // Add message to send queue.
        if(server_connection->send_tail) {
            server_connection->send_tail->next = cmd_info;
        }
        else {
            server_connection->send_head = cmd_info;
        }
        server_connection->send_tail = cmd_info;
        cmd_info->next = NULL;
    }

    // Increase connections if all are busy and not at max connections.
    if(num_connections < vssi_max_server_connections &&
       busy_connections == num_connections) {
        // TODO: Make another connection.
    }
}

VSSI_Result VSSI_NegotiateSession(VSSI_ObjectState* object,
                                  VSSI_SendState* send_state,
                                  const char* challenge, u8 len)
{
    VSSI_Result rv = VSSI_SUCCESS;

    // Find server connection for negotiation.
    VSSI_VSConnectionNode* connection;

    // Associate send state with server connection. (Reference taken.)
    // If already associated, skip.
    if(send_state->server_connection == NULL) {
        connection = VSSI_GetServerConnection(object->access.vs.cluster_id);
        if(connection == NULL) {
            rv = VSSI_NOMEM;
            goto done;
        }
        send_state->server_connection = connection;
    }
    else {
        connection = send_state->server_connection;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Negotiation needed for session "FMTu64" to cluster "FMTu64" on connection type %d.",
                      object->user_session->handle,
                      object->access.vs.cluster_id,
                      connection->type);

    // Take connection lock, then send state lock.
    VPLMutex_Lock(&(connection->send_mutex));
    VPLMutex_Lock(&(send_state->send_mutex));

    if(connection->connect_state == CONNECTION_ACTIVE) {
        // Connection established: Send negotiate request.
        char header[VSS_HEADER_SIZE] = {0};
        char request[VSS_NEGOTIATE_BASE_SIZE] = {0};
        VSSI_PendInfo cmd_info;
        VSSI_PendInfo* cmd_info_p;
        VSSI_MsgBuffer* msg = NULL;
        u32 xid;

        cmd_info.command = VSS_NEGOTIATE;
        cmd_info.delete_obj_when = DELETE_NEVER;
        cmd_info.object = NULL;
        cmd_info.callback = NULL;
        cmd_info.ctx = NULL;
        rv = VSSI_AddPendingCommand(send_state, &cmd_info, &xid);
        VPLMutex_Unlock(&(send_state->send_mutex));

        vss_set_command(header, VSS_NEGOTIATE);
        vss_set_xid(header, xid);
        switch(connection->type) {
        case VSSI_ROUTE_DIRECT_INTERNAL:
            if(!VPLNet_IsRoutableAddress(connection->inaddr.addr)) {
                vss_negotiate_set_signing_mode(request, VSS_NEGOTIATE_SIGNING_MODE_HEADER_ONLY);
                vss_negotiate_set_sign_type(request, VSS_NEGOTIATE_SIGN_TYPE_SHA1);
                vss_negotiate_set_encrypt_type(request, VSS_NEGOTIATE_ENCRYPT_TYPE_NONE);
                break;
            }
            // else fall through for not-really-internal direct case.
        default: // full security otherwise
            vss_negotiate_set_signing_mode(request, VSS_NEGOTIATE_SIGNING_MODE_FULL);
            vss_negotiate_set_sign_type(request, VSS_NEGOTIATE_SIGN_TYPE_SHA1);
            vss_negotiate_set_encrypt_type(request, VSS_NEGOTIATE_ENCRYPT_TYPE_AES128);
            break;
        }
        vss_negotiate_set_chal_resp_len(request, len);

        msg = VSSI_PrepareMessage(header, request, VSS_NEGOTIATE_BASE_SIZE,
                                  challenge, len, NULL, 0);
        if(msg == NULL) {
            rv = VSSI_NOMEM;
            goto exit_send_unlocked;
        }
        msg = VSSI_ComposeMessage(send_state->session,
                                  VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                  VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                                  VSS_NEGOTIATE_ENCRYPT_TYPE_AES128,
                                  msg);
        if(msg == NULL) {
            rv = VSSI_NOMEM;
            goto exit_send_unlocked;
        }

        cmd_info_p = VSSI_GetVSCommandSlot(send_state, xid);
        cmd_info_p->msg = msg;

        VSSI_SendToServerUnlocked(send_state->server_connection, cmd_info_p);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Sent negotiation request for session "FMTu64" to cluster "FMTu64" xid %u.",
                          object->user_session->handle, object->access.vs.cluster_id, xid);
    }
    else if(connection->connect_state == CONNECTION_DISCONNECTED ||
            connection->connect_state == CONNECTION_FAILED) {
        // No connection: connect to server. (should have no negotiation challenge)

        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Connecting to server for session "FMTu64" to cluster "FMTu64".",
                          object->user_session->handle,
                          object->access.vs.cluster_id);

        VPLMutex_Unlock(&(send_state->send_mutex));

        connection->connect_state = CONNECTION_CONNECTING;
        connection->route_id = 0;
        connection->object = object;

        // Use user session of object for making/authenticating connection.
        VSSI_ConnectVSServer(connection, VSSI_SUCCESS);
    }
    else {
        VPLMutex_Unlock(&(send_state->send_mutex));
    }

    // Negotiate request sent or will be sent once connection established.
 exit_send_unlocked:
    VPLMutex_Unlock(&(connection->send_mutex));
 done:
    return rv;
}

void VSSI_NegotiateSessionDone(VSSI_UserSession* session,
                               VSSI_SendState* send_state,
                               VSSI_VSConnectionNode* server_connection,
                               const char* msg)
{
    const char* body = msg + VSS_HEADER_SIZE;
    int rc = vss_get_status(msg);
    int slot;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Negotiation for session "FMTu64" cluster "FMTu64" result: %d.",
                      session->handle, server_connection->cluster_id, rc);

    // Release the pend info for negotiate command.
    VPLMutex_Lock(&(send_state->send_mutex));
    slot = VSSI_GetCommandSlotUnlocked(send_state, vss_get_xid(msg));
    if(slot >= 0) {
        VSSI_PendInfo* cmd_info = send_state->pending_cmds[slot];
        send_state->pending_cmds[slot] = NULL;
        send_state->num_cmd_slots_active--;
        memset(cmd_info, 0, sizeof(VSSI_PendInfo));
        cmd_info->next_free = send_state->free_stack;
        send_state->free_stack = cmd_info;
    }
    VPLMutex_Unlock(&(send_state->send_mutex));

    switch(rc) {
    case VSSI_SUCCESS:
        // Negotiation successful. Take parameters.
        VPLMutex_Lock(&(send_state->send_mutex));
        send_state->negotiated = NEGOTIATE_DONE;
        send_state->sign_mode = vss_negotiate_reply_get_signing_mode(body);
        send_state->sign_type = vss_negotiate_reply_get_sign_type(body);
        send_state->enc_type = vss_negotiate_reply_get_encrypt_type(body);
        send_state->seq_id = vss_negotiate_reply_get_xid_start(body);

        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Negotiation for session "FMTu64" cluster "FMTu64" signMode:%u signType:%u encType:%u xid:%u.",
                          session->handle, server_connection->cluster_id,
                          send_state->sign_mode, send_state->sign_type, send_state->enc_type, send_state->seq_id);

        // Compose and send all waiting messages.
        while(send_state->wait_head != NULL) {
            VSSI_PendInfo* cmd_info = send_state->wait_head;
            VSSI_MsgBuffer* msg = cmd_info->msg;
            u32 xid;
            send_state->wait_head = cmd_info->next;
            cmd_info->msg = NULL;

            VSSI_WaitingCommandToPending(send_state, cmd_info, &xid);
            vss_set_xid(msg->msg, xid);
            msg = VSSI_ComposeMessage(send_state->session,
                                      send_state->sign_mode,
                                      send_state->sign_type,
                                      send_state->enc_type,
                                      msg);

            cmd_info = VSSI_GetVSCommandSlot(send_state, xid);
            cmd_info->msg = msg;

            // Send to the server.
            VSSI_SendToServer(send_state->server_connection, cmd_info);
        }
        send_state->wait_tail = NULL;
        VPLMutex_Unlock(&(send_state->send_mutex));
        break;

    case VSSI_AGAIN:
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Negotiation for session "FMTu64" cluster "FMTu64" challenged.",
                          session->handle, server_connection->cluster_id);

        // Repeat negotiation with challenge. Use first object of session.
        rc = VSSI_NegotiateSession(session->objects, send_state,
                                   vss_negotiate_reply_get_challenge(body),
                                   vss_negotiate_reply_get_challenge_len(body));
        if(rc == VSSI_SUCCESS) {
            break;
        }
        else {
            // Treat as error. Fall-thru.
        }
    default:
        // Fatal error. Give up.
        VSSI_ReplyPendingCommands(send_state, NULL, rc);
        VSSI_DestroySendState(send_state);
        break;
    }
}

VSSI_MsgBuffer* VSSI_PrepareMessage(char* header,
                                    const char* fixed_data, size_t fixed_len,
                                    const char* var_data1, size_t var_len1,
                                    const char* var_data2, size_t var_len2)
{
    VSSI_MsgBuffer* rv = NULL;
    char* body_buf = NULL;
    size_t body_len = fixed_len + var_len1 + var_len2;
    size_t pad_len = ((CSL_AES_BLOCKSIZE_BYTES -
                       (body_len % CSL_AES_BLOCKSIZE_BYTES)) %
                      CSL_AES_BLOCKSIZE_BYTES);

    // Pack-up the message for send (adding space for encryption padding).
    rv = (VSSI_MsgBuffer*)calloc(sizeof(VSSI_MsgBuffer) +
                                  VSS_HEADER_SIZE + body_len + pad_len, 1);
    if(rv == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unable to allocate outgoing message.");
        goto fail;
    }

    // Fill header device ID
    vss_set_device_id(header, vssi_device_id);

    // Set version to format in-use
    vss_set_version(header, USE_VSSI_VERSION);

    // Set body length (ignore pad for now)
    vss_set_data_length(header, body_len);

    // Header into message buffer.
    memcpy((char*)(rv + 1), header, VSS_HEADER_SIZE);

    // Set message length to un-padded length.
    rv->length = VSS_HEADER_SIZE + body_len;

    // body into message buffer.
    body_buf = (char*)(rv + 1) + VSS_HEADER_SIZE;
    if(fixed_data != NULL && fixed_len > 0) {
        memcpy(body_buf, fixed_data, fixed_len);
        body_buf += fixed_len;
    }
    if(var_data1 != NULL && var_len1 > 0) {
        memcpy(body_buf, var_data1, var_len1);
        body_buf += var_len1;
    }
    if(var_data2 != NULL && var_len2 > 0) {
        memcpy(body_buf, var_data2, var_len2);
        body_buf += var_len2;
    }

    // Fill allocated pad space with random data -- need not be secure random,
    // message will be encrypted afterwards.
    while(pad_len > 0) {
        // Random number generator good for 16 bits of randomness.
        static int randomness = 99999; // seed
        randomness = randomness * 1103515245 + 12345;
        body_buf[0] = randomness & 0xff;
        body_buf++;
        pad_len--;
        if(pad_len > 0) {
            body_buf[0] = (randomness >> 8) & 0xff;
            pad_len--;
            body_buf++;
        }
    }

 fail:
    return rv;
}

VSSI_MsgBuffer* VSSI_ComposeMessage(VSSI_UserSession* session,
                                    u8 sign_mode, u8 sign_type, u8 enc_type,
                                    VSSI_MsgBuffer* msg)
{
    VSSI_MsgBuffer* rv = msg;
    char* body_buf = msg->msg + VSS_HEADER_SIZE;
    size_t body_len = vss_get_data_length(msg->msg);
    size_t pad_len = ((CSL_AES_BLOCKSIZE_BYTES -
                       (body_len % CSL_AES_BLOCKSIZE_BYTES)) %
                      CSL_AES_BLOCKSIZE_BYTES);

    // Set session handle
    vss_set_handle(msg->msg, session->handle);

    if(enc_type == VSS_NEGOTIATE_ENCRYPT_TYPE_AES128) {
        body_len += pad_len;
        // padding already randomized
        vss_set_data_length(msg->msg, body_len);
    }

    // Compute signatures as needed.

    // Body signature
    if(USE_VSSI_VERSION == 0) {
        compute_hmac(body_buf, body_len,
                     session->secret,
                     vss_get_data_hmac(msg->msg), VSS_HMAC_SIZE);
    }
    else {
        if(sign_mode == VSS_NEGOTIATE_SIGNING_MODE_FULL &&
           sign_type == VSS_NEGOTIATE_SIGN_TYPE_SHA1) {
            compute_hmac(body_buf, body_len,
                         session->signingKey,
                         vss_get_data_hmac(msg->msg), VSS_HMAC_SIZE);
        }
    }

    // Header signature
    if(USE_VSSI_VERSION == 0) {
        compute_hmac(msg->msg, VSS_HEADER_SIZE,
                     session->secret,
                     vss_get_header_hmac(msg->msg), VSS_HMAC_SIZE);
    }
    else {
        if(sign_mode != VSS_NEGOTIATE_SIGNING_MODE_NONE &&
           sign_type == VSS_NEGOTIATE_SIGN_TYPE_SHA1) {
            compute_hmac(msg->msg, VSS_HEADER_SIZE,
                         session->signingKey,
                         vss_get_header_hmac(msg->msg), VSS_HMAC_SIZE);
        }
    }

    // Do encryption if needed
    if(USE_VSSI_VERSION == 1 ||
       (USE_VSSI_VERSION == 2 &&
        enc_type == VSS_NEGOTIATE_ENCRYPT_TYPE_AES128)) {
        // Allocate a new message to receive the encrypted, signed message.
        rv = (VSSI_MsgBuffer*)calloc(sizeof(VSSI_MsgBuffer) +
                                     VSS_HEADER_SIZE + body_len, 1);
        if(rv == NULL) {
            goto exit;
        }

        // Copy header
        memcpy((char*)(rv + 1), msg->msg, VSS_HEADER_SIZE);
        rv->length = VSS_HEADER_SIZE + body_len;

        // Encrypt body (cryptotext to new buffer)
        encrypt_data((char*)(rv + 1) + VSS_HEADER_SIZE, body_buf, body_len,
                     (char*)(rv + 1), session->encryptingKey);

    exit:
        // Delete original buffer
        free(msg);
    }

    return rv;
}

void compute_hmac(const char* buf, size_t len,
                  const void* key, char* hmac, int hmac_size)
{
    CSL_HmacContext context;
    char calc_hmac[CSL_SHA1_DIGESTSIZE] = {0};

    if(len <= 0) {
        // No signature for empty data. Return 0.
        memset(hmac, 0, hmac_size);
        return;
    }

    CSL_ResetHmac(&context, key);
    CSL_InputHmac(&context, buf, len);
    CSL_ResultHmac(&context, (u8*)calc_hmac);

    if(hmac_size > CSL_SHA1_DIGESTSIZE) {
        hmac_size = CSL_SHA1_DIGESTSIZE;
    }
    memcpy(hmac, calc_hmac, hmac_size);
}

void encrypt_data(char* dest, const char* src, size_t len,
                  const char* iv_seed, const char* key)
{
    int rc;

    char iv[CSL_AES_IVSIZE_BYTES];
    memcpy(iv, iv_seed, sizeof(iv));

    rc = aes_SwEncrypt((u8*)key, (u8*)iv, (u8*)src, len, (u8*)dest);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to encrypt data.");
    }
}

void decrypt_data(char* dest, const char* src, size_t len,
                  const char* iv_seed, const char* key)
{
    int rc;

    char iv[CSL_AES_IVSIZE_BYTES];
    memcpy(iv, iv_seed, sizeof(iv));

    rc = aes_SwDecrypt((u8*)key, (u8*)iv, (u8*)src, len, (u8*)dest);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to decrypt data.");
    }
}
