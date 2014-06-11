//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_lazy_init.h"
#include "vplu.h"
#include "vpl_assert.h"

#ifdef _MSC_VER

/// A #PINIT_ONCE_FN.
static BOOL CALLBACK
vplLazyInitCondInit(
        PINIT_ONCE InitOnce, // Pointer to one-time initialization structure
        PVOID Parameter,     // Optional parameter passed by InitOnceExecuteOnce
        PVOID *lpContext)    // Receives pointer to event object
{
    UNUSED(InitOnce);
    UNUSED(lpContext);
    if (Parameter == NULL) {
        VPLIMPL_FAILED_ASSERT("Should not happen");
    } else {
        VPLLazyInitCond_t* lazyInitCond = (VPLLazyInitCond_t*)Parameter;
        int rv = VPLCond_Init(&lazyInitCond->c);
        if (rv == VPL_OK) {
            return TRUE;
        }
        VPL_REPORT_FATAL("%s failed: %d", "VPLCond_Init", rv);
    }
    return FALSE;
}

VPLCond_t* _VPLLazyInitCond__GetCond(VPLLazyInitCond_t* lazyInitCond)
{
    if (lazyInitCond == NULL) {
        return NULL;
    }
    if (InitOnceExecuteOnce(&lazyInitCond->o, vplLazyInitCondInit, lazyInitCond, NULL)) {
        return &lazyInitCond->c;
    } else {
        DWORD err = GetLastError();
        int rv = VPLError_XlatWinErrno(err);
        VPL_REPORT_FATAL("%s failed: %d", "InitOnceExecuteOnce", rv);
        return NULL;
    }
}

#else

static __thread VPLLazyInitCond_t* vplTlLazyInitContext;

static void vplLazyInitCondInit()
{
    int rv = VPLCond_Init(&vplTlLazyInitContext->c);
    if (rv != VPL_OK) {
        VPL_REPORT_FATAL("%s failed: %d", "VPLCond_Init", rv);
    }
}

VPLCond_t* _VPLLazyInitCond__GetCond(VPLLazyInitCond_t* lazyInitCond)
{
    if (lazyInitCond == NULL) {
        return NULL;
    }
    vplTlLazyInitContext = lazyInitCond;
    pthread_once(&lazyInitCond->o, vplLazyInitCondInit);
    return &lazyInitCond->c;
}

#endif
