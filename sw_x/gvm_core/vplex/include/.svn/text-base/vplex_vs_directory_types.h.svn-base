/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef __VPLEX_VS_DIRECTORY_TYPES_H__
#define __VPLEX_VS_DIRECTORY_TYPES_H__

//============================================================================
/// @file
/// Types related to the Virtual Storage Directory Services API.
//============================================================================

#include "vpl_types.h"
#include "vpl_user.h"
#include "vplex_plat.h"
#include "vplex_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @addtogroup VirtualStorage
///@{

/// A handle to a Virtual Storage Directory Services proxy.  Each proxy allows the client to
/// send one synchronous RPC at a time, and both the request and response are
/// encrypted.
/// Note that VPLUser sessions are decoupled from these proxies:
/// it is valid for multiple sessions from multiple local users to all
/// share the same proxy or to have a single user session use multiple proxies.
typedef struct {
    //% Pointer to implementation-specific struct.
    //% See #VPLVsDirectory_Proxy_t.
    void* ptr;
} VPLVsDirectory_ProxyHandle_t;

///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
