#include "executable_manager.hpp"

#include <sqlite3.h>
#include <string>
#include <vector>

#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>
#include <log.h>
#include <vpl_lazy_init.h>

#include "util_open_db_handle.hpp"
#include "gvm_file_utils.hpp"
#include "gvm_errors.h"

#define SQLITE_ERROR_BASE 0
static int map_sqlite_err_code(int ec)
{
    return SQLITE_ERROR_BASE - ec;
}

static bool is_sqlite_error(int ec)
{
    return ec != SQLITE_OK && ec != SQLITE_ROW && ec != SQLITE_DONE;
}

#define CHECK_DB_HANDLE(result, db, label)                              \
if (db==NULL) {                                                         \
      result = -1;                                                      \
      LOG_ERROR("No db handle.");                                       \
      goto label;                                                       \
}

#define CHECK_PREPARE(rv, rc, sqlstmt, db, label)                       \
if (is_sqlite_error(rc)) {                                              \
    rv = map_sqlite_err_code(rc);                                       \
    LOG_ERROR("Failed to prepare SQL stmt: %d, %s, %s",                 \
              rv, sqlite3_errmsg(db), sqlstmt);                         \
    goto label;                                                         \
}

#define CHECK_BIND(rv, rc, db, label)                                   \
if (is_sqlite_error(rc)) {                                              \
    rv = map_sqlite_err_code(rc);                                       \
    LOG_ERROR("Failed to bind value in prepared stmt: %d, %s", rv, sqlite3_errmsg(db)); \
    goto label;                                                         \
}

#define CHECK_STEP(rv, rc, db, label)                                   \
if (is_sqlite_error(rc)) {                                              \
    rv = map_sqlite_err_code(rc);                                       \
    LOG_ERROR("Failed to execute prepared stmt: %d, %s", rv, sqlite3_errmsg(db)); \
    goto label;                                                         \
}

#define CHECK_ROW_EXIST(rv, rc, db, label)                              \
if (rc!=SQLITE_ROW) {                                                   \
    rv = CCD_ERROR_NOT_FOUND;                                           \
    goto label;                                                         \
}

#define CHECK_FINALIZE(rv, rc, db, label)                               \
if (is_sqlite_error(rc)) {                                              \
    rv = map_sqlite_err_code(rc);                                       \
    LOG_ERROR("Failed to finalize prepared stmt: %d, %s", rv, sqlite3_errmsg(db)); \
    goto label;                                                         \
}

#define FINALIZE_STMT(rv, rc, db, stmt)                                 \
if (stmt) {                                                             \
    rc = sqlite3_finalize(stmt);                                        \
    if(rc != SQLITE_OK) {                                               \
        LOG_ERROR("Failed finalizing statement:%d, %s", rc, sqlite3_errmsg(db)); \
        rv = map_sqlite_err_code(rc);                                   \
    }                                                                   \
    stmt = NULL;                                                        \
}


class RemoteExecutableDBManager {
public:
    RemoteExecutableDBManager();
    int openDb(const std::string& db_rootpath);
    int closeDb();
    int insertRemoteExecutable(const std::string& app_key, const std::string& executable_name, const std::string& absolute_path, u64 version_num);
    int deleteRemoteExecutable(const std::string& app_key, const std::string& executable_name);
    int selectRemoteExecutable(const std::string& app_key, const std::string& executable_name, RemoteExecutable& output_remote_executable);
    int selectRemoteExecutablesByAppKey(const std::string& app_key, std::vector<RemoteExecutable>& output_remote_executables);
    int deleteAllRemoteExecutables();
    int getSchemaVersion(u64& version);
    int beginTransaction();
    int commitTransaction();

private:
    sqlite3 *m_db;
    std::string m_dbpath;
};

#define XSTR(s) STR(s)
#define STR(s) #s

#define CURRENT_SCHEMA_VERSION 1

// The key of SCHEMA_VERSION in db_attr table.
#define KEY_SCHEMA_VERSION 1

// The value of SCHEMA_VERSION in db_attr table.
#define VALUE_SCHEMA_VERSION CURRENT_SCHEMA_VERSION

