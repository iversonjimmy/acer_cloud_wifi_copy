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
#include "vplu_sstr.hpp"
#include "scopeguard.hpp"

#include "autotest_common_utils.hpp"
#include "dx_common.h"
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "ccdconfig.hpp"
#include "mca_diag.hpp"
#include "HttpAgent.hpp"

#include "autotest_mediametadata.hpp"

#include "cJSON2.h"

#include <string>
#include <sstream>

#if defined(CLOUDNODE)
#define IS_CLOUDNODE  (true)
#else
#define IS_CLOUDNODE  (false)
#define start_ccd_in_client_subdir()  (-1)
#endif

const bool useCcdInClientSubdir = IS_CLOUDNODE;

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

#define CM_TEMPLATE_OP(op, num, skip, arg1, arg2, arg3, rc) \
    BEGIN_MULTI_STATEMENT_MACRO \
    const char *testArgs[] = { "CloudMedia", op, arg1, arg2, arg3}; \
    rc = cloudmedia_commands(num, testArgs); \
    if (!skip) { CHECK_AND_PRINT_RESULT("SdkCloudMediaRelease", op, rv); } \
    END_MULTI_STATEMENT_MACRO

#define CM_ADD_PHOTO(absolutePath, rc)                            CM_TEMPLATE_OP("AddPhoto",         3,  false, absolutePath.c_str(), NULL, NULL, rc)
#define CM_CLOUDNODE_ADD_PHOTO(absolutePath, rc)                  CM_TEMPLATE_OP("CloudnodeAddPhoto",3,  false, absolutePath.c_str(), NULL, NULL, rc)
#define CM_ADD_MUSIC(absolutePath, rc)                            CM_TEMPLATE_OP("AddMusic",         3,  false, absolutePath.c_str(), NULL, NULL, rc)
#define CM_CLOUDNODE_ADD_MUSIC(absolutePath, rc)                  CM_TEMPLATE_OP("CloudnodeAddMusic",3,  false, absolutePath.c_str(), NULL, NULL, rc)
#define CM_DELETE_OBJECT(catalogType, collectionId, objectId, rc) CM_TEMPLATE_OP("DeleteObejct",     5,  false, catalogType.c_str(),  collectionId, objectId, rc)
#define CM_DELETE_COLLECTION(catalogType, collectionId, rc)       CM_TEMPLATE_OP("DeleteCollection", 4,  false, catalogType.c_str(),  collectionId.c_str(), NULL, rc)
#define CM_ADD_PHOTO2ALBUM(absolutePath, album, rc)               CM_TEMPLATE_OP("AddPhoto2Album",   4,  false, absolutePath.c_str(), album.c_str(), NULL, rc)

//
// Retrieve the actual list of MM thumbnail types for which syncing
// is enabled and compare that to the expected list as supplied
//
static int
__check_sync_enabled_list(std::set<ccd::SyncFeature_t>& enabledList)
{
    int rv;
    {
        ccd::GetSyncStateInput in;
        ccd::GetSyncStateOutput out;

        // Check the actual enabled list against the set passed in
        do {
            in.set_get_mm_thumb_sync_enabled(true);
            rv = CCDIGetSyncState(in, out);
            if (rv != 0) {
                LOG_ERROR("CCDIGetSyncState:%d", rv);
                break;
            }
            if (!out.has_mm_thumb_sync_enabled()) {
                LOG_ERROR("Unexpected SyncState: has_mm_thumb_sync_enabled: %s",
                    out.has_mm_thumb_sync_enabled() ? "true" : "false");
                rv = -1;
                break;
            }
            if (enabledList.empty()) {
                if (out.mm_thumb_sync_enabled()) {
                    LOG_ERROR("Unexpected SyncState: mm_thumb_sync_enabled: %s",
                        out.mm_thumb_sync_enabled() ? "true" : "false");
                    rv = -1;
                    break;
                }
            } else if (!out.mm_thumb_sync_enabled() ||
                        out.mm_thumb_sync_enabled_types_size() != enabledList.size()) {
                LOG_ERROR("Unexpected SyncState: mm_thumb_sync_enabled: %s",
                    out.mm_thumb_sync_enabled() ? "true" : "false");
                LOG_ERROR("  mm_thumb_sync_enabled_types_size: %d",
                    out.mm_thumb_sync_enabled_types_size());
                rv = -1;
                break;
            }

            for (int i = 0; i < out.mm_thumb_sync_enabled_types_size(); i++) {
                ccd::SyncFeature_t sf = out.mm_thumb_sync_enabled_types(i);
                if (enabledList.find(sf) == enabledList.end()) {
                    LOG_ERROR("Unrecognized thumb sync type %d", sf);
                    rv = -1;
                    break;
                }
                enabledList.erase(sf);
            }
            // The list should be empty now
            if (!enabledList.empty()) {
                LOG_ERROR("Did not find expected MM thumb types");
                rv = -1;
                break;
            }
        } while(0);
    }
    return rv;
}

