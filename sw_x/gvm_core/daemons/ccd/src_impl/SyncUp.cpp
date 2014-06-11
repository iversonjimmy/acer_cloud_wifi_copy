#include "SyncUp.hpp"
#include "SyncUp_private.hpp"

#include <vplex_file.h>
#include <vplex_http_util.hpp>
#include <vplu_mutex_autolock.hpp>
#include <scopeguard.hpp>

#include <gvm_errors.h>
#include <gvm_file_utils.h>
#include <util_open_db_handle.hpp>
#include <log.h>
#include <cJSON2.h>

#include "EventManagerPb.hpp"
#include "ans_connection.hpp"
#include "ccd_storage.hpp"
#include "config.h"
#include "s3_proxy.hpp"
#include "vcs_utils.hpp"

#define SYNCUP_THREAD_STACK_SIZE (128 * 1024)
#define SYNCUP_COPY_FILE_BUFFER_SIZE (4 * 1024)

// Target DB schema version.
#define TARGET_SCHEMA_VERSION 2

#define OK_STOP_DB_TRAVERSE 1

#define NUM_UPLOAD_RETRY 3

// for stringification
#define XSTR(s) STR(s)
#define STR(s) #s

// Starting with v2.5, tmppath will be a path relative to the workdir.
// This function is used to extend it to a full path at runtime.
// E.g., "tmp/su_123" -> "/aaa/bbb/ccdir/su/tmp/su_123"
// For backward compatibility (i.e., tmppath is full path), this function will replace the prefix part.
// E.g., "/zzz/yyy/tmp/su_123" -> "/aaa/bbb/ccdir/su/tmp/su_123"
static void extendTmpPathToFullPath(const std::string &tmppath, const std::string &workdir, std::string &fullpath)
{
    size_t pos = tmppath.rfind("tmp/");
    fullpath.assign(workdir);
    fullpath.append(tmppath, pos, std::string::npos);
}

int SyncUpJob::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    id          = (u64)sqlite3_column_int64(stmt, n++);
    opath       = (const char*)sqlite3_column_text(stmt, n++);
    spath       = (const char*)sqlite3_column_text(stmt, n++);
    is_tmp      = sqlite3_column_int(stmt, n++) == 1;
    ctime       = (u64)sqlite3_column_int64(stmt, n++);
    mtime       = (u64)sqlite3_column_int64(stmt, n++);
    did         = (u64)sqlite3_column_int64(stmt, n++);
    cpath       = (const char*)sqlite3_column_text(stmt, n++);
    feature     = (u64)sqlite3_column_int64(stmt, n++);

    return 0;
}

//----------------------------------------------------------------------

int SyncUpJobEx::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    id          = (u64)sqlite3_column_int64(stmt, n++);
    opath       = (const char*)sqlite3_column_text(stmt, n++);
    spath       = (const char*)sqlite3_column_text(stmt, n++);
    is_tmp      = sqlite3_column_int(stmt, n++) == 1;
    ctime       = (u64)sqlite3_column_int64(stmt, n++);
    mtime       = (u64)sqlite3_column_int64(stmt, n++);
    did         = (u64)sqlite3_column_int64(stmt, n++);
    cpath       = (const char*)sqlite3_column_text(stmt, n++);
    add_ts      = (u64)sqlite3_column_int64(stmt, n++);
    disp_ts     = (u64)sqlite3_column_int64(stmt, n++);
    ul_try_ts   = (u64)sqlite3_column_int64(stmt, n++);
    ul_failed   = (u64)sqlite3_column_int64(stmt, n++);
    ul_done_ts  = (u64)sqlite3_column_int64(stmt, n++);
    feature     = (u64)sqlite3_column_int64(stmt, n++);

    return 0;
}

static bool isSqliteError(int ec)
{
    return ec != SQLITE_OK && ec != SQLITE_ROW && ec != SQLITE_DONE;
}

#define CHECK_PREPARE(dberr, result, db, lbl)                           \
if (isSqliteError(dberr)) {                                             \
    result = mapSqliteErrCode(dberr);                                   \
    LOG_ERROR("Failed to prepare SQL stmt: %d, %s", result, sqlite3_errmsg(db)); \
    goto lbl;                                                           \
}

#define CHECK_BIND(dberr, result, db, lbl)                              \
if (isSqliteError(dberr)) {                                             \
    result = mapSqliteErrCode(dberr);                                   \
    LOG_ERROR("Failed to bind value in prepared stmt: %d, %s", result, sqlite3_errmsg(db)); \
    goto lbl;                                                           \
}

#define CHECK_STEP(dberr, result, db, lbl)                              \
if (isSqliteError(dberr)) {                                             \
    result = mapSqliteErrCode(dberr);                                   \
    LOG_ERROR("Failed to execute prepared stmt: %d, %s", result, sqlite3_errmsg(db)); \
    goto lbl;                                                           \
}

#define CHECK_FINALIZE(dberr, result, db, lbl)                          \
if (isSqliteError(dberr)) {                                             \
    result = mapSqliteErrCode(dberr);                                   \
    LOG_ERROR("Failed to finalize prepared stmt: %d, %s", result, sqlite3_errmsg(db)); \
    goto lbl;                                                           \
}

#define CHECK_ERR(err, result, lbl)             \
if ((err) != 0) {                               \
    result = err;                               \
    if(isSqliteError(err) && err!=CCD_ERROR_NOT_FOUND) { \
        LOG_ERROR("db error:%d", err);          \
    }                                           \
    goto lbl;                                   \
}

#define CHECK_RESULT(result, lbl)               \
if ((result) != 0) {                            \
    goto lbl;                                   \
}



#define SQLITE_ERROR_BASE 0
static int mapSqliteErrCode(int ec)
{
    return SQLITE_ERROR_BASE - ec;
}

#define ADMINID_SCHEMA_VERSION 1

// Below is the v1 (Before CCD SDK 2.6.0) database schema for PicStream and Cloud Docs
// However from CCD SDK 2.5.4 RC1, Cloud Docs applies DocTrackerDB, which is implemented in CloudDocMgr.cpp
// Only difference between test_and_create_db_sql_v1 and test_and_create_db_sql_v2
// is the addition of the feature field in the su_job table.
//static const char test_and_create_db_sql_v1[] =
//"BEGIN;"
//"CREATE TABLE IF NOT EXISTS su_admin ("
//    "id INTEGER PRIMARY KEY, "
//    "value INTEGER NOT NULL);"
//"INSERT OR IGNORE INTO su_admin VALUES ("XSTR(ADMINID_SCHEMA_VERSION)", "XSTR(1)");"
//"CREATE TABLE IF NOT EXISTS su_job ("
//    "id INTEGER PRIMARY KEY AUTOINCREMENT, "          // job ID
//
//    "opath STRING NOT NULL, "           // original path of source file on local device
//    "spath STRING NOT NULL, "           // source file path for upload. this could be the same as above, or a tmp file (tmp path is relative path)
//    "is_tmp INTEGER NOT NULL, "         // whether spath is tmp file; 1=yes, 0=no
//    "ctime INTEGER, "                   // createTime, passed to POST filemetadata
//    "mtime INTEGER, "                   // lastModified, passed to POST filemetadata
//
//    "did INTEGER NOT NULL, "            // destination dataset ID
//    "cpath STRING NOT NULL, "           // destination component path
//
//    "add_ts INTEGER NOT NULL, "         // time when job was added
//
//    "disp_ts INTEGER, "                 // time when job was dispatched
//
//    "ul_try_ts INTEGER, "               // time when upload was last tried; null if never
//    "ul_failed INTEGER DEFAULT 0, "     // number of times upload failed
//    "ul_done_ts INTEGER, "              // time when upload succeeded; null if not yet
//
//    "UNIQUE (did, cpath));"             // sanity check
//"COMMIT;";

static const char test_and_create_db_sql_v2[] =
"BEGIN;"
"CREATE TABLE IF NOT EXISTS su_admin ("
    "id INTEGER PRIMARY KEY, "
    "value INTEGER NOT NULL);"
"INSERT OR IGNORE INTO su_admin VALUES ("XSTR(ADMINID_SCHEMA_VERSION)", "XSTR(TARGET_SCHEMA_VERSION)");"
"CREATE TABLE IF NOT EXISTS su_job ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "          // job ID

    "opath TEXT NOT NULL, "           // original path of source file on local device
    "spath TEXT NOT NULL, "           // source file path for upload. this could be the same as above, or a tmp file (tmp path is relative path)
    "is_tmp INTEGER NOT NULL, "         // whether spath is tmp file; 1=yes, 0=no
    "ctime INTEGER, "                   // createTime, passed to POST filemetadata
    "mtime INTEGER, "                   // lastModified, passed to POST filemetadata

    "did INTEGER NOT NULL, "            // destination dataset ID
    "cpath TEXT NOT NULL, "           // destination component path

    "add_ts INTEGER NOT NULL, "         // VPLtime when job was added

    "disp_ts INTEGER, "                 // VPLtime when job was dispatched

    "ul_try_ts INTEGER, "               // VPLtime when upload was last tried; null if never
    "ul_failed INTEGER DEFAULT 0, "     // number of times upload failed
    "ul_done_ts INTEGER, "              // VPLtime when upload succeeded; null if not yet
    "feature INTEGER, "                 // identify the sync feature that the job belonged

    "UNIQUE (did, cpath));"             // sanity check
