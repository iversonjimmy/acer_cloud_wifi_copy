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

#include "CloudDocMgr.hpp"
#include "CloudDocMgr_private.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <vpl_lazy_init.h>
#include <vpl_fs.h>
#include <vplex_file.h>
#include <vplex_http_util.hpp>
#include <vplu_mutex_autolock.hpp>
#include <scopeguard.hpp>

#include <cJSON2.h>
#include <log.h>

#include "ans_connection.hpp"
#include "cache.h"
#include "ccd_storage.hpp"
#include "config.h"
#ifdef CLOUDNODE
#include "dataset.hpp"
#endif
#include "EventManagerPb.hpp"
#include "gvm_file_utils.hpp"
#include "util_open_db_handle.hpp"
#include "vcs_proxy.hpp"
#include "vcs_utils.hpp"
#include "virtual_device.hpp"
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
#include "cslsha.h"
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

#define SYNCDOWN_THREAD_STACK_SIZE (128 * 1024)
#define SYNCDOWN_COPY_FILE_BUFFER_SIZE (4 * 1024)

#define CLOUDDOC_MGR_STACK_SIZE (128 * 1024)

#define DOC_TMPFILE_SUFFIX "_file"
#define THUMB_TMPFILE_SUFFIX "_thumb"

// Target DB schema version.
#define TARGET_SCHEMA_VERSION 1

#define OK_STOP_DB_TRAVERSE 1

// for stringification
#define XSTR(s) STR(s)
#define STR(s) #s

// Starting with v2.5, dl_path will be a path relative to the workdir.
// This function is used to extend it to a full path at runtime.
// E.g., "tmp/sd_123" -> "/aaa/bbb/ccdir/sd/tmp/sd_123"
// For backward compatibility (i.e., dl_path is full path), this function will replace the prefix part.
// E.g., "/zzz/yyy/tmp/sd_123" -> "/aaa/bbb/ccdir/sd/tmp/sd_123"
static void extendPathToFullPath(const std::string &dlpath, const std::string &workdir, std::string &fullpath)
{
    size_t pos = dlpath.rfind("tmp/");
    fullpath.assign(workdir);
    fullpath.append(dlpath, pos, std::string::npos);
}

static int copy_file(const char *srcfile, const char *dstfile)
{
    int rv = 0;
    VPLFile_handle_t hsrc = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t hdst = VPLFILE_INVALID_HANDLE;

    LOG_INFO("Copying file from %s to %s", srcfile, dstfile);

    hsrc = VPLFile_Open(srcfile, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(hsrc)) {
        LOG_ERROR("Failed to open file %s for read:%d", srcfile, (int)hsrc);
        rv = hsrc;
        goto end;
    }
    hdst = VPLFile_Open(dstfile, 
                        VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE, 
                        VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
    if (!VPLFile_IsValidHandle(hdst)) {
        LOG_ERROR("Failed to open file %s for write:%d", dstfile, (int)hdst);
        rv = hdst;
        goto end;
    }
    while (1) {
        char buffer[32768];
        ssize_t nread = VPLFile_Read(hsrc, buffer, sizeof(buffer));
        if (nread < 0) {
            LOG_ERROR("Failed to read from doc file %s", srcfile);
            rv = nread;
            goto end;
        }
        if (nread == 0) {
            break;
        }
        ssize_t nwritten = VPLFile_Write(hdst, buffer, nread);
        if (nwritten < 0) {
            LOG_ERROR("Failed to write to file %s", dstfile);
            rv = nwritten;
            goto end;
        }
        if (nwritten != nread) {
        LOG_ERROR("Number of bytes written ("FMTd_ssize_t") is different from request ("FMTd_ssize_t")", nwritten, nread);
            rv = VPL_ERR_IO;
            goto end;
        }
    }
 end:
    if (VPLFile_IsValidHandle(hsrc)) {
        VPLFile_Close(hsrc);
    }
    if (VPLFile_IsValidHandle(hdst)) {
        VPLFile_Close(hdst);
    }
    return rv;
}

static int dump_thumb(const char *thumbdata, u32 thumbsize, const char *thumbfile)
{
    int rv = 0;
    ssize_t nwritten;
    VPLFile_handle_t h = VPLFile_Open(thumbfile, 
                                      VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE, 
                                      VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
    if (h < 0) {
        LOG_ERROR("Failed to open %s to save thumb", thumbfile);
        rv = h;
        goto end;
    }
    nwritten = VPLFile_Write(h, thumbdata, thumbsize);
    if (nwritten != thumbsize) {
        LOG_ERROR("Number of bytes written ("FMTd_ssize_t") is different from request ("FMTu32")", nwritten, thumbsize);
        rv = VPL_ERR_IO;
        goto end;
    }
 end:
    VPLFile_Close(h);
    return rv;
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

int DocSyncDownJob::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    id          = (u64)sqlite3_column_int64(stmt, n++);
    did         = (u64)sqlite3_column_int64(stmt, n++);
    docname     = (const char*)sqlite3_column_text(stmt, n++);
    compid      = (u64)sqlite3_column_int64(stmt, n++);
    lpath       = (const char*)sqlite3_column_text(stmt, n++);
    const char *dlpath = (const char*)sqlite3_column_text(stmt, n++);
    if (dlpath) {
        dl_path.assign(dlpath);
    }
    else {
        dl_path.clear();
    }
    dl_rev      = (u64)sqlite3_column_int64(stmt, n++);
    return 0;
}

//----------------------------------------------------------------------

int DocSyncDownJobEx::populateFrom(sqlite3_stmt *stmt)
{
    int n = 0;
    id          = (u64)sqlite3_column_int64(stmt, n++);
    did         = (u64)sqlite3_column_int64(stmt, n++);
    docname     = (const char*)sqlite3_column_text(stmt, n++);
    compid      = (u64)sqlite3_column_int64(stmt, n++);
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

static const char test_and_create_db_sql[] = 
"CREATE TABLE IF NOT EXISTS request ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " // request ID
    "uid INTEGER NOT NULL, "   // user ID
    "did INTEGER NOT NULL, "   // dataset ID
    "type INTEGER NOT NULL, "  // change type
    "name TEXT NOT NULL, "   // absolute path on local device where doc exists
    "newname TEXT, "         // absolute path on local device - used only in MOVE
    "mtime INTEGER, "          // last modification time (seconds since Unix epoch)
    "fpath TEXT, "           // path into cc/dsng/tmp/ where copy of doc content is saved; starting v2.5, this is relative path
#ifdef TEST_DB_UPGRADE
    "tpath TEXT)";           // path into cc/dsng/tmp/ where copy of preview image is saved; starting v2.5, this is relative path
#else
    "tpath TEXT, "
    // New columns added in v1.1.
    "dloc TEXT, "            // dataset location
    "compid INTEGER, "         // component ID
    "baserev INTEGER, "        // base revision
    "docname TEXT, "         // doc name = <origin-device-id>/<abs-path-on-origin-device>
    "conttype TEXT, "        // MIME content type
    "retry INTEGER)";          // retry count
#endif

#define ADMINID_SCHEMA_VERSION 1

#define VERID_DATASET_VERSION 1
#define VERID_SCHEMA_VERSION 2
static const char test_and_create_db_sql_1[] = 
    "BEGIN;"
    "CREATE TABLE IF NOT EXISTS version ("
    "id INTEGER PRIMARY KEY, "
    "value INTEGER NOT NULL);"
    "INSERT OR IGNORE INTO version VALUES ("XSTR(VERID_DATASET_VERSION)", 0);"
    "INSERT OR IGNORE INTO version VALUES ("XSTR(VERID_SCHEMA_VERSION)", "XSTR(TARGET_SCHEMA_VERSION)");"

    "CREATE TABLE IF NOT EXISTS component ("
    "id INTEGER PRIMARY KEY, "       // VCS compId, index created automatically (because INTEGER PRIMARY KEY)
    "name TEXT NOT NULL, "           // docname = <origin-device-id>/<abs-path-on-origin-device>
    "ctime INTEGER NOT NULL, "
    "mtime INTEGER NOT NULL, "
    "origin_dev INTEGER, "           // VCS originDevice
    "cur_rev INTEGER NOT NULL, "     // VCS revision
    "cksum INTEGER NOT NULL);"       // Row checksum

    "CREATE TABLE IF NOT EXISTS revision ("
    "compid INTEGER NOT NULL, "      // VCS compId == component.id
    "rev INTEGER NOT NULL, "         // VCS revision
    "size INTEGER NOT NULL, "
    "mtime INTEGER NOT NULL, "
    "update_dev INTEGER, "           // VCS updateDevice
    "hash TEXT, "                  // SHA1 hash of file (calculated locally, encoded as 40 byte text string)
    "cksum TEXT, "                 // Row checksum (calculated locally, encoded as 40 byte text string)
    "UNIQUE (compid), "
    "FOREIGN KEY (compid) REFERENCES component (id));"
    "CREATE INDEX IF NOT EXISTS i_rev_by_compid ON revision (compid);"

    "CREATE TABLE IF NOT EXISTS request ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " // request ID
    "uid INTEGER NOT NULL, "   // user ID
    "did INTEGER NOT NULL, "   // dataset ID
    "type INTEGER NOT NULL, "  // change type
    "name TEXT, "            // absolute path on local device where doc exists
    "newname TEXT, "         // absolute path on local device - used only in MOVE
    "mtime INTEGER, "          // last modification time (seconds since Unix epoch)
    "fpath TEXT, "           // path into cc/dsng/tmp/ where copy of doc is saved
    "tpath TEXT, "           // path where copy of preview image is stored (optional)
    "dloc TEXT, "            // dataset location
    "compid INTEGER, "         // component ID (optional)
    "baserev INTEGER, "        // base revision (optional)
    "docname TEXT,  "        // doc name = <origin-device-id>/<abs-path-on-origin-device>
    "conttype TEXT, "        // MIME content type
    "retry INTEGER);"          // retry count

    "CREATE TABLE IF NOT EXISTS sd_version ("
    "did INTEGER PRIMARY KEY NOT NULL UNIQUE, "         // dataset ID
    "version INTEGER NOT NULL);"                        // last-checked dataset version

    "CREATE TABLE IF NOT EXISTS sd_job ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "  // job ID
    "did INTEGER NOT NULL, "   // dataset ID
    "docname TEXT, "         // doc name = <origin-device-id>/<abs-path-on-origin-device>
    "compid INTEGER NOT NULL, "// component ID
    "lpath TEXT NOT NULL, "  // local path on device to copy-back to
    "add_ts INTEGER NOT NULL, "// time when job was added
    "disp_ts INTEGER, "        // time when job was dispatched
    "dl_path TEXT, "         // download path relative to workdir, where content is first downloaded
    "dl_try_ts INTEGER, "      // time when download was last tried; null if never
    "dl_failed INTEGER DEFAULT 0, "// number of times download failed
    "dl_done_ts INTEGER, "     // timestamp when download succeeded; null if not yet
    "dl_rev INTEGER, "         // revision of content downloaded
    "cb_try_ts INTEGER, "      // time when copyback was last tried; null if never
    "cb_failed INTEGER DEFAULT 0, "// number of times copyback failed
    "cb_done_ts INTEGER, "     // time when copyback succeeded; null if not yet
    "UNIQUE (did, docname)); "   // sanity check

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
    STMT_INSERT_OR_REPLACE_COMPONENT,
    STMT_INSERT_OR_REPLACE_REVISION,
    STMT_COUNT_REQUEST_BY_LOCALPATH,
    STMT_COUNT_REQUEST_BY_DOCNAME,
    STMT_SELECT_REVISION_BY_COMPID,
    STMT_UPDATE_HASH_BY_COMPID,
    STMT_UPDATE_REV_IN_COMPONENT_BY_COMPID,
    STMT_UPDATE_REV_IN_REVISION_BY_COMPID,
    STMT_COUNT_JOBS_BY_COMPID,
    // DocSNG Queries
    STMT_INSERT_REQUEST,
    STMT_UPDATE_PATHS,
    STMT_DELETE_REQUEST,
    STMT_SELECT_OLDEST_REQUEST,
    STMT_SELECT_REQUESTS_IN_ORDER,
    STMT_SELECT_REQUESTS_IN_ORDER_2,
    STMT_SELECT_ALL_REQUESTS_IN_ORDER,
    STMT_SELECT_REQUEST_BY_ID,
    STMT_DELETE_ALL_REQUESTS,
    STMT_GET_VERSION,
    STMT_DELETE_COMPONENT,
    STMT_DELETE_REVISION,
    STMT_GET_COMPID_BY_NAME,
    STMT_SELECT_HASH_BY_COMPID,
    STMT_INSERT_REQUEST_TO_NEWDB,
    STMT_MAX, // this must be last
};

// columns needed to populate DocSyncDownJob struct
#define JOB_COLS "id, did, docname, compid, lpath, dl_path, dl_rev"
// columns needed to populate DocSyncDownJobEx struct
#define JOBEX_COLS "id, did, docname, compid, lpath, add_ts, disp_ts, dl_path, dl_try_ts, dl_failed, dl_done_ts, dl_rev, cb_try_ts, cb_failed, cb_done_ts"

DEFSQL(BEGIN,                                   \
       "BEGIN IMMEDIATE");
DEFSQL(COMMIT,                                  \
       "COMMIT");
DEFSQL(ROLLBACK,                                \
       "ROLLBACK");
DEFSQL(ADD_JOB,                                                 \
       "INSERT INTO sd_job (did, docname, compid, lpath, add_ts) "   \
       "VALUES (:did, :docname, :compid, :lpath, :add_ts)");
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
DEFSQL(INSERT_OR_REPLACE_COMPONENT,                                                             \
       "INSERT OR REPLACE INTO component (id, name, ctime, mtime, origin_dev, cur_rev, cksum)"  \
       "VALUES (:id, :name, :ctime, :mtime, :origin_dev, :cur_rev, :cksum)");
DEFSQL(INSERT_OR_REPLACE_REVISION,                                                                     \
       "INSERT OR REPLACE INTO revision (compid, rev, size, mtime, update_dev, hash, cksum)"  \
       "VALUES (:compid, :rev, :size, :mtime, :update_dev, :hash, :cksum)");
DEFSQL(COUNT_REQUEST_BY_LOCALPATH,              \
       "SELECT COUNT(*) "                       \
       "FROM request "                          \
       "WHERE name=:name");
DEFSQL(COUNT_REQUEST_BY_DOCNAME,                \
       "SELECT COUNT(*) "                       \
       "FROM request "                          \
       "WHERE docname=:docname");
DEFSQL(SELECT_REVISION_BY_COMPID,               \
       "SELECT size, hash, rev "                \
       "FROM revision "                         \
       "WHERE compid=:compid");
DEFSQL(UPDATE_HASH_BY_COMPID,                   \
       "UPDATE revision "                       \
       "SET hash=:hash, size=:size "            \
       "WHERE compid=:compid"); 
DEFSQL(UPDATE_REV_IN_COMPONENT_BY_COMPID,                               \
       "UPDATE component "                                              \
       "SET name=:name, ctime=:ctime, mtime=:mtime, cur_rev=:cur_rev "  \
       "WHERE id=:id"); 
DEFSQL(UPDATE_REV_IN_REVISION_BY_COMPID,                                    \
       "UPDATE revision "                                                   \
       "SET rev=:rev, mtime=:mtime, update_dev=:update_dev "                \
       "WHERE compid=:compid"); 
DEFSQL(COUNT_JOBS_BY_COMPID,                    \
       "SELECT COUNT(*) "                       \
       "FROM sd_job "                           \
       "WHERE compid=:compid");

// DocSNG queries

DEFSQL(INSERT_REQUEST,                                                  \
       "INSERT INTO request (uid, did, type, name, newname, mtime, dloc, compid, baserev, docname, conttype, retry) " \
       "VALUES (:uid, :did, :type, :name, :newname, :mtime, :dloc, :compid, :baserev, :docname, :conttype, :retry)");
DEFSQL(UPDATE_PATHS,                            \
       "UPDATE request "                        \
       "SET fpath=:fpath, tpath=:tpath "        \
       "WHERE id=:id");
DEFSQL(DELETE_REQUEST,                          \
       "DELETE FROM request "                   \
       "WHERE id=:id");
DEFSQL(SELECT_OLDEST_REQUEST,                                           \
       "SELECT id, uid, did, type, name, newname, mtime, fpath, tpath, dloc, compid, baserev, docname, conttype " \
       "FROM request "                                                  \
       "WHERE uid=:uid "                                                \
       "ORDER BY id LIMIT 1");
DEFSQL(SELECT_REQUESTS_IN_ORDER,                \
       "SELECT id, type, name, newname, mtime " \
       "FROM request "                          \
       "WHERE uid=:uid "                        \
       "ORDER BY id");  
DEFSQL(SELECT_REQUESTS_IN_ORDER_2,                                      \
       "SELECT id, uid, did, type, name, newname, mtime, fpath, tpath " \
       "FROM request "                                                  \
       "WHERE uid=:uid "                                                \
       "ORDER BY id");
DEFSQL(SELECT_ALL_REQUESTS_IN_ORDER,                                    \
       "SELECT id, uid, did, type, name, newname, mtime, fpath, tpath, compid, baserev, docname " \
       "FROM request "                                                  \
       "ORDER BY id");
DEFSQL(SELECT_REQUEST_BY_ID,                                            \
       "SELECT id, uid, did, type, name, newname, mtime, fpath, tpath, compid, baserev, docname " \
       "FROM request "                                                  \
       "WHERE id=:id");
DEFSQL(DELETE_ALL_REQUESTS,                     \
       "DELETE FROM request");
DEFSQL(GET_VERSION,                             \
       "SELECT value "                          \
       "FROM version "                          \
       "WHERE id=:id");
DEFSQL(DELETE_COMPONENT,                        \
       "DELETE FROM component "                 \
       "WHERE id=:id");
DEFSQL(DELETE_REVISION,                         \
       "DELETE FROM revision "                  \
       "WHERE compid=:compid");
DEFSQL(GET_COMPID_BY_NAME,                      \
       "SELECT id "                             \
       "FROM component "                        \
       "WHERE name=:name");
DEFSQL(SELECT_HASH_BY_COMPID,                   \
       "SELECT hash "                           \
       "FROM revision "                         \
       "WHERE compid=:compid");
DEFSQL(INSERT_REQUEST_TO_NEWDB,                 \
       "INSERT INTO request (id, uid, did, type, name, newname, mtime, fpath, tpath, dloc, compid, baserev, docname, conttype, retry) " \
       "VALUES (:id, :uid, :did, :type, :name, :newname, :mtime, :fpath, :tpath, :dloc, :compid, :baserev, :docname, :conttype, :retry)");

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
static VPLLazyInitMutex_t db_mutex = VPLLAZYINITMUTEX_INIT;
#endif

DocSyncDownJobs::DocSyncDownJobs(const std::string &workdir)
    : workdir(workdir), db(NULL), purge_on_close(false)
{
    dbpath = workdir + "db";
    int err = 0;
    err = VPLMutex_Init(&mutex);
    if (err) {
        LOG_ERROR("Failed to initialize mutex: %d", err);
    }
}

DocSyncDownJobs::~DocSyncDownJobs()
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

int DocSyncDownJobs::openDB()
{
    int result = 0;

    if (!VPLMutex_LockedSelf(&mutex)) {
        FAILED_ASSERT("openDB called without holding mutex");
    }

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

        char *errmsg = NULL;
        dberr = sqlite3_exec(db, test_and_create_db_sql_1, NULL, NULL, &errmsg);
        if (dberr != SQLITE_OK) {
            LOG_ERROR("Failed to test/create tables: %d, %s", dberr, errmsg);
            sqlite3_free(errmsg);
            result = mapSqliteErrCode(dberr);
            goto end;
        }
        
        dbstmts.resize(STMT_MAX, NULL);
    }

 end:
    if (result) {
        closeDB();
    }
    return result;
}

int DocSyncDownJobs::closeDB()
{
    int result = 0;

    // Note: may not hold mutex here, since this is called from the destructor

    if (db) {
        std::vector<sqlite3_stmt*>::iterator it;
        for (it = dbstmts.begin(); it != dbstmts.end(); it++) {
            if (*it != NULL) {
                sqlite3_finalize(*it);
            }
            *it = NULL;
        }

        sqlite3_close(db);
        db = NULL;
        LOG_DEBUG("Closed DB");
    }

    //if (purge_on_close) {
    //    int vplerr = VPLFile_Delete(dbpath.c_str());
    //    if (!vplerr) {
    //        LOG_INFO("Deleted db file");
    //    }
    //    else {
    //        LOG_ERROR("Failed to delete db file: %d", vplerr);
    //        // don't propagate the error
    //    }
    //}

    return result;
}

int DocSyncDownJobs::beginTransaction()
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

int DocSyncDownJobs::commitTransaction()
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

int DocSyncDownJobs::rollbackTransaction()
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

int DocSyncDownJobs::getAdminValue(u64 adminId, u64 &value)
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

int DocSyncDownJobs::setDatasetVersion(u64 datasetid, u64 version)
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

int DocSyncDownJobs::getDatasetVersion(u64 datasetid, u64 &version)
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

int DocSyncDownJobs::removeDatasetVersion(u64 datasetId)
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

int DocSyncDownJobs::removeAllDatasetVersions()
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

int DocSyncDownJobs::addJob(u64 did, const std::string &docname, u64 compid,
                         const std::string &lpath, 
                         u64 timestamp, /*OUT*/ u64 &jobid)
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
        // bind :docname
        dberr = sqlite3_bind_text(stmt, n++, docname.data(), docname.size(), NULL);
        CHECK_BIND(dberr, result, db, end);
        // bind :compid
        dberr = sqlite3_bind_int64(stmt, n++, (s64)compid);
        CHECK_BIND(dberr, result, db, end);
        // bind :lpath
        dberr = sqlite3_bind_text(stmt, n++, lpath.data(), lpath.size(), NULL);
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

int DocSyncDownJobs::removeJob(u64 jobid)
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

int DocSyncDownJobs::removeJobsByDatasetId(u64 datasetId)
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

int DocSyncDownJobs::removeAllJobs()
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

int DocSyncDownJobs::findJob(u64 jobid, /*OUT*/ DocSyncDownJob &job)
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

int DocSyncDownJobs::findJob(const std::string &localpath, /*OUT*/ DocSyncDownJob &job)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB_BY_LOCALPATH)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB_BY_LOCALPATH), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :docname
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
int DocSyncDownJobs::findJob##Operation##NotDoneNoRecentDispatch(u64 cutoff, DocSyncDownJob &job) \
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

