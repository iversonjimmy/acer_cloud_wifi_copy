//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv

#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include "TargetDevice.hpp"
#include "mca_diag.hpp"
#include <ccdi.hpp>

#include <vpl_time.h>
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vplex_plat.h>

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>

#include <vplu_types.h>
#include "gvm_file_utils.hpp"
#include <log.h>

#define MODIFY_CCD_INSTANCE_CLOUD_ROOT(root) {              \
    if (testInstanceNum) {                                   \
        std::ostringstream instanceSubfolder;           \
        instanceSubfolder << "/CCD_" << testInstanceNum;     \
        root += instanceSubfolder.str();         \
    }                                                   \
}

int checkRunningCcd()
{
    LOG_ALWAYS("2014/05/29 PM6.15.");
    LOG_ALWAYS("Check ccd instance not present.");
    setDebugLevel(LOG_LEVEL_CRITICAL);  // Avoid confusing log where it gives error if ccd is not detected yet
    LOG_ALWAYS("after set debug level.");
    ccd::GetSystemStateInput request;
    LOG_ALWAYS("after get system state input.");
    request.set_get_players(true);
    ccd::GetSystemStateOutput response;
    LOG_ALWAYS("after get system state ouput.");
    int rv;
    LOG_ALWAYS("before CCDI get system state");
    rv = CCDIGetSystemState(request, response);
    LOG_ALWAYS("after CCDI get system state");
    resetDebugLevel();
    LOG_ALWAYS("after reset debug level");
    if (rv == CCD_OK) {
        LOG_ALWAYS("CCD is already present!\n");
    }
    return (rv == 0) ? -1 : 0;  // Only a success if CCDIGetSystemState failed.
}


int login(const std::string& username,
          const std::string& password,
          u64& userId)
{
    LOG_INFO("start");
    int rv = 0;
    userId=0;

    ccd::LoginInput loginRequest;
    loginRequest.set_user_name(username);
    loginRequest.set_password(password);
    ccd::LoginOutput loginResponse;
    LOG_INFO(">>>> CCDILogin");
    rv = CCDILogin(loginRequest, loginResponse);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDILogin fail: %d", rv);
    } else {
        userId = loginResponse.user_id();
        LOG_INFO("CCDILogin OK [userId: "FMTu64"]", userId);
    }
    return rv;
}

int login_with_eula(const std::string& username,
                    const std::string& password,
                    const bool eula,
                    u64& userId)
{
    LOG_INFO("start");
    int rv = 0;
    userId=0;

    ccd::LoginInput loginRequest;
    loginRequest.set_user_name(username);
    loginRequest.set_password(password);
    loginRequest.set_ac_eula_agreed(eula);
    ccd::LoginOutput loginResponse;
    LOG_INFO(">>>> CCDILogin");
    rv = CCDILogin(loginRequest, loginResponse);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDILogin fail: %d", rv);
    } else {
        userId = loginResponse.user_id();
        LOG_INFO("CCDILogin OK [userId: "FMTu64"]", userId);
    }
    return rv;
}

int linkDevice(u64 userId)
{
    LOG_INFO("start");
    int rv = 0;
    ccd::LinkDeviceInput linkInput;

    TargetDevice *target = getTargetDevice();

    LOG_INFO(">>>> CCDILinkDevice");
    linkInput.set_user_id(userId);
    linkInput.set_device_name(target->getDeviceName().c_str());
    linkInput.set_is_acer_device(/*target->getIsAcerDevice()*/true); 
    linkInput.set_device_has_camera(target->getDeviceHasCamera()); 
    linkInput.set_device_class(target->getDeviceClass().c_str());
    linkInput.set_os_version(target->getOsVersion().c_str());
    LOG_INFO("Linking device %s for user "FMTu64"", target->getDeviceName().c_str(), userId);
    rv = CCDILinkDevice(linkInput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDILinkDevice fail: %d", rv);
    } else {
        LOG_INFO("CCDILinkDevice OK"); 
    }

    delete target;
    return rv;
}

int getDeviceConnectionStatus(u64 userId, u64 deviceId, ccd::DeviceConnectionStatus& status_out)
{
    int rv = 0;
    ccd::ListLinkedDevicesInput lldIn;
    lldIn.set_user_id(userId);
    lldIn.set_only_use_cache(true);
    ccd::ListLinkedDevicesOutput lldOut;
    rv = CCDIListLinkedDevices(lldIn, lldOut);
    if (rv != 0) {
        LOG_ERROR("CCDIListLinkedDevices for user("FMTu64") failed: %d", userId, rv);
        return rv;
    }

    for (int i = 0; i < lldOut.devices_size(); i++) {
        const ccd::LinkedDeviceInfo& curr = lldOut.devices(i);
        if (curr.device_id() == deviceId) {
            status_out = curr.connection_status();
            return 0;
        }
    }
    LOG_ERROR("Requested device("FMTu64") was not found for user("FMTu64")!", deviceId, userId);
    return -1;
}

