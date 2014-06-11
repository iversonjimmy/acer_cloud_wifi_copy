/*
*                Copyright (C) 2005, BroadOn Communications Corp.
*
*   These coded instructions, statements, and computer programs contain
*   unpublished  proprietary information of BroadOn Communications Corp.,
*   and  are protected by Federal copyright law. They may not be disclosed
*   to  third  parties or copied or duplicated in any form, in whole or in
*   part, without the prior written consent of BroadOn Communications Corp.
*
*/

#include "vplex_plat.h"

#if defined(VPLTRACE_NO_TRACE) // VPLTRACE_NO_TRACE: strip VPL trace support.

// Leave this file empty. See vpl_no_trace.c.

#else // !VPLTRACE_NO_TRACE: compile in full VPLTrace support.

#include "vpl_types.h"
#include "vplu_debug.h"
#include "vplex_time.h"
#include "vpl_th.h"
#include "vplex_syslog.h"

#include "vplex_assert.h"
#include "vplex_safe_conversion.h"
#include "vplex_strings.h"
#include "vplex_trace.h"
#include "vplex_mem_utils.h"
#include "vplex_private.h"

#include <stdlib.h>
#include <ctype.h> // for isprint()

#ifdef ANDROID
# include <android/log.h>
# define compat_fprintf(fp_, ...)   __android_log_print(ANDROID_LOG_INFO, "vplex", __VA_ARGS__)
# define compat_vfprintf(fp_, fmt_, ap_)  __android_log_vprint(ANDROID_LOG_INFO, "vplex", (fmt_), (ap_))
#else
# define compat_fprintf(fp_, ...)   fprintf((fp_), __VA_ARGS__)
# define compat_vfprintf(fp_, fmt_, ap_)  vfprintf((fp_), (fmt_), (ap_))
#endif

#define DEF_TRACE_ENABLE                  true
#define DEF_TRACE_LEVEL                   TRACE_INFO
#define DEF_TRACE_FILE                    NULL

#define DEF_TRACE_SHOW_TIME_ABSOLUTE      false
#define DEF_TRACE_SHOW_TIME_RELATIVE      true
#define DEF_TRACE_SHOW_THREAD             true
#define DEF_TRACE_SHOW_FUNCTION           true

#define DEF_TRACE_VPL_ENABLE              true
#define DEF_TRACE_VPL_ENABLE_MASK         0xFFFFFFFF
#define DEF_TRACE_VPL_DISABLE_MASK        0
#define DEF_TRACE_VPL_LEVEL               VPLTRACE_DEFAULT

#define DEF_TRACE_LIB_ENABLE              true
#define DEF_TRACE_LIB_ENABLE_MASK         0xFFFFFFFF
#define DEF_TRACE_LIB_DISABLE_MASK        0
#define DEF_TRACE_LIB_LEVEL               VPLTRACE_DEFAULT

#define DEF_TRACE_APP_ENABLE              true
#define DEF_TRACE_APP_ENABLE_MASK         0xFFFFFFFF
#define DEF_TRACE_APP_DISABLE_MASK        0
#define DEF_TRACE_APP_LEVEL               VPLTRACE_DEFAULT

#define DEF_TRACE_BVS_ENABLE              true
#define DEF_TRACE_BVS_ENABLE_MASK         0xFFFFFFFF
#define DEF_TRACE_BVS_DISABLE_MASK        0
#define DEF_TRACE_BVS_LEVEL               VPLTRACE_DEFAULT

typedef struct {
    bool enable;
    // (evn = env var name)
    const char *enable_evn;
    unsigned    enable_mask;
    const char *enable_mask_evn;
    unsigned    disable_mask;
    const char *disable_mask_evn;
    VPLTrace_level level;
    const char *level_evn;
    VPLTrace_level levels[VPLTRACE_SUBGROUPS_PER_GROUP];
} _VPLTraceGrp;

static bool trace_initialized = false;
static bool trace_enable      = DEF_TRACE_ENABLE;
static VPLTrace_level vpl_trace_lv = DEF_TRACE_LEVEL;

