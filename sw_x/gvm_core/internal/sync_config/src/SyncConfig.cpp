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

#include "SyncConfig.hpp"

#include "SyncConfigDb.hpp"
#include "SyncConfigThreadPool.hpp"
#include "SyncConfigUtil.hpp"
#include "gvm_errors.h"
#include "gvm_file_utils.hpp"
#include "gvm_rm_utils.hpp"
#include "gvm_thread_utils.h"
#include "log.h"
#include "scopeguard.hpp"
#include "vcs_util.hpp"
#include "vpl_fs.h"
#include "vpl_string.h"
#include "vpl_th.h"
#include "vplex_assert.h"
#include "vplex_file.h"
#include "vplex_time.h"
#include "vplu_mutex_autolock.hpp"
#include "vplu_sstr.hpp"
#include "md5.h"

#if VPLFS_MONITOR_SUPPORTED
#include "FileMonitorDelayQ.hpp"
#endif

#include <deque>
#include <set>
#include <sstream>

using namespace std;

const u32 DEFAULT_MYSQL_DATE = 1;  // mysql cannot store 0 date value

#if VPLFS_MONITOR_SUPPORTED
/// s_fileMonitorDelayQ is static because it makes it easy to share a single delayer
/// thread across all SyncConfigs.  If we want this to be non-static, we'll need to
/// increase the complexity of the delayer thread or add an additional
/// thread per SyncConfig.
#  ifdef ANDROID
static FileMonitorDelayQ s_fileMonitorDelayQ(VPLTime_FromSec(2));
#  else  // TODO: We can reduce this delay for filesystems that provide better granularity, but 2 seconds is safest.
static FileMonitorDelayQ s_fileMonitorDelayQ(VPLTime_FromSec(2));
#  endif
#endif

// TODO:
// - Cleanup temporary downloads at some point?
// - What is responsible for creating .sync_temp and making it hidden?
// - Two-way sync: download deletion phase
// - Two-way sync: error handling
// - Two-way sync: conflicts

namespace {

/// Path relative to sync config root.
/// For storing entries within the local DB.
/// Can be empty to indicate the sync config root.
/// @invariant No leading or trailing slash.
class SCRelPath
{
public:
    SCRelPath(const string& path) : path(path) { checkInvariants(); }
    inline void set(const string& path) {
        this->path = path;
        checkInvariants();
    }
    inline void checkInvariants() {
        if (path.size() > 0) {
            ASSERT_NOT_EQUAL(path[0], '/', "%c");
            ASSERT_NOT_EQUAL(path[path.size()-1], '/', "%c");
        }
    }
    inline const string& str() const { return path; }
    inline const char* c_str() const { return path.c_str(); }
    SCRelPath getChild(const string& entryName) const {
        ASSERT(entryName.size() != 0);
        if(path.size() == 0) {
            return SCRelPath(entryName);
        }else{
            return SCRelPath(path + "/" + entryName);
        }
    }
    SCRelPath getParent() const {
        size_t index = path.rfind('/');
        if (index == string::npos) {
            return SCRelPath("");
        } else {
            return SCRelPath(path.substr(0, index));
        }
    }
    string getName() const {
        size_t index = path.rfind('/');
        if (index == string::npos) {
            return path;
        } else {
            return path.substr(index + 1);
        }
    }
    bool isSCRoot() const { return path.size() == 0; }
private:
    string path;
};

/// Path relative to dataset root.
/// For use when dealing with VCS APIs.
/// Can be empty to indicate the dataset root.
/// @invariant No leading or trailing slash.
class DatasetRelPath
{
public:
    DatasetRelPath(const string& path) : path(path) { checkInvariants(); }
    inline void set(const string& path) {
        this->path = path;
        checkInvariants();
    }
    inline void checkInvariants() {
        if (path.size() > 0) {
            ASSERT(path[0] != '/');
            ASSERT(path[path.size()-1] != '/');
        }
    }
    inline const string& str() const { return path; }
    inline const char* c_str() const { return path.c_str(); }
    DatasetRelPath getDescendent(const string& relPath) const {
        DatasetRelPath result("");
        if (relPath.size() == 0) {
            result.set(path);
        } else if(path.size()==0) {
            result.set(relPath);
        }else {
            result.set(path + "/" + relPath);
        }
        return result;
    }
private:
    string path;
};

/// Absolute path on local filesystem.
/// For use when dealing with VPL file APIs (ls a directory, stat a file, read/write a file).
/// @invariant No trailing slash.  (Leading slash is allowed.)
/// TODO: special case for the root itself ("/")?  Right now, it isn't allowed.
class AbsPath
{
public:
    AbsPath(const string& path) : path(path) { checkInvariants(); }
    inline void set(const string& path) {
        this->path = path;
        checkInvariants();
    }
    inline void checkInvariants() {
        if (path.size() > 0) {
            ASSERT(path[path.size()-1] != '/');
        }
    }
    inline const string& str() const { return path; }
    inline const char* c_str() const { return path.c_str(); }
    AbsPath appendRelPath(const SCRelPath& relativePath) const {
        if (relativePath.str().size() == 0) { // Special case if syncConfigRelPath is the sync config root.
            return AbsPath(path);
        } else {
            return AbsPath(path + "/" + relativePath.str());
        }
    }
private:
    string path;
};

}  // anonymous namespace

static const u32 NUM_IMMEDIATE_TRANSIENT_RETRY = 3;
/// Immediate retry interval for transient errors.
static const VPLTime_t QUICK_RETRY_INTERVAL = VPLTime_FromSec(10);
/// How long to wait before beginning a VCS scan after encountering an error that implies that
/// we need to do a VCS scan.
static const VPLTime_t VCS_RESCAN_DELAY = VPLTime_FromMinutes(1);
/// When to abandon errors.
static const u32 ERROR_COUNT_LIMIT = 500;

static const u32 VCS_GET_DIR_PAGE_SIZE = 500;
static const VPLTime_t FS_TIMESTAMP_GRANULARITY = VPLTime_FromSec(2);


//-------------------------------------------------
//---------- DB Transaction Optimization ----------

//  Warning: These DB batching macros only apply to the localDb sqlite DB handle
//           in the main syncConfig worker thread.  Does not support batching of
//           other handles in other threads, for example, deferredUploadThread.
#define BEGIN_TRANSACTION()                                                 \
    do {                                                                    \
        int trans_rc = beginDbTransaction();                                \
        if(trans_rc != 0) {                                                 \
            LOG_ERROR("beginDbTransaction:%d", trans_rc);                   \
        }                                                                   \
    } while(false)

#define CHECK_END_TRANSACTION(b_force)                                      \
    do {                                                                    \
        int trans_rc = commitDbTransaction(0,                               \
                                            b_force);                       \
        if(trans_rc != 0) {                                                 \
            LOG_ERROR("commitDbTransaction:%d", trans_rc);                  \
        }                                                                   \
    } while(false)

#define CHECK_END_TRANSACTION_BYTES(numBytes, b_force)                      \
    do {                                                                    \
        int trans_rc = commitDbTransaction(numBytes,                        \
                                            b_force);                       \
        if(trans_rc != 0) {                                                 \
            LOG_ERROR("commitDbTransaction:%d", trans_rc);                  \
        }                                                                   \
    } while(false)

struct ThreadTransactionState {
    bool b_inTransaction;
    u32 numTransactionChanges;
    u64 numTransactionBytes;

    // NOTE: MSVC does not allow a constructor for thread local objects.

    void reset() {
        b_inTransaction = false;
        numTransactionChanges = 0;
        numTransactionBytes = 0;
    }
};

#ifdef _MSC_VER
static VPL_THREAD_LOCAL ThreadTransactionState tl_transactionCounter = {false, 0, 0};
#else
#include <pthread.h>

static pthread_key_t threadLocalTransactionStateKey;
static pthread_once_t threadLocalTransactionStateKey_once = PTHREAD_ONCE_INIT;

static void destructThreadTransactionState(void* ptr)
{
    delete((ThreadTransactionState*)ptr);
}

static void privCreateTransactionCounterGlobalKey()
{
    int rc = pthread_key_create(&threadLocalTransactionStateKey, destructThreadTransactionState);
    if (rc != 0) {
        LOG_CRITICAL("pthread_key_create failed: %d", rc);
    }
}
#endif

/// Must be called before the first SyncConfig is created.  Can safely be called multiple times.
static void ensureThreadLocalsInit()
{
#ifdef _MSC_VER
    // Nothing to do.
#else
    int rc = pthread_once(&threadLocalTransactionStateKey_once, privCreateTransactionCounterGlobalKey);
    if (rc != 0) {
        LOG_CRITICAL("pthread_once failed: %d", rc);
    }
#endif
}

/// Returns the current thread's ThreadTransactionState.
static ThreadTransactionState* getThreadTransactionState()
{
#ifdef _MSC_VER
    return &tl_transactionCounter;
#else
    // Note: we rely on #CreateSyncConfig() to call #ensureThreadLocalsInit().

    ThreadTransactionState* ptr = (ThreadTransactionState*)pthread_getspecific(threadLocalTransactionStateKey);
    if (ptr == NULL) {
        ptr = new ThreadTransactionState();
        ptr->reset();
        int rc = pthread_setspecific(threadLocalTransactionStateKey, ptr);
        if (rc != 0) {
            LOG_CRITICAL("pthread_setspecific failed: %d", rc);
        }
    }
    return ptr;
#endif
}
//---------- DB Transaction Optimization ----------
//-------------------------------------------------

// TODO: bug 14854: is it really worth malloc'ing for small fixed size blocks (2*4KB)?
// An error from stat doesn't imply that the file doesn't exist (it might exist, but not be readable).
// If it exists, but isn't readable, it's probably not safe to conclude anything about its contents.
// Wrong log level: it's not an error if the file sizes don't match.
/// Returns true if the file contents are equal, else returns false;
static bool isFileContentsEqual(const AbsPath& file1, const AbsPath& file2)
{
    VPLFS_stat_t statFile1;
    VPLFS_stat_t statFile2;
    int rc;
    rc = VPLFS_Stat(file1.c_str(), &statFile1);
    if(rc != VPL_OK) {
        LOG_ERROR("file1 does not exist:%d, %s", rc, file1.c_str());
        return false;
    }
    if(statFile1.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("file1 not file:%d, %s", (int)statFile1.type, file1.c_str());
        return false;
    }

    rc = VPLFS_Stat(file2.c_str(), &statFile2);
    if(rc != VPL_OK) {
        LOG_ERROR("file2 does not exist:%d, %s", rc, file2.c_str());
        return false;
    }
    if(statFile2.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("file2 not file:%d, %s", (int)statFile2.type, file2.c_str());
        return false;
    }

    if(statFile1.size != statFile2.size) {
        LOG_ERROR("file1Size:"FMTu64" != file2Size:"FMTu64", file1:%s, file2:%s",
                (u64)statFile1.size, (u64)statFile2.size, file1.c_str(), file2.c_str());
        return false;
    }

    bool toReturn = false;
    VPLFile_handle_t fh1 = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t fh2 = VPLFILE_INVALID_HANDLE;
    const int CHUNK_TEMP_BUFFER_SIZE = 4096;
    char* chunkBuf1 = NULL;
    char* chunkBuf2 = NULL;
    chunkBuf1 = (char*) malloc(CHUNK_TEMP_BUFFER_SIZE);
    if(!chunkBuf1) {
        LOG_ERROR("Out of memory: %d", CHUNK_TEMP_BUFFER_SIZE);
        goto exit;
    }
    chunkBuf2 = (char*) malloc(CHUNK_TEMP_BUFFER_SIZE);
    if(!chunkBuf2) {
        LOG_ERROR("Out of memory: %d", CHUNK_TEMP_BUFFER_SIZE);
        goto exit;
    }

    fh1 = VPLFile_Open(file1.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fh1)) {
        LOG_ERROR("Fail to open file %s", file1.c_str());
        goto exit;
    }

    fh2 = VPLFile_Open(file2.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fh2)) {
        LOG_ERROR("Fail to open file %s", file2.c_str());
        goto exit;
    }

    {  // Compare in chunks
       for (u64 bytesCompared = 0; bytesCompared < statFile1.size;) {
           ssize_t bytesReadF1 = VPLFile_Read(fh1,
                   chunkBuf1,
                   CHUNK_TEMP_BUFFER_SIZE);
           ssize_t bytesReadF2 = VPLFile_Read(fh2,
                   chunkBuf2,
                   CHUNK_TEMP_BUFFER_SIZE);
           if(bytesReadF1 != bytesReadF2) {
               LOG_INFO("Conflict?: Bytes read diff: file1:%d, %s, and file2:%d, %s",
                       (int)bytesReadF1, file1.c_str(),
                       (int)bytesReadF2, file2.c_str());
               goto exit;
           }
           if(bytesReadF1 > 0) {
               if(memcmp(chunkBuf1, chunkBuf2, bytesReadF1) != 0) {
                   LOG_INFO("Conflict?: Byte diff between file1:%s and file2:%s at offset "FMTu64,
                           file1.c_str(), file2.c_str(), bytesCompared);
                   goto exit;
               }
               bytesCompared += bytesReadF1;
           }else{
               break;
           }
       }
    }

    // Files are identical
    toReturn = true;

exit:
    if(chunkBuf1){ free(chunkBuf1); }
    if(chunkBuf2){ free(chunkBuf2); }
    if (VPLFile_IsValidHandle(fh1)) { VPLFile_Close(fh1); }
    if (VPLFile_IsValidHandle(fh2)) { VPLFile_Close(fh2); }
    return toReturn;
}

class SyncConfigImpl : public SyncConfig
{
protected:
    VPL_DISABLE_COPY_AND_ASSIGN(SyncConfigImpl);
    
    u64 user_id;

    VcsDataset dataset; // Safe to access without holding mutex, since modification is not allowed once the SyncConfig is created.

    SyncType type;
    SyncPolicy sync_policy;
    /// Absolute path of the sync config root on the local filesystem, no trailing slash.
    AbsPath local_dir;
    AbsPath local_dir_ci; // for case insensitive
    /// Dataset-relative path of the sync config root, no leading or trailing slashes.
    DatasetRelPath server_dir;
    DatasetAccessInfo dataset_access_info; // Safe to access without holding mutex, since modification is not allowed once the SyncConfig is created.
    SyncConfigEventCallback event_cb;
    void* callback_context;

    /// When false, we will pause the worker thread at the next convenient moment (after finishing
    /// the current HTTP call / upload / download).
    bool allowed_to_run;
    
    /// When not NULL, this means there's a caller blocked waiting for the call
    /// Pause(true) to complete.
    VPLSem_t* worker_loop_wait_for_pause_sem;
    /// True when SyncConfig workerLoop is paused or stopped.  Resume() would need to be
    /// called to get the SyncConfig out of this state.
    bool worker_loop_paused_or_stopped;

    /// When not NULL, this means there's a caller blocked waiting for the call
    /// Pause(true) to complete.
    VPLSem_t* deferred_upload_loop_wait_for_pause_sem;
    /// True when SyncConfig deferred_upload_loop is paused or stopped.  Resume() would need to be
    /// called to get the SyncConfig out of this state.
    bool deferred_upload_loop_paused_or_stopped;

    /// When true, we will stop the worker thread as soon as possible.  This includes canceling
    /// any current HTTP call / upload / download.
    bool stop_thread;

