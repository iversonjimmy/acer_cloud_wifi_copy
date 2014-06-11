//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// Command-line "menu" for testing CCD.

#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv

#define CCDSRV_MON_TEST 1

#include <vpl_plat.h>
#include <vplex_plat.h>
#include <ccdi.hpp>
#include <log.h>
#include <vpl_th.h>
#include <vpl_time.h>
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>
#include <stack>
#include <set>

#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <ccdi_client_tcp.hpp>

#include "autotest.hpp"
#include "ccdconfig.hpp"
#include "ccd_utils.hpp"
#include "clouddochttp.hpp"
#include "picstreamhttp.hpp"
#include "common_utils.hpp"
#include "cr_test.hpp"
#include "dataset.hpp"
#include "dx_common.h"
#include "EventQueue.hpp"
#include "fs_test.hpp"
#include "ts_test.hpp"
#include "minidms_test.hpp"
#include "gvm_file_utils.hpp"
#include "HttpAgent.hpp"
#include "mca_diag.hpp"
#include "photo_share_test.hpp"
#include "scopeguard.hpp"
#include "setup_stream_test.hpp"
#include "target_cmds.hpp"
#include "TargetDevice.hpp"
#include "TimeStreamDownload.hpp"
#include "vcs_test.hpp"
#include "vssi_stress_test.hpp"
#include "syncbox_test.hpp"

#if CCDSRV_MON_TEST
#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
#include <Sddl.h>
#include "CCDMonSrvClient.h"
#endif // VPL_PLAT_IS_WIN_DESKTOP_MODE
#endif // CCDSRV_MON_TEST

#include "RemoteAgent.hpp"
#include "dx_remote_agent.pb.h"

#define DEFAULT_REMOTE_AGENT_PORT_NUMBER 24000

#if defined(WIN32)
# define EXE_PREFIX ""
# define EXE_NAME  "dxshell.exe"
#else
# define EXE_PREFIX "./"
# define EXE_NAME  "dxshell"
#endif

// To add new sub commands,
// 1) Add a map entry of the command string to the sub command function in init_cmds. The function pointer to the 
//    sub command function has the type subcmd_fn 
// 2) Implement the sub command function.
// 3) In the sub command function, it must implement print help (Reference the usage of checkHelp below)
//    [Note: Help printouts are printed with printf() while other logs are implemented with log lib functions]

static int print_help();

static const char* NOTES_STR = "Notes";
int notes_dispatch(int argc, const char* argv[]);
int cmd_notes_status(int argc, const char* argv[]);
int cmd_notes_enable(int argc, const char* argv[]);
int cmd_notes_disable(int argc, const char* argv[]);

static const char* POWER_STR =  "Power";
int foreground_mode(int argc, const char* argv[]);
int background_tasks(int argc, const char* argv[]);
int set_background_mode_interval(int argc, const char* argv[]);
int set_enable_network(int argc, const char* argv[]);
int set_allow_bg_data(int argc, const char* argv[]);
int set_allow_auto_sync(int argc, const char* argv[]);
int set_allow_mobile_network(int argc, const char* argv[]);
int set_clouddoc_sync(int argc, const char* argv[]);
int set_only_mobile_network_avail(int argc, const char* argv[]);
int set_stream_power_mode(int argc, const char* argv[]);
int sync_once(int argc, const char* argv[]);
int power_get_status(int argc, const char* argv[]);

static const char* CLOUDMEDIA_STR = "CloudMedia";
static int cloudmedia_add_music(int argc, const char* argv[]);
static int cloudmedia_add_photo(int argc, const char* argv[]);
static int cloudmedia_add_photo_to_album(int argc, const char* argv[]);
#if defined(CLOUDNODE)
static int cloudnode_cloudmedia_add_music(int argc, const char* argv[]);
static int cloudnode_cloudmedia_add_photo(int argc, const char* argv[]);
#endif // CLOUDNODE
static int cloudmedia_delete_object(int argc, const char* argv[]);
static int cloudmedia_delete_collection(int argc, const char* argv[]);

static const char* USERLOGIN_STR = "UserLogin";
static int userlogin_set_eula(int argc, const char* argv[]);

static int userlogout(int argc, const char* argv[]);
static int link_device(int argc, const char* argv[]);
static int unlink_device(int argc, const char* argv[]);
static int remote_sw_update(int argc, const char* argv[]);
static int list_datasets(int argc, const char* argv[]);
static int report_different_network(int argc, const char* argv[]);

static int start_remote_agent(int argc, const char* argv[]);
static int stop_remote_agent(int argc, const char* argv[]);
static int restart_remote_agent_app(int argc, const char* argv[]);
static int deploy_remote_agent(int argc, const char* argv[]);
static int add_device(int argc, const char* argv[]);
static int del_device(int argc, const char* argv[]);
static int collect_cc_log(int argc, const char* argv[]);
static int clean_cc_log(int argc, const char* argv[]);

#define GET_SYSTEM_STATE_CMD  "GetSysState"
static int get_sys_state(int argc, const char* argv[]);

#define GET_SYNC_STATE_CMD  "GetSyncState"
static int get_sync_state(int argc, const char* argv[]);

#if 0
static int check_picstream(int argc, char* argv[]);
static int upload_logs(int argc, char* argv[]);
#endif

static std::map<std::string, subcmd_fn> dx_cmds;
static std::map<std::string, subcmd_fn> g_cloudmedia_cmds;
static std::map<std::string, subcmd_fn> g_mca_cmds;
static std::map<std::string, subcmd_fn> g_notes_cmds;
static std::map<std::string, subcmd_fn> g_picstream_cmds;
static std::map<std::string, subcmd_fn> g_photo_share_cmds;
static std::map<std::string, subcmd_fn> g_power_cmds;
static std::map<std::string, subcmd_fn> g_userlogin_cmds;
static std::map<std::string, subcmd_fn> g_vcs_cmds;
static std::map<std::string, subcmd_fn> g_vcspic_cmds;
static std::map<std::string, std::set<std::string> > dx_not_remote_cmds;

std::string dxtool_root;         // Location where dxtool is installed

typedef struct {
    // TODO: Fields need more descriptive names
    // TODO: Types should be those useful for the rest of this program,
    //       like VPLNet_Port_t, VPLNet_Addr_t, OS enum.
    std::string machine_name;
    std::string machine_os;
    std::string machine_username;
    std::string machine_ip;
    std::string machine_port;
    std::string controller_ip;
    std::string controller_port;
} remote_agent_config;

#define TAG_DXRC_HEAD           "<Config=dxshell_rc>"
#define TAG_DXRC_TAIL           "</Config=dxshell_rc>"
#define TAG_DEVICEINFO_HEAD     "<Config=device_info>"
#define TAG_DEVICEINFO_TAIL     "</Config=device_info>"

#define REMOTE_AGENT_CONFIG_FILE "dxshellrc.cfg"

static void init_cmds()
{
    dx_cmds["SetDomain"]        = set_domain;
    dx_cmds["SetSyncUploadMode"] = set_sync_mode_upload;
    dx_cmds["SetSyncDownloadMode"] = set_sync_mode_download;
    dx_cmds["StartCCD"]         = start_ccd;
#if CCDSRV_MON_TEST
#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
    dx_cmds["StartCCDBySrv"]    = start_ccd_by_srv;
    dx_cmds["StopCCDBySrv"]     = stop_ccd_by_srv;
#endif // VPL_PLAT_IS_WIN_DESKTOP_MODE
#endif // CCDSRV_MON_TEST
    dx_cmds["StopCCD"]          = stop_ccd_hard;
    dx_cmds["StopCCDSoft"]      = stop_ccd_soft;
    dx_cmds["StartCloudPC"]     = start_cloudpc;
    dx_cmds["StopCloudPC"]      = stop_cloudpc;
    dx_cmds["UpdatePSN"]        = update_psn;
    dx_cmds["StartClient"]      = start_client;
    dx_cmds["StopClient"]       = stop_client;
    dx_cmds["CheckMetadata"]    = check_metadata;
    dx_cmds["CheckMetadata2"]    = check_metadata2;
    dx_cmds["CountMetadataFiles"] = mca_count_metadata_files_cmd;  // Legacy (also under Mca)
    dx_cmds["SetupCheckMetadata"] = setup_check_metadata;
    dx_cmds["ListDevices"]      = list_devices;
    dx_cmds["ListUserStorage"]  = list_user_storage;
    dx_cmds["McaListMetadata"]  = mca_list_metadata;
    dx_cmds["ClearMetadata"]    = msa_delete_catalog;
    dx_cmds["CheckStreaming"]   = check_streaming;
    dx_cmds["CheckStreaming2"]  = check_streaming2;
    dx_cmds["DumpEvents"]       = dump_events;
    dx_cmds["DownloadUpdates"]  = download_updates;
    dx_cmds["SetupStreamTest"]  = setup_stream_test;
    dx_cmds["TimeStreamDownload"] = time_stream_download;
    dx_cmds["WakeDevices"]      = wake_sleeping_devices;
    dx_cmds["HttpGet"]          = http_get;
    dx_cmds["CloudDocHttp"]     = dispatch_clouddochttp_cmd;
    dx_cmds["PicStreamHttp"]    = dispatch_picstreamhttp_cmd;
    dx_cmds["Dataset"]          = dispatch_dataset_cmd;
    dx_cmds["CCDConfig"]        = dispatch_ccdconfig_cmd;
    dx_cmds["Target"]           = dispatch_target_cmd;
    dx_cmds[GET_SYSTEM_STATE_CMD] = get_sys_state;
    dx_cmds[GET_SYNC_STATE_CMD] = get_sync_state;
    dx_cmds["ReportLanDevices"] = report_lan_devices;
    dx_cmds["ListLanDevices"]   = list_lan_devices;
    dx_cmds["ProbeLanDevices"]  = probe_lan_devices;
    dx_cmds["GetStorageNodePorts"] = get_storage_node_ports;
    dx_cmds["RemoteFile"]       = dispatch_fstest_cmd;
    dx_cmds["MediaRF"]          = dispatch_fstest_cmd;
    dx_cmds["ListStorageNodeDatasets"] = list_storage_node_datasets;
    dx_cmds["VSSIStressTest"]       = dispatch_vssi_stress_test_cmd;
    dx_cmds["ReportNetworkConnected"] = report_network_connected;

    dx_cmds["AddDevice"]         = add_device;
    dx_cmds["DelDevice"]         = del_device;
    dx_cmds["DeployRemoteAgent"] = deploy_remote_agent;
    dx_cmds["StartRemoteAgent"]  = start_remote_agent;
    dx_cmds["StopRemoteAgent"]   = stop_remote_agent;
    dx_cmds["RestartRemoteAgentApp"]   = restart_remote_agent_app;
    dx_cmds["CollectCCLog"]      = collect_cc_log;
    dx_cmds["CleanCCLog"]        = clean_cc_log;
	#if (defined LINUX || defined __CLOUDNODE__) && !defined LINUX_EMB
    dx_cmds["EnableInMemoryLogging"]    = enable_in_memory_logging;
    dx_cmds["DisableInMemoryLogging"]   = disable_in_memory_logging;
	#endif
    dx_cmds["FlushInMemoryLogs"]        = flush_in_memory_logs;

    dx_cmds[POWER_STR]                    = power_dispatch;
    g_power_cmds["FgMode"]                = foreground_mode;
    g_power_cmds["BgTasks"]               = background_tasks;
    g_power_cmds["SetBgModeInterval"]     = set_background_mode_interval;
    g_power_cmds["SetEnableNetwork"]      = set_enable_network;
    g_power_cmds["SetAllowBgData"]        = set_allow_bg_data;
    g_power_cmds["SetAllowAutoSync"]      = set_allow_auto_sync;
    g_power_cmds["SetAllowMobileNetwork"] = set_allow_mobile_network;
    g_power_cmds["SetOnlyMobileNetwork"]  = set_only_mobile_network_avail;
    g_power_cmds["SetStreamPowerMode"]    = set_stream_power_mode;
    g_power_cmds["SetCloudDocSync"]       = set_clouddoc_sync;
    g_power_cmds["SyncOnce"]              = sync_once;
    g_power_cmds["Status"]                = power_get_status;

#if 0
    dx_cmds["UploadLogs"]       = upload_logs;
#endif

    dx_cmds[NOTES_STR]                = notes_dispatch;
    g_notes_cmds["Status"]            = cmd_notes_status;
    g_notes_cmds["Enable"]            = cmd_notes_enable;
    g_notes_cmds["Disable"]           = cmd_notes_disable;

    dx_cmds[PICSTREAM_STR]                = picstream_dispatch;
    g_picstream_cmds["Status"]            = cmd_cr_check_status;
    g_picstream_cmds["SetEnableUpload"]   = cmd_cr_upload_enable;
    g_picstream_cmds["Clear"]             = cmd_cr_clear;
    g_picstream_cmds["UploadPhoto"]       = cmd_cr_upload_photo;
    g_picstream_cmds["UploadPhotoDelete"] = cmd_cr_upload_big_photo_delete;
    g_picstream_cmds["UploadPhotoSlow"]   = cmd_cr_upload_big_photo_slow;
    g_picstream_cmds["UploadDirsAdd"]     = cmd_cr_upload_dirs_add;
    g_picstream_cmds["UploadDirsRm"]      = cmd_cr_upload_dirs_rm;
    g_picstream_cmds["SendFileToStream"]  = cmd_cr_send_file_to_stream;
    g_picstream_cmds["SetFullResDir"]     = cmd_cr_set_full_res_dl_dir;
    g_picstream_cmds["SetLowResDir"]      = cmd_cr_set_low_res_dl_dir;
    g_picstream_cmds["TriggerUploadDir"]  = cmd_cr_trigger_upload_dir;
    g_picstream_cmds["SetThumbDir"]       = cmd_cr_set_thumb_dl_dir;
    g_picstream_cmds["ListItems"]       = cmd_cr_list_items;
    g_picstream_cmds["SetEnableGlobalDelete"]   = cmd_cr_global_delete_enable;

    dx_cmds[PHOTO_SHARE_STR]              = photo_share_dispatch;
    g_photo_share_cmds["Enable"]          = cmd_photo_share_enable;
    g_photo_share_cmds["Store"]           = cmd_sharePhoto_store;
    g_photo_share_cmds["Share"]           = cmd_sharePhoto_share;
    g_photo_share_cmds["Unshare"]         = cmd_sharePhoto_unshare;
    g_photo_share_cmds["DeleteSwm"]       = cmd_sharePhoto_deleteSharedWithMe;
    g_photo_share_cmds["DumpSwm"]         = cmd_sharePhoto_dumpSharedWithMe;
    g_photo_share_cmds["DumpSbm"]         = cmd_sharePhoto_dumpSharedByMe;

    dx_cmds[CLOUDMEDIA_STR]       = cloudmedia_commands;
    g_cloudmedia_cmds["AddMusic"] = cloudmedia_add_music;
    g_cloudmedia_cmds["AddPhoto"] = cloudmedia_add_photo;
    g_cloudmedia_cmds["AddPhoto2Album"] = cloudmedia_add_photo_to_album;
#if defined(CLOUDNODE)
    g_cloudmedia_cmds["CloudnodeAddMusic"] = cloudnode_cloudmedia_add_music;
    g_cloudmedia_cmds["CloudnodeAddPhoto"] = cloudnode_cloudmedia_add_photo;
#endif // CLOUDNODE
    g_cloudmedia_cmds["DeleteObject"] = cloudmedia_delete_object;
    g_cloudmedia_cmds["DeleteCollection"] = cloudmedia_delete_collection;

    dx_cmds[AUTOTEST_STR]   = autotest_commands;
    dx_cmds[TSTEST_STR]     = ts_test;
    dx_cmds[SYNCBOX_STR]    = syncbox_test_commands;

    dx_cmds[MCA_STR] = mca_dispatch;
    g_mca_cmds["MigrateThumb"] = mca_migrate_thumb_cmd;
    g_mca_cmds["StopThumbSync"] = mca_stop_thumb_sync_cmd;
    g_mca_cmds["ResumeThumbSync"] = mca_resume_thumb_sync_cmd;
    g_mca_cmds["Status"] = mca_status_cmd;
    g_mca_cmds["ListMetadata"] = mca_list_metadata;
    g_mca_cmds["ListAllPhotos"] = mca_list_allphotos;
    g_mca_cmds["CountMetadataFiles"] = mca_count_metadata_files_cmd;

    dx_cmds["MiniDMS"]       = dispatch_minidms_test_cmd;
    dx_cmds["EnableCloudPC"] = enable_cloudpc;
    dx_cmds["PostponeSleep"] = postpone_sleep;
 
    dx_cmds[USERLOGIN_STR]    = userlogin_dispatch;
    g_userlogin_cmds["SetEula"] = userlogin_set_eula;

    dx_cmds[VCS_CMD_STR] = vcs_dispatch;
    g_vcs_cmds["StartSession"] = cmd_vcs_start_session;
    g_vcs_cmds["SessionInfo"] = cmd_vcs_print_session_info;
    g_vcs_cmds["Tree"] = cmd_vcs_tree;
    g_vcs_cmds["GetDir"] = cmd_vcs_get_dir;
    g_vcs_cmds["GetDirPaged"] = cmd_vcs_get_dir_paged;
    g_vcs_cmds["DeleteFile"] = cmd_vcs_delete_file;
    g_vcs_cmds["DeleteDir"] = cmd_vcs_delete_dir;
    g_vcs_cmds["RmDirDashRf"] = cmd_vcs_rm_dash_rf;  // Very destructive
    g_vcs_cmds["MakeDir"] = cmd_vcs_make_dir;
    g_vcs_cmds["GetCompId"] = cmd_vcs_get_comp_id;
    g_vcs_cmds["FileGet"] = cmd_vcs_access_info_for_file_get;
    g_vcs_cmds["FilePut"] = cmd_vcs_access_info_for_file_put;
    g_vcs_cmds["VirtualFilePut"] = cmd_vcs_file_put_virtual;
    g_vcs_cmds["VirtualFilePutDeferred"] = cmd_vcs_file_put_deferred;
    g_vcs_cmds["PostAcsURL"] = cmd_vcs_post_acs_url;
    g_vcs_cmds["GetFileMetadata"] = cmd_vcs_get_file_metadata;
    g_vcs_cmds["DatasetInfo"] = cmd_vcs_get_dataset_info;

    dx_cmds[VCSPIC_CMD_STR] = vcspic_dispatch;
    g_vcspic_cmds["FilePut"] = cmd_vcs_v1_access_info_for_file_put;
    //g_vcspic_cmds["GetFileMetadata"] = cmd_vcs_v1_get_file_metadata;

    dx_cmds["UserLogout"] = userlogout;
    dx_cmds["LinkDevice"] = link_device;
    dx_cmds["UnlinkDevice"] = unlink_device;
    dx_cmds["RemoteSwUpdate"] = remote_sw_update;
    dx_cmds["ListDatasets"] = list_datasets;

    dx_cmds["ReportDifferentNetwork"] = report_different_network;
}

static void init_dx_not_remote_cmds()
{

//    dx_not_remote_cmds["CheckCloudDoc"] = std::set<std::string>();
//    dx_not_remote_cmds["CloudMedia"].insert("AddMusic");
//    dx_not_remote_cmds["CloudMedia"].insert("AddPhoto");
//    dx_not_remote_cmds["CloudMedia"].insert("DeleteObject");
//    dx_not_remote_cmds["CloudMedia"].insert("DeleteCollection");
}

static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

