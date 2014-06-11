//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <thread>
#include <ppltasks.h>

#include "ccdi.hpp"
#include "ccd_core.h"
#include "test_single_sign_on.h"
#include "log.h"
#include "vplex_file.h"

#define USERNAME L"defaultCCDTester@igware.com"
#define PASSWORD L"password"
#define LOGFOLDER "testCCDSingleSignOn"
#define WLOGFOLDER L"testCCDSingleSignOn"
#define WLOGFILE L"test_ccd_single_sign_on.log"

#define MAX_TOKEN_SIZE 1024

#define AUTOLOGIN L"autologin"

using namespace Concurrency;
using namespace test_ccd_single_sign_on;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Core;

using namespace Windows::Storage;

ref class folderChainer
{
internal:
    IStorageFolder^ folder;
};

// Build autologin path
// User should make sure path is freed by free()
static int GetAutologinPath(char **path)
{
    int rv = VPL_ERR_FAIL;
    folderChainer^ folder = ref new folderChainer();

    // Move autologin from Music/AcerCloud to LocalFolder
    task<StorageFolder^> taskGetWeakToken(KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(_VPL__SharedAppFolder),
                                            CreationCollisionOption::OpenIfExists));
    taskGetWeakToken.then([&rv, folder](StorageFolder^ syncagentFolder) {
        folder->folder = syncagentFolder;
    }).then([&rv](task<void> prevTask) {
        try {
            prevTask.get();
            rv = true;
        }
        catch(Exception^ ex) {
        }
    }).wait();

    if (rv) {
        auto autologinPath = folder->folder->Path + ref new String(L"\\") + ref new String(AUTOLOGIN);
        rv = _VPL__wstring_to_utf8_alloc(autologinPath->Data(), path);
    }

    return rv;
}

MainPage::MainPage()
{
	InitializeComponent();
}

