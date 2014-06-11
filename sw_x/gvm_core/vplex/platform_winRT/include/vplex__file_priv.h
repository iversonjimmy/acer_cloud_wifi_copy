//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLEX_FILE_PRIV_H__
#define __VPLEX_FILE_PRIV_H__

#pragma once

#include "vplex_file.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------
// WinRT-specific functions:
//--------------------------------

/// Get taken date in EXIF of given path
/// Return VPL_OK when success.
/// Return VPL_ERR_XXX otherwise
int _VPLFile__EXIFGetImageTimestamp(const char* filename, time_t* date_time);

int _VPLFile_CheckAccess(const char *pathname, int mode);

VPLFile_handle_t _VPLFile_Open(const char *pathname, int flags, int mode);

ssize_t _VPLFile_Write(VPLFile_handle_t h, const void *buffer, size_t bufsize);
ssize_t _VPLFile_Read(VPLFile_handle_t h, void *buffer, size_t bufsize);

ssize_t _VPLFile_WriteAt(VPLFile_handle_t h, const void *buffer, size_t bufsize, VPLFile_offset_t offset);
ssize_t _VPLFile_ReadAt(VPLFile_handle_t h, void *buffer, size_t bufsize, VPLFile_offset_t offset);

int _VPLFile_CreateTemp(char* filename_in_out, size_t bufSize);

int _VPLFile_TruncateAt(VPLFile_handle_t h, VPLFile_offset_t length);

int _VPLFile_Sync(VPLFile_handle_t h);

VPLFile_offset_t _VPLFile_Seek(VPLFile_handle_t h, VPLFile_offset_t offset, int whence);

int _VPLFile_Close(VPLFile_handle_t h);

int _VPLFile_Delete(const char *pathname);

int _VPLFile_SetTime(const char *pathname, VPLTime_t time);

int _VPLFile_Rename(const char *oldpath, const char *newpath);

int _VPLFile_Move(const char *oldpath, const char *newpath);

int _VPLFile_SetAttribute(const char *path, u32 attrs, u32 maskbits);

int _VPLDir_Create(const char *pathname, int mode);

int _VPLDir_Delete(const char *pathname);

/// Returns #VPL_OK if \a h is a valid file within the local app data path.
/// Returns #VPL_ERR_NOENT if \a h is a valid file outside of the local app data path.
int _VPLFile_IsFileInLocalStatePath(VPLFile_handle_t h);

int _VPLFS_FStat(VPLFile_handle_t fd, VPLFS_stat_t* buf);

//--------------------------------

#ifdef __cplusplus
}
#endif

#endif // include guard
