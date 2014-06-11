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
#ifndef __SYNC_CONFIG_WORKER_DB_HPP_2_19_2013__
#define __SYNC_CONFIG_WORKER_DB_HPP_2_19_2013__

#include <vpl_plat.h>
#include <vplu_types.h>

#include "sqlite3.h"

#include <string>
#include <vector>

//=============================================================
//============ Structures Reflecting Db Columns ===============
struct SCRow_admin
{
    u64 row_id;
    u64 schema_version;
    bool device_id_exists;
    u64 device_id;
    bool user_id_exists;
    u64 user_id;
    bool dataset_id_exists;
    u64 dataset_id;
    bool sync_config_id_exists;
    std::string sync_config_id;
    bool last_opened_timestamp_exists;
    u64 last_opened_timestamp;
    bool sync_type_exists;
    u64 sync_type;
    bool migrate_from_sync_type_exists;
    u64 migrate_from_sync_type;

    void clear() {
        row_id = 0;
        schema_version = 0;
        device_id_exists = false;
        device_id = 0;
        user_id_exists = false;
        user_id = 0;
        dataset_id_exists = false;
        dataset_id = 0;
        sync_config_id_exists = false;
        sync_config_id.clear();
        last_opened_timestamp_exists = false;
        last_opened_timestamp = 0;
        sync_type_exists = false;
        sync_type = 0;
        migrate_from_sync_type_exists = false;
        migrate_from_sync_type = 0;
    }
    SCRow_admin(){ clear(); }
};

struct SCRow_needDownloadScan
{
    u64 row_id;
    std::string dir_path; ///< Sync config-relative path, no leading slash, no trailing slash.
    u64 comp_id; ///< Not optional.
    u64 err_count;

    void clear() {
        row_id = 0;
        dir_path.clear();
        comp_id = 0;
        err_count = 0;
    }
    SCRow_needDownloadScan(){ clear(); }
};

struct SCRow_downloadChangeLog
{
    u64 row_id;
    std::string parent_path; ///< Sync config-relative path, no leading slash, no trailing slash.
    std::string name;
    bool is_dir;
    u64 download_action;
    u64 comp_id;
    u64 revision;
    u64 client_reported_mtime;
    bool is_on_acs;
    u64 download_err_count;
    bool download_succeeded;
    u64 copyback_err_count;
    u64 archive_off_err_count;
    std::string hash_value;
    bool file_size_exists;
    u64 file_size;
    bool parent_path_ci_exists;
    std::string parent_path_ci;
    bool name_ci_exists;
    std::string name_ci;

    void clear() {
        row_id = 0;
        parent_path.clear();
        name.clear();
        is_dir = false;
        download_action = 0;
        comp_id = 0;
        revision = 0;
        client_reported_mtime = 0;
        is_on_acs = false;
        download_err_count = 0;
        download_succeeded = false;
        copyback_err_count = 0;
        archive_off_err_count = 0;
        hash_value.clear();
        file_size_exists = false;
        file_size = 0;
        parent_path_ci_exists = false;
        parent_path_ci.clear();
        name_ci_exists = false;
        name_ci.clear();
    }
    SCRow_downloadChangeLog(){ clear(); }
};

struct SCRow_uploadChangeLog
{
    u64 row_id;
    std::string parent_path; ///< Sync config-relative path, no leading slash, no trailing slash.
    std::string name;
    bool is_dir;
    u64 upload_action;
    bool comp_id_exists;
    u64 comp_id;
    bool revision_exists;
    u64 revision;
    u64 upload_err_count;
    std::string hash_value;
    bool file_size_exists;
    u64 file_size;
    bool parent_path_ci_exists;
    std::string parent_path_ci;
    bool name_ci_exists;
    std::string name_ci;

    void clear() {
        row_id = 0;
        parent_path.clear();
        name.clear();
        is_dir = false;
        upload_action = 0;
        comp_id_exists = false;
        comp_id = 0;
        revision_exists = false;
        revision = 0;
        upload_err_count = 0;
        hash_value.clear();
        file_size_exists = false;
        file_size = 0;
        parent_path_ci_exists = false;
        parent_path_ci.clear();
        name_ci_exists = false;
        name_ci.clear();
    }
    SCRow_uploadChangeLog(){ clear(); }
};

