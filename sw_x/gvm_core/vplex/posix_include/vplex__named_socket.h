#ifndef __VPLEX__NAMED_SOCKET_H__
#define __VPLEX__NAMED_SOCKET_H__

#include "vplex_ipc_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    IPCSocket s;
} VPLNamedSocket__t;

#define FMT_VPLNamedSocket__t  "%d"
#define VAL_VPLNamedSocket__t(sock)  ((sock).s.fd)

typedef struct {
    IPCSocket c;
} VPLNamedSocketClient__t;

#define FMT_VPLNamedSocketClient__t  "%d"
#define VAL_VPLNamedSocketClient__t(sock)  ((sock).c.fd)

#ifdef  __cplusplus
}
#endif

#endif // include guard
