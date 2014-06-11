//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <log.h>
#include <vpl_th.h>
#include <vpl_time.h>

#define SYNC_TIMEOUT    300

int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int rv;
    int opt_ccd_instance = 0;
    std::string datasetName;
    u64 datasetId = 0;

    if((argc != 2) && (argc != 3)) {
        LOG_INFO("Usage: %s <datasetName> <instanceNum>\n", argv[0]);
        LOG_INFO("  Example datasetName: clear.fi, My\\ Cloud, CR\\ Upload, CameraRoll");
        LOG_INFO("Example: %s ", argv[0]);
        return -1;
    }

    datasetName.assign(argv[1]);

    if(argc == 3) {
        opt_ccd_instance = atoi(argv[2]);
        CCDIClient_SetTestInstanceNum(opt_ccd_instance);
    }

    ccd::ListSyncSubscriptionsInput listSyncSubscriptionsInput;
    listSyncSubscriptionsInput.set_only_use_cache(true);
    ccd::ListSyncSubscriptionsOutput listSyncSubscriptionsOutput;
    rv = CCDIListSyncSubscriptions(listSyncSubscriptionsInput,
                                   listSyncSubscriptionsOutput);
    if(rv != CCD_OK) {
        LOG_ERROR("CCDIListSyncSubscriptions:%d", rv);
        return rv;
    }
    for(int subIndex=0; subIndex < listSyncSubscriptionsOutput.subs_size(); ++subIndex) {
        const ccd::SyncSubscriptionDetail & sub = listSyncSubscriptionsOutput.subs(subIndex);
        if(sub.dataset_details().datasetname() == datasetName) {
            datasetId = sub.dataset_details().datasetid();
            break;
        }
    }

    if(datasetId == 0) {
        LOG_ERROR("Dataset name specified not subscribed: %s", datasetName.c_str());
        return -3;
    }

    int numLoops=1;
    // wait for sync state to be in-sync
    while(true) {
        int rc;
        ccd::GetSyncStateInput request;
        request.add_get_sync_states_for_datasets(datasetId);
        ccd::GetSyncStateOutput response;
        int rv;
        rc = CCDIGetSyncState(request, response);
        if (rc != CCD_OK) {
            LOG_ERROR("CCDIGetSyncState: %d", rc);
            rv = rc;
            return rv;
        }

        if(response.dataset_sync_state_summary(0).status() == ccd::CCD_SYNC_STATE_IN_SYNC)
        {
            LOG_INFO("Sync state in sync");
            break;
        }

        if((numLoops%15) == 0) {
            LOG_WARN("Still syncing: numLoops=%d, lastStatus=%d:",
                     numLoops, response.dataset_sync_state_summary(0).status());
            if (numLoops > SYNC_TIMEOUT) {
                LOG_ERROR("Fail to reach in sync state after %d sec", numLoops);
                rv = -1;
                goto exit;
            }
        }
        numLoops++;
        VPLThread_Sleep(VPLTIME_FROM_SEC(1));

    }

exit:
    return rv;
}
