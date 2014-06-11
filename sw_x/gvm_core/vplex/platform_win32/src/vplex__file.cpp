// defines and includes for generic function names to map to unicode versions
// http://msdn.microsoft.com/en-us/library/dd374107%28v=VS.85%29.aspx
// http://msdn.microsoft.com/en-us/library/dd374061%28v=VS.85%29.aspx
#ifndef UNICODE
# define UNICODE
#endif
#ifndef _UNICODE
# define _UNICODE
#endif

#include <wchar.h>

#include "vplex_file.h"
#include "vpl_fs.h"
#include "vpl_time.h"
#include "vplex_private.h"

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef VPL_PLAT_IS_WINRT
#include "vplex__file_priv.h"
#include "vpl__fs_priv.h"
#endif

#define MAX_BUFFER_SIZE (5 * 1024)

int 
VPLFile_CheckAccess(const char *pathname, int mode)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;
    DWORD attr;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_CheckAccess(pathname, mode);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

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

    {
#ifdef VPL_PLAT_IS_WINRT
        WIN32_FILE_ATTRIBUTE_DATA fileInfo = {0};
        BOOL bRt = GetFileAttributesEx(epathname, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileInfo);
        if (bRt == TRUE) {
            attr = fileInfo.dwFileAttributes;
        }
        else {
#else
        attr = GetFileAttributes(epathname);
        if (attr == INVALID_FILE_ATTRIBUTES) {
#endif
            rv = VPLError_XlatWinErrno(GetLastError());
            goto end;
        }
    }

    rv = VPL_OK;  // assume OK and try to prove otherwise
    if ((mode & VPLFILE__CHECKACCESS_WRITE) && (attr & FILE_ATTRIBUTE_READONLY)) {
      rv = -1;
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

VPLFile_handle_t 
VPLFile_Open(const char *pathname, int flags, int mode)
{
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;
    VPLFile_handle_t h = VPLFILE_INVALID_HANDLE;
    DWORD desiredAccess;
    DWORD shareMode;
    DWORD createDisposition;
    HANDLE h32;
    DWORD winAttrs;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_Open(pathname, flags, mode);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    rc = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rc != VPL_OK) {
        h = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
        h = rc;
        goto end;
    }

    desiredAccess = GENERIC_READ;  // always grant read access
    if (flags & (VPLFILE__OPENFLAG_WRITEONLY | VPLFILE__OPENFLAG_READWRITE)) {
        if (VPLFile_CheckAccess(pathname, VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
    #ifdef VPL_PLAT_IS_WINRT
            WIN32_FILE_ATTRIBUTE_DATA fileInfo = {0};
            BOOL bRt = GetFileAttributesEx(epathname, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileInfo);
            if (bRt == TRUE) {
                winAttrs = fileInfo.dwFileAttributes;
            }
            else {
    #else
            winAttrs = GetFileAttributes(epathname);
            if (winAttrs == INVALID_FILE_ATTRIBUTES) {
    #endif
                rc = VPLError_XlatWinErrno(GetLastError());
                goto end;
            }
            if (winAttrs & FILE_ATTRIBUTE_READONLY) {
                rc = VPL_ERR_ACCESS;
                goto end;
            }
        }
        desiredAccess |= GENERIC_WRITE;
    }
    shareMode = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;
    createDisposition = 0;
    switch (flags & (VPLFILE__OPENFLAG_CREATE | VPLFILE__OPENFLAG_TRUNCATE)) {
    case 0:
      createDisposition = OPEN_EXISTING;
      break;
    case VPLFILE__OPENFLAG_CREATE:
      createDisposition = OPEN_ALWAYS;
      break;
    case VPLFILE__OPENFLAG_TRUNCATE:
      createDisposition = TRUNCATE_EXISTING;
      break;
    case VPLFILE__OPENFLAG_CREATE | VPLFILE__OPENFLAG_TRUNCATE:
      createDisposition = CREATE_ALWAYS;
      break;
    }

    {
#ifdef VPL_PLAT_IS_WINRT
        CREATEFILE2_EXTENDED_PARAMETERS cf2ex = {0};
        cf2ex.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
        cf2ex.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        cf2ex.hTemplateFile = NULL;
        h32 = CreateFile2(epathname, desiredAccess, shareMode, createDisposition, &cf2ex);
#else
        h32 = CreateFile(epathname, desiredAccess, shareMode, NULL, createDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
#endif
        if (h32 == INVALID_HANDLE_VALUE) {
            h = VPLError_XlatWinErrno(GetLastError());
            // check if the error is due to a bad path; if so, override the error code
            rc = _VPLFS__CheckValidWpath(wpathname);
            if (rc != VPL_OK) {
                h = rc;
            }
            goto end;
        }
    }

    flags &= _O_APPEND | _O_RDONLY | _O_TEXT;  // only attributes described in http://msdn.microsoft.com/en-us/library/bdts1c9x.aspx
    h = _open_osfhandle((intptr_t)h32, flags);

 end:
    if (wpathname != NULL) {
        free(wpathname);
    }
    if (epathname != NULL) {
        free(epathname);
    }
    return h;
}

int 
VPLFile_IsValidHandle(VPLFile_handle_t h)
{
    return h >= 0;
}

ssize_t 
VPLFile_Write(VPLFile_handle_t h, const void *buffer, size_t bufsize)
{
    ssize_t nbytes = 0;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_Write(h, buffer, bufsize);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    nbytes = _write(h, buffer, static_cast<unsigned int>(bufsize));
    if (nbytes == -1) {
        nbytes = VPLError_XlatErrno(errno);
        goto end;
    }

 end:
    return nbytes;
}

ssize_t 
VPLFile_Read(VPLFile_handle_t h, void *buffer, size_t bufsize)
{
    ssize_t nbytes = 0;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_Read(h, buffer, bufsize);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    nbytes = _read(h, buffer, static_cast<unsigned int>(bufsize));
    if (nbytes == -1) {
        nbytes = VPLError_XlatErrno(errno);
        goto end;
    }

 end:
    return nbytes;
}

ssize_t 
VPLFile_WriteAt(VPLFile_handle_t h, const void *buffer, size_t bufsize, VPLFile_offset_t offset)
{
    // TODO need to guarantee atomicity
    __int64 curpos;
    int bytes = -1;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_WriteAt(h, buffer, bufsize, offset);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    curpos = _telli64(h);
    if (curpos < 0) {
        bytes = VPLError_XlatErrno(errno);
        goto end;
    }

    if (_lseeki64(h, offset, SEEK_SET) < 0) {
        bytes = VPLError_XlatErrno(errno);
        goto end;
    }

    bytes = _write(h, buffer, static_cast<unsigned int>(bufsize));
    if (bytes < 0) {
        bytes = VPLError_XlatErrno(errno);
        goto end;
    }

 end:
    if (curpos >= 0) {
        _lseeki64(h, curpos, SEEK_SET);
    }
    return bytes;
}

ssize_t 
VPLFile_ReadAt(VPLFile_handle_t h, void *buffer, size_t bufsize, VPLFile_offset_t offset)
{
    // TODO need to guarantee atomicity
    __int64 curpos;
    int bytes = -1;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_ReadAt(h, buffer, bufsize, offset);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    curpos = _telli64(h);
    if (curpos < 0) {
        bytes = VPLError_XlatErrno(errno);
        goto end;
    }

    if (_lseeki64(h, offset, SEEK_SET) < 0) {
        bytes = VPLError_XlatErrno(errno);
        goto end;
    }

    bytes = _read(h, buffer, static_cast<unsigned int>(bufsize));
    if (bytes < 0) {
        bytes = VPLError_XlatErrno(errno);
        goto end;
    }

 end:
    if (curpos >= 0) {
        _lseeki64(h, curpos, SEEK_SET);
    }
    return bytes;
}

int 
VPLFile_CreateTemp(char* filename_in_out, size_t bufSize)
{
    size_t actualLen = 0;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(filename_in_out);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_CreateTemp(filename_in_out, bufSize);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    actualLen = strnlen(filename_in_out, bufSize);
    if (actualLen == bufSize) {
        VPL_LIB_LOG_WARN(VPL_SG_FS, "No null-terminator found");
        return VPL_ERR_INVALID;
    }
    // mktemp on Windows apparently uses the thread id in the temp filename,
    // so this should be safe enough.
    if (mktemp(filename_in_out) == NULL) {
        VPL_LIB_LOG_WARN(VPL_SG_FS, "mktemp returned NULL");
        return VPL_ERR_INVALID;
    }
    return open(filename_in_out, O_CREAT | O_EXCL, S_IREAD | S_IWRITE);
}

int 
VPLFile_TruncateAt(VPLFile_handle_t h, VPLFile_offset_t length)
{
    /* MSDN mentions _chsize_t as a way to set file size using 64-bit offset.
     * http://msdn.microsoft.com/en-us/library/whx354w1%28v=VS.100%29.aspx
     * Unfortunately, our mingw doesn't know about it, so we'll do something else.
     */

    int rv = VPL_OK;
    HANDLE h2 = INVALID_HANDLE_VALUE;
    __int64 curpos = 0;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_TruncateAt(h, length);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    curpos = _telli64(h);
    if (curpos < 0) {
        rv = VPLError_XlatErrno(errno);
        goto end;
    }

    if (_lseeki64(h, length, SEEK_SET) < 0) {
        rv = VPLError_XlatErrno(errno);
        goto end;
    }

    // Bug 541: Do we need to call _close?
    h2 = (HANDLE)_get_osfhandle(h);
    if (h2 == INVALID_HANDLE_VALUE) {
        rv = VPL_ERR_BADF;
        goto end;
    }

    if (SetEndOfFile(h2) == 0) {
        rv = VPLError_XlatErrno(GetLastError());
        goto end;
    }

 end:
    if (curpos >= 0) {
        _lseeki64(h, curpos, SEEK_SET);
    }

    return rv;
}

int 
VPLFile_Sync(VPLFile_handle_t h)
{
    int rv = VPL_OK;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_Sync(h);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    if (_commit(h) == -1) {
        rv = VPLError_XlatErrno(errno);
        goto end;
    }

 end:
    return rv;
}

VPLFile_offset_t
VPLFile_Seek(VPLFile_handle_t h, VPLFile_offset_t offset, int whence)
{
    VPLFile_offset_t rv = -1;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_Seek(h, offset, whence);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    rv = _lseeki64(h, offset, whence);
    if(rv == -1) {
        rv = VPLError_XlatErrno(errno);
    }

    return rv;
}

int 
VPLFile_Close(VPLFile_handle_t h)
{
    int rv = VPL_OK;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(h);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_Close(h);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    // http://msdn.microsoft.com/en-us/library/bdts1c9x.aspx
    // To close a file opened with _open_osfhandle, call _close. The underlying
    // handle is also closed by a call to _close, so it is not necessary to call
    // the Win32 function CloseHandle on the original handle.
    if (_close(h) == -1) {
        rv = VPLError_XlatErrno(errno);
        goto end;
    }

 end:
    return rv;
}

#ifndef VPL_PLAT_IS_WINRT
FILE *
VPLFile_FOpen(const char *pathname, const char *mode)
{
    FILE *fp = NULL;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;
    DWORD desiredAccess;
    DWORD shareMode;
    DWORD createDisposition;
    HANDLE h32;
    int fd;

    rc = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rc != VPL_OK) {
        fp = NULL;
        // FIXME: set errno
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
        fp = NULL;
        // FIXME: set errno
        goto end;
    }

    desiredAccess = GENERIC_READ;  // always grant read access
    if (strchr(mode, 'w') || strchr(mode, 'a')) {
        desiredAccess |= GENERIC_WRITE;
    }
    shareMode = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;
    createDisposition = OPEN_ALWAYS;

    {
        h32 = CreateFile(epathname, desiredAccess, shareMode, NULL, createDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h32 == INVALID_HANDLE_VALUE) {
            fp = NULL;
            goto end;
        }
    }
    fd = _open_osfhandle((intptr_t)h32, 0);
    if (fd == -1) {
        fp = NULL;
        goto end;
    }
    fp = _fdopen(fd, mode);

 end:
    if (wpathname != NULL) {
        free(wpathname);
    }
    if (epathname != NULL) {
        free(epathname);
    }
    return fp;
}
#endif

int 
VPLFile_Delete(const char *pathname)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_Delete(pathname);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

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

    if (DeleteFile(epathname) == 0) {
        rv = VPLError_XlatWinErrno(GetLastError());
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
VPLFile_SetTime(const char *pathname, VPLTime_t time)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    ULARGE_INTEGER t;
    FILETIME ft;
    HANDLE h = INVALID_HANDLE_VALUE;
    int rc;

    /* VPLTime_t epoch is Jan 1, 1970.
     * Windows' epoch is Jan 1, 1601. (http://msdn.microsoft.com/en-us/library/ms724284%28v=VS.85%29.aspx)
     * EPOCH_DIFF defines the difference in 100ns ticks
     */
    ULONGLONG EPOCH_DIFF = 116444736000000000LL;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLFile_SetTime(pathname, time);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

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

    {
#ifdef VPL_PLAT_IS_WINRT
        CREATEFILE2_EXTENDED_PARAMETERS cf2ex = {0};
        cf2ex.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
        cf2ex.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        cf2ex.hTemplateFile = NULL;
        cf2ex.dwFileFlags = FILE_FLAG_BACKUP_SEMANTICS;
        h = CreateFile2(epathname, FILE_WRITE_ATTRIBUTES, 0, OPEN_EXISTING, &cf2ex);
#else
        /* opening a handle to a directory will fail unless FILE_FLAG_BACKUP_SEMANTICS is provided
         * cf. http://msdn.microsoft.com/en-us/library/aa363858%28v=VS.85%29.aspx
         */
        h = CreateFile(epathname, FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
#endif
        if (h == INVALID_HANDLE_VALUE) {
            rv = VPLError_XlatWinErrno(GetLastError());
            goto end;
        }
    }

#ifdef VPL_PLAT_IS_WINRT
    // SetFileInformationByHandle is in Win32 subset APIs
    // for Windows Store Apps to set file time
    UNUSED(t);
    UNUSED(ft);
    {
        FILE_BASIC_INFO  fbi = {0};
        fbi.LastWriteTime.QuadPart = EPOCH_DIFF + time * 10; 
        if (SetFileInformationByHandle(h, FileBasicInfo, &fbi, sizeof(fbi)) == 0) {
            rv = VPLError_XlatWinErrno(GetLastError());
            goto end;
        }
    }
#else
    t.QuadPart = EPOCH_DIFF + time * 10;
    ft.dwLowDateTime = t.u.LowPart;
    ft.dwHighDateTime = t.u.HighPart;
    if (SetFileTime(h, NULL, &ft, &ft) == 0) {
        rv = VPLError_XlatWinErrno(GetLastError());
        goto end;
    }
#endif

end:
    if (wpathname != NULL) {
        free(wpathname);
    }
    if (epathname != NULL) {
        free(epathname);
    }
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }
    return rv;
}

int 
VPLFile_Rename(const char *oldpath, const char *newpath)
{
    int rv = VPL_OK;
    wchar_t *woldpath = NULL, *wnewpath = NULL;
    wchar_t *eoldpath = NULL, *enewpath = NULL;
    DWORD flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
    int rc;

#ifdef VPL_PLAT_IS_WINRT
    {
        /// Support path in Libraries and in LocalState folder which
        /// length of path in LocalState folder are allowed to exceed MAX_PATH by using existed Win32 APIs
        /// If both oldpath and newpath are in Libraries, use WinRT SDK to rename file
        /// If both oldpath and newpath are in LocalState folder, use Win32 APIs to rename file
        /// Else, manually do file open and copying.
        int rc_old = _VPLFS_IsInLocalStatePath(oldpath);
        int rc_new = _VPLFS_IsInLocalStatePath(newpath);
        
        if ((rc_old != VPL_OK && rc_old != VPL_ERR_NOENT) || (rc_new != VPL_OK && rc_new != VPL_ERR_NOENT))
            return VPL_ERR_FAIL;

        if ( rc_old == VPL_ERR_NOENT && rc_new == VPL_ERR_NOENT )
            return _VPLFile_Rename(oldpath, newpath);
        if ( rc_old == VPL_OK && rc_new == VPL_OK )
            ;  // Do nothing
        else {
            VPLFile_handle_t h_old = VPLFILE_INVALID_HANDLE, h_new = VPLFILE_INVALID_HANDLE;
            // Open src file path to read
            if (rc_old == VPL_ERR_NOENT)
                h_old = _VPLFile_Open(oldpath, VPLFILE_OPENFLAG_READONLY, 0777);
            else
                h_old = VPLFile_Open(oldpath, VPLFILE_OPENFLAG_READONLY, 0777);
            if (!VPLFile_IsValidHandle(h_old)) {
                return h_old;
            }
            // Open dst file path to write
            if (rc_new == VPL_ERR_NOENT)
                h_new = _VPLFile_Open(newpath, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
            else
                h_new = VPLFile_Open(newpath, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
            if (!VPLFile_IsValidHandle(h_new)) {
                VPLFile_Close(h_old);
                return h_new;
            }
            
            {
                // Read from src file path, and write to dst file path
                rv = VPL_OK;
                ssize_t byteRead = 0;
                char buffer[MAX_BUFFER_SIZE] = {0};
                while ( (byteRead = VPLFile_Read(h_old, (void*)buffer, MAX_BUFFER_SIZE)) > 0 ) {
                    ssize_t sizeWritten = VPLFile_Write(h_new, buffer, byteRead);
                    if (sizeWritten < 0) {
                        rv = sizeWritten;
                        break;
                    }
                    if (sizeWritten != byteRead) {
                        rv = VPL_ERR_FAIL;
                        break;
                    }
                }
                if (byteRead < 0)
                    rv = byteRead;

                VPLFile_Close(h_old);
                VPLFile_Close(h_new);

                if (rv == VPL_OK)
                    // Finish copy, does oldpath deletion but VPLFile_Rename ignore result
                    VPLFile_Delete(oldpath);
                else
                    // if failed, rollback to delete newpath
                    VPLFile_Delete(newpath);

                return rv;
            }
        }
    }
#endif

    rc = _VPL__utf8_to_wstring(oldpath, &woldpath);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPL__utf8_to_wstring(newpath, &wnewpath);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(woldpath, &eoldpath, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    rc = _VPLFS__GetExtendedPath(wnewpath, &enewpath, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    // Remove MOVEFILE_REPLACE_EXISTING from the flags variable.
    // MOVEFILE_REPLACE_EXISTING cannot be used if either path is a directory
    // cf. http://msdn.microsoft.com/en-us/library/aa365240%28VS.85%29.aspx
    {
        DWORD attrs;
#ifdef VPL_PLAT_IS_WINRT
        WIN32_FILE_ATTRIBUTE_DATA fileInfo = {0};
        BOOL bRt = GetFileAttributesEx(eoldpath, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileInfo);
        if (bRt == TRUE) {
            attrs = fileInfo.dwFileAttributes;
        }
        else {
#else
        attrs = GetFileAttributes(eoldpath);
        if (attrs == INVALID_FILE_ATTRIBUTES) {
#endif
            rv = VPLError_XlatWinErrno(GetLastError());
            goto end;
        }
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            flags &= ~MOVEFILE_REPLACE_EXISTING;
        }
    }

    // If the target is a directory, remove it.
    // (Otherwise, MoveFileEx() will fail with ERROR_ALREADY_EXISTS.)
    {
        DWORD attrs;
#ifdef VPL_PLAT_IS_WINRT
        WIN32_FILE_ATTRIBUTE_DATA fileInfo = {0};
        BOOL bRt = GetFileAttributesEx(enewpath, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileInfo);
        if (bRt == TRUE) {
            attrs = fileInfo.dwFileAttributes;
        }
        else
            attrs = INVALID_FILE_ATTRIBUTES; 
#else
        attrs = GetFileAttributes(enewpath);
#endif
        if ((attrs != INVALID_FILE_ATTRIBUTES) &&
            (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            if (RemoveDirectory(enewpath) == 0) {
                rv = VPLError_XlatWinErrno(GetLastError());
                goto end;
            }
        }
    }

    if (MoveFileEx(eoldpath, enewpath, flags) == 0) {
        rv = VPLError_XlatWinErrno(GetLastError());
        goto end;
    }

 end:
    if (woldpath != NULL) {
        free(woldpath);
    }
    if (wnewpath != NULL) {
        free(wnewpath);
    }
    if (eoldpath != NULL) {
        free(eoldpath);
    }
    if (enewpath != NULL) {
        free(enewpath);
    }

    return rv;
}

int 
VPLFile_Move(const char *oldpath, const char *newpath)
{
    int rv = VPL_OK;
    wchar_t *woldpath = NULL, *wnewpath = NULL;
    wchar_t *eoldpath = NULL, *enewpath = NULL;
    DWORD flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED;
    int rc;

#ifdef VPL_PLAT_IS_WINRT
    {
        /// Support path in Libraries and in LocalState folder which
        /// length of path in LocalState folder are allowed to exceed MAX_PATH by using existed Win32 APIs
        /// If both oldpath and newpath are in Libraries, use WinRT SDK to move file
        /// If both oldpath and newpath are in LocalState folder, use Win32 APIs to move file
        /// Else, manually do file open and copying.
        int rc_old = _VPLFS_IsInLocalStatePath(oldpath);
        int rc_new = _VPLFS_IsInLocalStatePath(newpath);
        
        if ((rc_old != VPL_OK && rc_old != VPL_ERR_NOENT) || (rc_new != VPL_OK && rc_new != VPL_ERR_NOENT))
            return VPL_ERR_FAIL;

        if ( rc_old == VPL_ERR_NOENT && rc_new == VPL_ERR_NOENT )
            return _VPLFile_Rename(oldpath, newpath);
        if ( rc_old == VPL_OK && rc_new == VPL_OK )
            ;  // Do nothing
        else {
            VPLFile_handle_t h_old = VPLFILE_INVALID_HANDLE, h_new = VPLFILE_INVALID_HANDLE;
            // Open src file path to read
            if (rc_old == VPL_ERR_NOENT)
                h_old = _VPLFile_Open(oldpath, VPLFILE_OPENFLAG_READONLY, 0777);
            else
                h_old = VPLFile_Open(oldpath, VPLFILE_OPENFLAG_READONLY, 0777);
            if (!VPLFile_IsValidHandle(h_old)) {
                return h_old;
            }
            // Open dst file path to write
            if (rc_new == VPL_ERR_NOENT)
                h_new = _VPLFile_Open(newpath, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
            else
                h_new = VPLFile_Open(newpath, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
            if (!VPLFile_IsValidHandle(h_new)) {
                VPLFile_Close(h_old);
                return h_new;
            }
            
            {
                // Read from src file path, and write to dst file path
                rv = VPL_OK;
                ssize_t byteRead = 0;
                char buffer[MAX_BUFFER_SIZE] = {0};
                while ( (byteRead = VPLFile_Read(h_old, (void*)buffer, MAX_BUFFER_SIZE)) > 0 ) {
                    ssize_t sizeWritten = VPLFile_Write(h_new, buffer, byteRead);
                    if (sizeWritten < 0) {
                        rv = sizeWritten;
                        break;
                    }
                    if (sizeWritten != byteRead) {
                        rv = VPL_ERR_FAIL;
                        break;
                    }
                }
                if (byteRead < 0)
                    rv = byteRead;

                VPLFile_Close(h_old);
                VPLFile_Close(h_new);

                if (rv == VPL_OK)
                    // Finish copy, return results of oldpath deletion (VPLFile_Move returns failed if oldpath cannot be deleted)
                    rv = VPLFile_Delete(oldpath);
                else
                    // if failed, rollback to delete newpath
                    VPLFile_Delete(newpath);
                return rv;
            }
        }
    }
#endif
    rc = _VPL__utf8_to_wstring(oldpath, &woldpath);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPL__utf8_to_wstring(newpath, &wnewpath);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(woldpath, &eoldpath, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    rc = _VPLFS__GetExtendedPath(wnewpath, &enewpath, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    // Remove MOVEFILE_REPLACE_EXISTING from the flags variable.
    // MOVEFILE_REPLACE_EXISTING cannot be used if either path is a directory
    // cf. http://msdn.microsoft.com/en-us/library/aa365240%28VS.85%29.aspx
    {
        DWORD attrs;
#ifdef VPL_PLAT_IS_WINRT
        WIN32_FILE_ATTRIBUTE_DATA fileInfo = {0};
        BOOL bRt = GetFileAttributesEx(eoldpath, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileInfo);
        if (bRt == TRUE) {
            attrs = fileInfo.dwFileAttributes;
        }
        else {
#else
        attrs = GetFileAttributes(eoldpath);
        if (attrs == INVALID_FILE_ATTRIBUTES) {
#endif
            rv = VPLError_XlatWinErrno(GetLastError());
            goto end;
        }
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            flags &= ~MOVEFILE_REPLACE_EXISTING;
        }
    }

    // If the target is a directory, remove it.
    // (Otherwise, MoveFileEx() will fail with ERROR_ALREADY_EXISTS.)
    {
        DWORD attrs;
#ifdef VPL_PLAT_IS_WINRT
        WIN32_FILE_ATTRIBUTE_DATA fileInfo = {0};
        BOOL bRt = GetFileAttributesEx(enewpath, GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileInfo);
        if (bRt == TRUE) {
            attrs = fileInfo.dwFileAttributes;
        }
        else
            attrs = INVALID_FILE_ATTRIBUTES; 
#else
        attrs = GetFileAttributes(enewpath);
#endif
        if ((attrs != INVALID_FILE_ATTRIBUTES) &&
            (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            if (RemoveDirectory(enewpath) == 0) {
                rv = VPLError_XlatWinErrno(GetLastError());
                goto end;
            }
        }
    }

    if (MoveFileEx(eoldpath, enewpath, flags) == 0) {
        rv = VPLError_XlatWinErrno(GetLastError());
        goto end;
    }

 end:
    if (woldpath != NULL) {
        free(woldpath);
    }
    if (wnewpath != NULL) {
        free(wnewpath);
    }
    if (eoldpath != NULL) {
        free(eoldpath);
    }
    if (enewpath != NULL) {
        free(enewpath);
    }

    return rv;
}

#ifndef VPL_PLAT_IS_WINRT
int VPLFile_SetAttribute(const char *path, u32 attrs, u32 maskbits)
{
    int rv = VPL_OK;
    int rc;
    DWORD winAttrs;
    DWORD winNewAttrs;
    DWORD winMask;
    wchar_t *wpath = NULL;
    wchar_t *epath = NULL;

    if (attrs & ~(VPLFILE_ATTRIBUTE_MASK)) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    if (maskbits & ~(VPLFILE_ATTRIBUTE_MASK)) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    rc = _VPL__utf8_to_wstring(path, &wpath);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    rc = _VPLFS__GetExtendedPath(wpath, &epath, 0);
    if (rc != VPL_OK) {
        rv = rc;
        goto end;
    }

    {
        winAttrs = GetFileAttributes(epath);
        if (winAttrs == INVALID_FILE_ATTRIBUTES) {
            rv = VPLError_XlatWinErrno(GetLastError());
            goto end;
        }
    }

    // Remove any bits that SetFileAttribute() won't be able to take.
    // List of attributes supported by SetFileAttribute() can be found at:
    // http://msdn.microsoft.com/en-us/library/windows/desktop/aa365535%28v=vs.85%29.asxp
    winAttrs &= 
        FILE_ATTRIBUTE_ARCHIVE |
        FILE_ATTRIBUTE_HIDDEN |
        FILE_ATTRIBUTE_NORMAL |
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
        FILE_ATTRIBUTE_OFFLINE |
        FILE_ATTRIBUTE_READONLY |
        FILE_ATTRIBUTE_SYSTEM |
        FILE_ATTRIBUTE_TEMPORARY;

    winNewAttrs = 0;
    if (attrs & VPLFILE_ATTRIBUTE_READONLY)
        winNewAttrs |= FILE_ATTRIBUTE_READONLY;
    if (attrs & VPLFILE_ATTRIBUTE_HIDDEN)
        winNewAttrs |= FILE_ATTRIBUTE_HIDDEN;
    if (attrs & VPLFILE_ATTRIBUTE_SYSTEM)
        winNewAttrs |= FILE_ATTRIBUTE_SYSTEM;
    if (attrs & VPLFILE_ATTRIBUTE_ARCHIVE)
        winNewAttrs |= FILE_ATTRIBUTE_ARCHIVE;

    winMask = 0;
    if (maskbits & VPLFILE_ATTRIBUTE_READONLY)
        winMask |= FILE_ATTRIBUTE_READONLY;
    if (maskbits & VPLFILE_ATTRIBUTE_HIDDEN)
        winMask |= FILE_ATTRIBUTE_HIDDEN;
    if (maskbits & VPLFILE_ATTRIBUTE_SYSTEM)
        winMask |= FILE_ATTRIBUTE_SYSTEM;
    if (maskbits & VPLFILE_ATTRIBUTE_ARCHIVE)
        winMask |= FILE_ATTRIBUTE_ARCHIVE;

    winAttrs = (winAttrs & ~winMask) | (winNewAttrs & winMask);
    if (!winAttrs)
        winAttrs|= FILE_ATTRIBUTE_NORMAL;
    if (!SetFileAttributes(epath, winAttrs)) {
        rv = VPLError_XlatWinErrno(GetLastError());
        goto end;
    }

 end:
    if (wpath != NULL) {
        free(wpath);
    }
    if (epath != NULL) {
        free(epath);
    }
    return rv;
}
#endif

int
VPLDir_Create(const char *pathname, int mode)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;

    UNUSED(mode);

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLDir_Create(pathname, mode);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

    // if the request is something like "C:", just return success
    if (strlen(pathname) == 2 && isalpha(pathname[0]) && pathname[1] == ':')
        return VPL_OK;

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
VPLDir_Delete(const char *pathname)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFS_IsInLocalStatePath(pathname);
        if ( rc == VPL_ERR_NOENT )
            return _VPLDir_Delete(pathname);
        else if (rc != VPL_OK )
            return rc;
    }
#endif

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

#ifndef VPL_PLAT_IS_WINRT
int 
VPLPipe_Create(VPLFile_handle_t handles[2])
{
    return _pipe(handles, 4096, O_BINARY);
}
#endif

#define HAS_ATTR(flag, attr) ((flag & attr) ? VPL_TRUE : VPL_FALSE)

int
VPLFS_FStat(VPLFile_handle_t fd, VPLFS_stat_t* buf)
{
    int rv = VPL_OK;
    int	rc = -1;
    HANDLE h = INVALID_HANDLE_VALUE;

#ifdef VPL_PLAT_IS_WINRT
    {
        int rc = _VPLFile_IsFileInLocalStatePath(fd);
        if ( rc == VPL_ERR_NOENT ) {
            // File is not within the local app data, so need to use WinRT APIs.
            return _VPLFS_FStat(fd,buf);
        }
        else if (rc != VPL_OK ) {
            return rc;
        }
        // else VPL_OK => File is within the local app data, so it's safe to use Win32 APIs.
    }
#endif

    // number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
    const VPLTime_t EPOCH_DIFF = VPLTime_FromSec(11644473600LL);

    if (fd < 0 || buf == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    h = (HANDLE) _get_osfhandle(fd);

    // A file time is a 64-bit value that represents the number of 100-nanosecond
    // intervals that have elapsed since 12:00 A.M. January 1, 1601 Coordinated
    // Universal Time (UTC). The system records file times when applications
    // create, access, and write to files.
#ifdef VPL_PLAT_IS_WINRT
    {//YL: this may be the same..
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

        buf->isHidden = HAS_ATTR(fbi.FileAttributes, FILE_ATTRIBUTE_HIDDEN);
        buf->isSymLink = HAS_ATTR(fbi.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);
        buf->isReadOnly = HAS_ATTR(fbi.FileAttributes, FILE_ATTRIBUTE_READONLY);
        buf->isSystem = HAS_ATTR(fbi.FileAttributes, FILE_ATTRIBUTE_SYSTEM);
        buf->isArchive = HAS_ATTR(fbi.FileAttributes, FILE_ATTRIBUTE_ARCHIVE);
        buf->type = (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? VPLFS_TYPE_DIR : VPLFS_TYPE_FILE;

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

    BY_HANDLE_FILE_INFORMATION  info;
    rc = GetFileInformationByHandle(h, &info);

    if (!rc) {
        rv = VPLError_XlatErrno(errno);
        goto end;
    }

    // 1) Combine High and Low into a u64 integer, representing 100 Nanosec since Jan. 1, 1601 00:00
    // 2) 100 Nanosec -> Nanosecs
    // 3) Nanosecs -> VPLTime_t
    // 4) Starting Year 1601 -> Starting Year 1970 using EPOCH diff.
    buf->vpl_atime  = VPLTime_DiffClamp(VPLTime_FromNanosec(
                                            ((((u64)info.ftLastAccessTime.dwHighDateTime)<<32) + ((u64)info.ftLastAccessTime.dwLowDateTime)) * 100),
                                        EPOCH_DIFF);
    buf->vpl_mtime  = VPLTime_DiffClamp(VPLTime_FromNanosec(
                                            ((((u64)info.ftLastWriteTime.dwHighDateTime)<<32) + ((u64)info.ftLastWriteTime.dwLowDateTime)) * 100),
                                        EPOCH_DIFF);
    buf->vpl_ctime  = VPLTime_DiffClamp(VPLTime_FromNanosec(
                                            ((((u64)info.ftCreationTime.dwHighDateTime)<<32) + ((u64)info.ftCreationTime.dwLowDateTime)) * 100),
                                        EPOCH_DIFF);

    buf->isHidden = HAS_ATTR(info.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN);
    buf->isSymLink = HAS_ATTR(info.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);
    buf->isReadOnly = HAS_ATTR(info.dwFileAttributes, FILE_ATTRIBUTE_READONLY);
    buf->isSystem = HAS_ATTR(info.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM);
    buf->isArchive = HAS_ATTR(info.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE);
    buf->type = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? VPLFS_TYPE_DIR : VPLFS_TYPE_FILE;
#endif

    buf->atime = VPLTime_ToSec(buf->vpl_atime);
    buf->mtime = VPLTime_ToSec(buf->vpl_mtime);
    buf->ctime = VPLTime_ToSec(buf->vpl_ctime);

end:
    return  rv;
}