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

#include "syncbox_test.hpp"
#include "autotest_util.hpp"
#include "autotest_common_utils.hpp"
#include "dx_common.h"
#include <ccdi_client_tcp.hpp>
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "cr_test.hpp"
#include "dx_remote_agent.pb.h"
#include "HttpAgent.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>
#include "dx_remote_agent_util.h"
#include <gvm_file_utils.hpp>
#include "setup_stream_test.hpp"
#include "ccdconfig.hpp"
#include "TargetDevice.hpp"
#include "TargetRemoteDevice.hpp"
#include "EventQueue.hpp"
#include "clouddochttp.hpp"
#include "mca_diag.hpp"
#include "scopeguard.hpp"
#include "fs_test.hpp"
#include "vplu_sstr.hpp"
#include "TimeStreamDownload.hpp"
#include <algorithm>
#include "autotest_remotefile.hpp"
#include "vplu_mutex_autolock.hpp"
#include "syncbox_test_event.hpp"

#include "cJSON2.h"

#if defined(WIN32)
#include <comdef.h>
#endif

const char* SYNCBOX_STR = "SyncBox";

// To add new sub commands:
// 1) Add a map entry of the command string to the sub command function in syncbox_test_init_commands.
//    The function pointer to the sub command function has the type subcmd_fn
// 2) Implement the sub command function.
// 3) In the sub command function, it must implement print help
//    (Reference the usage of checkHelp below and in proc_cmd.cpp)
//    [Note: Help printouts are printed with printf() while other logs are implemented with log lib functions]

static int syncbox_test(int argc, const char* argv[]);
static int syncbox_test_file_sync(int argc, const char* argv[]);
static int syncbox_test_offline(int argc, const char* argv[]);
static int syncbox_enable(int argc, const char* argv[]);
static int syncbox_disable(int argc, const char* argv[]);
static int syncbox_getsettings(int argc, const char* argv[]);
static int syncbox_test_large_file(int argc, const char* argv[]);
static int syncbox_test_empty_file(int argc, const char* argv[]);
static int syncbox_test_file_size(int argc, const char* argv[]);
static int syncbox_test_many_files(int argc, const char* argv[]);
static int syncbox_test_file_conflict(int argc, const char* argv[]);
static int syncbox_test_sync_events(int argc, const char* argv[]);
static int syncbox_test_get_sync_states_for_paths(int argc, const char* argv[]);
static int syncbox_test_stress_test(int argc, const char* argv[]);
static int syncbox_test_remotefile(int argc, const char* argv[]);
static int syncbox_test_linking(int argc, const char* argv[]);
static int syncbox_test_longterm(int argc, const char* argv[]);

static int syncbox_get_sync_states_for_paths(int argc, const char* argv[]);
static int syncbox_monitor_sync_events(int argc, const char* argv[]);
static int syncbox_get_feature_sync_state_summary(int argc, const char* argv[]);
static int syncbox_get_sync_folder(int argc, const char* argv[]);

int init_test(const char* name, int argc, const char* argv[], std::vector<Target> &targets, int &retry);
int push_test_file(std::vector<Target> &targets, const std::string file_path, const std::string file_name, const int retry);
int create_test_dir(std::vector<Target> &targets, std::string dir_name, const int retry);
int get_random_number(int mod);

static std::map<std::string, subcmd_fn> g_syncbox_cmds;

class SyncStateExpectedVerifier
{
private:
    ccd::GetSyncStateInput input;
    ccd::GetSyncStateOutput output;
    std::vector<ccd::SyncStateType_t> exp_state_types; // input at the same time
    std::vector<bool> exp_is_sync_roots; // input at the same time

    // sequence results. input next after getting a expected output, not input at the same time.
    std::vector<std::string> seq_paths;
    std::vector<ccd::SyncStateType_t> seq_exp_state_types;
    std::vector<bool> seq_exp_is_sync_roots;
    int seq_idx;

public:
    SyncStateExpectedVerifier():seq_idx(0) {}

    void clear() {
        exp_state_types.clear();
        exp_is_sync_roots.clear();
        seq_exp_state_types.clear();
        seq_exp_is_sync_roots.clear();
        seq_paths.clear();
        input.clear_get_sync_states_for_paths();
        output.clear_sync_states_for_paths();
        seq_idx = 0;
    }

    void add_expected_result(const std::string& path,
            ccd::SyncStateType_t state_type,
            bool is_sync_root) {
        input.add_get_sync_states_for_paths(path);
        exp_state_types.push_back(state_type);
        exp_is_sync_roots.push_back(is_sync_root);
    }

    void add_seq_expected_result(const std::string& path,
            ccd::SyncStateType_t state_type,
            bool is_sync_root) {
        seq_paths.push_back(path);
        seq_exp_state_types.push_back(state_type);
        seq_exp_is_sync_roots.push_back(is_sync_root);
    }

    int wait_until_get_state(int retry) {
        int rv = -1;
        if (seq_paths.empty()) {
            LOG_ERROR("seq_paths is empty");
            return rv;
        }
        input.clear_get_sync_states_for_paths();
        input.add_get_sync_states_for_paths(seq_paths[0]);
        for (int i = 0; i < retry; i++) {
            rv = verify(true);
            if (rv == 0) {
                if (((int)seq_paths.size() - 1) > seq_idx) {
                    seq_idx++;
                    input.clear_get_sync_states_for_paths();
                    input.add_get_sync_states_for_paths(seq_paths[seq_idx]);
                } else {
                    break;
                }
            } else if (rv < 0){
                break;
            }
            VPLThread_Sleep(VPLTIME_FROM_SEC(1));
        }
        return rv;
    }

    int verify() {
        return verify(false);
    }

    int verify(bool is_seq) {
        int rv = 0;

        //output.clear_sync_states_for_paths();
        rv = CCDIGetSyncState(input, output);

        if(rv != 0) {
            LOG_ERROR("ccdigetsyncstate for syncbox fail rv %d", rv);
            rv = -1;
        } else {
            if (input.get_sync_states_for_paths_size() == output.sync_states_for_paths_size() &&
                    (is_seq ||
                     (output.sync_states_for_paths_size() == exp_state_types.size() &&
                      exp_state_types.size() == exp_is_sync_roots.size()))) {
                for (int i = 0; i < output.sync_states_for_paths_size(); i++) {
                    LOG_ALWAYS("result[%d]: input:%s, state: %d, isSyncRoot:%s",
                            i,
                            input.get_sync_states_for_paths(i).c_str(),
                            output.sync_states_for_paths(i).state(),
                            output.sync_states_for_paths(i).is_sync_folder_root()? "true":"false");
                    ccd::SyncStateType_t cur_exp_state;
                    bool cur_exp_is_root;
                    if (is_seq) {
                        cur_exp_state = seq_exp_state_types[seq_idx];
                        cur_exp_is_root = seq_exp_is_sync_roots[seq_idx];
                    } else {
                        cur_exp_state = exp_state_types[i];
                        cur_exp_is_root = exp_is_sync_roots[i];
                    }
                    if (output.sync_states_for_paths(i).state() != cur_exp_state) {
                        if (!is_seq) {
                            LOG_ERROR("Not as expected. Expected state is: %d", cur_exp_state);
                            rv = -1;
                        } else {
                            rv = 1; // return for caller to know it's able to input next one
                        }
                    }
                    if (output.sync_states_for_paths(i).is_sync_folder_root() != cur_exp_is_root) {
                        if (!is_seq) {
                            LOG_ERROR("is_sync_folder_root is not as expected. expected %s", cur_exp_is_root?"true":"false");
                            rv = -1;
                        } else {
                            rv = 1; // return for caller to know it's able to input next one
                        }
                    }
                }
            } else {
                LOG_ERROR("size of results are mismatch.");
                rv = -1;
            }
        }
        return rv;
    }
};

static int print_syncbox_test_help()
{
    int rv = 0;
    std::map<std::string, subcmd_fn>::iterator it;

    for (it = g_syncbox_cmds.begin(); it != g_syncbox_cmds.end(); ++it) {
        const char *argv[2];
        argv[0] = (const char *)it->first.c_str();
        argv[1] = "Help";
        it->second(2, argv);
    }

    return rv;
}

static void syncbox_test_init_commands()
{
    g_syncbox_cmds["Enable"]   = syncbox_enable;
    g_syncbox_cmds["Disable"]  = syncbox_disable;
    g_syncbox_cmds["GetSettings"] = syncbox_getsettings;
    g_syncbox_cmds["Test"]     = syncbox_test;
    g_syncbox_cmds["TestFileSync"] = syncbox_test_file_sync;
    g_syncbox_cmds["TestOffline"] = syncbox_test_offline;
    g_syncbox_cmds["TestLargeFile"] = syncbox_test_large_file;
    g_syncbox_cmds["TestEmptyFile"] = syncbox_test_empty_file;
    g_syncbox_cmds["TestFileSize"] = syncbox_test_file_size;
    g_syncbox_cmds["TestManyFiles"] = syncbox_test_many_files;
    g_syncbox_cmds["TestFileConflict"] = syncbox_test_file_conflict;
    g_syncbox_cmds["TestSyncEvents"] = syncbox_test_sync_events;
    g_syncbox_cmds["TestGetSyncStatesForPaths"] = syncbox_test_get_sync_states_for_paths;
    g_syncbox_cmds["TestStressTest"] = syncbox_test_stress_test;
    g_syncbox_cmds["TestRemoteFile"] = syncbox_test_remotefile;
    g_syncbox_cmds["TestLinking"] = syncbox_test_linking;
    g_syncbox_cmds["TestLongterm"] = syncbox_test_longterm;

    g_syncbox_cmds["GetSyncStatesForPaths"] = syncbox_get_sync_states_for_paths;
    g_syncbox_cmds["MonitorSyncEvents"] = syncbox_monitor_sync_events;
    g_syncbox_cmds["GetFeatureSyncStateSummary"] = syncbox_get_feature_sync_state_summary;
    g_syncbox_cmds["GetSyncFolder"] = syncbox_get_sync_folder;

}

int syncbox_test_commands(int argc, const char* argv[]) {
    int rv = 0;

    syncbox_test_init_commands();

    if (argc == 1 || checkHelp(argc, argv)) {
        print_syncbox_test_help();
        return rv;
    }

    if (g_syncbox_cmds.find(argv[1]) != g_syncbox_cmds.end()) {
        rv = g_syncbox_cmds[argv[1]](argc-1, &argv[1]);
    } else {
        LOG_ERROR("Command %s %s not supported", argv[0], argv[1]);
        rv = -1;
    }

    return rv;
}

static int syncbox_test(int argc, const char* argv[]) {
    int rv = 0;
    bool full = false;
    const char *SYNCBOX_TEST_STR = "Test";
    const char *testStr = "SyncBoxTestHarness";

    if (argc == 5 && (strcmp(argv[4], "-f") == 0 || strcmp(argv[4], "--fulltest") == 0) ) {
        full = true;
    }

    if (checkHelp(argc, argv) || (argc < 4) || (argc == 5 && !full)) {
        printf("%s %s <domain> <username> <password> [<fulltest>(-f/--fulltest)]\n", SYNCBOX_STR, argv[0]);
        goto exit;   // No arguments needed 
    }

    CHECK_AND_PRINT_EXPECTED_TO_FAIL(SYNCBOX_TEST_STR, testStr, rv, "17333");

exit:
    return rv;
}

static int syncbox_enable(int argc, const char* argv[])
{
    int rv = 0;
    int isArchive;
    u64 userId;
    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    
    if (checkHelpAndExactArgc(argc, argv, 3, rv)) {
        printf("%s %s <absolute path of syncbox root> <archive storage device? {0|1}>\n", SYNCBOX_STR, argv[0]);
        return rv;
    }
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        return rv;
    }

    isArchive = atoi(argv[2]);
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.mutable_configure_syncbox_sync()->set_enable_sync_feature(true);
    syncSettingsIn.mutable_configure_syncbox_sync()->set_set_sync_feature_path(argv[1]);
    syncSettingsIn.mutable_configure_syncbox_sync()->set_is_archive_storage((isArchive != 0));
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_sync_feature(true)"
                  " for SyncBox %s, userId="FMTu64", err=%d, path=\"%s\"",
                  rv, isArchive ? "server" : "client", userId, syncSettingsOut.configure_syncbox_sync_err(),
                  argv[1]);
    }
    return rv;
}

static int syncbox_disable(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    
    if (checkHelp(argc, argv)) {
        printf("%s %s\n", SYNCBOX_STR, argv[0]);
        return 0;   
    }
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        return rv;
    }
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.mutable_configure_syncbox_sync()->set_enable_sync_feature(false); 
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d trying to set_enable_sync_feature(false)"
                  " for SyncBox, userId="FMTu64", err=%d",
                  rv, userId, syncSettingsOut.configure_syncbox_sync_err());
    }
    return rv;
}

static int syncbox_getsettings(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    ccd::GetSyncStateInput getSyncStateIn;
    ccd::GetSyncStateOutput getSyncStateOut;

    if (checkHelp(argc, argv)) {
        printf("%s %s\n", SYNCBOX_STR, argv[0]);
        return 0;   
    }
    
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        return rv;
    }

    getSyncStateIn.set_get_syncbox_sync_settings(true);
    rv = CCDIGetSyncState(getSyncStateIn, getSyncStateOut);
    if(rv != 0) {
        LOG_ERROR("CCDIGetSyncState for syncbox fail rv %d", rv);
    } else if (getSyncStateOut.syncbox_sync_settings_size() == 0) {
        LOG_ALWAYS("Syncbox is not enabled.");
    } else {
        int i;
        for (i = 0; i < getSyncStateOut.syncbox_sync_settings_size(); i++) {
            LOG_ALWAYS("---- Settings(%d) ----", i);
            LOG_ALWAYS("Sync: %s", getSyncStateOut.syncbox_sync_settings(i).sync_feature_enabled()? "enabled":"disabled");
            LOG_ALWAYS("Path: %s", getSyncStateOut.syncbox_sync_settings(i).sync_feature_path().c_str());
            LOG_ALWAYS("IsArchiveStorage: %s", getSyncStateOut.syncbox_sync_settings(i).is_archive_storage()? "true":"false");
        }
    }
    return rv;
}

int init_test(const char* name, int argc, const char* argv[], std::vector<Target> &targets, int &retry) {
    
    const char *SYNCBOX_FOLDER = "SyncBox";

    int rv = 0;
    int device_num = 3;
    std::string domain;

    if (argc == 4) {
        retry = atoi(argv[3]);
    } else if (argc == 5) {
        domain = argv[3];
        device_num = atoi(argv[4]);
    } else if (argc == 6) {
        domain = argv[3];
        device_num = atoi(argv[4]);
        retry = atoi(argv[5]);
    }

    if (checkHelp(argc, argv) || (argc != 3 && argc != 4 && argc != 5 && argc != 6)) {
        printf("%s %s <username> <password> [<domain> <num of device>] [<retry times>]\n", SYNCBOX_STR, argv[0]);
        return -1;   // No arguments needed 
    }

    if (domain.empty()) {
        targets.push_back(Target("CloudPC", 1, true));
        targets.push_back(Target("MD", 2, false));
        targets.push_back(Target("Client", 3, false));
    } else {
        int id;
        for (int i = 0; i < device_num; i++) {
            id = i + 1;
            setCcdTestInstanceNum(id);
            const char *testStr = "SetDomain";
            const char *testArg[] = { testStr, domain.c_str() };
            rv = set_domain(2, testArg);
            CHECK_AND_PRINT_RESULT(name, testStr, rv);
            if (i == 0) {
               targets.push_back(Target(id, true));
            } else {
               targets.push_back(Target(id, false));
            }
        }
    }

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    LOG_ALWAYS("\n\n==== Starting CCD ====");
    for (int i = 0; i < device_num; i++) {
        rv = init_ccd(targets[i], argv[1], argv[2]);
        CHECK_AND_PRINT_RESULT(name, "LaunchCCD", rv);
        rv = delete_dir(targets[i], SYNCBOX_FOLDER);
        rv = create_dir(targets[i], SYNCBOX_FOLDER);
        CHECK_AND_PRINT_RESULT(name, "CreateSyncBoxFolder", rv);
        rv = enable_syncbox(targets[i], SYNCBOX_FOLDER);
        CHECK_AND_PRINT_RESULT(name, "EnableSyncBox", rv);
        get_syncbox_path(targets[i], targets[i].work_dir);
        LOG_ALWAYS("SyncBox work dir of %s is: %s", targets[i].alias.c_str(), targets[i].work_dir.c_str());
    }

exit:
    return rv;
}

