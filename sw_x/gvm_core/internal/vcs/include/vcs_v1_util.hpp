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
#ifndef VCS_V1_UTIL_HPP_4_29_2014__
#define VCS_V1_UTIL_HPP_4_29_2014__


// Functions that are split out or even repeated from vcs_util.hpp.
// This is because some modules, such as picstream require VCS requests that are
// VCS_API_VERSION 1, a fairly different style from the VCS_API_VERSION 3 API.

#include "vcs_defs.hpp"
#include "vcs_common.hpp"

int VcsV1_getAccessInfo(const VcsSession& vcsSession,
                        VPLHttp2& httpHandle,
                        const VcsDataset& dataset,
                        bool printLog,
                        VcsAccessInfo& accessInfo_out);

struct VcsV1_revisionListEntry {
    u64 revision;
    u64 size;
    VPLTime_t lastChanged;
    u64 updateDevice;
    std::string previewUri;

    void clear() {
        revision = 0;
        size = 0;
        lastChanged = 0;
        updateDevice = 0;
        previewUri.clear();
    }
    VcsV1_revisionListEntry() { clear(); }
};

struct VcsV1_postFileMetadataResponse {
    std::string name;
    u64 compId;
    u64 numOfRevisions;
    u64 originDevice;
    std::vector<VcsV1_revisionListEntry> revisionList;
    void clear() {
        name.clear();
        compId = 0;
        numOfRevisions = 0;
        originDevice = 0;
        revisionList.clear();
    }
    VcsV1_postFileMetadataResponse() { clear(); }
};

int VcsV1_share_postFileMetadata(const VcsSession& vcsSession,
                                 VPLHttp2& httpHandle,
                                 const VcsDataset& dataset,
                                 const std::string& path,
                                 VPLTime_t lastChanged,
                                 VPLTime_t createDate,
                                 u64 size,
                                 bool hasOpaqueMetadata,
                                 const std::string& opaqueMetadata,
                                 bool hasContentHash,
                                 const std::string& contentHash,
                                 u64 updateDevice,
                                 const std::string& accessUrl,
                                 bool printLog,
                                 VcsV1_postFileMetadataResponse& response_out);

int VcsV1_putPreview(const VcsSession& vcsSession,
                     VPLHttp2& httpHandle,
                     const VcsDataset& dataset,
                     const std::string& path,
                     u64 compId,
                     u64 revisionId, // For PicStream, the revision should be always 1
                     const std::string& destLocalFilepath,
                     bool printLog);

struct VcsV1_getRevisionListEntry {
    u64 revision;
    u64 size;
    VPLTime_t lastChanged;
    u64 updateDevice;
    u64 baseRevision;
    std::string previewUri;
    bool noAcs;
    std::string downloadUrl;

    void clear() {
        revision = 0;
        size = 0;
        lastChanged = 0;
        updateDevice = 0;
        previewUri.clear();
    }
    VcsV1_getRevisionListEntry() { clear(); }
};

struct VcsV1_getFileMetadataResponse {
    std::string name;
    u64 compId;
    u64 numOfRevisions;
    u64 originDevice;
    VPLTime_t lastChangedNano;
    VPLTime_t createDateNano;
    std::string opaqueMetadata;
    std::vector<std::string> recipientList;
    std::vector<VcsV1_getRevisionListEntry> revisionList;
    void clear() {
        name.clear();
        compId = 0;
        numOfRevisions = 0;
        originDevice = 0;
        lastChangedNano = 0;
        createDateNano = 0;
        opaqueMetadata.clear();
        recipientList.clear();
        revisionList.clear();
    }
    VcsV1_getFileMetadataResponse() { clear(); }
};

int VcsV1_getFileMetadata(const VcsSession& vcsSession,
                          VPLHttp2& httpHandle,
                          const VcsDataset& dataset,
                          const std::string& filepath,
                          u64 compId,
                          bool printLog,
                          VcsV1_getFileMetadataResponse& response);

//int VcsPic_getPreview(const VcsSession& vcsSession,
//                      VPLHttp2& httpHandle,
//                      const VcsDataset& dataset,
//                      u64 compId,
//                      u64 revisionId, // For PicStream, the revision should be always 1
//                      const std::string& destLocalFilepath,
//                      bool printLog);
//
//struct VcsPic_getDatasetChangesEntry {
//    std::string name;
//    u64 compId;
//    bool isFile;
//    bool isDeleted;
//};
//
//struct VcsPic_getDatasetChangesResponse {
//    u64 currentVersion;
//    u64 nextVersion;
//    std::vector<VcsPic_getDatasetChangesEntry> changeList;
//
//    void clear() {
//        currentVersion = 0;
//        nextVersion = 0;
//        changeList.clear();
//    }
//
//    VcsPic_getDatasetChangesResponse() {
//        clear();
//    }
//};
//
//int VcsPic_getDatasetChanges(const VcsSession& vcsSession,
//                             VPLHttp2& httpHandle,
//                             const VcsDataset& dataset,
//                             u64 changeSince,
//                             int max,
//                             bool includeDeleted,
//                             bool printLog,
//                             VcsPic_getDatasetChangesResponse& response);

#endif // VCS_V1_UTIL_HPP_4_29_2014__
