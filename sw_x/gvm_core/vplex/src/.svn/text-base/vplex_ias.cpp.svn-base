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

#include "vplex_ias.hpp"
#include "vplex_ias_priv.h"
#include "vplex_ias_service_types-xml.pb.h"
#include "vplex_private.h"
#include "vplex_soap.h"
#include "vplex_mem_utils.h"
#include "vplex_serialization.h"
#include "vplex_socket.h"

#include <stdlib.h>

#ifdef VPL_IAS_USE_SSL
static const VPLSoapProtocol PROTOCOL = VPL_SOAP_PROTO_HTTPS;
#define BROADON_AUTH_REQUESTER_NAME  ""
#define BROADON_AUTH_REQUESTER_SECRET  ""
#else
static const VPLSoapProtocol PROTOCOL = VPL_SOAP_PROTO_HTTP;
#define BROADON_AUTH_REQUESTER_NAME  "unitTest"
#define BROADON_AUTH_REQUESTER_SECRET  "8bfdc4e3-472d-42be-93a0-71d322883802"
#endif

#define SERVICE_NAME  "ias"

#define WS_NAMESPACE  "urn:ias.wsapi.broadon.com"

//--------------------------

int
VPLIas_CreateProxy(
        const char* serverHostname,
        u16 serverPort,
        VPLIas_ProxyHandle_t* proxyHandle_out)
{
    int rc;
    VPLIas_Proxy_t* newConnection;
    if (proxyHandle_out == NULL) {
        return VPL_ERR_INVALID;
    }
    proxyHandle_out->ptr = NULL;

    if (serverHostname == NULL) {
        return VPL_ERR_INVALID;
    }

    newConnection = (VPLIas_Proxy_t*)malloc(sizeof(VPLIas_Proxy_t));
    if(newConnection == NULL) {
        return VPL_ERR_NOMEM;
    }

    rc = VPLSoapProxy_Init(
            &(newConnection->soapProxy),
            PROTOCOL,
            serverHostname,
            serverPort,
            SERVICE_NAME,
            BROADON_AUTH_REQUESTER_NAME,
            BROADON_AUTH_REQUESTER_SECRET);
    if(rc != VPL_OK) {
        goto fail;
    }
    proxyHandle_out->ptr = newConnection;
    return VPL_OK;
fail:
    free(newConnection);
    return rc;
}

int
VPLIas_DestroyProxy(
        VPLIas_ProxyHandle_t proxyHandle)
{
    VPLIas_Proxy_t* proxy = getIasProxyFromHandle(proxyHandle);
    int rc = VPLSoapProxy_Cleanup(&proxy->soapProxy);
    free(proxy);
    return rc;
}

void
VPLIas_priv_insertCommonSoap(VPLXmlWriter* writer, const char* operName)
{
    VPLSoapUtil_InsertCommonSoap2(writer, operName, WS_NAMESPACE);
}

int
VPLIas_priv_IASErrToVPLErr(int iasErrCode)
{
    switch (iasErrCode) {
    case 0:
        return VPL_OK;
    case 908: // IAS - Invalid login name
        return VPL_ERR_INVALID_LOGIN;
    }
    return VPLSoapUtil_InfraErrToVPLErr(iasErrCode);
}

