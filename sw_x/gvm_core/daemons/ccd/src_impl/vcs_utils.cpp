/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include <vpl_types.h>
#include <cJSON2.h>
#include <iostream>
#include <sstream>

#include <vpl_plat.h>
#include <log.h>

#include "vcs_utils.hpp"
#include "vcs_util.hpp"
#include "cache.h"

using namespace std;

int VCS_getOwnedDatasets(u64 userId, ccd::ListOwnedDatasetsOutput &listOfDatasets)
{
    int err = 0;

    // First, try to get it from the cache without contacting infra.
    err = Cache_ListOwnedDatasets(userId, listOfDatasets, true);  // true == check cache only
    if (!err) return err;

    // Second, allow infra contact if necessary.
    err = Cache_ListOwnedDatasets(userId, listOfDatasets, false);  // false == allow infra contact
    if (err) {
        LOG_ERROR("Failed to get list of owned datasets: uid="FMTu64" err=%d", userId, err);
    }
    return err;
}

int VCS_getDatasetDetail(u64 userId, const std::string &datasetName, vplex::vsDirectory::DatasetDetail &datasetDetail)
{
    int err = 0;

    ccd::ListOwnedDatasetsOutput listOfDatasets;
    err = VCS_getOwnedDatasets(userId, listOfDatasets);
    if (err) return err;

    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;
    for (it = listOfDatasets.dataset_details().begin(); it != listOfDatasets.dataset_details().end(); it++) {
        if(it->datasetname() == datasetName){
            datasetDetail = *it;
            return err;
        }
    }
    return CCD_ERROR_NOT_FOUND;
}

int VCS_getDatasetDetail(u64 userId, u64 datasetId, vplex::vsDirectory::DatasetDetail &datasetDetail)
{
    int err = 0;

    ccd::ListOwnedDatasetsOutput listOfDatasets;
    err = VCS_getOwnedDatasets(userId, listOfDatasets);
    if (err) return err;

    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;
    for (it = listOfDatasets.dataset_details().begin(); it != listOfDatasets.dataset_details().end(); it++) {
        if(it->datasetid() == datasetId){
            datasetDetail = *it;
            return err;
        }
    }
    return CCD_ERROR_NOT_FOUND;
}

std::string VCS_getServer(u64 userId, const std::string &datasetName)
{
    int err = 0;

    vplex::vsDirectory::DatasetDetail datasetDetail;
    err = VCS_getDatasetDetail(userId, datasetName, datasetDetail);
    if (err) {
        LOG_ERROR("Failed to find dataset: name=%s err=%d", datasetName.c_str(), err);
        return "";
    }

    stringstream ss;
    ss << datasetDetail.storageclusterhostname() << ":" << datasetDetail.storageclusterport();
    return ss.str();
}

std::string VCS_getServer(u64 userId, u64 datasetId)
{
    int err = 0;

    vplex::vsDirectory::DatasetDetail datasetDetail;
    err = VCS_getDatasetDetail(userId, datasetId, datasetDetail);
    if (err) {
        LOG_ERROR("Failed to find dataset: id="FMTu64" err=%d", datasetId, err);
        return "";
    }

    stringstream ss;
    ss << datasetDetail.storageclusterhostname() << ":" << datasetDetail.storageclusterport();
    return ss.str();
}

int VCS_getDatasetID(u64 uid,
                     const std::string &datasetName,
                     u64 &datasetId_out)
{
    int err = 0;
    datasetId_out = 0;

    vplex::vsDirectory::DatasetDetail datasetDetail;
    err = VCS_getDatasetDetail(uid, datasetName, datasetDetail);
    if (err) {
        LOG_ERROR("Failed to find dataset: name=%s err=%d", datasetName.c_str(), err);
        return err;
    }

    if ( datasetDetail.suspendedflag() ) {
        LOG_WARN("Dataset(%s,"FMTu64") suspended.",
                  datasetDetail.datasetname().c_str(), datasetDetail.datasetid());
        err = CCD_ERROR_DATASET_SUSPENDED;
    }

    datasetId_out = datasetDetail.datasetid();

    return err;
}
