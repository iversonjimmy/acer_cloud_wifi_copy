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

// See http://wiki.ctbg.acer.com/wiki/index.php/CCD_Sync_Agent_Refactor_2
// See http://wiki.ctbg.acer.com/wiki/index.php/CCD_Sync_Agent_Refactor_4 for
//   changes after 9/24/2013

#include "SyncConfigDb.hpp"

#include "vpl_plat.h"
#include "gvm_errors.h"
#include "gvm_file_utils.hpp"
#include "util_open_db_handle.hpp"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplex_safe_conversion.h"
#include "gvm_file_utils.hpp"

#include "sqlite3.h"

#include <string>
#include <log.h>

// Stringification: (first expand macro, then to string)
#define XSTR(s) STR(s)
#define STR(s) #s

// admin table will only ever have 1 row with row_id 1
#define ADMIN_ROW_ID 1

// TARGET_SCHEMA_VERSION is 2 for CCD 2.5, CCD 2.6 releases.
#define TARGET_SCHEMA_VERSION 4

#define SCWDB_SQLITE3_TRUE 1
#define SCWDB_SQLITE3_FALSE 0

////////////////////////////
// NOTES:
//   All paths MUST NOT contain any leading or trailing slashes.
//   All path separators MUST be the character  '/'.
//   The character '\\' MUST NOT be used.
//   Strings MUST be UTF-8.
// TERMS:
//   dent - directory entry
//   comp_id - Infra VCS Component Id
//   revision - Infra VCS Revision

static const char SQL_CREATE_TABLES[] =
"BEGIN IMMEDIATE;"

// The admin table will only ever have 1 row with id==1 (ADMIN_ROW_ID).
"CREATE TABLE IF NOT EXISTS admin ("
        "row_id INTEGER PRIMARY KEY,"
        "schema_version INTEGER NOT NULL,"
        "device_id INTEGER,"               // informational only (log warning if bad)
        "user_id INTEGER,"                 // informational only (log warning if bad)
        "dataset_id INTEGER,"              // informational only (log warning if bad)
        "sync_config_id TEXT,"             // informational only (log warning if bad)
        "sync_type INTEGER,"               // used to determine migration
        "one_way_initial_scan INTEGER,"    // UNUSED
        "last_opened_timestamp INTEGER,"   // informational only
        "migrate_from_sync_type INTEGER);" // When going from oneWayDown normal to pure virtual, existing
                                           // files need to be deleted
                                           //    NULL if no need to delete content   (no transition needed)
                                           //    normal_sync_type                    (in transition)
                                           // When going from oneWayDown pure virtual to download normal, existing
                                           // entries need to be backed by files.
                                           //    NULL if no need to back content     (never transitioned)
                                           //    pure_virtual_sync_type              (in transition)

"INSERT OR IGNORE INTO admin (row_id,schema_version) "
    "VALUES ("XSTR(ADMIN_ROW_ID)","
              XSTR(TARGET_SCHEMA_VERSION)");"

// Table representing  FIFO queue of directories relative to the sync_config
// root that are known to have a local version_scanned that is less than their
// version on VCS.
"CREATE TABLE IF NOT EXISTS need_download_scan ("
        "row_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "dir_path TEXT NOT NULL,"  // path to directory relative to sync config root, empty string if root
        "comp_id INTEGER NOT NULL,"
        "err_count INTEGER NOT NULL);"

// This table contains ordered download tasks.
// There are no delete tasks here.
"CREATE TABLE IF NOT EXISTS download_change_log ("
        "row_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_path TEXT NOT NULL,"  // path to parent directory relative to sync config root, empty string if root
        "name TEXT NOT NULL,"
        "is_dir INTEGER NOT NULL,"
        "download_action INTEGER NOT NULL,"
        "comp_id INTEGER NOT NULL,"
        "revision INTEGER NOT NULL,"
        "client_reported_mtime INTEGER NOT NULL,"
        "is_on_acs INTEGER NOT NULL,"
        // Hash from schema v4
        "hash_value TEXT," // Optional
        "file_size INTEGER," // Optional
        "parent_path_ci TEXT," // Optional
        "name_ci TEXT," // Optional

        // Err handling.
        "download_err_count INTEGER NOT NULL,"
        "download_succeeded INTEGER NOT NULL,"
        "copyback_err_count INTEGER NOT NULL,"
        "archive_off_err_count INTEGER NOT NULL);"

// This table contains ordered upload tasks.
"CREATE TABLE IF NOT EXISTS upload_change_log ("
        "row_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_path TEXT NOT NULL,"  // path to parent directory relative to sync config root, empty string if root
        "name TEXT NOT NULL,"
        "is_dir INTEGER NOT NULL,"
        "upload_action INTEGER NOT NULL,"
        "comp_id INTEGER,"   // Always NULL when is_dir!=0.
        "revision INTEGER,"  // Always NULL when is_dir!=0.

        // Hash from schema v4
        "hash_value TEXT," // Optional
        "file_size INTEGER," // Optional
        "parent_path_ci TEXT," // Optional
        "name_ci TEXT," // Optional

        // Err handling.
        "upload_err_count INTEGER NOT NULL);"

// Keeps track of the syncing information for all synced dents
// An entry marks the point (compId,revision,localTime) where the local dent and
//   the server dent were consistent.
"CREATE TABLE IF NOT EXISTS sync_history_tree ("
        "parent_path TEXT NOT NULL,"   // path to parent directory relative to sync config root, empty string if root
        "name TEXT NOT NULL,"          // directory entry name
        "is_dir INTEGER NOT NULL,"
        "comp_id INTEGER,"             // The file or directory's component ID as reported by VCS.
                                       //  Note that the same path can have different compIds over time.
        "revision INTEGER,"            // The base VCS revision of the file that is currently stored on
                                       //  the local filesystem.
        "local_mtime INTEGER,"         // Only valid when is_dir==0
                                       //  Modification time (in unix vpl time) of local file updated when we
                                       //  last uploaded or downloaded it. If the current filesystem
                                       //  timestamp doesn't match this, then we have a local change to upload.

        // Fields specifically for VCS download scan
        "last_seen_in_version INTEGER," // Used to track which dents should be deleted from the local filesystem.
        "version_scanned INTEGER,"      // Valid only when is_dir!=0. Avoids traversing into this directory
                                        //  if it hasn't changed. This prevents redundant calls to VCS GET dir.
        "is_on_acs INTEGER NOT NULL,"

        // Hash from schema v4
        "hash_value TEXT," // Optional
        "file_size INTEGER," // Optional
        "parent_path_ci TEXT," // Optional
        "name_ci TEXT," // Optional

        "PRIMARY KEY(parent_path, name));"

"CREATE TABLE IF NOT EXISTS deferred_upload_job_set ("
        "row_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_path TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "last_synced_parent_comp_id INTEGER NOT NULL,"
        "last_synced_comp_id INTEGER,"
        "last_synced_revision_id INTEGER,"
        "local_mtime INTEGER NOT NULL,"
        "error_count INTEGER NOT NULL,"
        "next_error_retry_time INTEGER,"
        "error_reason INTEGER,"
        "parent_path_ci TEXT,"
        "name_ci TEXT,"
        "UNIQUE(parent_path, name));"

/// Table used by archive storage node sync config to clean up staging area
"CREATE TABLE IF NOT EXISTS asd_staging_area_cleanup ("
        "row_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "access_url TEXT NOT NULL,"                   // access_url of temp file in staging area
        "filename TEXT NOT NULL,"                     // filename of the temp file
        "cleanup_time INTEGER NOT NULL);"             // time when temp file is moved to sync folder

"COMMIT;";

static bool isSqliteError(int ec)
{
    return ec != SQLITE_OK && ec != SQLITE_ROW && ec != SQLITE_DONE;
}

#define SQLITE_ERROR_BASE 0
static int mapSqliteErrCode(int ec)
{
    return SQLITE_ERROR_BASE - ec;
}

#define CHECK_DB_HANDLE(result, db, lbl)                           \
do{ if (db==NULL) {                                                \
      result = SYNC_AGENT_DB_ERR_INTERNAL;                         \
      LOG_ERROR("No db handle.");                                  \
      goto lbl;                                                    \
}}while(0)

