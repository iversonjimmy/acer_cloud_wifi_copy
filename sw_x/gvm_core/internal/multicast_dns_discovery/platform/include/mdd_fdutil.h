#ifndef mdd_fdutil_h
#define mdd_fdutil_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mdd_fdutil_platform.h"
#include "mdd_socket.h"
#include "mdd_time.h"

typedef MDDFDSet_platform_t MDDFDSet_t;

/// clear fd set
int MDDFDUtil_FDZero(MDDFDSet_t *set);
/// Set socket to fd set
int MDDFDUtil_FDSet(MDDSocket_t socket, MDDFDSet_t *set);
/// check if the socket in fd set
/// return MDD_TRUE is exist, otherwise MDD_FALSE
int MDDFDUtil_FDIsset(MDDSocket_t socket, MDDFDSet_t *set);

/// use select to wait the change of sockets
/// if error occur, MDD_ERROR will be return
/// otherwise return the number of changes
int MDDFDUtil_FDSelect(MDDSocket_t max_socket, MDDFDSet_t *readfds, MDDFDSet_t *writefds, MDDFDSet_t *exceptfds, struct timeval *timeout);

#ifdef __cplusplus
}
#endif

#endif //mdd_fdutil_h
