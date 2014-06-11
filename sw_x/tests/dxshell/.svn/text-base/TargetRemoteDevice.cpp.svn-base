#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv
#endif

#include "TargetRemoteDevice.hpp"
#include "RemoteAgent.hpp"
#include "dx_remote_agent.pb.h"

#include "dx_common.h"
#include <vplex_file.h>
#include <vpl_fs.h>
#include <gvm_file_utils.h>
#include "ccd_utils.hpp"

#include <vpl_net.h>
#include <vplex_socket.h>
#include <log.h>

#ifdef LINUX
#include <stdlib.h>
#endif

TargetRemoteDevice::TargetRemoteDevice() :
    ipaddr(0),
    port(0),
    isCached(false),
    socket(VPLSOCKET_INVALID)
{
}

TargetRemoteDevice::TargetRemoteDevice(VPLNet_addr_t ipaddr, u16 port) :
    ipaddr(ipaddr),
    port(port),
    isCached(false)
{
}

TargetRemoteDevice::~TargetRemoteDevice()
{
    FreeSocket();
}

int TargetRemoteDevice::checkIpAddrPort()
{
    if (ipaddr == 0 || port == 0) {
        // TODO: get it from TLS
        ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
        port = 24000;
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<VPLNet_port_t>(strtoul(port_env, NULL, 0));
        }
    }

    return 0;
}

int TargetRemoteDevice::query()
{
    int rc = 0;
    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string request;  // right now, empty string
    std::string response;
    igware::dxshell::QueryDeviceOutput qdev;

    VPLTime_t retry_limit = VPLTime_FromSec(60);
    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t now;

    do
    {
        rc = ragent.send(igware::dxshell::DX_REQUEST_QUERY_DEVICE, request, response);
        if (rc != 0) {
            LOG_ERROR("Failed to request service from dx remote agent: %d", rc);
        }
        else if (!qdev.ParseFromString(response)) {
            LOG_ERROR("Failed to parse protobuf binary message");
        }
        else {
            deviceName      = qdev.device_name();
            deviceClass     = qdev.device_class();
            osVersion       = qdev.os_version();
            isAcerDevice    = qdev.is_acer_device();
            deviceHasCamera = qdev.device_has_camera();
            break;
        }

        now = VPLTime_GetTimeStamp(); 
        VPLThread_Sleep(VPLTime_FromSec(1));
    } while((now - start) < retry_limit);

    isCached = true;

    return rc;
}

std::string TargetRemoteDevice::getDeviceName()
{
    if (!isCached)
        query();
    return deviceName;
}

std::string TargetRemoteDevice::getDeviceClass()
{
    if (!isCached)
        query();
    return deviceClass;
}

std::string TargetRemoteDevice::getOsVersion()
{
    if (!isCached)
        query();
    return osVersion;
}

bool TargetRemoteDevice::getIsAcerDevice()
{
    if (!isCached)
        query();
    return isAcerDevice;
}

bool TargetRemoteDevice::getDeviceHasCamera()
{
    if (!isCached)
        query();
    return deviceHasCamera;
}

bool TargetRemoteDevice::isQueryNoneInfo()
{
    if(deviceName.empty() || deviceClass.empty() || osVersion.empty()) {
        return true;
    }

    return false;
}

int TargetRemoteDevice::pushCcdConfig(const std::string &config)
{
    int rc = VPL_OK;

    std::string pathSeparator;
    rc = getDirectorySeparator(pathSeparator);

    std::string remoteConfPath;
    std::string remoteConfAbsPath;
    rc = getCcdAppDataPath(remoteConfPath);
    if(rc != 0) {
        LOG_ERROR("Fail to get remote app data path:%d", rc);
        return rc;
    }
    remoteConfPath.append(pathSeparator).append("conf");
    rc = createDir(remoteConfPath.c_str(), 0755, true);
    if(rc != 0) {
        LOG_ERROR("Could not create path:%s", remoteConfPath.c_str());
    }
    remoteConfAbsPath = remoteConfPath;
    remoteConfAbsPath.append(pathSeparator).append("ccd.conf");

    deleteFile(remoteConfAbsPath);

    std::string tempfilepath;
    ::getCcdAppDataPath(tempfilepath); //get local data path using ccd_utils
    tempfilepath.append(DIR_DELIM "conf");
    rc = Util_CreatePath(tempfilepath.c_str(), VPL_TRUE);
    if(rc != 0) {
        LOG_ERROR("Unable to create temp path:%s: %d", tempfilepath.c_str(), rc);
        return rc;
    }
    tempfilepath.append(DIR_DELIM "ccd.conf.temp");

    rc = VPLFile_Delete(tempfilepath.c_str());
    if (rc != VPL_OK && rc != VPL_ERR_NOENT) {
        LOG_ERROR("Can't delete temp config file:%s: %d", tempfilepath.c_str(), rc);
        return rc;
    }

    rc = Util_WriteFile(tempfilepath.c_str(), config.data(), config.size());
    if (rc != VPL_OK) {
        LOG_ERROR("Fail to write to temp config file:%s: %d", tempfilepath.c_str(), rc);
        return rc;
    }

    rc = pushFile(tempfilepath, remoteConfAbsPath);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to push ccd config to %s: %d", remoteConfPath.c_str(), rc);
        return rc;
    }

    // Tell the device to push config file to the shared location
    // This request only affects iOS and WinRT since only these 2 platforms use shared object to store the config file
    // There is no method to get the target platform precisely, so we just send the request anyway
    // Other platforms will just simply return success
    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = 24000;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_PUSH_LOCAL_CONF_TO_SHARED_OBJECT);
    input = myReq.SerializeAsString();

    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to request device to push conf to shared location: %d", rc);
        return rc;
    }

    return rc;
}

