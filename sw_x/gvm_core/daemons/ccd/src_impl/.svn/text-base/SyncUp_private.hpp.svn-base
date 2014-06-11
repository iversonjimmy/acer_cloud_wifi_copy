#ifndef __SYNCUP_PRIVATE_HPP__
#define __SYNCUP_PRIVATE_HPP__

#if __cplusplus

#include <vpl_types.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <vplex_http2.hpp>
#include <sqlite3.h>
#include "ccdi_rpc.pb.h"

#include <map>
#include <string>
#include <vector>

// Naming convention:
// Names beginning with an upper-case char are intended for use by others.
// Names beginning with a lower-case char are intended for internal use.

class SyncUpJob {
    friend class SyncUpJobs;
public:
    u64 id;
    std::string opath;
    std::string spath;
    int is_tmp;
    u64 ctime;
    u64 mtime;
    u64 did;
    std::string cpath;
    u64 feature;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

class SyncUpJobEx {
    friend class SyncUpJobs;
public:
    u64 id;
    std::string opath;
    std::string spath;
    int is_tmp;
    u64 ctime;
    u64 mtime;
    u64 did;
    std::string cpath;
    u64 add_ts;
    u64 disp_ts;
    u64 ul_try_ts;
    u64 ul_failed;
    u64 ul_done_ts;
    u64 feature;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

// Return value of non-zero will cause traversal of jobs to be abandoned.
typedef int (*JobVisitor)(SyncUpJob &job, void *param);

class SyncUpJobs {
    friend class TestSyncUpJobs;
public:
    // assumption: workdir ends with '/'
    SyncUpJobs(const std::string &workdir);
    ~SyncUpJobs();

    int Check();

    int AddJob(const std::string &localpath, bool make_copy, u64 ctime, u64 mtime,
               u64 datasetid, const std::string &comppath, u64 syncfeature,
               /*OUT*/ u64 &jobid);
    int QueryStatus(u64 datasetId,
                    const std::string& path,
                    ccd::FeatureSyncStateSummary& syncState_out);
    int RemoveJob(u64 jobid);
    int RemoveJobsByDatasetId(u64 datasetId);
    int RemoveJobsByDatasetIdLocalPathPrefix(u64 datasetId, const std::string &prefix);
    int RemoveAllJobs();

    int GetNextJob(u64 cutoff, /*OUT*/ SyncUpJob &job);

    int TimestampJobDispatch(u64 jobid, u64 currVplTimeSec);
    int TimestampJobTryUpload(u64 jobid);
    int TimestampJobDoneUpload(u64 jobid);

    int ReportJobUploadFailed(u64 jobid);

    int PurgeOnClose();

    int GetJobCountByFeature(u64 feature,
                             u64 cutoff,
                             u32& jobCount_out,
                             u32& failedJobCount_out);

private:
    std::string workdir;
    std::string tmpdir;
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

    int migrate_schema_from_v1_to_v2();

    int getAdminValue(u64 adminId, /*OUT*/ u64 &value);

    int addJob(const std::string &opath, const std::string &spath, bool is_tmp, u64 ctime, u64 mtime,
               u64 did, const std::string &cpath,
               u64 timestamp, u64 syncfeature, /*OUT*/ u64 &jobid);
    int removeJob(u64 jobid);
    int removeJobsByDatasetId(u64 datasetId);
    int removeJobsByDatasetIdOpathPrefix(u64 datasetId, const std::string &prefix);
    int removeAllJobs();

    int findJob(u64 id, /*OUT*/ SyncUpJob &job);
    int findJob(u64 datasetid, const std::string &comppath, /*OUT*/ SyncUpJob &job);
    int findJobNotDoneNoRecentDispatch(u64 cutoff, /*OUT*/ SyncUpJob &job);
    int findJobEx(u64 id, /*OUT*/ SyncUpJobEx &jobex);

    int visitJobs(JobVisitor visitor, void *param);
    int visitJobsByDatasetId(u64 datasetId, bool fails_only, JobVisitor visitor, void *param);
    int visitJobsByDatasetIdOpathPrefix(u64 datasetId,
                                        const std::string &prefix,
                                        bool fails_only,
                                        JobVisitor visitor, void *param);

    int setJobDispatchTime(u64 jobid, u64 timestamp);
    int setJobTryUploadTime(u64 jobid, u64 timestamp);
    int setJobDoneUploadTime(u64 jobid, u64 timestamp);

    int incJobUploadFailedCount(u64 jobid);

    int setPurgeOnClose(bool purge);
};

class SyncUp {
public:
    // assumption: workdir ends with '/'
    SyncUp(const std::string &workdir);
    ~SyncUp();

    // start processing jobs
    int Start(u64 userId);

    // stop processing jobs (purge==true will purge all remaining jobs)
    int Stop(bool purge);

    // Report condition change, possibly resume upload.
    // If overrideAnsCheck is specified, upload will be attempted even if
    // ans is unreachable.
    int NotifyConnectionChange(bool overrideAnsCheck);

    // add a new job
    int AddJob(const std::string &localpath, bool make_copy, u64 ctime, u64 mtime,
               u64 datasetid, const std::string &comppath, u64 syncfeature);

    // remove all jobs destined for the given dataset
    int RemoveJobsByDatasetId(u64 datasetId);

    // remove all jobs destined for the given dataset having a particular source path prefix
    int RemoveJobsByDatasetIdLocalPathPrefix(u64 datasetId, const std::string &prefix);

    int QueryStatus(u64 userId,
                    u64 datasetId,
                    const std::string& path,
                    ccd::FeatureSyncStateSummary& syncState_out);
    
    int QueryStatus(u64 userId,
                    ccd::SyncFeature_t syncFeature,
                    ccd::FeatureSyncStateSummary& syncState_out);

private:
    VPLMutex_t mutex;
    VPLCond_t cond;

    bool overrideAnsCheck;

    std::string workdir;
    u64 userId;
    u64 cutoff_ts;

    SyncUpJobs *jobs;

    bool thread_spawned;
    bool thread_running;
    bool exit_thread;

    std::map<int, int> currentSyncStatus; // <syncFeature, syncStatus>

    std::map<u64, VPLHttp2*> httpInProgress;

    int spawnDispatcherThread();
    int signalDispatcherThreadToStop();

    static VPLTHREAD_FN_DECL syncUpDispatcherThreadMain(void *arg);
    void syncUpDispatcherThreadMain();
    int setSyncFeatureState(ccd::SyncFeature_t feature,
                            ccd::FeatureSyncStateType_t state,
                            u32 &pendingJobCounts,
                            u32 &failedJobCounts,
                            bool checkStateChange);
    int tryUpload(SyncUpJob &job);

    //
    // Photo 3.0: Update local PicStream Database when SyncUp photo via PicStream SyncUp.
    //
    int add2PicStreamDB(const std::string &lpath, const std::string &vcsRespJson);
};

#endif // __cplusplus

#endif // __SYNCUP_PRIVATE_HPP__
