#ifndef __SYNCUP_HPP__
#define __SYNCUP_HPP__

#ifdef __cplusplus

#include <vplu_types.h>

#include <string>

#include "ccdi_rpc.pb.h"

// Start SyncUp engine to start.
// Note that part of the actual work is done asynchronously; 
// thus, successful return does not necessarily mean the engine has actually started.
int SyncUp_Start(u64 userId);

// Stop SyncUp engine.
// If "purge" is true, all outstanding jobs will be removed.
// Note that most of the actual work is done asynchronously;
// thus, successful return does not necessarily mean the engine has actually stopped.
int SyncUp_Stop(bool purge);

// Notify that perhaps connection now available.
int SyncUp_NotifyConnectionChange(bool overrideAnsCheck);

// Add a job.
// Unless "make_copy" is true, the file must persist at "localpath".
int SyncUp_AddJob(const std::string &localpath, bool make_copy, u64 ctime, u64 mtime,
                  u64 datasetid, const std::string &comppath, u64 syncfeature);

// Remove all jobs for the given dataset.
int SyncUp_RemoveJobsByDataset(u64 datasetId);

// Remove all jobs for the given dataset whose local path matches the given prefix.
int SyncUp_RemoveJobsByDatasetSourcePathPrefix(u64 datasetId, const std::string &prefix);

int SyncUp_QueryStatus(u64 userId,
                       u64 datasetId,
                       const std::string& path,
                       ccd::FeatureSyncStateSummary& syncState_out);

// Function overload for New GetSyncState requirement
int SyncUp_QueryStatus(u64 userId,
                       ccd::SyncFeature_t syncFeature,
                       ccd::FeatureSyncStateSummary& syncState_out);

#endif // __cplusplus

#endif // __SYNCUP_HPP__
