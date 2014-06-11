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

/**
 * vss_comm.h
 *
 * Definitions of Virtual Storage Service wire protocol.
 * See [[VSS Protocol]] on the wiki.
 */

#ifndef __VSS_COMM_H__
#define __VSS_COMM_H__

#include "vpl_plat.h"
#include "vplu_types.h"
#include "vpl_conv.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__CLOUDNODE__)
static inline void vss_set_u64(u64 src, char *dst)
{
    src = VPLConv_hton_u64(src);
    memcpy(dst, &src, sizeof(u64)); 
}

static inline u64 vss_get_u64(const char *src)
{
    u64 dst;
    memcpy(&dst, src, sizeof(u64));
    return VPLConv_ntoh_u64(dst);
}
#endif

/// Commands and Replies

enum {
    // Reply code
    VSS_REPLY = 0x80,

    /// Commands
    VSS_NOOP      = 0x00,
    VSS_OPEN      = 0x01,
    VSS_CLOSE     = 0x02,
    VSS_START_SET = 0x03,
    VSS_COMMIT    = 0x04,
    VSS_DELETE    = 0x05,
    VSS_ERASE     = 0x06,
    VSS_READ      = 0x10,
    VSS_READ_DIFF = 0x11,
    VSS_READ_DIR  = 0x12,
    VSS_STAT      = 0x13,
    VSS_READ_TRASHCAN = 0x14,
    VSS_READ_TRASH = 0x15,
    VSS_READ_TRASH_DIR = 0x16,
    VSS_STAT_TRASH = 0x17,
    VSS_WRITE     = 0x20,
    VSS_MAKE_REF  = 0x21,
    VSS_MAKE_DIR  = 0x22,
    VSS_REMOVE    = 0x23,
    VSS_RENAME    = 0x24,
    VSS_COPY      = 0x25,
    VSS_COPY_MATCH= 0x26,
    VSS_SET_TIMES = 0x32,
    VSS_SET_SIZE  = 0x33,
    VSS_SET_METADATA = 0x34,
    VSS_EMPTY_TRASH = 0x40,
    VSS_DELETE_TRASH = 0x41,
    VSS_RESTORE_TRASH = 0x42,
    VSS_GET_SPACE = 0x50,
    VSS_RELEASE_FILE = 0x51,
    VSS_SET_LOCK  = 0x52,
    VSS_GET_LOCK  = 0x53,
    VSS_SET_LOCK_RANGE  = 0x54,
    VSS_GET_NOTIFY = 0x55,
    VSS_SET_NOTIFY = 0x56,
    VSS_NOTIFICATION = 0x57,
    VSS_OPEN_FILE  = 0x58,
    VSS_CLOSE_FILE = 0x59,
    VSS_READ_FILE  = 0x5a,
    VSS_WRITE_FILE = 0x5b,
    VSS_TRUNCATE_FILE = 0x5c,
    VSS_STAT_FILE  = 0x5d,
    VSS_CHMOD      = 0x5e,
    VSS_MAKE_DIR2  = 0x5f,
    VSS_READ_DIR2  = 0x60,
    VSS_STAT2      = 0x61,
    VSS_CHMOD_FILE = 0x62,
    VSS_RENAME2    = 0x63,

    /// Special commands
    VSS_NEGOTIATE     = 0x7A,
    VSS_TUNNEL_RESET  = 0x7B,
    VSS_AUTHENTICATE  = 0x7C,
    VSS_TUNNEL_DATA   = 0x7D,
    VSS_PROXY_REQUEST = 0x7E, 
    VSS_PROXY_CONNECT = 0x7F, 

    /// Replies
    VSS_ERROR           = VSS_NOOP + VSS_REPLY,
    VSS_OPEN_REPLY      = VSS_OPEN + VSS_REPLY,
    VSS_CLOSE_REPLY     = VSS_CLOSE + VSS_REPLY,
    VSS_START_SET_REPLY = VSS_START_SET + VSS_REPLY,
    VSS_COMMIT_REPLY    = VSS_COMMIT + VSS_REPLY,
    VSS_DELETE_REPLY    = VSS_DELETE + VSS_REPLY,
    VSS_ERASE_REPLY     = VSS_ERASE + VSS_REPLY,
    VSS_READ_REPLY      = VSS_READ + VSS_REPLY,
    VSS_READ_DIFF_REPLY = VSS_READ_DIFF + VSS_REPLY,
    VSS_READ_DIR_REPLY  = VSS_READ_DIR + VSS_REPLY,
    VSS_STAT_REPLY      = VSS_STAT + VSS_REPLY,
    VSS_READ_TRASHCAN_REPLY = VSS_READ_TRASHCAN + VSS_REPLY,
    VSS_READ_TRASH_REPLY = VSS_READ_TRASH + VSS_REPLY,
    VSS_READ_TRASH_DIR_REPLY = VSS_READ_TRASH_DIR + VSS_REPLY,
    VSS_STAT_TRASH_REPLY = VSS_STAT_TRASH + VSS_REPLY,
    VSS_WRITE_REPLY     = VSS_WRITE + VSS_REPLY,
    VSS_MAKE_REF_REPLY  = VSS_MAKE_REF + VSS_REPLY,
    VSS_MAKE_DIR_REPLY  = VSS_MAKE_DIR + VSS_REPLY,
    VSS_REMOVE_REPLY    = VSS_REMOVE + VSS_REPLY,
    VSS_RENAME_REPLY    = VSS_RENAME + VSS_REPLY,
    VSS_COPY_REPLY    = VSS_COPY + VSS_REPLY,
    VSS_COPY_MATCH_REPLY= VSS_COPY_MATCH + VSS_REPLY,
    VSS_SET_TIMES_REPLY  = VSS_SET_TIMES + VSS_REPLY,
    VSS_SET_SIZE_REPLY  = VSS_SET_SIZE + VSS_REPLY,
    VSS_SET_METADATA_REPLY = VSS_SET_METADATA + VSS_REPLY,
    VSS_EMPTY_TRASH_REPLY = VSS_EMPTY_TRASH + VSS_REPLY,
    VSS_DELETE_TRASH_REPLY = VSS_DELETE_TRASH + VSS_REPLY,
    VSS_RESTORE_TRASH_REPLY = VSS_RESTORE_TRASH + VSS_REPLY, 
    VSS_GET_SPACE_REPLY  = VSS_GET_SPACE + VSS_REPLY,
    VSS_RELEASE_FILE_REPLY = VSS_RELEASE_FILE + VSS_REPLY,
    VSS_SET_LOCK_REPLY  = VSS_SET_LOCK + VSS_REPLY,
    VSS_GET_LOCK_REPLY  = VSS_GET_LOCK + VSS_REPLY,
    VSS_SET_LOCK_RANGE_REPLY  = VSS_SET_LOCK_RANGE + VSS_REPLY,
    VSS_GET_NOTIFY_REPLY  = VSS_GET_NOTIFY + VSS_REPLY,
    VSS_SET_NOTIFY_REPLY  = VSS_SET_NOTIFY + VSS_REPLY,
    VSS_NOTIFICATION_REPLY  = VSS_NOTIFICATION + VSS_REPLY, // unused
    VSS_OPEN_FILE_REPLY   = VSS_OPEN_FILE + VSS_REPLY,
    VSS_CLOSE_FILE_REPLY  = VSS_CLOSE_FILE + VSS_REPLY,
    VSS_READ_FILE_REPLY   = VSS_READ_FILE + VSS_REPLY,
    VSS_WRITE_FILE_REPLY  = VSS_WRITE_FILE + VSS_REPLY,
    VSS_TRUNCATE_FILE_REPLY = VSS_TRUNCATE_FILE + VSS_REPLY,
    VSS_STAT_FILE_REPLY    = VSS_STAT_FILE + VSS_REPLY,
    VSS_CHMOD_REPLY = VSS_CHMOD + VSS_REPLY,
    VSS_MAKE_DIR2_REPLY  = VSS_MAKE_DIR2 + VSS_REPLY,
    VSS_READ_DIR2_REPLY  = VSS_READ_DIR2 + VSS_REPLY,
    VSS_STAT2_REPLY      = VSS_STAT2 + VSS_REPLY,
    VSS_CHMOD_FILE_REPLY = VSS_CHMOD_FILE + VSS_REPLY,
    VSS_RENAME2_REPLY    = VSS_RENAME2 + VSS_REPLY,
    VSS_NEGOTIATE_REPLY    = VSS_NEGOTIATE + VSS_REPLY,
    VSS_TUNNEL_RESET_REPLY = VSS_TUNNEL_RESET + VSS_REPLY,
    VSS_AUTHENTICATE_REPLY = VSS_AUTHENTICATE + VSS_REPLY,
    VSS_TUNNEL_DATA_REPLY = VSS_TUNNEL_DATA + VSS_REPLY, // unused
    VSS_PROXY_REQUEST_REPLY = VSS_PROXY_REQUEST + VSS_REPLY,
    VSS_PROXY_CONNECT_REPLY = VSS_PROXY_CONNECT + VSS_REPLY,
};

///  Standard VSS header
///     0                8                16               24            31
///    +----------------+----------------+----------------+----------------+
///  0 | VERSION        | COMMAND        | STATUS                          |
///    +----------------+----------------+----------------+----------------+
///  4 | XID                                                               |
///    +----------------+----------------+----------------+----------------+
///  8 | DEVICE ID                                                         |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | SECRET                                                            |
/// 20 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
/// 24 | DATA LENGTH                                                       |
///    +----------------+----------------+----------------+----------------+
/// 28 | DATA HMAC                                                         |
/// 32 |                                                                   |
///    +                                 +----------------+----------------+
/// 36 |                                 |                                 |
///    +----------------+----------------+                                 +
/// 40 | HEADER HMAC                                                       |
/// 44 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 48 | FIXED DATA                                                        |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | VARIABLE DATA                                                     |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    VERSION       - Protocol version. Set to 0.
///    COMMAND       - Command/Response code. See commands and responses below.
///    STATUS        - Response status code. Set to 0 in commands
///    XID           - Request identifier set by requestor.
///                    Associate replies to commands by matching XIDs at the
///                    client.
///                    Server also uses XID to screen for repeated or injected
///                    commands.
///                    XID must be monotonically increasing modulo XID range.
///    SECRET HANDLE - Handle to a shared secret between client and server.
///    DEVICE ID     - Requesting device ID. This is echoed back on reply.
///    DATA LENGTH   - Data Length, total length of fixed and variable data,
///                    in bytes
///    DATA HMAC     - Cryptographic signature of the fixed and variable data.
///                    (10 bytes)
///    HEADER HMAC   - Cryptographic signature of the command/response header.
///                    (10 bytes)
///    FIXED DATA    - Fixed size command/response data. Size depends on the
///                    command.
///    VARIABLE DATA - Variable size command/response data. Exactly what data
///                    and what size are identified in the fixed data.

/// Offsets for each header field
enum {
    VSS_VERSION_OFFSET      =  0,
    VSS_COMMAND_OFFSET      =  1,
    VSS_STATUS_OFFSET       =  2,
    VSS_XID_OFFSET          =  4,
    VSS_DEVICE_ID_OFFSET    =  8,
    VSS_HANDLE_OFFSET       = 16,
    VSS_DATA_LENGTH_OFFSET  = 24,
    VSS_DATA_HMAC_OFFSET    = 28,
    VSS_HEADER_HMAC_OFFSET  = 38,
    VSS_HMAC_SIZE      = 10,
    VSS_HEADER_SIZE    = 48
};

static inline u8 vss_get_version(const char* buf)
{
    return buf[VSS_VERSION_OFFSET];
}

static inline void vss_set_version(char* buf, u8 val)
{
    buf[VSS_VERSION_OFFSET] = val;
}

static inline u8 vss_get_command(const char* buf)
{
    return buf[VSS_COMMAND_OFFSET];
}

static inline void vss_set_command(char* buf, u8 val)
{
    buf[VSS_COMMAND_OFFSET] = val;
}

static inline s16 vss_get_status(const char* buf)
{
    return VPLConv_ntoh_s16(*((s16*)&(buf[VSS_STATUS_OFFSET])));
}

static inline void vss_set_status(char* buf, s16 val)
{
    *((s16*)&(buf[VSS_STATUS_OFFSET])) = VPLConv_hton_s16(val);
}

static inline u32 vss_get_xid(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_XID_OFFSET])));
}

static inline void vss_set_xid(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_XID_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_get_device_id(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DEVICE_ID_OFFSET])));
}

static inline void vss_set_device_id(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_DEVICE_ID_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_get_handle(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_HANDLE_OFFSET])));
}

static inline void vss_set_handle(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_HANDLE_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_get_data_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_DATA_LENGTH_OFFSET])));
}

static inline void vss_set_data_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_DATA_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline char* vss_get_data_hmac(const char* buf)
{
    return (char*)&(buf[VSS_DATA_HMAC_OFFSET]);
}

static inline void vss_set_data_hmac(char* buf, const char* val)
{
    memcpy(&(buf[VSS_DATA_HMAC_OFFSET]), val, VSS_HMAC_SIZE);
}

static inline char* vss_get_header_hmac(const char* buf)
{
    return (char*)&(buf[VSS_HEADER_HMAC_OFFSET]);
}

static inline void vss_set_header_hmac(char* buf, const char* val)
{
    memcpy(&(buf[VSS_HEADER_HMAC_OFFSET]), val, VSS_HMAC_SIZE);
}

/// VSS_NOOP
/// No-op command (serves as a ping)
/// Use just the header. No command body.
enum {
    VSS_NOOP_SIZE = 0
};

/// VSS_ERROR
/// Error reply to any command
/// Use just the header. No reply body.
enum {
    VSS_ERROR_SIZE = 0
};

/// VSS_OPEN
/// Open a file for access.
/// When a file is opened for write access, a synchronization point is set.
/// Writes to the file are not permanent until SYNC is sent.
///    +----------------+----------------+----------------+----------------+
///  0 | MODE           | RESERVED                                         |
///    +----------------+----------------+----------------+----------------+
///  4 | RESERVED                                                          |
///    +----------------+----------------+----------------+----------------+
///  8 | UID                                                               |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | DID                                                               |
/// 20 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    MODE - File access mode.
///           READ_ONLY (0x00) - File is opened read-only.
///           READ_WRITE (0x01) - File is opened read/write.
///           FORCE (0x02) - Object writes are to be forced (ignore version).
///           LOCK (0x04) - Object is to be locked while open.
///           NO_TRASH (0x08) - Object changes must not generate trash records.
///           SECONDARY (0x10) - Access secondary dataset copy
///    RESERVED - Reserved field. Set to 0
///    UID - User ID
///    DID - Dataset ID (or title ID in save-state namespace)

enum {
    VSS_OPEN_MODE_OFFSET     = 0,
    VSS_OPEN_MODE_READONLY       = 0x00,
    VSS_OPEN_MODE_READWRITE      = 0x01,
    VSS_OPEN_MODE_FORCE          = 0x02,
    VSS_OPEN_MODE_LOCK           = 0x04,
    VSS_OPEN_MODE_NO_TRASH       = 0x08,
    VSS_OPEN_MODE_SECONDARY      = 0x10,
    VSS_OPEN_MODE_INSTANT_CHANGE = 0x20,
    VSS_OPEN_RESERVED_OFFSET = 1,
    VSS_OPEN_RESERVED_SIZE   = 7,
    VSS_OPEN_UID_OFFSET      = 8,
    VSS_OPEN_DID_OFFSET      = 16,
    VSS_OPEN_BASE_SIZE       = 24
};

static inline u8 vss_open_get_mode(const char* buf)
{
    return buf[VSS_OPEN_MODE_OFFSET];
}

static inline void vss_open_set_mode(char* buf, u8 val)
{
    buf[VSS_OPEN_MODE_OFFSET] = val;
}

static inline void vss_open_set_reserved(char* buf)
{
    memset(buf + VSS_OPEN_RESERVED_OFFSET, 0, VSS_OPEN_RESERVED_SIZE);
}

static inline u64 vss_open_get_uid(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_OPEN_UID_OFFSET])));
}

static inline void vss_open_set_uid(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_OPEN_UID_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_open_get_did(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_OPEN_DID_OFFSET])));
}

static inline void vss_open_set_did(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_OPEN_DID_OFFSET])) = VPLConv_hton_u64(val);
}

/// VSS_OPEN_REPLY
///    +----------------+----------------+---------------------------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+---------------------------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle to use for the opened object if successful.
///    OBJECT_VERSION - Current committed version of the object opened if successful.
enum {
    VSS_OPENR_HANDLE_OFFSET   = 0,
    VSS_OPENR_VERSION_OFFSET  = 4,
    VSS_OPENR_SIZE            = 12
};

static inline u32 vss_open_reply_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_OPENR_HANDLE_OFFSET])));
}

static inline void vss_open_reply_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_OPENR_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_open_reply_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_OPENR_VERSION_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_OPENR_VERSION_OFFSET])));
#endif
}

static inline void vss_open_reply_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_OPENR_VERSION_OFFSET]));
#else
    *((u64*)&(buf[VSS_OPENR_VERSION_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_CLOSE
/// Close an opened object. Any writes not committed are lost.
///    +----------------+----------------+---------------------------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+---------------------------------|
///    RESERVED - Unused. Set to 0.
///    HANDLE - Handle to previously opened object.
enum {
    VSS_CLOSE_HANDLE_OFFSET   = 0,
    VSS_CLOSE_SIZE            = 4
};

static inline u32 vss_close_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CLOSE_HANDLE_OFFSET])));
}

static inline void vss_close_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CLOSE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_CLOSE_REPLY
/// Return for CLOSE command
/// There is no fixed or variable data. STATUS indicates success or failure.
enum {
    VSS_CLOSER_SIZE            = 0
};

