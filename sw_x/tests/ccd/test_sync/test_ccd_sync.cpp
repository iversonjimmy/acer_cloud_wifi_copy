//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// Command-line "menu" for testing CCD.  Asks for a username and password,
// logs in to infra, and displays download progress until all titles have
// finished or the program is interrupted, at which point it will log out
// and exit.

#include <ccdi.hpp>

#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include "vpl_th.h"
#include "gvm_file_utils.hpp"
#ifdef _MSC_VER
#include <vpl_user.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <iostream>
#include <cstdio>
#include <csignal>
#include <string>
#include <vector>
#include <set>

#include <log.h>
#include <vpl_plat.h>


#if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
int test_ccd_sync(int argc, const char ** argv)
#else
int main(int argc, char ** argv)
#endif
{
#ifndef VPL_PLAT_IS_WINRT
    LOGInit("test_ccd_sync", NULL);
    LOGSetMax(0); // No limit
#endif
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);
    VPL_Init();
    LOG_INFO("TC_RESULT=PASS ;;; TC_NAME=old_ccd_sync_test(Deprecated)\n");
    return 0;
}
