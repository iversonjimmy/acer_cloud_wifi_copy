//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// Old schema version to test upgrade from SyncConfigDb.cpp r41201 07/09/2013

// See http://wiki.ctbg.acer.com/wiki/index.php/CCD_Sync_Agent_Refactor_2

#include "SyncConfigDbSchemaV02.hpp"

#include "vpl_plat.h"
#include "gvm_errors.h"
#include "gvm_file_utils.hpp"
#include "util_open_db_handle.hpp"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplex_safe_conversion.h"

#include "sqlite3.h"

#include <string>
#include <log.h>

// Stringification: (first expand macro, then to string)
#define XSTR(s) STR(s)
#define STR(s) #s

// admin table will only ever have 1 row with row_id 1
#define ADMIN_ROW_ID 1
#define TARGET_SCHEMA_VERSION 2  // Incremented with each schema change

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
"BEGIN;"

// The admin table will only ever have 1 row with id==1 (ADMIN_ROW_ID).
"CREATE TABLE IF NOT EXISTS admin ("
        "row_id INTEGER PRIMARY KEY,"
        "schema_version INTEGER NOT NULL,"
        "device_id INTEGER,"               // informational only (log warning if bad)
        "user_id INTEGER,"                 // informational only (log warning if bad)
        "dataset_id INTEGER,"              // informational only (log warning if bad)
        "sync_config_id TEXT,"           // informational only (log warning if bad)
        "sync_type INTEGER,"               // informational only (log error if bad)
        "one_way_initial_scan INTEGER,"    // for one_way sync, boolean indicating initial scan status
        "last_opened_timestamp INTEGER);"  // informational only
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

        // Err handling.
        "download_err_count INTEGER NOT NULL,"
        "download_succeeded INTEGER NOT NULL,"
        "copyback_err_count INTEGER NOT NULL);"

// This table contains ordered upload tasks.
"CREATE TABLE IF NOT EXISTS upload_change_log ("
        "row_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_path TEXT NOT NULL,"  // path to parent directory relative to sync config root, empty string if root
        "name TEXT NOT NULL,"
        "is_dir INTEGER NOT NULL,"
        "upload_action INTEGER NOT NULL,"
        "comp_id INTEGER,"   // Always NULL when is_dir!=0.
        "revision INTEGER,"  // Always NULL when is_dir!=0.

        // Err handling.
        "upload_err_count INTEGER NOT NULL);"

// Keeps track of the syncing information for all synced dents
// An entry marks the point (compId,revision,localTime) where the local dent and
//   the server dent were consistent.
"CREATE TABLE IF NOT EXISTS sync_history_tree ("
        "parent_path TEXT NOT NULL," // path to parent directory relative to sync config root, empty string if root
        "name TEXT NOT NULL," // directory entry name
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
        "PRIMARY KEY(parent_path, name));"

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

SyncConfigDbSchemaV02::SyncConfigDbSchemaV02(const std::string& dbDir,
                           int syncType,
                           u64 userId,
                           u64 datasetId,
                           const std::string& syncConfigId)
