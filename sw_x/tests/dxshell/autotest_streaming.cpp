//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include <gvm_file_utils.hpp>
#include <ccdi_client_tcp.hpp>

#include "autotest_common_utils.hpp"
#include "dx_common.h"
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "ccdconfig.hpp"
#include "setup_stream_test.hpp"
#include "mca_diag.hpp"

#include "autotest_streaming.hpp"
#include "fs_test.hpp"
#include "TimeStreamDownload.hpp"

#include "cJSON2.h"

#include <string>
#include <sstream>

static int autotest_regression_streaming_generic(const char *TEST_REGRESSION_STREAM_STR, int argc, const char* argv[]);

int do_autotest_regression_streaming_internal_direct(int argc, const char* argv[])
{
    return autotest_regression_streaming_generic("RegressionStreaming_InterDirect", argc, argv);
}

int do_autotest_regression_streaming_proxy(int argc, const char* argv[])
{
    return autotest_regression_streaming_generic("RegressionStreaming_Proxy", argc, argv);
}

#define VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(tc_name, expectedTrackNum, rc, expected_to_fail, bug) \
    do { \
        int retry = 0; \
        u32 albumNum = 0, trackNum = 0; \
        VPLTime_t startTime = VPLTime_GetTimeStamp(); \
        while(1) { \
            rv = mca_get_music_object_num(albumNum, trackNum); \
            if(rv != 0) { \
                LOG_ERROR("Cannot get music count from metadata!"); \
                break; \
            } \
            else if(trackNum == expectedTrackNum) { \
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime); \
                LOG_ALWAYS("Retry (%d) times waited ("FMTu64") seconds for music to be synced.", retry, elapsedTime); \
                break; \
            } \
            if(retry++ > METADATA_SYNC_TIMEOUT) { \
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime); \
                LOG_ERROR("Timeout retry (%d) waiting for metadata to sync; waited for ("FMTu64") seconds.", retry, elapsedTime); \
                rv = -1; \
                break; \
            } else { \
                LOG_ALWAYS("Waiting (cnt %d) tracksNum (%d)", retry, trackNum); \
            } \
            VPLThread_Sleep(VPLTIME_FROM_SEC(1)); \
        } \
        LOG_ALWAYS("Music tracks: %d; Expected music tracks: %d!", trackNum, expectedTrackNum); \
        if (expected_to_fail) { \
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(tc_name, "VerifyCloudMediaMusicTrackNum", rc, bug); \
        } \
        else { \
            CHECK_AND_PRINT_RESULT(tc_name, "VerifyCloudMediaMusicTrackNum", rc); \
        } \
    } while (0)

#define VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(tc_name, expectedPhotoNum, rc, expected_to_fail, bug) \
    do { \
        int retry = 0; \
        u32 albumNum = 0, photoNum = 0; \
        VPLTime_t startTime = VPLTime_GetTimeStamp(); \
        while(1) { \
            rv = mca_get_photo_object_num(albumNum, photoNum); \
            if(rv != 0) { \
                LOG_ERROR("Cannot get photo count from metadata!"); \
                break; \
            } \
            else if(photoNum == expectedPhotoNum) { \
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime); \
                LOG_ALWAYS("Retry (%d) times waited ("FMTu64") seconds for photo to be synced.", retry, elapsedTime); \
                break; \
            } \
            if(retry++ > METADATA_SYNC_TIMEOUT) { \
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime); \
                LOG_ERROR("Timeout retry (%d) waiting for metadata to sync; waited for ("FMTu64") seconds.", retry, elapsedTime); \
                rv = -1; \
                break; \
            } else { \
                LOG_ALWAYS("Waiting (cnt %d) photoNum (%d)", retry, photoNum); \
            } \
            VPLThread_Sleep(VPLTIME_FROM_SEC(1)); \
        } \
        LOG_ALWAYS("Photo number: %d; Expected photo number: %d!", photoNum, expectedPhotoNum); \
        if (expected_to_fail) { \
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(tc_name, "VerifyCloudMediaPhotoNum", rc, bug); \
        } \
        else { \
            CHECK_AND_PRINT_RESULT(tc_name, "VerifyCloudMediaPhotoNum", rc); \
        } \
    } while (0)