// This function returns 0 when the "ccd.conf" file is either
//  1) successfully read and config contents are contained within configFileContent_out
//  2) OR the config file is simply missing and the configFileContent_out contains nothing.
int TargetRemoteDevice::pullCcdConfig(std::string &configFileContent_out)
{
    int rc = VPL_OK;
    configFileContent_out.clear();

    std::string pathSeparator;
    VPLFS_stat_t fStat;
    rc = getDirectorySeparator(pathSeparator);

    // Tell the device to pull the config file to the upload directory
    // This request only effects iOS and WinRT since only these 2 platforms use shared object to store the config file
    // There is no method to get the target platform precisely, so we just send the request anyway
    // Other platforms will just simply return success
    VPLNet_addr_t ipaddr = VPLNet_GetAddr(getenv("DX_REMOTE_IP"));
    u16 port = 24000;
    {
        const char *port_env = getenv("DX_REMOTE_PORT");
        if (port_env != NULL) {
            port = static_cast<u16>(strtoul(port_env, NULL, 0));
        }
    }
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_PULL_SHARED_CONF_TO_LOCAL_OBJECT);
    input = myReq.SerializeAsString();

    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to request device to move conf from shared location to upload directory: %d", rc);
        return rc;
    }

    std::string tempfilepath;
    ::getCcdAppDataPath(tempfilepath); //get local data path using ccd_utils
    tempfilepath.append(DIR_DELIM "conf");
    rc = Util_CreatePath(tempfilepath.c_str(), VPL_TRUE);
    if(rc != 0) {
        LOG_ERROR("Unable to create temp path:%s: %d", tempfilepath.c_str(), rc);
        return rc;
    }
    tempfilepath.append(DIR_DELIM "ccd.conf.temp");

    std::string remoteConfPath;
    rc = getCcdAppDataPath(remoteConfPath);
    if(rc != 0) {
        LOG_ERROR("Fail to get remote app data path:%d", rc);
        return rc;
    }
    remoteConfPath.append(pathSeparator).append("conf").append(pathSeparator).append("ccd.conf");

    rc = statFile(remoteConfPath.c_str(), fStat);
    if(rc != VPL_OK) {
        if(rc == VPL_ERR_NOENT) {
            return 0; //initial test may not have ccd.conf
        }

        LOG_ERROR("Stat remoteConfPath \"%s\" failed: (%d), exit pull ccd config" ,
                  remoteConfPath.c_str(), rc);
        return rc;
    }

    rc = pullFile(remoteConfPath, tempfilepath);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to pull ccd conf from %s: %d", remoteConfPath.c_str(), rc);
        return rc;
    }

    char *confbuf = NULL;
    int confsize = 0;
    confsize = Util_ReadFile(tempfilepath.c_str(), (void**)&confbuf, 0);
    if (confsize < 0) {
        rc = confsize;
        LOG_ERROR("Failed to read temp conf file: %d", rc);
        return rc;
    }

    configFileContent_out.assign(confbuf, confsize);

    return rc;
}

int TargetRemoteDevice::removeDeviceCredentials()
{
    int rc = 0;

    LOG_ERROR("NOT YET IMPLEMENTED");

    return rc;
}

int TargetRemoteDevice::getCcdAppDataPath(std::string &path)
{
    int rc = 0;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_GET_CCD_ROOT_PATH);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to request service from dx remote agent: %d", rc);
        goto end;
    }
    else {
        //LOG_ALWAYS("Success to send Get_CCD_App_Data_Path request");
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc == VPL_OK) {
            //LOG_ALWAYS("Success to Success to send Get_CCD_App_Data_Path");
            if (myRes.argument_size() == 0) {
                LOG_ERROR("Faild to parse the CCD_App_Data_Path");
                rc = VPL_ERR_FAIL;
                goto end;
            }
            else {
                //LOG_ALWAYS("Success to parse the CCD_App_Data_Path");
                path = myRes.argument(0).value();
            }
        }
        else {
            LOG_ERROR("Failed to Get_CCD_App_Data_Path");
            goto end;
        }
    }

end:
    return rc;
}

int TargetRemoteDevice::getDxRootPath(std::string &path)
{
    int rc = 0;

    LOG_ERROR("NOT YET IMPLEMENTED");

    return rc;
}

int TargetRemoteDevice::getWorkDir(std::string &path)
{
    int rc = 0;
    std::string tempFolder;
    std::string osVersion;
    const char* tmpFolderName = "dxshell_test";
    std::string input, output;
    std::string pathSeparator;

    rc = getDirectorySeparator(pathSeparator);
    osVersion = getOsVersion();
    if(osVersion.empty()) {
        LOG_ERROR("getOsVersion failed!");
        return -1;
    }
    if(osVersion == OS_WINDOWS_RT) {
        checkIpAddrPort();
        RemoteAgent ragent(ipaddr, (u16)port);
        igware::dxshell::DxRemoteMessage myReq, myRes;
        myReq.set_command(igware::dxshell::DxRemoteMessage_Command_GET_UPLOAD_PATH);
        input = myReq.SerializeAsString();
        int rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
        if (rc != 0) {
            LOG_ALWAYS("Fail to send Get_Upload_Path");
        }
        else {
            myRes.ParseFromString(output);
            rc = myRes.vpl_return_code();
            if (rc == VPL_OK) {
                if (myRes.argument_size() == 0) {
                    LOG_ERROR("Faild to parse the upload path");
                    rc = -1;
                }
                else {
                    std::string uploadPath = myRes.argument(0).value();
                    size_t found = uploadPath.find_last_of(pathSeparator);
                    if (found != std::string::npos) {
                        uploadPath = uploadPath.substr(0, found);
                    }
                    tempFolder = uploadPath.append(pathSeparator).append(tmpFolderName);
                    LOG_ALWAYS("tempFolder: %s", tempFolder.c_str());
                }
            }
            else {
                LOG_ERROR("Failed to Get_Upload_Path");
            }
        }
    }
    else if(osVersion.find(OS_WINDOWS) != std::string::npos || isIOS(osVersion) || osVersion == OS_ANDROID) {
        std::string remoteroot;
        rc = getDxRemoteRoot(remoteroot);
        tempFolder = remoteroot.append(pathSeparator).append(tmpFolderName);
    }
    // linux
    else {
        tempFolder.assign(getenv("HOME"));
        tempFolder = tempFolder.append(pathSeparator).append(tmpFolderName);
    }
    path = tempFolder;
    return rc;
}

