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
/// Tool to register storage node.
//============================================================================

#include <vplu.h>
#include <unistd.h>
#include "ccdi.hpp"
#include "log.h"

#include "LoginCCD.hpp"

using namespace std;

class RegisterStorageNodeCCD : public LoginCCD {
public:
    int registerStorageNode();
    int unregisterStorageNode();
};

int RegisterStorageNodeCCD::registerStorageNode()
{
    int rv = 0;
    u64 deviceId;
    bool localPsnAlreadyRegistered = false;

    {
        ccd::GetSystemStateInput req;
        ccd::GetSystemStateOutput res;
        req.set_get_device_id(true);
        rv = CCDIGetSystemState(req, res);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIGetSystemState failed (%d): Could not determine device ID", rv);
            goto end;
        }
        deviceId = res.device_id();
        LOG_INFO("Local device ID is "FMTx64, deviceId);
    }

    {
        ccd::GetPersonalCloudStateInput req;
        req.set_user_id(userId);
        req.set_list_storage_nodes(true);
        ccd::GetPersonalCloudStateOutput res;
        rv = CCDIGetPersonalCloudState(req, res);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIGetPersonalCloudState failed (%d)", rv);
            goto end;
        }
        LOG_INFO("Found %d PSN(s)", res.storage_nodes_size());
        for (int i = 0; i < res.storage_nodes_size(); i++) {
            const ccd::StorageNodeInfo &info = res.storage_nodes(i);
            LOG_INFO("Found PSN id="FMTx64" name=%s", info.device_id(), info.storage_name().c_str());
            if (info.device_id() == deviceId) {
                LOG_INFO("PSN id="FMTx64" is local device", info.device_id());
                localPsnAlreadyRegistered = true;
            }
            else {
                LOG_INFO("PSN id="FMTx64" is not local device: Unlinking this PSN", info.device_id());
                ccd::UnlinkDeviceInput req;
                req.set_user_id(userId);
                req.set_device_id(info.device_id());
                rv = CCDIUnlinkDevice(req);
                if (rv != CCD_OK) {
                    LOG_ERROR("CCDIUnlinkDevice failed (%d): Failed to unlink PSN "FMTx64, rv, info.device_id());
                    goto end;
                }
                LOG_INFO("PSN id="FMTx64" unlinked", info.device_id());
            }
        }
    }

    if (localPsnAlreadyRegistered) {
        LOG_INFO("Local device "FMTx64" already registered as PSN", deviceId);
    }
    else {
        LOG_INFO("Registering local device "FMTx64" as PSN", deviceId);
        ccd::RegisterStorageNodeInput req;
        req.set_user_id(userId);
        rv = CCDIRegisterStorageNode(req);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIRegisterStorageNode failed (%d)", rv);
            goto end;
        }
        LOG_INFO("Local device registered as PSN");
    }

 end:
    return rv;
}

int RegisterStorageNodeCCD::unregisterStorageNode()
{
    int rv = 0;

    LOG_INFO("Unregistering storage node...");

    ccd::UnregisterStorageNodeInput req;
    req.set_user_id(userId);
    rv = CCDIUnregisterStorageNode(req);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to unregister storage node: %d", rv);
        goto end;
    }
    LOG_INFO("Storage node unregistered");

 end:
    return rv;
}

static void print_usage_and_exit(const char *progname)
{
    fprintf(stderr, 
            "Usage: %s [-u] username password\n"
            "          -u = unregister (instead of register)\n", 
            progname);
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    string username;
    string password;
    bool doRegister;
private:
    Args();  // hide default constructor
};

Args::Args(int argc, char *argv[])
{
    if (argc == 1) {
        print_usage_and_exit(argv[0]);
    }

    // default value
    doRegister = true;

    int opt;
    while ((opt = getopt(argc, argv, "uh")) != -1) {
        switch (opt) {
        case 'u':
            doRegister = false;
            break;
        case 'h':
            print_usage_and_exit(argv[0]);
            break;
        default:
            fprintf(stderr, "ERROR: unknown option `%c' ignored\n", opt);
        }
    }

    if (optind >= argc) print_usage_and_exit(argv[0]);
    username = argv[optind++];
    if (optind >= argc) print_usage_and_exit(argv[0]);
    password = argv[optind++];
}

int main(int argc, char *argv[])
{
    LOGInit("registerStorageNode", NULL);
    LOGSetMax(0); // No limit
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);

    RegisterStorageNodeCCD ccd;
    int rv;
    ccd.dontLogoutOnExit();
    rv = ccd.login(args.username, args.password);
    if (rv) goto end;
    rv = ccd.linkDevice();
    if (rv) goto end;
    if (args.doRegister) {
        rv = ccd.registerStorageNode();
        if (rv) goto end;
    }
    else {
        rv = ccd.unregisterStorageNode();
        if (rv) goto end;
    }

 end:
    return rv;
}
