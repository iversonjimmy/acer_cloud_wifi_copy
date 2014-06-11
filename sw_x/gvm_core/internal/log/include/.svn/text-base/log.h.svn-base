//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __LOG_H__
#define __LOG_H__

#include <vplu_common.h>
#include <vpl_types.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_ALWAYS,
    LOG_NUM_LEVELS,
} LOGLevel;

#define LOG_OK              0
#define LOG_ERR_PARAMETER   -1
#define LOG_ERR_OPEN        -2
#define LOG_ERR_SHM         -3

#define KMSG_FILENO 3

/// Any and all processes must call this function first.
/// @param root Root directory for app data. "/logs/<process name>/" will be appended to this.
///     If NULL, we will not write logs to the filesystem.
void LOGInit(const char* processName, const char* root);

void LOGSetLevel(LOGLevel level, unsigned char enable);

void LOGStartSpecialLog(const char *logName, unsigned int max);

void LOGStopSpecialLogs();      // Free up resources used by special logs

/// @param maxSize Maximum log size. Use default if 0.
void LOGSetMax(unsigned int maxSize);

/// Pass VPL_FALSE to disable logging to stdout/stderr.
/// This can be useful if the output will be discarded (for GUI apps for example)
/// or to help detect messages that aren't being sent to the log file.
/// @return The previous setting.
VPL_BOOL LOGSetWriteToStdout(VPL_BOOL write);

/// Pass VPL_FALSE to disable logging to a file.
/// @return The previous setting.
VPL_BOOL LOGSetWriteToFile(VPL_BOOL write);

/// Pass VPL_FALSE to disable logging to the system log (currently only applies to Android).
/// @return The previous setting.
VPL_BOOL LOGSetWriteToSystemLog(VPL_BOOL write);

/// Pass VPL_TRUE to enable in-memory logging.
/// In-memory logging is only expected to be used by the Orbe CCD.
/// If writeToFile is also true, then in-memory logging will cache the logs into
/// a shared memory region first.
int LOGSetEnableInMemoryLogging(VPL_BOOL enable, int id);

/// Flush the in-memory logs.
void LOGFlushInMemoryLogs();

void LOGPrint(LOGLevel level, const char* file, int line, const char* function,
        const char* format, ...) ATTRIBUTE_PRINTF(5, 6);

void LOGVPrint(LOGLevel level, const char* file, int line, const char* function,
        const char* format, va_list ap);

#define __LOG(level, fmt, ...) do {                             \
    LOGPrint(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);    \
    } while (0)

#define __LOGV(level, fmt, ap) do {                             \
    LOGVPrint(level, __FILE__, __LINE__, __func__, fmt, ap);    \
    } while (0)

#define LOG_ENABLE_LEVEL(level)     LOGSetLevel(level, 1)
#define LOG_DISABLE_LEVEL(level)    LOGSetLevel(level, 0)

// For things that are expected to be logged repeatedly during normal use,
// even when nothing is interacting with the machine.
#define LOG_TRACE(...)              __LOG(LOG_LEVEL_TRACE, ##__VA_ARGS__)

#define LOG_DEBUG(...)              __LOG(LOG_LEVEL_DEBUG, ##__VA_ARGS__)
#define LOG_INFO(...)               __LOG(LOG_LEVEL_INFO, ##__VA_ARGS__)
#define LOG_WARN(...)               __LOG(LOG_LEVEL_WARN, ##__VA_ARGS__)
#define LOG_ERROR(...)              __LOG(LOG_LEVEL_ERROR, ##__VA_ARGS__)
#define LOG_CRITICAL(...)           __LOG(LOG_LEVEL_CRITICAL, ##__VA_ARGS__)

// For important info that doesn't indicate a problem.
#define LOG_ALWAYS(...)             __LOG(LOG_LEVEL_ALWAYS, ##__VA_ARGS__)

#define LOG_FUNC_ENTRY(level)       __LOG(level, "%s", __func__)

#ifdef __cplusplus
}
#endif

#endif // __LOG_H__
