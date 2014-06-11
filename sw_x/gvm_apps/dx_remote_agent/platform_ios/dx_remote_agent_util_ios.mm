#import <Foundation/Foundation.h>
#include "dx_remote_agent_util.h"
#include "dx_remote_agent_util_ios.h"
#include <vplex_file.h>
#include <string>
#include <vplex_shared_object.h>
#include <vplex_shared_credential.hpp>

int get_user_folder(std::string &path)
{
    int rc = 0;
    NSString *home_dir = NSHomeDirectory();
    NSString *test_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    NSString *push_dir = [test_dir stringByAppendingPathComponent:@"dxshell_pushfiles"];
    path.assign([push_dir UTF8String]);
    
    rc = VPLDir_Create(path.c_str(), 0755);
    if (rc == VPL_ERR_EXIST) {
        rc = VPL_OK;
    }
    
    return rc;
}

int get_cc_folder(std::string &path)
{
    int rc = 0;
    NSString *home_dir = NSHomeDirectory();
    NSString *test_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    path.assign([test_dir UTF8String]);

    return rc;
}

int get_app_data_folder(std::string &path)
{
    int rc = 0;
    NSString *home_dir = NSHomeDirectory();
    NSString *home_tmp_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    NSString *sync_agent_dir = [home_tmp_dir stringByAppendingPathComponent:@"SyncAgent"];
    path.assign([sync_agent_dir UTF8String]);
    
    return rc;
}

int clean_cc()
{
    int rc = 0;
    const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
    rc =VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_IS_DEVICE_LINKED_ID);
    
    rc = DeleteCredential(VPL_USER_CREDENTIAL);
    rc = DeleteCredential(VPL_DEVICE_CREDENTIAL);
    return rc;
}
