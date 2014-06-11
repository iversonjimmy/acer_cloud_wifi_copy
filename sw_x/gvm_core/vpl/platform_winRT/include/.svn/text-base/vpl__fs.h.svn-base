//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL__FS_H__
#define __VPL__FS_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#pragma once

#include <wchar.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int handle_key;
    UINT max_pos;
    HANDLE handle;
    void* finddata;  // pointer to WIN32_FIND_DATA object
    wchar_t* name;  // dir name needed in case of rewind
    int pos;  // need to track pos in case of telldir.
} VPLFS_dir__t;

#define VPLFS_INVALID_HANDLE -1
#define VPLFS__DIRENT_FILENAME_MAX (4*256)

typedef void* VPLFS_MonitorHandle;

typedef uint64_t VPLFS_file_size__t;
#define FMTu_VPLFS_file_size__t "%I64u"

//--------------------------------
// WinRT-specific functions:
//--------------------------------

int _VPLFS_Error_XlatErrno(HRESULT hResult);

int _VPLFS__ConvertPath(const wchar_t *path, wchar_t **epath, size_t extrachars);

int _crack_path(const wchar_t* wpathname, wchar_t** wparent, wchar_t** wfilename);

/// Check to make sure intermediate paths are valid paths.
/// For proper operation, the path must be an absolute path with a drive letter.
/// @note It has been observed that this function is very expensive time-wise.
///     DO NOT call this function as a path validator (as originally intended)
///     but call ONLY IF an error is encountered and we want to determine if it
///     was due to a bad path.
int _VPLFS__CheckValidWpath(const wchar_t* wpath);

/// Get the path in extended-length format.
/// http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
/// Example: "C:/abc/def" -> "\\?\C:\abc\def"
/// epath returned is a malloc'd memory, so caller must free after use.
int _VPLFS__GetExtendedPath(const wchar_t *path, wchar_t **epath, size_t extrachars);

// Returns the path to the current user's profile folder.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetProfilePath(char **upath);

// Returns the path to the current user's local app data folder.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetLocalAppDataPath(char **upath);

// Returns the path to the current user's Libraries folder.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetLibrariesPath(char **upath);

// Returns the path to the current user's profile folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetProfileWPath(wchar_t **upath);

// Returns the path to the current user's local app data folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetLocalAppDataWPath(wchar_t **upath);

// Returns the path to the current user's Libraries folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetLibrariesWPath(wchar_t **upath);

//--------------------------------

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <map>

#endif
#endif // include guard
