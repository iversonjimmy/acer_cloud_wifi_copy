//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#include <vplu.h>
#include <vpl_conv.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_user.h>
#include <vplex_assert.h>
#include <vplex_time.h>
#include <cslsha.h>
#include <aes.h>
#include <log.h>
#include <stdlib.h>

#ifndef IOS
#include <malloc.h>
#endif

#include <string.h>
#include <stdio.h>

#include <pxd_log.h>

#define date_format       "%04d/%02d/%02d %02d:%02d:%02d.%03d UTC"

/*
 *  Create a UTC timestamp using the current time.
 */
char *
pxd_ts(char *buffer, int size)
{
    return pxd_print_time(buffer, size, VPLTime_GetTime());
}

/*
 *  Create a UTC timestamp using the given time.
 */
char *
pxd_print_time(char *buffer, int size, VPLTime_t input_time)
{
    VPLTime_CalendarTime_t  vplex_time;

    VPLTime_ConvertToCalendarTimeUniversal(input_time, &vplex_time);

    snprintf(buffer, size, date_format,
        vplex_time.year,
        vplex_time.month,
        vplex_time.day,
        vplex_time.hour,
        vplex_time.min,
        vplex_time.sec,
        vplex_time.msec);

    /*
     *  Make sure there's a null termination.
     */
    buffer[size - 1] = 0;
    return buffer;
}
