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
/// Tool to list storage nodes.
//============================================================================

#include <vplu.h>
#include "ccdi.hpp"
#include "log.h"
#include "vpl_user.h"

#include "LoginCCD.hpp"

using namespace std;

class ListStorageNodesCCD : public LoginCCD {
public:
    int list_storage_nodes();
};

int ListStorageNodesCCD::list_storage_nodes()
{
    int rv = 0;

    ccd::GetPersonalCloudStateInput req;
    req.set_user_id(userId);
    req.set_list_storage_nodes(true);
    ccd::GetPersonalCloudStateOutput resp;
    rv = CCDIGetPersonalCloudState(req, resp);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to list storage nodes: %d", rv);
        goto end;
    }
    LOG_INFO("User "FMT_VPLUser_Id_t" has %d storage node(s).", userId, resp.storage_nodes_size());
    for (int i = 0; i < resp.storage_nodes_size(); i++) {
        LOG_INFO("[%d]: deviceId="FMTu64" name=\"%s\"", i,
                resp.storage_nodes(i).device_id(),
                resp.storage_nodes(i).storage_name().c_str());
    }

 end:
    return rv;
}
static void print_usage_and_exit(const char *progname)
{
    fprintf(stderr, 
            "Usage: %s username password\n",
            progname);
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    string username;
    string password;
private:
    Args();  // hide default constructor
};

Args::Args(int argc, char *argv[])
{
    if (argc < 3) print_usage_and_exit(argv[0]);

    username = argv[1];
    password = argv[2];
}

int main(int argc, char *argv[])
{
    LOGInit("listStorageNodes", NULL);
    LOGSetMax(0);       // No limit
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);

    ListStorageNodesCCD ccd;
    int rv;
    ccd.dontLogoutOnExit();
    rv = ccd.login(args.username, args.password);
    if (rv) goto end;
    rv = ccd.list_storage_nodes();
    if (rv) goto end;

 end:
    return rv;
}
