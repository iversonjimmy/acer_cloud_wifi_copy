// GENERATED FROM ccdi_rpc.proto, DO NOT EDIT

#include "ccdi_rpc-client-async.pb.h"

using std::string;

namespace ccd {

// -----------------------------------------------------------------------
// ccd::CCDIService
// -----------------------------------------------------------------------
ccd::CCDIServiceClientAsync::CCDIServiceClientAsync(proto_rpc::AsyncServiceClientState& asyncState, AcquireClientCallback acquireClient, ReleaseClientCallback releaseClient)
    : ServiceClientAsync(asyncState, acquireClient, releaseClient)
{}

int
ccd::CCDIServiceClientAsync::ProcessAsyncResponse()
{
    return _serviceState.processResult();
}

// ---------------------------------------
// EventsCreateQueue
// ---------------------------------------
static void CCDIService_EventsCreateQueue_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::EventsCreateQueueCallback callback =
            (ccd::CCDIServiceClientAsync::EventsCreateQueueCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::EventsCreateQueueOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::EventsCreateQueueInput, ccd::EventsCreateQueueOutput>
        EventsCreateQueueAsyncState;

static bool CCDIService_EventsCreateQueue_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EventsCreateQueueAsyncState* state = (EventsCreateQueueAsyncState*)arg;
    try {
        static const string method("EventsCreateQueue");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_EventsCreateQueue_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EventsCreateQueueAsyncState* state = (EventsCreateQueueAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::EventsCreateQueueAsync(const ccd::EventsCreateQueueInput& request,
        EventsCreateQueueCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        EventsCreateQueueAsyncState* asyncState =
                new EventsCreateQueueAsyncState(requestId, this, request,
                        CCDIService_EventsCreateQueue_send,
                        CCDIService_EventsCreateQueue_recv,
                        CCDIService_EventsCreateQueue_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// EventsDestroyQueue
// ---------------------------------------
static void CCDIService_EventsDestroyQueue_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::EventsDestroyQueueCallback callback =
            (ccd::CCDIServiceClientAsync::EventsDestroyQueueCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::EventsDestroyQueueInput, ccd::NoParamResponse>
        EventsDestroyQueueAsyncState;

static bool CCDIService_EventsDestroyQueue_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EventsDestroyQueueAsyncState* state = (EventsDestroyQueueAsyncState*)arg;
    try {
        static const string method("EventsDestroyQueue");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_EventsDestroyQueue_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EventsDestroyQueueAsyncState* state = (EventsDestroyQueueAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::EventsDestroyQueueAsync(const ccd::EventsDestroyQueueInput& request,
        EventsDestroyQueueCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        EventsDestroyQueueAsyncState* asyncState =
                new EventsDestroyQueueAsyncState(requestId, this, request,
                        CCDIService_EventsDestroyQueue_send,
                        CCDIService_EventsDestroyQueue_recv,
                        CCDIService_EventsDestroyQueue_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// EventsDequeue
// ---------------------------------------
static void CCDIService_EventsDequeue_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::EventsDequeueCallback callback =
            (ccd::CCDIServiceClientAsync::EventsDequeueCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::EventsDequeueOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::EventsDequeueInput, ccd::EventsDequeueOutput>
        EventsDequeueAsyncState;

static bool CCDIService_EventsDequeue_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EventsDequeueAsyncState* state = (EventsDequeueAsyncState*)arg;
    try {
        static const string method("EventsDequeue");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_EventsDequeue_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EventsDequeueAsyncState* state = (EventsDequeueAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::EventsDequeueAsync(const ccd::EventsDequeueInput& request,
        EventsDequeueCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        EventsDequeueAsyncState* asyncState =
                new EventsDequeueAsyncState(requestId, this, request,
                        CCDIService_EventsDequeue_send,
                        CCDIService_EventsDequeue_recv,
                        CCDIService_EventsDequeue_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetSystemState
// ---------------------------------------
static void CCDIService_GetSystemState_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetSystemStateCallback callback =
            (ccd::CCDIServiceClientAsync::GetSystemStateCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetSystemStateOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetSystemStateInput, ccd::GetSystemStateOutput>
        GetSystemStateAsyncState;

static bool CCDIService_GetSystemState_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetSystemStateAsyncState* state = (GetSystemStateAsyncState*)arg;
    try {
        static const string method("GetSystemState");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetSystemState_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetSystemStateAsyncState* state = (GetSystemStateAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetSystemStateAsync(const ccd::GetSystemStateInput& request,
        GetSystemStateCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetSystemStateAsyncState* asyncState =
                new GetSystemStateAsyncState(requestId, this, request,
                        CCDIService_GetSystemState_send,
                        CCDIService_GetSystemState_recv,
                        CCDIService_GetSystemState_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// Login
// ---------------------------------------
static void CCDIService_Login_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::LoginCallback callback =
            (ccd::CCDIServiceClientAsync::LoginCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::LoginOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::LoginInput, ccd::LoginOutput>
        LoginAsyncState;

static bool CCDIService_Login_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    LoginAsyncState* state = (LoginAsyncState*)arg;
    try {
        static const string method("Login");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_Login_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    LoginAsyncState* state = (LoginAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::LoginAsync(const ccd::LoginInput& request,
        LoginCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        LoginAsyncState* asyncState =
                new LoginAsyncState(requestId, this, request,
                        CCDIService_Login_send,
                        CCDIService_Login_recv,
                        CCDIService_Login_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// Logout
// ---------------------------------------
static void CCDIService_Logout_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::LogoutCallback callback =
            (ccd::CCDIServiceClientAsync::LogoutCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::LogoutInput, ccd::NoParamResponse>
        LogoutAsyncState;

static bool CCDIService_Logout_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    LogoutAsyncState* state = (LogoutAsyncState*)arg;
    try {
        static const string method("Logout");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_Logout_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    LogoutAsyncState* state = (LogoutAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::LogoutAsync(const ccd::LogoutInput& request,
        LogoutCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        LogoutAsyncState* asyncState =
                new LogoutAsyncState(requestId, this, request,
                        CCDIService_Logout_send,
                        CCDIService_Logout_recv,
                        CCDIService_Logout_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// InfraHttpRequest
// ---------------------------------------
static void CCDIService_InfraHttpRequest_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::InfraHttpRequestCallback callback =
            (ccd::CCDIServiceClientAsync::InfraHttpRequestCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::InfraHttpRequestOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::InfraHttpRequestInput, ccd::InfraHttpRequestOutput>
        InfraHttpRequestAsyncState;

static bool CCDIService_InfraHttpRequest_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    InfraHttpRequestAsyncState* state = (InfraHttpRequestAsyncState*)arg;
    try {
        static const string method("InfraHttpRequest");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_InfraHttpRequest_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    InfraHttpRequestAsyncState* state = (InfraHttpRequestAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::InfraHttpRequestAsync(const ccd::InfraHttpRequestInput& request,
        InfraHttpRequestCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        InfraHttpRequestAsyncState* asyncState =
                new InfraHttpRequestAsyncState(requestId, this, request,
                        CCDIService_InfraHttpRequest_send,
                        CCDIService_InfraHttpRequest_recv,
                        CCDIService_InfraHttpRequest_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UpdateAppState
// ---------------------------------------
static void CCDIService_UpdateAppState_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UpdateAppStateCallback callback =
            (ccd::CCDIServiceClientAsync::UpdateAppStateCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::UpdateAppStateOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UpdateAppStateInput, ccd::UpdateAppStateOutput>
        UpdateAppStateAsyncState;

static bool CCDIService_UpdateAppState_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateAppStateAsyncState* state = (UpdateAppStateAsyncState*)arg;
    try {
        static const string method("UpdateAppState");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UpdateAppState_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateAppStateAsyncState* state = (UpdateAppStateAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UpdateAppStateAsync(const ccd::UpdateAppStateInput& request,
        UpdateAppStateCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UpdateAppStateAsyncState* asyncState =
                new UpdateAppStateAsyncState(requestId, this, request,
                        CCDIService_UpdateAppState_send,
                        CCDIService_UpdateAppState_recv,
                        CCDIService_UpdateAppState_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UpdateSystemState
// ---------------------------------------
static void CCDIService_UpdateSystemState_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UpdateSystemStateCallback callback =
            (ccd::CCDIServiceClientAsync::UpdateSystemStateCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::UpdateSystemStateOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UpdateSystemStateInput, ccd::UpdateSystemStateOutput>
        UpdateSystemStateAsyncState;

static bool CCDIService_UpdateSystemState_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateSystemStateAsyncState* state = (UpdateSystemStateAsyncState*)arg;
    try {
        static const string method("UpdateSystemState");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UpdateSystemState_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateSystemStateAsyncState* state = (UpdateSystemStateAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UpdateSystemStateAsync(const ccd::UpdateSystemStateInput& request,
        UpdateSystemStateCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UpdateSystemStateAsyncState* asyncState =
                new UpdateSystemStateAsyncState(requestId, this, request,
                        CCDIService_UpdateSystemState_send,
                        CCDIService_UpdateSystemState_recv,
                        CCDIService_UpdateSystemState_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RegisterStorageNode
// ---------------------------------------
static void CCDIService_RegisterStorageNode_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RegisterStorageNodeCallback callback =
            (ccd::CCDIServiceClientAsync::RegisterStorageNodeCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RegisterStorageNodeInput, ccd::NoParamResponse>
        RegisterStorageNodeAsyncState;

static bool CCDIService_RegisterStorageNode_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RegisterStorageNodeAsyncState* state = (RegisterStorageNodeAsyncState*)arg;
    try {
        static const string method("RegisterStorageNode");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RegisterStorageNode_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RegisterStorageNodeAsyncState* state = (RegisterStorageNodeAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RegisterStorageNodeAsync(const ccd::RegisterStorageNodeInput& request,
        RegisterStorageNodeCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RegisterStorageNodeAsyncState* asyncState =
                new RegisterStorageNodeAsyncState(requestId, this, request,
                        CCDIService_RegisterStorageNode_send,
                        CCDIService_RegisterStorageNode_recv,
                        CCDIService_RegisterStorageNode_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UnregisterStorageNode
// ---------------------------------------
static void CCDIService_UnregisterStorageNode_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UnregisterStorageNodeCallback callback =
            (ccd::CCDIServiceClientAsync::UnregisterStorageNodeCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UnregisterStorageNodeInput, ccd::NoParamResponse>
        UnregisterStorageNodeAsyncState;

static bool CCDIService_UnregisterStorageNode_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UnregisterStorageNodeAsyncState* state = (UnregisterStorageNodeAsyncState*)arg;
    try {
        static const string method("UnregisterStorageNode");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UnregisterStorageNode_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UnregisterStorageNodeAsyncState* state = (UnregisterStorageNodeAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UnregisterStorageNodeAsync(const ccd::UnregisterStorageNodeInput& request,
        UnregisterStorageNodeCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UnregisterStorageNodeAsyncState* asyncState =
                new UnregisterStorageNodeAsyncState(requestId, this, request,
                        CCDIService_UnregisterStorageNode_send,
                        CCDIService_UnregisterStorageNode_recv,
                        CCDIService_UnregisterStorageNode_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UpdateStorageNode
// ---------------------------------------
static void CCDIService_UpdateStorageNode_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UpdateStorageNodeCallback callback =
            (ccd::CCDIServiceClientAsync::UpdateStorageNodeCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UpdateStorageNodeInput, ccd::NoParamResponse>
        UpdateStorageNodeAsyncState;

static bool CCDIService_UpdateStorageNode_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateStorageNodeAsyncState* state = (UpdateStorageNodeAsyncState*)arg;
    try {
        static const string method("UpdateStorageNode");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UpdateStorageNode_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateStorageNodeAsyncState* state = (UpdateStorageNodeAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UpdateStorageNodeAsync(const ccd::UpdateStorageNodeInput& request,
        UpdateStorageNodeCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UpdateStorageNodeAsyncState* asyncState =
                new UpdateStorageNodeAsyncState(requestId, this, request,
                        CCDIService_UpdateStorageNode_send,
                        CCDIService_UpdateStorageNode_recv,
                        CCDIService_UpdateStorageNode_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ReportLanDevices
// ---------------------------------------
static void CCDIService_ReportLanDevices_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ReportLanDevicesCallback callback =
            (ccd::CCDIServiceClientAsync::ReportLanDevicesCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::ReportLanDevicesInput, ccd::NoParamResponse>
        ReportLanDevicesAsyncState;

static bool CCDIService_ReportLanDevices_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ReportLanDevicesAsyncState* state = (ReportLanDevicesAsyncState*)arg;
    try {
        static const string method("ReportLanDevices");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ReportLanDevices_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ReportLanDevicesAsyncState* state = (ReportLanDevicesAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ReportLanDevicesAsync(const ccd::ReportLanDevicesInput& request,
        ReportLanDevicesCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ReportLanDevicesAsyncState* asyncState =
                new ReportLanDevicesAsyncState(requestId, this, request,
                        CCDIService_ReportLanDevices_send,
                        CCDIService_ReportLanDevices_recv,
                        CCDIService_ReportLanDevices_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ListLanDevices
// ---------------------------------------
static void CCDIService_ListLanDevices_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ListLanDevicesCallback callback =
            (ccd::CCDIServiceClientAsync::ListLanDevicesCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::ListLanDevicesOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::ListLanDevicesInput, ccd::ListLanDevicesOutput>
        ListLanDevicesAsyncState;

static bool CCDIService_ListLanDevices_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListLanDevicesAsyncState* state = (ListLanDevicesAsyncState*)arg;
    try {
        static const string method("ListLanDevices");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ListLanDevices_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListLanDevicesAsyncState* state = (ListLanDevicesAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ListLanDevicesAsync(const ccd::ListLanDevicesInput& request,
        ListLanDevicesCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ListLanDevicesAsyncState* asyncState =
                new ListLanDevicesAsyncState(requestId, this, request,
                        CCDIService_ListLanDevices_send,
                        CCDIService_ListLanDevices_recv,
                        CCDIService_ListLanDevices_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ProbeLanDevices
// ---------------------------------------
static void CCDIService_ProbeLanDevices_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ProbeLanDevicesCallback callback =
            (ccd::CCDIServiceClientAsync::ProbeLanDevicesCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, ccd::NoParamResponse>
        ProbeLanDevicesAsyncState;

static bool CCDIService_ProbeLanDevices_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ProbeLanDevicesAsyncState* state = (ProbeLanDevicesAsyncState*)arg;
    try {
        static const string method("ProbeLanDevices");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ProbeLanDevices_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ProbeLanDevicesAsyncState* state = (ProbeLanDevicesAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ProbeLanDevicesAsync(const ccd::NoParamRequest& request,
        ProbeLanDevicesCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ProbeLanDevicesAsyncState* asyncState =
                new ProbeLanDevicesAsyncState(requestId, this, request,
                        CCDIService_ProbeLanDevices_send,
                        CCDIService_ProbeLanDevices_recv,
                        CCDIService_ProbeLanDevices_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ListStorageNodeDatasets
// ---------------------------------------
static void CCDIService_ListStorageNodeDatasets_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ListStorageNodeDatasetsCallback callback =
            (ccd::CCDIServiceClientAsync::ListStorageNodeDatasetsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::ListStorageNodeDatasetsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, ccd::ListStorageNodeDatasetsOutput>
        ListStorageNodeDatasetsAsyncState;

static bool CCDIService_ListStorageNodeDatasets_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListStorageNodeDatasetsAsyncState* state = (ListStorageNodeDatasetsAsyncState*)arg;
    try {
        static const string method("ListStorageNodeDatasets");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ListStorageNodeDatasets_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListStorageNodeDatasetsAsyncState* state = (ListStorageNodeDatasetsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ListStorageNodeDatasetsAsync(const ccd::NoParamRequest& request,
        ListStorageNodeDatasetsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ListStorageNodeDatasetsAsyncState* asyncState =
                new ListStorageNodeDatasetsAsyncState(requestId, this, request,
                        CCDIService_ListStorageNodeDatasets_send,
                        CCDIService_ListStorageNodeDatasets_recv,
                        CCDIService_ListStorageNodeDatasets_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// AddDataset
// ---------------------------------------
static void CCDIService_AddDataset_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::AddDatasetCallback callback =
            (ccd::CCDIServiceClientAsync::AddDatasetCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::AddDatasetOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::AddDatasetInput, ccd::AddDatasetOutput>
        AddDatasetAsyncState;

static bool CCDIService_AddDataset_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    AddDatasetAsyncState* state = (AddDatasetAsyncState*)arg;
    try {
        static const string method("AddDataset");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_AddDataset_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    AddDatasetAsyncState* state = (AddDatasetAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::AddDatasetAsync(const ccd::AddDatasetInput& request,
        AddDatasetCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        AddDatasetAsyncState* asyncState =
                new AddDatasetAsyncState(requestId, this, request,
                        CCDIService_AddDataset_send,
                        CCDIService_AddDataset_recv,
                        CCDIService_AddDataset_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// AddSyncSubscription
// ---------------------------------------
static void CCDIService_AddSyncSubscription_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::AddSyncSubscriptionCallback callback =
            (ccd::CCDIServiceClientAsync::AddSyncSubscriptionCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::AddSyncSubscriptionInput, ccd::NoParamResponse>
        AddSyncSubscriptionAsyncState;

static bool CCDIService_AddSyncSubscription_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    AddSyncSubscriptionAsyncState* state = (AddSyncSubscriptionAsyncState*)arg;
    try {
        static const string method("AddSyncSubscription");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_AddSyncSubscription_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    AddSyncSubscriptionAsyncState* state = (AddSyncSubscriptionAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::AddSyncSubscriptionAsync(const ccd::AddSyncSubscriptionInput& request,
        AddSyncSubscriptionCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        AddSyncSubscriptionAsyncState* asyncState =
                new AddSyncSubscriptionAsyncState(requestId, this, request,
                        CCDIService_AddSyncSubscription_send,
                        CCDIService_AddSyncSubscription_recv,
                        CCDIService_AddSyncSubscription_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// DeleteDataset
// ---------------------------------------
static void CCDIService_DeleteDataset_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::DeleteDatasetCallback callback =
            (ccd::CCDIServiceClientAsync::DeleteDatasetCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::DeleteDatasetInput, ccd::NoParamResponse>
        DeleteDatasetAsyncState;

static bool CCDIService_DeleteDataset_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    DeleteDatasetAsyncState* state = (DeleteDatasetAsyncState*)arg;
    try {
        static const string method("DeleteDataset");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_DeleteDataset_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    DeleteDatasetAsyncState* state = (DeleteDatasetAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::DeleteDatasetAsync(const ccd::DeleteDatasetInput& request,
        DeleteDatasetCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        DeleteDatasetAsyncState* asyncState =
                new DeleteDatasetAsyncState(requestId, this, request,
                        CCDIService_DeleteDataset_send,
                        CCDIService_DeleteDataset_recv,
                        CCDIService_DeleteDataset_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// DeleteSyncSubscriptions
// ---------------------------------------
static void CCDIService_DeleteSyncSubscriptions_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::DeleteSyncSubscriptionsCallback callback =
            (ccd::CCDIServiceClientAsync::DeleteSyncSubscriptionsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::DeleteSyncSubscriptionsInput, ccd::NoParamResponse>
        DeleteSyncSubscriptionsAsyncState;

static bool CCDIService_DeleteSyncSubscriptions_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    DeleteSyncSubscriptionsAsyncState* state = (DeleteSyncSubscriptionsAsyncState*)arg;
    try {
        static const string method("DeleteSyncSubscriptions");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_DeleteSyncSubscriptions_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    DeleteSyncSubscriptionsAsyncState* state = (DeleteSyncSubscriptionsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::DeleteSyncSubscriptionsAsync(const ccd::DeleteSyncSubscriptionsInput& request,
        DeleteSyncSubscriptionsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        DeleteSyncSubscriptionsAsyncState* asyncState =
                new DeleteSyncSubscriptionsAsyncState(requestId, this, request,
                        CCDIService_DeleteSyncSubscriptions_send,
                        CCDIService_DeleteSyncSubscriptions_recv,
                        CCDIService_DeleteSyncSubscriptions_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetDatasetDirectoryEntries
// ---------------------------------------
static void CCDIService_GetDatasetDirectoryEntries_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetDatasetDirectoryEntriesCallback callback =
            (ccd::CCDIServiceClientAsync::GetDatasetDirectoryEntriesCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetDatasetDirectoryEntriesOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetDatasetDirectoryEntriesInput, ccd::GetDatasetDirectoryEntriesOutput>
        GetDatasetDirectoryEntriesAsyncState;

static bool CCDIService_GetDatasetDirectoryEntries_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetDatasetDirectoryEntriesAsyncState* state = (GetDatasetDirectoryEntriesAsyncState*)arg;
    try {
        static const string method("GetDatasetDirectoryEntries");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetDatasetDirectoryEntries_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetDatasetDirectoryEntriesAsyncState* state = (GetDatasetDirectoryEntriesAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetDatasetDirectoryEntriesAsync(const ccd::GetDatasetDirectoryEntriesInput& request,
        GetDatasetDirectoryEntriesCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetDatasetDirectoryEntriesAsyncState* asyncState =
                new GetDatasetDirectoryEntriesAsyncState(requestId, this, request,
                        CCDIService_GetDatasetDirectoryEntries_send,
                        CCDIService_GetDatasetDirectoryEntries_recv,
                        CCDIService_GetDatasetDirectoryEntries_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetInfraHttpInfo
// ---------------------------------------
static void CCDIService_GetInfraHttpInfo_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetInfraHttpInfoCallback callback =
            (ccd::CCDIServiceClientAsync::GetInfraHttpInfoCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetInfraHttpInfoOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetInfraHttpInfoInput, ccd::GetInfraHttpInfoOutput>
        GetInfraHttpInfoAsyncState;

static bool CCDIService_GetInfraHttpInfo_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetInfraHttpInfoAsyncState* state = (GetInfraHttpInfoAsyncState*)arg;
    try {
        static const string method("GetInfraHttpInfo");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetInfraHttpInfo_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetInfraHttpInfoAsyncState* state = (GetInfraHttpInfoAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetInfraHttpInfoAsync(const ccd::GetInfraHttpInfoInput& request,
        GetInfraHttpInfoCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetInfraHttpInfoAsyncState* asyncState =
                new GetInfraHttpInfoAsyncState(requestId, this, request,
                        CCDIService_GetInfraHttpInfo_send,
                        CCDIService_GetInfraHttpInfo_recv,
                        CCDIService_GetInfraHttpInfo_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetLocalHttpInfo
// ---------------------------------------
static void CCDIService_GetLocalHttpInfo_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetLocalHttpInfoCallback callback =
            (ccd::CCDIServiceClientAsync::GetLocalHttpInfoCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetLocalHttpInfoOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetLocalHttpInfoInput, ccd::GetLocalHttpInfoOutput>
        GetLocalHttpInfoAsyncState;

static bool CCDIService_GetLocalHttpInfo_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetLocalHttpInfoAsyncState* state = (GetLocalHttpInfoAsyncState*)arg;
    try {
        static const string method("GetLocalHttpInfo");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetLocalHttpInfo_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetLocalHttpInfoAsyncState* state = (GetLocalHttpInfoAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetLocalHttpInfoAsync(const ccd::GetLocalHttpInfoInput& request,
        GetLocalHttpInfoCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetLocalHttpInfoAsyncState* asyncState =
                new GetLocalHttpInfoAsyncState(requestId, this, request,
                        CCDIService_GetLocalHttpInfo_send,
                        CCDIService_GetLocalHttpInfo_recv,
                        CCDIService_GetLocalHttpInfo_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetPersonalCloudState
// ---------------------------------------
static void CCDIService_GetPersonalCloudState_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetPersonalCloudStateCallback callback =
            (ccd::CCDIServiceClientAsync::GetPersonalCloudStateCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetPersonalCloudStateOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetPersonalCloudStateInput, ccd::GetPersonalCloudStateOutput>
        GetPersonalCloudStateAsyncState;

static bool CCDIService_GetPersonalCloudState_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetPersonalCloudStateAsyncState* state = (GetPersonalCloudStateAsyncState*)arg;
    try {
        static const string method("GetPersonalCloudState");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetPersonalCloudState_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetPersonalCloudStateAsyncState* state = (GetPersonalCloudStateAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetPersonalCloudStateAsync(const ccd::GetPersonalCloudStateInput& request,
        GetPersonalCloudStateCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetPersonalCloudStateAsyncState* asyncState =
                new GetPersonalCloudStateAsyncState(requestId, this, request,
                        CCDIService_GetPersonalCloudState_send,
                        CCDIService_GetPersonalCloudState_recv,
                        CCDIService_GetPersonalCloudState_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetSyncState
// ---------------------------------------
static void CCDIService_GetSyncState_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetSyncStateCallback callback =
            (ccd::CCDIServiceClientAsync::GetSyncStateCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetSyncStateOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetSyncStateInput, ccd::GetSyncStateOutput>
        GetSyncStateAsyncState;

static bool CCDIService_GetSyncState_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetSyncStateAsyncState* state = (GetSyncStateAsyncState*)arg;
    try {
        static const string method("GetSyncState");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetSyncState_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetSyncStateAsyncState* state = (GetSyncStateAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetSyncStateAsync(const ccd::GetSyncStateInput& request,
        GetSyncStateCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetSyncStateAsyncState* asyncState =
                new GetSyncStateAsyncState(requestId, this, request,
                        CCDIService_GetSyncState_send,
                        CCDIService_GetSyncState_recv,
                        CCDIService_GetSyncState_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetSyncStateNotifications
// ---------------------------------------
static void CCDIService_GetSyncStateNotifications_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetSyncStateNotificationsCallback callback =
            (ccd::CCDIServiceClientAsync::GetSyncStateNotificationsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetSyncStateNotificationsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetSyncStateNotificationsInput, ccd::GetSyncStateNotificationsOutput>
        GetSyncStateNotificationsAsyncState;

static bool CCDIService_GetSyncStateNotifications_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetSyncStateNotificationsAsyncState* state = (GetSyncStateNotificationsAsyncState*)arg;
    try {
        static const string method("GetSyncStateNotifications");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetSyncStateNotifications_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetSyncStateNotificationsAsyncState* state = (GetSyncStateNotificationsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetSyncStateNotificationsAsync(const ccd::GetSyncStateNotificationsInput& request,
        GetSyncStateNotificationsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetSyncStateNotificationsAsyncState* asyncState =
                new GetSyncStateNotificationsAsyncState(requestId, this, request,
                        CCDIService_GetSyncStateNotifications_send,
                        CCDIService_GetSyncStateNotifications_recv,
                        CCDIService_GetSyncStateNotifications_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// LinkDevice
// ---------------------------------------
static void CCDIService_LinkDevice_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::LinkDeviceCallback callback =
            (ccd::CCDIServiceClientAsync::LinkDeviceCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::LinkDeviceInput, ccd::NoParamResponse>
        LinkDeviceAsyncState;

static bool CCDIService_LinkDevice_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    LinkDeviceAsyncState* state = (LinkDeviceAsyncState*)arg;
    try {
        static const string method("LinkDevice");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_LinkDevice_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    LinkDeviceAsyncState* state = (LinkDeviceAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::LinkDeviceAsync(const ccd::LinkDeviceInput& request,
        LinkDeviceCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        LinkDeviceAsyncState* asyncState =
                new LinkDeviceAsyncState(requestId, this, request,
                        CCDIService_LinkDevice_send,
                        CCDIService_LinkDevice_recv,
                        CCDIService_LinkDevice_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ListLinkedDevices
// ---------------------------------------
static void CCDIService_ListLinkedDevices_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ListLinkedDevicesCallback callback =
            (ccd::CCDIServiceClientAsync::ListLinkedDevicesCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::ListLinkedDevicesOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::ListLinkedDevicesInput, ccd::ListLinkedDevicesOutput>
        ListLinkedDevicesAsyncState;

static bool CCDIService_ListLinkedDevices_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListLinkedDevicesAsyncState* state = (ListLinkedDevicesAsyncState*)arg;
    try {
        static const string method("ListLinkedDevices");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ListLinkedDevices_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListLinkedDevicesAsyncState* state = (ListLinkedDevicesAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ListLinkedDevicesAsync(const ccd::ListLinkedDevicesInput& request,
        ListLinkedDevicesCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ListLinkedDevicesAsyncState* asyncState =
                new ListLinkedDevicesAsyncState(requestId, this, request,
                        CCDIService_ListLinkedDevices_send,
                        CCDIService_ListLinkedDevices_recv,
                        CCDIService_ListLinkedDevices_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ListOwnedDatasets
// ---------------------------------------
static void CCDIService_ListOwnedDatasets_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ListOwnedDatasetsCallback callback =
            (ccd::CCDIServiceClientAsync::ListOwnedDatasetsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::ListOwnedDatasetsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::ListOwnedDatasetsInput, ccd::ListOwnedDatasetsOutput>
        ListOwnedDatasetsAsyncState;

static bool CCDIService_ListOwnedDatasets_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListOwnedDatasetsAsyncState* state = (ListOwnedDatasetsAsyncState*)arg;
    try {
        static const string method("ListOwnedDatasets");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ListOwnedDatasets_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListOwnedDatasetsAsyncState* state = (ListOwnedDatasetsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ListOwnedDatasetsAsync(const ccd::ListOwnedDatasetsInput& request,
        ListOwnedDatasetsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ListOwnedDatasetsAsyncState* asyncState =
                new ListOwnedDatasetsAsyncState(requestId, this, request,
                        CCDIService_ListOwnedDatasets_send,
                        CCDIService_ListOwnedDatasets_recv,
                        CCDIService_ListOwnedDatasets_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ListSyncSubscriptions
// ---------------------------------------
static void CCDIService_ListSyncSubscriptions_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ListSyncSubscriptionsCallback callback =
            (ccd::CCDIServiceClientAsync::ListSyncSubscriptionsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::ListSyncSubscriptionsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::ListSyncSubscriptionsInput, ccd::ListSyncSubscriptionsOutput>
        ListSyncSubscriptionsAsyncState;

static bool CCDIService_ListSyncSubscriptions_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListSyncSubscriptionsAsyncState* state = (ListSyncSubscriptionsAsyncState*)arg;
    try {
        static const string method("ListSyncSubscriptions");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ListSyncSubscriptions_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListSyncSubscriptionsAsyncState* state = (ListSyncSubscriptionsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ListSyncSubscriptionsAsync(const ccd::ListSyncSubscriptionsInput& request,
        ListSyncSubscriptionsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ListSyncSubscriptionsAsyncState* asyncState =
                new ListSyncSubscriptionsAsyncState(requestId, this, request,
                        CCDIService_ListSyncSubscriptions_send,
                        CCDIService_ListSyncSubscriptions_recv,
                        CCDIService_ListSyncSubscriptions_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// OwnershipSync
// ---------------------------------------
static void CCDIService_OwnershipSync_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::OwnershipSyncCallback callback =
            (ccd::CCDIServiceClientAsync::OwnershipSyncCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, ccd::NoParamResponse>
        OwnershipSyncAsyncState;

static bool CCDIService_OwnershipSync_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    OwnershipSyncAsyncState* state = (OwnershipSyncAsyncState*)arg;
    try {
        static const string method("OwnershipSync");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_OwnershipSync_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    OwnershipSyncAsyncState* state = (OwnershipSyncAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::OwnershipSyncAsync(const ccd::NoParamRequest& request,
        OwnershipSyncCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        OwnershipSyncAsyncState* asyncState =
                new OwnershipSyncAsyncState(requestId, this, request,
                        CCDIService_OwnershipSync_send,
                        CCDIService_OwnershipSync_recv,
                        CCDIService_OwnershipSync_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// PrivateMsaDataCommit
// ---------------------------------------
static void CCDIService_PrivateMsaDataCommit_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::PrivateMsaDataCommitCallback callback =
            (ccd::CCDIServiceClientAsync::PrivateMsaDataCommitCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::PrivateMsaDataCommitInput, ccd::NoParamResponse>
        PrivateMsaDataCommitAsyncState;

static bool CCDIService_PrivateMsaDataCommit_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    PrivateMsaDataCommitAsyncState* state = (PrivateMsaDataCommitAsyncState*)arg;
    try {
        static const string method("PrivateMsaDataCommit");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_PrivateMsaDataCommit_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    PrivateMsaDataCommitAsyncState* state = (PrivateMsaDataCommitAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::PrivateMsaDataCommitAsync(const ccd::PrivateMsaDataCommitInput& request,
        PrivateMsaDataCommitCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        PrivateMsaDataCommitAsyncState* asyncState =
                new PrivateMsaDataCommitAsyncState(requestId, this, request,
                        CCDIService_PrivateMsaDataCommit_send,
                        CCDIService_PrivateMsaDataCommit_recv,
                        CCDIService_PrivateMsaDataCommit_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RemoteWakeup
// ---------------------------------------
static void CCDIService_RemoteWakeup_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RemoteWakeupCallback callback =
            (ccd::CCDIServiceClientAsync::RemoteWakeupCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RemoteWakeupInput, ccd::NoParamResponse>
        RemoteWakeupAsyncState;

static bool CCDIService_RemoteWakeup_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RemoteWakeupAsyncState* state = (RemoteWakeupAsyncState*)arg;
    try {
        static const string method("RemoteWakeup");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RemoteWakeup_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RemoteWakeupAsyncState* state = (RemoteWakeupAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RemoteWakeupAsync(const ccd::RemoteWakeupInput& request,
        RemoteWakeupCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RemoteWakeupAsyncState* asyncState =
                new RemoteWakeupAsyncState(requestId, this, request,
                        CCDIService_RemoteWakeup_send,
                        CCDIService_RemoteWakeup_recv,
                        CCDIService_RemoteWakeup_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RenameDataset
// ---------------------------------------
static void CCDIService_RenameDataset_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RenameDatasetCallback callback =
            (ccd::CCDIServiceClientAsync::RenameDatasetCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RenameDatasetInput, ccd::NoParamResponse>
        RenameDatasetAsyncState;

static bool CCDIService_RenameDataset_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RenameDatasetAsyncState* state = (RenameDatasetAsyncState*)arg;
    try {
        static const string method("RenameDataset");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RenameDataset_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RenameDatasetAsyncState* state = (RenameDatasetAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RenameDatasetAsync(const ccd::RenameDatasetInput& request,
        RenameDatasetCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RenameDatasetAsyncState* asyncState =
                new RenameDatasetAsyncState(requestId, this, request,
                        CCDIService_RenameDataset_send,
                        CCDIService_RenameDataset_recv,
                        CCDIService_RenameDataset_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SyncOnce
// ---------------------------------------
static void CCDIService_SyncOnce_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SyncOnceCallback callback =
            (ccd::CCDIServiceClientAsync::SyncOnceCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::SyncOnceOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SyncOnceInput, ccd::SyncOnceOutput>
        SyncOnceAsyncState;

static bool CCDIService_SyncOnce_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SyncOnceAsyncState* state = (SyncOnceAsyncState*)arg;
    try {
        static const string method("SyncOnce");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SyncOnce_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SyncOnceAsyncState* state = (SyncOnceAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SyncOnceAsync(const ccd::SyncOnceInput& request,
        SyncOnceCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SyncOnceAsyncState* asyncState =
                new SyncOnceAsyncState(requestId, this, request,
                        CCDIService_SyncOnce_send,
                        CCDIService_SyncOnce_recv,
                        CCDIService_SyncOnce_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UnlinkDevice
// ---------------------------------------
static void CCDIService_UnlinkDevice_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UnlinkDeviceCallback callback =
            (ccd::CCDIServiceClientAsync::UnlinkDeviceCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UnlinkDeviceInput, ccd::NoParamResponse>
        UnlinkDeviceAsyncState;

static bool CCDIService_UnlinkDevice_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UnlinkDeviceAsyncState* state = (UnlinkDeviceAsyncState*)arg;
    try {
        static const string method("UnlinkDevice");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UnlinkDevice_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UnlinkDeviceAsyncState* state = (UnlinkDeviceAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UnlinkDeviceAsync(const ccd::UnlinkDeviceInput& request,
        UnlinkDeviceCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UnlinkDeviceAsyncState* asyncState =
                new UnlinkDeviceAsyncState(requestId, this, request,
                        CCDIService_UnlinkDevice_send,
                        CCDIService_UnlinkDevice_recv,
                        CCDIService_UnlinkDevice_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UpdateSyncSettings
// ---------------------------------------
static void CCDIService_UpdateSyncSettings_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UpdateSyncSettingsCallback callback =
            (ccd::CCDIServiceClientAsync::UpdateSyncSettingsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::UpdateSyncSettingsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UpdateSyncSettingsInput, ccd::UpdateSyncSettingsOutput>
        UpdateSyncSettingsAsyncState;

static bool CCDIService_UpdateSyncSettings_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateSyncSettingsAsyncState* state = (UpdateSyncSettingsAsyncState*)arg;
    try {
        static const string method("UpdateSyncSettings");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UpdateSyncSettings_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateSyncSettingsAsyncState* state = (UpdateSyncSettingsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UpdateSyncSettingsAsync(const ccd::UpdateSyncSettingsInput& request,
        UpdateSyncSettingsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UpdateSyncSettingsAsyncState* asyncState =
                new UpdateSyncSettingsAsyncState(requestId, this, request,
                        CCDIService_UpdateSyncSettings_send,
                        CCDIService_UpdateSyncSettings_recv,
                        CCDIService_UpdateSyncSettings_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UpdateSyncSubscription
// ---------------------------------------
static void CCDIService_UpdateSyncSubscription_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UpdateSyncSubscriptionCallback callback =
            (ccd::CCDIServiceClientAsync::UpdateSyncSubscriptionCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UpdateSyncSubscriptionInput, ccd::NoParamResponse>
        UpdateSyncSubscriptionAsyncState;

static bool CCDIService_UpdateSyncSubscription_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateSyncSubscriptionAsyncState* state = (UpdateSyncSubscriptionAsyncState*)arg;
    try {
        static const string method("UpdateSyncSubscription");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UpdateSyncSubscription_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UpdateSyncSubscriptionAsyncState* state = (UpdateSyncSubscriptionAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UpdateSyncSubscriptionAsync(const ccd::UpdateSyncSubscriptionInput& request,
        UpdateSyncSubscriptionCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UpdateSyncSubscriptionAsyncState* asyncState =
                new UpdateSyncSubscriptionAsyncState(requestId, this, request,
                        CCDIService_UpdateSyncSubscription_send,
                        CCDIService_UpdateSyncSubscription_recv,
                        CCDIService_UpdateSyncSubscription_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ListUserStorage
// ---------------------------------------
static void CCDIService_ListUserStorage_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ListUserStorageCallback callback =
            (ccd::CCDIServiceClientAsync::ListUserStorageCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::ListUserStorageOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::ListUserStorageInput, ccd::ListUserStorageOutput>
        ListUserStorageAsyncState;

static bool CCDIService_ListUserStorage_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListUserStorageAsyncState* state = (ListUserStorageAsyncState*)arg;
    try {
        static const string method("ListUserStorage");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ListUserStorage_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListUserStorageAsyncState* state = (ListUserStorageAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ListUserStorageAsync(const ccd::ListUserStorageInput& request,
        ListUserStorageCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ListUserStorageAsyncState* asyncState =
                new ListUserStorageAsyncState(requestId, this, request,
                        CCDIService_ListUserStorage_send,
                        CCDIService_ListUserStorage_recv,
                        CCDIService_ListUserStorage_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RemoteSwUpdateMessage
// ---------------------------------------
static void CCDIService_RemoteSwUpdateMessage_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RemoteSwUpdateMessageCallback callback =
            (ccd::CCDIServiceClientAsync::RemoteSwUpdateMessageCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RemoteSwUpdateMessageInput, ccd::NoParamResponse>
        RemoteSwUpdateMessageAsyncState;

static bool CCDIService_RemoteSwUpdateMessage_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RemoteSwUpdateMessageAsyncState* state = (RemoteSwUpdateMessageAsyncState*)arg;
    try {
        static const string method("RemoteSwUpdateMessage");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RemoteSwUpdateMessage_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RemoteSwUpdateMessageAsyncState* state = (RemoteSwUpdateMessageAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RemoteSwUpdateMessageAsync(const ccd::RemoteSwUpdateMessageInput& request,
        RemoteSwUpdateMessageCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RemoteSwUpdateMessageAsyncState* asyncState =
                new RemoteSwUpdateMessageAsyncState(requestId, this, request,
                        CCDIService_RemoteSwUpdateMessage_send,
                        CCDIService_RemoteSwUpdateMessage_recv,
                        CCDIService_RemoteSwUpdateMessage_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SWUpdateCheck
// ---------------------------------------
static void CCDIService_SWUpdateCheck_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SWUpdateCheckCallback callback =
            (ccd::CCDIServiceClientAsync::SWUpdateCheckCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::SWUpdateCheckOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SWUpdateCheckInput, ccd::SWUpdateCheckOutput>
        SWUpdateCheckAsyncState;

static bool CCDIService_SWUpdateCheck_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateCheckAsyncState* state = (SWUpdateCheckAsyncState*)arg;
    try {
        static const string method("SWUpdateCheck");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SWUpdateCheck_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateCheckAsyncState* state = (SWUpdateCheckAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SWUpdateCheckAsync(const ccd::SWUpdateCheckInput& request,
        SWUpdateCheckCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SWUpdateCheckAsyncState* asyncState =
                new SWUpdateCheckAsyncState(requestId, this, request,
                        CCDIService_SWUpdateCheck_send,
                        CCDIService_SWUpdateCheck_recv,
                        CCDIService_SWUpdateCheck_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SWUpdateBeginDownload
// ---------------------------------------
static void CCDIService_SWUpdateBeginDownload_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SWUpdateBeginDownloadCallback callback =
            (ccd::CCDIServiceClientAsync::SWUpdateBeginDownloadCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::SWUpdateBeginDownloadOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SWUpdateBeginDownloadInput, ccd::SWUpdateBeginDownloadOutput>
        SWUpdateBeginDownloadAsyncState;

static bool CCDIService_SWUpdateBeginDownload_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateBeginDownloadAsyncState* state = (SWUpdateBeginDownloadAsyncState*)arg;
    try {
        static const string method("SWUpdateBeginDownload");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SWUpdateBeginDownload_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateBeginDownloadAsyncState* state = (SWUpdateBeginDownloadAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SWUpdateBeginDownloadAsync(const ccd::SWUpdateBeginDownloadInput& request,
        SWUpdateBeginDownloadCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SWUpdateBeginDownloadAsyncState* asyncState =
                new SWUpdateBeginDownloadAsyncState(requestId, this, request,
                        CCDIService_SWUpdateBeginDownload_send,
                        CCDIService_SWUpdateBeginDownload_recv,
                        CCDIService_SWUpdateBeginDownload_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SWUpdateGetDownloadProgress
// ---------------------------------------
static void CCDIService_SWUpdateGetDownloadProgress_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SWUpdateGetDownloadProgressCallback callback =
            (ccd::CCDIServiceClientAsync::SWUpdateGetDownloadProgressCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::SWUpdateGetDownloadProgressOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SWUpdateGetDownloadProgressInput, ccd::SWUpdateGetDownloadProgressOutput>
        SWUpdateGetDownloadProgressAsyncState;

static bool CCDIService_SWUpdateGetDownloadProgress_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateGetDownloadProgressAsyncState* state = (SWUpdateGetDownloadProgressAsyncState*)arg;
    try {
        static const string method("SWUpdateGetDownloadProgress");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SWUpdateGetDownloadProgress_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateGetDownloadProgressAsyncState* state = (SWUpdateGetDownloadProgressAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SWUpdateGetDownloadProgressAsync(const ccd::SWUpdateGetDownloadProgressInput& request,
        SWUpdateGetDownloadProgressCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SWUpdateGetDownloadProgressAsyncState* asyncState =
                new SWUpdateGetDownloadProgressAsyncState(requestId, this, request,
                        CCDIService_SWUpdateGetDownloadProgress_send,
                        CCDIService_SWUpdateGetDownloadProgress_recv,
                        CCDIService_SWUpdateGetDownloadProgress_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SWUpdateEndDownload
// ---------------------------------------
static void CCDIService_SWUpdateEndDownload_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SWUpdateEndDownloadCallback callback =
            (ccd::CCDIServiceClientAsync::SWUpdateEndDownloadCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SWUpdateEndDownloadInput, ccd::NoParamResponse>
        SWUpdateEndDownloadAsyncState;

static bool CCDIService_SWUpdateEndDownload_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateEndDownloadAsyncState* state = (SWUpdateEndDownloadAsyncState*)arg;
    try {
        static const string method("SWUpdateEndDownload");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SWUpdateEndDownload_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateEndDownloadAsyncState* state = (SWUpdateEndDownloadAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SWUpdateEndDownloadAsync(const ccd::SWUpdateEndDownloadInput& request,
        SWUpdateEndDownloadCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SWUpdateEndDownloadAsyncState* asyncState =
                new SWUpdateEndDownloadAsyncState(requestId, this, request,
                        CCDIService_SWUpdateEndDownload_send,
                        CCDIService_SWUpdateEndDownload_recv,
                        CCDIService_SWUpdateEndDownload_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SWUpdateCancelDownload
// ---------------------------------------
static void CCDIService_SWUpdateCancelDownload_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SWUpdateCancelDownloadCallback callback =
            (ccd::CCDIServiceClientAsync::SWUpdateCancelDownloadCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SWUpdateCancelDownloadInput, ccd::NoParamResponse>
        SWUpdateCancelDownloadAsyncState;

static bool CCDIService_SWUpdateCancelDownload_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateCancelDownloadAsyncState* state = (SWUpdateCancelDownloadAsyncState*)arg;
    try {
        static const string method("SWUpdateCancelDownload");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SWUpdateCancelDownload_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateCancelDownloadAsyncState* state = (SWUpdateCancelDownloadAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SWUpdateCancelDownloadAsync(const ccd::SWUpdateCancelDownloadInput& request,
        SWUpdateCancelDownloadCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SWUpdateCancelDownloadAsyncState* asyncState =
                new SWUpdateCancelDownloadAsyncState(requestId, this, request,
                        CCDIService_SWUpdateCancelDownload_send,
                        CCDIService_SWUpdateCancelDownload_recv,
                        CCDIService_SWUpdateCancelDownload_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SWUpdateSetCcdVersion
// ---------------------------------------
static void CCDIService_SWUpdateSetCcdVersion_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SWUpdateSetCcdVersionCallback callback =
            (ccd::CCDIServiceClientAsync::SWUpdateSetCcdVersionCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SWUpdateSetCcdVersionInput, ccd::NoParamResponse>
        SWUpdateSetCcdVersionAsyncState;

static bool CCDIService_SWUpdateSetCcdVersion_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateSetCcdVersionAsyncState* state = (SWUpdateSetCcdVersionAsyncState*)arg;
    try {
        static const string method("SWUpdateSetCcdVersion");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SWUpdateSetCcdVersion_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SWUpdateSetCcdVersionAsyncState* state = (SWUpdateSetCcdVersionAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SWUpdateSetCcdVersionAsync(const ccd::SWUpdateSetCcdVersionInput& request,
        SWUpdateSetCcdVersionCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SWUpdateSetCcdVersionAsyncState* asyncState =
                new SWUpdateSetCcdVersionAsyncState(requestId, this, request,
                        CCDIService_SWUpdateSetCcdVersion_send,
                        CCDIService_SWUpdateSetCcdVersion_recv,
                        CCDIService_SWUpdateSetCcdVersion_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSABeginCatalog
// ---------------------------------------
static void CCDIService_MSABeginCatalog_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSABeginCatalogCallback callback =
            (ccd::CCDIServiceClientAsync::MSABeginCatalogCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::BeginCatalogInput, ccd::NoParamResponse>
        MSABeginCatalogAsyncState;

static bool CCDIService_MSABeginCatalog_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSABeginCatalogAsyncState* state = (MSABeginCatalogAsyncState*)arg;
    try {
        static const string method("MSABeginCatalog");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSABeginCatalog_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSABeginCatalogAsyncState* state = (MSABeginCatalogAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSABeginCatalogAsync(const ccd::BeginCatalogInput& request,
        MSABeginCatalogCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSABeginCatalogAsyncState* asyncState =
                new MSABeginCatalogAsyncState(requestId, this, request,
                        CCDIService_MSABeginCatalog_send,
                        CCDIService_MSABeginCatalog_recv,
                        CCDIService_MSABeginCatalog_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSACommitCatalog
// ---------------------------------------
static void CCDIService_MSACommitCatalog_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSACommitCatalogCallback callback =
            (ccd::CCDIServiceClientAsync::MSACommitCatalogCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::CommitCatalogInput, ccd::NoParamResponse>
        MSACommitCatalogAsyncState;

static bool CCDIService_MSACommitCatalog_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSACommitCatalogAsyncState* state = (MSACommitCatalogAsyncState*)arg;
    try {
        static const string method("MSACommitCatalog");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSACommitCatalog_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSACommitCatalogAsyncState* state = (MSACommitCatalogAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSACommitCatalogAsync(const ccd::CommitCatalogInput& request,
        MSACommitCatalogCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSACommitCatalogAsyncState* asyncState =
                new MSACommitCatalogAsyncState(requestId, this, request,
                        CCDIService_MSACommitCatalog_send,
                        CCDIService_MSACommitCatalog_recv,
                        CCDIService_MSACommitCatalog_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSAEndCatalog
// ---------------------------------------
static void CCDIService_MSAEndCatalog_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSAEndCatalogCallback callback =
            (ccd::CCDIServiceClientAsync::MSAEndCatalogCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::EndCatalogInput, ccd::NoParamResponse>
        MSAEndCatalogAsyncState;

static bool CCDIService_MSAEndCatalog_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAEndCatalogAsyncState* state = (MSAEndCatalogAsyncState*)arg;
    try {
        static const string method("MSAEndCatalog");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSAEndCatalog_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAEndCatalogAsyncState* state = (MSAEndCatalogAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSAEndCatalogAsync(const ccd::EndCatalogInput& request,
        MSAEndCatalogCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSAEndCatalogAsyncState* asyncState =
                new MSAEndCatalogAsyncState(requestId, this, request,
                        CCDIService_MSAEndCatalog_send,
                        CCDIService_MSAEndCatalog_recv,
                        CCDIService_MSAEndCatalog_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSABeginMetadataTransaction
// ---------------------------------------
static void CCDIService_MSABeginMetadataTransaction_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSABeginMetadataTransactionCallback callback =
            (ccd::CCDIServiceClientAsync::MSABeginMetadataTransactionCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::BeginMetadataTransactionInput, ccd::NoParamResponse>
        MSABeginMetadataTransactionAsyncState;

static bool CCDIService_MSABeginMetadataTransaction_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSABeginMetadataTransactionAsyncState* state = (MSABeginMetadataTransactionAsyncState*)arg;
    try {
        static const string method("MSABeginMetadataTransaction");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSABeginMetadataTransaction_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSABeginMetadataTransactionAsyncState* state = (MSABeginMetadataTransactionAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSABeginMetadataTransactionAsync(const ccd::BeginMetadataTransactionInput& request,
        MSABeginMetadataTransactionCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSABeginMetadataTransactionAsyncState* asyncState =
                new MSABeginMetadataTransactionAsyncState(requestId, this, request,
                        CCDIService_MSABeginMetadataTransaction_send,
                        CCDIService_MSABeginMetadataTransaction_recv,
                        CCDIService_MSABeginMetadataTransaction_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSAUpdateMetadata
// ---------------------------------------
static void CCDIService_MSAUpdateMetadata_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSAUpdateMetadataCallback callback =
            (ccd::CCDIServiceClientAsync::MSAUpdateMetadataCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UpdateMetadataInput, ccd::NoParamResponse>
        MSAUpdateMetadataAsyncState;

static bool CCDIService_MSAUpdateMetadata_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAUpdateMetadataAsyncState* state = (MSAUpdateMetadataAsyncState*)arg;
    try {
        static const string method("MSAUpdateMetadata");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSAUpdateMetadata_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAUpdateMetadataAsyncState* state = (MSAUpdateMetadataAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSAUpdateMetadataAsync(const ccd::UpdateMetadataInput& request,
        MSAUpdateMetadataCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSAUpdateMetadataAsyncState* asyncState =
                new MSAUpdateMetadataAsyncState(requestId, this, request,
                        CCDIService_MSAUpdateMetadata_send,
                        CCDIService_MSAUpdateMetadata_recv,
                        CCDIService_MSAUpdateMetadata_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSADeleteMetadata
// ---------------------------------------
static void CCDIService_MSADeleteMetadata_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSADeleteMetadataCallback callback =
            (ccd::CCDIServiceClientAsync::MSADeleteMetadataCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::DeleteMetadataInput, ccd::NoParamResponse>
        MSADeleteMetadataAsyncState;

static bool CCDIService_MSADeleteMetadata_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSADeleteMetadataAsyncState* state = (MSADeleteMetadataAsyncState*)arg;
    try {
        static const string method("MSADeleteMetadata");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSADeleteMetadata_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSADeleteMetadataAsyncState* state = (MSADeleteMetadataAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSADeleteMetadataAsync(const ccd::DeleteMetadataInput& request,
        MSADeleteMetadataCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSADeleteMetadataAsyncState* asyncState =
                new MSADeleteMetadataAsyncState(requestId, this, request,
                        CCDIService_MSADeleteMetadata_send,
                        CCDIService_MSADeleteMetadata_recv,
                        CCDIService_MSADeleteMetadata_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSACommitMetadataTransaction
// ---------------------------------------
static void CCDIService_MSACommitMetadataTransaction_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSACommitMetadataTransactionCallback callback =
            (ccd::CCDIServiceClientAsync::MSACommitMetadataTransactionCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, ccd::NoParamResponse>
        MSACommitMetadataTransactionAsyncState;

static bool CCDIService_MSACommitMetadataTransaction_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSACommitMetadataTransactionAsyncState* state = (MSACommitMetadataTransactionAsyncState*)arg;
    try {
        static const string method("MSACommitMetadataTransaction");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSACommitMetadataTransaction_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSACommitMetadataTransactionAsyncState* state = (MSACommitMetadataTransactionAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSACommitMetadataTransactionAsync(const ccd::NoParamRequest& request,
        MSACommitMetadataTransactionCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSACommitMetadataTransactionAsyncState* asyncState =
                new MSACommitMetadataTransactionAsyncState(requestId, this, request,
                        CCDIService_MSACommitMetadataTransaction_send,
                        CCDIService_MSACommitMetadataTransaction_recv,
                        CCDIService_MSACommitMetadataTransaction_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSAGetMetadataSyncState
// ---------------------------------------
static void CCDIService_MSAGetMetadataSyncState_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSAGetMetadataSyncStateCallback callback =
            (ccd::CCDIServiceClientAsync::MSAGetMetadataSyncStateCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (media_metadata::GetMetadataSyncStateOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, media_metadata::GetMetadataSyncStateOutput>
        MSAGetMetadataSyncStateAsyncState;

static bool CCDIService_MSAGetMetadataSyncState_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAGetMetadataSyncStateAsyncState* state = (MSAGetMetadataSyncStateAsyncState*)arg;
    try {
        static const string method("MSAGetMetadataSyncState");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSAGetMetadataSyncState_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAGetMetadataSyncStateAsyncState* state = (MSAGetMetadataSyncStateAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSAGetMetadataSyncStateAsync(const ccd::NoParamRequest& request,
        MSAGetMetadataSyncStateCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSAGetMetadataSyncStateAsyncState* asyncState =
                new MSAGetMetadataSyncStateAsyncState(requestId, this, request,
                        CCDIService_MSAGetMetadataSyncState_send,
                        CCDIService_MSAGetMetadataSyncState_recv,
                        CCDIService_MSAGetMetadataSyncState_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSADeleteCollection
// ---------------------------------------
static void CCDIService_MSADeleteCollection_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSADeleteCollectionCallback callback =
            (ccd::CCDIServiceClientAsync::MSADeleteCollectionCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::DeleteCollectionInput, ccd::NoParamResponse>
        MSADeleteCollectionAsyncState;

static bool CCDIService_MSADeleteCollection_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSADeleteCollectionAsyncState* state = (MSADeleteCollectionAsyncState*)arg;
    try {
        static const string method("MSADeleteCollection");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSADeleteCollection_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSADeleteCollectionAsyncState* state = (MSADeleteCollectionAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSADeleteCollectionAsync(const ccd::DeleteCollectionInput& request,
        MSADeleteCollectionCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSADeleteCollectionAsyncState* asyncState =
                new MSADeleteCollectionAsyncState(requestId, this, request,
                        CCDIService_MSADeleteCollection_send,
                        CCDIService_MSADeleteCollection_recv,
                        CCDIService_MSADeleteCollection_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSADeleteCatalog
// ---------------------------------------
static void CCDIService_MSADeleteCatalog_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSADeleteCatalogCallback callback =
            (ccd::CCDIServiceClientAsync::MSADeleteCatalogCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::DeleteCatalogInput, ccd::NoParamResponse>
        MSADeleteCatalogAsyncState;

static bool CCDIService_MSADeleteCatalog_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSADeleteCatalogAsyncState* state = (MSADeleteCatalogAsyncState*)arg;
    try {
        static const string method("MSADeleteCatalog");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSADeleteCatalog_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSADeleteCatalogAsyncState* state = (MSADeleteCatalogAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSADeleteCatalogAsync(const ccd::DeleteCatalogInput& request,
        MSADeleteCatalogCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSADeleteCatalogAsyncState* asyncState =
                new MSADeleteCatalogAsyncState(requestId, this, request,
                        CCDIService_MSADeleteCatalog_send,
                        CCDIService_MSADeleteCatalog_recv,
                        CCDIService_MSADeleteCatalog_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSAListCollections
// ---------------------------------------
static void CCDIService_MSAListCollections_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSAListCollectionsCallback callback =
            (ccd::CCDIServiceClientAsync::MSAListCollectionsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (media_metadata::ListCollectionsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, media_metadata::ListCollectionsOutput>
        MSAListCollectionsAsyncState;

static bool CCDIService_MSAListCollections_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAListCollectionsAsyncState* state = (MSAListCollectionsAsyncState*)arg;
    try {
        static const string method("MSAListCollections");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSAListCollections_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAListCollectionsAsyncState* state = (MSAListCollectionsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSAListCollectionsAsync(const ccd::NoParamRequest& request,
        MSAListCollectionsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSAListCollectionsAsyncState* asyncState =
                new MSAListCollectionsAsyncState(requestId, this, request,
                        CCDIService_MSAListCollections_send,
                        CCDIService_MSAListCollections_recv,
                        CCDIService_MSAListCollections_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSAGetCollectionDetails
// ---------------------------------------
static void CCDIService_MSAGetCollectionDetails_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSAGetCollectionDetailsCallback callback =
            (ccd::CCDIServiceClientAsync::MSAGetCollectionDetailsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetCollectionDetailsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetCollectionDetailsInput, ccd::GetCollectionDetailsOutput>
        MSAGetCollectionDetailsAsyncState;

static bool CCDIService_MSAGetCollectionDetails_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAGetCollectionDetailsAsyncState* state = (MSAGetCollectionDetailsAsyncState*)arg;
    try {
        static const string method("MSAGetCollectionDetails");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSAGetCollectionDetails_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAGetCollectionDetailsAsyncState* state = (MSAGetCollectionDetailsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSAGetCollectionDetailsAsync(const ccd::GetCollectionDetailsInput& request,
        MSAGetCollectionDetailsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSAGetCollectionDetailsAsyncState* asyncState =
                new MSAGetCollectionDetailsAsyncState(requestId, this, request,
                        CCDIService_MSAGetCollectionDetails_send,
                        CCDIService_MSAGetCollectionDetails_recv,
                        CCDIService_MSAGetCollectionDetails_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MSAGetContentURL
// ---------------------------------------
static void CCDIService_MSAGetContentURL_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MSAGetContentURLCallback callback =
            (ccd::CCDIServiceClientAsync::MSAGetContentURLCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::MSAGetContentURLOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::MSAGetContentURLInput, ccd::MSAGetContentURLOutput>
        MSAGetContentURLAsyncState;

static bool CCDIService_MSAGetContentURL_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAGetContentURLAsyncState* state = (MSAGetContentURLAsyncState*)arg;
    try {
        static const string method("MSAGetContentURL");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MSAGetContentURL_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MSAGetContentURLAsyncState* state = (MSAGetContentURLAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MSAGetContentURLAsync(const ccd::MSAGetContentURLInput& request,
        MSAGetContentURLCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MSAGetContentURLAsyncState* asyncState =
                new MSAGetContentURLAsyncState(requestId, this, request,
                        CCDIService_MSAGetContentURL_send,
                        CCDIService_MSAGetContentURL_recv,
                        CCDIService_MSAGetContentURL_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// MCAQueryMetadataObjects
// ---------------------------------------
static void CCDIService_MCAQueryMetadataObjects_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::MCAQueryMetadataObjectsCallback callback =
            (ccd::CCDIServiceClientAsync::MCAQueryMetadataObjectsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::MCAQueryMetadataObjectsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::MCAQueryMetadataObjectsInput, ccd::MCAQueryMetadataObjectsOutput>
        MCAQueryMetadataObjectsAsyncState;

static bool CCDIService_MCAQueryMetadataObjects_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MCAQueryMetadataObjectsAsyncState* state = (MCAQueryMetadataObjectsAsyncState*)arg;
    try {
        static const string method("MCAQueryMetadataObjects");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_MCAQueryMetadataObjects_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    MCAQueryMetadataObjectsAsyncState* state = (MCAQueryMetadataObjectsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::MCAQueryMetadataObjectsAsync(const ccd::MCAQueryMetadataObjectsInput& request,
        MCAQueryMetadataObjectsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        MCAQueryMetadataObjectsAsyncState* asyncState =
                new MCAQueryMetadataObjectsAsyncState(requestId, this, request,
                        CCDIService_MCAQueryMetadataObjects_send,
                        CCDIService_MCAQueryMetadataObjects_recv,
                        CCDIService_MCAQueryMetadataObjects_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// EnableInMemoryLogging
// ---------------------------------------
static void CCDIService_EnableInMemoryLogging_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::EnableInMemoryLoggingCallback callback =
            (ccd::CCDIServiceClientAsync::EnableInMemoryLoggingCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, ccd::NoParamResponse>
        EnableInMemoryLoggingAsyncState;

static bool CCDIService_EnableInMemoryLogging_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EnableInMemoryLoggingAsyncState* state = (EnableInMemoryLoggingAsyncState*)arg;
    try {
        static const string method("EnableInMemoryLogging");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_EnableInMemoryLogging_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    EnableInMemoryLoggingAsyncState* state = (EnableInMemoryLoggingAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::EnableInMemoryLoggingAsync(const ccd::NoParamRequest& request,
        EnableInMemoryLoggingCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        EnableInMemoryLoggingAsyncState* asyncState =
                new EnableInMemoryLoggingAsyncState(requestId, this, request,
                        CCDIService_EnableInMemoryLogging_send,
                        CCDIService_EnableInMemoryLogging_recv,
                        CCDIService_EnableInMemoryLogging_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// DisableInMemoryLogging
// ---------------------------------------
static void CCDIService_DisableInMemoryLogging_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::DisableInMemoryLoggingCallback callback =
            (ccd::CCDIServiceClientAsync::DisableInMemoryLoggingCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, ccd::NoParamResponse>
        DisableInMemoryLoggingAsyncState;

static bool CCDIService_DisableInMemoryLogging_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    DisableInMemoryLoggingAsyncState* state = (DisableInMemoryLoggingAsyncState*)arg;
    try {
        static const string method("DisableInMemoryLogging");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_DisableInMemoryLogging_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    DisableInMemoryLoggingAsyncState* state = (DisableInMemoryLoggingAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::DisableInMemoryLoggingAsync(const ccd::NoParamRequest& request,
        DisableInMemoryLoggingCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        DisableInMemoryLoggingAsyncState* asyncState =
                new DisableInMemoryLoggingAsyncState(requestId, this, request,
                        CCDIService_DisableInMemoryLogging_send,
                        CCDIService_DisableInMemoryLogging_recv,
                        CCDIService_DisableInMemoryLogging_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// FlushInMemoryLogs
// ---------------------------------------
static void CCDIService_FlushInMemoryLogs_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::FlushInMemoryLogsCallback callback =
            (ccd::CCDIServiceClientAsync::FlushInMemoryLogsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::NoParamRequest, ccd::NoParamResponse>
        FlushInMemoryLogsAsyncState;

static bool CCDIService_FlushInMemoryLogs_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    FlushInMemoryLogsAsyncState* state = (FlushInMemoryLogsAsyncState*)arg;
    try {
        static const string method("FlushInMemoryLogs");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_FlushInMemoryLogs_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    FlushInMemoryLogsAsyncState* state = (FlushInMemoryLogsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::FlushInMemoryLogsAsync(const ccd::NoParamRequest& request,
        FlushInMemoryLogsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        FlushInMemoryLogsAsyncState* asyncState =
                new FlushInMemoryLogsAsyncState(requestId, this, request,
                        CCDIService_FlushInMemoryLogs_send,
                        CCDIService_FlushInMemoryLogs_recv,
                        CCDIService_FlushInMemoryLogs_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RespondToPairingRequest
// ---------------------------------------
static void CCDIService_RespondToPairingRequest_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RespondToPairingRequestCallback callback =
            (ccd::CCDIServiceClientAsync::RespondToPairingRequestCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RespondToPairingRequestInput, ccd::NoParamResponse>
        RespondToPairingRequestAsyncState;

static bool CCDIService_RespondToPairingRequest_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RespondToPairingRequestAsyncState* state = (RespondToPairingRequestAsyncState*)arg;
    try {
        static const string method("RespondToPairingRequest");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RespondToPairingRequest_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RespondToPairingRequestAsyncState* state = (RespondToPairingRequestAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RespondToPairingRequestAsync(const ccd::RespondToPairingRequestInput& request,
        RespondToPairingRequestCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RespondToPairingRequestAsyncState* asyncState =
                new RespondToPairingRequestAsyncState(requestId, this, request,
                        CCDIService_RespondToPairingRequest_send,
                        CCDIService_RespondToPairingRequest_recv,
                        CCDIService_RespondToPairingRequest_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RequestPairing
// ---------------------------------------
static void CCDIService_RequestPairing_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RequestPairingCallback callback =
            (ccd::CCDIServiceClientAsync::RequestPairingCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::RequestPairingOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RequestPairingInput, ccd::RequestPairingOutput>
        RequestPairingAsyncState;

static bool CCDIService_RequestPairing_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RequestPairingAsyncState* state = (RequestPairingAsyncState*)arg;
    try {
        static const string method("RequestPairing");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RequestPairing_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RequestPairingAsyncState* state = (RequestPairingAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RequestPairingAsync(const ccd::RequestPairingInput& request,
        RequestPairingCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RequestPairingAsyncState* asyncState =
                new RequestPairingAsyncState(requestId, this, request,
                        CCDIService_RequestPairing_send,
                        CCDIService_RequestPairing_recv,
                        CCDIService_RequestPairing_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RequestPairingPin
// ---------------------------------------
static void CCDIService_RequestPairingPin_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RequestPairingPinCallback callback =
            (ccd::CCDIServiceClientAsync::RequestPairingPinCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::RequestPairingPinOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RequestPairingPinInput, ccd::RequestPairingPinOutput>
        RequestPairingPinAsyncState;

static bool CCDIService_RequestPairingPin_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RequestPairingPinAsyncState* state = (RequestPairingPinAsyncState*)arg;
    try {
        static const string method("RequestPairingPin");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RequestPairingPin_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RequestPairingPinAsyncState* state = (RequestPairingPinAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RequestPairingPinAsync(const ccd::RequestPairingPinInput& request,
        RequestPairingPinCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RequestPairingPinAsyncState* asyncState =
                new RequestPairingPinAsyncState(requestId, this, request,
                        CCDIService_RequestPairingPin_send,
                        CCDIService_RequestPairingPin_recv,
                        CCDIService_RequestPairingPin_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// GetPairingStatus
// ---------------------------------------
static void CCDIService_GetPairingStatus_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::GetPairingStatusCallback callback =
            (ccd::CCDIServiceClientAsync::GetPairingStatusCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::GetPairingStatusOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::GetPairingStatusInput, ccd::GetPairingStatusOutput>
        GetPairingStatusAsyncState;

static bool CCDIService_GetPairingStatus_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetPairingStatusAsyncState* state = (GetPairingStatusAsyncState*)arg;
    try {
        static const string method("GetPairingStatus");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_GetPairingStatus_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    GetPairingStatusAsyncState* state = (GetPairingStatusAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::GetPairingStatusAsync(const ccd::GetPairingStatusInput& request,
        GetPairingStatusCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        GetPairingStatusAsyncState* asyncState =
                new GetPairingStatusAsyncState(requestId, this, request,
                        CCDIService_GetPairingStatus_send,
                        CCDIService_GetPairingStatus_recv,
                        CCDIService_GetPairingStatus_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// QueryPicStreamObjects
// ---------------------------------------
static void CCDIService_QueryPicStreamObjects_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::QueryPicStreamObjectsCallback callback =
            (ccd::CCDIServiceClientAsync::QueryPicStreamObjectsCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::CCDIQueryPicStreamObjectsOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::CCDIQueryPicStreamObjectsInput, ccd::CCDIQueryPicStreamObjectsOutput>
        QueryPicStreamObjectsAsyncState;

static bool CCDIService_QueryPicStreamObjects_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    QueryPicStreamObjectsAsyncState* state = (QueryPicStreamObjectsAsyncState*)arg;
    try {
        static const string method("QueryPicStreamObjects");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_QueryPicStreamObjects_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    QueryPicStreamObjectsAsyncState* state = (QueryPicStreamObjectsAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::QueryPicStreamObjectsAsync(const ccd::CCDIQueryPicStreamObjectsInput& request,
        QueryPicStreamObjectsCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        QueryPicStreamObjectsAsyncState* asyncState =
                new QueryPicStreamObjectsAsyncState(requestId, this, request,
                        CCDIService_QueryPicStreamObjects_send,
                        CCDIService_QueryPicStreamObjects_recv,
                        CCDIService_QueryPicStreamObjects_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SharedFilesStoreFile
// ---------------------------------------
static void CCDIService_SharedFilesStoreFile_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SharedFilesStoreFileCallback callback =
            (ccd::CCDIServiceClientAsync::SharedFilesStoreFileCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::SharedFilesStoreFileOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SharedFilesStoreFileInput, ccd::SharedFilesStoreFileOutput>
        SharedFilesStoreFileAsyncState;

static bool CCDIService_SharedFilesStoreFile_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesStoreFileAsyncState* state = (SharedFilesStoreFileAsyncState*)arg;
    try {
        static const string method("SharedFilesStoreFile");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SharedFilesStoreFile_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesStoreFileAsyncState* state = (SharedFilesStoreFileAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SharedFilesStoreFileAsync(const ccd::SharedFilesStoreFileInput& request,
        SharedFilesStoreFileCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SharedFilesStoreFileAsyncState* asyncState =
                new SharedFilesStoreFileAsyncState(requestId, this, request,
                        CCDIService_SharedFilesStoreFile_send,
                        CCDIService_SharedFilesStoreFile_recv,
                        CCDIService_SharedFilesStoreFile_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SharedFilesShareFile
// ---------------------------------------
static void CCDIService_SharedFilesShareFile_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SharedFilesShareFileCallback callback =
            (ccd::CCDIServiceClientAsync::SharedFilesShareFileCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SharedFilesShareFileInput, ccd::NoParamResponse>
        SharedFilesShareFileAsyncState;

static bool CCDIService_SharedFilesShareFile_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesShareFileAsyncState* state = (SharedFilesShareFileAsyncState*)arg;
    try {
        static const string method("SharedFilesShareFile");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SharedFilesShareFile_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesShareFileAsyncState* state = (SharedFilesShareFileAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SharedFilesShareFileAsync(const ccd::SharedFilesShareFileInput& request,
        SharedFilesShareFileCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SharedFilesShareFileAsyncState* asyncState =
                new SharedFilesShareFileAsyncState(requestId, this, request,
                        CCDIService_SharedFilesShareFile_send,
                        CCDIService_SharedFilesShareFile_recv,
                        CCDIService_SharedFilesShareFile_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SharedFilesUnshareFile
// ---------------------------------------
static void CCDIService_SharedFilesUnshareFile_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SharedFilesUnshareFileCallback callback =
            (ccd::CCDIServiceClientAsync::SharedFilesUnshareFileCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SharedFilesUnshareFileInput, ccd::NoParamResponse>
        SharedFilesUnshareFileAsyncState;

static bool CCDIService_SharedFilesUnshareFile_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesUnshareFileAsyncState* state = (SharedFilesUnshareFileAsyncState*)arg;
    try {
        static const string method("SharedFilesUnshareFile");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SharedFilesUnshareFile_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesUnshareFileAsyncState* state = (SharedFilesUnshareFileAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SharedFilesUnshareFileAsync(const ccd::SharedFilesUnshareFileInput& request,
        SharedFilesUnshareFileCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SharedFilesUnshareFileAsyncState* asyncState =
                new SharedFilesUnshareFileAsyncState(requestId, this, request,
                        CCDIService_SharedFilesUnshareFile_send,
                        CCDIService_SharedFilesUnshareFile_recv,
                        CCDIService_SharedFilesUnshareFile_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SharedFilesDeleteSharedFile
// ---------------------------------------
static void CCDIService_SharedFilesDeleteSharedFile_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SharedFilesDeleteSharedFileCallback callback =
            (ccd::CCDIServiceClientAsync::SharedFilesDeleteSharedFileCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SharedFilesDeleteSharedFileInput, ccd::NoParamResponse>
        SharedFilesDeleteSharedFileAsyncState;

static bool CCDIService_SharedFilesDeleteSharedFile_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesDeleteSharedFileAsyncState* state = (SharedFilesDeleteSharedFileAsyncState*)arg;
    try {
        static const string method("SharedFilesDeleteSharedFile");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SharedFilesDeleteSharedFile_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesDeleteSharedFileAsyncState* state = (SharedFilesDeleteSharedFileAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SharedFilesDeleteSharedFileAsync(const ccd::SharedFilesDeleteSharedFileInput& request,
        SharedFilesDeleteSharedFileCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SharedFilesDeleteSharedFileAsyncState* asyncState =
                new SharedFilesDeleteSharedFileAsyncState(requestId, this, request,
                        CCDIService_SharedFilesDeleteSharedFile_send,
                        CCDIService_SharedFilesDeleteSharedFile_recv,
                        CCDIService_SharedFilesDeleteSharedFile_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// SharedFilesQuery
// ---------------------------------------
static void CCDIService_SharedFilesQuery_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::SharedFilesQueryCallback callback =
            (ccd::CCDIServiceClientAsync::SharedFilesQueryCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::SharedFilesQueryOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::SharedFilesQueryInput, ccd::SharedFilesQueryOutput>
        SharedFilesQueryAsyncState;

static bool CCDIService_SharedFilesQuery_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesQueryAsyncState* state = (SharedFilesQueryAsyncState*)arg;
    try {
        static const string method("SharedFilesQuery");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_SharedFilesQuery_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    SharedFilesQueryAsyncState* state = (SharedFilesQueryAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::SharedFilesQueryAsync(const ccd::SharedFilesQueryInput& request,
        SharedFilesQueryCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        SharedFilesQueryAsyncState* asyncState =
                new SharedFilesQueryAsyncState(requestId, this, request,
                        CCDIService_SharedFilesQuery_send,
                        CCDIService_SharedFilesQuery_recv,
                        CCDIService_SharedFilesQuery_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// RegisterRemoteExecutable
// ---------------------------------------
static void CCDIService_RegisterRemoteExecutable_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::RegisterRemoteExecutableCallback callback =
            (ccd::CCDIServiceClientAsync::RegisterRemoteExecutableCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::RegisterRemoteExecutableInput, ccd::NoParamResponse>
        RegisterRemoteExecutableAsyncState;

static bool CCDIService_RegisterRemoteExecutable_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RegisterRemoteExecutableAsyncState* state = (RegisterRemoteExecutableAsyncState*)arg;
    try {
        static const string method("RegisterRemoteExecutable");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_RegisterRemoteExecutable_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    RegisterRemoteExecutableAsyncState* state = (RegisterRemoteExecutableAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::RegisterRemoteExecutableAsync(const ccd::RegisterRemoteExecutableInput& request,
        RegisterRemoteExecutableCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        RegisterRemoteExecutableAsyncState* asyncState =
                new RegisterRemoteExecutableAsyncState(requestId, this, request,
                        CCDIService_RegisterRemoteExecutable_send,
                        CCDIService_RegisterRemoteExecutable_recv,
                        CCDIService_RegisterRemoteExecutable_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// UnregisterRemoteExecutable
// ---------------------------------------
static void CCDIService_UnregisterRemoteExecutable_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::UnregisterRemoteExecutableCallback callback =
            (ccd::CCDIServiceClientAsync::UnregisterRemoteExecutableCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::NoParamResponse&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::UnregisterRemoteExecutableInput, ccd::NoParamResponse>
        UnregisterRemoteExecutableAsyncState;

static bool CCDIService_UnregisterRemoteExecutable_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UnregisterRemoteExecutableAsyncState* state = (UnregisterRemoteExecutableAsyncState*)arg;
    try {
        static const string method("UnregisterRemoteExecutable");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_UnregisterRemoteExecutable_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    UnregisterRemoteExecutableAsyncState* state = (UnregisterRemoteExecutableAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::UnregisterRemoteExecutableAsync(const ccd::UnregisterRemoteExecutableInput& request,
        UnregisterRemoteExecutableCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        UnregisterRemoteExecutableAsyncState* asyncState =
                new UnregisterRemoteExecutableAsyncState(requestId, this, request,
                        CCDIService_UnregisterRemoteExecutable_send,
                        CCDIService_UnregisterRemoteExecutable_recv,
                        CCDIService_UnregisterRemoteExecutable_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

// ---------------------------------------
// ListRegisteredRemoteExecutables
// ---------------------------------------
static void CCDIService_ListRegisteredRemoteExecutables_deliverFunc(
        proto_rpc::AsyncCallState* resultObj)
{
    ccd::CCDIServiceClientAsync::ListRegisteredRemoteExecutablesCallback callback =
            (ccd::CCDIServiceClientAsync::ListRegisteredRemoteExecutablesCallback)(resultObj->actualCallback);
    callback(resultObj->requestId, resultObj->actualCallbackParam,
            (ccd::ListRegisteredRemoteExecutablesOutput&)(resultObj->getResponse()), resultObj->status);
}

typedef proto_rpc::AsyncCallStateImpl<ccd::ListRegisteredRemoteExecutablesInput, ccd::ListRegisteredRemoteExecutablesOutput>
        ListRegisteredRemoteExecutablesAsyncState;

static bool CCDIService_ListRegisteredRemoteExecutables_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListRegisteredRemoteExecutablesAsyncState* state = (ListRegisteredRemoteExecutablesAsyncState*)arg;
    try {
        static const string method("ListRegisteredRemoteExecutables");
        client.sendRpcRequest(method, state->getRequest(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
        return false;
    }
    return true;
}

static void CCDIService_ListRegisteredRemoteExecutables_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)
{
    ListRegisteredRemoteExecutablesAsyncState* state = (ListRegisteredRemoteExecutablesAsyncState*)arg;
    try {
        client.recvRpcResponse(state->getResponse(), state->status);
    } catch (...) {
        state->status.set_status(RpcStatus::INTERNAL_ERROR);
    }
}

int
ccd::CCDIServiceClientAsync::ListRegisteredRemoteExecutablesAsync(const ccd::ListRegisteredRemoteExecutablesInput& request,
        ListRegisteredRemoteExecutablesCallback callback, void* callbackParam)
{
    int requestId = _serviceState.nextRequestId();
    if (requestId >= 0) {
        ListRegisteredRemoteExecutablesAsyncState* asyncState =
                new ListRegisteredRemoteExecutablesAsyncState(requestId, this, request,
                        CCDIService_ListRegisteredRemoteExecutables_send,
                        CCDIService_ListRegisteredRemoteExecutables_recv,
                        CCDIService_ListRegisteredRemoteExecutables_deliverFunc,
                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);
        int rv = _serviceState.enqueueRequest(asyncState);
        if (rv < 0) {
            requestId = rv;
            delete asyncState;
        }
    }
    return requestId;
}

} // namespace ccd

