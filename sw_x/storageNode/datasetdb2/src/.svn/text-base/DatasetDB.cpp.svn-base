#include "DatasetDB.hpp"

#include "vpl_conv.h"
#include "vplex_trace.h"
#include "vplex_file.h"
#include "utf8.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

// Target DB schema version.
// Anything less will be upgraded to this version by OpenDB().
#define TARGET_SCHEMA_VERSION 1

#define BACKUP_DELAY        VPLTIME_FROM_SEC(60)    // 1 minute delay

// for stringification
#define XSTR(s) STR(s)
#define STR(s) #s

#define LOG_ERROR(fmt, ...) VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%s: "fmt, dbpath.c_str(), __VA_ARGS__)
#define LOG_WARN(fmt, ...) VPLTRACE_LOG_WARN(TRACE_BVS, 0, "%s: "fmt, dbpath.c_str(), __VA_ARGS__)
#define LOG_INFO(fmt, ...) VPLTRACE_LOG_INFO(TRACE_BVS, 0, "%s: "fmt, dbpath.c_str(), __VA_ARGS__)

#define CHECK_PREPARE(rv, rc, db, lbl)                                  \
if (isSqliteError(rv)) {                                                \
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to prepare SQL stmt: %d (%d), %s", rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db)); \
    goto lbl;                                                           \
 }

#define CHECK_BIND(rv, rc, db, lbl)                                     \
if (isSqliteError(rv)) {                                                \
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to bind value in prepared stmt: %d (%d), %s", rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db)); \
    goto lbl;                                                           \
}

#define CHECK_STEP(rv, rc, db, lbl)                                     \
if (isSqliteError(rv)) {                                                \
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to execute prepared stmt: %d (%d), %s", rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db)); \
    goto lbl;                                                           \
}

#define CHECK_FINALIZE(rv, rc, db, lbl)                                 \
if (isSqliteError(rv)) {                                                \
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to finalize prepared stmt: %d (%d), %s", rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db)); \
    goto lbl;                                                           \
}

#define CHECK_RV(rv, rc, lbl)                   \
if (rv != DATASETDB_OK) {                       \
    rc = rv;                                    \
    goto lbl;                                   \
}

#define MAX_BACKUP_COPY             1

#define VERID_DATASET_VERSION 1
#define VERID_SCHEMA_VERSION 2
#define VERID_NEXT_CONTPATH_KEY 3

static bool isSqliteError(int ec)
{
    return ec != SQLITE_OK && ec != SQLITE_ROW && ec != SQLITE_DONE;
}

static DatasetDBError mapSqliteErrCode(int ec)
{
    return (DatasetDBError)(SQLITE_ERROR_BASE - ec);
}

static const char test_and_create_db_sql[] = 
"BEGIN;"
"CREATE TABLE IF NOT EXISTS version ("
    "id INTEGER PRIMARY KEY, "
    "value INTEGER NOT NULL);"
"INSERT OR IGNORE INTO version VALUES ("XSTR(VERID_DATASET_VERSION)", 0);"
"INSERT OR IGNORE INTO version VALUES ("XSTR(VERID_SCHEMA_VERSION)", "XSTR(TARGET_SCHEMA_VERSION)");"
"INSERT OR IGNORE INTO version VALUES ("XSTR(VERID_NEXT_CONTPATH_KEY)", 1);"
"CREATE TABLE IF NOT EXISTS component ("
    "id INTEGER PRIMARY KEY, "          // index created automatically (because INTEGER PRIMARY KEY)
    "name TEXT NOT NULL, "
    "upname TEXT NOT NULL, "
    "parent INTEGER NOT NULL, "
    "type INTEGER NOT NULL, "
    "perm INTEGER NOT NULL, "
    "ctime INTEGER NOT NULL, "
    "mtime INTEGER NOT NULL, "
    "version INTEGER NOT NULL, "
    "origin_dev INTEGER, "          // for VCS-like behavior
    "cur_rev INTEGER NOT NULL);"        // for VCS-like behavior; 0 means not
"CREATE INDEX IF NOT EXISTS i_comp_parent_upname ON component (parent,upname);"
"CREATE TABLE IF NOT EXISTS content ("
    "compid INTEGER NOT NULL, "
    "location TEXT NOT NULL UNIQUE, "       // index created automatically (because UNIQUE)
    "size INTEGER NOT NULL, "
    "mtime INTEGER NOT NULL, "
    "baserev INTEGER, "             // for VCS-like behavior
    "update_dev INTEGER, "          // for VCS-like behavior
    "rev INTEGER NOT NULL, "            // for VCS-like behavior; 0 means not
    "version INTEGER NOT NULL, "
    "UNIQUE (compid, rev), "
    "FOREIGN KEY (compid) REFERENCES component (id));"
"CREATE INDEX IF NOT EXISTS i_content_compid ON content (compid);"
"COMMIT;";

#define COMPONENT_COLS "id, name, upname, parent, type, perm, ctime, mtime, version, origin_dev, cur_rev"

#define DEFSQL(tag, def) static const char sql_ ## tag [] = def
#define SQLNUM(tag) DATASETDB_STMT_ ## tag
#define GETSQL(tag) sql_ ## tag

enum {
    DATASETDB_STMT_BEGIN,
    DATASETDB_STMT_COMMIT,
    DATASETDB_STMT_ROLLBACK,
    DATASETDB_STMT_GET_VERSION,
    DATASETDB_STMT_SET_VERSION,
    DATASETDB_STMT_ADD_COMPONENT_BY_NAME_PARENT,
    DATASETDB_STMT_ADD_CONTENT_BY_COMPID_REV,
    DATASETDB_STMT_GET_COMPONENT_BY_UPNAME_PARENT,
    DATASETDB_STMT_GET_CONTENT_BY_LOCATION,
    DATASETDB_STMT_GET_COMPONENT_BY_ID,
    DATASETDB_STMT_GET_CONTENT_BY_COMPID_REV,
    DATASETDB_STMT_GET_FILE_COMPONENT_ID_IN_RANGE,
    DATASETDB_STMT_GET_NEXT_CONTENT,
    DATASETDB_STMT_LIST_COMPONENTS_BY_PARENT,
    DATASETDB_STMT_LIST_COMPONENTS_INFO_BY_PARENT,
    DATASETDB_STMT_SET_COMPONENT_PERM_BY_ID,
    DATASETDB_STMT_SET_COMPONENT_CTIME_BY_ID,
    DATASETDB_STMT_SET_COMPONENT_MTIME_BY_ID,
    DATASETDB_STMT_SET_CONTENT_MTIME_BY_COMPID_REV,
    DATASETDB_STMT_SET_CONTENT_SIZE_BY_COMPID_REV,
    DATASETDB_STMT_SET_COMPONENT_VERSION_BY_ID,
    DATASETDB_STMT_SET_CONTENT_VERSION_BY_COMPID_REV,
    DATASETDB_STMT_SET_CONTENT_LOCATION_BY_COMPID_REV,
    DATASETDB_STMT_SET_COMPONENT_PARENT_BY_ID,
    DATASETDB_STMT_SET_COMPONENT_NAME_BY_ID,
    DATASETDB_STMT_SET_CONTENT_SIZE_BY_LOCATION,
    DATASETDB_STMT_DELETE_COMPONENT_BY_ID,
    DATASETDB_STMT_DELETE_CONTENT_BY_COMPID,
    DATASETDB_STMT_COUNT_LOCATION,
    DATASETDB_STMT_COUNT_CHILD_COMPS,
    DATASETDB_STMT_GET_MAX_COMPONENT_ID,
    DATASETDB_STMT_GET_COMPONENT_IN_RANGE,
    DATASETDB_STMT_SET_COMPONENT_ID_BY_ID,
    DATASETDB_STMT_SET_COMPONENT_PARENT_BY_PARENT_ID,
    DATASETDB_STMT_MAX,  // this must be last
};

DEFSQL(BEGIN,                                   \
       "BEGIN IMMEDIATE");
DEFSQL(COMMIT,                                  \
       "COMMIT");
DEFSQL(ROLLBACK,                                \
       "ROLLBACK");
DEFSQL(GET_VERSION,                             \
       "SELECT value "                          \
       "FROM version "                          \
       "WHERE id=:id");
DEFSQL(SET_VERSION,                             \
       "UPDATE version "                        \
       "SET value=:value "                      \
       "WHERE id=:id");
DEFSQL(ADD_COMPONENT_BY_NAME_PARENT,                                    \
       "INSERT INTO component (name, upname, parent, type, perm, ctime, mtime, version, cur_rev) " \
       "VALUES (:name, :upname, :parent, :type, :perm, :time, :time, :version, :cur_rev)");
DEFSQL(ADD_CONTENT_BY_COMPID_REV,                                       \
       "INSERT INTO content (compid, location, size, mtime, rev, version) " \
       "VALUES (:compid, :location, :size, :mtime, :rev, :version)");
DEFSQL(GET_COMPONENT_BY_UPNAME_PARENT,                  \
       "SELECT "COMPONENT_COLS" "                       \
       "FROM component "                                \
       "WHERE upname=:upname AND parent=:parent");
DEFSQL(GET_CONTENT_BY_LOCATION,                 \
       "SELECT * "                              \
       "FROM content "                          \
       "WHERE location=:location");
DEFSQL(GET_COMPONENT_BY_ID,                     \
       "SELECT "COMPONENT_COLS" "               \
       "FROM component "                        \
       "WHERE id=:id");
DEFSQL(GET_CONTENT_BY_COMPID_REV,               \
       "SELECT * "                              \
       "FROM content "                          \
       "WHERE compid=:compid AND rev=:rev");
DEFSQL(GET_FILE_COMPONENT_ID_IN_RANGE,          \
       "SELECT id "                             \
       "FROM component "                        \
       "WHERE type=1 "                          \
       "AND id >= :min_id "                     \
       "AND id <= :max_id ");
DEFSQL(GET_NEXT_CONTENT,                        \
       "SELECT * "                              \
       "FROM content "                          \
       "WHERE compid > :compid ORDER BY compid LIMIT 1");
DEFSQL(LIST_COMPONENTS_BY_PARENT,               \
       "SELECT name "                           \
       "FROM component "                        \
       "WHERE parent=:parent");
// for current schema, ComponentInfos could be retrieved from joint table more efficiently.
DEFSQL(LIST_COMPONENTS_INFO_BY_PARENT,                           \
       "SELECT component.name, component.type, "                 \
       "component.perm, component.ctime, component.mtime, "      \
       "component.version, content.location, content.size "      \
       "FROM component "                                         \
       "LEFT OUTER JOIN content ON content.compid=component.id " \
       "WHERE component.parent=:parent");
DEFSQL(SET_COMPONENT_PERM_BY_ID,                \
       "UPDATE component "                      \
       "SET perm=:perm "                        \
       "WHERE id=:id");
DEFSQL(SET_COMPONENT_CTIME_BY_ID,               \
       "UPDATE component "                      \
       "SET ctime=:ctime "                      \
       "WHERE id=:id");
DEFSQL(SET_COMPONENT_MTIME_BY_ID,               \
       "UPDATE component "                      \
       "SET mtime=:mtime "                      \
       "WHERE id=:id");
DEFSQL(SET_CONTENT_MTIME_BY_COMPID_REV,         \
       "UPDATE content "                        \
       "SET mtime=:mtime "                      \
       "WHERE compid=:compid AND rev=:rev");
DEFSQL(SET_CONTENT_SIZE_BY_COMPID_REV,          \
       "UPDATE content "                        \
       "SET size=:size "                        \
       "WHERE compid=:compid AND rev=:rev");
DEFSQL(SET_COMPONENT_VERSION_BY_ID,             \
       "UPDATE component "                      \
       "SET version=:version "                  \
       "WHERE id=:id");
DEFSQL(SET_CONTENT_VERSION_BY_COMPID_REV,       \
       "UPDATE content "                        \
       "SET version=:version "                  \
       "WHERE compid=:compid AND rev=:rev");
DEFSQL(SET_CONTENT_LOCATION_BY_COMPID_REV,      \
       "UPDATE content "                        \
       "SET location=:location "                \
       "WHERE compid=:compid AND rev=:rev");
DEFSQL(SET_COMPONENT_PARENT_BY_ID,              \
       "UPDATE component "                      \
       "SET parent=:parent "                    \
       "WHERE id=:id");
DEFSQL(SET_COMPONENT_NAME_BY_ID,                \
       "UPDATE component "                      \
       "SET name=:name, upname=:upname "        \
       "WHERE id=:id");
DEFSQL(SET_CONTENT_SIZE_BY_LOCATION,            \
       "UPDATE content "                        \
       "SET size=:size "                        \
       "WHERE location=:location");
DEFSQL(DELETE_COMPONENT_BY_ID,                  \
       "DELETE FROM component "                 \
       "WHERE id=:id");
DEFSQL(DELETE_CONTENT_BY_COMPID,                \
       "DELETE FROM content "                   \
       "WHERE compid=:compid");
DEFSQL(COUNT_LOCATION,                          \
       "SELECT COUNT(compid) "                  \
       "FROM content "                          \
       "WHERE location=:location");
DEFSQL(COUNT_CHILD_COMPS,                       \
       "SELECT COUNT(id) "                      \
       "FROM component "                        \
       "WHERE parent=:parent");
DEFSQL(GET_MAX_COMPONENT_ID,                    \
       "SELECT max(id) "                        \
       "FROM component");
DEFSQL(GET_COMPONENT_IN_RANGE,                  \
       "SELECT * "                              \
       "FROM component "                        \
       "WHERE id >= :min_id "                   \
       "AND id <= :max_id ");
DEFSQL(SET_COMPONENT_ID_BY_ID,                  \
       "UPDATE component "                      \
       "SET id=:new_id "                        \
       "WHERE id=:old_id");
DEFSQL(SET_COMPONENT_PARENT_BY_PARENT_ID,       \
       "UPDATE component "                      \
       "SET parent=:new_parent "                \
       "WHERE parent=:old_parent");

static int defaultConstructContentPathFromNum(u64 num,
                                              std::string &location)
{
    /* Explanation of scheme:
     *
     * (1) |       0 (34b)       | d1 (10b) | d2 (10b) | f3 (10b) | -> d1/d2/f3
     * (2) |  0 (21b) | d1 (11b) | d2 (11b) | d3 (10b) | f4 (11b) | -> d1/d2/d3/f4
     * (3) | d1 (18b) | d2 (12b) | d3 (11b) | d4 (11b) | f5 (12b) | -> d1/d2/d3/d4/f5
     * 
     * Discussion:
     * (1) will have no more than 1024 entries in each directory.
     * (2) will have no more than 2048 entries in each directory.
     * (3) will have no more than 4096 entries in each directory, except for the top directory.
     * To avoid name collision between directories and files, files will be prefixed by "x".
     */

    std::ostringstream oss;
    if (num >> 30 == 0) {  // case (1)
        oss << std::hex << ((num >> 20) & 0x3ff)<< "/" << ((num >> 10) & 0x3ff) << "/x" << (num & 0x3ff);
    }
    else if (num >> 43 == 0) {  // case (2)
        oss << std::hex << ((num >> 32) & 0x7ff) << "/" << ((num >> 21) & 0x7ff) << "/" << ((num >> 11) & 0x3ff) << "/x" << (num & 0x7ff);
    }
    else {  // case (3)
        oss << std::hex << (num >> 46) << "/" << ((num >> 34) & 0xfff) << "/" << ((num >> 23) & 0x7ff) << "/" << ((num >> 12) & 0x7ff) << "/x" << (num & 0xfff);
    }
    location = oss.str();

    return 0;
}

static int defaultConstructNumFromContentPath(const std::string &location,
                                              u64 &num)
{
    std::string str1, str2, str3, str4, str5;
    u32 nslashes = 2;   // can be 2, 3, or 4
    size_t pos1, pos2, pos3, pos4;

    pos1 = location.find('/');
    str1 = location.substr(0, pos1);

    pos2 = location.find('/', pos1 + 1);
    str2 = location.substr(pos1 + 1, pos2 - pos1 - 1);

    pos3 = location.find('/', pos2 + 1);
    if (pos3 == std::string::npos) {
        str3 = location.substr(pos2 + 2);   // skip the 'x'
    } else {
        nslashes++;
        str3 = location.substr(pos2 + 1, pos3 - pos2 - 1);

        pos4 = location.find('/', pos3 + 1);
        if (pos4 == std::string::npos) {
            str4 = location.substr(pos3 + 2);   // skip the 'x'
        } else {
            nslashes++;
            str4 = location.substr(pos3 + 1, pos4 - pos3 - 1);
            str5 = location.substr(pos4 + 2);   // skip the 'x'
        }
    }

    if (nslashes == 2) {
        num = (VPLConv_strToU64(str1.c_str(), NULL, 16) << 20) | (VPLConv_strToU64(str2.c_str(), NULL, 16) << 10) | (VPLConv_strToU64(str3.c_str(), NULL, 16));
    } else if (nslashes == 3) {
        num = (VPLConv_strToU64(str1.c_str(), NULL, 16) << 32) | (VPLConv_strToU64(str2.c_str(), NULL, 16) << 21) | (VPLConv_strToU64(str3.c_str(), NULL, 16) << 11) | (VPLConv_strToU64(str4.c_str(), NULL, 16));
    } else {    // nslashes == 4
        num = (VPLConv_strToU64(str1.c_str(), NULL, 16) << 46) | (VPLConv_strToU64(str2.c_str(), NULL, 16) << 34) | (VPLConv_strToU64(str3.c_str(), NULL, 16) << 23) | (VPLConv_strToU64(str4.c_str(), NULL, 16) << 12) | (VPLConv_strToU64(str5.c_str(), NULL, 16));
    }   

    return 0;
}

DatasetDBError DatasetDB::RegisterContentPathConstructFunction(int (*constructContentPathFromNum)(u64 num, std::string &path), int (*constructNumFromContentPath)(const std::string &path, u64 &num))
{
    this->constructContentPathFromNum = constructContentPathFromNum;
    this->constructNumFromContentPath = constructNumFromContentPath;

    return DATASETDB_OK;
}


DatasetDB::DatasetDB() : db(NULL),
    client_cnt(0),
    force_backup_stop(false)
{
    constructContentPathFromNum = defaultConstructContentPathFromNum;
    constructNumFromContentPath = defaultConstructNumFromContentPath;
    dbstmts.resize(DATASETDB_STMT_MAX, NULL);
    VPLMutex_Init(&mutex);    
    VPLMutex_Init(&cnt_mutex);
    VPLCond_Init(&backup_cond);
    options = 0;
}

DatasetDB::~DatasetDB()
{
    CloseDB();

    VPLMutex_Destroy(&mutex);
    VPLMutex_Destroy(&cnt_mutex);
    VPLCond_Destroy(&backup_cond);
}

static int count_version_tables(void *param, int, char**, char**)
{
    int *count = (int*)param;
    (*count)++;
    return 0;
}

#ifdef NOTDEF
static int check_integrity_rows(void* param, int num_cols, char** cols, char**)
{
    int *count = (int*)param;

    if(num_cols != 1 || strncmp(cols[0], "ok", 2) != 0) {
        (*count)++;
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Integrity check fail: %s", cols[0]);
    }

    return 0;
}
#endif // NOTDEF

DatasetDBError DatasetDB::OpenDB(const std::string &dbpath, u32 options)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;
    char *errmsg = NULL;
    int count = 0;
    
    db_lock();
    if (db) {
        result = DATASETDB_ERR_DB_ALREADY_OPEN;
        goto end;
    }

    this->dbpath.assign(dbpath);
    this->options = options;

    // recover from any aborted swap backup attempt
    swapInBackupRecover();
    force_backup_stop = false;

    dberr = sqlite3_open_v2(dbpath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to create/open DB file at %s: %d", dbpath.c_str(), dberr);
        result = DATASETDB_ERR_DB_OPEN_FAIL;
        goto end;
    }

    // Use WAL journals as they are less punishing to performance. PERSIST
    // transactions have a penalty of around 100ms/per commit. WAL is a 
    // a write ahead with the main benefits that writers do not block readers
    // and readers do not block writers. It also performs fewer fsync() ops.
    dberr = sqlite3_exec(db, "PRAGMA journal_mode=WAL", NULL, NULL, &errmsg);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to enable WAL journaling: %d(%d): %s", dberr, sqlite3_extended_errcode(db), errmsg);
        sqlite3_free(errmsg);
        result = mapSqliteErrCode(dberr);
        goto end;
    }

    // Define schema if missing.
    dberr = sqlite3_exec(db, test_and_create_db_sql, NULL, NULL, &errmsg);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to test/define schema: %d(%d): %s", dberr, sqlite3_extended_errcode(db), errmsg);
        sqlite3_free(errmsg);
        result = mapSqliteErrCode(dberr);
        goto end;
    }

    // Make sure the component table exists.
    dberr = sqlite3_exec(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='component'", count_version_tables, &count, &errmsg);
    if (dberr != SQLITE_OK) {
        LOG_ERROR("Failed to determine whether required tables exist: %d(%d): %s", dberr, sqlite3_extended_errcode(db), errmsg);
        sqlite3_free(errmsg);
        result = mapSqliteErrCode(dberr);
        goto end;
    }
    if (count == 0) {  // component table is missing
        LOG_ERROR("Required tables are missing: %d", count);
        result = DATASETDB_ERR_TABLES_MISSING;
        goto end;
    }

    // check db schema version
    // If schema version isn't latest, apply schema updates.
    dberr = updateSchema();
    if (dberr != DATASETDB_OK) {
        LOG_ERROR("Failed to update database schema: %d", dberr);
        goto end;
    }

    // It's been decided to remove the integrity check support. The feedback
    // we've seen is that it's usually used during development and debug and
    // not performed in the field. Issues include:
    // 1. Takes a long time to perform for a large database.
    // 2. Doesn't find all issues. Database can be fairly corrupt and
    //    it won't notice.
    // 3. Doesn't help in repairing. At best lets you know the database is bad.

 end:
    if (result != DATASETDB_OK) {
        sqlite3_close(db);
        db = NULL;
    }

    db_unlock();
    return result;
}

DatasetDBError DatasetDB::CloseDB()
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;
    int err_cnt = 0;

    db_lock();
    if (db) {
        std::vector<sqlite3_stmt*>::iterator it;
        for (it = dbstmts.begin(); it != dbstmts.end(); it++) {
            if (*it != NULL) {
                dberr = sqlite3_finalize(*it);
                if (isSqliteError(dberr)) {
                    err_cnt++;
                    result = mapSqliteErrCode(dberr);
                    LOG_ERROR("Failed to finalize prepared stmt: %d, %s", dberr, sqlite3_errmsg(db));
                }
                *it = NULL;
            }
        }

        dberr = sqlite3_close(db);
        if (isSqliteError(dberr)) {
            err_cnt++;
            result = mapSqliteErrCode(dberr);
            LOG_ERROR("Failed to close DB connection: %d, %s", dberr, sqlite3_errmsg(db));
        }

        force_backup_stop = false;
        db = NULL;
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::GetDatasetFullVersion(u64 &version)
{
    return getVersion(VERID_DATASET_VERSION, version);
}

DatasetDBError DatasetDB::SetDatasetFullVersion(u64 version)
{
    return setVersion(VERID_DATASET_VERSION, version);
}

// DEPRECATED - RETURNS DATASETDB_ERR_BAD_REQUEST
DatasetDBError DatasetDB::SetDatasetFullMergeVersion(u64 version)
{
    return DATASETDB_ERR_BAD_REQUEST;
}

DatasetDBError DatasetDB::BeginTransaction()
{
    return beginTransaction();
}

DatasetDBError DatasetDB::CommitTransaction()
{
    return commitTransaction();
}

DatasetDBError DatasetDB::RollbackTransaction()
{
    return rollbackTransaction();
}


static std::string normalizePath(const std::string& path)
{
    std::string rv = path;

    // Component names must not have trailing NULL characters.
    while(rv.size() > 0 && rv[rv.size() - 1] == '\0') {
        rv.erase(rv.size() - 1, 1);
    }

    // Component names must not have multiple consecutive,
    // leading, or trailing slashes.
    while(rv.find("//") != std::string::npos) {
        rv.replace(rv.find("//"), 2, "/");
    }
    while(rv.size() > 0 && rv[0] == '/') {
        rv.erase(0, 1);
    }
    while(rv.size() > 0 && rv[rv.size() - 1] == '/') {
        rv.erase(rv.size() - 1, 1);
    }

    return rv;
}

// Examples:
// "" -> ""
// "abc" -> ""
// "abc/def" -> "abc"
// "abc/def/ghi" -> "abc/def"
// precondition: path is normalized
static std::string getDirName(const std::string &path)
{
    size_t pos = path.rfind('/');
    if (pos != path.npos) {
        return path.substr(0, pos);
    }
    else {
        return "";
    }
}

// Examples:
// "" -> ""
// "abc" -> "abc"
// "abc/def" -> "def"
// "abc/def/ghi" -> "ghi"
// precondition: path is normalized
static std::string getBaseName(const std::string &path)
{
    size_t pos = path.rfind('/');
    if (pos != path.npos) {
        return path.substr(pos + 1);
    }
    else {
        return path;
    }
}

// Split component path into component names
// Examples:
// "" -> [""]
// "a" -> ["", "a"]
// "a/b/c" -> ["", "a", "b", "c"]
// precondition: path is normalized
static void splitComponentPath(const std::string &path,
                               std::vector<std::string> &names)
{
    names.clear();

    names.push_back("");
    if (path.empty())
        return;

    size_t start = 0;
    while (start < path.npos) {
        size_t pos = path.find('/', start);
        if (pos == path.npos) {  // last part
            names.push_back(path.substr(start));
            break;
        }
        else {
            names.push_back(path.substr(start, pos - start));
            start = pos + 1;
        }
    }
}

DatasetDBError DatasetDB::TestAndCreateComponent(const std::string &_path,
                                                 int type,
                                                 u32 perm,
                                                 u64 version,
                                                 VPLTime_t time,
                                                 std::string *location,
                                                 bool (*locationExists)(const std::string &data_path, const std::string &path),
                                                 std::string *data_path)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;

    db_lock();

    if (path == "") {  // special handling for "" (root component)
        result = testAndCreateRootComponent(type, perm, version, time);
    }
    else {
        result = testAndCreateNonRootComponent(path, type, perm, version, time, location, locationExists, data_path);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::TestAndCreateLostAndFound(u64 version,
                                                    bool lost_and_found_system,
                                                    VPLTime_t time)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = DATASETDB_OK;
    std::string path = normalizePath(DATASETDB_LOST_AND_FOUND_PATH);
    ComponentAttrs comp;
    std::vector<ComponentAttrs> comps;
    u64 component_id;

    db_lock();

    err = getComponentsOnPath(getDirName(path), comps);
    CHECK_RV(err, result, end);

    if (comps.back().type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        result = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
        goto end;
    }

    err = getComponentByNameParent(getBaseName(path), comps.back().id, comp);
    if (err == DATASETDB_ERR_UNKNOWN_COMPONENT) {
        // The "lost+found" directory doesn't exist, create it

        // Update version first
        err = setComponentsVersion(comps, version);
        CHECK_RV(err, result, end);

        err = addComponentByNameParent(getBaseName(path), comps.back().id, version, time, DATASETDB_COMPONENT_TYPE_DIRECTORY, 0, component_id);
        CHECK_RV(err, result, end);

        err = setComponentMTimeById(comps.back().id, time);
        CHECK_RV(err, result, end);

        goto end;
    } else {
        CHECK_RV(err, result, end);
    }

    // The "lost+found" directory already exists and it's created by the system, then
    // there's nothing left to do
    if (lost_and_found_system) {
        goto end;
    }    

    // If "lost+found" was created by the user, then move its contents
    // to "/lost+found/lost+found-(user)-<timestamp>"
    {
        std::stringstream ss;
        std::string user_name, temp_path;

        // Update root version and timestamp
        err = setComponentVersionById(comps.back().id, version);
        CHECK_RV(err, result, end);

        err = setComponentMTimeById(comps.back().id, time);
        CHECK_RV(err, result, end);

        // Create temporary directory
        ss << time;
        temp_path = "lost+found-temp-" + ss.str();
        temp_path = normalizePath(temp_path);

        err = addComponentByNameParent(getBaseName(temp_path), comps.back().id, version, time, DATASETDB_COMPONENT_TYPE_DIRECTORY, 0, component_id);
        CHECK_RV(err, result, end);

        // Move the user "lost+found" to the temporary directory and change its name
        err = setComponentParentById(comp.id, component_id);
        CHECK_RV(err, result, end);

        user_name = "lost+found-(user)-" + ss.str();
        user_name = normalizePath(user_name);
        err = setComponentNameById(comp.id, user_name);
        CHECK_RV(err, result, end);

        // Rename temporary directory to the final location
        err = setComponentNameById(component_id, getBaseName(path));
        CHECK_RV(err, result, end);

        // Update user "lost+found" version and timestamp
        err = setComponentVersionById(comp.id, version);
        CHECK_RV(err, result, end);

        err = setComponentMTimeById(comp.id, time);
        CHECK_RV(err, result, end);
    }

 end:
    db_unlock();

    return result;
}

DatasetDBError DatasetDB::TestExistComponent(const std::string &_path)
{
    std::string path = normalizePath(_path);
    ComponentAttrs _comp;  // dummy
    return getComponentByPath(path, _comp);
}

DatasetDBError DatasetDB::GetComponentInfo(const std::string &_path,
                                           ComponentInfo &info)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;

    ComponentAttrs comp;
    ContentAttrs content;
    DatasetDBError err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = getContentByCompIdRev(comp.id, 0, content);
        CHECK_RV(err, result, end);
    }

    info.name    = path;
    info.ctime   = comp.ctime;
    info.mtime   = comp.mtime;
    info.type    = (int)comp.type;
    info.perm    = comp.perm;
    info.version = comp.version;
    info.path    = content.location;
    if(comp.type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        info.size = 0;
    } else {
        info.size = content.size;
    }
    info.hash.clear();  // DEPRECATED
    info.metadata.clear();  // DEPRECATED

 end:
    return result;
}

DatasetDBError DatasetDB::GetComponentType(const std::string &_name,
                                                               int &type)
{
    std::string name = normalizePath(_name);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    err = getComponentByPath(name, comp);
    CHECK_RV(err, result, end);

    type = comp.type;

 end:
    return result;
}

DatasetDBError DatasetDB::GetComponentPermission(const std::string &_path,
                                                 u32 &perm)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    perm = comp.perm;

 end:
    return result;
}

