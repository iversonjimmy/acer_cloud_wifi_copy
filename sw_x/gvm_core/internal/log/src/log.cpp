//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "log.h"

#include "vplex_file.h"
#include "vplex_plat.h"
#include "vplex_time.h"
#include "vplex_trace.h"
#include "vplex_strings.h"
#include "vpl_fs.h"
#include "vpl_srwlock.h"
#include "vpl_th.h"
#if defined(LINUX) || defined(__CLOUDNODE__)
#include "vpl_shm.h"
#endif

#include "gvm_configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <list>

#define LOG_MAX_LINE                    256
#define LOG_DEFAULT_PRC                 "null"

/// Smallest allowed limit.  If a smaller value is requested, it will be bumped up to this value instead.
#define LOG_MIN_MAX_TOTAL_SIZE          (4 * 1024 * 1024)
#define LOG_MAX_FILE_SIZE               (1 * 1024 * 1024)       // Size for easier opening by editors

#define LOG_SHM_OBJECT_NAME     "/LOG_Shm"
#define LOG_SHM_OBJECT_SIZE     (10 * 1024 * 1024)
#define LOG_SHM_MAGIC_SIZE      16
#define LOG_SHM_RESERVE_SIZE    128

typedef struct {
    int         version;        // Header version
    u8          magic[LOG_SHM_MAGIC_SIZE];  // Magic for identifying a valid SHM after restart
    int         readOff;        // Starting offset for flushing (from shmHeader)
    int         writeOff;       // Offset to continue writing (from shmHeader)
    VPL_BOOL    wrapped;        // Whether the logs have wrapped around. If so, readOff == writeOff
    int         logSize;        // Size of logs, only matters if wrapped == FALSE
    u8          reserve[LOG_SHM_RESERVE_SIZE];  // Reserve for future expansion
} LOGShmHeader;

static const u8 shmMagic[LOG_SHM_MAGIC_SIZE] = {
    0x0a, 0x0a, 0x9a, 0xfd,
    0xd0, 0xa4, 0x76, 0xec,
    0xaa, 0xb3, 0x6a, 0x2d,
    0x9a, 0xba, 0x67, 0x26,
};

static VPLMutex_t shmMutex;

// Bug 6917: WinRT rootDir might contain non-ASCII characters, so multiply by 4.
static char rootDir[(128*4)+6] = "";

/// Name of the process hosting this logger.  This will be included in the log file filenames.
static char logProcessName[64] = LOG_DEFAULT_PRC;

static unsigned char logInit = 0;
static unsigned char logEnabled[LOG_NUM_LEVELS] = { 0, 0, 1, 1, 1, 1, 1 };
static VPL_BOOL writeToStdout = true;
static VPL_BOOL writeToFile = true;
static VPL_BOOL writeToSystemLog = true;
static VPL_BOOL enableInMemoryLogging = false;
static LOGShmHeader *shmHeader = NULL;
#if (defined(LINUX) || defined(__CLOUDNODE__)) && !defined(LINUX_EMB)
static int shmFd = -1;
#endif

// TODO: rename to activeLogFd
static VPLFile_handle_t logProcessFd = VPLFILE_INVALID_HANDLE;
// TODO: rename to activeLogFilename
static char logProcessFilename[1024];

static volatile int logDay = -1;
static unsigned int maxTotalLogSize = LOG_MIN_MAX_TOTAL_SIZE;
static int maxNumFiles = (LOG_MIN_MAX_TOTAL_SIZE / LOG_MAX_FILE_SIZE);

typedef struct {
    unsigned int maxSize;
    VPLFile_handle_t fd;
    char filename[1024];
    bool disabled;
} SpecialLogFile;

static std::list<SpecialLogFile*> specialLogFileList;

#ifdef WIN32
  // Unless we switch to the performance counters (VPLTime_GetTimestamp()), we only get 3 useful digits.
# define FMT_DECIMAL_DIGITS_SEC "%03u"
static inline unsigned getDecimalDigitsSec(const VPLTime_CalendarTime_t* t) { return t->msec; }
#else
# define FMT_DECIMAL_DIGITS_SEC "%06u"
static inline unsigned getDecimalDigitsSec(const VPLTime_CalendarTime_t* t) { return t->usec; }
#endif

#define LOG_LINE_FORMAT  "%02d:%02d:%02d."FMT_DECIMAL_DIGITS_SEC"%s|"FMT_VPLThreadId_t"|%s:%d:%s| "
const char *const logLevelNames[LOG_NUM_LEVELS] = {
    " TRACE ",
    " DBG   ",
    " INFO  ",
    "!!WRN!!",
    "##ERR##",
    "##CRITICAL##",
    " ALWAYS"
};

