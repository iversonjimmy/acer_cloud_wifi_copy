//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#include <gvm_file_utils.hpp>
#include <ccdi_client_tcp.hpp>
#include "scopeguard.hpp"

#include "dx_common.h"
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "TargetDevice.hpp"
#include "ccdconfig.hpp"

#include "autotest_common_utils.hpp"
#include "autotest_util.hpp"
#include "fs_test.hpp"

#include "cJSON2.h"

#include <string>
#include <sstream>
#include <deque>

int push_file(Target &target, const std::string & file_path, const std::string & file_name) {

    int rv = 0;

    std::string file_dst = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    rv = target.device->pushFile(file_path, file_dst);
    if (rv != 0) {
        LOG_ERROR("Fail to push file from local to remote: %s --> %s, rv = %d",
                  file_path.c_str(), file_dst.c_str(), rv);
    }

    return rv;
}

int rename_file(Target &target, const std::string & from, const std::string & to) {
    int rv = 0;
    std::string file_src = 
        convert_path_convention(target.separator, target.work_dir + "/" + from);
    std::string file_dst = 
        convert_path_convention(target.separator, target.work_dir + "/" + to);
    rv = target.device->renameFile(file_src, file_dst);
    if (rv != 0) {
        LOG_ERROR("Fail to rename file from %s to %s, rv = %d",
                  file_src.c_str(), file_dst.c_str(), rv);
    }
    return rv;
}

int delete_file(Target &target, const std::string & file_name) {
    int rv = 0;

    std::string file_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    rv = target.device->deleteFile(file_path);
    if (rv != 0) {
        LOG_ERROR("Fail to delete file: %s, rv = %d", file_path.c_str(), rv);
    }
    return rv;
}

int create_dir(Target &target, const std::string & dir_name) {
    int rv = 0;

    std::string dir_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + dir_name);

    rv = target.device->createDir(dir_path, 0777);
    if (rv != 0) {
        LOG_ERROR("Failed to create directory %s! rv = %d", dir_path.c_str(), rv);
    }
    return rv;
}

int delete_dir(Target &target, const std::string & dir_name) {
    int rv = 0;

    std::string dir_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + dir_name);

    rv = target.device->removeDirRecursive(dir_path);
    if (rv != 0) {
        LOG_ERROR("Failed to delete directory %s! rv = %d", dir_path.c_str(), rv);
    }
    return rv;
}

int compare_files(Target &target, const std::string & file_path, const std::string & file_name, const int max_retry) {
    int rv = 0;
    int retry = 0;
    VPLFS_stat_t stat_src, stat_dst;

    std::string file_src_clone = file_path + ".clone";
    std::string file_dst = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    if (VPLFS_Stat(file_path.c_str(), &stat_src)) {
        LOG_ERROR("fail to stat src file: %s", file_path.c_str());
    }

    LOG_INFO("Stat the file: %s", file_dst.c_str());
    while (retry++ < max_retry) {
        rv = target.device->statFile(file_dst, stat_dst);
        if (rv != 0) {
            //LOG_ERROR("Fail to stat download file: %s", file_dst.c_str());
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL));
        }
        else if (stat_src.size == stat_dst.size){
            //LOG_ALWAYS("Stat %s finish!!!", file_dst.c_str());
            break;
        }
        else {
            //LOG_ERROR("File length inconsistent: %s", file_dst.c_str());
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL));
        }
    }

    if (rv != 0) {
        LOG_ERROR("Unable to find file: %s, rv = %d", file_dst.c_str(), rv);
        goto exit;
    }

    rv = target.device->pullFile(file_dst, file_src_clone);
    if (rv != 0) {
        LOG_ERROR("Unable to pull file: %s, rv = %d", file_dst.c_str(), rv);
        goto exit;
    }

    rv = file_compare(file_path.c_str(), file_src_clone.c_str());

exit:

    Util_rm_dash_rf(file_src_clone);

    return rv;
}

int is_file_existed(Target &target, const std::string & file_name, const int max_retry) {
    int rv = 0;
    int retry = 0;
    VPLFS_stat_t stat;

    std::string file_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    LOG_INFO("Stat the file: %s", file_path.c_str());
    while (retry++ < max_retry) {
        rv = target.device->statFile(file_path, stat);
        if (rv != 0) {
            //LOG_ERROR("Fail to stat file: %s", file_path.c_str());
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL));
        }
        else if (stat.size == stat.size){
            //LOG_ALWAYS("File %s exists!!!", file_path.c_str());
            break;
        }
        else {
            //LOG_ERROR("File length inconsistent: %s", file_path.c_str());
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL));
        }
    }

    if (rv != 0) {
        LOG_ERROR("Unable to find file: %s, rv = %d", file_path.c_str(), rv);
    }

    return rv;
}

