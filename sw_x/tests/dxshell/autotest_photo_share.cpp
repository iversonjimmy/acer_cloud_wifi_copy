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
#include "autotest_photo_share.hpp"

#include "autotest_common_utils.hpp"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include "gvm_rm_utils.hpp"
#include "HttpAgent.hpp"
#include "photo_share_test.hpp"
#include "scopeguard.hpp"
#include "vplex_file.h"

#include <string>

struct PhotoShareAutoTestState {
    ////////////////////////////////////////////////
    ////  Global Information
    const std::string testName;
    const std::string testDir;
    const std::string testTempDeleteDir;
    const std::string testThumbDirRelToTestDir;

    const int WAIT_FOR_EVENTS_SEC;
    int randSeed;

    ////////////////////////////////////////////////
    ////  Photo Information
    const std::string testFileBase1;
    const std::string testFileBase2;
    const std::string testFileBase3;
    const std::string testFileBaseTwoAccounts4;
    const std::string testFileBaseTwoAccounts5;

    const std::string testFileBase1_opaqueMetadata;
    const std::string testFileBase2_opaqueMetadata;
    const std::string testFileBase3_opaqueMetadata;
    const std::string testFileBaseTwoAccounts4_opaqueMetadata;
    const std::string testFileBaseTwoAccounts5_opaqueMetadata;

    ////////////////////////////////////////////////
    ////  Device-based Information
    std::string cloudPcUsername;
    int cloudPcId;
    u64 cloudPcUserId;
    bool cloudPcEventQueueHandleExists;
    u64 cloudPcEventQueueHandle;

    std::string clientPc_2_Username;
    int clientPcId_2;
    u64 clientPc_2_UserId;
    bool clientPcId_2_EventQueueHandleExists;
    u64 clientPcId_2_EventQueueHandle;
    std::string clientPcId_2_osVersion;

    std::string clientPc_3_Username;
    int clientPcId_3;
    u64 clientPc_3_UserId;
    bool clientPcId_3_EventQueueHandleExists;
    u64 clientPcId_3_EventQueueHandle;

    PhotoShareAutoTestState()
    :   testName("PhotoShareTest"),
#ifdef WIN32
        testDir("C:\\temp\\igware\\autoPhotoShareTest"),
        testTempDeleteDir("C:\\temp\\igware\\tmpDelete"),
#else
        testDir("/tmp/autoPhotoShareTest"),
        testTempDeleteDir("/tmp/tmpDelete"),
#endif
        testThumbDirRelToTestDir("thumb"),
        WAIT_FOR_EVENTS_SEC(30),
        randSeed(0),

        testFileBase1("SharedPhoto_1"),
        testFileBase2("SharedPhoto_2"),
        testFileBase3("SharedPhoto_3"),
        testFileBaseTwoAccounts4("SharedPhotoTwoAccounts_4"),
        testFileBaseTwoAccounts5("SharedPhotoTwoAccounts_5"),

        testFileBase1_opaqueMetadata(testFileBase1),
        testFileBase2_opaqueMetadata(testFileBase2),
        testFileBase3_opaqueMetadata(testFileBase3),
        testFileBaseTwoAccounts4_opaqueMetadata(testFileBaseTwoAccounts4),
        testFileBaseTwoAccounts5_opaqueMetadata(testFileBaseTwoAccounts5),

        cloudPcId(1),
        cloudPcUserId(0),
        cloudPcEventQueueHandleExists(false),
        cloudPcEventQueueHandle(0),

        clientPcId_2(2),
        clientPc_2_UserId(0),
        clientPcId_2_EventQueueHandleExists(false),
        clientPcId_2_EventQueueHandle(0),

        clientPcId_3(3),
        clientPc_3_UserId(0),
        clientPcId_3_EventQueueHandleExists(false),
        clientPcId_3_EventQueueHandle(0)
    {}
};

enum PhotoShareTargetMachine {
    CloudPC,
    Client2,
    Client3
};

static void setTargetMachine(PhotoShareTargetMachine type,
                             const PhotoShareAutoTestState& ts)
{
    // The aliases "CloudPC", "MD", and "Client" are set by the outside makefile
    // in the dxshell command Config Set.
    // Domain is set by tests/make/test_rules.mk
    int rv = 0;
    switch (type) {
    case CloudPC:
        rv = set_target_machine("CloudPC");
        if (rv < 0) {
            setCcdTestInstanceNum(ts.cloudPcId);
        }
        break;
    case Client2:
        rv = set_target_machine("MD");
        if (rv < 0) {
            setCcdTestInstanceNum(ts.clientPcId_2);
        }
        break;
    case Client3:
        rv = set_target_machine("Client");
        if (rv < 0) {
            setCcdTestInstanceNum(ts.clientPcId_3);
        }
        break;
    }
}

static std::string getSharedResolutionFilename(const std::string& base)
{
    return base+".jpg";
}

static std::string getSharedResolutionAbsPath(const PhotoShareAutoTestState& ts,
                                              const std::string& base)
{
    std::string sharedResolutionPhoto;
    Util_appendToAbsPath(ts.testDir,
                         getSharedResolutionFilename(base),
                         sharedResolutionPhoto);
    return sharedResolutionPhoto;
}

static std::string getThumbResolutionFilename(const std::string& base)
{
    return base+"_thumb.jpg";
}

static std::string getThumbDirectory(const PhotoShareAutoTestState& ts)
{
    std::string sharedThumbDir;
    Util_appendToAbsPath(ts.testDir,
                         ts.testThumbDirRelToTestDir,
                         sharedThumbDir);
    return sharedThumbDir;
}

static std::string getThumbResolutionAbsPath(const PhotoShareAutoTestState& ts,
                                             const std::string& base)
{
    std::string thumbResolutionPhoto;
    Util_appendToAbsPath(getThumbDirectory(ts),
                         getThumbResolutionFilename(base),
                         thumbResolutionPhoto);
    return thumbResolutionPhoto;
}

static int PhotoShareEventQueue_create(u64& queueHandle_out)
{
    int rv = 0;

    ccd::EventsCreateQueueInput req;
    ccd::EventsCreateQueueOutput res;
    rv = CCDIEventsCreateQueue(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIEventsCreateQueue failed: %d", rv);
        return rv;
    }

    queueHandle_out = res.queue_handle();
    LOG_ALWAYS("Event queue handle "FMTu64" created.", queueHandle_out);
    return rv;
}

static int PhotoShareEventQueue_destroy(u64 queueHandle)
{
    int rv = 0;

    ccd::EventsDestroyQueueInput req;
    req.set_queue_handle(queueHandle);
    rv = CCDIEventsDestroyQueue(req);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIEventsDestroyQueue failed: %d", rv);
        return rv;
    }

    LOG_ALWAYS("Event queue handle "FMTu64" destroyed.", queueHandle);
    return rv;
}

// Clears all events in the queue
static int PhotoShareEventQueue_clear(u64 queueHandle)
{
    int rv;
    u32 numEvents;

    do {
        ccd::EventsDequeueInput req;
        ccd::EventsDequeueOutput res;
        req.set_queue_handle(queueHandle);
        req.set_max_count(500);
        req.set_timeout(0);
        rv = CCDIEventsDequeue(req, res);
        if (rv != CCD_OK) {
            LOG_ERROR("CCDIEventsDequeue failed: %d", rv);
            return rv;
        }
        numEvents = res.events_size();
    } while(numEvents > 0);

    return rv;
}

static int PhotoShareEventQueue_get(u64 queueHandle,
                                    ccd::EventsDequeueOutput& dequeuedEvents_out)
{
    int rv = 0;
    dequeuedEvents_out.Clear();

    ccd::EventsDequeueInput req;
    req.set_queue_handle(queueHandle);
    req.set_max_count(500);
    req.set_timeout(0);
    rv = CCDIEventsDequeue(req, dequeuedEvents_out);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIEventsDequeue failed: %d", rv);
        return rv;
    }
    return rv;
}

