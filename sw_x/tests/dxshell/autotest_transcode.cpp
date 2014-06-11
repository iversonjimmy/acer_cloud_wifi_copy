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
#include "setup_stream_test.hpp"
#include "mca_diag.hpp"

#include "autotest_transcode.hpp"

#include "cJSON2.h"

#include <string>

int do_autotest_regression_streaming_transcode_positive(int argc, const char* argv[])
{
    int rv = VPL_OK;
    u32 retry = 0;
    u32 photoAlbumNum = 0;
    u32 photoNum = 0;
    u32 expectedPhotoNum = 0;
    std::string photosToStream = "-1";       // meaning no limit
    int cloudPCId = 1;
    int clientPCId = 2;
    std::string CloudPC_alias = "CloudPC";
    std::string MD_alias = "MD";
    std::string osVersion;
    const char* TEST_TRANSCODE_STR = "SdkTranscodeStreamingNegative";

    if (checkHelp(argc, argv) || (argc != 5)) {
        printf("AutoTest %s <username> <password> <expectedPhotoNum>\n", argv[0]);
        return 0;   // No arguments needed
    }

    LOG_ALWAYS("AutoTest Transcode Streaming: Domain(%s) User(%s) Password(%s) ExpectedPhotoNum(%s)",
               argv[1], argv[2], argv[3], argv[4]);

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    expectedPhotoNum = static_cast<u32>(atoi(argv[4]));

    VPLFile_Delete("dumpfile");

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD (instanceId %d) ====", cloudPCId);
    SET_TARGET_MACHINE(TEST_TRANSCODE_STR, CloudPC_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

	CHECK_LINK_REMOTE_AGENT(CloudPC_alias, TEST_TRANSCODE_STR, rv);

    START_CCD(TEST_TRANSCODE_STR, rv);
    START_CLOUDPC(argv[2], argv[3], TEST_TRANSCODE_STR, true, rv);

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));

    LOG_ALWAYS("\n\n==== Launching MD CCD (instanceId %d) ====", clientPCId);
    SET_TARGET_MACHINE(TEST_TRANSCODE_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    CHECK_LINK_REMOTE_AGENT(MD_alias, TEST_TRANSCODE_STR, rv);

    QUERY_TARGET_OSVERSION(osVersion, TEST_TRANSCODE_STR, rv);

    START_CCD(TEST_TRANSCODE_STR, rv);

    UPDATE_APP_STATE(TEST_TRANSCODE_STR, rv);

    START_CLIENT(argv[2], argv[3], TEST_TRANSCODE_STR, true, rv);

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
	{
        std::vector<u64> deviceIds;
        u64 userId = 0;
        u64 cloudPCDeviceId = 0;
        u64 MDDeviceId = 0;
        const char *testCloudStr = "CheckCloudPCDeviceLinkStatus";
        const char *testMDStr = "CheckMDDeviceLinkStatus";

        SET_TARGET_MACHINE(TEST_TRANSCODE_STR, CloudPC_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
         }

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, testCloudStr, rv);
        }

        rv = getDeviceId(&cloudPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get CloudPC device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, testCloudStr, rv);
        }

        SET_TARGET_MACHINE(TEST_TRANSCODE_STR, MD_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        rv = getDeviceId(&MDDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get MD device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, testMDStr, rv);
        }


        deviceIds.push_back(cloudPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, cloudPCDeviceId);
        deviceIds.push_back(MDDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, MDDeviceId);

        rv = wait_for_devices_to_be_online_by_alias(TEST_TRANSCODE_STR, CloudPC_alias.c_str(), cloudPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, testCloudStr, rv);
        rv = wait_for_devices_to_be_online_by_alias(TEST_TRANSCODE_STR, MD_alias.c_str(), clientPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, testMDStr, rv);

        SET_TARGET_MACHINE(TEST_TRANSCODE_STR, MD_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        rv = wait_for_cloudpc_get_accesshandle(userId, cloudPCDeviceId, 20);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, "CheckCloudPCGetAccessHandle", rv);
    }

    LOG_ALWAYS("\n\n==== Testing ClearMetadata (CloudPC) ====");
	SET_TARGET_MACHINE(TEST_TRANSCODE_STR, CloudPC_alias.c_str(), rv);
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
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, testStr, rv);
    }