int DocSyncDownJobs::findJobEx(u64 jobid, /*OUT*/ DocSyncDownJobEx &job)
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

int DocSyncDownJobs::visitJobs(JobVisitor visitor, void *param)
{
    int result = 0;
    int dberr = 0;

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_JOB)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_JOB), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        DocSyncDownJob job;
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

int DocSyncDownJobs::visitJobsByDatasetId(u64 datasetId, bool fails_only,
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
        DocSyncDownJob job;
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

int DocSyncDownJobs::visitJobsByDatasetIdLocalpathPrefix(u64 datasetId,
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
        DocSyncDownJob job;
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

int DocSyncDownJobs::setJobDownloadPath(u64 id, const std::string &downloadpath)
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

int DocSyncDownJobs::setJobDownloadedRevision(u64 jobid, u64 revision)
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
int DocSyncDownJobs::setJob##EventName##Time(u64 jobid, u64 timestamp)     \
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

#undef setJobTime

#define incJobFailedCount(Operation,OPERATION)                          \
int DocSyncDownJobs::incJob##Operation##FailedCount(u64 jobid)             \
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

#undef incJobFailedCount

int DocSyncDownJobs::setPurgeOnClose(bool purge)
{
    purge_on_close = purge;

    return 0;
}

int DocSyncDownJobs::Check()
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int DocSyncDownJobs::SetDatasetLastCheckedVersion(u64 datasetid, u64 version)
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setDatasetVersion(datasetid, version);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int DocSyncDownJobs::GetDatasetLastCheckedVersion(u64 datasetid, u64 &version)
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    CHECK_ERR(err, result, end);

    err = getDatasetVersion(datasetid, version);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int DocSyncDownJobs::ResetAllDatasetLastCheckedVersions()
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    CHECK_ERR(err, result, end);

    err = removeAllDatasetVersions();
    CHECK_ERR(err, result, end);

 end:
    return result;
}

static int deleteTmpFile(DocSyncDownJob &job, void *param)
{
    if (!job.dl_path.empty()) {
        std::string &workdir = *(std::string*)param;
        std::string dl_path;
        extendPathToFullPath(job.dl_path, workdir, dl_path);
        int err = VPLFile_Delete(dl_path.c_str());
        if (err) {
            LOG_INFO("Failed to delete tmp file(%s): %d. Typically successfully moved to destination",
                dl_path.c_str(), err);
        }
    }
    return 0;
}

int DocSyncDownJobs::AddJob(u64 datasetid, const std::string &comppath, u64 compid,
                         const std::string &localpath,
                         /*OUT*/ u64 &jobid)
{
    int result = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);
        err = beginTransaction();
        CHECK_ERR(err, result, end);

        DocSyncDownJob job;
        err = findJob(localpath, job);
        if (err == CCD_ERROR_NOT_FOUND) {
            goto no_prior_job;
        }
        CHECK_ERR(err, result, end);
        deleteTmpFile(job, &workdir);
        err = removeJob(job.id);
        CHECK_ERR(err, result, end);

    no_prior_job:
        err = addJob(datasetid, comppath, compid, localpath, VPLTime_GetTime(), jobid);
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

int DocSyncDownJobs::RemoveJob(u64 jobid)
{
    int result = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    {
        int err = 0;
        err = openDB();
        CHECK_ERR(err, result, end);

        DocSyncDownJob job;
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

int DocSyncDownJobs::RemoveJobsByDatasetId(u64 datasetId)
{
    int result = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

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

int DocSyncDownJobs::RemoveAllJobs()
{
    int result = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

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

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
#define GetNextJobTo(Operation)                                         \
int DocSyncDownJobs::GetNextJobTo##Operation(u64 cutoff, /*OUT*/DocSyncDownJob &job) \
{                                                                       \
    int result = 0;                                                     \
    int err = 0;                                                        \
                                                                        \
    MutexAutoLock dblock(&mutex);                                       \
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));           \
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
#else
#define GetNextJobTo(Operation)                                         \
int DocSyncDownJobs::GetNextJobTo##Operation(u64 cutoff, /*OUT*/DocSyncDownJob &job) \
{                                                                       \
    int result = 0;                                                     \
    int err = 0;                                                        \
                                                                        \
    MutexAutoLock dblock(&mutex);                                       \
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
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

// define GetNextJobToDownload()
// calls findJobDownloadNotDoneNoRecentDispatch()
GetNextJobTo(Download)
// define GetNextJobToCopyback()
// calls findJobCopybackNotDoneNoRecentDispatch()
GetNextJobTo(Copyback)

#undef GetNextJobTo

int DocSyncDownJobs::SetJobDownloadPath(u64 jobid, const std::string &downloadpath)
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDownloadPath(jobid, downloadpath);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int DocSyncDownJobs::SetJobDownloadedRevision(u64 jobid, u64 revision)
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDownloadedRevision(jobid, revision);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

int DocSyncDownJobs::TimestampJobDispatch(u64 jobid, u64 vplCurrTimeSec)
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    CHECK_ERR(err, result, end);

    err = setJobDispatchTime(jobid, vplCurrTimeSec);
    CHECK_ERR(err, result, end);

end:
    return result;
}
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
#define TimestampJob(EventName)                                         \
int DocSyncDownJobs::TimestampJob##EventName(u64 jobid)                 \
{                                                                       \
    int result = 0;                                                     \
    int err = 0;                                                        \
                                                                        \
    MutexAutoLock dblock(&mutex);                                       \
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));           \
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
#else
#define TimestampJob(EventName)                                         \
int DocSyncDownJobs::TimestampJob##EventName(u64 jobid)                 \
{                                                                       \
    int result = 0;                                                     \
    int err = 0;                                                        \
                                                                        \
    MutexAutoLock dblock(&mutex);                                       \
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
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC
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

#undef TimestampJob

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
#define ReportJobFailed(Operation)                              \
int DocSyncDownJobs::ReportJob##Operation##Failed(u64 jobid)       \
{                                                               \
    int result = 0;                                             \
    int err = 0;                                                \
                                                                \
    MutexAutoLock dblock(&mutex);                               \
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));   \
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
#else
#define ReportJobFailed(Operation)                              \
int DocSyncDownJobs::ReportJob##Operation##Failed(u64 jobid)       \
{                                                               \
    int result = 0;                                             \
    int err = 0;                                                \
                                                                \
    MutexAutoLock dblock(&mutex);                               \
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
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

// define ReportJobDownloadFailed()
// calls incJobDownloadFailedCount()
ReportJobFailed(Download)
// define ReportJobCopybackFailed()
// calls incJobCopybackFailedCount()
ReportJobFailed(Copyback)

#undef ReportJobFailed

int DocSyncDownJobs::PurgeOnClose()
{
    int result = 0;
    int err = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = setPurgeOnClose(true);
    CHECK_ERR(err, result, end);

 end:
    return result;
}

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
std::string DocSyncDownJobs::ComputeSHA1(std::string filePath)
{
    CSL_ShaContext hashCtx;
    u8 hashVal[CSL_SHA1_DIGESTSIZE];   // Really SHA1, but be on the safe side
    std::string fileHash;
    
    VPLFile_handle_t fh;
    size_t n;
    unsigned char buf[1024];

    fh = VPLFile_Open(filePath.c_str(), VPLFILE_OPENFLAG_READONLY, 0666);
    if (!VPLFile_IsValidHandle(fh)) {
        LOG_ERROR("Error opening file: %s", filePath.c_str());
        return fileHash;
    } else {
        CSL_ResetSha(&hashCtx);
        while((n = VPLFile_Read(fh, buf, sizeof(buf))) > 0)
            CSL_InputSha(&hashCtx, buf, n);
        CSL_ResultSha(&hashCtx, hashVal);
    }
    
    fileHash.clear();
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        char byteStr[4];
        snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, hashVal[hashIndex]);
        fileHash.append(byteStr);
    }

    VPLFile_Close(fh);

    return fileHash;
}

std::string DocSyncDownJobs::calculateRowChecksum(std::string input)
{
    CSL_ShaContext hashCtx;
    std::string rowChecksum;
    u8 hashVal[CSL_SHA1_DIGESTSIZE];

    CSL_ResetSha(&hashCtx);
    CSL_InputSha(&hashCtx, input.c_str(), input.length());
    CSL_ResultSha(&hashCtx, hashVal);
    rowChecksum.clear();
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        char byteStr[4];
        snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, hashVal[hashIndex]);
        rowChecksum.append(byteStr);
    }

    return rowChecksum;
}

