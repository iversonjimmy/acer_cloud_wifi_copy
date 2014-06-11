#include "ccd_core.h"
#include "vpl_fs.h"
#include "gvm_file_utils.hpp"
#include "test_single_sign_on.h"

#include <ccdi.hpp>
#include <log.h>
#include <string>
#include <cerrno>

static u64 testUserId;

int test_login_with_password(const char* username,
                             const char* password)
{
    int rv;
    int success = 0;
    int fail = 0;
    std::string passOrFail;
    
    passOrFail.assign("PASS");
    
    LOG_INFO("Username:%s Password:%s", username, password);
    
    /*
     * Login with username and password.
     */
    {
        ccd::LoginInput loginRequest;
        ccd::LoginOutput loginResponse;
        loginRequest.set_player_index(0);
        loginRequest.set_user_name(username);
        loginRequest.set_password(password);
        
        LOG_INFO("###########################");
        LOG_INFO("Logging in with password...");
        LOG_INFO("###########################");
        rv = CCDILogin(loginRequest, loginResponse);
        if (rv != CCD_OK) {
            LOG_ERROR("Error: CCDILogin: %d", rv);
            fail++;
            passOrFail.assign("FAIL");
            goto end;
        }
        testUserId = loginResponse.user_id();
        LOG_INFO("Login complete, userId: %lld.", testUserId);
    }
    
    /*
     * Link device.
     */
    {
        LOG_INFO("###########################");
        LOG_INFO("Linking device...");
        LOG_INFO("###########################");
        
        ccd::LinkDeviceInput linkInput;
        linkInput.set_user_id(testUserId);
        linkInput.set_is_acer_device(true);
        const char* test_device_name = "SSOTestDevice";
        
        linkInput.set_device_name(test_device_name);
        LOG_INFO("Linking device %s", test_device_name);
        rv = CCDILinkDevice(linkInput);
        if (rv != CCD_OK) {
            LOG_ERROR("Error: CCDILinkDevice: %d", rv);
            fail++;
            passOrFail.assign("FAIL");
            goto end;
        }
        success++;
        LOG_INFO("Link device complete");
    }
    
end:
    LOG_INFO("TC_RESULT=%s ;;; TC_NAME=test_login_with_password\n", passOrFail.c_str());
    LOG_INFO("Tests SUCCEEDED: %d\n", success);
    LOG_INFO("Tests FAILED: %d\n", fail);
    
    printf("Summary: %s - test_login_with_password\n", passOrFail.c_str());
    printf("Summary: Tests SUCCEEDED: %d\n", success);
    printf("Summary: Tests FAILED: %d\n", fail);
    
    return rv;
}

int test_login_with_credential(const char* username)
{
    int rv;
    int success = 0;
    int fail = 0;
    std::string passOrFail;
    std::string retrievedUserName;
    
    passOrFail.assign("PASS");
    
    LOG_INFO("Username:%s", username);
    
    /*
     * Login with shared credential from keychain.
     */
    {
        LOG_INFO("##############################");
        LOG_INFO("Logging in with credential...");
        LOG_INFO("##############################");
        
        // First calling CCDIUpdateAppState to login the user with credential.
        ccd::UpdateAppStateInput request;
        request.set_app_id("CCD_PROFILING");
        request.set_foreground_mode(true);
        request.set_app_type(ccd::CCD_APP_VIDEO);
        ccd::UpdateAppStateOutput response;
        
        rv = CCDIUpdateAppState(request, response);

        if (rv != CCD_OK) {
            LOG_ERROR("Error: CCDIUpdateAppState: %d", rv);
            fail++;
            passOrFail.assign("FAIL");
            goto end;
        }
        
        // Then calling the CCDIGetSystemState to retrieve the user info.
        ccd::GetSystemStateInput systemStateRequest;
        systemStateRequest.set_get_players(true);
        ccd::GetSystemStateOutput systemStateResponse;
        rv = CCDIGetSystemState(systemStateRequest, systemStateResponse);
        
        if (rv != CCD_OK) {
            LOG_ERROR("Error: CCDIGetSystemState: %d", rv);
            fail++;
            passOrFail.assign("FAIL");
            goto end;
        }
        
        // Compare retrieved user name with expected one.
        retrievedUserName = systemStateResponse.players().players(0).username();
        
        if (retrievedUserName.length() <= 0) {
            LOG_ERROR("Error: Login failed");
            fail++;
            passOrFail.assign("FAIL");
            goto end;
        }
        
        testUserId = systemStateResponse.players().players(0).user_id();
        
        success++;
        LOG_INFO("Login complete, userName: %s.", retrievedUserName.c_str());
    }
    
end:
    LOG_INFO("TC_RESULT=%s ;;; TC_NAME=test_login_with_credential\n", passOrFail.c_str());
    LOG_INFO("Tests SUCCEEDED: %d\n", success);
    LOG_INFO("Tests FAILED: %d\n", fail);
    
    printf("Summary: %s - test_login_with_credential\n", passOrFail.c_str());
    printf("Summary: Tests SUCCEEDED: %d\n", success);
    printf("Summary: Tests FAILED: %d\n", fail);
    
    return rv;
}

int test_logout()
{
    int rv;
    int success = 0;
    int fail = 0;
    std::string passOrFail;
    
    passOrFail.assign("PASS");
    
    /*
     * Unlink Device.
     */
    {
        ccd::UnlinkDeviceInput unlinkInput;
        LOG_INFO("Unlinking device");
        unlinkInput.set_user_id(testUserId);
        rv = CCDIUnlinkDevice(unlinkInput);
        if (rv != CCD_OK) {
            LOG_ERROR("Error: CCDIUnlinkDevice: %d", rv);
            fail++;
            passOrFail.assign("FAIL");
            goto end;
        }
        LOG_INFO("Unlink device complete");
    }
    
    /*
     * Logout user.
     */
    {
        ccd::LogoutInput logoutRequest;
        LOG_INFO("Logging out user");
        rv = CCDILogout(logoutRequest);
        if (rv != CCD_OK && rv != CCD_ERROR_NOT_SIGNED_IN) {
            LOG_ERROR("Error: CCDILogout: %d", rv);
            fail++;
            passOrFail.assign("FAIL");
            goto end;
        }
        success++;
        LOG_INFO("Logout complete");
    }
    
end:
    LOG_INFO("TC_RESULT=%s ;;; TC_NAME=test_logout\n", passOrFail.c_str());
    LOG_INFO("Tests SUCCEEDED: %d\n", success);
    LOG_INFO("Tests FAILED: %d\n", fail);
    
    printf("Summary: %s - test_logout\n", passOrFail.c_str());
    printf("Summary: Tests SUCCEEDED: %d\n", success);
    printf("Summary: Tests FAILED: %d\n", fail);
    
    return rv;
}
