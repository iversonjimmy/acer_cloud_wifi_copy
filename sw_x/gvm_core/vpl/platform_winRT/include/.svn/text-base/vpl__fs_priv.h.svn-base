//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL__FS_PRIV_H__
#define __VPL__FS_PRIV_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::FileProperties;
using namespace Windows::Foundation::Collections;

ref class vplFileChainer
{
internal:
    StorageFile^ file;
    ULONGLONG curpos;
    
    vplFileChainer();
};

ref class vplFolderChainer
{
internal:
    StorageFolder^ folder;
    ULONGLONG curpos;
    IVectorView<IStorageItem^>^ items;

    vplFolderChainer();
};

enum class IOLOCK_TYPE {
    IOLOCK_OPEN,
    IOLOCK_READ,
    IOLOCK_WRITE,
    IOLOCK_STAT
};

/// Converts the UniversalTime field of a Windows::Foundation::DateTime to time_t.
time_t _vplfs_converttime(long long time);
int _vpl_openfile(const wchar_t* wpathname, vplFileChainer^ file);
void _fileio_lock(const wchar_t *wpathname, HANDLE *event, IOLOCK_TYPE type);
void _fileio_unlock(HANDLE event);

//--------------------------------
// WinRT-specific functions:
//--------------------------------

int _VPLFS_Error_XlatErrno(HRESULT hResult);

int _VPLFS__ConvertPath(const wchar_t *path, wchar_t **epath, size_t extrachars);

int _crack_path(const wchar_t* wpathname, wchar_t** wparent, wchar_t** wfilename);

// Check if given path locates in LocalState
// Return VPL_OK when it does.
// Return VPL_ERR_NOENT otherwise.
int _VPLFS_IsInLocalStatePath(const char *path);

// Check if given VPLFS_dir_t locates in LocalState
// Return VPL_OK when it does.
// Return VPL_ERR_NOENT otherwise.
int _VPLFS_IsDirInLocalStatePath(VPLFS_dir_t* dir);

int _VPLFS_Stat(const char* pathname, VPLFS_stat_t* buf);

void _VPLFS_Sync(void);

int _VPLFS_Opendir(const char* pathname, VPLFS_dir_t* dir);

int _VPLFS_Closedir(VPLFS_dir_t* dir);

int _VPLFS_Readdir(VPLFS_dir_t* dir, VPLFS_dirent_t* entry);

int _VPLFS_Rewinddir(VPLFS_dir_t* dir);

int _VPLFS_Seekdir(VPLFS_dir_t* dir, size_t pos);

int _VPLFS_Telldir(VPLFS_dir_t* dir, size_t* pos);

int _VPLFS_Mkdir(const char* pathname);

int _VPLFS_Rmdir(const char* pathname);

//--------------------------------

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <map>

#endif
#endif // include guard
