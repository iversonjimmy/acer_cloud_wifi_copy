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

#include "file_url_operation_ts.hpp"

#include "vplex_assert.h"
#include "vplex_file.h"
#include "vplex_http_util.hpp"
#include "vplex_serialization.h"
#include "vplu_mutex_autolock.hpp"
#include "vplu_sstr.hpp"

#include "cache.h"
#include "DeviceStateCache.hpp"
#include "gvm_errors.h"
#include "HttpFileDownloadStream.hpp"
#include "HttpFileUploadStream.hpp"
#include "HttpSvc_Utils.hpp"
#include "HttpSvc_Ccd_Handler_Helper.hpp"
#include "InStringStream.hpp"
#include "log.h"
#include "md5.h"
#include "OutStringStream.hpp"
#include "vcs_util.hpp"
#include "virtual_device.hpp"


class VCSFileUrlOperation_TsImpl : public VCSFileUrlOperation
{
public:
    virtual ~VCSFileUrlOperation_TsImpl();

    virtual int GetFile(const std::string& dest_file_abs_path);

    virtual int PutFile(const std::string& src_file_abs_path);
    virtual int PutFile(const std::string& src_file_abs_path, std::string& hash__out);

    virtual int DeleteFile();

    virtual int Cancel();

    // For debug purpose and possibly for later status use
    void fileReadCb(size_t bytes);

    int computeHash(const char* buf, size_t len);
protected:
    VCSFileUrlOperation_TsImpl(const VcsAccessInfo& vcsFileUrl,
                               bool verbose_log);
    int init(VPLUser_Id_t, u64, u64);
private:
    int GetStorageUrl(std::string& urlForStorageNode_out);
    int PutFileL(const std::string& src_file_abs_path, std::string& hash__out, bool compute_hash);
    VPL_DISABLE_COPY_AND_ASSIGN(VCSFileUrlOperation_TsImpl);
    VcsAccessInfo accessInfo;
    bool verbose_log;
    VPLMutex_t mutex;
    VPLUser_Id_t userId;
    u64 archiveDeviceId;
    u64 datasetId;
    bool cancel;
    HttpFileDownloadStream* fileDownStream;
    HttpFileUploadStream* fileUpStream;
    bool startedTransfer;
    int sentSoFar;
    MD5Context mHashContext;
    unsigned char mHashValue[MD5_DIGESTSIZE];

    friend VCSFileUrlOperation* CreateVCSFileUrlOperation_TsImpl(
            const VcsAccessInfo& accessInfo,
            bool verbose_log,
            int& errCode_out);

    void releaseStream()
    {
        MutexAutoLock lock(&mutex);
        if (fileDownStream) {
            delete fileDownStream;
            fileDownStream = NULL;
        }
        if (fileUpStream) {
            delete fileUpStream;
            fileUpStream = NULL;
        }
    }
};

static int getSyncboxArchiveStoragePriv(VPLUser_Id_t& userId_out,
                                 u64& serverDeviceId_out,
                                 u64& datasetId_out)
{
    int rv;
    bool isOnline = false;

    // This function returns an active syncbox archive storage. Only one Syncbox
    // is returned for now.  There maybe multiple Syncbox datasets in the future.
    rv = GetSyncboxArchiveStorage(/*OUT*/ userId_out,
                                  /*OUT*/ serverDeviceId_out,
                                  /*OUT*/ datasetId_out,
                                  /*OUT*/ isOnline);
    if (rv != 0) {
        LOG_WARN("GetSyncboxArchiveStorage:%d", rv);
        return rv;
    }

    if (!isOnline) {
        LOG_INFO("Archive storage device "FMTu64" is not ONLINE", serverDeviceId_out);
        // Note that SyncConfig specifically handles this error code.
        return CCD_ERROR_ARCHIVE_DEVICE_OFFLINE;
    }

    LOG_INFO("Will use archive storage device "FMTu64, serverDeviceId_out);
    return 0;
}

