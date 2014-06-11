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
#include "autotest_picstream.hpp"

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
    const char *testArgs[] = { "PicStreamHttp", op, arg1, arg2, arg3, arg4, arg5 }; \
    rc = dispatch_picstreamhttp_cmd_with_response(num, testArgs, resp); \
    if (rc < 0 && retry) { \
        LOG_INFO("Retry in 10 seconds!"); \
        VPLThread_Sleep(VPLTime_FromSec(10)); \
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
#define PS_SET_LOWRES_DIR(path, rc)        PS_TEMPLATE_OP("SetLowResDir",    3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_REMOVE_LOWRES_DIR(path, rc)     PS_TEMPLATE_OP("SetLowResDir",    4, false, false, "-c", path.c_str(), NULL, NULL, NULL, rc)
#define PS_STATUS(skip, rc)                PS_TEMPLATE_OP("Status",          2, skip,  false, NULL,         NULL, NULL, NULL, NULL, rc)
#define PS_TRIGGER_UPLOAD_DIR(path, rc)    PS_TEMPLATE_OP("TriggerUploadDir",3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_ADD_UPLOAD_DIRS(path, rc)       PS_TEMPLATE_OP("UploadDirsAdd",   3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_RM_UPLOAD_DIRS(path, rc)        PS_TEMPLATE_OP("UploadDirsRm",    3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_JPG(num, skip, rc) PS_TEMPLATE_OP("UploadPhoto",     3, skip,  false, num,          NULL, NULL, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_JPEG(num, skip, rc) PS_TEMPLATE_OP("UploadPhoto",     5, skip,  false, "-e", "jpeg", num, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_TIF(num, skip, rc)  PS_TEMPLATE_OP("UploadPhoto",     5, skip,  false, "-e", "tif",  num, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_TIFF(num, skip, rc) PS_TEMPLATE_OP("UploadPhoto",     5, skip,  false, "-e", "tiff", num, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_PNG(num, skip, rc)  PS_TEMPLATE_OP("UploadPhoto",     5, skip,  false, "-e", "png",  num, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_BMP(num, skip, rc)  PS_TEMPLATE_OP("UploadPhoto",     5, skip,  false, "-e", "bmp",  num, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_FRM_DIR(num, index, skip, rc)       PS_TEMPLATE_OP("UploadPhoto", 5, skip,  false, "-i", index, num, NULL, NULL, rc)
#define PS_UPLOAD_PHOTO_JPEG_FRM_DIR(num, index, skip, rc)  PS_TEMPLATE_OP("UploadPhoto", 7, skip,  false, "-i", index, "-e", "jpeg", num, rc)
#define PS_TRIGGER_UPLOAD_DIR_SKIP(path, skip, rc)    PS_TEMPLATE_OP("TriggerUploadDir",3, skip, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_SET_THUMBNAIL_DIR(path, rc)     PS_TEMPLATE_OP("SetThumbDir", 3, false, false, path.c_str(), NULL, NULL, NULL, NULL, rc)
#define PS_REMOVE_THUMBNAIL_DIR(path, rc)  PS_TEMPLATE_OP("SetThumbDir", 4, false, false, "-c", path.c_str(), NULL, NULL, NULL, rc)
#define PS_DOWNLOAD(skip, title, compId, type, dst, resp, rc) PS_HTTP_TEMPLATE_OP("Download",    6, skip, false, title.c_str(), compId.c_str(), type, dst.c_str(), NULL, resp, rc)
#define PS_DELETE(title, compId, resp, rc)                    PS_HTTP_TEMPLATE_OP("Delete",      4, false, false, title.c_str(), compId.c_str(), NULL, NULL, NULL, resp, rc)
#define PS_GETFILEINFO(albumName, type, resp, rc)             PS_HTTP_TEMPLATE_OP("GetFileInfo", 4, false, false, albumName.c_str(), type, NULL, NULL, NULL, resp, rc)
#define PS_SET_ENABLE_GLOBAL_DELETE(rc)           PS_TEMPLATE_OP("SetEnableGlobalDelete", 3, false, false, "true",       NULL, NULL, NULL, NULL, rc)
#define PS_SET_DISABLE_GLOBAL_DELETE(rc)          PS_TEMPLATE_OP("SetEnableGlobalDelete", 3, false, false, "false",      NULL, NULL, NULL, NULL, rc)

static int check_picstream_download_visitor(const ccd::CcdiEvent &_event, void *_ctx)
{
    LOG_ALWAYS("\n== check_picstream_download_visitor: %s", _event.DebugString().c_str());
    check_event_visitor_ctx *ctx = (check_event_visitor_ctx*)_ctx;

    int rv;
    ccd::GetSyncStateInput syncStateIn;
    ccd::GetSyncStateOutput syncStateOut;
    
    if(ctx->event.has_sync_feature_status_change())
    {
        if (!_event.has_sync_feature_status_change())
        {
            LOG_ALWAYS("No has_sync_feature_status_change!");
            return 0;
        }
        
        if (!ctx->event.has_sync_feature_status_change()) {
            LOG_ALWAYS("No has_sync_feature_status_change in ctx!");
            return 0;
        }
        
        const ccd::EventSyncFeatureStatusChange &event = _event.sync_feature_status_change();

        if (event.status().status() == ccd::CCD_FEATURE_STATE_SYNCING && 
            (event.feature() == ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES ||
             event.feature() == ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES ||
             event.feature() == ccd::SYNC_FEATURE_PICSTREAM_UPLOAD ||
             event.feature() == ccd::SYNC_FEATURE_PICSTREAM_DELETION)) {
            ctx->count++;
            syncStateIn.set_user_id(ctx->userid);
            syncStateIn.add_get_sync_states_for_features(event.feature());
            rv = CCDIGetSyncState(syncStateIn, syncStateOut);
                               
            LOG_ALWAYS("Get Sync Feature Status Change Event, feature = %d, count = %d, pending jobs = %d, failed jobs = %d.", event.feature(), ctx->count, syncStateOut.feature_sync_state_summary(0).pending_files(), syncStateOut.feature_sync_state_summary(0).failed_files());
        }
        else if (event.status().status() == ccd::CCD_FEATURE_STATE_IN_SYNC || event.status().status() == ccd::CCD_FEATURE_STATE_OUT_OF_SYNC) {
            // The ctx->count stop number should be twice of the upload photos. (Upload N photos and Download N photos)
            // In current tests, we upload 3 photos.
            if (ctx->count == ctx->expected_count) {
                LOG_ALWAYS("PicStream Sync test done. Got the event! ");
                ctx->done = true;
            }
        }
    }

    return 0;
}

static VPLThread_return_t listen(VPLThread_arg_t arg)
{
    check_event_visitor_ctx *ctx = (check_event_visitor_ctx*)arg;
    set_target_machine("MD");

    int rv = -1;
    int checkTimeout = 160;
    int secondsLeft = checkTimeout;
    EventQueue eq;

    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end = start + VPLTime_FromSec(secondsLeft);
    VPLTime_t now;

    ctx->count = 0;

    VPLSem_Post(&(ctx->sem));

    while ((now = VPLTime_GetTimeStamp()) < end && !ctx->done) {
        eq.visit(0, (s32)(VPLTime_ToMillisec(VPLTime_DiffClamp(end, now))), check_picstream_download_visitor, (void*)ctx);
    }
    if (!ctx->done) {
        LOG_ERROR("task didn't complete within %d seconds", checkTimeout);
    }

    return (VPLThread_return_t)rv;
}

#define CREATE_PICSTREAM_EVENT_VISIT_THREAD(testStr) \
    BEGIN_MULTI_STATEMENT_MACRO \
    VPLThread_t event_thread; \
    rv = VPLThread_Create(&event_thread, listen, (VPLThread_arg_t)&ctx, NULL, "listen"); \
    if (rv != VPL_OK) { \
        LOG_ERROR("Failed to spawn event listen thread: %d", rv); \
    } \
    VPLSem_Wait(&(ctx.sem)); \
    CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv); \
    END_MULTI_STATEMENT_MACRO


