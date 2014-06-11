/*
 *  Copyright 2014 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#include "ts_test_configure_router.hpp"
#include "ts_test.hpp"

#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"

#include "vpl_conv.h"
#include "vpl_thread.h"
#include "vpl_th.h"
#include "vplex_math.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include "ccdconfig.hpp"
#include "log.h"
#include "gvm_errors.h"

#if defined(WIN32)
#include <getopt_win.h>
#else
#include <getopt.h>
#endif


#include <map>
#include <string>

// Logs with LOGLevel enum value less than logEnableLevel will be disabled in the test application.
// i.e.; Decrementing logEnableLevel increases verbosity of test output.
//       Incrementing logEnableLevel decreases verbosity of test output.
// Examples: logEnableLevel == LOG_LEVEL_INFO
//                             enabled:  INFO, WARN, ERROR, CRITICAL, ALWAYS
//                             disabled: DEBUG, TRACE
//           logEnableLevel == LOG_LEVEL_ERROR
//                             enabled:  ERROR, CRITICAL, ALWAYS
//                             disabled: DEBUG, TRACE, INFO, WARN
// Multiple -v options can decrease logEnableLevel until it reaches 0 (i.e. LOG_LEVEL_TRACE).
// Multiple -q options can increase logEnableLevel until it reaches minVerbosityLevel.
// Example: minVerbosityLevel = LOG_LEVEL_ERROR means TRACE, DEBUG, INFO, AND WARN logs can
//          be enabled/disabled, but ERROR, CRITICAL, and ALWAYS logs can not be disabled.
static int logEnableLevel = (int) LOG_LEVEL_INFO;
static int minVerbosityLevel = (int) LOG_LEVEL_ERROR;

#define CHECK_AND_PRINT_RESULT(testsuite, subtestname, result) {              \
        if (result < 0) {                                       \
            LOG_ERROR("%s fail rv (%d)", subtestname, result);     \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=TsTest_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
            goto exit;                                          \
        } else {                                                \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=TsTest_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
        }                                                       \
}

/// ConfigRoute variables
static bool dropallconn = true;
static const char* test_str = "ConfigureRouter";

static void usgae(int argc, const char *argv[])
{
    printf("\n");
    printf ("Usage: dxshell TsTest %s [options]\n", argv[0]);
    printf("\n");
    printf("    Configure programmable router box.\n");
    printf("\n");
    printf("Options:\n");
    printf(" -v --verbose                 Raise verbosity one level. Repeat up to %d times (more has no effect).\n", logEnableLevel);
    printf(" -q --quiet                   Lower verbosity one level. Repeat up to %d times (more has no effect).\n",
                                                                                minVerbosityLevel - logEnableLevel);
    printf(" -D --drop-all-conn <#>            Number of data blocks to write, read, and check. Default %u.\n", dropallconn);
    printf("\n");
}

static int parse_args(int argc, const char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"dropallconn", no_argument, 0, 'D'},
        {0,0,0,0}
    };

    while (true) {
        int option_index = 0;

        int option = getopt_long(argc, (char * const *)argv, "vqD", long_options, &option_index);

        if (option == -1)
            break;

        switch(option) {
        case 'v':
            if (logEnableLevel > 0) {
                --logEnableLevel;
            }
            break;
        case 'q':
            if (logEnableLevel < minVerbosityLevel) {
                ++logEnableLevel;
            }
            break;
        case 'D':
            dropallconn = true;
            break;
        default:
            rv = -1;
            break;
        }
    }

    if (checkHelp(argc, argv)) {
        rv = -1;
    }

    if (rv != 0) {
        usgae (argc, argv);
    }

    return rv;
}

int drop_all_connections()
{
    int rv = VPL_OK;

    ccd::UpdateSystemStateInput request;
    ccd::UpdateSystemStateOutput response;

    LOG_ALWAYS("Executing..");

    request.set_report_network_connected(true);
    rv = CCDIUpdateSystemState(request, response);
    if (rv != 0) {
        LOG_ERROR("report network connected failed:%d", rv);
        goto exit;
    }
    LOG_ALWAYS("Network connectivity reported");
exit:
    return rv;
}

int configure_router_main(int argc, const char* argv[])
{
    int rv = VPL_OK;

    if (parse_args(argc, argv) != 0) {
        rv = VPL_ERR_INVALID;;
        goto exit;
    }

    // On entry, log level is set by setDebugLevel(g_defaultLogLevel) i.e. LOG_LEVEL_ERROR.
    // At least SET_DOMAIN and possibly other utilities also set log level to g_defaultLogLevel.
    // So it doesn't do any good to call setDebugLevel() before those utilities.
    // Could change g_defaultLogLevel, but will attempt not to do that.

    setDebugLevel(logEnableLevel);
    LOG_ALWAYS("setDebugLevel(%d)", logEnableLevel);

    if (dropallconn) {
        LOG_WARN("IT IS NECESSARY TO SET \"ts2TestNetworkEnv = 1\" IN ccd.conf");
        LOG_WARN("THIS COMMAND ONLY CALLS ReportNetworkConnected FOR 3.1 RELEASE");
        rv = drop_all_connections();
        if (rv) {
            LOG_ERROR("Fail to call drop_all_connections %d", rv);
        }
        CHECK_AND_PRINT_RESULT(test_str, "DropAllConnections", rv);
    }

exit:
    resetDebugLevel();
    return rv;
}