DatasetDBError DatasetDB::SetComponentPermission(const std::string &_path,
                                                 u32 perm,
                                                 u64 version)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::vector<ComponentAttrs> comps;
    ComponentAttrs comp;

    db_lock();

    err = getComponentsOnPath(getDirName(path), comps);
    if (err != DATASETDB_OK) {  // parent doesn't exist
        result = err;
        goto end;
    }

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    err = setComponentPermissionById(comp.id, perm);
    CHECK_RV(err, result, end);

    err = setComponentsVersion(comps, version);
    CHECK_RV(err, result, end);

    err = setComponentVersionById(comp.id, version);
    CHECK_RV(err, result, end);

    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = setContentVersionByCompIdRev(comp.id, 0, version);
        CHECK_RV(err, result, end);
    }

 end:
    db_unlock();
    return result;
}

DatasetDBError DatasetDB::GetComponentCreateTime(const std::string &_path,
                                                 VPLTime_t &ctime)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    ctime = comp.ctime;

 end:
    return result;
}

DatasetDBError DatasetDB::SetComponentCreationTime(const std::string &_path,
                                                   u64 version,
                                                   VPLTime_t ctime)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    db_lock();

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    err = setComponentCTimeById(comp.id, ctime);
    CHECK_RV(err, result, end);

 end:
    db_unlock();
    return result;
}