#define CHECK_PICSTREAM_EVENT_VISIT_RESULT(testStr, timeout) \
    BEGIN_MULTI_STATEMENT_MACRO \
    int retry = 0; \
    while(!ctx.done && retry++ < timeout) \
        VPLThread_Sleep(VPLTIME_FROM_SEC(1)); \
    if (!ctx.done) { \
        LOG_ERROR("picstream_sync didn't complete within %d seconds", timeout); \
        CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_PICSTREAM_STR, testStr, rv, "9597"); \
    } \
    END_MULTI_STATEMENT_MACRO

int do_autotest_sdk_release_picstream(int argc, const char* argv[]) {

    const char *TEST_PICSTREAM_STR = "SdkPicStreamRelease";
    int cloudPCId = 1;
    int clientPCId = 2;
    int retry = 0;
    int tmp_count = 0;
    u32 crUpPrevNum = 0;
    u32 crDownPrevNum = 0;
    u32 crUpCurNum = 0;
    u32 crDownCurNum = 0;
    u32 cloudDownPrevNum = 0;
    u32 cloudDownCurNum = 0;
    u32 clientUpPrevNum_dir2 = 0;
    u32 clientUpCurNum_dir2 = 0;
    u64 userId = 0;
    u64 datasetId = 0;
    std::string dxTempFolder;
    std::string clientPicstream_fullresupload;
    std::string clientPicstream_fullresupload2;
    std::string clientPicstream_fullresdownload;
    std::string clientPicstream_lowresupload;
    std::string clientPicstream_lowresupload2;
    std::string clientPicstream_lowresdownload;
    std::string clientPicstream_thumbupload;
    std::string clientPicstream_thumbupload2;
    std::string clientPicstream_thumbdownload;
    std::string cloudPicstream_fulldown;
    std::string cloudPicstream_lowdown;
    std::string cloudPicstream_thumbdown;
    ccd::GetSyncStateInput syncStateIn;
    ccd::GetSyncStateOutput syncStateOut;
    std::string pathSeparator;
    std::string osVersion;
    std::string currDir;
    std::string src_non_supported_file;
    std::string dst_non_supported_file;

    enum { FULL_RES = 0, LOW_RES, THUMBNAIL, LAST };
    int rv = 0;
    int i = 0;
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

    LOG_ALWAYS("Testing AutoTest SDK Release PicStream_Advanced: Domain(%s) User(%s) Password(%s)", argv[1], argv[2], argv[3]);

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
    src_non_supported_file = currDir + "/" + CLOUDDOC_DOCX_FILE;
    VPLFS_stat_t statBuf;
    rv = VPLFS_Stat(src_non_supported_file.c_str(), &statBuf);
    if(rv != 0) {
        LOG_ERROR("Test file inaccessible%d:%s", rv, src_non_supported_file.c_str());
        return -1;
    }
    LOG_ALWAYS("src_non_supported_file: %s", src_non_supported_file.c_str());

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
    cloudPicstream_fulldown = dxTempFolder;
    cloudPicstream_lowdown = dxTempFolder;
    cloudPicstream_thumbdown = dxTempFolder;
    cloudPicstream_fulldown = cloudPicstream_fulldown.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_cloudpc")
        .append(pathSeparator).append("pic_fullres");
    cloudPicstream_lowdown = cloudPicstream_lowdown.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_cloudpc")
        .append(pathSeparator).append("pic_lowres");
    cloudPicstream_thumbdown = cloudPicstream_thumbdown.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_cloudpc")
        .append(pathSeparator).append("pic_thumb");
    rv = target->removeDirRecursive(cloudPicstream_fulldown);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", cloudPicstream_fulldown.c_str());
    }
    rv = target->removeDirRecursive(cloudPicstream_lowdown);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", cloudPicstream_lowdown.c_str());
    }
    rv = target->removeDirRecursive(cloudPicstream_thumbdown);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", cloudPicstream_thumbdown.c_str());
    }
    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(2000));
    rv = target->createDir(cloudPicstream_fulldown.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", cloudPicstream_fulldown.c_str());
    }
    rv = target->createDir(cloudPicstream_lowdown.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", cloudPicstream_lowdown.c_str());
    }
    rv = target->createDir(cloudPicstream_thumbdown.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", cloudPicstream_thumbdown.c_str());
    }

    LOG_ALWAYS("\n\n==== Launching Client CCD ====");
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

    UPDATE_APP_STATE(TEST_PICSTREAM_STR, rv);

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
    clientPicstream_fullresupload2 = dxTempFolder;
    clientPicstream_fullresdownload = dxTempFolder;
    clientPicstream_lowresupload = dxTempFolder;
    clientPicstream_lowresupload2 = dxTempFolder;
    clientPicstream_lowresdownload = dxTempFolder;
    clientPicstream_thumbupload = dxTempFolder;
    clientPicstream_thumbupload2 = dxTempFolder;
    clientPicstream_thumbdownload = dxTempFolder;
    clientPicstream_fullresupload = clientPicstream_fullresupload.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_upload_fullres");
    clientPicstream_fullresupload2 = clientPicstream_fullresupload2.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_upload_fullres2");
    clientPicstream_fullresdownload = clientPicstream_fullresdownload.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_fullres");
    clientPicstream_lowresupload = clientPicstream_lowresupload.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_upload_lowres");
    clientPicstream_lowresupload2 = clientPicstream_lowresupload2.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_upload_lowres2");
    clientPicstream_lowresdownload = clientPicstream_lowresdownload.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_lowres");
    clientPicstream_thumbupload = clientPicstream_thumbupload.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_upload_thumb");
    clientPicstream_thumbupload2 = clientPicstream_thumbupload2.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_upload_thumb2");
    clientPicstream_thumbdownload = clientPicstream_thumbdownload.append(pathSeparator).append("cr_test_temp_folder")
        .append(pathSeparator).append("autotest").append(pathSeparator).append("instance_client")
        .append(pathSeparator).append("pic_thumb");
    dst_non_supported_file = clientPicstream_fullresupload + pathSeparator + CLOUDDOC_DOCX_FILE;
    LOG_ALWAYS("dst_non_supported_file: %s", dst_non_supported_file.c_str());

    // Makes sure that the paths exist and are clean.
    rv = target->removeDirRecursive(clientPicstream_fullresupload);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_fullresupload.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_fullresupload2);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_fullresupload2.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_fullresdownload);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_fullresdownload.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_lowresupload);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_lowresupload.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_lowresupload2);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_lowresupload2.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_lowresdownload);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_lowresdownload.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_thumbupload);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_thumbupload.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_thumbupload2);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_thumbupload2.c_str());
    }
    rv = target->removeDirRecursive(clientPicstream_thumbdownload);
    if (rv != 0) {
        LOG_ERROR("Failed to remove path: %s", clientPicstream_thumbdownload.c_str());
    }
    VPLThread_Sleep(VPLTIME_FROM_MILLISEC(2000));
    rv = target->createDir(clientPicstream_fullresupload.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_fullresupload.c_str());
    }
    rv = target->createDir(clientPicstream_fullresupload2.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_fullresupload2.c_str());
    }    rv = target->createDir(clientPicstream_fullresdownload.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_fullresdownload.c_str());
    }
    rv = target->createDir(clientPicstream_lowresupload.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_lowresupload.c_str());
    }
    rv = target->createDir(clientPicstream_lowresupload2.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_lowresupload2.c_str());
    }
    rv = target->createDir(clientPicstream_lowresdownload.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_lowresdownload.c_str());
    }
    rv = target->createDir(clientPicstream_thumbupload.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_thumbupload.c_str());
    }
    rv = target->createDir(clientPicstream_thumbupload2.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_thumbupload2.c_str());
    }
    rv = target->createDir(clientPicstream_thumbdownload.c_str(), 0755);
    if(rv != 0) {
        LOG_ERROR("Could not create path:%s", clientPicstream_thumbdownload.c_str());
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

    LOG_ALWAYS("\n\n==== Testing PicStream (ClientPC) ====");
    for(i = FULL_RES; i!= LAST; i++) {
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        std::string upload_dir;
        std::string upload_dir2;
        std::string download_dir;
        std::string cloud_download_dir;

        switch(i) {
            case FULL_RES:
                LOG_ALWAYS("\n\n==== Testing Fullres download ====");
                TEST_PICSTREAM_STR = "SdkPicStreamAdvancedRelease_Fullres";
                upload_dir = clientPicstream_fullresupload;
                upload_dir2 = clientPicstream_fullresupload2;
                download_dir = clientPicstream_fullresdownload;
                cloud_download_dir = cloudPicstream_fulldown;
            break;
            case LOW_RES:
                LOG_ALWAYS("\n\n==== Testing Lowres download ====");
                TEST_PICSTREAM_STR = "SdkPicStreamAdvancedRelease_Lowres";
                download_dir = clientPicstream_lowresdownload;
                upload_dir = clientPicstream_lowresupload;
                upload_dir2 = clientPicstream_lowresupload2;
                cloud_download_dir = cloudPicstream_lowdown;
                PS_REMOVE_FULLRES_DIR(clientPicstream_fullresdownload, rv);
                PS_RM_UPLOAD_DIRS(clientPicstream_fullresupload, rv);
            break;
            case THUMBNAIL:
                LOG_ALWAYS("\n\n==== Testing Thumbnail download ====");
                TEST_PICSTREAM_STR = "SdkPicStreamAdvancedRelease_Thumbnail";
                download_dir = clientPicstream_thumbdownload;
                upload_dir = clientPicstream_thumbupload;
                upload_dir2 = clientPicstream_thumbupload2;
                cloud_download_dir = cloudPicstream_thumbdown;
                PS_REMOVE_LOWRES_DIR(clientPicstream_lowresdownload, rv);
                PS_RM_UPLOAD_DIRS(clientPicstream_lowresupload, rv);
            break;
            default:
                ;
            break;
        }

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
            if (i == FULL_RES && syncStateOut.camera_roll_upload_dirs_size() != 0) {
                testStr = "CheckUploadDirsSize";
                LOG_ERROR("Camera roll upload dirs are not clean, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            if (i == FULL_RES && syncStateOut.camera_roll_full_res_download_dirs_size() != 0) {
                testStr = "CheckFullresDownloadDirsSize";
                LOG_ERROR("Camera roll upload dirs are not clean, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
        }

        LOG_ALWAYS("\n==== Add Upload Dirs (MD) ====");
        PS_ADD_UPLOAD_DIRS(upload_dir, rv);
        if (full) {
            PS_ADD_UPLOAD_DIRS(upload_dir2, rv);
        }
        clientUpPrevNum_dir2 = 0;
        clientUpCurNum_dir2 = 0;

        LOG_ALWAYS("\n==== Add Download Dirs (MD) ====");
        switch (i) {
            case FULL_RES:
                PS_SET_FULLRES_DIR(download_dir, rv);
            break;
            case LOW_RES:
                PS_SET_LOWRES_DIR(download_dir, rv);
            break;
            case THUMBNAIL:
                PS_SET_THUMBNAIL_DIR(download_dir, rv);
            break;
            default:
                ;
            break;
        }

        get_cr_photo_count(upload_dir, crUpPrevNum);
        get_cr_photo_count(download_dir, crDownPrevNum);
        LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d", crUpPrevNum, crDownPrevNum);

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
            if (full && syncStateOut.camera_roll_upload_dirs_size() != 2) {
                testStr = "CheckUploadDirsSize";
                LOG_ERROR("Camera roll upload dirs size is wrong, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            else if (!full && syncStateOut.camera_roll_upload_dirs_size() != 1) {
                testStr = "CheckUploadDirsSize";
                LOG_ERROR("Camera roll upload dirs size is wrong, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            if (i == FULL_RES && syncStateOut.camera_roll_full_res_download_dirs_size() != 1) {
                testStr = "CheckFullresDownloadDirsSize";
                LOG_ERROR("Camera roll full res download dirs size is wrong, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            else if ( i == LOW_RES && syncStateOut.camera_roll_low_res_download_dirs_size() != 1) {
                testStr = "CheckLowresDownloadDirsSize";
                LOG_ERROR("Camera roll low res download dirs size is wrong, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
            else if ( i ==THUMBNAIL && syncStateOut.camera_roll_thumb_download_dirs_size() != 1) {
                testStr = "CheckThumbDownloadDirsSize";
                LOG_ERROR("Camera roll thumbnail download dirs size is wrong, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
        }

        retry = 0;
        tmp_count = 0;
        while (retry++ < 30) {
            get_cr_photo_count(upload_dir, crUpCurNum);
            get_cr_photo_count(download_dir, crDownCurNum);
            if (crUpCurNum == crUpPrevNum && crDownCurNum == crDownPrevNum && tmp_count++ > 5) {
                break;
            }
            else {
                tmp_count = 0;
            }
            crUpPrevNum = crUpCurNum;
            crDownPrevNum = crDownCurNum;
            VPLThread_Sleep(VPLTIME_FROM_SEC(1));
        }
        crUpPrevNum = crUpCurNum;
        crDownPrevNum = crDownCurNum;
        LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d", crUpPrevNum, crDownPrevNum);
        // Removed the previous Sleep
        // Each Sdk test should be independent and expects user to create new user
        // Otherwise it is invalid

        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        switch (i) {
            case FULL_RES:
                LOG_ALWAYS("\n==== Add FullRes Download Dir (CloudPC) ====");
                PS_SET_FULLRES_DIR(cloud_download_dir, rv);
            break;
            case LOW_RES:
                PS_REMOVE_FULLRES_DIR(cloudPicstream_fulldown, rv);
                LOG_ALWAYS("\n==== Add LowRes Download Dir (CloudPC) ====");
                PS_SET_LOWRES_DIR(cloud_download_dir, rv);
            break;
            case THUMBNAIL:
                PS_REMOVE_LOWRES_DIR(cloudPicstream_lowdown, rv);
                LOG_ALWAYS("\n==== Add Thumbnail Download Dir (CloudPC) ====");
                PS_SET_THUMBNAIL_DIR(cloud_download_dir, rv);
            break;
            default:
                break;
        }
        if (i == FULL_RES) {
            retry = 0;
            tmp_count = 0;
            while (retry++ < 30) {
                get_cr_photo_count(cloud_download_dir, cloudDownCurNum);
                if (cloudDownCurNum == cloudDownPrevNum && tmp_count++ > 5) {
                    break;
                }
                else {
                    tmp_count = 0;
                }
                cloudDownPrevNum = cloudDownCurNum;
                VPLThread_Sleep(VPLTIME_FROM_SEC(1));
            }
        }
        LOG_ALWAYS("cloudDownPrevNum = %d", cloudDownPrevNum);

        LOG_ALWAYS("\n==== Upload 3 photos from UploadDir_1 (MD) ====");
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        {
            PS_UPLOAD_PHOTO_JPG("3", true, rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "UploadPhoto", rv);
        }

        if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
            PS_TRIGGER_UPLOAD_DIR(upload_dir, rv);
        }

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir, crUpPrevNum, crUpCurNum, 3, 10, rv, false, "");

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 0, 120, rv, false, "");

        PS_STATUS(false, rv);

#if defined(CLOUDNODE)
        CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
#else
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
        PS_STATUS(false, rv);
#endif

        crUpPrevNum = crUpCurNum;
        crDownPrevNum = crDownCurNum;
        cloudDownPrevNum = cloudDownCurNum;
        LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum);

        if (full) {
            LOG_ALWAYS("\n==== Upload 3 photos from UploadDir_2 (MD) ====");
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }
            {
                PS_UPLOAD_PHOTO_FRM_DIR("3", "1", false, rv);
            }

            if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
                PS_TRIGGER_UPLOAD_DIR(upload_dir2, rv);
            }

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir2, clientUpPrevNum_dir2, clientUpCurNum_dir2, 3, 10, rv, false, "");

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 0, 120, rv, false, "");

            PS_STATUS(false, rv);

    #if defined(CLOUDNODE)
            CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
    #else
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
            PS_STATUS(false, rv);
    #endif

            crUpPrevNum = crUpCurNum;
            crDownPrevNum = crDownCurNum;
            cloudDownPrevNum = cloudDownCurNum;
            clientUpPrevNum_dir2 = clientUpCurNum_dir2;
            LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d, clientUpPrevNum_dir2 = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum, clientUpPrevNum_dir2);
        }

        VPLThread_Sleep(VPLTIME_FROM_SEC(1));

        LOG_ALWAYS("\n==== Set Enable_Upload TRUE (MD) ====");
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        PS_SET_ENABLE_UPLOAD(rv);
        VPLThread_Sleep(VPLTIME_FROM_SEC(1));

        check_event_visitor_ctx ctx;
        {
            const char *testStr = "CheckSyncCompleteEventThread";

            ctx.done        = false;
            ctx.userid      = userId;
            ctx.count       = 0;
            ctx.expected_count = 6;

            switch (i) {
                case FULL_RES:
                    ctx.event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES);
                break;
                case LOW_RES:
                    ctx.event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES);
                break;
                case THUMBNAIL:
                    ctx.event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL);
                break;
                default:
                break;
            }

            CREATE_PICSTREAM_EVENT_VISIT_THREAD(testStr);
        }

        LOG_ALWAYS("\n==== Upload 3 photos(TIF) from UploadDir_1 (MD) ====");
        {
            PS_UPLOAD_PHOTO_TIF("3", true, rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "UploadPhoto_TIF", rv);
        }

        if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
            PS_TRIGGER_UPLOAD_DIR(upload_dir, rv);
        }

        CHECK_PICSTREAM_EVENT_VISIT_RESULT("SyncCompleteEvent", 40);

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir, crUpPrevNum, crUpCurNum, 3, 10, rv, false, "");

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 3, 180, rv, false, "");

        PS_STATUS(false, rv);