int GetSyncboxArchiveStorage(VPLUser_Id_t& userId_out,
                             u64& serverDeviceId_out,
                             u64& datasetId_out,
                             bool& serverIsOnline_out)
{
    int rv;

    userId_out = 0;
    serverDeviceId_out = 0;
    datasetId_out = 0;
    serverIsOnline_out = false;

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
  
    userId_out = user->user_id();

    const ccd::CachedUserDetails& userDetails = user->_cachedData.details();
    const vplex::vsDirectory::DatasetDetail* syncboxDataset = Util_FindSyncboxArchiveStorageDataset(userDetails);
    if (syncboxDataset == 0) {
        return CCD_ERROR_ARCHIVE_DEVICE_NOT_FOUND;
    } else {
        datasetId_out = syncboxDataset->datasetid();
        if (syncboxDataset->archivestoragedeviceid_size() > 0) {
            serverDeviceId_out = syncboxDataset->archivestoragedeviceid(0);
            if (DeviceStateCache_GetDeviceStatus(serverDeviceId_out).state() == ccd::DEVICE_CONNECTION_ONLINE)
                serverIsOnline_out = true;
        }
    }

    return CCD_OK;
}

static void FileReadCallback(void *ctx, size_t bytes, const char* buf)
{
    if (!ctx) {
        LOG_ERROR("Null context");
        return;
    }

    VCSFileUrlOperation_TsImpl *op = (VCSFileUrlOperation_TsImpl *)ctx;
    op->fileReadCb(bytes);
}

static void FileReadCallbackComputeHash(void *ctx, size_t bytes, const char* buf)
{
    if (!ctx) {
        LOG_ERROR("Null context");
        return;
    }
    VCSFileUrlOperation_TsImpl *op = (VCSFileUrlOperation_TsImpl *)ctx;
    int rv = op->computeHash(buf, bytes);
    if (rv != 0) {
        LOG_ERROR("compute hash error");
    }
    FileReadCallback(ctx, bytes, buf);
}

VCSFileUrlOperation_TsImpl::VCSFileUrlOperation_TsImpl(
        const VcsAccessInfo& accessInfo,
        bool verbose_log) 
:   accessInfo(accessInfo),
    verbose_log(verbose_log), 
    cancel(false), 
    fileDownStream(NULL), 
    fileUpStream(NULL), 
    startedTransfer(false),
    sentSoFar(0)
{
    VPL_SET_UNINITIALIZED(&mutex);
}

int VCSFileUrlOperation_TsImpl::init(VPLUser_Id_t usrId, u64 devId, u64 dsetId) 
{
    int rv;

    userId              = usrId;
    archiveDeviceId     = devId;
    datasetId           = dsetId;

    rv = VPLMutex_Init(&mutex);
    if (rv < 0) {
        LOG_WARN("%p: VPLMutex_Init failed: %d", this, rv);
        return rv;
    }

    return 0;
}

VCSFileUrlOperation* CreateVCSFileUrlOperation_TsImpl(
        const VcsAccessInfo& accessInfo,
        bool verbose_log,
        int& errCode_out)
{
    VPLUser_Id_t usrId;
    u64 devId;
    u64 dsetId;

    errCode_out = getSyncboxArchiveStoragePriv(usrId, devId, dsetId);
    if (errCode_out != 0) {
        return NULL;
    }

    VCSFileUrlOperation_TsImpl* newOperation =
            new (std::nothrow) VCSFileUrlOperation_TsImpl(accessInfo, verbose_log);
    if (newOperation == NULL) {
        LOG_ERROR("Out of mem");
        errCode_out = -1;
        return NULL;
    }

    errCode_out = newOperation->init(usrId, devId, dsetId);
    if (errCode_out != 0) {
        delete newOperation;
        newOperation = NULL;
    }

    return newOperation;
}

VCSFileUrlOperation_TsImpl::~VCSFileUrlOperation_TsImpl()
{
    if (VPL_IS_INITIALIZED(&mutex)) {
        int rc = VPLMutex_Destroy(&mutex);
        if (rc != 0) {
            LOG_ERROR("VPLMutex_Destroy:%d", rc);
        }
    }
}

// General form of this uri:
// acer-ts://<userId>/<archive storage deviceId>/<instanceId (always 0 for now)><urlForStorageNode>

