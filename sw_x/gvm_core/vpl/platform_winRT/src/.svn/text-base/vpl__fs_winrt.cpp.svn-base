//
//  Copyright 2012 iGware Inc.
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
#include "vpl__fs_priv.h"

#if __MSVCRT_VERSION__ < 0x0700
#error "__MSVCRT_VERSION__ must be at least 0x0700"
#endif


#include <errno.h>
#include <string.h>

#include "vplu.h"
#include "vpl__plat.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"
#include "scopeguard.hpp"

#ifdef __cplusplus
#include <string>
#include <map>
#endif

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Storage::FileProperties;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core;

static VPLLazyInitMutex_t s_mutex = VPLLAZYINITMUTEX_INIT;
static std::map<int, vplFolderChainer^> m_FolderHandlePool;
static std::map<std::wstring, VPLLazyInitMutex_t> m_FileMutexPool;
static VPLLazyInitMutex_t s_localpath_mutex = VPLLAZYINITMUTEX_INIT;
static wchar_t *s_wLocalAppPath = NULL;

int
_VPLFS_Error_XlatErrno(HRESULT hResult)
{
    int rv;
    if (hResult == 0x8000000B)
        rv = VPL_ERR_INVALID;
    else {
        DWORD dwRt = WIN32_FROM_HRESULT(hResult);
        rv = VPLError_XlatWinErrno(dwRt);
    }

    return rv;
}

/// Converts the UniversalTime field of a Windows::Foundation::DateTime to time_t.
time_t _vplfs_converttime(long long time)
{
    // DateTime::UniversalTime provides the date and time expressed in Coordinated Universal Time.
    // Windows::Foundation::DateTime is the number of 100ns units since 1/1/1601
    // http://msdn.microsoft.com/en-us/library/windows/apps/windows.foundation.datetime
    // Times in VPLFS_stat_t structure are time_t (the number of seconds since epoch (00:00:00 UTC on 1 January 1970))
    ULONGLONG COOR_TO_EPOCH_DIFF = 116444736000000000LL;

    // Here minus 1LL is because there is always 1 second difference on WinRT SDK and c runtime lib.
    return (time - COOR_TO_EPOCH_DIFF ) / 10000000LL - 1LL;
}

vplFileChainer::vplFileChainer()
{
    file=nullptr;
    curpos = 0;
}

vplFolderChainer::vplFolderChainer() 
{
    folder=nullptr;
    curpos = 0;
}

static int _push_folderhandle(vplFolderChainer^ folder)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    int h = folder->GetHashCode() % MAXINT;
    if (h < 0)
        h = 0;
    if (m_FolderHandlePool.size() > 0) {
        while (m_FolderHandlePool.count(h) > 0) {
            h = (h+1) % MAXINT;
        }
    }

    m_FolderHandlePool.insert(std::pair<int, vplFolderChainer^>(h, folder));
    return h;
}

static vplFolderChainer^ _get_folderhandle(int handle)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    vplFolderChainer^ h = nullptr;
    if (m_FolderHandlePool.size() > 0 && m_FolderHandlePool.count(handle) > 0)
        h = m_FolderHandlePool[handle];
    return h;
}

static void _reset_folderhandle(vplFolderChainer^ folder)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    folder->folder = nullptr;
    folder->curpos = 0;
}

static int _remove_folderhandle(int handle)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    int rv = VPL_OK;
    if (m_FolderHandlePool.size() > 0 && m_FolderHandlePool.count(handle) > 0) {
        _reset_folderhandle(m_FolderHandlePool[handle]);
        m_FolderHandlePool.erase(handle);
    }
    else {
        rv = VPL_ERR_INVALID;
    }
    return rv;
}

static void _clear_folderhandlepool()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (m_FolderHandlePool.size() > 0) {        
        m_FolderHandlePool.clear();
    }
}

