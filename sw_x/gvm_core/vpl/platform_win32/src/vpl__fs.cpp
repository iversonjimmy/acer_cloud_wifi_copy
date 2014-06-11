//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// defines and includes for generic function names to map to unicode versions
// http://msdn.microsoft.com/en-us/library/dd374107%28v=VS.85%29.aspx
// http://msdn.microsoft.com/en-us/library/dd374061%28v=VS.85%29.aspx
#ifndef UNICODE
# define UNICODE
#endif
#ifndef _UNICODE
# define _UNICODE
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#include <Windows.h>
#else
// Win7 needed for Libraries access functions
#define NTDDI_VERSION 0x06010000 // NTDDI_WIN7
#define _WIN32_WINNT 0x0601 // _WIN32_WINNT_WIN7
#define WINVER 0x0601 // _WIN32_WINNT_WIN7
#define _WIN32_IE 0x0700 // _WIN32_IE_IE70
#endif

#include "vpl_fs.h"

#if __MSVCRT_VERSION__ < 0x0700
#error "__MSVCRT_VERSION__ must be at least 0x0700"
#endif

#include <wchar.h>

#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <io.h>
#include <Sddl.h>
#include <Aclapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "vplu.h"
#include "vpl__plat.h"
#include "scopeguard.hpp"

#include "vplu_mutex_autolock.hpp"
#include "vpl_lazy_init.h"

#ifdef VPL_PLAT_IS_WINRT
#include "vpl__fs_priv.h"
#endif
#include <string>
#include <map>
#include <algorithm>
#include <list>

// for _VPLFS_CheckTrustedExecutable()
#ifndef VPL_PLAT_IS_WINRT
#include <Shellapi.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <Shobjidl.h>
#include <Userenv.h>

// Link with the Wintrust.lib file.
#pragma comment (lib, "wintrust")
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "Userenv.lib")
#endif //ndef VPL_PLAT_IS_WINRT

static VPLLazyInitMutex_t trusteesMutex = VPLLAZYINITMUTEX_INIT;
static std::list<SID_AND_ATTRIBUTES> _trustees;

int _VPLFS__CheckValidWpath(const wchar_t *wpath)
{
    int rv = VPL_OK;
    size_t wpathlen = wcslen(wpath);
    
    wchar_t *wbuf = NULL;
    wchar_t *epath = NULL;
    {
        wbuf = (wchar_t*)malloc((wpathlen + 1) * sizeof(wchar_t));
        if (wbuf == NULL) {
            rv = VPL_ERR_NOMEM;
            goto end;
        }
    }

    {
        const wchar_t *p = wpath;
#ifdef VPL_PLAT_IS_WINRT
        wchar_t* wLocal;
        _VPLFS__GetLocalAppDataWPath(&wLocal);
        // For winrt, first check if the file is located in app LocalState folder
        if (wcslen(wLocal) > wpathlen) {
            rv = VPL_ERR_NOENT;
            free(wLocal);
            wLocal = NULL;
            goto end;
        }
        for (size_t i=0; i < wcslen(wLocal); i++) {
            if (wLocal[i] != wpath[i] 
            && !(wLocal[i] == L'\\' || wLocal[i] == L'/') 
            && !(wpath[i] == L'\\' || wpath[i] == L'/')) {
                rv = VPL_ERR_NOENT;
                free(wLocal);
                wLocal = NULL;
                goto end;
            }
        }
        p = wpath + wcslen(wLocal);
        free(wLocal);
        wLocal = NULL;
#endif
        while ((p = wcschr(p, L'/')) != NULL) {
            if (p >= wpath + wpathlen - 1) {  // probably trailing slash
                break;
            }
            if (p - wpath > 2) {
                wcsncpy(wbuf, wpath, p - wpath);
                wbuf[p - wpath] = L'\0';

                rv = _VPLFS__GetExtendedPath(wbuf, &epath, 0);
                if (rv != VPL_OK) {
                    goto end;
                }
#ifdef VPL_PLAT_IS_WINRT
                WIN32_FILE_ATTRIBUTE_DATA fileInfo;
                if (!GetFileAttributesEx(epath,
                                         GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard,
                                         &fileInfo)) {
                    rv = VPLError_XlatWinErrno(GetLastError());
                    goto end;
                }
                DWORD attr = fileInfo.dwFileAttributes;
#else
                DWORD attr = GetFileAttributes(epath);
#endif
                if (attr == INVALID_FILE_ATTRIBUTES) {
                    rv = VPL_ERR_NOENT;
                    goto end;
                }
                if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                    rv = VPL_ERR_NOTDIR;
                    goto end;
                }
                if (epath) {
                    free(epath);
                    epath = NULL;
                }
            }
            p++;
        }
    }
 end:
    if (wbuf) {
        free(wbuf);
        wbuf = NULL;
    }
    if (epath) {
        free(epath);
        epath = NULL;
    }
    return rv;
}

static const wchar_t extended_format_prefix[] = L"\\\\?\\";

int _VPLFS__GetExtendedPath(const wchar_t *wpath, wchar_t **epath, size_t extrachars)
{
    if (wpath == NULL || epath == NULL)
        return VPL_ERR_INVALID;

    std::wstring wpath2(wpath);

    // collapse consecutive slashes into one
    size_t pos = wpath2.find_first_of(L"/\\");
    while (pos != std::wstring::npos) {
        size_t pos2 = wpath2.find_first_not_of(L"/\\", pos);
        // position interval [pos, pos2) contains slashes
        size_t len = pos2 != std::wstring::npos ? pos2 - pos : wpath2.length() - pos;
        wpath2.replace(pos, len, 1, L'\\');
        if (pos + 1 < wpath2.length())
            pos = wpath2.find_first_of(L"/\\", pos + 1);
        else 
            pos = std::wstring::npos;
    }

    // replace "\.\" with "\"
    pos = wpath2.find(L"\\.\\");
    while (pos != std::wstring::npos) {
        wpath2.replace(pos, 3, 1, L'\\');  // 3 == wsclen(L"\\.\\")
        if (pos + 1 < wpath2.length())
            pos = wpath2.find(L"\\.\\", pos + 1);
        else
            pos = std::wstring::npos;
    }

    size_t epath_bufsize = wpath2.length() + sizeof(extended_format_prefix) + 1 + extrachars;
    *epath = (wchar_t*)malloc(epath_bufsize * sizeof(wchar_t));
    if (*epath == NULL)
        return VPL_ERR_NOMEM;

    wcscpy(*epath, extended_format_prefix);
    wcscat(*epath, wpath2.c_str());
    
    return VPL_OK;
}

//--------------------------------------------------------------
// Note: There is VPLFS_FStat(VPLFile_handle_t fd, VPLFS_stat_t* buf) which takes file handle as parameter and returns the same information.
int 
VPLFS_Stat(const char* pathname, VPLFS_stat_t* buf)
{
    int rc = -1;
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    DWORD fileAttrs = 0;
    HANDLE h = INVALID_HANDLE_VALUE;

#ifndef VPL_PLAT_IS_WINRT
    FILETIME atim, mtim, ctim;
#endif

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFS_Stat(pathname, buf);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    // number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
    const VPLTime_t EPOCH_DIFF = VPLTime_FromSec(11644473600LL);

    if (pathname == NULL || buf == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (strlen(pathname) == 0) {
        rv = VPL_ERR_NOENT;
        goto end;
    }
    if (strlen(pathname) == 1 && (pathname[0] == L'/' || pathname[0] == L'\\')) {
        rv = VPL_ERR_NOENT;
        goto end;
    }

    rc = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    /* Note:
     * _stat() cannot handle trailing slashes, so we need to remove them
     * cf. http://msdn.microsoft.com/en-us/library/14h5k7ff%28v=VS.100%29.aspx
     * unless the path is to the root directory (e.g., C:/)
     */
    if (wcslen(wpathname) > 3)
    {
        int i = (int)(wcslen(wpathname) - 1);
        while ((i >= 0) && (wpathname[i] == L'/' || wpathname[i] == L'\\')) {
            wpathname[i] = L'\0';
            i--;
        }
    }

    rc = _VPLFS__GetExtendedPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }
#ifdef VPL_PLAT_IS_WINRT
    {
        CREATEFILE2_EXTENDED_PARAMETERS cf2ex = {0};
        cf2ex.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
        cf2ex.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        cf2ex.dwFileFlags = FILE_FLAG_BACKUP_SEMANTICS;
        h = CreateFile2(epathname, 0, FILE_SHARE_READ, OPEN_EXISTING, &cf2ex);
    }
#else
    h = CreateFile(epathname, 0, (FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
#endif
    if (h == INVALID_HANDLE_VALUE) {
        rv = VPLError_XlatWinErrno(GetLastError());
        // check if the error is due to a bad path; if so, override the error code
        rc = _VPLFS__CheckValidWpath(wpathname);
        if (rc != VPL_OK) {
        rv = rc;
      }
      goto end;
    }

    // A file time is a 64-bit value that represents the number of 100-nanosecond
    // intervals that have elapsed since 12:00 A.M. January 1, 1601 Coordinated
    // Universal Time (UTC). The system records file times when applications
    // create, access, and write to files.
#ifdef VPL_PLAT_IS_WINRT
    {
        // 1) QuadPart is representing 100 Nanosec since Jan. 1, 1601 00:00
        // 2) 100 Nanosec -> Nanosecs
        // 3) Nanosecs -> VPLTime_t
        // 4) Starting Year 1601 -> Starting Year 1970 using EPOCH diff.
        FILE_BASIC_INFO fbi = {0};
        if (GetFileInformationByHandleEx(h, FileBasicInfo, &fbi, sizeof(fbi)) == 0) {
            rv = VPLError_XlatErrno(errno);
            goto end;
        }
        buf->vpl_atime  = VPLTime_DiffClamp(VPLTime_FromNanosec(fbi.LastAccessTime.QuadPart*100), EPOCH_DIFF);
        buf->vpl_mtime  = VPLTime_DiffClamp(VPLTime_FromNanosec(fbi.LastWriteTime.QuadPart*100), EPOCH_DIFF);
        buf->vpl_ctime  = VPLTime_DiffClamp(VPLTime_FromNanosec(fbi.CreationTime.QuadPart*100), EPOCH_DIFF);

        FILE_STANDARD_INFO fsi = {0};
        if (GetFileInformationByHandleEx(h, FileStandardInfo, &fsi, sizeof(fsi)) == 0) {
            rv = VPLError_XlatErrno(errno);
            goto end;
        }
        buf->size = fsi.EndOfFile.QuadPart;
    }
#else

    LARGE_INTEGER fsize;
    memset(&fsize, 0, sizeof(LARGE_INTEGER));
    
    if (GetFileSizeEx(h, &fsize) == 0) {
            rv = VPLError_XlatErrno(errno);
            goto end;
    }

    buf->size = fsize.QuadPart;

    rc = GetFileTime(h, &ctim, &atim, &mtim);
    if (!rc) {
        rv = VPLError_XlatErrno(errno);
        // check if the error is due to a bad path; if so, override the error code
        if (rc != VPL_OK) {
            rv = rc;
        }
        goto end;
    }

    // 1) Combine High and Low into a u64 integer, representing 100 Nanosec since Jan. 1, 1601 00:00
    // 2) 100 Nanosec -> Nanosecs
    // 3) Nanosecs -> VPLTime_t
    // 4) Starting Year 1601 -> Starting Year 1970 using EPOCH diff.
    buf->vpl_atime  = VPLTime_DiffClamp(VPLTime_FromNanosec(
                                            ((((u64)atim.dwHighDateTime)<<32) + ((u64)atim.dwLowDateTime)) * 100),
                                        EPOCH_DIFF);
    buf->vpl_mtime  = VPLTime_DiffClamp(VPLTime_FromNanosec(
                                            ((((u64)mtim.dwHighDateTime)<<32) + ((u64)mtim.dwLowDateTime)) * 100),
                                        EPOCH_DIFF);
    buf->vpl_ctime  = VPLTime_DiffClamp(VPLTime_FromNanosec(
                                            ((((u64)ctim.dwHighDateTime)<<32) + ((u64)ctim.dwLowDateTime)) * 100),
                                        EPOCH_DIFF);
#endif

    buf->atime = VPLTime_ToSec(buf->vpl_atime);
    buf->mtime = VPLTime_ToSec(buf->vpl_mtime);
    buf->ctime = VPLTime_ToSec(buf->vpl_ctime);
    
    buf->isHidden = VPL_FALSE;
    buf->isSymLink = VPL_FALSE;
    buf->isReadOnly = VPL_FALSE;
    buf->isSystem = VPL_FALSE;
    buf->isArchive = VPL_FALSE;

#ifdef VPL_PLAT_IS_WINRT
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesEx(epathname,
                             GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard,
                             &fileInfo)) {
        rv = VPL_ERR_NOENT;
        goto end;
    }
    fileAttrs = fileInfo.dwFileAttributes;
#else
    fileAttrs = GetFileAttributes(epathname);
#endif
    if(fileAttrs != INVALID_FILE_ATTRIBUTES) {
        if(fileAttrs & FILE_ATTRIBUTE_HIDDEN) {
            buf->isHidden = VPL_TRUE;
        }
        if(fileAttrs & FILE_ATTRIBUTE_REPARSE_POINT) {
            buf->isSymLink = VPL_TRUE;
        }
        if(fileAttrs & FILE_ATTRIBUTE_READONLY) {
            buf->isReadOnly = VPL_TRUE;
        }
        if(fileAttrs & FILE_ATTRIBUTE_SYSTEM) {
            buf->isSystem = VPL_TRUE;
        }
        if(fileAttrs & FILE_ATTRIBUTE_ARCHIVE) {
            buf->isArchive = VPL_TRUE;
        }
        buf->type = (fileAttrs & FILE_ATTRIBUTE_DIRECTORY) ? VPLFS_TYPE_DIR : VPLFS_TYPE_FILE;
    }

end:
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }
    if (wpathname != NULL) {
        free(wpathname);
    }
    if (epathname != NULL) {
        free(epathname);
    }
    return rv;
}