/// VSS_START_SET
/// Indicate start of a change set. Optionally use this before starting
/// to write changes for an object to flush any previously sent but uncommitted
/// changes.
///    +----------------+----------------+---------------------------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+---------------------------------|
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Handle to previously opened object.
///    OBJECT_VERSION - Version of object this change set modifies.
enum {
    VSS_START_SET_HANDLE_OFFSET   =  0,
    VSS_START_SET_OBJVER_OFFSET   =  4,
    VSS_START_SET_SIZE            =  12,
};

static inline u32 vss_start_set_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_START_SET_HANDLE_OFFSET])));
}

static inline void vss_start_set_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_START_SET_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_start_set_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_START_SET_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_START_SET_OBJVER_OFFSET])));
#endif
}

static inline void vss_start_set_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_START_SET_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_START_SET_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_VERSION_REPLY
///    Return the current object version only.
///    +----------------+----------------+----------------+----------------+
///  0 | OBJECT_VERSION                                                    |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    OBJECT_VERSION - Current version of object.
enum {
    VSS_VERSIONR_OBJVER_OFFSET   = 0,
    VSS_VERSIONR_SIZE = 8
};

static inline u64 vss_version_reply_get_objver(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_VERSIONR_OBJVER_OFFSET])));
}

static inline void vss_version_reply_set_objver(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_VERSIONR_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
}

/// VSS_COMMIT
/// Commit changes to an object. Call only after all relevant writes have returned.
///    +----------------+----------------+---------------------------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+---------------------------------|
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Handle to previously opened object.
///    OBJECT_VERSION - Version of object over which changes are being committed.

enum {
    VSS_COMMIT_HANDLE_OFFSET   =  0,
    VSS_COMMIT_OBJVER_OFFSET   =  4,
    VSS_COMMIT_SIZE            =  12
};

static inline u32 vss_commit_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_COMMIT_HANDLE_OFFSET])));
}

static inline void vss_commit_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_COMMIT_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_commit_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_COMMIT_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_COMMIT_OBJVER_OFFSET])));
#endif
}

static inline void vss_commit_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_COMMIT_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_COMMIT_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_COMMIT_REPLY
/// Same as VSS_VERSION_REPLY

/// VSS_DELETE
/// Delete an object permanently
///    +----------------+----------------+----------------+----------------+
///  0 | RESERVED                                                          |
///    +----------------+----------------+----------------+----------------+
///  4 | UID                                                               |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | DID                                                               |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    RESERVED - Reserved space. Set to 0.
///    UID - User ID
///    DID - Dataset ID (or title ID in save-state namespace)
enum {
    VSS_DELETE_RESERVED_OFFSET = 0,
    VSS_DELETE_RESERVED_SIZE        = 4,
    VSS_DELETE_UID_OFFSET      = 4,
    VSS_DELETE_DID_OFFSET      = 12,
    VSS_DELETE_BASE_SIZE       = 20,
};

static inline void vss_delete_set_reserved(char* buf)
{
    *((u32*)&(buf[VSS_DELETE_RESERVED_OFFSET])) = VPLConv_hton_u32(0);
}

static inline u64 vss_delete_get_uid(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DELETE_UID_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DELETE_UID_OFFSET])));
#endif
}

static inline void vss_delete_set_uid(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DELETE_UID_OFFSET]));
#else
    *((u64*)&(buf[VSS_DELETE_UID_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_delete_get_did(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DELETE_DID_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DELETE_DID_OFFSET])));
#endif
}

static inline void vss_delete_set_did(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DELETE_DID_OFFSET]));
#else
    *((u64*)&(buf[VSS_DELETE_DID_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_DELETE_REPLY
/// Return for DELETE command
/// There is no fixed or variable data. STATUS indicates success or failure.
enum {
    VSS_DELETER_SIZE = 0
};

/// VSS_ERASE
/// Erase all data for an object.
///    +----------------+----------------+---------------------------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+---------------------------------|
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Handle to previously opened object.
///    OBJECT_VERSION - Version of object over which changes are being committed.
enum {
    VSS_ERASE_HANDLE_OFFSET   = 0,
    VSS_ERASE_OBJVER_OFFSET   = 4,
    VSS_ERASE_SIZE            = 12
};

static inline u32 vss_erase_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_ERASE_HANDLE_OFFSET])));
}

static inline void vss_erase_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_ERASE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_erase_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_ERASE_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_ERASE_OBJVER_OFFSET])));
#endif
}

static inline void vss_erase_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_ERASE_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_ERASE_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_ERASE_REPLY
/// Same as VSS_VERSION_REPLY

/// VSS_READ
/// Read data from an object-file
///    +----------------+----------------+---------------------------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | OFFSET                                                            |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// 24 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    OFFSET - Byte offset into file for read request.
///    LENGTH - Length of requested data, in bytes.
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_READ_HANDLE_OFFSET   =  0,
    VSS_READ_OBJVER_OFFSET   =  4,
    VSS_READ_OFFSET_OFFSET   = 12,
    VSS_READ_LENGTH_OFFSET   = 20,
    VSS_READ_NAME_LENGTH_OFFSET = 24,
    VSS_READ_NAME_OFFSET     = 26,
    VSS_READ_BASE_SIZE       = 26
};

static inline u32 vss_read_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_HANDLE_OFFSET])));
}

static inline void vss_read_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_OBJVER_OFFSET])));
#endif
}

static inline void vss_read_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_read_get_offset(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_OFFSET_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_OFFSET_OFFSET])));
#endif
}

static inline void vss_read_set_offset(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_OFFSET_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_read_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_LENGTH_OFFSET])));
}

static inline void vss_read_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_read_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_READ_NAME_LENGTH_OFFSET])));
}

static inline void vss_read_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_READ_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_read_get_name(const char* buf)
{
    return (char*)&(buf[VSS_READ_NAME_OFFSET]);
}

static inline void vss_read_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_READ_NAME_OFFSET]), val, vss_read_get_name_length(buf));
}

/// VSS_READ_REPLY
/// Return result for read with data (on success).
///    +----------------+----------------+----------------+----------------+
///  0 | OBJECT_VERSION                                                    |
///  4 |                                                                   |
///    +----------------+----------------+---------------------------------+
///  8 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// 12 | DATA                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    LENGTH - Length of data returned.
///    OBJECT_VERSION - Version of object actually read (current committed version).
///                     If different than requested, STATUS will indicate a mismatch warning.
///    DATA   - LENGTH bytes of data read from the file starting at OFFSET.
enum {
    VSS_READR_OBJVER_OFFSET   = 0,
    VSS_READR_LENGTH_OFFSET   = 8,
    VSS_READR_DATA_OFFSET     = 12,
    VSS_READR_BASE_SIZE       = 12
};

static inline u64 vss_read_reply_get_objver(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READR_OBJVER_OFFSET])));
}

static inline void vss_read_reply_set_objver(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_READR_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_read_reply_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READR_LENGTH_OFFSET])));
}

static inline void vss_read_reply_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READR_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline char* vss_read_reply_get_data(const char* buf)
{
    return (char*)&(buf[VSS_READR_DATA_OFFSET]);
}

/// VSS_READ_TRASH
/// Read data from an trash record's object-file.
///    +----------------+----------------+---------------------------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | RECORD_VERSION                                                    |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | RECORD_INDEX                                                      |
///    +----------------+----------------+----------------+----------------+
/// 24 | OFFSET                                                            |
/// 28 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 32 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// 36 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    RECORD_VERSION - Trashcan record identifier: version component.
///    RECORD_INDEX - Trashcan record identifier: change index component.
///    OFFSET - Byte offset into file for read request.
///    LENGTH - Length of requested data, in bytes.
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_READ_TRASH_HANDLE_OFFSET   =  0,
    VSS_READ_TRASH_OBJVER_OFFSET   =  4,
    VSS_READ_TRASH_RECVER_OFFSET   = 12,
    VSS_READ_TRASH_RECIDX_OFFSET   = 20,    
    VSS_READ_TRASH_OFFSET_OFFSET   = 24,
    VSS_READ_TRASH_LENGTH_OFFSET   = 32,
    VSS_READ_TRASH_NAME_LENGTH_OFFSET = 36,
    VSS_READ_TRASH_NAME_OFFSET     = 38,
    VSS_READ_TRASH_BASE_SIZE       = 38
};

static inline u32 vss_read_trash_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_TRASH_HANDLE_OFFSET])));
}

static inline void vss_read_trash_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_TRASH_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_trash_get_recver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_TRASH_RECVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_TRASH_RECVER_OFFSET])));
#endif
}

static inline void vss_read_trash_set_recver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_TRASH_RECVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_TRASH_RECVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_read_trash_get_recidx(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_TRASH_RECIDX_OFFSET])));
}

static inline void vss_read_trash_set_recidx(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_TRASH_RECIDX_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_trash_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_TRASH_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_TRASH_OBJVER_OFFSET])));
#endif
}

static inline void vss_read_trash_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_TRASH_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_TRASH_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_read_trash_get_offset(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_TRASH_OFFSET_OFFSET])));
}

static inline void vss_read_trash_set_offset(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_READ_TRASH_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_read_trash_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_TRASH_LENGTH_OFFSET])));
}

static inline void vss_read_trash_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_TRASH_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_read_trash_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_READ_TRASH_NAME_LENGTH_OFFSET])));
}

static inline void vss_read_trash_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_READ_TRASH_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_read_trash_get_name(const char* buf)
{
    return (char*)&(buf[VSS_READ_TRASH_NAME_OFFSET]);
}

static inline void vss_read_trash_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_READ_TRASH_NAME_OFFSET]), val, vss_read_trash_get_name_length(buf));
}

/// VSS_READ_TRASH_REPLY
/// Same as VSS_READ_REPLY

/// VSS_READ_DIFF
/// Read diff data from an object-file
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | REFERENCE_ID                                                      |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | OFFSET                                                            |
/// 24 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 28 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// 32 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    REFERENCE_ID - 64-bit reference identifier to specify reference copy to use.
///    OFFSET - Byte offset into component for start of read difference request.
///    LENGTH - Length of maximum difference to return, in bytes.
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_READ_DIFF_HANDLE_OFFSET         =  0,
    VSS_READ_DIFF_OBJVER_OFFSET         =  4,
    VSS_READ_DIFF_REFERENCE_ID_OFFSET   = 12,
    VSS_READ_DIFF_OFFSET_OFFSET         = 20,
    VSS_READ_DIFF_LENGTH_OFFSET         = 28,
    VSS_READ_DIFF_NAME_LENGTH_OFFSET    = 32,
    VSS_READ_DIFF_NAME_OFFSET           = 34,
    VSS_READ_DIFF_BASE_SIZE             = 34
};

static inline u32 vss_read_diff_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_DIFF_HANDLE_OFFSET])));
}

static inline void vss_read_diff_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_DIFF_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_diff_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_DIFF_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_DIFF_OBJVER_OFFSET])));
#endif
}

static inline void vss_read_diff_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_DIFF_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_DIFF_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_read_diff_get_reference_id(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_DIFF_REFERENCE_ID_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_DIFF_REFERENCE_ID_OFFSET])));
#endif
}

static inline void vss_read_diff_set_reference_id(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_DIFF_REFERENCE_ID_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_DIFF_REFERENCE_ID_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_read_diff_get_offset(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_DIFF_OFFSET_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_DIFF_OFFSET_OFFSET])));
#endif
}

static inline void vss_read_diff_set_offset(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_DIFF_OFFSET_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_DIFF_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_read_diff_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_DIFF_LENGTH_OFFSET])));
}

static inline void vss_read_diff_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_DIFF_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_read_diff_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_READ_DIFF_NAME_LENGTH_OFFSET])));
}

static inline void vss_read_diff_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_READ_DIFF_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_read_diff_get_name(const char* buf)
{
    return (char*)&(buf[VSS_READ_DIFF_NAME_OFFSET]);
}

static inline void vss_read_diff_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_READ_DIFF_NAME_OFFSET]), val, vss_read_diff_get_name_length(buf));
}

/// VSS_READ_DIFF_REPLY
/// Return result for read difference operation with data (on success).
///    +----------------+----------------+---------------------------------+
///  0 | OBJECT_VERSION                                                    |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// 12 | OFFSET                                                            |
/// 16 |                                                                   |
///    +----------------+----------------+---------------------------------+
/// 20 | DATA                                                              |
///    +                                                                   +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    OFFSET - Byte offset into file for read request.
///    LENGTH - Length of data returned.
///    OBJECT_VERSION - Version of object actually read (current committed version).
///                     If different than requested, STATUS will indicate a mismatch warning.
///    DATA   - LENGTH bytes of data read from the file starting at OFFSET.
enum {
    VSS_READ_DIFFR_OBJVER_OFFSET   =  0,
    VSS_READ_DIFFR_LENGTH_OFFSET   =  8,
    VSS_READ_DIFFR_OFFSET_OFFSET   = 12,
    VSS_READ_DIFFR_DATA_OFFSET     = 20,
    VSS_READ_DIFFR_BASE_SIZE       = 20
};

static inline u64 vss_read_diff_reply_get_objver(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_DIFFR_OBJVER_OFFSET])));
}

static inline void vss_read_diff_reply_set_objver(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_READ_DIFFR_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_read_diff_reply_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_DIFFR_LENGTH_OFFSET])));
}

static inline void vss_read_diff_reply_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_DIFFR_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_diff_reply_get_offset(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_DIFFR_OFFSET_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_DIFFR_OFFSET_OFFSET])));
#endif
}

static inline void vss_read_diff_reply_set_offset(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_DIFFR_OFFSET_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_DIFFR_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline char* vss_read_diff_reply_get_data(const char* buf)
{
    return (char*)&(buf[VSS_READ_DIFFR_DATA_OFFSET]);
}

/// VSS_READ_DIR
/// Read the current contents of an object's directory component.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | NAME_LENGTH                     |  NAME                           |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_READ_DIR_HANDLE_OFFSET   = 0,
    VSS_READ_DIR_OBJVER_OFFSET   = 4,
    VSS_READ_DIR_NAME_LENGTH_OFFSET = 12,
    VSS_READ_DIR_NAME_OFFSET     = 14,
    VSS_READ_DIR_BASE_SIZE       = 14
};

static inline u32 vss_read_dir_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_DIR_HANDLE_OFFSET])));
}

static inline void vss_read_dir_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_DIR_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_dir_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_DIR_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_DIR_OBJVER_OFFSET])));
#endif
}

static inline void vss_read_dir_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_DIR_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_DIR_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_read_dir_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_READ_DIR_NAME_LENGTH_OFFSET])));
}

static inline void vss_read_dir_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_READ_DIR_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_read_dir_get_name(const char* buf)
{
    return (char*)&(buf[VSS_READ_DIR_NAME_OFFSET]);
}

static inline void vss_read_dir_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_READ_DIR_NAME_OFFSET]), val, vss_read_dir_get_name_length(buf));
}

/// VSS_READ_DIR_REPLY
/// Return result for read directory/stat operations with data (on success).
///    +----------------+----------------+----------------+----------------+
///  0 | OBJECT_VERSION                                                    |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// 12 | DATA                                                              |
///    +                                                                   +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    OBJECT_VERSION - Version of object actually read (current committed version).
///                     If different than requested, STATUS will indicate a mismatch warning.
///    LENGTH - Length of total data returned.
///    DATA   - Data consisting of the accumulated directory entries read, remainder of the reply. See VSS_READ_DIR_ENTRY.
enum {
    VSS_READ_DIRR_OBJVER_OFFSET   =  0,
    VSS_READ_DIRR_LENGTH_OFFSET   =  8,
    VSS_READ_DIRR_DATA_OFFSET     = 12,
    VSS_READ_DIRR_BASE_SIZE       = 12
};

static inline u64 vss_read_dir_reply_get_objver(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_DIRR_OBJVER_OFFSET])));
}

static inline void vss_read_dir_reply_set_objver(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_READ_DIRR_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_read_dir_reply_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_DIRR_LENGTH_OFFSET])));
}

static inline void vss_read_dir_reply_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_DIRR_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline char* vss_read_dir_reply_get_data(const char* buf)
{
    return (char*)&(buf[VSS_READ_DIRR_DATA_OFFSET]);
}


/// VSS_READ_DIR2
/// Same as VSS_READ_DIR
/// VSS_READ_DIR2_REPLY
/// Same as VSS_READ_DIR

/// VSS_READ_TRASH_DIR
/// Read the current contents of an object's directory component.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | RECORD_VERSION                                                    |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | RECORD_INDEX                                                      |
///    +----------------+----------------+----------------+----------------+
/// 24 | NAME_LENGTH                     |  NAME                           |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    RECORD_VERSION - Trashcan record identifier: version component.
///    RECORD_INDEX - Trashcan record identifier: change index component.
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_READ_TRASH_DIR_HANDLE_OFFSET   = 0,
    VSS_READ_TRASH_DIR_OBJVER_OFFSET   = 4,
    VSS_READ_TRASH_DIR_RECVER_OFFSET   = 12,
    VSS_READ_TRASH_DIR_RECIDX_OFFSET   = 20,
    VSS_READ_TRASH_DIR_NAME_LENGTH_OFFSET = 24,
    VSS_READ_TRASH_DIR_NAME_OFFSET     = 26,
    VSS_READ_TRASH_DIR_BASE_SIZE       = 26
};

static inline u32 vss_read_trash_dir_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_TRASH_DIR_HANDLE_OFFSET])));
}

static inline void vss_read_trash_dir_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_TRASH_DIR_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_trash_dir_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_TRASH_DIR_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_TRASH_DIR_OBJVER_OFFSET])));
#endif
}

static inline void vss_read_trash_dir_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_TRASH_DIR_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_TRASH_DIR_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_read_trash_dir_get_recver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_TRASH_DIR_RECVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_TRASH_DIR_RECVER_OFFSET])));
#endif
}

