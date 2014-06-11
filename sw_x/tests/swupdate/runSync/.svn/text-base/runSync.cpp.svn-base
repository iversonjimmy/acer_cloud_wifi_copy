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
#define PATH_MAX_LENGTH         1024

static int closeFds()
{
    int fd = -1;
    int rv = 0;
    DIR* directory = NULL;
    struct dirent* entry = NULL;
    char path[PATH_MAX_LENGTH];

    snprintf(path, sizeof(path), "/proc/%d/fd", (int)getpid());

    directory = opendir(path);
    if (directory == NULL) {
        LOG_ERROR("opendir %s failed: %s", path, strerror(errno));
        goto out;
    }

    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(".",  entry->d_name, 2) == 0 ||
	    strncmp("..", entry->d_name, 3) == 0) {
            continue;
        }

        fd = atoi(entry->d_name);
        if (fd > KMSG_FILENO) {
            close(fd);
        }
    }

    closedir(directory);
out:
    directory = NULL;
    entry = NULL;

    return rv;
}

static pid_t launchCcd(void)
{
    LOG_INFO("start");
    char*  argv[] = { (char *)"/bin/ccd", (char *)NULL };

    pid_t pid;
    pid = fork();
    if (pid == 0) {
        closeFds();
        LOG_INFO("executing /bin/ccd");
        execv("/bin/ccd", argv);
        abort();
    }

    LOG_INFO("CCD[pid %d] started", pid);

    return pid;
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


static int shutdownCcd()
{
    LOG_INFO("start");
    ccd::UpdateSystemStateInput request;
    request.set_do_shutdown(true);
    ccd::UpdateSystemStateOutput response;
    int rv = CCDIUpdateSystemState(request, response);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIUpdateSystemState failed: %d", rv);
    }
    return (rv == 0) ? 0 : -1;

}

static int login(const std::string& username,
                 const std::string& password,
                 u64& userId_out)
{
    int rv = 0;
    userId_out=0;
    {
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
        userId_out = loginResponse.user_id();
        LOG_INFO("Login complete, userId: " FMTu64".", userId_out);
    }

 error_before_login:
       return rv;
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

    int rv;
    std::string machine;
    std::string username;
    std::string password;

    if(argc != 4) {
        LOG_INFO("Usage: %s <machine> <username> <password>\n", argv[0]);
        LOG_INFO("Example: %s C1 syncWbTestCaseA password", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);
    username.assign(argv[2]);
    password.assign(argv[3]);

    LOG_INFO("Client:%s Username:%s Password:%s\n", machine.c_str(), username.c_str(), password.c_str());


    // Launch ccd process
    launchCcd();

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));

    // Test app waiting for CCD to be ready
    waitCcd();


    u64 userId_out;
    rv = login(username, password, userId_out);
    if(rv != 0) {
        LOG_ERROR("Login failed:%d", rv);
        goto out;
    }

    rv = ownershipSync();
    if(rv != 0) {
        LOG_ERROR("CCD sync failed: %d", rv);
    }

    rv = logout(userId_out);
    if(rv != 0) {
        LOG_ERROR("Logout failed:%d", rv);
    }

out:
    shutdownCcd();
    return 0;
}
