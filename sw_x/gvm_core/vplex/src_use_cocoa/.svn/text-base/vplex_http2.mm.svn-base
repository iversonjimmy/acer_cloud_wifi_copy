//
//  vplex_http2.mm
//  vplex
//
//  Created by build on 12/10/30.
//
//

#include "vplex__http2.hpp"
#include "vplex_private.h"

#import "NSURLConnectionHelper.h"

VPLHttp2__Impl::VPLHttp2__Impl(VPLHttp2 *interface)
{
    this->vplHttp2Interface = interface;
    this->connectionHelper = (__bridge_retained void*)[[NSURLConnectionHelper alloc] init];
    this->isDebugging = NO;
}

VPLHttp2__Impl::~VPLHttp2__Impl()
{
    // iOS will do auto-release
    if (this->connectionHelper) {
        CFRelease(this->connectionHelper);
        this->connectionHelper = NULL;
    }
}

int VPLHttp2__Impl::Init(void)
{
    return VPL_OK;
}

void VPLHttp2__Impl::Shutdown(void)
{

}

int VPLHttp2__Impl::SetDebug(bool debug)
{
    int rv = 0;
    VPL_LIB_LOG_INFO(VPL_SG_HTTP,"SetDebug %d", debug);
    isDebugging = debug;
    [(__bridge NSURLConnectionHelper*)connectionHelper setIsDebugging:isDebugging];
    return rv;
}

int VPLHttp2__Impl::SetTimeout(VPLTime_t timeout)
{
    if (!connectionHelper) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,"connectionHelper is not initialized");
        return VPL_ERR_NOT_INIT;
    }
    
    [(__bridge NSURLConnectionHelper*)connectionHelper setTimeoutInterval:VPLTime_ToSec(timeout)];
    return VPL_OK;
}

int VPLHttp2__Impl::SetUri(const std::string &uri)
{
    if (!connectionHelper) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,"connectionHelper is not initialized");
        return VPL_ERR_NOT_INIT;
    }
    
    if (&uri != NULL) {
        [(__bridge NSURLConnectionHelper*)connectionHelper setConnectionURIFromCString:uri.c_str()];
    }
    return VPL_OK;
}

int VPLHttp2__Impl::AddRequestHeader(const std::string &field, const std::string &value)
{
    if (!connectionHelper) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,"connectionHelper is not initialized");
        return VPL_ERR_NOT_INIT;
    }
    
    if (&field != NULL && &value != NULL) {
        [(__bridge NSURLConnectionHelper*)connectionHelper addRequestHeaderWithField:field.c_str() Value:value.c_str()];
    }
    return VPL_OK;
}

// Methods to send a GET request.
int VPLHttp2__Impl::Get(std::string &respBody)
{
    int rv = startToConnect("GET");
    if (rv != VPL_OK) {
        return rv;
    }

    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }

    return rv;
}

int VPLHttp2__Impl::Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    [(__bridge NSURLConnectionHelper*)connectionHelper setReceiveFilePathFromFilePath:respBodyFilePath.c_str()];
    
    [(__bridge NSURLConnectionHelper*)connectionHelper registerReceiveProgressCallback:recvProgCb Interface:vplHttp2Interface Context:recvProgCtx];
    int rv = startToConnect("GET");
    if (rv != VPL_OK) {
        return rv;
    }

    return VPL_OK;
}

int VPLHttp2__Impl::Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    [(__bridge NSURLConnectionHelper*)connectionHelper registerReceiveProgressCallback:recvProgCb Interface:vplHttp2Interface Context:recvProgCtx];
    [(__bridge NSURLConnectionHelper*)connectionHelper registerReceiveCallback:recvCb Interface:vplHttp2Interface Context:recvCtx];

    int rv = startToConnect("GET");
    if (rv != VPL_OK) {
        return rv;
    }

    return VPL_OK;
}

// Methods to send a PUT request.
int VPLHttp2__Impl::Put(const std::string &reqBody, std::string &respBody)
{
    [(__bridge NSURLConnectionHelper*)connectionHelper setConnectionBodyFromData:reqBody.c_str() DataLength:reqBody.size()];
    int rv = startToConnect("PUT");
    if (rv != VPL_OK) {
        return rv;
    }

    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }
    return VPL_OK;
}

int VPLHttp2__Impl::Put(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    [(__bridge NSURLConnectionHelper*)connectionHelper setContentFilePathFromFilePath:reqBodyFilePath.c_str()];

    [(__bridge NSURLConnectionHelper*)connectionHelper registerSendProgressCallback:sendProgCb Interface:vplHttp2Interface Context:sendProgCtx];
    int rv = startToConnect("PUT");
    if (rv != VPL_OK) {
        return rv;
    }

    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }
    return VPL_OK;
}

int VPLHttp2__Impl::Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    NSCallbackInputStream *callbackInputStream = [[NSCallbackInputStream alloc] initWithHttp2SendCb:sendCb SendCtx:sendCtx VPLHttp2Interface:vplHttp2Interface];
    [(__bridge NSURLConnectionHelper*)connectionHelper setCallbackInputStream:callbackInputStream SendSize:sendSize];

    [(__bridge NSURLConnectionHelper*)connectionHelper registerSendProgressCallback:sendProgCb Interface:vplHttp2Interface Context:sendProgCtx];
    
    int rv = startToConnect("PUT");
    if (rv != VPL_OK) {
        return rv;
    }

    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }
    return VPL_OK;
}

