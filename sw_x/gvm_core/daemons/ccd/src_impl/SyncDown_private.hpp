#ifndef __SYNCDOWN_PRIVATE_HPP__
#define __SYNCDOWN_PRIVATE_HPP__

#ifdef __cplusplus

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

struct _PicStreamPhotoSet;
typedef _PicStreamPhotoSet PicStreamPhotoSet;

class HttpStream;

class SyncDownJob {
    friend class SyncDownJobs;
public:
    u64 id;
    u64 did;
    std::string cpath;
    u64 cid;
    std::string lpath;
    std::string dl_path;
    u64 dl_rev;
    u64 feature;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

class SyncDownJobEx {
    friend class SyncDownJobs;
public:
    u64 id;
    u64 did;
    std::string cpath;
    u64 cid;
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
    u64 feature;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

class DeleteJob {
    friend class SyncDownJobs;
public:
    u64 id;
    u64 cid;
    std::string name;
    u64 add_ts;
    u64 disp_ts;
    u64 del_try_ts;
    u64 del_failed;
    u64 del_done_ts;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

class PicStreamItem {
    friend class SyncDownJobs;
public:
    u64 compId;
    std::string name;
    u64 rev;
    u64 origCompId;
    u64 origDev;
    std::string identifier;
    std::string title;
    std::string albumName;
    u64 fileType;
    u64 fileSize;
    u64 takenTime;
    std::string lpath;
private:
    int populateFrom(sqlite3_stmt *stmt);
};

// Return value of non-zero will cause traversal of jobs to be abandoned.
typedef int (*JobVisitor)(SyncDownJob &job, void *param);

class SyncDownJobs {
    friend class TestSyncDownJobs;
public:
    // assumption: workdir ends with '/'
    SyncDownJobs(const std::string &workdir);
    ~SyncDownJobs();

    int Check();

    int SetDatasetLastCheckedVersion(u64 datasetid, u64 version);
    int GetDatasetLastCheckedVersion(u64 datasetid, /*OUT*/ u64 &version);
    int ResetDatasetLastCheckedVersion(u64 datasetId);
    int ResetAllDatasetLastCheckedVersions();

    int MapFullLowResInDatabase();

    int AddJob(u64 datasetid, const std::string &compname, u64 compid,
               const std::string &localpath, u64 syncfeature, u64 rev,
               /*OUT*/ u64 &jobid);
    int RemoveJob(u64 jobid);
    int RemoveJobsByDatasetId(u64 datasetId);
    int RemoveAllJobs();

    int GetNextJobToDownload(u64 cutoff, /*OUT*/ SyncDownJob &job);
    int GetNextJobToCopyback(u64 cutoff, /*OUT*/ SyncDownJob &job);

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
    int GetJobCountByFeature(u64 feature,
                             u64 cutoff,
                             u32& jobCount_out,
                             u32& failedJobCount_out);

    int AddDeleteJob(u64 compId, const std::string &name,
               u64 timestamp, /*OUT*/ u64 &jobid);
    int GetDeleteJobCount(u64 cutoff,
                             u32& jobCount_out,
                             u32& failedJobCount_out);
    int GetNextJobToDelete(u64 cutoff, /*OUT*/DeleteJob &job);
    int TimestampJobDeleteDispatch(u64 jobid);
    int TimestampJobDeleteTry(u64 jobid);
    int TimestampJobDeleteDone(u64 jobid);
    int ReportJobDeleteFailed(u64 jobid);
    int SetJobDeleteFailedInfo(u64 jobid,std::string &lpath);
    int RemoveAllJobDelete();
    int RemoveJobDelete(u64 jobid);

    int AddPicStreamItem(u64 compId,
                         const std::string &name,
                         u64 rev,
                         u64 origCompId,
                         u64 origDev,
                         const std::string &identifier,
                         const std::string &title,
                         const std::string &albumName,
                         u64 fileType,
                         u64 fileSize,
                         u64 takenTime,
                         const std::string &lpath,
                         u64 &jobid_out,
                         bool &syncUpAdded_out);
    int RemovePicStreamItems(u64 fullCompId);
    int RemovePicStreamItem(u64 CompId);
    int FindPicStreamItems(u64 fullCompId, std::queue<PicStreamItem> &items);
    int FindPicStreamItem(u64 compId, PicStreamItem &item);

