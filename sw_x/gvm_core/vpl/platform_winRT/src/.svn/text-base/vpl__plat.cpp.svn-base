#include "vpl_plat.h"
#include "vplu.h"
#include "log.h"
#include "vpl__socket_priv.h"
#include <thread>

using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::ExchangeActiveSyncProvisioning;
using namespace Platform;

static int gInitialized = 0;

LPCWSTR _hwUuidFile = L"device";

int VPL_Init(void)
{
    int err;

    if ( gInitialized != 0 ) {
        return VPL_ERR_IS_INIT;
    }

    // TODO: Initialization
    VPLSocket_Init();
    gInitialized = 1;
    return VPL_OK;
}

VPL_BOOL VPL_IsInit()
{
    if (gInitialized) {
        return VPL_TRUE;
    } else {
        return VPL_FALSE;
    }
}

int VPL_Quit(void)
{
    if ( gInitialized == 0 ) {
        return VPL_ERR_NOT_INIT;
    }

    // TODO: job before exit
    VPLSocket_Quit();
    gInitialized = 0;
    return VPL_OK;
}

int VPL_GetOSUserName(char** osUserName_out)
{
    if(osUserName_out == NULL) {
        return VPL_ERR_INVALID;
    }
    // Not used in WinRT
    // TODO: retrieve 
    UNUSED(osUserName_out);
    return VPL_ERR_NOOP;
}

void VPL_ReleaseOSUserName(char* osUserName)
{
    // TODO: properly release osUserName
    UNUSED(osUserName);
}

static String^ GenerateRndData()
{
    // Define the length, in bytes, of the buffer.
    uint32_t length = 32;

    // Generate random data and copy it to a buffer.
    auto buffer = CryptographicBuffer::GenerateRandom(length);

    // Encode the buffer to a hexadecimal string (for display).
    String^ hexRnd = CryptographicBuffer::EncodeToHexString(buffer);

    return hexRnd;
}

int VPL_GetHwUuid(char** hwUuid_out)
{
    int iRet = VPL_ERR_FAIL;

    // TODO: retrieve gwUuid
    // 1. if hwUuid doesn't exist, create a random GUID and store as a hwUuid file in Windows Libraries
    // 2. get such GUID as hwUuid from hwUuid file

    if (hwUuid_out == NULL) {
        return VPL_ERR_INVALID;
    }
    *hwUuid_out = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        return VPL_ERR_FAIL;
    }

    auto localfolder = Windows::Storage::KnownFolders::MusicLibrary;
    // 1. open/create AcerCloud folder in Music Library
    auto createFolder = localfolder->CreateFolderAsync(ref new String(_VPL__SharedAppFolder), CreationCollisionOption::OpenIfExists);
    createFolder->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^>(
        [&completedEvent, hwUuid_out, &iRet] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
            try {
                StorageFolder^ folder = op->GetResults();
                // 2. open deviceInfo file to read
                auto openDevInfo = folder->GetFileAsync(ref new String(_hwUuidFile));
                openDevInfo->Completed = ref new AsyncOperationCompletedHandler<StorageFile^>(
                    [&completedEvent, hwUuid_out, &iRet, folder] (IAsyncOperation<StorageFile^>^ op, AsyncStatus status) {
                        try {
                            // 3. read hwUuid from the file
                            auto reader = FileIO::ReadTextAsync(op->GetResults());
                            reader->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Platform::String^>(
                                [&completedEvent, hwUuid_out, &iRet] (Windows::Foundation::IAsyncOperation<Platform::String^>^ op, Windows::Foundation::AsyncStatus status) {
                                    //raed data from ccd.conf file
                                    try {
                                        _VPL__wstring_to_utf8_alloc(op->GetResults()->Data(), hwUuid_out);
                                        iRet = VPL_OK;
                                    }
                                    catch(Platform::Exception^ e){
                                        LOG_TRACE("Failed to read device file in Music Library, Exception HResult = %d",e->HResult);
                                        iRet = VPL_ERR_ACCESS;
                                    }
                                    SetEvent(completedEvent);
                                }
                            );
                        }
                        catch (Exception^ ex) {
                            // 4. Failed to get hwUuid because file doesn't exist, should create file and generate hwUuid
                            auto createFile = folder->CreateFileAsync(ref new String(_hwUuidFile), CreationCollisionOption::ReplaceExisting);
                            createFile->Completed = ref new AsyncOperationCompletedHandler<StorageFile^>(
                                [&completedEvent, hwUuid_out, &iRet] (IAsyncOperation<StorageFile^>^ op, AsyncStatus status) {
                                    String^ bufHwUuid = GenerateRndData();
                                    try {
                                        _VPL__wstring_to_utf8_alloc(bufHwUuid->Data(), hwUuid_out);
                                        auto storeUuid = FileIO::WriteTextAsync(op->GetResults(), bufHwUuid);
                                        storeUuid->Completed = ref new AsyncActionCompletedHandler(
                                            [&completedEvent, &iRet] (IAsyncAction^ op, AsyncStatus status) {
                                                try {
                                                    op->GetResults();
                                                    iRet = VPL_OK;
                                                }
                                                catch (Exception^ ex) {
                                                    HRESULT hr = ex->HResult;
                                                    iRet = VPL_ERR_NOENT;
                                                    LOG_ERROR("GetResults Exception Code: %d", hr);
                                                }
                                                SetEvent(completedEvent);
                                            }
                                        );
                                    }
                                    catch (Exception^ ex) {
                                        HRESULT hr = ex->HResult;
                                        iRet = VPL_ERR_NOENT;
                                        LOG_ERROR("WriteTextAsync Exception Code: %d", hr);
                                        SetEvent(completedEvent);
                                    }
                                }
                            );
                        }
                    }
                );
            }
            catch (Exception^ ex) {
                // Failed to create AcerCloud folder in Music Library
                LOG_TRACE("Failed to create AcerCloud folder in Music Library for device info, Exception HResult = %d",ex->HResult);
                iRet = VPL_ERR_ACCESS;
                SetEvent(completedEvent);
            }
        }
    );

    WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
    CloseHandle(completedEvent);