int is_file_deleted(Target &target, const std::string & file_name, const int max_retry) {
    int rv = 0;
    int retry = 0;
    VPLFS_stat_t stat;

    std::string file_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    LOG_INFO("Stat the file: %s", file_path.c_str());
    while (retry++ < max_retry) {
        rv = target.device->statFile(file_path, stat);
        if (rv != 0) {
            //LOG_ERROR("Fail to stat file: %s", file_path.c_str());
            break;
        }
        else {
            //LOG_ALWAYS("File %s exists!!!", file_path.c_str());
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL));
        }
    }
    if (rv != 0) {
        rv = 0;
        LOG_ALWAYS("Unable to find file: %s, rv = %d", file_path.c_str(), rv);
    } else {
        rv = -1;
        LOG_ERROR("File: %s exists!!!", file_path.c_str());
    }
    return rv;
}

int stat_file(Target &target, VPLFS_stat_t &stat, const std::string & file_name, const int max_retry) {
    int rv = 0;
    int retry = 0;

    std::string file_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    LOG_INFO("Stat the file: %s", file_path.c_str());
    while (retry++ < max_retry) {
        rv = target.device->statFile(file_path, stat);
        if (rv == 0) {
            break;
        } else {
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL));
        }
    }
    if (rv != 0) {
        LOG_ERROR("Unable to find file: %s, rv = %d", file_path.c_str(), rv);
    }

    return rv;
}

int touch_file(Target &target, const std::string & file_name) {
    int rv = 0;

    std::string file_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    LOG_INFO("Touching the file: %s", file_path.c_str());
    rv = target.device->touchFile(file_path);
    if (rv != 0) {
        LOG_ERROR("Unable to touch file: %s, rv = %d", file_path.c_str(), rv);
    }

    return rv;
}

int is_file_changed(Target &target, const std::string & file_name, const VPLFS_stat_t stat) {
    int rv = 0;
    VPLFS_stat_t stat_new;

    std::string file_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + file_name);

    LOG_INFO("Stat the file: %s", file_path.c_str());
    rv = target.device->statFile(file_path, stat_new);
    if (rv != 0) {
        LOG_ERROR("Unable to stat file: %s, rv = %d", file_path.c_str(), rv);
        goto exit;
    }

    if (stat.size == stat_new.size && stat.mtime == stat_new.mtime) {
        LOG_INFO("File %s is unchanged", file_path.c_str());
        return 0;
    } else {
        LOG_ERROR("File %s was changed, "FMTu64" -> "FMTu64"", file_path.c_str(), (unsigned long long)stat.mtime, (unsigned long long)stat_new.mtime);
        return 1;
    }
exit:
    return rv;
}