#if defined(CLOUDNODE)
        CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 3, 100, rv, false, "");
#else
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 3, 100, rv, false, "");
        PS_STATUS(false, rv);
#endif

        crUpPrevNum = crUpCurNum;
        crDownPrevNum = crDownCurNum;
        cloudDownPrevNum = cloudDownCurNum;
        LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum);

        if (full) {
            LOG_ALWAYS("\n==== Upload 3 photos(JPEG) from UploadDir_2 (MD) ====");
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }

            {
                PS_UPLOAD_PHOTO_JPEG_FRM_DIR("3", "1", false, rv);
            }

            if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
                PS_TRIGGER_UPLOAD_DIR(upload_dir2, rv);
            }

            CHECK_PICSTREAM_EVENT_VISIT_RESULT("SyncCompleteEvent", 40);

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir2, clientUpPrevNum_dir2, clientUpCurNum_dir2, 3, 10, rv, false, "");

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 3, 180, rv, false, "");

            PS_STATUS(false, rv);

    #if defined(CLOUDNODE)
            CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 3, 100, rv, false, "");
    #else
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 3, 100, rv, false, "");
            PS_STATUS(false, rv);
    #endif

            crUpPrevNum = crUpCurNum;
            crDownPrevNum = crDownCurNum;
            cloudDownPrevNum = cloudDownCurNum;
            clientUpPrevNum_dir2 = clientUpCurNum_dir2;
            LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d, clientUpPrevNum_dir2 = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum, clientUpPrevNum_dir2);
        }

        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
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
                !syncStateOut.is_camera_roll_upload_enabled()) {
                LOG_ERROR("Camera roll upload is not enabled, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
        }

        if (full) {
            LOG_ALWAYS("\n==== Upload non-supported file to UploadDir_1 (MD) ====");
            if (i == LOW_RES) {
                dst_non_supported_file = clientPicstream_lowresupload + pathSeparator + CLOUDDOC_DOCX_FILE;
            } else if (i == THUMBNAIL) {
                dst_non_supported_file = clientPicstream_thumbupload + pathSeparator + CLOUDDOC_DOCX_FILE;
            }
            LOG_ALWAYS("dst_non_supported_file: %s", dst_non_supported_file.c_str());
            target = getTargetDevice();
            rv = target->pushFile(src_non_supported_file, dst_non_supported_file);
            if(rv != 0) {
                LOG_ERROR("Copy of %s->%s failed",
                          src_non_supported_file.c_str(),
                          dst_non_supported_file.c_str());
            }
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "CopyNonSupportedFileToUploadDir", rv);

            if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
                PS_TRIGGER_UPLOAD_DIR(upload_dir, rv);
            }

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir, crUpPrevNum, crUpCurNum, 0, 10, rv, false, "");

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 0, 180, rv, false, "");

            PS_STATUS(false, rv);

    #if defined(CLOUDNODE)
            CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
    #else
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
            PS_STATUS(false, rv);
    #endif

            crUpPrevNum = crUpCurNum;
            crDownPrevNum = crDownCurNum;
            cloudDownPrevNum = cloudDownCurNum;
            LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum);
        }

        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(500));
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        {
            LOG_ALWAYS("\n==== Upload 3 photos(TIFF) from UploadDir_1 (MD) ====");
            PS_UPLOAD_PHOTO_TIFF("3", true, rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "UploadPhoto_TIFF", rv);
        }

        if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
            PS_TRIGGER_UPLOAD_DIR(upload_dir, rv);
        }

        // Wait for the uploaded photo to be synced
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir, crUpPrevNum, crUpCurNum, 3, 10, rv, false, "");

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 3, 180, rv, false, "");

        PS_STATUS(false, rv);