    /// (Only applies to OneWayUp and TwoWay sync.)
    /// Note: This will cause a new scan to begin as soon as the worker loop completes its current
    ///   "Scan + ApplyChanges" iteration.
    bool full_local_scan_requested;

    /// (Only applies to OneWayDown and TwoWay sync.)
    /// Note: This will cause a new scan to begin as soon as the worker loop completes its current
    ///   "Scan + ApplyChanges" iteration.
    bool download_scan_requested;

    /// Variable to track whether the archive storage device is available.
    /// VCSArchiveAccess is within DatasetAccessInfo struct.
    bool is_archive_storage_device_available;

    /// (Only applies to OneWayDown and TwoWay sync.)
    /// If there isn't already a download scan pending, will check that the dataset version number
    /// in VCS has changed from last_seen_dataset_version before requesting VCS GET dir.
    /// Note: This will cause a new scan to begin as soon as the worker loop completes its current
    ///   "Scan + ApplyChanges" iteration.
    bool dataset_version_check_requested;

    /// (Only applies to OneWayDown and TwoWay sync.)
    /// Similar to dataset_version_check_requested, but this will *not* cause a new scan to begin
    ///   as soon as the worker loop completes its current "Scan + ApplyChanges" iteration.
    ///   It will be processed when the error timeout expires (at next_error_processing_timestamp)
    ///   or when a local or remote scan is requested.
    bool dataset_version_check_error;

    /// (Only applies to OneWayUp and TwoWay sync) When true, we will do a local scan.
    /// Similar to full_local_scan_requested, but this will *not* cause a new scan to begin as soon
    ///   as the worker loop completes its current "Scan + ApplyChanges" iteration.
    ///   It will be processed when the error timeout expires (at next_error_processing_timestamp)
    ///   or when a local or remote scan is requested.
    bool uploadScanError;

    /// (Only applies to OneWayDown and TwoWay sync.)  When true, we will do a remote scan.
    /// Similar to download_scan_requested, but this will *not* cause a new scan to begin as soon
    ///   as the worker loop completes its current "Scan + ApplyChanges" iteration.
    ///   It will be processed when the error timeout expires (at next_error_processing_timestamp)
    ///   or when a local or remote scan is requested.
    bool downloadScanError;

    VPLTime_t next_error_processing_timestamp;

    /// If this timestamp is set, it takes precedence over next_error_processing_timestamp,
    /// but only if next_error_processing_timestamp is set.
    VPLTime_t force_error_retry_timestamp;

    /// Keeps track of the latest VCS version of our dataset.
    /// This is an optimization to minimize calls to VCS "GET dir", which is relatively expensive
    /// compared to checking the dataset version.
    // TODO: This would probably be better stored in the localDB (to avoid calling VCS "GET dir"
    //   once per process restart when the dataset didn't actually change).
    //   However, we will also need to deal with the following problem first:
    //   If checkDatasetVersionChanged succeeds, but then enqueueRootToNeedDownloadScan or
    //   VCS GET dir fails, we rely on having downloadScanError=true to skip the
    //   checkDatasetVersionChanged check next time (since checkDatasetVersionChanged already
    //   updated last_seen_dataset_version).  However, downloadScanError=true gets dropped
    //   when the SyncConfig is destroyed.  So if last_seen_dataset_version gets stored in
    //   the localDB and CCD is restarted, we could get into a situation where CCD wouldn't
    //   call VCS GET dir (until another dataset change is made), even though there are
    //   changes in VCS that haven't been downloaded yet.
    //   A simple (but inefficient) fix would be to have init() call ReportRemoteChange()
    //   instead of ReportPossibleRemoteChange(), but that defeats the purpose of storing this in
    //   the DB anyway (since it would result in us always calling VCS "GET dir" when the process
    //   restarts).
    //   Perhaps we should store downloadScanError in the DB as well?
    u64 last_seen_dataset_version;

    std::vector<VPLHttp2*> http_handles_to_cancel;
    std::vector<VCSArchiveOperation*> archive_operations_for_async_cancel;
    std::vector<VCSFileUrlOperation*> file_url_operation_for_async_cancel;
    bool verbose_http_log;

    // workerLoopStatus needed to track status separately from the
    // status in deferUploadLoop
    SyncConfigStatus workerLoopStatus;
    bool workerLoopStatusHasError;
    bool workerLoopStatusHasWorkToDo;
    u32 workerUploadsRemaining;
    u32 workerDownloadsRemaining;

    bool deferUploadLoopStatusHasError;
    bool deferUploadLoopStatusHasWorkToDo;

    // Previous aggregate (workerLoop and deferUploadLoop) status.
    SyncConfigStatus prevSyncConfigStatus;
    bool prevHasError;
    bool prevHasWorkToDo;
    u32 prevUploadsRemaining;
    u32 prevDownloadsRemaining;
    bool prevRemoteScanPending;

    VPLMutex_t mutex;
    /// Condition: there is work for the SyncConfig to do (applies to either the workerLoop
    ///     or the deferredUploadLoop, or possibly both).
    VPLCond_t work_to_do_cond_var;
    VPLDetachableThreadHandle_t worker_thread;
    VcsSession vcs_session; // Safe to access without holding mutex, since modification is not allowed once the SyncConfig is created.
    SyncConfigDb localDb;

    VPLMutex_t lookupDbMutex;  // lookupDb has same life-cycle as syncConfig.
    bool lookupDbInit;
    SyncConfigDb lookupDb;

    VPLMutex_t getStateDbMutex;  // getStateDb has same life-cycle as syncConfig.
    bool getStateDbInit;
    SyncConfigDb getStateDb;

    VPLMutex_t lightQueryDbMutex;  // lightQueryDb has same life-cycle as syncConfig. For threads which NOT query database frequently.
    bool lightQueryDbInit;
    SyncConfigDb lightQueryDb;

    /// These fields only apply to OneWayUp.
    //@{
    bool deferredUploadDbInit;
    SyncConfigDb deferredUploadDb;

    /// True when deferredUploadThread exists and is joinable.
    /// Whichever thread sets this to false promises to Join() or Detach() deferredUploadThread.
    bool deferredUploadThreadInit;

    VPLDetachableThreadHandle_t deferredUploadThread;
    bool deferredUploadThreadPerformUpload;
    bool deferredUploadThreadPerformError;
    //@}

    /// For parallel task support
    SyncConfigThreadPool* threadPool;
    bool hasDedicatedThread;
    VPLSem_t threadPoolNotifier;

    /// (Only applies to OneWayUp and TwoWay sync.)
    /// A more fine-grained version of full_local_scan_requested.
    set<string> incrementalLocalScanPaths;
#if VPLFS_MONITOR_SUPPORTED
    bool fileMonitorHandleSet;
    VPLFS_MonitorHandle fileMonitorHandle;
#endif

    /// (Only applies to OneWayUp Sync)
    /// - For OneWayUp Sync, this is the only way to do a VCS scan.  It will scan all the components
    ///   on VCS, populate the localDB with any components that appear to be the same (in VCS and on
    ///   the local filesystem), and delete any components from VCS that are in VCS but not on the
    ///   local filesystem.  We will then do a local scan.  (As such, there is no value in setting
    ///   uploadScanError to true when initial_scan_requested_OneWay is true.)
    /// .
    /// Note: Setting this to true will *not* cause a new scan to begin as soon as the worker loop
    ///   completes its current "Scan + ApplyChanges" iteration.
    ///   It will be processed when the error timeout expires (at next_error_processing_timestamp)
    ///   or when a local scan is requested.
    bool initial_scan_requested_OneWay;

public:
    SyncConfigImpl(
            u64 user_id,
            const VcsDataset& dataset,
            SyncType type,
            const SyncPolicy& sync_policy,
            const std::string& local_dir,
            const std::string& server_dir,
            const DatasetAccessInfo& dataset_access_info,
            SyncConfigThreadPool* thread_pool,
            bool make_dedicated_thread,
            SyncConfigEventCallback event_cb,
            void* callback_context)
    :   user_id(user_id),
        dataset(dataset),
        type(type),
        sync_policy(sync_policy),
        local_dir(Util_trimTrailingSlashes(local_dir)),
        local_dir_ci(Util_UTF8Upper(Util_trimTrailingSlashes(local_dir).c_str())),
        server_dir(Util_trimSlashes(server_dir)),
        dataset_access_info(dataset_access_info),
        event_cb(event_cb),
        callback_context(callback_context),
        allowed_to_run(false),
        worker_loop_wait_for_pause_sem(NULL),
        worker_loop_paused_or_stopped(true),
        deferred_upload_loop_wait_for_pause_sem(NULL),
        deferred_upload_loop_paused_or_stopped(true),
        stop_thread(false),
        full_local_scan_requested(false),
        download_scan_requested(false),
        is_archive_storage_device_available(false),
        dataset_version_check_requested(false),
        dataset_version_check_error(false),
        uploadScanError(false),
        downloadScanError(false),
        next_error_processing_timestamp(VPLTIME_INVALID),
        force_error_retry_timestamp(VPLTIME_INVALID),
        last_seen_dataset_version(0),
        verbose_http_log(false),
        workerLoopStatus(SYNC_CONFIG_STATUS_DONE),
        workerLoopStatusHasError(false),
        workerLoopStatusHasWorkToDo(false),
        workerUploadsRemaining(0),
        workerDownloadsRemaining(0),
        deferUploadLoopStatusHasError(false),
        deferUploadLoopStatusHasWorkToDo(false),
        prevSyncConfigStatus(SYNC_CONFIG_STATUS_DONE),
        prevHasError(false),
        prevHasWorkToDo(false),
        prevUploadsRemaining(0),
        prevDownloadsRemaining(0),
        prevRemoteScanPending(false),
        vcs_session(user_id,
                    dataset_access_info.deviceId,
                    dataset_access_info.urlPrefix,
                    dataset_access_info.sessionHandle,
                    dataset_access_info.serviceTicket),
        localDb(local_dir + "/" + SYNC_CONFIG_DB_DIR,
                getSyncDbFamily(type),
                user_id, dataset.id, SYNC_CONFIG_ID_STRING,
                sync_policy.case_insensitive_compare),
        lookupDbInit(false),
        lookupDb(local_dir + "/" + SYNC_CONFIG_DB_DIR,
                 getSyncDbFamily(type),
                 user_id, dataset.id, SYNC_CONFIG_ID_STRING,
                 sync_policy.case_insensitive_compare),
        getStateDbInit(false),
        getStateDb(local_dir + "/" + SYNC_CONFIG_DB_DIR,
                 getSyncDbFamily(type),
                 user_id, dataset.id, SYNC_CONFIG_ID_STRING,
                 sync_policy.case_insensitive_compare),
        lightQueryDbInit(false),
        lightQueryDb(local_dir + "/" + SYNC_CONFIG_DB_DIR,
                 getSyncDbFamily(type),
                 user_id, dataset.id, SYNC_CONFIG_ID_STRING,
                 sync_policy.case_insensitive_compare),
        deferredUploadDbInit(false),
        deferredUploadDb(local_dir + "/" + SYNC_CONFIG_DB_DIR,
                         getSyncDbFamily(type),
                         user_id, dataset.id, SYNC_CONFIG_ID_STRING,
                         sync_policy.case_insensitive_compare),
        deferredUploadThreadInit(false),
        deferredUploadThreadPerformUpload(false),
        deferredUploadThreadPerformError(false),
        threadPool(thread_pool),
        hasDedicatedThread(make_dedicated_thread),
#if VPLFS_MONITOR_SUPPORTED
        fileMonitorHandleSet(false),
#endif
        initial_scan_requested_OneWay(false)
    {
        VPL_SET_UNINITIALIZED(&mutex);
        VPL_SET_UNINITIALIZED(&work_to_do_cond_var);
        VPL_SET_UNINITIALIZED(&lookupDbMutex);
        VPL_SET_UNINITIALIZED(&getStateDbMutex);
        VPL_SET_UNINITIALIZED(&lightQueryDbMutex);
        VPL_SET_UNINITIALIZED(&threadPoolNotifier);
    }

