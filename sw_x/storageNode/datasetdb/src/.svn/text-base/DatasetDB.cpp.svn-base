#include "DatasetDB.hpp"

#include "vplex_trace.h"
#include "vplex_file.h"
#include "utf8.hpp"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

// Target DB schema version.
// Anything less will be upgraded to this version by OpenDB().
#define TARGET_SCHEMA_VERSION 4

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

#define CHECK_BIND(rv, rc, db, lbl)					\
if (isSqliteError(rv)) {						\
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to bind value in prepared stmt: %d (%d), %s", rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db)); \
    goto lbl;								\
}

#define CHECK_STEP(rv, rc, db, lbl)					\
if (isSqliteError(rv)) {						\
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to execute prepared stmt: %d (%d), %s", rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db)); \
    goto lbl;								\
}

#define CHECK_FINALIZE(rv, rc, db, lbl)					\
if (isSqliteError(rv)) {						\
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to finalize prepared stmt: %d (%d), %s", rc, sqlite3_extended_errcode(db), sqlite3_errmsg(db)); \
    goto lbl;								\
}

#define CHECK_RV(rv, rc, lbl)                   \
if (rv != DATASETDB_OK) {                       \
    rc = rv;                                    \
    goto lbl;                                   \
}

#define MAX_BACKUP_COPY             1

enum VersionId {
    VERSION_ID_FULL_VERSION = 1,
    VERSION_ID_MERGE_VERSION,
    VERSION_ID_SCHEMA_VERSION,
    VERSION_ID_RESTORE_VERSION,
};

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
"INSERT OR IGNORE INTO version VALUES (1, 0);"  // ID 1 = VERSION_ID_FULL_VERSION
"INSERT OR IGNORE INTO version VALUES (2, 0);"  // ID 2 = VERSION_ID_MERGE_VERSION
"INSERT OR IGNORE INTO version VALUES (3, "XSTR(TARGET_SCHEMA_VERSION)");"  // ID 3 = VERSION_ID_SCHEMA_VERSION
"INSERT OR IGNORE INTO version VALUES (4, 0);"  // ID 4 = VERSION_ID_RESTORE_VERSION
"CREATE TABLE IF NOT EXISTS component ("
    "id INTEGER PRIMARY KEY, "
    "name TEXT NOT NULL, "
    "content_id INTEGER, "
    "ctime INTEGER NOT NULL, "
    "mtime INTEGER NOT NULL, "
    "trash_id INTEGER, "
    "type INTEGER NOT NULL, "
    "version INTEGER NOT NULL, "
    "perm INTEGER NOT NULL, "
    "UNIQUE (name, trash_id), "
    "FOREIGN KEY (trash_id) REFERENCES trash (id) ON DELETE CASCADE, "
    "FOREIGN KEY (content_id) REFERENCES content (id));"
"CREATE TABLE IF NOT EXISTS metadata ("
    "id INTEGER PRIMARY KEY, "
    "component_id INTEGER NOT NULL, "
    "type INTEGER NOT NULL, "
    "value BLOB, "
    "UNIQUE (component_id, type), "
    "FOREIGN KEY (component_id) REFERENCES component (id) ON DELETE CASCADE);"
"CREATE TABLE IF NOT EXISTS content ("
    "id INTEGER PRIMARY KEY, "
    "path TEXT UNIQUE, "
    "size INTEGER, "
    "hash BLOB, "
    "mtime INTEGER, "
    "restore_id INTEGER);"
"CREATE TABLE IF NOT EXISTS trash ("
    "id INTEGER PRIMARY KEY, "
    "version INTEGER NOT NULL, "
    "idx INTEGER NOT NULL, "
    "dtime INTEGER NOT NULL, "
    "component_id INTEGER NOT NULL, "
    "size INTEGER NOT NULL, "
    "UNIQUE (version, idx));"
"CREATE INDEX IF NOT EXISTS i_comp_content_id on component (content_id);"
"COMMIT;";

static const char test_and_create_db_sql_nocase[] = 
"BEGIN;"
"CREATE TABLE IF NOT EXISTS version ("
    "id INTEGER PRIMARY KEY, "
    "value INTEGER NOT NULL);"
"INSERT OR IGNORE INTO version VALUES (1, 0);"  // ID 1 = VERSION_ID_FULL_VERSION
"INSERT OR IGNORE INTO version VALUES (2, 0);"  // ID 2 = VERSION_ID_MERGE_VERSION
"INSERT OR IGNORE INTO version VALUES (3, "XSTR(TARGET_SCHEMA_VERSION)");"  // ID 3 = VERSION_ID_SCHEMA_VERSION
"INSERT OR IGNORE INTO version VALUES (4, 0);"  // ID 4 = VERSION_ID_RESTORE_VERSION
"CREATE TABLE IF NOT EXISTS component ("
    "id INTEGER PRIMARY KEY, "
    "name TEXT NOT NULL, "
    "name_upper TEXT NOT NULL, "
    "content_id INTEGER, "
    "ctime INTEGER NOT NULL, "
    "mtime INTEGER NOT NULL, "
    "trash_id INTEGER, "
    "type INTEGER NOT NULL, "
    "version INTEGER NOT NULL, "
    "perm INTEGER NOT NULL, "
    "UNIQUE (name_upper, trash_id), "
    "FOREIGN KEY (trash_id) REFERENCES trash (id) ON DELETE CASCADE, "
    "FOREIGN KEY (content_id) REFERENCES content (id));"
"CREATE TABLE IF NOT EXISTS metadata ("
    "id INTEGER PRIMARY KEY, "
    "component_id INTEGER NOT NULL, "
    "type INTEGER NOT NULL, "
    "value BLOB, "
    "UNIQUE (component_id, type), "
    "FOREIGN KEY (component_id) REFERENCES component (id) ON DELETE CASCADE);"
"CREATE TABLE IF NOT EXISTS content ("
    "id INTEGER PRIMARY KEY, "
    "path TEXT UNIQUE, "
    "size INTEGER, "
    "hash BLOB, "
    "mtime INTEGER, "
    "restore_id INTEGER);"
"CREATE TABLE IF NOT EXISTS trash ("
    "id INTEGER PRIMARY KEY, "
    "version INTEGER NOT NULL, "
    "idx INTEGER NOT NULL, "
    "dtime INTEGER NOT NULL, "
    "component_id INTEGER NOT NULL, "
    "size INTEGER NOT NULL, "
    "UNIQUE (version, idx));"
"CREATE INDEX IF NOT EXISTS i_comp_content_id on component (content_id);"
"COMMIT;";


#define DEFSQL(tag, def) static const char sql_ ## tag [] = def
#define SQLNUM(tag) DATASETDB_STMT_ ## tag
#define GETSQL(tag) sql_ ## tag

enum {
    DATASETDB_STMT_BEGIN,
    DATASETDB_STMT_COMMIT,
    DATASETDB_STMT_ROLLBACK,
    DATASETDB_STMT_GET_DATASET_VERSION,
    DATASETDB_STMT_SET_DATASET_VERSION,
    DATASETDB_STMT_CREATE_COMPONENT,
    DATASETDB_STMT_CREATE_COMPONENT_NOCASE,
    DATASETDB_STMT_CREATE_TRASH_COMPONENT,
    DATASETDB_STMT_CREATE_TRASH_COMPONENT_NOCASE,
    DATASETDB_STMT_TEST_COMPONENT_BY_NAME,
    DATASETDB_STMT_TEST_COMPONENT_BY_NAME_NOCASE,
    DATASETDB_STMT_GET_COMPONENT_BY_NAME,
    DATASETDB_STMT_GET_COMPONENT_BY_NAME_NOCASE,
    DATASETDB_STMT_GET_COMPONENT_BY_NAME_TRASH_ID,
    DATASETDB_STMT_GET_COMPONENT_BY_NAME_TRASH_ID_NOCASE,
    DATASETDB_STMT_GET_ROOT_COMPONENT_BY_TRASH_ID,
    DATASETDB_STMT_GET_ROOT_COMPONENT_BY_TRASH_ID_NOCASE,
    DATASETDB_STMT_GET_COMPONENT_BY_VERSION_INDEX_NAME,
    DATASETDB_STMT_GET_COMPONENT_BY_VERSION_INDEX_NAME_NOCASE,
    DATASETDB_STMT_GET_COMPONENT_BY_ID,
    DATASETDB_STMT_GET_COMPONENTS_TOTAL_SIZE,
    DATASETDB_STMT_GET_COMPONENTS_TOTAL_SIZE_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_CONTENT_ID_BY_NAME,
    DATASETDB_STMT_SET_COMPONENT_CONTENT_ID_BY_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME,
    DATASETDB_STMT_SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_CTIME_BY_NAME,
    DATASETDB_STMT_SET_COMPONENT_CTIME_BY_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_MTIME_BY_NAME,
    DATASETDB_STMT_SET_COMPONENT_MTIME_BY_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_TRASH_ID_BY_NAME,
    DATASETDB_STMT_SET_COMPONENT_TRASH_ID_BY_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_TRASH_ID_BY_TRASH_ID,
    DATASETDB_STMT_SET_COMPONENT_PERM_BY_NAME,
    DATASETDB_STMT_SET_COMPONENT_PERM_BY_NAME_NOCASE,
    DATASETDB_STMT_CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID,
    DATASETDB_STMT_CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID_NOCASE,
    DATASETDB_STMT_CHANGE_COMPONENT_NAME_PREFIX_BY_NAME,
    DATASETDB_STMT_CHANGE_COMPONENT_NAME_PREFIX_BY_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_VERSION_TIME_BY_TRASH_ID,
    DATASETDB_STMT_COPY_COMPONENTS_BY_NAME,
    DATASETDB_STMT_COPY_COMPONENTS_BY_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_VERSION_BY_NAME,
    DATASETDB_STMT_SET_COMPONENT_VERSION_BY_NAME_NOCASE,
    DATASETDB_STMT_SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME,
    DATASETDB_STMT_SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME_NOCASE,
    DATASETDB_STMT_DELETE_COMPONENT_BY_NAME,
    DATASETDB_STMT_DELETE_COMPONENT_BY_NAME_NOCASE,
    DATASETDB_STMT_DELETE_ALL_COMPONENTS,
    DATASETDB_STMT_DELETE_COMPONENTS_UNMODIFIED_SINCE,
    DATASETDB_STMT_DELETE_COMPONENT_FILES_WITHOUT_CONTENT,
    DATASETDB_STMT_DELETE_COMPONENTS_BY_CONTENT_ID,
    DATASETDB_STMT_LIST_COMPONENTS_BY_NAME,
    DATASETDB_STMT_LIST_COMPONENTS_BY_NAME_NOCASE,
    DATASETDB_STMT_LIST_COMPONENTS_BY_NAME_TRASH_ID,
    DATASETDB_STMT_LIST_COMPONENTS_BY_NAME_TRASH_ID_NOCASE,
    DATASETDB_STMT_GET_NEW_CONTENT,
    DATASETDB_STMT_GET_NEXT_CONTENT,
    DATASETDB_STMT_SET_CONTENT_PATH_BY_ID,
    DATASETDB_STMT_GET_CONTENT_BY_ID,
    DATASETDB_STMT_GET_CONTENT_BY_PATH,
    DATASETDB_STMT_GET_CONTENT_BY_HASH_SIZE,
    DATASETDB_STMT_GET_CONTENT_ID_BY_PATH,
    DATASETDB_STMT_GET_CONTENT_REFCOUNT,
    DATASETDB_STMT_SET_CONTENT_SIZE_BY_ID,
    DATASETDB_STMT_SET_CONTENT_SIZE_IF_LARGER_BY_ID,
    DATASETDB_STMT_SET_CONTENT_SIZE_BY_PATH,
    DATASETDB_STMT_SET_CONTENT_HASH_BY_ID,
    DATASETDB_STMT_SET_CONTENT_HASH_BY_PATH,
    DATASETDB_STMT_SET_CONTENT_RESTORE_ID_BY_PATH,
    DATASETDB_STMT_SET_CONTENT_MTIME_BY_PATH,
    DATASETDB_STMT_DELETE_ALL_CONTENTS,
    DATASETDB_STMT_DELETE_CONTENT_BY_PATH,
    DATASETDB_STMT_CREATE_METADATA,
    DATASETDB_STMT_GET_METADATA_BY_COMPONENT_ID_TYPE,
    DATASETDB_STMT_GET_METADATA_BY_COMPONENT_ID,
    DATASETDB_STMT_SET_METADATA_BY_ID,
    DATASETDB_STMT_COPY_METADATA_BY_NAME,
    DATASETDB_STMT_COPY_METADATA_BY_NAME_NOCASE,
    DATASETDB_STMT_DELETE_METADATA,
    DATASETDB_STMT_CREATE_TRASH,
    DATASETDB_STMT_GET_TRASH_BY_VERSION_INDEX,
    DATASETDB_STMT_DELETE_TRASH_BY_ID,
    DATASETDB_STMT_DELETE_ALL_TRASH,
    DATASETDB_STMT_LIST_TRASH,
    DATASETDB_STMT_LIST_UNREF_CONTENTS,
    DATASETDB_STMT_MAX,  // this must be last
};

DEFSQL(BEGIN,                                   \
       "BEGIN");
DEFSQL(COMMIT,                                  \
       "COMMIT");
DEFSQL(ROLLBACK,                                \
       "ROLLBACK");
DEFSQL(GET_DATASET_VERSION,                     \
       "SELECT value "                          \
       "FROM version "                          \
       "WHERE id=:id");
DEFSQL(SET_DATASET_VERSION,                     \
       "UPDATE version "                        \
       "SET value=:value "                      \
       "WHERE id=:id");
DEFSQL(CREATE_COMPONENT,                                                \
       "INSERT INTO component (name, ctime, mtime, type, version, perm) " \
       "VALUES (:name, :time, :time, :type, :version, :perm)");
DEFSQL(CREATE_COMPONENT_NOCASE,                                         \
       "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) " \
       "VALUES (:name, :name_upper, :time, :time, :type, :version, :perm)");
DEFSQL(CREATE_TRASH_COMPONENT,                                          \
       "INSERT INTO component (trash_id, name, ctime, mtime, type, version) " \
       "VALUES ((SELECT id FROM trash WHERE version=:trash_version AND idx=:idx), :name, :time, :time, :type, :version)");
DEFSQL(CREATE_TRASH_COMPONENT_NOCASE,                                   \
       "INSERT INTO component (trash_id, name, name_upper, ctime, mtime, type, version) " \
       "VALUES ((SELECT id FROM trash WHERE version=:trash_version AND idx=:idx), :name, :name_upper, :time, :time, :type, :version)");
DEFSQL(TEST_COMPONENT_BY_NAME,                          \
       "SELECT 1 "                                      \
       "FROM component "                                \
       "WHERE name=:name AND trash_id IS NULL");
DEFSQL(TEST_COMPONENT_BY_NAME_NOCASE,                   \
       "SELECT 1 "                                      \
       "FROM component "                                \
       "WHERE name_upper=:name_upper AND trash_id IS NULL");
DEFSQL(GET_COMPONENT_BY_NAME,                                           \
       "SELECT id, name, content_id, ctime, mtime, type, version, trash_id, perm " \
       "FROM component "                                                \
       "WHERE name=:name AND trash_id IS NULL");
DEFSQL(GET_COMPONENT_BY_NAME_NOCASE,                                    \
       "SELECT id, name, content_id, ctime, mtime, type, version, trash_id, perm " \
       "FROM component "                                                \
       "WHERE name_upper=:name_upper AND trash_id IS NULL");
DEFSQL(GET_COMPONENT_BY_NAME_TRASH_ID,                                  \
       "SELECT id, name, content_id, ctime, mtime, type, version, trash_id, perm " \
       "FROM component "                                                \
       "WHERE name=:name AND trash_id=:trash_id");
DEFSQL(GET_COMPONENT_BY_NAME_TRASH_ID_NOCASE,                           \
       "SELECT id, name, content_id, ctime, mtime, type, version, trash_id, perm " \
       "FROM component "                                                \
       "WHERE name_upper=:name_upper AND trash_id=:trash_id");
DEFSQL(GET_ROOT_COMPONENT_BY_TRASH_ID,                                  \
       "SELECT id, name, content_id, ctime, mtime, type, version, trash_id, perm " \
       "FROM component "                                                \
       "WHERE trash_id=:trash_id ORDER BY length(name) LIMIT 1");
DEFSQL(GET_ROOT_COMPONENT_BY_TRASH_ID_NOCASE,                           \
       "SELECT id, name, content_id, ctime, mtime, type, version, trash_id, perm " \
       "FROM component "                                                \
       "WHERE trash_id=:trash_id ORDER BY length(name_upper) LIMIT 1");
DEFSQL(GET_COMPONENT_BY_VERSION_INDEX_NAME,                             \
       "SELECT c.id, c.name, c.content_id, c.ctime, c.mtime, c.type, c.version, c.trash_id, perm " \
       "FROM component c, trash t "                                     \
       "WHERE c.name=:name AND c.trash_id =t.id AND t.version=:version AND t.idx=:idx");
DEFSQL(GET_COMPONENT_BY_VERSION_INDEX_NAME_NOCASE,                      \
       "SELECT c.id, c.name, c.content_id, c.ctime, c.mtime, c.type, c.version, c.trash_id, perm " \
       "FROM component c, trash t "                                     \
       "WHERE c.name_upper=:name_upper AND c.trash_id =t.id AND t.version=:version AND t.idx=:idx");
DEFSQL(GET_COMPONENT_BY_ID,                                             \
       "SELECT id, name, content_id, ctime, mtime, type, version, trash_id, perm " \
       "FROM component "                                                \
       "WHERE id=:id");
DEFSQL(GET_COMPONENTS_TOTAL_SIZE,                                       \
       "SELECT sum(s.size) "                                            \
       "FROM component c, content s "                                   \
       "WHERE (c.name=:name OR c.name GLOB :name||'*') AND (c.trash_id IS NULL) AND (s.id=c.content_id)");
DEFSQL(GET_COMPONENTS_TOTAL_SIZE_NOCASE,                                \
       "SELECT sum(s.size) "                                            \
       "FROM component c, content s "                                   \
       "WHERE (c.name_upper=:name_upper OR c.name_upper GLOB :name_upper||'*') AND (c.trash_id IS NULL) AND (s.id=c.content_id)");
DEFSQL(SET_COMPONENT_CONTENT_ID_BY_NAME,                \
       "UPDATE component "                              \
       "SET content_id=:sid "                           \
       "WHERE name=:name AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_CONTENT_ID_BY_NAME_NOCASE,         \
       "UPDATE component "                              \
       "SET content_id=:sid "                           \
       "WHERE name_upper=:name_upper AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME,  \
       "UPDATE component "                              \
       "SET content_id=:sid "                           \
       "WHERE name=:name AND trash_id IN (SELECT id FROM trash WHERE version=:version AND idx=:idx)");
DEFSQL(SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME_NOCASE, \
       "UPDATE component "                              \
       "SET content_id=:sid "                           \
       "WHERE name_upper=:name_upper AND trash_id IN (SELECT id FROM trash WHERE version=:version AND idx=:idx)");
DEFSQL(SET_COMPONENT_CTIME_BY_NAME,                     \
       "UPDATE component "                              \
       "SET ctime=:ctime "                              \
       "WHERE name=:name AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_CTIME_BY_NAME_NOCASE,              \
       "UPDATE component "                              \
       "SET ctime=:ctime "                              \
       "WHERE name_upper=:name_upper AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_MTIME_BY_NAME,                     \
       "UPDATE component "                              \
       "SET mtime=:mtime "                              \
       "WHERE name=:name AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_MTIME_BY_NAME_NOCASE,              \
       "UPDATE component "                              \
       "SET mtime=:mtime "                              \
       "WHERE name_upper=:name_upper AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_TRASH_ID_BY_NAME,                                  \
       "UPDATE component "                                              \
       "SET trash_id=:tid "                                             \
       "WHERE (name=:name OR name GLOB :glob) AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_TRASH_ID_BY_NAME_NOCASE,                           \
       "UPDATE component "                                              \
       "SET trash_id=:tid "                                             \
       "WHERE (name_upper=:name_upper OR name_upper GLOB :glob_upper) AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_TRASH_ID_BY_TRASH_ID,      \
       "UPDATE component "                      \
       "SET trash_id=:new, mtime=:now "         \
       "WHERE trash_id=:old");
DEFSQL(SET_COMPONENT_PERM_BY_NAME,                      \
       "UPDATE component "                              \
       "SET perm=:perm "                                \
       "WHERE name=:name AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_PERM_BY_NAME_NOCASE,                       \
       "UPDATE component "                                      \
       "SET perm=:perm "                                        \
       "WHERE name_upper=:name_upper AND trash_id IS NULL");
DEFSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID,        \
       "UPDATE component "                              \
       "SET name=replaceprefix(name,:old,:new) "        \
       "WHERE trash_id=:tid");
DEFSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID_NOCASE,                 \
       "UPDATE component "                                              \
       "SET name=replaceprefix_nocase(name,:old,:new), "                \
       "name_upper=utf8_upper(replaceprefix_nocase(name_upper,:old,:new)) " \
       "WHERE trash_id=:tid");
DEFSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_NAME,                            \
       "UPDATE component "                                              \
       "SET name=replaceprefix(name,:old,:new), version=:version "      \
       "WHERE (name=:old OR name GLOB :old||'/*') AND trash_id IS NULL");
DEFSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_NAME_NOCASE,                     \
       "UPDATE component "                                              \
       "SET name=replaceprefix_nocase(name,:old_upper,:new), version=:version, " \
       "name_upper=utf8_upper(replaceprefix_nocase(name_upper,:old_upper,:new)) " \
       "WHERE (name_upper=:old_upper OR name_upper GLOB :old_upper||'/*') AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_VERSION_TIME_BY_TRASH_ID,  \
       "UPDATE component "                      \
       "SET version=:version, mtime=:now "      \
       "WHERE trash_id=:tid");
DEFSQL(COPY_COMPONENTS_BY_NAME,                                         \
       "INSERT INTO component (name, content_id, ctime, mtime, type, version, perm) " \
       "SELECT replaceprefix(name, :old, :new), content_id, ctime, mtime, type, :version, perm " \
       "FROM component "                                                \
       "WHERE (name=:old OR name GLOB :old||'/*') AND trash_id IS NULL");
DEFSQL(COPY_COMPONENTS_BY_NAME_NOCASE,                                  \
       "INSERT INTO component (name, name_upper, content_id, ctime, mtime, type, version, perm) " \
       "SELECT replaceprefix_nocase(name, :old_upper, :new), "          \
       "utf8_upper(replaceprefix_nocase(name_upper, :old_upper, :new)), " \
       "content_id, ctime, mtime, type, :version, perm "                \
       "FROM component "                                                \
       "WHERE (name_upper=:old_upper OR name_upper GLOB :old_upper||'/*') AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_VERSION_BY_NAME,                   \
       "UPDATE component "                              \
       "SET version=:version "                          \
       "WHERE name=:name AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_VERSION_BY_NAME_NOCASE,            \
       "UPDATE component "                              \
       "SET version=:version "                          \
       "WHERE name_upper=:name_upper AND trash_id IS NULL");
DEFSQL(SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME,              \
       "UPDATE component "                                            \
       "SET version=:version, mtime=:mtime "                          \
       "WHERE name=:name AND trash_id IN (SELECT id FROM trash WHERE version=:trash_version AND idx=:idx)");
DEFSQL(SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME_NOCASE,       \
       "UPDATE component "                                            \
       "SET version=:version, mtime=:mtime "                          \
       "WHERE name_upper=:name_upper AND trash_id IN (SELECT id FROM trash WHERE version=:trash_version AND idx=:idx)");
DEFSQL(DELETE_COMPONENT_BY_NAME,                                        \
       "DELETE FROM component "                                         \
       "WHERE (name=:name OR name GLOB :glob) AND trash_id IS NULL");
DEFSQL(DELETE_COMPONENT_BY_NAME_NOCASE,                                 \
       "DELETE FROM component "                                         \
       "WHERE (name_upper=:name_upper OR name_upper GLOB :glob_upper) AND trash_id IS NULL");
DEFSQL(DELETE_ALL_COMPONENTS,                   \
       "DELETE FROM component");
DEFSQL(DELETE_COMPONENTS_UNMODIFIED_SINCE,              \
       "DELETE FROM component "                         \
       "WHERE mtime<:time AND trash_id IS NULL");
DEFSQL(DELETE_COMPONENT_FILES_WITHOUT_CONTENT, \
       "DELETE FROM component "                         \
       "WHERE type=2 AND content_id IS NULL");
DEFSQL(DELETE_COMPONENTS_BY_CONTENT_ID,              \
       "DELETE FROM component "                         \
       "WHERE content_id=:content_id");
DEFSQL(LIST_COMPONENTS_BY_NAME,                                         \
       "SELECT basename(name) "                                         \
       "FROM component "                                                \
       "WHERE dirname(name)=:name AND trash_id IS NULL AND name<>''");
DEFSQL(LIST_COMPONENTS_BY_NAME_NOCASE,                                  \
       "SELECT basename(name) "                                         \
       "FROM component "                                                \
       "WHERE dirname(name_upper)=:name_upper AND trash_id IS NULL AND name_upper<>''");
DEFSQL(LIST_COMPONENTS_BY_NAME_TRASH_ID,                                \
       "SELECT basename(name) "                                         \
       "FROM component "                                                \
       "WHERE dirname(name)=:name AND trash_id=:trash_id AND name<>''");
DEFSQL(LIST_COMPONENTS_BY_NAME_TRASH_ID_NOCASE,                         \
       "SELECT basename(name) "                                         \
       "FROM component "                                                \
       "WHERE dirname(name_upper)=:name_upper AND trash_id=:trash_id AND name_upper<>''");
DEFSQL(GET_NEW_CONTENT,                         \
       "INSERT INTO content (path) "            \
       "VALUES (:path)");
DEFSQL(GET_NEXT_CONTENT,                         \
       "SELECT id, path, size, hash, mtime, restore_id " \
       "FROM content "                                   \
       "WHERE id > :id ORDER BY id LIMIT 1");
DEFSQL(SET_CONTENT_PATH_BY_ID,                  \
       "UPDATE content "                        \
       "SET path=:path "                        \
       "WHERE id=:id");
DEFSQL(GET_CONTENT_BY_ID,                                       \
       "SELECT id, path, size, hash, mtime, restore_id "        \
       "FROM content "                                          \
       "WHERE id=:id");
DEFSQL(GET_CONTENT_BY_PATH,                                     \
       "SELECT id, path, size, hash, mtime, restore_id "        \
       "FROM content "                                          \
       "WHERE path=:path");
DEFSQL(GET_CONTENT_BY_HASH_SIZE,                \
       "SELECT id, path, size, hash, mtime "    \
       "FROM content "                          \
       "WHERE hash=:hash AND size=:size");
DEFSQL(GET_CONTENT_ID_BY_PATH,                  \
       "SELECT id "                             \
       "FROM content "                          \
       "WHERE path=:path");
DEFSQL(GET_CONTENT_REFCOUNT,                            \
       "SELECT COUNT(c.id) "                            \
       "FROM component c, content s "                   \
       "WHERE c.content_id=s.id AND s.path=:path");
DEFSQL(SET_CONTENT_SIZE_BY_ID,                  \
       "UPDATE content "                        \
       "SET size=:size "                        \
       "WHERE id=:id");
DEFSQL(SET_CONTENT_SIZE_IF_LARGER_BY_ID,                        \
       "UPDATE content "                                        \
       "SET size=:size "                                        \
       "WHERE id=:id AND (size IS NULL OR size<:size)");
DEFSQL(SET_CONTENT_SIZE_BY_PATH,                \
       "UPDATE content "                        \
       "SET size=:size "                        \
       "WHERE path=:path");
DEFSQL(SET_CONTENT_HASH_BY_ID,                  \
       "UPDATE content "                        \
       "SET hash=:hash "                        \
       "WHERE id=:id");
DEFSQL(SET_CONTENT_HASH_BY_PATH,
       "UPDATE content SET hash=:hash WHERE path=:path");
DEFSQL(SET_CONTENT_RESTORE_ID_BY_PATH,
       "UPDATE content SET restore_id=:restore_id WHERE path=:path");
DEFSQL(SET_CONTENT_MTIME_BY_PATH,
       "UPDATE content SET mtime=:mtime WHERE path=:path");
DEFSQL(DELETE_ALL_CONTENTS,                     \
       "DELETE FROM content");
DEFSQL(DELETE_CONTENT_BY_PATH,                  \
       "DELETE FROM content "                   \
       "WHERE path=:path");
DEFSQL(CREATE_METADATA,                                 \
       "INSERT INTO metadata (component_id, type) "     \
       "VALUES (:cid, :type)");
DEFSQL(GET_METADATA_BY_COMPONENT_ID_TYPE,               \
       "SELECT id, component_id, type, value "          \
       "FROM metadata "                                 \
       "WHERE component_id=:cid AND type=:type");
DEFSQL(GET_METADATA_BY_COMPONENT_ID,            \
       "SELECT type, value "                    \
       "FROM metadata "                         \
       "WHERE component_id=:cid");
DEFSQL(SET_METADATA_BY_ID,                      \
       "UPDATE metadata "                       \
       "SET value=:value "                      \
       "WHERE id=:id");
DEFSQL(COPY_METADATA_BY_NAME,                                           \
       "INSERT INTO metadata (component_id, type, value) "              \
       "SELECT c2.id, m.type, m.value "                                 \
       "FROM metadata m, component c1, component c2 "                   \
       "WHERE (c1.name=:old OR c1.name GLOB :old||'/*') AND c1.trash_id IS NULL AND m.component_id=c1.id AND c2.name=replaceprefix(c1.name,:old,:new) AND c2.trash_id IS NULL");
DEFSQL(COPY_METADATA_BY_NAME_NOCASE,                                    \
       "INSERT INTO metadata (component_id, type, value) "              \
       "SELECT c2.id, m.type, m.value "                                 \
       "FROM metadata m, component c1, component c2 "                   \
       "WHERE (c1.name_upper=:old_upper OR c1.name_upper GLOB :old_upper||'/*') AND c1.trash_id IS NULL AND m.component_id=c1.id AND c2.name_upper=utf8_upper(replaceprefix_nocase(c1.name_upper,:old_upper,:new)) AND c2.trash_id IS NULL");
DEFSQL(DELETE_METADATA,                                 \
       "DELETE FROM metadata "                          \
       "WHERE component_id=:cid AND type=:type");
DEFSQL(CREATE_TRASH,                                                    \
       "INSERT INTO trash (version, idx, dtime, component_id, size) "   \
       "VALUES (:version, :idx, :dtime, :component_id, :size)");
DEFSQL(GET_TRASH_BY_VERSION_INDEX,                              \
       "SELECT id, version, idx, dtime, component_id, size "    \
       "FROM trash "                                            \
       "WHERE version=:version AND idx=:idx");
DEFSQL(DELETE_TRASH_BY_ID,                      \
       "DELETE FROM trash "                     \
       "WHERE id=:id");
DEFSQL(DELETE_ALL_TRASH,                        \
       "DELETE FROM trash");
DEFSQL(LIST_TRASH,                              \
       "SELECT version, idx "                   \
       "FROM trash");
DEFSQL(LIST_UNREF_CONTENTS,                                             \
       "SELECT s.path "                                                 \
       "FROM content s "                                                \
       "WHERE NOT EXISTS (SELECT 1 FROM component c WHERE c.content_id=s.id)");

static int defaultConstructContentPathFromNum(u64 num,
                                              std::string &path)
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
    path = oss.str();

    return DATASETDB_OK;
}

