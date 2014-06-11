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

#ifndef __VPLEX_NUS_PRIV_H__
#define __VPLEX_NUS_PRIV_H__

#include "vplex_nus_service_types.pb.h"
#include "vplex_xml_writer.h"
#include "vplex_soap_priv.h"
#include "vplex_private.h"
#include "vpl_socket.h"

#define VPL_NUS_DIRECTORY_BINDING_NAME  "NetUpdateSOAP"
#define VPL_NUS_REQUEST_ELEMENT_FMT  "%s"
#define VPL_NUS_RESPONSE_ELEMENT_FMT  "%sResponse"

#define XML_WRITER_BUF_LEN  (16*1024)
#define XML_READER_BUF_LEN  (16*1024)
    
typedef struct VPLNus_Proxy {
    
    VPLSoapProxy soapProxy;
    
} VPLNus_Proxy_t;


void VPLNus_priv_insertCommonSoap(
        VPLXmlWriter* writer,
        const char* operationName);

int VPLNus_priv_NUSErrToVPLErr(int nusErrCode);

int VPLNus_priv_ProcessRespParseStateBatched(
        int callRv,
        VPLSoap_RespParseStateBatched* parseState);

static inline
VPLNus_Proxy_t* getNusProxyFromHandle(
        VPLNus_ProxyHandle_t handle)
{
    return (VPLNus_Proxy_t*)(handle.ptr);
}

static inline
VPLSoapProxyHandle getSoapProxyHandle(
        VPLNus_Proxy_t* proxy)
{
    VPLSoapProxyHandle result = { &proxy->soapProxy };
    return result;
}

/// RequestT should be a subclass of #google::protobuf::Message.
/// ResponseT should be a subclass of #google::protobuf::Message.
/// ResponseParserT should be a subclass of #ProtoXmlParseState and come from a *-xml.pb.h file.
template<class RequestT, class ResponseT, class ResponseParserT>
static int
VPLNus_priv_ProtoSoapCall(const char* method,
        VPLNus_ProxyHandle_t proxyHandle,
        VPLTime_t timeout,
        const RequestT& request,
        ResponseT& response,
        void (*writeRequest)(VPLXmlWriter*, const RequestT&))
{
    VPLNus_Proxy_t* proxy = getNusProxyFromHandle(proxyHandle);
    if (proxy == NULL) {
        return VPL_ERR_INVALID;
    }

    // calculate required length?
    VPLXmlWriter* writer;
    writer = (VPLXmlWriter*)malloc(XML_WRITER_BUF_LEN);
    if (writer == NULL) {
        return VPL_ERR_NOMEM;
    }

    // Create the SOAP request.
    char buf[90];
    snprintf(buf, sizeof(buf), VPL_NUS_REQUEST_ELEMENT_FMT, method);
    VPLXmlWriter_Init(writer, XML_WRITER_BUF_LEN);
    VPLNus_priv_insertCommonSoap(writer, buf);
    writeRequest(writer, request);

    snprintf(buf, sizeof(buf), VPL_NUS_RESPONSE_ELEMENT_FMT, method);
    ResponseParserT parser(buf, &response);

    // Call the Web Service.
    snprintf(buf, sizeof(buf), "urn:nus.wsapi.broadon.com/%s", method);
    int rv = VPLSoapProxy_Call(getSoapProxyHandle(proxy),
            VPL_NUS_DIRECTORY_BINDING_NAME,
            buf, writer, &(parser.reader()), timeout);

    free(writer);
    if (rv != 0) {
        return rv;
    } else if (parser.hasError()) {
        VPL_LIB_LOG_WARN(VPL_SG_NUS, "Error in %s response: %s", method,
                parser.errorDetail().c_str());
        return VPL_ERR_INVALID_SERVER_RESPONSE;
    } else if (!response.IsInitialized()) {
        VPL_LIB_LOG_WARN(VPL_SG_NUS, "Incomplete %s response: %s", method,
                response.InitializationErrorString().c_str());
        return VPL_ERR_INVALID_SERVER_RESPONSE;
    } else {
        return VPL_OK;
    }
}

#endif // include guard
