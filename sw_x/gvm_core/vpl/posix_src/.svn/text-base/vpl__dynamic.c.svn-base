//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "vpl_dynamic.h"

#include <dlfcn.h>
#include <pthread.h>

#include "vplu.h"
#include "vplu_debug.h"

//--------------------

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

static void 
lockMutex(void)
{
    int rv = pthread_mutex_lock(&s_mutex);
    if (rv != 0) {
        VPL_REPORT_WARN("pthread_mutex_lock returned %d", rv);
    }
}

static void 
unlockMutex(void)
{
    int rv = pthread_mutex_unlock(&s_mutex);
    if (rv != 0) {
        VPL_REPORT_WARN("pthread_mutex_unlock returned %d", rv);
    }
}

//--------------------

#define REPORT_DLERROR(msg) \
    BEGIN_MULTI_STATEMENT_MACRO \
    const char* vplDynamicDlError = (msg); \
    if (vplDynamicDlError == NULL) { \
        VPL_REPORT_WARN("Failed, but dlerror was NULL"); \
    } else { \
        VPL_REPORT_WARN("%s", vplDynamicDlError); \
    } \
    END_MULTI_STATEMENT_MACRO

//--------------------

static const int VPLDL_HANDLE_GOOD = 0x900D900D;
static const int VPLDL_HANDLE_FAILED = 0xFA17FA17;
static const int VPLDL_HANDLE_CLOSED = 0x15C105ED;

int 
VPLDL_Open(const char* filename, VPL_BOOL global, VPLDL_LibHandle* handleOut)
{
    int rv;
    void* newHandle;
    
    if (filename == NULL) {
        // This doesn't need to be an error; dlopen interprets it as a request
        // to get a handle for the main program.
        // However, it is easier to add functionality than remove it.
        return VPL_ERR_INVALID;
    }
    if (handleOut == NULL) {
        return VPL_ERR_INVALID;
    }
    
    lockMutex();
    {
        int mode = RTLD_NOW;
        if (global) {
            mode |= RTLD_GLOBAL;
        }
        
        newHandle = dlopen(filename, mode);
        if (newHandle == NULL) {
            rv = VPL_ERR_FAIL;
            REPORT_DLERROR(dlerror());
            handleOut->status = VPLDL_HANDLE_FAILED;
        }
        else {
            rv = VPL_OK;
            handleOut->status = VPLDL_HANDLE_GOOD;
        }
        handleOut->handle = newHandle;
    }
    unlockMutex();
    return rv;
}

static int 
VPLDL_SymHelper(void* handle, const char* symbol, void** addrOut)
{
    int rv;
    if ((symbol == NULL) || (addrOut == NULL)) {
        return VPL_ERR_INVALID;
    }
    
    lockMutex();
    {
        // Clear dlerror
        dlerror();
        *addrOut = dlsym(handle, symbol);
        // It is not necessarily an error when dlsym returns NULL.  We need to
        // check dlerror() to know for sure.
        const char* errorStr = dlerror();
        if (errorStr == NULL) {
            rv = VPL_OK;
        }
        else {
            REPORT_DLERROR(errorStr);
            rv = VPL_ERR_FAIL;
        }
    }
    unlockMutex();
    return rv;
}

int 
VPLDL_Sym(VPLDL_LibHandle handle, const char* symbol, void** addrOut)
{
    if (handle.status != VPLDL_HANDLE_GOOD) {
        return VPL_ERR_BADF;
    }
    return VPLDL_SymHelper(handle.handle, symbol, addrOut);
}

// Removed from API until a developer asks for it:
#if 0
int 
VPLDL_SymSearch(VPLDL_SearchType_t searchType, const char* symbol, void** addrOut)
{
    void* handle;
    switch (searchType) {
    case VPLDL_SEARCH_LIB_DEFAULT:
        handle = RTLD_DEFAULT;
        break;
    case VPLDL_SEARCH_LIB_NEXT:
        handle = RTLD_NEXT;
        break;
    default:
        return VPL_ERR_INVALID;
    }
    return VPLDL_SymHelper(handle, symbol, addrOut);
}
#endif

int 
VPLDL_Close(VPLDL_LibHandle handle)
{
    int rv;
    lockMutex();
    {
        if (handle.status != VPLDL_HANDLE_GOOD) {
            rv = VPL_ERR_BADF;
            goto unlock;
        }
        int rc = dlclose(handle.handle);
        if (rc == 0) {
            rv = VPL_OK;
//% Problem: handle is passed by value
//%            handle.status = VPLDL_HANDLE_CLOSED;
        }
        else {
            REPORT_DLERROR(dlerror());
            rv = VPL_ERR_FAIL;
//% Problem: handle is passed by value
//%          handle.status = VPLDL_HANDLE_FAILED;
        }
    }
unlock:
    unlockMutex();
    return rv;
}
