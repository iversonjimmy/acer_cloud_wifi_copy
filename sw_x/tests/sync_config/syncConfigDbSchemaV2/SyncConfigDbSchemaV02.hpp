//
//  Copyright 2013 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef SYNC_CONFIG_DB_SCHEMA_V2_HPP_10_2_2013
#define SYNC_CONFIG_DB_SCHEMA_V2_HPP_10_2_2013

// Old schema version to test upgrade from SyncConfigDb.hpp r40268 06/25/2013

#include <vpl_plat.h>
#include <vplu_types.h>

#include "sqlite3.h"

#include <string>
#include <vector>

//=============================================================
//============ Structures Reflecting Db Columns ===============
struct SCV02Row_admin
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
    }
    SCV02Row_admin(){ clear(); }
};

struct SCV02Row_needDownloadScan
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
    SCV02Row_needDownloadScan(){ clear(); }
};

struct SCV02Row_downloadChangeLog
{
    u64 row_id;
    std::string parent_path; ///< Sync config-relative path, no leading slash, no trailing slash.
    std::string name;
    bool is_dir;
    u64 download_action;
    u64 comp_id;
    u64 revision;
    u64 client_reported_mtime;
    u64 download_err_count;
    bool download_succeeded;
    u64 copyback_err_count;

    void clear() {
        row_id = 0;
        parent_path.clear();
        name.clear();
        is_dir = false;
        download_action = 0;
        comp_id = 0;
        revision = 0;
        client_reported_mtime = 0;
        download_err_count = 0;
        download_succeeded = false;
        copyback_err_count = 0;
    }
    SCV02Row_downloadChangeLog(){ clear(); }
};

struct SCV02Row_uploadChangeLog
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
    }
    SCV02Row_uploadChangeLog(){ clear(); }
};

/// Invariant: If an entry is in the download change log, that file must also be in
///     the syncHistoryTree.
struct SCV02Row_syncHistoryTree
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

    /// Modification time of the file on local disk.  Only valid for files.
    /// This should only be set after the file has been successfully uploaded or downloaded
    /// (or we determined that the local file is the same as on infra).
    bool local_mtime_exists;
    u64 local_mtime;

    bool last_seen_in_version_exists;
    u64 last_seen_in_version;

    bool version_scanned_exists;
    u64 version_scanned;

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
    }
    SCV02Row_syncHistoryTree(){ clear(); }
};
//============ Structures Reflecting Db Columns ===============
//=============================================================

class SyncConfigDbSchemaV02
{
public:
    SyncConfigDbSchemaV02(const std::string& workingDir,
                 int syncType,
                 u64 userId,
                 u64 datasetId,
                 const std::string& syncConfigId);
    ~SyncConfigDbSchemaV02();
    // openDb can be called multiple times.  If the db has already been opened,
    // this function is a no-op, and success will be returned.
    int openDb();
    int openDb(bool &dbMissing_out);
    int closeDb();
    int beginTransaction();
    int commitTransaction();
    int rollbackTransaction();

    int admin_get(SCV02Row_admin& adminInfo_out);
    int admin_set(u64 deviceId,
                  u64 userId,
                  u64 datasetId,
                  const std::string& syncConfigId,
                  u64 lastOpenedTs);

    //===========================
    //  needDownloadScan

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int needDownloadScan_get(SCV02Row_needDownloadScan& needDownloadScan_out);
    int needDownloadScan_remove(u64 rowId);
    int needDownloadScan_clear();
    int needDownloadScan_add(const std::string& dirPath, u64 compId);
    //    Error Handling    //
    // Returns:
    //  maxRowId_out - largest rowId in current use in needDownloadScan table
    int needDownloadScan_getMaxRowId(u64& maxRowId_out);  // To visit all errors once.
    int needDownloadScan_incErr(u64 rowId);
    int needDownloadScan_getErr(u64 maxRowId,
                                SCV02Row_needDownloadScan& errorEntry_out);