int DocSyncDownJobs::GetRevisionbyCompid(u64 compId, u64 &size, std::string &hashval, u64 &rev)
{
    int result = 0;
    int dberr = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    dberr = openDB();
    if (dberr != 0) {
        return dberr;
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_REVISION_BY_COMPID)];

    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SELECT_REVISION_BY_COMPID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int64(stmt, 1, compId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (dberr == SQLITE_ROW) {
        size = (u64)sqlite3_column_int(stmt, 0);
        hashval = (const char*)sqlite3_column_text(stmt, 1);
        rev = (u64)sqlite3_column_int(stmt, 2);
    }
    else {  // SQLITE_DONE, meaning not found
        result = CCD_ERROR_NOT_FOUND;  
    }

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int DocSyncDownJobs::CountJobinUploadRequestbyPath(std::string filePath, u32& count)
{
    int result = 0;
    int err = 0;
    int dberr = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    if(err != 0) {
        return err;
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COUNT_REQUEST_BY_LOCALPATH)];

    if (!stmt) {
        err = sqlite3_prepare_v2(db, GETSQL(COUNT_REQUEST_BY_LOCALPATH), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :name
    dberr = sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, NULL);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (dberr == SQLITE_ROW) {
        count = (u32)sqlite3_column_int(stmt, 0);
    }
    else {  // SQLITE_DONE, meaning not found
        result = CCD_ERROR_NOT_FOUND;  
    }

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int DocSyncDownJobs::CountJobinUploadRequestbyDocName(std::string docname, u32& count)
{
    int result = 0;
    int err = 0;
    int dberr = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    if(err != 0) {
        return err;
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COUNT_REQUEST_BY_DOCNAME)];

    if (!stmt) {
        err = sqlite3_prepare_v2(db, GETSQL(COUNT_REQUEST_BY_DOCNAME), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :name
    dberr = sqlite3_bind_text(stmt, 1, docname.c_str(), -1, NULL);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (dberr == SQLITE_ROW) {
        count = (u32)sqlite3_column_int(stmt, 0);
    }
    else {  // SQLITE_DONE, meaning not found
        result = CCD_ERROR_NOT_FOUND;  
    }

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int DocSyncDownJobs::CountJobinDownloadRequestbyCompid(u64 compId, u32& count)
{
    int result = 0;
    int err = 0;
    int dberr = 0;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    err = openDB();
    if (err != 0){
        return err;
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COUNT_JOBS_BY_COMPID)];

    if (!stmt) {
        err = sqlite3_prepare_v2(db, GETSQL(COUNT_JOBS_BY_COMPID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :compid
    dberr = sqlite3_bind_int64(stmt, 1, compId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (dberr == SQLITE_ROW) {
        count = (u32)sqlite3_column_int(stmt, 0);
    }
    else {  // SQLITE_DONE, meaning not found
        result = CCD_ERROR_NOT_FOUND;  
    }

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return result;
}

int DocSyncDownJobs::UpdateHashbyCompId(u64 compId, std::string filePath)
{
    int rc = 0;
    int rv;
    std::stringstream ss;
    std::string rowChecksum;
    std::string hashval;
    VPLFS_stat_t filestat;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(UPDATE_HASH_BY_COMPID)];

    rv = beginTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(UPDATE_HASH_BY_COMPID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    if (VPLFS_Stat(filePath.c_str(), &filestat) != VPL_OK) {
        LOG_ERROR("Failed to stat %s", filePath.c_str());
        rc = -1;
        goto end;
    }
    hashval.clear();
    hashval = ComputeSHA1(filePath);
    if(hashval.empty()) {
        rc = -1;
        goto end;
    }
    // bind :hash
    rv = sqlite3_bind_text(stmt, 1, hashval.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :size
    rv = sqlite3_bind_int64(stmt, 2, filestat.size);
    CHECK_BIND(rv, rc, db, end);
    // bind :compid
    rv = sqlite3_bind_int64(stmt, 3, compId);
    CHECK_BIND(rv, rc, db, end);
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt) {
        sqlite3_reset(stmt);
    }

    return rc;
}

int DocSyncDownJobs::UpdateRevInComponent(u64 compId, std::string name, u64 ctime, u64 mtime, u64 cur_rev)
{
    int rc = 0;
    int rv;
    std::stringstream ss;
    std::string rowChecksum;
    
    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(UPDATE_REV_IN_COMPONENT_BY_COMPID)];

    rv = beginTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(UPDATE_REV_IN_COMPONENT_BY_COMPID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :name
    rv = sqlite3_bind_text(stmt, 1, name.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :ctime
    rv = sqlite3_bind_int64(stmt, 2, ctime);
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    rv = sqlite3_bind_int64(stmt, 3, mtime);
    CHECK_BIND(rv, rc, db, end);
    // bind :cur_rev
    rv = sqlite3_bind_int64(stmt, 4, cur_rev);
    CHECK_BIND(rv, rc, db, end);
    // bind :compid
    rv = sqlite3_bind_int64(stmt, 5, compId);
    CHECK_BIND(rv, rc, db, end);
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt) {
        sqlite3_reset(stmt);
    }

    return rc;
}

int DocSyncDownJobs::UpdateRevInRevision(u64 compId, u64 rev, u64 mtime, u64 update_dev)
{
    int rc = 0;
    int rv;
    std::stringstream ss;
    std::string rowChecksum;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(UPDATE_REV_IN_REVISION_BY_COMPID)];

    rv = beginTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(UPDATE_REV_IN_REVISION_BY_COMPID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :rev
    rv = sqlite3_bind_int64(stmt, 1, rev);
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    rv = sqlite3_bind_int64(stmt, 2, mtime);
    CHECK_BIND(rv, rc, db, end);
    // bind :update_dev
    rv = sqlite3_bind_int64(stmt, 3, update_dev);
    CHECK_BIND(rv, rc, db, end);
    // bind :compid
    rv = sqlite3_bind_int64(stmt, 4, compId);
    CHECK_BIND(rv, rc, db, end);
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt) {
        sqlite3_reset(stmt);
    }

    return rc;
}

int DocSyncDownJobs::WriteComponentToDB(u64 compid, std::string docname, u64 ctime, u64 mtime, u64 origin_dev, u64 cur_rev)
{
    int rc = 0;
    int rv;
    std::stringstream ss;
    std::string rowChecksum;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt_ins = dbstmts[SQLNUM(INSERT_OR_REPLACE_COMPONENT)];

    rv = beginTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

    if (!stmt_ins) {
        rv = sqlite3_prepare_v2(db, GETSQL(INSERT_OR_REPLACE_COMPONENT), -1, &stmt_ins, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    //:id, :name, :ctime, :mtime, :origin_dev, :cur_dev, :cksum
    // bind :id
    rv = sqlite3_bind_int64(stmt_ins, 1, compid);
    CHECK_BIND(rv, rc, db, end);
    // bind :name
    rv = sqlite3_bind_text(stmt_ins, 2, docname.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :ctime
    rv = sqlite3_bind_int64(stmt_ins, 3, ctime);
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    rv = sqlite3_bind_int64(stmt_ins, 4, mtime);
    CHECK_BIND(rv, rc, db, end);
    // bind :origin_dev
    rv = sqlite3_bind_int64(stmt_ins, 5, origin_dev);
    CHECK_BIND(rv, rc, db, end);
    // bind :cur_rev
    rv = sqlite3_bind_int64(stmt_ins, 6, cur_rev);
    CHECK_BIND(rv, rc, db, end);
    // bind :cksum
    ss.str("");
    ss << compid << "," << docname.c_str() << "," << ctime << "," << mtime << "," << origin_dev << "," << cur_rev << '\0';
    rowChecksum = calculateRowChecksum(ss.str());

    rv = sqlite3_bind_text(stmt_ins, 7, rowChecksum.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_ins);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt_ins) {
        sqlite3_reset(stmt_ins);
    }

    return rc;
}

int DocSyncDownJobs::WriteRevisionToDB(u64 compid, u64 rev, u64 mtime, u64 update_dev)
{
    int rc = 0;
    int rv;
    std::stringstream ss;
    std::string rowChecksum;

    MutexAutoLock dblock(&mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt_ins = dbstmts[SQLNUM(INSERT_OR_REPLACE_REVISION)];

    rv = beginTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

    if (!stmt_ins) {
        rv = sqlite3_prepare_v2(db, GETSQL(INSERT_OR_REPLACE_REVISION), -1, &stmt_ins, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // :compid, :rev, :size, :mtime, :update_dev, :hash, :cksum
    // bind :compid
    rv = sqlite3_bind_int64(stmt_ins, 1, compid);
    CHECK_BIND(rv, rc, db, end);
    // bind :rev
    rv = sqlite3_bind_int64(stmt_ins, 2, rev);
    CHECK_BIND(rv, rc, db, end);
    // bind :size, leave 0 here and will update after copy back successfully
    rv = sqlite3_bind_int64(stmt_ins, 3, 0);
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    rv = sqlite3_bind_int64(stmt_ins, 4, mtime);
    CHECK_BIND(rv, rc, db, end);
    // bind :update_dev
    rv = sqlite3_bind_int64(stmt_ins, 5, update_dev);
    CHECK_BIND(rv, rc, db, end);
    // bind :hash, leave blank here and will update after copy back successfully
    rv = sqlite3_bind_text(stmt_ins, 6, "", -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :cksum
    ss.str("");
    ss << compid << "," << rev << "," << "0" << "," << mtime << "," << update_dev << '\0';
    rowChecksum = calculateRowChecksum(ss.str());

    rv = sqlite3_bind_text(stmt_ins, 7, rowChecksum.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    rv = sqlite3_step(stmt_ins);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    if (rv != 0) {
        rc = rv;
        goto end;
    }

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt_ins) {
        sqlite3_reset(stmt_ins);
    }

    return rc;
}

int DocSyncDownJobs::CountUnfinishedJobsinDB(std::string docname, u64 compId, u32 &count)
{
    int err = 0;
    u32 uploadCount = 0;
    u32 downloadCount = 0;
    LOG_INFO("CountUnfinishedJobsinDB: %s, compID: "FMTu64"", docname.c_str(), compId);
    
    CountJobinUploadRequestbyDocName(docname, uploadCount);
    CountJobinDownloadRequestbyCompid(compId, downloadCount);
    
    count = uploadCount + downloadCount;
    return err;
}
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

//----------------------------------------------------------------------

#ifdef CLOUDNODE

// impl for CloudNode
DocSyncDownDstStor::DocSyncDownDstStor()
    : dstDatasetId(0), dstDataset(NULL)
{
}

// impl for CloudNode
DocSyncDownDstStor::~DocSyncDownDstStor()
{
    if (dstDataset) {
        dstDataset->release();
        dstDataset = NULL;
    }
}

// impl for CloudNode
int DocSyncDownDstStor::init(u64 userId)
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
bool DocSyncDownDstStor::fileExists(const std::string &path)
{
    VPLFS_stat_t _stat; // dummy
    return !dstDataset->stat_component(path, _stat);
}

// impl for CloudNode
int DocSyncDownDstStor::createMissingAncestorDirs(const std::string &path)
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
int DocSyncDownDstStor::copyFile(const std::string &srcPath, const std::string &dstPath)
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

int DocSyncDownDstStor::getVirtDriveDatasetId(u64 userId)
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
DocSyncDownDstStor::DocSyncDownDstStor()
{
    // nothing to do
}

// impl for non-CloudNode
DocSyncDownDstStor::~DocSyncDownDstStor()
{
    // nothing to do
}

// impl for non-CloudNode
int DocSyncDownDstStor::init(u64 userId)
{
    // nothing to do
    return 0;
}

// impl for non-CloudNode
bool DocSyncDownDstStor::fileExists(const std::string &path)
{
    return VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK;
}

// impl for non-CloudNode
int DocSyncDownDstStor::createMissingAncestorDirs(const std::string &path)
{
    return Util_CreatePath(path.c_str(), VPL_FALSE);
}

// impl for non-CloudNode
int DocSyncDownDstStor::copyFile(const std::string &srcPath, const std::string &dstPath)
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
        hout = VPLFile_Open(dstPath.c_str(), VPLFILE_OPENFLAG_WRITEONLY|VPLFILE_OPENFLAG_CREATE|VPLFILE_OPENFLAG_TRUNCATE, VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR);
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

DocSyncDown::DocSyncDown(const std::string &workdir)
:   workdir(workdir),
    userId(0),
    jobs(NULL),
    thread_spawned(false),
    thread_running(false),
    exit_thread(false),
    overrideAnsCheck(false),
    isDoingWork(false)
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
}

DocSyncDown::~DocSyncDown()
{
    int err = 0;

    if (jobs) {
        delete jobs;
        jobs = NULL;
        LOG_DEBUG("SyncDownJobs obj destroyed");
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

int DocSyncDown::spawnDispatcherThread()
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

int DocSyncDown::signalDispatcherThreadToStop()
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

VPLTHREAD_FN_DECL DocSyncDown::dispatcherThreadMain(void *arg)
{
    DocSyncDown *sd = (DocSyncDown*)arg;
    if (sd) {
        sd->dispatcherThreadMain();

        delete sd;
        LOG_DEBUG("DocSyncDown obj destroyed");
    }
    return VPLTHREAD_RETURN_VALUE;
}

void DocSyncDown::dispatcherThreadMain()
{
    thread_running = true;

    LOG_DEBUG("dispatcher thread started");

    int err = 0;

    /// main loop for the CloudDoc Sync thread
    while (1) {
        if (exit_thread) break;

        VPL_BOOL ansConnected;
        u64 datasetid = 0;  // assume there is no dataset to query for changes and prove otherwise

        {  // MutexAutoLock scope
            MutexAutoLock lock(&mutex);

            while (dataset_changes_queue.empty() && !exit_thread)
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
                        LOG_ERROR("Unexpected failure to wait on sem: %d", err);
                        exit_thread = true;
                        break;
                    }
                    lock.UnlockNow();
                    u64 datasetId_unused;
                    datasetIdErr = VCS_getDatasetID(userId, "Cloud Doc", datasetId_unused);
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

        // start upload processing in case it was paused for retryable error
        // this is a NOP if the upload queue is already running
        DocSNGQueue_Enable(userId);

        // apply the dataset changes
        applyChanges();
    }  // while

    LOG_DEBUG("dispatcher thread exiting");

    thread_running = false;
    thread_spawned = false;
}

int DocSyncDown::getChanges(u64 datasetid, u64 changeSince)
{
    int err = 0;

    u64 currentVersion = 0;
    do {
        int httpResponse = -1;
        std::string changes;
        err = VCSgetDatasetChanges(userId, "", "", datasetid, changeSince, /*max*/500, /*includeDeleted*/false, httpResponse, changes);
        if (err) {
            LOG_ERROR("Failed to get changes from VCS: %d", err);
            return err;
        }
        if (httpResponse != 200) {
            LOG_ERROR("VCS returned %d", httpResponse);
            return err;
        }
        LOG_DEBUG("response=%s", changes.c_str());

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
            if (!json_change) continue;

            cJSON2 *json_isDeleted = cJSON2_GetObjectItem(json_change, "isDeleted");
            if (!json_isDeleted) {
                LOG_ERROR("isDeleted missing");
                continue;
            }
            if (json_isDeleted->type == cJSON2_True) {
                // we don't sync-down deletion, so skip this entry
               continue;
            }
            if (json_isDeleted->type != cJSON2_False) {
                LOG_ERROR("isDeleted in unexpected format");
                // bad entry, ignore this entry
                continue;
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
            if (strcmp(json_type->valuestring, "file")) {
                // this change is for a directory - skip
                continue;
            }

            cJSON2 *json_name = cJSON2_GetObjectItem(json_change, "name");
            if (!json_name) {
                LOG_ERROR("name missing");
                continue;
            }

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
            if (isCloudDocDataset(datasetid)) {
                // docname should have the format <origin-deviceid>/<origin-path>
                u64 origin_deviceid = VPLConv_strToU64(json_name->valuestring, NULL, 10);
                if (origin_deviceid != VirtualDevice_GetDeviceId()) {
                    // doc originated from a different device, so skip.
                    continue;
                }
            }
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

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

            std::string comppath;
            std::string localpath;
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
            if (isCloudDocDataset(datasetid)) {
                // docname should have the format <origin-deviceid>/<origin-path>
                char *p = strchr(json_name->valuestring, '/');
                if (!p) {
                    LOG_ERROR("Bad docname %s", json_name->valuestring);
                    continue;
                }
                comppath.assign(json_name->valuestring);

                localpath.assign(p);
#ifdef WIN32
                // need to rewrite
                // e.g., /C/Users/fokushi/Documents/foo.docx -> C:/Users/fokushi/Documents/foo.docx
                if (localpath.length() < 3) {
                    LOG_ERROR("Bad docname %s", json_name->valuestring);
                    continue;
                }
                localpath[0] = localpath[1];  // copy drive letter
                localpath[1] = ':';
#endif // WIN32
            }
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

            if (!comppath.empty() && compid && !localpath.empty()) {
                u64 _jobid;
                err = jobs->AddJob(datasetid, comppath, compid, localpath, _jobid);
            }
        }  // for

        // Bug 7359:
        // Update last-checked version every time after processing all the changes in the response from a single call to VCSgetDatasetChanges().
        // This is to protect against SyncDown thread having to start all over again if the thread is killed inside this while loop.
        // (The call to update last-checked version used to be after this while loop.)
        // Note that there could still be a small loss if the thread is killed between the time a job is added and last-checked version is saved.
        // So the intent is not to fix it complete (not possible) but to reduce loss.
        // BTW, the fact that the list of jobs could be ahead of the last-checked version is not a concern.
        // When we reprocess the same changes, the "new" jobs will automatically replace the "old" jobs.  (This is how AddJob works.)
        if (changeSince > 0) {
            err = jobs->SetDatasetLastCheckedVersion(datasetid, changeSince);
            if (err) {
                LOG_ERROR("Failed to update last-checked version: %d", err);
            }
        }
    } while (changeSince < currentVersion);

    return err;
}

int DocSyncDown::applyChanges()
{
    int err;
    int rv = 0;
    u64 cutoff = VPLTime_GetTime();
    DocSyncDownJob job;
    std::map<int, int>::iterator it;

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
        }

        u64 currVplTimeSec = VPLTime_GetTime();
        if(currVplTimeSec < cutoff) {
            // Time went backwards.  Let's not get into infinite loop.
            LOG_ERROR("Time went backwards(%s): "FMTu64"->"FMTu64,
                job.docname.c_str(), cutoff, currVplTimeSec);
            break;  // expected to go to comment label: SYNCDOWN_OUTSIDE_WHILE_1
        }
        err = jobs->TimestampJobDispatch(job.id, currVplTimeSec);
        if (err) {
            LOG_ERROR("Failed to timestamp dispatch: %d", err);
            rv = err;
            continue;
        }

        err = tryCopyback(job);
        // errmsg logged by tryCopyback()
        if (err) {
            rv = err;
            continue;
        }

        if (isCloudDocDataset(job.did)) {
            ccd::CcdiEvent *ep = new ccd::CcdiEvent();
            ccd::EventSyncBackCompletion *cp = ep->mutable_syncback_completion();
            cp->set_user_id(userId);
            cp->set_dataset_id(job.did);
            cp->set_component_name(job.docname);
            cp->set_component_id(job.compid);
            cp->set_revision(job.dl_rev);
            cp->set_local_path(job.lpath);
            EventManagerPb_AddEvent(ep);
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
        if (err != VPL_OK) break;
#endif
        err = getJobDownloadReady(cutoff, job);
        // errmsg logged by getJobDownloadReady()
        if (err) 
        {
            // No more download job
            break; 
        }

        u64 currVplTimeSec = VPLTime_GetTime();
        if(currVplTimeSec < cutoff) {
            // Time went backwards.  Let's not get into infinite loop.
            LOG_ERROR("Time went backwards(%s): "FMTu64"->"FMTu64,
                job.docname.c_str(), cutoff, currVplTimeSec);
            break;  // expected to go to comment label: SYNCDOWN_OUTSIDE_WHILE_2
        }
        err = jobs->TimestampJobDispatch(job.id, currVplTimeSec);
        if (err) {
            LOG_ERROR("Failed to timestamp dispatch: %d", err);
            rv = err;
            continue;
        }

        err = tryDownload(job);
        // errmsg logged by tryDownload()
        if (err) {
            rv = err;
            continue;
        }

        err = tryCopyback(job);
        // errmsg logged by tryCopyback()
        if (err) {
            rv = err;
            continue;
        }

        if (isCloudDocDataset(job.did)) {
            ccd::CcdiEvent *ep = new ccd::CcdiEvent();
            ccd::EventSyncBackCompletion *cp = ep->mutable_syncback_completion();
            cp->set_user_id(userId);
            cp->set_dataset_id(job.did);
            cp->set_component_name(job.docname);
            cp->set_component_id(job.compid);
            cp->set_revision(job.dl_rev);
            cp->set_local_path(job.lpath);
            EventManagerPb_AddEvent(ep);
        }
    }
    // Comment Label: SYNCDOWN_OUTSIDE_WHILE_2

    return rv;
}

int DocSyncDown::getJobDownloadReady(u64 threshold, DocSyncDownJob &job)
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

int DocSyncDown::getJobCopybackReady(u64 threshold, DocSyncDownJob &job)
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

int DocSyncDown::tryDownload(DocSyncDownJob &job)
{
    int err = 0;

    LOG_DEBUG("id "FMTu64" docname %s", job.id, job.docname.c_str());

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
        LOG_ERROR("Failed to set download try timestamp: compname=%s", job.docname.c_str());
        return err;
    }

    do {
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
        // In order not to GET filemetadata from VCS frequently, the check syncdown algorithm is implemented in getLatestRevNum 
        u32 check_result = 0;
        err = getLatestRevNum(job, job.dl_rev, check_result);
        // errmsg logged by getLatestRevNum()
        if (err) break;
        if (check_result > 0) {
            // According to the check result, CCD do not need to download this job.
            // Drop it.
            // For detail check mechanism, please refer to https://www.ctbg.acer.com/wiki/index.php/CCD_CloudDoc_2.6.0_Plan
            err = 1;
            break;
        }
#else
        err = getLatestRevNum(job, job.dl_rev);
        // errmsg logged by getLatestRevNum()
        if (err) break;
#endif
        err = downloadRevision(job, job.dl_rev);
        // errmsg logged by downloadRevision()
        if (err) break;
    } while (0);

    if (err > 0) {  // error - drop job
        LOG_ERROR("Docs download jobid drop("FMTu64",%s)", job.id, job.docname.c_str());
        int err2 = jobs->RemoveJob(job.id);
        if (err2) {
            LOG_ERROR("Failed to remove job "FMTu64": %d", job.id, err2);
        }
    }
    else if (err < 0) {  // error - retry later
        LOG_WARN("Docs download jobid retry("FMTu64",%s)", job.id, job.docname.c_str());
        (void)jobs->ReportJobDownloadFailed(job.id);
    }
    else {  // success
        LOG_INFO("Docs download jobid downloaded("FMTu64",%s)", job.id, job.docname.c_str());
        err = jobs->TimestampJobDoneDownload(job.id);
        if (err != 0) {
            LOG_ERROR("Failed to set download done timestamp: compname=%s", job.docname.c_str());
        }
        else {
            err = jobs->SetJobDownloadedRevision(job.id, job.dl_rev);
            if (err) {
                LOG_ERROR("Failed to set downloaded revision of ("FMTu64",%s): %d", job.did, job.docname.c_str(), err);
            }
        }
    }

    return err;
}

int DocSyncDown::tryCopyback(DocSyncDownJob &job)
{
    int result = 0;
    int err = 0;
    bool drop_job = false;
    bool retry_later = false;

    LOG_DEBUG("id "FMTu64" docname %s", job.id, job.docname.c_str());

    do {
        err = jobs->TimestampJobTryCopyback(job.id);
        if (err < 0) {
            LOG_ERROR("Failed to set copyback try timestamp: docname=%s", job.docname.c_str());
            retry_later = true;
            break;
        }

        DocSyncDownDstStor sd3;
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
        extendPathToFullPath(job.dl_path, workdir, dl_path);
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
        err = jobs->TimestampJobDoneCopyback(job.id);
        if (err < 0) {
            LOG_ERROR("Failed to set copyback done timestamp: compname=%s", job.docname.c_str());
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

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    // Copyback success, update file hash value in DocTrackerDB
    err = jobs->UpdateHashbyCompId(job.compid, job.lpath);
    if(err) {
        LOG_ERROR("Failed to update hash vale for %s: %d", job.lpath.c_str(), err);
        if (!result) result = err;
    }
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    return result;
}

#if !CCD_ENABLE_SYNCDOWN_CLOUDDOC
int DocSyncDown::getLatestRevNum(DocSyncDownJob &job, /*OUT*/ u64 &revision)
{
    int err = 0;

    std::string uri;
    {
        std::ostringstream oss;
        oss << "/vcs/filemetadata/" << job.did << "/" << VPLHttp_UrlEncoding(job.docname, "/") << "?compId=" << job.compid;
        uri.assign(oss.str());
    }
    std::string metadata;
    err = VCSreadFile_helper(userId, job.did, "", "", VirtualDevice_GetDeviceId(), uri, NULL, metadata);
    if (err==CCD_ERROR_HTTP_STATUS ||
        err==CCD_ERROR_BAD_SERVER_RESPONSE)
    {
        // http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_API_Client_Side_Error_Handling#CCD_calling_VCS-v1-API_error_handling
        LOG_WARN("File assumed expired:%s", job.docname.c_str());
        return 1;  // drop job, positive error code.
    } else if (err) {
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
            return 1;
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
#endif // !CCD_ENABLE_SYNCDOWN_CLOUDDOC

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
int DocSyncDown::getLatestRevNum(DocSyncDownJob &job, /*OUT*/ u64 &revision, u32 &check_result)
{
    int err = 0;

    std::string uri;
    {
        std::ostringstream oss;
        oss << "/vcs/filemetadata/" << job.did << "/" << VPLHttp_UrlEncoding(job.docname, "/") << "?compId=" << job.compid;
        uri.assign(oss.str());
    }
    std::string metadata;
    err = VCSreadFile_helper(userId, job.did, "", "", VirtualDevice_GetDeviceId(), uri, NULL, metadata);
    if (err==CCD_ERROR_HTTP_STATUS ||
        err==CCD_ERROR_BAD_SERVER_RESPONSE)
    {
        // http://wiki.ctbg.acer.com/wiki/index.php/CCD_HTTP_API_Client_Side_Error_Handling#CCD_calling_VCS-v1-API_error_handling
        LOG_WARN("File assumed expired:%s", job.docname.c_str());
        return 1;  // drop job, positive error code.
    } else if (err) {
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
        "revisionList":[{"revision":2,
                         "size":65536,
                         "lastChanged":1351645142,
                         "updateDevice":28229213562,
                         "previewUri":"/clouddoc/preview/15540016/28229213562/temp/a9?compId=1223290246&revision=1",
                         "downloadUrl":"https://acercloud-lab1-1348691415.s3.amazonaws.com/15540016/8117365478537956100"}]}
    */
    // If the originDevice != local device, drop syncback
    // If there are more than 1 revision of 1 component ID, drop syncback
    // If updateDevice = local device, drop syncback
    // If local file does not exist, drop syncback
    //      Need to consider "Keep all" condition here. Parse the docname. "QQQ (twtpen0806026b 20130513150216).docx"
    //      Or, if the origin device == local device, but cannot find the compid in DB, it should be syncback. (Temporary solution)
    // If same path in upload request DB, drop syncback (conflict)
    // If the size is different with local copy, upload local (conflict)
    // If hash value of local file is not the same as it in DB, upload local (conflict)

    cJSON2 *json_originDevice = cJSON2_GetObjectItem(json, "originDevice");
    if(!json_originDevice) {
        LOG_ERROR("originDevice missing");
        return 1;  // drop job
    }
    if((u64)json_originDevice->valueint != VirtualDevice_GetDeviceId()) {
        LOG_INFO("Not the origin device, drop the job.");
        return 1;  // drop job
    }

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
    if (revisionListSize > 1) {
        LOG_INFO("More than 1 revision for 1 component ID, drop the job.");
        return 1;  // drop job
    }
    cJSON2 *json_compId = cJSON2_GetObjectItem(json, "compId");
    if(!json_compId) {
        LOG_ERROR("compId missing");
        return 1;  // drop job
    }
    cJSON2 *json_name = cJSON2_GetObjectItem(json, "name");
    if(!json_name) {
        LOG_ERROR("name missing");
        return 1;  // drop job
    }
    // Check file status
    bool fileErr = false;
    VPLFS_stat_t filestat;
    if (VPLFS_Stat(job.lpath.c_str(), &filestat) != VPL_OK) {
        LOG_ERROR("Failed to stat %s", job.lpath.c_str());
        fileErr = true;
    }
    // Get revision by compId and compare Hash value
    u64 sizeinDB = 0;
    u64 revInDB = 0;
    bool uploadLocal = false;
    bool createEntryinDB = false;
    std::string hashvalinDB;
    hashvalinDB.clear();
    err = jobs->GetRevisionbyCompid(json_compId->valueint, sizeinDB, hashvalinDB, revInDB);
    if(err == CCD_ERROR_NOT_FOUND) {
        // Not found in DB
        if (fileErr) {
            // The entry is created by conflict handling
            // The file will be syncdown and need to create a new entry in DocTrackerDB
            createEntryinDB = true;
        }
    }
    else if (!fileErr) {
        if(filestat.size != sizeinDB) {
            LOG_INFO("Local file size != size in DB. The file is modified without AcerCloud Portal. Drop the syncback job and upload the local file %s.", job.lpath.c_str());
            uploadLocal = true;
        } 
        else {
            // compare hash value
            if (hashvalinDB.compare(jobs->ComputeSHA1(job.lpath)) != 0) {
                // Hash value is not the same in DB
                LOG_INFO("Local file hash != hash in DB. The file is modified without AcerCloud Portal. Drop the syncback job and upload the local file %s.", job.lpath.c_str());
                uploadLocal = true;
            }
        }
    }
    else if (fileErr) {
        LOG_INFO("File is missing, and find a compID in DB. It should be a broken link.");
        return 1; // drop job
    }

    u32 count = 0;
    err = jobs->CountJobinUploadRequestbyPath(job.lpath, count);
    if (count > 0) {
        LOG_ERROR("There is a same path document in the upload queue, drop the job: %s", job.lpath.c_str());
        return 1; // drop job
    }
    u64 latestRevision = 0;
    u64 latestRevisionUpdateDevice = 0;
    u64 mtime = 0;
    cJSON2 *json_revision = NULL;
    cJSON2 *json_lastChanged = NULL;
    cJSON2 *json_updateDevice = NULL;

    for (int i = 0; i < revisionListSize; i++) {
        cJSON2 *json_revisionListEntry = cJSON2_GetArrayItem(json_revisionList, i);
        if (!json_revisionListEntry) continue;
        //cJSON2 *json_revision = cJSON2_GetObjectItem(json_revisionListEntry, "revision");
        json_revision = cJSON2_GetObjectItem(json_revisionListEntry, "revision");
        if (!json_revision) {
            LOG_ERROR("revision attr missing");
            return 1;  // drop job
        }
        if (json_revision->type != cJSON2_Number) {
            LOG_ERROR("revision attr value has wrong type");
        }
        json_lastChanged = cJSON2_GetObjectItem(json_revisionListEntry, "lastChanged");
        if (!json_lastChanged) {
            // This is an optional field:  if not present, use current time
            LOG_WARN("lastChanged attr missing");
            mtime = VPLTime_ToSec(VPLTime_GetTime());
        } else {
            mtime = (u64)json_lastChanged->valueint;
        }
        json_updateDevice = cJSON2_GetObjectItem(json_revisionListEntry, "updateDevice");
        if (!json_updateDevice) {
            LOG_ERROR("updateDevice attr missing");
            return 1;  // drop job
        }
        if (json_updateDevice->type != cJSON2_Number) {
            LOG_ERROR("updateDevice attr value has wrong type");
            return 1;  // drop job
        }
        // For conflict and keep all condition, ccd cannot find the file in local but still need to sync down it.
        if((u64)json_updateDevice->valueint == VirtualDevice_GetDeviceId() && !fileErr) {
            LOG_INFO("Last update device is the same as local, Update DB and drop the job.");
            int rv = 0;
            rv = jobs->UpdateRevInComponent((u64)json_compId->valueint, json_name->valuestring, mtime, mtime, (u64)json_revision->valueint);
            if (rv) {
                LOG_ERROR("Failed to Update Component: %d, Component id: "FMTu64", Doc name: %s", rv, (u64)json_compId->valueint, json_name->valuestring);
            }
            rv = jobs->UpdateRevInRevision((u64)json_compId->valueint, (u64)json_revision->valueint, mtime, (u64)json_updateDevice->valueint);
            if (rv) {
                LOG_ERROR("Failed to Update Revision: %d, Component id: "FMTu64", Doc name: %s, Revision: "FMTu64"", rv, (u64)json_compId->valueint, json_name->valuestring, (u64)json_revision->valueint);
            }
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

    if (uploadLocal) {
        // If not specify baseRev or baseRev = 0 here, will cause upload error
        int rv = uploadLocalDoc(job.lpath, json_compId->valueint, json_name->valuestring, revInDB);
        if (rv != 0) {
            LOG_INFO("Cannot upload local document %s. err = %d", job.lpath.c_str(), rv);
        }
        return 1;  // drop job
    }

    if (createEntryinDB) {
        // The job will be syncdown but cannot find compId in DB
        // create a new entry for the job
        int rv = 0;

        // component
        // :id, :name, :ctime, :mtime, :origin_dev, :cur_rev
        rv = jobs->WriteComponentToDB((u64)json_compId->valueint, json_name->valuestring, mtime, mtime, (u64)json_originDevice->valueint, (u64)json_revision->valueint);
        if (rv) {
            LOG_ERROR("Failed to Write Component: %d, Component id: "FMTu64", Doc name: %s", rv, (u64)json_compId->valueint, json_name->valuestring);
        }
        // revision
        // :compid, :rev, :mtime, :update_dev
        rv = jobs->WriteRevisionToDB((u64)json_compId->valueint, (u64)json_revision->valueint, mtime, (u64)json_updateDevice->valueint);
        if (rv) {
            LOG_ERROR("Failed to Write Revision: %d, Component id: "FMTu64", Doc name: %s, Revision: "FMTu64"", rv, (u64)json_compId->valueint, json_name->valuestring, (u64)json_revision->valueint);
        }
    } else {
        // The job will be syncdown and found compId in DB
        // Update c_time, m_time, cur_rev in DB
        int rv = 0;
        rv = jobs->UpdateRevInComponent((u64)json_compId->valueint, json_name->valuestring, mtime, mtime, (u64)json_revision->valueint);
        if (rv) {
            LOG_ERROR("Failed to Update Component: %d, Component id: "FMTu64", Doc name: %s", rv, (u64)json_compId->valueint, json_name->valuestring);
        }
        rv = jobs->UpdateRevInRevision((u64)json_compId->valueint, (u64)json_revision->valueint, mtime, (u64)json_updateDevice->valueint);
        if (rv) {
            LOG_ERROR("Failed to Update Revision: %d, Component id: "FMTu64", Doc name: %s, Revision: "FMTu64"", rv, (u64)json_compId->valueint, json_name->valuestring, (u64)json_revision->valueint);
        }
    }



    return err;
}

int DocSyncDown::uploadLocalDoc(std::string filePath, u64 compId, std::string docname, u64 baserev)
{
    int rv = 0;
    u64 datasetId=0;
    std::string datasetLocation;

    ccd::ListOwnedDatasetsOutput listOfDatasets;
    rv = Cache_ListOwnedDatasets(userId, listOfDatasets, true);
    if (rv != 0) {
        LOG_ERROR("Could not get list of datasets: %d", rv);
        return rv;
    }

    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;

    for (it = listOfDatasets.dataset_details().begin(); it != listOfDatasets.dataset_details().end(); it++) {
        //std::string datasetTypeName = vplex::vsDirectory::DatasetType_Name(it->datasettype());
        if(it->datasetname() == "Cloud Doc"){
            LOG_INFO("datasetid   ======>"FMTu64, it->datasetid());
            LOG_INFO("datasetname ======>%s", it->datasetname().c_str());
            datasetId = it->datasetid();
            datasetLocation = it->datasetlocation();
            break;
        }
    }

    if(it == listOfDatasets.dataset_details().end()){
        LOG_ERROR("CloudDoc dataset not found!!");
        return -1;
    }

    VPLFS_stat_t stat;

    rv = VPLFS_Stat(filePath.c_str(), &stat);
    if (rv != VPL_OK) {
        LOG_ERROR("Failed stat on upload file %s", filePath.c_str());
        return rv;
    }

    rv = DocSNGQueue_NewRequest(userId,
        datasetId,
        datasetLocation.c_str(),
        ccd::DOC_SAVE_AND_GO_UPDATE,
        docname.empty() ? NULL : urlEncode(docname).c_str(), // docname
        filePath.c_str(),
        NULL,
        stat.mtime,
        NULL, 
        compId, baserev,  
        NULL, // thumbpath
        NULL,
        0,
        NULL);  // requestid

    return rv;
}

u8 DocSyncDown::toHex(const u8 &x)
{
    return x > 9 ? x + 55: x + 48;
}

std::string DocSyncDown::urlEncode(const std::string &sIn)
{
    std::string sOut;
    for(u16 ix = 0; ix < sIn.size(); ix++) {
        u8 buf[4];
        memset( buf, 0, 4 );
        if (isalnum( (u8)sIn[ix]) ) {
            buf[0] = sIn[ix];
        } else if ( (u8)sIn[ix] == '/' || (u8)sIn[ix] == '\\' ) {
            buf[0] = sIn[ix];
        } else {
            buf[0] = '%';
            buf[1] = toHex( (u8)sIn[ix] >> 4 );
            buf[2] = toHex( (u8)sIn[ix] % 16);
        }
        sOut += (char *)buf;
    }
    return sOut;
}
#endif

int DocSyncDown::downloadRevision(DocSyncDownJob &job, u64 revision)
{
    int err = 0;

    stream_transaction transaction;
    transaction.uid = userId;
    transaction.deviceid = VirtualDevice_GetDeviceId();  // ?? doesn't seem to be used anyway
    std::ostringstream oss;
    oss << "/vcs/file/" << job.did << "/" << VPLHttp_UrlEncoding(job.docname, "/") + "?compId=" << job.compid << "&revision=" << revision;
    transaction.req = new http_request();
    transaction.req->uri = oss.str();

    std::string dl_path;
    extendPathToFullPath(job.dl_path, workdir, dl_path);
    err = Util_CreatePath(dl_path.c_str(), VPL_FALSE);
    if (err) {
        LOG_ERROR("Failed to create directory for %s", dl_path.c_str());
        return err;
    }

    transaction.out_content_file = VPLFile_Open(dl_path.c_str(), VPLFILE_OPENFLAG_WRITEONLY|VPLFILE_OPENFLAG_CREATE|VPLFILE_OPENFLAG_TRUNCATE, VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR);
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
    extern int S3_getFile(stream_transaction* transaction, std::string bucket, std::string filename);
    err = S3_getFile(&transaction, "", "");

    // Unpublish VPLHttp2 object from possible cancellation.
    {
        MutexAutoLock lock(&mutex);
        httpInProgress.erase(job.id);
    }

    return err;
}

bool DocSyncDown::isCloudDocDataset(u64 datasetId)
{
    std::string datasetName;
    int err = getDatasetName(datasetId, datasetName);
    return (!err) && (datasetName == "Cloud Doc");
}

int DocSyncDown::getDatasetName(u64 datasetId, /*OUT*/ std::string &datasetName)
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

    bool found = false;
    for (int i = 0; i < player->_cachedData.details().datasets_size(); i++) {
        const vplex::vsDirectory::DatasetDetail &dd = player->_cachedData.details().datasets(i).details();
        if (dd.datasetid() == datasetId) {
            datasetName.assign(dd.datasetname());
            found = true;
            break;
        }
    }
    if (!found) {
        LOG_ERROR("Dataset (id="FMTu64") not found", datasetId);
        result = CCD_ERROR_NOT_FOUND;
    }

    return result;
}

int DocSyncDown::Start(u64 userId)
{
    int result = 0;
    int err = 0;

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

    jobs = new DocSyncDownJobs(workdir);
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

int DocSyncDown::Stop(bool purge)
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

        err = jobs->PurgeOnClose();
        if (err) {
            LOG_ERROR("Failed to set purge-on-close: %d", err);
            // Do not return; try to clean up as much as possible.
        }
        else {
            LOG_DEBUG("SyncDownJobs obj marked to purge-on-close");
        }
    }

    if (thread_running) {
        err = signalDispatcherThreadToStop();
        if (err) {
            LOG_ERROR("Failed to signal dispatcher thread to stop: %d", err);
            result = err;
            // Do not return; try to clean up as much as possible.
        }

        // dispatch thread will destroy SyncDownJobs obj and SyncDown obj and then exit.
    }

    {
        // Must hold lock while cancelling VPLHttp2 objects.
        // Otherwise, dispatcher thread may destroy them.
        MutexAutoLock lock(&mutex);

        std::map<u64, VPLHttp2*>::iterator it;
        for (it = httpInProgress.begin(); it != httpInProgress.end(); it++) {
            it->second->Cancel();
        }
    }

    return result;
}

int DocSyncDown::NotifyDatasetContentChange(u64 datasetid,
                                         bool myOverrideAnsCheck)
{
    int result = 0;
    int err = 0;

    LOG_DEBUG("received for did "FMTu64, datasetid);

    MutexAutoLock lock(&mutex);

    overrideAnsCheck = myOverrideAnsCheck;
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

int DocSyncDown::RemoveJobsByDatasetId(u64 datasetId)
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

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
int DocSyncDown::CountUnfinishedJobsinDB(std::string docname, u64 compId, u32 &count)
{
    int err = 0;
    err = jobs->CountUnfinishedJobsinDB(docname, compId, count);
    if(err != 0) {
        LOG_ERROR("CountUnfinishedJobsinDB("FMTu64",%s):%d", compId, docname.c_str(), err);
        return err;
    }
    return err;
}
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

//----------------------------------------------------------------------

class OneDocSyncDown {
public:
    OneDocSyncDown() {
        VPLMutex_Init(&mutex);
    }
    ~OneDocSyncDown() {
        VPLMutex_Destroy(&mutex);
    }
    DocSyncDown *obj;
    VPLMutex_t mutex;
};
static OneDocSyncDown oneDocSyncDown;

int DocSyncDown_Start(u64 userId)
{
    int result = 0;

    MutexAutoLock lock(&oneDocSyncDown.mutex);
    std::string workingDir;

    {
        char docSNGPath[CCD_PATH_MAX_LENGTH];
        // docSNGPath will end in '/' and null terminate
        DiskCache::getPathForDocSNG(sizeof(docSNGPath), /*OUT*/ docSNGPath);
        workingDir = docSNGPath;  // Ends in '/'
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        // Each user will have a unique path for DocTrackerDB
        u64 localDeviceId = VirtualDevice_GetDeviceId();
        if (!localDeviceId) {
            LOG_ERROR("Could not determine local device ID");
            localDeviceId = 0;
        }
        char tempBuf[32];
        VPL_snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), "%016"PRIx64, localDeviceId);
        workingDir.append(tempBuf);
        workingDir.append("/");
        VPL_snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), "%016"PRIx64, userId);
        workingDir.append(tempBuf);
        workingDir.append("/");
        // assert: path ends in '/'
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    }
    LOG_INFO("dbpath=%s", workingDir.c_str());

    oneDocSyncDown.obj = new DocSyncDown(workingDir);

    int err = oneDocSyncDown.obj->Start(userId);
    if (err) {
        LOG_ERROR("Failed to start DocSyncDown: %d", err);
        result = err;
        oneDocSyncDown.obj = NULL;
        goto end;
    }

 end:
    return result;
}

int DocSyncDown_Stop(bool purge)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneDocSyncDown.mutex);

    if (!oneDocSyncDown.obj) {
        LOG_WARN("DocSyncDown never started");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneDocSyncDown.obj->Stop(purge);
    if (err) {
        // err msg printed by SyncDown::Stop()
        result = err;
        goto end;
    }

 end:
    // TODO: who calls the destructor on the obj?
    oneDocSyncDown.obj = NULL;  // Remove reference even if Stop() fails because there's nothing more we can do about it.
    return result;
}

int DocSyncDown_NotifyDatasetContentChange(u64 datasetId,
                                        bool overrideAnsCheck)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneDocSyncDown.mutex);

    if (!oneDocSyncDown.obj) {
        LOG_WARN("DocSyncDown not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneDocSyncDown.obj->NotifyDatasetContentChange(datasetId,
                                                      overrideAnsCheck);
    if (err) {
        LOG_ERROR("Failed to notify DocSyncDown of dataset content change: %d", err);
        result = err;
        goto end;
    }

 end:
    return result;
}

int DocSyncDown_RemoveJobsByDataset(u64 datasetId)
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneDocSyncDown.mutex);

    if (!oneDocSyncDown.obj) {
        LOG_WARN("DocSyncDown not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    LOG_DEBUG("did "FMTu64, datasetId);

    err = oneDocSyncDown.obj->RemoveJobsByDatasetId(datasetId);
    if (err) {
        LOG_ERROR("Failed to remove jobs for dataset "FMTu64": %d", datasetId, err);
        result = err;
        goto end;
    }

 end:
    return result;
}

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
int CountUnfinishedJobsinDB(std::string name, u64 compId, u32 &count)
{
    int rv = 0;
    int err = 0;

    MutexAutoLock lock(&oneDocSyncDown.mutex);

    if (!oneDocSyncDown.obj) {
        LOG_WARN("DocSyncDown not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneDocSyncDown.obj->CountUnfinishedJobsinDB(name, compId, count);
    if (err) {
        LOG_ERROR("CountUnfinishedJobsinDB: %s, compID: "FMTu64", err = %d", name.c_str(), compId, err);
        rv = err;
        goto end;
    }

end:
    return rv;
}
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

// ------------------ DocSNGQueue ------------------- //

static DocSNGQueue docSNGQueue;

DocSNGQueue::DocSNGQueue()
:  isRunning(false),
   isActive(false),
   db(NULL),
   activeTicket(false),
   currentUserId(0),
   isReady(false),
   vcs_handler(NULL)
{ }

int DocSNGQueue::init(const std::string &rootdir,
                      int (*completionCb)(int change_type,
                                          const char *name,
                                          const char *newname,
                                          u64 modify_time,
                                          int result,
                                          const char *docname,
                                          u64 comp_id,
                                          u64 revision),
                      int (*engineStateChangeCb)(bool engine_started))
{
    if (isReady) {
        LOG_ERROR("Already initialized");
        return CCD_ERROR_ALREADY_INIT;
    }
    VPLMutex_Init(&m_mutex);

    this->rootdir = rootdir;
    tmpdir = rootdir + "tmp/";
    dbpath = rootdir + "db";

    VPLDir_Create(rootdir.c_str(), 0777);
    VPLDir_Create(tmpdir.c_str(), 0777);

    LOG_INFO("rootdir=%s", rootdir.c_str());
    LOG_INFO("tmpdir=%s", tmpdir.c_str());
    LOG_INFO("dbpath=%s", dbpath.c_str());

    this->completionCb = completionCb;
    this->engineStateChangeCb = engineStateChangeCb;

    isReady = true;

    return 0;
}

int DocSNGQueue::release()
{
    if (isReady) {
        if (db)
            closeDB();
        VPLMutex_Destroy(&m_mutex);
        isReady = false;
    }
    return 0;
}

int DocSNGQueue::setVcsHandlerFunction(VPLDetachableThread_fn_t handler)
{
    VPLMutex_Lock(&m_mutex);
    vcs_handler = handler;
    VPLMutex_Unlock(&m_mutex);
    return 0;
}

DocSNGQueue::~DocSNGQueue()
{
    release();
}

// This function is called if the ticket could not be processed and needs to be retried
int DocSNGQueue::resetTicket(DocSNGTicket *tp)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rv = 0;

#ifdef USE_QUEUE_OPTIMIZATION
    // This means that the "queue optimization" logic (currently disabled) decided this ticket
    // was redundant.  Delete it and skip to the next ticket.
    if (tp->is_out_of_date()) {
        rv = dequeue(tp);
        delete tp;
        activateNextTicket();
        return rv;
    }
#endif // USE_QUEUE_OPTIMIZATION

#if 0
    // In this case, retry immediately
    // FIXME: this is too dangerous without a backoff mechanism.  Could be a tight loop.
    if (tp->result == CCD_ERROR_TRANSIENT) {
        VPLMutex_Lock(&m_mutex);
        activeTicket = NULL;
        VPLMutex_Unlock(&m_mutex);
        delete tp;
        activateNextTicket();
        return rv;
    }
#endif

    //
    // If the request failed for some other retryable transient error, keep the ticket and
    // temporarily stop processing any more tickets.
    // FIXME: implement controlled backoff and restart policy
    //
    VPLMutex_Lock(&m_mutex);
    activeTicket = NULL;
    isRunning = false;
    VPLMutex_Unlock(&m_mutex);

    LOG_INFO("Disabling queue processing due to transient error");

    if (isActive) {
        // Notify EventManagerPb that queue processing has gone to sleep.
        (*engineStateChangeCb)(false);
    }
    isActive = false;

    // Free current copy (it's still on the queue)
    delete tp;

    return rv;
}

// This function is called if the request in the ticket was processed or failed in a non-retryable way
int DocSNGQueue::completeTicket(DocSNGTicket *tp)
{
    int rv = 0;
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    if(tp->result != 0) {
        // operation failed with an unrecoverable error, just delete ticket
        LOG_ERROR("Operation type %d failed (result %d), drop ticket", tp->operation, tp->result);
    }
    else if(tp->operation == DOC_SNG_UPLOAD) {
        // upload done, update DocTrackerDB
        rv = writeComponentToDB(tp);
        if (rv) {
            LOG_ERROR("Failed to Write Component: %d, Component id: "FMTu64", Doc name: %s", rv, tp->compid, tp->docname.c_str());
        }
        rv = writeRevisionToDB(tp);
        if (rv) {
            LOG_ERROR("Failed to Write Revision: %d, Component id: "FMTu64", Doc name: %s, Revision: "FMTu64"", rv, tp->compid, tp->docname.c_str(), tp->baserev);
        }
    }
    else if(tp->operation == DOC_SNG_RENAME) {
        // rename done, update DocTrackerDB
        // Get the Hash value in DB
        if(tp->origin_device == VirtualDevice_GetDeviceId()) {
            rv = getHash_by_compid(tp->compid, tp->hashval);
            if (rv) {
                LOG_ERROR("Failed to get hash value: %d, Component id: "FMTu64", Doc name: %s", rv, tp->compid, tp->docname.c_str());
            } else {
                rv = writeComponentToDB(tp);
                if (rv) {
                    LOG_ERROR("Failed to Write Component: %d, Component id: "FMTu64", Doc name: %s", rv, tp->compid, tp->docname.c_str());
                }
                rv = writeRevisionToDB(tp);
                if (rv) {
                    LOG_ERROR("Failed to Write Revision: %d, Component id: "FMTu64", Doc name: %s, Revision: "FMTu64"", rv, tp->compid, tp->docname.c_str(), tp->baserev);
                }
            }
        }
    }
    else if(tp->operation == DOC_SNG_DELETE) {
        // delete done, update DocTrackerDB
        if(tp->compid == 0) {
            // Delete from File explorer directly
            rv = getCompid_by_name(tp);
            if (rv) {
                LOG_ERROR("Failed to Get Component ID: %d, Doc name: %s", rv, tp->docname.c_str());
            }
        }
        rv = deleteRevisionFromDB(tp);
        if (rv) {
            LOG_ERROR("Failed to Delete Revision: %d, Component id: "FMTu64", Doc name: %s, Revision: "FMTu64"", rv, tp->compid, tp->docname.c_str(), tp->baserev);
        }
        rv = deleteComponentFromDB(tp);
        if (rv) {
            LOG_ERROR("Failed to Delete Component: %d, Component id: "FMTu64", Doc name: %s", rv, tp->compid, tp->docname.c_str());
        }
    }
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

    rv = dequeue(tp);
    delete tp;
    activateNextTicket();

    return rv;
}

int DocSNGQueue::startProcessing(u64 userId)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    bool addTask = false;

    {
        MutexAutoLock lock(&m_mutex);
        // TODO: Bug 16520. Lock got expanded to here to prevent heap corruption (quick hack).
        //       This code seems awfully similar to the init() code above.  Do
        //       we even need it here and why?

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        {
            char docSNGPath[CCD_PATH_MAX_LENGTH];
            // docSNGPath will end in '/' and null terminate
            DiskCache::getPathForDocSNG(sizeof(docSNGPath), /*OUT*/ docSNGPath);
            // Each user will have a unique path for DocTrackerDB
            u64 localDeviceId = VirtualDevice_GetDeviceId();
            if (!localDeviceId) {
                LOG_ERROR("Could not determine local device ID");
                localDeviceId = 0;
            }
            std::string workingDir = docSNGPath;  // Ends in '/'
            char tempBuf[32];
            VPL_snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), "%016"PRIx64, localDeviceId);
            workingDir.append(tempBuf);
            workingDir.append("/");
            VPL_snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), "%016"PRIx64, userId);
            workingDir.append(tempBuf);
            workingDir.append("/");
            // assert: path ends in '/'
            rootdir.assign(workingDir);
        }
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        tmpdir = rootdir + "tmp/";
        dbpath = rootdir + "db";

        int err = 0;
        err = Util_CreatePath(rootdir.c_str(), VPL_TRUE);
        if (err) {
            LOG_ERROR("Failed to create directory %s", rootdir.c_str());
        }
        err = Util_CreatePath(tmpdir.c_str(), VPL_TRUE);
        if (err) {
            LOG_ERROR("Failed to create directory %s", tmpdir.c_str());
        }

        LOG_DEBUG("rootdir=%s", rootdir.c_str());
        LOG_DEBUG("tmpdir=%s", tmpdir.c_str());
        LOG_DEBUG("dbpath=%s", dbpath.c_str());


        if (!isRunning) {
            isRunning = true;
            addTask = true;
            currentUserId = userId;
        }
    }

    if (addTask) {
        activateNextTicket();
        LOG_INFO("Queue processing enabled for uid "FMTu64, userId);
    }

    return 0;
}

int DocSNGQueue::stopProcessing(bool purge)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    VPLMutex_Lock(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC
    isRunning = false;

    if (purge) {
        removeAllRequests();
        removeAllTmpFiles();
        LOG_INFO("Removed all requests");
    }

    if (db)
        closeDB();

    VPLMutex_Unlock(&m_mutex);

    LOG_INFO("Queue processing disabled");

    return 0;
}

bool DocSNGQueue::isProcessing()
{
    if (!isReady)
        return false;

    VPLMutex_Lock(&m_mutex);
    bool result = isActive;
    VPLMutex_Unlock(&m_mutex);

    return result;
}

bool DocSNGQueue::isEnabled()
{
    if (!isReady)
        return false;

    VPLMutex_Lock(&m_mutex);
    bool result = isRunning;
    VPLMutex_Unlock(&m_mutex);

    return result;
}

static bool add_column_if_missing(sqlite3 *db, const char *colname, const char *coltype)
{
    bool found = false;

    int rc = 0;
    int rv;

    std::string sql;
    {
        std::ostringstream oss;
        oss << "SELECT sql FROM sqlite_master WHERE tbl_name='request' AND sql LIKE '%" << colname << " %'";
        sql = oss.str();
    }

    sqlite3_stmt *stmt = NULL;
    rv = sqlite3_prepare_v2(db, sql.data(), sql.size(), &stmt, NULL);
    CHECK_PREPARE(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    if (rv == SQLITE_DONE) {  // column missing
        LOG_INFO("Adding %s column", colname);
        {
            std::ostringstream oss;
            oss << "ALTER TABLE request ADD COLUMN " << colname << " " << coltype;
            sql = oss.str();
        }
        char *errmsg = NULL;
        rv = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            LOG_WARN("Failed to add column %s: %d, %s", colname, rv, errmsg);
            sqlite3_free(errmsg);
            goto end;
        }
    }

 end:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    return found;
}

int DocSNGQueue::openDB()
{
    int rv = 0;
    char *errmsg = NULL;
    int retry = 0;

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    bool needMigrate = false;
    VPLFS_stat_t filestat;
    if (VPLFS_Stat(dbpath.c_str(), &filestat) != VPL_OK) {
        LOG_INFO("First time to create DB. Check if user has old dsng DB.");
        const char *rootPath = NULL;
        char oldDBpath[CCD_PATH_MAX_LENGTH];
        rootPath = CCDStorage_GetRoot();
        snprintf(oldDBpath, CCD_PATH_MAX_LENGTH, "%s/dsng/db", rootPath);
        if(VPLFS_Stat(oldDBpath, &filestat) == VPL_OK) {
            // Migrate DB after the DocTrackerDB is created.
            LOG_INFO("User has old DSNG DB. Migrate jobs from old DSNG upload DB to DocTrackerDB.");
            needMigrate = true;
        } else {
            LOG_INFO("User did not have old DSNG DB. Skip migration.");
            needMigrate = false;
        }
    }
#endif

    // Bug 14651 (https://bugs.ctbg.acer.com/show_bug.cgi?id=14651)
    // sqlite3_exec may result in SQLITE_BUSY. It happened rarely, but it did.
    // Add busy handler code, but still make this case as error for observation.
    // The final modification will be done before next release, which should be CCD 3.0.

    while(retry < 5) {

        rv = Util_OpenDbHandle(dbpath.c_str(), true, true, &db);
        if (rv != 0) {
            LOG_ERROR("Util_OpenDbHandle(%s):%d",
                      dbpath.c_str(), rv);
            goto end;
        }

        rv = sqlite3_exec(db, test_and_create_db_sql_1, NULL, NULL, &errmsg);
        if (rv == SQLITE_BUSY) {

            LOG_ERROR("Failed to create tables: %d, %s, retry=%d", rv, errmsg, retry);
            sqlite3_free(errmsg);

            rv = closeDB();
            if (rv != SQLITE_OK) {
                LOG_WARN("Failed to close DB");
            }

            retry++;
            if (retry >= 5) {
                LOG_ERROR("OpenDB failed after 5 times retry!");
            }
            VPLTime_SleepMs(100);
        } else if (rv != SQLITE_OK) {
            LOG_ERROR("Failed to create tables: %d, %s", rv, errmsg);
            sqlite3_free(errmsg);
            goto end;
        } else { //rv == SQLITE_OK
            break;
        }

    }

    // New columns added for v1.1.
    add_column_if_missing(db, "dloc", "TEXT");
    add_column_if_missing(db, "compid", "INTEGER");
    add_column_if_missing(db, "baserev", "INTEGER");
    add_column_if_missing(db, "docname", "TEXT");
    add_column_if_missing(db, "conttype", "TEXT");
    add_column_if_missing(db, "retry", "INTEGER");

    LOG_INFO("Opened DB");

    dbstmts.resize(STMT_MAX, NULL);

    // check db schema version
    // If schema version isn't latest, apply schema updates.
    rv = updateSchema();
    if (rv != 0) {
        LOG_ERROR("Failed to update database schema: %d", rv);
        goto end;
    }

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    // Migrate DB if necessary
    if(needMigrate) {
        migrateDB();
    }
#endif

 end:
    return rv;
}

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
int DocSNGQueue::migrateDB()
{
    int result = 0;
    int dberr = 0;

    u64 id = 0;
    u64 uid = 0;
    u64 did = 0;
    u64 type = 0;
    std::string name;
    std::string newname;
    u64 mtime = 0;
    std::string fpath;
    std::string tpath;
    std::string dloc;
    u64 compid = 0;
    u64 baserev = 0;
    std::string docname;
    std::string conttype;
    u64 retry = 0;

    name.clear();
    newname.clear();
    fpath.clear();
    tpath.clear();
    dloc.clear();
    docname.clear();
    conttype.clear();

    sqlite3 *oldDB;
    const char *rootPath = NULL;
    char oldDBpath[CCD_PATH_MAX_LENGTH];
    rootPath = CCDStorage_GetRoot();
    snprintf(oldDBpath, CCD_PATH_MAX_LENGTH, "%s/dsng/db", rootPath);

    VPLMutex_Lock(&m_mutex);
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));

    static const char* SQL_SELECT_ALL_REQUEST =
        "SELECT id, uid, did, type, name, newname, mtime, fpath, tpath, dloc, compid, baserev, docname, conttype, retry " \
        "FROM request ";

    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *&stmt2 = dbstmts[SQLNUM(INSERT_REQUEST_TO_NEWDB)];

    // Bug 13457: The old dsng DB is not a WAL-enabled DB.
    // If we would like to open it as READONLY, we should not use Util_OpenDbHandle here.
    // Util_OpenDbHandle will try to transform the db to WAL-enabled DB.
    // However, it will fail if it is opened as READONLY.
    dberr = sqlite3_open_v2(oldDBpath, &oldDB, SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, NULL);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to open db at %s: %d", oldDBpath, dberr);
        goto end;
    }

    if (!stmt) {
        dberr = sqlite3_prepare_v2(oldDB, SQL_SELECT_ALL_REQUEST, strlen(SQL_SELECT_ALL_REQUEST) + 1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, oldDB, end);
    }

    if (!stmt2) {
        dberr = sqlite3_prepare_v2(db, GETSQL(INSERT_REQUEST_TO_NEWDB), -1, &stmt2, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        // read from old DB
        int n = 0;
        id = (u64)sqlite3_column_int64(stmt, n++);
        uid = (u64)sqlite3_column_int64(stmt, n++);
        did = (u64)sqlite3_column_int64(stmt, n++);
        type = (u64)sqlite3_column_int64(stmt, n++);
        const char *tmp_name = (const char*)sqlite3_column_text(stmt, n++);
        if (tmp_name) {
            name.assign(tmp_name);
        }
        const char *tmp_newname = (const char*)sqlite3_column_text(stmt, n++);
        if (tmp_newname) {
            newname.assign(tmp_newname);
        }
        mtime = (u64)sqlite3_column_int64(stmt, n++);
        const char *tmp_fpath = (const char*)sqlite3_column_text(stmt, n++);
        if (tmp_fpath) {
            fpath.assign(tmp_fpath);
        }
        const char *tmp_tpath = (const char*)sqlite3_column_text(stmt, n++);
        if (tmp_tpath) {
            tpath.assign(tmp_tpath);
        }
        const char *tmp_dloc = (const char*)sqlite3_column_text(stmt, n++);
        if (tmp_dloc) {
            dloc.assign(tmp_dloc);
        }
        compid = (u64)sqlite3_column_int64(stmt, n++);
        baserev = (u64)sqlite3_column_int64(stmt, n++);
        const char *tmp_docname = (const char*)sqlite3_column_text(stmt, n++);
        if (tmp_docname) {
            docname.assign(tmp_docname);
        }
        const char *tmp_conttype = (const char*)sqlite3_column_text(stmt, n++);
        if (tmp_conttype) {
            conttype.assign(tmp_conttype);
        }
        retry = (u64)sqlite3_column_int64(stmt, n++);

        // Insert in New DB
        n = 1;
        // bind :id
        dberr = sqlite3_bind_int64(stmt2, n++, id);
        CHECK_BIND(dberr, result, db, end);
        // bind :uid
        dberr = sqlite3_bind_int64(stmt2, n++, uid);
        CHECK_BIND(dberr, result, db, end);
        // bind :did
        dberr = sqlite3_bind_int64(stmt2, n++, did);
        CHECK_BIND(dberr, result, db, end);
        // bind :type
        dberr = sqlite3_bind_int(stmt2, n++, type);
        CHECK_BIND(dberr, result, db, end);
        // bind :name
        if (!name.empty()) {
            dberr = sqlite3_bind_text(stmt2, n++, name.c_str(), -1, NULL);
        }
        else {
            // Name will be NULL for a delete request
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :newname
        if (!newname.empty()) {
            dberr = sqlite3_bind_text(stmt2, n++, newname.c_str(), -1, NULL);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :mtime
        if (mtime) {
            dberr = sqlite3_bind_int64(stmt2, n++, mtime);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :fpath
        if (!fpath.empty()) {
            dberr = sqlite3_bind_text(stmt2, n++, fpath.c_str(), -1, NULL);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :tpath
        if (!tpath.empty()) {
            dberr = sqlite3_bind_text(stmt2, n++, tpath.c_str(), -1, NULL);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :dloc
        if (!dloc.empty()) {
            dberr = sqlite3_bind_text(stmt2, n++, dloc.c_str(), -1, NULL);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :compid
        if (compid != 0) {
            dberr = sqlite3_bind_int64(stmt2, n++, compid);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :baserev
        if (baserev != 0) {
            dberr = sqlite3_bind_int64(stmt2, n++, baserev);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :docname
        if (!docname.empty()) {
            dberr = sqlite3_bind_text(stmt2, n++, docname.c_str(), -1, NULL);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :conttype
        if (!conttype.empty()) {
            dberr = sqlite3_bind_text(stmt2, n++, conttype.c_str(), -1, NULL);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);
        // bind :retry
        if (retry != 0) {
            dberr = sqlite3_bind_int64(stmt2, n++, retry);
        }
        else {
            dberr = sqlite3_bind_null(stmt2, n++);
        }
        CHECK_BIND(dberr, result, db, end);

        dberr = sqlite3_step(stmt2);
        CHECK_STEP(dberr, result, db, end);

        if (stmt2) {
            sqlite3_reset(stmt2);
        }
    }

end:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    if (stmt2) {
        sqlite3_reset(stmt2);
    }
    if(oldDB) {
        sqlite3_close(oldDB);
        oldDB = NULL;
        LOG_INFO("Closed old DSNG DB");
    }

    VPLMutex_Unlock(&m_mutex);
    return result;
}
#endif

int DocSNGQueue::updateSchema()
{
    int rv;
    u64 schemaVersion;

    rv = getVersion(VERID_SCHEMA_VERSION, schemaVersion);
    if (rv != 0) {
        LOG_ERROR("Failed to get current schema version: %d", rv);
        return rv;
    }
    if (schemaVersion < (u64)TARGET_SCHEMA_VERSION) {
        // BEGIN
        rv = beginTransaction();
        if (rv != 0) {
            LOG_ERROR("Failed to update schema version to "FMTu64": %d", 
                (u64)TARGET_SCHEMA_VERSION, rv);
            return rv;
        }

        // Apply updates ad needed.
        //switch(schemaVersion) {
        //default: // the first version, no update need         
        //    break;
        //}

        // COMMIT
        rv = commitTransaction();
        if (rv != 0) {
            LOG_ERROR("Failed to update schema version to "FMTu64": %d", 
                (u64)TARGET_SCHEMA_VERSION, rv);
            return rv;
        }

    }
    else if (schemaVersion > TARGET_SCHEMA_VERSION) {
        // DatasetDB file created by future version of DatasetDB.cpp
        // Nothing could be done.  Close DB and return error.
        closeDB();
        return -16987; //DATASETDB_ERR_FUTURE_SCHEMA_VERSION;
    }
    // ELSE schema version in db file is expected version - no action necessary

    return 0;
}


int DocSNGQueue::getVersion(int versionId, u64 &version)
{
    int result = 0;
    int dberr;

    //db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_VERSION)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_VERSION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int(stmt, 1, versionId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_DONE) {
        result = -16993; //DATASETDB_ERR_UNKNOWN_VERSION;
        goto end;
    }
    // assert: dberr == SQLITE_ROW
    if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER) {
        result = -16995; //DATASETDB_ERR_BAD_DATA;
        goto end;
    }
    version = sqlite3_column_int64(stmt, 0);

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    //db_unlock();
    return result;
}

int DocSNGQueue::closeDB()
{
    int rv = 0;

    std::vector<sqlite3_stmt*>::iterator it;
    for (it = dbstmts.begin(); it != dbstmts.end(); it++) {
        if (*it != NULL) {
            sqlite3_finalize(*it);
        }
        *it = NULL;
    }

    rv = sqlite3_close(db);
    if (rv != SQLITE_OK) {
        LOG_ERROR("Failed to close db: %d", rv);
    }
    db = NULL;

    LOG_INFO("Closed DB");

    dbstmts.clear();

    return rv;
}

int DocSNGQueue::beginTransaction()
{
    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(BEGIN)];
    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(BEGIN), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}

int DocSNGQueue::commitTransaction()
{
    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COMMIT)];
    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(COMMIT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}

int DocSNGQueue::rollbackTransaction()
{
    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ROLLBACK)];
    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(ROLLBACK), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}

int DocSNGQueue::enqueue(u64 uid,
                         u64 did,
                         const char *dloc,
                         int change_type,
                         const char *docname,
                         const char *name,
                         const char *newname,
                         u64 modtime,
                         const char *contentType,
                         u64 compid, u64 baserev,
                         const char *thumbpath,
                         const char *thumbdata, u32 thumbsize,
                         u64 *requestid)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    // You cannot provide both thumbpath and thumbdata.
    if (change_type == DOC_SNG_UPLOAD && thumbpath && thumbdata)
        return CCD_ERROR_PARAMETER;

    MutexAutoLock dblock(&m_mutex);

    {
        int err = 0;
        CacheAutoLock autoLock;
        err = autoLock.LockForRead();
        if (err < 0) {
            LOG_ERROR("Failed to obtain lock");
            return err;
        }

         CachePlayer *player = cache_getUserByUserId(uid);
         if(!player->_cachedData.mutable_details()->enable_clouddoc_sync())
             return CCD_ERROR_FEATURE_DISABLED;
    }
 
    int rc = 0;
    int rv;
    u64 id = 0;

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt_ins = dbstmts[SQLNUM(INSERT_REQUEST)];
    sqlite3_stmt *&stmt_upd = dbstmts[SQLNUM(UPDATE_PATHS)];

    rv = beginTransaction();
    CHECK_ERR(rv, rc, end);

    if (!stmt_ins) {
        rv = sqlite3_prepare_v2(db, GETSQL(INSERT_REQUEST), -1, &stmt_ins, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    // bind :uid
    rv = sqlite3_bind_int64(stmt_ins, 1, uid);
    CHECK_BIND(rv, rc, db, end);
    // bind :did
    rv = sqlite3_bind_int64(stmt_ins, 2, did);
    CHECK_BIND(rv, rc, db, end);
    // bind :type
    rv = sqlite3_bind_int(stmt_ins, 3, change_type);
    CHECK_BIND(rv, rc, db, end);
    // bind :name
    if (name) {
        rv = sqlite3_bind_text(stmt_ins, 4, name, -1, NULL);
    }
    else {
        // Name will be NULL for a delete request
        rv = sqlite3_bind_null(stmt_ins, 4);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :newname
    if (newname) {
        rv = sqlite3_bind_text(stmt_ins, 5, newname, -1, NULL);
    }
    else {
        rv = sqlite3_bind_null(stmt_ins, 5);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    if (modtime) {
        rv = sqlite3_bind_int64(stmt_ins, 6, modtime);
    }
    else {
        rv = sqlite3_bind_null(stmt_ins, 6);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :dloc
    if (dloc) {
        rv = sqlite3_bind_text(stmt_ins, 7, dloc, -1, NULL);
    }
    else {
        rv = sqlite3_bind_null(stmt_ins, 7);
    }
    // bind :compid
    if (compid != 0) {
        rv = sqlite3_bind_int64(stmt_ins, 8, compid);
    }
    else {
        rv = sqlite3_bind_null(stmt_ins, 8);
    }
    // bind :baserev
    if (baserev != 0) {
        rv = sqlite3_bind_int64(stmt_ins, 9, baserev);
    }
    else {
        rv = sqlite3_bind_null(stmt_ins, 9);
    }
    // bind :docname
    if (docname != NULL) {
        rv = sqlite3_bind_text(stmt_ins, 10, docname, -1, NULL);
    }
    else {
        rv = sqlite3_bind_null(stmt_ins, 10);
    }
    // bind :conttype
    if (contentType != NULL) {
        rv = sqlite3_bind_text(stmt_ins, 11, contentType, -1, NULL);
    }
    else {
        rv = sqlite3_bind_null(stmt_ins, 11);
    }
    // bind :retry
    {
        rv = sqlite3_bind_null(stmt_ins, 12);
    }

    rv = sqlite3_step(stmt_ins);
    CHECK_STEP(rv, rc, db, end);

    id = sqlite3_last_insert_rowid(db);

    if (change_type == DOC_SNG_UPLOAD) {
        std::string fpath, tpath;

        {
            std::ostringstream fpathbuf;
            // To test Bug 8267, temporarily change the string below to anything that ends with "tmp/"
            // E.g., fpathbuf << "/a/b/c/d/e/f/tmp/" << id << DOC_TMPFILE_SUFFIX;
            fpathbuf << "tmp/" << id << DOC_TMPFILE_SUFFIX;
            fpath = fpathbuf.str();
            std::string ffullpath;
            extendPathToFullPath(fpath, rootdir, ffullpath);
            // TODO: need version of this which calculates hash at the same time
            rv = ::copy_file(name, ffullpath.c_str());
            CHECK_ERR(rv, rc, end);
        }

        if (thumbpath) {
            std::ostringstream tpathbuf;
            // To test Bug 8267, temporarily change the string below to anything that ends with "tmp/"
            // E.g., tpathbuf << "/a/b/c/d/e/f/tmp/" << id << THUMB_TMPFILE_SUFFIX;
            tpathbuf << "tmp/" << id << THUMB_TMPFILE_SUFFIX;
            tpath = tpathbuf.str();
            std::string tfullpath;
            extendPathToFullPath(tpath, rootdir, tfullpath);
            rv = ::copy_file(thumbpath, tfullpath.c_str());
            CHECK_ERR(rv, rc, end);
        }
        else if (thumbdata) {
            std::ostringstream tpathbuf;
            // To test Bug 8267, temporarily change the string below to anything that ends with "tmp/"
            // E.g., tpathbuf << "/a/b/c/d/e/f/tmp/" << id << THUMB_TMPFILE_SUFFIX;
            tpathbuf << "tmp/" << id << THUMB_TMPFILE_SUFFIX;
            tpath = tpathbuf.str();
            std::string tfullpath;
            extendPathToFullPath(tpath, rootdir, tfullpath);
            rv = ::dump_thumb(thumbdata, thumbsize, tfullpath.c_str());
            CHECK_ERR(rv, rc, end);
        }

        if (!stmt_upd) {
            rv = sqlite3_prepare_v2(db, GETSQL(UPDATE_PATHS), -1, &stmt_upd, NULL);
            CHECK_PREPARE(rv, rc, db, end);
        }

        // bind :fpath
        rv = sqlite3_bind_text(stmt_upd, 1, fpath.data(), fpath.size(), NULL);
        CHECK_BIND(rv, rc, db, end);
        // bind :tpath
        if (thumbpath || thumbdata) {
            rv = sqlite3_bind_text(stmt_upd, 2, tpath.data(), tpath.size(), NULL);
        }
        else {
            rv = sqlite3_bind_null(stmt_upd, 2);
        }
        CHECK_BIND(rv, rc, db, end);

        // bind :id
        rv = sqlite3_bind_int64(stmt_upd, 3, id);
        CHECK_BIND(rv, rc, db, end);

        rv = sqlite3_step(stmt_upd);
        CHECK_STEP(rv, rc, db, end);
    }

    rv = commitTransaction();
    CHECK_ERR(rv, rc, end);

    LOG_INFO("Added request (%d,%s) as ID "FMTu64, change_type, docname, id);

    if (isRunning) {
        activateNextTicket();
    }

 end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt_ins) {
        sqlite3_reset(stmt_ins);
    }
    if (stmt_upd) {
        sqlite3_reset(stmt_upd);
    }

    if (rc == 0 && requestid != NULL)
        *requestid = id;

    return rc;
}

int DocSNGQueue::dequeue(DocSNGTicket *tp)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_REQUEST)];

    VPLMutex_Lock(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_REQUEST), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt, 1, tp->ticket_id);
    CHECK_BIND(rv, rc, db, dbcleanup);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, dbcleanup);

    if (sqlite3_changes(db) == 0) {
        // no rows deleted; this is unexpected.  emit warning
        LOG_WARN("Ticket ID "FMTu64" not found in queue", tp->ticket_id);
    }
    else {
        LOG_INFO("Removed request ID "FMTu64, tp->ticket_id);
    }

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    activeTicket = NULL;

    // Remove any temporary files created for this ticket.
    if (!tp->srcPath.empty()) {
        VPLFile_Delete(tp->srcPath.c_str());
    }
    if (!tp->thumbPath.empty()) {
        VPLFile_Delete(tp->thumbPath.c_str());
    }

    VPLMutex_Unlock(&m_mutex);

    // Notify EventManagerPb of completion.
    bool is_v1_1 = tp->dloc.find("VCS") != tp->dloc.npos;
    (*completionCb)(tp->operation, 
                    tp->origName.c_str(), 
                    tp->newName.empty() ? NULL : tp->newName.c_str(), 
                    tp->modifyTime,
                    tp->result,
                    is_v1_1 ? tp->docname.c_str() : NULL,
                    is_v1_1 ? tp->compid : 0,
                    is_v1_1 ? tp->baserev : 0);

    return rc;
}

int DocSNGQueue::removeAllRequests()
{
    int result = 0;
    int dberr = 0;

    if (!db) {
        dberr = openDB();
        if(dberr != 0) {
            return dberr;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_ALL_REQUESTS)];
    if (!stmt) {
        dberr = sqlite3_prepare_v2(db, GETSQL(DELETE_ALL_REQUESTS), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, dbcleanup);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, dbcleanup);

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    return result;
}

int DocSNGQueue::removeAllTmpFiles()
{
    int result = 0;

    {
        int err = 0;
        VPLFS_dir_t dir;
        VPLFS_dirent_t dirent;

        err = VPLFS_Opendir(tmpdir.c_str(), &dir);
        if (err) {
            LOG_ERROR("Failed to open tmpdir %s: %d", tmpdir.c_str(), err);
            goto end;
        }
        ON_BLOCK_EXIT(VPLFS_Closedir, &dir);

        while ((err = VPLFS_Readdir(&dir, &dirent)) == VPL_OK) {
            if (dirent.type != VPLFS_TYPE_FILE) continue;
            size_t filenamelen = strlen(dirent.filename);
            if (strcmp(dirent.filename + filenamelen - strlen(DOC_TMPFILE_SUFFIX), DOC_TMPFILE_SUFFIX) == 0 ||
                strcmp(dirent.filename + filenamelen - strlen(THUMB_TMPFILE_SUFFIX), THUMB_TMPFILE_SUFFIX) == 0) {
                std::string tmpfilepath = tmpdir + dirent.filename;
                err = VPLFile_Delete(tmpfilepath.c_str());
                if (err) {
                    LOG_ERROR("Failed to delete tmpfile %s: %d", tmpfilepath.c_str(), err);
                    // don't exit the loop but continue deleting other tmp files
                }
            }
        }
    }

 end:
    return result;
}

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
std::string DocSNGQueue::calculateRowChecksum(std::string input)
{
    CSL_ShaContext hashCtx;
    std::string rowChecksum;
    u8 hashVal[CSL_SHA1_DIGESTSIZE];

    CSL_ResetSha(&hashCtx);
    CSL_InputSha(&hashCtx, input.c_str(), input.length());
    CSL_ResultSha(&hashCtx, hashVal);
    rowChecksum.clear();
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        char byteStr[4];
        snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, hashVal[hashIndex]);
        rowChecksum.append(byteStr);
    }

    return rowChecksum;
}

int DocSNGQueue::writeComponentToDB(DocSNGTicket *tp)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;
    std::stringstream ss;
    std::string rowChecksum;

    MutexAutoLock lock1(&m_mutex);

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt_ins = dbstmts[SQLNUM(INSERT_OR_REPLACE_COMPONENT)];

    rv = beginTransaction();
    CHECK_ERR(rv, rc, end);

    if (!stmt_ins) {
        rv = sqlite3_prepare_v2(db, GETSQL(INSERT_OR_REPLACE_COMPONENT), -1, &stmt_ins, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    //:id, :name, :ctime, :mtime, :origin_dev, :cur_dev, :cksum
    // bind :id
    rv = sqlite3_bind_int64(stmt_ins, 1, tp->compid);
    CHECK_BIND(rv, rc, db, end);
    // bind :name
    rv = sqlite3_bind_text(stmt_ins, 2, tp->docname.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :ctime
    rv = sqlite3_bind_int64(stmt_ins, 3, tp->modifyTime);
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    rv = sqlite3_bind_int64(stmt_ins, 4, tp->modifyTime);
    CHECK_BIND(rv, rc, db, end);
    // bind :origin_dev
    rv = sqlite3_bind_int64(stmt_ins, 5, tp->origin_device);
    CHECK_BIND(rv, rc, db, end);
    // bind :cur_rev
    rv = sqlite3_bind_int64(stmt_ins, 6, tp->baserev);
    CHECK_BIND(rv, rc, db, end);
    // bind :cksum
    ss.str("");
    ss << tp->compid << "," << tp->docname.c_str() << "," << tp->modifyTime << "," << tp->modifyTime << "," << tp->origin_device << "," << tp->baserev << "," << '\0';
    rowChecksum = calculateRowChecksum(ss.str());

    rv = sqlite3_bind_text(stmt_ins, 7, rowChecksum.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_ins);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    CHECK_ERR(rv, rc, end);

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt_ins) {
        sqlite3_reset(stmt_ins);
    }

    return rc;
}

int DocSNGQueue::writeRevisionToDB(DocSNGTicket *tp)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;
    std::stringstream ss;
    std::string rowChecksum;

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt_ins = dbstmts[SQLNUM(INSERT_OR_REPLACE_REVISION)];

    rv = beginTransaction();
    CHECK_ERR(rv, rc, end);

    if (!stmt_ins) {
        rv = sqlite3_prepare_v2(db, GETSQL(INSERT_OR_REPLACE_REVISION), -1, &stmt_ins, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // :compid, :rev, :size, :mtime, :update_dev, :hash, :cksum
    // bind :compid
    rv = sqlite3_bind_int64(stmt_ins, 1, tp->compid);
    CHECK_BIND(rv, rc, db, end);
    // bind :rev, rev will be updated in getLatestRevNum() based on VCS response
    rv = sqlite3_bind_int64(stmt_ins, 2, tp->baserev);
    CHECK_BIND(rv, rc, db, end);
    // bind :size
    rv = sqlite3_bind_int64(stmt_ins, 3, tp->size);
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    rv = sqlite3_bind_int64(stmt_ins, 4, tp->modifyTime);
    CHECK_BIND(rv, rc, db, end);
    // bind :update_dev
    rv = sqlite3_bind_int64(stmt_ins, 5, tp->update_device);
    CHECK_BIND(rv, rc, db, end);
    // bind :hash
    rv = sqlite3_bind_text(stmt_ins, 6, tp->hashval.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :cksum
    ss.str("");
    ss << tp->compid << "," << tp->baserev << "," << tp->size << "," << tp->modifyTime << "," << tp->update_device << "," << tp->hashval.c_str() << '\0';
    rowChecksum = calculateRowChecksum(ss.str());

    rv = sqlite3_bind_text(stmt_ins, 7, rowChecksum.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_ins);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    CHECK_ERR(rv, rc, end);

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt_ins) {
        sqlite3_reset(stmt_ins);
    }

    return rc;
}

int DocSNGQueue::deleteComponentFromDB(DocSNGTicket *tp)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_COMPONENT)];

    rv = beginTransaction();
    CHECK_ERR(rv, rc, end);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_COMPONENT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt, 1, tp->compid);
    CHECK_BIND(rv, rc, db, end);
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    CHECK_ERR(rv, rc, end);

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt) {
        sqlite3_reset(stmt);
    }

    return rc;
}

