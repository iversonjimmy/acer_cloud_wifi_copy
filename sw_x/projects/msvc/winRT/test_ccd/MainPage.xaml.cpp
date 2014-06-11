//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <roapi.h>
#include <thread>

// header files for ccd test
#include <ppltasks.h>

#include "ccdi.hpp"
#include "ccd_core.h"
#include "SyncConfigTest.hpp"
#include "log.h"

#define USERNAME L"defaultCCDTester@igware.com"
#define PASSWORD L"password"
#define IAS_DOMAIN L"pc.igware.net"
#define IAS_PORT L"443"
#define LOGFOLDER "syncConfig"
#define WLOGFOLDER L"syncConfig"

using namespace Concurrency;
using namespace Windows::Storage;

using namespace test_ccd;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238
static MainPage^ s_main = nullptr;

ref class folderChainer
{
internal:
    StorageFolder^ folder;
};

MainPage::MainPage()
{
	InitializeComponent();
    s_main = this;
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

    std::thread t([] {
        s_main->UpdateResult(L"Test start running...");
        int rv = 0;
        {
            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if( !completedEvent ) {
                goto end;
            }

            // Query username/password directly from files in MusicLibrary/AcerCloud
            char *username = NULL, *password = NULL;
            char *ias_domain = NULL, *ias_port = NULL;
            // Get username/password from MusicLibrary/AcerCloud/LoginInfo
            auto createFolderAction = KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(L"AcerCloud"),CreationCollisionOption::OpenIfExists);
            createFolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^>(
                [&username, &password, &ias_domain, &ias_port, &completedEvent] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                    try {
                        auto getFile = op->GetResults()->GetFileAsync(ref new String(L"syncConfig"));
                        getFile->Completed = ref new AsyncOperationCompletedHandler<StorageFile^>(
                            [&username, &password, &ias_domain, &ias_port, &completedEvent] (IAsyncOperation<StorageFile^>^ op, AsyncStatus status) {
                                try {
                                    auto readFile = FileIO::ReadLinesAsync(op->GetResults());
                                    readFile->Completed = ref new AsyncOperationCompletedHandler<IVector<String^>^>(
                                        [&username, &password, &ias_domain, &ias_port, &completedEvent] (IAsyncOperation<IVector<String^>^>^ op, AsyncStatus status) {
                                            try {
                                                IVector<String^>^ syncConfigs = op->GetResults();
                                                if (syncConfigs->Size == 4) {
                                                    _VPL__wstring_to_utf8_alloc(syncConfigs->GetAt(0)->Data(), &username);
                                                    _VPL__wstring_to_utf8_alloc(syncConfigs->GetAt(1)->Data(), &password);
                                                    _VPL__wstring_to_utf8_alloc(syncConfigs->GetAt(2)->Data(), &ias_domain);
                                                    _VPL__wstring_to_utf8_alloc(syncConfigs->GetAt(3)->Data(), &ias_port);
                                                }
                                                else
                                                    throw ref new Exception(0, ref new String(L"Incorrect format of syncConf!!"));
                                                SetEvent(completedEvent);
                                            }
                                        catch (Exception^ ex) {
                                            _VPL__wstring_to_utf8_alloc(USERNAME, &username);
                                            _VPL__wstring_to_utf8_alloc(PASSWORD, &password);
                                            _VPL__wstring_to_utf8_alloc(IAS_DOMAIN, &ias_domain);
                                            _VPL__wstring_to_utf8_alloc(IAS_PORT, &ias_port);
                                            SetEvent(completedEvent);
                                        }
                                    });
                                }
                                catch (Exception^ ex) {
                                    _VPL__wstring_to_utf8_alloc(USERNAME, &username);
                                    _VPL__wstring_to_utf8_alloc(PASSWORD, &password);
                                    _VPL__wstring_to_utf8_alloc(IAS_DOMAIN, &ias_domain);
                                    _VPL__wstring_to_utf8_alloc(IAS_PORT, &ias_port);
                                    SetEvent(completedEvent);
                                }
                            }
                        );
                    }
                    catch (Exception^ ex) {
                        _VPL__wstring_to_utf8_alloc(USERNAME, &username);
                        _VPL__wstring_to_utf8_alloc(PASSWORD, &password);
                        _VPL__wstring_to_utf8_alloc(IAS_DOMAIN, &ias_domain);
                        _VPL__wstring_to_utf8_alloc(IAS_PORT, &ias_port);
                        SetEvent(completedEvent);
                    }
                }
            );

            WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
            CloseHandle(completedEvent);

            // Launch syncConfig test
            const char *argv[5] = {"sync_Config.appx", username, password, ias_domain, ias_port};
            rv = syncConfigTest(5, argv);

            // free username & password properly
            if (username != NULL)
                free(username);
            if (password != NULL)
                free(password);
            if (ias_domain != NULL)
                free(ias_domain);
            if (ias_port != NULL)
                free(ias_port);
        }
        {
            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            // At the end of the test, create file "test_done" in MusicLibrary/AcerCloud
            try {
                auto createFolderAction = KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(L"AcerCloud"),CreationCollisionOption::OpenIfExists);
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
        RoUninitialize();
        s_main->UpdateResult(L"Test Finished");
        return (rv != 0);
    });
    t.detach();
}