void _fileio_lock(const wchar_t *wpathname, HANDLE *event, IOLOCK_TYPE type)
{
    HashAlgorithmProvider ^alg = HashAlgorithmProvider::OpenAlgorithm(ref new String(L"SHA1"));
    String^ hashStr = ref new String(wpathname)+"*"; //"*" is to avoid file name because of Windows system do not allow * to be file name.
    switch(type) {
    case IOLOCK_TYPE::IOLOCK_OPEN:
        hashStr += "IOLOCK_OPEN";
        break;
    case IOLOCK_TYPE::IOLOCK_READ:
        hashStr += "IOLOCK_READ";
        break;
    case IOLOCK_TYPE::IOLOCK_WRITE:
        hashStr += "IOLOCK_WRITE";
        break;
    case IOLOCK_TYPE::IOLOCK_STAT:
        hashStr += "IOLOCK_STAT";
        break;
    default:
        break;
    }
    auto pathbuf = CryptographicBuffer::ConvertStringToBinary(hashStr, BinaryStringEncoding::Utf8);
    auto hashedbuf = alg->HashData(pathbuf);
    auto hashedstr = CryptographicBuffer::EncodeToBase64String(hashedbuf);
    (*event) = CreateMutexEx(nullptr, hashedstr->Data(), 0, MUTEX_ALL_ACCESS);

    WaitForSingleObjectEx((*event), INFINITE, TRUE);
}

void _fileio_unlock(HANDLE event)
{
    ReleaseMutex(event);
}

int _vpl_openfile(const wchar_t* wpathname, vplFileChainer^ file)
{
    int rv = VPL_OK;

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if ( completedEvent ) {
        IAsyncOperation<StorageFile^> ^openfileAction = StorageFile::GetFileFromPathAsync(ref new String(wpathname));
        openfileAction->Completed = ref new AsyncOperationCompletedHandler<StorageFile^>(
            [&rv, &completedEvent, file]
            (IAsyncOperation<StorageFile^>^ op, AsyncStatus status) {
            try {
                auto openedFile = op->GetResults();
                if (file != nullptr)
                    file->file = openedFile;
                rv = VPL_OK;
            }
            catch(Exception^ ex) {
                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == ex->HResult
                    || HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == ex->HResult)
                    rv = VPL_ERR_NOENT;
                else
                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
            }
            SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
        CloseHandle(completedEvent);
    }
    else
        rv = VPL_ERR_FAIL;

    return rv;
}

int _crack_path(const wchar_t* wpathname, wchar_t** wparent, wchar_t** wfilename)
{
    int rv = VPL_OK;

    // Retrieve parent folder for file creation
    std::wstring wpath(wpathname);
    size_t pos = wpath.find_last_of(L"/\\");
    
    if (pos == std::string::npos)
        rv = VPL_ERR_INVALID;
    else {
        std::wstring parent = wpath.substr(0, pos);
        std::wstring filename = wpath.substr(pos+1, wpath.size() - parent.size() - 1);

        *wparent = (wchar_t*)malloc((parent.size()+1) * sizeof(wchar_t));
        wcscpy(*wparent, parent.c_str());
        (*wparent)[parent.size()] = L'\0';

        *wfilename = (wchar_t*)malloc((filename.size()+1) * sizeof(wchar_t));
        wcscpy(*wfilename, filename.c_str());
        (*wfilename)[filename.size()] = L'\0';
    }

    return rv;
}

static int _create_folder(const wchar_t* wpathname, vplFolderChainer^ folder, CreationCollisionOption option)
{
    int rv = VPL_OK;
    wchar_t *wParentPath=NULL, *wFolderName=NULL;

    {
        // In WinRT, folder creation can only be achieved by having its parent's StorageFolder object
        // Retrieve parent folder for file creation
        rv = _crack_path(wpathname, &wParentPath, &wFolderName);
        if (rv == VPL_OK) {
            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if( completedEvent ) {
                IAsyncOperation<StorageFolder^> ^openfolderAction = StorageFolder::GetFolderFromPathAsync(ref new String(wParentPath));

                openfolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
                    [&completedEvent, &rv, wpathname, wParentPath, wFolderName, folder, option]
                    (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                    try {
                        auto openedfolder = op->GetResults();

                        HANDLE createfolder_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                        try {
                            openedfolder->CreateFolderAsync(ref new String(wFolderName), option)->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
                                [&createfolder_event, &rv, folder]
                                (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                                    try {
                                        if (folder != nullptr)
                                            folder->folder = op->GetResults();
                                        rv = VPL_OK;
                                    }
                                    catch (Exception^ ex) {
                                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                                    }
                                    SetEvent(createfolder_event);
                            });
                            WaitForSingleObjectEx(createfolder_event, INFINITE, TRUE);
                            CloseHandle(createfolder_event);
                        }
                        catch (Exception^ ex) {
                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        }
                    }
                    catch (Exception^ ex) {
                        if (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == ex->HResult) {
                            VPLFS_stat_t stat;
                            char *parentPath;
                            _VPL__wstring_to_utf8_alloc(wParentPath, &parentPath);
                            int rc = VPLFS_Stat(parentPath, &stat);
                            if (rc == VPL_OK && stat.type != VPLFS_TYPE_DIR)
                                rv = VPL_ERR_NOTDIR;
                            else
                                rv = _VPLFS_Error_XlatErrno(ex->HResult);
                            free(parentPath);
                        }
                        else {
                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        }
                    }
                    SetEvent(completedEvent);
                });
                WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
                CloseHandle(completedEvent);
            }
            else {
                rv = VPL_ERR_FAIL;
            }
        }
    }

