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


/// Test access to read/write datasets.
int test_vssi_dataset_access(u64 user_id,
                             u64 dataset_id,
                             const VSSI_RouteInfo& route_info,
                             int num_dirs,
                             int num_files,
                             int num_levels,
                             int file_size,
                             bool do_create,
                             const std::string& root_dir,
                             bool open_only);


#endif //include guard
