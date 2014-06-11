#include "vplex_soap.h"
#include "vplex_http2.hpp"
#include "vplex_soap_priv.h"
#include "vplex_private.h"
#include "vplex_strings.h"
#include "vplex_xml_reader.h"
#include "vpl_time.h"

#include <stdlib.h>

// Note: Enabling this is sort-of a security risk, since it leaks information
//   about how our data-centers work.
//#define VPLEX_SOAP_SUPPORT_BROADON_AUTH  defined

int VPLSoapProxy_Init(
        VPLSoapProxy* proxy,
        VPLSoapProtocol protocol,
        const char* serverHostname,
        u16 serverPort,
        const char* serviceName,
        const char* authRequesterName,
        const char* authRequesterSecret)
{
    if (proxy == NULL) {
        return VPL_ERR_INVALID;
    }
    proxy->protocol = protocol;
    proxy->serverHostname = serverHostname;
    proxy->serverPort = serverPort;
    proxy->serviceName = serviceName;
    proxy->authRequesterName = authRequesterName;
    proxy->authRequesterSecret = authRequesterSecret;
    return VPL_OK;
}

int VPLSoapProxy_Create(
        VPLSoapProtocol protocol,
        const char* serverHostname,
        u16 serverPort,
        const char* serviceName,
        const char* authRequesterName,
        const char* authRequesterSecret,
        VPLSoapProxyHandle* handle_out)
{
    VPLSoapProxy* proxy;
    int rc;
    
    if (handle_out == NULL) {
        return VPL_ERR_INVALID;
    }
    handle_out->ptr = NULL;
    
    proxy = (VPLSoapProxy*)malloc(sizeof(VPLSoapProxy));
    if(proxy == NULL) {
        return VPL_ERR_NOMEM;
    }
    rc = VPLSoapProxy_Init(proxy, protocol, serverHostname, serverPort,
            serviceName, authRequesterName, authRequesterSecret);
    if(rc == VPL_OK) {
        handle_out->ptr = proxy;
    } else {
        free(proxy);
    }
    return rc;
}

static inline VPLSoapProxy* handleToProxy(VPLSoapProxyHandle handle)
{
    return (VPLSoapProxy*)(handle.ptr);
}

int VPLSoapProxy_Cleanup(VPLSoapProxy* proxy)
{
    if (proxy == NULL) { return VPL_ERR_INVALID; }
    return VPL_OK;
}

int VPLSoapProxy_Destroy(VPLSoapProxyHandle handle)
{
    VPLSoapProxy* proxy = handleToProxy(handle);
    int rc = VPLSoapProxy_Cleanup(proxy);
    if (rc == VPL_OK) {
        free(proxy);
    }
    return rc;
}

#ifdef VPLEX_SOAP_SUPPORT_BROADON_AUTH
//------------------------------
// BroadOn-specific server auth
//------------------------------

static u64 getMsSinceEpoch(void)
{
    struct timeval tempWithMicrosec;
    (IGNORE_RESULT) gettimeofday(&tempWithMicrosec, NULL);
    u64 result =
            (((u64)tempWithMicrosec.tv_sec) * UINT64_C(1000)) +
            (((u64)tempWithMicrosec.tv_usec) / UINT64_C(1000));
    return result;
}

/// Length in bytes.
#define BROADON_INFRA_HASH_LENGTH  (1 + (MD5_DIGEST_LENGTH * 2))

/// \a hexString_out must be at least #BROADON_INFRA_HASH_LENGTH bytes.
static void computeHash(
        u64 timestamp,
        const char* clientName,
        const char* clientSecret,
        char* hexString_out)
{
    /* This should match the implementation in Java:
        private static String computeHash(String token)
        throws UnsupportedEncodingException, NoSuchAlgorithmException {
            MessageDigest md5 = MessageDigest.getInstance("MD5");
            byte[] bytes = md5.digest(token.getBytes("US-ASCII"));
            String hash = HexString.toHexString(bytes).toLowerCase();
            return hash;
        }
     */
    
    // Construct the special token.
    char token[512];
    int requiredLen =
            snprintf(token, ARRAY_SIZE_IN_BYTES(token), FMTu64"%s%s",
                    timestamp,
                    clientName,
                    clientSecret);
    // Make sure it was successful.
    ASSERT_GREATER_OR_EQ(requiredLen, 0, FMTint);
    // Make sure it wasn't truncated.
    ASSERT_LESS(requiredLen, (int)(ARRAY_SIZE_IN_BYTES(token)), FMTint);
    
    // Hash it.
    unsigned char msgDigest[MD5_DIGEST_LENGTH];
    (IGNORE_RESULT) MD5((unsigned char*)(token), requiredLen, msgDigest);

    // Convert hash bytes to hexadecimal string.
    unsigned i;
    for(i = 0; i < ARRAY_SIZE_IN_BYTES(msgDigest); i++) {
        sprintf(&hexString_out[i*2], "%02x", msgDigest[i]);
    }
    // Done.  Since we used sprintf, hexString_out will already be null-terminated.
}
#endif

//------------------------------