int listLinkedDevice(u64 userId, std::vector<u64>* sleepingDevices_out)
{
    LOG_INFO("start");

    int rv = 0;
    ccd::ListLinkedDevicesOutput listSnOut;

    {
        ccd::ListLinkedDevicesInput request;
        request.set_user_id(userId);
        request.set_only_use_cache(true);
        rv = CCDIListLinkedDevices(request, listSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListLinkedDevices for user("FMTu64") failed %d", userId, rv);
        } else {
            LOG_INFO("CCDIListLinkedDevices OK");
            for (int i = 0; i < listSnOut.devices_size(); i++) {
                const char *stateStr;
                const ccd::LinkedDeviceInfo& curr = listSnOut.devices(i);
                LOG_INFO("device[%d]: name(%s) id("FMTu64")",
                         i,
                         curr.device_name().c_str(),
                         curr.device_id());
                if (curr.is_storage_node()) {
                    LOG_INFO("\t**Storage Node**");
                }
                LOG_INFO("\tdevice class(%s)", curr.device_class().c_str());

                switch ((int)curr.connection_status().state()) {
                    case ccd::DEVICE_CONNECTION_OFFLINE:
                        stateStr = "OFFLINE";
                        break;
                    case ccd::DEVICE_CONNECTION_ONLINE:
                        stateStr = "ONLINE";
                        break;
                    case ccd::DEVICE_CONNECTION_STANDBY:
                        stateStr = "STANDBY";
                        if (sleepingDevices_out != NULL) {
                            sleepingDevices_out->push_back(curr.device_id());
                        }
                        break;
                    default:
                        stateStr = "*UNEXPECTED*";
                        break;
                }

                LOG_INFO("\tonline state(%s%s)",
                        (curr.connection_status().updating() ? "UNKNOWN, was previously " : ""),
                        stateStr);
                if (curr.has_os_version()) {
                    LOG_INFO("\tos_version(%s)", curr.os_version().c_str());
                } 

                if (curr.has_protocol_version()) {
                    LOG_INFO("\tprotocol_version(%s)", curr.protocol_version().c_str());
                }
                if (curr.has_build_info()) {
                    LOG_INFO("\tbuild_info(%s)", curr.build_info().c_str());
                }
                if (curr.has_model_number()) {
                    LOG_INFO("\tmodel_number(%s)", curr.model_number().c_str());
                }
                if (curr.has_feature_media_server_capable()) {
                    LOG_INFO("\tmedia_server_capable(%s)", curr.feature_media_server_capable() ? "true" : "false");
                }
                if (curr.has_feature_virt_drive_capable()) {
                    LOG_INFO("\tvirt_drive_capable(%s)", curr.feature_virt_drive_capable() ? "true" : "false");
                }
                if (curr.has_feature_remote_file_access_capable()) {
                    LOG_INFO("\tremote_file_access_capable(%s)", curr.feature_remote_file_access_capable() ? "true" : "false");
                }
                if (curr.has_feature_fsdatasettype_capable()) {
                    LOG_INFO("\tfs_datasettype_capable(%s)", curr.feature_fsdatasettype_capable() ? "true" : "false");
                }
            }
        }
    }

    return rv;
}

int listUserStorage(u64 userId, bool cache_only)
{
    LOG_INFO("start");

    int rv = 0;
    ccd::ListUserStorageOutput listSnOut;

    {
        ccd::ListUserStorageInput request;
        request.set_user_id(userId);
        request.set_only_use_cache(cache_only);
        rv = CCDIListUserStorage(request, listSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListUserStorage for user("FMTu64") failed %d", userId, rv);
        } else {
            LOG_INFO("CCDIListUserStorage OK - %d",
                listSnOut.user_storage_size());
            for (int i = 0; i < listSnOut.user_storage_size(); i++) {
                LOG_INFO("\n%s",
                    listSnOut.user_storage(i).DebugString().c_str());
                LOG_INFO("%d: has %d SA", i, listSnOut.user_storage(i).storageaccess_size());
            }
        }
    }

    return rv;
}

