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

#ifndef __VPLEX_NUS_H__
#define __VPLEX_NUS_H__

//============================================================================
/// @file
/// The client API for the NUS (Network Update Services) Web Service.
/// Please see @ref NetworkUpdate.
//============================================================================

#include "vplex_plat.h"
#include "vplex_nus_service_types.pb.h"
#include "vplex_user.h"
#include "vpl_socket.h"
#include "vpl_user.h"

// If this is ever disabled (for testing purposes only), you will probably need to
// enable VPLEX_SOAP_SUPPORT_BROADON_AUTH in vplex_soap.cpp.
// NOTE: It is important to keep this defined for production, since security
//       of the system depends on this connection being encrypted!
/// The connections will use HTTPS, so the requests and responses will be encrypted.
#define VPL_NUS_USE_SSL  defined

/** @addtogroup NetworkUpdate VPLex Nus
  VPLNus APIs (part of the VPLex library) provide the platform with
  access to the Network Update infrastructure.

  These APIs are all blocking operations. For uses where non-blocking operations
  are desired, a worker thread can be spawned to wait for the blocking operation
  to complete.
 */
///@{

/// A handle to an NUS proxy.
typedef struct {
    //% Pointer to implementation-specific struct.
    //% See #VPLNus_Proxy_t.
    void* ptr;
} VPLNus_ProxyHandle_t;

//============================================================================
/// @addtogroup NetworkUpdateStartupAndShutdown Startup and Shutdown
///@{


int VPLNus_CreateProxy(
                       const char* serverHostname,
                       u16 serverPort,
                       VPLNus_ProxyHandle_t* proxyHandle_out);
///< Prepares a new service proxy.  No network activity occurs until
///< the proxy is used.
///<
///< @note To avoid race conditions, you should ensure that #VPLHttp2::Init() is
///<     called before calling this.
///< @note The \a serverHostname buffer is not copied; you must make sure that the memory stays
///<     valid until you are done with the proxy.  You are allowed to later modify the
///<     \a serverHostname buffer as long as you ensure (via a mutex for example) that
///<     nothing else is using the proxy at the same time.
///< @param[in] serverHostname Internet hostname for the server (buffer is not copied).
///< @param[in] serverPort Internet port for the server.
///< @param[out] proxyHandle_out The handle for the new server proxy.


int VPLNus_DestroyProxy(
                        VPLNus_ProxyHandle_t proxyHandle);
///< Reclaims resources used by the service proxy.
///< @param[in] proxyHandle The handle for the proxy to destroy.


///@}
//============================================================================
/// @addtogroup NetworkUpdateCalls Service Calls
///@{

int VPLNus_GetSystemUpdate(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemUpdateRequestType& in,
                        vplex::nus::GetSystemUpdateResponseType& out);

int VPLNus_GetSystemTMD(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemTMDRequestType& in,
                        vplex::nus::GetSystemTMDResponseType& out);

int VPLNus_GetSystemPersonalizedETicket(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemPersonalizedETicketRequestType& in,
                        vplex::nus::GetSystemPersonalizedETicketResponseType& out);

int VPLNus_GetSystemCommonETicket(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemCommonETicketRequestType& in,
                        vplex::nus::GetSystemCommonETicketResponseType& out);
///@}

#endif // include guard
