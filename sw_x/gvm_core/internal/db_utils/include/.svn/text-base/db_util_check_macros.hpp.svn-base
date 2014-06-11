//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef DB_UTIL_CHECK_MACROS_HPP_07_21_2013_
#define DB_UTIL_CHECK_MACROS_HPP_07_21_2013_

#include "gvm_errors.h"

bool dbUtil_isSqliteError(int ec);
int dbUtil_mapSqliteErrCode(int ec);

#define DB_UTIL_CHECK_DB_HANDLE(result, db, lbl)                   \
do{ if (db==NULL) {                                                \
      result = DB_UTIL_ERR_INTERNAL;                               \
      LOG_ERROR("No db handle.");                                  \
      goto lbl;                                                    \
}}while(0)

#define DB_UTIL_CHECK_PREPARE(dberr, result, db, lbl)              \
do{ if (dbUtil_isSqliteError(dberr)) {                             \
      result = dbUtil_mapSqliteErrCode(dberr);                     \
      LOG_ERROR("Failed to prepare SQL stmt: %d, %s",              \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define DB_UTIL_CHECK_BIND(dberr, result, db, lbl)                 \
do{ if (dbUtil_isSqliteError(dberr)) {                             \
      result = dbUtil_mapSqliteErrCode(dberr);                     \
      LOG_ERROR("Failed to bind value in prepared stmt: %d, %s",   \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define DB_UTIL_CHECK_STEP(dberr, result, db, lbl)                 \
do{ if (dbUtil_isSqliteError(dberr)) {                             \
      result = dbUtil_mapSqliteErrCode(dberr);                     \
      LOG_ERROR("Failed to execute prepared stmt: %d, %s",         \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define DB_UTIL_CHECK_ROW_EXIST(dberr, result, db, lbl)            \
do{ if (dberr!=SQLITE_ROW) {                                       \
      result = DB_UTIL_ERR_ROW_NOT_FOUND;                          \
      goto lbl;                                                    \
}}while(0)

#define DB_UTIL_CHECK_FINALIZE(dberr, result, db, lbl)             \
do{ if (dbUtil_isSqliteError(dberr)) {                             \
      result = dbUtil_mapSqliteErrCode(dberr);                     \
      LOG_ERROR("Failed to finalize prepared stmt: %d, %s",        \
                result, sqlite3_errmsg(db));                       \
      goto lbl;                                                    \
}}while(0)

#define DB_UTIL_CHECK_ERR(err, result, lbl)     \
do{ if ((err) != 0) {                           \
      result = err;                             \
      goto lbl;                                 \
}}while(0)

#define DB_UTIL_CHECK_RESULT(result, lbl)       \
do{ if ((result) != 0) {                        \
      goto lbl;                                 \
}}while(0)


#endif // DB_UTIL_CHECK_MACROS_HPP_07_21_2013_
