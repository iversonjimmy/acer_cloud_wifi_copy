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
/// Tool to sync local folder with a dataset.
//============================================================================

#include <vplu.h>

#include "ccdi.hpp"
#include "log.h"

#include "FindDatasetCCD.hpp"

using namespace std;

class AddSyncSubscriptionCCD : public FindDatasetCCD {
public:
    int add_sync_subscription(const std::string &datasetname,
                              const std::string &folderpath);
};

int AddSyncSubscriptionCCD::add_sync_subscription(const std::string &datasetname,
                                                  const std::string &folderpath)
{
    int rv = 0;
    u64 datasetid;

    rv = findDataset(datasetname, datasetid);
    if (rv) goto end;

    {
        ccd::AddSyncSubscriptionInput req;
        req.set_user_id(userId);
        req.set_dataset_id(datasetid);
        req.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
        req.set_device_root(folderpath.c_str());
        rv = CCDIAddSyncSubscription(req);
        if (rv != CCD_OK) {
            LOG_ERROR("Failed to add sync subscription between local folder %s and dataset %s: %d", 
                      folderpath.c_str(), datasetname.c_str(), rv);
            goto end;
        }
    }

 end:
    return rv;
}

static void print_usage_and_exit(const char *progname)
{
    fprintf(stderr, 
            "Usage: %s username password datasetname folderpath\n",
            progname);
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    string username;
    string password;
    string datasetname;
    string folderpath;
private:
    Args();  // hide default constructor
};

Args::Args(int argc, char *argv[])
{
    if (argc < 5) {
        print_usage_and_exit(argv[0]);
    }

    username = argv[1];
    password = argv[2];
    datasetname = argv[3];
    folderpath = argv[4];
}

int main(int argc, char *argv[])
{
    LOGInit("addSyncSubscription", NULL);
    LOGSetMax(0);       // No limit
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);

    AddSyncSubscriptionCCD ccd;
    int rv;
    ccd.dontLogoutOnExit();
    rv = ccd.login(args.username, args.password);
    if (rv) goto end;
    //rv = ccd.linkDevice();
    //if (rv) goto end;
    rv = ccd.add_sync_subscription(args.datasetname, args.folderpath);
    if (rv) goto end;

 end:
    return rv;
}