"COMMIT;";

#define DEFSQL(tag, def) static const char sql_ ## tag [] = def
#define SQLNUM(tag) STMT_ ## tag
#define GETSQL(tag) sql_ ## tag

enum {
    STMT_BEGIN,
    STMT_COMMIT,
    STMT_ROLLBACK,
    STMT_ADD_JOB,
    STMT_REMOVE_JOB_BY_ID,
    STMT_REMOVE_JOBS_BY_DATASETID,
    STMT_REMOVE_JOBS_BY_DATASETID_OPATH_PREFIX,
    STMT_REMOVE_ALL_JOBS,
    STMT_GET_JOB,
    STMT_GET_JOB_BY_ID,
    STMT_GET_JOB_BY_DATASETID,
    STMT_GET_JOB_BY_DATASETID_FAILED,
    STMT_GET_JOB_BY_DATASETID_OPATH_PREFIX,
    STMT_GET_JOB_BY_DATASETID_OPATH_PREFIX_FAILED,
    STMT_GET_JOB_BY_DATASETID_COMPPATH,
    STMT_GET_JOB_NOT_DONE_DISPATCH_BEFORE,
    STMT_GET_JOB_WITH_TMP,
    STMT_GET_JOBEX_BY_ID,
    STMT_SET_JOB_DISPATCH_TIME,
    STMT_SET_JOB_TRY_UPLOAD_TIME,
    STMT_SET_JOB_DONE_UPLOAD_TIME,
    STMT_INC_JOB_UPLOAD_FAILED_COUNT,
    STMT_GET_ADMIN_VALUE,
    STMT_GET_JOB_COUNT_BY_FEATURE,
    STMT_GET_FAILED_JOB_COUNT_BY_FEATURE,
    STMT_MAX, // this must be last
};

// columns needed to populate SyncUpJob struct
#define JOB_COLS "id,"      \
                 "opath,"   \
                 "spath,"   \
                 "is_tmp,"  \
                 "ctime,"   \
                 "mtime,"   \
                 "did,"     \
                 "cpath,"   \
                 "feature"

// columns needed to populate SyncUpJobEx struct
#define JOBEX_COLS "id,"         \
                   "opath,"      \
                   "spath,"      \
                   "is_tmp,"     \
                   "ctime,"      \
                   "mtime,"      \
                   "did,"        \
                   "cpath,"      \
                   "add_ts,"     \
                   "disp_ts,"    \
                   "ul_try_ts,"  \
                   "ul_failed,"  \
                   "ul_done_ts," \
                   "feature"

DEFSQL(BEGIN,                                   \
       "BEGIN IMMEDIATE");
DEFSQL(COMMIT,                                  \
       "COMMIT");
DEFSQL(ROLLBACK,                                \
       "ROLLBACK");
DEFSQL(ADD_JOB,                                                         \
       "INSERT INTO su_job (opath, spath, is_tmp, ctime, mtime, did, cpath, add_ts, feature) " \
       "VALUES (:opath, :spath, :is_tmp, :ctime, :mtime, :did, :cpath, :add_ts, :feature)");
DEFSQL(REMOVE_JOB_BY_ID,                        \
       "DELETE FROM su_job "                    \
       "WHERE id=:id");
DEFSQL(REMOVE_JOBS_BY_DATASETID,                \
       "DELETE FROM su_job "                    \
       "WHERE did=:did");
DEFSQL(REMOVE_JOBS_BY_DATASETID_OPATH_PREFIX,           \
       "DELETE FROM su_job "                            \
       "WHERE did=:did AND opath GLOB :prefix||'*'");
DEFSQL(REMOVE_ALL_JOBS,                         \
       "DELETE FROM su_job");
DEFSQL(GET_JOB,                                 \
       "SELECT "JOB_COLS" "                     \
       "FROM su_job");
DEFSQL(GET_JOB_BY_ID,                           \
       "SELECT "JOB_COLS" "                     \
       "FROM su_job "                           \
       "WHERE id=:id");
DEFSQL(GET_JOB_BY_DATASETID,                    \
       "SELECT "JOB_COLS" "                     \
       "FROM su_job "                           \
       "WHERE did=:did");
DEFSQL(GET_JOB_BY_DATASETID_FAILED,             \
       "SELECT "JOB_COLS" "                     \
       "FROM su_job "                           \
       "WHERE did=:did AND ul_failed > 0");
DEFSQL(GET_JOB_BY_DATASETID_OPATH_PREFIX,       \
       "SELECT "JOB_COLS" "                     \
       "FROM su_job "                           \
       "WHERE did=:did AND opath GLOB :prefix||'*'");
DEFSQL(GET_JOB_BY_DATASETID_OPATH_PREFIX_FAILED,\
       "SELECT "JOB_COLS" "                     \
       "FROM su_job "                           \
       "WHERE did=:did AND ul_failed > 0 AND opath GLOB :prefix||'*'");
DEFSQL(GET_JOB_BY_DATASETID_COMPPATH,           \
       "SELECT "JOB_COLS" "                     \
       "FROM su_job "                           \
       "WHERE did=:did AND cpath=:cpath");
DEFSQL(GET_JOB_NOT_DONE_DISPATCH_BEFORE,                                \
       "SELECT "JOB_COLS" "                                             \
       "FROM su_job "                                                   \
       "WHERE ul_done_ts IS NULL AND (disp_ts IS NULL OR disp_ts<:ts)");
DEFSQL(GET_JOB_WITH_TMP,                        \
       "SELECT "JOB_COLS" "                     \
       "FROM su_job "                           \
       "WHERE is_tmp=1");
DEFSQL(GET_JOBEX_BY_ID,                         \
       "SELECT "JOBEX_COLS" "                   \
       "FROM su_job "                           \
       "WHERE id=:id");
DEFSQL(SET_JOB_DISPATCH_TIME,                   \
       "UPDATE su_job "                         \
       "SET disp_ts=:ts "                       \
       "WHERE id=:id");
DEFSQL(SET_JOB_TRY_UPLOAD_TIME,                 \
       "UPDATE su_job "                         \
       "SET ul_try_ts=:ts "                     \
       "WHERE id=:id");
DEFSQL(SET_JOB_DONE_UPLOAD_TIME,                \
       "UPDATE su_job "                         \
       "SET ul_done_ts=:ts "                    \
       "WHERE id=:id");
DEFSQL(INC_JOB_UPLOAD_FAILED_COUNT,             \
       "UPDATE su_job "                         \
       "SET ul_failed=ul_failed+1 "             \
       "WHERE id=:id");
DEFSQL(GET_ADMIN_VALUE,                         \
       "SELECT value "                          \
       "FROM su_admin "                         \
       "WHERE id=:id");
DEFSQL(GET_JOB_COUNT_BY_FEATURE,                \
       "SELECT COUNT(*) "                       \
       "FROM su_job "                           \
       "WHERE feature=:feature AND (disp_ts IS NULL OR disp_ts<=:ts)");
DEFSQL(GET_FAILED_JOB_COUNT_BY_FEATURE,         \
       "SELECT COUNT(*) "                       \
       "FROM su_job "                           \
       "WHERE feature=:feature AND (ul_failed > 0) AND disp_ts>:ts");

SyncUpJobs::SyncUpJobs(const std::string &workdir)
    : workdir(workdir), db(NULL), purge_on_close(false)
{
    tmpdir = workdir + "tmp/";
    dbpath = workdir + "db";

    int err = VPLMutex_Init(&mutex);
    if (err) {
        LOG_ERROR("Failed to initialize mutex: %d", err);
    }
}

SyncUpJobs::~SyncUpJobs()
{
    int err = closeDB();
    if (err) {
        LOG_ERROR("Failed to close DB: %d", err);
    }
    err = VPLMutex_Destroy(&mutex);
    if (err) {
        LOG_ERROR("Failed to destroy mutex: %d", err);
    }
}

int SyncUpJobs::migrate_schema_from_v1_to_v2()
{
    int result = 0;
    int dberr = 0;
    char *errmsg = NULL;

    // Set feature to SYNC_FEATURE_PICSTREAM_UPLOAD = 40
    static const char* SQL_MIGRATE_SCHEMA_FROM_V1_TO_V2 =
        "ALTER TABLE su_job ADD feature INTEGER;"   \
        "UPDATE su_job SET feature=40;"             \
        "UPDATE su_admin SET value="XSTR(TARGET_SCHEMA_VERSION)" WHERE id="XSTR(ADMINID_SCHEMA_VERSION)"";
    
    dberr = sqlite3_exec(db, SQL_MIGRATE_SCHEMA_FROM_V1_TO_V2, NULL, NULL, &errmsg);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to migrate schema from v1 to v2: %d, %s", dberr, errmsg);
        sqlite3_free(errmsg);
        result = mapSqliteErrCode(dberr);
    }
    return result;
}

