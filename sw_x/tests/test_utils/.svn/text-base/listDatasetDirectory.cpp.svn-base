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
/// Tool to list entries in a dataset directory.
//============================================================================

#include <vplu.h>
#include "ccdi.hpp"
#include "log.h"

#include "VisitDatasetEntryCCD.hpp"

using namespace std;

class ListDatasetDirectoryCCD : public VisitDatasetEntryCCD {
public:
    int list_dataset_entries(const std::string &datasetname, 
                             const std::string &directory);
};

static void entryVisitor(const ccd::DatasetDirectoryEntry &dde)
{
    printf("name=\"%s\"\tis_dir=%d\tsize="FMTu64"\turl=%s\tmtime="FMTu64"\n",
           dde.name().c_str(), dde.is_dir(), dde.size(), dde.url().c_str(), dde.mtime());
}

int ListDatasetDirectoryCCD::list_dataset_entries(const string &datasetname, 
                                                  const string &directory)
{
    int rv = 0;

    rv = visitDatasetEntries(datasetname, directory, entryVisitor);
    if (rv) goto end;

 end:
    return rv;
}

static void print_usage_and_exit(const char *progname)
{
    fprintf(stderr, 
            "Usage: %s username password datasetname directory\n",
            progname);
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    string username;
    string password;
    string datasetname;
    string directory;
private:
    Args();  // hide default constructor
};

Args::Args(int argc, char *argv[])
{
    if (argc < 5) print_usage_and_exit(argv[0]);

    username = argv[1];
    password = argv[2];
    datasetname = argv[3];
    directory = argv[4];
}

int main(int argc, char ** argv)
{
    LOGInit("listDatasetDirectory", NULL, 0);
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);
    
    ListDatasetDirectoryCCD ccd;
    int rv;
    ccd.dontLogoutOnExit();
    rv = ccd.login(args.username, args.password);
    if (rv) goto end;
    //rv = ccd.linkDevice();
    //if (rv) goto end;
    rv = ccd.list_dataset_entries(args.datasetname, args.directory);
    if (rv) goto end;

 end:
    return rv;
}
