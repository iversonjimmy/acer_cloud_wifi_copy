//
//  Copyright 2010 iGware Inc.
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

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

enum _VPLFS__DriveType {
    WIN_DRIVE_UNKNOWN,
    WIN_DRIVE_NO_ROOT_DIR,
    WIN_DRIVE_REMOVABLE,
    WIN_DRIVE_FIXED,
    WIN_DRIVE_REMOTE,
    WIN_DRIVE_CDROM,
    WIN_DRIVE_RAMDISK
};

typedef struct {
    HANDLE handle;
    void* finddata;  // pointer to WIN32_FIND_DATA object
    wchar_t* name;  // dir name needed in case of rewind
    int pos;  // need to track pos in case of telldir.
} VPLFS_dir__t;

// Max for Win32 is 256 chars (http://msdn.microsoft.com/en-us/library/aa365247%28VS.85%29.aspx#maxpath)
// The latest UTF8 spec (RFC3629) limits unicode characters to U+0000..U+10FFFF.
// This means the maximum number of bytes required per unicode character is 4.
#define VPLFS__DIRENT_FILENAME_MAX (4*256)

typedef void* VPLFS_MonitorHandle;

typedef uint64_t VPLFS_file_size__t;
#define FMTu_VPLFS_file_size__t "%I64u"

//--------------------------------
// Win32-specific functions:
//--------------------------------

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

// Returns the path to the current user's Desktop folder.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetDesktopPath(char **upath);

// Returns the path to the Public Desktop folder.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetPublicDesktopPath(char **upath);

// Returns the path to the current user's profile folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetProfileWPath(wchar_t **upath);

// Returns the path to the current user's local app data folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetLocalAppDataWPath(wchar_t **upath);

// Returns the path to the current user's Libraries folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetLibrariesWPath(wchar_t **upath);

// Returns the path to the current user's desktop folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetDesktopWPath(wchar_t **upath);

// Returns the path to the Public Desktop folder in Windows native UTF16.
// @note The caller is responsible for calling free() on the returned path after use.
int _VPLFS__GetPublicDesktopWPath(wchar_t **upath);

//--------------------------------

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <map>

// Get shortcut's target path, type (file or folder), and arguments(if any)
// Returned path will be an absolute path with a drive letter.
int _VPLFS__GetShortcutDetail(const std::string& targetpath, std::string& upath, std::string& type, std::string& args);

struct _VPLFS__LibFolderInfo {
    std::string n_name;  // locale-neutral name of the folder
    std::string l_name;  // localized name of the folder
    std::string path;    // absolute path to the folder
};

struct _VPLFS__LibInfo {
    std::string n_name;       // locale-neutral name of the library
    std::string l_name;       // localized name of the library
    std::string folder_type;  // locale-neutral string of the folder type in English (Video/Music/Photo/Documents/Generic)
    std::map<std::string, _VPLFS__LibFolderInfo> m;  // map from locale-neutral name of the folder to
                                                    // _VPLFS__LibFolderInfo object
};

// Returns display name about input path in Windows.
// @note Implemented properly only if compiled using Microsoft Visual C++.
int _VPLFS__GetDisplayName(const std::string &upath, std::string &displayname);

// Returns info about Library folders in Windows 7.
// @note Implemented properly only if compiled using Microsoft Visual C++.
int _VPLFS__GetLibraryFolders(const std::string &upath, _VPLFS__LibInfo *libinfo);

// Returns display name about input path in Windows.
// @note Implemented properly only if compiled using Microsoft Visual C++.
int _VPLFS__LocalizedPath(const std::string &upath, std::string &displayname);

// Returns info about windows computer drives in Windows 7.
// @note Implemented properly only if compiled using Microsoft Visual C++.
int _VPLFS__GetComputerDrives(std::map<std::string, _VPLFS__DriveType> &driveMap);

/// Returns result to insert ACE to trust tree of VPLFS.
/// @note This transfers ownership of the SID pointed at by "ace.Sid" to VPLFS.
///       #_VPLFS__ClearTrustees() will call LocalFree on it.
///       Therefore, the caller must ensure that the buffer remains valid until #_VPLFS__ClearTrustees() is called.
/// @note Implemented properly only if compiled using Microsoft Visual C++.
int _VPLFS__InsertTrustees(SID_AND_ATTRIBUTES &ace);

// clear trust tree info of VPLFS
// @note Implemented properly only if compiled using Microsoft Visual C++.
void _VPLFS__ClearTrustees();

// Returns result that path has access right or not 
//     VPL_OK: access right is granted
//     VPL_ERR_ACCESS: access denied
// @param access_make could be the following flags:
//     DELETE: delete access
//     FILE_ALL_ACCESS: all access
//     FILE_GENERIC_READ: read access
//     FILE_GENERIC_WRITE: write access
//     FILE_GENERIC_EXECUTE: execution access
// @param is_checkancestor check access read right of all ancestor paths
// @note Implemented properly only if compiled using Microsoft Visual C++.
int _VPLFS__CheckAccessRight(const wchar_t *wpath, ACCESS_MASK access_mask, bool is_checkancestor = false);

// Returns result that upathanme is self-signed and trusted or not
//     VPL_OK: self-signed and trusted
//     VPL_ERR_FAIL: un-signed, not trusted or unknown errors
//     VPL_ERR_NOENT: upathname doesn't exist
// @note Implemented properly only if compiled using Microsoft Visual C++.
int _VPLFS_CheckTrustedExecutable(const std::string &upathname);

#endif
#endif // include guard