void VPLFS_Sync(void)
{
    // XXX what's the windows equivalent??
}

static int 
opendir(wchar_t *epathname, VPLFS_dir_t* dir)
{
    int rv = VPL_OK;
    WIN32_FIND_DATA *finddata = NULL;

    finddata = (WIN32_FIND_DATA*)malloc(sizeof(WIN32_FIND_DATA));
    if (finddata == NULL) {
        rv = VPL_ERR_NOMEM;
        goto end;
    }

#ifdef VPL_PLAT_IS_WINRT
    dir->handle = FindFirstFileEx(epathname, 
                                  FINDEX_INFO_LEVELS::FindExInfoStandard,
                                  finddata,
                                  FINDEX_SEARCH_OPS::FindExSearchNameMatch,
                                  NULL,
                                  0);
#else
    dir->handle = FindFirstFile(epathname, finddata);
#endif
    if (dir->handle == INVALID_HANDLE_VALUE) {
        // this could happen if
        // (1) name was bad
        // (2) name was good but no files/dirs found
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {  // case (2)
            ;  // empty
        }
        else {  // case (1)
            rv = VPL_ERR_NOENT;
            goto end;
        }
    }
    else {
        dir->finddata = finddata;
    }
    dir->name = epathname;
    dir->pos = 0;

 end:
    if (rv != VPL_OK) {
        free(finddata);
    }
    return rv;
}

int
VPLFS_Opendir(const char* pathname, VPLFS_dir_t* dir)
{
    int rv = VPL_OK;
    int rc = 0;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFS_Opendir(pathname, dir);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    if (dir == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    memset(dir, 0, sizeof(*dir));
    dir->handle = INVALID_HANDLE_VALUE;

    if (pathname == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (strlen(pathname) == 0) {
        rv = VPL_ERR_NOENT;
        goto end;
    }
    {
        VPLFS_stat_t stat;
        rv = VPLFS_Stat(pathname, &stat);
        if (rv != VPL_OK) {
            goto end;
        }
        if (stat.type != VPLFS_TYPE_DIR) {
            rv = VPL_ERR_NOTDIR;
            goto end;
        }
    }

    rc = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(wpathname, &epathname, 2);  // "2" for appending "/*" later
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    // Note:
    // FindFirstFile() cannot handle trailing slashes nor an empty string.
    // cf. http://msdn.microsoft.com/en-us/library/aa364418%28VS.85%29.aspx
    //
    // To work around it, we'll simply append "/*".
    // Examples:
    //  "/dir" -> "/dir/*"
    //  "/dir/" -> "/dir//*"  (double slash seems to be okay for win32)
    //
    if (epathname[wcslen(epathname)-1] == L'\\') {
        wcscat(epathname, L"*");
    }
    else {
        wcscat(epathname, L"\\*");
    }
    rc = opendir(epathname, dir);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }
    // On success, the #VPLFS_dir_t owns the name passed as first arg (and is responsible for freeing it in #VPLFS_Closedir()).

 end:
    if (epathname) {
        free(wpathname);
        if (rv != VPL_OK)
            free(epathname);
    }
    else {
        if (rv != VPL_OK)
            free(wpathname);
    }

    return rv;
}

int 
VPLFS_Closedir(VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsDirInLocalStatePath(dir);
        if (rc == VPL_ERR_NOENT)
            return _VPLFS_Closedir(dir);
        else if (rc != VPL_OK)
            return rc;
    }
#endif

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if (dir->handle == INVALID_HANDLE_VALUE) {
        rv = VPL_ERR_BADF;
    }
    else {
        BOOL rc = FindClose(dir->handle);
        if (rc == 0) {
            rv = VPL_ERR_BADF;
        }
        dir->handle = INVALID_HANDLE_VALUE;
        if (dir->finddata != NULL) {
            free(dir->finddata);
            dir->finddata = NULL;
        }
        if (dir->name != NULL) {
            free(dir->name);
            dir->name = NULL;
        }
    }
    
    return rv;
}

static int
copy_finddata(VPLFS_dirent_t *entry, WIN32_FIND_DATA *finddata)
{
    int rc;

    rc = _VPL__wstring_to_utf8(finddata->cFileName, -1, entry->filename, sizeof(entry->filename));
    if (rc != VPL_OK) {
        VPL_REPORT_WARN("%s failed: %d", "_VPL__wstring_to_utf8", rc);
        return rc;
    }

    if ((finddata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        entry->type = VPLFS_TYPE_DIR;
    }
    else {
        entry->type = VPLFS_TYPE_FILE;
    }

    return VPL_OK;
}

int 
VPLFS_Readdir(VPLFS_dir_t* dir, VPLFS_dirent_t* entry)
{
    int rv = VPL_OK;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsDirInLocalStatePath(dir);
        if (rc == VPL_ERR_NOENT)
            return _VPLFS_Readdir(dir, entry);
        else if (rc != VPL_OK)
            return rc;
    }
#endif

    if(dir == NULL || entry == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (dir->handle == INVALID_HANDLE_VALUE) {
        rv = VPL_ERR_BADF;
        goto end;
    }

    if (dir->finddata != NULL) {
        copy_finddata(entry, (WIN32_FIND_DATA*)dir->finddata);
        free(dir->finddata);
        dir->finddata = NULL;
    }
    else {
        WIN32_FIND_DATA finddata;
        BOOL rc = FindNextFile(dir->handle, &finddata);
        if (rc == 0) {
            // this could happen if
            // (1) handle was bad
            // (2) handle was good but no files/dirs found
            if (GetLastError() == ERROR_NO_MORE_FILES) {  // case (2)
                rv = VPL_ERR_MAX;
            }
            else {  // case (1)
                rv = VPL_ERR_BADF;
            }
            goto end;
        }
        copy_finddata(entry, &finddata);
    }
    dir->pos++;

 end:
    return rv;
}

int 
VPLFS_Rewinddir(VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsDirInLocalStatePath(dir);
        if (rc == VPL_ERR_NOENT)
            return _VPLFS_Rewinddir(dir);
        else if (rc != VPL_OK)
            return rc;
    }
#endif

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if (dir->handle == INVALID_HANDLE_VALUE) {
        rv = VPL_ERR_BADF;
    }
    else if (FindClose(dir->handle) == 0) {
        rv = VPL_ERR_BADF;
    }
    else {
        int rc = opendir(dir->name, dir);
        if (rc != VPL_OK) {
            free(dir->name);
            dir->name = NULL;
            rv = rc;
        }
    }
    
    return rv;
}

int 
VPLFS_Seekdir(VPLFS_dir_t* dir, size_t pos)
{
    int rv = VPL_OK;
   
#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsDirInLocalStatePath(dir);
        if (rc == VPL_ERR_NOENT)
            return _VPLFS_Seekdir(dir, pos);
        else if (rc != VPL_OK)
            return rc;
    }
#endif

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (dir->handle == INVALID_HANDLE_VALUE) {
        rv = VPL_ERR_BADF;
        goto end;
    }

    if ((int)pos < dir->pos) {
        VPLFS_Rewinddir(dir);
    }

    while (dir->pos < (int)pos) {
        VPLFS_dirent_t dummy;
        int rc = VPLFS_Readdir(dir, &dummy);
        if (rc != VPL_OK) {
            rv = rc;
            goto end;
        }
    }

 end:    
    return rv;
}

int 
VPLFS_Telldir(VPLFS_dir_t* dir, size_t* pos)
{
    int rv = VPL_OK;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsDirInLocalStatePath(dir);
        if (rc == VPL_ERR_NOENT)
            return _VPLFS_Telldir(dir, pos);
        else if (rc != VPL_OK)
            return rc;
    }
#endif

    if(dir == NULL || pos == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if (dir->handle == INVALID_HANDLE_VALUE) {
        rv = VPL_ERR_BADF;
    }
    else {
        *pos = dir->pos;
    }
    
    return rv;
}

