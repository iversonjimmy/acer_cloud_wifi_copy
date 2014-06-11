//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_TIME_H__
#define __VPL_TIME_H__

//============================================================================
/// @file
/// Virtual Platform Layer API to get and compare time-of-day/timestamp values.
/// Please see @ref VPLTime.
//============================================================================

#include "vpl_plat.h"
#include "vpl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLTime VPL Time-of-day/Timestamp API
///@{

/// Representation of a time or timestamp, in microseconds.
typedef uint64_t VPLTime_t;

#define PRI_VPLTime_t  PRIu64
#define FMT_VPLTime_t  "%"PRIu64
#define SCN_VPLTime_t  SCNu64

#define VPLTIME_INVALID  UINT64_MAX

/// For use as a timeout value.
#define VPL_TIMEOUT_NONE  UINT64_MAX

#define VPLTIME_MILLISEC_PER_SEC      1000

#define VPLTIME_MICROSEC_PER_MILLISEC 1000
#define VPLTIME_MICROSEC_PER_SEC      1000000

#define VPLTIME_NANOSEC_PER_MICROSEC  1000
#define VPLTIME_NANOSEC_PER_MILLISEC  1000000
#define VPLTIME_NANOSEC_PER_SEC       1000000000

#define VPLTIME_SECONDS_PER_MINUTE  60
#define VPLTIME_SECONDS_PER_HOUR    3600

/// Convert from seconds to #VPLTime_t.
static inline VPLTime_t VPLTime_FromMinutes(uint64_t minutes) { return minutes * (VPLTIME_MICROSEC_PER_SEC * VPLTIME_SECONDS_PER_MINUTE); }

/// Convert from seconds to #VPLTime_t.
static inline VPLTime_t VPLTime_FromSec(uint64_t seconds) { return seconds * VPLTIME_MICROSEC_PER_SEC; }

/// Convert from #VPLTime_t to seconds (rounded down).
static inline uint64_t VPLTime_ToSec(VPLTime_t vplTime) { return vplTime / VPLTIME_MICROSEC_PER_SEC; }

/// Convert from #VPLTime_t to seconds (rounded up).
static inline uint64_t VPLTime_ToSecRoundUp(VPLTime_t vplTime) { return (vplTime + VPLTIME_MICROSEC_PER_SEC - 1) / VPLTIME_MICROSEC_PER_SEC; }

/// Convert from milliseconds to #VPLTime_t.
static inline VPLTime_t VPLTime_FromMillisec(uint64_t ms) { return ms * VPLTIME_MICROSEC_PER_MILLISEC; }

/// Convert from #VPLTime_t to milliseconds (rounded down).
static inline uint64_t VPLTime_ToMillisec(VPLTime_t vplTime) { return vplTime / VPLTIME_MICROSEC_PER_MILLISEC; }

/// Convert from #VPLTime_t to milliseconds (rounded up).
static inline uint64_t VPLTime_ToMillisecRoundUp(VPLTime_t vplTime) { return (vplTime + VPLTIME_MICROSEC_PER_MILLISEC - 1) / VPLTIME_MICROSEC_PER_MILLISEC; }

/// Convert from microseconds to #VPLTime_t.
static inline VPLTime_t VPLTime_FromMicrosec(uint64_t us) { return us; }

/// Convert from #VPLTime_t to microseconds.
static inline uint64_t VPLTime_ToMicrosec(VPLTime_t vplTime) { return vplTime; }

/// Convert from nanoseconds to #VPLTime_t (rounded down).
static inline VPLTime_t VPLTime_FromNanosec(uint64_t ns) { return ns / VPLTIME_NANOSEC_PER_MICROSEC; }

/// Convert from nanoseconds to #VPLTime_t (rounded up).
static inline VPLTime_t VPLTime_FromNanosecRoundUp(uint64_t ns) { return (ns + VPLTIME_NANOSEC_PER_MICROSEC - 1) / VPLTIME_NANOSEC_PER_MICROSEC; }

/// Convert from #VPLTime_t to nanoseconds.
static inline uint64_t VPLTime_ToNanosec(VPLTime_t vplTime) { return vplTime * VPLTIME_NANOSEC_PER_MICROSEC; }

/// @deprecated
#define VPLTIME_FROM_SEC  VPLTime_FromSec
/// @deprecated
#define VPLTIME_TO_SEC  VPLTime_ToSec
/// @deprecated
#define VPLTIME_FROM_MILLISEC  VPLTime_FromMillisec
/// @deprecated
#define VPLTIME_TO_MILLISEC  VPLTime_ToMillisec
/// @deprecated
#define VPLTIME_FROM_MICROSEC  VPLTime_FromMicrosec
/// @deprecated
#define VPLTIME_TO_MICROSEC  VPLTime_ToMicrosec
/// @deprecated
#define VPLTIME_FROM_NANOSEC  VPLTime_FromNanosec
/// @deprecated
#define VPLTIME_TO_NANOSEC  VPLTime_ToNanosec

/// Return the current time, in microseconds since the Epoch.
/// @note This value can move backwards if the system's clock is adjusted, so please
///     use #VPLTime_GetTimeStamp() if you want a monotonically-increasing value.
/// @note Depending on the platform, the precision may be coarser than 1 microsecond (particularly,
///     the result will currently be rounded to the nearest millisecond on Win32).
/// @return the current time, in microseconds since the Epoch
VPLTime_t VPLTime_GetTime(void);

/// Return time in microseconds since the first invocation of this function.
/// @note This is expected to be accurate to the nearest microsecond on every platform.
/// @note This value should never decrease, even if the system's clock is moved backwards.
/// @return the elapsed time, in microseconds since the first invocation of this function
VPLTime_t VPLTime_GetTimeStamp(void);

/// Get the absolute value of the difference in times, in microseconds.
/// @return the difference, in microseconds
static inline 
VPLTime_t VPLTime_DiffAbs(VPLTime_t a, VPLTime_t b)
{
    VPLTime_t result;
    if (a >= b) {
        result = (a - b);
    } else {
        result = (b - a);
    }
    return result;
}

/// Get the time elapsed from begin to end,
/// clamping to 0 if it would be negative.
/// @return the difference, in microseconds
static inline 
VPLTime_t VPLTime_DiffClamp(VPLTime_t end, VPLTime_t begin)
{
    VPLTime_t result;
    if (end > begin) {
        result = (end - begin);
    } else {
        result = 0;
    }
    return result;
}

/// Perform timestamp maintenance to ensure valid timestamps forever.
/// Call this from only one thread at least every 37 minutes.
void VPLTime_Update(void);

///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
