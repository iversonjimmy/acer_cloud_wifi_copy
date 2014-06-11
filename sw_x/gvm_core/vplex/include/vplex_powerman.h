/*
 *  Copyright 2012 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef __VPLEX_POWERMAN_H__
#define __VPLEX_POWERMAN_H__

//============================================================================
/// @file
/// Power-Management.
//============================================================================

#include "vplex_plat.h"
#include "vpl_time.h"

#ifdef  __cplusplus
extern "C" {
#endif

s32 VPLPowerMan_Start();

s32 VPLPowerMan_Stop();

typedef enum {
    VPL_POWERMAN_ACTIVITY_SERVING_DATA = 1,
    VPL_POWERMAN_ACTIVITY_PROGRAMMING_IOAC = 2,
} VPLPowerMan_Activity_t;

/// If the OS permits, postpones sleep that would occur due to no user input.
/// @param reason Specify the reason to postpone sleep.
/// @param duration Minimum duration to postpone sleep.
/// @param timestamp_out On success, this will be set to the earliest timestamp (see
///     #VPLTime_GetTimeStamp) when the system could go to sleep if this function is not
///     called again before then.  Can be NULL if the caller doesn't care.
s32 VPLPowerMan_KeepHostAwake(VPLPowerMan_Activity_t reason, VPLTime_t duration, VPLTime_t* timestamp_out);

// TODO: this is a temporary function!
/// @deprecated Use #VPLPowerMan_KeepHostAwake() to specify an explicit duration.
/// Same as #VPLPowerMan_KeepHostAwake(), but uses the default value for "duration".
s32 VPLPowerMan_PostponeSleep(VPLPowerMan_Activity_t reason, VPLTime_t* timestamp_out);

// TODO: this is a temporary function!
/// Sets the default duration for #VPLPowerMan_PostponeSleep().
/// @deprecated Use #VPLPowerMan_KeepHostAwake() to specify an explicit duration.
void VPLPowerMan_SetDefaultPostponeDuration(VPLTime_t duration);

#ifdef __cplusplus
}
#endif

#endif // include guard