int SyncUpJobs::openDB()
{
    int result = 0;

    if (!db) {
        int err = 0;
        err = Util_CreatePath(dbpath.c_str(), VPL_FALSE);
        if (err) {
            LOG_ERROR("Failed to create directory for %s", dbpath.c_str());
            result = err;
            goto end;
        }

        int dberr = 0;
        dberr = Util_OpenDbHandle(dbpath.c_str(), true, true, &db);
        if (dberr != 0) {
            LOG_ERROR("Util_OpenDbHandle(%s):%d",
                      dbpath.c_str(), dberr);
            result = mapSqliteErrCode(dberr);
            goto end;
        }

        char *errmsg;
        dberr = sqlite3_exec(db, test_and_create_db_sql_v2, NULL, NULL, &errmsg);
        if (dberr != SQLITE_OK) {
            LOG_ERROR("Failed to test/create tables: %d, %s", dberr, errmsg);
            sqlite3_free(errmsg);
            result = mapSqliteErrCode(dberr);
            goto end;
        }

        dbstmts.resize(STMT_MAX, NULL);

        u64 schemaVersion = 0;
        err = getAdminValue(ADMINID_SCHEMA_VERSION, schemaVersion);
        if (err) {
            LOG_ERROR("Failed to determine current schema version: %d", err);
            result = err;
            goto end;
        }
        if (schemaVersion > TARGET_SCHEMA_VERSION) {
            LOG_ERROR("Unknown schema version "FMTu64, schemaVersion);
            result = CCD_ERROR_NOT_IMPLEMENTED;
            goto end;
        }
        else if (schemaVersion == 1) {
            // Need to Migrate DB since the current DB version is smaller than TARGET_SCHEMA_VERSION
            result = migrate_schema_from_v1_to_v2();
            if (result) {
                LOG_ERROR("Failed to migrate sync up DB from v1 to v2: %d", result);
                goto end;
            }
        }
        else if (schemaVersion < TARGET_SCHEMA_VERSION) {
            LOG_ERROR("Unhandled schema version: "FMTu64, schemaVersion);
            result = CCD_ERROR_NOT_IMPLEMENTED;
            goto end;
        }
        else {
            // assert: schemaVersion == TARGET_SCHEMA_VERSION
            // nothing to do
        }
    }

 end:
    if (result) {
        closeDB();
    }
    return result;
}

int SyncUpJobs::closeDB()
{
    int result = 0;

    if (db != NULL) {
        std::vector<sqlite3_stmt*>::iterator it;
        for (it = dbstmts.begin(); it != dbstmts.end(); it++) {
            if (*it) {
                sqlite3_finalize(*it);
            }
            *it = NULL;
        }

        sqlite3_close(db);
        db = NULL;
        LOG_DEBUG("Closed DB");
    }

    if (purge_on_close) {
        int vplerr = VPLFile_Delete(dbpath.c_str());
        if (!vplerr) {
            LOG_INFO("Deleted db file");
        }
        else {
            LOG_ERROR("Failed to delete db file: %d", vplerr);
            // don't propagate the error
        }
    }

    return result;
}

int SyncUpJobs::beginTransaction()
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(BEGIN)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(BEGIN), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::commitTransaction()
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COMMIT)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(COMMIT), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::rollbackTransaction()
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ROLLBACK)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(ROLLBACK), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::getAdminValue(u64 adminId, u64 &value)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_ADMIN_VALUE)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_ADMIN_VALUE), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)adminId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        value = (u64)sqlite3_column_int64(stmt, 0);
    }
    else {  // SQLITE_DONE, meaning not found
        result = CCD_ERROR_NOT_FOUND;  // to indicate value missing; note that this is a valid outcome
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    return result;
}

// precondition: (datasetid,comppath) does not exist
int SyncUpJobs::addJob(const std::string &opath, const std::string &spath, bool is_tmp, u64 ctime, u64 mtime,
                       u64 did, const std::string &cpath,
                       u64 timestamp, u64 syncfeature, /*OUT*/ u64 &jobid)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ADD_JOB)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(ADD_JOB), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    {
        int n = 1;
        // bind :opath
        dberr = sqlite3_bind_text(stmt, n++, opath.data(), opath.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // bind :spath
        dberr = sqlite3_bind_text(stmt, n++, spath.data(), spath.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // bind :is_tmp
        dberr = sqlite3_bind_int(stmt, n++, is_tmp ? 1 : 0);
        CHECK_BIND(dberr, result, db, end);
        // bind :ctime
        dberr = sqlite3_bind_int64(stmt, n++, (s64)ctime);
        CHECK_BIND(dberr, result, db, end);
        // bind :mtime
        dberr = sqlite3_bind_int64(stmt, n++, (s64)mtime);
        CHECK_BIND(dberr, result, db, end);
        // bind :did
        dberr = sqlite3_bind_int64(stmt, n++, (s64)did);
        CHECK_BIND(dberr, result, db, end);
        // bind :cpath
        dberr = sqlite3_bind_text(stmt, n++, cpath.data(), cpath.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // bind :add_ts
        dberr = sqlite3_bind_int64(stmt, n++, (s64)timestamp);
        CHECK_BIND(dberr, result, db, end);
        // bind :syncfeature
        dberr = sqlite3_bind_int64(stmt, n++, (s64)syncfeature);
        CHECK_BIND(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    jobid = sqlite3_last_insert_rowid(db);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::removeJob(u64 jobid)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_JOB_BY_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_JOB_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)jobid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::removeJobsByDatasetId(u64 datasetId)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_JOBS_BY_DATASETID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_JOBS_BY_DATASETID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::removeJobsByDatasetIdOpathPrefix(u64 datasetId, const std::string &prefix)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_JOBS_BY_DATASETID_OPATH_PREFIX)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_JOBS_BY_DATASETID_OPATH_PREFIX), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetId);
    CHECK_BIND(dberr, result, db, end);

    // bind :prefix
    dberr = sqlite3_bind_text(stmt, 2, prefix.data(), prefix.size(), NULL);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::removeAllJobs()
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_ALL_JOBS)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_ALL_JOBS), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::findJob(u64 jobid, /*OUT*/ SyncUpJob &job)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_BY_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)jobid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        job.populateFrom(stmt);
    }
    else {  // assert: dberr == SQLITE_DONE
        result = CCD_ERROR_NOT_FOUND;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::findJob(u64 datasetid, const std::string &comppath, /*OUT*/ SyncUpJob &job)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_BY_DATASETID_COMPPATH)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_DATASETID_COMPPATH), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetid);
    CHECK_BIND(dberr, result, db, end);
    // bind :cpath
    dberr = sqlite3_bind_text(stmt, 2, comppath.data(), comppath.size(), NULL);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        job.populateFrom(stmt);
    }
    else {  // assert: dberr == SQLITE_DONE
        result = CCD_ERROR_NOT_FOUND;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::findJobNotDoneNoRecentDispatch(u64 cutoff, /*OUT*/ SyncUpJob &job)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_NOT_DONE_DISPATCH_BEFORE)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_NOT_DONE_DISPATCH_BEFORE), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :ts
    dberr = sqlite3_bind_int64(stmt, 1, (s64)cutoff);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        job.populateFrom(stmt);
    }
    else {  // assert: dberr == SQLITE_DONE
        result = CCD_ERROR_NOT_FOUND;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    return result;
}

