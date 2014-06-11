#ifndef mdd_time_h
#define mdd_time_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mdd_time_platform.h"

int MDDTime_Gettimeofday(struct timeval *tv);

/// the timeout_cb will be called with arg after interval (ms)
int MDDTime_AddTimeoutCallback(unsigned int interval, void(*timeout_cb)(void *arg), void *arg);

#ifdef __cplusplus
}
#endif

#endif //mdd_time_h
