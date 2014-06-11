#include <sqlite3.h>
#include <cstring>
#include "dataset_db.hpp"
#include "dataset_db_sql.hpp"

dataset_db::dataset_db() : db_handle(NULL)
{
    for (int i = 0; i < STMT_COUNT; ++i) {
        stmts[i] = NULL;
    }        
}

int dataset_db::open(const std::string& file_name)
{
    int sqlrv, rv;

    if (db_handle) {
        // Already open
        return DDB_DB_ALREADY_OPEN;
    }

    sqlrv = sqlite3_open(file_name.c_str(), &db_handle);
    if (sqlrv != SQLITE_OK) {
        // The example code calls sqlite3_close in the case of
        // sqlite3_open failure.
        rv = DDB_DB_OPEN_FAILED;
        goto fail;
    }

    // Ensure the tables we need are there.
    bool tables_present;
    rv = haveTables(tables_present);
    if (rv != DDB_SUCCESS) {
        goto fail;
    }
    if (!tables_present) {
        rv = execResultlessQuery(addTablesSQL);
        if (rv != DDB_SUCCESS) {
            goto fail;
        }
    }

    // Pre-compile SQL statements
    static const char *stmtSQL[STMT_COUNT] = {
        updateStatsSQL,
        setParentSQL,
        setRootParentSQL,
        updateDirSQL,
        getStatsSQL,
        deleteStatsSQL,
        checkDirSQL,
        dirContentsSQL,
        setMetadataSQL,
        getMetadataSQL
    };
    for (int i = 0; i < STMT_COUNT; ++i) {
        sqlrv = sqlite3_prepare_v2(db_handle, stmtSQL[i], -1, &stmts[i], NULL);
        if (sqlrv != SQLITE_OK) {
            rv = DDB_COMPILE_FAILED;
            goto fail; 
        }   
    }

    return DDB_SUCCESS;

fail:
    cleanUp();
    return rv;
}

void dataset_db::close()
{
    cleanUp();   
}

void dataset_db::cleanUp()
{
    if (db_handle) {
        sqlite3_close(db_handle);
        db_handle = NULL;
    }
    
    for (int i = 0; i < STMT_COUNT; ++i) {
        sqlite3_finalize(stmts[i]);
        stmts[i] = NULL;
    }
}

int dataset_db::haveTables(bool& tables_present)
{
    // We only currently have 1 table, but they are
    // in any case added atomically.   

    int sqlrv, rv;
    sqlite3_stmt* sql;

    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }

    sqlrv = sqlite3_prepare_v2(db_handle, haveTablesSQL, -1, &sql, NULL);          
    if (sqlrv != SQLITE_OK) {
        return DDB_QUERY_FAILED;
    }
    
    sqlrv = sqlite3_step(sql);
    switch (sqlrv) {
    case SQLITE_DONE:
        // The query executed but returned no rows
        tables_present = false;
        rv = DDB_SUCCESS;
        break;
    case SQLITE_ROW:
        // The query returned one or more rows 
        tables_present = true;
        rv = DDB_SUCCESS;
        break;
    default:
        rv = DDB_QUERY_FAILED;
    }
   
    sqlite3_finalize(sql); 
    return rv;
}

dataset_db::~dataset_db()
{
    cleanUp();
}

// This isn't the optimal way to perform queries.  Precompiled queries
// take less time.
int dataset_db::execResultlessQuery(const char *sql)
{
    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }

    int sqlrv = sqlite3_exec(db_handle, sql, NULL, 0, NULL);
    return (sqlrv == SQLITE_OK) ? DDB_SUCCESS : DDB_QUERY_FAILED;
}

int dataset_db::startTransaction()
{
    return execResultlessQuery(startTransactionSQL);
}

int dataset_db::abandonTransaction()
{
    return execResultlessQuery(abandonTransactionSQL);
}

int dataset_db::commitTransaction()
{
    return execResultlessQuery(commitTransactionSQL);
}

int dataset_db::stepResetResultlessStmt(sqlite3_stmt *stmt)
{
    int sqlrv, rv;

    rv = DDB_SUCCESS;

    sqlrv = sqlite3_step(stmt);
    if (sqlrv != SQLITE_DONE) {
        rv = DDB_QUERY_FAILED;
    }

    sqlrv = sqlite3_reset(stmt);
    if (sqlrv != SQLITE_OK) {
        rv = DDB_QUERY_FAILED;
    }

    return rv;
}