int VPLIas_CheckVirtualDeviceCredentialsRenewal(
                        VPLIas_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType& in,
                        vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType& out)
{
    vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
            vplex::ias::CheckVirtualDeviceCredentialsRenewalRequestType,
            vplex::ias::CheckVirtualDeviceCredentialsRenewalResponseType,
            vplex::ias::ParseStateCheckVirtualDeviceCredentialsRenewalResponseType>(
                    "CheckVirtualDeviceCredentialsRenewal", proxyHandle, timeout, in, result,
                    vplex::ias::writeCheckVirtualDeviceCredentialsRenewalRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_GetSessionKey(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::GetSessionKeyRequestType& in,
        vplex::ias::GetSessionKeyResponseType& out)
{
    vplex::ias::GetSessionKeyResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
            vplex::ias::GetSessionKeyRequestType,
            vplex::ias::GetSessionKeyResponseType,
            vplex::ias::ParseStateGetSessionKeyResponseType>(
                    "GetSessionKey", proxyHandle, timeout, in, result,
                    vplex::ias::writeGetSessionKeyRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_Login(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::LoginRequestType& in,
        vplex::ias::LoginResponseType& out)
{
    vplex::ias::LoginResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
            vplex::ias::LoginRequestType,
            vplex::ias::LoginResponseType,
            vplex::ias::ParseStateLoginResponseType>(
                    "Login", proxyHandle, timeout, in, result,
                    vplex::ias::writeLoginRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_Logout(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::LogoutRequestType& in,
        vplex::ias::LogoutResponseType& out)
{
    vplex::ias::LogoutResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
            vplex::ias::LogoutRequestType,
            vplex::ias::LogoutResponseType,
            vplex::ias::ParseStateLogoutResponseType>(
                    "Logout", proxyHandle, timeout, in, result,
                    vplex::ias::writeLogoutRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_RegisterVirtualDevice(
                        VPLIas_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::ias::RegisterVirtualDeviceRequestType& in,
                        vplex::ias::RegisterVirtualDeviceResponseType& out)
{
    vplex::ias::RegisterVirtualDeviceResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
            vplex::ias::RegisterVirtualDeviceRequestType,
            vplex::ias::RegisterVirtualDeviceResponseType,
            vplex::ias::ParseStateRegisterVirtualDeviceResponseType>(
                    "RegisterVirtualDevice", proxyHandle, timeout, in, result,
                    vplex::ias::writeRegisterVirtualDeviceRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_RenewVirtualDeviceCredentials(
                        VPLIas_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::ias::RenewVirtualDeviceCredentialsRequestType& in,
                        vplex::ias::RenewVirtualDeviceCredentialsResponseType& out)
{
    vplex::ias::RenewVirtualDeviceCredentialsResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
            vplex::ias::RenewVirtualDeviceCredentialsRequestType,
            vplex::ias::RenewVirtualDeviceCredentialsResponseType,
            vplex::ias::ParseStateRenewVirtualDeviceCredentialsResponseType>(
                    "RenewVirtualDeviceCredentials", proxyHandle, timeout, in, result,
                    vplex::ias::writeRenewVirtualDeviceCredentialsRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_GetServerKey(
        VPLIas_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const vplex::ias::GetServerKeyRequestType& in,
        vplex::ias::GetServerKeyResponseType& out)
{
    vplex::ias::GetServerKeyResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
            vplex::ias::GetServerKeyRequestType,
            vplex::ias::GetServerKeyResponseType,
            vplex::ias::ParseStateGetServerKeyResponseType>(
                    "GetServerKey", proxyHandle, timeout, in, result,
                    vplex::ias::writeGetServerKeyRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_RespondToPairingRequest(
    VPLIas_ProxyHandle_t proxyHandle,
    VPLTime_t timeout,
    const vplex::ias::RespondToPairingRequestRequestType& in,
    vplex::ias::RespondToPairingRequestResponseType& out)
{
    vplex::ias::RespondToPairingRequestResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
        vplex::ias::RespondToPairingRequestRequestType,
        vplex::ias::RespondToPairingRequestResponseType,
        vplex::ias::ParseStateRespondToPairingRequestResponseType>(
        "RespondToPairingRequest", proxyHandle, timeout, in, result,
        vplex::ias::writeRespondToPairingRequestRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
            result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_RequestPairing(
    VPLIas_ProxyHandle_t proxyHandle,
    VPLTime_t timeout,
    const vplex::ias::RequestPairingRequestType& in,
    vplex::ias::RequestPairingResponseType& out)
{
    vplex::ias::RequestPairingResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
        vplex::ias::RequestPairingRequestType,
        vplex::ias::RequestPairingResponseType,
        vplex::ias::ParseStateRequestPairingResponseType>(
        "RequestPairing", proxyHandle, timeout, in, result,
        vplex::ias::writeRequestPairingRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
            result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;

}

int VPLIas_RequestPairingPin(
    VPLIas_ProxyHandle_t proxyHandle,
    VPLTime_t timeout,
    const vplex::ias::RequestPairingPinRequestType& in,
    vplex::ias::RequestPairingPinResponseType& out)
{
    vplex::ias::RequestPairingPinResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
        vplex::ias::RequestPairingPinRequestType,
        vplex::ias::RequestPairingPinResponseType,
        vplex::ias::ParseStateRequestPairingPinResponseType>(
        "RequestPairingPin", proxyHandle, timeout, in, result,
        vplex::ias::writeRequestPairingPinRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
            result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLIas_GetPairingStatus(
    VPLIas_ProxyHandle_t proxyHandle,
    VPLTime_t timeout,
    const vplex::ias::GetPairingStatusRequestType& in,
    vplex::ias::GetPairingStatusResponseType& out)
{
    vplex::ias::GetPairingStatusResponseType result;
    int rv = VPLIas_priv_ProtoSoapCall<
        vplex::ias::GetPairingStatusRequestType,
        vplex::ias::GetPairingStatusResponseType,
        vplex::ias::ParseStateGetPairingStatusResponseType>(
        "GetPairingStatus", proxyHandle, timeout, in, result,
        vplex::ias::writeGetPairingStatusRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS, "error %d: %s", result._inherited().errorcode(),
            result._inherited().errormessage().c_str());
        rv = VPLIas_priv_IASErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}