int DocSNGQueue::deleteRevisionFromDB(DocSNGTicket *tp)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_REVISION)];

    rv = beginTransaction();
    CHECK_ERR(rv, rc, end);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_REVISION), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :compid
    rv = sqlite3_bind_int64(stmt, 1, tp->compid);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    rv = commitTransaction();
    CHECK_ERR(rv, rc, end);

end:
    if (rc) {
        rollbackTransaction();
    }

    if (stmt) {
        sqlite3_reset(stmt);
    }

    return rc;
}

int DocSNGQueue::getCompid_by_name(DocSNGTicket *tp)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_COMPID_BY_NAME)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_COMPID_BY_NAME), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :name
    rv = sqlite3_bind_text(stmt, 1, tp->docname.c_str(), -1, NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_ROW) {
        tp->compid = (u64)sqlite3_column_int64(stmt, 0);
    }
    else {  // SQLITE_DONE, meaning not found
        rc = CCD_ERROR_NOT_FOUND;  // to indicate version missing; note that this is a valid outcome
    }

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}

int DocSNGQueue::getHash_by_compid(u64 compid, std::string &hashval)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_HASH_BY_COMPID)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_HASH_BY_COMPID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :compid
    rv = sqlite3_bind_int64(stmt, 1, compid);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_ROW) {
        hashval = (const char*)sqlite3_column_text(stmt, 0);
    }
    else {  // SQLITE_DONE, meaning not found
        rc = CCD_ERROR_NOT_FOUND;  // to indicate version missing; note that this is a valid outcome
    }