int 
VPLFS_Mkdir(const char* pathname)
{
    int rv = VPL_OK;
    int rc;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFS_Mkdir(pathname);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    if (pathname == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (strlen(pathname) == 0) {
        rv = VPL_ERR_NOENT;
        goto end;
    }

    rc = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    if (CreateDirectory(epathname, NULL) == 0) {
        rv = VPLError_XlatWinErrno(GetLastError());
        // check if the error is due to a bad path; if so, override the error code
        rc = _VPLFS__CheckValidWpath(wpathname);
        if (rc != VPL_OK) {
            rv = rc;
        }
        goto end;
    }

 end:
    if (wpathname != NULL) {
        free(wpathname);
    }
    if (epathname != NULL) {
        free(epathname);
    }
    return rv;
}

int 
VPLFS_Rmdir(const char* pathname)
{
    int rv = VPL_OK;
    int rc;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFS_Rmdir(pathname);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    if (pathname == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (strlen(pathname) == 0) {
        rv = VPL_ERR_NOENT;
        goto end;
    }
    if (strcmp(pathname, ".") == 0) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    rc = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    if (RemoveDirectory(epathname) == 0) {
        rv = VPLError_XlatWinErrno(GetLastError());
        // check if the error is due to a bad path; if so, override the error code
        rc = _VPLFS__CheckValidWpath(wpathname);
        if (rc != VPL_OK) {
            rv = rc;
        }
        goto end;
    }

 end:
    if (wpathname != NULL) {
        free(wpathname);
    }
    if (epathname != NULL) {
        free(epathname);
    }
    return rv;
}

int 
VPLFS_GetSpace(const char* directory, u64* disk_size, u64* avail_size)
{
    int rv = VPL_OK;
    int rc;
    wchar_t *wdirectory = NULL;
    wchar_t *edirectory = NULL;
    unsigned __int64 avail, disk, free_size;

    if (directory == NULL || disk_size == NULL || avail_size == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (strlen(directory) == 0) {
        rv = VPL_ERR_NOENT;
        goto end;
    }

    rc = _VPL__utf8_to_wstring(directory, &wdirectory);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(wdirectory, &edirectory, 0);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    if (!GetDiskFreeSpaceEx(edirectory,
                            (PULARGE_INTEGER)&avail,
                            (PULARGE_INTEGER)&disk,
                            (PULARGE_INTEGER)&free_size)) {
        VPL_REPORT_WARN("%s failed: %s %d", "GetDiskFreeSpaceEx", directory, rc);
        rv = VPLError_XlatWinErrno(GetLastError());
        goto end;
    }

    *disk_size = (u64) disk;
    *avail_size = (u64) avail;

end:
    if (wdirectory != NULL) {
        free(wdirectory);
    }
    if (edirectory != NULL) {
        free(edirectory);
    }
    return rv;
}

/* There are a number of functions to call to get paths to specific named folders.
 * However, as of this writing (March 2012),
 * SHGetFolderPath is deprecated,
 * SHGetSpecialFolderPath is not supported.
 * It appears that Microsoft wants us to use SHGetKnownFolderPath.
 * However, mingw (3.17-2) does not know about this function.
 */

enum _folder_type {
#ifdef VPL_PLAT_IS_WINRT
    _FOLDER_SETTINGS = 1,
#else
    _FOLDER_PROFILE = 1,
    _FOLDER_DESKTOP = 4,
    _FOLDER_PUBLIC_DESKTOP = 5,
#endif
    _FOLDER_LOCALAPPDATA = 2,
    _FOLDER_LIBRARIES = 3,
};

#ifdef VPL_PLAT_IS_WINRT
static int _Win_GetLocalFolderWPath(wchar_t **wpath)
{
    int rv = VPL_OK;
    size_t wpath_leng = 0;
    Platform::String^ localPath;
    if (wpath == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    localPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path;

    wpath_leng = localPath->Length() + 1;
    *wpath = (wchar_t*)malloc(sizeof(wchar_t) * wpath_leng);
    if (*wpath == NULL) {
        rv = VPL_ERR_NOMEM;
        goto end;
    }
    memset(*wpath, 0, sizeof(wchar_t) * wpath_leng);
    wcsncpy(*wpath, localPath->Data(), wpath_leng-1);
    
end:
    return rv;
}
#else
// Based on sample code at http://msdn.microsoft.com/en-us/library/windows/desktop/aa387705%28v=vs.85%29.aspx
int modifyPrivilege(
    IN LPCTSTR szPrivilege,
    IN BOOL fEnable)
{
    int rv = 0;
    TOKEN_PRIVILEGES NewState;
    LUID             luid;
    HANDLE hToken    = NULL;

    // Open the process token for this process.
    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken )) {
            rv = VPLError_XlatWinErrno(GetLastError());
            VPL_REPORT_WARN("Failed OpenProcessToken: %d", rv);
            return rv;
    }

    // Get the local unique ID for the privilege.
    if ( !LookupPrivilegeValue( NULL,
        szPrivilege,
        &luid )) {
            rv = VPLError_XlatWinErrno(GetLastError());
            VPL_REPORT_WARN("Failed LookupPrivilegeValue: %d", rv);
            CloseHandle( hToken );
            return rv;
    }

    // Assign values to the TOKEN_PRIVILEGE structure.
    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid = luid;
    NewState.Privileges[0].Attributes = 
        (fEnable ? SE_PRIVILEGE_ENABLED : 0);

    // Adjust the token privilege.
    if (!AdjustTokenPrivileges(hToken,
        FALSE,
        &NewState,
        0,
        NULL,
        NULL)) {
            rv = VPLError_XlatWinErrno(GetLastError());
            VPL_REPORT_WARN("Failed AdjustTokenPrivileges: %d", rv);
    }

    // Close the handle.
    CloseHandle(hToken);

    return rv;
}

/// On success (rv == 0), caller must free() *wuserprofile_path.
int getProfileImagePath(char *strSid, wchar_t **wuserprofile_path)
{
    int rv = VPL_OK;
    static const char *HKEY_PROFILE_PREFIX = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\";

    size_t profile_hkey_length = strlen(HKEY_PROFILE_PREFIX) + strlen(strSid) + 1;

#if defined(_DEBUG)
    VPL_REPORT_INFO("User SID = %s, profile_hkey_length = %u", strSid, profile_hkey_length);
#endif //defined(_DEBUG)

    char *profile_hkey = (char *)malloc(profile_hkey_length);
    if (profile_hkey == NULL) {
        VPL_REPORT_WARN("Unable to allocate memory for profile HKEY");
        rv = VPL_ERR_NOMEM;
        return rv;
    }
    ON_BLOCK_EXIT(free, profile_hkey);

    // concat the prefix with the user SID
    strcpy(profile_hkey, HKEY_PROFILE_PREFIX);
    strcat(profile_hkey, strSid);

#if defined(_DEBUG)
    VPL_REPORT_INFO("HKEY = %s", profile_hkey);
#endif //defined(_DEBUG)

    // lookup registry
    HKEY hkey;
    DWORD result;
    result = RegOpenKeyA(HKEY_LOCAL_MACHINE, profile_hkey, &hkey);
    if (result != ERROR_SUCCESS) {
        rv = VPLError_XlatWinErrno(result);
        VPL_REPORT_WARN("Unable to open the registry for HKEY = %s, rv = %d", profile_hkey, rv);
        return rv;
    }
    ON_BLOCK_EXIT(RegCloseKey, hkey);

    DWORD key_length;
    DWORD key_type;
    result = RegQueryValueExW(hkey,
                              L"ProfileImagePath",
                              NULL,
                              &key_type,
                              NULL,
                              &key_length);
    if (result != ERROR_SUCCESS) {
        rv = VPLError_XlatWinErrno(result);
        VPL_REPORT_WARN("Unable to get registry entry for HKEY = %s\\ProfileImagePath, rv = %d", profile_hkey, rv);
        return rv;
    }

#if defined(_DEBUG)
    VPL_REPORT_INFO("HKEY lookup success, type = %u, length = %u", key_type, key_length);
#endif //defined(_DEBUG)

    *wuserprofile_path = (wchar_t*)malloc(key_length);
    if (*wuserprofile_path == NULL) {
        VPL_REPORT_WARN("Unable to allocate memory for user profile path");
        rv = VPL_ERR_NOMEM;
        return rv;
    }

    result = RegQueryValueExW(hkey,
                              L"ProfileImagePath",
                              NULL,
                              &key_type,
                              (LPBYTE)*wuserprofile_path,
                              &key_length);
    if (result != ERROR_SUCCESS) {
        rv = VPLError_XlatWinErrno(result);
        VPL_REPORT_WARN("Failed to retrieve user profile folder path, rv = %d", rv);
    }

    if (rv != 0) {
        free(*wuserprofile_path);
        *wuserprofile_path = NULL;
    } else {
        // caller will free wuserprofile_path.
    }
    return rv;
}

/// On success (rv == 0), caller must free() *wpath.
static int _Win_GetKnownFolderWPath(REFKNOWNFOLDERID rfid, wchar_t **wpath)
{
    int rv = VPL_OK;

    if (wpath == NULL) {
        rv = VPL_ERR_INVALID;
        goto out;
    }

    PSID sid = _VPL__GetUserSid();
    if (sid != NULL) {  // the process is running as the local system user; the user's SID is in "sid"

        // Ideally, we want to get an impersonation token and use that in a call to SHGetKnownFolderPath().
        // However, getting an impersonation token is not always easy, so we've given up on it.
        // Instead, we will lookup the username and manually construct the path.

        BOOL isOk;

        // First, call LookupAccountSid with null buffers, to determine the size of the buffers needed.
        DWORD usernamelen = 0;
        DWORD domainnamelen = 0;
        SID_NAME_USE use;
        isOk = LookupAccountSid(NULL,
                                sid,
                                NULL, &usernamelen,
                                NULL, &domainnamelen,
                                &use);
        // We expect this to fail with ERROR_INSUFFICIENT_BUFFER; bail out if it somehow succeeds.
        if (isOk) {
            rv = VPL_ERR_FAIL;
            goto out;
        }
        DWORD winerrno = GetLastError();
        if (winerrno != ERROR_INSUFFICIENT_BUFFER) {
            rv = VPLError_XlatWinErrno(winerrno);
            goto out;
        }

        wchar_t *username = (wchar_t*)malloc(sizeof(wchar_t) * usernamelen);
        if (username == NULL) {
            rv = VPL_ERR_NOMEM;
            goto out;
        }
        ON_BLOCK_EXIT(free, username);
        wchar_t *domainname = (wchar_t*)malloc(sizeof(wchar_t) * domainnamelen);
        if (domainname == NULL) {
            rv = VPL_ERR_NOMEM;
            goto out;
        }
        ON_BLOCK_EXIT(free, domainname);

        // Call LookupAccountSid() again with buffers of the correct size.
        isOk = LookupAccountSid(NULL,
                                sid,
                                username, &usernamelen,
                                domainname, &domainnamelen,
                                &use);
        if (!isOk) {
            rv = VPLError_XlatWinErrno(GetLastError());
            goto out;
        }

        char *strSid = NULL;
        if (ConvertSidToStringSidA(sid, &strSid) == 0) {
            rv = VPLError_XlatWinErrno(GetLastError());
            VPL_REPORT_WARN("Unable to convert SID to string, rv = %d", rv);
            goto out;
        }
        ON_BLOCK_EXIT(LocalFree, strSid);

        if (rfid == FOLDERID_Desktop) {
            // Bug 10824: compose registry key path by user sid to lookup desktop absolute path
            static const char *HKEY_SHELLFOLDER_SUFFIX = "\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
            char *shellfolder_hkey = NULL;
            size_t shellfolder_hkey_length;

            shellfolder_hkey_length = strlen(HKEY_SHELLFOLDER_SUFFIX) + strlen(strSid) + 1;
            shellfolder_hkey = (char *)malloc(shellfolder_hkey_length);
            if (shellfolder_hkey == NULL) {
                VPL_REPORT_WARN("Unable to allocate memory for profile HKEY");
                rv = VPL_ERR_NOMEM;
                goto out;
            }
            ON_BLOCK_EXIT(free, shellfolder_hkey);

            // concat the prefix with the user SID
            strcpy(shellfolder_hkey, strSid);
            strcat(shellfolder_hkey, HKEY_SHELLFOLDER_SUFFIX);

            // lookup registry
            HKEY hkey;
            DWORD result;
            BOOL hasBackupPrivilege = FALSE;
            BOOL hasRestorePrivilege = FALSE;
            BOOL isLoadFromHive = FALSE;
            BOOL isKeyLoaded = FALSE;
            
            result = RegOpenKeyA(HKEY_USERS, shellfolder_hkey, &hkey);
            if (result == ERROR_FILE_NOT_FOUND) {
                // Bug 12918: OS does not load RegKey HKEY_USERS from hive, before user login a Windows account
                // Solution: Force to load from hive
                VPL_REPORT_WARN("Try to load from hive. Unable to open the registry for HKEY = %s", shellfolder_hkey);
                rv = modifyPrivilege(SE_BACKUP_NAME, TRUE);
                if (rv != 0) {
                    VPL_REPORT_WARN("Failed to modify privilege, SE_BACKUP_NAME: %d", rv);
                    goto cleanup;
                } else {
                    hasBackupPrivilege = TRUE;
                }
                rv = modifyPrivilege(SE_RESTORE_NAME, TRUE);
                if (rv != 0) {
                    VPL_REPORT_WARN("Failed to modify privilege, SE_RESTORE_NAME: %d", rv);
                    goto cleanup;
                } else {
                    hasRestorePrivilege = TRUE;
                }
                // Get the ProfileImagePath
                wchar_t *wuserprofile_path = NULL;
                rv = getProfileImagePath(strSid, &wuserprofile_path);
                if (rv != VPL_OK) {
                    // Error code is recorded in getProfileImagePath()
                    VPL_REPORT_WARN("Failed to get ProfileImagePath");
                    goto out;
                }
                ON_BLOCK_EXIT(free, wuserprofile_path);
                // convert to forward slashes
                size_t len_userprofile_path = wcslen(wuserprofile_path);
                for (size_t i = 0; i < len_userprofile_path + 1; i++) {
                    if (wuserprofile_path[i] == L'\\') {
                        wuserprofile_path[i] = L'/';
                    }
                }
                // Add "/NTUSER.DAT", length = 11
                size_t bufsize = wcslen(wuserprofile_path) + 11 + 1;
                wchar_t *whive_path = (wchar_t*)malloc(sizeof(wchar_t) * bufsize);
                if (whive_path == NULL) {
                    rv = VPL_ERR_NOMEM;
                    goto out;
                }
                ON_BLOCK_EXIT(free, whive_path);

                wcscpy(whive_path, wuserprofile_path);
                wcscat(whive_path, L"/NTUSER.DAT");  

                int nIndex = MultiByteToWideChar(CP_ACP, 0, strSid, -1, NULL, 0);
                {
                    TCHAR *wstrSid = new TCHAR[nIndex + 1];
                    MultiByteToWideChar(CP_ACP, 0, strSid, -1, wstrSid, nIndex);

                    VPL_REPORT_INFO("Load from Hive, path = %S, Sid = %S", whive_path, wstrSid);
                    result = RegLoadKey(HKEY_USERS, wstrSid, whive_path);
                    delete wstrSid;
                }
                if (result != ERROR_SUCCESS) {
                    rv = VPLError_XlatWinErrno(result);
                    VPL_REPORT_WARN("Unable to load NTUSER.DAT, result = "FMT_DWORD, result);
                    goto cleanup;
                } else {
                    isLoadFromHive = TRUE;
                }
                result = RegOpenKeyA(HKEY_USERS, shellfolder_hkey, &hkey);
                if (result != ERROR_SUCCESS) {
                    rv = VPLError_XlatWinErrno(result);
                    VPL_REPORT_WARN("Unable to open the registry after reload RegKey from hive for HKEY = %s, result = "FMT_DWORD, shellfolder_hkey, result);
                    goto cleanup;
                }
            } else if (result != ERROR_SUCCESS) {
                rv = VPLError_XlatWinErrno(result);
                VPL_REPORT_WARN("Unable to open the registry for HKEY = %s, rv = %d", shellfolder_hkey, rv);
                goto cleanup;
            } else {
                // result == ERROR_SUCCESS
                isKeyLoaded = TRUE;
            }

            DWORD key_length;
            DWORD key_type;

            result = RegQueryValueExW(hkey,
                                      L"Desktop",
                                      NULL,
                                      &key_type,
                                      NULL,
                                      &key_length);
            if (result != ERROR_SUCCESS) {
                rv = VPLError_XlatWinErrno(result);
                VPL_REPORT_WARN("Unable to get registry entry for HKEY = %s\\Desktop, rv = %d", shellfolder_hkey, rv);
                goto cleanup;
            }

            *wpath = (wchar_t*)malloc(key_length);
            if (*wpath == NULL) {
                VPL_REPORT_WARN("Unable to allocate memory for user profile path");
                rv = VPL_ERR_NOMEM;
                goto cleanup;
            }

            result = RegQueryValueExW(hkey,
                                      L"Desktop",
                                      NULL,
                                      &key_type,
                                      (LPBYTE)*wpath,
                                      &key_length);

            if (result != ERROR_SUCCESS) {
                free(*wpath);
                *wpath = NULL;
                rv = VPLError_XlatWinErrno(result);
                VPL_REPORT_WARN("Failed to retrieved user profile folder path, rv = %d", rv);
                goto cleanup;
            }

            // convert to forward slashes
            for (size_t i = 0; i < wcslen(*wpath) + 1; i++) {
                if ((*wpath)[i] == L'\\') {
                    (*wpath)[i] = L'/';
                }
            }
cleanup:
            if (isKeyLoaded) {
                RegCloseKey(hkey);
            }
            if (isLoadFromHive) {
                RegUnLoadKeyA(HKEY_USERS, strSid);
            }
            if (hasBackupPrivilege) {
                modifyPrivilege(SE_BACKUP_NAME, FALSE);
            }
            if (hasRestorePrivilege) {
                modifyPrivilege(SE_RESTORE_NAME, FALSE);
            }
            goto out;
        }
        else { // (rfid != FOLDERID_Desktop)
            wchar_t *wuserprofile_path = NULL;
            rv = getProfileImagePath(strSid, &wuserprofile_path);
            if (VPL_OK != rv) {
                // Error code is recorded in getProfileImagePath()
                VPL_REPORT_WARN("Failed to get ProfileImagePath.");
                goto out;
            }
            ON_BLOCK_EXIT(free, wuserprofile_path);
            // convert to forward slashes
            size_t len_userprofile_path = wcslen(wuserprofile_path);
            for (size_t i = 0; i < len_userprofile_path + 1; i++) {
                if (wuserprofile_path[i] == L'\\') {
                    wuserprofile_path[i] = L'/';
                }
            }

#define handle_folder(folder_name, path_prefix, path_suffix)                \
            if (rfid == FOLDERID_##folder_name) {                           \
                size_t bufsize = wcslen(path_prefix) + ARRAY_ELEMENT_COUNT(path_suffix) + 1; \
                *wpath = (wchar_t*)malloc(sizeof(wchar_t) * bufsize);       \
                if (*wpath == NULL) {                                       \
                    rv = VPL_ERR_NOMEM;                                     \
                    goto out;                                               \
                }                                                           \
                wcscpy(*wpath, path_prefix);                                \
                wcscat(*wpath, path_suffix);                                \
                goto out;                                                   \
            }

            handle_folder(Profile, wuserprofile_path, L"");
            handle_folder(LocalAppData, wuserprofile_path, L"/AppData/Local");
            handle_folder(Libraries, wuserprofile_path, L"/AppData/Roaming/Microsoft/Windows/Libraries");
#undef handle_folder
        }
    }

    {
        // this means the process is running as a normal user or non-filtered folder ID
        wchar_t *wpathbuf = NULL;
        HRESULT hr = SHGetKnownFolderPath(rfid, 0, NULL, &wpathbuf);
        if (FAILED(hr)) {
            rv = VPL_ERR_FAIL;
            goto out;
        }
        ON_BLOCK_EXIT(CoTaskMemFree, wpathbuf);

        *wpath = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(wpathbuf) + 1));
        if (*wpath == NULL) {
            VPL_REPORT_WARN("Out of memory while allocating buffer for get known folder");
            rv = VPL_ERR_NOMEM;
            goto out;
        }
        for (size_t i = 0; i < wcslen(wpathbuf) + 1; i++) {
            if (wpathbuf[i] == L'\\')
                (*wpath)[i] = L'/';
            else
                (*wpath)[i] = wpathbuf[i];
        }
    }

out:
    return rv;
}
#endif

