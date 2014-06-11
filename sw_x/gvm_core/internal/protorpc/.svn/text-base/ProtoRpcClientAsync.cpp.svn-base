//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "ProtoRpcClientAsync.h"
#include <vplex_trace.h>
#include <vpl_th.h>
#include <stdio.h>
#ifdef ANDROID
# include <asm/page.h> // pthread.h on Android refers to PAGE_SIZE but doesn't include it!
#endif

#define LOG_PROTORPC_ERR(...) \
    VPLTRACE_LOG_ERR(VPLTRACE_GRP_LIB, VPL_SG_LIB_PROTORPC, __VA_ARGS__)

static const int FIRST_REQUEST_ID = 100;
static const int REQUEST_ID_LIMIT = INT_MAX; // Can temporarily set this lower to test wrap-around case.

namespace proto_rpc {

#define CLIENT_THREAD_STACK_SIZE  (8 * 1024)

static int spawnThread(VPLDetachableThread_fn_t threadFunc, void* threadArg)
{
    int rv;
    VPLThread_attr_t threadAttrs;
    rv = VPLThread_AttrInit(&threadAttrs);
    if (rv < 0) {
        goto out;
    }
    rv = VPLThread_AttrSetStackSize(&threadAttrs, VPLTHREAD_STACKSIZE_MIN + CLIENT_THREAD_STACK_SIZE);
    if (rv < 0) {
        goto out2;
    }
    rv = VPLThread_AttrSetDetachState(&threadAttrs, VPL_TRUE);
    if (rv < 0) {
        goto out2;
    }
    rv = VPLDetachableThread_Create(NULL, threadFunc, threadArg, &threadAttrs, "ProtoRpcClient");
out2:
    VPLThread_AttrDestroy(&threadAttrs);
out:
    return rv;
}

static void lockMutex(VPLMutex_t* mutex)
{
    int rv = VPLMutex_Lock(mutex);
    if (rv < 0) {
        LOG_PROTORPC_ERR("VPLMutex_Lock returned %d", rv);
    }
}
static void unlockMutex(VPLMutex_t* mutex)
{
    int rv = VPLMutex_Unlock(mutex);
    if (rv < 0) {
        LOG_PROTORPC_ERR("VPLMutex_Unlock returned %d", rv);
    }
}
static void semPost(VPLSem_t* sem)
{
    int rv = VPLSem_Post(sem);
    if (rv < 0) {
        LOG_PROTORPC_ERR("VPLSem_Post returned %d", rv);
    }
}

//--------------------------------------------

class MyQueue
{
public:
    MyQueue() :
        head(NULL),
        tail(NULL)
    {
        int rv;
        rv = VPLMutex_Init(&queueMutex);
        if (rv < 0) {
#if VPL_CPP_ENABLE_EXCEPTIONS
            throw "failed to init mutex";
#endif
        }
        rv = VPLSem_Init(&queueSem, VPLSEM_MAX_COUNT, 0);
        if (rv < 0) {
#if VPL_CPP_ENABLE_EXCEPTIONS
            throw "failed to init semaphore";
#endif
        }
    }

    ~MyQueue()
    {
        (void)VPLMutex_Destroy(&queueMutex);
        (void)VPLSem_Destroy(&queueSem);
    }

    void enqueue(AsyncCallState* state);
    AsyncCallState* dequeue(bool blockIfEmpty);

private:
    VPLMutex_t queueMutex;
    VPLSem_t queueSem;
    AsyncCallState* head;
    AsyncCallState* tail;
};

void MyQueue::enqueue(AsyncCallState* state) {
    state->nextInQueue = NULL;
    lockMutex(&queueMutex);
    if (tail == NULL) {
        head = state;
    } else {
        tail->nextInQueue = state;
    }
    tail = state;
    unlockMutex(&queueMutex);
    semPost(&queueSem);
}

AsyncCallState* MyQueue::dequeue(bool blockIfEmpty)
{
    int sem_rv;
    if (blockIfEmpty) {
        sem_rv = VPLSem_Wait(&queueSem);
    } else {
        sem_rv = VPLSem_TryWait(&queueSem);
    }
    if (sem_rv == 0) {
        AsyncCallState* result;
        lockMutex(&queueMutex);
        if (head == NULL) {
            result = NULL;
        } else {
            result = head;
            head = result->nextInQueue;
            if (head == NULL) {
                tail = NULL;
            }
            result->nextInQueue = NULL;
        }
        unlockMutex(&queueMutex);
        return result;
    } else {
        return NULL;
    }
}

//--------------------------------------------

class AsyncServiceClientStateImpl
{
public:
    AsyncServiceClientStateImpl() :
        _nextRequestId(FIRST_REQUEST_ID)
    {
        int rv;
        rv = VPLMutex_Init(&requestIdMutex);
        if (rv < 0) {
#if VPL_CPP_ENABLE_EXCEPTIONS
            throw "failed to init mutex";
#endif
        }
        rv = VPLMutex_Init(&sendRequestMutex);
        if (rv < 0) {
#if VPL_CPP_ENABLE_EXCEPTIONS
            throw "failed to init mutex";
#endif
        }
    }

