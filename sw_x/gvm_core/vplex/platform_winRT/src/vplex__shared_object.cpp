#include "vplex_plat.h"
#include "vplex_file.h"
#include "vpl_fs.h"
#include "vpl__fs_priv.h"
#include "vplex_shared_object.h"

#include <thread>

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

static char ActoolLoc[MAX_PATH+1] = {0};
static char CredLoc[MAX_PATH+1] = {0};

int VPLSharedObject_AddData(const char *shared_location, const char *object_id, const void *input_data, unsigned int data_length)
{
    int rv = VPL_OK;
    ssize_t bytesWritten = 0;
    char *obj_path=NULL;

    rv = VPLDir_Create(shared_location, 0777);
    if (rv != VPL_OK)
        goto end;

    {
        size_t szObjpath = strlen(shared_location) + strlen(object_id) + 2;
        obj_path = (char*)malloc(szObjpath);
        snprintf(obj_path, szObjpath, "%s\\%s", shared_location, object_id);

        VPLFile_handle_t h = VPLFile_Open(obj_path, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0);
        if ( !VPLFile_IsValidHandle(h) ) {
            rv = VPL_ERR_FAIL;
            goto end;
        }
        
        bytesWritten = VPLFile_Write(h, input_data, data_length);
        if (bytesWritten != data_length)
            rv = VPL_ERR_FAIL;

        rv = VPLFile_Close(h);
    }

end:
    if (obj_path != NULL)
        free(obj_path);

    return rv;
}

int VPLSharedObject_AddString(const char *shared_location, const char *object_id, const char *input_string)
{
    return VPLSharedObject_AddData(shared_location, object_id, input_string, strlen(input_string));
}

int VPLSharedObject_DeleteObject(const char *shared_location, const char *object_id)
{
    int rv = VPL_OK;
    ssize_t bytesRead = 0;
    char *obj_path = NULL;

    {
        size_t szObjpath = strlen(shared_location) + strlen(object_id) + 2;
        obj_path = (char*)malloc(szObjpath);
        snprintf(obj_path, szObjpath, "%s\\%s", shared_location, object_id);

        rv = VPLFile_Delete(obj_path);
    }
end:
    if (obj_path == NULL)
        free(obj_path);

    return rv;
}

void VPLSharedObject_GetData(const char *shared_location, const char *object_id, void **pData, unsigned int &data_length)
{
    ssize_t bytesWritten = 0;
    char *obj_path=NULL;

    (*pData) = NULL;
    {
        size_t szObjpath = strlen(shared_location) + strlen(object_id) + 2;
        obj_path = (char*)malloc(szObjpath);
        snprintf(obj_path, szObjpath, "%s\\%s", shared_location, object_id);

        VPLFile_handle_t h = VPLFile_Open(obj_path, VPLFILE_OPENFLAG_READONLY, 0);
        if ( !VPLFile_IsValidHandle(h) ) {
            goto end;
        }

        data_length = VPLFile_Seek(h, 0, VPLFILE_SEEK_END);
        VPLFile_Seek(h, 0, VPLFILE_SEEK_SET);
        if (data_length > 0) {
            (*pData) = (void*)malloc(data_length);
            if ((*pData) == NULL)
                goto end;

            bytesWritten = VPLFile_Read(h, *pData, data_length);
            if (bytesWritten != data_length) {
                free(*pData);
                (*pData) = NULL;
            }
        }
        VPLFile_Close(h);
    }

end:
    if (obj_path != NULL)
        free(obj_path);
}

void VPLSharedObject_FreeData(void* pData)
{
    if (pData != NULL) {
        free(pData);
    }
}

