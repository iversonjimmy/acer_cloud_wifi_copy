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

#ifndef __VCS_UTIL_HPP_2_14_2013__
#define __VCS_UTIL_HPP_2_14_2013__

#include "vcs_common.hpp"
#include "vcs_defs.hpp"
#include "vcs_errors.hpp"
#include "vpl_plat.h"
#include "vplex_http2.hpp"
#include <vector>

#define ARCHIVE_ACCESS_URI_SCHEME               "acer-ts"

struct VcsFileRevision {
    u64 revision;
    u64 size;
    std::string previewUri;
    std::string downloadUrl;
    VPLTime_t lastChangedSecResolution;
    u64 updateDevice;
    bool noAcs;

    void clear() {
        revision = 0;
        size = 0;
        previewUri.clear();
        downloadUrl.clear();
        lastChangedSecResolution = 0;
        updateDevice = 0;
        noAcs = false;
    }
    VcsFileRevision(){ clear(); }
};

struct VcsFile {
    std::string name;
    u64 compId;
    u64 version;
    u64 originDevice;
    u64 numRevisions;
    VPLTime_t lastChanged;
    VPLTime_t createDate;
    VcsFileRevision latestRevision;
    std::string hashValue;

    void clear() {
        name.clear();
        compId = 0;
        version = 0;
        originDevice = 0;
        numRevisions = 0;
        lastChanged = 0;
        createDate = 0;
        latestRevision.clear();
        hashValue.clear();
    }
    VcsFile(){ clear(); }
};


struct VcsFolder {
    std::string name;
    u64 compId;
    u64 version;

    void clear() {
        name.clear();
        compId = 0;
        version = 0;
    }
    VcsFolder(){ clear(); }
};

struct VcsGetDirResponse {
    std::vector<VcsFile> files;
    std::vector<VcsFolder> dirs;
    u64 numOfFiles;
    u64 currentDirVersion;

    void clear() {
        files.clear();
        dirs.clear();
        numOfFiles = 0;
        currentDirVersion = 0;
    }
    VcsGetDirResponse(){ clear(); }
};

struct VcsMakeDirResponse {
    std::string name;
    u64 compId;
    u64 originDevice;
    VPLTime_t lastChangedSecResolution;

    void clear() {
        name.clear();
        compId = 0;
        originDevice = 0;
        lastChangedSecResolution = 0;
    }
    VcsMakeDirResponse(){ clear(); }
};

// For now this struct can be shared by PostFileMetadata and GetFileMetadata.
// Be sure to split this struct at the first sign of divergence.
struct VcsFileMetadataResponse {
    std::string name;
    u64 compId;
    u64 numOfRevisions;
    u64 originDevice;
    VPLTime_t lastChanged;
    VPLTime_t createDate;
    std::vector<VcsFileRevision> revisionList;
    void clear() {
        name.clear();
        compId = 0;
        numOfRevisions = 0;
        originDevice = 0;
        lastChanged = 0;
        createDate = 0;
        revisionList.clear();
    }
    VcsFileMetadataResponse(){ clear(); }
};

// Same inputs and outputs as vcs_read_folder above.
// Additional Inputs:
//   startIndex - Return the results starting at the given index.
//                startIndex of 1 means starting from the beginning.
//   maxPerPage - Return at most <max> results.
int vcs_read_folder_paged(const VcsSession& vcsSession,
                          VPLHttp2& httpHandle,
                          const VcsDataset& dataset,
                          const std::string& folder,
                          u64 compId,
                          u64 startIndex,
                          u64 maxPerPage,
                          bool printLog,
                          VcsGetDirResponse& getDirResponse);

int vcs_access_info_for_file_get(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 const std::string& path,
                                 u64 compId,
                                 u64 revision,
                                 bool printLog,
                                 VcsAccessInfo& accessInfoResponse);
int vcs_access_info_for_file_put(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 bool printLog,
                                 VcsAccessInfo& accessInfoResponse);

int vcs_post_file_metadata(const VcsSession& vcsSession,
                           VPLHttp2& httpHandle,
                           const VcsDataset& dataset,
                           const std::string& path,
                           u64 parentCompId,
                           bool hasCompId,      // compId is unknown for new files
                           u64 compId,          // Only valid when hasCompId==true
                           u64 uploadRevision,  // lastKnownRevision+1, (for example, 1 for new files)
                           VPLTime_t lastChanged,
                           VPLTime_t createDate,
                           u64 fileSize,
                           const std::string& contentHash,
                           const std::string& clientGeneratedHash, // Hash value of the local file, as
                                                                   // computed by the local device.
                           u64 infoUpdateDeviceId,
                           bool accessUrlExists,
                           const std::string& accessUrl,
                           bool printLog,
                           VcsFileMetadataResponse& response_out);

