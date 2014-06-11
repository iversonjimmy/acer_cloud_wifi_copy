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
#include "cr_test.hpp"
#include "TargetDevice.hpp"
#include "EventQueue.hpp"

#include "picstreamhttp.hpp"
#include "autotest_picstreamIndexOnly.hpp"

#include "cJSON2.h"

#include <sstream>
#include <string>
#include <queue>
#define CHECK_UPLOADED_PHOTO_NUM(tc_name, check_photo_dir, prevNum, curNum, expected_add_num, retry, rc, expected_to_fail, bug) \
        CHECK_UPLOADED_PHOTO_NUM_FUNC(get_cr_photo_count, tc_name, check_photo_dir, prevNum, curNum, expected_add_num, retry, rc, expected_to_fail, bug)

#ifdef CLOUDNODE
#define CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(tc_name, check_photo_dir, prevNum, curNum, expected_add_num, retry, rc, expected_to_fail, bug) \
        CHECK_UPLOADED_PHOTO_NUM_FUNC(get_cr_photo_count_cloudnode, tc_name, check_photo_dir, prevNum, curNum, expected_add_num, retry, rc, expected_to_fail, bug)
#else
#define CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(tc_name, check_photo_dir, prevNum, curNum, expected_add_num, retry, rc, expected_to_fail, bug) \
        CHECK_UPLOADED_PHOTO_NUM_FUNC(get_cr_photo_count, tc_name, check_photo_dir, prevNum, curNum, expected_add_num, retry, rc, expected_to_fail, bug)
#endif

#define CHECK_UPLOADED_PHOTO_NUM_FUNC(func, tc_name, check_photo_dir, prevNum, curNum, expected_add_num, retry, rc, expected_to_fail, bug) \
    do { \
        int i = 0; \
        while (i++ < retry) { \
            func(check_photo_dir, curNum); \
            if(curNum == prevNum + expected_add_num) { \
                LOG_ALWAYS("Got expected photo number!!!!!!!!!!!!!!!!!"); \
                rc = 0; \
                break; \
            } \
            else { \
                rc = -1; \
                VPLThread_Sleep(VPLTIME_FROM_SEC(1)); \
            } \
        } \
        LOG_ALWAYS("check_photo_dir = %s", check_photo_dir.c_str()); \
        LOG_ALWAYS("prevNum = %d, curNum = %d, expectedNum = %d", prevNum, curNum, prevNum + expected_add_num); \
        if (expected_to_fail) { \
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(tc_name, "CheckUploadedPhotoNum", rc, bug); \
        } \
        else { \
            CHECK_AND_PRINT_RESULT(tc_name, "CheckUploadedPhotoNum", rc); \
        } \
    } while (0)

#define PS_TEMPLATE_OP(op, num, skip, retry, arg1, arg2, arg3, arg4, arg5, rc) \
    do { \
        const char *testArgs[] = { "PicStream", op, arg1, arg2, arg3, arg4, arg5 }; \
        rc = picstream_dispatch(num, testArgs); \
        if (rc < 0 && retry) { \
            LOG_INFO("Retry in 10 seconds!"); \
            VPLThread_Sleep(VPLTime_FromSec(10)); \
            rc = picstream_dispatch(num, testArgs); \
        } \
        if (!skip) { \
            CHECK_AND_PRINT_RESULT("PicStream", op, rc); \
        } \
    } while (0)

#define PS_HTTP_TEMPLATE_OP(op, num, skip, retry, arg1, arg2, arg3, arg4, arg5, resp, rc) \
do { \
    int retryTimes=retry; \
    const char *testArgs[] = { "PicStreamHttp", op, arg1, arg2, arg3, arg4, arg5 }; \
    rc = dispatch_picstreamhttp_cmd_with_response(num, testArgs, resp); \
    while (rc < 0 && retryTimes-- > 0) { \
        LOG_INFO("Retry in 5 seconds!"); \
        VPLThread_Sleep(VPLTime_FromSec(5)); \
        rc = dispatch_picstreamhttp_cmd_with_response(num, testArgs, resp); \
    } \
    if (!skip) { \
        CHECK_AND_PRINT_RESULT("PicStreamHttp", op, rc); \
    } \
} while (0)

