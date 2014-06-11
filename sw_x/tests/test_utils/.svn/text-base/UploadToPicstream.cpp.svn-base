#include <vplu_types.h>
#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <log.h>

#include "LoginCCD.hpp"

#include <iostream>

using namespace std;

class UploadToPicstream : public LoginCCD {
public:
    int UploadFile(const std::string &path);
};

int UploadToPicstream::UploadFile(const std::string &path)
{
    int err = 0;

    ccd::UpdateSyncSettingsInput req;
    ccd::UpdateSyncSettingsOutput res;
    req.set_user_id(userId);
    req.set_send_file_to_camera_roll(path);
    err = CCDIUpdateSyncSettings(req, res);
    if (err != CCD_OK) {
        LOG_ERROR("CCDIUpdateSyncSettings failed: %d", err);
        goto end;
    }

    if (res.send_file_to_camera_roll_err() != 0) {
        LOG_ERROR("send_file_to_camera_roll failed: %d", res.send_file_to_camera_roll_err());
        goto end;
    }

 end:
    return err;
}

static void print_usage_and_exit(const char *progname)
{
    cerr << "Usage: " << progname << " testInstanceNum username password path" << endl;
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    int testInstanceNum;
    string username;
    string password;
    string path;
};

Args::Args(int argc, char *argv[])
{
    if (argc < 5) {
        print_usage_and_exit(argv[0]);
    }

    testInstanceNum  = atoi(argv[1]);
    username    = argv[2];
    password    = argv[3];
    path        = argv[4];
}

int main(int argc, char *argv[])
{
    LOGInit("UploadToPicstream", NULL);
    LOGSetMax(0);
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);

    CCDIClient_SetTestInstanceNum(args.testInstanceNum);

    UploadToPicstream ccd;
    int err = 0;
    ccd.dontLogoutOnExit();
    err = ccd.login(args.username, args.password);
    if (err) goto end;
    err = ccd.UploadFile(args.path);
    if (err) goto end;

 end:
    return err;
}
