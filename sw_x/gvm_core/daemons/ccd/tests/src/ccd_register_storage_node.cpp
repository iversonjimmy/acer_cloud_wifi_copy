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
/// Register or unregister the local device as a storage node.

#include "ccdi_test_common.hpp"

int main(int argc, char** argv)
{
    LOGInit("ccd_register_storage_node", NULL);
    LOGSetMax(0);
    int rv;
    {
        rv = CCDITest_SetupCcdiTest();
        if (rv != 0) {
            LOG_ERROR("CCDITest_SetupCcdiTest failed: %d", rv);
            goto out;
        }
        u64 userId = 0;
        {
            ccd::GetSystemStateInput req;
            req.set_get_players(true);
            ccd::GetSystemStateOutput resp;
            rv = CCDIGetSystemState(req, resp);
            if (rv != 0) {
                LOG_ERROR("CCDIGetSystemState failed: %d", rv);
                goto out;
            } else {
                userId = resp.players().players(0).user_id();
            }
        }
        if (argc >= 2) {
            LOG_INFO("Registering StorageNode");
            ccd::RegisterStorageNodeInput req;
            req.set_user_id(userId);
            rv = CCDIRegisterStorageNode(req);
            if (rv != 0) {
                LOG_ERROR("CCDIRegisterStorageNode failed: %d", rv);
                goto out;
            }
        } else {
            LOG_INFO("Unregistering StorageNode");
            ccd::UnregisterStorageNodeInput req;
            req.set_user_id(userId);
            rv = CCDIUnregisterStorageNode(req);
            if (rv != 0) {
                LOG_ERROR("CCDIUnregisterStorageNode failed: %d", rv);
                goto out;
            }
        }
    }
out:
    // Since the return code usually ends up "mod 256", remap it to avoid ever returning 0 when
    // something actually failed.
    return (rv == 0) ? 0 : 255;
}