#define PS_CLEAR(arg1, rc)                 PS_TEMPLATE_OP("Clear",           3, false, false, arg1.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_SET_ENABLE_UPLOAD(rc)           PS_TEMPLATE_OP("SetEnableUpload", 3, false, false, "true",       NULL, NULL, NULL, NULL, rc)
#define PS_SET_DISABLE_UPLOAD(rc)          PS_TEMPLATE_OP("SetEnableUpload", 3, false, false, "false",      NULL, NULL, NULL, NULL, rc)
#define PS_SET_FULLRES_DIR(path, rc)       PS_TEMPLATE_OP("SetFullResDir",   3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_REMOVE_FULLRES_DIR(path, rc)    PS_TEMPLATE_OP("SetFullResDir",   4, false, false, "-c", path.c_str(), NULL, NULL, NULL, rc)
#define PS_STATUS(skip, rc)                PS_TEMPLATE_OP("Status",          2, skip,  false, NULL,         NULL, NULL, NULL, NULL, rc)
#define PS_TRIGGER_UPLOAD_DIR(path, rc)    PS_TEMPLATE_OP("TriggerUploadDir",3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_ADD_UPLOAD_DIRS(path, rc)       PS_TEMPLATE_OP("UploadDirsAdd",   3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_RM_UPLOAD_DIRS(path, rc)        PS_TEMPLATE_OP("UploadDirsRm",    3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_JPG(num, skip, rc) PS_TEMPLATE_OP("UploadPhoto",     3, skip,  false, num,          NULL, NULL, NULL, NULL, rc)
#define PS_DOWNLOAD(skip, title, compId, type, dst, resp, rc) PS_HTTP_TEMPLATE_OP("Download", 6, skip, 3, title.c_str(), compId.c_str(), type, dst.c_str(), NULL, resp, rc)

