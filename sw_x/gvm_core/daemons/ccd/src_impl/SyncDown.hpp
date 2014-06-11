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

#ifndef __SYNCDOWN_HPP__
#define __SYNCDOWN_HPP__

#ifdef __cplusplus

#include <vplu_types.h>

#include <string>
#include <queue>

#include "ccdi_rpc.pb.h"

typedef struct _PicStreamPhotoSet {
    std::string albumName;
    std::string name;
    std::string identifier;
    std::string title;
    std::string compId;
    u64 dateTime;
    u64 ori_deviceid;
    std::string full_url;
    u64 full_size;
    std::string low_url;
    std::string thumb_url;

    std::string lpath;
    u64 fileType;
    std::string origCompId;
    std::string url;
} PicStreamPhotoSet;

enum FileType_Enum {
    FileType_FullRes = 0,
    FileType_LowRes = 1,
    FileType_Thumbnail = 2,
    FileType_Ignored = 3
};

class HttpStream;

// Start SyncDown engine to start.
// Note that part of the actual work is done asynchronously; 
// thus, successful return does not necessarily mean the engine has actually started.
int SyncDown_Start(u64 userId);

// Stop SyncDown engine.
// If "purge" is true, all outstanding jobs will be removed.
// Note that most of the actual work is done asynchronously;
// thus, successful return does not necessarily mean the engine has actually stopped.
int SyncDown_Stop(bool purge);

// Notify dataset content change.  Download only happens when ans is available,
// unless, overrideAnsCheck is true.
int SyncDown_NotifyDatasetContentChange(u64 datasetid, bool overrideAnsCheck, bool syncDownAdded, int syncDownFileType);

// Reset the last-checked version number of the given dataset.
int SyncDown_ResetDatasetLastCheckedVersion(u64 datasetid);

// Remove all jobs for the given dataset.
int SyncDown_RemoveJobsByDataset(u64 datasetId);

int SyncDown_QueryStatus(u64 userId,
                         u64 datasetId,
                         const std::string& path,
                         ccd::FeatureSyncStateSummary& syncState_out);

int SyncDown_QueryStatus(u64 userId,
                         ccd::SyncFeature_t syncFeature,
                         ccd::FeatureSyncStateSummary& syncState_out);

int SyncDown_QueryPicStreamItems(const std::string& searchField,
                                 const std::string& sortOrder,
                                 std::queue< PicStreamPhotoSet > &output);

int SyncDown_QueryPicStreamItems(ccd::PicStream_DBFilterType_t filter_type,
                                const std::string& searchField,
                                const std::string& sortOrder,
                                google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output);

int SyncDown_QuerySharedFilesItems(ccd::SyncFeature_t sync_feature,
                                   const std::string& searchField,
                                   const std::string& sortOrder,
                                   google::protobuf::RepeatedPtrField<ccd::SharedFilesQueryObject> &output);

int SyncDown_QuerySharedFilesEntryByCompId(u64 compId,
                                           u64 syncFeature,
                                           u64& revision_out,
                                           u64& datasetId_out,
                                           std::string& name_out,
                                           bool& thumbDownloaded_out,
                                           std::string& thumbRelPath_out);

int SyncDown_DownloadOnDemand(u64 datasetId,
                              const std::string& compPath,
                              u64 compId,
                              u64 fileType,
                              HttpStream *hs,
                              /*OUT*/std::string &filePath);

void SyncDown_GetCacheFilePath(u64 userId,
                               u64 datasetId,
                               u64 compId,
                               u64 fileType,
                               const std::string& compPath,
                               /*OUT*/ std::string &cacheFile);

void SyncDown_SetEnableGlobalDelete(bool enable = true);
int SyncDown_GetEnableGlobalDelete(bool &enable);

int SyncDown_AddPicStreamItem(  u64 compId, const std::string &name, u64 rev, u64 origCompId, u64 originDev,
                                const std::string &identifier, const std::string &title, const std::string &albumName,
                                u64 fileType, u64 fileSize, u64 takenTime, const std::string &lpath, /*OUT*/ u64 &jobid, bool &syncUpAdded);

#endif // __cplusplus

#endif // __SYNCDOWN_HPP__
