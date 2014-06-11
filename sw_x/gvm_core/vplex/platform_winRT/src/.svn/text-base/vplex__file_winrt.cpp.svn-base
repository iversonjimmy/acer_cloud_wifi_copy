// defines and includes for generic function names to map to unicode versions
// Using Windows.Storage namespace
// http://msdn.microsoft.com/en-us/library/windows/apps/windows.storage.aspx
#ifndef UNICODE
# define UNICODE
#endif
#ifndef _UNICODE
# define _UNICODE
#endif

#include <wchar.h>

#include "vplex_file.h"
#include "vplex__file_priv.h"
#include "vpl_fs.h"
#include "vpl__fs_priv.h"
#include "vpl_time.h"
#include "vplex_private.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"
#include "log.h"
#include <windows.h>
#include <wincodec.h>
#include "scopeguard.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <Shcore.h>
#pragma comment(lib, "Shcore")

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Storage::FileProperties;
using namespace Windows::Foundation;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core ;

#pragma comment(lib, "windowscodecs")

static VPLLazyInitMutex_t s_mutex = VPLLAZYINITMUTEX_INIT;
static std::map<VPLFile_handle_t, vplFileChainer^> m_FileHandlePool;

static VPLFile_handle_t _push_filehandle(vplFileChainer^ file)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    int h = file->GetHashCode() % MAXINT;
    if (h < 0)
        h = 0;
    if (m_FileHandlePool.size() > 0) {
        while (m_FileHandlePool.count(h) > 0) {
            h = (h+1) % MAXINT;
        }
    }

    m_FileHandlePool.insert(std::pair<VPLFile_handle_t, vplFileChainer^>(h, file));
    return (VPLFile_handle_t)h;
}

static vplFileChainer^ _get_filehandle(VPLFile_handle_t handle)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    vplFileChainer^ h = nullptr;

    if (m_FileHandlePool.size() > 0 && m_FileHandlePool.count(handle) > 0)
        h = m_FileHandlePool[handle];

    return h;
}

static void _reset_filehandle(vplFileChainer^ file)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    file->file = nullptr;
    file->curpos = 0;
}

static int _remove_filehandle(VPLFile_handle_t handle)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    int rv = VPL_OK;
    if (m_FileHandlePool.size() > 0 && m_FileHandlePool.count(handle) > 0) {
        _reset_filehandle(m_FileHandlePool[handle]);
        m_FileHandlePool.erase(handle);
    }
    else {
        rv = VPL_ERR_NOENT;
    }
    return rv;
}

static int _replace_filehandle(vplFileChainer^ file, VPLFile_handle_t h)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    int rv = VPL_OK;
    rv = _remove_filehandle(h);
    if (rv != VPL_OK && rv != VPL_ERR_NOENT)
        return rv;

    m_FileHandlePool.insert(std::pair<VPLFile_handle_t, vplFileChainer^>(h, file));
    return VPL_OK;
}

static void _clear_filehandlepool()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (m_FileHandlePool.size() > 0) {        
        m_FileHandlePool.clear();
    }
}