int TargetRemoteDevice::getDirectorySeparator(std::string &separator)
{
    int rc = 0;
    std::string osVersion = getOsVersion();
    if (osVersion.empty()) {
        LOG_ERROR("getOsVersion failed!");
        return -1;
    }
    LOG_INFO("osVersion = %s", osVersion.c_str());
    if (osVersion.find(OS_WINDOWS) != std::string::npos) {
        separator.assign("\\");
    }
    else {
        separator.assign("/");
    }
    return rc;
}

int TargetRemoteDevice::getAliasPath(const std::string &alias, std::string &path)
{
    int rc = VPL_OK;

    std::string input, output;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_GET_ALIAS_PATH);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(alias);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != 0) {
        LOG_ERROR("Fail to send out the request to the target device, rc = %d", rc);
        return rc;
    }

    myRes.ParseFromString(output);
    rc = myRes.vpl_return_code();
    if (rc != 0) {
        LOG_ERROR("No path mapping for alias %s, rc = %d", alias.c_str(), rc);
        return rc;
    }
    if (myRes.argument_size() == 0) {
        LOG_ERROR("No value returned from the target device for alias %s", alias.c_str());
        return VPL_ERR_FAIL;
    }

    path = myRes.argument(0).value();

    return rc;
}

int TargetRemoteDevice::createDir(const std::string &targetPath, int mode, bool last)
{
    int rc = 0;
    std::string osVersion;
    std::string tempPath;
    std::string pathSeparator;
    VPLFS_stat_t stat;

    osVersion = getOsVersion();
    if(osVersion.empty()) {
        LOG_ERROR("getOsVersion failed!");
        return -1;
    }
    rc = getDirectorySeparator(pathSeparator);

    size_t found = targetPath.find_first_of(pathSeparator);
    if(found != std::string::npos) {
        while((found = targetPath.find_first_of(pathSeparator, found+1)) != std::string::npos) {
            tempPath = targetPath.substr(0, found);
            
            rc = statFile(tempPath, stat);
            if(rc == VPL_ERR_NOENT){
                rc = createDirHelper(tempPath, mode);
                if (rc != 0) {
                    LOG_ERROR("create dir %s failed: %d", targetPath.c_str(), rc);
                    goto out;
                }
            }
            else if(rc == VPL_ERR_ACCESS) {
                // Access error means something exists. Keep going. A lower directory should provide access.
            }
            else if(rc != 0) { // If something exists, it's OK.
                LOG_ERROR("stat %s failed: %d", tempPath.c_str(), rc);
                goto out;
            }
        }
    }
    
    if (last) {
        rc = createDirHelper(targetPath, mode);
        if (rc != 0) {
            LOG_ERROR("create dir %s failed: %d", targetPath.c_str(), rc);
            goto out;
        }
    }

out:
    return rc;
}

int TargetRemoteDevice::createDirHelper(const std::string &targetPath, int mode)
{
    int rc = 0;
    std::string input, output;
    std::string osVersion;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    osVersion = getOsVersion();
    if(osVersion.empty()) {
        LOG_ERROR("getOsVersion failed!");
        return -1;
    }
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLDIR_CREATE);
    myReq.set_create_dir_mode(mode);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTDIRNAME);
    myArg->set_value(targetPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != 0) {
        LOG_ERROR("Fail to send VPLDir_Create, rc = %d", rc);
        return rc;
    }

    myRes.ParseFromString(output);
    rc = myRes.vpl_return_code();
    if (rc != VPL_OK) {
        if (rc == VPL_ERR_EXIST) {
            rc = VPL_OK;
        }
        else {
            LOG_ERROR ("Failed to VPLDir_Create: %d, %s", rc, targetPath.c_str());
        }
    }

    return rc;
}

int TargetRemoteDevice::readDir(const std::string &targetPath, VPLFS_dirent_t &entry)
{
    int rc = 0;
    if (opendir_set.find(targetPath) == opendir_set.end()) {
        LOG_ERROR("Didn't open dir: %s", targetPath.c_str());
        return VPL_ERR_BADF;
    }
    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFS_READDIR);
    myReq.mutable_dir_folder()->set_alias(get_target_machine());
    myReq.mutable_dir_folder()->set_dir_path(targetPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != 0) {
        LOG_ERROR("Failed to send VPLFS_Readdir, %d, %s", rc, targetPath.c_str());
    }
    else {
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc == VPL_OK) {
            entry.type = (VPLFS_file_type_t)(int)myRes.mutable_folderdirent()->type();
            #ifdef WIN32
            strncpy_s(entry.filename, _countof(entry.filename), myRes.mutable_folderdirent()->filename().c_str(), myRes.mutable_folderdirent()->filename().size());
            #else
            std::copy(myRes.mutable_folderdirent()->filename().begin(), myRes.mutable_folderdirent()->filename().end(), entry.filename);
            entry.filename[myRes.mutable_folderdirent()->filename().size()] = '\0';
            #endif
        }
    }

    return rc;
}

int TargetRemoteDevice::removeDirRecursive(const std::string &targetPath)
{
    int rc = 0;
    std::string input, output;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_UTIL_RM_DASH_RF);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTDIRNAME);
    myArg->set_value(targetPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != 0) {
        LOG_ERROR("Fail to send Util_rm_dash_rf, rc = %d", rc);
        return rc;
    }

    myRes.ParseFromString(output);
    rc = myRes.vpl_return_code();
    if (rc != VPL_OK) {
        LOG_ERROR ("Failed to Util_rm_dash_rf: %d", rc);
    }

    return rc;
}