    int QueryPicStreamItems(ccd::PicStream_DBFilterType_t filter_type,
            const std::string& searchField,
            const std::string& sortOrder,
            google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output);

    int QueryPicStreamItems(const std::string& searchField,
                            const std::string& sortOrder,
                            std::queue<PicStreamPhotoSet> &output);

    // update the local path in PicStream table.
    int UpdateLocalPath(const std::string& name, const std::string& localPath);

    int querySharedFilesItems(ccd::SyncFeature_t syncFeature,
            const std::string& searchField,
            const std::string& sortOrder,
            google::protobuf::RepeatedPtrField<ccd::SharedFilesQueryObject>& output);

    int getJobByCompIdAndDatasetId(u64 compId,
                                   u64 datasetId,
                                   SyncDownJob& job_out);


    struct SharedFilesDbEntry {
        u64 compId;
        std::string name;
        u64 datasetId;
        u64 feature;
        bool b_thumb_downloaded;
        std::string recipientList;  // comma separated e-mails
        u64 revision;
        u64 size;
        bool lastChangedExists;
        VPLTime_t lastChanged;
        bool createDateExists;
        VPLTime_t createDate;
        bool updateDeviceExists;
        u64 updateDevice;
        bool opaqueMetadataExists;
        std::string opaqueMetadata;
        bool relThumbPathExists;
        std::string relThumbPath;  // Path relative to getS[bw]mDownloadDir

        void clear() {
            compId = 0;
            name.clear();
            datasetId = 0;
            feature = 0;
            b_thumb_downloaded = false;
            recipientList.clear();
            revision = 0;
            size = 0;
            lastChangedExists = false;
            lastChanged = 0;
            createDateExists = false;
            createDate = 0;
            updateDeviceExists = false;
            updateDevice = 0;
            opaqueMetadataExists = false;
            opaqueMetadata.clear();
            relThumbPathExists = false;
            relThumbPath.clear();
        }
        SharedFilesDbEntry() { clear(); }
    };
    // Shared Files
    int sharedFilesItemGet(u64 datasetId, u64 compId,
                           SharedFilesDbEntry& entry_out);
    int sharedFilesItemAdd(const SharedFilesDbEntry& entry);
    int sharedFilesUpdateThumbDownloaded(u64 compId,
                                         u64 feature,
                                         bool thumbDownloaded);
    int sharedFilesEntryGetByCompId(u64 compId,
                                    u64 syncFeature,
                                    u64& revision_out,
                                    u64& datasetId_out,
                                    std::string& name_out,
                                    bool& thumbDownloaded_out,
                                    std::string& thumbRelPath_out);
    int sharedFilesEntryDelete(u64 compId, u64 datasetId);
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

    int migrate_schema_from_v1_to_v2();
    int migrate_schema_from_v2_to_v3();
    int migrate_schema_from_v1_to_v3();

    int getAdminValue(u64 adminId, /*OUT*/ u64 &value);

    int setDatasetVersion(u64 datasetid, u64 version);
    int getDatasetVersion(u64 datasetid, /*OUT*/ u64 &version);
    int removeDatasetVersion(u64 datasetId);
    int removeAllDatasetVersions();

    int addJob(u64 did, const std::string &cpath, u64 cid,
               const std::string &lpath, 
               u64 timestamp, u64 syncfeature, u64 rev, /*OUT*/ u64 &jobid);
    int removeJob(u64 jobid);
    int removeJobsByDatasetId(u64 datasetId);
    int removeAllJobs();

    int findJob(u64 jobid, /*OUT*/ SyncDownJob &job);
    int findJob(const std::string &localpath, /*OUT*/ SyncDownJob &job);
    int findJobDownloadNotDoneNoRecentDispatch(u64 cutoff, /*OUT*/ SyncDownJob &job);
    int findJobCopybackNotDoneNoRecentDispatch(u64 cutoff, /*OUT*/ SyncDownJob &job);
    int findJobEx(u64 jobid, /*OUT*/ SyncDownJobEx &job);

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