int updateStorageNode(u64 userId, u64 deviceId,
                      bool media_server_is_set,
                      bool media_server_enabled,
                      bool virt_drive_is_set,
                      bool virt_drive_enabled,
                      bool remote_file_access_is_set,
                      bool remote_file_access_enabled)
{
    LOG_INFO("start");

    int rv = 0;

    {
        ccd::UpdateStorageNodeInput request;
        request.set_user_id(userId);
        if ( deviceId != 0 ) {
            request.set_device_id(deviceId);
        }
        if ( media_server_is_set ) {
            request.set_feature_media_server_enabled(media_server_enabled);
        }
        if ( virt_drive_is_set ) {
            request.set_feature_virt_drive_enabled(virt_drive_enabled);
        }
        if ( remote_file_access_is_set ) {
            request.set_feature_remote_file_access_enabled(
                remote_file_access_enabled);
        }
        rv = CCDIUpdateStorageNode(request);
        if (rv != 0) {
            LOG_ERROR("CCDIUpdateStorageNode for user("FMTu64") failed %d", userId, rv);
        } else {
            LOG_INFO("CCDIUpdateStorageNode OK");
        }
    }

    return rv;
}

int unsubscribe_clearfi(u64 userId)
{
    LOG_INFO("start");
    int         rv = 0;
    int         i;
    s32         numDatasets; 

    LOG_INFO(">>>> CCDIListOwnedDatasets");
    ccd::ListOwnedDatasetsInput ownedDatasetsInput;
    ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
    ownedDatasetsInput.set_user_id(userId);
    ownedDatasetsInput.set_only_use_cache(false);
    rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIListOwnedDatasets fail: %d", rv);
        return -1;
    } else {
        LOG_INFO("CCDIListOwnedDatasets OK");
    }
    LOG_INFO("   Num datasets %d", ownedDatasetsOutput.dataset_details_size());
    numDatasets = ownedDatasetsOutput.dataset_details_size();

    LOG_INFO(">>>> CCDIDeleteSyncSubscription");
    for (i = 0; i < numDatasets; i++) {
        ccd::DeleteSyncSubscriptionsInput delSyncSubInput;
        u64 datasetId = ownedDatasetsOutput.dataset_details(i).datasetid(); 
        std::string datasetName = ownedDatasetsOutput.dataset_details(i).datasetname(); 

        if (ownedDatasetsOutput.dataset_details(i).datasettype() == vplex::vsDirectory::CLEAR_FI) {
            delSyncSubInput.set_user_id(userId);
            delSyncSubInput.add_dataset_ids(datasetId);
            rv = CCDIDeleteSyncSubscriptions(delSyncSubInput);
            if (rv != CCD_OK) {
                LOG_WARN("CCDIDeleteSyncSubscription failed for CLEARFI dataset_id "FMTu64": %d", datasetId, rv);
            }
            break;
        }
    }

    return 0;
}

static int subscribeDatasetHelper(u64 userId, u64 datasetId,
                                  bool useType,
                                  ccd::SyncSubscriptionType_t subType,
                                  const std::string& appDataPath,
                                  const std::string& dsetName,
                                  bool useAppDataPathOnly,
                                  u64 maxSizeBytes, bool useMaxBytes,
                                  u64 maxFiles, bool useMaxFiles)
{
    int rv = 0;
    std::string subscriptionPath;
    if(useAppDataPathOnly) {
        subscriptionPath = Util_CleanupPath(appDataPath);
    }else{
        char temp[17];
        snprintf(temp, ARRAY_SIZE_IN_BYTES(temp), "%016"PRIx64, userId);
        subscriptionPath = Util_CleanupPath(appDataPath + "/" +
                                            temp + "/" + dsetName);
    }
    LOG_ALWAYS("Subscribing %s dataset: %s",
               dsetName.c_str(), subscriptionPath.c_str());

    ccd::AddSyncSubscriptionInput addSyncSubInput;
    addSyncSubInput.set_user_id(userId);
    addSyncSubInput.set_dataset_id(datasetId);
    addSyncSubInput.set_device_root(subscriptionPath);
    if(useType) {
        addSyncSubInput.set_subscription_type(subType);
    }
    if(useMaxBytes) {
        addSyncSubInput.set_max_size(maxSizeBytes);
    }
    if(useMaxFiles) {
        addSyncSubInput.set_max_files(maxFiles);
    }
    rv = CCDIAddSyncSubscription(addSyncSubInput);
    if(rv == DATASET_ALREADY_SUBSCRIBED) {
        LOG_ERROR("Old %s subscription found, please use command StopCloudPC/StopClient "
                  "to unsubscribe.  Subscription variables may be different.",
                  dsetName.c_str());
    }else if (rv != CCD_OK) {
        LOG_ERROR("CCDIAddSyncSubscription failed for %s.  userId:"FMTx64" dataset_id "FMTx64": %d",
                  dsetName.c_str(), userId, datasetId, rv);
    } else {
        LOG_INFO("CCDIAddSyncSubscription for %s is OK.", dsetName.c_str());
    }
    return rv;
}

