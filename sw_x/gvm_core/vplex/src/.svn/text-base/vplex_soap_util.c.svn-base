#include "vplex_soap.h"
#include "vplex_soap_priv.h"
#include "vplex_private.h"
#include "vplex_strings.h"
#include "vplex_xml_reader.h"

#include <stdlib.h>

//--------

void VPLSoapUtil_InsertCommonSoap(VPLXmlWriter* writer)
{
    VPLXmlWriter_InsertXml(writer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    VPLXmlWriter_OpenTagV(writer, "S:Envelope", 2,
            "xmlns:S", "http://schemas.xmlsoap.org/soap/envelope/",
            "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    VPLXmlWriter_OpenTagV(writer, "S:Body", 0);
}

void VPLSoapUtil_InsertCommonSoap2(VPLXmlWriter* writer, const char* operName, const char* wsNamespace)
{
    VPLSoapUtil_InsertCommonSoap(writer);
    VPLXmlWriter_OpenTagV(writer, operName, 1, "xmlns", wsNamespace);
}

//--------

VPLTime_CalendarTime_t VPLSoapUtil_parseDateTime(
        int* clientErrorCode,
        const char* tagName,
        const char* dateTime)
{
    VPLTime_CalendarTime_t result;
    memset(&result, 0, sizeof(result));
    // TODO!
    return result;
}

// No easy equivalent of vsscanf for MSVC
#if 0
void VPLSoapUtil_scanfAndCheck(
        int* clientErrorCode,
        const char* tagName,
        const char* str,
        const char* fmt,
        ...)
{
    va_list args;
    va_start(args, fmt);
    {
        int numRead = vsscanf(str, fmt, args);
        if (numRead != 1) {
            *clientErrorCode = VPL_ERR_INVALID_SERVER_RESPONSE;
            VPL_LIB_LOG_WARN(VPL_SG_HTTP, "%s invalid", tagName);
        }
    }
    va_end(args);
}
#else
#define VPLSoapUtil_scanfAndCheck(clientErrorCode_, tagName_, str_, fmt_, arg_) \
    BEGIN_MULTI_STATEMENT_MACRO \
    int numRead_ = sscanf(str_, fmt_, arg_); \
    if (numRead_ != 1) { \
        *(clientErrorCode_) = VPL_ERR_INVALID_SERVER_RESPONSE; \
        VPL_LIB_LOG_WARN(VPL_SG_HTTP, "%s invalid", tagName_); \
    } \
    END_MULTI_STATEMENT_MACRO
#endif

//--------

void VPLSoapUtil_openTagCbBasic(
        const char* tag,
        const char* attr_name[],
        const char* attr_value[],
        void* param)
{
    VPLSoap_RespParseStateBasic* parseState = (VPLSoap_RespParseStateBasic*)param;
    if (strcmp(tag, "ErrorCode") == 0)
        parseState->parserState = 1;
}

void VPLSoapUtil_closeTagCbBasic(const char* tag, void* param)
{
    VPLSoap_RespParseStateBasic* parseState = (VPLSoap_RespParseStateBasic*)param;
    if (strcmp(tag, "ErrorCode") == 0)
        parseState->parserState = 0;
}

void VPLSoapUtil_dataCbBasic(const char* data, void* param)
{
    VPLSoap_RespParseStateBasic* parseState = (VPLSoap_RespParseStateBasic*)param;
    if (parseState->parserState) {
        VPLSoapUtil_scanfAndCheck(&parseState->clientErrorCode, "<ErrorCode>",
                data, "%d", &parseState->serverErrorCode);
    }
}

//--------

enum {
    TAG_ERROR_CODE = 0x0001,
};

static const VPLXmlUtil_TagMapping_t tagsBatched[] = {
        { "ErrorCode", TAG_ERROR_CODE },
};

void VPLSoapUtil_openTagCbBatched(
        const char* tag,
        const char* attr_name[],
        const char* attr_value[],
        void* param)
{
    VPLSoap_RespParseStateBatched* parseState = (VPLSoap_RespParseStateBatched*)param;
    VPLXmlUtil_processXmlTag(tag, &parseState->parserState,
            tagsBatched, ARRAY_ELEMENT_COUNT(tagsBatched),
            VPL_TRUE);
}

void VPLSoapUtil_closeTagCbBatched(const char* tag, void* param)
{
    VPLSoap_RespParseStateBatched* parseState = (VPLSoap_RespParseStateBatched*)param;
    VPLXmlUtil_processXmlTag(tag, &parseState->parserState,
            tagsBatched, ARRAY_ELEMENT_COUNT(tagsBatched),
            VPL_FALSE);
}

void VPLSoapUtil_dataCbBatched(const char* data, void* param)
{
    VPLSoap_RespParseStateBatched* parseState = (VPLSoap_RespParseStateBatched*)param;
    if (parseState->parserState != 0) {
        if (parseState->currNumServerErrorCodes >= parseState->expectedNumServerErrorCodes) {
            // We got too many!
            parseState->clientErrorCode = VPL_ERR_INVALID_SERVER_RESPONSE;
        }
        else {
            VPLSoapUtil_scanfAndCheck(&parseState->clientErrorCode, "<ErrorCode>",
                    data, "%d",
                    &(parseState->serverErrorCodes[parseState->currNumServerErrorCodes]));
        }
        parseState->currNumServerErrorCodes++;
    }
}