static int createRandomFile(const std::string& filename,
                            u64 sizeOfDataFile_bytes,
                            int rand_seed)
{
    int toReturn = 0;

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

static int createSharedAndThumbResFiles(const std::string& baseFilename,
                                        PhotoShareAutoTestState& ts)
{
    int rv = 0;
    std::string sharedThumbDir;

    {   // Create sharedDir
        rv = Util_CreateDir(ts.testDir.c_str());
        if (rv != 0) {
            LOG_ERROR("Util_CreateDir(%s):%d", ts.testDir.c_str(), rv);
            return rv;
        }

        // Create thumbDir
        sharedThumbDir = getThumbDirectory(ts);
        rv = Util_CreateDir(sharedThumbDir.c_str());
        if (rv != 0) {
            LOG_ERROR("Util_CreateDir(%s):%d", sharedThumbDir.c_str(), rv);
            return rv;
        }
    }
    {   // Generate shared resolution
        std::string sharedResolutionPhoto =
                getSharedResolutionAbsPath(ts, baseFilename);
        rv = createRandomFile(sharedResolutionPhoto,
                              500000,
                              ++ts.randSeed);
        if (rv != 0) {
            LOG_ERROR("createRandomFile(%s):%d", sharedResolutionPhoto.c_str(), rv);
            return rv;
        }
    }

    {   // Generate thumb resolution
        std::string thumbResolutionPhoto =
                getThumbResolutionAbsPath(ts, baseFilename);
       rv = createRandomFile(thumbResolutionPhoto,
                             7000,
                             ++ts.randSeed);
       if (rv != 0) {
           LOG_ERROR("createRandomFile(%s):%d", thumbResolutionPhoto.c_str(), rv);
           return rv;
       }
    }
    return rv;
}

static int setupAutotestPhotoShareFiles(PhotoShareAutoTestState& ts)
{
    int rv = 0;
    rv = Util_rmRecursive(ts.testDir, ts.testTempDeleteDir);
    if (rv != 0) {
        LOG_ERROR("Util_rmRecursive(%s):%d", ts.testDir.c_str(), rv);
        return rv;
    }

    std::string thumbDir = "thumb";

    // How are CCD accounts reset?
    //     Assumption is that each the state has been reset

    // Generate random files.
    rv = createSharedAndThumbResFiles(ts.testFileBase1, ts);
    if (rv != 0) {
        LOG_ERROR("createSharedAndThumbResFiles(%s):%d", ts.testFileBase1.c_str(), rv);
        return rv;
    }
    rv = createSharedAndThumbResFiles(ts.testFileBase2, ts);
    if (rv != 0) {
        LOG_ERROR("createSharedAndThumbResFiles(%s):%d", ts.testFileBase2.c_str(), rv);
        return rv;
    }
    rv = createSharedAndThumbResFiles(ts.testFileBase3, ts);
    if (rv != 0) {
        LOG_ERROR("createSharedAndThumbResFiles(%s):%d", ts.testFileBase3.c_str(), rv);
        return rv;
    }
    rv = createSharedAndThumbResFiles(ts.testFileBaseTwoAccounts4, ts);
    if (rv != 0) {
        LOG_ERROR("createSharedAndThumbResFiles(%s):%d", ts.testFileBaseTwoAccounts4.c_str(), rv);
        return rv;
    }
    rv = createSharedAndThumbResFiles(ts.testFileBaseTwoAccounts5, ts);
    if (rv != 0) {
        LOG_ERROR("createSharedAndThumbResFiles(%s):%d", ts.testFileBaseTwoAccounts5.c_str(), rv);
        return rv;
    }

    return rv;
}

static int startCcdInstances(PhotoShareAutoTestState& ts)
{
    int rv = 0;
    {
        LOG_ALWAYS("\n\n== Start CloudPC CCD ==");
        setTargetMachine(CloudPC, ts);
        {
            const char *testStr = "StartCCD";
            const char *testArg[] = { testStr };
            rv = start_ccd(1, testArg);
            CHECK_AND_PRINT_RESULT(ts.testName.c_str(), testStr, rv);
        }
    }
    {
        LOG_ALWAYS("\n\n== Start Client CCD ==");
        setTargetMachine(Client2, ts);
        {
            const char *testStr = "StartCCD";
            const char *testArg[] = { testStr };
            rv = start_ccd(1, testArg);
            CHECK_AND_PRINT_RESULT(ts.testName.c_str(), testStr, rv);
        }

        QUERY_TARGET_OSVERSION(ts.clientPcId_2_osVersion, ts.testName.c_str(), rv);
    }

    {
        LOG_ALWAYS("\n\n== Start Client CCD ==");
        setTargetMachine(Client3, ts);
        {
            const char *testStr = "StartCCD";
            const char *testArg[] = { testStr };
            rv = start_ccd(1, testArg);
            CHECK_AND_PRINT_RESULT(ts.testName.c_str(), testStr, rv);
        }
    }
 exit:
    return rv;
}

static int endCcdInstances(bool using_remote_agent, PhotoShareAutoTestState& ts)
{
    int rv = 0;
    LOG_ALWAYS("\n\n== Freeing cloud PC(1) ==");
    setTargetMachine(CloudPC, ts);
    {
        {
            const char *testArg[] = { "StopCloudPC" };
            int temp_rc = stop_cloudpc(1, testArg);
            if (temp_rc != 0) {
                LOG_ERROR("stop_cloudpc:%d", temp_rc);
                rv = temp_rc;
            }
        }
        {
            const char *testArg[] = { "StopCCDSoft" };
            stop_ccd_soft(1, testArg);
        }
    }

    LOG_ALWAYS("\n\n== Freeing client(2) ==");
    setTargetMachine(Client2, ts);
    {
        {
            const char *testArg[] = { "StopClient" };
            int temp_rc = stop_client(1, testArg);
            if (temp_rc != 0) {
                LOG_ERROR("stop_client:%d",temp_rc);
                rv = temp_rc;
            }
        }
        if (isWindows(ts.clientPcId_2_osVersion) ||
            ts.clientPcId_2_osVersion.compare(OS_LINUX) == 0)
        {
            const char *testArg[] = { "StopCCDSoft" };
            int temp_rc = stop_ccd_soft(1, testArg);
            if (temp_rc != 0) {
                LOG_ERROR("stop_client:%d",temp_rc);
                rv = temp_rc;
            }
        }
    }

    LOG_ALWAYS("\n\n== Freeing client(3) ==");
    setTargetMachine(Client3, ts);
    {
        {
            const char *testArg[] = { "StopClient" };
            int temp_rc = stop_client(1, testArg);
            if (temp_rc != 0) {
                LOG_ERROR("stop_client:%d",temp_rc);
                rv = temp_rc;
            }
        }
        {
            const char *testArg[] = { "StopCCDSoft" };
            int temp_rc = stop_ccd_soft(1, testArg);
            if (temp_rc != 0) {
                LOG_ERROR("stop_client:%d",temp_rc);
                rv = temp_rc;
            }
        }
    }

    return rv;
}

static int loginCcdInstances(const std::string& user1, const std::string& password1,
                             const std::string& user2, const std::string& password2,
                             const std::string& user3, const std::string& password3,
                             PhotoShareAutoTestState& ts)
{
    int rv;

    {
        u64 userId;
        setTargetMachine(CloudPC, ts);
        START_CLOUDPC(user1.c_str(), password1.c_str(), ts.testName.c_str(), true, rv);
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("getUserIdBasic:%d", rv);
            goto exit;
        }
        rv = photo_share_enable(userId);
        if (rv != 0) {
            LOG_ERROR("photo_share_enable(%s, "FMTu64"):%d", user1.c_str(), userId, rv);
            goto exit;
        }
        ts.cloudPcUsername = user1;
        ts.cloudPcUserId = userId;
    }

    {
        u64 userId;
        setTargetMachine(Client2, ts);
        START_CLIENT(user2.c_str(), password2.c_str(), ts.testName.c_str(), true, rv);
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("getUserIdBasic:%d", rv);
            goto exit;
        }
        rv = photo_share_enable(userId);
        if (rv != 0) {
            LOG_ERROR("photo_share_enable(%s, "FMTu64"):%d", user1.c_str(), userId, rv);
            goto exit;
        }
        ts.clientPc_2_Username = user2;
        ts.clientPc_2_UserId = userId;
    }
    {
        u64 userId;
        setTargetMachine(Client3, ts);
        START_CLIENT(user3.c_str(), password3.c_str(), ts.testName.c_str(), true, rv);
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("getUserIdBasic:%d", rv);
            goto exit;
        }
        rv = photo_share_enable(userId);
        if (rv != 0) {
            LOG_ERROR("photo_share_enable(%s, "FMTu64"):%d", user1.c_str(), userId, rv);
            goto exit;
        }
        ts.clientPc_3_Username = user3;
        ts.clientPc_3_UserId = userId;
    }
 exit:  // The macros in this function jump here...
    return rv;
}

