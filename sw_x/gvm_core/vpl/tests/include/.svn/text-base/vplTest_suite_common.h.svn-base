//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_TEST_SUITE_COMMON_H__
#define __VPL_TEST_SUITE_COMMON_H__

//============================================================================
/// @file
/// Functionality shared by all VPL-based test suites.
//============================================================================

#include "vplTest_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void vplTest_resetErrCount(void);

extern const char VPLTEST_DEFAULT_NAME[];
extern const char* vplTest_curTestName;

#define VPLTEST_START(test_name) \
    VPLTEST_NOTICE("Starting test VPL%s\n", test_name); \
    vplTest_curTestName = test_name; \
    vplTest_resetErrCount()

#define VPLTEST_END() \
    VPLTEST_NOTICE("VPL%s TEST %s (%d errors)\n" \
            "TC_RESULT = %s ;;; TC_NAME = VPL%s\n", \
            vplTest_curTestName, \
            ((vplTest_getErrCount()==0) ? "PASSED" : "FAILED !!!"), \
            vplTest_getErrCount(), \
            ((vplTest_getErrCount()==0) ? "PASS" : "FAIL"), \
            vplTest_curTestName); \
    vplTest_curTestName = VPLTEST_DEFAULT_NAME; \

typedef struct {
    void (*testFunc)(void);
    const char* name;
    VPL_BOOL enabledByDefault;
} VPLTest_test;

static inline void VPLTest_DoTest(const VPLTest_test* test)
{
    VPLTEST_START(test->name);
    if (vplTest_setjmp()) { \
        test->testFunc();
    }
    VPLTEST_END();
}

/// Run all tests in the array that are "enabledByDefault".
void VPLTest_RunAll(const VPLTest_test tests[], int numTests);

/// For each argument in @a argv (not including the first), run the test with
/// the specified "name".
void VPLTest_RunByName(int argc, char* argv[], const VPLTest_test tests[], int numTests);

#ifdef __cplusplus
}
#endif

//----------------------------------------------------------------------------

#endif // include guard