int subscribeDatasetCrUp(u64 userId, u64 datasetId,
                         bool useType,
                         ccd::SyncSubscriptionType_t subType,
                         const std::string& appDataPath,
                         const std::string& dsetName,
                         bool useAppDataPathOnly)
{
    int rv;
    rv = subscribeDatasetHelper(userId, datasetId,
                                useType,
                                subType,
                                appDataPath,
                                dsetName,
                                useAppDataPathOnly,
                                0, false,
                                0, false);
    if(rv != 0) {
        LOG_ERROR("subscribeDatasetHelper:%d", rv);
    }
    return rv;
}

int subscribeDatasetCrDown(u64 userId, u64 datasetId,
                           bool useType,
                           ccd::SyncSubscriptionType_t subType,
                           const std::string& appDataPath,
                           const std::string& dsetName,
                           u64 maxSizeBytes, bool useMaxBytes,
                           u64 maxFiles, bool useMaxFiles)
{
    int rv;
    rv = subscribeDatasetHelper(userId, datasetId,
                                useType,
                                subType,
                                appDataPath,
                                dsetName,
                                false,
                                maxSizeBytes, useMaxBytes,
                                maxFiles, useMaxFiles);
    if(rv != 0) {
        LOG_ERROR("subscribeDatasetHelper:%d", rv);
    }
    return rv;
}

int unsubscribeDataset(u64 userId, u64 datasetId, const std::string& debugDsetName)
{
    int rv = 0;
    ccd::DeleteSyncSubscriptionsInput delSyncSubInput;
    delSyncSubInput.set_user_id(userId);
    delSyncSubInput.add_dataset_ids(datasetId);
    rv = CCDIDeleteSyncSubscriptions(delSyncSubInput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIDeleteSyncSubscription failed for %s user_id:"FMTx64", dataset_id:"FMTx64", %d",
                  debugDsetName.c_str(), userId, datasetId, rv);
    } else {
        LOG_INFO("CCDIDeleteSyncSubscription for %s OK", debugDsetName.c_str());
    }
    return rv;
}

int getUserId(u64& userId)
{
    int rv = 0;
    {
        ccd::GetSystemStateInput ccdiRequest;
        ccdiRequest.set_get_players(true);
        ccd::GetSystemStateOutput ccdiResponse;
        rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
            goto out;
        }
        userId = ccdiResponse.players().players(0).user_id();
        if (userId == 0) {
            LOG_ERROR("Not signed-in!");
            rv = -1;
            goto out;
        }
    }
    {
        ccd::GetSyncStateInput ccdiRequest;
        ccdiRequest.set_user_id(userId);
        ccdiRequest.set_only_use_cache(true);
        ccd::GetSyncStateOutput ccdiResponse;
        rv = CCDIGetSyncState(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "CCDIGetSyncState", rv);
            goto out;
        }
        if (!ccdiResponse.is_device_linked()) {
            LOG_WARN("Local cache does not currently show local device as linked.");
        }
    }
 out:
    return rv;
}

int getDatasetId(u64 userId, const std::string& name, u64& datasetId_out)
{
    int rv = 0;
    int numDatasets;
    int i;

    LOG_INFO(">>>> CCDIListOwnedDatasets");
    ccd::ListOwnedDatasetsInput ownedDatasetsInput;
    ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
    ownedDatasetsInput.set_user_id(userId);
    ownedDatasetsInput.set_only_use_cache(false);
    rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIListOwnedDatasets fail: %d", rv);
        return rv;
    } else {
        LOG_INFO("CCDIListOwnedDatasets OK");
    }

    LOG_INFO("   Num datasets %d", ownedDatasetsOutput.dataset_details_size());
    numDatasets = ownedDatasetsOutput.dataset_details_size();

    rv = -1;
    for (i = 0; i < numDatasets; i++) {
        std::string datasetName = ownedDatasetsOutput.dataset_details(i).datasetname();

        if (datasetName == name) {
            rv = 0;
            datasetId_out = ownedDatasetsOutput.dataset_details(i).datasetid();
            break;
        }
    }
    if(rv != 0) {
        LOG_ERROR("User:"FMTx64" Dataset:%s does not exist", userId, name.c_str());
    }
    return rv;
}