:  m_db(NULL)
{
    Util_trimTrailingSlashes(dbDir, m_dbDir);

    // dbName is format <label>-<userId>-<datasetId>-<syncConfigId>.db
    char c_str[32];
    snprintf(c_str, 32, FMTu32, syncType);
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

SyncConfigDbSchemaV02::~SyncConfigDbSchemaV02()
{
    int rc = closeDb();
    if(rc != 0) {
        LOG_ERROR("Failed to close Db (%s,%s):%d",
                  m_dbDir.c_str(), m_dbName.c_str(), rc);
    }
}

int SyncConfigDbSchemaV02::openDb()
{
    bool unused_out = false;
    return openDb(unused_out);
}

int SyncConfigDbSchemaV02::privOpenDb(std::string& dbPath_out, bool &dbMissing_out)
{
    int rc;
    rc = Util_CreatePath(m_dbDir.c_str(), VPL_TRUE);
    if (rc) {
        LOG_ERROR("Failed to create directory %s:%d", m_dbDir.c_str(), rc);
        return rc;
    }

    int dberr = 0;
    Util_appendToAbsPath(m_dbDir, m_dbName, dbPath_out);
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(dbPath_out.c_str(), &statBuf);
    if(rc != 0 || statBuf.type != VPLFS_TYPE_FILE) {
        LOG_INFO("Initial scan set");
        dbMissing_out = true;
    }

    dberr = Util_OpenDbHandle(dbPath_out.c_str(), true, true, &m_db);
    if (dberr != 0) {
        LOG_ERROR("Failed to Util_OpenDbHandle(%s):%d",
                  dbPath_out.c_str(), dberr);
        return SYNC_AGENT_DB_CANNOT_OPEN;
    }

    char *errmsg;
    dberr = sqlite3_exec(m_db, SQL_CREATE_TABLES, NULL, NULL, &errmsg);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to test/create tables: %d, %s", dberr, errmsg);
        sqlite3_free(errmsg);
        return SYNC_AGENT_DB_CANNOT_OPEN;
    }
    return 0;
}

int SyncConfigDbSchemaV02::privDeleteDb(const std::string& dbPath)
{
    closeDb();
    int rv = VPLFile_Delete(dbPath.c_str());
    if (rv != 0) {
        LOG_ERROR("Failed to delete DB: %d", rv);
    }
    return rv;
}

int SyncConfigDbSchemaV02::openDb(bool &dbMissing_out)
{
    int rv = 0;

    if (!m_db) {
        std::string dbPath;
        rv = privOpenDb(dbPath, dbMissing_out);
        if (rv != 0) {
            goto end;
        }

        SCV02Row_admin adminInfo_out;
        int rc = admin_get(adminInfo_out);
        if (rc != 0) {
            LOG_ERROR("Failed to get admin info:%d", rc);
            rv = SYNC_AGENT_DB_CANNOT_OPEN;
            goto end;
        }
        if (adminInfo_out.schema_version > TARGET_SCHEMA_VERSION) {
            LOG_ERROR("Existing DB is future schema version "FMTu64" > curr(%d)",
                      adminInfo_out.schema_version, TARGET_SCHEMA_VERSION);
            rv = SYNC_AGENT_DB_FUTURE_SCHEMA;
            goto end;
        }
        else if (adminInfo_out.schema_version < TARGET_SCHEMA_VERSION) {
#if TARGET_SCHEMA_VERSION == 2
            if (adminInfo_out.schema_version == 1) {
                LOG_WARN("Previous DB schema version "FMTu64" < curr(%d). Deleting.",
                        adminInfo_out.schema_version, TARGET_SCHEMA_VERSION);
                rv = privDeleteDb(dbPath);
                if (rv != 0) {
                    goto end;
                }
                rv = privOpenDb(dbPath, dbMissing_out);
                if (rv != 0) {
                    goto end;
                }
            }
#endif
            LOG_ERROR("Previous DB schema version "FMTu64" < curr(%d).  Upgrade "
                      "not supported.",
                      adminInfo_out.schema_version, TARGET_SCHEMA_VERSION);
            rv = SYNC_AGENT_DB_UNSUPPORTED_PREV_SCHEMA;
            goto end;
        }

        // Log data
        LOG_INFO("Opened Db:%s - version:"FMTu64
                " deviceId:"FMTu64
                " user_id:"FMTu64
                " dataset_id:"FMTu64
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
                 adminInfo_out.sync_config_id_exists?
                         adminInfo_out.sync_config_id.c_str():"",
                 adminInfo_out.last_opened_timestamp_exists?
                         adminInfo_out.last_opened_timestamp:0);

        ASSERT_EQUAL(adminInfo_out.schema_version, ((u64)TARGET_SCHEMA_VERSION), FMTu64);
    }

 end:
    if (rv != 0) {
        closeDb();
    }
    return rv;
}

int SyncConfigDbSchemaV02::closeDb()
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