int TargetRemoteDevice::openDir(const std::string &targetPath)
{
    if (opendir_set.find(targetPath) != opendir_set.end()) {
        return VPL_OK;
    }

    int rc = 0;
    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFS_OPENDIR);
    myReq.mutable_dir_folder()->set_alias(get_target_machine());
    myReq.mutable_dir_folder()->set_dir_path(targetPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != 0) {
        LOG_ERROR("Fail to send VPLFS_Opendir, rc = %d", rc);
    }
    else {
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc == VPL_OK) {
            opendir_set.insert(targetPath);
        }
    }

    return rc;
}

int TargetRemoteDevice::closeDir(const std::string &targetPath)
{
    if (opendir_set.find(targetPath) == opendir_set.end()) {
        return VPL_ERR_BADF;
    }

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFS_CLOSEDIR);
    myReq.mutable_dir_folder()->set_alias(get_target_machine());
    myReq.mutable_dir_folder()->set_dir_path(targetPath);
    input = myReq.SerializeAsString();
    int rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != 0) {
        LOG_ERROR("Failed to VPLFS_Closedir, %d, %s", rc, targetPath.c_str());
    }
    else {
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc == VPL_OK) {
            opendir_set.erase(targetPath);
        }
        else {
            LOG_ERROR("Failed to VPLFS_Closedir, %d, %s", rc, targetPath.c_str());
        }
    }

    return rc;
}

int TargetRemoteDevice::readLibrary(const std::string &library_type, std::vector<std::pair<std::string, std::string> > &folders)
{
    int rc = VPL_OK;

    std::string input, output;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_READ_LIBRARY);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(library_type);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != 0) {
        LOG_ERROR("Fail to send out the request to the target device, rc = %d", rc);
        return rc;
    }
    myRes.ParseFromString(output);
    rc = myRes.vpl_return_code();
    if (rc != 0) {
        LOG_ERROR("No library info found for type %s, rc = %d", library_type.c_str(), rc);
        return rc;
    }
    for (int i = 0; i < myRes.lib_info_size(); i++) {
        const igware::dxshell::DxRemoteMessage_DxRemote_LibraryInfo info = myRes.lib_info(i);
        //LOG_ALWAYS("lib[%d], type = %s, real = %s, virt = %s",
        //           i, info.type().c_str(), info.real_path().c_str(), info.virt_path().c_str());
        folders.push_back(std::make_pair(info.real_path(), info.virt_path()));
    }
    return rc;
}

int TargetRemoteDevice::pushFile(const std::string &hostPath, const std::string &targetPath)
{
    // No pulse limit, no delays.
    return pushFileGeneric(hostPath, targetPath, 0, 0);
}

int TargetRemoteDevice::pushFileSlow(const std::string &hostPath, const std::string &targetPath,
    const int timeMs, const int pulseSizeKb)
{
    return pushFileGeneric(hostPath, targetPath, timeMs, pulseSizeKb);
}