int subscribe_clearfi(const std::string &appDataPath, u64 userId, u64 &clearfiDatasetId)
{
    LOG_INFO("start");
    int         rv = 0;
    int         i;
    s32         numDatasets;

    LOG_INFO(">>>> CCDIListOwnedDatasets");
    ccd::ListOwnedDatasetsInput ownedDatasetsInput;
    ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
    ownedDatasetsInput.set_user_id(userId);
    ownedDatasetsInput.set_only_use_cache(false);
    rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIListOwnedDatasets fail: %d", rv);
        return -1;
    } else {
        LOG_INFO("CCDIListOwnedDatasets OK");
    }

    LOG_INFO("   Num datasets %d", ownedDatasetsOutput.dataset_details_size());
    numDatasets = ownedDatasetsOutput.dataset_details_size();

    LOG_INFO(">>>> CCDIAddSyncSubscription");
    for (i = 0; i < numDatasets; i++) {
        ccd::AddSyncSubscriptionInput addSyncSubInput;
        u64 datasetId = ownedDatasetsOutput.dataset_details(i).datasetid();
        std::string datasetName = ownedDatasetsOutput.dataset_details(i).datasetname();

        if (ownedDatasetsOutput.dataset_details(i).datasettype() ==
                   vplex::vsDirectory::CLEAR_FI) {
            char temp[17];
            snprintf(temp, ARRAY_SIZE_IN_BYTES(temp), "%016"PRIx64, userId);
            std::string s_metadataSubscriptionPath =
                    Util_CleanupPath(appDataPath + "/" +  temp + "/clear.fi");
            LOG_ALWAYS("Subscribing clearfi dataset: %s", s_metadataSubscriptionPath.c_str());

            addSyncSubInput.set_user_id(userId);
            addSyncSubInput.set_device_root(s_metadataSubscriptionPath);
            addSyncSubInput.set_dataset_id(datasetId);
            addSyncSubInput.set_subscription_type(ccd::SUBSCRIPTION_TYPE_NORMAL);
            rv = CCDIAddSyncSubscription(addSyncSubInput);
            if (rv == DATASET_ALREADY_SUBSCRIBED) {
                LOG_ERROR("Old clearfi subscription found, please use command StopCloudPC/StopClient to unsubscribe");
                return -1;
            } else if (rv != CCD_OK) {
                LOG_ERROR("CCDIAddSyncSubscription failed for dataset_id "FMTu64": %d", datasetId, rv);
                return -1;
            } else {
                LOG_INFO("CCDIAddSyncSubscription for clear.fi OK");
                clearfiDatasetId = datasetId;
            }
        }
    }

    return 0;
}

int getDatasetRoot(u64 userId,
                   const std::string& name,
                   std::string& datasetRoot)
{
    int rv = 0;
    int numDatasets;
    int i;

    LOG_INFO(">>>> CCDIListSyncSubscriptions");
    ccd::ListSyncSubscriptionsInput subInput;
    ccd::ListSyncSubscriptionsOutput subOutput;
    subInput.set_user_id(userId);
    subInput.set_only_use_cache(false);
    rv = CCDIListSyncSubscriptions(subInput, subOutput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIListSyncSubscriptions fail: %d", rv);
        return rv;
    } else {
        LOG_INFO("CCDIListSyncSubscriptions OK");
    }

    LOG_INFO("   Num subscribed datasets %d", subOutput.subs_size());
    numDatasets = subOutput.subs_size();

    rv = -1;
    for (i = 0; i < numDatasets; i++) {
        std::string datasetName = subOutput.subs(i).dataset_details().datasetname();

        if (datasetName == name) {
            rv = 0;
            datasetRoot = subOutput.subs(i).absolute_device_root();
            break;
        }
    }
    if(rv != 0) {
        LOG_WARN("User:"FMTx64" Dataset:%s not subscribed.  May be ok if subscription no longer required.", userId, name.c_str());
    }
    return rv;
}

int getLocalHttpInfo(u64 userId,
                     std::string &base_url,
                     std::string &service_ticket,
                     std::string &session_handle)
{
    int rv = 0;

    ccd::GetLocalHttpInfoInput req;
    ccd::GetLocalHttpInfoOutput res;
    req.set_user_id(userId);
    req.set_service(ccd::LOCAL_HTTP_SERVICE_REMOTE_FILES);
    rv = CCDIGetLocalHttpInfo(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIGetLocalHttpInfo failed: %d", rv);
        goto end;
    }

    base_url = res.url_prefix();
    service_ticket = res.service_ticket();
    session_handle = res.session_handle();

 end:
    return rv;
}