int SyncConfigDbSchemaV02::beginTransaction()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_BEGIN =
            "BEGIN";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_BEGIN, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDbSchemaV02::commitTransaction()
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

int SyncConfigDbSchemaV02::rollbackTransaction()
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
                          "last_opened_timestamp"
#define ADMIN_COLUMNS_GET "row_id,"\
                          ADMIN_COLUMNS_ADD
static int admin_readRow(sqlite3_stmt* stmt,
                         SCV02Row_admin& adminInfo_out)
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
                         adminInfo_out.last_opened_timestamp_exists,
                         adminInfo_out.last_opened_timestamp,
                         resIndex++, rv, end);

 end:
    return rv;
}

int SyncConfigDbSchemaV02::admin_get(SCV02Row_admin& adminInfo_out)
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

int SyncConfigDbSchemaV02::admin_set(u64 deviceId,
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
                                    SCV02Row_needDownloadScan& row_out)
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

int SyncConfigDbSchemaV02::needDownloadScan_get(
        SCV02Row_needDownloadScan& downloadScan_out)
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

int SyncConfigDbSchemaV02::needDownloadScan_remove(u64 rowId)
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


int SyncConfigDbSchemaV02::needDownloadScan_clear()
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

int SyncConfigDbSchemaV02::needDownloadScan_add(const std::string& dirPath,
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

int SyncConfigDbSchemaV02::needDownloadScan_getMaxRowId(u64& maxRowId_out)
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

int SyncConfigDbSchemaV02::needDownloadScan_incErr(u64 rowId)
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

int SyncConfigDbSchemaV02::needDownloadScan_getErr(
                            u64 maxRowId,
                            SCV02Row_needDownloadScan& errorEntry_out)
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
                                        "download_err_count,"\
                                        "download_succeeded,"\
                                        "copyback_err_count"
#define DOWNLOAD_CHANGE_LOG_COLUMNS_GET "row_id,"\
                                        DOWNLOAD_CHANGE_LOG_COLUMNS_ADD
static int downloadChangeLog_readRow(sqlite3_stmt* stmt,
                                     SCV02Row_downloadChangeLog& row_out)
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
    GET_SQLITE_U64(      stmt,
                         row_out.download_err_count,
                         resIndex++, rv, end);
    GET_SQLITE_BOOL(     stmt,
                         row_out.download_succeeded,
                         resIndex++, rv, end);
    GET_SQLITE_U64(      stmt,
                         row_out.copyback_err_count,
                         resIndex++, rv, end);
 end:
    return rv;
}

int SyncConfigDbSchemaV02::downloadChangeLog_get(
        SCV02Row_downloadChangeLog& changeLog_out)
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

int SyncConfigDbSchemaV02::downloadChangeLog_getAfterRowId(
        u64 afterRowId,
        SCV02Row_downloadChangeLog& changeLog_out)
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

int SyncConfigDbSchemaV02::downloadChangeLog_setDownloadSuccess(u64 rowId)
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

int SyncConfigDbSchemaV02::downloadChangeLog_remove(u64 rowId)
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

int SyncConfigDbSchemaV02::downloadChangeLog_clear()
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

int SyncConfigDbSchemaV02::downloadChangeLog_add(const std::string& parent_path,
                                        const std::string& name,
                                        bool is_dir,
                                        u64 download_action,
                                        u64 comp_id,
                                        u64 revision,
                                        u64 client_reported_mtime)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_DOWNLOAD_CHANGE_LOG_ADD =
            "INSERT INTO download_change_log ("DOWNLOAD_CHANGE_LOG_COLUMNS_ADD") "
            "VALUES (?,?,?,?,?,?,?,0,0,0)";

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

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDbSchemaV02::downloadChangeLog_getMaxRowId(u64& maxRowId_out)
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

int SyncConfigDbSchemaV02::downloadChangeLog_incErrDownload(u64 rowId)
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

int SyncConfigDbSchemaV02::downloadChangeLog_getErrDownload(
                                     u64 maxRowId,
                                     SCV02Row_downloadChangeLog& changeLog_out)
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