/// Invariant: If an entry is in the download change log, that file must also be in
///     the syncHistoryTree.
struct SCRow_syncHistoryTree
{
    std::string parent_path; ///< Sync config-relative path, no leading slash, no trailing slash.
    std::string name;
    bool is_dir;

    /// The file or directory's component ID as reported by VCS. Note that the same path can
    /// have different compIds over time. This field should match the file that is currently
    /// stored on the local filesystem. For directories, we always update this to the latest
    /// compId as soon as we learn it from VCS.
    bool comp_id_exists;
    u64 comp_id;

    /// For two-way sync and one-way download:
    /// VCS revision of the file on local disk.  Only valid for files.
    /// This should only be set after the file has been successfully uploaded or downloaded
    /// (or we determined that the local file is the same as on infra).
    ///
    /// For one-way upload:
    /// VCS revision of the file in the infra.  Only valid for files.
    /// If local_mtime_exists is true, then this is also the VCS revision of the file on the local
    /// disk.
    /// If local_mtime_exists is false, this is the VCS revision to replace when we do the initial
    /// upload from this device.
    bool revision_exists;
    u64 revision;

    /// Modification time of the file on local disk, in microseconds (VPLTime_t).  Only valid for files.
    /// This should only be set after the file has been successfully uploaded or downloaded
    /// (or we determined that the local file is the same as on infra).
    bool local_mtime_exists;
    u64 local_mtime;

    bool last_seen_in_version_exists;
    u64 last_seen_in_version;

    bool version_scanned_exists;
    u64 version_scanned;

    bool is_on_acs;

    std::string hash_value;

    bool file_size_exists;
    u64 file_size;

    bool parent_path_ci_exists;
    std::string parent_path_ci;

    bool name_ci_exists;
    std::string name_ci;

    void clear() {
        parent_path.clear();
        name.clear();
        is_dir = false;
        comp_id_exists = false;
        comp_id = 0;
        revision_exists = false;
        revision = 0;
        local_mtime_exists = false;
        local_mtime = 0;
        last_seen_in_version_exists = false;
        last_seen_in_version = 0;
        version_scanned_exists = false;
        version_scanned = 0;
        is_on_acs = false;
        hash_value.clear();
        file_size_exists = false;
        file_size = 0;
        parent_path_ci_exists = false;
        parent_path_ci.clear();
        name_ci_exists = false;
        name_ci.clear();
    }
    SCRow_syncHistoryTree(){ clear(); }
};

struct SCRow_deferredUploadJobSet {
    u64 row_id;
    bool row_valid;
    std::string parent_path;
    std::string name;
    u64 last_synced_parent_comp_id;
    bool last_synced_comp_id_exists;
    u64 last_synced_comp_id;
    bool last_synced_revision_id_exists;
    u64 last_synced_revision_id;
    u64 local_mtime;
    u64 error_count;
    bool next_error_retry_time_exists;
    u64 next_error_retry_time;
    bool error_reason_exists;
    s64 error_reason;
    bool parent_path_ci_exists;
    std::string parent_path_ci;
    bool name_ci_exists;
    std::string name_ci;

    void clear() {
        row_id = 0;
        row_valid = false;
        parent_path.clear();
        name.clear();
        last_synced_parent_comp_id = 0;
        last_synced_comp_id_exists = false;
        last_synced_comp_id = 0;
        last_synced_revision_id_exists = false;
        last_synced_revision_id = 0;
        local_mtime = 0;
        error_count = 0;
        next_error_retry_time_exists = false;
        next_error_retry_time = 0;
        error_reason_exists = false;
        error_reason = 0;
        parent_path_ci_exists = false;
        parent_path_ci.clear();
        name_ci_exists = false;
        name_ci.clear();
    }
    SCRow_deferredUploadJobSet(){ clear(); }
};

// Archive Storage Device staging area cleanup info
struct SCRow_asdStagingAreaCleanUp
{
    u64 row_id;
    std::string access_url;     // Access URL associated with the file in staging area
    std::string filename;       // Staging file name
    u64 cleanup_time;           // Time when file should be cleaned up

    void clear() {
        row_id = 0;
        access_url.clear();
        cleanup_time = 0;
    }
    SCRow_asdStagingAreaCleanUp(){ clear(); }
};

//============ Structures Reflecting Db Columns ===============
//=============================================================

// SyncTypes of the same db family may upgrade db amongst each other.
enum SyncDbFamily {
    SYNC_DB_FAMILY_TWO_WAY,
    SYNC_DB_FAMILY_UPLOAD,
    SYNC_DB_FAMILY_DOWNLOAD,