// We could deal with ancestors using triggers, except SQLite has a
// hardwired limit on trigger recursion (1000 levels by default).
// Assumes the directory separator is a /.
// FIXME -- the separator search doesn't handle UTF8.
int dataset_db::setStats(const std::string& name,
                        u64 size,
                        u64 atime,
                        u64 mtime,
                        u64 ctime,
                        bool isDir,
                        u64 versionChanged)
{
    int sqlrv;
    size_t slash_pos;
    std::string path_str;
    u64 parent_rowid;    
    sqlite3_stmt* stmt;

    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }

    // Add/update ancestor directories from the top down.
    // If the file/directory is in the root, it is added here.
    slash_pos = name.find_first_of('/', 1);
    bool done_final;
    if (slash_pos != std::string::npos) {
        path_str = name.substr(0, slash_pos);
        done_final = false;
    } else {
        path_str = name;
        done_final = true;
    }
    parent_rowid = 0;
    for (;;) {
        if (!parent_rowid) {
            stmt = stmts[STMT_SET_ROOT_PARENT];
            sqlrv = sqlite3_bind_text(stmt, 1, path_str.c_str(), -1,
                                           SQLITE_TRANSIENT);
        } else {
            stmt = stmts[STMT_SET_PARENT];
            sqlrv = SQLITE_OK;   // = 0
            sqlrv |= sqlite3_bind_text(stmt, 1, path_str.c_str(), -1,
                                           SQLITE_TRANSIENT);
            sqlrv |= sqlite3_bind_int64(stmt, 2, parent_rowid);
        }

        if (sqlrv != SQLITE_OK) {
            return DDB_QUERY_FAILED;
        }
        
        if (stepResetResultlessStmt(stmt) != DDB_SUCCESS) {
            return DDB_QUERY_FAILED;
        }

        parent_rowid = sqlite3_last_insert_rowid(db_handle);

        slash_pos = name.find_first_of('/', slash_pos + 1);
        if (slash_pos == std::string::npos) {
            break;
        }

        path_str = name.substr(0, slash_pos);        
    } 

    // If the specified file/dir wasn't in the root it won't have been added,
    // so add it now.
    if (!done_final) {
        stmt = stmts[STMT_SET_PARENT];

        sqlrv = SQLITE_OK;   // = 0
        sqlrv |= sqlite3_bind_text(stmt, 1, name.c_str(), -1,
                                           SQLITE_TRANSIENT);
        sqlrv |= sqlite3_bind_int64(stmt, 2, parent_rowid);
        if (sqlrv != SQLITE_OK) {
            return DDB_QUERY_FAILED;
        }

        if (stepResetResultlessStmt(stmt) != DDB_SUCCESS) {
            return DDB_QUERY_FAILED;
        }
    }

    // Update the specified file/dir's stats.
    stmt = stmts[STMT_UPDATE_STATS];
    sqlrv = SQLITE_OK;   // = 0
    sqlrv |= sqlite3_bind_int64(stmt, 1, isDir ? 0ULL : size);
    sqlrv |= sqlite3_bind_int64(stmt, 2, atime);
    sqlrv |= sqlite3_bind_int64(stmt, 3, mtime);
    sqlrv |= sqlite3_bind_int64(stmt, 4, ctime);
    sqlrv |= sqlite3_bind_int(stmt, 5, isDir ? 1 : 0);
    sqlrv |= sqlite3_bind_int64(stmt, 6, versionChanged);
    sqlrv |= sqlite3_bind_text(stmt, 7, name.c_str(), -1,
                                SQLITE_TRANSIENT);
    if (sqlrv != SQLITE_OK) {
        return DDB_QUERY_FAILED;
    }

    if (stepResetResultlessStmt(stmt) != DDB_SUCCESS) {
        return DDB_QUERY_FAILED;
    }    

    // Update ancestor dir's sizes/versions
    slash_pos = name.find_last_of('/');
    stmt = stmts[STMT_UPDATE_DIR];
    while (slash_pos != std::string::npos && slash_pos) { 
        path_str = name.substr(0, slash_pos);
        
        sqlrv = SQLITE_OK;   // = 0
        sqlrv |= sqlite3_bind_int64(stmt, 1, versionChanged);
        sqlrv |= sqlite3_bind_text(stmt, 2, path_str.c_str(), -1,
                                    SQLITE_TRANSIENT);
        if (sqlrv != SQLITE_OK) {
            return DDB_QUERY_FAILED;
        }

        if (stepResetResultlessStmt(stmt) != DDB_SUCCESS) {
            return DDB_QUERY_FAILED;
        }   

        slash_pos = name.find_last_of('/', slash_pos - 1);
    }

    return DDB_SUCCESS; 
}

