//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_time.h"
#include "vpl_th.h"

#include <time.h>
#include <sys/time.h>

typedef struct tm* (*VPLTime_ConversionFunc)(const time_t* timer, struct tm* result);

static void VPLTime_priv_GetCalendarTime(
        VPLTime_CalendarTime_t* calendarTime_out,
        VPLTime_ConversionFunc conversionFunc,
        struct timeval tempWithMicrosec)
{
    if (calendarTime_out != NULLPTR) {
        struct tm temp;
        (IGNORE_RESULT) conversionFunc(&tempWithMicrosec.tv_sec, &temp);
        calendarTime_out->year = temp.tm_year + 1900;
        calendarTime_out->month = temp.tm_mon + 1;
        calendarTime_out->day = temp.tm_mday;
        calendarTime_out->hour = temp.tm_hour;
        calendarTime_out->min = temp.tm_min;
        calendarTime_out->sec = temp.tm_sec;
        calendarTime_out->msec = tempWithMicrosec.tv_usec / 1000;
        calendarTime_out->usec = tempWithMicrosec.tv_usec;
    }
}

void VPLTime_ConvertToCalendarTimeLocal(
        VPLTime_t microsecondsSinceEpoch,
        VPLTime_CalendarTime_t* calendarTime_out)
{
    struct timeval tempWithMicrosec;
    tempWithMicrosec.tv_sec = microsecondsSinceEpoch / MICROSECS_PER_SEC;
    tempWithMicrosec.tv_usec = microsecondsSinceEpoch % MICROSECS_PER_SEC;
    VPLTime_priv_GetCalendarTime(calendarTime_out, &localtime_r, tempWithMicrosec);
}

void VPLTime_ConvertToCalendarTimeUniversal(
        VPLTime_t microsecondsSinceEpoch,
        VPLTime_CalendarTime_t* calendarTime_out)
{
    struct timeval tempWithMicrosec;
    tempWithMicrosec.tv_sec = microsecondsSinceEpoch / MICROSECS_PER_SEC;
    tempWithMicrosec.tv_usec = microsecondsSinceEpoch % MICROSECS_PER_SEC;
    VPLTime_priv_GetCalendarTime(calendarTime_out, &gmtime_r, tempWithMicrosec);
}

void VPLTime_GetCalendarTimeLocal(VPLTime_CalendarTime_t* calendarTime_out)
{
    struct timeval tempWithMicrosec;
    (IGNORE_RESULT) gettimeofday(&tempWithMicrosec, NULL);
    VPLTime_priv_GetCalendarTime(calendarTime_out, &localtime_r, tempWithMicrosec);
}

void VPLTime_GetCalendarTimeUniversal(VPLTime_CalendarTime_t* calendarTime_out)
{
    struct timeval tempWithMicrosec;
    (IGNORE_RESULT) gettimeofday(&tempWithMicrosec, NULL);
    VPLTime_priv_GetCalendarTime(calendarTime_out, &gmtime_r, tempWithMicrosec);
}