    ~AsyncServiceClientStateImpl()
    {
        (void)VPLMutex_Destroy(&requestIdMutex);
        (void)VPLMutex_Destroy(&sendRequestMutex);
    }

    /// Request queue (waiting to be sent to RPC server).
    MyQueue requests;

    /// Result queue (waiting to be processed by consumer of the RPC client).
    MyQueue results;

    VPLMutex_t requestIdMutex;
    int _nextRequestId;

    /// Lock to prevent multiple threads from sending request messages concurrently.
    VPLMutex_t sendRequestMutex;

    static VPLTHREAD_FN_DECL doRpc(void* arg);
};

AsyncServiceClientState::AsyncServiceClientState() :
        pImpl(new AsyncServiceClientStateImpl()) {}

AsyncServiceClientState::~AsyncServiceClientState()
{
    delete pImpl;
}

int AsyncServiceClientState::nextRequestId()
{
    int result;
    lockMutex(&pImpl->requestIdMutex);
    result = pImpl->_nextRequestId;
    if (pImpl->_nextRequestId == REQUEST_ID_LIMIT) {
        pImpl->_nextRequestId = FIRST_REQUEST_ID;
    } else {
        pImpl->_nextRequestId++;
    }
    unlockMutex(&pImpl->requestIdMutex);
    return result;
}

VPLTHREAD_FN_DECL AsyncServiceClientStateImpl::doRpc(void* arg)
{
    AsyncServiceClientState* clientState = (AsyncServiceClientState*)arg;
    // Lock sendRequestMutex before dequeuing, to ensure that we send the
    // request messages in the same order that they were enqueued.
    lockMutex(&clientState->pImpl->sendRequestMutex);
    AsyncCallState* callState = clientState->pImpl->requests.dequeue(true);
    if (callState == NULL) {
        unlockMutex(&clientState->pImpl->sendRequestMutex);
        // TODO: assert failed!
    } else {
        ProtoRpcClient* tempClient = callState->asyncClient->_acquireClientCallback();
        if (tempClient == NULL) {
            unlockMutex(&clientState->pImpl->sendRequestMutex);
            callState->status.set_status(RpcStatus_Status_INTERNAL_ERROR);
            callState->status.set_errordetail("Failed to acquire ProtoRpcClient");
        } else {
            // Must call this with sendRequestMutex held.
            bool sendSuccess = callState->sendRpcRequest(*tempClient);
            // Request message sent; we can release the lock now.
            unlockMutex(&clientState->pImpl->sendRequestMutex);
            if (sendSuccess) {
                callState->recvRpcResponse(*tempClient);
            }
            callState->asyncClient->_releaseClientCallback(tempClient);
        }
        clientState->pImpl->results.enqueue(callState);
    }
    return VPLTHREAD_RETURN_VALUE;
}

int AsyncServiceClientState::enqueueRequest(AsyncCallState* state)
{
    int rv = spawnThread(AsyncServiceClientStateImpl::doRpc, this);
    if (rv == 0) {
        pImpl->requests.enqueue(state);
    }
    return rv;
}

int AsyncServiceClientState::processResult()
{
    AsyncCallState* toProcess = pImpl->results.dequeue(false);
    if (toProcess == NULL) {
        // TODO: error code for no more
        return -1;
    } else {
        toProcess->callbackWrapper(toProcess);
        delete toProcess;
        return 0;
    }
}

} // end namespace proto_rpc