static int autotest_sdk_release_mediametadata_generic(int argc, const char* argv[],
                                               bool using_remote_agent)
{
    const char *TEST_MEDIAMETADATA_STR = "SdkMediaMetadataRelease";
    int rv = 0;
    int cloudPCId = 1;
    int clientPCId = 2;
    int clientPCId2 = 3;
    bool full = false;
    bool unrecognizedOption = false;
    std::string clientPCOSVersion;

    //input value for check_metadata_generic
    int num_music_albums = 3;
    int num_tracks_per_music_album = 10;
    int num_photo_albums = 2;
    int num_photos_per_photo_album = 5;

    // Parse optional
    for(int optionalArgIndex = 4; optionalArgIndex < argc; ++optionalArgIndex) {
        if(std::string(argv[optionalArgIndex]) == "-f") {
            full = true;
        } else if (std::string(argv[optionalArgIndex]) == "--fulltest") {
            full = true;
        } else {
            LOG_ERROR("Unrecognized optional parameter(%d):%s",
                      optionalArgIndex, argv[optionalArgIndex]);
            unrecognizedOption = true;
            rv = -1;
        }
    }


    if (checkHelp(argc, argv) || (argc < 4) || unrecognizedOption) {
        printf("AutoTest %s <domain> <username> <password> "
               "[<fulltest>(-f/--fulltest)]\n",
                argv[0]);
        return rv;   // No arguments needed
    }

    LOG_ALWAYS("Auto Test SDK Release MediaMetadata: "
               "Domain(%s) User(%s) Password(%s) FullTest(%d) ",
               argv[1], argv[2], argv[3], full);

    // TODO: This doesn't actually hard stop on Linux, nor does it apply to all instances.
    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string alias = "CloudPC";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_MEDIAMETADATA_STR, rv);
    }

    START_CCD(TEST_MEDIAMETADATA_STR, rv);
    START_CLOUDPC(argv[2], argv[3], TEST_MEDIAMETADATA_STR, true, rv);

    LOG_ALWAYS("\n\n==== Launching Client CCD ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        std::string alias = "MD";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_MEDIAMETADATA_STR, rv);
    }

    {
        QUERY_TARGET_OSVERSION(clientPCOSVersion, TEST_MEDIAMETADATA_STR, rv);
    }

    if (!using_remote_agent) {
        const char *testStr = "StartCCD";
        if (useCcdInClientSubdir) {
            rv = start_ccd_in_client_subdir();
        } else {
            const char *testArg[] = { testStr };
            rv = start_ccd(1, testArg);
        }
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    } else {
        START_CCD(TEST_MEDIAMETADATA_STR, rv);
    }

    UPDATE_APP_STATE(TEST_MEDIAMETADATA_STR, rv);

    START_CLIENT(argv[2], argv[3], TEST_MEDIAMETADATA_STR, true, rv);

    LOG_ALWAYS("\n\n==== Launching Client CCD 2 ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "Client", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId2);
    }

    {
        std::string alias = "Client";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_MEDIAMETADATA_STR, rv);
    }

    if (!using_remote_agent) {
        const char *testStr = "StartCCD";
        if (useCcdInClientSubdir) {
            rv = start_ccd_in_client_subdir();
        } else {
            const char *testArg[] = { testStr };
            rv = start_ccd(1, testArg);
        }
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    } else {
        START_CCD(TEST_MEDIAMETADATA_STR, rv);
    }

    START_CLIENT(argv[2], argv[3], TEST_MEDIAMETADATA_STR, true, rv);

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
    {
        std::vector<u64> deviceIds;
        u64 userId = 0;
        //u64 deviceId = 0;
        u64 deviceId[4] = {0, 0, 0, 0};

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }

        //get CloudPC deviceid
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
        rv = getDeviceId(&deviceId[cloudPCId]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        deviceIds.push_back(deviceId[cloudPCId]);
        LOG_ALWAYS("cloud pc deviceId: "FMTu64, deviceId[cloudPCId]);

        //get MD deviceid
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        rv = getDeviceId(&deviceId[clientPCId]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        deviceIds.push_back(deviceId[clientPCId]);
        LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[clientPCId]);

        //check if devices all online
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "CheckCloudPCDeviceLinkStatus", rv);
 
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "CheckMDDeviceLinkStatus", rv);
    }

