//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// Command-line "menu" for testing CCD.  Asks for a username and password,
// logs in to infra, and displays download progress until all titles have
// finished or the program is interrupted, at which point it will log out
// and exit.

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

#define CCD_TIMEOUT             30

static int login(const std::string& username,
                 const std::string& password,
                 u64& userId)
{
    int rv = 0;
    userId=0;

    ccd::LoginInput loginRequest;
    loginRequest.set_player_index(0);
    loginRequest.set_user_name(username);
    loginRequest.set_password(password);
    ccd::LoginOutput loginResponse;
    LOG_INFO("Logging in...");
    rv = CCDILogin(loginRequest, loginResponse);
    if (rv != CCD_OK) {
        LOG_ERROR("Error: CCDILogin: %d", rv);
        goto error_before_login;
    }
    userId = loginResponse.user_id();
    LOG_INFO("Login complete, userId: "FMTu64".", userId);

error_before_login:
    return rv;
}

static int waitCcd()
{
    LOG_INFO("start");
    VPLTime_t timeout = VPLTIME_FROM_SEC(CCD_TIMEOUT);
    VPLTime_t endTime = VPLTime_GetTimeStamp() + timeout;

    ccd::GetSystemStateInput request;
    request.set_get_players(true);
    ccd::GetSystemStateOutput response;
    int rv;
    while(1) {
        rv = CCDIGetSystemState(request, response);
        if (rv == CCD_OK) {
            LOG_INFO("CCD is ready!");
            break;
        }
        if (rv != IPC_ERROR_SOCKET_CONNECT) {
            LOG_ERROR("Unexpected error: %d", rv);
            break;
        }
        LOG_INFO("Still waiting for CCD to be ready...");
        if (VPLTime_GetTimeStamp() >= endTime) {
            LOG_ERROR("Timed out after "FMTu64"ms", VPLTIME_TO_MILLISEC(timeout));
            break;
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(500));
    }
    return (rv == 0) ? 0 : -1;
}

#define USER_INFO_FILE                  "user_info"

int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int rv;
    std::string machine;
    std::string username;
    std::string password;
    std::string testroot;
    std::string cloudRoot;
    VPLFile_handle_t  fH = 0;
    std::string userInfoFile("");
    ssize_t writeSz;

    if(argc != 6) {
        LOG_INFO("Usage: %s <machine> <username> <password> <test dir name> <cloud_root> \n", argv[0]);
        LOG_INFO("Example: %s C1 syncWbTestCaseA password testroot \"/home/<user>/My Cloud\"", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);
    username.assign(argv[2]);
    password.assign(argv[3]);
    testroot.assign(argv[4]);
    cloudRoot.assign(argv[5]);

#ifdef WIN32
    if(testroot.size()>0 && testroot[0]=='/') {
        testroot = std::string("C:") + testroot;
    }
#endif

    LOG_INFO("Client:%s Username:%s Password:%s CloudRoot:%s testRoot:%s\n",
            machine.c_str(), username.c_str(), password.c_str(), cloudRoot.c_str(), testroot.c_str());

    // Waiting for CCD to be ready
    waitCcd();

    u64 userId;
    rv = login(username, password, userId);
    if(rv != 0) {
        LOG_ERROR("Login failed:%d", rv);
        goto out;
    }

    userInfoFile.append(testroot.c_str());
    userInfoFile.append("/");
    userInfoFile.append(USER_INFO_FILE);

    fH = VPLFile_Open(userInfoFile.c_str(), VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE, 0777);
    if (!VPLFile_IsValidHandle(fH)) {
        LOG_ERROR("Error opening %s", userInfoFile.c_str());
        rv = -1;
        goto out;
    }

    LOG_INFO("Writing userInfo to %s", userInfoFile.c_str());
    writeSz = VPLFile_Write(fH, &userId, sizeof(userId));
    if (writeSz != sizeof(userId)) {
        LOG_ERROR("Error writing user information in %s", userInfoFile.c_str());
        rv = -1;
        goto out;
    }

out:
    if (VPLFile_IsValidHandle(fH))
        VPLFile_Close(fH);
    return rv;
}
