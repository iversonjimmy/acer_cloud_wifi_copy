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

#include <gvm_file_utils.hpp>
#include <ccdi_client_tcp.hpp>
#include "scopeguard.hpp"

#include "dx_common.h"
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "TargetDevice.hpp"

#include "autotest_common_utils.hpp"

#include "cJSON2.h"

#include <string>

void setCcdTestInstanceNum(int id)
{
    LOG_ALWAYS("Setting testInstanceNum %d", id);
    testInstanceNum = id;    // DXShell global variable need to set to the instanceId desired.
    CCDIClient_SetTestInstanceNum(id);
}

int file_compare_range(const char* src, const char* dst, VPLFile_offset_t from, VPLFS_file_size_t length) {

    const int BUF_SIZE = 16 * 1024;

    int rv = 0;
    VPLFS_stat_t stat_src, stat_dst;
    VPLFile_handle_t handle_src = 0, handle_dst = 0;
    char *src_buf = NULL;
    char *dst_buf = NULL;
    ssize_t src_read, dst_read;

    VPLFile_offset_t ret_seek;

    if (src == NULL || dst == NULL) {
        LOG_ERROR("file path is NULL");
        return -1;
    }
    LOG_ALWAYS("comparing %s with %s", src, dst);
    if (VPLFS_Stat(src, &stat_src)) {
        LOG_ERROR("fail to stat src file: %s", src);
        return -1;
    }
    if (VPLFS_Stat(dst, &stat_dst)) {
        LOG_ERROR("fail to stat dst file: %s", dst);
        return -1;
    }

    // not equal
    if (length != stat_dst.size) {
        LOG_ERROR("file size doesn't match: length = "FMTu64", dst = "FMTu64,
                  length, stat_dst.size);
        return -1;
    }
    if (stat_src.size < stat_dst.size) {
        LOG_ERROR("file size of dst is bigger: src= "FMTu64", dst = "FMTu64,
                  stat_src.size, stat_dst.size);
        return -1;
    }

    // open file
    handle_src = VPLFile_Open(src, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_src)) {
        LOG_ERROR("cannot open src file: %s", src);
        rv = -1;
        goto exit;
    }
    handle_dst = VPLFile_Open(dst, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_dst)) {
        LOG_ERROR("cannot open dst file: %s", dst);
        rv = -1;
        goto exit;
    }
    src_buf = new char[BUF_SIZE];
    if (src_buf == NULL) {
        LOG_ERROR("cannot allocate memory for src buffer");
        rv = -1;
        goto exit;
    }
    dst_buf = new char[BUF_SIZE];
    if (dst_buf == NULL) {
        LOG_ERROR("cannot allocate memory for dst buffer");
        rv = -1;
        goto exit;
    }

    ret_seek = VPLFile_Seek(handle_src, from, VPLFILE_SEEK_SET);
    if(ret_seek != from){
        LOG_ERROR("src VPLFile_Seek failed!!");
        rv = -1;
        goto exit;
    }

    do {
        src_read = VPLFile_Read(handle_src, src_buf, BUF_SIZE);
        dst_read = VPLFile_Read(handle_dst, dst_buf, BUF_SIZE);
        if (src_read < 0) {
            LOG_ERROR("error while reading src: %s, %d", src, src_read);
            rv = -1;
            break;
        }
        if (dst_read < 0) {
            LOG_ERROR("error while reading dst: %s, %d", dst, dst_read);
            rv = -1;
            break;
        }
        //if (src_read != dst_read || memcmp(src_buf, dst_buf, src_read)) {
        if (memcmp(src_buf, dst_buf, dst_read)) {
            LOG_ERROR("file is different, dst_read:%d", dst_read);
            rv = -1;
            break;
        }
    } while (src_read == BUF_SIZE && dst_read == BUF_SIZE);

exit:
    if (VPLFile_IsValidHandle(handle_src)) {
        VPLFile_Close(handle_src);
    }
    if (VPLFile_IsValidHandle(handle_dst)) {
        VPLFile_Close(handle_dst);
    }
    if (src_buf != NULL) {
        delete [] src_buf;
        src_buf = NULL;
    }
    if (dst_buf != NULL) {
        delete [] dst_buf;
        dst_buf = NULL;
    }
    return rv;
}

