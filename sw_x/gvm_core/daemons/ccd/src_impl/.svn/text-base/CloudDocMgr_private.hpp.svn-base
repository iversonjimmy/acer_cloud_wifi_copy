/*
 *  Copyright 2012 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#ifndef __DOCSYNCDOWN_PRIVATE_HPP__
#define __DOCSYNCDOWN_PRIVATE_HPP__

#ifdef __cplusplus

#include <ccd_features.h>
#include <vpl_types.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <vplex_http2.hpp>
#include <sqlite3.h>
#ifdef CLOUDNODE
#include <dataset.hpp>
#endif
#include "ccdi_rpc.pb.h"

#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

// Naming convention:
// Names beginning with an upper-case char are intended for use by others.
// Names beginning with a lower-case char are intended for internal use.

class DocSyncDownJob {
    friend class DocSyncDownJobs;
public:
    u64 id;
    u64 did;
    std::string docname;
    u64 compid;
    std::string lpath;
    std::string dl_path;
    u64 dl_rev;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

class DocSyncDownJobEx {
    friend class DocSyncDownJobs;
public:
    u64 id;
    u64 did;
    std::string docname;
    u64 compid;
    std::string lpath;
    u64 add_ts;
    u64 disp_ts;
    std::string dl_path;
    u64 dl_try_ts;
    u64 dl_failed;
    u64 dl_done_ts;
    u64 dl_rev;
    u64 cb_try_ts;
    u64 cb_failed;
    u64 cb_done_ts;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

// Return value of non-zero will cause traversal of jobs to be abandoned.
typedef int (*JobVisitor)(DocSyncDownJob &job, void *param);

class DocSyncDownJobs {
    friend class TestDocSyncDownJobs;
public:
    // assumption: workdir ends with '/'
    DocSyncDownJobs(const std::string &workdir);
    ~DocSyncDownJobs();

    int Check();

    int SetDatasetLastCheckedVersion(u64 datasetid, u64 version);
    int GetDatasetLastCheckedVersion(u64 datasetid, /*OUT*/ u64 &version);
    int ResetAllDatasetLastCheckedVersions();

    int AddJob(u64 datasetid, const std::string &compname, u64 compid,
               const std::string &localpath,
               /*OUT*/ u64 &jobid);
    int RemoveJob(u64 jobid);
    int RemoveJobsByDatasetId(u64 datasetId);
    int RemoveAllJobs();

    int GetNextJobToDownload(u64 cutoff, /*OUT*/ DocSyncDownJob &job);
    int GetNextJobToCopyback(u64 cutoff, /*OUT*/ DocSyncDownJob &job);

    int SetJobDownloadPath(u64 jobid, const std::string &downloadpath);
    int SetJobDownloadedRevision(u64 datasetid, u64 revision);

    int TimestampJobDispatch(u64 jobid, u64 vplCurrTimeSec);
    int TimestampJobTryDownload(u64 jobid);
    int TimestampJobDoneDownload(u64 jobid);
    int TimestampJobTryCopyback(u64 jobid);
    int TimestampJobDoneCopyback(u64 jobid);

    int ReportJobDownloadFailed(u64 jobid);
    int ReportJobCopybackFailed(u64 jobid);

    int PurgeOnClose();

    int QueryStatus(u64 datasetId,
                    const std::string& path,
                    ccd::FeatureSyncStateSummary& syncState_out);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    int CountJobinUploadRequestbyPath(std::string filePath, u32& count);
    int CountJobinUploadRequestbyDocName(std::string docname, u32& count);
    int CountJobinDownloadRequestbyCompid(u64 compId, u32& count);
    int GetRevisionbyCompid(u64 compId, u64 &size, std::string &hashval, u64 &rev);
    std::string ComputeSHA1(std::string filePath);
    int UpdateHashbyCompId(u64 compId, std::string filePath);
    int UpdateRevInComponent(u64 compId, std::string name, u64 ctime, u64 mtime, u64 cur_rev);
    int UpdateRevInRevision(u64 compId, u64 rev, u64 mtime, u64 update_dev);
    int WriteComponentToDB(u64 compid, std::string docname, u64 ctime, u64 mtime, u64 origin_dev, u64 cur_rev);
    int WriteRevisionToDB(u64 compid, u64 rev, u64 mtime, u64 update_dev);
    int CountUnfinishedJobsinDB(std::string docname, u64 compId, u32 &count);
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