int TargetRemoteDevice::pushFileGeneric(const std::string &hostPath, const std::string &targetPath,
    const int timeMs, const int pulseSizeKb)
{
    int rv = VPL_OK;
    VPLFile_handle_t fHandle = VPLFILE_INVALID_HANDLE;
    VPLFS_stat_t fStat;
    VPLFS_file_size_t sourceSize = 0, remoteRecv = 0, fileRead = 0;
    igware::dxshell::DxRemoteFileTransfer myReq, myRes;
    uint32_t resSize = 0;
    char tempChunkBuf[igware::dxshell::DX_REMOTE_FILE_TRANS_PKT_SIZE];
    ssize_t bytesRead = 0;
    ssize_t bytesSent = 0;
    u8 reqType = (u8)igware::dxshell::DX_REQUEST_DXREMOTE_TRANSFER_FILES;
    uint64_t bytesReceived;
    u32 pulseSent = 0;

    rv = VPLFS_Stat(hostPath.c_str(), &fStat);
    if(rv != VPL_OK) {
        LOG_ERROR("File %s doesn't exist, rv = %d", hostPath.c_str(), rv);
        goto exit;
    }
    sourceSize = fStat.size;
    LOG_ALWAYS("sourceSize:"FMTu_VPLFS_file_size_t, sourceSize);
    
    fHandle = VPLFile_Open(hostPath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fHandle)) {
        LOG_ERROR("VPLFile_Open %s fail", hostPath.c_str());
        rv = VPL_ERR_INVALID;
        goto exit;
    }

    myReq.set_type(igware::dxshell::DX_REMOTE_PUSH_FILE);
    myReq.set_vpl_return_code(VPL_OK);
    myReq.set_path_on_agent(targetPath);
    myReq.set_file_size(sourceSize);

    checkIpAddrPort();
    rv = InitSocket();
    if (rv != VPL_OK) {
        LOG_ERROR("InitSocket failed: %d", rv);
        goto exit;
    }

    rv = VPLSocket_Write(socket, &reqType, sizeof(reqType), VPL_TIMEOUT_NONE);
    if(rv < 0) {
        LOG_ERROR("Send reqType failed: %d", rv);
        goto exit;
    }
    else if(rv == 0) {
        LOG_ERROR("Send reqType failed on disconnect.");
        rv = VPL_ERR_IO;
        goto exit;
    }

    rv = SendProtoSize(myReq.ByteSize());
    if (rv != VPL_OK) {
        LOG_ERROR("SendProtoSize failed: %d", rv);
        goto exit;
    }

    rv = SendProtoRequest(myReq);
    if (rv != VPL_OK) {
        LOG_ERROR("SendProtoRequest failed: %d", rv);
        goto exit;
    }

    rv = RecvProtoSize(resSize);
    if (rv != VPL_OK) {
        LOG_ERROR("RecvProtoSize failed: %d", rv);
        goto exit;
    }
    
    rv = RecvProtoResponse(resSize, myRes);
    if (rv != VPL_OK) {
        LOG_ERROR("RecvProtoResponse failed: %d", rv);
        goto exit;
    }
    
    if (myRes.vpl_return_code() != VPL_OK) {
        rv = myRes.vpl_return_code();
        LOG_ERROR("vpl_return_code() is not VPL_OK: %d", rv);
        goto exit;
    }

    // Send file data. If file I/O error occurs, must drop connection. 
    // File will be partially written on other side.
    // Stop on I/O error.
    // Meter the send activity according to parameters (pulseSizeKb,timeMs)
    while(fileRead < sourceSize) {
        size_t readLimit = sizeof(tempChunkBuf);
        if(readLimit > (sourceSize - fileRead)) {
            readLimit = (size_t)(sourceSize - fileRead);
        }
        bytesRead = VPLFile_Read(fHandle, tempChunkBuf, readLimit);
        if (bytesRead <= 0) {
            LOG_ERROR("VPLFile_Read failed %d", bytesRead);
            rv = VPL_ERR_IO;
            goto exit;
        }
        
        fileRead += bytesRead;
        bytesSent = 0;

        // If requested, send the buffer in pulses, with a sleep between each pulse.
        // This is useful for testing camera roll upload behavior when the file is written incrementally.
        // For other cases, pulseSizeKb should be 0.
        while(bytesSent < bytesRead) {
            // ((pulseSizeKb * 1000) == pulseSent) checks if a complete pulse has been sent.
            if(pulseSizeKb > 0 && (pulseSizeKb * 1000) == pulseSent) {
                VPLThread_Sleep(VPLTIME_FROM_MILLISEC(timeMs));
                pulseSent = 0;
            }
            size_t transferBytes = (bytesRead - bytesSent);
            // ((pulseSizeKb * 1000) - pulseSent) is the amount remaining to send for the current
            // pulse.
            // TODO: Now that we use VPLSocket_Write, pulseSent should always be 0 at this point,
            //   since we will either send the entire pulse or error out.
            if((pulseSizeKb > 0) && ((pulseSizeKb * 1000) - pulseSent) < transferBytes) {
                transferBytes = ((pulseSizeKb * 1000) - pulseSent);
            }
            rv = VPLSocket_Write(socket, tempChunkBuf + bytesSent, transferBytes, VPL_TIMEOUT_NONE);
            if(rv < 0) {
                LOG_ERROR("VPLSocket_Write failed %d", rv);
                goto exit;
            }
            else if(rv == 0) {
                LOG_ERROR("VPLSocket_Write detects connection lost.");
                rv = VPL_ERR_IO;
                goto exit;
            }
            else if(rv != transferBytes) {
                LOG_ERROR("VPLSocket_Write sent "FMT_size_t"/"FMT_size_t".",
                          rv, transferBytes);
                rv = VPL_ERR_IO;
                goto exit;
            }
            pulseSent += rv;
            bytesSent += rv;
        }
    }

    rv = VPLSocket_Read(socket, &bytesReceived, sizeof(bytesReceived), VPL_TIMEOUT_NONE);
    if(rv < 0) {
        LOG_ERROR("VPLSocket_Read failed %d", rv);
        goto exit;
    }
    else if(rv == 0) {
        LOG_ERROR("VPLSocket_Read detects connection lost.");
        rv = VPL_ERR_IO;
        goto exit;
    }
    else if(rv != sizeof(bytesReceived)) {
        LOG_ERROR("VPLSocket_Read sent truncated buffer: "FMT_size_t"/"FMT_size_t".",
                  rv, sizeof(bytesReceived));
        rv = VPL_ERR_IO;
        goto exit;
    }

    remoteRecv = VPLConv_ntoh_u64(bytesReceived);
    rv = (remoteRecv == sourceSize)? VPL_OK : VPL_ERR_FAIL;
    if(remoteRecv != sourceSize){
        LOG_ERROR("size mismatch, remoteRecv:"FMTu64", sourceSize:"FMTu64, remoteRecv, sourceSize);
    }

 exit:
    if (VPLFile_IsValidHandle(fHandle)) {
        VPLFile_Close(fHandle);
        fHandle = VPLFILE_INVALID_HANDLE;
    }

    FreeSocket();

    return rv;
}

