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
#include "vcs_common.hpp"

#include "vcs_common_priv.hpp"
#include "vcs_defs.hpp"
#include "vplex_file.h"
#include "md5.h"
#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include "log.h"

int vcs_translate_http_status_code(int httpStatus)
{
    if ((httpStatus >= 500 && httpStatus < 600) ||
        httpStatus == 401)  // 401 is auth error - retry after reauthentication
    {
        return CCD_ERROR_TRANSIENT;
    }
    if (httpStatus >= 400 && httpStatus < 500)
    {
        return CCD_ERROR_HTTP_STATUS;
    }
    if (httpStatus < 200 || httpStatus >= 300)
    {
        return CCD_ERROR_BAD_SERVER_RESPONSE;
    }
    return 0;
}

static int setVcsAccessInfo(const VcsAccessInfo& accessInfo,
                            VPLHttp2& httpHandle)
{
    int rc;
    for(size_t headerIdx=0; headerIdx<accessInfo.header.size(); ++headerIdx)
    {
        rc = httpHandle.AddRequestHeader(accessInfo.header[headerIdx].headerName,
                                         accessInfo.header[headerIdx].headerValue);
        if(rc != 0) {
            LOG_WARN("Error adding header(%d) %s:%s.  Continuing to see if request still works.",
                     rc,
                     accessInfo.header[headerIdx].headerName.c_str(),
                     accessInfo.header[headerIdx].headerValue.c_str());
        }
    }

    rc = httpHandle.SetUri(accessInfo.accessUrl);
    if(rc != 0) {
        LOG_ERROR("SetUri:%d, %s", rc, accessInfo.accessUrl.c_str());
        return rc;
    }
    return 0;
}


struct VcsFileHashContext {
    VPLFile_handle_t fileHandle;
    MD5Context hashContext;
    u8 hashValue[MD5_DIGESTSIZE];
    void clear() {
        fileHandle = VPLFILE_INVALID_HANDLE;
        hashValue[0] = '\0';
    }
    VcsFileHashContext() { clear(); }

};

static s32 FileReadCallback(VPLHttp2 *http, void *ctx, char *buf, u32 size)
{
    u32 bytesToSend = size;
    ssize_t byteRead = 0;
    char *curPos = buf;
    VcsFileHashContext* vfileCtx = (VcsFileHashContext*)ctx;

    do {
        byteRead = VPLFile_Read(vfileCtx->fileHandle, curPos, bytesToSend);
        if (byteRead == 0) {
            // EOF
            break;
        } else if (byteRead < 0) {
            LOG_ERROR("Error %d reading from file after reading %d bytes.",
                            (int)byteRead, (curPos-(char *)buf));
            break;
        }
        int rv = MD5Input(&(vfileCtx->hashContext), curPos, byteRead);
        if (rv != 0) {
            LOG_ERROR("MD5Input:%d", rv);
        }
        curPos += byteRead;
        bytesToSend -= byteRead;
    } while (bytesToSend > 0);
    return curPos - (char *)buf;
}