int logout(u64 userId)
{
    int rv = 0;
    ccd::LogoutInput logoutRequest;
    logoutRequest.set_local_user_id(userId);
    LOG_INFO(">>>> CCDILogout");
    rv = CCDILogout(logoutRequest);

    if (rv == CCD_ERROR_NOT_SIGNED_IN)
        rv = CCD_OK;

    if (rv != CCD_OK) {
        LOG_ERROR("CCDILogout: %d", rv);
    } else {
        LOG_INFO("CCDILogout OK");
    }
    return rv;
}

int unlinkDevice(u64 userId)
{
    int rv;
    ccd::UnlinkDeviceInput unlinkInput;
    LOG_INFO(">>>> Unlinking local device for user "FMTu64"...", userId);
    unlinkInput.set_user_id(userId);
    rv = CCDIUnlinkDevice(unlinkInput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIUnlinkDevice failed: %d", rv);
        return -1;
    } else {
        LOG_INFO("CCDIUnlinkDevice OK");
        return 0;
    }
}

int unlinkDevice(u64 userId, u64 deviceId)
{
    int rv;
    ccd::UnlinkDeviceInput unlinkInput;
    LOG_INFO(">>>> Unlinking local device for user "FMTu64"...", userId);
    unlinkInput.set_user_id(userId);
    if (deviceId != 0) {
        unlinkInput.set_device_id(deviceId);
    }
    rv = CCDIUnlinkDevice(unlinkInput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIUnlinkDevice failed: %d", rv);
        return -1;
    } else {
        LOG_INFO("CCDIUnlinkDevice OK");
        return 0;
    }
}

int registerPsn(u64 userId)
{
    LOG_INFO("start");
    int rv = 0;
    ccd::RegisterStorageNodeInput regInput;
    regInput.set_user_id(userId);
    LOG_INFO(">>>> Register storageNode");
    rv = CCDIRegisterStorageNode(regInput);
    if (rv == VPL_ERR_ALREADY)
        rv = 0;
    return rv;
}

int unregisterPsn(u64 userId)
{
    LOG_INFO("start");
    int rv = 0;
    ccd::UnregisterStorageNodeInput unregInput;
    unregInput.set_user_id(userId);
    LOG_INFO(">>>> Unregister storageNode");
    rv = CCDIUnregisterStorageNode(unregInput);
    if (rv == VPL_ERR_ALREADY)
        rv = 0;
    return rv;
}

int getUserIdBasic(u64 *userId)
{
    LOG_INFO("start");
    int rv = 0;
    ccd::GetSystemStateInput ccdiRequest;
    ccdiRequest.set_get_players(true);
    ccd::GetSystemStateOutput ccdiResponse;
    rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
        goto exit;
    }
    *userId = ccdiResponse.players().players(0).user_id();
    if (*userId == 0) {
        LOG_ERROR("Not signed-in!");
        rv = -1;
        goto exit;
    }

exit:
    return rv;
}

int getDeviceId(u64 *deviceId)
{
    LOG_INFO("start");
    int rv = 0;
    ccd::GetSystemStateInput ccdiRequest;
    ccdiRequest.set_get_device_id(true);
    ccd::GetSystemStateOutput ccdiResponse;
    rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
        goto exit;
    }
    *deviceId = ccdiResponse.device_id();
 exit:
    return rv;
}

bool isCloudpc(u64 userId, u64 deviceId)
{
    LOG_INFO("start");
    bool isPsn = false;
    int rv = 0;

    ccd::ListLinkedDevicesOutput listSnOut;
    ccd::ListLinkedDevicesInput request;
    request.set_user_id(userId);
    request.set_storage_nodes_only(true);

    // Try contacting infra for the most updated information first
    rv = CCDIListLinkedDevices(request, listSnOut);
    if (rv != 0) {
        LOG_ERROR("CCDIListLinkedDevice for user("FMTu64") failed %d", userId, rv);

        // Fall back to cache if cannot reach server
        LOG_ALWAYS("Retry with only_use_cache option");
        request.set_only_use_cache(true);
        rv = CCDIListLinkedDevices(request, listSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListLinkedDevice for user("FMTu64") failed %d", userId, rv);
        }
    } 
   
    if (rv == 0) {
        for (int i = 0; i < listSnOut.devices_size(); i++) {
            LOG_ALWAYS("user("FMTu64") device("FMTu64") is cloudpc", userId, deviceId);
            if (listSnOut.devices(i).device_id() == deviceId) {
                isPsn = true;
                break;
            }
        }
    }

    return isPsn;
}