// Current expected forms (for Syncbox):
// 1. acer-ts://<userId>/<archive storage deviceId>/<instanceId (always 0 for now)>/rf/file/<archive storage device's "Device Storage" datasetId>/[stagingArea:<Syncbox datasetId>]/<uniqueNumberGeneratedByVcs>
// 2. acer-ts://<userId>/<deviceId>/<instanceId (always 0 for now)>/vcs_archive/<deviceId>/<datasetId>/<componentId>/<revision>
int VCSFileUrlOperation_TsImpl::GetStorageUrl(std::string& urlForStorageNode_out)
{
    std::string uri_prefix = ARCHIVE_ACCESS_URI_SCHEME;
    uri_prefix += ":/";
    std::string uri = accessInfo.accessUrl.substr(uri_prefix.length());
    std::vector<std::string> uri_tokens;
    VPLHttp_SplitUri(uri, uri_tokens);

    urlForStorageNode_out.clear();
    for (int i = 3; i < (int)uri_tokens.size(); i++) {
        urlForStorageNode_out.append("/");
        urlForStorageNode_out.append(uri_tokens[i]);
    }
    return 0;
}

int VCSFileUrlOperation_TsImpl::GetFile(const std::string& dest_file_abs_path)
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

    std::string requestHeaderStr; 
    std::string storageUrl;
    GetStorageUrl(storageUrl);
    requestHeaderStr = SSTR("GET " << storageUrl << " HTTP/1.1\r\n\r\n"); 

    InStringStream issReqHdr(requestHeaderStr);
    InStringStream issReqBody;
    OutStringStream ossResp;
    fileDownStream = new HttpFileDownloadStream(&issReqHdr, &issReqBody, destFile, &ossResp);
    ON_BLOCK_EXIT_OBJ(*this, &VCSFileUrlOperation_TsImpl::releaseStream);

    // TODO: bug 16824: this should really be data-driven from accessInfo.header:
    fileDownStream->SetReqHeader(HttpSvc::Utils::HttpHeader_ac_tolerateFileModification, "1");
    //for (curr in accessInfo.header) {
    //    fileDownStream->SetReqHeader(curr.headerName, curr.headerValue);
    //}

    // Add extra fields for tunnel layer.
    fileDownStream->SetUserId(userId);
    fileDownStream->SetDeviceId(archiveDeviceId);

    // Set local device Id
    {
        std::ostringstream oss;
        oss << VirtualDevice_GetDeviceId(); 
        fileDownStream->SetReqHeader(HttpSvc::Utils::HttpHeader_ac_origDeviceId, oss.str());
    }

    // Note: Tunnel layer takes care of authentication, so no need to add session headers.

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
        LOG_WARN("HTTP status was %d for request %s", statusCode, requestHeaderStr.c_str());
        LOG_WARN("     accessUrl from VCS %s", accessInfo.accessUrl.c_str());
    }
    return rv;
}