int init_ccd(Target &target, const char* username, const char* password) {
    int rv = 0;

    if (!target.alias.empty()) {
        rv = set_target_machine(target.alias.c_str());
        if (rv != 0) {
            LOG_ERROR("Unable to set target machine %s, rv = %d", target.alias.c_str(), rv);
            goto exit;
        }
        rv = check_link_dx_remote_agent(target.alias);
        if (rv != 0) {
            LOG_ERROR("Unable to link device %s, rv = %d", target.alias.c_str(), rv);
            goto exit;
        }
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    rv = get_target_osversion(target.os_version);
    if (rv != 0) {
        LOG_ERROR("Unable to get os version, rv = %d", rv);
        goto exit;
    }

    {
        const char *testArg[] = { "StartCCD" };
        rv = start_ccd(1, testArg);
        if (rv != 0) {
            LOG_ERROR("Unable to start CCD, rv = %d", rv);
            goto exit;
        }
    }

    if (target.is_cloud_pc) {
        const char *testArgs[] = { "StartCloudPC", username, password };
        rv = start_cloudpc(3, testArgs);
        if (rv < 0) {
            rv = start_cloudpc(3, testArgs);
            if (rv != 0) {
                LOG_ERROR("Unable to start cloud pc, rv = %d", rv);
                goto exit;
            }
        }
    } else {
        const char *testArgs[] = { "StartClient", username, password };
        rv = start_client(3, testArgs);
        if (rv < 0) {
            rv = start_client(3, testArgs);
            if (rv != 0) {
                LOG_ERROR("Unable to start client, rv = %d", rv);
                goto exit;
            }
        }
    }

    target.device = getTargetDevice();
    rv = target.device->getDirectorySeparator(target.separator);
    if (rv != 0) {
        LOG_ERROR("Unable to get the path separator, rv = %d", rv);
        goto exit;
    }

    rv = getUserIdBasic(&target.user_id);
    if (rv != 0) {
        LOG_ERROR("Unable to get the user id, rv = %d", rv);
        goto exit;
    }

    rv = target.device->getDxRemoteRoot(target.work_dir);
    if (rv != 0) {
        LOG_ERROR("Unable to get the remote root, rv = %d", rv);
        goto exit;
    }

    rv = getDeviceId(&target.device_id);
    if (rv != 0) {
        LOG_ERROR("Unable to get the device id, rv = %d", rv);
        goto exit;
    }

    ccdconfig_set("syncboxSyncConfigErrPollInterval", "10");

    target.username = username;
    target.password = password;
exit:
    return rv;
}

int resume_ccd(Target &target) {
    int rv = 0;
    if (!target.alias.empty()) {
        rv = set_target_machine(target.alias.c_str());
        if (rv != 0) {
            LOG_ERROR("Unable to set target machine %s, rv = %d", target.alias.c_str(), rv);
            goto exit;
        }
        rv = check_link_dx_remote_agent(target.alias);
        if (rv != 0) {
            LOG_ERROR("Unable to link device %s, rv = %d", target.alias.c_str(), rv);
            goto exit;
        }
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }
    {
        const char *testArg[] = { "StartCCD" };
        rv = start_ccd(1, testArg);
        if (rv != 0) {
            LOG_ERROR("Unable to start CCD, rv = %d", rv);
            goto exit;
        }
    }
exit:
    return rv;
}

void stop_ccd(Target &target) {
    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    if (target.is_cloud_pc) {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(1, testArg);
    } else {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }
    delete target.device;
}

int suspend_ccd(Target &target) {
    int rv = 0;
    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    {
        const char *testArg[] = { "StopCCD" };
        rv = stop_ccd_soft(1, testArg);
        if (rv != 0) {
            LOG_ERROR("Unable to stop ccd, rv = %d", rv);
            goto exit;
        }
    }
exit:
    return rv;
}

int link_target(Target &target) {
    int rv = 0;
    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    if (target.is_cloud_pc) {
        const char *testArgs[] = { "StartCloudPC", target.username.c_str(), target.password.c_str() };
        rv = start_cloudpc(3, testArgs);
        if (rv < 0) {
            rv = start_cloudpc(3, testArgs);
            if (rv != 0) {
                LOG_ERROR("Unable to start cloud pc, rv = %d", rv);
                goto exit;
            }
        }
    } else {
        const char *testArgs[] = { "StartClient", target.username.c_str(), target.password.c_str() };
        rv = start_client(3, testArgs);
        if (rv < 0) {
            rv = start_client(3, testArgs);
            if (rv != 0) {
                LOG_ERROR("Unable to start client, rv = %d", rv);
                goto exit;
            }
        }
    }

exit:
    return rv;
}

int unlink_target(Target &target) {
    int rv = 0;
    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    rv = unlinkDevice(target.user_id);
    if (rv != 0) {
        LOG_ERROR("Unable to unlink device, rv = %d", rv);
        goto exit;
    }

exit:
    return rv;
}

int enable_syncbox(Target &target, std::string path) {
    int rv = 0;
    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;

    std::string absolute_path = 
        convert_path_convention(target.separator, target.work_dir + "/" + path);

    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    syncSettingsIn.set_user_id(target.user_id);
    syncSettingsIn.mutable_configure_syncbox_sync()->set_enable_sync_feature(true);
    syncSettingsIn.mutable_configure_syncbox_sync()->set_set_sync_feature_path(absolute_path.c_str());
    syncSettingsIn.mutable_configure_syncbox_sync()->set_is_archive_storage(target.is_cloud_pc);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_sync_feature(true)"
                  " for SyncBox %s, userId="FMTu64", err=%d, path=\"%s\"",
                  rv, target.is_cloud_pc ? "server" : "client", target.user_id, syncSettingsOut.configure_syncbox_sync_err(),
                  absolute_path.c_str());
    }
    return rv;
}