static int setupEventQueues(PhotoShareAutoTestState& ts)
{
    int rv;
    setTargetMachine(CloudPC, ts);
    rv = PhotoShareEventQueue_create(ts.cloudPcEventQueueHandle);
    if (rv != 0) {
        LOG_ERROR("PhotoShareEventQueue_create:%d", rv);
        return rv;
    }
    ts.cloudPcEventQueueHandleExists = true;

    setTargetMachine(Client2, ts);
    rv = PhotoShareEventQueue_create(ts.clientPcId_2_EventQueueHandle);
    if (rv != 0) {
        LOG_ERROR("PhotoShareEventQueue_create:%d", rv);
        return rv;
    }
    ts.clientPcId_2_EventQueueHandleExists = true;

    setTargetMachine(Client3, ts);
    rv = PhotoShareEventQueue_create(ts.clientPcId_3_EventQueueHandle);
    if (rv != 0) {
        LOG_ERROR("PhotoShareEventQueue_create:%d", rv);
        return rv;
    }
    ts.clientPcId_3_EventQueueHandleExists = true;

    return rv;
}

static int clearEventQueues(PhotoShareAutoTestState& ts)
{
    int rv;
    setTargetMachine(CloudPC, ts);
    rv = PhotoShareEventQueue_clear(ts.cloudPcEventQueueHandle);
    if (rv != 0) {
        LOG_ERROR("PhotoShareEventQueue_clear:%d", rv);
        return rv;
    }

    setTargetMachine(Client2, ts);
    rv = PhotoShareEventQueue_clear(ts.clientPcId_2_EventQueueHandle);
    if (rv != 0) {
        LOG_ERROR("PhotoShareEventQueue_clear:%d", rv);
        return rv;
    }

    setTargetMachine(Client3, ts);
    rv = PhotoShareEventQueue_clear(ts.clientPcId_3_EventQueueHandle);
    if (rv != 0) {
        LOG_ERROR("PhotoShareEventQueue_clear:%d", rv);
        return rv;
    }
    return rv;
}

static int teardownEventQueues(PhotoShareAutoTestState& ts)
{
    int rv = 0;
    if (ts.cloudPcEventQueueHandleExists) {
        setTargetMachine(CloudPC, ts);
        rv = PhotoShareEventQueue_destroy(ts.cloudPcEventQueueHandle);
        if (rv != 0) {
            LOG_ERROR("PhotoShareEventQueue_destroy:%d", rv);
        }
        ts.cloudPcEventQueueHandleExists = false;
    }
    if (ts.clientPcId_2_EventQueueHandleExists) {
        setTargetMachine(Client2, ts);
        rv = PhotoShareEventQueue_destroy(ts.clientPcId_2_EventQueueHandle);
        if (rv != 0) {
            LOG_ERROR("PhotoShareEventQueue_destroy:%d", rv);
        }
        ts.clientPcId_2_EventQueueHandleExists = false;
    }
    if (ts.clientPcId_3_EventQueueHandleExists) {
        setTargetMachine(Client3, ts);
        rv = PhotoShareEventQueue_destroy(ts.clientPcId_3_EventQueueHandle);
        if (rv != 0) {
            LOG_ERROR("PhotoShareEventQueue_destroy:%d", rv);
        }
        ts.clientPcId_3_EventQueueHandleExists = false;
    }
    return rv;
}

// returns 0 when no syncing/insync events found.
static int verifyNoSyncingAndInsyncEvents(ccd::SyncFeature_t feature,
                                          u64 queueHandle)
{
    int rv = 0;
    bool syncingEvent = false;
    bool insyncEvent = false;
    ccd::EventsDequeueOutput eventsOut;
    rv = PhotoShareEventQueue_get(queueHandle, /*OUT*/ eventsOut);
    if (rv != 0) {
        LOG_ERROR("PhotoShareEventQueue_get("FMTu64"):%d",
                  queueHandle, rv);
        return rv;
    }
    for (int evtIdx = 0; evtIdx < eventsOut.events_size(); ++evtIdx) {
        const ccd::CcdiEvent& event = eventsOut.events(evtIdx);
        if (event.has_sync_feature_status_change()) {
            const ccd::EventSyncFeatureStatusChange& featureChange =
                    event.sync_feature_status_change();
            if (featureChange.feature() == feature)
            {
                if (featureChange.status().status() ==
                        ccd::CCD_FEATURE_STATE_SYNCING)
                {
                    LOG_INFO("feature(%d) SyncingEvent.", (int)feature);
                    syncingEvent = true;
                } else if (featureChange.status().status() ==
                        ccd::CCD_FEATURE_STATE_IN_SYNC)
                {
                    LOG_INFO("feature(%d) InsyncEvent.", (int)feature);
                    insyncEvent = true;
                }
            }
        }
    }
    if (syncingEvent || insyncEvent) {
        LOG_ERROR("SyncEvent found (%d,%d)", syncingEvent, insyncEvent);
        return -1;
    }
    return 0;
}

static int waitForSyncingAndInsyncEvents(ccd::SyncFeature_t feature,
                                         u64 queueHandle,
                                         int maxWaitSecs)
{
    int rv = 0;
    bool syncingEvent = false;
    bool insyncEvent = false;

    for (int i = 0; i <= maxWaitSecs; ++i) {
        if (i != 0) {  // No need to wait for first iteration
            VPLThread_Sleep(VPLTime_FromSec(1));
        }
        ccd::EventsDequeueOutput eventsOut;
        rv = PhotoShareEventQueue_get(queueHandle, /*OUT*/ eventsOut);
        if (rv != 0) {
            LOG_ERROR("PhotoShareEventQueue_get("FMTu64"):%d",
                      queueHandle, rv);
            return rv;
        }
        for (int evtIdx = 0; evtIdx < eventsOut.events_size(); ++evtIdx) {
            const ccd::CcdiEvent& event = eventsOut.events(evtIdx);
            if (event.has_sync_feature_status_change()) {
                const ccd::EventSyncFeatureStatusChange& featureChange =
                        event.sync_feature_status_change();
                if (featureChange.feature() == feature)
                {
                    if (featureChange.status().status() ==
                            ccd::CCD_FEATURE_STATE_SYNCING)
                    {
                        LOG_INFO("feature(%d) SyncingEvent.", (int)feature);
                        syncingEvent = true;
                    } else if (featureChange.status().status() ==
                            ccd::CCD_FEATURE_STATE_IN_SYNC)
                    {
                        LOG_INFO("feature(%d) InsyncEvent.", (int)feature);
                        insyncEvent = true;
                    }
                }
            }
        }
        if (syncingEvent && insyncEvent) {
            break;
        }
    }
    if (!syncingEvent) {
        LOG_ERROR("feature(%d, "FMTu64"): SyncingEvent never received",
                  (int)feature, queueHandle);
        rv = -1;
    }
    if (!insyncEvent) {
        LOG_ERROR("feature(%d, "FMTu64"): InsyncEvent never received",
                  (int)feature, queueHandle);
        rv = -1;
    }
    return rv;
}

static int doStoreAndSharePhotoTestCloudPcToClient2(PhotoShareAutoTestState& ts,
                                                    const std::string& testFileBase,
                                                    const std::string& opaqueMetadata)
{
    int rv;
    u64 compId;
    std::string name;

    setTargetMachine(CloudPC, ts);
    std::string absTargetSharedFile;
    std::string absTargetPreviewFile;
    {   // Push content file and thumbnail file to store.
        // Set absTargetSharedFile and absTargetPreviewFile to correct paths
        TargetDevice* cloudDevice = getTargetDevice();
        if (cloudDevice == NULL) {
            LOG_ERROR("getTargetDevice");
            return -1;
        }
        ON_BLOCK_EXIT(deleteObj<TargetDevice>, cloudDevice);

        std::string targetWorkDir;
        rv = cloudDevice->getDxRemoteRoot(/*OUT*/ targetWorkDir);
        if (rv != 0) {
            LOG_ERROR("cloudDevice->getDxRemoteRoot():%d", rv);
            return rv;
        }

        std::string dirSeparator;
        rv = cloudDevice->getDirectorySeparator(/*OUT*/dirSeparator);
        if (rv != 0) {
            LOG_ERROR("cloudDevice->getDirectorySeparator():%d", rv);
            return rv;
        }

        std::string targetSharedFilename = std::string("pushed_") +
                                           getSharedResolutionFilename(testFileBase);
        absTargetSharedFile = targetWorkDir + dirSeparator + targetSharedFilename;
        rv = cloudDevice->pushFile(getSharedResolutionAbsPath(ts, testFileBase),
                                   absTargetSharedFile);
        if (rv != 0) {
            LOG_ERROR("cloudDevice->pushFile(%s,%s):%d",
                      getSharedResolutionAbsPath(ts, testFileBase).c_str(),
                      targetSharedFilename.c_str(), rv);
            return rv;
        }


        std::string targetPreviewFilename = std::string("pushed_") +
                                            getThumbResolutionFilename(testFileBase);
        absTargetPreviewFile = targetWorkDir + dirSeparator + targetPreviewFilename;
        rv = cloudDevice->pushFile(getThumbResolutionAbsPath(ts, testFileBase),
                                   absTargetPreviewFile);
        if (rv != 0) {
            LOG_ERROR("cloudDevice->pushFile(%s,%s):%d",
                      getSharedResolutionAbsPath(ts, testFileBase).c_str(),
                      targetPreviewFilename.c_str(), rv);
            return rv;
        }

    }

    rv = clearEventQueues(ts);  // Could change target machine.
    if (rv != 0) {
        LOG_ERROR("clearEventQueues:%d", rv);
        return rv;
    }

    setTargetMachine(CloudPC, ts);
    rv = sharePhoto_store(ts.cloudPcUserId,
                          absTargetSharedFile,
                          opaqueMetadata,
                          absTargetPreviewFile,
                          /*OUT*/ compId,
                          /*OUT*/ name);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_store(%s):%d", testFileBase.c_str(), rv);
        return rv;
    }

    rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                       ts.cloudPcEventQueueHandle,
                                       ts.WAIT_FOR_EVENTS_SEC);
    if (rv != 0) {
        LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
        return rv;
    }
    rv = clearEventQueues(ts);  // Will switch target machine.
    if (rv != 0) {
        LOG_ERROR("clearEventQueues:%d", rv);
        return rv;
    }

    setTargetMachine(CloudPC, ts);
    std::vector<std::string> accounts;
    accounts.push_back(ts.clientPc_2_Username);
    // Share with account 2
    rv = sharePhoto_share(ts.cloudPcUserId,
                          compId,
                          name,
                          accounts);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_share(["FMTu64",%s,"FMTu64",%s] --> ["FMTu64",%s]):%d",
                  ts.cloudPcUserId, ts.cloudPcUsername.c_str(), compId, name.c_str(),
                  ts.clientPc_2_UserId, ts.clientPc_2_Username.c_str(), rv);
        return rv;
    }
    LOG_ALWAYS("sharePhoto_share(["FMTu64",%s,"FMTu64",%s] --> ["FMTu64",%s])",
               ts.cloudPcUserId, ts.cloudPcUsername.c_str(), compId, name.c_str(),
               ts.clientPc_2_UserId, ts.clientPc_2_Username.c_str());
    return rv;
}

