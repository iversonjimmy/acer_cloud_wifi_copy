//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vpl_time.h"
#include "vpl_thread.h"

#include "vplTest.h"

VPLTime_t initialTimeStamp;

static void testTimeDiffs(void)
{
    // Make sure that the diff functions work as advertised.
    VPLTEST_CALL_AND_CHK_UNSIGNED(VPLTime_DiffAbs((VPLTime_t)5432, (VPLTime_t)4321), 1111);
    VPLTEST_CALL_AND_CHK_UNSIGNED(VPLTime_DiffAbs((VPLTime_t)4321, (VPLTime_t)5432), 1111);
    VPLTEST_CALL_AND_CHK_UNSIGNED(VPLTime_DiffClamp((VPLTime_t)5432, (VPLTime_t)4321), 1111);
    VPLTEST_CALL_AND_CHK_UNSIGNED(VPLTime_DiffClamp((VPLTime_t)4321, (VPLTime_t)5432), 0);
}

static void testTimestamp(void)
{
    VPLTime_t relativeTimeStart;
    VPLTime_t realTimeStart;
    VPLTime_t relativeTime;
    VPLTime_t realTime;
    VPLTime_t diff;
    int i;

    realTimeStart = VPLTime_GetTime();
    relativeTimeStart = VPLTime_GetTimeStamp();

    // Timestamp should be 0 at first invocation.
    // Note: initialTimeStamp is set in vplTest.c.
    VPLTEST_CHK_EQUAL(initialTimeStamp, (VPLTime_t)0, FMT_VPLTime_t,
            "Timestamp should be 0 at first invocation.");

    VPLTEST_LOG("Timestamp:"FMT_VPLTime_t" at real time "FMT_VPLTime_t".",
            relativeTimeStart, realTimeStart);

    VPLTEST_LOG("Timestamps should move in lockstep with real time.");
    for (i = 0; i < 10; i++) {
        VPLThread_Sleep(i * i * 1000);

        realTime = VPLTime_GetTime();
        relativeTime = VPLTime_GetTimeStamp();
        VPLTEST_LOG("Timestamp:"FMT_VPLTime_t" at real time "FMT_VPLTime_t".",
                relativeTime, realTime);

        // Allow 20ms difference between times.
        diff = VPLTime_DiffAbs((realTime - realTimeStart), (relativeTime - relativeTimeStart));
        if (diff > 20000) {
            VPLTEST_FAIL("VPLTime_GetTimeStamp(): Timestamp and clock out of sync.");
            VPLTEST_LOG("Actual difference in time: "FMT_VPLTime_t, diff);
        }

        // Not measuring precision of timestamps since that really tests the
        // precision of sleep. See thread tests for that.
    }

    // Make sure that we can do timestamp maintenance.
    VPLTime_Update();
}

void testVPLTime(void)
{
    VPLTEST_LOG("testTimeDiffs");
    testTimeDiffs();

    VPLTEST_LOG("testTimestamp");
    testTimestamp();
}