int do_autotest_sdk_release_picstream_index_only(int argc, const char* argv[]) {

    const char *TEST_PICSTREAM_STR = "SdkPicStreamIndexOnlyRelease";
    int cloudPCId = 1;
    int clientPCId = 2;
    u32 crUpPrevNum = 0;
    u32 crUpCurNum = 0;
    u64 userId = 0;
    u64 datasetId = 0;
    std::string dxTempFolder;
    std::string clientPicstream_fullresupload;
    std::string clientPicstream_dod; //download on damend path
    ccd::GetSyncStateInput syncStateIn;
    ccd::GetSyncStateOutput syncStateOut;
    std::string pathSeparator;
    std::string osVersion;
    std::string currDir;
    const char *photoNum = "10";
    int UploadPhotoNum = 10;

    int rv = 0;
    bool full = false;
    ccd::ListLinkedDevicesOutput listSnOut;
    ccd::ListLinkedDevicesInput request;
    TargetDevice *target = NULL;

    if (argc == 5 && (strcmp(argv[4], "-f") == 0 || strcmp(argv[4], "--fulltest") == 0) ) {
        full = true;
    }

    if (checkHelp(argc, argv) || (argc < 4) || (argc == 5 && !full)) {
        printf("AutoTest %s <domain> <username> <password> [<fulltest>(-f/--fulltest)]\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Testing AutoTest SDK Release PicStreamIndexOnly_Advanced: Domain(%s) User(%s) Password(%s)", argv[1], argv[2], argv[3]);

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    rv = getCurDir(currDir);
    if (rv < 0) {
        LOG_ERROR("Cannot get current directory");
        return -1;
    }

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
    SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string alias = "CloudPC";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_PICSTREAM_STR, rv);
    }

    START_CCD(TEST_PICSTREAM_STR, rv);
    START_CLOUDPC(argv[2], argv[3], TEST_PICSTREAM_STR, true, rv);

    target = getTargetDevice();
    rv = target->getWorkDir(dxTempFolder);
    if (rv != 0) {
        LOG_ERROR("Failed to get workdir on target device: %d", rv);
        return -1;
    }
    rv = target->getDirectorySeparator(pathSeparator);
    if (rv != 0) {
        LOG_ERROR("Failed to get directory separator on target device: %d", rv);
        return -1;
    }
    LOG_ALWAYS("dxTempFolder: %s", dxTempFolder.c_str());
    
    LOG_ALWAYS("\n\n==== Launching Client CCD ====\n\n");
    SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
    if(rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        std::string alias = "MD";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_PICSTREAM_STR, rv);
    }

    {
        QUERY_TARGET_OSVERSION(osVersion, TEST_PICSTREAM_STR, rv);
    }

    {
        START_CCD(TEST_PICSTREAM_STR, rv);
    }

    if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
        UPDATE_APP_STATE(TEST_PICSTREAM_STR, rv);
    }

    START_CLIENT(argv[2], argv[3], TEST_PICSTREAM_STR, true, rv);

    target = getTargetDevice();
    rv = target->getWorkDir(dxTempFolder);
    if (rv != 0) {
        LOG_ERROR("Failed to get workdir on target device: %d", rv);
        return -1;
    }
    rv = target->getDirectorySeparator(pathSeparator);
    if (rv != 0) {
        LOG_ERROR("Failed to get directory separator on target device: %d", rv);
        return -1;
    }
    
    clientPicstream_fullresupload = dxTempFolder;
    clientPicstream_dod = dxTempFolder;
    clientPicstream_fullresupload = clientPicstream_fullresupload.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_upload_fullres");
    clientPicstream_dod = clientPicstream_dod.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("dod");

    // Makes sure that the paths exist and are clean.
    rv = target->removeDirRecursive(clientPicstream_fullresupload);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_fullresupload.c_str());
    }    
    rv = target->removeDirRecursive(clientPicstream_dod);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_dod.c_str());
    }
    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(2000));
    
    rv = target->createDir(clientPicstream_fullresupload.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_fullresupload.c_str());
    }   
    rv = target->createDir(clientPicstream_dod.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_dod.c_str());
    }

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
    {
        std::vector<u64> deviceIds;
        u64 cloudPCDeviceId = 0;
        u64 clientPCDeviceId = 0;
        const char *testCloudStr = "CheckCloudPCDeviceLinkStatus";
        const char *testMDStr = "CheckMDDeviceLinkStatus";

        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testCloudStr, rv);
        }

        rv = getDeviceId(&cloudPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get CloudPC device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testCloudStr, rv);
        }

        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        rv = getDeviceId(&clientPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get MD device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testCloudStr, rv);
        }

        deviceIds.push_back(cloudPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, cloudPCDeviceId);
        deviceIds.push_back(clientPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, clientPCDeviceId);

        rv = wait_for_devices_to_be_online_by_alias(TEST_PICSTREAM_STR, "CloudPC", cloudPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testCloudStr, rv);
        rv = wait_for_devices_to_be_online_by_alias(TEST_PICSTREAM_STR, "MD", clientPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testMDStr, rv);

    }

    LOG_ALWAYS("\n\n==== PicStream: Get userId and PicStream datasetId ====");
    {
        const char *testStr = "GetUserId";
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
        }
        LOG_ALWAYS("userId = "FMTu64, userId);
        CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
    }

    {
        const char *testStr = "GetDatasetId";
        rv = getDatasetId(userId, "PicStream", datasetId);
        if (rv != 0) {
            LOG_ERROR("Fail to get dataset id:%d", rv);
        }
        LOG_ALWAYS("datasetId = "FMTu64, datasetId);
        CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
    }

    LOG_ALWAYS("\n\n==== Upload photos (MD)====\n\n");
    {
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        std::string upload_dir;
      
        TEST_PICSTREAM_STR = "SdkPicStreamAdvancedRelease_UploadPhoto";
        upload_dir = clientPicstream_fullresupload;

        PS_STATUS(false, rv);

        syncStateIn.set_get_is_camera_roll_upload_enabled(true);
        syncStateIn.set_get_camera_roll_upload_dirs(true);
        syncStateIn.set_get_camera_roll_download_dirs(true);
        rv = CCDIGetSyncState(syncStateIn, syncStateOut);
        if(rv != 0)
        {
            LOG_ERROR("CCDIGetSyncState:%d, CRU_Enabled:%d",
                      rv,
                      syncStateOut.has_is_camera_roll_upload_enabled());
            return rv;
        }
        else {
            const char *testStr = "CheckCRUEnabledStatus";
            if (syncStateOut.has_is_camera_roll_upload_enabled() && 
                syncStateOut.is_camera_roll_upload_enabled()) {
                LOG_ERROR("Camera roll upload enabled, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            if (syncStateOut.camera_roll_upload_dirs_size() != 0) {
                testStr = "CheckUploadDirsSize";
                LOG_ERROR("Camera roll upload dirs are not clean, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            if (syncStateOut.camera_roll_full_res_download_dirs_size() != 0) {
                testStr = "CheckFullresDownloadDirsSize";
                LOG_ERROR("Camera roll upload dirs are not clean, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
        }

        LOG_ALWAYS("\n==== Add Upload Dirs (MD) ====");
        PS_ADD_UPLOAD_DIRS(upload_dir, rv);

        LOG_ALWAYS("\n==== Clear Both crUp and crDown (MD) ====");
        {
            std::string arg1 = "both";
            PS_CLEAR(arg1, rv);
        }
        get_cr_photo_count(upload_dir, crUpPrevNum);
        LOG_ALWAYS("crUpPrevNum = %d", crUpPrevNum);

        rv = CCDIGetSyncState(syncStateIn, syncStateOut);
        if(rv != 0)
        {
            LOG_ERROR("CCDIGetSyncState:%d, CRU_Enabled:%d",
                      rv,
                      syncStateOut.has_is_camera_roll_upload_enabled());
            return rv;
        }
        else {
            const char *testStr = "CheckCRUEnabledStatus";
            if (syncStateOut.has_is_camera_roll_upload_enabled() && 
                syncStateOut.is_camera_roll_upload_enabled()) {
                LOG_ERROR("Camera roll upload enabled, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            if (full && syncStateOut.camera_roll_upload_dirs_size() != 1) {
                testStr = "CheckUploadDirsSize";
                LOG_ERROR("Camera roll upload dirs size is wrong, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
        }

        LOG_ALWAYS("\n==== Set Enable_Upload TRUE (MD) ====");
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        PS_SET_ENABLE_UPLOAD(rv);
        VPLThread_Sleep(VPLTIME_FROM_SEC(1));

        LOG_ALWAYS("\n==== Upload %d photos from UploadDir (MD) ====", UploadPhotoNum);
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        {
            PS_UPLOAD_PHOTO_JPG(photoNum, true, rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "UploadPhoto", rv);
        }

        if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
            PS_TRIGGER_UPLOAD_DIR(upload_dir, rv);
        }

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir, crUpPrevNum, crUpCurNum, UploadPhotoNum, 10, rv, false, "");
        PS_STATUS(false, rv);

    }

    LOG_ALWAYS("\n\n==== Download on Demand (MD)====\n\n");
    {//test for download on demand
        int repeat = 0, cnt = 0;
        std::queue<std::string> albums;
        ccd::CCDIQueryPicStreamObjectsInput req;
        ccd::CCDIQueryPicStreamObjectsOutput resp;
        std::queue<std::string> titles;
        std::queue<std::string> compIds;
        std::string title, compId,dst_path, httpResp;
            
        TEST_PICSTREAM_STR = "SdkPicStreamAdvancedRelease_DownloadOnDemand";
        repeat = (full) ? 6 : 2;
        LOG_ALWAYS("\n\n==== Download %d photos on Demand from infra (MD) ====\n\n", repeat);

        {//wait until at least repeat low resolution are available.
            int times = 0;
            while(1){
                std::queue<std::string> empty1, empty2;
                std::swap(empty1,titles);
                std::swap(empty2,compIds);

                req.Clear();
                req.set_filter_field(ccd::PICSTREAM_QUERY_ITEM);
                req.clear_search_field();
                rv = CCDIQueryPicStreamObjects(req, resp);
                if(rv != 0) {
                    LOG_ERROR("CCDIQueryPicStreamObjects [PICSTREAM_QUERY_ITEM]: %d ", rv);
                    return rv;
                }
                if(resp.mutable_content_objects()->size() > repeat ) {

                    google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject >::iterator it;
                    for (it = resp.mutable_content_objects()->begin(); it != resp.mutable_content_objects()->end(); ++it) {

                        if (it->has_low_res_url() && it->low_res_url().length() > 0){
                            titles.push(it->mutable_pcdo()->mutable_picstream_item()->title());
                            compIds.push(it->mutable_pcdo()->comp_id());
                        }
                    }

                    if(titles.size() > (u32) repeat && resp.mutable_content_objects()->size() == UploadPhotoNum) {
                        LOG_ALWAYS(FMTu_size_t" Low Resolution photos are ready for download test.", titles.size());
                        break;
                    }
                }
                if (times++ > 30) {
                    if (titles.size() < (u32) repeat) {
                        LOG_ERROR("Wait for low resolution photos TIMEOUT!");
                    }
                    if (resp.mutable_content_objects()->size() < UploadPhotoNum) {
                        LOG_ERROR(FMTu_size_t" items in DB.", resp.mutable_content_objects()->size());
                    }
                    rv = -1;
                    CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "PhotoNum", rv);
                }
                LOG_ALWAYS(FMTu_size_t" items in DB.", resp.mutable_content_objects()->size());
                VPLThread_Sleep(VPLTIME_FROM_SEC(3));
            }
        }

        u32 downlowdPreCnt = 0;
        u32 downlowdCurCnt = 0;
        for(cnt = 0; cnt < repeat; cnt++) {
            title = titles.front();
            compId = compIds.front();
            LOG_ALWAYS("Download %s compId: %s", title.c_str(), compId.c_str());

            dst_path = clientPicstream_dod + "/full_" + title;
            PS_DOWNLOAD(false, title, compId, "0" , dst_path, httpResp, rv);
            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, clientPicstream_dod, downlowdPreCnt, downlowdCurCnt, 1, 5, rv, false, "");
            downlowdPreCnt = downlowdCurCnt;

            dst_path = clientPicstream_dod + "/low_" + title;
            PS_DOWNLOAD(false, title, compId, "1", dst_path, httpResp, rv);
            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, clientPicstream_dod, downlowdPreCnt, downlowdCurCnt, 1, 5, rv, false, "");
            downlowdPreCnt = downlowdCurCnt;

            dst_path = clientPicstream_dod + "/thumb_" + title;
            PS_DOWNLOAD(true, title, compId, "2", dst_path, httpResp, rv);
            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, clientPicstream_dod, downlowdPreCnt, downlowdCurCnt, 1, 5, rv, false, "");
            downlowdPreCnt = downlowdCurCnt;

            titles.pop();
            compIds.pop();
            titles.push(title);
            compIds.push(compId);
        }

    }// test for download on demand

exit:
    LOG_ALWAYS("\n\n== Freeing cloud PC ==");
    set_target_machine("CloudPC");
    {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client ==");
    if(set_target_machine("MD") < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    if (isWindows(osVersion) || osVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    if(target != NULL)
        delete target;
    return rv;
}
