//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

//============================================================================
/// @file
/// Command-line example for logging out of the Cloud Client.
///
/// Prerequisites:
/// - The Cloud Client process (ccd) must be running.
//============================================================================

#include "ccdi.hpp"
#include <stdio.h>

#define LOG_INFO(fmt_, ...)  printf("line %d: "fmt_"\n", __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(fmt_, ...)  printf("***line %d: "fmt_"\n", __LINE__, ##__VA_ARGS__)

int main(int argc, char ** argv)
{
    if (argc > 1) {
        LOG_ERROR("Wrong number of arguments, exiting.");
        fprintf(stderr,
                "\n"
                "Usage:\n"
                "  %s\n"
                "Example:\n"
                "  %s\n\n", argv[0], argv[0]);
        return -1;
    }
    
    LOG_INFO("Calling CCDILogout");
    {
        ccd::LogoutInput input;
        int rv = CCDILogout(input);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDILogout failed: %d", rv);
            return -1;
        }
        LOG_INFO("Logout success.");
    }
    return 0;
}
