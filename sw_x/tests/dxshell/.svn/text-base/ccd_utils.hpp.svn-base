//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef CCD_UTILS_HPP_
#define CCD_UTILS_HPP_

#include <vplex_plat.h>
#include <string>
#include <vector>

#include <ccdi.hpp>

// Note that 5 seconds is too short when running under valgrind.
#define CCD_TIMEOUT     25

// Known error codes (TODO: Should be exposed by SDK)
#define DATASET_ALREADY_SUBSCRIBED      -32228

//// Low Level CCD control functions ////
int checkRunningCcd();
int shutdownCcd();
int startCcd(const char* titleId);
int startCcdInClientSubdir();
/// Get the userId and report if the local device is already linked.
int getUserId(u64& userId);
int login(const std::string& username,
          const std::string& password,
          u64& userId);
int login_with_eula(const std::string& username,
                    const std::string& password,
                    const bool eula,
                    u64& userId);
int linkDevice(u64 userId);
int listLinkedDevice(u64 userId,
                     std::vector<u64>* sleepingDevices_out = NULL);
int listUserStorage(u64 userId, bool cache_only);
int updateStorageNode(u64 userId, u64 deviceId,
                      bool media_server_is_set,
                      bool media_server_enabled,
                      bool virt_drive_is_set,
                      bool virt_drive_enabled,
                      bool remote_file_access_is_set,
                      bool remote_file_access_enabled);
int subscribe_clearfi(const std::string &appDataPath,
                      u64 userId,
                      u64 &datasetId);
int subscribeDatasetCrUp(u64 userId, u64 datasetId,
                         bool useType,
                         ccd::SyncSubscriptionType_t subType,
                         const std::string& appDataPath,
                         const std::string& dsetName,
                         bool useAppDataPathOnly);
int subscribeDatasetCrDown(u64 userId, u64 datasetId,
                           bool useType,
                           ccd::SyncSubscriptionType_t subType,
                           const std::string& appDataPath,
                           const std::string& dsetName,
                           u64 maxSizeBytes, bool useMaxBytes,
                           u64 maxFiles, bool useMaxFiles);
int unsubscribe_clearfi(u64 userId);
int logout(u64 userId);
int unlinkDevice(u64 userId);
int unlinkDevice(u64 userId, u64 deviceId);
int registerPsn(u64 userId);
int unregisterPsn(u64 userId);
int unsubscribeDataset(u64 userId,
                       u64 datasetId,
                       const std::string& debugDsetName);
int getUserIdBasic(u64 *userId);
int getDeviceId(u64 *deviceId);
int getDatasetId(u64 userId, const std::string& name, u64& datasetId_out);
int getDatasetRoot(u64 userId,
                   const std::string& name,
                   std::string& datasetRoot);
int getLocalHttpInfo(u64 userId,
                     std::string &base_url,
                     std::string &service_ticket,
                     std::string &session_handle);
int getDeviceConnectionStatus(u64 userId,
                              u64 deviceId,
                              ccd::DeviceConnectionStatus& status_out);
bool isCloudpc(u64 userId, u64 deviceId);

//// Misc util functions
int getDxRootPath(std::string &rootPath);
int getCcdAppDataPath(std::string &ccdAppDataPath);
int ccd_conf_set_domain(const std::string &conf_tmpl,
                        const std::string &conf_target,
                        const std::string &target_domain);
int getCurDir(std::string& dir);

int waitCloudPCOnline(ccd::ListLinkedDevicesOutput& mediaServerDevices_out);

static inline int waitCloudPCOnline()
{
    ccd::ListLinkedDevicesOutput dummy_server_out;
    return waitCloudPCOnline(dummy_server_out);
}

int wakeSleepingDevices(u64 userId);
int remoteSwUpdate(u64& userId, u64 target);
int dumpDatasetList(u64 userId);

#endif /* CCD_UTILS_HPP_ */
