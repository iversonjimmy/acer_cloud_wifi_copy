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

#ifndef AUTOTEST_COMMON_UTILS_HPP_
#define AUTOTEST_COMMON_UTILS_HPP_

#include <vpl_th.h>
#include <vplex_file.h>
#include <gvm_file_utils.hpp>
#include <ccdi_client_tcp.hpp>

#include "EventQueue.hpp"

#include <string>
#include <vector>

#define CHECK_AND_PRINT_EXPECTED_TO_FAIL(testsuite, subtestname, result, bug) { \
        if (result < 0) {                                        \
            LOG_ERROR("%s fail rv (%d)", subtestname, result);   \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=AutoTest_%s_%s(BUG %s)", \
                       (result == 0)? "PASS":"EXPECTED_TO_FAIL", testsuite, subtestname, bug); \
        } else {                                                 \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=AutoTest_%s_%s(BUG %s)", \
                       (result == 0)? "PASS":"EXPECTED_TO_FAIL", testsuite, subtestname, bug); \
        }                                                        \
}

#define CHECK_AND_PRINT_RESULT(testsuite, subtestname, result) { \
        if (result < 0) {                                        \
            LOG_ERROR("%s fail rv (%d)", subtestname, result);   \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=AutoTest_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
            goto exit;                                           \
        } else {                                                 \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=AutoTest_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
        }                                                        \
}

#define START_CCD(tc_name, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testStr = "StartCCD"; \
        const char *testArg[] = { testStr }; \
        rv = start_ccd(1, testArg); \
        CHECK_AND_PRINT_RESULT(tc_name, testStr, rc); \
	} \
    END_MULTI_STATEMENT_MACRO

#define SET_TARGET_MACHINE(tc_name, alias, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testStr = "SetTargetMachine"; \
        rc = set_target_machine(alias); \
        if (rc == VPL_ERR_FAIL) { \
            CHECK_AND_PRINT_RESULT(tc_name, testStr, rc); \
        } \
	} \
    END_MULTI_STATEMENT_MACRO

#define SET_GROUP(groupName, tc_name, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
    const char *testStr = "SetGroup"; \
    const char *testArg[] = { testStr, groupName }; \
    rc = set_group(2, testArg); \
    CHECK_AND_PRINT_RESULT(tc_name, testStr, rc); \
	} \
    END_MULTI_STATEMENT_MACRO

#define START_CLOUDPC(user_name, pwd, tc_name, retry, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testArgs[] = { "StartCloudPC", user_name, pwd }; \
        rc = start_cloudpc(3, testArgs); \
        if (rc < 0 && retry) { \
            /**/ \
            rc = start_cloudpc(3, testArgs); \
        } \
        CHECK_AND_PRINT_RESULT(tc_name, "StartCloudPC", rc); \
	} \
    END_MULTI_STATEMENT_MACRO

#define START_CLIENT(user_name, pwd, tc_name, retry, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testArgs[] = { "StartClient", user_name, pwd }; \
        rc = start_client(3, testArgs); \
        if (rc < 0 && retry) { \
            /**/ \
            rc = start_client(3, testArgs); \
        } \
        CHECK_AND_PRINT_RESULT(tc_name, "StartClient", rc); \
	} \
    END_MULTI_STATEMENT_MACRO

#define QUERY_TARGET_OSVERSION(osversion, tc_name, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
        rc = get_target_osversion(osversion); \
        CHECK_AND_PRINT_RESULT(tc_name, "Get_Target_OSVersion", rc); \
	} \
    END_MULTI_STATEMENT_MACRO

#define CHECK_LINK_REMOTE_AGENT(alias, tc_name, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
        rc = check_link_dx_remote_agent(alias); \
        CHECK_AND_PRINT_RESULT(tc_name, "CheckDxRemoteAgent", rc); \
	} \
    END_MULTI_STATEMENT_MACRO

// Note: This function must be called before login for sandbox application(iOS and WinRT)
// Or it will not sync down any metadata.
#define UPDATE_APP_STATE(tc_name, rc) \
	BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testStr1 = "Power"; \
        const char *testStr2 = "FgMode"; \
        const char *testStr3 = "dx_remote_agent"; \
        const char *testStr4 = "5"; \
        const char *testStr5 = "1"; \
        const char *testArg[] = { testStr1, testStr2, testStr3, testStr4, testStr5 }; \
        rc = power_dispatch(5, testArg); \
        CHECK_AND_PRINT_RESULT(tc_name, "UpdateAppState", rc); \
	} \
    END_MULTI_STATEMENT_MACRO

#define METADATA_SYNC_TIMEOUT   300

