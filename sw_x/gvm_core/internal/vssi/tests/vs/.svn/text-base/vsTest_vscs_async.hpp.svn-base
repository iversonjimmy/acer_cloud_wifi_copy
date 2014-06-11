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

/// Virtual Storage Server Interface Test

#ifndef __VSTEST_VSCS_ASYNC_H__
#define __VSTEST_VSCS_ASYNC_H__

#include "vplex_vs_directory.h"
#include <string>


/// Test access to read-only contents.
int test_vssi_content_access(const std::string& location,
                             bool longTest);

/// Test access to read/write datasets.
int test_vssi_dataset_access(const std::string& location,
                             u64 user_id,
                             u64 dataset_id,
                             const VSSI_RouteInfo& route_info,
                             bool use_xml_api,
                             vplex::vsDirectory::DatasetType type);

int test_vss_session_recognition(const vplex::vsDirectory::SessionInfo& loginSession,
                                 const std::string& location,
                                 u64 user_id,
                                 u64 dataset_id,
                                 const VSSI_RouteInfo& route_info,
                                 bool use_xml_api);

/// Test CameraRoll feature
int test_camera_roll(const std::string& upload_location,
                     const std::string& download_location,
                     u64 user_id,
                     u64 upload_dataset_id,
                     u64 download_dataset_id,
                     const VSSI_RouteInfo& route_info,
                     bool use_xml_api);

#endif //include guard
