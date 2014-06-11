using namespace Windows::Storage;
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplex_shared_object.h>
#include "dx_remote_agent_util.h"
#include "dx_remote_agent_util_winrt.h"
#include <string>
#include "ccd_core.h"
#include <log.h>

unsigned short getFreePort()
{
    return 24000;
}

int recordPortUsed(unsigned short port)
{
    // no-op on this platform
    return 0;
}

int get_user_folderW(std::wstring &path)
{
    Platform::String ^currPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
    std::wstring wstrPath = std::wstring(currPath->Begin(), currPath->End());
    wstrPath.append(L"\\dxshell_pushfiles");

    path = wstrPath;

    return 0;
}

int get_cc_folderW(std::wstring &path)
{
    Platform::String ^currPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
    std::wstring wstrPath = std::wstring(currPath->Begin(), currPath->End());
    //wstrPath.append(L"\\cc");

    path = wstrPath;

    return 0;
}

int get_user_folder(std::string &path)
{
    int rc = 0;
    std::wstring wstrPath;
    char *szPath = NULL;
    do
    {
        if ( (rc = get_user_folderW(wstrPath)) != 0) {
            break;
        }

        if ( (rc = _VPL__wstring_to_utf8_alloc(wstrPath.c_str(), &szPath)) != VPL_OK) {
            break;
        }

        path = std::string(szPath);

        rc = VPLDir_Create(szPath, 0755);
        if (rc == VPL_ERR_EXIST) {
            rc = VPL_OK;
        }
    } while (false);

    if (szPath) {
        free(szPath);
        szPath = NULL;
    }
    return rc;
}

int get_cc_folder(std::string &path)
{
    int rc = 0;
    std::wstring wstrPath;
    char *szPath = NULL;
    do
    {
        if ( (rc = get_cc_folderW(wstrPath)) != 0) {
            break;
        }

        if ( (rc = _VPL__wstring_to_utf8_alloc(wstrPath.c_str(), &szPath)) != VPL_OK) {
            break;
        }

        path = std::string(szPath);

        rc = VPLDir_Create(szPath, 0755);
        if (rc == VPL_ERR_EXIST) {
            rc = VPL_OK;
        }
    } while (false);

    if (szPath) {
        free(szPath);
        szPath = NULL;
    }
    return rc;
}

int create_picstream_path()
{
    int rc = 0;
    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder ^> ^iPicstreamFolder = KnownFolders::PicturesLibrary->CreateFolderAsync("picstream", CreationCollisionOption::OpenIfExists);
    iPicstreamFolder->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFolder^>(
        [&completedEvent, &rc] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder^>^ op, Windows::Foundation::AsyncStatus status) {
            try {
                Windows::Storage::StorageFolder^ picstreamFolder = op->GetResults();
                SetEvent(completedEvent);
            }
            catch (Platform::Exception^ e) {
                LOG_DEBUG("Get picstream Exception");
                rc = -1;
                SetEvent(completedEvent);
            }
        }
    );

    WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
    return rc;
}

int clean_cc()
{
    int rc = VPLFS_Rmdir(VPLSharedObject_GetCredentialsLocation());
    return rc;
}

int startccd(const char* titleId)
{
    std::string ccFolder;
    get_cc_folder(ccFolder);
    return CCDStart("dx_remote_agent", ccFolder.c_str(), NULL, titleId);
}

int startccd(int testInstanceNum, const char* titleId) {
    // Due to the sandboxed model, testInstanceNum shouldn't be needed for WinRT.
    return -1;
}

int stopccd()
{
    //CCDShutdown();
    //CCDWaitForExit();
    return 0;
}