int autotest_regression_streaming_generic(const char *TEST_REGRESSION_STREAM_STR, int argc, const char* argv[])
{
    int rv = 0;
    u32 retry = 0;
    //u32 musicAlbumNum = 0, photoAlbumNum = 0;
    //u32 tracksNum = 0, photoNum = 0;
    u32 expectedMusicNum = 0;
    u32 expectedPhotoNum = 0;
    std::string musicTracksToStream = "-1";  // meaning no limit
    std::string photosToStream = "-1";       // meaning no limit
    std::string dxroot;
    std::string dumpfilePath;
    std::string collectionid;
    int cloudPCId = 1;
    std::string cloudpc_alias = "CloudPC";
    int clientPC1Id = 2;
    std::string MD_alias = "MD";
#if defined(CLOUDNODE)
    int clientPC2Id = 3;
    std::string client_alias = "Client";
#endif
    std::string osVersion;
    bool is_full_test = false;

    if (argc == 5 && (strcmp(argv[4], "-f") == 0 || strcmp(argv[4], "--fulltest") == 0)) {
        is_full_test = true;
        if(strcmp(TEST_REGRESSION_STREAM_STR, "RegressionStreaming_Proxy") == 0) {
            musicTracksToStream.assign("90");
        }
    }

    if (checkHelp(argc, argv) || (argc != 4 && argc != 6 && (argc == 5 && !is_full_test)) || (argc > 6)) {
        printf("AutoTest %s <domain> <username> <password> [<musicTracksToStream> <photosToStream>] [<-f/--fulltest>]\n", argv[0]);
        return 0;   // No arguments needed 
    } 

    if (argc == 4) {
        LOG_ALWAYS("AutoTest Regression Streaming: Domain(%s) User(%s) Password(%s)",
                   argv[1], argv[2], argv[3]);
    }
    else if (argc == 6) {
        LOG_ALWAYS("AutoTest Regression Streaming: Domain(%s) User(%s) Password(%s) musicTracksToStream(%s) photosToStream(%s)",
                   argv[1], argv[2], argv[3], argv[4], argv[5]);
        musicTracksToStream.assign(argv[4]);
        photosToStream.assign(argv[5]);
    }

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    VPLFile_Delete("dumpfile1");
    VPLFile_Delete("dumpfile2");

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD (instanceId %d) ====", cloudPCId);
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    CHECK_LINK_REMOTE_AGENT(cloudpc_alias, TEST_REGRESSION_STREAM_STR, rv);
    START_CCD(TEST_REGRESSION_STREAM_STR, rv);

    {
        const char *testStr = "StartCloudPC";
        const char *testArg[] = { testStr, argv[2], argv[3] };
        rv = start_cloudpc(ARRAY_ELEMENT_COUNT(testArg), testArg);
        if (rv < 0) {
            // If fail, try again before declaring failure
            rv = start_cloudpc(ARRAY_ELEMENT_COUNT(testArg), testArg);
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, testStr, rv);
    }

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));

    LOG_ALWAYS("\n\n==== Launching MD CCD (instanceId %d) ====", clientPC1Id);
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC1Id);
    }

    CHECK_LINK_REMOTE_AGENT(MD_alias, TEST_REGRESSION_STREAM_STR, rv);

    {
        QUERY_TARGET_OSVERSION(osVersion, TEST_REGRESSION_STREAM_STR, rv);
    }

    CHECK_LINK_REMOTE_AGENT(MD_alias, TEST_REGRESSION_STREAM_STR, rv);
    START_CCD(TEST_REGRESSION_STREAM_STR, rv);

    UPDATE_APP_STATE(TEST_REGRESSION_STREAM_STR, rv);

    START_CLIENT(argv[2], argv[3], TEST_REGRESSION_STREAM_STR, true, rv);

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));

