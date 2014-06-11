//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifdef WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include "ccd_core.h"
#include "netman.hpp"
#include "log.h"
#include <stddef.h>
#include <vplex_http2.hpp>
#include <google/protobuf/stubs/common.h>

#ifdef _MSC_VER
#include <atlbase.h>
/// Nothing references this, but it appears to be required instantiation.
CComModule _Module;
#endif

/*
 * WINDOWS ONLY
 * To get the command-line args in UTF16, 
 * we will use wmain(), a Microsoft extension, as the entry point.
 * Unfortunately, mingw doesn't support wmain.
 * This means that mingw-compiled ccd.exe won't support non-ascii chars on the command-line.
 */

#if defined(WIN32)
static
#endif
int
main(int argc, char** argv)
{
    const char* localAppDataPath = NULL;
    const char* osUserId = NULL;
    const char* titleId = NULL;
    if (argc > 1 && strlen(argv[1]) > 0) {
        localAppDataPath = argv[1];
        if (argc > 2 && strlen(argv[2]) > 0) {
            osUserId = argv[2];
        }
#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
        if (argc > 4 && strlen(argv[3]) > 0 && strlen(argv[4]) > 0) {
            // Bug 5997: additional params to present 
            // anonymous handle and its size
            int rv = CCDHandleStartParams(argv[3], argv[4]);
            if (rv != 0) {
                return 255;
            }
        }
        if (argc > 5 && strlen(argv[5]) > 0) {
            NetMan_SetGlobalAccessDataPath(argv[5]);
        }
        if (argc > 6 && strlen(argv[6]) > 0) {
            titleId = argv[6];
        }
#else
        if (argc > 3 && strlen(argv[3]) > 0) {
            titleId = argv[3];
        }
#endif
    }
    int rv = CCDStart("ccd", localAppDataPath, osUserId, titleId);
    if (rv == 0) {
        CCDWaitForExit();
    }

    // Check if we should do a full shutdown (only useful if we want to run valgrind).
    if (getenv("CCD_FULL_SHUTDOWN") != NULL) {
        LOG_INFO("Performing full shutdown; CCD main rv=%d", rv);
        // TODO: This sleep only reduces the occurrence of race conditions; it would be more
        //   reliable to wait for all threads to exit.
        VPLThread_Sleep(VPLTime_FromSec(5));
        VPLHttp2::Shutdown();
        google::protobuf::ShutdownProtobufLibrary();
    }
    
    LOG_INFO("Exiting CCD process main(); rv=%d", rv);

    //------------------------------------
    // Since we don't explicitly close the log file, it looks like some of the logs may not make
    // it to the disk.  Attempt to fix that, for Linux at least:
    //------------------------------------
    // TODO: refactor to VPL
    fflush(NULL);
#ifndef _WIN32
    sync();
#endif
    //------------------------------------

    // Since the return code usually ends up "mod 256", remap it to avoid ever returning 0 when
    // something actually failed.
    return (rv != 0) ? 255 : 0;
}

#if defined(WIN32) 
int wmain(int argc, wchar_t *argv[])
{
    int exitcode = 0;
    char **argv_utf8 = new char* [argc + 1];
    for (int i = 0; i < argc; i++) {
        int rc = _VPL__wstring_to_utf8_alloc(argv[i], &argv_utf8[i]);
        if (rc != VPL_OK) {
            fprintf(stderr, "Failed to convert argv[%d]\n", i);  // our logger is not yet ready so call stdio
            exitcode = -1;
            goto end;
        }
    }
    argv_utf8[argc] = NULL;
    exitcode = main(argc, argv_utf8);

 end:
    delete [] argv_utf8;
    return exitcode;
}
#endif