static VPLSlimRWLock_t logLock;

static LOGLevel mapVPLTraceLevel(VPLTrace_level level)
{
    switch (level) {
    case TRACE_ERROR:
        return LOG_LEVEL_ERROR;
    case TRACE_WARN:
        return LOG_LEVEL_WARN;
    case TRACE_INFO:
        return LOG_LEVEL_INFO;
    case TRACE_FINE:
        return LOG_LEVEL_DEBUG;
    case TRACE_FINER:
        return LOG_LEVEL_DEBUG;
    case TRACE_FINEST:
        return LOG_LEVEL_TRACE;
    case TRACE_ALWAYS:
        return LOG_LEVEL_ALWAYS;
    default:
        return LOG_LEVEL_CRITICAL;
    }
}

/// A #VPLTrace_LogCallback_t.
static void vplexLogCallback(
        VPLTrace_level level,
        const char* sourceFilename,
        int lineNum,
        const char* functionName,
        VPL_BOOL appendNewline,
        int grp_num,
        int sub_grp,
        const char* fmt,
        va_list ap)
{
    LOGVPrint(mapVPLTraceLevel(level), sourceFilename, lineNum, functionName, fmt, ap);
    UNUSED(appendNewline);
    UNUSED(grp_num);
    UNUSED(sub_grp);
}

void
LOGInit(const char* processName, const char* root)
{
    if (logInit) {
        LOG_WARN("LOGInit() was called more than once; ignoring new values");
        return;
    }

    VPLSlimRWLock_Init(&logLock);

    VPLMutex_Init(&shmMutex);

    if (processName == NULL) {
        processName = LOG_DEFAULT_PRC;
    } else {
        processName = VPLString_GetFilenamePart(processName);
    }

    VPLSlimRWLock_LockWrite(&logLock);

    snprintf(logProcessName, sizeof logProcessName,
            "%s", processName);
    if (root != NULL) {
        snprintf(rootDir, sizeof rootDir, "%s/logs", root);
    }
    logDay = -1;
    logInit = 1;

    VPLSlimRWLock_UnlockWrite(&logLock);

    VPLTrace_Init(vplexLogCallback);
}

// Separate out the maxSize setting from LOGInit as it needs to be called before CCDConfig_Init which reads the
// maximum size of log file. 
void
LOGSetMax(unsigned int maxSize)
{
    if (!logInit)       // Ignore this call if log is not initialized yet
        return;

    VPLSlimRWLock_LockWrite(&logLock);

    if (maxSize) {
        if (maxSize > LOG_MIN_MAX_TOTAL_SIZE)
            maxTotalLogSize = maxSize;
        else 
            maxTotalLogSize = LOG_MIN_MAX_TOTAL_SIZE;

        maxNumFiles = maxTotalLogSize / LOG_MAX_FILE_SIZE;
    } else {
        // Old behavior if maxSize configured is 0
        maxTotalLogSize = 0;
        maxNumFiles = 3;
    }

    VPLSlimRWLock_UnlockWrite(&logLock);
}

static
void checkInit()
{
    // TODO: probably better to use VPLThread_Once to ensure atomic initialization from any thread.
    if (!logInit) {
        LOGInit(NULL, NULL);
        LOG_WARN("NOTE: Please call LOGInit() before other any log functions to avoid race conditions!");
    }
}


VPL_BOOL
LOGSetWriteToStdout(VPL_BOOL write)
{
    VPL_BOOL prev = writeToStdout;
    writeToStdout = write;
    return prev;
}

VPL_BOOL
LOGSetWriteToFile(VPL_BOOL write)
{
    VPL_BOOL prev = writeToFile;
    writeToFile = write;
    return prev;
}

VPL_BOOL
LOGSetWriteToSystemLog(VPL_BOOL write)
{
    VPL_BOOL prev = writeToSystemLog;
    writeToSystemLog = write;
    return prev;
}

void
LOGSetLevel(LOGLevel level, unsigned char enable)
{
    checkInit();
    if(level < LOG_NUM_LEVELS) {
        VPLSlimRWLock_LockWrite(&logLock);
        logEnabled[level] = enable;
        VPLSlimRWLock_UnlockWrite(&logLock);
    }
}

void
LOGPrint(LOGLevel level, const char* file, int line,
        const char* function, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    LOGVPrint(level, file, line, function, format, ap);
    va_end(ap);
}