int VCSFileUrlOperation_TsImpl::PutFileL(const std::string& src_file_abs_path, std::string& hash__out, bool compute_hash)
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

    VPLFile_handle_t srcFile = VPLFile_Open(src_file_abs_path.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(srcFile)) {
        LOG_ERROR("[%p]: Failed to open file %s", this, src_file_abs_path.c_str());
        return srcFile;
    }

    ON_BLOCK_EXIT(VPLFile_Close, srcFile);

    std::string requestHeaderStr; 
    std::string storageUrl;
    GetStorageUrl(storageUrl);
    requestHeaderStr = SSTR("PUT " << storageUrl << " HTTP/1.1\r\n\r\n"); 

    InStringStream issReqHdr(requestHeaderStr);
    OutStringStream ossResp;
    fileUpStream = new (std::nothrow) HttpFileUploadStream(&issReqHdr, srcFile, &ossResp);
    if (!fileUpStream) {
        LOG_ERROR("this[%p]: No memory to create HttpFileUploadStream obj", this);
        return CCD_ERROR_NOMEM;
    }

    ON_BLOCK_EXIT_OBJ(*this, &VCSFileUrlOperation_TsImpl::releaseStream);

    // TODO: bug 16824: technically, we should set headers based on accessInfo.header,
    //   but we don't expect any yet.

    // Add extra fields for tunnel layer.
    fileUpStream->SetUserId(userId);

    // Set Content-Length field
    {
        VPLFS_stat_t statBuf;
        int rc = VPLFS_Stat(src_file_abs_path.c_str(), &statBuf);
        if (rc != 0) {
            LOG_ERROR("VPLFS_Stat() fail on file %s, rc %d", src_file_abs_path.c_str(), rc);
            return CCD_ERROR_NOT_FOUND;
        }
        std::ostringstream oss;
        oss << statBuf.size;
        fileUpStream->SetReqHeader(HttpSvc::Utils::HttpHeader_ContentLength, oss.str());
    }

    // Set local device Id
    {
        std::ostringstream oss;
        oss << VirtualDevice_GetDeviceId(); 
        fileUpStream->SetReqHeader(HttpSvc::Utils::HttpHeader_ac_origDeviceId, oss.str());
    }

    // Note: Tunnel layer takes care of authentication, so no need to add session headers.

    if (compute_hash == true) {
        hash__out.clear();
        rv = MD5Reset(&mHashContext);
        if (rv < 0 ) {
            LOG_ERROR("CSL Reset MD5 fail from file:%s, result:%d", src_file_abs_path.c_str(), rv);
        }
        fileUpStream->SetBodyReadCb(FileReadCallbackComputeHash, this);
    } else {
        fileUpStream->SetBodyReadCb(FileReadCallback, this);
    }
    fileUpStream->SetDeviceId(archiveDeviceId);

    // Perform the blocking call without holding the mutex.
    lock.UnlockNow();
    rv = HttpSvc::Ccd::Handler_Helper::ForwardToServerCcd(fileUpStream);
    if (rv != 0) {
        LOG_WARN("ForwardToServerCcd failed: %d, treating as CCD_ERROR_TRANSIENT (-14179)", rv);
        return CCD_ERROR_TRANSIENT;
    }
    int statusCode = fileUpStream->GetStatusCode();
    rv = vcs_translate_http_status_code(statusCode);
    if (rv != 0) {
        LOG_WARN("HTTP status was %d for request %s", statusCode, requestHeaderStr.c_str());
        LOG_WARN("     accessUrl from VCS %s", accessInfo.accessUrl.c_str());
    } else {
        // don't compute hash if PutFile has some error.
        if (compute_hash == true) {
            rv = MD5Result(&mHashContext, mHashValue);
            if (rv != 0) {
                LOG_ERROR("CSL MD5 get result fail from file:%s, result:%d", src_file_abs_path.c_str(), rv);
            } else {
                for(int hashIndex = 0; hashIndex < MD5_DIGESTSIZE; hashIndex++) {
                    char byteStr[4];
                    snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, mHashValue[hashIndex]);
                    hash__out.append(byteStr);
                }
            }
        }
    }
    return rv;
}

int VCSFileUrlOperation_TsImpl::PutFile(const std::string& src_file_abs_path)
{
    std::string tmp;
    return PutFileL(src_file_abs_path, tmp, false);
}

int VCSFileUrlOperation_TsImpl::PutFile(const std::string& src_file_abs_path, std::string& hash__out)
{
    return PutFileL(src_file_abs_path, hash__out, true);
}

int VCSFileUrlOperation_TsImpl::DeleteFile()
{
    FAILED_ASSERT("Not implemented");
    return -0340;
}

int VCSFileUrlOperation_TsImpl::Cancel()
{
    MutexAutoLock lock(&mutex);
    cancel = true;
    if (fileDownStream != NULL) {
        fileDownStream->StopIo();
    }
    return 0;
}

void VCSFileUrlOperation_TsImpl::fileReadCb(size_t bytes)
{
    sentSoFar += bytes;
    LOG_DEBUG("[%p] uploaded %d bytes", this, sentSoFar);
}

int VCSFileUrlOperation_TsImpl::computeHash(const char* buf, size_t len)
{
    return MD5Input(&mHashContext, buf, len);
}