int SyncUpJobs::findJobEx(u64 jobid, /*OUT*/ SyncUpJobEx &jobex)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOBEX_BY_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOBEX_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)jobid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        jobex.populateFrom(stmt);
    }
    else {  // assert: dberr == SQLITE_DONE
        result = CCD_ERROR_NOT_FOUND;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::visitJobs(JobVisitor visitor, void *param)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        SyncUpJob job;
        job.populateFrom(stmt);
        int err = visitor(job, param);
        if(err == OK_STOP_DB_TRAVERSE) {
            goto end;
        }
        if (err) {
            LOG_ERROR("visitor returned error %d", err);
            result = err;
            goto end;
        }
    }
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::visitJobsByDatasetId(u64 datasetId, bool fails_only,
                                     JobVisitor visitor, void *param)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt **stmtChoice;
    sqlite3_stmt *stmt = NULL;
    if(fails_only) {
        sqlite3_stmt *&stmtInit = dbstmts[SQLNUM(GET_JOB_BY_DATASETID_FAILED)];
        if (!stmtInit) {
            dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_DATASETID_FAILED), -1, &stmtInit, NULL);
            CHECK_PREPARE(dberr, result, db, end);
        }
        stmtChoice = &dbstmts[SQLNUM(GET_JOB_BY_DATASETID_FAILED)];
    }else{
        sqlite3_stmt *&stmtInit = dbstmts[SQLNUM(GET_JOB_BY_DATASETID)];
        if (!stmtInit) {
            dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_DATASETID), -1, &stmtInit, NULL);
            CHECK_PREPARE(dberr, result, db, end);
        }
        stmtChoice = &dbstmts[SQLNUM(GET_JOB_BY_DATASETID)];
    }

    stmt = *stmtChoice;

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetId);
    CHECK_BIND(dberr, result, db, end);

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        SyncUpJob job;
        job.populateFrom(stmt);
        int err = visitor(job, param);
        if(err == OK_STOP_DB_TRAVERSE) {
            goto end;
        }
        if (err) {
            LOG_ERROR("visitor returned error %d", err);
            result = err;
            goto end;
        }
    }
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncUpJobs::visitJobsByDatasetIdOpathPrefix(u64 datasetId,
                                                const std::string &prefix,
                                                bool fails_only,
                                                JobVisitor visitor,
                                                void *param)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt **stmtChoice;
    sqlite3_stmt *stmt = NULL;
    if(fails_only) {
        sqlite3_stmt *&stmtInit = dbstmts[SQLNUM(GET_JOB_BY_DATASETID_OPATH_PREFIX_FAILED)];
        if (!stmtInit) {
            dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_DATASETID_OPATH_PREFIX_FAILED), -1, &stmtInit, NULL);
            CHECK_PREPARE(dberr, result, db, end);
        }
        stmtChoice = &dbstmts[SQLNUM(GET_JOB_BY_DATASETID_OPATH_PREFIX_FAILED)];
    }else{
        sqlite3_stmt *&stmtInit = dbstmts[SQLNUM(GET_JOB_BY_DATASETID_OPATH_PREFIX)];
        if (!stmtInit) {
            dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_DATASETID_OPATH_PREFIX), -1, &stmtInit, NULL);
            CHECK_PREPARE(dberr, result, db, end);
        }
        stmtChoice = &dbstmts[SQLNUM(GET_JOB_BY_DATASETID_OPATH_PREFIX)];
    }

    stmt = *stmtChoice;

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetId);
    CHECK_BIND(dberr, result, db, end);

    // bind :prefix
    dberr = sqlite3_bind_text(stmt, 2, prefix.data(), prefix.size(), NULL);
    CHECK_BIND(dberr, result, db, end);

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        SyncUpJob job;
        job.populateFrom(stmt);
        int err = visitor(job, param);
        if(err == OK_STOP_DB_TRAVERSE) {
            goto end;
        }
        if (err) {
            LOG_ERROR("visitor returned error %d", err);
            result = err;
            goto end;
        }
    }
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

#define setJobTime(EventName,EVENT_NAME)                                \
int SyncUpJobs::setJob##EventName##Time(u64 jobid, u64 timestamp)       \
{                                                                       \
    int result = 0;                                                     \
    int dberr = 0;                                                      \
                                                                        \
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_JOB_##EVENT_NAME##_TIME)]; \
    if (!stmt) {                                                        \
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_JOB_##EVENT_NAME##_TIME), -1, &stmt, NULL); \
        CHECK_PREPARE(dberr, result, db, end);                          \
    }                                                                   \
                                                                        \
    dberr = sqlite3_bind_int64(stmt, 1, (s64)timestamp);                \
    CHECK_BIND(dberr, result, db, end);                                 \
    dberr = sqlite3_bind_int64(stmt, 2, (s64)jobid);                    \
    CHECK_BIND(dberr, result, db, end);                                 \
                                                                        \
    dberr = sqlite3_step(stmt);                                         \
    CHECK_STEP(dberr, result, db, end);                                 \
                                                                        \
    switch (sqlite3_changes(db)) {                                      \
    case 0:                                                             \
        result = CCD_ERROR_NOT_FOUND;                                   \
        break;                                                          \
    case 1:  /* expected */                                             \
        break;                                                          \
    default:  /* SHOULD NOT HAPPEN - "id" is primary key; at most one row can change. */ \
        result = CCD_ERROR_INTERNAL;                                    \
    }                                                                   \
                                                                        \
 end:                                                                   \
    if (stmt) {                                                         \
        sqlite3_reset(stmt);                                            \
    }                                                                   \
    return result;                                                      \
}

// define setJobDispatchTime()
// uses SET_JOB_DISPATCH_TIME
setJobTime(Dispatch,DISPATCH)
// define setJobTryUploadTime()
// uses SET_JOB_TRY_UPLOAD_TIME
setJobTime(TryUpload,TRY_UPLOAD)
// define setJobDoneUploadTime()
// uses SET_JOB_DONE_UPLOAD_TIME
setJobTime(DoneUpload,DONE_UPLOAD)

#undef setJobTime

#define incJobFailedCount(Operation,OPERATION)                          \
int SyncUpJobs::incJob##Operation##FailedCount(u64 jobid)               \
{                                                                       \
    int result = 0;                                                     \
    int dberr = 0;                                                      \
                                                                        \
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(INC_JOB_##OPERATION##_FAILED_COUNT)]; \
    if (!stmt) {                                                        \
        dberr = sqlite3_prepare_v2(db, GETSQL(INC_JOB_##OPERATION##_FAILED_COUNT), -1, &stmt, NULL); \
        CHECK_PREPARE(dberr, result, db, end);                          \
    }                                                                   \
                                                                        \
    dberr = sqlite3_bind_int64(stmt, 1, (s64)jobid);                    \
    CHECK_BIND(dberr, result, db, end);                                 \
                                                                        \
    dberr = sqlite3_step(stmt);                                         \
    CHECK_STEP(dberr, result, db, end);                                 \
                                                                        \
    switch (sqlite3_changes(db)) {                                      \
    case 0:                                                             \
        result = CCD_ERROR_NOT_FOUND;                                   \
        break;                                                          \
    case 1:  /* expected */                                             \
        break;                                                          \
    default:  /* SHOULD NOT HAPPEN - "id" is primary key; at most one row can change. */ \
        result = CCD_ERROR_INTERNAL;                                    \
    }                                                                   \
                                                                        \
 end:                                                                   \
    if (stmt) {                                                         \
        sqlite3_reset(stmt);                                            \
    }                                                                   \
    return result;                                                      \
}

// define incJobUploadFailedCount()
// uses INC_JOB_UPLOAD_FAILED_COUNT
incJobFailedCount(Upload,UPLOAD)

#undef incJobFailedCount

int SyncUpJobs::setPurgeOnClose(bool purge)
{
    purge_on_close = purge;

    return 0;
}

int SyncUpJobs::Check()
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

 end:
    return result;
}

static int deleteTmpFile(SyncUpJob &job, void *param)
{
    if (job.is_tmp) {
        std::string &workdir = *(std::string*)param;
        std::string spath;
        extendTmpPathToFullPath(job.spath, workdir, spath);
        int err = VPLFile_Delete(spath.c_str());
        if (err) {
            LOG_WARN("Failed to delete tmp file %s: %d, Continuing traversal",
                     spath.c_str(), err);
        }
    }
    return 0;
}

int SyncUpJobs::AddJob(const std::string &localpath, bool make_copy, u64 ctime, u64 mtime,
                       u64 datasetid, const std::string &comppath, u64 syncfeature,
                       /*OUT*/ u64 &jobid)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
      int err = 0;
      err = openDB();
      CHECK_ERR(err, result, end);
      err = beginTransaction();
      CHECK_ERR(err, result, end);

      // There can be at most one job with a given (datasetid,comppath).
      // Thus, if there is already a job with the same (datasetid,comppath), remove.
      SyncUpJob job;
      err = findJob(datasetid, comppath, job);
      if (err == CCD_ERROR_NOT_FOUND) {
          goto no_prior_job;
      }
      CHECK_ERR(err, result, end);
      deleteTmpFile(job, &workdir);
      err = removeJob(job.id);
      CHECK_ERR(err, result, end);

    no_prior_job:
      std::string tmppath;
      if (make_copy) {
          // open source file
          VPLFile_handle_t fin = VPLFile_Open(localpath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
          if (!VPLFile_IsValidHandle(fin)) {
              LOG_ERROR("Failed to open %s for read", localpath.c_str());
              result = fin < 0 ? (int)fin : CCD_ERROR_FOPEN;
              goto end;
          }
          ON_BLOCK_EXIT(VPLFile_Close, fin);

          // generate filename for destination
          static int tmpfile_counter = 0;
          std::ostringstream oss;
          oss << VPLTime_GetTime() << "-" << tmpfile_counter++;
          // To test Bug 8267, temporarily change the string below to anything that ends with "tmp/"
          // E.g., tmppath = "/a/b/c/d/e/tmp/" + oss.str();
          tmppath = "tmp/" + oss.str();

          std::string fullpath;
          extendTmpPathToFullPath(tmppath, workdir, fullpath);

          // make sure destination directory exists
          err = Util_CreatePath(fullpath.c_str(), VPL_FALSE);
          if (err) {
              LOG_ERROR("Failed to create directory for %s", fullpath.c_str());
              result = err;
              goto end;
          }

          // open destination file
          VPLFile_handle_t fout = VPLFile_Open(fullpath.c_str(),
                                               VPLFILE_OPENFLAG_CREATE|VPLFILE_OPENFLAG_WRITEONLY,
                                               VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR);
          if (!VPLFile_IsValidHandle(fout)) {
              LOG_ERROR("Failed to open %s for write:%d", fullpath.c_str(), (int)fout);
              result = fin < 0 ? (int)fin : CCD_ERROR_FOPEN;
              goto end;
          }
          ON_BLOCK_EXIT(VPLFile_Close, fout);

          // copy contents
          void *buffer = malloc(SYNCUP_COPY_FILE_BUFFER_SIZE);
          if (!buffer) {
              result = CCD_ERROR_NOMEM;
              goto end;
          }
          ON_BLOCK_EXIT(free, buffer);
          ssize_t bytes_read;
          while ((bytes_read = VPLFile_Read(fin, buffer, SYNCUP_COPY_FILE_BUFFER_SIZE)) > 0) {
              ssize_t bytes_written = VPLFile_Write(fout, buffer, bytes_read);
              if (bytes_written < 0) {
                  result = bytes_written;
                  goto end;
              }
              if (bytes_written != bytes_read) {
                  LOG_ERROR("Failed to make copy of %s", localpath.c_str());
                  result = CCD_ERROR_DISK_SERIALIZE;
                  goto end;
              }
          }
          if (bytes_read < 0) {
              result = bytes_read;
              goto end;
          }
      }

      err = addJob(localpath,
                   make_copy ? tmppath : localpath,
                   make_copy,
                   ctime,
                   mtime,
                   datasetid,
                   comppath,
                   VPLTime_GetTime(),
                   syncfeature,
                   jobid);
      CHECK_ERR(err, result, end);
      err = commitTransaction();
      CHECK_ERR(err, result, end);
    }

 end:
    if (result) {
        rollbackTransaction();
    }
    return result;
}

int SyncUpJobs::RemoveJob(u64 jobid)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        SyncUpJob job;
        err = findJob(jobid, job);
        if (err == CCD_ERROR_NOT_FOUND) {
            goto end;
        }
        CHECK_ERR(err, result, end);

        deleteTmpFile(job, &workdir);

        err = removeJob(jobid);
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncUpJobs::RemoveJobsByDatasetId(u64 datasetId)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        err = visitJobsByDatasetId(datasetId, false, deleteTmpFile, &workdir);
        CHECK_ERR(err, result, end);

        err = removeJobsByDatasetId(datasetId);
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncUpJobs::RemoveJobsByDatasetIdLocalPathPrefix(u64 datasetId, const std::string &prefix)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        err = visitJobsByDatasetIdOpathPrefix(datasetId, prefix, false, deleteTmpFile, &workdir);
        CHECK_ERR(err, result, end);

        err = removeJobsByDatasetIdOpathPrefix(datasetId, prefix);
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

static int stopAfterOneJob(SyncUpJob &job, void *param)
{
    bool* jobExists = (bool*)param;
    *jobExists = true;
    return OK_STOP_DB_TRAVERSE;
}

int SyncUpJobs::GetJobCountByFeature(u64 feature,
                                     u64 cutoff,
                                     u32& jobCount_out,
                                     u32& failedJobCount_out)
{
    int result = 0;
    int dberr = 0;

    jobCount_out = 0;
    failedJobCount_out = 0;

    MutexAutoLock lock(&mutex);

    if (!db) {
        dberr = openDB();
        if(dberr != 0) {
            return dberr;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_COUNT_BY_FEATURE)];
    sqlite3_stmt *&stmt2 = dbstmts[SQLNUM(GET_FAILED_JOB_COUNT_BY_FEATURE)];

    // Get the number of pending jobs
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_COUNT_BY_FEATURE), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :feature
    dberr = sqlite3_bind_int64(stmt, 1, (s64)feature);
    CHECK_BIND(dberr, result, db, end);

    // bind :cutoff
    dberr = sqlite3_bind_int64(stmt, 2, (s64)cutoff);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (dberr == SQLITE_ROW) {
        jobCount_out = sqlite3_column_int(stmt, 0);
    } else if (dberr == SQLITE_DONE) {
        result = 0;
    } else {
        LOG_ERROR("Failed to execute prepared stmt to get jobCount: %d", result);
        result = dberr;
        goto end;
    }

    // Get the number of failed jobs
    if (!stmt2) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_FAILED_JOB_COUNT_BY_FEATURE), -1, &stmt2, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :feature
    dberr = sqlite3_bind_int64(stmt2, 1, (s64)feature);
    CHECK_BIND(dberr, result, db, end);

    // bind :cutoff
    dberr = sqlite3_bind_int64(stmt2, 2, (s64)cutoff);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt2);
    CHECK_STEP(dberr, result, db, end);

    if (dberr == SQLITE_ROW) {
        failedJobCount_out = sqlite3_column_int(stmt2, 0);
    } else if (dberr == SQLITE_DONE) {
        result = 0;
    } else {
        LOG_ERROR("Failed to execute prepared stmt to get failedJobCount: %d", result);
        result = dberr;
    }

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    if (stmt2) {
        sqlite3_reset(stmt2);
    }
    return result;
}