// TODO: Document the layout of the file to be parsed.
// TODO: Config lines should have consistent field index to meaning relationships.
//       Example: The 5th field should always have the same meaning, not IP address for some platforms and port
//       for others. Put optional fields at the end of the line form most to least common.
static int parse_remote_agent_cfg(std::map<std::string, remote_agent_config> &remote_agents)
{
    int rv = VPL_OK;
    VPLFS_stat_t stat;
    std::string config_path;
    std::fstream cfgfile;
    std::string line;
    std::map<std::string, remote_agent_config>::iterator it, it2;

    rv = getCurDir(config_path);
    if (rv < 0) {
        LOG_ERROR("Unable to get current folder path, rv = %d", rv);
        goto err;
    }
    config_path += "/"REMOTE_AGENT_CONFIG_FILE;

    if (VPLFS_Stat(config_path.c_str(), &stat) != VPL_OK) {
        LOG_ERROR("Config file %s doesn't exist", config_path.c_str());
        rv = VPL_ERR_NOENT;
        goto err;
    }
    LOG_ALWAYS("Config file = %s", config_path.c_str());

    // clean-up before parsing the configuration file
    remote_agents.clear();

    cfgfile.open(config_path.c_str());

    if (!cfgfile.is_open()) {
        LOG_ERROR("Open file %s failed", config_path.c_str());
        rv = VPL_ERR_FAIL;
        goto err_file_open;
    }

    while (std::getline(cfgfile, line)) {
        // search for the starting tag and ending tag of either dxshell_rc or device_info
        if (line.find(TAG_DXRC_HEAD) != std::string::npos) {
            while (std::getline(cfgfile, line)) {
                if (line.find("#") == 0) {
                    // skip comment out line
                    //LOG_ALWAYS("COMMENT LINE: %s", line.c_str());
                    continue;
                }
                // break if it's end of TAG_DXRC_TAIL
                if (line.find(TAG_DXRC_TAIL) != std::string::npos) {
                    //LOG_ALWAYS("EOF DXRC found: %s", line.c_str());
                    break;
                }
                std::vector<std::string> tokens;
                split(line, ',', tokens);
                //LOG_ALWAYS("PARSE LINE: %s, token # = %d", line.c_str(), tokens.size());
                if (tokens.size() < 5) {
                    LOG_ALWAYS("Invalid configuration: %s. Skip", line.c_str());
                    continue;
                }
                remote_agent_config cfg;
                if (remote_agents.find(tokens[0]) != remote_agents.end()) {
                    cfg = remote_agents[tokens[0]];
                }
                cfg.machine_os = tokens[1];
                cfg.machine_username = tokens[2];
                cfg.machine_ip = tokens[3];  
                if (cfg.machine_os.compare(OS_ANDROID) == 0 || cfg.machine_os.compare(OS_WINDOWS_RT) == 0|| cfg.machine_os.compare(OS_iOS) == 0) {
                    cfg.machine_port = std::string("24000");
                } else {
                    cfg.machine_port = tokens[4];
                }

                if (cfg.machine_os.compare(OS_ANDROID) == 0) {
                    cfg.controller_ip = tokens[5];
                    cfg.controller_port = tokens[6];
                } 
                else if (cfg.machine_os.compare(OS_iOS) == 0) {
                    cfg.controller_ip = tokens[5];
                    cfg.controller_port = std::string("");
                } else {
                    cfg.controller_ip = std::string("");
                    cfg.controller_port = std::string("");
                }
                remote_agents[tokens[0]] = cfg;
            }
            // skip end of DXRC
            continue;
        }
        if (line.find(TAG_DEVICEINFO_HEAD) != std::string::npos) {
            while (std::getline(cfgfile, line)) {
                if (line.find("#") == 0) {
                    // skip comment out line
                    //LOG_ALWAYS("COMMENT LINE: %s", line.c_str());
                    continue;
                }
                // break if it's end of TAG_DEVICEINFO_TAIL
                if (line.find(TAG_DEVICEINFO_TAIL) != std::string::npos) {
                    //LOG_ALWAYS("EOF DXRC found: %s", line.c_str());
                    break;
                }
                //LOG_ALWAYS("PARSE LINE: %s", line.c_str());
                std::vector<std::string> tokens;
                split(line, ',', tokens);
                if (tokens.size() < 2) {
                    LOG_ALWAYS("Invalid device configuration: %s. Skip", line.c_str());
                    continue;
                }
                remote_agent_config cfg;
                if (remote_agents.find(tokens[0]) != remote_agents.end()) {
                    cfg = remote_agents[tokens[0]];
                }
                cfg.machine_name = tokens[1];
                remote_agents[tokens[0]] = cfg;
            }
            // skip end of DXRC
            continue;
        }
    }

    // Sanity Check: check OS and DeviceName not empty.
    for(it = remote_agents.begin(); it != remote_agents.end();) {
        it2 = it++;
        if (it2->second.machine_os.empty() || it2->second.machine_name.empty()) {
            LOG_ALWAYS("Incomplete configuration [%s]. Drop!", it2->first.c_str());
            remote_agents.erase(it2);
        }
    }

    // Check if there are two different targets with the same control ip and port.
    for(it = remote_agents.begin(); it != remote_agents.end(); it++) {
        it2 = it;
        it2++;
        for(; it2 != remote_agents.end(); it2++) {
            if((it->second.machine_ip.compare(it2->second.machine_ip.c_str()) == 0) && (it->second.machine_port.compare(it2->second.machine_port.c_str()) == 0)) {
                LOG_ERROR("Targets %s and %s have same IP and port %s:%s",
                          it->first.c_str(), it2->first.c_str(), it->second.machine_ip.c_str(), it->second.machine_port.c_str());
                rv = VPL_ERR_FAIL;
                goto err_file_open;
            }
        }
    }

err_file_open:
    cfgfile.close();
err:
    return rv;
}

static int set_env_ip_port(const std::map<std::string, remote_agent_config>::iterator& it)
{
    int rv = 0;
#ifdef WIN32
    if (_putenv_s(DX_REMOTE_IP_ENV, it->second.machine_ip.c_str()) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%s", it->second.machine_ip.c_str());
        return VPL_ERR_FAIL;
    }
    if (_putenv_s(DX_REMOTE_PORT_ENV, it->second.machine_port.c_str()) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%s", it->second.machine_port.c_str());
        return VPL_ERR_FAIL;
    }
#else
    if (setenv(DX_REMOTE_IP_ENV, it->second.machine_ip.c_str(), 1) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%s", it->second.machine_ip.c_str());
        return VPL_ERR_FAIL;
    }
    if (setenv(DX_REMOTE_PORT_ENV, it->second.machine_port.c_str(), 1) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%s", it->second.machine_port.c_str());
        return VPL_ERR_FAIL;
    }
#endif

    return rv;
}

// this function only support for both of Android and iOS
static int set_env_control_ip_port(const std::map<std::string, remote_agent_config>::iterator& it)
{
    int rv = 0;
    if (it->second.machine_os.compare(OS_ANDROID) == 0 || it->second.machine_os.compare(OS_iOS) == 0) {
#ifdef WIN32
        if (_putenv_s(DX_REMOTE_IP_ENV, it->second.controller_ip.c_str()) != 0) {
            LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%s", it->second.controller_ip.c_str());
            return VPL_ERR_FAIL;
        }
        if (_putenv_s(DX_REMOTE_PORT_ENV, it->second.controller_port.c_str()) != 0) {
            LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%s", it->second.controller_port.c_str());
            return VPL_ERR_FAIL;
        }
#else
        if (setenv(DX_REMOTE_IP_ENV, it->second.controller_ip.c_str(), 1) != 0) {
            LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%s", it->second.machine_ip.c_str());
            return VPL_ERR_FAIL;
        }
        if (setenv(DX_REMOTE_PORT_ENV, it->second.controller_port.c_str(), 1) != 0) {
            LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%s", it->second.machine_port.c_str());
            return VPL_ERR_FAIL;
        }
#endif
    }

    return rv;
}

int set_domain(int argc, const char* argv[])
{
    int rv = 0;
    std::string tmp;
    std::string credPath;
    TargetDevice *target = NULL;

    if (checkHelp(argc, argv)) {
        printf("%s [domain]: If domain is not specified, default domain is used\n", argv[0]);
        return 0;   
    }
   	LOG_ALWAYS("000000000000000000000000000000000"); // jimmy
    LOG_ALWAYS("Executing..");
    rv = checkRunningCcd();
   	LOG_ALWAYS("111111111111111111111111111111111"); // jimmy
    if (rv != 0) {
        LOG_ERROR("An active CCD was detected. Please stop CCD ("EXE_NAME" StopCCD)");
        goto exit;
    }

    rv = ccdconfig_set(CCDCONF_INFRADOMAIN_KEY, argc > 1 ? argv[1] : DEFAULT_INFRA_DOMAIN);
   	LOG_ALWAYS("222222222222222222222222222222222"); // jimmy
    if (rv < 0) {
        if (argc > 1) {
            LOG_ERROR("Failed to set domain to %s: %d", argv[1], rv);
        }
        else {
            LOG_ERROR("Failed to reset domain to default: %d", rv);
        }
    }

    // Set default group for backward compatibility
    rv = ccdconfig_set(CCDCONF_USERGROUP_KEY, DEFAULT_USER_GROUP);
   	LOG_ALWAYS("3333333333333333333333333333333333"); // jimmy
    if (rv < 0) {
            LOG_ERROR("Failed to reset group to default: %d", rv);
    }

    // do not remove the following code
    // the message is needed by pubtests.sh
    {
   		LOG_ALWAYS("44444444444444444444444444444444444444"); // jimmy
        std::string confDir;
   		LOG_ALWAYS("55555555555555555555555555555555555555"); // jimmy
        if (getCcdAppDataPath(confDir) == 0) {
            confDir.append(DIR_DELIM "conf");
            LOG_ALWAYS("Config folder %s created", confDir.c_str());
        }
    }

    LOG_ALWAYS("Clearing old credentials");
    target = getTargetDevice();
    target->removeDeviceCredentials();

exit:
    if (target != NULL)
        delete target;
    return rv;
}

int set_sync_mode_upload(int argc, const char* argv[])
{
    int rc;
    int rv = 0;
    std::string toSet;
    if(argc == 2) {
        toSet = argv[1];
        if (! (toSet == "0" || toSet == "1" || toSet == "2"))
        {   // Invalid arguments
            toSet.clear();
            rv = -1;
        }
    } else {
        rv = -2;
    }

    if (checkHelp(argc, argv) || rv != 0) {
        if(checkHelp(argc, argv)) {
            rv = 0;
        }
        printf("%s <sync_up_mode:(0,1,or 2)>: Meant to be called after SetDomain"
               "0-->Normal Sync old behavior, "
               "1-->Hybrid sync postToVcs then uploadFile then postUrl, "
               "2-->PureVirtualSync postToVcsOnly\n", argv[0]);
        return rv;
    }

    rv = checkRunningCcd();
    if (rv != 0) {
        LOG_ERROR("An active CCD was detected. Please stop CCD ("EXE_NAME" StopCCD)");
        goto exit;
    }

    LOG_ALWAYS("Executing.  Setting sync_mode_upload to %s (%s)",
               argv[1], CCDCONF_ENABLE_UPLOAD_VIRTUAL_SYNC);

    rc = ccdconfig_set(CCDCONF_ENABLE_UPLOAD_VIRTUAL_SYNC, toSet);
    if (rc != 0) {
        LOG_ERROR("Failed to set sync_mode_upload to %s: %d", toSet.c_str(), rc);
        rv = rc;
    }

    {
        // TODO: Enable "test" streaming code.  Will not be needed once this is enabled
        // by default.
        std::string enableTs = "13";
        rc = ccdconfig_set(CCDCONF_ENABLE_TS, enableTs);
        if (rc != 0) {
            LOG_ERROR("Failed to set enableTs to %s: %d", enableTs.c_str(), rc);
            rv = rc;
        }

        // TODO: Internal direct connection: this can be used to specify connection type.
        //       For now, the other modes aren't stable.  Won't be needed once all modes
        //       are stable.
        std::string clearfiMode = "14";
        rc = ccdconfig_set(CCDCONF_CLEARFI_MODE, clearfiMode);
        if (rc != 0) {
            LOG_ERROR("Failed to set clearfi mode to %s: %d", clearfiMode.c_str(), rc);
            rv = rc;
        }
    }

 exit:
    return rv;
}

int set_sync_mode_download(int argc, const char* argv[])
{
    int rc;
    int rv = 0;
    std::string toSet;
    if(argc == 2) {
        toSet = argv[1];
        if (! (toSet == "0" || toSet == "1"))
        {   // Invalid arguments
            toSet.clear();
            rv = -1;
        }
    } else {
        rv = -2;
    }

    if (checkHelp(argc, argv) || rv != 0) {
        if(checkHelp(argc, argv)) {
            rv = 0;
        }
        printf("%s <sync_down_mode:(0 or 1)>: Meant to be called after SetDomain"
               "0-->Normal Sync old behavior, "
               "1-->PureVirtualSync no file downloads\n", argv[0]);
        return rv;
    }

    rv = checkRunningCcd();
    if (rv != 0) {
        LOG_ERROR("An active CCD was detected. Please stop CCD ("EXE_NAME" StopCCD)");
        goto exit;
    }

    LOG_ALWAYS("Executing.  Setting sync_mode_download to %s (%s)",
               argv[1], CCDCONF_ENABLE_ARCHIVE_DOWNLOAD);

    rc = ccdconfig_set(CCDCONF_ENABLE_ARCHIVE_DOWNLOAD, toSet);
    if (rc != 0) {
        LOG_ERROR("Failed to set sync_mode_download to %s: %d", toSet.c_str(), rc);
        rv = rc;
    }

    {
        // TODO: Enable "test" streaming code.
        std::string enableTs = "13";
        rc = ccdconfig_set(CCDCONF_ENABLE_TS, enableTs);
        if (rc != 0) {
            LOG_ERROR("Failed to set enableTs to %s: %d", enableTs.c_str(), rc);
            rv = rc;
        }

        // TODO: Internal direct connection: this can be used to specify connection type.
        //       For now, the other modes aren't stable.  Won't be needed once all modes
        //       are stable.
        std::string clearfiMode = "14";
        rc = ccdconfig_set(CCDCONF_CLEARFI_MODE, clearfiMode);
        if (rc != 0) {
            LOG_ERROR("Failed to set clearfi mode to %s: %d", clearfiMode.c_str(), rc);
            rv = rc;
        }
    }

 exit:
    return rv;
}

int set_group(int argc, const char* argv[])
{
    int rv = 0;
    std::string tmp;
    std::string credPath;
    TargetDevice *target = NULL;

    if (checkHelp(argc, argv)) {
        printf("%s [group]: If group is not specified, default group is used\n", argv[0]);
        return 0;   
    }

    LOG_ALWAYS("Executing..");

    rv = checkRunningCcd();
    if (rv != 0) {
        LOG_ERROR("An active CCD was detected. Please stop CCD ("EXE_NAME" StopCCD)");
        goto exit;
    }

    rv = ccdconfig_set(CCDCONF_USERGROUP_KEY, argc > 1 ? argv[1] : DEFAULT_USER_GROUP);
    if (rv < 0) {
        if (argc > 1) {
            LOG_ERROR("Failed to set group to %s: %d", argv[1], rv);
        }
        else {
            LOG_ERROR("Failed to reset group to default: %d", rv);
        }
    }

    // do not remove the following code
    // the message is needed by pubtests.sh
    {
        std::string confDir;
        if (getCcdAppDataPath(confDir) == 0) {
            confDir.append(DIR_DELIM "conf");
            LOG_ALWAYS("Config folder %s created", confDir.c_str());
        }
    }

    LOG_ALWAYS("Clearing old credentials");
    target = getTargetDevice();
    target->removeDeviceCredentials();

exit:
    if (target != NULL)
        delete target;
    return rv;
}

static int start_ccd_remote(const char* titleId)
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_LAUNCH_PROCESS);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(std::string("CCD.exe"));
    if (titleId != NULL)
    {
        igware::dxshell::DxRemoteMessage_DxRemoteArgument *titleIdArg = myReq.add_argument();
        titleIdArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTEXECUTEARG);
        titleIdArg->set_value(titleId);
    }
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("StartCCD Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Fail to start CCD: %d", rc);
    } else {
        LOG_ALWAYS("Success to start CCD");
    }
    return rc;
}

static int stop_ccd_remote()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_KILL_PROCESS);
    igware::dxshell::DxRemoteMessage_DxRemoteArgument *myArg = myReq.add_argument();
    myArg->set_name(igware::dxshell::DxRemoteMessage_ArgumentName_DXARGUMENTFILENAME);
    myArg->set_value(std::string("CCD.exe"));
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("StopCCD Executing..");
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
end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Fail to stop CCD: %d", rc);
    } else {
        LOG_ALWAYS("Success to stop CCD");
    }
    return rc;
}

int launch_android_cc_service()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_LAUNCH_CONNECTED_ANDROID_CC_SERVICE);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("launch_android_cc_service Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to launch_android_cc_service");
    } else {
        LOG_ALWAYS("Success to launch_android_cc_service");
    }
    return rc;
}

int launch_android_dx_remote_agent()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_LAUNCH_CONNECTED_ANDROID_DXREMOTEAGENT);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("launch_android_dx_remote_agent Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to launch_android_dx_remote_agent");
    } else {
        LOG_ALWAYS("Success to launch_android_dx_remote_agent");
    }
    return rc;
}

int stop_android_cc_service()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_STOP_CONNECTED_ANDROID_CC_SERVICE);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("stop_android_cc_service Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to stop_android_cc_service");
    } else {
        LOG_ALWAYS("Success to stop_android_cc_service");
    }
    return rc;
}

int stop_android_dx_remote_agent()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_STOP_CONNECTED_ANDROID_DXREMOTEAGENT);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("stop_android_dx_remote_agent Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to stop_android_dx_remote_agent");
    } else {
        LOG_ALWAYS("Success to stop_android_dx_remote_agent");
    }
    return rc;
}

int restart_android_dx_remote_agent()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_RESTART_CONNECTED_ANDROID_DXREMOTEAGENT);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("restart_android_dx_remote_agent Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to restart_android_dx_remote_agent");
    } else {
        LOG_ALWAYS("Success to restart_android_dx_remote_agent");
    }
    return rc;
}

int get_ccd_log_from_android()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_GET_CONNECTED_ANDROID_CCD_LOG);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("get_ccd_log_from_android Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to get_ccd_log_from_android");
    } else {
        LOG_ALWAYS("Success to get_ccd_log_from_android");
    }
    return rc;
}

int clean_ccd_log_on_android()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_CLEAN_CONNECTED_ANDROID_CCD_LOG);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("clean_ccd_log_on_androi Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to clean_ccd_log_on_android");
    } else {
        LOG_ALWAYS("Success to clean_ccd_log_on_android");
    }
    return rc;
}

int check_android_net_status()
{
    int rc = VPL_OK;
    VPLNet_addr_t ipaddr;
    u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;

    std::string input, output;
    igware::dxshell::DxRemoteMessage myReq, myRes;
    myReq.set_command(igware::dxshell::DxRemoteMessage_Command_CHECK_CONNECTED_ANDROID_NET_STATUS);
    input = myReq.SerializeAsString();

    char const* env_str;

    // If DX_REMOTE_IP is set, connect to a TCP socket instead.
    if ((env_str = getenv(DX_REMOTE_IP_ENV)) == NULL) {
        LOG_ALWAYS("No remote ip address found! Abort");
        return VPL_ERR_FAIL;
    }
    ipaddr = VPLNet_GetAddr(env_str);

    // DX_REMOTE_PORT is optional.
    if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
        port = static_cast<u16>(strtoul(env_str, NULL, 0));
    }

    RemoteAgent ragent(ipaddr, (u16)port);

    LOG_ALWAYS("check_android_net_outgoing Executing..");
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

end:
    if (rc != VPL_OK) {
        LOG_ALWAYS("Failed to check_android_net_status");
    } else {
        LOG_ALWAYS("Success to check_android_net_status");
    }
    return rc;
}

int launch_dx_remote_agent_app(const char *alias)
{
    std::stringstream ssCmd;
 
    ssCmd << "Remote_Agent_Launcher.py \"LaunchApp\" \"" << alias << "\"";
    int rv = doSystemCall(ssCmd.str().c_str());
    return rv;
}

int stop_dx_remote_agent_app(const char *alias)
{
    std::stringstream ssCmd;

    ssCmd << "Remote_Agent_Launcher.py \"StopApp\" \"" << alias << "\"";
    int rv = doSystemCall(ssCmd.str().c_str());
    return rv;
}

int restart_dx_remote_agent_ios_app(const char *alias)
{
    std::stringstream ssCmd;
    int rv = 0;

    do
    {
        ssCmd << "Remote_Agent_Launcher.py \"RestartiOSApp\" \"" << alias << "\"";
        rv = doSystemCall(ssCmd.str().c_str());
        if(rv != 0) {
            LOG_ERROR("DxRemoteAgent[%s] Restart iOS App fail !!!", alias);
            break;
        }

        if(set_target_machine(alias) == 0) {
            if ((rv = remote_agent_poll(alias)) != 0) {
                LOG_ERROR("DxRemoteAgent[%s] did not create connection with dxshell in %d seconds", alias, DX_REMOTE_POLL_TIME_OUT);
                break;
            }
        }
    } while(false);

    return rv;
}

int remote_agent_poll(const char *alias)
{
    int rc = VPL_OK;
    TargetDevice *target = NULL;
    VPLTime_t startTime = VPLTime_GetTimeStamp();
    VPLTime_t endTime = startTime + VPLTime_FromSec(DX_REMOTE_POLL_TIME_OUT);

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("target is NULL, return!");
        return rc;
    }

    while(VPLTime_GetTimeStamp() < endTime) {
        rc = target->checkRemoteAgent();
        if (rc != VPL_OK) {
            LOG_ERROR("checkRemoteAgent[%s] failed! %d. Check communication with dx_remote_agent!", alias, rc);
        }
        else {
            LOG_ALWAYS("DxRemoteAgent[%s] is connected!", alias);
            break;
        }
    }

    if (target != NULL)
        delete target;
    return rc;
}