end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

DocSNGTicket *DocSNGQueue::activateNextTicket()
{
    DocSNGTicket *tp = NULL;

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        int rv = openDB();
        if (rv != 0) {
            return tp;
        }
    }

    if (isRunning && !activeTicket) {
        int rc = 0;
        int rv;
        sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_OLDEST_REQUEST)];
        if (!stmt) {
            rv = sqlite3_prepare_v2(db, GETSQL(SELECT_OLDEST_REQUEST), -1, &stmt, NULL);
            CHECK_PREPARE(rv, rc, db, dbcleanup);
        }

        // bind :uid
        rv = sqlite3_bind_int64(stmt, 1, currentUserId);
        CHECK_BIND(rv, rc, db, dbcleanup);

        rv = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, db, dbcleanup);

        if (rv == SQLITE_ROW) {
            tp = new DocSNGTicket(this);
            tp->ticket_id = sqlite3_column_int64(stmt, 0);  // id
            tp->uid = sqlite3_column_int64(stmt, 1);  // uid
            tp->did = sqlite3_column_int64(stmt, 2); // did
            if (sqlite3_column_type(stmt, 9) == SQLITE_TEXT) {  // dloc
                tp->dloc.assign((const char*)sqlite3_column_text(stmt, 9));
            }
            tp->operation = (DocSNGChangeType)sqlite3_column_int(stmt, 3);  // type
            if (sqlite3_column_type(stmt, 7) == SQLITE_TEXT) {  // fpath
                extendPathToFullPath((const char*)sqlite3_column_text(stmt, 7), rootdir, tp->srcPath);
            }
            if (sqlite3_column_type(stmt, 8) == SQLITE_TEXT) {  // tpath
                extendPathToFullPath((const char*)sqlite3_column_text(stmt, 8), rootdir, tp->thumbPath);
            }
            if (sqlite3_column_type(stmt, 12) == SQLITE_TEXT) {  // docname
                tp->docname.assign((const char*)sqlite3_column_text(stmt, 12));
            }
            if (sqlite3_column_type(stmt, 4) == SQLITE_TEXT) {  // name
                tp->origName.assign((const char*)sqlite3_column_text(stmt, 4));
            }
            if (sqlite3_column_type(stmt, 5) == SQLITE_TEXT) {  // newname
                tp->newName.assign((const char*)sqlite3_column_text(stmt, 5));
            }
            if (sqlite3_column_type(stmt, 6) == SQLITE_INTEGER) {  // mtime
                tp->modifyTime = sqlite3_column_int64(stmt, 6);
            }
            else {
                tp->modifyTime = 0;
            }
            if (sqlite3_column_type(stmt, 13) == SQLITE_TEXT) {  // conttype
                tp->contentType.assign((const char*)sqlite3_column_text(stmt, 13));
            }
            if (sqlite3_column_type(stmt, 10) == SQLITE_INTEGER) {  // compid
                tp->compid = sqlite3_column_int64(stmt, 10);
            }
            else {
                tp->compid = 0;
            }
            if (sqlite3_column_type(stmt, 11) == SQLITE_INTEGER) {  // baserev
                tp->baserev = sqlite3_column_int64(stmt, 11);
                tp->baserev_valid = true;
            }
            else {
                tp->baserev = 0;
                tp->baserev_valid = false;
            }
            activeTicket = tp;
        }

    dbcleanup:
        if (stmt) {
            sqlite3_reset(stmt);
        }
    }

    if (!activeTicket && !tp) {  // idle - close DB
        closeDB();

        if (isActive) {
            // Notify EventManagerPb that queue processing has gone to sleep.
            (*engineStateChangeCb)(false);
        }
        isActive = false;
    }

    if (tp) {
#ifdef USE_QUEUE_OPTIMIZATION
        bool is_available = isAvailable(tp);

        if (!is_available) {
            dequeue(tp);
            // FIXME: can we afford recursion here?
            activateNextTicket();
        } else
#endif
        {
            LOG_INFO("Placed ID "FMTu64" in work queue", tp->ticket_id);
            tp->inProgress = true;

            // 2.4.0 later always assumes v1.1 CloudDoc dataset in VCS
            VPLThread_attr_t attr;
            VPLThread_AttrInit(&attr);
            VPLThread_AttrSetDetachState(&attr, VPL_TRUE);
            VPLThread_AttrSetStackSize(&attr, CLOUDDOC_MGR_STACK_SIZE);
            VPLDetachableThread_Create(NULL, vcs_handler, (void*)tp, &attr, NULL);
            VPLThread_AttrDestroy(&attr);
        }
        if (!isActive) {
            // Notify EventManagerPb that queue processing has resumed.
            (*engineStateChangeCb)(true);
        }
        isActive = true;
    }

    return tp;
}

