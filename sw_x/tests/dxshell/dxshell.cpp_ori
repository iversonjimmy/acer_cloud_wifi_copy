//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>
#include <set>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "gvm_file_utils.h"
#include <log.h>

using namespace std;

//#define DEFAULT_CCD_PATH "C:\\MSFT_Program_Files_(x86)\\Acer\\Acer Cloud\\ccd.exe"
//#define DEFAULT_CCD_APP_DATA_PATH "C:\\Users\\<User_name>\\AppData\\Local\\iGware\\SyncAgent"

// Default values for MSA Cloud upload test.
int testInstanceNum = 0;

static int dxmain(int argc, const char *argv[])
{
	cout << "enter dxmain function" << endl;
    LOGInit("dxshell", NULL);
    LOGSetMax(0);
    int rv = 0;
    std::string dxroot;
    int argvIdx = 1;

    VPL_Init();
    VPLHttp2::Init();

    // Initialize default test root path
    rv = getDxRootPath(dxroot);
    if (rv != 0) {
        LOG_ERROR("Fail to set %s root path", argv[0]);
        goto exit;
    }

    LOG_INFO("==== ACT dxshell ====\n");
    /// Check Diagnostic Environment Is Correct
    rv = Util_CreatePath(dxroot.c_str(), true);
    if (rv != VPL_OK) {
        LOG_ERROR("Fail to create directory %s, rv = %d", dxroot.c_str(), rv);
        return -1;
    }

    VPLFS_dir_t testDir;
    if (VPLFS_Opendir(dxroot.c_str(), &testDir) != VPL_OK) {
        LOG_ERROR("Cannot access test folder %s", dxroot.c_str());
        return -1;
    }
    VPLFS_Closedir(&testDir);

    // Multiple CCD instance support is not currently exposed to common users. It is
    // used for stress testing infra-structure and automated test.
    // Note that you MUST set the testInstanceNum within ${appDataPath}/conf/ccd.conf,
    // otherwise CCD will be listening on the wrong named socket. (getDefaultConfig(),
    // which is called by set_domain() will do this already.)
    // Parse instance ID if any.
    if (argc > 1) {
        if (argv[1][0] == '-') {
            switch (argv[1][1]) {
                case 'i':
                    argvIdx = 2;
                    argc--;
                    if (argc > 2) {
                        testInstanceNum = atoi(argv[argvIdx++]);
                        argc--;
                    } else {
                        testInstanceNum = 0;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    proc_subcmd(dxroot, argc - 1, &argv[argvIdx]);

 exit:
    return 0;
}

#if defined(WIN32)
int wmain(int argc, const wchar_t *argv[])
{
	LOG_ALWAYS("win32 main function start"); // jimmy
    int exitcode = 0;
    const char **dx_argv = NULL;
    char **argv_utf8 = new (std::nothrow) char* [argc + 1];
    if (!argv_utf8) {
        goto end;
    }

    for (int i = 0; i < argc; i++) {
        int rc = _VPL__wstring_to_utf8_alloc(argv[i], &argv_utf8[i]);
        if (rc != VPL_OK) {
            fprintf(stderr, "Failed to convert argv[%d]\n", i);  // our logger is not yet ready so call stdio
            exitcode = -1;
            goto end;
        }
    }

    argv_utf8[argc] = NULL;
    dx_argv = (const char **)(void *)&argv_utf8[0];

    exitcode = dxmain(argc, dx_argv);

 end:
    delete [] argv_utf8;
    return exitcode;
}
#else
int main(int argc, const char* argv[])
{
	cout << "enter main function" << endl; // jimmy
	LOG_ALWAYS("main function start"); // jimmy
    int exitcode = 0;
    exitcode = dxmain(argc, argv);
    return exitcode;
}
#endif