static void log_vprint(LOGLevel level, const char* file, int line,
        const char* function, const VPLTime_CalendarTime_t* currTime, const char* format, va_list ap)
{
    // Fixed-size buffer for most log messages.
    char logline[LOG_MAX_LINE];

    // Points to malloc'ed buffer if the message is too long for "logline".
    char* bigLogline = NULL;

    // Points to either "logline" or "bigLogline".
    char* actualLogline = logline;
    size_t actualLoglineLen = sizeof(logline);

    size_t pos = 0;

    // TODO: assigned, but never read
    int r;

    const char* shortFilename = VPLString_GetFilenamePart(file);

    if (function == NULL) {
        function = "<n/a>";
    }

    {
        // TODO: copy & pasted; refactor.
        pos += snprintf(actualLogline + pos, actualLoglineLen - pos,
                LOG_LINE_FORMAT,
                currTime->hour, currTime->min, currTime->sec,
                getDecimalDigitsSec(currTime),
                logLevelNames[level],
                VPLDetachableThread_Self(),
                shortFilename, line, function);

        {
            size_t bufRemaining = (pos < actualLoglineLen) ? (actualLoglineLen - pos) : 0;
            va_list apcopy; // Need to copy first, since we may need it again later.
            va_copy(apcopy, ap);
            pos += VPL_vsnprintf(actualLogline + pos, bufRemaining, format, apcopy);
            va_end(apcopy);
        }
    }

    // Detect overflow:
    if (pos >= sizeof(logline)) {
        // Overflowed; try again with a malloc'ed buffer.
        // Reserve an extra byte for the newline.
        bigLogline = (char*)malloc(pos + 1);
        if (bigLogline != NULL) {
            actualLogline = bigLogline;
            actualLoglineLen = pos + 1;
            pos = 0;
            {
                // TODO: copy & pasted; refactor.
                pos += snprintf(actualLogline + pos, actualLoglineLen - pos,
                        LOG_LINE_FORMAT,
                        currTime->hour, currTime->min, currTime->sec,
                        getDecimalDigitsSec(currTime),
                        logLevelNames[level],
                        VPLDetachableThread_Self(),
                        shortFilename, line, function);
                ASSERT(pos < actualLoglineLen);
                pos += VPL_vsnprintf(actualLogline + pos, actualLoglineLen - pos, format, ap);
            }
            // TODO: We better end up with the same length.
            ASSERT((pos + 1) == actualLoglineLen);
        } else {
            pos = sizeof(logline) - 4;
            logline[pos++] = '.';
            logline[pos++] = '.';
            logline[pos++] = '.';
        }
    }

    // We are using write() to send the buffer, so it doesn't have to be
    // NULL terminated, but for our sanity it needs to end with a newline.
    ASSERT(pos > 0); // Must be > 0 because we always start with LOG_LINE_FORMAT.
    if (actualLogline[pos - 1] != '\n') {
        actualLogline[pos++] = '\n';
    }

#ifdef GVM_CONSOLE
    // log to kernel first, then stdout/err, then the per-process file;
    if (level == LOG_LEVEL_ERROR || level == LOG_LEVEL_CRITICAL) {
        r = VPLFile_Write(KMSG_FILENO, actualLogline, pos);
    }
#endif

    if (writeToStdout) {
        // Writing the entire log message to stdout with a single write() call is important
        // to prevent interleaving messages from different threads and processes.
        r = VPLFile_Write(STDOUT_FILENO, actualLogline, pos);
    }

    if (writeToFile && (specialLogFileList.size() > 0)) {
        std::list<SpecialLogFile *>::iterator it, tmp; 
        for (it = specialLogFileList.begin(); it != specialLogFileList.end();) { 
            int rv;
            VPLFS_stat_t stat;

            tmp = it++;
            if((*tmp)->disabled) {
                continue;
            }

            rv = VPLFS_FStat((*tmp)->fd, &stat);
            if (rv == 0) {
                if (stat.size < (*tmp)->maxSize) {
                    r = VPLFile_Write((*tmp)->fd, actualLogline, pos);
                } else {
                    (*tmp)->disabled = true;
                }
            } else {
                (*tmp)->disabled = true;
            }
        }
    }

    if (writeToFile && VPLFile_IsValidHandle(logProcessFd)) {
        if (!enableInMemoryLogging) {
            r = VPLFile_Write(logProcessFd, actualLogline, pos);
        } else {
            // There can be multiple readers, so need a separate mutex to protect the
            // the shared memory region. During this short period of time, the shared
            // memory region isn't valid, so reset the magic field
            VPLMutex_Lock(&shmMutex);
            memset(shmHeader->magic, 0, sizeof(shmHeader->magic));

            // Sanity check to avoid buffer overflow
            if (pos <= ((size_t) (LOG_SHM_OBJECT_SIZE - sizeof(LOGShmHeader)))) {
                int nBytesToEnd;

                // logSize is always updated, sometimes just for info if SHM is already wrapped
                shmHeader->logSize += ((int) pos);

                nBytesToEnd = LOG_SHM_OBJECT_SIZE - shmHeader->writeOff;
                if (((size_t) nBytesToEnd) >= pos) {
                    memcpy(((u8 *) (((u8 *) shmHeader) + shmHeader->writeOff)), actualLogline, pos);
                    shmHeader->writeOff += ((int) pos);
                    if (shmHeader->wrapped) {
                        // If already wrapped, then readOff == writeOff
                        shmHeader->readOff = shmHeader->writeOff;
                    }
                } else {
                    // Copy the line, but wrap back to the beginning
                    memcpy(((u8 *) (((u8 *) shmHeader) + shmHeader->writeOff)), actualLogline, nBytesToEnd);
                    memcpy((u8 *) (((u8 *) shmHeader) + sizeof(LOGShmHeader)),
                           &actualLogline[nBytesToEnd], pos - nBytesToEnd);

                    // In the wrapped case, readOff == writeOff
                    shmHeader->wrapped = true;
                    shmHeader->writeOff = sizeof(LOGShmHeader) + ((int) pos) - nBytesToEnd;
                    shmHeader->readOff = shmHeader->writeOff;
                }
            }

            memcpy(shmHeader->magic, shmMagic, sizeof(shmHeader->magic));
            VPLMutex_Unlock(&shmMutex);
        }
    }

    free(bigLogline);
}