static inline void vss_read_trash_dir_set_recver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_TRASH_DIR_RECVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_TRASH_DIR_RECVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_read_trash_dir_get_recidx(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_TRASH_DIR_RECIDX_OFFSET])));
}

static inline void vss_read_trash_dir_set_recidx(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_TRASH_DIR_RECIDX_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_read_trash_dir_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_READ_TRASH_DIR_NAME_LENGTH_OFFSET])));
}

static inline void vss_read_trash_dir_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_READ_TRASH_DIR_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_read_trash_dir_get_name(const char* buf)
{
    return (char*)&(buf[VSS_READ_TRASH_DIR_NAME_OFFSET]);
}

static inline void vss_read_trash_dir_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_READ_TRASH_DIR_NAME_OFFSET]), val, vss_read_trash_dir_get_name_length(buf));
}

/// VSS_READ_TRASH_DIR_REPLY
/// Same as VSS_READ_DIR_REPLY

/// VSS_CHMOD
/// Read the current contents of an object's directory component.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | ATTRS                                                             |
///    +----------------+----------------+----------------+----------------+
/// 16 | ATTRS_MASK                                                        |
///    +----------------+----------------+----------------+----------------+
/// 20 | NAME_LENGTH                     |  NAME                           |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    ATTRS - Object attributes to be applied
///    ATTRS_MASK - Attribute mask to control clearing/setting of bits.
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_CHMOD_HANDLE_OFFSET   = 0,
    VSS_CHMOD_OBJVER_OFFSET   = 4,
    VSS_CHMOD_ATTRS_OFFSET    = 12,
    VSS_CHMOD_ATTRS_MASK_OFFSET  = 16,
    VSS_CHMOD_NAME_LENGTH_OFFSET = 20,
    VSS_CHMOD_NAME_OFFSET     = 22,
    VSS_CHMOD_BASE_SIZE       = 22
};

static inline u32 vss_chmod_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CHMOD_HANDLE_OFFSET])));
}

static inline void vss_chmod_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CHMOD_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_chmod_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_CHMOD_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_CHMOD_OBJVER_OFFSET])));
#endif
}

static inline void vss_chmod_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_CHMOD_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_CHMOD_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_chmod_get_attrs(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CHMOD_ATTRS_OFFSET])));
}

static inline void vss_chmod_set_attrs(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CHMOD_ATTRS_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_chmod_get_attrs_mask(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CHMOD_ATTRS_MASK_OFFSET])));
}

static inline void vss_chmod_set_attrs_mask(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CHMOD_ATTRS_MASK_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_chmod_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_CHMOD_NAME_LENGTH_OFFSET])));
}

static inline void vss_chmod_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_CHMOD_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_chmod_get_name(const char* buf)
{
    return (char*)&(buf[VSS_CHMOD_NAME_OFFSET]);
}

static inline void vss_chmod_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_CHMOD_NAME_OFFSET]), val, vss_chmod_get_name_length(buf));
}

/// VSS_CHMOD_REPLY
/// Same as VSS_VERSION_REPLY

/// VSS_STAT
/// Read the current contents of an object's directory component.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | NAME_LENGTH                     |  NAME                           |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_STAT_HANDLE_OFFSET   = 0,
    VSS_STAT_OBJVER_OFFSET   = 4,
    VSS_STAT_NAME_LENGTH_OFFSET = 12,
    VSS_STAT_NAME_OFFSET     = 14,
    VSS_STAT_BASE_SIZE       = 14
};

static inline u32 vss_stat_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_STAT_HANDLE_OFFSET])));
}

static inline void vss_stat_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_STAT_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_stat_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_STAT_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_STAT_OBJVER_OFFSET])));
#endif
}

static inline void vss_stat_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_STAT_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_STAT_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_stat_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_STAT_NAME_LENGTH_OFFSET])));
}

static inline void vss_stat_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_STAT_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_stat_get_name(const char* buf)
{
    return (char*)&(buf[VSS_STAT_NAME_OFFSET]);
}

static inline void vss_stat_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_STAT_NAME_OFFSET]), val, vss_stat_get_name_length(buf));
}

/// VSS_STAT_REPLY
/// Same as VSS_READ_DIR_REPLY


/// VSS_STAT2
/// Same as VSS_STAT
/// VSS_STAT2_REPLY
/// Same as VSS_READ_DIR2_REPLY

/// VSS_STAT_TRASH
/// Read the current contents of an object's directory component.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | RECORD_VERSION                                                    |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | RECORD_INDEX                                                      |
///    +----------------+----------------+----------------+----------------+
/// 24 | NAME_LENGTH                     |  NAME                           |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
///    RECORD_VERSION - Trashcan record identifier: version component.
///    RECORD_INDEX - Trashcan record identifier: change index component.
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_STAT_TRASH_HANDLE_OFFSET   = 0,
    VSS_STAT_TRASH_OBJVER_OFFSET   = 4,
    VSS_STAT_TRASH_RECVER_OFFSET   = 12,
    VSS_STAT_TRASH_RECIDX_OFFSET   = 20,
    VSS_STAT_TRASH_NAME_LENGTH_OFFSET = 24,
    VSS_STAT_TRASH_NAME_OFFSET     = 26,
    VSS_STAT_TRASH_BASE_SIZE       = 26
};

static inline u32 vss_stat_trash_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_STAT_TRASH_HANDLE_OFFSET])));
}

static inline void vss_stat_trash_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_STAT_TRASH_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_stat_trash_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_STAT_TRASH_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_STAT_TRASH_OBJVER_OFFSET])));
#endif
}

static inline void vss_stat_trash_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_STAT_TRASH_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_STAT_TRASH_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_stat_trash_get_recver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_STAT_TRASH_RECVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_STAT_TRASH_RECVER_OFFSET])));
#endif
}

static inline void vss_stat_trash_set_recver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_STAT_TRASH_RECVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_STAT_TRASH_RECVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_stat_trash_get_recidx(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_STAT_TRASH_RECIDX_OFFSET])));
}

static inline void vss_stat_trash_set_recidx(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_STAT_TRASH_RECIDX_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_stat_trash_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_STAT_TRASH_NAME_LENGTH_OFFSET])));
}

static inline void vss_stat_trash_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_STAT_TRASH_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_stat_trash_get_name(const char* buf)
{
    return (char*)&(buf[VSS_STAT_TRASH_NAME_OFFSET]);
}

static inline void vss_stat_trash_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_STAT_TRASH_NAME_OFFSET]), val, vss_stat_trash_get_name_length(buf));
}

/// VSS_STAT_TRASH_REPLY
/// Same as VSS_READ_DIR_REPLY

/// VSS_READ_DIR_ENTRY
/// READ_DIR_REPLY and STAT_REPLY data has the following layout per entry:
///    +----------------+----------------+----------------+----------------+
///  0 | SIZE                                                              |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | CTIME                                                             |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | MTIME                                                             |
/// 20 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 24 | CHANGE_VERSION                                                    |
/// 28 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 32 | SIGNATURE                                                         |
/// 36 |                                                                   |
/// 40 |                                                                   |
/// 44 |                                                                   |
/// 48 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 52 | IS_DIR         | RESERVED       |  META_SIZE                      |
///    +----------------+----------------+----------------+----------------+
/// 56 | NAME_LEN                        | NAME                            |
///    +----------------+----------------+                                 |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | METADATA                                                          |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    SIZE - For files, size of the file; for directories, zero.
///    CTIME - Creation time, in microseconds, since epoch.
///    MTIME - Modification time, in microseconds, since epoch.
///    CHANGE_VERSION - Object version when this component was last changed.
///    SIGNATURE - For files, SHA-1 hash of the file. For directories, all 0.
///    IS_DIR - 0 for files, nonzero for directories.
///    META_CNT - Count of metadata entries.
///    META_SIZE - Total size of all metadata for this entry.
///    NAME_LEN - Length of the name for this entry.
///    NAME - NAME_LEN bytes of the entry name.
///    METADATA - Attached metadata for this entry.
enum {
    VSS_DIRENT_SIZE_OFFSET      =  0,
    VSS_DIRENT_CTIME_OFFSET     =  8,
    VSS_DIRENT_MTIME_OFFSET     = 16,
    VSS_DIRENT_CHANGE_VER_OFFSET= 24,
    VSS_DIRENT_SIGNATURE_OFFSET = 32,
    VSS_DIRENT_SIGNATURE_SIZE = 20,
    VSS_DIRENT_IS_DIR_OFFSET    = 52,
    VSS_DIRENT_RESERVED_OFFSET  = 53,
    VSS_DIRENT_META_SIZE_OFFSET = 54,
    VSS_DIRENT_NAME_LEN_OFFSET  = 56,
    VSS_DIRENT_NAME_OFFSET      = 58,    
    VSS_DIRENT_BASE_SIZE        = 58
};

static inline u64 vss_dirent_get_size(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT_SIZE_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT_SIZE_OFFSET])));
#endif
}

static inline void vss_dirent_set_size(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT_SIZE_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT_SIZE_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_dirent_get_ctime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT_CTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT_CTIME_OFFSET])));
#endif
}

static inline void vss_dirent_set_ctime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT_CTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT_CTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_dirent_get_mtime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT_MTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT_MTIME_OFFSET])));
#endif
}

static inline void vss_dirent_set_mtime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT_MTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT_MTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_dirent_get_change_ver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT_CHANGE_VER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT_CHANGE_VER_OFFSET])));
#endif
}

static inline void vss_dirent_set_change_ver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT_CHANGE_VER_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT_CHANGE_VER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline char* vss_dirent_get_signature(const char* buf)
{
    return (char*)&(buf[VSS_DIRENT_SIGNATURE_OFFSET]);
}

static inline void vss_dirent_set_signature(char* buf, const char* val)
{
    memcpy(&(buf[VSS_DIRENT_SIGNATURE_OFFSET]),
           val, VSS_DIRENT_SIGNATURE_SIZE);
}

static inline u8 vss_dirent_get_is_dir(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_DIRENT_IS_DIR_OFFSET])));
}

static inline void vss_dirent_set_is_dir(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_DIRENT_IS_DIR_OFFSET])) = VPLConv_hton_u8(val);
}

static inline void vss_dirent_set_reserved(char* buf)
    {
    *((u8*)&(buf[VSS_DIRENT_RESERVED_OFFSET])) = VPLConv_hton_u8(0);
}

static inline u16 vss_dirent_get_meta_size(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_DIRENT_META_SIZE_OFFSET])));
}

static inline void vss_dirent_set_meta_size(char* buf, u16 val)
    {
    *((u16*)&(buf[VSS_DIRENT_META_SIZE_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u16 vss_dirent_get_name_len(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_DIRENT_NAME_LEN_OFFSET])));
}

static inline void vss_dirent_set_name_len(char* buf, u16 val)
    {
    *((u16*)&(buf[VSS_DIRENT_NAME_LEN_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_dirent_get_name(const char* buf)
{
    return (char*)&(buf[VSS_DIRENT_NAME_OFFSET]);
}

static inline void vss_dirent_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_DIRENT_NAME_OFFSET]), val, vss_dirent_get_name_len(buf));
}

static inline char* vss_dirent_get_metadata(const char* buf)
{
    return (char*)&(buf[VSS_DIRENT_NAME_OFFSET + vss_dirent_get_name_len(buf)]);
}

static inline void vss_dirent_set_metadata(char* buf, const char* val)
{
    memcpy(vss_dirent_get_metadata(buf), val, vss_dirent_get_meta_size(buf));
}

/// VSS_READ_DIR2_ENTRY
/// READ_DIR2_REPLY and STAT2_REPLY data has the following layout per entry:
///    +----------------+----------------+----------------+----------------+
///  0 | SIZE                                                              |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | CTIME                                                             |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | MTIME                                                             |
/// 20 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 24 | CHANGE_VERSION                                                    |
/// 28 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 32 | SIGNATURE                                                         |
/// 36 |                                                                   |
/// 40 |                                                                   |
/// 44 |                                                                   |
/// 48 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 52 | ATTRS                                                             |
///    +----------------+----------------+----------------+----------------+
/// 56 | IS_DIR         | RESERVED       |  META_SIZE                      |
///    +----------------+----------------+----------------+----------------+
/// 60 | NAME_LEN                        | NAME                            |
///    +----------------+----------------+                                 |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | METADATA                                                          |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    SIZE - For files, size of the file; for directories, zero.
///    CTIME - Creation time, in microseconds, since epoch.
///    MTIME - Modification time, in microseconds, since epoch.
///    CHANGE_VERSION - Object version when this component was last changed.
///    SIGNATURE - For files, SHA-1 hash of the file. For directories, all 0.
///    ATTRS - File attribute
///    IS_DIR - 0 for files, nonzero for directories.
///    META_CNT - Count of metadata entries.
///    META_SIZE - Total size of all metadata for this entry.
///    NAME_LEN - Length of the name for this entry.
///    NAME - NAME_LEN bytes of the entry name.
///    METADATA - Attached metadata for this entry.
enum {
    VSS_DIRENT2_SIZE_OFFSET      =  0,
    VSS_DIRENT2_CTIME_OFFSET     =  8,
    VSS_DIRENT2_MTIME_OFFSET     = 16,
    VSS_DIRENT2_CHANGE_VER_OFFSET= 24,
    VSS_DIRENT2_SIGNATURE_OFFSET = 32,
    VSS_DIRENT2_SIGNATURE_SIZE = 20,
    VSS_DIRENT2_ATTRS_OFFSET     = 52,
    VSS_DIRENT2_IS_DIR_OFFSET    = 56,
    VSS_DIRENT2_RESERVED_OFFSET  = 57,
    VSS_DIRENT2_META_SIZE_OFFSET = 58,
    VSS_DIRENT2_NAME_LEN_OFFSET  = 60,
    VSS_DIRENT2_NAME_OFFSET      = 62,    
    VSS_DIRENT2_BASE_SIZE        = 62
};

static inline u64 vss_dirent2_get_size(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT2_SIZE_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT2_SIZE_OFFSET])));
#endif
}

static inline void vss_dirent2_set_size(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT2_SIZE_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT2_SIZE_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_dirent2_get_ctime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT2_CTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT2_CTIME_OFFSET])));
#endif
}

static inline void vss_dirent2_set_ctime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT2_CTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT2_CTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_dirent2_get_mtime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT2_MTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT2_MTIME_OFFSET])));
#endif
}

static inline void vss_dirent2_set_mtime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT2_MTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT2_MTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_dirent2_get_change_ver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DIRENT2_CHANGE_VER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DIRENT2_CHANGE_VER_OFFSET])));
#endif
}

static inline void vss_dirent2_set_change_ver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DIRENT2_CHANGE_VER_OFFSET]));
#else
    *((u64*)&(buf[VSS_DIRENT2_CHANGE_VER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline char* vss_dirent2_get_signature(const char* buf)
{
    return (char*)&(buf[VSS_DIRENT2_SIGNATURE_OFFSET]);
}

static inline void vss_dirent2_set_signature(char* buf, const char* val)
{
    memcpy(&(buf[VSS_DIRENT2_SIGNATURE_OFFSET]),
           val, VSS_DIRENT2_SIGNATURE_SIZE);
}

static inline u32 vss_dirent2_get_attrs(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_DIRENT2_ATTRS_OFFSET])));
}

static inline void vss_dirent2_set_attrs(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_DIRENT2_ATTRS_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u8 vss_dirent2_get_is_dir(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_DIRENT2_IS_DIR_OFFSET])));
}

static inline void vss_dirent2_set_is_dir(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_DIRENT2_IS_DIR_OFFSET])) = VPLConv_hton_u8(val);
}

static inline void vss_dirent2_set_reserved(char* buf)
    {
    *((u8*)&(buf[VSS_DIRENT2_RESERVED_OFFSET])) = VPLConv_hton_u8(0);
}

static inline u16 vss_dirent2_get_meta_size(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_DIRENT2_META_SIZE_OFFSET])));
}

static inline void vss_dirent2_set_meta_size(char* buf, u16 val)
    {
    *((u16*)&(buf[VSS_DIRENT2_META_SIZE_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u16 vss_dirent2_get_name_len(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_DIRENT2_NAME_LEN_OFFSET])));
}

static inline void vss_dirent2_set_name_len(char* buf, u16 val)
    {
    *((u16*)&(buf[VSS_DIRENT2_NAME_LEN_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_dirent2_get_name(const char* buf)
{
    return (char*)&(buf[VSS_DIRENT2_NAME_OFFSET]);
}

static inline void vss_dirent2_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_DIRENT2_NAME_OFFSET]), val, vss_dirent2_get_name_len(buf));
}

static inline char* vss_dirent2_get_metadata(const char* buf)
{
    return (char*)&(buf[VSS_DIRENT2_NAME_OFFSET + vss_dirent2_get_name_len(buf)]);
}

static inline void vss_dirent2_set_metadata(char* buf, const char* val)
{
    memcpy(vss_dirent2_get_metadata(buf), val, vss_dirent2_get_meta_size(buf));
}

/// VSS_READ_DIR_ENTRY_METADATA
/// Each directory entry metadata field has this format:
///    +----------------+----------------+----------------+----------------+
///  0 | TYPE           | LENGTH         | VALUE                           |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    TYPE - Metadata type.
///    LENGTH - Length of metadata data.
///    VALUE - Metadata value, for LENGTH bytes.
enum {
    VSS_DIRENT_META_TYPE_OFFSET      =  0,
    VSS_DIRENT_META_LENGTH_OFFSET    =  1,
    VSS_DIRENT_META_VALUE_OFFSET     =  2,
    VSS_DIRENT_META_BASE_SIZE        =  2
};

static inline u8 vss_dirent_metadata_get_type(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_DIRENT_META_TYPE_OFFSET])));
}

