#include "target_cmds.hpp"
#include "common_utils.hpp"
#include "TargetDevice.hpp"
#include "dx_common.h"

#include <log.h>

#include <iostream>
#include <string>
#include <map>
#include <cstring>

int target_getAliasPath(std::string alias)
{
    int rc = 0;
    std::string path;
    TargetDevice *target = getTargetDevice();

    rc = target->getAliasPath(alias, path);
    if (rc != 0) {
        LOG_ERROR("Unable to get the path for alias %s, rc = %d", alias.c_str(), rc);
    } else {
        LOG_ALWAYS("Alias = %s, Path = %s", alias.c_str(), path.c_str());
    }
    delete target;
    return rc;
}

int target_getCcdAppDataPath(std::string &ccdAppDataPath)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    rc = target->getCcdAppDataPath(ccdAppDataPath);
    if (rc != 0) {
        LOG_ERROR("Failed to get CCD App Data path: %d", rc);
        goto end;
    }

 end:
    if (target != NULL)
        delete target;
    return rc;
}

int target_getDeviceName(std::string &deviceName)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    deviceName.assign(target->getDeviceName());

    if (target != NULL)
        delete target;
    return rc;
}

int target_getDeviceClass(std::string &deviceClass)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    deviceClass.assign(target->getDeviceClass());

    if (target != NULL)
        delete target;
    return rc;
}

int target_getLibraryInfo(const std::string &lib_type)
{
    int rc = 0;
    TargetDevice *target = getTargetDevice();

    std::vector<std::pair<std::string, std::string> > folder;

    rc = target->readLibrary(lib_type, folder);
    if (rc != 0) {
        LOG_ERROR("Unable to get the library info for type %s, rc = %d", lib_type.c_str(), rc);
    } else {
        std::vector<std::pair<std::string, std::string> >::iterator it;
        for (it = folder.begin(); it != folder.end(); it++) {
            LOG_ALWAYS("--> Real: %s, Virt: %s", it->first.c_str(), it->second.c_str());
        }
    }
    delete target;
    return rc;
}

int target_getOsVersion(std::string &osVersion)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    osVersion.assign(target->getOsVersion());

    if (target != NULL)
        delete target;
    return rc;
}

int target_getIsAcerDevice(bool &isAcerDevice)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    isAcerDevice = target->getIsAcerDevice();

    if (target != NULL)
        delete target;
    return rc;
}

int target_getDeviceHasCamera(bool &deviceHasCamera)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    deviceHasCamera = target->getDeviceHasCamera();

    if (target != NULL)
        delete target;
    return rc;
}

int target_pullFile(const std::string &targetpath, const std::string &localpath)
{
    int rc = 0;
    
    TargetDevice *target = getTargetDevice();
    rc = target->pullFile(targetpath, localpath);

    if (target != NULL) {
        delete target;
    }

    return rc;
}

int target_pushFile(const std::string &localpath, const std::string &targetpath)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    rc = target->pushFile(localpath, targetpath);

    if (target != NULL) {
        delete target;
    }

    return rc;
}

int target_renameFile(const std::string &srcpath, const std::string &dstpath)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    rc = target->renameFile(srcpath, dstpath);

    if (target != NULL) {
        delete target;
    }

    return rc;
}

int target_createDir(const std::string &targetpath, int mode)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    rc = target->createDir(targetpath, mode);

    if (target != NULL) {
        delete target;
    }

    return rc;
}

int target_removeDirRf(const std::string &targetpath)
{
    int rc = 0;

    TargetDevice *target = getTargetDevice();
    rc = target->removeDirRecursive(targetpath);

    if (target != NULL) {
        delete target;
    }

    return rc;
}

int target_setPermission(const std::string &targetpath, const std::string &mode)
{
    int rc = -1;

    TargetDevice *target = getTargetDevice();
    if (target != NULL) {
        rc = target->setFilePermission(targetpath, mode);
        delete target;
    }

    return rc;
}