// Check if CloudPC get access handle  successfully after CloudPC login
// This make sure CloudPC are ready before streaming
int wait_for_cloudpc_get_accesshandle(const u64 &userId, const u64 &cloudpc_deviceID, int max_retry)
{
    int rv = VPL_OK;
    int ans_retry = 0;
    u64 accesshandle = 0;
    bool accesshandle_ready = false;

    ccd::ListUserStorageInput psnrequest;
    ccd::ListUserStorageOutput psnlistSnOut;   
    psnrequest.set_user_id(userId);
    psnrequest.set_only_use_cache(false);

    while (!accesshandle_ready && ans_retry++ < max_retry) {
        rv = CCDIListUserStorage(psnrequest, psnlistSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListUserStorage for user("FMTu64") failed %d", userId, rv);
            goto exit;
        } else {
            LOG_ALWAYS("CCDIListUserStorage OK - %d", psnlistSnOut.user_storage_size());
            rv = -1;
            for (int i = 0; i < psnlistSnOut.user_storage_size(); i++) {
                if (psnlistSnOut.user_storage(i).storageclusterid() == cloudpc_deviceID) {
                    if (psnlistSnOut.user_storage(i).has_accesshandle()) { 
                        accesshandle = psnlistSnOut.user_storage(i).accesshandle();
                        LOG_ALWAYS("CloudPC's accesshandle: "FMTu64, accesshandle); 
                    }
                }
            }
        }
        if (accesshandle != 0) {
            accesshandle_ready = true;
            break;
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));
    }

exit:
    return accesshandle_ready? VPL_OK : VPL_ERR_FAIL;
}

/// Switch to the CcdInstanceID and call ListLinkedDevices to make sure all
/// the devices listed in \a deviceIDs are linked and in the requested state.
static int wait_for_devices(int CcdInstanceID,
        u64 userId,
        const std::vector<u64>& deviceIDs,
        int max_retry,
        ccd::DeviceConnectionState_t requestedState)
{
    int rv = VPL_OK;
    int ans_retry = 0;
    bool all_devices_ready = false;

    ccd::ListLinkedDevicesOutput lldOut;
    ccd::ListLinkedDevicesInput request;

    request.set_user_id(userId);
    request.set_only_use_cache(true);

    if(CcdInstanceID > 0)
        setCcdTestInstanceNum(CcdInstanceID);

    while (!all_devices_ready && ans_retry++ < max_retry) {
        LOG_ALWAYS("Waiting for "FMTu_size_t" device(s) to be %s."
                   " ccd ID = %d, ans_retry = %d",
                   deviceIDs.size(), DeviceConnectionState_t_Name(requestedState).c_str(),
                   CcdInstanceID, ans_retry);

        rv = CCDIListLinkedDevices(request, lldOut);
        if (rv != 0) {
            LOG_ERROR("Fail to list devices:%d", rv);
            return rv;
        }
        for (int i = 0; i < lldOut.devices_size(); i++) {
            const ccd::LinkedDeviceInfo& curr = lldOut.devices(i);
            LOG_ALWAYS("DeviceId "FMTu64", %s (%d), updating=%s",
                       curr.device_id(),
                       ccd::DeviceConnectionState_t_Name(curr.connection_status().state()).c_str(),
                       (int)curr.connection_status().state(),
                       curr.connection_status().updating()? "TRUE" : "FALSE");
        }
        // scan all devices and make sure it's updated
        all_devices_ready = true;
        for (unsigned int deviceIdx = 0; deviceIdx < deviceIDs.size(); deviceIdx++) {
            // Compare device with the devices listed in ListSnOut and make sure it's in the requested state.
			bool isLinked = false;
            bool isUpdating = false;
            ccd::DeviceConnectionState_t currState;
            for (int i = 0; i < lldOut.devices_size(); i++) {
                const ccd::LinkedDeviceInfo& curr = lldOut.devices(i);
                if (curr.device_id() == deviceIDs[deviceIdx]) {
                    isLinked = true;
                    isUpdating = curr.connection_status().updating();
                    currState = curr.connection_status().state();
					break;
				}
            }
            // Device not found or not in requested state. Break and wait for next round.
            if (!isLinked) {
                LOG_ALWAYS("[%u]: Still waiting for device "FMTu64" (not linked)", deviceIdx, deviceIDs[deviceIdx]);
                all_devices_ready = false;
                break;
			} else if (isUpdating || (currState != requestedState)) {
                LOG_ALWAYS("[%u]: Still waiting for device "FMTu64" (%s%s)", deviceIdx, deviceIDs[deviceIdx],
                        DeviceConnectionState_t_Name(currState).c_str(), (isUpdating ? ", updating" : ""));
                all_devices_ready = false;
                break;
            } else {
                LOG_ALWAYS("[%u]: Device "FMTu64" is ready.", deviceIdx, deviceIDs[deviceIdx]);
            }
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));
    }
    return all_devices_ready? VPL_OK : VPL_ERR_FAIL;
}

