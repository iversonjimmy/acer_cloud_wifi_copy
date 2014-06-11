// CCDIClientSanityTest.cpp : Defines the entry point for the console application.
//
#include "CCLibSanityTest.h"

#include "vplu.h"
#include "vpl_user.h"
#include "ccdi.hpp"
#include "ccd_core.h"
#include "log.h"

#define MAX_LOG_SIZE 10*1024*1024

/// A #VPL_DebugCallback_t.
static void debugCallback(const VPL_DebugMsg_t* data)
{
    printf("debugCallback:%s:%d: %s\n", data->file, data->line, data->msg);
}

static int CallGetSyncState(u64 userId)
{
    std::string test_dir(getTestPath());
    ccd::GetSyncStateInput request;
    request.set_user_id(userId);
    request.set_get_device_name(true);
    request.add_get_sync_states_for_paths(test_dir.c_str());
    request.add_get_sync_states_for_paths((test_dir+"My Photos and other stuff").c_str());
    request.add_get_sync_states_for_paths((test_dir+"My Photo").c_str());
    request.add_get_sync_states_for_paths((test_dir+"My Photos").c_str());
    request.add_get_sync_states_for_paths((test_dir+"MY PHOTOS").c_str()); // TODO: see Bug 9225
    request.add_get_sync_states_for_paths((test_dir+"My Photos/img3333.jpg").c_str());
    request.add_get_sync_states_for_paths((test_dir+"My Photos/My Videos/").c_str());
    request.add_get_sync_states_for_paths((test_dir+"My Photos\\").c_str());
    request.add_get_sync_states_for_paths((test_dir+"My Photos\\Whatever\\And Some More Stuff - Here\\foobar.txt").c_str());
    request.add_get_sync_states_for_paths((test_dir+"Whatever\\").c_str());
    request.add_get_sync_states_for_paths((test_dir+"My Cloud/").c_str());
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

void runSanityTest()
{
    std::string test_dir(getTestPath());
    printf("Init Path: %s\n", test_dir.c_str());
    LOGInit("ccdi_client_sanity_test", (test_dir).c_str());
    LOGSetMax(MAX_LOG_SIZE);
    //LOGSetWriteToStdout(VPL_FALSE);
    LOG_INFO("START"); 

    VPL_RegisterDebugCallback(debugCallback);

    int rv;

    rv = CCDStart("ccd", getTestPath(), NULL);
    if (rv != 0) {
        LOG_ERROR("CCDStart failed!");
        return;
    }

    {
        ccd::GetInfraHttpInfoInput request;
        request.set_service(ccd::INFRA_HTTP_SERVICE_OPS);
        request.set_secure(true);
        ccd::GetInfraHttpInfoOutput response;
        rv = CCDIGetInfraHttpInfo(request, response);
        printf("CCDIGetInfraHttpInfo returned %d\n", rv);
        if (rv < 0) {
//            goto end;
        }
        printf("  prefix: %s\n", response.url_prefix().c_str());
    }
    u64 userId = 0;
    {
        ccd::LoginInput request;
        request.set_user_name(USERNAME);
        request.set_password(PASSWORD);
        ccd::LoginOutput response;
        rv = CCDILogin(request, response);
        printf("CCDILogin returned %d\n", rv);
        if (rv < 0) {
            goto end;
        }
        userId = response.user_id();
        printf("  userId: "FMT_VPLUser_Id_t"\n", userId);
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
    rv = CallGetSyncState(userId);
    if (rv < 0) {
        goto end;
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
    VPLThread_Sleep(VPLTime_FromSec(3));
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
        for (int i = 0; i < response.dataset_details_size(); i++) {
            ccd::GetDatasetDirectoryEntriesInput request2;
            u64 datasetId = response.dataset_details(i).datasetid();
            if (i == 0) {
                ccd::AddSyncSubscriptionInput request3;
                request3.set_user_id(userId);
                request3.set_dataset_id(datasetId);
                request3.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
                rv = CCDIAddSyncSubscription(request3);
                printf("CCDIAddSyncSubscription(%s) returned %d\n", response.dataset_details(i).datasetname().c_str(), rv);
            }
            request2.set_dataset_id(datasetId);
            request2.set_user_id(userId);
            request2.set_directory_name("/");
            ccd::GetDatasetDirectoryEntriesOutput response2;
            rv = CCDIGetDatasetDirectoryEntries(request2, response2);
            printf("CCDIGetDatasetDirectoryEntries returned %d\n", rv);
            if (rv < 0){
                goto end;
            }
            for (int j = 0; j < response2.entries_size(); j++) {
                printf("  dataset[%d]("FMTx64")[%d] = %s%s\n",
                    i, datasetId, j,
                    response2.entries(j).name().c_str(),
                    (response2.entries(j).is_dir() ? "/" : ""));
            }
        }
    }
    rv = CallGetSyncState(userId);
    if (rv < 0) {
        goto end;
    }

end:
    {
        ccd::LogoutInput request;
        int logoutErr = CCDILogout(request);
        printf("CCDILogout returned %d\n", logoutErr);
    }
}