// Methods to send a POST request.
int VPLHttp2__Impl::Post(const std::string &reqBody, std::string &respBody)
{
    [(__bridge NSURLConnectionHelper*)connectionHelper setConnectionBodyFromData:reqBody.c_str() DataLength:reqBody.size()];
    int rv = startToConnect("POST");
    if (rv != VPL_OK) {
        return rv;
    }

    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }
    return VPL_OK;
}

int VPLHttp2__Impl::Post(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    [(__bridge NSURLConnectionHelper*)connectionHelper setContentFilePathFromFilePath:reqBodyFilePath.c_str()];

    [(__bridge NSURLConnectionHelper*)connectionHelper registerSendProgressCallback:sendProgCb Interface:vplHttp2Interface Context:sendProgCtx];
    int rv = startToConnect("POST");
    if (rv != VPL_OK) {
        return rv;
    }

    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }
    return VPL_OK;
}

int VPLHttp2__Impl::Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx, std::string &respBody)
{
    char reqBody[sendSize];
    sendCb(vplHttp2Interface, sendCtx, reqBody, sendSize);
    [(__bridge NSURLConnectionHelper*)connectionHelper setConnectionBodyFromData:reqBody DataLength:sendSize];

    [(__bridge NSURLConnectionHelper*)connectionHelper registerSendProgressCallback:sendProgCb Interface:vplHttp2Interface Context:sendProgCtx];
    
    int rv = startToConnect("POST");
    if (rv != VPL_OK) {
        return rv;
    }

    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }
    return VPL_OK;
}

int VPLHttp2__Impl::Post(const std::string &reqBody, VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx)
{
    // pass the receive callback functions
    [(__bridge NSURLConnectionHelper*)connectionHelper registerReceiveProgressCallback:recvProgCb Interface:vplHttp2Interface Context:recvProgCtx];
    [(__bridge NSURLConnectionHelper*)connectionHelper registerReceiveCallback:recvCb Interface:vplHttp2Interface Context:recvCtx];
    // set the request body with a input string
    [(__bridge NSURLConnectionHelper*)connectionHelper setConnectionBodyFromData:reqBody.c_str() DataLength:reqBody.size()];
    
    int rv = startToConnect("POST");
    if (rv != VPL_OK) {
        return rv;
    }
    
    return VPL_OK;
}

// Method to send a DELETE request.
int VPLHttp2__Impl::Delete(std::string& respBody)
{
    int rv = startToConnect("DELETE");
    
    @autoreleasepool {
        std::string respondString((char*)[((__bridge NSURLConnectionHelper*)connectionHelper).responseData bytes], [((__bridge NSURLConnectionHelper*)connectionHelper).responseData length]);
        respBody = respondString;
    }
    
    return rv;
}

// Get HTTP status code.
int VPLHttp2__Impl::GetStatusCode()
{
    return [(__bridge NSURLConnectionHelper*)connectionHelper getStatusCode];
}

// Find response header value.
const std::string *VPLHttp2__Impl::FindResponseHeader(const std::string &field)
{
    NSString *fieldString = [NSString stringWithCString:field.c_str() encoding:[NSString defaultCStringEncoding]];
    const char *value = [(__bridge NSURLConnectionHelper*)connectionHelper findHeader:fieldString];
    
    const std::string *headerValueString = NULL;
    if (value != NULL) {
        headerValueString = new std::string(value);
    }
    
    return headerValueString;
}

// Abort transfer.
int VPLHttp2__Impl::Cancel()
{
    if (!connectionHelper) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,"connectionHelper is not initialized");
        return VPL_ERR_NOT_INIT;
    }
    
    [(__bridge NSURLConnectionHelper*)connectionHelper cancelConnection];
    return VPL_OK;
}

//====================== Private

int VPLHttp2__Impl::startToConnect(const char* method)
{
    if (!connectionHelper) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,"connectionHelper is not initialized");
        return VPL_ERR_NOT_INIT;
    }

    if ([((__bridge NSURLConnectionHelper*)connectionHelper).connectionURIString length] <= 0) {
        VPL_LIB_LOG_ERR(VPL_SG_HTTP,"URI is empty");
        return VPL_ERR_HOSTNAME;
    }

    [(__bridge NSURLConnectionHelper*)connectionHelper setConnectionMethodFromCString:method];

    if (isDebugging) {
        VPL_LIB_LOG_INFO(VPL_SG_HTTP,"connection URI: %s", [((__bridge NSURLConnectionHelper*)connectionHelper).connectionURIString cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        VPL_LIB_LOG_INFO(VPL_SG_HTTP,"connection method: %s", method);
        VPL_LIB_LOG_INFO(VPL_SG_HTTP,"connection timeout: %d", ((__bridge NSURLConnectionHelper*)connectionHelper).timeoutInterval);
    }
    
    NSCondition *condition = [[NSCondition alloc] init];
    NSThread *thread = [[NSThread alloc] initWithTarget:(__bridge NSURLConnectionHelper*)connectionHelper selector:@selector(startConnection:) object:condition ];

    [thread start];
    [condition lock];
    while (!((__bridge NSURLConnectionHelper*)connectionHelper).isFinished) {
        [condition wait];
    }
    [condition unlock];
    [thread cancel];

    return ((__bridge NSURLConnectionHelper*)connectionHelper).errorReturnCode;
}

