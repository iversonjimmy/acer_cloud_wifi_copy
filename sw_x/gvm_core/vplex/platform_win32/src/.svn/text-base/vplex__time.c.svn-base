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

// According to http://msdn.microsoft.com/en-us/library/0z9czt0w%28v=vs.80%29.aspx
// "...gmtime, mktime, mkgmtime, and localtime all use a single tm structure per thread for the conversion..."
// Since the structure is thread-local, there is no need for reentrant "*_r" versions of those functions.

#ifndef _MSC_VER

#include <sys/time.h>

typedef struct tm* (*VPLTime_ConversionFunc)(const time_t* timer);

static void VPLTime_priv_GetCalendarTime(
        VPLTime_CalendarTime_t* calendarTime_out,
        VPLTime_ConversionFunc conversionFunc,
        struct timeval tempWithMicrosec)
{
    if (calendarTime_out != NULLPTR) {
        struct tm* temp = conversionFunc(&tempWithMicrosec.tv_sec);
        calendarTime_out->year = temp->tm_year + 1900;
        calendarTime_out->month = temp->tm_mon + 1;
        calendarTime_out->day = temp->tm_mday;
        calendarTime_out->hour = temp->tm_hour;
        calendarTime_out->min = temp->tm_min;
        calendarTime_out->sec = temp->tm_sec;
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
    VPLTime_priv_GetCalendarTime(calendarTime_out, &localtime, tempWithMicrosec);
}

void VPLTime_ConvertToCalendarTimeUniversal(
        VPLTime_t microsecondsSinceEpoch,
        VPLTime_CalendarTime_t* calendarTime_out)
{
    struct timeval tempWithMicrosec;
    tempWithMicrosec.tv_sec = microsecondsSinceEpoch / MICROSECS_PER_SEC;
    tempWithMicrosec.tv_usec = microsecondsSinceEpoch % MICROSECS_PER_SEC;
    VPLTime_priv_GetCalendarTime(calendarTime_out, &gmtime, tempWithMicrosec);
}

void VPLTime_GetCalendarTimeLocal(VPLTime_CalendarTime_t* calendarTime_out)
{
    struct timeval tempWithMicrosec;
    (IGNORE_RESULT) gettimeofday(&tempWithMicrosec, NULL);
    VPLTime_priv_GetCalendarTime(calendarTime_out, &localtime, tempWithMicrosec);
}

void VPLTime_GetCalendarTimeUniversal(VPLTime_CalendarTime_t* calendarTime_out)
{
    struct timeval tempWithMicrosec;
    (IGNORE_RESULT) gettimeofday(&tempWithMicrosec, NULL);
    VPLTime_priv_GetCalendarTime(calendarTime_out, &gmtime, tempWithMicrosec);
}

#else
void VPLTime_ConvertToCalendarTimeLocal(
        VPLTime_t microsecondsSinceEpoch,
        VPLTime_CalendarTime_t* calendarTime_out)
{
    time_t rawtime = VPLTime_ToSec(microsecondsSinceEpoch);
    struct tm* calendarTime_in = localtime(&rawtime);

    calendarTime_out->year  = calendarTime_in->tm_year + 1900;
    calendarTime_out->month = calendarTime_in->tm_mon + 1;
    calendarTime_out->day   = calendarTime_in->tm_mday;
    calendarTime_out->hour  = calendarTime_in->tm_hour;
    calendarTime_out->min   = calendarTime_in->tm_min;
    calendarTime_out->sec   = calendarTime_in->tm_sec;
    calendarTime_out->msec  = microsecondsSinceEpoch % MICROSECS_PER_SEC / 1000;
    calendarTime_out->usec  = microsecondsSinceEpoch % MICROSECS_PER_SEC;
}
void VPLTime_ConvertToCalendarTimeUniversal(
        VPLTime_t microsecondsSinceEpoch,
        VPLTime_CalendarTime_t* calendarTime_out)
{
    time_t rawtime = VPLTime_ToSec(microsecondsSinceEpoch);
    struct tm* calendarTime_in = gmtime(&rawtime);

    calendarTime_out->year  = calendarTime_in->tm_year + 1900;
    calendarTime_out->month = calendarTime_in->tm_mon + 1;
    calendarTime_out->day   = calendarTime_in->tm_mday;
    calendarTime_out->hour  = calendarTime_in->tm_hour;
    calendarTime_out->min   = calendarTime_in->tm_min;
    calendarTime_out->sec   = calendarTime_in->tm_sec;
    calendarTime_out->msec  = microsecondsSinceEpoch % MICROSECS_PER_SEC / 1000;
    calendarTime_out->usec  = microsecondsSinceEpoch % MICROSECS_PER_SEC;
}
static void VPLTime_priv_GetCalendarTime(
        VPLTime_CalendarTime_t* calendarTime_out,
        const SYSTEMTIME* time)
{
    if (calendarTime_out != NULLPTR) {
        calendarTime_out->year = time->wYear;
        calendarTime_out->month = time->wMonth;
        calendarTime_out->day = time->wDay;
        calendarTime_out->hour = time->wHour;
        calendarTime_out->min = time->wMinute;
        calendarTime_out->sec = time->wSecond;
        calendarTime_out->msec = time->wMilliseconds;
        calendarTime_out->usec = time->wMilliseconds * 1000;
    }
}
void VPLTime_GetCalendarTimeLocal(VPLTime_CalendarTime_t* calendarTime_out)
{
    SYSTEMTIME localTime;
    GetLocalTime(&localTime);
    VPLTime_priv_GetCalendarTime(calendarTime_out, &localTime);
}
void VPLTime_GetCalendarTimeUniversal(VPLTime_CalendarTime_t* calendarTime_out)
{
    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);
    VPLTime_priv_GetCalendarTime(calendarTime_out, &sysTime);
}
#endif
