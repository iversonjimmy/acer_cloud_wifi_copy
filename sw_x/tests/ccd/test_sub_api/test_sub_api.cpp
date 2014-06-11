//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// Command-line "menu" for testing CCD.  Asks for a username and password,
// logs in to infra, and displays download progress until all titles have
// finished or the program is interrupted, at which point it will log out
// and exit.

#include <ccdi.hpp>
#include <log.h>

#include <unistd.h>
#include <semaphore.h>

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>

// test_sub_api:  sub is short for subscription

//CCDILinkDevice
//CCDIUnlinkDevice
//CCDIAddDataSet
//CCDIAddSubscriptions
//CCDIDeleteSubscriptions
//CCDIListOwnedDataSets
//CCDIListSubscriptions 

static void printListSubscriptions(u64 userId)
{
    ccd::ListSyncSubscriptionsInput request;
    ccd::ListSyncSubscriptionsOutput response;

    request.set_user_id(userId);

    LOG_INFO("Calling CCDIListSyncSubscriptions for user "FMTu64, userId);

    int rv = CCDIListSyncSubscriptions(request, response);

    if(rv != 0) {
        LOG_ERROR("Error CCDIListSyncSubscriptions:%d", rv);
    }else{
        for(int i=0; i<response.subscriptions_size(); i++) {
            printf("--Subscription %d BEGIN--\n", i);
            if(response.subscriptions(i).has_datasetname()) {
                printf("  Dataset Name:%s\n", response.subscriptions(i).datasetname().c_str());
            }
            if(response.subscriptions(i).has_datasetid()) {
                printf("  Dataset ID:"FMTu64"\n", response.subscriptions(i).datasetid());
            }
            if(response.subscriptions(i).has_deviceroot()) {
                printf("  DeviceRoot:%s\n", response.subscriptions(i).deviceroot().c_str());
            }
            if(response.subscriptions(i).has_datasetroot()) {
                printf("  DatasetRoot:%s\n", response.subscriptions(i).datasetroot().c_str());
            }
            if(response.subscriptions(i).has_uploadok()) {
                printf("  UploadOk:%d\n", response.subscriptions(i).uploadok());
            }
            if(response.subscriptions(i).has_downloadok()) {
                printf("  DownloadOk:%d\n", response.subscriptions(i).downloadok());
            }
            if(response.subscriptions(i).has_uploaddeleteok()) {
                printf("  UploadDeleteOk:%d\n", response.subscriptions(i).uploaddeleteok());
            }
            if(response.subscriptions(i).has_downloaddeleteok()) {
                printf("  DownloadDeleteOk:%d\n", response.subscriptions(i).downloaddeleteok());
            }
            if(response.subscriptions(i).has_datasetlocation()) {
                printf("  DatasetLocation:%s\n", response.subscriptions(i).datasetlocation().c_str());
            }
            if(response.subscriptions(i).has_filter()) {
                printf("  Filter:%s\n", response.subscriptions(i).filter().c_str());
            }
            printf("--Subscription %d END--\n", i);
        }
    }
}

static void printCCDISyncState(u64 userId)
{
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(userId);
    request.set_get_device_name(true);

    LOG_INFO("Calling CCDIGetSyncState for user "FMTu64, userId);

    int rv = CCDIGetSyncState(request, response);

    if(rv != 0) {
        LOG_ERROR("Error CCDIGetSyncState:%d", rv);
    }else{
        printf("  Device is Linked bool:%d\n", response.is_device_linked());
        printf("  SyncAgent enabled bool:%d\n", response.is_sync_agent_enabled());
        if(response.has_my_device_name()) {
            printf("  Device Name:%s\n", response.my_device_name().c_str());
        }
        if(response.has_max_download_rate_bytes_sec()) {
            printf("  MaxDownloadRate:"FMTu64"\n", response.max_download_rate_bytes_sec());
        }
        if(response.has_max_upload_rate_bytes_sec()) {
            printf("  MaxUploadRate:"FMTu64"\n", response.max_upload_rate_bytes_sec());
        }
        for(int i=0; i<response.sync_states_for_paths_size(); i++) {
            printf("    Path#%d -not printed- \n", i);
        }
    }
}

static void addCameraDataSetAndSubscribe(u64 userId)
{
    {
        ccd::AddDatasetInput request;
        ccd::AddDatasetOutput response;
    
        request.set_user_id(userId);
        request.set_dataset_name("MyCameraDataset");
        request.set_dataset_type(ccd::NEW_DATASET_TYPE_CAMERA);

        LOG_INFO("Calling CCDIAddDataset for MyCameraDataset");
        int rv = CCDIAddDataset(request, response);
    
        if(rv != 0) {
            LOG_ERROR("Error CCDIAddDataset: %d", rv);
        }else{
            printf("  MyCameraDataset added, with datasetId:"FMTu64"\n", response.dataset_id());
            
            ccd::AddSyncSubscriptionInput in;
            in.set_user_id(userId);
            in.set_dataset_id(response.dataset_id());
            in.set_subscription_type(ccd::SUBSCRIPTION_TYPE_CAMERA);
    
            LOG_INFO("Calling AddSyncSubscription for camera dataset.");
            rv = CCDIAddSyncSubscription(in);
            if(rv != 0) {
                LOG_ERROR("Error AddSyncSubscription: %d", rv);
            }else{
                printf("  AddSyncSubscription complete\n");
            }
        }
    }
    
    // Check that it shows up as created by this device.
    {
        ccd::ListOwnedDatasetsInput request;
        request.set_user_id(userId);
        ccd::ListOwnedDatasetsOutput response;
        LOG_INFO("Calling CCDIListOwnedDatasets");
        int rv = CCDIListOwnedDatasets(request, response);
        if (rv != 0) {
            LOG_ERROR("Error CCDIListOwnedDatasets: %d", rv);
        } else {
            printf("  Datasets:\n%s\n", response.DebugString().c_str());
        }
    }
}