int syncbox_test_file_sync(int argc, const char* argv[]) {

    const char *TEST_SYNCBOX_STR = "SyncboxFileSync";

    const int cloudpc = 0;
    const int client1 = 1;
    const int client2 = 2;

    int device_num; 
    int i;
    int rv = 0;
    int retry = MAX_RETRY;

    std::string work_dir;
    std::string xyz_name;
    std::string xyz_path;

    std::vector<Target> targets;
    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }
    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox File Sync: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "GetCurDir", rv);
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    xyz_name = "xyz.txt";
    xyz_path = work_dir + "/" + xyz_name;

    Util_rm_dash_rf(xyz_path);

    // 1. Add xyz.txt on client1
    LOG_ALWAYS("\n\n== Add xyz.txt on client1 ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_CreateDummyFile", rv);

        rv = push_file(targets[client1], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_PushFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_VerifyAddedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_VerifyAddedFile", rv, "17333");
            }
        }
    }

    // 2. Modify xyz.txt on client1
    LOG_ALWAYS("\n\n== Modify xyz.txt on client1 ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 2*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_CreateDummyFile", rv);

        rv = push_file(targets[client1], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_PushFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_VerifyModifiedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "2_VerifyModifiedFile", rv, "17333");
            }
        }
    }
    
    // 3. Rename xyz.txt to abc.txt on client1
    LOG_ALWAYS("\n\n== Rename xyz.txt to abc.txt on client1 ==");
    {
        std::string abc_name = "abc.txt";

        rv = rename_file(targets[client1], xyz_name, abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_RenameFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_VerifyRenamedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "3_VerifyRenamedFile", rv, "17333");

                rv = is_file_deleted(targets[i], xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_VerifyRemovedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "3_VerifyRemovedFile", rv, "17333");
            }
        }
    }

    // 4. Create dir xyz/ on client1
    LOG_ALWAYS("\n\n== Create dir xyz/ on client1 ==");
    {
        std::string xyz_dir_name = "xyz/";

        rv = create_dir(targets[client1], xyz_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_CreateDir", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], xyz_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_VerifyAddedDir", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_VerifyAddedDir", rv, "17333");
            }
        }
    }

    // 5. Move abc.txt to xyz/ on client1
    LOG_ALWAYS("\n\n== Move abc.txt to xyz/ on client1 ==");
    {
        std::string abc_name = "abc.txt";
        std::string abc_in_xyz_name = "xyz/abc.txt";

        rv = rename_file(targets[client1], abc_name, abc_in_xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_RenameFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], abc_in_xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_VerifyRenamedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_VerifyRenamedFile", rv, "17333");

                rv = is_file_deleted(targets[i], abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_VerifyRemovedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_VerifyRemovedFile", rv, "17333");
            }
        }
    }

    // 6. Rename xyz/ to abc/ on client1
    LOG_ALWAYS("\n\n== Rename xyz/ to abc/ on client1 ==");
    {
        std::string xyz_dir_name = "xyz/";
        std::string abc_dir_name = "abc/";
        std::string abc_in_abc_name = "abc/abc.txt";

        rv = rename_file(targets[client1], xyz_dir_name, abc_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_RenameFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], abc_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_VerifyRenamedDir", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_VerifyRenamedDir", rv, "17333");

                rv = is_file_existed(targets[i], abc_in_abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_VerifyFileExisted", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_VerifyFileExisted", rv, "17333");

                rv = is_file_deleted(targets[i], xyz_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_VerifyRemovedDir", rv);
            }
        }
    }   

    // 7. Touch abc/abc.txt on client1
    LOG_ALWAYS("\n\n== Touch abc/abc.txt on client1 ==");
    {
        VPLFS_stat_t stat;
        std::string abc_in_abc_name = "abc/abc.txt";
        std::vector<VPLFS_stat_t> orig_stat;

        for (i = 0; i < device_num; i++) {
            rv = stat_file(targets[i], stat, abc_in_abc_name, retry);
            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_StatFile", rv);
            orig_stat.push_back(stat);
        }

        rv = touch_file(targets[client1], abc_in_abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_TouchFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                int wait_time = 0;
                while (wait_time++ < 5) {
                    rv = is_file_changed(targets[i], abc_in_abc_name, orig_stat[i]);
                    if (rv != 0) {
                        // File was changed
                        rv = -1;
                        break;
                    }
                }
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_VerifyUnchangedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_VerifyUnchangedFile", rv, "17333");
            }
        }
    }

    // 8. Touch abc/ on client1
    LOG_ALWAYS("\n\n== Touch abc/ on client1 ==");
    {
        VPLFS_stat_t stat;
        std::string abc_dir_name = "abc/";
        std::vector<VPLFS_stat_t> orig_stat;

        for (i = 0; i < device_num; i++) {
            rv = stat_file(targets[i], stat, abc_dir_name, retry);
            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_StatFile", rv);
            orig_stat.push_back(stat);
        }

        rv = touch_file(targets[client1], abc_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_TouchFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                int wait_time = 0;
                while (wait_time++ < 5) {
                    rv = is_file_changed(targets[i], abc_dir_name, orig_stat[i]);
                    if (rv != 0) {
                        // File was changed
                        rv = -1;
                        break;
                    }
                }
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_VerifyUnchangedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "8_VerifyUnchangedDir", rv, "17333");
            }
        }
    }

    // 9. Delete abc/abc.txt on client1
    LOG_ALWAYS("\n\n== Delete abc/abc.txt on client1 ==");
    {
        std::string abc_in_abc_name = "abc/abc.txt";

        rv = delete_file(targets[client1], abc_in_abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_DeleteFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], abc_in_abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_VerifyDeletedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "9_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 10. Delete abc/ on client1
    LOG_ALWAYS("\n\n== Delete abc/ on client1 ==");
    {
        std::string abc_dir_name = "abc/";

        rv = delete_dir(targets[client1], abc_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "10_DeleteFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], abc_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "10_VerifyDeletedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "10_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 11. Add xyz.txt on cloudpc
    LOG_ALWAYS("\n\n== Add xyz.txt on cloudpc ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "11_CreateDummyFile", rv);

        rv = push_file(targets[cloudpc], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "11_PushFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "11_VerifyAddedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "11_VerifyAddedFile", rv, "17333");
            }
        }
    }

    // 12. Modify xyz.txt on cloudpc
    LOG_ALWAYS("\n\n== Modify xyz.txt on cloudpc ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 2*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "12_CreateDummyFile", rv);

        rv = push_file(targets[cloudpc], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "12_PushFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "12_VerifyModifiedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "12_VerifyModifiedFile", rv, "17333");
            }
        }
    }
    
    // 13. Rename xyz.txt to abc.txt on cloudpc
    LOG_ALWAYS("\n\n== Rename xyz.txt to abc.txt on cloudpc ==");
    {
        std::string abc_name = "abc.txt";

        rv = rename_file(targets[cloudpc], xyz_name, abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_RenameFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_VerifyRenamedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "13_VerifyRenamedFile", rv, "17333");

                rv = is_file_deleted(targets[i], xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_VerifyRemovedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "13_VerifyRemovedFile", rv, "17333");
            }
        }
    }

    // 4. Create dir xyz/ on cloudpc
    LOG_ALWAYS("\n\n== Create dir xyz/ on cloudpc ==");
    {
        std::string xyz_dir_name = "xyz/";

        rv = create_dir(targets[cloudpc], xyz_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "14_CreateDir", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], xyz_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "14_VerifyAddedDir", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "14_VerifyAddedDir", rv, "17333");
            }
        }
    }

    // 15. Move abc.txt to xyz/ on cloudpc
    LOG_ALWAYS("\n\n== Move abc.txt to xyz/ on cloudpc ==");
    {
        std::string abc_name = "abc.txt";
        std::string abc_in_xyz_name = "xyz/abc.txt";

        rv = rename_file(targets[cloudpc], abc_name, abc_in_xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "15_RenameFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], abc_in_xyz_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "15_VerifyRenamedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "15_VerifyRenamedFile", rv, "17333");

                rv = is_file_deleted(targets[i], abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "15_VerifyRemovedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "15_VerifyRemovedFile", rv, "17333");
            }
        }
    }

    // 16. Rename xyz/ to abc/ on cloudpc
    LOG_ALWAYS("\n\n== Rename xyz/ to abc/ on cloudpc ==");
    {
        std::string xyz_dir_name = "xyz/";
        std::string abc_dir_name = "abc/";
        std::string abc_in_abc_name = "abc/abc.txt";

        rv = rename_file(targets[cloudpc], xyz_dir_name, abc_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "16_RenameFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_existed(targets[i], abc_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "16_VerifyRenamedDir", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "16_VerifyRenamedDir", rv, "17333");

                rv = is_file_existed(targets[i], abc_in_abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "16_VerifyFileExisted", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "16_VerifyFileExisted", rv, "17333");

                rv = is_file_deleted(targets[i], xyz_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "16_VerifyRemovedDir", rv);
            }
        }
    }   

    // 17. Touch abc/abc.txt on cloudpc
    LOG_ALWAYS("\n\n== Touch abc/abc.txt on cloudpc ==");
    {
        VPLFS_stat_t stat;
        std::string abc_in_abc_name = "abc/abc.txt";
        std::vector<VPLFS_stat_t> orig_stat;

        for (i = 0; i < device_num; i++) {
            rv = stat_file(targets[i], stat, abc_in_abc_name, retry);
            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "17_StatFile", rv);
            orig_stat.push_back(stat);
        }

        rv = touch_file(targets[cloudpc], abc_in_abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "17_TouchFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                int wait_time = 0;
                while (wait_time++ < 5) {
                    rv = is_file_changed(targets[i], abc_in_abc_name, orig_stat[i]);
                    if (rv != 0) {
                        // File was changed
                        rv = -1;
                        break;
                    }
                }
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "17_VerifyUnchangedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "17_VerifyUnchangedFile", rv, "17333");
            }
        }
    }

    // 18. Touch abc/ on cloudpc
    LOG_ALWAYS("\n\n== Touch abc/ on cloudpc ==");
    {
        VPLFS_stat_t stat;
        std::string abc_dir_name = "abc/";
        std::vector<VPLFS_stat_t> orig_stat;

        for (i = 0; i < device_num; i++) {
            rv = stat_file(targets[i], stat, abc_dir_name, retry);
            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "18_StatFile", rv);
            orig_stat.push_back(stat);
        }

        rv = touch_file(targets[cloudpc], abc_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "18_TouchFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                int wait_time = 0;
                while (wait_time++ < 5) {
                    rv = is_file_changed(targets[i], abc_dir_name, orig_stat[i]);
                    if (rv != 0) {
                        // File was changed
                        rv = -1;
                        break;
                    }
                }
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "18_VerifyUnchangedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "18_VerifyUnchangedDir", rv, "17333");
            }
        }
    }

    // 19. Delete abc/abc.txt on cloudpc
    LOG_ALWAYS("\n\n== Delete abc/abc.txt on cloudpc ==");
    {
        std::string abc_in_abc_name = "abc/abc.txt";

        rv = delete_file(targets[cloudpc], abc_in_abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "19_DeleteFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], abc_in_abc_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "19_VerifyDeletedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "19_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 20. Delete abc/ on cloudpc
    LOG_ALWAYS("\n\n== Delete abc/ on cloudpc ==");
    {
        std::string abc_dir_name = "abc/";

        rv = delete_dir(targets[cloudpc], abc_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "20_DeleteFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], abc_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "20_VerifyDeletedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "20_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 21. Test the same name for directory and file
    LOG_ALWAYS("\n\n== Test the same name for directory and file ==");
    {
        std::string xyz_dir_name = "xyz";
        
        rv = create_dir(targets[client1], xyz_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "21_CreateDir", rv);
        
        rv = delete_dir(targets[client1], xyz_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "21_DeleteFile", rv);

        rv = push_file(targets[client1], xyz_path, xyz_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "21_PushFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_dir_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "21_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "21_VerifyAddedFile", rv, "18016");
            }
        }
    }

    // 22. Test file sync from syncbox folder instead of staging folder
    LOG_ALWAYS("\n\n== Test file sync from syncbox folder instead of staging folder ==");
    {
        std::string test_file = "abc22.txt";
        std::string test_dir = "abc22";
        
        rv = suspend_ccd(targets[client2]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_SuspendCCD", rv);

        rv = push_file(targets[client1], xyz_path, test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_PushFile", rv);
    
        rv = create_dir(targets[client1], test_dir);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_CreateDir", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1 && i != client2) {
                rv = compare_files(targets[i], xyz_path, test_file, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_VerifyAddedFile", rv);
                
                rv = is_file_existed(targets[i], test_dir, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_VerifyAddedDir", rv);
            }
        }

        rv = resume_ccd(targets[client2]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_ResumeCCD", rv);

        LOG_ALWAYS("Verifying [%d, %s]", targets[client2].instance_num, targets[client2].alias.c_str());
        rv = compare_files(targets[client2], xyz_path, test_file, retry);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_VerifyAddedFile", rv);
        rv = is_file_existed(targets[client2], test_dir, retry);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "22_VerifyAddedDir", rv);
    }
exit:

    // Stop CCD
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int syncbox_test_offline(int argc, const char* argv[]) {
    const char *TEST_SYNCBOX_STR = "SyncboxOfflineTest";

    const int cloudpc = 0;
    const int client1 = 1;
    const int client2 = 2;

    int device_num; 
    int i;
    int rv = 0;
    int retry = MAX_RETRY;

    std::string work_dir;
    std::string xyz_name;
    std::string xyz_path;
    std::string abc_name;
    std::string abc_path;

    std::vector<Target> targets;
    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }
    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox Offline Test: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "GetCurDir", rv);
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    xyz_name = "xyz.txt";
    xyz_path = work_dir + "/" + xyz_name;
    abc_name = "abc.txt";
    abc_path = work_dir + "/" + abc_name;

    Util_rm_dash_rf(xyz_path);
    Util_rm_dash_rf(abc_path);

    // 1. Add xyz.txt to client
    LOG_ALWAYS("\n\n== Add xyz.txt to client ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_CreateDummyFile", rv);

        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_SuspendCCD", rv);

        rv = push_file(targets[client1], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_PushFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_VerifyAddedFile", rv, "17333");
            }
        }
    }

    // 2. Add abc.txt to server
    LOG_ALWAYS("\n\n== Add abc.txt to server ==");
    {
        rv = create_dummy_file(abc_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_CreateDummyFile", rv);

        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], abc_path, abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_PushFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], abc_path, abc_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "2_VerifyAddedFile", rv, "17333");
            }
        }
    }

    // 3. Modify xyz.txt on client
    LOG_ALWAYS("\n\n== Modify xyz.txt on client ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 2*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_CreateDummyFile", rv);

        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_SuspendCCD", rv);

        rv = push_file(targets[client1], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_PushFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "3_VerifyModifiedFile", rv, "17333");
            }
        }
    }

    // 4. Modify abc.txt on server
    LOG_ALWAYS("\n\n== Modify abc.txt on server ==");
    {
        rv = create_dummy_file(abc_path.c_str(), 2*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_CreateDummyFile", rv);

        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], abc_path, abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_PushFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], abc_path, abc_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_VerifyModifiedFile", rv, "17333");
            }
        }
    }

    // 5. Delete xyz.txt on client
    LOG_ALWAYS("\n\n== Delete xyz.txt on client ==");
    {
        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_SuspendCCD", rv);

        rv = delete_file(targets[client1], xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_DeleteFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_VerifyFileDeleted", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 6. Delete abc.txt on server
    LOG_ALWAYS("\n\n== Delete abc.txt on server ==");
    {
        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_SuspendCCD", rv);

        rv = delete_file(targets[cloudpc], abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_DeleteFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], abc_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_VerifyFileDeleted", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 7. Add xyz.txt to client
    LOG_ALWAYS("\n\n== Add xyz.txt to client ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_CreateDummyFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_SuspendCCD", rv);

        rv = push_file(targets[client1], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_VerifyAddedFile", rv, "17333");
            }
        }
    }

    // 8. Add abc.txt to server
    LOG_ALWAYS("\n\n== Add abc.txt to server ==");
    {
        rv = create_dummy_file(abc_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_CreateDummyFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], abc_path, abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], abc_path, abc_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "8_VerifyAddedFile", rv, "17333");
            }
        }
    }

    // 9. Modify xyz.txt on client
    LOG_ALWAYS("\n\n== Modify xyz.txt on client ==");
    {
        rv = create_dummy_file(xyz_path.c_str(), 2*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_CreateDummyFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_SuspendCCD", rv);

        rv = push_file(targets[client1], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "9_VerifyModifiedFile", rv, "17333");
            }
        }
    }

    // 10. Modify abc.txt on server
    LOG_ALWAYS("\n\n== Modify abc.txt on server ==");
    {
        rv = create_dummy_file(abc_path.c_str(), 2*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "10_CreateDummyFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "10_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], abc_path, abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "10_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "10_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], abc_path, abc_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "10_VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "10_VerifyModifiedFile", rv, "17333");
            }
        }
    }

    // 11. Delete xyz.txt on client
    LOG_ALWAYS("\n\n== Delete xyz.txt on client ==");
    {
        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "11_SuspendCCD", rv);

        rv = delete_file(targets[client1], xyz_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "11_DeleteFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "11_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "11_VerifyFileDeleted", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "11_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 12. Delete abc.txt on server
    LOG_ALWAYS("\n\n== Delete abc.txt on client ==");
    {
        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "12_SuspendCCD", rv);

        rv = delete_file(targets[cloudpc], abc_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "12_DeleteFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "12_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != cloudpc) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = is_file_deleted(targets[i], abc_name, retry);
                //CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "12_VerifyFileDeleted", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "12_VerifyDeletedFile", rv, "17333");
            }
        }
    }

    // 13. Test the same name for directory and file
    LOG_ALWAYS("\n\n== Test the same name for directory and file ==");
    {
        std::string xyz_dir_name = "xyz";

        rv = create_test_dir(targets, xyz_dir_name, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_CreateTestDir", rv);

        rv = suspend_ccd(targets[client2]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_SuspendCCD", rv);
        
        rv = delete_dir(targets[client1], xyz_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_DeleteFile", rv);

        rv = push_file(targets[client1], xyz_path, xyz_dir_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_PushFile", rv);

        rv = resume_ccd(targets[client2]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_dir_name, retry);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "13_VerifyAddedFile", rv);
                // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "13_VerifyAddedFile", rv, "17333");
            }
        }
    }
exit:

    // Stop CCD
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int syncbox_test_large_file(int argc, const char* argv[])
{
    const char *TEST_SYNCBOX_STR = "TestLargeFile";
    const int client1 = 1;

    int device_num; 
    int i;
    int rv = 0;
    int retry = 7000;

    std::string work_dir;
    std::string file_name;
    std::string file_path;

    std::vector<Target> targets;

    if (checkHelp(argc, argv) || (argc < 6)) {
        printf("%s %s <username> <password> <domain> <devices> <retry>\n", SYNCBOX_STR, argv[0]);
        return rv; 
    }

    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox File Sync: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    if (rv < 0) {
        LOG_ERROR("Failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    file_name = "xyz.txt";
    file_path = work_dir + "/" + file_name;

    Util_rm_dash_rf(file_path);

    LOG_ALWAYS("\n\n== Create xyz.txt ==");
    {
        rv = create_dummy_file(file_path.c_str(), MAX_FILE_SIZE);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateLargeFile", rv);

        rv = push_file(targets[client1], file_path, file_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], file_path, file_name, retry);
//                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyLargeFile", rv, "17333");
            }
        }
    }

exit:

    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int syncbox_test_empty_file(int argc, const char* argv[])
{
    const char *TEST_SYNCBOX_STR = "TestEmptyFile";
    const int client1 = 1;

    int device_num; 
    int i;
    int rv = 0;
    int retry = 7000;

    std::string work_dir;
    std::string file_name;
    std::string file_path;

    std::vector<Target> targets;

    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }

    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox File Sync: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    if (rv < 0) {
        LOG_ERROR("Failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    file_name = "xyz.txt";
    file_path = work_dir + "/" + file_name;

    Util_rm_dash_rf(file_path);

    LOG_ALWAYS("\n\n== Create xyz.txt ==");
    {
        rv = create_dummy_file(file_path.c_str(), 0);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateEmptyFile", rv);

        rv = push_file(targets[client1], file_path, file_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], file_path, file_name, retry);
//                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyEmptyFile", rv, "17333");
            }
        }
    }