    int init(bool allowCreateDB)
    {
        int rv;

        rv = VPLMutex_Init(&mutex);
        if (rv < 0) {
            LOG_WARN("%p: VPLMutex_Init failed: %d", this, rv);
            return rv;
        }
        rv = VPLCond_Init(&work_to_do_cond_var);
        if (rv < 0) {
            LOG_WARN("%p: VPLCond_Init failed: %d", this, rv);
            return rv;
        }
        rv = VPLMutex_Init(&lookupDbMutex);
        if (rv < 0) {
            LOG_WARN("%p: VPLMutex_Init:%d", this, rv);
            return rv;
        }
        rv = VPLMutex_Init(&getStateDbMutex);
        if (rv < 0) {
            LOG_WARN("%p: VPLMutex_Init:%d", this, rv);
            return rv;
        }
        rv = VPLMutex_Init(&lightQueryDbMutex);
        if (rv < 0) {
            LOG_WARN("%p: VPLMutex_Init:%d", this, rv);
            return rv;
        }
        if (threadPool != NULL) {
            rv = VPLSem_Init(&threadPoolNotifier, 1, 0);
            if (rv < 0) {
                LOG_WARN("%p: VPLSem_Init:%d", this, rv);
                return rv;
            }
            if (hasDedicatedThread) {
                rv = threadPool->RegisterForTaskCompleteNotificationAndAddDedicatedThread(
                        &threadPoolNotifier);
                if (rv != 0) {
                    LOG_ERROR("%p:RegisterForTaskCompleteNotificationAndAddDedicatedThread:%d",
                              this, rv);
                    return rv;
                }
            } else {
                rv = threadPool->RegisterForTaskCompleteNotification(&threadPoolNotifier);
                if (rv != 0) {
                    LOG_ERROR("%p:RegisterForTaskCompleteNotification:%d", this, rv);
                    return rv;
                }
            }
        }
        bool isNewDb = false;
        if (allowCreateDB) {
            rv = localDb.openDb(/*OUT*/isNewDb);
        } else {
            rv = localDb.openDbNoCreate();
        }
        if (rv < 0) {
            LOG_WARN("%p: openDb(allowCreate=%d) failed: %d", this, (int)allowCreateDB, rv);
            return rv;
        }
        initial_scan_requested_OneWay |= isNewDb;

        rv = determineSyncTypeMigrateStatus(isNewDb);
        if (rv != 0) {
            LOG_ERROR("%p:determineSyncTypeMigrateStatus:%d", this, rv);
            return rv;
        }

        {   // All SyncConfig types need to support lookup now.
            rv = lookupDb.openDbReadOnly();
            if (rv != 0) {
                LOG_WARN("%p: openDb (for lookup) failed:%d", this, rv);
                return rv;
            }
            lookupDbInit = true;
        }

        {
            rv = getStateDb.openDbReadOnly();
            if (rv != 0) {
                LOG_WARN("%p: openDb (for getState) failed:%d", this, rv);
                return rv;
            }
            getStateDbInit = true;
        }
        {
            rv = lightQueryDb.openDbReadOnly();
            if (rv != 0) {
                LOG_WARN("%p: openDb (for lightQuery) failed:%d", this, rv);
                return rv;
            }
            lightQueryDbInit = true;
        }

        // See end of function for scan request.
        rv = Util_SpawnThread(SyncConfigWorker_ThreadFn, this,
                UTIL_DEFAULT_THREAD_STACK_SIZE, VPL_TRUE, &worker_thread);
        if (rv < 0) {
            LOG_WARN("%p: Util_SpawnThread failed: %d", this, rv);
            return rv;
        }

        SCRow_admin adminEntry;
        rv = localDb.admin_get(adminEntry);
        if (rv != 0) {
            LOG_ERROR("admin_get:%d", rv);
        }

        if (type==SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC ||
              (type==SYNC_TYPE_ONE_WAY_UPLOAD &&
               adminEntry.migrate_from_sync_type_exists))
        {   // Deferred upload modes (hybrid and pure virtual) require this thread.
            // Normal upload also requires the deferred thread if we migrated
            //   from a deferred mode, since successful migration requires that
            //   any deferred work be completed.  Once we empty the deferred queue,
            //   we can safely clear migrate_from_sync_type.
            rv = deferredUploadDb.openDbNoCreate();
            if (rv != 0) {
                LOG_ERROR("%p:openDbNoCreate failed:%d", this, rv);
                return rv;
            }
            deferredUploadDbInit = true;
            // Make sure the first run will perform any work already ready in DB.
            deferredUploadThreadPerformUpload = true;
            deferredUploadThreadPerformError = true;

            deferredUploadThreadInit = true;

            // See end of function for scan request.
            rv = Util_SpawnThread(SyncConfigWorker_DeferredUploadThreadFn, this,
                    UTIL_DEFAULT_THREAD_STACK_SIZE, VPL_TRUE, &deferredUploadThread);
            if (rv < 0) {
                LOG_WARN("%p: Util_SpawnThread failed: %d", this, rv);
                deferredUploadThreadInit = false;
                return rv;
            }
        }

#if VPLFS_MONITOR_SUPPORTED
        if(getSyncDbFamily(type) == SYNC_DB_FAMILY_TWO_WAY ||
           getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD)
        {
            int rc;

            rc = s_fileMonitorDelayQ.Init();
            if(rc != 0) {
                LOG_ERROR("%p: s_fileMonitorDelayQ.Init():%d. "
                          "Continuing without fileMonitor:%s",
                          this, rc, local_dir.c_str());
                goto file_monitor_failed;
            }
            rc = s_fileMonitorDelayQ.AddMonitor(local_dir.str(),
                                                20,
                                                SyncConfigImpl::fnFileMonitorDelayQCallback,
                                                (void*)this,
                                                &fileMonitorHandle);
            if(rc != 0) {
                LOG_ERROR("%p: AddMonitor %s:%d. Continuing without fileMonitor",
                          this, local_dir.c_str(), rc);
                rc = s_fileMonitorDelayQ.Shutdown();
                if(rc != 0) {
                    LOG_ERROR("%p: s_fileMonitorDelayQ.Shutdown():%d", this, rc);
                }
                goto file_monitor_failed;
            }
            fileMonitorHandleSet = true;
        }
 file_monitor_failed:
#endif
        // Automatically schedule a full local scan.
        if(getSyncDbFamily(type) != SYNC_DB_FAMILY_DOWNLOAD)
        {
            RequestFullLocalScan();
        }
        // Automatically schedule a remote scan.
        if(getSyncDbFamily(type) != SYNC_DB_FAMILY_UPLOAD)
        {
            ReportPossibleRemoteChange();
        }
        return rv;
    }

    virtual ~SyncConfigImpl()
    {
#if VPLFS_MONITOR_SUPPORTED
        if(fileMonitorHandleSet) {
            int rc;
            rc = s_fileMonitorDelayQ.RemoveMonitor(fileMonitorHandle);
            if(rc != 0) {
                LOG_ERROR("%p: RemoveMonitor:%d", this, rc);
            }
            rc = s_fileMonitorDelayQ.Shutdown();
            if(rc != 0) {
                LOG_ERROR("%p: s_fileMonitorDelayQ.Shutdown():%d", this, rc);
            }
            fileMonitorHandleSet = false;
        }
#endif
        int rv;
        {
            // Mutex required because lookupDb mutex access is not internally controlled.
            MutexAutoLock lock(&lookupDbMutex);
            if (lookupDbInit) {
                rv = lookupDb.closeDb();
                if (rv != 0) {
                    LOG_ERROR("%p: closeDb lookupDb failed:%d", this, rv);
                }
                lookupDbInit = false;
            }
        }
        {
            // Mutex required because getStateDb mutex access is not internally controlled.
            MutexAutoLock lock(&getStateDbMutex);
            if (getStateDbInit) {
                rv = getStateDb.closeDb();
                if (rv != 0) {
                    LOG_ERROR("%p: closeDb getStateDb failed:%d", this, rv);
                }
                getStateDbInit = false;
            }
        }
        {
            // Mutex required because lightQueryDb mutex access is not internally controlled.
            MutexAutoLock lock(&lightQueryDbMutex);
            if (lightQueryDbInit) {
                rv = lightQueryDb.closeDb();
                if (rv != 0) {
                    LOG_ERROR("%p: closeDb lightQueryDb failed:%d", this, rv);
                }
                lightQueryDbInit = false;
            }
        }
        if (deferredUploadDbInit) {
            rv = deferredUploadDb.closeDb();
            if (rv != 0) {
                LOG_ERROR("%p: close deferredUploadDb failed:%d", this, rv);
            }
            deferredUploadDbInit = false;
        }
        // TODO: commit, rollback, or neither?
        rv = localDb.closeDb();
        if (rv < 0) {
            LOG_ERROR("%p: closeDb failed: %d", this, rv);
        }
        if (threadPool != NULL) {
            if (VPL_IS_INITIALIZED(&threadPoolNotifier)) {
                int rc = threadPool->UnregisterForTaskCompleteNotification(&threadPoolNotifier);
                if(rc != 0) {
                    LOG_ERROR("%p:UnregisterForTaskCompleteNotification:%d", this, rc);
                }
                VPLSem_Destroy(&threadPoolNotifier);
            }
        }
        if (VPL_IS_INITIALIZED(&lookupDbMutex)) {
            VPLMutex_Destroy(&lookupDbMutex);
        }
        if (VPL_IS_INITIALIZED(&getStateDbMutex)) {
            VPLMutex_Destroy(&getStateDbMutex);
        }
        if (VPL_IS_INITIALIZED(&lightQueryDbMutex)) {
            VPLMutex_Destroy(&lightQueryDbMutex);
        }
        if (VPL_IS_INITIALIZED(&work_to_do_cond_var)) {
            VPLCond_Destroy(&work_to_do_cond_var);
        }
        if (VPL_IS_INITIALIZED(&mutex)) {
            VPLMutex_Destroy(&mutex);
        }
    }

protected:
    int determineSyncTypeMigrateStatus(bool isNewDb)
    {
        int rc;

        if (isNewDb) {
            rc = localDb.admin_set(false, 0,
                                   true, user_id,
                                   true, dataset.id,
                                   false, "",
                                   true, (u64)VPLTime_GetTimeStamp(),
                                   true, (u64)type,
                                   false, 0);
            if (rc != 0) {
                LOG_ERROR("%p:admin_set:%d", this, rc);
                return rc;
            }
        } else
        {   // Once 2.6 (SyncConfigSchema version 2) clients no longer exist,
            // this block of code may be removed.
            //
            // Normalizing the initial state.  sync_type should be defined
            // for the previous state was not the case 2.6 and before.
            SCRow_admin adminRow;
            rc = localDb.admin_get(adminRow);
            if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_CRITICAL("%p: admin row does not exist", this);
                return rc;
            } else if (rc != 0) {
                LOG_ERROR("%p:admin_get:%d", this, rc);
                return rc;
            }

            if (!adminRow.sync_type_exists) {
                // Previous version of sync_config did not set sync_type, so the assumption
                // is that the previous syncType was one of
                // {SYNC_TYPE_TWO_WAY, SYNC_TYPE_ONE_WAY_UPLOAD, SYNC_TYPE_ONE_WAY_DOWNLOAD}.
                // This assumption is valid because the previous db filename has the
                // syncType (now syncFamily) in the db filename -- and we would not have found
                // the file if current syncType was a different family.
                SyncType toSet = (SyncType)0;
                SyncDbFamily family = getSyncDbFamily(type);
                switch (family) {
                case SYNC_DB_FAMILY_TWO_WAY:
                    toSet = SYNC_TYPE_TWO_WAY;
                    break;
                case SYNC_DB_FAMILY_UPLOAD:
                    toSet = SYNC_TYPE_ONE_WAY_UPLOAD;
                    break;
                case SYNC_DB_FAMILY_DOWNLOAD:
                    toSet = SYNC_TYPE_ONE_WAY_DOWNLOAD;
                    break;
                }
                LOG_INFO("%p:Previous sync_type version was %d", this, (int)toSet);
                rc = localDb.admin_set(false, 0,
                                       true, user_id,
                                       true, dataset.id,
                                       false, "",
                                       true, (u64)VPLTime_GetTimeStamp(),
                                       true, (u64)toSet,
                                       false, 0);
                if (rc != 0) {
                    LOG_ERROR("%p:admin_set:%d", this, rc);
                    return rc;
                }
            }
        }

        {   // Check if migration is necessary -- current sync_type is different
            // from previous sync_type
            SCRow_admin adminRow;
            rc = localDb.admin_get(adminRow);
            if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_CRITICAL("%p: admin row does not exist", this);
                return rc;
            } else if (rc != 0) {
                LOG_ERROR("%p:admin_get:%d", this, rc);
                return rc;
            }

            if (!adminRow.sync_type_exists) {
                LOG_ERROR("%p:sync type should exist", this);
                return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
            }