int DocSNGQueue::visitTickets(int (*visitor)(void *ctx,
                                             int change_type,
                                             int in_progress,
                                             const char *name,
                                             const char *newname,
                                             u64 modify_time,
                                             u64 upload_size,
                                             u64 upload_progress),
                              void *ctx)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rc = 0;
    int rv;
    bool queue_is_empty = true;  // assume empty and prove otherwise

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_REQUESTS_IN_ORDER)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_REQUESTS_IN_ORDER), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :uid
    rv = sqlite3_bind_int64(stmt, 1, currentUserId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        u64 id = sqlite3_column_int64(stmt, 0);
        bool isActiveTicket = activeTicket && id == activeTicket->ticket_id;
        int type = sqlite3_column_int(stmt, 1);
        queue_is_empty = false;
        rv = (*visitor)(ctx,
                        type,
                        isActiveTicket ? 1 : 0, 
                        (const char*)sqlite3_column_text(stmt, 2), // name
                        sqlite3_column_type(stmt, 3) == SQLITE_TEXT ? (const char*)sqlite3_column_text(stmt, 3) : NULL, // newname
                        sqlite3_column_type(stmt, 4) == SQLITE_INTEGER ? sqlite3_column_int64(stmt, 4) : 0, // mtime
                        isActiveTicket && type == DOC_SNG_UPLOAD
                        ? activeTicket->uploadFileSize + activeTicket->uploadThumbSize : 0,
                        isActiveTicket && type == DOC_SNG_UPLOAD
                        ? activeTicket->uploadFileProgress + activeTicket->uploadThumbProgress : 0);
        if (rv) {
            goto dbcleanup;
        }
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    if (queue_is_empty) {
        closeDB();
    }

    return rc;
}

