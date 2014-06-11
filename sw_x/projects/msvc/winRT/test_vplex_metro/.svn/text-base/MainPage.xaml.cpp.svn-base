//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <roapi.h>
#include <thread>

#include <ppltasks.h>
#include "MainPage.xaml.h"
#include "vplexTest.h"

#define SERVER_URL L"ccd-http-test.ctbg.acer.com"
#define BRANCH     L"DEV"
#define PRODUCT    L"winrt"

using namespace Concurrency;
using namespace Windows::Storage;

using namespace test_vplex_metro;

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
    IStorageFolder^ folder;
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
        // Change thread to MTA
        HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
        if (FAILED(hr)) {
            return;
        }

        // Run vplex tests
        auto wLogPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path + ref new String(L"\\test_vplex.log");
        char* logPath;
        _VPL__wstring_to_utf8_alloc(wLogPath->Data(), &logPath);
        VPLTEST_LOGINIT(logPath);
        free(logPath);

        s_main->UpdateResult(L"Test start running...");

        int rv = 0;
        {
            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if( !completedEvent ) {
                goto end;
            }

            // Query server_url/branch/product
            char *server_url = NULL;
            char *branch = NULL;
            char *product = NULL;
            // Get server_url/branch from ApplicationData/<Current>/LocalState
            try {
                auto getFile = Windows::Storage::ApplicationData::Current->LocalFolder->GetFileAsync(ref new String(L"vplexTestConfig"));
                getFile->Completed = ref new AsyncOperationCompletedHandler<StorageFile^>(
                    [&server_url, &branch, &product, &completedEvent] (IAsyncOperation<StorageFile^>^ op, AsyncStatus status) {
                        try {
                            auto readFile = FileIO::ReadLinesAsync(op->GetResults());
                            readFile->Completed = ref new AsyncOperationCompletedHandler<IVector<String^>^>(
                                [&server_url, &branch, &product, &completedEvent] (IAsyncOperation<IVector<String^>^>^ op, AsyncStatus status) {
                                    try {
                                        IVector<String^>^ vplexTest_Configs = op->GetResults();
                                        if (vplexTest_Configs->Size == 3) {
                                            _VPL__wstring_to_utf8_alloc(vplexTest_Configs->GetAt(0)->Data(), &server_url);
                                            _VPL__wstring_to_utf8_alloc(vplexTest_Configs->GetAt(1)->Data(), &branch);
                                            _VPL__wstring_to_utf8_alloc(vplexTest_Configs->GetAt(2)->Data(), &product);
                                        }
                                        else
                                            throw ref new Exception(0, ref new String(L"Incorrect format of syncConf!!"));
                                        SetEvent(completedEvent);
                                    }
                                catch (Exception^ ex) {
                                    _VPL__wstring_to_utf8_alloc(SERVER_URL, &server_url);
                                    _VPL__wstring_to_utf8_alloc(BRANCH, &branch);
                                    _VPL__wstring_to_utf8_alloc(PRODUCT, &product);
                                    SetEvent(completedEvent);
                                }
                            });
                        }
                        catch (Exception^ ex) {
                            _VPL__wstring_to_utf8_alloc(SERVER_URL, &server_url);
                            _VPL__wstring_to_utf8_alloc(BRANCH, &branch);
                            _VPL__wstring_to_utf8_alloc(PRODUCT, &product);
                            SetEvent(completedEvent);
                        }
                    }
                );
            }
            catch (Exception^ ex) {
                _VPL__wstring_to_utf8_alloc(SERVER_URL, &server_url);
                _VPL__wstring_to_utf8_alloc(BRANCH, &branch);
                _VPL__wstring_to_utf8_alloc(PRODUCT, &product);
                SetEvent(completedEvent);
            }

            WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
            CloseHandle(completedEvent);

            // Launch syncConfig test
            char *argv[7] = {"TestVplexMetro",
                                   "--test-server-url",
                                   server_url,
                                   "--branch",
                                   branch,
                                   "--product",
                                   product
                                   };
            rv = testVPLex(7, argv);

            VPLTEST_LOGCLOSE();
            // free server_url & branch properly
            if (server_url != NULL)
                free(server_url);
            if (branch != NULL)
                free(branch);
            if (product != NULL)
                free(product);
        }

        // At the end of the test, create file "test_done" in MusicLibrary/AcerCloud
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
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
                                    s_main->UpdateResult(L"create test_done exception.");
                                }
                                SetEvent(completedEvent);
                            }
                        );
                    }
                    catch (Exception^ ex) {
                        s_main->UpdateResult(L"create AcerCloud exception.");
                        SetEvent(completedEvent);
                    }
                }
            );
        }
        catch (Exception^ ex) {
            s_main->UpdateResult(L"create AcerCloud exception.");
            SetEvent(completedEvent);
        }

        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        CloseHandle(completedEvent);
end:
        s_main->UpdateResult(L"Test Finished");
    });
    t.detach();
}