static int _exist_folder(const wchar_t* wpathname, vplFolderChainer^ folder)
{
    int rv = VPL_OK;
    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( completedEvent ) {
        IAsyncOperation<StorageFolder^> ^openFolderAction = StorageFolder::GetFolderFromPathAsync(ref new String(wpathname));

        openFolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
            [&completedEvent, &rv, folder]
            (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                try {
                    auto openedfolder = op->GetResults();
                    if (folder != nullptr)
                        folder->folder = openedfolder;
                    rv = VPL_OK;
                }
                catch (Exception^ ex) {
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

static int _create_folder(const wchar_t* wpathname, vplFolderChainer^ folder, CreationCollisionOption option)
{
    int rv = VPL_OK;
    wchar_t *wParentPath=NULL, *wFolderName=NULL;

    {
        // In WinRT, folder creation can only be achieved by having its parent's StorageFolder object
        // Retrieve parent folder for file creation
        rv = _crack_path(wpathname, &wParentPath, &wFolderName);
        if (rv == VPL_OK) {
            // open parent folder to create target folder
            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if( completedEvent ) {
                IAsyncOperation<StorageFolder^> ^getPFolderAction = StorageFolder::GetFolderFromPathAsync(ref new String(wParentPath));

                getPFolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
                    [&completedEvent, &rv, wpathname, wFolderName, folder, option]
                    (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                        try {
                            auto openedfolder = op->GetResults();

                            HANDLE createfolder_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

                            auto createdFileOp = openedfolder->CreateFolderAsync(ref new String(wFolderName), option);
                            createdFileOp->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
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

static int _open_folder(const wchar_t* wfolderpath, vplFolderChainer^ folder, int creationOption)
{
    int rv = VPL_OK;    
    rv = _exist_folder(wfolderpath, folder);
    if (rv != VPL_OK)
        if (creationOption & VPLFILE_OPENFLAG_CREATE)
            rv = _create_folder(wfolderpath, folder, CreationCollisionOption::ReplaceExisting);

    return rv;
}

static int _exist_file(const wchar_t* wpathname, vplFileChainer^ file)
{
    int rv = false;
    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if ( completedEvent ) {
        IAsyncOperation<StorageFile^> ^openFileAction = StorageFile::GetFileFromPathAsync(ref new String(wpathname));

        openFileAction->Completed = ref new AsyncOperationCompletedHandler<StorageFile^>(
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

static int _create_file(const wchar_t* wpathname, vplFileChainer^ file, CreationCollisionOption option)
{
    int rv = VPL_OK;
    // In WinRT, file creation can only be achieved by having its parent's StorageFolder object
    // Retrieve parent folder for file creation
    wchar_t *wParentPath=NULL, *wFileName=NULL;

    if (file == nullptr) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    rv = _crack_path(wpathname, &wParentPath, &wFileName);
    if (rv == VPL_OK) {
        // open parent folder to create file
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if( completedEvent ) {
            IAsyncOperation<StorageFolder^> ^openFolderAction = StorageFolder::GetFolderFromPathAsync(ref new String(wParentPath));

            openFolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
                [&completedEvent, &rv, wpathname, wFileName, file, option]
                (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                try {
                    auto folder = op->GetResults();
                    HANDLE createfile_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                    try {
                        folder->CreateFileAsync(ref new String(wFileName), option)->Completed = ref new AsyncOperationCompletedHandler<StorageFile^> (
                            [&createfile_event, &rv, file]
                            (IAsyncOperation<StorageFile^>^ op, AsyncStatus status) {
                                try {
                                    file->file = op->GetResults();
                                    rv = VPL_OK;
                                }
                                catch (Exception^ ex) {
                                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                                }
                                SetEvent(createfile_event);
                        });
                        WaitForSingleObjectEx(createfile_event, INFINITE, TRUE);
                        CloseHandle(createfile_event);
                    }
                    catch (Exception^ ex) {
                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        LOG_TRACE("vplex file io exception: %d", rv);
                    }
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
            rv = VPL_ERR_FAIL;
        }
    }

end:
    if (wParentPath != NULL)
        free(wParentPath);
    if (wFileName != NULL)
        free(wFileName);

    return rv;
}

static VPLFile_handle_t _open_file(const wchar_t* wpathname, DWORD createDisposition)
{
    VPLFile_handle_t h;
    vplFileChainer^ file = ref new vplFileChainer();
    int rv = VPL_OK;

    rv = _exist_file(wpathname, file);
    if (createDisposition == CREATE_ALWAYS) {
        // If the specified file exists and is writable, the function overwrites the file, the function succeeds
        if (rv == VPL_OK) {
            // file exists
            _reset_filehandle(file);
            rv = _create_file(wpathname, file, CreationCollisionOption::ReplaceExisting);
        }
        else {
            // file doesn't exist
            rv = _create_file(wpathname, file, CreationCollisionOption::OpenIfExists);
        }
        if (rv == VPL_OK)
            h = _push_filehandle(file);
    }
    else if (createDisposition == CREATE_NEW) {
        // If the specified file exists, the function fails 
        if (rv == VPL_OK) {
            // file exists
            delete file;
            rv = VPL_ERR_EXIST;
        }
        else {
            // file doesn't exist
            rv = _create_file(wpathname, file, CreationCollisionOption::ReplaceExisting);
            if (rv == VPL_OK)
                h = _push_filehandle(file);
        }
    }
    else if (createDisposition == OPEN_ALWAYS) {
        // If the specified file exists, the function succeeds
        if (rv != VPL_OK) {
            // file doesn't exist
            rv = _create_file(wpathname, file, CreationCollisionOption::ReplaceExisting);
        }
        if (rv == VPL_OK)
            h = _push_filehandle(file);
    }
    else if (createDisposition == OPEN_EXISTING) {
        // Opens a file or device, only if it exists.
        if (rv == VPL_OK) {
            // file exists
            h = _push_filehandle(file);
        }
        else {
            // file doesn't exist
            delete file;
            rv = VPL_ERR_NOT_EXIST;
        }
    }
    else if (createDisposition == TRUNCATE_EXISTING) {
        // Opens a file and truncates it so that its size is zero bytes, only if it exists.
        if (rv == VPL_OK) {
            // file exists
            rv = _create_file(wpathname, file, CreationCollisionOption::ReplaceExisting);
            if (rv == VPL_OK)
                h = _push_filehandle(file);
        }
        else {
            // file doesn't exist
            delete file;
            rv = VPL_ERR_NOT_EXIST;
        }
    }

    if (rv != VPL_OK)
        h = rv;

    return h;
}

static ssize_t _write_file(vplFileChainer^ file, const void *buffer, size_t bufsize, bool bAppend, VPLFile_offset_t offset)
{
    HANDLE filelock = NULL;    
    _fileio_lock(file->file->Path->Data(), &filelock, IOLOCK_TYPE::IOLOCK_WRITE);
    ON_BLOCK_EXIT(_fileio_unlock, filelock);

    int rv = VPL_OK;
    ssize_t nbytes = 0;

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        goto end;
    }
    try {
        // Get random access stream
        auto randomStreamTask = file->file->OpenTransactedWriteAsync();
        randomStreamTask->Completed = ref new AsyncOperationCompletedHandler<StorageStreamTransaction^> (
            [&completedEvent, &rv, file, buffer, bufsize, &nbytes, bAppend, offset]
            (IAsyncOperation<StorageStreamTransaction^>^ op, AsyncStatus status) {
                try{
                    auto streamTransacted = op->GetResults();
                    // read from stream
                    HANDLE write_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                    if( write_event ) {
                        // open output stream to write
                        // if bAppend is true, ignore offset argument and append buffer to file
                        // else, open output stream to offset
                        if (bAppend) {
                            file->curpos = streamTransacted->Stream->Size;
                        }
                        else
                            file->curpos = offset;
                        Streams::IOutputStream^ outputStream= streamTransacted->Stream->GetOutputStreamAt(file->curpos);
                        Streams::DataWriter^ dataWriter  = ref new Streams::DataWriter(outputStream);

                        // convert buffer to Platform::Array to write out
                        auto platformBuffer = ref new Platform::Array<BYTE>((BYTE*)buffer, bufsize);

                        // write from buffer
                        dataWriter->WriteBytes(platformBuffer);
                        try {
                            auto writeOp = dataWriter->StoreAsync();
                            writeOp->Completed = ref new AsyncOperationCompletedHandler<unsigned int> (
                                [&rv, &write_event, streamTransacted, file, &nbytes]
                                (IAsyncOperation<unsigned int>^ op, AsyncStatus status) {
                                try {
                                    unsigned int byteWritten = op->GetResults();

                                    // return output size
                                    nbytes = byteWritten;

                                    // flush to file
                                    HANDLE flush_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                                    if( flush_event ) {
                                        try {
                                            auto flushOp = streamTransacted->CommitAsync();
                                            flushOp->Completed = ref new AsyncActionCompletedHandler (
                                                [&rv, file, nbytes, &flush_event]
                                                (IAsyncAction^ op, AsyncStatus status) {
                                                    try {
                                                        op->GetResults();
                                                        // update file pos
                                                        file->curpos += nbytes;
                                                        rv = VPL_OK;
                                                    }
                                                    catch(Exception^ ex) {
                                                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                                                    }
                                                    SetEvent(flush_event);
                                            });
                                            WaitForSingleObjectEx(flush_event, INFINITE, TRUE);
                                            CloseHandle(flush_event);
                                        }
                                        catch (Exception^ ex) {
                                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                                        }
                                    }
                                    else {
                                        rv = VPL_ERR_FAIL;
                                    }
                                }
                                catch(Exception^ ex) {
                                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                                }
                                SetEvent(write_event);
                            });
                            WaitForSingleObjectEx(write_event, INFINITE, TRUE);
                            CloseHandle(write_event);
                        }
                        catch (Exception^ ex) {
                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        }
                        // remember to detach DataWriter
                        dataWriter->DetachStream();
                    }
                    else 
                        rv = VPL_ERR_FAIL;                
                }
                catch (Exception^ ex) {
                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                }
                SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
    }
    catch (Exception^ ex) {
        rv = _VPLFS_Error_XlatErrno(ex->HResult);
        LOG_TRACE("vplex file io exception: %d", rv);
    }
    CloseHandle(completedEvent);

end:
    if (rv != VPL_OK)
        nbytes = rv;
    return nbytes;
}

static ssize_t _read_file(vplFileChainer^ file, void *buffer, size_t bufsize, VPLFile_offset_t offset)
{
    int rv = VPL_OK;
    ssize_t nbytes = 0;

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        goto end;
    }
    try {
        // Get random access stream
        auto randomStreamTask = file->file->OpenAsync(FileAccessMode::Read);
        randomStreamTask->Completed = ref new AsyncOperationCompletedHandler<IRandomAccessStream^> (
            [&completedEvent, &rv, file, &nbytes, &buffer, bufsize, offset]
            (IAsyncOperation<IRandomAccessStream^>^ op, AsyncStatus status) {
            try{
                auto stream = op->GetResults();
                if (buffer == NULL) {
                    // return left file size if buffer is NULL
                    nbytes = stream->Size;
                }
                else {
                    // read from stream
                    HANDLE read_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                    if( read_event ) {
                        // open input stream
                        Streams::IInputStream^ inputStream= stream->GetInputStreamAt(offset);
                        Streams::DataReader^ dataReader  = ref new Streams::DataReader(inputStream);

                        // read to buffer
                        dataReader->InputStreamOptions = InputStreamOptions::Partial;
                        
                        // check left file size and buffer size, and load the min one
                        unsigned int loadSize;
                        if (stream->Size > bufsize)
                            loadSize = bufsize;
                        else
                            loadSize = stream->Size;
                        
                        try {
                            dataReader->LoadAsync(loadSize)->Completed = ref new AsyncOperationCompletedHandler<unsigned int> (
                                [&rv, &read_event, dataReader, file, &nbytes, &buffer, offset]
                                (IAsyncOperation<unsigned int>^ op, AsyncStatus status) {
                                try {
                                    unsigned int byteAvailable = op->GetResults();

                                    // create Platform::Array to read
                                    auto platformBuffer = ref new Platform::Array<BYTE>(byteAvailable);
                                    dataReader->ReadBytes(platformBuffer);
                                    memcpy(buffer, platformBuffer->Data, byteAvailable);
                                    nbytes = byteAvailable;

                                    // update file pos
                                    file->curpos = offset + nbytes;
                                    rv = VPL_OK;
                                }
                                catch(Exception^ ex) {
                                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                                }
                                SetEvent(read_event);
                            });
                            WaitForSingleObjectEx(read_event ,INFINITE, TRUE);
                            CloseHandle(read_event);
                        }
                        catch(Exception^ ex) {
                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        }
                        // remember to detach DataReader
                        dataReader->DetachStream();
                    }
                    else 
                        rv = VPL_ERR_FAIL;
                }
            }
            catch (Exception^ ex) {
                rv = _VPLFS_Error_XlatErrno(ex->HResult);
            }
            SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
    }
    catch (Exception^ ex) {
        rv = _VPLFS_Error_XlatErrno(ex->HResult);
        LOG_TRACE("vplex file io exception: %d", rv);
    }
    CloseHandle(completedEvent);

end:
    if (rv != VPL_OK)
        nbytes = rv;
    return nbytes;
}

int 
_VPLFile_CheckAccess(const char *pathname, int mode)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;
    
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
        vplFileChainer^ file = ref new vplFileChainer();

        if (mode == VPLFILE_CHECKACCESS_EXISTS) {
            rv = _exist_file(epathname, file);
        }
        else {
            FileAccessMode fMode = FileAccessMode::Read;
            if (mode == VPLFILE_CHECKACCESS_READ) {
                fMode = FileAccessMode::Read;
            }
            else if (mode == VPLFILE_CHECKACCESS_WRITE) {
                fMode = FileAccessMode::ReadWrite;
            }

            if (mode == VPLFILE_CHECKACCESS_READ || mode == VPLFILE_CHECKACCESS_WRITE) {
                rv = _vpl_openfile(epathname,file);
                if(rv != VPL_OK) 
                    goto end;

                HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                if ( completedEvent ) {
                    try {
                        auto randomStreamTask = file->file->OpenAsync(fMode);
                        randomStreamTask->Completed = ref new AsyncOperationCompletedHandler<IRandomAccessStream^>(
                            [&rv, &completedEvent] (IAsyncOperation<IRandomAccessStream^>^ op, AsyncStatus status) {
                            try {
                                auto randomstream = op->GetResults();
                                rv = VPL_OK;
                            }
                            catch (Exception^ ex) {
                                rv = _VPLFS_Error_XlatErrno(ex->HResult);
                            }
                            SetEvent(completedEvent);
                        });
                        WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
                    }
                    catch(Exception^ ex) {
                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        LOG_TRACE("vplex file io exception: %d", rv);
                    }
                    CloseHandle(completedEvent);
                }
                else
                    rv = VPL_ERR_FAIL;
            }
        }
        
        if (rv == VPL_ERR_NOENT) {
            rv = _exist_folder(epathname, nullptr);
        }
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
_VPLFile_Open(const char *pathname, int flags, int mode)
{
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;
    VPLFile_handle_t h;
    DWORD desiredAccess;
    DWORD shareMode;
    DWORD createDisposition;
    HANDLE h32;

    rc = _VPL__utf8_to_wstring(pathname, &wpathname);
    if (rc != VPL_OK) {
        h = rc;
        goto end;
    }

    rc = _VPLFS__ConvertPath(wpathname, &epathname, 0);
    if (rc != VPL_OK) {
        h = rc;
        goto end;
    }

    desiredAccess = GENERIC_READ;  // always grant read access
    if (flags & (VPLFILE__OPENFLAG_WRITEONLY | VPLFILE__OPENFLAG_READWRITE)) {
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

    h = _open_file(epathname, createDisposition);

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
_VPLFile_IsValidHandle(VPLFile_handle_t h)
{
    return h >= 0;
}

ssize_t 
_VPLFile_Write(VPLFile_handle_t h, const void *buffer, size_t bufsize)
{
    ssize_t nbytes = 0;
    auto file = _get_filehandle(h);

    if (file == nullptr) {
        nbytes = VPL_ERR_INVALID;
        goto end;
    }
    nbytes = _write_file(file, buffer, bufsize, true, 0);

end:
    return nbytes;
}

ssize_t 
_VPLFile_Read(VPLFile_handle_t h, void *buffer, size_t bufsize)
{
    ssize_t nbytes = 0;
    auto file = _get_filehandle(h);

    if (file == nullptr) {
        nbytes = VPL_ERR_INVALID;
        goto end;
    }
    nbytes = _read_file(file, buffer, bufsize, file->curpos);

end:
    return nbytes;
}

ssize_t 
_VPLFile_WriteAt(VPLFile_handle_t h, const void *buffer, size_t bufsize, VPLFile_offset_t offset)
{
    ssize_t nbytes = 0;
    auto file = _get_filehandle(h);

    if (file == nullptr) {
        nbytes = VPL_ERR_INVALID;
        goto end;
    }
    nbytes = _write_file(file, buffer, bufsize, false, offset);

end:
    return nbytes;
}

ssize_t 
_VPLFile_ReadAt(VPLFile_handle_t h, void *buffer, size_t bufsize, VPLFile_offset_t offset)
{
    ssize_t nbytes = 0;
    auto file = _get_filehandle(h);

    if (file == nullptr) {
        nbytes = VPL_ERR_INVALID;
        goto end;
    }
    nbytes = _read_file(file, buffer, bufsize, offset);
    
 end:
    return nbytes;
}

VPLFile_handle_t 
_VPLFile_CreateTemp(char* filename_in_out, size_t bufSize)
{
    int rv = VPL_OK;
    VPLFile_handle_t h = VPLFILE_INVALID_HANDLE;
    wchar_t *wfilename_in_out=NULL, *efilename_in_out=NULL;
    char *efilename=NULL;

    size_t actualLen = strnlen(filename_in_out, bufSize);
    if (actualLen == bufSize) {
        VPL_LIB_LOG_WARN(VPL_SG_FS, "No null-terminator found");
        goto end;
    }

    rv = _VPL__utf8_to_wstring(filename_in_out, &wfilename_in_out);
    if (rv != VPL_OK) {
        goto end;
    }

    rv = _VPLFS__ConvertPath(wfilename_in_out, &efilename_in_out, 0);
    if (rv != VPL_OK) {
        goto end;
    }

    // mktemp on Windows apparently uses the thread id in the temp filename,
    // so this should be safe enough.
    
    if (_wmktemp(efilename_in_out) == NULL) {
        VPL_LIB_LOG_WARN(VPL_SG_FS, "mktemp returned NULL");
        goto end;
    }

    _VPL__wstring_to_utf8_alloc(efilename_in_out, &efilename);

    h = VPLFile_Open(efilename, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_EXCLUSIVE, VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);

end:
    if (wfilename_in_out != NULL)
        free(wfilename_in_out);

    if (efilename_in_out != NULL)
        free(efilename_in_out);

    if (efilename != NULL)
        free(efilename);

    return h;
}

int 
_VPLFile_TruncateAt(VPLFile_handle_t h, VPLFile_offset_t length)
{
    int rv = VPL_OK;
    HANDLE h2 = INVALID_HANDLE_VALUE;
    VPLFile_handle_t hTemp;
    char *filepath=NULL, *tempfilepath=NULL;
    wchar_t *wfilepath=NULL;
    size_t szTempfilepath = 0;

    auto file = _get_filehandle(h);
    if (file == nullptr) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    {
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if ( completedEvent ) {
            try {
                auto randomStreamTask = file->file->OpenAsync(FileAccessMode::ReadWrite);
                randomStreamTask->Completed = ref new AsyncOperationCompletedHandler<IRandomAccessStream^>(
                    [&rv, &completedEvent, length] (IAsyncOperation<IRandomAccessStream^>^ op, AsyncStatus status) {
                    try {
                        auto randomstream = op->GetResults();
                        
                        // Truncate at specific length
                        randomstream->Size = length;

                        // flush to file
                        HANDLE flush_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
                        if( flush_event ) {
                            try {
                                auto flushOp = randomstream->FlushAsync();
                                flushOp->Completed = ref new AsyncOperationCompletedHandler<bool> (
                                    [&rv, &flush_event]
                                    (IAsyncOperation<bool>^ op, AsyncStatus status) {
                                        try {
                                            op->GetResults();
                                            rv = VPL_OK;
                                        }
                                        catch(Exception^ ex) {
                                            rv = _VPLFS_Error_XlatErrno(ex->HResult);
                                        }
                                        SetEvent(flush_event);
                                });
                                WaitForSingleObjectEx(flush_event, INFINITE, TRUE);
                                CloseHandle(flush_event);
                            }
                            catch (Exception^ ex) {
                                rv = _VPLFS_Error_XlatErrno(ex->HResult);
                            }
                        }
                        else {
                            rv = VPL_ERR_FAIL;
                        }
                    }
                    catch (Exception^ ex) {
                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                    }
                    SetEvent(completedEvent);
                });
                WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
            }
            catch(Exception^ ex) {
                rv = _VPLFS_Error_XlatErrno(ex->HResult);
                LOG_TRACE("vplex file io exception: %d", rv);
            }
            CloseHandle(completedEvent);
        }
        else
            rv = VPL_ERR_FAIL;
    }
end:
    VPLFile_Seek(h, 0, VPLFILE_SEEK_SET);

    return rv;
}

int 
_VPLFile_Sync(VPLFile_handle_t h)
{
    int rv = VPL_OK;

    auto file = _get_filehandle(h);
    if (file == nullptr)
        rv = VPL_ERR_INVALID;

 end:
    return rv;
}

VPLFile_offset_t
_VPLFile_Seek(VPLFile_handle_t h, VPLFile_offset_t offset, int whence)
{
    VPLFile_offset_t rv = -1;

    auto file = _get_filehandle(h);
    if (file == nullptr) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    {
        // TODO: open file and get random access stream at specific offset to see if there is any error
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if ( completedEvent ) {
            try {
                auto randomStreamTask = file->file->OpenAsync(FileAccessMode::Read);
                randomStreamTask->Completed = ref new AsyncOperationCompletedHandler<IRandomAccessStream^>(
                    [&rv, &completedEvent, file, whence, offset] (IAsyncOperation<IRandomAccessStream^>^ op, AsyncStatus status) {
                    try {
                        auto randomstream = op->GetResults();

                        if (whence == VPLFILE_SEEK_END)
                            // If whence is VPLFILE_SEEK_END, ignore offset
                            rv = randomstream->Size;
                        else {
                            // get input stream at offset
                            if (whence == VPLFILE_SEEK_SET) {
                                rv = 0 + offset;
                            }
                            else if (whence == VPLFILE_SEEK_CUR) {
                                rv = file->curpos + offset;
                            }
                            if (rv > randomstream->Size)
                                rv = randomstream->Size;
                            Streams::IInputStream^ inputStream= randomstream->GetInputStreamAt(rv);

                            file->curpos = rv;
                        }
                    }
                    catch (Exception^ ex) {
                        rv = _VPLFS_Error_XlatErrno(ex->HResult);
                        // Reset curpos to begining of the file
                        file->curpos = 0;
                    }
                    SetEvent(completedEvent);
                });
                WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
            }
            catch(Exception^ ex) {
                rv = _VPLFS_Error_XlatErrno(ex->HResult);
                LOG_TRACE("vplex file io exception: %d", rv);
            }
            CloseHandle(completedEvent);
        }
        else
            rv = VPL_ERR_FAIL;
    }

end:
    return rv;
}

int 
_VPLFile_Close(VPLFile_handle_t h)
{
    int rv = _remove_filehandle(h);

    return rv;
}

int 
_VPLFile_Delete(const char *pathname)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;

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
        vplFileChainer^ file = ref new vplFileChainer();
        rv = _exist_file(epathname, file);
        if (rv == VPL_OK) {
            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if( !completedEvent ) {
                rv = VPL_ERR_FAIL;
            }
            else {
                auto deleteAction = file->file->DeleteAsync(StorageDeleteOption::Default);
                deleteAction->Completed = ref new AsyncActionCompletedHandler(
                    [&rv, &completedEvent] (IAsyncAction^ op, AsyncStatus status) {
                    try {
                        op->GetResults();
                        rv = VPL_OK;
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
        delete file;
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
_VPLFile_SetTime(const char *pathname, VPLTime_t time)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    ULARGE_INTEGER t;
    FILETIME ft;
    HANDLE completedEvent = INVALID_HANDLE_VALUE;
    int rc;

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

    rv = _VPLFile_CheckAccess(pathname, VPLFILE_CHECKACCESS_EXISTS);
    if (rv != VPL_OK)
        goto end;
 
    // TODO: http://msdn.microsoft.com/en-us/library/windows/apps/br212125.aspx
    rv = VPL_ERR_NOOP;
    goto end;

end:
    if (wpathname != NULL) {
        free(wpathname);
    }
    if (epathname != NULL) {
        free(epathname);
    }
    if (completedEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(completedEvent);
    }
    return rv;
}

int 
_VPLFile_Rename(const char *oldpath, const char *newpath)
{
    int rv;
    // For winrt, there is no difference between rename and move
    rv = _VPLFile_Move (oldpath, newpath);

    return rv;
}

int 
_VPLFile_Move(const char *oldpath, const char *newpath)
{
    int rv = VPL_OK;
    wchar_t *woldpath = NULL, *wnewpath = NULL;
    wchar_t *eoldpath = NULL, *enewpath = NULL;
    wchar_t *woldparent = NULL, *woldfilename = NULL;
    wchar_t *wnewparent = NULL, *wnewfilename = NULL;

    int rc;

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

    rc = _VPLFS__ConvertPath(woldpath, &eoldpath, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    rc = _VPLFS__ConvertPath(wnewpath, &enewpath, 0);
    if (rc != VPL_OK) {
      rv = rc;
      goto end;
    }

    {
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if( !completedEvent ) {
            rv = VPL_ERR_FAIL;
            goto end;
        }
        rv = _crack_path(eoldpath, &woldparent, &woldfilename);
        if (rv != VPL_OK)
            goto end;
        rv = _crack_path(enewpath, &wnewparent, &wnewfilename);
        if (rv != VPL_OK)
            goto end;

        vplFileChainer^ file = ref new vplFileChainer();
        rv = _exist_file(eoldpath, file);
        if (rv == VPL_OK) {
            if (wcsicmp(woldparent, wnewparent) == 0) {
                // the same parent folder, just rename
                auto renameAction = file->file->RenameAsync(ref new String(wnewfilename), NameCollisionOption::ReplaceExisting);
                renameAction->Completed = ref new AsyncActionCompletedHandler(
                    [&rv, &completedEvent] (IAsyncAction^ op, AsyncStatus status) {
                    try {
                        op->GetResults();
                        rv = VPL_OK;
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
                // different parent folder
                // open new folder path, and then move from old to new path
                vplFolderChainer^ folder = ref new vplFolderChainer();
                rv = _open_folder(wnewparent, folder, VPLFILE_OPENFLAG_CREATE);
                if (rv == VPL_OK) {
                    auto moveAction = file->file->MoveAsync(folder->folder, ref new String(wnewfilename), NameCollisionOption::ReplaceExisting);
                    moveAction->Completed = ref new AsyncActionCompletedHandler(
                        [&rv, &completedEvent] (IAsyncAction^ op, AsyncStatus status) {
                        try {
                            op->GetResults();
                            rv = VPL_OK;
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
        }
        else {
            vplFolderChainer^ folder = ref new vplFolderChainer();
            rv = _exist_folder(eoldpath, folder);
            
            if (rv == VPL_OK) {
                if (wcsicmp(woldparent, wnewparent) == 0) {
                    // the same parent folder, just rename
                    auto renameAction = folder->folder->RenameAsync(ref new String(wnewfilename), NameCollisionOption::ReplaceExisting);
                    renameAction->Completed = ref new AsyncActionCompletedHandler(
                        [&rv, &completedEvent] (IAsyncAction^ op, AsyncStatus status) {
                        try {
                            op->GetResults();
                            rv = VPL_OK;
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
                    // different parent folder
                    // open new folder path, and then move from old to new path
                    rv = _open_folder(enewpath, nullptr, VPLFILE_OPENFLAG_CREATE);
                    if (rv == VPL_OK) {
                        // StorageFolder doesn't have MoveAsync function to move folder recursively
                        // recursive move file and dir to newpath
                        VPLFS_dir_t directory;
                        VPLFS_dirent_t entry;
                        char filename[MAX_PATH]={0};
                        rv = VPLFS_Opendir(oldpath, &directory);
                        if (rv == VPL_OK) {
                            while ((rv = VPLFS_Readdir(&directory, &entry)) == VPL_OK) {
                                char src[MAX_PATH]={0}, dst[MAX_PATH]={0};
                                snprintf(src, MAX_PATH, "%s\\%s", oldpath, entry.filename);
                                snprintf(dst, MAX_PATH, "%s\\%s", newpath, entry.filename);
                                if (entry.type == VPLFS_TYPE_FILE) {
                                    // Move to new path
                                    rv = VPLFile_Move(src, dst);
                                }
                                else if (entry.type == VPLFS_TYPE_DIR) {
                                    // 1. Create dst path
                                    rv = VPLDir_Create(dst, 0);
                                    if (rv != VPL_OK) {
                                        LOG_TRACE("VPLFS_Move(%s, %s) returned %d", src, dst, rv);
                                        break;
                                    }
                                    // 2. Recursive Move dir
                                    rv = VPLFile_Move(src, dst);
                                    if (rv != VPL_OK) {
                                        LOG_TRACE("VPLFS_Move(%s, %s) returned %d", src, dst, rv);
                                        break;
                                    }
                                }
                            }
                            if (rv == VPL_ERR_MAX) {
                                // No more directories
                                rv = VPL_OK;
                            }

                            VPLFS_Closedir(&directory);
                            // 3. Delete src path
                            rv = VPLDir_Delete(oldpath);
                            if (rv != VPL_OK) {
                                LOG_TRACE("VPLDir_Delete(%s) returned %d", oldpath, rv);
                            }
                        }
                    }
                }
            }
        }
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
    if (woldparent != NULL) {
        free(woldparent);
    }
    if (woldfilename != NULL) {
        free(woldfilename);
    }
    if (wnewparent != NULL) {
        free(wnewparent);
    }
    if (wnewfilename != NULL) {
        free(wnewfilename);
    }

    return rv;
}

int
_VPLDir_Create(const char *pathname, int mode)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    int rc;

    UNUSED(mode);

    // if the request is something like "C:", just return success
    if (strlen(pathname) == 2 && isalpha(pathname[0]) && pathname[1] == ':')
        return VPL_OK;

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

    rv = _open_folder(epathname, nullptr, VPLFILE_OPENFLAG_CREATE);

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
_VPLDir_Delete(const char *pathname)
{
    return VPLFS_Rmdir(pathname);
}

int
_VPLFile_IsFileInLocalStatePath (VPLFile_handle_t h)
{
    int rv = VPL_OK;
    wchar_t *epath = NULL, *wlocalpath = NULL;

    if(!VPLFile_IsValidHandle(h)) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    {
        vplFileChainer^ file = _get_filehandle(h);
        if (file == nullptr) {
            rv = VPL_OK;
            goto end;
        }

        rv = _VPLFS__ConvertPath(file->file->Path->Data(), &epath, 0);
        if (rv != VPL_OK) {
            goto end;
        }

        rv = _VPLFS__GetLocalAppDataWPath(&wlocalpath);
        if (rv != VPL_OK)
            goto end;

        if (wcsnicmp(wlocalpath, epath, wcslen(wlocalpath)) == 0)
            rv = VPL_OK;
        else
            rv = VPL_ERR_NOENT;
    }

end:
    if (wlocalpath != NULL)
        free(wlocalpath);

    if (epath != NULL)
        free(epath);

    return rv;
    
}

int _VPLFS_FStat(VPLFile_handle_t fd, VPLFS_stat_t* buf)
{
    int rv = VPL_OK;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;

    auto file = _get_filehandle(fd);
    
    if (file == nullptr) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    
    {
        size_t pathlen = wcslen(file->file->Path->Data())+1;
        wpathname = (wchar_t*) malloc(pathlen * sizeof(wchar_t));
    
        swprintf_s(wpathname,pathlen,L"%s",file->file->Path->Data());
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
    }

    {
        HANDLE stat_mutex;
        _fileio_lock(epathname,&stat_mutex, IOLOCK_TYPE::IOLOCK_STAT);
        ON_BLOCK_EXIT(_fileio_unlock, stat_mutex);

        // WinRT limitation, cannot tell hidden and symbolic attributes
        buf->isHidden = VPL_FALSE;
        buf->isSymLink = VPL_FALSE;

        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if ( !completedEvent ) {
            rv = VPL_ERR_FAIL;
            goto end;
        }

        // collect its state
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
       
        buf->vpl_atime = VPLTime_FromSec(buf->atime);
        buf->vpl_mtime = VPLTime_FromSec(buf->mtime);
        buf->vpl_ctime = VPLTime_FromSec(buf->ctime);
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

VPL_BOOL _VPLFile_IsWinRTHandle(VPLFile_handle_t)
{
    return VPL_TRUE;
}

#define EXIF_DATETIME_FORMAT "%Y:%m:%d %H:%M:%S"

time_t String2time_t(char* time_string, char* time_format)
{
    std::tm t;
    std::istringstream ss(time_string);
    ss >> std::get_time(&t, time_format);
    if (ss.fail()) {
        LOG_WARN("[%s] Input DateTime string's format is incorrect!", time_string);
        return (time_t) 0;
    }
    return std::mktime(&t);
}

int _VPLFile__EXIFGetImageTimestamp(const char* filename, time_t* date_time)
{
    int rv = VPL_ERR_FAIL;
    *date_time = 0;
    // Parse image metadata by WIC (Windows Imaging Component) COM APIs .
    // See MSDN: http://msdn.microsoft.com/en-us/library/windows/desktop/ee719653(v=vs.85).aspx#readingmetadata
    IWICImagingFactory *pFactory = NULL;
    IWICBitmapDecoder *pDecoder = NULL;
    IWICBitmapFrameDecode *pFrameDecode = NULL;
    IWICMetadataQueryReader *pQueryReader = NULL;
    PROPVARIANT value;
    wchar_t *wpathname = NULL;
    wchar_t *epathname = NULL;
    vplFileChainer^ file = ref new vplFileChainer();

    HRESULT hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if(FAILED(hr)) goto end;

    // Create the decoder
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        (LPVOID*)&pFactory);
    if(FAILED(hr)) goto end;
    
    rv = _VPL__utf8_to_wstring(filename, &wpathname);
    if (rv != VPL_OK) {
        goto end;
    }
    rv = _VPLFS__ConvertPath(wpathname, &epathname, 0);
    if (rv != VPL_OK) {
        goto end;
    }
    rv = _vpl_openfile(epathname,file);
    if(rv != VPL_OK) 
        goto end;
    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        goto end;
    }
    try {

        auto ImgStream = file->file->OpenAsync(FileAccessMode::Read);
        ImgStream->Completed = ref new AsyncOperationCompletedHandler<IRandomAccessStream^> (
            [&completedEvent,&date_time, &rv, &pFactory, &pDecoder, &pFrameDecode, &pQueryReader, &value, &hr]
            (IAsyncOperation<IRandomAccessStream^>^ op, AsyncStatus status) {

            IStream* pStream;
            hr = CreateStreamOverRandomAccessStream(reinterpret_cast<IUnknown*>(op->GetResults()), IID_PPV_ARGS(&pStream));

            if (FAILED(hr)){ 
                throw Platform::Exception::CreateException(hr); 
            }

            hr = pFactory->CreateDecoderFromStream(
                pStream,
                NULL,
                WICDecodeMetadataCacheOnDemand,
                &pDecoder);
            if(FAILED(hr)) goto com_end;
               
            // Get a single frame from the image
            hr = pDecoder->GetFrame(
                0,  //JPEG has only one frame.
                &pFrameDecode);
            if(FAILED(hr)) goto com_end;

            // Get the query reader
            hr = pFrameDecode->GetMetadataQueryReader(&pQueryReader);
            if(FAILED(hr)) goto com_end;
            PropVariantInit(&value);
            {
                //
                // Reading metadata path: http://msdn.microsoft.com/en-us/library/windows/desktop/ee719904(v=vs.85).aspx
                //
                // 1. Exif DateTimeDigitized tag
                // "/app1/ifd/exif/{ushort=36868}"
                if(SUCCEEDED(pQueryReader->GetMetadataByName(L"/app1/ifd/exif/{ushort=36868}", &value))) {
                    if (value.vt == VT_LPSTR) {
                        *date_time = String2time_t(value.pszVal, EXIF_DATETIME_FORMAT);
                        if(*date_time != 0)
                            goto valueObtained;
                    }
                }

                // 2. Exif DateTimeOriginal tag
                // "/app1/ifd/exif/{ushort=36867}"
                if(SUCCEEDED(pQueryReader->GetMetadataByName(L"/app1/ifd/exif/{ushort=36867}", &value))) {
                    if (value.vt == VT_LPSTR) {
                        *date_time = String2time_t(value.pszVal, EXIF_DATETIME_FORMAT);
                        if(*date_time != 0)
                            goto valueObtained;
                    }
                }

                // 3. Tiff DateTime tag
                // "/app1/ifd/{ushort=306}"
                if(SUCCEEDED(pQueryReader->GetMetadataByName(L"/app1/ifd/{ushort=306}", &value))) {
                    if (value.vt == VT_LPSTR) {
                        *date_time = String2time_t(value.pszVal, EXIF_DATETIME_FORMAT);
                        if(*date_time != 0)
                            goto valueObtained;
                    }
                }
            }
            LOG_INFO("No Value From EXIF");
valueObtained:
            PropVariantClear(&value);
            LOG_INFO("EXIF timestamp: %I64u", *date_time);
com_end:
            SetEvent(completedEvent);
        }); //OpenAsync Completed

        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);

    } catch (Exception^ ex) {
        hr = ex->HResult;
        rv = _VPLFS_Error_XlatErrno(ex->HResult);
        LOG_TRACE("vplex file io exception: %d", rv);
    }
    
    CloseHandle(completedEvent);

end:
    if (wpathname != NULL) free(wpathname);
    if (epathname != NULL) free(epathname);
    if(pQueryReader) pQueryReader->Release();
    if(pFrameDecode) pFrameDecode->Release();
    if(pDecoder) pDecoder->Release();
    if(pFactory) pFactory->Release();
    CoUninitialize();

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr
        || HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == hr)
        rv = VPL_ERR_NOENT;
    else if(FAILED(hr))
        rv = _VPLFS_Error_XlatErrno(hr);
    else if(*date_time == 0)
        rv = VPL_ERR_NOT_EXIST;
    else
        rv = VPL_OK;
    return rv;
}

