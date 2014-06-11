//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_error.h"
#include "vplu_debug.h"

#include <errno.h>

int VPLError_XlatErrno(int errCode) {
    int rv;
    switch(errCode) {
    case 0:
        rv = VPL_OK;
        break;
    case EACCES:
        rv = VPL_ERR_ACCESS;
        break;
    case EADDRINUSE:
        rv = VPL_ERR_ADDRINUSE;
        break;
    case EADDRNOTAVAIL:
        rv = VPL_ERR_ADDRNOTAVAIL;
        break;
    // From "man accept": POSIX.1-2001 allows either error to be returned for this case,
    // and does not require these constants to have the same value, so a portable application
    // should check for both possibilities.
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
    case EAGAIN:
        rv = VPL_ERR_AGAIN;
        break;
    case EBADF:
        rv = VPL_ERR_BADF;
        break;
    case EBUSY:
        rv = VPL_ERR_BUSY;
        break;
    case EINPROGRESS:
        rv = VPL_ERR_BUSY;
        break;
    case ECONNABORTED:
        rv = VPL_ERR_CONNABORT;
        break;
    case ECONNRESET:
        rv = VPL_ERR_CONNRESET;
        break;
    case ECONNREFUSED:
        rv =  VPL_ERR_CONNREFUSED;
        break;
    case EDESTADDRREQ:
        rv = VPL_ERR_DESTADDRREQ;
        break;
    case EEXIST:
        rv = VPL_ERR_EXIST;
        break;
    case EFAULT:
        rv = VPL_ERR_FAULT;
        break;
    case EHOSTUNREACH:
        rv = VPL_ERR_UNREACH;
        break;
    case EINTR:
        rv = VPL_ERR_INTR;
        break;
    case EINVAL:
        rv = VPL_ERR_INVALID;
        break;
    case EIO:
        rv = VPL_ERR_IO;
        break;
    case EISCONN:
        rv = VPL_ERR_ISCONN;
        break;
    case ELOOP:
        rv = VPL_ERR_LOOP;
        break;
    case EMFILE:
        rv = VPL_ERR_MAX;
        break;
    case EMSGSIZE:
        rv = VPL_ERR_MSGSIZE;
        break;
    case ENAMETOOLONG:
        rv = VPL_ERR_NAMETOOLONG;
        break;
    case ENETDOWN:
        rv = VPL_ERR_NETDOWN;
        break;
    case ENETRESET:
        rv = VPL_ERR_NETRESET;
        break;
    case ENETUNREACH:
        rv = VPL_ERR_UNREACH;
        break;
    case ENFILE:
        rv = VPL_ERR_MAX;
        break;
    case ENOBUFS:
        rv = VPL_ERR_NOBUFS;
        break;
    case ENODEV:
        rv = VPL_ERR_NODEV;
        break;
    case ENOENT:
        rv = VPL_ERR_NOENT;
        break;
    case ENOMEM:
        rv = VPL_ERR_NOMEM;
        break;
    case ENOSPC:
        rv = VPL_ERR_NOSPC;
        break;
    case ENOSYS:
        rv = VPL_ERR_NOSYS;
        break;
    case ENOTCONN:
        rv = VPL_ERR_NOTCONN;
        break;
    case ENOTDIR:
        rv = VPL_ERR_NOTDIR;
        break;
    case ENOTEMPTY:
        rv = VPL_ERR_NOTEMPTY;
        break;
    case ENOTSOCK:
        rv = VPL_ERR_NOTSOCK;
        break;
    case EOPNOTSUPP:
        rv = VPL_ERR_OPNOTSUPPORTED;
        break;
    case EPERM:
        rv = VPL_ERR_PERM;
        break;
    case EPIPE:
        rv = VPL_ERR_PIPE;
        break;
    case EROFS:
        rv = VPL_ERR_ROFS;
        break;
    case ETIMEDOUT:
        rv = VPL_ERR_TIMEOUT;
        break;
    case ETXTBSY:
        rv = VPL_ERR_TXTBSY;
        break;
    case EDEADLK:
        rv = VPL_ERR_DEADLK;
        break;
    case EXDEV:
        rv = VPL_ERR_CROSS_DEVICE_LINK;
        break;
    default:
        // Generic error for the unmapped.
        VPL_REPORT_WARN("Converting unknown error code %d to VPL_ERR_FAIL.", errCode);
        rv = VPL_ERR_FAIL;
        break;
    }

    return rv;
}
