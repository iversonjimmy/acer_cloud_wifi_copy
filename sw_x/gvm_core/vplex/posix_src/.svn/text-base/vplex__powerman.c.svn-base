#include "vplex_powerman.h"

s32 VPLPowerMan_Start()
{
    return VPL_OK;
}

s32 VPLPowerMan_Stop()
{
    return VPL_OK;
}

s32 VPLPowerMan_KeepHostAwake(VPLPowerMan_Activity_t reason, VPLTime_t duration, VPLTime_t* timestamp_out)
{
    UNUSED(reason);
    UNUSED(duration);
    if (timestamp_out != NULL) {
        // Sleep management isn't supported for this platform; arbitrarily returning an hour to
        // simplify the client code.
        *timestamp_out = VPLTime_GetTimeStamp() + VPLTIME_FROM_SEC(3600);
    }
    return VPL_OK;
}

s32 VPLPowerMan_PostponeSleep(VPLPowerMan_Activity_t reason, VPLTime_t* timestamp_out)
{
    return VPLPowerMan_KeepHostAwake(reason, 0, timestamp_out);
}

void VPLPowerMan_SetDefaultPostponeDuration(VPLTime_t duration)
{
    UNUSED(duration);
}