//===========================================================================
//==================== VCS BatchPostFileMetadata ============================
// Reqest/Response specified here: http://wiki.ctbg.acer.com/wiki/index.php/VCS_Batch_APIs

struct VcsBatchFileMetadataRequest
{
    u64 folderCompId;
    std::string folderPath;

    std::string fileName;
    u64 size;
    u64 updateDevice;
    bool hasAccessUrl;
    std::string accessUrl;
    VPLTime_t lastChanged;
    VPLTime_t createDate;
    bool hasContentHash;
    std::string contentHash;
    u64 uploadRevision;
    bool hasBaseRevision;
    u64 baseRevision;
    bool hasCompId;
    u64 compId;

    void clear()
    {
        folderCompId = 0;
        folderPath.clear();
        fileName.clear();
        size = 0;
        updateDevice = 0;
        hasAccessUrl = false;
        accessUrl.clear();
        lastChanged = 0;
        createDate = 0;
        hasContentHash = false;
        contentHash.clear();
        uploadRevision = 0;
        hasBaseRevision = false;
        baseRevision = 0;
        hasCompId = false;
        compId = 0;
    }
    VcsBatchFileMetadataRequest() { clear(); }
};

struct VcsBatchFileMetadataResponse_RevisionList
{
    u64 revision;
    u64 size;
    VPLTime_t lastChangedSecResolution;
    u64 updateDevice;
    std::string previewUri;
    bool noACS;

    void clear()
    {
        revision = 0;
        size = 0;
        lastChangedSecResolution = 0;
        updateDevice = 0;
        previewUri.clear();
        noACS = false;
    }
    VcsBatchFileMetadataResponse_RevisionList() { clear(); }
};

struct VcsBatchFileMetadataResponse_File
{
    std::string name;
    bool success;
    s32 errCode;
    u64 compId;
    u64 numOfRevisions;
    VPLTime_t lastChanged;
    VPLTime_t createDate;
    std::vector<VcsBatchFileMetadataResponse_RevisionList> revisionList;

    void clear()
    {
        name.clear();
        success = false;
        errCode = 0;
        compId = 0;
        numOfRevisions = 0;
        lastChanged = 0;
        createDate = 0;
        revisionList.clear();
    }
    VcsBatchFileMetadataResponse_File() { clear(); }
};

struct VcsBatchFileMetadataResponse_Folder
{
    u64 folderCompId;
    std::vector<VcsBatchFileMetadataResponse_File> files;

    void clear()
    {
        folderCompId = 0;
        files.clear();
    }
    VcsBatchFileMetadataResponse_Folder() { clear(); }
};

struct VcsBatchFileMetadataResponse
{
    u64 entriesReceived;
    u64 entriesFailed;
    std::vector<VcsBatchFileMetadataResponse_Folder> folders;

    void clear()
    {
        entriesReceived = 0;
        entriesFailed = 0;
        folders.clear();
    }
    VcsBatchFileMetadataResponse() { clear(); }
};

int vcs_batch_post_file_metadata(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 bool printLog,
                                 const std::vector<VcsBatchFileMetadataRequest>& request,
                                 VcsBatchFileMetadataResponse& response_out);

//==================== VCS BatchPostFileMetadata ============================
//===========================================================================

int vcs_get_file_metadata(const VcsSession& vcsSession,
                          VPLHttp2& httpHandle,
                          const VcsDataset& dataset,
                          const std::string& filepath,
                          u64 compId,
                          bool printLog,
                          VcsFileMetadataResponse& response_out);

//  vcsSession - all fields must be populated
//  httpHandle - newly allocated handle (single use).  Timeout time may be already
//              set.  Allows for cancellation.
//  filepath - filepath relative to dataset root to be deleted
//  compId - component ID of filepath to be deleted
//  revision - revision of filepath to be deleted.
//  printLog - set option to print verbose log
// Returns:
//  0 upon success.
//  TODO: VCS_ERR_CONFLICT_DETECTED
int vcs_delete_file(const VcsSession& vcsSession,
                    VPLHttp2& httpHandle,
                    const VcsDataset& dataset,
                    const std::string& filepath,
                    u64 compId,
                    u64 revision,
                    bool printLog);

