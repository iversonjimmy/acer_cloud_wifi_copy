//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "util_open_db_handle.hpp"
#include "sqlite3.h"
#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include "vplex_assert.h"
#include <string>
#include "log.h"

#if SQLITE_TEMP_DIR_REQUIRED
static bool s_tempPathInit = false;
const int MAX_TEMP_DIR_LENGTH = 512;
static char SQLITE_TEMP_DIR[MAX_TEMP_DIR_LENGTH];
extern "C" char* sqlite3_temp_directory;
#endif // SQLITE_TEMP_DIR_REQUIRED

int Util_InitSqliteTempDir(const std::string& tempPath)
{
    /// RefB: See http://www.sqlite.org/c3ref/temp_directory.html
    /// Currently the following platforms need the temporary directory set,
    /// otherwise subtle errors could occur:
    ///   cloudnode, winRT, and android.
    /// The other platforms of linux and win32 seems to have a sensible default
    /// already set.

    /// RefC: See http://www.sqlite.org/pragma.html#pragma_temp_store_directory
    /// Calling temp_store_directory_pragma is an error.  Do not ever use this pragma:
    /// This pragma is deprecated and exists for backwards compatibility only.
    int rv = 0;

#if SQLITE_TEMP_DIR_REQUIRED

    int rc;
    if (s_tempPathInit) {
        LOG_ERROR("ALREADY INIT.  Should be only initialized exactly once.");
        rv = UTIL_ERR_INVALID;
        goto end;
    }

    if (tempPath.empty()) {
        LOG_ERROR("Empty tempPath");
        rv = UTIL_ERR_INVALID;
        goto end;
    }
    if (tempPath.size() > MAX_TEMP_DIR_LENGTH-1) {
        LOG_ERROR("tempPath(%s) too long:%d chars", tempPath.c_str(), tempPath.size());
        rv = UTIL_ERR_INVALID;
        goto end;
    }
    rc = Util_CreateDir(tempPath.c_str());
    if (rc != 0) {
        LOG_ERROR("Could not create dir(%s):%d", tempPath.c_str(), rc);
        rv = rc;
        goto end;
    }
    memcpy(SQLITE_TEMP_DIR, tempPath.c_str(), tempPath.size()+1);
    sqlite3_temp_directory = SQLITE_TEMP_DIR;

    LOG_INFO("Util_InitSqliteTempDir(%s)", tempPath.c_str());
    s_tempPathInit = true;
 end:

#endif  // SQLITE_TEMP_DIR_REQUIRED

    return rv;
}

// See http://www.sqlite.org/c3ref/exec.html for callback definition
static int verifyWalMode(void* unUsed,
                         int numColumns,
                         char** columnValueText,
                         char** columnName)
{   // Purpose of this callback function is to log an error if setting wal mode failed.
    bool foundJournalMode = false;
    if (numColumns == 0) {
        LOG_ERROR("Expecting 1 column reporting journal_mode:%d", numColumns);
    }
    for(int i=0; i<numColumns; ++i) {
        std::string strColumnName(columnName[i]);
        if (strColumnName == std::string("journal_mode")) {
            foundJournalMode = true;
            std::string strColumnValue(columnValueText[i]);
            if (!(strColumnValue == std::string("wal"))) {
                LOG_ERROR("Setting wal journal_mode failed:(%s,%s)",
                          strColumnName.c_str(), strColumnValue.c_str());
            }
        }
    }
    if (!foundJournalMode) {
        LOG_ERROR("No journal_mode setting returned.  Columns returned:%d", numColumns);
        for(int j=0; j<numColumns; ++j) {
            LOG_ERROR("  journal_mode column[%d]:(%s,%s)",
                      j, columnName[j], columnValueText[j]);
        }
    }

    // Non-zero return value would cause sqlite3_exec to return SQLITE_ABORT.
    return 0;
}

int Util_OpenDbHandle(const std::string& dbpath,
                      bool write,
                      bool create,
                      sqlite3 **dbHandle_out)
{
    /// RefA: See http://www.sqlite.org/c3ref/open.html for documentation
    int rv = 0;
    int rc;

#if SQLITE_TEMP_DIR_REQUIRED
    if (!s_tempPathInit) {
        LOG_CRITICAL("Util_InitSqliteTempDir never called.  Should be called "
                     "exactly once to prevent subtle errors.");
        ASSERT(s_tempPathInit);
        return SQLITE_ERROR;  // returning sqlite code because sqlite codes are
                              // often remapped by the function caller.
    }
#endif  // SQLITE_TEMP_DIR_REQUIRED

    // Let's ALWAYS use fullmutex mode.  This is the safest, without much performance loss.
    int flags = SQLITE_OPEN_FULLMUTEX;
    if (create) {
        flags |= SQLITE_OPEN_CREATE;
    }
    if (write) {
        flags |= SQLITE_OPEN_READWRITE;
    } else {
        flags |= SQLITE_OPEN_READONLY;
    }

    rc = sqlite3_open_v2(dbpath.c_str(), dbHandle_out, flags, NULL);
    if (rc != 0) {
        LOG_ERROR("Failed to open(write:%d, create:%d) db(%s):%d(%s)",
                  write, create, dbpath.c_str(), rc, sqlite3_errmsg(*dbHandle_out));
        rv = rc;
        goto end;
    }

    // http://sqlite.org/c3ref/busy_timeout.html
    // For multiple threads, the wait time in milliseconds while a db is locked
    // before the error 5 (database is locked) is returned.  When not specified,
    // this error returns immediately whenever there's the slightest contention.
    rc = sqlite3_busy_timeout(*dbHandle_out, 60000);  // 1 minute
    if (rc != 0) {
        LOG_ERROR("sqlite3_busy_timeout:%d", rc);
        rv = rc;
        goto end;
    }

    if(write) {
        // Use WAL journals as they are less punishing to performance.
        // Internal experiments showed the commit time was reduced from 200ms to
        // 40ms using wal-mode.  PERSIST transactions (not set below) have a
        // penalty of around 100ms per commit.  See Bug 12192 for more info.
        char *errmsg = NULL;
        rc = sqlite3_exec(*dbHandle_out, "PRAGMA journal_mode=WAL",
                          verifyWalMode, NULL,
                          &errmsg);
        if (rc != SQLITE_OK) {
            LOG_ERROR("Failed to enable WAL journaling(%s): %d(%d): %s",
                      dbpath.c_str(), rc,
                      sqlite3_extended_errcode(*dbHandle_out), errmsg);
            sqlite3_free(errmsg);
            rv = rc;
            goto end;
        }
    }

 end:
    if (rv != 0) {
        // FROM RefA Documentation:
        // Whether or not an error occurs when it is opened, resources associated
        // with the database connection handle should be released by passing it to
        // sqlite3_close() when it is no longer required.
        rc = sqlite3_close(*dbHandle_out);
        if(rc != 0) {
            LOG_ERROR("sqlite3_close:%d, continuing", rc);
        }
    }
    return rv;
}