DatasetDBError DatasetDB::GetComponentLastModifyTime(const std::string &_path,
                                                     VPLTime_t &mtime)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    mtime = comp.mtime;

 end:
    return result;
}

DatasetDBError DatasetDB::SetComponentLastModifyTime(const std::string &_path,
                                                     u64 version,
                                                     VPLTime_t mtime)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    db_lock();

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    err = setComponentMTimeById(comp.id, mtime);
    CHECK_RV(err, result, end);

    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = setContentMTimeByCompIdRev(comp.id, 0, mtime);
        CHECK_RV(err, result, end);
    }
 end:
    db_unlock();
    return result;
}

DatasetDBError DatasetDB::GetComponentSize(const std::string &_path,
                                           u64 &size)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;
    ContentAttrs content;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);
   
    // TODO: workaround here, should not call this function for dir
    if (comp.type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        size = 0;
        goto end;
    }

    err = getContentByCompIdRev(comp.id, 0, content);
    CHECK_RV(err, result, end);

    size = content.size;

 end:
    return result;
}

DatasetDBError DatasetDB::SetComponentSize(const std::string &_path,
                                           u64 size,
                                           u64 version,
                                           VPLTime_t time,
                                           bool update_modify_time)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::vector<ComponentAttrs> comps;
    ComponentAttrs comp;

    db_lock();

    err = getComponentsOnPath(getDirName(path), comps);
    if (err != DATASETDB_OK) {  // parent doesn't exist
        result = err;
        goto end;
    }

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    if (comp.type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        result = DATASETDB_ERR_IS_DIRECTORY;
        goto end;
    }

    err = setContentSizeByCompIdRev(comp.id, 0, size);
    CHECK_RV(err, result, end);

    if (update_modify_time) {
        err = SetComponentLastModifyTime(path, version, time);
        CHECK_RV(err, result, end);    
    }

    err = setComponentsVersion(comps, version);
    CHECK_RV(err, result, end);

    err = setComponentVersionById(comp.id, version);
    CHECK_RV(err, result, end);

    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = setContentVersionByCompIdRev(comp.id, 0, version);
        CHECK_RV(err, result, end);
    }

 end:
    db_unlock();
    return result;
}

DatasetDBError DatasetDB::GetComponentVersion(const std::string &_path,
                                              u64 &version)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    version = comp.version;

 end:
    return result;
}

DatasetDBError DatasetDB::SetComponentVersion(const std::string &_path,
                                              u32 perm,
                                              u64 version,
                                              VPLTime_t time)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    db_lock();

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    err = setComponentVersionById(comp.id, version);
    CHECK_RV(err, result, end);
    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = setContentVersionByCompIdRev(comp.id, 0, version);
        CHECK_RV(err, result, end);
    }
 end:
    db_unlock();
    return result;
}

DatasetDBError DatasetDB::GetComponentPath(const std::string &_path,
                                           std::string &location)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;
    ContentAttrs content;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    if (comp.type != DATASETDB_COMPONENT_TYPE_FILE) {
        result = DATASETDB_ERR_IS_DIRECTORY;
        goto end;
    }

    err = getContentByCompIdRev(comp.id, 0, content);
    CHECK_RV(err, result, end);

    if (content.location.empty()) {
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

    location.assign(content.location);

 end:
    return result;
}

// SOON TO BE DEPRECATED
DatasetDBError DatasetDB::SetComponentPath(const std::string &_path,
                                           const std::string &location,
                                           u32 perm,
                                           u64 version,
                                           VPLTime_t time)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;
    ContentAttrs content;
    
    db_lock();

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        // test and create content
        err = getContentByCompIdRev(comp.id, 0, content);
        if(err == DATASETDB_ERR_UNKNOWN_CONTENT) {
            addContentByCompIdRev(comp.id, location, 0, time, 0, version);
        } else if(err != DATASETDB_OK){
            result = err;
            goto end;
        }
        if (content.location != location) {
            err = setContentLocationByCompIdRev(comp.id, 0, location);
            CHECK_RV(err, result, end);
        }
    }

 end:
    db_unlock();
    return result;
}

// DEPRECATED - RETURNS DATASETDB_ERR_BAD_REQUEST
DatasetDBError DatasetDB::SetComponentMetadata(const std::string &_path,
                                               int type, const std::string &value)
{
    return DATASETDB_ERR_BAD_REQUEST;
}

// DEPRECATED - RETURNS DATASETDB_ERR_BAD_REQUEST
DatasetDBError DatasetDB::DeleteComponentMetadata(const std::string &_path,
                                                  int type)
{
    return DATASETDB_ERR_BAD_REQUEST;
}

