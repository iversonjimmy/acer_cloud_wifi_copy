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
/// Tool to list owned datasets.
//============================================================================

#include <vplu.h>
#include "ccdi.hpp"
#include "log.h"

#include "VisitDatasetCCD.hpp"

using namespace std;

class ListDatasetCCD : public VisitDatasetCCD {
public:
    int list_datasets();
};

static void datasetVisitor(const vplex::vsDirectory::DatasetDetail &dd)
{
    printf("name=\"%s\"\ttype=%d\tid="FMTx64"\tsname=%s\n", dd.datasetname().c_str(), dd.datasettype(), dd.datasetid(), dd.storageclustername().c_str());
}

int ListDatasetCCD::list_datasets()
{
    int rv = 0;

    rv = visitDatasets(datasetVisitor);
    if (rv) goto end;

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
    LOGInit("listDatasets", NULL);
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);

    ListDatasetCCD ccd;
    int rv;
    ccd.dontLogoutOnExit();
    rv = ccd.login(args.username, args.password);
    if (rv) goto end;
    //rv = ccd.linkDevice();
    //if (rv) goto end;
    rv = ccd.list_datasets();
    if (rv) goto end;

 end:
    return rv;
}