void MainPage::UpdateResult(Platform::String^ msg)
{
    auto callback = ref new DispatchedHandler(
        [this, msg]() { 
            this->txtResult->Text += (msg + L"\n"); 
        }
    );
        
    this->txtResult->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, callback);
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter

    std::thread t([this] {
        int rv = VPL_OK;
        char *localPath=NULL;
        char *autologinPath=NULL;
        char *username=NULL, *password=NULL;

        this->UpdateResult(L"Test start running...");

        auto wLocalPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
        _VPL__wstring_to_utf8_alloc(wLocalPath->Data(), &localPath);

        auto testResultPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path
                            + ref new String(L"\\")
                            + ref new String(WLOGFILE);
        char *logpath;
        _VPL__wstring_to_utf8_alloc(testResultPath->Data(), &logpath);

        // remove cc folder if exists
        char *cc_path;
        _VPL__wstring_to_utf8_alloc(String::Concat(wLocalPath, ref new String(L"\\cc"))->Data(), &cc_path);
        rv = VPLFile_CheckAccess(cc_path, VPLFILE_CHECKACCESS_EXISTS);
        if (rv == VPL_OK) {
            auto get_cc_folder = StorageFolder::GetFolderFromPathAsync(String::Concat(wLocalPath, ref new String(L"\\cc")));
            task<StorageFolder^> task_cc_folder(get_cc_folder);
            task_cc_folder.then([this](StorageFolder^ cc_folder) {
                return cc_folder->DeleteAsync();
            }).then([this](task<void> pervTask) {
                try {
                    pervTask.get();
                    this->UpdateResult(L"cc folder deleted.");
                }
                catch(Exception^ ex) {
                    this->UpdateResult(L"cc folder failed to delete.");
                }
            }).wait();
        }
        else {
            this->UpdateResult(L"cc folder doesn't exist.");
        }
        free(cc_path);

        // start CCD
        rv = CCDStart(LOGFOLDER, localPath, NULL, NULL);
        if (rv != 0) {
            LOG_ERROR("CCDStart failed!");
            goto end;
        }
        {
            ccd::GetInfraHttpInfoInput request;
            request.set_service(ccd::INFRA_HTTP_SERVICE_OPS);
            request.set_secure(true);
            ccd::GetInfraHttpInfoOutput response;
            response.set_service_ticket("");
            rv = CCDIGetInfraHttpInfo(request, response);
            if (rv < 0) {
                goto end;
            }
        }

        // Query username/password directly from files in MusicLibrary/AcerCloud
        {
            // Get username/password from MusicLibrary/AcerCloud/LoginInfo
            task<StorageFolder^> taskUsername(KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(_VPL__SharedAppFolder),
                                                                                            CreationCollisionOption::OpenIfExists));
            taskUsername.then([&username, &password] (StorageFolder^ syncagentFolder) {
                return syncagentFolder->GetFileAsync(ref new String(L"LoginInfo"));
            }).then([&username, &password] (StorageFile^ usernameFile) {
                return FileIO::ReadLinesAsync(usernameFile);
            }).then([&username, &password] (IVector<String^> ^LoginInfo) {
                if (LoginInfo->Size == 2) {
                    _VPL__wstring_to_utf8_alloc(LoginInfo->GetAt(0)->Data(), &username);
                    _VPL__wstring_to_utf8_alloc(LoginInfo->GetAt(1)->Data(), &password);
                }
                else
                    throw ref new Exception(0, ref new String(L"Incorrect format of LoginInfo!!"));
            }).then([&username, &password] (task<void> prevTask) {
                try {
                    prevTask.get();
                }
                catch(Exception^ ex) {
                    _VPL__wstring_to_utf8_alloc(USERNAME, &username);
                    _VPL__wstring_to_utf8_alloc(PASSWORD, &password);
                }
            }).wait();
        }

        rv = GetAutologinPath(&autologinPath);
        if (rv != VPL_OK)
            this->UpdateResult(ref new String(L"Cannot get autologin to LocalFolder"));
        // 1. if no autologin found, login with username/password
        // 2. else, login with autologin
        rv = VPLFile_CheckAccess(autologinPath, VPLFILE_CHECKACCESS_EXISTS);
        if (rv != VPL_OK) {
            // Cannot find autologin file, login with username/password
            rv = test_login_with_password(username, password);
            
            if (rv == VPL_OK) {
                // Preserve autologin to file
                int flags = VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE;
                VPLFile_handle_t hToken = VPLFile_Open(autologinPath, flags, 0777);
                if (!VPLFile_IsValidHandle(hToken)) {
                    LOG_ERROR("Create file error. file:%s (hToken:%d)", autologinPath, hToken);
                    rv = -1;
                    goto end;
                }
                VPLFile_Close(hToken);
            }
            else{
                LOG_ERROR("test_login_with_password failed: %d", rv);
            }

            {
                // Output result to test_ccd_single_sign_on.log
                int flags = VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_APPEND;
                VPLFile_handle_t hToken = VPLFile_Open(logpath, flags, 0777);
                if (!VPLFile_IsValidHandle(hToken)) {
                    LOG_ERROR("Create file error. file:%s (hToken:%d)", logpath, hToken);
                    rv = -1;
                    goto end;
                }
                else
                    this->UpdateResult(ref new String(L"Export autologin to Music/AcerCloud"));
                char strResult[MAX_PATH] = {0};
                snprintf(strResult, MAX_PATH, "Summary: %s - test_ccd_single_sign_on_with_password\n", (rv == VPL_OK ? "PASS" : "FAIL"));
                VPLFile_Write(hToken, strResult, strlen(strResult));
                VPLFile_Close(hToken);
            }
        }
        else {
            // autologin file is found, login with username
            rv = test_login_with_credential(username);
            if (rv != VPL_OK) {
                LOG_ERROR("test_login_with_password failed: %d", rv);
            }

            {
                // Output result to test_ccd_single_sign_on.log
                int flags = VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_APPEND;
                VPLFile_handle_t hToken = VPLFile_Open(logpath, flags, 0777);
                if (!VPLFile_IsValidHandle(hToken)) {
                    LOG_ERROR("Create file error. file:%s (hToken:%d)", logpath, hToken);
                    rv = -1;
                    goto end;
                }
                char strResult[MAX_PATH] = {0};
                snprintf(strResult, MAX_PATH, "Summary: %s - test_ccd_single_sign_on_with_credential\n", (rv == VPL_OK ? "PASS" : "FAIL"));
                VPLFile_Write(hToken, strResult, strlen(strResult));
                VPLFile_Close(hToken);
            }
        }
        {
            // Shutdown CCD
            CCDShutdown();
            CCDWaitForExit();

            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            // At the end of the test, create file "test_done" in MusicLibrary/AcerCloud
            try {
                auto createFolderAction = KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(_VPL__SharedAppFolder),CreationCollisionOption::OpenIfExists);
                createFolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^>(
                    [&completedEvent] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                        try {
                            auto folder = op->GetResults();

                            auto createAction = folder->CreateFileAsync(ref new String(L"test_done"), CreationCollisionOption::ReplaceExisting);
                            createAction->Completed = ref new AsyncOperationCompletedHandler<StorageFile^>(
                                [&completedEvent] (IAsyncOperation<StorageFile^>^ op, AsyncStatus status) {
                                    try {
                                        op->GetResults();
                                    }
                                    catch (Exception^ ex) {
                                        LOG_ERROR("create test_done exception.");
                                    }
                                    SetEvent(completedEvent);
                                }
                            );
                        }
                        catch (Exception^ ex) {
                            LOG_ERROR("create AcerCloud exception.");
                            SetEvent(completedEvent);
                        }
                    }
                );
            }
            catch (Exception^ ex) {
                LOG_ERROR("create AcerCloud exception.");
                SetEvent(completedEvent);
            }

            WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
            CloseHandle(completedEvent);
        }
end:
        if (localPath != NULL)
            free(localPath);
        if (username != NULL)
            free(username);
        if (password != NULL)
            free(password);
        if (logpath != NULL)
            free(logpath);
        if (autologinPath != NULL)
            free(autologinPath);

        this->UpdateResult(L"Test Finished");
        
        return (rv != 0);
    });
    t.detach();
}
