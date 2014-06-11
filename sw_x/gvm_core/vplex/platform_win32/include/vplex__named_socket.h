#ifndef __VPLEX__NAMED_SOCKET_H__
#define __VPLEX__NAMED_SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sfd; //% server-side file descriptor
} VPLNamedSocket__t;

#define FMT_VPLNamedSocket__t  "%d"
#define VAL_VPLNamedSocket__t(sock)  ((sock).sfd)

typedef struct {
    int cfd; //% client-side file descriptor
} VPLNamedSocketClient__t;

#define FMT_VPLNamedSocketClient__t  "%d"
#define VAL_VPLNamedSocketClient__t(sock)  ((sock).cfd)

#ifdef  __cplusplus
}
#endif

#endif // include guard
