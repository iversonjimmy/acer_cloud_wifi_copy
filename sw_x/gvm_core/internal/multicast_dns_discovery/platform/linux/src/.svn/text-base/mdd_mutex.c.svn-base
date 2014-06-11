#include "mdd_mutex.h"
#include "mdd_utils.h"

int MDDMutex_Init(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    if (pthread_mutex_init(mutex, NULL) == 0) {
        return MDD_OK;
    }
    return MDD_ERROR;
}

int MDDMutex_Lock(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    if (pthread_mutex_lock(mutex) == 0) {
        return MDD_OK;
    }
    return MDD_ERROR;
}

int MDDMutex_Unlock(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    if (pthread_mutex_unlock(mutex) == 0) {
        return MDD_OK;
    }
    return MDD_ERROR;
}

int MDDMutex_Destroy(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    if (pthread_mutex_destroy(mutex) == 0) {
        return MDD_OK;
    }
    return MDD_ERROR;
}
