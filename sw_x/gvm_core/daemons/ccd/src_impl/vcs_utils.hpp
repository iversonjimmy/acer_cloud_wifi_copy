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

#ifndef __VCS_UTILS_HPP__
#define __VCS_UTILS_HPP__

#include <vplu_types.h>
#include <vpl_plat.h>

#include <string>
#include <cJSON2.h>

#include "cache.h"

std::string VCS_getServer(u64 userId, const std::string& datasetName);
std::string VCS_getServer(u64 userId, u64 datasetId);
int VCS_getDatasetID(u64 uid, const std::string& datasetName, u64& datasetId_out);

int VCS_getOwnedDatasets(u64 userId, ccd::ListOwnedDatasetsOutput& listOfDatasets_out);
int VCS_getDatasetDetail(u64 userId, const std::string& datasetName, vplex::vsDirectory::DatasetDetail& datasetDetail_out);
int VCS_getDatasetDetail(u64 userId, u64 datasetId, vplex::vsDirectory::DatasetDetail& datasetDetail_out);

#endif // include guard
