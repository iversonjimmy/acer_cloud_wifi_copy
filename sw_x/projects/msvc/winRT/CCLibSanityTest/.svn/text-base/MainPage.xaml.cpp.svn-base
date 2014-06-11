//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <ppltasks.h>

#include "ccdi.hpp"
#include "ccd_core.h"
#include "log.h"
#define USERNAME "defaultCCDTester@igware.com"
#define PASSWORD "password"
#define LOGFOLDER "TestCCDMetro"
#define WLOGFOLDER L"TestCCDMetro"

using namespace Concurrency;
using namespace Windows::Storage;

using namespace App1;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238
MainPage::MainPage()
{
	InitializeComponent();
    VPLTestResult = "This is ccd sanity tests!";
}

static int CallGetSyncState(u64 userId)
{
    ccd::GetSyncStateInput request;
    request.set_user_id(userId);
    request.set_get_device_name(true);

    {
        auto wLocalPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
        size_t localPath_size = wcslen(wLocalPath->Data());
        char* localPath = (char*)malloc(localPath_size+1);
        memset(localPath, 0, localPath_size+1);
        _VPL__wstring_to_utf8(wLocalPath->Data(), wcslen(wLocalPath->Data()), localPath, localPath_size);

        request.add_get_sync_states_for_paths(localPath);
        free(localPath);
    }

    request.add_get_sync_states_for_paths("D:/test");
    request.add_get_sync_states_for_paths("D:/test/");
    request.add_get_sync_states_for_paths("D:/test/My Photos and other stuff");
    request.add_get_sync_states_for_paths("D:/test/My Photo");
    request.add_get_sync_states_for_paths("D:/test/My Photos");
    request.add_get_sync_states_for_paths("D:/TEST/MY PHOTOS"); // TODO: see Bug 9225
    request.add_get_sync_states_for_paths("D:/test/My Photos/img3333.jpg");
    request.add_get_sync_states_for_paths("D:/test/My Photos/My Videos/");
    request.add_get_sync_states_for_paths("D:\\test\\My Photos\\");
    request.add_get_sync_states_for_paths("D:\\test////My Photos\\Whatever\\And Some More Stuff - Here\\foobar.txt");
    request.add_get_sync_states_for_paths("C:\\test\\Whatever\\");
    request.add_get_sync_states_for_paths("C:/My Cloud/");

    //request.set_get_sync_state_summary(true);
    ccd::GetSyncStateOutput response;
    int rv = CCDIGetSyncState(request, response);
    printf("CCDIGetSyncState returned %d\n", rv);
    if (rv < 0) {
        return rv;
    }
    printf("response:\n----\n%s----\n", response.DebugString().c_str());
    return rv;
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

void MainPage::Button_Click_Start(Platform::Object^ Sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto ccdAgent = create_async([this] {
        // 1. retrieve LocalState folder in app container as CCDStart's LocalAppData input
        auto wLocalPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
        size_t localPath_size = wcslen(wLocalPath->Data());
        char* localPath = (char*)malloc(localPath_size+1);
        memset(localPath, 0, localPath_size+1);
        _VPL__wstring_to_utf8(wLocalPath->Data(), wcslen(wLocalPath->Data()), localPath, localPath_size);

        // 2. Init
        int rv = CCDStart(LOGFOLDER, localPath, NULL, NULL);
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
        // 3. Login
        u64 userId = 0;
        {
            ccd::LoginInput request;
            request.set_user_name(USERNAME);
            request.set_password(PASSWORD);
            ccd::LoginOutput response;
            rv = CCDILogin(request, response);
            if (rv < 0) {
                goto end;
            }
            else {
                userId = response.user_id();
            }
        }
        {
            ccd::GetInfraHttpInfoInput request;
            request.set_service(ccd::INFRA_HTTP_SERVICE_OPS);
            request.set_secure(true);
            request.set_user_id(userId);
            ccd::GetInfraHttpInfoOutput response;
            rv = CCDIGetInfraHttpInfo(request, response);
            printf("CCDIGetInfraHttpInfo returned %d\n", rv);
            if (rv < 0) {
                goto end;
            }
            printf("  prefix: %s\n", response.url_prefix().c_str());
        }
        {
            ccd::LinkDeviceInput request;
            request.set_user_id(userId);
            request.set_is_acer_device(true);
            request.set_device_name("My test PC");
            rv = CCDILinkDevice(request);
            printf("CCDILinkDevice returned %d\n", rv);
            if (rv < 0) {
                goto end;
            }

            rv = CallGetSyncState(userId);
            if (rv < 0) {
                goto end;
            }
        }
        {
            ccd::ListLinkedDevicesInput request;
            request.set_user_id(userId);
            ccd::ListLinkedDevicesOutput response;
            rv = CCDIListLinkedDevices(request, response);
            printf("CCDIListLinkedDevices returned %d\n", rv);
            if (rv < 0){
                goto end;
            }
            printf("response (%d):\n----\n%s----\n", response.devices_size(), response.DebugString().c_str());

            VPLThread_Sleep(VPLTime_FromSec(3));
        }
        {
            ccd::ListLinkedDevicesInput request;
            request.set_user_id(userId);
            ccd::ListLinkedDevicesOutput response;
            rv = CCDIListLinkedDevices(request, response);
            printf("CCDIListLinkedDevices returned %d\n", rv);
            if (rv < 0){
                goto end;
            }
            printf("response (%d):\n----\n%s----\n", response.devices_size(), response.DebugString().c_str());
        }
        {
            ccd::ListOwnedDatasetsInput request;
            request.set_user_id(userId);
            ccd::ListOwnedDatasetsOutput response;
            rv = CCDIListOwnedDatasets(request, response);
            printf("CCDIListOwnedDatasets returned %d\n", rv);
            if (rv < 0){
                goto end;
            }
            printf("response (%d):\n----\n%s----\n", response.dataset_details_size(), response.DebugString().c_str());
            for (int i = 0; i < response.dataset_details_size(); i++) {
                ccd::GetDatasetDirectoryEntriesInput request;
                u64 datasetId = response.dataset_details(i).datasetid();
                if (i == 0) {
                    ccd::AddSyncSubscriptionInput req;
                    req.set_user_id(userId);
                    req.set_dataset_id(datasetId);
                    req.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
                    rv = CCDIAddSyncSubscription(req);
                    printf("CCDIAddSyncSubscription returned %d\n", rv);
                }
                request.set_dataset_id(datasetId);
                request.set_user_id(userId);
                request.set_directory_name("/");
                
                ccd::GetDatasetDirectoryEntriesOutput response;
                rv = CCDIGetDatasetDirectoryEntries(request, response);
                printf("CCDIGetDatasetDirectoryEntries returned %d\n", rv);
                if (rv < 0){
                    goto end;
                }
                for (int j = 0; j < response.entries_size(); j++) {
                    printf("  dataset[%d]("FMTx64")[%d] = %s%s\n",
                        i, datasetId, j,
                        response.entries(j).name().c_str(),
                        (response.entries(j).is_dir() ? "/" : ""));
                }
            }
        }
end:
        free(localPath);
        {
            ccd::LogoutInput request;
            int logoutErr = CCDILogout(request);
            printf("CCDILogout returned %d\n", logoutErr);
        }
        return (rv != 0);
    });

}
