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


#ifndef __VPLEX_TRACE_H__
#define __VPLEX_TRACE_H__

#include "vplex_plat.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
There are several trace groups that can be individually enabled/disabled
programmatically or via environment variables:
- #VPLTRACE_GRP_VPL is for VPL and VPLex libraries.
- #VPLTRACE_GRP_LIB is for our internal libraries.
- #VPLTRACE_GRP_APP can be used by any app/test/demo.
.

Environment variable names and defaults are specified in vplex_trace.c.

The trace parameters that apply to all groups are referred to
as the base trace parameters.

Each trace group has 32 sub-groups with an enable and disable mask
that can be overridden by environment variables.

The meaning of the 32 sub-groups can be defined independently
for each group.

A trace level and log file can be specified independently for each
group and sub-group.

Each trace message has a group, sub-group, and level designation.

By default, all groups and sub-groups are enabled and use the 
base trace level and stdout.

The base trace level is initialized to TRACE_INFO.

If a sub-group trace level is 0 (#VPLTRACE_DEFAULT), the group trace level
is used.  If the group trace level is also 0, the base trace level is used.

If a sub-group trace log filename is an empty string or NULL, the group trace
log file is used.
If the group trace log filename is also empty or NULL, the base trace log
file is used.
If the base trace log filename is also empty or NULL, the message will be
written to stdout.

The string "stdout" can be specified as the log filename of a group or
sub-group to force it to stdout regardless of the base log file value.
"stdout" can also be specified for the base log filename instead of NULL.
*/

//-------------------------------------
// Trace level definitions
//-------------------------------------

typedef s32 VPLTrace_level;

/// Something bad happened, but the problem could be external to this process.
#define TRACE_ERROR      1

/// Expected error cases (may be due to user error).
#define TRACE_WARN       2

#define TRACE_INFO       3

#define TRACE_FINE       4

#define TRACE_FINER      5

#define TRACE_FINEST     6

#define TRACE_OFF        7

#define TRACE_MAX_LEVEL  7

/// Indicates "use default".
/// If a subgroup's level is set to #VPLTRACE_DEFAULT, its actual setting will be inherited from its
/// parent group's setting (#VPLTrace_SetGroupLevel()).
/// If a group's level is set to #VPLTRACE_DEFAULT, its actual setting will be inherited from the global
/// setting (#VPLTrace_SetBaseLevel()).
#define VPLTRACE_DEFAULT        0

/// Indicates that the log message should always be printed; takes precedence over other settings.
/// Intended to allow using VPLTrace as "a better printf".
#define TRACE_ALWAYS     -1

/// Indicates that the log message should always be printed and emphasized; takes precedence over other settings.
/// This should generally only be used for temporary debugging/testing.
#define TRACE_TEMP_DEBUG      -2

//-------------------------------------
// Trace group definitions.
//-------------------------------------

#define VPLTRACE_GRP_VPL  0
#define VPLTRACE_GRP_LIB  1
#define VPLTRACE_GRP_APP  2
#define VPLTRACE_GRP_BVS  3
#define VPLTRACE_MAX_GRP  3

/// @deprecated Please switch away from the short name.
#define TRACE_APP  VPLTRACE_GRP_APP

/// @deprecated Please switch away from the short name.
#define TRACE_BVS  VPLTRACE_GRP_BVS

#define VPLTRACE_SUBGROUPS_PER_GROUP  32

//-------------------------------------
// Logging sub-groups for the #VPLTRACE_GRP_VPL group.
//-------------------------------------

#define VPL_SG_MISC  0
#define VPL_SG_PLAT  1
#define VPL_SG_TH    2
#define VPL_SG_MEM   3
#define VPL_SG_MEM_MANAGER  4
#define VPL_SG_RM        5
#define VPL_SG_SOCKET    6
#define VPL_SG_TIMER     7
#define VPL_SG_SHIM      8
#define VPL_SG_CERT      9
/// HTTP (also includes SOAP)
#define VPL_SG_HTTP     10
#define VPL_SG_COMMUNITY  11
#define VPL_SG_VS       12
#define VPL_SG_IAS      13
#define VPL_SG_FS       14
#define VPL_SG_NUS      15
#define VPL_SG_POWER    16
#define VPL_SG_TRACING  31

//-------------------------------------
// Logging sub-groups for the #VPLTRACE_GRP_LIB group.
//-------------------------------------

#define VPL_SG_LIB_PROTORPC  2
#define VPL_SG_LIB_VSS  5

//-------------------------------------
// Trace functions
//-------------------------------------

#define VPLTRACE_LOG(level, group, subgroup, ...) \
    VPLTrace_TraceEx(level, _FILE_NO_DIRS_, __LINE__, __func__, true, group, subgroup, __VA_ARGS__)

#define VPLTRACE_VLOG(level, group, subgroup, ...) \
    VPLTrace_VTraceEx(level, _FILE_NO_DIRS_, __LINE__, __func__, true, group, subgroup, __VA_ARGS__)

#define VPLTRACE_DUMP_BUF(level, group, subgroup, buf, len) \
    VPLTrace_DumpBuf(level, _FILE_NO_DIRS_, __LINE__, __func__, true, group, subgroup, buf, len)

// Always log.
#define VPLTRACE_LOG_ALWAYS(group, subgroup, ...) \
    VPLTRACE_LOG(TRACE_ALWAYS, group, subgroup, __VA_ARGS__)

#define VPLTRACE_DUMP_BUF_ALWAYS(group, subgroup, buf, len) \
    VPLTRACE_DUMP_BUF(TRACE_ALWAYS, group, subgroup, buf, len)

// Log an error.
#define VPLTRACE_LOG_ERR(group, subgroup, ...) \
    VPLTRACE_LOG(TRACE_ERROR, group, subgroup, __VA_ARGS__)

#define VPLTRACE_DUMP_BUF_ERR(group, subgroup, buf, len) \
    VPLTRACE_DUMP_BUF(TRACE_ERROR, group, subgroup, buf, len)

// Log a warning.
#define VPLTRACE_LOG_WARN(group, subgroup, ...) \
    VPLTRACE_LOG(TRACE_WARN, group, subgroup, __VA_ARGS__)

#define VPLTRACE_DUMP_BUF_WARN(group, subgroup, buf, len) \
    VPLTRACE_DUMP_BUF(TRACE_WARN, group, subgroup, buf, len)

#ifdef VPLTRACE_TRACE_BE_VERY_QUIET
#  define VPLTRACE_LOG_INFO(group, subgroup, ...)
#  define VPLTRACE_LOG_FINE(group, subgroup, ...)
#  define VPLTRACE_DUMP_BUF_INFO(group, subgroup, buf, len)
#  define VPLTRACE_DUMP_BUF_FINE(group, subgroup, buf, len)
#else
#  define VPLTRACE_LOG_INFO(group, subgroup, ...) \
        VPLTRACE_LOG(TRACE_INFO, group, subgroup, __VA_ARGS__)
#  define VPLTRACE_LOG_FINE(group, subgroup, ...) \
        VPLTRACE_LOG(TRACE_FINE, group, subgroup, __VA_ARGS__)
#  define VPLTRACE_DUMP_BUF_INFO(group, subgroup, buf, len) \
        VPLTRACE_DUMP_BUF(TRACE_INFO, group, subgroup, buf, len)
#  define VPLTRACE_DUMP_BUF_FINE(group, subgroup, buf, len) \
        VPLTRACE_DUMP_BUF(TRACE_FINE, group, subgroup, buf, len)
#endif

#if defined(VPLTRACE_BE_QUIET) || defined(VPLTRACE_BE_VERY_QUIET)
#  define VPLTRACE_LOG_FINER(group, subgroup, ...)
#  define VPLTRACE_LOG_FINEST(group, subgroup, ...)
#  define VPLTRACE_DUMP_BUF_FINER(group, subgroup, buf, len)
#  define VPLTRACE_DUMP_BUF_FINEST(group, subgroup, buf, len)
#else
#  define VPLTRACE_LOG_FINER(group, subgroup, ...) \
        VPLTRACE_LOG(TRACE_FINER, group, subgroup, __VA_ARGS__)
#  define VPLTRACE_LOG_FINEST(group, subgroup, ...) \
        VPLTRACE_LOG(TRACE_FINEST, group, subgroup, __VA_ARGS__)
#  define VPLTRACE_DUMP_BUF_FINER(group, subgroup, buf, len) \
        VPLTRACE_DUMP_BUF(TRACE_FINER, group, subgroup, buf, len)
#  define VPLTRACE_DUMP_BUF_FINEST(group, subgroup, buf, len) \
        VPLTRACE_DUMP_BUF(TRACE_FINEST, group, subgroup, buf, len)
#endif

//% TODO: This doesn't currently write to the logs (only stdout).
/// Requests that a stack trace be printed.  May be ignored on some platforms.
#if defined __GLIBC__ && !defined __UCLIBC__
void VPLStack_Trace(void);
#endif
// ############################################################################
#ifndef VPLTRACE_NO_TRACE

typedef void (*VPLTrace_LogCallback_t)(
        VPLTrace_level level,
        const char* sourceFilename,
        int lineNum,
        const char* functionName,
        VPL_BOOL appendNewline,
        int grp_num,
        int sub_grp,
        const char* fmt,
        va_list ap);

/// Initialize the logging module.
/// @param logCallback Can be NULL to use the default.
void VPLTrace_Init(VPLTrace_LogCallback_t logCallback);

#ifndef VPL_PLAT_IS_WINRT
/// Pull in the tracing configuration from environment variables (if any).
void VPLTrace_LoadConfig(void);
#endif

/// Plug in a different log function to handle logging.
/// Can specify NULL to restore the default.
/// @return The previous setting.
VPLTrace_LogCallback_t VPLTrace_SetReplacementLogger(VPLTrace_LogCallback_t fn);

/* Changes to trace options are usually done
*  by environment variable before starting the app.
*
*  Trace options are updated from env vars only on the first
*  call to a trace function.
*
*  The "set" functions below return the prev option value.
*
*  sub_grp 0-31 corresponds to sub_grp mask bits 0-31
*/

VPLTrace_level VPLTrace_GetBaseLevel(void);
VPLTrace_level VPLTrace_SetBaseLevel(VPLTrace_level level);

VPLTrace_level VPLTrace_GetGroupLevel(int grp_num);
VPLTrace_level VPLTrace_SetGroupLevel(int grp, VPLTrace_level level);

VPLTrace_level VPLTrace_GetSgLevel(int grp_num, int sub_grp);
VPLTrace_level VPLTrace_SetSgLevel(int grp, int sub_grp, VPLTrace_level level);

/// @return The previous setting.
bool VPLTrace_SetEnable(bool enable);
bool VPLTrace_SetGroupEnable(int grp_num, bool enable);
uint32_t VPLTrace_SetGroupEnableMask( int grp, uint32_t mask);
uint32_t VPLTrace_SetGroupDisableMask(int grp, uint32_t mask);

/// @return Is the specified level active for the specified subgroup.
bool VPLTrace_On(VPLTrace_level level, int grp, int sub_grp);

//----

const char* VPLTrace_LevelStr(VPLTrace_level level);

const char* VPLTrace_GroupStr(int grp_num);

/// printf's full info about the subgroup's current setting.
void VPLTrace_PrintLevelInfo(int grp_num, int sub_grp);

//----

/// Direct call for when you want to pass the line number; you should usually use the
/// VPLTRACE_LOG*() macros instead.
void VPLTrace_TraceEx(VPLTrace_level level,
                      const char* sourceFilename,
                      int lineNum,
                      const char* functionName,
                      bool appendNewline,
                      int grp_num,
                      int sub_grp,
                      const char* fmt,
                      ...) ATTRIBUTE_PRINTF(8, 9);

/// Direct call for when you want to pass a va_list; you should usually use the
/// VPLTRACE_LOG*() macros instead.
void VPLTrace_VTraceEx(VPLTrace_level level,
                       const char* sourceFilename,
                       int lineNum,
                       const char* functionName,
                       bool appendNewline,
                       int grp_num,
                       int sub_grp,
                       const char* fmt,
                       va_list ap);

/// Direct call for when you want to pass the line number; you should usually use the
/// VPLTRACE_DUMP_BUF*() macros instead.
void VPLTrace_DumpBuf(VPLTrace_level level,
                      const char* sourceFilename,
                      int lineNum,
                      const char* functionName,
                      bool appendNewline,
                      int grp_num,
                      int sub_grp,
                      const void* buf,
                      size_t len);

//---------------------------------------------------------
// These are ignored when using a replacement logger.
//---------------------------------------------------------

/// Specify an optional prefix for each log line.
void VPLTrace_SetPrefix(const char *prefix);

/// @return The previous setting.
bool VPLTrace_SetShowFunction(bool show_function);

/// @return The previous setting.
bool VPLTrace_SetShowThread(bool show_thread);

/// Specify if the real-world clock time should appear in each log entry.
/// @note This is orthogonal to #VPLTrace_SetShowTimeRel(); they can
///     both be active simultaneously.
/// @return The previous setting.
bool VPLTrace_SetShowTimeAbs(bool show_time);

/// Specify if the "number of milliseconds since program start" should appear in each log entry.
/// @note This is orthogonal to #VPLTrace_SetShowTimeAbs(); they can
///     both be active simultaneously.
/// @return The previous setting.
bool VPLTrace_SetShowTimeRel(bool show_time);

/// @note This keeps a pointer to filename and expects it to remain valid and constant.
const char* VPLTrace_SetTraceFile(const char* filename);

// ############################################################################
#else    // !VPLTRACE_NO_TRACE:  normal trace support.

#define VPLTrace_init()
#define VPLTrace_load_config()
#define VPLTrace_SetEnable(enable)                   false
#define VPLTrace_SetBaseLevel(level)                     ((VPLTrace_level)0)
#define VPLTrace_SetShowTimeAbs(show_time)    false
#define VPLTrace_SetShowTimeRel(show_time)    false
#define VPLTrace_SetShowThread(show_thread)         false
#define VPLTrace_SetShowFunction(show_function)     false

#define VPLTrace_SetGroupEnable(grp, enable)          false
#define VPLTrace_SetGroupLevel(grp, level)            ((VPLTrace_level)0)
#define VPLTrace_SetSgLevel(grp, sub_grp, level)    ((VPLTrace_level)0)

#define VPLTrace_SetGroupEnableMask(grp, mask)       0
#define VPLTrace_SetGroupDisableMask(grp, mask)      0

#define VPLTrace_SetTraceFile(filename)                   ((char const*)0)

#define VPLTrace_LevelStr(level)                       "OFF "
#define VPLTrace_GroupStr(grp_num)                     "OFF "

#define VPLTrace_GetBaseLevel()                     ((VPLTrace_level)0)
#define VPLTrace_GetGroupLevel(grp_num)               ((VPLTrace_level)0)
#define VPLTrace_GetSgLevel(grp_num, sub_grp)          ((VPLTrace_level)0)

#define VPLTrace_PrintLevelInfo(grp_num, sub_grp)

#define VPLTrace_On(...) (false)

#define VPLTrace_TraceEx(level, ...)

#define VPLTrace_VTraceEx(...)

#define VPLTrace_DumpBuf(...)

#define VPLTrace_SetPrefix(prefix)

#endif      // !VPLTRACE_NO_TRACE:  normal trace support.
// ############################################################################

#ifdef  __cplusplus
}
#endif

#endif // include guard