static int vcs_s3_putFileHelperInternal(const VcsAccessInfo& accessInfo,
                         VPLHttp2& httpHandle,
                         VPLHttp2_ProgressCb progressCb,
                         void* progressCbCtx,
                         const std::string& srcLocalFilepath,
                         bool printLog,
                         std::string& hash__out,
                         bool computeHash)
{
    int rv;
    VcsFileHashContext vfileCtx;

    VPLFS_stat_t filestat;
    if ((rv=VPLFS_Stat(srcLocalFilepath.c_str(), &filestat)) != VPL_OK) {
        LOG_ERROR("Failed to stat %s:%d", srcLocalFilepath.c_str(), rv);
        return rv;
    }

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }

    rv = setVcsAccessInfo(accessInfo, httpHandle);
    if(rv != 0) {
        LOG_ERROR("setVcsAccessInfo:%d", rv);
        return CCD_ERROR_INTERNAL;
    }

    std::string httpPutResponse;
    if (computeHash == true) {

        vfileCtx.fileHandle = VPLFile_Open(srcLocalFilepath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(vfileCtx.fileHandle)) {
            LOG_ERROR("Fail to open source file:%d, %s",
                            (int)vfileCtx.fileHandle, srcLocalFilepath.c_str());
            return vfileCtx.fileHandle;
        }
        s64 fileSize =  VPLFile_Seek(vfileCtx.fileHandle, 0, VPLFILE_SEEK_END);
        if (fileSize < 0) {
            LOG_ERROR("Seek File size is invalid "FMTu64, fileSize);
            VPLFile_Close(vfileCtx.fileHandle);
            return fileSize;
        }
        VPLFile_Seek(vfileCtx.fileHandle, 0, VPLFILE_SEEK_SET); // Reset

        rv = MD5Reset(&vfileCtx.hashContext);
        if (rv < 0 ) {
            LOG_ERROR("CSL Reset MD5 fail from file:%s, result:%d", srcLocalFilepath.c_str(), rv);
        }

        rv = httpHandle.Put(FileReadCallback,
                            &vfileCtx,
                            (u64)fileSize,
                            progressCb, progressCbCtx,
                            /*out*/httpPutResponse);

        VPLFile_Close(vfileCtx.fileHandle);
    } else {
        rv = httpHandle.Put(srcLocalFilepath,
                            progressCb, progressCbCtx,
                            /*out*/httpPutResponse);
    }
    if(rv != 0) {
        LOG_ERROR("HTTP Put %d, %s, %s",
                rv, accessInfo.accessUrl.c_str(), srcLocalFilepath.c_str());
        return rv;
    }

    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("HTTP Put status %d mapped to %d, %s, %s",
                rv, httpHandle.GetStatusCode(), accessInfo.accessUrl.c_str(), srcLocalFilepath.c_str());
        goto exit_with_server_response;
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\", response: %s", rv, accessInfo.accessUrl.c_str(), httpPutResponse.c_str());
    }
    if (computeHash == true) {
        rv = MD5Result(&vfileCtx.hashContext, vfileCtx.hashValue);
        if (rv != 0) {
            LOG_ERROR("CSL MD5 get result fail from file:%s, result:%d", srcLocalFilepath.c_str(), rv);
        } else {
            hash__out.clear();
            for(int hashIndex = 0; hashIndex < MD5_DIGESTSIZE; hashIndex++) {
                char byteStr[4];
                snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, vfileCtx.hashValue[hashIndex]);
                hash__out.append(byteStr);
            }
            LOG_DEBUG("File: %s,Hash:%s", srcLocalFilepath.c_str(), hash__out.c_str());
        }
    }
    return rv;
}

int vcs_s3_putFileHelper(const VcsAccessInfo& accessInfo,
                         VPLHttp2& httpHandle,
                         VPLHttp2_ProgressCb progressCb,
                         void* progressCbCtx,
                         const std::string& srcLocalFilepath,
                         bool printLog)
{
    std::string tmp;
    return vcs_s3_putFileHelperInternal(accessInfo,httpHandle, progressCb,
         progressCbCtx, srcLocalFilepath, printLog, tmp,
         false);
}

int vcs_s3_putFileHelper(const VcsAccessInfo& accessInfo,
                         VPLHttp2& httpHandle,
                         VPLHttp2_ProgressCb progressCb,
                         void* progressCbCtx,
                         const std::string& srcLocalFilepath,
                         bool printLog,
                         std::string& hash__out)
{
    return vcs_s3_putFileHelperInternal(accessInfo,httpHandle, progressCb,
         progressCbCtx, srcLocalFilepath, printLog, hash__out,
         true);
}

int vcs_s3_getFileHelper(const VcsAccessInfo& accessInfo,
                         VPLHttp2& httpHandle,
                         VPLHttp2_ProgressCb progressCb,
                         void* progressCbCtx,
                         const std::string& destLocalFilepath,
                         bool printLog)
{
    int rv;

    rv = Util_CreatePath(destLocalFilepath.c_str(), false);
    if(rv != 0) {
        LOG_ERROR("Cannot create dest path:%s, %d",
                  destLocalFilepath.c_str(), rv);
        return rv;
    }

    rv = httpHandle.SetDebug(printLog);
    if(rv != 0) {
        LOG_WARN("SetDebug failed(%d), Not critical. Continuing.", rv);
    }

    rv = setVcsAccessInfo(accessInfo, httpHandle);
    if(rv != 0) {
        LOG_ERROR("setVcsAccessInfo:%d", rv);
        return CCD_ERROR_INTERNAL;
    }

    rv = httpHandle.Get(destLocalFilepath, progressCb, progressCbCtx);
    if(rv != 0) {
        LOG_ERROR("HTTP Get %d, %s, %s",
                rv, accessInfo.accessUrl.c_str(), destLocalFilepath.c_str());
        return rv;
    }

    rv = vcs_translate_http_status_code(httpHandle.GetStatusCode());
    if(rv != 0) {
        LOG_ERROR("HTTP Get status %d mapped to %d, %s, %s",
                httpHandle.GetStatusCode(), rv, accessInfo.accessUrl.c_str(), destLocalFilepath.c_str());
        goto exit_with_server_response;
    }

exit_with_server_response:
    if (rv != 0) {
        LOG_WARN("rv=%d, url=\"%s\"", rv, accessInfo.accessUrl.c_str());
    }
    return rv;
}
