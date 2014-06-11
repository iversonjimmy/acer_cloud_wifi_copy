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
vplLazyInitMutexInit(
        PINIT_ONCE InitOnce, // Pointer to one-time initialization structure
        PVOID Parameter,     // Optional parameter passed by InitOnceExecuteOnce
        PVOID *lpContext)    // Receives pointer to event object
{
    UNUSED(InitOnce);
    UNUSED(lpContext);
    if (Parameter == NULL) {
        VPLIMPL_FAILED_ASSERT("Should not happen");
    } else {
        VPLLazyInitMutex_t* lazyInitMutex = (VPLLazyInitMutex_t*)Parameter;
        int rv = VPLMutex_Init(&lazyInitMutex->m);
        if (rv == VPL_OK) {
            return TRUE;
        }
        VPL_REPORT_FATAL("%s failed: %d", "VPLMutex_Init", rv);
    }
    return FALSE;
}

VPLMutex_t* _VPLLazyInitMutex__GetMutex(VPLLazyInitMutex_t* lazyInitMutex)
{
    if (lazyInitMutex == NULL) {
        return NULL;
    }
    if (InitOnceExecuteOnce(&lazyInitMutex->o, vplLazyInitMutexInit, lazyInitMutex, NULL)) {
        return &lazyInitMutex->m;
    } else {
        DWORD err = GetLastError();
        int rv = VPLError_XlatWinErrno(err);
        VPL_REPORT_FATAL("%s failed: %d", "InitOnceExecuteOnce", rv);
        return NULL;
    }
}

#else

static __thread VPLLazyInitMutex_t* vplTlLazyInitContext;

static void vplLazyInitMutexInit()
{
    int rv = VPLMutex_Init(&vplTlLazyInitContext->m);
    if (rv != VPL_OK) {
        VPL_REPORT_FATAL("%s failed: %d", "VPLMutex_Init", rv);
    }
}

VPLMutex_t* _VPLLazyInitMutex__GetMutex(VPLLazyInitMutex_t* lazyInitMutex)
{
    if (lazyInitMutex == NULL) {
        return NULL;
    }
    vplTlLazyInitContext = lazyInitMutex;
    pthread_once(&lazyInitMutex->o, vplLazyInitMutexInit);
    return &lazyInitMutex->m;
}

#endif
