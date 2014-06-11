//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplexTest.h"
#include "vplTest_suite_common.h"
#include "vplex_trace.h"

#include <setjmp.h>
#include <signal.h>
#include <assert.h>
#include <getopt.h>

static VPLTest_test vplTest_allTests[] = {
    // Function,                      Test "name",         Included in "VPLTest_RunAll"?
    { testVPLMath,                    "Math",                    VPL_TRUE },
    { testVPLMemHeap,                 "MemHeap",                 VPL_TRUE },
    { testVPLSafeSerialization,       "SafeSerialization",       VPL_TRUE },
    { testVPLSerialization,           "Serialization",           VPL_TRUE },
    { testVPLXmlReader,               "XmlReader",               VPL_TRUE },
    { testVPLXmlReaderUnescape,       "XmlReaderUnescape",       VPL_TRUE },
    { testVPLXmlWriter,               "XmlWriter",               VPL_TRUE },
    // temperarily disable it on iOS and winrt until test urls are ready
#if defined(LINUX) || (defined(_WIN32) ) || defined(ANDROID)
    { testVPLHttp,                    "Http",                    VPL_TRUE },
#endif
#if !defined(_WIN32) && !defined(ANDROID)
    { testVPLWstring,                 "Wstring",                 VPL_TRUE },
#endif
};

static jmp_buf vplTestSignalExit;

//--------------------------
// Config
//--------------------------

const char* server_url;
const char* branch;
const char* product;

#if !defined(_WIN32) && !defined(ANDROID)
static void signal_handler(int sig)
{
    printf("*** Received signal %d during %s!\n", sig, vplTest_curTestName);
    longjmp(vplTestSignalExit, 1);
}
#endif

static void usage(int argc, char* argv[])
{
    printf("Usage: %s [options] [test names]\n", argv[0]);
    printf("Options:\n");
    printf(" -U --test-server-url URL   TEST SERVER URL (%s)\n",
            server_url);
    printf(" -b --branch BRANCH         Branch (%s)\n",
            branch);
    printf(" -p --product PRODUCT       Product (%s)\n",
            product);

}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"test-server-url", required_argument, 0, 'U'},
        {"branch", required_argument, 0, 'b'},
        {"product", required_argument, 0, 'p'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "U:b:p",
                            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {

        case 'U':
            rv += 2;
            server_url = optarg;
            break;

        case 'b':
            rv += 2;
            branch = optarg;
            break;

        case 'p':
            rv += 2;
            product = optarg;
            break;

        default:
            usage(argc, argv);
            rv = -1;
            break;
        }
    }

    return rv;
}

#define LAB_DOMAIN  "lab18.routefree.com"

#if defined(VPL_PLAT_IS_WINRT)
extern VPLFile_handle_t hVplTestLog;

static void winrtLogCallback(
        VPLTrace_level level,
        const char* sourceFilename,
        int lineNum,
        const char* functionName,
        VPL_BOOL appendNewline,
        int grp_num,
        int sub_grp,
        const char* fmt,
        va_list ap)
{
    //share the file with VPLTEST_LOGWRITE
    //see vpl/tests/src/vplTest_common.c

    if (VPLFile_IsValidHandle(hVplTestLog)) {
        va_list apcopy; // Need to copy first, since we may need it again later.

        char buff[5120+1]={0};

        va_copy(apcopy, ap);
        //va_start (apcopy, fmt);
        vsnprintf(buff, 5120, fmt, apcopy);
        va_end (apcopy);

        VPLFile_Write(hVplTestLog, buff, strlen(buff));
        VPLFile_Write(hVplTestLog, "\n", 1);
    }

    UNUSED(sourceFilename);
    UNUSED(lineNum);
    UNUSED(functionName);
    UNUSED(appendNewline);
    UNUSED(grp_num);
    UNUSED(sub_grp);

}
#endif  //#if defined(VPL_PLAT_IS_WINRT)

#if defined(VPL_PLAT_IS_WINRT) || defined(ANDROID)
int testVPLex(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    // volatile to suppress compile error: variable 'parsed_arg_count' might be clobbered by 'longjmp' or 'vfork'
    volatile int parsed_arg_count;
    if ((parsed_arg_count = parse_args(argc, argv)) < 0) {
        return -1;
    }

#if !defined(_WIN32) && !defined(ANDROID)
    // catch SIGABRT
    __sighandler_t sig_rv = signal(SIGABRT, signal_handler);
    VPLTEST_ENSURE_EQUAL(sig_rv, VPL_VOID_PTR_NULL, "%p", "signal(SIGABRT, signal_handler)");
#endif
    if (setjmp(vplTestSignalExit) == 0) { // initial invocation of setjmp

#if defined(VPL_PLAT_IS_WINRT)
        VPLTrace_Init(winrtLogCallback);
#else
        VPLTrace_Init(NULL);
#endif

        VPLTEST_START("_Init");
        VPLTEST_CALL_AND_CHK_RV(VPL_Init(), VPL_OK);
        VPLTEST_END();

        // If no command-line args, run all tests.
        // If we have command-line args, run tests whose VPL-module name
        // matches each arg.
        if (argc - parsed_arg_count == 1) {
            VPLTest_RunAll(vplTest_allTests, ARRAY_ELEMENT_COUNT(vplTest_allTests));
        } else {
            VPLTest_RunByName(argc - parsed_arg_count, &argv[parsed_arg_count],
                    vplTest_allTests, ARRAY_ELEMENT_COUNT(vplTest_allTests));
        }

        VPLTEST_START("_Quit");
        VPLTEST_CALL_AND_CHK_RV(VPL_Quit(), VPL_OK);
        VPLTEST_END();
    }

    if (vplTest_getTotalErrCount() > 0) {
        VPLTEST_LOG("*** TEST SUITE FAILED, %d error(s)\n", vplTest_getTotalErrCount());
    }
    VPLTEST_LOG("\nCLEAN EXIT\n");
    return vplTest_getTotalErrCount();
}