//---------------------------------------------------------
// These are ignored when using a replacement logger.
//---------------------------------------------------------
static bool trace_show_thread = DEF_TRACE_SHOW_THREAD;
static bool trace_show_function = DEF_TRACE_SHOW_FUNCTION;
static bool trace_show_time_absolute = DEF_TRACE_SHOW_TIME_ABSOLUTE;
static bool trace_show_time_relative = DEF_TRACE_SHOW_TIME_RELATIVE;
static const char *trace_filename    = DEF_TRACE_FILE;
static char prefix[32];

//---------------------------------------------------------
// Env var names for base trace options
//---------------------------------------------------------

#define ENV_CORE_TRACE_ENABLE  "VPLTRACE_ENABLE"  /* 0 or not 0 */
#define ENV_CORE_TRACE_LEVEL   "VPLTRACE_LEVEL"   /* 0 or above */
#define ENV_CORE_TRACE_FILE    "VPLTRACE_FILE"    /* 0, filename, or stdout */

#define ENV_CORE_TRACE_TIME_ABS    "VPLTRACE_TIME_ABS"    /* 0 or not 0 */

/// ENV_CORE_TRACE_TIME_REL=1 specifies to display timestamped messages.
#define ENV_CORE_TRACE_TIME_REL    "VPLTRACE_TIME_REL"    /* 0 or not 0 */

/// ENV_CORE_TRACE_THREAD=1 specifies to display thread id/
#define ENV_CORE_TRACE_THREAD  "VPLTRACE_THREAD"  /* 0 or not 0 */

/// ENV_CORE_TRACE_FUNCTION=1 specifies to display function name/
#define ENV_CORE_TRACE_FUNCTION  "VPLTRACE_FUNCTION"  /* 0 or not 0 */

//  Env var names for Groups and Sub-groups
/*
* Group environment variable names are specified in
* the initialization of grps[] below.
*
* Bit 0-31 of the enable/disable masks refer to sub-groups 0-31.
*
* Sub-group env vars available for setting:
* - sub-group trace level
* - sub-group trace log filename
* .
* Sub-group env var names are formulated as "<GroupEnvVarName>_<sg>"
*   where <GroupEnvVarName> is the group env var name
*   and <sg> is the sub-group number.
* For example:
*   "VPLTRACE_VPL_ENABLE"   - specifies VPL group trace level.
*   "VPLTRACE_VPL_ENABLE_2" - specifies VPL sub-group 2 trace level.
*/

// TODO: would like a compile time assert: (ARRAY_ELEMENT_COUNT(grps) == (VPLTRACE_MAX_GRP + 1))

static _VPLTraceGrp grps[] = {
    { DEF_TRACE_VPL_ENABLE,            "VPLTRACE_VPL_ENABLE",
      DEF_TRACE_VPL_ENABLE_MASK,       "VPLTRACE_VPL_ENABLE_MASK",
      DEF_TRACE_VPL_DISABLE_MASK,      "VPLTRACE_VPL_DISABLE_MASK",
      DEF_TRACE_VPL_LEVEL,             "VPLTRACE_VPL_LEVEL",
      {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  }
    },
    { DEF_TRACE_LIB_ENABLE,            "VPLTRACE_LIB_ENABLE",
      DEF_TRACE_LIB_ENABLE_MASK,       "VPLTRACE_LIB_ENABLE_MASK",
      DEF_TRACE_LIB_DISABLE_MASK,      "VPLTRACE_LIB_DISABLE_MASK",
      DEF_TRACE_LIB_LEVEL,             "VPLTRACE_LIB_LEVEL",
      {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  }
    },
    { DEF_TRACE_APP_ENABLE,            "VPLTRACE_APP_ENABLE",
      DEF_TRACE_APP_ENABLE_MASK,       "VPLTRACE_APP_ENABLE_MASK",
      DEF_TRACE_APP_DISABLE_MASK,      "VPLTRACE_APP_DISABLE_MASK",
      DEF_TRACE_APP_LEVEL,             "VPLTRACE_APP_LEVEL",
      {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  }
    },
    { DEF_TRACE_BVS_ENABLE,            "VPLTRACE_BVS_ENABLE",
      DEF_TRACE_BVS_ENABLE_MASK,       "VPLTRACE_BVS_ENABLE_MASK",
      DEF_TRACE_BVS_DISABLE_MASK,      "VPLTRACE_BVS_DISABLE_MASK",
      DEF_TRACE_BVS_LEVEL,             "VPLTRACE_BVS_LEVEL",
      {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  }
    }
};