DatasetDBError DatasetDB::DeleteComponent(const std::string &_path,
                                          u64 version,
                                          VPLTime_t time)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::vector<ComponentAttrs> comps;
    ComponentAttrs comp;
    std::vector<std::string> names;

    db_lock();

    // Do not allow deleting the "lost+found" directory
    if (utf8_casencmp(path.size(), path.c_str(), strlen(DATASETDB_LOST_AND_FOUND_PATH), DATASETDB_LOST_AND_FOUND_PATH) == 0) {
        result = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    err = getComponentsOnPath(getDirName(path), comps);
    CHECK_RV(err, result, end);

    err = getComponentByNameParent(getBaseName(path), comps.back().id, comp);
    CHECK_RV(err, result, end);

    if (comp.type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        // if the component is a directory, it must be empty
        listComponentsByParent(comp.id, names);
        if(!names.empty()) {
            result = DATASETDB_ERR_BAD_REQUEST; 
            goto end;
        }
    }

    // must delete content first since there is a foreign key constrain in compid
    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = deleteContentByCompId(comp.id);
        CHECK_RV(err, result, end);
    }

    err = deleteComponentById(comp.id);
    CHECK_RV(err, result, end);

    // update the mtime of the component's parent directory
    err = setComponentMTimeById(comps.back().id, time);
    CHECK_RV(err, result, end);

    // update version of all components on the path
    err = setComponentsVersion(comps, version);
    CHECK_RV(err, result, end);

 end:
    db_unlock();
    return result;
}

DatasetDBError DatasetDB::MoveComponent(const std::string &_old_path,
                                        const std::string &_new_path,
                                        u32 perm,
                                        u64 version,
                                        VPLTime_t time)
{
    // replace parent with new parent, replace name with new name
    std::string old_path = normalizePath(_old_path);
    std::string new_path = normalizePath(_new_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::vector<ComponentAttrs> src_comps;
    std::vector<ComponentAttrs> dest_comps;
    ComponentAttrs comp;
    ComponentAttrs dest_comp;
    std::string up_new_path;
    std::string up_old_path;
    std::vector<std::string> names;

    db_lock();

    // Do not allow renaming the "lost+found" directory
    if (utf8_casencmp(old_path.size(), old_path.c_str(), strlen(DATASETDB_LOST_AND_FOUND_PATH), DATASETDB_LOST_AND_FOUND_PATH) == 0) {
        result = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    // Do not allow creating children or grand-children of "lost+found" by
    // end users directly
    splitComponentPath(new_path, names);
    if (((names.size() == 3) || (names.size() == 4)) &&
            (utf8_casencmp(names[1].size(), names[1].c_str(), strlen(DATASETDB_LOST_AND_FOUND_PATH), DATASETDB_LOST_AND_FOUND_PATH) == 0)) {
        result = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    // make sure old component exist.
    err = getComponentByPath(old_path, comp);
    CHECK_RV(err, result, end);

    utf8_upper(_new_path.c_str(), up_new_path);
    utf8_upper(_old_path.c_str(), up_old_path);

    err = getComponentsOnPath(getDirName(old_path), src_comps);
    if (err != DATASETDB_OK) {  // source parent doesn't exist
        result = err;
        goto end;
    }

    err = getComponentsOnPath(getDirName(new_path), dest_comps);
    if (err != DATASETDB_OK) {  // destination parent doesn't exist
        result = err;
        goto end;
    }
    
    // if old and new are the same, just update version numbers and modtimes
    if (up_new_path != up_old_path) {
        // assert: parent exists
        if (dest_comps.back().type != 
            DATASETDB_COMPONENT_TYPE_DIRECTORY) {  // parent isn't a directory
            result = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
            goto end;
        }
        // assert: parent exists and is a directory
        err = getComponentByNameParent(getBaseName(new_path), dest_comps.back().id, dest_comp);
        if (err == DATASETDB_OK) {  // new dest component exists
            result = DATASETDB_ERR_BAD_REQUEST;
            goto end;
        }
        if (err != DATASETDB_ERR_UNKNOWN_COMPONENT) {  // unexpected outcome - bail
            result = err;
            goto end;
        }

        err = setComponentParentById(comp.id, dest_comps.back().id);
        CHECK_RV(err, result, end);
    }
    
    err = setComponentNameById(comp.id, getBaseName(new_path));
    CHECK_RV(err, result, end);

    err = setComponentsVersion(src_comps, version);
    CHECK_RV(err, result, end);

    err = setComponentMTimeById(src_comps.back().id, time);
    CHECK_RV(err, result, end); 
    
    err = setComponentsVersion(dest_comps, version);
    CHECK_RV(err, result, end);

    err = setComponentMTimeById(dest_comps.back().id, time);
    CHECK_RV(err, result, end); 

    err = setComponentVersionById(comp.id, version);
    CHECK_RV(err, result, end); 

    if (comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = setContentVersionByCompIdRev(comp.id, 0, version);
        CHECK_RV(err, result, end); 
    }
end:
    db_unlock();
    return result;
}

DatasetDBError DatasetDB::ListComponents(const std::string &_path,
                                         std::vector<std::string> &names)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    if (comp.type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        result = DATASETDB_ERR_NOT_DIRECTORY;
        goto end;
    }

    err = listComponentsByParent(comp.id, names);
    CHECK_RV(err, result, end);

 end:
    return result;
}

DatasetDBError DatasetDB::ListComponentsInfo(const std::string &_path,
                                             std::vector<ComponentInfo> &components_info)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;

    err = getComponentByPath(path, comp);
    CHECK_RV(err, result, end);

    if (comp.type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        result = DATASETDB_ERR_NOT_DIRECTORY;
        goto end;
    }

    err = listComponentsInfoByParent(comp.id,
                                     components_info);
    CHECK_RV(err, result, end);

 end:
    return result;
}

DatasetDBError DatasetDB::GetSiblingComponentsCount(const std::string &_path,
                                                    int &count)
{
    std::string path = normalizePath(_path);
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::vector<ComponentAttrs> comps;

    err = getComponentsOnPath(getDirName(path), comps);
    if (err != DATASETDB_OK) {  // parent doesn't exist
        result = err;
        goto end;
    }
    // assert: parent exists
    if (comps.back().type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {  // parent isn't a directory
        result = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
        goto end;
    }
    
    err = getChildComponentsCount(comps.back().id, count);
    CHECK_RV(err, result, end);

 end:
    return result;
}

DatasetDBError DatasetDB::GetContentByLocation(const std::string &location,
                                               ContentAttrs& content)
{
    return getContentByLocation(location, content);
}

DatasetDBError DatasetDB::GetContentRefCount(const std::string &location,
                                             int &count)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;

    if (location.empty()) {
        result = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    err = getContentRefCount(location, count);
    CHECK_RV(err, result, end);

 end:
    return result;
}

// DEPRECATED - NOOP
DatasetDBError DatasetDB::SetContentHash(const std::string &_path,
                                         const std::string &hash)
{
    return DATASETDB_OK;
}

DatasetDBError DatasetDB::AllocateContentPath(std::string &location)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;

    err = generateLocation(location);
    CHECK_RV(err, result, end);

 end:
    return result;
}

DatasetDBError DatasetDB::DeleteContentComponentByLocation(const std::string &location)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ContentAttrs content;
    ComponentAttrs comp;

    db_lock();

    err = getContentByLocation(location, content);
    if( err == DATASETDB_ERR_UNKNOWN_CONTENT) {
        // not found error - just ignore it
        // a content is missing
        // this should not happen
        LOG_WARN("Can't find a content with given location: %s",
                 location.c_str());
        result = DATASETDB_OK;
        goto end;
    } else if (err != DATASETDB_OK) {
        result = err;
        goto end;
    }
    
    // must delete content first since there is a foreign key constrain in compid
    err = deleteContentByCompId(content.compid);
    CHECK_RV(err, result, end);

    err = getComponentById(content.compid, comp);
    if( err == DATASETDB_ERR_UNKNOWN_COMPONENT) {
        // not found error - just ignore it
        // a component is missing
        // since there is a foreign key constrain, this should not happen
        // if this happens, it must be a sqlite bug
        LOG_WARN("A mapped component is missing. component id: "FMTu64","
                 " location: %s",
                 content.compid,
                 location.c_str());
        result = DATASETDB_OK;
        goto end;
    } else if(err != DATASETDB_OK) {
        result = err;
        goto end;
    }

    if(comp.type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = deleteComponentById(comp.id);
        CHECK_RV(err, result, end);
    } else {
        // a content is mapped to a dir component
        LOG_WARN("A content is mapped to a dir component. component id: "FMTu64","
                 " name: %s, location: %s",
                 comp.id,
                 comp.name.c_str(),
                 location.c_str());
    }

 end:
    db_unlock();

    return result;
}

DatasetDBError DatasetDB::DeleteComponentFilesWithoutContent(u64 &id_offset,
                                                  u64 &num_deleted,
                                                  u64 range)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = DATASETDB_OK;
    std::vector<u64> ids;
    u64 max_id = 0;

    db_lock();

    err = getMaxComponentId(max_id);
    CHECK_RV(err, result, end);

    if(range > 0) {
        err = getFileComponentIdInRange(id_offset, range, ids);
        CHECK_RV(err, result, end);

        num_deleted = 0;
        for(int i = 0; i < ids.size(); i++) {
            ContentAttrs content;
            err = getContentByCompIdRev(ids[i], 0, content);
            if(err == DATASETDB_ERR_UNKNOWN_CONTENT) {
                err = deleteComponentById(ids[i]);
                CHECK_RV(err, result, end);
                num_deleted++;
            } else{
                CHECK_RV(err, result, end);
            }
        }

        id_offset = id_offset + range;
        if(id_offset > max_id) {
            result = DATASETDB_ERR_REACH_COMPONENT_END;
        }
    } else {
        result = DATASETDB_ERR_BAD_REQUEST;
    }
 end:
    db_unlock();

    return result;
}

DatasetDBError DatasetDB::CheckComponentConsistency(u64 &id_offset,
                                                    u64 &num_deleted_this_round,
                                                    u64 &num_missing_parents_total,
                                                    VPLTime_t &dir_time,
                                                    u64 maximum_components,
                                                    u64 version,
                                                    u64 range)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = DATASETDB_OK;
    std::vector<ComponentAttrs> comps;
    std::vector<u64> fixed_parent_ids;
    u64 max_id = 0;
    bool fixed;

    db_lock();

    err = getMaxComponentId(max_id);
    CHECK_RV(err, result, end);

    err = getComponentInRange(id_offset, range, comps);
    CHECK_RV(err, result, end);

    num_deleted_this_round = 0;
    for (int i = 0; i < comps.size(); i++) {
        ComponentAttrs comp;

        // Delete file components without a corresponding content entry
        if (comps[i].type == DATASETDB_COMPONENT_TYPE_FILE) {
            ContentAttrs content;

            err = getContentByCompIdRev(comps[i].id, 0, content);
            if (err == DATASETDB_ERR_UNKNOWN_CONTENT) {
                err = deleteComponentById(comps[i].id);
                CHECK_RV(err, result, end);
                num_deleted_this_round++;
            } else {
                CHECK_RV(err, result, end);
            }
        }

        // Move components with invalid parents to "lost+found"

        // Root's parent is 0, skip it
        if (comps[i].parent == 0) {
            continue;
        }

        // The same invalid parent may already be fixed within the same call, no need
        // to fix it again
        fixed = false;
        for (int j = 0; j < fixed_parent_ids.size(); j++) {
            if (comps[i].parent == fixed_parent_ids[j]) {
                fixed = true;
                break;
            }
        }
        if (fixed) {
            continue;
        }

        // Fix components without a valid parent
        err = getComponentById(comps[i].parent, comp);
        if ((err == DATASETDB_ERR_UNKNOWN_COMPONENT) ||
            ((err == DATASETDB_OK) && (comp.type != DATASETDB_COMPONENT_TYPE_DIRECTORY))) {
            // The dir_time is used for setting the directory names of "MM-DD-YYYY" and
            // "orphaned-entries-<timestamp>
            if ((num_missing_parents_total % maximum_components) == 0) {
                dir_time = VPLTime_GetTime();
            }

            err = recoverMissingParentComponent(err, comps[i], comp,
                                                num_missing_parents_total, maximum_components,
                                                dir_time, version, VPLTime_GetTime());
            CHECK_RV(err, result, end);

            fixed_parent_ids.push_back(comps[i].parent);
            num_missing_parents_total++;
        } else {
            CHECK_RV(err, result, end);
        }
    }

    id_offset = id_offset + range;
    if (id_offset > max_id) {
        result = DATASETDB_ERR_REACH_COMPONENT_END;
    }

 end:
    db_unlock();

    return result;
}

