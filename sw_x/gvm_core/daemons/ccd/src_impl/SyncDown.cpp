//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#include "SyncDown.hpp"
#include "ccdi_rpc.pb.h"
#include "SyncDown_private.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <vpl_fs.h>
#include <vplex_file.h>
#include <vplex_http_util.hpp>
#include <vplu_mutex_autolock.hpp>
#include <scopeguard.hpp>

#include <http_request.hpp>
#include <cJSON2.h>
#include <log.h>

#include "ans_connection.hpp"
#include "cache.h"
#include "ccd_features.h"
#include "ccd_storage.hpp"
#include "config.h"
#ifdef CLOUDNODE
#include "dataset.hpp"
#endif
#include "db_util_access_macros.hpp"
#include "db_util_check_macros.hpp"
#include "EventManagerPb.hpp"
#include "gvm_file_utils.hpp"
#include "gvm_rm_utils.hpp"
#include "stream_transaction.hpp"
#include "util_open_db_handle.hpp"
#include "vcs_proxy.hpp"
#include "vcs_utils.hpp"
#include "vcs_v1_util.hpp"
#include "virtual_device.hpp"
#include "query.h"
#include "vplu_sstr.hpp"

#define SYNC_DOWN_SQLITE3_TRUE 1
#define SYNC_DOWN_SQLITE3_FALSE 0

#define SYNCDOWN_THREAD_STACK_SIZE (128 * 1024)
#define SYNCDOWN_COPY_FILE_BUFFER_SIZE (4 * 1024)
#define DOD_CACHE_FOLDER "DoDCache/"

// Target DB schema version.
#define TARGET_SCHEMA_VERSION 3

#define OK_STOP_DB_TRAVERSE 1

// for stringification
#define XSTR(s) STR(s)
#define STR(s) #s

void getCacheFilePath(u64 datasetId, u64 compId, u64 fileType, std::string compPath, /*OUT*/ std::string &cacheFile);

// Starting with v2.5, dl_path will be a path relative to the workdir.
// This function is used to extend it to a full path at runtime.
// E.g., "tmp/sd_123" -> "/aaa/bbb/ccdir/sd/tmp/sd_123"
// For backward compatibility (i.e., dl_path is full path), this function will replace the prefix part.
// E.g., "/zzz/yyy/tmp/sd_123" -> "/aaa/bbb/ccdir/sd/tmp/sd_123"
static void extendDlPathToFullPath(const std::string &dlpath, const std::string &workdir, std::string &fullpath)
{
    size_t pos = dlpath.rfind("tmp/");
    fullpath.assign(workdir);
    fullpath.append(dlpath, pos, std::string::npos);
}

#if defined(CLOUDNODE)
#define RESERVED_PERCENT_OF_SPACE 0.01
static int isSpaceAvailable(const std::string workdir)
{
    // We always reserved 1% disk space on Orbe
    int err = VPL_OK;
    u64 disk_size = 0;
    u64 avail_size = 0;
    err = VPLFS_GetSpace(workdir.c_str(), &disk_size, &avail_size);
    if(err != VPL_OK) {
        LOG_ERROR("Failed to get disk space for partition of %s, err %d",
                  workdir.c_str(), err);
    } else {
        if(avail_size < disk_size * RESERVED_PERCENT_OF_SPACE) {
            LOG_WARN("There is no available disk space (total: "FMTu64""
                     ", avail: "FMTu64") for partition of %s",
                      disk_size, avail_size, workdir.c_str());
            err = VPL_ERR_NOSPC;
        }
    }
    return err;
}
#endif

int SyncDownJob::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    id          = (u64)sqlite3_column_int64(stmt, n++);
    did         = (u64)sqlite3_column_int64(stmt, n++);
    cpath       = (const char*)sqlite3_column_text(stmt, n++);
    cid         = (u64)sqlite3_column_int64(stmt, n++);
    lpath       = (const char*)sqlite3_column_text(stmt, n++);
    const char *dlpath = (const char*)sqlite3_column_text(stmt, n++);
    if (dlpath) {
        dl_path.assign(dlpath);
    }
    else {
        dl_path.clear();
    }
    dl_rev      = (u64)sqlite3_column_int64(stmt, n++);
    feature     = (u64)sqlite3_column_int64(stmt, n++);
    return 0;
}

int DeleteJob::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    id          = (u64)sqlite3_column_int64(stmt, n++);
    cid         = (u64)sqlite3_column_int64(stmt, n++);
    name       = (const char*)sqlite3_column_text(stmt, n++);
    add_ts      = (u64)sqlite3_column_int64(stmt, n++);
    disp_ts     = (u64)sqlite3_column_int64(stmt, n++);
    del_try_ts  = (u64)sqlite3_column_int64(stmt, n++);
    del_failed  = (u64)sqlite3_column_int64(stmt, n++);
    del_done_ts = (u64)sqlite3_column_int64(stmt, n++);
    return 0;
}

int PicStreamItem::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    compId = (u64)sqlite3_column_int64(stmt, n++);
    name = (const char*)sqlite3_column_text(stmt, n++);
    rev  = (u64)sqlite3_column_int64(stmt, n++);
    origCompId = (u64)sqlite3_column_int64(stmt, n++);
    origDev = (u64)sqlite3_column_int64(stmt, n++);
    identifier = (const char*)sqlite3_column_text(stmt, n++);
    title = (const char*)sqlite3_column_text(stmt, n++);
    albumName = (const char*)sqlite3_column_text(stmt, n++);
    fileType = (u64)sqlite3_column_int64(stmt, n++);
    fileSize = (u64)sqlite3_column_int64(stmt, n++);
    takenTime = (u64)sqlite3_column_int64(stmt, n++);
    lpath = (const char*)sqlite3_column_text(stmt, n++);
    return 0;
}

//----------------------------------------------------------------------

int SyncDownJobEx::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    id          = (u64)sqlite3_column_int64(stmt, n++);
    did         = (u64)sqlite3_column_int64(stmt, n++);
    cpath       = (const char*)sqlite3_column_text(stmt, n++);
    cid         = (u64)sqlite3_column_int64(stmt, n++);
    lpath       = (const char*)sqlite3_column_text(stmt, n++);
    add_ts      = (u64)sqlite3_column_int64(stmt, n++);
    disp_ts     = (u64)sqlite3_column_int64(stmt, n++);
    const char *dlpath = (const char*)sqlite3_column_text(stmt, n++);
    if (dlpath) {
        dl_path.assign(dlpath);
    }
    else {
        dl_path.clear();
    }
    dl_try_ts   = (u64)sqlite3_column_int64(stmt, n++);
    dl_failed   = (u64)sqlite3_column_int64(stmt, n++);
    dl_done_ts  = (u64)sqlite3_column_int64(stmt, n++);
    dl_rev      = (u64)sqlite3_column_int64(stmt, n++);
    cb_try_ts   = (u64)sqlite3_column_int64(stmt, n++);
    cb_failed   = (u64)sqlite3_column_int64(stmt, n++);
    cb_done_ts  = (u64)sqlite3_column_int64(stmt, n++);
    feature     = (u64)sqlite3_column_int64(stmt, n++);

    return 0;
}

//----------------------------------------------------------------------

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
    goto lbl;                                   \
}

#define CHECK_RESULT(result, lbl)               \
if ((result) != 0) {                            \
    goto lbl;                                   \
}

static bool isSqliteError(int ec)
{
    return ec != SQLITE_OK && ec != SQLITE_ROW && ec != SQLITE_DONE;
}

#define SQLITE_ERROR_BASE 0
static int mapSqliteErrCode(int ec)
{
    return SQLITE_ERROR_BASE - ec;
}

#define ADMINID_SCHEMA_VERSION 1

// Below is the v1 (Before CCD SDK 2.6.0) database schema for PicStream and Cloud Docs
// However from CCD SDK 2.5.4 RC1, Cloud Docs moved to DocTrackerDB, which is implemented in CloudDocMgr.cpp
// Only difference between test_and_create_db_sql_v1 and test_and_create_db_sql_v2
// is the addition of the feature field in the sd_job table.
//static const char test_and_create_db_sql_v1[] =
//"BEGIN;"
//"CREATE TABLE IF NOT EXISTS sd_admin ("
//    "id INTEGER PRIMARY KEY, "
//    "value INTEGER NOT NULL);"
//"INSERT OR IGNORE INTO sd_admin VALUES ("XSTR(ADMINID_SCHEMA_VERSION)", "XSTR(1)");"
//"CREATE TABLE IF NOT EXISTS sd_version ("
//    "did INTEGER PRIMARY KEY NOT NULL UNIQUE, "         // dataset ID
//    "version INTEGER NOT NULL);"        // last-checked dataset version
//"CREATE TABLE IF NOT EXISTS sd_job ("
//    "id INTEGER PRIMARY KEY AUTOINCREMENT, "            // job ID
//
//    "did INTEGER NOT NULL, "            // dataset ID
//    "cpath STRING NOT NULL, "           // component path; path in dataset
//    "cid INTEGER NOT NULL, "            // component ID
//
//    "lpath STRING NOT NULL, "           // local path on device to copy-back to
//
//    "add_ts INTEGER NOT NULL, "         // time when job was added
//
//    "disp_ts INTEGER, "                 // time when job was dispatched
//
//    "dl_path STRING, "                  // download path, where content is first downloaded; relative to workdir
//    "dl_try_ts INTEGER, "               // time when download was last tried; null if never
//    "dl_failed INTEGER DEFAULT 0, "     // number of times download failed
//    "dl_done_ts INTEGER, "              // timestamp when download succeeded; null if not yet
//    "dl_rev INTEGER, "                  // revision of content downloaded
//
//    "cb_try_ts INTEGER, "               // time when copyback was last tried; null if never
//    "cb_failed INTEGER DEFAULT 0, "     // number of times copyback failed
//    "cb_done_ts INTEGER, "              // time when copyback succeeded; null if not yet
//
//    "UNIQUE (did, cpath));"             // sanity check
//"COMMIT;";
//
//static const char test_and_create_db_sql_v2[] =
//"BEGIN;"
//"CREATE TABLE IF NOT EXISTS sd_admin ("
//    "id INTEGER PRIMARY KEY, "
//    "value INTEGER NOT NULL);"
//"INSERT OR IGNORE INTO sd_admin VALUES ("XSTR(ADMINID_SCHEMA_VERSION)", "XSTR(TARGET_SCHEMA_VERSION)");"
//"CREATE TABLE IF NOT EXISTS sd_version ("
//    "did INTEGER PRIMARY KEY NOT NULL UNIQUE, "         // dataset ID
//    "version INTEGER NOT NULL);"        // last-checked dataset version
//"CREATE TABLE IF NOT EXISTS sd_job ("
//    "id INTEGER PRIMARY KEY AUTOINCREMENT, "            // job ID
//
//    "did INTEGER NOT NULL, "            // dataset ID
//    "cpath STRING NOT NULL, "           // component path; path in dataset
//    "cid INTEGER NOT NULL, "            // component ID
//
//    "lpath STRING NOT NULL, "           // local path on device to copy-back to
//
//    "add_ts INTEGER NOT NULL, "         // VPLtime when job was added
//
//    "disp_ts INTEGER, "                 // VPLtime when job was dispatched
//
//    "dl_path STRING, "                  // download path, where content is first downloaded; relative to workdir
//    "dl_try_ts INTEGER, "               // VPLtime when download was last tried; null if never
//    "dl_failed INTEGER DEFAULT 0, "     // number of times download failed
//    "dl_done_ts INTEGER, "              // VPLtime when download succeeded; null if not yet
//    "dl_rev INTEGER, "                  // revision of content downloaded
//
//    "cb_try_ts INTEGER, "               // VPLtime when copyback was last tried; null if never
//    "cb_failed INTEGER DEFAULT 0, "     // number of times copyback failed
//    "cb_done_ts INTEGER, "              // VPLtime when copyback succeeded; null if not yet
//    "feature INTEGER, "                 // identify the sync feature that the job belonged
//
//    "UNIQUE (did, cpath));"             // sanity check
//"COMMIT;";

static const char test_and_create_db_sql_v3[] =
"BEGIN IMMEDIATE;"
"CREATE TABLE IF NOT EXISTS sd_admin ("
    "id INTEGER PRIMARY KEY, "
    "value INTEGER NOT NULL);"
"INSERT OR IGNORE INTO sd_admin VALUES ("XSTR(ADMINID_SCHEMA_VERSION)", "XSTR(TARGET_SCHEMA_VERSION)");"
"CREATE TABLE IF NOT EXISTS sd_version ("
    "did INTEGER PRIMARY KEY NOT NULL UNIQUE, "         // dataset ID
    "version INTEGER NOT NULL);"                        // last-checked dataset version
"CREATE TABLE IF NOT EXISTS sd_job ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "            // job ID

    "did INTEGER NOT NULL, "            // dataset ID
    "cpath TEXT NOT NULL, "           // component path; path in dataset
    "cid INTEGER NOT NULL, "            // component ID

    "lpath TEXT NOT NULL, "           // local path on device to copy-back to

    "add_ts INTEGER NOT NULL, "         // VPLtime when job was added

    "disp_ts INTEGER, "                 // VPLtime when job was dispatched

    "dl_path TEXT, "                  // download path, where content is first downloaded; relative to workdir
    "dl_try_ts INTEGER, "               // VPLtime when download was last tried; null if never
    "dl_failed INTEGER DEFAULT 0, "     // number of times download failed
    "dl_done_ts INTEGER, "              // VPLtime when download succeeded; null if not yet
    "dl_rev INTEGER, "                  // revision of content downloaded

    "cb_try_ts INTEGER, "               // VPLtime when copyback was last tried; null if never
    "cb_failed INTEGER DEFAULT 0, "     // number of times copyback failed
    "cb_done_ts INTEGER, "              // VPLtime when copyback succeeded; null if not yet
    "feature INTEGER, "                 // identify the sync feature that the job belonged

    "UNIQUE (did, cpath));"             // sanity check
//added for PicStream v3
// TODO: delete_job table should have feature or dataset in order to support
//       other features besides PicStream
"CREATE TABLE IF NOT EXISTS delete_job ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "            // job ID
    "cid INTEGER NOT NULL, "            // component ID
    "name TEXT, "                        // <file_type>/<album_name>/<title>
    "add_ts INTEGER NOT NULL, "         // VPLtime when job was added
    "disp_ts INTEGER, "                 // VPLtime when job was dispatched
    "del_try_ts INTEGER, "              // VPLtime when delete was last tried; null if never
    "del_failed INTEGER DEFAULT 0, "    // number of times delete failed
    "del_done_ts INTEGER, "             // VPLtime when delete succeeded; null if not yet
    "UNIQUE (cid, name));"             // sanity check
"CREATE TABLE IF NOT EXISTS picstream ("
    "name TEXT PRIMARY KEY,"            // <file_type>/<album_name>/<title>
    "comp_id INTEGER,"                  // Compid
    "rev INTEGER,"                      // Latest rev
    "orig_comp_id INTEGER,"             // Origin compid. Only for low-res and thumbnail items.
    "origin_dev INTEGER,"               // Originator device Id. Only for full-res items.
    "identifier TEXT,"                  // identifier for application. could be the abs_path in originator. Only for full-res items.
    "title TEXT,"                       // title generated by Infra, should be YYYYMMDDHHMMSS.jpg
    "album_name TEXT,"                  // folder name generated by Infra, should be YYYY-MM
    "file_type INTEGER,"                // 0 = full-res, 1 = low-res, 2 = thumbnail
    "file_size INTEGER,"
    "date_time INTEGER,"                // Taken time
    "lpath TEXT);"                      // The local_path of the downloaded item. NULL for non-download files.
"CREATE INDEX IF NOT EXISTS picstream_index ON picstream(title);"
"CREATE TABLE IF NOT EXISTS shared_files ("
    "comp_id INTEGER PRIMARY KEY,"
    "name TEXT NOT NULL,"
    "dataset_id INTEGER NOT NULL,"
    "feature INTEGER NOT NULL,"
    "b_thumb_downloaded INTEGER NOT NULL,"
    "recipient_list TEXT NOT NULL,"
    "revision INTEGER NOT NULL,"
    "size INTEGER NOT NULL,"
    "last_changed INTEGER,"
    "create_date INTEGER,"
    "update_device INTEGER,"
    "opaque_metadata TEXT,"
    "rel_thumb_path TEXT);"  // Path to thumbnail relative to getS[wb]mDownloadDir()
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
    STMT_REMOVE_ALL_JOBS,
    STMT_GET_JOB,
    STMT_GET_JOB_BY_ID,
    STMT_GET_JOB_BY_LOCALPATH,
    STMT_GET_JOB_BY_DATASETID,
    STMT_GET_JOB_BY_COMPID_AND_DATASETID,
    STMT_GET_JOB_BY_DATASETID_FAILED,
    STMT_GET_JOB_BY_DATASETID_LOCALPATH_PREFIX,
    STMT_GET_JOB_BY_DATASETID_LOCALPATH_PREFIX_FAILED,
    STMT_GET_JOB_DOWNLOAD_NOT_DONE_DISPATCH_BEFORE,
    STMT_GET_JOB_COPYBACK_NOT_DONE_DISPATCH_BEFORE,
    STMT_GET_JOBEX_BY_ID,
    STMT_SET_JOB_DOWNLOAD_PATH,
    STMT_SET_JOB_DOWNLOADED_REVISION,
    STMT_SET_JOB_DISPATCH_TIME,
    STMT_SET_JOB_TRY_DOWNLOAD_TIME,
    STMT_SET_JOB_DONE_DOWNLOAD_TIME,
    STMT_SET_JOB_TRY_COPYBACK_TIME,
    STMT_SET_JOB_DONE_COPYBACK_TIME,
    STMT_INC_JOB_DOWNLOAD_FAILED_COUNT,
    STMT_INC_JOB_COPYBACK_FAILED_COUNT,
    STMT_SET_DATASET_VERSION,
    STMT_GET_DATASET_VERSION,
    STMT_REMOVE_DATASET_VERSION,
    STMT_REMOVE_ALL_DATASET_VERSIONS,
    STMT_GET_ADMIN_VALUE,
    STMT_GET_JOB_COUNT_BY_FEATURE,
    STMT_GET_FAILED_JOB_COUNT_BY_FEATURE,

    STMT_ADD_JOB_DELETE,
    STMT_GET_JOB_DELETE_NOT_DONE_DISPATCH_BEFORE,
    STMT_INC_JOB_DELETE_FAILED_COUNT,
    STMT_SET_JOB_DELETE_DISPATCH_TIME,
    STMT_SET_JOB_DELETE_TRY_TIME,
    STMT_SET_JOB_DELETE_DONE_TIME,
    STMT_SET_JOB_DELETE_FAILED_INFO,
    STMT_GET_JOB_DELETE_BY_COMP_ID,
    STMT_GET_JOB_DELETE_COUNT,
    STMT_GET_JOB_DELETE_FAILED_COUNT,
    STMT_REMOVE_JOB_DELETE_BY_COMP_ID,
    STMT_REMOVE_ALL_JOB_DELETE,

    STMT_ADD_PICSTREAM_ITEM,
    STMT_GET_ITEM_BY_FILE_TYPE_AND_ALBUM,
    STMT_GET_ITEM_BY_COMP_ID,
    STMT_GET_ITEM_BY_NAME,
    STMT_GET_ITEM_BY_FULL_COMP_ID,
    STMT_REMOVE_ITEM_BY_COMP_ID,
    STMT_REMOVE_ITEM_BY_FULL_COMP_ID,

    STMT_ADD_SHARED_FILES_ITEM,
    STMT_GET_SHARED_FILES_BY_COMP_ID_AND_DSET,
    STMT_UPDATE_SHARED_FILES_THUMB_DOWNLOADED,
    STMT_GET_SHARED_FILES_BY_COMP_ID_AND_FEAT,
    STMT_DELETE_SHARED_FILES_ITEM_BY_COMP_ID_AND_DSET,

    STMT_MAX, // this must be last
};

// columns needed to populate SyncDownJob struct
#define JOB_COLS "id,"      \
                 "did,"     \
                 "cpath,"   \
                 "cid,"     \
                 "lpath,"   \
                 "dl_path," \
                 "dl_rev,"  \
                 "feature"

// columns needed to populate SyncDownJobEx struct
#define JOBEX_COLS "id,"         \
                   "did,"        \
                   "cpath,"      \
                   "cid,"        \
                   "lpath,"      \
                   "add_ts,"     \
                   "disp_ts,"    \
                   "dl_path,"    \
                   "dl_try_ts,"  \
                   "dl_failed,"  \
                   "dl_done_ts," \
                   "dl_rev,"     \
                   "cb_try_ts,"  \
                   "cb_failed,"  \
                   "cb_done_ts," \
                   "feature"

DEFSQL(BEGIN,                                   \
       "BEGIN IMMEDIATE");
DEFSQL(COMMIT,                                  \
       "COMMIT");
DEFSQL(ROLLBACK,                                \
       "ROLLBACK");
DEFSQL(ADD_JOB,                                                 \
       "INSERT INTO sd_job (did, cpath, cid, lpath, add_ts, feature, dl_rev) "   \
       "VALUES (:did, :cpath, :cid, :lpath, :add_ts, :feature, :rev)");
DEFSQL(REMOVE_JOB_BY_ID,                        \
       "DELETE FROM sd_job "                    \
       "WHERE id=:id");
DEFSQL(REMOVE_JOBS_BY_DATASETID,                \
       "DELETE FROM sd_job "                    \
       "WHERE did=:did");
DEFSQL(REMOVE_ALL_JOBS,                         \
       "DELETE FROM sd_job");
DEFSQL(GET_JOB,                                 \
       "SELECT "JOB_COLS" "                     \
       "FROM sd_job");
DEFSQL(GET_JOB_BY_ID,                           \
       "SELECT "JOB_COLS" "                     \
       "FROM sd_job "                           \
       "WHERE id=:id");
DEFSQL(GET_JOB_BY_LOCALPATH,                    \
       "SELECT "JOB_COLS" "                     \
       "FROM sd_job "                           \
       "WHERE lpath=:lpath");
DEFSQL(GET_JOB_BY_DATASETID,                    \
       "SELECT "JOB_COLS" "                     \
       "FROM sd_job "                           \
       "WHERE did=:did");
// SQL_GET_JOB_BY_COMPID_AND_DATASETID
DEFSQL(GET_JOB_BY_DATASETID_FAILED,             \
       "SELECT "JOB_COLS" "                     \
       "FROM sd_job "                           \
       "WHERE did=:did AND (dl_failed > 0 OR cb_failed > 0)");
DEFSQL(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX,   \
       "SELECT "JOB_COLS" "                     \
       "FROM sd_job "                           \
       "WHERE did=:did AND lpath GLOB :prefix||'*'");
DEFSQL(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX_FAILED,                    \
       "SELECT "JOB_COLS" "                                             \
       "FROM sd_job "                                                   \
       "WHERE did=:did AND (dl_failed > 0 OR cb_failed > 0) AND lpath GLOB :prefix||'*'");
DEFSQL(GET_JOB_DOWNLOAD_NOT_DONE_DISPATCH_BEFORE,                       \
       "SELECT "JOB_COLS" "                                             \
       "FROM sd_job "                                                   \
       "WHERE dl_done_ts IS NULL AND (disp_ts IS NULL OR disp_ts<:ts)");
DEFSQL(GET_JOB_COPYBACK_NOT_DONE_DISPATCH_BEFORE,                       \
       "SELECT "JOB_COLS" "                                             \
       "FROM sd_job "                                                   \
       "WHERE dl_done_ts IS NOT NULL AND cb_done_ts IS NULL AND (disp_ts IS NULL OR disp_ts<:ts)");
DEFSQL(GET_JOBEX_BY_ID,                         \
       "SELECT "JOBEX_COLS" "                   \
       "FROM sd_job "                           \
       "WHERE id=:id");
DEFSQL(SET_JOB_DOWNLOAD_PATH,                   \
       "UPDATE sd_job "                         \
       "SET dl_path=:dl_path "                  \
       "WHERE id=:id");
DEFSQL(SET_JOB_DOWNLOADED_REVISION,             \
       "UPDATE sd_job "                         \
       "SET dl_rev=:dl_rev "                    \
       "WHERE id=:id");
DEFSQL(SET_JOB_DISPATCH_TIME,                   \
       "UPDATE sd_job "                         \
       "SET disp_ts=:ts "                       \
       "WHERE id=:id");
DEFSQL(SET_JOB_TRY_DOWNLOAD_TIME,               \
       "UPDATE sd_job "                         \
       "SET dl_try_ts=:ts "                     \
       "WHERE id=:id");
DEFSQL(SET_JOB_DONE_DOWNLOAD_TIME,              \
       "UPDATE sd_job "                         \
       "SET dl_done_ts=:ts "                    \
       "WHERE id=:id");
DEFSQL(SET_JOB_TRY_COPYBACK_TIME,               \
       "UPDATE sd_job "                         \
       "SET cb_try_ts=:ts "                     \
       "WHERE id=:id");
DEFSQL(SET_JOB_DONE_COPYBACK_TIME,              \
       "UPDATE sd_job "                         \
       "SET cb_done_ts=:ts "                    \
       "WHERE id=:id");
DEFSQL(INC_JOB_DOWNLOAD_FAILED_COUNT,           \
       "UPDATE sd_job "                         \
       "SET dl_failed=dl_failed+1 "             \
       "WHERE id=:id");
DEFSQL(INC_JOB_COPYBACK_FAILED_COUNT,           \
       "UPDATE sd_job "                         \
       "SET cb_failed=cb_failed+1 "             \
       "WHERE id=:id");
DEFSQL(SET_DATASET_VERSION,                                     \
       "INSERT OR REPLACE INTO sd_version (did, version) "      \
       "VALUES (:did, :version)");
DEFSQL(GET_DATASET_VERSION,                     \
       "SELECT version "                        \
       "FROM sd_version "                       \
       "WHERE did=:did");
DEFSQL(REMOVE_DATASET_VERSION,                  \
       "DELETE FROM sd_version "                \
       "WHERE did=:did");
DEFSQL(REMOVE_ALL_DATASET_VERSIONS,             \
       "DELETE FROM sd_version");
DEFSQL(GET_ADMIN_VALUE,                         \
       "SELECT value "                          \
       "FROM sd_admin "                         \
       "WHERE id=:id");
DEFSQL(GET_JOB_COUNT_BY_FEATURE,                \
       "SELECT COUNT(*) "                       \
       "FROM sd_job "                           \
       "WHERE feature=:feature AND (disp_ts IS NULL OR disp_ts<=:ts)");
DEFSQL(GET_FAILED_JOB_COUNT_BY_FEATURE,         \
       "SELECT COUNT(*) "                       \
       "FROM sd_job "                           \
       "WHERE feature=:feature AND (dl_failed > 0 OR cb_failed > 0) AND disp_ts>:ts");

////////////// Delete Job 
DEFSQL(ADD_JOB_DELETE,                                                 \
       "INSERT INTO delete_job (cid, name, add_ts) "   \
       "VALUES (:cid, :name, :add_ts)");
DEFSQL(GET_JOB_DELETE_NOT_DONE_DISPATCH_BEFORE,                       \
       "SELECT id, cid, name, add_ts, disp_ts, del_try_ts, del_failed, del_done_ts "    \
       "FROM delete_job "                                                   \
       "WHERE del_done_ts IS NULL AND (disp_ts IS NULL OR disp_ts<:ts)");
DEFSQL(INC_JOB_DELETE_FAILED_COUNT,           \
       "UPDATE delete_job "                         \
       "SET del_failed=del_failed+1 "             \
       "WHERE id=:id");
DEFSQL(SET_JOB_DELETE_FAILED_INFO,           \
       "UPDATE delete_job "                         \
       "SET del_failed=del_failed+1, name=:name "             \
       "WHERE id=:id");
DEFSQL(SET_JOB_DELETE_DISPATCH_TIME,           \
       "UPDATE delete_job "                         \
       "SET disp_ts =:ts "             \
       "WHERE id=:id");
DEFSQL(SET_JOB_DELETE_TRY_TIME,           \
       "UPDATE delete_job "                         \
       "SET del_try_ts =:ts "             \
       "WHERE id=:id");
DEFSQL(SET_JOB_DELETE_DONE_TIME,           \
       "UPDATE delete_job "                         \
       "SET del_done_ts =:ts "             \
       "WHERE id=:id");
DEFSQL(GET_JOB_DELETE_BY_COMP_ID,                       \
       "SELECT id, cid, name, add_ts, disp_ts, del_try_ts, del_failed, del_done_ts "    \
       "FROM delete_job "                                                   \
       "WHERE cid=:compId");
DEFSQL(GET_JOB_DELETE_COUNT,                \
       "SELECT COUNT(*) "                       \
       "FROM delete_job "                           \
       "WHERE disp_ts IS NULL OR disp_ts<=:ts");
DEFSQL(GET_JOB_DELETE_FAILED_COUNT,         \
       "SELECT COUNT(*) "                       \
       "FROM delete_job "                           \
       "WHERE del_failed > 0 AND disp_ts>:ts");
DEFSQL(REMOVE_JOB_DELETE_BY_COMP_ID,                        \
       "DELETE FROM delete_job "                    \
       "WHERE cid=:id");
