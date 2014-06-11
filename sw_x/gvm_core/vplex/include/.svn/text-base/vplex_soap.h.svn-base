//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLEX_SOAP_H__
#define __VPLEX_SOAP_H__

//============================================================================
/// @file Generic mechanism for making SOAP calls to a web service.
/// APIs for specific web services can be built on top of this.
//============================================================================

#include "vplex_plat.h"
#include "vplex_xml_reader.h"
#include "vplex_xml_writer.h"
#include "vplex_time.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// A context for making SOAP calls.
///
typedef struct {
    //% Pointer to implementation-specific struct.
    //% See #VPLSoapProxy.
    void* ptr;
} VPLSoapProxyHandle;

typedef enum {
    VPL_SOAP_PROTO_HTTP,
    VPL_SOAP_PROTO_HTTPS,
} VPLSoapProtocol;

///
/// Allocates a new SOAP Proxy (via malloc).
/// To avoid race conditions, you should ensure that #VPLHttp2::Init() is called
/// before calling this.
/// @note None of the string buffers are copied.  You must make sure that the memory stays valid
///     until you are done with the #VPLSoapProxyHandle.  You are allowed to later modify the
///     \a serverHostname buffer as long as you ensure (via a mutex for example) that
///     nothing else is using the proxy at the same time.
//%     CCD relies on this behavior, so don't change it without updating CCD first.
///
int VPLSoapProxy_Create(
                        VPLSoapProtocol protocol,
                        const char* serverHostname,
                        u16 serverPort,
                        const char* serviceName,
                        const char* authRequesterName,
                        const char* authRequesterSecret,
                        VPLSoapProxyHandle* handle_out);

///
/// Deallocates a SOAP Proxy.
///
int VPLSoapProxy_Destroy(
                        VPLSoapProxyHandle handle);

///
/// Performs a SOAP RPC.
/// (This function internally uses the #VPLHttp2 implementation, but the caller
/// doesn't need to be aware of this detail.)
///
int VPLSoapProxy_Call(
                        VPLSoapProxyHandle handle,
                        const char* bindingName,
                        const char* soapAction,
                        VPLXmlWriter* writer,
                        _VPLXmlReader* reader,
                        VPLTime_t timeout);

//----------------

///
/// Map infrastructure error code (positive) into client error code (negative).
///
int VPLSoapUtil_InfraErrToVPLErr(int infraErrCode);

//----------------

///
/// Insert the constant XML headers, leaving the writer inside the "Body" tag.
/// \code <?xml ...><Envelope ...><Body> \endcode
///
void VPLSoapUtil_InsertCommonSoap(VPLXmlWriter* writer);

///
/// As above, but also add the operation tag.
/// \code <?xml ...><Envelope ...><Body><OPERATION_NAME xmlns="WS_NAMESPACE">\endcode
///
void VPLSoapUtil_InsertCommonSoap2(VPLXmlWriter* writer, const char* operName, const char* wsNamespace);

//----------------

/// 
typedef u32  VPLXmlUtil_ParseFlags_t;

///
typedef struct {
    const char* tagName;
    VPLXmlUtil_ParseFlags_t flagBit;
} VPLXmlUtil_TagMapping_t;

///
static inline void VPLXmlUtil_processXmlTag(
        const char* tag,
        VPLXmlUtil_ParseFlags_t* parserState,
        const VPLXmlUtil_TagMapping_t mappings[],
        u32 numMappings,
        VPL_BOOL openTag)
{
    unsigned i;
    for (i = 0; i < numMappings; i++) {
        if (strcmp(tag, mappings[i].tagName) == 0) {
            if (openTag) {
                *parserState |= mappings[i].flagBit;
            }
            else {
                *parserState &= ~mappings[i].flagBit;
            }
            break;
        }
    }
}

//----------------

static inline
void VPL_setIntArray(int array[], u32 count, int value)
{
    unsigned i;
    for (i = 0; i < count; i++) {
        array[i] = value;
    }
}

void VPLSoapUtil_scanfAndCheck(
        int* clientErrorCode,
        const char* tagName,
        const char* str,
        const char* fmt,
        ...) ATTRIBUTE_SCANF(4, 5);

VPLTime_CalendarTime_t VPLSoapUtil_parseDateTime(
        int* clientErrorCode,
        const char* tagName,
        const char* dateTime);

//----------------

/// Reusable parser functionality for SOAP responses where we only
/// care about the "ErrorCode" value.
typedef struct {
    /// If non-zero, then we are within the ErrorCode tag.
    VPLXmlUtil_ParseFlags_t parserState;
    int serverErrorCode;
    int clientErrorCode;
} VPLSoap_RespParseStateBasic;

static inline
void VPLSoapUtil_initRespParseStateBasic(
        VPLSoap_RespParseStateBasic* parseState)
{
    parseState->parserState = 0;
    parseState->serverErrorCode = VPL_ERR_INVALID_SERVER_RESPONSE;
    parseState->clientErrorCode = VPL_OK;
}

/// A #VPLXmlReader_TagOpenCallback.
void VPLSoapUtil_openTagCbBasic(const char* tag, const char* attr_name[],
        const char* attr_value[], void* param);

/// A #VPLXmlReader_TagCloseCallback.
void VPLSoapUtil_closeTagCbBasic(const char* tag, void* param);

/// A #VPLXmlReader_DataCallback.
void VPLSoapUtil_dataCbBasic(const char* data, void* param);

//--------

/// Reusable parser functionality for SOAP responses where we only
/// care about the "ErrorCode" and an array of "ExtraErrorCode" values.
typedef struct {
    VPLXmlUtil_ParseFlags_t parserState;
    int* serverErrorCodes;
    u32 expectedNumServerErrorCodes;
    u32 currNumServerErrorCodes;
    int clientErrorCode;
} VPLSoap_RespParseStateBatched;

static inline
void VPLSoapUtil_initRespParseStateBatched(
        VPLSoap_RespParseStateBatched* parseState,
        int dstErrorCodes[],
        u32 numErrorCodes)
{
    parseState->parserState = 0;
    parseState->serverErrorCodes = dstErrorCodes;
    parseState->expectedNumServerErrorCodes = numErrorCodes;
    VPL_setIntArray(dstErrorCodes, numErrorCodes, VPL_ERR_INVALID_SERVER_RESPONSE);
    parseState->currNumServerErrorCodes = 0;
    parseState->clientErrorCode = VPL_OK;
}

/// A #VPLXmlReader_TagOpenCallback.
void VPLSoapUtil_openTagCbBatched(const char* tag, const char* attr_name[],
        const char* attr_value[], void* param);

/// A #VPLXmlReader_TagCloseCallback.
void VPLSoapUtil_closeTagCbBatched(const char* tag, void* param);

/// A #VPLXmlReader_DataCallback.
void VPLSoapUtil_dataCbBatched(const char* data, void* param);

//--------

#ifdef __cplusplus
}
#endif

#endif // include guard