// Get UTF16 path to specified folder.
// Function allocates memory to store the path.
// Caller is responsible for calling free() to free the memory after use.
static int _GetFolderWPath(enum _folder_type folder, wchar_t **wpath)
{
    int rv = VPL_OK;

    if (wpath == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    switch (folder) {
#ifndef VPL_PLAT_IS_WINRT
    case _FOLDER_PROFILE:
#if defined(VPL_PLAT_IS_WINRT)
        rv = _Win_GetCurrentFolderWPath(folder, wpath);
#else
        rv = _Win_GetKnownFolderWPath(FOLDERID_Profile, wpath);
#endif
        break;
    case _FOLDER_DESKTOP:
        rv = _Win_GetKnownFolderWPath(FOLDERID_Desktop, wpath);
        break;
    case _FOLDER_PUBLIC_DESKTOP:
        rv = _Win_GetKnownFolderWPath(FOLDERID_PublicDesktop, wpath);
        break;
#endif
    case _FOLDER_LOCALAPPDATA:
#if defined(VPL_PLAT_IS_WINRT)
        rv = _Win_GetLocalFolderWPath(wpath);
#else
        rv = _Win_GetKnownFolderWPath(FOLDERID_LocalAppData, wpath);
#endif
        break;
    case _FOLDER_LIBRARIES:
#if defined(VPL_PLAT_IS_WINRT)
        //rv = _Win_GetLocalFolderWPath(wpath);  // FIXME
#else
        rv = _Win_GetKnownFolderWPath(FOLDERID_Libraries, wpath);
#endif
        break;
    default:
        rv = VPL_ERR_INVALID;
    }

 end:
    return rv;
}

// Get UTF8 path to specified folder.
// Function allocates memory to store the path.
// Caller is responsible for calling free() to free the memory after use.
static int _GetFolderUPath(enum _folder_type folder, char **upath)
{
    int rv = VPL_OK;
    wchar_t *wpath = NULL;

    if (upath == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    rv = _GetFolderWPath(folder, &wpath);
    if (rv != VPL_OK)
        goto end;

    rv = _VPL__wstring_to_utf8_alloc(wpath, upath);

 end:
    if (wpath != NULL)
        free(wpath);
    return rv;
}

#ifndef VPL_PLAT_IS_WINRT
int _VPLFS__GetProfilePath(char **upath)
{
    return _GetFolderUPath(_FOLDER_PROFILE, upath);
}

int _VPLFS__GetProfileWPath(wchar_t **wpath)
{
    return _GetFolderWPath(_FOLDER_PROFILE, wpath);
}
#endif

int _VPLFS__GetLocalAppDataPath(char **upath)
{
    return _GetFolderUPath(_FOLDER_LOCALAPPDATA, upath);
}

int _VPLFS__GetLocalAppDataWPath(wchar_t **wpath)
{
    return _GetFolderWPath(_FOLDER_LOCALAPPDATA, wpath);
}

int _VPLFS__GetLibrariesPath(char **upath)
{
    return _GetFolderUPath(_FOLDER_LIBRARIES, upath);
}

int _VPLFS__GetLibrariesWPath(wchar_t **wpath)
{
    return _GetFolderWPath(_FOLDER_LIBRARIES, wpath);
}

#ifndef VPL_PLAT_IS_WINRT
int _VPLFS__GetDesktopPath(char **upath)
{
    return _GetFolderUPath(_FOLDER_DESKTOP, upath);
}

int _VPLFS__GetDesktopWPath(wchar_t **wpath)
{
    return _GetFolderWPath(_FOLDER_DESKTOP, wpath);
}
int _VPLFS__GetPublicDesktopPath(char **upath)
{
    return _GetFolderUPath(_FOLDER_PUBLIC_DESKTOP, upath);
}

int _VPLFS__GetPublicDesktopWPath(wchar_t **wpath)
{
    return _GetFolderWPath(_FOLDER_PUBLIC_DESKTOP, wpath);
}
#endif

#ifndef VPL_PLAT_IS_WINRT
int _VPLFS__GetShortcutDetail(const std::string& targetpath, std::string& upath, std::string& type, std::string& args)
{
    int rv = VPL_ERR_FAIL;

    HRESULT hRes = E_FAIL;
    IShellLink* ipShellLink = NULL;
    IPersistFile* ipPersistFile = NULL;
    // buffer that receives the null-terminated string 
    // for the drive and path
    wchar_t szPath[MAX_PATH];
    // buffer that receives the null-terminated 
    // string for the arguments
    wchar_t szArgs[MAX_PATH];

    ComInitGuard comInit;
    hRes = comInit.init(COINIT_MULTITHREADED);
    if (FAILED(hRes)) {
        VPL_REPORT_FATAL("ComInit failed: "FMT_HRESULT, hRes);
        rv = VPL_ERR_FAIL;
        goto out;
    }

    // Get a pointer to the IShellLink interface
    hRes = CoCreateInstance(CLSID_ShellLink,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (void**)&ipShellLink);
    if (FAILED(hRes)) {
        VPL_REPORT_FATAL("CoCreateInstance failed: "FMT_HRESULT, hRes);
        rv = VPL_ERR_FAIL;
        goto out;
    }

    {
        wchar_t *wpath = NULL;
        rv = _VPL__utf8_to_wstring(targetpath.c_str(), &wpath);
        if (rv != VPL_OK)
            goto out;
        // Get a pointer to the IPersistFile interface
        hRes = ipShellLink->QueryInterface(IID_IPersistFile, (void**)&ipPersistFile);
        if (hRes != S_OK) {
            VPL_REPORT_WARN("QueryInterface failed: "FMT_HRESULT, hRes);
            rv = VPL_ERR_FAIL;
            goto out;
        }
        // Open the shortcut file and initialize it from its contents
        hRes = ipPersistFile->Load(wpath, STGM_READ);
        free(wpath);
        if (SUCCEEDED(hRes)) {
            hRes = ipShellLink->GetPath(szPath, MAX_PATH, NULL, SLGP_RAWPATH);
            if (FAILED(hRes)) {
                VPL_REPORT_WARN("GetPath failed: "FMT_HRESULT, hRes);
                rv = VPL_ERR_FAIL;
                goto out;
            }
            else {
                char *path;
                ::_VPL__wstring_to_utf8_alloc(szPath, &path);
                upath = path;
                free(path);
            }
            // Get the arguments of the target
            hRes = ipShellLink->GetArguments(szArgs, MAX_PATH);
            if (FAILED(hRes)) {
                VPL_REPORT_WARN("GetArguments failed: "FMT_HRESULT, hRes);
                rv = VPL_ERR_FAIL;
                goto out;
            }
            else {
                char *strArgs;
                ::_VPL__wstring_to_utf8_alloc(szArgs, &strArgs);
                args = strArgs;
                free(strArgs);
            }
            if (wcsstr(szPath, L"%") == NULL) {
                // Check type if target path doesn't contain environment variables
                DWORD attr = GetFileAttributes(szPath);
                if (attr == INVALID_FILE_ATTRIBUTES) {
                    DWORD err = GetLastError();
                    VPL_REPORT_WARN("Error get attribute: "FMT_DWORD, err);
                    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
                        // Error Case that Target Path does not exist. It is a 64-bit only issue.
                        // When using GetPath on 64-bit machine with a 32-bit application, the target path will sometimes be wrong.
                        // For example: C:\Program Files\123.exe. GetPath will return C:\Program Files (x86)\123.exe
                        // http://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/6f2e7920-50a9-459d-bfdd-316e459e87c0/ishelllink-getpath-returns-wrong-folder-for-64-bit-application-when-called-from-32-bit-application

                        std::wstring targetPath(szPath);
                        size_t index = targetPath.find(L"\\Program Files (x86)\\");
                        if (index == std::string::npos) {
                            rv = VPLError_XlatWinErrno(err);
                            goto out;
                        } else {
                            // 21 is the number of characters in "\\Program Files (x86)\\"
                            targetPath.replace(index, 21, L"\\Program Files\\");
                            attr = GetFileAttributes(targetPath.c_str());
                            if (attr == INVALID_FILE_ATTRIBUTES) {
                                rv = VPLError_XlatWinErrno(GetLastError());
                                goto out;
                            } else {
                                char *path;
                                ::_VPL__wstring_to_utf8_alloc(targetPath.c_str(), &path);
                                upath = path;
                                free(path);
                            }
                        }
                    } else {
                        rv = VPLError_XlatWinErrno(err);
                        goto out;
                    }
                }

                if (attr & FILE_ATTRIBUTE_DIRECTORY) {
                    type = "dir";
                }
                else {
                    type = "file";
                }
            }
            rv = VPL_OK;
        } 
    } 
out:
    if (ipShellLink)
        ipShellLink->Release();
    if (ipPersistFile)
        ipPersistFile->Release();
    return rv;
}

int _VPLFS__GetDisplayName(const std::string &upath, std::string &displayname)
{
    int rv = VPL_OK;
    int rc = VPL_OK;
    wchar_t *wpath = NULL;
    wchar_t *wdisplayname = NULL;
    char *udisplayname = NULL;
    IShellItem *libfile = NULL;
    HRESULT hr;

    ComInitGuard comInit;
    hr = comInit.init(COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        VPL_REPORT_FATAL("ComInit failed: "FMT_HRESULT, hr);
        rv = VPL_ERR_FAIL;
        goto out;
    }

    // convert Libraries/Music/... to wpath (wchar)
    rv = _VPL__utf8_to_wstring(upath.c_str(), &wpath);
    if (rv != VPL_OK) {
        goto out;
    }

    // replace slash with backslash
    // SHCreateItemFromParsingName() doesn't like forward-slash, so replace.
    for (wchar_t *p = wpath; *p != L'\0'; p++) {
        if (*p == L'/')
            *p = L'\\';
    }

    hr = SHCreateItemFromParsingName(wpath, NULL, IID_PPV_ARGS(&libfile));
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }

    hr = libfile->GetDisplayName(SIGDN_NORMALDISPLAY, &wdisplayname);
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }

    // localized libinfo
    rv = _VPL__wstring_to_utf8_alloc(wdisplayname, &udisplayname);
    if (rv != VPL_OK) {
        goto out;
    }

    displayname = udisplayname;