static inline void vss_dirent_metadata_set_type(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_DIRENT_META_TYPE_OFFSET])) = VPLConv_hton_u8(val);
}

static inline u8 vss_dirent_metadata_get_length(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_DIRENT_META_LENGTH_OFFSET])));
}

static inline void vss_dirent_metadata_set_length(char* buf, u8 val)
    {
    *((u8*)&(buf[VSS_DIRENT_META_LENGTH_OFFSET])) = VPLConv_hton_u8(val);
}

static inline char* vss_dirent_metadata_get_value(const char* buf)
{
    return (char*)&(buf[VSS_DIRENT_META_VALUE_OFFSET]);
}

static inline void vss_dirent_metadata_set_value(char* buf, const char* val)
{
    memcpy(vss_dirent_metadata_get_value(buf), val, vss_dirent_metadata_get_length(buf));
}

/// VSS_READ_TRASHCAN
/// Read the current contents of an object's directory component.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being read (last committed version).
enum {
    VSS_READ_TRASHCAN_HANDLE_OFFSET   = 0,
    VSS_READ_TRASHCAN_OBJVER_OFFSET   = 4,
    VSS_READ_TRASHCAN_BASE_SIZE       = 12
};

static inline u32 vss_read_trashcan_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_TRASHCAN_HANDLE_OFFSET])));
}

static inline void vss_read_trashcan_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_TRASHCAN_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_trashcan_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_READ_TRASHCAN_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_TRASHCAN_OBJVER_OFFSET])));
#endif
}

static inline void vss_read_trashcan_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_READ_TRASHCAN_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_READ_TRASHCAN_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_READ_TRASHCAN_REPLY
/// Same as VSS_READ_DIR_REPLY

/// VSS_TRASHCAN_RECORD
/// READ_TRASHCAN_REPLY data has the following layout per entry:
///    +----------------+----------------+----------------+----------------+
///  0 | RECORD_VERSION                                                    |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | RECORD_INDEX                                                      |
///    +----------------+----------------+----------------+----------------+
/// 12 | SIZE                                                              |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | CTIME                                                             |
/// 24 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 28 | MTIME                                                             |
/// 32 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 36 | DTIME                                                             |
/// 40 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 44 | IS_DIR         | RESERVED       |  NAME_LEN                       |
///    +----------------+----------------+----------------+----------------+
/// 48 | NAME                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    RECORD_VERSION - Trashcan record identifier: version component.
///    RECORD_INDEX - Trashcan record identifier: change index component.
///    SIZE - For files, size of the file; for directories, sum of contained file sizes.
///    CTIME - Creation time, in microoseconds, since epoch.
///    MTIME - Modification time, in microseconds, since epoch.
///    DTIME - Deletion time, in microseconds, since epoch.
///    IS_DIR - 0 for files, nonzero for directories.
///    NAME_LEN - Length of the name for this entry.
///    NAME - NAME_LEN bytes of the entry name.
enum {
    VSS_TRASHREC_RECVER_OFFSET    =  0,
    VSS_TRASHREC_RECIDX_OFFSET    =  8,
    VSS_TRASHREC_SIZE_OFFSET      = 12,
    VSS_TRASHREC_CTIME_OFFSET     = 20,
    VSS_TRASHREC_MTIME_OFFSET     = 28,
    VSS_TRASHREC_DTIME_OFFSET     = 36,
    VSS_TRASHREC_IS_DIR_OFFSET    = 44,
    VSS_TRASHREC_RESERVED_OFFSET  = 45,
    VSS_TRASHREC_NAME_LEN_OFFSET  = 46,
    VSS_TRASHREC_NAME_OFFSET      = 48,    
    VSS_TRASHREC_BASE_SIZE        = 48
};

static inline u64 vss_trashrec_get_recver(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_TRASHREC_RECVER_OFFSET])));
}

static inline void vss_trashrec_set_recver(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_TRASHREC_RECVER_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_trashrec_get_recidx(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_TRASHREC_RECIDX_OFFSET])));
}

static inline void vss_trashrec_set_recidx(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_TRASHREC_RECIDX_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_trashrec_get_size(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_TRASHREC_SIZE_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_TRASHREC_SIZE_OFFSET])));
#endif
}

static inline void vss_trashrec_set_size(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_TRASHREC_SIZE_OFFSET]));
#else
    *((u64*)&(buf[VSS_TRASHREC_SIZE_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_trashrec_get_ctime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_TRASHREC_CTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_TRASHREC_CTIME_OFFSET])));
#endif
}

static inline void vss_trashrec_set_ctime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_TRASHREC_CTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_TRASHREC_CTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_trashrec_get_mtime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_TRASHREC_MTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_TRASHREC_MTIME_OFFSET])));
#endif
}

static inline void vss_trashrec_set_mtime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_TRASHREC_MTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_TRASHREC_MTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_trashrec_get_dtime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_TRASHREC_DTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_TRASHREC_DTIME_OFFSET])));
#endif
}

static inline void vss_trashrec_set_dtime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_TRASHREC_DTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_TRASHREC_DTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u8 vss_trashrec_get_is_dir(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_TRASHREC_IS_DIR_OFFSET])));
}

static inline void vss_trashrec_set_is_dir(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_TRASHREC_IS_DIR_OFFSET])) = VPLConv_hton_u8(val);
}

static inline void vss_trashrec_set_reserved(char* buf)
    {
    *((u8*)&(buf[VSS_TRASHREC_RESERVED_OFFSET])) = VPLConv_hton_u8(0);
}

static inline u16 vss_trashrec_get_name_len(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_TRASHREC_NAME_LEN_OFFSET])));
}

static inline void vss_trashrec_set_name_len(char* buf, u16 val)
    {
    *((u16*)&(buf[VSS_TRASHREC_NAME_LEN_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_trashrec_get_name(const char* buf)
{
    return (char*)&(buf[VSS_TRASHREC_NAME_OFFSET]);
}

static inline void vss_trashrec_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_TRASHREC_NAME_OFFSET]), val, vss_trashrec_get_name_len(buf));
}

/// VSS_WRITE
/// Write data to an open writable file
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+---------------------------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | OFFSET                                                            |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// 24 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// 28 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | DATA                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    OFFSET - Byte offset into file for write.
///    LENGTH - Length of data to write, in bytes.
///    NAME_LENGTH - Length of the component file name, including absolute path, to write.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
///    DATA - LENGTH bytes of data to write to the file starting at OFFSET.
enum {
    VSS_WRITE_HANDLE_OFFSET   =  0,
    VSS_WRITE_OBJVER_OFFSET   =  4,
    VSS_WRITE_OFFSET_OFFSET   = 12,
    VSS_WRITE_LENGTH_OFFSET   = 20,
    VSS_WRITE_NAME_LENGTH_OFFSET = 24,
    VSS_WRITE_NAME_OFFSET     = 26,
    VSS_WRITE_BASE_SIZE       = 26
};

static inline u32 vss_write_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_WRITE_HANDLE_OFFSET])));
}

static inline void vss_write_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_WRITE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_write_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_WRITE_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_WRITE_OBJVER_OFFSET])));
#endif
}

static inline void vss_write_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_WRITE_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_WRITE_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_write_get_offset(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_WRITE_OFFSET_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_WRITE_OFFSET_OFFSET])));
#endif
}

static inline void vss_write_set_offset(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_WRITE_OFFSET_OFFSET]));
#else
    *((u64*)&(buf[VSS_WRITE_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_write_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_WRITE_LENGTH_OFFSET])));
}

static inline void vss_write_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_WRITE_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_write_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_WRITE_NAME_LENGTH_OFFSET])));
}

static inline void vss_write_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_WRITE_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_write_get_name(const char* buf)
{
    return (char*)&(buf[VSS_WRITE_NAME_OFFSET]);
}

static inline void vss_write_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_WRITE_NAME_OFFSET]), val, vss_write_get_name_length(buf));
}

static inline char* vss_write_get_data(const char* buf)
{
    return (char*)&(buf[VSS_WRITE_NAME_OFFSET + vss_write_get_name_length(buf)]);
}

/// VSS_WRITE_REPLY
/// Return result for write.
///    +----------------+----------------+---------------------------------+
///  0 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    LENGTH - Length of data successfully written to file starting from OFFSET, in bytes.
///    OBJECT_VERSION - Version of object actually written (current committed version).
enum {
    VSS_WRITER_LENGTH_OFFSET   = 0,
    VSS_WRITER_OBJVER_OFFSET   = 4,
    VSS_WRITER_SIZE            = 12
};

static inline u32 vss_write_reply_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_WRITER_LENGTH_OFFSET])));
}

static inline void vss_write_reply_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_WRITER_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_write_reply_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_WRITER_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_WRITER_OBJVER_OFFSET])));
#endif
}

static inline void vss_write_reply_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_WRITER_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_WRITER_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_MAKE_REF
/// Make a reference copy of an object file.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | REFERENCE_ID                                                      |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | NAME_LENGTH                     | NAMES                           |
///    +----------------+----------------+                                 |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    REFERENCE_ID - Unique 64-bit reference ID to use for this copy.
///    NAME_LENGTH - Length of the component file name, including absolute path, to copy.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_MAKE_REF_HANDLE_OFFSET   =  0,
    VSS_MAKE_REF_OBJVER_OFFSET   =  4,
    VSS_MAKE_REF_ID_OFFSET       =  12,
    VSS_MAKE_REF_NAME_LENGTH_OFFSET = 20,
    VSS_MAKE_REF_NAME_OFFSET     = 22,
    VSS_MAKE_REF_BASE_SIZE       = 22
};

static inline u32 vss_make_ref_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_MAKE_REF_HANDLE_OFFSET])));
}

static inline void vss_make_ref_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_MAKE_REF_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_make_ref_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_MAKE_REF_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_MAKE_REF_OBJVER_OFFSET])));
#endif
}

static inline void vss_make_ref_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_MAKE_REF_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_MAKE_REF_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_make_ref_get_id(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_MAKE_REF_ID_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_MAKE_REF_ID_OFFSET])));
#endif
}

static inline void vss_make_ref_set_id(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_MAKE_REF_ID_OFFSET]));
#else
    *((u64*)&(buf[VSS_MAKE_REF_ID_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_make_ref_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_MAKE_REF_NAME_LENGTH_OFFSET])));
}

static inline void vss_make_ref_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_MAKE_REF_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_make_ref_get_name(const char* buf)
{
    return (char*)&(buf[VSS_MAKE_REF_NAME_OFFSET]);
}

static inline void vss_make_ref_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_MAKE_REF_NAME_OFFSET]), val, vss_make_ref_get_name_length(buf));
}

/// VSS_MAKE_REF_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_MAKE_DIR
/// Make a directory component for an object.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    NAME_LENGTH - Length of the directory name, including absolute path.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_MAKE_DIR_HANDLE_OFFSET   = 0,
    VSS_MAKE_DIR_OBJVER_OFFSET   = 4,
    VSS_MAKE_DIR_NAME_LENGTH_OFFSET = 12,
    VSS_MAKE_DIR_NAME_OFFSET     = 14,
    VSS_MAKE_DIR_BASE_SIZE       = 14
};

static inline u32 vss_make_dir_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_MAKE_DIR_HANDLE_OFFSET])));
}

static inline void vss_make_dir_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_MAKE_DIR_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_make_dir_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_MAKE_DIR_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_MAKE_DIR_OBJVER_OFFSET])));
#endif
}

static inline void vss_make_dir_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_MAKE_DIR_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_MAKE_DIR_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_make_dir_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_MAKE_DIR_NAME_LENGTH_OFFSET])));
}

static inline void vss_make_dir_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_MAKE_DIR_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_make_dir_get_name(const char* buf)
{
    return (char*)&(buf[VSS_MAKE_DIR_NAME_OFFSET]);
}

static inline void vss_make_dir_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_MAKE_DIR_NAME_OFFSET]), val, vss_make_dir_get_name_length(buf));
}

/// VSS_MAKE_DIR_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_MAKE_DIR2
/// Make a directory component for an object.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | ATTRS                                                             |
///    +----------------+----------------+----------------+----------------+
/// 16 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    NAME_LENGTH - Length of the directory name, including absolute path.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_MAKE_DIR2_HANDLE_OFFSET   = 0,
    VSS_MAKE_DIR2_OBJVER_OFFSET   = 4,
    VSS_MAKE_DIR2_ATTRS_OFFSET    = 12,
    VSS_MAKE_DIR2_NAME_LENGTH_OFFSET = 16,
    VSS_MAKE_DIR2_NAME_OFFSET     = 18,
    VSS_MAKE_DIR2_BASE_SIZE       = 18
};

static inline u32 vss_make_dir2_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_MAKE_DIR2_HANDLE_OFFSET])));
}

static inline void vss_make_dir2_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_MAKE_DIR2_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_make_dir2_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_MAKE_DIR2_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_MAKE_DIR2_OBJVER_OFFSET])));
#endif
}

static inline void vss_make_dir2_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_MAKE_DIR2_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_MAKE_DIR2_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_make_dir2_get_attrs(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_MAKE_DIR2_ATTRS_OFFSET])));
}

static inline void vss_make_dir2_set_attrs(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_MAKE_DIR2_ATTRS_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_make_dir2_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_MAKE_DIR2_NAME_LENGTH_OFFSET])));
}

static inline void vss_make_dir2_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_MAKE_DIR2_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_make_dir2_get_name(const char* buf)
{
    return (char*)&(buf[VSS_MAKE_DIR2_NAME_OFFSET]);
}

static inline void vss_make_dir2_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_MAKE_DIR2_NAME_OFFSET]), val, vss_make_dir2_get_name_length(buf));
}

/// VSS_MAKE_DIR2_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_REMOVE
/// Remove a file or directory.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    NAME_LENGTH - Length of the file or directory name.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_REMOVE_HANDLE_OFFSET   = 0,
    VSS_REMOVE_OBJVER_OFFSET   = 4,
    VSS_REMOVE_NAME_LENGTH_OFFSET = 12,
    VSS_REMOVE_NAME_OFFSET     = 14,
    VSS_REMOVE_BASE_SIZE       = 14
};

static inline u32 vss_remove_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_REMOVE_HANDLE_OFFSET])));
}

static inline void vss_remove_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_REMOVE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_remove_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_REMOVE_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_REMOVE_OBJVER_OFFSET])));
#endif
}

static inline void vss_remove_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_REMOVE_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_REMOVE_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_remove_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_REMOVE_NAME_LENGTH_OFFSET])));
}

static inline void vss_remove_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_REMOVE_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_remove_get_name(const char* buf)
{
    return (char*)&(buf[VSS_REMOVE_NAME_OFFSET]);
}

static inline void vss_remove_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_REMOVE_NAME_OFFSET]), val, vss_remove_get_name_length(buf));
}

/// VSS_REMOVE_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_RENAME
/// Rename a file or directory.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | NAME_LENGTH                     | NEW_NAME_LENGTH                 |
///    +----------------+----------------+---------------------------------+
/// 16 | NAME                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | NEW_NAME                                                          |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    NAME_LENGTH - Length of the file or directory name.
///    NEW_NAME_LENGTH - Length of the new file or directory name.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
///    NEW_NAME - Directory name and absolute path for NEW_NAME_LENGTH bytes.
enum {
    VSS_RENAME_HANDLE_OFFSET          = 0,
    VSS_RENAME_OBJVER_OFFSET          = 4,
    VSS_RENAME_NAME_LENGTH_OFFSET     = 12,
    VSS_RENAME_NEW_NAME_LENGTH_OFFSET = 14,
    VSS_RENAME_NAME_OFFSET            = 16,
    VSS_RENAME_BASE_SIZE              = 16
};

static inline u32 vss_rename_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_RENAME_HANDLE_OFFSET])));
}

static inline void vss_rename_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_RENAME_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_rename_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_RENAME_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_RENAME_OBJVER_OFFSET])));
#endif
}

static inline void vss_rename_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_RENAME_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_RENAME_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_rename_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_RENAME_NAME_LENGTH_OFFSET])));
}

static inline void vss_rename_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_RENAME_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u16 vss_rename_get_new_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_RENAME_NEW_NAME_LENGTH_OFFSET])));
}

static inline void vss_rename_set_new_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_RENAME_NEW_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_rename_get_name(const char* buf)
{
    return (char*)&(buf[VSS_RENAME_NAME_OFFSET]);
}

static inline void vss_rename_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_RENAME_NAME_OFFSET]), val, vss_rename_get_name_length(buf));
}

static inline char* vss_rename_get_new_name(const char* buf)
{
    return (char*)&(buf[VSS_RENAME_NAME_OFFSET + vss_rename_get_name_length(buf)]);
}

static inline void vss_rename_set_new_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_RENAME_NAME_OFFSET + vss_rename_get_name_length(buf)]),
           val, vss_rename_get_new_name_length(buf));
}

/// VSS_RENAME_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_RENAME2
/// Rename a file or directory.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | FLAGS                                                             |
///    +----------------+----------------+----------------+----------------+
///  8 | OBJECT_VERSION                                                    |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | NAME_LENGTH                     | NEW_NAME_LENGTH                 |
///    +----------------+----------------+---------------------------------+
/// 20 | NAME                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | NEW_NAME                                                          |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    FLAGS  - Option flags to control RENAME behavior.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    NAME_LENGTH - Length of the file or directory name.
///    NEW_NAME_LENGTH - Length of the new file or directory name.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
///    NEW_NAME - Directory name and absolute path for NEW_NAME_LENGTH bytes.
enum {
    VSS_RENAME2_HANDLE_OFFSET          = 0,
    VSS_RENAME2_FLAGS_OFFSET           = 4,
    VSS_RENAME2_OBJVER_OFFSET          = 8,
    VSS_RENAME2_NAME_LENGTH_OFFSET     = 16,
    VSS_RENAME2_NEW_NAME_LENGTH_OFFSET = 18,
    VSS_RENAME2_NAME_OFFSET            = 20,
    VSS_RENAME2_BASE_SIZE              = 20
};

