//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#define _CRT_SECURE_NO_WARNINGS

#include "ccdi_test_common.hpp"

int CCDITest_SetupCcdiTest()
{
    char const* env_str;
    if ((env_str = getenv(CCD_TEST_INSTANCE_ID)) != NULL) {
        LOG_INFO("Read "CCD_TEST_INSTANCE_ID" (%s)", env_str);
        int testInstanceNum = strtol(env_str, NULL, 0);
        CCDIClient_SetTestInstanceNum(testInstanceNum);
    } else {
        LOG_INFO(CCD_TEST_INSTANCE_ID" was not specified; using default");
    }
    return 0;
}