int SyncUpJobs::QueryStatus(u64 datasetId,
                            const std::string& path,
                            ccd::FeatureSyncStateSummary& syncState_out)
{
    // Ensure that path ends with a '/'
    bool jobExists = false;
    bool errorJobExists = false;
    int rc;
    syncState_out.set_status(ccd::CCD_FEATURE_STATE_OUT_OF_SYNC);

    if(path.size()==0) {
        rc = visitJobsByDatasetId(datasetId,
                                  false,
                                  stopAfterOneJob,
                                  &jobExists);
        if(rc != 0) {
            LOG_ERROR("visitJobsByDatasetId("FMTu64",%s):%d",
                      datasetId, path.c_str(), rc);
            return rc;
        }
        if(jobExists) {
            rc = visitJobsByDatasetId(datasetId,
                                      true,
                                      stopAfterOneJob,
                                      &errorJobExists);
            if(rc != 0) {
                LOG_ERROR("visitJobsByDatasetId("FMTu64",%s):%d",
                          datasetId, path.c_str(), rc);
                return rc;
            }
        }
    }else{
        std::string mutablePath = path;
        if(path[path.size()-1] != '/') {
            mutablePath.append("/");
        }
        rc = visitJobsByDatasetIdOpathPrefix(datasetId,
                                             mutablePath,
                                             false,
                                             stopAfterOneJob,
                                             &jobExists);
        if(rc != 0) {
            LOG_ERROR("visitJobsByDatasetIdOpathPrefix("FMTu64",%s):%d",
                      datasetId, path.c_str(), rc);
            return rc;
        }
        if(jobExists) {
            rc = visitJobsByDatasetIdOpathPrefix(datasetId,
                                                 mutablePath,
                                                 true,
                                                 stopAfterOneJob,
                                                 &errorJobExists);
            if(rc != 0) {
                LOG_ERROR("visitJobsByDatasetIdOpathPrefix("FMTu64",%s):%d",
                          datasetId, path.c_str(), rc);
                return rc;
            }
        }
    }
    if(jobExists) {
        if(errorJobExists) {
            syncState_out.set_status(ccd::CCD_FEATURE_STATE_OUT_OF_SYNC);
        }else{
            syncState_out.set_status(ccd::CCD_FEATURE_STATE_SYNCING);
        }
    }else{
        syncState_out.set_status(ccd::CCD_FEATURE_STATE_IN_SYNC);
    }
    return 0;
}

int SyncUpJobs::RemoveAllJobs()
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        err = visitJobs(deleteTmpFile, &workdir);
        CHECK_ERR(err, result, end);

        err = removeAllJobs();
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncUpJobs::GetNextJob(u64 cutoff, /*OUT*/ SyncUpJob &job)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = findJobNotDoneNoRecentDispatch(cutoff, job);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncUpJobs::TimestampJobDispatch(u64 jobid, u64 vplGetTimeSec)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDispatchTime(jobid, vplGetTimeSec);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncUpJobs::TimestampJobTryUpload(u64 jobid)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobTryUploadTime(jobid, VPLTime_GetTime());
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncUpJobs::TimestampJobDoneUpload(u64 jobid)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDoneUploadTime(jobid, VPLTime_GetTime());
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncUpJobs::ReportJobUploadFailed(u64 jobid)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = incJobUploadFailedCount(jobid);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncUpJobs::PurgeOnClose()
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = setPurgeOnClose(true);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

//----------------------------------------------------------------------

SyncUp::SyncUp(const std::string &workdir)
:   overrideAnsCheck(false),
    workdir(workdir),
    userId(0),
    cutoff_ts(0),
    jobs(NULL),
    thread_spawned(false),
    thread_running(false),
    exit_thread(false)
{
    int err = 0;

    err = VPLMutex_Init(&mutex);
    if (err) {
        LOG_ERROR("Failed to initialize mutex: %d", err);
    }
    err = VPLCond_Init(&cond);
    if (err) {
        LOG_ERROR("Failed to initialize condvar: %d", err);
    }

    currentSyncStatus.clear();
    currentSyncStatus.insert(std::pair<int, int>(ccd::SYNC_FEATURE_PICSTREAM_UPLOAD,
                                                 ccd::CCD_SYNC_STATE_IN_SYNC));
}

// Assumption: Called only by SyncUp dispatcher thread.
SyncUp::~SyncUp()
{
    int err = 0;

    if (jobs) {
        delete jobs;
        jobs = NULL;
        LOG_DEBUG("SyncUpJobs obj destroyed");
    }

    err = VPLMutex_Destroy(&mutex);
    if (err) {
        LOG_ERROR("Failed to destroy mutex: %d", err);
    }
    err = VPLCond_Destroy(&cond);
    if (err) {
        LOG_ERROR("Failed to destroy condvar: %d", err);
    }
}

