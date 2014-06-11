//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "SyncConfigTest.hpp"
#include "SyncConfigTestFiles.hpp"

#include "SyncConfig.hpp"
#include "VcsTestSession.hpp"
#include "VcsTestUtils.hpp"

#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include "gvm_file_utils.hpp"
#include "gvm_rm_utils.hpp"
#include "util_open_db_handle.hpp"
#include "vcs_file_url_access_basic_impl.hpp"
#include "vcs_file_url_operation_http.hpp"

#include "vplex_file.h"
#include "vpl_fs.h"
#include "vpl_types.h"

#include "scopeguard.hpp"

#include <google/protobuf/stubs/common.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h> // for srand/rand
#include <log.h>

enum ClientSpecIndex {
    SYNC_SPEC_CLIENT_1_TWO_WAY = 0,
    SYNC_SPEC_CLIENT_2_TWO_WAY,
    SYNC_SPEC_CLIENT_1_ONE_WAY_UP,
    SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN,
    SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN_PURE_VIRTUAL,
    MAX_SPEC_MAP_SIZE
};

#define TEST_SYNC_TEMP ".sync_temp"

#if defined(IOS) || defined(VPL_PLAT_IS_WINRT) || defined(ANDROID)
static char* root_path = NULL;
static std::string TEST_BASE_DIR;
static std::string TEST_TMP_DIR;
static std::string TEST_MOVE_DIR;
static std::string TEST_DELETE_DIR;
#elif defined(CLOUDNODE)
static std::string TEST_BASE_DIR = "tmp/testSyncConfig";
static std::string TEST_TMP_DIR = TEST_BASE_DIR + "/tmp";
static std::string TEST_MOVE_DIR = TEST_TMP_DIR + "/to_move";
static std::string TEST_DELETE_DIR = TEST_TMP_DIR + "/to_delete";
#else
static std::string TEST_BASE_DIR = std::string(WIN32_DRIVE_LETTER) + "/tmp/testSyncConfig";
static std::string TEST_TMP_DIR = TEST_BASE_DIR + "/tmp";
static std::string TEST_MOVE_DIR = TEST_TMP_DIR + "/to_move";
static std::string TEST_DELETE_DIR = TEST_TMP_DIR + "/to_delete";
#endif

// FAT has the largest granularity of modify time, say 2 seconds
static int MAX_FS_GRANULARITY_SEC = 2;

static SCWTestClient g_TestSpecs[MAX_SPEC_MAP_SIZE];
static void initSyncClientMap()
{
    {
        std::string client_1_local(TEST_BASE_DIR + "/twoway/localClient1");
        std::string client_2_local(TEST_BASE_DIR + "/twoway/localClient2");
        std::string serverPath("twoway");
        {
            SCWTestClient &tss = g_TestSpecs[SYNC_SPEC_CLIENT_1_TWO_WAY];
            tss.localPath = client_1_local;
            tss.serverPath = serverPath;
            tss.syncType = SYNC_TYPE_TWO_WAY;
            tss.syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_DEFAULT;
        }
        {
            SCWTestClient &tss = g_TestSpecs[SYNC_SPEC_CLIENT_2_TWO_WAY];
            tss.localPath = client_2_local;
            tss.serverPath = serverPath;
            tss.syncType = SYNC_TYPE_TWO_WAY;
            tss.syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_DEFAULT;
        }
    }
    {
        std::string client_1_local_ow(TEST_BASE_DIR + "/oneway/localClient1");
        std::string client_2_local_ow(TEST_BASE_DIR + "/oneway/localClient2");
        std::string serverPathOw("oneway");
        {
            SCWTestClient &tss = g_TestSpecs[SYNC_SPEC_CLIENT_1_ONE_WAY_UP];
            tss.localPath = client_1_local_ow;
            tss.serverPath = serverPathOw;
            tss.syncType = SYNC_TYPE_ONE_WAY_UPLOAD;
        }
        {
            SCWTestClient &tss = g_TestSpecs[SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN];
            tss.localPath = client_2_local_ow;
            tss.serverPath = serverPathOw;
            tss.syncType = SYNC_TYPE_ONE_WAY_DOWNLOAD;
        }
        {
            SCWTestClient &tss = g_TestSpecs[SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN_PURE_VIRTUAL];
            tss.localPath = client_2_local_ow;
            tss.serverPath = serverPathOw;
            tss.syncType = SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC;
        }
    }
}

static SCWTestClient* getTestSpec(ClientSpecIndex index)
{
    if(index == MAX_SPEC_MAP_SIZE) {
        return NULL;
    }
    return &g_TestSpecs[index];
}

/////////////////////////////////////////////////////////////////////

#define CHECK_DO_TEST_ERR(err, rv, str)                 \
    do{ if(err != 0) {                                  \
            LOG_ERROR("Do test err,%s: %d", str, err);  \
            rv++;                                       \
        }                                               \
    }while(false)

#define CHECK_DIR_EXIST(dir, totError, totSuccess, msg) \
    do{ VPLFS_stat_t macro_buf;                         \
        int macro_res;                                  \
        macro_res = VPLFS_Stat(dir, &macro_buf);        \
        if(macro_res != VPL_OK) {                       \
            LOG_ERROR("Dir does not exist %s:%s, %d", msg, dir, macro_res); \
            totError++;                                 \
            break;                                      \
        }                                               \
        if(macro_buf.type != VPLFS_TYPE_DIR) {          \
            LOG_ERROR("Not dir %s:%s, %d", msg, dir, macro_buf.type); \
            totError++;                                 \
            break;                                      \
        }                                               \
        totSuccess++;                                   \
    }while(false)

#define CHECK_DENT_NOT_EXIST(dir, totError, totSuccess, msg) \
    do{ VPLFS_stat_t macro_buf;                         \
        int macro_res;                                  \
        macro_res = VPLFS_Stat(dir, &macro_buf);        \
        if(macro_res == VPL_OK) {                       \
            LOG_ERROR("Dent exists %d,%s:%s", (int)macro_buf.type, msg, dir); \
            totError++;                                 \
            break;                                      \
        }                                               \
        totSuccess++;                                   \
    }while(false)

static int deleteLocalFiles(const std::string& path)
{
    int rv = 0;
    int rc;
    rc = Util_rmRecursive(path, TEST_DELETE_DIR);
    if(rc != 0) {
        LOG_ERROR("rm_dash_rf:%d, %s",
                  rc, path.c_str());
        rv = rc;;
    }

    return rv;
}

static int deleteLocalFilesExceptSyncTemp(const std::string& path)
{
    int rv = 0;
    int rc;
    rc = Util_rmRecursiveExcludeDir(path, TEST_DELETE_DIR, SYNC_TEMP_DIR);
    if(rc != 0) {
        LOG_ERROR("rm_dash_rf:%d, %s",
                  rc, path.c_str());
        rv = rc;;
    }

    return rv;
}

static int resetTestState(const SCWTestUserAcct& tua, bool printLog)
{
    int rv = 0;
    int err;

    // Delete local test tmp directory
    err = deleteLocalFiles(std::string(TEST_TMP_DIR));
    CHECK_DO_TEST_ERR(err, rv, "deleteTmp");
    err = Util_CreatePath(TEST_MOVE_DIR.c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirTmp");

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    SCWTestClient* client1ow = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2ow = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);

    err = deleteLocalFiles(client1->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(client2->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");
    err = deleteLocalFiles(client1ow->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(client2ow->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");

    // Check that the clients have the same server dir
    err = (client1->serverPath != client2->serverPath);
    CHECK_DO_TEST_ERR(err,
                      rv,
                      "clientServerSettings");
    if(err) {
        LOG_ERROR("client1 server path:%s does not match "
                  "client2 server path:%s -- Continuing with client1",
                  client1->serverPath.c_str(), client2->serverPath.c_str());
    }
    // Check that the clients have the same server dir
    err = (client1ow->serverPath != client2ow->serverPath);
    CHECK_DO_TEST_ERR(err,
                      rv,
                      "clientServerSettings");
    if(err) {
        LOG_ERROR("client1 server path:%s does not match "
                  "client2 server path:%s -- Continuing with client1",
                  client1ow->serverPath.c_str(), client2ow->serverPath.c_str());
    }

    // Performing the server deletion directly, no need to do the round-about
    // in-band sync that depended on sync working.
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(tua.vcsTestSession, vcsSession);
    u64 rootCompId = 0;
    {
        VPLHttp2 httpHandle;
        err = vcs_get_comp_id(vcsSession,
                              httpHandle,
                              tua.getDataset(),
                              std::string(""),
                              printLog,
                              /*OUT*/ rootCompId);
        CHECK_DO_TEST_ERR(err, rv, "vcs_get_comp_id");
    }
    err = vcsRmFolderRecursive(vcsSession,
                               tua.getDataset(),
                               "",  //client1->serverPath
                               rootCompId,
                               true,
                               printLog);
    CHECK_DO_TEST_ERR(err, rv, "vcsRmFolderRecursive");

    // Create base directories
    err = Util_CreateDir(client1->localPath.c_str());
    CHECK_DO_TEST_ERR(err, rv, "createBaseC1");
    err = Util_CreateDir(client2->localPath.c_str());
    CHECK_DO_TEST_ERR(err, rv, "createBaseC2");
    err = Util_CreateDir(client1ow->localPath.c_str());
    CHECK_DO_TEST_ERR(err, rv, "createBaseC1ow");
    err = Util_CreateDir(client2ow->localPath.c_str());
    CHECK_DO_TEST_ERR(err, rv, "createBaseC2ow");

    return rv;
}

/// A #SyncConfigEventCallback.
static void SyncConfigEventCb(SyncConfigEvent& event)
{
    std::string type;
    switch(event.type) {
    case SYNC_CONFIG_EVENT_STATUS_CHANGE:
        type = "SC_EVENT_STATUS_CHANGE";
        break;
    case SYNC_CONFIG_EVENT_ENTRY_DOWNLOADED:
        type = "SC_EVENT_ENTRY_DOWNLOADED";
        break;
    case SYNC_CONFIG_EVENT_ENTRY_UPLOADED:
        type = "SC_EVENT_ENTRY_UPLOADED";
        break;
    default:
        LOG_ERROR("Unrecognized type:%d", (int)event.type);
        break;
    }
    LOG_ALWAYS("EVENT_RECEIVED: user:"FMTu64",dset:"FMTu64",type:%s,ptr:%p,cb_ctx:%s",
               event.user_id,
               event.dataset_id,
               type.c_str(),
               &event.sync_config,
               (char*)event.callback_context);
}

static VCSFileUrlAccessBasicImpl myFileUrlAccess(CreateVCSFileUrlOperation_HttpImpl, NULL);

int SyncConfigWorkerSync(const SCWTestUserAcct& tua,
                         const SCWTestClient& tss)
{
    DatasetAccessInfo dsetAccessInfo;
    dsetAccessInfo.sessionHandle = tua.vcsTestSession.sessionHandle;
    dsetAccessInfo.serviceTicket = tua.vcsTestSession.sessionServiceTicket;
    dsetAccessInfo.urlPrefix = tua.vcsTestSession.urlPrefix;
    dsetAccessInfo.fileUrlAccess = &myFileUrlAccess;
    SyncConfigThreadPool threadPool;
    SyncConfigThreadPool* threadPoolPtr = NULL;
    if(!threadPool.CheckInitSuccess()) {
        LOG_ERROR("CheckInitSuccess");
    }
    if(tss.syncType == SYNC_TYPE_ONE_WAY_UPLOAD ||
       tss.syncType == SYNC_TYPE_ONE_WAY_DOWNLOAD)
    {
        threadPool.AddGeneralThreads(3);
        threadPoolPtr = &threadPool;
    }

    int rc;
    int rv = 0;
    SyncConfig* syncConfig = CreateSyncConfig(
            tua.vcsTestSession.userId,
            tua.getDataset(),
            tss.syncType,
            tss.syncPolicy,
            tss.localPath,
            tss.serverPath,
            dsetAccessInfo,
            threadPoolPtr,
            true, // dedicatedThread
            SyncConfigEventCb,
            (void*)"TEST_CTX",  // callback_context
            rc,
            true);
    if(rc != 0) {
        LOG_ERROR("SyncConfig:%d", rc);
        return rc;
    }

    rc = syncConfig->Resume();
    if(rc != 0) {
        LOG_ERROR("Resume:%d", rc);
        rv = rc;
        goto exit;
    }

    {
        int i = 0;
        const int MAX_LOOPS = 2000;          // TODO: Have this as absurdly high number for debugging.
        for(; i < MAX_LOOPS; i++) {

            VPLThread_Sleep(VPLTime_FromMillisec(500));

            SyncConfigStatus status_out;
            bool error_out;
            bool work_to_do_out;
            u32 tmp1, tmp2;
            bool tmpb;
            rc = syncConfig->GetSyncStatus(status_out, error_out, work_to_do_out,
                    tmp1, tmp2, tmpb);
            if(rc != 0) {
                LOG_ERROR("GetSyncStatus:%d", rc);
                break;
            }

            if(status_out==SYNC_CONFIG_STATUS_DONE && error_out==0) {  // TODO: work_to_do_out currently wrong.
                LOG_INFO("Sync for %s done. status_out:%d, error_out:%d, work_to_do_out:%d",
                         tss.localPath.c_str(), status_out, error_out, work_to_do_out);
                break;
            }else{
                LOG_INFO("Still syncing %s.  Will wait: status_out:%d, error_out:%d, work_to_do_out:%d",
                         tss.localPath.c_str(), status_out, error_out, work_to_do_out);
            }
        }

        if(i >= MAX_LOOPS) {
            LOG_ERROR("Sync FAILED.");
            rv = SYNC_AGENT_ERR_FAIL;
            goto exit;
        }
    }
 exit:
    {
        int reqCloseRes = syncConfig->RequestClose();
        if(reqCloseRes != 0) {
            LOG_ERROR("RequestClose:%d", reqCloseRes);
            if(rv != 0) { rv = reqCloseRes; }
        }
        if(reqCloseRes==0) {  // No sense in joining if the request failed.
            rc = syncConfig->Join();
            if(rc != 0) {
                LOG_ERROR("Join:%d", rc);
                if(rv != 0) { rv = rc; }
            }
        }
    }

    DestroySyncConfig(syncConfig);

    // There should be no active threads, so this should not be
    // doing anything messy (it will return an error if there's still an active
    // task and prevent other tasks from being enqueued)
    rc = threadPool.CheckAndPrepareShutdown();
    if (rc != 0) {
        u32 unoccupied_threads;
        u32 total_threads;
        bool shuttingDown;
        threadPool.GetInfoIncludeDedicatedThread(NULL,
                                                 /*OUT*/unoccupied_threads,
                                                 /*OUT*/total_threads,
                                                 /*OUT*/shuttingDown);
        LOG_ERROR("threadPool.Shutdown:%d, unoccupied_threads(%d), "
                  "total_threads(%d), shuttingDown(%d).  THERE SHOULD BE NO TASKS!",
                  rc, unoccupied_threads, total_threads, shuttingDown);
        if(rc != 0) { rv = rc; }
    }
    return rv;
}

static int doTest1_create_dirs(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // create client 1 directories
    err = Util_CreatePath((c1Dir+"/testDir1").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = Util_CreatePath((c1Dir+"/testDir2").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = Util_CreatePath((c1Dir+"/testDir3/nest3").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = Util_CreatePath((c1Dir+"/testDir4/nest4/nest44/nest444").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");

    // create client 2 directories
    err = Util_CreatePath((c2Dir+"/testDir5").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC2");
    err = Util_CreatePath((c2Dir+"/testDir6").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC2");
    err = Util_CreatePath((c2Dir+"/testDir7/nest7").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC2");
    err = Util_CreatePath((c2Dir+"/testDir8/nest8/nest88/nest888").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC2");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server, and client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client2's data on server to client1
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    return rv;
}

static void chkTest1_create_dirs(int& error_out,
                                 int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // check directories originally from client1 exist
    CHECK_DIR_EXIST((c1Dir+"/testDir1").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir1").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir2").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir2").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir3/nest3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir3/nest3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir4").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir4/nest4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir4/nest4").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir4/nest4/nest44/nest444").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir4/nest4/nest44/nest444").c_str(), error_out, success_out, "c2");

    // check directories originally from client2 exist
    CHECK_DIR_EXIST((c1Dir+"/testDir5").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir5").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir6").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir6").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir7").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir7").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir7/nest7").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir7/nest7").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir8").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir8").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir8/nest8").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir8/nest8").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir8/nest8/nest88/nest888").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir8/nest8/nest88/nest888").c_str(), error_out, success_out, "c2");
}

// Test removal of directories.  Requires doTest1_create_dirs to be run before.
static int doTest2_remove_dirs(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Remove directories from client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    err = deleteLocalFiles(client1->localPath+std::string("/testDir6"));
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(client1->localPath+std::string("/testDir8/nest8/nest88"));
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");

    // Remove directories from client 2
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    err = deleteLocalFiles(client2->localPath+std::string("/testDir2"));
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");
    err = deleteLocalFiles(client2->localPath+std::string("/testDir4/nest4/nest44"));
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server, and client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client2's data on server to client1
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    return rv;
}

static void chkTest2_remove_dirs(int& error_out,
                                 int& success_out)

{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check that correct directories are missing from both clients.  Originally deleted from client 1
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir6").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir6").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c2");

    // Check that correct directories are missing from both clients.  Originally deleted from client 2
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir2").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir2").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c2");

    // Check that directories not deleted are still present

    // check directories originally from client1 exist
    CHECK_DIR_EXIST((c1Dir+"/testDir1").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir1").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir3/nest3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir3/nest3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir4").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir4/nest4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir4/nest4").c_str(), error_out, success_out, "c2");

    // check directories originally from client2 exist
    CHECK_DIR_EXIST((c1Dir+"/testDir5").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir5").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir7").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir7").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir7/nest7").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir7/nest7").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir8").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir8").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/testDir8/nest8").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir8/nest8").c_str(), error_out, success_out, "c2");
}

static int createRandomFile(const std::string& filename,
                            u64 sizeOfDataFile_bytes,
                            int rand_seed)
{
    int toReturn = 0;
    // For intertwine, see: sw_c/gvm/tests/tools/createRandFile/createRandFile.cpp

    srand(rand_seed);
    const size_t blocksize = (1 << 15);  // 32768
    u8* randBlock = (u8*)malloc(blocksize);
    if(randBlock == NULL) {
        LOG_ERROR("malloc failed\n");
        return -1;
    }

    VPLFile_handle_t fp = VPLFile_Open(filename.c_str(),
                                       VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE,
                                       0777);
    if(!VPLFile_IsValidHandle(fp)) {
        LOG_ERROR("Fail open file:%s, %d", filename.c_str(), (int)fp);
        toReturn = (int)fp;
        goto fail_open;
    }
    for(u64 totalBytesWritten = 0; totalBytesWritten < sizeOfDataFile_bytes;) {
        int myRand;
        for(u32 randIndex = 0; randIndex < blocksize; randIndex += sizeof(myRand)) {
            myRand = rand();
            memcpy(&randBlock[randIndex], &myRand, sizeof(myRand));
        }
        u64 bytesToWrite = sizeOfDataFile_bytes - totalBytesWritten;
        if(bytesToWrite >= blocksize) {
            bytesToWrite = blocksize;
        }
        u32 bytesWritten = VPLFile_Write(fp, randBlock, (size_t)bytesToWrite);
        if(bytesWritten != bytesToWrite) {
            LOG_ERROR("incomplete bytes written %d:"FMTu64", %s\n",
                      bytesWritten, bytesToWrite, filename.c_str());
            toReturn = -3;
            break;
        }
        if(bytesWritten <= 0) {
            LOG_ERROR("writeError:%d,%s at offset:"FMTu64,
                      bytesWritten, filename.c_str(), totalBytesWritten);
            toReturn = bytesWritten;
            break;
        }
        totalBytesWritten += bytesWritten;
    }

    VPLFile_Close(fp);
 fail_open:
    free(randBlock);
    return toReturn;
}