bool DocSNGQueue::isAvailable(DocSNGTicket* ticket)
{
    bool is_available = true;

    // see bug 4658 for background
#ifdef USE_QUEUE_OPTIMIZATION
    int rc;
    int rv;
    DocSNGTicket* tp;
    std::vector<DocSNGTicket*> unchecked_tickets;

    VPLMutex_Lock(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        openDB();
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_REQUESTS_IN_ORDER_2)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_REQUESTS_IN_ORDER_2), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :uid
    rv = sqlite3_bind_int64(stmt, 1, currentUserId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        tp = new DocSNGTicket(this);
        tp->ticket_id = sqlite3_column_int64(stmt, 0);  // id
        tp->uid = sqlite3_column_int64(stmt, 1);  // uid
        tp->did = sqlite3_column_int64(stmt, 2); // did
        tp->operation = (DocSNGChangeType)sqlite3_column_int(stmt, 3);  // type
        if (sqlite3_column_type(stmt, 7) == SQLITE_TEXT) {  // fpath
            extendPathToFullPath((const char*)sqlite3_column_text(stmt, 7), rootdir, tp->srcPath);
        }
        if (sqlite3_column_type(stmt, 8) == SQLITE_TEXT) {  // tpath
            extendPathToFullPath((const char*)sqlite3_column_text(stmt, 8), rootdir, tp->thumbPath);
        }
        tp->origName.assign((const char*)sqlite3_column_text(stmt, 4));  // name
        if (sqlite3_column_type(stmt, 5) == SQLITE_TEXT) {  // newname
            tp->newName.assign((const char*)sqlite3_column_text(stmt, 5));
        }
        if (sqlite3_column_type(stmt, 6) == SQLITE_INTEGER) {  // mtime
            tp->modifyTime = sqlite3_column_int64(stmt, 6);
        }
        else {
            tp->modifyTime = 0;
        }

        unchecked_tickets.push_back(tp);
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

    if (unchecked_tickets.size()<=1) {
        goto dbcleanup;
    }

    /*
    ##### Pseudo Code #####
    {
        Input: ticket

        var checking_item = ticket;
        var filepath = checking_item.filepath;
        Map rename_mapping_table;
        While (has unchecked items) {
            var next_item = pop the first unchecked item

            if (next_item.type == UPLOAD|DELETE) {
                if((next_item.filepath in rename_mapping_table && rename_mapping_table(next_item.filepath) == filepath) ||
                    (next_item.filepath not in rename_mapping_table && next_item.filepath == filepath)) {
                    //The same file changes.
                    if (checking_item.type == UPLOAD && next_item.type == UPLOAD) {
                        is_available = false;
                        goto dbcleanup;

                    } else if (checking_item.type == DELETE && next_item.type == UPLOAD) {
                        is_available = false;
                        goto dbcleanup;
                    }
                }
            } else if (next_item.type == RENAME) {
                //Update rename_mapping_table
                if(next_item.filepath in rename_mapping_table){
                    var original_path = rename_mapping_table[next_item.filepath];
                    rename_mapping_table[next_item.newpath] = original_path;
                    rename_mapping_table.remove(next_item.filepath);
                }else{
                    rename_mapping_table[next_item.newpath] = next_item.filepath;
                }
            }
        }
    }
    */
    {
        DocSNGTicket* checking_item_ticket = NULL;
        DocSNGTicket* next_item;
        std::string file_path;
        std::map<std::string, std::string> rename_mapping_table;

        //Start from the ticket
        do {
            if (checking_item_ticket!=NULL) {
                delete checking_item_ticket;
            }
            checking_item_ticket = unchecked_tickets.front();
            unchecked_tickets.erase(unchecked_tickets.begin());

        } while(checking_item_ticket->ticket_id != ticket->ticket_id && unchecked_tickets.size() > 0);

        if (checking_item_ticket->ticket_id != ticket->ticket_id) {
            //Should not happened.
            LOG_WARN("Can not find ticket-"FMTu64" in the queue.", ticket->ticket_id);
            delete checking_item_ticket;
            goto dbcleanup;
        }

        if (checking_item_ticket->operation == DOC_SNG_RENAME) {
            delete checking_item_ticket;
            goto dbcleanup;
        }

        file_path = checking_item_ticket->origName;

        while (unchecked_tickets.size() > 0) {
            next_item = unchecked_tickets.front();
            unchecked_tickets.erase(unchecked_tickets.begin());

            if ((next_item->operation == DOC_SNG_UPLOAD || next_item->operation == DOC_SNG_DELETE)) {
                if ((rename_mapping_table.count(next_item->origName)>0 && rename_mapping_table[next_item->origName] == file_path) ||
                    (rename_mapping_table.count(next_item->origName)<=0 && next_item->origName == file_path)) {

                    //UPLOAD(a') >> UPLOAD(a") ==> UPLOAD(a")
                    if (checking_item_ticket->operation == DOC_SNG_UPLOAD && next_item->operation == DOC_SNG_UPLOAD) {
                        is_available = false;
                        goto dbcleanup;

                    //UPLOAD >> DELETE ==> DELETE
                    } else if (checking_item_ticket->operation == DOC_SNG_DELETE && next_item->operation == DOC_SNG_UPLOAD) {
                        is_available = false;
                        goto dbcleanup;
                    }
                }

            } else if (next_item->operation == DOC_SNG_RENAME) {
                if (rename_mapping_table.count(next_item->origName)>0) {
                    std::string original_path = rename_mapping_table[next_item->origName];
                    rename_mapping_table.erase(next_item->origName);
                    rename_mapping_table[next_item->newName] = original_path;

                } else {
                    rename_mapping_table[next_item->newName] = next_item->origName;
                }
            }
            delete next_item;
        }
        delete checking_item_ticket;
    }

 dbcleanup:
    while (unchecked_tickets.size() > 0) {
        DocSNGTicket* next_item;
        next_item = unchecked_tickets.back();
        unchecked_tickets.pop_back();
        delete next_item;
    }

    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);
