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
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>

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

#define USER_INFO_FILE                  "user_info"

int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int         rv = 0;
    std::string machine;
    std::string testroot;
    VPLFile_handle_t  fH = 0;
    std::string userInfoFile("");
    ssize_t readSz; 

    if(argc != 3) {
        LOG_INFO("Usage: %s <machine> <test dir>\n", argv[0]);
        LOG_INFO("Example: %s C1 testroot", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);
    testroot.assign(argv[2]);

    u64 userId_out;

#ifdef WIN32
    if(testroot.size()>0 && testroot[0]=='/') {
        testroot = std::string("C:") + testroot;
    }
#endif

    userInfoFile.append(testroot.c_str());
    userInfoFile.append("/");
    userInfoFile.append(USER_INFO_FILE);

    fH = VPLFile_Open(userInfoFile.c_str(), VPLFILE_OPENFLAG_READWRITE, 0777);
    if (!VPLFile_IsValidHandle(fH)) {
        LOG_ERROR("Error opening in %s", userInfoFile.c_str());
        rv = -1;
        goto out;
    }

    readSz = VPLFile_Read(fH, &userId_out, sizeof(userId_out));
    if (readSz <= 0) {
        LOG_ERROR("Error reading user information in %s", userInfoFile.c_str());
        rv = -1;
        goto out;
    }

    LOG_INFO("Logging out user "FMTu64"...", userId_out);

    rv = logout(userId_out);
    if(rv != 0) {
        LOG_ERROR("Logout failed:%d", rv);
        goto out;
    }

out:
    if (VPLFile_IsValidHandle(fH))
        VPLFile_Close(fH);
    return rv;
}