    // Note:  Order of enums is important for backwards compatibility -- enums
    //        must match order of first three enums in SyncType
};

enum SyncConfigDb_RowFilter {
    SCDB_ROW_FILTER_ALLOW_ALL,
    SCDB_ROW_FILTER_ALLOW_CREATE_AND_UPDATE,
    SCDB_ROW_FILTER_ALLOW_FILE_DELETE,
    SCDB_ROW_FILTER_ALLOW_DIRECTORY_DELETE
};

#define SCDB_UPLOAD_ACTION_CREATE 1
#define SCDB_UPLOAD_ACTION_UPDATE 2
#define SCDB_UPLOAD_ACTION_DELETE 3
#define SCDB_UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE 4

enum UploadAction_t {
    UPLOAD_ACTION_CREATE = SCDB_UPLOAD_ACTION_CREATE,
    UPLOAD_ACTION_UPDATE = SCDB_UPLOAD_ACTION_UPDATE,
    UPLOAD_ACTION_DELETE = SCDB_UPLOAD_ACTION_DELETE,
    UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE = SCDB_UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE,
};

enum DownloadAction_t {
    DOWNLOAD_ACTION_GET_FILE = 1,
    DOWNLOAD_ACTION_DELETE = 2,
};

class SyncConfigDb
{
public:
    SyncConfigDb(const std::string& workingDir,
                 SyncDbFamily syncDbFamily,
                 u64 userId,
                 u64 datasetId,
                 const std::string& syncConfigId,
                 bool isCaseInsensitive = false); ///< To enable case insensitive file/path names (more expensive).
    ~SyncConfigDb();

    std::string getDbDir() { return m_dbDir; }
    // openDb can be called multiple times.  If the db has already been opened,
    // this function is a no-op, and success will be returned.
    int openDb();
    int openDb(bool &dbMissing_out);
    int openDbReadOnly();
    int openDbNoCreate();
    int closeDb();
    int beginTransaction();
    int beginExclusiveTransaction();
    int commitTransaction();
    int rollbackTransaction();

    int admin_get_schema_version(u64& schema_version_out);
    int admin_get(SCRow_admin& adminInfo_out);
    int admin_set(u64 deviceId,
                  u64 userId,
                  u64 datasetId,
                  const std::string& syncConfigId,
                  u64 lastOpenedTs);
    int admin_set(bool deviceIdExists,
                  u64 deviceId,
                  bool userIdExists,
                  u64 userId,
                  bool datasetIdExists,
                  u64 datasetId,
                  bool syncConfigIdExists,
                  const std::string& syncConfigId,
                  bool lastOpenedTimestampExists,
                  u64 lastOpenedTimestamp,
                  bool syncTypeExists,
                  u64 syncType,
                  bool migrateFromSyncTypeExists,
                  u64 migrateFromSyncType);
    int admin_set_sync_type(u64 syncType,
                            bool migrateFromSyncTypeExists,
                            u64 migrateFromSyncType);

    //===========================
    //  needDownloadScan

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int needDownloadScan_get(SCRow_needDownloadScan& needDownloadScan_out);
    int needDownloadScan_remove(u64 rowId);
    int needDownloadScan_clear();
    int needDownloadScan_add(const std::string& dirPath, u64 compId);
    //    Error Handling    //
    // Returns:
    //  maxRowId_out - largest rowId in current use in needDownloadScan table
    int needDownloadScan_getMaxRowId(u64& maxRowId_out);  // To visit all errors once.
    int needDownloadScan_incErr(u64 rowId);
    int needDownloadScan_getErr(u64 maxRowId,
                                SCRow_needDownloadScan& errorEntry_out);

    //===========================
    //  downloadChangeLog
    int downloadChangeLog_get(SCRow_downloadChangeLog& changeLog_out);

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if the specified {parent_path, name} pair is not found.
    int downloadChangeLog_getState(const std::string& parent_path,
                            const std::string& name,
                            SCRow_downloadChangeLog& changeLog_out);
    int downloadChangeLog_getRowCount(u64& count__out);

    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete.
    int downloadChangeLog_getAfterRowId(u64 afterRowId,
                                        SCRow_downloadChangeLog& changeLog_out);
    // Mark intermediate result, Set download_err_count to 0.
    int downloadChangeLog_setDownloadSuccess(u64 rowId);
    int downloadChangeLog_remove(u64 rowId);
    int downloadChangeLog_remove(const std::string& parent_path,
                                 const std::string& name);
    int downloadChangeLog_clear();
    int downloadChangeLog_add(const std::string& parent_path,
                              const std::string& name,
                              bool is_dir,
                              u64 download_action,
                              u64 comp_id,
                              u64 revision,
                              u64 client_reported_modify_time,
                              bool is_on_acs,
                              const std::string& hash_value,
                              bool file_size_exists,
                              u64 file_size);