/// Switch to the CcdInstanceID and call ListLinkedDevices to make sure all
/// the devices listed in \a deviceIDs are linked and ONLINE.
int wait_for_devices_to_be_online(int CcdInstanceID,
                                   const u64 &userId,
                                   const std::vector<u64> &deviceIDs,
                                   const int max_retry)
{
    return wait_for_devices(CcdInstanceID, userId, deviceIDs, max_retry, ccd::DEVICE_CONNECTION_ONLINE);
}

/// Switch to the CcdInstanceID and call ListLinkedDevices to make sure all
/// the devices listed in \a deviceIDs are linked and OFFLINE.
int wait_for_devices_to_be_offline(int CcdInstanceID,
                                   const u64 &userId,
                                   const std::vector<u64> &deviceIDs,
                                   const int max_retry)
{
    return wait_for_devices(CcdInstanceID, userId, deviceIDs, max_retry, ccd::DEVICE_CONNECTION_OFFLINE);
}

int wait_for_devices_to_be_online_by_alias(const std::string tc_name,
                                            const std::string &alias,
                                            int CcdInstanceID,
                                            const u64 &userId,
                                            const std::vector<u64> &deviceIDs,
                                            const int max_retry)
{
    int rv = 0;
    SET_TARGET_MACHINE(tc_name.c_str(), alias.c_str(), rv);
    if (rv < 0) {
        rv = wait_for_devices_to_be_online(CcdInstanceID, userId, deviceIDs, max_retry);
    } else {
        rv = wait_for_devices_to_be_online(-1, userId, deviceIDs, max_retry);
    }

exit:
    return rv;
}

// check whether the "json" string is a valid JSON format and contains "errMsg" string attribute
// in additional, if "expected' string is provided, it will do the comparison as well
int check_json_errmsg(std::string& json, const std::string &expected) {
    cJSON2 *jsonResponse = cJSON2_Parse(json.c_str());
    if (!jsonResponse) {
        LOG_ERROR("Invalid JSON response");
        return -1;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, jsonResponse);

    cJSON2 *errMsg = cJSON2_GetObjectItem(jsonResponse, "errMsg");
    if (errMsg == NULL) {
        LOG_ERROR("Unable to get errMsg attribute from JSON response");
        return -1;
    }
    if (errMsg->type != cJSON2_String) {
        LOG_ERROR("errMsg is not type string: %d", errMsg->type);
        return -1;
    }
    if (!expected.empty() && expected != errMsg->valuestring) {
        LOG_ERROR("Unexpected errMsg: %s != %s", errMsg->valuestring, expected.c_str());
        return -1;
    }
    LOG_DEBUG("valid errMsg: %s", errMsg->valuestring);
    return 0;
}