// TODO: would like a compile time assert: (ARRAY_ELEMENT_COUNT(trace_lev_str) == (TRACE_MAX_LEVEL + 1))

static const char* trace_lev_str[] =
{
    "^^DEFAULT^^",
    "##ERR##",
    "!!WRN!!",
    " INF ",
    " FN1 ",
    " FN2 ",
    " FN3 "
};

// TODO: would like a compile time assert: (ARRAY_ELEMENT_COUNT(trace_grp_str) == (VPLTRACE_MAX_GRP + 1))

static const char* trace_grp_str[] =
{
    "VPL",
    "LIB",
    "App",
    "BVS"
};

#ifdef VPL_PLAT_USE_PRINTF_MUTEX
static VPLMutex_t printfMutex;
#endif

#if defined(_WIN32) || defined(LINUX) || defined(__CDT_PARSER__)
static inline bool VPLTrace_GetBoolEnvVar(char const* env_str) {
    return (strtol(env_str, NULL, 0) != 0);
}
#endif

#ifndef VPL_PLAT_IS_WINRT
void VPLTrace_LoadConfig(void)
{
#if defined(_WIN32) || defined(LINUX) || defined(__CDT_PARSER__)
    char const* env_str;
    int const num_grps = ARRAY_ELEMENT_COUNT(grps);
    int i;

    for (i = 0;  i < num_grps;  ++i) {
        int j;
        if ((env_str = getenv(grps[i].enable_evn)) != NULL) {
            grps[i].enable = VPLTrace_GetBoolEnvVar(env_str);
        }
        if ((env_str = getenv(grps[i].enable_mask_evn)) != NULL) {
            grps[i].enable_mask = strtoul(env_str, NULL, 0);
        }
        if ((env_str = getenv(grps[i].disable_mask_evn)) != NULL) {
            grps[i].disable_mask = strtoul(env_str, NULL, 0);
        }
        if ((env_str = getenv(grps[i].level_evn)) != NULL) {
            grps[i].level = strtoul(env_str, NULL, 0);
        }
        for (j = 0; j < 32; ++j) {
            char level_env_var[256];
            // Make the 'level' environment variable string for the subgroup.
            {
                int len = VPL_snprintf(level_env_var, sizeof(level_env_var), "%s_%d", grps[i].level_evn, j);
                ASSERT_GREATER(len, 0, "%d");
                ASSERT_LESS(INT_TO_SIZE_T(len), sizeof(level_env_var), FMTuSizeT);
            }
            if ((env_str = getenv(level_env_var)) != NULL) {
                grps[i].levels[j] = strtoul(env_str, NULL, 0);
            }
        }
    }
    if ((env_str = getenv(ENV_CORE_TRACE_ENABLE)) != NULL) {
        trace_enable = VPLTrace_GetBoolEnvVar(env_str);
    }
    if ((env_str = getenv(ENV_CORE_TRACE_LEVEL)) != NULL) {
        vpl_trace_lv = strtoul(env_str, NULL, 0);
    }
    if ((env_str = getenv(ENV_CORE_TRACE_TIME_ABS)) != NULL) {
        trace_show_time_absolute = VPLTrace_GetBoolEnvVar(env_str);
    }
    if ((env_str = getenv(ENV_CORE_TRACE_TIME_REL)) != NULL) {
        trace_show_time_relative = VPLTrace_GetBoolEnvVar(env_str);
    }
    if ((env_str = getenv(ENV_CORE_TRACE_THREAD)) != NULL) {
        trace_show_thread = VPLTrace_GetBoolEnvVar(env_str);
    }
    if ((env_str = getenv(ENV_CORE_TRACE_FUNCTION)) != NULL) {
        trace_show_function = VPLTrace_GetBoolEnvVar(env_str);
    }
    if ((env_str = getenv(ENV_CORE_TRACE_FILE)) != NULL) {
        trace_filename = env_str;  /* Note: not copied */
    }
#endif
}
#endif

static inline void warnNotInit()
{
    compat_fprintf(stderr, "NOTE: it is recommended that you call VPLTrace_Init() before using other VPLTrace functions!\n");
    VPLTrace_Init(NULL);
}