// Return true if the URL download and reference file are equal
static bool isUrlFileEqualToReference(
        const PhotoShareAutoTestState& ts,
        const std::string& url,
        const std::string& testFileBase,
        const std::string& dlFilePrefix, // Unique prefix to identify downloaded file
        const std::string& dlFileSuffix, // Unique suffix to identify downloaded file
        bool trueWhenPreviewAndFalseWhenContent)
{
    bool toReturn = false;
    int temp_rc;  // Even if the files are different, we can continue on.
    HttpAgent *agent = getHttpAgent();
    if (agent == NULL) {
        LOG_ERROR("HttpAgent");
        return false;
    }
    ON_BLOCK_EXIT(deleteObj<HttpAgent>, agent);

    TargetDevice* target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("getTargetDevice");
        return false;
    }
    ON_BLOCK_EXIT(deleteObj<TargetDevice>, target);

    std::string separator;
    temp_rc = target->getDirectorySeparator(/*OUT*/ separator);
    if (temp_rc != 0) {
        LOG_ERROR("target->getDirectorySeparator():%d", temp_rc);
        return false;
    }

    std::string remoteRoot;
    temp_rc = target->getDxRemoteRoot(/*OUT*/ remoteRoot);
    if (temp_rc != 0) {
        LOG_ERROR("target->getDxRemoteRoot():%d", temp_rc);
        return false;
    }

    std::string filename = dlFilePrefix+testFileBase+dlFileSuffix;
    std::string targetFilePath = remoteRoot + separator + filename;

    // Download from URL to the filename.
    temp_rc = agent->get_extended(url, 0, 0, NULL, 0, targetFilePath.c_str());
    if (temp_rc != 0) {
        LOG_ERROR("httpGet(%s)->%s:%d", url.c_str(), targetFilePath.c_str(), temp_rc);
    } else {
        LOG_ALWAYS("Download(%s)->target(%s) Successful!",
                   url.c_str(), targetFilePath.c_str());
        std::string localAbsPath;
        Util_appendToAbsPath(ts.testDir, filename, /*OUT*/ localAbsPath);
        temp_rc = target->pullFile(targetFilePath, localAbsPath);
        if (temp_rc != 0) {
            LOG_ERROR("pullFile(%s->%s):%d",
                      targetFilePath.c_str(), localAbsPath.c_str(), temp_rc);
        } else {
            std::string absPathRefFile;
            if (trueWhenPreviewAndFalseWhenContent) {
                absPathRefFile = getThumbResolutionAbsPath(ts, testFileBase);
            } else {
                absPathRefFile = getSharedResolutionAbsPath(ts, testFileBase);
            }
            temp_rc = file_compare(localAbsPath.c_str(), absPathRefFile.c_str());
            if (temp_rc == 0) {
                toReturn = true;
            } else {
                LOG_ERROR("file_compare(%s,%s):%d",
                          localAbsPath.c_str(), absPathRefFile.c_str(), temp_rc);
            }
        }
    }
    return toReturn;
}

#define ERR_GET_ENTRY_NOT_FOUND -700  // Just some made up error code.
static int getEntryFromQuery(u64 userId,
                             ccd::SyncFeature_t feature,
                             const std::string& opaqueMetadata,  // Used as the key to find the entry
                             ccd::SharedFilesQueryObject& queryObject_out)
{
    queryObject_out.Clear();
    int rv = 0;
    ccd::SharedFilesQueryOutput queryResponse;
    rv = sharePhoto_dumpFilesQuery(userId,
                                   feature,
                                   true,
                                   /*OUT*/ queryResponse);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_dumpFilesQuery("FMTu64"):%d",
                  userId, rv);
        return rv;
    }

    for (int qIndex=0; qIndex < queryResponse.query_objects_size(); ++qIndex) {
        const ccd::SharedFilesQueryObject& sfObject = queryResponse.query_objects(qIndex);
        if (sfObject.opaque_metadata() == opaqueMetadata) {
            queryObject_out = sfObject;
            return 0;
        }
    }
    return ERR_GET_ENTRY_NOT_FOUND;
}

static int verifyStoreAndSharePhotoTestCloudPcToClient2(PhotoShareAutoTestState& ts,
                                                        const std::string& testFileBase,
                                                        const std::string& opaqueMetadata)
{

    int rv = 0;
    {   // Check Shared By Me dataset for Account 1
        setTargetMachine(CloudPC, ts);
        ccd::SharedFilesQueryObject sfObject;
        bool sharedAccountFound = false;

        for (int retry=0; retry<3; ++retry)
        {   // Make the autotest more forgiving for extra events.  It's possible
            // that an extra event or two is generated because PhotoShare_Store
            // is 2 steps, postFileMetadata and add Preview (These events were
            // supposedly consumed in the doStoreAndSharePhotoTestCloudPcToClient2 step,
            // but only 1 event is consumed because the events may or may not be
            // aggregated.
            // The third event would be due to the actual share, which is the one
            // meant to be consumed below, but allow a few retries in case previous
            // events have not been consumed.
            rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                               ts.cloudPcEventQueueHandle,
                                               ts.WAIT_FOR_EVENTS_SEC);
            if (rv != 0) {
                LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
                return rv;
            }


            rv = getEntryFromQuery(ts.cloudPcUserId,
                                   ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                   opaqueMetadata,
                                   /*OUT*/ sfObject);
            if (rv != 0) {
                LOG_ERROR("SBM record(%s) not found:%d ", opaqueMetadata.c_str(), rv);
                return rv;
            }


            for (int saIndex = 0; saIndex < sfObject.recipient_list_size(); ++saIndex) {
                if (sfObject.recipient_list(saIndex) == ts.clientPc_2_Username) {
                    sharedAccountFound = true;
                }
            }
            if (sharedAccountFound) {
                break;
            } else { // !sharedAccountFound
                LOG_WARN("SBM record(%s) does not have sharedAcct(%s), will perhaps retry(%d), "
                         "extra events are allowed to be sent.",
                         opaqueMetadata.c_str(), ts.clientPc_2_Username.c_str(), retry);
            }
        }
        if (!sharedAccountFound) {
            LOG_ERROR("SBM record(%s) does not have sharedAcct(%s)",
                      opaqueMetadata.c_str(), ts.clientPc_2_Username.c_str());
            return -1;
        }

        LOG_ALWAYS("testBaseSbm(%s) previewUrl(%s), contentUrl(%s)",
                   testFileBase.c_str(), sfObject.preview_url().c_str(),
                   sfObject.content_url().c_str());

        {   // Check Content url equal to ref
            bool contentEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.content_url(),
                                                          testFileBase,
                                                          "SBM_dev1_",
                                                          ".jpg",
                                                          false);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verifySbmContentDev1_%s",
                       contentEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
        {   // Check Preview url equal to ref
            bool previewEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.preview_url(),
                                                          testFileBase,
                                                          "SBM_dev1_",
                                                          "_thumb.jpg",
                                                          true);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verifySbmPreviewDev1_%s",
                       previewEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
    }

    {   // Check Shared With Me dataset for Account 2
        setTargetMachine(Client2, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_2_EventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            return rv;
        }

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_2_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SWM record(%s) not found:%d ", opaqueMetadata.c_str(), rv);
            return rv;
        }

        LOG_ALWAYS("testBaseSwm(%s) previewUrl(%s), contentUrl(%s)",
                   testFileBase.c_str(), sfObject.preview_url().c_str(),
                   sfObject.content_url().c_str());
        {   // Check Content url equal to ref
            bool contentEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.content_url(),
                                                          testFileBase,
                                                          "SWM_dev2_",
                                                          ".jpg",
                                                          false);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verifySwmContentDev2_%s",
                       contentEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
        {   // Check Preview url equal to ref
            bool previewEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.preview_url(),
                                                          testFileBase,
                                                          "SWM_dev2_",
                                                          "_thumb.jpg",
                                                          true);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verifySwmPreviewDev2_%s",
                       previewEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
    }
    return rv;
}