out:
    if (wdisplayname != NULL) {
        CoTaskMemFree(wdisplayname);
        wdisplayname = NULL;
    }
    if (udisplayname != NULL) {
        free (udisplayname);
        udisplayname = NULL;
    }
    if (libfile != NULL) {
        libfile->Release();
        libfile = NULL;
    }
    return rv;
}

int _VPLFS__LocalizedPath(const std::string &upath, std::string &displayname)
{
    if(upath.size() <= 1) {
        displayname = upath;
        return 0;
    }

    int rv = 0;
    for (int i = 1; upath[i]; i++) {
        if (upath[i] == '\\') {
            std::string tempPath = upath.substr(0, i);
            std::string toAppend;
            rv = _VPLFS__GetDisplayName(tempPath, toAppend);
            if (rv != 0) {
                VPL_REPORT_WARN("_VPLFS__GetDisplayName(%s):%d", tempPath.c_str(), rv);
                return rv;
            }

            if (!displayname.empty()) {
                displayname.append(std::string("\\"));
            }
            displayname.append(toAppend);
        }
    }

    std::string toAppend;
    rv = _VPLFS__GetDisplayName(upath, toAppend);
    if (rv != 0) {
        VPL_REPORT_WARN("_VPLFS__GetDisplayName(%s):%d", upath.c_str(), rv);
        return rv;
    }
    if (!displayname.empty()) {
        displayname.append(std::string("\\"));
    }
    displayname.append(toAppend);

    return rv;
}