#define BASIC_TEMPLATE_OP(op, num, skip, is_media_rf, arg1, arg2, arg3, arg4, resp, rc) \
    BEGIN_MULTI_STATEMENT_MACRO \
    const char *command = (is_media_rf == true) ? "MediaRF" : "RemoteFile"; \
    const char *tc_name = "SdkBasicRelease2"; \
    const char *testArgs[] = { command, op, arg1, arg2, arg3, arg4 }; \
    resp.clear(); \
    rc = dispatch_fstest_cmd_with_response(num, testArgs, resp); \
    if (skip == false) { \
        CHECK_AND_PRINT_RESULT(tc_name, op, rc); \
    } else { \
        if (rc != VPL_OK && rc != VPL_ERR_TIMEOUT && rc != VPL_ERR_CONNREFUSED) { \
            int rv = check_json_errmsg(resp); \
            if (rv == -1) { \
                std::string op_invalid_errmsg = op; \
                op_invalid_errmsg.append("_InvalidErrorMessage"); \
                /*CHECK_AND_PRINT_RESULT(tc_name, op_invalid_errmsg.c_str(), rc);*/\
            } \
        } \
    } \
    END_MULTI_STATEMENT_MACRO

#define RF_TEMPLATE_OP(op, num, skip, is_media_rf, arg1, arg2, arg3, arg4, resp, rc) \
    BEGIN_MULTI_STATEMENT_MACRO \
    const char *command = (is_media_rf == true) ? "MediaRF" : "RemoteFile"; \
    const char *tc_name = (is_media_rf == true) ? "SdkRemoteFileRelease_MediaRF" : "SdkRemoteFileRelease_RF"; \
    const char *testArgs[] = { command, op, arg1, arg2, arg3, arg4 }; \
    resp.clear(); \
    rc = dispatch_fstest_cmd_with_response(num, testArgs, resp); \
    if (skip == false) { \
        CHECK_AND_PRINT_RESULT(tc_name, op, rc); \
    } else { \
        if (rc != VPL_OK && rc != VPL_ERR_TIMEOUT && rc != VPL_ERR_CONNREFUSED) { \
            int rv = check_json_errmsg(resp); \
            if (rv == -1) { \
                std::string op_invalid_errmsg = op; \
                op_invalid_errmsg.append("_InvalidErrorMessage"); \
                /*CHECK_AND_PRINT_RESULT(tc_name, op_invalid_errmsg.c_str(), rc);*/\
            } \
        } \
    } \
    END_MULTI_STATEMENT_MACRO

#define RF_XUPLOAD(is_mrf, ds, from, to, resp, rc)           RF_TEMPLATE_OP("AsyncUpload",  5, false, is_mrf, ds.c_str(), to,    from, NULL, resp, rc)
#define RF_DELETE_DIR(is_mrf, ds, path, resp, rc)            RF_TEMPLATE_OP("DeleteDir",    4, false, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_DELETE_FILE(is_mrf, ds, path, resp, rc)           RF_TEMPLATE_OP("DeleteFile",   4, false, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_DOWNLOAD(is_mrf, ds, from, to, resp, rc)          RF_TEMPLATE_OP("Download",     5, false, is_mrf, ds.c_str(), from,  to,   NULL, resp, rc)
#define RF_DOWNLOAD_RANGE(is_mrf, ds, from, to, rr, rp, rc)  RF_TEMPLATE_OP("Download",     5, false, is_mrf, ds.c_str(), from,  to,     rr,   rp, rc)
#define RF_GET_ALL_PROGRESS(is_mrf, resp, rc)                RF_TEMPLATE_OP("GetProgress",  2, false, is_mrf, NULL,       NULL,  NULL, NULL, resp, rc)
#define RF_GET_PROGRESS(is_mrf, req, resp, rc)               RF_TEMPLATE_OP("GetProgress",  3, false, is_mrf, req,        NULL,  NULL, NULL, resp, rc)
#define RF_MAKE_DIR(is_mrf, ds, path, resp, rc)              RF_TEMPLATE_OP("MakeDir",      4, false, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_READ_DIR(is_mrf, ds, path, resp, rc)              RF_TEMPLATE_OP("ReadDir",      4, false, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_READ_METADATA(is_mrf, ds, path, resp, rc)         RF_TEMPLATE_OP("ReadMetadata", 4, false, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_UPLOAD(is_mrf, ds, from, to, resp, rc)            RF_TEMPLATE_OP("Upload",       5, false, is_mrf, ds.c_str(), to,    from, NULL, resp, rc)
#define RF_READ_DIR_PAGI(is_mrf, ds, path, pagi, resp, rc)   RF_TEMPLATE_OP("ReadDir",      5, false, is_mrf, ds.c_str(), path,  pagi, NULL, resp, rc)