//--------------------------------------------------
// dispatcher

static int target_help(int argc, const char* argv[])
{
    // argv[0] == "Target" if called from dispatch_target_cmd()
    // Otherwise, called from target_*() handler function

    bool printAll = strcmp(argv[0], "Target") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll || strcmp(argv[0], "GetAliasPath") == 0)
        std::cout << "Target GetAliasPath <alias>" << std::endl;
    if (printAll || strcmp(argv[0], "GetCCDAppDataPath") == 0)
        std::cout << "Target GetCCDAppDataPath" << std::endl;
    if (printAll || strcmp(argv[0], "GetDeviceName") == 0)
        std::cout << "Target GetDeviceName" << std::endl;
    if (printAll || strcmp(argv[0], "GetDeviceClass") == 0)
        std::cout << "Target GetDeviceClass" << std::endl;
    if (printAll || strcmp(argv[0], "GetOsVersion") == 0)
        std::cout << "Target GetOsVersion" << std::endl;
    if (printAll || strcmp(argv[0], "GetLibraryInfo") == 0)
        std::cout << "Target GetLibraryInfo <library type>" << std::endl;
    if (printAll || strcmp(argv[0], "GetIsAcerDevice") == 0)
        std::cout << "Target GetIsAcerDevice" << std::endl;
    if (printAll || strcmp(argv[0], "GetDeviceHasCamera") == 0)
        std::cout << "Target GetDeviceHasCamera" << std::endl;
    if (printAll || strcmp(argv[0], "PushFile") == 0)
        std::cout << "Target PushFile local-path target-path" << std::endl;
    if (printAll || strcmp(argv[0], "PullFile") == 0)
        std::cout << "Target PullFile target-path local-path" << std::endl;
    if (printAll || strcmp(argv[0], "RenameFile") == 0)
        std::cout << "Target RenameFile src-path dst-path" << std::endl;
    if (printAll || strcmp(argv[0], "CreateDir") == 0)
        std::cout << "Target Createdir target-path" << std::endl;
    if (printAll || strcmp(argv[0], "RemoveDirRecursive") == 0)
        std::cout << "Target RemoveDirRecursive target-path" << std::endl;
    if (printAll || strcmp(argv[0], "SetPermission") == 0)
        std::cout << "Target SetPermission target-path [r|w|x]" << std::endl;

    return 0;
}

static int target_getAliasPath(int argc, const char* argv[])
{
    if (argc < 1) {
        target_help(argc, argv);
        return -1;
    }
    return target_getAliasPath(argv[1]);
}

static int target_getCcdAppDataPath(int argc, const char* argv[])
{
    int rc = 0;

    std::string ccdAppDataPath;
    rc = target_getCcdAppDataPath(ccdAppDataPath);
    if (rc != 0) {
        LOG_ERROR("Failed to get CCD App Data Path: %d", rc);
        goto end;
    }

    std::cout << "CCD App Data path: " << ccdAppDataPath << std::endl;

 end:
    return rc;
}

static int target_getDeviceName(int argc, const char* argv[])
{
    int rc = 0;

    std::string deviceName;
    rc = target_getDeviceName(deviceName);
    if (rc != 0) {
        LOG_ERROR("Failed to get Device Name: %d", rc);
        goto end;
    }

    std::cout << "Device Name: " << deviceName << std::endl;

 end:
    return rc;
}

static int target_getDeviceClass(int argc, const char* argv[])
{
    int rc = 0;

    std::string deviceClass;
    rc = target_getDeviceClass(deviceClass);
    if (rc != 0) {
        LOG_ERROR("Failed to get Device Class: %d", rc);
        goto end;
    }

    std::cout << "Device Class: " << deviceClass << std::endl;

 end:
    return rc;
}

static int target_getLibraryInfo(int argc, const char* argv[])
{
    if (argc < 1) {
        target_help(argc, argv);
        return -1;
    }
    return target_getLibraryInfo(argv[1]);
}

