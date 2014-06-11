//
//  VPLTestWrapper.c
//  CCDLibrary
//
//  Created by wukon hsieh on 12/5/30.
//  Copyright (c) 2012 Acer Inc. All rights reserved.
//

#include "VPLTestWrapper.h"

#include "vplTest.h"
#include "vplTest_suite_common.h"
#include "vplu_debug.h"

#include <setjmp.h>
#include <signal.h>
#include <assert.h>

static VPLTest_test vplTest_allTests[] = {
    { testVPLPlatformCompliance, "PlatformCompliance", VPL_TRUE },
    { testVPLPlat,       "Plat",       VPL_TRUE },
    { testVPLConv,       "Conv",       VPL_TRUE },
//    { testVPLDL,         "DL",         VPL_TRUE },
    { testVPLTime,       "Time",       VPL_TRUE },
    { testVPLThread,     "Thread",     VPL_TRUE },
    { testVPLMutex,      "Mutex",      VPL_TRUE },
    { testVPLCond,       "Cond",       VPL_TRUE },
    { testVPLSem,        "Sem",        VPL_TRUE },
    { testVPLSlimRWLock, "SlimRWLock", VPL_TRUE },
    { testVPLSocket,     "Socket",     VPL_TRUE },
    { testVPLFS,         "FS",         VPL_TRUE },
    { testVPLNetwork,    "Network",    VPL_TRUE },
};

static jmp_buf vplTestSignalExit;

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

int runVPLTests()
{
    if (setjmp(vplTestSignalExit) == 0) { // initial invocation of setjmp
        VPLTEST_LOG("Build ID = %s", VPL_GetBuildId());
        testInvalidParameters();

        VPLTEST_START("_Init");
        VPLTEST_CALL_AND_CHK_RV(VPL_Init(), VPL_OK);
        VPLTEST_END();

        // To avoid requiring testVPLTime to always run first, we need to grab
        // the initial value from VPLTime_GetTimeStamp() early on.
        initialTimeStamp = VPLTime_GetTimeStamp();

        VPL_RegisterDebugCallback(debugCallback);

        VPLTest_RunAll(vplTest_allTests, ARRAY_ELEMENT_COUNT(vplTest_allTests)); 

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