DatasetDBError DatasetDB::MoveContentToLostAndFound(const std::string& location,
                                                    int count,
                                                    VPLTime_t dir_time,
                                                    u64 size,
                                                    u64 version,
                                                    VPLTime_t time)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = DATASETDB_OK;
    std::string lost_and_found_path = normalizePath(DATASETDB_LOST_AND_FOUND_PATH);
    std::string date_path, unref_dir_path, unref_file_path, location_str;
    std::stringstream ss;
    char date_dir[11];
    VPLTime_CalendarTime_t calendar_time;

    db_lock();

    // Move the unreferenced file to
    // "/lost+found/MM-DD-YYYY/unreferenced-files-<timestamp>/<#>"

    // Create the MM-DD-YYYY directory if needed
    VPLTime_ConvertToCalendarTimeLocal(dir_time, &calendar_time);
    snprintf(date_dir, sizeof(date_dir), "%02d-%02d-%04d",
             calendar_time.month,
             calendar_time.day,
             calendar_time.year);
    date_path = lost_and_found_path + "/" + date_dir;
    date_path = normalizePath(date_path);

    err = testAndCreateNonRootComponent(date_path, DATASETDB_COMPONENT_TYPE_DIRECTORY,
                                        0, version, time, NULL, NULL, NULL, true);
    CHECK_RV(err, result, end);

    // Create the unreferenced-files-<timestamp> directory if needed
    ss << dir_time;
    unref_dir_path = date_path + "/" + "unreferenced-files-" + ss.str();
    unref_dir_path = normalizePath(unref_dir_path);

    err = testAndCreateNonRootComponent(unref_dir_path, DATASETDB_COMPONENT_TYPE_DIRECTORY,
                                        0, version, time, NULL, NULL, NULL, true);
    CHECK_RV(err, result, end);

    // Create the <#> file
    ss.str("");
    ss << std::hex << std::setfill('0') << std::setw(4) << std::uppercase << count;
    unref_file_path = unref_dir_path + "/" + ss.str();
    unref_file_path = normalizePath(unref_file_path);

    location_str = location;
    err = testAndCreateNonRootComponent(unref_file_path, DATASETDB_COMPONENT_TYPE_FILE,
                                        0, version, time, &location_str, NULL, NULL, true, size);
    CHECK_RV(err, result, end);

 end:
    db_unlock();

    return result;
}

DatasetDBError DatasetDB::SetContentSizeByLocation(const std::string& location,
                                         u64 size)
{
    return setContentSizeByLocation(location, size);
}

DatasetDBError DatasetDB::GetNextContent(ContentAttrs& content)
{
    return getNextContent(content);
}

//--------------------------------------------------

DatasetDBError DatasetDB::updateSchema()
{
    DatasetDBError rv;
    u64 schemaVersion;

    rv = getVersion(VERID_SCHEMA_VERSION, schemaVersion);
    if (rv != DATASETDB_OK) {
        LOG_ERROR("Failed to get current schema version: %d", rv);
        return rv;
    }
    if (schemaVersion < (u64)TARGET_SCHEMA_VERSION) {
        // BEGIN
        rv = beginTransaction();
        if (rv != DATASETDB_OK) {
            LOG_ERROR("Failed to update schema version to "FMTu64": %d", 
                      (u64)TARGET_SCHEMA_VERSION, rv);
            return rv;
        }

        // Apply updates ad needed.
        switch(schemaVersion) {
        default: // the first version, no update need         
            break;
        }

        // COMMIT
        rv = commitTransaction();
        if (rv != DATASETDB_OK) {
            LOG_ERROR("Failed to update schema version to "FMTu64": %d", 
                      (u64)TARGET_SCHEMA_VERSION, rv);
            return rv;
        }

    }
    else if (schemaVersion > TARGET_SCHEMA_VERSION) {
        // DatasetDB file created by future version of DatasetDB.cpp
        // Nothing could be done.  Close DB and return error.
        CloseDB();
        return DATASETDB_ERR_FUTURE_SCHEMA_VERSION;
    }
    // ELSE schema version in db file is expected version - no action necessary

    return DATASETDB_OK;
}

DatasetDBError DatasetDB::beginTransaction()
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(BEGIN)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(BEGIN), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::commitTransaction()
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COMMIT)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(COMMIT), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::rollbackTransaction()
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ROLLBACK)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(ROLLBACK), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}


DatasetDBError DatasetDB::getVersion(int versionId, u64 &version)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr;

    db_lock();

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
	result = DATASETDB_ERR_UNKNOWN_VERSION;
	goto end;
    }
    // assert: dberr == SQLITE_ROW
    if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER) {
	result = DATASETDB_ERR_BAD_DATA;
	goto end;
    }
    version = sqlite3_column_int64(stmt, 0);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    db_unlock();
    return result;
}

DatasetDBError DatasetDB::setVersion(int versionId, u64 version)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_VERSION)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_VERSION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :value
    dberr = sqlite3_bind_int64(stmt, 1, version);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int(stmt, 2, versionId);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    db_unlock();
    return result;
}


DatasetDBError DatasetDB::testAndCreateRootComponent(int type,
                                                     u32 perm,
                                                     u64 version,
                                                     VPLTime_t time)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    ComponentAttrs comp;
    u64 _component_id;  // dummy

    err = getComponentByNameParent("", 0, comp);
    if (err == DATASETDB_OK) {
        result = (comp.type == type) ? DATASETDB_OK : DATASETDB_ERR_WRONG_COMPONENT_TYPE;
        goto end;
    }
    if (err != DATASETDB_ERR_UNKNOWN_COMPONENT) {  // unexpected outcome - bail
        result = err;
        goto end;
    }
    // assert: "" is missing
    if (type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        result = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    err = addComponentByNameParent("", 0, version, time, type, perm, _component_id);
    CHECK_RV(err, result, end);

 end:
    return result;
}

// precondition: path is normalized
DatasetDBError DatasetDB::testAndCreateNonRootComponent(const std::string &path,
                                                        int type,
                                                        u32 perm,
                                                        u64 version,
                                                        VPLTime_t time,
                                                        std::string *location,
                                                        bool (*locationExists)(const std::string &data_path, const std::string &path),
                                                        std::string *data_path,
                                                        bool is_lost_and_found,
                                                        u64 content_size)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::vector<ComponentAttrs> comps;
    ComponentAttrs comp;
    ContentAttrs content;
    u64 _component_id;
    std::string content_loc;
    std::vector<std::string> names;

    if (!is_lost_and_found) {
        // If not from lost+found, then location is an output argument
        if(location != NULL) {
            *location = "";
        }

        // Do not allow creating children or grand-children of "lost+found" by
        // end users directly
        splitComponentPath(path, names);
        if (((names.size() == 3) || (names.size() == 4)) &&
                (utf8_casencmp(names[1].size(), names[1].c_str(), strlen(DATASETDB_LOST_AND_FOUND_PATH), DATASETDB_LOST_AND_FOUND_PATH) == 0)) {
            result = DATASETDB_ERR_BAD_REQUEST;
            goto end;
        }
    }

    err = getComponentsOnPath(getDirName(path), comps);
    if (err != DATASETDB_OK) {  // parent doesn't exist
        result = err;
        goto end;
    }
    // assert: parent exists
    if (comps.back().type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {  // parent isn't a directory
        result = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
        goto end;
    }
    // assert: parent exists and is a directory
    err = getComponentByNameParent(getBaseName(path), comps.back().id, comp);
    if (err == DATASETDB_OK) {  // component exists
        result = (comp.type == type) ? DATASETDB_OK : DATASETDB_ERR_WRONG_COMPONENT_TYPE;
        goto end;
    }
    if (err != DATASETDB_ERR_UNKNOWN_COMPONENT) {  // unexpected outcome - bail
        result = err;
        goto end;
    }
    // update version first
    err = setComponentsVersion(comps, version);
    CHECK_RV(err, result, end);

    // assert: parent is a directory but component doesn't exist
    err = addComponentByNameParent(getBaseName(path), comps.back().id,
                                   version, time, type, perm, _component_id);
    CHECK_RV(err, result, end);
    err = setComponentMTimeById(comps.back().id, time);
    CHECK_RV(err, result, end);

    if (type == DATASETDB_COMPONENT_TYPE_FILE) {
        if (!is_lost_and_found) {
            // In normal cases, generateLocation() should always generate an unique
            // path since it relies on a monotonically incrementing version. However,
            // in the case of a db restore, the version could have been reset to 0.
            // Before fsck is completed, conflict is thus possible between new files
            // created and old "lost+found" files. To work-around this problem, we
            // pro-activly check for conflicts and call generateLocation again if a
            // conflict is detected
            do {
                err = generateLocation(content_loc);
                CHECK_RV(err, result, end);

                if ((locationExists != NULL) && (data_path != NULL)) {
                    if (!locationExists(*data_path, content_loc)) {
                        break;
                    }
                } else {
                    break;
                }   
            } while (1);
        } else {
            u64 key, implied_key;

            // For the lost+found case, the location is pre-determined
            //
            // In the database back-up restore case, it's possible that the content
            // path version number is back to 0, and thus introduce conflict when we
            // restore old content paths. Before inserting this content path into the
            // database, we would need to update the content path version number if
            // it's greater than the current number in the database
            err = getVersion(VERID_NEXT_CONTPATH_KEY, key);
            CHECK_RV(err, result, end);

            constructNumFromContentPath(*location, implied_key);
            if (implied_key >= key) {
                err = setVersion(VERID_NEXT_CONTPATH_KEY, implied_key + 1);
                CHECK_RV(err, result, end);
            }

            content_loc = *location;
        }
    }

    // test and create content
    err = getContentByCompIdRev(_component_id, 0, content);
    if(err == DATASETDB_OK) {
        // should be one-to-one mapping currently
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    } else if(err != DATASETDB_ERR_UNKNOWN_CONTENT){
        result = err;
        goto end;
    }
    if(type == DATASETDB_COMPONENT_TYPE_FILE) {
        err = addContentByCompIdRev(_component_id, content_loc, content_size, time, 0, version);
        CHECK_RV(err, result, end);
        if(!is_lost_and_found && (location != NULL)) {
            *location = content_loc;
        }
    }

 end:
    return result;
}

// precondition: component must not exist
DatasetDBError DatasetDB::addComponentByNameParent(const std::string &name,
                                                   u64 parent,
                                                   u64 version,
                                                   VPLTime_t time,
                                                   int type,
                                                   u32 perm,
                                                   u64 &component_id)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;
    u32 bindnum = 1;
    std::string upname;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ADD_COMPONENT_BY_NAME_PARENT)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(ADD_COMPONENT_BY_NAME_PARENT), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :name
    dberr = sqlite3_bind_text(stmt, bindnum++, name.data(), name.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
    // bind :upname
    utf8_upper(name.c_str(), upname);
    dberr = sqlite3_bind_text(stmt, bindnum++, upname.data(), upname.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
    // bind :parent
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)parent);
    CHECK_BIND(dberr, result, db, end);
    // bind :type
    dberr = sqlite3_bind_int(stmt, bindnum++, type);
    CHECK_BIND(dberr, result, db, end);
    // bind :perm
    dberr = sqlite3_bind_int(stmt, bindnum++, perm);
    CHECK_BIND(dberr, result, db, end);
    // bind :time
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)time);
    CHECK_BIND(dberr, result, db, end);
    // bind :version
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)version);
    CHECK_BIND(dberr, result, db, end);
    // bind :cur_rev
    // TODO: vcs support
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)0);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    component_id = sqlite3_last_insert_rowid(db);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition component must exist, content must not exist
DatasetDBError DatasetDB::addContentByCompIdRev(u64 compId,
                                                const std::string &location,
                                                u64 size,
                                                VPLTime_t mtime,
                                                u64 rev,
                                                u64 version)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;
    u32 bindnum = 1;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ADD_CONTENT_BY_COMPID_REV)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(ADD_CONTENT_BY_COMPID_REV), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :compId
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)compId);
    CHECK_BIND(dberr, result, db, end);
    // bind :location
    dberr = sqlite3_bind_text(stmt, bindnum++, location.data(), location.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
    // bind :size
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)size);
    CHECK_BIND(dberr, result, db, end);
    // bind :mtime
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)mtime);
    CHECK_BIND(dberr, result, db, end);
    // bind :rev
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)rev);
    CHECK_BIND(dberr, result, db, end);
    // bind :version
    dberr = sqlite3_bind_int64(stmt, bindnum++, (s64)version);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getComponentFromStmt(sqlite3_stmt *stmt,
                                               ComponentAttrs &comp)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_DONE) {
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }
    // assert: dberr == SQLITE_ROW

    comp.id      = sqlite3_column_int64(stmt, 0);
    comp.name.assign((const char*)sqlite3_column_text(stmt, 1));
    comp.upname.assign((const char*)sqlite3_column_text(stmt, 2));
    comp.parent  = (u64)sqlite3_column_int64(stmt, 3);
    comp.type    = (DatasetDBComponentType)sqlite3_column_int(stmt, 4);
    comp.perm    = sqlite3_column_int(stmt, 5);
    comp.ctime   = (VPLTime_t)sqlite3_column_int64(stmt, 6);
    comp.mtime   = (VPLTime_t)sqlite3_column_int64(stmt, 7);
    comp.version = (u64)sqlite3_column_int64(stmt, 8);
    comp.origin_dev = (u64)sqlite3_column_int64(stmt, 9);
    comp.cur_rev = (u64)sqlite3_column_int64(stmt, 10);

 end:
    return result;
}