exit:

    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int syncbox_test_file_size(int argc, const char* argv[])
{
    const char *TEST_SYNCBOX_STR = "TestFileSize";
    const int client1 = 1;

    int device_num; 
    int i;
    int rv = 0;
    int retry = 7000;
    std::vector<u32> sizes;

    std::string work_dir;
    std::stringstream file_name;
    std::string file_path;

    std::vector<Target> targets;

    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }

    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox File Sync: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    if (rv < 0) {
        LOG_ERROR("Failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32

    sizes.push_back(0);
    sizes.push_back(1024);
    sizes.push_back(10*1024);
    sizes.push_back(100*1024);
    sizes.push_back(1024*1024);
    sizes.push_back(10*1024*1024);
    sizes.push_back(100*1024*1024);
    sizes.push_back(500*1024*1024);
    
    LOG_ALWAYS("\n\n== Create files ==");
    {
        for(std::vector<u32>::iterator it = sizes.begin(); it != sizes.end(); ++it) {

            file_name.str(std::string());
            file_name << "xyz" << *it;
            file_path = work_dir + "/" + file_name.str();
            Util_rm_dash_rf(file_path);

            rv = create_dummy_file(file_path.c_str(), *it);
            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateFile", rv);
    
            rv = push_file(targets[client1], file_path, file_name.str());
            if (rv != 0) {
                goto exit;
            }
        }
    }
    LOG_ALWAYS("\n\n== Check files ==");
    {
        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                for(std::vector<u32>::iterator it = sizes.begin(); it != sizes.end(); ++it) {

                    file_name.str(std::string());
                    file_name << "xyz" << *it;
                    file_path = work_dir + "/" + file_name.str();

                    LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                    rv = compare_files(targets[i], file_path, file_name.str(), retry);
//                  CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
                    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyAddedFile", rv, "17333");

                }                
            }
        }
    }

exit:

    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int syncbox_test_many_files(int argc, const char* argv[])
{
    const char *TEST_SYNCBOX_STR = "TestManyFiles";
    const int client1 = 1;

    int device_num; 
    int i;
    int rv = 0;
    int retry = 7000;

    std::string work_dir;
    std::stringstream file_name;
    std::string file_path;

    std::vector<Target> targets;

    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }

    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox File Sync: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    if (rv < 0) {
        LOG_ERROR("Failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    
    // TODO: Currently checking only 2000 files for time considerations. 
    // Actual system max is 65535 for Fat32, and NTFS does not have a hard limit.
    // A local test of a dataset of that size would take an estimated 3.8 hours
    LOG_ALWAYS("\n\n== Create files ==");
    {
        for(i = 0; i < MAX_FILE_NUM; i++) {

            file_name.str(std::string());
            file_name << "xyz" << i;
            file_path = work_dir + "/" + file_name.str();
            Util_rm_dash_rf(file_path);

            rv = create_dummy_file(file_path.c_str(), 0);
            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateFile", rv);
    
            rv = push_file(targets[client1], file_path, file_name.str());
            if (rv != 0) {
                goto exit;            }

        }
    }
    LOG_ALWAYS("\n\n== Check files ==");
    {
        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                for(int j = 0; j < MAX_FILE_NUM; j++) {

                    file_name.str(std::string());
                    file_name << "xyz" << j;
                    file_path = work_dir + "/" + file_name.str();

                    LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                    rv = compare_files(targets[i], file_path, file_name.str(), retry);
//                  CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
                    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyAddedFile", rv, "17333");

                }                
            }
        }
    }

