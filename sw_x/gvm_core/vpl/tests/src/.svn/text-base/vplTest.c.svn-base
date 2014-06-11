//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplTest.h"
#include "vplTest_suite_common.h"
#include "vplu_debug.h"

#include <setjmp.h>
#include <signal.h>
#include <assert.h>

// TODO commented out VPLEvent code needs libvplgame

static VPLTest_test vplTest_allTests[] = {
    // Function,         Test "name",  Included in "VPLTest_RunAll"?
    { testVPLPlatformCompliance, "PlatformCompliance", VPL_TRUE },
    { testVPLPlat,       "Plat",       VPL_TRUE },
    { testVPLConv,       "Conv",       VPL_TRUE },
#   if !defined(ANDROID) && !defined(_WIN32) && !defined(__CLOUDNODE__)
    { testVPLDL,         "DL",         VPL_TRUE },
#   endif
    //{ testVPLTime,       "Time",       VPL_TRUE },
    { testVPLThread,     "Thread",     VPL_TRUE },
    { testVPLMutex,      "Mutex",      VPL_TRUE },
    { testVPLCond,       "Cond",       VPL_TRUE },
    { testVPLSem,        "Sem",        VPL_TRUE },
    { testVPLSlimRWLock, "SlimRWLock", VPL_TRUE },
#   if !defined(ANDROID)
    { testVPLSocket,     "Socket",     VPL_TRUE },
#   endif
    { testVPLFS,         "FS",         VPL_TRUE },
    { testVPLNetwork,    "Network",    VPL_TRUE },
#   if defined(LINUX) || defined(__CLOUDNODE__)
    { testVPLShm,        "Shm",        VPL_TRUE },
#   endif
};

static jmp_buf vplTestSignalExit;

#if !defined(_WIN32) && !defined(ANDROID)
static void signal_handler(int sig)
{
    printf("*** Received signal %d during %s!\n", sig, vplTest_curTestName);
    longjmp(vplTestSignalExit, 1);
}
#endif

static void testInvalidParameters(void)
{
    VPLTEST_CALL_AND_CHK_RV(VPL_Quit(), VPL_ERR_NOT_INIT);
    VPLTEST_CALL_AND_CHK_RV(VPL_Init(), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPL_Init(), VPL_ERR_IS_INIT);
    VPLTEST_CALL_AND_CHK_RV(VPL_Quit(), VPL_OK);
}

/// A #VPL_DebugCallback_t.
static void debugCallback(const VPL_DebugMsg_t* data)
{
    VPLTEST_LOG_ERR("debugCallback:%s:%d: %s", data->file, data->line, data->msg);
}

#ifdef VPL_PLAT_IS_WINRT
int testVPL(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
#if !defined(_WIN32) && !defined(ANDROID)
    // catch SIGABRT
    __sighandler_t sig_rv = signal(SIGABRT, signal_handler);
    VPLTEST_ENSURE_EQUAL(sig_rv, VPL_VOID_PTR_NULL, "%p", "signal(SIGABRT, signal_handler)");
#endif
    if (setjmp(vplTestSignalExit) == 0) { // initial invocation of setjmp

        // We need to do these tests before calling VPL_Init().
#      if 0
#      ifdef GVM
        {
            VPLEvent_t event;
            // Test calling Event_Poll before Init is called. Should get VPL_ERR_NOT_INIT.
            VPLTEST_CALL_AND_CHK_RV(VPLEvent_Pop(&event), VPL_ERR_NOT_INIT);
            // Test calling Event_Update before Init is called. Should get VPL_ERR_NOT_INIT.
            VPLTEST_CALL_AND_CHK_RV(VPLEvent_Update(), VPL_ERR_NOT_INIT);
        }
#      endif
#      endif

        VPLTEST_LOG("Build ID = %s", VPL_GetBuildId());
        testInvalidParameters();

        VPLTEST_START("_Init");
        VPLTEST_CALL_AND_CHK_RV(VPL_Init(), VPL_OK);
        VPLTEST_END();

        // To avoid requiring testVPLTime to always run first, we need to grab
        // the initial value from VPLTime_GetTimeStamp() early on.
        initialTimeStamp = VPLTime_GetTimeStamp();

        VPL_RegisterDebugCallback(debugCallback);

        // If no command-line args, run all tests.
        // If we have command-line args, run tests whose VPL-module name
        // matches each arg.
        if (argc == 1) {
            VPLTest_RunAll(vplTest_allTests, ARRAY_ELEMENT_COUNT(vplTest_allTests));
        } else {
            VPLTest_RunByName(argc, argv, vplTest_allTests, ARRAY_ELEMENT_COUNT(vplTest_allTests));
        }

        VPLTEST_START("_Quit");
        VPLTEST_CALL_AND_CHK_RV(VPL_Quit(), VPL_OK);
        VPLTEST_END();
    }

    if (vplTest_getTotalErrCount() > 0) {
        printf("*** TEST SUITE FAILED, %d error(s)\n", vplTest_getTotalErrCount());
    }
    printf("\nCLEAN EXIT\n");
    return vplTest_getTotalErrCount();
}