            if (adminRow.sync_type != type) {

                if (getSyncDbFamily((SyncType)adminRow.sync_type) !=
                    getSyncDbFamily(type))
                {
                    LOG_CRITICAL("%p:Cannot switch between sync families (%d,%d)",
                                 this, (int)adminRow.sync_type, (int)type);
                    return SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
                }
                if (adminRow.migrate_from_sync_type_exists) {
                    LOG_INFO("%p:Migrate from sync type currently already"
                             "exists(syncType:%d,migrateFrom:%d) currSyncType:%d, "
                              "Currently ok to overwrite",
                              this,
                              (int)adminRow.sync_type,
                              (int)adminRow.migrate_from_sync_type,
                              (int)type);
                    // Note: In the future, it may be necessary to have a cancel
                    // function to put the DB in one sync_type state or the other.
                }

                // Move the previous sync_type over
                rc = localDb.admin_set_sync_type(type,
                                                 true, adminRow.sync_type);
                if (rc != 0) {
                    LOG_ERROR("%p:admin_set_sync_type:%d", this, rc);
                    return rc;
                }
            }
        }
        return 0;
    }

    bool virtualUpload()
    {   // Currently only supported for one-way-up, not two way
        ASSERT(getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD);
        return type == SYNC_TYPE_ONE_WAY_UPLOAD_PURE_VIRTUAL_SYNC ||
               type == SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC;
    }

    bool allowLocalFSUpdate()
    {   // Currently only supported for one-way-down, not two way
        ASSERT(getSyncDbFamily(type) == SYNC_DB_FAMILY_DOWNLOAD);
        return type != SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC;
    }

    bool usesDownloadChangeLog()
    {
        SyncDbFamily family = getSyncDbFamily(type);
        return (family == SYNC_DB_FAMILY_DOWNLOAD) || (family == SYNC_DB_FAMILY_TWO_WAY);
    }

    bool usesUploadChangeLog()
    {
        SyncDbFamily family = getSyncDbFamily(type);
        return (family == SYNC_DB_FAMILY_UPLOAD) || (family == SYNC_DB_FAMILY_TWO_WAY);
    }

    bool useDownloadThreadPool()
    {
        return threadPool && allowLocalFSUpdate();
    }

    /// If true, the deferredUploadThread will enqueue new upload tasks to the
    /// threadPool.  The deferredUploadThread will block on threadPoolNotifier
    /// whenever it cannot proceed.
    /// Note that for migration from hybrid to normal upload, the deferredUploadThread
    /// may actually be running, but it will not be allowed to use the threadPool.
    bool useDeferredUploadThreadPool()
    {
        // If threadPool is present:
        // Cases:
        //  1) type == SYNC_TYPE_ONE_WAY_UPLOAD_PURE_VIRTUAL_SYNC
        //       deferredUploadThread will not be initialized
        //       threadPool will NOT be used in upload worker thread.
        //  2) type == SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC
        //       deferredUploadThread will be initialized
        //       threadPool will be used in deferredUploadThread
        //       threadPool will NOT be used in upload worker thread
        //  3) type == SYNC_TYPE_ONE_WAY_UPLOAD
        //       deferredUploadThread may be initialized if migrating
        //       threadPool will NOT be used in deferredUploadThread
        //       threadPool will be used in upload worker thread
        return threadPool && virtualUpload();
    }

    /// If true, the primary worker thread will enqueue new tasks to the
    /// threadPool.  The primary worker thread will block on threadPoolNotifier
    /// whenever it cannot proceed.
    bool useUploadThreadPool()
    {
        return threadPool && !virtualUpload();
    }

    bool hasFreeThread(bool& shutdown)
    {
        shutdown = false;
        if(threadPool!=NULL) {
            u32 unoccupiedThreads = 0;
            u32 totalThreads = 0;
            if(hasDedicatedThread) {
                threadPool->GetInfoIncludeDedicatedThread(
                                    &threadPoolNotifier,
                                    /*OUT*/ unoccupiedThreads,
                                    /*OUT*/ totalThreads,
                                    /*OUT*/ shutdown);
            }else{
                threadPool->GetInfo(/*OUT*/ unoccupiedThreads,
                                    /*OUT*/ totalThreads,
                                    /*OUT*/ shutdown);
            }
            if(unoccupiedThreads > 0) {
                return true;
            }
        }
        return false;
    }

    void HANDLE_DB_FAILURE()
    {
        LOG_CRITICAL("%p: Local database failure.  If this problem persists, please delete \"%s\".",
                this, localDb.getDbDir().c_str());
        // TODO: let's only enable this for 2.6, when we have more testing time:
        //RequestClose();
    }
    
    void localDb_needDownloadScan_add(const std::string& dirPath, u64 compId)
    {
        int rc = localDb.needDownloadScan_add(dirPath, compId);
        if (rc < 0) {
            LOG_CRITICAL("%p: needDownloadScan_add(%s, "FMTu64") failed: %d",
                    this, dirPath.c_str(), compId, rc);
            HANDLE_DB_FAILURE();
        }
    }

    void localDb_uploadChangeLog_add_and_setSyncStatus(
            const std::string& parent_path,
            const std::string& name,
            bool is_dir,
            u64 upload_action,
            bool comp_id_exist,
            u64 comp_id,
            bool base_revision_exist,
            u64 base_revision,
            const std::string& hash_value,
            bool file_size_exist,
            u64 file_size)
    {
        int rc;
        rc = localDb.uploadChangeLog_remove(parent_path, name);
        if (rc == 0) {
            LOG_INFO("%p: Removed repeat uploadChangeLog(%s,%s)",
                     this, parent_path.c_str(), name.c_str());
        }else if (rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            // IGNORE, expected to not have deleted anything
        } else {
            LOG_ERROR("%p:uploadChangeLog_remove(%s,%s):%d",
                      this, parent_path.c_str(),
                      name.c_str(), rc);
            // can continue on.  Although, error here not expected.
        }

        rc = localDb.uploadChangeLog_add(parent_path, name, is_dir, upload_action,
                comp_id_exist, comp_id, base_revision_exist, base_revision,
                hash_value, file_size_exist, file_size);
        if (rc < 0) {
            LOG_CRITICAL("%p: uploadChangeLog_add(%s, %s) failed: %d",
                    this, parent_path.c_str(), name.c_str(), rc);
            HANDLE_DB_FAILURE();
        }
        updateUploadsRemaining();
        SetStatus(SYNC_CONFIG_STATUS_SYNCING);
    }


    int localDb_downloadChangeLog_add_and_setSyncStatus(
                                      const std::string& parent_path,
                                      const std::string& name,
                                      bool is_dir,
                                      u64 download_action,
                                      u64 comp_id,
                                      u64 revision,
                                      u64 client_reported_mtime,
                                      bool is_on_acs,
                                      const std::string& hash_value,
                                      bool file_size_exist,
                                      u64 file_size)
    {
        int rc;
        rc = localDb.downloadChangeLog_remove(parent_path,
                                              name);
        if (rc == 0) {
            LOG_INFO("%p: Removed repeat downloadChangeLog(%s,%s)",
                     this, parent_path.c_str(), name.c_str());
        }else if (rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            // IGNORE, expected to not have deleted anything
        } else {
            LOG_ERROR("%p:downloadChangeLog_remove(%s,%s):%d",
                      this, parent_path.c_str(),
                      name.c_str(), rc);
            // can continue on.  Although, error here not expected.
        }

        rc = localDb.downloadChangeLog_add(parent_path,
                                           name,
                                           is_dir,
                                           download_action,
                                           comp_id,
                                           revision,
                                           client_reported_mtime,
                                           is_on_acs,
                                           hash_value,
                                           file_size_exist,
                                           file_size);
        updateDownloadsRemaining();
        SetStatus(SYNC_CONFIG_STATUS_SYNCING);
        return rc;
    }

    /// @return true if the worker thread should completely stop.
    bool checkForPauseStop(bool transientErrPauseMode,
                           VPLTime_t transientErrPauseTime)
    {
        return checkForPauseStopHelper(transientErrPauseMode,
                                       transientErrPauseTime,
                                       false,
                                       false);
    }

    /// Version of the checkForPauseStop function within a task.
    /// Transient retry pausing is acceptable, because the task is still
    /// making forward progress, but a SyncConfig pause could be arbitrarily long.
    /// If the SyncConfig is paused, this version will immediately return with an error
    /// rather than blocking.  Pausing while in a task of a thread pool
    /// would affect other tasks waiting for time within that thread pool.
    /// @return true if the worker thread should completely stop.
    bool checkForPauseStopWithinTask(bool transientErrPauseMode,
                                     VPLTime_t transientErrPauseTime)
    {
        return checkForPauseStopHelper(transientErrPauseMode,
                                       transientErrPauseTime,
                                       false,
                                       true);
    }

    /// @return true if the worker thread should completely stop.
    bool checkForPauseStop()
    {
        MutexAutoLock lock(&mutex);
        bool result = checkForPauseStop(false, VPLTIME_INVALID);
        return result;
    }

    /// @return true if the worker thread should completely stop.
    bool checkForPauseStopWithinDeferredUploadLoop()
    {
        MutexAutoLock lock(&mutex);
        bool result = checkForPauseStopHelper(false,
                                              VPLTIME_INVALID,
                                              true,
                                              false);
        return result;
    }

    /// @return true if the worker thread should completely stop.
    bool clearHttpHandleAndCheckForPauseStop(VPLHttp2& httpHandle)
    {
        return clearHttpHandleAndCheckForPauseStop(httpHandle, false, VPLTIME_INVALID);
    }

    /// @return true if the worker thread should completely stop.
    bool clearHttpHandleAndCheckForStopWithinTask(VPLHttp2& httpHandle)
    {
        MutexAutoLock lock(&mutex);
        clearHttpHandle(httpHandle);
        if (stop_thread) {
            return true;
        }
        return false;
    }

    /// @return true if the worker thread should completely stop.
    bool setHttpHandleAndCheckForStop(VPLHttp2& httpHandle)
    {
        MutexAutoLock lock(&mutex);
        if (stop_thread) {
            return true;
        }
        http_handles_to_cancel.push_back(&httpHandle);
        return false;
    }

    void clearHttpHandle(VPLHttp2& httpHandle)
    {
        MutexAutoLock lock(&mutex);
        for(std::vector<VPLHttp2*>::iterator httpIter = http_handles_to_cancel.begin();
            httpIter != http_handles_to_cancel.end();
            ++httpIter)
        {
            if(&httpHandle == *httpIter) {
                http_handles_to_cancel.erase(httpIter);
                break;
            }
        }
    }

    void registerForAsyncCancel(VCSArchiveOperation* vcsArchiveOperation)
    {
        MutexAutoLock lock(&mutex);
        archive_operations_for_async_cancel.push_back(vcsArchiveOperation);
    }

    void unregisterForAsyncCancel(VCSArchiveOperation* vcsArchiveOperation)
    {
        MutexAutoLock lock(&mutex);
        for(std::vector<VCSArchiveOperation*>::iterator aoIter =
                archive_operations_for_async_cancel.begin();
            aoIter != archive_operations_for_async_cancel.end();
            ++aoIter)
        {
            if(vcsArchiveOperation == *aoIter) {
                archive_operations_for_async_cancel.erase(aoIter);
                break;
            }
        }
    }

    void registerForAsyncCancel(VCSFileUrlOperation* vcsFileUrlOperation)
    {
        MutexAutoLock lock(&mutex);
        file_url_operation_for_async_cancel.push_back(vcsFileUrlOperation);
    }

    void unregisterForAsyncCancel(VCSFileUrlOperation* vcsFileUrlOperation)
    {
        MutexAutoLock lock(&mutex);
        for(std::vector<VCSFileUrlOperation*>::iterator vcsFUOIter =
                file_url_operation_for_async_cancel.begin();
            vcsFUOIter != file_url_operation_for_async_cancel.end();
            ++vcsFUOIter)
        {
            if(vcsFileUrlOperation == *vcsFUOIter) {
                file_url_operation_for_async_cancel.erase(vcsFUOIter);
                break;
            }
        }
    }

    /// Make a best effort to convert a filesystem time to a normalized local time.
    /// The goal is to get back the same value after a user changes their system's timezone.
    static VPLTime_t fsTimeToNormLocalTime(const AbsPath& absPath, VPLTime_t fsTime)
    {
        // For fat32, the OS attempts to convert to UTC time; however,
        // there is no time zone data stored in fat32 fs timestamp, so the
        // conversion to UTC is incorrect.  Thus, the time will change along
        // with timezone change. The sync algorithm does not care about UTC time,
        // and only cares that the time remains consistent, undoing the UTC
        // conversion done by the OS.
#if defined(ANDROID)
        // Assume ANDROID platform means the filesystem is FAT32.
        // TODO: is this true?
        fsTime = fsTime - VPLTime_FromSec(timezone);
#endif
        return fsTime;
    }

    static u64 normLocalTimeToVcsTime(VPLTime_t time)
    {
        // Bug 10508: Don't convert the time if it is 0.
        if (time == 0) {
            return 0;
        }
        // TODO: impl
        return time;
    }

    static VPLTime_t vcsTimeToNormLocalTime(VPLTime_t time)
    {
        // Bug 10508: Don't convert the time if it is 0.
        if (time == 0) {
            return 0;
        }
        // TODO: impl
        return time;
    }

    /// To avoid the case where a user changes a file and the filesystem writes the same
    /// timestamp that was already there, we should only set timestamps that are at least
    /// one timestamp interval in the past.
    static VPLTime_t getLatestNormLocalTimeToSet(const AbsPath& absPath)
    {
        VPLTime_t currTime = VPLTime_GetTime();
        // TODO: normalize to UTC?
        return VPLTime_DiffClamp(currTime, FS_TIMESTAMP_GRANULARITY);
    }

    static VPLTime_t vcsTimeToVPLFileSetTime(const AbsPath& absPath, u64 time)
    {
        // TODO: impl
        return time;
    }

    template<class T> inline
    bool contains(const std::set<T>& container, const T& value)
    {
        return container.find(value) != container.end();
    }

    static const int FILESYS_OPERATIONS_PER_STOP_CHECK = 250;

    /// @return Absolute path on the local filesystem for the relative (to sync config root) path.
    AbsPath getAbsPath(const SCRelPath& syncConfigRelPath) const
    {
        return local_dir.appendRelPath(syncConfigRelPath);
    }

    static bool isRoot(const SCRow_syncHistoryTree& entry)
    {
        // Root is stored in the tree.  Root is an exception case because
        // the root will be returned as a child when getting the children
        // of the root path.
        if (entry.name.size() == 0) {
            if (entry.parent_path.size() != 0) {
                FAILED_ASSERT("Unexpected parent_path \"%s\"", entry.parent_path.c_str());
            }
            ASSERT(entry.is_dir);
            return true;
        }
        return false;
    }

    static std::string getConflictRenamePath(const AbsPath& destAbsPath, const std::string& devName)
    {
        std::string toReturn; // TODO: StringStream would be more efficient.
        std::string ext;

        size_t pos = destAbsPath.str().find_last_of("/.");
        if (pos != std::string::npos) {
            if (destAbsPath.str().at(pos) == '.') {
                ext = destAbsPath.str().substr(pos, destAbsPath.str().size());
            }
        }

        if (ext.empty())
            toReturn = destAbsPath.str();
        else {
            toReturn = destAbsPath.str().substr(0, pos);
        }

        toReturn.append("_CONFLICT_(");

        if (devName.empty()) {
            toReturn += "Unknown";
        } else {
            toReturn += devName;
        }
        toReturn.append("-");
        VPLTime_CalendarTime_t myTime;
        VPLTime_GetCalendarTimeLocal(&myTime);
        char tempBuf[32];
        snprintf(tempBuf, sizeof(tempBuf), "%d", myTime.year);
        toReturn.append(tempBuf);
        toReturn.append("-");
        snprintf(tempBuf, sizeof(tempBuf), "%02d", myTime.month);
        toReturn.append(tempBuf);
        toReturn.append("-");
        snprintf(tempBuf, sizeof(tempBuf), "%02d", myTime.day);
        toReturn.append(tempBuf);
        toReturn.append("-");
        snprintf(tempBuf, sizeof(tempBuf), "%02d.%02d.%02d.%03d", myTime.hour, myTime.min, myTime.sec, myTime.msec);
        toReturn.append(tempBuf);
        toReturn.append(")");

        if (!ext.empty()) {
            toReturn.append(ext);
        }

        return toReturn;
    }

    /// @return the relative (to sync config root) path for the entry, no leading or trailing slashes.
    static SCRelPath getRelPath(const SCRow_syncHistoryTree& entry)
    {
        if (entry.parent_path.size() == 0) {
            // Entry is in the sync config root (or is the sync config root itself).
            return SCRelPath(entry.name);
        } else {
            ASSERT(entry.name.size() != 0);
            return SCRelPath(entry.parent_path + "/" + entry.name);
        }
    }
    /// @return the relative (to sync config root) path for the entry, no leading or trailing slashes.
    static SCRelPath getRelPath(const SCRow_uploadChangeLog& entry)
    {
        if (entry.parent_path.size() == 0) {
            // Entry is in the sync config root (or is the sync config root itself).
            return SCRelPath(entry.name);
        } else {
            ASSERT(entry.name.size() != 0);
            return SCRelPath(entry.parent_path + "/" + entry.name);
        }
    }
    /// @return the relative (to sync config root) path for the entry, no leading or trailing slashes.
    static SCRelPath getRelPath(const SCRow_deferredUploadJobSet& entry)
    {
        if (entry.parent_path.size() == 0) {
            // Entry is in the sync config root (or is the sync config root itself).
            return SCRelPath(entry.name);
        } else {
            ASSERT(entry.name.size() != 0);
            return SCRelPath(entry.parent_path + "/" + entry.name);
        }
    }
    /// @return the relative (to sync config root) path for the entry, no leading or trailing slashes.
    static SCRelPath getRelPath(const SCRow_downloadChangeLog& entry)
    {
        if (entry.parent_path.size() == 0) {
            // Entry is in the sync config root (or is the sync config root itself).
            return SCRelPath(entry.name);
        } else {
            ASSERT(entry.name.size() != 0);
            return SCRelPath(entry.parent_path + "/" + entry.name);
        }
    }
    /// @return the relative (to sync config root) path for the entry, no leading or trailing slashes.
    static SCRelPath getRelPath(const SCRow_needDownloadScan& entry)
    {
        return SCRelPath(entry.dir_path);
    }

    /// @return the dataset relative path for the SyncConfig-relative path.
    DatasetRelPath getDatasetRelPath(const SCRelPath& scRelPath) const
    {
        return server_dir.getDescendent(scRelPath.str());
    }
    /// @return the dataset relative path for the entry.
    DatasetRelPath getDatasetRelPath(const SCRow_uploadChangeLog& entry) const
    {
        SCRelPath scRelPath(getRelPath(entry));
        return getDatasetRelPath(scRelPath);
    }
    /// @return the dataset relative path for the entry.
    DatasetRelPath getDatasetRelPath(const SCRow_downloadChangeLog& entry) const
    {
        SCRelPath scRelPath(getRelPath(entry));
        return getDatasetRelPath(scRelPath);
    }
    /// @return the dataset relative path for the entry.
    DatasetRelPath getDatasetRelPath(const SCRow_deferredUploadJobSet& entry) const
    {
        SCRelPath scRelPath(getRelPath(entry));
        return getDatasetRelPath(scRelPath);
    }

    u64 my_device_id() const
    {
        return dataset_access_info.deviceId;
    }

    int syncHistoryTree_getEntry(const SCRelPath& scRelPath, SCRow_syncHistoryTree& entry_out)
    {
        return localDb.syncHistoryTree_get(scRelPath.getParent().str(), scRelPath.getName(), entry_out);
    }

    /// Store the componentId of the SyncConfig root to the LocalDB syncHistoryTree.
    int setRootCompId(u64 compId){
        // The root is represented by parent_dir = "", name = "".
        int rv;
        SCRow_syncHistoryTree newEntry;
        newEntry.parent_path = "";
        newEntry.name = "";
        newEntry.is_dir = true;
        newEntry.comp_id_exists = true;
        newEntry.comp_id = compId;
        rv = localDb.syncHistoryTree_add(newEntry);
        if (rv != 0) {
            LOG_CRITICAL("%p: syncHistoryTree_add failed for sync config root: %d", this, rv);
            HANDLE_DB_FAILURE();
        }
        return rv;
    }

    /// Create the sync config root (and any missing parent directories within the dataset),
    /// and update the localDB with sync config root compId.
    /// The caller is responsible for retrying if there is any error.
    int createSyncConfigRootOnVcs(u64& compId_out)
    {
        int rc = 0;
        compId_out = 0; // zero it out in case caller fails to check for error
        u64 currCompId = 0;
        LOG_INFO("%p: ACTION VCS root create:%s", this, server_dir.c_str());
        // Get the compId for the dataset root (and store it to currCompId).
        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
        {
            VPLHttp2 httpHandle;
            if (setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            rc = vcs_get_comp_id(vcs_session,
                                 httpHandle,
                                 dataset,
                                 "",
                                 verbose_http_log,
                                 currCompId);
            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            if (!isTransientError(rc)) { break; }
            LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                    return SYNC_AGENT_ERR_STOPPING;
                }
            }
        }
        if (rc != 0) {
            LOG_WARN("%p: vcs_get_comp_id failed:%d", this, rc);
            // Caller should try again later.
            return rc;
        }

        // For each descendant of the dataset root on the path to the sync config root,
        // create the directory (if necessary) and get the compId.
        if (server_dir.str().size() > 0) {
            // Consider there to be an imaginary slash at [-1].
            size_t currSlash = (size_t)(-1);
            do {
                // If there are no more slashes, we will get npos, which can be safely passed
                // to substr to get the entire string.
                currSlash = server_dir.str().find('/', currSlash + 1);

                VcsMakeDirResponse vcsMakeDirOut;
                string currRelPath = server_dir.str().substr(0, currSlash);

                LOG_INFO("%p: ACTION    VCS root create mkdir:%s", this, currRelPath.c_str());
                for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                {
                    VPLHttp2 httpHandle;
                    if (setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
                    rc = vcs_make_dir(vcs_session, httpHandle, dataset,
                            currRelPath,
                            currCompId,
                            DEFAULT_MYSQL_DATE /* infoLastChanged */,
                            DEFAULT_MYSQL_DATE /* infoCreateDate */,
                            my_device_id(),
                            verbose_http_log,
                            vcsMakeDirOut);
                    if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                            return SYNC_AGENT_ERR_STOPPING;
                        }
                    }
                }
                if (rc != 0) {
                    LOG_WARN("%p: vcs_make_dir %s,"FMTu64" returned %d",
                              this, currRelPath.c_str(), currCompId, rc);
                    // Caller should try again later.
                    return rc;
                }
                LOG_INFO("%p: ACTION    VCS root create mkdir done:%s", this, currRelPath.c_str());
                currCompId = vcsMakeDirOut.compId;

            } while (currSlash != string::npos);
        }

        // Success; currCompId is now the sync config root dir's compId.
        LOG_INFO("%p: ACTION VCS root create done:%s", this, server_dir.c_str());
        compId_out = currCompId;

        rc = setRootCompId(currCompId);
        if (rc != 0) {
            LOG_ERROR("%p: Setting root compId failed:%d", this, rc);
            // HANDLE_DB_FAILURE already called inside setRootCompId().
        }
        return rc;
    }

    /// Enqueue SyncConfig root to LocalDB.need_download_scan.
    /// The caller is responsible for dealing with the following error cases:
    /// - #SYNC_AGENT_ERR_STOPPING
    /// - #VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT
    /// - isTransientError
    /// - other unexpected errors
    /// .
    int enqueueRootToNeedDownloadScan()
    {
        u64 compId;
        int rc;

        SCRelPath scrRoot = SCRelPath("");
        DatasetRelPath drpRoot = getDatasetRelPath(scrRoot);
        SCRow_syncHistoryTree entry_out;
        rc = localDb.syncHistoryTree_get("", scrRoot.str(), entry_out);
        if (rc == 0 && entry_out.comp_id_exists) {
            compId = entry_out.comp_id;
        } else {
            LOG_INFO("%p: localDb get root failed:%d, may be first traverse, getting compId",
                     this, rc);
            for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
            {
                VPLHttp2 httpHandle;
                if (setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
                rc = vcs_get_comp_id(vcs_session,
                                     httpHandle,
                                     dataset,
                                     drpRoot.str(),
                                     verbose_http_log,
                                     compId);
                if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
                if (!isTransientError(rc)) { break; }
                LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                    if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                        return SYNC_AGENT_ERR_STOPPING;
                    }
                }
            }
            if (rc < 0) {
                // Caller is responsible for dealing with the error.
                return rc;
            }

            rc = setRootCompId(compId);
            if (rc != 0) {
                LOG_ERROR("%p: Setting root compId failed:%d", this, rc);
                // HANDLE_DB_FAILURE already called inside setRootCompId().
                return rc;
            }
        }

        localDb_needDownloadScan_add(scrRoot.str(), compId);
        return 0;
    }

    /// The caller is responsible for retrying if there is any error.
    int getParentCompId(const SCRow_uploadChangeLog& currUploadEntry,
                        u64& parentCompId_out)
    {
        int rc;
        SCRow_syncHistoryTree parentHistoryEntry;
        rc = syncHistoryTree_getEntry(SCRelPath(currUploadEntry.parent_path), parentHistoryEntry);
        if ((rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) ||
            ((rc == 0) && !parentHistoryEntry.comp_id_exists)) {
            // No record for the parent directory compId!
            // Special case: if parent dir is the sync config root, create it now.
            if (currUploadEntry.parent_path.size() == 0) {
                // Parent is the sync config root!
                rc = createSyncConfigRootOnVcs(parentCompId_out);
                if (checkForPauseStop()) {
                    return SYNC_AGENT_ERR_STOPPING;
                }
                if (rc < 0) {
                    LOG_WARN("%p: createSyncConfigRootOnVcs failed: %d", this, rc);
                    // Warning because this may be transient error.
                    // Up to caller to decide whether to try again later.
                    return rc;
                }
                return rc;
            } else {
                LOG_WARN("%p: Parent dir (%s) doesn't have a comp_id!", this, currUploadEntry.parent_path.c_str());
                // This is Case J.  We should recover next time we do an upload scan.
                // This can happen if the VCS POST dir call for the parent directory fails for some
                // reason (such as VCS_PROVIDED_FOLDER_PATH_DOESNT_MATCH_PROVIDED_PARENTCOMPID).
                return SYNC_AGENT_ERR_NEED_UPLOAD_SCAN;
            }
        }
        else if (rc != 0) {
            LOG_CRITICAL("%p: syncHistoryTree_getEntry(%s) failed: %d", this, currUploadEntry.parent_path.c_str(), rc);
            HANDLE_DB_FAILURE();
            return rc;
        }
        else {
            parentCompId_out = parentHistoryEntry.comp_id;
            return 0;
        }
        return rc;
    }

    // Special case checking for file content matches during VCS scanning.
    // Only applies to OneWayUp and OneWayDown.
    bool checkIfLocalFileMatches(const AbsPath& localFilePath,
                                 const VcsFile& vcsFile,
                                 VPLTime_t& localFsMtime_out)
    {
        switch (sync_policy.how_to_compare) {
        case SyncPolicy::FILE_COMPARE_POLICY_DEFAULT:
            return false;
        case SyncPolicy::FILE_COMPARE_POLICY_USE_SIZE_FOR_MIGRATION:
            if ((vcsFile.lastChanged != 0) || (vcsFile.createDate != 0)) {
                // Not a migration file.  The variables lastChangedNanoSecs and createDateNanoSecs
                // are not set if the files were migrated.  Normally, these variables should always
                // be set when uploaded by a 2.5.0 (or more recent) client.
                return false;
            } else {  // This is a migration file.
                VPLFS_stat_t statBuf;
                int rc = VPLFS_Stat(localFilePath.c_str(), &statBuf);
                if (rc < 0) {
                    LOG_INFO("%p: Unable to stat \"%s\": %d", this, localFilePath.c_str(), rc);
                    return false;
                }
                if (statBuf.type == VPLFS_TYPE_DIR) {
                    return false;
                }
                localFsMtime_out = statBuf.vpl_mtime;
                return (statBuf.size == vcsFile.latestRevision.size);
            }
            break;
        case SyncPolicy::FILE_COMPARE_POLICY_USE_HASH:
            // TODO: Fill this in if we want to support hashes for the OneWay syncs.
            //   Compare local file hash with vcsFile.hashValue;
        default:
            FAILED_ASSERT("Unhandled: %d", (int)sync_policy.how_to_compare);
            return false;
        }
    }

    /// @param[out] changed_out will be set appropriately even if this function returns an error.
    /// @return SYNC_AGENT_ERR_STOPPING if the caller should stop immediately.
    int checkDatasetVersionChanged(bool& changed_out)
    {
        u64 currDatasetVersion = 0;
        changed_out = false;

        // This function calls VCS, so we shouldn't be holding the mutex.
        ASSERT(!VPLMutex_LockedSelf(&mutex));

        // Get the current dataset version from VCS.
        int rc;
        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
        {
            VPLHttp2 httpHandle;
            if (setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            rc = vcs_get_dataset_info(vcs_session,
                                      httpHandle,
                                      dataset,
                                      verbose_http_log,
                                      /*out*/ currDatasetVersion);
            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            if (!isTransientError(rc)) { break; }
            LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                    return SYNC_AGENT_ERR_STOPPING;
                }
            }
        }
        if (rc != 0) {
            if (isTransientError(rc)) {
                LOG_WARN("%p: Transient error: vcs_get_dataset_info returned %d", this, rc);
                MutexAutoLock lock(&mutex);
                dataset_version_check_error = true;
                setErrorTimeout(sync_policy.error_retry_interval);
            } else {
                // Non-retryable error.
                LOG_ERROR("%p: Unexpected: vcs_get_dataset_info returned %d for "FMTu64,
                        this, rc, dataset.id);
                // If the dataset was suspended, we expect the infra to send us another
                // dataset_content_change notification when the dataset is unsuspended.
                // If the dataset was destroyed, we expect CCD to
                // eventually get updated from VSDS and recreate the SyncConfig.
            }
            return rc;
        }

        MutexAutoLock lock(&mutex);
        if (currDatasetVersion != last_seen_dataset_version) {
            if (currDatasetVersion < last_seen_dataset_version) {
                LOG_ERROR("%p: dataset version went backward "FMTu64"->"FMTu64"!",
                        this, last_seen_dataset_version, currDatasetVersion);
            }
            LOG_INFO("%p: dataset version changed "FMTu64"->"FMTu64"; performing scan.",
                    this, last_seen_dataset_version, currDatasetVersion);
            last_seen_dataset_version = currDatasetVersion;
            changed_out = true;
        } else {
            LOG_INFO("%p: dataset version is still "FMTu64"; skipping scan.",
                    this, currDatasetVersion);
        }
        return 0;
    }

    int beginDbTransaction()
    {
        ThreadTransactionState* transactionState = getThreadTransactionState();
        if(!transactionState->b_inTransaction) {
            LOG_INFO("%p: begin DB transaction", this);
            int dbBegin = localDb.beginTransaction();
            if(dbBegin != 0) {
                LOG_CRITICAL("%p: beginTransaction:%d", this, dbBegin);
                return dbBegin;
            }else{
                transactionState->b_inTransaction = true;
            }
        }
        return 0;
    }

    static const u32 TRANSACTION_NUM_THRESHHOLD = 10;
    static const u64 TRANSACTION_BYTES_THRESHHOLD = 2*1024*1024;
    int commitDbTransaction(u64 numTransactionBytes,
                            bool force_commit)
    {
        ThreadTransactionState* transactionState = getThreadTransactionState();
        if(transactionState->b_inTransaction)
        {
            ++(transactionState->numTransactionChanges);
            transactionState->numTransactionBytes += numTransactionBytes;
            if(force_commit ||
               transactionState->numTransactionChanges >= TRANSACTION_NUM_THRESHHOLD ||
               transactionState->numTransactionBytes >= TRANSACTION_BYTES_THRESHHOLD)
            {
                LOG_INFO("%p: commit to DB (forceCommit=%d, numChanges="FMTu32", numBytes="FMTu64")",
                        this, force_commit, transactionState->numTransactionChanges,
                        transactionState->numTransactionBytes);
                int rc = localDb.commitTransaction();
                if(rc != 0) {
                    LOG_CRITICAL("%p: commitTransaction:%d - %d",
                              this, rc, transactionState->numTransactionChanges);
                    int revertRes = localDb.rollbackTransaction();
                    if(revertRes != 0) {
                        LOG_CRITICAL("%p: rollbackTransaction:%d", this, revertRes);
                    }else{  // Revert successful
                        transactionState->reset();
                    }
                    return rc;
                }else{
                    transactionState->reset();
                }
            }
        }
        return 0;
    }

    AbsPath getTempDownloadFilePath(u64 compId, u64 revision)
    {
        ostringstream temp;
        temp << hex; // Convert compId and revision to hexadecimal.
        temp << local_dir.str() << "/" << SYNC_TEMP_DIR << "/tmpDown" << compId << "_" << revision;
        return AbsPath(temp.str());
    }

    AbsPath getTempDeleteDir()
    {   // Temp directory used for deletions
        ostringstream temp;
        temp << local_dir.str() << "/" << SYNC_TEMP_DIR << "/to_delete";
        return AbsPath(temp.str());
    }

    /// Temp file to touch and stat to get the current filesystem timestamp.
    AbsPath getTempTouchTimestampFile()
    {
        return AbsPath(SSTR(local_dir.str() << "/" << SYNC_TEMP_DIR << "/touch_timestamp"));
    }

    bool hasRetryErrors()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        return next_error_processing_timestamp != VPLTIME_INVALID;
    }

    /// Is it time for the primary worker to do error processing?
    bool isTimeToRetryErrors()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        return (getTimeoutUntilNextRetryErrors() == 0);
    }

    VPLTime_t getTimeoutUntilNextRetryErrors()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        if (next_error_processing_timestamp == VPLTIME_INVALID) {
            // next_error_processing_timestamp is used as indication if an error retry is needed.
            return VPL_TIMEOUT_NONE;
        } else if (force_error_retry_timestamp == VPLTIME_INVALID) {
            // Fall back to next_error_processing_timestamp if force_error_retry_timestamp is not set.
            return VPLTime_DiffClamp(next_error_processing_timestamp, VPLTime_GetTimeStamp());
        } else {
            VPLTime_t earliest = MIN(next_error_processing_timestamp, force_error_retry_timestamp);
            return VPLTime_DiffClamp(earliest, VPLTime_GetTimeStamp());
        }
    }

    void clearErrorRetry() {
        next_error_processing_timestamp = VPLTIME_INVALID;
        force_error_retry_timestamp = VPLTIME_INVALID;
    }

    void setErrorTimeout(VPLTime_t waitDuration) {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        ASSERT(waitDuration != VPLTIME_INVALID);
        VPLTime_t setErrorTime = VPLTime_GetTimeStamp() + waitDuration;
        if(next_error_processing_timestamp == VPLTIME_INVALID ||
           next_error_processing_timestamp < setErrorTime)
        {  // Wait at least setErrorTime, because if there's a lot of work (that
           // takes sync_policy.error_retry_interval), we don't want the CPU to continuously spin.
            next_error_processing_timestamp = setErrorTime;
        }
    }

    void forceErrorRetry(VPLTime_t waitDuration) {
        force_error_retry_timestamp = VPLTime_GetTimeStamp() + waitDuration;
    }

    void clearForceRetry() {
        force_error_retry_timestamp = VPLTIME_INVALID;
    }

    void clearErrorRetryIfNoErrorsRemain()
    {
        int nextChangeRc;
        
        ASSERT(VPLMutex_LockedSelf(&mutex));
        
        if (dataset_version_check_error) {
            LOG_INFO("%p: A dataset version check error remains.", this);
            return;
        }
        if (downloadScanError) {
            LOG_INFO("%p: A download scan error remains.", this);
            return;
        }
        if (uploadScanError) {
            LOG_INFO("%p: An upload scan error remains.", this);
            return;
        }
        if ((getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD) && initial_scan_requested_OneWay) {
            LOG_INFO("%p: An error requiring a VCS scan remains.", this);
            return;
        }
        if (usesDownloadChangeLog()) {
            SCRow_downloadChangeLog dummyEntry;
            // Download:
            nextChangeRc = localDb.downloadChangeLog_getErrDownload(INT64_MAX, /*out*/dummyEntry);
            if (nextChangeRc == 0) {
                LOG_INFO("%p: At least one download error remains: "FMTu64","FMTu64",%s,%s",
                        this, dummyEntry.comp_id, dummyEntry.revision,
                        dummyEntry.parent_path.c_str(), dummyEntry.name.c_str());
                return;
            } else if (nextChangeRc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_ERROR("%p: downloadChangeLog_getErrDownload failed: %d", this, nextChangeRc);
                return;
            }
            ASSERT(nextChangeRc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);
            // Copyback:
            nextChangeRc = localDb.downloadChangeLog_getErrCopyback(INT64_MAX, /*out*/dummyEntry);
            if (nextChangeRc == 0) {
                LOG_INFO("%p: At least one copyback error remains: "FMTu64","FMTu64",%s,%s",
                        this, dummyEntry.comp_id, dummyEntry.revision,
                        dummyEntry.parent_path.c_str(), dummyEntry.name.c_str());
                return;
            } else if (nextChangeRc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_ERROR("%p: downloadChangeLog_getErrCopyback failed: %d", this, nextChangeRc);
                return;
            }
            ASSERT(nextChangeRc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);
        }
        if (usesUploadChangeLog()) {
            SCRow_uploadChangeLog dummyEntry;
            // Upload:
            nextChangeRc = localDb.uploadChangeLog_getErr(INT64_MAX, /*out*/dummyEntry);
            if (nextChangeRc == 0) {
                LOG_INFO("%p: At least one upload error remains: "FMTu64","FMTu64",%s,%s",
                        this, dummyEntry.comp_id, dummyEntry.revision,
                        dummyEntry.parent_path.c_str(), dummyEntry.name.c_str());
                return;
            } else if (nextChangeRc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_ERROR("%p: uploadChangeLog_getErr failed: %d", this, nextChangeRc);
                return;
            }
            ASSERT(nextChangeRc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);
        }
        LOG_INFO("%p: No errors remain.", this);
        clearErrorRetry();
    }

    static VPLTHREAD_FN_DECL SyncConfigWorker_ThreadFn(void* arg)
    {
        SyncConfigImpl* self = (SyncConfigImpl*)arg;
        LOG_INFO("%p: This is the primary worker thread.", self);
        self->workerLoop();
        LOG_INFO("%p: Primary worker thread exiting.", self);

        MutexAutoLock lock(&self->mutex);
        self->worker_loop_paused_or_stopped = true;
        // If there is a pause request blocking, signal the waiting thread now.
        if (self->worker_loop_wait_for_pause_sem != NULL) {
            int rc = VPLSem_Post(self->worker_loop_wait_for_pause_sem);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Post(%p):%d",
                          self, self->worker_loop_wait_for_pause_sem, rc);
            }
        }
        return VPLTHREAD_RETURN_VALUE;
    }

    static VPLTHREAD_FN_DECL SyncConfigWorker_DeferredUploadThreadFn(void* arg)
    {
        SyncConfigImpl* self = (SyncConfigImpl*)arg;
        LOG_INFO("%p: This is the deferred upload thread.", self);
        self->deferredUploadLoop();
        LOG_INFO("%p: Deferred upload thread exiting", self);

        MutexAutoLock lock(&self->mutex);
        self->deferred_upload_loop_paused_or_stopped = true;
        // If there is a pause request blocking, signal the waiting thread now.
        if (self->deferred_upload_loop_wait_for_pause_sem != NULL) {
            int rc = VPLSem_Post(self->deferred_upload_loop_wait_for_pause_sem);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Post(%p):%d",
                          self, self->deferred_upload_loop_wait_for_pause_sem, rc);
            }
        }
        return VPLTHREAD_RETURN_VALUE;
    }

    virtual void workerLoop() = 0;

    virtual void deferredUploadLoop() = 0;

    // NoteA: We use VPLCond_Broadcast rather than VPLCond_Signal because
    // now that we have a threadPool per syncConfig, there could be a chance
    // that more than one thread is waiting on the condition variable
    // work_to_do_cond_var (in the Task function).  VPLCond_Signal would choose
    // a single random thread to wake up and that is not the behavior we want.
    // If we want to cancel like in RequestClose below, we want all waiting
    // threads to exit, not just one.

    virtual int RequestClose()
    {
        LOG_INFO("%p: RequestClose", this);
        MutexAutoLock lock(&mutex);
        stop_thread = true;
        for(std::vector<VPLHttp2*>::iterator httpIter = http_handles_to_cancel.begin();
            httpIter != http_handles_to_cancel.end();
            ++httpIter)
        {
            (*httpIter)->Cancel();
        }
        for(std::vector<VCSArchiveOperation*>::iterator aoIter =
                archive_operations_for_async_cancel.begin();
            aoIter != archive_operations_for_async_cancel.end();
            ++aoIter)
        {
            (*aoIter)->Cancel();
        }
        for(std::vector<VCSFileUrlOperation*>::iterator fuoIter =
                file_url_operation_for_async_cancel.begin();
            fuoIter != file_url_operation_for_async_cancel.end();
            ++fuoIter)
        {
            (*fuoIter)->Cancel();
        }
        return VPLCond_Broadcast(&work_to_do_cond_var);  // See "NoteA:" above
    }

    virtual int Join()
    {
        int rc;
        LOG_INFO("%p: Join worker", this);
        rc = VPLDetachableThread_Join(&worker_thread);
        LOG_INFO("%p: Join worker Complete:%d", this, rc);

        bool deferredUploadThreadInitCopy;
        {
            MutexAutoLock lock(&mutex);
            deferredUploadThreadInitCopy = deferredUploadThreadInit;
            deferredUploadThreadInit = false;
        }
        if (deferredUploadThreadInitCopy) {
            LOG_INFO("%p: Join deferredUploadThread", this);
            rc = VPLDetachableThread_Join(&deferredUploadThread);
            LOG_INFO("%p: Join deferredUploadThread complete:%d", this, rc);
        }
        return rc;
    }

    virtual int Pause(bool blocking)
    {
        LOG_INFO("%p: Pause block(%d)", this, (int)blocking);
        MutexAutoLock lock(&mutex);
        allowed_to_run = false;

        {   // Ensure that we jump any waiting state into the "Pause" state.
            int rc = VPLCond_Broadcast(&work_to_do_cond_var);  // See "NoteA" above
            if(rc != 0) {
                LOG_ERROR("%p: VPLCond_Broadcast:%d", this, rc);
            }
            if (!blocking) {
                return rc;
            }
        }

        ASSERT(blocking==true);

        if(getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD) {
            if (worker_loop_paused_or_stopped &&
                deferred_upload_loop_paused_or_stopped)
            {  // Already paused, success
                return 0;
            }
        } else if (getSyncDbFamily(type) == SYNC_DB_FAMILY_DOWNLOAD) {
            if (worker_loop_paused_or_stopped) { // Already paused
                return 0;
            }
        } else {
            FAILED_ASSERT("Blocking pause for syncType(%d) not supported", type);
        }

        if (worker_loop_wait_for_pause_sem != NULL ||
            deferred_upload_loop_wait_for_pause_sem != NULL)
        {
            LOG_ERROR("%p: Another thread already waiting for pause(%p, %p). "
                      "Currently support only 1 pause-wait at a time.",
                      this, worker_loop_wait_for_pause_sem,
                      deferred_upload_loop_wait_for_pause_sem);
            return SYNC_AGENT_ERR_NOT_IMPL;
        }
        int rv = 0;

        // Pause worker loop first, as the worker loop generates work for the deferred_upload_loop.
        if (!worker_loop_paused_or_stopped)
        {   // Need to wait for pause -- create semaphore that is to be notified
            // when pause happens.
            int rc;
            VPLSem_t waitForPause;
            rc = VPLSem_Init(&waitForPause, 1, 0);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Init:%d", this, rc);
                return rc;
            }
            worker_loop_wait_for_pause_sem = &waitForPause;
            lock.UnlockNow();

            LOG_INFO("%p: Waiting for pause(%p)", this, &waitForPause);
            rc = VPLSem_Wait(&waitForPause);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Wait:%d", this, rc);
                if (rv==0) {rv = rc;}
                // continue to clean up semaphore
            }
            LOG_INFO("%p: Wait for pause(%p) done", this, &waitForPause);

            lock.Relock(&mutex);
            worker_loop_wait_for_pause_sem = NULL;
            rc = VPLSem_Destroy(&waitForPause);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Destroy:%d", this, rc);
                if (rv==0) {rv = rc;}
            }
        }

        if (deferred_upload_loop_wait_for_pause_sem != NULL)
        {  // Need to check again because we may have released the lock.
            LOG_ERROR("%p: Another thread already waiting for deferred pause(%p). "
                      "Currently support only 1 pause-wait at a time.",
                      this, deferred_upload_loop_wait_for_pause_sem);
            return SYNC_AGENT_ERR_NOT_IMPL;
        }
        if (deferredUploadThreadInit && !deferred_upload_loop_paused_or_stopped)
        {   // Need to wait for pause -- create semaphore that is to be notified
            // when pause happens.
            int rc;
            VPLSem_t waitForPause;
            rc = VPLSem_Init(&waitForPause, 1, 0);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Init:%d", this, rc);
                return rc;
            }
            deferred_upload_loop_wait_for_pause_sem = &waitForPause;
            lock.UnlockNow();

            LOG_INFO("%p: Waiting for pause(%p)", this, &waitForPause);
            rc = VPLSem_Wait(&waitForPause);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Wait:%d", this, rc);
                if (rv==0) {rv = rc;}
                // continue to clean up semaphore
            }
            LOG_INFO("%p: Wait for pause(%p) done", this, &waitForPause);

            lock.Relock(&mutex);
            deferred_upload_loop_wait_for_pause_sem = NULL;
            rc = VPLSem_Destroy(&waitForPause);
            if (rc != 0) {
                LOG_ERROR("%p: VPLSem_Destroy:%d", this, rc);
                if (rv==0) {rv = rc;}
            }
        }

        // Sanity checks (should be asserts, but we don't compile asserts out)
        ASSERT(worker_loop_wait_for_pause_sem==NULL);
        ASSERT(deferred_upload_loop_wait_for_pause_sem==NULL);
        if (allowed_to_run)
        {   // Resume() should have prevented this case.
            LOG_ERROR("%p: Resume() called during waitForPause -- allowed_to_run", this);
            if (rv==0) {rv = SYNC_AGENT_ERR_RESUME_CALLED_DURING_WAIT_FOR_PAUSE;}
        }
        if (!worker_loop_paused_or_stopped)
        {   // Resume() should have prevented this case.
            LOG_ERROR("%p: Resume() called during waitForPause -- state changed", this);
            if (rv==0) {rv = SYNC_AGENT_ERR_RESUME_CALLED_DURING_WAIT_FOR_PAUSE;}
        }
        if (!deferred_upload_loop_paused_or_stopped)
        {   // Resume() should have prevented this case.
            LOG_ERROR("%p: Resume() called during waitForPause -- state changed", this);
            if (rv==0) {rv = SYNC_AGENT_ERR_RESUME_CALLED_DURING_WAIT_FOR_PAUSE;}
        }
        return rv;
    }

    virtual int Resume()
    {
        LOG_INFO("%p: Resume", this);
        MutexAutoLock lock(&mutex);

        if(worker_loop_wait_for_pause_sem != NULL) {
            LOG_WARN("%p: Resume() called during waitForPause. "
                     "Blocking caller(%p) responsible for resuming",
                     this, worker_loop_wait_for_pause_sem);
            return SYNC_AGENT_ERR_RESUME_CALLED_DURING_WAIT_FOR_PAUSE;
        }

        if (deferredUploadThreadInit) {
            if(deferred_upload_loop_wait_for_pause_sem != NULL) {
                LOG_WARN("%p: Resume() called during waitForPause. "
                         "Blocking caller(%p) responsible for resuming",
                         this, deferred_upload_loop_wait_for_pause_sem);
                return SYNC_AGENT_ERR_RESUME_CALLED_DURING_WAIT_FOR_PAUSE;
            }
            deferred_upload_loop_paused_or_stopped = false;
        }
        worker_loop_paused_or_stopped = false;

        allowed_to_run = true;
        int rv = VPLCond_Broadcast(&work_to_do_cond_var);  // See "NoteA" above
        if(rv != 0) {
            LOG_ERROR("%p:VPLCond_Broadcast:%d", this, rv);
        }

        {   // In our current design, only 1 thread can wait on a threadpool-notifier semaphore.
            // Assert otherwise.  Otherwise, if more threads need to wait on the
            // notifier, we need to use a broadcast condition variable rather than semaphore.
            if(getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD) {
                ASSERT(!(useUploadThreadPool() && useDeferredUploadThreadPool()));
            }
        }

        if (threadPool != NULL &&
            ((getSyncDbFamily(type) == SYNC_DB_FAMILY_DOWNLOAD &&  // to avoid assert
                    useDownloadThreadPool()) ||
             (getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD &&  // to avoid assert
                     useUploadThreadPool()) ||
             (getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD &&  // to avoid assert
                     useDeferredUploadThreadPool())))
        {
            int rcB = VPLSem_Post(&threadPoolNotifier);
            if (rcB != 0 && rcB != VPL_ERR_MAX) {
                LOG_ERROR("%p: VPLSem_Post(%p):%d",
                          this, &threadPoolNotifier, rcB);

                if (rv == 0) {
                    rv = rcB;
                }
            }
        }
        return rv;  // Return first error encountered.
    }

    virtual int LookupComponentByPath(const std::string& sync_config_relative_path,
                                      u64& component_id__out,
                                      u64& revision__out,
                                      bool& is_on_acs__out)
    {
        component_id__out = 0;
        revision__out = 0;
        is_on_acs__out = false;
        int rv = 0;

        MutexAutoLock lock(&lookupDbMutex);
        if (lookupDbInit) {
            SCRelPath parent = Util_getParent(sync_config_relative_path);
            std::string name = Util_getChild(sync_config_relative_path);
            SCRow_syncHistoryTree lookupEntry;
            int rc;

            rc = lookupDb.syncHistoryTree_get(parent.str(),
                                              name,
                                              lookupEntry);
            if(rc != 0) {
                if(rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_ERROR("lookup(%s):%d", sync_config_relative_path.c_str(), rc);
                }
                rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
                goto end;
            }
            if(lookupEntry.is_dir) {
                LOG_WARN("is_dir(%s)", sync_config_relative_path.c_str());
                rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
                goto end;
            }
            if(!lookupEntry.comp_id_exists ||
               !lookupEntry.revision_exists)
            {
                LOG_WARN("Missing fields(%s), (%d, %d)",
                          sync_config_relative_path.c_str(),
                          (int)lookupEntry.comp_id_exists,
                          (int)lookupEntry.revision_exists);
                rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
                goto end;
            }
            // success
            component_id__out = lookupEntry.comp_id;
            revision__out = lookupEntry.revision;
            is_on_acs__out = lookupEntry.is_on_acs;
        } else {
            LOG_ERROR("dblookup not init");
            rv = CCD_ERROR_NOT_INIT;
        }
     end:
        return rv;
    }

    virtual int LookupAbsPath(u64 component_id,
                              u64 revision,
                              std::string& absolute_path__out,
                              u64& local_modify_time__out,
                              std::string& hash__out)
    {
        absolute_path__out.clear();
        local_modify_time__out = 0;
        int rv = 0;

        MutexAutoLock lock(&lookupDbMutex);
        if (lookupDbInit) {
            int rc;
            SCRow_syncHistoryTree lookupEntry;
            rc = lookupDb.syncHistoryTree_getByCompId(component_id,
                                                      lookupEntry);
            if (rc != 0) {
                // Bug 16681: Since we don't record the row in the SyncHistoryTree until after we
                // POST filemetadata, SYNC_AGENT_DB_ERR_ROW_NOT_FOUND is ambiguous.  It may
                // mean "newly-created" (in which case we want to send a transient error), or it
                // may mean deleted or never-existed (which should be a permanent error).
                // Pass up the error code as is and Handler_vcs_archive will consider it as a transient error.
                LOG_WARN("syncHistoryTree_getByCompId("FMTu64"):%d",
                          component_id, rc);
                rv = rc;
                goto end;
            }

            if (!lookupEntry.comp_id_exists ||
                !lookupEntry.revision_exists ||
                lookupEntry.is_dir)
            {
                // !lookupEntry.revision_exists can happen in TwoWaySync if we learned of the file
                // from VCS but have not downloaded it yet. 
                // Return SYNC_CONFIG_ERR_REVISION_NOT_READY so that Handler_vcs_archive knows to
                // return a transient error to the client.
                LOG_WARN("Entry not currently servable:("FMTu64"):%d,%d,%d,%s,%s)",
                          component_id,
                          lookupEntry.comp_id_exists,
                          lookupEntry.revision_exists,
                          lookupEntry.is_dir,
                          lookupEntry.parent_path.c_str(),
                          lookupEntry.name.c_str());
                rv = SYNC_CONFIG_ERR_REVISION_NOT_READY;
                goto end;
            }

            if (revision < lookupEntry.revision) {
                LOG_WARN("Asked for obsolete revision:("FMTu64",%s,%s):"FMTu64" < "FMTu64,
                          component_id,
                          lookupEntry.parent_path.c_str(),
                          lookupEntry.name.c_str(),
                          revision,
                          lookupEntry.revision);
                rv = SYNC_CONFIG_ERR_REVISION_IS_OBSOLETE;
                goto end;
            } else if (revision > lookupEntry.revision) {
                // Bug 16681: This can happen if the apply changelog thread has called VCS POST filemetadata
                // but hasn't committed the DB write yet.
                // Return SYNC_CONFIG_ERR_REVISION_NOT_READY so that Handler_vcs_archive knows to
                // return a transient error to the client.
                LOG_WARN("Asked for newer revision than we can serve:("FMTu64",%s,%s):"FMTu64" > "FMTu64,
                          component_id,
                          lookupEntry.parent_path.c_str(),
                          lookupEntry.name.c_str(),
                          revision,
                          lookupEntry.revision);
                rv = SYNC_CONFIG_ERR_REVISION_NOT_READY;
                goto end;
            }

            {
                SCRelPath scRelPath = getRelPath(lookupEntry);
                AbsPath absPath = getAbsPath(scRelPath);

                absolute_path__out = absPath.str();
                if(lookupEntry.local_mtime_exists) {
                    local_modify_time__out = lookupEntry.local_mtime;
                }
                if(!lookupEntry.hash_value.empty()) {
                    hash__out = lookupEntry.hash_value;
                }
            }

        } else {
            LOG_ERROR("lookupDb not init");
            rv = CCD_ERROR_NOT_INIT;
        }
     end:
        return rv;
    }


    /// Update the status to SYNC_CONFIG_STATUS_SCANNING (but only if needed).
    void updateStatusDueToScanRequest()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        // As per the API, we cannot transition from SYNCING to SCANNING until
        // "all downloads and uploads were completed successfully".
        if (workerLoopStatus != SYNC_CONFIG_STATUS_SYNCING) {
            setStatus(SYNC_CONFIG_STATUS_SCANNING);
        }
    }

    /// Called when there is a local change that possibly needs to be uploaded.
    /// This is only needed on platforms that don't support filesystem monitoring.
    /// On platforms that support filesystem monitoring, the monitoring will automatically
    /// be registered and hooked up within #CreateSyncConfig().
    // Should be called:
    // - immediately after CreateSyncConfig.
    // - whenever FileMonitor overflows.
    virtual int RequestFullLocalScan()
    {
        if(getSyncDbFamily(type) == SYNC_DB_FAMILY_DOWNLOAD) {
            LOG_DEBUG("%p: Ignore RequestFullLocalScan for SYNC_FAMILY_DOWNLOAD(%d)",
                      this, (int)type);
            return 0;
        }
        LOG_INFO("%p: RequestFullLocalScan", this);
        MutexAutoLock lock(&mutex);
        full_local_scan_requested = true;
        updateStatusDueToScanRequest();
        return VPLCond_Broadcast(&work_to_do_cond_var);  // See "NoteA" above
    }

    // Should be called:
    // - when a FileMonitor change event arrives.
    // - (for OS without file monitor) when the app tells us that it made a change
    virtual int ReportLocalChange(const std::string& path)
    {
        if(getSyncDbFamily(type) == SYNC_DB_FAMILY_DOWNLOAD) {
            LOG_DEBUG("%p: Ignore ReportLocalChange for SYNC_FAMILY_DOWNLOAD(%d)",
                      this, (int)type);
            return 0;
        }
        LOG_INFO("%p: ReportLocalChange \"%s\"", this, path.c_str());
        MutexAutoLock lock(&mutex);
        incrementalLocalScanPaths.insert(path);
        updateStatusDueToScanRequest();
        return VPLCond_Broadcast(&work_to_do_cond_var);  // See "NoteA" above
    }

    // Should be called:
    // - when ANS informs CCD of a VCS change
    virtual int ReportRemoteChange()
    {
        if(getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD) {
            LOG_DEBUG("%p: Ignore ReportRemoteChange for SYNC_FAMILY_UPLOAD(%d)",
                      this, (int)type);
            return 0;
        }
        LOG_INFO("%p: ReportRemoteChange", this);
        MutexAutoLock lock(&mutex);
        setDownloadScanRequested(true);
        updateStatusDueToScanRequest();
        return VPLCond_Broadcast(&work_to_do_cond_var);  // See "NoteA" above
    }

    // Should be called:
    // - when ANS reconnects
    // - when a user requests a manual sync
    virtual int ReportPossibleRemoteChange()
    {
        if(getSyncDbFamily(type) == SYNC_DB_FAMILY_UPLOAD) {
            LOG_DEBUG("%p: Ignore ReportPossibleRemoteChange for SYNC_FAMILY_UPLOAD(%d)",
                      this, (int)type);
            return 0;
        }
        LOG_INFO("%p: ReportPossibleRemoteChange", this);
        MutexAutoLock lock(&mutex);
        dataset_version_check_requested = true;
        updateStatusDueToScanRequest();
        return VPLCond_Broadcast(&work_to_do_cond_var);  // See "NoteA" above
    }