int dataset_db::deleteStats(const std::string& name)
{
    int sqlrv;
    sqlite3_stmt* stmt;
 
    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }
   
    stmt = stmts[STMT_DELETE_STATS];
  
    // Delete descendents, if any 
    sqlrv = sqlite3_bind_text(stmt, 1, (name + "/%").c_str(), -1,
                                    SQLITE_TRANSIENT);
    if (sqlrv != SQLITE_OK) {
        return DDB_QUERY_FAILED;
    }    

    if (stepResetResultlessStmt(stmt) != DDB_SUCCESS) {
        return DDB_QUERY_FAILED;
    }   

    // Delete the file/dir
    sqlrv = sqlite3_bind_text(stmt, 1, name.c_str(), -1,
                                    SQLITE_TRANSIENT);
    if (stepResetResultlessStmt(stmt) != DDB_SUCCESS) {
        return DDB_QUERY_FAILED;
    }   

    return DDB_SUCCESS;
}

int dataset_db::getStats(const std::string& name,
                        u64& size,
                        u64& atime,
                        u64& mtime,
                        u64& ctime,
                        bool& isDir,
                        u64& versionChanged)
{
    int sqlrv, rv;
    sqlite3_stmt* stmt;
 
    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }
   
    rv = DDB_SUCCESS;
    stmt = stmts[STMT_GET_STATS];
   
    sqlrv = sqlite3_bind_text(stmt, 1, name.c_str(), -1,
                                    SQLITE_TRANSIENT);
    if (sqlrv != SQLITE_OK) {
        rv = DDB_QUERY_FAILED;
        goto cleanup;
    }

    sqlrv = sqlite3_step(stmt);
    if (sqlrv == SQLITE_DONE) {
        // No rows were returned
        rv = DDB_NOT_FOUND;
        goto cleanup;
    }

    if (sqlrv != SQLITE_ROW) {
        rv = DDB_QUERY_FAILED;
        goto cleanup;
    }

    size = sqlite3_column_int64(stmt, 0);
    atime = sqlite3_column_int64(stmt, 1);
    mtime = sqlite3_column_int64(stmt, 2);
    ctime = sqlite3_column_int64(stmt, 3);
    isDir = sqlite3_column_int(stmt, 4) ? true : false;
    versionChanged = sqlite3_column_int64(stmt, 5);

cleanup:
    sqlrv = sqlite3_reset(stmt);
    if (sqlrv != SQLITE_OK) {
        rv = DDB_QUERY_FAILED;
    }

    return rv;
}           

int dataset_db::getDirID(const std::string& name, u64& id)
{
    int sqlrv, rv;
    sqlite3_stmt* stmt;
    bool isDir;

    rv = DDB_SUCCESS;
    stmt = stmts[STMT_CHECK_DIR];
    
    sqlrv = sqlite3_bind_text(stmt, 1, name.c_str(), -1,
                                    SQLITE_TRANSIENT);
    if (sqlrv != SQLITE_OK) {
        return DDB_QUERY_FAILED;
    }   
 
    sqlrv = sqlite3_step(stmt);
    if (sqlrv == SQLITE_DONE) {
        rv = DDB_NOT_FOUND;
        goto cleanup;
    }

    if (sqlrv != SQLITE_ROW) {
        rv = DDB_QUERY_FAILED;
        goto cleanup;
    }

    isDir = sqlite3_column_int(stmt, 0) ? true : false;
    if (!isDir) {
        rv = DDB_NOT_DIRECTORY;
        goto cleanup;
    }

    id = sqlite3_column_int64(stmt, 1); 

cleanup:
    if (sqlite3_reset(stmt) != SQLITE_OK) {
        rv = DDB_QUERY_FAILED;
    }
    
    return rv;
}