DEFSQL(REMOVE_ALL_JOB_DELETE,                         \
       "DELETE FROM delete_job");
////////////// End of Delete Job

//////////PicStream Items
DEFSQL(ADD_PICSTREAM_ITEM,                                                 \
       "INSERT OR REPLACE INTO picstream (comp_id , name, rev, orig_comp_id, origin_dev, identifier, title, album_name, file_type, file_size, date_time, lpath) "   \
       "VALUES (:compId, :name, :rev, :origCompId, :originDev, :identifier, :title, :albumName, :fileType, :fileSize, :takenTime, :lpath)");
DEFSQL(GET_ITEM_BY_FILE_TYPE_AND_ALBUM,                       \
       "SELECT comp_id , name, rev, orig_comp_id, origin_dev, identifier, title, album_name, file_type, file_size, date_time, lpath "    \
       "FROM picstream "                                                   \
       "WHERE file_type = :fileType AND album_name = :albumName");
DEFSQL(GET_ITEM_BY_COMP_ID,                       \
       "SELECT comp_id , name, rev, orig_comp_id, origin_dev, identifier, title, album_name, file_type, file_size, date_time, lpath "    \
       "FROM picstream "                                                   \
       "WHERE comp_id = :compId");
DEFSQL(GET_ITEM_BY_NAME,                       \
       "SELECT comp_id , name, rev, orig_comp_id, origin_dev, identifier, title, album_name, file_type, file_size, date_time, lpath "    \
       "FROM picstream "                                                   \
       "WHERE name = :name");
DEFSQL(GET_ITEM_BY_FULL_COMP_ID,                       \
       "SELECT comp_id , name, rev, orig_comp_id, origin_dev, identifier, title, album_name, file_type, file_size, date_time, lpath "    \
       "FROM picstream "                                                   \
       "WHERE comp_id = :fullCompId OR orig_comp_id = :fullCompId");
DEFSQL(REMOVE_ITEM_BY_COMP_ID,                  \
       "DELETE FROM picstream "                \
       "WHERE comp_id = :CompId");
DEFSQL(REMOVE_ITEM_BY_FULL_COMP_ID,                  \
       "DELETE FROM picstream "                \
       "WHERE comp_id = :fullCompId OR orig_comp_id = :fullCompId");
///////// End of PicStream Items

//////// Shared Files
// Share files sql defined with the function that uses them
// SQL_ADD_SHARED_FILES_ITEM
// SQL_GET_SHARED_FILES_BY_COMP_ID_AND_DSET
// SQL_UPDATE_SHARED_FILES_THUMB_DOWNLOADED
// SQL_GET_SHARED_FILES_BY_COMP_ID_AND_FEAT
// SQL_DELETE_SHARED_FILES_ITEM_BY_COMP_ID_AND_DSET

SyncDownJobs::SyncDownJobs(const std::string &workdir)
    : workdir(workdir), db(NULL), purge_on_close(false)
{
    dbpath = workdir + "db";

    int err = VPLMutex_Init(&mutex);
    if (err) {
        LOG_ERROR("Failed to initialize mutex: %d", err);
    }
}

SyncDownJobs::~SyncDownJobs()
{
    int err;
    {
        MutexAutoLock lock(&mutex);  // Expecting no contention here.  SyncDownJobs should be already quiet.
        err = closeDB();
        if (err) {
            LOG_ERROR("Failed to close DB: %d", err);
        }
    }
    err = VPLMutex_Destroy(&mutex);
    if (err) {
        LOG_ERROR("Failed to destroy mutex: %d", err);
    }
}

int SyncDownJobs::migrate_schema_from_v1_to_v2()
{
    int result = 0;
    int dberr = 0;
    char *errmsg = NULL;

    // Set default feature to SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES = 60
    // Reasons:
    // 1. From CCD SDK v2.5.4 RC1, Cloud Docs will download by CloudDocsMgr.cpp,
    //    SyncDown.cpp only handle SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES and SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES.
    // 2. Only Win32 PC downloads SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES.
    // 3. Win32 PC Photo application does not show the download progress to user.
    static const char* SQL_MIGRATE_SCHEMA_FROM_V1_TO_V2 =
        "ALTER TABLE sd_job ADD feature INTEGER;"   \
        "UPDATE sd_job SET feature=60;"             \
        "UPDATE sd_admin SET value="XSTR(TARGET_SCHEMA_VERSION)" WHERE id="XSTR(ADMINID_SCHEMA_VERSION)"";

    dberr = sqlite3_exec(db, SQL_MIGRATE_SCHEMA_FROM_V1_TO_V2, NULL, NULL, &errmsg);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to migrate schema from v1 to v2: %d, %s", dberr, errmsg);
        sqlite3_free(errmsg);
        result = mapSqliteErrCode(dberr);
    }

    return result;
}

int SyncDownJobs::migrate_schema_from_v2_to_v3()
{
    int result = 0;
    int dberr = 0;
    char *errmsg = NULL;

    static const char* SQL_MIGRATE_SCHEMA_FROM_V2_TO_V3 =
        "UPDATE sd_admin SET value="XSTR(TARGET_SCHEMA_VERSION)" WHERE id="XSTR(ADMINID_SCHEMA_VERSION)"";

    dberr = sqlite3_exec(db, SQL_MIGRATE_SCHEMA_FROM_V2_TO_V3, NULL, NULL, &errmsg);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to migrate schema from v2 to v3: %d, %s", dberr, errmsg);
        sqlite3_free(errmsg);
        result = mapSqliteErrCode(dberr);
    }

    return result;
}

int SyncDownJobs::migrate_schema_from_v1_to_v3()
{
    int result = 0;
    result = migrate_schema_from_v1_to_v2();
    if(result != 0)
        return result;
    return migrate_schema_from_v2_to_v3();
}

int SyncDownJobs::openDB() {
    int result = 0;
    ASSERT(VPLMutex_LockedSelf(&mutex));

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
        LOG_INFO("opening DB(%s):%p", dbpath.c_str(), db);

        char *errmsg = NULL;
        dberr = sqlite3_exec(db, test_and_create_db_sql_v3, NULL, NULL, &errmsg);
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

        if (schemaVersion == 1) {
            // Need to Migrate DB since the current DB version is smaller than TARGET_SCHEMA_VERSION
            result = migrate_schema_from_v1_to_v3();
            if (result) {
                LOG_ERROR("Failed to migrate sync down DB from v1 to v2: %d", result);
                goto end;
            }
        } else if (schemaVersion == 2) {
            result = migrate_schema_from_v2_to_v3();
            if (result) {
                LOG_ERROR("Failed to migrate sync down DB from v1 to v2: %d", result);
                goto end;
            }
        } else if (schemaVersion < TARGET_SCHEMA_VERSION || schemaVersion > TARGET_SCHEMA_VERSION) {
            LOG_ERROR("Unhandled schema version: "FMTu64, schemaVersion);
            result = CCD_ERROR_NOT_IMPLEMENTED;
            goto end;
        } else {
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

int SyncDownJobs::closeDB()
{
    int result = 0;
    ASSERT(VPLMutex_LockedSelf(&mutex));
    if (db) {
        std::vector<sqlite3_stmt*>::iterator it;
        for (it = dbstmts.begin(); it != dbstmts.end(); it++) {
            if (*it != NULL) {
                sqlite3_finalize(*it);
            }
            *it = NULL;
        }

        int temp_rc = sqlite3_close(db);
        if (temp_rc != 0) {
            LOG_ERROR("sqlite3_close(%p):%d", db, temp_rc);
        }
        db = NULL;
        LOG_INFO("Closed DB(%s)", dbpath.c_str());
    } else {
        LOG_INFO("DB(%s) already closed", dbpath.c_str());
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

int SyncDownJobs::beginTransaction()
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

int SyncDownJobs::commitTransaction()
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

int SyncDownJobs::rollbackTransaction()
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

int SyncDownJobs::getAdminValue(u64 adminId, u64 &value)
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

int SyncDownJobs::setDatasetVersion(u64 datasetid, u64 version)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_DATASET_VERSION)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_DATASET_VERSION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetid);
    CHECK_BIND(dberr, result, db, end);
    // bind :version
    dberr = sqlite3_bind_int64(stmt, 2, (s64)version);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = CCD_ERROR_NOT_FOUND;
        break;
    case 1:  // expected
        break;
    default:  // SHOULD NOT HAPPEN - "did" is primary key; at most one row can change.
        result = CCD_ERROR_INTERNAL;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::getDatasetVersion(u64 datasetid, u64 &version)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_DATASET_VERSION)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_DATASET_VERSION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        version = (u64)sqlite3_column_int64(stmt, 0);
    }
    else {  // SQLITE_DONE, meaning not found
        result = CCD_ERROR_NOT_FOUND;  // to indicate version missing; note that this is a valid outcome
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    return result;
}

int SyncDownJobs::removeDatasetVersion(u64 datasetId)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_DATASET_VERSION)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_DATASET_VERSION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = CCD_ERROR_NOT_FOUND;
        break;
    case 1:  // expected
        break;
    default:  // SHOULD NOT HAPPEN - "did" is primary key; at most one row can change.
        result = CCD_ERROR_INTERNAL;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    return result;
}

int SyncDownJobs::removeAllDatasetVersions()
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_ALL_DATASET_VERSIONS)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_ALL_DATASET_VERSIONS), -1, &stmt, NULL);
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

int SyncDownJobs::addJob(u64 did, const std::string &cpath, u64 cid,
                         const std::string &lpath, 
                         u64 timestamp, u64 syncfeature, u64 rev, /*OUT*/ u64 &jobid)
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
        // bind :did
        dberr = sqlite3_bind_int64(stmt, n++, (s64)did);
        CHECK_BIND(dberr, result, db, end);
        // bind :cpath
        dberr = sqlite3_bind_text(stmt, n++, cpath.data(), cpath.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // bind :cid
        dberr = sqlite3_bind_int64(stmt, n++, (s64)cid);
        CHECK_BIND(dberr, result, db, end);
        // bind :lpath
        dberr = sqlite3_bind_text(stmt, n++, lpath.data(), lpath.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // bind :add_ts
        dberr = sqlite3_bind_int64(stmt, n++, (s64)timestamp);
        CHECK_BIND(dberr, result, db, end);
        // bind :feature
        dberr = sqlite3_bind_int64(stmt, n++, (s64)syncfeature);
        CHECK_BIND(dberr, result, db, end);
        // bind :feature
        dberr = sqlite3_bind_int64(stmt, n++, (s64)rev);
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

int SyncDownJobs::removeJob(u64 jobid)
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

int SyncDownJobs::removeJobsByDatasetId(u64 datasetId)
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

int SyncDownJobs::removeAllJobs()
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

int SyncDownJobs::findJob(u64 jobid, /*OUT*/ SyncDownJob &job)
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

int SyncDownJobs::findJob(const std::string &localpath, /*OUT*/ SyncDownJob &job)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_BY_LOCALPATH)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_LOCALPATH), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :cpath
    dberr = sqlite3_bind_text(stmt, 1, localpath.data(), localpath.size(), NULL);
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

#define findJobNotDoneNoRecentDispatch(Operation,OPERATION)             \
int SyncDownJobs::findJob##Operation##NotDoneNoRecentDispatch(u64 cutoff, SyncDownJob &job) \
{                                                                       \
    int result = 0;                                                     \
    int dberr = 0;                                                      \
                                                                        \
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_##OPERATION##_NOT_DONE_DISPATCH_BEFORE)]; \
    if (!stmt) {                                                        \
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_##OPERATION##_NOT_DONE_DISPATCH_BEFORE), -1, &stmt, NULL); \
        CHECK_PREPARE(dberr, result, db, end);                          \
    }                                                                   \
                                                                        \
    dberr = sqlite3_bind_int64(stmt, 1, (s64)cutoff);                   \
    CHECK_BIND(dberr, result, db, end);                                 \
                                                                        \
    dberr = sqlite3_step(stmt);                                         \
    CHECK_STEP(dberr, result, db, end);                                 \
    if (dberr == SQLITE_ROW) {                                          \
        job.populateFrom(stmt);                                         \
    }                                                                   \
    else {  /* assert: dberr == SQLITE_DONE */                          \
        result = CCD_ERROR_NOT_FOUND;                                   \
    }                                                                   \
                                                                        \
 end:                                                                   \
    if (stmt) {                                                         \
        sqlite3_reset(stmt);                                            \
    }                                                                   \
                                                                        \
    return result;                                                      \
}

// define findJobDownloadNotDoneNoRecentDispatch()
// uses GET_JOB_DOWNLOAD_NOT_DONE_DISPATCH_BEFORE (sql_GET_JOB_DOWNLOAD_NOT_DONE_DISPATCH_BEFORE)
findJobNotDoneNoRecentDispatch(Download,DOWNLOAD)
// define findJobCopybackNotDoneNoRecentDispatch()
// uses GET_JOB_COPYBACK_NOT_DONE_DISPATCH_BEFORE (sql_GET_JOB_COPYBACK_NOT_DONE_DISPATCH_BEFORE)
findJobNotDoneNoRecentDispatch(Copyback,COPYBACK)

#undef findJobNotDoneNoRecentDispatch

int SyncDownJobs::findJobEx(u64 jobid, /*OUT*/ SyncDownJobEx &job)
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

int SyncDownJobs::visitJobs(JobVisitor visitor, void *param)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        SyncDownJob job;
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

int SyncDownJobs::visitJobsByDatasetId(u64 datasetId, bool fails_only,
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
        SyncDownJob job;
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

int SyncDownJobs::visitJobsByDatasetIdLocalpathPrefix(u64 datasetId,
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
        sqlite3_stmt *&stmtInit = dbstmts[SQLNUM(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX_FAILED)];
        if (!stmtInit) {
            dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX_FAILED), -1, &stmtInit, NULL);
            CHECK_PREPARE(dberr, result, db, end);
        }
        stmtChoice = &dbstmts[SQLNUM(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX_FAILED)];
    }else{
        sqlite3_stmt *&stmtInit = dbstmts[SQLNUM(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX)];
        if (!stmtInit) {
            dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX), -1, &stmtInit, NULL);
            CHECK_PREPARE(dberr, result, db, end);
        }
        stmtChoice = &dbstmts[SQLNUM(GET_JOB_BY_DATASETID_LOCALPATH_PREFIX)];
    }

    stmt = *stmtChoice;

    // bind :did
    dberr = sqlite3_bind_int64(stmt, 1, (s64)datasetId);
    CHECK_BIND(dberr, result, db, end);

    // bind :prefix
    dberr = sqlite3_bind_text(stmt, 2, prefix.data(), prefix.size(), NULL);
    CHECK_BIND(dberr, result, db, end);

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        SyncDownJob job;
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

int SyncDownJobs::setJobDownloadPath(u64 id, const std::string &downloadpath)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_JOB_DOWNLOAD_PATH)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_JOB_DOWNLOAD_PATH), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :dl_path
    dberr = sqlite3_bind_text(stmt, 1, downloadpath.data(), downloadpath.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = CCD_ERROR_NOT_FOUND;
        break;
    case 1:  // expected
        break;
    default:  // SHOULD NOT HAPPEN - "id" is primary key; at most one row can change.
        result = CCD_ERROR_INTERNAL;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::setJobDownloadedRevision(u64 jobid, u64 revision)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_JOB_DOWNLOADED_REVISION)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_JOB_DOWNLOADED_REVISION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :dl_rev
    dberr = sqlite3_bind_int64(stmt, 1, (s64)revision);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)jobid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = CCD_ERROR_NOT_FOUND;
        break;
    case 1:  // expected
        break;
    default:  // SHOULD NOT HAPPEN - "id" is primary key; at most one row can change.
        result = CCD_ERROR_INTERNAL;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

#define setJobTime(EventName,EVENT_NAME)                                \
int SyncDownJobs::setJob##EventName##Time(u64 jobid, u64 timestamp)     \
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
// uses SET_JOB_DISPATCH_TIME (sql_SET_JOB_DISPATCH_TIME)
setJobTime(Dispatch,DISPATCH)
// define setJobTryDownloadTime()
// uses SET_JOB_TRY_DOWNLOAD_TIME (sql_SET_JOB_TRY_DOWNLOAD_TIME)
setJobTime(TryDownload,TRY_DOWNLOAD)
// define setJobDoneDownloadTime()
// uses SET_JOB_DONE_DOWNLOAD_TIME (sql_SET_JOB_DONE_DOWNLOAD_TIME)
setJobTime(DoneDownload,DONE_DOWNLOAD)
// define setJobTryCopybackTime()
// uses SET_JOB_TRY_COPYBACK_TIME (sql_SET_JOB_TRY_COPYBACK_TIME)
setJobTime(TryCopyback,TRY_COPYBACK)
// define setJobDoneCopybackTime()
// uses SET_JOB_DONE_COPYBACK_TIME (sql_SET_JOB_DONE_COPYBACK_TIME)
setJobTime(DoneCopyback,DONE_COPYBACK)

// define setJobDeleteDispatchTime()
// uses SET_JOB_DELETE_Dispatch_TIME (sql_SET_JOB_DELETE_DISPATCH_TIME)
setJobTime(DeleteDispatch,DELETE_DISPATCH)
// define setJobDeleteDoneTime()
// uses SET_JOB_DELETE_TRY_TIME (sql_SET_JOB_DELETE_TRY_TIME)
setJobTime(DeleteTry,DELETE_TRY)
// define setJobDeleteDoneTime()
// uses SET_JOB_DELETE_DONE_TIME (sql_SET_JOB_DELETE_DONE_TIME)
setJobTime(DeleteDone,DELETE_DONE)

#undef setJobTime

#define incJobFailedCount(Operation,OPERATION)                          \
int SyncDownJobs::incJob##Operation##FailedCount(u64 jobid)             \
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

// define incJobDownloadFailedCount()
// uses INC_JOB_DOWNLOAD_FAILED_COUNT
incJobFailedCount(Download,DOWNLOAD)
// define incJobCopybackFailedCount()
// uses INC_JOB_COPYBACK_FAILED_COUNT
incJobFailedCount(Copyback,COPYBACK)

// define incJobDeleteFailedCount()
// uses INC_JOB_DELETE_FAILED_COUNT
incJobFailedCount(Delete,DELETE)

#undef incJobFailedCount

int SyncDownJobs::setPurgeOnClose(bool purge)
{
    purge_on_close = purge;

    return 0;
}

int SyncDownJobs::Check()
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncDownJobs::SetDatasetLastCheckedVersion(u64 datasetid, u64 version)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setDatasetVersion(datasetid, version);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncDownJobs::GetDatasetLastCheckedVersion(u64 datasetid, u64 &version)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = getDatasetVersion(datasetid, version);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncDownJobs::ResetDatasetLastCheckedVersion(u64 datasetId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = removeDatasetVersion(datasetId);
    if (err == CCD_ERROR_NOT_FOUND) {
        result = err;
        LOG_WARN("Cannot find the Dataset Id, "FMTu64", in DB", datasetId);
        goto end;
    } else {
        CHECK_ERR(err, result, end);
    }
    
 end:
    return result;
}

int SyncDownJobs::ResetAllDatasetLastCheckedVersions()
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = removeAllDatasetVersions();
    CHECK_ERR(err, result, end);

 end:
    return result;
}

static int deleteTmpFile(SyncDownJob &job, void *param)
{
    if (!job.dl_path.empty()) {
        std::string &workdir = *(std::string*)param;
        std::string dl_path;
        extendDlPathToFullPath(job.dl_path, workdir, dl_path);
        int err = VPLFile_Delete(dl_path.c_str());
        if (err) {
            LOG_INFO("Failed to delete tmp file(%s): %d. Typically successfully moved to destination",
                     dl_path.c_str(), err);
        }
    }
    return 0;
}

int SyncDownJobs::AddJob(u64 datasetid, const std::string &comppath, u64 compid,
                         const std::string &localpath, u64 syncfeature, u64 rev,
                         u64 &jobid_out)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);
        err = beginTransaction();
        CHECK_ERR(err, result, end);

        SyncDownJob job;
        err = findJob(localpath, job);
        if (err == CCD_ERROR_NOT_FOUND) {
            goto no_prior_job;
        }
        CHECK_ERR(err, result, end);
        deleteTmpFile(job, &workdir);
        err = removeJob(job.id);
        CHECK_ERR(err, result, end);

    no_prior_job:
        err = addJob(datasetid, comppath, compid, localpath, VPLTime_GetTime(), syncfeature, rev, jobid_out);
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

int SyncDownJobs::RemoveJob(u64 jobid)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        SyncDownJob job;
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

int SyncDownJobs::RemoveJobsByDatasetId(u64 datasetId)
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

int SyncDownJobs::RemoveAllJobs()
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

int SyncDownJobs::RemoveJobDelete(u64 compid)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        DeleteJob job;
        err = findJobDelete(compid, job);
        if (err == CCD_ERROR_NOT_FOUND) {
            goto end;
        }
        CHECK_ERR(err, result, end);
        
        err = removeJobDelete(compid);
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncDownJobs::RemoveAllJobDelete()
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        err = removeAllJobDelete();
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}
#define GetNextJobTo(Operation)                                         \
int SyncDownJobs::GetNextJobTo##Operation(u64 cutoff, /*OUT*/SyncDownJob &job) \
{                                                                       \
    int result = 0;                                                     \
    int err = 0;                                                        \
                                                                        \
    MutexAutoLock lock(&mutex);                                         \
                                                                        \
    err = openDB();                                                     \
    CHECK_ERR(err, result, end);                                        \
                                                                        \
    err = findJob##Operation##NotDoneNoRecentDispatch(cutoff, job);     \
    CHECK_ERR(err, result, end);                                        \
                                                                        \
 end:                                                                   \
    return result;                                                      \
}

// define GetNextJobToDownload()
// calls findJobDownloadNotDoneNoRecentDispatch()
GetNextJobTo(Download)
// define GetNextJobToCopyback()
// calls findJobCopybackNotDoneNoRecentDispatch()
GetNextJobTo(Copyback)

#undef GetNextJobTo

int SyncDownJobs::SetJobDownloadPath(u64 jobid, const std::string &downloadpath)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDownloadPath(jobid, downloadpath);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncDownJobs::SetJobDownloadedRevision(u64 jobid, u64 revision)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDownloadedRevision(jobid, revision);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncDownJobs::TimestampJobDispatch(u64 jobid, u64 vplCurrTimeSec)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDispatchTime(jobid, vplCurrTimeSec);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

#define TimestampJob(EventName)                                         \
int SyncDownJobs::TimestampJob##EventName(u64 jobid)\
{                                                                       \
    int result = 0;                                                     \
    int err = 0;                                                        \
                                                                        \
    MutexAutoLock lock(&mutex);                                         \
                                                                        \
    err = openDB();                                                     \
    CHECK_ERR(err, result, end);                                        \
                                                                        \
    err = setJob##EventName##Time(jobid, VPLTime_GetTime());            \
    CHECK_ERR(err, result, end);                                        \
                                                                        \
 end:                                                                   \
    return result;                                                      \
}

// define TimestampJobTryDownload()
// calls setJobTryDownloadTime()
TimestampJob(TryDownload)
// define TimestampJobDoneDownload()
// calls setJobDoneDownloadTime()
TimestampJob(DoneDownload)
// define TimestampJobTryCopyback()
// calls setJobTryCopybackTime()
TimestampJob(TryCopyback)
// define TimestampJobDoneCopyback()
// calls setJobDoneCopybackTime()
TimestampJob(DoneCopyback)

// define TimestampJobDeleteDispatch()
// calls setJobDeleteDispatchTime()
TimestampJob(DeleteDispatch)
// define TimestampJobDeleteTry()
// calls setJobDeleteTryTime()
TimestampJob(DeleteTry)
// define TimestampJobDeleteDone()
// calls setJobDeleteDoneTime()
TimestampJob(DeleteDone)

#undef TimestampJob

#define ReportJobFailed(Operation)                              \
int SyncDownJobs::ReportJob##Operation##Failed(u64 jobid)       \
{                                                               \
    int result = 0;                                             \
    int err = 0;                                                \
                                                                \
    MutexAutoLock lock(&mutex);                                 \
                                                                \
    err = openDB();                                             \
    CHECK_ERR(err, result, end);                                \
                                                                \
    err = incJob##Operation##FailedCount(jobid);                \
    CHECK_ERR(err, result, end);                                \
                                                                \
 end:                                                           \
    return result;                                              \
}

// define ReportJobDownloadFailed()
// calls incJobDownloadFailedCount()
ReportJobFailed(Download)
// define ReportJobCopybackFailed()
// calls incJobCopybackFailedCount()
ReportJobFailed(Copyback)

// define ReportJobDeleteFailed()
// calls incJobDeleteFailedCount()
ReportJobFailed(Delete)

#undef ReportJobFailed

int SyncDownJobs::removeJobDelete(u64 compid)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_JOB_DELETE_BY_COMP_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_JOB_DELETE_BY_COMP_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :compid
    dberr = sqlite3_bind_int64(stmt, 1, (s64)compid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::removeAllJobDelete()
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_ALL_JOB_DELETE)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_ALL_JOB_DELETE), -1, &stmt, NULL);
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

int SyncDownJobs::PurgeOnClose()
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = setPurgeOnClose(true);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

static int stopAfterOneJob(SyncDownJob &job, void *param)
{
    bool* jobExists = (bool*)param;
    *jobExists = true;
    return OK_STOP_DB_TRAVERSE;
}

int SyncDownJobs::QueryStatus(u64 datasetId,
                              const std::string& path,
                              ccd::FeatureSyncStateSummary& syncState_out)
{
    // Ensure that path ends with a '/'
    bool jobExists = false;
    bool errorJobExists = false;
    int rc;
    syncState_out.set_status(ccd::CCD_FEATURE_STATE_OUT_OF_SYNC);

    MutexAutoLock lock(&mutex);

    if (!db) {
        rc = openDB();
        if(rc != 0) {
            return rc;
        }
    }

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
        rc = visitJobsByDatasetIdLocalpathPrefix(datasetId,
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
            rc = visitJobsByDatasetIdLocalpathPrefix(datasetId,
                                                     mutablePath,
                                                     true,
                                                     stopAfterOneJob,
                                                     &errorJobExists);
            if(rc != 0) {
                LOG_ERROR("visitJobsByDatasetIdLocalpathPrefix("FMTu64",%s):%d",
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

int SyncDownJobs::GetJobCountByFeature(u64 feature,
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
    if (result) {
        jobCount_out = 0;
        failedJobCount_out = 0;
    }
    return result;
}

int SyncDownJobs::AddDeleteJob(u64 compId, const std::string &name, 
               u64 timestamp, /*OUT*/ u64 &jobid)
{
    int result = 0;

    MutexAutoLock lock(&mutex);
    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);
        DeleteJob job;
        err = findJobDelete(compId, job);
        if (err == CCD_ERROR_NOT_FOUND) {
            err = addDeleteJob(compId, name, timestamp, jobid);
            CHECK_ERR(err, result, end);
        } else {
            LOG_WARN("Item:"FMTu64" (%s) is already in delete list.", job.cid, job.name.c_str());
        }
        CHECK_ERR(err, result, end);
    }

 end:
    if (result) {
        rollbackTransaction();
    }
    return result;
}

int SyncDownJobs::addDeleteJob(u64 cid, const std::string &name, 
               u64 timestamp, /*OUT*/ u64 &jobid)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ADD_JOB_DELETE)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(ADD_JOB_DELETE), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    {
        int n = 1;
        // bind :cid
        dberr = sqlite3_bind_int64(stmt, n++, (s64)cid);
        CHECK_BIND(dberr, result, db, end);
        // bind :name
        dberr = sqlite3_bind_text(stmt, n++, name.data(), name.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // bind :add_ts
        dberr = sqlite3_bind_int64(stmt, n++, (s64)timestamp);
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

int SyncDownJobs::GetDeleteJobCount(u64 cutoff,
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

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_DELETE_COUNT)];
    sqlite3_stmt *&stmt2 = dbstmts[SQLNUM(GET_JOB_DELETE_FAILED_COUNT)];

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_DELETE_COUNT), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :cutoff
    dberr = sqlite3_bind_int64(stmt, 1, (s64)cutoff);
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
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_DELETE_FAILED_COUNT), -1, &stmt2, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :cutoff
    dberr = sqlite3_bind_int64(stmt2, 1, (s64)cutoff);
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
    if (result) {
        jobCount_out = 0;
        failedJobCount_out = 0;
    }
    return result;
}

int SyncDownJobs::findJobDeleteNotDoneNoRecentDispatch(u64 cutoff, DeleteJob &job)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_DELETE_NOT_DONE_DISPATCH_BEFORE)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_DELETE_NOT_DONE_DISPATCH_BEFORE), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_bind_int64(stmt, 1, (s64)cutoff);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        job.populateFrom(stmt);
    }
    else {  /* assert: dberr == SQLITE_DONE */
        result = CCD_ERROR_NOT_FOUND;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    return result;
}

int SyncDownJobs::GetNextJobToDelete(u64 cutoff, /*OUT*/DeleteJob &job)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = findJobDeleteNotDoneNoRecentDispatch(cutoff, job);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncDownJobs::findJobDelete(u64 compId, /*OUT*/ DeleteJob &job)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_DELETE_BY_COMP_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_DELETE_BY_COMP_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)compId);
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

