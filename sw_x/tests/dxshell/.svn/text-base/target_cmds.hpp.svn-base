#ifndef _TARGET_CMDS_HPP_
#define _TARGET_CMDS_HPP_

#include <string>

int target_getAliasPath(std::string alias);
int target_getCcdAppDataPath(std::string &ccdAppDataPath);
int target_getDeviceName(std::string &deviceName);
int target_getDeviceClass(std::string &deviceClass);
int target_getOsVersion(std::string &osVersion);
int target_getLibraryInfo(const std::string &lib_type);
int target_getIsAcerDevice(bool &isAcerDevice);
int target_getDeviceHasCamera(bool &deviceHasCamera);
int target_pullFile(const std::string &targetpath, const std::string &localpath);
int target_pushFile(const std::string &localpath, const std::string &targetpath);
int target_renameFile(const std::string &srcpath, const std::string &dstpath);
int target_createDir(const std::string &targetpath, int mode);
int target_removeDirRf(const std::string &targetpath);
int target_setPermission(const std::string &targetpath, const std::string &mode);

int dispatch_target_cmd(int argc, const char* argv[]);

#endif // _TARGET_CMDS_HPP_
