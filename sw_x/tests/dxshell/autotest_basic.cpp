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
#include "fs_test.hpp"
#include "TargetDevice.hpp"
#include "EventQueue.hpp"

#include "autotest_basic.hpp"

#include "cJSON2.h"

#include <sstream>
#include <string>


static int check_event_visitor(const ccd::CcdiEvent &_event, void *_ctx)
{

    check_event_visitor_ctx *ctx = (check_event_visitor_ctx*)_ctx;
   
    if (ctx->event.has_storage_node_change()){

        if (!_event.has_storage_node_change()) {
            LOG_ALWAYS("No has_storage_node_change!");
            return 0;
        }

        if (!ctx->event.has_storage_node_change()){
            LOG_ALWAYS("No has_storage_node_change in ctx!");
            return 0;
        }

        const ccd::EventStorageNodeChange &event = _event.storage_node_change();
        const ccd::EventStorageNodeChange &ctxevent = ctx->event.storage_node_change();

        LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        LOG_ALWAYS("device_id,   "FMTu64, event.device_id());
        LOG_ALWAYS("change_type, %d", event.change_type());
        LOG_ALWAYS("ctx->device_id: "FMTu64, ctxevent.device_id());
        if (event.change_type() == ctxevent.change_type() &&
                event.device_id() == ctxevent.device_id()) {
            LOG_INFO("request completed: %d , device_id:"FMTu64, event.change_type(), event.device_id());
            ctx->done = true;
        }
    }

    if (ctx->event.has_device_connection_change()){

        if (!_event.has_device_connection_change()) {
            LOG_ALWAYS("No has_device_connection_change!");
            return 0;
        }

        const ccd::EventDeviceConnectionChange &event = _event.device_connection_change();
        const ccd::EventDeviceConnectionChange &ctxevent = ctx->event.device_connection_change();

        LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        LOG_ALWAYS("device_id,   "FMTu64, event.device_id());
        LOG_ALWAYS("state, %d", event.status().state());
        LOG_ALWAYS("ctx->device_id: "FMTu64, ctxevent.device_id());
        if (event.status().state() == ctxevent.status().state() &&
                event.device_id() == ctxevent.device_id()) {
            LOG_INFO("request completed: %d , device_id:"FMTu64, event.status().state(), event.device_id());
            ctx->done = true;
        }
    }

    if (ctx->event.has_device_info_change()){

        if (!_event.has_device_info_change()) {
            LOG_ALWAYS("No has_device_info_change!");
            return 0;
        }

        const ccd::EventDeviceInfoChange &event = _event.device_info_change();
        const ccd::EventDeviceInfoChange &ctxevent = ctx->event.device_info_change();

        LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        LOG_ALWAYS("device_id,   "FMTu64, event.device_id());
        LOG_ALWAYS("state, %d", event.change_type());
        LOG_ALWAYS("ctx->device_id: "FMTu64, ctxevent.device_id());
        if (event.change_type() == ctxevent.change_type() &&
                event.device_id() == ctxevent.device_id()) {
            LOG_INFO("request completed: %d , device_id:"FMTu64, event.change_type(), event.device_id());
            ctx->done = true;
        }
    }

    if (ctx->event.has_sync_feature_status_change()){

        if (!_event.has_sync_feature_status_change()) {
            LOG_ALWAYS("No has_sync_feature_status_change!");
            return 0;
        }

        const ccd::EventSyncFeatureStatusChange &event = _event.sync_feature_status_change();
        const ccd::EventSyncFeatureStatusChange &ctxevent = ctx->event.sync_feature_status_change();

        if (event.feature() == ctxevent.feature()) {
            LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            if (event.status().status() == ccd::CCD_FEATURE_STATE_SYNCING) {
                ctx->count++;
                LOG_INFO("CCD_FEATURE_STATE_SYNCING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            }
            else if(event.status().status() == ccd::CCD_FEATURE_STATE_IN_SYNC) {
                LOG_INFO("CCD_FEATURE_STATE_IN_SYNC!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                if (ctx->count == 1) {
                    ctx->done =true;
                }
            }
        }
    }

    if (ctx->event.has_lan_devices_change()){

        if (!_event.has_lan_devices_change()) {
            LOG_ALWAYS("No has_lan_devices_change!");
            return 0;
        }

        const ccd::EventLanDevicesChange &event = _event.lan_devices_change();

        LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        if (event.has_timestamp()) {
            LOG_INFO("Timestamp is   "FMTu64, event.timestamp());
        }
            
        ctx->done = true;
    }

    if (ctx->event.has_lan_devices_probe_request()){

        if (!_event.has_lan_devices_probe_request()) {
            LOG_ALWAYS("No has_lan_devices_probe_request!");
            return 0;
        }

        const ccd::EventLanDevicesProbeRequest &event = _event.lan_devices_probe_request();

        LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        if (event.has_timestamp()) {
            LOG_INFO("Timestamp is   "FMTu64, event.timestamp());
        }
            
        ctx->done = true;
    }

    if (ctx->event.has_user_logout()){

        if (!_event.has_user_logout()) {
            LOG_ALWAYS("No has_user_logout!");
            return 0;
        }

        const ccd::EventUserLogout &event = _event.user_logout();
        const ccd::EventUserLogout &ctxevent = ctx->event.user_logout();

        if (event.user_id() == ctxevent.user_id()) {
            LOG_ALWAYS("Got the event!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            if (event.reason() == ctxevent.reason()) {
                LOG_ALWAYS("Request completed: User logged out for LOGOUT_REASON_DEVICE_UNLINKED!!!!");
                ctx->done = true;
            }
        }
    }

    return 0;
}

#define CHECK_EVENT_VISIT_PREP \
    check_event_visitor_ctx* ctx = new check_event_visitor_ctx()

#define CHECK_EVENT_VISIT_RESULT(testStr, timeout) \
    BEGIN_MULTI_STATEMENT_MACRO \
    VPLTime_t start = VPLTime_GetTimeStamp(); \
    VPLTime_t end = start + VPLTime_FromSec(timeout); \
    VPLTime_t now; \
    u64 deviceId = 0; \
    set_target_machine(ctx->alias.c_str()); \
    while((now = VPLTime_GetTimeStamp()) < end && !ctx->done) { \
        rv = getDeviceId(&deviceId); \
        if (rv != 0) { \
            LOG_ERROR("Fail to get device id:%d", rv); \
        } \
        else{ \
            LOG_ALWAYS("device id:"FMTu64, deviceId); \
        } \
        ctx->eq.visit(0, (s32)(VPLTime_ToMillisec(VPLTime_DiffClamp(end, now))), check_event_visitor, (void*)ctx); \
    } \
    if(!ctx->done) { \
        LOG_ERROR("%s.%s didn't complete within "FMT_VPLTime_t" seconds",  \
                  TEST_BASIC_STR, testStr, VPLTime_ToSec(VPLTime_DiffClamp(VPLTime_GetTimeStamp(), start))); \
        CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, -1); \
    } \
    delete ctx; \
    ctx = NULL; \
    CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, 0); \
    END_MULTI_STATEMENT_MACRO

#define ENABLE_PLAYLIST_SYNC(testStr) \
    BEGIN_MULTI_STATEMENT_MACRO \
    ccd::UpdateAppStateInput uasInput; \
    ccd::UpdateAppStateOutput uasOutput; \
    uasInput.set_app_id("PLAYLIST_EVENT_SYNC_TEST"); \
    uasInput.set_app_type(ccd::CCD_APP_ALL_MEDIA); \
    uasInput.set_foreground_mode(true); \
    rv = CCDIUpdateAppState(uasInput, uasOutput); \
    if(rv != 0) { \
        LOG_ERROR("CCDIUpdateAppState:%d, err:%d", rv, uasOutput.foreground_mode_err()); \
    }else{ \
        LOG_INFO("CCDIUpdateAppState ok"); \
    } \
    CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv); \
    END_MULTI_STATEMENT_MACRO

#define GET_PLAYLIST_PATH(testStr, path) \
    BEGIN_MULTI_STATEMENT_MACRO \
    ccd::GetSyncStateInput ccdiRequest; \
    ccd::GetSyncStateOutput ccdiResponse; \
    ccdiRequest.set_get_media_playlist_path(true); \
    rv = CCDIGetSyncState(ccdiRequest, ccdiResponse); \
    if (rv != 0) { \
        LOG_ERROR("%s failed: %d", "CCDIGetSyncState", rv); \
    } \
    if (!ccdiResponse.has_media_playlist_path()) { \
        LOG_ERROR("Failed get playlist path"); \
    } \
    else { \
        path = ccdiResponse.media_playlist_path(); \
    } \
    CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv); \
    END_MULTI_STATEMENT_MACRO

#define SET_POWER_FGMODE(app_id, foreground) \
    BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testStr1 = "Power"; \
        const char *testStr2 = "FgMode"; \
        const char *testStr3 = app_id; \
        const char *testStr4 = "5"; \
        const char *testStr5 = foreground; \
        const char *testArg[] = { testStr1, testStr2, testStr3, testStr4, testStr5 }; \
        LOG_INFO("Set %s to foreground(%s)", app_id, foreground); \
        rv = power_dispatch(5, testArg); \
        const char *testStr = "SetPowerFgMode"; \
        CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv); \
    } \
    END_MULTI_STATEMENT_MACRO

