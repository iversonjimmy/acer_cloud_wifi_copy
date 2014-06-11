//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "db_util_check_macros.hpp"
#include "sqlite3.h"

bool dbUtil_isSqliteError(int ec)
{
    return ec != SQLITE_OK && ec != SQLITE_ROW && ec != SQLITE_DONE;
}

#define SQLITE_ERROR_BASE 0
int dbUtil_mapSqliteErrCode(int ec)
{
    return SQLITE_ERROR_BASE - ec;
}