#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n==== Launching Client CCD (testInstanceNum %d) ====", clientPC2Id);
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, client_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC2Id);
    }

    CHECK_LINK_REMOTE_AGENT(client_alias, TEST_REGRESSION_STREAM_STR, rv);

    {
        QUERY_TARGET_OSVERSION(osVersion, TEST_REGRESSION_STREAM_STR, rv);
    }

    CHECK_LINK_REMOTE_AGENT(client_alias, TEST_REGRESSION_STREAM_STR, rv);
    START_CCD(TEST_REGRESSION_STREAM_STR, rv);
    START_CLIENT(argv[2], argv[3], TEST_REGRESSION_STREAM_STR, true, rv);

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));
#endif

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
    {
        std::vector<u64> deviceIds;
        u64 userId = 0;
        u64 cloudPCDeviceId = 0;
        u64 clientPCDeviceId = 0;
#if defined(CLOUDNODE)
        u64 clientPC2DeviceId = 1;
#endif
        SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPC1Id);
        }

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }

        rv = getDeviceId(&clientPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }

        SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        rv = getDeviceId(&cloudPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }

#if defined(CLOUDNODE)
        SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, client_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPC2Id);
        }

        rv = getDeviceId(&clientPC2DeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
#endif
        deviceIds.push_back(clientPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, clientPCDeviceId);
        deviceIds.push_back(cloudPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, cloudPCDeviceId);
#if defined(CLOUDNODE)
        deviceIds.push_back(clientPC2DeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, clientPC2DeviceId);
#endif

        rv = wait_for_devices_to_be_online_by_alias(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), cloudPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CheckCloudDeviceLinkStatus", rv);
        rv = wait_for_devices_to_be_online_by_alias(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), clientPC1Id, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CheckMDDeviceLinkStatus", rv);
#if defined(CLOUDNODE)
        rv = wait_for_devices_to_be_online_by_alias(TEST_REGRESSION_STREAM_STR, client_alias.c_str(), clientPC2Id, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CheckClientDeviceLinkStatus", rv);
#endif

        SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPC1Id);
        }

        rv = wait_for_cloudpc_get_accesshandle(userId, cloudPCDeviceId, 20);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CheckCloudPCGetAccessHandle", rv);
    }

    LOG_ALWAYS("\n\n==== Testing ClearMetadata (CloudPC) ====");
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        const char *testStr = "ClearMetadata";
        const char *testArg[] = { testStr };
        rv = msa_delete_catalog(ARRAY_ELEMENT_COUNT(testArg), testArg);
        if(rv != 0) {
            LOG_ERROR("RegressionStreaming_ClearMetadata failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, testStr, rv);
    }

    // Test 1 - Streaming in different modes and simulate basic user behavior
#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n==== Testing CloudMedia CloudnodeAddMusic (Client PC) ====");
    // XXX temp workaround to let the client device link up before
    // trying to access the cloudnode
    sleep(5);
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, client_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC2Id);
    }
    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        photoSetPath.append("/GoldenTest/TestMusicData/MusicTestSet1");
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "CloudnodeAddMusic";
        const char *testArg1[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg1);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_CloudnodeAddMusic failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_CloudnodeAddMusic", rv);
    }
#endif // !CLOUDNODE

    LOG_ALWAYS("\n\n==== Testing CloudMedia AddMusic (CloudPC) ====");
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        photoSetPath.append("/GoldenTest/TestMusicData/MusicTestSet1");
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "AddMusic";
        const char *testArg1[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg1);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_AddMusic failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_AddMusic", rv);
        {
            u32 musicNum = 0;
            countMusicNum(photoSetPath, musicNum);
            expectedMusicNum += musicNum;
        }
    }

    LOG_ALWAYS("expectedMusicNum: %d", expectedMusicNum);

    // Verify if the metadata is synced
    retry = 0;
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC1Id);
    }

    LOG_ALWAYS("\n\n==== List Metadata (Client) ====");
    VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_REGRESSION_STREAM_STR, expectedMusicNum, rv, false, "");

