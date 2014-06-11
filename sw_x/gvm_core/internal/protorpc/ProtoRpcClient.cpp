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

#include "ProtoRpcClient.h"

ProtoRpcClient::ProtoRpcClient(ProtoChannel& channel)
    : _channel(channel)
{}


bool
ProtoRpcClient::sendRpcRequest(
        const std::string& methodName,
        const ProtoMessage& request,
        RpcStatus& status)
{
    // Check for previous IO errors
    if (_channel.outputStreamError()) {
        status.set_status(RpcStatus_Status_IO_ERROR);
        status.set_errordetail("IO error occurred previously on output stream");
        goto fail;
    }
    if (_channel.inputStreamError()) {
        status.set_status(RpcStatus_Status_IO_ERROR);
        status.set_errordetail("IO error occurred previously on input stream");
        goto fail;
    }

    // Send request header
    {
        RpcRequestHeader reqHdr;
        reqHdr.set_methodname(methodName);
        if (!_channel.writeMessage(reqHdr, NULL)) {
            if (_channel.outputStreamError()) {
                goto output_stream_error;
            } else {
                status.set_status(RpcStatus_Status_INTERNAL_ERROR);
                status.set_errordetail("Problem sending request header: ");
                status.mutable_errordetail()->append(reqHdr.InitializationErrorString());
                goto fail;
            }
        }
    }

    // Send request body
    if (!_channel.writeMessage(request, NULL)) {
        if (_channel.outputStreamError()) {
            goto output_stream_error;
        } else {
            status.set_status(RpcStatus_Status_BAD_REQUEST);
            status.set_errordetail("Missing request field(s): ");
            status.mutable_errordetail()->append(request.InitializationErrorString());
            goto fail;
        }
    }

    // Flush stream
    if (!_channel.flushOutputStream()) {
        goto output_stream_error;
    }

    // Everything OK so far.
    return true;

output_stream_error:
    {
        status.set_status(RpcStatus_Status_IO_ERROR);
        std::string errorDetail;
        _channel.outputStreamErrorDetail(errorDetail);
        status.set_errordetail(errorDetail);
    }

fail:
    return false;
}

void
ProtoRpcClient::recvRpcResponse(ProtoMessage& response, RpcStatus& status)
{
    // Get response header
    if (!_channel.extractMessage(status)) {
        if (_channel.inputStreamError()) {
            status.Clear();
            goto input_stream_error;
        } else {
            std::string error("Problem receiving response header: ");
            error.append(status.InitializationErrorString());
            status.Clear();
            status.set_status(RpcStatus_Status_HEADER_ERROR);
            status.set_errordetail(error);
            goto done;
        }
    }

    // Get response body if status OK
    if ((status.status() == RpcStatus_Status_OK) && (status.appstatus() >= 0)) {
        if (!_channel.extractMessage(response)) {
            if (_channel.inputStreamError()) {
                status.Clear();
                goto input_stream_error;
            } else {
                status.set_status(RpcStatus_Status_BAD_RESPONSE);
                status.set_errordetail(response.InitializationErrorString());
                goto done;
            }
        }
    }

    // Everything OK.  Status was parsed from channel.
    goto done;

input_stream_error:
    {
        status.set_status(RpcStatus_Status_IO_ERROR);
        std::string errorDetail;
        _channel.inputStreamErrorDetail(errorDetail);
        status.set_errordetail(errorDetail);
    }
    goto done;

done:
    NO_OP();
}

void
ProtoRpcClient::sendRpc(const std::string& methodName,
        const ProtoMessage& request, ProtoMessage& response,
        RpcStatus& status)
{
    bool success = sendRpcRequest(methodName, request, status);
    if (success) {
        response.Clear();
        recvRpcResponse(response, status);
    }
}

const ProtoChannel&
ProtoRpcClient::channel(void)
{
    return _channel;
}
