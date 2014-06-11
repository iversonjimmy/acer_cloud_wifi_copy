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

#include "ts_test_network_test.hpp"
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

#define SET_TARGET_MACHINE(tc_name, alias, rc) \
    do { \
        const char *testStr = "SetTargetMachine"; \
        rc = set_target_machine(alias); \
        if (rc == VPL_ERR_FAIL) { \
            CHECK_AND_PRINT_RESULT(tc_name, testStr, rc); \
        } \
    } while (0)

/// AutoTest variables
static const char* test_str = "NetworkTest";
static int disconn_delay_sec = 3;
static bool use_remote_agent = false;
static const char* total_size_default = "20480"; // KB
static const char* total_size = total_size_default;
static const char* domain_default = "pc-int.igware.net";
static const char* domain = domain_default;
static const char* username = 0;
static const char* password_default = "password";
static const char* password = password_default;

static void usgae(int argc, const char *argv[])
{
    printf("\n");
    printf ("Usage: dxshell TsTest %s -u <username> [options]\n", argv[0]);
    printf("\n");
    printf("    Test streaming with network drop environment.\n");
    printf("\n");
    printf("    The only required argument is username.  Other arguments can be specified\n");
    printf("    to override defaults.\n");
    printf("\n");
    printf("Options:\n");
    printf(" -v --verbose                 Raise verbosity one level. Repeat up to %d times (more has no effect).\n", logEnableLevel);
    printf(" -q --quiet                   Lower verbosity one level. Repeat up to %d times (more has no effect).\n",
                                                                                minVerbosityLevel - logEnableLevel);
    printf(" -s --size  <#>               Total transfered size(KB). Default %s KB.\n", total_size_default);
    printf(" -t --disconn-delay <#>       Interval of seconds for network drop %d\n", disconn_delay_sec);
    printf(" -o --domain                  default is \"%s\".\n", domain_default);
    printf(" -u --username                Required argument.\n");
    printf(" -p --password                default is \"password\".\n");
    printf(" -r --useremoteagent          Use dx_remote_agent\n");
    printf("\n");
}

static int parse_args(int argc, const char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"size", required_argument, 0, 's'},
        {"disconn-delay", required_argument, 0, 't'},
        {"domain", required_argument, 0, 'o'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"useremoteagent", no_argument, 0, 'r'},
        {0,0,0,0}
    };

    while (true) {
        int option_index = 0;

        int option = getopt_long(argc, (char * const *)argv, "vqs:t:o:u:p:r", long_options, &option_index);

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
        case 's':
            total_size = optarg;
            break;
        case 'o':
            domain = optarg;
            break;
        case 'u':
            username = optarg;
            break;
        case 'p':
            password = optarg;
            break;
        case 't':
            {
                disconn_delay_sec = atoi(optarg);
            }
            break;
        case 'r':
            use_remote_agent = true;
            break;
        default:
            rv = -1;
            break;
        }
    }

    if (rv == 0) {

        if (!username) {
            rv = -1;
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

static VPLThread_return_t network_thread_main (VPLThread_arg_t arg)
{
    bool* stop_th = (bool*)arg;
    int rv = 0;
    LOG_ALWAYS("network thread is starting");

    // This is temporary, we use report_netwokr_connected to simulate network dropped
    // Once switchbox is setup, we don't wait for ccd status
    {
        u32 retry_max = 600;
        bool initDone = false;
        while (retry_max-- && !*stop_th) {
            initDone = isInitDone();
            if (initDone) {
                break;
            }
            VPLThread_Sleep(VPLTime_FromSec(1));
        }
        if (!initDone) {
            LOG_ERROR("Fail to wait ccd to be executed %d", rv);
            rv = -1;
            goto fail;
        }
    }

    while (!*stop_th) {

        // This is temporary, we use report_netwokr_connected to simulate network dropped
        // Once switchbox is setup, we don't need to call SET_TARGET_MACHINE
        if (use_remote_agent) {
            SET_TARGET_MACHINE(test_str, "MD", rv);
        }

        rv = drop_all_connections();
        if (rv) {
            // A ccd instance probably get terminated by echo test,
            // a report_network_connected behavior might failed because of IPC error.
            // Since this is a temporary solution, we leave it as a WARNING
            LOG_WARN("Fail to call drop_all_connections() %d", rv);
            rv = 0;
        }
        LOG_ALWAYS("Connections dropped");
        VPLThread_Sleep(VPLTime_FromSec(disconn_delay_sec));
    }

fail:
    CHECK_AND_PRINT_RESULT(test_str, "ConfigureRouter", rv);
exit:
    LOG_ALWAYS("network thread is exiting");
    return (VPLThread_return_t)rv;
}


int network_test_main(int argc, const char* argv[])
{
    int rv = VPL_OK;
    VPLThread_t th;
    bool stop_th = false;
    bool thr_started = false;

    if (parse_args(argc, argv) != 0) {
        rv = VPL_ERR_INVALID;
        goto exit;
    }

    // On entry, log level is set by setDebugLevel(g_defaultLogLevel) i.e. LOG_LEVEL_ERROR.
    // At least SET_DOMAIN and possibly other utilities also set log level to g_defaultLogLevel.
    // So it doesn't do any good to call setDebugLevel() before those utilities.
    // Could change g_defaultLogLevel, but will attempt not to do that.

    setDebugLevel(logEnableLevel);
    LOG_ALWAYS("setDebugLevel(%d)", logEnableLevel);

    {
        // Fork network playing around thread
        rv = VPLThread_Create(&th, network_thread_main, &stop_th, NULL, "TS_Write_thread");
        if (rv) {
            LOG_ERROR("Fork thread failed with %d", rv);
            CHECK_AND_PRINT_RESULT(test_str, "ConfigureRouter", rv);
        } else {
            thr_started = true;
        }
    }

    {
        optind = 1;

        const char *testArg[] = { "TsTest", use_remote_agent ? "echo2": "echo",
                "-u", username,
                "-p", password,
                "-o", domain,
                "-s", "1024",
                "-c", total_size,
                "-e"};
        rv = tstest_commands(13, testArg);
        if (rv) {
            LOG_ERROR("EchoTest failed with %d", rv);
        }
        CHECK_AND_PRINT_RESULT(test_str, "EchoTest", rv);
    }

exit:
    if (thr_started) {
        int rc = VPL_OK;
        VPLThread_return_t return_v = 0;
        stop_th = true;
        rc = VPLThread_Join(&th, &return_v);
        if (rc) {
            LOG_ERROR("Join thread failed with %d", rc);
        }
        rv = rv == VPL_OK ? (int)return_v :rv;
        rv = rv == VPL_OK ? rc :rv;
    }
    resetDebugLevel();
    return rv;
}