#define CHECK_PREPARE(dberr, result, db, lbl)                      \
do{ if (isSqliteError(dberr)) {                                    \
      result = mapSqliteErrCode(dberr);                            \
      LOG_ERROR("Failed to prepare SQL stmt: %d, %s",              \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define CHECK_BIND(dberr, result, db, lbl)                         \
do{ if (isSqliteError(dberr)) {                                    \
      result = mapSqliteErrCode(dberr);                            \
      LOG_ERROR("Failed to bind value in prepared stmt: %d, %s",   \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define CHECK_STEP(dberr, result, db, lbl)                         \
do{ if (isSqliteError(dberr)) {                                    \
      result = mapSqliteErrCode(dberr);                            \
      LOG_ERROR("Failed to execute prepared stmt: %d, %s",         \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define CHECK_ROW_EXIST(dberr, result, db, lbl)                    \
do{ if (dberr!=SQLITE_ROW) {                                       \
      result = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;                    \
      goto lbl;                                                    \
}}while(0)

#define CHECK_FINALIZE(dberr, result, db, lbl)                     \
do{ if (isSqliteError(dberr)) {                                    \
      result = mapSqliteErrCode(dberr);                            \
      LOG_ERROR("Failed to finalize prepared stmt: %d, %s",        \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define CHECK_ERR(err, result, lbl)             \
do{ if ((err) != 0) {                           \
      result = err;                             \
      goto lbl;                                 \
}}while(0)

#define CHECK_RESULT(result, lbl)               \
do{ if ((result) != 0) {                        \
      goto lbl;                                 \
}}while(0)

// Max value is still 2^63 since sqlite3 is inherently signed.
#define GET_SQLITE_U64(sqlite3stmt, int64_out, resIndex, rv, lbl)                   \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            int64_out = sqlite3_column_int64(sqlite3stmt, d_resIndex);              \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

// Null is a valid value.
#define GET_SQLITE_U64_NULL(sqlite3stmt, valid_out, int64_out, resIndex, rv, lbl)   \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            valid_out = true;                                                       \
            int64_out = sqlite3_column_int64(sqlite3stmt, d_resIndex);              \
        }else if(d_sqlType == SQLITE_NULL){                                         \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

#define GET_SQLITE_STR(sqlite3stmt, str_out, resIndex, rv, lbl)                     \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_TEXT) {                                              \
            str_out.assign(reinterpret_cast<const char*>(                           \
                             sqlite3_column_text(sqlite3stmt, d_resIndex)));        \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_TEXT, d_resIndex);                          \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

// Null is a valid value.
#define GET_SQLITE_STR_NULL(sqlite3stmt, valid_out, str_out, resIndex, rv, lbl)     \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_TEXT) {                                              \
            valid_out = true;                                                       \
            str_out.assign(reinterpret_cast<const char*>(                           \
                             sqlite3_column_text(sqlite3stmt, d_resIndex)));        \
        }else if(d_sqlType == SQLITE_NULL){                                         \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_TEXT, d_resIndex);                          \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

#define GET_SQLITE_BOOL(sqlite3stmt, bool_out, resIndex, rv, lbl)                   \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            bool_out = (sqlite3_column_int64(sqlite3stmt, d_resIndex) != 0);        \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

#define GET_SQLITE_BOOL_NULL(sqlite3stmt, valid_out, bool_out, resIndex, rv, lbl)   \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            valid_out = true;                                                       \
            bool_out = (sqlite3_column_int64(sqlite3stmt, d_resIndex) != 0);        \
        }else if(d_sqlType == SQLITE_NULL){                                         \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

SyncConfigDb::SyncConfigDb(const std::string& dbDir,
                           SyncDbFamily syncFamily,
                           u64 userId,
                           u64 datasetId,
                           const std::string& syncConfigId,
                           bool isCaseInsensitive)
:  m_db(NULL), m_caseInsensitive(isCaseInsensitive)
{
    Util_trimTrailingSlashes(dbDir, m_dbDir);

    // dbName is format <sync_family>-<userId>-<datasetId>-<syncConfigId>.db
    char c_str[32];
    snprintf(c_str, 32, FMTu32, syncFamily);
    m_dbName.assign(c_str);
    m_dbName.append("-");
    snprintf(c_str, 32, FMTu64, userId);
    m_dbName.append(c_str);
    m_dbName.append("-");
    snprintf(c_str, 32, FMTu64, datasetId);
    m_dbName.append(c_str);
    m_dbName.append("-");
    m_dbName.append(syncConfigId);
    m_dbName.append(".db");
}

SyncConfigDb::~SyncConfigDb()
{
    int rc = closeDb();
    if(rc != 0) {
        LOG_ERROR("Failed to close Db (%s,%s):%d",
                  m_dbDir.c_str(), m_dbName.c_str(), rc);
    }
}

int SyncConfigDb::openDb()
{
    bool unused_out = false;
    return openDb(unused_out);
}

int SyncConfigDb::privOpenDb(bool createOk,
                             std::string& dbPath_out,
                             bool &dbMissing_out)
{
    int rc;
    if(createOk) {
        rc = Util_CreatePath(m_dbDir.c_str(), VPL_TRUE);
        if (rc) {
            LOG_ERROR("Failed to create directory %s:%d", m_dbDir.c_str(), rc);
            return rc;
        }
        // Hide sync temp folder for all SyncConfig types
#if !defined(VPL_PLAT_IS_WINRT)
        rc = VPLFile_SetAttribute(m_dbDir.c_str(), VPLFILE_ATTRIBUTE_HIDDEN,
                VPLFILE_ATTRIBUTE_MASK);
        if (rc != 0) {
           LOG_WARN("Failed to set attributes to the path %s, error %d.",
                    m_dbDir.c_str(), rc);
        }
#endif
    }

    int dberr = 0;
    Util_appendToAbsPath(m_dbDir, m_dbName, dbPath_out);
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(dbPath_out.c_str(), &statBuf);
    if(rc != 0 || statBuf.type != VPLFS_TYPE_FILE) {
        LOG_INFO("Initial scan set");
        dbMissing_out = true;
    }

    dberr = Util_OpenDbHandle(dbPath_out.c_str(), true, createOk, &m_db);
    if (dberr != 0) {
        LOG_ERROR("Failed to Util_OpenDbHandle(%s):%d",
                  dbPath_out.c_str(), dberr);
        if (dberr == SQLITE_CANTOPEN && dbMissing_out && !createOk) {
            return SYNC_AGENT_DB_NOT_EXIST_TO_OPEN;
        }
        return SYNC_AGENT_DB_CANNOT_OPEN;
    }

    if (createOk) {
        char *errmsg;
        dberr = sqlite3_exec(m_db, SQL_CREATE_TABLES, NULL, NULL, &errmsg);
        if (dberr != SQLITE_OK) {
            LOG_ERROR("Failed to test/create tables: %d, %s", dberr, errmsg);
            sqlite3_free(errmsg);
            return SYNC_AGENT_DB_CANNOT_OPEN;
        }
    }
    return 0;
}

int SyncConfigDb::privDeleteDb(const std::string& dbPath)
{
    closeDb();
    int rv = VPLFile_Delete(dbPath.c_str());
    if (rv != 0) {
        LOG_ERROR("Failed to delete DB: %d", rv);
    }
    return rv;
}

int SyncConfigDb::migrate_schema_v2_to_v3()
{
    int rv = 0;
    int rc;
    char *errmsg;

    // ASSERT that this function is within a transaction, which means the db
    // is NOT in autocommit mode (get_autocommit returns 0).
    ASSERT(sqlite3_get_autocommit(m_db) == 0);  // See http://www.sqlite.org/c3ref/get_autocommit.html

    const char* SQL_MIGRATE_V2_TO_V3_step1 =
            "ALTER TABLE admin "
            "ADD COLUMN migrate_from_sync_type INTEGER;";
    const char* SQL_MIGRATE_V2_TO_V3_step2 =
            "ALTER TABLE download_change_log "
            "ADD COLUMN archive_off_err_count INTEGER DEFAULT 0;";
    const char* SQL_MIGRATE_V2_TO_V3_step3 =
            "ALTER TABLE download_change_log "
            "ADD COLUMN is_on_acs INTEGER DEFAULT "XSTR(SCWDB_SQLITE3_TRUE)" NOT NULL;";
    const char* SQL_MIGRATE_V2_TO_V3_step4 =
            "ALTER TABLE sync_history_tree "
            "ADD COLUMN is_on_acs INTEGER DEFAULT "XSTR(SCWDB_SQLITE3_TRUE)" NOT NULL;";
    const char* SQL_MIGRATE_V2_TO_V3_step5 =
            "UPDATE admin "
            "SET schema_version=3 "
            "WHERE row_id="XSTR(ADMIN_ROW_ID)";";

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V2_TO_V3_step1, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V2_TO_V3_step1:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V2_TO_V3_step2, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V2_TO_V3_step2:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V2_TO_V3_step3, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V2_TO_V3_step3:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V2_TO_V3_step4, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V2_TO_V3_step4:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V2_TO_V3_step5, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V2_TO_V3_step5:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    return rv;;
}

// Things we update db from v3 to v4:
// 1. add a field hash_value(string) to the tables download_change_log,
//    upload_change_log and sync_history_tree
// 2. add a field file_size(integer) to the tables download_change_log,
//    upload_change_log and sync_history_tree
int SyncConfigDb::migrate_schema_v3_to_v4()
{
    int rv = 0;
    int rc;
    char *errmsg;

    // ASSERT that this function is within a transaction, which means the db
    // is NOT in autocommit mode (get_autocommit returns 0).
    ASSERT(sqlite3_get_autocommit(m_db) == 0);  // See http://www.sqlite.org/c3ref/get_autocommit.html

    const char* SQL_MIGRATE_V3_TO_V4_step1 =
            "ALTER TABLE sync_history_tree "
            "ADD COLUMN hash_value TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step2 =
            "ALTER TABLE download_change_log "
            "ADD COLUMN hash_value TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step3 =
            "ALTER TABLE upload_change_log "
            "ADD COLUMN hash_value TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step4 =
            "ALTER TABLE sync_history_tree "
            "ADD COLUMN file_size INTEGER;";
    const char* SQL_MIGRATE_V3_TO_V4_step5 =
            "ALTER TABLE download_change_log "
            "ADD COLUMN file_size INTEGER;";
    const char* SQL_MIGRATE_V3_TO_V4_step6 =
            "ALTER TABLE upload_change_log "
            "ADD COLUMN file_size INTEGER;";
    const char* SQL_MIGRATE_V3_TO_V4_step7 =
            "ALTER TABLE sync_history_tree "
            "ADD COLUMN parent_path_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step8 =
            "ALTER TABLE download_change_log "
            "ADD COLUMN parent_path_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step9 =
            "ALTER TABLE upload_change_log "
            "ADD COLUMN parent_path_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step10 =
            "ALTER TABLE deferred_upload_job_set "
            "ADD COLUMN parent_path_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step11 =
            "ALTER TABLE sync_history_tree "
            "ADD COLUMN name_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step12 =
            "ALTER TABLE download_change_log "
            "ADD COLUMN name_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step13 =
            "ALTER TABLE upload_change_log "
            "ADD COLUMN name_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step14 =
            "ALTER TABLE deferred_upload_job_set "
            "ADD COLUMN name_ci TEXT;";
    const char* SQL_MIGRATE_V3_TO_V4_step_final =
            "UPDATE admin "
            "SET schema_version=4 "
            "WHERE row_id="XSTR(ADMIN_ROW_ID)";";

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step1, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V2_TO_V3_step1:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step2, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step2:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step3, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step3:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step4, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step4:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step5, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step5:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step6, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step6:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step7, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step7:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step8, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step8:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step9, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step9:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step10, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step10:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step11, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step11:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step12, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step12:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step13, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step13:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step14, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step14:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    rc = sqlite3_exec(m_db, SQL_MIGRATE_V3_TO_V4_step_final, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL_MIGRATE_V3_TO_V4_step_final:%d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        return rv;
    }

    return rv;;
}

int SyncConfigDb::openDb(bool& dbMissing_out)
{
    return openDbHelper(true, dbMissing_out);
}

int SyncConfigDb::openDbNoCreate()
{
    bool dbMissing_unused_out;
    return openDbHelper(false, dbMissing_unused_out);
}

int SyncConfigDb::openDbHelper(bool createOk,
                               bool &dbMissing_out)
{
    int rv = 0;
    bool inTransaction = false;

    if (!m_db) {
        std::string dbPath;
        u64 schema_version = 0;
        rv = privOpenDb(createOk, dbPath, dbMissing_out);
        if (rv != 0) {
            goto end;
        }

        rv = admin_get_schema_version(/*OUT*/schema_version);
        if (rv != 0) {
            LOG_ERROR("admin_get_schema_version(%s):%d", dbPath.c_str(), rv);
            goto end;
        }

        if (createOk) {
            rv = beginTransaction();
            if (rv != 0) {
                LOG_ERROR("beginTransaction:%d", rv);
                goto end;
            }
            inTransaction = true;

            if (schema_version > TARGET_SCHEMA_VERSION) {
                LOG_ERROR("Existing DB is future schema version "FMTu64" > curr(%d)",
                          schema_version, TARGET_SCHEMA_VERSION);
                rv = SYNC_AGENT_DB_FUTURE_SCHEMA;
                goto end;
            }
            else if (schema_version < TARGET_SCHEMA_VERSION) {
#if TARGET_SCHEMA_VERSION == 2
                // schema_version 1 is an internal version, no need to exist in production.
                if (schema_version == 1) {
                    LOG_WARN("Previous DB schema version "FMTu64" < curr(%d). Deleting.",
                             schema_version, TARGET_SCHEMA_VERSION);
                    rv = privDeleteDb(dbPath);
                    if (rv != 0) {
                        LOG_ERROR("privDeleteDb(%s):%d", dbPath.c_str(), rv);
                        goto end;
                    }
                    rv = privOpenDb(dbPath, dbMissing_out);
                    if (rv != 0) {
                        LOG_ERROR("privOpenDb(%s):%d", dbPath.c_str(), rv);
                        goto end;
                    }
                }
#endif
                if (schema_version == 2) {
                    LOG_INFO("Migrating SyncConfigDb(%s) from v2 to v3", dbPath.c_str());
                    rv = migrate_schema_v2_to_v3();
                    if (rv != 0) {
                        LOG_ERROR("migrate_schema_v2_to_v3(%s):%d", dbPath.c_str(), rv);
                        goto end;
                    } else {
                        schema_version = 3;
                    }
                }
                if (rv != 0) {
                    LOG_ERROR("admin_get_schema_version(%s):%d", dbPath.c_str(), rv);
                    goto end;
                }
                if (schema_version == 3) {
                    LOG_INFO("Migrating SyncConfigDb(%s) from v3 to v4", dbPath.c_str());
                    rv = migrate_schema_v3_to_v4();
                    if (rv != 0) {
                        LOG_ERROR("migrate_schema_v3_to_v4(%s):%d", dbPath.c_str(), rv);
                        goto end;
                    }
                } else {
                    LOG_ERROR("Previous DB schema version "FMTu64" < curr(%d). "
                              "Upgrade not supported.",
                              schema_version, TARGET_SCHEMA_VERSION);
                    rv = SYNC_AGENT_DB_UNSUPPORTED_PREV_SCHEMA;
                    goto end;
                }
            }
            rv = commitTransaction();
            if(rv != 0) {
                LOG_ERROR("commitTransaction:%d", rv);
                goto end;
            }
            inTransaction = false;
        } else {
            if (schema_version != TARGET_SCHEMA_VERSION) {
                LOG_ERROR("dbSchema(%s) version mismatch:"FMTu64"!=target(%d)",
                          dbPath.c_str(), schema_version, TARGET_SCHEMA_VERSION);
                rv = SYNC_AGENT_DB_UNSUPPORTED_PREV_SCHEMA;
                goto end;
            }
        }

        SCRow_admin adminInfo_out;
        rv = admin_get(adminInfo_out);
        if (rv != 0) {
            LOG_ERROR("Failed to get admin info:%d", rv);
            rv = SYNC_AGENT_DB_CANNOT_OPEN;
            goto end;
        }

        // Log data
        LOG_INFO("Opened Db:%s - version:"FMTu64
                " deviceId:"FMTu64
                " user_id:"FMTu64
                " dataset_id:"FMTu64
                " synctype:"FMTu64
                " syncId:%s"
                " lastOpenedTime:"FMTu64,
                 dbPath.c_str(),
                 adminInfo_out.schema_version,
                 adminInfo_out.device_id_exists?
                         adminInfo_out.device_id:0,
                 adminInfo_out.user_id_exists?
                         adminInfo_out.user_id:0,
                 adminInfo_out.dataset_id_exists?
                         adminInfo_out.dataset_id:0,
                 adminInfo_out.sync_type_exists?
                         adminInfo_out.sync_type:9999,
                 adminInfo_out.sync_config_id_exists?
                         adminInfo_out.sync_config_id.c_str():"",
                 adminInfo_out.last_opened_timestamp_exists?
                         adminInfo_out.last_opened_timestamp:0);

        ASSERT_EQUAL(adminInfo_out.schema_version, ((u64)TARGET_SCHEMA_VERSION), FMTu64);
    }

 end:
    if (inTransaction) {
        int rc = rollbackTransaction();
        if(rc != 0) {
            LOG_ERROR("rollbackTransaction:%d", rc);
        }
    }
    if (rv != 0) {
        closeDb();
    }
    return rv;
}

int SyncConfigDb::openDbReadOnly()
{
    std::string dbPath;
    Util_appendToAbsPath(m_dbDir, m_dbName, dbPath);
    int rv = 0;
    int rc;
    if (!m_db) {
        u64 schema_version;

        rc = Util_OpenDbHandle(dbPath.c_str(), false, false, &m_db);
        if (rc != 0) {
            LOG_ERROR("Failed to Util_OpenDbHandle(%s):%d",
                      dbPath.c_str(), rc);
            return SYNC_AGENT_DB_CANNOT_OPEN;
        }
        rc = admin_get_schema_version(/*OUT*/ schema_version);
        if (rc != 0) {
            LOG_ERROR("Schema version read(%s):%d", dbPath.c_str(), rc);
            rv = rc;
            goto fail_schema_version;
        }
        if (schema_version != TARGET_SCHEMA_VERSION) {
            LOG_ERROR("schema version mismatch: "FMTu64"!="FMTu64,
                      (u64)schema_version, (u64)TARGET_SCHEMA_VERSION);
            rv = SYNC_AGENT_DB_CANNOT_OPEN;
            goto fail_schema_version;
        }
    } else {
        LOG_WARN("read-only db(%s) already open", dbPath.c_str());
    }
    return 0;
 fail_schema_version:
    closeDb();
    return rv;

}

int SyncConfigDb::closeDb()
{
    int rv = 0;

    if (m_db != NULL) {
        rv = sqlite3_close(m_db);
        m_db = NULL;
        if(rv != 0) {
            LOG_ERROR("Close Db failed (%s, %s):%d",
                      m_dbDir.c_str(), m_dbName.c_str(), rv);
        }else{
            LOG_INFO("Closed Db:(%s,%s)",
                     m_dbDir.c_str(), m_dbName.c_str());
        }
    }

    return rv;
}

int SyncConfigDb::beginTransaction()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_BEGIN =
            "BEGIN IMMEDIATE";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_BEGIN, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::beginExclusiveTransaction()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_BEGIN_EXCLUSIVE =
            "BEGIN EXCLUSIVE";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_BEGIN_EXCLUSIVE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::commitTransaction()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_COMMIT =
            "COMMIT";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_COMMIT, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::rollbackTransaction()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_ROLLBACK =
            "ROLLBACK";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ROLLBACK, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

//============================================================================
//============================== ADMIN TABLE =================================

#define ADMIN_COLUMNS_ADD "schema_version,"\
                          "device_id,"\
                          "user_id,"\
                          "dataset_id,"\
                          "sync_config_id,"\
                          "sync_type,"\
                          "last_opened_timestamp,"\
                          "migrate_from_sync_type"
#define ADMIN_COLUMNS_GET "row_id,"\
                          ADMIN_COLUMNS_ADD
static int admin_readRow(sqlite3_stmt* stmt,
                         SCRow_admin& adminInfo_out)
{
    int rv = 0;
    int resIndex = 0;
    adminInfo_out.clear();

    GET_SQLITE_U64(      stmt,
                         adminInfo_out.row_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         adminInfo_out.schema_version,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         adminInfo_out.device_id_exists,
                         adminInfo_out.device_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         adminInfo_out.user_id_exists,
                         adminInfo_out.user_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         adminInfo_out.dataset_id_exists,
                         adminInfo_out.dataset_id,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         adminInfo_out.sync_config_id_exists,
                         adminInfo_out.sync_config_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         adminInfo_out.sync_type_exists,
                         adminInfo_out.sync_type,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         adminInfo_out.last_opened_timestamp_exists,
                         adminInfo_out.last_opened_timestamp,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         adminInfo_out.migrate_from_sync_type_exists,
                         adminInfo_out.migrate_from_sync_type,
                         resIndex++, rv, end);

 end:
    return rv;
}

int SyncConfigDb::admin_get_schema_version(u64& schema_version_out)
{
    int rv = 0;
    int rc;
    schema_version_out = 0;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_ADMIN_GET_SCHEMA_VERSION =
            "SELECT schema_version "
            "FROM admin "
            "WHERE row_id=1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ADMIN_GET_SCHEMA_VERSION, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    {
        int resIndex = 0;
        GET_SQLITE_U64(stmt,
                       schema_version_out,
                       resIndex++, rv, end);
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::admin_get(SCRow_admin& adminInfo_out)
{
    int rv = 0;
    int rc;
    adminInfo_out.clear();
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_ADMIN_GET =
            "SELECT "ADMIN_COLUMNS_GET" "
            "FROM admin "
            "WHERE row_id=1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ADMIN_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = admin_readRow(stmt, adminInfo_out);
    if(rc != 0) {
        LOG_ERROR("admin_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::admin_set(bool deviceIdExists,
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
                            u64 migrateFromSyncType)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_ADMIN_UPDATE_FULL =
            "UPDATE admin "
            "SET device_id=?,"
                "user_id=?,"
                "dataset_id=?,"
                "sync_config_id=?,"
                "last_opened_timestamp=?,"
                "sync_type=?,"
                "migrate_from_sync_type=? "
            "WHERE row_id="XSTR(ADMIN_ROW_ID);

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ADMIN_UPDATE_FULL, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);
    // deviceId
    if (deviceIdExists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, deviceId);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // userId
    if (userIdExists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, userId);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // datasetId
    if (datasetIdExists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, datasetId);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // sync_config_id
    if (syncConfigIdExists) {
        rc = sqlite3_bind_text(stmt, ++bindPos, syncConfigId.data(), syncConfigId.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // last_opened_timestamp
    if (lastOpenedTimestampExists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, lastOpenedTimestamp);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // syncType
    if (syncTypeExists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, syncType);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // migrateFromSyncType
    if (migrateFromSyncTypeExists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, migrateFromSyncType);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::admin_set(u64 deviceId,
                            u64 userId,
                            u64 datasetId,
                            const std::string& syncConfigId,
                            u64 lastOpenedTs)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_ADMIN_UPDATE =
            "UPDATE admin "
            "SET device_id=?,"
                "user_id=?,"
                "dataset_id=?,"
                "sync_config_id=?,"
                "last_opened_timestamp=? "
            "WHERE row_id="XSTR(ADMIN_ROW_ID);

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ADMIN_UPDATE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);
    // deviceId
    rc = sqlite3_bind_int64(stmt, ++bindPos, deviceId);
    CHECK_BIND(rc, rv, m_db, end);
    // userId
    rc = sqlite3_bind_int64(stmt, ++bindPos, userId);
    CHECK_BIND(rc, rv, m_db, end);
    // datasetId
    rc = sqlite3_bind_int64(stmt, ++bindPos, datasetId);
    CHECK_BIND(rc, rv, m_db, end);
    // sync_config_id
    rc = sqlite3_bind_text(stmt, ++bindPos, syncConfigId.data(), syncConfigId.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // last_opened_timestamp
    rc = sqlite3_bind_int64(stmt, ++bindPos, lastOpenedTs);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::admin_set_sync_type(u64 syncType,
                                      bool migrateFromSyncTypeExists,
                                      u64 migrateFromSyncType)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_ADMIN_UPDATE_SYNC_TYPE =
            "UPDATE admin "
            "SET sync_type=?,"
                "migrate_from_sync_type=? "
            "WHERE row_id="XSTR(ADMIN_ROW_ID);

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ADMIN_UPDATE_SYNC_TYPE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);
    // syncType
    rc = sqlite3_bind_int64(stmt, ++bindPos, syncType);
    CHECK_BIND(rc, rv, m_db, end);
    // migrateFromSyncType
    if (migrateFromSyncTypeExists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, migrateFromSyncType);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}


//============================== ADMIN TABLE =================================
//============================================================================

//============================================================================
//======================== NEED DOWNLOAD SCAN TABLE ==========================

#define NEED_DOWNLOAD_SCAN_COLUMNS_ADD "dir_path,"\
                                       "comp_id,"\
                                       "err_count"

#define NEED_DOWNLOAD_SCAN_COLUMNS_GET "row_id,"\
                                       NEED_DOWNLOAD_SCAN_COLUMNS_ADD
static int needDownloadScan_readRow(sqlite3_stmt* stmt,
                                    SCRow_needDownloadScan& row_out)
{
    int rv = 0;
    int resIndex = 0;
    row_out.clear();

    GET_SQLITE_U64(      stmt,
                         row_out.row_id,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.dir_path,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.comp_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.err_count,
                         resIndex++, rv, end);

 end:
    return rv;
}

int SyncConfigDb::needDownloadScan_get(
        SCRow_needDownloadScan& downloadScan_out)
{
    int rv = 0;
    int rc;
    downloadScan_out.clear();
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_NEED_DOWNLOAD_SCAN_GET =
            "SELECT "NEED_DOWNLOAD_SCAN_COLUMNS_GET" "
            "FROM need_download_scan "
            "WHERE err_count=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = needDownloadScan_readRow(stmt, downloadScan_out);
    if(rc != 0) {
        LOG_ERROR("downloadScan_out:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::needDownloadScan_remove(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_NEED_DOWNLOAD_SCAN_REMOVE =
            "DELETE FROM need_download_scan WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_REMOVE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::needDownloadScan_clear()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;

    static const char* SQL_NEED_DOWNLOAD_SCAN_CLEAR =
            "DELETE FROM need_download_scan";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_CLEAR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    // Success as long as table is empty (doesn't matter if modification happened).

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::needDownloadScan_add(const std::string& dirPath,
                                       u64 compId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_NEED_DOWNLOAD_SCAN_ADD =
            "INSERT INTO need_download_scan ("NEED_DOWNLOAD_SCAN_COLUMNS_ADD") "
            "VALUES (?,?,0)";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_ADD, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // dir_path
    rc = sqlite3_bind_text(stmt, ++bindPos, dirPath.data(), dirPath.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // comp_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, compId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::needDownloadScan_getMaxRowId(u64& maxRowId_out)
{
    int rv = 0;
    int rc;
    int resIndex=0;
    sqlite3_stmt *stmt = NULL;
    maxRowId_out = 0;

    static const char* SQL_NEED_DOWNLOAD_SCAN_MAX_ROWID =
            "SELECT MAX(row_id) FROM need_download_scan";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_MAX_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(rc == SQLITE_ROW)
    {  // Actually a row is always returned.  NULL is returned if no row exists.
        bool maxRowExists_out = false;
        GET_SQLITE_U64_NULL(stmt, maxRowExists_out, maxRowId_out, resIndex++, rv, end);
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::needDownloadScan_incErr(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_NEED_DOWNLOAD_SCAN_INC_ERR =
            "UPDATE need_download_scan "
            "SET row_id=(SELECT MAX(row_id) FROM need_download_scan)+1,"
                "err_count=err_count+1 "
            "WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_INC_ERR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::needDownloadScan_getErr(
                            u64 maxRowId,
                            SCRow_needDownloadScan& errorEntry_out)
{
    int rv = 0;
    int rc;
    errorEntry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_NEED_DOWNLOAD_SCAN_GET =
            "SELECT "NEED_DOWNLOAD_SCAN_COLUMNS_GET" "
            "FROM need_download_scan "
            "WHERE row_id<=? AND err_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = needDownloadScan_readRow(stmt, errorEntry_out);
    if(rc != 0) {
        LOG_ERROR("downloadScan_out:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

//======================== NEED DOWNLOAD SCAN TABLE ==========================
//============================================================================

//============================================================================
//======================== DOWNLOAD CHANGE LOG TABLE =========================

#define DOWNLOAD_CHANGE_LOG_COLUMNS_ADD "parent_path,"\
                                        "name,"\
                                        "is_dir,"\
                                        "download_action,"\
                                        "comp_id,"\
                                        "revision,"\
                                        "client_reported_mtime,"\
                                        "is_on_acs,"\
                                        "hash_value,"\
                                        "file_size,"\
                                        "parent_path_ci,"\
                                        "name_ci,"\
                                        "download_err_count,"\
                                        "download_succeeded,"\
                                        "copyback_err_count,"\
                                        "archive_off_err_count"

#define DOWNLOAD_CHANGE_LOG_COLUMNS_GET "row_id,"\
                                        DOWNLOAD_CHANGE_LOG_COLUMNS_ADD
static int downloadChangeLog_readRow(sqlite3_stmt* stmt,
                                     SCRow_downloadChangeLog& row_out)
{
    int rv = 0;
    int resIndex = 0;
    bool getHashOut = false;
    row_out.clear();

    GET_SQLITE_U64(      stmt,
                         row_out.row_id,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.parent_path,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.name,
                         resIndex++, rv, end);
    GET_SQLITE_BOOL(     stmt,
                         row_out.is_dir,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.download_action,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.comp_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.revision,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.client_reported_mtime,
                         resIndex++, rv, end);
    GET_SQLITE_BOOL(     stmt,
                         row_out.is_on_acs,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         getHashOut,
                         row_out.hash_value,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.file_size_exists,
                         row_out.file_size,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.parent_path_ci_exists,
                         row_out.parent_path_ci,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.name_ci_exists,
                         row_out.name_ci,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.download_err_count,
                         resIndex++, rv, end);
    GET_SQLITE_BOOL(     stmt,
                         row_out.download_succeeded,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.copyback_err_count,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.archive_off_err_count,
                         resIndex++, rv, end);
 end:
    return rv;
}

// Get the row count number from downloadChangeLog table
int SyncConfigDb::downloadChangeLog_getRowCount(u64& count__out)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    count__out = 0;
    static const char* SQL_DOWNLOAD_CHANGE_LOG_GET_ROW_COUNT =
        "SELECT COUNT(*) FROM download_change_log";
    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET_ROW_COUNT,
            -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);

    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    GET_SQLITE_U64(stmt,
                   count__out,
                   0, rv, end);

    if(rv != 0) {
        LOG_ERROR("downloadChangeLog_getRowCount:%d", rv);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_get(
        SCRow_downloadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_DOWNLOAD_CHANGE_LOG_GET =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            "WHERE download_err_count=0 AND copyback_err_count=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = downloadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}


// Get the state of a given path (parent_path + name) and to know if
// uploadChangeLog contains this path or not, name could also be a directory
int SyncConfigDb::downloadChangeLog_getState(const std::string& parent_path,
                                             const std::string& name,
                                             SCRow_downloadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;
    std::string separater = std::string("/");
    std::string like_exp = std::string("/%");
    if (parent_path.empty()) {
        separater = std::string("");
        if (name.empty()) {
            like_exp = std::string("%");
        }
    }
    std::string full_path = parent_path.data() + separater + name.data();
    std::string like_full_path = parent_path.data() + separater +
        name.data() + like_exp;
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;
    std::string full_path_ci;
    std::string like_full_path_ci;

    if (m_caseInsensitive) {
        static const char* SQL_DOWNLOAD_CHANGE_LOG_GET =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            // Case 1: requested path is a file, changelog entry is exact match.
            "WHERE (parent_path_ci=? AND name_ci=?) OR "
            // Case 2: requested path is a directory, changelog entry is directly within that directory.
            "parent_path_ci=? OR "
            // Case 3: requested path is a directory, changelog entry is nested within a subdirectory beneath that directory.
            "parent_path_ci LIKE ? "
            "ORDER BY parent_path_ci ASC "
            "LIMIT 1";

        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        Util_UTF8Upper(full_path.data(), full_path_ci);
        Util_UTF8Upper(like_full_path.data(), like_full_path_ci);

        LOG_DEBUG("parent(%s), name(%s), full_path(%s), like_full_path(%s)", 
                  parent_path_ci.c_str(), name_ci.c_str(), full_path_ci.c_str(), like_full_path_ci.c_str());

        CHECK_DB_HANDLE(rv, m_db, end); 

        rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path as parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, full_path_ci.data(), full_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path like parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, like_full_path_ci.data(), like_full_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        static const char* SQL_DOWNLOAD_CHANGE_LOG_GET =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            // Case 1: requested path is a file, changelog entry is exact match.
            "WHERE (parent_path=? AND name=?) OR "
            // Case 2: requested path is a directory, changelog entry is directly within that directory.
            "parent_path=? OR "
            // Case 3: requested path is a directory, changelog entry is nested within a subdirectory beneath that directory.
            "parent_path LIKE ? "
            "ORDER BY parent_path ASC "
            "LIMIT 1";

        CHECK_DB_HANDLE(rv, m_db, end); 

        rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(),
                               parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path as parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, full_path.data(),
                               full_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path like parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, like_full_path.data(),
                               like_full_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = downloadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_getAfterRowId(
        u64 afterRowId,
        SCRow_downloadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    int bindPos=0;  // Left-most position is 1
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_DOWNLOAD_CHANGE_LOG_GET_AFTER_ROW_ID =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            "WHERE row_id>? AND download_err_count=0 AND copyback_err_count=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET_AFTER_ROW_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_bind_int64(stmt, ++bindPos, afterRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = downloadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_setDownloadSuccess(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_SET_DOWNLOAD_SUCCESS =
            "UPDATE download_change_log "
            "SET download_succeeded=1,"
                "download_err_count=0 "
            "WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_SET_DOWNLOAD_SUCCESS, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_remove(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_REMOVE =
            "DELETE FROM download_change_log WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_REMOVE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_remove(const std::string& parent_path,
                                           const std::string& name)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_DOWNLOAD_CHANGE_LOG_REMOVE_BY_NAME =
            "DELETE FROM download_change_log WHERE parent_path_ci=? AND name_ci=?";

        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);

        LOG_DEBUG("parent(%s), name(%s)", parent_path_ci.c_str(), name_ci.c_str());

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_REMOVE_BY_NAME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        static const char* SQL_DOWNLOAD_CHANGE_LOG_REMOVE_BY_NAME =
            "DELETE FROM download_change_log WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_REMOVE_BY_NAME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_clear()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;

    static const char* SQL_DOWNLOAD_CHANGE_LOG_CLEAR =
            "DELETE FROM download_change_log";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_CLEAR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    // Success as long as table is empty
    // (doesn't matter if modification happened or not).

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_add(const std::string& parent_path,
                                        const std::string& name,
                                        bool is_dir,
                                        u64 download_action,
                                        u64 comp_id,
                                        u64 revision,
                                        u64 client_reported_mtime,
                                        bool is_on_acs,
                                        const std::string& hash_value,
                                        bool file_size_exists,
                                        u64 file_size)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    static const char* SQL_DOWNLOAD_CHANGE_LOG_ADD =
            "INSERT INTO download_change_log ("DOWNLOAD_CHANGE_LOG_COLUMNS_ADD") "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,0,0,0,0)";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_ADD, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // is_dir
    if(is_dir) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_TRUE);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_FALSE);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // download_action
    rc = sqlite3_bind_int64(stmt, ++bindPos, download_action);
    CHECK_BIND(rc, rv, m_db, end);
    // comp_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, comp_id);
    CHECK_BIND(rc, rv, m_db, end);
    // revision
    rc = sqlite3_bind_int64(stmt, ++bindPos, revision);
    CHECK_BIND(rc, rv, m_db, end);
    // client_reported_mtime
    rc = sqlite3_bind_int64(stmt, ++bindPos, client_reported_mtime);
    CHECK_BIND(rc, rv, m_db, end);
    // is_on_acs
    if(is_on_acs) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_TRUE);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_FALSE);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // hash_value
    if (!hash_value.empty()) {
        rc = sqlite3_bind_text(stmt, ++bindPos, hash_value.data(), hash_value.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // file_size
    if (file_size_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, file_size);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);

        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        // parent_path_ci
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_getMaxRowId(u64& maxRowId_out)
{
    int rv = 0;
    int rc;
    int resIndex=0;
    sqlite3_stmt *stmt = NULL;
    maxRowId_out = 0;

    static const char* SQL_DOWNLOAD_CHANGE_LOG_MAX_ROWID =
            "SELECT MAX(row_id) FROM download_change_log";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_MAX_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(rc == SQLITE_ROW)
    {  // Actually a row is always returned for MAX().  NULL is returned if no row exists.
        bool maxRowExists_out = false;
        GET_SQLITE_U64_NULL(stmt, maxRowExists_out, maxRowId_out, resIndex++, rv, end);
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_incErrDownload(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_INC_DOWNLOAD_ERR =
            "UPDATE download_change_log "
            "SET row_id=(SELECT MAX(row_id) FROM download_change_log)+1,"
                "download_err_count=download_err_count+1 "
            "WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_INC_DOWNLOAD_ERR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_getErrDownload(
                                     u64 maxRowId,
                                     SCRow_downloadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_DOWNLOAD =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            "WHERE row_id<=? AND download_err_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_DOWNLOAD, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = downloadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_getErrDownloadAfterRowId(
                                        u64 afterRowId,
                                        u64 maxRowId,
                                        SCRow_downloadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_DOWNLOAD_AFTER_ROWID =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            "WHERE row_id>? AND row_id<=? AND download_err_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_DOWNLOAD_AFTER_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // afterRowId
    rc = sqlite3_bind_int64(stmt, ++bindPos, afterRowId);
    CHECK_BIND(rc, rv, m_db, end);

    // maxRowId
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = downloadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_incErrCopyback(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_INC_COPYBACK_ERR =
            "UPDATE download_change_log "
            "SET row_id=(SELECT MAX(row_id) FROM download_change_log)+1,"
                "copyback_err_count=copyback_err_count+1 "
            "WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_INC_COPYBACK_ERR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_getErrCopyback(
                                     u64 maxRowId,
                                     SCRow_downloadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_COPYBACK =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            "WHERE row_id<=? AND copyback_err_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_COPYBACK, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = downloadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::downloadChangeLog_getErrCopybackAfterRowId(
                                        u64 afterRowId,
                                        u64 maxRowId,
                                        SCRow_downloadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_COPYBACK_AFTER_ROWID =
            "SELECT "DOWNLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM download_change_log "
            "WHERE row_id>? AND row_id<=? AND copyback_err_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DOWNLOAD_CHANGE_LOG_GET_ERR_COPYBACK_AFTER_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // afterRowId
    rc = sqlite3_bind_int64(stmt, ++bindPos, afterRowId);
    CHECK_BIND(rc, rv, m_db, end);

    // maxRowId
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = downloadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

//======================== DOWNLOAD CHANGE LOG TABLE =========================
//============================================================================

//============================================================================
//========================= UPLOAD CHANGE LOG TABLE ==========================

#define UPLOAD_CHANGE_LOG_COLUMNS_ADD "parent_path,"\
                                      "name,"\
                                      "is_dir,"\
                                      "upload_action,"\
                                      "comp_id,"\
                                      "revision,"\
                                      "hash_value,"\
                                      "file_size,"\
                                      "parent_path_ci,"\
                                      "name_ci,"\
                                      "upload_err_count"

#define UPLOAD_CHANGE_LOG_COLUMNS_GET "row_id,"\
                                      UPLOAD_CHANGE_LOG_COLUMNS_ADD

static int uploadChangeLog_readRow(sqlite3_stmt* stmt,
                                   SCRow_uploadChangeLog& row_out)
{
    int rv = 0;
    int resIndex = 0;
    bool getHashOut = false;
    row_out.clear();

    GET_SQLITE_U64(      stmt,
                         row_out.row_id,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.parent_path,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.name,
                         resIndex++, rv, end);
    GET_SQLITE_BOOL(     stmt,
                         row_out.is_dir,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.upload_action,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.comp_id_exists,
                         row_out.comp_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.revision_exists,
                         row_out.revision,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         getHashOut,
                         row_out.hash_value,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.file_size_exists,
                         row_out.file_size,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.parent_path_ci_exists,
                         row_out.parent_path_ci,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.name_ci_exists,
                         row_out.name_ci,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.upload_err_count,
                         resIndex++, rv, end);
 end:
    return rv;
}

// Get the row count number from uploadChangeLog table
int SyncConfigDb::uploadChangeLog_getRowCount(u64& count__out)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    count__out = 0;
    static const char* SQL_UPLOAD_CHANGE_LOG_GET_ROW_COUNT =
        "SELECT COUNT(*) FROM upload_change_log";
    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_GET_ROW_COUNT,
            -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);

    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    GET_SQLITE_U64(stmt,
                   count__out,
                   0, rv, end);

    if(rv != 0) {
        LOG_ERROR("uploadChangeLog_getRowCount:%d", rv);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_get(SCRow_uploadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_UPLOAD_CHANGE_LOG_GET =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE upload_err_count=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = uploadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("uploadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

// Get the state of a given path (parent_path + name) and to know if
//  uploadChangeLog contains this path or not, name could also be a directory
int SyncConfigDb::uploadChangeLog_getState(const std::string& parent_path,
                                           const std::string& name,
                                           SCRow_uploadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;
    std::string separater = std::string("/");
    std::string like_exp = std::string("/%");
    if (parent_path.empty()) {
        separater = std::string("");
        if (name.empty()) {
            like_exp = std::string("%");
        }
    }
    std::string full_path = parent_path.data() + separater + name.data();
    std::string like_full_path = parent_path.data() + separater +
        name.data() + like_exp;
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;
    std::string full_path_ci;
    std::string like_full_path_ci;

    if (m_caseInsensitive) {
        static const char* SQL_UPLOAD_CHANGE_LOG_GET =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            // Case 1: requested path is a file, changelog entry is exact match.
            "WHERE (parent_path_ci=? AND name_ci=?) OR "
            // Case 2: requested path is a directory, changelog entry is directly within that directory.
            "parent_path_ci=? OR "
            // Case 3: requested path is a directory, changelog entry is nested within a subdirectory beneath that directory.
            "parent_path_ci LIKE ? "
            "ORDER BY parent_path_ci ASC "
            "LIMIT 1";

        CHECK_DB_HANDLE(rv, m_db, end);

        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        Util_UTF8Upper(full_path.data(), full_path_ci);
        Util_UTF8Upper(like_full_path.data(), like_full_path_ci);

        rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path as parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, full_path_ci.data(), full_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path like parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, like_full_path_ci.data(), like_full_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        static const char* SQL_UPLOAD_CHANGE_LOG_GET =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            // Case 1: requested path is a file, changelog entry is exact match.
            "WHERE (parent_path=? AND name=?) OR "
            // Case 2: requested path is a directory, changelog entry is directly within that directory.
            "parent_path=? OR "
            // Case 3: requested path is a directory, changelog entry is nested within a subdirectory beneath that directory.
            "parent_path LIKE ? "
            "ORDER BY parent_path ASC "
            "LIMIT 1";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path as parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, full_path.data(), full_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // full path like parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, like_full_path.data(), like_full_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = uploadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("uploadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}


int SyncConfigDb::uploadChangeLog_getAfterRowId(
                                        u64 afterRowId,
                                        SyncConfigDb_RowFilter rowFilter,
                                        SCRow_uploadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_ALL =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND upload_err_count=0 "
            "ORDER BY row_id LIMIT 1";
    static const char* SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_CREATE_AND_UPDATE =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND upload_err_count=0 AND "
                "(upload_action="XSTR(SCDB_UPLOAD_ACTION_CREATE)" OR "
                    "upload_action="XSTR(SCDB_UPLOAD_ACTION_UPDATE)
                ") "
            "ORDER BY row_id LIMIT 1";
    static const char* SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_FILE_DELETE =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND upload_err_count=0 AND "
                "upload_action="XSTR(SCDB_UPLOAD_ACTION_DELETE)" AND is_dir=0 "
            "ORDER BY row_id LIMIT 1";
    static const char* SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_DIRECTORY_DELETE =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND upload_err_count=0 AND "
                "upload_action="XSTR(SCDB_UPLOAD_ACTION_DELETE)" AND is_dir!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    switch (rowFilter)
    {
    case SCDB_ROW_FILTER_ALLOW_ALL:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_ALL,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    case SCDB_ROW_FILTER_ALLOW_CREATE_AND_UPDATE:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_CREATE_AND_UPDATE,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    case SCDB_ROW_FILTER_ALLOW_FILE_DELETE:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_FILE_DELETE,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    case SCDB_ROW_FILTER_ALLOW_DIRECTORY_DELETE:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID_FILTER_ALLOW_DIRECTORY_DELETE,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    }

    rc = sqlite3_bind_int64(stmt, ++bindPos, afterRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = uploadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("uploadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_remove(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1

    LOG_DEBUG("Row "FMTu64, rowId);

    static const char* SQL_UPLOAD_CHANGE_LOG_REMOVE =
            "DELETE FROM upload_change_log WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_REMOVE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_remove(const std::string& parent_path,
                                         const std::string& name)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    LOG_DEBUG("(%s,%s)", parent_path.c_str(), name.c_str());
    if (m_caseInsensitive) {
        static const char* SQL_UPLOAD_CHANGE_LOG_REMOVE_BY_NAME =
            "DELETE FROM upload_change_log WHERE parent_path_ci=? AND name_ci=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);

        rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_REMOVE_BY_NAME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        static const char* SQL_UPLOAD_CHANGE_LOG_REMOVE_BY_NAME =
            "DELETE FROM upload_change_log WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_REMOVE_BY_NAME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_clear()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;

    static const char* SQL_UPLOAD_CHANGE_LOG_CLEAR =
            "DELETE FROM upload_change_log";

    LOG_DEBUG("start");
    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_CLEAR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    // Success as long as table is empty
    // (doesn't matter if modification happened or not).

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_add(const std::string& parent_path,
                                      const std::string& name,
                                      bool is_dir,
                                      u64 upload_action,
                                      bool comp_id_exist,
                                      u64 comp_id,
                                      bool base_revision_exist,
                                      u64 base_revision,
                                      const std::string& hash_value,
                                      bool file_size_exists,
                                      u64 file_size)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    static const char* SQL_UPLOAD_CHANGE_LOG_ADD =
            "INSERT INTO upload_change_log ("UPLOAD_CHANGE_LOG_COLUMNS_ADD") "
            "VALUES (?,?,?,?,?,?,?,?,?,?,0)";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_ADD, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // is_dir
    if(is_dir) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_TRUE);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_FALSE);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // upload_action
    rc = sqlite3_bind_int64(stmt, ++bindPos, upload_action);
    CHECK_BIND(rc, rv, m_db, end);
    // comp_id
    if(comp_id_exist) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, comp_id);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // base_revision
    if(base_revision_exist) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, base_revision);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // hash_value
    if (!hash_value.empty()) {
        rc = sqlite3_bind_text(stmt, ++bindPos, hash_value.data(), hash_value.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // file_size
    if (file_size_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, file_size);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);

        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        // parent_path_ci
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_getMaxRowId(u64& maxRowId_out)
{
    int rv = 0;
    int rc;
    int resIndex=0;
    sqlite3_stmt *stmt = NULL;
    maxRowId_out = 0;

    static const char* SQL_UPLOAD_CHANGE_LOG_MAX_ROWID =
            "SELECT MAX(row_id) FROM upload_change_log";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_MAX_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(rc == SQLITE_ROW)
    {  // Actually a row is always returned for MAX().  NULL is returned if no row exists.
        bool maxRowExists_out = false;
        GET_SQLITE_U64_NULL(stmt, maxRowExists_out, maxRowId_out, resIndex++, rv, end);
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}


int SyncConfigDb::uploadChangeLog_incErr(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    LOG_DEBUG("Row "FMTu64, rowId);

    static const char* SQL_UPLOAD_CHANGE_LOG_INC_ERR =
            "UPDATE upload_change_log "
            "SET row_id=(SELECT MAX(row_id) FROM upload_change_log)+1,"
                "upload_err_count=upload_err_count+1 "
            "WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_INC_ERR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_getErr(
                           u64 maxRowId,
                           SCRow_uploadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_UPLOAD_CHANGE_LOG_GET_ERR =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id<=? AND upload_err_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_GET_ERR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = uploadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("changeLog_out:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::uploadChangeLog_getErrAfterRowId(
                                         u64 afterRowId,
                                         u64 maxRowId,
                                         SyncConfigDb_RowFilter rowFilter,
                                         SCRow_uploadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_ALLOW_ALL =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND row_id<=? AND upload_err_count!=0 "
            "ORDER BY row_id LIMIT 1";
    static const char* SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_CREATE_AND_UPDATE =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND row_id<=? AND upload_err_count!=0 AND "
                "(upload_action="XSTR(SCDB_UPLOAD_ACTION_CREATE)" OR "
                    "upload_action="XSTR(SCDB_UPLOAD_ACTION_UPDATE)
                ") "
            "ORDER BY row_id LIMIT 1";
    static const char* SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_FILE_DELETE =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND row_id<=? AND upload_err_count!=0 AND "
                "upload_action="XSTR(SCDB_UPLOAD_ACTION_DELETE)" AND is_dir=0 "
            "ORDER BY row_id LIMIT 1";
    static const char* SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_DIRECTORY_DELETE =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND row_id<=? AND upload_err_count!=0 AND "
                "upload_action="XSTR(SCDB_UPLOAD_ACTION_DELETE)" AND is_dir!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    switch (rowFilter)
    {
    case SCDB_ROW_FILTER_ALLOW_ALL:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_ALLOW_ALL,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    case SCDB_ROW_FILTER_ALLOW_CREATE_AND_UPDATE:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_CREATE_AND_UPDATE,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    case SCDB_ROW_FILTER_ALLOW_FILE_DELETE:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_FILE_DELETE,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    case SCDB_ROW_FILTER_ALLOW_DIRECTORY_DELETE:
        rc = sqlite3_prepare_v2(m_db,
                SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID_FILTER_DIRECTORY_DELETE,
                -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        break;
    }

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, afterRowId);
    CHECK_BIND(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = uploadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("uploadChangeLog_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

//========================= UPLOAD CHANGE LOG TABLE ==========================
//============================================================================

//============================================================================
//========================= SYNC HISTORY TREE TABLE ==========================

#define SYNC_HISTORY_TREE_COLUMNS "parent_path,"\
                                  "name,"\
                                  "is_dir,"\
                                  "comp_id,"\
                                  "revision,"\
                                  "local_mtime,"\
                                  "last_seen_in_version,"\
                                  "version_scanned,"\
                                  "is_on_acs,"\
                                  "hash_value,"\
                                  "file_size,"\
                                  "parent_path_ci,"\
                                  "name_ci"

static int syncHistoryTree_readRow(sqlite3_stmt* stmt,
                                   SCRow_syncHistoryTree& row_out)
{
    int rv = 0;
    int resIndex = 0;
    bool getHashOut = false;
    row_out.clear();

    GET_SQLITE_STR(      stmt,
                         row_out.parent_path,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.name,
                         resIndex++, rv, end);
    GET_SQLITE_BOOL(     stmt,
                         row_out.is_dir,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.comp_id_exists,
                         row_out.comp_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.revision_exists,
                         row_out.revision,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.local_mtime_exists,
                         row_out.local_mtime,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.last_seen_in_version_exists,
                         row_out.last_seen_in_version,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.version_scanned_exists,
                         row_out.version_scanned,
                         resIndex++, rv, end);
    GET_SQLITE_BOOL(     stmt,
                         row_out.is_on_acs,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         getHashOut,
                         row_out.hash_value,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.file_size_exists,
                         row_out.file_size,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.parent_path_ci_exists,
                         row_out.parent_path_ci,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.name_ci_exists,
                         row_out.name_ci,
                         resIndex++, rv, end);
 end:
    return rv;
}

int SyncConfigDb::syncHistoryTree_get(const std::string& parent_path,
                                      const std::string& name,
                                      SCRow_syncHistoryTree& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_GET =
            "SELECT "SYNC_HISTORY_TREE_COLUMNS" "
            "FROM sync_history_tree "
            "WHERE parent_path_ci=? AND name_ci=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        static const char* SQL_SYNC_HISTORY_TREE_GET =
            "SELECT "SYNC_HISTORY_TREE_COLUMNS" "
            "FROM sync_history_tree "
            "WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = syncHistoryTree_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("syncHistoryTree_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_getChildren(
                            const std::string& path,
                            std::vector<SCRow_syncHistoryTree>& entries_out)
{
    int rv = 0;
    int rc;
    entries_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_GET_CHILDREN =
            "SELECT "SYNC_HISTORY_TREE_COLUMNS" "
            "FROM sync_history_tree "
            "WHERE parent_path_ci=?";

        Util_UTF8Upper(path.data(), parent_path_ci);

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_GET_CHILDREN, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        static const char* SQL_SYNC_HISTORY_TREE_GET_CHILDREN =
            "SELECT "SYNC_HISTORY_TREE_COLUMNS" "
            "FROM sync_history_tree "
            "WHERE parent_path=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_GET_CHILDREN, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, path.data(), path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    while(rc == SQLITE_ROW) {
        SCRow_syncHistoryTree entry_out;
        rc = syncHistoryTree_readRow(stmt, entry_out);
        if(rc != 0) {
            LOG_ERROR("syncHistoryTree_readRow:%d", rc);
            rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
            goto end;
        }
        entries_out.push_back(entry_out);

        rc = sqlite3_step(stmt);
        CHECK_STEP(rc, rv, m_db, end);
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_getByCompId(u64 component_id,
                                              SCRow_syncHistoryTree& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;

    static const char* SQL_SYNC_HISTORY_TREE_GET_BY_COMP_ID =
            "SELECT "SYNC_HISTORY_TREE_COLUMNS" "
            "FROM sync_history_tree "
            "WHERE comp_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_GET_BY_COMP_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // comp_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, component_id);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = syncHistoryTree_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("syncHistoryTree_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_remove(const std::string& parent_path,
                                         const std::string& name)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_REMOVE =
            "DELETE FROM sync_history_tree WHERE parent_path_ci=? AND name_ci=?";

        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_REMOVE, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path 
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        static const char* SQL_SYNC_HISTORY_TREE_REMOVE =
            "DELETE FROM sync_history_tree WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_REMOVE, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        // parent_path 
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_add(const SCRow_syncHistoryTree& entry)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    static const char* SQL_SYNC_HISTORY_TREE_ADD =
            "INSERT OR REPLACE INTO sync_history_tree ("SYNC_HISTORY_TREE_COLUMNS") "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_ADD, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, entry.parent_path.data(), entry.parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, entry.name.data(), entry.name.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // is_dir
    if(entry.is_dir) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_TRUE);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_FALSE);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // comp_id
    if(entry.comp_id_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, entry.comp_id);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // base_revision
    if(entry.revision_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, entry.revision);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // local_mtime
    if(entry.local_mtime_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, entry.local_mtime);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // last_seen_in_version
    if(entry.last_seen_in_version_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, entry.last_seen_in_version);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // version_scanned
    if(entry.version_scanned_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, entry.version_scanned);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // is_on_acs
    if(entry.is_on_acs) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_TRUE);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_int64(stmt, ++bindPos, SCWDB_SQLITE3_FALSE);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // hash_value
    if (!entry.hash_value.empty()) {
        rc = sqlite3_bind_text(stmt, ++bindPos, entry.hash_value.data(), entry.hash_value.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // file_size
    if (entry.file_size_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, entry.file_size);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    if (m_caseInsensitive) { 
        Util_UTF8Upper(entry.parent_path.data(), parent_path_ci);
        Util_UTF8Upper(entry.name.data(), name_ci);
        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }


    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_updateLocalTime(
                                    const std::string& parent_path,
                                    const std::string& name,
                                    bool local_mtime_exists,
                                    u64 local_mtime)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET local_mtime=? "
            "WHERE parent_path_ci=? AND name_ci=?";
        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    } else {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET local_mtime=? "
            "WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    }
        
    // local_mtime
    if(local_mtime_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, local_mtime);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }

    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_updateCompId(
                                    const std::string& parent_path,
                                    const std::string& name,
                                    bool comp_id_exists,
                                    u64 comp_id,
                                    bool revision_exists,
                                    u64 revision)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET comp_id=?,revision=? "
            "WHERE parent_path_ci=? AND name_ci=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    } else {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET comp_id=?,revision=?,hash_value=?,file_size=? "
            "WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    }

    // comp_id
    if(comp_id_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, comp_id);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // revision
    if(revision_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, revision);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_updateEntryInfo(
                                    const std::string& parent_path,
                                    const std::string& name,
                                    bool local_mtime_exists,
                                    u64 local_mtime,
                                    bool comp_id_exists,
                                    u64 comp_id,
                                    bool revision_exists,
                                    u64 revision,
                                    const std::string& hash_value,
                                    bool file_size_exists,
                                    u64 file_size)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET local_mtime=?,comp_id=?,revision=?,hash_value=?,file_size=? "
            "WHERE parent_path_ci=? AND name_ci=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    } else {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET local_mtime=?,comp_id=?,revision=?,hash_value=?,file_size=? "
            "WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    }

    // local_mtime
    if(local_mtime_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, local_mtime);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // comp_id
    if(comp_id_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, comp_id);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // revision
    if(revision_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, revision);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // hash_value
    if (!hash_value.empty()) {
        rc = sqlite3_bind_text(stmt, ++bindPos, hash_value.data(), hash_value.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // file_size
    if (file_size_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, file_size);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_updateLastSeenInVersion(
                                            const std::string& parent_path,
                                            const std::string& name,
                                            bool last_seen_in_version_exists,
                                            u64 last_seen_in_version)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET last_seen_in_version=? "
            "WHERE parent_path_ci=? AND name_ci=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    } else {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET last_seen_in_version=? "
            "WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    }

    // last_seen_in_version
    if(last_seen_in_version_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, last_seen_in_version);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_updateVersionScanned(
                                         const std::string& parent_path,
                                         const std::string& name,
                                         bool version_scanned_exists,
                                         u64 version_scanned)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET version_scanned=? "
            "WHERE parent_path_ci=? AND name_ci=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    } else {
        static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET version_scanned=? "
            "WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
    }

    // last_seen_in_version
    if(version_scanned_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, version_scanned);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }

    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci 
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name 
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }
 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::syncHistoryTree_clearTraversalInfo()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_SYNC_HISTORY_TREE_CLEAR_TRAVERSAL_INFO =
            "UPDATE sync_history_tree "
            "SET version_scanned=NULL, last_seen_in_version=NULL";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_CLEAR_TRAVERSAL_INFO, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

//========================= SYNC HISTORY TREE TABLE ==========================
//============================================================================

//=============================================================================
//======================= DEFERRED UPLOAD JOB SET TABLE =======================
#define DEFERRED_UPLOAD_JOB_SET_COLUMNS_ADD "parent_path,"\
                                            "name,"\
                                            "last_synced_parent_comp_id,"\
                                            "last_synced_comp_id,"\
                                            "last_synced_revision_id,"\
                                            "local_mtime,"\
                                            "error_count,"\
                                            "next_error_retry_time,"\
                                            "error_reason,"\
                                            "parent_path_ci,"\
                                            "name_ci"

#define DEFERRED_UPLOAD_JOB_SET_COLUMNS_GET "row_id,"\
                                            DEFERRED_UPLOAD_JOB_SET_COLUMNS_ADD

static int deferredUploadJobSet_readRow(sqlite3_stmt* stmt,
                                        SCRow_deferredUploadJobSet& row_out)
{
    int rv = 0;
    int resIndex = 0;
    row_out.clear();

    GET_SQLITE_U64(      stmt,
                         row_out.row_id,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.parent_path,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.name,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.last_synced_parent_comp_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.last_synced_comp_id_exists,
                         row_out.last_synced_comp_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.last_synced_revision_id_exists,
                         row_out.last_synced_revision_id,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.local_mtime,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.error_count,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.next_error_retry_time_exists,
                         row_out.next_error_retry_time,
                         resIndex++, rv, end);
    GET_SQLITE_U64_NULL( stmt,
                         row_out.error_reason_exists,
                         row_out.error_reason,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.parent_path_ci_exists,
                         row_out.parent_path_ci,
                         resIndex++, rv, end);
    GET_SQLITE_STR_NULL( stmt,
                         row_out.name_ci_exists,
                         row_out.name_ci,
                         resIndex++, rv, end);
 end:
    return rv;
}

int SyncConfigDb::deferredUploadJobSet_get(const std::string& parent_path,
                                           const std::string& name,
                                           SCRow_deferredUploadJobSet& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;
    // These strings must stay in scope until the end of the function
    std::string parent_path_ci;
    std::string name_ci;

    if (m_caseInsensitive) {
        static const char* SQL_DEFERRED_UPLOAD_JOB_SET_GET =
            "SELECT "DEFERRED_UPLOAD_JOB_SET_COLUMNS_GET" "
            "FROM deferred_upload_job_set "
            "WHERE parent_path_ci=? AND name_ci=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);

        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);

        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);

    } else {
        static const char* SQL_DEFERRED_UPLOAD_JOB_SET_GET =
            "SELECT "DEFERRED_UPLOAD_JOB_SET_COLUMNS_GET" "
            "FROM deferred_upload_job_set "
            "WHERE parent_path=? AND name=?";

        CHECK_DB_HANDLE(rv, m_db, end);

        rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_GET, -1, &stmt, NULL);
        CHECK_PREPARE(rc, rv, m_db, end);
        // parent_path
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name
        rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = deferredUploadJobSet_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("deferredUploadJobSet_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::deferredUploadJobSet_getNext(SCRow_deferredUploadJobSet& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_DEFERRED_UPLOAD_JOB_SET_GET =
            "SELECT "DEFERRED_UPLOAD_JOB_SET_COLUMNS_GET" "
            "FROM deferred_upload_job_set "
            "WHERE error_count=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = deferredUploadJobSet_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("deferredUploadJobSet_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::deferredUploadJobSet_remove(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DEFERRED_UPLOAD_JOB_SET_REMOVE =
            "DELETE FROM deferred_upload_job_set WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_REMOVE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::deferredUploadJobSet_add(const std::string& parent_path,
                                           const std::string& name,
                                           u64 last_synced_parent_comp_id,
                                           bool last_synced_comp_id_exists,
                                           u64 last_synced_comp_id,
                                           bool last_synced_revision_id_exists,
                                           u64 last_synced_revision_id,
                                           u64 local_mtime)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos = 0;
    // These strings must stay in scope until the end of the function 
    std::string parent_path_ci;
    std::string name_ci;

    static const char* SQL_DEFERRED_UPLOAD_JOB_SET_ADD =
            "INSERT OR REPLACE INTO deferred_upload_job_set ("DEFERRED_UPLOAD_JOB_SET_COLUMNS_ADD") "
            "VALUES (?,?,?,?,?,?,0,NULL,NULL,?,?)";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_ADD, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // last_synced_parent_comp_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, last_synced_parent_comp_id);
    CHECK_BIND(rc, rv, m_db, end);
    // last_synced_comp_id
    if(last_synced_comp_id_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, last_synced_comp_id);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // last_synced_revision_id
    if(last_synced_revision_id_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, last_synced_revision_id);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // local_mtime
    rc = sqlite3_bind_int64(stmt, ++bindPos, local_mtime);
    CHECK_BIND(rc, rv, m_db, end);

    if (m_caseInsensitive) {
        Util_UTF8Upper(parent_path.data(), parent_path_ci);
        Util_UTF8Upper(name.data(), name_ci);
        // parent_path_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, parent_path_ci.data(), parent_path_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_text(stmt, ++bindPos, name_ci.data(), name_ci.size(), NULL);
        CHECK_BIND(rc, rv, m_db, end);
    } else {
        // parent_path_ci
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
        // name_ci
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::deferredUploadJobSet_getMaxRowId(u64& maxRowId_out)
{
    int rv = 0;
    int rc;
    int resIndex=0;
    sqlite3_stmt *stmt = NULL;
    maxRowId_out = 0;

    static const char* SQL_DEFERRED_UPLOAD_JOB_SET_MAX_ROWID =
            "SELECT MAX(row_id) FROM deferred_upload_job_set";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_MAX_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(rc == SQLITE_ROW)
    {  // Actually a row is always returned for MAX().  NULL is returned if no row exists.
        bool maxRowExists_out = false;
        GET_SQLITE_U64_NULL(stmt, maxRowExists_out, maxRowId_out, resIndex++, rv, end);
    }

 end:
    sqlite3_finalize(stmt);
    return rv;

}

int SyncConfigDb::deferredUploadJobSet_getAfterRowId(
                                        u64 afterRowId,
                                        SCRow_deferredUploadJobSet& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DEFERRED_UPLOAD_JOB_SET_GET_AFTER_ROWID =
            "SELECT "DEFERRED_UPLOAD_JOB_SET_COLUMNS_GET" "
            "FROM deferred_upload_job_set "
            "WHERE row_id>? AND error_count=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_GET_AFTER_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_bind_int64(stmt, ++bindPos, afterRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = deferredUploadJobSet_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("deferredUploadJobSet_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::deferredUploadJobSet_getErrAfterRowId(
                                         u64 afterRowId,
                                         u64 maxRowId,
                                         SCRow_deferredUploadJobSet& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DEFERRED_UPLOAD_JOB_SET_GET_ERR_AFTER_ROWID =
            "SELECT "DEFERRED_UPLOAD_JOB_SET_COLUMNS_GET" "
            "FROM deferred_upload_job_set "
            "WHERE row_id>? AND row_id<=? AND error_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DEFERRED_UPLOAD_JOB_SET_GET_ERR_AFTER_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, afterRowId);
    CHECK_BIND(rc, rv, m_db, end);

    // row_id
    rc = sqlite3_bind_int64(stmt, ++bindPos, maxRowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = deferredUploadJobSet_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("deferredUploadJobSet_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::deferredUploadJobSet_incErr(u64 rowId,
                                              u64 next_retry_time,
                                              u64 error_reason)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos = 0;

    static const char* SQL_NEED_DOWNLOAD_SCAN_INC_ERR =
            "UPDATE deferred_upload_job_set "
            "SET row_id=(SELECT MAX(row_id) FROM deferred_upload_job_set)+1,"
                "error_count=error_count+1,"
                "next_error_retry_time=?,"
                "error_reason=? "
            "WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_NEED_DOWNLOAD_SCAN_INC_ERR, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // next_retry_time
    rc = sqlite3_bind_int64(stmt, ++bindPos, next_retry_time);
    CHECK_BIND(rc, rv, m_db, end);
    // error_reason
    rc = sqlite3_bind_int64(stmt, ++bindPos, error_reason);
    CHECK_BIND(rc, rv, m_db, end);
    // rowId
    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

//======================= DEFERRED UPLOAD JOB SET TABLE =======================
//=============================================================================

//=============================================================================
//=================== ASD STAGING AREA CLEANUP TABLE ==========================

#define ASD_STAGING_AREA_CLEANUP_COLUMNS_ADD "access_url,"      \
                                             "filename,"         \
                                             "cleanup_time"

#define ASD_STAGING_AREA_CLEANUP_COLUMNS_GET "row_id,"\
                                      ASD_STAGING_AREA_CLEANUP_COLUMNS_ADD

static int asdStagingAreaCleanUp_readRow(sqlite3_stmt* stmt,
                                   SCRow_asdStagingAreaCleanUp& row_out)
{
    int rv = 0;
    int resIndex = 0;
    row_out.clear();

    GET_SQLITE_U64(      stmt,
                         row_out.row_id,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.access_url,
                         resIndex++, rv, end);
    GET_SQLITE_STR(      stmt,
                         row_out.filename,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.cleanup_time,
                         resIndex++, rv, end);
 end:
    return rv;
}

int SyncConfigDb::asdStagingAreaCleanUp_get(SCRow_asdStagingAreaCleanUp& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_ASD_STAGING_AREA_CLEANUP_GET =
            "SELECT "ASD_STAGING_AREA_CLEANUP_COLUMNS_GET" "
            "FROM asd_staging_area_cleanup "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ASD_STAGING_AREA_CLEANUP_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = asdStagingAreaCleanUp_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("asdStagingAreaCleanUp_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::asdStagingAreaCleanUp_add(const std::string& access_url,
                                            const std::string& filename,
                                            u64 cleanup_time)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_ASD_STAGING_AREA_CLEANUP_ADD =
            "INSERT INTO asd_staging_area_cleanup ("ASD_STAGING_AREA_CLEANUP_COLUMNS_ADD") "
            "VALUES (?,?,?)";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ASD_STAGING_AREA_CLEANUP_ADD, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // access_url
    rc = sqlite3_bind_text(stmt, ++bindPos, access_url.data(), access_url.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);

    // filename
    rc = sqlite3_bind_text(stmt, ++bindPos, filename.data(), filename.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);

    // cleanup_time
    rc = sqlite3_bind_int64(stmt, ++bindPos, cleanup_time);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::asdStagingAreaCleanUp_getMaxRowId(u64& maxRowId_out)
{
    int rv = 0;
    int rc;
    int resIndex=0;
    sqlite3_stmt *stmt = NULL;
    maxRowId_out = 0;

    static const char* SQL_ASD_STAGING_AREA_CLEANUP_MAX_ROWID =
            "SELECT MAX(row_id) FROM asd_staging_area_cleanup";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ASD_STAGING_AREA_CLEANUP_MAX_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(rc == SQLITE_ROW)
    {  // Actually a row is always returned for MAX().  NULL is returned if no row exists.
        bool maxRowExists_out = false;
        GET_SQLITE_U64_NULL(stmt, maxRowExists_out, maxRowId_out, resIndex++, rv, end);
    }

 end:
    sqlite3_finalize(stmt);
    return rv;

}

int SyncConfigDb::asdStagingAreaCleanUp_remove(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_ASD_STAGING_AREA_CLEANUP_REMOVE =
            "DELETE FROM asd_staging_area_cleanup WHERE row_id=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ASD_STAGING_AREA_CLEANUP_REMOVE, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_bind_int64(stmt, ++bindPos, rowId);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

    if(sqlite3_changes(m_db) == 0) {  // No rows changed
        rv = SYNC_AGENT_DB_ERR_ROW_NOT_FOUND;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDb::asdStagingAreaCleanUp_getRow(const std::string& filename,
                                               SCRow_asdStagingAreaCleanUp& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;

    static const char* SQL_ASD_STAGING_AREA_CLEANUP_GET =
            "SELECT "ASD_STAGING_AREA_CLEANUP_COLUMNS_GET" "
            "FROM asd_staging_area_cleanup "
            "WHERE filename=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_ASD_STAGING_AREA_CLEANUP_GET, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // filename
    rc = sqlite3_bind_text(stmt, ++bindPos, filename.data(), filename.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    rc = asdStagingAreaCleanUp_readRow(stmt, entry_out);
    if(rc != 0) {
        LOG_ERROR("asdStagingAreaCleanUp_readRow:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

//==================== ASD STAGING AREA CLEANUP TABLE =========================
//=============================================================================