#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n==== Preparing Metadata Sync (Client) ====");
    // XXX temp workaround to let the client device link up before
    // trying to access the cloudnode
    sleep(5);
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        const char *testStr = "SetupCheckMetadata";
        const char *testArg[] = { testStr };
        rv = setup_check_metadata(1, testArg);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
#endif

    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    LOG_ALWAYS("\n\n==== Testing Metadata Sync (CloudPC) ====");
    {
        std::stringstream ss;
        std::string num_music_albums_str;
        std::string num_tracks_per_music_album_str;
        std::string num_photo_albums_str;
        std::string num_photos_per_photo_album_str;

        ss.str("");
        ss << num_music_albums;
        num_music_albums_str = ss.str();

        ss.str("");
        ss << num_tracks_per_music_album;
        num_tracks_per_music_album_str = ss.str();

        ss.str("");
        ss << num_photo_albums;
        num_photo_albums_str = ss.str();

        ss.str("");
        ss << num_photos_per_photo_album;
        num_photos_per_photo_album_str = ss.str();

        const char *testStr = "CheckMetadata";
        const char *testArg[] = { testStr,
                                  "-T", // increment timestamp
                                  num_music_albums_str.c_str(), 
                                  num_tracks_per_music_album_str.c_str(), 
                                  num_photo_albums_str.c_str(), 
                                  num_photos_per_photo_album_str.c_str()};
        rv = check_metadata_generic(6, testArg, using_remote_agent);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }

    LOG_ALWAYS("\n\n==== Testing Metadata Sync (Client) ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    LOG_ALWAYS("\n\n==== List Metadata (ClientPC) ====");
    {
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, (num_photo_albums*num_photos_per_photo_album), rv, false, "");
        VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_MEDIAMETADATA_STR, (num_music_albums*num_tracks_per_music_album), rv, false, "");
    }

    {
        const char *testStr = "ResumeThumbSync_All";
        const char *testArg[] = { testStr , "All"};
        rv = mca_resume_thumb_sync_cmd(2, testArg);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    {
        const char *testStr = "CheckMetadata";
        const char *testArg[] = { testStr };
        rv = check_metadata_generic(1, testArg, using_remote_agent);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    {
        const char *testStr = "GetThumbSyncState_All_Enabled";
        std::set<ccd::SyncFeature_t> enabledList;
        enabledList.insert(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
        enabledList.insert(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
        enabledList.insert(ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);

        rv = __check_sync_enabled_list(enabledList);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    // Now test ability to disable MM thumb syncing by individual type
    {
        const char *testStr = "StopThumbSync_Photo_Only";
        const char *testArg[] = { testStr , "Photo"};
        rv = mca_stop_thumb_sync_cmd(2, testArg);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    {
        const char *testStr = "GetThumbSyncState_Music_and_Video_Enabled";
        std::set<ccd::SyncFeature_t> enabledList;
        enabledList.insert(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
        enabledList.insert(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);

        rv = __check_sync_enabled_list(enabledList);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    // Now individually disable the other two types
    {
        const char *testStr = "StopThumbSync_Music_and_Video";
        const char *testArg[] = { testStr , "Music", "Video"};
        rv = mca_stop_thumb_sync_cmd(3, testArg);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    {
        const char *testStr = "GetThumbSyncState_All_Disabled";
        std::set<ccd::SyncFeature_t> enabledList;
        enabledList.clear();

        rv = __check_sync_enabled_list(enabledList);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }

    LOG_ALWAYS("\n\n==== Testing Streaming (Client) ====");
    {
        const char *testStr = "ResumeThumbSync_Photo_and_Music";
        const char *testArg[] = { testStr , "Photo", "Music"};
        rv = mca_resume_thumb_sync_cmd(3, testArg);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    {
        const char *testStr = "ResumeThumbSync_Video";
        const char *testArg[] = { testStr , "Video"};
        rv = mca_resume_thumb_sync_cmd(2, testArg);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
    {
        const char *testStr = "GetThumbSyncState_All_Enabled";
        std::set<ccd::SyncFeature_t> enabledList;
        enabledList.insert(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
        enabledList.insert(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
        enabledList.insert(ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);

        rv = __check_sync_enabled_list(enabledList);

        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }

    VPLThread_Sleep(VPLTIME_FROM_SEC(1));
    LOG_ALWAYS("\n\n==== List Metadata (ClientPC) ====");
    {
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, (num_photo_albums*num_photos_per_photo_album), rv, false, "");
        VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_MEDIAMETADATA_STR, (num_music_albums*num_tracks_per_music_album), rv, false, "");
    }

    {
        const char *testStr0 = "CheckStreaming";
        const char *testStr1 = "-m";
        const char *testArg[] = { testStr0, testStr1 };
        rv = check_streaming_generic(1, testArg, using_remote_agent);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr0, rv);
    }

    // Client gets correct metadata after CloudPC ClearMetadata
    // Client can get metadata of 1000 jpgs
    // Client can get metadata of jpg, bmp, png, tif (various sizes)
    // Client can get metadata of 10 albums, 10 songs each
    // Client can get metadata after ClearMetadata and re-add songs
    if (full) {
        int retry = 0;
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);

        LOG_ALWAYS("\n\n==== Clear Metadata (CloudPC) ====");
        {
            const char *testStr = "ClearMetadata";
            const char *testArg[] = { testStr };
            rv = msa_delete_catalog(ARRAY_ELEMENT_COUNT(testArg), testArg);
            if(rv != 0) {
                LOG_ERROR("RegressionStreaming_ClearMetadata failed!");
            }
            CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
        }

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        LOG_ALWAYS("\n\n==== List Music Metadata (Client) ====");
        VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_MEDIAMETADATA_STR, 0, rv, false, "");
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, 0, rv, false, "");

        LOG_ALWAYS("\n\n==== Add 1000 Photo (CloudPC) ====");
    #if defined(CLOUDNODE)
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        {
            std::string dxroot;
            std::string photoSetPath;
            rv = getDxRootPath(dxroot);
            if (rv != 0) {
                LOG_ERROR("Fail to set %s root path", argv[0]);
                goto exit;
            }
            photoSetPath.assign(dxroot.c_str());
            photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet1");
            CM_CLOUDNODE_ADD_PHOTO(photoSetPath, rv);
        }
    #endif // CLOUDNODE
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
        {
            std::string dxroot;
            std::string photoSetPath;
            rv = getDxRootPath(dxroot);
            if (rv != 0) {
                LOG_ERROR("Fail to set %s root path", argv[0]);
                goto exit;
            }
            photoSetPath.assign(dxroot.c_str());
            photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet1");
            CM_ADD_PHOTO(photoSetPath, rv);
        }

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        retry = 0;
        LOG_ALWAYS("\n\n==== List Photo Metadata (Client) ====");
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, 1000, rv, false, "");

        LOG_ALWAYS("\n\n==== Add 100 music tracks(CloudPC) ====");
    #if defined(CLOUDNODE)
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
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
            CM_CLOUDNODE_ADD_MUSIC(photoSetPath, rv);
        }
    #endif // !CLOUDNODE

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
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
            CM_ADD_MUSIC(photoSetPath, rv);
        }

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        LOG_ALWAYS("\n\n==== List Music Metadata (Client) ====");
        VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_MEDIAMETADATA_STR, 100, rv, false, "");
    }


    {//test for Virtual Album
        std::string virtualAlbum0 = "newAlbum0";
        std::string virtualAlbum1 = "newAlbum1";
        std::string deleteCollection = virtualAlbum0 + "Collection";

        LOG_ALWAYS("\n\n==== test for VirtualAlbum ====\n\n");
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);

        LOG_ALWAYS("\n\n==== Clear Metadata (CloudPC) ====");
        {
            const char *testStr = "ClearMetadata";
            const char *testArg[] = { testStr };
            rv = msa_delete_catalog(ARRAY_ELEMENT_COUNT(testArg), testArg);
            if(rv != 0) {
                LOG_ERROR("RegressionStreaming_ClearMetadata failed!");
            }
            CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
        }

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        LOG_ALWAYS("\n\n==== Check Music and Photo Metadata (MD) ====");
        VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_MEDIAMETADATA_STR, 0, rv, false, "");
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, 0, rv, false, "");

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
        {
            std::string dxroot;
            std::string photoSetPath;
            rv = getDxRootPath(dxroot);
            if (rv != 0) {
                LOG_ERROR("Fail to set %s root path", argv[0]);
                goto exit;
            }
            photoSetPath.assign(dxroot.c_str());
            photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet2_nothumb");
            CM_ADD_PHOTO2ALBUM(photoSetPath, virtualAlbum0, rv);  //20 photos
            VPLThread_Sleep(VPLTIME_FROM_SEC(3));
            CM_ADD_PHOTO2ALBUM(photoSetPath, virtualAlbum1, rv); //20 photos
            VPLThread_Sleep(VPLTIME_FROM_SEC(3));
            photoSetPath.assign(dxroot.c_str());
            photoSetPath.append("/GoldenTest/TestPhotoData/PhotoTestSet4");//2 photos
            CM_ADD_PHOTO2ALBUM(photoSetPath, virtualAlbum1, rv);
        }

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        LOG_ALWAYS("\n\n==== Check Photo Metadata (MD) ====");
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, 42, rv, false, "");

        // delete one album and check if the shared files will be removed.
        // It's an error if the shared files are absent!

        std::string catalogPhoto="photo";
        LOG_ALWAYS("\n\n==== Delete %s (CloudPC) ====\n", deleteCollection.c_str());
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
        CM_DELETE_COLLECTION(catalogPhoto, deleteCollection, rv);

        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
        LOG_ALWAYS("\n\n==== Check Photo Metadata (MD) ====");
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, 22, rv, false, "");
    }