exit:

    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int syncbox_test_file_conflict(int argc, const char* argv[]) {

    const char *TEST_SYNCBOX_STR = "SyncboxFileConflict";

    const int cloudpc = 0;
    const int client1 = 1;
    const int client2 = 2;

    int device_num; 
    int i;
    int rv = 0;
    int retry = MAX_RETRY;
    int expect_conflict_num;

    std::string work_dir;
    std::string xyz_name;
    std::string xyz_path;
    std::vector<std::string> temp_file;
    std::vector<std::string> conflict_file;
    std::stringstream ss;

    std::vector<Target> targets;
    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }
    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox File Conflict: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "GetCurDir", rv);
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    xyz_name = "xyz.txt";
    xyz_path = work_dir + "/" + xyz_name;
    rv = create_dummy_file(xyz_path.c_str(), 1 * 1024);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);

    for (i = 0; i < device_num; i++) {
        ss.str("");
        ss << "temp" << i;
        std::string file_path = work_dir + "/" + ss.str();
        VPLFS_file_size_t size = 1 * 1024 + (i + 1);
        rv = create_dummy_file(file_path.c_str(), size);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);
        temp_file.push_back(file_path);
    }

    // 1. Offline client1; Modify xyz.txt in client B and server A 
    LOG_ALWAYS("\n\n== Offline client1; Modify xyz.txt in client B and server A ==");
    {
        expect_conflict_num = 1;

        std::string test_dir = "test1";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_PushTestFile", rv);

        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], temp_file[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_PushFile", rv);

        rv = push_file(targets[client1], temp_file[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_PushFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, expect_conflict_num, retry);
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_VerifyConflictFileExisted", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_VerifyConflictFileExisted", rv, "17333");
        }

        rv = find_conflict_file(targets[cloudpc], test_dir, xyz_name, conflict_file);
        // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_GetConflictFilePath", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_GetConflictFilePath", rv, "17333");
        
        if (conflict_file.size() != expect_conflict_num) {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_ConflictFileNumNotMatch", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_ConflictFileNumNotMatch", rv, "17333");
        }

        // The origin file is indeterminate, could be A or B.
        if ((rv = compare_files(targets[cloudpc], temp_file[cloudpc], test_file, 1)) == 0) {
            LOG_ALWAYS("CloudPC POST first");
            rv = compare_files(targets[cloudpc], temp_file[client1], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_CompareConflictFileContent", rv, "17333");
        } else if ((rv = compare_files(targets[cloudpc], temp_file[client1], test_file, 1)) == 0) {
            LOG_ALWAYS("Client1 POST first");
            rv = compare_files(targets[cloudpc], temp_file[cloudpc], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_CompareConflictFileContent", rv, "17333");
        } else {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "1_CompareFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "1_CompareFileContent", rv, "17333");
        }
    }

    // 2. Offline client1; Modify xyz.txt in client B and delete xyz.txt in server A 
    LOG_ALWAYS("\n\n== Offline client1; Modify xyz.txt in client B and delete xyz.txt in server A ==");
    {
        expect_conflict_num = 0;

        std::string test_dir = "test2";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_PushTestFile", rv);

        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_SuspendCCD", rv);

        rv = delete_file(targets[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_DeleteFile", rv);

        rv = push_file(targets[client1], temp_file[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_PushFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = compare_files(targets[i], temp_file[client1], test_file, retry);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_VerifyModifiedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "2_VerifyModifiedFile", rv, "17333");
        }

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = find_conflict_file(targets[i], test_dir, xyz_name, conflict_file);
            if (rv != expect_conflict_num) {
                rv = -1;
            }
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "2_VerifyConflictFileShouldNotAppear", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "2_VerifyConflictFileShouldNotAppear", rv, "17333");
        }
    }

    // 3. Offline client1; Delete xyz.txt in client B and modify xyz.txt in server A 
    LOG_ALWAYS("\n\n== Offline client1; Delete xyz.txt in client B and Modify xyz.txt in server A ==");
    {
        expect_conflict_num = 0;

        std::string test_dir = "test3";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_PushTestFile", rv);

        rv = suspend_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], temp_file[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_PushFile", rv);

        rv = delete_file(targets[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_DeleteFile", rv);

        rv = resume_ccd(targets[client1]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = compare_files(targets[i], temp_file[cloudpc], test_file, retry);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_VerifyModifiedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "3_VerifyModifiedFile", rv, "17333");
        }

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = find_conflict_file(targets[i], test_dir, xyz_name, conflict_file);
            if (rv != expect_conflict_num) {
                rv = -1;
            }
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "3_VerifyConflictFileShouldNotAppear", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "3_VerifyConflictFileShouldNotAppear", rv, "17333");
        }
    }
    
    // 4. Offline cloudpc; Modify xyz.txt in client B and server A 
    LOG_ALWAYS("\n\n== Offline cloudpc; Modify xyz.txt in client B and server A ==");
    {
        expect_conflict_num = 1;

        std::string test_dir = "test4";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_PushTestFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], temp_file[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_PushFile", rv);

        rv = push_file(targets[client1], temp_file[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, expect_conflict_num, retry);
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_VerifyConflictFileExisted", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_VerifyConflictFileExisted", rv, "17333");
        }
        
        rv = find_conflict_file(targets[cloudpc], test_dir, xyz_name, conflict_file);
        // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_GetConflictFilePath", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_GetConflictFilePath", rv, "17333");
        
        if (conflict_file.size() != expect_conflict_num) {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_ConflictFileNumNotMatch", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_ConflictFileNumNotMatch", rv, "17333");
        }

        // The origin file is indeterminate, could be A or B.
        if ((rv = compare_files(targets[cloudpc], temp_file[cloudpc], test_file, 1)) == 0) {
            LOG_ALWAYS("CloudPC POST first");
            rv = compare_files(targets[cloudpc], temp_file[client1], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_CompareConflictFileContent", rv, "17333");
        } else if ((rv = compare_files(targets[cloudpc], temp_file[client1], test_file, 1)) == 0) {
            LOG_ALWAYS("Client1 POST first");
            rv = compare_files(targets[cloudpc], temp_file[cloudpc], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_CompareConflictFileContent", rv, "17333");
        } else {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "4_CompareFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "4_CompareFileContent", rv, "17333");
        }
    }

    // 5. Offline cloudpc; Modify xyz.txt in client B and client C
    LOG_ALWAYS("\n\n== Offline cloudpc; Modify xyz.txt in client B and client C ==");
    {
        expect_conflict_num = 1;

        std::string test_dir = "test5";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_PushTestFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_SuspendCCD", rv);

        rv = push_file(targets[client1], temp_file[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_PushFile", rv);

        rv = push_file(targets[client2], temp_file[client2], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, expect_conflict_num, retry);
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_VerifyConflictFileExisted", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_VerifyConflictFileExisted", rv, "17333");
        }
        
        rv = find_conflict_file(targets[cloudpc], test_dir, xyz_name, conflict_file);
        // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_GetConflictFilePath", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_GetConflictFilePath", rv, "17333");
        
        if (conflict_file.size() != expect_conflict_num) {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_ConflictFileNumNotMatch", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_ConflictFileNumNotMatch", rv, "17333");
        }

        // The origin file is indeterminate, could be B or C.
        if ((rv = compare_files(targets[cloudpc], temp_file[client1], test_file, 1)) == 0) {
            LOG_ALWAYS("Client1 POST first");
            rv = compare_files(targets[cloudpc], temp_file[client2], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_CompareConflictFileContent", rv, "17333");
        } else if ((rv = compare_files(targets[cloudpc], temp_file[client2], test_file, 1)) == 0) {
            LOG_ALWAYS("Client2 POST first");
            rv = compare_files(targets[cloudpc], temp_file[client1], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_CompareConflictFileContent", rv, "17333");
        } else {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "5_CompareFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "5_CompareFileContent", rv, "17333");
        }
    }

    // 6. Offline cloudpc; Modify xyz.txt in client B, client C and server A
    LOG_ALWAYS("\n\n== Offline cloudpc; Modify xyz.txt in client B, client C and server A ==");
    {
        expect_conflict_num = 2;

        std::string test_dir = "test6";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_PushTestFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], temp_file[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_PushFile", rv);

        rv = push_file(targets[client1], temp_file[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_PushFile", rv);

        rv = push_file(targets[client2], temp_file[client2], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, expect_conflict_num, retry);
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_VerifyConflictFileExisted", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_VerifyConflictFileExisted", rv, "17333");
        }

        rv = find_conflict_file(targets[cloudpc], test_dir, xyz_name, conflict_file);
        // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_GetConflictFilePath", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_GetConflictFilePath", rv, "17333");
        
        if (conflict_file.size() != expect_conflict_num) {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_ConflictFileNumNotMatch", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_ConflictFileNumNotMatch", rv, "17333");
        }

        // The origin file is indeterminate, could be A, B or C.
        for (i = cloudpc; i <= client2; i++) {
            if ((rv = compare_files(targets[cloudpc], temp_file[i], test_file, 1)) == 0) {
                for (int j = cloudpc; j <= client2; j++) {
                    if (j != i) {
                        if ((rv = compare_files(targets[cloudpc], temp_file[j], conflict_file[0], 1)) == 0) {
                            for (int k = cloudpc; k <= client2; k++) {
                                if (k != i && k != j) {
                                    rv = compare_files(targets[cloudpc], temp_file[k], conflict_file[1], 1);
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
        // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_CompareConflictFileContent", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_CompareConflictFileContent", rv, "17333");
    }

    // 7. Offline cloudpc; Modify xyz.txt in client B, client C and delete xyz.txt in server A 
    LOG_ALWAYS("\n\n== Offline cloudpc; Modify xyz.txt in client B, client C and delete xyz.txt in server A ==");
    {
        expect_conflict_num = 1;

        std::string test_dir = "test7";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_PushTestFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_SuspendCCD", rv);

        rv = delete_file(targets[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_DeleteFile", rv);

        rv = push_file(targets[client1], temp_file[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_PushFile", rv);

        rv = push_file(targets[client2], temp_file[client2], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, expect_conflict_num, retry);
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_VerifyConflictFileExisted", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_VerifyConflictFileExisted", rv, "17333");
        }

        rv = find_conflict_file(targets[cloudpc], test_dir, xyz_name, conflict_file);
        // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_GetConflictFilePath", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_GetConflictFilePath", rv, "17333");
        
        if (conflict_file.size() != expect_conflict_num) {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_ConflictFileNumNotMatch", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_ConflictFileNumNotMatch", rv, "17333");
        }

        // The origin file is indeterminate, could be B or C.
        if ((rv = compare_files(targets[cloudpc], temp_file[client1], test_file, 1)) == 0) {
            LOG_ALWAYS("Client1 POST first");
            rv = compare_files(targets[cloudpc], temp_file[client2], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_CompareConflictFileContent", rv, "17333");
        } else if ((rv = compare_files(targets[cloudpc], temp_file[client2], test_file, 1)) == 0) {
            LOG_ALWAYS("Client2 POST first");
            rv = compare_files(targets[cloudpc], temp_file[client1], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_CompareConflictFileContent", rv, "17333");
        } else {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "7_CompareFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "7_CompareFileContent", rv, "17333");
        }
    }

    // 8. Offline cloudpc; Delete xyz.txt in client B and modify xyz.txt in client C and server A 
    LOG_ALWAYS("\n\n== Offline cloudpc; Delete xyz.txt in client B and modify xyz.txt in client C and server A ==");
    {
        expect_conflict_num = 1;

        std::string test_dir = "test8";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_PushTestFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], temp_file[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_PushFile", rv);

        rv = delete_file(targets[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_DeleteFile", rv);

        rv = push_file(targets[client2], temp_file[client2], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, expect_conflict_num, retry);
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_VerifyConflictFileExisted", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "8_VerifyConflictFileExisted", rv, "17333");
        }

        rv = find_conflict_file(targets[cloudpc], test_dir, xyz_name, conflict_file);
        // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_GetConflictFilePath", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_GetConflictFilePath", rv, "17333");
        
        if (conflict_file.size() != expect_conflict_num) {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "6_ConflictFileNumNotMatch", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "6_ConflictFileNumNotMatch", rv, "17333");
        }

        // The origin file is indeterminate, could be A or C.
        if ((rv = compare_files(targets[cloudpc], temp_file[cloudpc], test_file, 1)) == 0) {
            LOG_ALWAYS("CloudPC POST first");
            rv = compare_files(targets[cloudpc], temp_file[client2], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "8_CompareConflictFileContent", rv, "17333");
        } else if ((rv = compare_files(targets[cloudpc], temp_file[client2], test_file, 1)) == 0) {
            LOG_ALWAYS("Client2 POST first");
            rv = compare_files(targets[cloudpc], temp_file[cloudpc], conflict_file[0], 1);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_CompareConflictFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "8_CompareConflictFileContent", rv, "17333");
        } else {
            rv = -1;
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "8_CompareFileContent", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "8_CompareFileContent", rv, "17333");
        }
    }

    // 9. Offline cloudpc; Delete xyz.txt in client B, client C and modify xyz.txt server A 
    LOG_ALWAYS("\n\n== Offline cloudpc; Delete xyz.txt in client B, client C and modify xyz.txt server A ==");
    {
        expect_conflict_num = 0;

        std::string test_dir = "test9";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_PushTestFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_SuspendCCD", rv);

        rv = push_file(targets[cloudpc], temp_file[cloudpc], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_PushFile", rv);

        rv = delete_file(targets[client1], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_DeleteFile", rv);

        rv = delete_file(targets[client2], test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_DeleteFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = compare_files(targets[i], temp_file[cloudpc], test_file, retry);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_VerifyModifiedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "9_VerifyModifiedFile", rv, "17333");
        }

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = find_conflict_file(targets[i], test_dir, xyz_name, conflict_file);
            if (rv != expect_conflict_num) {
                rv = -1;
            }
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "9_VerifyConflictFileShouldNotAppear", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "9_VerifyConflictFileShouldNotAppear", rv, "17333");
        }
    }

exit:

    // Stop CCD
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int push_test_file(std::vector<Target> &targets, const std::string file_path, const std::string file_name, const int retry) {
    int rv = 0;
    int device_num;

    rv = push_file(targets[0], file_path, file_name);
    if (rv != 0) {
        goto exit;
    }

    device_num = targets.size();
    for (int i = 1; i < device_num; i++) {
        rv = is_file_existed(targets[i], file_name, retry);
        if (rv != 0) {
            goto exit;
        }
    }
exit:
    return rv;
}

int create_test_dir(std::vector<Target> &targets, std::string dir_name, const int retry) {
    int rv = 0;
    int device_num;

    rv = create_dir(targets[0], dir_name);
    if (rv != 0) {
        goto exit;
    }

    device_num = targets.size();
    for (int i = 1; i < device_num; i++) {
        rv = is_file_existed(targets[i], dir_name, retry);
        if (rv != 0) {
            goto exit;
        }
    }
exit:
    return rv;
}

static int syncbox_get_feature_sync_state_summary(int argc, const char* argv[])
{
    int rv = 0;
    if (checkHelp(argc, argv)) {
        printf("%s %s\n", SYNCBOX_STR, argv[0]);
        return 0;
    }

    ccd::GetSyncStateInput getSyncStateIn;
    ccd::GetSyncStateOutput getSyncStateOut;
    getSyncStateIn.add_get_sync_states_for_features(ccd::SYNC_FEATURE_SYNCBOX);

    rv = CCDIGetSyncState(getSyncStateIn, getSyncStateOut);
    if(rv != 0) {
        LOG_ERROR("CCDIGetSyncState for get_sync_states_for_features "
                "(SYNC_FEATURE_SYNCBOX) fail rv %d", rv);
    } else {
        int i;
        for (i = 0; i < getSyncStateOut.feature_sync_state_summary_size(); i++) {
            LOG_ALWAYS("---- feature_sync_state_summary(%d) ----\n%s",
                    i, getSyncStateOut.feature_sync_state_summary(i).DebugString().c_str());
        }
    }
    return rv;
}

static int syncbox_get_sync_states_for_paths(int argc, const char* argv[])
{
    int rv = 0;
    u64 userId;
    ccd::GetSyncStateInput getSyncStateIn;
    ccd::GetSyncStateOutput getSyncStateOut;

    if (checkHelp(argc, argv)) {
        printf("%s %s %s %s\n", SYNCBOX_STR, argv[0],
                "paths1", "[path2] [path3]...");
        return 0;
    }

    if (argc < 2) {
        LOG_ERROR("Please specify paths");
    }

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id");
        return rv;
    }

    for (int i = 1; i < argc; i++) {
        LOG_ALWAYS("path:%s", argv[i]);
        getSyncStateIn.add_get_sync_states_for_paths(argv[i]);

    }
    rv = CCDIGetSyncState(getSyncStateIn, getSyncStateOut);
    if(rv != 0) {
        LOG_ERROR("CCDIGetSyncState for syncbox fail rv %d", rv);
    } else {
        int i;
        for (i = 0; i < getSyncStateOut.sync_states_for_paths_size(); i++) {
            LOG_ALWAYS("---- Sync_states_for_paths(%d) ----\n%s",
                    i, getSyncStateOut.sync_states_for_paths(i).DebugString().c_str());
        }
    }
    return rv;
}

static int check_sync_event_visitor(const ccd::CcdiEvent &_event, void *_ctx)
{
    if (_event.has_sync_history() &&
            _event.sync_history().feature() == ccd::SYNC_FEATURE_SYNCBOX) {
        LOG_ALWAYS("Got EventSyncHistory. Type:%d, path:%s,"
                " dataset_id:"FMTu64", event_time:"FMTu64", conflict_file_original_path:%s",
                _event.sync_history().type(),
                _event.sync_history().path().c_str(),
                _event.sync_history().dataset_id(),
                _event.sync_history().event_time(),
                _event.sync_history().conflict_file_original_path().c_str());
    }
    if (_event.has_sync_feature_status_change() &&
        _event.sync_feature_status_change().feature() == ccd::SYNC_FEATURE_SYNCBOX) {
                LOG_ALWAYS("Got EventSyncFeatureStatusChange(status,UpRmn,"
                        "DlRmn,RmtScnPnd,ScnInPgrs):"
                        " (%d, %d, %d, %d, %d)",
                _event.sync_feature_status_change().status().status(),
                _event.sync_feature_status_change().status().uploads_remaining(),
                _event.sync_feature_status_change().status().downloads_remaining(),
                _event.sync_feature_status_change().status().remote_scan_pending(),
                _event.sync_feature_status_change().status().scan_in_progress());
    }
    return 0;
}

// Monitor events:
// 1. EventSyncHistory (only SYNC_FEATURE_SYNCBOX)
// 2. sync_feature_status_change (only SYNC_FEATURE_SYNCBOX)
static int syncbox_monitor_sync_events(int argc, const char* argv[])
{
    int rv = 0;

    int wait_sec = 1000;
    if (checkHelp(argc, argv) ) {
        printf("%s %s [monitor_seconds(default=%d)]\n", SYNCBOX_STR, argv[0], wait_sec);
        return 0;
    }
    if (argc >= 2) {
        wait_sec = atoi(argv[1]);
    }
    EventQueue eq;

    LOG_ALWAYS("\n\n== Monitoring for SyncBox EventSyncHistory "
            "and sync_feature_status_change for %d seconds ==", wait_sec);
    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end = start + VPLTime_FromSec(wait_sec);
    VPLTime_t now;
    u64 deviceId = 0;
    while((now = VPLTime_GetTimeStamp()) < end) {
        rv = getDeviceId(&deviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d. make sure you setup CCD correctly.", rv);
            break;
        }
        eq.visit(0, (s32)(VPLTime_ToMillisec(VPLTime_DiffClamp(end, now))),
                check_sync_event_visitor, NULL);
    }

    return 0;
}


static int check_test_sync_event_visitor(const ccd::CcdiEvent &_event, void *arg)
{ // Under ctx lock
    syncbox_event_visitor_ctx *_ctx = (syncbox_event_visitor_ctx*)arg;
    MutexAutoLock lock(&(_ctx->ctx_mutext));

    struct syncbox_expected_sync_events* exp_list = &_ctx->expected_sync_event_list;
    if (_ctx->consume_all) {
        // Consume all events, just return to get next event.
        return 0;
    }
    if (_event.has_sync_history() &&
            _event.sync_history().feature() == ccd::SYNC_FEATURE_SYNCBOX) {
        LOG_ALWAYS("Got EventSyncHistory. Type:%d, path:%s,"
                " dataset_id:"FMTu64", event_time:"FMTu64", conflict_file_original_path:%s",
                _event.sync_history().type(),
                _event.sync_history().path().c_str(),
                _event.sync_history().dataset_id(),
                _event.sync_history().event_time(),
                _event.sync_history().conflict_file_original_path().c_str());
        if (_ctx->consume_all) return 0;
        if (!exp_list->sync_event_type.empty()) {
            // Expected as a sync_history
            if (exp_list->sync_event_type.front() == _event.sync_history().type()) {
                // TODO: Bug 17333: Also check the correctness of the path and conflict_file_original_path
                LOG_ALWAYS("EventSyncHistory is as expected");
                exp_list->sync_event_type.erase(exp_list->sync_event_type.begin());
            } else {
                LOG_ERROR("Got EventSyncHistory type:%d, expected type is:%d",
                        _event.sync_history().type(),
                        exp_list->sync_event_type.front());
                _ctx->result = -1;
            }
        } else {
            LOG_ERROR("Got EventSyncHistory, but expected result is not a EventSyncHistory");
            _ctx->result = -1;
        }
    }
    if (_ctx->result == 0 && _event.has_sync_feature_status_change() &&
        _event.sync_feature_status_change().feature() == ccd::SYNC_FEATURE_SYNCBOX) {
        ccd::FeatureSyncStateSummary event_status = _event.sync_feature_status_change().status();
        LOG_ALWAYS("Got EventSyncFeatureStatusChange(status,UpRmn,"
                "DlRmn,RmtScnPnd,ScnInPgrs):"
                "(%d, %d, %d, %d, %d)",
                event_status.has_status()?event_status.status():-1,
                event_status.has_uploads_remaining()?event_status.uploads_remaining():-1,
                event_status.has_downloads_remaining()?event_status.downloads_remaining():-1,
                event_status.has_remote_scan_pending()?event_status.remote_scan_pending():-1,
                event_status.has_scan_in_progress()?event_status.scan_in_progress():-1);

        if (event_status.has_status() && event_status.has_uploads_remaining() &&
                event_status.has_downloads_remaining() && event_status.has_remote_scan_pending() &&
                event_status.has_scan_in_progress()) {
            if (!exp_list->status.empty() &&
                    exp_list->status.front() == event_status.status()) {
                exp_list->status.erase(exp_list->status.begin());
            }
            if (!exp_list->uploads_remaining.empty() &&
                    exp_list->uploads_remaining.front() == event_status.uploads_remaining()) {
                exp_list->uploads_remaining.erase(exp_list->uploads_remaining.begin());
            }
            if (!exp_list->downloads_remaining.empty() &&
                    exp_list->downloads_remaining.front() == event_status.downloads_remaining()) {
                exp_list->downloads_remaining.erase(exp_list->downloads_remaining.begin());
            }
            if (!exp_list->remote_scan_pending.empty() &&
                    exp_list->remote_scan_pending.front() == event_status.remote_scan_pending()) {
                exp_list->remote_scan_pending.erase(exp_list->remote_scan_pending.begin());
            }
            if (!exp_list->scan_in_progress.empty() &&
                    exp_list->scan_in_progress.front() == event_status.scan_in_progress()) {
                exp_list->scan_in_progress.erase(exp_list->scan_in_progress.begin());
            }
        }
        LOG_ALWAYS("Remaining expected results(sync_event_type,status,UpRmn,"
                "DlRmn,RmtScnPnd,ScnInPgrs):("FMTu_size_t", "FMTu_size_t
                ", "FMTu_size_t", "FMTu_size_t", "FMTu_size_t", "FMTu_size_t")",
                exp_list->sync_event_type.size(),
                exp_list->status.size(),
                exp_list->uploads_remaining.size(),
                exp_list->downloads_remaining.size(),
                exp_list->remote_scan_pending.size(),
                exp_list->scan_in_progress.size()
                );
        if (exp_list->status.empty()) {
            if( exp_list->uploads_remaining.empty() &&
                    exp_list->downloads_remaining.empty() &&
                    exp_list->remote_scan_pending.empty() &&
                    exp_list->scan_in_progress.empty()) {
                // We don't check scan_in_progress here because sometimes it
                // will get extra event from local file change because
                // of time difference, it's a normal behavior so skip to count it.
                // Just check if this flag set and unset
                if (event_status.status() == 1 &&
                        event_status.uploads_remaining() == 0 &&
                        event_status.downloads_remaining() == 0 &&
                        event_status.remote_scan_pending() == 0) {
                    LOG_ALWAYS("All expected event met. Pass.");
                    _ctx->check_extra_event = true;
                } else {
                    LOG_ERROR("All expected event consumed."
                            " But event sequence is still not finish yet.");
                    _ctx->result = -1;
                }
            }
        }
    }

    return 0;
}

static VPLThread_return_t listen_syncbox_events(VPLThread_arg_t arg)
{
    syncbox_event_visitor_ctx *ctx = (syncbox_event_visitor_ctx*)arg;
    if (ctx == NULL) {
        LOG_ERROR("syncbox_event_visitor_ctx is NULL!");
        return (VPLThread_return_t)0;
    }

    { // Under ctx lock
        MutexAutoLock lock(&ctx->ctx_mutext);
        if (ctx->is_run_from_remote) {
            int rv = set_target_machine(ctx->alias.c_str());
            if (rv != 0) {
                LOG_ERROR("Unable to set target machine %s, rv = %d", ctx->alias.c_str(), rv);
                return (VPLThread_return_t)0;
            }
        } else {
            setCcdTestInstanceNum(ctx->instance_num);
        }
    }

    EventQueue eq;

    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end = start + VPLTime_FromSec(ctx->timeout_sec);
    VPLTime_t now;
    bool is_check_extra_event = false;
    int check_extra_event_timeout = 3 * 1000;

    ctx->count = 0;

    VPLSem_Post(&(ctx->sem));

    while ((now = VPLTime_GetTimeStamp()) < end) {
        s32 timeout;
        { // Under ctx lock
            MutexAutoLock lock(&ctx->ctx_mutext);
            if (!is_check_extra_event && ctx->check_extra_event) {
                timeout = check_extra_event_timeout;
                end = now + VPLTime_FromMillisec(check_extra_event_timeout);
                LOG_ALWAYS("Wait extra %d ms to see if there are any extra unexpected events.", timeout);
                is_check_extra_event = true;
            } else {
                timeout = (s32)VPLTime_ToMillisec(VPLTime_DiffClamp(end, now));
            }
        }
        int rc = eq.visit(0, timeout, check_test_sync_event_visitor, (void*)ctx);
        if (rc != 0) {
            LOG_ERROR("EventQueue visit fail:%d", rc);
            break;
        }
        { // Under ctx lock
            MutexAutoLock lock(&ctx->ctx_mutext);
            if (ctx->result != 0) {
                break;
            }
        }
    }
    { // Under ctx lock
        MutexAutoLock lock(&ctx->ctx_mutext);
        if (ctx->result == 0 && is_check_extra_event == true) {
            LOG_ALWAYS("No suspicious event found while checking extra event. Pass.");
            ctx->result = 1;
        }
        if (ctx->result == 0 && !ctx->consume_all) {
            LOG_ERROR("check_test_sync_event_visitor didn't complete within %d seconds", ctx->timeout_sec);
        } else if (ctx->result < 0) {
            LOG_ERROR("check_test_sync_event_visitor got error:%d", ctx->result);
        }
    }

    return (VPLThread_return_t)0;

}

static int wait_event_thread_done(syncbox_event_visitor_ctx& ctx)
{
    int rv = 0;

    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end;
    VPLTime_t now;
    { // Under ctx lock
        MutexAutoLock lock(&ctx.ctx_mutext);
        end = start + VPLTime_FromSec(ctx.timeout_sec);
    }
    while ((now = VPLTime_GetTimeStamp()) < end) {
        VPLThread_Sleep(VPLTIME_FROM_SEC(1));
        { // Under ctx lock
            MutexAutoLock lock(&ctx.ctx_mutext);
            if (ctx.result != 0) {
                break;
            }
        }
    }
    { // Under ctx lock
        MutexAutoLock lock(&ctx.ctx_mutext);
        if (ctx.result == 0 && !ctx.consume_all) {
            LOG_ERROR("event thread didn't complete within %d seconds", ctx.timeout_sec);
            rv = -1;
        } else if (ctx.result < 0) {
            // We got some error in thread
            LOG_ERROR("Got error in events thread:%d", ctx.result);
            rv = ctx.result;
        }
    }
    LOG_ALWAYS("wait_event_thread_done finished");
    return rv;
}

static int setup_event_context_and_start_listener(int instance_num,
        VPLThread_t* listener_thread, struct syncbox_event_visitor_ctx& ctx,
        ccd::SyncEventType_t type, Expected_SyncEvnetSequence expected_seq)
{
    int rv = 0;
    ctx.result = 0;
    ctx.alias = "MD";
    ctx.instance_num = instance_num;
    ctx.consume_all = false;
    ctx.check_extra_event = false;
    ctx.timeout_sec = MAX_TIME_FOR_SYNC;
    struct syncbox_expected_sync_events expected_event;

    switch (expected_seq)
    {
        case SYNC_EVENT_SEQ_SERVER_UP_FILE_CLIENT_LISTEN:
        case SYNC_EVENT_SEQ_SERVER_DEL_CLIENT_LISTEN:
        case SYNC_EVENT_SEQ_SERVER_UP_DIR_CLIENT_LISTEN:
        {
            int  st[]   =   {1, 2, 1}; // IN_SYNC; SYNC; IN_SYNC
            int  up[]   =   {0}; // deleting, folder will not show in download_remaining
            int  dl[]   =   {0};
            bool remote[] = {1, 0}; // got remote event once as a start
            bool scan[] =   {0, 1, 0}; // due to local file scan may vary,
                                       // just check if working but not counting

            expected_event.sync_event_type.push_back(type);

            expected_event.status.assign(st, st + sizeof(st)/sizeof(*st));
            expected_event.uploads_remaining.assign(up, up + sizeof(up)/sizeof(*up));
            expected_event.downloads_remaining.assign(dl, dl + sizeof(dl)/sizeof(*dl));
            expected_event.remote_scan_pending.assign(remote, remote + sizeof(remote)/sizeof(*remote));
            expected_event.scan_in_progress.assign(scan, scan + sizeof(scan)/sizeof(*scan));
            if (SYNC_EVENT_SEQ_SERVER_UP_FILE_CLIENT_LISTEN) {
                int  dl[]   =   {0, 1, 0}; // download one entry
                expected_event.downloads_remaining.assign(dl, dl + sizeof(dl)/sizeof(*dl));
            }
        }
            break;
        case SYNC_EVENT_SEQ_CLIENT_UP_FILE_CLIENT_LISTEN:
        case SYNC_EVENT_SEQ_CLIENT_UP_DIR_CLIENT_LISTEN:
        case SYNC_EVENT_SEQ_CLIENT_DEL_CLIENT_LISTEN:
        {
            int  st[]   =   {1, 2, 1}; // IN_SYNC; SYNC; IN_SYNC
            int  up[]   =   {0, 1, 0}; // upload one entry
            int  dl[]   =   {0};
            bool remote[] = {0, 1, 0}; // got remote event once after upload
            bool scan[] =   {1, 0}; // due to local file scan may vary,
                                    // just check if working but not counting.
                                    // 1st event is 1 because local file change

            expected_event.sync_event_type.push_back(type);

            expected_event.status.assign(st, st + sizeof(st)/sizeof(*st));
            expected_event.uploads_remaining.assign(up, up + sizeof(up)/sizeof(*up));
            expected_event.downloads_remaining.assign(dl, dl + sizeof(dl)/sizeof(*dl));
            expected_event.remote_scan_pending.assign(remote, remote + sizeof(remote)/sizeof(*remote));
            expected_event.scan_in_progress.assign(scan, scan + sizeof(scan)/sizeof(*scan));
        }
            break;
        case SYNC_EVENT_SEQ_CLIENT_CONFLICT_CLIENT_LISTEN:
        {
            ccd::SyncEventType_t stype[] = {ccd::CCD_SYNC_EVENT_CONFLICT_FILE_CREATED,
                                           ccd::CCD_SYNC_EVENT_MODIFIED_FILE_DOWNLOADED,
                                           ccd::CCD_SYNC_EVENT_NEW_FILE_UPLOADED};
            int  st[]   =   {1, 2, 1, 2, 1}; // 1st SYNC is for file download; 2nd SYNC is for new conflict file upload
            int  up[]   =   {0, 1, 0, 1, 0}; // 1st up is local file but fail; 2nd up is new generated conflict file
            int  dl[]   =   {0, 1, 0}; // download one entry
            bool remote[] = {0, 1, 0}; // got remote event once after upload
            bool scan[] =   {1, 0}; // due to local file scan may vary,
                                    // just check if working but not counting.
                                    // 1st event is 1 because local file change

            expected_event.sync_event_type.assign(stype, stype + sizeof(stype)/sizeof(*stype));
            expected_event.status.assign(st, st + sizeof(st)/sizeof(*st));
            expected_event.uploads_remaining.assign(up, up + sizeof(up)/sizeof(*up));
            expected_event.downloads_remaining.assign(dl, dl + sizeof(dl)/sizeof(*dl));
            expected_event.remote_scan_pending.assign(remote, remote + sizeof(remote)/sizeof(*remote));
            expected_event.scan_in_progress.assign(scan, scan + sizeof(scan)/sizeof(*scan));
        }
            break;
        // No test case yet
        case SYNC_EVENT_SEQ_SERVER_UP_FILE_SERVER_LISTEN:
        case SYNC_EVENT_SEQ_CLIENT_UP_FILE_SERVER_LISTEN:
        case SYNC_EVENT_SEQ_SERVER_DEL_SERVER_LISTEN:
        case SYNC_EVENT_SEQ_CLIENT_DEL_SERVER_LISTEN:
        case SYNC_EVENT_SEQ_SERVER_UP_DIR_SERVER_LISTEN:
        case SYNC_EVENT_SEQ_CLIENT_UP_DIR_SERVER_LISTEN:
            break;
    }
    ctx.expected_sync_event_list = expected_event;

    rv = VPLThread_Create(listener_thread, listen_syncbox_events, (VPLThread_arg_t)&ctx, NULL, "listen_syncbox_events");
    if (rv != VPL_OK) {
        LOG_ERROR("Failed to spawn event listen thread: %d", rv);
    }
    VPLSem_Wait(&(ctx.sem)); // OK, since thread is started, we need to lock for ctx modifying
    return rv;
}

static int consume_all_events(int instance_num, int time_sec,
        struct syncbox_event_visitor_ctx& ctx, VPLThread_t* listener_thread)
{
    int rv = 0;
    {
        ctx.result = 0;
        ctx.alias = "MD";
        ctx.instance_num = instance_num;
        ctx.consume_all = true;
        ctx.check_extra_event = false;
        ctx.timeout_sec = time_sec;
    }
    rv = VPLThread_Create(listener_thread, listen_syncbox_events, (VPLThread_arg_t)&ctx, NULL, "listen_syncbox_events");
    if (rv != VPL_OK) {
        LOG_ERROR("Failed to spawn event listen thread: %d", rv);
    }
    VPLSem_Wait(&(ctx.sem)); // OK, since thread is started, we need to lock for ctx modifying
    return rv;
}

static void lower_string(std::string& intput, std::string& output)
{
    output = intput;
    std::string::iterator end = output.end();
    for (std::string::iterator i = output.begin(); i != end; ++i) {
        // tolower() changes based on locale.  We don't want this!
        if ('A' <= *i && *i <= 'Z') *i += 'a' - 'A';
    }
}

static void upper_string(std::string& intput, std::string& output)
{
    output = intput;
    std::string::iterator end = output.end();
    for (std::string::iterator i = output.begin(); i != end; ++i) {
        // toupper() changes based on locale.  We don't want this!
        if ('a' <= *i && *i <= 'z') *i += 'A' - 'a';
    }
}

static int syncbox_test_get_sync_states_for_paths(int argc, const char* argv[])
{

    const char *test_name = argv[0];

    const int cloudPC = 0;
    const int clientMD = 1;
    //const int client3 = 2;

    int agent1;
    int agent2;

    int device_num;
    int rv = 0;
    int retry = MAX_RETRY;

    SyncStateExpectedVerifier state_verifier;

    std::string dummy_file_path;
    std::string dummy_file_path2;

    std::string work_dir;
    std::string xyz_name;
    std::string xyz_dir;
    std::string xyz_name2;
    std::string xyz_dir2;
    std::string xyzBig_name;
    std::string xyzBig_dir;
    std::string xyzBig_name2;
    std::string xyzBig_dir2;
    std::string xyzBig_name3;
    std::string xyzBig_dir_no_ts;
    std::string xyzBig_name4;

    int big_file_size_base = 256 * 1024 * 1024;

    std::vector<Target> targets;
    rv = init_test(test_name, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }
    syncbox_event_visitor_ctx ctx;
    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox Sync Events: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    if (rv < 0) {
        LOG_ERROR("Failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    dummy_file_path = work_dir + "/" + "dummy_states_for_paths_file.txt";
    dummy_file_path2 = work_dir + "/" + "dummy_states_for_paths_file2.txt";

    xyz_name = "st4path_xyz.txt"; // not in xyz_dir folder
    xyz_dir = "st4path_xyz_folder/";

    xyz_name2 = "st4path_xyz2.txt";
    xyz_dir2 = "st4path_xyz_folder2/";
    xyz_name2 = xyz_dir2 + xyz_name2; // in xyz_dir2 folder

    xyzBig_name = "st4path_xyzBig.txt"; // not in xyz_dir folder
    xyzBig_name4 = "st4path_xyzBig4.txt"; // not in xyz_dir folder
    xyzBig_dir = "st4path_xyzBig_folder/";
    xyzBig_dir_no_ts = "st4path_xyzBig_folder";

    xyzBig_name2 = "st4path_xyzBig2.txt"; // not in xyz_dir folder
    xyzBig_dir2 = xyzBig_dir + "st4path_xyzBig_folder2/";
    xyzBig_name2 = xyzBig_dir2 + xyzBig_name2;

    xyzBig_name3 = xyzBig_dir + "st4path_xyzBig3.txt";

    {
        // [file structure]
        // st4path_xyz_folder/
        // st4path_xyz.txt
        // st4path_xyz_folder2/st4path_xyz2.txt
        // st4path_xyzBig_folder/st4path_xyzBig_folder2/

        Util_rm_dash_rf(dummy_file_path);

        rv = create_test_dir(targets, xyz_dir, retry);
        CHECK_AND_PRINT_RESULT(test_name, "CreateTestDir", rv);

        rv = create_dummy_file(dummy_file_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
        rv = push_test_file(targets, dummy_file_path, xyz_name, retry);
        CHECK_AND_PRINT_RESULT(test_name, "PushTestFile", rv);

        rv = create_test_dir(targets, xyz_dir2, retry);
        CHECK_AND_PRINT_RESULT(test_name, "CreateTestDir", rv);

        rv = create_test_dir(targets, xyzBig_dir, retry);
        CHECK_AND_PRINT_RESULT(test_name, "CreateTestDir", rv);

        rv = create_test_dir(targets, xyzBig_dir2, retry);
        CHECK_AND_PRINT_RESULT(test_name, "CreateTestDir", rv);

        rv = create_dummy_file(dummy_file_path.c_str(), 1*2048);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
        rv = push_test_file(targets, dummy_file_path, xyz_name2, retry);
        CHECK_AND_PRINT_RESULT(test_name, "PushTestFile", rv);

    }

    // Set Tareget to ClientMD
    if (!targets[clientMD].alias.empty()) {
        // run from remote
        rv = set_target_machine(targets[clientMD].alias.c_str());
        CHECK_AND_PRINT_RESULT(test_name, "SetTarget", rv);
    } else {
        setCcdTestInstanceNum(targets[clientMD].instance_num);
    }

    LOG_ALWAYS("\n\n== Check SYNC_STATE_NOT_IN_SYNC_FOLDER on ClientMD ==");
    {
        state_verifier.clear();
        std::string sync_root = targets[clientMD].work_dir;
        std::string tmp_path;

        tmp_path = std::string(sync_root + "bad");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_NOT_IN_SYNC_FOLDER, false);

        tmp_path = std::string("/tmp/");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_NOT_IN_SYNC_FOLDER, false);

        tmp_path = std::string("C:\\");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_NOT_IN_SYNC_FOLDER, false);

        rv = state_verifier.verify();
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
    }

    LOG_ALWAYS("\n\n== Check SYNC_STATE_FILTERED and SYNC_STATE_UNKNOWN on ClientMD ==");
    {
        state_verifier.clear();
        std::string sync_root = targets[clientMD].work_dir;
        std::string sync_temp_dir = ".sync_temp";
        std::string tmp_path;

        tmp_path = std::string(sync_root + "/" + sync_temp_dir);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_FILTERED, false);

        tmp_path = std::string(sync_root + "/" + sync_temp_dir + "/tmp/");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_FILTERED, false);

        tmp_path = std::string(sync_root + "/" + sync_temp_dir + "Test");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UNKNOWN, false);

        tmp_path = std::string(sync_root + "/Unknown");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UNKNOWN, false);

        rv = state_verifier.verify();
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
    }

    LOG_ALWAYS("\n\n== Check SYNC_STATE_UP_TO_DATE on ClientMD ==");
    {
        state_verifier.clear();
        std::string sync_root = targets[clientMD].work_dir;
        std::string tmp_path;

        tmp_path = std::string(sync_root + "/");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, true);

        tmp_path = std::string(sync_root + "\\");
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, true);

        tmp_path = std::string(sync_root + "/" + xyz_name);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

        tmp_path = std::string(sync_root + "/" + xyz_dir);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

        tmp_path = std::string(sync_root + "/" + xyz_name2);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

        tmp_path = std::string(sync_root + "/" + xyz_dir2);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

        rv = state_verifier.verify();
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
    }

    LOG_ALWAYS("\n\n== Check case sensitivity ==");
    {
        state_verifier.clear();
        std::string sync_root = targets[clientMD].work_dir;
        std::string tmp_path;

        upper_string(sync_root, tmp_path);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, true);

        lower_string(sync_root, tmp_path);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, true);

        std::string tmp_file_path = std::string(sync_root + "/" + xyz_name);
        lower_string(tmp_file_path, tmp_path);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

        upper_string(tmp_file_path, tmp_path);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

        rv = state_verifier.verify();
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
    }

    agent1 = clientMD;
    agent2 = cloudPC;
    for (int j = 0; j < 2; j++) {
        LOG_ALWAYS("\n\n== Check SYNC_STATE_NEED_TO_UPLOAD on target [%d, %s] ==",
                targets[agent1].instance_num, targets[agent1].alias.c_str());
        {
            state_verifier.clear();
            std::string sync_root = targets[agent1].work_dir;
            std::string tmp_path;

            // [file structure]
            // st4path_xyzBig.txt (upload)

            rv = create_dummy_file(dummy_file_path.c_str(), big_file_size_base);
            CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
            rv = push_file(targets[agent1], dummy_file_path, xyzBig_name);
            CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

            tmp_path = std::string(sync_root + "/" + xyzBig_name);
            state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_NEED_TO_UPLOAD, false);
            //state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

            rv = state_verifier.wait_until_get_state(retry);
            //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");

            for (int i = 0; i < device_num; i++) {
                if (i != agent1) {
                    LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                    rv = compare_files(targets[i], dummy_file_path, xyzBig_name, retry);
                    //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedFile", rv);
                    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedFile", rv, "");
                }
            }

            state_verifier.clear();
            state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

            rv = state_verifier.wait_until_get_state(retry);
            //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
        }
        LOG_ALWAYS("\n\n== Check SYNC_STATE_NEED_TO_DOWNLOAD on target [%d, %s] ==",
                targets[agent1].instance_num, targets[agent1].alias.c_str());
        {
            state_verifier.clear();
            std::string sync_root = targets[agent1].work_dir;
            std::string tmp_path;

            // [file structure]
            // st4path_xyzBig4.txt (download)

            rv = create_dummy_file(dummy_file_path.c_str(), big_file_size_base + 5);
            CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
            rv = push_file(targets[agent2], dummy_file_path, xyzBig_name4);
            CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

            tmp_path = std::string(sync_root + "/" + xyzBig_name4);
            if (j == 0) {
                // Only test ClientMD, don't test download if we are checking CloudPC,
                //  because downlaod can be very fast and hard to get it on CloudPC.
                state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_NEED_TO_DOWNLOAD, false);

                rv = state_verifier.wait_until_get_state(retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
            }

            for (int i = 0; i < device_num; i++) {
                if (i != agent2) {
                    LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                    rv = compare_files(targets[i], dummy_file_path, xyzBig_name4, retry);
                    //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedFile", rv);
                    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedFile", rv, "");
                }
            }

            state_verifier.clear();
            state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);

            rv = state_verifier.wait_until_get_state(retry);
            //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
        }
        { // Clean up files
            rv = delete_file(targets[agent1], xyzBig_name);
            //CHECK_AND_PRINT_RESULT(test_name, "DeleteFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "DeleteFile", rv, "");
            rv = delete_file(targets[agent2], xyzBig_name4);
            //CHECK_AND_PRINT_RESULT(test_name, "DeleteFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "DeleteFile", rv, "");
            for (int i = 0; i < device_num; i++) {
                if (i != agent1) {
                    rv = is_file_deleted(targets[agent1], xyzBig_name, retry);
                    //CHECK_AND_PRINT_RESULT(test_name, "VerifyDeletedFile", rv);
                    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyDeletedFile", rv, "");
                }
            }
            for (int i = 0; i < device_num; i++) {
                if (i != agent2) {
                    rv = is_file_deleted(targets[agent2], xyzBig_name4, retry);
                    //CHECK_AND_PRINT_RESULT(test_name, "VerifyDeletedFile", rv);
                    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyDeletedFile", rv, "");
                }
            }
        }
        if (j == 0) {
            // setup next target
            agent1 = cloudPC;
            agent2 = clientMD;
            big_file_size_base = big_file_size_base + 30;
        } else if (j == 1) {
            // reset target before we leave
            agent1 = clientMD;
            agent2 = cloudPC;
            big_file_size_base = big_file_size_base - 30;
        }
        // Set Tareget
        if (!targets[agent1].alias.empty()) {
            // run from remote
            rv = set_target_machine(targets[agent1].alias.c_str());
            CHECK_AND_PRINT_RESULT(test_name, "SetTarget", rv);
        } else {
            setCcdTestInstanceNum(targets[agent1].instance_num);
        }
    }

    LOG_ALWAYS("\n\n== Check SYNC_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD on ClientMD ==");
    {
        state_verifier.clear();
        std::string sync_root = targets[clientMD].work_dir;
        std::string tmp_path;

        // [file structure]
        // st4path_xyzBig_folder/st4path_xyzBig_folder2/st4path_xyzBig2.txt (download)
        // st4path_xyzBig_folder/st4path_xyzBig3.txt (upload)
        // st4path_xyzBig_folder (check this folder)

        rv = create_dummy_file(dummy_file_path.c_str(), big_file_size_base * 2);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
        rv = push_file(targets[cloudPC], dummy_file_path, xyzBig_name2);
        CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

        tmp_path = std::string(sync_root + "/" + xyzBig_name2);
        state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_NEED_TO_DOWNLOAD, false);
        rv = state_verifier.wait_until_get_state(retry);
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");

        rv = create_dummy_file(dummy_file_path2.c_str(), big_file_size_base);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
        rv = push_file(targets[clientMD], dummy_file_path2, xyzBig_name3);
        CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

        state_verifier.clear();
        tmp_path = std::string(sync_root + "/" + xyzBig_dir);
        state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD, false);
        rv = state_verifier.wait_until_get_state(retry);
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");

        // Check path without trail slash
        state_verifier.clear();
        tmp_path = std::string(sync_root + "/" + xyzBig_dir_no_ts);
        state_verifier.add_expected_result(tmp_path, ccd::SYNC_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD, false);
        rv = state_verifier.verify();
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");

        // Check similar path
        state_verifier.clear();
        std::string tmp_file_path;
        tmp_file_path = std::string(sync_root + "/" + xyzBig_dir_no_ts + "Bad");
        state_verifier.add_expected_result(tmp_file_path, ccd::SYNC_STATE_UNKNOWN, false);
        rv = state_verifier.verify();
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");

        // Check case sensitivity
        state_verifier.clear();
        upper_string(tmp_path, tmp_file_path);
        state_verifier.add_expected_result(tmp_file_path, ccd::SYNC_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD, false);
        lower_string(tmp_path, tmp_file_path);
        state_verifier.add_expected_result(tmp_file_path, ccd::SYNC_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD, false);
        rv = state_verifier.verify();
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");

        for (int i = 0; i < device_num; i++) {
            if (i != cloudPC) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], dummy_file_path, xyzBig_name2, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedFile", rv, "");
            }
        }
        for (int i = 0; i < device_num; i++) {
            if (i != clientMD) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], dummy_file_path2, xyzBig_name3, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedFile", rv, "");
            }
        }

        state_verifier.clear();
        tmp_path = std::string(sync_root + "/" + xyzBig_dir);
        state_verifier.add_seq_expected_result(tmp_path, ccd::SYNC_STATE_UP_TO_DATE, false);
        rv = state_verifier.wait_until_get_state(retry);
        //CHECK_AND_PRINT_RESULT(test_name, "VerifyGetSyncStatesForPaths", rv);
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyGetSyncStatesForPaths", rv, "");
        rv = 0; // remove this line once we remove expected to fail
    }

