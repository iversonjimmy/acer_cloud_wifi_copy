//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_FS_H__
#define __VPL_FS_H__

//============================================================================
/// @file
/// Virtual Platform Layer for File System operations.
/// Please see @ref VPLFS.
//============================================================================

#include "vpl_plat.h"
#include "vpl_time.h"
#include <time.h> // for time_t
#include "vpl__fs.h"
#include "vplu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLFS VPL File System API
///
/// This API provides access to file system operations not available in C or
/// C++ standard libraries. Specifically, operations concerning directories
/// and file metadata are provided here.
///
/// For operations on files, use the C standard library functions in stdlib.h
/// (fopen, fread, fwrite, fclose, etc.) or C++ iostream objects and operators.
///
/// Directory operations are similar to POSIX.1-2001 APIs.
///
/// File metadata operations are limited to getting the statistics of a file:
/// current size, last update time, and last access time.
///@{

/// Types for a file in the filesystem.
/// See #VPLFS_stat_t.type and #VPLFS_dirent_t.type.
typedef enum {
    
    /// Regular file.
    VPLFS_TYPE_FILE  = 0,
    
    /// Directory.
    VPLFS_TYPE_DIR   = 1,
    
    /// Other filesystem object.
    VPLFS_TYPE_OTHER = 2
    
} VPLFS_file_type_t;

typedef VPLFS_file_size__t VPLFS_file_size_t;
#define FMTu_VPLFS_file_size_t FMTu_VPLFS_file_size__t

//----------------------------------------------------------------------------
/// @name VPLFS File Metadata Operations
/// File metadata access operators
//@{

/// Metadata about a file or directory, as returned by #VPLFS_Stat().
typedef struct {
    
    /// Size of the file.
    VPLFS_file_size_t size;
    
    /// Type of file.
    VPLFS_file_type_t type;
    
    /// Time of last access.
    time_t atime;
    
    /// Time of last modification.
    time_t mtime;
    
    /// Time of creation.
    time_t ctime;

    /// Time of last access.(in VPLTime_t)
    VPLTime_t vpl_atime;
    
    /// Time of last modification.(in VPLTime_t)
    VPLTime_t vpl_mtime;
    
    /// Time of creation.(in VPLTime_t)
    VPLTime_t vpl_ctime;
    
    /// #VPL_TRUE if the file is hidden, else #VPL_FALSE.
    VPL_BOOL isHidden;
    
    /// #VPL_TRUE if the file is symlink, else #VPL_FALSE.
    VPL_BOOL isSymLink;

    /// #VPL_TRUE if the file is readonly, else #VPL_FALSE.
    VPL_BOOL isReadOnly;

    /// #VPL_TRUE if the file is system, else #VPL_FALSE.
    VPL_BOOL isSystem;

    /// #VPL_TRUE if the file is archive, else #VPL_FALSE.
    VPL_BOOL isArchive;
} VPLFS_stat_t;

/// Retrieve file status and store in @a buf.
///
/// @param[in] path Complete path and filename of file to stat.
/// @param[out] buf Pointer to a #VPLFS_stat_t structure.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_ACCESS Access denied.
/// @retval #VPL_ERR_INVALID @a buf is NULL.
//%/ @retval #VPL_ERR_LOOP A reference loop was encountered in path (can't happen without symlink support)
/// @retval #VPL_ERR_NAMETOOLONG @a path too long.
/// @retval #VPL_ERR_NOENT A component of @a path does not exist, or @a path is an empty string.
/// @retval #VPL_ERR_NOMEM Out of memory.
/// @retval #VPL_ERR_NOTDIR A component of @a path is not a directory.
int VPLFS_Stat(const char* path,
               VPLFS_stat_t* buf);

/// Commit buffer cache to disk.
///
/// @par Usage:
/// Use this after closing file with fclose(), etc. to ensure data is physically stored on disk.
/// The actual time this function takes to complete depends on the size of the data in the buffer
/// cache and contention for disk resources.
/// @note This function is always successful.
void VPLFS_Sync(void);

//% End of VPLFS File Metadata Operations
//@}

//----------------------------------------------------------------------------
/// @name VPLFS Directory Operations
/// Directory access and manipulation operations.
//@{

/// Abstract representation of a VPL Directory object.
/// Users of the VPL API must treat this object as opaque.
typedef VPLFS_dir__t  VPLFS_dir_t;

#define VPLFS_DIRENT_FILENAME_MAX VPLFS__DIRENT_FILENAME_MAX