int TargetRemoteDevice::pullFile(const std::string &targetPath, const std::string &hostPath)
{
    int rv = VPL_OK;
    int rc;
    VPLFile_handle_t fHandle = VPLFILE_INVALID_HANDLE;
    VPLFS_file_size_t sourceSize = 0;
    igware::dxshell::DxRemoteFileTransfer myReq, myRes;
    uint32_t resSize = 0;
    u8 reqType = (u8)igware::dxshell::DX_REQUEST_DXREMOTE_TRANSFER_FILES;
    const int flagDst = VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY;
    char tempChunkBuf[igware::dxshell::DX_REMOTE_FILE_TRANS_PKT_SIZE];
    uint64_t recvSize = 0;
    uint64_t fileSize = 0;
    bool writeFail = false;

    fHandle = VPLFile_Open(hostPath.c_str(), flagDst, 0666);
    if (!VPLFile_IsValidHandle(fHandle)) {
        LOG_ERROR("VPLFile_Open %s failed: %d", hostPath.c_str(), fHandle);
        rv = VPL_ERR_INVALID;
        goto exit;
    }

    myReq.set_type(igware::dxshell::DX_REMOTE_GET_FILE);
    myReq.set_vpl_return_code(VPL_OK);
    myReq.set_path_on_agent(targetPath);

    checkIpAddrPort();
    rv = InitSocket();
    if (rv != VPL_OK) {
        LOG_ERROR("InitSocket failed: %d", rv);
        goto exit;
    }

    rv = VPLSocket_Write(socket, &reqType, sizeof(reqType), VPL_TIMEOUT_NONE);
    if(rv < 0) {
        LOG_ERROR("Send reqType failed: %d", rv);
        goto exit;
    }
    else if(rv == 0) {
        LOG_ERROR("Send reqType failed on disconnect.");
        rv = VPL_ERR_IO;
        goto exit;
    }

    rv = SendProtoSize(myReq.ByteSize());
    if (rv != VPL_OK) {
        LOG_ERROR("SendProtoSize failed: %d", rv);
        goto exit;
    }

    rv = SendProtoRequest(myReq);
    if (rv != VPL_OK) {
        LOG_ERROR("SendProtoRequest failed: %d", rv);
        goto exit;
    }

    rv = RecvProtoSize(resSize);
    if (rv != VPL_OK) {
        LOG_ERROR("SendProtoSize failed: %d", rv);
        goto exit;
    }
    
    rv = RecvProtoResponse(resSize, myRes);
    if (rv != VPL_OK) {
        LOG_ERROR("SendProtoResponse failed: %d", rv);
        goto exit;
    }

    if (myRes.vpl_return_code() != VPL_OK) {
        rv = myRes.vpl_return_code();
        LOG_ERROR("vpl_return_code() is not VPL_OK: %d", rv);
        goto exit;
    }

    sourceSize = myRes.file_size();
    LOG_ALWAYS("sourceSize:"FMTu_VPLFS_file_size_t, sourceSize);
        
    // Receive file data. Must receive all contracted data.
    // If receive fails, truncate file.
    while(recvSize < sourceSize) {
        size_t recvLimit = sizeof(tempChunkBuf);
        if(recvLimit > (sourceSize - recvSize)) {
            recvLimit = (size_t)(sourceSize - recvSize);
        }
        int recvCnt = VPLSocket_Recv(socket, tempChunkBuf, recvLimit);
        if(recvCnt < 0) {
            LOG_ERROR("I/O error receiving file data: (%d)", recvCnt);
            rv = recvCnt;
            goto exit;
        }
        else if(recvCnt == 0) {
            LOG_ERROR("Connection lost receiving file data.");
            rv = VPL_ERR_IO;
            goto exit;
        }
        recvSize += recvCnt;

        if(!writeFail) {
            int writeCnt = 0;
            while(writeCnt < recvCnt) {
                rc = VPLFile_Write(fHandle, tempChunkBuf + writeCnt, recvCnt - writeCnt);
                if(rc < 0) {
                    LOG_ERROR("Disk I/O error writing file data: (%d)", rc);
                    writeFail = true;
                    break;
                }
                else {
                    writeCnt += rc;
                }
            }
            fileSize += writeCnt;
        }
    }
    
    fileSize = VPLConv_hton_u64(fileSize);
    rc = VPLSocket_Write(socket, &fileSize, sizeof(fileSize), VPL_TIMEOUT_NONE);
    if(rc < 0) {
        rv = rc;
        LOG_ERROR("I/O error sending received file size: (%d).", rv);        
    }
    else if(rc != sizeof(fileSize)) {
        LOG_ERROR("Short send of received file size. %d/"FMT_size_t".",
                  rc, sizeof(fileSize));
        rv = -1;
    }

    rv = (recvSize == sourceSize)? VPL_OK : VPL_ERR_FAIL;
    if(recvSize != sourceSize){
        LOG_ERROR("size mismatch, received:"FMTu64", sourceSize:"FMTu64, recvSize, sourceSize);
    }
    

 exit:
    if (VPLFile_IsValidHandle(fHandle)) {
        VPLFile_Close(fHandle);
    }

    FreeSocket();

    return rv;
}

int TargetRemoteDevice::deleteFile(const std::string &targetPath)
{
    int rc = 0;
    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFILE_DELETE);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(targetPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Fail to send VPLFile_Delete: file:%s :%d", targetPath.c_str(), rc);
    }
    else {
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc != VPL_OK) {
            LOG_ERROR("Failed to VPLFile_Delete: file:%s :%d", targetPath.c_str(), rc);
        }
    }

    return rc;
}

int TargetRemoteDevice::touchFile(const std::string &targetPath)
{
    int rc = 0;
    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFILE_TOUCH);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(targetPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Fail to send VPLFile_Touch: file:%s :%d", targetPath.c_str(), rc);
    }
    else {
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc != VPL_OK) {
            LOG_ERROR("Failed to VPLFile_Touch: file:%s :%d", targetPath.c_str(), rc);
        }
    }

    return rc;
}

int TargetRemoteDevice::statFile(const std::string &targetPath, VPLFS_stat_t &stat)
{
    int rc = 0;
    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFS_STAT);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(targetPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Fail to send VPLFS_Stat, rc = %d", rc);
    }
    else {
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc == VPL_OK) {
            stat.size = myRes.file_stat().size();
            stat.type = (VPLFS_file_type_t)(int)myRes.file_stat().type();
            stat.atime = myRes.file_stat().atime();
            stat.mtime = myRes.file_stat().mtime();
            stat.ctime = myRes.file_stat().ctime();
            stat.isHidden = myRes.file_stat().ishidden();
            stat.isSymLink = myRes.file_stat().issymlink();
        }
        else if ((rc == VPL_ERR_ACCESS) || (rc == VPL_ERR_NOENT)) {
            // VPL_ERR_ACCESS is expected for some cases on WinRT.
            // VPL_ERR_NOENT is expected for some cases on all platforms.
            // The caller of this function should log an error if it is not
            // an expected case.
            LOG_INFO("VPLFS_Stat(%s):%d (may be OK)", targetPath.c_str(), rc);
        } else {
            LOG_ERROR("VPLFS_Stat(%s):%d", targetPath.c_str(), rc);
        }
    }

    return rc;
}

int TargetRemoteDevice::renameFile(const std::string &srcPath, const std::string &dstPath)
{
    int rc = 0;
    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFILE_RENAME);
    myReq.set_rename_source(srcPath);
    myReq.set_rename_destination(dstPath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Fail to send VPLFile_Rename");
    }
    else {
        myRes.ParseFromString(output);
        rc = myRes.vpl_return_code();
        if (rc != VPL_OK) {
            LOG_ERROR ("Failed to VPLFile_Rename: %d", rc);
        }
    }

    return rc;
}

int TargetRemoteDevice::pushFileContent(const std::string &content, const std::string &targetPath)
{
    int rc = 0;

    LOG_ERROR("NOT YET IMPLEMENTED");

    return rc;
}

int TargetRemoteDevice::pullFileContent(const std::string &targetPath, std::string &content)
{
    int rc = 0;

    LOG_ERROR("NOT YET IMPLEMENTED");

    return rc;
}