#if defined(CLOUDNODE)
        CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 3, 100, rv, false, "");
#else
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 3, 100, rv, false, "");
        PS_STATUS(false, rv);
#endif

        crUpPrevNum = crUpCurNum;
        crDownPrevNum = crDownCurNum;
        cloudDownPrevNum = cloudDownCurNum;
        LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum);

        if (full) {
            LOG_ALWAYS("\n==== Remove UploadDir_2 (MD) ====");
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }
            PS_RM_UPLOAD_DIRS(upload_dir2, rv);

            LOG_ALWAYS("\n==== Upload 3 photos from UploadDir_2 (MD) ====");
            {
                PS_UPLOAD_PHOTO_FRM_DIR("3", "1", true, rv);
                if (rv != 0) {
                    LOG_ALWAYS("Expected to FAIL! UploadDir_2 was removed in last step!!!");
                    rv = 0;
                }
                else {
                    rv = -1;
                }
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "UploadPhotoToRemovedDir", rv);
            }

            if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
                PS_TRIGGER_UPLOAD_DIR_SKIP(upload_dir2, true, rv);
                if (rv != 0) {
                    LOG_ALWAYS("Expected to FAIL! UploadDir_2 was removed in last step!!!");
                    rv = 0;
                }
                else {
                    rv = -1;
                }
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "TriggerUploadDir", rv);
            }

            CHECK_PICSTREAM_EVENT_VISIT_RESULT("SyncCompleteEvent", 40);

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir2, clientUpPrevNum_dir2, clientUpCurNum_dir2, 0, 10, rv, false, "");

            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 0, 180, rv, false, "");

            PS_STATUS(false, rv);

    #if defined(CLOUDNODE)
            CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
    #else
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
            PS_STATUS(false, rv);
    #endif

            crUpPrevNum = crUpCurNum;
            crDownPrevNum = crDownCurNum;
            cloudDownPrevNum = cloudDownCurNum;
            clientUpPrevNum_dir2 = clientUpCurNum_dir2;
            LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d, clientUpPrevNum_dir2 = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum, clientUpPrevNum_dir2);
        }

        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        VPLThread_Sleep(VPLTIME_FROM_SEC(2));

        PS_SET_DISABLE_UPLOAD(rv);

        VPLThread_Sleep(VPLTIME_FROM_SEC(1));
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
                LOG_ERROR("Camera roll upload is not enabled, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
        }

        VPLThread_Sleep(VPLTIME_FROM_SEC(1));
        {
            LOG_ALWAYS("\n==== Upload 3 photos(PNG) from UploadDir_1 (MD) ====");
            PS_UPLOAD_PHOTO_PNG("3", true, rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "UploadPhoto_PNG", rv);
        }

        if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
            PS_TRIGGER_UPLOAD_DIR(upload_dir, rv);
        }

        // Wait for the uploaded photo to be synced
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir, crUpPrevNum, crUpCurNum, 3, 10, rv, false, "");

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 0, 180, rv, false, "");

        PS_STATUS(false, rv);

