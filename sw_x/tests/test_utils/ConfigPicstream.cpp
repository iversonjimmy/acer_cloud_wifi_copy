#include <vplu_types.h>
#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <log.h>

#include "LoginCCD.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cassert>

class ConfigPicstreamDir : public LoginCCD {
public:
    int EnablePicstream(bool on);
    int AddUploadDir(const std::string &path);
    int GetUploadDirs(bool &enabled, std::vector<std::string> &paths);
    int SetFullResDownloadDir(const std::string &path);
    int GetFullResDownloadDir(std::string &path);
    int SetLowResDownloadDir(const std::string &path);
    int GetLowResDownloadDir(std::string &path);
};

int ConfigPicstreamDir::EnablePicstream(bool on)
{
    int err = 0;

    ccd::UpdateSyncSettingsInput req;
    ccd::UpdateSyncSettingsOutput res;
    req.set_user_id(userId);
    req.set_enable_camera_roll(on);
    err = CCDIUpdateSyncSettings(req, res);
    assert(!err);

    if (res.enable_camera_roll_err() != 0) {
        LOG_ERROR("add_camera_roll_upload_dirs failed: %d", res.enable_camera_roll_err());
        goto end;
    }

 end:
    return err;
}

int ConfigPicstreamDir::AddUploadDir(const std::string &path)
{
    int err = 0;

    ccd::UpdateSyncSettingsInput req;
    ccd::UpdateSyncSettingsOutput res;
    req.set_user_id(userId);
    req.add_add_camera_roll_upload_dirs(path);
    err = CCDIUpdateSyncSettings(req, res);
    assert(!err);

    if (res.add_camera_roll_upload_dirs_err() != 0) {
        LOG_ERROR("add_camera_roll_upload_dirs failed: %d", res.add_camera_roll_upload_dirs_err());
        goto end;
    }

 end:
    return err;
}

int ConfigPicstreamDir::GetUploadDirs(bool &enabled, std::vector<std::string> &paths)
{
    int err = 0;

    ccd::GetSyncStateInput req;
    ccd::GetSyncStateOutput res;
    req.set_user_id(userId);
    req.set_get_is_camera_roll_upload_enabled(true);
    req.set_get_camera_roll_upload_dirs(true);
    err = CCDIGetSyncState(req, res);
    assert(!err);

    enabled = res.is_camera_roll_upload_enabled();

    paths.clear();
    for (int i = 0; i < res.camera_roll_upload_dirs_size(); i++) {
        paths.push_back(res.camera_roll_upload_dirs(i));
    }

    return err;
}

#define SetDownloadDir(ResType,res_type)                                \
int ConfigPicstreamDir::Set##ResType##DownloadDir(const std::string &path) \
{                                                                       \
    int err = 0;                                                        \
                                                                        \
    ccd::UpdateSyncSettingsInput req;                                   \
    ccd::UpdateSyncSettingsOutput res;                                  \
    req.set_user_id(userId);                                            \
    ccd::CameraRollDownloadDirSpec *spec = req.mutable_add_camera_roll_##res_type##_download_dir(); \
    if (!spec) {                                                        \
        LOG_ERROR("mutable object is null");                            \
        return -1;                                                      \
    }                                                                   \
    spec->set_dir(path);                                                \
    err = CCDIUpdateSyncSettings(req, res);                             \
    if (err != CCD_OK) {                                                \
        LOG_ERROR("CCDIUpdateSyncSettings failed: %d", err);            \
        goto end;                                                       \
    }                                                                   \
                                                                        \
    if (res.add_camera_roll_##res_type##_download_dir_err() != 0) {     \
	LOG_ERROR("add_camera_roll_" #res_type "_download_dir failed: %d", res.add_camera_roll_##res_type##_download_dir_err()); \
	goto end;                                                       \
    }                                                                   \
                                                                        \
 end:                                                                   \
    return err;                                                         \
}

SetDownloadDir(FullRes,full_res);
SetDownloadDir(LowRes,low_res);

#undef SetDownloadDir

#define GetDownloadDir(ResType,res_type)                                \
int ConfigPicstreamDir::Get##ResType##DownloadDir(std::string &path)    \
{                                                                       \
    int err = 0;                                                        \
                                                                        \
    ccd::GetSyncStateInput req;                                         \
    ccd::GetSyncStateOutput res;                                        \
    req.set_user_id(userId);                                            \
    req.set_get_camera_roll_download_dirs(true);                        \
    err = CCDIGetSyncState(req, res);                                   \
    if (err != CCD_OK) {                                                \
        LOG_ERROR("CCDIGetSyncState failed: %d", err);                  \
        goto end;                                                       \
    }                                                                   \
                                                                        \
    if (res.camera_roll_##res_type##_download_dirs_size() > 0) {        \
        path.assign(res.camera_roll_##res_type##_download_dirs(0).dir()); \
    }                                                                   \
                                                                        \
 end:                                                                   \
    return err;                                                         \
}

GetDownloadDir(FullRes,full_res);
GetDownloadDir(LowRes,low_res);

#undef GetDownloadDir

static void print_usage_and_exit(const char *progname)
{
    std::cerr << "Usage: " << progname << " testInstanceNum username password {on|off|up|df|dl} [path]" << std::endl;
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    int testInstanceNum;
    std::string username;
    std::string password;
    std::string key;
    std::string path;
};

Args::Args(int argc, char *argv[])
{
    if (argc < 5) {
        print_usage_and_exit(argv[0]);
    }

    testInstanceNum	= atoi(argv[1]);
    username	= argv[2];
    password	= argv[3];
    key		= argv[4];
    if (argc >= 6) {
        path	= argv[5];
    }
}

int main(int argc, char *argv[])
{
    LOGInit("PicStreamDownloadDir", NULL);
    LOGSetMax(0);
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);

    CCDIClient_SetTestInstanceNum(args.testInstanceNum);

    ConfigPicstreamDir ccd;
    int err = 0;
    ccd.dontLogoutOnExit();
    err = ccd.login(args.username, args.password);
    assert(!err);

    if (args.key == "on") {
        err = ccd.EnablePicstream(true);
        assert(!err);
    }
    else if (args.key == "off") {
        err = ccd.EnablePicstream(false);
        assert(!err);
    }
    else if (args.key == "up") {
        if (args.path.empty()) {
            bool enabled;
            std::vector<std::string> paths;
            err = ccd.GetUploadDirs(enabled, paths);
            assert(!err);
            std::cout << "Upload is " << (enabled ? "enabled" : "disabled") << std::endl;
            for (int i = 0; i < paths.size(); i++) {
                std::cout << paths[i] << std::endl;
            }
        }
        else {
            err = ccd.AddUploadDir(args.path);
            assert(!err);
        }
    }
    else if (args.key == "df") {
        if (args.path.empty()) {
            std::string path;
            err = ccd.GetFullResDownloadDir(path);
            assert(!err);
            std::cout << path << std::endl;
        }
        else {
            err = ccd.SetFullResDownloadDir(args.path);
            assert(!err);
        }
    }
    else if (args.key == "dl") {
        if (args.path.empty()) {
            std::string path;
            err = ccd.GetLowResDownloadDir(path);
            assert(!err);
            std::cout << path << std::endl;
        }
        else {
            err = ccd.SetLowResDownloadDir(args.path);
            assert(!err);
        }
    }
    else {
        LOG_ERROR("Unknown key %s", args.key.c_str());
        goto end;
    }

 end:
    return err;
}
