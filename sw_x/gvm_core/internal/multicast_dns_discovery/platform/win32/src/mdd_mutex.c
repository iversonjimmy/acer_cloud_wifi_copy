#include "mdd_mutex.h"
#include "mdd_utils.h"

int MDDMutex_Init(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    InitializeCriticalSection(mutex);
    return MDD_OK;
}

int MDDMutex_Lock(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    EnterCriticalSection(mutex);
    return MDD_OK;
}

int MDDMutex_Unlock(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    LeaveCriticalSection(mutex);
    return MDD_OK;
}

int MDDMutex_Destroy(MDDMutex_t *mutex)
{
    if (mutex == NULL) {
        return MDD_ERROR;
    }

    DeleteCriticalSection(mutex);
    return MDD_OK;
}