exit:

    // Stop CCD
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

static int syncbox_test_sync_events(int argc, const char* argv[])
{

    const char *test_name = argv[0];

    const int cloudPC = 0;
    const int clientMD = 1;
    //const int client3 = 2;

    int device_num;
    int i;
    int rv = 0;
    int retry = MAX_RETRY;

    std::string work_dir;
    std::string xyz_name;
    std::string xyz_path;

    VPLThread_t event_thread;

    std::vector<Target> targets;
    rv = init_test(test_name, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }
    syncbox_event_visitor_ctx ctx;
    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox Sync Events: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    if (rv < 0) {
        LOG_ERROR("Failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    xyz_name = "event_xyz.txt";
    xyz_path = work_dir + "/" + xyz_name;

    Util_rm_dash_rf(xyz_path);

    if (!targets[cloudPC].alias.empty()) {
        ctx.is_run_from_remote = true;
    }
    // Consume all events not belong to these test case.
    rv = consume_all_events(targets[clientMD].instance_num, 5, ctx, &event_thread);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    LOG_ALWAYS("Consume all events not belong to these test case. wait for %d seconds", ctx.timeout_sec);
    rv = wait_event_thread_done(ctx);
    CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);

    // 1. Add xyz.txt on CloudPC
    LOG_ALWAYS("\n\n== Add event_xyz.txt on CloudPC ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_NEW_FILE_DOWNLOADED,
            SYNC_EVENT_SEQ_SERVER_UP_FILE_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);

    { // file operation
        rv = create_dummy_file(xyz_path.c_str(), 1);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
        rv = push_file(targets[cloudPC], xyz_path, xyz_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != cloudPC) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 2. Modify xyz.txt on CloudPC
    LOG_ALWAYS("\n\n== Modify event_xyz.txt on CLoudPC ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_MODIFIED_FILE_DOWNLOADED,
            SYNC_EVENT_SEQ_SERVER_UP_FILE_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        rv = create_dummy_file(xyz_path.c_str(), 2);
        CHECK_AND_PRINT_RESULT(test_name, "ModifyDummyFile", rv);

        rv = push_file(targets[cloudPC], xyz_path, xyz_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != cloudPC) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyModifiedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyModifiedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 3. Delete xyz.txt on CloudPC
    LOG_ALWAYS("\n\n== Delete event_xyz.txt on CloudPC ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_FILE_DELETE_DOWNLOADED,
            SYNC_EVENT_SEQ_SERVER_DEL_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        rv = delete_file(targets[cloudPC], xyz_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != cloudPC) {
                rv = is_file_deleted(targets[i], xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyDeletedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyDeletedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 4. Create dir xyz/ on CloudPC
    LOG_ALWAYS("\n\n== Create dir event_xyz_folder/ on CloudPC ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_FOLDER_CREATE_DOWNLOADED,
            SYNC_EVENT_SEQ_SERVER_UP_DIR_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        std::string xyz_dir_name = "event_xyz_folder/";

        rv = create_dir(targets[cloudPC], xyz_dir_name);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDir", rv);
        for (i = 0; i < device_num; i++) {
            if (i != cloudPC) {
                rv = is_file_existed(targets[i], xyz_dir_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedDir", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedDir", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 5. Delete xyz/ on CloudPC
    LOG_ALWAYS("\n\n== Delete dir event_xyz_folder/ on Cloud PC ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_FOLDER_DELETE_DOWNLOADED,
            SYNC_EVENT_SEQ_SERVER_DEL_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        std::string xyz_dir_name = "event_xyz_folder/";

        rv = delete_dir(targets[cloudPC], xyz_dir_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != cloudPC) {
                rv = is_file_deleted(targets[i], xyz_dir_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyDeletedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyDeletedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 2.1. Add xyz.txt on ClientMD
    LOG_ALWAYS("\n\n== Add event_xyz.txt on ClientMD ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_NEW_FILE_UPLOADED,
            SYNC_EVENT_SEQ_CLIENT_UP_FILE_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        rv = create_dummy_file(xyz_path.c_str(), 1);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);

        rv = push_file(targets[clientMD], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != clientMD) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 2.2. Modify xyz.txt on ClientMD
    LOG_ALWAYS("\n\n== Modify event_xyz.txt on ClientMD ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_MODIFIED_FILE_UPLOADED,
            SYNC_EVENT_SEQ_CLIENT_UP_FILE_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        rv = create_dummy_file(xyz_path.c_str(), 2);
        CHECK_AND_PRINT_RESULT(test_name, "ModifyDummyFile", rv);

        rv = push_file(targets[clientMD], xyz_path, xyz_name);
        CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

        for (i = 0; i < device_num; i++) {
            if (i != clientMD) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_files(targets[i], xyz_path, xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyModifiedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyModifiedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 2.3. Delete xyz.txt on ClientMD
    LOG_ALWAYS("\n\n== Delete event_xyz.txt on ClientMD ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_FILE_DELETE_UPLOADED,
            SYNC_EVENT_SEQ_CLIENT_DEL_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        rv = delete_file(targets[clientMD], xyz_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != clientMD) {
                rv = is_file_deleted(targets[i], xyz_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyDeletedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyDeletedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 2.4. Create dir xyz/ on ClientMD
    LOG_ALWAYS("\n\n== Create dir event_xyz_folder/ on CloudPC ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_FOLDER_CREATE_UPLOADED,
            SYNC_EVENT_SEQ_CLIENT_UP_DIR_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        std::string xyz_dir_name = "event_xyz_folder/";

        rv = create_dir(targets[clientMD], xyz_dir_name);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDir", rv);
        for (i = 0; i < device_num; i++) {
            if (i != clientMD) {
                rv = is_file_existed(targets[i], xyz_dir_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedDir", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedDir", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 2.5. Delete xyz/ on ClientMD
    LOG_ALWAYS("\n\n== Delete dir event_xyz_folder/ on ClientMD ==");
    rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
            &event_thread, ctx, ccd::CCD_SYNC_EVENT_FOLDER_DELETE_UPLOADED,
            SYNC_EVENT_SEQ_CLIENT_DEL_CLIENT_LISTEN);
    CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);
    {
        std::string xyz_dir_name = "event_xyz_folder/";

        rv = delete_dir(targets[clientMD], xyz_dir_name);
        if (rv != 0) {
            goto exit;
        }

        for (i = 0; i < device_num; i++) {
            if (i != clientMD) {
                rv = is_file_deleted(targets[i], xyz_dir_name, retry);
                //CHECK_AND_PRINT_RESULT(test_name, "VerifyDeletedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyDeletedFile", rv, "");
            }
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");

    // 3.0 Conflict xyz.txt on ClientMD
    LOG_ALWAYS("\n\n== Conflict event_xyz.txt on ClientMD ==");
    {
        std::string test_dir = "";
        rv = create_dummy_file(xyz_path.c_str(), 1);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);

        std::string test_file = test_dir + "/" + xyz_name;

        rv = create_test_dir(targets, test_dir, retry);
        CHECK_AND_PRINT_RESULT(test_name, "CreateTestDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry);
        CHECK_AND_PRINT_RESULT(test_name, "PushTestFile", rv);

        rv = suspend_ccd(targets[clientMD]);
        CHECK_AND_PRINT_RESULT(test_name, "SuspendCCD", rv);

        rv = create_dummy_file(xyz_path.c_str(), 2);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
        rv = push_file(targets[cloudPC], xyz_path, test_file);
        CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

        rv = create_dummy_file(xyz_path.c_str(), 3);
        CHECK_AND_PRINT_RESULT(test_name, "CreateDummyFile", rv);
        rv = push_file(targets[clientMD], xyz_path, test_file);
        CHECK_AND_PRINT_RESULT(test_name, "PushFile", rv);

        rv = resume_ccd(targets[clientMD]);
        CHECK_AND_PRINT_RESULT(test_name, "ResumeCCD", rv);

        // after resume ccd, start event listener
        rv = setup_event_context_and_start_listener(targets[clientMD].instance_num,
                &event_thread, ctx, (ccd::SyncEventType_t)0,
                SYNC_EVENT_SEQ_CLIENT_CONFLICT_CLIENT_LISTEN);

        CHECK_AND_PRINT_RESULT(test_name, "CreateSyncBoxEventThread", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, 1, retry);
            //CHECK_AND_PRINT_RESULT(test_name, "VerifyAddedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "VerifyAddedFile", rv, "17333");
        }
    }
    rv = wait_event_thread_done(ctx);
    //CHECK_AND_PRINT_RESULT(test_name, "WaitEventThread", rv);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(test_name, "WaitEventThread", rv, "");
    rv = 0; // TODO: Remove this once we remove expected to fail

exit:

    // Stop CCD
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int syncbox_test_stress_test(int argc, const char* argv[]) {
    const char *TEST_SYNCBOX_STR = "SyncboxStressTest";

    const int cloudpc = 0;
    const int client1 = 1;
    const int client2 = 2;

    int device_num; 
    int i, j, k, l;
    int rv = 0;
    int retry = MAX_RETRY_FOR_STRESS_TEST;
    int expect_conflict_num;

    std::string work_dir;
    std::string xyz_name;
    std::string xyz_path;
    std::vector<std::string> src;

    std::vector<Target> targets;
    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }
    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox Offline Test: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "GetCurDir", rv);
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32
    xyz_name = "xyz.txt";
    xyz_path = work_dir + "/" + xyz_name;

    {
        rv = create_dummy_file(xyz_path.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);

        char dir_num[5];
        for (i = 0; i < 10; i++) {
            sprintf(dir_num, "L%d", i);  
            std::string dir_level1 = dir_num;
            // Create L0 ~ L9
            LOG_ALWAYS("Create dir %s", dir_level1.c_str());
            rv = create_dir(targets[client1], dir_level1);
            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDir", rv);

            for (j = 0; j < 10; j++) {
                sprintf(dir_num, "L%d", (j + 10));
                std::string dir_level2 = dir_level1 + "/" + dir_num;
                // Create L10 ~ L 19
               LOG_ALWAYS("Create dir %s", dir_level2.c_str());
               rv = create_dir(targets[client1], dir_level2);
               CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDir", rv);

               for (k = 0; k < 10; k++) {
                   sprintf(dir_num, "L%d", (k + 100));
                   std::string dir_level3 = dir_level2 + "/" + dir_num;
                   // Create L100 ~ L 109
                   LOG_ALWAYS("Create dir %s", dir_level3.c_str());
                   rv = create_dir(targets[client1], dir_level3);
                   CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDir", rv);

                   for (l = 0; l < 10; l++) {
                       sprintf(dir_num, "F0%d", l);
                       std::string file_level3 = dir_level3 + "/" + dir_num + ".txt";
                       LOG_ALWAYS("Create file %s", file_level3.c_str());
                       rv = push_file(targets[client1], xyz_path, file_level3);
                       CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);
                    }
                }
            }
        }
    
        list_dir(targets[client1], "", src);
        for (i = 0; i < device_num; i++) {
            if (i != client1) {
                LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                rv = compare_dir(targets[i], "", src, retry);
//                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyAddedFile", rv, "17333");
            }
        }
    }

    // Modify larget file xyz.txt in client B and client C
    LOG_ALWAYS("\n\n== Modify large file xyz.txt in client B and client C ==");
    {
        expect_conflict_num = 1;
        rv = create_dummy_file(xyz_path.c_str(), 512 * 1024 * 1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);

        std::string test_dir = "test";
        std::string test_file = test_dir + "/" + xyz_name;
        
        rv = create_test_dir(targets, test_dir, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDir", rv);

        rv = push_test_file(targets, xyz_path, test_file, retry); 
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushTestFile", rv);

        rv = suspend_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "SuspendCCD", rv);

        rv = create_dummy_file(xyz_path.c_str(), 512 * 1024 * 1024 + 5);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);
        rv = push_file(targets[client1], xyz_path, test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);

        rv = create_dummy_file(xyz_path.c_str(), 512 * 1024 * 1024 + 10);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);
        rv = push_file(targets[client2], xyz_path, test_file);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);

        rv = resume_ccd(targets[cloudpc]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "ResumeCCD", rv);

        for (i = 0; i < device_num; i++) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = is_conflict_file_existed(targets[i], test_dir, xyz_name, expect_conflict_num, retry);
//            CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyAddedFile", rv, "17333");
        }
    }
exit:

    // Stop CCD
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return 0;
}

static int syncbox_check_syncFolderPath(const std::vector<Target> &targets,
                                        const std::string &datasetStr,
                                        const std::string &syncboxdatasetStr,
                                        const std::string &syncPath,
                                        const std::string &absPathExpected)
{
    int rv = -1;
    const int cloudpc = 0; //, client1 = 1, client2 = 2;

    std::string response;
    cJSON2 *jsonResponse = NULL;
    cJSON2 *absPath = NULL; 

    rv = fs_test_readdirmetadata(targets[cloudpc].user_id,
                                 datasetStr,
                                 syncboxdatasetStr,
                                 syncPath,
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
        if (absPath == NULL || absPath->type != cJSON2_String ||
            strcmp(absPathExpected.c_str(), absPath->valuestring)) {
            LOG_ERROR("absPath mismatch: %s, %s", absPathExpected.c_str(), absPath->valuestring);
            rv = -1;
        }
    }
    if (jsonResponse) {
        cJSON2_Delete(jsonResponse);
        jsonResponse = NULL;
    }

    return rv;
}


int syncbox_test_remotefile(int argc, const char* argv[]) {
    const char *TEST_SYNCBOX_STR = "SyncboxTestRemoteFile";

    const int cloudpc = 0;
    const int client1 = 1;
    const int client2 = 2;

    int i;
    int rv = 0;
    int device_num;
    int retry = MAX_RETRY;
    u64 dataset_id;

    std::string work_dir;
    std::string upload_dir;
    std::stringstream ss;

    static const int MAX_SIZE = 1*1024*1024;
    static const int NR_FILES = 10;
    std::vector< std::pair<std::string, VPLFS_file_size_t> > filelist;

    std::vector<Target> targets;

    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }

    device_num = targets.size();

    rv = getCurDir(work_dir);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "GetCurDir", rv);
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32

    // Prepare work_dir for remote file usage, should not be syncbox folder
    rv = targets[client1].device->getDxRemoteRoot(targets[client1].work_dir);
    rv = targets[client2].device->getDxRemoteRoot(targets[client2].work_dir);

    std::replace(targets[client1].work_dir.begin(), targets[client1].work_dir.end(), '\\', '/');
    std::replace(targets[client2].work_dir.begin(), targets[client2].work_dir.end(), '\\', '/');

    LOG_ALWAYS("work dir for cloudpc is %s", targets[cloudpc].work_dir.c_str());
    LOG_ALWAYS("work dir for client1 is %s", targets[client1].work_dir.c_str());
    LOG_ALWAYS("work dir for client2 is %s", targets[client2].work_dir.c_str());

    // Prepare upload_dir in remote file format
    if (targets[cloudpc].os_version.find(OS_WINDOWS) != std::string::npos) {
        upload_dir = "Computer/" + targets[cloudpc].work_dir + "/remotefile";
        std::replace(upload_dir.begin(), upload_dir.end(), '\\', '/');
        size_t index = upload_dir.find_first_of(":");
        if (index != std::string::npos) {
            upload_dir.erase(index, 1);
        }
    } else {
        upload_dir = targets[cloudpc].work_dir + "/remotefile";
    }
    LOG_ALWAYS("upload dir is %s", upload_dir.c_str());

    for (i = 0; i < NR_FILES; i++) {
        ss.str("");
        ss << "temp" << i;
        std::string file_name = ss.str();
        std::string file_path = work_dir + "/" + file_name;
        VPLFS_file_size_t size = MAX_SIZE / (i + 1);

        Util_rm_dash_rf(file_name);
        rv = create_dummy_file(file_path.c_str(), size);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);

        std::string path = targets[client1].work_dir + "/" + file_name;
        filelist.push_back(std::make_pair(path, size));

        rv = push_file(targets[client1], file_path, file_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);
        
        Util_rm_dash_rf(file_name);
    }
    
    {
        std::string large_file_name = "test_large.jpg";
        std::string large_file_path = work_dir + "/" + large_file_name;
        push_file(targets[client1], large_file_path, large_file_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);
    }

    remotefile_feature_enable(
        targets[cloudpc].instance_num,
        targets[client1].instance_num,
        targets[cloudpc].user_id,
        targets[cloudpc].device_id,
        true,
        false,
        targets[cloudpc].alias,
        targets[client1].alias
    );

    rv = get_dataset_id(targets[cloudpc], "Device Storage", dataset_id);
    ss.str("");
    ss << dataset_id;

    // Point target to MD to test remotefile_basic
    if (!targets[client1].alias.empty()) {
        set_target_machine(targets[client1].alias.c_str());
    } else {
        setCcdTestInstanceNum(targets[client1].instance_num);
    }

    autotest_sdk_release_remotefile_basic(
        false,
        false,
        false,
        targets[cloudpc].user_id,
        targets[cloudpc].device_id,
        ss.str(),
        targets[client1].work_dir,
        targets[client2].work_dir,
        work_dir,
        upload_dir,
        filelist,
        targets[cloudpc].separator,
        targets[client1].separator,
        targets[client2].separator
    );

    //Test /rf/readdirmetadata
    {
        std::string datasetStr = ss.str();
        std::string syncboxdatasetName;
        std::string syncboxdatasetStr;
        std::string response;
        std::string folderPath;
        u64 syncboxdataset_id = 0;

        ss.str("");
        ss << "Syncbox-" << targets[cloudpc].device_id;
        syncboxdatasetName = ss.str();
        rv = get_dataset_id(targets[cloudpc], syncboxdatasetName, syncboxdataset_id);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "GetSyncBoxDatasetId", rv);
        ss.str("");
        ss << syncboxdataset_id;
        syncboxdatasetStr = ss.str();

        if (!targets[client1].alias.empty()) {
            set_target_machine(targets[client1].alias.c_str());
        } else {
            setCcdTestInstanceNum(targets[client1].instance_num);
        }

        rv = syncbox_check_syncFolderPath(targets, datasetStr, syncboxdatasetStr,
                                          "/", targets[cloudpc].work_dir);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "ReadDirMetadata_root", rv);

        folderPath = upload_dir;
        RF_MAKE_DIR_SKIP(false, datasetStr, folderPath.c_str(), response, rv);
        folderPath = upload_dir + "/foo";
        RF_MAKE_DIR_SKIP(false, datasetStr, folderPath.c_str(), response, rv);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDir", rv);

        rv = syncbox_check_syncFolderPath(targets, datasetStr, syncboxdatasetStr,
                                          "remotefile/foo", targets[cloudpc].work_dir + "\\remotefile\\foo");
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "ReadDirMetadata_foo", rv);

        folderPath = upload_dir + "/foo/bar";
        RF_MAKE_DIR_SKIP(false, datasetStr, folderPath.c_str(), response, rv);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDir", rv);
        
        rv = syncbox_check_syncFolderPath(targets, datasetStr, syncboxdatasetStr,
                                          "remotefile/foo/bar", targets[cloudpc].work_dir + "\\remotefile\\foo\\bar");
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "ReadDirMetadata_foobar", rv);

        std::string file_name = "abc.txt";
        std::string file_path = work_dir + "/" + file_name;
        std::string remote_file_path = folderPath + "/" + file_name;
 
        rv = create_dummy_file(file_path.c_str(), 1024);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);
        rv = push_file(targets[client1], file_path, file_name);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);

        file_path = targets[client1].work_dir + "/" + file_name;
        RF_UPLOAD(false, datasetStr, file_path.c_str(), remote_file_path.c_str(), response, rv);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "UploadFile", rv);

        std::vector<std::string> src;
        int retry_count = 0;
        get_syncbox_path(targets[client2], targets[client2].work_dir);
        while (retry_count++ < retry) {
            LOG_ALWAYS("retry %d", retry_count);
            src.clear();
            list_dir(targets[cloudpc], "", src);
            rv = compare_dir(targets[client2], "", src, 1);
            if (rv == 0) {
                break;
            }
        }
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CompareDirectory", rv);
    }


exit:

    // Stop CCD
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return 0;
}

int syncbox_test_linking(int argc, const char* argv[])
{
    const char *TEST_SYNCBOX_STR = "TestLinking";
    const int client1 = 1;
    const int server = 0;

    int device_num; 
    int i;
    int rv = 0;
    int retry = 7000;

    std::string work_dir;
    std::string file_name_client;
    std::string file_name_server;
    std::string file_path;
    std::string sync_root;

    std::string server_sync_root;
    std::string client_sync_root;

    std::vector<Target> targets;
    std::vector<std::string> sync_root_contents;

    rv = init_test(TEST_SYNCBOX_STR, argc, argv, targets, retry);
    if (rv != 0) {
        return rv;
    }

    device_num = targets.size();

    LOG_ALWAYS("Auto Test Syncbox File Sync: User(%s) Password(%s) Devices(%d) Retry(%d)", argv[1], argv[2], device_num, retry);

    rv = getCurDir(work_dir);
    if (rv < 0) {
        LOG_ERROR("Failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');
#endif // WIN32

    LOG_ALWAYS("\n\n== Unlink Client ==");

    rv = unlink_target(targets[client1]);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "UnlinkClient", rv);

    LOG_ALWAYS("\n\n== Relink Client ==");

    rv = link_target(targets[client1]);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "LinkClient", rv);

    file_name_server = "server.txt";
    file_path = work_dir + "/" + file_name_server;
    Util_rm_dash_rf(file_path);
    rv = create_dummy_file(file_path.c_str(), 0);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateFileOnServer", rv);

    rv = push_file(targets[server], file_path, file_name_server);
    if (rv != 0) {
        goto exit;
    }

    for (i = 0; i < device_num; i++) {
        if (i != client1) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = compare_files(targets[i], file_path, file_name_client, retry);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyFileOnClient", rv, "17333");
        }
    }

    LOG_ALWAYS("\n\n== Unlink Server ==");

    rv = unlink_target(targets[server]);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "UnlinkServer", rv);

    LOG_ALWAYS("\n\n== Relink Server ==");

    rv = link_target(targets[server]);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "LinkServer", rv);

    file_name_client = "client.txt";
    file_path = work_dir + "/" + file_name_client;
    Util_rm_dash_rf(file_path);
    rv = create_dummy_file(file_path.c_str(), 0);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateFileOnClient", rv);

    rv = push_file(targets[client1], file_path, file_name_client);
    if (rv != 0) {
        goto exit;
    }

    for (i = 0; i < device_num; i++) {
        if (i != client1) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = compare_files(targets[i], file_path, file_name_client, retry);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyFileOnServer", rv, "17333");
        }
    }

    LOG_ALWAYS("\n\n== Change Sync Root ==");

    rv = get_syncbox_path(targets[server], sync_root);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "GetArchiveDeviceSyncRoot", rv, "17333");

    rv = disable_syncbox(targets[server]);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "DisableSyncRoot", rv, "17333");

    // rv = delete_dir(targets[server], sync_root);
    // CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "DeleteOldSyncRootDir", rv, "17333");

    // Util_rm_dash_rf(sync_root);
    sync_root.append("_new");

    rv = create_dir(targets[server], sync_root);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "CreateNewSyncRootDir", rv, "17333");

    rv = enable_syncbox(targets[server], sync_root);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "EnableNewSyncRoot", rv, "17333");

    file_name_client = "new_root.txt";
    file_path = work_dir + "/" + file_name_client;
    Util_rm_dash_rf(file_path);
    rv = create_dummy_file(file_path.c_str(), 0);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "CreateFile", rv, "17333");

    rv = push_file(targets[client1], file_path, file_name_client);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "PushFileOnClient", rv, "17333");

    for (i = 0; i < device_num; i++) {
        if (i != client1) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = compare_files(targets[i], file_path, file_name_client, retry);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyFile", rv, "17333");
        }
    }

    list_dir(targets[server], "", sync_root_contents);
    rv = compare_dir(targets[client1], "", sync_root_contents, 1);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CompareServerAndClientRootDirectories", rv);

    LOG_ALWAYS("\n\n== Change Archive Storage Device ==");

    rv = get_syncbox_path(targets[server], server_sync_root);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "GetArchiveDeviceSyncRoot", rv, "17333");

    rv = get_syncbox_path(targets[server], client_sync_root);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "GetClientSyncRoot", rv, "17333");

    rv = disable_syncbox(targets[server]);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "DisableServer", rv, "17333");

    rv = disable_syncbox(targets[client1]);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "DisableClient", rv, "17333");

    targets[server].is_cloud_pc = false;
    targets[client1].is_cloud_pc = true;

    rv = enable_syncbox(targets[client1], client_sync_root);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "EnableNewServer", rv, "17333");

    rv = enable_syncbox(targets[server], server_sync_root);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "EnableNewClient", rv, "17333");

    file_name_client = "new_server.txt";
    file_path = work_dir + "/" + file_name_client;
    Util_rm_dash_rf(file_path);
    rv = create_dummy_file(file_path.c_str(), 0);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "CreateFile", rv, "17333");

    rv = push_file(targets[server], file_path, file_name_client);
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "PushFileOnClient", rv, "17333");

    for (i = 0; i < device_num; i++) {
        if (i != server) {
            LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
            rv = compare_files(targets[i], file_path, file_name_client, retry);
            // CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "VerifyAddedFile", rv);
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_SYNCBOX_STR, "VerifyFile", rv, "17333");
        }
    }

    sync_root_contents.clear();
    list_dir(targets[server], "", sync_root_contents);
    rv = compare_dir(targets[client1], "", sync_root_contents, 1);
    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CompareServerAndClientRootDirectories", rv);