int VPLSoapUtil_InfraErrToVPLErr(int infraErrCode)
{
    if (infraErrCode <= 0) {
        return infraErrCode;
    }
    // Remap server error codes without losing information.
    // Client conventions are that error codes must be negative; see "Error Codes by Subsystem" on the Wiki.
    if (infraErrCode > VPL_MAX_EXPECTED_INFRA_ERRCODE) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error code (%d) not in expected range!", infraErrCode);
        return VPL_ERR_INVALID_SERVER_RESPONSE;
    } else {
        return VPL_INFRA_ERR_TO_CLIENT_ERR(infraErrCode);
    }
}

static
const char* protocolToUrlString(VPLSoapProtocol protocol)
{
    switch(protocol) {
    case VPL_SOAP_PROTO_HTTP:
        return "http";
    case VPL_SOAP_PROTO_HTTPS:
        return "https";
    default:
        // TODO: log?
        return "error";
    }
}

static int
VPLSoapProxy_Call_common(
        VPLSoapProxyHandle proxyHandle,
        const char* bindingName,
        const char* soapAction,
        VPLXmlWriter* writer,
        void* reader,
        VPLHttp2_RecvCb writeCallback,
        VPLTime_t timeout)
{
    int rv;
    VPLSoapProxy* proxy = handleToProxy(proxyHandle);
    char* bindingUrl = NULL;
#ifdef VPLEX_SOAP_SUPPORT_BROADON_AUTH
    char* requesterTimestamp = NULL;
    char requesterHash[BROADON_INFRA_HASH_LENGTH];
#endif
    char* soapActionValue = NULL;
    VPLTime_t startTimestamp;

    bindingUrl = VPLString_MallocAndSprintf("%s://%s:"FMTu16"/%s/services/%s",
            protocolToUrlString(proxy->protocol),
            proxy->serverHostname,
            proxy->serverPort,
            proxy->serviceName,
            bindingName);
    // Need to surround the action with quotes.
    soapActionValue = VPLString_MallocAndSprintf("\"%s\"", soapAction);
    
#ifdef VPLEX_SOAP_SUPPORT_BROADON_AUTH
    {
        u64 timestamp = getMsSinceEpoch();
        computeHash(timestamp,
                proxy->authRequesterName,
                proxy->authRequesterSecret,
                requesterHash);
        requesterTimestamp = VPLString_MallocAndSprintf(FMTu64, timestamp);
    }
#endif

    VPLHttp2 tempHandle;
    tempHandle.SetUri(bindingUrl);
    tempHandle.SetTimeout(timeout);
    tempHandle.SetDebug(1);
    {
#ifdef VPLEX_SOAP_SUPPORT_BROADON_AUTH
        tempHandle.AddRequestHeader("com.broadon.RequesterName", proxy->authRequesterName);
        tempHandle.AddRequestHeader("com.broadon.RequesterTimestamp", requesterTimestamp);
        tempHandle.AddRequestHeader("com.broadon.RequesterHash", requesterHash);
#endif
        tempHandle.AddRequestHeader("SOAPAction", soapActionValue);
        tempHandle.AddRequestHeader("Content-Type", "text/xml; charset=utf-8");
        
        const char* payload = VPLXmlWriter_GetString(writer, NULL);
        if (payload == NULL) {
            VPL_LIB_LOG_WARN(VPL_SG_HTTP, "Unexpected overflow!");
            rv = VPL_ERR_BUF_TOO_SMALL;
            goto out;
        }
        startTimestamp = VPLTime_GetTimeStamp();
        {
            int httpCode;
            rv = tempHandle.Post(payload, writeCallback, reader, NULL, NULL);

            httpCode = tempHandle.GetStatusCode();
            if (rv < 0) {
                VPL_LIB_LOG_WARN(VPL_SG_HTTP, "VPLHttp_Request returned %d", rv);
            } else if (httpCode != 200) {
                VPL_LIB_LOG_WARN(VPL_SG_HTTP, "HTTP result: %d", httpCode);
                rv = VPL_ERR_UNABLE_TO_SERVICE;
            } else {
                // success!
            }
        }
    }
    {
        VPLTime_t elapsed = VPLTime_GetTimeStamp() - startTimestamp;
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "SOAP timing: %s %s: "FMT_VPLTime_t"ms",
                proxy->serviceName, soapAction, VPLTime_ToMillisec(elapsed));
    }
out:
    free(soapActionValue);
#ifdef VPLEX_SOAP_SUPPORT_BROADON_AUTH
    free(requesterTimestamp);
#endif
    free(bindingUrl);
    return rv;
}

static s32 VPLXmlReader_CurlWriteCallbackWrapper(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    return (s32)VPLXmlReader_CurlWriteCallback(buf, size, 1, ctx);
}


int VPLSoapProxy_Call(
        VPLSoapProxyHandle proxyHandle,
        const char* bindingName,
        const char* soapAction,
        VPLXmlWriter* writer,
        _VPLXmlReader* reader,
        VPLTime_t timeout)
{
    return VPLSoapProxy_Call_common(proxyHandle, bindingName, soapAction,
            writer, reader, VPLXmlReader_CurlWriteCallbackWrapper, timeout);
}
