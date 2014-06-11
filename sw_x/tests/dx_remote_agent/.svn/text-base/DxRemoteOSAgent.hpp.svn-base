#ifndef DX_REMOTE_OS_AGENT_H_
#define DX_REMOTE_OS_AGENT_H_

#include "IDxRemoteAgent.h"
#include "dx_remote_agent.pb.h"
#include <vpl_fs.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <map>

class DxRemoteOSAgent : public IDxRemoteAgent
{
public:
    DxRemoteOSAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    ~DxRemoteOSAgent();
    int doAction();
protected:
    int DxRemote_Launch_Process(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Kill_Process(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Get_Connected_Android_IP(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLFS_Opendir(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLFS_Readdir(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLFS_Closedir(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLFS_Stat(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Util_rm_dash_rf(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLDir_Create(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLFile_Rename(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_CopyFile(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLFile_Delete(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_VPLFile_Touch(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Get_Upload_Path(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Get_CCD_Root_Path(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Set_Clearfi_Mode(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Push_Local_Conf_To_Shared_Object(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Pull_Shared_Conf_To_Local_Object(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_CleanCC();
    int DxRemote_Launch_Connected_Android_RemoteAgent();
    int DxRemote_Stop_Connected_Android_RemoteAgent();
    int DxRemote_Launch_Connected_Android_CC_Service();
    int DxRemote_Stop_Connected_Android_CC_Service();
    int DxRemote_Restart_Connected_Android_RemoteAgent();
    int DxRemote_Get_Connected_Android_CCD_Log();
    int DxRemote_Clean_Connected_Android_CCD_Log();
    int DxRemote_Check_Connected_Android_Net_Status();
    int DxRemote_Get_Alias_Path(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Read_Library(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);
    int DxRemote_Set_Permission(igware::dxshell::DxRemoteMessage &req, igware::dxshell::DxRemoteMessage &res);

    bool isHeadOfLine(const std::string &lines, size_t pos);
    bool isFollowedByEqual(const std::string &lines, size_t pos);
    size_t findLineEnd(const std::string &lines, size_t pos);
    void updateConfig(std::string &config, const std::string &key, const std::string &value);
    int changeSharedConfClearfiMode(std::string mode);
    std::string getSharedCCDConfID();

    static std::map<std::string, VPLFS_dir_t> fsdir_map;
};

#endif