    //Delete Jobs
    int addDeleteJob(u64 compId, const std::string &name,
               u64 timestamp, /*OUT*/ u64 &jobid);
    int setJobDeleteDispatchTime(u64 jobid, u64 timestamp);
    int setJobDeleteTryTime(u64 jobid, u64 timestamp);
    int setJobDeleteDoneTime(u64 jobid, u64 timestamp);
    int setJobDeleteFailedInfo(u64 jobid,std::string &lpath);
    int incJobDeleteFailedCount(u64 jobid);
    int findJobDeleteNotDoneNoRecentDispatch(u64 cutoff, /*OUT*/ DeleteJob &job);
    int findJobDelete(u64 compId, /*OUT*/ DeleteJob &job);
    int removeJobDelete(u64 jobid);
    int removeAllJobDelete();

    //Picstream
    int addPicStreamItem(u64 compId,
                         const std::string &name,
                         u64 rev,
                         u64 origCompId,
                         u64 origDev,
                         const std::string &identifier,
                         const std::string &title,
                         const std::string &albumName,
                         u64 fileType,
                         u64 fileSize,
                         u64 takenTime,
                         const std::string &lpath,
                         u64 &jobid_out);
    int findPicStreamItem(u64 compId, /*OUT*/ PicStreamItem &item);
    int findPicStreamItem(const std::string &name, /*OUT*/ PicStreamItem &item);
    int findPicStreamItems(u64 fileType, std::string albumName, std::queue<PicStreamItem> &items);
    int findPicStreamItems(u64 fullCompId, std::queue<PicStreamItem> &items);
    int removePicStreamItems(u64 fullCompId);
    int removePicStreamItem(u64 CompId);

    int getPicStreamItems(const std::string& searchField,
                          const std::string& sortOrder,
                          google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output);

    int getPicStreamAlbums(const std::string& searchField,
                           const std::string& sortOrder,
                           google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output);

    // update the local path in PicStream table.
    int updateLocalPath(const std::string& name, const std::string& localPath);

    int sharedFiles_readRow(sqlite3_stmt* stmt,
                            SharedFilesDbEntry& row_out);
};

// Abstraction of SyncDown destination storage.
// For CloudNode, it maps to the "Virt Drive" dataset.
// For others, it maps to the local disk.
class SyncDownDstStor {
public:
    SyncDownDstStor();
    ~SyncDownDstStor();
    int init(u64 userId);
    bool fileExists(const std::string &path);
    int createMissingAncestorDirs(const std::string &path);
    int copyFile(const std::string &srcPath, const std::string &dstPath);
#ifdef CLOUDNODE
    int getSiblingComponentCount(const std::string path, int& count_out);
#endif // CLOUDNODE
private:
#ifdef CLOUDNODE
    u64 dstDatasetId;
    dataset *dstDataset;

    int getVirtDriveDatasetId(u64 userId);
#endif // CLOUDNODE
};

class SyncDown {
public:
    // assumption: workdir ends with '/'
    SyncDown(const std::string &workdir);
    ~SyncDown();

    // start processing
    int Start(u64 userId);

    // stop processing
    int Stop(bool purge);

    // Report dataset change.
    int NotifyDatasetContentChange(u64 datasetid, bool overrideAnsCheck, bool syncDownAdded_, int syncDownFileType_);

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

    int QueryPicStreamItems(ccd::PicStream_DBFilterType_t filter_type,
                                const std::string& searchField,
                                const std::string& sortOrder,
                                google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output);
    int QuerySharedFilesItems(ccd::SyncFeature_t syncFeature,
                              const std::string& searchField,
                              const std::string& sortOrder,
                              google::protobuf::RepeatedPtrField<ccd::SharedFilesQueryObject> &output);
    int QuerySharedFilesEntryByCompId(u64 compId,
                                      u64 syncFeature,
                                      u64& revision_out,
                                      u64& datasetId_out,
                                      std::string& name_out,
                                      bool& thumbDownloaded_out,
                                      std::string& thumbRelPath_out);
    int QueryPicStreamItems(const std::string& searchField,
                            const std::string& sortOrder,
                            std::queue<PicStreamPhotoSet> &output);
    int AddPicStreamItem(u64 compId,
                         const std::string &name,
                         u64 rev,
                         u64 origCompId,
                         u64 originDev,
                         const std::string &identifier,
                         const std::string &title,
                         const std::string &albumName,
                         u64 fileType,
                         u64 fileSize,
                         u64 takenTime,
                         const std::string &lpath,
                         u64 &jobid_out,
                         bool &syncUpAdded_out);

