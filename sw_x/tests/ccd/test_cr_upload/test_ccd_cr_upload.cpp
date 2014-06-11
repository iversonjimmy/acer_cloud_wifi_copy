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

#include <getopt.h>
#include <unistd.h>
#include <semaphore.h>

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>

#include <vplex_assert.h>
#include <vpl_plat.h>

static sem_t interruptSem;

static char *opt_username = NULL;
static char *opt_password = NULL;
static bool opt_dont_subscribe = false;
static bool opt_pause_sync = false;
static bool opt_exit_after_login = false;

static void
usage(char *argv[])
{
    printf("Usage: %s [-h] [-u username] [-p password] [-n]\n", argv[0]);
    printf("    -u, --username  Username. [ask via stdin]\n");
    printf("    -p, --password  Password. [ask via stdin]\n");
    printf("    -n, --dont-subscribe  Don't subscribe to any of the datasets. [auto subscribe]\n");
    printf("    -e, --exit-after-login  Exit after logging in. [wait for SIGINT to exit]\n");
    printf("    -h, --help  Show this message.\n");
}

static void
parse_args(int argc, char *argv[])
{
    static struct option long_options[] = {
        { "username", required_argument, 0, 'u' },
        { "password", required_argument, 0, 'p' },
        { "dont-subscribe", no_argument, 0, 'n' },
        { "pause-sync", no_argument, 0, 's' },
        { "exit-after-login", no_argument, 0, 'e' },
        { "help", no_argument, 0, 'h'},
        { 0, 0, 0, 0 }
    };

    for (;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "u:p:nse", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 'u':
            opt_username = optarg;
            break;

        case 'p':
            opt_password = optarg;
            break;

        case 'n':
            opt_dont_subscribe = true;
            break;

        case 's':
            opt_pause_sync = true;
            LOG_ERROR("opt_pause_sync option has been removed");
            FAILED_ASSERT("opt_pause_sync option has been removed");
            break;

        case 'e':
            opt_exit_after_login = true;
            break;

        case 'h':
        default:
            usage(argv);
            exit(1);
            return;
        }
    }
}

static void printCCDISyncState(u64 userId)
{
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(userId);
    request.set_get_device_name(true);
    request.set_get_is_camera_roll_upload_enabled(true);

    LOG_INFO("Calling CCDIGetSyncState for user "FMTu64, userId);

    int rv = CCDIGetSyncState(request, response);

    if(rv != 0) {
        LOG_ERROR("Error CCDIGetSyncState:%d", rv);
    }else{
        printf("  Device is Linked bool:%d\n", response.is_device_linked());
        if(response.has_my_device_name()) {
            printf("  Device Name:%s\n", response.my_device_name().c_str());
        }
        if(response.has_is_camera_roll_upload_enabled()) {
            printf("  Camera Roll upload enabled bool:%d\n", response.is_camera_roll_upload_enabled());
        }
        if(response.has_max_download_rate_bytes_sec()) {
            printf("  MaxDownloadRate:"FMTu64"\n", response.max_download_rate_bytes_sec());
        }
        if(response.has_max_upload_rate_bytes_sec()) {
            printf("  MaxUploadRate:"FMTu64"\n", response.max_upload_rate_bytes_sec());
        }
    }
}

static void setCameraRollUploadEnable(u64 userId, bool crUploadEnable)
{
    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;

    request.set_user_id(userId);
    request.set_enable_camera_roll(crUploadEnable);

    LOG_INFO("Calling CCDIUpdateSyncSettings cameraRoll:%d for user "FMTu64, crUploadEnable, userId);

    int rv = CCDIUpdateSyncSettings(request, response);

    if(rv != 0) {
        LOG_ERROR("Error CCDIUpdateSyncSettings:%d", rv);
    }
}

