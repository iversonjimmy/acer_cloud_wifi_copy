#include <cstdlib>
#include <string>
#include "ccdi.hpp"
#include "log.h"
#include "vpl_time.h"
#include "vpl_th.h"

class LoginCCD {
public:
    LoginCCD();
    ~LoginCCD();
    int login(const std::string &username, const std::string &password);
    int linkDevice();
    int linkDevice(const std::string &devicename);
    void dontLogoutOnExit() { logoutOnExit = false; }
protected:
    u64 userId;
    bool deviceIsLinked;
    bool logoutOnExit;
};

LoginCCD::LoginCCD() : userId(0), deviceIsLinked(false), logoutOnExit(true)
{
    VPLTime_t timeout = VPLTIME_FROM_SEC(30);
    LOG_INFO("Will wait up to "FMTu64"ms for CCD to be ready...", VPLTIME_TO_MILLISEC(timeout));
    VPLTime_t endTime = VPLTime_GetTimeStamp() + timeout;

    ccd::GetSystemStateInput request;
    request.set_get_players(true);
    ccd::GetSystemStateOutput response;
    int rv = -1;
    while(1) {
        rv = CCDIGetSystemState(request, response);
        if (rv == CCD_OK) {
            LOG_INFO("CCD is ready!");
            userId = 0;
            return;
        }
        if ((rv != IPC_ERROR_SOCKET_CONNECT) && (rv != VPL_ERR_NAMED_SOCKET_NOT_EXIST)) {
            LOG_ERROR("Unexpected error: %d", rv);
            break;
        }
        LOG_INFO("Still waiting for CCD to be ready...");
        if (VPLTime_GetTimeStamp() >= endTime) {
            LOG_ERROR("Timed out after "FMTu64"ms", VPLTIME_TO_MILLISEC(timeout));
            rv = VPL_ERR_TIMEOUT;
            break;
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(2000));
    }

    LOG_ERROR("CCD is not ready: %d", rv);
    exit(1);
}

LoginCCD::~LoginCCD()
{
#if 0
    // never unlink, as that causes psn datasets to be lost
    // http://intwww/wiki/index.php/VSDS#UnlinkDevice
    if (deviceIsLinked) {
        LOG_INFO("Unlinking...");

        ccd::UnlinkDeviceInput req;
        req.set_user_id(userId);
        int rv = CCDIUnlinkDevice(req);
        if (rv != CCD_OK) {
            LOG_ERROR("Failed to unlink device: %d", rv);
        }
    }
#endif

    if (logoutOnExit && userId != 0) {
        LOG_INFO("Logging out...");

        ccd::LogoutInput req;
        req.set_local_user_id(userId);
        int rv = CCDILogout(req);
        if (rv != CCD_OK) {
            LOG_ERROR("Failed to logout: %d", rv);
        }
        userId = 0;
    }
}

int LoginCCD::login(const std::string& username,
                    const std::string& password)
{
    int rv = 0;

    LOG_INFO("Logging in...");

    ccd::LoginInput req;
    ccd::LoginOutput res;
    req.set_player_index(0);
    req.set_user_name(username);
    req.set_password(password);
    rv = CCDILogin(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to login: %d", rv);
        goto end;
    }
    userId = res.user_id();
    LOG_INFO("Login complete, userId: "FMTx64".", userId);

 end:
    return rv;
}

int LoginCCD::linkDevice()
{
    char devicename[1024];
    gethostname(devicename, sizeof(devicename));
    return linkDevice(devicename);
}

int LoginCCD::linkDevice(const std::string &devicename)
{
    int rv = 0;

    LOG_INFO("Linking device...");

    ccd::LinkDeviceInput req;
    req.set_user_id(userId);
    req.set_device_name(devicename.c_str());
    req.set_is_acer_device(true);  // pretend Acer device
    rv = CCDILinkDevice(req);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to link device: %d", rv);
        goto end;
    }
    deviceIsLinked = true;
    LOG_INFO("Device linking complete");

 end:
    return rv;
}