#if defined(CLOUDNODE)
        CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
#else
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
        PS_STATUS(false, rv);
#endif

        crUpPrevNum = crUpCurNum;
        crDownPrevNum = crDownCurNum;
        cloudDownPrevNum = cloudDownCurNum;
        LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum);

        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

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
                LOG_ERROR("Camera roll upload is not enabled, return!");
                rv = -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, testStr, rv);
            }
        }

        PS_STATUS(false, rv);

        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(500));
        {
            LOG_ALWAYS("\n==== Upload 3 photos(BMP) from UploadDir_1 (MD) ====");
            PS_UPLOAD_PHOTO_BMP("3", true, rv);
            CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "UploadPhoto_BMP", rv);
        }

        if (osVersion == OS_WINDOWS_RT || isIOS(osVersion)) {
            PS_TRIGGER_UPLOAD_DIR(upload_dir, rv);
        }

        // Wait for the uploaded photo to be synced
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, upload_dir, crUpPrevNum, crUpCurNum, 3, 10, rv, false, "");

        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, 0, 180, rv, false, "");

        PS_STATUS(false, rv);

#if defined(CLOUDNODE)
        CHECK_UPLOADED_PHOTO_NUM_CLOUDNODE(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
#else
        SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, cloud_download_dir, cloudDownPrevNum, cloudDownCurNum, 0, 100, rv, false, "");
        PS_STATUS(false, rv);