int
main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);
    
    std::string username, password;
    int rv;
    u64 userId;

    parse_args(argc, argv);

    VPL_Init();   

    rv = sem_init(&interruptSem, 0, 0);
    if (rv < 0) {
        printf("Error: sem_init: %d\n", errno);
        goto error_before_login;
    }

    if (opt_username != NULL) {  // username supplied on command line
        username = opt_username;
    }
    else {
        printf("Enter username: ");
        std::cin >> username;
    }
    if (opt_password != NULL) {  // password supplied on command line
        password = opt_password;
    }
    else {
        printf("Enter password: ");
        std::cin >> password;
    }

    {
        ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
        // Login
        ccd::LoginInput loginRequest;
        loginRequest.set_player_index(0);
        loginRequest.set_user_name(username);
        loginRequest.set_password(password);
        ccd::LoginOutput loginResponse;
        printf("Logging in...\n");
        rv = CCDILogin(loginRequest, loginResponse);
        if (rv != CCD_OK) {
            printf("Error: CCDILogin: %d\n", rv);
            goto error_before_login;
        }
        userId = loginResponse.user_id();
        printf("Login complete.\n");

        // Link device
        ccd::LinkDeviceInput linkInput;
        linkInput.set_user_id(userId);
        char myname[1024];
        gethostname(myname, sizeof(myname));
        linkInput.set_device_name(myname);
        linkInput.set_device_has_camera(true);  ///////////// Needed to subscribe to CR Upload
        printf("Linking device %s\n", myname);
        rv = CCDILinkDevice(linkInput);
        if (rv != CCD_OK) {
            printf("Error: CCDILinkDevice: %d\n", rv);
            goto error_after_login;
        }

        if (!opt_dont_subscribe) {
            // Subscribe dataset
            int i;
            ccd::ListOwnedDatasetsInput ownedDatasetsInput;
            ownedDatasetsInput.set_user_id(userId);
            rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
            if (rv != CCD_OK) {
                printf("Error: CCDIListOwnedDatasets: %d\n", rv);
                goto error_after_link;
            }

            for (i = 0; i < ownedDatasetsOutput.dataset_details_size(); i++) {
                ccd::AddSyncSubscriptionInput addSyncSubInput;
                if(ownedDatasetsOutput.dataset_details(i).datasetname() != "CR Upload") {
                    continue;
                }
                u64 datasetId = ownedDatasetsOutput.dataset_details(i).datasetid(); 
                addSyncSubInput.set_user_id(userId);
                addSyncSubInput.set_dataset_id(datasetId);
                addSyncSubInput.set_subscription_type(ccd::SUBSCRIPTION_TYPE_PRODUCER);
                rv = CCDIAddSyncSubscription(addSyncSubInput);
                const static int ALREADY_SUBSCRIBED_ERR_CODE = -32228;
                if (rv == CCD_OK) {
                }else if(rv == ALREADY_SUBSCRIBED_ERR_CODE) {
                    printf("Ignoring %d which means dataset already subscribed (dset:"FMTu64")\n",
                           ALREADY_SUBSCRIBED_ERR_CODE, datasetId);
                }else {
                    printf("Error: CCDIAddSyncSubscription failed for dataset_id "FMTu64": %d\n", datasetId, rv);
                    goto error_after_link;
                }
            }
        }

        if (opt_exit_after_login) {
            return rv;
        }

        {
            int option = -1;
            while(option != 0){
                printf("\nEnter an option number:\n");
                printf("  0) Exit\n");
                printf("  1) Start Camera Upload\n");
                printf("  2) Stop Camera Upload.\n");
                printf("  3) Get CCDISyncState\n");

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
                    setCameraRollUploadEnable(userId, true);
                    break;
                case 2:
                    setCameraRollUploadEnable(userId, false);
                    break;
                case 3:
                    printCCDISyncState(userId);
                    break;
                default:
                    printf("Option %d not supported\n", option); 
                }
            }
        }

        if (!opt_dont_subscribe) {
            // Delete Subscribiption.
            int i;
            for (i = 0; i < ownedDatasetsOutput.dataset_details_size(); i++) {
                ccd::DeleteSyncSubscriptionsInput deleteSyncSubInput;
                if(ownedDatasetsOutput.dataset_details(i).datasetname() != "CR Upload") {
                    continue;
                }
                u64 datasetId = ownedDatasetsOutput.dataset_details(i).datasetid(); 
                deleteSyncSubInput.set_user_id(userId);
                deleteSyncSubInput.add_dataset_ids(datasetId);;
                CCDIDeleteSyncSubscriptions(deleteSyncSubInput);
            }
        }
error_after_link:
#if 0
        // don't unlink, as that would cause associated psn datasets to be lost
        ccd::UnlinkDeviceInput unlinkInput;
        printf("Unlinking device\n");
        unlinkInput.set_user_id(userId);
        rv = CCDIUnlinkDevice(unlinkInput);
        if (rv != CCD_OK) {
            printf("Error: CCDIUnlinkDevice: %d\n", rv);
            goto error_after_login;
        }
#else
        ;  // empty statement to keep label happy
#endif

    }

error_after_login:
    {
        ccd::LogoutInput logoutRequest;
        logoutRequest.set_local_user_id(userId);
        rv = CCDILogout(logoutRequest);
        if (rv != CCD_OK) {
            printf("Error: CCDILogout: %d\n", rv);
        }
    }
 error_before_login:
    return rv;
}