// precondition: path is normalized
DatasetDBError DatasetDB::getComponentByPath(const std::string &path,
                                             ComponentAttrs &comp)
{
    DatasetDBError result = DATASETDB_OK;

    std::vector<std::string> names;
    splitComponentPath(path, names);

    ComponentAttrs _comp;
    u64 parent = 0;
    std::vector<std::string>::const_iterator it;
    for (it = names.begin(); it != names.end(); it++) {
        DatasetDBError err = getComponentByNameParent(*it, parent, _comp);
        CHECK_RV(err, result, end);
        parent = _comp.id;
    }
    comp = _comp;

 end:
    return result;
}

DatasetDBError DatasetDB::getComponentByNameParent(const std::string &name, u64 parent,
                                                   ComponentAttrs &comp)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::string upname;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_COMPONENT_BY_UPNAME_PARENT)];
    if (stmt == NULL) {
        err = sqlite3_prepare_v2(db, GETSQL(GET_COMPONENT_BY_UPNAME_PARENT), -1, &stmt, NULL);
        CHECK_PREPARE(err, result, db, end);
    }

    // bind :upname
    utf8_upper(name.c_str(), upname);
    err = sqlite3_bind_text(stmt, 1, upname.data(), upname.size(), NULL);
    CHECK_BIND(err, result, db, end);
    // bind :parent
    err = sqlite3_bind_int64(stmt, 2, (s64)parent);
    CHECK_BIND(err, result, db, end);

    err = getComponentFromStmt(stmt, comp);
    CHECK_RV(err, result, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getContentByLocation(const std::string &location,
                                               ContentAttrs &content)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;
    std::string upname;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_CONTENT_BY_LOCATION)];
    if (stmt == NULL) {
        err = sqlite3_prepare_v2(db, GETSQL(GET_CONTENT_BY_LOCATION), -1, &stmt, NULL);
        CHECK_PREPARE(err, result, db, end);
    }

    // bind :location
    err = sqlite3_bind_text(stmt, 1, location.data(), location.size(), NULL);
    CHECK_BIND(err, result, db, end);

    err = getContentByStmt(stmt, content);
    CHECK_RV(err, result, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getComponentById(u64 id,
                                           ComponentAttrs &comp)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_COMPONENT_BY_ID)];
    if (stmt == NULL) {
        err = sqlite3_prepare_v2(db, GETSQL(GET_COMPONENT_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(err, result, db, end);
    }

    // bind :id
    err = sqlite3_bind_int(stmt, 1, (s64)id);
    CHECK_BIND(err, result, db, end);

    err = getComponentFromStmt(stmt, comp);
    CHECK_RV(err, result, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getComponentPathById(u64 id,
                                               std::string &path)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_COMPONENT_BY_ID)];
    if (stmt == NULL) {
        err = sqlite3_prepare_v2(db, GETSQL(GET_COMPONENT_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(err, result, db, end);
    }

    path.clear();
    while (1) {
        // bind :id
        err = sqlite3_bind_int64(stmt, 1, (s64)id);
        CHECK_BIND(err, result, db, end);

        ComponentAttrs comp;
        err = getComponentFromStmt(stmt, comp);
        CHECK_RV(err, result, end);

        if (comp.name == "") {
            break;
        }

        if (path.empty()) {
            path = comp.name;
        }
        else {
            path = comp.name + "/"+ path;
        }

        id = comp.parent;

        sqlite3_reset(stmt);
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// get vector of components
// Examples:
// "" -> [""]
// "a" -> ["", "a"]
// "a/b/c" -> ["", "a", "b", "c"]
// precondition: path is normalized
DatasetDBError DatasetDB::getComponentsOnPath(const std::string &path,
                                              std::vector<ComponentAttrs> &comps)
{
    DatasetDBError result = DATASETDB_OK;
    comps.clear();

    std::vector<std::string> names;
    splitComponentPath(path, names);

    u64 parent = 0;
    std::vector<std::string>::const_iterator it;
    for (it = names.begin(); it != names.end(); it++) {
        ComponentAttrs comp;
        DatasetDBError err = getComponentByNameParent(*it, parent, comp);
        CHECK_RV(err, result, end);
        comps.push_back(comp);
        parent = comp.id;
    }

 end:
    return result;
}

// precondition: content must exists
DatasetDBError DatasetDB::getContentByCompIdRev(u64 compId,
                                                u64 rev,
                                                ContentAttrs &content) 
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_CONTENT_BY_COMPID_REV)];
    if (stmt == NULL) {
        err = sqlite3_prepare_v2(db, GETSQL(GET_CONTENT_BY_COMPID_REV), -1, &stmt, NULL);
        CHECK_PREPARE(err, result, db, end);
    }

    // bind :compid
    err = sqlite3_bind_int64(stmt, 1, (s64)compId);
    CHECK_BIND(err, result, db, end);

    // bind :rev
    err = sqlite3_bind_int64(stmt, 2, (s64)rev);
    CHECK_BIND(err, result, db, end);

    err = getContentByStmt(stmt, content);
    CHECK_RV(err, result, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getContentByStmt(sqlite3_stmt *stmt,
                                           ContentAttrs &content)
{
    int rc = DATASETDB_OK;
    int rv;
    const char *location;

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
    rc = DATASETDB_ERR_UNKNOWN_CONTENT;
    goto end;
    }
    // rv == SQLITE_ROW

    content.compid = sqlite3_column_int64(stmt, 0);
    location = (const char*)sqlite3_column_text(stmt, 1);
    if (location) {
    content.location.assign(location);
    }
    else {
        content.location.clear();
    }
    content.size = sqlite3_column_int64(stmt, 2);
    content.mtime = sqlite3_column_int64(stmt, 3);
    content.baserev = sqlite3_column_int64(stmt, 4);
    content.update_dev = sqlite3_column_int64(stmt, 5);
    content.rev = sqlite3_column_int64(stmt, 6);
    content.version = sqlite3_column_int64(stmt, 7);

 end:
    return rc;
}

// precondition: component exists
DatasetDBError DatasetDB::setComponentIdById(u64 old_id, u64 new_id)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_ID_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_ID_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :new_id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)new_id);
    CHECK_BIND(dberr, result, db, end);
    // bind :old_id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)old_id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setComponentPermissionById(u64 id,
                                                     u32 perm)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_PERM_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_PERM_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :perm
    dberr = sqlite3_bind_int(stmt, 1, perm);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setComponentCTimeById(u64 id,
                                                VPLTime_t ctime)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_CTIME_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_CTIME_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :ctime
    dberr = sqlite3_bind_int64(stmt, 1, (s64)ctime);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setComponentMTimeById(u64 id,
                                                VPLTime_t mtime)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_MTIME_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_MTIME_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :mtime
    dberr = sqlite3_bind_int64(stmt, 1, (s64)mtime);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setContentMTimeByCompIdRev(u64 compid,
                                                     u64 rev,
                                                     VPLTime_t mtime)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_MTIME_BY_COMPID_REV)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_MTIME_BY_COMPID_REV), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :mtime
    dberr = sqlite3_bind_int64(stmt, 1, (s64)mtime);
    CHECK_BIND(dberr, result, db, end);
    // bind :compid
    dberr = sqlite3_bind_int64(stmt, 2, (s64)compid);
    CHECK_BIND(dberr, result, db, end);
    // bind :rev
    dberr = sqlite3_bind_int64(stmt, 3, (s64)rev);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setContentSizeByCompIdRev(u64 compid,
                                                    u64 rev,
                                                    u64 size)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_SIZE_BY_COMPID_REV)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_SIZE_BY_COMPID_REV), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :size
    dberr = sqlite3_bind_int64(stmt, 1, (s64)size);
    CHECK_BIND(dberr, result, db, end);
    // bind :compid
    dberr = sqlite3_bind_int64(stmt, 2, (s64)compid);
    CHECK_BIND(dberr, result, db, end);
    // bind :rev
    dberr = sqlite3_bind_int64(stmt, 3, (s64)rev);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setComponentVersionById(u64 id,
                                                  u64 version)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_VERSION_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_VERSION_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :version
    dberr = sqlite3_bind_int64(stmt, 1, (s64)version);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // multiple rows updated -> something wrong with db
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setContentVersionByCompIdRev(u64 compid,
                                                       u64 rev,
                                                       u64 version)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_VERSION_BY_COMPID_REV)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_VERSION_BY_COMPID_REV), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :version
    dberr = sqlite3_bind_int64(stmt, 1, (s64)version);
    CHECK_BIND(dberr, result, db, end);
    // bind :compid
    dberr = sqlite3_bind_int64(stmt, 2, (s64)compid);
    CHECK_BIND(dberr, result, db, end);
    // bind :rev
    dberr = sqlite3_bind_int64(stmt, 3, (s64)rev);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::setContentLocationByCompIdRev(u64 compid,
                                                        u64 rev,
                                                        const std::string &location)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_LOCATION_BY_COMPID_REV)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_LOCATION_BY_COMPID_REV), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :location
    dberr = sqlite3_bind_text(stmt, 1, location.data(), location.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
    // bind :compid
    dberr = sqlite3_bind_int64(stmt, 2, (s64)compid);
    CHECK_BIND(dberr, result, db, end);
    // bind :rev
    dberr = sqlite3_bind_int64(stmt, 3, (s64)rev);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: parent component exists
DatasetDBError DatasetDB::setComponentParentById(u64 id,
                                                 u64 parent)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_PARENT_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_PARENT_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :location
    dberr = sqlite3_bind_int64(stmt, 1, (s64)parent);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 2, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: parent component exists
DatasetDBError DatasetDB::setComponentParentByParentId(u64 old_parent,
                                                       u64 new_parent)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_PARENT_BY_PARENT_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_PARENT_BY_PARENT_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :new_parent
    dberr = sqlite3_bind_int64(stmt, 1, (s64)new_parent);
    CHECK_BIND(dberr, result, db, end);
    // bind :old_parent
    dberr = sqlite3_bind_int64(stmt, 2, (s64)old_parent);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    default: // one or more rows updated
        break;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: new name component doesn't exists
DatasetDBError DatasetDB::setComponentNameById(u64 id,
                                               const std::string &name)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;
    std::string upname;
    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_NAME_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_NAME_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :name
    dberr = sqlite3_bind_text(stmt, 1, name.data(), name.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
// bind :upname
    utf8_upper(name.c_str(), upname);
    dberr = sqlite3_bind_text(stmt, 2, upname.data(), upname.size(), NULL);
    CHECK_BIND(dberr, result, db, end);
    // bind :id
    dberr = sqlite3_bind_int64(stmt, 3, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    switch (sqlite3_changes(db)) {
    case 0:
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        result = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

// precondition: all components exist
DatasetDBError DatasetDB::setComponentsVersion(const std::vector<ComponentAttrs> &comps,
                                               u64 version)
{
    DatasetDBError result = DATASETDB_OK;

    std::vector<ComponentAttrs>::const_iterator it;
    for (it = comps.begin(); it != comps.end(); it++) {
        // from top to bottom
        DatasetDBError err = setComponentVersionById(it->id, version);
        CHECK_RV(err, result, end);
        if(it->type == DATASETDB_COMPONENT_TYPE_FILE) {
            DatasetDBError err = setContentVersionByCompIdRev(it->id, 0, version);
            CHECK_RV(err, result, end);
        }
    }

 end:
    return result;
}

DatasetDBError DatasetDB::setContentSizeByLocation(const std::string &location,
                                                   u64 size)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_SIZE_BY_LOCATION)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_SIZE_BY_LOCATION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :size
    dberr = sqlite3_bind_int64(stmt, 1, (s64)size);
    CHECK_BIND(dberr, result, db, end);
    // bind :location
    dberr = sqlite3_bind_text(stmt, 2, location.data(), location.size(), NULL);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (sqlite3_changes(db) == 0) {  // no rows affected -> component is missing
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getNextContent(ContentAttrs& content)
{
    int rc = DATASETDB_OK;
    int rv;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_NEXT_CONTENT)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_NEXT_CONTENT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_bind_int64(stmt, 1, content.compid);
    CHECK_BIND(rv, rc, db, end);

    rv = getContentByStmt(stmt, content);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    db_unlock();
    return rc;
}

DatasetDBError DatasetDB::getFileComponentIdInRange(u64 id_offset,
                                                    u64 range,
                                                    std::vector<u64> &ids)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_FILE_COMPONENT_ID_IN_RANGE)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_FILE_COMPONENT_ID_IN_RANGE), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :min
    dberr = sqlite3_bind_int(stmt, 1, (s64)id_offset);
    CHECK_BIND(dberr, result, db, end);

    // bind :max
    dberr = sqlite3_bind_int(stmt, 2, (s64)(id_offset + range - 1));
    CHECK_BIND(dberr, result, db, end);

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        ids.push_back((u64)sqlite3_column_int64(stmt, 0));
    }
    CHECK_STEP(dberr, result, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getComponentInRange(u64 id_offset,
                                              u64 range,
                                              std::vector<ComponentAttrs> &comps)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_COMPONENT_IN_RANGE)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_COMPONENT_IN_RANGE), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :min
    dberr = sqlite3_bind_int(stmt, 1, (s64)id_offset);
    CHECK_BIND(dberr, result, db, end);

    // bind :max
    dberr = sqlite3_bind_int(stmt, 2, (s64)(id_offset + range - 1));
    CHECK_BIND(dberr, result, db, end);

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        ComponentAttrs comp;

        comp.id      = sqlite3_column_int64(stmt, 0);
        comp.name.assign((const char*)sqlite3_column_text(stmt, 1));
        comp.upname.assign((const char*)sqlite3_column_text(stmt, 2));
        comp.parent  = (u64)sqlite3_column_int64(stmt, 3);
        comp.type    = (DatasetDBComponentType)sqlite3_column_int(stmt, 4);
        comp.perm    = sqlite3_column_int(stmt, 5);
        comp.ctime   = (VPLTime_t)sqlite3_column_int64(stmt, 6);
        comp.mtime   = (VPLTime_t)sqlite3_column_int64(stmt, 7);
        comp.version = (u64)sqlite3_column_int64(stmt, 8);
        comp.origin_dev = (u64)sqlite3_column_int64(stmt, 9);
        comp.cur_rev = (u64)sqlite3_column_int64(stmt, 10);

        comps.push_back(comp);
    }
    CHECK_STEP(dberr, result, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getMaxComponentId(u64 &id)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();
     
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_MAX_COMPONENT_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(GET_MAX_COMPONENT_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    id = (u64)sqlite3_column_int64(stmt, 0);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::recoverMissingParentComponent(DatasetDBError missing_type,
                                                        const ComponentAttrs &child,
                                                        const ComponentAttrs &parent,
                                                        u64 num_missing_parents,
                                                        u64 maximum_components,
                                                        VPLTime_t dir_time,
                                                        u64 version, VPLTime_t time)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = DATASETDB_OK;
    std::string lost_and_found_path = normalizePath(DATASETDB_LOST_AND_FOUND_PATH);
    std::string date_path, orphaned_top_dir_path, orphaned_bottom_dir_path;
    std::stringstream ss;
    char date_dir[11];
    VPLTime_CalendarTime_t calendar_time;
    ComponentAttrs comp;
    u64 num_missing_parents_mod;

    db_lock();

    // Move the orphaned component to
    // "/lost+found/MM-DD-YYYY/orphaned-entries-<timestamp>/<#>"
    // The "<#>" directory is the replacement for the missing parent directory

    // Create the MM-DD-YYYY directory if needed
    VPLTime_ConvertToCalendarTimeLocal(dir_time, &calendar_time);
    snprintf(date_dir, sizeof(date_dir), "%02d-%02d-%04d",
             calendar_time.month,
             calendar_time.day,
             calendar_time.year);
    date_path = lost_and_found_path + "/" + date_dir;
    date_path = normalizePath(date_path);

    err = testAndCreateNonRootComponent(date_path, DATASETDB_COMPONENT_TYPE_DIRECTORY,
                                        0, version, time, NULL, NULL, NULL, true);
    CHECK_RV(err, result, end);

    // Create the orphaned-entries-<timestamp> directory if needed
    ss << dir_time;
    orphaned_top_dir_path = date_path + "/orphaned-entries-" + ss.str();
    orphaned_top_dir_path = normalizePath(orphaned_top_dir_path);

    err = testAndCreateNonRootComponent(orphaned_top_dir_path, DATASETDB_COMPONENT_TYPE_DIRECTORY,
                                        0, version, time, NULL, NULL, NULL, true);
    CHECK_RV(err, result, end);

    // Create a new "<#>" directory to replace the missing parent
    ss.str("");
    num_missing_parents_mod = num_missing_parents % maximum_components;
    ss << std::hex << std::setfill('0') << std::setw(4) << std::uppercase << num_missing_parents_mod;
    orphaned_bottom_dir_path = orphaned_top_dir_path + "/" + ss.str();
    orphaned_bottom_dir_path = normalizePath(orphaned_bottom_dir_path);

    err = testAndCreateNonRootComponent(orphaned_bottom_dir_path, DATASETDB_COMPONENT_TYPE_DIRECTORY,
                                        0, version, time, NULL, NULL, NULL, true);
    CHECK_RV(err, result, end);

    err = getComponentByPath(orphaned_bottom_dir_path, comp);
    CHECK_RV(err, result, end);

    if (missing_type == DATASETDB_ERR_UNKNOWN_COMPONENT) {
        // Old parent doesn't exist at all, so it's okay to re-use the same parent ID
        err = setComponentIdById(comp.id, child.parent);
        CHECK_RV(err, result, end);
    } else {
        // Old parent is of file component type, so need to update all children to
        // point to the new parent component
        err = setComponentParentByParentId(parent.id, comp.id);
        CHECK_RV(err, result, end);
    }

 end:
    db_unlock();

    return result;
}

// precondition: component exists
DatasetDBError DatasetDB::deleteComponentById(u64 id)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_COMPONENT_BY_ID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(DELETE_COMPONENT_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :id
    dberr = sqlite3_bind_int64(stmt, 1, (s64)id);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (sqlite3_changes(db) == 0) {  // no rows deleted -> component is missing
        result = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::deleteContentByCompId(u64 compid)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_CONTENT_BY_COMPID)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(DELETE_CONTENT_BY_COMPID), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :compid
    dberr = sqlite3_bind_int64(stmt, 1, (s64)compid);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);

    if (sqlite3_changes(db) == 0) {  // no rows deleted -> component is missing
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::listComponentsByParent(u64 parent,
                                                 std::vector<std::string> &names)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    names.clear();

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(LIST_COMPONENTS_BY_PARENT)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(LIST_COMPONENTS_BY_PARENT), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :parent
    dberr = sqlite3_bind_int64(stmt, 1, (s64)parent);
    CHECK_BIND(dberr, result, db, end);

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        names.push_back((const char*)sqlite3_column_text(stmt, 0));
    }
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();
    return result;
}

