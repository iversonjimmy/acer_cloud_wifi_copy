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
/// Waits for the CCD process to become ready, or times out after the number of seconds specified
/// on the command line (or 30 seconds by default).

#include "ccdi_test_common.hpp"

int main(int argc, char** argv)
{
    LOGInit("ccd_ready", NULL);
    LOGSetMax(0);
    int rv;
    {
        rv = CCDITest_SetupCcdiTest();
        if (rv != 0) {
            LOG_ERROR("CCDITest_SetupCcdiTest failed: %d", rv);
            goto out;
        }
        VPLTime_t timeout = VPLTIME_FROM_SEC(30);
        if (argc > 1) {
            // Parse argv[1] as number of seconds to wait before timeout.
            int arg = atoi(argv[1]);
            timeout = VPLTIME_FROM_SEC(arg);
        }
        LOG_INFO("Will wait up to "FMTu64"ms for CCD to be ready...", VPLTIME_TO_MILLISEC(timeout));
        VPLTime_t endTime = VPLTime_GetTimeStamp() + timeout;
        
        ccd::GetSystemStateInput request;
        request.set_get_players(true);
        ccd::GetSystemStateOutput response;
        int rv;
        while(1) {
            rv = CCDIGetSystemState(request, response);
            if (rv == CCD_OK) {
                LOG_INFO("CCD is ready!");
                break;
            }
            if ((rv != IPC_ERROR_SOCKET_CONNECT) && (rv != VPL_ERR_NAMED_SOCKET_NOT_EXIST)) {
                LOG_ERROR("Unexpected error: %d", rv);
                break;
            }
            LOG_INFO("Still waiting for CCD to be ready...");
            if (VPLTime_GetTimeStamp() >= endTime) {
                LOG_ERROR("Timed out after "FMTu64"ms", VPLTIME_TO_MILLISEC(timeout));
                break;
            }
            VPLThread_Sleep(VPLTIME_FROM_MILLISEC(500));
        }
    }
out:
    // Since the return code usually ends up "mod 256", remap it to avoid ever returning 0 when
    // something actually failed.
    return (rv == 0) ? 0 : 255;
}
