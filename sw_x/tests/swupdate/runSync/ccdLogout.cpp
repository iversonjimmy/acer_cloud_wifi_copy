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

static int logout(u64 userId)
{
    int rv;
    ccd::LogoutInput logoutRequest;
    logoutRequest.set_local_user_id(userId);
    rv = CCDILogout(logoutRequest);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDILogout: %d", rv);
    }
    return rv;
}

static int unlinkDevice(u64 userId)
{
    int rv = 0;

    ccd::UnlinkDeviceInput unlinkInput;
    printf("Unlinking device\n");
    unlinkInput.set_user_id(userId);
    rv = CCDIUnlinkDevice(unlinkInput);
    if (rv != CCD_OK) {
        printf("Error: CCDIUnlinkDevice: %d\n", rv);
        return -1;
    }

    return 0;
}

int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int rv = 0;
    const char* userInfoFile = "/tmp/userInfo"; 
    FILE*       fH;
    std::string machine;

    if(argc != 2) {
        LOG_INFO("Usage: %s <machine>\n", argv[0]);
        LOG_INFO("Example: %s C1", argv[0]);
        return -1;
    }

    u64 userId;

    machine.assign(argv[1]);

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

    rv = unlinkDevice(userId);
    if(rv != 0) {
        LOG_ERROR("unlinkDevice failed:%d", rv);
        goto out;
    }

    rv = logout(userId);
    if(rv != 0) {
        LOG_ERROR("Logout failed:%d", rv);
        goto out;
    }

out:
    return rv;
}