DatasetDBError DatasetDB::listComponentsInfoByParent(u64 parent,
                                                     std::vector<ComponentInfo> &components_info)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    components_info.clear();

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(LIST_COMPONENTS_INFO_BY_PARENT)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(LIST_COMPONENTS_INFO_BY_PARENT), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :parent
    dberr = sqlite3_bind_int64(stmt, 1, (s64)parent);
    CHECK_BIND(dberr, result, db, end);

    while ((dberr = sqlite3_step(stmt)) == SQLITE_ROW) {
        ComponentInfo info;
        const char * location;
        info.name.assign((const char*)sqlite3_column_text(stmt, 0));
        info.type    = (DatasetDBComponentType)sqlite3_column_int(stmt, 1);
        info.perm    = sqlite3_column_int(stmt, 2);
        info.ctime   = (VPLTime_t)sqlite3_column_int64(stmt, 3);
        info.mtime   = (VPLTime_t)sqlite3_column_int64(stmt, 4);
        info.version = (u64)sqlite3_column_int64(stmt, 5);
        location = (const char*)sqlite3_column_text(stmt, 6);
        if(location) {
            info.path.assign((const char*)sqlite3_column_text(stmt, 6));
        } else {
            info.path.clear();
        }
        if(info.type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
            info.size = 0;
        } else {
            info.size = sqlite3_column_int64(stmt, 7);
        }
        info.hash.clear();  // DEPRECATED
        info.metadata.clear();  // DEPRECATED 
        components_info.push_back(info);
    }
    CHECK_STEP(dberr, result, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();
    return result;
}

DatasetDBError DatasetDB::generateLocation(std::string &location)
{
    DatasetDBError result = DATASETDB_OK;
    DatasetDBError err = DATASETDB_OK;
    u64 key = 1;

    err = getVersion(VERID_NEXT_CONTPATH_KEY, key);
    if (err != DATASETDB_OK) {
        LOG_ERROR("Failed to get next contpath key: %d", err);
        result = err;
        goto end;
    }
    constructContentPathFromNum(key, location);
    err = setVersion(VERID_NEXT_CONTPATH_KEY, ++key);
    if (err != DATASETDB_OK) {
        LOG_ERROR("Failed to set next contpath key: %d", err);
        result = err;
        goto end;
    }

 end:
    return result;
}

// return the number of components (both non-trashed and trashed) that are referencing the given content path
// return DATASETDB_ERR_UNKNOWN_CONTENT if the given content path is not found in the content table
// this is to allow distinction between a content path that doesn't exist and a content node with no component referencing it.
DatasetDBError DatasetDB::getContentRefCount(const std::string &location,
                                             int &count)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COUNT_LOCATION)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(COUNT_LOCATION), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :location
    dberr = sqlite3_bind_text(stmt, 1, location.data(), location.size(), NULL);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_DONE) {
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

    count = sqlite3_column_int(stmt, 0);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

DatasetDBError DatasetDB::getChildComponentsCount(u64 parent,
                                                  int &count)
{
    DatasetDBError result = DATASETDB_OK;
    int dberr = 0;

    db_lock();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COUNT_CHILD_COMPS)];
    if (stmt == NULL) {
        dberr = sqlite3_prepare_v2(db, GETSQL(COUNT_CHILD_COMPS), -1, &stmt, NULL);
        CHECK_PREPARE(dberr, result, db, end);
    }

    // bind :parent
    dberr = sqlite3_bind_int64(stmt, 1, (s64)parent);
    CHECK_BIND(dberr, result, db, end);

    dberr = sqlite3_step(stmt);
    CHECK_STEP(dberr, result, db, end);
    if (dberr == SQLITE_DONE) {
        result = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

    count = sqlite3_column_int(stmt, 0);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    db_unlock();

    return result;
}

int DatasetDB::file_cp(const std::string& src,
                       const std::string& dst,
                       bool allow_interrupt,
                       bool& was_interrupted)
{
    VPLFile_handle_t ifd = VPLFILE_INVALID_HANDLE, ofd = VPLFILE_INVALID_HANDLE;
    ssize_t inchar, outchar;
    const ssize_t blksize = 1024 * 1024;
    char *buffer = NULL;
    int rv = VPL_OK;
    
    buffer = (char *)malloc(blksize);
    ifd = VPLFile_Open(src.c_str(), VPLFILE_OPENFLAG_READONLY,
        VPLFILE_MODE_IRUSR);
    if (!VPLFile_IsValidHandle(ifd)) {
        rv = ifd;
        goto done;
    }
    ofd = VPLFile_Open(dst.c_str(),
        VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE,
        VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
    if (!VPLFile_IsValidHandle(ofd)) {
        rv = ofd;
        goto done;
    }

    while( (inchar = VPLFile_Read(ifd, buffer, blksize)) > 0) {
        outchar = VPLFile_Write(ofd, buffer, inchar);
        if (outchar != inchar) {
            rv = VPL_ERR_IO;
            goto done;
        }
        if ( (allow_interrupt && (client_cnt != 1)) || force_backup_stop ) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "file_cp: was interrupted: "
                              "force: %s allow: %s clients %d",
                              force_backup_stop ? "true" : "false",
                              allow_interrupt ? "true" : "false",
                              client_cnt);
            was_interrupted = true;
            rv = VPL_OK;
            goto done;
        }
    }

    if ( inchar < 0 ) {
        rv = inchar;
    }

