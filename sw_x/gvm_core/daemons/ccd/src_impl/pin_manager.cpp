#include "pin_manager.hpp"

#include <sqlite3.h>
#include <string>
#include <vector>

#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>
#include <log.h>
#include <vpl_lazy_init.h>

#include "ccd_storage.hpp"
#include "util_open_db_handle.hpp"
#include "gvm_file_utils.hpp"

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


class PinnedMediaDBManager {
public:
    PinnedMediaDBManager();
    int openDb();
    int closeDb();
    int insertPinnedMediaItem(const std::string& path, const std::string& object_id, u64 device_id);
    int deletePinnedMediaItem(const std::string& path);
    int selectPinnedMediaItem(const std::string& path, PinnedMediaItem& output_pinned_media_item);
    int selectPinnedMediaItem(const std::string& object_id, u64 device_id, PinnedMediaItem& output_pinned_media_item);
    int deleteAllPinnedMediaItems();
    int deleteAllRelatedPinnedMediaItems(u64 device_id);
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

static const char SQL_CREATE_PINNED_MEDIA_ITEM_TABLE[] =
"BEGIN;"
"CREATE TABLE IF NOT EXISTS db_attr ("
    "id INTEGER PRIMARY KEY, "
    "value INTEGER NOT NULL);"
"INSERT OR IGNORE INTO db_attr VALUES ("XSTR(KEY_SCHEMA_VERSION)", "XSTR(VALUE_SCHEMA_VERSION)");"
"CREATE TABLE IF NOT EXISTS pin_item ("
    "path TEXT,"
    "object_id TEXT,"
    "device_id INTEGER,"
    "PRIMARY KEY(path));"
"COMMIT;";

PinnedMediaDBManager::PinnedMediaDBManager():m_db(NULL)
{
}

int PinnedMediaDBManager::openDb()
{
    int rv = 0;
    char path[CCD_PATH_MAX_LENGTH];
    memset(path, 0, sizeof(path));
    DiskCache::getPathForPIN(sizeof(path), path);

    m_dbpath.assign(path);
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
        err = sqlite3_exec(m_db, SQL_CREATE_PINNED_MEDIA_ITEM_TABLE, NULL, NULL, &errmsg);
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

int PinnedMediaDBManager::closeDb()
{
    int rv = 0;
    if (m_db) {
        sqlite3_close(m_db);
        m_db = NULL;
    }

    return rv;
}

int PinnedMediaDBManager::insertPinnedMediaItem(const std::string& path, const std::string& object_id, u64 device_id)
{
    int rv = 0;
    int rc;
    static const char* SQL_UPDATE_PINNED_MEDIA_ITEM =
            "INSERT OR REPLACE INTO pin_item (path,"
                                             "object_id,"
                                             "device_id) "
            "VALUES (?,?,?)";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_PINNED_MEDIA_ITEM, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_PINNED_MEDIA_ITEM, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, object_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_int64(stmt, 3, device_id);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int PinnedMediaDBManager::deletePinnedMediaItem(const std::string& path)
{
    int rv = 0;
    int rc;
    static const char* SQL_DELETE_PINNED_MEDIA_ITEM_BY_PATH =
            "DELETE FROM pin_item WHERE path=?";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_PINNED_MEDIA_ITEM_BY_PATH, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_PINNED_MEDIA_ITEM_BY_PATH, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}


int PinnedMediaDBManager::selectPinnedMediaItem(const std::string& path, PinnedMediaItem& output_pinned_media_item)
{
    int rv = 0;
    int rc;
    static const char* SQL_SELECT_PINNED_MEDIA_ITEM_BY_PATH =
            "SELECT path, object_id, device_id "
            "FROM pin_item "
            "WHERE path=? ";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SELECT_PINNED_MEDIA_ITEM_BY_PATH, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SELECT_PINNED_MEDIA_ITEM_BY_PATH, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    {
        std::string path;
        std::string object_id;
        u64 device_id;
        int sql_type;
        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);
        CHECK_ROW_EXIST(rv, rc, m_db, end);

        sql_type = sqlite3_column_type(stmt, 0);
        if (sql_type == SQLITE_TEXT) {
            path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        } else {
            LOG_WARN("Bad column type index: %d", 0);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 1);
        if (sql_type == SQLITE_TEXT) {
            object_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        } else {
            LOG_WARN("Bad column type index: %d", 1);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 2);
        if (sql_type == SQLITE_INTEGER) {
            device_id = sqlite3_column_int64(stmt, 2);
        } else {
            LOG_WARN("Bad column type index: %d", 2);
            rv = -1;
            goto end;
        }

        output_pinned_media_item.path = path;
        output_pinned_media_item.object_id = object_id;
        output_pinned_media_item.device_id = device_id;
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int PinnedMediaDBManager::selectPinnedMediaItem(const std::string& object_id, u64 device_id, PinnedMediaItem& output_pinned_media_item)
{
    int rv = 0;
    int rc;
    static const char* SQL_SELECT_PINNED_MEDIA_ITEM_BY_OBJECT_ID_AND_DEVICE_ID =
            "SELECT path, object_id, device_id "
            "FROM pin_item "
            "WHERE object_id=? AND device_id=?";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_SELECT_PINNED_MEDIA_ITEM_BY_OBJECT_ID_AND_DEVICE_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SELECT_PINNED_MEDIA_ITEM_BY_OBJECT_ID_AND_DEVICE_ID, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, object_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_int64(stmt, 2, device_id);
    CHECK_BIND(rv, rc, m_db, end);

    {
        std::string path;
        std::string object_id;
        u64 device_id;
        int sql_type;
        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);
        CHECK_ROW_EXIST(rv, rc, m_db, end);

        sql_type = sqlite3_column_type(stmt, 0);
        if (sql_type == SQLITE_TEXT) {
            path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        } else {
            LOG_WARN("Bad column type index: %d", 0);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 1);
        if (sql_type == SQLITE_TEXT) {
            object_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        } else {
            LOG_WARN("Bad column type index: %d", 1);
            rv = -1;
            goto end;
        }

        sql_type = sqlite3_column_type(stmt, 2);
        if (sql_type == SQLITE_INTEGER) {
            device_id = sqlite3_column_int64(stmt, 2);
        } else {
            LOG_WARN("Bad column type index: %d", 2);
            rv = -1;
            goto end;
        }

        output_pinned_media_item.path = path;
        output_pinned_media_item.object_id = object_id;
        output_pinned_media_item.device_id = device_id;
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int PinnedMediaDBManager::deleteAllPinnedMediaItems()
{
    int rv = 0;
    int rc;
    static const char* SQL_DELETE_ALL_PINNED_MEDIA_ITEMS =
            "DELETE FROM pin_item";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_ALL_PINNED_MEDIA_ITEMS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_ALL_PINNED_MEDIA_ITEMS, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int PinnedMediaDBManager::deleteAllRelatedPinnedMediaItems(u64 device_id)
{
    int rv = 0;
    int rc;
    static const char* SQL_DELETE_PINNED_MEDIA_ITEM_BY_DEVICE_ID =
            "DELETE FROM pin_item WHERE device_id=?";
    sqlite3_stmt *stmt = NULL;

    CHECK_DB_HANDLE(rv, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_PINNED_MEDIA_ITEM_BY_DEVICE_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_PINNED_MEDIA_ITEM_BY_DEVICE_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, device_id);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

int PinnedMediaDBManager::getSchemaVersion(u64& version)
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

int PinnedMediaDBManager::beginTransaction()
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

int PinnedMediaDBManager::commitTransaction()
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

static PinnedMediaDBManager pinned_media_db_manager;
static VPLLazyInitMutex_t m_mutex = VPLLAZYINITMUTEX_INIT;
static bool is_initialized = false;

int PinnedMediaManager_Init()
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    u64 schema_version;
    rv = pinned_media_db_manager.openDb();
    if (rv) {
        LOG_ERROR("Failed to open pin_manager db, rv = %d", rv);
    }
    is_initialized = true;

    rv = pinned_media_db_manager.getSchemaVersion(schema_version);
    if (rv) {
        LOG_WARN("Failed to get pin_manager db schema version, rv = %d", rv);
    } else {
        LOG_INFO("Current schema version of pin_manager: "FMTu64, schema_version);
    }

    return rv;
}

int PinnedMediaManager_Destroy()
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    rv = pinned_media_db_manager.closeDb();
    if (rv) {
        LOG_ERROR("Failed to close pin_manager db, rv = %d", rv);
    }
    is_initialized = false;
    return rv;
}

int PinnedMediaManager_InsertOrUpdatePinnedMediaItem(const std::string& path, const std::string& object_id, u64 device_id)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call PinnedMediaManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = pinned_media_db_manager.beginTransaction() ||
         pinned_media_db_manager.insertPinnedMediaItem(path, object_id, device_id) ||
         pinned_media_db_manager.commitTransaction();
    if (rv) {
        LOG_ERROR("Failed to insert pinned_media_item, rv = %d (path=%s, object_id=%s, device_id="FMTu64")",
                rv, path.c_str(), object_id.c_str(), device_id);
    }
    return rv;
}

int PinnedMediaManager_RemovePinnedMediaItem(const std::string& path)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call PinnedMediaManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = pinned_media_db_manager.beginTransaction() ||
         pinned_media_db_manager.deletePinnedMediaItem(path) ||
         pinned_media_db_manager.commitTransaction();
    if (rv) {
        LOG_ERROR("Failed to delete pinned_media_item, rv = %d (path=%s)", rv, path.c_str());
    }
    return rv;
}

int PinnedMediaManager_GetPinnedMediaItem(const std::string& path, PinnedMediaItem& output_pinned_media_item)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call PinnedMediaManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = pinned_media_db_manager.selectPinnedMediaItem(path, output_pinned_media_item);
    if (rv == CCD_ERROR_NOT_FOUND) {
        LOG_DEBUG("No such item in database. (path=%s)", path.c_str());
    } else if (rv) {
        LOG_ERROR("Failed to get pinned_media_item, rv = %d (path=%s)", rv, path.c_str());
    }
    return rv;
}

int PinnedMediaManager_GetPinnedMediaItem(const std::string& object_id, u64 device_id, PinnedMediaItem& output_pinned_media_item)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call PinnedMediaManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = pinned_media_db_manager.selectPinnedMediaItem(object_id, device_id, output_pinned_media_item);
    if (rv == CCD_ERROR_NOT_FOUND) {
        LOG_DEBUG("No such item in database. (object_id=%s, device_id="FMTu64")",
                object_id.c_str(), device_id);
    } else if (rv) {
        LOG_ERROR("Failed to get pinned_media_item, rv = %d (object_id=%s, device_id="FMTu64")",
                rv, object_id.c_str(), device_id);
    }
    return rv;
}

int PinnedMediaManager_RemoveAllPinnedMediaItems()
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call PinnedMediaManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = pinned_media_db_manager.deleteAllPinnedMediaItems();
    if (rv) {
        LOG_ERROR("Failed to remove all pinned media items, rv = %d", rv);
    }
    return rv;
}

int PinnedMediaManager_RemoveAllPinnedMediaItemsByDeviceId(u64 device_id)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&m_mutex));
    if (!is_initialized) {
        LOG_ERROR("Need to call PinnedMediaManager_Init()");
        return CCD_ERROR_NOT_INIT;
    }

    rv = pinned_media_db_manager.deleteAllRelatedPinnedMediaItems(device_id);
    if (rv) {
        LOG_ERROR("Failed to remove all pinned media items by device_id, rv = %d", rv);
    }
    return rv;
}