void VPLSharedObject_GetString(const char *shared_location, const char *object_id, char **pString)
{
    int rv = VPL_OK;
    ssize_t bytesWritten=0, data_length=0;
    char *obj_path=NULL;

    (*pString) = NULL;
    {
        size_t szObjpath = strlen(shared_location) + strlen(object_id) + 2;
        obj_path = (char*)malloc(szObjpath);
        snprintf(obj_path, szObjpath, "%s\\%s", shared_location, object_id);

        VPLFile_handle_t h = VPLFile_Open(obj_path, VPLFILE_OPENFLAG_READONLY, 0);
        if ( !VPLFile_IsValidHandle(h) ) {
            goto end;
        }

        data_length = VPLFile_Seek(h, 0, VPLFILE_SEEK_END);
        VPLFile_Seek(h, 0, VPLFILE_SEEK_SET);
        if (data_length > 0) {
            (*pString) = (char*)malloc(data_length+1);
            if ((*pString) == NULL)
                goto end;

            bytesWritten = VPLFile_Read(h, (void*)*pString, data_length);
            (*pString)[bytesWritten] = '\0';
            if (bytesWritten != data_length) {
                free(*pString);
                (*pString) = NULL;
            }
        }
        VPLFile_Close(h);
    }

end:
    if (obj_path != NULL)
        free(obj_path);
}

void VPLSharedObject_FreeString(char* pString)
{
    if (pString != NULL) {
        free(pString);
    }
}

const char* VPLSharedObject_GetActoolLocation()
{
    if (strlen(ActoolLoc) > 0)
        return (const char*)ActoolLoc;

    char *actoolLocation = NULL;
    const wchar_t* _confFolder = L"conf";

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        goto end;
    }

    {
        int rv = VPL_OK;
        IAsyncOperation<StorageFolder^> ^getSharedFolder = KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(_VPL__SharedAppFolder), CreationCollisionOption::OpenIfExists);
        vplFolderChainer^ sharedfolder = ref new vplFolderChainer();
        getSharedFolder->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
            [&rv, &completedEvent, &actoolLocation, sharedfolder] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                try {
                    sharedfolder->folder = op->GetResults();
                    rv = VPL_OK;
                }
                catch (Exception^ ex) {
                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                }
                SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        ResetEvent(completedEvent);
        if (rv != VPL_OK)
            goto end;

        auto getConfFolder = sharedfolder->folder->CreateFolderAsync(ref new String(_confFolder), CreationCollisionOption::OpenIfExists);
        getConfFolder->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
            [&rv, &completedEvent, &actoolLocation] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                try {
                    _VPL__wstring_to_utf8_alloc(op->GetResults()->Path->Data(), &actoolLocation);
                    rv = VPL_OK;
                }
                catch (Exception^ ex) {
                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                }
                SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        CloseHandle(completedEvent);
        if (rv == VPL_OK)
            memcpy(ActoolLoc, actoolLocation, strlen(actoolLocation));
    }

end:
    if (actoolLocation != NULL)
        delete actoolLocation;
    return (const char*)ActoolLoc;
}

const char* VPLSharedObject_GetCredentialsLocation()
{
    if (strlen(CredLoc) > 0)
        return (const char*)CredLoc;

    char *credLocation = NULL;
    const wchar_t _credFolder[] = L"cred";

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        goto end;
    }

    {
        int rv = VPL_OK;
        IAsyncOperation<StorageFolder^> ^getSharedFolder = KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(_VPL__SharedAppFolder), CreationCollisionOption::OpenIfExists);
        vplFolderChainer^ sharedfolder = ref new vplFolderChainer();
        getSharedFolder->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
            [&rv, &completedEvent, sharedfolder] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                try {
                    sharedfolder->folder = op->GetResults();
                    rv = VPL_OK;
                }
                catch (Exception^ ex) {
                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                }
                SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        ResetEvent(completedEvent);
        if (rv != VPL_OK)
            goto end;

        auto getCCFolder = sharedfolder->folder->CreateFolderAsync(ref new String(_credFolder), CreationCollisionOption::OpenIfExists);
        getCCFolder->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^> (
            [&rv, &completedEvent, &credLocation] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                try {
                    _VPL__wstring_to_utf8_alloc(op->GetResults()->Path->Data(), &credLocation);
                    rv = VPL_OK;
                }
                catch (Exception^ ex) {
                    rv = _VPLFS_Error_XlatErrno(ex->HResult);
                }
                SetEvent(completedEvent);
        });
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        CloseHandle(completedEvent);
        if (rv == VPL_OK)
            memcpy(CredLoc, credLocation, strlen(credLocation));
    }

end:
    if (credLocation != NULL)
        delete credLocation;
    return (const char*)CredLoc;
}