int _VPLFS__GetLibraryFolders(const std::string &upath, _VPLFS__LibInfo *libinfo)
{
    int rv = VPL_OK;
    int rc = VPL_OK;
    // don't compile if mingw - mingw doesn't have IShellItem (among others) 
    wchar_t *wpath = NULL;
    IShellItem *libfile = NULL;
    wchar_t *wlibname = NULL;
    char *ulibname = NULL;
    IShellLibrary *lib = NULL;
    IShellItemArray *folders = NULL;
    DWORD nfolders;
    HRESULT hr;
    size_t start, end;
    std::string tempstr;
    wchar_t *wsystem_folder = NULL;
    wchar_t *wuserpf_folder = NULL;

    // make sure libinfo is not null
    if (libinfo == NULL) {
        rv = VPL_ERR_FAIL;
        goto out;
    }

    // convert Libraries/Music/... to wpath (wchar)
    rv = _VPL__utf8_to_wstring(upath.c_str(), &wpath);
    if (rv != VPL_OK) {
        goto out;
    }

    // replace slash with backslash
    // SHCreateItemFromParsingName() doesn't like forward-slash, so replace.
    for (wchar_t *p = wpath; *p != L'\0'; p++) {
        if (*p == L'/')
            *p = L'\\';
    }

    hr = SHCreateItemFromParsingName(wpath, NULL, IID_PPV_ARGS(&libfile));
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }

    hr = libfile->GetDisplayName(SIGDN_NORMALDISPLAY, &wlibname);
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }

    // localized libinfo
    rv = _VPL__wstring_to_utf8_alloc(wlibname, &ulibname);
    if (rv != VPL_OK) {
        goto out;
    }
    libinfo->l_name = ulibname;

    if (wlibname != NULL) {
        CoTaskMemFree(wlibname);
        wlibname = NULL;
    }
    if (ulibname != NULL) {
        free(ulibname);
        ulibname = NULL;
    }
    // local-neutral libinfo
    hr = libfile->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &wlibname);
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }
    rv = _VPL__wstring_to_utf8_alloc(wlibname, &ulibname);
    if (rv != VPL_OK) {
        goto out;
    }
    // Assume the format looks like ::{{xxxxxx}\<TOKEN>.library-ms
    // OR C:\xxxxxx\<TOKEN>.library-ms
    tempstr = ulibname;
    if ((start = tempstr.find_last_of("\\")) != std::string::npos &&
        (end = tempstr.find(".library-ms")) != std::string::npos) {
        libinfo->n_name.assign(tempstr.substr(start+1, end-start-1));
    } else {
        // fallback to localized one...
        libinfo->n_name = libinfo->l_name;
    }

    hr = SHLoadLibraryFromItem(libfile, STGM_READ, IID_PPV_ARGS(&lib));
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }

    hr = lib->GetFolders(LFF_FORCEFILESYSTEM, IID_PPV_ARGS(&folders));
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }

    {
        FOLDERTYPEID folder_type;
        hr = lib->GetFolderType(&folder_type);
        if (FAILED(hr)) {
            rv = VPL_ERR_FAIL;
            goto out;
        }
        if (folder_type == FOLDERTYPEID_Documents) {
            libinfo->folder_type = "Documents";
        } else if (folder_type == FOLDERTYPEID_Music) {
            libinfo->folder_type = "Music";
        } else if (folder_type == FOLDERTYPEID_Pictures) {
            libinfo->folder_type = "Photo";
        } else if (folder_type == FOLDERTYPEID_Videos) {
            libinfo->folder_type = "Video";
        } else {
            libinfo->folder_type = "Generic";
        }
    }
    rv = _Win_GetKnownFolderWPath(FOLDERID_System, &wsystem_folder);
    if (rc != VPL_OK) {
        VPL_REPORT_WARN("Unable to get system folder, %d", rv);
    } else {
        for (wchar_t *p = wsystem_folder; *p != L'\0'; p++) {
            if (*p == L'/') {
                *p = L'\\';
            }
        }
    }
    rv = _Win_GetKnownFolderWPath(FOLDERID_Profile, &wuserpf_folder);
    if (rc != VPL_OK) {
        VPL_REPORT_WARN("Unable to get user profile folder = %d", rv);
    } else {
        for (wchar_t *p = wuserpf_folder; *p != L'\0'; p++) {
            if (*p == L'/') {
                *p = L'\\';
            }
        }
    }
    hr = folders->GetCount(&nfolders);
    if (FAILED(hr)) {
        rv = VPL_ERR_FAIL;
        goto out;
    }
    for (DWORD i = 0; i < nfolders; i++) {
        IShellItem *folder = NULL;
        wchar_t *wfolderpath = NULL;
        wchar_t *wfolderpath_modified = NULL; // used when the folder is under systemprofile dir
        wchar_t *wfoldername = NULL;
        char *ufolderpath = NULL;
        char *ufoldername = NULL;
        std::string ufolder_non_localized;
        _VPLFS__LibFolderInfo libfolder;
        DWORD attr;

        hr = folders->GetItemAt(i, &folder);
        if (FAILED(hr)) goto next;
        // filesystem path
        hr = folder->GetDisplayName(SIGDN_FILESYSPATH, &wfolderpath);
        if (FAILED(hr)) goto next;

        // Make sure the folder is accessible
        // When user create a folder under the C:\Users\xxx\foldername
        // and add the folder into the Library, it will return path like
        // C:/Windows/System32/config/systemprofile/foldername
        // However, it's not accessible by the VPLFS API and it's not a directory
        // (not even a shortcut of soft link)
        // NOTE: this only happens when the CCD is launched by CCDMonitorService

        // stat the filepath to make sure it's a valid directory
        attr = GetFileAttributes(wfolderpath);
        if (attr == INVALID_FILE_ATTRIBUTES) {
            rc = VPLError_XlatWinErrno(GetLastError());
        } else {
            if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                rc = VPL_ERR_NOTDIR;
            } else {
                rc = VPL_OK;
            }
        }
        // only perform the modification when
        // 1) we are not able to access the folder
        // 2) the file is not a folder
        if (rc != VPL_OK) {
            const static wchar_t *wsystem_profile = L"\\config\\systemprofile\\";
            size_t len_sys_folder, len_sys_profile, len_folder, len_modified;

            if (rc != VPL_ERR_NOENT && rc != VPL_ERR_NOTDIR) {
                 // unhandled error. don't expose to user
                int tmp = _VPL__wstring_to_utf8_alloc(wfolderpath, &ufolderpath);
                if (tmp != VPL_OK || ufolderpath == NULL) {
                    VPL_REPORT_WARN("unable to access folderpath (failed to convert"
                                    " to utf8 err=%d), unknown error = %d", tmp, rc);
                } else {
                    VPL_REPORT_WARN("unable to access folderpath = %s, unknown error = %d",
                                    ufolderpath, rc);
                }
                goto next;
            }
            // compare string w/ "%windir%\system32\config\systemprofile\"
            if (wsystem_folder == NULL || wuserpf_folder == NULL) {
                rc = _VPL__wstring_to_utf8_alloc(wfolderpath, &ufolderpath);
                if (rc != VPL_OK || ufolderpath == NULL) {
                    VPL_REPORT_WARN("unable to access folderpath (failed to convert to utf8"
                                    " err=%d), unable to get system/user profile path.", rc);
                } else {
                    VPL_REPORT_WARN("unable to access folderpath = %s, unable to get"
                                    " system/user profile path.",
                                    ufolderpath);
                }
                goto next;
            }

            len_sys_folder = wcslen(wsystem_folder);
            len_sys_profile = wcslen(wsystem_profile);
            len_folder = wcslen(wfolderpath);

            if ((len_folder <= len_sys_folder+len_sys_profile) ||
                _wcsnicmp(wsystem_folder, wfolderpath, len_sys_folder) != 0 ||
                _wcsnicmp(wsystem_profile, wfolderpath+len_sys_folder, len_sys_profile) != 0) {
                rc = _VPL__wstring_to_utf8_alloc(wfolderpath, &ufolderpath);
                if (rc != VPL_OK || ufolderpath == NULL) {
                    VPL_REPORT_WARN("unable to access folderpath (failed to convert to utf8 err=%d)"
                                    ", prefix doesn't match system profile folder", rc);
                } else {
                    VPL_REPORT_WARN("unable to access folderpath = %s, prefix doesn't match"
                                    " system profile folder", ufolderpath);
                }
                goto next;
            }

            len_modified = len_sys_folder + len_sys_profile;
            len_modified = wcslen(wuserpf_folder) + (len_folder - len_modified) + 2;
            wfolderpath_modified = (wchar_t*)malloc(len_modified * sizeof(wchar_t));
            if (wfolderpath_modified == NULL) {
                rc = _VPL__wstring_to_utf8_alloc(wfolderpath, &ufolderpath);
                if (rc != VPL_OK || ufolderpath == NULL) {
                    VPL_REPORT_WARN("unable to access folderpath (failed to convert to utf8 err=%d)"
                                    ", out of memory while allocating buffer for modified path", rc);
                } else {
                    VPL_REPORT_WARN("unable to access folderpath = %s, out of memory while"
                                    " allocating buffer for modified path",
                                    ufolderpath);
                }
                goto next;
            }

            wcscpy(wfolderpath_modified, wuserpf_folder);
            wcscat(wfolderpath_modified, wfolderpath+len_sys_folder+len_sys_profile-1);
            wfolderpath_modified[len_modified-1] = L'\0';

            // stat the filepath to make sure it's a valid directory
            attr = GetFileAttributes(wfolderpath_modified);
            if (attr == INVALID_FILE_ATTRIBUTES) {
                rc = VPLError_XlatWinErrno(GetLastError());
                int tmp = _VPL__wstring_to_utf8_alloc(wfolderpath_modified, &ufolderpath);
                if (tmp != VPL_OK || ufolderpath == NULL) {
                    VPL_REPORT_WARN("unable to access folderpath (failed to convert to utf8"
                                    " err=%d), error = %d", tmp, rc);
                } else {
                    VPL_REPORT_WARN("unable to access folderpath = %s, error = %d",
                                    ufolderpath, rc);
                }
                goto next;
            }
            // make sure it's a directory
            if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                rc = _VPL__wstring_to_utf8_alloc(wfolderpath_modified, &ufolderpath);
                if (rc != VPL_OK || ufolderpath == NULL) {
                    VPL_REPORT_WARN("modified folderpath (failed to convert to utf8 err=%d) "
                                    "is not a directory. skip", rc);
                } else {
                    VPL_REPORT_WARN("modified folderpath = %s is not a directory. skip", ufolderpath);
                }
                goto next;
            }
        }

        // used modified path if there's one
        if (wfolderpath_modified != NULL) {
            rc = _VPL__wstring_to_utf8_alloc(wfolderpath_modified, &ufolderpath);
        } else {
            rc = _VPL__wstring_to_utf8_alloc(wfolderpath, &ufolderpath);
        }

        if (rc != VPL_OK) goto next;
        // replace back slashes as slashes
        for (char *p = ufolderpath; *p != '\0'; p++) {
            if (*p == '\\')
                *p = '/';
        }
        // localized name
        hr = folder->GetDisplayName(SIGDN_NORMALDISPLAY, &wfoldername);
        if (FAILED(hr)) goto next;
        rc = _VPL__wstring_to_utf8_alloc(wfoldername, &ufoldername);
        if (rc != VPL_OK) goto next;

        libfolder.l_name = ufoldername;
        ufolder_non_localized = ufolderpath;
        // escape "/"
        std::replace(ufolder_non_localized.begin(), ufolder_non_localized.end(), '/', ':');
        libfolder.n_name = ufolder_non_localized;
        libfolder.path = ufolderpath;
        // use the localized one to distinguish the folders... otherwise, it will be the same
        // ex: My Music -> Music, Public Music -> Music
        libinfo->m[ufolder_non_localized] = libfolder;