int start_ccd(int argc, const char* argv[])
{
    int rv = 0;
    const char* titleId = NULL;

    if (checkHelp(argc, argv)) {
        printf("%s [titleId]\n", argv[0]);
        return 0;
    }

    if (argc > 1) {
        titleId = argv[1];
    }

    LOG_ALWAYS("Executing..");
    LOG_ALWAYS("before getenv(DX_REMOTE_IP_ENV)"); // jimmy
	//LOG_ALWAYS("DX_REMOTE_IP_ENV: %s", &DX_REMOTE_IP_ENV); // jimmy
    if (getenv(DX_REMOTE_IP_ENV) == NULL) {
    	LOG_ALWAYS("if");
    	LOG_ALWAYS("before startCcd");
		//LOG_ALWAYS("titleId: %d\n", titleId);
        rv = startCcd(titleId);
    	LOG_ALWAYS("after startCcd");
    } else {
    	LOG_ALWAYS("else");
        TargetDevice *target = NULL;
    	LOG_ALWAYS("after getTargetDevice");
        target = getTargetDevice();
    	LOG_ALWAYS("before target->getOsVersion");
        std::string osVersion = target->getOsVersion();
        if (target != NULL)
            delete target;
    	LOG_ALWAYS("before start_ccd_remote");
        rv = start_ccd_remote(titleId);
    	LOG_ALWAYS("after start_ccd_remote");
    }
    LOG_ALWAYS("before failed to start CCD");
    if (rv != 0) {
        LOG_ERROR("Fail to start CCD");
        goto exit;
    }
   	LOG_ALWAYS("before fail waiting for CCD");
    rv = waitCcd();
    if (rv != 0) {
        LOG_ERROR("Fail waiting for CCD");
        goto exit;
    }

exit:
    return rv;
}

#if defined (CLOUDNODE)

// Internal only version of start_ccd
// for starting client ccd in some cloudnode tests (bug 9608)

int start_ccd_in_client_subdir()
{
    int rv = 0;

    LOG_ALWAYS("Executing..");
    rv = startCcdInClientSubdir();
    if (rv != 0) {
        LOG_ERROR("Fail to start CCD");
        goto exit;
    }

    rv = waitCcd();
    if (rv != 0) {
        LOG_ERROR("Fail waiting for CCD");
        goto exit;
    }

exit:
    return rv;
}

#endif



#if CCDSRV_MON_TEST
#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
int start_ccd_by_srv(int argc, const char* argv[])
{
    int rv = -1;
    HANDLE hToken = NULL;
    BOOL bResult = TRUE;
    DWORD dwSize = 0;
    LPSTR userSid=NULL;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   // No arguments needed
    }

    CCDMonSrv_initClient();

    bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
    if (!bResult) {
        LOG_ERROR("OpenProcessToken failed!");
        goto out;
    }
    {
        CCDMonSrv::REQINPUT input;
        CCDMonSrv::REQOUTPUT output;

        // Get token of the user
        bResult = GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize);
        PTOKEN_USER pTokenUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenUser, pTokenUserInfo, dwSize, &dwSize);
        if (!bResult) {
            LOG_ERROR("GetTokenInformation failed!");
            goto out;
        }
        if (!ConvertSidToStringSidA(pTokenUserInfo->User.Sid, &userSid)) {
            LOG_ERROR("ConvertSidToStringSidA failed!");
            goto out;
        }

        {
            input.set_type(CCDMonSrv::NEW_CCD);
            input.set_sid(userSid);
            std::string path;
            int rc = getCcdAppDataPath(path);
            if (rc != 0) {
                LOG_ERROR("getCcdAppDataPath failed: %d", rc);
                rv = rc;
                goto out;
            }

            input.set_localpath(path.c_str());
            DWORD rt = ::CCDMonSrv_sendRequest(input, output);
            if (rt == 0) {
                switch(output.result()) {
                    case CCDMonSrv::SUCCESS:
                        LOG_INFO("success: %s", output.DebugString().c_str());
                        rv = 0;
                        break;
                    case CCDMonSrv::ERROR_CREATE_PROCESS:
                    case CCDMonSrv::ERROR_UNKNOWN:
                        LOG_ERROR("failed: %s", output.DebugString().c_str());
                        break;
                }
            } else {
                // Usually, this "%ld" would be FMT_DWORD, but CCDMonSrv_sendRequest uses signed values
                // such as CCDM_ERROR_OTHERS.
                LOG_ERROR("CCDMonSrv_sendRequest failed: %ld; please make sure that CCDMonitorService is started.", rt);
            }
            if (userSid != NULL)
                LocalFree(userSid);
        }
    }

out:
    return rv;
}

int stop_ccd_by_srv(int argc, const char* argv[])
{
    int rv = -1;
    HANDLE hToken = NULL;
    BOOL bResult = TRUE;
    DWORD dwSize = 0;
    LPSTR userSid=NULL;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   // No arguments needed
    }

    CCDMonSrv_initClient();

    bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
    if (!bResult) {
        goto out;
    }
    {
        DWORD rt;
        CCDMonSrv::REQINPUT input;
        CCDMonSrv::REQOUTPUT output;

        // Get token of the user
        bResult = GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize);
        PTOKEN_USER pTokenUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenUser, pTokenUserInfo, dwSize, &dwSize);
        if (!bResult)
            goto out;
        if (!ConvertSidToStringSidA(pTokenUserInfo->User.Sid, &userSid))
            goto out;

        input.set_type(CCDMonSrv::CLOSE_CCD);
        input.set_sid(userSid);
        rt= ::CCDMonSrv_sendRequest(input, output);
        if (rt == 0) {
            switch(output.result()) {
            case CCDMonSrv::SUCCESS:
                LOG_INFO("%s", output.DebugString().c_str());
                rv = 0;
                break;
            case CCDMonSrv::ERROR_EMPTY_USER:
            case CCDMonSrv::ERROR_TERMINATE_PROCESS:
            case CCDMonSrv::ERROR_UNKNOWN:
                LOG_ERROR("%s", output.DebugString().c_str());
                break;
            }
        }
        if (userSid != NULL)
            LocalFree(userSid);
    }

out:
    return rv;
}
#endif // VPL_PLAT_IS_WIN_DESKTOP_MODE
#endif // CCDSRV_MON_TEST

int stop_ccd_hard(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Executing..");
    if (getenv(DX_REMOTE_IP_ENV) == NULL) {
        shutdownCcd();
    } else {
        stop_ccd_remote();
    }


    return rv;
}

int stop_ccd_soft(int argc, const char* argv[])
{
    int rv = 0;
    VPLTime_t start, end, checkTime;
    CCDIError rc = -1;

    TargetDevice *target = getTargetDevice();
    ON_BLOCK_EXIT(deleteObj<TargetDevice>, target);

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        // No arguments needed
        goto exit;
    }

    LOG_ALWAYS("Executing..");
    
    if(getenv(DX_REMOTE_IP_ENV) != NULL && (isWindows(target->getOsVersion()) || target->getOsVersion() == OS_ANDROID)){
        rv = stop_ccd_remote();
        if (rv != 0) {
            LOG_ERROR("clean shutdown failed:%d", rv);
            goto exit;
        }
    }else if(getenv(DX_REMOTE_IP_ENV) != NULL && (target->getOsVersion() == OS_WINDOWS_RT || isIOS(target->getOsVersion()))){
        if ((rv = stop_dx_remote_agent_app("MD")) != 0) {
            LOG_ERROR("Unable stop dx_remote_agent app: %d", rv);
            goto exit;
        }
    }else{
        ccd::UpdateSystemStateInput request;
        request.set_do_shutdown(true);
        ccd::UpdateSystemStateOutput response;
        rv = CCDIUpdateSystemState(request, response);
        if (rv != 0) {
            LOG_ERROR("clean shutdown failed:%d", rv);
            goto exit;
        }
    }

    // Bug 17937 Work Around to prevent dxshell hang in CCDI GetSystemState loop.
    if (target->getOsVersion() == OS_ANDROID) {
        VPLThread_Sleep(VPLTime_FromSec(5));
        goto exit;
    }

    // Confirm CCD is stopped. Limit to 10 seconds.
    // This also handles the Windows impl. requirement that another command follow a shutdown request.
    // For details, read the comment in ccd_core.cpp::mainLoop().
    start = VPLTime_GetTimeStamp();
    end = start + VPLTime_FromSec(10);
    while((checkTime = VPLTime_GetTimeStamp()) < end) {
        ccd::GetSystemStateInput request;
        ccd::GetSystemStateOutput response;
        rc = CCDIGetSystemState(request, response);

        // For the dx_remote_agent on Win32 case, we will get CCDI_ERROR_RPC_FAILURE (-14090) after the
        // CCD process stops serving the named socket and dx_remote_agent is still running.
        //
        // For the dx_remote_agent on any platform case, we expect VPL_ERR_CONNREFUSED (-9039) after the
        // remote agent process stops listening.
        //
        // For the non-RemoteAgent case, we should get VPL_ERR_NAMED_SOCKET_NOT_EXIST (-9055) after the
        // CCD process stops serving the named socket.  But there is also a high probability that we will
        // get one -14090 when CCD closes the named socket, since there can be a connection
        // that got closed without any response bytes being written to it.
        if(rc != 0 && rc != CCD_ERROR_SHUTTING_DOWN) {
            LOG_ALWAYS("clean shutdown assumed on CCDIGetSystemState result %d. Done in "FMT_VPLTime_t"us.",
                       rc, VPLTime_GetTimeStamp() - start);
            goto exit;
        }

        // Sleep at least half a second between checks so CCD isn't flooded during shutdown.
        checkTime = VPLTime_GetTimeStamp() - checkTime;
        if(checkTime < VPLTime_FromMillisec(500)) {
            VPLThread_Sleep(VPLTime_FromMillisec(500) - checkTime);
        }
    }
    LOG_ALWAYS("Cannot confirm clean shutdown; latest result was %d.", rc);

exit:
    return rv;
}

int start_cloudpc(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    ccd::UpdateSyncSettingsInput updateIn;
    ccd::UpdateSyncSettingsOutput updateOut;

    if (checkHelp(argc, argv)) {
        printf("%s username password\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");
    if (argc < 3) {
        LOG_ERROR("Invalid sub-command (Too few arguments %d)", argc);
        LOG_ERROR("Sub-command usage: "EXE_NAME" StartCloudpc <username> <password>");
        return -1;
    }

    LOG_ALWAYS("Login: Username(%s), Password(%s)", argv[1], argv[2]);
    rv = login(argv[1], argv[2], userId);
    if (rv != 0) {
        LOG_ERROR("login failed:%d", rv);
        goto fail_login;
    }

    LOG_ALWAYS("Link Device");
    rv = linkDevice(userId);
    if (rv != 0) {
        LOG_ERROR("linkDevice failed:%d", rv);
        goto fail_link;
    }

    VPLThread_Sleep(VPLTIME_FROM_SEC(1));

    LOG_ALWAYS("Register PSN");
    rv = registerPsn(userId);
    if (rv != 0) {
        LOG_ERROR("RegisterPsn failed: %d", rv);
        LOG_ALWAYS("UnlinkDevice");
        unlinkDevice(userId);
fail_link:
        LOG_ALWAYS("Logout");
        logout(userId);
        goto fail_login;
    }

    {
        //enable media server feature
        VPLThread_Sleep(VPLTIME_FROM_SEC(2)); //prevent "storage node is not media server capable" error
        const char *arg[] = { "UpdatePsn", "-M"};
        rv = update_psn(2, arg);
        if (rv != VPL_OK) {
            LOG_ERROR("Error while updating psn flag to enable mediaserver: flag = %s, rv = %d", arg[1], rv);
        }
    }

fail_login:

    return rv;
}

int stop_cloudpc(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        goto exit;
    }

    LOG_ALWAYS("Unlink Device userId("FMTu64")", userId);
    rv = unlinkDevice(userId);
    if (rv != 0) {
        LOG_ERROR("unlinkDevice failed:%d", rv);
    }

    LOG_ALWAYS("Logout userId("FMTu64")", userId);
    rv = logout(userId);
    if (rv != 0) {
        LOG_ERROR("logout failed:%d", rv);
        goto exit;
    }

exit:
    return rv;
}

int update_psn(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    u64 deviceId = 0;
    bool media_server_is_set = false;
    bool media_server_enabled = false;
    bool virt_drive_is_set = false;
    bool virt_drive_enabled = false;
    bool remote_file_access_is_set = false;
    bool remote_file_access_enabled = false;

    if (checkHelp(argc, argv)) {
        printf("%s flags\n", argv[0]);
        printf("  -M            # enable media server\n");
        printf("  -m            # disable media server\n");
        printf("  -V            # enable virt drive\n");
        printf("  -v            # disable virt drive\n");
        printf("  -R            # enable remote file access\n");
        printf("  -r            # disable remote file access drive\n");
        printf("  -d <deviceId> # device ID of the storage node\n");
        return 0;
    }
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'm': 
                media_server_is_set = true;
                media_server_enabled = false;
                break;
            case 'M':
                media_server_is_set = true;
                media_server_enabled = true;
                break;
            case 'V': 
                virt_drive_is_set = true;
                virt_drive_enabled = false;
                break;
            case 'v':
                virt_drive_is_set = true;
                virt_drive_enabled = true;
                break;
            case 'r': 
                remote_file_access_is_set = true;
                remote_file_access_enabled = false;
                break;
            case 'R':
                remote_file_access_is_set = true;
                remote_file_access_enabled = true;
                break;
            case 'd':
                i++;
                if ( i < argc ) {
                    deviceId = strtoull(argv[i], NULL, 0);
                }
                break;
            default:
                LOG_ALWAYS("Unknown option %c", argv[i][0]);
                rv = -1;
                goto exit;
            }
        }
        else {
            LOG_ALWAYS("Invalid arg %s", argv[i]);
            rv = -1;
            goto exit;
        }
    }

    if ( !media_server_is_set && !virt_drive_is_set && 
            !remote_file_access_is_set) {
        LOG_ALWAYS("no arguments specified.");
        rv = -1;
        goto exit;
    }

    LOG_ALWAYS("Executing..");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);
    rv = updateStorageNode(userId, deviceId, 
        media_server_is_set, media_server_enabled,
        virt_drive_is_set, virt_drive_enabled,
        remote_file_access_is_set, remote_file_access_enabled);
    resetDebugLevel();
    if (rv != 0) {
        LOG_ERROR("updateStorageNode() failed: %d", rv);
        goto exit;
    }
exit:
    return rv;
}

int start_client(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    ccd::UpdateSyncSettingsInput updateIn;
    ccd::UpdateSyncSettingsOutput updateOut;

    if (checkHelp(argc, argv)) {
        printf("%s username password\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");
    if (argc < 3) {
        LOG_ERROR("Invalid sub-command (Too few arguments)");
        LOG_ERROR("Sub-command usage: "EXE_NAME" StartClient <username> <password>");
        return -1;
    }

    LOG_ALWAYS("Login: Username(%s), Password(%s)", argv[1], argv[2]);
    rv = login(argv[1], argv[2], userId);
    if (rv != 0) {
        LOG_ERROR("login failed:%d", rv);
        goto fail_login;
    }

    LOG_ALWAYS("Link Device");
    rv = linkDevice(userId);
    if (rv != 0) {
        LOG_ERROR("linkDevice failed:%d", rv);
        LOG_ALWAYS("Logout");
        logout(userId);
    }

fail_login:
    return rv;
}

int stop_client(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    if(checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    int rc;

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("getUserIdBasic:%d", rv);
        goto exit;
    }

    LOG_ALWAYS("Unlink Device userId("FMTu64")", userId);
    rc = unlinkDevice(userId);
    if (rc != 0) {
        LOG_ERROR("unlinkDevice failed:%d", rc);
        rv = rc;
    }

    LOG_ALWAYS("Logout userId("FMTu64")", userId);
    rc = logout(userId);
    if (rc != 0) {
        LOG_ERROR("logout failed:%d", rc);
        rv = rc;
        goto exit;
    }

 exit:
    return rv;
}

int check_metadata_generic(int argc, const char* argv[], bool using_remote_agent)
{
    int rv = 0;
    std::string test_clip_file;
    std::string test_thumb_name_path;
    std::string test_clip_name;
    u64 deviceId;
    u64 userId;
    bool useLongPath = false;
    bool incrementTimestamp = false;
    bool useMediaDir = false;
    bool useThumbnailDir = false;
    bool addOnly = false;
    std::string mediaDir;
    std::string thumbnailDir;
    u32 opt_num_music_albums = 3;
    u32 opt_num_tracks_per_music_album = 10;
    u32 opt_num_photo_albums = 2;
    u32 opt_num_photos_per_photo_album = 5;
    int currArg = 1;
#if defined(CLOUDNODE)
    u64 datasetVDId = 0;
#endif

    if(!checkHelp(argc, argv)) {
        if(argc > currArg && (strcmp(argv[currArg], "-L")==0)) {
            useLongPath = true;
            currArg++;
        }

        if(argc > currArg && (strcmp(argv[currArg], "-T")==0)) {
            incrementTimestamp = true;
            currArg++;
        }

        if(argc > currArg+1 && (strcmp(argv[currArg], "-M")==0)) {
            useMediaDir = true;
            mediaDir = argv[currArg + 1];
            currArg += 2;
        }

        if(argc > currArg+1 && (strcmp(argv[currArg], "-N")==0)) {
            useThumbnailDir = true;
            thumbnailDir = argv[currArg + 1];
            currArg += 2;
        }

        if(argc > currArg && (strcmp(argv[currArg], "-A")==0)) {
            addOnly = true;
            currArg++;
        }

        if(argc == currArg+4) {
            opt_num_music_albums = static_cast<u32>(atoi(argv[currArg++]));
            opt_num_tracks_per_music_album = static_cast<u32>(atoi(argv[currArg++]));
            opt_num_photo_albums = static_cast<u32>(atoi(argv[currArg++]));
            opt_num_photos_per_photo_album = static_cast<u32>(atoi(argv[currArg++]));
        }
    }

    if(argc != currArg || checkHelp(argc, argv)) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
        }
        printf("%s [-L (long pathname)] [-T (increment Timestamp)] "
               "[-M {mediaDir}] [-N {thumbnailDir}] [-A (addOperationOnly)] "
               "[NumMusicAlbums NumTracksPerAlbum NumPhotoAlbums NumPhotosPerAlbum]  "
               "(default %d %d %d %d)\n",
               argv[0],
               opt_num_music_albums,
               opt_num_tracks_per_music_album,
               opt_num_photo_albums,
               opt_num_photos_per_photo_album);
        return 0;
    }

    LOG_ALWAYS("Executing... CheckMetadata %s%s%d %d %d %d",
               useLongPath?"LongPath ":"",
               incrementTimestamp?"IncrementTimestamp ":"",
               opt_num_music_albums,
               opt_num_tracks_per_music_album,
               opt_num_photo_albums,
               opt_num_photos_per_photo_album);

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        goto exit;
    }

    rv = getDeviceId(&deviceId);
    if (rv != 0) {
        LOG_ERROR("Fail to get device id");
        goto exit;
    }

    test_clip_name.assign(METADATA_PHOTO_TEST_CLIP_FILE);

    setDebugLevel(LOG_LEVEL_INFO);

    if (isCloudpc(userId, deviceId)) {
        std::string curDir;

        // FIXME: following code will only work against local ccd

        // MSA path
        rv = getCurDir(curDir);
        if (rv < 0) {
            goto exit;
        }

#if defined(CLOUDNODE)
        rv = getDatasetId(userId, "Virt Drive", datasetVDId);
        if (rv != 0) {
            LOG_ERROR("Fail to get dataset id");
            goto exit;
        }
#endif

        if (useMediaDir) {
            test_clip_file.assign(mediaDir.c_str());
        } else {
            test_clip_file.assign(curDir.c_str());
#if defined(CLOUDNODE)
            std::ostringstream datasetVDPrefix;
            datasetVDPrefix << "/dataset/" << userId << "/" << datasetVDId << "/";
            
            test_clip_file.assign(datasetVDPrefix.str());
#endif
        }
        test_clip_file.append("\\");
        test_clip_file.append(METADATA_PHOTO_TEST_CLIP_FILE);

        // For cloudnode the clip is uploaded in SetupCheckMetadata

        if (useThumbnailDir) {
            test_thumb_name_path.assign(thumbnailDir.c_str());
        }
#if defined(CLOUDNODE)
        else {
            test_thumb_name_path.assign("/native");
        }

        TargetDevice *target = getTargetDevice();
        std::string workDir;
        rv = target->getWorkDir(workDir);
        if (rv != 0) {
            LOG_ERROR("Failed to get workdir on target device: %d", rv);
            goto exit;
        }
        {
            std::string tempSrcFile = curDir+"/"+METADATA_PHOTO_TEST_THUMB_FILE;
            std::string tempDstFile = workDir+"/"+METADATA_PHOTO_TEST_THUMB_FILE;
            rv = target->pushFile(tempSrcFile, tempDstFile);
            if (rv != 0) {
                LOG_ERROR("Failed to pushFile(%s->%s):%d",
                          tempSrcFile.c_str(), tempDstFile.c_str(), rv);
                goto exit;
            }
            test_thumb_name_path.append(tempDstFile);
        }
#else
        test_thumb_name_path.append(curDir.c_str());
        test_thumb_name_path.append("\\"); 
        test_thumb_name_path.append(METADATA_PHOTO_TEST_THUMB_FILE);
#endif

        LOG_ALWAYS("Cloud PC detected");

        LOG_ALWAYS("Perform MSA metadata commit");
        rv = msa_diag(test_clip_file,
                      test_clip_name,
                      test_thumb_name_path,
                      useLongPath,
                      incrementTimestamp,
                      addOnly,
                      opt_num_music_albums,
                      opt_num_tracks_per_music_album,
                      opt_num_photo_albums,
                      opt_num_photos_per_photo_album,
                      using_remote_agent);
        if (rv != 0) {
            LOG_ERROR("MSA test fail");
            goto exit;
        }
    } else {
        std::string test_url;
        std::string test_thumb_url;
        LOG_ALWAYS("Client PC detected");
        LOG_ALWAYS("Perform MCA metadata get");
        rv = mca_diag(test_clip_name, test_url, test_thumb_url, false, true, using_remote_agent);
        if (rv != 0) {
            LOG_ERROR("mca_diag fail");
            goto exit;
        }
    }

    resetDebugLevel();