int TargetRemoteDevice::InitSocket()
{
    int rc = VPL_OK;
    sockAddr.family = VPL_PF_INET;
    sockAddr.addr = ipaddr;
    sockAddr.port = VPLNet_port_hton(port);
    FreeSocket();
    socket = VPLSocket_Create( VPL_PF_INET, VPLSOCKET_STREAM, /*block*/0 );
    if (!VPLSocket_Equal(socket,VPLSOCKET_INVALID)) {
        VPLSocket_SetSockOpt( socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR, (void*)&reUse, sizeof(reUse));
        VPLSocket_SetSockOpt( socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, (void*)&noDelay, sizeof(noDelay));
        rc = VPLSocket_ConnectWithTimeout(socket, &sockAddr, sizeof(sockAddr),
                                          VPLTime_FromSec(30));
    } else {
        LOG_ERROR("Fail to create socket");
        rc = VPL_ERR_FAIL;
    }
    return rc;
}

void TargetRemoteDevice::FreeSocket()
{
    if (!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
    }
}

int TargetRemoteDevice::SendProtoSize(uint32_t reqSize)
{
    int rv = VPL_OK;
    uint32_t size = VPLConv_hton_u32(reqSize);
    int rc = VPLSocket_Write(socket, &size, sizeof(size), VPL_TIMEOUT_NONE);
    if(rc < 0) {
        rv = rc;
    }
    else if(rc != sizeof(size)) {
        rv = VPL_ERR_IO;
    }

    return rv;
}

int TargetRemoteDevice::SendProtoRequest(igware::dxshell::DxRemoteFileTransfer &myReq)
{
    int rc;
    int rv = VPL_OK;
    char *data = NULL;

    try {
        data = new char[myReq.ByteSize()];

        if (!myReq.SerializeToArray(data, myReq.ByteSize())) {
            rc = VPL_ERR_FAIL;
            goto exit;
        }
        
        rc = VPLSocket_Write(socket, data, myReq.ByteSize(), VPL_TIMEOUT_NONE);
        if(rc < 0) {
            LOG_ERROR("Write error: %d.", rc);
            rv = rc;
        }
        else if(rc == 0) {
            LOG_ERROR("Connection lost.");
            rv = VPL_ERR_IO;
        }
        else if (rc != myReq.ByteSize()) {
            rv = VPL_ERR_IO;
        }
    }
    catch(std::exception& e) {
        LOG_ERROR("Fatal exception sending proto request: %s", e.what());
        rv = -1;
    }

 exit:
    delete[] data;

    return rv;
}

int TargetRemoteDevice::RecvProtoSize(uint32_t &resSize)
{
    int rv = VPL_OK;
    int rc;
    uint32_t size = 0;
    rc = VPLSocket_Read(socket, &size, sizeof(size), VPL_TIMEOUT_NONE);
    if(rc < 0) {
        LOG_ERROR("Read error: %d.", rc);
        rv = rc;
    }
    else if(rc == 0) {
        LOG_ERROR("Connection lost.");
        rv = VPL_ERR_IO;
    }
    else if(rc != sizeof(size)) {
        rv = VPL_ERR_IO;
    }
    else {
        resSize = VPLConv_ntoh_u32(size);
    }

    return rv;
}

int TargetRemoteDevice::RecvProtoResponse(uint32_t resSize, igware::dxshell::DxRemoteFileTransfer &myRes)
{
    int rv = VPL_OK;
    int rc;
    char *data = NULL;

    try {
        data = new char[resSize];

        rc = VPLSocket_Read(socket, data, resSize, VPL_TIMEOUT_NONE);
        if(rc < 0) {
            LOG_ERROR("Read error: %d.", rc);
            rv = rc;
        }
        else if(rc == 0) {
            LOG_ERROR("Connection lost.");
            rv = VPL_ERR_IO;
        }
        else if(rc != resSize) {
            rv = VPL_ERR_IO;
        }
        else {
            if(!myRes.ParseFromArray(data, resSize)) {
                rv = VPL_ERR_FAIL;
            }
        }
    }
    catch(std::exception& e) {
        LOG_ERROR("Fatal exception receiving proto request: %s", e.what());
        rv = -1;
    }

    delete[] data;

    return rv;
}

int TargetRemoteDevice::getFileSize(const std::string &filepath, uint64_t &fileSize)
{
    int rc = 0;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_VPLFS_STAT);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(filepath);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to request service from dx remote agent: %d", rc);
        goto end;
    }

    if (!myRes.ParseFromString(output)) {
        LOG_ERROR("Failed to parse protobuf binary message");
        rc = -1;
        goto end;
    }

    rc = myRes.vpl_return_code();
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to do VPLFS_Stat on dx remote agent: %d", rc);
        goto end;
    }

    fileSize = myRes.file_stat().size();

end:
    return rc;
}

int TargetRemoteDevice::getDxRemoteRoot(std::string &path)
{
    int rc = 0;
    std::string uploadpath;
    std::string uploadToken = std::string("dxshell_pushfiles");
    std::string::size_type uploadTokenIdx = 0;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_GET_UPLOAD_PATH);
    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to request service from dx remote agent: %d", rc);
        goto end;
    }

    if (!myRes.ParseFromString(output)) {
        rc = -1;
        LOG_ERROR("Failed to parse protobuf binary message");
        goto end;
    }

    rc = myRes.vpl_return_code();
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to get dx_remote_root on dx remote agent: %d", rc);
        goto end;
    }

    uploadpath = myRes.argument(0).value();
    uploadTokenIdx = uploadpath.find(uploadToken);
    if (uploadTokenIdx == std::string::npos || uploadTokenIdx == 0) {
        rc = -1;
        LOG_ERROR("Failed to get dx_remote_root on dx remote agent: %d", rc);
        goto end;
    }

    path = uploadpath.substr(0, uploadTokenIdx - 1);

end:
    return rc;
}