end:
    if (wParentPath != NULL)
        free(wParentPath);
    if (wFolderName != NULL)
        free(wFolderName);
    return rv;
}

static int _openfolder(const wchar_t* wpathname, vplFolderChainer^ folder)
{
    int rv = 0;

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( completedEvent ) {
        IAsyncOperation<StorageFolder^> ^openfolderAction = StorageFolder::GetFolderFromPathAsync(ref new String(wpathname));

        openfolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
            [&completedEvent, &rv, folder]
            (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
            try {
                auto openedfolder = op->GetResults();
                if (folder != nullptr)
                    folder->folder = openedfolder;
                rv = VPL_OK;
            }
            catch (Exception^ ex) {
                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == ex->HResult
                    || HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == ex->HResult)
                    rv = VPL_ERR_NOENT;
                else
                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
            }
            SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
        CloseHandle(completedEvent);
    }
    else
        rv = VPL_ERR_FAIL;

    return rv;
}

static int _reloaddir(VPLFS_dir_t *dir)
{
    int rv = VPL_OK;
    HANDLE completedEvent = INVALID_HANDLE_VALUE;
    completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        rv = VPL_ERR_FAIL;
    }
    else {
        vplFolderChainer^ folder = _get_folderhandle(dir->handle_key);
        if (folder != nullptr) {
            // refresh vplFolderChainer::items
            try {
                auto getItemsAction = folder->folder->GetItemsAsync();
                getItemsAction->Completed = ref new AsyncOperationCompletedHandler<IVectorView<IStorageItem^>^> (
                    [&rv, &completedEvent, dir, folder] (IAsyncOperation<IVectorView<IStorageItem^>^>^ op, AsyncStatus status) {
                    try {
                        auto items = op->GetResults();
                        if (items->Size >= 0)
                            dir->max_pos = items->Size;
                        folder->items = items;
                    }
                    catch (Exception^ ex) {
                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                    }
                    SetEvent(completedEvent);
                });
                WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
            }
            catch (Exception^ ex) {
                rv = _VPLFS_Error_XlatErrno(ex->HResult);
            }
            CloseHandle(completedEvent);
        }
    }

    return rv;
}

int _VPLFS__ConvertPath(const wchar_t *wpath, wchar_t **epath, size_t extrachars)
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

    pos = wpath2.find_last_of(L"/\\");
    if (pos == wpath2.length() - 1)
        wpath2.erase(wpath2.length() - 1);

    // replace "\.\" with "\"
    pos = wpath2.find(L"\\.\\");
    while (pos != std::wstring::npos) {
        wpath2.replace(pos, 3, 1, L'\\');  // 3 == wsclen(L"\\.\\")
        if (pos + 1 < wpath2.length())
            pos = wpath2.find(L"\\.\\", pos + 1);
        else
            pos = std::wstring::npos;
    }

    size_t epath_bufsize = wpath2.length() + 1 + extrachars;
    *epath = (wchar_t*)malloc(epath_bufsize * sizeof(wchar_t));
    memset((void*)*epath, 0, epath_bufsize * sizeof(wchar_t));
    if (*epath == NULL)
        return VPL_ERR_NOMEM;
    wcscpy(*epath, wpath2.c_str());

    return VPL_OK;
}