exit:
    return rv;
}

int check_metadata(int argc, const char* argv[])
{
    return check_metadata_generic(argc, argv, false);
}

int check_metadata2(int argc, const char* argv[])
{
    return check_metadata_generic(argc, argv, true);
}

int setup_check_metadata(int argc, const char* argv[])
{
    int rv = 0;
    u64 deviceId;
    u64 userId;
    std::string mediaDir;
    std::string thumbnailDir;
    int currArg = 1;
#if defined(CLOUDNODE)
    u64 datasetVDId = 0;
#endif

    if(argc != currArg || checkHelp(argc, argv)) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
        }
        printf("%s\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing... SetupCheckMetadata");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        goto exit;
    }

    rv = getDeviceId(&deviceId);
    if (rv != 0) {
        LOG_ERROR("Fail to get device id");
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    if (!isCloudpc(userId, deviceId)) {
        std::string curDir;

        LOG_ALWAYS("Client PC detected");

        // MSA path
        rv = getCurDir(curDir);
        if (rv < 0) {
            goto exit;
        }

#if defined(CLOUDNODE)
        rv = getDatasetId(userId, "Virt Drive", datasetVDId);
        if (rv != 0) {
            LOG_ERROR("Fail to get dataset id");
            goto exit;
        }

        {
            std::string resString = "";
            std::stringstream st;
            st << datasetVDId;
            std::string remotePath;
            remotePath.append("/");
            remotePath.append(METADATA_PHOTO_TEST_CLIP_FILE);
            LOG_ALWAYS("dataset id: %s", st.str().c_str());

            //just try to remove image from dataset
            fs_test_deletefile(userId, st.str(),
                                remotePath,
                                resString);
            
            rv = fs_test_upload(userId, st.str(),
                                 remotePath, curDir + "/" + METADATA_PHOTO_TEST_CLIP_FILE,
                                 resString);
            if (rv != 0) {
                LOG_ERROR("Failed to import file "\
                          METADATA_PHOTO_TEST_CLIP_FILE\
                          " into dataset Virt Drive as component "\
                          METADATA_PHOTO_TEST_CLIP_FILE\
                          ": %d", rv);
                goto exit;
            }
        }
#endif
        rv = 0;
    } else {
        std::string test_url;
        std::string test_thumb_url;
        LOG_ALWAYS("Cloud PC detected");

        // nothing to do in this case.
        rv = 0;
    }

    resetDebugLevel();
exit:
    return rv;
}

int msa_delete_catalog(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || argc != 1) {
        printf("%s - Deletes the cloudPC catalog\n",
               argv[0]);
        return 0;
    }
    int rv;

    rv = msaDiagDeleteCatalog();
    if(rv != 0) {
        LOG_ERROR("msaDiagDeleteCatalog:%d", rv);
    }
    return rv;
}

int download_updates(int argc, const char* argv[])
{
    int rv = 0;
    std::string opt_guidlist;
    std::string opt_outdir;
    std::string opt_appver;
    bool opt_checkonly = false;
    bool opt_polled = false;
    bool opt_android = false;
    bool opt_refresh = false;
    bool opt_testapi = false;
    bool opt_saveoutput = false;
    TargetDevice *target = NULL;
    std::string workDir;
    std::string separator;

    if (checkHelp(argc, argv)) {
        std::cout << "DownloadUpdates [-A] [-a] [-g <GUIDs>] [-C] [-a] [-o <outdir>] [-p] [-r] [-T] [-s]" << std::endl;
        std::cout << "  -A version to use for app" << std::endl;
        std::cout << "  -a use Acer Android GUIDs instead of Windows" << std::endl;
        std::cout << "  -C check only, do not download" << std::endl;
        std::cout << "  -g <GUIDS> indicates a list of GUIDs to download/check" << std::endl;
        std::cout << "  -o <outdir> output directory." << std::endl;
        std::cout << "  -p polled download (default is event driven)" << std::endl;
        std::cout << "  -r refresh versions in CCD cache" << std::endl;
        std::cout << "  -s save output from the download" << std::endl;
        std::cout << "  -T test SW Update API" << std::endl;
        return 0;
    }

    target = getTargetDevice();
    rv = target->getWorkDir(workDir);
    if(rv != 0) {
        LOG_ERROR("getWorkDir failed!");
        return rv;
    }
    rv = target->getDirectorySeparator(separator);
    if(rv != 0) {
        LOG_ERROR("getDirectorySeparator failed!");
        goto exit;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'A': 
                if (i + 1 < argc) {
                    opt_appver = argv[i+1];
                    i++;
                }
                break;
            case 'a':
                opt_android = true;
                break;
            case 'C':
                opt_checkonly = true;
                break;
            case 'g':
                if (i + 1 < argc) {
                    opt_guidlist.assign(argv[i + 1]);
                    i++;
                }
                break;
            case 'o':
                if (i + 1 < argc) {
                    opt_outdir.assign(argv[i + 1]);
                    i++;
                }
                opt_saveoutput = true;
                break;
            case 'p':
                opt_polled = true;
                break;
            case 'r':
                opt_refresh = true;
                break;
            case 's':
                opt_saveoutput = true;
                break;
            case 'T':
                opt_testapi = true;
                break;
            default:
                break;
            }
        }
    }
    // remove the contents of the output directory (can be dangerous)
    opt_outdir = workDir.append(separator).append("swupdate").append(separator);
    if (opt_testapi) {
        LOG_ALWAYS("opt_outdir: %s", opt_outdir.c_str());
        rv = swup_diag(opt_android ? 1 : 0, opt_outdir);
    } else {
        rv = swup_fetch_all(opt_guidlist, opt_outdir, opt_appver, opt_checkonly,
            opt_polled, opt_android, opt_refresh);
    }
    if ( !opt_saveoutput ) {
        int rc = -1;
        std::string clean_path = Util_CleanupPath(opt_outdir);
        rc = target->removeDirRecursive(clean_path);
        if (rc != 0) {
            LOG_ERROR("removeDirRecursive failed: %d! clean_path: %s", rc, clean_path.c_str());
            rv = rc;
        }
    }

exit:
    if (target != NULL)
        delete target;
    return rv;
}

int dump_events(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        std::cout << "DumpEvents [-m maxCount] [-t timeout]" << std::endl;
        std::cout << "  maxCount = Max number of events to fetch per attempt. Default is 0 = unlimited." << std::endl;
        std::cout << "  timeout  = If the event queue is empty, wait at most this amount of milliseconds. Default is 0 = don't wait." << std::endl;
        return 0;
    }

    u32 maxCount = 0;  // unlimited
    s32 timeout = 0;   // never wait for events

    setDebugLevel(LOG_LEVEL_INFO);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'm':
                if (i + 1 < argc) {
                    maxCount = atoi(argv[++i]);
                }
                break;
            case 't':
                if (i + 1 < argc) {
                    timeout = atoi(argv[++i]);
                }
                break;
            default:
                LOG_ERROR("Unknown option %s", argv[i]);
                return -1;
            }
        } else {
            LOG_ERROR("Unknown option %s", argv[i]);
            return -1;
        }
    }

    EventQueue eq;
    while (1) {
        int rc = eq.dump(maxCount, timeout);
        if(rc != 0)
        {   // When something goes wrong, let's not log a crazy amount.
            LOG_ERROR("Dump queue failed:%d, sleep 1 second", rc);
            VPLThread_Sleep(VPLTime_FromMillisec(1000));
        }
    }

    resetDebugLevel();

    return rv;
}

int time_stream_download(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        std::cout << "TimeStreamDownload [-d dumpfile] [-r repeat] [-m maxbytes[,delay]] [-D delay] [-t threads] [-R [width],[height]] [-f format] [-o outputdir]" << std::endl;
        std::cout << "  dumpfile = File containing streamable object URLs. Default is \"dumpfile\"." << std::endl;
        std::cout << "  repeat   = Number of times each file will be downloaded. Default is 1." << std::endl;
        std::cout << "  maxbytes = Number of bytes before download is interrupted. Default is unlimited." << std::endl;
        std::cout << "  delay    = Number of milliseconds to wait between files. Default is 0." << std::endl;
        std::cout << "  threads  = Number of threads that would request files. Default is 1." << std::endl;
        std::cout << "  resolution = (Only for photo). The transcoding resolution of target. Default is NULL." << std::endl;
        std::cout << "  format   = (Only for photo). The transcoding format of target. Default is NULL." << std::endl;
        std::cout << "  outputdir= (Only for photo). The output dir. Default is NULL." << std::endl;
        return 0;
    }

    std::string dumpfile = "dumpfile";  // default is to look for "dumpfile" in the current directory
    int repeat = 1;                     // default is to download the files once
    std::vector<std::string> dumpfiles; // default is to look for "dumpfile" in the current directory
    std::vector<int> maxbytes;          // default is to allow unlimited download (indicated by 0)
    std::vector<VPLTime_t> maxbytes_delay; // default is no wait after reaching maxbytes
    std::vector<VPLTime_t> delay;          // default is no wait between files.
    int nthreads = 1;                      // default is 1 thread.
    std::string dimension;
    unsigned int width = 0;
    unsigned int height = 0;
    std::string format;
    std::string outputDir;

    setDebugLevel(LOG_LEVEL_INFO);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'd':
                if (i + 1 < argc) {
                    size_t commaPos;
                    size_t curPos = 0;
                    std::string strDumpfiles;
                    strDumpfiles.assign(argv[++i]);
                    while ((commaPos = strDumpfiles.find(',', curPos)) != std::string::npos) {
                        dumpfile.assign(strDumpfiles, curPos, commaPos - curPos);
                        dumpfiles.push_back(dumpfile);
                        curPos = commaPos+1;
                    }
                    if(curPos == 0) {
                        dumpfile.assign(strDumpfiles.c_str());
                        dumpfiles.push_back(dumpfile);
                    }
                    else {
                        dumpfile.assign(strDumpfiles.c_str()+curPos+1);
                        dumpfiles.push_back(dumpfile);
                    }
                }
                break;
            case 'r':
                if (i + 1 < argc) {
                    repeat = atoi(argv[++i]);
                }
                break;
            case 'm':
                if (i + 1 < argc) {
                    bool ismaxbytes = false;
                    i++;
                    maxbytes.push_back(atoi(argv[i]));
                    const char *comma = strchr(argv[i], ',');
                    while (comma != NULL) {
                        if(!ismaxbytes) {
                            maxbytes_delay.push_back(VPLTime_FromMillisec(atoi(comma + 1)));
                        }
                        else {
                            maxbytes.push_back(atoi(comma + 1));
                        }
                        ismaxbytes = !ismaxbytes;
                        comma = strchr(comma+1, ',');
                    }
                }
                break;
            case 'D':
                if (i + 1 < argc) {
                    delay.push_back(VPLTime_FromMillisec(atoi(argv[++i])));
                    const char *comma = strchr(argv[i], ',');
                    while (comma != NULL) {
                        delay.push_back(VPLTime_FromMillisec(atoi(comma+1)));
                        comma = strchr(comma+1, ',');
                    }
                }
                break;
            case 't':
                if (i + 1 < argc) {
                    nthreads = atoi(argv[++i]);
                }
                break;
            case 'R':
                dimension.assign(argv[++i]);
                if (dimension.find(',') != std::string::npos) {
                    int pos = dimension.find(',');
                    std::string str_width = dimension.substr(0, pos);
                    std::string str_height = dimension.substr(pos+1, dimension.size());
                    width = atoi(str_width.c_str());
                    height = atoi(str_height.c_str());
                }
                break;
            case 'f':
                format.assign(argv[++i]);
                break;
            case 'o':
                outputDir.assign(argv[++i]);
                break;
            default:
                LOG_WARN("Ignoring unknown option %s", argv[i]);
            }
        }
    }

    while(dumpfiles.size() != 1 && static_cast<int>(dumpfiles.size()) < nthreads) {
        dumpfiles.push_back(dumpfile);
    }

    while(static_cast<int>(maxbytes.size()) < nthreads) {
        maxbytes.push_back(0);
    }

    while(static_cast<int>(maxbytes_delay.size()) < nthreads) {
        maxbytes_delay.push_back(0);
    }

    while(static_cast<int>(delay.size()) < nthreads) {
        delay.push_back(0);
    }

    u64 userId = 0;
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get user ID");
        return rv;
    }

    u64 deviceId = 0;
    rv = getDeviceId(&deviceId);
    if (rv != 0) {
        LOG_ERROR("Failed to get device ID");
        return rv;
    }

    TimeStreamDownload tsd;

    // Wake cloud PC if in standby
    rv = wakeSleepingDevices(userId);
    if (rv != 0) goto end;

    rv = waitCloudPCOnline(); 
    if (rv != 0) goto end;

    if (width > 0 && height > 0) {
        tsd.setTranscodingResolution(width, height);
    }

    if (format.size() > 0) {
        tsd.setTranscodingFormat(format);
    }

    if (outputDir.size() > 0) {
        tsd.setTranscodingOutputDir(outputDir);
    }

    rv = tsd.downloadAllFiles(dumpfiles, repeat, maxbytes, maxbytes_delay, delay, nthreads);
    resetDebugLevel();

 end:
    return rv;
}

// Returns true if the file contents are equal, else false.
bool isEqualFileContents(const char* lhsPath, const char* rhsPath)
{
    bool rv = false;
    const int bufSize = 1024;
    unsigned char* lhsBuf = NULL;
    unsigned char* rhsBuf = NULL;
    VPLFile_handle_t lhsHandle = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t rhsHandle = VPLFILE_INVALID_HANDLE;
    int bytesCompared = 0;
    int lhsBytesRead = 0;
    int rhsBytesRead = 0;

    lhsBuf = (unsigned char*) malloc(bufSize);
    rhsBuf = (unsigned char*) malloc(bufSize);
    if(lhsBuf == NULL || rhsBuf == NULL) {
        LOG_ERROR("Out of memory");
        goto end;
    }

    lhsHandle = VPLFile_Open(lhsPath,
                             VPLFILE_OPENFLAG_READONLY,
                             0777);
    if(!VPLFile_IsValidHandle(lhsHandle)) {
        LOG_ERROR("Cannot open %s:%d", lhsPath, lhsHandle);
        goto end;
    }

    rhsHandle = VPLFile_Open(rhsPath,
                             VPLFILE_OPENFLAG_READONLY,
                             0777);
    if(!VPLFile_IsValidHandle(rhsHandle)) {
        LOG_ERROR("Cannot open %s:%d", rhsPath, rhsHandle);
        goto end;
    }

    do {
        lhsBytesRead = VPLFile_Read(lhsHandle, lhsBuf, bufSize);
        if(lhsBytesRead < 0) {
            LOG_ERROR("Error reading %s,%d", lhsPath, lhsBytesRead);
            goto end;
        }

        rhsBytesRead = VPLFile_Read(rhsHandle, rhsBuf, bufSize);
        if(rhsBytesRead < 0) {
            LOG_ERROR("Error reading %s,%d", rhsPath, rhsBytesRead);
            goto end;
        }

        if(lhsBytesRead != rhsBytesRead) {
            LOG_ERROR("Different number of bytes read lhs %d, rhs %d.",
                      lhsBytesRead, rhsBytesRead);
            goto end;
        }

        if(lhsBytesRead > 0 &&
           memcmp(lhsBuf, rhsBuf, lhsBytesRead) != 0) {
            LOG_ERROR("Files compared different within %d bytes after %d byte offset",
                      bufSize, bytesCompared);
            goto end;
        }
        bytesCompared += lhsBytesRead;

    }while(lhsBytesRead > 0);

    rv = true;

 end:
    if(lhsBuf) {
        free(lhsBuf);
    }
    if(rhsBuf) {
        free(rhsBuf);
    }
    if(VPLFile_IsValidHandle(lhsHandle)) {
        VPLFile_Close(lhsHandle);
    }
    if(VPLFile_IsValidHandle(rhsHandle)) {
        VPLFile_Close(rhsHandle);
    }
    return rv;
}

int check_streaming_generic(int argc, const char* argv[], bool using_remote_agent)
{
    int rv = 0;
    std::string test_clip_name;
    std::string test_url;
    std::string test_thumb_url;
    HttpAgent *agent = NULL;
    int retry = 0;
    bool doHelp = false;
    bool includeMultiRangeTest = false;
    bool offlineTest = false;

    if(argc == 2) {
        if(strcmp(argv[1], "-m")==0) {
            includeMultiRangeTest = true;
        }else if(strcmp(argv[1], "-o")==0) {
            offlineTest = true;
        }else if(!checkHelp(argc, argv)){
            LOG_ERROR("Bad arg %s", argv[1]);
            doHelp = true;
        }
    }else if (argc == 3) {
        if(strcmp(argv[2], "-m")==0) {
            includeMultiRangeTest = true;
        }else if(strcmp(argv[2], "-o")==0) {
            offlineTest = true;
        }else if(!checkHelp(argc, argv)){
            LOG_ERROR("Bad arg %s", argv[1]);
            doHelp = true;
        }
    }else if (argc != 1){
        LOG_ERROR("Bad numArgs %d", argc);
        doHelp = true;
    }

    if (doHelp || checkHelp(argc, argv)) {
        printf("%s [-m (include multirange test)] [-o (offlineTest)]\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    agent = getHttpAgent();
    if (!agent) { 
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);
    test_clip_name.assign(STREAMING_TEST_CLIP_NAME);

    // FIXME: this will only work against local ccd
    test_url = test_thumb_url = "";
get_metadata:
    if (offlineTest) {
        LOG_ALWAYS("Running offline..");
        rv = mca_diag(test_clip_name, test_url, test_thumb_url, false, false, using_remote_agent);
    } else {
        rv = mca_diag(test_clip_name, test_url, test_thumb_url, true, true, using_remote_agent);
    }
    if (rv != 0) {
        LOG_ERROR("mca_diag fail");
        goto exit;
    } else if ((test_url == "") || (test_thumb_url == "")){
        LOG_ALWAYS("URL for streaming test not ready yet. Waiting..");
        LOG_ALWAYS("Verify \"dxshell CheckMetadata\" was run on the CloudPC?  "
                   "(required step for CheckStreaming on client)");
        if (retry++ > 120) {
            LOG_ERROR("Fail to get URL for streaming test");
            rv = -1;
            goto exit;
        } else {
            VPLThread_Sleep(VPLTIME_FROM_SEC(1));
            goto get_metadata;
        }
    } else {
        LOG_ALWAYS("========== Streaming: %s =========", test_url.c_str());
        LOG_ALWAYS("Begin [Streaming Content]");
        rv = agent->get(test_url, 0, 0);
        if (rv != 0) {
            LOG_ERROR("[Streaming Content] FAIL %d. (Verify if CloudPC is running)", rv);
            rv = -1;
            goto exit;
        } else {
            LOG_ALWAYS("[Streaming Content] PASS");
        }

        LOG_ALWAYS("Begin [Streaming Thumbnail]");
        if (test_thumb_url.length()) {
            rv = agent->get(test_thumb_url, 0, 0);
            if (rv != 0) {
                LOG_ERROR("[Streaming Thumbnail] FAIL %d. (Verify if CloudPC is running)", rv);
                rv = -1;
                goto exit;
            } else {
                LOG_ALWAYS("[Streaming Thumbnail] PASS");
            }
        } else {
            rv = -1;
            LOG_ERROR("!!!Thumbnail URL not found!!!");
            goto exit;
        }

        if(includeMultiRangeTest) {
            std::ostringstream filename;
            filename << "./checkStreamMultirangeResult_" << testInstanceNum << ".out";
            std::string filenameStr = filename.str();
            const char* header[1] = {"Range: bytes=100-249,5000-5749,6000-6500"};
            LOG_ALWAYS("Begin [Stream Content Multi-Range]");
            rv = agent->get_extended(test_url, 0, 0,
                                     header, 1,
                                     filenameStr.c_str());
            if (rv != 0) {
                LOG_ERROR("Content Streaming FAIL %d. (Verify if CloudPC is running)", rv);
                rv = -1;
                goto exit;
            }else{
                const char* refResponse = "./checkStreamMultirangeResult_ref.out";
                if(!isEqualFileContents(filenameStr.c_str(), refResponse)) {
                    LOG_ERROR("[Stream Content Multi-Range] FAIL, "
                              "response %s is different from reference response %s",
                              filenameStr.c_str(), refResponse);
                    rv = -1;
                    goto exit;
                }
                LOG_ALWAYS("[Stream Content Multi-Range] PASS");
            }
        }
    }

exit:
    delete agent;
    resetDebugLevel();
    return rv;
}

int check_streaming(int argc, const char* argv[])
{
    return check_streaming_generic(argc, argv, /*using_remote_agent*/false);
}

int check_streaming2(int argc, const char* argv[])
{
    return check_streaming_generic(argc, argv, /*using_remote_agent*/true);
}

#if 0
static int upload_logs(int argc, char* argv[])
{
    int rv = 0;
    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }
    LOG_ALWAYS("Executing..");
    return rv;
}
#endif

int list_devices(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Executing..");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);
    rv = listLinkedDevice(userId);
    resetDebugLevel();
    if (rv != 0) {
        LOG_ERROR("List devices fail");
        goto exit;
    }
exit:
    return rv;
}

int list_user_storage(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    bool cache_only = false;

    if (checkHelp(argc, argv)) {
        printf("%s [-c]\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Executing..");

    for( int i = 1 ; i < argc ; i++) {
        if ( strncmp(argv[i], "-c", 2) != 0 ) {
            LOG_ERROR("Invalid argument %s", argv[i]);
            return -1;
        }
        cache_only = true;
    }

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get user id");
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);
    rv = listUserStorage(userId, cache_only);
    resetDebugLevel();
    if (rv != 0) {
        LOG_ERROR("List UserStorage failed: %d", rv);
        goto exit;
    }
exit:
    return rv;
}

int wake_sleeping_devices(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   // No arguments needed
    }

    LOG_ALWAYS("Executing..");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);
    rv = wakeSleepingDevices(userId);
    if (rv != 0) {
        LOG_ERROR("Fail to wakeup sleeping devices rv = %d", rv);
        goto exit;
    }
