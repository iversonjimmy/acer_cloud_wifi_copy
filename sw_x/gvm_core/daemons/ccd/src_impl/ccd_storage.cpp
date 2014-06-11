//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "ccd_storage.hpp"

#include "gvm_file_utils.hpp"
#include "vplex_strings.h"

#include <string>
#ifdef WIN32
#include "vpl_fs.h"
#endif

/// See #CCDStorage_GetRoot().
static std::string s_storageRoot("/ccd_root_not_initialized/");

const char* CCDStorage_GetRoot()
{
    return s_storageRoot.c_str();
}

void CCDStorage_Init(const char* storageRoot)
{
    if (storageRoot == NULL) {
        GVM_ASSERT_FAILED("storageRoot = NULL");
    } else {
        //bug 6840,6917
        // storageRoot = %LOCAL_APP_DATA%
        // 3 = strlen("/cc")
        // 1 = null terminal (\0)
        char tempBuf[LOCAL_APP_DATA_MAX_LENGTH+3+1];
        snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), "%s/cc", storageRoot);
        s_storageRoot = Util_CleanupPath(tempBuf);
    }
}

void CCDStorage_GetCacheRoot(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache", s_storageRoot.c_str());
}

void DiskCache::getPathForAllCachedUsers(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users", s_storageRoot.c_str());
}
void DiskCache::getPathForCachedUser(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64, s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForUserCacheFileTemp(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/userdata.temp",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForUserHttpThumbTemp(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/httpthumb.temp",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getDirectoryForMcaDb(VPLUser_Id_t userId,
        size_t maxPathLen, char* dir_out)
{
    snprintf(dir_out, maxPathLen, "%s/cache/users/%016"PRIx64, s_storageRoot.c_str(), userId);
}
void DiskCache::getDirectoryForMediaMetadataDownload(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/mm",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getDirectoryForMediaMetadataUpload(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/mediaMetadataUp",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getDirectoryForMediaMetadataUploadTemp(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/msa_temp",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getDirectoryForNotesSync(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/notes",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForUserUploadPicstream(VPLUser_Id_t userId,
        size_t maxPathLen, char* dir_out)
{
    snprintf(dir_out, maxPathLen, "%s/cache/users/%016"PRIx64"/picstream",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForUserCacheFile(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/userdata.bin", s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForUserRFAccessControlList(VPLUser_Id_t userId,
        size_t maxPathLen, char* path_out)
{
    //Remote File Access Control List
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/rfacl.bin", s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForDeletion(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/to_delete/", s_storageRoot.c_str());
}
void DiskCache::getRootPathForDeviceCreds(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/device/", s_storageRoot.c_str());
}
void DiskCache::getPathForDeviceCredsClear(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/device/dev_cred_clear", s_storageRoot.c_str());
}
void DiskCache::getPathForDeviceCredsSecret(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/device/dev_cred_secret", s_storageRoot.c_str());
}
void DiskCache::getPathForDeviceId(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/device/deviceId", s_storageRoot.c_str());
}
void DiskCache::getPathForOldRenewalToken(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/device/renewToken", s_storageRoot.c_str());
}
void DiskCache::getPathForNewRenewalToken(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/device/renewal_token", s_storageRoot.c_str());
}
void DiskCache::getPathForStorageNode(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/sn/", s_storageRoot.c_str());
}
void DiskCache::getPathForUpdate(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/update/", s_storageRoot.c_str());
}
void DiskCache::getPathForDocSNG(size_t maxPathLen, char* path_out)
{
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    // For Windows PC, to solve bug 9617, we need to put the DocTrackerDB to a place that will not be deleted, even user uninstall the application.
    char *DocTrackerDBpath = NULL;
    int rv = _VPLFS__GetLocalAppDataPath(&DocTrackerDBpath);
    if (rv == VPL_OK) {
        // Get %localappdata% by _VPLFS__GetLocalAppDataPath correctly
        snprintf(path_out, maxPathLen, "%s/acer/dsng/", DocTrackerDBpath);
    } else {
        // To solve bug 12112: Assume s_storageRoot = %localappdata%/clear.fi/AcerCloud/SyncAgent/cc
        std::string token;
        std::string dsngDbPath;
        token.assign("/clear.fi/AcerCloud/SyncAgent/cc");
        dsngDbPath.assign(s_storageRoot);
        u32 index = dsngDbPath.find(token);
        if (index != std::string::npos) {
            dsngDbPath.replace(index, token.length(), "");
        } else {
            FAILED_ASSERT("Root path token has been changed!");
        }
        snprintf(path_out, maxPathLen, "%s/acer/dsng/", dsngDbPath.c_str());   
    }
#else
    snprintf(path_out, maxPathLen, "%s/dsng/", s_storageRoot.c_str());
#endif

}
void DiskCache::getPathForSyncBack(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/sb/", s_storageRoot.c_str());
}
void DiskCache::getPathForSyncDown(u64 userId, size_t maxPathLen, char* path_out)
{
    // Historical note: before 3.1.0, this used to be
    // snprintf(path_out, maxPathLen, "%s/sd/", s_storageRoot.c_str());
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/sd/",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForSharedByMe(u64 userId, size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/shared/sbm",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForSharedWithMe(u64 userId, size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/users/%016"PRIx64"/shared/swm",
             s_storageRoot.c_str(), userId);
}
void DiskCache::getPathForSyncUp(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/su/", s_storageRoot.c_str());
}
void DiskCache::getPathForPIN(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/pin/", s_storageRoot.c_str());
}
void DiskCache::getPathForRemoteExecutable(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/rexe/", s_storageRoot.c_str());
}
void DiskCache::getPathForCcdMainState(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/main.bin", s_storageRoot.c_str());
}
void DiskCache::getPathForCcdMainStateTemp(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/cache/main.temp", s_storageRoot.c_str());
}
void DiskCache::getPathForAsyncUpload(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/asyn_upload/", s_storageRoot.c_str());
}
void DiskCache::getPathForRemoteFile(size_t maxPathLen, char* path_out)
{
    snprintf(path_out, maxPathLen, "%s/remotefile", s_storageRoot.c_str());
}