#endif // USE_QUEUE_OPTIMIZATION

    return is_available;
}

int DocSNGQueue::checkTicketsAfterEnqueue()
{
    int rc = 0;

    // see bug 4658 for background
#ifdef USE_QUEUE_OPTIMIZATION
    int rv;
    DocSNGTicket* tp;
    std::vector<DocSNGTicket*> unchecked_tickets;

    VPLMutex_Lock(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        openDB();
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_REQUESTS_IN_ORDER_2)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_REQUESTS_IN_ORDER_2), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :uid
    rv = sqlite3_bind_int64(stmt, 1, currentUserId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        tp = new DocSNGTicket(this);
        tp->ticket_id = sqlite3_column_int64(stmt, 0);  // id
        tp->uid = sqlite3_column_int64(stmt, 1);  // uid
        tp->did = sqlite3_column_int64(stmt, 2); // did
        tp->operation = (DocSNGChangeType)sqlite3_column_int(stmt, 3);  // type
        if (sqlite3_column_type(stmt, 7) == SQLITE_TEXT) {  // fpath
            extendPathToFullPath((const char*)sqlite3_column_text(stmt, 7), rootdir, tp->srcPath);
        }
        if (sqlite3_column_type(stmt, 8) == SQLITE_TEXT) {  // tpath
            extendPathToFullPath((const char*)sqlite3_column_text(stmt, 8), rootdir, tp->thumbPath);
        }
        tp->origName.assign((const char*)sqlite3_column_text(stmt, 4));  // name
        if (sqlite3_column_type(stmt, 5) == SQLITE_TEXT) {  // newname
            tp->newName.assign((const char*)sqlite3_column_text(stmt, 5));
        }
        if (sqlite3_column_type(stmt, 6) == SQLITE_INTEGER) {  // mtime
            tp->modifyTime = sqlite3_column_int64(stmt, 6);
        }
        else {
            tp->modifyTime = 0;
        }

        unchecked_tickets.push_back(tp);
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

    if (unchecked_tickets.size()<1) {
        goto dbcleanup;
    }

    /*
    ##### Pseudo Code #####
    {
        Input: enqueued_ticket

        var checking_item = enqueued_ticket;
        var filepath = checking_item.filepath;
        Map rename_mapping_table;
        While (has unchecked items) {
            var next_item = pop the last unchecked item

            if (next_item.type == UPLOAD|DELETE) {
                if((next_item.filepath in rename_mapping_table && rename_mapping_table(next_item.filepath) == filepath) ||
                    (next_item.filepath not in rename_mapping_table && next_item.filepath == filepath)) {
                    //The same file changes.
                    if (checking_item.type == UPLOAD && next_item.type == UPLOAD) {
                        if (next_item is processing) {
                            Set activeTicket is out-of-date (As the canceled flag be checked, the ticket will be dequeued)
                        } else {
                            IGNORE
                        }

                    } else if (checking_item.type == DELETE && next_item.type == UPLOAD) {
                        if (next_item is processing) {
                            Set activeTicket is out-of-date (As the canceled flag be checked, the ticket will be dequeued)
                        } else {
                            IGNORE
                        }
                    }
                }
            } else if (next_item.type == RENAME) {
                //Update rename_mapping_table
                if(next_item.newpath in rename_mapping_table){
                    var final_path = rename_mapping_table[next_item.newpath];
                    rename_mapping_table[next_item.filepath] = final_path;
                    rename_mapping_table.remove(next_item.newpath);
                }else{
                    rename_mapping_table[next_item.filepath] = next_item.newpath;
                }
            }
        }
    }
    */
    {
        DocSNGTicket* checking_item_ticket;
        DocSNGTicket* next_item;
        std::string file_path;
        std::map<std::string, std::string> rename_mapping_table;

        checking_item_ticket = unchecked_tickets.back();
        unchecked_tickets.pop_back();

        if (checking_item_ticket->operation == DOC_SNG_RENAME) {
            delete checking_item_ticket;
            goto dbcleanup;
        }

        file_path = checking_item_ticket->origName;

        while (unchecked_tickets.size() > 0) {
            next_item = unchecked_tickets.back();
            unchecked_tickets.pop_back();

            if ((next_item->operation == DOC_SNG_UPLOAD || next_item->operation == DOC_SNG_DELETE)) {
                if ((rename_mapping_table.count(next_item->origName)>0 && rename_mapping_table[next_item->origName] == file_path) ||
                    (rename_mapping_table.count(next_item->origName)<=0 && next_item->origName == file_path)) {

                    //Check the next item and Merge if needs
                    //UPLOAD(a') >> UPLOAD(a") ==> UPLOAD(a")
                    if (checking_item_ticket->operation == DOC_SNG_UPLOAD && next_item->operation == DOC_SNG_UPLOAD) {
                        if (isRunning && activeTicket && (activeTicket->ticket_id == next_item->ticket_id)) {
                            LOG_INFO("Set out-of-date:%s", next_item->origName.c_str());
                            activeTicket->set_out_of_date(true);
                        }

                    //UPLOAD >> DELETE ==> DELETE
                    } else if (checking_item_ticket->operation == DOC_SNG_DELETE && next_item->operation == DOC_SNG_UPLOAD) {
                        if (isRunning && activeTicket && activeTicket->ticket_id == next_item->ticket_id) {
                            LOG_INFO("Set out-of-date:%s", next_item->origName.c_str());
                            activeTicket->set_out_of_date(true);
                        }
                    }
                }

            } else if (next_item->operation == DOC_SNG_RENAME) {
                if (rename_mapping_table.count(next_item->newName)>0) {
                    std::string final_path = rename_mapping_table[next_item->newName];
                    rename_mapping_table.erase(next_item->newName);
                    rename_mapping_table[next_item->origName] = final_path;

                } else {
                    rename_mapping_table[next_item->origName] = next_item->newName;
                }
            }
            delete next_item;
        }
        delete checking_item_ticket;
    }

 dbcleanup:
    while (unchecked_tickets.size() > 0) {
        DocSNGTicket* next_item;
        next_item = unchecked_tickets.back();
        unchecked_tickets.pop_back();
        delete next_item;
    }

    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);
#endif // USE_QUEUE_OPTIMIZATION

    return rc;
}

int DocSNGQueue::getRequestListInJson(char **json_out)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    u64 deviceId = 0;
    if (ESCore_GetDeviceGuid(&deviceId) != 0)
        return CCD_ERROR_NOT_INIT;

    if (json_out == NULL)
        return CCD_ERROR_PARAMETER;

    int rc = 0;
    int rv;
    bool queue_is_empty = true;  // assume empty and prove otherwise
    int count = 0;
    std::ostringstream oss;

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_ALL_REQUESTS_IN_ORDER)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_ALL_REQUESTS_IN_ORDER), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    oss << "{\"requestList\":[";

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        queue_is_empty = false;
        count++;

        if (count > 1) {
            oss << ",";
        }

        u64 id = sqlite3_column_int64(stmt, 0);
        oss << "{\"id\":" << id;

        if (activeTicket && id == activeTicket->ticket_id) {
            oss << ",\"active\":true";
        }
        else {
            oss << ",\"active\":false";
        }

        u64 uid = sqlite3_column_int64(stmt, 1);
        u64 did = sqlite3_column_int64(stmt, 2);
        oss << ",\"userId\":" << uid
            << ",\"datasetId\":" << did;

        int type = sqlite3_column_int(stmt, 3);
        switch (type) {
        case DOC_SNG_UPLOAD:
            oss << ",\"type\":\"upload\"";
            break;
        case DOC_SNG_RENAME:
            oss << ",\"type\":\"rename\"";
            break;
        case DOC_SNG_DELETE:
            oss << ",\"type\":\"delete\"";
            break;
        default:
            oss << ",\"type\":\"unknown\"";
        }

        oss << ",\"name\":\"";
        if (sqlite3_column_type(stmt, 11) == SQLITE_TEXT) {
            oss << sqlite3_column_text(stmt, 11) << "\"";
        }
        else {
            oss << std::hex << std::uppercase << deviceId << "/files/" << sqlite3_column_text(stmt, 4) << "\"";
        }
        if (sqlite3_column_type(stmt, 5) == SQLITE_TEXT) {
            oss << ",\"newName\":\"" << sqlite3_column_text(stmt, 5) << "\"";
        }
        oss << ",\"lastChanged\":" << sqlite3_column_int64(stmt, 6);
#if 0
        // internal so don't display them
        if (sqlite3_column_type(stmt, 7) == SQLITE_TEXT) {
            oss << ",\"fpath\":" << sqlite3_column_text(stmt, 7);
        }
        if (sqlite3_column_type(stmt, 8) == SQLITE_TEXT) {
            oss << ",\"tpath\":" << sqlite3_column_text(stmt, 8);
        }
#endif
        if (sqlite3_column_type(stmt, 9) == SQLITE_INTEGER) {
            oss << ",\"compId\":" << sqlite3_column_int64(stmt, 9);
        }
        if (sqlite3_column_type(stmt, 10) == SQLITE_INTEGER) {
            oss << ",\"baseRev\":" << sqlite3_column_int64(stmt, 10);
        }
        oss << "}";
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

    oss << "],\"numOfRequests\":" << count << "}";

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    if (queue_is_empty) {
        closeDB();
    }

    if (rc == 0) {
        std::string json = oss.str();
        *json_out = (char*)malloc(json.size() + 1);
        if (*json_out == NULL) {
            rc = CCD_ERROR_NOMEM;
        }
        else {
            memcpy(*json_out, json.data(), json.size());
            (*json_out)[json.size()] = '\0';
        }
    }

    return rc;
}

int DocSNGQueue::getRequestStatusInJson(u64 requestId, char **json_out)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;  // return 500

    if (json_out == NULL)
        return CCD_ERROR_PARAMETER;  // return 500

    int rc = 0;  // return 200
    int rv;
    std::ostringstream oss;
    bool output_json = false;

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_REQUEST_BY_ID)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_REQUEST_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt, 1, requestId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    if ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        oss << "{\"id\":" << requestId;
        if (activeTicket && requestId == activeTicket->ticket_id) {
            oss << ",\"status\":\"active\"";
            oss << ",\"totalSize\":" << activeTicket->uploadFileSize + activeTicket->uploadThumbSize;
            oss << ",\"xferedSize\":" << activeTicket->uploadFileProgress + activeTicket->uploadThumbProgress;
        }
        else {
            oss << ",\"status\":\"wait\"";
            oss << ",\"totalSize\":" << 0;
            oss << ",\"xferedSize\":" << 0;
        }
        oss << "}";
        output_json = true;
    }
    else if (rv == SQLITE_DONE) {
        oss << "{\"errMsg\":\"requestId not found\"}";  // return 404
        rc = CCD_ERROR_NOT_FOUND;
        output_json = true;
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    if (output_json) {
        std::string json = oss.str();
        *json_out = (char*)malloc(json.size() + 1);
        if (*json_out == NULL) {
            rc = CCD_ERROR_NOMEM;  // return 500
        }
        else {
            memcpy(*json_out, json.data(), json.size());
            (*json_out)[json.size()] = '\0';
        }
    }

    return rc;  // any non-0 rc, return 500
}

int DocSNGQueue::cancelRequest(u64 requestId)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;  // return 500

    int rc = 0;  // return 200
    int rv;
    int type = 0;
    u64 mtime = 0;
    const char *name = NULL, *newname = NULL, *fpath = NULL, *tpath = NULL;

    MutexAutoLock lock1(&m_mutex);
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&db_mutex));
#endif //CCD_ENABLE_SYNCDOWN_CLOUDDOC

    if (!db) {
        rv = openDB();
        if(rv != 0) {
            return rv;
        }
    }

    sqlite3_stmt *&stmt_sel = dbstmts[SQLNUM(SELECT_REQUEST_BY_ID)];
    sqlite3_stmt *&stmt_del = dbstmts[SQLNUM(DELETE_REQUEST)];

    if (activeTicket && requestId == activeTicket->ticket_id) {

        MutexAutoLock lock(&(activeTicket->http_handle_mutex));
        if(activeTicket->http_handle && activeTicket->http_handle->Cancel() == VPL_OK){
            LOG_INFO("HTTP cancelled!");
            activeTicket->http_handle = NULL;
            rc = 0;
            goto dbcleanup;
        }else{
            rc = -1;   
            goto dbcleanup;
        }
    }

    if (!stmt_sel) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_REQUEST_BY_ID), -1, &stmt_sel, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt_sel, 1, requestId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    // SELECT id, uid, did, type, name, newname, mtime, fpath, tpath, compid, baserev, docname

    if ((rv = sqlite3_step(stmt_sel)) == SQLITE_ROW) {
        type = sqlite3_column_int(stmt_sel, 3);
        name = (const char*)sqlite3_column_text(stmt_sel, 4);
        if (sqlite3_column_type(stmt_sel, 5) == SQLITE_TEXT) {
            newname = (const char*)sqlite3_column_text(stmt_sel, 5);
        }
        if (sqlite3_column_type(stmt_sel, 6) == SQLITE_INTEGER) {
            mtime = sqlite3_column_int64(stmt_sel, 6);
        }
        if (sqlite3_column_type(stmt_sel, 7) == SQLITE_TEXT) {
            fpath = (const char*)sqlite3_column_text(stmt_sel, 7);
        }
        if (sqlite3_column_type(stmt_sel, 8) == SQLITE_TEXT) {
            tpath = (const char*)sqlite3_column_text(stmt_sel, 8);
        }
    }
    else if (rv == SQLITE_DONE) {
        // not found
        // TODO return 404
        LOG_WARN("Request not found: "FMTu64, requestId);
        rc = CCD_ERROR_NOT_FOUND;
        goto dbcleanup;
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

    if (!stmt_del) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_REQUEST), -1, &stmt_del, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt_del, 1, requestId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    rv = sqlite3_step(stmt_del);
    CHECK_STEP(rv, rc, db, dbcleanup);

    if (sqlite3_changes(db) == 0) {
        // no rows deleted; this is unexpected.  emit warning
        LOG_WARN("Request ID "FMTu64" not found in queue", requestId);
        rc = -1;
    }
    else {
        LOG_INFO("Canceled request ID "FMTu64, requestId);
    }

 dbcleanup:
    // Remove any temporary files created for this request.
    if (fpath) {
        std::string ffullpath;
        std::string fpathstring(fpath);
        LOG_INFO("Delete tmp file %s/%s", rootdir.c_str(), fpathstring.c_str());
        extendPathToFullPath(fpathstring, rootdir, ffullpath);
        VPLFile_Delete(ffullpath.c_str());
    }
    if (tpath) {
        std::string tfullpath;
        std::string tpathstring(tpath);
        LOG_INFO("Delete tmp file %s/%s", rootdir.c_str(), tpathstring.c_str());
        extendPathToFullPath(tpathstring, rootdir, tfullpath);
        VPLFile_Delete(tfullpath.c_str());
    }

#define RESULT_CANCELLED 1

    //
    // Notify EventManagerPb of cancelation.
    //
    // Need to do this before the reset statements, but we still hold the mutex here.
    // This should not cause deadlock, since the callback handler only grabs its own mutex
    //
    if (rc == 0) {
        (*completionCb)(type, name, newname, mtime, RESULT_CANCELLED, "", 0, 0);
    }

    // Don't reset the SQL statement until after all references to the data
    if (stmt_sel) {
        sqlite3_reset(stmt_sel);
    }
    if (stmt_del) {
        sqlite3_reset(stmt_del);
    }

    return rc;  // any non-0 rc, return 500
}

// --- External Entrypoints to access DocSNGQueue ---

int DocSNGQueue_NewRequest(u64 uid,
                           u64 did,
                           const char *dloc,
                           int change_type,
                           const char *docname,
                           const char *name,
                           const char *newname,
                           u64 modtime,
                           const char *contentType,
                           u64 compid, u64 baserev,
                           const char *thumbpath,
                           const char *thumbdata, u32 thumbsize,
                           u64 *requestid)
{
    int rv = docSNGQueue.enqueue(uid, did, dloc,
                                 change_type,
                                 docname, name, newname, modtime, contentType,
                                 compid, baserev,
                                 thumbpath,
                                 thumbdata, thumbsize,
                                 requestid);
    if (rv) {
        return rv;
    }

    return docSNGQueue.checkTicketsAfterEnqueue();
}

int DocSNGQueue_VisitTickets(int (*visitor)(void *ctx,
                                            int change_type,
                                            int in_progress,
                                            const char *name,
                                            const char *newname,
                                            u64 modify_time,
                                            u64 upload_size,
                                            u64 upload_progress),
                              void *ctx)
{
    return docSNGQueue.visitTickets(visitor, ctx);
}

int DocSNGQueue_EngineIsRunning()
{
    return docSNGQueue.isProcessing();
}

int DocSNGQueue_Enable(u64 userId)
{
    return docSNGQueue.startProcessing(userId);
}

int DocSNGQueue_Disable(int purge)
{
    return docSNGQueue.stopProcessing(purge != 0);
}

int DocSNGQueue_Init(const char *rootdir,
                     int (*completionCb)(int change_type,
                                         const char *name,
                                         const char *newname,
                                         u64 modify_time,
                                         int result,
                                         const char *docname,
                                         u64 comp_id,
                                         u64 revision),
                     int (*engineStateChangeCb)(bool engine_started))
{
    return docSNGQueue.init(rootdir, completionCb, engineStateChangeCb);
}

int DocSNGQueue_Release()
{
    return docSNGQueue.release();
}

int DocSNGQueue_SetVcsHandlerFunction(VPLDetachableThread_fn_t handler)
{
    return docSNGQueue.setVcsHandlerFunction(handler);
}

int DocSNGQueue_GetRequestListInJson(char **json_out)
{
    return docSNGQueue.getRequestListInJson(json_out);
}

int DocSNGQueue_GetRequestStatusInJson(u64 requestId, char **json_out)
{
    return docSNGQueue.getRequestStatusInJson(requestId, json_out);
}

int DocSNGQueue_CancelRequest(u64 requestId)
{
    return docSNGQueue.cancelRequest(requestId);
}
