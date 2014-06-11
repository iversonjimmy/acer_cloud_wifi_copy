//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __MEDIA_DB_UTIL_HPP__
#define __MEDIA_DB_UTIL_HPP__

#include <sqlite3.h>

#define SQL_MAX_QUERY_LENGTH 1024

static int is_sqlite_error(int error_code)
{
    return (error_code != SQLITE_OK) && (error_code != SQLITE_ROW) && (error_code != SQLITE_DONE);
}
static int conv_sqlite_errcode(int ec)
{
    return -17100 - ec;
}

#define CHECK_PREPARE(rv, rc, sqlstmt, db, label)                       \
if (is_sqlite_error(rc)) {                                              \
    rv = conv_sqlite_errcode(rc);                                       \
    LOG_ERROR("Failed to prepare SQL stmt: %d, %s, %s",                 \
              rv, sqlite3_errmsg(db), sqlstmt);                         \
    goto label;                                                         \
 }

#define CHECK_BIND(rv, rc, db, label)                                   \
if (is_sqlite_error(rc)) {                                              \
    rv = conv_sqlite_errcode(rc);                                       \
    LOG_ERROR("Failed to bind value in prepared stmt: %d, %s", rv, sqlite3_errmsg(db)); \
    goto label;                                                         \
}

#define CHECK_STEP(rv, rc, db, label)                                   \
if (is_sqlite_error(rc)) {                                              \
    rv = conv_sqlite_errcode(rc);                                       \
    LOG_ERROR("Failed to execute prepared stmt: %d, %s", rv, sqlite3_errmsg(db)); \
    goto label;                                                         \
}

#define CHECK_FINALIZE(rv, rc, db, label)                               \
if (is_sqlite_error(rc)) {                                              \
    rv = conv_sqlite_errcode(rc);                                       \
    LOG_ERROR("Failed to finalize prepared stmt: %d, %s", rv, sqlite3_errmsg(db)); \
    goto label;                                                         \
}

#define FINALIZE_STMT(rv, rc, db, stmt)                                 \
if (stmt) {                                                             \
    rc = sqlite3_finalize(stmt);                                        \
    if(rc != SQLITE_OK) {                                               \
        LOG_ERROR("Failed finalizing statement:%d, %s", rc, sqlite3_errmsg(db)); \
        rv = conv_sqlite_errcode(rc);                                   \
    }                                                                   \
    stmt = NULL;                                                        \
}

#define CHECK_EXEC(rv, rc, db, label)                                   \
if (is_sqlite_error(rc)) {                                              \
    rv = conv_sqlite_errcode(rc);                                       \
    LOG_ERROR("Failed to exec stmt: %d, %s", rv, sqlite3_errmsg(db));   \
    goto label;                                                         \
}

#define CHECK_RV(rv, rc, label)                 \
if (rc != DATASETDB_OK) {                       \
    rv = rc;                                    \
    goto label;                                 \
}

#endif /* __MEDIA_DB_UTIL_HPP__ */