int disable_syncbox(Target &target) {
    int rv = 0;
    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;

    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    syncSettingsIn.set_user_id(target.user_id);
    syncSettingsIn.mutable_configure_syncbox_sync()->set_enable_sync_feature(false); 
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_sync_feature(false)"
                  " for SyncBox, userId="FMTu64", err=%d",
                  rv, target.user_id, syncSettingsOut.configure_syncbox_sync_err());
    }
    return rv;
}


int get_syncbox_path(Target &target, std::string &path) {
    int rv = 0;
    ccd::GetSyncStateInput getSyncStateIn;
    ccd::GetSyncStateOutput getSyncStateOut;

    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    getSyncStateIn.set_get_syncbox_sync_settings(true);
    rv = CCDIGetSyncState(getSyncStateIn, getSyncStateOut);
    if(rv != 0) {
        LOG_ERROR("CCDIGetSyncState for syncbox fail rv %d", rv);
    } else {
        if (getSyncStateOut.syncbox_sync_settings_size() > 0) {
            path = getSyncStateOut.syncbox_sync_settings(0).sync_feature_path();
        }
    }
    return rv;
}

int find_conflict_file(Target &target, const std::string & dir_name, const std::string & file_name, std::vector<std::string> & conflict_file) {
    int rv;
    unsigned int i;
    size_t index;
    std::vector<std::string> results;

    index = file_name.find_last_of(".");
    std::string prefix = file_name.substr(0, index) + "_CONFLICT_";
    std::string postfix = file_name.substr(index + 1);

    rv = list_dir(target, dir_name, results);
    if (rv != 0) {
        return rv;
    }

    conflict_file.clear();
    for (i = 0; i < results.size(); i++) {
        if (results[i].find("_CONFLICT_") != std::string::npos) {
            index = results[i].find_last_of("/");
            std::string name = results[i].substr(index + 1);
            if ((rv = name.compare(0, prefix.size(), prefix)) == 0) {
                index = name.find_last_of(".");
                if ((rv = name.compare(index + 1, postfix.size(), postfix)) == 0) {
                    conflict_file.push_back(results[i]);
                }
            }
        }
    }
    return 0;
}

int is_conflict_file_existed(Target &target, const std::string & dir_name, const std::string & file_name, const int conflict_file_count, const int max_retry) {
    int rv = 0;
    int retry = 0;
    std::vector<std::string> conflict_files;
    int count = 0;

    while (retry++ < max_retry) {
        rv = find_conflict_file(target, dir_name, file_name, conflict_files);
        count = conflict_files.size();
        if (rv == 0 && count == conflict_file_count) {
            break;
        } else {
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL));
        }
    }

    if (rv != 0 || count != conflict_file_count) {
        LOG_ERROR("Found %d conflict file for %s", count, file_name.c_str());
        rv = -1;
    } else {
        rv = 0;
    }

    return rv;
}

// list_dir will list all files and directories to results vector.
int list_dir(Target &target, const std::string & path, std::vector<std::string> &results) {
    LOG_ALWAYS("List directory for client %d", target.instance_num);

    int rv = 0;

    std::deque<std::string> local_dir;
    local_dir.push_back(path);

    while(!local_dir.empty()) {
        std::string dir_path(local_dir.front());
        local_dir.pop_front();
        
        std::string absolute_path = 
            convert_path_convention(target.separator, target.work_dir + "/" + dir_path);

        rv = target.device->openDir(absolute_path.c_str());
        if(rv == VPL_ERR_NOENT){
            // dir is empty
            continue;
        } else if(rv != VPL_OK) {
            LOG_ERROR("Unable to open %s:%d", absolute_path.c_str(), rv);
            continue;
        }

        VPLFS_dirent_t folderDirent;
        while((rv = target.device->readDir(absolute_path, folderDirent)) == VPL_OK) {
            std::string dirent(folderDirent.filename);

            if (folderDirent.type == VPLFS_TYPE_FILE) {
                std::string file_name = dir_path + "/" + dirent;
                results.push_back(file_name);
                LOG_ALWAYS("\t%s", file_name.c_str());
            } else {
                if (dirent=="." || dirent==".." || dirent==".sync_temp") {
                    // We want to skip these directories
                } else {
                    // save directory for later processing
                    std::string deeper_dir = dir_path + "/" + dirent;
                    local_dir.push_back(deeper_dir);
                    results.push_back(deeper_dir);
                    LOG_ALWAYS("\t%s", deeper_dir.c_str());
                }
                continue;
            }
        }

        rv = target.device->closeDir(absolute_path);
        if(rv != VPL_OK) {
            LOG_ERROR("Closing %s:%d", absolute_path.c_str(), rv);
        }
    }
    return rv;
}