#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n==== Testing CloudMedia CloudnodeAddPhoto (Client PC) ====");
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, client_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC1Id);
    }
    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "CloudnodeAddPhoto";
        photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet2");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_CloudnodeAddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_Cloudnode_AddPhotoTestSet2", rv);
    }
    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "CloudnodeAddPhoto";
        photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet2_nothumb");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_CloudnodeAddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_Cloudnode_AddPhotoTestSet2_nothumb", rv);
    }

    if (is_full_test) {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "CloudnodeAddPhoto";
        photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet4");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_CloudnodeAddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_Cloudnode_AddPhotoTestSet4", rv);
    }
#endif // CLOUDNODE
    LOG_ALWAYS("\n\n==== Testing CloudMedia AddPhoto (CloudPC) ====");
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "AddPhoto";
        photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet2");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_AddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_AddPhotoTestSet2", rv);
        {
            u32 photoNum = 0;
            countPhotoNum(photoSetPath, photoNum);
            expectedPhotoNum += photoNum;
        }
    }
    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "AddPhoto";
        photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet2_nothumb");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_AddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_AddPhotoTestSet2_nothumb", rv);
        {
            u32 photoNum = 0;
            countPhotoNum(photoSetPath, photoNum);
            expectedPhotoNum += photoNum;
        }
    }

    if (is_full_test) {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != 0) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "AddPhoto";
        photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet4");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != 0) {
            LOG_ERROR("RegressionStreaming_CloudMedia_AddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "CloudMedia_AddPhotoTestSet4", rv);
        {
            u32 photoNum = 0;
            countPhotoNum(photoSetPath, photoNum);
            expectedPhotoNum += photoNum;
        }
    }

    LOG_ALWAYS("expectedPhotoNum: %d", expectedPhotoNum);

    // Verify if the metadata is synced
    retry = 0;
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC1Id);
    }

    LOG_ALWAYS("\n\n==== List Metadata (Client) ====");
    VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_REGRESSION_STREAM_STR, expectedPhotoNum, rv, false, "");

    LOG_ALWAYS("\n\n==== Testing Streaming (Client) ====");
    VPLFile_Delete("dumpfile1");
    LOG_ALWAYS("\n\n==== SetupStreamTest to regenerate dump file (dumpfile1) ====");
    {
        const char *testStr = "SetupStreamTest";
        const char *testArg[] = { testStr, "-d", "dumpfile1", "-f", "1", "-M", musicTracksToStream.c_str() };
        rv = setup_stream_test(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "SetupStreamTest_dumpfile1", rv);
    }
    {
        const char *testStr = "SetupStreamTest";
        const char *testArg[] = { testStr, "-d", "dumpfile2", "-f", "2", "-M", photosToStream.c_str() };
        rv = setup_stream_test(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "SetupStreamTest_dumpfile2", rv);
    }

#ifdef WIN32
    LOG_ALWAYS("\n\n==== TagEdit before streaming (dumpfile2) ====");
    {
        SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), rv);

        u64 userId;
        u64 deviceId;
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }
        rv = getDeviceId(&deviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        std::string deviceIdStr;
        std::stringstream ss;
        ss << deviceId;
        deviceIdStr = ss.str();
        std::string response;
        std::string photo_object_id_to_edit;
        
        TimeStreamDownload download;
        std::vector<std::string> dumpfiles;
        std::vector<std::vector<FileInfo> > vvfileInfo;
        dumpfiles.push_back("dumpfile2");
        rv = download.loadDumpfile(dumpfiles, vvfileInfo);
        if(rv != 0){
            LOG_ERROR("fail to loadDumpfile:%d, %s", rv, dumpfiles[0].c_str());
            goto exit;
        }
        photo_object_id_to_edit = vvfileInfo[0][0].oid;

        SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);

        rv = fs_test_edittag(userId, deviceIdStr, photo_object_id_to_edit, "Title=dxShellTest", response, true /*is_mediarf*/);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "EditTagTitle", rv);

        rv = fs_test_edittag(userId, deviceIdStr, photo_object_id_to_edit, "Title=dxShellTest;Author=dxShellTest2", response, true /*is_mediarf*/);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "EditTagTitleAuthor", rv);
    }

    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC1Id);
    }