static u32 g_uniqueNum = 0;
// Copy file first before moving to the destination directory to not lose
// the reference file.
static int copyMoveFile(const std::string& src, const std::string& dest)
{
    int rv = 0;
    VPLFS_stat_t statSrc;
    rv = VPLFS_Stat(src.c_str(), &statSrc);
    if(rv != VPL_OK) {
        LOG_ERROR("VPLFS_Stat:%d, %s", rv, src.c_str());
        return rv;
    }
    if(statSrc.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("Source file not file:%d, %s", (int)statSrc.type, src.c_str());
    }

    const int CHUNK_TEMP_BUFFER_SIZE = 4096;
    char* chunkTempBuffer;
    chunkTempBuffer = (char*) malloc(CHUNK_TEMP_BUFFER_SIZE);
    if(!chunkTempBuffer) {
        LOG_ERROR("Out of memory: %d", CHUNK_TEMP_BUFFER_SIZE);
        return -1;
    }

    char uniqueNumber[32];
    sprintf(uniqueNumber, "%d", g_uniqueNum++);
    std::string tempFilename = TEST_MOVE_DIR+
                               std::string("/tmpFile")+
                               std::string(uniqueNumber);

    VPLFile_handle_t fHDst = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t fHSrc = VPLFILE_INVALID_HANDLE;
    const int flagTmp = VPLFILE_OPENFLAG_CREATE |
                        VPLFILE_OPENFLAG_WRITEONLY |
                        VPLFILE_OPENFLAG_TRUNCATE;
    fHDst = VPLFile_Open(tempFilename.c_str(), flagTmp, 0666);
    if (!VPLFile_IsValidHandle(fHDst)) {
        LOG_ERROR("Fail to create temp copy file %s", tempFilename.c_str());
        rv = -1;
        goto exit;
    }

    fHSrc = VPLFile_Open(src.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fHSrc)) {
        LOG_ERROR("Fail to open src %s", src.c_str());
        rv = -1;
        goto exit;
    }

    {  // Perform the copy in chunks
        for (ssize_t bytesTransfered = 0; bytesTransfered < statSrc.size;) {
            ssize_t bytesRead = VPLFile_Read(fHSrc,
                                             chunkTempBuffer,
                                             CHUNK_TEMP_BUFFER_SIZE);
            if (bytesRead > 0) {
                ssize_t wrCnt = VPLFile_Write(fHDst, chunkTempBuffer, bytesRead);
                if (wrCnt != bytesRead) {
                    LOG_ERROR("Fail to write to temp file %s, src:%s, %d/%d",
                              tempFilename.c_str(),
                              src.c_str(),
                              (int)bytesRead,
                              (int)wrCnt);
                    rv = -1;
                    goto exit;
                }
                bytesTransfered += bytesRead;
            } else {
                break;
            }
        }
    }

 exit:
    free(chunkTempBuffer);
    if (VPLFile_IsValidHandle(fHDst)) {
        VPLFile_Close(fHDst);
    }
    if (VPLFile_IsValidHandle(fHSrc)) {
        VPLFile_Close(fHSrc);
    }

    if (rv == 0) {
        rv = VPLFile_Rename(tempFilename.c_str(), dest.c_str());
        if(rv != 0) {
            LOG_ERROR("Moving file:%d, %s->%s",
                      rv, tempFilename.c_str(), dest.c_str());
        }
    }

    return rv;
}

static int createCopyMoveFile(const std::string& refFile,
                              const std::string& dstFile,
                              u64 sizeOfDataFile_bytes,
                              int randSeed)
{
    int rv = 0;

    rv = createRandomFile(refFile, sizeOfDataFile_bytes, randSeed);
    if(rv != 0) {
        LOG_ERROR("createRand:%s, %d, %d",
                  refFile.c_str(), (int)sizeOfDataFile_bytes, randSeed);
        return rv;
    }

    rv = copyMoveFile(refFile, dstFile);
    if(rv != 0) {
        LOG_ERROR("copyMove:%d, %d, %s->%s",
                  rv, (int)sizeOfDataFile_bytes,
                  refFile.c_str(), dstFile.c_str());
        return rv;
    }

    return rv;
}

// Test creation of files.  Requires doTest1_create_dirs to be run before.
static int doTest3_create_file(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Create files on client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    err = createCopyMoveFile(rfDir+"/ref_file1.txt",
                             c1Dir+"/file1.txt",
                             100, 1);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_file2.txt",
                             c1Dir+"/file2.txt",
                             200, 2);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");
    err = createCopyMoveFile(rfDir+"/ref_emptyFile.txt",
                             c1Dir+"/emptyFile.txt",
                             0, 0);
    CHECK_DO_TEST_ERR(err, rv, "ccmEmptyFile");
    err = createCopyMoveFile(rfDir+"/ref_almostChunk.txt",
                             c1Dir+"/almostChunk.txt",
                             32767, 3);
    CHECK_DO_TEST_ERR(err, rv, "ccmAlmostChunk");
    err = createCopyMoveFile(rfDir+"/ref_chunk.txt",
                             c1Dir+"/chunk.txt",
                             32768, 4);
    CHECK_DO_TEST_ERR(err, rv, "ccmChunk");
    err = createCopyMoveFile(rfDir+"/ref_biggerChunk.txt",
                             c1Dir+"/biggerChunk.txt",
                             32769, 5);
    CHECK_DO_TEST_ERR(err, rv, "ccmBiggerChunk");
    err = createCopyMoveFile(rfDir+"/ref_fairlyLarge.txt",
                             c1Dir+"/fairlyLarge.txt",
                             555777, 6);
    CHECK_DO_TEST_ERR(err, rv, "ccmFairlyLarge");
    err = createCopyMoveFile(rfDir+"/ref_file_in_dir1.txt",
                             c1Dir+"/testDir1/file_in_dir1.txt",
                             300, 7);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInDir1");
    err = createCopyMoveFile(rfDir+"/ref_file_in_dir3.txt",
                             c1Dir+"/testDir3/file_in_dir3.txt",
                             400, 8);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInDir3");
    err = createCopyMoveFile(rfDir+"/ref_file1_in_nest3.txt",
                             c1Dir+"/testDir3/nest3/file1_in_nest3.txt",
                             500, 9);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1InNest3");
    err = createCopyMoveFile(rfDir+"/ref_file2_in_nest3.txt",
                             c1Dir+"/testDir3/nest3/file2_in_nest3.txt",
                             600, 10);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2InNest3");
    err = createCopyMoveFile(rfDir+"/ref_file_in_nest4.txt",
                             c1Dir+"/testDir4/nest4/file_in_nest4.txt",
                             700, 11);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInNest4");
    err = Util_CreatePath((c1Dir+"/testDir4/nest4/nest44").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = createCopyMoveFile(rfDir+"/ref_file1_in_nest44.txt",
                             c1Dir+"/testDir4/nest4/nest44/file1_in_nest44.txt",
                             800, 15);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1InNest44");
    err = createCopyMoveFile(rfDir+"/ref_file2_in_nest44.txt",
                             c1Dir+"/testDir4/nest4/nest44/file2_in_nest44.txt",
                             900, 16);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2InNest44");

    // Files for client 2
    err = createCopyMoveFile(rfDir+"/ref_file3.txt",
                             c2Dir+"/file3.txt",
                             101, 21);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile3");
    err = createCopyMoveFile(rfDir+"/ref_same_name1.txt",
                             c2Dir+"/same_name.txt",
                             201, 22);
    CHECK_DO_TEST_ERR(err, rv, "ccmSameName1");
    err = createCopyMoveFile(rfDir+"/ref_same_name2.txt",
                             c2Dir+"/testDir5/same_name.txt",
                             301, 23);
    CHECK_DO_TEST_ERR(err, rv, "ccmSameName2");
    err = createCopyMoveFile(rfDir+"/ref_file_in_dir7.txt",
                             c2Dir+"/testDir7/file_in_dir7.txt",
                             401, 24);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInDir7");
    err = createCopyMoveFile(rfDir+"/ref_file1_in_nest7.txt",
                             c2Dir+"/testDir7/nest7/file1_in_nest7.txt",
                             501, 25);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1InNest7");
    err = createCopyMoveFile(rfDir+"/ref_file2_in_nest7.txt",
                             c2Dir+"/testDir7/nest7/file2_in_nest7.txt",
                             501, 25);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2InNest7");
    err = createCopyMoveFile(rfDir+"/ref_file_in_nest8.txt",
                             c2Dir+"/testDir8/nest8/file_in_nest8.txt",
                             601, 26);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInNest8");
    err = Util_CreatePath((c2Dir+"/testDir8/nest8/nest88").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC2");
    err = createCopyMoveFile(rfDir+"/ref_file1_in_nest88.txt",
                             c2Dir+"/testDir8/nest8/nest88/file1_in_nest88.txt",
                             701, 27);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1InNest88");
    err = createCopyMoveFile(rfDir+"/ref_file2_in_nest88.txt",
                             c2Dir+"/testDir8/nest8/nest88/file2_in_nest88.txt",
                             801, 28);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2InNest88");

    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client1's data to server, and client2's data on server to client1
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

// Returns true if files are identical.
static bool checkFilesExistAndIdentical(const std::string& file1,
                                        const std::string& file2)
{
    VPLFS_stat_t statFile1;
    VPLFS_stat_t statFile2;
    int rc;
    rc = VPLFS_Stat(file1.c_str(), &statFile1);
    if(rc != VPL_OK) {
        LOG_ERROR("file1 does not exist:%d, %s", rc, file1.c_str());
        return false;
    }
    if(statFile1.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("file1 not file:%d, %s", (int)statFile1.type, file1.c_str());
        return false;
    }

    rc = VPLFS_Stat(file2.c_str(), &statFile2);
    if(rc != VPL_OK) {
        LOG_ERROR("file2 does not exist:%d, %s", rc, file2.c_str());
        return false;
    }
    if(statFile2.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("file2 not file:%d, %s", (int)statFile2.type, file2.c_str());
        return false;
    }

    if(statFile1.size != statFile2.size) {
        LOG_ERROR("file1Size:"FMTu64" != file2Size:"FMTu64", file1:%s, file2:%s",
                  (u64)statFile1.size, (u64)statFile2.size, file1.c_str(), file2.c_str());
        return false;
    }

    bool toReturn = false;
    VPLFile_handle_t fh1 = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t fh2 = VPLFILE_INVALID_HANDLE;
    const int CHUNK_TEMP_BUFFER_SIZE = 4096;
    char* chunkBuf1 = NULL;
    char* chunkBuf2 = NULL;
    chunkBuf1 = (char*) malloc(CHUNK_TEMP_BUFFER_SIZE);
    if(!chunkBuf1) {
        LOG_ERROR("Out of memory: %d", CHUNK_TEMP_BUFFER_SIZE);
        goto exit;
    }
    chunkBuf2 = (char*) malloc(CHUNK_TEMP_BUFFER_SIZE);
    if(!chunkBuf2) {
        LOG_ERROR("Out of memory: %d", CHUNK_TEMP_BUFFER_SIZE);
        goto exit;
    }

    fh1 = VPLFile_Open(file1.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fh1)) {
        LOG_ERROR("Fail to open file %s", file1.c_str());
        goto exit;
    }

    fh2 = VPLFile_Open(file2.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fh2)) {
        LOG_ERROR("Fail to open file %s", file2.c_str());
        goto exit;
    }

    {  // Compare in chunks
        for (u64 bytesCompared = 0; bytesCompared < statFile1.size;) {
            ssize_t bytesReadF1 = VPLFile_Read(fh1,
                                               chunkBuf1,
                                               CHUNK_TEMP_BUFFER_SIZE);
            ssize_t bytesReadF2 = VPLFile_Read(fh2,
                                               chunkBuf2,
                                               CHUNK_TEMP_BUFFER_SIZE);
            if(bytesReadF1 != bytesReadF2) {
                LOG_ERROR("Bytes read difference: file1:%d, %s, and file2:%d, %s",
                          (int)bytesReadF1, file1.c_str(),
                          (int)bytesReadF2, file2.c_str());
                goto exit;
            }

            if(bytesReadF1 > 0) {
                if(memcmp(chunkBuf1, chunkBuf2, bytesReadF1) != 0) {
                    LOG_ERROR("Byte difference between file1:%s and file2:%s at offset "FMTu64,
                              file1.c_str(), file2.c_str(), bytesCompared);
                    goto exit;
                }
                bytesCompared += bytesReadF1;
            }else{
                break;
            }
        }
    }

    // Files are identical
    toReturn = true;

 exit:
    if(chunkBuf1){ free(chunkBuf1); }
    if(chunkBuf2){ free(chunkBuf2); }
    if (VPLFile_IsValidHandle(fh1)) { VPLFile_Close(fh1); }
    if (VPLFile_IsValidHandle(fh2)) { VPLFile_Close(fh2); }
    return toReturn;
}

#define CHECK_FILE_TO_REF(refFile, c1File, c2File, error, success)  \
        do{ bool rc1; bool rc2;                                     \
            rc1 = checkFilesExistAndIdentical(refFile, c1File);     \
            if(!rc1) {                                              \
                LOG_ERROR("refFile:%s != c1File:%s",                \
                          (refFile).c_str(), (c1File).c_str());     \
                error++;                                            \
            }                                                       \
            rc2 = checkFilesExistAndIdentical(refFile, c2File);     \
            if(!rc2) {                                              \
                LOG_ERROR("refFile:%s != c2File:%s",                \
                          (refFile).c_str(), (c2File).c_str());     \
                error++;                                            \
            }                                                       \
            if(rc1 && rc2) { success++; }                           \
        }while(false)

#define CHECK_SINGLE_FILE_TO_REF(refFile, c1File, error, success)   \
        do{ bool rc1;                                               \
            rc1 = checkFilesExistAndIdentical(refFile, c1File);     \
            if(!rc1) {                                              \
                LOG_ERROR("refFile:%s != c1File:%s",                \
                          (refFile).c_str(), (c1File).c_str());     \
                error++;                                            \
            }                                                       \
            if(rc1) { success++; }                                  \
        }while(false)

static void chkTest3_create_files(int& error_out,
                                  int& success_out)

{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_file1.txt",
                      c1Dir+"/file1.txt",
                      c2Dir+"/file1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_file2.txt",
                      c1Dir+"/file2.txt",
                      c2Dir+"/file2.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_emptyFile.txt",
                      c1Dir+"/emptyFile.txt",
                      c2Dir+"/emptyFile.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_almostChunk.txt",
                      c1Dir+"/almostChunk.txt",
                      c2Dir+"/almostChunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_chunk.txt",
                      c1Dir+"/chunk.txt",
                      c2Dir+"/chunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_biggerChunk.txt",
                      c1Dir+"/biggerChunk.txt",
                      c2Dir+"/biggerChunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_fairlyLarge.txt",
                      c1Dir+"/fairlyLarge.txt",
                      c2Dir+"/fairlyLarge.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+     "/ref_file_in_dir1.txt",
                      c1Dir+"/testDir1/file_in_dir1.txt",
                      c2Dir+"/testDir1/file_in_dir1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+     "/ref_file_in_dir3.txt",
                      c1Dir+"/testDir3/file_in_dir3.txt",
                      c2Dir+"/testDir3/file_in_dir3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_file1_in_nest3.txt",
                      c1Dir+"/testDir3/nest3/file1_in_nest3.txt",
                      c2Dir+"/testDir3/nest3/file1_in_nest3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_file2_in_nest3.txt",
                      c1Dir+"/testDir3/nest3/file2_in_nest3.txt",
                      c2Dir+"/testDir3/nest3/file2_in_nest3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_file_in_nest4.txt",
                      c1Dir+"/testDir4/nest4/file_in_nest4.txt",
                      c2Dir+"/testDir4/nest4/file_in_nest4.txt", error_out, success_out);
    CHECK_DIR_EXIST((c1Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c2");
    CHECK_FILE_TO_REF(rfDir+                  "/ref_file1_in_nest44.txt",
                      c1Dir+"/testDir4/nest4/nest44/file1_in_nest44.txt",
                      c2Dir+"/testDir4/nest4/nest44/file1_in_nest44.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                  "/ref_file2_in_nest44.txt",
                      c1Dir+"/testDir4/nest4/nest44/file2_in_nest44.txt",
                      c2Dir+"/testDir4/nest4/nest44/file2_in_nest44.txt", error_out, success_out);

    // Check files originally on client 2
    CHECK_FILE_TO_REF(rfDir+"/ref_file3.txt",
                      c1Dir+"/file3.txt",
                      c2Dir+"/file3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_same_name1.txt",
                      c1Dir+"/same_name.txt",
                      c2Dir+"/same_name.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_same_name2.txt",
                      c1Dir+"/testDir5/same_name.txt",
                      c2Dir+"/testDir5/same_name.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+     "/ref_file_in_dir7.txt",
                      c1Dir+"/testDir7/file_in_dir7.txt",
                      c2Dir+"/testDir7/file_in_dir7.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_file1_in_nest7.txt",
                      c1Dir+"/testDir7/nest7/file1_in_nest7.txt",
                      c2Dir+"/testDir7/nest7/file1_in_nest7.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_file2_in_nest7.txt",
                      c1Dir+"/testDir7/nest7/file2_in_nest7.txt",
                      c2Dir+"/testDir7/nest7/file2_in_nest7.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_file_in_nest8.txt",
                      c1Dir+"/testDir8/nest8/file_in_nest8.txt",
                      c2Dir+"/testDir8/nest8/file_in_nest8.txt", error_out, success_out);
    CHECK_DIR_EXIST((c1Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c2");
    CHECK_FILE_TO_REF(rfDir+                  "/ref_file1_in_nest88.txt",
                      c1Dir+"/testDir8/nest8/nest88/file1_in_nest88.txt",
                      c2Dir+"/testDir8/nest8/nest88/file1_in_nest88.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                  "/ref_file2_in_nest88.txt",
                      c1Dir+"/testDir8/nest8/nest88/file2_in_nest88.txt",
                      c2Dir+"/testDir8/nest8/nest88/file2_in_nest88.txt", error_out, success_out);
}

// Test update of files.  Requires doTest3_create_file to be run before.
static int doTest4_update_file(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Create files on client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    err = createCopyMoveFile(rfDir+"/ref_update_file1.txt",
                             c1Dir+"/file1.txt",
                             100, 50);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_update_file2.txt",
                             c1Dir+"/file2.txt",
                             150, 51);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmFile2");
    err = createCopyMoveFile(rfDir+"/ref_update_almostChunk.txt",
                             c1Dir+"/almostChunk.txt",
                             0, 0);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmTruncate");
    err = createCopyMoveFile(rfDir+"/ref_update_file1_in_nest44.txt",
                             c1Dir+"/testDir4/nest4/nest44/file1_in_nest44.txt",
                             800, 52);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmFile1InNest44");
    err = createCopyMoveFile(rfDir+"/ref_fairlyLarge.txt",
                             c1Dir+"/fairlyLarge.txt",
                             555777, 7);
    CHECK_DO_TEST_ERR(err, rv, "ccmFairlyLarge");
    err = createCopyMoveFile(rfDir+"/ref_biggerChunk.txt",
                             c1Dir+"/biggerChunk.txt",
                             32769, 5);
    CHECK_DO_TEST_ERR(err, rv, "ccmBiggerChunk");

    // Files for client 2
    err = createCopyMoveFile(rfDir+"/ref_update_file3.txt",
                             c2Dir+"/file3.txt",
                             101, 53);
    err = createCopyMoveFile(rfDir+"/ref_update_file_in_nest8.txt",
                             c2Dir+"/testDir8/nest8/file_in_nest8.txt",
                             600, 54);

    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client1's data to server, and client2's data on server to client1
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest4_update_files(int& error_out,
                                  int& success_out)

{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_update_file1.txt",
                      c1Dir+"/file1.txt",
                      c2Dir+"/file1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_update_file2.txt",
                      c1Dir+"/file2.txt",
                      c2Dir+"/file2.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_update_almostChunk.txt",
                      c1Dir+"/almostChunk.txt",
                      c2Dir+"/almostChunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_update_file1_in_nest44.txt",
                      c1Dir+"/testDir4/nest4/nest44/file1_in_nest44.txt",
                      c2Dir+"/testDir4/nest4/nest44/file1_in_nest44.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_fairlyLarge.txt",
                      c1Dir+"/fairlyLarge.txt",
                      c2Dir+"/fairlyLarge.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_biggerChunk.txt",
                      c1Dir+"/biggerChunk.txt",
                      c2Dir+"/biggerChunk.txt", error_out, success_out);

    // Check files originally on client 2
    CHECK_FILE_TO_REF(rfDir+"/ref_update_file3.txt",
                      c1Dir+"/file3.txt",
                      c2Dir+"/file3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+    "/ref_update_file_in_nest8.txt",
                      c1Dir+"/testDir8/nest8/file_in_nest8.txt",
                      c2Dir+"/testDir8/nest8/file_in_nest8.txt", error_out, success_out);
}

// Test removal of files.  Requires doTest3_create_files to be run before.
static int doTest5_remove_file(const SCWTestUserAcct& tua)
{
    int rv = 0;
    int err;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Remove from Client 1
    err = deleteLocalFiles(c1Dir+"/file3.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/testDir5/same_name.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/testDir7/file_in_dir7.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/testDir7/nest7/file2_in_nest7.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/testDir8/nest8/nest88");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");

    // Remove from Client 2
    err = deleteLocalFiles(c2Dir+"/file2.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");
    err = deleteLocalFiles(c2Dir+"/testDir1/file_in_dir1.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");
    err = deleteLocalFiles(c2Dir+"/testDir3/file_in_dir3.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");
    err = deleteLocalFiles(c2Dir+"/testDir3/nest3/file2_in_nest3.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");
    err = deleteLocalFiles(c2Dir+"/testDir4/nest4/nest44");
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server, and client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client2's data on server to client1
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    return rv;
}

static void chkTest5_remove_files(int& error_out,
                                  int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally removed from Client 1
    CHECK_DENT_NOT_EXIST((c1Dir+"/file3.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/file3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir5/same_name.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir5/same_name.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir7/file_in_dir7.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir7/file_in_dir7.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir7/nest7/file2_in_nest7.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir7/nest7/file2_in_nest7.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir8/nest8/nest88").c_str(), error_out, success_out, "c2");

    // Check files originally removed from Client 2
    CHECK_DENT_NOT_EXIST((c1Dir+"/file2.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/file2.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir1/file_in_dir1.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir1/file_in_dir1.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir3/file_in_dir3.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir3/file_in_dir3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir3/nest3/file2_in_nest3.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir3/nest3/file2_in_nest3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/testDir4/nest4/nest44").c_str(), error_out, success_out, "c2");

    // Make sure files that are supposed to be there are still there

    // Files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_update_file1.txt",
                      c1Dir+"/file1.txt",
                      c2Dir+"/file1.txt", error_out, success_out);
    CHECK_DIR_EXIST((c1Dir+"/testDir1").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir1").c_str(), error_out, success_out, "c2");
    CHECK_FILE_TO_REF(rfDir+           "/ref_file1_in_nest3.txt",
                      c1Dir+"/testDir3/nest3/file1_in_nest3.txt",
                      c2Dir+"/testDir3/nest3/file1_in_nest3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+           "/ref_file_in_nest4.txt",
                      c1Dir+"/testDir4/nest4/file_in_nest4.txt",
                      c2Dir+"/testDir4/nest4/file_in_nest4.txt", error_out, success_out);

    // Files originally in client 2
    CHECK_FILE_TO_REF(rfDir+"/ref_same_name1.txt",
                      c1Dir+"/same_name.txt",
                      c2Dir+"/same_name.txt", error_out, success_out);
    CHECK_DIR_EXIST((c1Dir+"/testDir5").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/testDir5").c_str(), error_out, success_out, "c2");
    CHECK_FILE_TO_REF(rfDir+           "/ref_file1_in_nest7.txt",
                      c1Dir+"/testDir7/nest7/file1_in_nest7.txt",
                      c2Dir+"/testDir7/nest7/file1_in_nest7.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+    "/ref_update_file_in_nest8.txt",
                      c1Dir+"/testDir8/nest8/file_in_nest8.txt",
                      c2Dir+"/testDir8/nest8/file_in_nest8.txt", error_out, success_out);
}

static int setupTestAcctSession(SCWTestUserAcct& tua)
{
    int rc;

    rc = vcsGetTestSession(tua.ias_central_url,
                           tua.ias_central_port,
                           tua.acctNamespace,
                           tua.username,
                           tua.password,
                           tua.vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("vcs_get_test_session:%d, (%s,%d,%s,%s,%s)",
                  rc,
                  tua.ias_central_url.c_str(),
                  tua.ias_central_port,
                  tua.acctNamespace.c_str(),
                  tua.username.c_str(),
                  tua.password.c_str());
        return rc;
    }

    rc = vcsGetDatasetIdFromName(tua.vcsTestSession,
                                 tua.getDatasetName(),
                                 tua.datasetNum);
    if(rc != 0) {
        LOG_ERROR("vcsGetDatasetIdFromName:%d, (%s)",
                  rc, tua.vcsTestSession.serverHostname.c_str());
        return rc;
    }

    return 0;
}

static int doTest6_create_dirs_OW(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // create client 1 directories
    err = Util_CreatePath((c1Dir+"/ow_testDir1").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = Util_CreatePath((c1Dir+"/ow_testDir2").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = Util_CreatePath((c1Dir+"/ow_testDir3/ow_nest3").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = Util_CreatePath((c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_nest444").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server, and client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest6_create_dirs_OW(int& error_out,
                                    int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // check directories originally from client1 exist
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir1").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir1").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir2").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir2").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir3/ow_nest3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir3/ow_nest3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir4").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir4/ow_nest4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir4/ow_nest4").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_nest444").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_nest444").c_str(), error_out, success_out, "c2");
}

// Test removal of directories.  Requires doTest1_create_dirs to be run before.
static int doTest7_remove_dirs_OW(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;


    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);

    // Remove directories from client 1
    err = deleteLocalFiles(client1->localPath+std::string("/ow_testDir2"));
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(client1->localPath+std::string("/ow_testDir4/ow_nest4/ow_nest44"));
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server, and client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");


    return rv;
}

static void chkTest7_remove_dirs_OW(int& error_out,
                                 int& success_out)

{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check that correct directories are missing from both clients.  Originally deleted from client 2
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir2").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir2").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c2");

    // Check that directories not deleted are still present

    // check directories originally from client1 exist
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir1").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir1").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir3/ow_nest3").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir3/ow_nest3").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir4").c_str(), error_out, success_out, "c2");
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir4/ow_nest4").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir4/ow_nest4").c_str(), error_out, success_out, "c2");
}