int ccd_conf_set_domain(const std::string &conf_tmpl, const std::string &conf_target, const std::string &target_domain)
{
    std::string domain_tag = "${DOMAIN}";
    VPLFile_handle_t fp_tmpl = VPLFILE_INVALID_HANDLE;  
    ssize_t tmplSz;
    ssize_t rdSz;
    char *fileBuffer = NULL;
    int rv = 0;
    std::string conf_tmpl_clean = Util_CleanupPath(conf_tmpl); 
    std::string conf_target_clean = Util_CleanupPath(conf_target); 
    std::string output;
    std::ostringstream cfgSetInstance;
    size_t pos = 0;

    fp_tmpl = VPLFile_Open(conf_tmpl_clean.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fp_tmpl)) { 
        LOG_ERROR("Fail to open ccd config template file %s", conf_tmpl_clean.c_str());
        rv = -1;
        goto exit;
    }

    tmplSz = (ssize_t) VPLFile_Seek(fp_tmpl, 0, VPLFILE_SEEK_END);
    if (tmplSz <= 0) {
        LOG_ERROR("Template file size invalid %d", (int)tmplSz);
        rv = -1;
        goto exit;
    }

    VPLFile_Seek(fp_tmpl, 0, VPLFILE_SEEK_SET); // Reset

    fileBuffer = (char *) malloc(tmplSz + 1);
    if (fileBuffer == NULL) { 
        LOG_ERROR("Fail to allocate input file buffer");
        rv = -1;
        goto exit;
    }

    if ((rdSz = VPLFile_Read(fp_tmpl, fileBuffer, tmplSz)) != tmplSz) {
        LOG_ERROR("Error reading template file: file sz(%d), read sz(%d)", (int)tmplSz, (int)rdSz);
        rv = -1;
        goto exit;
    }

    fileBuffer[tmplSz] = 0;

    output.assign(fileBuffer);
    while ((pos = output.find(domain_tag.c_str(), pos)) != std::string::npos) {
        output.replace(pos, domain_tag.length(), target_domain.c_str());
    }

    // Write ccd testInstanceNum
    if (testInstanceNum != 0) {
        cfgSetInstance << "\ntestInstanceNum =";
        cfgSetInstance << testInstanceNum;
        output += cfgSetInstance.str();
    }

    rv = Util_WriteFile(conf_target_clean.c_str(), output.data(), output.length());
    if (rv < 0) {
        LOG_ERROR("Failed to write to config file %s rv %d", conf_target_clean.c_str(), rv);
        rv = -1;
        goto exit;
    }

    LOG_INFO("== New config ==");
    LOG_INFO("%s", output.c_str());

exit:
    if (VPLFile_IsValidHandle(fp_tmpl)) {
        VPLFile_Close(fp_tmpl);
    }
    if (fileBuffer) {
        free(fileBuffer);
    }
    return rv;

}
    
int getCurDir(std::string& dir)
{
    int rv = 0;
#define CURDIR_MAX_LENGTH       1024
#ifdef _MSC_VER
    wchar_t curDir[CURDIR_MAX_LENGTH];
    GetCurrentDirectory(CURDIR_MAX_LENGTH, curDir);
    char *utf8 = NULL;
    rv = _VPL__wstring_to_utf8_alloc(curDir, &utf8);
    if (rv != VPL_OK) {
        LOG_ERROR("Fail to generate current directory");
        goto exit;
    }
    dir.assign(utf8);
    if (utf8 != NULL) {
        free(utf8);
    }
#else
    char buffer[CURDIR_MAX_LENGTH];
    char *curDir;

    curDir = getcwd(buffer, sizeof(buffer));
    if (curDir == 0) {
        LOG_ERROR("Fail to generate current directory");
        rv = -1;
        goto exit;
    }
    dir.assign(curDir);
#endif
exit:
    return rv;
}

// Wait until cloud pc is back online (e.g. after sleep)
int waitCloudPCOnline(ccd::ListLinkedDevicesOutput& mediaServerDevices_out)
{
    return mca_get_media_servers(mediaServerDevices_out, 30, true);
}

