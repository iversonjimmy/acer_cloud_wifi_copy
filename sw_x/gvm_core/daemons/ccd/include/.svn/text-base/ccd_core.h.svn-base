//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCD_CORE_H__
#define __CCD_CORE_H__

#include "vpl_types.h"

//============================================================================
/// @file
/// Entry point for starting the CCD services in the same process.
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
/// Handle params from command line arguments.
/// Currently, trustees will be passed for file access control.
/// This should be called before CCDStart.
/// @param pszCmdLine Handle(INT64) of anonymous shared memory.
/// @param pszMemSize Size of anonymous shared memory.
int CCDHandleStartParams(const char* pszCmdLine, const char* pszMemSize);
#endif

/// Start the CCD services.
/// This should only be called by a single thread (but that thread is permitted to call it again
/// after calling #CCDWaitForExit()).
/// @note Additional requirement for Windows desktop mode only:  After calling this, CCD will not be fully
///     functional until the same thread calls #CCDWaitForExit().
/// @param processName Hosting process name; used for logging.
/// @param localAppDataPath Root directory for this process under which all internal files will be written; any
///     trailing slashes will be removed.  Can be NULL to use a default.
/// @param osUserId Specify the osUserId for the CCDI socket.  If NULL, we will use #VPL_GetOSUserId().
/// @param titleId TitleId of the application hosting CCD.  We need this to let IAS/ANS distinguish multiple
///     instances of CCD running under the same virtual device.  Can be NULL to use a default on platforms
///     where we share a single instance of CCD across multiple apps.
int CCDStart(const char* processName, const char* localAppDataPath, const char* osUserId, const char* titleId);

/// Retrieve the ProcessName for this instance of CCD.
const char* CCDGetProcessName();

#ifndef VPL_PLAT_IS_WINRT
/// Retrieve the OsUserId for this instance of CCD.
/// Should never be NULL.
const char* CCDGetOsUserId();
#endif

#ifdef ANDROID
/// Open only for Android to change config and log path.  
/// @param brandName force CCD to read config path under "/sdcard/AOP/$(brandName)/conf".
///    Let log file dump to path ""/sdcard/AOP/$(brandName)/logs".
///    If NULL, CCD will read default conf path "/sdcard/AOP/AcerCloud/conf"
///    And log file will locate at default path "/sdcard/AOP/AcerCloud/logs"
void CCDSetBrandName(const char* brandName);
#endif

/// Retrieve the titleId for this instance of CCD.
/// Should never be NULL.
const char* CCDGetTitleId();

/// Requests CCD to shut down and returns immediately.
/// You must call #CCDWaitForExit() and wait for it to return before calling #CCDStart() again.
void CCDShutdown();

/// Block until the CCD services thread is finished.
/// This should only be called by a single thread (but that thread is permitted to call it multiple times).
/// @note Additional requirement for Windows desktop mode only:  You must dedicate a thread to block within this
///     function after calling #CCDStart().  The thread must be the same thread that called #CCDStart().
void CCDWaitForExit();

#ifdef __cplusplus
}
#endif

#endif // include guard
