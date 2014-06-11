//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#include "vpl__mac_time.h"
#include <mach/mach_time.h>


int clock_gettime(int clk_id, struct timespec *timeSpec_out){
    int rv = 0;
    
    if (clk_id == CLOCK_REALTIME) {
        struct timeval  now = {0, 0};
        
        rv = gettimeofday(&now, NULL);
        if (rv == 0) {
            timeSpec_out->tv_sec = now.tv_sec;
            timeSpec_out->tv_nsec = now.tv_usec * 1000;
        }
    } else if (clk_id == CLOCK_MONOTONIC) {
        // TODO: bug 521: ready for CLOCK_MONOTONIC
        // Untested for real use, leave TODO comment here for code tracing.
        //
        // Other alternative method can be found here:
        // http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
        mach_timebase_info_data_t timebase;
        mach_timebase_info(&timebase);
        
        uint64_t time;
        time = mach_absolute_time();
        
        double nseconds = ((double)time * (double)timebase.numer)/((double)timebase.denom);
        double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
        
        timeSpec_out->tv_sec = seconds;
        timeSpec_out->tv_nsec = nseconds;
    } else {
        return -1;
    }
    
    return rv;
}