out:
    return iRet;
}

void VPL_ReleaseHwUuid(char* hwUuid)
{
    // User should call VPL_ReleaseHwUuid to properly release hwUuid
    if (hwUuid != NULL) {
        free (hwUuid);
        hwUuid = NULL;
    }
}

int VPL_GetDeviceInfo(char **manufacturer, char** model)
{
    int rv = VPL_ERR_FAIL;

    if (manufacturer == NULL) {
        return VPL_ERR_INVALID;
    }
    if (model == NULL) {
        return VPL_ERR_INVALID;
    }

    *manufacturer = NULL;
    *model = NULL;

    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }


    EasClientDeviceInformation^ info = ref new EasClientDeviceInformation();
    String^ systemSku = info->SystemSku;
    String^ systemManufacturer = info->SystemManufacturer;
    String^ systemProductName = info->SystemProductName;

    try {
        if(!systemSku->IsEmpty()){
            _VPL__wstring_to_utf8_alloc(systemSku->Data(), model);
        }else{
            _VPL__wstring_to_utf8_alloc(systemProductName->Data(), model);
        }
        _VPL__wstring_to_utf8_alloc(systemManufacturer->Data(), manufacturer);
        rv = VPL_OK;
    }
    catch(Platform::Exception^ e){
        LOG_TRACE("Failed to read SystemProductName, Exception HResult = %d",e->HResult);
        rv = VPL_ERR_ACCESS;
    }

    return rv;
}

void VPL_ReleaseDeviceInfo(char* manufacturer, char* model)
{
    // User should call VPL_ReleaseDeviceInfo to properly release manufacturer/model
    if (manufacturer != NULL) {
        free (manufacturer);
        manufacturer = NULL;
    }


    if (model != NULL) {
        free (model);
        model = NULL;
    }
}

int VPL_GetOSVersion(char** osVersion_out)
{

    int rv = VPL_OK;

    if (osVersion_out == NULL) {
        return VPL_ERR_INVALID;
    }

    *osVersion_out = NULL;

    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }

    char* buf = NULL;

    buf = strdup("Windows RT");
    if(buf == NULL){
        rv = VPL_ERR_NOMEM;
        goto cleanup;
    }
    *osVersion_out = buf;

cleanup:

    if(buf != NULL && rv != VPL_OK){
        free(buf);
        buf = NULL;
        *osVersion_out = NULL;
    }


    return rv;
}

void VPL_ReleaseOSVersion(char* osVersion)
{
    // User should call VPL_ReleaseOSVersion to properly release osVersion
    if (osVersion != NULL) {
        free(osVersion);
        osVersion = NULL;
    }

}