static inline u32 vss_rename2_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_RENAME2_HANDLE_OFFSET])));
}

static inline void vss_rename2_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_RENAME2_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_rename2_get_flags(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_RENAME2_FLAGS_OFFSET])));
}

static inline void vss_rename2_set_flags(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_RENAME2_FLAGS_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_rename2_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_RENAME2_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_RENAME2_OBJVER_OFFSET])));
#endif
}

static inline void vss_rename2_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_RENAME2_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_RENAME2_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_rename2_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_RENAME2_NAME_LENGTH_OFFSET])));
}

static inline void vss_rename2_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_RENAME2_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u16 vss_rename2_get_new_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_RENAME2_NEW_NAME_LENGTH_OFFSET])));
}

static inline void vss_rename2_set_new_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_RENAME2_NEW_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_rename2_get_name(const char* buf)
{
    return (char*)&(buf[VSS_RENAME2_NAME_OFFSET]);
}

static inline void vss_rename2_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_RENAME2_NAME_OFFSET]), val, vss_rename2_get_name_length(buf));
}

static inline char* vss_rename2_get_new_name(const char* buf)
{
    return (char*)&(buf[VSS_RENAME2_NAME_OFFSET + vss_rename2_get_name_length(buf)]);
}

static inline void vss_rename2_set_new_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_RENAME2_NAME_OFFSET + vss_rename2_get_name_length(buf)]),
           val, vss_rename2_get_new_name_length(buf));
}

/// VSS_RENAME2_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_COPY
/// Copy a file or directory.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | SOURCE_LENGTH                   | DESTINATION_LENGTH              |
///    +----------------+----------------+---------------------------------+
/// 16 | SOURCE                                                            |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | DESTINATION                                                       |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    SOURCE_LENGTH - Length of the source file or directory name.
///    DESTINATION_LENGTH - Length of the destination file or directory name.
///    SOURCE - Directory name and absolute path for SOURCE_LENGTH bytes.
///    DESTINATION - Directory name and absolute path for DESTINATION_LENGTH bytes.
enum {
    VSS_COPY_HANDLE_OFFSET          = 0,
    VSS_COPY_OBJVER_OFFSET          = 4,
    VSS_COPY_SOURCE_LENGTH_OFFSET   = 12,
    VSS_COPY_DESTINATION_LENGTH_OFFSET = 14,
    VSS_COPY_SOURCE_OFFSET          = 16,
    VSS_COPY_BASE_SIZE              = 16
};

static inline u32 vss_copy_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_COPY_HANDLE_OFFSET])));
}

static inline void vss_copy_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_COPY_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_copy_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_COPY_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_COPY_OBJVER_OFFSET])));
#endif
}

static inline void vss_copy_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_COPY_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_COPY_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_copy_get_source_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_COPY_SOURCE_LENGTH_OFFSET])));
}

static inline void vss_copy_set_source_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_COPY_SOURCE_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u16 vss_copy_get_destination_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_COPY_DESTINATION_LENGTH_OFFSET])));
}

static inline void vss_copy_set_destination_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_COPY_DESTINATION_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_copy_get_source(const char* buf)
{
    return (char*)&(buf[VSS_COPY_SOURCE_OFFSET]);
}

static inline void vss_copy_set_source(char* buf, const char* val)
{
    memcpy(&(buf[VSS_COPY_SOURCE_OFFSET]), val, vss_copy_get_source_length(buf));
}

static inline char* vss_copy_get_destination(const char* buf)
{
    return (char*)&(buf[VSS_COPY_SOURCE_OFFSET + vss_copy_get_source_length(buf)]);
}

static inline void vss_copy_set_destination(char* buf, const char* val)
{
    memcpy(&(buf[VSS_COPY_SOURCE_OFFSET + vss_copy_get_source_length(buf)]),
           val, vss_copy_get_destination_length(buf));
}

/// VSS_COPY_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_COPY_MATCH
/// Make a copy of an existing file with matching size and signature.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | SIZE                                                              |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | DESTINATION_LENGTH              | SIGNATURE                       |
///    +----------------+----------------+                                 +
/// 24 |                                                                   |
/// 28 |                                                                   |
/// 32 |                                                                   |
/// 36 |                                                                   |
///    |                                 +----------------+----------------+
/// 40 |                                 | DESTINATION                     |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    SIZE - Size of the file to match.
///    DESTINATION_LENGTH - Length of the destination file path and name.
///    SIGNATURE - Signature of file to match (SHA1 hash, 20 bytes)
///    DESTINATION - File name and absolute path for DESTINATION_LENGTH bytes.
enum {
    VSS_COPY_MATCH_HANDLE_OFFSET          = 0,
    VSS_COPY_MATCH_OBJVER_OFFSET          = 4,
    VSS_COPY_MATCH_SIZE_OFFSET            = 12,
    VSS_COPY_MATCH_DESTINATION_LENGTH_OFFSET = 20,
    VSS_COPY_MATCH_SIGNATURE_OFFSET       = 22,
    VSS_COPY_MATCH_SIGNATURE_SIZE     = 20,
    VSS_COPY_MATCH_DESTINATION_OFFSET     = 42,
    VSS_COPY_MATCH_BASE_SIZE              = 42
};

static inline u32 vss_copy_match_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_COPY_MATCH_HANDLE_OFFSET])));
}

static inline void vss_copy_match_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_COPY_MATCH_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_copy_match_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_COPY_MATCH_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_COPY_MATCH_OBJVER_OFFSET])));
#endif
}

static inline void vss_copy_match_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_COPY_MATCH_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_COPY_MATCH_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_copy_match_get_size(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_COPY_MATCH_SIZE_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_COPY_MATCH_SIZE_OFFSET])));
#endif
}

static inline void vss_copy_match_set_size(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_COPY_MATCH_SIZE_OFFSET]));
#else
    *((u64*)&(buf[VSS_COPY_MATCH_SIZE_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_copy_match_get_destination_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_COPY_MATCH_DESTINATION_LENGTH_OFFSET])));
}

static inline void vss_copy_match_set_destination_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_COPY_MATCH_DESTINATION_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_copy_match_get_signature(const char* buf)
{
    return (char*)&(buf[VSS_COPY_MATCH_SIGNATURE_OFFSET]);
}

static inline void vss_copy_match_set_signature(char* buf, const char* val)
{
    memcpy(&(buf[VSS_COPY_MATCH_SIGNATURE_OFFSET]), val, VSS_COPY_MATCH_SIGNATURE_SIZE);
}

static inline char* vss_copy_match_get_destination(const char* buf)
{
    return (char*)&(buf[VSS_COPY_MATCH_DESTINATION_OFFSET]);
}

static inline void vss_copy_match_set_destination(char* buf, const char* val)
{
    memcpy(&(buf[VSS_COPY_MATCH_DESTINATION_OFFSET]),
           val, vss_copy_match_get_destination_length(buf));
}

/// VSS_COPY_MATCH_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_SET_TIMES
/// Set times for a file
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | CTIME                                                             |
///    +                                                                   +
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | MTIME                                                             |
///    +                                                                   +
/// 24 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 28 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    TIMES - File times to set.
///    NAME_LENGTH - Length of the file or directory name.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_SET_TIMES_HANDLE_OFFSET   = 0,
    VSS_SET_TIMES_OBJVER_OFFSET   = 4,
    VSS_SET_TIMES_CTIME_OFFSET    = 12,
    VSS_SET_TIMES_MTIME_OFFSET    = 20,
    VSS_SET_TIMES_NAME_LENGTH_OFFSET = 28,
    VSS_SET_TIMES_NAME_OFFSET     = 30,
    VSS_SET_TIMES_BASE_SIZE       = 30
};

static inline u32 vss_set_times_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_TIMES_HANDLE_OFFSET])));
}

static inline void vss_set_times_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_TIMES_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_set_times_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_SET_TIMES_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_TIMES_OBJVER_OFFSET])));
#endif
}

static inline void vss_set_times_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_SET_TIMES_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_SET_TIMES_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_set_times_get_ctime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_SET_TIMES_CTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_TIMES_CTIME_OFFSET])));
#endif
}

static inline void vss_set_times_set_ctime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_SET_TIMES_CTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_SET_TIMES_CTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_set_times_get_mtime(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_SET_TIMES_MTIME_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_TIMES_MTIME_OFFSET])));
#endif
}

static inline void vss_set_times_set_mtime(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_SET_TIMES_MTIME_OFFSET]));
#else
    *((u64*)&(buf[VSS_SET_TIMES_MTIME_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u16 vss_set_times_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_SET_TIMES_NAME_LENGTH_OFFSET])));
}

static inline void vss_set_times_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_SET_TIMES_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_set_times_get_name(const char* buf)
{
    return (char*)&(buf[VSS_SET_TIMES_NAME_OFFSET]);
}

static inline void vss_set_times_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_SET_TIMES_NAME_OFFSET]), val, vss_set_times_get_name_length(buf));
}

/// VSS_SET_TIMES_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_SET_SIZE
/// Set size for a file
///    +----------------+----------------+----------------+----------------+
///  0 | SIZE                                                              |
///    +                                                                   +
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
/// 12 | OBJECT_VERSION                                                    |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    SIZE - File size to set.
///    NAME_LENGTH - Length of the file or directory name.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_SET_SIZE_SIZE_OFFSET     = 0,
    VSS_SET_SIZE_HANDLE_OFFSET   = 8,
    VSS_SET_SIZE_OBJVER_OFFSET   = 12,
    VSS_SET_SIZE_NAME_LENGTH_OFFSET = 20,
    VSS_SET_SIZE_NAME_OFFSET     = 22,
    VSS_SET_SIZE_BASE_SIZE       = 22
};

static inline u32 vss_set_size_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_SIZE_HANDLE_OFFSET])));
}

static inline void vss_set_size_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_SIZE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_set_size_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_SET_SIZE_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_SIZE_OBJVER_OFFSET])));
#endif
}

static inline void vss_set_size_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_SET_SIZE_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_SET_SIZE_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_set_size_get_size(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_SIZE_SIZE_OFFSET])));
}

static inline void vss_set_size_set_size(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_SET_SIZE_SIZE_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u16 vss_set_size_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_SET_SIZE_NAME_LENGTH_OFFSET])));
}

static inline void vss_set_size_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_SET_SIZE_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_set_size_get_name(const char* buf)
{
    return (char*)&(buf[VSS_SET_SIZE_NAME_OFFSET]);
}

static inline void vss_set_size_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_SET_SIZE_NAME_OFFSET]), val, vss_set_size_get_name_length(buf));
}

/// VSS_SET_SIZE_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_SET_METADATA
/// Set a metadata entry for a component.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | TYPE           | LENGTH         | NAME_LENGTH                     |
///    +----------------+----------------+---------------------------------+
/// 16 | NAME                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// ?? | VALUE                                                             |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    TYPE - Metadata type (key).
///    LENGTH - Length of metadata value in bytes.
///    NAME_LENGTH - Length of the file or directory name.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
///    VALUE - Mtadata value, for LENGTH bytes.
enum {
    VSS_SET_METADATA_HANDLE_OFFSET        =  0,
    VSS_SET_METADATA_OBJVER_OFFSET        =  4,
    VSS_SET_METADATA_TYPE_OFFSET          = 12,
    VSS_SET_METADATA_LENGTH_OFFSET        = 13,
    VSS_SET_METADATA_NAME_LENGTH_OFFSET   = 14,
    VSS_SET_METADATA_NAME_OFFSET          = 16,
    VSS_SET_METADATA_BASE_SIZE            = 16
};

static inline u32 vss_set_metadata_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_METADATA_HANDLE_OFFSET])));
}

static inline void vss_set_metadata_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_METADATA_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_set_metadata_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_SET_METADATA_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_METADATA_OBJVER_OFFSET])));
#endif
}

static inline void vss_set_metadata_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_SET_METADATA_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_SET_METADATA_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u8 vss_set_metadata_get_type(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_SET_METADATA_TYPE_OFFSET])));
}

static inline void vss_set_metadata_set_type(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_SET_METADATA_TYPE_OFFSET])) = VPLConv_hton_u8(val);
}

static inline u8 vss_set_metadata_get_length(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_SET_METADATA_LENGTH_OFFSET])));
}

static inline void vss_set_metadata_set_length(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_SET_METADATA_LENGTH_OFFSET])) = VPLConv_hton_u8(val);
}

static inline u16 vss_set_metadata_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_SET_METADATA_NAME_LENGTH_OFFSET])));
}

static inline void vss_set_metadata_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_SET_METADATA_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_set_metadata_get_name(const char* buf)
{
    return (char*)&(buf[VSS_SET_METADATA_NAME_OFFSET]);
}

static inline void vss_set_metadata_set_name(char* buf, const char* val)
{
    memcpy(vss_set_metadata_get_name(buf), val, vss_set_metadata_get_name_length(buf));
}

static inline char* vss_set_metadata_get_value(const char* buf)
{
    return (char*)&(buf[VSS_SET_METADATA_NAME_OFFSET + vss_set_metadata_get_name_length(buf)]);
}

static inline void vss_set_metadata_set_value(char* buf, const char* val)
{
    memcpy(vss_set_metadata_get_value(buf), val, vss_set_metadata_get_length(buf));
}

/// VSS_SET_METADATA_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_EMPTY_TRASH
/// Delete all trash records of the object permanently.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
enum {
    VSS_EMPTY_TRASH_HANDLE_OFFSET          = 0,
    VSS_EMPTY_TRASH_OBJVER_OFFSET          = 4,
    VSS_EMPTY_TRASH_BASE_SIZE              = 12
};

static inline u32 vss_empty_trash_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_EMPTY_TRASH_HANDLE_OFFSET])));
}

static inline void vss_empty_trash_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_EMPTY_TRASH_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_empty_trash_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_EMPTY_TRASH_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_EMPTY_TRASH_OBJVER_OFFSET])));
#endif
}

static inline void vss_empty_trash_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_EMPTY_TRASH_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_EMPTY_TRASH_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_EMPTY_TRASH_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_DELETE_TRASH
///Delete a trash record of the object permanently.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | RECORD_VERSION                                                    |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | RECORD_INDEX                                                      |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    RECORD_VERSION - Trashcan record identifier: version component.
///    RECORD_INDEX - Trashcan record identifier: change index component.
enum {
    VSS_DELETE_TRASH_HANDLE_OFFSET          = 0,
    VSS_DELETE_TRASH_OBJVER_OFFSET          = 4,
    VSS_DELETE_TRASH_RECVER_OFFSET          = 12,
    VSS_DELETE_TRASH_RECIDX_OFFSET          = 20,
    VSS_DELETE_TRASH_BASE_SIZE              = 24
};

static inline u32 vss_delete_trash_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_DELETE_TRASH_HANDLE_OFFSET])));
}

static inline void vss_delete_trash_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_DELETE_TRASH_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_delete_trash_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DELETE_TRASH_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DELETE_TRASH_OBJVER_OFFSET])));
#endif
}

static inline void vss_delete_trash_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DELETE_TRASH_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_DELETE_TRASH_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_delete_trash_get_recver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_DELETE_TRASH_RECVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_DELETE_TRASH_RECVER_OFFSET])));
#endif
}

static inline void vss_delete_trash_set_recver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_DELETE_TRASH_RECVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_DELETE_TRASH_RECVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_delete_trash_get_recidx(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_DELETE_TRASH_RECIDX_OFFSET])));
}

static inline void vss_delete_trash_set_recidx(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_DELETE_TRASH_RECIDX_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_DELETE_TRASH_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_RESTORE_TRASH
/// Restore a trash record to the object.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | OBJECT_VERSION                                                    |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 12 | RECORD_VERSION                                                    |
/// 16 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 20 | RECORD_INDEX                                                      |
///    +----------------+----------------+----------------+----------------+
/// 24 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    OBJECT_VERSION - Version of object being written (last committed version).
///    RECORD_VERSION - Trashcan record identifier: version component.
///    RECORD_INDEX - Trashcan record identifier: change index component.
///    NAME_LENGTH - Length of the file or directory name.
///    NAME - Directory name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_RESTORE_TRASH_HANDLE_OFFSET          = 0,
    VSS_RESTORE_TRASH_OBJVER_OFFSET          = 4,
    VSS_RESTORE_TRASH_RECVER_OFFSET          = 12,
    VSS_RESTORE_TRASH_RECIDX_OFFSET          = 20,
    VSS_RESTORE_TRASH_NAME_LENGTH_OFFSET     = 24,
    VSS_RESTORE_TRASH_NAME_OFFSET            = 26,
    VSS_RESTORE_TRASH_BASE_SIZE              = 26
};

static inline u32 vss_restore_trash_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_RESTORE_TRASH_HANDLE_OFFSET])));
}

static inline void vss_restore_trash_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_RESTORE_TRASH_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_restore_trash_get_objver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_RESTORE_TRASH_OBJVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_RESTORE_TRASH_OBJVER_OFFSET])));
#endif
}

static inline void vss_restore_trash_set_objver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_RESTORE_TRASH_OBJVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_RESTORE_TRASH_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u64 vss_restore_trash_get_recver(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_RESTORE_TRASH_RECVER_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_RESTORE_TRASH_RECVER_OFFSET])));
#endif
}

static inline void vss_restore_trash_set_recver(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_RESTORE_TRASH_RECVER_OFFSET]));
#else
    *((u64*)&(buf[VSS_RESTORE_TRASH_RECVER_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_restore_trash_get_recidx(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_RESTORE_TRASH_RECIDX_OFFSET])));
}

static inline void vss_restore_trash_set_recidx(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_RESTORE_TRASH_RECIDX_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_restore_trash_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_RESTORE_TRASH_NAME_LENGTH_OFFSET])));
}