    //    Error Handling    //
    // Returns:
    //  maxRowId_out - largest rowId in current use in downloadChangeLog table
    int downloadChangeLog_getMaxRowId(u64& maxRowId_out);
    // DownloadErr
    int downloadChangeLog_incErrDownload(u64 rowId);
    int downloadChangeLog_getErrDownload(u64 maxRowId,
                                         SCRow_downloadChangeLog& changeLog_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete
    int downloadChangeLog_getErrDownloadAfterRowId(u64 afterRowId,
                                                   u64 maxRowId,
                                                   SCRow_downloadChangeLog& changeLog_out);
    // CopybackErr
    int downloadChangeLog_incErrCopyback(u64 rowId);
    int downloadChangeLog_getErrCopyback(u64 maxRowId,
                                         SCRow_downloadChangeLog& changeLog_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete
    int downloadChangeLog_getErrCopybackAfterRowId(u64 afterRowId,
                                                   u64 maxRowId,
                                                   SCRow_downloadChangeLog& changeLog_out);

    //===========================
    //  uploadChangeLog

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int uploadChangeLog_get(SCRow_uploadChangeLog& changeLog_out);

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if the specified {parent_path, name} pair is not found.
    int uploadChangeLog_getState(const std::string& parent_path,
                            const std::string& name,
                            SCRow_uploadChangeLog& changeLog_out);
    int uploadChangeLog_getRowCount(u64& count__out);

    /// @param afterRowId Supports getting the next request before the previous
    ///                   request is marked complete.  This is to allow parallel uploads.
    /// @param rowFilter Only return the specific type of change.  This is to allow
    ///                  batching multiple changes of the same type into a single VCS call.
    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int uploadChangeLog_getAfterRowId(u64 afterRowId,
                                      SyncConfigDb_RowFilter rowFilter,
                                      SCRow_uploadChangeLog& changeLog_out);

    int uploadChangeLog_remove(u64 rowId);
    int uploadChangeLog_remove(const std::string& parent_path,
                               const std::string& name);
    int uploadChangeLog_clear();
    // comp_id - ignored when comp_id_exist==false
    // base_revision - ignored when base_revision_exist==false
    int uploadChangeLog_add(const std::string& parent_path,
                            const std::string& name,
                            bool is_dir,
                            u64 upload_action,
                            bool comp_id_exist,
                            u64 comp_id,
                            bool base_revision_exist,
                            u64 base_revision,
                            const std::string& hash_value,
                            bool file_size_exists,
                            u64 file_size);
    //    Error Handling
    // Returns:
    //  maxRowId_out - largest rowId in current use in uploadChangeLog table
    int uploadChangeLog_getMaxRowId(u64& maxRowId_out);
    int uploadChangeLog_incErr(u64 rowId);
    int uploadChangeLog_getErr(u64 maxRowId,
                               SCRow_uploadChangeLog& changeLog_out);
    /// @param afterRowId Supports getting the next request before the previous
    ///                   request is marked complete.  This is to allow parallel uploads.
    /// @param maxRowId Ignore any rowId's greater than maxRowId.  This is needed
    ///                 for the error case because if an error case fails, the rowId
    ///                 is incremented, and uploadChangeLog_getErr would never
    ///                 finish traversing the error rows.
    /// @param rowFilter Only return the specific type of change.  This is to allow
    ///                  batching multiple changes of the same type into a single VCS call.
    int uploadChangeLog_getErrAfterRowId(u64 afterRowId,
                                         u64 maxRowId,
                                         SyncConfigDb_RowFilter rowFilter,
                                         SCRow_uploadChangeLog& changeLog_out);

    //===========================
    //  syncHistoryTree

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if the specified {parent_path, name} pair is not found.
    int syncHistoryTree_get(const std::string& parent_path,
                            const std::string& name,
                            SCRow_syncHistoryTree& entry_out);
    int syncHistoryTree_getChildren(const std::string& path,
                                    std::vector<SCRow_syncHistoryTree>& entries_out);
    int syncHistoryTree_getByCompId(u64 component_id,
                                    SCRow_syncHistoryTree& entry_out); // TODO: Do we need a comp_id index?
    int syncHistoryTree_remove(const std::string& parent_path,
                               const std::string& name);
    int syncHistoryTree_add(const SCRow_syncHistoryTree& entry);
    // Updates entry fields without changing other fields.
    // Will return error if the entry does not exist.
    int syncHistoryTree_updateLocalTime(const std::string& parent_path,
                                        const std::string& name,
                                        bool local_mtime_exists,
                                        u64 local_mtime);
    int syncHistoryTree_updateCompId(const std::string& parent_path,
                                     const std::string& name,
                                     bool comp_id_exists,
                                     u64 comp_id,
                                     bool revision_exists,
                                     u64 revision);
    int syncHistoryTree_updateEntryInfo(const std::string& parent_path,
                                        const std::string& name,
                                        bool local_mtime_exists,
                                        u64 local_mtime,
                                        bool comp_id_exists,
                                        u64 comp_id,
                                        bool revision_exists,
                                        u64 revision,
                                        const std::string& hash_value,
                                        bool file_size_exists,
                                        u64 file_size);
    int syncHistoryTree_updateLastSeenInVersion(const std::string& parent_path,
                                                const std::string& name,
                                                bool last_seen_in_version_exists,
                                                u64 last_seen_in_version);
    int syncHistoryTree_updateVersionScanned(const std::string& parent_path,
                                             const std::string& name,
                                             bool version_scanned_exists,
                                             u64 version_scanned);
    int syncHistoryTree_clearTraversalInfo();

    //===========================
    //  deferredUploadJobSet

    int deferredUploadJobSet_get(const std::string& parent_path,
                                 const std::string& name,
                                 SCRow_deferredUploadJobSet& entry_out);
    int deferredUploadJobSet_getNext(SCRow_deferredUploadJobSet& entry_out);
    int deferredUploadJobSet_remove(u64 rowId);
    int deferredUploadJobSet_add(const std::string& parent_path,
                                 const std::string& name,
                                 u64 last_synced_parent_comp_id,
                                 bool last_synced_comp_id_exists,
                                 u64 last_synced_comp_id,
                                 bool Last_synced_revision_id_exists,
                                 u64 last_synced_revision_id,
                                 u64 local_mtime);
    int deferredUploadJobSet_getMaxRowId(u64& maxRowId_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete.
    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int deferredUploadJobSet_getAfterRowId(u64 afterRowId,
                                           SCRow_deferredUploadJobSet& entry_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete
    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int deferredUploadJobSet_getErrAfterRowId(u64 afterRowId,
                                              u64 maxRowId,
                                              SCRow_deferredUploadJobSet& entry_out);
    int deferredUploadJobSet_incErr(u64 rowId,
                                    u64 next_retry_time,
                                    u64 error_reason);

    //===========================
    //  asdStagingAreaCleanUp
    int asdStagingAreaCleanUp_getMaxRowId(u64& maxRowId_out);
    int asdStagingAreaCleanUp_get(SCRow_asdStagingAreaCleanUp& entry_out);
    int asdStagingAreaCleanUp_add(const std::string& accessUrl, const std::string& filename, u64 cleanup_time);
    int asdStagingAreaCleanUp_remove(u64 rowId);
    int asdStagingAreaCleanUp_getRow(const std::string& filename, SCRow_asdStagingAreaCleanUp& entry_out);

 private:

    std::string m_dbDir;
    std::string m_dbName;
    sqlite3* m_db;
    bool m_caseInsensitive;
    int openDbHelper(bool createOk,
                     bool &dbMissing_out);
    int privOpenDb(bool createOk,
                   std::string& dbPath_out,
                   bool &dbMissing_out);
    int privDeleteDb(const std::string& dbPath);
    int migrate_schema_v2_to_v3();
    int migrate_schema_v3_to_v4();
};

// See http://www.sqlite.org/autoinc.html  (2^63)
#define SQLITE3_MAX_ROW_ID 9223372036854775807ULL

#endif /* __SYNC_CONFIG_WORKER_DB_HPP_2_19_2013__ */
