//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_th.h"

#include <pthread.h>

#include "vplu.h"

int
VPLThread_Once(VPLThread_once_t* onceControl,
        VPLThread_OnceFn_t initRoutine)
{
    int rv;

    if ((onceControl == NULL) || (initRoutine == NULL)) {
        rv = VPL_ERR_INVALID;
    } else {
        rv = pthread_once((pthread_once_t*)&(onceControl->val), initRoutine);
        if (rv != 0) {
            rv = VPLError_XlatErrno(rv);
        }
    }
    return rv;
}
