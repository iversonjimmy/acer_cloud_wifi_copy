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

#ifndef __VPLEX_IAS_HPP__
#define __VPLEX_IAS_HPP__

//============================================================================
/// @file
/// The client API for the IAS (Identity Authentication Services) Web Service.
/// Please see @ref IdentityAuthentication.
//============================================================================

#include "vplex_plat.h"
#include "vplex_ias_service_types.pb.h"
#include "vplex_user.h"
#include "vpl_socket.h"
#include "vpl_user.h"

// If this is ever disabled (for testing purposes only), you will probably need to
// enable VPLEX_SOAP_SUPPORT_BROADON_AUTH in vplex_soap.cpp.
// NOTE: It is important to keep this defined for production, since security
//       of the system depends on this connection being encrypted!
/// The connections will use HTTPS, so the requests and responses will be encrypted.
#define VPL_IAS_USE_SSL  defined

/** @addtogroup IdentityAuthentication VPLex Ias
  VPLIas APIs (part of the VPLex library) provide the platform with
  access to the Identity Authentication infrastructure.

  These APIs are all blocking operations. For uses where non-blocking operations
  are desired, a worker thread can be spawned to wait for the blocking operation
  to complete.
 */
///@{

/// A handle to an IAS proxy.
typedef struct {
    //% Pointer to implementation-specific struct.
    //% See #VPLIas_Proxy_t.
    void* ptr;
} VPLIas_ProxyHandle_t;

//============================================================================
/// @addtogroup IdentityAuthenticationStartupAndShutdown Startup and Shutdown
///@{


int VPLIas_CreateProxy(
                        const char* serverHostname,
                        u16 serverPort,
                        VPLIas_ProxyHandle_t* proxyHandle_out);
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


int VPLIas_DestroyProxy(
                        VPLIas_ProxyHandle_t proxyHandle);
///< Reclaims resources used by the service proxy.
///< @param[in] proxyHandle The handle for the proxy to destroy.


///@}
//============================================================================
/// @addtogroup IdentityAuthenticationCalls Service Calls
///@{

int VPLIas_CheckVirtualDeviceCredentialsRenewal(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType& in,
        vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType& out);

int VPLIas_GetSessionKey(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::GetSessionKeyRequestType& in,
        vplex::ias::GetSessionKeyResponseType& out);

int VPLIas_Login(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::LoginRequestType& in,
        vplex::ias::LoginResponseType& out);

int VPLIas_Logout(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::LogoutRequestType& in,
        vplex::ias::LogoutResponseType& out);

int VPLIas_RegisterVirtualDevice(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::RegisterVirtualDeviceRequestType& in,
        vplex::ias::RegisterVirtualDeviceResponseType& out);

int VPLIas_RenewVirtualDeviceCredentials(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::RenewVirtualDeviceCredentialsRequestType& in,
        vplex::ias::RenewVirtualDeviceCredentialsResponseType& out);

int VPLIas_GetServerKey(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::GetServerKeyRequestType& in,
        vplex::ias::GetServerKeyResponseType& out);

int VPLIas_RespondToPairingRequest(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::RespondToPairingRequestRequestType& in,
        vplex::ias::RespondToPairingRequestResponseType& out);

int VPLIas_RequestPairing(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::RequestPairingRequestType& in,
        vplex::ias::RequestPairingResponseType& out);

int VPLIas_RequestPairingPin(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::RequestPairingPinRequestType& in,
        vplex::ias::RequestPairingPinResponseType& out);

int VPLIas_GetPairingStatus(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::GetPairingStatusRequestType& in,
        vplex::ias::GetPairingStatusResponseType& out);

///@}

#endif // include guard