int SyncUp::spawnDispatcherThread()
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    if (!thread_spawned) {
        int err = 0;

        VPLThread_attr_t attr;
        err = VPLThread_AttrInit(&attr);
        if (err != VPL_OK) {
            LOG_ERROR("Failed to initialize thread attr obj: %d", err);
            return err;
        }
        ON_BLOCK_EXIT(VPLThread_AttrDestroy, &attr);
        VPLThread_AttrSetDetachState(&attr, VPL_TRUE);
        VPLThread_AttrSetStackSize(&attr, SYNCUP_THREAD_STACK_SIZE);
        VPLDetachableThreadHandle_t thread;
        err = VPLDetachableThread_Create(&thread, syncUpDispatcherThreadMain, (void*)this, &attr, NULL);
        if (err != VPL_OK) {
            LOG_ERROR("Failed to spawn SyncUp dispatcher thread: %d", err);
            return err;
        }
        thread_spawned = true;
    }

    return result;
}

int SyncUp::signalDispatcherThreadToStop()
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    if (thread_running) {
        exit_thread = true;

        int err = VPLCond_Signal(&cond);
        if (err != VPL_OK) {
            LOG_ERROR("Unexpected error signaling condvar: %d", err);
            result = err;
        }
        LOG_DEBUG("signaled dispatcher thread to stop");
    }

    return result;
}

VPLTHREAD_FN_DECL SyncUp::syncUpDispatcherThreadMain(void *arg)
{
    SyncUp *su = (SyncUp*)arg;
    if (su) {
        su->syncUpDispatcherThreadMain();

        delete su;
        LOG_DEBUG("SyncUp obj destroyed");
    }
    return VPLTHREAD_RETURN_VALUE;
}

void SyncUp::syncUpDispatcherThreadMain()
{
    thread_running = true;

    LOG_DEBUG("dispatcher thread started");

    while (1) {
        if (exit_thread) break;

        u64 cutoff = VPLTime_GetTime();
        cutoff_ts = cutoff;
        int err = 0;
        int uploadErrs = 0;

        u32 failedJobCounts = 0;
        u32 pendingJobCounts = 0;

        while (1) {
            if (exit_thread) break;

            VPL_BOOL ansConnected = ANSConn_IsActive();
            SyncUpJob job;
            {
                MutexAutoLock lock(&mutex);
                if(overrideAnsCheck || ansConnected) {
                    err = jobs->GetNextJob(cutoff, job);
                }
                if ((!overrideAnsCheck && !ansConnected) ||
                    (err == CCD_ERROR_NOT_FOUND) ||
                    uploadErrs >= NUM_UPLOAD_RETRY)
                {
                    if(uploadErrs >= NUM_UPLOAD_RETRY) {
                        LOG_WARN("%d upload errs encountered, going to sleep. "
                                 "ansOverrideCheck:%d",
                                 uploadErrs, overrideAnsCheck);
                    } else if(err == CCD_ERROR_NOT_FOUND) {
                        LOG_DEBUG("No job found - going to sleep.");
                    } else {
                        LOG_INFO("ANS not connected -- going to sleep.");
                    }
                    overrideAnsCheck = false;
                    int datasetIdErr;
                    do {
                        err = VPLCond_TimedWait(&cond,
                                                &mutex,
                                                VPLTime_FromSec(__ccdConfig.syncUpRetryInterval));
                        if (err != 0 && err != VPL_ERR_TIMEOUT) {
                            LOG_ERROR("Unexpected error from VPLCond_TimedWait: %d", err);
                            return;
                        }
                        lock.UnlockNow();
                        u64 datasetId_unused;
                        datasetIdErr = VCS_getDatasetID(userId, "PicStream", datasetId_unused);
                        lock.Relock(&mutex);
                    } while (datasetIdErr==CCD_ERROR_DATASET_SUSPENDED && !exit_thread);
                    LOG_DEBUG("woke up - due to %s", err ? "timeout" : "signal");
                    break;
                    // This break goes to the comment label SYNCUP_OUTSIDE_WHILE
                }
                if (err) {
                    LOG_ERROR("Unexpected error from GetNextJob: %d", err);
                    return;
                }
                // Get next job successfully, set status to SYNCING and generate event if necessary
                err = jobs->GetJobCountByFeature(job.feature, cutoff, pendingJobCounts, failedJobCounts);
                if(err) {
                    LOG_ERROR("Error when getting job counts! error = %d", err);
                } else {
                    setSyncFeatureState((ccd::SyncFeature_t)job.feature,
                                        ccd::CCD_FEATURE_STATE_SYNCING,
                                        pendingJobCounts,
                                        failedJobCounts,
                                        true);
                }
                u64 currVplTimeSec = VPLTime_GetTime();
                if(currVplTimeSec < cutoff) {
                    // Time went backwards.  Let's not get into infinite loop.
                    LOG_ERROR("Time went backwards(%s): "FMTu64"->"FMTu64,
                              job.cpath.c_str(), cutoff, currVplTimeSec);
                    break;  // expected to go to comment label: SYNCUP_OUTSIDE_WHILE
                }
                jobs->TimestampJobDispatch(job.id, currVplTimeSec);
            }

            err = tryUpload(job);
            if(err != 0) {
                uploadErrs++;
            }
            err = jobs->GetJobCountByFeature(job.feature, cutoff, pendingJobCounts, failedJobCounts);
            if(err) {
                // Error when get job counts 
                LOG_ERROR("Error when getting job counts! error = %d", err);
            } else {
                if(pendingJobCounts > 0) {
                    setSyncFeatureState((ccd::SyncFeature_t)job.feature,
                                        ccd::CCD_FEATURE_STATE_SYNCING,
                                        pendingJobCounts,
                                        failedJobCounts,
                                        false);
                } else if (failedJobCounts > 0) {
                    setSyncFeatureState((ccd::SyncFeature_t)job.feature,
                                        ccd::CCD_FEATURE_STATE_OUT_OF_SYNC,
                                        pendingJobCounts,
                                        failedJobCounts,
                                        false);
                } else {
                    setSyncFeatureState((ccd::SyncFeature_t)job.feature,
                                        ccd::CCD_FEATURE_STATE_IN_SYNC,
                                        pendingJobCounts,
                                        failedJobCounts,
                                        false);
                }
            }
        }
        // label: SYNCUP_OUTSIDE_WHILE
    }

    LOG_DEBUG("dispatcher thread exiting");

    {
        // Need to acquire lock to prevent signaling context
        // from having its state ripped out by this thread.  (SyncUp::Stop)
        MutexAutoLock lock(&mutex);
        thread_running = false;
        thread_spawned = false;
    }
}

//   
// Note that no mutual exclusion is required for this routine:
//   The currentSyncStatus map is initialized in the constructor and
//   this routine only modifies the state entries, not the actual map.
//   At present, this is called only by one thread in any case.
//   
int SyncUp::setSyncFeatureState(ccd::SyncFeature_t feature,
                                ccd::FeatureSyncStateType_t state,
                                u32 &pendingJobCounts,
                                u32 &failedJobCounts,
                                bool checkStateChange)
{
    std::map<int, int>::iterator it;
    it = currentSyncStatus.find(feature);
    if(it!=currentSyncStatus.end())
    {
        if(checkStateChange) {
            // Generate event only when the previous state is different than current setting state
            if (it->second != state) {
                it->second = state;
                ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
                ccd::EventSyncFeatureStatusChange* mutable_esfsc;
                mutable_esfsc = ccdiEvent->mutable_sync_feature_status_change();
                mutable_esfsc->set_feature(feature);
                mutable_esfsc->mutable_status()->set_status(state);
                mutable_esfsc->mutable_status()->set_pending_files(pendingJobCounts);
                mutable_esfsc->mutable_status()->set_failed_files(failedJobCounts);
                EventManagerPb_AddEvent(ccdiEvent);
                return 0;
            }
            return 0;
        } else {
            // No matter the previous state was, set the state and generate event
            it->second = state;
            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
            ccd::EventSyncFeatureStatusChange* mutable_esfsc;
            mutable_esfsc = ccdiEvent->mutable_sync_feature_status_change();
            mutable_esfsc->set_feature(feature);
            mutable_esfsc->mutable_status()->set_status(state);
            mutable_esfsc->mutable_status()->set_pending_files(pendingJobCounts);
            mutable_esfsc->mutable_status()->set_failed_files(failedJobCounts);
            EventManagerPb_AddEvent(ccdiEvent);
            return 0;
        }
    } else {
        LOG_ERROR("Error when getting sync feature! feature = %d, state = %d", (int)feature, (int)state);
        return -1;
    }
}


