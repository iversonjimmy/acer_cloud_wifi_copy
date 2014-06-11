//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

/// @file
/// Issues a CCD shutdown request.

#include "ccdi_test_common.hpp"

int main(int argc, char** argv)
{
    LOGInit("ccd_shutdown", NULL);
    LOGSetMax(0);

    int rv;
    {
        rv = CCDITest_SetupCcdiTest();
        if (rv != 0) {
            LOG_ERROR("CCDITest_SetupCcdiTest failed: %d", rv);
            goto out;
        }
        LOG_INFO("Requesting shutdown...");
        ccd::UpdateSystemStateInput request;
        request.set_do_shutdown(true);
        ccd::UpdateSystemStateOutput response;
        int rv = CCDIUpdateSystemState(request, response);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIUpdateSystemState failed: %d", rv);
        }
    }
out:
    // Since the return code usually ends up "mod 256", remap it to avoid ever returning 0 when
    // something actually failed.
    return (rv == 0) ? 0 : 255;
}
