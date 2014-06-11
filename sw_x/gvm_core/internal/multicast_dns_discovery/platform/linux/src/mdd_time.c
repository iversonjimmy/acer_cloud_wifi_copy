#include "mdd_time.h"
#include "mdd_utils.h"

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include "vpl_time.h"
#include "log.h"

struct TimeoutHandler {
    int interval;
    void (*timeout_cb)(void *arg);
    void *arg;
};

int MDDTime_Gettimeofday(struct timeval *tv)
{
    // This function seems to be only used as relative time.
    VPLTime_t currTime = VPLTime_GetTimeStamp();
    tv->tv_sec = (long)VPLTIME_TO_SEC(currTime);
    tv->tv_usec = VPLTIME_TO_MICROSEC(currTime) % VPLTIME_MICROSEC_PER_SEC;
    return 0;

    // Previous implementation
    //return gettimeofday(tv, 0);
}

static void *timeout_thread(void *arg)
{
    struct TimeoutHandler *handler = (struct TimeoutHandler *)arg;
    struct timeval interval;

    if (arg == NULL) {
        LOG_ERROR("Error on timeout thread, handler is null\n");
        return NULL;
    }

    interval.tv_sec = (handler->interval / 1000);
    interval.tv_usec = (handler->interval - (interval.tv_sec * 1000)) * 1000;

    // use select to instead of sleep
    select(1, 0, 0, 0, &interval);

    // call timeout callback
    handler->timeout_cb(handler->arg);

    // release the handler
    free(handler);
    return NULL;
}

int MDDTime_AddTimeoutCallback(unsigned int interval, void(*timeout_cb)(void *arg), void *arg)
{
    pthread_t thread;
    struct TimeoutHandler *handler;

    if (timeout_cb == NULL) {
        return MDD_ERROR;
    }

    handler = malloc(sizeof(struct TimeoutHandler));
    handler->interval = interval;
    handler->timeout_cb = timeout_cb;
    handler->arg = arg;

    if (pthread_create(&thread, NULL, timeout_thread, handler) != 0) {
        free(handler);
        return MDD_ERROR;
    }
    pthread_detach(thread);

    return MDD_OK;
}
