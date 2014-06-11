//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_th.h"
#include "vplu.h"

/// A #PINIT_ONCE_FN.
static BOOL CALLBACK
vplThreadOnceWrapper(
        PINIT_ONCE InitOnce, // Pointer to one-time initialization structure
        PVOID Parameter,     // Optional parameter passed by InitOnceExecuteOnce
        PVOID *lpContext)    // Receives pointer to event object
{
    UNUSED(InitOnce);
    UNUSED(lpContext);
    if (Parameter != NULL) {
        VPLThread_OnceFn_t initRoutine = (VPLThread_OnceFn_t) Parameter;
        initRoutine();
    }
    return TRUE;
}

int VPLThread_Once(VPLThread_once_t* onceControl, VPLThread_OnceFn_t initRoutine)
{
    int rv;
    if (onceControl == NULL || initRoutine == NULL) {
        return VPL_ERR_INVALID;
    }
    if (InitOnceExecuteOnce(&onceControl->o, vplThreadOnceWrapper, initRoutine, NULL)) {
        rv = VPL_OK;
    } else {
        DWORD err = GetLastError();
        rv = VPLError_XlatWinErrno(err);
    }

    return rv;
}
