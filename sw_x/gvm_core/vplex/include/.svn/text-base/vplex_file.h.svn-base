//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLEX_FILE_H__
#define __VPLEX_FILE_H__

//============================================================================
/// @file
/// This file abstracts file operations using file descriptors and file names.
//============================================================================

#include "vplex_plat.h"
#include "vpl_time.h"

#include "vplex__file.h"
#include "vpl_fs.h"

#include <stdio.h>

#ifndef __VPLFile_handle_t_defined
#error VPLFile_handle_t not defined for this platform
#endif

#ifndef __VPLFile_offset_t_defined
#error VPLFile_offset_t not defined for this platform
#endif

#define FMTu_VPLFile_offset_t FMTu_VPLFile_offset__t

#ifdef __cplusplus
extern "C" {
#endif

#define VPLFILE_CHECKACCESS_READ     VPLFILE__CHECKACCESS_READ
#define VPLFILE_CHECKACCESS_WRITE    VPLFILE__CHECKACCESS_WRITE
#define VPLFILE_CHECKACCESS_EXECUTE  VPLFILE__CHECKACCESS_EXECUTE
#define VPLFILE_CHECKACCESS_EXISTS   VPLFILE__CHECKACCESS_EXISTS

#define VPLFILE_OPENFLAG_CREATE     VPLFILE__OPENFLAG_CREATE
#define VPLFILE_OPENFLAG_READONLY   VPLFILE__OPENFLAG_READONLY
#define VPLFILE_OPENFLAG_WRITEONLY  VPLFILE__OPENFLAG_WRITEONLY
#define VPLFILE_OPENFLAG_READWRITE  VPLFILE__OPENFLAG_READWRITE
#define VPLFILE_OPENFLAG_TRUNCATE   VPLFILE__OPENFLAG_TRUNCATE
#define VPLFILE_OPENFLAG_APPEND     VPLFILE__OPENFLAG_APPEND
#define VPLFILE_OPENFLAG_EXCLUSIVE  VPLFILE__OPENFLAG_EXCLUSIVE

#define VPLFILE_MODE_IRUSR  VPLFILE__MODE_IRUSR
#define VPLFILE_MODE_IWUSR  VPLFILE__MODE_IWUSR

#define VPLFILE_SEEK_SET VPLFILE__SEEK_SET
#define VPLFILE_SEEK_CUR VPLFILE__SEEK_CUR
#define VPLFILE_SEEK_END VPLFILE__SEEK_END

#define VPLFILE_INVALID_HANDLE VPLFILE__INVALID_HANDLE

#define VPLFILE_ATTRIBUTE_READONLY 0x0001
#define VPLFILE_ATTRIBUTE_HIDDEN   0x0002
#define VPLFILE_ATTRIBUTE_SYSTEM   0x0004
#define VPLFILE_ATTRIBUTE_ARCHIVE  0x0008
#define VPLFILE_ATTRIBUTE_MASK  (VPLFILE_ATTRIBUTE_HIDDEN | VPLFILE_ATTRIBUTE_READONLY | VPLFILE_ATTRIBUTE_SYSTEM | VPLFILE_ATTRIBUTE_ARCHIVE)

// Access mask for dataset.hpp::check_access_right()
// For now, only Remote File is using the check_access_right() and only Win32 platform
// has the implementation called _VPL_CheckAccessRight()
#define VPLFILE_CHECK_PERMISSION_READ     (0x0001)
#define VPLFILE_CHECK_PERMISSION_WRITE    (0x0002)
#define VPLFILE_CHECK_PERMISSION_DELETE   (0x0004)
#define VPLFILE_CHECK_PERMISSION_EXECUTE  (0x0008)

/// @param mode Use the VPLFILE_CHECKACCESS_* flags.
int VPLFile_CheckAccess(const char *pathname, int mode);

/// @param flags Use the VPLFILE_OPENFLAG_* flags (you can bitwise-OR them together).
/// @param mode Mode to set when creating the file.  Ignored if #VPLFILE_OPENFLAG_CREATE is not set in @a flags.
///     Use the VPLFILE_MODE_* flags (you can bitwise-OR them together).  Our codebase currently uses
///     this interchangeably with POSIX open()'s mode param, so octal constants like 0666 or 0777 are also
///     acceptable for now.
// TODO: this breaks the VPLFile_handle_t abstraction:
/// @return If negative, it should be interpreted as an error code.
VPLFile_handle_t VPLFile_Open(const char *pathname, int flags, int mode);

int VPLFile_IsValidHandle(VPLFile_handle_t h);

ssize_t VPLFile_Write(VPLFile_handle_t h, const void *buffer, size_t bufsize);

// TODO: Bug 15175: Currently, it may be possible for this to return less than bufsize, even though
//   there are more bytes before end-of-file.
/// @return Number of bytes actually read.  0 indicates end-of-file.
ssize_t VPLFile_Read(VPLFile_handle_t h, void *buffer, size_t bufsize);

// using "At" to mean offset is in the argument
ssize_t VPLFile_WriteAt(VPLFile_handle_t h, const void *buffer, size_t bufsize, VPLFile_offset_t offset);
ssize_t VPLFile_ReadAt(VPLFile_handle_t h, void *buffer, size_t bufsize, VPLFile_offset_t offset);

/// Works just like mkstemp - generates a unique temporary filename, creates and opens the file,
/// and returns the open file descriptor.
/// The last six characters of @a filename_in_out must be "XXXXXX" and
/// they will be replaced with a string that makes the filename unique.
/// @param bufSize The largest possible size of @a filename_in_out, including the null-terminator.
///     The actual number of characters in @a filename_in_out may be less than this value.
int VPLFile_CreateTemp(char* filename_in_out, size_t bufSize);

int VPLFile_TruncateAt(VPLFile_handle_t h, VPLFile_offset_t length);

int VPLFile_Sync(VPLFile_handle_t h);

/// @param whence Specify #VPLFILE_SEEK_SET, #VPLFILE_SEEK_CUR, or #VPLFILE_SEEK_END.
/// @return If negative, treat it as a VPL error code.  Otherwise, it's the new offset.
VPLFile_offset_t VPLFile_Seek(VPLFile_handle_t h, VPLFile_offset_t offset, int whence);

int VPLFile_Close(VPLFile_handle_t h);

FILE *VPLFile_FOpen(const char *pathname, const char *mode);

int VPLFile_Delete(const char *pathname);

/// Sets the "last accessed" (#VPLFS_stat_t#atime) and "last modified" (#VPLFS_stat_t#mtime) times.
int VPLFile_SetTime(const char *pathname, VPLTime_t time);

/// Rename a file, by only making changes to metadata.
/// Unlike VPLFile_Move(), it will not involve any transfer of file content.
/// (On WinRT, cannot guarantee that file content won't be transferred. File a bug if this is a problem.)
/// Return an error if Rename cannot be completed under the above constraints.
int VPLFile_Rename(const char *oldpath, const char *newpath);

/// Move a file, possibly across different volumes/shares/partitions.
/// Unlike VPLFile_Rename(), this may involve transfer of file content.
int VPLFile_Move(const char *oldpath, const char *newpath);

int VPLFile_SetAttribute(const char *path, u32 attrs, u32 maskbits);

/// @param mode Can specify permissions as 0777 (leading 0 indicates octal literal).
///     Bit values are read=4, write=2, execute=1 for Linux systems.
///     This is currently ignored for Windows.
int VPLDir_Create(const char *pathname, int mode);

int VPLDir_Delete(const char *pathname);

#ifndef VPL_PLAT_IS_WINRT
int VPLPipe_Create(VPLFile_handle_t handles[2]);
#endif

/// Retrieve file status and store in @a buf.
/// @param[in] file descriptor of file to stat.
/// @param[out] buf Pointer to a #VPLFS_stat_t structure.
/// @retval the same as #VPLFS_Stat()
int	VPLFS_FStat(VPLFile_handle_t fd, VPLFS_stat_t* buf);

#ifdef __cplusplus
}
#endif

#endif // include guard
