#ifndef mdd_mutex_h
#define mdd_mutex_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mdd_mutex_platform.h"

typedef MDDMutex_platform_t MDDMutex_t;

/// Initial a mutex
int MDDMutex_Init(MDDMutex_t *mutex);
/// Lock a mutex
int MDDMutex_Lock(MDDMutex_t *mutex);
/// Unlock a mutex
int MDDMutex_Unlock(MDDMutex_t *mutex);
/// Destroy a mutex
int MDDMutex_Destroy(MDDMutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif //mdd_mutex_h
