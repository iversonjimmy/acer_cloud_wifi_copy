#include "vplex_powerman.h"

#include "vplex_private.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"

#ifndef _MSC_VER
// Missing from mingw:
#  define ES_AWAYMODE_REQUIRED ((DWORD)0x00000040)
#endif

static VPLLazyInitMutex_t s_powerManMutex = VPLLAZYINITMUTEX_INIT;
static bool s_isRunning = false;

//-----------------------------
// These are only valid when s_isRunning is true.
//-----------------------------
static VPLCond_t s_powerManCondVar = VPLCOND_SET_UNDEF;
/// If VPLTime_GetTimeStamp() is less than this value, we need to set the flags that prevent sleep.
static VPLTime_t s_nextAllowSleepTimestamp;
static bool s_quitPowerManThread;
static VPLDetachableThreadHandle_t s_powerManThread;
static bool s_allowingSleep;
//-----------------------------

// Notes:
// 1) When calling SetThreadExecutionState without ES_CONTINUOUS, the behavior appears to be:
//   If there was user input since the last time the machine woke up, this resets the system idle
//   timer (user-configurable in the Windows GUIs, down to 1 minute).
//   If there wasn't any user input since the last time the machine woke up, this resets the
//   unattended idle timer (which seems to always be 2 minutes).
// 2) When using ES_CONTINUOUS, it is critical that all the calls occur from the same thread.
// 3) Use "POWERCFG -REQUESTS" to see the current state.

#define SET_THREAD_EXECUTION_STATE(flags_) \
    BEGIN_MULTI_STATEMENT_MACRO \
    if (SetThreadExecutionState(flags_) == 0) { \
        VPL_LIB_LOG_ERR(VPL_SG_POWER, "SetThreadExecutionState failed: "FMT_DWORD, GetLastError()); \
    } \
    END_MULTI_STATEMENT_MACRO

static VPLTHREAD_FN_DECL powermanThreadFn(void* arg)
{
    VPLMutex_t* powerManMutex = VPLLazyInitMutex_GetMutex(&s_powerManMutex);
    MutexAutoLock lock(powerManMutex);
    while (!s_quitPowerManThread) {
        VPLTime_t now = VPLTime_GetTimeStamp();
        VPLTime_t timeout = VPLTime_DiffClamp(s_nextAllowSleepTimestamp, now);
        if (timeout == 0) {
            // s_nextAllowSleepTimestamp is in the past (or exactly now); allow sleep.
            VPL_LIB_LOG_INFO(VPL_SG_POWER, "Allowing sleep");
            // When using ES_CONTINUOUS, it is critical that all the calls occur from the same thread.
            SET_THREAD_EXECUTION_STATE(ES_CONTINUOUS);
            s_allowingSleep = true;
            timeout = VPL_TIMEOUT_NONE; // No work to do until we are signaled.
        } else {
            // s_nextAllowSleepTimestamp is in the future; prevent sleep until timeout elapses.
            if (s_allowingSleep) {
                VPL_LIB_LOG_INFO(VPL_SG_POWER, "Setting flags to keep awake for at least "FMT_VPLTime_t"us", timeout);
                // When using ES_CONTINUOUS, it is critical that all the calls occur from the same thread.
                SET_THREAD_EXECUTION_STATE(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
                s_allowingSleep = false;
            } else {
                // s_allowingSleep is already false.
                VPL_LIB_LOG_INFO(VPL_SG_POWER, "Continuing to keep awake for at least "FMT_VPLTime_t"us", timeout);
            }
        }
        int rv = VPLCond_TimedWait(&s_powerManCondVar, powerManMutex, timeout);
        if (rv == VPL_ERR_TIMEOUT) {
            VPL_LIB_LOG_INFO(VPL_SG_POWER, "Time to update");
        } else if (rv == VPL_OK) {
            VPL_LIB_LOG_INFO(VPL_SG_POWER, "Signaled");
        } else {
            VPL_LIB_LOG_INFO(VPL_SG_POWER, "%s failed: %d", "VPLCond_TimedWait", rv);
        }
    }

    // Clear all flags before exiting the thread.
    // When using ES_CONTINUOUS, it is critical that all the calls occur from the same thread.
    SET_THREAD_EXECUTION_STATE(ES_CONTINUOUS);
    VPL_LIB_LOG_INFO(VPL_SG_POWER, "Thread exiting");
    return VPLTHREAD_RETURN_VALUE;
}

s32 VPLPowerMan_Start()
{
    s32 rv;

    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_powerManMutex));
    if (s_isRunning) {
        return VPL_ERR_ALREADY;
    }
    rv = VPLCond_Init(&s_powerManCondVar);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_POWER, "%s failed: %d", "VPLCond_Init", rv);
        goto out;
    }
    s_nextAllowSleepTimestamp = 0;
    s_quitPowerManThread = false;
    s_allowingSleep = true;
    rv = VPLDetachableThread_Create(&s_powerManThread, powermanThreadFn, NULL, NULL, NULL);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_POWER, "%s failed: %d", "VPLDetachableThread_Create", rv);
        goto failed_create_thread;
    }
    s_isRunning = true;
    goto out;