//  vcsSession - all fields must be populated
//  httpHandle - newly allocated handle (single use).  Timeout time may be already
//              set.  Allows for cancellation.
//  filepath - filepath relative to dataset root to be deleted
//  compId - component ID of filepath to be deleted
//  isRecursive - If true, recursively deletes.
//                If false, deletes directory only if empty.
//  hasDirDatasetVersion - dirDatasetVersion is specified.
//  dirDatasetVersion - If hasDirDatasetVersion is true, directory must match
//                      the datasetVersion if deletion is to succeed.
//  printLog - set option to print verbose log
// Returns:
//  0 upon success.
//  TODO: VCS_ERR_CONFLICT_DETECTED
int vcs_delete_dir(const VcsSession& vcsSession,
                   VPLHttp2& httpHandle,
                   const VcsDataset& dataset,
                   const std::string& dirpath,
                   u64 compId,
                   bool isRecursive,
                   bool hasDirDatasetVersion,
                   u64 dirDatasetVersion,
                   bool printLog);

//  vcsSession - all fields must be populated
//  httpHandle - newly allocated handle (single use).  Timeout time may be already
//              set.  Allows for cancellation.
//  dirpath - dirpath relative to dataset root to be added
//  parentCompId - component ID of parent directory
//  printLog - set option to print verbose log
// Returns:
//  0 upon success.
//  makeDirResponse - response from the makeDir request,
//                  - Of interest is the new component id.
// Errors include:
//  Error http status error codes
//  VCS_ERR_PARENT_COMPID_NOT_FOUND
int vcs_make_dir(const VcsSession& vcsSession,
                 VPLHttp2& httpHandle,
                 const VcsDataset& dataset,
                 const std::string& dirpath,
                 u64 parentCompId,
                 VPLTime_t infoLastChanged,
                 VPLTime_t infoCreateDate,
                 u64 infoUpdateDeviceId,
                 bool printLog,
                 VcsMakeDirResponse& makeDirResponse);

// Expensive operation for the server (the deeper the path, the
// slower the operation) to get the compId of a dirEntry.
//  vcsSession - all fields must be populated
//  httpHandle - newly allocated handle (single use).  Timeout time may be already
//              set.  Allows for cancellation.
//  path - path relative to dataset root to be queried
//  hasRevision - revision is specified.
//  printLog - set option to print verbose log
// Returns:
//  0 upon success.
//  -35352: VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT: Couldn't find the component given the <path> in the requestURI.
//  compId_out - component ID.
int vcs_get_comp_id(const VcsSession& vcsSession,
                    VPLHttp2& httpHandle,
                    const VcsDataset& dataset,
                    const std::string& path,
                    bool printLog,
                    u64& compId_out);

/// Calls VCS POST getdatasetinfo.
int vcs_get_dataset_info(const VcsSession& vcsSession,
                         VPLHttp2& httpHandle,
                         const VcsDataset& dataset,
                         bool printLog,
                         u64& currentVersion_out);

// Specified here: http://wiki.ctbg.acer.com/wiki/index.php/VCS_Virtual_Sync_2.7.0
int vcs_post_acs_access_url_virtual(const VcsSession& vcsSession,
                                    VPLHttp2& httpHandle,
                                    const VcsDataset& dataset,
                                    const std::string& path,
                                    u64 compId,
                                    u64 revisionId,
                                    const std::string& acs_access_url,
                                    bool printLog);

// Used for syncbox archive storage. access_url is composed fully by infra
int vcs_post_archive_url(const VcsSession& vcsSession,
                         VPLHttp2& httpHandle,
                         const VcsDataset& dataset,
                         const std::string& path,
                         u64 compId,     
                         u64 revision,  
                         bool printLog);

// DEPRECATED vcs_read_folder:  Use vcs_read_folder_paged
//  vcsSession - all fields must be populated
//  httpHandle - newly allocated handle (single use).  Timeout time may be already
//              set.  Allows for cancellation.
//  folder - path to directory relative to dataset root
//  compId - component ID of folder.  Pass 0 for dataset root.
//  printLog - set option to print verbose log
// Returns:
//  0 upon success.
//  getDirResponse - (out), when successful, the parsed dir response.
//int vcs_read_folder(const VcsSession& vcsSession,
//                    VPLHttp2& httpHandle,
//                    const VcsDataset& dataset,
//                    const std::string& folder,
//                    u64 compId,
//                    bool printLog,
//                    VcsGetDirResponse& getDirResponse);

#endif /* __VCS_UTIL_HPP_2_14_2013__ */

