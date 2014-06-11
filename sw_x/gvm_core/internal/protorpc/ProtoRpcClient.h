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

#ifndef __PROTORPCCLIENT_H__
#define __PROTORPCCLIENT_H__

#include "vpl_types.h"
#include "vplu_common.h"

#include <string>

#include "ProtoChannel.h"
#include "ProtoRpc.h"
#include "rpc.pb.h"

/**
 * Class to send RPCs over a ProtoChannel and receive replies.
 */
class ProtoRpcClient
{
public:
    ProtoRpcClient(ProtoChannel& channel);

    /// Sends an RPC over the channel.  The RPC is dispatched to the named
    /// method.  Upon return, a valid status will be filled in, and the
    /// response may or may not be filled in.  The caller should check the
    /// status to see if the call was successful and the response was
    /// filled in.
    void sendRpc(const std::string& methodName, const ProtoMessage& request,
            ProtoMessage& response, RpcStatus& status);

    const ProtoChannel& channel(void);

    // sendRpc() split into two halves, for async support:
    bool sendRpcRequest(const std::string& methodName, const ProtoMessage& request, RpcStatus& status);
    void recvRpcResponse(ProtoMessage& response, RpcStatus& status);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(ProtoRpcClient);

    ProtoChannel& _channel;
};

#endif // include guard