static inline void vss_restore_trash_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_RESTORE_TRASH_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_restore_trash_get_name(const char* buf)
{
    return (char*)&(buf[VSS_RESTORE_TRASH_NAME_OFFSET]);
}

static inline void vss_restore_trash_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_RESTORE_TRASH_NAME_OFFSET]), val, vss_restore_trash_get_name_length(buf));
}

/// VSS_RESTORE_TRASH_REPLY
/// Same as VSS_VERSION_REPLY reply

/// VSS_GET_SPACE
/// Get current space used and available for dataset.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
enum {
    VSS_GET_SPACE_HANDLE_OFFSET   = 0,
    VSS_GET_SPACE_SIZE       = 4
};

static inline u32 vss_get_space_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_GET_SPACE_HANDLE_OFFSET])));
}

static inline void vss_get_space_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_GET_SPACE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_GET_SPACE_REPLY
///    +----------------+----------------+----------------+----------------+
///  0 | DISK_SIZE                                                         |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | DATASET_SIZE                                                      |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | AVAIL_SIZE                                                        |
/// 20 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    DISK_SIZE - Size of entire disk on which dataset is stored.
///    DATASET_SIZE - Size of dataset on disk.
///    AVAIL_SIZE - Size of space available for this dataset.
enum {
    VSS_GET_SPACER_DISK_SIZE_OFFSET      =  0,
    VSS_GET_SPACER_DATASET_SIZE_OFFSET   =  8,
    VSS_GET_SPACER_AVAIL_SIZE_OFFSET     = 16,
    VSS_GET_SPACER_SIZE       = 24
};

static inline u64 vss_get_space_reply_get_disk_size(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_GET_SPACER_DISK_SIZE_OFFSET])));
}

static inline void vss_get_space_reply_set_disk_size(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_GET_SPACER_DISK_SIZE_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_get_space_reply_get_dataset_size(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_GET_SPACER_DATASET_SIZE_OFFSET])));
}

static inline void vss_get_space_reply_set_dataset_size(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_GET_SPACER_DATASET_SIZE_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_get_space_reply_get_avail_size(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_GET_SPACER_AVAIL_SIZE_OFFSET])));
}

static inline void vss_get_space_reply_set_avail_size(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_GET_SPACER_AVAIL_SIZE_OFFSET])) = VPLConv_hton_u64(val);
}

/// VSS_SET_LOCK
/// Set locking mode for a file.
/// Also used to unlock when a previously set mode bit is cleared.
///    +----------------+----------------+----------------+----------------+
///  0 | MODE                                                              |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | OBJHANDLE                                                         |
///    +----------------+----------------+----------------+----------------+
/// 12 | FILEHANDLE                                                        |
///    +----------------+----------------+----------------+----------------+
///    MODE - Bitmask of lock modes.
///    OBJHANDLE - Object handle from OPEN command.
///    FHANDLE - File handle from OPEN_FILE command.
enum {
    VSS_SET_LOCK_MODE_OFFSET      =  0,
    VSS_SET_LOCK_OBJHANDLE_OFFSET =  8,
    VSS_SET_LOCK_FHANDLE_OFFSET   = 12,
    VSS_SET_LOCK_SIZE             = 16
};

static inline u64 vss_set_lock_get_mode(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_LOCK_MODE_OFFSET])));
}

static inline void vss_set_lock_set_mode(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_SET_LOCK_MODE_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_set_lock_get_objhandle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_LOCK_OBJHANDLE_OFFSET])));
}

static inline void vss_set_lock_set_objhandle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_LOCK_OBJHANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_set_lock_get_fhandle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_LOCK_FHANDLE_OFFSET])));
}

static inline void vss_set_lock_set_fhandle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_LOCK_FHANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_SET_LOCK_REPLY
/// Just VSS header with status

/// VSS_GET_LOCK
/// Get the current locking mode for a file.
///    +----------------+----------------+----------------+----------------+
///  0 | OBJHANDLE                                                         |
///    +----------------+----------------+----------------+----------------+
///  4 | FILEHANDLE                                                        |
///    +----------------+----------------+----------------+----------------+
///    OBJHANDLE - Object handle from OPEN command.
///    FHANDLE - File handle from OPEN_FILE command.
enum {
    VSS_GET_LOCK_OBJHANDLE_OFFSET = 0,
    VSS_GET_LOCK_FHANDLE_OFFSET   = 4,
    VSS_GET_LOCK_SIZE             = 8
};

static inline u32 vss_get_lock_get_objhandle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_GET_LOCK_OBJHANDLE_OFFSET])));
}

static inline void vss_get_lock_set_objhandle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_GET_LOCK_OBJHANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_get_lock_get_fhandle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_GET_LOCK_FHANDLE_OFFSET])));
}

static inline void vss_get_lock_set_fhandle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_GET_LOCK_FHANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_GET_LOCK_REPLY
///    Return current lock mode for the file.
///    +----------------+----------------+----------------+----------------+
///  0 | MODE                                                              |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    MODE - Current lock mode in-force for the file.
enum {
    VSS_GET_LOCKR_MODE_OFFSET   = 0,
    VSS_GET_LOCKR_SIZE = 8
};

static inline u64 vss_get_lock_reply_get_mode(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_GET_LOCKR_MODE_OFFSET])));
}

static inline void vss_get_lock_reply_set_mode(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_GET_LOCKR_MODE_OFFSET])) = VPLConv_hton_u64(val);
}

/// VSS_SET_LOCK_RANGE
/// Set locking mode for a file-range.
/// Also used to unlock when a previously set mode bit is cleared.
///    +----------------+----------------+----------------+----------------+
///  0 | ORIGINATOR ID                                                     |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | START                                                             |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | END                                                               |
/// 20 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 24 | MODE                                                              |
/// 28 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 32 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
/// 36 | FLAGS                                                             |
///    +----------------+----------------+----------------+----------------+
///    ORIGIN - Unique ID of originator of the request.
///    START - Offset of first byte in lock range.
///    END - Offset of first byte after lock range.
///    MODE - Bitmask of lock_range modes.
///    HANDLE - File handle from OPEN_FILE command.
///    FLAGS - Flag bit mask
enum {
    VSS_SET_LOCK_RANGE_ORIGIN_OFFSET =  0,
    VSS_SET_LOCK_RANGE_START_OFFSET  =  8,
    VSS_SET_LOCK_RANGE_END_OFFSET    = 16,
    VSS_SET_LOCK_RANGE_MODE_OFFSET   = 24,
    VSS_SET_LOCK_RANGE_HANDLE_OFFSET = 32,
    VSS_SET_LOCK_RANGE_FLAGS_OFFSET  = 36,
    VSS_SET_LOCK_RANGE_SIZE          = 40
};

static inline u64 vss_set_lock_range_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_LOCK_RANGE_ORIGIN_OFFSET])));
}

static inline void vss_set_lock_range_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_SET_LOCK_RANGE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_set_lock_range_get_start(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_LOCK_RANGE_START_OFFSET])));
}

static inline void vss_set_lock_range_set_start(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_SET_LOCK_RANGE_START_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_set_lock_range_get_end(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_LOCK_RANGE_END_OFFSET])));
}

static inline void vss_set_lock_range_set_end(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_SET_LOCK_RANGE_END_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_set_lock_range_get_mode(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_LOCK_RANGE_MODE_OFFSET])));
}

static inline void vss_set_lock_range_set_mode(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_SET_LOCK_RANGE_MODE_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_set_lock_range_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_LOCK_RANGE_HANDLE_OFFSET])));
}

static inline void vss_set_lock_range_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_LOCK_RANGE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_set_lock_range_get_flags(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_LOCK_RANGE_FLAGS_OFFSET])));
}

static inline void vss_set_lock_range_set_flags(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_LOCK_RANGE_FLAGS_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_SET_LOCK_RANGE_REPLY
/// Just VSS Header with status


/// VSS_GET_NOTIFY
/// Get current async notice settings.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
enum {
    VSS_GET_NOTIFY_HANDLE_OFFSET   = 0,
    VSS_GET_NOTIFY_SIZE       = 4
};

static inline u32 vss_get_notify_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_GET_NOTIFY_HANDLE_OFFSET])));
}

static inline void vss_get_notify_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_GET_NOTIFY_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_GET_NOTIFY_REPLY
///    Return current lock mode for the file.
///    +----------------+----------------+----------------+----------------+
///  0 | MASK                                                              |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    MASK - Currently enabled notifications bitmask.
///      NOTIFY_CHANGE    (0x1)   Dataset has been changed
///      NOTIFY_DISK_FULL (0x10)  Disk is full
///      NOTIFY_LOCK      (0x100) A lock needs attention
enum {
    VSS_GET_NOTIFYR_MASK_OFFSET   = 0,
    VSS_GET_NOTIFYR_SIZE = 8
};
static inline u64 vss_get_notify_reply_get_mode(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_GET_NOTIFYR_MASK_OFFSET])));
}

static inline void vss_get_notify_reply_set_mode(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_GET_NOTIFYR_MASK_OFFSET])) = VPLConv_hton_u64(val);
}

/// VSS_SET_NOTIFY
/// Set async notices requested for a dataset.
/// Also used to turn off notifications by clearing a previously set bit.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | MASK                                                              |
///  8 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - Object handle from OPEN command.
///    MASK - Bitmask of notifications requested.
enum {
    VSS_SET_NOTIFY_HANDLE_OFFSET   = 0,
    VSS_SET_NOTIFY_MASK_OFFSET     = 4,
    VSS_SET_NOTIFY_SIZE       = 12
};

static inline u32 vss_set_notify_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_SET_NOTIFY_HANDLE_OFFSET])));
}

static inline void vss_set_notify_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_SET_NOTIFY_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_set_notify_get_mode(const char* buf)
{
#if defined(__CLOUDNODE__)
    return vss_get_u64(&(buf[VSS_SET_NOTIFY_MASK_OFFSET]));
#else
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_SET_NOTIFY_MASK_OFFSET])));
#endif
}

static inline void vss_set_notify_set_mode(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_SET_NOTIFY_MASK_OFFSET]));
#else
    *((u64*)&(buf[VSS_SET_NOTIFY_MASK_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

/// VSS_SET_NOTIFY_REPLY
/// Same as VSS_GET_NOTIFY_REPLY.

/// VSS_NOTIFICATION
/// Asynchronous notification sent as a result of VSS_SET_NOTIFY enabling the
/// notification type flag and server event occurred.
///    +----------------+----------------+----------------+----------------+
///  0 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
///  8 | MASK                                                              |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | DATA                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///     HANDLE - Object handle registered for the notification
///     LENGTH - Length of notification data returned
///     MASK - Notification bitmask (only one bit set) indicating type.
///     DATA - Notification data, specific to notification type.
enum {
    VSS_NOTIFICATION_HANDLE_OFFSET   =  0,
    VSS_NOTIFICATION_LENGTH_OFFSET   =  4,
    VSS_NOTIFICATION_MASK_OFFSET     =  8,
    VSS_NOTIFICATION_DATA_OFFSET     = 16,
    VSS_NOTIFICATION_BASE_SIZE       = 16
};

static inline u32 vss_notification_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_NOTIFICATION_HANDLE_OFFSET])));
}

static inline void vss_notification_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_NOTIFICATION_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_notification_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_NOTIFICATION_LENGTH_OFFSET])));
}

static inline void vss_notification_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_NOTIFICATION_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_notification_get_mask(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_NOTIFICATION_MASK_OFFSET])));
}

static inline void vss_notification_set_mask(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_NOTIFICATION_MASK_OFFSET])) = VPLConv_hton_u64(val);
}

static inline char* vss_notification_get_data(const char* buf)
{
    return (char*)&(buf[VSS_NOTIFICATION_DATA_OFFSET]);
}

static inline void vss_notification_set_data(char* buf, const char* val)
{
    memcpy(&(buf[VSS_NOTIFICATION_DATA_OFFSET]), val, vss_notification_get_length(buf));
}

/// VSS_NOTIFICATION_OPLOCK_BREAK
/// Asynchronous notification sent as a result of VSS_SET_NOTIFY enabling the
/// notification type flag and server event occurred.
/// 
/// This is the layout of the additional returned data on an oplock break
/// VSS_NOTIFICATION message (starting VSS_NOTIFICATION_DATA_OFFSET).
///    +----------------+----------------+----------------+----------------+
///  0 | MODE                                                              |
///  4 | 
///    +----------------+----------------+----------------+----------------+
///  8 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///     MASK - Notification bitmask (only one bit set) indicating type.
///     HANDLE - Object handle registered for the notification
enum {
    VSS_OPLOCK_BREAK_MODE_OFFSET     =  0,
    VSS_OPLOCK_BREAK_HANDLE_OFFSET   =  8,
    VSS_OPLOCK_BREAK_SIZE            = 12,
};

static inline u64 vss_oplock_break_get_mode(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_OPLOCK_BREAK_MODE_OFFSET])));
}

static inline void vss_oplock_break_set_mode(char* buf, u64 val)
{
#if defined(__CLOUDNODE__)
    vss_set_u64(val, &(buf[VSS_OPLOCK_BREAK_MODE_OFFSET]));
#else
    *((u64*)&(buf[VSS_OPLOCK_BREAK_MODE_OFFSET])) = VPLConv_hton_u64(val);
#endif
}

static inline u32 vss_oplock_break_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_OPLOCK_BREAK_HANDLE_OFFSET])));
}

static inline void vss_oplock_break_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_OPLOCK_BREAK_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_OPEN_FILE
/// Open a (file) handle to a file
///    +----------------+----------------+---------------------------------+
///  0 | OBJECT_VERSION                                                    |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | ORIGIN
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
/// 20 | FLAGS                                                             |
///    +----------------+----------------+----------------+----------------+
/// 24 | ATTRIBUTES                                                        |
///    +----------------+----------------+----------------+----------------+
/// 28 | NAME_LENGTH                     | NAME                            |
///    +----------------+----------------+                                 +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    OBJECT_VERSION - Version of object being read (last committed version).
///    HANDLE - Object handle from OPEN call.
///    FLAGS - Open mode and sharing mode.
///    NAME_LENGTH - Length of the component file name, including absolute path, to read.
///    NAME - File name and absolute path for NAME_LENGTH bytes.
enum {
    VSS_OPEN_FILE_OBJVER_OFFSET   =  0,
    VSS_OPEN_FILE_ORIGIN_OFFSET   =  8,
    VSS_OPEN_FILE_HANDLE_OFFSET   = 16,
    VSS_OPEN_FILE_FLAGS_OFFSET    = 20,
    VSS_OPEN_FILE_ATTRS_OFFSET    = 24,
    VSS_OPEN_FILE_NAME_LENGTH_OFFSET = 28,
    VSS_OPEN_FILE_NAME_OFFSET     = 30,
    VSS_OPEN_FILE_BASE_SIZE       = 30
};

static inline u32 vss_open_file_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_OPEN_FILE_HANDLE_OFFSET])));
}

static inline void vss_open_file_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_OPEN_FILE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_open_file_get_objver(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_OPEN_FILE_OBJVER_OFFSET])));
}

static inline void vss_open_file_set_objver(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_OPEN_FILE_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_open_file_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_OPEN_FILE_ORIGIN_OFFSET])));
}

static inline void vss_open_file_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_OPEN_FILE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_open_file_get_flags(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_OPEN_FILE_FLAGS_OFFSET])));
}

static inline void vss_open_file_set_flags(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_OPEN_FILE_FLAGS_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_open_file_get_attrs(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_OPEN_FILE_ATTRS_OFFSET])));
}

static inline void vss_open_file_set_attrs(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_OPEN_FILE_ATTRS_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_open_file_get_name_length(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_OPEN_FILE_NAME_LENGTH_OFFSET])));
}

static inline void vss_open_file_set_name_length(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_OPEN_FILE_NAME_LENGTH_OFFSET])) = VPLConv_hton_u16(val);
}

static inline char* vss_open_file_get_name(const char* buf)
{
    return (char*)&(buf[VSS_OPEN_FILE_NAME_OFFSET]);
}

static inline void vss_open_file_set_name(char* buf, const char* val)
{
    memcpy(&(buf[VSS_OPEN_FILE_NAME_OFFSET]), val, vss_open_file_get_name_length(buf));
}

/// VSS_OPEN_FILE_REPLY
/// Return result for open file handle.
///    +----------------+----------------+---------------------------------+
///  0 | OBJECT_VERSION                                                    |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
/// 12 | FLAGS                                                             |
///    +----------------+----------------+----------------+----------------+
///    OBJECT_VERSION - Version of object actually opened (current committed version).
///                     If different than requested, STATUS will indicate a mismatch warning.
///    HANDLE - returned file handle value (if successful)
///    FLAGS - Open mode flags as modified by the server
enum {
    VSS_OPEN_FILER_OBJVER_OFFSET   = 0,
    VSS_OPEN_FILER_HANDLE_OFFSET   = 8,
    VSS_OPEN_FILER_FLAGS_OFFSET    = 12,
    VSS_OPEN_FILER_BASE_SIZE       = 16
};

static inline u32 vss_open_file_reply_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_OPEN_FILER_HANDLE_OFFSET])));
}

static inline void vss_open_file_reply_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_OPEN_FILER_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_open_file_reply_get_objver(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_OPEN_FILER_OBJVER_OFFSET])));
}

static inline void vss_open_file_reply_set_objver(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_OPEN_FILER_OBJVER_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_open_file_reply_get_flags(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_OPEN_FILER_FLAGS_OFFSET])));
}