int SyncConfigDbSchemaV02::downloadChangeLog_getErrDownloadAfterRowId(
                                        u64 afterRowId,
                                        u64 maxRowId,
                                        SCV02Row_downloadChangeLog& changeLog_out)
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

int SyncConfigDbSchemaV02::downloadChangeLog_incErrCopyback(u64 rowId)
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

int SyncConfigDbSchemaV02::downloadChangeLog_getErrCopyback(
                                     u64 maxRowId,
                                     SCV02Row_downloadChangeLog& changeLog_out)
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

int SyncConfigDbSchemaV02::downloadChangeLog_getErrCopybackAfterRowId(
                                        u64 afterRowId,
                                        u64 maxRowId,
                                        SCV02Row_downloadChangeLog& changeLog_out)
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
                                      "upload_err_count"
#define UPLOAD_CHANGE_LOG_COLUMNS_GET "row_id,"\
                                      UPLOAD_CHANGE_LOG_COLUMNS_ADD

static int uploadChangeLog_readRow(sqlite3_stmt* stmt,
                                   SCV02Row_uploadChangeLog& row_out)
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
    GET_SQLITE_U64(      stmt,
                         row_out.upload_err_count,
                         resIndex++, rv, end);
 end:
    return rv;
}

int SyncConfigDbSchemaV02::uploadChangeLog_get(SCV02Row_uploadChangeLog& changeLog_out)
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

