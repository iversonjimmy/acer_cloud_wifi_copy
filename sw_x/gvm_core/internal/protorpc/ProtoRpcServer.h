/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#ifndef __PROTORPC_SERVER_H__
#define __PROTORPC_SERVER_H__

#include "ProtoRpc.h"
#include <rpc.pb.h>
#include <sstream>
#include <string>

static inline
void debugLogInvoke(DebugMsgCallback debugCallback, const std::string& methodName)
{
    if (debugCallback != NULL) {
        std::ostringstream temp;
        temp << "Invoking \"" << methodName << "\"";
        debugCallback(temp.str().c_str());
    }
}

static inline
void debugLogRequest(DebugRequestMsgCallback debugRequestCallback,
                     const std::string& methodName,
                     bool isValid,
                     const ProtoMessage& reqMsg)
{
    if (debugRequestCallback != NULL) {
        debugRequestCallback(methodName, isValid, reqMsg);
    }
}

static inline
void debugLogResponse(DebugResponseMsgCallback debugResponseCallback,
                      const std::string& methodName,
                      const RpcStatus& status,
                      const ProtoMessage *respMsg)
{
    if (debugResponseCallback != NULL) {
        debugResponseCallback(methodName, status, respMsg);
    }
}

static bool topBoilerplate(ProtoChannel& _channel,
                           ProtoMessage& request,
                           RpcStatus& status,
                           DebugRequestMsgCallback debugRequestCallback,
                           const RpcRequestHeader& header)
{
    bool isRequestValid = true;
    if (!_channel.extractMessage(request)) {
        if (!_channel.inputStreamError()) {
            status.set_status(RpcStatus::BAD_REQUEST_SERVER);
            std::string error("Error in request body: ");
            error.append(request.InitializationErrorString());
            status.set_errordetail(error);
        }
        isRequestValid = false;
    }
    debugLogRequest(debugRequestCallback, header.methodname(), isRequestValid, request);
    return isRequestValid;
}

static void bottomBoilerplate(ProtoMessage& response, RpcStatus& status)
{
    if ((status.appstatus() >= 0) && !response.IsInitialized()) {
        status.set_status(RpcStatus::BAD_RESPONSE_SERVER);
        status.set_errordetail(response.InitializationErrorString());
    } else {
        status.set_status(RpcStatus::OK);
    }
}

#endif // include guard