DatasetDBError DatasetDB::RegisterContentPathConstructFunction(int (*constructContentPathFromNum)(u64 num, std::string &path))
{
    this->constructContentPathFromNum = constructContentPathFromNum;

    return DATASETDB_OK;
}

DatasetDB::DatasetDB() : db(NULL), 
    is_modified(false)
{
    constructContentPathFromNum = defaultConstructContentPathFromNum;
    dbstmts.resize(DATASETDB_STMT_MAX, NULL);
    VPLMutex_Init(&mutex);    
    VPLCond_Init(&condvar);
    options = 0;
}

DatasetDB::~DatasetDB()
{
    CloseDB();

    VPLMutex_Destroy(&mutex);
    VPLCond_Destroy(&condvar);
}

static void utf8_upper_handler(sqlite3_context *context, int nargs, sqlite3_value *args[])
{
    const char *name = (const char*)sqlite3_value_text(args[0]);
    std::string name_upper;

    utf8_upper(name, name_upper);

    sqlite3_result_text(context, name_upper.data(), name_upper.length(), SQLITE_TRANSIENT);
}

#ifndef TESTDATASETDB
static
#endif
void replaceprefix(const std::string &name,
                   const std::string &prefix,
                   const std::string &replstr,
                   std::string &newname,
                   bool nocase)
{
    newname.clear();

    bool match = false;
    std::string commonname;  // part of name after prefix, and without leading slashes

    if (prefix.empty()) {  // prefix pattern is empty, so it is a trivial match
        match = true;
        commonname = name;
    }
    else {
        bool equal;

        if (nocase) {
            equal = (utf8_casencmp(prefix.length(), name.substr(0, prefix.length()).c_str(), prefix.length(), prefix.c_str()) == 0);
        } else {
            equal = (name.compare(0, prefix.length(), prefix) == 0);
        }

        if (equal) {
            if (name.length() == prefix.length()) {
                match = true;
                commonname = "";
            }
            else if (name[prefix.length()] == '/') {
                match = true;
                commonname = name.substr(prefix.length() + 1);
            }
        }
        // ELSE match does not end with '/'
    }

    if (match) {
        newname.append(replstr);
        if (!replstr.empty() && !commonname.empty()) {
            newname.append(1, '/');
        }
        newname.append(commonname);
    }
    else {
        newname.append(name);
    }
}