int wakeSleepingDevices(u64 userId)
{
    int rv = 0;

    std::vector<u64> sleepingDevices;
    rv = listLinkedDevice(userId, &sleepingDevices);
    if (rv != 0) {
        LOG_ERROR("List devices fail");
        goto exit;
    }

    for (size_t i = 0; i < sleepingDevices.size(); i++) {
        LOG_INFO("Waking device "FMTu64, sleepingDevices[i]);
        ccd::RemoteWakeupInput request2;
        request2.set_user_id(userId);
        request2.set_device_to_wake(sleepingDevices[i]);
        int temp_rv = CCDIRemoteWakeup(request2);
        if (temp_rv != 0) {
            LOG_WARN("CCDIRemoteWakeup returned %d", temp_rv);
        } else {
            // CCDIRemoteWakeup returns success after queuing the wakeup request.
            // If the caller immediately calls CCDILogout or shuts down CCD, the request
            // might not ever be sent.
            // To be safe, do a short sleep now.
            VPLThread_Sleep(VPLTime_FromSec(1));
        }
    }
exit:
    return rv;
}

int remoteSwUpdate(u64& userId, u64 target)
{
    ccd::RemoteSwUpdateMessageInput req;

    req.set_user_id(userId);
    req.set_target_device_id(target);
    int rv = CCDIRemoteSwUpdateMessage(req);
    if (rv != 0) {
        LOG_WARN("CCDIRemoteSwUpdate returned %d", rv);
    } 

    return rv;
}

int dumpDatasetList(u64 userId)
{
    int i;
    s32 numDatasets = 0;
    int rv;

    LOG_ALWAYS(">>>> CCDIListOwnedDatasets");
    ccd::ListOwnedDatasetsInput ownedDatasetsInput;
    ccd::ListOwnedDatasetsOutput ownedDatasetsOutput;
    ownedDatasetsInput.set_user_id(userId);
    ownedDatasetsInput.set_only_use_cache(false);
    rv = CCDIListOwnedDatasets(ownedDatasetsInput, ownedDatasetsOutput);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIListOwnedDatasets fail: %d", rv);
        goto exit;
    } else {
        LOG_INFO("CCDIListOwnedDatasets OK");
    }

    numDatasets = ownedDatasetsOutput.dataset_details_size();

    for (i = 0; i < numDatasets; i++) {
        LOG_ALWAYS("["FMTu64"] Dataset name: %s [Content type(%s)]", 
                   ownedDatasetsOutput.dataset_details(i).datasetid(),
                   ownedDatasetsOutput.dataset_details(i).datasetname().c_str(), 
                   ownedDatasetsOutput.dataset_details(i).contenttype().c_str());

        if (ownedDatasetsOutput.dataset_details(i).has_datasettype()) {
            LOG_ALWAYS("    DatasetType(%d)", (int)ownedDatasetsOutput.dataset_details(i).datasettype());
        }

        if (ownedDatasetsOutput.dataset_details(i).has_storageclustername()) {
            LOG_ALWAYS("    StorageClusterName(%s)", ownedDatasetsOutput.dataset_details(i).storageclustername().c_str()); 
        }

        if (ownedDatasetsOutput.dataset_details(i).has_storageclusterhostname()) {
            LOG_ALWAYS("    StorageClusterHostName(%s)", ownedDatasetsOutput.dataset_details(i).storageclusterhostname().c_str()); 
        }

        if (ownedDatasetsOutput.dataset_details(i).has_linkedto()) {
            LOG_ALWAYS("    LinkedTo("FMTu64")", ownedDatasetsOutput.dataset_details(i).linkedto()); 
        }

        if (ownedDatasetsOutput.dataset_details(i).has_clusterid()) {
            LOG_ALWAYS("    ClusterId("FMTu64")", ownedDatasetsOutput.dataset_details(i).clusterid()); 
        }

        if (ownedDatasetsOutput.dataset_details(i).has_userid()) {
            LOG_ALWAYS("    UserId("FMTu64")", ownedDatasetsOutput.dataset_details(i).userid()); 
        }

        if (ownedDatasetsOutput.dataset_details(i).has_datasetlocation()) {
            LOG_ALWAYS("    DatasetLocation(%s)", ownedDatasetsOutput.dataset_details(i).datasetlocation().c_str()); 
        }
        if (ownedDatasetsOutput.dataset_details(i).archivestoragedeviceid_size() > 0) {
            for (int j = 0; j < ownedDatasetsOutput.dataset_details(i).archivestoragedeviceid_size(); j++) {
                LOG_ALWAYS("    ArchiveStorageDeviceId("FMTu64")", ownedDatasetsOutput.dataset_details(i).archivestoragedeviceid(j)); 
            }
        }
        if (ownedDatasetsOutput.dataset_details(i).has_displayname()) {
            LOG_ALWAYS("    displayName(%s)", ownedDatasetsOutput.dataset_details(i).displayname().c_str()); 
        }
    }
exit:
    return rv;
}