exit:
    resetDebugLevel();
    return rv;
}

static int print_menu()
{
    int rv = 0;
    std::map<std::string, subcmd_fn>::iterator it;

    printf("Run \""EXE_NAME" Help\" to list all subcommands.\n"
           "Run \""EXE_NAME" <command> Help\" to list subcommands for \"command\".\n"
           "[Supported Top-level Commands]\n");
    for (it = dx_cmds.begin(); it != dx_cmds.end(); ++it) {
        const char* curr = it->first.c_str();
        const char* notes = "";
        if (getenv(DX_REMOTE_IP_ENV) != NULL) {
            bool disabled = (dx_not_remote_cmds.find(curr) != dx_not_remote_cmds.end());
            if (disabled) {
                if (dx_not_remote_cmds[curr].empty()) {
                    notes = " (disabled for "DX_REMOTE_IP_ENV")";
                } else {
                    notes = " (some subcommands disabled for "DX_REMOTE_IP_ENV")";
                }
            }
        }
        printf("%s%s\n", curr, notes);
    }

    return rv;
}

static int print_help()
{
    printf("[Supported Commands]\n");
    printHelpFromCmdMap(dx_cmds);
    return 0;
}

static int dispatch(int argc, const char* argv[])
{
    return dispatch(dx_cmds, argc, argv);
}

int mca_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_mca_cmds, argc, argv);
}

int photo_share_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_photo_share_cmds, argc, argv);
}

int picstream_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_picstream_cmds, argc, argv);
}

int vcs_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_vcs_cmds, argc, argv);
}

int vcspic_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_vcspic_cmds, argc, argv);
}

int cloudmedia_commands(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_cloudmedia_cmds, argc, argv);
}

#if defined(CLOUDNODE)
static int mkdir_implicit(u64 userId,
                          u64 datasetId,
                          const std::string &remotePath)
{
    // simple way to implement mkdir -p
    std::string response = "";
    int rc = 0;
    std::stringstream st;
    st << datasetId;
    std::string datasetIdString = st.str();

    std::string path(remotePath);
    std::string name = "";
    std::stack<std::string> dirs;
     while(path.compare("") &&
           path.compare("/")) {
        std::string tmp = "";
        splitAbsPath(path, tmp, name);
        if(name.compare("") && 
           name.compare("/") &&
           name.compare(".") &&
           name.compare("..")) {
            dirs.push(tmp + "/" + name);
        }
        path = tmp;
    }

    while(!dirs.empty()) {
        std::string dirPath = "";
        dirPath = dirs.top();
        dirs.pop();
        LOG_INFO("MKDIR remote: %s", dirPath.c_str());
        rc = fs_test_makedir(userId, datasetIdString, dirPath, response);
        if (rc != 0) {
            LOG_WARN("Upload might failed.");
        }
    }
    return rc;
}

struct RemoteFileAsyncTransferStatus {
    enum RemoteFileAsyncTransferStatusEnum {
        STATUS_NONE,
        STATUS_WAIT,
        STATUS_ACTIVE,
        STATUS_DONE,
        STATUS_ERROR
    };
    u64 id;
    RemoteFileAsyncTransferStatusEnum status;
    u64 totalSize;
    u64 xferedSize;

    void clear() {
        id = 0;
        status = STATUS_NONE;
        totalSize = 0;
        xferedSize = 0;
    }

    RemoteFileAsyncTransferStatus() { clear(); }
};

// http://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Dataset_Access#check_async_transfer_request_status
static int parseAsyncTransferStatus(cJSON2* asyncTransferObject,
                                    RemoteFileAsyncTransferStatus& asyncTransferStatus)
{
    asyncTransferStatus.clear();

    if(asyncTransferObject == NULL) {
        LOG_ERROR("Null object");
        return -1;
    }
    if(asyncTransferObject->type != cJSON2_Object) {
        LOG_ERROR("Not an object type:%d", asyncTransferObject->type);
        return -1;
    }

    cJSON2* entryAttrib = asyncTransferObject->child;

    while(entryAttrib != NULL) {
        if(entryAttrib->type == cJSON2_String) {
            if(strcmp("status", entryAttrib->string)==0) {
                std::string statusStr = entryAttrib->valuestring;
                if(statusStr == "wait") {
                    asyncTransferStatus.status = RemoteFileAsyncTransferStatus::STATUS_WAIT;
                }else if(statusStr == "active") {
                    asyncTransferStatus.status = RemoteFileAsyncTransferStatus::STATUS_ACTIVE;
                }else if(statusStr == "done") {
                    asyncTransferStatus.status = RemoteFileAsyncTransferStatus::STATUS_DONE;
                }else if(statusStr == "error") {
                    asyncTransferStatus.status = RemoteFileAsyncTransferStatus::STATUS_ERROR;
                }else {
                    LOG_ERROR("Unrecognized status:%s", statusStr.c_str());
                }
            }
        }else if(entryAttrib->type == cJSON2_Number) {
            if(strcmp("id", entryAttrib->string)==0) {
                asyncTransferStatus.id = entryAttrib->valueint;
            }else if(strcmp("totalSize", entryAttrib->string)==0) {
                asyncTransferStatus.totalSize = entryAttrib->valueint;
            }else if(strcmp("xferedSize", entryAttrib->string)==0) {
                asyncTransferStatus.xferedSize = entryAttrib->valueint;
            }
        }
        entryAttrib = entryAttrib->next;
    }

    return 0;
}

static const char* transferStatusToString(
        RemoteFileAsyncTransferStatus::RemoteFileAsyncTransferStatusEnum myEnum)
{
    switch(myEnum) {
    case RemoteFileAsyncTransferStatus::STATUS_NONE:
        return "none";
    case RemoteFileAsyncTransferStatus::STATUS_WAIT:
        return "wait";
    case RemoteFileAsyncTransferStatus::STATUS_ACTIVE:
        return "active";
    case RemoteFileAsyncTransferStatus::STATUS_ERROR:
        return "error";
    case RemoteFileAsyncTransferStatus::STATUS_DONE:
        return "done";
    default:
        LOG_ERROR("Unrecognized:%d", (int)myEnum);
        return "ERR-Unrecognized";
    }
}

static int recursive_upload(u64 userId,
                            u64 datasetId,
                            const std::string &remotePath,
                            const std::string &localPath) 
{
    std::string response = "";
    std::string trimedRemotePath = "";
    std::string trimedLocalPath = "";
    int rc = 0;
    std::stringstream st;
    st << datasetId;
    std::string datasetIdString = st.str();

    rmTrailingSlashes(remotePath, trimedRemotePath);
    rmTrailingSlashes(localPath, trimedLocalPath);

    mkdir_implicit(userId, datasetId, remotePath);

    std::string localBasePath = "";
    std::string localName = "";
    splitAbsPath(localPath, localBasePath, localName);
    size_t replaceIndex = localBasePath.length() + 1;

    LOG_INFO("localBasePath %d: %s", replaceIndex,localBasePath.c_str());
    std::queue<std::string> dirs;
    dirs.push(trimedLocalPath);

    // simple apply BFS
    while(!dirs.empty()) {

        VPLFS_dir_t dirFolder;
        std::string dirPath = dirs.front();
        dirs.pop();
        rc = VPLFS_Opendir(dirPath.c_str(), &dirFolder);

        std::string uploadPath(dirPath);
        uploadPath.replace(0, replaceIndex,
                           trimedRemotePath + "/");
        if(rc == VPL_OK) {
            LOG_INFO("MKDIR remote: %s", uploadPath.c_str());
            rc = fs_test_makedir(userId, datasetIdString, uploadPath, response);
            if (rc != 0) {
                LOG_WARN("Upload might failed.");
            }
            VPLFS_dirent_t dirent;
            while((rc = VPLFS_Readdir(&dirFolder, &dirent)) == VPL_OK) {
                std::string dirent_name = std::string(dirent.filename);
                if(dirent_name.compare(".") &&
                   dirent_name.compare("..") &&
                   dirent_name.compare(".sync_temp")) {
                    dirs.push(dirPath + "/" + dirent_name);
                }
            }
        } else {
            // or treat it as file, and upload it
            // Modified to use async upload (as opposed to synchronous upload) due to Bug 9743
            std::string async_handle;
            std::string cjson_response;
            LOG_INFO("FILE remote: %s, %s", uploadPath.c_str(), dirPath.c_str());
            rc = fs_test_async_upload(userId,
                                      datasetIdString,
                                      uploadPath,
                                      dirPath,
                                      async_handle);
            if (rc != 0) {
                LOG_ERROR("Upload failed(%s,%s):%d, handle:%s",
                         uploadPath.c_str(), dirPath.c_str(), rc, async_handle.c_str());
                goto exit;
            }
            RemoteFileAsyncTransferStatus asyncTransferStatus;
            u32 numTimes = 0;

            while(asyncTransferStatus.status != RemoteFileAsyncTransferStatus::STATUS_DONE &&
                  asyncTransferStatus.status != RemoteFileAsyncTransferStatus::STATUS_ERROR)
            {
                rc = fs_test_async_progress(userId,
                                            async_handle,
                                            cjson_response);
                if(rc != 0) {
                    LOG_ERROR("fs_test_async_progress:%d, handle:%s",
                              rc, async_handle.c_str());
                    goto exit;
                }

                cJSON2 *jsonResponse = cJSON2_Parse(cjson_response.c_str());
                if (jsonResponse == NULL) {
                    LOG_ERROR("Invalid root json data:%s", cjson_response.c_str());
                    rc = -1;
                    goto exit;
                }
                ON_BLOCK_EXIT(cJSON2_Delete, jsonResponse);
                parseAsyncTransferStatus(jsonResponse, asyncTransferStatus);

                if(asyncTransferStatus.status != RemoteFileAsyncTransferStatus::STATUS_DONE &&
                   asyncTransferStatus.status != RemoteFileAsyncTransferStatus::STATUS_ERROR)
                {
                    if((numTimes % 5) == 0) {
                        // Only print out every second.
                        LOG_INFO("asyncTransferStatus: id("FMTu64"), status(%s), "
                                 "xfer:"FMTu64"/"FMTu64"bytes",
                                 asyncTransferStatus.id,
                                 transferStatusToString(asyncTransferStatus.status),
                                 asyncTransferStatus.xferedSize,
                                 asyncTransferStatus.totalSize);
                    }
                    VPLThread_Sleep(VPLTime_FromMillisec(200));
                } else {
                    LOG_INFO("asyncTransferStatus: id("FMTu64"), status(%s), "
                             "xfer:"FMTu64"/"FMTu64"bytes, fileUploadComplete(%s)",
                             asyncTransferStatus.id,
                             transferStatusToString(asyncTransferStatus.status),
                             asyncTransferStatus.xferedSize,
                             asyncTransferStatus.totalSize,
                             dirPath.c_str());

                    if(asyncTransferStatus.status == RemoteFileAsyncTransferStatus::STATUS_ERROR){
                        rc = -1;
                        goto exit;
                    }
                }
                numTimes++;
            }
        }
    }

exit:
    return rc;
}
#endif

#if defined(CLOUDNODE)
static int cloudnode_cloudmedia_add_music(int argc, const char* argv[])
{
    int rv = 0;
    int rc;
    std::string absMusicPath;
    u64 userId;
    u64 deviceId;
    if (checkHelp(argc, argv) || argc != 2) {
        printf("%s %s <absoluteMusicPath>\n", CLOUDMEDIA_STR, argv[0]);
        return 0;   // No arguments needed
    }
    absMusicPath = argv[1];

    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        goto exit;
    }

    rc = getDeviceId(&deviceId);
    if (rc != 0) {
        LOG_ERROR("Fail to get device id:%d", rc);
        rv = rc;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    if (!isCloudpc(userId, deviceId)) {
        // FIXME: following code will only work against local ccd

        LOG_ALWAYS("Client PC detected");
        u64 datasetVDId = 0;
        
        rc = getDatasetId(userId, "Virt Drive", datasetVDId);
        if (rc != 0) {
            rv = rc;
            LOG_ERROR("Fail to get dataset id");
            goto exit;
        }

        std::string uploadBasePath = "";
        std::string name = "";
        splitAbsPath(absMusicPath, uploadBasePath, name);

        rc = recursive_upload(userId, datasetVDId, uploadBasePath, absMusicPath);
        if (rc != 0) {
            rv = rc;
            LOG_WARN("Upload might failed.");
        }

        LOG_ALWAYS("Add CloudnodeCloudMedia music success: %s", absMusicPath.c_str());
    } else {
        LOG_ERROR("Cloud PC detected.  This command only works on Cliet PC.");
        rv = -1;
    }

 exit:
    resetDebugLevel();
    return rv;
}
#endif // CLOUDNODE

static int cloudmedia_add_music(int argc, const char* argv[])
{
    int rv = 0;
    int rc;
    std::string absMusicPath;
    u64 userId;
    u64 deviceId;
    if (checkHelp(argc, argv) || argc != 2) {
        printf("%s %s <absoluteMusicPath>\n", CLOUDMEDIA_STR, argv[0]);
        return 0;   // No arguments needed
    }
    absMusicPath = argv[1];

    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        goto exit;
    }

    rc = getDeviceId(&deviceId);
    if (rc != 0) {
        LOG_ERROR("Fail to get device id:%d", rc);
        rv = rc;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    if (isCloudpc(userId, deviceId)) {
        // FIXME: following code will only work against local ccd

        LOG_ALWAYS("Cloud PC detected");
        rc = msa_add_cloud_music(absMusicPath);
        if (rc != 0) {
            LOG_ERROR("msa_add_cloud_music test fail:%d", rc);
            rv = rc;
            goto exit;
        }
        LOG_ALWAYS("Add CloudMedia music success: %s", absMusicPath.c_str());
    } else {
        LOG_ERROR("Client PC detected.  This command only works on Cloud PC.");
        rv = -1;
    }

 exit:
    resetDebugLevel();
    return rv;
}

static int cloudmedia_delete_object(int argc, const char* argv[])
{
    int rc;
    int rv = 0;
    u64 userId;
    u64 deviceId;
    int currArg = 1;
    std::string collectionId;
    std::string objectId;
    media_metadata::CatalogType_t catType = media_metadata::MM_CATALOG_MUSIC;
    if(!checkHelp(argc, argv)) {
        if(argc == currArg + 3) {
            std::string catTypeStr = argv[currArg];
            if(catTypeStr == "music") {
                catType = media_metadata::MM_CATALOG_MUSIC;
            }else if(catTypeStr == "photo") {
                catType = media_metadata::MM_CATALOG_PHOTO;
            }else if(catTypeStr == "video") {
                catType = media_metadata::MM_CATALOG_VIDEO;
            }else{
                LOG_ERROR("catalog Type:%s not recognized.", catTypeStr.c_str());
                currArg = 9999;
            }
            currArg++;
        }else{
            LOG_ERROR("No arguments");
            currArg = 9999;
        }

        if(argc == currArg + 2) {
            collectionId = argv[currArg++];
        }else{
            currArg = -99;
        }

        if(argc == currArg + 1) {
            objectId = argv[currArg++];
        }else{
            currArg = -99;
        }
    }

    if(argc != currArg || checkHelp(argc, argv)) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
        }
        printf("%s %s <catalogType:music,photo,video> <collectionId> <objectId>\n",
                CLOUDMEDIA_STR, argv[0]);
        return 0;
    }

    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        goto exit;
    }

    rc = getDeviceId(&deviceId);
    if (rc != 0) {
        LOG_ERROR("Fail to get device id:%d", rc);
        rv = rc;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    LOG_INFO("CMD Delete metadata object: catalogType(%d), collectionId(%s), objectId(%s)",
             (int)catType, collectionId.c_str(), objectId.c_str());

    if (isCloudpc(userId, deviceId)) {
        // FIXME: following code will only work against local ccd

        LOG_ALWAYS("Cloud PC detected");
        rc = msa_delete_object(catType,
                               collectionId,
                               objectId);
        if(rc != 0) {
            LOG_ERROR("msa_delete_object:%d", rc);
            rv = rc;
        }
    } else {
        LOG_ERROR("Client PC detected.  This command only works on Cloud PC.");
        rv = -1;
    }

 exit:
    resetDebugLevel();
    return rv;
}

static int cloudmedia_delete_collection(int argc, const char* argv[])
{
    int rc;
    int rv = 0;
    u64 userId;
    u64 deviceId;
    int currArg = 1;
    std::string collectionId;
    std::string objectId;
    media_metadata::CatalogType_t catType = media_metadata::MM_CATALOG_MUSIC;
    if(!checkHelp(argc, argv)) {
        if(argc == currArg + 2) {
            std::string catTypeStr = argv[currArg];
            if(catTypeStr == "music") {
                catType = media_metadata::MM_CATALOG_MUSIC;
            }else if(catTypeStr == "photo") {
                catType = media_metadata::MM_CATALOG_PHOTO;
            }else if(catTypeStr == "video") {
                catType = media_metadata::MM_CATALOG_VIDEO;
            }else{
                LOG_ERROR("catalog Type:%s not recognized.", catTypeStr.c_str());
                currArg = 9999;
                rv = -3;
            }
            currArg++;
        }else{
            LOG_ERROR("No arguments");
            currArg = 9999;
            rv = -4;
        }

        if(argc == currArg + 1) {
            collectionId = argv[currArg++];
        }else{
            currArg = 9999;
            rv = -5;
        }
    }

    if(argc != currArg || checkHelp(argc, argv)) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
        }
        printf("%s %s <music|photo|video> <collectionId>\n",
                CLOUDMEDIA_STR, argv[0]);
        return rv;
    }

    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        goto exit;
    }

    rc = getDeviceId(&deviceId);
    if (rc != 0) {
        LOG_ERROR("Fail to get device id:%d", rc);
        rv = rc;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    if (isCloudpc(userId, deviceId)) {
        // FIXME: following code will only work against local ccd

        LOG_ALWAYS("Cloud PC detected");
        rc = msa_delete_collection(catType,
                                   collectionId);
        if(rc != 0) {
            LOG_ERROR("msa_delete_object:%d", rc);
            rv = rc;
        }
    } else {
        LOG_ERROR("Client PC detected.  This command only works on Cloud PC.");
        rv = -1;
    }

 exit:
    resetDebugLevel();
    return rv;
}