exit:

    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

int get_random_number(int mod) {
    srand((unsigned int)time(NULL));
    return rand() % mod;
}

static int syncbox_test_longterm(int argc, const char* argv[]) {
    
    const char *SYNCBOX_STR = "SyncBox";
    const char *TEST_SYNCBOX_STR = "TestLongTerm";
    const char *SYNCBOX_FOLDER = "SyncBox";

    const static int MAX_FILE_SIZE = 128 * 1024 * 1024;

    const static int OP_ADD_FILE    = 0;
    const static int OP_MODIFY_FILE = 1;
    const static int OP_RENAME_FILE = 2;
    const static int OP_MOVE_FILE   = 3;
    const static int OP_DELETE_FILE = 4;
    const static int OP_ADD_DIR     = 5;
    const static int OP_RENAME_DIR  = 6;
    const static int OP_MOVE_DIR    = 7;
    const static int OP_DELETE_DIR  = 8;

    const static int cloudpc = 0;

    int rv;
    int op_count;
    int loop_count;
    int device_num;
    std::vector<Target> targets;

    std::vector< std::pair<std::string,std::string> > files;
    std::vector< std::pair<std::string,std::string> > dirs;
    std::vector<std::string> src;
    int target;
    int op;
    int loop;
    int retry;
    std::string work_dir;
    std::stringstream ss;

    if (checkHelp(argc, argv) || argc < 7) {
        printf("%s %s <username> <password> <domain> <num of device> <loop count> <retry times>\n", SYNCBOX_STR, argv[0]);
        return -1;   // No arguments needed 
    }

    device_num = atoi(argv[4]);
    loop_count = atoi(argv[5]);
    retry = atoi(argv[6]);

    int id;
    for (int i = 0; i < device_num; i++) {
        id = i + 1;
        setCcdTestInstanceNum(id);
        const char *testStr = "SetDomain";
        const char *testArg[] = { testStr, argv[3] };
        rv = set_domain(2, testArg);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, testStr, rv);
        if (i == 0) {
           targets.push_back(Target(id, true));
        } else {
           targets.push_back(Target(id, false));
        }
    }

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    LOG_ALWAYS("\n\n==== Starting CCD ====");
    for (int i = 0; i < device_num; i++) {
        rv = init_ccd(targets[i], argv[1], argv[2]);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "LaunchCCD", rv);
        rv = delete_dir(targets[i], SYNCBOX_FOLDER);
        rv = create_dir(targets[i], SYNCBOX_FOLDER);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateSyncBoxFolder", rv);
        rv = enable_syncbox(targets[i], SYNCBOX_FOLDER);
        CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "EnableSyncBox", rv);
        get_syncbox_path(targets[i], targets[i].work_dir);
        LOG_ALWAYS("SyncBox work dir of %s is: %s", targets[i].alias.c_str(), targets[i].work_dir.c_str());
    }

    work_dir = "/home/tony/dxshell";
    rv = 0;
    loop = 0;
    op_count = OP_DELETE_DIR + 1;
    dirs.push_back(std::make_pair("", ""));
    target = get_random_number(device_num);

    while (loop++ < loop_count || loop_count == -1) {
        op = get_random_number(op_count);

        std::pair<std::string, std::string> dir;
        std::pair<std::string, std::string> file;
        std::string name;
        std::string path;
        int size;
        int index;
        switch(op) {
            case OP_ADD_FILE:
add_file:
                dir = dirs.at(get_random_number(dirs.size()));
                path = dir.first + dir.second;
                size = get_random_number(MAX_FILE_SIZE) + 1;
                ss.str("");
                ss << "file_" << time(NULL);
                name = ss.str();
                rv = create_dummy_file(name.c_str(), size);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);
                rv = push_file(targets[target], name, path + name);
                Util_rm_dash_rf(name);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);
                files.push_back(std::make_pair(path, name));
                LOG_ALWAYS("Add file %s%s, size %d", path.c_str(), name.c_str(), size);
                break;
            case OP_MODIFY_FILE:
                if (files.size() > 0) {
                    index = get_random_number(files.size());
                    file = files.at(index);
                    path = file.first + file.second;
                    name = file.second;
                    size = get_random_number(MAX_FILE_SIZE) + 1;
                    rv = create_dummy_file(name.c_str(), size);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDummyFile", rv);
                    rv = push_file(targets[target], name, path);
                    Util_rm_dash_rf(name);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "PushFile", rv);
                    LOG_ALWAYS("Modify file %s", path.c_str());
                } else {
                    goto add_file;
                }
                break;
            case OP_RENAME_FILE:
                if (files.size() > 0) {
                    index = get_random_number(files.size());
                    file = files.at(index);
                    //path = file.first + file.second;
                    size = get_random_number(MAX_FILE_SIZE) + 1;
                    ss.str("");
                    ss << "file_" << time(NULL);
                    name = ss.str();
                    rv = rename_file(targets[target], file.first + file.second, file.first + name);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "RenameFile", rv);
                    files.erase(files.begin() + index);
                    files.push_back(std::make_pair(file.first, name));
                    LOG_ALWAYS("Rename file %s->%s", file.second.c_str(), name.c_str());
                } else {
                    goto add_file;
                }
                break; 
            case OP_MOVE_FILE:
                if (files.size() > 0 && dirs.size() > 1) {
                    int file_index = get_random_number(files.size());
                    int dir_index = get_random_number(dirs.size());
                    path = files[file_index].first + files[file_index].second;
                    name = files[file_index].second;
                    std::string new_path = dirs[dir_index].first + dirs[dir_index].second;
                    while (files[file_index].first.compare(new_path) == 0) {
                        dir_index = get_random_number(dirs.size());
                        new_path = dirs[dir_index].first + dirs[dir_index].second;
                    }
                    rv = rename_file(targets[target], path, new_path + name);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "RenameFile", rv);
                    files.erase(files.begin() + file_index);
                    files.push_back(std::make_pair(new_path, name));
                    LOG_ALWAYS("Move file %s->%s%s", path.c_str(), new_path.c_str(), name.c_str());
                } else {
                    goto add_file;
                }
                break;
            case OP_DELETE_FILE:
                if (files.size() > 0) {
                    index = get_random_number(files.size());
                    file = files.at(index);
                    path = file.first + file.second;
                    rv = delete_file(targets[target], path);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "DeleteFile", rv);
                    files.erase(files.begin() + index);
                    LOG_ALWAYS("Delete file %s", path.c_str());
                } else {
                    goto add_file;
                }
                break;
            case OP_ADD_DIR:
