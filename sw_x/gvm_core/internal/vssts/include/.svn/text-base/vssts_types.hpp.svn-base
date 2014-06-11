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

///
/// vsstsi_types.h
///
/// Virtual Storage Server Interface: User-Visible types.

#ifndef __VSSTS_TYPES_HPP__
#define __VSSTS_TYPES_HPP__

#include "vplu_types.h"
#include "vpl_time.h"

#ifdef IS_VSSTS
namespace vssts {
#endif // IS_VSSTS
#ifdef IS_VSSTS_WRAPPER
namespace vssts_wrapper {
#endif // IS_VSSTS_WRAPPER

/// App attributes as part of the app ID
#define VSSI_APP_ATTRIBUTE_MASK             0xffffffff00000000ULL
#define VSSI_APP_ATTRIBUTE_SHIFT            32

enum {
    VSSI_APP_ATTRIBUTE_LOOPBACK_ONLY = 0x1,     /// Use only loopback route for local apps
};

/// Virtual Storage object handle.
typedef void* VSSI_Object;

/// Virtual Storage open file handle.
typedef void* VSSI_File;

/// Virtual Storage identifier for open server file.
typedef u32 VSSI_ServerFileId;

/// File open modes
enum {
    VSSI_READONLY  = 0x00, /// Open object read-only.
    VSSI_READWRITE = 0x01, /// Open object read/write, create as needed.
    VSSI_FORCE     = 0x02, /// Object must ignore version conflict on change.
                           /// Must only use if changes cannot conflict.
    VSSI_LOCK      = 0x04, /// Object will be locked while open.
                           /// Lock will write lock if VSSI_READWRITE is set,
                           /// otherwise it will be a read lock.
                           /// SERVER USE ONLY!
    VSSI_INSTANT   = 0x20, /// Modifications to this object are instantly applied.
                           /// Commit and start-set commands will have no effect.
};

/// Object directory handle.
typedef void* VSSI_Dir2;

typedef struct {
    u64 size; /// Size of the file (bytes).
    VPLTime_t ctime; /// Change time in microseconds since epoch.
    VPLTime_t mtime; /// Modification time in microseconds since epoch.
    u64 changeVer; /// Object version when component last changed.
    u32 attrs; /// File attributes
    u8 isDir; /// 0 for files, nonzero for directories.
    const char* signature; /// For files, SHA-1 hash of the file (20 bytes)
    const char* name; /// Entry name relative to containing directory.
    void* metadata; /// Entry metadata.
} VSSI_Dirent2;

typedef u64 VSSI_NotifyMask;
enum {
    VSSI_NOTIFY_DISCONNECTED_EVENT   = 0, // For callback only. 
    VSSI_NOTIFY_DATASET_CHANGE_EVENT = (1 << 0),
    VSSI_NOTIFY_OPLOCK_BREAK_EVENT   = (1 << 1),
    /// TODO: Add more notification events as appropriate here.
};

/// Callback type for async notifications.
typedef void (*VSSI_NotifyCallback)(void* ctx, 
                                    VSSI_NotifyMask mask,
                                    const char* message,
                                    u32 message_length);

/// File Locking State
///   Supports two types of locks that apply to whole files:  
///   access locks and cache state ownership locks
typedef u64 VSSI_FileLockState;

/// Values for File Open mode flag
enum {
    VSSI_FILE_OPEN_READ    = (1 << 0),
    VSSI_FILE_OPEN_WRITE   = (1 << 1),
    VSSI_FILE_OPEN_DELETE  = (1 << 2),   // application intends to delete
    VSSI_FILE_OPEN_CREATE  = (1 << 3),   // create if not found (only if WRITE)
    VSSI_FILE_OPEN_OPEN_ALWAYS = VSSI_FILE_OPEN_CREATE,  // synonym for CREATE
    VSSI_FILE_OPEN_CREATE_ALWAYS   = (1 << 4), // create even if already exists
    VSSI_FILE_OPEN_CREATE_NEW      = (1 << 5), // create only if not exists
    VSSI_FILE_OPEN_TRUNCATE_ALWAYS = (1 << 6), // truncate if found
    VSSI_FILE_SHARE_READ   = (1 << 16),  // share with other readers
    VSSI_FILE_SHARE_WRITE  = (1 << 17),  // share with other writers
    VSSI_FILE_SHARE_DELETE = (1 << 18),  // share with other deleters
};

#define VSSI_FILE_ACCESS_MODES   0x0000ffff
#define VSSI_FILE_SHARING_MODES  0xffff0000

/// Values for File Open attributes (only used if create required)
enum {
    VSSI_ATTR_READONLY  = (1 << 0),
    VSSI_ATTR_HIDDEN    = (1 << 1),
    VSSI_ATTR_SYS       = (1 << 2),
    VSSI_ATTR_ARCHIVE   = (1 << 3),
};

/// Values for File Locking mode bit mask
enum {
    VSSI_FILE_LOCK_NONE          = (0),      // no locks
    VSSI_FILE_LOCK_READ_SHARED   = (1 << 0), // shared read
    VSSI_FILE_LOCK_WRITE_EXCL    = (1 << 1), // exclusive write

    VSSI_FILE_LOCK_CACHE_READ    = (1 << 8), // cache for read only
    VSSI_FILE_LOCK_CACHE_WRITE   = (1 << 9), // cache for write exclusive
    VSSI_FILE_LOCK_CACHE_BREAK   = (1 << 10), // cache state needs to be released
};

/// Values for Set Byte Range Lock flags bit mask
enum {
    VSSI_RANGE_LOCK   = (1 << 0),  // Lock the specified range
    VSSI_RANGE_UNLOCK = (1 << 1),  // Unlock 
    VSSI_RANGE_NOWAIT = (1 << 2),  // Return immediate error on conflict
    VSSI_RANGE_WAIT   = (1 << 3),  // Wait for release of conflicting lock
};

/// Byte Range Locks
///   Supports read and write locking of arbitrary byte ranges within a file.
///   Note that the offset does not need to be within the existing contents
///   of the file.
typedef struct {
    u64 lock_mask;  // same as for File Locks, but cache modes not valid
    u64 offset;     // first byte of the locked range
    u64 length;     // number of bytes covered by the lock
} VSSI_ByteRangeLock;

/// Values for Rename2 flags word
enum {
    VSSI_RENAME_DEFAULT           = (0),
    VSSI_RENAME_REPLACE_EXISTING  = (1 << 0), // Replace existing target file (not dir)
};

/// Result code for all operations.
typedef int VSSI_Result;

/// Callback type for all asynchronous operations.
/// Callback parameters: callback context and operation result.
typedef void (*VSSI_Callback)(void*, VSSI_Result);

#ifdef IS_VSSTS_WRAPPER
}
#endif // IS_VSSTS_WRAPPER
#ifdef IS_VSSTS
}
#endif // IS_VSSTS

#endif  // include guard