const char* VPLTrace_LevelStr(VPLTrace_level level)
{
    if (level == TRACE_TEMP_DEBUG) {
        return "***DEBUG***";
    }
    else if (level == TRACE_ALWAYS) {
        return " ALL ";
    }
    else if ((level < 0) || (level >= (VPLTrace_level)ARRAY_ELEMENT_COUNT(trace_lev_str))) {
        return "???UNKNOWN???";
    }
    else {
        return trace_lev_str[level];
    }
}

const char* VPLTrace_GroupStr(int grp_num)
{
    if ((grp_num < 0) || (grp_num >= (int)ARRAY_ELEMENT_COUNT(trace_grp_str))) {
        return "???UNKNOWN???";
    }
    else {
        return trace_grp_str[grp_num];
    }
}

//---------------------------------------------------------
// These are ignored when using a replacement logger.
//---------------------------------------------------------

void VPLTrace_SetPrefix(const char *str)
{
    if (!trace_initialized) {
        warnNotInit();
    }
    VPLString_SafeStrncpy(prefix, sizeof(prefix), str);
}

bool VPLTrace_SetShowFunction(bool show_function)
{
    bool prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = trace_show_function;
    trace_show_function = show_function;
    return prev;
}

bool VPLTrace_SetShowThread(bool show_thread)
{
    bool prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = trace_show_thread;
    trace_show_thread = show_thread;
    return prev;
}

bool VPLTrace_SetShowTimeAbs(bool show_time)
{
    bool prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = trace_show_time_absolute;
    trace_show_time_absolute = show_time;
    return prev;
}

bool VPLTrace_SetShowTimeRel(bool show_time)
{
    bool prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = trace_show_time_relative;
    trace_show_time_relative = show_time;
    return prev;
}

const char* VPLTrace_SetTraceFile(const char* filename)
{
    const char *prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = trace_filename;
    trace_filename = filename;
    VPL_LIB_LOG(TRACE_ALWAYS, VPL_SG_TRACING,
            "Changing VPLex log file from \"%s\" to \"%s\"",
            ((prev != NULL) ? prev : ""),
            ((filename != NULL) ? filename : ""));
    return prev;
}

static FILE* VPL_AcquireLogfile(const char* logfileName)
{
    FILE* result;
#if defined(_WIN32) || defined(LINUX)
    if ( (logfileName != NULLPTR) &&
         (logfileName[0] != '\0') &&
         (!VPLString_EqualLiteral(logfileName, "stdout")) )
    {
        result = fopen(logfileName, "a");
    }
    else
    {
        result = NULLPTR;
    }
#else
    UNUSED(logfileName);
    result = NULLPTR;
#endif
    return result;
}

static void VPL_ReleaseLogfile(FILE* logfile)
{
#if defined(_WIN32) || defined(LINUX)
    if (logfile != NULLPTR) {
        fclose(logfile);
    }
    else {
        fflush(stdout);
    }
#else
    UNUSED(logfile);
#endif
}

static void VPLTrace_vprintf(
        VPLTrace_level level,
        const char* filename,
        int         lineNum,
        const char* functionName,
        VPL_BOOL    newline,
        int         groupNum,
        int         subgrpNum,
        const char* fmt,
        va_list     ap)
{
    char msgBuf[VPL_BUFFER_SIZED_FOR_IPC(768)] VPL_BUFFER_FOR_IPC_ATTRIBUTE;
    const size_t MSG_BUF_LEN = sizeof(msgBuf);
    size_t currMsgLen = 0;
#ifndef _MSC_VER
    int syslogRetVal;
#endif
    bool derived_show_relative_time;
    size_t endHeader;

    if (!trace_initialized) {
        warnNotInit();
    }

    ASSERT_LESS_OR_EQ(MSG_BUF_LEN, VPLSYSLOG_MAX_MSG_LEN, FMTuSizeT);

    if (prefix[0] != '\0') {
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], ARRAY_ELEMENT_COUNT(prefix),
                "%s", prefix);
    }

    derived_show_relative_time = trace_show_time_relative;
    if (trace_show_time_absolute) {
#if VPL_PLAT_HAS_CALENDAR_TIME
        VPLTime_CalendarTime_t calendarTime;
        VPLTime_GetCalendarTimeLocal(&calendarTime);
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                FMT_VPLTime_CalendarTime_t, VAL_VPLTime_CalendarTime_t(calendarTime));
#else
        derived_show_relative_time = true;