next:
        if (folder != NULL) {
            folder->Release();
            folder = NULL;
        }
        if (wfolderpath != NULL) {
            CoTaskMemFree(wfolderpath);
            wfolderpath = NULL;
        }
        if (wfolderpath_modified != NULL) {
            free(wfolderpath_modified);
            wfolderpath_modified = NULL;
        }
        if (wfoldername != NULL) {
            CoTaskMemFree(wfoldername);
            wfoldername = NULL;
        }
        if (ufolderpath != NULL) {
            free(ufolderpath);
            ufolderpath = NULL;
        }
        if (ufoldername != NULL) {
            free(ufoldername);
            ufoldername = NULL;
        }
    }

 out:
    if (wpath != NULL) {
        free(wpath);
        wpath = NULL;
    }
    if (libfile != NULL) {
        libfile->Release();
        libfile = NULL;
    }
    if (wlibname != NULL) {
        CoTaskMemFree(wlibname);
        wlibname = NULL;
    }
    if (ulibname != NULL) {
        free(ulibname);
        ulibname = NULL;
    }
    if (lib != NULL) {
        lib->Release();
        lib = NULL;
    }
    if (folders != NULL) {
        folders->Release();
        folders = NULL;
    }
    if (wuserpf_folder != NULL) {
        free(wuserpf_folder);
        wuserpf_folder = NULL;
    }
    if (wsystem_folder != NULL) {
        free(wsystem_folder);
        wsystem_folder = NULL;
    }

    return rv;
}

#define BUFSIZE 512
int _VPLFS__GetComputerDrives(std::map<std::string, _VPLFS__DriveType> &driveMap)
{
    int rv = VPL_OK;
    BOOL bFound = FALSE;
    UINT uDriveRet;
    WCHAR szTemp[BUFSIZE];
    szTemp[0] = '\0';
    WCHAR szDrive[3] = L" :";
    WCHAR *p = szTemp;
    std::wstring wstrDrive;

    driveMap.clear();
    if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) {
        do {
            // Copy the drive letter to the template string
            // Both pointers point to the same data, *p will
            // be used to skip the NULL
            *szDrive = *p;
            wstrDrive = szDrive;
            std::string strDrive(wstrDrive.begin(), wstrDrive.end());
            uDriveRet = GetDriveType(szDrive);
            driveMap.insert(std::make_pair(strDrive, static_cast<_VPLFS__DriveType>(uDriveRet)));
            while (*p++);// skip the next NULL character, starts reading new drive string
        } while (!bFound && *p); // end of string
    }
    else {
        rv = VPLError_XlatWinErrno(GetLastError());
    }

    return rv;
}

int _VPLFS__InsertTrustees(SID_AND_ATTRIBUTES &ace)
{
    int rv = VPL_OK;
    MutexAutoLock autolock(::VPLLazyInitMutex_GetMutex(&trusteesMutex));
    
    _trustees.push_back(ace);

    return rv;
}

void _VPLFS__ClearTrustees()
{
    MutexAutoLock autolock(::VPLLazyInitMutex_GetMutex(&trusteesMutex));
    
    if (!_trustees.empty()) {
        // clean up _trustees is not empty
        std::list<SID_AND_ATTRIBUTES>::iterator it;
        for (it=_trustees.begin(); it != _trustees.end(); it++) {
            LocalFree(it->Sid);
        }
        _trustees.clear();
    }
}

static int _CheckAccessRight(const wchar_t *wpath, ACCESS_MASK access_mask)
{
    int rv = VPL_ERR_ACCESS;
    MutexAutoLock autolock(::VPLLazyInitMutex_GetMutex(&trusteesMutex));
    ACCESS_MASK totalMask = 0; // record all the granted access rights
    PACL pacl = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = NULL;

    if (_trustees.size() == 0) {
        rv = VPL_OK;
    }
    else if (_trustees.size() > 0) {
        // check access right of the path
        DWORD dwRt = GetNamedSecurityInfo(wpath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pacl, NULL, &pSecDesc);
        if (dwRt != ERROR_SUCCESS) {
            rv = VPLError_XlatWinErrno(dwRt);
        }
        else {
            if (pacl == NULL) {
                rv = VPL_OK;
                goto out;
            }
            for (USHORT i=0; i < pacl->AceCount; i++) {
                LPVOID pAce = NULL;
                if (GetAce(pacl, i, &pAce)) {
#if defined(_DEBUG)
                    VPL_REPORT_WARN("[%d] ACE type = %d", i, ((ACE_HEADER*)pAce)->AceType);
#endif
                    if (((ACE_HEADER*)pAce)->AceType == ACCESS_DENIED_ACE_TYPE) {
                        // 1. An access-denied ACE explicitly denies 
                        //    any of the requested access rights to one of the trustees.
                        ACCESS_DENIED_ACE *pDenied = (ACCESS_DENIED_ACE *)pAce;
                        ACCESS_MASK permission = pDenied->Mask;
                        PSID pAceSid = (PSID)(&(pDenied->SidStart));
#if defined(_DEBUG)
                        {
                            LPSTR strSid;
                            ConvertSidToStringSidA(pAceSid, &strSid);
                            VPL_REPORT_WARN("    Sid = %s", strSid);
                            VPL_REPORT_WARN("    Access Mask = %lu", permission);
                            LocalFree(strSid);
                        }
#endif
                        std::list<SID_AND_ATTRIBUTES>::iterator it;
                        for (it=_trustees.begin(); it != _trustees.end(); it++) {
                            if (it->Attributes == 0 
                                || (it->Attributes & SE_GROUP_ENABLED)
                                || (it->Attributes & SE_GROUP_USE_FOR_DENY_ONLY)) {
                                if (EqualSid(pAceSid, it->Sid) && (permission & access_mask)) {
                                    rv = VPL_ERR_ACCESS;
                                    goto out;
                                }
                            }
                        }
                    }
                    else if (((ACE_HEADER*)pAce)->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                        ACCESS_ALLOWED_ACE *pAllowed = (ACCESS_ALLOWED_ACE *)pAce;
                        ACCESS_MASK permission = pAllowed->Mask;
                        PSID pAceSid = (PSID)(&(pAllowed->SidStart));
#if defined(_DEBUG)
                        {
                            LPSTR strSid;
                            ConvertSidToStringSidA(pAceSid, &strSid);
                            VPL_REPORT_WARN("    Sid = %s", strSid);
                            VPL_REPORT_WARN("    Access Mask = %lu", permission);
                            LocalFree(strSid);
                        }
#endif
                        std::list<SID_AND_ATTRIBUTES>::iterator it;
                        for (it=_trustees.begin(); it != _trustees.end(); it++) {
                            if (it->Attributes == 0 || (it->Attributes & SE_GROUP_ENABLED)) {
                                if (EqualSid(pAceSid, it->Sid) && (permission & access_mask)) {
                                    totalMask |= (permission & access_mask);
                                }
                            }
                        }
                    }
                }
                else {
                    // return failed if any GetAce() failed
                    VPL_REPORT_INFO("GetAce() failed: %u", GetLastError());
                    rv = VPL_ERR_ACCESS;
                    goto out;
                }
            }
            if ((totalMask & access_mask) == access_mask) {
                // 2. One or more access-allowed ACEs for trustees listed
                //    in the thread's access token explicitly grant all the requested access rights.
                rv = VPL_OK;
                goto out;
            }
            else {
                // 3. All ACEs have been checked and there is still at least one requested access right
                // that has not been explicitly allowed, in which case, access is implicitly denied.
                rv = VPL_ERR_ACCESS;
            }
        }
    }

out:
    if (pSecDesc != NULL)
        LocalFree((HLOCAL)pSecDesc);
    return rv;
}

static const wchar_t extended_drive_prefix[] = L"\\\\?\\x:\\";

int _VPLFS__CheckAccessRight(const wchar_t *wpath, ACCESS_MASK access_mask, bool is_checkancestor)
{
    int rv;

    if (!is_checkancestor) {
        // check if path has specific access right
        rv = _CheckAccessRight(wpath, access_mask);
    }
    else {
        // recursively check path and its ancestor folder have specific access right
        wchar_t *tmp_path = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(wpath)+1));
        if(tmp_path == NULL) {
            rv = VPL_ERR_NOMEM;
            goto end;
        }
        wcsncpy(tmp_path, wpath, (wcslen(wpath)+1));
        if (wcslen(tmp_path) < wcslen(extended_drive_prefix)) {
            // wpath is shorter than extended_drive_prefix, error!!
            rv = VPL_ERR_NOMEM;
        }
        else if (wcslen(tmp_path) == wcslen(extended_drive_prefix)) {
            // wpath is extended_drive_prefix
            rv = _CheckAccessRight(tmp_path, access_mask);
        }
        else {
            // wpath is longer than extended_drive_prefix
            for (size_t i=wcslen(extended_drive_prefix); i < wcslen(tmp_path); i++) {
                if (i == wcslen(tmp_path)-1) {
                    rv = _CheckAccessRight(tmp_path, access_mask);
                }
                else if (tmp_path[i] == L'\\') {
                    tmp_path[i] = L'\0';
                    rv = _CheckAccessRight(tmp_path, FILE_GENERIC_READ);
                    if (rv < 0) {
                        char *utmp_path = NULL;
                        int rc = _VPL__wstring_to_utf8_alloc(tmp_path, &utmp_path);
                        if (rc != VPL_OK || utmp_path == NULL) {
                            VPL_REPORT_INFO("_CheckAccessRight failed(failed to convert to utf8 err=%d): %u",
                                            rc, GetLastError());
                        } else {
                            VPL_REPORT_INFO("_CheckAccessRight %s failed: %u", utmp_path, GetLastError());
                            free(utmp_path);
                        }
                        tmp_path[i] = L'\\';
                        break;
                    }
                    tmp_path[i] = L'\\';
                }
            }
        }
        if (tmp_path != NULL) {
            free(tmp_path);
            tmp_path = NULL;
        }
    }

end:
    return rv;
}