private:
    std::string workdir;
    std::string dbpath;
    sqlite3 *db;
    std::vector<sqlite3_stmt*> dbstmts;
    bool purge_on_close;

    VPLMutex_t mutex;

    int openDB();
    int closeDB();

    int beginTransaction();
    int commitTransaction();
    int rollbackTransaction();

    int migrate_jobs_to_v2();

    int getAdminValue(u64 adminId, /*OUT*/ u64 &value);

    int setDatasetVersion(u64 datasetid, u64 version);
    int getDatasetVersion(u64 datasetid, /*OUT*/ u64 &version);
    int removeDatasetVersion(u64 datasetId);
    int removeAllDatasetVersions();

    int addJob(u64 did, const std::string &docname, u64 compid,
               const std::string &lpath, 
               u64 timestamp, /*OUT*/ u64 &jobid);
    int removeJob(u64 jobid);
    int removeJobsByDatasetId(u64 datasetId);
    int removeAllJobs();

    int findJob(u64 jobid, /*OUT*/ DocSyncDownJob &job);
    int findJob(const std::string &localpath, /*OUT*/ DocSyncDownJob &job);
    int findJobDownloadNotDoneNoRecentDispatch(u64 cutoff, /*OUT*/ DocSyncDownJob &job);
    int findJobCopybackNotDoneNoRecentDispatch(u64 cutoff, /*OUT*/ DocSyncDownJob &job);
    int findJobEx(u64 jobid, /*OUT*/ DocSyncDownJobEx &job);

    int visitJobs(JobVisitor visitor, void *param);
    int visitJobsByDatasetId(u64 datasetId, bool fails_only,
                             JobVisitor visitor, void *param);
    int visitJobsByDatasetIdLocalpathPrefix(u64 datasetId,
                                            const std::string &prefix,
                                            bool fails_only,
                                            JobVisitor visitor,
                                            void *param);

    int setJobDownloadPath(u64 jobid, const std::string &downloadpath);
    int setJobDownloadedRevision(u64 jobid, u64 revision);

    int setJobDispatchTime(u64 jobid, u64 timestamp);
    int setJobTryDownloadTime(u64 jobid, u64 timestamp);
    int setJobDoneDownloadTime(u64 jobid, u64 timestamp);
    int setJobTryCopybackTime(u64 jobid, u64 timestamp);
    int setJobDoneCopybackTime(u64 jobid, u64 timestamp);

    int incJobDownloadFailedCount(u64 jobid);
    int incJobCopybackFailedCount(u64 jobid);

    int setPurgeOnClose(bool purge);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    /*
    int writeComponentToDB(DocSNGTicket *tp);
    int writeRevisionToDB(DocSNGTicket *tp);
    int deleteComponentFromDB(DocSNGTicket *tp);
    int deleteRevisionFromDB(DocSNGTicket *tp);
    int getCompid_by_name(DocSNGTicket *tp);
    */
    std::string calculateRowChecksum(std::string input);
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC
};

// Abstraction of SyncDown destination storage.
// For CloudNode, it maps to the "Virt Drive" dataset.
// For others, it maps to the local disk.
class DocSyncDownDstStor {
public:
    DocSyncDownDstStor();
    ~DocSyncDownDstStor();
    int init(u64 userId);
    bool fileExists(const std::string &path);
    int createMissingAncestorDirs(const std::string &path);
    int copyFile(const std::string &srcPath, const std::string &dstPath);
private:
#ifdef CLOUDNODE
    u64 dstDatasetId;
    dataset *dstDataset;

    int getVirtDriveDatasetId(u64 userId);
#endif // CLOUDNODE
};

class DocSyncDown {
public:
    // assumption: workdir ends with '/'
    DocSyncDown(const std::string &workdir);
    ~DocSyncDown();

    // start processing
    int Start(u64 userId);

    // stop processing
    int Stop(bool purge);

    // Report dataset change.
    int NotifyDatasetContentChange(u64 datasetid, bool overrideAnsCheck);

    // Reset last-checked dataset version.
    int ResetDatasetLastCheckedVersion(u64 datasetId);

    int RemoveJobsByDatasetId(u64 datasetId);

    int QueryStatus(u64 userId,
                    u64 datasetId,
                    const std::string& path,
                    ccd::FeatureSyncStateSummary& syncState_out);

    int QueryStatus(u64 userId,
                    ccd::SyncFeature_t syncFeature,
                    ccd::FeatureSyncStateSummary& syncState_out);
    int CountUnfinishedJobsinDB(std::string docname, u64 compId, u32 &count);

private:
    VPLMutex_t mutex;
    VPLCond_t cond;

    std::string workdir;
    std::string tmpdir;

    u64 userId;

    DocSyncDownJobs *jobs;

    bool thread_spawned;
    bool thread_running;
    bool exit_thread;

    bool overrideAnsCheck;
    bool isDoingWork;
    std::set<u64> dataset_changes_set;
    std::queue<u64> dataset_changes_queue;

    std::map<u64, VPLHttp2*> httpInProgress;

    int spawnDispatcherThread();
    int signalDispatcherThreadToStop();

    static VPLTHREAD_FN_DECL dispatcherThreadMain(void *arg);
    void dispatcherThreadMain();

    int getChanges(u64 datasetid, u64 changeSince);
    int applyChanges();
    int getJobDownloadReady(u64 threshold, DocSyncDownJob &job);
    int getJobCopybackReady(u64 threshold, DocSyncDownJob &job);
    int tryDownload(DocSyncDownJob &job);
    int tryCopyback(DocSyncDownJob &job);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    int getLatestRevNum(DocSyncDownJob &job, /*OUT*/ u64 &revision, u32 &check_result);
    int uploadLocalDoc(std::string filePath, u64 compId, std::string docname, u64 baserev);
    u8 toHex(const u8 &x);
    std::string urlEncode(const std::string &sIn);
#else
    int getLatestRevNum(DocSyncDownJob &job, /*OUT*/ u64 &revision);
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC
    int downloadRevision(DocSyncDownJob &job, u64 revision);

    bool isCloudDocDataset(u64 datasetId);
    int getDatasetName(u64 datasetId, /*OUT*/ std::string &datasetName);

};

#endif // __cplusplus

#endif // __DOCSYNCDOWN_PRIVATE_HPP__