// Test creation of files.  Requires doTest1_create_dirs to be run before.
static int doTest8_create_file_OW(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Create files on client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    err = createCopyMoveFile(rfDir+"/ow_ref_file1.txt",
                             c1Dir+"/ow_file1.txt",
                             100, 71);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ow_ref_file2.txt",
                             c1Dir+"/ow_file2.txt",
                             200, 72);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");
    err = createCopyMoveFile(rfDir+"/ow_ref_emptyFile.txt",
                             c1Dir+"/ow_emptyFile.txt",
                             0, 0);
    CHECK_DO_TEST_ERR(err, rv, "ccmEmptyFile");
    err = createCopyMoveFile(rfDir+"/ow_ref_almostChunk.txt",
                             c1Dir+"/ow_almostChunk.txt",
                             32767, 73);
    CHECK_DO_TEST_ERR(err, rv, "ccmAlmostChunk");
    err = createCopyMoveFile(rfDir+"/ow_ref_chunk.txt",
                             c1Dir+"/ow_chunk.txt",
                             32768, 74);
    CHECK_DO_TEST_ERR(err, rv, "ccmChunk");
    err = createCopyMoveFile(rfDir+"/ow_ref_biggerChunk.txt",
                             c1Dir+"/ow_biggerChunk.txt",
                             32769, 75);
    CHECK_DO_TEST_ERR(err, rv, "ccmBiggerChunk");
    err = createCopyMoveFile(rfDir+"/ow_ref_fairlyLarge.txt",
                             c1Dir+"/ow_fairlyLarge.txt",
                             555777, 76);
    CHECK_DO_TEST_ERR(err, rv, "ccmFairlyLarge");
    err = createCopyMoveFile(rfDir+"/ow_ref_file_in_dir1.txt",
                             c1Dir+"/ow_testDir1/ow_file_in_dir1.txt",
                             300, 77);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInDir1");
    err = createCopyMoveFile(rfDir+"/ow_ref_file_in_dir3.txt",
                             c1Dir+"/ow_testDir3/ow_file_in_dir3.txt",
                             400, 78);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInDir3");
    err = createCopyMoveFile(rfDir+"/ow_ref_file1_in_nest3.txt",
                             c1Dir+"/ow_testDir3/ow_nest3/ow_file1_in_nest3.txt",
                             500, 79);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1InNest3");
    err = createCopyMoveFile(rfDir+"/ow_ref_file2_in_nest3.txt",
                             c1Dir+"/ow_testDir3/ow_nest3/ow_file2_in_nest3.txt",
                             600, 80);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2InNest3");
    err = createCopyMoveFile(rfDir+"/ow_ref_file_in_nest4.txt",
                             c1Dir+"/ow_testDir4/ow_nest4/ow_file_in_nest4.txt",
                             700, 81);
    CHECK_DO_TEST_ERR(err, rv, "ccmFileInNest4");
    err = Util_CreatePath((c1Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "mkdirC1");
    err = createCopyMoveFile(rfDir+"/ow_ref_file1_in_nest44.txt",
                             c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file1_in_nest44.txt",
                             800, 85);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1InNest44");
    err = createCopyMoveFile(rfDir+"/ow_ref_file2_in_nest44.txt",
                             c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file2_in_nest44.txt",
                             900, 86);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2InNest44");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest8_create_files_OW(int& error_out,
                                     int& success_out)

{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_file1.txt",
                      c1Dir+"/ow_file1.txt",
                      c2Dir+"/ow_file1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_file2.txt",
                      c1Dir+"/ow_file2.txt",
                      c2Dir+"/ow_file2.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_emptyFile.txt",
                      c1Dir+"/ow_emptyFile.txt",
                      c2Dir+"/ow_emptyFile.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_almostChunk.txt",
                      c1Dir+"/ow_almostChunk.txt",
                      c2Dir+"/ow_almostChunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_chunk.txt",
                      c1Dir+"/ow_chunk.txt",
                      c2Dir+"/ow_chunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_biggerChunk.txt",
                      c1Dir+"/ow_biggerChunk.txt",
                      c2Dir+"/ow_biggerChunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_fairlyLarge.txt",
                      c1Dir+"/ow_fairlyLarge.txt",
                      c2Dir+"/ow_fairlyLarge.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+        "/ow_ref_file_in_dir1.txt",
                      c1Dir+"/ow_testDir1/ow_file_in_dir1.txt",
                      c2Dir+"/ow_testDir1/ow_file_in_dir1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+        "/ow_ref_file_in_dir3.txt",
                      c1Dir+"/ow_testDir3/ow_file_in_dir3.txt",
                      c2Dir+"/ow_testDir3/ow_file_in_dir3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                 "/ow_ref_file1_in_nest3.txt",
                      c1Dir+"/ow_testDir3/ow_nest3/ow_file1_in_nest3.txt",
                      c2Dir+"/ow_testDir3/ow_nest3/ow_file1_in_nest3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                 "/ow_ref_file2_in_nest3.txt",
                      c1Dir+"/ow_testDir3/ow_nest3/ow_file2_in_nest3.txt",
                      c2Dir+"/ow_testDir3/ow_nest3/ow_file2_in_nest3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                 "/ow_ref_file_in_nest4.txt",
                      c1Dir+"/ow_testDir4/ow_nest4/ow_file_in_nest4.txt",
                      c2Dir+"/ow_testDir4/ow_nest4/ow_file_in_nest4.txt", error_out, success_out);
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c2");
    CHECK_FILE_TO_REF(rfDir+                     "/ow_ref_file1_in_nest44.txt",
                      c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file1_in_nest44.txt",
                      c2Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file1_in_nest44.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                           "/ow_ref_file2_in_nest44.txt",
                      c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file2_in_nest44.txt",
                      c2Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file2_in_nest44.txt", error_out, success_out);
}

static int doTest8a_sync_type_switch_OWD(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Sync Down Pure virtual (should delete files already present).
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN_PURE_VIRTUAL);
    std::string c2Dir = client2->localPath;

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest8a_sync_type_switch_OWD(int& error_out,
                                           int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files do not exist on client 2 (pure virtual sync should have deleted them).
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file1.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file2.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_emptyFile.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_almostChunk.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_chunk.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_biggerChunk.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_fairlyLarge.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file_in_dir1.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file_in_dir3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file1_in_nest3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file2_in_nest3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file_in_nest4.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file1_in_nest44.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_ref_file2_in_nest44.txt").c_str(), error_out, success_out, "c2");
}

static int doTest8b_sync_type_switch_OWD(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Regular Sync Down after pure virtual (should bring files back).
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string c2Dir = client2->localPath;

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest8b_sync_type_switch_OWD(int& error_out,
                                           int& success_out)
{
    chkTest8_create_files_OW(error_out, success_out);
}

// Test update of files.  Requires doTest3_create_file to be run before.
static int doTest9_update_file_OW(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Create files on client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    err = createCopyMoveFile(rfDir+"/ow_ref_update_file1.txt",
                             c1Dir+"/ow_file1.txt",
                             100, 750);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmFile1");
    err = createCopyMoveFile(rfDir+"/ow_ref_update_file2.txt",
                             c1Dir+"/ow_file2.txt",
                             150, 751);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmFile2");
    err = createCopyMoveFile(rfDir+"/ow_ref_update_almostChunk.txt",
                             c1Dir+"/ow_almostChunk.txt",
                             0, 0);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmTruncate");
    err = createCopyMoveFile(rfDir+"/ow_ref_update_file1_in_nest44.txt",
                             c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file1_in_nest44.txt",
                             800, 752);
    CHECK_DO_TEST_ERR(err, rv, "update_ccmFile1InNest44");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest9_update_files_OW(int& error_out,
                                     int& success_out)

{
    error_out = 0;
    success_out = 0;

    // Create files on client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_update_file1.txt",
                      c1Dir+"/ow_file1.txt",
                      c2Dir+"/ow_file1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_update_file2.txt",
                      c1Dir+"/ow_file2.txt",
                      c2Dir+"/ow_file2.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_update_almostChunk.txt",
                      c1Dir+"/ow_almostChunk.txt",
                      c2Dir+"/ow_almostChunk.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                    "/ow_ref_update_file1_in_nest44.txt",
                      c1Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file1_in_nest44.txt",
                      c2Dir+"/ow_testDir4/ow_nest4/ow_nest44/ow_file1_in_nest44.txt", error_out, success_out);
}

// Test removal of files.  Requires doTest3_create_files to be run before.
static int doTest10_remove_file_OW(const SCWTestUserAcct& tua)
{
    int rv = 0;
    int err;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Remove from Client 1
    err = deleteLocalFiles(c1Dir+"/ow_file2.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/ow_testDir1/ow_file_in_dir1.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/ow_testDir3/ow_file_in_dir3.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/ow_testDir3/ow_nest3/ow_file2_in_nest3.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = deleteLocalFiles(c1Dir+"/ow_testDir4/ow_nest4/ow_nest44");
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest10_remove_files_OW(int& error_out,
                                      int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files removed from Client 1
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_file2.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_file2.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir1/ow_file_in_dir1.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir1/ow_file_in_dir1.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir3/ow_file_in_dir3.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir3/ow_file_in_dir3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir3/ow_nest3/ow_file2_in_nest3.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir3/ow_nest3/ow_file2_in_nest3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir4/ow_nest4/ow_nest44").c_str(), error_out, success_out, "c2");

    // Make sure files that are supposed to be there are still there

    // Files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_update_file1.txt",
                      c1Dir+"/ow_file1.txt",
                      c2Dir+"/ow_file1.txt", error_out, success_out);
    CHECK_DIR_EXIST((c1Dir+"/ow_testDir1").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/ow_testDir1").c_str(), error_out, success_out, "c2");
    CHECK_FILE_TO_REF(rfDir+                 "/ow_ref_file1_in_nest3.txt",
                      c1Dir+"/ow_testDir3/ow_nest3/ow_file1_in_nest3.txt",
                      c2Dir+"/ow_testDir3/ow_nest3/ow_file1_in_nest3.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+                 "/ow_ref_file_in_nest4.txt",
                      c1Dir+"/ow_testDir4/ow_nest4/ow_file_in_nest4.txt",
                      c2Dir+"/ow_testDir4/ow_nest4/ow_file_in_nest4.txt", error_out, success_out);
}

// Test initial sync.  Triggered by removal of DB.
static int doTest11_initial_sync_OW(const SCWTestUserAcct& tua)
{
    int rv = 0;
    int err;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    err = deleteLocalFiles(client1->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = Util_CreatePath(client1->localPath.c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "makeClientDir");
    // Delete only the sync temp directory.  See if the unneeded files get removed.
    err = deleteLocalFiles(client2->localPath+"/"TEST_SYNC_TEMP);
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");

    err = createCopyMoveFile(rfDir+"/ow_ref_TheOneSingleFile.txt",
                             c1Dir+"/ow_TheOneSingleFile.txt",
                             75755, 800);
    CHECK_DO_TEST_ERR(err, rv, "OneSingleFile");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static int countEntriesInDir(const std::string& directory,
                                   u32& numEntries_out)
{
    numEntries_out = 0;
    VPLFS_dir_t dirStream;
    int rc = VPLFS_Opendir(directory.c_str(), &dirStream);
    if (rc != 0) {
        LOG_ERROR("VPLFS_Opendir(%s) failed: %d", directory.c_str(), rc);
        return rc;
    }
    ON_BLOCK_EXIT(VPLFS_Closedir, &dirStream);

    // For each dirEntry in currDir:
    VPLFS_dirent_t dirEntry;
    while ((rc = VPLFS_Readdir(&dirStream, &dirEntry)) == VPL_OK) {
        // Ignore special directories and filenames that contain unsupported characters.
        std::string entryName = dirEntry.filename;
        if (entryName == "." ||
            entryName == ".." ||
            entryName == TEST_SYNC_TEMP)
        {
            continue;
        }
        numEntries_out++;
    }
    return 0;
}

static void chkTest11_initial_sync_OW(int& error_out,
                                      int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_TheOneSingleFile.txt",
                      c1Dir+"/ow_TheOneSingleFile.txt",
                      c2Dir+"/ow_TheOneSingleFile.txt", error_out, success_out);
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_file1.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_file1.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir1").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir1").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir3/ow_nest3/ow_file1_in_nest3.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir3/ow_nest3/ow_file1_in_nest3.txt").c_str(), error_out, success_out, "c2");
    CHECK_DENT_NOT_EXIST((c1Dir+"/ow_testDir4/ow_nest4/ow_file_in_nest4.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/ow_testDir4/ow_nest4/ow_file_in_nest4.txt").c_str(), error_out, success_out, "c2");

    // Check to make sure there are exactly the amount of files expected.
    u32 numEntries;
    int rc = countEntriesInDir(c2Dir, /*out*/ numEntries);
    if (rc != 0) {
        LOG_ERROR("countEntriesInDir(%s):%d", c2Dir.c_str(), rc);
        error_out++;
        return;
    }

    // Check that there's only 1 file.
    if (numEntries != 1) {
        LOG_ERROR("Bad count in (%s):%d.  Should be %d",
                  c2Dir.c_str(), numEntries, 1);
        error_out++;
    } else {
        success_out++;
    }
}

// 5 Conflict test cases to try
//
//        | Create | Update | Delete
// =======+========+========+========
// Create |  test  |   NA   |   NA
// Update |   NA   |  test  |  test
// Delete |   NA   |  test  |  test-justInCase
static int doTest12_A_file_conflict_1stToServerWins_loserDelete(
                                                const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Create files on client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Ensure default conflict policies are what we want to test.
    if(client1->syncPolicy.how_to_determine_winner !=
            SyncPolicy::SYNC_CONFLICT_POLICY_FIRST_TO_SERVER_WINS)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client1 not set to FIRST_TO_SERVER_WINS");
    }
    if(client2->syncPolicy.how_to_determine_winner !=
                SyncPolicy::SYNC_CONFLICT_POLICY_FIRST_TO_SERVER_WINS)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client2 not set to FIRST_TO_SERVER_WINS");
    }
    if(client1->syncPolicy.what_to_do_with_loser !=
            SyncPolicy::SYNC_CONFLICT_POLICY_DELETE_LOSER)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client1 not set to POLICY_DELETE_LOSER");
    }
    if(client2->syncPolicy.what_to_do_with_loser !=
            SyncPolicy::SYNC_CONFLICT_POLICY_DELETE_LOSER)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client2 not set to POLICY_DELETE_LOSER");
    }

    // Setup Test Files - Expects (but verifies) that sync works.
    {
        // Create client paths, paths may not exist before these test files can be placed.
        err = Util_CreatePath(client1->localPath.c_str(), true);
        CHECK_DO_TEST_ERR(err, rv, "mkdir");
        err = Util_CreatePath(client2->localPath.c_str(), true);
        CHECK_DO_TEST_ERR(err, rv, "mkdir");

        err = createCopyMoveFile(rfDir+"/ref_a_updateConflict_client1.txt",
                                 c1Dir+"/a_updateConflict_client1.txt",
                                 300, 1700);
        CHECK_DO_TEST_ERR(err, rv, "a_ccmFile1");
        err = createCopyMoveFile(rfDir+"/ref_a_updateConflict_client2.txt",
                                 c1Dir+"/a_updateConflict_client2.txt",
                                 300, 1705);
        CHECK_DO_TEST_ERR(err, rv, "a_ccmFile2");
        err = createCopyMoveFile(rfDir+"/ref_a_updateWinDelete_client1.txt",
                                 c1Dir+"/a_updateWinDeleteConflict_client1.txt",
                                 300, 1710);
        CHECK_DO_TEST_ERR(err, rv, "a_ccmFile1");
        err = createCopyMoveFile(rfDir+"/ref_a_updateWinDelete_client2.txt",
                                 c1Dir+"/a_updateWinDeleteConflict_client2.txt",
                                 300, 1715);
        err = createCopyMoveFile(rfDir+"/ref_a_updateDeleteWin_client1.txt",
                                 c1Dir+"/a_updateDeleteWinConflict_client1.txt",
                                 300, 1720);
        CHECK_DO_TEST_ERR(err, rv, "a_ccmFile1");
        err = createCopyMoveFile(rfDir+"/ref_a_updateDeleteWin_client2.txt",
                                 c1Dir+"/a_updateDeleteWinConflict_client2.txt",
                                 300, 1725);
        CHECK_DO_TEST_ERR(err, rv, "a_ccmFile2");
        err = createCopyMoveFile(rfDir+"/ref_a_deleteDelete_client.txt",
                                 c1Dir+"/a_deleteDeleteConflict_client.txt",
                                 300, 1730);
        CHECK_DO_TEST_ERR(err, rv, "a_ccmFile2");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "a_syncC1");
        // Sync client2's data to server
        err = SyncConfigWorkerSync(tua, *client2);
        CHECK_DO_TEST_ERR(err, rv, "a_syncC2");

        // Verify sync happened:
        int error_out = 0;
        int unused_out = 0;

        CHECK_FILE_TO_REF(rfDir+"/ref_a_updateConflict_client1.txt",
                          c1Dir+"/a_updateConflict_client1.txt",
                          c2Dir+"/a_updateConflict_client1.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_a_updateConflict_client2.txt",
                          c1Dir+"/a_updateConflict_client2.txt",
                          c2Dir+"/a_updateConflict_client2.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_a_updateWinDelete_client1.txt",
                          c1Dir+"/a_updateWinDeleteConflict_client1.txt",
                          c2Dir+"/a_updateWinDeleteConflict_client1.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_a_updateWinDelete_client2.txt",
                          c1Dir+"/a_updateWinDeleteConflict_client2.txt",
                          c2Dir+"/a_updateWinDeleteConflict_client2.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_a_updateDeleteWin_client1.txt",
                          c1Dir+"/a_updateDeleteWinConflict_client1.txt",
                          c2Dir+"/a_updateDeleteWinConflict_client1.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_a_updateDeleteWin_client2.txt",
                          c1Dir+"/a_updateDeleteWinConflict_client2.txt",
                          c2Dir+"/a_updateDeleteWinConflict_client2.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_a_deleteDelete_client.txt",
                          c1Dir+"/a_deleteDeleteConflict_client.txt",
                          c2Dir+"/a_deleteDeleteConflict_client.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
    }
    /////////////////////// CLIENT 1 WINS ////////////////////////////////////

    // Create conflict winner client1
    err = createCopyMoveFile(rfDir+"/ref_a_conflict_createfile_client1.txt",
                             c1Dir+"/a_conflict_createfile_client1.txt",
                             300, 1735);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_a_conflict_createfile_client1_lose.txt",
                             c2Dir+"/a_conflict_createfile_client1.txt",
                             300, 1736);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");

    // Update conflict winner client1
    err = createCopyMoveFile(rfDir+"/ref_a_updateConflict_client1_win.txt",
                             c1Dir+"/a_updateConflict_client1.txt",
                             300, 1701);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_a_updateConflict_client1_lose.txt",
                             c2Dir+"/a_updateConflict_client1.txt",
                             300, 1702);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");

    // UpdateWinDelete winner client1
    err = createCopyMoveFile(rfDir+"/ref_a_updateWinDelete_client1_win.txt",
                             c1Dir+"/a_updateWinDeleteConflict_client1.txt",
                             300, 1711);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = Util_rmRecursive(c2Dir+"/a_updateWinDeleteConflict_client1.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");

    // UpdateDeleteWin winner client1
    err = Util_rmRecursive(c1Dir+"/a_updateDeleteWinConflict_client1.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");
    err = createCopyMoveFile(rfDir+"/ref_a_updateDeleteWin_client1_win.txt",
                             c2Dir+"/a_updateDeleteWinConflict_client1.txt",
                             300, 1721);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");

    err = Util_rmRecursive(c1Dir+"/a_deleteDeleteConflict_client.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");
    err = Util_rmRecursive(c2Dir+"/a_deleteDeleteConflict_client.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server -- CONFLICT
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client1's data to server -- Propagate override of deleteWin in updateDelete (to preserve data).
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");


    /////////////////////// CLIENT 2 WINS ////////////////////////////////////
    // Create conflict winner client2
    err = createCopyMoveFile(rfDir+"/ref_a_conflict_createfile_client2_lose.txt",
                             c1Dir+"/a_conflict_createfile_client2.txt",
                             300, 1737);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_a_conflict_createfile_client2.txt",
                             c2Dir+"/a_conflict_createfile_client2.txt",
                             300, 1738);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");

    // Update conflict winner client2
    err = createCopyMoveFile(rfDir+"/ref_a_updateConflict_client2_lose.txt",
                             c1Dir+"/a_updateConflict_client2.txt",
                             300, 1706);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_a_updateConflict_client2_wing.txt",
                             c2Dir+"/a_updateConflict_client2.txt",
                             300, 1707);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");

    // UpdateWinDelete winner client2
    err = createCopyMoveFile(rfDir+"/ref_a_updateWinDelete_client2_win.txt",
                             c2Dir+"/a_updateWinDeleteConflict_client2.txt",
                             300, 1716);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = Util_rmRecursive(c1Dir+"/a_updateWinDeleteConflict_client2.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");

    // UpdateDeleteWin winner client2
    err = Util_rmRecursive(c2Dir+"/a_updateDeleteWinConflict_client2.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");
    err = createCopyMoveFile(rfDir+"/ref_a_updateDeleteWin_client2_win.txt",
                             c1Dir+"/a_updateDeleteWinConflict_client2.txt",
                             300, 1726);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");


    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client1's data to server -- CONFLICT
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server -- Propagate override of deleteWin in updateDelete (to preserve data).
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest12_A_file_conflict_first2ServerWins_loserDelete(
                                                int& error_out,
                                                int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Create file tests
    CHECK_FILE_TO_REF(rfDir+"/ref_a_conflict_createfile_client1.txt",
                      c1Dir+"/a_conflict_createfile_client1.txt",
                      c2Dir+"/a_conflict_createfile_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_a_conflict_createfile_client2.txt",
                      c1Dir+"/a_conflict_createfile_client2.txt",
                      c2Dir+"/a_conflict_createfile_client2.txt", error_out, success_out);

    // Update file tests
    CHECK_FILE_TO_REF(rfDir+"/ref_a_updateConflict_client1_win.txt",
                      c1Dir+"/a_updateConflict_client1.txt",
                      c2Dir+"/a_updateConflict_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_a_updateConflict_client1_win.txt",
                      c1Dir+"/a_updateConflict_client1.txt",
                      c2Dir+"/a_updateConflict_client1.txt", error_out, success_out);

    // UpdateWinDelete
    CHECK_FILE_TO_REF(rfDir+"/ref_a_updateWinDelete_client1_win.txt",
                      c1Dir+"/a_updateWinDeleteConflict_client1.txt",
                      c2Dir+"/a_updateWinDeleteConflict_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_a_updateWinDelete_client2_win.txt",
                      c1Dir+"/a_updateWinDeleteConflict_client2.txt",
                      c2Dir+"/a_updateWinDeleteConflict_client2.txt", error_out, success_out);

    // UpdateDelete -- preserve data, update should win.
    CHECK_FILE_TO_REF(rfDir+"/ref_a_updateDeleteWin_client1_win.txt",
                      c1Dir+"/a_updateDeleteWinConflict_client1.txt",
                      c2Dir+"/a_updateDeleteWinConflict_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_a_updateDeleteWin_client2_win.txt",
                      c1Dir+"/a_updateDeleteWinConflict_client2.txt",
                      c2Dir+"/a_updateDeleteWinConflict_client2.txt", error_out, success_out);

    // DeleteDelete -- should not conflict but just checking for behavior
    CHECK_DENT_NOT_EXIST((c1Dir+"/a_deleteDeleteConflict_client.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/a_deleteDeleteConflict_client.txt").c_str(), error_out, success_out, "c2");
}