int SyncDownJobs::SetJobDeleteFailedInfo(u64 jobid, std::string &lpath)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDeleteFailedInfo(jobid, lpath);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int SyncDownJobs::setJobDeleteFailedInfo(u64 jobid, std::string &lpath)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_JOB_DELETE_FAILED_INFO)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_JOB_DELETE_FAILED_INFO), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_bind_text(stmt, 1, lpath.data(), lpath.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
    dberr = sqlite3_bind_int64(stmt, 2, (s64)jobid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = CCD_ERROR_NOT_FOUND;
        break;
    case 1:  /* expected */
        result = VPL_OK;
        break;
    default:  /* SHOULD NOT HAPPEN - "id" is primary key; at most one row can change. */
        result = CCD_ERROR_INTERNAL;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

#define DB_BIND_INT64(var)                                \
    dberr = sqlite3_bind_int64(stmt, n++, (s64)var);      \
    CHECK_BIND(dberr, result, db, end);

#define DB_BIND_TEXT(var)                                                  \
    dberr = sqlite3_bind_text(stmt, n++, var.data(), var.size(), NULL); \
        CHECK_BIND(dberr, result, db, end);

int SyncDownJobs::AddPicStreamItem(u64 compId,
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
                                   bool &syncUpAdded)
{
    int result = 0;
    syncUpAdded = false;
    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        PicStreamItem item;
        //err = findPicStreamItem(compId, item);
        err = findPicStreamItem(name, item);
        if (err != CCD_ERROR_NOT_FOUND) {
            // Items (full/low/thumbnail), whether they are not subscribed or not, are 
            // added to PicStream DB for App to query when getChanges().
            // (The subscribed item is added with lpath.)
            // But, Once un-subscribed items are subscribed later, the records in DB should be overwritten with lpath.
            // To achieve this goal, we use "Insert or Replace into DB" here to update lpath.
            // So, addPicStreamItems can be called again.

            if(item.identifier.length() > 0) {
                LOG_WARN("name: %s compId:"FMTu64" is already added by SyncUp.", item.name.c_str(), compId);
                result = 0;
                syncUpAdded = true;
                goto end;
            } else {
                LOG_WARN("Item:"FMTu64" is already in PicStream DB. It will be replaced.", compId);
            }
        }
        err = addPicStreamItem(compId, name, rev, origCompId, originDev, identifier,
                               title, albumName, fileType, fileSize, takenTime, lpath, jobid_out);
        CHECK_ERR(err, result, end);
    }

 end:
    if (result) {
        rollbackTransaction();
    }
    return result;
}


int SyncDownJobs::getJobByCompIdAndDatasetId(u64 compId,
                                             u64 datasetId,
                                             SyncDownJob& job_out)
{
    int result = 0;
    int dberr = 0;

    static const char* SQL_GET_JOB_BY_COMPID_AND_DATASETID =
           "SELECT "JOB_COLS" "
           "FROM sd_job "
           "WHERE cid=? AND did=? "
           "LIMIT 1";

    MutexAutoLock lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_BY_COMPID_AND_DATASETID)];

    dberr = openDB();
    CHECK_ERR(dberr, result, end);

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, SQL_GET_JOB_BY_COMPID_AND_DATASETID, -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    {
        int bindPos = 0;
        // comp_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, compId);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);
        // dataset_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, datasetId);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);

        ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));
    }
    dberr = sqlite3_step(stmt);
    DB_UTIL_CHECK_STEP(dberr, result, db, end);
    DB_UTIL_CHECK_ROW_EXIST(dberr, result, db, end);

    job_out.populateFrom(stmt);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

#define SHARED_FILES_COLUMNS "comp_id,"               \
                             "name,"                  \
                             "dataset_id,"            \
                             "feature,"               \
                             "b_thumb_downloaded,"    \
                             "recipient_list,"        \
                             "revision,"              \
                             "size,"                  \
                             "last_changed,"          \
                             "create_date,"           \
                             "update_device,"         \
                             "rel_thumb_path,"        \
                             "opaque_metadata"

int SyncDownJobs::sharedFiles_readRow(sqlite3_stmt* stmt,
                                      SharedFilesDbEntry& row_out)
{
    int rv = 0;
    int resIndex = 0;
    row_out.clear();

    DB_UTIL_GET_SQLITE_U64(     stmt,
                                row_out.compId,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_STR(     stmt,
                                row_out.name,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_U64(     stmt,
                                row_out.datasetId,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_U64(     stmt,
                                row_out.feature,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_BOOL(    stmt,
                                row_out.b_thumb_downloaded,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_STR(     stmt,
                                row_out.recipientList,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_U64(     stmt,
                                row_out.revision,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_U64(     stmt,
                                row_out.size,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_U64_NULL(stmt,
                                row_out.lastChangedExists, row_out.lastChanged,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_U64_NULL(stmt,
                                row_out.createDateExists, row_out.createDate,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_U64_NULL(stmt,
                                row_out.updateDeviceExists, row_out.updateDevice,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_STR_NULL(stmt,
                                row_out.relThumbPathExists, row_out.relThumbPath,
                                resIndex++, rv, end);
    DB_UTIL_GET_SQLITE_STR_NULL(stmt,
                                row_out.opaqueMetadataExists, row_out.opaqueMetadata,
                                resIndex++, rv, end);
 end:
    return rv;
}

int SyncDownJobs::sharedFilesItemGet(u64 datasetId, u64 compId,
                                     SharedFilesDbEntry& entry_out)
{
    int result = 0;
    int dberr = 0;
    const char* SQL_GET_SHARED_FILES_BY_COMP_ID_AND_DSET =
        "SELECT "SHARED_FILES_COLUMNS" FROM shared_files "
        "WHERE comp_id=? AND dataset_id=?";

    MutexAutoLock lock(&mutex);
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_SHARED_FILES_BY_COMP_ID_AND_DSET)];

    dberr = openDB();
    CHECK_ERR(dberr, result, end);

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, SQL_GET_SHARED_FILES_BY_COMP_ID_AND_DSET, -1, &stmt, NULL);
        DB_UTIL_CHECK_PREPARE(dberr, result, db, end);
    }
    {
        int bindPos = 0;
        // comp_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, compId);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);
        // dataset_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, datasetId);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);

        ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));
    }
    dberr = sqlite3_step(stmt);
    DB_UTIL_CHECK_STEP(dberr, result, db, end);
    DB_UTIL_CHECK_ROW_EXIST(dberr, result, db, end);

    result = sharedFiles_readRow(stmt, entry_out);
    if (result != 0) {
        LOG_ERROR("sharedFiles_readRow:%d", result);
        goto end;
    }
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::sharedFilesItemAdd(const SharedFilesDbEntry& entry)
{
    int result = 0;
    int dberr = 0;

    const char* SQL_ADD_SHARED_FILES_ITEM =
        "INSERT OR REPLACE INTO shared_files ("SHARED_FILES_COLUMNS") "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)";

    MutexAutoLock lock(&mutex);
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ADD_SHARED_FILES_ITEM)];

    dberr = openDB();
    CHECK_ERR(dberr, result, end);

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, SQL_ADD_SHARED_FILES_ITEM, -1, &stmt, NULL);
        DB_UTIL_CHECK_PREPARE(dberr, result, db, end);
    }
    {
        int bindPos = 0;
        // comp_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.compId);
        CHECK_BIND(dberr, result, db, end);
        // name
        dberr = sqlite3_bind_text(stmt, ++bindPos,
                                  entry.name.data(), entry.name.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // dataset_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.datasetId);
        CHECK_BIND(dberr, result, db, end);
        // feature
        dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.feature);
        CHECK_BIND(dberr, result, db, end);
        // b_thumb_downloaded
        if (entry.b_thumb_downloaded) {
            dberr = sqlite3_bind_int64(stmt, ++bindPos, SYNC_DOWN_SQLITE3_TRUE);
            CHECK_BIND(dberr, result, db, end);
        } else {
            dberr = sqlite3_bind_int64(stmt, ++bindPos, SYNC_DOWN_SQLITE3_FALSE);
            CHECK_BIND(dberr, result, db, end);
        }
        // recipient_list
        dberr = sqlite3_bind_text(stmt, ++bindPos,
                                  entry.recipientList.data(), entry.recipientList.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // revision
        dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.revision);
        CHECK_BIND(dberr, result, db, end);
        // size
        dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.size);
        CHECK_BIND(dberr, result, db, end);
        // last_changed
        if (entry.lastChangedExists) {
            dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.lastChanged);
            CHECK_BIND(dberr, result, db, end);
        } else {
            dberr = sqlite3_bind_null(stmt, ++bindPos);
            CHECK_BIND(dberr, result, db, end);
        }
        // create_date
        if (entry.createDateExists) {
            dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.createDate);
            CHECK_BIND(dberr, result, db, end);
        } else {
            dberr = sqlite3_bind_null(stmt, ++bindPos);
            CHECK_BIND(dberr, result, db, end);
        }
        // update_device
        if (entry.updateDeviceExists) {
            dberr = sqlite3_bind_int64(stmt, ++bindPos, entry.updateDevice);
            CHECK_BIND(dberr, result, db, end);
        } else {
            dberr = sqlite3_bind_null(stmt, ++bindPos);
            CHECK_BIND(dberr, result, db, end);
        }
        // rel_thumb_path
        if (entry.relThumbPathExists) {
            dberr = sqlite3_bind_text(stmt, ++bindPos,
                                      entry.relThumbPath.data(),
                                      entry.relThumbPath.size(), NULL);
            CHECK_BIND(dberr, result, db, end);
        } else {
            dberr = sqlite3_bind_null(stmt, ++bindPos);
            CHECK_BIND(dberr, result, db, end);
        }
        // opaque_metadata
        if (entry.opaqueMetadataExists) {
            dberr = sqlite3_bind_text(stmt, ++bindPos,
                                      entry.opaqueMetadata.data(),
                                      entry.opaqueMetadata.size(), NULL);
            CHECK_BIND(dberr, result, db, end);
        } else {
            dberr = sqlite3_bind_null(stmt, ++bindPos);
            CHECK_BIND(dberr, result, db, end);
        }
        ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::sharedFilesUpdateThumbDownloaded(u64 compId,
                                                   u64 feature,
                                                   bool thumbDownloaded)
{
    int result = 0;
    int dberr = 0;

    const char* SQL_UPDATE_SHARED_FILES_THUMB_DOWNLOADED =
        "UPDATE shared_files "
        "SET b_thumb_downloaded=? "
        "WHERE comp_id=? AND feature=?";

    MutexAutoLock lock(&mutex);
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(UPDATE_SHARED_FILES_THUMB_DOWNLOADED)];

    dberr = openDB();
    CHECK_ERR(dberr, result, end);

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, SQL_UPDATE_SHARED_FILES_THUMB_DOWNLOADED, -1, &stmt, NULL);
        DB_UTIL_CHECK_PREPARE(dberr, result, db, end);
    }
    {
        int bindPos = 0;
        // b_thumb_downloaded
        if (thumbDownloaded) {
            dberr = sqlite3_bind_int64(stmt, ++bindPos, SYNC_DOWN_SQLITE3_TRUE);
            CHECK_BIND(dberr, result, db, end);
        } else {
            dberr = sqlite3_bind_int64(stmt, ++bindPos, SYNC_DOWN_SQLITE3_FALSE);
            CHECK_BIND(dberr, result, db, end);
        }
        // comp_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, compId);
        CHECK_BIND(dberr, result, db, end);
        // feature
        dberr = sqlite3_bind_int64(stmt, ++bindPos, feature);
        CHECK_BIND(dberr, result, db, end);

        ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::sharedFilesEntryGetByCompId(u64 compId,
                                              u64 syncFeature,
                                              u64& revision_out,
                                              u64& datasetId_out,
                                              std::string& name_out,
                                              bool& thumbDownloaded_out,
                                              std::string& thumbRelPath_out)
{
    revision_out = 0;
    datasetId_out = 0;
    name_out.clear();
    thumbDownloaded_out = false;
    thumbRelPath_out.clear();
    int result = 0;
    int dberr = 0;

    const char* SQL_GET_SHARED_FILES_BY_COMP_ID_AND_FEAT =
        "SELECT "SHARED_FILES_COLUMNS" "
        "FROM shared_files "
        "WHERE comp_id=? AND feature=?";

    MutexAutoLock lock(&mutex);
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_SHARED_FILES_BY_COMP_ID_AND_FEAT)];

    dberr = openDB();
    CHECK_ERR(dberr, result, end);

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, SQL_GET_SHARED_FILES_BY_COMP_ID_AND_FEAT, -1, &stmt, NULL);
        DB_UTIL_CHECK_PREPARE(dberr, result, db, end);
    }
    {
        int bindPos = 0;
        // comp_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, compId);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);
        // feature
        dberr = sqlite3_bind_int64(stmt, ++bindPos, syncFeature);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);

        ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));
    }

    dberr = sqlite3_step(stmt);
    DB_UTIL_CHECK_STEP(dberr, result, db, end);
    DB_UTIL_CHECK_ROW_EXIST(dberr, result, db, end);

    {
        SharedFilesDbEntry entry;
        result = sharedFiles_readRow(stmt, /*OUT*/entry);
        if (result != 0) {
            LOG_ERROR("sharedFiles_readRow:%d", result);
            goto end;
        }
        revision_out = entry.revision;
        datasetId_out = entry.datasetId;
        name_out = entry.name;
        thumbDownloaded_out = entry.b_thumb_downloaded;
        thumbRelPath_out = entry.relThumbPath;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::sharedFilesEntryDelete(u64 compId,
                                         u64 datasetId)
{
    int result = 0;
    int dberr = 0;

    const char* SQL_DELETE_SHARED_FILES_ITEM_BY_COMP_ID_AND_DSET =
        "DELETE FROM shared_files "
        "WHERE comp_id=? AND dataset_id=?";

    MutexAutoLock lock(&mutex);
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_SHARED_FILES_ITEM_BY_COMP_ID_AND_DSET)];

    dberr = openDB();
    CHECK_ERR(dberr, result, end);

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db,
                SQL_DELETE_SHARED_FILES_ITEM_BY_COMP_ID_AND_DSET, -1, &stmt, NULL);
        DB_UTIL_CHECK_PREPARE(dberr, result, db, end);
    }
    {
        int bindPos = 0;
        // comp_id
        dberr = sqlite3_bind_int64(stmt, ++bindPos, compId);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);
        // feature
        dberr = sqlite3_bind_int64(stmt, ++bindPos, datasetId);
        DB_UTIL_CHECK_BIND(dberr, result, db, end);

        ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));
    }

    dberr = sqlite3_step(stmt);
    DB_UTIL_CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::addPicStreamItem(u64 compId, const std::string &name, u64 rev,
               u64 origCompId, u64 originDev, const std::string &identifier,
               const std::string &title, const std::string &albumName,
               u64 fileType, u64 fileSize, u64 takenTime, const std::string &lpath, /*OUT*/ u64 &jobid)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ADD_PICSTREAM_ITEM)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(ADD_PICSTREAM_ITEM), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    {
        int n = 1;
        DB_BIND_INT64(compId);
        DB_BIND_TEXT(name);
        DB_BIND_INT64(rev);
        DB_BIND_INT64(origCompId);
        DB_BIND_INT64(originDev);
        DB_BIND_TEXT(identifier);
        DB_BIND_TEXT(title);
        DB_BIND_TEXT(albumName);
        DB_BIND_INT64(fileType);
        DB_BIND_INT64(fileSize);
        DB_BIND_INT64(takenTime);
        DB_BIND_TEXT(lpath);
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

int SyncDownJobs::findPicStreamItems(u64 fileType, std::string albumName, std::queue<PicStreamItem> &items)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_ITEM_BY_FILE_TYPE_AND_ALBUM)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_ITEM_BY_FILE_TYPE_AND_ALBUM), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    {
        int n = 1;
        DB_BIND_INT64(fileType);
        DB_BIND_TEXT(albumName);
    }

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        PicStreamItem item;
        item.populateFrom(stmt);
        items.push(item);
    }
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::FindPicStreamItems(u64 fullCompId, std::queue<PicStreamItem> &items)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        err = findPicStreamItems(fullCompId, items);
        if (err == CCD_ERROR_NOT_FOUND) {
            goto end;
        }
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncDownJobs::FindPicStreamItem(u64 compId, PicStreamItem &item)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        err = findPicStreamItem(compId, item);
        if (err == CCD_ERROR_NOT_FOUND) {
            result = CCD_ERROR_NOT_FOUND;
            goto end;
        }
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncDownJobs::findPicStreamItems(u64 fullCompId, std::queue<PicStreamItem> &items)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_ITEM_BY_FULL_COMP_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_ITEM_BY_FULL_COMP_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    {
        int n = 1;
        DB_BIND_INT64(fullCompId);
    }

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        PicStreamItem item;
        item.populateFrom(stmt);
        items.push(item);
    }
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::findPicStreamItem(u64 compId, /*OUT*/ PicStreamItem &item)
{
    int result = VPL_OK;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_ITEM_BY_COMP_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_ITEM_BY_COMP_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :comp_id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)compId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        item.populateFrom(stmt);
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

int SyncDownJobs::findPicStreamItem(const std::string &name, /*OUT*/ PicStreamItem &item)
{
    int result = VPL_OK;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_ITEM_BY_NAME)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_ITEM_BY_NAME), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :name
    dberr = sqlite3_bind_text(stmt, 1, name.c_str(), name.size(), NULL);

    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_ROW) {
        item.populateFrom(stmt);
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

int SyncDownJobs::removePicStreamItems(u64 fullCompId)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_ITEM_BY_FULL_COMP_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_ITEM_BY_FULL_COMP_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :comp_id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)fullCompId);
    CHECK_BIND(dberr, result, db, end);

    // bind :orig_comp_id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)fullCompId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::removePicStreamItem(u64 CompId)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(REMOVE_ITEM_BY_COMP_ID)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(REMOVE_ITEM_BY_COMP_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :comp_id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)CompId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int SyncDownJobs::RemovePicStreamItem(u64 CompId)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        PicStreamItem item;
        err = findPicStreamItem(CompId, item);
        if (err == CCD_ERROR_NOT_FOUND) {
            goto end;
        }
        CHECK_ERR(err, result, end);

        err = removePicStreamItem(CompId);
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncDownJobs::RemovePicStreamItems(u64 fullCompId)
{
    int result = 0;

    MutexAutoLock lock(&mutex);

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        std::queue<PicStreamItem> items;
        err = findPicStreamItems(fullCompId, items);
        if (err == CCD_ERROR_NOT_FOUND) {
            goto end;
        }
        CHECK_ERR(err, result, end);

        err = removePicStreamItems(fullCompId);
        CHECK_ERR(err, result, end);
    }

 end:
    return result;
}

int SyncDownJobs::QueryPicStreamItems(const std::string& searchField,
                                    const std::string& sortOrder,
                                    std::queue<PicStreamPhotoSet> &output)
{
    int rv = CCD_OK;
    int err = 0;

    std::string SQL =
        "SELECT comp_id , orig_comp_id, origin_dev, identifier, title,  album_name, file_size, date_time, file_type, name , lpath "
        "FROM picstream ";

    sqlite3_stmt *stmt = NULL;
    
    if (searchField.size() > 0) {
        SQL.append("WHERE (" + searchField + ") ");
    }
    if (sortOrder.size() > 0) {
        SQL.append("ORDER BY " + sortOrder + " ");
    }

    std::ostringstream urlPrefix;
    rv = Query_GetUrlPrefix(urlPrefix, ccd::LOCAL_HTTP_SERVICE_REMOTE_FILES);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "Query_GetUrlPrefix", rv);
        return rv;
    }

    PicStreamPhotoSet set;

    MutexAutoLock lock(&mutex);
    LOG_DEBUG("SQL: %s", SQL.c_str());
    {

        err = openDB();
        CHECK_ERR(err, rv, end);

        err = sqlite3_prepare_v2(db, SQL.c_str(), -1, &stmt, NULL);
        CHECK_PREPARE(err, rv, db, end);

        while(rv == 0){
            int resIndex = 0;
            int sqlType;
            std::string albumName, identifier, title, name, lpath;
            u64 compId, origCompId, origDev, fileSize, dateTime, fileType;

            err = sqlite3_step(stmt);
            CHECK_STEP(err, rv, db, end);

            if (err == SQLITE_DONE) {
                rv = CCD_OK;
                break;
            }

            // compId
            sqlType = sqlite3_column_type(stmt, resIndex);
            if (sqlType == SQLITE_INTEGER) {
                compId = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // origCompId
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                origCompId = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // origDev
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                origDev = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // identifier
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                identifier = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // title
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                title = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // albumName
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                albumName = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // fileSize
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                fileSize = (u64) sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // dateTime
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                dateTime = (u64) sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // fileType
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                fileType = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }
            
            // name
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                name = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
                LOG_DEBUG("name: %s", name.c_str());
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // lpath
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                lpath = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            char compIdStr[128], origCompIdStr[128];
            sprintf(compIdStr,""FMTu64, compId);
            sprintf(origCompIdStr,""FMTu64, origCompId);

            set.albumName = albumName;
            set.compId = compIdStr;
            set.dateTime = dateTime;
            set.identifier = identifier;
            set.ori_deviceid = origDev;
            set.title = title;
            set.name = name;
            set.full_size = fileSize;
            set.lpath = lpath;
            set.fileType = fileType;
            set.origCompId = origCompIdStr;
            set.full_url = "";
            set.thumb_url = "";
            set.low_url = "";
            if(fileType == FileType_FullRes) {
                set.full_url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + compIdStr + "&type=0";
                set.url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + compIdStr + "&type=0";
            } else if(fileType == FileType_LowRes) {
                set.low_url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + origCompIdStr + "&type=1";
                set.url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + origCompIdStr + "&type=1";
            } else {
                set.thumb_url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + compIdStr + "&type=2";
                set.url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + compIdStr + "&type=2";
            }
            output.push(set);
        }//while(rv == 0) DB Loops
    }

end:
    if(stmt != NULL) {
        err = sqlite3_finalize(stmt);
        if (err != 0) {
            LOG_ERROR("sqlite3_finalize:%d", err);
        }
    }
    return rv;
}

int SyncDownJobs::getPicStreamItems(const std::string& searchField,
                                    const std::string& sortOrder,
                                    google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output)
{
    int rv = CCD_OK;
    int err = 0;
    output.Clear();

    std::string SQL =
        "SELECT comp_id , orig_comp_id, origin_dev, identifier, title,  album_name, file_size, date_time, file_type, name , lpath "
        "FROM picstream ";

    sqlite3_stmt *stmt = NULL;
    
    if (searchField.size() > 0) {
        SQL.append("WHERE (" + searchField + ") ");
    }
    if (sortOrder.size() > 0) {
        SQL.append("ORDER BY " + sortOrder + " ");
    }

    std::ostringstream urlPrefix;
    rv = Query_GetUrlPrefix(urlPrefix, ccd::LOCAL_HTTP_SERVICE_REMOTE_FILES);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "Query_GetUrlPrefix", rv);
        return rv;
    }

    PicStreamPhotoSet *curSet;
    PicStreamPhotoSet set;
    std::map<std::string, PicStreamPhotoSet> PhotoSets;
    std::map<std::string, PicStreamPhotoSet>::iterator itrSet;
    PhotoSets.clear();

    MutexAutoLock lock(&mutex);
    LOG_DEBUG("SQL: %s", SQL.c_str());
    {

        err = openDB();
        CHECK_ERR(err, rv, end);

        err = sqlite3_prepare_v2(db, SQL.c_str(), -1, &stmt, NULL);
        CHECK_PREPARE(err, rv, db, end);

        while(rv == 0){
            int resIndex = 0;
            int sqlType;
            std::string albumName, identifier, title, name, lpath;
            u64 compId, origCompId, origDev, fileSize, dateTime, fileType;

            err = sqlite3_step(stmt);
            CHECK_STEP(err, rv, db, end);

            if (err == SQLITE_DONE) {
                rv = CCD_OK;
                break;
            }

            // compId
            sqlType = sqlite3_column_type(stmt, resIndex);
            if (sqlType == SQLITE_INTEGER) {
                compId = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // origCompId
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                origCompId = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // origDev
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                origDev = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // identifier
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                identifier = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // title
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                title = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // albumName
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                albumName = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // fileSize
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                fileSize = (u64) sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // dateTime
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                dateTime = (u64) sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // fileType
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                fileType = sqlite3_column_int64(stmt, resIndex);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }
            
            // name
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                name = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
                LOG_DEBUG("name: %s", name.c_str());
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            // lpath
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_TEXT) {
                lpath = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            itrSet = PhotoSets.find(title);
            if (itrSet == PhotoSets.end() ){
                set.full_url = "";
                set.low_url = "";
                set.thumb_url = "";
                PhotoSets[title] = set;
                curSet = &(PhotoSets[title]);
                LOG_DEBUG("New Items: %s, filetype:"FMTu64, title.c_str(), fileType);
            } else {
                curSet = &(itrSet->second);
                LOG_DEBUG("Items: %s, filetype:"FMTu64, title.c_str(), fileType);
            }

            char compIdStr[128];
            sprintf(compIdStr,""FMTu64, compId);

            if(fileType == FileType_FullRes) {
                curSet->albumName = albumName;
                curSet->compId = compIdStr;
                curSet->dateTime = dateTime;
                curSet->full_size = fileSize;
                curSet->full_url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + compIdStr + "&type=0";
                curSet->identifier = identifier;
                curSet->ori_deviceid = origDev;
                curSet->title = title;
                curSet->name = name;
            } else if(fileType == FileType_LowRes) {
                sprintf(compIdStr,""FMTu64, origCompId);
                curSet->low_url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + compIdStr + "&type=1";
            } else {
                curSet->thumb_url = urlPrefix.str() +  "/picstream/file/" + title + "?compId=" + compIdStr + "&type=2";
            }

        }//while(rv == 0) DB Loops
    }

    //fill protobuf
    {
        for (itrSet = PhotoSets.begin(); itrSet != PhotoSets.end(); ++itrSet) {

            curSet = &(itrSet->second);
            LOG_INFO("add item: %s %s", curSet->title.c_str(), curSet->full_url.c_str());

            if(curSet->full_url.length() == 0){ //Items must have full resolution photo
                LOG_INFO("skip due to no full photo information. item: %s", curSet->title.c_str());
                continue;
            }

            ccd::PicStreamQueryObject* obj = output.Add();
            obj->set_full_res_url(curSet->full_url);
            obj->set_low_res_url(curSet->low_url);
            obj->set_thumbnail_url(curSet->thumb_url);
            obj->mutable_pcdo()->set_comp_id(curSet->compId);
            obj->mutable_pcdo()->mutable_picstream_item()->set_date_time(curSet->dateTime);
            obj->mutable_pcdo()->mutable_picstream_item()->set_file_size(curSet->full_size);
            obj->mutable_pcdo()->mutable_picstream_item()->set_album_name(curSet->albumName);
            obj->mutable_pcdo()->mutable_picstream_item()->set_title(curSet->title);
            obj->mutable_pcdo()->mutable_picstream_item()->set_identifier(curSet->identifier);
            obj->mutable_pcdo()->mutable_picstream_item()->set_ori_deviceid(curSet->ori_deviceid);
        }
    }

end:
    if(stmt != NULL) {
        err = sqlite3_finalize(stmt);
        if (err != 0) {
            LOG_ERROR("sqlite3_finalize:%d", err);
        }
    }
    return rv;
}

int SyncDownJobs::getPicStreamAlbums(const std::string& searchField,
                                     const std::string& sortOrder,
                                     google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output)
{
    int rv = CCD_OK;
    int err = 0;
    output.Clear();

    std::string SQL =
        "SELECT album_name, COUNT(comp_id) as itemCount, SUM(file_size) as itemTotalSize "
        "FROM picstream ";

    SQL.append(" WHERE ( file_type=0 ");

    if (searchField.size() > 0) {
        SQL.append(" AND " + searchField + " ");
    }
    SQL.append(")");

    SQL.append(" GROUP BY album_name ");
    if (sortOrder.size() > 0) {
        SQL.append("ORDER BY " + sortOrder + " ");
    }

    sqlite3_stmt *stmt = NULL;

    MutexAutoLock lock(&mutex);
    LOG_INFO("sql:%s", SQL.c_str());
    {

        err = openDB();
        CHECK_ERR(err, rv, end);

        err = sqlite3_prepare_v2(db, SQL.c_str(), -1, &stmt, NULL);
        CHECK_PREPARE(err, rv, db, end);

        while(rv == 0){
            int resIndex = 0;
            int sqlType;
            std::string albumName;
            u64 itemCount, itemTotalSize;

            err = sqlite3_step(stmt);
            CHECK_STEP(err, rv, db, end);

            if (err == SQLITE_DONE) {
                rv = CCD_OK;
                goto end;
            }

            ccd::PicStreamQueryObject* obj = output.Add();

            // album_name
            sqlType = sqlite3_column_type(stmt, resIndex);
            if (sqlType == SQLITE_TEXT) {
                albumName = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
                obj->mutable_picstream_album()->set_album_name(albumName);
                LOG_DEBUG("albumName: %s", albumName.c_str());
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }
            // itemCount
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                itemCount = sqlite3_column_int64(stmt, resIndex);
                obj->mutable_picstream_album()->set_item_count((u32) itemCount);
                LOG_DEBUG("itemCount:"FMTu32, (u32) itemCount);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }
            // itemTotalSize
            sqlType = sqlite3_column_type(stmt, ++resIndex);
            if (sqlType == SQLITE_INTEGER) {
                itemTotalSize = sqlite3_column_int64(stmt, resIndex);
                obj->mutable_picstream_album()->set_item_total_size((u32) itemTotalSize);
                LOG_DEBUG("itemTotalSize: "FMTu32, (u32) itemTotalSize);
            }else{
                LOG_ERROR("Bad column type index:%d", resIndex);
                rv = VPL_ERR_INVALID;
                goto end;
            }
        }
    }

