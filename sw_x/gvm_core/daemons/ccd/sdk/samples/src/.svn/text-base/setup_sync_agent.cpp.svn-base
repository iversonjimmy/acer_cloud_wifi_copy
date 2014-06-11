//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

//============================================================================
/// @file
/// Command-line example for using the Sync Agent features of the Cloud Client.
///
/// Given a user name and password, it performs login.
/// If a device name is provided, it attempts to link the device and subscribe
/// to all datasets.
///
/// Prerequisites:
/// - The Cloud Client process (ccd) must be running.
//============================================================================

#include "ccdi.hpp"
#include <stdio.h>

#define LOG_INFO(fmt_, ...)  printf("line %d: "fmt_"\n", __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(fmt_, ...)  printf("***line %d: "fmt_"\n", __LINE__, ##__VA_ARGS__)

int main(int argc, char ** argv)
{
    if(argc < 3 || argc > 4) {
        LOG_ERROR("Wrong number of arguments, exiting.");
        fprintf(stderr,
                "\n"
                "Usage:\n"
                "  %s <username> <password> [device_name]\n"
                "Example:\n"
                "  %s testuser password \"My PC\"\n\n", argv[0], argv[0]);
        return -1;
    }
    
    std::string username = argv[1];
    std::string password = argv[2];
    bool attemptToLink = (argc >= 4);

    LOG_INFO("Calling CCDIGetSystemState");
    {
        ccd::GetSystemStateInput input;
        input.set_get_players(true);
        ccd::GetSystemStateOutput output;
        int rv = CCDIGetSystemState(input, output);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIGetSystemState failed: %d", rv);
            return -1;
        }
        if (output.players().players(0).user_id() != 0) {
            LOG_ERROR("\"%s\" is already logged-in; please run the Logout example first.",
                    output.players().players(0).username().c_str());
            return -1;
        }
    }

    uint64_t userId;
    LOG_INFO("Calling CCDILogin");
    {
        ccd::LoginInput input;
        input.set_user_name(username);
        input.set_password(password);
        ccd::LoginOutput output;
        int rv = CCDILogin(input, output);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDILogin failed: %d", rv);
            return -1;
        }
        userId = output.user_id();
        LOG_INFO("Login success; userId=%"PRIx64, userId);
    }

    LOG_INFO("Calling CCDIGetSyncState");
    {
        ccd::GetSyncStateInput input;
        input.set_get_device_name(true);
        ccd::GetSyncStateOutput output;
        int rv = CCDIGetSyncState(input, output);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIGetSyncState failed: %d", rv);
            return -1;
        }
        LOG_INFO("CCDIGetSyncState success; state=%s", output.DebugString().c_str());

        if (output.is_device_linked() == attemptToLink) {
            if (attemptToLink) {
                LOG_ERROR("Device is already linked for this user.");
                return -1;
            } else {
                LOG_INFO("Device is not yet linked for this user; pass a device_name to link it.");
            }
        }
    }

    if (attemptToLink) {
        std::string deviceName = argv[3];
        LOG_INFO("Calling CCDILinkDevice");
        {
            ccd::LinkDeviceInput input;
            input.set_user_id(userId);
            input.set_device_name(deviceName);
            int rv = CCDILinkDevice(input);
            if (rv != CCD_OK) {
                LOG_ERROR("CCDILinkDevice failed: %d", rv);
                return -1;
            }
        }

        LOG_INFO("Calling CCDIListOwnedDatasets");
        ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
        {
            ccd::ListOwnedDatasetsInput input;
            input.set_user_id(userId);
            int rv = CCDIListOwnedDatasets(input, ownedDatasetsOutput);
            if (rv != CCD_OK) {
                LOG_ERROR("CCDIListOwnedDatasets failed: %d", rv);
                return -1;
            }
        }

        for (int i = 0; i < ownedDatasetsOutput.dataset_details_size(); i++) {
            const vplex::vsDirectory::DatasetDetail& currDataset = ownedDatasetsOutput.dataset_details(i);
            LOG_INFO("Calling CCDIAddSyncSubscription for dataset[%d]; %s", i, currDataset.DebugString().c_str());
            ccd::AddSyncSubscriptionInput input;
            input.set_user_id(userId);
            input.set_dataset_id(currDataset.datasetid());
            input.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
            int rv = CCDIAddSyncSubscription(input);
            if (rv != CCD_OK) {
                LOG_ERROR("CCDIAddSyncSubscription for dataset[%d] failed: %d", i, rv);
                continue;
            }
            LOG_INFO("CCDIAddSyncSubscription for dataset[%d] succeeded", i);
        }
    }

    LOG_INFO("Done.  The user will remain logged-in after this example program exits.");

    return 0;
}