MMError TargetRemoteDevice::MSABeginCatalog(const ccd::BeginCatalogInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSABeginCatalog,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSABeginCatalog failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSACommitCatalog(const ccd::CommitCatalogInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSACommitCatalog,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSACommitCatalog failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSAEndCatalog(const ccd::EndCatalogInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSAEndCatalog,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSAEndCatalog failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSABeginMetadataTransaction,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSABeginMetadataTransaction failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSAUpdateMetadata(const ccd::UpdateMetadataInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSAUpdateMetadata,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSAUpdateMetadata failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSADeleteMetadata(const ccd::DeleteMetadataInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSADeleteMetadata,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSADeleteMetadata failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSACommitMetadataTransaction(void)
{
    int rc = 0;

    std::string dummy_request, dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSACommitMetadataTransaction,
                           dummy_request, dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSACommitMetadataTransaction failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSAGetMetadataSyncState(media_metadata::GetMetadataSyncStateOutput& output)
{
    int rc = 0;

    std::string dummy_request, response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSAGetMetadataSyncState,
                           dummy_request, response);
    if (rc != 0) {
        LOG_ERROR("MSAGetMetadataSyncState failed; %d", rc);
    }
    else {
        output.ParseFromString(response);
    }

    return rc;
}

MMError TargetRemoteDevice::MSADeleteCollection(const ccd::DeleteCollectionInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSADeleteCollection,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSADeleteCollection failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSADeleteCatalog(const ccd::DeleteCatalogInput& input)
{
    int rc = 0;

    std::string dummy_response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSADeleteCatalog,
                           input.SerializeAsString(),
                           dummy_response);
    if (rc != 0) {
        LOG_ERROR("MSADeleteCatalog failed; %d", rc);
    }

    return rc;
}

MMError TargetRemoteDevice::MSAListCollections(media_metadata::ListCollectionsOutput& output)
{
    int rc = 0;

    std::string dummy_request, response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSAListCollections,
                           dummy_request, response);
    if (rc != 0) {
        LOG_ERROR("MSAListCollections failed; %d", rc);
    }
    else {
        output.ParseFromString(response);
    }

    return rc;
}

MMError TargetRemoteDevice::MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& input,
                                                    ccd::GetCollectionDetailsOutput& output)
{
    int rc = 0;

    std::string response;
    rc = msaGenericHandler(igware::dxshell::DxRemoteMSA_Function_MSAGetCollectionDetails,
                           input.SerializeAsString(),
                           response);
    if (rc != 0) {
        LOG_ERROR("MSAGetCollectionDetails failed; %d", rc);
    }
    else {
        output.ParseFromString(response);
    }

    return rc;
}

int TargetRemoteDevice::msaGenericHandler(igware::dxshell::DxRemoteMSA_Function func, const std::string &input, std::string &output)
{
    int rc = 0;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    igware::dxshell::DxRemoteMSA dxmsa_in, dxmsa_out;
    dxmsa_in.set_func(func);
    dxmsa_in.set_msa_input(input);

    std::string response;
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_MSA, dxmsa_in.SerializeAsString(), response);
    if (rc != 0) {
        LOG_ERROR("Failed to send request to DxRA: %d", rc);
        goto out;
    }

    dxmsa_out.ParseFromString(response);
    rc = dxmsa_out.func_return();
    output.assign(dxmsa_out.msa_output());

 out:
    return rc;
}

int TargetRemoteDevice::tsTest(const TSTestParameters& test, TSTestResult& result)
{
    int rc = 0;
    igware::dxshell::DxRemoteTSTest req;
    igware::dxshell::DxRemoteTSTest res;
    igware::dxshell::DxRemoteTSTest::TSOpenParms* ts_open_parms = req.mutable_ts_open_parms();

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    ts_open_parms->set_user_id (test.tsOpenParms.user_id);
    ts_open_parms->set_device_id (test.tsOpenParms.device_id);
    ts_open_parms->set_service_name  (test.tsOpenParms.service_name);
    ts_open_parms->set_credentials  (test.tsOpenParms.credentials);
    ts_open_parms->set_flags  (test.tsOpenParms.flags);
    ts_open_parms->set_timeout(test.tsOpenParms.timeout);

    req.set_test_id(test.testId);
    req.set_log_enable_level(test.logEnableLevel);
    req.set_xfer_cnt(test.xfer_cnt);
    req.set_xfer_size(test.xfer_size);
    req.set_num_test_iterations(test.nTestIterations);
    req.set_num_clients(test.nClients);
    req.set_client_write_delay(test.client_write_delay);
    req.set_server_read_delay(test.server_read_delay);

    std::string response;
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_TS_TEST, req.SerializeAsString(), response);
    if (rc != 0) {
        LOG_ERROR("Request to run remote TS test failed: %d", rc);
        goto out;
    }

    if (!res.ParseFromString(response)) {;
        LOG_ERROR("Failed to parse protobuf binary message");
        rc = -1;
        goto out;
    }

    result.return_value = res.return_value();
    result.error_msg = res.error_msg();

 out:
    return rc;
}

int TargetRemoteDevice::checkRemoteAgent(void)
{
    int rc = VPL_OK;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);
    rc = ragent.checkSocketConnect();
    if (rc != 0) {
        LOG_ERROR("Fail to send out the request to the target device, rc = %d", rc);
    }

    return rc;
}

int TargetRemoteDevice::setFilePermission(const std::string &path, const std::string &mode)
{
    int rc = 0;

    checkIpAddrPort();
    RemoteAgent ragent(ipaddr, (u16)port);

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_SET_PERMISSION);
    //setup path
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(path);
    //setup mode
    myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENT_NONE);
    myArg->set_value(mode);

    input = myReq.SerializeAsString();
    rc = ragent.send(igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL, input, output);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to request service from dx remote agent: %d", rc);
        goto end;
    }

    if (!myRes.ParseFromString(output)) {
        LOG_ERROR("Failed to parse protobuf binary message");
        rc = -1;
        goto end;
    }

    rc = myRes.vpl_return_code();
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to do VPLFS_Stat on dx remote agent: %d", rc);
        goto end;
    }


end:
    return rc;
}

