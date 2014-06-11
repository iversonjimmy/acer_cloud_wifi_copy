//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_TIME_H__
#define __VPLEX_TIME_H__

#include "vpl_time.h"
#include "vplex_plat.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MS_PER_SEC             1000
#define MICROSECS_PER_MS       1000
#define NANOSECS_PER_MICROSEC  1000
#define MICROSECS_PER_SEC      (MICROSECS_PER_MS * MS_PER_SEC)
#define NANOSECS_PER_SEC       (MICROSECS_PER_SEC * NANOSECS_PER_MICROSEC)
#define NANOSECS_PER_MS        (MICROSECS_PER_MS * NANOSECS_PER_MICROSEC)

typedef struct VPLTime_CalendarTime
{
    s32 year;  ///< Year (starting at 0)
    s32 month; ///< Month (starting with January) [1, 12]
    s32 day;   ///< Day of the month [1, 31]
    s32 hour;  ///< Hours since midnight [0, 23]
    s32 min;   ///< Minutes after the hour [0, 59]
    s32 sec;   ///< Seconds after the minute [0, 59 (or up to 61 for leap seconds)]
    s32 msec;  ///< Milliseconds after the second [0,999]
    s32 usec;  ///< Microseconds after the second [0,999999]
} VPLTime_CalendarTime_t;

#define FMT_VPLTime_CalendarTime_t  "%d/%02d/%02d %02d:%02d:%02d.%03d"
#define VAL_VPLTime_CalendarTime_t(time) \
    (time).year, (time).month, (time).day, (time).hour, (time).min, (time).sec, (time).msec

#if VPL_PLAT_HAS_CALENDAR_TIME
void VPLTime_GetCalendarTimeLocal(VPLTime_CalendarTime_t* calendarTime_out);

void VPLTime_GetCalendarTimeUniversal(VPLTime_CalendarTime_t* calendarTime_out);

void VPLTime_ConvertToCalendarTimeLocal(
                        VPLTime_t microsecondsSinceEpoch,
                        VPLTime_CalendarTime_t* calendarTime_out);

void VPLTime_ConvertToCalendarTimeUniversal(
                        VPLTime_t microsecondsSinceEpoch,
                        VPLTime_CalendarTime_t* calendarTime_out);
#endif

/// VPL equivalent of POSIX timespec.
typedef struct VPLTimeSpec {
    u64 tv_sec;
    s32 tv_nsec;
} VPLTimeSpec_t;

/// Get the difference in times, clamping to 0 if it would be negative.
static inline VPLTimeSpec_t VPLTimeSpec_DiffClamp(VPLTimeSpec_t end, VPLTimeSpec_t begin)
{
    VPLTimeSpec_t result;
    u64 borrow = 0;  // opposite of carry
    if(begin.tv_nsec > end.tv_nsec) {
        borrow = 1;
        end.tv_nsec += NANOSECS_PER_SEC;
    }

    if(end.tv_sec >= borrow + begin.tv_sec) { // (Timespec end) >= (Timespec begin)
        result.tv_sec = end.tv_sec - borrow - begin.tv_sec;
        result.tv_nsec = end.tv_nsec - begin.tv_nsec;
    } else { // Clamp
        result.tv_sec = 0;
        result.tv_nsec = 0;
    }
    return result;
}

/// Sleep for some number of seconds.
void VPLTime_SleepSec(unsigned int seconds);

/// Sleep for some number of milliseconds.
void VPLTime_SleepMs(unsigned int milliseconds);

#ifdef __cplusplus
}
#endif

#endif // include guard