end:
    return rv;
}

int SyncDownJobs::querySharedFilesItems(ccd::SyncFeature_t syncFeature,
                                        const std::string& searchField,
                                        const std::string& sortOrder,
                                        google::protobuf::RepeatedPtrField<ccd::SharedFilesQueryObject>& output)
{
    int rv = CCD_OK;
    int err = 0;
    output.Clear();
    std::string urlPrefix;

    std::string SQL_GET_ITEMS_BY_SEARCH_FIELD_AND_SORT_ORDER =
        "SELECT "SHARED_FILES_COLUMNS
        " FROM shared_files";

    {
        std::ostringstream urlPrefixStr;
        rv = Query_GetUrlPrefix(urlPrefixStr, ccd::LOCAL_HTTP_SERVICE_REMOTE_FILES);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "Query_GetUrlPrefix", rv);
            return rv;
        }
        urlPrefix = urlPrefixStr.str();
    }

    std::string featureStr = SSTR(static_cast<int>(syncFeature));
    SQL_GET_ITEMS_BY_SEARCH_FIELD_AND_SORT_ORDER.append(" WHERE feature="+featureStr);

    if (searchField.size() > 0) {
        SQL_GET_ITEMS_BY_SEARCH_FIELD_AND_SORT_ORDER.append(" AND (" + searchField + ")");
    }
    if (sortOrder.size() > 0) {
        SQL_GET_ITEMS_BY_SEARCH_FIELD_AND_SORT_ORDER.append(" ORDER BY " + sortOrder);
    }

    sqlite3_stmt *stmt = NULL;
    MutexAutoLock lock(&mutex);
    LOG_INFO("sql:%s", SQL_GET_ITEMS_BY_SEARCH_FIELD_AND_SORT_ORDER.c_str());

    err = openDB();
    CHECK_ERR(err, rv, end);

    err = sqlite3_prepare_v2(db, SQL_GET_ITEMS_BY_SEARCH_FIELD_AND_SORT_ORDER.c_str(), -1, &stmt, NULL);
    DB_UTIL_CHECK_PREPARE(err, rv, db, end);

    while(rv == 0){
        SyncDownJobs::SharedFilesDbEntry sharedFilesDbEntry;

        err = sqlite3_step(stmt);
        DB_UTIL_CHECK_STEP(err, rv, db, end);

        if (err == SQLITE_DONE) {
            rv = CCD_OK;
            goto end;
        }

        rv = sharedFiles_readRow(stmt, /*OUT*/ sharedFilesDbEntry);
        if (rv != 0) {
            LOG_ERROR("sharedFiles_readRow:%d", rv);
            goto end;
        }

        {
            ccd::SharedFilesQueryObject* sharedFilesQueryObj = output.Add();
            if (sharedFilesQueryObj == NULL) {
                LOG_ERROR("sharedFilesQueryObj NULL, OutOfMemory?");
                rv = CCD_ERROR_INTERNAL;
                goto end;
            }

            sharedFilesQueryObj->set_comp_id(sharedFilesDbEntry.compId);
            sharedFilesQueryObj->set_revision(sharedFilesDbEntry.revision);
            sharedFilesQueryObj->set_name(sharedFilesDbEntry.name);

            {   // Generate urls:
                // http://wiki.ctbg.acer.com/wiki/index.php/Photo_Sharing_Design#New_HTTP_API_to_fetch_shared_items
                std::string compIdStr = SSTR(sharedFilesDbEntry.compId);
                std::string featureStr = SSTR(sharedFilesDbEntry.feature);
                std::string commonUrl = urlPrefix +  "/share/file" +
                                        "?feature=" + featureStr +
                                        "&compId=" + compIdStr;
                std::string contentUrl = commonUrl + "&type=0";    // type 0 represents "shared resolution" content
                sharedFilesQueryObj->set_content_url(contentUrl);
                if (sharedFilesDbEntry.b_thumb_downloaded) {
                    std::string thumbUrl =  commonUrl + "&type=1"; // type 1 represents "thumbnail resolution" content
                    sharedFilesQueryObj->set_preview_url(thumbUrl);
                }
            }

            if (sharedFilesDbEntry.opaqueMetadataExists) {
                sharedFilesQueryObj->set_opaque_metadata(sharedFilesDbEntry.opaqueMetadata);
            }
            if (!sharedFilesDbEntry.recipientList.empty())
            {   // Tokenize e-mails by comma delimiter into repeated protobuf field.
                std::stringstream ss(sharedFilesDbEntry.recipientList);
                std::string email;
                char delim = ','; // E-mails in this string are separated by commas.
                while (std::getline(ss, email, delim)) {
                    sharedFilesQueryObj->add_recipient_list(email);
                }
            }
        }
    }
 end:
    if(stmt != NULL) {
        err = sqlite3_finalize(stmt);
        if (err != 0) {
            LOG_ERROR("sqlite3_finalize:%d", err);
        }
    }
    return rv;
}

int SyncDownJobs::QueryPicStreamItems(ccd::PicStream_DBFilterType_t filter_type,
                                const std::string& searchField,
                                const std::string& sortOrder,
                                google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output)
{
    output.Clear();

    //note: lock mutex and openDB are done inside functions below.
    switch(filter_type) {
    case ccd::PICSTREAM_QUERY_ALBUM:
            return getPicStreamAlbums(searchField, sortOrder, output);
    case ccd::PICSTREAM_QUERY_ITEM:
            return getPicStreamItems(searchField, sortOrder, output);
    default:
            return VPL_ERR_INVALID;
    }

    return VPL_ERR_INVALID;
}

int SyncDownJobs::MapFullLowResInDatabase()
{
    int rv = CCD_OK;
    int err = 0;

    std::string SQL = "SELECT name FROM Picstream WHERE file_type=1 AND orig_comp_id=0";
    std::queue<std::string> lowResKey; //the name (primary key) of low-res data
    std::string name;

    sqlite3_stmt *stmt = NULL;
    int sqlType;
    u64 compId;

    MutexAutoLock lock(&mutex);

    { //get all low-res rows whose orig_comp_id=0 from database
        err = openDB();
        CHECK_ERR(err, rv, end);

        err = sqlite3_prepare_v2(db, SQL.c_str(), -1, &stmt, NULL);
        CHECK_PREPARE(err, rv, db, end);

        while(rv == 0){

            err = sqlite3_step(stmt);
            CHECK_STEP(err, rv, db, end);
            
            if (err == SQLITE_DONE) {
                rv = 0;
                break;
            }

            // name
            sqlType = sqlite3_column_type(stmt, 0);
            if (sqlType == SQLITE_TEXT) {
                name = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, 0));
                LOG_DEBUG("name: %s", name.c_str());
                lowResKey.push(name);
            }else{
                LOG_ERROR("Bad column type index:%d", 0);
                rv = VPL_ERR_INVALID;
                goto end;
            }

        }
    }

    while(lowResKey.size() > 0) {

        name = lowResKey.front();
        // change from "lowres/2014_01/20140127_051545.jpg" to "photos/2014_01/20140127_051545.jpg"
        name.replace(0, strlen("lowres"), "photos");
        SQL = "SELECT comp_id FROM picstream WHERE name=:name";

        err = sqlite3_prepare_v2(db, SQL.c_str(), -1, &stmt, NULL);
        CHECK_PREPARE(err, rv, db, end);

        err = sqlite3_bind_text(stmt, 1, name.data(), name.size(), NULL);
        CHECK_BIND(err, rv, db, end);

        do {
            err = sqlite3_step(stmt);
            CHECK_STEP(err, rv, db, end);

            if (err == SQLITE_DONE) { // no data!
                rv = 0;
                LOG_ERROR("Mapping Failed. Can NOT find %s", name.c_str());
                break;
            }

            // compId
            sqlType = sqlite3_column_type(stmt, 0);
            if (sqlType == SQLITE_INTEGER) {
                compId = sqlite3_column_int64(stmt, 0);
                LOG_DEBUG("%s, Full compId:"FMTu32, name.c_str(), (u32) compId);
            }else{
                LOG_ERROR("Bad column type index:%d", 0);
                rv = VPL_ERR_INVALID;
                goto end;
            }

            name = lowResKey.front();
            SQL = "Update picstream SET orig_comp_id=:compId WHERE name=:name";

            err = sqlite3_prepare_v2(db, SQL.c_str(), -1, &stmt, NULL);
            CHECK_PREPARE(err, rv, db, end);

            // bind :id
            err = sqlite3_bind_int64(stmt, 1, (s64)compId);
            CHECK_BIND(err, rv, db, end);

            // bind :name
            err = sqlite3_bind_text(stmt, 2, name.data(), name.size(), NULL);
            CHECK_BIND(err, rv, db, end);

            err = sqlite3_step(stmt);
            CHECK_STEP(err, rv, db, end);

            if (err == SQLITE_DONE) {
                rv = 0;
                break;
            }

        } while(0);

        lowResKey.pop();
    }

end:
    if(stmt != NULL) {
        err = sqlite3_finalize(stmt);
        if (err != 0) {
            LOG_ERROR("sqlite3_finalize:%d", err);
        }
    }
    return rv;
}

int SyncDownJobs::UpdateLocalPath(const std::string& name, const std::string& localPath)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = openDB();
    CHECK_ERR(err, result, end);

    err = updateLocalPath(name, localPath);
    CHECK_ERR(err, result, end);

end:
    return result;
}

