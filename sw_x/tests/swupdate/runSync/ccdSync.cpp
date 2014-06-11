//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <ccdi.hpp>
#include <log.h>
#include <vpl_th.h>
#include <vpl_time.h>

#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>

static int ownershipSync()
{
    int rv;
    rv = CCDIOwnershipSync();
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIOwnershipSync: %d", rv);
    }
    return rv;
}

int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int rv = 0;
    std::string machine;
    std::string password;
    const char* userInfoFile = "/tmp/userInfo"; 
    FILE*       fH;

    if(argc != 2) {
        LOG_INFO("Usage: %s <machine>\n", argv[0]);
        LOG_INFO("Example: %s C1", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);

    u64 userId;

    fH = fopen(userInfoFile, "r");
    if (fH == NULL) {
        LOG_ERROR("Fail to read user id %s", userInfoFile);
        goto out;
    }

    rv = fscanf(fH, "%lld\n", &userId);
    if (rv < 0) {
        LOG_ERROR("Error reading user info");
        goto out;
    }

    rv = ownershipSync();
    if (rv != 0) {
        LOG_ERROR("CCD sync failed: %d", rv);
        goto out;
    }

out:
    return rv;
}