#endif
    }

    if (derived_show_relative_time) {
        // "seconds.microseconds "
        // "0000.000000 "
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                "%04"PRI_VPLTime_t".%06"PRIu32,
                (VPLTime_GetTimeStamp() / MICROSECS_PER_SEC),
                U64_TO_U32(VPLTime_GetTimeStamp() % MICROSECS_PER_SEC));
    }

    {
        char const* level_str = VPLTrace_LevelStr(level);
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                "%s", level_str);
    }

    {
        char const* group_str = VPLTrace_GroupStr(groupNum);
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                "%s%d,", group_str, subgrpNum);
    }

    if (trace_show_thread) {
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                FMT_VPLThread_t"|", VAL_VPLThread_t(VPLThread_Self()));
    }

    if (filename != NULL) {
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                "%s:%d:", VPLString_GetFilenamePart(filename), lineNum);
    }

    if ((functionName != NULL) && trace_show_function) {
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                "%s|", functionName);
    }
    
    {
        ASSERT_GREATER(MSG_BUF_LEN, currMsgLen, FMTuSizeT);
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                " ");
    }

    // Remember the size of the header, in case the message overflows.
    endHeader = MIN(currMsgLen, MSG_BUF_LEN);

    if (currMsgLen <= MSG_BUF_LEN) {
        va_list apcopy; // Need to copy first, since we may need it again later.
        va_copy(apcopy, ap);
        currMsgLen += VPL_vsnprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen,
                fmt, apcopy);
        va_end(apcopy);
    }

    if (newline) {
        /// Make sure the newline is included, even if the message was already truncated.
        if (currMsgLen >= MSG_BUF_LEN) {
            currMsgLen = MSG_BUF_LEN - 1;
        }
        currMsgLen += VPL_snprintf(&msgBuf[currMsgLen], MSG_BUF_LEN - currMsgLen, "\n");
    }

#ifndef _MSC_VER
    // Send to syslog.
    syslogRetVal = VPLSyslog(msgBuf, MIN(currMsgLen, MSG_BUF_LEN));
#endif

    // Send to file/stdout.
#ifdef VPL_PLAT_USE_PRINTF_MUTEX
    VPLMutex_Lock(&printfMutex);
#endif
    {
        bool write_to_stdout = true;
        {
            FILE* const fp = VPL_AcquireLogfile(trace_filename);

            /// Log to the file (if one was specified).
            if (fp != NULLPTR && fp != stdout) {
                if (currMsgLen < MSG_BUF_LEN) {
                    // Normal case:
                    compat_fprintf(fp, "%s", msgBuf);
                } else {
                    // Overflowed the fixed-size buffer.
                    // Print just the header from the fixed-size buffer.

                    va_list apcopy; // Need to copy first, since we may need it again later.

                    compat_fprintf(fp, "%.*s", (int)endHeader, msgBuf);

                    // vfprintf the actual message.
                    va_copy(apcopy, ap);
                    compat_vfprintf(fp, fmt, apcopy);
                    va_end(apcopy);

#ifndef ANDROID
                    if (newline) {
                        compat_fprintf(fp, "\n");
                    }
#endif
                }
                // Logged to file; only send to stdout if it's a "warning" (or more severe).
                if (level >= TRACE_INFO) {
                    write_to_stdout = false;
                }
            }
            VPL_ReleaseLogfile(fp);
        }
        // Also log to stdout.
        {
            if (write_to_stdout) {
                FILE* const fp = stdout;
                if (currMsgLen < MSG_BUF_LEN) {
                    compat_fprintf(fp, "%s", msgBuf);
                } else {
                    compat_fprintf(fp, "%.*s", (int)endHeader, msgBuf);
                    compat_vfprintf(fp, fmt, ap);
#ifndef ANDROID
                    if (newline) {
                        compat_fprintf(fp, "\n");
                    }
#endif
                }
                fflush(fp);
            }
        }
    }
#ifdef VPL_PLAT_USE_PRINTF_MUTEX
    VPLMutex_Unlock(&printfMutex);
#endif
}

//---------------------------------------------------------

