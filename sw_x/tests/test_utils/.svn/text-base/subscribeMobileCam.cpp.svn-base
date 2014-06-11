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

static int subscribeMobileCam(u64 userId, const char* camName)
{
    int         rv = 0;
    int         i;
    int         datasetExists = 0;
    s32         numDatasets; 
    u64         datasetId = 0;
    std::string datasetName; 

    LOG_INFO("Subscribing to dataset[%s] as CAMERA type for user "FMTu64"...", camName, userId);
    ccd::ListOwnedDatasetsInput ownedDatasetsInput;
    ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
    ownedDatasetsInput.set_user_id(userId);
    rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIListOwnedDatasets: %d\n", rv);
        return -1;
    }

    LOG_INFO("Number of data sets %d", ownedDatasetsOutput.dataset_details_size());
    numDatasets = ownedDatasetsOutput.dataset_details_size();

    for (i = 0; i < numDatasets; i++) {
        std::string datasetName = ownedDatasetsOutput.dataset_details(i).datasetname(); 
        if (strncmp(datasetName.c_str(), camName, datasetName.length()) == 0) {
            datasetExists = 1;
            datasetId = ownedDatasetsOutput.dataset_details(i).datasetid(); 
            LOG_INFO("dataset[%s] exists", camName);
            break;
        }
    }

    if (!datasetExists) {
        LOG_INFO("dataset[%s] does not exist, add one", camName);
        ccd::AddDatasetInput addDatasetInput;
        ccd::AddDatasetOutput addDatasetOutput;

        addDatasetInput.set_user_id(userId);
        addDatasetInput.set_dataset_name(camName);
        addDatasetInput.set_dataset_type(ccd::NEW_DATASET_TYPE_CAMERA);

        rv = CCDIAddDataset(addDatasetInput, addDatasetOutput); 
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIAddDataset[%s]: rv(%d)\n", camName, rv);
            return -1;
        }

        datasetId = addDatasetOutput.dataset_id();
    }

    ccd::AddSyncSubscriptionInput addSyncSubInput;
    LOG_INFO("Subscribe to dataset id "FMTx64" [%s]", datasetId, camName);
    addSyncSubInput.set_user_id(userId);
    addSyncSubInput.set_dataset_id(datasetId);
    addSyncSubInput.set_subscription_type(ccd::SUBSCRIPTION_TYPE_CAMERA);
    rv = CCDIAddSyncSubscription(addSyncSubInput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIAddSyncSubscription failed for dataset_id "FMTx64": %d\n", datasetId, rv);
        return -1;
    }

    return 0;
}


#define USER_INFO_FILE                  "user_info"

int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int         rv = 0;
    std::string machine;
    std::string testroot;
    std::string camName;
    VPLFile_handle_t  userInfoFH = 0;
    std::string userInfoFile("");
    std::string datasetInfoFile("");
    ssize_t readSz; 

    if(argc != 4) {
        LOG_INFO("Usage: %s <machine> <test dir> <cameraName>\n", argv[0]);
        LOG_INFO("Example: %s my-desktop testroot mobileCam1 1", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);
    testroot.assign(argv[2]);
    camName.assign(argv[3]);

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

    rv = subscribeMobileCam(userId, camName.c_str());
    if (rv != 0) {
        LOG_ERROR("subscribe failed:%d", rv);
        goto out;
    }

out:
    if (VPLFile_IsValidHandle(userInfoFH))
        VPLFile_Close(userInfoFH);

    return rv;
}