static void printDatasetDirectoryEntries(u64 userId)
{
    ccd::GetDatasetDirectoryEntriesInput request;
    ccd::GetDatasetDirectoryEntriesOutput response;
    int rv = 0;
    u64 datasetId = 0;
    const char* directoryName = "/";
    char tempStr[1024];
    printf(" DatasetId [0]:");
    rv = scanf("%"SCNu64, &datasetId);
    if(rv<0){
        LOG_ERROR("scanf Err.");
    }
    printf( "/n  Directory Name [/]:");
    rv = scanf("%s", tempStr);
    if(rv<0){
        LOG_ERROR("scanf Err.");
    }
    if(std::string(tempStr) != "") {
        directoryName = tempStr;
    }

    request.set_user_id(userId);
    request.set_dataset_id(datasetId);
    request.set_directory_name(directoryName);
    printf("Querying user:"FMTx64", datasetId:"FMTu64", directoryName:%s",
            userId, datasetId, directoryName);
    rv = CCDIGetDatasetDirectoryEntries(request, response);
    if(rv != 0) {
        LOG_ERROR("Error calling CCDIGetDatasetDirectoryEntries:%d", rv);
    } else {
        LOG_INFO("CCDIGetDatasetDirectoryEntries complete");
    }
    for(int i =0; i<response.entries_size(); ++i) {
        printf("  %s, isDir:%d, size:"FMTu64"\n",
               response.entries(i).name().c_str(),
               response.entries(i).is_dir(),
               response.entries(i).size());
    }
}

int
main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);
    
    std::string username, password;
    int rv;
    u64 userId = 0;
    bool user_is_logged_in = false;
    bool device_is_linked = false;

    printf("Enter username: ");
    std::cin >> username;
    printf("Enter password: ");
    std::cin >> password;

    {
        ccd::LoginInput loginRequest;
        loginRequest.set_player_index(0);
        loginRequest.set_user_name(username);
        loginRequest.set_password(password);
        ccd::LoginOutput loginResponse;
        printf("Logging in...\n");
        rv = CCDILogin(loginRequest, loginResponse);
        if (rv != CCD_OK) {
            printf("Error: CCDILogin: %d\n", rv);
            goto out;
        }
        user_is_logged_in = true;
        userId = loginResponse.user_id();
        printf("Login complete.\n");
    }

    {
        // Link device
        ccd::LinkDeviceInput linkInput;
        linkInput.set_user_id(userId);
        char myname[1024];
        gethostname(myname, sizeof(myname));
        linkInput.set_device_name(myname);
        printf("Linking device %s\n", myname);
        rv = CCDILinkDevice(linkInput);
        if (rv != CCD_OK) {
            printf("Error: CCDILinkDevice: %d\n", rv);
            goto out;
        }
        device_is_linked = true;
    }

    {
        // Subscribe dataset
        int i;
        ccd::ListOwnedDatasetsInput ownedDatasetsInput;
        ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
        ownedDatasetsInput.set_user_id(userId);
        rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
        if (rv != CCD_OK) {
            printf("Error: CCDIListOwnedDatasets: %d\n", rv);
            goto out;
        }

        for (i = 0; i < ownedDatasetsOutput.dataset_details_size(); i++) {
            ccd::AddSyncSubscriptionInput addSyncSubInput;
            u64 datasetId = ownedDatasetsOutput.dataset_details(i).datasetid(); 
            addSyncSubInput.set_user_id(userId);
            addSyncSubInput.set_dataset_id(datasetId);
            addSyncSubInput.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
            rv = CCDIAddSyncSubscription(addSyncSubInput);
            if (rv != CCD_OK) {
                printf("Error: CCDIAddSyncSubscription failed for dataset_id "FMTu64": %d\n", datasetId, rv);
                goto out;
            }
        }
    }

    {
        int option = -1;
        while(option != 0){
            printf("\nEnter an option number:\n");
            printf("  0) Exit\n");
            printf("  1) List subscriptions\n");
            printf("  2) Get CCDISyncState\n");
            printf("  3) Add Camera Dataset AND subscribe to it.\n");
            printf("  4) GetDatasetDirectoryEntries\n");

            rv = scanf("%d", &option);
            if(rv <= 0) {
                LOG_ERROR("Problem parsing option:%d", rv);
                continue;
            }

            switch(option) {
            case 0:
                printf("Exiting...\n");
                break;
            case 1:
                printListSubscriptions(userId);
                break;
            case 2:
                printCCDISyncState(userId);
                break;
            case 3:
                addCameraDataSetAndSubscribe(userId);
                break;
            case 4:
                printDatasetDirectoryEntries(userId);
                break;
            default:
                printf("Option %d not supported\n", option); 
            }
        }
    }

 out:
    if (device_is_linked) {
        ccd::UnlinkDeviceInput unlinkInput;
        printf("Unlinking device\n");
        unlinkInput.set_user_id(userId);
        rv = CCDIUnlinkDevice(unlinkInput);
        if (rv != CCD_OK) {
            printf("Error: CCDIUnlinkDevice: %d\n", rv);
        }
        device_is_linked = false;
    }

    if (user_is_logged_in) {
        ccd::LogoutInput logoutRequest;
        logoutRequest.set_local_user_id(userId);
        rv = CCDILogout(logoutRequest);
        if (rv != CCD_OK) {
            printf("Error: CCDILogout: %d\n", rv);
        }
        user_is_logged_in = false;
    }

    return rv;
}
