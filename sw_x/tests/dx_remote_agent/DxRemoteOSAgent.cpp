#include "DxRemoteOSAgent.hpp"
#include "dx_remote_agent_util.h"
#include <vplex_file.h>
#include <vplex_shared_object.h>
#include "log.h"

#include <algorithm>
#include <sstream>
#include <queue>

#ifdef VPL_PLAT_IS_WINRT
#include "dx_remote_agent_util_winrt.h"
#include <ppltasks.h>
using namespace Concurrency;
using namespace Windows::Storage;
#elif defined(IOS)
#include "dx_remote_agent_util_ios.h"
#elif defined (WIN32)
#include <dx_remote_agent_util_win.h>
#elif defined (LINUX)
#include <dx_remote_agent_util_linux.h>
#else
#endif
#include "log.h"

#if defined(WIN32) || defined(VPL_PLAT_IS_WINRT)
#define DIR_DELIM "\\"
#else
#define DIR_DELIM "/"
#endif

#include <gvm_file_utils.h>
#include <gvm_file_utils.hpp>
#include "common_utils.hpp"

std::map<std::string, VPLFS_dir_t> DxRemoteOSAgent::fsdir_map;

DxRemoteOSAgent::DxRemoteOSAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) : IDxRemoteAgent(skt, buf, pktSize)
{
}

DxRemoteOSAgent::~DxRemoteOSAgent()
{
}