static inline
void log_print(LOGLevel level, const char* file, int line,
        const char* function, const VPLTime_CalendarTime_t* currTime, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    log_vprint(level, file, line, function, currTime, format, ap);
    va_end(ap);
}

// TODO: logic is copy & pasted
static inline int
log_mkdirHelper(const char* path)
{
    int rv = VPLDir_Create(path, 0755);
    if (rv == VPL_ERR_EXIST) {
        rv = VPL_OK;
    }
    return rv;
}
// TODO: logic is copy & pasted
static int
log_createPathEx(const char* path, VPL_BOOL last, VPL_BOOL loggingAllowed)
{
    u32 i;
    int rv = 0;
    char* tmp_path = NULL;
#ifdef VPL_PLAT_IS_WINRT
    size_t localpath_length = 0;
#endif

    if (loggingAllowed) {
        LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    }

#ifdef VPL_PLAT_IS_WINRT 
    {
        // metro app can only access LocalState folder in the sandbox.
        // path creation should be started from LocalState folder
        char* local_path;
        rv = _VPLFS__GetLocalAppDataPath(&local_path);
        if (rv != VPL_OK) {
            local_path = NULL;
            goto out;
        }
        localpath_length = strlen(local_path);
        for (i = 1; i < localpath_length; i++) {
            if (local_path[i] == '\\') {
                local_path[i] = '/';
            }
        }
        
        rv = strnicmp(local_path, path, localpath_length);
        if (local_path) {
            free(local_path);
            local_path = NULL;
        }
        if (rv != 0) {
            // TODO: implement path creation for win8 libraries
            rv = VPL_OK;
            goto out;
        }
    }
#endif

    tmp_path = (char*)malloc(strlen(path)+1);
    if(tmp_path == NULL) {
        rv = -1;
        goto out;
    }
    strncpy(tmp_path, path, (strlen(path)+1));

    for (i = 1; tmp_path[i]; i++) {
#ifdef WIN32
        if (tmp_path[i] == '\\') {
            tmp_path[i] = '/';
        }
#endif
        if (tmp_path[i] == '/') {
            tmp_path[i] = '\0';
#ifdef VPL_PLAT_IS_WINRT
            if (i < localpath_length) {
                tmp_path[i] = '/';
                continue;
            }
#endif
            rv = log_mkdirHelper(tmp_path);
            if (rv < 0) {
                if (loggingAllowed) {
                    LOG_ERROR("VPLDir_Create(%s) failed: %d", tmp_path, rv);
                }
                goto out;
            }
            tmp_path[i] = '/';
        }
    }

    if (last) {
        rv = log_mkdirHelper(path);
        if (rv < 0) {
            if (loggingAllowed) {
                LOG_ERROR("VPLDir_Create(%s) failed: %d", path, rv);
            }
            goto out;
        }
    }

out:
    if(tmp_path) free(tmp_path);
    return rv;
}