static void replaceprefix_handler(sqlite3_context *context, int nargs, sqlite3_value *args[])
{
    std::string name = (const char*)sqlite3_value_text(args[0]);
    std::string prefix = (const char*)sqlite3_value_text(args[1]);
    std::string replstr = (const char*)sqlite3_value_text(args[2]);
    std::string newname;

    replaceprefix(name, prefix, replstr, newname, false);
    sqlite3_result_text(context, newname.data(), newname.length(), SQLITE_TRANSIENT);
}

static void replaceprefix_nocase_handler(sqlite3_context *context, int nargs, sqlite3_value *args[])
{
    std::string name = (const char*)sqlite3_value_text(args[0]);
    std::string prefix = (const char*)sqlite3_value_text(args[1]);
    std::string replstr = (const char*)sqlite3_value_text(args[2]);
    std::string newname;

    replaceprefix(name, prefix, replstr, newname, true);
    sqlite3_result_text(context, newname.data(), newname.length(), SQLITE_TRANSIENT);
}

static void dirname_handler(sqlite3_context *context, int nargs, sqlite3_value *args[])
{
    const char *name = (const char*)sqlite3_value_text(args[0]);

    const char *p = strrchr(name, '/');
    if (p) {
        sqlite3_result_text(context, name, p - name, SQLITE_TRANSIENT);
    }
    else {
        static char empty[] = "";
        sqlite3_result_text(context, empty, -1, SQLITE_STATIC);
    }
}

static void basename_handler(sqlite3_context *context, int nargs, sqlite3_value *args[])
{
    const char *name = (const char*)sqlite3_value_text(args[0]);

    const char *p = strrchr(name, '/');
    if (p) {
        sqlite3_result_text(context, p + 1, -1, SQLITE_TRANSIENT);
    }
    else {
        sqlite3_result_text(context, name, -1, SQLITE_TRANSIENT);
    }
}

