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
                             bool run_cloudnode_tests,
                             bool run_async_client_object_test,
                             bool run_file_access_and_share_modes_test,
                             bool run_many_file_open_test,
                             bool run_case_insensitivity_test);

/// Test CameraRoll feature
int test_camera_roll(const std::string& upload_location,
                     const std::string& download_location,
                     u64 user_id,
                     u64 upload_dataset_id,
                     u64 download_dataset_id,
                     bool use_xml_api);

/// Test Database Error Handling
int test_db_error_handling(
                           u64 user_id,
                           u64 dataset_id,
                           s32 db_err_hand_param,
                           const std::string& db_dir);


#endif //include guard
