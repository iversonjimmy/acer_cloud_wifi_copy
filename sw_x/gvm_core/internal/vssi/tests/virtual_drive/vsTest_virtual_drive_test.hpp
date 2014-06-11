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

#ifndef __VSTEST_VIRTUAL_DRIVE_TEST_H__
#define __VSTEST_VIRTUAL_DRIVE_TEST_H__

#include "vplex_vs_directory.h"
#include <string>

/// Test access to read/write virtual drive.
int test_virtual_drive_access(const u64 userId,
                              const u64 datasetId,
                              const u64 deviceId,
                              const u64 handle,
                              const std::string& ticket,
                              const VSSI_RouteInfo& vssRouteInfo);

#endif //include guard
