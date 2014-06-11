#include "mdd_fdutil.h"
#include "mdd_utils.h"

#pragma comment(lib, "Ws2_32.lib")

int MDDFDUtil_FDZero(MDDFDSet_t *set)
{
    FD_ZERO(set);
    return MDD_OK;
}

int MDDFDUtil_FDSet(MDDSocket_t socket, MDDFDSet_t *set)
{
    FD_SET(socket, set);
    return MDD_OK;
}

int MDDFDUtil_FDIsset(MDDSocket_t socket, MDDFDSet_t *set)
{
    if (FD_ISSET(socket, set)) {
        return MDD_TRUE;
    }
    return MDD_FALSE;
}

int MDDFDUtil_FDSelect(MDDSocket_t max_socket, MDDFDSet_t *readfds, MDDFDSet_t *writefds, MDDFDSet_t *exceptfds, struct timeval *timeout)
{
    int ret = 0;
    if ((ret = select(max_socket + 1, readfds, writefds, exceptfds, timeout)) < 0) {
        return MDD_ERROR;
    }
    return ret;
}