#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n==== Testing CloudMedia CloudnodeAddPhoto (ClientPC) ====");
    SET_TARGET_MACHINE(TEST_TRANSCODE_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
	}

    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != VPL_OK) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "CloudnodeAddPhoto";
        photoSetPath.append("/GoldenTest/image_transcode_positive_tests");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != VPL_OK) {
            LOG_ERROR("SdkTranscodeStreaming_CloudMedia_CloudnodeAddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, "CloudMedia_CloudnodeAddPhoto", rv);
    }
#endif // CLOUDNODE

    LOG_ALWAYS("\n\n==== Testing CloudMedia AddPhoto (CloudPC) ====");
    SET_TARGET_MACHINE(TEST_TRANSCODE_STR, CloudPC_alias.c_str(), rv);
    if (rv < 0) {
         setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != VPL_OK) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "AddPhoto";
        photoSetPath.append("/GoldenTest/image_transcode_positive_tests");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != VPL_OK) {
            LOG_ERROR("SdkTranscodeStreaming_CloudMedia_AddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, "CloudMedia_AddPhoto", rv);
    }

    // Verify if the metadata is synced
    retry = 0;
	SET_TARGET_MACHINE(TEST_TRANSCODE_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
	
    LOG_ALWAYS("\n\n==== List Metadata (ClientPC) ====");
    {
        VPLTime_t startTime = VPLTime_GetTimeStamp();
        while(1) {
            rv = mca_get_photo_object_num(photoAlbumNum, photoNum);
            if(rv != VPL_OK) {
                LOG_ERROR("Cannot get photo count from metadata");
            }
            else if(photoNum == expectedPhotoNum) {
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime);
                LOG_ALWAYS("Retry (%d) times waited ("FMTu64") seconds for photo to be synced.", retry, elapsedTime);
                break;
            }
            if(retry++ > METADATA_SYNC_TIMEOUT) {
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime);
                LOG_ERROR("Timeout retry (%d) waiting for metadata to sync; waited for ("FMTu64") seconds.", retry, elapsedTime);
                rv = -1;
                break;
            } else {
                LOG_ALWAYS("Waiting (cnt %d) photoNum (%d) photoAlbumNum (%d)", retry, photoNum, photoAlbumNum);
            }
            VPLThread_Sleep(VPLTIME_FROM_SEC(1));
        }
        LOG_ALWAYS("Photo added by CloudPC: %d; Expected photo: %d", photoNum, expectedPhotoNum);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, "Verify_Uploaded_Photo_Num1", rv);
    }

    LOG_ALWAYS("\n\n==== SetupStreamTest to generate dump file ====");
    {
        const char *testStr = "SetupStreamTest";
        const char *testArg[] = { testStr, "-d", "dumpfile", "-f", "2", "-M", photosToStream.c_str() };
        rv = setup_stream_test(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, "SetupStreamTest_dumpfile", rv);
    }

    // Positive Test - Scale the photos to 800x800
    LOG_ALWAYS("\n\n==== Positive Test, scale photos to 800x800 ====");
    {
        const char *testStr = "TimeStreamDownload";
        const char *testArg[] = { testStr, "-d", "dumpfile", "-R", "800,800", "-f", "JPG" };
        rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);

		/*
         * Bug 10697: Above test failed on  builder_win_desk_on_win7_test, but passed on builder_win_desk_on_win8_test and my desktop.
         *            Mark this as EXPECTED_TO_FAIL until we found the root cause.
         */
        if (rv == VPL_OK) {
            // It passed on win8, now.
            CHECK_AND_PRINT_RESULT(TEST_TRANSCODE_STR, "Scale_to_800x800", rv);
        } else {
            // It failed on builder_win_desk_on_win7_test.
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_TRANSCODE_STR, "Scale_to_800x800", rv, "10697");
            rv = 0;
        }
    }

