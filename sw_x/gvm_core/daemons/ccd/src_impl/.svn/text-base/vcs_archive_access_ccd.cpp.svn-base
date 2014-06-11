//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#include "vcs_archive_access_ccd.hpp"

#include "vplex_file.h"
#include "vplex_http_util.hpp"
#include "vplex_serialization.h"
#include "vplu_mutex_autolock.hpp"
#include "vplu_sstr.hpp"

#include "cache.h"
#include "ccd_util.hpp"
#include "DeviceStateCache.hpp"
#include "gvm_errors.h"
#include "log.h"
#include "HttpFileDownloadStream.hpp"
#include "HttpSvc_Ccd_Handler_Helper.hpp"
#include "HttpSvc_Utils.hpp"
#include "InStringStream.hpp"
#include "OutStringStream.hpp"
#include "scopeguard.hpp"
#include "vcs_util.hpp"

class VCSArchiveOperationCcdImpl : public VCSArchiveOperation
{
public:
    VCSArchiveOperationCcdImpl(VPLUser_Id_t userId, u64 archiveDeviceId, u64 datasetId) :
        userId(userId), archiveDeviceId(archiveDeviceId), datasetId(datasetId),
        cancel(false), fileDownStream(NULL), startedTransfer(false)
    {
        VPL_SET_UNINITIALIZED(&mutex);
    }

    int init()
    {
        int rv;

        rv = VPLMutex_Init(&mutex);
        if (rv < 0) {
            LOG_WARN("%p: VPLMutex_Init failed: %d", this, rv);
            return rv;
        }
        return rv;
    }

    virtual ~VCSArchiveOperationCcdImpl()
    {
        if (VPL_IS_INITIALIZED(&mutex)) {
            int rc = VPLMutex_Destroy(&mutex);
            if (rc != 0) {
                LOG_ERROR("VPLMutex_Destroy:%d", rc);
            }
        }
    }

protected:
    VPLMutex_t mutex;

    VPLUser_Id_t userId;
    u64 archiveDeviceId;
    u64 datasetId;

    bool cancel;
    HttpFileDownloadStream* fileDownStream;
    /// As per the API, this enforces that each #VCSArchiveOperation can only be used once.
    bool startedTransfer;

    void releaseFileDownStream()
    {
        MutexAutoLock lock(&mutex);
        delete fileDownStream;
        fileDownStream = NULL;
    }

    virtual int GetFile(
            u64 component_id,
            u64 revision,
            const std::string& dataset_rel_path,
            const std::string& dest_file_abs_path)
    {
        int rv;

        MutexAutoLock lock(&mutex);
        if (cancel) {
            return VPL_ERR_CANCELED;
        }
        if (startedTransfer) {
            return VPL_ERR_ALREADY;
        }
        startedTransfer = true;

        VPLFile_handle_t destFile = VPLFile_Open(dest_file_abs_path.c_str(),
                VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE,
                0666);
        if (destFile < 0) {
            return destFile;
        }
        ON_BLOCK_EXIT(VPLFile_Close, destFile);

        std::string requestHeaderStr = SSTR("GET /vcs_archive/" <<
                archiveDeviceId << "/" << datasetId <<
                "/" << component_id << "/" << revision <<
                " HTTP/1.1\r\n\r\n");

        InStringStream issReqHdr(requestHeaderStr);
        InStringStream issReqBody;
        OutStringStream ossResp;
        fileDownStream = new HttpFileDownloadStream(&issReqHdr, &issReqBody, destFile, &ossResp);
        ON_BLOCK_EXIT_OBJ(*this, &VCSArchiveOperationCcdImpl::releaseFileDownStream);

        fileDownStream->SetReqHeader(HttpSvc::Utils::HttpHeader_ac_datasetRelPath, VPLHttp_UrlEncoding(dataset_rel_path, ""));
        fileDownStream->SetReqHeader(HttpSvc::Utils::HttpHeader_ac_tolerateFileModification, "1");

        // Add extra fields for tunnel layer.
        fileDownStream->SetUserId(userId);
        fileDownStream->SetDeviceId(archiveDeviceId);

        // Perform the blocking call without holding the mutex.
        lock.UnlockNow();
        rv = HttpSvc::Ccd::Handler_Helper::ForwardToServerCcd(fileDownStream);
        if (rv != 0) {
            LOG_WARN("ForwardToServerCcd failed: %d, treating as CCD_ERROR_TRANSIENT (-14179)", rv);
            return CCD_ERROR_TRANSIENT;
        }
        int statusCode = fileDownStream->GetStatusCode();
        rv = vcs_translate_http_status_code(statusCode);
        if (rv != 0) {
            LOG_WARN("HTTP status was %d", statusCode);
        }
        return rv;
    }

    virtual int Cancel()
    {
        MutexAutoLock lock(&mutex);
        cancel = true;
        if (fileDownStream != NULL) {
            fileDownStream->StopIo();
        }
        return 0;
    }
private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSArchiveOperationCcdImpl);
};

