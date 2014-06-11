//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef UTIL_OPEN_DB_HANDLE_HPP_
#define UTIL_OPEN_DB_HANDLE_HPP_

#include "sqlite3.h"
#include <string>

#if defined(CLOUDNODE) || defined(VPL_PLAT_IS_WINRT) || defined(ANDROID)
#define SQLITE_TEMP_DIR_REQUIRED 1
#else
#define SQLITE_TEMP_DIR_REQUIRED 0
#endif // defined(CLOUDNODE) || defined(VPL_PLAT_IS_WINRT) || defined(ANDROID)

int Util_InitSqliteTempDir(const std::string& tempPath);

/// Util to make the opening of sqlite3 more consistent across the code-base.
/// @param dbpath Path to the sqlite3 db file.
/// @param write When true, read/write access will be allowed.  When false, only read
///        access will be allowed.
/// @param create When true, will create the db if it does not exist.
/// @result dbHandle_out The handle to the sqlite3 db.  If successfully returned,
///         the handle should be closed by passing it to sqlite3_close()
int Util_OpenDbHandle(const std::string& dbpath,
                      bool write,
                      bool create,
                      sqlite3 **dbHandle_out);


#endif /* UTIL_OPEN_DB_HANDLE_HPP_ */
