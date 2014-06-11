#include "mdd_time.h"
#include "mdd_utils.h"

#include <time.h>
#include <windows.h>
#include <stdio.h>

#include "vpl_time.h"
#include "log.h"

typedef struct TimeoutHandler {
    int interval;
    void (*timeout_cb)(void *arg);
    void *arg;
} TIMEOUTHANDLER, *PTIMEOUTHANDLER;

int MDDTime_Gettimeofday(struct timeval *tv)
{
    // This function seems to be only used as relative time.
    VPLTime_t currTime = VPLTime_GetTimeStamp();
    tv->tv_sec = (long)VPLTIME_TO_SEC(currTime);
    tv->tv_usec = VPLTIME_TO_MICROSEC(currTime) % VPLTIME_MICROSEC_PER_SEC;

    // Previous implementation
    //    time_t clock;
    //    struct tm tm;
    //    SYSTEMTIME wtm;
    //    GetLocalTime(&wtm);
    //    tm.tm_year     = wtm.wYear - 1900;
    //    tm.tm_mon     = wtm.wMonth - 1;
    //    tm.tm_mday     = wtm.wDay;
    //    tm.tm_hour     = wtm.wHour;
    //    tm.tm_min     = wtm.wMinute;
    //    tm.tm_sec     = wtm.wSecond;
    //    tm. tm_isdst    = -1;
    //    clock = mktime(&tm);
    //    tv->tv_sec = clock;
    //    tv->tv_usec = wtm.wMilliseconds * 1000;
    return 0;
}

DWORD WINAPI TimeoutThreadFunc(LPVOID lpParm)
{
    PTIMEOUTHANDLER handler;

    if (lpParm == NULL) {
        LOG_ERROR("Error on timeout thread, handler is null\n");
        return -1;
    }

    handler = (PTIMEOUTHANDLER)lpParm;
    Sleep(handler->interval);

    // call timeout callback
    handler->timeout_cb(handler->arg);

    // release the handler
    free(handler);
    return 0;
}

int MDDTime_AddTimeoutCallback(unsigned int interval, void(*timeout_cb)(void *arg), void *arg)
{
    PTIMEOUTHANDLER handler;
    HANDLE hThread;

    handler = (PTIMEOUTHANDLER) malloc(sizeof(TIMEOUTHANDLER));
    SecureZeroMemory(handler, sizeof(TIMEOUTHANDLER));
    handler->interval = interval;
    handler->timeout_cb = timeout_cb;
    handler->arg = arg;

    hThread = CreateThread(NULL, 0, TimeoutThreadFunc, handler, 0, NULL);
    if (hThread == NULL)
    {
        LOG_ERROR("MDDTime, CreateThread failed with error: %d\n", GetLastError());
        return MDD_ERROR;
    }

    return MDD_OK;
}