DatasetDBError DatasetDB::updateSchema()
{
    DatasetDBError rv;
    u64 schemaVersion;
    char *errmsg = NULL;

    rv = GetDatasetSchemaVersion(schemaVersion);
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
        case 1: {
            // Schema v2 adds restore_id column to contents table.
            const char update_to_v2_sql[] =
                "ALTER TABLE content ADD COLUMN restore_id INTEGER; ";
            
            LOG_INFO("Updating schema v"FMTu64"->v2",
                     schemaVersion);

            rv = sqlite3_exec(db, update_to_v2_sql, NULL, NULL, &errmsg);
            if (rv != SQLITE_OK) {
                LOG_ERROR("Failed to apply v2 schema update.: %d: %s", rv, errmsg);
                sqlite3_free(errmsg);
                return mapSqliteErrCode(rv);
            }
        }   // fall-through to apply next update.
        case 2: // no change. fall-through          
        case TARGET_SCHEMA_VERSION:
            // Current schema
            rv = SetDatasetSchemaVersion(TARGET_SCHEMA_VERSION);
            if (rv != DATASETDB_OK) {
                LOG_ERROR("Failed to update schema version to "FMTu64": %d", 
                          (u64)TARGET_SCHEMA_VERSION, rv);
                return rv;
            }

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

static int count_version_tables(void *param, int, char**, char**)
{
    int *count = (int*)param;
    (*count)++;
    return 0;
}

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

DatasetDBError DatasetDB::openDBUnlocked(const std::string &dbpath, bool &fsck_needed, u32 options)
{
    char *errmsg = NULL;
    int rv = DATASETDB_OK;
    int count = 0;
    
    if (db) {
	rv = DATASETDB_ERR_DB_ALREADY_OPEN;
        goto exit;
    }

    this->dbpath.assign(dbpath);
    this->options = options;

    fsck_needed |= testModMarker();

    rv = sqlite3_open_v2(dbpath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (rv != SQLITE_OK) {
	LOG_ERROR("Failed to create/open db at %s: %d", dbpath.c_str(), rv);
	rv = DATASETDB_ERR_DB_OPEN_FAIL;
        goto exit;
    }

    // define new functions
    rv = sqlite3_create_function(db, "replaceprefix", 3, SQLITE_UTF8, NULL, replaceprefix_handler, NULL, NULL);
    if (rv != SQLITE_OK) {
        LOG_ERROR("Failed to register function replaceprefix: %d(%d), %s", rv, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
        rv = mapSqliteErrCode(rv);
        goto exit;
    }
    rv = sqlite3_create_function(db, "dirname", 1, SQLITE_UTF8, NULL, dirname_handler, NULL, NULL);
    if (rv != SQLITE_OK) {
        LOG_ERROR("Failed to register function dirname: %d (%d), %s", rv, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
        rv = mapSqliteErrCode(rv);
        goto exit;
    }
    rv = sqlite3_create_function(db, "basename", 1, SQLITE_UTF8, NULL, basename_handler, NULL, NULL);
    if (rv != SQLITE_OK) {
        LOG_ERROR("Failed to register function basename: %d (%d), %s", rv, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
        rv = mapSqliteErrCode(rv);
        goto exit;
    }
    if (options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        rv = sqlite3_create_function(db, "replaceprefix_nocase", 3, SQLITE_UTF8, NULL, replaceprefix_nocase_handler, NULL, NULL);
        if (rv != SQLITE_OK) {
            LOG_ERROR("Failed to register function replaceprefix_nocase: %d(%d), %s", rv, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
            rv = mapSqliteErrCode(rv);
            goto exit;
        }
        rv = sqlite3_create_function(db, "utf8_upper", 1, SQLITE_UTF8, NULL, utf8_upper_handler, NULL, NULL);
        if (rv != SQLITE_OK) {
            LOG_ERROR("Failed to register function utf8_upper: %d (%d), %s", rv, sqlite3_extended_errcode(db), sqlite3_errmsg(db));
            rv = mapSqliteErrCode(rv);
            goto exit;
        }
    }

    // enable foreign keys
    rv = sqlite3_exec(db, "PRAGMA foreign_keys=ON", NULL, NULL, &errmsg);
    if (rv != SQLITE_OK) {
	LOG_ERROR("Failed to enable foreign key support: %d(%d): %s", rv, sqlite3_extended_errcode(db), errmsg);
	sqlite3_free(errmsg);
        rv =  mapSqliteErrCode(rv);
        goto exit;
    }

    // Use WAL journals as they are less punishing to performance. PERSIST
    // transactions have a penalty of around 100ms/per commit. WAL is a 
    // a write ahead with the main benefits that writers do not block readers
    // and readers do not block writers. It also perfoms fewer fsync() ops.
    rv = sqlite3_exec(db, "PRAGMA journal_mode=WAL", NULL, NULL, &errmsg);
    if (rv != SQLITE_OK) {
	LOG_ERROR("Failed to enable WAL journaling: %d(%d): %s", rv, sqlite3_extended_errcode(db), errmsg);
	sqlite3_free(errmsg);
        rv =  mapSqliteErrCode(rv);
        goto exit;
    }

    // make sure db has needed tables defined
    if (options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        rv = sqlite3_exec(db, test_and_create_db_sql_nocase, NULL, NULL, &errmsg);
    } else {
        rv = sqlite3_exec(db, test_and_create_db_sql, NULL, NULL, &errmsg);
    }
    if (rv != SQLITE_OK) {
	LOG_ERROR("Failed to create tables: %d(%d): %s", rv, sqlite3_extended_errcode(db), errmsg);
	sqlite3_free(errmsg);
        rv = mapSqliteErrCode(rv);
        goto exit;
    }

    rv = sqlite3_exec(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='version'", count_version_tables, &count, &errmsg);
    if (rv != SQLITE_OK) {
        LOG_ERROR("Failed to determine whether tables exist: %d(%d): %s", rv, sqlite3_extended_errcode(db), errmsg);
	sqlite3_free(errmsg);
        rv = mapSqliteErrCode(rv);
        goto exit;
    }
    if (count == 0) {  // tables are missing
        LOG_ERROR("Required tables are missing: %d", count);
        rv = DATASETDB_ERR_TABLES_MISSING;
        goto exit;
    }

    // If schema version isn't latest, apply schema updates.
    rv = updateSchema();
    if (rv != DATASETDB_OK) {
        LOG_ERROR("Failed to update database schema: %d", rv);
        goto exit;
    }

    // Check database integrity
    count = 0;
    rv = sqlite3_exec(db, "PRAGMA integrity_check;", check_integrity_rows, &count, &errmsg);
    if (rv != SQLITE_OK || count > 0) {
	LOG_WARN("Integrity check failed: %d(%d): %s", rv, sqlite3_extended_errcode(db), errmsg);
	sqlite3_free(errmsg);
        
        // Try common repair steps before giving up
        rv = sqlite3_exec(db, "vacuum; reindex; PRAGMA integrity_check;", check_integrity_rows, &count, &errmsg);
        if (rv != SQLITE_OK || count > 0) {
            LOG_ERROR("Integrity check failed after repair steps: %d(%d): %s", rv, sqlite3_extended_errcode(db), errmsg);
            sqlite3_free(errmsg);
            
            rv = DATASETDB_ERR_CORRUPTED;
            goto exit;
        }
    }

    if ( fsck_needed ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Database needs an fsck");
    }

 exit:
    if(rv != DATASETDB_OK) {
        sqlite3_close(db);
        db = NULL;
    }
    return rv;
}

DatasetDBError DatasetDB::OpenDB(const std::string &dbpath, bool &fsck_needed, u32 options)
{
    int rv;

    VPLMutex_Lock(&mutex);
    rv = openDBUnlocked(dbpath, fsck_needed, options);
    VPLMutex_Unlock(&mutex);

    return rv;
}

void DatasetDB::ClearModMarker()
{
    clearModMarker();
}

DatasetDBError DatasetDB::CloseDB()
{
    if (db) {
        int err_cnt = 0;
        int rv;

        std::vector<sqlite3_stmt*>::iterator it;
        for (it = dbstmts.begin(); it != dbstmts.end(); it++) {
            if (*it != NULL) {
                rv = sqlite3_finalize(*it);
                if (isSqliteError(rv)) {
                    err_cnt++;
                    rv = mapSqliteErrCode(rv);
                    LOG_ERROR("Failed to finalize prepared stmt: %d, %s", rv, sqlite3_errmsg(db));
                }
                *it = NULL;
            }
        }

	rv = sqlite3_close(db);
        if (isSqliteError(rv)) {
            err_cnt++;
            rv = mapSqliteErrCode(rv);
            LOG_ERROR("Failed to close db connection: %d, %s", rv, sqlite3_errmsg(db));
        }

        // set or clear the mod marker depending upon the state of the world.
        if ( err_cnt ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Forcing mod marker.");
            setModMarker();
        }
        else {
            clearModMarker();
        }

	db = NULL;
    }

    return DATASETDB_OK;
}

DatasetDBError DatasetDB::GetDatasetFullVersion(u64 &version)
{
    return getDatasetVersion(VERSION_ID_FULL_VERSION, version);
}

DatasetDBError DatasetDB::SetDatasetFullVersion(u64 version)
{
    return setDatasetVersion(VERSION_ID_FULL_VERSION, version);
}

DatasetDBError DatasetDB::GetDatasetFullMergeVersion(u64 &version)
{
    return getDatasetVersion(VERSION_ID_MERGE_VERSION, version);
}

DatasetDBError DatasetDB::SetDatasetFullMergeVersion(u64 version)
{
    return setDatasetVersion(VERSION_ID_MERGE_VERSION, version);
}

DatasetDBError DatasetDB::GetDatasetSchemaVersion(u64 &version)
{
    return getDatasetVersion(VERSION_ID_SCHEMA_VERSION, version);
}

DatasetDBError DatasetDB::SetDatasetSchemaVersion(u64 version)
{
    return setDatasetVersion(VERSION_ID_SCHEMA_VERSION, version);
}

DatasetDBError DatasetDB::GetDatasetRestoreVersion(u64 &version)
{
    return getDatasetVersion(VERSION_ID_RESTORE_VERSION, version);
}

DatasetDBError DatasetDB::SetDatasetRestoreVersion(u64 version)
{
    return setDatasetVersion(VERSION_ID_RESTORE_VERSION, version);
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


DatasetDBError DatasetDB::TestAndCreateComponent(const std::string &name,
						 int type,
                                                 u32 perm,
						 u64 version,
						 VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getComponentByName(name, componentAttrs);
    if (rv == DATASETDB_OK) {
        if (componentAttrs.type != type) {
            rc = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
        }
    }
    else if (rv == DATASETDB_ERR_UNKNOWN_COMPONENT) {
        rv = createComponentAncestryList(name, type, true, names);
        CHECK_RV(rv, rc, end);

        rv = createComponentsIfMissing(names, perm, version, time);
        CHECK_RV(rv, rc, end);

        rv = updateComponentsVersion(names, version);
        CHECK_RV(rv, rc, end);
    }

 end:
    return rc;
}

DatasetDBError DatasetDB::TestExistComponent(const std::string &name)
{
    int rc = DATASETDB_OK;
    int rv;

    rv = testExistComponent(name);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentInfo(const std::string &name,
					   ComponentInfo &info)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    info.name    = name;
    info.ctime   = componentAttrs.ctime;
    info.mtime   = componentAttrs.mtime;
    info.type    = componentAttrs.type;
    info.perm    = componentAttrs.perm;
    info.version = componentAttrs.version;

    if (componentAttrs.content_id != 0) {
	rv = getContentById(componentAttrs.content_id, contentAttrs);
	CHECK_RV(rv, rc, end);

	info.path = contentAttrs.path;
	info.size = contentAttrs.size;
	info.hash = contentAttrs.hash;
        info.restore_id = contentAttrs.restore_id;
    }
    else {
	info.path.clear();
	info.size = 0;
	info.hash.clear();
    }

    rv = getMetadataByComponentId(componentAttrs.id, info.metadata);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentInfo(u64 version, u32 index,
                                           const std::string &name,
					   ComponentInfo &info)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;

    rv = getComponentByVersionIndexName(version, index, name, componentAttrs);
    CHECK_RV(rv, rc, end);

    info.name    = name;
    info.ctime   = componentAttrs.ctime;
    info.mtime   = componentAttrs.mtime;
    info.type    = componentAttrs.type;
    info.perm    = componentAttrs.perm;
    info.version = componentAttrs.version;

    if (componentAttrs.content_id != 0) {
	rv = getContentById(componentAttrs.content_id, contentAttrs);
	CHECK_RV(rv, rc, end);

	info.path = contentAttrs.path;
	info.size = contentAttrs.size;
	info.hash = contentAttrs.hash;
    }
    else {
	info.path.clear();
	info.size = 0;
	info.hash.clear();
    }

    rv = getMetadataByComponentId(componentAttrs.id, info.metadata);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentType(const std::string &name,
					   int &type)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    type = componentAttrs.type;

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentPath(const std::string &name,
                                           std::string &path)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    if (componentAttrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
	rc = DATASETDB_ERR_IS_DIRECTORY;
	goto end;
    }
    if (componentAttrs.content_id == 0) {
	rc = DATASETDB_ERR_UNKNOWN_CONTENT;
	goto end;
    }

    rv = getContentById(componentAttrs.content_id, contentAttrs);
    CHECK_RV(rv, rc, end);
    path.assign(contentAttrs.path);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentPath(u64 version, u32 index,
                                           const std::string &name,
                                           std::string &path)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;
    TrashAttrs trashAttrs;

    rv = getTrashByVersionIndex(version, index, trashAttrs);
    CHECK_RV(rv, rc, end);

    rv = getComponentByNameTrashId(name, trashAttrs.id, componentAttrs);
    CHECK_RV(rv, rc, end);
    if (componentAttrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
        rc = DATASETDB_ERR_IS_DIRECTORY;
        goto end;
    }

    rv = getContentById(componentAttrs.content_id, contentAttrs);
    CHECK_RV(rv, rc, end);

    path = contentAttrs.path;

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentPath(const std::string &name,
                                           const std::string &path,
                                           u32 perm,
                                           u64 version,
                                           VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    u64 content_id;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;
    std::vector<std::pair<std::string, int> > names;
    bool createIfNecessary = version != 0 || time != 0;

    if (createIfNecessary) {
        rv = createComponentAncestryList(name, DATASETDB_COMPONENT_TYPE_FILE, true, names);
        CHECK_RV(rv, rc, end);

        rv = createComponentsIfMissing(names, perm, version, time);
        CHECK_RV(rv, rc, end);
    }

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    /* cases:
     * c.content_id == NULL -> find/create content and set c.content_id
     * c.content_id != NULL, s.path == NULL -> set path in s.path
     * c.content_id != NULL, s.path == path -> NOOP
     * c.content_id != NULL, s.path != path -> find/create content and set c.content_id
     */
    if (componentAttrs.content_id == 0) {
        rv = createContentIfMissing(path, content_id);
        CHECK_RV(rv, rc, end);

        rv = updateComponentContentIdByName(name, content_id);
        CHECK_RV(rv, rc, end);
    }
    else {
        rv = getContentById(componentAttrs.content_id, contentAttrs);
        CHECK_RV(rv, rc, end);

        if (contentAttrs.path != path) {
            rv = createContentIfMissing(path, content_id);
            CHECK_RV(rv, rc, end);

            rv = updateComponentContentIdByName(name, content_id);
            CHECK_RV(rv, rc, end);
        }
    }

    if (createIfNecessary) {
        rv = updateComponentsVersion(names, version);
        CHECK_RV(rv, rc, end);
    }

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentPath(u64 trash_version, u32 index,
                                           const std::string &name,
                                           const std::string &path,
                                           u64 version,
                                           VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    u64 content_id;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;
    std::vector<std::pair<std::string, int> > names;
    bool createIfNecessary = version != 0 || time != 0;
    u64 component_id;
    
    if(createIfNecessary) {
        rv = createComponentIfMissing(trash_version, index, name, version, time,
                                      DATASETDB_COMPONENT_TYPE_FILE, component_id);
        CHECK_RV(rv, rc, end);
    }

    rv = getComponentByVersionIndexName(trash_version, index, name, componentAttrs);
    CHECK_RV(rv, rc, end);

    /* cases:
     * c.content_id == NULL -> find/create content and set c.content_id
     * c.content_id != NULL, s.path == NULL -> set path in s.path
     * c.content_id != NULL, s.path == path -> NOOP
     * c.content_id != NULL, s.path != path -> find/create content and set c.content_id
     */
    if (componentAttrs.content_id == 0) {
        rv = createContentIfMissing(path, content_id);
        CHECK_RV(rv, rc, end);

        rv = updateComponentContentIdByVersionIndexName(trash_version, index, name, content_id);
        CHECK_RV(rv, rc, end);
    }
    else {
        rv = getContentById(componentAttrs.content_id, contentAttrs);
        CHECK_RV(rv, rc, end);

        if (contentAttrs.path != path) {
            rv = createContentIfMissing(path, content_id);
            CHECK_RV(rv, rc, end);

            rv = updateComponentContentIdByVersionIndexName(trash_version, index, name, content_id);
            CHECK_RV(rv, rc, end);
        }
    }

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentCreateTime(const std::string &name,
						 VPLTime_t &ctime)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    ctime = componentAttrs.ctime;

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentLastModifyTime(const std::string &name,
						     VPLTime_t &mtime)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    mtime = componentAttrs.mtime;

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentCreationTime(const std::string &name,
                                                   u64 version,
                                                   VPLTime_t ctime)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs attrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getComponentByName(name, attrs);
    CHECK_RV(rv, rc, end);

    rv = updateComponentCreationTime(name, ctime);
    CHECK_RV(rv, rc, end);

    rv = createComponentAncestryList(name, attrs.type, true, names);
    CHECK_RV(rv, rc, end);

    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentLastModifyTime(const std::string &name,
                                                     u64 version,
						     VPLTime_t mtime)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs attrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getComponentByName(name, attrs);
    CHECK_RV(rv, rc, end);

    rv = updateComponentLastModifyTime(name, mtime);
    CHECK_RV(rv, rc, end);

    rv = createComponentAncestryList(name, attrs.type, true, names);
    CHECK_RV(rv, rc, end);

    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentPermission(const std::string &name,
                                                 u32 &perm)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    perm = componentAttrs.perm;

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentPermission(const std::string &name,
                                                 u32 perm,
                                                 u64 version)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs attrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getComponentByName(name, attrs);
    CHECK_RV(rv, rc, end);

    rv = updateComponentPermission(name, perm);
    CHECK_RV(rv, rc, end);

    rv = createComponentAncestryList(name, attrs.type, true, names);
    CHECK_RV(rv, rc, end);

    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentSize(const std::string &name,
					   u64 &size)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    switch (componentAttrs.type) {
    case DATASETDB_COMPONENT_TYPE_DIRECTORY:
        rv = getComponentsTotalSize(name, size);
        CHECK_RV(rv, rc, end);
        break;

    case DATASETDB_COMPONENT_TYPE_FILE:
        if (componentAttrs.content_id == 0) {
            rc = DATASETDB_ERR_UNKNOWN_CONTENT;
            goto end;
        }

        rv = getContentById(componentAttrs.content_id, contentAttrs);
        CHECK_RV(rv, rc, end);
        if (contentAttrs.size_type != SQLITE_INTEGER) {
            rc = DATASETDB_ERR_BAD_DATA;
            goto end;
        }
        size = contentAttrs.size;
        break;

    default:
        rv = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
    }

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentSize(const std::string &name,
                                           u64 size,
                                           u64 version,
                                           VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs attrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getComponentByName(name, attrs);
    CHECK_RV(rv, rc, end);
    if (attrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
        rc = DATASETDB_ERR_IS_DIRECTORY;
        goto end;
    }
    if (attrs.content_id == 0) {
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

    rv = updateContentSizeById(attrs.content_id, size);
    CHECK_RV(rv, rc, end);

    rv = createComponentAncestryList(name, DATASETDB_COMPONENT_TYPE_FILE, true, names);
    CHECK_RV(rv, rc, end);

    rv = updateComponentLastModifyTime(name, time);
    CHECK_RV(rv, rc, end);

    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentSize(u64 trash_version, u32 index,
                                           const std::string &name,
					   u64 size,
                                           u64 version,
					   VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs attrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getComponentByVersionIndexName(trash_version, index, name, attrs);
    CHECK_RV(rv, rc, end);
    if (attrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
	rc = DATASETDB_ERR_IS_DIRECTORY;
	goto end;
    }
    if (attrs.content_id == 0) {
	rc = DATASETDB_ERR_UNKNOWN_CONTENT;
	goto end;
    }

    rv = updateContentSizeById(attrs.content_id, size);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentSizeIfLarger(const std::string &name,
						   u64 size,
						   u64 version,
						   VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs attrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getComponentByName(name, attrs);
    CHECK_RV(rv, rc, end);
    if (attrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
	rc = DATASETDB_ERR_IS_DIRECTORY;
	goto end;
    }
    if (attrs.content_id == 0) {
	rc = DATASETDB_ERR_UNKNOWN_CONTENT;
	goto end;
    }

    rv = updateContentSizeIfLargerById(attrs.content_id, size);
    CHECK_RV(rv, rc, end);

    rv = createComponentAncestryList(name, DATASETDB_COMPONENT_TYPE_FILE, true, names);
    CHECK_RV(rv, rc, end);

    rv = updateComponentLastModifyTime(name, time);
    CHECK_RV(rv, rc, end);

    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetContentSize(const std::string& path, u64 size)
{
    int rc = DATASETDB_OK;
    int rv;

    rv = updateContentSizeByPath(path, size);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentHash(const std::string &name,
					   std::string &hash)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    ContentAttrs contentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    if (componentAttrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
	rc = DATASETDB_ERR_IS_DIRECTORY;
	goto end;
    }
    if (componentAttrs.content_id == 0) {
	rc = DATASETDB_ERR_UNKNOWN_CONTENT;
	goto end;
    }

    rv = getContentById(componentAttrs.content_id, contentAttrs);
    CHECK_RV(rv, rc, end);
    if (contentAttrs.hash.empty()) {
	rc = DATASETDB_ERR_BAD_DATA;
	goto end;
    }
    hash.assign(contentAttrs.hash);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentHash(const std::string &name,
					   const std::string &hash)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    
    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    if (componentAttrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
	rc = DATASETDB_ERR_IS_DIRECTORY;
	goto end;
    }
    if (componentAttrs.content_id == 0) {
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

    rv = updateContentHashById(componentAttrs.content_id, hash);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentHash(u64 version, u32 index,
                                           const std::string &name,
					   const std::string &hash)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    
    rv = getComponentByVersionIndexName(version, index, name, componentAttrs);
    CHECK_RV(rv, rc, end);
    if (componentAttrs.type != DATASETDB_COMPONENT_TYPE_FILE) {
	rc = DATASETDB_ERR_IS_DIRECTORY;
	goto end;
    }
    if (componentAttrs.content_id == 0) {
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

    rv = updateContentHashById(componentAttrs.content_id, hash);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentMetadata(const std::string &name,
					       int type, std::string &value)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    MetadataAttrs metadataAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    rv = getMetadataByComponentIdType(componentAttrs.id, type, metadataAttrs);
    CHECK_RV(rv, rc, end);

    if (metadataAttrs.value_type != SQLITE_BLOB) {
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }
    value.assign(metadataAttrs.value);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentAllMetadata(const std::string &name,
                                                  std::vector<std::pair<int, std::string> > &metadata)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    rv = getMetadataByComponentId(componentAttrs.id, metadata);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentMetadata(const std::string &name,
					       int type, const std::string &value)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    u64 metadata_id;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    rv = createMetadataIfMissing(componentAttrs.id, type, metadata_id);
    CHECK_RV(rv, rc, end);

    rv = updateMetadataValueById(metadata_id, value);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentMetadata(u64 version, u32 index, 
                                               const std::string &name,
					       int type, const std::string &value)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    u64 metadata_id;

    rv = getComponentByVersionIndexName(version, index, name, componentAttrs);
    CHECK_RV(rv, rc, end);

    rv = createMetadataIfMissing(componentAttrs.id, type, metadata_id);
    CHECK_RV(rv, rc, end);

    rv = updateMetadataValueById(metadata_id, value);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::DeleteComponentMetadata(const std::string &name,
                                                  int type)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    rv = deleteMetadata(componentAttrs.id, type);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetComponentVersion(const std::string &name,
					      u64 &version)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    version = componentAttrs.version;

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentVersion(const std::string &name,
                                              u32 perm,
					      u64 version,
					      VPLTime_t time,
                                              int type)
{
    int rc = DATASETDB_OK;
    int rv;
    std::vector<std::pair<std::string, int> > names;

    rv = createComponentAncestryList(name, type, true, names);
    CHECK_RV(rv, rc, end);

    rv = createComponentsIfMissing(names, perm, version, time);
    CHECK_RV(rv, rc, end);

    rv = updateComponentLastModifyTime(name, time);
    CHECK_RV(rv, rc, end);

    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetComponentVersion(u64 trash_version, u32 index,
                                              const std::string &name,
					      u64 version,
					      VPLTime_t time,
                                              int type)
{
    int rc = DATASETDB_OK;
    int rv;
    u64 component_id;

    rv = createComponentIfMissing(trash_version, index, name, version, time,
                                  type, component_id);
    CHECK_RV(rv, rc, end);

    rv = updateTrashComponentVersion(trash_version, index, name, version, time);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::DeleteComponent(const std::string &name,
                                          u64 version,
                                          VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::vector<std::pair<std::string, int> > names;

    rv = createComponentAncestryList(name, DATASETDB_COMPONENT_TYPE_ANY, false, names);
    CHECK_RV(rv, rc, end);

    // delete all components under the given component
    rv = deleteComponentByName(name);
    CHECK_RV(rv, rc, end);

    // update the mtime for the component's parent directory, if any
    rv = updateComponentParentLastModifyTime(name, time);
    CHECK_RV(rv, rc, end);

    // update version from the parent of the component, up to root.
    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::DeleteComponentsByContentId(u64 content_id)
{
    return deleteComponentsByContentId(content_id);
}

DatasetDBError DatasetDB::TrashComponent(const std::string &name,
                                         u64 version, u32 index,
                                         VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    u64 trash_id;
    std::vector<std::pair<std::string, int> > names;
    u64 size;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);

    rv = createComponentAncestryList(componentAttrs.name, DATASETDB_COMPONENT_TYPE_ANY, false, names);
    CHECK_RV(rv, rc, end);

    rv = GetComponentSize(name, size);
    CHECK_RV(rv, rc, end);

    // create a new trash record
    rv = createTrash(version, index, time, componentAttrs.id, size, trash_id);
    CHECK_RV(rv, rc, end);

    // tag all components under the given component with the trash_id
    rv = updateComponentTrashIdByName(name, trash_id);
    CHECK_RV(rv, rc, end);

    // update the mtime for the component's parent directory, if any
    rv = updateComponentParentLastModifyTime(name, time);
    CHECK_RV(rv, rc, end);

    // update version from the parent of the component, up to root.
    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::AddTrashRecord(TrashInfo& info)
{
    int rc = DATASETDB_OK;
    int rv;
    u64 trash_id;

    // create a new trash record
    rv = createTrash(info.version, info.index, info.ctime, 0, info.size, trash_id);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetTrashInfo(u64 version, u32 index,
                                       TrashInfo &info)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;
    TrashAttrs trashAttrs;
    ComponentAttrs componentAttrs;

    rv = getTrashByVersionIndex(version, index, trashAttrs);
    CHECK_RV(rv, rc, end);

    rv = getRootComponentByTrashId(trashAttrs.id, componentAttrs);
    CHECK_RV(rv, rc, end);

    info.name = componentAttrs.name;
    info.ctime = componentAttrs.ctime;
    info.mtime = componentAttrs.mtime;
    info.dtime = trashAttrs.dtime;
    info.type = componentAttrs.type;
    info.version = version;
    info.index = index;
    info.size = trashAttrs.size;

 end:
    return rc;
}

DatasetDBError DatasetDB::RestoreTrash(u64 trash_version, u32 index,
				       const std::string &name,
                                       u32 perm,
                                       u64 version,
                                       VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    TrashAttrs trashAttrs;
    ComponentAttrs componentAttrs;
    std::vector<std::pair<std::string, int> > names;

    rv = getTrashByVersionIndex(trash_version, index, trashAttrs);
    CHECK_RV(rv, rc, end);

    rv = getRootComponentByTrashId(trashAttrs.id, componentAttrs);
    CHECK_RV(rv, rc, end);

    rv = createComponentAncestryList(name, DATASETDB_COMPONENT_TYPE_ANY, false, names);
    CHECK_RV(rv, rc, end);

    // make sure there are parent directories where the component is restored
    rv = createComponentsIfMissing(names, perm, version, time);
    CHECK_RV(rv, rc, end);

    // restore components and update version/mtime
    rv = updateComponentVersionByTrashId(trashAttrs.id, version, time);
    CHECK_RV(rv, rc, end);
    rv = updateComponentNameByTrashId(trashAttrs.id, componentAttrs.name, name, time);
    CHECK_RV(rv, rc, end);
    rv = updateComponentTrashIdByTrashId(trashAttrs.id, 0, time);
    CHECK_RV(rv, rc, end);

    // update the mtime for the component's parent directory, if any
    rv = updateComponentParentLastModifyTime(name, time);
    CHECK_RV(rv, rc, end);

    // update version from restore point up
    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

    // delete the trash record
    rv = deleteTrashById(trashAttrs.id);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::DeleteTrash(u64 version, u32 index)
{
    int rc = DATASETDB_OK;
    int rv;
    TrashAttrs trashAttrs;

    rv = getTrashByVersionIndex(version, index, trashAttrs);
    CHECK_RV(rv, rc, end);

    // delete the trash record
    rv = deleteTrashById(trashAttrs.id);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::DeleteAllTrash()
{
    return deleteAllTrash();
}

DatasetDBError DatasetDB::DeleteAllComponents()
{
    return deleteAllComponents();
}

DatasetDBError DatasetDB::DeleteComponentsUnmodifiedSince(VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_COMPONENTS_UNMODIFIED_SINCE)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_COMPONENTS_UNMODIFIED_SINCE), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :time
    rv = sqlite3_bind_int64(stmt, 1, time);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows deleted -> nothing deleted
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::DeleteComponentFilesWithoutContent()
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_COMPONENT_FILES_WITHOUT_CONTENT)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_COMPONENT_FILES_WITHOUT_CONTENT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows deleted -> nothing deleted
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::MoveComponent(const std::string &old_name,
                                        const std::string &new_name,
                                        u32 perm,
                                        u64 version,
                                        VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::vector<std::pair<std::string, int> > names;

    // if old and new are the same, just update version numbers and modtimes
    if (new_name == old_name) {
        rv = createComponentAncestryList(new_name, DATASETDB_COMPONENT_TYPE_ANY, true, names);
        CHECK_RV(rv, rc, end);

        // update the mtime for the component's parent directory, if any
        rv = updateComponentParentLastModifyTime(new_name, time);
        CHECK_RV(rv, rc, end);

        // update version of ancestor components of destination and destination itself
        rv = updateComponentsVersion(names, version);
        CHECK_RV(rv, rc, end);

        goto end;
    }

    // make sure path to destination component exists
    rv = createComponentAncestryList(new_name, DATASETDB_COMPONENT_TYPE_ANY, false, names);
    CHECK_RV(rv, rc, end);
    rv = createComponentsIfMissing(names, perm, version, time);
    CHECK_RV(rv, rc, end);

    // rewrite component name prefix and update version
    rv = updateComponentNameByName(old_name, new_name, version);
    CHECK_RV(rv, rc, end);

    // update the mtime for the component's parent directory, if any
    rv = updateComponentParentLastModifyTime(new_name, time);
    CHECK_RV(rv, rc, end);

    // update version of ancestor components of destination
    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);
    
    // update version/mtime of ancestor components of source
    rv = createComponentAncestryList(old_name, DATASETDB_COMPONENT_TYPE_ANY, false, names);
    CHECK_RV(rv, rc, end);
    // update the mtime for the component's parent directory, if any
    rv = updateComponentParentLastModifyTime(old_name, time);
    CHECK_RV(rv, rc, end);
    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::CopyComponent(const std::string &old_name,
                                        const std::string &new_name,
                                        u32 perm,
                                        u64 version,
                                        VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::vector<std::pair<std::string, int> > names;

    // make sure old and new are different
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        if (utf8_casencmp(new_name.length(), new_name.c_str(), old_name.length(), old_name.c_str()) == 0) {
            rc = DATASETDB_ERR_BAD_REQUEST;
            goto end;
        }
    } else {
        if (new_name == old_name) {
            rc = DATASETDB_ERR_BAD_REQUEST;
            goto end;
        }
    }

    // make sure path to target component exists
    rv = createComponentAncestryList(new_name, DATASETDB_COMPONENT_TYPE_ANY, false, names);
    CHECK_RV(rv, rc, end);

    rv = createComponentsIfMissing(names, perm, version, time);
    CHECK_RV(rv, rc, end);

    // copy component entries
    rv = copyComponentsByName(old_name, new_name, version);
    CHECK_RV(rv, rc, end);

    // copy metadata entries
    rv = copyMetadataByComponentName(old_name, new_name);
    CHECK_RV(rv, rc, end);

    // update the mtime for the component's parent directory, if any
    rv = updateComponentParentLastModifyTime(new_name, time);
    CHECK_RV(rv, rc, end);

    // update mtime from target component up
    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);
    
 end:
    return rc;
}

DatasetDBError DatasetDB::ListComponents(const std::string &name,
                                         std::vector<std::string> &names)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    CHECK_RV(rv, rc, end);
    if (componentAttrs.type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        rc = DATASETDB_ERR_NOT_DIRECTORY;
        goto end;
    }

    rv = listComponentsByName(name, names);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::ListComponents(const std::string &name,
                                         u64 version, u32 index,
                                         std::vector<std::string> &names)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;
    TrashAttrs trashAttrs;

    rv = getTrashByVersionIndex(version, index, trashAttrs);
    CHECK_RV(rv, rc, end);

    rv = getComponentByNameTrashId(name, trashAttrs.id, componentAttrs);
    CHECK_RV(rv, rc, end);
    if (componentAttrs.type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        rc = DATASETDB_ERR_NOT_DIRECTORY;
        goto end;
    }

    rv = listComponentsByNameTrashId(name, trashAttrs.id, names);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::ListTrash(std::vector<std::pair<u64, u32> > &trashvec)
{
    int rc = DATASETDB_OK;
    int rv;

    rv = listTrash(trashvec);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetContentPathBySizeHash(u64 size,
						   const std::string &hash,
						   std::string &path)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;
    ContentAttrs contentAttrs;

    if (hash.empty()) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    rv = getContentBySizeHash(size, hash, contentAttrs);
    CHECK_RV(rv, rc, end);

    path = contentAttrs.path;

 end:
    return rc;
}

DatasetDBError DatasetDB::GetContentRefCount(const std::string &path,
                                             int &count)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;
    ContentAttrs contentAttrs;

    if (path.empty()) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    rv = getContentRefCount(path, count);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetContentHash(const std::string &path,
                                         std::string &hash)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;
    ContentAttrs contentAttrs;

    if (path.empty()) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    rv = getContentByPath(path, contentAttrs);
    CHECK_RV(rv, rc, end);

    hash = contentAttrs.hash;

 end:
    return rc;
}

DatasetDBError DatasetDB::SetContentHash(const std::string &path,
                                         const std::string &hash)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;

    if (path.empty()) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    rv = updateContentHashByPath(path, hash);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::SetContentRestoreId(const std::string &path,
                                              u64 restore_id)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;

    if (path.empty()) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    rv = updateContentRestoreIdByPath(path, restore_id);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::GetContentLastModifyTime(const std::string &path,
                                                   time_t &mtime)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;
    ContentAttrs contentAttrs;

    if (path.empty()) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    rv = getContentByPath(path, contentAttrs);
    CHECK_RV(rv, rc, end);

    mtime = contentAttrs.mtime;

 end:
    return rc;
}

DatasetDBError DatasetDB::SetContentLastModifyTime(const std::string &path,
                                                   time_t mtime)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;

    if (path.empty()) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    rv = updateContentLastModifyTimeByPath(path, mtime);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::AllocateContentPath(std::string &path)
{
    DatasetDBError rc = DATASETDB_OK;
    DatasetDBError rv;
    u64 dummy_content_id;

    rv = allocateContentPath(path, dummy_content_id);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::DeleteAllContent()
{
    return deleteAllContent();
}

DatasetDBError DatasetDB::DeleteContent(const std::string &path)
{
    return deleteContent(path);
}

DatasetDBError DatasetDB::ListUnrefContents(std::vector<std::string> &paths)
{
    return listUnrefContents(paths);
}

DatasetDBError DatasetDB::GetContentByPath(const std::string &path, ContentAttrs &attrs)
{
    return getContentByPath(path, attrs);
}

DatasetDBError DatasetDB::GetNextContent(ContentAttrs &attrs)
{
    return getNextContent(attrs);
}

//--------------------------------------------------

DatasetDBError DatasetDB::beginTransaction()
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    setModMarker();

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(BEGIN)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(BEGIN), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);


 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }
    VPLMutex_Unlock(&mutex);

    return rc;
}

DatasetDBError DatasetDB::commitTransaction()
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COMMIT)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(COMMIT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::rollbackTransaction()
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ROLLBACK)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(ROLLBACK), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::getDatasetVersion(int versionId, u64 &version)
{
    DatasetDBError rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_DATASET_VERSION)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_DATASET_VERSION), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :id
    rv = sqlite3_bind_int(stmt, 1, versionId);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
	rc = DATASETDB_ERR_UNKNOWN_VERSION;
	goto end;
    }
    // rv == SQLITE_ROW
    if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER) {
	rc = DATASETDB_ERR_BAD_DATA;
	goto end;
    }
    version = sqlite3_column_int64(stmt, 0);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::setDatasetVersion(int versionId, u64 version)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_DATASET_VERSION)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_DATASET_VERSION), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :value
    rv = sqlite3_bind_int64(stmt, 1, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int(stmt, 2, versionId);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}


// assumption: name is well-formed
DatasetDBError DatasetDB::createComponentAncestryList(const std::string &name,
                                                      int componentType,
                                                      bool includeComponent,
						      std::vector<std::pair<std::string, int> > &names)
{
    int rc = DATASETDB_OK;
    size_t pos;
    std::string component = normalizeComponentName(name);

    names.clear();

    // root component cannot be a file
    if (component == "" && componentType == DATASETDB_COMPONENT_TYPE_FILE) {
        rc = DATASETDB_ERR_BAD_REQUEST;
        goto end;
    }

    if (component != "") {
        names.push_back(std::pair<std::string, int>("", DATASETDB_COMPONENT_TYPE_DIRECTORY));
    }
    pos = component.find_first_of('/');
    while (pos != std::string::npos) {
	names.push_back(std::pair<std::string, int>(component.substr(0, pos), DATASETDB_COMPONENT_TYPE_DIRECTORY));
	pos = component.find_first_of('/', pos+1);
    }
    if (includeComponent) {
        names.push_back(std::pair<std::string, int>(component, componentType));
    }

 end:
    return rc;
}

// add entry to component table
// precondition: component must be new
DatasetDBError DatasetDB::createComponent(const std::string &name,
                                          u64 version,
                                          VPLTime_t time,
                                          int type,
                                          u32 perm,
                                          u64 &component_id)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum, bindnum = 1;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(CREATE_COMPONENT_NOCASE);
        getsql = GETSQL(CREATE_COMPONENT_NOCASE);
    } else {
        sqlnum = SQLNUM(CREATE_COMPONENT);
        getsql = GETSQL(CREATE_COMPONENT);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    rv = sqlite3_bind_text(stmt, bindnum++, component.data(), component.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :name_upper
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, bindnum++, upper.data(), upper.size(), NULL);
        CHECK_BIND(rv, rc, db, end);
    }
    // bind :time
    rv = sqlite3_bind_int64(stmt, bindnum++, time);
    CHECK_BIND(rv, rc, db, end);
    // bind :type
    rv = sqlite3_bind_int(stmt, bindnum++, type);
    CHECK_BIND(rv, rc, db, end);
    // bind :version
    rv = sqlite3_bind_int64(stmt, bindnum++, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :version
    rv = sqlite3_bind_int(stmt, bindnum++, perm);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    component_id = sqlite3_last_insert_rowid(db);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::createComponent(u64 trash_version, u32 trash_index,
                                          const std::string &name,
                                          u64 version,
                                          VPLTime_t time,
                                          int type,
                                          u64 &component_id)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum, bindnum = 1;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(CREATE_TRASH_COMPONENT_NOCASE);
        getsql = GETSQL(CREATE_TRASH_COMPONENT_NOCASE);
    } else {
        sqlnum = SQLNUM(CREATE_TRASH_COMPONENT);
        getsql = GETSQL(CREATE_TRASH_COMPONENT);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :trash_version
    rv = sqlite3_bind_int64(stmt, bindnum++, trash_version);
    CHECK_BIND(rv, rc, db, end);
    // bind :idx
    rv = sqlite3_bind_int(stmt, bindnum++, trash_index);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    rv = sqlite3_bind_text(stmt, bindnum++, component.data(), component.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :name_upper
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, bindnum++, upper.data(), upper.size(), NULL);
        CHECK_BIND(rv, rc, db, end);
    }
    // bind :time
    rv = sqlite3_bind_int64(stmt, bindnum++, time);
    CHECK_BIND(rv, rc, db, end);
    // bind :type
    rv = sqlite3_bind_int(stmt, bindnum++, type);
    CHECK_BIND(rv, rc, db, end);
    // bind :version
    rv = sqlite3_bind_int64(stmt, bindnum++, version);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    component_id = sqlite3_last_insert_rowid(db);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// add entry to component table only if new
DatasetDBError DatasetDB::createComponentIfMissing(const std::string &name,
                                                   u64 version,
						   VPLTime_t time,
						   int type,
                                                   u32 perm,
                                                   u64 &component_id)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByName(name, componentAttrs);
    if (rv == DATASETDB_OK) {
	if (componentAttrs.type != type) {
	    rc = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
	    goto end;
	}
    }
    else if (rv == DATASETDB_ERR_UNKNOWN_COMPONENT) {
        rv = createComponent(name, version, time, type, perm, component_id);
        CHECK_RV(rv, rc, end);

        rv = updateComponentParentLastModifyTime(name, time);
        CHECK_RV(rv, rc, end);
    }
    else {
        CHECK_RV(rv, rc, end);
    }
 end:
    return rc;
}

DatasetDBError DatasetDB::createComponentIfMissing(u64 trash_version, u32 index,
                                                   const std::string &name,
                                                   u64 version,
						   VPLTime_t time,
						   int type,
                                                   u64 &component_id)
{
    int rc = DATASETDB_OK;
    int rv;
    ComponentAttrs componentAttrs;

    rv = getComponentByVersionIndexName(trash_version, index, 
                                        name, componentAttrs);
    if (rv == DATASETDB_OK) {
	if (componentAttrs.type != type) {
	    rc = DATASETDB_ERR_WRONG_COMPONENT_TYPE;
	    goto end;
	}
    }
    else if (rv == DATASETDB_ERR_UNKNOWN_COMPONENT) {
        rv = createComponent(trash_version, index, name, version, time, type, component_id);
    }
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::createComponentsIfMissing(const std::vector<std::pair<std::string, int> > &names,
                                                    u32 perm,
                                                    u64 version,
						    VPLTime_t time)
{
    int rc = DATASETDB_OK;

    std::vector<std::pair<std::string, int> >::const_iterator it;
    for (it = names.begin(); it != names.end(); it++) {
        u64 component_id;
	int rv = createComponentIfMissing(it->first, version, time, it->second, perm, component_id);
	CHECK_RV(rv, rc, end);
    }

 end:
    return rc;
}

DatasetDBError DatasetDB::createComponentsIfMissing(u64 trash_version, u32 index,
                                                    const std::vector<std::pair<std::string, int> > &names,
                                                    u64 version,
						    VPLTime_t time)
{
    int rc = DATASETDB_OK;

    std::vector<std::pair<std::string, int> >::const_iterator it;
    for (it = names.begin(); it != names.end(); it++) {
        u64 component_id;
	int rv = createComponentIfMissing(trash_version, index, it->first, version, time, it->second, component_id);
	CHECK_RV(rv, rc, end);
    }

 end:
    return rc;
}

DatasetDBError DatasetDB::createComponentsToRootIfMissing(const std::string &name,
                                                          u32 perm,
                                                          u64 version,
							  VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::vector<std::pair<std::string, int> > names;

    rv = createComponentAncestryList(name, DATASETDB_COMPONENT_TYPE_FILE, true, names);
    CHECK_RV(rv, rc, end);

    rv = createComponentsIfMissing(names, perm, version, time);
    CHECK_RV(rv, rc, end);

    rv = updateComponentsVersion(names, version);
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::testExistComponent(const std::string &name)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(TEST_COMPONENT_BY_NAME_NOCASE);
        getsql = GETSQL(TEST_COMPONENT_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(TEST_COMPONENT_BY_NAME);
        getsql = GETSQL(TEST_COMPONENT_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
	rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
	goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get component entry from sqlite3_stmt object
DatasetDBError DatasetDB::getComponentByStmt(sqlite3_stmt *stmt,
					     ComponentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
	rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
	goto end;
    }
    // rv == SQLITE_ROW

    attrs.id = sqlite3_column_int64(stmt, 0);
    attrs.name.assign((const char*)sqlite3_column_text(stmt, 1));
    attrs.content_id = sqlite3_column_int64(stmt, 2);
    attrs.ctime = sqlite3_column_int64(stmt, 3);
    attrs.mtime = sqlite3_column_int64(stmt, 4);
    attrs.type = sqlite3_column_int(stmt, 5);
    attrs.version = sqlite3_column_int64(stmt, 6);
    attrs.trash_id = sqlite3_column_int64(stmt, 7);
    attrs.perm = sqlite3_column_int(stmt, 8);
 end:
    return rc;
}

// get component table entry by name
// component must not be in the trash can
DatasetDBError DatasetDB::getComponentByName(const std::string &name,
					     ComponentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(GET_COMPONENT_BY_NAME_NOCASE);
        getsql = GETSQL(GET_COMPONENT_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(GET_COMPONENT_BY_NAME);
        getsql = GETSQL(GET_COMPONENT_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = getComponentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get component table entry by name
// component must be in the trash can
DatasetDBError DatasetDB::getComponentByNameTrashId(const std::string &name,
                                                    u64 trash_id,
                                                    ComponentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(GET_COMPONENT_BY_NAME_TRASH_ID_NOCASE);
        getsql = GETSQL(GET_COMPONENT_BY_NAME_TRASH_ID_NOCASE);
    } else {
        sqlnum = SQLNUM(GET_COMPONENT_BY_NAME_TRASH_ID);
        getsql = GETSQL(GET_COMPONENT_BY_NAME_TRASH_ID);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :trash_id
    rv = sqlite3_bind_int64(stmt, 2, trash_id);
    CHECK_BIND(rv, rc, db, end);

    rv = getComponentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get root component table entry by trashid
// component must be in the trash can
DatasetDBError DatasetDB::getRootComponentByTrashId(u64 trash_id,
                                                    ComponentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;
    u32 sqlnum;
    const char *getsql;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(GET_ROOT_COMPONENT_BY_TRASH_ID_NOCASE);
        getsql = GETSQL(GET_ROOT_COMPONENT_BY_TRASH_ID_NOCASE);
    } else {
        sqlnum = SQLNUM(GET_ROOT_COMPONENT_BY_TRASH_ID);
        getsql = GETSQL(GET_ROOT_COMPONENT_BY_TRASH_ID);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :trash_id
    rv = sqlite3_bind_int64(stmt, 1, trash_id);
    CHECK_BIND(rv, rc, db, end);

    rv = getComponentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get component table entry by trash_version, trash_index, and name
// component must be in the trash can
DatasetDBError DatasetDB::getComponentByVersionIndexName(u64 version, u32 index,
                                                         const std::string &name,
                                                         ComponentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(GET_COMPONENT_BY_VERSION_INDEX_NAME_NOCASE);
        getsql = GETSQL(GET_COMPONENT_BY_VERSION_INDEX_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(GET_COMPONENT_BY_VERSION_INDEX_NAME);
        getsql = GETSQL(GET_COMPONENT_BY_VERSION_INDEX_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :version
    rv = sqlite3_bind_int64(stmt, 2, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :index
    rv = sqlite3_bind_int(stmt, 3, index);
    CHECK_BIND(rv, rc, db, end);

    rv = getComponentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get component table entry by rowid
DatasetDBError DatasetDB::getComponentById(u64 id,
					   ComponentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_COMPONENT_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_COMPONENT_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt, 1, id);
    CHECK_BIND(rv, rc, db, end);

    rv = getComponentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// precondition: component already exists and is a directory
DatasetDBError DatasetDB::getComponentsTotalSize(const std::string &name,
                                                 u64 &size)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(GET_COMPONENTS_TOTAL_SIZE_NOCASE);
        getsql = GETSQL(GET_COMPONENTS_TOTAL_SIZE_NOCASE);
    } else {
        sqlnum = SQLNUM(GET_COMPONENTS_TOTAL_SIZE);
        getsql = GETSQL(GET_COMPONENTS_TOTAL_SIZE);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    if(!component.empty()) {
        component += "/";
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv != SQLITE_ROW) {  // this could happen if the component is an empty directory
        size = 0;
        goto end;
    }

    size = sqlite3_column_int64(stmt, 0);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.content_id of non-trashed component by name
// precondition: component already exists
DatasetDBError DatasetDB::updateComponentContentIdByName(const std::string &name,
                                                         u64 content_id)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_CONTENT_ID_BY_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_CONTENT_ID_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_CONTENT_ID_BY_NAME);
        getsql = GETSQL(SET_COMPONENT_CONTENT_ID_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :sid
    rv = sqlite3_bind_int64(stmt, 1, content_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 2, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.content_id of non-trashed component by name
// precondition: component already exists
DatasetDBError DatasetDB::updateComponentContentIdByVersionIndexName(u64 version,
                                                                     u32 index,
                                                                     const std::string &name,
                                                                     u64 content_id)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME);
        getsql = GETSQL(SET_COMPONENT_CONTENT_ID_BY_VERSION_INDEX_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :sid
    rv = sqlite3_bind_int64(stmt, 1, content_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 2, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :version
    rv = sqlite3_bind_int64(stmt, 3, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :idx
    rv = sqlite3_bind_int(stmt, 4, index);
    CHECK_BIND(rv, rc, db, end);    

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}


// update component.ctime of non-trashed component by name
// precondition: component already exists
DatasetDBError DatasetDB::updateComponentCreationTime(const std::string &name,
                                                      VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_CTIME_BY_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_CTIME_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_CTIME_BY_NAME);
        getsql = GETSQL(SET_COMPONENT_CTIME_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :mtime
    rv = sqlite3_bind_int64(stmt, 1, time);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 2, upper.data(), upper.length(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, component.data(), component.length(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }
    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.mtime of non-trashed component by name
// precondition: component already exists
DatasetDBError DatasetDB::updateComponentLastModifyTime(const std::string &name,
							VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_MTIME_BY_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_MTIME_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_MTIME_BY_NAME);
        getsql = GETSQL(SET_COMPONENT_MTIME_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :mtime
    rv = sqlite3_bind_int64(stmt, 1, time);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 2, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }
    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.mtime of non-trashed component by name
// precondition: component already exists
DatasetDBError DatasetDB::updateComponentParentLastModifyTime(const std::string &name,
                                                              VPLTime_t time)
{
    int rc = DATASETDB_OK;
    std::string component = normalizeComponentName(name);

    if(component == "") {
        // No parent to update.
    }
    else {
        size_t pos = component.find_last_of("/");
        if(pos == std::string::npos) {
            // parent is dataset root
            component = "";
        }
        else {
            // parent is a directory
            component = component.substr(0, pos);
        }

        rc = updateComponentLastModifyTime(component, time);
    }

    return rc;
}

// update component.trash_id of non-trashed components by name
// precondition: root component exists
DatasetDBError DatasetDB::updateComponentTrashIdByName(const std::string &name,
                                                       u64 trash_id)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string globpattern;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper_1, upper_2;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_TRASH_ID_BY_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_TRASH_ID_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_TRASH_ID_BY_NAME);
        getsql = GETSQL(SET_COMPONENT_TRASH_ID_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :tid
    rv = sqlite3_bind_int64(stmt, 1, trash_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper_1);
        rv = sqlite3_bind_text(stmt, 2, upper_1.data(), upper_1.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :glob
    globpattern = component.empty() ? "*" : component + "/*";
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(globpattern.c_str(), upper_2);
        rv = sqlite3_bind_text(stmt, 3, upper_2.data(), upper_2.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 3, globpattern.data(), globpattern.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows updated -> component is missing
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.trash_id of trashed components by trash_id
// this function could update multiple rows
// precondition: root component exists
DatasetDBError DatasetDB::updateComponentTrashIdByTrashId(u64 old_trash_id, 
                                                          u64 new_trash_id,
                                                          VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_TRASH_ID_BY_TRASH_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_TRASH_ID_BY_TRASH_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :new
    if (new_trash_id) {
        rv = sqlite3_bind_int64(stmt, 1, new_trash_id);
    }
    else {
        rv = sqlite3_bind_null(stmt, 1);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :now
    rv = sqlite3_bind_int64(stmt, 2, time);
    CHECK_BIND(rv, rc, db, end);
    // bind :old
    rv = sqlite3_bind_int64(stmt, 3, old_trash_id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows updated -> component is missing
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.name of trashed component by trash_id
// old_name as prefix is replaced by new_name
// this function could update multiple rows
DatasetDBError DatasetDB::updateComponentNameByTrashId(u64 trash_id,
						       const std::string &old_name,
						       const std::string &new_name,
						       VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string old_component = normalizeComponentName(old_name);
    std::string new_component = normalizeComponentName(new_name);
    u32 sqlnum;
    const char *getsql;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID_NOCASE);
        getsql = GETSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID_NOCASE);
    } else {
        sqlnum = SQLNUM(CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID);
        getsql = GETSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_TRASH_ID);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :old
    rv = sqlite3_bind_text(stmt, 1, old_component.data(), old_component.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :new
    rv = sqlite3_bind_text(stmt, 2, new_component.data(), new_component.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :tid
    rv = sqlite3_bind_int64(stmt, 3, trash_id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows updated -> component is missing
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.name of non-trashed components by name
// any component.name having old_name as prefix will be modified by new_name
// this function could modify multiple rows
DatasetDBError DatasetDB::updateComponentNameByName(const std::string &old_name,
                                                    const std::string &new_name,
						    u64 version)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string old_component = normalizeComponentName(old_name);
    std::string new_component = normalizeComponentName(new_name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    // make sure old and new are different
    if (new_component == old_component) {
        return DATASETDB_ERR_BAD_REQUEST;
    }

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(CHANGE_COMPONENT_NAME_PREFIX_BY_NAME_NOCASE);
        getsql = GETSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(CHANGE_COMPONENT_NAME_PREFIX_BY_NAME);
        getsql = GETSQL(CHANGE_COMPONENT_NAME_PREFIX_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :old
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(old_component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, old_component.data(), old_component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :new
    rv = sqlite3_bind_text(stmt, 2, new_component.data(), new_component.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :version
    rv = sqlite3_bind_int64(stmt, 3, version);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows updated -> component is missing
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }
    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.version and component.time of trashed components by trash_id
// this function could modify multiple rows
DatasetDBError DatasetDB::updateComponentVersionByTrashId(u64 trash_id,
                                                          u64 version,
                                                          VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_COMPONENT_VERSION_TIME_BY_TRASH_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_COMPONENT_VERSION_TIME_BY_TRASH_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :version
    rv = sqlite3_bind_int64(stmt, 1, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :now
    rv = sqlite3_bind_int64(stmt, 2, time);
    CHECK_BIND(rv, rc, db, end);
    // bind :tid
    rv = sqlite3_bind_int64(stmt, 3, trash_id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows updated -> component is missing
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// copy components by name
// components whose name starts with old_name is duplicated with old_name-part replaced with new_name
// precondition: component must exist
DatasetDBError DatasetDB::copyComponentsByName(const std::string &old_name,
					       const std::string &new_name,
					       u64 version)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string old_component = normalizeComponentName(old_name);
    std::string new_component = normalizeComponentName(new_name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    // make sure old and new are different
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        if (utf8_casencmp(new_component.length(), new_component.c_str(), old_component.length(), old_component.c_str()) == 0) {
            return DATASETDB_ERR_BAD_REQUEST;
        }
    } else {
        if (new_component == old_component) {
            return DATASETDB_ERR_BAD_REQUEST;
        }
    }

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(COPY_COMPONENTS_BY_NAME_NOCASE);
        getsql = GETSQL(COPY_COMPONENTS_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(COPY_COMPONENTS_BY_NAME);
        getsql = GETSQL(COPY_COMPONENTS_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    } 

    // bind :old
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(old_component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, old_component.data(), old_component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :new
    rv = sqlite3_bind_text(stmt, 2, new_component.data(), new_component.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :version
    rv = sqlite3_bind_int64(stmt, 3, version);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows updated -> component is missing
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.version of the untrashed component specified by name
// precondition: component already exists
DatasetDBError DatasetDB::updateComponentVersion(const std::string &name,
                                                 u64 version,
                                                 int type)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_VERSION_BY_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_VERSION_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_VERSION_BY_NAME);
        getsql = GETSQL(SET_COMPONENT_VERSION_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :version
    rv = sqlite3_bind_int64(stmt, 1, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 2, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
	rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
	goto end;
    case 1:  // expected outcome
        break;
    default:  // multiple rows updated -> something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update component.version and component.time of the trashed component specified by name, version, and index
// precondition: component already exists
DatasetDBError DatasetDB::updateTrashComponentVersion(u64 trash_version, u32 index,
                                                      const std::string &name,
                                                      u64 version,
                                                      VPLTime_t time)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME);
        getsql = GETSQL(SET_COMPONENT_VERSION_TIME_BY_VERSION_INDEX_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :version
    rv = sqlite3_bind_int64(stmt, 1, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :mtime
    rv = sqlite3_bind_int64(stmt, 2, time);
    CHECK_BIND(rv, rc, db, end);
    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 3, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 3, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :trash_version
    rv = sqlite3_bind_int64(stmt, 4, trash_version);
    CHECK_BIND(rv, rc, db, end);
    // bind :idx
    rv = sqlite3_bind_int64(stmt, 5, index);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
	rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
	goto end;
    case 1:  // expected outcome
        break;
    default:  // multiple rows updated -> something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// precondition: all components already exists
DatasetDBError DatasetDB::updateComponentsVersion(const std::vector<std::pair<std::string, int> > &names,
                                                  u64 version)
{
    int rc = DATASETDB_OK;

    std::vector<std::pair<std::string, int> >::const_iterator it;
    for (it = names.begin(); it != names.end(); it++) {
        int rv = updateComponentVersion(it->first, version, it->second);
        CHECK_RV(rv, rc, end);
    }

 end:
    return rc;
}

DatasetDBError DatasetDB::updateComponentPermission(const std::string &name,
                                                    u32 perm)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(SET_COMPONENT_PERM_BY_NAME_NOCASE);
        getsql = GETSQL(SET_COMPONENT_PERM_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(SET_COMPONENT_PERM_BY_NAME);
        getsql = GETSQL(SET_COMPONENT_PERM_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :perm
    rv = sqlite3_bind_int(stmt, 1, perm);
    CHECK_BIND(rv, rc, db, end);
    // bind :name
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 2, upper.data(), upper.length(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, component.data(), component.length(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    case 1:  // expected outcome
        break;
    default: // more than one row updated
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }
    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::deleteComponentByName(const std::string &name)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string globpattern;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper_1, upper_2;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(DELETE_COMPONENT_BY_NAME_NOCASE);
        getsql = GETSQL(DELETE_COMPONENT_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(DELETE_COMPONENT_BY_NAME);
        getsql = GETSQL(DELETE_COMPONENT_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper_1);
        rv = sqlite3_bind_text(stmt, 1, upper_1.data(), upper_1.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :glob
    globpattern = component.empty() ? "*" : component + "/*";
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(globpattern.c_str(), upper_2);
        rv = sqlite3_bind_text(stmt, 2, upper_2.data(), upper_2.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 2, globpattern.data(), globpattern.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows deleted -> component is missing
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::deleteComponentsByContentId(u64 content_id)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_COMPONENTS_BY_CONTENT_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_COMPONENTS_BY_CONTENT_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :content_id
    rv = sqlite3_bind_int64(stmt, 1, content_id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {  // no rows deleted -> nothing deleted
        rc = DATASETDB_ERR_UNKNOWN_COMPONENT;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::deleteAllComponents()
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_ALL_COMPONENTS)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_ALL_COMPONENTS), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}


DatasetDBError DatasetDB::listComponentsByName(const std::string &name,
                                               std::vector<std::string> &names)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    names.clear();

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(LIST_COMPONENTS_BY_NAME_NOCASE);
        getsql = GETSQL(LIST_COMPONENTS_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(LIST_COMPONENTS_BY_NAME);
        getsql = GETSQL(LIST_COMPONENTS_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        names.push_back((const char*)sqlite3_column_text(stmt, 0));
    }
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::listComponentsByNameTrashId(const std::string &name,
                                                      u64 trash_id,
                                                      std::vector<std::string> &names)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string component = normalizeComponentName(name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    names.clear();

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(LIST_COMPONENTS_BY_NAME_TRASH_ID_NOCASE);
        getsql = GETSQL(LIST_COMPONENTS_BY_NAME_TRASH_ID_NOCASE);
    } else {
        sqlnum = SQLNUM(LIST_COMPONENTS_BY_NAME_TRASH_ID);
        getsql = GETSQL(LIST_COMPONENTS_BY_NAME_TRASH_ID);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :component
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, component.data(), component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :trash_id
    rv = sqlite3_bind_int64(stmt, 2, trash_id);
    CHECK_BIND(rv, rc, db, end);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        names.push_back((const char*)sqlite3_column_text(stmt, 0));
    }
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// create entry in the content table
// precondition: the entry must not exist
DatasetDBError DatasetDB::createContent(const std::string &path,
                                         u64 &content_id)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_NEW_CONTENT)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_NEW_CONTENT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :path
    rv = sqlite3_bind_text(stmt, 1, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    content_id = sqlite3_last_insert_rowid(db);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// create an entry in the content table if new
DatasetDBError DatasetDB::createContentIfMissing(const std::string &path,
                                                 u64 &content_id)
{
    int rc = DATASETDB_OK;
    int rv;
    ContentAttrs contentAttrs;

    rv = getContentByPath(path, contentAttrs);
    if (rv == DATASETDB_OK) {
        content_id = contentAttrs.id;
    }
    else if (rv == DATASETDB_ERR_UNKNOWN_CONTENT) {
        rv = createContent(path, content_id);
    }
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

DatasetDBError DatasetDB::allocateContentPath(std::string &path,
                                              u64 &content_id)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt_new = dbstmts[SQLNUM(GET_NEW_CONTENT)];
    sqlite3_stmt *&stmt_set = dbstmts[SQLNUM(SET_CONTENT_PATH_BY_ID)];
    if (stmt_new == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_NEW_CONTENT), -1, &stmt_new, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :path to null
    rv = sqlite3_bind_null(stmt_new, 1);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_new);
    CHECK_STEP(rv, rc, db, end);

    content_id = sqlite3_last_insert_rowid(db);

    constructContentPathFromNum(content_id, path);

    if (stmt_set == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_PATH_BY_ID), -1, &stmt_set, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :path
    rv = sqlite3_bind_text(stmt_set, 1, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt_set, 2, content_id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_set);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt_new) {
	sqlite3_reset(stmt_new);
    }
    if (stmt_set) {
	sqlite3_reset(stmt_set);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get content entry from sqlite3_stmt object
DatasetDBError DatasetDB::getContentByStmt(sqlite3_stmt *stmt,
                                           ContentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;
    const char *path;
    const char *hash;

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
	rc = DATASETDB_ERR_UNKNOWN_CONTENT;
	goto end;
    }
    // rv == SQLITE_ROW

    attrs.id = sqlite3_column_int64(stmt, 0);
    path = (const char*)sqlite3_column_text(stmt, 1);
    if (path) {
	attrs.path.assign(path);
    }
    else {
        attrs.path.clear();
    }
    attrs.size_type = sqlite3_column_type(stmt, 2);
    if (attrs.size_type == SQLITE_INTEGER) {
	attrs.size = sqlite3_column_int64(stmt, 2);
    }
    else {
	attrs.size = 0;
    }
    hash = (const char*)sqlite3_column_blob(stmt, 3);
    if (hash) {
	attrs.hash.assign(hash, sqlite3_column_bytes(stmt, 3));
    }
    else {
        attrs.hash.clear();
    }
    attrs.mtime = sqlite3_column_int64(stmt, 4);
    attrs.restore_id = sqlite3_column_int64(stmt, 5);

 end:
    return rc;
}

// get content entry by rowid
DatasetDBError DatasetDB::getContentById(u64 id,
                                         ContentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_CONTENT_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_CONTENT_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt, 1, id);
    CHECK_BIND(rv, rc, db, end);

    rv = getContentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get content entry by path
DatasetDBError DatasetDB::getContentByPath(const std::string &path,
                                           ContentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_CONTENT_BY_PATH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_CONTENT_BY_PATH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_bind_text(stmt, 1, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = getContentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get content entry by size and hash
// precondition: hash is not empty
DatasetDBError DatasetDB::getContentBySizeHash(u64 size,
					       const std::string &hash,
					       ContentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_CONTENT_BY_HASH_SIZE)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_CONTENT_BY_HASH_SIZE), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :hash
    rv = sqlite3_bind_blob(stmt, 1, hash.data(), hash.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :size
    rv = sqlite3_bind_int64(stmt, 2, size);
    CHECK_BIND(rv, rc, db, end);

    rv = getContentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get content entry by path
DatasetDBError DatasetDB::getNextContent(ContentAttrs &attrs)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_NEXT_CONTENT)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_NEXT_CONTENT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_bind_int64(stmt, 1, attrs.id);
    CHECK_BIND(rv, rc, db, end);

    rv = getContentByStmt(stmt, attrs);
    CHECK_RV(rv, rc, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// return the number of components (both non-trashed and trashed) that are referencing the given content path
// return DATASETDB_ERR_UNKNOWN_CONTENT if the given content path is not found in the content table
// this is to allow distinction between a content path that doesn't exist and a content node with no component referencing it.
DatasetDBError DatasetDB::getContentRefCount(const std::string &path,
                                             int &count)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt_id = dbstmts[SQLNUM(GET_CONTENT_ID_BY_PATH)];
    sqlite3_stmt *&stmt_count = dbstmts[SQLNUM(GET_CONTENT_REFCOUNT)];
    if (stmt_id == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_CONTENT_ID_BY_PATH), -1, &stmt_id, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :path
    rv = sqlite3_bind_text(stmt_id, 1, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_id);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    }

    if (stmt_count == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_CONTENT_REFCOUNT), -1, &stmt_count, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :path
    rv = sqlite3_bind_text(stmt_count, 1, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_count);
    CHECK_STEP(rv, rc, db, end);

    count = sqlite3_column_int(stmt_count, 0);

 end:
    if (stmt_id) {
        rv = sqlite3_reset(stmt_id);
    }
    if (stmt_count) {
        rv = sqlite3_reset(stmt_count);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update content.path of content entry with given rowid
// precondition: content already exists
DatasetDBError DatasetDB::updateContentPathById(u64 id, 
                                                const std::string &path)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_PATH_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_PATH_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :path
    rv = sqlite3_bind_text(stmt, 1, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt, 2, id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // multiple rows updated -> something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update content.size of content entry selected by rowid
// precondition: content already exists
DatasetDBError DatasetDB::updateContentSizeById(u64 id, 
                                                u64 size)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_SIZE_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_SIZE_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :size
    rv = sqlite3_bind_int64(stmt, 1, size);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt, 2, id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update content.size of content entry selected by rowid only if larger than what's already in DB
// precondition: content already exists
DatasetDBError DatasetDB::updateContentSizeIfLargerById(u64 id, 
							u64 size)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_SIZE_IF_LARGER_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_SIZE_IF_LARGER_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :size
    rv = sqlite3_bind_int64(stmt, 1, size);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt, 2, id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:  // possible outcome
    case 1:  // possible outcome
        break;
    default:  // something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::updateContentSizeByPath(const std::string &path,
                                                  u64 size)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_SIZE_BY_PATH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_SIZE_BY_PATH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :size
    rv = sqlite3_bind_int64(stmt, 1, size);
    CHECK_BIND(rv, rc, db, end);
    // bind :path
    rv = sqlite3_bind_text(stmt, 2, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update content.hash of content entry selected by rowid
// precondition: content already exists
DatasetDBError DatasetDB::updateContentHashById(u64 id, 
                                                const std::string &hash)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_HASH_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_HASH_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :hash
    rv = sqlite3_bind_blob(stmt, 1, hash.data(), hash.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt, 2, id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::updateContentHashByPath(const std::string &path,
                                                  const std::string &hash)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_HASH_BY_PATH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_HASH_BY_PATH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :hash
    rv = sqlite3_bind_blob(stmt, 1, hash.data(), hash.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :path
    rv = sqlite3_bind_text(stmt, 2, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::updateContentRestoreIdByPath(const std::string &path,
                                                       u64 restore_id)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_RESTORE_ID_BY_PATH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_RESTORE_ID_BY_PATH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :restore_id
    rv = sqlite3_bind_int64(stmt, 1, restore_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :path
    rv = sqlite3_bind_text(stmt, 2, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::updateContentLastModifyTimeByPath(const std::string &path,
                                                            time_t mtime)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_CONTENT_MTIME_BY_PATH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_CONTENT_MTIME_BY_PATH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :mtime
    rv = sqlite3_bind_int64(stmt, 1, mtime);
    CHECK_BIND(rv, rc, db, end);
    // bind :path
    rv = sqlite3_bind_text(stmt, 2, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_CONTENT;
        goto end;
    case 1:  // expected outcome
        break;
    default:  // something wrong with db
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::deleteAllContent()
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_ALL_CONTENTS)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_ALL_CONTENTS), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::deleteContent(const std::string &path)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_CONTENT_BY_PATH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_CONTENT_BY_PATH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :path
    rv = sqlite3_bind_text(stmt, 1, path.data(), path.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// create entry in metadata table
// precondition: metadata entry does not exist yet
DatasetDBError DatasetDB::createMetadata(u64 component_id,
                                         int type,
                                         u64 &metadata_id)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(CREATE_METADATA)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(CREATE_METADATA), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :cid
    rv = sqlite3_bind_int64(stmt, 1, component_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :type
    rv = sqlite3_bind_int(stmt, 2, type);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    metadata_id = sqlite3_last_insert_rowid(db);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// create entry in metadata table if new
DatasetDBError DatasetDB::createMetadataIfMissing(u64 component_id,
                                                  int type,
                                                  u64 &metadata_id)
{
    int rc = DATASETDB_OK;
    int rv;
    MetadataAttrs metadataAttrs;

    rv = getMetadataByComponentIdType(component_id, type, metadataAttrs);
    if (rv == DATASETDB_OK) {
        metadata_id = metadataAttrs.id;
    }
    else if (rv == DATASETDB_ERR_UNKNOWN_METADATA) {
        rv = createMetadata(component_id, type, metadata_id);
    }
    CHECK_RV(rv, rc, end);

 end:
    return rc;
}

// get metadata entry by component_id and type
DatasetDBError DatasetDB::getMetadataByComponentIdType(u64 component_id,
                                                       int type,
                                                       MetadataAttrs &metadataAttrs)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_METADATA_BY_COMPONENT_ID_TYPE)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_METADATA_BY_COMPONENT_ID_TYPE), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :cid
    rv = sqlite3_bind_int64(stmt, 1, component_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :type
    rv = sqlite3_bind_int(stmt, 2, type);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
	rc = DATASETDB_ERR_UNKNOWN_METADATA;
	goto end;
    }
    // rv == SQLITE_ROW

    metadataAttrs.id = sqlite3_column_int64(stmt, 0);
    metadataAttrs.component_id = sqlite3_column_int64(stmt, 1);
    metadataAttrs.type = sqlite3_column_int(stmt, 2);
    metadataAttrs.value_type = sqlite3_column_type(stmt, 3);
    if (metadataAttrs.value_type == SQLITE_BLOB) {
        metadataAttrs.value.assign((const char*)sqlite3_column_blob(stmt, 3), sqlite3_column_bytes(stmt, 3));
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get all metadata entries matching given component_id
DatasetDBError DatasetDB::getMetadataByComponentId(u64 component_id,
                                                   std::vector<std::pair<int, std::string> > &metadata)
{
    int rc = DATASETDB_OK;
    int rv;

    metadata.clear();

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_METADATA_BY_COMPONENT_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_METADATA_BY_COMPONENT_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :cid
    rv = sqlite3_bind_int64(stmt, 1, component_id);
    CHECK_BIND(rv, rc, db, end);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        int type = sqlite3_column_int(stmt, 0);
        std::string value((const char*)sqlite3_column_blob(stmt, 1), sqlite3_column_bytes(stmt, 1));
        metadata.push_back(std::pair<int, std::string>(type, value));
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// update metadata.value by rowid
// precondition: metadata entry already exists
DatasetDBError DatasetDB::updateMetadataValueById(u64 id,
                                                  const std::string &value)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SET_METADATA_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(SET_METADATA_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :value
    rv = sqlite3_bind_blob(stmt, 1, value.data(), value.size(), NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt, 2, id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
        rc = DATASETDB_ERR_UNKNOWN_METADATA;
        goto end;
    case 1:  // expected outcome
        break;
    default:
        rc = DATASETDB_ERR_BAD_DATA;
        goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// copy metadata entries by component name
// metadata belonging to components whose prefix is old_name is copied
DatasetDBError DatasetDB::copyMetadataByComponentName(const std::string &old_name,
                                                      const std::string &new_name)
{
    int rc = DATASETDB_OK;
    int rv;
    std::string old_component = normalizeComponentName(old_name);
    std::string new_component = normalizeComponentName(new_name);
    u32 sqlnum;
    const char *getsql;
    std::string upper;

    VPLMutex_Lock(&mutex);

    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        sqlnum = SQLNUM(COPY_METADATA_BY_NAME_NOCASE);
        getsql = GETSQL(COPY_METADATA_BY_NAME_NOCASE);
    } else {
        sqlnum = SQLNUM(COPY_METADATA_BY_NAME);
        getsql = GETSQL(COPY_METADATA_BY_NAME);
    }

    sqlite3_stmt *&stmt = dbstmts[sqlnum];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, getsql, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :old
    if (this->options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        utf8_upper(old_component.c_str(), upper);
        rv = sqlite3_bind_text(stmt, 1, upper.data(), upper.size(), NULL);
    } else {
        rv = sqlite3_bind_text(stmt, 1, old_component.data(), old_component.size(), NULL);
    }
    CHECK_BIND(rv, rc, db, end);
    // bind :new
    rv = sqlite3_bind_text(stmt, 2, new_component.data(), new_component.size(), NULL);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    // It is possible for no new rows to be added.
    // This happens if the components have no metadata associated with them.

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// delete metadata entry by component_id and type
DatasetDBError DatasetDB::deleteMetadata(u64 component_id,
                                         int type)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_METADATA)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_METADATA), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :cid
    rv = sqlite3_bind_int64(stmt, 1, component_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :type
    rv = sqlite3_bind_int(stmt, 2, type);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    switch (sqlite3_changes(db)) {
    case 0:
	rc = DATASETDB_ERR_UNKNOWN_METADATA;
	goto end;
    case 1:  // expected outcome
	break;
    default:  // this cannot happen -> bad data in db
	rc = DATASETDB_ERR_BAD_DATA;
	goto end;
    }

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// create entry in trash table
DatasetDBError DatasetDB::createTrash(u64 version, u32 index,
                                      VPLTime_t time,
                                      u64 component_id,
                                      u64 size,
                                      u64 &trash_id)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(CREATE_TRASH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(CREATE_TRASH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :version
    rv = sqlite3_bind_int64(stmt, 1, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :idx
    rv = sqlite3_bind_int(stmt, 2, index);
    CHECK_BIND(rv, rc, db, end);
    // bind :dtime
    rv = sqlite3_bind_int64(stmt, 3, time);
    CHECK_BIND(rv, rc, db, end);
    // bind component_id
    rv = sqlite3_bind_int64(stmt, 4, component_id);
    CHECK_BIND(rv, rc, db, end);
    // bind :size
    rv = sqlite3_bind_int64(stmt, 5, size);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

    trash_id = sqlite3_last_insert_rowid(db);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// get trash entry by version and index
DatasetDBError DatasetDB::getTrashByVersionIndex(u64 version, u32 index,
                                                 TrashAttrs &trashAttrs)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(GET_TRASH_BY_VERSION_INDEX)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(GET_TRASH_BY_VERSION_INDEX), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :version
    rv = sqlite3_bind_int64(stmt, 1, version);
    CHECK_BIND(rv, rc, db, end);
    // bind :idx
    rv = sqlite3_bind_int(stmt, 2, index);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (rv == SQLITE_DONE) {
	rc = DATASETDB_ERR_UNKNOWN_TRASH;
	goto end;
    }
    // rv == SQLITE_ROW

    trashAttrs.id = sqlite3_column_int64(stmt, 0);
    trashAttrs.version = sqlite3_column_int64(stmt, 1);
    trashAttrs.index = sqlite3_column_int(stmt, 2);
    trashAttrs.dtime = sqlite3_column_int64(stmt, 3);
    trashAttrs.component_id = sqlite3_column_int64(stmt, 4);
    trashAttrs.size = sqlite3_column_int64(stmt, 5);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// delete trash entry by rowid
DatasetDBError DatasetDB::deleteTrashById(u64 id)
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_TRASH_BY_ID)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_TRASH_BY_ID), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt, 1, id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
    if (sqlite3_changes(db) == 0) {
	rc = DATASETDB_ERR_UNKNOWN_TRASH;
	goto end;
    }

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

// delete all entries from trash table
DatasetDBError DatasetDB::deleteAllTrash()
{
    int rc = DATASETDB_OK;
    int rv;

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_ALL_TRASH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_ALL_TRASH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
	sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::listTrash(std::vector<std::pair<u64, u32> > &trashvec)
{
    int rc = DATASETDB_OK;
    int rv;

    trashvec.clear();

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(LIST_TRASH)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(LIST_TRASH), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        trashvec.push_back(std::pair<u64, u32>(sqlite3_column_int64(stmt, 0),
                                                    sqlite3_column_int(stmt, 1)));
    }
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}

DatasetDBError DatasetDB::listUnrefContents(std::vector<std::string> &paths)
{
    int rc = DATASETDB_OK;
    int rv;

    paths.clear();

    VPLMutex_Lock(&mutex);

    sqlite3_stmt *&stmt = dbstmts[SQLNUM(LIST_UNREF_CONTENTS)];
    if (stmt == NULL) {
        rv = sqlite3_prepare_v2(db, GETSQL(LIST_UNREF_CONTENTS), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        if(name) {
            paths.push_back(name);
        }
    }
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&mutex);
    return rc;
}


std::string normalizeComponentName(const std::string& name)
{
    std::string rv = name;
    
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

static int file_cp(const std::string& src, const std::string& dst)
{
    VPLFile_handle_t ifd = VPLFILE_INVALID_HANDLE, ofd = VPLFILE_INVALID_HANDLE;
    ssize_t inchar, outchar;
    const ssize_t blksize = 1024 * 1024;
    char *buffer = NULL;
    int rv = 0;
    
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

    // remove any partial copies.
    if ( rv ) {
        (void)VPLFile_Delete(dst.c_str());
    }

    return rv;
}

DatasetDBError DatasetDB::Backup()
{
    int rv;
    std::string cur_dbpath;
    std::string backup_file;
    u32 cur_options = 0;
    bool db_is_open = true;
    bool fsck_needed = false;

    VPLMutex_Lock(&mutex);

    if ( db == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "db not open");
        rv = DATASETDB_ERR_NOT_OPEN;
        goto done;
    }

    // Save thse so they're not lost
    cur_dbpath = dbpath;
    cur_options = options;

    // It's also weird that it doesn't hold any locks.
    rv = CloseDB();
    if ( rv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "CloseDB() returned %d", rv);
        goto done;
    }
    db_is_open = false;
        
    // Create a backup
    backup_file = cur_dbpath + "-tmp";
    (void)VPLFile_Delete(backup_file.c_str());

    rv = file_cp(cur_dbpath, backup_file);
    if ( rv ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "file_cp(%s, %s) returned %d", 
            cur_dbpath.c_str(), backup_file.c_str(), rv);
        goto done;
    }

    // re-open the database
    // We assume that the copy is identical to source so opening
    // the source should be sufficient to tell us if the backup is good.
    // Assuming the copy was successful.
    rv = openDBUnlocked(cur_dbpath, fsck_needed, cur_options);
    if ( rv != DATASETDB_OK ) {
        // the backup is bad. Do not use it.
        (void)VPLFile_Delete(backup_file.c_str());
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "openDB() returned %d", rv);
        goto done;
    }
    db_is_open = true;

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
                if ( (rv = VPLFile_Rename(sname.c_str(), dname.c_str())) ) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
                        sname.c_str(), dname.c_str(), rv);
                }
            }
        }

        // finally, move the new backup into position
        if ( i == 1 ) {
            if ( (rv = VPLFile_Rename(backup_file.c_str(), sname.c_str())) ) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "rename(%s, %s) - %d",
                    backup_file.c_str(), sname.c_str(), rv);
                continue;
            }
            // just to be safe
            break;
        }
    }

done:
    if ( !db_is_open ) {
        // reopen the database
        rv = openDBUnlocked(cur_dbpath, fsck_needed, cur_options);
        if ( rv != DATASETDB_OK ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "OpenDB() returned %d", rv);
        }
    }
    VPLMutex_Unlock(&mutex);

    return rv;
}

void DatasetDB::setModMarker(void)
{
    VPLMutex_Lock(&mutex);
    if ( !is_modified ) {
        std::string marker_path;
        FILE *fp;

        marker_path = dbpath + "-mod";
        if ( (fp = VPLFile_FOpen(marker_path.c_str(), "w")) == NULL ) {
            LOG_ERROR("Failed to create marker %s", marker_path.c_str());
            goto done;
        }

        (void)fclose(fp);
        is_modified = true;
    }
done:
    VPLMutex_Unlock(&mutex);
}

void DatasetDB::clearModMarker(void)
{
    std::string marker_path;

    marker_path = dbpath + "-mod";

    VPLMutex_Lock(&mutex);
    (void)VPLFile_Delete(marker_path.c_str());
    is_modified = false;
    VPLMutex_Unlock(&mutex);
}

bool DatasetDB::testModMarker(void)
{
    std::string marker_path;
    bool is_present;

    marker_path = dbpath + "-mod";
    is_present = (VPLFile_CheckAccess(marker_path.c_str(),
                                      VPLFILE_CHECKACCESS_READ) == 0);

    return is_present;
}