int do_autotest_sdk_release_basic(int argc, const char* argv[])
{
    const char *TEST_BASIC_STR = "SdkBasicRelease";
    int rv = 0;
    int cloudPCId = 1;
    int clientPCId = 2;
    int clientPCId2 = 3;
    bool full = false;
    std::string clientPCOSVersion;

    u64 deviceId[4] = {0, 0, 0, 0};
    bool check_download_sync = true;

    if (argc == 5 && (strcmp(argv[4], "-f") == 0 || strcmp(argv[4], "--fulltest") == 0) ) {
        full = true;
    }

    if (checkHelp(argc, argv) || (argc < 4) || (argc == 5 && !full)) {
        printf("AutoTest %s <domain> <username> <password> [<fulltest>(-f/--fulltest)]\n", argv[0]);
        return 0;   // No arguments needed 
    }

    LOG_ALWAYS("Auto Test SDK Release Basic: Domain(%s) User(%s) Password(%s)", argv[1], argv[2], argv[3]);

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
    SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        std::string alias = "CloudPC";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_BASIC_STR, rv);
    }

    START_CCD(TEST_BASIC_STR, rv);
    START_CLOUDPC(argv[2], argv[3], TEST_BASIC_STR, true, rv);

    SET_POWER_FGMODE("dx_remote_agent_CloudPC", "1");

    LOG_ALWAYS("\n\n==== Launching Client CCD ====");
    SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        std::string alias = "MD";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_BASIC_STR, rv);
    }

    {
        QUERY_TARGET_OSVERSION(clientPCOSVersion, TEST_BASIC_STR, rv);
    }

    {
        START_CCD(TEST_BASIC_STR, rv);
    }

    UPDATE_APP_STATE(TEST_BASIC_STR, rv);


    START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

    SET_POWER_FGMODE("dx_remote_agent_MD", "1");

    if(full){

        //Make sure only 1 report generated before Case2 
        SET_POWER_FGMODE("dx_remote_agent_MD", "0");
        VPLThread_Sleep(VPLTIME_FROM_SEC(25));

        //Make sure only 2 report (2 entry) generated for Case2
        SET_POWER_FGMODE("dx_remote_agent_MD_Case2", "1");
        SET_POWER_FGMODE("dx_remote_agent_MD_Case2", "0");
        VPLThread_Sleep(VPLTIME_FROM_SEC(15));
        SET_POWER_FGMODE("dx_remote_agent_MD_Case2", "1");
        SET_POWER_FGMODE("dx_remote_agent_MD_Case2", "0");
        VPLThread_Sleep(VPLTIME_FROM_SEC(15));

        //Generate event over limit (100)
        //we should see 2 entry for App1/App2, each with 50 event_count, limit_reached is Y
        for(int i=0; i<60; i++){
            SET_POWER_FGMODE("dx_remote_agent_MD_Case3_App1", "1");
            SET_POWER_FGMODE("dx_remote_agent_MD_Case3_App2", "1");
            VPLThread_Sleep(VPLTime_FromMillisec(1));
            SET_POWER_FGMODE("dx_remote_agent_MD_Case3_App1", "0");
            SET_POWER_FGMODE("dx_remote_agent_MD_Case3_App2", "0");
        }

        //Keep MD foreground, Make sure event generated repeatly
        SET_POWER_FGMODE("dx_remote_agent_MD", "1");
    }

    LOG_ALWAYS("\n\n==== Launching Client CCD 2 ====");
    SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId2);
    }

    {
        std::string alias = "Client";
        CHECK_LINK_REMOTE_AGENT(alias, TEST_BASIC_STR, rv);
    }

    START_CCD(TEST_BASIC_STR, rv);
    START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

    SET_POWER_FGMODE("dx_remote_agent_Client", "1");
    
    SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        rv = getDeviceId(&deviceId[cloudPCId]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
    }

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
    {
        std::vector<u64> deviceIds;
        u64 userId = 0;
        //u64 deviceId = 0;

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }

        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        rv = getDeviceId(&deviceId[cloudPCId]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        deviceIds.push_back(deviceId[cloudPCId]);
        LOG_ALWAYS("cloud pc deviceId: "FMTu64, deviceId[cloudPCId]);

        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        rv = getDeviceId(&deviceId[clientPCId]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        deviceIds.push_back(deviceId[clientPCId]);
        LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[clientPCId]);

        SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }
        rv = getDeviceId(&deviceId[clientPCId2]);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id:%d", rv);
            goto exit;
        }
        deviceIds.push_back(deviceId[clientPCId2]);
        LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[clientPCId2]);

        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckCloudPCDeviceLinkStatus", rv);
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckMDDeviceLinkStatus", rv);
        SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }
        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckClientPCDeviceLinkStatus", rv);
    }

    // CloudPC StopCCD, verify devices link status
    if (full) {
        LOG_ALWAYS("\n\n== Stop CloudPC CCD, Checking cloudpc and Client device link status ==");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        {
            const char *testStr = "StopCCD";
            const char *testArg[] = { testStr };
            rv = stop_ccd_soft(1, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        {
            std::vector<u64> online_deviceIds;
            std::vector<u64> offline_deviceIds;
            u64 userId = 0;

            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }

            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }

            offline_deviceIds.push_back(deviceId[cloudPCId]);
            LOG_ALWAYS("cloud pc deviceId: "FMTu64, deviceId[cloudPCId]);

            online_deviceIds.push_back(deviceId[clientPCId]);
            LOG_ALWAYS("MD pc deviceId: "FMTu64, deviceId[clientPCId]);

            online_deviceIds.push_back(deviceId[clientPCId2]);
            LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[clientPCId2]);

            rv = wait_for_devices_to_be_online(-1, userId, online_deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckDevicesOnlineFromMD", rv);
            rv = wait_for_devices_to_be_offline(-1, userId, offline_deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckCloudPCOfflineFromMD", rv);

            SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId2);
            }
            rv = wait_for_devices_to_be_online(-1, userId, online_deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckDevicesOnlineFromClientPC", rv);
            rv = wait_for_devices_to_be_offline(-1, userId, offline_deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckCloudPCOfflineFromClientPC", rv);
        }

        LOG_ALWAYS("\n\n== Stop Client CCD, Checking cloudpc and Client device link status ==");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }
        {
            const char *testStr = "StopCCD";
            const char *testArg[] = { testStr };
            rv = stop_ccd_soft(1, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        {
            std::vector<u64> online_deviceIds;
            std::vector<u64> offline_deviceIds;
            u64 userId = 0;

            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }

            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }

            offline_deviceIds.push_back(deviceId[cloudPCId]);
            LOG_ALWAYS("cloud pc deviceId: "FMTu64, deviceId[cloudPCId]);

            online_deviceIds.push_back(deviceId[clientPCId]);
            LOG_ALWAYS("MD pc deviceId: "FMTu64, deviceId[clientPCId]);

            offline_deviceIds.push_back(deviceId[clientPCId2]);
            LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[clientPCId2]);

            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }
            rv = wait_for_devices_to_be_online(-1, userId, online_deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckMDOnlineFromMD", rv);
            rv = wait_for_devices_to_be_offline(-1, userId, offline_deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckDevicesOfflineFromMD", rv);
        }

        LOG_ALWAYS("\n\n== Start Client CCD ==");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }
        {
            const char *testStr = "StartCCD";
            const char *testArg[] = { testStr };
            rv = start_ccd(1, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        LOG_ALWAYS("\n\n== Start CloudPC CCD ==");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        {
            const char *testStr = "StartCCD";
            const char *testArg[] = { testStr };
            rv = start_ccd(1, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }
    }

    // create context to listen to the complete event
    {
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        CHECK_EVENT_VISIT_PREP;
        ctx->alias       = "MD";
        ctx->event.mutable_storage_node_change()->set_device_id(deviceId[cloudPCId]);
        ctx->event.mutable_storage_node_change()->set_change_type(ccd::STORAGE_NODE_CHANGE_TYPE_UPDATED);

        LOG_ALWAYS("\n\n==== Disable CloudPC ====");

        //setCcdTestInstanceNum(cloudPCId);
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        {
            const char *testStr = "EnableCloudPC";
            const char *testArg[] = { testStr, "false" };
            rv = enable_cloudpc(2, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        CHECK_EVENT_VISIT_RESULT("DisableCloudPCEvent", 30);

        //check if media server feature disable!
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        u64 devId;
        u64 userId = 0;
        rv = getDeviceId(&devId);
        if (rv != 0) {
            LOG_ERROR("Fail to get Device id:%d", rv);
            goto exit;
        }

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }

        ccd::ListUserStorageInput psnrequest;
        ccd::ListUserStorageOutput psnlistSnOut;   
        psnrequest.set_user_id(userId);
        psnrequest.set_only_use_cache(false);
        
        rv = CCDIListUserStorage(psnrequest, psnlistSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListUserStorage for user("FMTu64") failed %d", userId, rv);
            goto exit;
        } else {

            LOG_ALWAYS("CCDIListUserStorage OK - %d", psnlistSnOut.user_storage_size());
            rv = -1;
            for (int i = 0; i < psnlistSnOut.user_storage_size(); i++) {
                if (psnlistSnOut.user_storage(i).storageclusterid() == devId) {
                    LOG_INFO("Media Server Feature: %d ", psnlistSnOut.user_storage(i).featuremediaserverenabled()); 
                    if ((psnlistSnOut.user_storage(i).featuremediaserverenabled() == false)) { 
                        rv = 0;
                        break;
                    }
                }
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "DisableCloudPC", rv);
        }
    }   

    // create context to listen to the complete event
    {
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        CHECK_EVENT_VISIT_PREP;
        ctx->alias       = "MD";
        ctx->event.mutable_storage_node_change()->set_device_id(deviceId[cloudPCId]);
        ctx->event.mutable_storage_node_change()->set_change_type(ccd::STORAGE_NODE_CHANGE_TYPE_UPDATED);

        LOG_ALWAYS("\n\n==== Enable CloudPC ====");

        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        {
            const char *testStr = "EnableCloudPC";
            const char *testArg[] = { testStr, "true" };
            rv = enable_cloudpc(2, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        CHECK_EVENT_VISIT_RESULT("EnableCloudPCEvent", 30);

        //check if media server feature enable
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        u64 devId;
        u64 userId = 0;
        rv = getDeviceId(&devId);
        if (rv != 0) {
            LOG_ERROR("Fail to get Device id:%d", rv);
            goto exit;
        }

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }

        //Check if media server enable
        ccd::ListUserStorageInput psnrequest;
        ccd::ListUserStorageOutput psnlistSnOut;   
        psnrequest.set_user_id(userId);
        psnrequest.set_only_use_cache(false);
        
        rv = CCDIListUserStorage(psnrequest, psnlistSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListUserStorage for user("FMTu64") failed %d", userId, rv);
            goto exit;
        } else {

            LOG_ALWAYS("CCDIListUserStorage OK - %d", psnlistSnOut.user_storage_size());
            rv = -1;
            for (int i = 0; i < psnlistSnOut.user_storage_size(); i++) {
                if (psnlistSnOut.user_storage(i).storageclusterid() == devId) {
                    LOG_INFO("Media Server Feature: %d ", psnlistSnOut.user_storage(i).featuremediaserverenabled()); 
                    if ((psnlistSnOut.user_storage(i).featuremediaserverenabled() == true)) { 
                        rv = 0;
                        break;
                    }
                }
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "EnableCloudPC", rv);
        }
    }   

    // create context to listen to the complete event
    {
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        CHECK_EVENT_VISIT_PREP;
        ctx->alias       = "MD";
        ctx->event.mutable_device_connection_change()->set_device_id(deviceId[cloudPCId]);
        ctx->event.mutable_device_connection_change()->mutable_status()->set_state(ccd::DEVICE_CONNECTION_OFFLINE);

        LOG_ALWAYS("\n\n==== StopCCD ====");

        //setCcdTestInstanceNum(cloudPCId);
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        {
            const char *testStr = "StopCCD";
            const char *testArg[] = { testStr };
            rv = stop_ccd_soft(1, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        CHECK_EVENT_VISIT_RESULT("StopCCDEvent", 30);
    }   


    // create context to listen to the complete event
    {
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        CHECK_EVENT_VISIT_PREP;
        ctx->alias       = "MD";
        ctx->event.mutable_device_connection_change()->set_device_id(deviceId[cloudPCId]);
        ctx->event.mutable_device_connection_change()->mutable_status()->set_state(ccd::DEVICE_CONNECTION_ONLINE);
        
        LOG_ALWAYS("\n\n==== StartCCD ====");

        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        {
            std::string alias = "CloudPC";
            CHECK_LINK_REMOTE_AGENT(alias, TEST_BASIC_STR, rv);
        }

        START_CCD(TEST_BASIC_STR, rv);

        CHECK_EVENT_VISIT_RESULT("StartCCDEvent", 30);
    }  

    // create context to listen to the complete event
    {
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        CHECK_EVENT_VISIT_PREP;
        ctx->alias       = "MD";
        ctx->event.mutable_device_info_change()->set_device_id(deviceId[cloudPCId]);
        ctx->event.mutable_device_info_change()->set_change_type(ccd::DEVICE_INFO_CHANGE_TYPE_UNLINK);
        //ctx->change_type = ccd::STORAGE_NODE_CHANGE_TYPE_DELETED;

        // Bug 10207: Add more delay as temporarily workaround to avoid deadlock in cloud pc logout.
        //            The problem is when the client detects cloud pc is online, it immediately makes
        //            connection to cloud pc (connection pool). vss_server then verify the connection and make 
        //            CCDIListLinkedDevice call, it needs to hold a cache lock. The CCD side CachePlayer::stopStorageNode 
        //            also need to hold cache lock, causing deadlock that prevents the vss_server thread from exiting.
        //            Add delay is to make sure that the connection is made well ahead of logout
        VPLThread_Sleep(VPLTIME_FROM_SEC(10));

        LOG_ALWAYS("\n\n==== StopCloudPC ====");

        //setCcdTestInstanceNum(cloudPCId);
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        {
            const char *testStr = "StopCloudPC";
            const char *testArg[] = { testStr };
            rv = stop_cloudpc(1, testArg);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        CHECK_EVENT_VISIT_RESULT("StopCloudPCEvent", 30);
    }   

    // create context to listen to the complete event
    {
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        CHECK_EVENT_VISIT_PREP;
        ctx->alias       = "MD";
        ctx->event.mutable_device_info_change()->set_device_id(deviceId[cloudPCId]);
        ctx->event.mutable_device_info_change()->set_change_type(ccd::DEVICE_INFO_CHANGE_TYPE_LINK);
        //ctx->change_type = ccd::STORAGE_NODE_CHANGE_TYPE_DELETED;

        LOG_ALWAYS("\n\n==== StartCloudPC ====");

        //setCcdTestInstanceNum(cloudPCId);
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        START_CLOUDPC(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        CHECK_EVENT_VISIT_RESULT("StartCloudPCEvent", 30);
    }   

    //setCcdTestInstanceNum(clientPCId);
    SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
    LOG_ALWAYS("\n\n==== Testing Software Update (Client) ====");
    {
        const char *testStr = "DownloadUpdates";
        const char *testArg[] = { testStr, "-T" };
        rv = download_updates(2, testArg);
        //CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        TargetDevice *myRemoteDevice = getTargetDevice();
        if(myRemoteDevice->getOsVersion() == OS_ANDROID) {
            CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_BASIC_STR, testStr, rv, "11791");
        }
        else {
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }
        if (myRemoteDevice != NULL) {
            delete myRemoteDevice;
            myRemoteDevice = NULL;
        }
        rv = 0; // reset rv to make final test PASS 
    }

    //event sync
    if (full) {
        int i = 0;
        while (i++ < 2) {
            if (check_download_sync) {
                LOG_ALWAYS("\n\n==== Testing Playlist Sync Event (Download) ====");
            }
            else {
                LOG_ALWAYS("\n\n==== Testing Playlist Sync Event (Upload) ====");
            }
            LOG_ALWAYS("\n\n== Enable playlist sync (CloudPC) ==");
            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            ENABLE_PLAYLIST_SYNC("CloudPC_Enable_Playlist_Sync");

            LOG_ALWAYS("\n\n== Get playlist paths (CloudPC) ==");
            std::string cloudpc_playlist_path;
            GET_PLAYLIST_PATH("CloudPC_Get_Playlist_Path", cloudpc_playlist_path);

            if (check_download_sync) {
                SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(clientPCId);
                }
                LOG_ALWAYS("\n\n== Enable playlist sync (Client) ==");
                ENABLE_PLAYLIST_SYNC("Client_Enable_Playlist_Sync");

                LOG_ALWAYS("\n\n== Get playlist paths (Client) ==");
                std::string client_playlist_path;
                GET_PLAYLIST_PATH("Client_Get_Playlist_Path", client_playlist_path);
            }
            // Create event check context
            LOG_ALWAYS("\n\n== Push playlist to cloudpc ==");
            CHECK_EVENT_VISIT_PREP;
            if (check_download_sync) {
                ctx->alias   = "MD";
            }
            else {
                ctx->alias   = "CloudPC";
            }
            ctx->event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PLAYLISTS);

            if (check_download_sync) {
                SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(cloudPCId);
                }
            }
            std::string playlist1 = "/pl1.txt";
            std::string playlist1_local = "/pl1.txt";
            std::string cur_dir;
            rv = getCurDir(cur_dir);
            if (rv < 0) {
                LOG_ERROR("failed to get current dir. error = %d", rv);
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "GetCurrentDir", rv);

        #ifdef WIN32
            std::replace(cur_dir.begin(), cur_dir.end(), '\\', '/');
        #endif
            playlist1_local.insert(0, cur_dir);
    
            playlist1.insert(0, cloudpc_playlist_path);

            // clean-up local dummy file
            Util_rm_dash_rf(playlist1_local);
            // create local dummy file
            rv = create_dummy_file(playlist1_local.c_str(), 1*1024);
            if (rv != 0) {
                LOG_ERROR("create_dummy_file failed. error = %d", rv);
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "PrepareDataForCreatePlaylist", rv);

            {
                TargetDevice *target = getTargetDevice();
                std::string cloudPCSeparator;
                target->getDirectorySeparator(cloudPCSeparator);
                rv = target->pushFile(playlist1_local,
                                      convert_path_convention(cloudPCSeparator, playlist1));
                if (rv != 0) {
                    LOG_ERROR("pushFile failed. error = %d", rv);
                }
                delete target;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CreatePlaylistFile", rv);

            if (check_download_sync) {
                LOG_ALWAYS("\n\n== Waiting for sync event(MD) ==");
                SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(clientPCId);
                }
            }
            CHECK_EVENT_VISIT_RESULT("PlaylistSyncEvent", 45);

            LOG_ALWAYS("\n\n== Modify playlist on cloudpc ==");
            // Create event check context
            ctx = new check_event_visitor_ctx();
            if (check_download_sync) {
                ctx->alias   = "MD";
            }
            else {
                ctx->alias   = "CloudPC";
            }
            ctx->event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PLAYLISTS);

            if (check_download_sync) {
                SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(cloudPCId);
                }
            }

            // clean-up local dummy file
            Util_rm_dash_rf(playlist1_local);
            // create local dummy file
            rv = create_dummy_file(playlist1_local.c_str(), 2*1024);
            if (rv != 0) {
                LOG_ERROR("create_dummy_file failed. error = %d", rv);
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "PrepareDataForModifyPlaylist", rv);

            {
                TargetDevice *target = getTargetDevice();
                std::string cloudPCSeparator;
                target->getDirectorySeparator(cloudPCSeparator);
                rv = target->pushFile(playlist1_local,
                                      convert_path_convention(cloudPCSeparator, playlist1));
                if (rv != 0) {
                    LOG_ERROR("pushFile failed. error = %d", rv);
                }
                delete target;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "ModifyPlaylistFile", rv);

            if (check_download_sync) {
                LOG_ALWAYS("\n\n== Waiting for sync event(MD) ==");
                SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(clientPCId);
                }
            }

            CHECK_EVENT_VISIT_RESULT("PlaylistSyncEvent", 45);

            LOG_ALWAYS("\n\n== Delete playlist on cloudpc ==");
            // Create event check context
            ctx = new check_event_visitor_ctx();
            if (check_download_sync) {
                ctx->alias   = "MD";
            }
            else {
                ctx->alias   = "CloudPC";
            }
            ctx->event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PLAYLISTS);

            if (check_download_sync) {
                SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(cloudPCId);
                }
            }

            {
                TargetDevice *target = getTargetDevice();
                std::string cloudPCSeparator;
                target->getDirectorySeparator(cloudPCSeparator);
                rv = target->deleteFile(convert_path_convention(cloudPCSeparator, playlist1));
                if (rv != 0) {
                    LOG_ERROR("deleteFile failed! rv = %d", rv);
                }
                delete target;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "DeletePlaylistFile", rv);

            if (check_download_sync) {
                LOG_ALWAYS("\n\n== Waiting for sync event(MD) ==");
                SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(clientPCId);
                }
            }
            CHECK_EVENT_VISIT_RESULT("PlaylistSyncEvent", 45);

            LOG_ALWAYS("\n\n== Create directory on cloudpc ==");
            std::string playlist_dir = "/pl_dir1";
            playlist_dir.insert(0, cloudpc_playlist_path);
            // Create event check context
            ctx = new check_event_visitor_ctx();
            if (check_download_sync) {
                ctx->alias   = "MD";
            }
            else {
                ctx->alias   = "CloudPC";
            }
            ctx->event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PLAYLISTS);

            if (check_download_sync) {
                SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(cloudPCId);
                }
            }

            {
                TargetDevice *target = getTargetDevice();
                std::string cloudPCSeparator;
                target->getDirectorySeparator(cloudPCSeparator);
                rv = target->createDir(convert_path_convention(cloudPCSeparator, playlist_dir), 777);
                if (rv != 0) {
                    LOG_ERROR("createDir failed! rv = %d", rv);
                }
                delete target;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CreatePlaylistDir", rv);

            if (check_download_sync) {
                LOG_ALWAYS("\n\n== Waiting for sync event(MD) ==");
                SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(clientPCId);
                }
            }
            CHECK_EVENT_VISIT_RESULT("PlaylistSyncEvent", 45);

            LOG_ALWAYS("\n\n== Delete directory on cloudpc ==");
            // Create event check context
            ctx = new check_event_visitor_ctx();
            if (check_download_sync) {
                ctx->alias   = "MD";
            }
            else {
                ctx->alias   = "CloudPC";
            }
            ctx->event.mutable_sync_feature_status_change()->set_feature(ccd::SYNC_FEATURE_PLAYLISTS);

            if (check_download_sync) {
                SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(cloudPCId);
                }
            }

            {
                TargetDevice *target = getTargetDevice();
                std::string cloudPCSeparator;
                target->getDirectorySeparator(cloudPCSeparator);
                rv = target->removeDirRecursive(convert_path_convention(cloudPCSeparator, playlist_dir));
                if (rv != 0) {
                    LOG_ERROR("removeDirRecursive failed! rv = %d", rv);
                }
                delete target;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "DeletePlaylistDir", rv);

            if (check_download_sync) {
                LOG_ALWAYS("\n\n== Waiting for sync event(MD) ==");
                SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
                if (rv < 0) {
                    setCcdTestInstanceNum(clientPCId);
                }
            }
            CHECK_EVENT_VISIT_RESULT("PlaylistSyncEvent", 45);
            check_download_sync = !check_download_sync;
        }

        // Currently only Windows and Linux platforms can restart CCD
        // For WinRT/iOS/Android, they need to restart dx_remote_agent to pick up the new configuration
        std::string osVersion;
        {
            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            QUERY_TARGET_OSVERSION(osVersion, TEST_BASIC_STR, rv);
        }
        if (isWindows(osVersion) || osVersion.compare(OS_LINUX) == 0) {
            {
                VPLThread_Sleep(VPLTIME_FROM_SEC(3));

                LOG_ALWAYS("\n\n==== Testing Software Update in odm group (Client), StopCCD ====");

                set_target_machine("MD");
                { 
                    const char *testStr = "StopCCD";
                    const char *testArg[] = { testStr };
                    rv = stop_ccd_soft(1, testArg);
                    CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
                }
            }

            LOG_ALWAYS("\n\n==== Testing Software Update in odm group (Client), Set group to odm ====");
            setCcdTestInstanceNum(clientPCId);
            SET_GROUP("odm", TEST_BASIC_STR, rv);

            {
                VPLThread_Sleep(VPLTIME_FROM_SEC(3));

                LOG_ALWAYS("\n\n==== Testing Software Update in odm group (Client), Start CCD ====");

                set_target_machine("MD");

                {
                    std::string alias = "MD";
                    CHECK_LINK_REMOTE_AGENT(alias, TEST_BASIC_STR, rv);
                }

                START_CCD(TEST_BASIC_STR, rv);
            }

            set_target_machine("MD");
            LOG_ALWAYS("\n\n==== Testing Software Update from odm group (Client) ====");
            {
                const char *testStr = "CheckUpdates_ODMGroup";
                const char *testArg[] = { testStr, "-T", "-C" };
                rv = download_updates(3, testArg);

                if (rv == SWU_ERR_NOT_FOUND) {
                    LOG_ALWAYS("Expected error code SWU_ERR_NOT_FOUND for checking for update in odm group");
                    rv = 0;
                }
                TargetDevice *myRemoteDevice = getTargetDevice();
                if(myRemoteDevice->getOsVersion() == OS_ANDROID) {
                    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_BASIC_STR, testStr, rv, "11791");
                }
                else {
                    CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
                }
                if (myRemoteDevice != NULL) {
                    delete myRemoteDevice;
                    myRemoteDevice = NULL;
                }
                rv = 0; // reset rv to make final test PASS
            }
        }

        LOG_ALWAYS("\n\n==== Sign in with invalid username (CloudPC) ====");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }
        {
            const char *testArg[] = { "StopCloudPC" };
            stop_cloudpc(1, testArg);
        }

        {
            std::string invalid_user = argv[2];
            invalid_user.append(".invalid");
            const char *testArgs[] = { "StartCloudPC", invalid_user.c_str(), argv[3] };
            rv = start_cloudpc(3, testArgs);
            if (rv == VPL_OK) {
                LOG_ERROR("Sign in with invalid username (%s) succeeded!", invalid_user.c_str());
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "StartCloudPC_InvalidUser", rv);
        }

        START_CLOUDPC(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Sign in with invalid password (CloudPC) ====");
        {
            const char *testArg[] = { "StopCloudPC" };
            stop_cloudpc(1, testArg);
        }

        {
            std::string invalid_pwd = argv[3];
            invalid_pwd.append(".invalid");
            const char *testArgs[] = { "StartCloudPC", argv[2], invalid_pwd.c_str() };
            rv = start_cloudpc(3, testArgs);
            if (rv == VPL_OK) {
                LOG_ERROR("Sign in with invalid password (%s) succeeded!", invalid_pwd.c_str());
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "StartCloudPC_InvalidPassword", rv);
        }

        START_CLOUDPC(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Unlink Self Session Invalidated (CloudPC) ====");
        {
            u64 userId = 0;
            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }
            rv = unlinkDevice(userId);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "UnlinkCloudPC", rv);
        }

        {
            u64 userId = 0;
            const char *testStr = "GetUserIdUnlinkCloudPC";
            rv = getUserIdBasic(&userId);
            if (rv == VPL_OK) {
                LOG_ERROR("Get UserId succeeded when cloudPC unlinked!");
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        START_CLOUDPC(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Sign in with invalid username (MD) ====");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        {
            const char *testArg[] = { "StopClient" };
            stop_client(1, testArg);
        }

        {
            std::string invalid_user = argv[2];
            invalid_user.append(".invalid");
            const char *testArgs[] = { "StartClient", invalid_user.c_str(), argv[3] };
            rv = start_client(3, testArgs);
            if (rv == VPL_OK) {
                LOG_ERROR("Sign in with invalid username (%s) succeeded!", invalid_user.c_str());
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "StartMD_InvalidUser", rv);
        }

        START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Sign in with invalid password (MD) ====");
        {
            const char *testArg[] = { "StopClient" };
            stop_client(1, testArg);
        }

        {
            std::string invalid_pwd = argv[3];
            invalid_pwd.append(".invalid");
            const char *testArgs[] = { "StartClient", argv[2], invalid_pwd.c_str() };
            rv = start_client(3, testArgs);
            if (rv == VPL_OK) {
                LOG_ERROR("Sign in with invalid password (%s) succeeded!", invalid_pwd.c_str());
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "StartMD_InvalidPassword", rv);
        }

        START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Unlink Self Session Invalidated (MD) ====");
        {
            u64 userId = 0;
            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }
            rv = unlinkDevice(userId);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "UnlinkMD", rv);
        }

        {
            u64 userId = 0;
            const char *testStr = "GetUserIdUnlinkMD";
            rv = getUserIdBasic(&userId);
            if (rv == VPL_OK) {
                LOG_ERROR("Get UserId succeeded when MD unlinked!");
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Sign in with invalid username (Client) ====");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }
        {
            const char *testArg[] = { "StopClient" };
            stop_client(1, testArg);
        }

        {
            std::string invalid_user = argv[2];
            invalid_user.append(".invalid");
            const char *testArgs[] = { "StartClient", invalid_user.c_str(), argv[3] };
            rv = start_client(3, testArgs);
            if (rv == VPL_OK) {
                LOG_ERROR("Sign in with invalid username (%s) succeeded!", invalid_user.c_str());
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "StartClient_InvalidUser", rv);
        }

        START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Sign in with invalid password (Client) ====");
        {
            const char *testArg[] = { "StopClient" };
            stop_client(1, testArg);
        }

        {
            std::string invalid_pwd = argv[3];
            invalid_pwd.append(".invalid");
            const char *testArgs[] = { "StartClient", argv[2], invalid_pwd.c_str() };
            rv = start_client(3, testArgs);
            if (rv == VPL_OK) {
                LOG_ERROR("Sign in with invalid password (%s) succeeded!", invalid_pwd.c_str());
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "StartClient_InvalidPassword", rv);
        }

        START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        LOG_ALWAYS("\n\n==== Unlink Self Session Invalidated (Client) ====");
        {
            u64 userId = 0;
            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }
            rv = unlinkDevice(userId);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "UnlinkClient", rv);
        }

        {
            u64 userId = 0;
            const char *testStr = "GetUserIdUnlinkClient";
            rv = getUserIdBasic(&userId);
            if (rv == VPL_OK) {
                LOG_ERROR("Get UserId succeeded when Client unlinked!");
                rv = VPL_ERR_FAIL;
            }
            else {
                rv = VPL_OK;
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        //Case1: On device 2, register an event queue.
        //       From device 1, unlink device 2.
        //       On device 2, call CCDIGetSystemState, set_get_logged_out_users(true).
        LOG_ALWAYS("\n\n==== Unlink by CloudPC (MD) ====");
        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        CHECK_EVENT_VISIT_PREP;
        u64 userId = 0;
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }
        ctx->alias       = "MD";
        ctx->event.mutable_user_logout()->set_user_id(userId);
        ctx->event.mutable_user_logout()->set_reason(ccd::LOGOUT_REASON_DEVICE_UNLINKED);

        SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        userId = 0;
        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            goto exit;
        }
        rv = unlinkDevice(userId, deviceId[clientPCId]);
        CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "UnlinkMDbyCloudPC", rv);

        CHECK_EVENT_VISIT_RESULT("UserLogoutEvent", 30);

        SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        {
            ccd::GetSystemStateInput ccdiRequest;
            ccdiRequest.set_get_logged_out_users(true);
            ccd::GetSystemStateOutput ccdiResponse;
            rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
            if (rv == 0) {
                if (ccdiResponse.logged_out_users().size() > 0 &&
                    ccdiResponse.logged_out_users(0).reason() == ccd::LOGOUT_REASON_DEVICE_UNLINKED) {
                    LOG_ALWAYS("User logged out for reason LOGOUT_REASON_DEVICE_UNLINKED!");
                }
                else {
                    rv = -1;
                }
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "MDUserLogoutReason", rv);
        }

        START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);

        {
            ccd::GetSystemStateInput ccdiRequest;
            ccdiRequest.set_get_logged_out_users(true);
            ccd::GetSystemStateOutput ccdiResponse;
            rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
            if (rv == 0) {
                if (ccdiResponse.logged_out_users().size() == 0) {
                    LOG_ALWAYS("Logged_out_users is empty!!!!");
                }
                else {
                    LOG_ERROR("Logged_out_users is expected to be empty.  Actual: %s",
                            ccdiResponse.DebugString().c_str());
                    rv = -1;
                }
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckLoggedOutUsers", rv);
        }

        //Case2: On device 2, shutdown CCD
        //       From device 1, unlink device 2.
        //       Start CCD again on device 2.
        //       On device 2, call CCDIGetSystemState, set_get_logged_out_users(true).
        {

            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            {
                const char *testStr = "StopCCD";
                const char *testArg[] = { testStr };
                rv = stop_ccd_soft(1, testArg);
                CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
            }

            LOG_ALWAYS("\n\n==== Unlink by CloudPC (MD Restart) ====");
            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            {
                userId = 0;
                rv = getUserIdBasic(&userId);
                if (rv != 0) {
                    LOG_ERROR("Fail to get user id:%d", rv);
                    goto exit;
                }
                rv = unlinkDevice(userId, deviceId[clientPCId]);
                CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "UnlinkMDbyCloudPC", rv);
            }

            //VSDS session cache expiration is 30s, sleep 30s before we restart MD
            VPLThread_Sleep(VPLTIME_FROM_SEC(30));

            if(clientPCOSVersion.compare(OS_WINDOWS_RT) == 0 || isIOS(clientPCOSVersion)){
                rv = launch_dx_remote_agent_app("MD");
                if (rv != 0) {
                    LOG_ERROR("Unable launch dx_remote_agent app: %d", rv);
                    goto exit;
                }
            }

            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            {
                std::string alias = "MD";
                CHECK_LINK_REMOTE_AGENT(alias, TEST_BASIC_STR, rv);

                START_CCD(TEST_BASIC_STR, rv);
                CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "START_CCD", rv);

                int retry = 0;
                while (1) {
                    ccd::GetSystemStateInput ccdiRequest;
                    ccdiRequest.set_get_logged_out_users(true);
                    ccd::GetSystemStateOutput ccdiResponse;
                    rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
                    if (rv == 0) {
                        if (ccdiResponse.logged_out_users().size() > 0) {
                            if (ccdiResponse.logged_out_users(0).reason() == ccd::LOGOUT_REASON_DEVICE_UNLINKED) {
                                LOG_ALWAYS("User logged out for reason LOGOUT_REASON_DEVICE_UNLINKED!");
                            } else {
                                LOG_ERROR("Wrong logout reason; expected LOGOUT_REASON_DEVICE_UNLINKED, got %s!",
                                        LogoutReason_t_Name(ccdiResponse.logged_out_users(0).reason()).c_str());
                                rv = -1;
                            }
                            break;
                        }
                    }
                    LOG_ALWAYS("Wait for logout reason: %d, %d", rv, retry);
                    if(++retry >= 30){
                        LOG_ERROR("Timeout waiting for logout.");
                        // This could happen if there is no network connection.
                        // Also report our ANS connection status.
                        ccd::DeviceConnectionStatus selfStatus;
                        int temp_rc = getDeviceConnectionStatus(userId, deviceId[clientPCId], selfStatus);
                        if (temp_rc == 0) {
                            LOG_ALWAYS("getDeviceConnectionStatus: %s", selfStatus.ShortDebugString().c_str());
                        } else {
                            LOG_ERROR("getDeviceConnectionStatus failed: %d", temp_rc);
                        }
                        rv = -1;
                        break;
                    }
                    VPLThread_Sleep(VPLTIME_FROM_SEC(1));
                }
                CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "MDUserLogoutReason_StopStartMD", rv);
            }

            START_CLIENT(argv[2], argv[3], TEST_BASIC_STR, true, rv);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "START_CLIENT", rv);
        }