static int doShareExistingPhotoCloudPcToClient3(PhotoShareAutoTestState& ts,
                                                const std::string& testFileBase,
                                                const std::string& opaqueMetadata)
{
    int rv;

    rv = clearEventQueues(ts);  // Could change target machine.
    if (rv != 0) {
        LOG_ERROR("clearEventQueues:%d", rv);
        return rv;
    }

    setTargetMachine(CloudPC, ts);

    ccd::SharedFilesQueryObject sfObject;
    rv = getEntryFromQuery(ts.cloudPcUserId,
                           ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                           opaqueMetadata,
                           /*OUT*/ sfObject);
    if (rv != 0) {
        LOG_ERROR("SBM record(%s) not found:%d ", opaqueMetadata.c_str(), rv);
        return rv;
    }

    std::vector<std::string> accounts;
    accounts.push_back(ts.clientPc_3_Username);
    // Share with account 2
    rv = sharePhoto_share(ts.cloudPcUserId,
                          sfObject.comp_id(),
                          sfObject.name(),
                          accounts);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_share(["FMTu64",%s,"FMTu64",%s] --> ["FMTu64",%s]):%d",
                  ts.cloudPcUserId, ts.cloudPcUsername.c_str(),
                  sfObject.comp_id(), sfObject.name().c_str(),
                  ts.clientPc_3_UserId, ts.clientPc_3_Username.c_str(), rv);
        return rv;
    }
    LOG_ALWAYS("sharePhoto_share(["FMTu64",%s,"FMTu64",%s] --> ["FMTu64",%s])",
               ts.cloudPcUserId, ts.cloudPcUsername.c_str(),
               sfObject.comp_id(), sfObject.name().c_str(),
               ts.clientPc_3_UserId, ts.clientPc_3_Username.c_str());

    return 0;
}

static int verifyShareExistingPhotoCloudPcToClient3(PhotoShareAutoTestState& ts,
                                                    const std::string& testFileBase,
                                                    const std::string& opaqueMetadata)
{
    int rv = 0;
    {   // Check Shared By Me dataset for Account 1
        setTargetMachine(CloudPC, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                           ts.cloudPcEventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            return rv;
        }

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.cloudPcUserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                               opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SBM record(%s) not found:%d ", opaqueMetadata.c_str(), rv);
            return rv;
        }

        bool sharedAccountClient2Found = false;
        bool sharedAccountClient3Found = false;
        for (int saIndex = 0; saIndex < sfObject.recipient_list_size(); ++saIndex) {
            if (sfObject.recipient_list(saIndex) == ts.clientPc_2_Username) {
                sharedAccountClient2Found = true;
            } else
            if (sfObject.recipient_list(saIndex) == ts.clientPc_3_Username) {
                sharedAccountClient3Found = true;
            }
        }
        if (!sharedAccountClient2Found) {
            LOG_ERROR("SBM record(%s) does not have sharedAcct(%s)",
                      opaqueMetadata.c_str(), ts.clientPc_2_Username.c_str());
            return -1;
        }
        if (!sharedAccountClient3Found) {
            LOG_ERROR("SBM record(%s) does not have sharedAcct(%s)",
                      opaqueMetadata.c_str(), ts.clientPc_3_Username.c_str());
            return -1;
        }

        LOG_ALWAYS("testBaseSbm(%s) previewUrl(%s), contentUrl(%s)",
                   testFileBase.c_str(), sfObject.preview_url().c_str(),
                   sfObject.content_url().c_str());

        {   // Check Content url equal to ref
            bool contentEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.content_url(),
                                                          testFileBase,
                                                          "2ndSBM_dev1_",
                                                          ".jpg",
                                                          false);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verify2ndSbmContentDev1_%s",
                       contentEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
        {   // Check Preview url equal to ref
            bool previewEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.preview_url(),
                                                          testFileBase,
                                                          "2ndSBM_dev1_",
                                                          "_thumb.jpg",
                                                          true);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verify2ndSbmPreviewDev1_%s",
                       previewEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
    }

    {   // Check Shared With Me dataset for Account 2
        setTargetMachine(Client2, ts);

        // No events are sent to previously shared account.  Just re-verifying
        // nothing has changed for previous shared Account2 when photo shared to Account3.
        rv = verifyNoSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                            ts.clientPcId_2_EventQueueHandle);
        if (rv != 0) {
            LOG_ERROR("FailedEventRetrieval or unexpected events (%s):%d, Noting and Continuing"
                      "\nTC_RESULT=FAIL ;;; TC_NAME=UnexpectedEvents_%s",
                      ts.clientPc_2_Username.c_str(), rv, opaqueMetadata.c_str());
        }

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_2_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SWM record(%s) not found:%d ", opaqueMetadata.c_str(), rv);
            return rv;
        }

        LOG_ALWAYS("testBaseSwm(%s) previewUrl(%s), contentUrl(%s)",
                   testFileBase.c_str(), sfObject.preview_url().c_str(),
                   sfObject.content_url().c_str());
        {   // Check Content url equal to ref
            bool contentEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.content_url(),
                                                          testFileBase,
                                                          "2ndSWM_dev2_",
                                                          ".jpg",
                                                          false);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verify2ndSwmContentDev2_%s",
                       contentEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
        {   // Check Preview url equal to ref
            bool previewEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.preview_url(),
                                                          testFileBase,
                                                          "2ndSWM_dev2_",
                                                          "_thumb.jpg",
                                                          true);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verify2ndSwmPreviewDev2_%s",
                       previewEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
    }

    {   // Check Shared With Me dataset for Account 3
        setTargetMachine(Client3, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_3_EventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            return rv;
        }

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_3_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SWM record(%s) not found:%d ", opaqueMetadata.c_str(), rv);
            return rv;
        }

        LOG_ALWAYS("testBaseSwm(%s) previewUrl(%s), contentUrl(%s)",
                   testFileBase.c_str(), sfObject.preview_url().c_str(),
                   sfObject.content_url().c_str());
        {   // Check Content url equal to ref
            bool contentEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.content_url(),
                                                          testFileBase,
                                                          "2ndSWM_dev3_",
                                                          ".jpg",
                                                          false);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verify2ndSwmContentDev3_%s",
                       contentEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
        {   // Check Preview url equal to ref
            bool previewEqRef = isUrlFileEqualToReference(ts,
                                                          sfObject.preview_url(),
                                                          testFileBase,
                                                          "2ndSWM_dev3_",
                                                          "_thumb.jpg",
                                                          true);
            LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verify2ndSwmPreviewDev3_%s",
                       previewEqRef?"PASS":"FAIL",
                       testFileBase.c_str());
        }
    }
    return 0;
}