// 5 Conflict test cases to try
//
//        | Create | Update | Delete
// =======+========+========+========
// Create |  test  |   NA   |   NA
// Update |   NA   |  test  |  test
// Delete |   NA   |  test  |  test-justInCase

// Additional cases: doubleCreate same file - conflict file should NOT appear.
//                   doubleUpdate to same file - conflict file should NOT appear.
static int doTest12_B_file_conflict_1stToServerWins_loserPropagate(
                                                const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    // Create files on client 1
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;
    client1->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client2->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;

    // Ensure default conflict policies are what we want to test.
    if(client1->syncPolicy.how_to_determine_winner !=
            SyncPolicy::SYNC_CONFLICT_POLICY_FIRST_TO_SERVER_WINS)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client1 not set to FIRST_TO_SERVER_WINS");
    }
    if(client2->syncPolicy.how_to_determine_winner !=
                SyncPolicy::SYNC_CONFLICT_POLICY_FIRST_TO_SERVER_WINS)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client2 not set to FIRST_TO_SERVER_WINS");
    }
    if(client1->syncPolicy.what_to_do_with_loser !=
          SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client1 not set to POLICY_PROPAGATE_LOSER");
    }
    if(client2->syncPolicy.what_to_do_with_loser !=
            SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER)
    {
        CHECK_DO_TEST_ERR(-1, rv, "client2 not set to POLICY_PROPAGATE_LOSER");
    }

    // Setup Test Files - Expects (but verifies) that sync works.
    {
        // Create client paths, paths may not exist before these test files can be placed.
        err = Util_CreatePath(client1->localPath.c_str(), true);
        CHECK_DO_TEST_ERR(err, rv, "mkdir");
        err = Util_CreatePath(client2->localPath.c_str(), true);
        CHECK_DO_TEST_ERR(err, rv, "mkdir");

        err = createCopyMoveFile(rfDir+"/ref_b_updateConflict_client1.txt",
                                 c1Dir+"/b_updateConflict_client1.txt",
                                 300, 1900);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
        err = createCopyMoveFile(rfDir+"/ref_b_updateConflict_client2.txt",
                                 c1Dir+"/b_updateConflict_client2.txt",
                                 300, 1905);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile2");
        err = createCopyMoveFile(rfDir+"/ref_b_updateWinDelete_client1.txt",
                                 c1Dir+"/b_updateWinDeleteConflict_client1.txt",
                                 300, 1910);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
        err = createCopyMoveFile(rfDir+"/ref_b_updateWinDelete_client2.txt",
                                 c1Dir+"/b_updateWinDeleteConflict_client2.txt",
                                 300, 1915);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile2");
        err = createCopyMoveFile(rfDir+"/ref_b_updateDeleteWin_client1.txt",
                                 c1Dir+"/b_updateDeleteWinConflict_client1.txt",
                                 300, 1920);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
        err = createCopyMoveFile(rfDir+"/ref_b_updateDeleteWin_client2.txt",
                                 c1Dir+"/b_updateDeleteWinConflict_client2.txt",
                                 300, 1925);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile2");
        err = createCopyMoveFile(rfDir+"/ref_b_deleteDelete_client.txt",
                                 c1Dir+"/b_deleteDeleteConflict_client.txt",
                                 300, 1930);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile2");
        err = createCopyMoveFile(rfDir+"/ref_b_updateNoConflictFile.txt",
                                 c1Dir+"/b_updateNoConflictFile.txt",
                                 300, 1950);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile1");

        // Setup for test dir->file without conflict
        err = Util_CreateDir((c1Dir+"/b_noConflictDirUpdateToFile/a").c_str());
        CHECK_DO_TEST_ERR(err, rv, "Dir");
        err = createCopyMoveFile(rfDir+"/ref_b_noConflictDirUpdateToFile_a.txt",
                                 c1Dir+"/b_noConflictDirUpdateToFile/a/b_noConflictDirUpdateToFile_a.txt",
                                 300, 1981);

        // Setup for test file->dir without conflict
        err = createCopyMoveFile(rfDir+"/ref_b_noConflictFileUpdateToDir",
                                 c1Dir+"/b_noConflictFileUpdateToDir",
                                 300, 1985);
        CHECK_DO_TEST_ERR(err, rv, "ccmFile1");

        // Setup for test dir->file with conflict
        err = Util_CreateDir((c1Dir+"/b_dirToFileConflict_dirWin").c_str());
        CHECK_DO_TEST_ERR(err, rv, "Dir");
        err = Util_CreateDir((c1Dir+"/b_dirToFileConflict_fileWin").c_str());
        CHECK_DO_TEST_ERR(err, rv, "Dir");

        // Setup for test file->dir with conflict
        err = createCopyMoveFile(rfDir+"/ref_b_fileToDirConflict_dirWin",
                                 c1Dir+"/b_fileToDirConflict_dirWin",
                                 300, 1986);
        CHECK_DO_TEST_ERR(err, rv, "file");
        err = createCopyMoveFile(rfDir+"/ref_b_fileToDirConflict_fileWin",
                                 c1Dir+"/b_fileToDirConflict_fileWin",
                                 300, 1987);
        CHECK_DO_TEST_ERR(err, rv, "file");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "syncC1");
        // Sync client2's data to server
        err = SyncConfigWorkerSync(tua, *client2);
        CHECK_DO_TEST_ERR(err, rv, "syncC2");

        // Note ENSURE_DIFFERENT_MTIME: Need to ensure that subsequent updates
        // to files do not have the same modified time, otherwise, the files
        // will be treated as identical.
        VPLThread_Sleep(VPLTime_FromSec(MAX_FS_GRANULARITY_SEC));

        // Verify sync happened:
        int error_out = 0;
        int unused_out = 0;

        CHECK_FILE_TO_REF(rfDir+"/ref_b_updateConflict_client1.txt",
                          c1Dir+"/b_updateConflict_client1.txt",
                          c2Dir+"/b_updateConflict_client1.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_updateConflict_client2.txt",
                          c1Dir+"/b_updateConflict_client2.txt",
                          c2Dir+"/b_updateConflict_client2.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_updateWinDelete_client1.txt",
                          c1Dir+"/b_updateWinDeleteConflict_client1.txt",
                          c2Dir+"/b_updateWinDeleteConflict_client1.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_updateWinDelete_client2.txt",
                          c1Dir+"/b_updateWinDeleteConflict_client2.txt",
                          c2Dir+"/b_updateWinDeleteConflict_client2.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_updateDeleteWin_client1.txt",
                          c1Dir+"/b_updateDeleteWinConflict_client1.txt",
                          c2Dir+"/b_updateDeleteWinConflict_client1.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_updateDeleteWin_client2.txt",
                          c1Dir+"/b_updateDeleteWinConflict_client2.txt",
                          c2Dir+"/b_updateDeleteWinConflict_client2.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_deleteDelete_client.txt",
                          c1Dir+"/b_deleteDeleteConflict_client.txt",
                          c2Dir+"/b_deleteDeleteConflict_client.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_noConflictDirUpdateToFile_a.txt",
                          c1Dir+"/b_noConflictDirUpdateToFile/a/b_noConflictDirUpdateToFile_a.txt",
                          c2Dir+"/b_noConflictDirUpdateToFile/a/b_noConflictDirUpdateToFile_a.txt", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
        CHECK_FILE_TO_REF(rfDir+"/ref_b_noConflictFileUpdateToDir",
                          c1Dir+"/b_noConflictFileUpdateToDir",
                          c2Dir+"/b_noConflictFileUpdateToDir", error_out, unused_out);
        CHECK_DO_TEST_ERR(error_out, rv, "sync");
        error_out = 0;
    }
    /////////////////////// CLIENT 1 WINS ////////////////////////////////////

    // Create conflict winner client1
    err = createCopyMoveFile(rfDir+"/ref_b_conflict_createfile_client1.txt",
                             c1Dir+"/b_conflict_createfile_client1.txt",
                             300, 1935);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_b_conflict_createfile_client1_lose.txt",
                             c2Dir+"/b_conflict_createfile_client1.txt",
                             300, 1936);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");

    // Update conflict winner client1
    err = createCopyMoveFile(rfDir+"/ref_b_updateConflict_client1_win.txt",
                             c1Dir+"/b_updateConflict_client1.txt",
                             300, 1901);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_b_updateConflict_client1_lose.txt",
                             c2Dir+"/b_updateConflict_client1.txt",
                             300, 1902);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");

    // UpdateWinDelete winner client1
    err = createCopyMoveFile(rfDir+"/ref_b_updateWinDelete_client1_win.txt",
                             c1Dir+"/b_updateWinDeleteConflict_client1.txt",
                             300, 1911);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = Util_rmRecursive(c2Dir+"/b_updateWinDeleteConflict_client1.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");

    // UpdateDeleteWin winner client1
    err = Util_rmRecursive(c1Dir+"/b_updateDeleteWinConflict_client1.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");
    err = createCopyMoveFile(rfDir+"/ref_b_updateDeleteWin_client1_win.txt",
                             c2Dir+"/b_updateDeleteWinConflict_client1.txt",
                             300, 1921);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");

    err = Util_rmRecursive(c1Dir+"/b_deleteDeleteConflict_client.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");
    err = Util_rmRecursive(c2Dir+"/b_deleteDeleteConflict_client.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");

    err = Util_CreateDir((c1Dir+"/b_dir_first_conflict").c_str());
    CHECK_DO_TEST_ERR(err, rv, "dotest");
    err = createCopyMoveFile(rfDir+"/ref_b_dir_first_conflict",
                             c2Dir+"/b_dir_first_conflict",
                             300, 1950);
    CHECK_DO_TEST_ERR(err, rv, "dotest");

    err = createCopyMoveFile(rfDir+"/ref_b_file_first_conflict",
                             c1Dir+"/b_file_first_conflict",
                             300, 1951);
    CHECK_DO_TEST_ERR(err, rv, "dotest");
    err = Util_CreateDir((c2Dir+"/b_file_first_conflict").c_str());
    CHECK_DO_TEST_ERR(err, rv, "dotest");

    // dir->file no conflict
    err = Util_rmRecursive(c1Dir+"/b_noConflictDirUpdateToFile", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "dotest");
    err = createCopyMoveFile(rfDir+"/ref_b_noConflictDirUpdateToFile",
                             c1Dir+"/b_noConflictDirUpdateToFile",
                             300, 1960);
    CHECK_DO_TEST_ERR(err, rv, "dotest");

    // file->dir no conflict
    err = Util_rmRecursive(c1Dir+"/b_noConflictFileUpdateToDir", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "dotest");
    err = Util_CreateDir((c1Dir+"/b_noConflictFileUpdateToDir").c_str());
    CHECK_DO_TEST_ERR(err, rv, "dotest");

    {   // Action for test dir->file with conflict
        err = createCopyMoveFile(rfDir+"/ref_b_subDirFile_client1.txt",
                                 c1Dir+"/b_dirToFileConflict_dirWin/b_subDirFile_client1.txt",
                                 300, 2100);
        CHECK_DO_TEST_ERR(err, rv, "Dir");
        err = Util_rmRecursive(c2Dir+"/b_dirToFileConflict_dirWin", TEST_DELETE_DIR);
        CHECK_DO_TEST_ERR(err, rv, "dotest");
        err = createCopyMoveFile(rfDir+"/ref_b_dirToFileConflict_dirWin",
                                 c2Dir+"/b_dirToFileConflict_dirWin",
                                 300, 2101);
        CHECK_DO_TEST_ERR(err, rv, "dotest");

        err = Util_rmRecursive(c1Dir+"/b_dirToFileConflict_fileWin", TEST_DELETE_DIR);
        CHECK_DO_TEST_ERR(err, rv, "dotest");
        err = createCopyMoveFile(rfDir+"/ref_b_dirToFileConflict_fileWin",
                                 c1Dir+"/b_dirToFileConflict_fileWin",
                                 300, 2101);
        CHECK_DO_TEST_ERR(err, rv, "dotest");
        err = createCopyMoveFile(rfDir+"/ref_b_subDirFile_client2.txt",
                                 c2Dir+"/b_dirToFileConflict_fileWin/b_subDirFile_client2.txt",
                                 300, 2102);
        CHECK_DO_TEST_ERR(err, rv, "dotest");
    }

    {   // Action for test file->dir with conflict
        err = Util_rmRecursive(c1Dir+"/b_fileToDirConflict_dirWin", TEST_DELETE_DIR);
        err = Util_CreateDir((c1Dir+"/b_fileToDirConflict_dirWin").c_str());
        CHECK_DO_TEST_ERR(err, rv, "dotest");
        err = createCopyMoveFile(rfDir+"/ref_b_fileToDirConflict_dirWin_client1.txt",
                                 c1Dir+"/b_fileToDirConflict_dirWin/b_fileToDirConflict_dirWin_client1.txt",
                                 300, 2105);
        CHECK_DO_TEST_ERR(err, rv, "file");
        err = Util_rmRecursive(c2Dir+"/b_fileToDirConflict_dirWin", TEST_DELETE_DIR);
        err = createCopyMoveFile(rfDir+"/ref_b_fileToDirConflict_dirWin_client2.txt",
                                 c2Dir+"/b_fileToDirConflict_dirWin",
                                 300, 2106);
        CHECK_DO_TEST_ERR(err, rv, "file");

        err = Util_rmRecursive(c1Dir+"/b_fileToDirConflict_fileWin", TEST_DELETE_DIR);
        CHECK_DO_TEST_ERR(err, rv, "file");
        err = createCopyMoveFile(rfDir+"/ref_b_fileToDirConflict_fileWin_client1.txt",
                                 c1Dir+"/b_fileToDirConflict_fileWin",
                                 300, 2108);
        CHECK_DO_TEST_ERR(err, rv, "file");
        err = Util_rmRecursive(c2Dir+"/b_fileToDirConflict_fileWin", TEST_DELETE_DIR);
        err = Util_CreateDir((c2Dir+"/b_fileToDirConflict_fileWin").c_str());
        CHECK_DO_TEST_ERR(err, rv, "dotest");
        err = createCopyMoveFile(rfDir+"/ref_b_fileToDirConflict_fileWin_client2.txt",
                                 c2Dir+"/b_fileToDirConflict_fileWin/b_dirToFileConflict_fileWin_client2.txt",
                                 300, 2109);
        CHECK_DO_TEST_ERR(err, rv, "file");
    }

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server -- CONFLICT
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client1's data to server -- Propagate override of deleteWin in updateDelete (to preserve data).
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // See Note: ENSURE_DIFFERENT_MTIME
    VPLThread_Sleep(VPLTime_FromSec(MAX_FS_GRANULARITY_SEC));

    /////////////////////// CLIENT 2 WINS ////////////////////////////////////
    // Create conflict winner client2
    err = createCopyMoveFile(rfDir+"/ref_b_conflict_createfile_client2_lose.txt",
                             c1Dir+"/b_conflict_createfile_client2.txt",
                             300, 1937);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_b_conflict_createfile_client2.txt",
                             c2Dir+"/b_conflict_createfile_client2.txt",
                             300, 1938);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");

    // Update conflict winner client2
    err = createCopyMoveFile(rfDir+"/ref_b_updateConflict_client2_lose.txt",
                             c1Dir+"/b_updateConflict_client2.txt",
                             300, 1906);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_b_updateConflict_client2_wing.txt",
                             c2Dir+"/b_updateConflict_client2.txt",
                             300, 1907);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");

    // UpdateWinDelete winner client2
    err = createCopyMoveFile(rfDir+"/ref_b_updateWinDelete_client2_win.txt",
                             c2Dir+"/b_updateWinDeleteConflict_client2.txt",
                             300, 1916);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = Util_rmRecursive(c1Dir+"/b_updateWinDeleteConflict_client2.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");

    // UpdateDeleteWin winner client2
    err = Util_rmRecursive(c2Dir+"/b_updateDeleteWinConflict_client2.txt", TEST_DELETE_DIR);
    CHECK_DO_TEST_ERR(err, rv, "deleteFile2");
    err = createCopyMoveFile(rfDir+"/ref_b_updateDeleteWin_client2_win.txt",
                             c1Dir+"/b_updateDeleteWinConflict_client2.txt",
                             300, 1926);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");

    ////////////////// TIE //////////////////
    //  no conflict file should be created
    // Create file
    err = createCopyMoveFile(rfDir+"/ref_b_createNoConflictFile1.txt",
                             c1Dir+"/b_createNoConflictFile.txt",
                             300, 1940);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_b_createNoConflictFile2.txt",
                             c2Dir+"/b_createNoConflictFile.txt",
                             300, 1940);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");
    // UpdateFile
    err = createCopyMoveFile(rfDir+"/ref_b_updateNoConflictFile1.txt",
                             c1Dir+"/b_updateNoConflictFile.txt",
                             300, 1990);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile1");
    err = createCopyMoveFile(rfDir+"/ref_b_updateNoConflictFile2.txt",
                             c2Dir+"/b_updateNoConflictFile.txt",
                             300, 1990);
    CHECK_DO_TEST_ERR(err, rv, "ccmFile2");

    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client1's data to server -- CONFLICT
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server -- Propagate override of deleteWin in updateDelete (to preserve data).
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    // Sync client1's data to server -- Propagate conflict files
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // Sync once more to make sure there's no feedback loop bug:
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    return rv;
}