static void
log_redirect(const VPLTime_CalendarTime_t* currTime, VPL_BOOL heldWriteLock)
{
    VPLFS_file_size_t currentFileSz = 0;

    // already read locked when entering function
    if (!logInit || (rootDir[0] == '\0') || (currTime == NULL)) 
        return;

    // Check the log file size and log date to determine if further action is needed
    { 
        VPLFS_stat_t stat;
        if (VPLFile_IsValidHandle(logProcessFd)) {
            int rv = VPLFS_FStat(logProcessFd, &stat);
            if (rv == 0) {
                currentFileSz = stat.size;
            }
        }
        if ((logDay == currTime->day) && ((currentFileSz < LOG_MAX_FILE_SIZE) || (maxTotalLogSize == 0)))
            return;
    }

    if (!heldWriteLock) {
        VPLSlimRWLock_UnlockRead(&logLock);
        VPLSlimRWLock_LockWrite(&logLock); // obtain write lock
    }

    // Now that we have the write lock, make sure that things haven't changed:
    {
        VPLFS_stat_t stat;
        if (VPLFile_IsValidHandle(logProcessFd)) {
            int rv = VPLFS_FStat(logProcessFd, &stat);
            if (rv == 0) {
                currentFileSz = stat.size;
            } else {
                log_print(LOG_LEVEL_WARN,  __FILE__, __LINE__, __func__, currTime,
                        "Failed to stat (%s).", logProcessFilename);
            }
        }
    }
    if ((logDay != currTime->day) || ((currentFileSz >= LOG_MAX_FILE_SIZE) && (maxTotalLogSize != 0))) {
        snprintf(logProcessFilename, sizeof(logProcessFilename),
                "%s/%s/%s.%d%02d%02d_%02d%02d%02d_%06d.log", rootDir, logProcessName,
                logProcessName, currTime->year, currTime->month, currTime->day, 
                currTime->hour, currTime->min, currTime->sec, currTime->usec);

        if (log_createPathEx(logProcessFilename, VPL_FALSE, VPL_FALSE) < 0) {
            fprintf(stderr, "\nERROR: Failed to create parent dir for \"%s\"\n", logProcessFilename);
        }

        if (VPLFile_IsValidHandle(logProcessFd)) {
            log_print(LOG_LEVEL_ALWAYS,  __FILE__, __LINE__, __func__, currTime,
                    "Time to switch to new log file (%s); closing this log.", logProcessFilename);
            VPLFile_Close(logProcessFd);
        }

        logProcessFd = VPLFile_Open(logProcessFilename,
                                    VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_APPEND | VPLFILE_OPENFLAG_CREATE,
                                    0600);

        // Success; update logDay.
        logDay = currTime->day;
        if (!VPLFile_IsValidHandle(logProcessFd)) {
            fprintf(stderr, "\nERROR: Failed to open log file \"%s\"\n", logProcessFilename);
            log_print(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, currTime, "Failed to open log file \"%s\"", logProcessFilename);
        } else {
            log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, currTime, "Now writing to log file \"%s\"", logProcessFilename);
            // Now that our automated tests collect crash dump files, it would be helpful to correlate the logs with
            // the crash dumps (which are identified by processId).
#ifdef WIN32
            log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, currTime, "My processId: "FMT_DWORD, GetCurrentProcessId());
#elif defined(LINUX)
            log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, currTime, "My pid: "FMTs64, (s64)getpid());