#define RF_XUPLOAD_SKIP(is_mrf, ds, from, to, resp, rc)      RF_TEMPLATE_OP("AsyncUpload",  5,  true, is_mrf, ds.c_str(), to,    from, NULL, resp, rc)
#define RF_GET_PROGRESS_SKIP(is_mrf, req, resp, rc)          RF_TEMPLATE_OP("GetProgress",  3,  true, is_mrf, req,        NULL,  NULL, NULL, resp, rc)
#define RF_LISTDATASET_SKIP(is_mrf, resp, rc)                RF_TEMPLATE_OP("ListDatasets", 2,  true, is_mrf, NULL,       NULL,  NULL, NULL, resp, rc)
#define RF_LISTDEVICE_SKIP(is_mrf, resp, rc)                 RF_TEMPLATE_OP("ListDevices",  2,  true, is_mrf, NULL,       NULL,  NULL, NULL, resp, rc)
#define RF_MAKE_DIR_SKIP(is_mrf, ds, path, resp, rc)         RF_TEMPLATE_OP("MakeDir",      4,  true, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_DELETE_DIR_SKIP(is_mrf, ds, path, resp, rc)       RF_TEMPLATE_OP("DeleteDir",    4,  true, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_READ_DIR_SKIP(is_mrf, ds, path, resp, rc)         RF_TEMPLATE_OP("ReadDir",      4,  true, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_MOVE_DIR_SKIP(is_mrf, ds, from, to, resp, rc)     RF_TEMPLATE_OP("RenameDir",    5,  true, is_mrf, ds.c_str(), from,    to, NULL, resp, rc)
#define RF_COPY_FILE_SKIP(is_mrf, ds, from, to, resp, rc)    RF_TEMPLATE_OP("CopyFile",     5,  true, is_mrf, ds.c_str(), from,    to, NULL, resp, rc)
#define RF_MOVE_FILE_SKIP(is_mrf, ds, from, to, resp, rc)    RF_TEMPLATE_OP("MoveFile",     5,  true, is_mrf, ds.c_str(), from,    to, NULL, resp, rc)
#define RF_DELETE_FILE_SKIP(is_mrf, ds, path, resp, rc)      RF_TEMPLATE_OP("DeleteFile",   4,  true, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_READ_METADATA_SKIP(is_mrf, ds, path, resp, rc)    RF_TEMPLATE_OP("ReadMetadata", 4,  true, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)
#define RF_SETPERM_SKIP(is_mrf, ds, path, flags, resp, rc)   RF_TEMPLATE_OP("SetPermission",5,  true, is_mrf, ds.c_str(), path, flags, NULL, resp, rc)
#define RF_GET_ALL_PROGRESS_SKIP(is_mrf, resp, rc)           RF_TEMPLATE_OP("GetProgress",  2,  true, is_mrf, NULL,       NULL,  NULL, NULL, resp, rc)
#define RF_DOWNLOAD_SKIP(is_mrf, ds, from, to, resp, rc)     RF_TEMPLATE_OP("Download",     5,  true, is_mrf, ds.c_str(), from,    to, NULL, resp, rc)

#define BASIC_UPLOAD(is_mrf, ds, from, to, resp, rc)         BASIC_TEMPLATE_OP("Upload",       5, false, is_mrf, ds.c_str(), to,    from, NULL, resp, rc)
#define BASIC_READ_METADATA(is_mrf, ds, path, resp, rc)      BASIC_TEMPLATE_OP("ReadMetadata", 4, false, is_mrf, ds.c_str(), path,  NULL, NULL, resp, rc)

struct check_event_visitor_ctx {
    bool done;
    ccd::CcdiEvent event;
    u32 count;
    u32 expected_count;
    u64 userid;
    std::string alias;
    EventQueue eq;
    VPLSem_t sem;
    
    check_event_visitor_ctx() :
        done(false),
        count(0)
    {
        VPLSem_Init(&sem, 1, 0);
    }

    ~check_event_visitor_ctx()
    {
        VPLSem_Destroy(&sem);
    }
};

void setCcdTestInstanceNum(int id);
int file_compare_range(const char* src, const char* dst, VPLFile_offset_t from, VPLFS_file_size_t length);
int check_json_errmsg(std::string& json, const std::string &expected="");
int remotefile_mkdir_recursive(const std::string &dir_path, const std::string &datasetId_str, bool is_media);

int wait_for_cloudpc_get_accesshandle(const u64 &userId, const u64 &cloudpc_deviceID, int max_retry);
int wait_for_devices_to_be_online(int CcdInstanceID,
                                   const u64 &userId,
                                   const std::vector<u64> &deviceIDs,
                                   const int max_retry);
int wait_for_devices_to_be_offline(int CcdInstanceID,
                                   const u64 &userId,
                                   const std::vector<u64> &deviceIDs,
                                   const int max_retry);
int wait_for_devices_to_be_online_by_alias(const std::string tc_name,
                                            const std::string &alias,
                                            int CcdInstanceID,
                                            const u64 &userId,
                                            const std::vector<u64> &deviceIDs,
                                            const int max_retry);

std::string convert_path_convention(const std::string &separator, const std::string &path);

int create_dummy_file(const char* dst, VPLFS_file_size_t size);

int check_link_dx_remote_agent(const std::string &alias);

int get_target_osversion(std::string &osversion);

#endif // #ifndef AUTOTEST_COMMON_UTILS_HPP_
