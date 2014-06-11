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
/// Calls CCDILogin with the specified username and password.
/// Or calls CCDILogout if no parameters.

#include "ccdi_test_common.hpp"

int main(int argc, char** argv)
{
    LOGInit("ccd_login", NULL);
    LOGSetMax(0); // No limit
    int rv;
    {
        rv = CCDITest_SetupCcdiTest();
        if (rv != 0) {
            LOG_ERROR("CCDITest_SetupCcdiTest failed: %d", rv);
            goto out;
        }
        if (argc < 2) {
            LOG_INFO("Logging out");
            ccd::LogoutInput req;
            rv = CCDILogout(req);
            if (rv != 0) {
                LOG_ERROR("CCDILogout failed: %d", rv);
                goto out;
            }
        } else {
            ccd::LoginInput req;
            req.set_user_name(argv[1]);
            LOG_INFO("Logging in as %s", argv[1]);
            if (argc > 2) {
                req.set_password(argv[2]);
            } else {
                LOG_ERROR("CCDILogin requires password");
                goto out;
            }
            ccd::LoginOutput resp;
            rv = CCDILogin(req, resp);
            if (rv != 0) {
                LOG_ERROR("CCDILogin failed: %d", rv);
                goto out;
            } else {
                LOG_INFO("Result: %s", resp.DebugString().c_str());
            }
        }
    }
out:
    // Since the return code usually ends up "mod 256", remap it to avoid ever returning 0 when
    // something actually failed.
    return (rv == 0) ? 0 : 255;
}