int SyncConfigDbSchemaV02::uploadChangeLog_getAfterRowId(
                                        u64 afterRowId,
                                        SCV02Row_uploadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND upload_err_count=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_GET_AFTER_ROWID, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

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

int SyncConfigDbSchemaV02::uploadChangeLog_remove(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1

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

int SyncConfigDbSchemaV02::uploadChangeLog_clear()
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;

    static const char* SQL_UPLOAD_CHANGE_LOG_CLEAR =
            "DELETE FROM upload_change_log";

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

int SyncConfigDbSchemaV02::uploadChangeLog_add(const std::string& parent_path,
                                      const std::string& name,
                                      bool is_dir,
                                      u64 upload_action,
                                      bool comp_id_exist,
                                      u64 comp_id,
                                      bool base_revision_exist,
                                      u64 base_revision)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_UPLOAD_CHANGE_LOG_ADD =
            "INSERT INTO upload_change_log ("UPLOAD_CHANGE_LOG_COLUMNS_ADD") "
            "VALUES (?,?,?,?,?,?,0)";

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

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDbSchemaV02::uploadChangeLog_getMaxRowId(u64& maxRowId_out)
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


int SyncConfigDbSchemaV02::uploadChangeLog_incErr(u64 rowId)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

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

int SyncConfigDbSchemaV02::uploadChangeLog_getErr(
                           u64 maxRowId,
                           SCV02Row_uploadChangeLog& changeLog_out)
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
        LOG_ERROR("downloadScan_out:%d", rc);
        rv = SYNC_AGENT_DB_ERR_UNEXPECTED_DATA;
        goto end;
    }

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDbSchemaV02::uploadChangeLog_getErrAfterRowId(
                                         u64 afterRowId,
                                         u64 maxRowId,
                                         SCV02Row_uploadChangeLog& changeLog_out)
{
    int rv = 0;
    int rc;
    changeLog_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID =
            "SELECT "UPLOAD_CHANGE_LOG_COLUMNS_GET" "
            "FROM upload_change_log "
            "WHERE row_id>? AND row_id<=? AND upload_err_count!=0 "
            "ORDER BY row_id LIMIT 1";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPLOAD_CHANGE_LOG_GET_ERR_AFTER_ROWID, -1, &stmt, NULL);
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

    rc = uploadChangeLog_readRow(stmt, changeLog_out);
    if(rc != 0) {
        LOG_ERROR("downloadScan_out:%d", rc);
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
                                  "version_scanned"

static int syncHistoryTree_readRow(sqlite3_stmt* stmt,
                                   SCV02Row_syncHistoryTree& row_out)
{
    int rv = 0;
    int resIndex = 0;
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
 end:
    return rv;
}

int SyncConfigDbSchemaV02::syncHistoryTree_get(const std::string& parent_path,
                                      const std::string& name,
                                      SCV02Row_syncHistoryTree& entry_out)
{
    int rv = 0;
    int rc;
    entry_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;

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

int SyncConfigDbSchemaV02::syncHistoryTree_getChildren(
                            const std::string& path,
                            std::vector<SCV02Row_syncHistoryTree>& entries_out)
{
    int rv = 0;
    int rc;
    entries_out.clear();
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;

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

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);
    CHECK_ROW_EXIST(rc, rv, m_db, end);

    while(rc == SQLITE_ROW) {
        SCV02Row_syncHistoryTree entry_out;
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

int SyncConfigDbSchemaV02::syncHistoryTree_remove(const std::string& parent_path,
                                         const std::string& name)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt=NULL;
    int bindPos=0;  // Left-most position is 1

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

int SyncConfigDbSchemaV02::syncHistoryTree_add(const SCV02Row_syncHistoryTree& entry)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_SYNC_HISTORY_TREE_ADD =
            "INSERT OR REPLACE INTO sync_history_tree ("SYNC_HISTORY_TREE_COLUMNS") "
            "VALUES (?,?,?,?,?,?,?,?)";

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

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rc, rv, m_db, end);

 end:
    sqlite3_finalize(stmt);
    return rv;
}

int SyncConfigDbSchemaV02::syncHistoryTree_updateLocalTime(
                                    const std::string& parent_path,
                                    const std::string& name,
                                    bool local_mtime_exists,
                                    u64 local_mtime)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET local_mtime=? "
            "WHERE parent_path=? AND name=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // local_mtime
    if(local_mtime_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, local_mtime);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
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

int SyncConfigDbSchemaV02::syncHistoryTree_updateCompId(
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

    static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET comp_id=?,revision=? "
            "WHERE parent_path=? AND name=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

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
    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
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

int SyncConfigDbSchemaV02::syncHistoryTree_updateEntryInfo(
                                    const std::string& parent_path,
                                    const std::string& name,
                                    bool local_mtime_exists,
                                    u64 local_mtime,
                                    bool comp_id_exists,
                                    u64 comp_id,
                                    bool revision_exists,
                                    u64 revision)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET local_mtime=?,comp_id=?,revision=? "
            "WHERE parent_path=? AND name=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

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
    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
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

int SyncConfigDbSchemaV02::syncHistoryTree_updateLastSeenInVersion(
                                            const std::string& parent_path,
                                            const std::string& name,
                                            bool last_seen_in_version_exists,
                                            u64 last_seen_in_version)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET last_seen_in_version=? "
            "WHERE parent_path=? AND name=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // last_seen_in_version
    if(last_seen_in_version_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, last_seen_in_version);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
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

int SyncConfigDbSchemaV02::syncHistoryTree_updateVersionScanned(
                                         const std::string& parent_path,
                                         const std::string& name,
                                         bool version_scanned_exists,
                                         u64 version_scanned)
{
    int rv = 0;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME =
            "UPDATE sync_history_tree "
            "SET version_scanned=? "
            "WHERE parent_path=? AND name=?";

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SYNC_HISTORY_TREE_UPDATE_LOCAL_TIME, -1, &stmt, NULL);
    CHECK_PREPARE(rc, rv, m_db, end);

    // last_seen_in_version
    if(version_scanned_exists) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, version_scanned);
        CHECK_BIND(rc, rv, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, ++bindPos);
        CHECK_BIND(rc, rv, m_db, end);
    }
    // parent_path
    rc = sqlite3_bind_text(stmt, ++bindPos, parent_path.data(), parent_path.size(), NULL);
    CHECK_BIND(rc, rv, m_db, end);
    // name
    rc = sqlite3_bind_text(stmt, ++bindPos, name.data(), name.size(), NULL);
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

int SyncConfigDbSchemaV02::syncHistoryTree_clearTraversalInfo()
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