#endif
        }

        // Check if files exceed size limit 
        {
            int numFiles = 0;
            time_t ctimeOldest;
            int rv = 0;
            char oldestLog[1024];
            char logPath[1024];
            unsigned int totalSize = 0;

            snprintf(logPath, sizeof(logPath), "%s/%s", rootDir, logProcessName);
            log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, currTime, "Purging old logs \"%s\"", logPath);

            ctimeOldest = 0;
            oldestLog[0] = '\0';

            rv = -1;
            VPLFS_stat_t stat;
            if (VPLFile_IsValidHandle(logProcessFd)) {
                rv = VPLFS_FStat(logProcessFd, &stat);
            }

            if (rv == 0) {
                time_t ctimeCurrent = stat.ctime;
                bool futureCtimeDetected = false;
                bool updateOldestLog = false;

                // Check total size and oldest file 
                VPLFS_dir_t dir;
                rv = VPLFS_Opendir(logPath, &dir);
                if (rv == VPL_OK) {
                    VPLFS_dirent_t dirent;
                    while (VPLFS_Readdir(&dir, &dirent) == VPL_OK) {
                        char fullPath[1024];
                        if (dirent.type == VPLFS_TYPE_FILE &&
                            VPLString_StartsWith(dirent.filename, logProcessName)) {
                            snprintf(fullPath, sizeof(fullPath), "%s/%s", logPath, dirent.filename);
                            rv = VPLFS_Stat(fullPath, &stat);
                            if (rv == 0) {
                                totalSize += (unsigned int) stat.size;
                                numFiles++;
                                if (stat.ctime != ctimeCurrent) {
                                    updateOldestLog = false;
                                    if (ctimeOldest == 0) {             
                                        // Initialize the oldest log if not done yet
                                        updateOldestLog = true; 
                                    } else if (futureCtimeDetected) {   
                                        // Once a future log detected. Delete future log one at a time, starting with the
                                        // oldest future log, until all are cleared if quota is exceeded.
                                        if ((stat.ctime > ctimeCurrent) &&
                                            (stat.ctime < ctimeOldest)) { 
                                            updateOldestLog = true;
                                            log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, currTime, "Future log detected\"%s\"", fullPath);
                                        }
                                    } else if (stat.ctime > ctimeCurrent) {
                                        // Detect if there is a future log
                                        futureCtimeDetected = true; 
                                        updateOldestLog = true;
                                        log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, currTime, "Future log detected \"%s\"", fullPath);
                                    } else if (stat.ctime < ctimeOldest) {
                                        // Under normal condition, mark the oldest log for deletion if quota is exceeded.
                                        updateOldestLog = true;
                                    }
                                        
                                    if (updateOldestLog) {
                                        ctimeOldest = stat.ctime;
                                        VPLString_SafeStrncpy(oldestLog, ARRAY_SIZE_IN_BYTES(oldestLog), fullPath);
                                    }
                                }
                            }
                        }
                    } 
                    VPLFS_Closedir(&dir);
                }
            }

            // If max num of files is reached or maximum total file size is reached
            // If logs with future timestamp exist (e.g. when system clock is dialed back), the oldest future log file is deleted
            // else the oldest log is deleted. 
            if ((maxTotalLogSize && (totalSize > maxTotalLogSize)) || (numFiles > maxNumFiles)) {
                if (ctimeOldest) {
                    VPLFile_Delete(oldestLog);
                    log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, currTime, "Delete \"%s\"", oldestLog);
                }
            }
        }
    }

    if (!heldWriteLock) {
        VPLSlimRWLock_UnlockWrite(&logLock);
        VPLSlimRWLock_LockRead(&logLock); // restore read lock
    }
}

#if defined(ANDROID)
#include <android/log.h>

static int
loglevelToAndroidPriority(LOGLevel level)
{
    switch(level) {
    case LOG_LEVEL_TRACE:
        return ANDROID_LOG_VERBOSE;
    case LOG_LEVEL_DEBUG:
        return ANDROID_LOG_DEBUG;
    case LOG_LEVEL_INFO:
        return ANDROID_LOG_INFO;
    case LOG_LEVEL_WARN:
        return ANDROID_LOG_WARN;
    case LOG_LEVEL_ERROR:
        return ANDROID_LOG_ERROR;
    case LOG_LEVEL_CRITICAL:
        return ANDROID_LOG_FATAL;
    case LOG_LEVEL_ALWAYS:
        return ANDROID_LOG_INFO; // This is kind of sketchy; there isn't a clear mapping for this one.
    case LOG_NUM_LEVELS:
    default:
        return ANDROID_LOG_FATAL;
    }
}

#endif

void
LOGVPrint(LOGLevel level, const char* file, int line,
        const char* function, const char* format, va_list ap)
{
    if ((level >= LOG_NUM_LEVELS) || !logEnabled[level]) {
        return;
    }

#if defined(ANDROID)
    if (writeToSystemLog) {
        char header[LOG_MAX_LINE];
        const char* shortFilename = VPLString_GetFilenamePart(file);
        snprintf(header, ARRAY_SIZE_IN_BYTES(header),
                "%s:%s:%d", logProcessName, shortFilename, line);
        __android_log_vprint(loglevelToAndroidPriority(level), header, format, ap);
    }
    if (!writeToFile) {
        return;
    }
#endif
    
    checkInit();
    VPLSlimRWLock_LockRead(&logLock);
    {
        VPLTime_CalendarTime_t currTime;
        VPLTime_GetCalendarTimeLocal(&currTime);
        if (!enableInMemoryLogging) {
            // Don't touch the file system if memory logging is enabled. The main purpose
            // of in-memory logging is to allow the disk to spin down
            log_redirect(&currTime, false);
        }
        log_vprint(level, file, line, function, &currTime, format, ap);
    }
    VPLSlimRWLock_UnlockRead(&logLock);
}