static void chkTest12_B_file_conflict_first2ServerWins_loserPropagate(
                                                int& error_out,
                                                int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Create file tests
    CHECK_FILE_TO_REF(rfDir+"/ref_b_conflict_createfile_client1.txt",
                      c1Dir+"/b_conflict_createfile_client1.txt",
                      c2Dir+"/b_conflict_createfile_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_b_conflict_createfile_client2.txt",
                      c1Dir+"/b_conflict_createfile_client2.txt",
                      c2Dir+"/b_conflict_createfile_client2.txt", error_out, success_out);

    // Update file tests
    CHECK_FILE_TO_REF(rfDir+"/ref_b_updateConflict_client1_win.txt",
                      c1Dir+"/b_updateConflict_client1.txt",
                      c2Dir+"/b_updateConflict_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_b_updateConflict_client1_win.txt",
                      c1Dir+"/b_updateConflict_client1.txt",
                      c2Dir+"/b_updateConflict_client1.txt", error_out, success_out);

    // UpdateWinDelete
    CHECK_FILE_TO_REF(rfDir+"/ref_b_updateWinDelete_client1_win.txt",
                      c1Dir+"/b_updateWinDeleteConflict_client1.txt",
                      c2Dir+"/b_updateWinDeleteConflict_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_b_updateWinDelete_client2_win.txt",
                      c1Dir+"/b_updateWinDeleteConflict_client2.txt",
                      c2Dir+"/b_updateWinDeleteConflict_client2.txt", error_out, success_out);

    // UpdateDelete -- preserve data, update should win.
    CHECK_FILE_TO_REF(rfDir+"/ref_b_updateDeleteWin_client1_win.txt",
                      c1Dir+"/b_updateDeleteWinConflict_client1.txt",
                      c2Dir+"/b_updateDeleteWinConflict_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_b_updateDeleteWin_client2_win.txt",
                      c1Dir+"/b_updateDeleteWinConflict_client2.txt",
                      c2Dir+"/b_updateDeleteWinConflict_client2.txt", error_out, success_out);

    // DeleteDelete -- should not conflict but just checking for behavior
    CHECK_DENT_NOT_EXIST((c1Dir+"/b_deleteDeleteConflict_client.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/b_deleteDeleteConflict_client.txt").c_str(), error_out, success_out, "c2");

    // Verify Dir to File
    CHECK_FILE_TO_REF(rfDir+"/ref_b_noConflictDirUpdateToFile",
                      c1Dir+"/b_noConflictDirUpdateToFile",
                      c2Dir+"/b_noConflictDirUpdateToFile", error_out, success_out);

    // Verify Dir to File Conflict
    CHECK_DIR_EXIST((c1Dir+"/b_noConflictFileUpdateToDir").c_str(), error_out, success_out, "c1");
    CHECK_DIR_EXIST((c2Dir+"/b_noConflictFileUpdateToDir").c_str(), error_out, success_out, "c2");

    CHECK_FILE_TO_REF(rfDir+"/ref_b_subDirFile_client1.txt",
                      c1Dir+"/b_dirToFileConflict_dirWin/b_subDirFile_client1.txt",
                      c2Dir+"/b_dirToFileConflict_dirWin/b_subDirFile_client1.txt", error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_b_dirToFileConflict_fileWin",
                      c1Dir+"/b_dirToFileConflict_fileWin",
                      c2Dir+"/b_dirToFileConflict_fileWin", error_out, success_out);

    // Verify File to Dir  conflict
    CHECK_FILE_TO_REF(rfDir+"/ref_b_fileToDirConflict_dirWin_client1.txt",
                      c1Dir+"/b_fileToDirConflict_dirWin/b_fileToDirConflict_dirWin_client1.txt",
                      c2Dir+"/b_fileToDirConflict_dirWin/b_fileToDirConflict_dirWin_client1.txt",
                      error_out, success_out);
    CHECK_FILE_TO_REF(rfDir+"/ref_b_fileToDirConflict_fileWin_client1.txt",
                      c1Dir+"/b_fileToDirConflict_fileWin",
                      c2Dir+"/b_fileToDirConflict_fileWin", error_out, success_out);

    // Check for existence and non-existence of conflict files, which don't have a set name.
    do {  // Client 1
        VPLFS_dir_t dirStream;
        int rc = VPLFS_Opendir(c1Dir.c_str(), &dirStream);
        if (rc != 0) {
            LOG_ERROR("VPLFS_Opendir(%s): %d", c1Dir.c_str(), rc);
            error_out++;
            break;  // goto COMMENT_LABEL_ClientCheck_1_exit;
        }
        ON_BLOCK_EXIT(VPLFS_Closedir, &dirStream);

        int b_shouldExist1 = 0;
        int b_shouldExist2 = 0;
        int b_shouldExist3 = 0;
        int b_shouldExist4 = 0;

        // For each dirEntry in currDir:
        VPLFS_dirent_t dirEntry;
        while ((rc = VPLFS_Readdir(&dirStream, &dirEntry)) == VPL_OK) {
            std::string strDirEntry(dirEntry.filename);
            if (strDirEntry==".." || strDirEntry==".") {
                continue;
            }
            std::string checkExist = "b_conflict_createfile_client1_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                CHECK_FILE_TO_REF(rfDir+"/ref_b_conflict_createfile_client1_lose.txt",
                                  c1Dir+"/"+strDirEntry,
                                  c2Dir+"/"+strDirEntry, error_out, success_out);
                b_shouldExist1++;
                continue;
            }
            checkExist = "b_updateConflict_client1_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                CHECK_FILE_TO_REF(rfDir+"/ref_b_updateConflict_client1_lose.txt",
                                  c1Dir+"/"+strDirEntry,
                                  c2Dir+"/"+strDirEntry, error_out, success_out);
                b_shouldExist2++;
                continue;
            }
            checkExist = "b_conflict_createfile_client2_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                CHECK_FILE_TO_REF(rfDir+"/ref_b_conflict_createfile_client2_lose.txt",
                                  c1Dir+"/"+strDirEntry,
                                  c2Dir+"/"+strDirEntry, error_out, success_out);
                b_shouldExist3++;
                continue;
            }
            checkExist = "b_updateConflict_client2_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                CHECK_FILE_TO_REF(rfDir+"/ref_b_updateConflict_client2_lose.txt",
                                  c1Dir+"/"+strDirEntry,
                                  c2Dir+"/"+strDirEntry, error_out, success_out);
                b_shouldExist4++;
                continue;
            }

            checkExist = "b_createNoConflictFile_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
            checkExist = "b_updateNoConflictFile_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
            checkExist = "b_noConflictDirUpdateToFile_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
            checkExist = "b_noConflictFileUpdateToDir_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
        }

        if(b_shouldExist1 != 1) {
            LOG_ERROR("%s conflict file not 1, but %d", "b_conflict_createfile_client1_CONFLICT_*", b_shouldExist1);
            error_out++;
        } else {
            success_out++;
        }
        if(b_shouldExist2 != 1) {
            LOG_ERROR("%s conflict file not 1, but %d", "b_updateConflict_client1_CONFLICT_*", b_shouldExist2);
            error_out++;
        } else {
            success_out++;
        }
        if(b_shouldExist3 != 1) {
            LOG_ERROR("%s conflict file not 1, but %d", "b_conflict_createfile_client2_CONFLICT_*", b_shouldExist3);
            error_out++;
        } else {
            success_out++;
        }
        if(b_shouldExist4 != 1) {
            LOG_ERROR("%s conflict file not 1, but %d", "b_updateConflict_client2_CONFLICT_*", b_shouldExist4);
            error_out++;
        } else {
            success_out++;
        }
    } while (false);
    // COMMENT_LABEL_ClientCheck_1_exit

    do {  // Client 2
        VPLFS_dir_t dirStream;
        int rc = VPLFS_Opendir(c1Dir.c_str(), &dirStream);
        if (rc != 0) {
            LOG_ERROR("VPLFS_Opendir(%s): %d", c1Dir.c_str(), rc);
            error_out++;
            break;  // goto COMMENT_LABEL_ClientCheck_2_exit;
        }
        ON_BLOCK_EXIT(VPLFS_Closedir, &dirStream);

        // For each dirEntry in currDir:
        VPLFS_dirent_t dirEntry;
        while ((rc = VPLFS_Readdir(&dirStream, &dirEntry)) == VPL_OK) {
            std::string strDirEntry(dirEntry.filename);
            if (strDirEntry==".." || strDirEntry==".") {
                continue;
            }
            std::string checkExist = "b_createNoConflictFile_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
            checkExist = "b_updateNoConflictFile_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
            checkExist = "b_noConflictDirUpdateToFile_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
            checkExist = "b_noConflictFileUpdateToDir_CONFLICT_";
            if (strDirEntry.substr(0, checkExist.size()) == checkExist) {
                LOG_ERROR("(%s,%s) should not exist", c1Dir.c_str(), strDirEntry.c_str());
                error_out++;
                continue;
            }
        }
    } while (false);
    // COMMENT_LABEL_ClientCheck_2_exit
}

// Test that a file is only uploaded once and download once.
//   This test depends on the implementation of OneWayDown, where once a file
//   is downloaded, changes to the downloaded file will NOT cause a further download.
//   The test will upload a 3 files synced separately.  The test is successful if the
//   revision of each of these files uploaded files is 1
static int doTest13_uploadOnce_downloadOnce_OW(const SCWTestUserAcct& tua)
{
    int rv = 0;
    int err;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    { // File 1
        err = createCopyMoveFile(rfDir+"/ow_ref_doOnce_file1.txt",
                                 c1Dir+"/ow_doOnce_file1.txt",
                                 500, 901);
        CHECK_DO_TEST_ERR(err, rv, "uploadOnce_1");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "syncC1_1");
        // Sync client2's data to server
        err = SyncConfigWorkerSync(tua, *client2);
        CHECK_DO_TEST_ERR(err, rv, "syncC2_1");

        {   // Check to make sure the download occurred.
            int errorCount = 0;
            int successCount = 0;
            CHECK_FILE_TO_REF(rfDir+"/ow_ref_doOnce_file1.txt",
                              c1Dir+"/ow_doOnce_file1.txt",
                              c2Dir+"/ow_doOnce_file1.txt",
                              /*OUT*/errorCount, /*OUT*/successCount);
            if (errorCount != 0) {
                LOG_ERROR("Error during basic sync.""file1");
                rv = -5;
            }
        }

        // Change the downloaded file.  If download only happens once, the change
        // should remain because SyncOneWayDown does not actively replace locally
        // changed files.
        err = createCopyMoveFile(rfDir+"/ow_ref_doOnce_file1_changed.txt",
                                 c2Dir+"/ow_doOnce_file1.txt",
                                 600, 911);
    }

    { // File 2
        err = createCopyMoveFile(rfDir+"/ow_ref_doOnce_file2.txt",
                                 c1Dir+"/ow_doOnce_file2.txt",
                                 500, 902);
        CHECK_DO_TEST_ERR(err, rv, "uploadOnce_2");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "syncC1_2");
        // Sync client2's data to server
        err = SyncConfigWorkerSync(tua, *client2);
        CHECK_DO_TEST_ERR(err, rv, "syncC2_2");

        {   // Check to make sure the download occurred.
            int errorCount = 0;
            int successCount = 0;
            CHECK_FILE_TO_REF(rfDir+"/ow_ref_doOnce_file2.txt",
                              c1Dir+"/ow_doOnce_file2.txt",
                              c2Dir+"/ow_doOnce_file2.txt",
                              /*OUT*/errorCount, /*OUT*/successCount);
            if (errorCount != 0) {
                LOG_ERROR("Error during basic sync.""file2");
                rv = -5;
            }
        }

        // Change the downloaded file.  If download only happens once, the change
        // should remain because SyncOneWayDown does not actively replace locally
        // changed files.
        err = createCopyMoveFile(rfDir+"/ow_ref_doOnce_file2_changed.txt",
                                 c2Dir+"/ow_doOnce_file2.txt",
                                 600, 912);
    }

    { // File 3
        err = createCopyMoveFile(rfDir+"/ow_ref_doOnce_file3.txt",
                                 c1Dir+"/ow_doOnce_file3.txt",
                                 500, 903);
        CHECK_DO_TEST_ERR(err, rv, "uploadOnce_3");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "syncC1_3");
        // Sync client2's data to server
        err = SyncConfigWorkerSync(tua, *client2);
        CHECK_DO_TEST_ERR(err, rv, "syncC2_3");

        {   // Check to make sure the download occurred.
            int errorCount = 0;
            int successCount = 0;
            CHECK_FILE_TO_REF(rfDir+"/ow_ref_doOnce_file3.txt",
                              c1Dir+"/ow_doOnce_file3.txt",
                              c2Dir+"/ow_doOnce_file3.txt",
                              /*OUT*/errorCount, /*OUT*/successCount);
            if (errorCount != 0) {
                LOG_ERROR("Error during basic sync.""file3");
                rv = -5;
            }
        }

        // Change the downloaded file.  If download only happens once, the change
        // should remain because SyncOneWayDown does not actively replace locally
        // changed files.
        err = createCopyMoveFile(rfDir+"/ow_ref_doOnce_file3_changed.txt",
                                 c2Dir+"/ow_doOnce_file3.txt",
                                 600, 913);
    }

    return rv;
}

static int checkRevisionIsOneOnInfra(const SCWTestUserAcct& tua,
                                     const std::string& serverPath)
{
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(tua.vcsTestSession, vcsSession);
    int err;
    u64 fileCompId = 0;
    {
        VPLHttp2 httpHandle;
        err = vcs_get_comp_id(vcsSession,
                              httpHandle,
                              tua.getDataset(),
                              serverPath,
                              true,
                              /*OUT*/ fileCompId);
        if (err != 0) {
            LOG_ERROR("vcs_get_comp_id(dset:"FMTu64",%s):%d",
                      tua.getDataset().id, serverPath.c_str(), err);
            return err;
        }
    }
    {
        VPLHttp2 httpHandle;
        VcsFileMetadataResponse response;
        err = vcs_get_file_metadata(vcsSession,
                                    httpHandle,
                                    tua.getDataset(),
                                    serverPath,
                                    fileCompId,
                                    true,
                                    /*OUT*/ response);
        if (err != 0) {
            LOG_ERROR("vcs_get_file_metadata(dset:"FMTu64", compId:"FMTu64",%s):%d",
                      tua.getDataset().id, fileCompId, serverPath.c_str(), err);
            return err;
        }

        if(response.revisionList[0].revision == 1) {
            return 0;
        } else {
            LOG_ERROR("Revision for file1 != 1.  Multiple Uploads happened");
            return -1;
        }
    }
}

static void chkTest13_uploadOnce_downloadOnce_OW(const SCWTestUserAcct& tua,
                                                 int& error_out,
                                                 int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Look directly into infra and check revision numbers are 1.
    {   // File 1
        int err;
        std::string serverPath = client1->serverPath+"/ow_doOnce_file1.txt";
        err = checkRevisionIsOneOnInfra(tua, serverPath);
        if (err != 0) {
            LOG_ERROR("checkRevisionIsOne(%s):%d", serverPath.c_str(), err);
            error_out++;
        } else {
            success_out++;
        }
    }
    {   // File 2
        int err;
        std::string serverPath = client1->serverPath+"/ow_doOnce_file2.txt";
        err = checkRevisionIsOneOnInfra(tua, serverPath);
        if (err != 0) {
            LOG_ERROR("checkRevisionIsOne(%s):%d", serverPath.c_str(), err);
            error_out++;
        } else {
            success_out++;
        }
    }
    {   // File 3
        int err;
        std::string serverPath = client1->serverPath+"/ow_doOnce_file3.txt";
        err = checkRevisionIsOneOnInfra(tua, serverPath);
        if (err != 0) {
            LOG_ERROR("checkRevisionIsOne(%s):%d", serverPath.c_str(), err);
            error_out++;
        } else {
            success_out++;
        }
    }

    // Check download once tests
    CHECK_SINGLE_FILE_TO_REF(rfDir+"/ow_ref_doOnce_file1_changed.txt",
                             c2Dir+"/ow_doOnce_file1.txt",
                             error_out, success_out);
    CHECK_SINGLE_FILE_TO_REF(rfDir+"/ow_ref_doOnce_file2_changed.txt",
                             c2Dir+"/ow_doOnce_file2.txt",
                             error_out, success_out);
    CHECK_SINGLE_FILE_TO_REF(rfDir+"/ow_ref_doOnce_file3_changed.txt",
                             c2Dir+"/ow_doOnce_file3.txt",
                             error_out, success_out);
}