void VPLTrace_PrintLevelInfo(int grp_num, int sub_grp)
{
    unsigned sub_grp_mask = (unsigned)1 << sub_grp;
    _VPLTraceGrp* const grp = &grps[grp_num];

    if (!trace_initialized) {
        warnNotInit();
    }

    compat_fprintf(stdout, "sg%d=%s%s%s, group%d=%s%s, global=%s%s",
           sub_grp,
           ((sub_grp_mask & grp->enable_mask) ? "" : "(!ENABLED)"),
           ((sub_grp_mask & grp->disable_mask) ? "(DISABLED)" : ""),
           VPLTrace_LevelStr(grp->levels[sub_grp]),
           grp_num,
           (grp->enable ? "" : "(DISABLED)"),
           VPLTrace_LevelStr(grp->level),
           VPLTrace_LevelStr(vpl_trace_lv),
           (trace_enable ? "" : "(!ENABLED)"));
}

bool VPLTrace_On(VPLTrace_level level, int grp_num, int sub_grp)
{
    if (!trace_initialized) {
        warnNotInit();
    }
    if ( (grp_num > VPLTRACE_MAX_GRP) || (grp_num < 0) ||
         (sub_grp >= VPLTRACE_SUBGROUPS_PER_GROUP) || sub_grp < 0) {
        FAILED_ASSERT("Bad group/subgroup: %d/%d", grp_num, sub_grp);
        return false;;
    }
    if (level > TRACE_MAX_LEVEL) {
        level = TRACE_MAX_LEVEL;
    }

    if ( (level == TRACE_TEMP_DEBUG) || (level == TRACE_ALWAYS) ) {
        return true;
    }
    else {
        const VPLTrace_level threshold = VPLTrace_GetSgLevel(grp_num, sub_grp);
        if (threshold == TRACE_OFF) {
            return false;
        }
        else {
            return (level <= threshold);
        }
    }
}

void VPLTrace_TraceEx(VPLTrace_level level,
                      const char* sourceFilename,
                      int lineNum,
                      const char* functionName,
                      bool appendNewline,
                      int grp_num,
                      int sub_grp,
                      const char* fmt,
                      ...)
{
    va_list ap;

    va_start(ap, fmt);
    VPLTrace_VTraceEx(level, sourceFilename, lineNum,
                      functionName, appendNewline,
                      grp_num, sub_grp, fmt, ap);
    va_end(ap);
}



VPLTrace_level VPLTrace_GetSgLevel(int grp_num, int sub_grp)
{
    unsigned sub_grp_mask = (unsigned)1 << sub_grp;
    _VPLTraceGrp* const grp = &grps[grp_num];
    VPLTrace_level maxlevel;

    if (!trace_initialized) {
        warnNotInit();
    }

    if (grp->levels[sub_grp] != VPLTRACE_DEFAULT) {
        maxlevel = grp->levels[sub_grp];
    } else if (grp->level != VPLTRACE_DEFAULT) {
        maxlevel = grp->level;
    } else {
        maxlevel = vpl_trace_lv;
    }

    if (!trace_enable ||
        !grp->enable  ||
        !(sub_grp_mask & grp->enable_mask) ||
         (sub_grp_mask & grp->disable_mask)) {
        return TRACE_OFF;
    } else {
        return maxlevel;
    }
}

bool VPLTrace_SetEnable(bool enable)
{
    bool prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = trace_enable;
    trace_enable = enable;
    return prev;
}


bool VPLTrace_SetGroupEnable(int grp, bool enable)
{
    bool prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = grps[grp].enable;
    grps[grp].enable = enable;
    return prev;
}

static VPLTrace_level VPLTrace_SetTraceLevelPrivate(VPLTrace_level newLevel, VPLTrace_level* levelPtr)
{
    VPLTrace_level prev;
    ASSERT_NOT_NULL(levelPtr);
    prev = *levelPtr;
    if (newLevel > TRACE_MAX_LEVEL) {
        *levelPtr = TRACE_MAX_LEVEL;
    }
    else if (newLevel < VPLTRACE_DEFAULT) {
        *levelPtr = VPLTRACE_DEFAULT;
    }
    else {
        *levelPtr = newLevel;
    }
    return prev;
}