void LOGStartSpecialLog(const char *logName, unsigned int max)
{
    VPLTime_CalendarTime_t currTime;
    VPLFile_handle_t fd;
    char specialLogFilename[1024];
    SpecialLogFile *newLog = NULL;

    // Don't proceed if log library is not initialized yet
    if (!logInit || !logName) 
        return;

    VPLSlimRWLock_LockWrite(&logLock); // obtain write lock

    VPLTime_GetCalendarTimeLocal(&currTime);

    snprintf(specialLogFilename, sizeof(specialLogFilename), "%s/%s/special_logs/%s", rootDir, logProcessName, logName);
    // Take the time to clean up any special logs that have been disabled while
    // the write lock is conveniently held.
    std::list<SpecialLogFile *>::iterator specialLogIter = specialLogFileList.begin();
    while(specialLogIter != specialLogFileList.end()) {
        if(!((*specialLogIter)->disabled)) {
            // If we are going to reopen the same file again, close it before reopening.
            if (strcmp((*specialLogIter)->filename, specialLogFilename) != 0) {
                // Current element is enabled and doesn't have the same filename as
                // the new special log; leave it alone.
                ++specialLogIter;
                continue;
            }
        }
        // Current element is either disabled or has the same filename as the new special log;
        // close it now.
        VPLFile_Close((*specialLogIter)->fd);
        free(*specialLogIter);
        specialLogIter = specialLogFileList.erase(specialLogIter);
    }
    
    if (log_createPathEx(specialLogFilename, VPL_FALSE, VPL_FALSE) < 0) {
        fprintf(stderr, "\nERROR: Failed to create dir \"%s\"\n", logName);
    }

    fd = VPLFile_Open(specialLogFilename,
                      VPLFILE_OPENFLAG_READWRITE | VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE,
                      0600);

    if (!VPLFile_IsValidHandle(fd)) {
        fprintf(stderr, "\nERROR: Failed to open special log file \"%s\"\n", specialLogFilename);
        log_print(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, &currTime, "Failed to open special log file \"%s\": %d", specialLogFilename, fd);
        goto exit;
    } else {
        log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, &currTime, "Now start writing to special log file \"%s\"", specialLogFilename);
    }

    newLog = (SpecialLogFile *) malloc(sizeof(SpecialLogFile));
    if (!newLog) {
        log_print(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, &currTime, "Failed to allocate memory");
        VPLFile_Close(fd);
        goto exit;
    }

    newLog->fd = fd;
    newLog->maxSize = max;
    newLog->disabled = false;
    memcpy(newLog->filename, specialLogFilename, sizeof(newLog->filename));
    specialLogFileList.push_back(newLog);

exit:
    VPLSlimRWLock_UnlockWrite(&logLock);
}

void LOGStopSpecialLogs()
{
    if (!logInit)
        return;

    VPLSlimRWLock_LockWrite(&logLock); // obtain write lock

    if (specialLogFileList.size() > 0) {
        std::list<SpecialLogFile *>::iterator it, tmp; 
        for (it = specialLogFileList.begin(); it != specialLogFileList.end();) { 
            tmp = it++;
            if(!(*tmp)->disabled) {
                VPLFile_Close((*tmp)->fd);
            }
            free(*tmp);
            specialLogFileList.erase(tmp);
        }
    }

    VPLSlimRWLock_UnlockWrite(&logLock);
}

static void
logFlushInMemoryLogsHelper()
{
    VPLTime_CalendarTime_t currTime;

    // Must acquire write lock before calling this function

    // Nothing to flush if in-memory logging is not enabled
    if (!enableInMemoryLogging) {
        goto exit;
    }

    // Temporarily turn off in-memory logging so logs can go into the log file directly
    enableInMemoryLogging = false;

    // Flush the logs, and the shared memory region is invalid during this period
    memset(shmHeader->magic, 0, sizeof(shmHeader->magic));
    {
        int nBytesToFlush;

        if (shmHeader->wrapped) {
            nBytesToFlush = LOG_SHM_OBJECT_SIZE - sizeof(LOGShmHeader);

            // Log info message
            VPLTime_GetCalendarTimeLocal(&currTime);
            log_print(LOG_LEVEL_ALWAYS, __FILE__, __LINE__, __func__, &currTime,
                      "%d bytes of logging were dropped here",
                      shmHeader->logSize - (LOG_SHM_OBJECT_SIZE - sizeof(LOGShmHeader)));
        } else {
            nBytesToFlush = shmHeader->logSize;
        }

        // In a loop, flush up to LOG_MAX_FILE_SIZE at a time
        while (nBytesToFlush > 0) {
            int nBytesToEnd, nBytesThisRound;

            // Set the correct log file
            VPLTime_GetCalendarTimeLocal(&currTime);
            log_redirect(&currTime, true);

            // Write a max of LOG_MAX_FILE_SIZE each time
            nBytesThisRound = MIN(nBytesToFlush, LOG_MAX_FILE_SIZE);

            // Beware of wrap-around
            nBytesToEnd = LOG_SHM_OBJECT_SIZE - shmHeader->readOff;
            if (nBytesToEnd == 0) {
                // Special case to avoid calling VPLFile_Write with zero bytes
                nBytesToEnd = LOG_SHM_OBJECT_SIZE - sizeof(LOGShmHeader);
                shmHeader->readOff = sizeof(LOGShmHeader);
            }
            if (nBytesThisRound > nBytesToEnd) {
                nBytesThisRound = nBytesToEnd;
            }

            if (VPLFile_IsValidHandle(logProcessFd)) {
                VPLFile_Write(logProcessFd, (u8 *) (((u8 *) shmHeader) + shmHeader->readOff), nBytesThisRound);
            }

            shmHeader->readOff += nBytesThisRound;
            nBytesToFlush -= nBytesThisRound;
        }

        // Done flushing, reset the shared memory region
        shmHeader->readOff = sizeof(LOGShmHeader);
        shmHeader->writeOff = sizeof(LOGShmHeader);
        shmHeader->wrapped = false;
        shmHeader->logSize = 0;
    }
    memcpy(shmHeader->magic, shmMagic, sizeof(shmHeader->magic));

    enableInMemoryLogging = true;

exit:
    return;
}