// Tied together with performAndVerifyBasicUnshareTests, please add new tests
// in advanced section.
static int performAndVerifyBasicShareTests(PhotoShareAutoTestState& testState)
{
    // Steps referenced in this function are outline here:
    // http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Share_Add_Photo_Test

    int rv;
    {   // testFileBase1  Steps (3) and (4)
        rv = doStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                      testState.testFileBase1,
                                                      testState.testFileBase1_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("storeAndSharePhotoTestCloudPcToClient2(%s):%d",
                      testState.testFileBase1.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=doShare_%s", testState.testFileBase1.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=doShare_%s", testState.testFileBase1.c_str());

        rv = verifyStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                          testState.testFileBase1,
                                                          testState.testFileBase1_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("verifyStoreAndSharePhotoTestCloudPcToClient2(%s):%d",
                     testState.testFileBase1.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=checkDoShare_%s", testState.testFileBase1.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=checkDoShare_%s", testState.testFileBase1.c_str());
    }

    {   // testFileBase2  Step (5), 1st repeat
        rv = doStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                      testState.testFileBase2,
                                                      testState.testFileBase2_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("storeAndSharePhotoTestCloudPcToClient2(%s):%d",
                      testState.testFileBase2.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=doShare_%s", testState.testFileBase2.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=doShare_%s", testState.testFileBase2.c_str());

        rv = verifyStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                          testState.testFileBase2,
                                                          testState.testFileBase2_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("verifyStoreAndSharePhotoTestCloudPcToClient2(%s):%d",
                     testState.testFileBase2.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=checkDoShare_%s", testState.testFileBase2.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=checkDoShare_%s", testState.testFileBase2.c_str());
    }

    {   // testFileBase3  Step (5), 2nd repeat
        rv = doStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                      testState.testFileBase3,
                                                      testState.testFileBase3_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("storeAndSharePhotoTestCloudPcToClient2(%s):%d",
                      testState.testFileBase3.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=doShare_%s", testState.testFileBase3.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=doShare_%s", testState.testFileBase3.c_str());

        rv = verifyStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                          testState.testFileBase3,
                                                          testState.testFileBase3_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("verifyStoreAndSharePhotoTestCloudPcToClient2(%s):%d",
                     testState.testFileBase3.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=checkDoShare_%s", testState.testFileBase3.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=checkDoShare_%s", testState.testFileBase3.c_str());
    }

    {   // testFileBaseTwoAccounts4  Step (5), 3rd repeat
        rv = doStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                      testState.testFileBaseTwoAccounts4,
                                                      testState.testFileBaseTwoAccounts4_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("storeAndSharePhotoTestCloudPcToClient2(%s):%d",
                      testState.testFileBaseTwoAccounts4.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=doShare_%s", testState.testFileBaseTwoAccounts4.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=doShare_%s", testState.testFileBaseTwoAccounts4.c_str());

        rv = verifyStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                          testState.testFileBaseTwoAccounts4,
                                                          testState.testFileBaseTwoAccounts4_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("verifyStoreAndSharePhotoTestCloudPcToClient2(%s):%d",
                     testState.testFileBaseTwoAccounts4.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=checkDoShare_%s", testState.testFileBaseTwoAccounts4.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=checkDoShare_%s", testState.testFileBaseTwoAccounts4.c_str());

        // Do sharing to the second account (Client3), Step (6)
        rv = doShareExistingPhotoCloudPcToClient3(testState,
                                                  testState.testFileBaseTwoAccounts4,
                                                  testState.testFileBaseTwoAccounts4_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("doShareExistingPhotoCloudPcToClient3(%s):%d",
                      testState.testFileBaseTwoAccounts4.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=do2ndShare_%s", testState.testFileBaseTwoAccounts4.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=do2ndShare_%s", testState.testFileBaseTwoAccounts4.c_str());

        rv = verifyShareExistingPhotoCloudPcToClient3(testState,
                                                      testState.testFileBaseTwoAccounts4,
                                                      testState.testFileBaseTwoAccounts4_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("verifyShareExistingPhotoCloudPcToClient3(%s):%d",
                      testState.testFileBaseTwoAccounts4.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=checkDo2ndShare_%s", testState.testFileBaseTwoAccounts4.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=checkDo2ndShare_%s", testState.testFileBaseTwoAccounts4.c_str());
    }

    {   // testFileBaseTwoAccounts5  Step (5), 4th repeat
        rv = doStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                    testState.testFileBaseTwoAccounts5,
                                                    testState.testFileBaseTwoAccounts5_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("storeAndSharePhotoTestCloudPcToClient2(%s):%d",
                      testState.testFileBaseTwoAccounts5.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=doShare_%s", testState.testFileBaseTwoAccounts5.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=doShare_%s", testState.testFileBaseTwoAccounts5.c_str());

        rv = verifyStoreAndSharePhotoTestCloudPcToClient2(testState,
                                                          testState.testFileBaseTwoAccounts5,
                                                          testState.testFileBaseTwoAccounts5_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("verifyStoreAndSharePhotoTestCloudPcToClient2(%s):%d",
                     testState.testFileBaseTwoAccounts5.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=checkDoShare_%s", testState.testFileBaseTwoAccounts5.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=checkDoShare_%s", testState.testFileBaseTwoAccounts5.c_str());

        // Do sharing to the second account (Client3), Step (6)
        rv = doShareExistingPhotoCloudPcToClient3(testState,
                                                  testState.testFileBaseTwoAccounts5,
                                                  testState.testFileBaseTwoAccounts5_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("doShareExistingPhotoCloudPcToClient3(%s):%d",
                      testState.testFileBaseTwoAccounts5.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=do2ndShare_%s", testState.testFileBaseTwoAccounts5.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=do2ndShare_%s", testState.testFileBaseTwoAccounts5.c_str());

        rv = verifyShareExistingPhotoCloudPcToClient3(testState,
                                                      testState.testFileBaseTwoAccounts5,
                                                      testState.testFileBaseTwoAccounts5_opaqueMetadata);
        if (rv != 0) {
            LOG_ERROR("verifyShareExistingPhotoCloudPcToClient3(%s):%d",
                      testState.testFileBaseTwoAccounts5.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=checkDo2ndShare_%s", testState.testFileBaseTwoAccounts5.c_str());
            goto end;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=checkDo2ndShare_%s", testState.testFileBaseTwoAccounts5.c_str());
    }
 end:
    return rv;
}

static int performUnshare(u64 userId,
                          const std::string& opaqueMetadata,
                          const std::string& accountToUnshare)
{   // Assumes the proper target client has already been set.
    int rv;
    ccd::SharedFilesQueryObject sfObject;
    rv = getEntryFromQuery(userId,
                           ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                           opaqueMetadata,
                           /*OUT*/ sfObject);
    if (rv != 0) {
        LOG_ERROR("SBM record(%s) not found:%d ", opaqueMetadata.c_str(), rv);
        return rv;
    }
    std::vector<std::string> toUnshare;
    toUnshare.push_back(accountToUnshare);
    rv = sharePhoto_unshare(userId,
                            sfObject.comp_id(),
                            sfObject.name(),
                            toUnshare);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_unshare("FMTu64", compId:"FMTu64",%s, acct:%s):%d",
                  userId, sfObject.comp_id(), sfObject.name().c_str(),
                  accountToUnshare.c_str(), rv);
        return rv;
    }
    LOG_ALWAYS("Success sharePhoto_unshare("FMTu64", compId:"FMTu64",%s, acct:%s)",
               userId, sfObject.comp_id(), sfObject.name().c_str(),
               accountToUnshare.c_str());
    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Step (3)
static int doAcct1UnsharePhoto3WithAcct2(PhotoShareAutoTestState& ts)
{
    int rv;
    rv = clearEventQueues(ts);  // Could change target machine.
    if (rv != 0) {
        LOG_ERROR("clearEventQueues:%d", rv);
        return rv;
    }

    setTargetMachine(CloudPC, ts);
    rv = performUnshare(ts.cloudPcUserId,
                        ts.testFileBase3_opaqueMetadata,
                        ts.clientPc_2_Username);
    if (rv != 0) {
        LOG_ERROR("performUnshare("FMTu64",%s,%s):%d",
                  ts.cloudPcUserId, ts.testFileBase3_opaqueMetadata.c_str(),
                  ts.clientPc_2_Username.c_str(), rv);
        return rv;
    }
    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Verify Step (3)
static int verifyDoAcct1UnsharePhoto3WithAcct2(PhotoShareAutoTestState& ts)
{
    int rv = 0;
    {   // Check Shared By Me dataset for Account 1
        setTargetMachine(CloudPC, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                           ts.cloudPcEventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_1");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_1");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.cloudPcUserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                               ts.testFileBase3_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv == 0) {
            LOG_ERROR("SBM record(%s) found! Expected to be unshared ",
                      ts.testFileBase3_opaqueMetadata.c_str());
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_2");
            return rv;
        } else if (rv != ERR_GET_ENTRY_NOT_FOUND) {
            LOG_ERROR("getEntryFromQuery("FMTu64"):%d", ts.cloudPcUserId, rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_2");
            return rv;
        } else {  // Success!
            ASSERT(rv == ERR_GET_ENTRY_NOT_FOUND);
            LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_2");
            rv = 0;
        }
    }

    {   // Check Shared With Me dataset for Account 2
        setTargetMachine(Client2, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_2_EventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_3");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_3");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_2_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               ts.testFileBase3_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv == 0) {
            LOG_ERROR("SBM record(%s) found! Expected to be unshared ",
                      ts.testFileBase3_opaqueMetadata.c_str());
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_4");
            return rv;
        } else if (rv != ERR_GET_ENTRY_NOT_FOUND) {
            LOG_ERROR("getEntryFromQuery("FMTu64"):%d", ts.clientPc_2_UserId, rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_4");
            return rv;
        } else {  // Success!
            ASSERT(rv == ERR_GET_ENTRY_NOT_FOUND);
            LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_4");
            rv = 0;
        }
    }

    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Step (4)
static int doAcct1UnsharePhoto4WithAcct2(PhotoShareAutoTestState& ts)
{
    int rv;
    rv = clearEventQueues(ts);  // Could change target machine.
    if (rv != 0) {
        LOG_ERROR("clearEventQueues:%d", rv);
        return rv;
    }

    setTargetMachine(CloudPC, ts);
    rv = performUnshare(ts.cloudPcUserId,
                        ts.testFileBaseTwoAccounts4_opaqueMetadata,
                        ts.clientPc_2_Username);
    if (rv != 0) {
        LOG_ERROR("performUnshare("FMTu64",%s,%s):%d",
                  ts.cloudPcUserId, ts.testFileBaseTwoAccounts4_opaqueMetadata.c_str(),
                  ts.clientPc_2_Username.c_str(), rv);
        return rv;
    }
    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Verify Step (4)
static int verifyDoAcct1UnsharePhoto4WithAcct2(PhotoShareAutoTestState& ts)
{
    int rv = 0;
    {   // Check Shared By Me dataset for Account 1
        setTargetMachine(CloudPC, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                           ts.cloudPcEventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_5");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_5");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.cloudPcUserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                               ts.testFileBaseTwoAccounts4_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SBM record(%s):%d not found, Not expected since still sharing with %s",
                      ts.testFileBaseTwoAccounts4_opaqueMetadata.c_str(),
                      rv, ts.clientPc_3_Username.c_str());
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_6");
            return rv;
        } else {
            LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_6");
        }
    }

    {   // Check Shared With Me dataset for Account 2, should no longer exist
        setTargetMachine(Client2, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_2_EventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_7");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_7");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_2_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               ts.testFileBaseTwoAccounts4_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv == 0) {
            LOG_ERROR("SBM record(%s) found! Expected to be unshared to %s",
                      ts.testFileBaseTwoAccounts4_opaqueMetadata.c_str(),
                      ts.clientPc_2_Username.c_str());
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_8");
            return rv;
        } else if (rv != ERR_GET_ENTRY_NOT_FOUND) {
            LOG_ERROR("getEntryFromQuery("FMTu64"):%d", ts.clientPc_2_UserId, rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_8");
            return rv;
        } else {  // Success!
            ASSERT(rv == ERR_GET_ENTRY_NOT_FOUND);
            LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_8");
            rv = 0;
        }
    }

    {   // Check Shared With Me dataset for Account 3, should be still shared
        setTargetMachine(Client3, ts);
        rv = verifyNoSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_3_EventQueueHandle);
        if (rv != 0) {
            LOG_ERROR("verifyNoSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_9");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_9");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_3_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               ts.testFileBaseTwoAccounts4_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SBM record(%s,%s):%d not found!",
                      ts.testFileBaseTwoAccounts4_opaqueMetadata.c_str(),
                      ts.clientPc_2_Username.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_10");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_10");
    }
    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Step (5)
static int doAcct1UnsharePhoto4WithAcct3(PhotoShareAutoTestState& ts)
{
    int rv;
    rv = clearEventQueues(ts);  // Could change target machine.
    if (rv != 0) {
        LOG_ERROR("clearEventQueues:%d", rv);
        return rv;
    }

    setTargetMachine(CloudPC, ts);
    rv = performUnshare(ts.cloudPcUserId,
                        ts.testFileBaseTwoAccounts4_opaqueMetadata,
                        ts.clientPc_3_Username);
    if (rv != 0) {
        LOG_ERROR("performUnshare("FMTu64",%s,%s):%d",
                  ts.cloudPcUserId, ts.testFileBaseTwoAccounts4_opaqueMetadata.c_str(),
                  ts.clientPc_3_Username.c_str(), rv);
        return rv;
    }
    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Verify Step (5)
static int verifyDoAcct1UnsharePhoto4WithAcct3(PhotoShareAutoTestState& ts)
{
    int rv = 0;
    {   // Check Shared By Me dataset for Account 1
        setTargetMachine(CloudPC, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                           ts.cloudPcEventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_11");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_11");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.cloudPcUserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                               ts.testFileBaseTwoAccounts4_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv == 0) {
            LOG_ERROR("SBM record(%s) found! Expected to be unshared ",
                      ts.testFileBaseTwoAccounts4_opaqueMetadata.c_str());
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_12");
            return rv;
        } else if (rv != ERR_GET_ENTRY_NOT_FOUND) {
            LOG_ERROR("getEntryFromQuery("FMTu64"):%d", ts.cloudPcUserId, rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_12");
            return rv;
        } else {  // Success!
            ASSERT(rv == ERR_GET_ENTRY_NOT_FOUND);
            LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_12");
            rv = 0;
        }
    }

    {   // Check Shared With Me dataset for Account 3
        setTargetMachine(Client3, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_3_EventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_13");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_13");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_3_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               ts.testFileBaseTwoAccounts4_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv == 0) {
            LOG_ERROR("SBM record(%s) found! Expected to be unshared ",
                      ts.testFileBaseTwoAccounts4_opaqueMetadata.c_str());
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_14");
            return rv;
        } else if (rv != ERR_GET_ENTRY_NOT_FOUND) {
            LOG_ERROR("getEntryFromQuery("FMTu64"):%d", ts.clientPc_3_UserId, rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_14");
            return rv;
        } else {  // Success!
            ASSERT(rv == ERR_GET_ENTRY_NOT_FOUND);
            LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_14");
            rv = 0;
        }
    }

    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Step (6)
static int doAcct2DeleteSwmPhoto5(PhotoShareAutoTestState& ts)
{
    int rv;
    rv = clearEventQueues(ts);  // Could change target machine.
    if (rv != 0) {
        LOG_ERROR("clearEventQueues:%d", rv);
        return rv;
    }

    setTargetMachine(Client2, ts);

    ccd::SharedFilesQueryObject sfObject;
    rv = getEntryFromQuery(ts.clientPc_2_UserId,
                           ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                           ts.testFileBaseTwoAccounts5_opaqueMetadata,
                           /*OUT*/ sfObject);
    if (rv != 0) {
        LOG_ERROR("SWM record(%s) not found:%d ",
                  ts.testFileBaseTwoAccounts5_opaqueMetadata.c_str(), rv);
        return rv;
    }

    rv = sharePhoto_deleteSharedWithMe(ts.clientPc_2_UserId,
                                       sfObject.comp_id(),
                                       sfObject.name());
    if (rv != 0) {
        LOG_ERROR("sharePhoto_deleteSharedWithMe("FMTu64", compId:"FMTu64",%s):%d",
                  ts.clientPc_2_UserId, sfObject.comp_id(), sfObject.name().c_str(),
                  rv);
        return rv;
    }
    LOG_ALWAYS("Success sharePhoto_deleteSharedWithMe("FMTu64", compId:"FMTu64",%s)",
               ts.clientPc_2_UserId,
               sfObject.comp_id(), sfObject.name().c_str());
    return rv;
}

// http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test
// Verify Step (6)
static int verifyDoAcct2DeleteSwmPhoto5(PhotoShareAutoTestState& ts)
{
    int rv = 0;
    {   // Check Shared With dataset for Account 2
        setTargetMachine(Client2, ts);
        rv = waitForSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_2_EventQueueHandle,
                                           ts.WAIT_FOR_EVENTS_SEC);
        if (rv != 0) {
            LOG_ERROR("waitForSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_15");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_15");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_2_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               ts.testFileBaseTwoAccounts5_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv == 0) {
            LOG_ERROR("SWM record(%s) found! Expected to be unshared ",
                      ts.testFileBaseTwoAccounts5_opaqueMetadata.c_str());
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_16");
            return rv;
        } else if (rv != ERR_GET_ENTRY_NOT_FOUND) {
            LOG_ERROR("getEntryFromQuery("FMTu64"):%d", ts.clientPc_2_UserId, rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_16");
            return rv;
        } else {  // Success!
            ASSERT(rv == ERR_GET_ENTRY_NOT_FOUND);
            LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_16");
            rv = 0;
        }    }

    {   // Check Shared With Me dataset for Account 3, photo should still be shared
        setTargetMachine(Client3, ts);
        rv = verifyNoSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                           ts.clientPcId_3_EventQueueHandle);
        if (rv != 0) {
            LOG_ERROR("verifyNoSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_17");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_17");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.clientPc_3_UserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                               ts.testFileBaseTwoAccounts5_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SWM record(%s,%s):%d not found!",
                      ts.testFileBaseTwoAccounts5_opaqueMetadata.c_str(),
                      ts.clientPc_3_Username.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_18");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_18");
    }

    {   // Check Shared By Me dataset for Account 1, verify nothing changed.
        setTargetMachine(CloudPC, ts);
        rv = verifyNoSyncingAndInsyncEvents(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                            ts.cloudPcEventQueueHandle);
        if (rv != 0) {
            LOG_ERROR("verifyNoSyncingAndInsyncEvents:%d", rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_19");
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_19");

        ccd::SharedFilesQueryObject sfObject;
        rv = getEntryFromQuery(ts.cloudPcUserId,
                               ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                               ts.testFileBaseTwoAccounts5_opaqueMetadata,
                               /*OUT*/ sfObject);
        if (rv != 0) {
            LOG_ERROR("SBM record(%s,%s):%d not found!",
                      ts.testFileBaseTwoAccounts5_opaqueMetadata.c_str(),
                      ts.cloudPcUsername.c_str(), rv);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_20");
            return rv;
        }

        bool account2Found = false;
        bool account3Found = false;
        for (int index=0; index<sfObject.recipient_list_size(); ++index) {
            if (sfObject.recipient_list(index) == ts.clientPc_2_Username) {
                account2Found = true;
            } else
            if (sfObject.recipient_list(index) == ts.clientPc_3_Username) {
                account3Found = true;
            }
        }

        if (!account2Found || !account3Found) {
            LOG_ERROR("Photo(%s) not shared with (%s,%d) or (%s,%d)",
                      ts.testFileBaseTwoAccounts5_opaqueMetadata.c_str(),
                      ts.clientPc_2_Username.c_str(), account2Found,
                      ts.clientPc_3_Username.c_str(), account3Found);
            LOG_ALWAYS("\nTC_RESULT=FAIL ;;; TC_NAME=VerifyUnshare_20");
            rv = -1;
            return rv;
        }
        LOG_ALWAYS("\nTC_RESULT=PASS ;;; TC_NAME=VerifyUnshare_20");
    }
    return rv;
}

// Unshare tests require performAndVerifyBasicShareTests to be successful;
// otherwise, there would be nothing to share!
static int performAndVerifyBasicUnshareTests(PhotoShareAutoTestState& testState)
{
    int rv;
    //asdf
    // http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Photo_Sharing#Unshare_Photo_Test

    {   // Step (3)
        rv = doAcct1UnsharePhoto3WithAcct2(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=doAcct1UnsharePhoto3WithAcct2",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
        rv = verifyDoAcct1UnsharePhoto3WithAcct2(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=checkDoAcct1UnsharePhoto3WithAcct2",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
    }

    {   // Step (4)
        rv = doAcct1UnsharePhoto4WithAcct2(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=doAcct1UnsharePhoto4WithAcct2",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
        rv = verifyDoAcct1UnsharePhoto4WithAcct2(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=checkDoAcct1UnsharePhoto4WithAcct2",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
    }

    {   // Step (5)
        rv = doAcct1UnsharePhoto4WithAcct3(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=doAcct1UnsharePhoto4WithAcct3",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
        rv = verifyDoAcct1UnsharePhoto4WithAcct3(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=checkDoAcct1UnsharePhoto4WithAcct3",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
    }

    {   // Step (6)
        rv = doAcct2DeleteSwmPhoto5(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=doAcct2DeleteSwmPhoto5",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
        rv = verifyDoAcct2DeleteSwmPhoto5(testState);
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=verifyDoAcct2DeleteSwmPhoto5",
                   (rv==0)?"PASS":"FAIL");
        if (rv != 0) {
            goto end;
        }
    }
 end:
    return rv;
}

int do_autotest_sdk_release_photo_share(int argc, const char* argv[])
{
    PhotoShareAutoTestState testState;
    std::string domain;
    std::string user1;
    std::string password1;
    std::string user2;
    std::string password2;
    std::string user3;
    std::string password3;

    if (!checkHelp(argc, argv) && argc==8) {
        int argIndex = 1;
        domain = argv[argIndex++];
        for (;argIndex<argc;++argIndex)
        {
            std::string argKey = argv[argIndex];
            if (argKey == "--acct1" && (argIndex+1)<argc) {
                std::string argValue = argv[argIndex+1];
                argIndex++;
                int commaIndex = argValue.find_first_of(",");
                if (commaIndex != std::string::npos) {
                    user1 = argValue.substr(0, commaIndex);
                    password1 = argValue.substr(commaIndex+1);  // skip over the comma
                }
            } else if (argKey == "--acct2") {
                std::string argValue = argv[argIndex+1];
                argIndex++;
                int commaIndex = argValue.find_first_of(",");
                if (commaIndex != std::string::npos) {
                    user2 = argValue.substr(0, commaIndex);
                    password2 = argValue.substr(commaIndex+1);  // skip over the comma
                }
            } else if (argKey == "--acct3") {
                std::string argValue = argv[argIndex+1];
                argIndex++;
                int commaIndex = argValue.find_first_of(",");
                if (commaIndex != std::string::npos) {
                    user3 = argValue.substr(0, commaIndex);
                    password3 = argValue.substr(commaIndex+1);  // skip over the comma
                }
            }
        }
    }

    if (checkHelp(argc, argv) || domain.empty() ||
        user1.empty() || password1.empty() ||
        user2.empty() || password2.empty() ||
        user3.empty() || password3.empty())
    {
        printf("AutoTest %s <domain> --acct1 <username>,<password> "
                                    "--acct2 <username>,<password> "
                                    "--acct3 <username>,<password>\n",
               argv[0]);
        if (!checkHelp(argc, argv)) {
            printf("Domain:%s, acct1:%s,%s, acct2:%s,%s, acct3:%s,%s\n",
                   domain.c_str(),
                   user1.c_str(), password1.c_str(),
                   user2.c_str(), password2.c_str(),
                   user3.c_str(), password3.c_str());
        }
        return 0;   // No arguments needed
    }

    int rv = setupAutotestPhotoShareFiles(testState);
    if (rv != 0) {
        LOG_ERROR("setupAutotestPhotoShareFiles:%d", rv);
        goto end;
    }

    rv = startCcdInstances(testState);
    if (rv != 0) {
        LOG_ERROR("startCcdInstances:%d", rv);
        goto end;
    }

    rv = loginCcdInstances(user1, password1,
                           user2, password2,
                           user3, password3,
                           testState);
    if (rv != 0) {
        LOG_ERROR("loginCcdInstances(%s,%s,%s,%s,%s,%s):%d",
                  user1.c_str(), password1.c_str(),
                  user2.c_str(), password2.c_str(),
                  user3.c_str(), password3.c_str(),
                  rv);
        goto end;
    }

    rv = setupEventQueues(testState);
    if (rv != 0) {
        LOG_ERROR("setupEventQueues:%d", rv);
        goto end;
    }

    rv = performAndVerifyBasicShareTests(testState);
    if (rv != 0) {
        LOG_ERROR("performAndVerifyBasicShareTests:%d, Unshare tests will not be valid, skipping."
                  "\nTC_RESULT=FAIL ;;; TC_NAME=UnshareTestsInvalidDueToShareFailure_Skipping",
                  rv);
    } else
    {   // Unshare tests require performAndVerifyBasicShareTests to be successful;
        // otherwise, there would be nothing to share!
        rv = performAndVerifyBasicUnshareTests(testState);
        if (rv != 0) {
            LOG_ERROR("performAndVerifyBasicUnshareTests:%d", rv);
        }
        LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=Unshare", (rv==0)?"PASS":"FAIL");
    }

 end:
    {
        int rc;
        rc = teardownEventQueues(testState);
        if (rc != 0) {
            LOG_ERROR("teardownEventQueues:%d", rc);
        }

        rc = endCcdInstances(true, testState);
        if (rc != 0) {
            LOG_ERROR("endCcdInstances:%d", rc);
        }
#ifdef LINUX
        // Give CCD process time to stop before valgrind examines leaks.
        LOG_ALWAYS("Pausing for 15 sec, give CCD time to shut down before leaks examined");
        VPLThread_Sleep(VPLTime_FromSec(15));
#endif
    }

    LOG_ALWAYS("\nTC_RESULT=%s ;;; TC_NAME=%s_Suite",
               rv==0?"PASS":"FAIL",  testState.testName.c_str());

    return rv;
}