exit:
    LOG_ALWAYS("\n\n== Freeing Client ==");
    if(set_target_machine(MD_alias.c_str()) < 0)
        setCcdTestInstanceNum(clientPCId);
    {
        const char *testArg[] = { "StopClient" };
        stop_client(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

	if (isWindows(osVersion) || osVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    LOG_ALWAYS("\n\n== Freeing Cloud PC ==");
    if(set_target_machine(CloudPC_alias.c_str()) < 0)
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

int do_autotest_regression_streaming_transcode_negative(int argc, const char* argv[])
{
    int rv = VPL_OK;
    u32 retry = 0;
    u32 photoAlbumNum = 0;
    u32 photoNum = 0;
    u32 expectedPhotoNum = 0;
    std::string photosToStream = "-1";       // meaning no limit
    int cloudPCId = 1;
    int clientPCId = 2;
    std::string CloudPC_alias = "CloudPC";
    std::string MD_alias = "MD";
    std::string osVersion;
    const char* TEST_TRANSCODE2_STR = "SdkTranscodeStreamingPositive";

    bool full = false;

    if (argc == 6 && (strcmp(argv[5], "-f") == 0 || strcmp(argv[5], "--fulltest") == 0) ) {
        full = true;
    }

    if (checkHelp(argc, argv) || (argc < 5) || (argc == 6 && !full)) {
        printf("AutoTest %s <username> <password> <expectedPhotoNum> [<fulltest>(-f/--fulltest)]\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("AutoTest Transcode Streaming: Domain(%s) User(%s) Password(%s) ExpectedPhotoNum(%s)",
               argv[1], argv[2], argv[3], argv[4]);

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    expectedPhotoNum = static_cast<u32>(atoi(argv[4]));

    VPLFile_Delete("dumpfile");

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD (instanceId %d) ====", cloudPCId);
	SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, CloudPC_alias.c_str(), rv);
    if (rv < 0) {
		setCcdTestInstanceNum(cloudPCId);
    }

	CHECK_LINK_REMOTE_AGENT(CloudPC_alias, TEST_TRANSCODE2_STR, rv);

	START_CCD(TEST_TRANSCODE2_STR, rv);
    START_CLOUDPC(argv[2], argv[3], TEST_TRANSCODE2_STR, true, rv);

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));

    LOG_ALWAYS("\n\n==== Launching MD CCD (instanceId %d) ====", clientPCId);
    SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    CHECK_LINK_REMOTE_AGENT(MD_alias, TEST_TRANSCODE2_STR, rv);

    QUERY_TARGET_OSVERSION(osVersion, TEST_TRANSCODE2_STR, rv);

    START_CCD(TEST_TRANSCODE2_STR, rv);

    UPDATE_APP_STATE(TEST_TRANSCODE2_STR, rv);

    START_CLIENT(argv[2], argv[3], TEST_TRANSCODE2_STR, true, rv);

    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
    {
		std::vector<u64> deviceIds;
        u64 userId = 0;
        u64 cloudPCDeviceId = 0;
        u64 MDDeviceId = 0;
        const char *testCloudStr = "CheckCloudPCDeviceLinkStatus";
        const char *testMDStr = "CheckMDDeviceLinkStatus";

        SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, CloudPC_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, testCloudStr, rv);
        }

        rv = getDeviceId(&cloudPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get CloudPC device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, testCloudStr, rv);
       }

        SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, MD_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        rv = getDeviceId(&MDDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get MD device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, testMDStr, rv);
        }


        deviceIds.push_back(cloudPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, cloudPCDeviceId);
        deviceIds.push_back(MDDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, MDDeviceId);

        rv = wait_for_devices_to_be_online_by_alias(TEST_TRANSCODE2_STR, CloudPC_alias.c_str(), cloudPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, testCloudStr, rv);
        rv = wait_for_devices_to_be_online_by_alias(TEST_TRANSCODE2_STR, MD_alias.c_str(), clientPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, testMDStr, rv);

        SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, MD_alias.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        rv = wait_for_cloudpc_get_accesshandle(userId, cloudPCDeviceId, 20);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, "CheckCloudPCGetAccessHandle", rv);
	}

    LOG_ALWAYS("\n\n==== Testing ClearMetadata (CloudPC) ====");
	SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, CloudPC_alias.c_str(), rv);
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
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, testStr, rv);
    }