#if (defined LINUX || defined __CLOUDNODE__) && !defined LINUX_EMB
int
LOGSetEnableInMemoryLogging(VPL_BOOL enable, int id)
{
#if defined(LINUX) || defined(__CLOUDNODE__)
    // Currently, this feature is only needed for Orbe CCD, which only runs on the Linux
    // and real Orbe platforms. For other platforms, the shared memory VPL functions may
    // not be defined, so it's easier to just return LOG_OK for those platforms

    int rv = LOG_OK, fd = -1;
    char shmName[32];
    void *addr = NULL;

    VPLSlimRWLock_LockWrite(&logLock);

    if (enable == enableInMemoryLogging) {
        // No change, exit
        goto exit;
    }

    // Add the ID to the name in case there are multiple CCD instances
    snprintf(shmName, ARRAY_SIZE_IN_BYTES(shmName), "%s_%d", LOG_SHM_OBJECT_NAME, id);

    if (enable) {
        fd = VPLShm_Open(shmName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            rv = LOG_ERR_SHM;
            goto exit;
        }

        rv = VPL_Fallocate(fd, 0, LOG_SHM_OBJECT_SIZE);
        if (rv < 0) {
            rv = LOG_ERR_SHM;
            goto exit;
        }

        rv = VPL_Mmap(NULL, LOG_SHM_OBJECT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, &addr);
        if (rv < 0) {
            rv = LOG_ERR_SHM;
            goto exit;
        }

        shmHeader = (LOGShmHeader *) addr;
        shmFd = fd;

        // Initialize SHM if it's not already valid
        if (memcmp(shmHeader->magic, shmMagic, ARRAY_SIZE_IN_BYTES(shmHeader->magic)) != 0) {
            shmHeader->version = 1;
            shmHeader->readOff = sizeof(LOGShmHeader);
            shmHeader->writeOff = sizeof(LOGShmHeader);
            shmHeader->wrapped = false;
            shmHeader->logSize = 0;
            memcpy(shmHeader->magic, shmMagic, ARRAY_SIZE_IN_BYTES(shmHeader->magic));
        }
    } else {
        // Flush out existing logs
        logFlushInMemoryLogsHelper();

        memset(shmHeader->magic, 0, ARRAY_SIZE_IN_BYTES(shmHeader->magic));
        VPL_Munmap((void *) shmHeader, LOG_SHM_OBJECT_SIZE);
        shmHeader = NULL;

        VPL_Close(shmFd);
        shmFd = -1;

        VPLShm_Unlink(shmName);
    }

    enableInMemoryLogging = enable;

exit:
    if (rv != LOG_OK && enable) {
        if (addr != NULL) {
            VPL_Munmap(addr, LOG_SHM_OBJECT_SIZE);
        }

        if (fd >= 0) {
            VPL_Close(fd);
        }

        VPLShm_Unlink(shmName);
    }

    VPLSlimRWLock_UnlockWrite(&logLock);

    return rv;
#else
    return LOG_OK;
#endif
}
#endif

void
LOGFlushInMemoryLogs()
{
    VPLSlimRWLock_LockWrite(&logLock);

    if (enableInMemoryLogging) {
        logFlushInMemoryLogsHelper();
    }

    VPLSlimRWLock_UnlockWrite(&logLock);
}