#define SYNC_CONFIG_FORCE_RETRY_MIN_DELAY_IN_SECS    10
#define SYNC_CONFIG_FORCE_RETRY_MAX_DELAY_IN_SECS    (15 * 60)
    virtual int ReportArchiveStorageDeviceAvailability(bool is_online)
    {
        // NOTE: If this ever changes to include ONE_WAY_UP, be sure to call clearForceRetry()
        // in the OneWayUp impl.
        if(type != SYNC_TYPE_ONE_WAY_DOWNLOAD && type != SYNC_TYPE_TWO_WAY) {
            return 0;
        }

        u64 timeoutSecs = SYNC_CONFIG_FORCE_RETRY_MIN_DELAY_IN_SECS;
        if (is_online) {
            // To avoid dead lock, query DB out side of lock(&mutex)
            MutexAutoLock lock(&lightQueryDbMutex);
            if (lightQueryDbInit) {
                SCRow_uploadChangeLog uploadEntry;
                SCRow_downloadChangeLog downloadEntry;
                u64 count = 1;
                int rc = localDb.uploadChangeLog_getErr(INT64_MAX, /*out*/uploadEntry);
                if (rc == 0 && uploadEntry.upload_err_count > 1) {
                    count = uploadEntry.upload_err_count;
                } else if (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_ERROR("%p: uploadChangeLog_getErr failed: %d", this, rc);
                }
                rc = localDb.downloadChangeLog_getErrDownload(INT64_MAX, /*out*/downloadEntry);
                if (rc == 0 && downloadEntry.download_err_count > 1) {
                    count = MAX(count, downloadEntry.download_err_count);
                } else if (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_ERROR("%p: downloadChangeLog_getErrDownload failed: %d", this, rc);
                }
                timeoutSecs = count * SYNC_CONFIG_FORCE_RETRY_MIN_DELAY_IN_SECS;
                timeoutSecs = MIN(timeoutSecs, (u64)SYNC_CONFIG_FORCE_RETRY_MAX_DELAY_IN_SECS);
            } else {
                LOG_ERROR("lightQueryDB not init");
            }
        }
        MutexAutoLock lock(&mutex);
        is_archive_storage_device_available = is_online;
        if (is_online) {
            // Force retry sooner. Add some small delay as a precaution in case there is ever
            // an extreme online/offline toggling bug.
            LOG_INFO("%p: ReportArchiveStorageDeviceAvailability(true), force error retry in "FMTu64" secs",
                     this, timeoutSecs);
            forceErrorRetry(VPLTime_FromSec(timeoutSecs));
        } else {
            LOG_INFO("%p: ReportArchiveStorageDeviceAvailability(false)", this);
        }
        return VPLCond_Broadcast(&work_to_do_cond_var);
    }

    bool hasScanToDo()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        return full_local_scan_requested || dataset_version_check_requested || download_scan_requested ||
                (incrementalLocalScanPaths.size() > 0);
    }

    /// Should the primary worker be woken up?
    /// This is only relevant when the primary worker thread is waiting for work to do (at the
    /// bottom of workerLoop()).
    bool hasWorkToDo()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        return hasScanToDo() || isTimeToRetryErrors();
    }

    bool checkForWorkToDo()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        bool newHasWorkToDo = hasWorkToDo();
        bool newRemoteScanPending = isRemoteScanPending();
        updateStatusWorkerLoop(workerLoopStatus,
                               workerLoopStatusHasError,
                               newHasWorkToDo,
                               workerUploadsRemaining,
                               workerDownloadsRemaining,
                               newRemoteScanPending);
        return newHasWorkToDo;
    }

    void updateStatusWorkerLoop(SyncConfigStatus newStatus,
                                bool newHasError,
                                bool newHasWorkToDo,
                                u32 newUploadsRemaining,
                                u32 newDownloadsRemaining,
                                bool newRemoteScanPending)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        workerLoopStatus = newStatus;
        workerLoopStatusHasError = newHasError;
        workerLoopStatusHasWorkToDo = newHasWorkToDo;

        if(deferredUploadThreadInit) {
            calcDeferredUploadLoopStatus(localDb);
        } else {
            updateStatusHelper(workerLoopStatus,
                               workerLoopStatusHasError,
                               workerLoopStatusHasWorkToDo,
                               newUploadsRemaining,
                               newDownloadsRemaining,
                               newRemoteScanPending);
        }
    }

    void calcDeferredUploadLoopStatus(SyncConfigDb& db)
    {
        int rc = 0;

        bool hasWorkToDo;
        bool hasError;
        SCRow_deferredUploadJobSet entry;
        rc = db.deferredUploadJobSet_getAfterRowId(0, /*OUT*/entry);
        if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            hasWorkToDo = false;
        } else {
            hasWorkToDo = true;
        }
        rc = db.deferredUploadJobSet_getErrAfterRowId(
                0,
                SQLITE3_MAX_ROW_ID,
                /*OUT*/ entry);
        if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            hasError = false;
        } else {
            hasError = true;
        }

        ASSERT(VPLMutex_LockedSelf(&mutex));
        deferUploadLoopStatusHasWorkToDo = hasWorkToDo;
        deferUploadLoopStatusHasError = hasError;
        updateStatusDeferredLoop();
    }

    void updateStatusDeferredLoop()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));

        SyncConfigStatus aggregateStatus = workerLoopStatus;
        bool aggregateHasWorkToDo = workerLoopStatusHasWorkToDo;
        bool aggregateHasError = workerLoopStatusHasError;

        if (deferUploadLoopStatusHasWorkToDo) {
            aggregateStatus = SYNC_CONFIG_STATUS_SYNCING;
            aggregateHasWorkToDo = true;
        }
        if (deferUploadLoopStatusHasError) {
            aggregateHasError = true;
        }

        updateStatusHelper(aggregateStatus,
                           aggregateHasError,
                           aggregateHasWorkToDo,
                           0, 0, false);
    }

    void updateStatusHelper(SyncConfigStatus newStatus,
                            bool newHasError,
                            bool newHasWorkToDo,
                            u32 newUploadsRemaining,
                            u32 newDownloadsRemaining,
                            bool newRemoteScanPending)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        // Did anything change from our last report?
        if ((newStatus != prevSyncConfigStatus) ||
            (newHasError != prevHasError) ||
            (newHasWorkToDo != prevHasWorkToDo) ||
            (newUploadsRemaining != prevUploadsRemaining) ||
            (newDownloadsRemaining != prevDownloadsRemaining))
        {
            // Call the callback first.
            if (event_cb != NULL) {
                SyncConfigEventStatusChange event(*this, callback_context,
                        user_id, dataset.id,
                        newStatus, newHasError, newHasWorkToDo,
                        prevSyncConfigStatus, prevHasError, prevHasWorkToDo,
                        newUploadsRemaining, newDownloadsRemaining, newRemoteScanPending,
                        prevUploadsRemaining, prevDownloadsRemaining, prevRemoteScanPending);
                event_cb(event);
            }
            LOG_INFO("%p: Updating (status,hasEr,workToDo,UpRmn,"
                    "DlRmn,RmtScnPnd) from (%d,%d,%d,%d,%d,%d)"
                    " to (%d,%d,%d,%d,%d,%d)",
                    this,
                    prevSyncConfigStatus, prevHasError, prevHasWorkToDo,
                    prevUploadsRemaining, prevDownloadsRemaining, prevRemoteScanPending,
                    newStatus, newHasError, newHasWorkToDo,
                    newUploadsRemaining, newDownloadsRemaining, newRemoteScanPending);
            // Now, overwrite the prev state.
            prevSyncConfigStatus = newStatus;
            prevHasError = newHasError;
            prevHasWorkToDo = newHasWorkToDo;

            // Following members are only meaningful for Syncbox.
            prevUploadsRemaining = newUploadsRemaining;
            prevDownloadsRemaining = newDownloadsRemaining;
            prevRemoteScanPending = newRemoteScanPending;
        }
    }

    SyncConfigEventDetailType convertDetailType(bool isDeletion, bool isDir, bool isNewFile)
    {
        SyncConfigEventDetailType detailType = SYNC_CONFIG_EVENT_DETAIL_NONE;
        if (isDir) {
            if (isDeletion) {
                detailType = SYNC_CONFIG_EVENT_DETAIL_FOLDER_DELETE;
            } else {
                detailType = SYNC_CONFIG_EVENT_DETAIL_FOLDER_CREATE;
            }
        } else {
            if (isDeletion) {
                detailType = SYNC_CONFIG_EVENT_DETAIL_FILE_DELETE;
            } else {
                if (isNewFile) {
                    detailType = SYNC_CONFIG_EVENT_DETAIL_NEW_FILE;
                } else {
                    detailType = SYNC_CONFIG_EVENT_DETAIL_MODIFIED_FILE;
                }
            }
        }
        return detailType;
    }
    
    void reportFileDownloaded(bool isDeletion, const SCRelPath& relPath, bool isDir,
            bool isNewFile)
    {
        if (event_cb != NULL) {
            AbsPath absPath = getAbsPath(relPath);
            SyncConfigEventDetailType detailType = SYNC_CONFIG_EVENT_DETAIL_NONE;
            u64 event_time = 0;
            SyncDbFamily family = getSyncDbFamily(type);
            if (family == SYNC_DB_FAMILY_TWO_WAY) {
                event_time = VPLTime_GetTime();
                detailType = convertDetailType(isDeletion, isDir, isNewFile);
            }
            SyncConfigEventEntryDownloaded event(*this, callback_context,
                    user_id, dataset.id, isDeletion, relPath.str(), absPath.str(),
                    detailType, event_time, std::string(""));
            event_cb(event);
        }
    }

    void setStatus(SyncConfigStatus newStatus)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        updateStatusWorkerLoop(newStatus, hasRetryErrors(), hasWorkToDo(),
                               workerUploadsRemaining, workerDownloadsRemaining,
                               isRemoteScanPending());
    }

    void SetStatus(SyncConfigStatus newStatus)
    {
        MutexAutoLock lock(&mutex);
        setStatus(newStatus);
    }

    virtual int GetSyncStatus(
            SyncConfigStatus& status__out,
            bool& has_error__out,
            bool& work_to_do__out,
            u32& uploads_remaining__out,
            u32& downloads_remaining__out,
            bool& remote_scan_pending__out)
    {
        MutexAutoLock lock(&mutex);
        {
            checkForWorkToDo();
            status__out = prevSyncConfigStatus;
            has_error__out = prevHasError;
            // TODO: This isn't very useful to the caller.  To make it useful, we
            // would probably want to "OR" it with !(are all worker thread sleeping).
            // The original goal was to let a scheduler decide if all higher-priority SyncConfigs
            // are completely idle, which is only the case when the primary worker and deferred worker
            // (if applicable) are both waiting for something to do.
            work_to_do__out = prevHasWorkToDo;

            // Following statuses are only valid for SYNC_FEATURE_SYNCBOX:
            uploads_remaining__out = prevUploadsRemaining;
            downloads_remaining__out = prevDownloadsRemaining;
            remote_scan_pending__out = prevRemoteScanPending;
        }
        return 0;
    }

    virtual u64 GetUserId()
    {
        return user_id;
    }

    virtual u64 GetDatasetId()
    {
        return dataset.id;
    }

    virtual std::string GetLocalDir()
    {
        return local_dir.str();
    }

    std::string convertCaseByPolicy(const std::string& path) const
    {
        if (sync_policy.case_insensitive_compare) {
            return Util_UTF8Upper(path.c_str());
        }
        return path;
    }

    bool isValidDirEntry(const std::string& entry)
    {
        std::string dirEntry = convertCaseByPolicy(entry);
        if ((dirEntry.size() == 0) ||
            (dirEntry == "..") ||
            (dirEntry == ".") ||
            (dirEntry == convertCaseByPolicy(SYNC_TEMP_DIR)) ||
            (dirEntry == convertCaseByPolicy(DESKTOP_INI)))
        {
            return false;
        }
        if(dirEntry.find_first_of("\\/:*?\"<>|") != string::npos) {
            // http://msdn.microsoft.com/en-us/library/aa365247(v=vs.85).aspx#naming_conventions
            return false;
        }
        return true;
    }

    static bool isTransientError(int error)
    {
        if(error == CCD_ERROR_TRANSIENT)
        {   // HTTP transient error
            return true;
        }
        if (error == CCD_ERROR_ARCHIVE_DEVICE_OFFLINE) 
        {
            // The online status of archive device may not be updated.
            // Consider this transient for now.
            // TODO: Bug 17706: Eventually, it would be better to use ReportArchiveStorageDeviceAvailability()
            //   to instead trigger the retry when the device comes online.
            //   We will want that anyway.
            return true;
        }
        // See http://www.ctbg.acer.com/wiki/index.php/RESTful_API_External_Error_Codes#RESTful_API_External_Error_Code_Ranges
        // See #VCS_ERR_INTERNAL_SERVER_ERROR and friends in vcs_errors.hpp for the specific error codes that the client knows about.
        if(error >= -35799 && error <= -35750)
        {   // VCS Transient error
            return true;
        }
        if(error >= -35999 && error <= -35950)
        {  // ACS Transient error
            return true;
        }
        if(error == VPL_ERR_HOSTNAME ||
           error == VPL_ERR_SOCKET ||
           error == VPL_ERR_CONNABORT ||
           error == VPL_ERR_UNREACH ||
           error == VPL_ERR_NETDOWN ||
           error == VPL_ERR_NETRESET ||
           error == VPL_ERR_CONNREFUSED ||
           error == VPL_ERR_TIMEOUT ||
           error == VPL_ERR_HTTP_ENGINE ||
           error == VPL_ERR_IO)
        {
            return true;
        }
        if (error == VCS_ERR_DS_DATASET_NOT_FOUND) // This may cause by log out from CloudPC,
                                                   // we assume that user may login again so able to retry transient
        {
            return true;
        }
        return false;
    }
