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
#include <log.h>
#include <vpl_th.h>
#include <vpl_time.h>
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vpl_user.h>
#include <vplu_types.h>

#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>

static int subscribe(u64 userId, const char* dsName)
{
    int         rv = 0;
    int         i;
    s32         numDatasets; 
    int         retryCnt = 0;

    ccd::ListOwnedDatasetsInput ownedDatasetsInput;
    ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;

    LOG_INFO("Subscribing dataset[%s] as NORMAL type for user "FMT_VPLUser_Id_t"...", dsName, userId);
    do {
        ownedDatasetsInput.set_user_id(userId);
        rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIListOwnedDatasets: %d", rv);
            return -1;
        }

        LOG_INFO("Number of data sets %d", ownedDatasetsOutput.dataset_details_size());
        numDatasets = ownedDatasetsOutput.dataset_details_size();
        if (!numDatasets && (retryCnt++ < 5)) {
            VPLThread_Sleep(VPLTIME_FROM_SEC(1));
            LOG_INFO("Waiting for dataset update cnt(%d)", retryCnt);
        } else {
            break;
        }
    } while (1);


    for (i = 0; i < numDatasets; i++) {
        ccd::AddSyncSubscriptionInput addSyncSubInput;
        u64 datasetId = ownedDatasetsOutput.dataset_details(i).datasetid(); 
        std::string datasetName = ownedDatasetsOutput.dataset_details(i).datasetname(); 
        if (strncmp(datasetName.c_str(), dsName, datasetName.length()) == 0) {
            LOG_INFO("Subscribe to dataset id "FMTx64" [%s]", datasetId, datasetName.c_str());
            addSyncSubInput.set_user_id(userId);
            addSyncSubInput.set_dataset_id(datasetId);
            addSyncSubInput.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
            rv = CCDIAddSyncSubscription(addSyncSubInput);
            if (rv != CCD_OK) {
                LOG_ERROR("CCDIAddSyncSubscription failed for dataset_id "FMTu64": %d", datasetId, rv);
                return -1;
            }
        }
    }

    return 0;
}


#define USER_INFO_FILE                  "user_info"
#define DATASET_INFO_FILE               "dataset_info"

int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int         rv = 0;
    std::string machine;
    std::string testroot;
    std::string dsName;
    VPLFile_handle_t  userInfoFH = 0;
    std::string userInfoFile("");
    std::string datasetInfoFile("");
    ssize_t readSz; 

    if(argc != 4) {
        LOG_INFO("Usage: %s <machine> <test dir> <dataset name>\n", argv[0]);
        LOG_INFO("Example: %s my-desktop testroot iGware", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);
    testroot.assign(argv[2]);
    dsName.assign(argv[3]);

    u64 userId;

#ifdef WIN32
    if(testroot.size()>0 && testroot[0]=='/') {
        testroot = std::string("C:") + testroot;
    }
#endif

    userInfoFile.append(testroot.c_str());
    userInfoFile.append("/");
    userInfoFile.append(USER_INFO_FILE);

    userInfoFH = VPLFile_Open(userInfoFile.c_str(), VPLFILE_OPENFLAG_READWRITE, 0777);
    if (!VPLFile_IsValidHandle(userInfoFH)) {
        LOG_ERROR("Error opening in %s", userInfoFile.c_str());
        rv = -1;
        goto out;
    }

    readSz = VPLFile_Read(userInfoFH, &userId, sizeof(userId));
    if (readSz <= 0) {
        LOG_ERROR("Error reading user information in %s", userInfoFile.c_str());
        rv = -1;
        goto out;
    }

    rv = subscribe(userId, dsName.c_str());
    if(rv != 0) {
        LOG_ERROR("subscribe failed:%d", rv);
        goto out;
    }

out:
    if (VPLFile_IsValidHandle(userInfoFH))
        VPLFile_Close(userInfoFH);

    return rv;
}