exit:
    LOG_ALWAYS("\n\n== Freeing cloud PC ==");
    if (!using_remote_agent || set_target_machine("CloudPC") < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }
    {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client ==");
    if (!using_remote_agent || set_target_machine("MD") < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client 2 ==");
    if (!using_remote_agent || set_target_machine("Client") < 0) {
        setCcdTestInstanceNum(clientPCId2);
    }
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    return rv;
}

int do_autotest_sdk_release_mediametadata(int argc, const char* argv[])
{
    return autotest_sdk_release_mediametadata_generic(argc, argv, true);
}

// Designed to ensure that the "SyncConfig deferred upload" logic (upload to ACS, then POST url)
// works and also that the "download from ACS" logic still works.
// Overview of steps for this test:
//   Run Media Server (in hybrid upload mode).
//   Wait until all media metadata is uploaded (SyncFeature status == DONE).
//   Shutdown Media Server CCD.
//   Start Client CCD.
//   Test an on-demand thumbnail download (which must come from ACS since Media Server is off).
//   Wait for SyncConfig download of all the media metadata (which must also come from ACS).
// NOTE: This test is really only meant to be launched via
//   tests/sdk_release_mediametadata_download_from_acs/Makefile.
int do_autotest_sdk_mediametadata_download_from_acs(int argc, const char* argv[])
{
    const char *TEST_MEDIAMETADATA_STR = "SdkMediaMetadataDownloadFromAcs";
    int rv = 0;
    const int CLOUD_PC_ID = 1;
    const int MD_ID = 2; // To test sync.
    const int CLIENT_PC_ID = 3; // Only to help CloudNode get set up.
    std::string mdOSVersion;
    
    //input value for check_metadata_generic
    int num_music_albums = 3;
    int num_tracks_per_music_album = 10;
    int num_photo_albums = 2;
    int num_photos_per_photo_album = 5;

    if (checkHelp(argc, argv) || (argc < 4)) {
        printf("AutoTest %s <domain> <username> <password>\n", argv[0]);
        return rv;
    }

    LOG_ALWAYS("Auto Test SDK MediaMetadata Download From ACS: "
               "Domain(%s) User(%s) Password(%s)",
               argv[1], argv[2], argv[3]);
    
    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(CLOUD_PC_ID);
    }
    CHECK_LINK_REMOTE_AGENT("CloudPC", TEST_MEDIAMETADATA_STR, rv);
    START_CCD(TEST_MEDIAMETADATA_STR, rv);
    START_CLOUDPC(argv[2], argv[3], TEST_MEDIAMETADATA_STR, true, rv);

    // We cannot put files directly on CloudNode, so we need the ClientPC to upload
    // the media via SetupCheckMetadata.
#if defined(CLOUDNODE)
    LOG_ALWAYS("\n\n==== Launching ClientPC CCD ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "Client", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(CLIENT_PC_ID);
    }
    CHECK_LINK_REMOTE_AGENT("Client", TEST_MEDIAMETADATA_STR, rv);
    START_CCD(TEST_MEDIAMETADATA_STR, rv);
    START_CLIENT(argv[2], argv[3], TEST_MEDIAMETADATA_STR, true, rv);
    
    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking CloudPC and Client device link status ==");
    {
        std::vector<u64> deviceIds;
        u64 deviceId[2] = {0, 0};

        u64 userId = 0;
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }

        //get CloudPC deviceid
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
        rv = getDeviceId(&deviceId[0]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        deviceIds.push_back(deviceId[0]);
        LOG_ALWAYS("cloud pc deviceId: "FMTu64, deviceId[0]);

        //get MD deviceid
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "Client", rv);
        rv = getDeviceId(&deviceId[1]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        deviceIds.push_back(deviceId[1]);
        LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[1]);

        //check if devices all online
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "CheckCloudPCDeviceLinkStatus", rv);
 
        SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "Client", rv);
        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "CheckClientDeviceLinkStatus", rv);
    }
    
    LOG_ALWAYS("\n\n==== Preparing Metadata Sync (Client) ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "Client", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(CLIENT_PC_ID);
    }
    {
        const char *testStr = "SetupCheckMetadata";
        const char *testArg[] = { testStr };
        rv = setup_check_metadata(1, testArg);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }
#endif

    LOG_ALWAYS("\n\n==== Testing Metadata Sync (CloudPC) ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(CLOUD_PC_ID);
    }
    {
        std::string num_music_albums_str = SSTR(num_music_albums);
        std::string num_tracks_per_music_album_str = SSTR(num_tracks_per_music_album);
        std::string num_photo_albums_str = SSTR(num_photo_albums);
        std::string num_photos_per_photo_album_str = SSTR(num_photos_per_photo_album);

        const char *testStr = "CheckMetadata";
        const char *testArg[] = { testStr,
                                  "-T",  // increment timestamp
                                  num_music_albums_str.c_str(), 
                                  num_tracks_per_music_album_str.c_str(), 
                                  num_photo_albums_str.c_str(), 
                                  num_photos_per_photo_album_str.c_str()};
        rv = check_metadata_generic(6, testArg, /*using_remote_agent (ignored)*/false);
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, testStr, rv);
    }

    // Wait for SyncConfig status to indicate that the media metadata was uploaded to ACS.
    // TODO: Technically, it could be possible for everything to be IN SYNC because SyncConfig
    //   hasn't been notified of the changes yet.  As such, it would be safer to create an
    //   EventQueue before committing to MSA so that we can confirm that files were actually
    //   uploaded.  In practice, the following approach seems to work fine.
    {
        VPLTime_t startTime = VPLTime_GetTimeStamp();
        ccd::GetSyncStateInput request;
        request.set_only_use_cache(true);
        request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD);
        request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD);
        request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD);
        request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD);
        const int numSyncFeatures = request.get_sync_states_for_features_size();
        while(1) {
            ccd::GetSyncStateOutput response;
            rv = CCDIGetSyncState(request, response);
            if (rv != 0) {
                LOG_ERROR("CCDIGetSyncState failed: %d", rv);
                break;
            }
            VPLTime_t elapsed = VPLTime_GetTimeStamp() - startTime;
            LOG_ALWAYS("Waiting for metadata to be uploaded (%d SyncFeatures, "FMTu64"ms elapsed):",
                    numSyncFeatures,
                    VPLTime_ToMillisec(elapsed));
            bool allInSync = true;
            for (int i = 0; i < numSyncFeatures; i++) {
                ccd::FeatureSyncStateType_t status = response.feature_sync_state_summary(i).status();
                if (status != ccd::CCD_FEATURE_STATE_IN_SYNC) {
                    allInSync = false;
                    LOG_ALWAYS("%s: %s",
                            ccd::SyncFeature_t_Name(request.get_sync_states_for_features(i)).c_str(),
                            ccd::FeatureSyncStateType_t_Name(status).c_str());
                }
            }
            if (allInSync) {
                rv = 0;
                break;
            }
            const int WAIT_FOR_SYNC_TIMEOUT_SEC = 40;
            if (elapsed > VPLTime_FromSec(WAIT_FOR_SYNC_TIMEOUT_SEC)) {
                LOG_ERROR("Timeout (%ds) waiting for uploads!", WAIT_FOR_SYNC_TIMEOUT_SEC);
                rv = -1;
                break;
            }
            VPLThread_Sleep(VPLTime_FromSec(1));
        }
        CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "WaitForMetadataUpload", rv);
    }

    LOG_ALWAYS("\n\n==== Shutdown CloudPC ====");
    {
        if (set_target_machine("CloudPC") < 0) {
            setCcdTestInstanceNum(CLOUD_PC_ID);
        }
        // Note: Do NOT unlink the CloudPC here.
        {
            const char *testArg[] = { "StopCCD" };
            stop_ccd_soft(1, testArg);
        }
    }

    LOG_ALWAYS("\n\n==== Launching Client Device CCD ====");
    SET_TARGET_MACHINE(TEST_MEDIAMETADATA_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(MD_ID);
    }
    CHECK_LINK_REMOTE_AGENT("MD", TEST_MEDIAMETADATA_STR, rv);
    QUERY_TARGET_OSVERSION(mdOSVersion, TEST_MEDIAMETADATA_STR, rv);
    START_CCD(TEST_MEDIAMETADATA_STR, rv);
    UPDATE_APP_STATE(TEST_MEDIAMETADATA_STR, rv);
    START_CLIENT(argv[2], argv[3], TEST_MEDIAMETADATA_STR, true, rv);

    // Test "on-demand thumbnail download" from ACS.
    {
        LOG_ALWAYS("\n\n==== Download thumbnail on demand (Client Device) ====");
        std::string test_thumb_url;
        {
            std::string test_url;
            rv = mca_diag(METADATA_PHOTO_TEST_THUMB_FILE,
                    /*out*/test_url, /*out*/test_thumb_url,
                    false /*CloudPC is expected to be offline.*/, true, /*using_remote_agent (ignored)*/false);
            CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "Get_test_thumbnail_URL", rv);
        }
        LOG_ALWAYS("test thumbnail URL is \"%s\"", test_thumb_url.c_str());
        {
            // TODO: bug 16432: Prepending currDir is a workaround for vplex file functions
            // not supporting relative paths on Windows.
            std::string currDir;
            rv = getCurDir(currDir);
            CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "getCurDir", rv);

            HttpAgent* agent = getHttpAgent();
            ON_BLOCK_EXIT(deleteObj<HttpAgent>, agent);

            std::string filenameStr = SSTR(currDir << "/checkThumbOnDemandDl_" << testInstanceNum << ".out");

            std::string fileResponse;
            rv = agent->get_response_back(test_thumb_url, 0, 0,
                    NULL, 0,
                    /*out*/ fileResponse);
            CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "Download_thumbnail_on_demand1", rv);
            rv = Util_WriteFile(filenameStr.c_str(), fileResponse.data(), fileResponse.size());
            CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "Download_thumbnail_on_demand2", rv);

            std::string refResponse = SSTR(currDir << "/"METADATA_PHOTO_TEST_THUMB_FILE);
            if(!isEqualFileContents(filenameStr.c_str(), refResponse.c_str())) {
                LOG_ERROR("Downloaded %s is different from reference response %s",
                        filenameStr.c_str(), refResponse.c_str());
                { // Write the bad file to DX_FAILURE_DIRECTORY so that it can be analyzed later.
                    std::string failFilenameStr = SSTR(currDir << "/"DX_FAILURE_DIRECTORY"/checkThumbOnDemandDl_" << testInstanceNum << ".out");
                    LOG_ALWAYS("Writing thumbnail to \"%s\"", failFilenameStr.c_str());
                    /*ignore result*/ Util_WriteFile(failFilenameStr.c_str(), fileResponse.data(), fileResponse.size());
                }
                rv = -1;
            }
            CHECK_AND_PRINT_RESULT(TEST_MEDIAMETADATA_STR, "Download_thumbnail_on_demand3", rv);
        }
    }

    LOG_ALWAYS("\n\n==== List Metadata (Client Device) ====");
    {
        VERIFY_CLOUDMEDIA_PHOTO_NUM_FUNC(TEST_MEDIAMETADATA_STR, (num_photo_albums*num_photos_per_photo_album), rv, false, "");
        VERIFY_CLOUDMEDIA_MUSIC_NUM_FUNC(TEST_MEDIAMETADATA_STR, (num_music_albums*num_tracks_per_music_album), rv, false, "");
    }

exit:
    // To reduce test time, we don't unlink the CloudPC.
    // This should be okay, since tests should start with a fresh account each time anyway.

    // TODO: if we failed before the "Shutdown CloudPC" step, the CloudPC will still be running.
    // We might want to shut it down here.
#if 0
    LOG_ALWAYS("\n\n==== Freeing CloudPC ====");
    {
        if (set_target_machine("CloudPC") < 0) {
            setCcdTestInstanceNum(CLOUD_PC_ID);
        }
        {
            const char *testArg[] = { "StopCCD" };
            stop_ccd_soft(1, testArg);
        }
    }
#endif

    LOG_ALWAYS("\n\n== Freeing client ==");
    if (set_target_machine("MD") < 0) {
        setCcdTestInstanceNum(MD_ID);
    }
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }
    if (isWindows(mdOSVersion) || mdOSVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client 2 ==");
    if (set_target_machine("Client") < 0) {
        setCcdTestInstanceNum(CLIENT_PC_ID);
    }
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    return rv;;
}
