//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_time.h"

#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#ifdef IOS
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

void VPLTime_Update(void) 
{
    /* no-op */
}

#ifdef IOS
VPLTime_t VPLTime_GetTime(void)
{
    VPLTime_t result;
    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    result =((unsigned long long)now_time.tv_sec * 1000000) + now_time.tv_usec;
    return result;
}

VPLTime_t VPLTime_GetTimeStamp(void)
{
    static mach_timebase_info_data_t timebase_info;
    static uint64_t start_time = 0;
    uint64_t now_time;
    uint64_t duration;

    if ( timebase_info.denom == 0 ) {
        mach_timebase_info(&timebase_info);
    }

    now_time = mach_absolute_time();

    if (start_time == 0) {
        start_time = now_time;
        return 0;
    } else {
        duration = now_time - start_time;
        //Translate to microseconds
        duration = (duration * timebase_info.numer / timebase_info.denom) / 1000;
        return duration;
    }
}
#else
VPLTime_t VPLTime_GetTime(void)
{
    VPLTime_t result;
    struct timespec now_time;

    clock_gettime(CLOCK_REALTIME, &now_time);

    result = (((VPLTime_t)now_time.tv_sec) * 1000000 +
              (VPLTime_t)(now_time.tv_nsec / 1000));

    return result;
}

VPLTime_t VPLTime_GetTimeStamp(void)
{
    VPLTime_t result;
    struct timespec now_time;

    /* Using -1 to represent uninitialized */
    static struct timespec start_time = {.tv_sec = 0, .tv_nsec = -1};

    clock_gettime(CLOCK_MONOTONIC, &now_time);

    if(start_time.tv_nsec == -1) {

        start_time.tv_sec = now_time.tv_sec;
        start_time.tv_nsec = now_time.tv_nsec;

        /* If timestamps aren't behaving well, uncomment this block to check resolution.
        {
            struct timespec resolution;
            clock_getres(CLOCK_MONOTONIC, &resolution);
            // Use some printf variant here
        }
        */
    }

    result = (((VPLTime_t)(now_time.tv_sec - start_time.tv_sec)) * 1000000 +
              (VPLTime_t)((now_time.tv_nsec - start_time.tv_nsec) / 1000));

    return result;
}
#endif