#if defined (CLOUDNODE)
        {
            LOG_ALWAYS("\n\n==== Testing Feature and Capability bits ====");
            const char *testStr = "FeatureCapability";
            u64 userId = 0;
            u64 devId = 0;
            ccd::ListLinkedDevicesOutput listSnOut;
            ccd::ListLinkedDevicesInput request;
            ccd::ListUserStorageInput psnrequest;
            ccd::ListUserStorageOutput psnlistSnOut;   

            SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }
            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }
            LOG_ALWAYS("userId = "FMTu64, userId);
            request.set_user_id(userId);
            rv = CCDIListLinkedDevices(request, listSnOut);
            if (rv != 0) {
                LOG_ERROR("Fail to list devices:%d", rv);
                goto exit;
            }
            for (int i = 0; i < listSnOut.devices_size(); i++) {
                const ccd::LinkedDeviceInfo& curr = listSnOut.devices(i);
                LOG_ALWAYS("Device Id "FMTu64" Name %s, Status %d, updating %s is_storage_node %s", 
                         curr.device_id(), curr.device_name().c_str(),
                         (int)curr.connection_status().state(),
                         curr.connection_status().updating()? "TRUE" : "FALSE",
                         curr.is_storage_node()? "TRUE" : "FALSE");
                        // Identify if this is cloudPC based on is_storage_node()    
                        if (curr.is_storage_node()) {
                            LOG_ALWAYS("\tTHIS IS CLOUDPC");
                            devId = curr.device_id();
                            if (curr.has_feature_virt_drive_capable()) {
                                LOG_ALWAYS("\tvirt_drive_capable(%s)", curr.feature_virt_drive_capable() ? "true" : "false");
                            } else {
                                LOG_ERROR("virt_drive_capable expected for cloudnode");
                                rv = -1;
                                goto exit;
                            }
                            if (curr.has_feature_media_server_capable()) {
                                LOG_ALWAYS("\tmedia_server_capable(%s)", curr.feature_media_server_capable() ? "true" : "false");
                            } else {
                                LOG_ERROR("media_server_capable expected for cloudnode");
                                rv = -1;
                                goto exit;
                            }
                            if (curr.has_feature_remote_file_access_capable()) {
                                LOG_ALWAYS("\tmedia_server_capable(%s)", curr.feature_remote_file_access_capable() ? "true" : "false");
                            } else {
                                LOG_ERROR("remote_file_access_capable expected for cloudnode");
                                rv = -1;
                                goto exit;
                            }
                        // else it is client   
                        } else {
                            LOG_ALWAYS("\tTHIS IS CLIENT");
                            if (!curr.has_feature_virt_drive_capable()) {
                                LOG_ALWAYS("has_feature_virt_drive_capable is false as expected for client");
                            } else {
                                LOG_ERROR("virt_drive_capable not expected for client");
                                rv = -1;
                                goto exit;
                            }
                            if (!curr.has_feature_media_server_capable()) {
                                LOG_ALWAYS("has_feature_media_server_capable is false as expected for client");
                            } else {
                                LOG_ERROR("media_server_capable not expected for client");
                                rv = -1;
                                goto exit;
                            }
                            if (!curr.has_feature_remote_file_access_capable()) {
                                LOG_ALWAYS("has_feature_remote_file_access_capable is false as expected for client");
                            } else {
                                LOG_ERROR("remote_file_access_capable not expected for client");
                                rv = -1;
                                goto exit;
                            }
                        }

                        if (curr.has_os_version()) {
                            LOG_ALWAYS("\tos_version(%s)", curr.os_version().c_str());
                        } else { 
                            LOG_ALWAYS("\tNO os_version");
                        }

                        if (curr.has_protocol_version()) {
                            LOG_ALWAYS("\tprotocol_version(%s)", curr.protocol_version().c_str());
                        }
                        if (curr.has_build_info()) {
                            LOG_ALWAYS("\tbuild_info(%s)", curr.build_info().c_str());
                        }
                        if (curr.has_model_number()) {
                            LOG_ALWAYS("\tmodel_number(%s)", curr.model_number().c_str());
                        }
                        if (curr.has_feature_fsdatasettype_capable()) {
                            LOG_ERROR("\tFAIL fs_datasettype_capable(%s). Not expected for orbe", curr.feature_fsdatasettype_capable() ? "true" : "false");
                            rv = -1;
                            goto exit;
                        } else {
                            LOG_ALWAYS("\tfs_datasettype_capable(%s) as expected for orbe", curr.feature_fsdatasettype_capable() ? "true" : "false");
                        }
            }
            
            psnrequest.set_user_id(userId);
            psnrequest.set_only_use_cache(false);
                
            LOG_ALWAYS("devId = "FMTu64, devId);
                
            rv = CCDIListUserStorage(psnrequest, psnlistSnOut);
            if (rv != 0) {
                LOG_ERROR("CCDIListUserStorage for user("FMTu64") failed %d", userId, rv);
                goto exit;
            } else {

                LOG_ALWAYS("CCDIListUserStorage OK - %d", psnlistSnOut.user_storage_size());
                for (int i = 0; i < psnlistSnOut.user_storage_size(); i++) {
                    LOG_ALWAYS("storage cluster Id = "FMTu64, psnlistSnOut.user_storage(i).storageclusterid());
                    LOG_ALWAYS("Media Server Feature: %d ", psnlistSnOut.user_storage(i).featuremediaserverenabled()); 
                    if (psnlistSnOut.user_storage(i).storageclusterid() == devId) {
                        if (psnlistSnOut.user_storage(i).featuremediaserverenabled() != true) { 
                            LOG_ERROR("Media server feature not true for cloudPC. Failed test\n");
                            rv = -1;
                            goto exit;
                       }
                    }
                    LOG_ALWAYS("Virtual Drive Feature: %d ", psnlistSnOut.user_storage(i).featurevirtdriveenabled()); 
                    if (psnlistSnOut.user_storage(i).storageclusterid() == devId) {
                        if (psnlistSnOut.user_storage(i).featurevirtdriveenabled() != true) { 
                            LOG_ERROR("Virtual drive feature not true for cloudPC. Failed test\n");
                            rv = -1;
                            goto exit;
                       }
                    }
                    LOG_ALWAYS("Remote File Access Feature: %d ", psnlistSnOut.user_storage(i).featureremotefileaccessenabled()); 
                    if (psnlistSnOut.user_storage(i).storageclusterid() == devId) {
                        if (psnlistSnOut.user_storage(i).featureremotefileaccessenabled() != true) { 
                            LOG_ERROR("Remote File Access feature not true for cloudPC. Failed test\n");
                            rv = -1;
                            goto exit;
                       }
                    }
                }
                CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
            }
        }

        // make sure both cloudpc/client has the device linked info updated
        LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
        {
            std::vector<u64> deviceIds;
            u64 userId = 0;
            //u64 deviceId = 0;

            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }

            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            rv = getDeviceId(&deviceId[cloudPCId]);
            if (rv != 0) {
                LOG_ERROR("Fail to get device id:%d", rv);
                goto exit;
            }
            deviceIds.push_back(deviceId[cloudPCId]);
            LOG_ALWAYS("cloud pc deviceId: "FMTu64, deviceId[cloudPCId]);

            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }
            rv = getDeviceId(&deviceId[clientPCId]);
            if (rv != 0) {
                LOG_ERROR("Fail to get device id:%d", rv);
                goto exit;
            }
            deviceIds.push_back(deviceId[clientPCId]);
            LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[clientPCId]);

            SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId2);
            }
            rv = getDeviceId(&deviceId[clientPCId2]);
            if (rv != 0) {
                LOG_ERROR("Fail to get device id:%d", rv);
                goto exit;
            }
            deviceIds.push_back(deviceId[clientPCId2]);
            LOG_ALWAYS("client pc deviceId: "FMTu64, deviceId[clientPCId2]);

            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckCloudPCDeviceLinkStatus", rv);
            SET_TARGET_MACHINE(TEST_BASIC_STR, "MD", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }
            rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckMDDeviceLinkStatus", rv);
            SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId2);
            }
            rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckClientPCDeviceLinkStatus", rv);
        }

        {
            LOG_ALWAYS("\n\n==== Testing ListStorageNodeDatasets ====");
            const char *testStr = "StorageNodeDatasets";
            u64 userId = 0;
            bool userFound = false;
            bool datasetFound = false;
            ccd::ListStorageNodeDatasetsOutput response;
            bool is_media = false;
            cJSON2 *datasetlist  = NULL;
            std::string responseListDataset;
            std::string responseUpload;
            cJSON2 *jsonResponse = NULL;
            u64 datasetId = 0;
            std::string datasetId_str;
            std::string datasetId_str_2;
            std::stringstream ss;
            std::string subfolder = "/rf_test";
            std::string clientPCSeparator;
            std::string test_clip_file;
            std::string test_clip_file_local;
            std::string upload_dir = "[LOCALAPPDATA]/clear.fi/rf_autotest";
            std::string upload_file = upload_dir+"/"+RF_TEST_LARGE_FILE;
            std::string work_dir;
            std::string work_dir_local;
            VPLFS_file_size_t upload_size;
            std::vector<u64> deviceIds;
            u64 cloudPCDeviceId = 0;
            u64 deviceId = 0;

            // Call ListDatasets to get datasetId for ReadDir
            RF_LISTDATASET_SKIP(is_media, responseListDataset, rv);
            if (rv == 0) {
                jsonResponse = cJSON2_Parse(responseListDataset.c_str());
                if (jsonResponse == NULL) {
                    rv = -1;
                }
            }
            if (rv == 0) {
                // get the deviceList array object
                datasetlist = cJSON2_GetObjectItem(jsonResponse, "datasetList");
                if (datasetlist == NULL) {
                    rv = -1;
                }
            }
            if (rv == 0) {
                rv = -1;
                cJSON2 *node = NULL;
                cJSON2 *subNode = NULL;
                bool found = false;
                for (int i = 0; i < cJSON2_GetArraySize(datasetlist); i++) {
                    node = cJSON2_GetArrayItem(datasetlist, i);
                    if (node == NULL) {
                        continue;
                    }
                    subNode = cJSON2_GetObjectItem(node, "datasetType");
                    if (subNode == NULL || subNode->type != cJSON2_String ||
                        strcmp(subNode->valuestring, "VIRT_DRIVE") != 0
                    ) {
                        continue;
                    }
                    subNode = cJSON2_GetObjectItem(node, "datasetId");
                    if (subNode == NULL || subNode->type != cJSON2_Number) {
                        continue;
                    }
                    if (!found) {
                        datasetId = (u64) subNode->valueint;
                        ss.str("");
                        ss << datasetId;
                        datasetId_str = ss.str();
                        rv = 0;
                        found = true;
                        rv = 0;
                    } else {
                        // more than one PSN. report error
                        LOG_ERROR("More than FS dataset found!");
                        rv = -1;
                        break;
                    }
                }
            }

            if (jsonResponse) {
                cJSON2_Delete(jsonResponse);
                jsonResponse = NULL;
            }
            // The following step is only to touch the dataset, otherwise it does not show up in ListStorageNodeDatasets
            LOG_ALWAYS("Try to read directory: %s\n", subfolder.c_str());
            RF_READ_DIR_SKIP(is_media, datasetId_str, subfolder.c_str(), responseListDataset, rv);
            rv = 0;

            // ListStorageNodeDatasets needs target to be CloudPC
            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            LOG_ALWAYS("Set to cloudPCId");

            // Retrieve current userId to check if it exists in datasets returned
            rv = getUserIdBasic(&userId);
            if (rv != 0) {
                LOG_ERROR("Fail to get user id:%d", rv);
                goto exit;
            }
            LOG_ALWAYS("userId = "FMTx64, userId);
            
            rv = CCDIListStorageNodeDatasets(response);
            if (rv < 0) {
                LOG_ERROR("Failed to list storage node datasets, rv=%d\n", rv);
                goto exit;
            }
            LOG_ALWAYS("Dataset size is %d:\n", response.datasets_size());

            for (int i = 0; i < response.datasets_size(); i++) {
                LOG_ALWAYS("Dataset %d:\n", i);
                LOG_ALWAYS("User ID is "FMTx64"\n", response.datasets(i).user_id());
                if (response.datasets(i).user_id() == userId) {
                    LOG_ALWAYS("User ID found!\n");
                    userFound = true;
                    if (datasetId == response.datasets(i).dataset_id()) {
                        LOG_ALWAYS ("DatasetId found!\n");
                        datasetFound = true;
                    }
                }
                LOG_ALWAYS("Dataset ID is "FMTx64"\n", response.datasets(i).dataset_id());
            }
            if ((userFound == false) || (datasetFound == false)) {
                LOG_ERROR("UserId or DatasetID not found!\n");
                rv = -1;
            }

            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
            
            LOG_ALWAYS("\n\n==== Testing Reclaim Datasets ====");
            testStr = "Reclaim Datasets";

            // Set target to client for this test that uses remote files functions
            SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }

            rv = getDeviceId(&deviceId);
            if (rv != 0) {
                LOG_ERROR("Fail to get device id:%d", rv);
                goto exit;
            }

            {
                TargetDevice *target = getTargetDevice();

                rv = target->getDxRemoteRoot(work_dir);
                if (rv != 0) {
                    LOG_ERROR("Unable to get the work directory from target device, rv = %d", rv);
                    delete target;
                    goto exit;
                }
                rv = target->getDirectorySeparator(clientPCSeparator);
                if (rv != 0) {
                    LOG_ERROR("Unable to get the client pc path separator, rv = %d", rv);
                    goto exit;
                }

                if( target != NULL) {
                    delete target;
                    target =NULL;
                }
            }

            // convert to forward slashes no matter how
            std::replace(work_dir.begin(), work_dir.end(), '\\', '/');

            // get test clip file path
            rv = getCurDir(work_dir_local);
            if (rv < 0) {
                LOG_ERROR("failed to get current dir. error = %d", rv);
                goto exit;
            }

            test_clip_file.assign(work_dir.c_str());
            test_clip_file.append("/");
            test_clip_file.append(RF_TEST_LARGE_FILE);

            test_clip_file_local.assign(work_dir_local.c_str());
            test_clip_file_local.append("/");
            test_clip_file_local.append(RF_TEST_LARGE_FILE);

            // make sure the test clip exists
            // push the test clip from local to remote. therefore
            // 1. stat locally
            // 2. push to remote

            {
                VPLFS_stat_t stat;

                rv = VPLFS_Stat(test_clip_file_local.c_str(), &stat);
                LOG_ALWAYS("StatTestClipFile rv = %d\n", rv);
                upload_size = stat.size;
            }

            {
                TargetDevice *target = getTargetDevice();
                rv = target->pushFile(test_clip_file_local,
                                  convert_path_convention(clientPCSeparator, test_clip_file));
                if (rv != 0) {
                    // clean-up remote upload directory
                    // for cloudnode, since the user is delete and create everytime. the directories are always clean
                    LOG_ERROR("Fail to push file from local to remote: %s --> %s, rv = %d",
                          test_clip_file_local.c_str(), test_clip_file.c_str(), rv);
                    goto exit;
                }
                delete target;
            }

            rv = remotefile_mkdir_recursive(upload_dir, datasetId_str, false);
            LOG_ALWAYS("PrepareDirectoryForUpload rv = %d\n", rv);

            // uplaod the file
            BASIC_UPLOAD(is_media, datasetId_str, test_clip_file.c_str(), upload_file.c_str(), responseUpload, rv);
            if (rv != 0) {
                LOG_ERROR("Unable to submit upload file using remote files upload: %s -> %s", test_clip_file.c_str(), upload_file.c_str());
                goto exit;
            }
            LOG_ALWAYS("Uploaded file using remote files upload: %s -> %s", test_clip_file.c_str(), upload_file.c_str());
             
            {
                cJSON2 *metadata = NULL;

                LOG_ALWAYS ("datasetId_str is %s\n", datasetId_str.c_str());
                BASIC_READ_METADATA(is_media, datasetId_str, upload_file.c_str(), responseUpload, rv);

                jsonResponse = cJSON2_Parse(responseUpload.c_str());
                if (jsonResponse == NULL) {
                    rv = -1;
                }
                metadata = cJSON2_GetObjectItem(jsonResponse, "size");
                if (metadata == NULL || metadata->type != cJSON2_Number) {
                    LOG_ERROR("error while parsing metadata");
                    rv = -1;
                } 

                if (jsonResponse) {
                    cJSON2_Delete(jsonResponse);
                    jsonResponse = NULL;
                }
            }
            
            // StopCloudPC to unlink
            LOG_ALWAYS("StopCloudPC to unlink\n");
            set_target_machine("CloudPC");
            {
                const char *testArg[] = { "StopCloudPC" };
                stop_cloudpc(1, testArg);
            }

            LOG_ALWAYS("StartCloudPC to link again\n");
            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            START_CLOUDPC(argv[2], argv[3], TEST_BASIC_STR, true, rv);

            rv = getDeviceId(&cloudPCDeviceId);
            if (rv != 0) {
                LOG_ERROR("Fail to get device id:%d", rv);
                goto exit;
            }

            deviceIds.push_back(deviceId);
            deviceIds.push_back(cloudPCDeviceId);

            rv = wait_for_devices_to_be_online_by_alias(TEST_BASIC_STR, "CloudPC", clientPCId, userId, deviceIds, 20);
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, "CheckClientDeviceLinkStatus", rv);
            
            // Call ListDatasets to get datasetId for ReadDir
            RF_LISTDATASET_SKIP(is_media, responseListDataset, rv);
            if (rv == 0) {
                jsonResponse = cJSON2_Parse(responseListDataset.c_str());
                if (jsonResponse == NULL) {
                    rv = -1;
                }
            }
            if (rv == 0) {
                // get the deviceList array object
                datasetlist = cJSON2_GetObjectItem(jsonResponse, "datasetList");
                if (datasetlist == NULL) {
                    rv = -1;
                }
            }
            if (rv == 0) {
                rv = -1;
                cJSON2 *node = NULL;
                cJSON2 *subNode = NULL;
                bool found = false;
                for (int i = 0; i < cJSON2_GetArraySize(datasetlist); i++) {
                    node = cJSON2_GetArrayItem(datasetlist, i);
                    if (node == NULL) {
                        continue;
                    }
                    subNode = cJSON2_GetObjectItem(node, "datasetType");
                    if (subNode == NULL || subNode->type != cJSON2_String ||
                        strcmp(subNode->valuestring, "VIRT_DRIVE") != 0
                    ) {
                        continue;
                    }
                    subNode = cJSON2_GetObjectItem(node, "datasetId");
                    if (subNode == NULL || subNode->type != cJSON2_Number) {
                        continue;
                    }
                    if (!found) {
                        datasetId = (u64) subNode->valueint;
                        ss.str("");
                        ss << datasetId;
                        datasetId_str_2 = ss.str();
                        rv = 0;
                        found = true;
                        rv = 0;
                    } else {
                        // more than one PSN. report error
                        LOG_ERROR("More than FS dataset found!");
                        rv = -1;
                        break;
                    }
                }
            }

            if (jsonResponse) {
                cJSON2_Delete(jsonResponse);
                jsonResponse = NULL;
            }
            {
                cJSON2 *metadata = NULL;

                LOG_ALWAYS ("datasetId_str2 is %s\n", datasetId_str_2.c_str());
                // Read MetaData to check if file still exists
                BASIC_READ_METADATA(is_media, datasetId_str_2, upload_file.c_str(), responseUpload, rv);
                if (rv != 0) {
                    LOG_ERROR("Error reading metadata for file rv = %d\n", rv);
                    goto exit;
                }
                LOG_ALWAYS("Read metadata for file succeeded.\n");

                jsonResponse = cJSON2_Parse(responseUpload.c_str());
                if (jsonResponse == NULL) {
                    rv = -1;
                }
                metadata = cJSON2_GetObjectItem(jsonResponse, "size");
                if (metadata == NULL || metadata->type != cJSON2_Number) {
                    LOG_ERROR("error while parsing metadata");
                    rv = -1;
                } 

                if (jsonResponse) {
                    cJSON2_Delete(jsonResponse);
                    jsonResponse = NULL;
                }
            }
            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }
        
        {
            const char *testStr = "ServiceDiscovery";
            ccd::ReportLanDevicesInput request;
            ccd::LanDeviceInfo *info;      
            ccd::ListLanDevicesInput requestList;
            ccd::ListLanDevicesOutput responseList; 
            u64 cloudPCDeviceId = 0;
            std::string TEST_UUID = "dummy_uuid";
            std::string TEST_DEVICE_NAME = "dummy_sevice_name";
            std::string TEST_IP_V4_ADDRESS = "xxx.yyy.zzz.www";
            int TEST_LAN_DEVICE_TYPE = 1;
            int TEST_LAN_INTERFACE_TYPE = 1;
            u64 TEST_CLOUDPC_DEVICE_ID_2 = 2;
            std::string TEST_UUID_2 = "dummy_uuid_2";
            std::string TEST_DEVICE_NAME_2 = "dummy_sevice_name_2";
            std::string TEST_IP_V4_ADDRESS_2 = "xx2.yy2.zz2.ww2";
            int TEST_LAN_DEVICE_TYPE_2 = 1;
            int TEST_LAN_INTERFACE_TYPE_2 = 2;

            LOG_ALWAYS("\n\n==== Testing Service Discovery APIs ====");

            // Set target to cloudPC to get deviceId
            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }

            rv = getDeviceId(&cloudPCDeviceId);
            if (rv != 0) {
                LOG_ERROR("Fail to get device id:%d", rv);
                goto exit;
            }
            LOG_ALWAYS ("CloudPC device ID is %llu\n", cloudPCDeviceId);

            info = request.add_infos();
            info->set_type((ccd::LanDeviceType_t)(TEST_LAN_DEVICE_TYPE));
            info->set_device_id(cloudPCDeviceId);
            info->set_uuid(TEST_UUID);
            info->set_device_name(TEST_DEVICE_NAME);
            info->mutable_route_info()->set_type((ccd::LanInterfaceType_t)(TEST_LAN_INTERFACE_TYPE));
            info->mutable_route_info()->set_ip_v4_address(TEST_IP_V4_ADDRESS);

            info = request.add_infos();
            info->set_type((ccd::LanDeviceType_t)(TEST_LAN_DEVICE_TYPE_2));
            info->set_device_id(TEST_CLOUDPC_DEVICE_ID_2);
            info->set_uuid(TEST_UUID_2);
            info->set_device_name(TEST_DEVICE_NAME_2);
            info->mutable_route_info()->set_type((ccd::LanInterfaceType_t)(TEST_LAN_INTERFACE_TYPE_2));
            info->mutable_route_info()->set_ip_v4_address(TEST_IP_V4_ADDRESS_2);

            // Set target to client for this test
            SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }

            CHECK_EVENT_VISIT_PREP;
            ctx->alias       = "Client";
            ctx->event.mutable_lan_devices_change()->set_timestamp(0);
            
            rv = CCDIReportLanDevices(request);
            if (rv < 0) {
                LOG_ERROR("Failed to report LAN devices, rv=%d\n", rv);
                goto exit;
            }
            LOG_ALWAYS ("ReportLanDevices finished.\n");        

            CHECK_EVENT_VISIT_RESULT("LanDevicesChangeEvent", 30);

            requestList.set_include_unregistered(true);
            requestList.set_include_registered_but_not_linked(true);
            requestList.set_include_linked(true);

            rv = CCDIListLanDevices(requestList, responseList);
            if (rv < 0) {
                LOG_ERROR("Failed to list LAN devices, rv=%d\n", rv);
                goto exit;
            }
            LOG_ALWAYS ("ListLanDevices finished. %u devices returned.\n", responseList.infos_size());        
            if (responseList.infos_size() != 2) {
                LOG_ERROR("%u infos returned. 2 expected\n", responseList.infos_size());
                rv = -1;
                goto exit;
            }

            for (int i = 0; i < responseList.infos_size(); i++) {
                LOG_ALWAYS("Device type is %u\n", responseList.infos(i).type());
                if (i == 0) {
                    if (responseList.infos(i).type() != TEST_LAN_DEVICE_TYPE) {
                        LOG_ERROR("Device type is %u expected %u\n", responseList.infos(i).type(), TEST_LAN_DEVICE_TYPE);
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("uuid is %s\n", responseList.infos(i).uuid().c_str());
                    if (responseList.infos(i).uuid().compare(TEST_UUID) != 0) {
                        LOG_ERROR("uuid is %s expected %s\n", responseList.infos(i).uuid().c_str(), TEST_UUID.c_str());
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("Device name is %s\n", responseList.infos(i).device_name().c_str());
                    if (responseList.infos(i).device_name().compare(TEST_DEVICE_NAME) != 0) {
                        LOG_ERROR("device_name is %s expected %s\n", responseList.infos(i).device_name().c_str(), TEST_DEVICE_NAME.c_str());
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).has_device_id()) {
                        LOG_ALWAYS("Device ID is 0x%llx\n", responseList.infos(i).device_id());
                        if (cloudPCDeviceId != responseList.infos(i).device_id()) {
                            LOG_ERROR("Device Id is 0x%llx expected 0x%llx\n", responseList.infos(i).device_id(), cloudPCDeviceId);
                            rv = -1;
                            goto exit;
                        }
                    } else {
                        LOG_ERROR ("DeviceId expected. None found\n");
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("LAN interface type is %u\n", responseList.infos(i).route_info().type());
                    if (responseList.infos(i).route_info().type() != TEST_LAN_INTERFACE_TYPE) {
                        LOG_ERROR("LAN interface type is %u expected %u\n", responseList.infos(i).route_info().type(), TEST_LAN_INTERFACE_TYPE);
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("IPv4 address is %s\n", responseList.infos(i).route_info().ip_v4_address().c_str());
                    if (responseList.infos(i).route_info().ip_v4_address().compare(TEST_IP_V4_ADDRESS) != 0) {
                        LOG_ERROR("IPv4 address is %s expected %s\n", responseList.infos(i).route_info().ip_v4_address().c_str(), TEST_IP_V4_ADDRESS.c_str());
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_ip_v6_address()) {
                        LOG_ERROR ("IPV6 address not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_media_server_port()) {
                        LOG_ERROR ("Media server port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_virtual_drive_port()) {
                        LOG_ERROR ("Virtual drive port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_web_front_port()) {
                        LOG_ERROR ("Web front port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_tunnel_service_port()) {
                        LOG_ERROR ("Tunnel service port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_ext_tunnel_service_port()) {
                        LOG_ERROR ("External tunnel service port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).has_pd_instance_id()) {
                        LOG_ERROR ("Pd instance id not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).has_notifications()) {
                        LOG_ERROR ("Has notifications not expected\n");
                        rv = -1;
                        goto exit;
                    }
                } else if (i == 1) {
                    if (responseList.infos(i).type() != TEST_LAN_DEVICE_TYPE_2) {
                        LOG_ERROR("Device type is %u expected %u\n", responseList.infos(i).type(), TEST_LAN_DEVICE_TYPE_2);
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("uuid is %s\n", responseList.infos(i).uuid().c_str());
                    if (responseList.infos(i).uuid().compare(TEST_UUID_2) != 0) {
                        LOG_ERROR("uuid is %s expected %s\n", responseList.infos(i).uuid().c_str(), TEST_UUID_2.c_str());
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("Device name is %s\n", responseList.infos(i).device_name().c_str());
                    if (responseList.infos(i).device_name().compare(TEST_DEVICE_NAME_2) != 0) {
                        LOG_ERROR("device_name is %s expected %s\n", responseList.infos(i).device_name().c_str(), TEST_DEVICE_NAME_2.c_str());
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).has_device_id()) {
                        LOG_ALWAYS("Device ID is 0x%llx\n", responseList.infos(i).device_id());
                        if (TEST_CLOUDPC_DEVICE_ID_2 != responseList.infos(i).device_id()) {
                            LOG_ERROR("Device Id is 0x%llx expected 0x%llx\n", responseList.infos(i).device_id(), TEST_CLOUDPC_DEVICE_ID_2);
                            rv = -1;
                            goto exit;
                        }
                    } else {
                        LOG_ERROR ("DeviceId expected. None found\n");
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("LAN interface type is %u\n", responseList.infos(i).route_info().type());
                    if (responseList.infos(i).route_info().type() != TEST_LAN_INTERFACE_TYPE_2) {
                        LOG_ERROR("LAN interface type is %u expected %u\n", responseList.infos(i).route_info().type(), TEST_LAN_INTERFACE_TYPE_2);
                        rv = -1;
                        goto exit;
                    }
                    LOG_ALWAYS("IPv4 address is %s\n", responseList.infos(i).route_info().ip_v4_address().c_str());
                    if (responseList.infos(i).route_info().ip_v4_address().compare(TEST_IP_V4_ADDRESS_2) != 0) {
                        LOG_ERROR("IPv4 address is %s expected %s\n", responseList.infos(i).route_info().ip_v4_address().c_str(), TEST_IP_V4_ADDRESS_2.c_str());
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_ip_v6_address()) {
                        LOG_ERROR ("IPV6 address not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_media_server_port()) {
                        LOG_ERROR ("Media server port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_virtual_drive_port()) {
                        LOG_ERROR ("Virtual drive port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_web_front_port()) {
                        LOG_ERROR ("Web front port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_tunnel_service_port()) {
                        LOG_ERROR ("Tunnel service port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).route_info().has_ext_tunnel_service_port()) {
                        LOG_ERROR ("External tunnel service port not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).has_pd_instance_id()) {
                        LOG_ERROR ("Pd instance id not expected\n");
                        rv = -1;
                        goto exit;
                    }
                    if (responseList.infos(i).has_notifications()) {
                        LOG_ERROR ("Has notifications not expected\n");
                        rv = -1;
                        goto exit;
                    }
                }
            }

            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        {
            const char *testStr = "ProbeLanDevices";

            LOG_ALWAYS("\n\n==== Testing CCDIProbeLanDevices API ====");
            
            // Set target to client for this test
            SET_TARGET_MACHINE(TEST_BASIC_STR, "Client", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(clientPCId);
            }

            CHECK_EVENT_VISIT_PREP;
            ctx->alias       = "Client";
            ctx->event.mutable_lan_devices_probe_request()->set_timestamp(0);

            rv = CCDIProbeLanDevices();
            if (rv < 0) {
                LOG_ERROR("Failed to probe LAN devices, rv=%d\n", rv);
                goto exit;
            }
            LOG_ALWAYS ("ProbeLanDevices finished.\n");        

            CHECK_EVENT_VISIT_RESULT("ProbeLanDevicesChangeEvent", 30);

            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }

        {
            const char *testStr = "InMemoryLogging";

            LOG_ALWAYS("\n\n==== Testing CCDI In Memory Logging API ====");

            // Set target to cloudPC 
            SET_TARGET_MACHINE(TEST_BASIC_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
			#if (defined(LINUX) || defined(__CLOUDNODE__)) && !defined(LINUX_EMB)
            {
                rv = CCDIDisableInMemoryLogging();
                if (rv < 0) {
                    LOG_ERROR("Failed to disable in memory logging, rv=%d\n", rv);
                    goto exit;
                }
                LOG_ALWAYS ("CCDIDisableInMemoryLogging finished.\n");        

            }

            {
                rv = CCDIEnableInMemoryLogging();
                if (rv < 0) {
                    LOG_ERROR("Failed to enable in memory logging, rv=%d\n", rv);
                    goto exit;
                }
                LOG_ALWAYS ("CCDIEnableInMemoryLogging finished.\n");        

            }
			#endif
            {
                rv = CCDIFlushInMemoryLogs();
                if (rv < 0) {
                    LOG_ERROR("Failed to flush memory logs, rv=%d\n", rv);
                    goto exit;
                }
                LOG_ALWAYS ("CCDIFlushInMemoryLogs finished.\n");        

            }

            CHECK_AND_PRINT_RESULT(TEST_BASIC_STR, testStr, rv);
        }
#endif // CLOUDNODE
    }

exit:
    LOG_ALWAYS("\n\n== Freeing cloud PC ==");
    //setCcdTestInstanceNum(cloudPCId);
    set_target_machine("CloudPC");
    {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing MD ==");
    //setCcdTestInstanceNum(clientPCId);
    set_target_machine("MD");
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client ==");
    //setCcdTestInstanceNum(clientPCId2);
    set_target_machine("Client");
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
