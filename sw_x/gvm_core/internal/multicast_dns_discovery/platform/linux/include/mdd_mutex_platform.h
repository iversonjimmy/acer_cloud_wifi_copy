#ifndef mdd_mutex_platform_h
#define mdd_mutex_platform_h

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

typedef pthread_mutex_t MDDMutex_platform_t;

#ifdef __cplusplus
}
#endif

#endif //mdd_mutex_platform_h