static inline void vss_open_file_reply_set_flags(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_OPEN_FILER_FLAGS_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_CLOSE_FILE
/// Close a (file) handle to a file
///    +----------------+----------------+---------------------------------+
///  0 | ORIGINATOR ID                                                     |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - File handle from OPEN_FILE call.
enum {
    VSS_CLOSE_FILE_ORIGIN_OFFSET   =  0,
    VSS_CLOSE_FILE_HANDLE_OFFSET   =  8,
    VSS_CLOSE_FILE_SIZE            = 12
};

static inline u64 vss_close_file_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_CLOSE_FILE_ORIGIN_OFFSET])));
}

static inline void vss_close_file_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_CLOSE_FILE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_close_file_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CLOSE_FILE_HANDLE_OFFSET])));
}

static inline void vss_close_file_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CLOSE_FILE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_CLOSE_FILE_REPLY
/// Return for CLOSE_FILE command
/// Returns VSS_VERSION_REPLY with status and new object version.

/// VSS_READ_FILE
/// Read data from an open file
///    +----------------+----------------+----------------+----------------+
///  0 | ORIGINATOR ID                                                     |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | OFFSET                                                            |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | SERVER FILE HANDLE                                                |
///    +----------------+----------------+---------------------------------+
/// 20 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
///    ORIGIN - Unique ID of originator of the request.
///    OFFSET - Byte offset into file for read.
///    HANDLE - File handle returned by server from OPEN_FILE command.
///    LENGTH - Length of data to read, in bytes.
enum {
    VSS_READ_FILE_ORIGIN_OFFSET   =  0,
    VSS_READ_FILE_OFFSET_OFFSET   =  8,
    VSS_READ_FILE_HANDLE_OFFSET   = 16,
    VSS_READ_FILE_LENGTH_OFFSET   = 20,
    VSS_READ_FILE_SIZE            = 24
};

static inline u32 vss_read_file_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_FILE_HANDLE_OFFSET])));
}

static inline void vss_read_file_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_FILE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_read_file_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_FILE_ORIGIN_OFFSET])));
}

static inline void vss_read_file_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_READ_FILE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_read_file_get_offset(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_READ_FILE_OFFSET_OFFSET])));
}

static inline void vss_read_file_set_offset(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_READ_FILE_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_read_file_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_FILE_LENGTH_OFFSET])));
}

static inline void vss_read_file_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_FILE_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_READ_FILE_REPLY
/// Return result for read.
///    +----------------+----------------+---------------------------------+
///  0 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// ?? | DATA                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    LENGTH - Length of data successfully read from file starting from OFFSET, in bytes.
///    DATA - LENGTH bytes of data returned from the file starting at OFFSET.
enum {
    VSS_READ_FILER_LENGTH_OFFSET = 0,
    VSS_READ_FILER_BASE_SIZE     = 4
};

static inline u32 vss_read_file_reply_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_READ_FILER_LENGTH_OFFSET])));
}

static inline void vss_read_file_reply_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_READ_FILER_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline char* vss_read_file_reply_get_data(const char* buf)
{
    return (char*)&(buf[VSS_READ_FILER_BASE_SIZE]);
}

/// VSS_WRITE_FILE
/// Write data to an open writable file
///    +----------------+----------------+----------------+----------------+
///  0 | ORIGINATOR ID                                                     |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | OFFSET                                                            |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | FILE HANDLE                                                       |
///    +----------------+----------------+---------------------------------+
/// 20 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
/// ?? | DATA                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    ORIGIN - Unique ID of originator of the request.
///    OFFSET - Byte offset into file for write.
///    HANDLE - File handle from OPEN_FILE command.
///    LENGTH - Length of data to write, in bytes.
///    DATA - LENGTH bytes of data to write to the file starting at OFFSET.
enum {
    VSS_WRITE_FILE_ORIGIN_OFFSET   =  0,
    VSS_WRITE_FILE_OFFSET_OFFSET   =  8,
    VSS_WRITE_FILE_HANDLE_OFFSET   = 16,
    VSS_WRITE_FILE_LENGTH_OFFSET   = 20,
    VSS_WRITE_FILE_BASE_SIZE       = 24
};

static inline u32 vss_write_file_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_WRITE_FILE_HANDLE_OFFSET])));
}

static inline void vss_write_file_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_WRITE_FILE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u64 vss_write_file_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_WRITE_FILE_ORIGIN_OFFSET])));
}

static inline void vss_write_file_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_WRITE_FILE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_write_file_get_offset(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_WRITE_FILE_OFFSET_OFFSET])));
}

static inline void vss_write_file_set_offset(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_WRITE_FILE_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_write_file_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_WRITE_FILE_LENGTH_OFFSET])));
}

static inline void vss_write_file_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_WRITE_FILE_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline char* vss_write_file_get_data(const char* buf)
{
    return (char*)&(buf[VSS_WRITE_FILE_BASE_SIZE]);
}

/// VSS_WRITE_FILE_REPLY
/// Return result for write.
///    +----------------+----------------+---------------------------------+
///  0 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
///    LENGTH - Length of data successfully written to file starting from OFFSET, in bytes.
enum {
    VSS_WRITE_FILER_LENGTH_OFFSET = 0,
    VSS_WRITE_FILER_SIZE          = 4
};

static inline u32 vss_write_file_reply_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_WRITE_FILER_LENGTH_OFFSET])));
}

static inline void vss_write_file_reply_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_WRITE_FILER_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_TRUNCATE_FILE
/// Truncate an open file at a specific offset
///    +----------------+----------------+----------------+----------------+
///  0 | ORIGINATOR ID                                                     |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | OFFSET                                                            |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// 16 | SERVER FILE HANDLE                                                |
///    +----------------+----------------+---------------------------------+
///    ORIGIN - Unique ID of originator of the request.
///    OFFSET - Byte offset into file at which to truncate (new length of file).
///    HANDLE - File handle returned by server from OPEN_FILE command.
enum {
    VSS_TRUNCATE_FILE_ORIGIN_OFFSET   =  0,
    VSS_TRUNCATE_FILE_OFFSET_OFFSET   =  8,
    VSS_TRUNCATE_FILE_HANDLE_OFFSET   = 16,
    VSS_TRUNCATE_FILE_SIZE            = 20
};

static inline u64 vss_truncate_file_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_TRUNCATE_FILE_ORIGIN_OFFSET])));
}

static inline void vss_truncate_file_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_TRUNCATE_FILE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u64 vss_truncate_file_get_offset(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_TRUNCATE_FILE_OFFSET_OFFSET])));
}

static inline void vss_truncate_file_set_offset(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_TRUNCATE_FILE_OFFSET_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_truncate_file_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_TRUNCATE_FILE_HANDLE_OFFSET])));
}

static inline void vss_truncate_file_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_TRUNCATE_FILE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_TRUNCATE_FILE_REPLY
/// Just the VSS header with STATUS indicating success or failure.
enum {
    VSS_TRUNCATE_FILER_SIZE = 0
};

/// VSS_CHMOD_FILE
/// Truncate an open file at a specific offset
///    +----------------+----------------+----------------+----------------+
///  0 | ORIGINATOR ID                                                     |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | ATTRS                                                             |
///    +----------------+----------------+----------------+----------------+
/// 12 | ATTRS_MASK                                                        |
///    +----------------+----------------+---------------------------------+
/// 16 | SERVER FILE HANDLE                                                |
///    +----------------+----------------+---------------------------------+
///    ORIGIN - Unique ID of originator of the request.
///    ATTRS - Object attributes to be applied
///    ATTRS_MASK - Attribute mask to control clearing/setting of bits.
///    HANDLE - File handle returned by server from OPEN_FILE command.
enum {
    VSS_CHMOD_FILE_ORIGIN_OFFSET        =  0,
    VSS_CHMOD_FILE_ATTRS_OFFSET         =  8,
    VSS_CHMOD_FILE_ATTRS_MASK_OFFSET    = 12,
    VSS_CHMOD_FILE_HANDLE_OFFSET        = 16,
    VSS_CHMOD_FILE_SIZE                 = 20
};

static inline u64 vss_chmod_file_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_CHMOD_FILE_ORIGIN_OFFSET])));
}

static inline void vss_chmod_file_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_CHMOD_FILE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_chmod_file_get_attrs(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CHMOD_FILE_ATTRS_OFFSET])));
}

static inline void vss_chmod_file_set_attrs(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CHMOD_FILE_ATTRS_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_chmod_file_get_attrs_mask(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CHMOD_FILE_ATTRS_MASK_OFFSET])));
}

static inline void vss_chmod_file_set_attrs_mask(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CHMOD_FILE_ATTRS_MASK_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_chmod_file_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_CHMOD_FILE_HANDLE_OFFSET])));
}

static inline void vss_chmod_file_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_CHMOD_FILE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_CHMOD_FILE_REPLY
/// Just the VSS header with STATUS indicating success or failure.
enum {
    VSS_CHMOD_FILER_SIZE = 0
};

/// VSS_RELEASE_FILE
/// Release a (file) handle to a file
///    +----------------+----------------+---------------------------------+
///  0 | ORIGINATOR ID                                                     |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | HANDLE                                                            |
///    +----------------+----------------+----------------+----------------+
///    HANDLE - File handle from OPEN_FILE call.
enum {
    VSS_RELEASE_FILE_ORIGIN_OFFSET   =  0,
    VSS_RELEASE_FILE_HANDLE_OFFSET   =  8,
    VSS_RELEASE_FILE_SIZE            = 12
};

static inline u64 vss_release_file_get_origin(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_RELEASE_FILE_ORIGIN_OFFSET])));
}

static inline void vss_release_file_set_origin(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_RELEASE_FILE_ORIGIN_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_release_file_get_handle(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_RELEASE_FILE_HANDLE_OFFSET])));
}

static inline void vss_release_file_set_handle(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_RELEASE_FILE_HANDLE_OFFSET])) = VPLConv_hton_u32(val);
}

/// VSS_RELEASE_FILE_REPLY
/// Return for RELEASE_FILE command
/// Returns VSS_VERSION_REPLY with status and new object version.

/// VSS_NEGOTIATE
/// Request a sub-session with specific security parameters.
/// Must be used with VSSI version 2.
///    +----------------+----------------+----------------+----------------+
///  0 | SIGNING_MODE   | SIGN_TYPE      | ENCRYPT_TYPE   | CHAL_RESP_LEN  |
///    +----------------+----------------+----------------+----------------+
///  4 | CHAL_RESPONSE                                                     |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    SIGNING_MODE - Message signing mode requested.
///      NONE        (0) Do not sign messages.
///      HEADER_ONLY (1) Sign headers but do not sign body
///      FULL        (2) Sign both headers and body
///    SIGN_TYPE - Type of signing algorithm to use
///      NONE        (0) All signatures are zeroes
///      SHA1        (1) Use SHA-1 HMAC
///    ENCRYPT_TYPE - Type of encryption to use for body data
///      NONE        (0) No encryption
///      AES128      (1) Use AES-128 encryption0
///    CHAL_RESP_LEN - Length of challenge-response data included.
///    CHAL_RESPONSE - Challenge response data, up to 255 bytes.
enum {
    VSS_NEGOTIATE_SIGNING_MODE_OFFSET  =  0,
      VSS_NEGOTIATE_SIGNING_MODE_NONE        = 0,
      VSS_NEGOTIATE_SIGNING_MODE_HEADER_ONLY = 1,
      VSS_NEGOTIATE_SIGNING_MODE_FULL        = 2,
    VSS_NEGOTIATE_SIGN_TYPE_OFFSET     =  1,
      VSS_NEGOTIATE_SIGN_TYPE_NONE           = 0,
      VSS_NEGOTIATE_SIGN_TYPE_SHA1           = 1,
    VSS_NEGOTIATE_ENCRYPT_TYPE_OFFSET  =  2,
      VSS_NEGOTIATE_ENCRYPT_TYPE_NONE        = 0,
      VSS_NEGOTIATE_ENCRYPT_TYPE_AES128      = 1,
    VSS_NEGOTIATE_CHAL_RESP_LEN_OFFSET =  3,
    VSS_NEGOTIATE_CHAL_RESPONSE_OFFSET =  4,
    VSS_NEGOTIATE_BASE_SIZE = 4
};

static inline u8 vss_negotiate_get_signing_mode(const char* buf)
{
    return buf[VSS_NEGOTIATE_SIGNING_MODE_OFFSET];
}

static inline void vss_negotiate_set_signing_mode(char* buf, u8 val)
{
    buf[VSS_NEGOTIATE_SIGNING_MODE_OFFSET] = val;
}

static inline u8 vss_negotiate_get_sign_type(const char* buf)
{
    return buf[VSS_NEGOTIATE_SIGN_TYPE_OFFSET];
}

static inline void vss_negotiate_set_sign_type(char* buf, u8 val)
{
    buf[VSS_NEGOTIATE_SIGN_TYPE_OFFSET] = val;
}

static inline u8 vss_negotiate_get_encrypt_type(const char* buf)
{
    return buf[VSS_NEGOTIATE_ENCRYPT_TYPE_OFFSET];
}

static inline void vss_negotiate_set_encrypt_type(char* buf, u8 val)
{
    buf[VSS_NEGOTIATE_ENCRYPT_TYPE_OFFSET] = val;
}

static inline u8 vss_negotiate_get_chal_resp_len(const char* buf)
{
    return buf[VSS_NEGOTIATE_CHAL_RESP_LEN_OFFSET];
}

static inline void vss_negotiate_set_chal_resp_len(char* buf, u8 val)
{
    buf[VSS_NEGOTIATE_CHAL_RESP_LEN_OFFSET] = val;
}

static inline char* vss_negotiate_get_chal_response(const char* buf)
{
    return (char*)&(buf[VSS_NEGOTIATE_CHAL_RESPONSE_OFFSET]);
}

static inline void vss_negotiate_set_chal_response(char* buf, const char* val)
{
    memcpy(&(buf[VSS_NEGOTIATE_CHAL_RESPONSE_OFFSET]), val, vss_negotiate_get_chal_resp_len(buf));
}

/// VSS_NEGOTIATE_REPLY
///    +----------------+----------------+----------------+----------------+
///  0 | XID_START                                                         |
///    +----------------+----------------+----------------+----------------+
///  4 | SIGNING_MODE   | SIGN_TYPE      | ENCRYPT_TYPE   | CHALLENGE_LEN  |
///    +----------------+----------------+----------------+----------------+
///  8 | CHALLENGE                                                         |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    XID_START - Next XID to use for this sub-session.
///    SIGNING_MODE - Message signing mode requested.
///      NONE        (0) Do not sign messages.
///      HEADER_ONLY (1) Sign headers but do not sign body
///      FULL        (2) Sign both headers and body
///    SIGN_TYPE - Type of signing algorithm to use
///      NONE        (0) All signatures are zeroes
///      SHA1        (1) Use SHA-1 HMAC
///    ENCRYPT_TYPE - Type of encryption to use for body data
///      NONE        (0) No encryption
///      AES128      (1) Use AES-128 encryption
///    CHALLENGE_LEN - If server is challenging client, nonzero length of challenge data.
///    CHALLENGE - Challenge data, up to 255 bytes.
enum {
    VSS_NEGOTIATER_XID_START_OFFSET     =  0,
    VSS_NEGOTIATER_SIGNING_MODE_OFFSET  =  4,
    VSS_NEGOTIATER_SIGN_TYPE_OFFSET     =  5,
    VSS_NEGOTIATER_ENCRYPT_TYPE_OFFSET  =  6,
    VSS_NEGOTIATER_CHALLENGE_LEN_OFFSET =  7,
    VSS_NEGOTIATER_CHALLENGE_OFFSET     =  8,
    VSS_NEGOTIATER_BASE_SIZE = 8
};

static inline u32 vss_negotiate_reply_get_xid_start(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_NEGOTIATER_XID_START_OFFSET])));
}

static inline void vss_negotiate_reply_set_xid_start(char* buf, u32 val)
{
    *((u64*)&(buf[VSS_NEGOTIATER_XID_START_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u8 vss_negotiate_reply_get_signing_mode(const char* buf)
{
    return buf[VSS_NEGOTIATER_SIGNING_MODE_OFFSET];
}

static inline void vss_negotiate_reply_set_signing_mode(char* buf, u8 val)
{
    buf[VSS_NEGOTIATER_SIGNING_MODE_OFFSET] = val;
}

static inline u8 vss_negotiate_reply_get_sign_type(const char* buf)
{
    return buf[VSS_NEGOTIATER_SIGN_TYPE_OFFSET];
}

static inline void vss_negotiate_reply_set_sign_type(char* buf, u8 val)
{
    buf[VSS_NEGOTIATER_SIGN_TYPE_OFFSET] = val;
}

static inline u8 vss_negotiate_reply_get_encrypt_type(const char* buf)
{
    return buf[VSS_NEGOTIATER_ENCRYPT_TYPE_OFFSET];
}

static inline void vss_negotiate_reply_set_encrypt_type(char* buf, u8 val)
{
    buf[VSS_NEGOTIATER_ENCRYPT_TYPE_OFFSET] = val;
}

static inline u8 vss_negotiate_reply_get_challenge_len(const char* buf)
{
    return buf[VSS_NEGOTIATER_CHALLENGE_LEN_OFFSET];
}

static inline void vss_negotiate_reply_set_challenge_len(char* buf, u8 val)
{
    buf[VSS_NEGOTIATER_CHALLENGE_LEN_OFFSET] = val;
} 

static inline char* vss_negotiate_reply_get_challenge(const char* buf)
{
    return (char*)&(buf[VSS_NEGOTIATER_CHALLENGE_OFFSET]);
}

static inline void vss_negotiate_reply_set_challenge(char* buf, const char* val)
{
    memcpy(&(buf[VSS_NEGOTIATER_CHALLENGE_OFFSET]), val, vss_negotiate_reply_get_challenge_len(buf));
}

/// VSS_AUTHENTICATE
/// Request authentication of the server. Used for PSN nodes to make sure the
/// connection goes to the intended destination.
/// Must be the first command sent on a connection.
///    +----------------+----------------+----------------+----------------+
///  0 | CLUSTER_ID                                                        |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | EXT_VERSION                                                       |
///    +----------------+----------------+----------------+----------------+
/// 12 | RESERVED                                                          |
///    +----------------+----------------+----------------+----------------+
/// 16 | SIGNING_MODE   | SIGN_TYPE      | ENCRYPT_TYPE   |
///    +----------------+----------------+----------------+
///    CLUSTER_ID - Cluster ID of true target of this connection.
///    EXT_VERSION - (v1) For authenticate reqeusts beyond version 0, version of the request format.
///    RESERVED - Reserved bytes. Set to 0. (needed to make v1 encrypted length larger than v0)
///    SIGNING_MODE - (v1) Message signing mode requested. (See VSS_NEGOTIATE above)
///    SIGN_TYPE - (v1) Type of signing algorithm to use. (See VSS_NEGOTIATE above)
///    ENCRYPT_TYPE - (v1) Type of encryption to use for body data. (See VSS_NEGOTIATE above)
enum {
    VSS_AUTHENTICATE_CLUSTER_ID_OFFSET   =  0,
    VSS_AUTHENTICATE_EXT_VERSION_OFFSET  =  8,
    VSS_AUTHENTICATE_SIGNING_MODE_OFFSET = 16,
    VSS_AUTHENTICATE_SIGN_TYPE_OFFSET    = 17,
    VSS_AUTHENTICATE_ENCRYPT_TYPE_OFFSET = 18,
    VSS_AUTHENTICATE_SIZE_EXT_VER_0      =  8,
    VSS_AUTHENTICATE_SIZE_EXT_VER_1      = 19
};

static inline u64 vss_authenticate_get_cluster_id(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_AUTHENTICATE_CLUSTER_ID_OFFSET])));
}

