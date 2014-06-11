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

#include "common_utils.hpp"
#include "autotest_common_utils.hpp"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "TargetDevice.hpp"

#define MAX_TIME_FOR_SYNC            (5 * 60) // In sec
#define POLLING_INTERVAL             1        // In sec
#define MAX_RETRY                    (MAX_TIME_FOR_SYNC / POLLING_INTERVAL)
#define POLLING_INTERVAL_FOR_STRESS_TEST      60        // In sec
#define MAX_RETRY_FOR_STRESS_TEST             (MAX_TIME_FOR_SYNC / POLLING_INTERVAL_FOR_STRESS_TEST)

static const u32 MAX_FILE_SIZE = 1024*1024*1024;
static const u32 MAX_FILE_NUM = 2000;

typedef struct target {
    std::string alias;
    int instance_num;
    bool is_cloud_pc;
    std::string separator;
    std::string work_dir;
    std::string os_version;
    std::string username;
    std::string password;
    u64 user_id;
    u64 device_id;
    TargetDevice *device;

    target(const int instance_num, const bool is_cloud_pc) :
        instance_num(instance_num),
        is_cloud_pc(is_cloud_pc) {}

    target(const std::string alias, const int instance_num, const bool is_cloud_pc) :
        alias(alias),
        instance_num(instance_num),
        is_cloud_pc(is_cloud_pc) {}

} Target;

int push_file(Target &target, const std::string & file_path, const std::string & file_name);

int rename_file(Target &target, const std::string & from, const std::string & to);

int delete_file(Target &target, const std::string & file_name);

int create_dir(Target &target, const std::string & dir_name);

int delete_dir(Target &target, const std::string & dir_name);

int compare_files(Target &target, const std::string & file_path, const std::string & file_name, const int max_retry);

int is_file_existed(Target &target, const std::string & file_name, const int max_retry);

int is_file_deleted(Target &target, const std::string & file_name, const int max_retry);

int stat_file(Target &target, VPLFS_stat_t &stat, const std::string & file_name, const int max_retry);

int touch_file(Target &target, const std::string & file_name);

int is_file_changed(Target &target, const std::string & file_name, VPLFS_stat_t stat);

int init_ccd(Target &target, const char* username, const char* password);

/// Starts CCD (intended for use after calling #suspend_ccd()).
int resume_ccd(Target &target);

// TODO: This function name is a bit misleading, as it does much more than just stop CCD.
/// Unlinks the device, stops CCD, and destroys the Target object.
void stop_ccd(Target &target);

/// Shuts down CCD without logging out the user.
int suspend_ccd(Target &target);

int link_target(Target &target);

int unlink_target(Target &target);

int enable_syncbox(Target &target, std::string path);

int disable_syncbox(Target &target);

int get_syncbox_path(Target &target, std::string &path);

int find_conflict_file(Target &target, const std::string & dir_name, const std::string & file_name, std::vector<std::string> & conflict_file);

int is_conflict_file_existed(Target &target, const std::string & dir_name, const std::string & file_name, const int conflict_file_count, const int max_retry);

int list_dir(Target &target, const std::string & path, std::vector<std::string> &dirs);

int compare_dir(Target &target, const std::string & dir_path, const std::vector<std::string> &src, const int retry);

int get_dataset_id(Target &target, const std::string & dataset_name, u64 &dataset_id);

int get_sync_folder(std::string & path);
