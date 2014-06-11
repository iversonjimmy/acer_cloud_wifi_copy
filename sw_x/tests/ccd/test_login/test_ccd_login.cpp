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
static bool opt_display_sync_status = false;
static bool opt_display_notifications = false;

static void
sigintHandler(int sig)
{
    sem_post(&interruptSem);
}

static void
usage(char *argv[])
{
    printf("Usage: %s [-h] [-u username] [-p password] [-n]\n", argv[0]);
    printf("    -u, --username  Username. [ask via stdin]\n");
    printf("    -p, --password  Password. [ask via stdin]\n");
    printf("    ------------ Other options --------------\n");
    printf("    -d, --display-sync-status  Displays sync status, calls CCDIGetSyncState:get_sync_states_for_datasets\n");
    printf("    -e, --exit-after-login  Exit after logging in. [wait for SIGINT to exit]\n");
    printf("    -f, --display-notifications  Creates and queries notification queue CCDIEventsDequeue\n");
    printf("    -n, --dont-subscribe  Don't subscribe to any of the datasets. [auto subscribe]\n");
    printf("    -h, --help  Show this message.\n");
}

static void
parse_args(int argc, char *argv[])
{
    static struct option long_options[] = {
        { "username", required_argument, 0, 'u' },
        { "password", required_argument, 0, 'p' },
        { "display-sync-status", no_argument, 0, 'd'},
        { "exit-after-login", no_argument, 0, 'e' },
        { "display-notifications", no_argument, 0, 'f'},
        { "my-cloud-only", no_argument, 0, 'm' },
        { "dont-subscribe", no_argument, 0, 'n' },
        { "pause-sync", no_argument, 0, 's' },
        { "help", no_argument, 0, 'h'},
        { 0, 0, 0, 0 }
    };

    for (;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "u:p:defmnsh", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 'u':
            opt_username = optarg;
            break;
        case 'd':
            opt_display_sync_status = true;
            break;
        case 'e':
            opt_exit_after_login = true;
            break;
        case 'f':
            opt_display_notifications = true;
            break;
        case 'p':
            opt_password = optarg;
            break;
        case 'n':
            opt_dont_subscribe = true;
            break;
        case 's':
            opt_pause_sync = true;
            LOG_ERROR("opt_pause_sync has been removed.");
            FAILED_ASSERT("opt_pause_sync has been removed.");
            break;
        case 'h':
        default:
            usage(argv);
            exit(1);
            return;
        }
    }
}

int
main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);
    
    std::string username, password;
    int rv;
    u64 userId;
    u64 queue_handle = 0;

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
        u64 cloudDsetId = 0;

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

                u64 datasetId = ownedDatasetsOutput.dataset_details(i).datasetid();
                addSyncSubInput.set_user_id(userId);
                addSyncSubInput.set_dataset_id(datasetId);
                addSyncSubInput.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
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

        if(opt_display_notifications) {
            ccd::EventsCreateQueueInput request;
            ccd::EventsCreateQueueOutput response;
            int rc = CCDIEventsCreateQueue(request, response);
            if(rc != 0) {
                LOG_ERROR("CCDIEventsCreateQueue:%d", rc);
            }
            queue_handle = response.queue_handle();
            LOG_INFO("QueueHandle:"FMTx64, queue_handle);
        }

        (void)signal(SIGINT, sigintHandler);
        while (sem_trywait(&interruptSem)) {
            VPLThread_Sleep(VPLTime_FromSec(2));
            static int seconds = 0;
            printf("%s logged in to ccd, %d sec elapsed. Send SIGINT to quit.  (Control-c)\n",
                   username.c_str(), seconds);
            seconds += 2;
            if(opt_display_sync_status) {
                ccd::GetSyncStateInput request;
                ccd::GetSyncStateOutput response;
                request.add_get_sync_states_for_datasets(cloudDsetId);
                int rc = CCDIGetSyncState(request, response);
                if(rc != 0) {
                    LOG_ERROR("CCDIGetSyncState for cloud dset:%d", rc);
                }
                for(int i=0; i<response.dataset_sync_state_summary_size(); i++) {
                    const ccd::DatasetSyncStateSummary& summary =
                            response.dataset_sync_state_summary(i);
                    LOG_INFO("SyncStats(%d): status:%d, "
                             "pending_files_download:%d, "
                             "pending_files_upload:%d, "
                             "total_files_download:%d, "
                             "total_files_upload:%d",
                             i, summary.status(),
                             summary.pending_files_download(),
                             summary.pending_files_upload(),
                             summary.total_files_downloaded(),
                             summary.total_files_uploaded());
                }
            }
            if(opt_display_notifications) {
                ccd::EventsDequeueInput request;
                ccd::EventsDequeueOutput response;
                request.set_queue_handle(queue_handle);
                int rc = CCDIEventsDequeue(request, response);
                if(rc != 0) {
                    LOG_ERROR("CCDIEventsDequeue:%d", rc);
                }
                for(int eventIndex = 0; eventIndex < response.events_size(); ++eventIndex) {
                    const ccd::CcdiEvent &event = response.events(eventIndex);
                    std::string eventStr;
                    if(event.has_device_connection_change()) {
                        eventStr += std::string(" DeviceConnectionChange:");
                    }
                    if(event.has_device_info_change()) {
                        eventStr += std::string(" DeviceConnectionChange:");
                    }
                    if(event.has_doc_save_and_go_completion()) {
                        eventStr += std::string(" DocSaveAndGoCompletion:");
                    }
                    if(event.has_sw_update_progress()) {
                        eventStr += std::string(" SwUpdateProgress:");
                    }
                    if(event.has_user_login()) {
                        eventStr += std::string(" UserLogin:");
                    }
                    if(event.has_user_logout()) {
                        eventStr += std::string(" UserLogout:");
                    }
                    LOG_INFO("Notification Received: %s", eventStr.c_str());
                }
            }
        }
        printf("Received SIGINT.  Exiting.\n");

        if(opt_display_notifications) {
            ccd::EventsDestroyQueueInput request;
            request.set_queue_handle(queue_handle);
            int rc = CCDIEventsDestroyQueue(request);
            if(rc != 0) {
                LOG_ERROR("CCDIEventsDestroyQueue:%d", rc);
            }
        }

        if (!opt_dont_subscribe) {
            // Delete Subscribiption.
            int i;
            for (i = 0; i < ownedDatasetsOutput.dataset_details_size(); i++) {
                ccd::DeleteSyncSubscriptionsInput deleteSyncSubInput;
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