done:
    if ( VPLFile_IsValidHandle(ifd) ) {
        VPLFile_Close(ifd);
    }

    if ( VPLFile_IsValidHandle(ofd) ) {
        VPLFile_Close(ofd);
    }

    if ( buffer ) {
        free(buffer);
    }

    return rv;
}

// The following is based on code defined at the following link
// and modified for our environment.
// http://www.sqlite.org/backup.html
DatasetDBError DatasetDB::Backup(bool& was_interrupted, VPLTime_t& end_time)
{
    int dberr = 0;
    DatasetDBError result = DATASETDB_OK;
    sqlite3 *pFile;             /* Database connection for new backup db */
    sqlite3_backup *pBackup;    /* Backup handle used to copy data */
    std::string cur_dbpath;
    std::string backup_file;
    u32 cur_options = 0;
    bool was_successful = false;

    was_interrupted = false;

    db_lock();

    if ( db == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "db not open");
        result = DATASETDB_ERR_NOT_OPEN;
        goto end;
    }

    // Save these so they're not lost
    cur_dbpath = dbpath;
    cur_options = options;

    // Create a backup
    backup_file = cur_dbpath + "-tmp";
    (void)VPLFile_Delete(backup_file.c_str());

    /* Open the new backup database file */
    dberr = sqlite3_open(backup_file.c_str(), &pFile);
    if ( dberr != SQLITE_OK ) {
        LOG_ERROR("sqlite3_open(%s): %d", backup_file.c_str(), dberr);
        result = mapSqliteErrCode(dberr);
        goto end;
    }

    /* Open the sqlite3_backup object used to accomplish the transfer */
    pBackup = sqlite3_backup_init(pFile, "main", db, "main");
    if( pBackup == NULL) {
        dberr = sqlite3_errcode(pFile);
        LOG_ERROR("sqlite3_backup_init(): %d", dberr);
        result = mapSqliteErrCode(dberr);
        goto end;
    }

    /* Each iteration of this loop copies 5 database pages from database
    ** pDb to the backup database. If the return value of backup_step()
    ** indicates that there are still further pages to copy, sleep for
    ** 60 seconds before repeating. */
    do {
#ifdef NOTDEF
        // We don't want this in the loop
        if ( client_cnt != 1 ) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "backup delayed, cnt %d",
                client_cnt);
        }
#endif // NOTDEF
        while ((client_cnt != 1) && !force_backup_stop) {
            // This will drop the mutex to let the waiting client
            // get in. It will also hold off the backup for a minute
            // or so to have less impact on the client. Since it's
            // a cond_timedwait() we can wake up out of this for shutdown
            // very quickly.
            VPLCond_TimedWait(&backup_cond, &mutex, BACKUP_DELAY);
        }

        if ( force_backup_stop ) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Backup forced to stop");
            was_interrupted = true;
            break;
        }

        dberr = sqlite3_backup_step(pBackup, 5);
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "pages %d of %d remaining", 
            sqlite3_backup_remaining(pBackup),
            sqlite3_backup_pagecount(pBackup));
    } while( dberr==SQLITE_OK || dberr==SQLITE_BUSY || dberr==SQLITE_LOCKED );

    /* Release resources allocated by backup_init(). */
    (void)sqlite3_backup_finish(pBackup);

#ifdef NOTDEF
    // The example shows this step being done but it appears to have
    // undefined results the way its being used?
    // was the backup successful?
    if ( !was_interrupted && (dberr == SQLITE_DONE) ) {
        dberr = sqlite3_errcode(pFile);
    }
#endif // NOTDEF

    // If we were interrupted or failed, do clean up
    if ( was_interrupted || (dberr != SQLITE_DONE) ) {
        // TODO: dberr != SQLITE_DONE should be used to decide to
        // revert to an older database. But, that would be pushed up
        // to a higher level to decide.
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "backup failed. dberr %d stop %s",
            dberr, was_interrupted ? "true" : "false");
        (void)VPLFile_Delete(backup_file.c_str());
        if ( !was_interrupted ) {
            result = mapSqliteErrCode(dberr);
        }
        goto end_close;
    }

    was_successful = true;
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "backup was successful.");

 end_close:
    /* Close the database connection opened on database file zFilename
    ** and return the result of this function. */
    (void)sqlite3_close(pFile);

    if ( !was_successful ) {
        goto end;
    }

    // rename / delete older backups
    for( int i = MAX_BACKUP_COPY ; i > 0 ; i-- ) {
        std::string sname;
        std::string dname;
        
        {
            std::stringstream name_tmp;
            // This scoping forces name_tmp to get cleared.
            name_tmp << i;
            sname = cur_dbpath + "-" + name_tmp.str();
        }
        if ( i == MAX_BACKUP_COPY ) {
            (void)VPLFile_Delete(sname.c_str());
        }
        else {
            std::stringstream name_tmp;

            name_tmp << i + 1;
            dname = cur_dbpath + "-" + name_tmp.str();
            if ( VPLFile_CheckAccess(sname.c_str(),
                                     VPLFILE_CHECKACCESS_READ) == 0 ) {
                int rv;
                if ( (rv = VPLFile_Rename(sname.c_str(), dname.c_str())) ) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
                        sname.c_str(), dname.c_str(), rv);
                }
            }
        }

        // finally, move the new backup into position
        if ( i == 1 ) {
            int rv;
            if ( (rv = VPLFile_Rename(backup_file.c_str(), sname.c_str())) ) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
                    backup_file.c_str(), sname.c_str(), rv);
                continue;
            }
            // just to be safe
            break;
        }
    }

 end:
    end_time = VPLTime_GetTimeStamp();
    db_unlock();
    return result;
}

// delete the existing backup (if it exists) and move a backup copy
// into its place.
DatasetDBError DatasetDB::Restore(const std::string& dbpath, bool& had_backup)
{
    int rv = DATASETDB_OK;
    std::string sname;
    std::string dname;

    had_backup = false;

    db_lock();
    if ( db != NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "db is open");
        rv = DATASETDB_ERR_BUSY;
        goto done;
    }

    // first remove the existing bad database.
    VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Deleting corrupted database %s.",
        dbpath.c_str());
    (void)VPLFile_Delete(dbpath.c_str());

    dname = dbpath;
    // Now shuffle the backups into place.
    for( int i = 1 ; i <= MAX_BACKUP_COPY ; i++ ) {
        std::stringstream name_tmp;

        name_tmp << i;
        sname = dbpath + "-" + name_tmp.str();
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "attempting mv %s %s. (%d)",
            sname.c_str(), dname.c_str(), i);
        if ( VPLFile_CheckAccess(sname.c_str(),
                                 VPLFILE_CHECKACCESS_READ) == 0 ) {
            if ( (rv = VPLFile_Rename(sname.c_str(), dname.c_str())) ) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
                    sname.c_str(), dname.c_str(), rv);
                // What can you do here? honestly?
                break;
            }
            had_backup = true;
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Backup db %s not found",
                sname.c_str());
            break;
        }
        dname = sname;
    }

done:
    db_unlock();

    return rv;
}

void DatasetDB::db_lock(void)
{
    // Note: we need to increment before grabbing the lock to
    // indicate we're waiting for database access so that
    // backup can release the lock.

    // Would have used VPL_ATOMIC_INC_UNSIGNED except that
    // we don't have support for atomic inc/dec under ARM.
    VPLMutex_Lock(&cnt_mutex);
    client_cnt++;
    VPLMutex_Unlock(&cnt_mutex);

    VPLMutex_Lock(&mutex);
}

void DatasetDB::db_unlock(void)
{
    VPLMutex_Unlock(&mutex);

    // Would have used VPL_ATOMIC_DEC_UNSIGNED except that
    // we don't have support for atomic inc/dec under ARM.
    VPLMutex_Lock(&cnt_mutex);
    client_cnt--;
    VPLMutex_Unlock(&cnt_mutex);
}

// This routine takes swaps in the freshly backed up
// version of the database and swaps it for the live
// database.
// Steps:
//   * Close Current database
//   * rename files
//   * Open New Database
// Issues:
//   * Handling a crash during the middle of this...
//   * This adds additional work on the OpenDB.
DatasetDBError DatasetDB::SwapInBackup(void)
{
    DatasetDBError result = DATASETDB_OK;
    int rv = 0;
    bool db_is_open = true;
    bool close_failed = false;
    u32 cur_options = 0;
    std::string curdb_path;
    std::string backup_path;
    std::string swap_path;

    db_lock();

    // Save these so they're not lost
    curdb_path = dbpath;
    cur_options = options;
    backup_path = dbpath + "-1";
    swap_path = dbpath + "-swap";
    
    // Takes advantage of mutex allowing nested calls by same thread.
    result = CloseDB();
    if ( result != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "CloseDB() returned %d", result);
        // If close failed then things are really messed up. Continue with
        // the swap and trigger a restart. When we restart we'll open the
        // new copy.
        close_failed = true;
    }
    db_is_open = false;

    // Move the current backup to db-swap
    if ( (rv = VPLFile_Rename(backup_path.c_str(), swap_path.c_str())) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
            backup_path.c_str(), swap_path.c_str(), rv);
        goto done;
    }

    // Move the current db to the backup
    if ( (rv = VPLFile_Rename(curdb_path.c_str(), backup_path.c_str())) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
            curdb_path.c_str(), backup_path.c_str(), rv);
        goto done;
    }

    // Move the swap to the curdb location
    if ( (rv = VPLFile_Rename(swap_path.c_str(), curdb_path.c_str())) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
            swap_path.c_str(), curdb_path.c_str(), rv);
        goto done;
    }

    // reopen this thing...
    if ( close_failed ) {
        goto done;
    }

    result = OpenDB(curdb_path, cur_options);
    if ( result != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "OpenDB() - %d", result);
        goto done;
    }
    db_is_open = true;

done:
    if ( close_failed || !db_is_open ) {
        result = DATASETDB_ERR_RESTART;
    }
    else {
        result = DATASETDB_OK;
    }
    db_unlock();

    return result;
}

// This function tries to restore order if we crashed in the middle of a
// SwapInBackup() operation.
void DatasetDB::swapInBackupRecover(void)
{
    int rv = 0;
    std::string curdb_path;
    std::string backup_path;
    std::string swap_path;

    // Save these so they're not lost
    curdb_path = dbpath;
    backup_path = dbpath + "-1";
    swap_path = dbpath + "-swap";
    
    // If the swap file doesn't exist then there's nothing to do.
    if ( VPLFile_CheckAccess(swap_path.c_str(),
                             VPLFILE_CHECKACCESS_READ) != 0 ) {
        goto done;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "backup swap file found.");
    if ( VPLFile_CheckAccess(curdb_path.c_str(),
                             VPLFILE_CHECKACCESS_READ) == 0 ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Moving curdb to backup");

        // Move the current db to the backup
        if ( (rv = VPLFile_Rename(curdb_path.c_str(), backup_path.c_str())) ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
                curdb_path.c_str(), backup_path.c_str(), rv);
            goto fail;
        }
    }

    // Move the swap to the curdb location
    if ( (rv = VPLFile_Rename(swap_path.c_str(), curdb_path.c_str())) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
            swap_path.c_str(), curdb_path.c_str(), rv);
        goto fail;
    }
    goto done;

fail:
    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unfolding the swap");
    // Things are in a bad way...
    if ((rv = VPLFile_Rename(swap_path.c_str(), backup_path.c_str()))) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
            swap_path.c_str(), backup_path.c_str(), rv);
        // Just remove the swap file at this point.
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Removing the swap file.");
        (void)VPLFile_Delete(swap_path.c_str());
    }

done:
    return;
}

void DatasetDB::BackupCancel(void)
{
    db_lock();
    force_backup_stop = true;
    VPLCond_Signal(&backup_cond);
    db_unlock();
}