int DxRemoteOSAgent::doAction()
{
    int rc;

    LOG_DEBUG("DxRemoteOSAgent::doAction() Start");
    igware::dxshell::DxRemoteMessage cliReq;
    cliReq.ParseFromArray(recvBuf, recvBufSize);

    igware::dxshell::DxRemoteMessage resToCli;
    resToCli.set_command(cliReq.command());

    switch(cliReq.command()) {
    case igware::dxshell::DxRemoteMessage_Command_LAUNCH_PROCESS:
        LOG_INFO("DxRemoteMessage_Command_LAUNCH_PROCESS:%s",
                 cliReq.argument(0).value().c_str());
        rc = DxRemote_Launch_Process(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_KILL_PROCESS:
        LOG_INFO("DxRemoteMessage_Command_KILL_PROCESS:%s",
                 cliReq.argument(0).value().c_str());
        rc = DxRemote_Kill_Process(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_GET_CONNECTED_ANDROID_IP:
        LOG_INFO("DxRemoteMessage_Command_GET_CONNECTED_ANDROID_IP");
        rc = DxRemote_Get_Connected_Android_IP(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_LAUNCH_CONNECTED_ANDROID_DXREMOTEAGENT:
        LOG_INFO("DxRemoteMessage_Command_LAUNCH_CONNECTED_ANDROID_DXREMOTEAGENT");
        rc = DxRemote_Launch_Connected_Android_RemoteAgent();
        break;
        
    case igware::dxshell::DxRemoteMessage_Command_CLEAN_CC:
        LOG_INFO("DxRemoteMessage_Command_CLEAN_CC");
        rc = DxRemote_CleanCC();
        break;

    case igware::dxshell::DxRemoteMessage_Command_PUSH_LOCAL_CONF_TO_SHARED_OBJECT:
        LOG_INFO("DxRemoteMessage_Command_PUSH_LOCAL_CONF_TO_SHARED_OBJECT");
        rc = DxRemote_Push_Local_Conf_To_Shared_Object(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_PULL_SHARED_CONF_TO_LOCAL_OBJECT:
        LOG_INFO("DxRemoteMessage_Command_PULL_SHARED_CONF_TO_LOCAL_OBJECT");
        rc = DxRemote_Pull_Shared_Conf_To_Local_Object(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLFS_OPENDIR:
        LOG_INFO("DxRemoteMessage_Command_VPLFS_OPENDIR:%s",
                 cliReq.dir_folder().dir_path().c_str());
        rc = this->DxRemote_VPLFS_Opendir(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLFS_READDIR:
        LOG_INFO("DxRemoteMessage_Command_VPLFS_READDIR:%s",
                 cliReq.dir_folder().dir_path().c_str());
        rc = this->DxRemote_VPLFS_Readdir(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLFS_CLOSEDIR:
        LOG_INFO("DxRemoteMessage_Command_VPLFS_CLOSEDIR:%s",
                 cliReq.dir_folder().dir_path().c_str());
        rc = this->DxRemote_VPLFS_Closedir(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLFS_STAT:
        LOG_INFO("DxRemoteMessage_Command_VPLFS_STAT:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_VPLFS_Stat(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_UTIL_RM_DASH_RF:
        LOG_INFO("DxRemoteMessage_Command_UTIL_RM_DASH_RF:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_Util_rm_dash_rf(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLDIR_CREATE:
        LOG_INFO("DxRemoteMessage_Command_VPLDIR_CREATE:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_VPLDir_Create(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLFILE_RENAME:
        LOG_INFO("DxRemoteMessage_Command_VPLFILE_RENAME:%s to %s",
                 cliReq.rename_source().c_str(), cliReq.rename_destination().c_str());
        rc = this->DxRemote_VPLFile_Rename(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_COPYFILE:
        LOG_INFO("DxRemoteMessage_Command_COPYFILE:%s to %s",
                 cliReq.argument(0).value().c_str(), cliReq.argument(1).value().c_str());
        rc = this->DxRemote_CopyFile(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLFILE_DELETE:
        LOG_INFO("DxRemoteMessage_Command_VPLFILE_DELETE:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_VPLFile_Delete(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_VPLFILE_TOUCH:
        LOG_INFO("DxRemoteMessage_Command_VPLFILE_TOUCH:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_VPLFile_Touch(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_GET_UPLOAD_PATH:
        LOG_INFO("DxRemoteMessage_Command_GET_UPLOAD_PATH");
        rc = this->DxRemote_Get_Upload_Path(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_GET_CCD_ROOT_PATH:
        LOG_INFO("DxRemoteMessage_Command_GET_CCD_ROOT_PATH");
        rc = this->DxRemote_Get_CCD_Root_Path(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_SET_CLEARFI_MODE:
        LOG_INFO("DxRemoteMessage_Command_SET_CLEARFI_MODE:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_Set_Clearfi_Mode(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_STOP_CONNECTED_ANDROID_DXREMOTEAGENT:
        LOG_INFO("DxRemoteMessage_Command_STOP_CONNECTED_ANDROID_DXREMOTEAGENT");
        rc = this->DxRemote_Stop_Connected_Android_RemoteAgent();
        break;

    case igware::dxshell::DxRemoteMessage_Command_LAUNCH_CONNECTED_ANDROID_CC_SERVICE:
        LOG_INFO("DxRemoteMessage_Command_LAUNCH_CONNECTED_ANDROID_CC_SERVICE");
        rc = this->DxRemote_Launch_Connected_Android_CC_Service();
        break;

    case igware::dxshell::DxRemoteMessage_Command_STOP_CONNECTED_ANDROID_CC_SERVICE:
        LOG_INFO("DxRemoteMessage_Command_STOP_CONNECTED_ANDROID_CC_SERVICE");
        rc = this->DxRemote_Stop_Connected_Android_CC_Service();
        break;
        
    case igware::dxshell::DxRemoteMessage_Command_RESTART_CONNECTED_ANDROID_DXREMOTEAGENT:
        LOG_INFO("DxRemoteMessage_Command_RESTART_CONNECTED_ANDROID_DXREMOTEAGENT");
        rc = this->DxRemote_Restart_Connected_Android_RemoteAgent();
        break;

    case igware::dxshell::DxRemoteMessage_Command_GET_CONNECTED_ANDROID_CCD_LOG:
        LOG_INFO("DxRemoteMessage_Command_GET_CONNECTED_ANDROID_CCD_LOG");
        rc = this->DxRemote_Get_Connected_Android_CCD_Log();
        break;

    case igware::dxshell::DxRemoteMessage_Command_CLEAN_CONNECTED_ANDROID_CCD_LOG:
        LOG_INFO("DxRemoteMessage_Command_CLEAN_CONNECTED_ANDROID_CCD_LOG");
        rc = this->DxRemote_Clean_Connected_Android_CCD_Log();
        break;

    case igware::dxshell::DxRemoteMessage_Command_CHECK_CONNECTED_ANDROID_NET_STATUS:
        LOG_INFO("DxRemoteMessage_Command_CHECK_CONNECTED_ANDROID_NET_STATUS");
        rc = this->DxRemote_Check_Connected_Android_Net_Status();
        break;

    case igware::dxshell::DxRemoteMessage_Command_GET_ALIAS_PATH:
        LOG_INFO("DxRemoteMessage_Command_GET_ALIAS_PATH:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_Get_Alias_Path(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_READ_LIBRARY:
        LOG_INFO("DxRemoteMessage_Command_READ_LIBRARY:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_Read_Library(cliReq, resToCli);
        break;

    case igware::dxshell::DxRemoteMessage_Command_SET_PERMISSION:
        LOG_INFO("DxRemoteMessage_Command_SET_PERMISSION:%s",
                 cliReq.argument(0).value().c_str());
        rc = this->DxRemote_Set_Permission(cliReq, resToCli);
        break;
 
    case igware::dxshell::DxRemoteMessage_Command_COMMAND_NONE:
    default:
        rc = -1;
        resToCli.set_vpl_return_code(-1);
        break;
    }

    resToCli.set_vpl_return_code(rc);

    LOG_INFO("Serialize Response for vpl_return_code: %d", resToCli.vpl_return_code());

    response = resToCli.SerializeAsString();
    LOG_DEBUG("DxRemoteOSAgent::doAction() End");

    return 0;
}

bool DxRemoteOSAgent::isHeadOfLine(const std::string &lines, size_t pos)
{
    if (pos == 0)
        return true;
    pos--;
    while (1) {
        if (pos == 0)
            return true;
        if (lines[pos] == '\r' || lines[pos] == '\n')
            return true;
        if (lines[pos] != ' ' && lines[pos] != '\t')
            return false;
        pos--;
    }

    return true;
}

// returns true iff first non-blank char starting at "pos" is '='
bool DxRemoteOSAgent::isFollowedByEqual(const std::string &lines, size_t pos)
{
    while (pos < lines.length()) {
        if (lines[pos] == '=')
            return true;
        if (lines[pos] != ' ' && lines[pos] != '\t')
            return false;
        pos++;
    }
    return false;
}

// returns the position one char beyond the last char on the line
size_t DxRemoteOSAgent::findLineEnd(const std::string &lines, size_t pos)
{
    while (pos < lines.length()) {
        if (lines[pos] == '\r' || lines[pos] == '\n')
            return pos;
        pos++;
    }
    return pos;
}

void DxRemoteOSAgent::updateConfig(std::string &config, const std::string &key, const std::string &value)
{
    bool modified = false;
    size_t keypos = 0;
    size_t nextpos = 0;
    // TODO: need to make it case-insensitive comparison
    while ( (keypos = config.find(key, nextpos)) != config.npos) {

        bool isHead = isHeadOfLine(config, keypos);
        if (!isHead) {
            nextpos = keypos + 1;
            continue;
        }

        bool isEqual = isFollowedByEqual(config, keypos + key.length());
        if (!isEqual) {
            nextpos = keypos + 1;
            continue;
        }

        std::string replacement = " = " + value;
        config.replace(keypos + key.length(), findLineEnd(config, keypos) - keypos - key.length(), replacement);
        modified = true;
        nextpos = keypos + 1;
    }

    if (!modified) {
        config += "\n" + key + " = " + value + "\n";
    }
}

std::string DxRemoteOSAgent::getSharedCCDConfID()
{
#if defined(VPL_PLAT_IS_WINRT)
    return std::string("ccd.conf");
#elif defined(IOS)
    return std::string(VPL_SHARED_CCD_CONF_ID);
#else
    return std::string("");
#endif
}

int DxRemoteOSAgent::changeSharedConfClearfiMode(std::string mode)
{
    int rv = 0;
#if defined (VPL_PLAT_IS_WINRT) || defined(IOS)
    std::string modeName = "clearfiMode";
    std::string config;
    uint32_t fileSize = 0;
    char *fileContentBuf = NULL;
    do
    {
        const char* actoolLocation = VPLSharedObject_GetActoolLocation();

        VPLSharedObject_GetData(actoolLocation, getSharedCCDConfID().c_str(), (void**)&fileContentBuf, fileSize);
        config = std::string(fileContentBuf);
        VPLSharedObject_FreeData((void*)&fileContentBuf);

        updateConfig(config, modeName, mode);
        rv = VPLSharedObject_DeleteObject(actoolLocation, getSharedCCDConfID().c_str());
        if (rv != VPL_OK) {
            break;
        }
        //rc = VPLSharedObject_AddString(getSharedCCDConfID().c_str(), config.c_str());
        rv = VPLSharedObject_AddData(actoolLocation, getSharedCCDConfID().c_str(), (void*)config.data(), config.size());
        if (rv != VPL_OK) {
            break;
        }
    }while(false);
#endif
    return rv;
}


int DxRemoteOSAgent::DxRemote_Launch_Process(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    std::string launchFile = req.argument(0).value();
    transform(launchFile.begin(), launchFile.end(), launchFile.begin(), tolower);
    if (launchFile.compare("ccd.exe") == 0) {
        const char *titleId = NULL;
        if (req.argument().size() > 1) {
            if (req.argument(1).value().length() > 0) {
                titleId = req.argument(1).value().c_str();
            }
        }
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
        rv = startccd(titleId);
#else
        // use port number as instance id to launch multiple ccd under same user env
        int testInstanceNum = (int)VPLNet_port_ntoh(VPLSocket_GetPort(clienttcpsocket));
        rv = startccd(testInstanceNum, titleId);
#endif
    }
    else {
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#else
        std::stringstream ssCmd;
        ssCmd << req.argument(0).value() << L" " << req.argument(1).value();
        rv = startprocess(ssCmd.str());
#endif
    }

    return rv;
}

int DxRemoteOSAgent::DxRemote_Kill_Process(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    std::string launchFile = req.argument(0).value();
    std::string launchFileLower = std::string(launchFile);
    transform(launchFileLower.begin(), launchFileLower.end(), launchFileLower.begin(), tolower);
    if (launchFileLower.compare("ccd.exe") == 0) {
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        int testInstanceNum = (int)VPLNet_port_ntoh(VPLSocket_GetPort(clienttcpsocket));
        rv = stopccd(testInstanceNum);
#else
        rv = stopccd();
#endif
    }
    else {
        #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
        #elif defined(WIN32)
        rv = _shutdownprocess(launchFile);
        _shutdownprocess(launchFileLower);
        #else
        #endif
    }

    return rv;
}

int DxRemoteOSAgent::DxRemote_Get_Connected_Android_IP(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    std::string strIpAddr;
    rv = get_connected_android_device_ip(strIpAddr);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = res.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENT_IP);
    rv == 0 ? myArg->set_value(strIpAddr) : myArg->set_value("");
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Launch_Connected_Android_RemoteAgent()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    rv = launch_dx_remote_android_agent();
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_CleanCC()
{
    int rv = 0;
    rv = clean_cc();
    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLFS_Opendir(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    VPLFS_dir_t dir;
    std::pair<std::map<std::string, VPLFS_dir_t>::iterator, bool> ret;
    if (fsdir_map.find(req.dir_folder().dir_path()) == fsdir_map.end()) {
        rv = VPLFS_Opendir(req.dir_folder().dir_path().c_str(), &dir);
        if (rv == VPL_OK) {
            ret = fsdir_map.insert(std::pair<std::string, VPLFS_dir_t>(req.dir_folder().dir_path(), dir));
        }
    }
    else {
        rv = VPL_OK;
    }

    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLFS_Readdir(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    VPLFS_dirent_t folderDirent;
    if (fsdir_map.find(req.dir_folder().dir_path()) != fsdir_map.end()) {
        rv = VPLFS_Readdir(&(fsdir_map.find(req.dir_folder().dir_path())->second), &folderDirent);
        if (rv == VPL_OK) {
            igware::dxshell::DxRemoteMessage_DxRemote_VPLFS_dirent_t *mentry = res.mutable_folderdirent();
            mentry->set_type((igware::dxshell::DxRemoteMessage_DxRemote_VPLFS_file_type_t)(int)folderDirent.type);
            mentry->set_filename(folderDirent.filename);
        }
    }
    else {
        rv = VPL_ERR_BADF;
    }

    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLFS_Closedir(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    if (fsdir_map.find(req.dir_folder().dir_path()) != fsdir_map.end()) {
        rv = VPLFS_Closedir(&(fsdir_map[req.dir_folder().dir_path()]));
        if (rv == VPL_OK) {
            this->fsdir_map.erase(req.dir_folder().dir_path());
        }
    }
    else {
        rv = VPL_ERR_BADF;
    }

    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLFS_Stat(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    VPLFS_stat_t stat;

    do
    {
        if (req.argument_size() < 1) {
            rv = VPL_ERR_INVALID;
            break;
        }
        rv = VPLFS_Stat(req.argument(0).value().c_str(), &stat);
        if (rv == VPL_OK) {
            igware::dxshell::DxRemoteMessage_DxRemote_VPLFS_stat_t *mstat = res.mutable_file_stat();
            mstat->set_size(stat.size);
            mstat->set_type((igware::dxshell::DxRemoteMessage_DxRemote_VPLFS_file_type_t)(int)stat.type);
            mstat->set_atime(stat.atime);
            mstat->set_mtime(stat.mtime);
            mstat->set_ctime(stat.ctime);
            mstat->set_ishidden(stat.isHidden);
            mstat->set_issymlink(stat.isSymLink);
        }
    } while (false);

    return rv;
}

int DxRemoteOSAgent::DxRemote_Util_rm_dash_rf(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_OK;
    do
    {
        if (req.argument_size() < 1) {
            rv = VPL_ERR_INVALID;
            break;
        }

        std::string originalPath = req.argument(0).value();
        rv = Util_rm_dash_rf(originalPath);
    } while (false);

    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLDir_Create(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_OK;
    do
    {
        if (req.argument_size() < 1) {
            rv = VPL_ERR_INVALID;
            break;
        }

        std::string dirPath = req.argument(0).value();
        rv = VPLDir_Create(dirPath.c_str(), req.create_dir_mode());
    } while (false);

    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLFile_Rename(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_OK;
    do
    {
        std::string srcPath = req.rename_source(), dstPath = req.rename_destination();
        rv = VPLFile_Rename(srcPath.c_str(), dstPath.c_str());
    } while (false);

    return rv;
}

int DxRemoteOSAgent::DxRemote_CopyFile(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_OK;
    VPLFile_handle_t fHSrc = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t fHDst = VPLFILE_INVALID_HANDLE;
    char *tempChunkBuf = NULL;
    u32 tempChunkBufSize = 32 * 1024;
    do
    {
        if (req.argument_size() < 2) {
            rv = VPL_ERR_INVALID;
            break;
        }

        std::string srcPath = req.argument(0).value(), dstPath = req.argument(1).value();
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#ifdef VPL_PLAT_IS_WINRT
        std::string dstFolderPath = dstPath.substr(0, dstPath.find_last_of("\\"));
#else
        std::string dstFolderPath = dstPath.substr(0, dstPath.find_last_of("/"));
#endif
        if ( (rv = VPLDir_Create(dstFolderPath.c_str(), 0755)) != VPL_OK) {
            if (rv != VPL_ERR_EXIST) {
                break;
            }

            rv = VPL_OK;
        }
#else
        if ( (rv = Util_CreatePath(dstPath.c_str(), VPL_FALSE)) != VPL_OK) {
            break;
        }
#endif

        const int flagDst = VPLFILE_OPENFLAG_CREATE |
                            VPLFILE_OPENFLAG_WRITEONLY |
                            VPLFILE_OPENFLAG_TRUNCATE;

        fHSrc = VPLFile_Open(srcPath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(fHSrc)) {
            rv = VPL_ERR_INVALID;
            break;
        }

        fHDst = VPLFile_Open(dstPath.c_str(), flagDst, 0666);
        if (!VPLFile_IsValidHandle(fHDst)) {
            rv = VPL_ERR_INVALID;
            break;
        }

        tempChunkBuf = new (std::nothrow) char[tempChunkBufSize];
        if (!tempChunkBuf) {
            rv = VPL_ERR_NOMEM;
            break;
        }

        ssize_t bytesRead = 0;
        while ( (bytesRead = VPLFile_Read(fHSrc, tempChunkBuf, tempChunkBufSize)) > 0)
        {
            ssize_t wrCnt = 0;
            if ( (wrCnt = VPLFile_Write(fHDst, tempChunkBuf, bytesRead)) != bytesRead) {
                rv = VPL_ERR_IO;
                break;
            }
        }

    } while (false);

    if (tempChunkBuf) {
        delete[] tempChunkBuf;
        tempChunkBuf = NULL;
    }

    if (VPLFile_IsValidHandle(fHDst)) {
        VPLFile_Close(fHDst);
        fHDst = VPLFILE_INVALID_HANDLE;
    }

    if (VPLFile_IsValidHandle(fHSrc)) {
        VPLFile_Close(fHSrc);
        fHSrc = VPLFILE_INVALID_HANDLE;
    }

    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLFile_Delete(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_OK;
    do
    {
        if (req.argument_size() < 1) {
            rv = VPL_ERR_INVALID;
            break;
        }

        std::string dirPath = req.argument(0).value();
        rv = VPLFile_Delete(dirPath.c_str());
    } while (false);

    return rv;
}

int DxRemoteOSAgent::DxRemote_VPLFile_Touch(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    LOG_DEBUG("DxRemoteOSAgent::DxRemote_VPLFile_Touch() Start");
    int rv = VPL_OK;
    do
    {
        if (req.argument_size() < 1) {
            rv = VPL_ERR_INVALID;
            break;
        }

        std::string dirPath = req.argument(0).value();
        rv = VPLFile_SetTime(dirPath.c_str(), VPLTime_GetTime());

        LOG_DEBUG("VPLFile_Touch result: %d", rv);
    } while (false);

    return rv;
}

int DxRemoteOSAgent::DxRemote_Get_Upload_Path(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_OK;
    std::string dirPath;
    do
    {
        rv = get_user_folder(dirPath);
        if (rv != VPL_OK) {
            break;
        }

        igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = res.add_argument();
        myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTDIRNAME);
        myArg->set_value(dirPath);
    } while (false);

    return rv;
}

int DxRemoteOSAgent::DxRemote_Get_CCD_Root_Path(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_OK;
    std::string ccdRootPath;
    do
    {
        rv = get_cc_folder(ccdRootPath);
        if (rv != VPL_OK) {
            break;
        }

        igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = res.add_argument();
        myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTDIRNAME);
        myArg->set_value(ccdRootPath);
    } while (false);

    return rv;
} 

int DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
    LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() Start");
    do
    {
        if (req.argument_size() < 1) {
            rv = VPL_ERR_INVALID;
            LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() VPL_ERR_INVALID");
            break;
        }

        std::string modeName = "clearfiMode";
        std::string modeType = req.argument(0).value();
        std::string config;
        std::string ccdConfPath;

#ifdef VPL_PLAT_IS_WINRT
        LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() CreateEventEx Start");
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder ^> ^iSyncAgentFolder = KnownFolders::MusicLibrary->GetFolderAsync(_VPL__SharedAppFolder);
        iSyncAgentFolder->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFolder^>(
            [this, &modeName, &modeType, &completedEvent] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder^>^ op, Windows::Foundation::AsyncStatus status) {
                try {
                    Windows::Storage::StorageFolder^ agentfolder = op->GetResults();
                    Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder ^> ^iSyncConfFolder = agentfolder->GetFolderAsync(L"conf");
                    iSyncConfFolder->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFolder^>(
                        [this, &modeName, &modeType, &completedEvent] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder^>^ op, Windows::Foundation::AsyncStatus status) {
                            try {
                                Windows::Storage::StorageFolder^ conffolder = op->GetResults();
                                Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile ^> ^iSyncConfFile = conffolder->GetFileAsync("ccd.conf");
                                iSyncConfFile->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFile^>(
                                    [this, &modeName, &modeType, conffolder, &completedEvent] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile^>^ op, Windows::Foundation::AsyncStatus status) {
                                        try {
                                            Windows::Storage::StorageFile ^cfOld = op->GetResults();
                                            Windows::Foundation::IAsyncOperation<Platform::String ^> ^readOld = Windows::Storage::FileIO::ReadTextAsync(cfOld);
                                            readOld->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Platform::String ^>(
                                                [this, &modeName, &modeType, conffolder, &completedEvent] (Windows::Foundation::IAsyncOperation<Platform::String ^>^ op, Windows::Foundation::AsyncStatus status) {
                                                    try {
                                                        Platform::String ^psOldConf = op->GetResults();
                                                        char *utf8 = NULL;
                                                        _VPL__wstring_to_utf8_alloc(psOldConf->Data(), &utf8);
                                                        std::string oldConfData(utf8);
                                                        if (utf8 != NULL) {
                                                            free(utf8);
                                                            utf8 = NULL;
                                                        }
                                                        updateConfig(oldConfData, modeName, modeType);
                                                        Platform::Array<unsigned char> ^platWriteData = ref new Platform::Array<unsigned char>((unsigned char *)oldConfData.data(), oldConfData.size());
                                                        Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile ^> ^confFileWrite = conffolder->CreateFileAsync("ccd.conf", Windows::Storage::CreationCollisionOption::ReplaceExisting);
                                                        confFileWrite->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFile^>(
                                                            [platWriteData, &completedEvent] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile^>^ op, Windows::Foundation::AsyncStatus status) {
                                                                try {
                                                                    Windows::Storage::StorageFile ^iSyncConfFileWrite = op->GetResults();
                                                                    Windows::Foundation::IAsyncAction ^ iSyncWrite = FileIO::WriteBytesAsync(iSyncConfFileWrite, platWriteData);
                                                                    iSyncWrite->Completed = ref new Windows::Foundation::AsyncActionCompletedHandler(
                                                                        [&completedEvent] (Windows::Foundation::IAsyncAction ^op, Windows::Foundation::AsyncStatus status) {
                                                                            try {
                                                                                op->GetResults();
                                                                                SetEvent(completedEvent);
                                                                            }
                                                                            catch (Platform::Exception^ e) {
                                                                                Platform::String^ psWriteException = e->ToString();
                                                                                std::wstring wstrWriteException = (psWriteException->Begin(), psWriteException->End());

                                                                                char *utf8Write = NULL;
                                                                                _VPL__wstring_to_utf8_alloc(wstrWriteException.c_str(), &utf8Write);
                                                                                std::string strWrite(utf8Write);

                                                                                if (utf8Write != NULL) {
                                                                                    free(utf8Write);
                                                                                    utf8Write = NULL;
                                                                                }

                                                                                LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() CreateFileAsync Exception: %s", strWrite.c_str());
                                                                                SetEvent(completedEvent);
                                                                            }
                                                                        }
                                                                    );
                                                                }
                                                                catch (Platform::Exception^ e) {
                                                                    Platform::String^ psCreateException = e->ToString();
                                                                    std::wstring wstrCreateException = (psCreateException->Begin(), psCreateException->End());

                                                                    char *utf8Create = NULL;
                                                                    _VPL__wstring_to_utf8_alloc(wstrCreateException.c_str(), &utf8Create);
                                                                    std::string strCreate(utf8Create);

                                                                    if (utf8Create != NULL) {
                                                                        free(utf8Create);
                                                                        utf8Create = NULL;
                                                                    }

                                                                    LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() ReadTextAsync Exception: %s", strCreate.c_str());
                                                                    SetEvent(completedEvent);
                                                                }
                                                            }
                                                        );
                                                    }
                                                    catch (Platform::Exception^ e) {
                                                        Platform::String^ psReadException = e->ToString();
                                                        std::wstring wstrReadException = (psReadException->Begin(), psReadException->End());

                                                        char *utf8Read = NULL;
                                                        _VPL__wstring_to_utf8_alloc(wstrReadException.c_str(), &utf8Read);
                                                        std::string strRead(utf8Read);

                                                        if (utf8Read != NULL) {
                                                            free(utf8Read);
                                                            utf8Read = NULL;
                                                        }

                                                        LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() ReadTextAsync Exception: %s", strRead.c_str());
                                                        SetEvent(completedEvent);
                                                    }
                                                }
                                            );
                                        }
                                        catch (Platform::Exception^ e) {
                                            Platform::String^ psGetFileException = e->ToString();
                                            std::wstring wstrGetFileException = (psGetFileException->Begin(), psGetFileException->End());

                                            char *utf8GetFileAgent= NULL;
                                            _VPL__wstring_to_utf8_alloc(wstrGetFileException.c_str(), &utf8GetFileAgent);
                                            std::string strGetFileAgent(utf8GetFileAgent);

                                            if (utf8GetFileAgent != NULL) {
                                                free(utf8GetFileAgent);
                                                utf8GetFileAgent = NULL;
                                            }

                                            LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() Get File Music/SyncAgent/conf/ccd.conf Exception: %s", strGetFileAgent.c_str());
                                            SetEvent(completedEvent);
                                        }
                                    }
                                );
                            }
                            catch (Platform::Exception^ e) {
                                Platform::String^ psConfException = e->ToString();
                                std::wstring wstrConfException = (psConfException->Begin(), psConfException->End());

                                char *utf8ConfAgent= NULL;
                                _VPL__wstring_to_utf8_alloc(wstrConfException.c_str(), &utf8ConfAgent);
                                std::string strConfAgent(utf8ConfAgent);

                                if (utf8ConfAgent != NULL) {
                                    free(utf8ConfAgent);
                                    utf8ConfAgent = NULL;
                                }

                                LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() Get Folder Music/SyncAgent/conf Exception: %s", strConfAgent.c_str());
                                SetEvent(completedEvent);
                            }
                        }
                    );
                }
                catch (Platform::Exception^ e) {
                    Platform::String^ psSyncAgentException = e->ToString();
                    std::wstring wstrSyncAgentException = (psSyncAgentException->Begin(), psSyncAgentException->End());

                    char *utf8SyncAgent= NULL;
                    _VPL__wstring_to_utf8_alloc(wstrSyncAgentException.c_str(), &utf8SyncAgent);
                    std::string strSyncAgent(utf8SyncAgent);

                    if (utf8SyncAgent != NULL) {
                        free(utf8SyncAgent);
                        utf8SyncAgent = NULL;
                    }

                    LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() Get Folder Music/SyncAgent Exception: %s", strSyncAgent.c_str());
                    SetEvent(completedEvent);
                }
            }
        );

        LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() WaitForSingleObjectEx Start");
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() WaitForSingleObjectEx End");
#else
#if defined(IOS)
        get_cc_folder(ccdConfPath);
#elif defined(WIN32)

        std::wstring wpath;
        char *utf8 = NULL;
        wchar_t *localAppDataWPath = NULL;

        rv = _VPLFS__GetLocalAppDataWPath(&localAppDataWPath);
        if (rv != VPL_OK) {
            break;
        }
        wpath.assign(localAppDataWPath);
        wpath.append(L"\\iGware\\SyncAgent");
        std::wstring wstrCcdName = get_dx_ccd_nameW();
        if (wstrCcdName.size() > 0) {
            wpath.append(L"_");
            wpath.append(wstrCcdName);
        }
        rv = _VPL__wstring_to_utf8_alloc(wpath.c_str(), &utf8);
        if (rv != 0) {
            break;
        }

        ccdConfPath.assign(utf8);
        if (utf8 != NULL) {
            free(utf8);
            utf8 = NULL;
        }
#else
#endif
        ccdConfPath.append(DIR_DELIM "conf" DIR_DELIM "ccd.conf");
        rv = VPLFile_CheckAccess(ccdConfPath.c_str(), VPLFILE_CHECKACCESS_EXISTS);
        if (rv != VPL_OK) {
            // ccd.conf does not exist -> return empty config
            config.clear();
            break;
        }

        //read conf
        char *fileContentBuf = NULL;
        int fileSize = -1;
        fileSize = Util_ReadFile(ccdConfPath.c_str(), (void**)&fileContentBuf, 0);
        if (fileSize < 0) {
            rv = fileSize;
            break;
        }
        config.assign(fileContentBuf, fileSize);
        updateConfig(config, modeName, modeType);

        rv = Util_WriteFile(ccdConfPath.c_str(), config.data(), config.size());
        if (rv != VPL_OK) {
            break;
        }
#endif

#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
        rv = changeSharedConfClearfiMode(modeType);
        if (rv != 0) {
            break;
        }
#endif

    } while (false);

    LOG_DEBUG("DxRemoteOSAgent::DxRemote_Set_Clearfi_Mode() End");

    return rv;
}

int DxRemoteOSAgent::DxRemote_Push_Local_Conf_To_Shared_Object(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    std::string config;
    std::string ccdConfPath;

    do
    {
        rv = get_cc_folder(ccdConfPath);
        if (rv != VPL_OK) {
            // ccd.conf does not exist -> return empty config
            ccdConfPath.clear();
            break;
        }
#if defined(VPL_PLAT_IS_WINRT)
        ccdConfPath.append(DIR_DELIM "conf" DIR_DELIM "ccd.conf");
#elif defined(IOS)
        ccdConfPath.append(DIR_DELIM "conf" DIR_DELIM "ccd.conf");
#else
#endif
        rv = VPLFile_CheckAccess(ccdConfPath.c_str(), VPLFILE_CHECKACCESS_EXISTS);
        if (rv != VPL_OK) {
            // ccd.conf does not exist -> return empty config
            config.clear();
            break;
        }

        //read conf
        char *fileContentBuf = NULL;
        int fileSize = -1;
        fileSize = Util_ReadFile(ccdConfPath.c_str(), (void**)&fileContentBuf, 0);
        if (fileSize < 0) {
            rv = fileSize;
            break;
        }
        config.assign(fileContentBuf, fileSize);

        const char* actoolLocation = VPLSharedObject_GetActoolLocation();

        rv = VPLSharedObject_AddData(actoolLocation, getSharedCCDConfID().c_str(), config.c_str(), config.size());
        if (rv != VPL_OK) {
            break;
        }

    }while(false);
#endif

    return rv;
}

int DxRemoteOSAgent::DxRemote_Pull_Shared_Conf_To_Local_Object(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    std::string config;
    std::string ccdConfPath;

    do
    {
        get_cc_folder(ccdConfPath);
        ccdConfPath.append(DIR_DELIM "conf" DIR_DELIM "ccd.conf");
        const char* actoolLocation = VPLSharedObject_GetActoolLocation();

        unsigned int confLength = 0;
        void* confBytes = NULL;
        VPLSharedObject_GetData(actoolLocation, VPL_SHARED_CCD_CONF_ID, &confBytes, confLength);
        rv = Util_WriteFile(ccdConfPath.c_str(), confBytes, confLength);
        if (rv < 0) {
            LOG_ERROR("Failed to write to \"%s\"", ccdConfPath.c_str());
            return rv;
        }

    }while(false);
#endif

    return rv;
}

int DxRemoteOSAgent::DxRemote_Stop_Connected_Android_RemoteAgent()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    rv = stop_dx_remote_android_agent();
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Launch_Connected_Android_CC_Service()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    rv = launch_dx_remote_android_cc_service();
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Stop_Connected_Android_CC_Service()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    rv = stop_dx_remote_android_cc_service();
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Restart_Connected_Android_RemoteAgent()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    rv = restart_dx_remote_android_agent();
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Get_Connected_Android_CCD_Log()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    rv = get_ccd_log_from_android();
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Clean_Connected_Android_CCD_Log()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    rv = clean_ccd_log_on_android();
    #else
    #endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Check_Connected_Android_Net_Status()
{
    int rv = 0;
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    #elif defined(WIN32)
    char dst[256];
    VPLNet_addr_t localAddr = VPLNet_GetLocalAddr();
    if (localAddr == VPLNET_ADDR_INVALID) {
        LOG_ERROR("DxRemoteOSAgent::DxRemote_Check_Connected_Android_Net_Status() Fail to GetLocalAddr");
        return -1;
    }

    if (VPLNet_Ntop(&localAddr, dst, 256) != dst) {
        LOG_ERROR("DxRemoteOSAgent::DxRemote_Check_Connected_Android_Net_Status() Fail to VPLNet_Ntop");
        return -1; 
    }
    else {
        LOG_ALWAYS("DxRemoteOSAgent::DxRemote_Check_Connected_Android_Net_Status() VPLNet_GetLocalAddr(): %s", dst);
    }
    rv = check_android_net_status(dst);
    #else
    #endif   
    return rv;
}

int DxRemoteOSAgent::DxRemote_Get_Alias_Path(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = VPL_ERR_FAIL;

#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#elif defined(WIN32)
    char *tmp_path = NULL;
    std::string alias;
    std::string path;

    if (req.argument_size() < 1) {
        rv = VPL_ERR_INVALID;
        goto exit;
    }

    // extract alias from the req
    alias = req.argument(0).value();

    // get local app / user profile folder path
    if (alias == "LOCALAPPDATA") {
        rv = _VPLFS__GetLocalAppDataPath(&tmp_path);
        if (rv != VPL_OK || tmp_path == NULL) {
            LOG_ERROR("Unable to get local app folder path, rv = %d", rv);
            rv = VPL_ERR_FAIL;
            goto exit;
        } else {
            path = tmp_path;
            free(tmp_path);
            rv = VPL_OK;
        }
    } else if (alias == "USERPROFILE") {
        rv = _VPLFS__GetProfilePath(&tmp_path);
        if (rv != VPL_OK || tmp_path == NULL) {
            LOG_ERROR("Unable to get user profile folder path, rv = %d", rv);
            rv = VPL_ERR_FAIL;
            goto exit;
        } else {
            path = tmp_path;
            free(tmp_path);
            rv = VPL_OK;
        }
    }

    if (!path.empty()) {
        igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = res.add_argument();
        myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTDIRNAME);
        myArg->set_value(path);
    }
exit:
#else
#endif

    return rv;
}

int DxRemoteOSAgent::DxRemote_Read_Library(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#elif defined(WIN32)
    char *librariesPath = NULL;
    std::string lib_type;

    // Call CoInitialized to utilize the VPL_GetLibrary function
    {
        HRESULT hres;

        // Initialize COM
        hres =  CoInitialize(NULL);
        if (FAILED(hres)) {
            LOG_ERROR("Unable to initialize COM");
            return VPL_ERR_FAIL;
        }
    }

    if (req.argument_size() < 1) {
        return VPL_ERR_INVALID;
    }
    lib_type = req.argument(0).value();

    if (_VPLFS__GetLibrariesPath(&librariesPath) == VPL_OK) {
        VPLFS_dir_t dir;
        VPLFS_dirent_t dirent;

        if (VPLFS_Opendir(librariesPath, &dir) == VPL_OK) {
            while (VPLFS_Readdir(&dir, &dirent) == VPL_OK) {
                char *p = strstr(dirent.filename, ".library-ms");
                if (p != NULL && p[strlen(".library-ms")] == '\0') {
                    // found library description file
                    std::string libDescFilePath;
                    libDescFilePath.assign(librariesPath).append("/").append(dirent.filename);

                    // grab both localized and non-localized name of the library folders
                    _VPLFS__LibInfo libinfo;
                    rv = _VPLFS__GetLibraryFolders(libDescFilePath.c_str(), &libinfo);
                    if (libinfo.folder_type != lib_type) {
                        // only report requested type
                        continue;
                    }
                    // subfolders
                    std::map<std::string, _VPLFS__LibFolderInfo>::const_iterator it;
                    for (it = libinfo.m.begin(); it != libinfo.m.end(); it++) {
                        igware::dxshell::DxRemoteMessage_DxRemote_LibraryInfo *info = res.add_lib_info();
                        info->set_type(libinfo.folder_type);
                        info->set_real_path(it->second.path);
                        info->set_virt_path(("Libraries/" + libinfo.n_name + "/" + it->second.n_name));
                    }
                }
            }
            VPLFS_Closedir(&dir);
        }
        if (librariesPath) {
            free(librariesPath);
            librariesPath = NULL;
        }
    }
#else
#endif
    return rv;
}

int DxRemoteOSAgent::DxRemote_Set_Permission(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res)
{
    int rv = 0;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    if(req.argument_size() < 2 ||
       req.argument(0).name() != igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME){

        rv = VPL_ERR_INVALID;
    } else {
        rv = set_permission(req.argument(0).value(), req.argument(1).value());
    }
#endif

    return rv;
}