static int cloudmedia_add_photo(int argc, const char* argv[])
{
    int rv = 0;
    int rc;
    std::string absPhotoPath;
    u64 userId;
    u64 deviceId;
    int currArg = 1;
    int intervalSec = -1;

    if(!checkHelp(argc, argv)) {
        if(argc > currArg && (strcmp(argv[currArg], "-t")==0)) {
            currArg++;
            if(argc > currArg) {
                intervalSec = atoi(argv[currArg++]);
            }else{
                currArg = -99;
            }
        }

        if(argc > currArg) {
            absPhotoPath = argv[currArg++];
        } else {
            currArg = -99;
        }
    }

    if(argc != currArg || checkHelp(argc, argv)) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
        }
        printf("%s %s [-t <seconds> (enables single photo commit every interval)] "
               "<absolutePhotoPath>\n",
               CLOUDMEDIA_STR,
               argv[0]);
        return 0;
    }

    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        goto exit;
    }

    rc = getDeviceId(&deviceId);
    if (rc != 0) {
        LOG_ERROR("Fail to get device id:%d", rc);
        rv = rc;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    if (isCloudpc(userId, deviceId)) {
        // FIXME: following code will only work against local ccd

        LOG_ALWAYS("Cloud PC detected");
        rc = msa_add_cloud_photo(absPhotoPath, intervalSec, "", "");
        if (rc != 0) {
            LOG_ERROR("msa_add_cloud_photo test fail:%d", rc);
            rv = rc;
            goto exit;
        }
        LOG_ALWAYS("Add CloudMedia photo success: %s", absPhotoPath.c_str());
    } else {
        LOG_ERROR("Client PC detected.  This command only works on Cloud PC.");
        rv = -1;
    }

 exit:
    resetDebugLevel();
    return rv;
}

static int cloudmedia_add_photo_to_album(int argc, const char* argv[])
{
    int rv = 0;
    int rc;
    std::string absPhotoPath;
    std::string albumName = "defaultAlbum";
    std::string thumbnailPath = "";
    u64 userId;
    u64 deviceId;

    if (checkHelp(argc, argv) || argc < 2) {
        printf("%s %s <absoluteMusicPath> [album_name] [path\\to\\thumbnail]\n", CLOUDMEDIA_STR, argv[0]);
        return 0;   // No arguments needed
    }
    absPhotoPath = argv[1];
    if (argc == 3) {
        albumName = argv[2];
    } else if (argc == 4) {
        albumName = argv[2];
        thumbnailPath = argv[3];
    }

    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        goto exit;
    }

    rc = getDeviceId(&deviceId);
    if (rc != 0) {
        LOG_ERROR("Fail to get device id:%d", rc);
        rv = rc;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    if (isCloudpc(userId, deviceId)) {
        // FIXME: following code will only work against local ccd

        LOG_ALWAYS("Cloud PC detected");
        rc = msa_add_cloud_photo(absPhotoPath, -1, albumName, thumbnailPath);
        if (rc != 0) {
            LOG_ERROR("msa_add_cloud_photo test fail:%d", rc);
            rv = rc;
            goto exit;
        }
        LOG_ALWAYS("Add CloudMedia photo success: %s", absPhotoPath.c_str());
    } else {
        LOG_ERROR("Client PC detected.  This command only works on Cloud PC.");
        rv = -1;
    }

 exit:
    resetDebugLevel();
    return rv;
}

#if defined(CLOUDNODE)
static int cloudnode_cloudmedia_add_photo(int argc, const char* argv[])
{
    int rv = 0;
    int rc;
    std::string absPhotoPath;
    u64 userId;
    u64 deviceId;
    int currArg = 1;

    if(!checkHelp(argc, argv)) {
        if(argc > currArg) {
            absPhotoPath = argv[currArg++];
        } else {
            currArg = -99;
        }
    }

    if(argc != currArg || checkHelp(argc, argv)) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
        }
        printf("%s %s  <absolutePhotoPath>\n", CLOUDMEDIA_STR, argv[0]);
        return 0;
    }

    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        goto exit;
    }

    rc = getDeviceId(&deviceId);
    if (rc != 0) {
        LOG_ERROR("Fail to get device id:%d", rc);
        rv = rc;
        goto exit;
    }

    setDebugLevel(LOG_LEVEL_INFO);

    if (!isCloudpc(userId, deviceId)) {
        // FIXME: following code will only work against local ccd

        LOG_ALWAYS("Client PC detected");
        u64 datasetVDId = 0;
        
        rv = getDatasetId(userId, "Virt Drive", datasetVDId);
        if (rv != 0) {
            LOG_ERROR("Fail to get dataset id");
            goto exit;
        }

        std::string uploadBasePath = "";
        std::string name = "";
        splitAbsPath(absPhotoPath, uploadBasePath, name);

        rc = recursive_upload(userId, datasetVDId, uploadBasePath, absPhotoPath);
        if (rv != 0) {
            LOG_WARN("Upload might failed.");
        }
        LOG_ALWAYS("Add CloudnodeCloudMedia photo success: %s", absPhotoPath.c_str());
    } else {
        LOG_ERROR("Cloud PC detected.  This command only works on Client PC.");
        rv = -1;
    }

 exit:
    resetDebugLevel();
    return rv;
}
#endif // CLOUDNODE

int enable_cloudpc(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    bool enable = false;

    if (checkHelp(argc, argv) || (argc != 2)) {
        printf("%s [T|F]\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Executing..");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        goto exit;
    }

    // Remove registerPsn/unregisterPsn, because both cloud and client PC are storage nodes.
    // So, we only need to enable/disable media server feature.

    if ((argv[1][0] == 'T') || (argv[1][0] == 't')) {
        enable = true;
    } else {
        enable = false;
    }

    {
        ///enable/disable media server update
        const char *arg[] = { "UpdatePsn", (enable) ? "-M" : "-m"};
        rv = update_psn(2, arg);
        if (rv != VPL_OK) {
            LOG_ERROR("Error while updating psn flag for mediaserver: flag = %s, rv = %d", arg[1], rv);
        }
    }

exit:
    return rv;
}

static int get_sys_state(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || (argc < 2)) {
        printf("%s all     Dump all basic status.\n", argv[0]);
        printf("%s ioac    Dump IOAC status.\n", argv[0]);
        printf("%s users   Dump status of logged-in and logged-out users.\n", argv[0]);
        return 0;
    }
    
    bool getAll = (strcmp(argv[1], "all") == 0);

    ccd::GetSystemStateInput gssInput;
    if (getAll) {
        gssInput.set_get_device_id(true);
        gssInput.set_get_network_info(true);
        gssInput.set_get_background_mode_interval_sec(true);
        gssInput.set_get_only_mobile_network_available(true);
        gssInput.set_get_stream_power_mode(true);
        gssInput.set_get_power_mode(true);
    }
    if (getAll || (strcmp(argv[1], "ioac") == 0)) {
        gssInput.set_get_ioac_status(true);
        gssInput.set_get_enable_ioac(true);
        gssInput.set_get_ioac_already_in_use(true);
    }
    if (getAll || (strcmp(argv[1], "users") == 0)) {
        gssInput.set_get_users(true);
        gssInput.set_get_logged_out_users(true);
    }
    LOG_ALWAYS("Calling CCDIGetSystemState");
    ccd::GetSystemStateOutput gssOutput;
    int rv = CCDIGetSystemState(gssInput, gssOutput);
    if (rv != 0) {
        LOG_ERROR("CCDIGetSystemState failed: %d", rv);
        goto exit;
    }
    LOG_ALWAYS("CCD System State:\n%s", gssOutput.DebugString().c_str());
exit:
    return rv;
}

static int get_sync_state(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s all        Dump all sync status.\n", argv[0]);
        printf("%s basic      Dump all basic sync status.\n", argv[0]);
        printf("%s mca        Dump all media metadata client (download) status.\n", argv[0]);
        printf("%s msa        Dump all media metadata server (upload) status.\n", argv[0]);
        printf("%s notes      Dump all Notes sync status.\n", argv[0]);
        printf("%s clouddoc   Dump all CloudDoc sync status.\n", argv[0]);
        printf("%s picstream  Dump all Picstream upload/download status.\n", argv[0]);
        return 0;
    }

    bool recognized = false;

    bool getAll = (strcmp(argv[1], "all") == 0);
    bool getBasic = (strcmp(argv[1], "basic") == 0);

    ccd::GetSyncStateInput gssInput;
    gssInput.set_only_use_cache(true);
    if (getAll || getBasic) {
        recognized = true;
        gssInput.set_get_device_name(true);
        gssInput.set_get_is_camera_roll_upload_enabled(true);
        gssInput.set_get_is_camera_roll_global_delete_enabled(true);
        gssInput.set_get_is_network_activity_enabled(true);
        gssInput.set_get_bandwidth_limits(true);
        gssInput.set_get_background_data(true);
        gssInput.set_get_auto_sync(true);
        gssInput.set_get_mobile_network_data(true);
    }
    if (getAll || (strcmp(argv[1], "mca") == 0)) {
        recognized = true;
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PHOTO_METADATA);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_MUSIC_METADATA);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_VIDEO_METADATA);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PLAYLISTS);
        gssInput.set_get_media_metadata_download_path(true);
        gssInput.set_get_media_playlist_path(true);
        gssInput.set_get_mm_thumb_download_path(true);
        gssInput.set_get_mm_thumb_sync_enabled(true);
    }
    if (getAll || (strcmp(argv[1], "picstream") == 0)) {
        recognized = true;
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_UPLOAD);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES);
        gssInput.set_get_camera_roll_upload_dirs(true);
        gssInput.set_get_camera_roll_download_dirs(true);
    }
    if (getAll || (strcmp(argv[1], "msa") == 0)) {
        recognized = true;
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD);
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD);
        gssInput.set_get_media_metadata_upload_path(true);
    }
    if (getAll || (strcmp(argv[1], "notes") == 0)) {
        recognized = true;
        gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_NOTES);
        gssInput.set_get_notes_sync_settings(true);
    }
    if (getAll || (strcmp(argv[1], "clouddoc") == 0)) {
        recognized = true;
        gssInput.set_get_clouddoc_sync(true);
    }
    if (!recognized) {
        printf("Unrecognized option \"%s\"\n", argv[1]);
        return 0;
    }
    LOG_ALWAYS("Calling CCDIGetSyncState");
    ccd::GetSyncStateOutput gssOutput;
    rv = CCDIGetSyncState(gssInput, gssOutput);
    if (rv != 0) {
        LOG_ERROR("CCDIGetSyncState failed: %d", rv);
        goto exit;
    }
    LOG_ALWAYS("CCD Sync State:\n%s", gssOutput.DebugString().c_str());
exit:
    return rv;
}

int http_get(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        std::cout << "HttpGet url" << std::endl;
        return 0;
    }

    std::string url = argv[1];

    setDebugLevel(LOG_LEVEL_INFO);

    HttpAgent *agent = getHttpAgent();
    rv = agent->get(url, 0, 0);
    delete agent;

    resetDebugLevel();

    return rv;
}

static std::string target_alias;
int set_target_machine(const char *machine_alias)
{
    if (machine_alias == NULL) {
        LOG_ERROR("Null machine alias parameter passed. abort!");
        return VPL_ERR_INVALID;
    }
    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_INVALID;
    }

    std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(machine_alias);
    if (it == remote_agents.end()) {
        LOG_ERROR("No matching target %s in "REMOTE_AGENT_CONFIG_FILE", Abort!", machine_alias);
        return VPL_ERR_FAIL;
    }

    LOG_ALWAYS("Set target[%s] to network address %s:%s", machine_alias, it->second.machine_ip.c_str(), it->second.machine_port.c_str());


#ifdef WIN32
    if (_putenv_s(DX_REMOTE_IP_ENV, it->second.machine_ip.c_str()) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%u", it->second.machine_ip.c_str());
        return VPL_ERR_FAIL;
    }
    if (_putenv_s(DX_REMOTE_PORT_ENV, it->second.machine_port.c_str()) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%u", it->second.machine_port.c_str());
        return VPL_ERR_FAIL;
    }
#else
    if (setenv(DX_REMOTE_IP_ENV, it->second.machine_ip.c_str(), 1) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%s", it->second.machine_ip.c_str());
        return VPL_ERR_FAIL;
    }
    if (setenv(DX_REMOTE_PORT_ENV, it->second.machine_port.c_str(), 1) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%s", it->second.machine_port.c_str());
        return VPL_ERR_FAIL;
    }
#endif

    // setup ccdi remote socket
    VPLNet_addr_t ipAddr;
    u16 port;
    
    ipAddr = VPLNet_GetAddr(it->second.machine_ip.c_str());
    port = static_cast<u16>(strtoul(it->second.machine_port.c_str(), NULL, 0));

    VPLSocket_addr_t sockAddr = { VPL_PF_INET, ipAddr, VPLNet_port_hton(port) };
    CCDIClient_SetRemoteSocketForThread(sockAddr);
    target_alias = machine_alias;

    return VPL_OK;
}

int set_target_machine(const char *machine_alias, VPLNet_addr_t &ip_addr, VPLNet_port_t &port_num)
{
    if (machine_alias == NULL) {
        LOG_ERROR("Null machine alias parameter passed. abort!");
        return VPL_ERR_INVALID;
    }
    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_INVALID;
    }

    std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(machine_alias);
    if (it == remote_agents.end()) {
        LOG_ERROR("No matching target %s in "REMOTE_AGENT_CONFIG_FILE", Abort!", machine_alias);
        return VPL_ERR_FAIL;
    }

    LOG_ALWAYS("Set target[%s] to network address %s:%s", machine_alias, it->second.machine_ip.c_str(), it->second.machine_port.c_str());


#ifdef WIN32
    if (_putenv_s(DX_REMOTE_IP_ENV, it->second.machine_ip.c_str()) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%u", it->second.machine_ip.c_str());
        return VPL_ERR_FAIL;
    }
    if (_putenv_s(DX_REMOTE_PORT_ENV, it->second.machine_port.c_str()) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%u", it->second.machine_port.c_str());
        return VPL_ERR_FAIL;
    }
#else
    if (setenv(DX_REMOTE_IP_ENV, it->second.machine_ip.c_str(), 1) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_IP_ENV=%s", it->second.machine_ip.c_str());
        return VPL_ERR_FAIL;
    }
    if (setenv(DX_REMOTE_PORT_ENV, it->second.machine_port.c_str(), 1) != 0) {
        LOG_ERROR("Unable to setenv DX_REMOTE_PORT_ENV=%s", it->second.machine_port.c_str());
        return VPL_ERR_FAIL;
    }
#endif

    // setup ccdi remote socket
    VPLNet_addr_t ipAddr;
    u16 port;
    
    ipAddr = VPLNet_GetAddr(it->second.machine_ip.c_str());
    port = static_cast<u16>(strtoul(it->second.machine_port.c_str(), NULL, 0));
    ip_addr = ipAddr;
    port_num = port;

    VPLSocket_addr_t sockAddr = { VPL_PF_INET, ipAddr, VPLNet_port_hton(port) };
    CCDIClient_SetRemoteSocketForThread(sockAddr);
    target_alias = machine_alias;
    LOG_ALWAYS("IpAddr: %s, port: %s, target_alias: %s", it->second.machine_ip.c_str(), it->second.machine_port.c_str(), machine_alias);

    return VPL_OK;
}

std::string get_target_machine(void)
{
    return target_alias;
}

static int next_ccd_instance_id = 1;
static std::map<std::string, int> target_mapping;
int set_target(const char *machine_alias)
{
    if (machine_alias == NULL) {
        LOG_ERROR("Null alias pointer, Abort!");
        return VPL_ERR_FAIL;
    }
    // parse dxshellrc.cfg and check if the alias is in the configuration
    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) == VPL_OK) {
        std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(machine_alias);
        if (it != remote_agents.end()) {
            return set_target_machine(machine_alias);
        }
    }

    // unset env
#ifdef WIN32
    _putenv_s(DX_REMOTE_IP_ENV, "");
    _putenv_s(DX_REMOTE_PORT_ENV, "");
#else
    unsetenv(DX_REMOTE_IP_ENV);
    unsetenv(DX_REMOTE_PORT_ENV);
#endif //def WIN32

    if (target_mapping.find(machine_alias) == target_mapping.end()) {
        LOG_ALWAYS("Map new target %s with instance id %d", machine_alias, next_ccd_instance_id);
        target_mapping[machine_alias] = next_ccd_instance_id++;
    }

    testInstanceNum = target_mapping[machine_alias];    // DXShell global variable need to set to the testInstanceNum desired.
    target_alias = machine_alias;
    CCDIClient_SetTestInstanceNum(testInstanceNum);

    LOG_ALWAYS("Set target[%s] with testInstanceNum %d", machine_alias, testInstanceNum);

    return VPL_OK;
}

int proc_subcmd(const std::string &testroot, int argc, const char* argv[])
{
    int rv = 0;
    int argIdx = 0;
    const char *machine_alias = NULL;

    dxtool_root.assign(testroot.c_str());

    resetDebugLevel();

    init_cmds();
    init_dx_not_remote_cmds();

    // Check whether "-m <machine aliases>" is provided
    if (argc > 0) {
        if (strcmp("-m", argv[0]) == 0) {
            argc--;
            argIdx++;
            if (argc > 0) {
                machine_alias = argv[1];
                argc--;
                argIdx++;
            }
        }
    }
    // if machine_alias != NULL, then parse dxshellrc.cfg and setup
    // DX_REMOTE_IP_ENV and DX_REMOTE_PORT_ENV
    if (machine_alias != NULL) {
        if (set_target_machine(machine_alias) != 0) {
            LOG_ERROR("Unable to find machine \"%s\" in "REMOTE_AGENT_CONFIG_FILE", abort!", machine_alias);
            return 0;
        }
    }

    if (argc == 0) {
        print_help();
        return 0;
    } else if ((strcmp(argv[argIdx], "Help") == 0) ||
                (strcmp(argv[argIdx], "help") == 0)) {
        print_help();
        return 0;
    } else if ((strcmp(argv[argIdx], "Menu") == 0) ||
                (strcmp(argv[argIdx], "menu") == 0)) {
        print_menu();
        return 0;
    }

    {
        char const* env_str;
        // If DX_REMOTE_IP is set, connect to a TCP socket instead.
        if ((env_str = getenv(DX_REMOTE_IP_ENV)) != NULL) {
            if (machine_alias == NULL) {
                // only setup if user export the ENV variables directly
                // for machine alias, it will be handled by set_target_machine()
                VPLNet_addr_t ipAddr = VPLNet_GetAddr(env_str);
                u16 port = DEFAULT_REMOTE_AGENT_PORT_NUMBER;
                // DX_REMOTE_PORT is optional.
                if ((env_str = getenv(DX_REMOTE_PORT_ENV)) != NULL) {
                    port = static_cast<u16>(strtoul(env_str, NULL, 0));
                }
                VPLSocket_addr_t sockAddr = { VPL_PF_INET, ipAddr, VPLNet_port_hton(port) };
                CCDIClient_SetRemoteSocket(sockAddr);
            }
        } else {
            // Default case: connect to the local CCD via a named socket.
            CCDIClient_SetTestInstanceNum(testInstanceNum);
            // This is mostly for testing named pipe security.
            if ((env_str = getenv(DX_OS_USER_ID_ENV)) != NULL) {
                CCDIClient_SetOsUserId(env_str);
            }
        }
    }

    if (getenv(DX_REMOTE_IP_ENV) != NULL && dx_not_remote_cmds.find(argv[argIdx]) != dx_not_remote_cmds.end()) {
        if (argc == 1 ||
            (argc > 1 && dx_not_remote_cmds[argv[argIdx]].find(argv[argIdx+1]) != dx_not_remote_cmds[argv[argIdx]].end()) ||
            dx_not_remote_cmds[argv[argIdx]].empty()) {
            rv = -1;
            LOG_ERROR("\"%s\" is not supported when DX_REMOTE_IP is set.", argv[argIdx]);
        }
        else {
            rv = dispatch(argc, argv+argIdx);
        }
    }
    else { 
        rv = dispatch(argc, argv+argIdx);
    }

    LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=%s\n", (rv == 0)? "PASS":"FAIL",  argv[argIdx]);

    return rv;
}