static inline void vss_authenticate_set_cluster_id(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_AUTHENTICATE_CLUSTER_ID_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u32 vss_authenticate_get_ext_version(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_AUTHENTICATE_EXT_VERSION_OFFSET])));
}

static inline void vss_authenticate_set_ext_version(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_AUTHENTICATE_EXT_VERSION_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u8 vss_authenticate_get_signing_mode(const char* buf)
{
    return buf[VSS_AUTHENTICATE_SIGNING_MODE_OFFSET];
}

static inline void vss_authenticate_set_signing_mode(char* buf, u8 val)
{
    buf[VSS_AUTHENTICATE_SIGNING_MODE_OFFSET] = val;
}

static inline u8 vss_authenticate_get_sign_type(const char* buf)
{
    return buf[VSS_AUTHENTICATE_SIGN_TYPE_OFFSET];
}

static inline void vss_authenticate_set_sign_type(char* buf, u8 val)
{
    buf[VSS_AUTHENTICATE_SIGN_TYPE_OFFSET] = val;
}

static inline u8 vss_authenticate_get_encrypt_type(const char* buf)
{
    return buf[VSS_AUTHENTICATE_ENCRYPT_TYPE_OFFSET];
}

static inline void vss_authenticate_set_encrypt_type(char* buf, u8 val)
{
    buf[VSS_AUTHENTICATE_ENCRYPT_TYPE_OFFSET] = val;
}

/// VSS_AUTHENTICATE_REPLY
/// Return for AUTHENTICATE command
/// (v0)
///  There is no fixed or variable data. STATUS indicates success or failure.
/// (v1)
///    +----------------+----------------+----------------+----------------+
///  0 | EXT_VERSION                                                       |
///    +----------------+----------------+----------------+----------------+
///  4 | SIGNING_MODE   | SIGN_TYPE      | ENCRYPT_TYPE   |
///    +----------------+----------------+----------------+
///    EXT_VERSION - For authenticate reqeusts beyond version 0, version of the request format.
///    SIGNING_MODE - Message signing mode requested. (See VSS_NEGOTIATE above)
///    SIGN_TYPE - Type of signing algorithm to use. (See VSS_NEGOTIATE above)
///    ENCRYPT_TYPE - Type of encryption to use for body data. (See VSS_NEGOTIATE above)
enum {
    VSS_AUTHENTICATER_EXT_VERSION_OFFSET  =  0,
    VSS_AUTHENTICATER_SIGNING_MODE_OFFSET =  4,
    VSS_AUTHENTICATER_SIGN_TYPE_OFFSET    =  5,
    VSS_AUTHENTICATER_ENCRYPT_TYPE_OFFSET =  6,
    VSS_AUTHENTICATER_SIZE_EXT_VER_0      =  0,
    VSS_AUTHENTICATER_SIZE_EXT_VER_1      =  7
};

static inline u32 vss_authenticate_reply_get_ext_version(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_AUTHENTICATER_EXT_VERSION_OFFSET])));
}

static inline void vss_authenticate_reply_set_ext_version(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_AUTHENTICATER_EXT_VERSION_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u8 vss_authenticate_reply_get_signing_mode(const char* buf)
{
    return buf[VSS_AUTHENTICATER_SIGNING_MODE_OFFSET];
}

static inline void vss_authenticate_reply_set_signing_mode(char* buf, u8 val)
{
    buf[VSS_AUTHENTICATER_SIGNING_MODE_OFFSET] = val;
}

static inline u8 vss_authenticate_reply_get_sign_type(const char* buf)
{
    return buf[VSS_AUTHENTICATER_SIGN_TYPE_OFFSET];
}

static inline void vss_authenticate_reply_set_sign_type(char* buf, u8 val)
{
    buf[VSS_AUTHENTICATER_SIGN_TYPE_OFFSET] = val;
}

static inline u8 vss_authenticate_reply_get_encrypt_type(const char* buf)
{
    return buf[VSS_AUTHENTICATER_ENCRYPT_TYPE_OFFSET];
}

static inline void vss_authenticate_reply_set_encrypt_type(char* buf, u8 val)
{
    buf[VSS_AUTHENTICATER_ENCRYPT_TYPE_OFFSET] = val;
}

/// VSS_TUNNEL_DATA
/// Request to have data tunnelled to recipient using VSSI security.
/// Used by clients and servers to securely pass foriegn data over VSSI.
/// Think of the tunnel as a non-negotiated SSL connection.
///    +----------------+----------------+----------------+----------------+
///  0 | LENGTH                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | DATA                                                              |
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    LENGTH - Length of data in this block, in bytes. Max is 1MB.
///    DATA - Data being tunneled.
enum {
    VSS_TUNNEL_DATA_LENGTH_OFFSET = 0,
    VSS_TUNNEL_DATA_DATA_OFFSET = 4,
    VSS_TUNNEL_DATA_BASE_SIZE = 4
};

static inline u32 vss_tunnel_data_get_length(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_TUNNEL_DATA_LENGTH_OFFSET])));
}

static inline void vss_tunnel_data_set_length(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_TUNNEL_DATA_LENGTH_OFFSET])) = VPLConv_hton_u32(val);
}

static inline char* vss_tunnel_data_get_data(const char* buf)
{
    return (char*)&(buf[VSS_TUNNEL_DATA_DATA_OFFSET]);
}

/// VSS_TUNNEL_DATA_REPLY
/// Not used.

/// VSS_PROXY_REQUEST
/// Request to have connection proxied through to another destination.
/// Used by clients when requesting proxy route through infrastructure to a
/// personal storage node.
/// Must be the first command sent on a connection.
///    +----------------+----------------+----------------+----------------+
///  0 | CLUSTER_ID                                                        |
///  4 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///  8 | PORT                            | TYPE           |
///    +----------------+----------------+----------------+
///    CLUSTER_ID - Cluster ID of true target of this connection.
///    PORT - Client port open for peer-to-peer connection.
///    TYPE - Type of connection to make. See VSS_PROXY_CONNECT for types.
enum {
    VSS_PROXY_REQUEST_CLUSTER_ID_OFFSET   = 0,
    VSS_PROXY_REQUEST_PORT_OFFSET         = 8,
    VSS_PROXY_REQUEST_TYPE_OFFSET         = 10,
    VSS_PROXY_REQUEST_SIZE                = 11
};

static inline u64 vss_proxy_request_get_cluster_id(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_PROXY_REQUEST_CLUSTER_ID_OFFSET])));
}

static inline void vss_proxy_request_set_cluster_id(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_PROXY_REQUEST_CLUSTER_ID_OFFSET])) = VPLConv_hton_u64(val);
}

static inline u16 vss_proxy_request_get_port(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_PROXY_REQUEST_PORT_OFFSET])));
}

static inline void vss_proxy_request_set_port(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_PROXY_REQUEST_PORT_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u8 vss_proxy_request_get_type(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_PROXY_REQUEST_TYPE_OFFSET])));
}

static inline void vss_proxy_request_set_type(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_PROXY_REQUEST_TYPE_OFFSET])) = VPLConv_hton_u8(val);
}

/// VSS_PROXY_REQUEST_REPLY
///    +----------------+----------------+----------------+----------------+
///  0 | DESTINATION_IP                                                    |
///    +----------------+----------------+----------------+----------------+
///  4 | PORT                            |
///    +----------------+----------------+
///    DESTINATION_IP - IPv4 address of destination as detected by proxy.
///                     May be used to attempt peer-to-peer connection.
///    PORT - Port open by destination for peer-to-peer connection.
enum {
    VSS_PROXY_REQUESTR_DESTINATION_IP_OFFSET   = 0,
    VSS_PROXY_REQUESTR_PORT_OFFSET             = 4,
    VSS_PROXY_REQUESTR_SIZE                    = 6
};

static inline u32 vss_proxy_request_reply_get_destination_ip(const char* buf)
{
    // Always network byte order
    return *((u32*)&(buf[VSS_PROXY_REQUESTR_DESTINATION_IP_OFFSET]));
}

static inline void vss_proxy_request_reply_set_destination_ip(char* buf, u32 val)
{
    // Always network byte order
    *((u32*)&(buf[VSS_PROXY_REQUESTR_DESTINATION_IP_OFFSET])) = val;
}

static inline u16 vss_proxy_request_reply_get_port(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_PROXY_REQUESTR_PORT_OFFSET])));
}

static inline void vss_proxy_request_reply_set_port(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_PROXY_REQUESTR_PORT_OFFSET])) = VPLConv_hton_u16(val);
}

/// VSS_PROXY_CONNECT
/// Request for device to connect to proxy server to complete proxy connection
/// from a client.
/// Used between proxy server (VSS) and personal storage node to request PSN
/// to connect to the proxy server so client traffic may be forwarded.
/// Command is sent via ANS.
///    +----------------+----------------+----------------+----------------+
///  0 | COOKIE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | CLIENT_IP                                                         |
///    +----------------+----------------+----------------+----------------+
///  8 | CLIENT_PORT                     | SERVER_PORT                     |
///    +----------------+----------------+----------------+----------------+
/// 12 | ADDRLEN                         | TYPE           | SERVER_ADDR    |
///    +----------------+----------------+----------------+                +
/// ?? |                                                                   |
///    +----------------+----------------+----------------+----------------+
/// COOKIE - Connection cookie, to be sent back to proxy server in reply.
/// CLIENT_IP - IPv4 client address as detected by proxy server.
/// CLIENT_PORT - Client port opened for peer-to-peer connection.
/// SERVER_PORT - Proxy server port.
/// ADDRLEN - Length of proxy server address.
/// TYPE - Type of connection being made.
///   VSSP     (0) - VSSP connection
///   VSS-HTTP (1) - VSS-over-HTTP connection
///   STREAM   (2) - Non-secure generic stream
///   SSTREAM  (3) - Secure tunnel connection
/// SERVER_ADDR - Proxy server address for ADDRLEN bytes.

enum {
    VSS_PROXY_CONNECT_COOKIE_OFFSET       =  0,
    VSS_PROXY_CONNECT_CLIENT_IP_OFFSET     =  4,
    VSS_PROXY_CONNECT_CLIENT_PORT_OFFSET   =  8,
    VSS_PROXY_CONNECT_SERVER_PORT_OFFSET   = 10,
    VSS_PROXY_CONNECT_ADDRLEN_OFFSET      = 12,
    VSS_PROXY_CONNECT_TYPE_OFFSET         = 14,
      VSS_PROXY_CONNECT_TYPE_VSSP     = 0, /// VSSP connection
      VSS_PROXY_CONNECT_TYPE_HTTP     = 1, /// VSS-HTTP connection
      VSS_PROXY_CONNECT_TYPE_STREAM   = 2, /// Data stream connection
      VSS_PROXY_CONNECT_TYPE_SSTREAM  = 3, /// Secure data stream connection
    VSS_PROXY_CONNECT_SERVER_ADDR_OFFSET  = 15,
    VSS_PROXY_CONNECT_BASE_SIZE           = 15
};

static inline u32 vss_proxy_connect_get_cookie(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_PROXY_CONNECT_COOKIE_OFFSET])));
}

static inline void vss_proxy_connect_set_cookie(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_PROXY_CONNECT_COOKIE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u32 vss_proxy_connect_get_client_ip(const char* buf)
{
    // Always network byte order
    return *((u32*)&(buf[VSS_PROXY_CONNECT_CLIENT_IP_OFFSET]));
}

static inline void vss_proxy_connect_set_client_ip(char* buf, u32 val)
{
    // Always network byte order
    *((u32*)&(buf[VSS_PROXY_CONNECT_CLIENT_IP_OFFSET])) = val;
}

static inline u16 vss_proxy_connect_get_client_port(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_PROXY_CONNECT_CLIENT_PORT_OFFSET])));
}

static inline void vss_proxy_connect_set_client_port(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_PROXY_CONNECT_CLIENT_PORT_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u16 vss_proxy_connect_get_server_port(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_PROXY_CONNECT_SERVER_PORT_OFFSET])));
}

static inline void vss_proxy_connect_set_server_port(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_PROXY_CONNECT_SERVER_PORT_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u16 vss_proxy_connect_get_addrlen(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_PROXY_CONNECT_ADDRLEN_OFFSET])));
}

static inline void vss_proxy_connect_set_addrlen(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_PROXY_CONNECT_ADDRLEN_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u8 vss_proxy_connect_get_type(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_PROXY_CONNECT_TYPE_OFFSET])));
}

static inline void vss_proxy_connect_set_type(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_PROXY_CONNECT_TYPE_OFFSET])) = VPLConv_hton_u8(val);
}

static inline char* vss_proxy_connect_get_server_addr(const char* buf)
{
    return (char*)&(buf[VSS_PROXY_CONNECT_SERVER_ADDR_OFFSET]);
}

static inline void vss_proxy_connect_set_server_addr(char* buf, const char* val)
{
    memcpy(&(buf[VSS_PROXY_CONNECT_SERVER_ADDR_OFFSET]), val, vss_proxy_connect_get_addrlen(buf));
}

/// VSS_PROXY_CONNECT_REPLY
/// Proxy connection response.
/// Send as first message on connection made to complete proxy connection.
///    +----------------+----------------+----------------+----------------+
///  0 | COOKIE                                                            |
///    +----------------+----------------+----------------+----------------+
///  4 | PORT                            | TYPE           | UNUSED         |
///    +----------------+----------------+----------------+----------------+
///  8 | CLUSTER_ID                                                        |
/// 12 |                                                                   |
///    +----------------+----------------+----------------+----------------+
///    COOKIE - Connection cookie, from request.
///    PORT - Port open by destination for peer-to-peer connection.
///    TYPE - Type of connection being made, from request.
///    CLUSTER_ID - Cluster ID of connecting PSN.

enum {
    VSS_PROXY_CONNECTR_COOKIE_OFFSET     =  0,
    VSS_PROXY_CONNECTR_PORT_OFFSET       =  4,
    VSS_PROXY_CONNECTR_TYPE_OFFSET       =  6,
    VSS_PROXY_CONNECTR_CLUSTER_ID_OFFSET =  8,
    VSS_PROXY_CONNECTR_SIZE              = 16
};

static inline u32 vss_proxy_connect_reply_get_cookie(const char* buf)
{
    return VPLConv_ntoh_u32(*((u32*)&(buf[VSS_PROXY_CONNECTR_COOKIE_OFFSET])));
}

static inline void vss_proxy_connect_reply_set_cookie(char* buf, u32 val)
{
    *((u32*)&(buf[VSS_PROXY_CONNECTR_COOKIE_OFFSET])) = VPLConv_hton_u32(val);
}

static inline u16 vss_proxy_connect_reply_get_port(const char* buf)
{
    return VPLConv_ntoh_u16(*((u16*)&(buf[VSS_PROXY_CONNECTR_PORT_OFFSET])));
}

static inline void vss_proxy_connect_reply_set_port(char* buf, u16 val)
{
    *((u16*)&(buf[VSS_PROXY_CONNECTR_PORT_OFFSET])) = VPLConv_hton_u16(val);
}

static inline u8 vss_proxy_connect_reply_get_type(const char* buf)
{
    return VPLConv_ntoh_u8(*((u8*)&(buf[VSS_PROXY_CONNECTR_TYPE_OFFSET])));
}

static inline void vss_proxy_connect_reply_set_type(char* buf, u8 val)
{
    *((u8*)&(buf[VSS_PROXY_CONNECTR_TYPE_OFFSET])) = VPLConv_hton_u8(val);
}

static inline u64 vss_proxy_connect_reply_get_cluster_id(const char* buf)
{
    return VPLConv_ntoh_u64(*((u64*)&(buf[VSS_PROXY_CONNECTR_CLUSTER_ID_OFFSET])));
}

static inline void vss_proxy_connect_reply_set_cluster_id(char* buf, u64 val)
{
    *((u64*)&(buf[VSS_PROXY_CONNECTR_CLUSTER_ID_OFFSET])) = VPLConv_hton_u64(val);
}

#ifdef __cplusplus
}
#endif

#endif // include guard