#if VPLFS_MONITOR_SUPPORTED
    // Callback for FileMonitorDelayQCallback
    static void fnFileMonitorDelayQCallback(VPLFS_MonitorHandle handle,
                                            void* eventBuffer,
                                            int eventBufferSize,
                                            int error,
                                            void* ctx)
    {
        SyncConfigImpl* scImpl = static_cast<SyncConfigImpl*>(ctx);

        if(error == VPLFS_MonitorCB_OVERFLOW) {
            LOG_INFO("%p: Overflow occurred.", scImpl);
            scImpl->RequestFullLocalScan();
        }else if(error == VPLFS_MonitorCB_OK) {
            int bufferSize = 0;
            void* currEventBuf = eventBuffer;
            while(bufferSize < eventBufferSize) {
                VPLFS_MonitorEvent* monitorEvent = (VPLFS_MonitorEvent*) currEventBuf;
                switch(monitorEvent->action) {
                case VPLFS_MonitorEvent_FILE_ADDED:
                    LOG_INFO("%p: File added:%s", scImpl, monitorEvent->filename);
                    scImpl->ReportLocalChange(monitorEvent->filename);
                    break;
                case VPLFS_MonitorEvent_FILE_REMOVED:
                    LOG_INFO("%p: File removed:%s", scImpl, monitorEvent->filename);
                    scImpl->ReportLocalChange(monitorEvent->filename);
                    break;
                case VPLFS_MonitorEvent_FILE_MODIFIED:
                    LOG_INFO("%p: File modified:%s", scImpl, monitorEvent->filename);
                    scImpl->ReportLocalChange(monitorEvent->filename);
                    break;
                case VPLFS_MonitorEvent_FILE_RENAMED:
                    LOG_INFO("%p: File renamed:%s->%s",
                             scImpl, monitorEvent->filename, monitorEvent->moveTo);
                    scImpl->ReportLocalChange(monitorEvent->filename);
                    scImpl->ReportLocalChange(monitorEvent->moveTo);
                    break;
                case VPLFS_MonitorEvent_NONE:
                default:
                    LOG_INFO("%p: action not supported:%d, %s",
                             scImpl, monitorEvent->action, monitorEvent->filename);
                    break;
                }
                if(monitorEvent->nextEntryOffsetBytes == 0) {
                    break;
                }

                currEventBuf = (u8*)currEventBuf + monitorEvent->nextEntryOffsetBytes;
                bufferSize +=   monitorEvent->nextEntryOffsetBytes;
            }
        }else{
            LOG_ERROR("%p: Unhandled error:%d.", scImpl, error);
            scImpl->RequestFullLocalScan();
        }
    }