VPLTrace_level VPLTrace_GetBaseLevel(void)
{
    if (!trace_initialized) {
        warnNotInit();
    }
    return vpl_trace_lv;
}

VPLTrace_level VPLTrace_SetBaseLevel(VPLTrace_level level)
{
    if (!trace_initialized) {
        warnNotInit();
    }
    return VPLTrace_SetTraceLevelPrivate(level, &vpl_trace_lv);
}

VPLTrace_level VPLTrace_GetGroupLevel(int grp_num)
{
    if (!trace_initialized) {
        warnNotInit();
    }
    return grps[grp_num].level;
}

VPLTrace_level VPLTrace_SetGroupLevel(int grp, VPLTrace_level level)
{
    if (!trace_initialized) {
        warnNotInit();
    }
    if ( (grp > VPLTRACE_MAX_GRP) || (grp < 0) ) {
        return VPL_ERR_INVALID;
    }
    return VPLTrace_SetTraceLevelPrivate(level, &grps[grp].level);
}

VPLTrace_level VPLTrace_SetSgLevel(int grp, int sub_grp, VPLTrace_level level)
{
    if ( (grp > VPLTRACE_MAX_GRP) || (grp < 0) ||
         (sub_grp >= VPLTRACE_SUBGROUPS_PER_GROUP) || (sub_grp < 0) ) {
        return VPL_ERR_INVALID;
    }
    if (!trace_initialized) {
        warnNotInit();
    }
    return VPLTrace_SetTraceLevelPrivate(level, &grps[grp].levels[sub_grp]);
}

unsigned VPLTrace_SetGroupEnableMask(int grp, unsigned mask)
{
    unsigned prev;
    if (!trace_initialized) {
        warnNotInit();
    }
    if (grp > VPLTRACE_MAX_GRP) {
        return 0;
    }
    prev = grps[grp].enable_mask;
    grps[grp].enable_mask = mask;
    return prev;
}

unsigned VPLTrace_SetGroupDisableMask(int grp, unsigned mask)
{
    unsigned prev;
    if (grp > VPLTRACE_MAX_GRP) {
        return 0;
    }
    if (!trace_initialized) {
        warnNotInit();
    }
    prev = grps[grp].disable_mask;
    grps[grp].disable_mask = mask;
    return prev;
}

/* dump len bytes into the provided line buffer in readable form. */
static void dump16(char* line, char const* bytes, size_t len)
{
    if (len > 0) {
        size_t i;
        size_t pos = 0;

        pos += sprintf(line, "%10p ", bytes);

        for( i = 0 ; i < len ; i++ ) { /* Print the hex dump for len
                                         chars. */
            unsigned int outbyte = (unsigned char)bytes[i];
            pos += sprintf(line + pos, " %02x", outbyte);
        }
        for( i = len ; i < 16 ; i++ ) { /* Pad out the dump field to the
                                           ASCII field. */
            pos += sprintf(line + pos, "   ");
        }

        pos += sprintf(line + pos, " ");

        for( i = 0 ; i < len ; i++ ) { /* Now print the ASCII field. */
            char c = (char)(0x7f & bytes[i]);      /* Mask out bit 7 */

            if (!(isprint(c))) {    /* If not printable */
                pos += sprintf(line + pos, ".");          /* print a dot */
            } else {
                pos += sprintf(line + pos, "%c", c);      /* else display it */
            }
        }
    }
}

void VPLTrace_DumpBuf(VPLTrace_level level,
                      const char* sourceFilename,
                      int lineNum,
                      const char* functionName,
                      bool appendNewline,
                      int grp_num,
                      int sub_grp,
                      const void* buf,
                      size_t len)
{
    if (!VPLTrace_On(level, grp_num, sub_grp)) {
        return;
    }

    {
        // Buffer good for 16-bytes of readable hex output
        char line[100];
        size_t offset;
        
        for (offset = 0; offset < len; offset += 16) {
            size_t nchars = len - offset;
            if(nchars > 16) { nchars = 16; }
            dump16(line, (const char*)VPLPtr_AddUnsigned(buf, offset), nchars);
            VPLTrace_TraceEx(level, sourceFilename, lineNum,
                             functionName, appendNewline, grp_num,
                             sub_grp, "%s", line);
        }
    }
}