int postpone_sleep(int argc, const char* argv[])
{
    int rv = 0;
    std::ostringstream urlStrm;
    char tempBuf[17];
    unsigned int httpPort;
    u64 userId, deviceId;

    if (checkHelp(argc, argv)) {
        printf("%s: Postpone cloudpc sleep\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Executing..");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        goto exit;
    }

    {
        ccd::ListLinkedDevicesOutput listSnOut;
        ccd::ListLinkedDevicesInput request;
        request.set_user_id(userId);
        request.set_storage_nodes_only(true);

        // Try contacting infra for the most updated information first
        rv = CCDIListLinkedDevices(request, listSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListLinkedDevice for user("FMTu64") failed %d", userId, rv);

            // Fall back to cache if cannot reach server
            LOG_ALWAYS("Retry with only_use_cache option");
            request.set_only_use_cache(true);
            rv = CCDIListLinkedDevices(request, listSnOut);
            if (rv != 0) {
                LOG_ERROR("CCDIListLinkedDevice for user("FMTu64") failed %d", userId, rv);
                goto exit;
            }
        }

        if (listSnOut.devices_size() == 0) {
            LOG_ERROR("No cloud PC found.");
            rv = -1;
            goto exit;
        }

        deviceId = listSnOut.devices(0).device_id();
    } 

    {
        HttpAgent *agent = getHttpAgent();

        ccd::GetSystemStateInput ccdiRequest;
        ccdiRequest.set_get_network_info(true);
        ccd::GetSystemStateOutput ccdiResponse;
        rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
            goto exit;
        }

        httpPort = ccdiResponse.network_info().proxy_agent_port();
        snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), "%016"PRIx64, deviceId);
        urlStrm << "http://127.0.0.1:" << httpPort << "/cmd/" << tempBuf << "/keepAwake";

        rv = agent->get(urlStrm.str(), 0, 0);
        delete agent;
    }

exit:
    return rv;
}

int report_lan_devices(int argc, const char* argv[])
{
    int rv = 0;
    ccd::ReportLanDevicesInput request;

    if (checkHelp(argc, argv)) {
        printf("%s [device_type,uuid,device_name,device_id,lan_interface_type,ipv_v4_address,ip_v6_address,media_server_port,virtual_drive_port,web_front_port,ts_port,ts_ext_port,pd_inst_id,notifications]\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    for (int i = 1; i < argc; i++) {
        std::string deviceInfoStr = argv[i];
        std::string str;
        int pos;
        ccd::LanDeviceInfo *info;
        u64 deviceId;
        u32 notifications;

        info = request.add_infos();

        // device_type
        pos = deviceInfoStr.find(',');
        if ((pos == 0) || (pos == std::string::npos)) {
            LOG_ERROR("Failed to find device_type in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        str = deviceInfoStr.substr(0, pos);
        info->set_type((ccd::LanDeviceType_t) atoi(str.c_str()));
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // uuid
        pos = deviceInfoStr.find(',');
        if ((pos == 0) || (pos == std::string::npos)) {
            LOG_ERROR("Failed to find uuid in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        str = deviceInfoStr.substr(0, pos);
        info->set_uuid(str);
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // device_name
        pos = deviceInfoStr.find(',');
        if ((pos == 0) || (pos == std::string::npos)) {
            LOG_ERROR("Failed to find device_name in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        str = deviceInfoStr.substr(0, pos);
        info->set_device_name(str);
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // device_id - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find device_id in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        if (pos != 0) {
            str = deviceInfoStr.substr(0, pos);
            deviceId = VPLConv_strToU64(str.c_str(), NULL, /*base*/16);
            info->set_device_id(deviceId);
        }
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // lan_interface_type
        pos = deviceInfoStr.find(',');
        if ((pos == 0) || (pos == std::string::npos)) {
            LOG_ERROR("Failed to find lan_interface_type in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        str = deviceInfoStr.substr(0, pos);
        info->mutable_route_info()->set_type((ccd::LanInterfaceType_t) atoi(str.c_str()));
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // ip_v4_address
        pos = deviceInfoStr.find(',');
        if ((pos == 0) || (pos == std::string::npos)) {
            LOG_ERROR("Failed to find ip_v4_address in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        str = deviceInfoStr.substr(0, pos);
        info->mutable_route_info()->set_ip_v4_address(str);
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // ip_v6_address - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find ip_v6_address in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        if (pos != 0) {
            str = deviceInfoStr.substr(0, pos);
            info->mutable_route_info()->set_ip_v6_address(str);
        }
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // media_server_port - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find media_server_port in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        if (pos != 0) {
            str = deviceInfoStr.substr(0, pos);
            info->mutable_route_info()->set_media_server_port(atoi(str.c_str()));
        }
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // virtual_drive_port - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find virtual_drive_port in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        if (pos != 0) {
            str = deviceInfoStr.substr(0, pos);
            info->mutable_route_info()->set_virtual_drive_port(atoi(str.c_str()));
        }
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // web_front_port - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find web_front_port in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        if (pos != 0) {
            str = deviceInfoStr.substr(0, pos);
            info->mutable_route_info()->set_web_front_port(atoi(str.c_str()));
        }
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // ts_port - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find ts_port in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        if (pos != 0) {
            str = deviceInfoStr.substr(0, pos);
            info->mutable_route_info()->set_tunnel_service_port(atoi(str.c_str()));
        }
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // ts_ext_port - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find ts_ext_port in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        if (pos != 0) {
            str = deviceInfoStr.substr(0, pos);
            info->mutable_route_info()->set_ext_tunnel_service_port(atoi(str.c_str()));
        }
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // pd_inst_id - optional
        pos = deviceInfoStr.find(',');
        if ((pos == std::string::npos)) {
            LOG_ERROR("Failed to find pd_inst_id in %s", argv[i]);
            rv = -1;
            goto exit;
        }
        str = deviceInfoStr.substr(0, pos);
        // string-typed pd_instance_id field is deprecated now, so don't set anything there
        info->set_pd_instance_id_num(atoi(str.c_str()));
        deviceInfoStr = deviceInfoStr.substr(pos + 1, deviceInfoStr.length());

        // notifications - optional
        if (deviceInfoStr.length() != 0) {
            sscanf(deviceInfoStr.c_str(), "0x%x", &notifications);
            info->set_notifications(notifications);
        }
    }

    rv = CCDIReportLanDevices(request);
    if (rv < 0) {
        LOG_ERROR("Failed to report LAN devices, rv=%d\n", rv);
        goto exit;
    }

exit:
    return rv;
}

int list_lan_devices(int argc, const char* argv[])
{
    int rv = 0;
    ccd::ListLanDevicesInput request;
    ccd::ListLanDevicesOutput response;
    u64 userId;
    std::string optionsStr;
    std::string str;
    int pos;

    if (checkHelp(argc, argv)) {
        printf("%s [include_unregistered,include_registered_but_not_linked,include_linked]\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    rv = getUserIdBasic(&userId);
    if (rv < 0) {
        LOG_ALWAYS("Can't get user ID, rv=%d, continuing without user ID", rv);
        rv = 0;
    } else {
        request.set_user_id(userId);
    }

    if (argc > 1) {
        optionsStr = argv[1];

        // include_unregistered
        pos = optionsStr.find(',');
        if ((pos == 0) || (pos == std::string::npos)) {
            LOG_ERROR("Failed to find include_unregistered in %s", argv[1]);
            rv = -1;
            goto exit;
        }
        str = optionsStr.substr(0, pos);
        if (atoi(str.c_str()) != 0) {
            request.set_include_unregistered(true);
        }
        optionsStr = optionsStr.substr(pos + 1, optionsStr.length());

        // include_registered_but_not_linked
        pos = optionsStr.find(',');
        if ((pos == 0) || (pos == std::string::npos)) {
            LOG_ERROR("Failed to find include_registered_but_not_linked in %s", argv[1]);
            rv = -1;
            goto exit;
        }
        str = optionsStr.substr(0, pos);
        if (atoi(str.c_str()) != 0) {
            request.set_include_registered_but_not_linked(true);
        }
        optionsStr = optionsStr.substr(pos + 1, optionsStr.length());

        // include_linked - optional
        if (optionsStr.length() == 0) {
            LOG_ERROR("Failed to find include_linked in %s", argv[1]);
            rv = -1;
            goto exit;
        }
        str = optionsStr.substr(0, pos);
        if (atoi(str.c_str()) != 0) {
            request.set_include_linked(true);
        }
    }

    rv = CCDIListLanDevices(request, response);
    if (rv < 0) {
        LOG_ERROR("Failed to list LAN devices, rv=%d\n", rv);
        goto exit;
    }

    for (int i = 0; i < response.infos_size(); i++) {
        printf("Device type is %u\n", response.infos(i).type());
        printf("uuid is %s\n", response.infos(i).uuid().c_str());
        printf("Device name is %s\n", response.infos(i).device_name().c_str());
        if (response.infos(i).has_device_id()) {
            printf("Device ID is 0x%llx\n", response.infos(i).device_id());
        }
        printf("LAN interface type is %u\n", response.infos(i).route_info().type());
        printf("IPv4 address is %s\n", response.infos(i).route_info().ip_v4_address().c_str());
        if (response.infos(i).route_info().has_ip_v6_address()) {
            printf("IPv6 address is %s\n", response.infos(i).route_info().ip_v6_address().c_str());
        }
        if (response.infos(i).route_info().has_media_server_port()) {
            printf("Media server port is %u\n", response.infos(i).route_info().media_server_port());
        }
        if (response.infos(i).route_info().has_virtual_drive_port()) {
            printf("Virtual drive port is %u\n", response.infos(i).route_info().virtual_drive_port());
        }
        if (response.infos(i).route_info().has_web_front_port()) {
            printf("Web front port is %u\n", response.infos(i).route_info().web_front_port());
        }
        if (response.infos(i).route_info().has_tunnel_service_port()) {
            printf("Tunnel service port is %u\n", response.infos(i).route_info().tunnel_service_port());
        }
        if (response.infos(i).route_info().has_ext_tunnel_service_port()) {
            printf("External tunnel service port is %u\n", response.infos(i).route_info().ext_tunnel_service_port());
        }
        if (response.infos(i).has_pd_instance_id()) {
            printf("DEPRECATED - Pd instance (string) id is %s\n", response.infos(i).pd_instance_id().c_str());
        }
        if (response.infos(i).has_pd_instance_id_num()) {
            printf("Pd instance (numerical) id is %u\n", response.infos(i).pd_instance_id_num());
        }
        if (response.infos(i).has_notifications()) {
            printf("Notifications is 0x%x\n", response.infos(i).notifications());
        }
        printf("\n");
    }

exit:
    return rv;
}

int get_storage_node_ports(int argc, const char* argv[])
{
    int rv = 0;
    ccd::GetSystemStateInput ccdiRequest;
    ccd::GetSystemStateOutput ccdiResponse;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    ccdiRequest.set_get_network_info(true);
    rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
    if (rv != 0) {
        LOG_ERROR("CCDIGetSystemState failed, rv=%d", rv);
        goto exit;
    }

    if (ccdiResponse.network_info().has_proxy_agent_port()) {
        printf("Proxy agent port is %u\n", ccdiResponse.network_info().proxy_agent_port());
    }
    if (ccdiResponse.network_info().has_media_server_port()) {
        printf("Media server port is %u\n", ccdiResponse.network_info().media_server_port());
    }
    if (ccdiResponse.network_info().has_virtual_drive_port()) {
        printf("Virtual drive port is %u\n", ccdiResponse.network_info().virtual_drive_port());
    }

exit:
    return rv;
}

int probe_lan_devices(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   
    }

    rv = CCDIProbeLanDevices();
    if (rv != 0) {
        LOG_ERROR("CCDIProbeLanDevices failed, rv=%d", rv);
        goto exit;
    }

exit:
    return rv;
}

int userlogin_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_userlogin_cmds, argc, argv);
}

int userlogin_set_eula(int argc, const char* argv[])
{
    u64 userId;
    int eula;
    int rv;

    if (checkHelp(argc, argv)) {
        printf("%s %s username password eulaFlag[0|1]\n", USERLOGIN_STR, argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");
    if (argc < 4) {
        LOG_ERROR("Too few arguments");
        return -1;
    }

    LOG_ALWAYS("Login: Username(%s), Password(%s) Eula(%s)", argv[1], argv[2], argv[3]);
    eula = atoi(argv[3]);

    rv = login_with_eula(argv[1], argv[2], (eula != 0), userId);
    if (rv != 0) {
        LOG_ERROR("login failed:%d", rv);
        goto exit;
    }

exit:
    return rv;
}

int userlogout(int argc, const char*argv[])
{
    int rv = 0;
    u64 userId;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("getUserIdBasic");
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("getUserIdBasic:%d", rv);
        goto exit;
    }

    rv = logout(userId);
    if (rv != 0) {
        LOG_ERROR("logout failed:%d", rv);
        goto exit;
    }

exit:
    return rv;
}

int link_device(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("getUserIdBasic");
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("getUserIdBasic:%d", rv);
        goto exit;
    }

    LOG_ALWAYS("Link Device");
    rv = linkDevice(userId);
    if (rv != 0) {
        LOG_ERROR("linkDevice failed:%d", rv);
        goto exit;
    }

exit:
    return rv;
}
   
int unlink_device(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    u64 deviceId = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        printf(" - <deviceId>                   [Optional]. Unlink self if deviceId not provided.\n");
        return 0;
    }

    if (argc > 1 && argv[1]) {
        deviceId = VPLConv_strToU64(argv[1], NULL, 10);
    }

    LOG_ALWAYS("getUserIdBasic");
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("getUserIdBasic:%d", rv);
        goto exit;
    }

    LOG_ALWAYS("Unlink Device");
    if (deviceId != 0) {
        rv = unlinkDevice(userId, deviceId);
        if (rv != 0) {
            LOG_ERROR("unlinkDevice failed:%d", rv);
            goto exit;
        }
    }
    else {
        rv = unlinkDevice(userId);
        if (rv != 0) {
            LOG_ERROR("unlinkDevice failed:%d", rv);
            goto exit;
        }
    }

exit:
    return rv;
}

int remote_sw_update(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    u64 target;

    if (checkHelp(argc, argv) || argc < 2) {
        printf("%s <targetDeviceId>\n", argv[0]);
        return 0;
    }

    target = strtoull(argv[1], NULL, 0);

    LOG_ALWAYS("Attempt to update "FMTu64, target);

    LOG_ALWAYS("getUserIdBasic");
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("getUserIdBasic:%d", rv);
        goto exit;
    }

    rv = remoteSwUpdate(userId, target);
    if (rv != 0) {
        LOG_ERROR("remoteSwUpdate:%d", rv);
        goto exit;
    }

exit:
    return rv;
}

int notes_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_notes_cmds, argc, argv);
}

int cmd_notes_status(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 1, /*out*/rv)) {
        printf("%s %s - Alias for 'GetSyncState notes'\n", NOTES_STR, argv[0]);
        return rv;
    }
    const char* testArgs[] = { "GetSyncState", "notes" };
    return get_sync_state(2, testArgs);
}

int cmd_notes_enable(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, /*out*/rv)) {
        printf("%s %s <local directory for sync>\n", NOTES_STR, argv[0]);
        return rv;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.mutable_configure_notes_sync()->set_enable_sync_feature(true);
    syncSettingsIn.mutable_configure_notes_sync()->set_set_sync_feature_path(argv[1]);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_sync_feature(true)"
                  " for Notes, userId="FMTu64", err=%d, path=\"%s\"",
                  rv, userId, syncSettingsOut.configure_notes_sync_err(),
                  argv[1]);
    }
    return rv;
}

int cmd_notes_disable(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 1, /*out*/rv)) {
        printf("%s %s\n", NOTES_STR, argv[0]);
        return rv;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.mutable_configure_notes_sync()->set_enable_sync_feature(false);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_sync_feature(false)"
                  " for Notes, userId="FMTu64", err=%d",
                  rv, userId, syncSettingsOut.configure_notes_sync_err());
    }
    return rv;
}

int power_dispatch(int argc, const char* argv[])
{
    return dispatch2ndLevel(g_power_cmds, argc, argv);
}

int foreground_mode(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 4, rv)) {
        printf("%s %s <app_id> <app_type> <foreground{0|1}>\n", POWER_STR, argv[0]);
        printf("(Application types)  0 - No functionalities specified\n");
        printf("                     1 - Photo\n");
        printf("                     2 - Music\n");
        printf("                     3 - Video\n");
        printf("                     4 - Music and Video\n");
        printf("                     5 - All Media\n");
        return rv;
    }

    LOG_ALWAYS("Executing..");

    ccd::CcdApp_t appType = (ccd::CcdApp_t) atoi(argv[2]);
    int foreground = atoi(argv[3]);

    ccd::UpdateAppStateInput req;
    req.set_app_id(argv[1]);
    req.set_app_type(appType);
    req.set_foreground_mode(foreground != 0);
    ccd::UpdateAppStateOutput resp;
    rv = CCDIUpdateAppState(req, resp);
    if (rv != 0) {
        LOG_ERROR("CCDIUpdateAppState(set_foreground_mode) failed: %d", rv);
        goto exit;
    }

exit:
    return rv;
}

int background_tasks(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s %s\n", POWER_STR, argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    ccd::UpdateSystemStateInput req;
    req.set_perform_background_tasks(true);
    ccd::UpdateSystemStateOutput resp;
    rv = CCDIUpdateSystemState(req, resp);
    if (rv != 0) {
        LOG_ERROR("CCDIUpdateSystemState(set_perform_background_tasks) failed: %d", rv);
        goto exit;
    }

exit:
    return rv;
}

int set_background_mode_interval(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <interval_secs>\n", POWER_STR, argv[0]);
        return rv;
    }

    LOG_ALWAYS("Executing..");
    s32 interval_secs = atoi(argv[1]);

    ccd::UpdateSystemStateInput req;
    req.set_background_mode_interval_sec(interval_secs);
    ccd::UpdateSystemStateOutput resp;
    rv = CCDIUpdateSystemState(req, resp);
    if (rv != 0) {
        LOG_ERROR("CCDIUpdateSystemState(set_background_mode_interval_sec) failed: %d", rv);
        goto exit;
    }

exit:
    return rv;
}

int set_enable_network(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <true|false>\n", POWER_STR, argv[0]);
        return rv;
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument '%s'.  Try '"EXE_NAME" %s help' for more info.",
                  argv[1], argv[0]);
        return -1;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_enable_network_activity(enable);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_network to "
                  "%d for userId:"FMTu64" err:%d",
                  rv, enable, userId,
                  syncSettingsOut.enable_network_activity_err());
    }
    return rv;
}

int set_allow_bg_data(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <true|false>\n", POWER_STR, argv[0]);
        return rv;
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument '%s'.  Try '"EXE_NAME" %s help' for more info.",
                  argv[1], argv[0]);
        return -1;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_background_data(enable);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_background_data to "
                  "%d for userId:"FMTu64" err:%d",
                  rv, enable, userId, syncSettingsOut.background_data_err());
    }
    return rv;
}

int set_allow_auto_sync(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <true|false>\n", POWER_STR, argv[0]);
        return rv;
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument '%s'.  Try '"EXE_NAME" %s help' for more info.",
                  argv[1], argv[0]);
        return -1;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_auto_sync(enable);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_auto_sync to %d for"
                  " userId:"FMTu64" err:%d",
                  rv, enable, userId, syncSettingsOut.auto_sync_err());
    }
    return rv;
}

int set_allow_mobile_network(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <true|false>\n", POWER_STR, argv[0]);
        return rv;
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument '%s'.  Try '"EXE_NAME" %s help' for more info.",
                  argv[1], argv[0]);
        return -1;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_mobile_network_data(enable);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_mobile_network_data"
                  " to %d for userId:"FMTu64" err:%d",
                  rv, enable, userId, syncSettingsOut.mobile_network_data_err());
    }
    return rv;
}

int set_only_mobile_network_avail(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <true|false>\n", POWER_STR, argv[0]);
        return rv;
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument '%s'.  Try '"EXE_NAME" %s help' for more info.",
                  argv[1], argv[0]);
        return -1;
    }

    ccd::UpdateSystemStateInput sysStateIn;
    ccd::UpdateSystemStateOutput sysStateOut;
    sysStateIn.set_only_mobile_network_available(enable);
    rv = CCDIUpdateSystemState(sysStateIn, sysStateOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSystemState:%d trying to "
                  "set_only_mobile_network_available to %d, err:%d",
                  rv, enable,
                  sysStateOut.only_mobile_network_available_err());
    }
    return rv;
}

int set_stream_power_mode(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <true|false>\n", POWER_STR, argv[0]);
        return rv;
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument '%s'.  Try '"EXE_NAME" %s help' for more info.",
                  argv[1], argv[0]);
        return -1;
    }

    ccd::UpdateSystemStateInput sysStateIn;
    ccd::UpdateSystemStateOutput sysStateOut;
    sysStateIn.set_stream_power_mode(enable);
    rv = CCDIUpdateSystemState(sysStateIn, sysStateOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSystemState:%d trying to "
                  "set_stream_power_mode to %d, err:%d",
                  rv, enable,
                  sysStateOut.only_mobile_network_available_err());
    }
    return rv;
}