static const char SQL_CREATE_REMOTE_EXECUTABLE_TABLE[] =
"BEGIN;"
"CREATE TABLE IF NOT EXISTS db_attr ("
    "id INTEGER PRIMARY KEY, "
    "value INTEGER NOT NULL);"
"INSERT OR IGNORE INTO db_attr VALUES ("XSTR(KEY_SCHEMA_VERSION)", "XSTR(VALUE_SCHEMA_VERSION)");"
"CREATE TABLE IF NOT EXISTS remote_executable ("
    "app_key TEXT,"
    "executable_name TEXT,"
    "absolute_path TEXT,"
    "version_num INTEGER,"
    "PRIMARY KEY(app_key, executable_name));"
"COMMIT;";

RemoteExecutableDBManager::RemoteExecutableDBManager():m_db(NULL)
{
}

int RemoteExecutableDBManager::openDb(const std::string& db_rootpath)
{
    int rv = 0;

    m_dbpath.assign(db_rootpath);
    m_dbpath.append("db");

    rv = Util_CreatePath(m_dbpath.c_str(), VPL_FALSE);
    if (rv) {
        LOG_ERROR("Failed to create directory for %s", m_dbpath.c_str());
        goto end;
    }

    rv = Util_OpenDbHandle(m_dbpath.c_str(), true, true, &m_db);
    if (rv) {
        LOG_ERROR("Util_OpenDbHandle(%s):%d", m_dbpath.c_str(), rv);
        goto end;
    }

    CHECK_DB_HANDLE(rv, m_db, end);

    {
        int err = 0;
        char* errmsg = NULL;
        err = sqlite3_exec(m_db, SQL_CREATE_REMOTE_EXECUTABLE_TABLE, NULL, NULL, &errmsg);
        if (err != SQLITE_OK) {
            LOG_ERROR("Failed to test/create tables: %d, %s", err, errmsg);
            sqlite3_free(errmsg);
            rv = err;
            goto end;
        }
    }

end:
    if (rv) {
        closeDb();
    }
    return rv;
}

int RemoteExecutableDBManager::closeDb()
{
    int rv = 0;
    if (m_db) {
        sqlite3_close(m_db);
        m_db = NULL;
    }

    return rv;
}