#endif

        crUpPrevNum = crUpCurNum;
        crDownPrevNum = crDownCurNum;
        cloudDownPrevNum = cloudDownCurNum;
        LOG_ALWAYS("crUpPrevNum = %d, crDownPrevNum = %d, cloudDownPrevNum = %d", crUpPrevNum, crDownPrevNum, cloudDownPrevNum);


        {//Global Delete
            TEST_PICSTREAM_STR = "SdkPicStreamAdvancedRelease_GlobalDelete";
            LOG_ALWAYS("\n\n==== Global Delete Test ====\n\n");
            LOG_ALWAYS("\n\ntarget download path: %s current photo count:%d\n\n", download_dir.c_str(), crDownPrevNum);
            SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }
            std::queue<std::string> titles;
            std::queue<std::string> compIds;
            std::string title, compId, httpResp;
            ccd::CCDIQueryPicStreamObjectsInput req;
            ccd::CCDIQueryPicStreamObjectsOutput resp;
            int repeat = 2;

            {//wait until at least repeat items are available.
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
                    if(resp.mutable_content_objects()->size() > repeat*2 ) {

                        google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject >::iterator it;
                        for (it = resp.mutable_content_objects()->begin(); it != resp.mutable_content_objects()->end(); ++it) {
                            titles.push(it->mutable_pcdo()->mutable_picstream_item()->title());
                            compIds.push(it->mutable_pcdo()->comp_id());
                        }

                        if(titles.size() > (u32) repeat*2) {
                            LOG_ALWAYS(FMTu_size_t" photos are ready for test.", titles.size());
                            break;
                        }
                    }

                    if (times++ > 30) {
                        LOG_ERROR("Wait for photos TIMEOUT!");
                        rv = -1;
                        CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "PhotoReady", rv);
                    }
                    LOG_ALWAYS("photo items: %d", titles.size());
                    VPLThread_Sleep(VPLTIME_FROM_SEC(3));
                }
            }

            //
            // When global deletion is disabled, the deleted items' records remain in local DB while 
            // their data in infra are deleted. So, we remove them from the query results to prevent 
            // from using them during test.
            //
            int itemsRemainInDb = 0;
            for (int k=0; k < i * repeat;k++) {
                titles.pop();
                compIds.pop();
                itemsRemainInDb++;
            }

            for( int enabled = 1; enabled >= 0; enabled--) {

                //
                // If Global Delete is enabled, the local photos and data in DB will be deleted.
                //
                if (1 == enabled) {
                    LOG_ALWAYS("\n\n==== Enable Global Delete ====\n\n");
                    SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
                    if (rv < 0) {
                        setCcdTestInstanceNum(cloudPCId);
                    }
                    PS_SET_ENABLE_GLOBAL_DELETE(rv);
                    SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
                    if (rv < 0) {
                        setCcdTestInstanceNum(clientPCId);
                    }
                    PS_SET_ENABLE_GLOBAL_DELETE(rv);
                } else {
                    LOG_ALWAYS("\n\n==== Disable Global Delete ====\n\n");
                    SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "CloudPC", rv);
                    if (rv < 0) {
                        setCcdTestInstanceNum(cloudPCId);
                    }
                    PS_SET_DISABLE_GLOBAL_DELETE(rv);
                    SET_TARGET_MACHINE(TEST_PICSTREAM_STR, "MD", rv);
                    if (rv < 0) {
                        setCcdTestInstanceNum(clientPCId);
                    }
                    PS_SET_DISABLE_GLOBAL_DELETE(rv);
                }
                VPLThread_Sleep(VPLTIME_FROM_SEC(1));
                PS_STATUS(false, rv);

                {
                    const char *testStr = "CheckSyncCompleteEventThread";

                    ctx.done        = false;
                    ctx.userid      = userId;
                    ctx.count       = 0;
                    ctx.expected_count = repeat;
                    ctx.event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PICSTREAM_DELETION);
                    CREATE_PICSTREAM_EVENT_VISIT_THREAD(testStr);
                }

                LOG_ALWAYS("\n==== Delete %d photos (MD) ====\n", repeat);
                for(int cnt = 0; cnt < repeat; cnt++) {

                    title = titles.front();
                    compId = compIds.front();
                    LOG_ALWAYS("Delete %s compId %s ", title.c_str(), compId.c_str());
                    PS_DELETE(title, compId, httpResp, rv);

                    titles.pop();
                    compIds.pop();
                }
                VPLThread_Sleep(VPLTIME_FROM_SEC(5));
                CHECK_PICSTREAM_EVENT_VISIT_RESULT("SyncCompleteEvent", 20);
                CHECK_UPLOADED_PHOTO_NUM(TEST_PICSTREAM_STR, download_dir, crDownPrevNum, crDownCurNum, repeat * (-1) * enabled, 20, rv, false, "");

                //check if data in DB is deleted!
                req.set_filter_field(ccd::PICSTREAM_QUERY_ITEM);
                req.clear_search_field();
                rv = CCDIQueryPicStreamObjects(req, resp);
                if(rv != 0) {
                    LOG_ERROR("CCDIQueryPicStreamObjects [PICSTREAM_QUERY_ITEM]: %d", rv);
                    return rv;
                }

                //
                // When checking number of items in DB, we need to calculate items of not being deleted 
                // due to global deletion being disabled. (itemsRemainInDb)
                //
                rv = (resp.mutable_content_objects()->size() == crDownCurNum + itemsRemainInDb) ? 0 : -1;
                CHECK_AND_PRINT_RESULT(TEST_PICSTREAM_STR, "Check_DB", rv);
                crDownPrevNum = crDownCurNum;
            }//for( int enabled = 1; enabled >= 0; enabled--)

            //
            // Adjust number of photos of sync down in both cloudPC and MD because 
            // photo items are deleted in global delete test above. Twice deletion tests are performed.
            // One is for global deletion enabled while the other is for disabled.
            // Each test deletes number("repeat") of photos.
            //
            cloudDownPrevNum -= repeat*2;
            crDownPrevNum -= repeat;
        }//Global Delete

        if (!full)
            break;
    }

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