#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n==== Testing CloudMedia CloudnodeAddPhoto (ClientPC) ====");
    SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != VPL_OK) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "CloudnodeAddPhoto";
        photoSetPath.append("/GoldenTest/image_transcode_negative_test_1");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != VPL_OK) {
            LOG_ERROR("SdkTranscodeStreaming2_CloudMedia_CloudnodeAddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, "CloudMedia_CloudnodeAddPhoto", rv);
    }
#endif // CLOUDNODE

    LOG_ALWAYS("\n\n==== Testing CloudMedia AddPhoto (CloudPC) ====");
    SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, CloudPC_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string dxroot;
        std::string photoSetPath;
        rv = getDxRootPath(dxroot);
        if (rv != VPL_OK) {
            LOG_ERROR("Fail to set %s root path", argv[0]);
            goto exit;
        }
        photoSetPath.assign(dxroot.c_str());
        const char *testStr1 = "CloudMedia";
        const char *testStr2 = "AddPhoto";
        photoSetPath.append("/GoldenTest/image_transcode_negative_test_1");
        const char *testArg4[] = { testStr1, testStr2, photoSetPath.c_str() };
        rv = cloudmedia_commands(3, testArg4);
        if (rv != VPL_OK) {
            LOG_ERROR("SdkTranscodeStreaming2_CloudMedia_AddPhoto failed!");
        }
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, "CloudMedia_AddPhoto", rv);
    }

    // Verify if the metadata is synced
    retry = 0;
    SET_TARGET_MACHINE(TEST_TRANSCODE2_STR, MD_alias.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    LOG_ALWAYS("\n\n==== List Metadata (ClientPC) ====");
    {
        VPLTime_t startTime = VPLTime_GetTimeStamp();
        while(1) {
            rv = mca_get_photo_object_num(photoAlbumNum, photoNum);
            if(rv != VPL_OK) {
                LOG_ERROR("Cannot get photo count from metadata");
            }
            else if(photoNum == expectedPhotoNum) {
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime);
                LOG_ALWAYS("Retry (%d) times waited ("FMTu64") seconds for photo to be synced.", retry, elapsedTime);
                break;
            }
            if(retry++ > METADATA_SYNC_TIMEOUT) {
                u64 elapsedTime = VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - startTime);
                LOG_ERROR("Timeout retry (%d) waiting for metadata to sync; waited for ("FMTu64") seconds.", retry, elapsedTime);
                rv = -1;
                break;
            } else {
                LOG_ALWAYS("Waiting (cnt %d) photoNum (%d) photoAlbumNum (%d)", retry, photoNum, photoAlbumNum);
            }
            VPLThread_Sleep(VPLTIME_FROM_SEC(1));
        }
        LOG_ALWAYS("Photo added by CloudPC: %d; Expected photo: %d", photoNum, expectedPhotoNum);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, "Verify_Uploaded_Photo_Num1", rv);
    }

    LOG_ALWAYS("\n\n==== SetupStreamTest to generate dump file ====");
    {
        const char *testStr = "SetupStreamTest";
        const char *testArg[] = { testStr, "-d", "dumpfile", "-f", "2", "-M", photosToStream.c_str() };
        rv = setup_stream_test(ARRAY_ELEMENT_COUNT(testArg), testArg);
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, "SetupStreamTest_dumpfile", rv);
    }

    // Negative Test - Scale an invalid photo to 800x800
    LOG_ALWAYS("\n\n==== Negative Test, scale an invalid photo to 800x800 ====");
    {
        const char *testStr = "TimeStreamDownload";
        const char *testArg[] = { testStr, "-d", "dumpfile", "-R", "800,800", "-f", "JPG" };
        rv = time_stream_download(ARRAY_ELEMENT_COUNT(testArg), testArg);
        if (rv == -1) {
            rv = 0;
        } else if (rv == 0) {
            if(osVersion == OS_LINUX) {
                rv = -1;
                CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_TRANSCODE2_STR, "NegativeTest_InvalidImage", rv, "13344");
                rv = 0;
                goto exit;
            }
        } else {
            LOG_ERROR("Expected -1, but got %d", rv);
            rv = -1;
        }
        CHECK_AND_PRINT_RESULT(TEST_TRANSCODE2_STR, "NegativeTest_InvalidImage", rv);
    }

exit:
    LOG_ALWAYS("\n\n== Freeing Client ==");
    if(set_target_machine(MD_alias.c_str()) < 0)
        setCcdTestInstanceNum(clientPCId);
    {
        const char *testArg[] = { "StopClient" };
        stop_client(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    if (isWindows(osVersion) || osVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(ARRAY_ELEMENT_COUNT(testArg), testArg);
    }

    LOG_ALWAYS("\n\n== Freeing Cloud PC ==");
    if(set_target_machine(CloudPC_alias.c_str()) < 0)
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