int sync_once(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <appId> - Syncs only once, ignoring power settings\n", POWER_STR, argv[0]);
        return rv;
    }

    ccd::SyncOnceInput soIn;
    ccd::SyncOnceOutput soOut;
    soIn.set_app_id(argv[1]);
    rv = CCDISyncOnce(soIn, soOut);
    if(rv != 0) {
        LOG_ERROR("CCDISyncOnce:%d", rv);
    }
    return rv;
}

int power_get_status(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 1, rv)) {
        printf("%s %s\n", POWER_STR, argv[0]);
        return rv;
    }

    ccd::GetSystemStateInput getSysStateInput;
    getSysStateInput.set_get_only_mobile_network_available(true);
    getSysStateInput.set_get_background_mode_interval_sec(true);
    getSysStateInput.set_get_stream_power_mode(true);
    getSysStateInput.set_get_power_mode(true);
    ccd::GetSystemStateOutput getSysStateOutput;

    int rc = CCDIGetSystemState(getSysStateInput, getSysStateOutput);
    if(rc != 0) {
        LOG_ERROR("CCDIGetSystemState:%d", rv);
        rv = rc;
    }else{
        const char* powerStateStr;
        switch(getSysStateOutput.power_mode_status().power_mode())
        {
            case ccd::POWER_NO_SYNC:
                powerStateStr = "POWER_NO_SYNC";
                break;
            case ccd::POWER_FOREGROUND:
                powerStateStr = "POWER_FOREGROUND";
                break;
            case ccd::POWER_BACKGROUND:
                powerStateStr = "POWER_BACKGROUND";
                break;
            default:
                LOG_ERROR("Not handled");
                powerStateStr = "ERR_NOT_HANDLED";
                break;
        }
        LOG_ALWAYS("System Status:"
                   "\n Only-mobile-network-available:%d"
                   "\n bg_interval_sec:%d"
                   "\n stream_power_mode:%d"
                   "\n ccd_power_state:%s",
                   getSysStateOutput.only_mobile_network_available(),
                   getSysStateOutput.background_mode_interval_sec(),
                   getSysStateOutput.stream_power_mode(),
                   powerStateStr);
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::GetSyncStateInput getSyncStateInput;
    getSyncStateInput.set_user_id(userId);
    getSyncStateInput.set_get_is_network_activity_enabled(true);
    getSyncStateInput.set_get_background_data(true);
    getSyncStateInput.set_get_auto_sync(true);
    getSyncStateInput.set_get_mobile_network_data(true);
    ccd::GetSyncStateOutput getSyncStateOutput;

    rc = CCDIGetSyncState(getSyncStateInput, getSyncStateOutput);
    if(rc != 0) {
        LOG_ERROR("CCDIGetSyncState:%d", rv);
        rv = rc;
    }else{
        LOG_ALWAYS("User Setting Status: UserId:"FMTx64
                   "\n network_activity_enabled:%d"
                   "\n background_data_enabled:%d"
                   "\n auto_sync_enabled:%d"
                   "\n mobile_network_data_enabled:%d",
                   userId,
                   getSyncStateOutput.is_network_activity_enabled(),
                   getSyncStateOutput.background_data(),
                   getSyncStateOutput.auto_sync(),
                   getSyncStateOutput.mobile_network_data());
    }
    return rv;
}

int set_clouddoc_sync(int argc, const char* argv[])
{
    int rv;
    if (checkHelpAndExactArgc(argc, argv, 2, rv)) {
        printf("%s %s <true|false>\n", POWER_STR, argv[0]);
        return rv;
    }

    std::string strTrue("true");
    std::string strFalse("false");
    bool enable;

    if(strTrue == argv[1]) {
        enable = true;
    }else if(strFalse == argv[1]) {
        enable = false;
    }else {
        LOG_ERROR("Bad argument '%s'.  Try '"EXE_NAME" %s help' for more info.",
                  argv[1], argv[0]);
        return -1;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_enable_clouddoc_sync(enable);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_cloud_doc_sync to %d for"
                  " userId:"FMTu64" err:%d",
                  rv, enable, userId, syncSettingsOut.enable_clouddoc_sync_err());
    }
    return rv;
}

static int list_datasets(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    dumpDatasetList(userId);

    return rv;

}

int add_device(int argc, const char* argv[])
{
    // Usage: AddDevice <machine alias> <OS> <machine name> <username> <ip> <ip2>
    // This function will update the device into "dxshellrc.cfg"

    int rv = 0;
    if (argc < 6|| checkHelp(argc, argv)) {
        printf("%s <machine alias> <OS> <machine name> <username> <IP> <Controller IP>\n", argv[0]);
        printf(" - <machine alias> : CloudPC, MD or Client\n");
        printf(" - <OS> : Windows/Android/WindowsRT/iOS/Orbe/Linux\n");
        printf(" - <machine name> : Comupter Name or Mobile Device Name. ex: build-PC, Acer/A200 or build-iPhone\n");
        printf(" - <username> : SSH User Name. ex: build\n");
        printf(" - <IP> : Computer's IP(Windows/Linux/Orbe) or Mobile Device IP(iOS/Android/WindowsRT)\n");
        printf(" - <Controller IP> : optional for both of Android (Windows's IP) and iOS device(Mac's IP) only\n");
        return -1;
    }

    if (strcmp(argv[2], OS_ANDROID) == 0 || strcmp(argv[2], OS_iOS) == 0) {
        if (argc < 7) {
            LOG_ERROR("No <Controller IP> is provided for add Android device or iOS device");
            return VPL_ERR_FAIL;
        }
        const char *config[] = {
            "ConfigAutoTest",
            argv[1],
            argv[2],
            argv[4],
            argv[6],
            "",
            argv[5]
        };
        rv = config_autotest(7, config);
    }
    else {
        const char *config[] = {
            "ConfigAutoTest",
            argv[1],
            argv[2],
            argv[4],
            argv[5]
        };
        rv = config_autotest(5, config);
    }
    if (rv != 0) {
        LOG_ERROR("Fail to configure device, err = %d", rv);
        return rv;
    }

    const char *config[] = {
        "ConfigDevice",
        argv[1],
        argv[3],
    };
    rv = config_device(3, config);

    return rv;
}

int del_device(int argc, const char* argv[])
{
    // Usage: DelDevice -- cleanUp all || DelDevice <machine alias>

    int rv = 0;
    if (argc < 1 || checkHelp(argc, argv)) {
        printf("DelDevice - CleanUp all devices\n");
        printf("DelDevice <machine alias> - Delete a device\n");
        return -1;
    }

    if (argc == 1) {
        LOG_ALWAYS("1");
        const char *config[] = {
            "ConfigAutoTest",
            "CleanUp"
        };
        rv = config_autotest(2, config);
    } else if (argc == 2) {
        LOG_ALWAYS("2");
        const char *config[] = {
            "ConfigAutoTest",
            "CleanUp",
            argv[2]
        };
        rv = config_autotest(3, config);
    }

    return rv;
}

int deploy_remote_agent(int argc, const char* argv[])
{
    int rv = 0;
    if (argc < 2 || checkHelp(argc, argv)) {
        printf("%s <alias 1> ... <alias N>\n", argv[0]);
        return -1;
    }
    // XXX Disable checking remote_agent configurations for we don't have port number set yet.
#if 0
    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_FAIL;
    }
#endif

    for (int i = 1; i < argc; i++) {
#if 0
        if (remote_agents.find(argv[i]) == remote_agents.end()) {
            LOG_ERROR("No matched target %s in "REMOTE_AGENT_CONFIG_FILE", abort!", argv[i]);
            return VPL_ERR_FAIL;
        }
#endif
        std::stringstream ssCmd;
        ssCmd << EXE_PREFIX << "Remote_Agent_Launcher.py CopyFiles " << argv[i];
        LOG_ALWAYS("Deploying dx_remote_agent to target = %s", argv[i]);
        rv = doSystemCall(ssCmd.str().c_str());
    }
    return rv;
}

static int start_remote_agent(int argc, const char* argv[])
{
    int rv = 0;

    // Usage: ./dxshell StartRemoteAgent <alias1> <alias2> <alias3> ...

    if (checkHelp(argc, argv) || argc < 2) {
        printf("%s <alias 1> ... <alias N>\n", argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        std::stringstream ssCmd;
        ssCmd << EXE_PREFIX << "Remote_Agent_Launcher.py Init " << argv[i];
        LOG_ALWAYS("Starting target = %s", argv[i]);
        rv = doSystemCall(ssCmd.str().c_str());
    }

    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_FAIL;
    }

    for (int i = 1; i < argc; i++) {
        std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(argv[i]);
        if (remote_agents.find(argv[i]) == remote_agents.end()) {
            // Likely error candidate is App_Launcher.py not successfully "
            // starting CCD, or not generating required files "
            // ie: port.txt required, search \"recordPortUsed\",
            // TODO: Document Other required files)
            LOG_ERROR("No matching target %s in "REMOTE_AGENT_CONFIG_FILE
                      ", Abort! See Comment", argv[i]);
            return VPL_ERR_FAIL;
        }


        if(it->second.machine_os.compare(OS_ANDROID) == 0) {
            if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = stop_android_dx_remote_agent()) != 0) {
                LOG_ERROR("Unable stop android dx_remote_agent: %d", rv);
                break;
            }

            if ((rv = stop_android_cc_service()) != 0) {
                LOG_ERROR("Unable stop android cc_service: %d", rv);
                break;
            }

            if ((rv = launch_android_cc_service()) != 0) {
                LOG_ERROR("Unable stop android cc_service: %d", rv);
                break;
            }

            if ((rv = launch_android_dx_remote_agent()) != 0) {
                LOG_ERROR("Unable stop android dx_remote_agent: %d", rv);
                break;
            }

            if ((rv = check_android_net_status()) != 0) {
                LOG_ERROR("Unable check_android_net_outgoing: %d", rv);
                break;
            }
        }
        else if(it->second.machine_os.compare(OS_iOS) == 0) {
             if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = stop_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable stop dx_remote_agent app: %d", rv);
                break;
            }

            if ((rv = launch_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable launch dx_remote_agent app: %d", rv);
                break;
            }
        }
        else if(it->second.machine_os.compare(OS_WINDOWS_RT) == 0) {
            if (set_env_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = stop_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable stop dx_remote_agent app: %d", rv);
                break;
            }

            if ((rv = launch_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable launch dx_remote_agent app: %d", rv);
                break;
            }
        }
        else {
            rv = 0;
        }

        if (set_env_ip_port(it) < 0) {
            rv = VPL_ERR_FAIL;
            break;
        }

        if ((rv = remote_agent_poll(argv[i])) != 0) {
            LOG_ERROR("DxRemoteAgent[%s] did not create connection with dxshell in %d seconds", argv[i], DX_REMOTE_POLL_TIME_OUT);
            break;
        }
    }

    return rv;
}

static int stop_remote_agent(int argc, const char* argv[])
{
    int rv = 0;

    // Usage: ./dxshell StopRemoteAgent <alias1> <alias2> <alias3> ...

    if (checkHelp(argc, argv) || argc < 2) {
        printf("%s <alias 1> ... <alias N>\n", argv[0]);
        return 0;
    }

    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_FAIL;
    }

    for (int i = 1; i < argc; i++) {
        std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(argv[i]);
        if (it == remote_agents.end()) {
            LOG_ERROR("No matched target %s in "REMOTE_AGENT_CONFIG_FILE", abort!", argv[i]);
            return VPL_ERR_FAIL;
        }
       
        if(it->second.machine_os.compare(OS_ANDROID) == 0) {
            if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = stop_android_dx_remote_agent()) != 0) {
                LOG_ERROR("Unable stop android dx_remote_agent: %d", rv);
                break;
            }

            if ((rv = stop_android_cc_service()) != 0) {
                LOG_ERROR("Unable stop android cc_service: %d", rv);
                break;
            }
        }
        else if(it->second.machine_os.compare(OS_iOS) == 0) {
            if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = stop_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable stop dx_remote_agent app: %d", rv);
                break;
            }
        }
        else if(it->second.machine_os.compare(OS_WINDOWS_RT) == 0) {
            if (set_env_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = stop_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable stop dx_remote_agent app: %d", rv);
                break;
            }
        }
        else {
            rv = 0;
        }
    }

    for (int i = 1; i < argc; i++) {
        std::stringstream ssCmd;
        ssCmd << EXE_PREFIX << "Remote_Agent_Launcher.py StopProcess " << argv[i];
        LOG_ALWAYS("Stopping target = %s", argv[i]);
        rv = doSystemCall(ssCmd.str().c_str());
    }

    return rv;
}

static int restart_remote_agent_app(int argc, const char* argv[])
{
    int rv = 0;

    // Usage: ./dxshell RestartRemoteAgentApp <alias1> <alias2> <alias3> ...

    if (checkHelp(argc, argv) || argc < 2) {
        printf("%s <alias 1> ... <alias N>\n", argv[0]);
        return 0;
    }

    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_FAIL;
    }

    for (int i = 1; i < argc; i++) {
        std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(argv[i]);
        if (remote_agents.find(argv[i]) == remote_agents.end()) {
            LOG_ERROR("No matched target %s in "REMOTE_AGENT_CONFIG_FILE", abort!", argv[i]);
            return VPL_ERR_FAIL;
        }


        if(it->second.machine_os.compare(OS_ANDROID) == 0) {
            if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = restart_android_dx_remote_agent()) != 0) {
                LOG_ERROR("Unable restart android dx_remote_agent: %d", rv);
                break;
            }

        }
        else if(it->second.machine_os.compare(OS_iOS) == 0) {
            if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = restart_dx_remote_agent_ios_app(argv[i])) != 0) {
                LOG_ERROR("Unable restart dx_remote_agent ios app: %d", rv);
                break;
            }

        }
        else if(it->second.machine_os.compare(OS_WINDOWS_RT) == 0) {
             if (set_env_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = stop_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable stop dx_remote_agent app: %d", rv);
                break;
            }

            if ((rv = launch_dx_remote_agent_app(argv[i])) != 0) {
                LOG_ERROR("Unable launch dx_remote_agent app: %d", rv);
                break;
            }
        }
        else {
            rv = 0;
        }
    }

    return rv;
}

static int collect_cc_log(int argc, const char* argv[])
{
     int rv = 0;

    // Usage: ./dxshell CollectCCLog <alias1> <alias2> <alias3> ...

    if (checkHelp(argc, argv) || argc < 2) {
        printf("%s <alias 1> ... <alias N>\n", argv[0]);
        return 0;
    }

    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_FAIL;
    }

    for (int i = 1; i < argc; i++) {
        std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(argv[i]);
        if (it == remote_agents.end()) {
            LOG_ERROR("No matched target %s in "REMOTE_AGENT_CONFIG_FILE", abort!", argv[i]);
            return VPL_ERR_FAIL;
        }
       
        if(it->second.machine_os.compare(OS_ANDROID) == 0) {
            if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = get_ccd_log_from_android()) != 0) {
                LOG_ERROR("Unable stop android dx_remote_agent: %d", rv);
                break;
            }
        }

        if(it->second.machine_os.compare(OS_LINUX) != 0) {
            std::stringstream ssCmd;
            ssCmd << EXE_PREFIX << "Remote_Agent_Launcher.py CollectCCDLog " << argv[i];
            LOG_ALWAYS("CollectCCLog from target = %s", argv[i]);
            rv = doSystemCall(ssCmd.str().c_str());
        }
    }

    return rv;
}

static int clean_cc_log(int argc, const char* argv[])
{
     int rv = 0;

    // Usage: ./dxshell CleanCCLog <alias1> <alias2> <alias3> ...

    if (checkHelp(argc, argv) || argc < 2) {
        printf("%s <alias 1> ... <alias N>\n", argv[0]);
        return 0;
    }

    std::map<std::string, remote_agent_config> remote_agents;
    if (parse_remote_agent_cfg(remote_agents) != VPL_OK) {
        LOG_ERROR("Unable to parse file "REMOTE_AGENT_CONFIG_FILE", Abort!");
        return VPL_ERR_FAIL;
    }

    for (int i = 1; i < argc; i++) {
        std::map<std::string, remote_agent_config>::iterator it = remote_agents.find(argv[i]);
        if (it == remote_agents.end()) {
            LOG_ERROR("No matched target %s in "REMOTE_AGENT_CONFIG_FILE", abort!", argv[i]);
            return VPL_ERR_FAIL;
        }
       
        if(it->second.machine_os.compare(OS_ANDROID) == 0) {
            if (set_env_control_ip_port(it) < 0) {
                rv = VPL_ERR_FAIL;
                break;
            }

            if ((rv = clean_ccd_log_on_android()) != 0) {
                LOG_ERROR("Unable stop android dx_remote_agent: %d", rv);
                break;
            }
        }

        if(it->second.machine_os.compare(OS_LINUX) != 0) {
            std::stringstream ssCmd;
            ssCmd << EXE_PREFIX << "Remote_Agent_Launcher.py CleanCCDLog " << argv[i];
            LOG_ALWAYS("CleanCCLog from target = %s", argv[i]);
            rv = doSystemCall(ssCmd.str().c_str());
        }
    }

    return rv;
}

static int report_different_network(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    ccd::UpdateSystemStateInput request;
    request.set_report_different_network(true);
    ccd::UpdateSystemStateOutput response;
    rv = CCDIUpdateSystemState(request, response);
    if (rv != 0) {
        LOG_ERROR("report network change failed:%d", rv);
    }
    return rv;
}


int list_storage_node_datasets(int argc, const char* argv[])
{
    int rv = 0;
    ccd::ListStorageNodeDatasetsOutput response;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    rv = CCDIListStorageNodeDatasets(response);
    if (rv < 0) {
        LOG_ERROR("Failed to list storage node datasets, rv=%d\n", rv);
        goto exit;
    }

    for (int i = 0; i < response.datasets_size(); i++) {
        printf("Dataset %d:\n", i);
        printf("User ID is "FMTx64"\n", response.datasets(i).user_id());
        printf("Dataset ID is "FMTx64"\n", response.datasets(i).dataset_id());
    }

exit:
    return rv;
}

int report_network_connected(int argc, const char* argv[])
{
    int rv = 0;
    ccd::UpdateSystemStateInput request;
    ccd::UpdateSystemStateOutput response;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;
    }

    LOG_ALWAYS("Executing..");

    request.set_report_network_connected(true);
    rv = CCDIUpdateSystemState(request, response);
    if (rv != 0) {
        LOG_ERROR("report network connected failed:%d", rv);
        goto exit;
    }

    LOG_ALWAYS("Network connectivity reported");

exit:
    return rv;
}

// choose a folder where renaming from this folder to the final destination
// folder will not result in a cross-volume error.
std::string getDxshellTempFolder()
{
    std::string tempFolder;
    const char* tmpFolderName = "dxshell_tmp";
#ifdef WIN32
#ifdef VPL_PLAT_IS_WINRT
    char *root;
    getRootPath(&root);
    tempFolder = std::string(root).append("/").append(tmpFolderName);
    releaseRootPah(root);
#else
    char currentDirectory[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDirectory);
    tempFolder = std::string(currentDirectory).append("/").append(tmpFolderName);
#endif
#elif defined(IOS)
    tempFolder = std::string(getHomePath()).append("/tmp/").append(tmpFolderName);
#else
    tempFolder.assign(getenv("HOME"));
    tempFolder = tempFolder.append("/").append(tmpFolderName);
#endif
    return tempFolder;
}

int stop_remoteagent(int argc, const char* argv[])
{
    return stop_remote_agent(argc, argv);
}

int start_remoteagent(int argc, const char* argv[])
{
    return start_remote_agent(argc, argv);
}

int restart_remoteagent_app(int argc, const char* argv[])
{
    return restart_remote_agent_app(argc, argv);
}

#if (defined LINUX || defined __CLOUDNODE__) && !defined LINUX_EMB
int enable_in_memory_logging(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   
    }

    rv = CCDIEnableInMemoryLogging();
    if (rv != 0) {
        LOG_ERROR("CCDIEnableInMemoryLogging failed, rv=%d", rv);
        goto exit;
    }

exit:
    return rv;
}

int disable_in_memory_logging(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   
    }

    rv = CCDIDisableInMemoryLogging();
    if (rv != 0) {
        LOG_ERROR("CCDIDisableInMemoryLogging failed, rv=%d", rv);
        goto exit;
    }

exit:
    return rv;
}
#endif

int flush_in_memory_logs(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        return 0;   
    }

    rv = CCDIFlushInMemoryLogs();
    if (rv != 0) {
        LOG_ERROR("CCDIFlushInMemoryLogs failed, rv=%d", rv);
        goto exit;
    }

exit:
    return rv;
}

int ts_test(int argc, const char* argv[])
{
    return tstest_commands(argc, argv);
}