static BOOL _CheckCertificateInfo(PCCERT_CONTEXT pCertContext)
{
    BOOL fReturn = FALSE;
    LPWSTR szName = NULL;
    DWORD dwData;

    __try {
        // TODO: check the serial number and the issuer info as well
#if 0
        // Print Serial Number.
        VPL_REPORT_WARN("Serial Number: ");
        dwData = pCertContext->pCertInfo->SerialNumber.cbData;
        for (DWORD n = 0; n < dwData; n++) {
            VPL_REPORT_WARN("%02x ", pCertContext->pCertInfo->SerialNumber.pbData[dwData - (n + 1)]);
        }

        // Get Issuer name size.
        if (!(dwData = CertGetNameString(pCertContext,
                                         CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                         CERT_NAME_ISSUER_FLAG,
                                         NULL,
                                         NULL,
                                         0))) {
            VPL_REPORT_WARN("CertGetNameString failed.");
            __leave;
        }

        // Allocate memory for Issuer name.
        szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
        if (!szName) {
            VPL_REPORT_WARN("Unable to allocate memory for issuer name.");
            __leave;
        }

        // Get Issuer name.
        if (!(CertGetNameString(pCertContext,
                                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                CERT_NAME_ISSUER_FLAG,
                                NULL,
                                szName,
                                dwData))) {
            VPL_REPORT_WARN("CertGetNameString failed.");
            __leave;
        }

        // print Issuer name.
        LocalFree(szName);
        szName = NULL;
#endif
        // Get Subject name size.
        if (!(dwData = CertGetNameStringW(pCertContext,
                                          CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                          0,
                                          NULL,
                                          NULL,
                                          0))) {
            VPL_REPORT_WARN("CertGetNameString failed.");
            __leave;
        }

        // Allocate memory for subject name.
        szName = (LPWSTR)LocalAlloc(LPTR, dwData * sizeof(wchar_t));
        if (!szName) {
            VPL_REPORT_WARN("Unable to allocate memory for subject name.");
            __leave;
        }

        // Get subject name.
        if (!(CertGetNameStringW(pCertContext,
                                 CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                 0,
                                 NULL,
                                 szName,
                                 dwData))) {
            VPL_REPORT_WARN("CertGetNameString failed.");
            __leave;
        }

        if (wcscmp(szName, L"Acer Incorporated") == 0) {
            fReturn = TRUE;
        } else {
            VPL_REPORT_WARN("Subject name is different from the expected");
            fReturn = FALSE;
        }
    } __finally {
        if (szName != NULL) {
            LocalFree(szName);
        }
    }

    return fReturn;
}

static BOOL _CheckIfHasAcerSign(const wchar_t *wfilepath)
{
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fResult;
    DWORD dwEncoding, dwContentType, dwFormatType;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    PCMSG_SIGNER_INFO pCounterSignerInfo = NULL;
    DWORD dwSignerInfo;
    CERT_INFO CertInfo;

    __try {
        // Get message handle and store handle from the signed file.
        fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                                   wfilepath,
                                   CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                                   CERT_QUERY_FORMAT_FLAG_BINARY,
                                   0,
                                   &dwEncoding,
                                   &dwContentType,
                                   &dwFormatType,
                                   &hStore,
                                   &hMsg,
                                   NULL);
        if (!fResult) {
            VPL_REPORT_WARN("CryptQueryObject failed with %x", GetLastError());
            __leave;
        }

        // Get signer information size.
        fResult = CryptMsgGetParam(hMsg,
                                   CMSG_SIGNER_INFO_PARAM,
                                   0,
                                   NULL,
                                   &dwSignerInfo);
        if (!fResult) {
            VPL_REPORT_WARN("CryptMsgGetParam failed with %x", GetLastError());
            __leave;
        }

        // Allocate memory for signer information.
        pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
        if (!pSignerInfo) {
            VPL_REPORT_WARN("Unable to allocate memory for Signer Info.");
            __leave;
        }

        // Get Signer Information.
        fResult = CryptMsgGetParam(hMsg,
                                   CMSG_SIGNER_INFO_PARAM,
                                   0,
                                   (PVOID)pSignerInfo,
                                   &dwSignerInfo);
        if (!fResult) {
            VPL_REPORT_WARN("CryptMsgGetParam failed with %x", GetLastError());
            __leave;
        }

        // Search for the signer certificate in the temporary
        // certificate store.
        CertInfo.Issuer = pSignerInfo->Issuer;
        CertInfo.SerialNumber = pSignerInfo->SerialNumber;

        pCertContext = CertFindCertificateInStore(hStore,
                                                  (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING),
                                                  0,
                                                  CERT_FIND_SUBJECT_CERT,
                                                  (PVOID)&CertInfo,
                                                  NULL);
        if (!pCertContext) {
            VPL_REPORT_WARN("CertFindCertificateInStore failed with %x", GetLastError());
            __leave;
        }

        // Check Signer certificate information.
        fResult = _CheckCertificateInfo(pCertContext);

    } __finally {
        // Clean up.
        if (pSignerInfo != NULL) {
            LocalFree(pSignerInfo);
        }
        if (pCounterSignerInfo != NULL) {
            LocalFree(pCounterSignerInfo);
        }
        if (pCertContext != NULL) {
            CertFreeCertificateContext(pCertContext);
        }
        if (hStore != NULL) {
            CertCloseStore(hStore, 0);
        }
        if (hMsg != NULL) {
            CryptMsgClose(hMsg);
        }
    }
    return fResult;
}

// Reference: http://msdn.microsoft.com/en-us/library/aa382384.aspx
int _VPLFS_CheckTrustedExecutable(const std::string &upathname)
{

    int rv = VPL_OK;
    wchar_t *wpathname = NULL;

    // convert utf-8 pathname to extended pathname
    if (upathname.empty()) {
        return VPL_ERR_NOENT;
    }

    if (upathname.length() == 1 && (upathname[0] == L'/' || upathname[0] == L'\\')) {
        return VPL_ERR_NOENT;
    }

    rv = _VPL__utf8_to_wstring(upathname.c_str(), &wpathname);
    if (rv != VPL_OK) {
        VPL_REPORT_WARN("failed to convert to wstring: %s, %d", upathname.c_str(), rv);
        return VPL_ERR_FAIL;
    }
    ON_BLOCK_EXIT(free, wpathname);

    /* Note:
     * _stat() cannot handle trailing slashes, so we need to remove them
     * cf. http://msdn.microsoft.com/en-us/library/14h5k7ff%28v=VS.100%29.aspx
     * unless the path is to the root directory (e.g., C:/)
     */
    if (wcslen(wpathname) > 3) {
        int i = (int)(wcslen(wpathname) - 1);
        while ((i >= 0) && (wpathname[i] == L'/' || wpathname[i] == L'\\')) {
            wpathname[i] = L'\0';
            i--;
        }
    }

    LONG lStatus;
    DWORD dwLastError;

    WINTRUST_FILE_INFO FileData;

    // Initialize the WINTRUST_FILE_INFO structure.
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath = wpathname;
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    /*
       WVTPolicyGUID specifies the policy to apply on the file
       WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:

       1) The certificate used to sign the file chains up to a root
       certificate located in the trusted root certificate store. This
       implies that the identity of the publisher has been verified by
       a certification authority.

       2) In cases where user interface is displayed (which this example
       does not do), WinVerifyTrust will check for whether the
       end entity certificate is stored in the trusted publisher store,
       implying that the user trusts content from this publisher.

       3) The end entity certificate has sufficient permission to sign
       code, as indicated by the presence of a code signing EKU or no
       EKU.
     */

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;

    // Initialize the WinVerifyTrust input data structure.

    // Default all fields to 0.
    memset(&WinTrustData, 0, sizeof(WinTrustData));

    WinTrustData.cbStruct = sizeof(WinTrustData);

    // Use default code signing EKU.
    WinTrustData.pPolicyCallbackData = NULL;

    // No data to pass to SIP.
    WinTrustData.pSIPClientData = NULL;

    // Disable WVT UI.
    WinTrustData.dwUIChoice = WTD_UI_NONE;

    // No revocation checking.
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;

    // Verify an embedded signature on a file.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

    // Default verification.
    WinTrustData.dwStateAction = 0;

    // Not applicable for default verification of embedded signature.
    WinTrustData.hWVTStateData = NULL;

    // Not used.
    WinTrustData.pwszURLReference = NULL;

    // This is not applicable if there is no UI because it changes
    // the UI to accommodate running applications instead of
    // installing applications.
    WinTrustData.dwUIContext = 0;

    // Set pFile.
    WinTrustData.pFile = &FileData;

    // WinVerifyTrust verifies signatures as specified by the GUID
    // and Wintrust_Data.
    lStatus = WinVerifyTrust(NULL, &WVTPolicyGUID, &WinTrustData);

    switch (lStatus) {
    case ERROR_SUCCESS:
        /*
           Signed file:
           - Hash that represents the subject is trusted.
           - Trusted publisher without any verification errors.
           - UI was disabled in dwUIChoice. No publisher or
             time stamp chain errors.
           - UI was enabled in dwUIChoice and the user clicked
             "Yes" when asked to install and run the signed
           subject.
         */
        //VPL_REPORT_INFO("The file \"%s\" is signed and the signature was verified.",
        //                upathname.c_str());
        if (_CheckIfHasAcerSign(wpathname)) {
            rv = VPL_OK;
        } else {
            VPL_REPORT_WARN("The file \"%s\" is not signed by Acer", upathname.c_str());
            rv = VPL_ERR_FAIL;
        }
        break;

    case TRUST_E_NOSIGNATURE:
        // The file was not signed or had a signature
        // that was not valid.

        // Get the reason for no signature.
        dwLastError = GetLastError();
        if (TRUST_E_NOSIGNATURE == dwLastError ||
            TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
            TRUST_E_PROVIDER_UNKNOWN == dwLastError) {
            // The file was not signed.
            VPL_REPORT_WARN("The file \"%s\" is not signed.", upathname.c_str());
            rv = VPL_ERR_FAIL;
        } else {
            // The signature was not valid or there was an error
            // opening the file.
            VPL_REPORT_WARN("An unknown error occurred trying to verify"
                            " the signature of the \"%s\" file.",
                            upathname.c_str());
            rv = VPL_ERR_NOENT;
        }
        break;

    case TRUST_E_EXPLICIT_DISTRUST:
        // The hash that represents the subject or the publisher
        // is not allowed by the admin or user.
        VPL_REPORT_WARN("The signature is present, but specifically disallowed.");
        rv = VPL_ERR_FAIL;
        break;

    case TRUST_E_SUBJECT_NOT_TRUSTED:
        // The user clicked "No" when asked to install and run.
        VPL_REPORT_WARN("The signature is present, but not trusted.");
        rv = VPL_ERR_FAIL;
        break;

    case CRYPT_E_SECURITY_SETTINGS:
        /*
           The hash that represents the subject or the publisher
           was not explicitly trusted by the admin and the
           admin policy has disabled user trust. No signature,
           publisher or time stamp errors.
         */
        VPL_REPORT_WARN("CRYPT_E_SECURITY_SETTINGS - The hash "
                        "representing the subject or the publisher wasn't "
                        "explicitly trusted by the admin and admin policy "
                        "has disabled user trust. No signature, publisher "
                        "or timestamp errors.");
        rv = VPL_ERR_FAIL;
        break;

    default:
        // The UI was disabled in dwUIChoice or the admin policy
        // has disabled user trust. lStatus contains the
        // publisher or time stamp chain error.
        VPL_REPORT_WARN("Unknown error: 0x%x", lStatus);
        rv = VPL_ERR_FAIL;
        break;
    }

    return rv;
}

#endif // not VPL_PLAT_IS_WINRT