int SyncUp::tryUpload(SyncUpJob &job)
{
    int result = 0;
    int err = 0;

    // if job.spath is tmppath, extend it
    std::string spath;
    if (job.is_tmp) {
        extendTmpPathToFullPath(job.spath, workdir, spath);
    }
    else {
        spath.assign(job.spath);
    }

    VPLHttp2 *http2 = new VPLHttp2();

    // Publish VPLHttp2 object for possible cancellation.
    {
        MutexAutoLock lock(&mutex);
        httpInProgress[job.id] = http2;
    }

    // Blocking call, can take a while.
    std::string _vcsresponse;
    err = S3_putFileSyncUp("vcs",
                           VPLHttp_UrlEncoding(job.cpath,"/"),
                           job.ctime,
                           userId,
                           job.did,
                           false, 0, 0,  // baserev, compid
                           "",  // no contentType
                           spath,
                           http2,
                           _vcsresponse);

    // Unpublish VPLHttp2 object from possible cancellation and destroy it.
    {
        MutexAutoLock lock(&mutex);
        httpInProgress.erase(job.id);
        delete http2;
        http2 = NULL;
    }
    {
        if (err == 0) {  // Success, remove job
            LOG_INFO("SUjobid uploaded("FMTu64",%s)", job.id, job.cpath.c_str());
            MutexAutoLock lock(&mutex);
            err = jobs->RemoveJob(job.id);
            if(err!=0) {
                LOG_ERROR("DB err RemoveJob(%d)", err);
            }

            add2PicStreamDB(job.opath , _vcsresponse);

        } else if(err > 0) { // Permanent failure, Drop job
            LOG_ERROR("SUjobid drop("FMTu64",%s)", job.id, job.cpath.c_str());
            MutexAutoLock lock(&mutex);
            err = jobs->RemoveJob(job.id);
            if(err!=0) {
                LOG_ERROR("DB err RemoveJob(%d)", err);
            }
        } else if(err < 0) { // Retry Job
            LOG_WARN("SUjobid retry("FMTu64",%s)", job.id, job.cpath.c_str());
            result = err;  // This is an error that we want to count towards the
                           // number of errors before pausing.
            MutexAutoLock lock(&mutex);
            err = jobs->ReportJobUploadFailed(job.id);
            if(err!=0) {
                LOG_ERROR("DB err ReportJobUploadFailed(%d)", err);
            }
        }
    }

    return result;
}

int SyncUp::Start(u64 userId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    if (jobs || thread_spawned) {
        LOG_ERROR("SyncUp obj already started");
        return CCD_ERROR_ALREADY_INIT;
        // do not jump to "end" which will stop SyncUp obj
    }

    this->userId = userId;

    jobs = new SyncUpJobs(workdir);
    if (!jobs) {
        LOG_ERROR("Failed to allocate SyncUpJobs object");
        result = err;
        goto end;
    }

    err = jobs->Check();
    if (err) {
        LOG_ERROR("SyncUpJobs obj failed Check: %d", err);
        result = err;
        goto end;
    }

    err = spawnDispatcherThread();
    if (err) {
        LOG_ERROR("Failed to spawn dispatcher thread: %d", err);
        result = err;
        goto end;
    }

 end:
    if (result) {
        Stop(false);
    }
    return result;
}

int SyncUp::Stop(bool purge)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    if (jobs && purge) {
        err = jobs->RemoveAllJobs();
        if (err) {
            LOG_ERROR("Failed to remove all jobs: %d", err);
            if (!result) result = err;
            // Do not return; try to clean up as much as possible.
        }
        else {
            LOG_DEBUG("purged all remaining jobs");
        }

        err = jobs->PurgeOnClose();
        if (err) {
            LOG_ERROR("Failed to set purge-on-close: %d", err);
            // Do not return; try to clean up as much as possible.
        }
        else {
            LOG_DEBUG("SyncUpJobs obj marked to purge-on-close");
        }
    }

    {
        // Must hold lock until after canceling VPLHttp2 objects.
        // Otherwise, dispatcher thread may destroy them.
        ASSERT(VPLMutex_LockedSelf(&mutex));

        if (thread_running) {
            err = signalDispatcherThreadToStop();
            if (err) {
                LOG_ERROR("Failed to signal dispatcher thread to stop: %d", err);
                if (!result) result = err;
                // Do not return; try to clean up as much as possible.
            }

            // dispatch thread will destroy SyncUpJobs obj and SyncUp obj and then exit.
        }

        std::map<u64, VPLHttp2*>::iterator it;
        for (it = httpInProgress.begin(); it != httpInProgress.end(); it++) {
            it->second->Cancel();
        }
    }

    return result;
}

