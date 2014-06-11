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

#include "vplex_nus.hpp"
#include "vplex_nus_priv.h"
#include "vplex_nus_service_types-xml.pb.h"
#include "vplex_private.h"
#include "vplex_soap.h"
#include "vplex_mem_utils.h"
#include "vplex_serialization.h"
#include "vplex_socket.h"

#include <stdlib.h>

#ifdef VPL_NUS_USE_SSL
static const VPLSoapProtocol PROTOCOL = VPL_SOAP_PROTO_HTTPS;
#define BROADON_AUTH_REQUESTER_NAME  ""
#define BROADON_AUTH_REQUESTER_SECRET  ""
#else
static const VPLSoapProtocol PROTOCOL = VPL_SOAP_PROTO_HTTP;
#define BROADON_AUTH_REQUESTER_NAME  "unitTest"
#define BROADON_AUTH_REQUESTER_SECRET  "8bfdc4e3-472d-42be-93a0-71d322883802"
#endif

#define SERVICE_NAME  "nus"

#define WS_NAMESPACE  "urn:nus.wsapi.broadon.com"

//--------------------------

int
VPLNus_CreateProxy(
        const char* serverHostname,
        u16 serverPort,
        VPLNus_ProxyHandle_t* proxyHandle_out)
{
    int rc;
    VPLNus_Proxy_t* newConnection;
    if (proxyHandle_out == NULL) {
        return VPL_ERR_INVALID;
    }
    proxyHandle_out->ptr = NULL;

    if (serverHostname == NULL) {
        return VPL_ERR_INVALID;
    }

    newConnection = (VPLNus_Proxy_t*)malloc(sizeof(VPLNus_Proxy_t));
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
VPLNus_DestroyProxy(
        VPLNus_ProxyHandle_t proxyHandle)
{
    VPLNus_Proxy_t* proxy = getNusProxyFromHandle(proxyHandle);
    int rc = VPLSoapProxy_Cleanup(&proxy->soapProxy);
    free(proxy);
    return rc;
}

void
VPLNus_priv_insertCommonSoap(VPLXmlWriter* writer, const char* operName)
{
    VPLSoapUtil_InsertCommonSoap2(writer, operName, WS_NAMESPACE);
}

int
VPLNus_priv_NUSErrToVPLErr(int nusErrCode)
{
    switch (nusErrCode) {
    case 0:
        return VPL_OK;
    case 908: // NUS - Invalid login name
        return VPL_ERR_INVALID_LOGIN;
    }
    return VPLSoapUtil_InfraErrToVPLErr(nusErrCode);
}

int VPLNus_GetSystemUpdate(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemUpdateRequestType& in,
                        vplex::nus::GetSystemUpdateResponseType& out)
{
    vplex::nus::GetSystemUpdateResponseType result;
    int rv = VPLNus_priv_ProtoSoapCall<
            vplex::nus::GetSystemUpdateRequestType,
            vplex::nus::GetSystemUpdateResponseType,
            vplex::nus::ParseStateGetSystemUpdateResponseType>(
                    "GetSystemUpdate", proxyHandle, timeout, in, result,
                    vplex::nus::writeGetSystemUpdateRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_NUS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLNus_priv_NUSErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLNus_GetSystemTMD(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemTMDRequestType& in,
                        vplex::nus::GetSystemTMDResponseType& out)
{
    vplex::nus::GetSystemTMDResponseType result;
    int rv = VPLNus_priv_ProtoSoapCall<
            vplex::nus::GetSystemTMDRequestType,
            vplex::nus::GetSystemTMDResponseType,
            vplex::nus::ParseStateGetSystemTMDResponseType>(
                    "GetSystemTMD", proxyHandle, timeout, in, result,
                    vplex::nus::writeGetSystemTMDRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_NUS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLNus_priv_NUSErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLNus_GetSystemPersonalizedETicket(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemPersonalizedETicketRequestType& in,
                        vplex::nus::GetSystemPersonalizedETicketResponseType& out)
{
    vplex::nus::GetSystemPersonalizedETicketResponseType result;
    int rv = VPLNus_priv_ProtoSoapCall<
            vplex::nus::GetSystemPersonalizedETicketRequestType,
            vplex::nus::GetSystemPersonalizedETicketResponseType,
            vplex::nus::ParseStateGetSystemPersonalizedETicketResponseType>(
                    "GetSystemPersonalizedETicket", proxyHandle, timeout, in, result,
                    vplex::nus::writeGetSystemPersonalizedETicketRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_NUS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLNus_priv_NUSErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLNus_GetSystemCommonETicket(
                        VPLNus_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::nus::GetSystemCommonETicketRequestType& in,
                        vplex::nus::GetSystemCommonETicketResponseType& out)
{
    vplex::nus::GetSystemCommonETicketResponseType result;
    int rv = VPLNus_priv_ProtoSoapCall<
            vplex::nus::GetSystemCommonETicketRequestType,
            vplex::nus::GetSystemCommonETicketResponseType,
            vplex::nus::ParseStateGetSystemCommonETicketResponseType>(
                    "GetSystemCommonETicket", proxyHandle, timeout, in, result,
                    vplex::nus::writeGetSystemCommonETicketRequestType);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result._inherited().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_NUS, "error %d: %s", result._inherited().errorcode(),
                result._inherited().errormessage().c_str());
        rv = VPLNus_priv_NUSErrToVPLErr(result._inherited().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}