// Test that a file is only uploaded once.  The two way download once portion is not possible
//   to test without keeping track the number of times the file is downloaded.
//   The test will upload a 3 files synced separately.  The test is successful if the
//   revision of each of these files uploaded files is 1
static int doTest14_uploadOnce_TwoWayUp(const SCWTestUserAcct& tua)
{
    int rv = 0;
    int err;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;

    { // File 1
        err = createCopyMoveFile(rfDir+"/tw_ref_doOnce_file1.txt",
                                 c1Dir+"/tw_doOnce_file1.txt",
                                 500, 921);
        CHECK_DO_TEST_ERR(err, rv, "uploadOnce_tw_1");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "syncC1_1");
    }

    { // File 2
        err = createCopyMoveFile(rfDir+"/tw_ref_doOnce_file2.txt",
                                 c1Dir+"/tw_doOnce_file2.txt",
                                 500, 922);
        CHECK_DO_TEST_ERR(err, rv, "uploadOnce_tw_2");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "syncC1_2");
    }

    { // File 3
        err = createCopyMoveFile(rfDir+"/tw_ref_doOnce_file3.txt",
                                 c1Dir+"/tw_doOnce_file3.txt",
                                 500, 923);
        CHECK_DO_TEST_ERR(err, rv, "uploadOnce_3");

        // Sync client1's data to server
        err = SyncConfigWorkerSync(tua, *client1);
        CHECK_DO_TEST_ERR(err, rv, "syncC1_3");
    }

    return rv;
}

static void chkTest14_uploadOnce_TwoWayUp(const SCWTestUserAcct& tua,
                                          int& error_out,
                                          int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;

    // Look directly into infra and check revision numbers are 1.
    {   // File 1
        int err;
        std::string serverPath = client1->serverPath+"/tw_doOnce_file1.txt";
        err = checkRevisionIsOneOnInfra(tua, serverPath);
        if (err != 0) {
            LOG_ERROR("checkRevisionIsOne(%s):%d", serverPath.c_str(), err);
            error_out++;
        } else {
            success_out++;
        }
    }
    {   // File 2
        int err;
        std::string serverPath = client1->serverPath+"/tw_doOnce_file2.txt";
        err = checkRevisionIsOneOnInfra(tua, serverPath);
        if (err != 0) {
            LOG_ERROR("checkRevisionIsOne(%s):%d", serverPath.c_str(), err);
            error_out++;
        } else {
            success_out++;
        }
    }
    {   // File 3
        int err;
        std::string serverPath = client1->serverPath+"/tw_doOnce_file3.txt";
        err = checkRevisionIsOneOnInfra(tua, serverPath);
        if (err != 0) {
            LOG_ERROR("checkRevisionIsOne(%s):%d", serverPath.c_str(), err);
            error_out++;
        } else {
            success_out++;
        }
    }
}

#ifdef COMPUTE_HASH_FOR_ACS_SYNC
static void check_file_timestamp_consistent(int& error_out, int& success_out,
        VPLTime_t& local_time, const std::string file_path)
{
    VPLFS_stat_t statBuf;
    int statrv =  VPLFS_Stat(file_path.c_str(), &statBuf);
    if (statrv == 0) {
        if (local_time == statBuf.vpl_mtime) {
            LOG_DEBUG("target timestamp:"FMTu64, statBuf.vpl_mtime);
            success_out++;
        } else {
            LOG_ERROR("timestamp of file%s changed to:"FMTu64", supposed to be "FMTu64,
                 file_path.c_str(), statBuf.vpl_mtime, local_time);
            error_out++;
        }
    } else {
        error_out++;
        LOG_ERROR("cannot stat file%s", file_path.c_str());
    }
}

static int doTest15_download_hash_match(const SCWTestUserAcct& tua, VPLTime_t& local_time)
{
    int err;
    int rv = 0;
    int fileSize1 = 200;
    int fileSize2 = 201;
    int random_seed1 = 30;
    int random_seed2 = 31;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    client1->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client2->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client1->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    client2->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // -----------------
    // Test: 
    //   1. Client1: Create a file, sync to server
    //   2. Client2: sync from server
    //   3. Client1: update the file -> sync -> rollback -> sync
    //   4. Client2: sync from server
    // client2's file should not change timestamp because no file downloaded

    // Create files on client 1
    // hash value is "a6333a914fffc73f71cb59b802d45bba421ffb1a"
    err = createCopyMoveFile(rfDir+"/ref_hash_1.txt",
                             c1Dir+"/hash_1.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // See Note: ENSURE_DIFFERENT_MTIME
    VPLThread_Sleep(VPLTime_FromSec(MAX_FS_GRANULARITY_SEC));

    local_time = 0;
    // Assume we client2 get synced, set its timestamp as reference
    VPLFS_stat_t statBuf;
    err = VPLFS_Stat((c2Dir+"/hash_1.txt").c_str(), &statBuf);
    CHECK_DO_TEST_ERR(err, rv, "ccmFsStat");
    if (err == 0) {
        LOG_INFO("reference timestamp:"FMTu64, statBuf.vpl_mtime);
        local_time = statBuf.vpl_mtime;
    }

    // Change client1's data
    err = createCopyMoveFile(rfDir+"/ref_hash_1.txt",
                             c1Dir+"/hash_1.txt",
                             fileSize1, random_seed2);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // See Note: ENSURE_DIFFERENT_MTIME
    VPLThread_Sleep(VPLTime_FromSec(MAX_FS_GRANULARITY_SEC));

    // Rollback client1's file
    err = createCopyMoveFile(rfDir+"/ref_hash_1.txt",
                             c1Dir+"/hash_1.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // -----------------
    // Test: both clients create the same file simultaneously, sync client1 first
    // client2's file should not change timestamp because no file downloaded
    err = createCopyMoveFile(rfDir+"/ref_hash_2.txt",
                             c1Dir+"/hash_2.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash2");
    err = createCopyMoveFile(rfDir+"/ref_hash_2.txt",
                             c2Dir+"/hash_2.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash2");

    err = VPLFile_SetTime((c2Dir+"/hash_2.txt").c_str(), local_time);
    CHECK_DO_TEST_ERR(err, rv, "ccmSetTime");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // -----------------
    // Test: 
    //   1. Client1: Create a file, sync to server
    //   2. Client2: Sync from server
    //   3. Client1: Update to content X -> sync to server
    //   4. Client2: local update to content X -> sync to server
    // Client2's file should not change timestamp because no file downloaded
    // Create files on client 1
    err = createCopyMoveFile(rfDir+"/ref_hash_2_1.txt",
                             c1Dir+"/hash_2_1.txt",
                             fileSize2, random_seed2);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash2_1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // See Note: ENSURE_DIFFERENT_MTIME
    VPLThread_Sleep(VPLTime_FromSec(MAX_FS_GRANULARITY_SEC));

    // Update Client1's content to X
    // hash value is "a6333a914fffc73f71cb59b802d45bba421ffb1a"
    err = createCopyMoveFile(rfDir+"/ref_hash_2_1.txt",
                             c1Dir+"/hash_2_1.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash2_1");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // Update Client2's content to X
    err = createCopyMoveFile(rfDir+"/ref_hash_2_1.txt",
                             c2Dir+"/hash_2_1.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash2_1");
    err = VPLFile_SetTime((c2Dir+"/hash_2_1.txt").c_str(), local_time);
    CHECK_DO_TEST_ERR(err, rv, "ccmSetTime");

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    return rv;
}

static void chkTest15_download_hash_match(int& error_out,
                                  int& success_out, VPLTime_t& local_time)

{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_hash_1.txt",
                      c1Dir+"/hash_1.txt",
                      c2Dir+"/hash_1.txt", error_out, success_out);

    check_file_timestamp_consistent(error_out, success_out, local_time, c2Dir+"/hash_1.txt");
    
    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_hash_2.txt",
                      c1Dir+"/hash_2.txt",
                      c2Dir+"/hash_2.txt", error_out, success_out);

    check_file_timestamp_consistent(error_out, success_out, local_time, c2Dir+"/hash_2.txt");

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_hash_2_1.txt",
                      c1Dir+"/hash_2_1.txt",
                      c2Dir+"/hash_2_1.txt", error_out, success_out);

    check_file_timestamp_consistent(error_out, success_out, local_time, c2Dir+"/hash_2_1.txt");
}

static int getLastChangeOnInfra(const SCWTestUserAcct& tua,
                                     const std::string& serverPath, u64& out_time)
{
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(tua.vcsTestSession, vcsSession);
    int err;
    u64 fileCompId = 0;
    {
        VPLHttp2 httpHandle;
        err = vcs_get_comp_id(vcsSession,
                              httpHandle,
                              tua.getDataset(),
                              serverPath,
                              true,
                              /*OUT*/ fileCompId);
        if (err != 0) {
            LOG_ERROR("vcs_get_comp_id(dset:"FMTu64",%s):%d",
                      tua.getDataset().id, serverPath.c_str(), err);
            return err;
        }
    }
    {
        VPLHttp2 httpHandle;
        VcsFileMetadataResponse response;
        err = vcs_get_file_metadata(vcsSession,
                                    httpHandle,
                                    tua.getDataset(),
                                    serverPath,
                                    fileCompId,
                                    true,
                                    /*OUT*/ response);
        if (err != 0) {
            LOG_ERROR("vcs_get_file_metadata(dset:"FMTu64", compId:"FMTu64",%s):%d",
                      tua.getDataset().id, fileCompId, serverPath.c_str(), err);
            return err;
        }

        out_time = response.lastChanged;
    }
    return 0;
}


static int doTest16_upload_hash_match(const SCWTestUserAcct& tua, VPLTime_t& vcs_time)
{
    int err;
    int rv = 0;
    int fileSize1 = 200;
    int random_seed1 = 30;
    int random_seed2 = 31;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    client1->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client2->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client1->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    client2->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // -----------------
    //   1. Client1: Create a file, sync to server
    //   2. Client2: sync from server
    //   3. Client1: update the file -> rollback -> sync
    //   4. Client2: sync from server
    // LastChang Time in CVS should not be changed by client2 or
    // updated by client1 again

    // Create files on client 1
    // hash value is "a6333a914fffc73f71cb59b802d45bba421ffb1a"
    err = createCopyMoveFile(rfDir+"/ref_hash_3.txt",
                             c1Dir+"/hash_3.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHashHash3");

    VPLTime_t ref_mtime = 0;
    // Get mtime from client1 for reference
    VPLFS_stat_t statBuf;
    err = VPLFS_Stat((c1Dir+"/hash_3.txt").c_str(), &statBuf);
    CHECK_DO_TEST_ERR(err, rv, "ccmFsStat");
    if (err == 0) {
        LOG_INFO("reference timestamp:"FMTu64, statBuf.vpl_mtime);
        ref_mtime = statBuf.vpl_mtime;
        // Need to set mtime for this file again because of different granularity
        err = VPLFile_SetTime((c1Dir+"/hash_3.txt").c_str(), ref_mtime);
        CHECK_DO_TEST_ERR(err, rv, "ccmSetTime");
    }
    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // See Note: ENSURE_DIFFERENT_MTIME
    VPLThread_Sleep(VPLTime_FromSec(MAX_FS_GRANULARITY_SEC));

    // Get current lastChanged from VCS
    err = getLastChangeOnInfra(tua, client1->serverPath+"/hash_3.txt", /* OUT */ vcs_time);
    if (err != 0) {
        LOG_ERROR("get LastChange Time from VCS failed");
        rv++;
    }

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // Change & Rollback client1's data
    err = createCopyMoveFile(rfDir+"/ref_hash_3.txt",
                             c1Dir+"/hash_3.txt",
                             fileSize1, random_seed2);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash3");
    err = createCopyMoveFile(rfDir+"/ref_hash_3.txt",
                             c1Dir+"/hash_3.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash3");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // -----------------
    // Test: both clients create the same file simultaneously, sync client1 first.
    // LastChang Time in CVS should not be changed by client2

    // Create a file on Client1 and set reference mtime
    err = createCopyMoveFile(rfDir+"/ref_hash_4.txt",
                             c1Dir+"/hash_4.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash5");
    err = VPLFile_SetTime((c1Dir+"/hash_4.txt").c_str(), ref_mtime);
    CHECK_DO_TEST_ERR(err, rv, "ccmSetTime");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    VPLTime_t vcs_time2 = 0;
    err = getLastChangeOnInfra(tua, client1->serverPath+"/hash_4.txt",
         /* OUT */ vcs_time2);
    if (err != 0) {
        LOG_ERROR("get LastChange Time from VCS failed");
        rv++;
    } else {
        if (vcs_time != vcs_time2) {
            LOG_ERROR("Original LastChange Time in VCS has been changed. old:"FMTu64
                ", new:"FMTu64, vcs_time, vcs_time2);
            rv++;
        } else {
            LOG_INFO("original LastChange Time from VCS is:"FMTu64, vcs_time2);
        }
    }

    // Create the same file on Client2
    err = createCopyMoveFile(rfDir+"/ref_hash_4.txt",
                             c2Dir+"/hash_4.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash4");

    // Sync client1's data on server to client2, should nothing uploaded
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest16_upload_hash_match(const SCWTestUserAcct& tua, int& error_out,
                                  int& success_out, VPLTime_t& vcs_time)
{
    error_out = 0;
    success_out = 0;
    int err = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_hash_3.txt",
                      c1Dir+"/hash_3.txt",
                      c2Dir+"/hash_3.txt", error_out, success_out);

    // TODO: revision of hash_3.txt in Client2 should be 1

    // Check current lastChanged from VCS and the 1st time we upload, should be the same.
    VPLTime_t final_vcs_time = 0;
    err = getLastChangeOnInfra(tua, client1->serverPath+"/hash_3.txt",
         /* OUT */ final_vcs_time);
    if (err != 0) {
        LOG_ERROR("get LastChange Time from VCS failed");
        error_out++;
    }
    if (vcs_time != final_vcs_time) {
        LOG_ERROR("lastChanged in VCS should NOT be updated. original time:"
            FMTu64", new time:"FMTu64, vcs_time, final_vcs_time);
        error_out++;
    } else {
        LOG_INFO("lastChanged time in VCS of file %s:"FMTu64,
             (c1Dir+"/hash_3.txt").c_str(), vcs_time);
        success_out++;
    }
    
    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_hash_4.txt",
                      c1Dir+"/hash_4.txt",
                      c2Dir+"/hash_4.txt", error_out, success_out);

    // Check current lastChanged from VCS and the 1st time we upload, should be the same.
    err = getLastChangeOnInfra(tua, client1->serverPath+"/hash_4.txt",
         /* OUT */ final_vcs_time);
    if (err != 0) {
        LOG_ERROR("get LastChange Time from VCS failed");
        error_out++;
    }
    if (vcs_time != final_vcs_time) {
        LOG_ERROR("lastChanged in VCS should NOT be updated. original time:"FMTu64
            ", new time:"FMTu64", file:%s", vcs_time, final_vcs_time,
                 (client1->serverPath+"/hash_4.txt").c_str());
        error_out++;
    } else {
        LOG_INFO("lastChanged time in VCS of file %s:"FMTu64,
            (client1->serverPath+"/hash_4.txt").c_str(), vcs_time);
        success_out++;
    }
}

static int doTest17_hash_fix_conflict(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;
    int fileSize1 = 200;
    int fileSize3 = 202;
    int random_seed1 = 30;
    int random_seed2 = 31;
    int random_seed3 = 32;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    client1->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client2->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client1->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    client2->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    std::string rfDir(TEST_TMP_DIR);  // reference directory
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // -----------------
    //   1. Client1: Create a file, sync to server
    //   2. Client2: sync from server
    //   3. Client1: update the file -> rollback
    //   4. Client2: update/delete the file to different content
    //   5. Client1: sync to server. (nothing uploaded)
    //   6. Client2: sync to server. (updated file uploaded/deleted)
    //   7. Client1: sync from server
    // File should be the content updated by Client2

    // Create files on client 1
    // hash value is "a6333a914fffc73f71cb59b802d45bba421ffb1a"
    err = createCopyMoveFile(rfDir+"/ref_hash_5.txt",
                             c1Dir+"/hash_5.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash5");

    err = createCopyMoveFile(rfDir+"/ref_hash_6.txt",
                             c1Dir+"/hash_6.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash6");
    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // See Note: ENSURE_DIFFERENT_MTIME
    VPLThread_Sleep(VPLTime_FromSec(MAX_FS_GRANULARITY_SEC));

    // Change & Rollback client1's data
    err = createCopyMoveFile(rfDir+"/ref_hash_5.txt",
                             c1Dir+"/hash_5.txt",
                             fileSize1, random_seed2);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash5");
    err = createCopyMoveFile(rfDir+"/ref_hash_5.txt",
                             c1Dir+"/hash_5.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash5");
    err = createCopyMoveFile(rfDir+"/ref_hash_6.txt",
                             c1Dir+"/hash_6.txt",
                             fileSize1, random_seed2);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash6");
    err = createCopyMoveFile(rfDir+"/ref_hash_6.txt",
                             c1Dir+"/hash_6.txt",
                             fileSize1, random_seed1);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash6");

    // Update client2's data
    err = createCopyMoveFile(rfDir+"/ref_hash_5.txt",
                             c2Dir+"/hash_5.txt",
                             fileSize3, random_seed3);
    CHECK_DO_TEST_ERR(err, rv, "ccmHash5");

    // Delete client2's data (another test)
    err = deleteLocalFiles(c2Dir+"/hash_6.txt");
    CHECK_DO_TEST_ERR(err, rv, "deleteHash6");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");

    return rv;
}

static void chkTest17_hash_fix_conflict(const SCWTestUserAcct& tua, int& error_out,
                                  int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Check files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ref_hash_5.txt",
                      c1Dir+"/hash_5.txt",
                      c2Dir+"/hash_5.txt", error_out, success_out);

    CHECK_DENT_NOT_EXIST((c1Dir+"/hash_6.txt").c_str(), error_out, success_out, "c1");
    CHECK_DENT_NOT_EXIST((c2Dir+"/hash_6.txt").c_str(), error_out, success_out, "c2");
}

static int createTestFile(const std::string& filename,
                            const unsigned char* content,
                            u64 sizeOfDataFile_bytes)
{
    int toReturn = 0;
    const size_t blocksize = (1 << 15);  // 32768

    VPLFile_handle_t fp = VPLFile_Open(filename.c_str(),
                                       VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE,
                                       0777);
    if(!VPLFile_IsValidHandle(fp)) {
        LOG_ERROR("Fail open file:%s, %d", filename.c_str(), (int)fp);
        toReturn = (int)fp;
        goto fail_open;
    }
    for(u64 totalBytesWritten = 0; totalBytesWritten < sizeOfDataFile_bytes;) {
        u64 bytesToWrite = sizeOfDataFile_bytes - totalBytesWritten;
        if(bytesToWrite >= blocksize) {
            bytesToWrite = blocksize;
        }
        u32 bytesWritten = VPLFile_Write(fp, content + totalBytesWritten, (size_t)bytesToWrite);
        if(bytesWritten != bytesToWrite) {
            LOG_ERROR("incomplete bytes written %d:"FMTu64", %s\n",
                      bytesWritten, bytesToWrite, filename.c_str());
            toReturn = -3;
            break;
        }
        if(bytesWritten <= 0) {
            LOG_ERROR("writeError:%d,%s at offset:"FMTu64,
                      bytesWritten, filename.c_str(), totalBytesWritten);
            toReturn = bytesWritten;
            break;
        }
        totalBytesWritten += bytesWritten;
    }

    VPLFile_Close(fp);
 fail_open:
    return toReturn;
}

static void checkHashCorrectness(const SCWTestUserAcct& tua, const SCWTestClient& tss,
        const std::string& rel_path, const std::string hash_value, int& error_out,
                                  int& success_out)
{
    int rc;
    u64 component_id = 0;
    u64 rev_id = 0;
    bool is_on_acs;
    std::string absolute_path;
    u64 local_modify_time;
    std::string hash_out;

    DatasetAccessInfo dsetAccessInfo;
    dsetAccessInfo.sessionHandle = tua.vcsTestSession.sessionHandle;
    dsetAccessInfo.serviceTicket = tua.vcsTestSession.sessionServiceTicket;
    dsetAccessInfo.urlPrefix = tua.vcsTestSession.urlPrefix;
    dsetAccessInfo.fileUrlAccess = &myFileUrlAccess;
    SyncConfigThreadPool threadPool;
    SyncConfigThreadPool* threadPoolPtr = NULL;
    if(!threadPool.CheckInitSuccess()) {
        LOG_ERROR("CheckInitSuccess");
    }
    if(tss.syncType == SYNC_TYPE_ONE_WAY_UPLOAD ||
       tss.syncType == SYNC_TYPE_ONE_WAY_DOWNLOAD)
    {
        threadPool.AddGeneralThreads(3);
        threadPoolPtr = &threadPool;
    }

    SyncConfig* syncConfig = CreateSyncConfig(
            tua.vcsTestSession.userId,
            tua.getDataset(),
            tss.syncType,
            tss.syncPolicy,
            tss.localPath,
            tss.serverPath,
            dsetAccessInfo,
            threadPoolPtr,
            true, // dedicatedThread
            SyncConfigEventCb,
            (void*)"TEST_CTX",  // callback_context
            rc);
    if(rc != 0) {
        LOG_ERROR("SyncConfig:%d", rc);
        error_out++;
        return;
    }

    rc = syncConfig->LookupComponentByPath(rel_path, component_id, rev_id, is_on_acs);
    if (rc != 0) {
        LOG_ERROR("LookupComponentByPath failed");
        error_out++;
        goto exit;
    }
    rc = syncConfig->LookupAbsPath(component_id, rev_id, absolute_path, local_modify_time, hash_out);
    if (rc != 0) {
        LOG_ERROR("LoockupAbsPath failed");
        error_out++;
        goto exit;
    }

 exit:
    {
        int reqCloseRes = syncConfig->RequestClose();
        if(reqCloseRes != 0) {
            LOG_ERROR("RequestClose:%d", reqCloseRes);
            error_out++;
        }
        if(reqCloseRes==0) {  // No sense in joining if the request failed.
            rc = syncConfig->Join();
            if(rc != 0) {
                LOG_ERROR("Join:%d", rc);
                error_out++;
            }
        }
    }
    DestroySyncConfig(syncConfig);

    // There should be no active threads, so this should not be
    // doing anything messy (it will return an error if there's still an active
    // task and prevent other tasks from being enqueued)
    rc = threadPool.CheckAndPrepareShutdown();
    if (rc != 0) {
        u32 unoccupied_threads;
        u32 total_threads;
        bool shuttingDown;
        threadPool.GetInfoIncludeDedicatedThread(NULL,
                                                 /*OUT*/unoccupied_threads,
                                                 /*OUT*/total_threads,
                                                 /*OUT*/shuttingDown);
        LOG_ERROR("threadPool.Shutdown:%d, unoccupied_threads(%d), "
                  "total_threads(%d), shuttingDown(%d).  THERE SHOULD BE NO TASKS!",
                  rc, unoccupied_threads, total_threads, shuttingDown);
        if(rc != 0) {
            error_out++;
        }
    }

    rc = hash_out.compare(hash_value);
    if (rc != 0) {
        LOG_ERROR("Hash in DB is incorrect. hash_value in DB:%s, correct hash:%s, file name:%s",
                hash_out.c_str(), hash_value.c_str(), absolute_path.c_str());
        error_out++;
    } else {
        success_out++;
    };
}