add_dir:
                dir = dirs.at(get_random_number(dirs.size()));
                path = dir.first + dir.second;
                ss.str("");
                ss << "dir_" << time(NULL);
                name = ss.str() + "/";
                rv = create_dir(targets[target], path + name);
                CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CreateDir", rv);
                dirs.push_back(std::make_pair(path, name));
                LOG_ALWAYS("Add directory %s%s", path.c_str(), name.c_str());
                break;
            case OP_RENAME_DIR:
                if (dirs.size() > 1) {
                    index = get_random_number(dirs.size() - 1) + 1;
                    dir = dirs.at(index);
                    path = dir.first + dir.second;
                    name = dir.second;
                    ss.str("");
                    ss << "dir_" << time(NULL);
                    std::string new_name = ss.str() + "/";
                    rv = rename_file(targets[target], path, dir.first + new_name);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "RenameFile", rv);
                    dirs.erase(dirs.begin() + index);
                    dirs.push_back(std::make_pair(dir.first, new_name));
                    int pos;
                    for (int i = dirs.size() - 1; i > 0; i--) {
                        dir = dirs.at(i);
                        if ((pos = dir.first.find(name)) != std::string::npos) {
                            LOG_ALWAYS("Find %s in %s, pos %d", name.c_str(), dir.first.c_str(), pos);
                            dirs.erase(dirs.begin() + i);
                            dir.first.replace(pos, name.size(), new_name.c_str());
                            dirs.push_back(std::make_pair(dir.first, dir.second));
                        }
                    }
                    for (int i = files.size() - 1; i >= 0; i--) {
                        file = files.at(i);
                        if ((pos = file.first.find(name)) != std::string::npos) {
                            LOG_ALWAYS("Find %s in %s, pos %d", name.c_str(), file.first.c_str(), pos);
                            files.erase(files.begin() + i);
                            file.first.replace(pos, name.size(), new_name.c_str());
                            files.push_back(std::make_pair(file.first, file.second));
                        }
                    }
                    LOG_ALWAYS("Rename directory %s->%s", name.c_str(), new_name.c_str());
                } else {
                    goto add_dir;
                }
                break;
            case OP_MOVE_DIR:
                if (dirs.size() > 2) {
                    index = get_random_number(dirs.size() - 1) + 1;
                    path = dirs[index].first;
                    name = dirs[index].second;
                    int new_index = get_random_number(dirs.size());
                    std::string new_path = dirs[new_index].first + dirs[new_index].second;
                    if (name.compare(dirs[new_index].second) == 0 || new_path.find(path) != std::string::npos) {
                        goto add_dir;
                    }
                    rv = rename_file(targets[target], path + name, new_path + name);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "RenameFile", rv);
                    dirs.erase(dirs.begin() + index);
                    dirs.push_back(std::make_pair(new_path, name));
                    path = path + name;
                    new_path = new_path + name;
                    int pos;
                    for (int i = dirs.size() - 1; i > 0; i--) {
                        dir = dirs.at(i);
                        if ((pos = dir.first.find(path)) != std::string::npos) {
                            LOG_ALWAYS("Find %s in %s, pos %d", name.c_str(), dir.first.c_str(), pos);
                            dirs.erase(dirs.begin() + i);
                            dir.first.replace(pos, path.size(), new_path.c_str());
                            dirs.push_back(std::make_pair(dir.first, dir.second));
                        }
                    }
                    for (int i = files.size() - 1; i >= 0; i--) {
                        file = files.at(i);
                        if ((pos = file.first.find(path)) != std::string::npos) {
                            LOG_ALWAYS("Find %s in %s, pos %d", path.c_str(), file.first.c_str(), pos);
                            files.erase(files.begin() + i);
                            file.first.replace(pos, path.size(), new_path.c_str());
                            files.push_back(std::make_pair(file.first, file.second));
                        }
                    }
                    LOG_ALWAYS("Move directory %s->%s", path.c_str(), new_path.c_str());
                } else {
                    goto add_dir;
                }
                break;
            case OP_DELETE_DIR:
                if (dirs.size() > 1) {
                    index = get_random_number(dirs.size() - 1) + 1;
                    dir = dirs.at(index);
                    path = dir.first + dir.second;
                    name = dir.second;
                    rv = delete_dir(targets[target], path);
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "DeleteFile", rv);
                    dirs.erase(dirs.begin() + index);
                    for (int i = dirs.size() - 1; i > 0; i--) {
                        dir = dirs.at(i);
                        if (dir.first.find(name) != std::string::npos) {
                            LOG_ALWAYS("Remove %s%s", dir.first.c_str(), dir.second.c_str());
                            dirs.erase(dirs.begin() + i);
                        }
                    }
                    for (int i = files.size() - 1; i >= 0; i--) {
                        file = files.at(i);
                        if (file.first.find(name) != std::string::npos) {
                            LOG_ALWAYS("Remove %s%s", dir.first.c_str(), dir.second.c_str());
                            files.erase(files.begin() + i);
                        }
                    }
                    LOG_ALWAYS("Delete directory %s", path.c_str());
                } else {
                    goto add_dir;
                }
                break;
        }

        for (unsigned int i = 1; i < (unsigned int)dirs.size(); i++) {
            LOG_ALWAYS("*Dir %s%s", dirs[i].first.c_str(), dirs[i].second.c_str());
        }
        for (unsigned int i = 0; i < (unsigned int)files.size(); i++) {
            LOG_ALWAYS("*File %s%s", files[i].first.c_str(), files[i].second.c_str());
        }
        
        VPLThread_Sleep(VPLTIME_FROM_SEC(1));
        
        LOG_ALWAYS("loop: %d", loop );
        if ((loop % 20) == 0 ) {
            for (int i = 0; i < device_num; i++) {
                if (i != cloudpc) {
                    int retry_count = 0;
                    LOG_ALWAYS("Verifying [%d, %s]", targets[i].instance_num, targets[i].alias.c_str());
                    while (retry_count++ < retry) {
                        LOG_ALWAYS("retry %d", retry_count);
                        src.clear();
                        list_dir(targets[target], "", src);
                        rv = compare_dir(targets[i], "", src, 1);
                        if (rv == 0) {
                            break;
                        }
                    }
                    CHECK_AND_PRINT_RESULT(TEST_SYNCBOX_STR, "CompareDirectory", rv);
                }
            }
            target = get_random_number(device_num);
        }
    }

exit:
    
    LOG_ALWAYS("\n\n==== Stopping CCD ====");
    for (int i = 0; i < device_num; i++) {
        disable_syncbox(targets[i]);
        stop_ccd(targets[i]);
    }

    return rv;
}

static int syncbox_get_sync_folder(int argc, const char* argv[]) {
    
    std::string path;

    if (checkHelp(argc, argv)) {
        printf("%s %s\n", SYNCBOX_STR, argv[0]);
        return 0;   
    }
    return get_sync_folder(path);
}