int dataset_db::getDirectoryContents(const std::string& dir_name,
                                std::vector<ddb_stats>& recs)
{
    int sqlrv, rv;
    sqlite3_stmt* stmt;
    u64 id;
 
    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }
  
    // Check that dir_name exists and is a directory
    rv = getDirID(dir_name, id);
    if (rv != DDB_SUCCESS) {
        return rv;
    }

    // Find entries in this directory
    stmt = stmts[STMT_DIR_CONTENTS];
    sqlrv = sqlite3_bind_int64(stmt, 1, id);
    if (sqlrv != SQLITE_OK) {
        return DDB_QUERY_FAILED;
    }
 
    recs.clear();
    sqlrv = sqlite3_step(stmt);
    while (sqlrv == SQLITE_ROW) {
        ddb_stats rec;

        rec.size = sqlite3_column_int64(stmt, 0);
        rec.atime = sqlite3_column_int64(stmt, 1);
        rec.mtime = sqlite3_column_int64(stmt, 2);
        rec.ctime = sqlite3_column_int64(stmt, 3);
        rec.isDir = sqlite3_column_int(stmt, 4) ? true : false;
        rec.versionChanged = sqlite3_column_int64(stmt, 5);

        const char *name = (const char *)sqlite3_column_text(stmt, 6);
        if (!name) {
            // Should only happen with a malformed database
            rv = DDB_QUERY_FAILED;
            goto cleanup;
        }
        rec.name = std::string(name);
        recs.push_back(rec); 
    
        sqlrv = sqlite3_step(stmt);
    }

    if (sqlrv != SQLITE_DONE) {
        rv = DDB_QUERY_FAILED;
    }    
 
cleanup:
    sqlrv = sqlite3_reset(stmt);
    if (sqlrv != SQLITE_OK) {
        rv = DDB_QUERY_FAILED;
    }

    return rv;
}

// As things stand, setting metadata for a nonexistent file/dir will
// result in nothing happening.  If desired, we could check for existence.
// 
// We only currently support the signature metadata type (0).
int dataset_db::setMetadata(const std::string& name,
                    int type, const void *data, int len)
{
    int sqlrv;
    sqlite3_stmt* stmt;
 
    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }
          
    if (type != DDB_METADATA_SIGNATURE) {
        return DDB_BAD_METADATA_TYPE;
    }

    if (len < 0 || len > DDB_MAX_METADATA_SIZE) {
        return DDB_BAD_METADATA_SIZE;
    }

    stmt = stmts[STMT_SET_METADATA];
  
    sqlrv = SQLITE_OK;  // = 0
    sqlrv |= sqlite3_bind_blob(stmt, 1, data, len,
                                    SQLITE_TRANSIENT);
    sqlrv |= sqlite3_bind_text(stmt, 2, name.c_str(), -1,
                                    SQLITE_TRANSIENT);
 
    if (sqlrv != SQLITE_OK) {
        return DDB_QUERY_FAILED;
    } 

    if (stepResetResultlessStmt(stmt) != DDB_SUCCESS) {
        return DDB_QUERY_FAILED;
    }
    
    return DDB_SUCCESS;   
}

int dataset_db::getMetadata(const std::string& name,
                    std::vector<ddb_metadata>& recs)
{
    int sqlrv, rv;
    sqlite3_stmt* stmt;
    const void* src;

    if (!db_handle) {
        return DDB_DB_NOT_OPEN;
    }

    rv = DDB_SUCCESS;
    stmt = stmts[STMT_GET_METADATA];
    
    sqlrv = sqlite3_bind_text(stmt, 1, name.c_str(), -1,
                                    SQLITE_TRANSIENT);
    if (sqlrv != SQLITE_OK) {
        return DDB_QUERY_FAILED;
    }   
 
    sqlrv = sqlite3_step(stmt);
    if (sqlrv == SQLITE_DONE) {
        rv = DDB_NOT_FOUND;
        goto cleanup;
    }

    if (sqlrv != SQLITE_ROW) {
        rv = DDB_QUERY_FAILED;
        goto cleanup;
    }

    recs.clear();
    ddb_metadata data;
    data.type = DDB_METADATA_SIGNATURE;
    data.len = sqlite3_column_bytes(stmt, 0);
    if (!data.len) {
        // This record has no metadata
        rv = DDB_SUCCESS;
        goto cleanup;
    }

    src = sqlite3_column_blob(stmt, 0);
    if (!src) {
        rv = DDB_QUERY_FAILED;
        goto cleanup;
    }
    if ((size_t)data.len > sizeof(data.val)) {
        rv = DDB_BAD_METADATA_SIZE;
        goto cleanup;
    }
    memcpy(data.val, src, data.len);
    recs.push_back(data);

cleanup:
    if (sqlite3_reset(stmt) != SQLITE_OK) {
        rv = DDB_QUERY_FAILED;
    }
    
    return rv;
}

int dataset_db::deleteMetadata(const std::string& name)
{
    // Right now, this is equivalent to setting the signature length to 0
    return setMetadata(name, DDB_METADATA_SIGNATURE, NULL, 0);
}