#endif

private:
    virtual int updateUploadsRemaining() = 0;
    virtual int updateDownloadsRemaining() = 0;

    virtual void setDownloadScanRequested(bool value) = 0;
    virtual void setDownloadScanError(bool value) = 0;

    virtual bool isRemoteScanPending() = 0;

    /// @return true if the worker thread should completely stop.
    bool clearHttpHandleAndCheckForPauseStop(VPLHttp2& httpHandle,
                                             bool pauseMode,
                                             VPLTime_t pauseTime)
    {
        MutexAutoLock lock(&mutex);
        clearHttpHandle(httpHandle);
        bool result = checkForPauseStop(pauseMode, pauseTime);
        return result;
    }

    /// @param transientErrPauseMode When true, indicates the desire to pause
    ///   (effectively sleep) for transientErrPauseTime time before trying the
    ///   operation again.
    /// @param transientErrPauseTime Only valid when transientErrPauseMode is true,
    ///   and indicates the amount of pause time desired before retrying the
    ///   operation.
    /// @param withinDeferredUpload When within deferred upload loop, we want to
    ///   notify the deferredUploadLoop-specific semaphore that we are paused.
    /// @param withinTask When within task, we do not actually want to pause, since
    ///   a general threadPool thread could become occupied and not doing work
    ///   for an extended length of time.
    /// @return true if the worker thread should completely stop.
    bool checkForPauseStopHelper(bool transientErrPauseMode,
                                 VPLTime_t transientErrPauseTime,
                                 bool withinDeferredUpload,
                                 bool withinTask)
    {
        MutexAutoLock lock(&mutex);
        VPLTime_t pauseUntil = VPLTime_GetTimeStamp() + transientErrPauseTime;
      top:
        if (stop_thread) {
            return true;
        }
        if (!allowed_to_run) { // SyncConfig API client requested Pause().

            if (withinTask) {
                return false;
            } else if(withinDeferredUpload) {
                deferred_upload_loop_paused_or_stopped = true;
                // If the pause request was blocking, signal the waiting thread now.
                if (deferred_upload_loop_wait_for_pause_sem != NULL) {
                    int rc = VPLSem_Post(deferred_upload_loop_wait_for_pause_sem);
                    if (rc != 0) {
                        LOG_ERROR("%p: VPLSem_Post(%p):%d",
                                  this, deferred_upload_loop_wait_for_pause_sem, rc);
                    }
                }
                VPLCond_TimedWait(&work_to_do_cond_var, &mutex, VPL_TIMEOUT_NONE);
                deferred_upload_loop_paused_or_stopped = false;
            } else {
                // Bug 16261: Commit transaction before pausing, so that the deferred upload
                //   thread won't be blocked waiting for write access to the DB.
                CHECK_END_TRANSACTION(true);

                worker_loop_paused_or_stopped = true;
                // If the pause request was blocking, signal the waiting thread now.
                if (worker_loop_wait_for_pause_sem != NULL) {
                    int rc = VPLSem_Post(worker_loop_wait_for_pause_sem);
                    if (rc != 0) {
                        LOG_ERROR("%p: VPLSem_Post(%p):%d",
                                  this, worker_loop_wait_for_pause_sem, rc);
                    }
                }
                VPLCond_TimedWait(&work_to_do_cond_var, &mutex, VPL_TIMEOUT_NONE);
                worker_loop_paused_or_stopped = false;
            }
            goto top;
        } else if (transientErrPauseMode) { // Temporary pause due to transient error.
            VPLCond_TimedWait(&work_to_do_cond_var, &mutex,
                              VPLTime_DiffClamp(pauseUntil, VPLTime_GetTimeStamp()));
            if (VPLTime_GetTimeStamp() > pauseUntil) {
                transientErrPauseMode = false;
            }
            goto top;
        }
        return false;
    }
}; // end class SyncConfigImpl