failed_create_thread:
    VPLCond_Destroy(&s_powerManCondVar);
out:
    return rv;
}

s32 VPLPowerMan_Stop()
{
    s32 rv;
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_powerManMutex));
        if (!s_isRunning) {
            return VPL_ERR_ALREADY;
        }
        if (s_quitPowerManThread) {
            return VPL_ERR_BUSY;
        }
        s_quitPowerManThread = true;
        rv = VPLCond_Signal(&s_powerManCondVar);
        if (rv != 0) {
            VPL_LIB_LOG_ERR(VPL_SG_POWER, "%s failed: %d", "VPLCond_Signal", rv);
            goto out;
        }
    }
    VPL_LIB_LOG_INFO(VPL_SG_POWER, "Waiting for worker thread");
    rv = VPLDetachableThread_Join(&s_powerManThread);
    if (rv != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_POWER, "%s failed: %d", "VPLDetachableThread_Join", rv);
        goto out;
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_powerManMutex));
        (IGNORE_RESULT)VPLCond_Destroy(&s_powerManCondVar);
        s_isRunning = false;
    }
out:
    return rv;
}

/// @return Do we need to signal the worker thread?
bool requestTimestampUpdate(VPLTime_t howLong)
{
    VPLTime_t request = VPLTime_GetTimeStamp() + howLong;
    if (request > s_nextAllowSleepTimestamp) {
        s_nextAllowSleepTimestamp = request;
        // We only need to signal the thread if it is waiting indefinitely (which it does when
        // allowing sleep).
        return s_allowingSleep;
    } else {
        // There was already a request that will keep the system awake longer than this one would.
        return false;
    }
}

s32 VPLPowerMan_KeepHostAwake(VPLPowerMan_Activity_t reason, VPLTime_t duration, VPLTime_t* timestamp_out)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_powerManMutex));
    if (!s_isRunning) {
        return VPL_ERR_NOT_RUNNING;
    }
    VPL_LIB_LOG_INFO(VPL_SG_POWER, "Request to postpone sleep for "FMT_VPLTime_t"us", duration);
    bool needToSignal = requestTimestampUpdate(duration);
    if (timestamp_out != NULL) {
        *timestamp_out = s_nextAllowSleepTimestamp;
    }
    s32 rv = VPL_OK;
    if (needToSignal) {
        rv = VPLCond_Signal(&s_powerManCondVar);
        if (rv != 0) {
            VPL_LIB_LOG_ERR(VPL_SG_POWER, "%s failed: %d", "VPLCond_Signal", rv);
        }
    }
    return rv;
}

//---------------------------------------
// TODO: Temporary; to avoid plumbing CCDConfig through vss_server for the 2.3.4 release.
//---------------------------------------
static VPLTime_t s_defaultDuration = VPLTIME_FROM_SEC(30);

static inline
VPLTime_t getDefaultHowLong()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_powerManMutex));
    return s_defaultDuration;
}

s32 VPLPowerMan_PostponeSleep(VPLPowerMan_Activity_t reason, VPLTime_t* timestamp_out)
{
    return VPLPowerMan_KeepHostAwake(reason, getDefaultHowLong(), timestamp_out);
}

void VPLPowerMan_SetDefaultPostponeDuration(VPLTime_t defaultDuration)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_powerManMutex));
    s_defaultDuration = defaultDuration;
}
//---------------------------------------