/// Directory entry details, used with #VPLFS_Readdir().
typedef struct {
    
    /// The "type" of a file. A value from #VPLFS_file_type_t.
    VPLFS_file_type_t type;
    
    /// The name of the file, encoded in UTF-8 and NULL terminated.
    char filename[VPLFS_DIRENT_FILENAME_MAX];
    
} VPLFS_dirent_t;

/// Open a directory stream.
///
/// @param[in] name Directory name.
/// @param[out] dir A pointer to the open directory stream, on success.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_ACCESS Permission denied.
/// @retval #VPL_ERR_INVALID @a dir is NULL.
/// @retval #VPL_ERR_MAX Too many files and directories are open.
/// @retval #VPL_ERR_NOENT Directory @a name does not exist, or @a name is an empty string.
/// @retval #VPL_ERR_NOMEM Insufficient memory to complete the operation.
/// @retval #VPL_ERR_NOTDIR @a name is not a directory.
int VPLFS_Opendir(const char* name, VPLFS_dir_t* dir);

/// Close a directory stream.
///
/// @param[in] dir Directory stream returned by #VPLFS_Opendir().
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_BADF @a dir is invalid.
/// @retval #VPL_ERR_INVALID @a dir is NULL.
int VPLFS_Closedir(VPLFS_dir_t* dir);

/// Return the directory entry information for the next entry in a directory stream.
///
/// @param[in] dir Directory handle returned by #VPLFS_Opendir().
/// @param[out] entry On success, filled with the directory entry information.
/// @retval #VPL_OK Success; @a entry will have valid information for the current entry.
/// @retval #VPL_ERR_BADF @a dir is invalid.
/// @retval #VPL_ERR_INVALID @a dir and/or @a entry is NULL.
/// @retval #VPL_ERR_MAX There are no more entries in the @a dir stream.
int VPLFS_Readdir(VPLFS_dir_t* dir, VPLFS_dirent_t* entry);

/// Reset the directory stream to the beginning of the directory.
///
/// @param[in] dir Directory handle returned by #VPLFS_Opendir().
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_BADF @a dir is invalid.
/// @retval #VPL_ERR_INVALID @a dir is NULL.
int VPLFS_Rewinddir(VPLFS_dir_t* dir);

/// Set the position of the next #VPLFS_Readdir() call in the directory stream.
///
/// @param[in] dir Directory handle returned by #VPLFS_Opendir().
/// @param[in] pos A stream position returned by #VPLFS_Telldir().
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_BADF @a dir is invalid.
/// @retval #VPL_ERR_INVALID @a dir is NULL.
int VPLFS_Seekdir(VPLFS_dir_t* dir, size_t pos);

/// Get the current location in a directory stream.
///
/// @param[in] dir Directory handle returned by #VPLFS_Opendir().
/// @param[out] pos Set to the current location in the directory stream on success.
/// @retval #VPL_OK Success; @a pos will be the current offset.
/// @retval #VPL_ERR_BADF @a dir is invalid.
/// @retval #VPL_ERR_INVALID @a dir and/or @a pos is NULL.
int VPLFS_Telldir(VPLFS_dir_t* dir, size_t* pos);

/// Create a directory.
///
/// The permissions will be set in a platform-dependent manner, and will generally match the
/// permissions or umask of the calling process.
//% For internal developers: if you need more control over permissions, try #VPLDir_Create().
/// @param[in] pathName Name of directory to create
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_ACCESS Access denied, probably because the filesystem is read-only.
/// @retval #VPL_ERR_EXIST @a pathName already exists.
/// @retval #VPL_ERR_FAULT @a pathName is outside your addressable address space.
//%/ @retval #VPL_ERR_LOOP Too many symbolic links resolving @a pathName (can't happen without symlinks)
/// @retval #VPL_ERR_NAMETOOLONG The length of @a pathName is too long.
/// @retval #VPL_ERR_NOENT A component of the path prefix in @a pathName does not name an existing directory or @a pathName is an empty string.
/// @retval #VPL_ERR_NOMEM Insufficient memory to complete the operation.
/// @retval #VPL_ERR_NOSPC Insufficient disk space for the new directory.
/// @retval #VPL_ERR_NOTDIR A component of the path prefix is not a directory.
/// @retval #VPL_ERR_ROFS The parent directory is read-only.
int VPLFS_Mkdir(const char* pathName);

