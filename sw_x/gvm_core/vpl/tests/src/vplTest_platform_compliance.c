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

#include "vpl_plat.h"

static int testHelper_vsnprintf(char *str, size_t size, const char *format, ...)
{
    int rv;
    va_list ap;
    va_start(ap, format);
    rv = VPL_vsnprintf(str, size, format, ap);
    va_end(ap);
    return rv;
}

// The preprocessor will escape quotes when stringifying, but it doesn't know to also double-up
// "%" when it is used inside a string.  Add a layer of indirection.
static const char TEST_FMT[] = "%s";

void testVPLPlatformCompliance(void)
{
    char tempBuf3[3];
    char tempBuf4[4];
    char tempBuf5[5];

    // Fill the buffers with junk:
    memset(tempBuf3, '#', sizeof(tempBuf3));
    memset(tempBuf4, '#', sizeof(tempBuf4));
    memset(tempBuf5, '#', sizeof(tempBuf5));


    // Make sure snprintf behaves as C99 dictates.
    VPLTEST_CALL_AND_CHK_INT(snprintf(tempBuf5, sizeof(tempBuf5), TEST_FMT, "test"), 4);
    VPLTEST_CALL_AND_CHK_INT(strcmp(tempBuf5, "test"), 0);
    VPLTEST_CALL_AND_CHK_INT(snprintf(tempBuf4, sizeof(tempBuf4), TEST_FMT, "test"), 4);
    VPLTEST_CALL_AND_CHK_INT(strcmp(tempBuf4, "tes"), 0);
    VPLTEST_CALL_AND_CHK_INT(snprintf(tempBuf3, sizeof(tempBuf3), TEST_FMT, "test"), 4);
    VPLTEST_CALL_AND_CHK_INT(strcmp(tempBuf3, "te"), 0);
    VPLTEST_CALL_AND_CHK_INT(snprintf(NULL, 0, TEST_FMT, "test"), 4);

    // Fill the buffers with junk:
    memset(tempBuf3, '#', sizeof(tempBuf3));
    memset(tempBuf4, '#', sizeof(tempBuf4));
    memset(tempBuf5, '#', sizeof(tempBuf5));

    // Make sure vsnprintf behaves as C99 dictates.
    VPLTEST_CALL_AND_CHK_INT((testHelper_vsnprintf(tempBuf5, sizeof(tempBuf5), TEST_FMT, "test")), 4);
    VPLTEST_CALL_AND_CHK_INT(strcmp(tempBuf5, "test"), 0);
    VPLTEST_CALL_AND_CHK_INT(testHelper_vsnprintf(tempBuf4, sizeof(tempBuf4), TEST_FMT, "test"), 4);
    VPLTEST_CALL_AND_CHK_INT(strcmp(tempBuf4, "tes"), 0);
    VPLTEST_CALL_AND_CHK_INT(testHelper_vsnprintf(tempBuf3, sizeof(tempBuf3), TEST_FMT, "test"), 4);
    VPLTEST_CALL_AND_CHK_INT(strcmp(tempBuf3, "te"), 0);
    VPLTEST_CALL_AND_CHK_INT(testHelper_vsnprintf(NULL, 0, TEST_FMT, "test"), 4);
}