int RemoteExecutableDBManager::insertRemoteExecutable(const std::string& app_key, const std::string& executable_name, const std::string& absolute_path, u64 version_num)
{
    int rv = 0;
    int rc;
    static const char* SQL_UPDATE_REMOTE_EXECUTABLE =
            "INSERT OR REPLACE INTO remote_executable (app_key,"
                                                      "executable_name,"
                                                      "absolute_path,"
                                                      "version_num) "
            "VALUES (?,?,?,?)";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_REMOTE_EXECUTABLE, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_REMOTE_EXECUTABLE, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, app_key.data(), app_key.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, executable_name.data(), executable_name.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 3, absolute_path.data(), absolute_path.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_int64(stmt, 4, version_num);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int RemoteExecutableDBManager::deleteRemoteExecutable(const std::string& app_key, const std::string& executable_name)
{
    int rv = 0;
    int rc;
    static const char* SQL_DELETE_REMOTE_EXECUTABLE_BY_APPKEY_AND_APPNAME =
            "DELETE FROM remote_executable WHERE app_key=? AND executable_name=?";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_REMOTE_EXECUTABLE_BY_APPKEY_AND_APPNAME, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_REMOTE_EXECUTABLE_BY_APPKEY_AND_APPNAME, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, app_key.data(), app_key.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, executable_name.data(), executable_name.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int RemoteExecutableDBManager::selectRemoteExecutable(const std::string& app_key, const std::string& executable_name, RemoteExecutable& output_remote_executable)
{
    int rv = 0;
    int rc;
    static const char* SQL_SELECT_REMOTE_EXECUTABLE_BY_APPKEY_AND_APPNAME =
            "SELECT app_key, executable_name, absolute_path, version_num "
            "FROM remote_executable "
            "WHERE app_key=? AND executable_name=?";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SELECT_REMOTE_EXECUTABLE_BY_APPKEY_AND_APPNAME, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SELECT_REMOTE_EXECUTABLE_BY_APPKEY_AND_APPNAME, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, app_key.data(), app_key.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, executable_name.data(), executable_name.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    {
        std::string m_app_key;
        std::string m_executable_name;
        std::string m_absolute_path;
        u64 m_version_num;
        int sql_type;
        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);
        CHECK_ROW_EXIST(rv, rc, m_db, end);

        sql_type = sqlite3_column_type(stmt, 0);
        if (sql_type == SQLITE_TEXT) {
            m_app_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        } else {
            LOG_WARN("Bad column type index: %d", 0);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 1);
        if (sql_type == SQLITE_TEXT) {
            m_executable_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        } else {
            LOG_WARN("Bad column type index: %d", 1);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 2);
        if (sql_type == SQLITE_TEXT) {
            m_absolute_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        } else {
            LOG_WARN("Bad column type index: %d", 2);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 3);
        if (sql_type == SQLITE_INTEGER) {
            m_version_num = sqlite3_column_int64(stmt, 3);
        } else {
            LOG_WARN("Bad column type index: %d", 3);
            rv = -1;
            goto end;
        }

        output_remote_executable.app_key = m_app_key;
        output_remote_executable.name = m_executable_name;
        output_remote_executable.absolute_path = m_absolute_path;
        output_remote_executable.version_num = m_version_num;
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int RemoteExecutableDBManager::selectRemoteExecutablesByAppKey(const std::string& app_key, std::vector<RemoteExecutable>& output_remote_executables)
{
    int rv = 0;
    int rc;
    static const char* SQL_SELECT_REMOTE_EXECUTABLE_BY_APPKEY =
                "SELECT app_key, executable_name, absolute_path, version_num "
                "FROM remote_executable "
                "WHERE app_key=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_SELECT_REMOTE_EXECUTABLE_BY_APPKEY, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SELECT_REMOTE_EXECUTABLE_BY_APPKEY, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, app_key.data(), app_key.size(), SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    output_remote_executables.clear();

    while(rv == 0) {
        RemoteExecutable remote_executable;
        int sql_type;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);
        if (rc == SQLITE_DONE) {
            break;
        }

        sql_type = sqlite3_column_type(stmt, 0);
        if (sql_type == SQLITE_TEXT) {
            remote_executable.app_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        } else {
            LOG_WARN("Bad column type index: %d", 0);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 1);
        if (sql_type == SQLITE_TEXT) {
            remote_executable.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        } else {
            LOG_WARN("Bad column type index: %d", 1);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 2);
        if (sql_type == SQLITE_TEXT) {
            remote_executable.absolute_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        } else {
            LOG_WARN("Bad column type index: %d", 2);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 3);
        if (sql_type == SQLITE_INTEGER) {
            remote_executable.version_num = sqlite3_column_int64(stmt, 3);
        } else {
            LOG_WARN("Bad column type index: %d", 3);
            rv = -1;
            goto end;
        }

        output_remote_executables.push_back(remote_executable);
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int RemoteExecutableDBManager::deleteAllRemoteExecutables()
{
    int rv = 0;
    int rc;
    static const char* SQL_DELETE_ALL_REMOTE_EXECUTABLES =
            "DELETE FROM remote_executable";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_ALL_REMOTE_EXECUTABLES, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_ALL_REMOTE_EXECUTABLES, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int RemoteExecutableDBManager::getSchemaVersion(u64& version)
{
    int rv = 0;
    int rc;
    int sql_type;
    static const char* SQL_GET_SCHEMA_VERSION_FROM_DB_ATTR =
            "SELECT id, value "
            "FROM db_attr "
            "WHERE id="XSTR(KEY_SCHEMA_VERSION);
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_GET_SCHEMA_VERSION_FROM_DB_ATTR, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_SCHEMA_VERSION_FROM_DB_ATTR, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);
    CHECK_ROW_EXIST(rv, rc, m_db, end);

    sql_type = sqlite3_column_type(stmt, 1);
    if (sql_type == SQLITE_INTEGER) {
        version = sqlite3_column_int64(stmt, 1);
    } else {
        LOG_WARN("Bad column type index: %d", 1);
        rv = -1;
        goto end;
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int RemoteExecutableDBManager::beginTransaction()
{
    int rv = 0;
    int rc;
    static const char* SQL_BEGIN = "BEGIN IMMEDIATE";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_BEGIN, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_BEGIN, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int RemoteExecutableDBManager::commitTransaction()
{
    int rv = 0;
    int rc;
    static const char* SQL_COMMIT = "COMMIT";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_COMMIT, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_COMMIT, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

static RemoteExecutableDBManager remote_executable_db_manager;
static VPLLazyInitMutex_t m_mutex = VPLLAZYINITMUTEX_INIT;
static bool is_initialized = false;

int RemoteExecutableManager_Init(const std::string& db_rootpath)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    u64 schema_version;
    rv = remote_executable_db_manager.openDb(db_rootpath);
    if (rv) {
        LOG_ERROR("Failed to open executable_manager db, rv = %d", rv);
    }
    is_initialized = true;

    rv = remote_executable_db_manager.getSchemaVersion(schema_version);
    if (rv) {
        LOG_WARN("Failed to get executable_manager db schema version, rv = %d", rv);
    } else {
        LOG_INFO("Current schema version of executable_manager: "FMTu64, schema_version);
    }

    return rv;
}

int RemoteExecutableManager_Destroy()
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    rv = remote_executable_db_manager.closeDb();
    if (rv) {
        LOG_ERROR("Failed to close executable_manager db, rv = %d", rv);
    }
    is_initialized = false;
    return rv;
}

int RemoteExecutableManager_InsertOrUpdateExecutable(const std::string& app_key, const std::string& executable_name, const std::string& absolute_path, u64 version_num)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call RemoteExecutableManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = remote_executable_db_manager.beginTransaction() ||
         remote_executable_db_manager.insertRemoteExecutable(app_key, executable_name, absolute_path, version_num) ||
         remote_executable_db_manager.commitTransaction();
    if (rv) {
        LOG_ERROR("Failed to insert remote_executable, rv = %d (app_key=%s, executable_name=%s, absolute_path=%s version_num="FMTu64")",
                rv, app_key.c_str(), executable_name.c_str(), absolute_path.c_str(), version_num);
    }
    return rv;
}

int RemoteExecutableManager_RemoveExecutable(const std::string& app_key, const std::string& executable_name)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call RemoteExecutableManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = remote_executable_db_manager.beginTransaction() ||
         remote_executable_db_manager.deleteRemoteExecutable(app_key, executable_name) ||
         remote_executable_db_manager.commitTransaction();
    if (rv) {
        LOG_ERROR("Failed to delete remote_executable, rv = %d (app_key=%s, executable_name=%s)", rv, app_key.c_str(), executable_name.c_str());
    }
    return rv;
}

int RemoteExecutableManager_GetExecutable(const std::string& app_key, const std::string& executable_name, RemoteExecutable& output_remote_executable)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call RemoteExecutableManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = remote_executable_db_manager.selectRemoteExecutable(app_key, executable_name, output_remote_executable);
    if (rv == CCD_ERROR_NOT_FOUND) {
        LOG_DEBUG("No such item in database. (app_key=%s, executable_name=%s)",
                app_key.c_str(), executable_name.c_str());
    } else if (rv) {
        LOG_ERROR("Failed to get remote_executable, rv = %d (app_key=%s, executable_name=%s)",
                rv, app_key.c_str(), executable_name.c_str());
    }
    return rv;
}

int RemoteExecutableManager_ListExecutables(const std::string& app_key, std::vector<RemoteExecutable>& output_remote_executables)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call RemoteExecutableManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = remote_executable_db_manager.selectRemoteExecutablesByAppKey(app_key, output_remote_executables);
    if (rv) {
        LOG_ERROR("Failed to get remote_executable, rv = %d", rv);
    }

    return rv;
}

int RemoteExecutableManager_RemoveAllExecutables()
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call RemoteExecutableManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = remote_executable_db_manager.deleteAllRemoteExecutables();
    if (rv) {
        LOG_ERROR("Failed to remove all remote executables, rv = %d", rv);
    }
    return rv;
}
