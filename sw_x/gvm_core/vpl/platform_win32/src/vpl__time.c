//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vpl_time.h"
#include "vplu.h"

#include <time.h>
#include <sys/timeb.h>

void VPLTime_Update(void) 
{
    /* no-op */
}

VPLTime_t VPLTime_GetTime(void)
{
    VPLTime_t result;

#if __MSVCRT_VERSION__ >= 0x0601
    struct __timeb64  timebuffer;

    // use 'obsoleted' ftime for ms resolution.
    // gettimeofday is not available on Windows.
# ifdef _MSC_VER
    _ftime64_s(&timebuffer);
# else
    _ftime64(&timebuffer);
# endif
#else
    struct _timeb  timebuffer;

    // use 'obsoleted' ftime for ms resolution.
    // gettimeofday is not available on Windows.
    _ftime(&timebuffer);
#endif
    result =
        ( ((VPLTime_t)(timebuffer.time ) * 1000) +
          (timebuffer.millitm) );
    // Multiply up for microseconds.
    result *= 1000;

    return result;
}

VPLTime_t VPLTime_GetTimeStamp(void)
{
    // Not thread-safe, but this approach is also used in the
    // Posix implementation.
    static LARGE_INTEGER freq = { {0, 0} };  // 0 indicates we haven't been called
    static LARGE_INTEGER startTime;
    LARGE_INTEGER time;
    
    if (!QueryPerformanceCounter(&time)) {
        return VPLTIME_INVALID;
    }

    if (!freq.QuadPart) {
        if (!QueryPerformanceFrequency(&freq)) {
            return VPLTIME_INVALID;
        }

        startTime = time;
    }

    // freq is in ticks/s.  We want microseconds.
    // ticks * (us/s) / (ticks/s) = us
    return (time.QuadPart - startTime.QuadPart) * VPLTIME_MICROSEC_PER_SEC / freq.QuadPart;
}