#endif

    LOG_ALWAYS("\n\n==== Download tracks continuously (dumpfile2) ====");
    {
        const char *testStr = "TimeStreamDownload";
        const char *testArg[] = { testStr, "-d", "dumpfile2" };
        rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "TimeStreamDownload", rv);
    }

    if (is_full_test) {
        LOG_ALWAYS("\n\n==== Download tracks continuously ====");
        {
            const char *testStr = "TimeStreamDownload";
            const char *testArg[] = { testStr, "-d", "dumpfile1" };
            rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
            CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "TimeStreamDownload", rv);
        }
    }

    LOG_ALWAYS("\n\n==== Download tracks with 1 sec delay in between ====");
    {
        const char *testStr = "TimeStreamDownload";
        const char *testArg[] = { testStr, "-d", "dumpfile2", "-D", "1000" };
        rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "dumpfile2_1sec_Delay", rv);
    }

    LOG_ALWAYS("\n\n==== Download each track for 8K bytes ====");
    {
        const char *testStr = "TimeStreamDownload";
        const char *testArg[] = { testStr, "-d", "dumpfile2", "-m", "8192" };
        rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "dumpfile2_8K_Max", rv);
    }

    if (is_full_test) {
        LOG_ALWAYS("\n\n==== Download tracks with 1 sec delay in between ====");
        {
            const char *testStr = "TimeStreamDownload";
            const char *testArg[] = { testStr, "-d", "dumpfile1", "-D", "1000" };
            rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
            CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "1sec_Delay", rv);
        }

        LOG_ALWAYS("\n\n==== Download each track for 1M bytes ====");
        {
            const char *testStr = "TimeStreamDownload";
            const char *testArg[] = { testStr, "-d", "dumpfile1", "-m", "1024000" };
            rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
            CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "1M_Max", rv);
        }
    }

    // Test 3 - Multiple client applications against one CCD
    LOG_ALWAYS("\n\n==== Download in 2 threads ====");
    {
        const char *testStr = "TimeStreamDownload";
        const char *testArg[] = { testStr, "-d", "dumpfile1, dumpfile2", "-m", "0, 0, 0, 0", "-t", "2" };
        rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "Two_Threads", rv);
    }

    //Test streaming after deletion
    LOG_ALWAYS("\n\n==== Testing ClearMetadata (CloudPC) ====");
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, cloudpc_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        const char *testStr = "ClearMetadata";
        const char *testArg[] = { testStr };
        rv = msa_delete_catalog(ARRAY_ELEMENT_COUNT(testArg), testArg);
        if(rv != 0) {
            LOG_ERROR("RegressionStreaming_ClearMetadata failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, testStr, rv);
    }

    // Verify if the metadata is synced
    retry = 0;
    SET_TARGET_MACHINE(TEST_REGRESSION_STREAM_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPC1Id);
    }

    LOG_ALWAYS("\n\n==== List Metadata (MD) ====");
    VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_REGRESSION_STREAM_STR, 0, rv, false, "");

    LOG_ALWAYS("\n\n==== Download tracks continuously (dumpfile1) ====");
    {
        const char *testStr = "TimeStreamDownload";
        const char *testArg[] = { testStr, "-d", "dumpfile1" };
        rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
        //This is expected to fail
        rv = (rv == 0)? -1 : 0;
        CHECK_AND_PRINT_RESULT(TEST_REGRESSION_STREAM_STR, "Stream_After_Delete", rv);
    }

exit:
    LOG_ALWAYS("\n\n== Freeing MD ==");
    if(set_target_machine(MD_alias.c_str()) < 0)
        setCcdTestInstanceNum(clientPC1Id);
    {
        const char *testArg[] = { "StopClient" };
        stop_client(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    if (isWindows(osVersion) || osVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n== Freeing Client ==");
    if(set_target_machine(client_alias.c_str()) < 0)
        setCcdTestInstanceNum(clientPC2Id);
    {
        const char *testArg[] = { "StopClient" };
        stop_client(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    if (isWindows(osVersion) || osVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }
#endif

    LOG_ALWAYS("\n\n== Freeing Cloud PC ==");
    if(set_target_machine(cloudpc_alias.c_str()) < 0)
        setCcdTestInstanceNum(cloudPCId);
    {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    return rv;
}
