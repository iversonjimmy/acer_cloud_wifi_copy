//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef DB_UTIL_ACCESS_MACROS_HPP_07_21_2013_
#define DB_UTIL_ACCESS_MACROS_HPP_07_21_2013_

#include "sqlite3.h"
#include "log.h"

#define DB_UTIL_SQLITE3_TRUE 1
#define DB_UTIL_SQLITE3_FALSE 0

// Max value is still 2^63 since sqlite3 is inherently signed.
#define DB_UTIL_GET_SQLITE_U64(sqlite3stmt, int64_out, resIndex, rv, lbl)           \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            int64_out = sqlite3_column_int64(sqlite3stmt, d_resIndex);              \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

// Null is a valid value.
#define DB_UTIL_GET_SQLITE_U64_NULL(sqlite3stmt, valid_out, int64_out, resIndex, rv, lbl) \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            valid_out = true;                                                       \
            int64_out = sqlite3_column_int64(sqlite3stmt, d_resIndex);              \
        }else if(d_sqlType == SQLITE_NULL){                                         \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

#define DB_UTIL_GET_SQLITE_STR(sqlite3stmt, str_out, resIndex, rv, lbl)             \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_TEXT) {                                              \
            str_out.assign(reinterpret_cast<const char*>(                           \
                             sqlite3_column_text(sqlite3stmt, d_resIndex)));        \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_TEXT, d_resIndex);                          \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

// Null is a valid value.
#define DB_UTIL_GET_SQLITE_STR_NULL(sqlite3stmt, valid_out, str_out, resIndex, rv, lbl)  \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_TEXT) {                                              \
            valid_out = true;                                                       \
            str_out.assign(reinterpret_cast<const char*>(                           \
                             sqlite3_column_text(sqlite3stmt, d_resIndex)));        \
        }else if(d_sqlType == SQLITE_NULL){                                         \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_TEXT, d_resIndex);                          \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

#define DB_UTIL_GET_SQLITE_BOOL(sqlite3stmt, bool_out, resIndex, rv, lbl)           \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            bool_out = (sqlite3_column_int64(sqlite3stmt, d_resIndex) != 0);        \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

#define DB_UTIL_GET_SQLITE_BOOL_NULL(sqlite3stmt, valid_out, bool_out, resIndex, rv, lbl) \
    do{ int d_resIndex = resIndex;                                                  \
        int d_sqlType = sqlite3_column_type(sqlite3stmt, d_resIndex);               \
        if(d_sqlType == SQLITE_INTEGER) {                                           \
            valid_out = true;                                                       \
            bool_out = (sqlite3_column_int64(sqlite3stmt, d_resIndex) != 0);        \
        }else if(d_sqlType == SQLITE_NULL){                                         \
        }else{                                                                      \
            LOG_ERROR("Bad column type %d.  Expected type %d at index:%d",          \
                      d_sqlType, SQLITE_INTEGER, d_resIndex);                       \
            rv = -1;                                                                \
            goto lbl;                                                               \
        }                                                                           \
    }while(0)

#endif // DB_UTIL_ACCESS_MACROS_HPP_07_21_2013_
