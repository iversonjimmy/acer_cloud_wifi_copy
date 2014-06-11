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

#include <vpl_plat.h>

#define CCD_TIMEOUT             30

std::string dsetName;

int _dset_types[] = {
    vplex::vsDirectory::PIM_CONTACTS,
    vplex::vsDirectory::PIM_EVENTS,
    vplex::vsDirectory::PIM_NOTES,
    vplex::vsDirectory::PIM_TASKS,
    vplex::vsDirectory::PIM_FAVORITES,
    vplex::vsDirectory::MEDIA_METADATA,
};
#define ARRAY_LEN(x)    (sizeof(x)/sizeof(x[0]))

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
        goto out;
    }
    userId = loginResponse.user_id();
    LOG_INFO("Login complete, userId: %lld.", userId);

out:
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

static int linkDevice(u64 userId)
{
    int rv = 0;

    ccd::LinkDeviceInput linkInput;
    linkInput.set_user_id(userId);
    char myname[1024];
    gethostname(myname, sizeof(myname));
    linkInput.set_device_name(myname);
    linkInput.set_is_acer_device(true);
    LOG_INFO("Linking device %s\n", myname);
    rv = CCDILinkDevice(linkInput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDILinkDevice: %d", rv);
        return -1;
    }

    return 0;
}

static int unlinkDevice(u64 userId)
{
    int rv = 0;

    ccd::UnlinkDeviceInput unlinkInput;
    LOG_INFO("Unlinking device\n");
    unlinkInput.set_user_id(userId);
    rv = CCDIUnlinkDevice(unlinkInput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIUnlinkDevice: %d\n", rv);
        return -1;
    }

    return 0;
}


int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    int rv;
    std::string machine;
    std::string username;
    std::string password;
    std::string cloudRoot;
    VPLFile_handle_t  fH = 0;
    const char* userInfoFile = "/tmp/userInfo"; 
    const char* dsetInfoFile = "/tmp/dsetInfo"; 
    int         loggedIn = 0;
    int         linked = 0;
    ssize_t     writeSz;

    VPL_Init();

    if(argc != 5) {
        LOG_INFO("Usage: %s <machine> <username> <password> <cloud_root>\n", argv[0]);
        LOG_INFO("Example: %s C1 syncWbTestCaseA password \"/home/xxx/My Cloud\"", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);
    username.assign(argv[2]);
    password.assign(argv[3]);
    cloudRoot.assign(argv[4]);

    LOG_INFO("Client:%s Username:%s Password:%s cloudRoot:%s\n", 
            machine.c_str(), username.c_str(), password.c_str(), cloudRoot.c_str());

    // Waiting for CCD to be ready
    waitCcd();

    u64 userId;
    rv = login(username, password, userId);
    if(rv != 0) {
        LOG_ERROR("Login failed:%d", rv);
        goto out;
    }

    loggedIn = 1;

    rv = linkDevice(userId);
    if (rv != 0) {
        LOG_ERROR("linkDevice failed:%d", rv);
        goto out;
    }
    linked = 1;

    fH = VPLFile_Open(userInfoFile, VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE, 0777);
    if (!VPLFile_IsValidHandle(fH)) {
        LOG_ERROR("Error saving user information in %s", userInfoFile);
        rv = -1;
        goto out;
    }

    writeSz = VPLFile_Write(fH, &userId, sizeof(userId));
    if (writeSz != sizeof(userId)) {
        LOG_ERROR("Error writing user information in %s", userInfoFile);
        rv = -1;
        VPLFile_Close(fH);
        goto out;
    }

    VPLFile_Close(fH);

    fH = VPLFile_Open(dsetInfoFile, VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE, 0777);
    if (!VPLFile_IsValidHandle(fH)) {
        LOG_ERROR("Error saving dataset information in %s", dsetInfoFile);
        rv = -1;
        goto out;
    }

    writeSz = VPLFile_Write(fH, dsetName.data(), dsetName.length());
    if (writeSz != dsetName.length()) {
        LOG_ERROR("Error writing dataset information in %s", dsetInfoFile); 
        rv = -1;
        VPLFile_Close(fH);
        goto out;
    }

    VPLFile_Close(fH);

out:
    if (rv != 0) {

        if (loggedIn) 
            logout(userId);

        if (linked)
            unlinkDevice(userId);
    }
    return rv;
}
