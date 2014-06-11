//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCD_STORAGE_HPP__
#define __CCD_STORAGE_HPP__

//---------------------------------------------------
/// @file 
/// This dictates the directory structure of the disk cache.
/// Let's keep all of the definitions in one place.
//---------------------------------------------------

#include "base.h"

void CCDStorage_Init(const char* storageRoot);

/// Local filesystem path where CCD will store its private data.
/// This will not have a trailing "/".
/// Determined by:
/// - #CCDStart() parameter (if provided), otherwise
/// - ccd.conf property (if provided), otherwise
/// - platform-specific default (see #GVM_DEFAULT_LOCAL_APP_DATA_PATH)
/// .
const char* CCDStorage_GetRoot();

/// @a path_out should be CCD_PATH_MAX_LENGTH
void CCDStorage_GetCacheRoot(size_t maxPathLen, char* path_out);

class DiskCache {
 public:
    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForAllCachedUsers(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForCachedUser(
            VPLUser_Id_t userId, size_t maxPathLen, char* path_out);

    /// @a dir_out should be CCD_PATH_MAX_LENGTH
    static void getDirectoryForMcaDb(VPLUser_Id_t userId,
            size_t maxPathLen, char* dir_out);

    /// @a dir_out should be CCD_PATH_MAX_LENGTH
    static void getDirectoryForMediaMetadataDownload(VPLUser_Id_t userId,
            size_t maxPathLen, char* dir_out);

    /// @a dir_out should be CCD_PATH_MAX_LENGTH
    static void getDirectoryForMediaMetadataUpload(VPLUser_Id_t userId,
            size_t maxPathLen, char* dir_out);

    /// @a dir_out should be CCD_PATH_MAX_LENGTH
    static void getDirectoryForMediaMetadataUploadTemp(VPLUser_Id_t userId,
            size_t maxPathLen, char* dir_out);

    /// @a dir_out should be CCD_PATH_MAX_LENGTH
    static void getDirectoryForNotesSync(VPLUser_Id_t userId,
            size_t maxPathLen, char* dir_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForUserCacheFile(
            VPLUser_Id_t userId, size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForUserRFAccessControlList(
            VPLUser_Id_t userId, size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForUserCacheFileTemp(
            VPLUser_Id_t userId, size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForUserHttpThumbTemp(
            VPLUser_Id_t userId, size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForUserUploadPicstream(
            VPLUser_Id_t userId, size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForDeletion(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getRootPathForDeviceCreds(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForDeviceCredsClear(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForDeviceCredsSecret(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForDeviceId(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForOldRenewalToken(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForNewRenewalToken(size_t maxPathLen, char* path_out);
    
    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForStorageNode(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForUpdate(size_t maxPathLen, char* path_out);
    
    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForDocSNG(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForSyncBack(size_t maxPathLen, char* path_out);
    
    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForSyncDown(u64 userId, size_t maxPathLen, char* path_out);
    
    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForSharedByMe(u64 userId, size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForSharedWithMe(u64 userId, size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForSyncUp(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForPIN(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForRemoteExecutable(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForCcdMainState(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForCcdMainStateTemp(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForAsyncUpload(size_t maxPathLen, char* path_out);

    /// @a path_out should be CCD_PATH_MAX_LENGTH
    static void getPathForRemoteFile(size_t maxPathLen, char* path_out);

};

#endif // include guard