int compare_dir(Target &target, const std::string & dir_path, const std::vector<std::string> & src, const int max_retry) {
    int rv = 1;
    int i;
    std::vector<std::string> dst;
    int retry = 0;
    int src_size = src.size();
    unsigned int dst_size = 0;

    while (retry++ < max_retry && rv == 1) {
        dst.clear();
        list_dir(target, dir_path, dst);
        if (dst.size() == src_size) {
            if (src_size == 0) {
                rv = 0;
            }
            // rv should be 0 or 1 after entered this block
            for (i = 0; i < src_size; i++) {
                LOG_ALWAYS("Compare %s, %s", src[i].c_str(), dst[i].c_str());
                if ((rv = src[i].compare(dst[i])) != 0) {
                    rv = 1;
                    VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL_FOR_STRESS_TEST));
                    break;
                }
            }
        } else {
            LOG_ERROR("File number is not consistent, src %d, dst %d, retry after %ds", src_size, dst.size(), POLLING_INTERVAL_FOR_STRESS_TEST);
            if (dst.size() > dst_size) {
                retry = 0;
                dst_size = dst.size();
            }
            VPLThread_Sleep(VPLTIME_FROM_SEC(POLLING_INTERVAL_FOR_STRESS_TEST));
        }
    }

    if (rv != 0) {
        LOG_ERROR("Directory is not consistent");
        rv = -1;
    }

    return rv;
}

int get_dataset_id(Target &target, const std::string & dataset_name, u64 &dataset_id) {
    
    if (!target.alias.empty()) {
        set_target_machine(target.alias.c_str());
    } else {
        setCcdTestInstanceNum(target.instance_num);
    }

    getDatasetId(target.user_id, dataset_name, dataset_id);
    return 0;
}

int get_sync_folder(std::string & path) {
    int rv;
    u64 user_id;
    u64 device_id;
    u64 dataset_id;
    std::string response;
    std::string dataset_id_str;
    std::string syncbox_dataset_id_str;
    std::string syncbox_dataset_name;
    std::stringstream ss;

    cJSON2 *jsonResponse = NULL;
    cJSON2 *absPath = NULL; 

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id: "FMTu64"", user_id);
        return rv;
    }

    rv = getDeviceId(&device_id);
    if (rv != 0) {
        LOG_ERROR("Fail to get device id: "FMTu64"", device_id);
        return rv;
    }

    rv = getDatasetId(user_id, "Device Storage", dataset_id);
    if (rv != 0) {
        LOG_ERROR("Fail to get dataset id: "FMTu64"", device_id);
        return rv;
    }
    ss.str("");
    ss << dataset_id;
    dataset_id_str = ss.str();

    ss.str("");
    ss << "Syncbox-" << device_id;
    syncbox_dataset_name = ss.str();
    rv = getDatasetId(user_id, syncbox_dataset_name, dataset_id);
    if (rv != 0) {
        LOG_ERROR("Fail to get syncbox dataset id: "FMTu64"", device_id);
        return rv;
    }
    ss.str("");
    ss << dataset_id;
    syncbox_dataset_id_str = ss.str();

    rv = fs_test_readdirmetadata(user_id,
                                 dataset_id_str,
                                 syncbox_dataset_id_str,
                                 "",
                                 response,
                                 false);

    if (!rv) {
        jsonResponse = cJSON2_Parse(response.c_str());
        if (jsonResponse == NULL) {
            rv = -1;
        }
    }
    if (!rv) {
        absPath = cJSON2_GetObjectItem(jsonResponse, "absPath");
        if (absPath != NULL && absPath->type == cJSON2_String) {
            path = absPath->valuestring;
            LOG_ALWAYS("SyncFolder path: %s", path.c_str());
        } else {
            rv = -1;
        }
    }
    if (jsonResponse) {
        cJSON2_Delete(jsonResponse);
        jsonResponse = NULL;
    }

    return rv;
}
