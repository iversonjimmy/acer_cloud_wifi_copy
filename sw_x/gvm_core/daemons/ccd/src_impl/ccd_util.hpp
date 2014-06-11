//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include "base.h"
#include <ccd_types.pb.h>

// TODO: Please document; is secret supposed to be raw bytes or base64-encoded?  Since it also takes 
//       a secretSize param, I'd guess the former, but at the call site, it is used as the latter.
s32 Util_GetServiceTicket(UCFBuffer* ticket, const char* serviceId, const void* secret, u32 secretSize);

int Util_RemoveAllFilesInDir(const char* dirName);

std::string Util_MakeMessageId();

bool Util_IsUserStorageAddressEqual(const vplex::vsDirectory::UserStorageAddress& lhs,
                                    const vplex::vsDirectory::UserStorageAddress& rhs);

bool Util_IsUserStorageEqual(const vplex::vsDirectory::UserStorage* lhs,
                             const vplex::vsDirectory::UserStorage* rhs);

bool Util_IsUserStorageAccessEqual(const google::protobuf::RepeatedPtrField<vplex::vsDirectory::StorageAccess>& lhs,
                                   const google::protobuf::RepeatedPtrField<vplex::vsDirectory::StorageAccess>& rhs);

const vplex::vsDirectory::DeviceInfo*
Util_FindInDeviceInfoList(u64 deviceId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>& devices);

static inline bool
Util_IsInDeviceInfoList(u64 deviceId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>& devices)
{
    return (Util_FindInDeviceInfoList(deviceId, devices) != NULL);
}

const vplex::vsDirectory::UserStorage*
Util_FindInUserStorageList(u64 deviceId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>& storage);

static inline bool
Util_IsInUserStorageList(u64 deviceId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>& storage)
{
    return (Util_FindInUserStorageList(deviceId, storage) != NULL);
}

int Util_InitVsCore(u64 deviceId);

int Util_CloseVsCore();

/// Generates a 20-byte hash and encodes it as a 40 character hex string.
int Util_CalcHash(const void* buf, size_t bufLen, std::string& hash_out);
/// Generates a 20-byte hash and encodes it as a 40 character hex string.
int Util_CalcHash(const char* cString, std::string& hash_out);

/// Filters out "sensitive data" from msg_ and then logs it (at INFO level).
/// prefix_ (which is *not* filtered) will be prepended to the filtered message.
/// suffix_ (which is *not* filtered) will be appended to the filtered message.
/// See the implementation for the hard-coded list of strings that indicate "sensitive data".
#define UTIL_LOG_SENSITIVE_STRING_INFO(prefix_, msg_, suffix_) \
        Util_LogSensitiveString((prefix_), (msg_), (suffix_), __FILE__, __LINE__, __func__)
void Util_LogSensitiveString(const char* prefix, const char* msg, const char* suffix, const char* file, int line, const char* function);

// previously in stream_service.cpp as range_parser()
int Util_ParseRange(const std::string* reqRange, u64 filesize, u64 &start, u64 &end);

#define MM_VCS_DATASET_NAME                     "Media MetaData VCS"
#define MM_VCS_DATASET_TYPE                     vplex::vsDirectory::USER_CONTENT_METADATA
#define NOTES_DATASET_NAME                      "Notes"
#define NOTES_DATASET_TYPE                      vplex::vsDirectory::USER_CONTENT_METADATA
#define SBM_DATASET_NAME                        "Shared by Me"
#define SBM_DATASET_TYPE                        vplex::vsDirectory::SBM
#define SWM_DATASET_NAME                        "Shared with Me"
#define SWM_DATASET_TYPE                        vplex::vsDirectory::SWM
#define SYNCBOX_DATASET_NAME_BASE               "Syncbox"
#define SYNCBOX_DATASET_TYPE                    vplex::vsDirectory::SYNCBOX

const vplex::vsDirectory::DatasetDetail* Util_FindDataset(const ccd::CachedUserDetails& userDetails, const char* datasetName, vplex::vsDirectory::DatasetType datasetType);

static inline const vplex::vsDirectory::DatasetDetail*
Util_FindMediaMetadataDataset(const ccd::CachedUserDetails& userDetails)
{
    return Util_FindDataset(userDetails, MM_VCS_DATASET_NAME, MM_VCS_DATASET_TYPE);
}

static inline const vplex::vsDirectory::DatasetDetail*
Util_FindNotesDataset(const ccd::CachedUserDetails& userDetails)
{
    return Util_FindDataset(userDetails, NOTES_DATASET_NAME, NOTES_DATASET_TYPE);
}

static inline const vplex::vsDirectory::DatasetDetail*
Util_FindSbmDataset(const ccd::CachedUserDetails& userDetails)
{
    return Util_FindDataset(userDetails, SBM_DATASET_NAME, SBM_DATASET_TYPE);
}

static inline const vplex::vsDirectory::DatasetDetail*
Util_FindSwmDataset(const ccd::CachedUserDetails& userDetails)
{
    return Util_FindDataset(userDetails, SWM_DATASET_NAME, SWM_DATASET_TYPE);
}

/// Find a Syncbox dataset that is currently associated with archive storage.
/// Assumes only one such Syncbox dataset is possible for now.
const vplex::vsDirectory::DatasetDetail*
Util_FindSyncboxArchiveStorageDataset(const ccd::CachedUserDetails& userDetails);

/// Find the "Device Storage" dataset for the specified device.  It will only exist if the
/// specified device is a StorageNode and our local cache is up-to-date.
/// (The device storage dataset id is used for remote file access.)
int Util_GetDeviceStorageDatasetId(const ccd::CachedUserDetails& userDetails, u64 deviceId, u64& datasetId_out);

/// Returns true if the local device is an Archive Storage Device for the specified dataset.
bool Util_IsLocalDeviceArchiveStorage(const vplex::vsDirectory::DatasetDetail& datasetDetail);

#endif // include guard