/// Remove a directory, which must be empty.
///
/// @param[in] pathName Directory path to remove
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_ACCESS Access denied.
/// @retval #VPL_ERR_BUSY The directory is in use and cannot be removed.
/// @retval #VPL_ERR_FAULT @a pathName is outside your addressable address space.
/// @retval #VPL_ERR_INVALID @a pathName has . as the last component.
/// @retval #VPL_ERR_LOOP Too many symbolic links were encountered trying to resolve @a pathName.
/// @retval #VPL_ERR_NAMETOOLONG The length of @a pathName is too long.
/// @retval #VPL_ERR_NOENT A component of the path prefix in @a pathName does not name an existing directory or @a pathName is an empty string.
/// @retval #VPL_ERR_NOMEM Insufficient memory to complete the operation.
/// @retval #VPL_ERR_NOTDIR A component of the path prefix is not a directory.
/// @retval #VPL_ERR_NOTEMPTY @a pathName contains entries other than . and .. or has .. as its final component.
/// @retval #VPL_ERR_ROFS The parent directory is read-only.
int VPLFS_Rmdir(const char* pathName);

//% End of VPLFS Directory Operations
///@}

#if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
#  define VPLFS_MONITOR_SUPPORTED  0
#else
#  define VPLFS_MONITOR_SUPPORTED  1
#endif
    
/// Initializes VPLFS_Monitor APIs.
int VPLFS_MonitorInit();
/// Cleans up initialization of VPLFS_Monitor
int VPLFS_MonitorDestroy();

typedef enum {
    VPLFS_MonitorEvent_NONE,
    VPLFS_MonitorEvent_FILE_ADDED,
    VPLFS_MonitorEvent_FILE_REMOVED,
    VPLFS_MonitorEvent_FILE_MODIFIED,
    VPLFS_MonitorEvent_FILE_RENAMED
} VPLFS_MonitorEventType;

typedef struct {
    /// Number of bytes to the next event entry.
    u32 nextEntryOffsetBytes;
    /// Event type
    VPLFS_MonitorEventType action;
    /// Pointer to NULL terminated string of file or directory related to the event.
    /// If the action is VPLFS_MonitorEvent_FILE_RENAMED, this is the moveFrom
    /// field.
    const char* filename;
    /// Pointer to NULL terminated string if action is VPLFS_MonitorEvent_FILE_RENAMED.
    /// Otherwise, simply null.
    const char* moveTo;
} VPLFS_MonitorEvent;

#define VPLFS_MonitorCB_OK 0
#define VPLFS_MonitorCB_OVERFLOW -1
#define VPLFS_MonitorCB_UNMOUNT -2

/// Callback to be passed to VPLFS_MonitorDir.
/// @param[in] handle Handle that generated the event
/// @param[in] eventBuffer Buffer containing zero or more events.  The event buffer
///                        is owned by the VPL library, and once the callback is
///                        complete, the event buffer must not be modified by the
///                        user.
/// @param[in] eventBufferSize Size of buffer returned
/// @param[in] error Error code
///                  VPLFS_MonitorCB_OK
///                  VPLFS_MonitorCB_OVERFLOW buffer not large enough, events lost
///                  VPLFS_MonitorCB_UNMOUNT
typedef void (*VPLFS_MonitorCallback)(VPLFS_MonitorHandle handle,
                                      void* eventBuffer,
                                      int eventBufferSize,
                                      int error);

/// Begins recursive monitoring of a directory.  Events will be returned in the
/// callback as they occur.
/// @param[in] directory Directory name to monitor.
/// @param[in] num_events_internal Amount of internal buffer to allocate to
///                                store, events. The more buffer, the less
///                                chance of overrun.
/// @param[in] cb Callback function to call when an event is received.  Note:
///               win32 users must call VPL_Yield or VPL_Sleep for events to
///               be received.
/// @param[out] handle_out Handle to identify the monitor.  The handle is returned
///                        with events and used to identify the monitor to stop.
int VPLFS_MonitorDir(const char* directory,
                     int num_events_internal,
                     VPLFS_MonitorCallback cb,
                     VPLFS_MonitorHandle* handle_out);
int VPLFS_MonitorDirStop(VPLFS_MonitorHandle handle);

/// Get disk space information
/// @param[in] directory Any directory on the disk.
/// @param[out] disk_size Total size of the disk.
/// @param[out] avail_size Available size on the disk.
int VPLFS_GetSpace(const char* directory,
                   u64* disk_size,
                   u64* avail_size);

//% end of defgroup VPLFS.
///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