    int DownloadOnDemand(SyncDownJob &job, HttpStream *hs);
    void setEnableGlobalDelete(bool enable = true);
    bool getEnableGlobalDelete();
private:
    VPLMutex_t mutex;
    VPLCond_t cond;

    std::string workdir;
    std::string tmpdir;

    u64 userId;
    u64 cutoff_ts;

    SyncDownJobs *jobs;

    bool thread_spawned;
    bool thread_running;
    bool exit_thread;
    bool syncDownAdded;
    int syncDownFileType;

    bool overrideAnsCheck;
    bool isDoingWork;
    std::map<int, int> currentSyncStatus; // <syncFeature, syncStatus>
    std::set<u64> dataset_changes_set;
    std::queue<u64> dataset_changes_queue;

    std::map<u64, VPLHttp2*> httpInProgress;

    int spawnDispatcherThread();
    int signalDispatcherThreadToStop();

    static VPLTHREAD_FN_DECL dispatcherThreadMain(void *arg);
    void dispatcherThreadMain();

    int getChanges(u64 datasetid, u64 changeSince);
    int addSyncDownJobs(u64 datasetid, int fileType);
    int applyChanges();
    int applyDeletions(u64 userId, u64 datasetId);
    int getJobDownloadReady(u64 threshold, SyncDownJob &job);
    int getJobCopybackReady(u64 threshold, SyncDownJob &job);
    int tryDownload(SyncDownJob &job);
    int tryCopyback(SyncDownJob &job);
    int getLatestRevNum(SyncDownJob &job, /*OUT*/ u64 &revision);
    int downloadRevision(SyncDownJob &job, u64 revision, HttpStream *hs = NULL);
    int downloadPreview(SyncDownJob &job, u64 revision, u64 feature);
    int getLatestMetadata(const std::string& name,
                          u64 compId,
                          u64 datasetId,
                          u64 &origDev_out,
                          u64 &rev_out,
                          u64 &fileSize_out,
                          std::string &dlUrl_out,
                          std::string &previewUrl_out);
    int handleSwmAndSbmDelete(u64 compId,
                              u64 datasetId,
                              u64 syncFeature,
                              const std::string& localPathAndThumbName);

    int getDatasetInfo(u64 datasetId,
                       bool& isPicstream_out,
                       bool& isSharedByMe_out,
                       bool& isSharedWithMe_out);
    int setSyncFeatureState(ccd::SyncFeature_t feature, 
                            ccd::FeatureSyncStateType_t state, 
                            u32 &pendingJobCounts, 
                            u32 &failedJobCounts,
                            bool checkStateChange);

    enum PicStreamDownloadDirEnum {
        PicStreamDownloadDirEnum_FullRes,
        PicStreamDownloadDirEnum_LowRes,
        PicStreamDownloadDirEnum_Thumbnail,
    };
    int getPicStreamDownloadDir(PicStreamDownloadDirEnum downloadDirType,
                                std::string &downloadDir,
                                u32& maxFiles,
                                u32& maxBytes,
                                u32& preserveFreeDiskPercentage,
                                u64& preserveFreeDiskSizeBytes);
    int getSwmDownloadDir(std::string &swmDownloadDir_out);
    int getSbmDownloadDir(std::string &sbmDownloadDir_out);

    // string directory, int numNewPhotos
    typedef std::map<std::string, int> PicstreamDirToNumNewMap;

    // Enforces picstream thresholds (max files, max bytes) for a directory.
    // WILL delete files that are not recognized as photos to clear up room.
    // Removes any picstreamDirs from the map where the thresholds were enforced.
    void enforcePicstreamThresh(PicstreamDirToNumNewMap& picstreamDirs);

    bool globalDeleteEnabled;
};

#endif // __cplusplus

#endif // __SYNCDOWN_PRIVATE_HPP__
