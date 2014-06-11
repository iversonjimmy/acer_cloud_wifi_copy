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

/// CameraRoll feature test

#ifndef __VSTEST_CAMERA_ROLL_HPP__
#define __VSTEST_CAMERA_ROLL_HPP__

#include "vssi.h"

int test_camera_roll_feature(VSSI_Session session,
                             const char* upload_location,
                             const char* download_location,
                             u64 user_id,
                             u64 upload_dataset_id,
                             u64 download_dataset_id,
                             const VSSI_RouteInfo& route_info,
                             bool use_xml_api);


#endif //include guard