static int target_getOsVersion(int argc, const char* argv[])
{
    int rc = 0;

    std::string osVersion;
    rc = target_getOsVersion(osVersion);
    if (rc != 0) {
        LOG_ERROR("Failed to get OS Version: %d", rc);
        goto end;
    }

    std::cout << "OS Version: " << osVersion << std::endl;

 end:
    return rc;
}

static int target_getIsAcerDevice(int argc, const char* argv[])
{
    int rc = 0;

    bool isAcerDevice = false;
    rc = target_getIsAcerDevice(isAcerDevice);
    if (rc != 0) {
        LOG_ERROR("Failed to get IsAcerDevice attribute: %d", rc);
        goto end;
    }

    std::cout << "IsAcerDevice: " << isAcerDevice << std::endl;

 end:
    return rc;
}

static int target_getDeviceHasCamera(int argc, const char* argv[])
{
    int rc = 0;

    bool deviceHasCamera = false;
    rc = target_getDeviceHasCamera(deviceHasCamera);
    if (rc != 0) {
        LOG_ERROR("Failed to get DeviceHasCamera attribute: %d", rc);
        goto end;
    }

    std::cout << "DeviceHasCamera: " << deviceHasCamera << std::endl;

 end:
    return rc;
}

static int target_pullFile(int argc, const char* argv[])
{
    if (argc < 3) {
        target_help(argc, argv);
        return -1;
    }

    return target_pullFile(argv[1], argv[2]);
}

static int target_pushFile(int argc, const char* argv[])
{
    if (argc < 3) {
        target_help(argc, argv);
        return -1;
    }

    return target_pushFile(argv[1], argv[2]);
}

static int target_renameFile(int argc, const char* argv[])
{
    if (argc < 3) {
        target_help(argc, argv);
        return -1;
    }

    return target_renameFile(argv[1], argv[2]);
}

static int target_createDir(int argc, const char* argv[])
{
    if (argc < 2) {
        target_help(argc, argv);
        return -1;
    }

    // ignore mode now
    return target_createDir(argv[1], 0777);
}

static int target_removeDirRf(int argc, const char* argv[])
{
    if (argc < 1) {
        target_help(argc, argv);
        return -1;
    }

    return target_removeDirRf(argv[1]);
}

static int target_setPermission(int argc, const char* argv[])
{
    if (argc < 2) {
        target_help(argc, argv);
        return -1;
    }

    return target_setPermission(argv[1], argv[2]);
}

class TargetCmdDispatchTable {
public:
    TargetCmdDispatchTable() {
        cmds["GetAliasPath"]      = target_getAliasPath;
        cmds["GetCCDAppDataPath"] = target_getCcdAppDataPath;
        cmds["GetDeviceName"]     = target_getDeviceName;
        cmds["GetDeviceClass"]    = target_getDeviceClass;
        cmds["GetOsVersion"]      = target_getOsVersion;
        cmds["GetLibraryInfo"]    = target_getLibraryInfo;
        cmds["GetIsAcerDevice"]   = target_getIsAcerDevice;
        cmds["GetDeviceHasCamera"]= target_getDeviceHasCamera;
        cmds["PullFile"]          = target_pullFile;
        cmds["PushFile"]          = target_pushFile;
        cmds["RenameFile"]        = target_renameFile;
        cmds["CreateDir"]         = target_createDir;
        cmds["RemoveDirRf"]       = target_removeDirRf;
        cmds["SetPermission"]     = target_setPermission;
        cmds["Help"]              = target_help;
    }
    std::map<std::string, subcmd_fn> cmds;
};

static TargetCmdDispatchTable targetCmdDispatchTable;

int dispatch_target_cmd(int argc, const char* argv[])
{
    if (argc <= 1)
        return target_help(argc, argv);
    else
        return dispatch(targetCmdDispatchTable.cmds, argc - 1, &argv[1]);
}