//--------------------------------------------------------------

int 
_VPLFS_Stat(const char* pathname, VPLFS_stat_t* buf)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

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

    rv = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rv != VPL_OK) {
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

    rv = _VPLFS__ConvertPath(wpathname, &epathname, 0);
    if (rv != VPL_OK) {
        goto end;
    }

    {
        HANDLE stat_mutex;
        _fileio_lock(epathname,&stat_mutex, IOLOCK_TYPE::IOLOCK_STAT);
        ON_BLOCK_EXIT(_fileio_unlock, stat_mutex);

        vplFileChainer^ file = ref new vplFileChainer();
        vplFolderChainer^ folder = ref new vplFolderChainer();

        // WinRT limitation, cannot tell hidden and symbolic attributes
        buf->isHidden = VPL_FALSE;
        buf->isSymLink = VPL_FALSE;

        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if ( !completedEvent ) {
            rv = VPL_ERR_FAIL;
            goto end;
        }

        rv = _vpl_openfile(epathname, file);
        if (rv == VPL_OK) {
            // file found, collect its state
            auto getPropsAction = file->file->GetBasicPropertiesAsync();
            getPropsAction->Completed = ref new AsyncOperationCompletedHandler<BasicProperties^> (
                [&rv, &completedEvent, &buf, file] (IAsyncOperation<BasicProperties^>^ op, AsyncStatus status) {
                    try {
                        auto props = op->GetResults();
                        buf->type = VPLFS_TYPE_FILE;
                        buf->size = props->Size;

                        buf->atime = _vplfs_converttime(props->DateModified.UniversalTime);  // WinRT limitation, cannot retrieve last access time
                        buf->mtime = _vplfs_converttime(props->DateModified.UniversalTime);
                        buf->ctime = _vplfs_converttime(file->file->DateCreated.UniversalTime);
                    }
                    catch (Exception^ ex) {
                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                    }
                    SetEvent(completedEvent);
            });
            WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
            CloseHandle(completedEvent);
        }
        else {
            // file not found, try folder
            rv = _openfolder(epathname, folder);
            if (rv == VPL_OK) {
                auto getPropsAction = folder->folder->GetBasicPropertiesAsync();
                getPropsAction->Completed = ref new AsyncOperationCompletedHandler<BasicProperties^> (
                    [&rv, &completedEvent, &buf, folder] (IAsyncOperation<BasicProperties^>^ op, AsyncStatus status) {
                        try {
                            auto props = op->GetResults();
                            buf->type = VPLFS_TYPE_DIR;

                            buf->atime = _vplfs_converttime(props->DateModified.UniversalTime);  // WinRT limitation, cannot retrieve last access time
                            buf->mtime = _vplfs_converttime(props->DateModified.UniversalTime);
                            buf->ctime = _vplfs_converttime(folder->folder->DateCreated.UniversalTime);
                        }
                        catch (Exception^ ex) {
                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        }
                        SetEvent(completedEvent);
                });
                WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
                CloseHandle(completedEvent);
            }
        }

        buf->vpl_atime = VPLTime_FromSec(buf->atime);
        buf->vpl_mtime = VPLTime_FromSec(buf->mtime);
        buf->vpl_ctime = VPLTime_FromSec(buf->ctime);

        delete file;
        delete folder;
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

void
_VPLFS_Sync(void)
{
    // Do nothing
    ;
}

static int 
opendir(wchar_t *epathname, VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

    vplFolderChainer^ folder = ref new vplFolderChainer();
    rv = _openfolder(epathname, folder);
    if (rv == VPL_OK)
    {
        int key = _push_folderhandle(folder);
        dir->handle_key = key;
        dir->name = epathname;
        dir->pos = 0;
    }
    else {
        dir->handle_key = VPLFS_INVALID_HANDLE;
        dir->name = NULL;
        dir->pos = 0;
    }

end:
    return rv;
}

int
_VPLFS_Opendir(const char* pathname, VPLFS_dir_t* dir)
{
    int rv = VPL_OK;
    int rc = 0;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

    if (dir == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    memset(dir, 0, sizeof(*dir));
    dir->handle_key = VPLFS_INVALID_HANDLE;

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

    rc = _VPLFS__ConvertPath(wpathname, &epathname, 2);  // "2" for appending "/*" later
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
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
_VPLFS_Closedir(VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if (dir->handle_key == VPLFS_INVALID_HANDLE) {
        rv = VPL_ERR_BADF;
    }
    else {
        rv = _remove_folderhandle(dir->handle_key);

        dir->handle_key = VPLFS_INVALID_HANDLE;

        if (dir->name != NULL) {
            free(dir->name);
            dir->name = NULL;
        }

        dir->max_pos = 0;
        dir->pos = 0;
    }
    
    return rv;
}

int 
_VPLFS_Readdir(VPLFS_dir_t* dir, VPLFS_dirent_t* entry)
{
    int rv = VPL_OK;
    
    if(dir == NULL || entry == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (dir->handle_key == VPLFS_INVALID_HANDLE) {
        rv = VPL_ERR_BADF;
        goto end;
    }

    {
        vplFolderChainer^ folder = _get_folderhandle(dir->handle_key);
        if (folder == nullptr) {
            rv = VPL_ERR_INVALID;
            goto end;
        }

        if (folder->items == nullptr)
            // refresh items in dir
            _reloaddir(dir);
        
        {
            // already got items
            if (folder->items->Size > dir->pos) {
                IStorageItem^ item = folder->items->GetAt(dir->pos);
                dir->pos++;
                
                // retrieve filename
                _VPL__wstring_to_utf8(item->Name->Data(), (int)item->Name->Length(), entry->filename, ARRAY_SIZE_IN_BYTES(entry->filename));
                // retrieve type
                if (item->IsOfType(StorageItemTypes::File))
                    entry->type = VPLFS_TYPE_FILE;
                else if (item->IsOfType(StorageItemTypes::Folder))
                    entry->type = VPLFS_TYPE_DIR;
                else
                    entry->type = VPLFS_TYPE_OTHER;
            }
            else {
                // no items or no more items exists
                rv = VPL_ERR_MAX;
            }
        }
    }

 end:
    return rv;
}

int 
_VPLFS_Rewinddir(VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if (dir->handle_key == VPLFS_INVALID_HANDLE) {
        rv = VPL_ERR_BADF;
    }
    else {
        rv = _reloaddir(dir);
        dir->pos = 0;
    }
    
    return rv;
}

int 
_VPLFS_Seekdir(VPLFS_dir_t* dir, size_t pos)
{
    int rv = VPL_OK;
    
    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (dir->handle_key == VPLFS_INVALID_HANDLE) {
        rv = VPL_ERR_BADF;
        goto end;
    }

    {
        dir->pos = 0;

        VPLFS_dirent_t dummy;
        int rc = VPLFS_Readdir(dir, &dummy);
        if (rc != VPL_OK) {
            rv = rc;
        }
        else {
            if (pos < dir->max_pos) {
                dir->pos = (int)pos;
            }
            else
                rv = VPL_ERR_MAX;
        }
    }

 end:    
    return rv;
}

int 
_VPLFS_Telldir(VPLFS_dir_t* dir, size_t* pos)
{
    int rv = VPL_OK;
    
    if(dir == NULL || pos == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if (dir->handle_key == VPLFS_INVALID_HANDLE) {
        rv = VPL_ERR_BADF;
    }
    else {
        *pos = dir->pos;
    }
    
    return rv;
}

int 
_VPLFS_Mkdir(const char* pathname)
{
    int rv = VPL_OK;
    int rc;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

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

    rc = _VPLFS__ConvertPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }
    
    {
        VPLFS_stat_t stat;
        rv = VPLFS_Stat(pathname, &stat);
        if (rv == VPL_OK) {
            if (stat.type != VPLFS_TYPE_DIR)
                rv = VPL_ERR_NOTDIR;
            else
                rv = VPL_ERR_EXIST;
            goto end;
        }
    }

    rv = _create_folder(epathname, nullptr, CreationCollisionOption::FailIfExists);

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
_VPLFS_Rmdir(const char* pathname)
{
    int rv = VPL_OK;
    int rc;
    HANDLE completedEvent = INVALID_HANDLE_VALUE;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

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

    rc = _VPLFS__ConvertPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
        rv = rc;
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

    completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        rv = VPL_ERR_FAIL;
    }
    else {
        vplFolderChainer^ folder = ref new vplFolderChainer();
        rv = _openfolder(epathname, folder);
        if (rv == VPL_OK) {
            if (folder->folder != nullptr) {
                auto deleteAction = folder->folder->DeleteAsync(StorageDeleteOption::Default);
                deleteAction->Completed = ref new AsyncActionCompletedHandler(
                    [&rv, &completedEvent] (IAsyncAction^ op, AsyncStatus status) {
                    try {
                        op->GetResults();
                        rv = VPL_OK;
                    }
                    catch (Exception^ ex) {
                        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == ex->HResult)
                            rv = VPL_ERR_NOENT;
                        else
                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                    }
                    SetEvent(completedEvent);
                });
                WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
                CloseHandle(completedEvent);
            }
        }
        delete folder;
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

int _VPLFS_IsInLocalStatePath(const char *path)
{
    int rv = VPL_OK;
    wchar_t *wpath=NULL, *epath=NULL;

    if (strlen(path) == 0) {
        rv = VPL_ERR_NOENT;
        goto end;
    }

    rv = _VPL__utf8_to_wstring(path, &wpath);
    if (rv != VPL_OK) {
        goto end;
    }

    rv = _VPLFS__ConvertPath(wpath, &epath, 0);
    if (rv != VPL_OK) {
        goto end;
    }

    if (s_wLocalAppPath == NULL) {
        // Get LocalAppData path to s_wLocalAppPath for reuse
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localpath_mutex));
        rv = _VPLFS__GetLocalAppDataWPath(&s_wLocalAppPath);
        if (rv != VPL_OK)
            goto end;
    }

    if (wcsnicmp(s_wLocalAppPath, epath, wcslen(s_wLocalAppPath)) == 0)
        rv = VPL_OK;
    else
        rv = VPL_ERR_NOENT;
    
end:
    if (wpath != NULL)
        free(wpath);

    if (epath != NULL)
        free(epath);

    return rv;
}

int _VPLFS_IsDirInLocalStatePath(VPLFS_dir_t* dir)
{
    int rv = VPL_OK;
    wchar_t *epath = NULL;

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if (dir->handle_key == VPLFS_INVALID_HANDLE) {
        rv = VPL_ERR_BADF;
        goto end;
    }

    {
        vplFolderChainer^ folder = _get_folderhandle(dir->handle_key);
        if (folder == nullptr) {
            rv = VPL_OK;
            goto end;
        }

        rv = _VPLFS__ConvertPath(folder->folder->Path->Data(), &epath, 0);
        if (rv != VPL_OK) {
            rv = false;
            goto end;
        }

        if (s_wLocalAppPath == NULL) {
            // Get LocalAppData path to s_wLocalAppPath for reuse
            MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localpath_mutex));
            rv = _VPLFS__GetLocalAppDataWPath(&s_wLocalAppPath);
            if (rv != VPL_OK)
                goto end;
        }

        if (wcsnicmp(s_wLocalAppPath, epath, wcslen(s_wLocalAppPath)) == 0)
            rv = VPL_OK;
        else
            rv = VPL_ERR_NOENT;
    }

end:
    if (epath != NULL)
        free(epath);

    return rv;
}