    //===========================
    //  downloadChangeLog
    int downloadChangeLog_get(SCV02Row_downloadChangeLog& changeLog_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete.
    int downloadChangeLog_getAfterRowId(u64 afterRowId,
                                        SCV02Row_downloadChangeLog& changeLog_out);
    // Mark intermediate result, Set download_err_count to 0.
    int downloadChangeLog_setDownloadSuccess(u64 rowId);
    int downloadChangeLog_remove(u64 rowId);
    int downloadChangeLog_clear();
    int downloadChangeLog_add(const std::string& parent_path,
                              const std::string& name,
                              bool is_dir,
                              u64 download_action,
                              u64 comp_id,
                              u64 revision,
                              u64 client_reported_modify_time);
    //    Error Handling    //
    // Returns:
    //  maxRowId_out - largest rowId in current use in downloadChangeLog table
    int downloadChangeLog_getMaxRowId(u64& maxRowId_out);
    // DownloadErr
    int downloadChangeLog_incErrDownload(u64 rowId);
    int downloadChangeLog_getErrDownload(u64 maxRowId,
                                         SCV02Row_downloadChangeLog& changeLog_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete
    int downloadChangeLog_getErrDownloadAfterRowId(u64 afterRowId,
                                                   u64 maxRowId,
                                                   SCV02Row_downloadChangeLog& changeLog_out);
    // CopybackErr
    int downloadChangeLog_incErrCopyback(u64 rowId);
    int downloadChangeLog_getErrCopyback(u64 maxRowId,
                                         SCV02Row_downloadChangeLog& changeLog_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete
    int downloadChangeLog_getErrCopybackAfterRowId(u64 afterRowId,
                                                   u64 maxRowId,
                                                   SCV02Row_downloadChangeLog& changeLog_out);

    //===========================
    //  uploadChangeLog

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int uploadChangeLog_get(SCV02Row_uploadChangeLog& changeLog_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete.
    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if there are no more entries.
    int uploadChangeLog_getAfterRowId(u64 afterRowId,
                                      SCV02Row_uploadChangeLog& changeLog_out);
    int uploadChangeLog_remove(u64 rowId);
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
                            u64 base_revision);
    //    Error Handling    //
    // Returns:
    //  maxRowId_out - largest rowId in current use in uploadChangeLog table
    int uploadChangeLog_getMaxRowId(u64& maxRowId_out);
    int uploadChangeLog_incErr(u64 rowId);
    int uploadChangeLog_getErr(u64 maxRowId,
                               SCV02Row_uploadChangeLog& changeLog_out);
    /// @param afterRowId supports getting the next request before the previous
    ///                   request is marked complete
    int uploadChangeLog_getErrAfterRowId(u64 afterRowId,
                                         u64 maxRowId,
                                         SCV02Row_uploadChangeLog& changeLog_out);

    //===========================
    //  syncHistoryTree

    /// @return SYNC_AGENT_DB_ERR_ROW_NOT_FOUND if the specified {parent_path, name} pair is not found.
    int syncHistoryTree_get(const std::string& parent_path,
                            const std::string& name,
                            SCV02Row_syncHistoryTree& entry_out);
    int syncHistoryTree_getChildren(const std::string& path,
                                    std::vector<SCV02Row_syncHistoryTree>& entries_out);
    int syncHistoryTree_remove(const std::string& parent_path,
                               const std::string& name);
    int syncHistoryTree_add(const SCV02Row_syncHistoryTree& entry);
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
                                        u64 revision);
    int syncHistoryTree_updateLastSeenInVersion(const std::string& parent_path,
                                                const std::string& name,
                                                bool last_seen_in_version_exists,
                                                u64 last_seen_in_version);
    int syncHistoryTree_updateVersionScanned(const std::string& parent_path,
                                             const std::string& name,
                                             bool version_scanned_exists,
                                             u64 version_scanned);
    int syncHistoryTree_clearTraversalInfo();

    std::string getDbDir() { return m_dbDir; }

private:

    std::string m_dbDir;
    std::string m_dbName;
    sqlite3* m_db;

    int privOpenDb(std::string& dbPath_out, bool &dbMissing_out);
    int privDeleteDb(const std::string& dbPath);
};

#endif // SYNC_CONFIG_DB_SCHEMA_V2_HPP_10_2_2013