// To test the correctness of the hash value of each file
static int doTest18_hash_value(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    SyncConfigTestFileList flist;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    client1->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client2->syncPolicy.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    client1->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    client2->syncPolicy.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;

    // Create file to verify hash calculation
    err = createTestFile(rfDir+"/ref_haveValueTest_0.txt", flist.mFiles[0].content, flist.mFiles[0].len);
    CHECK_DO_TEST_ERR(err, rv, "chv0");
    err = createTestFile(c1Dir+"/haveValueTest_0.txt", flist.mFiles[0].content, flist.mFiles[0].len);
    CHECK_DO_TEST_ERR(err, rv, "chv0");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client1's data on server to client2
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");
    return rv;
}


static void chkTest18_hash_value(const SCWTestUserAcct& tua, int& error_out,
                                  int& success_out)
{
    SyncConfigTestFileList flist;
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    CHECK_FILE_TO_REF(rfDir+"/ref_haveValueTest_0.txt",
                      c1Dir+"/haveValueTest_0.txt",
                      c2Dir+"/haveValueTest_0.txt", error_out, success_out);

    checkHashCorrectness(tua, *client1, "haveValueTest_0.txt",
            flist.mFiles[0].hashValue, error_out, success_out);
}
#endif // COMPUTE_HASH_FOR_ACS_SYNC

// Greater than VCS GetDir page size, which is 500 as of 11/13/2013
const u32 NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE = 888;

// Number of files should be at least 1 VCS GetDir page size
static int doTest50_large_number_of_files_add_OW(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Complete reset of state being done for all the 50's series of large tests.
    // Comment Note: 50s_SERIES_TEST_RESET
    err = resetTestState(tua, false);
    CHECK_DO_TEST_ERR(err, rv, "resetTestState");

    for(u32 i=0; i<NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE; i++) {
        std::ostringstream myStream;
        myStream << std::setw(4) << std::setfill('0') << i << ".txt";
        std::string toAppend = myStream.str();

        std::string refFileName = rfDir + "/ow_ref_many_files_add_" + toAppend;
        std::string c1FileName = c1Dir + "/ow_many_files_add_" + toAppend;
        err = createCopyMoveFile(refFileName,
                                 c1FileName,
                                 17, 5000+i);
        CHECK_DO_TEST_ERR(err, rv, c1FileName.c_str());
    }

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest50_large_number_of_files_add_OW(int& error_out,
                                                   int& success_out)
{
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    for(u32 i=0; i<NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE; i++) {
        std::ostringstream myStream;
        myStream << std::setw(4) << std::setfill('0') << i << ".txt";
        std::string toAppend = myStream.str();

        CHECK_FILE_TO_REF(rfDir+"/ow_ref_many_files_add_"+toAppend,
                          c1Dir+"/ow_many_files_add_"+toAppend,
                          c2Dir+"/ow_many_files_add_"+toAppend,
                          error_out, success_out);
    }

    // Check to make sure there are exactly the amount of files expected.
    u32 numEntries;
    int rc = countEntriesInDir(c2Dir, /*out*/ numEntries);
    if (rc != 0) {
        LOG_ERROR("countEntriesInDir(%s):%d", c2Dir.c_str(), rc);
        error_out++;
        return;
    }

    if (numEntries != NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE) {
        LOG_ERROR("Bad count in (%s):%d.  Should be %d",
                  c2Dir.c_str(), numEntries,
                  NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE);
        error_out++;
    } else {
        success_out++;
    }
    return;
}

static int doTest51_initial_sync_large_delete_OW(const SCWTestUserAcct& tua)
{
    int rv = 0;
    int err;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    err = deleteLocalFiles(client1->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = Util_CreatePath(client1->localPath.c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "makeClientDir");
    // Delete only the sync temp directory.  See if the unneeded files get removed.
    err = deleteLocalFiles(client2->localPath+"/"TEST_SYNC_TEMP);
    CHECK_DO_TEST_ERR(err, rv, "deleteC2");

    err = createCopyMoveFile(rfDir+"/ow_ref_TheOneSingleFile.txt",
                             c1Dir+"/ow_TheOneSingleFile.txt",
                             75755, 800);
    CHECK_DO_TEST_ERR(err, rv, "OneSingleFile");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest51_initial_sync_large_delete_OW(int& error_out,
                                                   int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_ONE_WAY_UP);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_ONE_WAY_DOWN);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/ow_ref_TheOneSingleFile.txt",
                      c1Dir+"/ow_TheOneSingleFile.txt",
                      c2Dir+"/ow_TheOneSingleFile.txt", error_out, success_out);

    // Check to make sure there are exactly the amount of files expected.
    u32 numEntries;
    int rc = countEntriesInDir(c2Dir, /*out*/ numEntries);
    if (rc != 0) {
        LOG_ERROR("countEntriesInDir(%s):%d", c2Dir.c_str(), rc);
        error_out++;
        return;
    }

    // Check that there's only 1 file.
    if (numEntries != 1) {
        LOG_ERROR("Bad count in (%s):%d.  Should be %d",
                  c2Dir.c_str(), numEntries, 1);
        error_out++;
    } else {
        success_out++;
    }
}