int SyncDownJobs::updateLocalPath(const std::string& name, const std::string& localPath)
{
    int rv = 0;
    int err = 0;
    sqlite3_stmt *stmt = NULL;

    std::string SQL = "Update picstream SET lpath=:lpath WHERE name=:name";

    LOG_DEBUG("name:%s localpath:%s", name.c_str(), localPath.c_str());
    err = sqlite3_prepare_v2(db, SQL.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(err, rv, db, end);

    // bind :locapPath
    err = sqlite3_bind_text(stmt, 1, localPath.data(), localPath.size(), NULL);
    CHECK_BIND(err, rv, db, end);

    // bind :name
    err = sqlite3_bind_text(stmt, 2, name.data(), name.size(), NULL);
    CHECK_BIND(err, rv, db, end);

    err = sqlite3_step(stmt);
    CHECK_STEP(err, rv, db, end);

end:
    if(stmt != NULL) {
        err = sqlite3_finalize(stmt);
        if (err != 0) {
            LOG_ERROR("sqlite3_finalize:%d", err);
        }
    }
    return rv;
}

//----------------------------------------------------------------------

#ifdef CLOUDNODE

// impl for CloudNode
SyncDownDstStor::SyncDownDstStor()
    : dstDatasetId(0), dstDataset(NULL)
{
}

// impl for CloudNode
SyncDownDstStor::~SyncDownDstStor()
{
    if (dstDataset) {
        dstDataset->release();
        dstDataset = NULL;
    }
}

// impl for CloudNode
int SyncDownDstStor::init(u64 userId)
{
    int err = 0;

    err = getVirtDriveDatasetId(userId);
    if (err) {
        LOG_ERROR("Could not find local Virt Drive dataset: %d", err);
        return err;
    }

    MutexAutoLock lock(LocalServers_GetMutex());
    vss_server* sn = LocalServers_getStorageNode();
    if (sn == NULL) {
        LOG_ERROR("StorageNode not found");
        return CCD_ERROR_NOT_FOUND;
    }
    err = sn->getDataset(userId, dstDatasetId, /*out*/ dstDataset);
    if (err) {
        LOG_ERROR("Could not get dataset: %d", err);
        return err;
    }

    return 0;
}

// impl for CloudNode
bool SyncDownDstStor::fileExists(const std::string &path)
{
    VPLFS_stat_t _stat; // dummy
    return !dstDataset->stat_component(path, _stat);
}

// impl for CloudNode
int SyncDownDstStor::createMissingAncestorDirs(const std::string &path)
{
    int result = 0;
    int err = 0;

    bool need_to_commit = false;
    size_t pos = 0;
    while ((pos = path.find('/', pos)) != path.npos) {
        std::string dirCompPath = path.substr(0, pos);
        VPLFS_stat_t stat;
        err = dstDataset->stat_component(dirCompPath, stat);
        if (!err) {
            if (stat.type == VPLFS_TYPE_FILE) {
                LOG_ERROR("Cannot create directory at %s because file is there", dirCompPath.c_str());
                result = VSSI_FEXISTS;
                break;
            }
            else {  // stat.type = VPLFS_TYPE_DIR
                    // nothing to do
            }
        }
        else if (err != VSSI_NOTFOUND) {
            // unexpected error
            LOG_ERROR("Could not stat %s: %d", dirCompPath.c_str(), err);
            result = err;
            break;
        }
        else {  // err == VSSI_NOTFOUND
            err = dstDataset->make_directory_iw(dirCompPath, 0);
            if (err && err != VSSI_ISDIR) {
                LOG_ERROR("Could not create directory at %s: %d", dirCompPath.c_str(), err);
                result = err;
                break;
            }
            need_to_commit = true;
        }
        pos++;
    }
    if (need_to_commit && !result) {
        err = dstDataset->commit_iw();
        if (err) {
            LOG_ERROR("Failed to commit: %d", err);
            result = err;
        }
    }

    return result;
}

// impl for CloudNode
int SyncDownDstStor::copyFile(const std::string &srcPath, const std::string &dstPath)
{
    int result = 0;
    int err = 0;

    vss_file *file = NULL;
    do {
        VPLFile_handle_t hin = VPLFILE_INVALID_HANDLE;
        hin = VPLFile_Open(srcPath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(hin)) {
            LOG_ERROR("Failed to open %s for read", srcPath.c_str());
            result = (int)hin;
            break;
        }
        ON_BLOCK_EXIT(VPLFile_Close, hin);

        err = dstDataset->open_file(dstPath, 0, VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE, 0, file);
        if (err) {
            LOG_ERROR("Failed to open file %s: %d", dstPath.c_str(), err);
            result = err;
            break;
        }

        char *buffer = (char*)malloc(SYNCDOWN_COPY_FILE_BUFFER_SIZE);
        if (!buffer) {
            result = CCD_ERROR_NOMEM;
            break;
        }
        ON_BLOCK_EXIT(free, buffer);

        ssize_t bytes_read = 0;
        u64 offset = 0;
        while ((bytes_read = VPLFile_Read(hin, buffer, SYNCDOWN_COPY_FILE_BUFFER_SIZE)) > 0) {
            ssize_t bytes_remaining = bytes_read;
            while (bytes_remaining > 0) {
                u32 length = bytes_remaining;
                err = dstDataset->write_file(file, NULL, /*origin*/0, offset, length, buffer);
                if (err) {
                    LOG_ERROR("Failed to write to %s: %d", dstPath.c_str(), err);
                    result = err;
                    goto write_error;
                }
                bytes_remaining -= length;
                offset += length;
            }
        }
        if (bytes_read < 0) {
            LOG_ERROR("Failed to read from %s: %d", srcPath.c_str(), (int)bytes_read);
            if (!result) result = bytes_read;
        }
    write_error:
        ;
    } while (0);

    if (file) {
        err = dstDataset->close_file(file, NULL, 0);
        file = NULL;  // drop handle regardless of outcome
        if (err) {
            LOG_ERROR("Failed to close file: %d", err);
            if (!result) result = err;
        }
    }

    err = dstDataset->commit_iw();
    if (err) {
        LOG_ERROR("Failed to commit: %d", err);
        if (!result) result = err;
    }

    return result;
}

int SyncDownDstStor::getSiblingComponentCount(const std::string path, int& count_out)
{
    int result = 0;
    int err = 0;

    count_out = 0;
    err = dstDataset->get_sibling_components_count(path, count_out);
    if (err) {
        LOG_ERROR("Failed to get component count: %d", err);
        if (!result) result = err;
    }

    return result;
}

int SyncDownDstStor::getVirtDriveDatasetId(u64 userId)
{
    int result = 0;

    if (!userId) {
        return CCD_ERROR_USER_NOT_FOUND;
    }

    CacheAutoLock autoLock;
    int err = autoLock.LockForRead();
    if (err < 0) {
        LOG_ERROR("Failed to obtain lock");
        return err;
    }

    CachePlayer *player = cache_getUserByUserId(userId);
    if (!player) {
        LOG_ERROR("No user found");
        return CCD_ERROR_USER_NOT_FOUND;
    }

    u64 localDeviceId = VirtualDevice_GetDeviceId();
    if (!localDeviceId) {
        LOG_ERROR("Could not determine local device ID");
        return CCD_ERROR_NOT_FOUND;
    }

    bool found = false;
    for (int i = 0; i < player->_cachedData.details().datasets_size(); i++) {
        const vplex::vsDirectory::DatasetDetail &dd = player->_cachedData.details().datasets(i).details();
        if (dd.datasetname() == "Virt Drive" && dd.clusterid() == localDeviceId) {
            dstDatasetId = dd.datasetid();
            found = true;
            break;
        }
    }
    if (!found) {
        LOG_ERROR("Virt Drive dataset not found on local device");
        result = CCD_ERROR_NOT_FOUND;
    }

    return result;
}

#else

// impl for non-CloudNode
SyncDownDstStor::SyncDownDstStor()
{
    // nothing to do
}

// impl for non-CloudNode
SyncDownDstStor::~SyncDownDstStor()
{
    // nothing to do
}

// impl for non-CloudNode
int SyncDownDstStor::init(u64 userId)
{
    // nothing to do
    return 0;
}

// impl for non-CloudNode
bool SyncDownDstStor::fileExists(const std::string &path)
{
    return VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK;
}

// impl for non-CloudNode
int SyncDownDstStor::createMissingAncestorDirs(const std::string &path)
{
    return Util_CreatePath(path.c_str(), VPL_FALSE);
}

// impl for non-CloudNode
int SyncDownDstStor::copyFile(const std::string &srcPath, const std::string &dstPath)
{
    int result = 0;

    do {
        VPLFile_handle_t hin = VPLFILE_INVALID_HANDLE;
        hin = VPLFile_Open(srcPath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(hin)) {
            LOG_ERROR("Failed to open %s for read: %d", srcPath.c_str(), (int)hin);
            result = (int)hin;
            break;
        }
        ON_BLOCK_EXIT(VPLFile_Close, hin);

        VPLFile_handle_t hout = VPLFILE_INVALID_HANDLE;
        hout = VPLFile_Open(dstPath.c_str(),
                            VPLFILE_OPENFLAG_WRITEONLY|VPLFILE_OPENFLAG_CREATE|VPLFILE_OPENFLAG_TRUNCATE,
                            VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR);
        if (!VPLFile_IsValidHandle(hout)) {
            LOG_ERROR("Failed to open %s for write: %d", dstPath.c_str(), (int)hout);
            result = (int)hout;
            break;
        }
        ON_BLOCK_EXIT(VPLFile_Close, hout);
    
        char *buffer = (char*)malloc(SYNCDOWN_COPY_FILE_BUFFER_SIZE);
        if (!buffer) {
            result = CCD_ERROR_NOMEM;
            break;
        }
        ON_BLOCK_EXIT(free, buffer);

        ssize_t bytes_read = 0;
        while ((bytes_read = VPLFile_Read(hin, buffer, SYNCDOWN_COPY_FILE_BUFFER_SIZE)) > 0) {
            ssize_t bytes_remaining = bytes_read;
            while (bytes_remaining > 0) {
                ssize_t bytes_written = VPLFile_Write(hout, buffer + bytes_read - bytes_remaining, bytes_remaining);
                if (bytes_written < 0) {
                    LOG_ERROR("Failed to write to %s: %d", dstPath.c_str(), (int)bytes_written);
                    if (!result) result = bytes_written;
                    goto write_error;
                }
                bytes_remaining -= bytes_written;
            }
        }
        if (bytes_read < 0) {
            LOG_ERROR("Failed to read from %s: %d", srcPath.c_str(), result);
            if (!result) result = bytes_read;
        }
    write_error:
        ;
    } while (0);

    return result;
}

#endif // CLOUDNODE




//----------------------------------------------------------------------

SyncDown::SyncDown(const std::string &workdir)
:   workdir(workdir),
    userId(0),
    cutoff_ts(0),
    jobs(NULL),
    thread_spawned(false),
    thread_running(false),
    exit_thread(false),
    overrideAnsCheck(false),
    isDoingWork(false),
    globalDeleteEnabled(false)
{
    int err = 0;

    tmpdir = workdir + "tmp/";

    err = VPLMutex_Init(&mutex);
    if (err) {
        LOG_ERROR("Failed to initialize mutex: %d", err);
    }
    err = VPLCond_Init(&cond);
    if (err) {
        LOG_ERROR("Failed to initialize condvar: %d", err);
    }

    currentSyncStatus.clear();
    currentSyncStatus.insert(std::pair<int, int>(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES,
                                                 ccd::CCD_SYNC_STATE_IN_SYNC));
    currentSyncStatus.insert(std::pair<int, int>(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES,
                                                 ccd::CCD_SYNC_STATE_IN_SYNC));
    currentSyncStatus.insert(std::pair<int, int>(ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL,
                                                 ccd::CCD_SYNC_STATE_IN_SYNC));
    currentSyncStatus.insert(std::pair<int, int>(ccd::SYNC_FEATURE_PICSTREAM_DELETION,
                                                 ccd::CCD_SYNC_STATE_IN_SYNC));
    currentSyncStatus.insert(std::pair<int, int>(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                                 ccd::CCD_SYNC_STATE_IN_SYNC));
    currentSyncStatus.insert(std::pair<int, int>(ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                                 ccd::CCD_SYNC_STATE_IN_SYNC));
}

SyncDown::~SyncDown()
{
    int err = 0;

    if (jobs) {
        delete jobs;
        jobs = NULL;
        LOG_INFO("SyncDownJobs obj destroyed");
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

int SyncDown::spawnDispatcherThread()
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
        VPLThread_AttrSetStackSize(&attr, SYNCDOWN_THREAD_STACK_SIZE);
        VPLDetachableThreadHandle_t thread;
        err = VPLDetachableThread_Create(&thread, dispatcherThreadMain, (void*)this, &attr, NULL);
        if (err != VPL_OK) {
            LOG_ERROR("Failed to spawn SyncDown dispatcher thread: %d", err);
            return err;
        }
        thread_spawned = true;
    }

    return result;
}

int SyncDown::signalDispatcherThreadToStop()
{
    int result = 0;

    ASSERT(VPLMutex_LockedSelf(&mutex));

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

VPLTHREAD_FN_DECL SyncDown::dispatcherThreadMain(void *arg)
{
    SyncDown *sd = (SyncDown*)arg;
    if (sd) {
        sd->dispatcherThreadMain();

        delete sd;
        LOG_DEBUG("SyncDown obj destroyed");
    }
    return VPLTHREAD_RETURN_VALUE;
}

void SyncDown::dispatcherThreadMain()
{
    thread_running = true;

    LOG_DEBUG("dispatcher thread started");

    int err = 0;
    syncDownAdded = false;
    syncDownFileType = 0;

    /// main loop for the syncback thread
    while (1) {
        if (exit_thread) break;

        VPL_BOOL ansConnected;
        u64 datasetid = 0;  // assume there is no dataset to query for changes and prove otherwise

        {  // MutexAutoLock scope
            MutexAutoLock lock(&mutex);

            while (dataset_changes_queue.empty() && !exit_thread && !syncDownAdded)
            {
                isDoingWork = false;
                overrideAnsCheck = false;
                int datasetIdErr;
                do {
                    err = VPLCond_TimedWait(&cond,
                                            &mutex,
                                            VPLTime_FromSec(__ccdConfig.syncDownRetryInterval));
                    if (err != 0 && err != VPL_ERR_TIMEOUT) {
                        // unexpected condvar error - give up
                        LOG_ERROR("Unexpected error from VPLCond_TimedWait: %d", err);
                        exit_thread = true;
                        break;
                    }
                    lock.UnlockNow();
                    u64 datasetId_unused;
                    datasetIdErr = VCS_getDatasetID(userId, "PicStream", datasetId_unused);
                    lock.Relock(&mutex);
                } while (datasetIdErr==CCD_ERROR_DATASET_SUSPENDED && !exit_thread);

                if (err == VPL_ERR_TIMEOUT) {
                    LOG_DEBUG("condvar timeout");
                    isDoingWork = true;
                    break;  // break out of while loop and try any pending work
                }
            }

            ansConnected = ANSConn_IsActive();

            if (exit_thread) break;

            if (!dataset_changes_queue.empty()) {
                datasetid = dataset_changes_queue.front();
                dataset_changes_queue.pop();
                dataset_changes_set.erase(datasetid);
            }

            if(syncDownAdded){
                addSyncDownJobs(datasetid, syncDownFileType);
                syncDownAdded = false;
            }
        }  // MutexAutoLock scope

        if (datasetid && (overrideAnsCheck || ansConnected))
        {  // there's a dataset with changes to be queried
            u64 version;
            err = jobs->GetDatasetLastCheckedVersion(datasetid, version);
            if (err == CCD_ERROR_NOT_FOUND) {
                version = 0;
            }
            else if (err) {
                // unexpected db error - give up
                LOG_ERROR("Failed to get last-checked version for "FMTu64": %d", 
                          datasetid, err);
                exit_thread = true;
                break;
            }

            if (exit_thread) break;
            getChanges(datasetid, version);
        }  // if (datasetid)

        if (exit_thread) break;
        applyChanges();
        applyDeletions(userId, datasetid);

    }  // while

    LOG_DEBUG("dispatcher thread exiting");

    {
        // Need to acquire lock to prevent signaling context
        // from having its state ripped out by this thread.  (SyncDown::Stop)
        MutexAutoLock lock(&mutex);
        thread_running = false;
        thread_spawned = false;
    }
}

int SyncDown::addSyncDownJobs(u64 datasetid, int fileType)
{

    std::string downloadDir;
    u32 unusedMaxFiles, unusedMaxBytes;
    u32 unusedPreserveFreeDiskPercentage;
    u64 unusedPreserveFreeDiskSizeBytes;
    PicStreamDownloadDirEnum downloadDirType;
    u64 syncfeature = 0;
    std::string localpath;
    std::string prefix;
    int err;

    SyncDownDstStor sdds;
    err = sdds.init(userId);
    if (err) {
        LOG_ERROR("SDDS init failed: %d", err);
        return err;
    }

    {
        switch(fileType) {
            case FileType_FullRes:
                downloadDirType = PicStreamDownloadDirEnum_FullRes;
                syncfeature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES;
                prefix = "photos/";
            break;
            case FileType_LowRes:
                downloadDirType = PicStreamDownloadDirEnum_LowRes;
                syncfeature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES;
                prefix = "lowres/";
            break;
            case FileType_Thumbnail:
                downloadDirType = PicStreamDownloadDirEnum_Thumbnail;
                syncfeature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL;
                prefix = "thumbnail/";
            break;
        default:
            LOG_ERROR("Unexpected file type!");
            return 0;
            break;
        }

        err = getPicStreamDownloadDir(downloadDirType,
                                          downloadDir,
                                          unusedMaxFiles,
                                          unusedMaxBytes,
                                          unusedPreserveFreeDiskPercentage,
                                          unusedPreserveFreeDiskSizeBytes);
        if (err == CCD_ERROR_NOT_FOUND) {
            LOG_WARN("no download dir set for type:%d", fileType);
            return err;
        } else if (err) {
            LOG_ERROR("Failed to get PicStream download dir for fileType:%d err: %d", fileType, err);
            return err;
        }

        std::queue< PicStreamPhotoSet > items;
        std::string criteria = SSTR("file_type=" << fileType);
        this->jobs->QueryPicStreamItems(criteria, "", items);

        while(items.size() > 0) {
            PicStreamPhotoSet cur = items.front();
            items.pop();

            Util_appendToAbsPath(downloadDir,
                                 cur.name.substr(prefix.length()),
                                 localpath);

            if (cur.lpath.compare(localpath) != 0) {
                jobs->UpdateLocalPath(cur.name, localpath);
            }

            if (!localpath.empty() && sdds.fileExists(localpath)) {
                // A file exists at the corresponding local path.
                // Most likely a duplicate, so skip file.
                LOG_DEBUG("skip %s - local file exists", cur.name.c_str());
                continue;
            }

            // For full resolution photo subscription, if photos were uploaded from local device, try 
            // to copy them before downloading from infra.
            if (cur.identifier.length() > 0){
                err = sdds.createMissingAncestorDirs(localpath);
                if (err) {
                    LOG_WARN("Failed to create ancestor dirs for %s: %d", localpath.c_str(), err);
                }

                err = sdds.copyFile(cur.identifier, localpath);
                if (err) {
                    LOG_ERROR("Failed to copy file from %s to %s: %d. Try to download from infra.", cur.identifier.c_str(), localpath.c_str(), err);
                } else {
                    LOG_DEBUG("Copy file from %s to %s.", cur.identifier.c_str(), localpath.c_str());
                    continue;
                }
            }

            u64 _jobid;
            u64 compid =  VPLConv_strToU64(cur.compId.c_str(), NULL, 10);
            err = jobs->AddJob(datasetid, cur.name/*comppath*/, compid, localpath, syncfeature, 1 /*revision*/ ,  _jobid);
            if (err != 0) {
                LOG_ERROR("Add SyncDown Job Error! name:%s localpath:%s syncFeature:"FMTu64" error:%d",
                          cur.name.c_str(), localpath.c_str(), syncfeature, err);
            }
        }//loop photo items
    }

    return 0;
}

int SyncDown::handleSwmAndSbmDelete(u64 compId,
                                    u64 datasetId,
                                    u64 syncFeature,
                                    const std::string& localPathAndThumbName)
{
    LOG_INFO("SwmOrSbmDelete(compId:"FMTu64",dset:"FMTu64",feature:"FMTu64",%s)",
             compId, datasetId, syncFeature, localPathAndThumbName.c_str());
    bool actualRemoval = false;
    // 1) Remove entry from the (a) job_queue then (b) the shared_files
    while (1) {
        SyncDownJob job_entry;
        int temp_rc = jobs->getJobByCompIdAndDatasetId(compId,
                                                       datasetId,
                                                       /*OUT*/ job_entry);
        if (temp_rc == DB_UTIL_ERR_ROW_NOT_FOUND) {
            break;  // goto COMMENT_LABEL_REMOVE_FROM_JOB_QUEUE_DONE
        } else if (temp_rc != 0) {
            LOG_ERROR("getJobByCompIdAndDatasetId("FMTu64","FMTu64"):%d",
                      compId, datasetId, temp_rc);
            break;  // goto COMMENT_LABEL_REMOVE_FROM_JOB_QUEUE_DONE
        }
        actualRemoval = true;
        // row exists.
        temp_rc = jobs->RemoveJob(job_entry.id);
        if (temp_rc != 0) {
            LOG_ERROR("RemoveJob("FMTu64","FMTu64","FMTu64"):%d",
                      compId, datasetId, job_entry.id, temp_rc);
        }
    }
    // COMMENT_LABEL_REMOVE_FROM_JOB_QUEUE_DONE

    // 2) Remove the downloaded thumbnail file.
    {
        VPLFS_stat_t statBuf;
        int temp_rc = VPLFS_Stat(localPathAndThumbName.c_str(), /*OUT*/ &statBuf);
        if (temp_rc == 0 && statBuf.type == VPLFS_TYPE_FILE) {
            temp_rc = VPLFile_Delete(localPathAndThumbName.c_str());
            if (temp_rc != 0) {
                LOG_ERROR("VPLFile_Delete(%s):%d. Continuing",
                          localPathAndThumbName.c_str(), temp_rc);
            }
        }
        temp_rc = jobs->sharedFilesEntryDelete(compId, datasetId);
        if (temp_rc != 0) {
            LOG_ERROR("sharedFilesEntryDelete("FMTu64","FMTu64"):%d. Continuing",
                      compId, datasetId, temp_rc);
        }
    }
    // COMMENT_LABEL_REMOVE_FROM_SHARED_FILES_DONE
    return 0;
}

int SyncDown::getChanges(u64 datasetid, u64 changeSince)
{
    int err = 0;
    u64 currentVersion = 0;
    bool isDBUpdated = false;

    bool isPicstreamDataset = false;
    bool isSwmDataset = false;
    bool isSbmDataset = false;
    int temp_rc = getDatasetInfo(datasetid,
                                /*OUT*/ isPicstreamDataset,
                                /*OUT*/ isSbmDataset,
                                /*OUT*/ isSwmDataset);
    if (temp_rc != 0) {
        LOG_WARN("getDatasetInfo("FMTu64"):%d, Continuing",
                 datasetid, temp_rc);
    }

    LOG_INFO("Global Delete: %s, dataset:"FMTu64"(%d,%d,%d)",
             globalDeleteEnabled ? "enabled":"disabled", datasetid,
             isPicstreamDataset, isSwmDataset, isSbmDataset);

    // When this happens, there needs to be a final event update at the end of
    // the loop.
    bool swmOrSbmEventUpdate = false;
    u64 swmOrSbmEventUpdateSyncFeature = 0;  // valid only if swmOrSwmEventUpdate is true.

    do {
        int httpResponse = -1;
        std::string changes;

        err = VCSgetDatasetChanges(userId, "", "", datasetid, changeSince, /*max*/500, /*includeDeleted*/true, httpResponse, changes);
        if (err) {
            LOG_ERROR("Failed to get changes from VCS: %d", err);
            return err;
        }
        if (httpResponse != 200) {
            LOG_ERROR("VCS returned %d", httpResponse);
            return err;
        }
        LOG_DEBUG("did:"FMTu64", response=%s", datasetid, changes.c_str());

        cJSON2 *json = cJSON2_Parse(changes.c_str());
        if (!json) {
            LOG_ERROR("Failed to parse response: %s", changes.c_str());
            return -1;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json);

        /* sample response
           {"currentVersion":49,"changeList":
             [{"name":"28229213562/home/fokushi/WORK/build_x.i686/debug/linux/tests/dxshell/CloudDoc.docx",
               "compId":105641,
               "isDeleted":true},
              {"name":"28229213562/a/b/c.docs",
               "compId":107691,
               "isDeleted":false}]}
        */

        cJSON2 *json_currentVersion = cJSON2_GetObjectItem(json, "currentVersion");
        if (!json_currentVersion) {
            LOG_ERROR("currentVersion missing");
            return -1;
        }
        if (json_currentVersion->type != cJSON2_Number) {
            LOG_ERROR("currentVersion in unexpected format");
            return -1;
        }
        currentVersion = json_currentVersion->valueint;
        LOG_DEBUG("currentVersion = "FMTu64, currentVersion);

        cJSON2 *json_changeList = cJSON2_GetObjectItem(json, "changeList");
        if (!json_changeList) {
            LOG_ERROR("changeList missing");
            return -1;
        }
        if (json_changeList->type != cJSON2_Array) {
            LOG_ERROR("changeList has unexpected value");
            return -1;
        }

        // NOTE: "nextVersion" is optional
        cJSON2 *json_nextVersion = cJSON2_GetObjectItem(json, "nextVersion");
        if (json_nextVersion) {
            if (json_nextVersion->type != cJSON2_Number) {
                LOG_ERROR("nextVersion in unexpected format");
                return -1;
            }
            changeSince = json_nextVersion->valueint;
        }
        else {
            changeSince = currentVersion;  // this will cause the do-while loop to terminate
        }

        int changeListSize = cJSON2_GetArraySize(json_changeList);
        for (int i = 0; i < changeListSize; i++) {
            cJSON2 *json_change = cJSON2_GetArrayItem(json_changeList, i);
            bool isDeleteEntry = false;
            if (!json_change) continue;

            {
                cJSON2 *json_isDeleted = cJSON2_GetObjectItem(json_change, "isDeleted");
                if (!json_isDeleted) {
                    LOG_ERROR("isDeleted missing");
                    continue;
                }
                if (json_isDeleted->type == cJSON2_True) {
                    if (isPicstreamDataset || isSwmDataset || isSbmDataset) {
                        isDeleteEntry = true;
                        // Deletion is supported
                    } else {
                        // Deletion is not supported, skip this entry.
                        continue;
                    }
                }
                else if (json_isDeleted->type != cJSON2_False) {
                    LOG_ERROR("isDeleted in unexpected format");
                    // bad entry, ignore this entry
                    continue;
                }
            }

            cJSON2 *json_type = cJSON2_GetObjectItem(json_change, "type");
            if (!json_type) {
                LOG_ERROR("type missing");
                continue;
            }
            if (json_type->type != cJSON2_String) {
                LOG_ERROR("type in unexpected format");
                continue;
            }
            if (strcmp(json_type->valuestring, "file") == 0 ||
                strcmp(json_type->valuestring, "link") == 0)
            {
                // Expected file types.
                //  1) "file" for datasets picstream and SBM
                //  2) "link" for dataset SWM
            } else {
                // this change is for a directory - skip
                continue;
            }

            std::string componentName;
            {
                cJSON2 *json_name = cJSON2_GetObjectItem(json_change, "name");
                if (!json_name) {
                    LOG_ERROR("name missing");
                    continue;
                }
                componentName = json_name->valuestring;
            }

            cJSON2 *json_compId = cJSON2_GetObjectItem(json_change, "compId");
            if (!json_compId) {
                LOG_ERROR("compId missing");
                continue;
            }
            if (json_compId->type != cJSON2_Number) {
                LOG_ERROR("compId in unexpected format");
                continue;
            }
            u64 compid = json_compId->valueint;

#if CCD_ENABLE_SYNCDOWN_PICSTREAM
            if (isPicstreamDataset)
            {
                u64 syncfeature = 0;
                std::string comppath;
                std::string localpath;
                u64 jobid;
                PicStreamItem item;
                u64 origDev, rev, fileSize;
                std::string dlUrl, previewUrl;
                bool syncUpAdded = false;

                if (isDeleteEntry)
                {
                    if(!globalDeleteEnabled) {
                        LOG_INFO("received global delete for %s", componentName.c_str());
                        continue;
                    }
                    // check if deleted item is existed, then add to del_job table
                    // full and lower res deletions come individually!
                    err = jobs->FindPicStreamItem(compid, item);
                    if (err == CCD_ERROR_NOT_FOUND) {
                        err = VPL_OK;
                        LOG_WARN("No item with compId="FMTu64" exists in PicStream DB! Count as success.", compid);
                        continue;
                    } else if( err != VPL_OK) {
                        LOG_ERROR("Read PicStream items error. err=%d", err);
                        return err;
                    }

                    LOG_INFO("Add Delete Item: name= %s, compId= "FMTu64, item.name.c_str(), item.compId);

                    // TODO: Why are we adding this as a job?  is there a long running operation?
                    //       Is it to retry deletes if a delete fails?  Please document.
                    u64 jobid;
                    err = jobs->AddDeleteJob(compid, componentName /*name*/, VPLTime_GetTime(), jobid);
                    continue;
                }

                // component path should have one of the following formats:
                // photos/YYYY_MM/YYYYMMDD_HHMMSS.suffix
                // lowres/YYYY_MM/YYYYMMDD_HHMMSS.suffix

                comppath.assign(componentName);

                const std::string prefix_photos = "photos/";
                const std::string prefix_lowres = "lowres/";
                const std::string prefix_thumb = "thumbnail/";

                std::string albumName, title;
                u64 takenTime = 0;
                u64 fileType;

                int pos_first_slash = comppath.find_first_of('/');
                int pos_second_slash = std::string::npos;
                int pos_dot = std::string::npos;
                if (std::string::npos != pos_first_slash) {
                    pos_second_slash = comppath.find_first_of('/', pos_first_slash+1);
                }

                if (std::string::npos != pos_second_slash) {
                    albumName = comppath.substr(pos_first_slash+1, pos_second_slash - pos_first_slash - 1);
                    title = comppath.substr(pos_second_slash+1, string::npos);

                    std::string takenTimeString = "";
                    char timeStamp[20];
                    char *pEnd;
                    pos_dot = comppath.find_first_of('.', pos_second_slash+1);
                    if (std::string::npos != pos_dot) {
                        takenTimeString = comppath.substr(pos_second_slash+1, pos_dot - pos_second_slash - 1);
                    }
                    takenTimeString.replace(8, 1, "");
                    sprintf(timeStamp, "%s", takenTimeString.c_str());
                    takenTime = (u64) strtoull(timeStamp, &pEnd, 10);
                }

                if (comppath.compare(0, prefix_photos.size(), prefix_photos.data()) == 0) {  // "photos/..." case
                    std::string downloadDir;
                    u32 unusedMaxFiles, unusedMaxBytes;
                    u32 unusedPreserveFreeDiskPercentage;
                    u64 unusedPreserveFreeDiskSizeBytes;
                    int err = getPicStreamDownloadDir(PicStreamDownloadDirEnum_FullRes,
                                                      downloadDir,
                                                      unusedMaxFiles,
                                                      unusedMaxBytes,
                                                      unusedPreserveFreeDiskPercentage,
                                                      unusedPreserveFreeDiskSizeBytes);
                    if (err == CCD_ERROR_NOT_FOUND) {
                        LOG_DEBUG("Skipping %s - no full-res download dir set", comppath.c_str());
                        localpath = "";
                    } else if (err) {
                        LOG_ERROR("Failed to get PicStream full-res download dir: %d", err);
                        continue;
                    } else {
                        Util_appendToAbsPath(downloadDir,
                                             comppath.substr(prefix_photos.length()),
                                             localpath);
                        syncfeature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES;
                    }
                    fileType = FileType_FullRes;

                }
                else if (comppath.compare(0, prefix_lowres.size(), prefix_lowres.data()) == 0) {  // "lowres/..." case
                    std::string downloadDir;
                    u32 unusedMaxFiles, unusedMaxBytes;
                    u32 unusedPreserveFreeDiskPercentage;
                    u64 unusedPreserveFreeDiskSizeBytes;
                    int err = getPicStreamDownloadDir(PicStreamDownloadDirEnum_LowRes,
                                                      downloadDir,
                                                      unusedMaxFiles,
                                                      unusedMaxBytes,
                                                      unusedPreserveFreeDiskPercentage,
                                                      unusedPreserveFreeDiskSizeBytes);
                    if (err == CCD_ERROR_NOT_FOUND) {
                        LOG_DEBUG("Skipping %s - no low-res download dir set", comppath.c_str());
                        localpath = "";
                    } else if (err) {
                        LOG_ERROR("Failed to get PicStream low-res download dir: %d", err);
                        continue;
                    } else {
                        Util_appendToAbsPath(downloadDir,
                                             comppath.substr(prefix_lowres.length()),
                                             localpath);
                        syncfeature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES;
                    }
                    fileType = FileType_LowRes;

                }
                else {
                    LOG_ERROR("Unexpected comppath %s", componentName.c_str());
                    continue;
                }

                getLatestMetadata(componentName, compid, datasetid, origDev, rev, fileSize, dlUrl, previewUrl);
                LOG_INFO("download URL=%s", dlUrl.c_str());
                LOG_INFO("preview URL=%s", previewUrl.c_str());

                err = jobs->AddPicStreamItem(compid, comppath , rev, 0 /*origCompId*/, origDev, ""/*identifier*/,
                                title, albumName, fileType, fileSize, takenTime, localpath, jobid, syncUpAdded);
                if (err != VPL_OK) {
                    LOG_ERROR("DB operation Failed!");
                    return err;
                }

                isDBUpdated = true;

                SyncDownDstStor sdds;
                err = sdds.init(userId);
                if (err) {
                    LOG_ERROR("SDDS init failed: %d", err);
                    continue;
                }

                if (!localpath.empty() && sdds.fileExists(localpath)) {
                    // A file exists at the corresponding local path.
                    // Most likely a duplicate, so skip file.
                    LOG_DEBUG("skip %s - local file exists", comppath.c_str());
                    //continue;
                    goto add_thumbnail;
                }

                if (!comppath.empty() && compid && !localpath.empty()) {
                    u64 _jobid;
                    err = jobs->AddJob(datasetid, comppath, compid, localpath, syncfeature, rev,  _jobid);
                }

add_thumbnail:
                if(FileType_FullRes == fileType) {
                    LOG_DEBUG("add thumbnail");

                    fileType = FileType_Thumbnail;
                    std::string thumbName = prefix_thumb;

                    size_t pos = comppath.find_first_of("/");
                    thumbName.append(comppath.substr(pos+1, string::npos));

                    std::string downloadDir;
                    u32 unusedMaxFiles, unusedMaxBytes;
                    u32 unusedPreserveFreeDiskPercentage;
                    u64 unusedPreserveFreeDiskSizeBytes;
                    int err = getPicStreamDownloadDir(PicStreamDownloadDirEnum_Thumbnail,
                                                      downloadDir,
                                                      unusedMaxFiles,
                                                      unusedMaxBytes,
                                                      unusedPreserveFreeDiskPercentage,
                                                      unusedPreserveFreeDiskSizeBytes);
                    if (err == CCD_ERROR_NOT_FOUND) {
                        LOG_DEBUG("Skipping %s - no thumbnail download dir set", thumbName.c_str());
                        localpath="";
                    } else if (err) {
                        LOG_ERROR("Failed to get PicStream thumbnail download dir: %d", err);
                        continue;
                    } else{
                        Util_appendToAbsPath(downloadDir,
                                             thumbName.substr(prefix_thumb.length()),
                                             localpath);
                        syncfeature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL;
                    }

                    err = jobs->AddPicStreamItem(compid, thumbName , rev, compid /*origCompId*/, origDev, ""/*identifier*/,
                                title, albumName, fileType, 0 /*fileSize*/, takenTime, localpath, jobid, syncUpAdded);
                    if (err != VPL_OK) {
                        LOG_ERROR("DB operation Failed!");
                        return err;
                    }

                    isDBUpdated = true;

                    if (!localpath.empty() && sdds.fileExists(localpath)) {
                        LOG_DEBUG("skip %s - local file exists", comppath.c_str());
                        continue;
                    }

                    //Note: compId are the same as full-res !
                    if (!thumbName.empty() && compid && !localpath.empty()) {
                        u64 _jobid;
                        err = jobs->AddJob(datasetid, thumbName, compid, localpath, syncfeature, rev, _jobid);
                    }

                } // add thumbnail

            } else
#endif // CCD_ENABLE_SYNCDOWN_PICSTREAM
            /*else*/ if (isSwmDataset || isSbmDataset)
            {
                u64 syncfeature = 0;
                std::string localpath;

                if (isSwmDataset) {
                    syncfeature = ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME;
                    int temp_rc = getSwmDownloadDir(/*OUT*/ localpath);
                    if (temp_rc != 0) {
                        LOG_ERROR("getSwmDownloadDir:%d", temp_rc);
                        continue;
                    }
                } else if (isSbmDataset) {
                    syncfeature = ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME;
                    int temp_rc = getSbmDownloadDir(/*OUT*/ localpath);
                    if (temp_rc != 0) {
                        LOG_ERROR("getSwmDownloadDir:%d", temp_rc);
                        continue;
                    }
                } else {
                    LOG_ERROR("Should never happen.  Not handled");
                }

                {   // Generate appropriate status event - so long as there changes in getChanges,
                    // generate a syncing event.
                    u32 pendingJobCounts = 0;
                    u32 failedJobCounts = 0;
                    // Get next job to download, change status to CCD_FEATURE_STATE_SYNCING,
                    // and generate EventSyncFeatureStatusChange if necessary.  Even metadata
                    // updates are important
                    int temp_rc = jobs->GetJobCountByFeature(syncfeature,
                                                             cutoff_ts,  // Previous cutoff time
                                                             /*OUT*/ pendingJobCounts,
                                                             /*OUT*/ failedJobCounts);
                    if(temp_rc != 0) {
                        LOG_ERROR("GetJobCountByFeature("FMTu64"):%d", syncfeature, temp_rc);
                    } else {
                        setSyncFeatureState((ccd::SyncFeature_t)syncfeature,
                                            ccd::CCD_FEATURE_STATE_SYNCING,
                                            pendingJobCounts,
                                            failedJobCounts,
                                            true);
                        swmOrSbmEventUpdate = true;
                        swmOrSbmEventUpdateSyncFeature = syncfeature;
                    }
                }

                std::string thumbName = componentName;
                std::string localPathAndName;
                Util_appendToAbsPath(localpath, thumbName, /*OUT*/ localPathAndName);

                if (isDeleteEntry)
                {
                    int temp_rc = handleSwmAndSbmDelete(compid,
                                                        datasetid,
                                                        syncfeature,
                                                        localPathAndName);
                    if (temp_rc != 0) {
                        LOG_ERROR("handleSwmAndSbmDelete("FMTu64","FMTu64","FMTu64"):%d, Continuing",
                                  compid, datasetid, syncfeature, temp_rc);
                    }

                    continue;
                }

                u64 revisionId;
                bool doThumbnailDownload = false;
                SyncDownJobs::SharedFilesDbEntry sharedFilesDbEntry;
                {
                    VcsV1_getFileMetadataResponse gfmResponse;
                    std::ostringstream oss;
                    oss << "https://" << VCS_getServer(userId, datasetid);

                    VcsSession vcsSession;
                    vcsSession.userId = userId;
                    vcsSession.deviceId = VirtualDevice_GetDeviceId();
                    vcsSession.urlPrefix = oss.str();
                    int temp_rc = VCSGetCredentials(userId,
                                                    /*OUT*/ vcsSession.sessionHandle,
                                                    /*OUT*/ vcsSession.sessionServiceTicket);
                    if (temp_rc != 0) {
                        LOG_ERROR("VCSGetCredentials(uid:"FMTu64",did:"FMTu64"):%d",
                                  userId, datasetid, temp_rc);
                        continue;
                    }


                    const VcsCategory* vcsCategory = NULL;
                    if (isSwmDataset) {
                        vcsCategory = &VCS_CATEGORY_SHARED_WITH_ME;
                    } else if (isSbmDataset) {
                        vcsCategory = &VCS_CATEGORY_SHARED_BY_ME;
                    } else {
                        LOG_ERROR("Should never happen.  Not handled");
                        continue;
                    }

                    VcsDataset vcsDataset(datasetid, *vcsCategory);
                    VPLHttp2 httpHandle;
                    temp_rc = VcsV1_getFileMetadata(vcsSession,
                                                    httpHandle,
                                                    vcsDataset,
                                                    componentName,
                                                    compid,
                                                    true,
                                                    /*OUT*/gfmResponse);
                    if (temp_rc != 0) {
                        LOG_ERROR("VcsV1_getFileMetadata(compId:"FMTu64",%s,did:"FMTu64"):%d",
                                  compid, componentName.c_str(), datasetid, temp_rc);
                        continue;
                    }

                    if (gfmResponse.revisionList.size() == 0) {
                        LOG_ERROR("No revisions(compId:"FMTu64",%s,did:"FMTu64")",
                                  compid, componentName.c_str(), datasetid);
                        continue;
                    }
                    VcsV1_getRevisionListEntry& revision = gfmResponse.revisionList[gfmResponse.revisionList.size()-1];
                    revisionId = revision.revision;

                    err = jobs->sharedFilesItemGet(datasetid, compid,
                                                   /*OUT*/ sharedFilesDbEntry);
                    if (err == DB_UTIL_ERR_ROW_NOT_FOUND) {
                        // Normal case, drop through. Revision Id will be different (non-zero),
                        // and download will happen.
                    } else if (err != 0) {
                        LOG_ERROR("sharedFilesItemGet(did:"FMTu64",cid:"FMTu64"):%d",
                                  datasetid, compid, err);
                    }

                    if (!sharedFilesDbEntry.b_thumb_downloaded ||
                        sharedFilesDbEntry.revision != revisionId)
                    {
                        doThumbnailDownload = true;
                    }

                    sharedFilesDbEntry.compId = compid;
                    sharedFilesDbEntry.name = componentName;
                    sharedFilesDbEntry.datasetId = datasetid;
                    sharedFilesDbEntry.feature = syncfeature;
                    // b_thumb_downloaded takes previous setting

                    std::string recipientList;
                    bool skipFirst = true;
                    for (std::vector<std::string>::iterator iter =  gfmResponse.recipientList.begin();
                         iter != gfmResponse.recipientList.end(); ++iter)
                    {
                        if (!skipFirst) {
                            recipientList.append(",");
                        }
                        skipFirst = false;

                        recipientList.append(*iter);
                    }
                    sharedFilesDbEntry.recipientList = recipientList;
                    sharedFilesDbEntry.revision = revision.revision;
                    sharedFilesDbEntry.size = revision.size;
                    sharedFilesDbEntry.lastChangedExists = (gfmResponse.lastChangedNano != 0);
                    sharedFilesDbEntry.lastChanged = gfmResponse.lastChangedNano;
                    sharedFilesDbEntry.createDateExists = (gfmResponse.createDateNano != 0);
                    sharedFilesDbEntry.createDate = gfmResponse.createDateNano;
                    sharedFilesDbEntry.updateDeviceExists = (revision.updateDevice != 0);
                    sharedFilesDbEntry.updateDevice = revision.updateDevice;
                    sharedFilesDbEntry.opaqueMetadataExists = (!gfmResponse.opaqueMetadata.empty());
                    sharedFilesDbEntry.opaqueMetadata = gfmResponse.opaqueMetadata;
                    sharedFilesDbEntry.relThumbPathExists = true;
                    sharedFilesDbEntry.relThumbPath = thumbName;
                }

                if (doThumbnailDownload)
                {   // Adding work to be done before
                    u64 jobId_unused;
                    err = jobs->AddJob(datasetid,
                                       thumbName,
                                       compid,
                                       localPathAndName,
                                       syncfeature,
                                       revisionId,
                                       /*OUT*/ jobId_unused);
                    if (err != 0) {
                        LOG_ERROR("Unable to add job("FMTu64","FMTu64",%s,%s):%d",
                                  datasetid, compid, thumbName.c_str(),
                                  localPathAndName.c_str(), err);
                        continue;
                    }
                }

                err = jobs->sharedFilesItemAdd(sharedFilesDbEntry);
                if (err != 0) {
                    LOG_ERROR("sharedFilesItemAdd("FMTu64","FMTu64",%s,%s):%d",
                              datasetid, compid, thumbName.c_str(),
                              localPathAndName.c_str(), err);
                }
            }
        }  // for (int i = 0; i < changeListSize; i++)

        // Bug 7359:
        // Update last-checked version every time after processing all the changes
        // in the response from a single call to VCSgetDatasetChanges().
        // This is to protect against SyncDown thread having to start all over
        // again if the thread is killed inside this while loop.
        // (The call to update last-checked version used to be after this while loop.)
        // Note that there could still be a small loss if the thread is killed
        // between the time a job is added and last-checked version is saved.
        // So the intent is not to fix it complete (not possible) but to reduce loss.
        // BTW, the fact that the list of jobs could be ahead of the last-checked
        // version is not a concern.
        // When we reprocess the same changes, the "new" jobs will automatically
        // replace the "old" jobs.  (This is how AddJob works.)
        if (changeSince > 0) {
            err = jobs->SetDatasetLastCheckedVersion(datasetid, changeSince);
            if (err) {
                LOG_ERROR("Failed to update last-checked version: %d", err);
            }
        }
    } while (changeSince < currentVersion);

    if (isPicstreamDataset) {
        jobs->MapFullLowResInDatabase();

        if(isDBUpdated) {
            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
            ccdiEvent->mutable_picstreamdb_updated()->set_dataset_id(datasetid);
            EventManagerPb_AddEvent(ccdiEvent);
        }
    } else if (isSwmDataset || isSbmDataset) {
        if (swmOrSbmEventUpdate)
        {   // Generate appropriate status event
            u32 pendingJobCounts = 0;
            u32 failedJobCounts = 0;
            // Get next job to download, change status to CCD_FEATURE_STATE_SYNCING,
            // and generate EventSyncFeatureStatusChange if necessary.
            int temp_rc = jobs->GetJobCountByFeature(swmOrSbmEventUpdateSyncFeature,
                                                     cutoff_ts,  // Previous cutoff time
                                                     /*OUT*/ pendingJobCounts,
                                                     /*OUT*/ failedJobCounts);
            if(temp_rc != 0) {
                LOG_ERROR("GetJobCountByFeature("FMTu64"):%d",
                          swmOrSbmEventUpdateSyncFeature, temp_rc);
            } else {
                if(pendingJobCounts > 0) {
                    setSyncFeatureState((ccd::SyncFeature_t)swmOrSbmEventUpdateSyncFeature,
                                        ccd::CCD_FEATURE_STATE_SYNCING,
                                        pendingJobCounts,
                                        failedJobCounts,
                                        false);
                } else if (failedJobCounts > 0) {
                    setSyncFeatureState((ccd::SyncFeature_t)swmOrSbmEventUpdateSyncFeature,
                                        ccd::CCD_FEATURE_STATE_OUT_OF_SYNC,
                                        pendingJobCounts,
                                        failedJobCounts,
                                        false);
                } else {
                    setSyncFeatureState((ccd::SyncFeature_t)swmOrSbmEventUpdateSyncFeature,
                                        ccd::CCD_FEATURE_STATE_IN_SYNC,
                                        pendingJobCounts,
                                        failedJobCounts,
                                        false);
                }
            }
        }
    }

    return err;
}

int SyncDown::applyChanges()
{
    int err;
    int rv = 0;
    u64 cutoff = VPLTime_GetTime();
    cutoff_ts = cutoff;
    SyncDownJob job;
    bool copyback_to_picstream = false;
    u64 picstream_datasetId = 0;
    u64 syncfeature = 0;
    PicstreamDirToNumNewMap picstreamDirs;
    const int PICSTREAM_CHECK_THRESH = 30;

    u32 failedJobCounts = 0;
    u32 pendingJobCounts = 0;

    while (1) {
#if defined(CLOUDNODE)
        err = isSpaceAvailable(workdir);
        if (err != VPL_OK) break;
#endif
        err = getJobCopybackReady(cutoff, job);
        // errmsg logged by getJobCopybackReady()
        if (err) {
            // No next Copyback job, break the while and try to get next download job
            break;
        } else {
#if defined(CLOUDNODE)
//This define is from storageNode/src/managed_dataset.hpp
#define MAXIMUM_COMPONENTS 64*1024

            SyncDownDstStor sdds;
            int component_count = 0;
            err = sdds.init(userId);
            if (err) {
                LOG_ERROR("SDDS init failed: %d", err);
            }
            err = sdds.getSiblingComponentCount(job.lpath, component_count);
            if (err) {
                LOG_ERROR("get component count failed: %d", err);
            }
            if (component_count >= MAXIMUM_COMPONENTS){
                LOG_ERROR("sibling component count over limit %d: %d", MAXIMUM_COMPONENTS, component_count);
                break;
            }
#endif  //defined(CLOUDNODE)
            // Get next job to download, change status to CCD_FEATURE_STATE_SYNCING,
            // and generate EventSyncFeatureStatusChange if necessary.
            err = jobs->GetJobCountByFeature(job.feature,
                                             cutoff,
                                             pendingJobCounts,
                                             failedJobCounts);
            if(err) {
                LOG_ERROR("Error when getting job counts! error = %d", err);
            } else {
                setSyncFeatureState((ccd::SyncFeature_t)job.feature,
                                    ccd::CCD_FEATURE_STATE_SYNCING,
                                    pendingJobCounts,
                                    failedJobCounts,
                                    true);
            }
        }
        u64 currVplTimeSec = VPLTime_GetTime();
        if(currVplTimeSec < cutoff) {
            // Time went backwards.  Let's not get into infinite loop.
            LOG_ERROR("Time went backwards(%s): "FMTu64"->"FMTu64,
                      job.cpath.c_str(), cutoff, currVplTimeSec);
            break;  // expected to go to comment label: SYNCDOWN_OUTSIDE_WHILE_1
        }

        err = jobs->TimestampJobDispatch(job.id, currVplTimeSec);
        if (err) {
            LOG_ERROR("Failed to timestamp dispatch: %d", err);
            rv = err;
            goto check_copyback_state;
        }

        err = tryCopyback(job);
        // errmsg logged by tryCopyback()
        if (err) {
            rv = err;
            goto check_copyback_state;
        }

        {
            bool isPicstreamDataset = false;
            bool isSwmDataset = false;
            bool isSbmDataset = false;
            int temp_rc = getDatasetInfo(job.did,
                                        /*OUT*/ isPicstreamDataset,
                                        /*OUT*/ isSbmDataset,
                                        /*OUT*/ isSwmDataset);
            if (temp_rc != 0) {
                LOG_WARN("getDatasetInfo("FMTu64"):%d, Continueing",
                         job.did, temp_rc);
            }
            if (isPicstreamDataset) {
                copyback_to_picstream = true;
                picstream_datasetId = job.did;
                syncfeature = job.feature;
                std::string parentDir = Util_getParent(job.lpath);
                picstreamDirs[parentDir]++;
                if(picstreamDirs[parentDir] > PICSTREAM_CHECK_THRESH) {
                    enforcePicstreamThresh(picstreamDirs);
                }
            }
        }

      check_copyback_state:
        err = jobs->GetJobCountByFeature(job.feature,
                                         cutoff,
                                         pendingJobCounts,
                                         failedJobCounts);
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
    // Comment Label: SYNCDOWN_OUTSIDE_WHILE_1

    while (1)
    {
        VPL_BOOL ansConnected = ANSConn_IsActive();
        if (!ansConnected && !overrideAnsCheck) {
            rv = CCD_ERROR_ANS_FAILED;
            break;
        }
#if defined(CLOUDNODE)
        err = isSpaceAvailable(workdir);
        if (err != VPL_OK) { break; }
#endif
        err = getJobDownloadReady(cutoff, job);
        // errmsg logged by getJobDownloadReady()
        if (err) {
            // No next Download job, break the while
            break;
        } else {
#if defined(CLOUDNODE)
//This define is from storageNode/src/managed_dataset.hpp
#define MAXIMUM_COMPONENTS 64*1024

            SyncDownDstStor sdds;
            int component_count = 0;
            err = sdds.init(userId);
            if (err) {
                LOG_ERROR("SDDS init failed: %d", err);
            }
            err = sdds.getSiblingComponentCount(job.lpath, component_count);
            if (err) {
                LOG_ERROR("get component count failed: %d", err);
            }
            if (component_count >= MAXIMUM_COMPONENTS){
                LOG_ERROR("sibling component count over limit %d: %d", MAXIMUM_COMPONENTS, component_count);
                break;
            }
#endif  //defined(CLOUDNODE)
            // Get next job to download, change status to CCD_FEATURE_STATE_SYNCING,
            // and generate EventSyncFeatureStatusChange if necessary.
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
        }

        u64 currVplTimeSec = VPLTime_GetTime();
        if(currVplTimeSec < cutoff) {
            // Time went backwards.  Let's not get into infinite loop.
            LOG_ERROR("Time went backwards(%s): "FMTu64"->"FMTu64,
                      job.cpath.c_str(), cutoff, currVplTimeSec);
            break;  // expected to go to comment label: SYNCDOWN_OUTSIDE_WHILE_2
        }

        err = jobs->TimestampJobDispatch(job.id, currVplTimeSec);
        if (err) {
            LOG_ERROR("Failed to timestamp dispatch: %d", err);
            rv = err;
            goto check_download_state;
        }

        err = tryDownload(job);
        // errmsg logged by tryDownload()
        if (err) {
            rv = err;
            goto check_download_state;
        }

        err = tryCopyback(job);
        // errmsg logged by tryCopyback()
        if (err) {
            rv = err;
            goto check_download_state;
        }

        {   // Copyback success
            bool isPicstreamDataset = false;
            bool isSwmDataset = false;
            bool isSbmDataset = false;
            int temp_rc = getDatasetInfo(job.did,
                                        /*OUT*/ isPicstreamDataset,
                                        /*OUT*/ isSbmDataset,
                                        /*OUT*/ isSwmDataset);
            if (temp_rc != 0) {
                LOG_WARN("getDatasetInfo("FMTu64"):%d, Continueing",
                         job.did, temp_rc);
            }
            if (isPicstreamDataset) {
                copyback_to_picstream = true;
                picstream_datasetId = job.did;
                syncfeature = job.feature;
                std::string parentDir = Util_getParent(job.lpath);
                picstreamDirs[parentDir]++;
                if(picstreamDirs[parentDir] > PICSTREAM_CHECK_THRESH) {
                    enforcePicstreamThresh(picstreamDirs);
                }
            }
        }

      check_download_state:
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
    // Comment Label: SYNCDOWN_OUTSIDE_WHILE_2

    if (copyback_to_picstream) {
        enforcePicstreamThresh(picstreamDirs);
    }

    return rv;
}

int SyncDown::applyDeletions(VPLUser_Id_t userId, u64 datasetId)
{
    int rv = VPL_OK;
    int result = VPL_OK;
    u64 cutoff = VPLTime_GetTime();
    std::string delete_path;
    cutoff_ts = cutoff;
    DeleteJob job;
    PicStreamItem item;

    bool isDBUpdated = false;

    u32 failedJobCounts = 0;
    u32 pendingJobCounts = 0;

    const std::string prefix_photos = "photos/";
    const std::string prefix_lowres = "lowres/";

    std::string syncDownWorkPath;
    std::string fullDir, lowResDir, thumbDir;
    std::string fullPath, lowPath, thumbPath;
    std::queue<std::string> delPaths;

    {
        u32 unusedMaxFiles, unusedMaxBytes;
        u32 unusedPreserveFreeDiskPercentage;
        u64 unusedPreserveFreeDiskSizeBytes;
        int err = getPicStreamDownloadDir(PicStreamDownloadDirEnum_FullRes,
                                          fullDir,
                                          unusedMaxFiles,
                                          unusedMaxBytes,
                                          unusedPreserveFreeDiskPercentage,
                                          unusedPreserveFreeDiskSizeBytes);
        if (err == CCD_ERROR_NOT_FOUND) {
            fullDir = "";
        } else if (err) {
            LOG_WARN("Failed to get PicStream full-res download dir: %d", err);
            fullDir = "";
        }

        err = getPicStreamDownloadDir(PicStreamDownloadDirEnum_LowRes,
                                          lowResDir,
                                          unusedMaxFiles,
                                          unusedMaxBytes,
                                          unusedPreserveFreeDiskPercentage,
                                          unusedPreserveFreeDiskSizeBytes);
        if (err == CCD_ERROR_NOT_FOUND) {
            lowResDir = "";
        } else if (err) {
            LOG_WARN("Failed to get PicStream low-res download dir: %d", err);
            lowResDir = "";
        }

        err = getPicStreamDownloadDir(PicStreamDownloadDirEnum_Thumbnail,
                                          thumbDir,
                                          unusedMaxFiles,
                                          unusedMaxBytes,
                                          unusedPreserveFreeDiskPercentage,
                                          unusedPreserveFreeDiskSizeBytes);
        if (err == CCD_ERROR_NOT_FOUND) {
            thumbDir = "";
        } else if (err) {
            LOG_WARN("Failed to get PicStream thumbnail download dir: %d", err);
            thumbDir = "";
        }

        char workPath[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForSyncDown(userId, sizeof(workPath), workPath);
        syncDownWorkPath.assign(workPath);
    }

    while (1) {

        fullPath.clear();
        lowPath.clear();
        thumbPath.clear();

        rv = jobs->GetDeleteJobCount(cutoff,
                                     pendingJobCounts,
                                     failedJobCounts);
        if(rv) {
            LOG_ERROR("Error when getting job counts! error = %d", rv);
            goto end;
        } else {
            setSyncFeatureState((ccd::SyncFeature_t) ccd::SYNC_FEATURE_PICSTREAM_DELETION,
                                ccd::CCD_FEATURE_STATE_SYNCING,
                                pendingJobCounts,
                                failedJobCounts,
                                true);
        }

        rv = jobs->GetNextJobToDelete(cutoff, job);
        if(CCD_ERROR_NOT_FOUND == rv) {
            break;
        }
        rv = jobs->TimestampJobDeleteDispatch(job.id);
        if(rv != VPL_OK) {
            LOG_ERROR(FMTu64" Failed to timestamp dispatch: %d", job.id, rv);
            goto end;
        }
        rv = jobs->TimestampJobDeleteTry(job.id);
        if(rv != VPL_OK) {
            LOG_ERROR(FMTu64" Failed to timestamp Try: %d", job.id, rv);
            goto end;
        }

        if (job.name.compare(0, prefix_photos.size(), prefix_photos.data()) == 0) {  // "photos/..." case
            if(fullDir.length() > 0){
                Util_appendToAbsPath(fullDir,
                    job.name.substr(prefix_photos.length()),
                    fullPath);
            } else {
                getCacheFilePath(datasetId, job.cid, 0, job.name, fullPath);
                fullPath = syncDownWorkPath + fullPath;
            }
            delPaths.push(fullPath);

            if(thumbDir.length() > 0){ //thumbnail is always jpg
                Util_appendToAbsPath(thumbDir,
                    job.name.substr(prefix_photos.length()),
                    thumbPath);
            }  else {
                getCacheFilePath(datasetId, job.cid, 2, job.name, thumbPath);
                thumbPath = syncDownWorkPath + thumbPath;
            }
            delPaths.push(thumbPath);
        } else {

            if(lowResDir.length() > 0){
                Util_appendToAbsPath(lowResDir,
                    job.name.substr(prefix_lowres.length()),
                    lowPath);
            } else {
                getCacheFilePath(datasetId, job.cid, 0, job.name, lowPath);
                lowPath = syncDownWorkPath + lowPath;
            }
            delPaths.push(lowPath);
        }

        std::string path;
        while(delPaths.size() > 0) {
            path = delPaths.front();
            LOG_INFO("Remove File: %s", path.c_str());
            rv = VPLFile_Delete(path.c_str());
            if(rv != VPL_OK) {
                VPLFS_stat_t stat;
                result = rv;
                rv = VPLFS_Stat(path.c_str(), &stat);
                if(rv != VPL_OK) {
                    LOG_WARN("File %s, %d does not exist. Count as success.", path.c_str(), result);
                    rv = VPL_OK;
                } else {
                    LOG_ERROR("Delete of %s failed:%d. Continuing.", path.c_str(), result);
                    rv = result;
                    goto updateInfo;
                }
            }
            delPaths.pop();
        }

        // Because full and thumbnail have the same compId, they are removed at the same time!
        rv = jobs->RemovePicStreamItem(job.cid);
        if(CCD_ERROR_NOT_FOUND == rv) {
            rv = VPL_OK;
        } else if(rv != VPL_OK) {
            LOG_ERROR("Remove Items in DB Failed. (compId: "FMTu64") err=%d", job.cid, rv);
        }

updateInfo:
        if(VPL_OK == rv) {
            isDBUpdated = true;
            LOG_INFO("Remove items (compId: "FMTu64") Done.", job.cid);
            rv = jobs->TimestampJobDeleteDone(job.id);
            if(rv != VPL_OK) {
                LOG_ERROR(FMTu64" Failed to timestamp Done: %d", job.id, rv);
                goto end;
            }
            rv = jobs->RemoveJobDelete(job.cid);
            if(rv != VPL_OK) {
                LOG_ERROR(FMTu64" Failed to remove delete job: %d", job.id, rv);
                goto end;
            }
        } else {
            LOG_ERROR("Remove Items Failed. (compId: "FMTu64") err=%d, retry later.", job.cid, rv);
            rv = jobs->ReportJobDeleteFailed(job.id);
            if(rv != VPL_OK) {
                LOG_ERROR(FMTu64" Failed to Update Failed Info: %d", job.id, rv);
                goto end;
            }
        }
    }

end:
    result = rv;
    rv = jobs->GetDeleteJobCount(cutoff,
                                 pendingJobCounts,
                                 failedJobCounts);
    if(rv) {
        LOG_ERROR("Error when getting job counts! error = %d", rv);
    } else {
        if(pendingJobCounts > 0) {
            setSyncFeatureState((ccd::SyncFeature_t) ccd::SYNC_FEATURE_PICSTREAM_DELETION,
                    ccd::CCD_FEATURE_STATE_SYNCING,
                    pendingJobCounts,
                    failedJobCounts,
                    false);
        } else if (failedJobCounts > 0) {
            setSyncFeatureState((ccd::SyncFeature_t) ccd::SYNC_FEATURE_PICSTREAM_DELETION,
                    ccd::CCD_FEATURE_STATE_OUT_OF_SYNC,
                    pendingJobCounts,
                    failedJobCounts,
                    false);
        } else {
            setSyncFeatureState((ccd::SyncFeature_t) ccd::SYNC_FEATURE_PICSTREAM_DELETION,
                    ccd::CCD_FEATURE_STATE_IN_SYNC,
                    pendingJobCounts,
                    failedJobCounts,
                    false);
        }
    }

    if (rv != VPL_OK) result = rv;

    if(isDBUpdated) {
        ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
        ccdiEvent->mutable_picstreamdb_updated()->set_dataset_id(datasetId);
        EventManagerPb_AddEvent(ccdiEvent);
    }

    return result;
}

int SyncDown::setSyncFeatureState(ccd::SyncFeature_t feature,
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
                ccdiEvent->mutable_sync_feature_status_change()->set_feature(feature);
                ccdiEvent->mutable_sync_feature_status_change()->mutable_status()->set_status(state);
                ccdiEvent->mutable_sync_feature_status_change()->mutable_status()->set_pending_files(pendingJobCounts);
                ccdiEvent->mutable_sync_feature_status_change()->mutable_status()->set_failed_files(failedJobCounts);
                EventManagerPb_AddEvent(ccdiEvent);
                return 0;
            }
            return 0;
        } else {
            // No matter the previous state was, set the state and generate event
            it->second = state;
            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
            ccdiEvent->mutable_sync_feature_status_change()->set_feature(feature);
            ccdiEvent->mutable_sync_feature_status_change()->mutable_status()->set_status(state);
            ccdiEvent->mutable_sync_feature_status_change()->mutable_status()->set_pending_files(pendingJobCounts);
            ccdiEvent->mutable_sync_feature_status_change()->mutable_status()->set_failed_files(failedJobCounts);
            EventManagerPb_AddEvent(ccdiEvent);
            return 0;
        }
    } else {
        LOG_ERROR("Error when getting sync feature! feature = %d, state = %d", (int)feature, (int)state);
        return -1;
    }
}

int SyncDown::getJobDownloadReady(u64 threshold, SyncDownJob &job)
{
    int result = 0;

    int err = jobs->GetNextJobToDownload(threshold, job);
    if (err == CCD_ERROR_NOT_FOUND) {
        LOG_DEBUG("No more job ready for download");
    }
    else if (err) {
        LOG_ERROR("Failed to get job ready for download: %d", err);
        result = err;
    }

    return err;
}

int SyncDown::getJobCopybackReady(u64 threshold, SyncDownJob &job)
{
    int result = 0;

    int err = jobs->GetNextJobToCopyback(threshold, job);
    if (err == CCD_ERROR_NOT_FOUND) {
        LOG_DEBUG("No more job ready for copyback");
    }
    else if (err) {
        LOG_ERROR("Failed to get job ready for copyback: %d", err);
        result = err;
    }

    return err;
}

int SyncDown::tryDownload(SyncDownJob &job)
{
    int err = 0;

    LOG_DEBUG("id "FMTu64" cpath %s", job.id, job.cpath.c_str());

    if (job.dl_path.empty()) {
        std::ostringstream oss;
        // To test Bug 8267, temporarily change the string below to anything that ends with "tmp/sd_"
        // E.g., oss << "/a/b/c/d/tmp/sd_" << job.id;
        oss << "tmp/sd_" << job.id;
        job.dl_path.assign(oss.str());
        err = jobs->SetJobDownloadPath(job.id, job.dl_path);
        if (err) {
            LOG_ERROR("Failed to set download path: %d", err);
            return err;
        }
    }

    err = jobs->TimestampJobTryDownload(job.id);
    if (err < 0) {
        LOG_ERROR("Failed to set download try timestamp: compname=%s", job.cpath.c_str());
        return err;
    }

    do {
        //revision was set when getChanges in PicStream 3.0. 
        //err = getLatestRevNum(job, job.dl_rev);
        // errmsg logged by getLatestRevNum()
        if (err) break;

        if(job.feature == ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL ||
           job.feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME ||
           job.feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME)
        {
            err = downloadPreview(job, job.dl_rev, job.feature);
        } else {
            err = downloadRevision(job, job.dl_rev, NULL);
            // errmsg logged by downloadRevision()
        }
        if (err) break;
    } while (0);

    if (err > 0) {  // error - drop job
        LOG_ERROR("SDjobid drop("FMTu64",%s)", job.id, job.cpath.c_str());
        int err2 = jobs->RemoveJob(job.id);
        if (err2) {
            LOG_ERROR("Failed to remove job "FMTu64": %d", job.id, err2);
        }
    }
    else if (err < 0) {  // error - retry later
        LOG_WARN("SDjobid retry("FMTu64",%s)", job.id, job.cpath.c_str());
        (void)jobs->ReportJobDownloadFailed(job.id);
    }
    else {  // success
        LOG_INFO("SDjobid downloaded("FMTu64",%s)", job.id, job.cpath.c_str());
        err = jobs->TimestampJobDoneDownload(job.id);
        if (err != 0) {
            LOG_ERROR("Failed to set download done timestamp: compname=%s", job.cpath.c_str());
        }
        else {
            err = jobs->SetJobDownloadedRevision(job.id, job.dl_rev);
            if (err) {
                LOG_ERROR("Failed to set downloaded revision of ("FMTu64",%s): %d", job.did, job.cpath.c_str(), err);
            }
        }
    }

    return err;
}

int SyncDown::tryCopyback(SyncDownJob &job)
{
    int result = 0;
    int err = 0;
    bool drop_job = false;
    bool retry_later = false;

    LOG_DEBUG("id "FMTu64" cpath %s", job.id, job.cpath.c_str());

    do {
        err = jobs->TimestampJobTryCopyback(job.id);
        if (err < 0) {
            LOG_ERROR("Failed to set copyback try timestamp: cpath=%s", job.cpath.c_str());
            retry_later = true;
            break;
        }

        if (job.feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME ||
            job.feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME)
        {   // Avoid special logic that's put in for picstream orbe.  These are
            // files private to CCD that do need to be exposed to any other features.
            err = Util_CreatePath(job.lpath.c_str(), VPL_FALSE);
            if (err != 0) {
                LOG_ERROR("Util_CreatePath(%s):%d", job.lpath.c_str(), err);
                retry_later = true;
                break;
            }
            std::string dl_path;
            extendDlPathToFullPath(job.dl_path, workdir, dl_path);
            err = VPLFile_Rename(dl_path.c_str(), job.lpath.c_str());
            if (err) {
                LOG_ERROR("VPLFile_Rename(%s->%s):%d", dl_path.c_str(), job.lpath.c_str(), err);
                retry_later = true;
                break;
            }
        } else
        {   // VCS picstream seems to require some special logic not needed by sbm/swm.
            // TODO: Someone document why, I have no idea.  Guess is remote file access related?
            SyncDownDstStor sd3;
            err = sd3.init(userId);
            if (err) {
                retry_later = true;
                break;
            }

            err = sd3.createMissingAncestorDirs(job.lpath);
            if (err) {
                // TODO: should this be retried?
                LOG_WARN("Failed to create ancestor dirs for %s: %d", job.lpath.c_str(), err);
                retry_later = true;
                break;
            }

            std::string dl_path;
            extendDlPathToFullPath(job.dl_path, workdir, dl_path);
#ifndef CLOUDNODE
            err = VPLFile_Rename(dl_path.c_str(), job.lpath.c_str());
            if (err) {
                LOG_WARN("Failed to move file from %s to %s: %d", dl_path.c_str(), job.lpath.c_str(), err);
                LOG_WARN("Try to copyFile");
#endif  //CLOUDNODE
                err = sd3.copyFile(dl_path, job.lpath);
                if (err) {
                    // TODO: should this be retried?
                    LOG_ERROR("Failed to copy file from %s to %s: %d", dl_path.c_str(), job.lpath.c_str(), err);
                    retry_later = true;
                    break;
                }
#ifndef CLOUDNODE
            }
#endif  //CLOUDNODE
        }

        if (err == 0)
        {   // Success!
            if (job.feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME ||
                job.feature == ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME)
            {   // Mark thumbnail as downloaded before the job is marked done;
                // otherwise, if we mark anywhere else, an inopportune restart
                // could skip this step
                int temp_rc;
                temp_rc = jobs->sharedFilesUpdateThumbDownloaded(job.cid,
                                                                 job.feature,
                                                                 true);  // thumbnail downloaded.
                if (temp_rc != 0) {
                    LOG_ERROR("sharedFilesUpdateThumbDownloaded("FMTu64","FMTu64"):%d.  Continuing.",
                              job.cid, job.feature, temp_rc);
                }
            }
        }

        err = jobs->TimestampJobDoneCopyback(job.id);
        if (err < 0) {
            LOG_ERROR("Failed to set copyback done timestamp: compname=%s", job.cpath.c_str());
            drop_job = true;
            break;
        }

        LOG_DEBUG("id "FMTu64" done", job.id);
        drop_job = true;
    } while (0);

    result = err;

    if (retry_later) {
        LOG_DEBUG("id "FMTu64" retry", job.id);
        err = jobs->ReportJobCopybackFailed(job.id);
        if (err) {
            LOG_ERROR("Failed to inc copyback fail count "FMTu64": %d", job.id, err);
            if (!result) result = err;
        }
    }

    if (drop_job) {
        LOG_DEBUG("id "FMTu64" drop", job.id);
        err = jobs->RemoveJob(job.id);
        if (err) {
            LOG_ERROR("Failed to remove job "FMTu64": %d", job.id, err);
            if (!result) result = err;
        }
    }

    return result;
}

int SyncDown::getLatestRevNum(SyncDownJob &job, /*OUT*/ u64 &revision)
{
    int err = 0;

    std::string uri;
    {
        std::ostringstream oss;
        oss << "/vcs/filemetadata/" << job.did << "/" << VPLHttp_UrlEncoding(job.cpath, "/") << "?compId=" << job.cid;
        uri.assign(oss.str());
    }
    std::string metadata;
    err = VCSreadFile_helper(userId, job.did, "", "", VirtualDevice_GetDeviceId(), uri, NULL, metadata);
    if (err==CCD_ERROR_HTTP_STATUS ||
        err==CCD_ERROR_BAD_SERVER_RESPONSE)
    {
        // http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_API_Client_Side_Error_Handling#CCD_calling_VCS-v1-API_error_handling
        LOG_WARN("File assumed expired:%s", job.cpath.c_str());
        return 1;  // drop job, positive error code.
    }else if (err) {
        LOG_ERROR("Failed to get file metadata from VCS: %d", err);
        return err;
    }

    cJSON2 *json = cJSON2_Parse(metadata.c_str());
    if (!json) {
        LOG_ERROR("Failed to parse response: %s", metadata.c_str());
        return 1;  // drop job
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json);

    /* sample response
       {"name":"28229213562/temp/a9",
        "compId":1223290246,
        "numOfRevisions":1,
        "originDevice":28229213562,
        "revisionList":[{"revision":1,
                         "size":65536,
                         "lastChanged":1351645142,
                         "updateDevice":28229213562,
                         "previewUri":"/clouddoc/preview/15540016/28229213562/temp/a9?compId=1223290246&revision=1",
                         "downloadUrl":"https://acercloud-lab1-1348691415.s3.amazonaws.com/15540016/8117365478537956100"}]}
    */

    cJSON2 *json_revisionList = cJSON2_GetObjectItem(json, "revisionList");
    if (!json_revisionList) {
        LOG_ERROR("revisionList missing");
        return 1;  // drop job
    }
    if (json_revisionList->type != cJSON2_Array) {
        LOG_ERROR("revisionList has unexpected value");
        return 1;  // drop job
    }
    int revisionListSize = cJSON2_GetArraySize(json_revisionList);
    u64 latestRevision = 0;
    u64 latestRevisionUpdateDevice = 0;
    for (int i = 0; i < revisionListSize; i++) {
        cJSON2 *json_revisionListEntry = cJSON2_GetArrayItem(json_revisionList, i);
        if (!json_revisionListEntry) continue;
        cJSON2 *json_revision = cJSON2_GetObjectItem(json_revisionListEntry, "revision");
        if (!json_revision) {
            LOG_ERROR("revision attr missing");
            return 1;  // drop job
        }
        if (json_revision->type != cJSON2_Number) {
            LOG_ERROR("revision attr value has wrong type");
        }
        cJSON2 *json_updateDevice = cJSON2_GetObjectItem(json_revisionListEntry, "updateDevice");
        if (!json_updateDevice) {
            LOG_ERROR("updateDevice attr missing");
            return 1;  // drop job
        }
        if (json_updateDevice->type != cJSON2_Number) {
            LOG_ERROR("updateDevice attr value has wrong type");
            return 1;  // drop job
        }
        if ((u64)json_revision->valueint > latestRevision) {
            latestRevisionUpdateDevice = (u64)json_updateDevice->valueint;
            latestRevision = (u64)json_revision->valueint;
        }
    }
    LOG_DEBUG("local deviceid "FMTu64, VirtualDevice_GetDeviceId());
    LOG_DEBUG("latestRev updateDevice "FMTu64, latestRevisionUpdateDevice);

    revision = latestRevision;

    return err;
}

int SyncDown::downloadRevision(SyncDownJob &job, u64 revision, HttpStream *hs)
{
    int err = 0;

    stream_transaction transaction;
    transaction.uid = userId;
    transaction.deviceid = VirtualDevice_GetDeviceId();  // ?? doesn't seem to be used anyway
    std::ostringstream oss;
    oss << "/vcs/file/" << job.did << "/" << VPLHttp_UrlEncoding(job.cpath, "/") + "?compId=" << job.cid << "&revision=" << revision;
    transaction.req = new http_request();
    transaction.req->uri = oss.str();
    transaction.origHs = hs;

    std::string dl_path;
    extendDlPathToFullPath(job.dl_path, workdir, dl_path);
    err = Util_CreatePath(dl_path.c_str(), VPL_FALSE);
    if (err) {
        LOG_ERROR("Failed to create directory for %s", dl_path.c_str());
        if(hs) {
            LOG_INFO("Send server response to httpstream only.");
        } else {
            return err;
        }
    }

    transaction.out_content_file = VPLFile_Open(dl_path.c_str(),
            VPLFILE_OPENFLAG_WRITEONLY|VPLFILE_OPENFLAG_CREATE|VPLFILE_OPENFLAG_TRUNCATE,
            VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR);
    if (!VPLFile_IsValidHandle(transaction.out_content_file)) {
        err = transaction.out_content_file;
        LOG_ERROR("Failed to open %s for write: %d", dl_path.c_str(), err);
        if(hs) {
            LOG_INFO("Send server response to httpstream only.");
            transaction.out_content_file = NULL;
        } else {
            return err;
        }
    }
    ON_BLOCK_EXIT(VPLFile_Close, transaction.out_content_file);

    transaction.http2 = new VPLHttp2();
    // to be destroyed by stream_transaction dtor

    // Publish VPLHttp2 object for possible cancellation.
    {
        MutexAutoLock lock(&mutex);
        httpInProgress[job.id] = transaction.http2;
    }

    // Blocking call, can take a while.
    extern int S3_getFile2HttpStreamAndFile(stream_transaction* transaction, string bucket, string filename);
    err = S3_getFile2HttpStreamAndFile(&transaction, "", "");

    // Unpublish VPLHttp2 object from possible cancellation.
    {
        MutexAutoLock lock(&mutex);
        httpInProgress.erase(job.id);
    }

    // response to httpstream successfully, but write to file fails.
    if (err == 0 && !VPLFile_IsValidHandle(transaction.out_content_file)){
        err = VPL_ERR_NOENT;
    }

    return err;
}

int SyncDown::downloadPreview(SyncDownJob &job, u64 revision, u64 feature)
{
    int err = 0;

    std::string cpath = job.cpath;

    if (feature == ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL) {
        // replace job.cpath thumbnail/ to photos/
        // This's the limitation of current design.
        // In PicStream DB, we treat full resolution, low resolution and thumbnail as
        // three individual components for consistency. So, the "name" of each components
        // for the same photo will be "photos/2014_02/20140201_120000.jpg",
        // "lowres/2014_02/20140201_120000.jpg" and "thumbnail/2014_02/20140201_120000.jpg".
        //
        // But, in current infra design, thumbnail is actually one of the properties of
        // full resolution. That's why when downloading thumbnail, we need to query VCS by
        // using full resolution "name". So, job.cpath is manipulated here.
        cpath.assign("photos");
        size_t pos = job.cpath.find_first_of("/");
        if(pos != string::npos) {
            cpath.append(job.cpath.substr(pos, string::npos));
        } else {
            LOG_ERROR("job.cpath is not thumbnail! %s", job.cpath.c_str());
            return -1;
        }
    }

    //Note: To reduce infra access, VCS path is composed by comp path and comp Id here.
    stream_transaction transaction;
    transaction.uid = userId;
    transaction.deviceid = VirtualDevice_GetDeviceId();  // ?? doesn't seem to be used anyway
    std::ostringstream oss;
    oss << "/vcs/preview/" << job.did << "/" << VPLHttp_UrlEncoding(cpath, "/") + "?compId=" << job.cid << "&revision=" << revision;
    transaction.req = new http_request();
    transaction.req->uri = oss.str();

    std::string dl_path;
    extendDlPathToFullPath(job.dl_path, workdir, dl_path);
    err = Util_CreatePath(dl_path.c_str(), VPL_FALSE);
    if (err) {
        LOG_ERROR("Failed to create directory for %s", dl_path.c_str());
        return err;
    }
    LOG_DEBUG("download Path: %s", dl_path.c_str());
    transaction.out_content_file = VPLFile_Open(dl_path.c_str(),
            VPLFILE_OPENFLAG_WRITEONLY|VPLFILE_OPENFLAG_CREATE|VPLFILE_OPENFLAG_TRUNCATE,
            VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR);
    if (!VPLFile_IsValidHandle(transaction.out_content_file)) {
        err = transaction.out_content_file;
        LOG_ERROR("Failed to open %s for write: %d", dl_path.c_str(), err);
        return err;
    }
    ON_BLOCK_EXIT(VPLFile_Close, transaction.out_content_file);

    transaction.http2 = new VPLHttp2();
    // to be destroyed by stream_transaction dtor

    // Publish VPLHttp2 object for possible cancellation.
    {
        MutexAutoLock lock(&mutex);
        httpInProgress[job.id] = transaction.http2;
    }

    // Blocking call, can take a while.
    err = VCSgetPreview(&transaction, job.did);

    // Unpublish VPLHttp2 object from possible cancellation.
    {
        MutexAutoLock lock(&mutex);
        httpInProgress.erase(job.id);
    }

    return err;
}

int SyncDown::getDatasetInfo(u64 datasetId,
                             bool& isPicstream_out,
                             bool& isSharedByMe_out,
                             bool& isSharedWithMe_out)
{
    int result = 0;
    isPicstream_out = false;
    isSharedByMe_out = false;
    isSharedWithMe_out = false;

    if (!userId) {//asdf
        return CCD_ERROR_USER_NOT_FOUND;
    }

    CacheAutoLock autoLock;
    int err = autoLock.LockForRead();
    if (err < 0) {
        LOG_ERROR("Failed to obtain lock");
        return err;
    }

    CachePlayer *player = cache_getUserByUserId(userId);
    if (!player) {
        LOG_ERROR("No user found");
        return CCD_ERROR_USER_NOT_FOUND;
    }
    std::string datasetName;
    vplex::vsDirectory::DatasetType datasetType;
    bool found = false;
    for (int i = 0; i < player->_cachedData.details().datasets_size(); i++) {
        const vplex::vsDirectory::DatasetDetail &dd = player->_cachedData.details().datasets(i).details();
        if (dd.datasetid() == datasetId) {
            datasetName.assign(dd.datasetname());
            datasetType = dd.datasettype();
            found = true;
            break;
        }
    }
    if (!found) {
        LOG_ERROR("Dataset (id="FMTu64") not found", datasetId);
        return CCD_ERROR_NOT_FOUND;
    }

    if (datasetName == "PicStream") {
        isPicstream_out = true;
    } else if (datasetType == vplex::vsDirectory::SWM) {
        isSharedWithMe_out = true;
    } else if (datasetType == vplex::vsDirectory::SBM) {
        isSharedByMe_out = true;
    }

    return result;
}

int SyncDown::getPicStreamDownloadDir(PicStreamDownloadDirEnum downloadDirType,
                                      std::string &downloadDir,
                                      u32& maxFiles,
                                      u32& maxBytes,
                                      u32& preserveFreeDiskPercentage,
                                      u64& preserveFreeDiskSizeBytes)
{
    int result = 0;
    maxFiles = 0;
    maxBytes = 0;

    if (!userId) {
        return CCD_ERROR_USER_NOT_FOUND;
    }

    CacheAutoLock autoLock;
    int err = autoLock.LockForRead();
    if (err < 0) {
        LOG_ERROR("Failed to obtain lock");
        return err;
    }

    CachePlayer *player = cache_getUserByUserId(userId);
    if (!player) {
        LOG_ERROR("No user found");
        return CCD_ERROR_USER_NOT_FOUND;
    }

    google::protobuf::RepeatedPtrField< ::ccd::CameraRollDownloadDirSpecInternal > picstreamDir;
    switch(downloadDirType) {
    case PicStreamDownloadDirEnum_FullRes:
        player->get_camera_download_full_res_dirs(picstreamDir);
        break;
    case PicStreamDownloadDirEnum_LowRes:
        player->get_camera_download_low_res_dirs(picstreamDir);
        break;
    case PicStreamDownloadDirEnum_Thumbnail:
        player->get_camera_download_thumb_dirs(picstreamDir);
        break;
    default:
        LOG_ERROR("Unrecognized case:%d", (int)downloadDirType);
        return CCD_ERROR_NOT_FOUND;
    }

    if (picstreamDir.size() == 0) {
        return CCD_ERROR_NOT_FOUND;
    }
    downloadDir.assign(picstreamDir.Get(0).dir());
    if (downloadDir.empty()) {
        return CCD_ERROR_NOT_FOUND;
    }

    maxFiles = picstreamDir.Get(0).max_files();
    maxBytes = picstreamDir.Get(0).max_size();
    preserveFreeDiskPercentage = picstreamDir.Get(0).preserve_free_disk_percentage();
    preserveFreeDiskSizeBytes = picstreamDir.Get(0).preserve_free_disk_size_bytes();

    return result;
}

int SyncDown::getSwmDownloadDir(std::string &swmDownloadDir_out)
{
    swmDownloadDir_out.clear();
    if (userId == 0) {
        return CCD_ERROR_USER_NOT_FOUND;
    }
    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForSharedWithMe(userId, sizeof(path), /*OUT*/ path);
    swmDownloadDir_out.assign(path);
    return 0;
}

int SyncDown::getSbmDownloadDir(std::string &sbmDownloadDir_out)
{
    sbmDownloadDir_out.clear();
    if (userId == 0) {
        return CCD_ERROR_USER_NOT_FOUND;
    }
    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForSharedByMe(userId, sizeof(path), /*OUT*/ path);
    sbmDownloadDir_out.assign(path);
    return 0;

}

int SyncDown::Start(u64 userId)
{
    int result = 0;
    int err = 0;
    std::string doDCacheFolder;

    MutexAutoLock lock(&mutex);

    if (jobs || thread_spawned) {
        LOG_ERROR("SyncDown obj already started");
        return CCD_ERROR_ALREADY_INIT;
        // do not jump to "end" which will stop SyncDown obj
    }

    this->userId = userId;

    err = Util_CreatePath(tmpdir.c_str(), VPL_TRUE);
    if (err) {
        LOG_ERROR("Failed to create directory for %s", tmpdir.c_str());
        result = err;
        goto end;
    }

    doDCacheFolder = tmpdir + DOD_CACHE_FOLDER; //Download on Demand cache
    err = Util_CreatePath(doDCacheFolder.c_str(), VPL_TRUE);
    if (err) {
        LOG_ERROR("Failed to create directory for %s", doDCacheFolder.c_str());
        result = err;
        goto end;
    }

    jobs = new SyncDownJobs(workdir);
    if (!jobs) {
        LOG_ERROR("Failed to allocate SyncDownJobs object");
        result = err;
        goto end;
    }

    err = jobs->Check();
    if (err) {
        LOG_ERROR("SyncDownJobs obj failed Check: %d", err);
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

int SyncDown::Stop(bool purge)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    if (jobs && purge) {
        err = jobs->RemoveAllJobs();
        if (err) {
            LOG_ERROR("Failed to remove all jobs: %d", err);
            result = err;
            // Do not return; try to clean up as much as possible.
        }
        else {
            LOG_DEBUG("purged all remaining jobs");
        }

        err = jobs->ResetAllDatasetLastCheckedVersions();
        if (err) {
            LOG_ERROR("Failed to reset last-checked dataset versions: %d", err);
            result = err;
            // Do not return; try to clean up as much as possible.
        }
        else {
            LOG_DEBUG("Reset all last-checked dataset versions");
        }

        err = jobs->RemoveAllJobDelete();
        if (err) {
            LOG_ERROR("Failed to remove all delete jobs: %d", err);
            result = err;
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
            LOG_DEBUG("SyncDownJobs obj marked to purge-on-close");
        }
    }

    {
        // Must hold lock until after cancelling VPLHttp2 objects.
        // Otherwise, dispatcher thread may destroy them after being signaled
        ASSERT(VPLMutex_LockedSelf(&mutex));
        if (thread_running) {
            err = signalDispatcherThreadToStop();
            if (err) {
                LOG_ERROR("Failed to signal dispatcher thread to stop: %d", err);
                result = err;
                // Do not return; try to clean up as much as possible.
            }
            // dispatch thread will destroy SyncDownJobs obj and SyncDown obj and then exit.
        }

        std::map<u64, VPLHttp2*>::iterator it;
        for (it = httpInProgress.begin(); it != httpInProgress.end(); it++) {
            it->second->Cancel();
        }
    }

    if (purge) {//userLogout
        std::string cachePath = workdir + "tmp/"DOD_CACHE_FOLDER;
        std::string toDelPath = workdir + "tmp/to_delete";
        size_t pos = cachePath.rfind("/");
        cachePath.replace(pos, 1, "");
        LOG_INFO("Remove DoD Cache. Path: %s", cachePath.c_str());
        err = Util_rmRecursive(cachePath.c_str(), toDelPath.c_str());
        if(err != 0) {
            LOG_ERROR("Util_rm_dash_rf:%d, %s", err, cachePath.c_str());
            result = err;
        }
    }//if (purge)

    return result;
}

int SyncDown::NotifyDatasetContentChange(u64 datasetid,
                                         bool myOverrideAnsCheck, bool syncDownAdded_, int syncDownFileType_)
{
    int result = 0;
    int err = 0;

    LOG_DEBUG("received for did "FMTu64, datasetid);

    MutexAutoLock lock(&mutex);

    overrideAnsCheck = myOverrideAnsCheck;
    syncDownAdded = syncDownAdded_;
    syncDownFileType = syncDownFileType_;
    if (dataset_changes_set.find(datasetid) == dataset_changes_set.end()) {
        dataset_changes_queue.push(datasetid);
        dataset_changes_set.insert(datasetid);
        LOG_DEBUG("added did "FMTu64, datasetid);
    }
    else {
        LOG_DEBUG("ignored duplicate did "FMTu64, datasetid);
    }
    err = VPLCond_Signal(&cond);
    if (err) {
        LOG_ERROR("Failed to signal condvar: %d", err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncDown::ResetDatasetLastCheckedVersion(u64 datasetId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = jobs->ResetDatasetLastCheckedVersion(datasetId);
    if (err == CCD_ERROR_NOT_FOUND) {
        LOG_WARN("Cannot find the Dataset Id, "FMTu64", in DB", datasetId);
        result = err;
        goto end;
    } else if (err) {
        LOG_ERROR("Failed to reset last-checked version for dataset "FMTu64": %d", datasetId, err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncDown::RemoveJobsByDatasetId(u64 datasetId)
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

int SyncDown::QueryStatus(u64 myUserId,
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
    // Bug 7427:  This isn't accurate (different datasets cause SYNCING STATUS),
    //     but it's a minimal change to make user spinning icon display
    //     more friendly for the short term.  This should be corrected in the future.
    if(syncState_out.status()==ccd::CCD_FEATURE_STATE_OUT_OF_SYNC && isDoingWork) {
        syncState_out.set_status(ccd::CCD_FEATURE_STATE_SYNCING);
    }
    return 0;
}

int SyncDown::QueryStatus(u64 myUserId,
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
    if(it!=currentSyncStatus.end()) {
        syncState_out.set_status((ccd::FeatureSyncStateType_t)(it->second));
    } else {
        LOG_ERROR("Cannot find feature: %d", (int)syncFeature);
        return CCD_ERROR_NOT_FOUND;
    }

    u32 pendingJobCounts = 0;
    u32 failedJobCounts = 0;
    if (ccd::SYNC_FEATURE_PICSTREAM_DELETION == syncFeature) {
        err = jobs->GetDeleteJobCount(cutoff_ts, pendingJobCounts, failedJobCounts);
    } else {
        err = jobs->GetJobCountByFeature(syncFeature, cutoff_ts, pendingJobCounts, failedJobCounts);
    }
    if(err) {
        // Error when get job counts
        LOG_ERROR("Error when getting job counts!, feature: %d, err: %d", (int)syncFeature, err);
        syncState_out.set_pending_files(0);
        syncState_out.set_failed_files(0);
        return err;
    } else {
        syncState_out.set_pending_files(pendingJobCounts);
        syncState_out.set_failed_files(failedJobCounts);
    }
    return 0;
}

int SyncDown::DownloadOnDemand(SyncDownJob &job, HttpStream *hs)
{
    int err = 0;
    std::string dl_fullpath;

    LOG_INFO("Download photo name:%s compId: "FMTu64" type:"FMTu64, job.cpath.c_str(), job.cid, job.feature);

    extendDlPathToFullPath(job.dl_path, workdir, job.lpath);
    // use a temp file to garantee that the final file is available when it's ready.
    // Or, the caller may receive a partial or cracked file.
    job.dl_path += ".tmp";
    extendDlPathToFullPath(job.dl_path, workdir, dl_fullpath);

    if(job.feature == ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL) {
        err = downloadPreview(job, job.dl_rev, job.feature);
    } else {
        err = downloadRevision(job, job.dl_rev, hs);
    }

    if(err != 0) {
        if (VPL_ERR_NOENT == err) {
            LOG_ERROR("Response to httpstream successfully, but writing to file failed. Try to delete cache file.");
        } else {
            LOG_ERROR("Download failed. Try to delete cache file.");
        }
        int result;
        int rv = VPLFile_Delete(dl_fullpath.c_str());
        result = rv;
        if(rv != VPL_OK) {
            VPLFS_stat_t stat;
            rv = VPLFS_Stat(dl_fullpath.c_str(), &stat);
            if(rv != VPL_OK) {
                LOG_WARN("File %s does not exist. Count as success.", dl_fullpath.c_str());
            } else {
                LOG_ERROR("Deletion of %s failed:%d.", dl_fullpath.c_str(), result);
            }
        }

        job.lpath = "";
        return err;

    } else {
#ifndef CLOUDNODE
        err = VPLFile_Rename(dl_fullpath.c_str(), job.lpath.c_str());
        if (err != 0) {
            LOG_ERROR("Failed to rename \"%s\" to \"%s\": %d",
                    dl_fullpath.c_str(), job.lpath.c_str(), err);
            job.lpath = "";

            VPLFile_Delete(dl_fullpath.c_str());
        }
#else
        do {
            SyncDownDstStor sd3;
            err = sd3.init(userId);
            if (err) {
                LOG_ERROR("Cancel file copy from \"%s\" to \"%s\" Failed to init SyncDownDstStor: %d",
                                                   dl_fullpath.c_str(), job.lpath.c_str(),err);
                VPLFile_Delete(dl_fullpath.c_str());
                break;
            }
        
            err = sd3.createMissingAncestorDirs(job.lpath);
            if (err) {
                LOG_ERROR("Cancel file copy. Failed to create ancestor dirs for %s: %d", job.lpath.c_str(), err);
                VPLFile_Delete(dl_fullpath.c_str());
                break;
            }
        
            err = sd3.copyFile(dl_fullpath, job.lpath);
            if (err) {
                LOG_ERROR("Failed to copy file from %s to %s: %d", dl_fullpath.c_str(), job.lpath.c_str(), err);
                VPLFile_Delete(dl_fullpath.c_str());
                break;
            }
        } while(0);
#endif

    }

    LOG_DEBUG("Download Path: %s", job.lpath.c_str());

    return err;
}

//copy from getLatestRevNum
int SyncDown::getLatestMetadata(const std::string& name,
                                u64 compId,
                                u64 datasetId,
                                u64 &origDev_out,
                                u64 &rev_out,
                                u64 &fileSize_out,
                                std::string &dlUrl_out,
                                std::string &previewUrl_out)
{
    int err = 0;
    std::string uri;
    {
        std::ostringstream oss;
        oss << "/vcs/filemetadata/" << datasetId << "/" << VPLHttp_UrlEncoding(name, "/") << "?compId=" << compId;
        uri.assign(oss.str());
    }
    std::string metadata;
    err = VCSreadFile_helper(userId, datasetId, "", "", VirtualDevice_GetDeviceId(), uri, NULL, metadata);
    if (err==CCD_ERROR_HTTP_STATUS ||
        err==CCD_ERROR_BAD_SERVER_RESPONSE)
    {
        // http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_API_Client_Side_Error_Handling#CCD_calling_VCS-v1-API_error_handling
        LOG_WARN("File assumed expired:%s", name.c_str());
        return 1;
    }else if (err) {
        LOG_ERROR("Failed to get file metadata from VCS: %d", err);
        return err;
    }
    LOG_INFO("=> %s",metadata.c_str());
    cJSON2 *json = cJSON2_Parse(metadata.c_str());
    if (!json) {
        LOG_ERROR("Failed to parse response: %s", metadata.c_str());
        return 1;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json);

    /* sample response
       {"name":"28229213562/temp/a9",
        "compId":1223290246,
        "numOfRevisions":1,
        "originDevice":28229213562,
        "revisionList":[{"revision":1,
                         "size":65536,
                         "lastChanged":1351645142,
                         "updateDevice":28229213562,
                         "previewUri":"/clouddoc/preview/15540016/28229213562/temp/a9?compId=1223290246&revision=1",
                         "downloadUrl":"https://acercloud-lab1-1348691415.s3.amazonaws.com/15540016/8117365478537956100"}]}
    */

    cJSON2 *json_revisionList = cJSON2_GetObjectItem(json, "revisionList");
    if (!json_revisionList) {
        LOG_ERROR("revisionList missing");
        return 1;
    }
    if (json_revisionList->type != cJSON2_Array) {
        LOG_ERROR("revisionList has unexpected value");
        return 1;
    }

    cJSON2 *json_origDevice = cJSON2_GetObjectItem(json, "originDevice");
    if (!json_origDevice) {
        LOG_ERROR("originDevice attr missing");
        return 1;
    }
    if (json_origDevice->type != cJSON2_Number) {
        LOG_ERROR("originDevice attr value has wrong type");
        return 1;
    }
    origDev_out = (u64)json_origDevice->valueint;

    int revisionListSize = cJSON2_GetArraySize(json_revisionList);
    rev_out = 0;
    for (int i = 0; i < revisionListSize; i++) {
        cJSON2 *json_revisionListEntry = cJSON2_GetArrayItem(json_revisionList, i);
        if (!json_revisionListEntry) continue;
        cJSON2 *json_revision = cJSON2_GetObjectItem(json_revisionListEntry, "revision");
        if (!json_revision) {
            LOG_ERROR("revision attr missing");
            return 1;
        }
        if (json_revision->type != cJSON2_Number) {
            LOG_ERROR("revision attr value has wrong type");
            return 1;
        }

        cJSON2 *size = cJSON2_GetObjectItem(json_revisionListEntry, "size");
        if (!size) {
            LOG_ERROR("size attr missing");
            return 1;
        }
        if (size->type != cJSON2_Number) {
            LOG_ERROR("size attr value has wrong type");
            return 1;
        }

        cJSON2 *previewUri = cJSON2_GetObjectItem(json_revisionListEntry, "previewUri");
        if (!previewUri) {
            LOG_ERROR("previewUri missing");
            return 1;
        }
        if (previewUri->type != cJSON2_String) {
            LOG_ERROR("previewUri in unexpected format");
            return 1;
        }

        cJSON2 *downloadUrl = cJSON2_GetObjectItem(json_revisionListEntry, "downloadUrl");
        if (!downloadUrl) {
            LOG_ERROR("downloadUrl missing");
            return 1;
        }
        if (downloadUrl->type != cJSON2_String) {
            LOG_ERROR("downloadUrl in unexpected format");
            return 1;
        }

        if ((u64)json_revision->valueint > rev_out) {
            rev_out = (u64)json_revision->valueint;
            fileSize_out = (u64) size->valueint;
            dlUrl_out = downloadUrl->valuestring;
            previewUrl_out = previewUri->valuestring;
        }
    }
    LOG_DEBUG("Lastest version: "FMTu64, rev_out);

    return err;
}

void SyncDown::setEnableGlobalDelete(bool enable)
{
    globalDeleteEnabled = enable;
}

bool SyncDown::getEnableGlobalDelete()
{
    return globalDeleteEnabled;
}
//----------------------------------------------------------------------

class OneSyncDown {
public:
    OneSyncDown() {
        VPLMutex_Init(&mutex);
    }
    ~OneSyncDown() {
        VPLMutex_Destroy(&mutex);
    }
    SyncDown *obj;
    VPLMutex_t mutex;
};
static OneSyncDown oneSyncDown;

int SyncDown_Start(u64 userId)
{
    int result = 0;

    MutexAutoLock lock(&oneSyncDown.mutex);

    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForSyncDown(userId, sizeof(path), path);
    // assert: path ends in '/'

    oneSyncDown.obj = new SyncDown(path);

    int err = oneSyncDown.obj->Start(userId);
    if (err) {
        LOG_ERROR("Failed to start SyncDown: %d", err);
        result = err;
        oneSyncDown.obj = NULL;
        goto end;
    }

 end:
    return result;
}

int SyncDown_Stop(bool purge)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncDown.mutex);

    if (!oneSyncDown.obj) {
        LOG_WARN("SyncDown never started");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncDown.obj->Stop(purge);
    if (err) {
        // err msg printed by SyncDown::Stop()
        result = err;
        goto end;
    }

 end:
    oneSyncDown.obj = NULL;  // Remove reference even if Stop() fails because there's nothing more we can do about it.
    return result;
}

int SyncDown_NotifyDatasetContentChange(u64 datasetId,
                                        bool overrideAnsCheck, bool syncDownAdded, int syncDownFileType)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncDown.mutex);

    if (!oneSyncDown.obj) {
        LOG_WARN("SyncDown not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncDown.obj->NotifyDatasetContentChange(datasetId,
                                                      overrideAnsCheck, syncDownAdded, syncDownFileType);
    if (err) {
        LOG_ERROR("Failed to notify SyncDown of dataset content change: %d", err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncDown_ResetDatasetLastCheckedVersion(u64 datasetId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncDown.mutex);

    if (!oneSyncDown.obj) {
        LOG_WARN("SyncDown not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncDown.obj->ResetDatasetLastCheckedVersion(datasetId);
    if (err == CCD_ERROR_NOT_FOUND) {
        LOG_WARN("Cannot find the Dataset Id, "FMTu64", in DB", datasetId);
        result = err;
        goto end;
    } else if (err) {
        LOG_ERROR("Failed to reset last checked version for dataset "FMTu64": %d", datasetId, err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncDown_RemoveJobsByDataset(u64 datasetId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncDown.mutex);

    if (!oneSyncDown.obj) {
        LOG_WARN("SyncDown not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    LOG_DEBUG("did "FMTu64, datasetId);

    err = oneSyncDown.obj->RemoveJobsByDatasetId(datasetId);
    if (err) {
        LOG_ERROR("Failed to remove jobs for dataset "FMTu64": %d", datasetId, err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int SyncDown_QueryStatus(u64 userId,
                         u64 datasetId,
                         const std::string& path,
                         ccd::FeatureSyncStateSummary& syncState_out)
{
    int rv = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncDown.mutex);

    if (!oneSyncDown.obj) {
        LOG_WARN("SyncDown not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncDown.obj->QueryStatus(userId,
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

int SyncDown_QueryStatus(u64 userId,
                         ccd::SyncFeature_t syncFeature,
                         ccd::FeatureSyncStateSummary& syncState_out)
{
    int rv = 0;
    int err = 0;

    MutexAutoLock lock(&oneSyncDown.mutex);

    if (!oneSyncDown.obj) {
        LOG_WARN("SyncDown not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneSyncDown.obj->QueryStatus(userId,
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

int SyncDown_QueryPicStreamItems(ccd::PicStream_DBFilterType_t filter_type,
                                const std::string& searchField,
                                const std::string& sortOrder,
                                google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output)
{
    return oneSyncDown.obj->QueryPicStreamItems(filter_type, searchField, sortOrder, output);
}

int SyncDown_QuerySharedFilesItems(ccd::SyncFeature_t sync_feature,
        const std::string& searchField,
        const std::string& sortOrder,
        google::protobuf::RepeatedPtrField<ccd::SharedFilesQueryObject> &output)
{
    return oneSyncDown.obj->QuerySharedFilesItems(sync_feature,
                                                  searchField,
                                                  sortOrder,
                                                  /*OUT*/ output);
}

int SyncDown_QuerySharedFilesEntryByCompId(u64 compId,
                                           u64 syncFeature,
                                           u64& revision_out,
                                           u64& datasetId_out,
                                           std::string& name_out,
                                           bool& thumbDownloaded_out,
                                           std::string& thumbRelPath_out)
{
    return oneSyncDown.obj->
            QuerySharedFilesEntryByCompId(compId,
                                          syncFeature,
                                          /*OUT*/ revision_out,
                                          /*OUT*/ datasetId_out,
                                          /*OUT*/ name_out,
                                          /*OUT*/ thumbDownloaded_out,
                                          /*OUT*/ thumbRelPath_out);
}

void getCacheFilePath(u64 datasetId, u64 compId, u64 fileType, std::string compPath, /*OUT*/ std::string &cacheFile)
{
    stringstream oss;
    size_t pos;
    pos = compPath.rfind('/');
    oss << "tmp/"DOD_CACHE_FOLDER << datasetId << "_" << compId << "_" << fileType << "_" << compPath.substr(pos+1, string::npos);
    cacheFile.assign(oss.str());
}

void SyncDown_GetCacheFilePath(VPLUser_Id_t userId,
                               u64 datasetId,
                               u64 compId,
                               u64 fileType,
                               const std::string& compPath,
                               /*OUT*/ std::string &cacheFile)
{
    VPLFS_stat_t stat;
    std::string filePath;
    char workPath[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForSyncDown(userId, sizeof(workPath), workPath);

    getCacheFilePath(datasetId, compId, fileType, compPath, filePath);
    cacheFile.assign(workPath);
    cacheFile.append(filePath);

    LOG_INFO("cachefile path %s", cacheFile.c_str());
    int err = VPLFS_Stat(cacheFile.c_str(), &stat);
    if (err) {
        cacheFile = "";
        LOG_INFO("cachefile path error!");
    }
}

int SyncDown_DownloadOnDemand(u64 datasetId,
                              const std::string& compPath,
                              u64 compId,
                              u64 fileType,
                              HttpStream *hs,
                              /*OUT*/std::string &filePath)
{
    SyncDownJob job;
    u64 curTime = VPLTime_GetTime();
    std::string dl_path;
    getCacheFilePath(datasetId, compId, fileType, compPath, dl_path);

    job.id = curTime;
    job.did = datasetId;
    job.cpath = compPath;
    job.cid = compId;
    job.dl_path = dl_path;
    job.dl_rev = 1;

    switch (fileType) {
        case FileType_FullRes:
            job.feature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES;
        break;
        case FileType_LowRes:
            job.feature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES;
        break;
        case FileType_Thumbnail:
            job.feature = ccd::SYNC_FEATURE_PICSTREAM_DOWNLOAD_THUMBNAIL;
        break;
        default:
            LOG_ERROR("file type:"FMTu64" is wrong.", fileType);
            return -1;
        break;
    }

    if(hs == NULL){
        LOG_ERROR("httpStream should not be NULL!");
    }

    int err = oneSyncDown.obj->DownloadOnDemand(job, hs);

    filePath = job.lpath;
    return err;
}

int SyncDown_AddPicStreamItem(  u64 compId, const std::string &name, u64 rev, u64 origCompId, u64 originDev,
                                const std::string &identifier, const std::string &title, const std::string &albumName,
                                u64 fileType, u64 fileSize, u64 takenTime, const std::string &lpath, /*OUT*/ u64 &jobid, /*OUT*/ bool &syncUpAdded)
{
    LOG_INFO("Add data from SyncUp %s", lpath.c_str());
    return oneSyncDown.obj->AddPicStreamItem(compId, name, rev, origCompId, originDev, identifier, title, albumName,
                                                    fileType, fileSize, takenTime, lpath, /*OUT*/ jobid, /*OUT*/ syncUpAdded);
}

void SyncDown_SetEnableGlobalDelete(bool enable)
{
    oneSyncDown.obj->setEnableGlobalDelete(enable);
}

int SyncDown_GetEnableGlobalDelete(bool &enable)
{
    if(oneSyncDown.obj) {
        enable = oneSyncDown.obj->getEnableGlobalDelete();
        return 0;
    } else {
        return CCD_ERROR_NOT_INIT;
    }
    return 0;
}

int SyncDown_QueryPicStreamItems( const std::string& searchField,
                                const std::string& sortOrder,
                                std::queue< PicStreamPhotoSet > &output
                                )
{
    return oneSyncDown.obj->QueryPicStreamItems(searchField, sortOrder, output);
}

///////////////////////////////////////////////////////////////////////
////////////////// Picstream Download Specific functions //////////////

struct PicStreamPhoto {
    std::string filename;
    std::string directory;
    u64 size;

    PicStreamPhoto(const std::string& myFilename,
                   const std::string& myDirectory,
                   u64 mySize)
        :  filename(myFilename),
           directory(myDirectory),
           size(mySize)
    {}
};

// returns true if valid PicStream filename, else returns false.
// photo file name is 'yyyyMMdd_HHmmss[_n].ext', n being an integer, "_n" not present
static bool isValidPicStreamFilename(const PicStreamPhoto& photo,
                                     std::string& nameTime_out,
                                     int& nValue_out)
{
    static const std::string requiredPart("DDDDDDDD_DDDDDD");
    static const u32 EXTENSION_LEN_1_MIN = 3; // ext, also minimum length
    static const u32 EXTENSION_LEN_2 = 4;
    const u32 MIN_NAME_LEN = requiredPart.size() + 1 /* period */ + EXTENSION_LEN_1_MIN;

    nameTime_out.clear();
    nValue_out = -2;

    if(photo.filename.size() < MIN_NAME_LEN) {
        return false;
    }
    u32 charIndex = 0;
    for(;charIndex < requiredPart.size();++charIndex) {
        if(requiredPart[charIndex] == 'D') {
            if(isdigit(photo.filename[charIndex])==0) {
                return false;
            }
        }else{
            if(photo.filename[charIndex] != requiredPart[charIndex]) {
                return false;
            }
        }
    }

    if(photo.filename[charIndex] == '.') {
        // continue on to extension checking
        nameTime_out = photo.filename.substr(0, charIndex);
        nValue_out = -1;
        charIndex++;  // increment to first extension character
    }else if(photo.filename[charIndex] == '_') {
        // check for [_n]
        nameTime_out = photo.filename.substr(0, charIndex);
        charIndex++;
        std::size_t periodIdx = photo.filename.find_first_of('.', charIndex);
        int nStart = charIndex;
        if(periodIdx == std::string::npos) {
            return false;
        }
        if(periodIdx-requiredPart.size() > 10) {
            // sanity check
            return false;
        }
        for(;charIndex<periodIdx;++charIndex) {
            if(isdigit(photo.filename[charIndex])==0) {
                return false;
            }
        }
        nValue_out = atoi(photo.filename.substr(nStart, periodIdx-nStart).c_str());
        // should be at period
        charIndex++;  // increment to first extension character
    }else{
        // Not recognized, next character should be '.' or '_'
        return false;
    }

    // Extension checking.
    std::size_t extraPeriods = photo.filename.find_first_of('.', charIndex);
    if(extraPeriods != std::string::npos) {
        // extra periods found.
        return false;
    }

    int extensionSize = photo.filename.size() - charIndex;
    if(extensionSize == EXTENSION_LEN_1_MIN || extensionSize == EXTENSION_LEN_2) {
        return true;
    }
    return false;
}

// most recent to oldest.
// return false when we want to swap
static bool PicStreamPhotoComparator(const PicStreamPhoto &lhs,
                                     const PicStreamPhoto &rhs)
{
    std::string lhsNameTime;
    std::string rhsNameTime;
    int lhsNValue = 0;
    int rhsNValue = 0;
    bool lhsValid;
    bool rhsValid;

    lhsValid = isValidPicStreamFilename(lhs, lhsNameTime, lhsNValue);
    rhsValid = isValidPicStreamFilename(rhs, rhsNameTime, rhsNValue);

    if(lhsValid == true && rhsValid == true) {
        // Order we want valid photos in happens to be in ascii order
        // since '_' char is greater than '.' char.
        // Because "_n" part of the file has no leading zeros, special logic is needed.
        if(lhsNameTime == rhsNameTime) {
            if(lhsNValue >= rhsNValue) {
                return true;
            }else{
                return false;
            }
        }

        if(lhs.filename.compare(rhs.filename) > 0) {
            return true;
        }else{
            return false;
        }
    }else if(lhsValid == false && rhsValid == false) {
        if(lhs.filename.compare(rhs.filename) > 0) {
            return true;
        }else{
            return false;
        }
    }

    // lhsValid != rhsValid.
    if(lhsValid){
        return true;
    }else{
        return false;
    }
}

#ifdef ANDROID
//FIXME: maybe need to move into vpl layer
static int isExternalStorage(const std::string& path)
{
    LOG_DEBUG("enter isExternalStorage, %s", path.c_str());

    std::string internalPath;
    int rv = VPL_GetExternalStorageDirectory(internalPath);
    if(rv != VPL_OK)
        LOG_ERROR("VPL_GetExternalStorageDirectory failed: %d", rv);
    LOG_INFO("internalPath: %s", internalPath.c_str());

    FILE* file = fopen("/proc/mounts", "r");
    if(!file){
        LOG_ERROR("Fail to read /proc/mounts");
        return CCD_ERROR_NOT_FOUND;
    }

    ON_BLOCK_EXIT(fclose, file);

    /*
        * /dev/block/vold/179:17 /storage/extSdCard vfat rw,....
        * Regular expression:
        *  1. (?i) set case insensitive flag.
        *  2. include vold
        *  3. after 2, include vfat or ntfs or exfat or fat32
        *  4. after 3, include rw
        */

    char buf[256];
    std::set<std::string> result;

    while(fgets(buf, 256, file) != NULL){
        std::string s(buf);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if(s.find("asec") != std::string::npos)
            continue;
        if(s.find("staging") != std::string::npos)
            continue;
        size_t found1 = s.find("vold");
        if(found1 != std::string::npos){
            size_t found2;
            found1 += 4;
            found2 = s.find("vfat", found1);
            if(found2 == std::string::npos) found2 = s.find("ntfs", found1);
            if(found2 == std::string::npos) found2 = s.find("exfat", found1);
            if(found2 == std::string::npos) found2 = s.find("fat32", found1);

            if(found2 != std::string::npos){
                size_t found3;
                if(s[found2] == 'v' || s[found2] == 'n')
                    found2 += 4;
                else
                    found2 += 5;

                found3 = s.find("rw", found2);
                if(found3 != std::string::npos){
                    LOG_DEBUG("vold found, %s", s.c_str());
                    std::istringstream iss(buf);
                    std::vector<std::string> tokens;
                    //split buf with space, save into tokens
                    std::copy(std::istream_iterator<std::string>(iss),
                        std::istream_iterator<std::string>(),
                        std::back_inserter<std::vector<std::string> >(tokens));
                    for(int i=0; i<tokens.size(); i++){
                        std::string token = tokens[i];
                        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
                        if(token[0] == '/' && token.find("vold") == std::string::npos){
                            //We will treat the path we get from android getExternalStorageDirectory API as internal
                            if(tokens[i] == internalPath){
                                LOG_DEBUG("Skip internalPath, %s", internalPath.c_str());
                                break;
                            }
                            result.insert(tokens[i]);
                            break;
                        }
                    }

                }

            }
        }
    }

    std::set<std::string>::iterator it;

    for(it=result.begin(); it!=result.end(); it++){
        LOG_DEBUG("mount: %s", (*it).c_str());
        if(path.find((*it).c_str(), 0, (*it).size()) != std::string::npos){
            LOG_INFO("path is in external storage: %s", path.c_str());
            return CCD_OK;
        }
    }

    return CCD_ERROR_NOT_FOUND;
}
#endif //ANDROID


static int PicstreamDown_RemoveOldPhotosOverLimits(u64 userId,
                                                   const std::string& directory,
                                                   u64 fileThreshold,
                                                   u64 bytesThreshold,
                                                   u32 preserveFreeDiskPercentage,
                                                   u64 preserveFreeDiskSizeBytes)
{
    if(fileThreshold == 0 &&
        bytesThreshold == 0 &&
        preserveFreeDiskPercentage == 0 &&
        preserveFreeDiskSizeBytes == 0) {

        return 0;
    }

    std::vector<PicStreamPhoto> photos;
    u64 totalBytes = 0;

    int rv = 0;
    int rc = 0;
    VPLFS_stat_t statBufOne;
    rc = VPLFS_Stat(directory.c_str(), &statBufOne);
    if(rc != VPL_OK) {
        LOG_WARN("Picstream directory %s, %d does not exist. Count as success.",
                 directory.c_str(), rc);
        return 0;
    }

    std::vector<std::string> dirPaths;
    if (statBufOne.type == VPLFS_TYPE_FILE) {
        LOG_WARN("Picstream directory %s, %d is a file. Count as success.",
                 directory.c_str(), rc);
       return 0;
    }else{
        dirPaths.push_back(directory);
    }

    while(!dirPaths.empty()) {
        VPLFS_dir_t dp;
        VPLFS_dirent_t dirp;
        std::string currDir(dirPaths.back());
        dirPaths.pop_back();

        if((rc = VPLFS_Opendir(currDir.c_str(), &dp)) != VPL_OK) {
            LOG_ERROR("Error(%d) opening %s",
                      rc, currDir.c_str());
            rv = rc;
            continue;
        }

        while(VPLFS_Readdir(&dp, &dirp) == VPL_OK) {
            std::string dirent(dirp.filename);
            std::string absFile;
            Util_appendToAbsPath(currDir, dirent, absFile);
            VPLFS_stat_t statBuf;

            if(dirent == "." || dirent == "..") {
                continue;
            }

            if((rc = VPLFS_Stat(absFile.c_str(), &statBuf)) != VPL_OK) {
                LOG_ERROR("Error(%d) using stat on (%s,%s), type:%d",
                          rc, currDir.c_str(), dirent.c_str(), (int)dirp.type);
                rv = rc;
                continue;
            }
            if(statBuf.type == VPLFS_TYPE_FILE) {
                u64 bytes = statBuf.size;
                totalBytes += statBuf.size;
                photos.push_back(PicStreamPhoto(dirent, currDir, bytes));
            } else if(statBuf.type == VPLFS_TYPE_DIR){
                dirPaths.push_back(absFile);
            } else {
                LOG_ERROR("Strange file type %s: %d",
                          absFile.c_str(), (int)statBuf.type);
            }
        }
        VPLFS_Closedir(&dp);
    }

    std::sort(photos.begin(), photos.end(), PicStreamPhotoComparator);

    if(fileThreshold != 0) {
        // Enforce file threshold
        while(photos.size() > fileThreshold)
        {
            std::string fileToDelete;
            Util_appendToAbsPath(photos.back().directory,
                                 photos.back().filename,
                                 fileToDelete);
            totalBytes -= photos.back().size;
            LOG_INFO("Deleting %s: over file threshold:%d > "FMTu64,
                     fileToDelete.c_str(), (int)photos.size(), fileThreshold);
            photos.pop_back();

            rc = VPLFile_Delete(fileToDelete.c_str());
            if(rc != 0) {
                LOG_ERROR("Delete of %s failed:%d. Continuing.", fileToDelete.c_str(), rc);
                rv = rc;
            }
        }
    }

    if(bytesThreshold != 0) {
        //Enforce bytes threshold
        while(totalBytes > bytesThreshold) {
            std::string fileToDelete;
            Util_appendToAbsPath(photos.back().directory,
                                 photos.back().filename,
                                 fileToDelete);
            totalBytes -= photos.back().size;
            LOG_INFO("Deleting %s, sizeBytes:"FMTu64": over bytes threshold:"FMTu64" > "FMTu64,
                     fileToDelete.c_str(),
                     photos.back().size,
                     totalBytes,
                     bytesThreshold);

            photos.pop_back();
            rc = VPLFile_Delete(fileToDelete.c_str());
            if(rc != 0) {
                LOG_ERROR("Delete of %s failed:%d. Continuing.", fileToDelete.c_str(), rc);
                rv = rc;
            }
        }
    }

#ifdef ANDROID

    if(isExternalStorage(directory) == CCD_OK){
        LOG_INFO("isExternalStorage is true, skip checking disk space");
        return rv;
    }

    if(preserveFreeDiskPercentage !=0 && preserveFreeDiskSizeBytes !=0){
        //Determine the smaller one and use it
        int err = VPL_OK;
        u64 disk_size = 0;
        u64 avail_size = 0;
        u64 preserve_size = 0;

        err = VPLFS_GetSpace(directory.c_str(), &disk_size, &avail_size);
        if(err != VPL_OK) {
            LOG_ERROR("Failed to get disk space for partition of %s, err %d",
                    directory.c_str(), err);
            rv = err;
        }else{
            preserve_size = disk_size * 0.01 * preserveFreeDiskPercentage;
            if(preserveFreeDiskSizeBytes <= preserve_size){
                LOG_INFO("Use preserveFreeDiskSizeBytes only, "FMTu64, preserveFreeDiskSizeBytes);
                preserveFreeDiskPercentage = 0;
            }else{
                LOG_INFO("Use preserveFreeDiskPercentage only, %d, "FMTu64, preserveFreeDiskPercentage, preserve_size);
                preserveFreeDiskSizeBytes = 0;
            }

        }
    }

    if(preserveFreeDiskPercentage != 0) {
        int err = VPL_OK;
        u64 disk_size = 0;
        u64 avail_size = 0;
        u64 preserve_size = 0;
        err = VPLFS_GetSpace(directory.c_str(), &disk_size, &avail_size);
        if(err != VPL_OK) {
            LOG_ERROR("Failed to get disk space for partition of %s, err %d",
                    directory.c_str(), err);
            rv = err;
        }else{
            preserve_size = disk_size * 0.01 * preserveFreeDiskPercentage;

            while(avail_size < preserve_size){
                if(photos.size() == 0){
                    LOG_WARN("No photos available for delete");
                    break;
                }
                std::string fileToDelete;
                Util_appendToAbsPath(photos.back().directory,
                        photos.back().filename,
                        fileToDelete);

                LOG_INFO("Deleting %s, sizeBytes:"FMTu64": lower percentage threshold:"FMTu64" < "FMTu64 ", %d percent",
                        fileToDelete.c_str(),
                        photos.back().size,
                        avail_size,
                        preserve_size,
                        preserveFreeDiskPercentage);

                rc = VPLFile_Delete(fileToDelete.c_str());
                if(rc != 0) {
                    LOG_ERROR("Delete of %s failed:%d. Continuing.", fileToDelete.c_str(), rc);
                    rv = rc;
                }else{
                    avail_size += photos.back().size;
                }
                photos.pop_back();
                //Do we need to get avail space again, in case some other app is writing disk...
                //Generate EventPicStreamStorageConservation is_dropping_mode=true
                do{
                    CacheAutoLock autoLock;
                    int err = autoLock.LockForRead();
                    if (err < 0) {
                        LOG_ERROR("Failed to obtain lock");
                        rv = err;
                        break;
                    }
                    CachePlayer *player = cache_getUserByUserId(userId);
                    if (!player) {
                        LOG_ERROR("No user found");
                        rv = CCD_ERROR_NOT_FOUND;
                    }else{
                        if(!player->_cachedData.details().picstream_storage_conservation_dropping_mode()){
                            //since we are in deleting loop now, generate the event
                            player->_cachedData.mutable_details()->set_picstream_storage_conservation_dropping_mode(true);

                            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
                            ccdiEvent->mutable_picstream_storage_conservation()->set_is_dropping_mode(true);
                            ccdiEvent->mutable_picstream_storage_conservation()->set_free_disk_size_bytes(avail_size);
                            EventManagerPb_AddEvent(ccdiEvent);
                        }
                    }
                }while(0);
            }
            //Generate EventPicStreamStorageConservation is_dropping_mode=false
            do{
                CacheAutoLock autoLock;
                int err = autoLock.LockForRead();
                if (err < 0) {
                    LOG_ERROR("Failed to obtain lock");
                    rv = err;
                    break;
                }
                CachePlayer *player = cache_getUserByUserId(userId);
                if (!player) {
                    LOG_ERROR("No user found");
                    rv = CCD_ERROR_NOT_FOUND;
                }else{
                    if(player->_cachedData.details().picstream_storage_conservation_dropping_mode()){
                        //generate event if avail size over 10% of preserve size
                        if(avail_size > preserve_size*1.1){
                            player->_cachedData.mutable_details()->set_picstream_storage_conservation_dropping_mode(false);

                            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
                            ccdiEvent->mutable_picstream_storage_conservation()->set_is_dropping_mode(false);
                            ccdiEvent->mutable_picstream_storage_conservation()->set_free_disk_size_bytes(avail_size);
                            EventManagerPb_AddEvent(ccdiEvent);
                        }
                    }
                }
            }while(0);
        }
    }

    if(preserveFreeDiskSizeBytes != 0) {
        int err = VPL_OK;
        u64 disk_size = 0;
        u64 avail_size = 0;
        err = VPLFS_GetSpace(directory.c_str(), &disk_size, &avail_size);
        if(err != VPL_OK) {
            LOG_ERROR("Failed to get disk space for partition of %s, err %d",
                    directory.c_str(), err);
            rv = err;
        }else{

            while(avail_size < preserveFreeDiskSizeBytes){
                if(photos.size() == 0){
                    LOG_WARN("No photos available for delete");
                    break;
                }
                std::string fileToDelete;
                Util_appendToAbsPath(photos.back().directory,
                        photos.back().filename,
                        fileToDelete);

                LOG_INFO("Deleting %s, sizeBytes:"FMTu64": lower bytes threshold:"FMTu64" < "FMTu64,
                        fileToDelete.c_str(),
                        photos.back().size,
                        avail_size,
                        preserveFreeDiskSizeBytes);

                rc = VPLFile_Delete(fileToDelete.c_str());
                if(rc != 0) {
                    LOG_ERROR("Delete of %s failed:%d. Continuing.", fileToDelete.c_str(), rc);
                    rv = rc;
                }else{
                    avail_size += photos.back().size;
                }
                photos.pop_back();
                //Do we need to get disk space again, in case some other app is writing disk...
                //Generate EventPicStreamStorageConservation is_dropping_mode=true
                do{
                    CacheAutoLock autoLock;
                    int err = autoLock.LockForRead();
                    if (err < 0) {
                        LOG_ERROR("Failed to obtain lock");
                        rv = err;
                        break;
                    }
                    CachePlayer *player = cache_getUserByUserId(userId);
                    if (!player) {
                        LOG_ERROR("No user found");
                        rv = CCD_ERROR_NOT_FOUND;
                    }else{
                        if(!player->_cachedData.details().picstream_storage_conservation_dropping_mode()){
                            //since we are in deleting loop now, generate the event
                            player->_cachedData.mutable_details()->set_picstream_storage_conservation_dropping_mode(true);

                            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
                            ccdiEvent->mutable_picstream_storage_conservation()->set_is_dropping_mode(true);
                            ccdiEvent->mutable_picstream_storage_conservation()->set_free_disk_size_bytes(avail_size);
                            EventManagerPb_AddEvent(ccdiEvent);
                        }
                    }
                }while(0);
            }
            //Generate EventPicStreamStorageConservation is_dropping_mode=false
            do{
                CacheAutoLock autoLock;
                int err = autoLock.LockForRead();
                if (err < 0) {
                    LOG_ERROR("Failed to obtain lock");
                    rv = err;
                    break;
                }
                CachePlayer *player = cache_getUserByUserId(userId);
                if (!player) {
                    LOG_ERROR("No user found");
                    rv = CCD_ERROR_NOT_FOUND;
                }else{
                    if(player->_cachedData.details().picstream_storage_conservation_dropping_mode()){
                        //generate event if avail size over 10% of preserve size
                        if(avail_size > preserveFreeDiskSizeBytes*1.1){
                            player->_cachedData.mutable_details()->set_picstream_storage_conservation_dropping_mode(false);

                            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
                            ccdiEvent->mutable_picstream_storage_conservation()->set_is_dropping_mode(false);
                            ccdiEvent->mutable_picstream_storage_conservation()->set_free_disk_size_bytes(avail_size);
                            EventManagerPb_AddEvent(ccdiEvent);
                        }
                    }
                }
            }while(0);
        }
    }
#endif

    return rv;
}

void SyncDown::enforcePicstreamThresh(PicstreamDirToNumNewMap& picstreamDirs)
{
    if(picstreamDirs.size() == 0) {
        // No new photos
        return;
    }
    int rc;

    bool fullResValid = false;
    std::string fullResDir;
    u32 fullResMaxFiles = 0;
    u32 fullResMaxBytes = 0;
    u32 fullResPreserveFreeDiskPercentage = 0;
    u64 fullResPreserveFreeDiskSizeBytes = 0;
    rc = getPicStreamDownloadDir(PicStreamDownloadDirEnum_FullRes,
                                 fullResDir,
                                 fullResMaxFiles,
                                 fullResMaxBytes,
                                 fullResPreserveFreeDiskPercentage,
                                 fullResPreserveFreeDiskSizeBytes);
    if(rc == 0) {
        fullResValid = true;
    }

    bool lowResValid = false;
    std::string lowResDir;
    u32 lowResMaxFiles = 0;
    u32 lowResMaxBytes = 0;
    u32 lowResPreserveFreeDiskPercentage = 0;
    u64 lowResPreserveFreeDiskSizeBytes = 0;
    rc = getPicStreamDownloadDir(PicStreamDownloadDirEnum_LowRes,
                                 lowResDir,
                                 lowResMaxFiles,
                                 lowResMaxBytes,
                                 lowResPreserveFreeDiskPercentage,
                                 lowResPreserveFreeDiskSizeBytes);
    if(rc == 0) {
        lowResValid = true;
    }

    bool thumbValid = false;
    std::string thumbDir;
    u32 thumbMaxFiles = 0;
    u32 thumbMaxBytes = 0;
    u32 thumbPreserveFreeDiskPercentage = 0;
    u64 thumbPreserveFreeDiskSizeBytes = 0;
    rc = getPicStreamDownloadDir(PicStreamDownloadDirEnum_Thumbnail,
                                 thumbDir,
                                 thumbMaxFiles,
                                 thumbMaxBytes,
                                 thumbPreserveFreeDiskPercentage,
                                 thumbPreserveFreeDiskSizeBytes);
    if(rc == 0) {
        thumbValid = true;
    }

    PicstreamDirToNumNewMap::iterator iter = picstreamDirs.begin();
    while(iter != picstreamDirs.end())
    {
        if(fullResValid &&
           iter->first.size() > fullResDir.size() &&
           fullResDir == iter->first.substr(0, fullResDir.size()) &&
           iter->first[fullResDir.size()] == '/')
        {  // iter->first begins with fullResDir directory
            rc = PicstreamDown_RemoveOldPhotosOverLimits(userId,
                                                         fullResDir,
                                                         fullResMaxFiles,
                                                         fullResMaxBytes,
                                                         fullResPreserveFreeDiskPercentage,
                                                         fullResPreserveFreeDiskSizeBytes);
            if(rc != 0) {
                LOG_ERROR("Issues enforcing fullRes thresholds: %s, %d",
                          fullResDir.c_str(), rc);
            }
        }else if(lowResValid &&
                 iter->first.size() > lowResDir.size() &&
                 lowResDir == iter->first.substr(0, lowResDir.size()) &&
                 iter->first[lowResDir.size()] == '/')
        {
            rc = PicstreamDown_RemoveOldPhotosOverLimits(userId,
                                                         lowResDir,
                                                         lowResMaxFiles,
                                                         lowResMaxBytes,
                                                         lowResPreserveFreeDiskPercentage,
                                                         lowResPreserveFreeDiskSizeBytes);
            if(rc != 0) {
                LOG_ERROR("Issues enforcing lowRes thresholds: %s, %d",
                          lowResDir.c_str(), rc);
            }
        }else if(thumbValid &&
                 iter->first.size() > thumbDir.size() &&
                 thumbDir == iter->first.substr(0, thumbDir.size()) &&
                 iter->first[thumbDir.size()] == '/')
        {
            rc = PicstreamDown_RemoveOldPhotosOverLimits(userId,
                                                         thumbDir,
                                                         thumbMaxFiles,
                                                         thumbMaxBytes,
                                                         thumbPreserveFreeDiskPercentage,
                                                         thumbPreserveFreeDiskSizeBytes);
            if(rc != 0) {
                LOG_ERROR("Issues enforcing thumbnail thresholds: %s, %d",
                          thumbDir.c_str(), rc);
            }
        }else
        {
            LOG_ERROR("Picstream Dir %s unrecognized as fullRes:%s,%s or lowRes:%s,%s or thumbnail:%s,%s",
                      iter->first.c_str(),
                      fullResValid?"T":"F", fullResValid ? fullResDir.c_str():"",
                      lowResValid ?"T":"F", lowResValid  ? lowResDir.c_str():"",
                      thumbValid  ?"T":"F", thumbValid   ? thumbDir.c_str():"");
        }

        picstreamDirs.erase(iter);
        iter = picstreamDirs.begin();
    }
}

int SyncDown::QueryPicStreamItems(ccd::PicStream_DBFilterType_t filter_type,
                                  const std::string& searchField,
                                  const std::string& sortOrder,
                                  google::protobuf::RepeatedPtrField< ccd::PicStreamQueryObject > &output)
{
    return this->jobs->QueryPicStreamItems(filter_type, searchField, sortOrder, output);
}

int SyncDown::QueryPicStreamItems(const std::string& searchField,
                          const std::string& sortOrder,
                          std::queue<PicStreamPhotoSet> &output)
{
    return this->jobs->QueryPicStreamItems(searchField, sortOrder, output);
}

int SyncDown::QuerySharedFilesItems(ccd::SyncFeature_t syncFeature,
                                    const std::string& searchField,
                                    const std::string& sortOrder,
                                    google::protobuf::RepeatedPtrField<ccd::SharedFilesQueryObject> &output)
{
    return this->jobs->querySharedFilesItems(syncFeature,
                                             searchField,
                                             sortOrder,
                                             /*OUT*/ output);
}

int SyncDown::QuerySharedFilesEntryByCompId(u64 compId,
                                            u64 syncFeature,
                                            u64& revision_out,
                                            u64& datasetId_out,
                                            std::string& name_out,
                                            bool& thumbDownloaded_out,
                                            std::string& thumbRelPath_out)
{
    return this->jobs->sharedFilesEntryGetByCompId(compId,
                                                   syncFeature,
                                                   /*OUT*/ revision_out,
                                                   /*OUT*/ datasetId_out,
                                                   /*OUT*/ name_out,
                                                   /*OUT*/ thumbDownloaded_out,
                                                   /*OUT*/ thumbRelPath_out);
}

int SyncDown::AddPicStreamItem(  u64 compId, const std::string &name, u64 rev, u64 origCompId, u64 originDev,
                                const std::string &identifier, const std::string &title, const std::string &albumName,
                                u64 fileType, u64 fileSize, u64 takenTime, const std::string &lpath, /*OUT*/ u64 &jobid, bool &syncUpAdded)
{
    return this->jobs->AddPicStreamItem(compId, name, rev, origCompId, originDev, identifier, title, albumName,
                                                    fileType, fileSize, takenTime, lpath, /*OUT*/ jobid, syncUpAdded);
}

////////////////// Picstream Download Specific functions //////////////
///////////////////////////////////////////////////////////////////////
