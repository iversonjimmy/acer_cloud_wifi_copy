//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __PROTORPCCLIENTASYNC_H__
#define __PROTORPCCLIENTASYNC_H__

#include "ProtoRpcClient.h"
#include <memory>

namespace proto_rpc {

class ServiceClientAsync;

/// Tracks the state of a specific request.
class AsyncCallState
{
public:
    typedef bool (*SendRpcRequestCallback)(AsyncCallState* state, ProtoRpcClient& client);
    typedef void (*RecvRpcResponseCallback)(AsyncCallState* state, ProtoRpcClient& client);
    typedef void (*ProcessResultCallback)(AsyncCallState* state);
    typedef void (*AnyFunc)(void);
    
    AsyncCallState(int reqId,
            std::auto_ptr<ProtoMessage> req, std::auto_ptr<ProtoMessage> resp,
            ServiceClientAsync* asyncClient,
            SendRpcRequestCallback sendRpcRequest, RecvRpcResponseCallback recvRpcResponse,
            ProcessResultCallback callbackWrapper, AnyFunc callback, void* callbackParam) :
        requestId(reqId),
        request(req),
        response(resp),
        asyncClient(asyncClient),
        sendRpcRequestCallback(sendRpcRequest),
        recvRpcResponseCallback(recvRpcResponse),
        callbackWrapper(callbackWrapper),
        actualCallback(callback),
        actualCallbackParam(callbackParam),
        nextInQueue(NULL) {}
    
    /// Request ID, assigned by a #AsyncServiceClientState.
    const int requestId;

    std::auto_ptr<ProtoMessage> request;
    std::auto_ptr<ProtoMessage> response;

    /// Actual type is from "<service_name>-client-async.pb.h"
    ServiceClientAsync* asyncClient;

    RpcStatus status;

    /// Function to send/write the RPC request.
    /// Provided by "<service_name>-client-async.pb.cc".
    SendRpcRequestCallback sendRpcRequestCallback;
    /// Function to recv/read the RPC response.
    /// Provided by "<service_name>-client-async.pb.cc".
    RecvRpcResponseCallback recvRpcResponseCallback;

    /// Function that calls #actualCallback (after casting).
    /// Provided by "<service_name>-client-async.pb.cc".
    ProcessResultCallback callbackWrapper;

    /// Opaque-to-this-file consumer-provided callback.
    AnyFunc actualCallback;

    /// Opaque-to-this-file consumer-provided callback param.
    void* actualCallbackParam;

    AsyncCallState* nextInQueue;
    
    inline const ProtoMessage& getRequest() { return *(request.get()); }
    inline ProtoMessage& getResponse() { return *(response.get()); }
    inline bool sendRpcRequest(ProtoRpcClient& client) { return sendRpcRequestCallback(this, client); }
    inline void recvRpcResponse(ProtoRpcClient& client) { recvRpcResponseCallback(this, client); }
};

template <class req_type, class resp_type>
class AsyncCallStateImpl : public AsyncCallState
{
public:
    AsyncCallStateImpl(int reqId, ServiceClientAsync* asyncClient, const req_type& request,
            SendRpcRequestCallback sendRpcRequest, RecvRpcResponseCallback recvRpcResponse,
            ProcessResultCallback callbackWrapper, AnyFunc callback, void* callbackParam) :
        AsyncCallState(reqId, std::auto_ptr<ProtoMessage>(new req_type()),
                std::auto_ptr<ProtoMessage>(new resp_type()),
                asyncClient, sendRpcRequest, recvRpcResponse,
                callbackWrapper, callback, callbackParam)
    {
        ((req_type*)(this->request.get()))->CopyFrom(request);
    }
    
    inline const req_type& getRequest() { return *(req_type*)(request.get()); }
    inline resp_type& getResponse() { return *(resp_type*)(response.get()); }
};

class AsyncServiceClientStateImpl;

///
/// Aggregates multiple requests into a single response queue.
/// Multiple async service clients (of the same or different types) can all
/// share a single instance of #AsyncServiceClientState.
///
class AsyncServiceClientState
{
public:
    // For use by module-specific implementations:
    
    AsyncServiceClientState();
    ~AsyncServiceClientState();
    
    int processResult();
    
    // For use in generated code "<service_name>-client-async.pb.cc":
    
    int nextRequestId();
    int enqueueRequest(AsyncCallState* state);

//private:
    // For impl-use only; consider this private.
    AsyncServiceClientStateImpl* pImpl;
};

class ServiceClientAsync
{
public:
    typedef ProtoRpcClient* (*AcquireClientCallback)(void);
    typedef void (*ReleaseClientCallback)(ProtoRpcClient* client);

    ServiceClientAsync(
            proto_rpc::AsyncServiceClientState& serviceState,
            AcquireClientCallback acquireClient,
            ReleaseClientCallback releaseClient) :
        _serviceState(serviceState),
        _acquireClientCallback(acquireClient),
        _releaseClientCallback(releaseClient) {}

//private:
    // For impl-use only; consider this private.
    proto_rpc::AsyncServiceClientState& _serviceState;
    // For impl-use only; consider this private.
    AcquireClientCallback _acquireClientCallback;
    // For impl-use only; consider this private.
    ReleaseClientCallback _releaseClientCallback;
};

} // end namespace proto_rpc

#endif // include guard