int SyncUp::NotifyConnectionChange(bool myOverrideAnsCheck)
{
    int result = 0;
    int err = 0;

    LOG_DEBUG("Notification received");

    MutexAutoLock lock(&mutex);

    overrideAnsCheck = myOverrideAnsCheck;
    err = VPLCond_Signal(&cond);
    if (err) {
        LOG_ERROR("Failed to signal condvar: %d", err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncUp::AddJob(const std::string &localpath, bool make_copy, u64 ctime, u64 mtime,
                   u64 datasetid, const std::string &comppath, u64 syncfeature)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    u64 _jobid;  // dummy
    err = jobs->AddJob(localpath, make_copy, ctime, mtime, datasetid, comppath, syncfeature, _jobid);
    if (err) {
        LOG_ERROR("Failed to add job (%s): %d", localpath.c_str(), err);
        result = err;
        goto end;
    }

    err = VPLCond_Signal(&cond);
    if (err) {
        LOG_ERROR("Unexpected error signaling condvar: %d", err);
        // don't override value of "result"
    }

 end:
    return result;
}

int SyncUp::RemoveJobsByDatasetId(u64 datasetId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = jobs->RemoveJobsByDatasetId(datasetId);
    if (err) {
        LOG_ERROR("Failed to remove jobs for dataset "FMTu64": %d", datasetId, err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncUp::RemoveJobsByDatasetIdLocalPathPrefix(u64 datasetId, const std::string &prefix)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = jobs->RemoveJobsByDatasetIdLocalPathPrefix(datasetId, prefix);
    if (err) {
        LOG_ERROR("Failed to remove jobs for dataset "FMTu64" with path prefix %s: %d",
                  datasetId, prefix.c_str(), err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncUp::QueryStatus(u64 myUserId,
                        u64 datasetId,
                        const std::string& path,
                        ccd::FeatureSyncStateSummary& syncState_out)
{
    int err = 0;

    MutexAutoLock lock(&mutex);
    if(myUserId != userId) {
        LOG_ERROR("query (uid:"FMTu64",did:"FMTu64",path:%s) does not match userId("FMTu64").",
                  myUserId, datasetId, path.c_str(), userId);
        return CCD_ERROR_NOT_SIGNED_IN;
    }

    err = jobs->QueryStatus(datasetId, path, syncState_out);
    if(err != 0) {
        LOG_ERROR("QueryStatus("FMTu64",%s):%d", datasetId, path.c_str(), err);
        return err;
    }
    return 0;
}

int SyncUp::QueryStatus(u64 myUserId,
                        ccd::SyncFeature_t syncFeature,
                        ccd::FeatureSyncStateSummary& syncState_out)
{
    int err = 0;
    std::map<int, int>::iterator it;

    MutexAutoLock lock(&mutex);
    if(myUserId != userId) {
        LOG_ERROR("query (uid:"FMTu64",feat:%d) does not match userId("FMTu64").",
            myUserId, (int)syncFeature, userId);
        return CCD_ERROR_NOT_SIGNED_IN;
    }

    it = currentSyncStatus.find(syncFeature);
    if(it != currentSyncStatus.end()) {
        syncState_out.set_status((ccd::FeatureSyncStateType_t)(it->second));
    }
    else {
        LOG_ERROR("Cannot find feature: %d", (int)syncFeature);
        return CCD_ERROR_NOT_FOUND;
    }

    u32 pendingJobCounts = 0;
    u32 failedJobCounts = 0;
    err = jobs->GetJobCountByFeature(syncFeature, cutoff_ts, pendingJobCounts, failedJobCounts);
    if(err) {
        // Error when get job counts 
        LOG_ERROR("Error when getting job counts, feature = %d, err = %d", (int)syncFeature, err);
        syncState_out.set_pending_files(0);
        syncState_out.set_failed_files(0);
        return err;
    }
    else {
        syncState_out.set_pending_files(pendingJobCounts);
        syncState_out.set_failed_files(failedJobCounts);
    }
    return 0;
}

int SyncUp::add2PicStreamDB(const std::string &lpath, const std::string &vcsRespJson)
{

    cJSON2 *json = cJSON2_Parse(vcsRespJson.c_str());
    if (!json) {
        LOG_ERROR("Failed to parse response: %s", vcsRespJson.c_str());
        return -1;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json);

    /*
         vcsresponse = {"name":"photos/2014_01/20140127_051545.jpg",
                        "compId":838414148,
                        "originDevice":30015151148,
                        "revisionList":[ {
                           "revision":1,
                           "size":95995,
                           "lastChanged":1390799745,
                           "updateDevice":30015151148,
                           "previewUri":"/vcs/preview/3610144/photos/2014_01/20140127_051545.jpg?compId=838414148&revision=1",
                           "downloadUrl":"https://acercloud-lab13-1348867707.s3.amazonaws.com/3610144/4819751847544692034"
                         }],
                         "numOfRevisions":1}
    */
    std::string name, albumName, title, identifier;
    u64 compId, originDev, rev = 0, size = 0, takenTime = 0;

    identifier = lpath;
    {//parse Json

        cJSON2 *json_name = cJSON2_GetObjectItem(json, "name");
        if (!json_name) {
            LOG_ERROR("name missing");
            return -1;
        }
        if (json_name->type != cJSON2_String) {
            LOG_ERROR("name in unexpected format");
            return -1;
        }
        name = json_name->valuestring;
        LOG_DEBUG("name = %s", name.c_str());

        cJSON2 *json_compId = cJSON2_GetObjectItem(json, "compId");
        if (!json_compId) {
            LOG_ERROR("compId missing");
            return -1;
        }
        if (json_compId->type != cJSON2_Number) {
            LOG_ERROR("compId in unexpected format");
            return -1;
        }
        compId = json_compId->valueint;
        LOG_DEBUG("compId = "FMTu64, compId);

        cJSON2 *json_origDevId = cJSON2_GetObjectItem(json, "originDevice");
        if (!json_origDevId) {
            LOG_ERROR("originDevice missing");
            return -1;
        }
        if (json_origDevId->type != cJSON2_Number) {
            LOG_ERROR("originDevice in unexpected format");
            return -1;
        }
        originDev = json_origDevId->valueint;
        LOG_DEBUG("originDevice = "FMTu64, originDev);

        cJSON2 *json_revList = cJSON2_GetObjectItem(json, "revisionList");
        if (!json_revList) {
            LOG_ERROR("changeList missing");
            return -1;
        }
        if (json_revList->type != cJSON2_Array) {
            LOG_ERROR("changeList has unexpected value");
            return -1;
        }

        int revListSize = cJSON2_GetArraySize(json_revList);
        for (int i = 0; i < revListSize; i++) {
            cJSON2 *json_revItem = cJSON2_GetArrayItem(json_revList, i);
            if (!json_revItem) continue;

            cJSON2 *json_rev = cJSON2_GetObjectItem(json_revItem, "revision");
            if (!json_rev) {
                LOG_ERROR("revision missing");
                return -1;
            }
            if (json_rev->type != cJSON2_Number) {
                LOG_ERROR("revision in unexpected format");
                return -1;
            }
            rev = json_rev->valueint;
            LOG_DEBUG("revision = "FMTu64, rev);

            cJSON2 *json_size = cJSON2_GetObjectItem(json_revItem, "size");
            if (!json_size) {
                LOG_ERROR("size missing");
                return -1;
            }
            if (json_size->type != cJSON2_Number) {
                LOG_ERROR("size in unexpected format");
                return -1;
            }
            size = json_size->valueint;
            LOG_DEBUG("size = "FMTu64, size);
        }
    } //parse JSON

    {//parse name to albumName, title, takenTime
        int pos_first_slash = name.find_first_of('/');
        int pos_second_slash = std::string::npos;
        int pos_dot = std::string::npos;
        if (std::string::npos != pos_first_slash) {
            pos_second_slash = name.find_first_of('/', pos_first_slash+1);
        }

        if (std::string::npos != pos_second_slash) {
            albumName = name.substr(pos_first_slash+1, pos_second_slash - pos_first_slash - 1);
            title = name.substr(pos_second_slash+1, string::npos);

            std::string takenTimeString = "";
            char timeStamp[20];
            char *pEnd;
            pos_dot = name.find_first_of('.', pos_second_slash+1);
            if (std::string::npos != pos_dot) {
                takenTimeString = name.substr(pos_second_slash+1, pos_dot - pos_second_slash - 1);
            }
            takenTimeString.replace(8, 1, "");
            sprintf(timeStamp, "%s", takenTimeString.c_str());
            takenTime = (u64) strtoull(timeStamp, &pEnd, 10);
        }
        LOG_DEBUG("albumName =%s, title=%s, takenTime="FMTu64, albumName.c_str(), title.c_str(), takenTime);
    }//parse name

    extern int SyncDown_AddPicStreamItem(  u64 compId, const std::string &name, u64 rev, u64 origCompId, u64 originDev,
                            const std::string &identifier, const std::string &title, const std::string &albumName,
                            u64 fileType, u64 fileSize, u64 takenTime, const std::string &lpath, /*OUT*/ u64 &jobid, bool &syncUpAdded);

    u64 _jobid;
    bool _syncUpAdded;
    return SyncDown_AddPicStreamItem(compId, name, rev, 0/*origCompId*/, originDev, identifier, title, albumName,
                                0/*fileType*/, size, takenTime, lpath, /*OUT*/ _jobid, _syncUpAdded);

}

//----------------------------------------------------------------------

class OneSyncUp {
public:
    OneSyncUp() {
        VPLMutex_Init(&mutex);
    }
    ~OneSyncUp() {
        VPLMutex_Destroy(&mutex);
    }
    SyncUp *obj;
    VPLMutex_t mutex;
};
static OneSyncUp oneSyncUp;

int SyncUp_Start(u64 userId)
{
    int result = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForSyncUp(sizeof(path), path);
    // assert: path ends in '/'

    oneSyncUp.obj = new SyncUp(path);

    int err = oneSyncUp.obj->Start(userId);
    if (err) {
        LOG_ERROR("Failed to start SyncUp: %d", err);
        result = err;
        oneSyncUp.obj = NULL;
        goto end;
    }

 end:
    return result;
}

int SyncUp_Stop(bool purge)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    if (!oneSyncUp.obj) {
        LOG_WARN("SyncUp never started");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncUp.obj->Stop(purge);
    if (err) {
        // err msg printed by SyncUp::Stop()
        result = err;
        goto end;
    }

 end:
    oneSyncUp.obj = NULL;  // Remove reference even if Stop() fails because there's nothing more we can do about it.
    return result;
}

int SyncUp_NotifyConnectionChange(bool overrideAnsCheck)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    if (!oneSyncUp.obj) {
        LOG_WARN("SyncUp not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncUp.obj->NotifyConnectionChange(overrideAnsCheck);
    if (err) {
        LOG_ERROR("Failed to notify SyncUp of connection change: %d", err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncUp_AddJob(const std::string &localpath, bool make_copy, u64 ctime, u64 mtime,
                  u64 datasetid, const std::string &comppath, u64 syncfeature)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    if (!oneSyncUp.obj) {
        LOG_WARN("SyncUp not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    LOG_DEBUG("lpath %s did "FMTu64, localpath.c_str(), datasetid);

    err = oneSyncUp.obj->AddJob(localpath, make_copy, ctime, mtime,
                                datasetid, comppath, syncfeature);
    if (err) {
        LOG_ERROR("Failed to add job %s: %d", localpath.c_str(), err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncUp_RemoveJobsByDataset(u64 datasetId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    if (!oneSyncUp.obj) {
        LOG_WARN("SyncUp not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    LOG_DEBUG("did "FMTu64, datasetId);

    err = oneSyncUp.obj->RemoveJobsByDatasetId(datasetId);
    if (err) {
        LOG_ERROR("Failed to remove jobs for dataset "FMTu64": %d", datasetId, err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncUp_RemoveJobsByDatasetSourcePathPrefix(u64 datasetId, const std::string &_prefix)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    if (!oneSyncUp.obj) {
        LOG_WARN("SyncUp not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    LOG_DEBUG("did "FMTu64" prefix %s", datasetId, _prefix.c_str());

    {
        std::string prefix = _prefix;
        // Make sure the prefix ends with a slash.
        // This is to avoid cases like "/abc/de" matching "/abc/def".
        if (prefix[prefix.length() - 1] != '/') {
            prefix.append("/");
        }

        err = oneSyncUp.obj->RemoveJobsByDatasetIdLocalPathPrefix(datasetId, prefix);
        if (err) {
            LOG_ERROR("Failed to remove jobs for dataset "FMTu64" with path prefix %s: %d",
                      datasetId, prefix.c_str(), err);
            result = err;
            goto end;
        }
    }

 end:
    return result;
}

int SyncUp_QueryStatus(u64 userId,
                       u64 datasetId,
                       const std::string& path,
                       ccd::FeatureSyncStateSummary& syncState_out)
{
    int rv = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    if (!oneSyncUp.obj) {
        LOG_WARN("SyncUp not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncUp.obj->QueryStatus(userId,
                                     datasetId,
                                     path,
                                     syncState_out);
    if (err) {
         LOG_ERROR("Failed to query status("FMTu64","FMTu64",%s):%d",
                   userId, datasetId, path.c_str(), err);
         rv = err;
         goto end;
     }

  end:
     return rv;
}

int SyncUp_QueryStatus(u64 userId,
                       ccd::SyncFeature_t syncFeature,
                       ccd::FeatureSyncStateSummary& syncState_out)
{
    int rv = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncUp.mutex);

    if (!oneSyncUp.obj) {
        LOG_WARN("SyncUp not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncUp.obj->QueryStatus(userId,
                                     syncFeature,
                                     syncState_out);
    if (err) {
        LOG_ERROR("Failed to query status("FMTu64",%d):%d",
            userId, (int)syncFeature, err);
        rv = err;
        goto end;
    }

end:
    return rv;
}