// Number of files should be at least 1 VCS GetDir page size
static int doTest52_large_number_of_files_add_TW(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Test reset should've been done in the 50's series of tests.  Not clearing infra
    // state here as that takes a long time.  See Note: 50s_SERIES_TEST_RESET
    err = deleteLocalFiles(client1->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = Util_CreatePath(client1->localPath.c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "makeClientDir");

    for(u32 i=0; i<NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE; i++) {
        std::ostringstream myStream;
        myStream << std::setw(4) << std::setfill('0') << i << ".txt";
        std::string toAppend = myStream.str();

        std::string refFileName = rfDir + "/tw_ref_many_files_add_" + toAppend;
        std::string c1FileName = c1Dir + "/tw_many_files_add_" + toAppend;
        err = createCopyMoveFile(refFileName,
                                 c1FileName,
                                 17, 6000+i);
        CHECK_DO_TEST_ERR(err, rv, c1FileName.c_str());
    }

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest52_large_number_of_files_add_TW(int& error_out,
                                                   int& success_out)
{
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    for(u32 i=0; i<NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE; i++) {
        std::ostringstream myStream;
        myStream << std::setw(4) << std::setfill('0') << i << ".txt";
        std::string toAppend = myStream.str();

        CHECK_FILE_TO_REF(rfDir+"/tw_ref_many_files_add_"+toAppend,
                          c1Dir+"/tw_many_files_add_"+toAppend,
                          c2Dir+"/tw_many_files_add_"+toAppend,
                          error_out, success_out);
    }

    // Check to make sure there are exactly the amount of files expected.
    u32 numEntries;
    int rc = countEntriesInDir(c2Dir, /*out*/ numEntries);
    if (rc != 0) {
        LOG_ERROR("countEntriesInDir(%s):%d", c2Dir.c_str(), rc);
        error_out++;
        return;
    }

    if (numEntries != NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE) {
        LOG_ERROR("Bad count in (%s):%d.  Should be %d",
                  c2Dir.c_str(), numEntries,
                  NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE);
        error_out++;
    } else {
        success_out++;
    }
    return;
}

static int doTest53_large_number_of_files_touch_TW(const SCWTestUserAcct& tua)
{
    int err;
    int rv = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    for(u32 i=0; i<NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE; i++) {
        std::ostringstream myStream;
        myStream << std::setw(4) << std::setfill('0') << i << ".txt";
        std::string toAppend = myStream.str();

        std::string refFileName = rfDir + "/tw_ref_many_files_add_" + toAppend;
        std::string c1FileName = c1Dir + "/tw_many_files_add_" + toAppend;
        err = createCopyMoveFile(refFileName,
                                 c1FileName,
                                 17, 6000+i);
        CHECK_DO_TEST_ERR(err, rv, c1FileName.c_str());
    }

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest53_large_number_of_files_touch_TW(int& error_out,
                                                   int& success_out)
{
    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    for(u32 i=0; i<NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE; i++) {
        std::ostringstream myStream;
        myStream << std::setw(4) << std::setfill('0') << i << ".txt";
        std::string toAppend = myStream.str();

        CHECK_FILE_TO_REF(rfDir+"/tw_ref_many_files_add_"+toAppend,
                          c1Dir+"/tw_many_files_add_"+toAppend,
                          c2Dir+"/tw_many_files_add_"+toAppend,
                          error_out, success_out);
    }

    // Check to make sure there are exactly the amount of files expected.
    u32 numEntries;
    int rc = countEntriesInDir(c2Dir, /*out*/ numEntries);
    if (rc != 0) {
        LOG_ERROR("countEntriesInDir(%s):%d", c2Dir.c_str(), rc);
        error_out++;
        return;
    }

    if (numEntries != NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE) {
        LOG_ERROR("Bad count in (%s):%d.  Should be %d",
                  c2Dir.c_str(), numEntries,
                  NUM_GREATER_THAN_CLIENT_VCS_GET_DIR_PAGE);
        error_out++;
    } else {
        success_out++;
    }
    return;
}

// Number of files should be at least 1 VCS GetDir page size
static int doTest54_large_number_of_files_delete_TW(const SCWTestUserAcct& tua)
{
    int rv = 0;
    int err;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    err = deleteLocalFilesExceptSyncTemp(client1->localPath);
    CHECK_DO_TEST_ERR(err, rv, "deleteC1");
    err = Util_CreatePath(client1->localPath.c_str(), true);
    CHECK_DO_TEST_ERR(err, rv, "makeClientDir");

    err = createCopyMoveFile(rfDir+"/tw_ref_TheOneSingleFile.txt",
                             c1Dir+"/tw_TheOneSingleFile.txt",
                             75755, 800);
    CHECK_DO_TEST_ERR(err, rv, "OneSingleFile");

    // Sync client1's data to server
    err = SyncConfigWorkerSync(tua, *client1);
    CHECK_DO_TEST_ERR(err, rv, "syncC1");
    // Sync client2's data to server
    err = SyncConfigWorkerSync(tua, *client2);
    CHECK_DO_TEST_ERR(err, rv, "syncC2");

    return rv;
}

static void chkTest54_large_number_of_files_delete_TW(int& error_out,
                                                      int& success_out)
{
    error_out = 0;
    success_out = 0;

    SCWTestClient* client1 = getTestSpec(SYNC_SPEC_CLIENT_1_TWO_WAY);
    SCWTestClient* client2 = getTestSpec(SYNC_SPEC_CLIENT_2_TWO_WAY);
    std::string rfDir(TEST_TMP_DIR);
    std::string c1Dir = client1->localPath;
    std::string c2Dir = client2->localPath;

    // Files originally on client 1
    CHECK_FILE_TO_REF(rfDir+"/tw_ref_TheOneSingleFile.txt",
                      c1Dir+"/tw_TheOneSingleFile.txt",
                      c2Dir+"/tw_TheOneSingleFile.txt", error_out, success_out);

    // Check to make sure there are exactly the amount of files expected.
    u32 numEntries;
    int rc = countEntriesInDir(c2Dir, /*out*/ numEntries);
    if (rc != 0) {
        LOG_ERROR("countEntriesInDir(%s):%d", c2Dir.c_str(), rc);
        error_out++;
        return;
    }

    // Check that there's only 1 file.
    if (numEntries != 1) {
        LOG_ERROR("Bad count in (%s):%d.  Should be %d",
                  c2Dir.c_str(), numEntries, 1);
        error_out++;
    } else {
        success_out++;
    }
}

#if defined(IOS) || defined(VPL_PLAT_IS_WINRT) || defined(ANDROID)
int syncConfigTest(int argc, const char ** argv)
#else
int main(int argc, char* argv[])
#endif
{
    int totalFail = 0;
    int totalSuccess = 0;
    int errors = 0 ;
    int t0_ResetErr = 0;
    int t1_createDirsErr = 0;
    int t1_createDirsSuccess = 0;
    int t2_removeDirsErr = 0;
    int t2_removeDirsSuccess = 0;
    int t3_createFileErr = 0;
    int t3_createFileSuccess = 0;
    int t4_updateFileErr = 0;
    int t4_updateFileSuccess = 0;
    int t5_removeFileErr = 0;
    int t5_removeFileSuccess = 0;
    int t6_createDirsErr = 0;
    int t6_createDirsSuccess = 0;
    int t7_removeDirsErr = 0;
    int t7_removeDirsSuccess = 0;
    int t8_createFileErr = 0;
    int t8_createFileSuccess = 0;
    int t8a_syncTypeSwitchErr = 0;
    int t8a_syncTypeSwitchSuccess = 0;
    int t8b_syncTypeSwitchErr = 0;
    int t8b_syncTypeSwitchSuccess = 0;
    int t9_updateFileErr = 0;
    int t9_updateFileSuccess = 0;
    int t10_removeFileErr = 0;
    int t10_removeFileSuccess = 0;
    int t11_initialSyncErr = 0;
    int t11_initialSyncSuccess = 0;
    int t12_A_conflictFileErr = 0;
    int t12_A_conflictFileSuccess = 0;
    int t12_B_conflictFileErr = 0;
    int t12_B_conflictFileSuccess = 0;
    int t13_doOnceOWErr = 0;
    int t13_doOnceOWSuccess = 0;
    int t14_doOnceTWUpErr = 0;
    int t14_doOnceTWUpSuccess = 0;
#ifdef COMPUTE_HASH_FOR_ACS_SYNC
    int t15_downloadHashMatchErr = 0;
    int t15_downloadHashMatchSuccess = 0;
    int t16_uploadHashMatchErr = 0;
    int t16_uploadHashMatchSuccess = 0;
    int t17_hashFixConflictErr = 0;
    int t17_hashFixConflictSuccess = 0;
    int t18_hashValueErr = 0;
    int t18_hashValueSuccess = 0;
#endif // COMPUTE_HASH_FOR_ACS_SYNC
    int t50_manyFilesAddOwErr = 0;
    int t50_manyFilesAddOwSuccess = 0;
    int t51_initialSyncLargeDeleteOwErr = 0;
    int t51_initialSyncLargeDeleteOwSuccess = 0;
    int t52_manyFilesAddTwErr = 0;
    int t52_manyFilesAddTwSuccess = 0;
    int t53_manyFilesTouchTwErr = 0;
    int t53_manyFilesTouchTwSuccess = 0;
    int t54_initialSyncLargeDeleteTwErr = 0;
    int t54_initialSyncLargeDeleteTwSuccess = 0;
    bool setupFailed = false;
    bool printVerboseLog = false;
    bool do_large_test_only = false;

    const bool DO_ONE_WAY_ONLY = false;
    const bool DO_CONFLICT_FILE_ONLY = false;

    const bool SKIP_SYNC_TYPE_SWITCH = false;

    int setup = VPL_Init();
    if(setup != 0) {
        LOG_ERROR("VPL_Init():%d", setup);
        return setup;
    }

    setup = VPLHttp2::Init();
    if(setup != 0) {
        LOG_ERROR("VPLHttp2::Init():%d", setup);
        return setup;
    }

#ifdef VPL_PLAT_IS_WINRT
    {
        _VPLFS__GetLocalAppDataPath(&root_path);
        for (int i = 1; i < strlen(root_path); i++) {
            if (root_path[i] == '\\') {
                root_path[i] = '/';
            }
        }
        std::string strPath(root_path);
        LOGInit("syncConfig", root_path);

        // Init TEST_BASE_DIR & TEST_TMP_DIR
        TEST_BASE_DIR = strPath + "/tmp/testSyncConfig";
        TEST_TMP_DIR = TEST_BASE_DIR + "/tmp";
        TEST_MOVE_DIR = TEST_TMP_DIR + "/to_move";
        TEST_DELETE_DIR = TEST_TMP_DIR + "/to_delete";
    }
#elif defined(IOS)
    {
        _VPLFS__GetHomeDirectory(&root_path);
        std::string strPath(root_path);
        LOGInit("SyncConfig", root_path);

        // Init TEST_BASE_DIR & TEST_TMP_DIR
        TEST_BASE_DIR = strPath + "/tmp/testSyncConfig";
        TEST_TMP_DIR = TEST_BASE_DIR + "/tmp";
        TEST_MOVE_DIR = TEST_TMP_DIR + "/to_move";
        TEST_DELETE_DIR = TEST_TMP_DIR + "/to_delete";
    }
#elif defined(ANDROID)
    {
        root_path = NULL;
        // Init TEST_BASE_DIR & TEST_TMP_DIR
        TEST_BASE_DIR = "/sdcard/testSyncConfig";
        TEST_TMP_DIR = TEST_BASE_DIR + "/tmp";
        TEST_MOVE_DIR = TEST_TMP_DIR + "/to_move";
        TEST_DELETE_DIR = TEST_TMP_DIR + "/to_delete";
    }
#elif defined(CLOUDNODE)
    {
        LOGInit(NULL, NULL);
    }    
#else
    LOGInit(NULL, NULL);
#endif

    {
        int rv = 0;
        int err = 0;
        std::string strTEST_SQLITE_TEMP = TEST_BASE_DIR+"/tmp_sqlite";

        err = Util_rmRecursive(strTEST_SQLITE_TEMP, TEST_DELETE_DIR);
        CHECK_DO_TEST_ERR(err, rv, "clearTmpSqlite");

        err = Util_CreateDir(strTEST_SQLITE_TEMP.c_str());
        CHECK_DO_TEST_ERR(err, rv, "mkdirTmpSqlite");

        err = Util_InitSqliteTempDir(strTEST_SQLITE_TEMP);
        if (err!=0) {
            LOG_ERROR("Util_InitSqliteTempDir(%s):%d",
                       strTEST_SQLITE_TEMP.c_str(), err);
            rv++;
        }
    }

    if(printVerboseLog) {
        LOGSetLevel(LOG_LEVEL_DEBUG, 1);
    }

    initSyncClientMap();
    if(argc != 5 && argc != 6) {
        LOG_ERROR("Incorrect Args: Usage: %s <username> <password>"
                  " <ias_central_domain> <ias_central_port>"
                  " [--do_large_test (does not include default smoke test)]", argv[0]);
        LOG_ALWAYS("Example arguments: %s sync_config_test_acct001@acercloud.net password "
                   "pc-int.igware.net 443",
                   argv[0]);
        return -7;
    }
    SCWTestUserAcct tua;
    tua.username = std::string(argv[1]);
    tua.password = std::string(argv[2]);
    tua.ias_central_url = std::string(argv[3]);
    tua.ias_central_port = atoi(argv[4]);
    tua.acctNamespace = std::string("acer");
    tua.datasetName = std::string("Media Metadata");

    if (argc == 6) {
        if (std::string("--do_large_test") == argv[5]) {
            do_large_test_only = true;
        } else {
            LOG_ERROR("Unexpected 5th argument:%s", argv[5]);
            return -10;
        }
    }

    LOG_ALWAYS("%s user(%s),password(%s),ias_central_url(%s),ias_central_port(%d),do_large_test(%d)",
               argv[0], tua.username.c_str(), tua.password.c_str(),
               tua.ias_central_url.c_str(), tua.ias_central_port,
               do_large_test_only);

    int rc;
    rc = setupTestAcctSession(tua);
    if(rc != 0) {
        LOG_ERROR("setupTestAcctSession:%d - Fatal error (%s, %s, %s)"
                  "\nTC_RESULT=FAIL ;;; TC_NAME=SetupTestAccount",
                  rc,
                  tua.username.c_str(),
                  tua.password.c_str(),
                  tua.ias_central_url.c_str());
        return -8;
    }

    LOG_INFO("==== BEGINNING TEST - Reset test state ====");
    t0_ResetErr = resetTestState(tua, printVerboseLog);
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T0_reset", t0_ResetErr==0?"PASS":"FAIL");
    totalFail += t0_ResetErr;

    if(t0_ResetErr>0) {
        LOG_ERROR("Beginning test failed, cutting short");
        setupFailed = true;
        goto print_results;
    }

    if(DO_ONE_WAY_ONLY) {
        goto one_way_label;
    }
    if(DO_CONFLICT_FILE_ONLY) {
        goto conflict_file_label;
    }
    if(do_large_test_only) {
        goto large_test_label_1;
    }

    LOG_INFO("==== Setup Test1 - Create Dir ====");
    errors = doTest1_create_dirs(tua);
    chkTest1_create_dirs(t1_createDirsErr, t1_createDirsSuccess);
    t1_createDirsErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T1_create_dirs", t1_createDirsErr==0?"PASS":"FAIL");
    totalFail += t1_createDirsErr;
    totalSuccess += t1_createDirsSuccess;

    LOG_INFO("==== Setup Test2 - Remove Dir ====");
    errors = doTest2_remove_dirs(tua);
    chkTest2_remove_dirs(t2_removeDirsErr, t2_removeDirsSuccess);
    t2_removeDirsErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T2_remove_dirs", t2_removeDirsErr==0?"PASS":"FAIL");
    totalFail += t2_removeDirsErr;
    totalSuccess += t2_removeDirsSuccess;

    LOG_INFO("==== Setup Test3 - Create File ====");
    errors = doTest3_create_file(tua);
    chkTest3_create_files(t3_createFileErr, t3_createFileSuccess);
    t3_createFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T3_create_files", t3_createFileErr==0?"PASS":"FAIL");
    totalFail += t3_createFileErr;
    totalSuccess += t3_createFileSuccess;

    LOG_INFO("==== Setup Test4 - Update File ====");
    errors = doTest4_update_file(tua);
    chkTest4_update_files(t4_updateFileErr, t4_updateFileSuccess);
    t4_updateFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T4_update_files", t4_updateFileErr==0?"PASS":"FAIL");
    totalFail += t4_updateFileErr;
    totalSuccess += t4_updateFileSuccess;

    LOG_INFO("==== Setup Test5 - Remove File ====");
    errors = doTest5_remove_file(tua);
    chkTest5_remove_files(t5_removeFileErr, t5_removeFileSuccess);
    t5_removeFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T5_remove_files", t5_removeFileErr==0?"PASS":"FAIL");
    totalFail += t5_removeFileErr;
    totalSuccess += t5_removeFileSuccess;

 one_way_label:
    LOG_INFO("==== Setup Test6 - Create Dir OW ====");
    errors = doTest6_create_dirs_OW(tua);
    chkTest6_create_dirs_OW(t6_createDirsErr, t6_createDirsSuccess);
    t6_createDirsErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T6_create_dirs_ow", t6_createDirsErr==0?"PASS":"FAIL");
    totalFail += t6_createDirsErr;
    totalSuccess += t6_createDirsSuccess;

    LOG_INFO("==== Setup Test7 - Remove Dir OW ====");
    errors = doTest7_remove_dirs_OW(tua);
    chkTest7_remove_dirs_OW(t7_removeDirsErr, t7_removeDirsSuccess);
    t7_removeDirsErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T7_remove_dirs_ow", t7_removeDirsErr==0?"PASS":"FAIL");
    totalFail += t7_removeDirsErr;
    totalSuccess += t7_removeDirsSuccess;

    LOG_INFO("==== Setup Test8 - Create File OW ====");
    errors = doTest8_create_file_OW(tua);
    chkTest8_create_files_OW(t8_createFileErr, t8_createFileSuccess);
    t8_createFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T8_create_file_ow", t8_createFileErr==0?"PASS":"FAIL");
    totalFail += t8_createFileErr;
    totalSuccess += t8_createFileSuccess;

    if(SKIP_SYNC_TYPE_SWITCH) {
        goto skip_sync_type_switch;
    }

    LOG_INFO("==== Setup Test8a - Sync Type Switch A OWD ====");
    errors = doTest8a_sync_type_switch_OWD(tua);
    chkTest8a_sync_type_switch_OWD(t8a_syncTypeSwitchErr, t8a_syncTypeSwitchSuccess);
    t8a_syncTypeSwitchErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T8a_sync_type_switch_owd", t8a_syncTypeSwitchErr==0?"PASS":"FAIL");
    totalFail += t8a_syncTypeSwitchErr;
    totalSuccess += t8a_syncTypeSwitchSuccess;

    LOG_INFO("==== Setup Test8b - Sync Type Switch B OWD ====");
    errors = doTest8b_sync_type_switch_OWD(tua);
    chkTest8b_sync_type_switch_OWD(t8b_syncTypeSwitchErr, t8b_syncTypeSwitchSuccess);
    t8b_syncTypeSwitchErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T8b_sync_type_switch_owd", t8b_syncTypeSwitchErr==0?"PASS":"FAIL");
    totalFail += t8b_syncTypeSwitchErr;
    totalSuccess += t8b_syncTypeSwitchSuccess;

 skip_sync_type_switch:

    LOG_INFO("==== Setup Test9 - Update File OW ====");
    errors = doTest9_update_file_OW(tua);
    chkTest9_update_files_OW(t9_updateFileErr, t9_updateFileSuccess);
    t9_updateFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T9_update_files_ow", t9_updateFileErr==0?"PASS":"FAIL");
    totalFail += t9_updateFileErr;
    totalSuccess += t9_updateFileSuccess;

    LOG_INFO("==== Setup Test10 - Remove File OW ====");
    errors = doTest10_remove_file_OW(tua);
    chkTest10_remove_files_OW(t10_removeFileErr, t10_removeFileSuccess);
    t10_removeFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T10_remove_files_ow", t10_removeFileErr==0?"PASS":"FAIL");
    totalFail += t10_removeFileErr;
    totalSuccess += t10_removeFileSuccess;

    LOG_INFO("==== Setup Test11 - Initial Sync OW ====");
    errors = doTest11_initial_sync_OW(tua);
    chkTest11_initial_sync_OW(t11_initialSyncErr, t11_initialSyncSuccess);
    t11_initialSyncErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T11_initial_sync_ow", t11_initialSyncErr==0?"PASS":"FAIL");
    totalFail += t11_initialSyncErr;
    totalSuccess += t11_initialSyncSuccess;

    if(DO_ONE_WAY_ONLY) {
        goto one_way_do_once;
    }

 conflict_file_label:
    LOG_INFO("==== Setup Test12_A - Conflict File 1stToServerWins DeleteLoser ====");
    errors = doTest12_A_file_conflict_1stToServerWins_loserDelete(tua);
    chkTest12_A_file_conflict_first2ServerWins_loserDelete(t12_A_conflictFileErr, t12_A_conflictFileSuccess);
    t12_A_conflictFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T12_A_cf_1stToServWins_deleteLoser", t12_A_conflictFileErr==0?"PASS":"FAIL");
    totalFail += t12_A_conflictFileErr;
    totalSuccess += t12_A_conflictFileSuccess;

    if(DO_CONFLICT_FILE_ONLY) {
        goto conflict_label_B;
    }

    if(!do_large_test_only) {
        goto one_way_do_once;
    }

 conflict_label_B:
 large_test_label_1:
    // Putting this test in the large (or full) test catagory.
    LOG_INFO("==== Setup Test12_B - Conflict File 1stToServerWins PropagateLoser ====");
    errors = doTest12_B_file_conflict_1stToServerWins_loserPropagate(tua);
    chkTest12_B_file_conflict_first2ServerWins_loserPropagate(t12_B_conflictFileErr, t12_B_conflictFileSuccess);
    t12_B_conflictFileErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T12_B_cf_1stToServWins_propagateLoser", t12_B_conflictFileErr==0?"PASS":"FAIL");
    totalFail += t12_B_conflictFileErr;
    totalSuccess += t12_B_conflictFileSuccess;

    if(DO_CONFLICT_FILE_ONLY) {
        goto print_results;
    }
    if(do_large_test_only) {
        goto large_test_label_2;
    }

 one_way_do_once:
    LOG_INFO("==== Setup Test13 - Do once upload/download OW ====");
    errors = doTest13_uploadOnce_downloadOnce_OW(tua);
    chkTest13_uploadOnce_downloadOnce_OW(tua, t13_doOnceOWErr, t13_doOnceOWSuccess);
    t13_doOnceOWErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T13_do_once_OW", t13_doOnceOWErr==0?"PASS":"FAIL");
    totalFail += t13_doOnceOWErr;
    totalSuccess += t13_doOnceOWSuccess;

    if(DO_ONE_WAY_ONLY) {
         goto print_results;
    }

    LOG_INFO("==== Setup Test14 - Do once upload TW ====");
    errors = doTest14_uploadOnce_TwoWayUp(tua);
    chkTest14_uploadOnce_TwoWayUp(tua, t14_doOnceTWUpErr, t14_doOnceTWUpSuccess);
    t14_doOnceTWUpErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T14_do_once_TW_up", t14_doOnceTWUpErr==0?"PASS":"FAIL");
    totalFail += t14_doOnceTWUpErr;
    totalSuccess += t14_doOnceTWUpSuccess;

#ifdef COMPUTE_HASH_FOR_ACS_SYNC
    // These unit test not really working without ACS support hash,
    //  but still useful for verifing the hash comparing logic.
    // Hard code a hash output of parseFileEntry() in vcs_util.cpp
    // if isDir is false. Then we can test the logic of hash match
    //  by these test case.
    LOG_INFO("==== Setup Test15 - Download Hash match ====");
    VPLTime_t t15_local_time;
    errors = doTest15_download_hash_match(tua, t15_local_time);
    chkTest15_download_hash_match(t15_downloadHashMatchErr, t15_downloadHashMatchSuccess, t15_local_time);
    t15_downloadHashMatchErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T15_download_hash_match", t15_downloadHashMatchErr==0?"PASS":"FAIL");
    totalFail += t15_downloadHashMatchErr;
    totalSuccess += t15_downloadHashMatchSuccess;

    LOG_INFO("==== Setup Test16 - Upload Hash match ====");
    VPLTime_t t16_cvs_time;
    errors = doTest16_upload_hash_match(tua, t16_cvs_time);
    chkTest16_upload_hash_match(tua, t16_uploadHashMatchErr, t16_uploadHashMatchSuccess, t16_cvs_time);
    t16_uploadHashMatchErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T16_upload_hash_match", t16_uploadHashMatchErr==0?"PASS":"FAIL");
    totalFail += t16_uploadHashMatchErr;
    totalSuccess += t16_uploadHashMatchSuccess;

    LOG_INFO("==== Setup Test17 - Hash fix conflict ====");
    errors = doTest17_hash_fix_conflict(tua);
    chkTest17_hash_fix_conflict(tua, t17_hashFixConflictErr, t17_hashFixConflictSuccess);
    t17_hashFixConflictErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T17_hash_fix_conflict", t17_hashFixConflictErr==0?"PASS":"FAIL");
    totalFail += t17_hashFixConflictErr;
    totalSuccess += t17_hashFixConflictSuccess;

    LOG_INFO("==== Setup Test18 - Hash value ====");
    errors = doTest18_hash_value(tua);
    chkTest18_hash_value(tua, t18_hashValueErr, t18_hashValueSuccess);
    t18_hashValueErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T18_hash_value", t18_hashValueErr==0?"PASS":"FAIL");
    totalFail += t18_hashValueErr;
    totalSuccess += t18_hashValueSuccess;
#endif
 large_test_label_2:
    if (!do_large_test_only) {
        goto print_results;
    }
    LOG_INFO("==== Setup Test50 - Many Files Add OW ====");
    errors = doTest50_large_number_of_files_add_OW(tua);
    chkTest50_large_number_of_files_add_OW(t50_manyFilesAddOwErr, t50_manyFilesAddOwSuccess);
    t50_manyFilesAddOwErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T50_many_files_add_ow", t50_manyFilesAddOwErr==0?"PASS":"FAIL");
    totalFail += t50_manyFilesAddOwErr;
    totalSuccess += t50_manyFilesAddOwSuccess;

    LOG_INFO("==== Setup Test51 - Many Files Delete OW ====");
    errors = doTest51_initial_sync_large_delete_OW(tua);
    chkTest51_initial_sync_large_delete_OW(t51_initialSyncLargeDeleteOwErr, t51_initialSyncLargeDeleteOwSuccess);
    t51_initialSyncLargeDeleteOwErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T51_initSyncLargeDelete_ow", t51_initialSyncLargeDeleteOwErr==0?"PASS":"FAIL");
    totalFail += t51_initialSyncLargeDeleteOwErr;
    totalSuccess += t51_initialSyncLargeDeleteOwSuccess;

    LOG_INFO("==== Setup Test52 - Many Files Add TW ====");
    errors = doTest52_large_number_of_files_add_TW(tua);
    chkTest52_large_number_of_files_add_TW(t52_manyFilesAddTwErr, t52_manyFilesAddTwSuccess);
    t52_manyFilesAddTwErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T52_many_files_add_tw", t52_manyFilesAddTwErr==0?"PASS":"FAIL");
    totalFail += t52_manyFilesAddTwErr;
    totalSuccess += t52_manyFilesAddTwSuccess;

    LOG_INFO("==== Setup Test53 - Many Files Touch TW ====");
    errors = doTest53_large_number_of_files_touch_TW(tua);
    chkTest53_large_number_of_files_touch_TW(t53_manyFilesTouchTwErr, t53_manyFilesTouchTwSuccess);
    t53_manyFilesTouchTwErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T53_many_files_touch_tw", t53_manyFilesTouchTwErr==0?"PASS":"FAIL");
    totalFail += t53_manyFilesTouchTwErr;
    totalSuccess += t53_manyFilesTouchTwSuccess;
    goto print_results;

    LOG_INFO("==== Setup Test54 - Many Files Delete TW ====");
    errors = doTest54_large_number_of_files_delete_TW(tua);
    chkTest54_large_number_of_files_delete_TW(t54_initialSyncLargeDeleteTwErr, t54_initialSyncLargeDeleteTwSuccess);
    t54_initialSyncLargeDeleteTwErr += errors;
    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=T54_manyFilesDelete_tw", t54_initialSyncLargeDeleteTwErr==0?"PASS":"FAIL");
    totalFail += t54_initialSyncLargeDeleteTwErr;
    totalSuccess += t54_initialSyncLargeDeleteTwSuccess;

    if(do_large_test_only) {
        goto print_results;
    }

 print_results:
    int fw = 4; // fixed_width
    LOG_ALWAYS("================================================================");
    LOG_ALWAYS("=============== SYNC_CONFIG_WORKER TEST TOTALS =================");
    LOG_ALWAYS("  T0_reset:                                 Errors:%*d", fw,t0_ResetErr);
    if(setupFailed) {LOG_ALWAYS("Setup failed -- Skipping the rest of tests.");}
    LOG_ALWAYS("  T1_create_dirs:            Succeeded:%*d Errors:%*d",fw, t1_createDirsSuccess, fw,t1_createDirsErr);
    LOG_ALWAYS("  T2_remove_dirs:            Succeeded:%*d Errors:%*d",fw, t2_removeDirsSuccess, fw,t2_removeDirsErr);
    LOG_ALWAYS("  T3_create_files:           Succeeded:%*d Errors:%*d",fw, t3_createFileSuccess, fw,t3_createFileErr);
    LOG_ALWAYS("  T4_update_files:           Succeeded:%*d Errors:%*d",fw, t4_updateFileSuccess, fw,t4_updateFileErr);
    LOG_ALWAYS("  T5_remove_files:           Succeeded:%*d Errors:%*d",fw, t5_removeFileSuccess, fw,t5_removeFileErr);
    LOG_ALWAYS("  T6_create_dirs_ow:         Succeeded:%*d Errors:%*d",fw, t6_createDirsSuccess, fw,t6_createDirsErr);
    LOG_ALWAYS("  T7_remove_dirs_ow:         Succeeded:%*d Errors:%*d",fw, t7_removeDirsSuccess, fw,t7_removeDirsErr);
    LOG_ALWAYS("  T8_create_files_ow:        Succeeded:%*d Errors:%*d",fw, t8_createFileSuccess, fw,t8_createFileErr);
    LOG_ALWAYS("  T8a_sync_type_switch_owd:  Succeeded:%*d Errors:%*d",fw, t8a_syncTypeSwitchSuccess, fw,t8a_syncTypeSwitchErr);
    LOG_ALWAYS("  T8b_sync_type_switch_owd:  Succeeded:%*d Errors:%*d",fw, t8b_syncTypeSwitchSuccess, fw,t8b_syncTypeSwitchErr);
    LOG_ALWAYS("  T9_update_files_ow:        Succeeded:%*d Errors:%*d",fw, t9_updateFileSuccess, fw,t9_updateFileErr);
    LOG_ALWAYS("  T10_remove_files_ow:       Succeeded:%*d Errors:%*d",fw, t10_removeFileSuccess, fw,t10_removeFileErr);
    LOG_ALWAYS("  T11_initial_sync_ow:       Succeeded:%*d Errors:%*d",fw, t11_initialSyncSuccess, fw,t11_initialSyncErr);
    LOG_ALWAYS("  T12_A_conflictFileDefault: Succeeded:%*d Errors:%*d",fw, t12_A_conflictFileSuccess, fw,t12_A_conflictFileErr);
    LOG_ALWAYS("  T13_do_once_ow:            Succeeded:%*d Errors:%*d",fw, t13_doOnceOWSuccess, fw,t13_doOnceOWErr);
    LOG_ALWAYS("  T14_do_once_tw_up          Succeeded:%*d Errors:%*d",fw, t14_doOnceTWUpSuccess, fw,t14_doOnceTWUpErr);
#ifdef COMPUTE_HASH_FOR_ACS_SYNC
    LOG_ALWAYS("  T15_download_hash_match    Succeeded:%*d Errors:%*d",fw, t15_downloadHashMatchSuccess, fw,t15_downloadHashMatchErr);
    LOG_ALWAYS("  T16_upload_hash_match      Succeeded:%*d Errors:%*d",fw, t16_uploadHashMatchSuccess, fw,t16_uploadHashMatchErr);
    LOG_ALWAYS("  T17_hash_fix_conflict      Succeeded:%*d Errors:%*d",fw, t17_hashFixConflictSuccess, fw,t17_hashFixConflictErr);
    LOG_ALWAYS("  T18_hash_value             Succeeded:%*d Errors:%*d",fw, t18_hashValueSuccess, fw,t18_hashValueErr);
#endif // COMPUTE_HASH_FOR_ACS_SYNC
    LOG_ALWAYS(" --do_large_test");
    LOG_ALWAYS("  T12_B_conflictFileProp:    Succeeded:%*d Errors:%*d",fw, t12_B_conflictFileSuccess, fw,t12_B_conflictFileErr);
    LOG_ALWAYS("  T50_many_files_add_ow:     Succeeded:%*d Errors:%*d",fw, t50_manyFilesAddOwSuccess, fw,t50_manyFilesAddOwErr);
    LOG_ALWAYS("  T51_initSync_large_del_ow: Succeeded:%*d Errors:%*d",fw, t51_initialSyncLargeDeleteOwSuccess, fw,t51_initialSyncLargeDeleteOwErr);
    LOG_ALWAYS("  T52_many_files_add_tw:     Succeeded:%*d Errors:%*d",fw, t52_manyFilesAddTwSuccess, fw,t52_manyFilesAddTwErr);
    LOG_ALWAYS("  T53_many_files_touch_tw:   Succeeded:%*d Errors:%*d",fw, t53_manyFilesTouchTwSuccess, fw,t53_manyFilesTouchTwErr);
    LOG_ALWAYS("  T54_many_files_del_tw:     Succeeded:%*d Errors:%*d",fw, t54_initialSyncLargeDeleteTwSuccess, fw,t54_initialSyncLargeDeleteTwErr);

    LOG_ALWAYS("End");
    LOG_ALWAYS("TOTAL SUCCEEDED:%d", totalSuccess);
    LOG_ALWAYS("TOTAL ERRORS:%d", totalFail);
    LOG_ALWAYS("=============== SYNC_CONFIG_WORKER TEST TOTALS =================");
    LOG_ALWAYS("================================================================");

    //LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=syncConfigWorkerTest_Suite", totalFail==0?"PASS":"FAIL");

    VPLHttp2::Shutdown();
    google::protobuf::ShutdownProtobufLibrary();  // Login uses protobuf.
    VPLThread_Sleep(VPLTime_FromSec(3));
    return 0;
}