static int getMediaServerArchiveStoragePriv(
        VPLUser_Id_t& userId_out,
        u64& mediaServerDeviceId_out,
        u64& datasetId_out)
{
    int rv;
    bool isOnline = false;
    bool isArchiveStorageCapable = false;

    rv = GetMediaServerArchiveStorage(/*OUT*/ userId_out,
                                      /*OUT*/ mediaServerDeviceId_out,
                                      /*OUT*/ datasetId_out,
                                      /*OUT*/ isArchiveStorageCapable,
                                      /*OUT*/ isOnline);
    if (rv != 0) {
        LOG_INFO("GetMediaServerArchiveStorage:%d", rv);
        return rv;
    }

    if(!isArchiveStorageCapable) {
        LOG_INFO("Media server "FMTu64" does not support virtual sync", mediaServerDeviceId_out);
        return CCD_ERROR_ARCHIVE_DEVICE_NOT_FOUND;
    }

    if(!isOnline) {
        LOG_INFO("Media server "FMTu64" is not ONLINE", mediaServerDeviceId_out);
        // Note that SyncConfig specifically handles this error code.
        return CCD_ERROR_ARCHIVE_DEVICE_OFFLINE;
    }

    LOG_INFO("Will use archive storage device "FMTu64, mediaServerDeviceId_out);
    return 0;
}

int GetMediaServerArchiveStorage(
        VPLUser_Id_t& userId_out,
        u64& mediaServerDeviceId_out,
        u64& mediaMetadataDatasetId_out,
        bool& mediaServerIsArchiveStorageCapable_out,
        bool& mediaServerIsOnline_out)
{
    int rv;

    userId_out = 0;
    mediaServerDeviceId_out = 0;
    mediaMetadataDatasetId_out = 0;
    mediaServerIsArchiveStorageCapable_out = false;
    mediaServerIsOnline_out = false;

    CacheAutoLock autoLock;
    rv = autoLock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        return rv;
    }
    CachePlayer* user = cache_getUserByPlayerIndex(0);
    if (user == NULL) {
        LOG_INFO("No user logged in");
        return CCD_ERROR_NOT_SIGNED_IN;
    }

    const vplex::vsDirectory::DatasetDetail* mediaMetadataDataset =
            Util_FindMediaMetadataDataset(user->_cachedData.details());
    if (mediaMetadataDataset == NULL) {
        LOG_INFO("Media metadata dataset not found");
        return CCD_ERROR_DATASET_NOT_FOUND;
    }

    // Find out which device is the media server.
    const google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>& userStorage =
            user->_cachedData.details().cached_user_storage();
    for (int i = 0; i < userStorage.size(); i++) {
        const vplex::vsDirectory::UserStorage& currStorage = userStorage.Get(i);
        if (currStorage.featuremediaserverenabled()) {
            // Found the media server.  Next, get its DeviceInfo.
            const google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>& devices =
                    user->_cachedData.details().cached_devices();
            for (int j = 0; j < devices.size(); j++) {
                const vplex::vsDirectory::DeviceInfo& currDevice = devices.Get(j);
                if (currDevice.deviceid() == currStorage.storageclusterid()) {
                    // Found the media server's DeviceInfo.
                    // Check that it is able to serve the dataset content.
                    userId_out = user->user_id();
                    mediaServerDeviceId_out = currDevice.deviceid();
                    mediaMetadataDatasetId_out = mediaMetadataDataset->datasetid();

                    u32 protocolVersion = VPL_strToU32(currDevice.protocolversion().c_str(), NULL, 10);
                    if (errno != 0) {
                        LOG_WARN("Failed to parse protocol version \"%s\": %s",
                                currDevice.protocolversion().c_str(), strerror(errno));
                    }
                    // CCD_PROTOCOL_VERSION 5 is the minimum version that supports "Archive Device File Serving".
                    if (protocolVersion >= 5) {
                        // Check that it is online.
                        mediaServerIsArchiveStorageCapable_out = true;
                        if (DeviceStateCache_GetDeviceStatus(currDevice.deviceid()).state() ==
                                ccd::DEVICE_CONNECTION_ONLINE)
                        {
                            mediaServerIsOnline_out = true;
                        }
                    }
                    return 0;
                }
            }
        }
    }
    return CCD_ERROR_ARCHIVE_DEVICE_NOT_FOUND;
}

VCSArchiveOperation* VCSArchiveAccessCcdMediaMetadataImpl::createOperation(int& err_code__out)
{
    VPLUser_Id_t userId;
    u64 mediaServerDeviceId;
    u64 datasetId;
    err_code__out = getMediaServerArchiveStoragePriv(/*out*/userId, /*out*/mediaServerDeviceId, /*out*/datasetId);
    if (err_code__out != 0) {
        return NULL;
    }
    VCSArchiveOperationCcdImpl* result =
            new VCSArchiveOperationCcdImpl(userId, mediaServerDeviceId, datasetId);
    err_code__out = result->init();
    if (err_code__out != 0) {
        delete result;
        result = NULL;
    }
    return result;
}

void VCSArchiveAccessCcdMediaMetadataImpl::destroyOperation(VCSArchiveOperation* vcs_archive_access)
{
    delete vcs_archive_access;
}
