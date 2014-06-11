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
#include "vplTest.h"

using namespace Concurrency;
using namespace Windows::Storage;
using namespace test_vpl_metro;

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
        String^ localFolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
        auto wLogPath = localFolder + ref new String(L"\\test_vpl.log");
        char* logPath;
        _VPL__wstring_to_utf8_alloc(wLogPath->Data(), &logPath);
        
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if (!completedEvent) {
            return;
        }

        auto chainer = ref new folderChainer();
        IAsyncOperation<StorageFolder^>^ createFolderAction = KnownFolders::MusicLibrary->CreateFolderAsync(ref new String(_VPL__SharedAppFolder),CreationCollisionOption::OpenIfExists);
        createFolderAction->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^>(
            [chainer,&completedEvent] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                try {
                    chainer->folder = op->GetResults();
                }
                catch (Exception^ ex) {
                    auto msg = ex->Message;
                }
                SetEvent(completedEvent);
            }
        );
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        ResetEvent(completedEvent);

        VPLTEST_LOGINIT(logPath);
        free(logPath);

        s_main->UpdateResult(L"Test start running...");

        // run all vplex tests
        char *argv[1] = {"TestVplMetro"};
        int rv = testVPL(1, argv);

        VPLTEST_LOGCLOSE();

        s_main->UpdateResult(L"Test Finished");

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
    });
    t.detach();
}