#include "SyncConfigTwoWayImpl.fragment.cpp"
#include "SyncConfigOneWayUpImpl.fragment.cpp"
#include "SyncConfigOneWayDownImpl.fragment.cpp"

SyncConfig* CreateSyncConfig(
        u64 user_id,
        const VcsDataset& dataset,
        SyncType type,
        const SyncPolicy& sync_policy,
        const std::string& local_dir,
        const std::string& server_dir,
        const DatasetAccessInfo& dataset_access_info,
        SyncConfigThreadPool* thread_pool,
        bool make_dedicated_thread,
        SyncConfigEventCallback event_cb,
        void* callback_context,
        int& err_code__out,
        bool allow_create_db)
{
    ensureThreadLocalsInit(); // Must call this before any SyncConfigs are created.

    SyncConfigImpl* newWorker;
    switch (type) {
        case SYNC_TYPE_TWO_WAY:
        case SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE:
            if(thread_pool) {
                LOG_ERROR("THREAD_POOL defined but not used!!!!!! %p", thread_pool);
            }
            if (dataset_access_info.fileUrlAccess == NULL) {
                LOG_ERROR("dataset_access_info.fileUrlAccess cannot be null for a TWO_WAY SyncConfig");
                err_code__out = SYNC_AGENT_ERR_INVALID_ARG;
                return NULL;
            }
            newWorker = new SyncConfigTwoWayImpl(
                user_id, dataset, type,
                sync_policy, local_dir, server_dir,
                dataset_access_info, event_cb, callback_context);
            break;
        case SYNC_TYPE_ONE_WAY_UPLOAD:
        case SYNC_TYPE_ONE_WAY_UPLOAD_PURE_VIRTUAL_SYNC:
        case SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC:
            newWorker = new SyncConfigOneWayUpImpl(
                user_id, dataset, type,
                sync_policy, local_dir, server_dir,
                dataset_access_info,
                thread_pool, make_dedicated_thread,
                event_cb, callback_context);
            break;
        case SYNC_TYPE_ONE_WAY_DOWNLOAD:
        case SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC:
            newWorker = new SyncConfigOneWayDownImpl(
                user_id, dataset, type,
                sync_policy, local_dir, server_dir,
                dataset_access_info,
                thread_pool, make_dedicated_thread,
                event_cb, callback_context);
            break;
        case SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE_AND_USE_ACS:
        default:
            err_code__out = SYNC_AGENT_ERR_INVALID_ARG;
            return NULL;
    }
    err_code__out = newWorker->init(allow_create_db);
    if (err_code__out < 0) {
        delete newWorker;
        newWorker = NULL;
    }

    return newWorker;
}

void DestroySyncConfig(SyncConfig* syncConfig)
{
    delete syncConfig;
}