//---------------------------------------------------------
// Replacement logger.
//---------------------------------------------------------
static VPLTrace_LogCallback_t s_logging_callback = VPLTrace_vprintf;

VPLTrace_LogCallback_t VPLTrace_SetReplacementLogger(VPLTrace_LogCallback_t fn)
{
    VPLTrace_LogCallback_t prev = s_logging_callback;
    if (!trace_initialized) {
        warnNotInit();
    }
    if (fn == NULL) {
        s_logging_callback = VPLTrace_vprintf;
    } else {
        s_logging_callback = fn;
    }
    return prev;
}

//---------------------------------------------------------

void VPLTrace_VTraceEx(VPLTrace_level level,
                       const char* sourceFilename,
                       int lineNum,
                       const char* functionName,
                       bool appendNewline,
                       int grp_num,
                       int sub_grp,
                       const char* fmt,
                       va_list ap)
{
    if (!VPLTrace_On(level, grp_num, sub_grp)) {
        return;
    }
    if (functionName == NULL) {
        functionName = "<n/a>";
    }
    s_logging_callback(
            level,
            sourceFilename,
            lineNum,
            functionName,
            appendNewline,
            grp_num,
            sub_grp,
            fmt,
            ap);
}

/// A #VPL_DebugCallback_t.
static void
vplDebugCallback(const VPL_DebugMsg_t* data)
{
    VPLTrace_level vplexLevel;
    switch(data->level) {
    case VPLDEBUG_LEVEL_FATAL:
        vplexLevel = TRACE_ERROR;
        break;
    case VPLDEBUG_LEVEL_WARN:
        vplexLevel = TRACE_WARN;
        break;
    case VPLDEBUG_LEVEL_INFO:
        vplexLevel = TRACE_INFO;
        break;
    default:
        VPL_LIB_LOG(TRACE_ERROR, VPL_SG_TRACING, "Unexpected level %d", data->level);
        vplexLevel = TRACE_ALWAYS;
        break;
    }
    VPLTrace_TraceEx(vplexLevel, data->file, data->line, NULL, VPL_TRUE,
            VPLTRACE_GRP_VPL, VPL_SG_MISC, "%s", data->msg);
}

void VPLTrace_Init(VPLTrace_LogCallback_t logCallback)
{
    if (trace_initialized) {
        if (logCallback != s_logging_callback) {
            VPL_LIB_LOG(TRACE_ERROR, VPL_SG_TRACING,
                    "VPLTrace_Init was already called, and the logCallback is different!");
        } else {
            VPL_LIB_LOG(TRACE_WARN, VPL_SG_TRACING,
                    "VPLTrace_Init was already called");
        }
        return;
    }
#ifndef VPL_PLAT_IS_WINRT
    VPLTrace_LoadConfig();
#endif

#ifdef VPL_PLAT_USE_PRINTF_MUTEX
    VPLMutex_Init(&printfMutex);
#endif

    // Make sure we set this to true before logging anything, such as the current date/time.
    trace_initialized = true;

    // Call for the side-effect of initializing the starting timestamp.
    (IGNORE_RESULT) VPLTime_GetTimeStamp();

    if (logCallback != NULL) {
        VPLTrace_SetReplacementLogger(logCallback);
    }

    // Log the current date/time.
#if VPL_PLAT_HAS_CALENDAR_TIME
    {
        VPLTime_CalendarTime_t calendarTimeLocal;
        VPLTime_GetCalendarTimeLocal(&calendarTimeLocal);
        VPL_LIB_LOG(TRACE_ALWAYS, VPL_SG_TRACING,
                    "VPLex trace start time: "FMT_VPLTime_CalendarTime_t,
                    VAL_VPLTime_CalendarTime_t(calendarTimeLocal));
    }
#endif
    if (logCallback == NULL) {
        if (trace_filename != NULL) {
            VPL_LIB_LOG(TRACE_ALWAYS, VPL_SG_TRACING,
                "VPLex log file: \"%s\"", trace_filename);
        } else {
            VPL_LIB_LOG(TRACE_ALWAYS, VPL_SG_TRACING, "No VPLex log file (stdout only)");
        }
    }

    // Hook VPL debug messages.
    VPL_RegisterDebugCallback(vplDebugCallback);
}

#endif /* !defined(VPLTRACE_NO_TRACE) */
