#ifndef mdd_mutex_platform_h
#define mdd_mutex_platform_h

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

typedef CRITICAL_SECTION MDDMutex_platform_t;

#ifdef __cplusplus
}
#endif

#endif //mdd_mutex_platform_h