int remotefile_mkdir_recursive(const std::string &dir_path, const std::string &datasetId_str, bool is_media)
{
    int rv = 0;
    size_t idx = 0;
    std::string response;

    // pave the way for making upload folder
    // create each level of the upload folder if any
    LOG_ALWAYS("\n== Create directory (%s) recursively, media_rf? %d ==",
               dir_path.c_str(), is_media);

    do {
        idx = dir_path.find_first_of("/", idx+1);
        std::string parent_dir = dir_path.substr(0, idx);
        LOG_ALWAYS("checking directory %s, media_rf? %d", parent_dir.c_str(), is_media);

        RF_READ_DIR_SKIP(is_media, datasetId_str, parent_dir.c_str(), response, rv);
        if (rv) {
            RF_MAKE_DIR_SKIP(is_media, datasetId_str, parent_dir.c_str(), response, rv);

            // double check to make sure it exist
            RF_READ_DIR_SKIP(is_media, datasetId_str, parent_dir.c_str(), response, rv);
            if (rv) {
                if (is_media &&
                    ((response == "{\"errMsg\":\"Folder Access Denied\"}") ||
                     (response.find("\"errMsg\":\"No access via media_rf;") != response.npos))) {
                    LOG_ALWAYS("MediaRF fail to read/make directory %s due to access denied, skip",
                               parent_dir.c_str());
                } else {
                    LOG_ALWAYS("Failed to read/make directory %s, media_rf? %d",
                               parent_dir.c_str(), is_media);
                    return rv;
                }
            }
        }
    } while (idx != std::string::npos);

exit:
    return rv;
}

int create_dummy_file(const char* dst, VPLFS_file_size_t size)
{
    int rv = 0;
 
    VPLFile_handle_t handle_dst;
    VPLFile_offset_t ret_seek;

    ssize_t dst_write = 0;

    handle_dst = VPLFile_Open(dst, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);

    if (!VPLFile_IsValidHandle(handle_dst)) {
        LOG_ERROR("cannot open dst file: %s", dst);
        rv = -1;
        goto exit;
    }

    ret_seek = VPLFile_Seek(handle_dst, size, VPLFILE_SEEK_SET);
    if(ret_seek != size){
        LOG_ERROR("VPLFile_Seek failed!!");
        rv = -1;
        goto exit;
    }

    dst_write = VPLFile_Write(handle_dst, "x", 1);
    if(dst_write != 1){
        LOG_ERROR("VPLFile_Write failed!!");
        rv = -1;
        goto exit;
    }

exit:
    if (VPLFile_IsValidHandle(handle_dst)) {
        VPLFile_Sync(handle_dst);
        VPLFile_Close(handle_dst);
    }

    return rv;
}

std::string convert_path_convention(const std::string &separator, const std::string &path) {
    std::string new_path = path;
    if (separator == "/") {
        std::replace(new_path.begin(), new_path.end(), '\\', '/');
    } else {
        std::replace(new_path.begin(), new_path.end(), '/', '\\');
    }
    return new_path;
}

int check_link_dx_remote_agent(const std::string &alias) {
    int rc = VPL_OK;
    int retry = 0;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("target is NULL, return!");
        return rc;
    }

    do {
        rc = target->checkRemoteAgent();
        if (rc != VPL_OK) {
            LOG_ERROR("checkRemoteAgent[%s] failed! %d. Check communication with dx_remote_agent!", alias.c_str(), rc);
        }
        else {
            LOG_ALWAYS("DxRemoteAgent[%s] is connected!", alias.c_str());
            break;
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));
    } while (retry++ < 15);

    if (target != NULL)
        delete target;
    return rc;
}

int get_target_osversion(std::string &osversion) {
    int rc = VPL_OK;
    bool retry = true;
    TargetDevice *target = getTargetDevice();

    osversion = target->getOsVersion();
    if(osversion.empty() && retry) {
        VPLThread_Sleep(VPLTIME_FROM_SEC(2));
        osversion = target->getOsVersion();
    }

    if (target != NULL) {
        delete target;
        target = NULL;
    }

    if(osversion.empty()) {
        rc = -1;
        LOG_ERROR("could not get osversion info from target device");
    }

    return rc;
}
