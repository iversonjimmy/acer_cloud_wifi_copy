//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplTest_common.h"
#include "vplTest_suite_common.h"
#include "vplu_debug.h"
#ifdef VPL_PLAT_IS_WINRT
#include "vplex_file.h"
#include <stdarg.h>
#endif

#include <setjmp.h>

static int vpl_test_err_count;
static int vpl_test_total_err_count = 0;

const char VPLTEST_DEFAULT_NAME[] = "[NO TEST]";
const char* vplTest_curTestName = VPLTEST_DEFAULT_NAME;

#ifdef VPL_PLAT_IS_WINRT
VPLFile_handle_t hVplTestLog = -1;

void VPLTEST_LOGINIT(const char* logPath)
{
    hVplTestLog = VPLFile_Open(logPath,
                               VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_APPEND | VPLFILE_OPENFLAG_CREATE,
                               0);
}

void VPLTEST_LOGCLOSE()
{
    if (VPLFile_IsValidHandle(hVplTestLog)) {
        VPLFile_Close(hVplTestLog);
        hVplTestLog = NULL;
    }
}

void VPLTEST_LOGWRITE(const char * fmt, ...)
{
    if (VPLFile_IsValidHandle(hVplTestLog)) {
        va_list arg;

        char buff[5120+1]={0};

        va_start (arg, fmt);
        vsnprintf(buff, 5120, fmt, arg);
        va_end (arg);

        VPLFile_Write(hVplTestLog, buff, strlen(buff));
    }
}
#endif

void vplTest_resetErrCount(void) {
    vpl_test_err_count = 0;
}

int vplTest_getErrCount(void) {
    return vpl_test_err_count;
}

int vplTest_getTotalErrCount(void) {
    return vpl_test_total_err_count;
}

void vplTest_incrErrCount(void) {
    ++vpl_test_total_err_count;
    ++vpl_test_err_count;
}

jmp_buf vplTestErrExit;

void vplTest_fail(char const* file, char const* func, int line)
{
    vplTest_incrErrCount();
    VPLTEST_NOTICE("Test FAILED at %s:%s:%d\n", file, func, line);
    fflush(stdout);
    longjmp(vplTestErrExit, 1);
}

const char* vplErrToString(int errCode)
{
    // Cast is required to have the compiler check for completeness (see -Wswitch-enum).
    switch((enum VPLError_t)errCode) {
#define ROW(name) \
    case name: return #name;
    VPL_ERR_CODES
#undef ROW
    }
    return "???";
}

void VPLTest_RunAll(const VPLTest_test tests[], int numTests)
{
    int testIdx;
    for (testIdx = 0; testIdx < numTests; testIdx++) {
        if (tests[testIdx].enabledByDefault) {
            VPLTest_DoTest(&tests[testIdx]);
        }
    }
}

static const VPLTest_test*
vplTest_FindTest(const char* testName, const VPLTest_test tests[], int numTests)
{
    int testIdx;
    for (testIdx = 0; testIdx < numTests; testIdx++) {
        if (strcmp(testName, tests[testIdx].name) == 0) {
            return &tests[testIdx];
        }
    }
    return NULL;
}

void VPLTest_RunByName(int argc, char* argv[], const VPLTest_test tests[], int numTests)
{
    int argIdx;
    // Check that all the requested test names are known.
    VPL_BOOL nameNotFound = VPL_FALSE;
    for (argIdx = 1; argIdx < argc; argIdx++) {
        const char* currName = argv[argIdx];
        if (vplTest_FindTest(currName, tests, numTests) == NULL) {
            VPLTEST_NOTICE("FAILED: No test matching \"%s\"\n", currName);
            vplTest_incrErrCount();
            nameNotFound = VPL_TRUE;
        }
    }
    if (nameNotFound) {
        // Abort and print valid choices.
        int testIdx;
        fprintf(stderr, "Invalid test name specified; valid choices are:\n");
        for (testIdx = 0; testIdx < numTests; testIdx++) {
            fprintf(stderr, "  %s\n", tests[testIdx].name);
        }
        fprintf(stderr, "\n");
    } else {
        // Run each test.
        for (argIdx = 1; argIdx < argc; argIdx++) {
            const char* currName = argv[argIdx];
            const VPLTest_test* currTest = vplTest_FindTest(currName, tests, numTests);
            if (currTest != NULL) {
                VPLTest_DoTest(currTest);
            } else {
                VPLTEST_NOTICE("THIS SHOULDN'T HAPPEN!");
                vplTest_incrErrCount();
            }
        }
    }
}
