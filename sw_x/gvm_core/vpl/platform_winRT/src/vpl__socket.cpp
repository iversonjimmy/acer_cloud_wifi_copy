/*
*                Copyright (C) 2008, BroadOn Communications Corp.
*
*   These coded instructions, statements, and computer programs contain
*   unpublished proprietary information of BroadOn Communications Corp.,
*   and are protected by Federal copyright law. They may not be disclosed
*   to third parties or copied or duplicated in any form, in whole or in
*   part, without the prior written consent of BroadOn Communications Corp.
*
*/

#include "vpl_socket.h"
#include "vpl__socket_priv.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"
#include "vplu.h"
#include "log.h"

#include <string>
#include <process.h>
#include <ppltasks.h>

#include "vpl_error.h"
#include "vpl_types.h"
#include "vpl_time.h"
#include "vpl_th.h" /// for VPLThread_Yield

#include <assert.h>

using namespace Windows::Security::Cryptography;
using namespace concurrency;

static VPLLazyInitMutex_t s_mutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitMutex_t s_pollpoolmutex = VPLLAZYINITMUTEX_INIT;
std::map<int,_SocketData^> m_SocketPool;
std::map<int,HANDLE> s_PollingPool;
static int s_pollkeygen = 0;
static int s_socketkeygen = 0;

//--------------------------------------------------------------------------
// Internal use functions

int exceptionHandling(HRESULT hr)
{
    SocketErrorStatus err_code = SocketError::GetStatus(hr);
    LOG_WARN("exception with HResult 0x%X", hr);

    int rv = VPLError_XlatWinRTSockErrno(static_cast<DWORD>(err_code));
    if (VPL_ERR_FAIL != rv) {
        return rv;
    }
    rv = VPLError_XlatHResult(hr);
    return rv;
}


int AddPollingJob(HANDLE completeHandle)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_pollpoolmutex));
    //bug 2353: Change PollingPool key generate rule to avoid random key duplicate, which might cause unable release of VPLSocket_Poll
    int ret = 0;
    //gen polling key
    if( s_PollingPool.size() > 0) {
        do{
            s_pollkeygen++;
            if(s_pollkeygen == INT_MAX)
                s_pollkeygen = 0;
            ret = s_pollkeygen;
        }
        while( s_PollingPool.count(ret) > 0 );
    }
    //put to map
    s_PollingPool.insert( std::pair<int,HANDLE>(ret,completeHandle) );
    return ret;
}

bool IsPollingHandleAlive(int pollingKey)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_pollpoolmutex));
    bool ret = false;
    if( s_PollingPool.size() > 0) {
        ret = s_PollingPool.count(pollingKey) > 0;
    }
    return ret;
}

void SetPollEvent(int pollingKey)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_pollpoolmutex));
    if( s_PollingPool.size() > 0) {
        if( s_PollingPool.count(pollingKey) > 0 ) {
            SetEvent( s_PollingPool.find(pollingKey)->second );
        }
    }
}

void RemovePollingJob(int pollingKey)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_pollpoolmutex));
    if( s_PollingPool.size() > 0) {
        if( s_PollingPool.count(pollingKey) > 0 ) {
            CloseHandle(s_PollingPool.find(pollingKey)->second);
            s_PollingPool.erase(pollingKey);
        }
    }
}

_SocketData^ GetSocketData(VPLSocket_t socket)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    _SocketData^ ret = nullptr;
    if( m_SocketPool.count(socket.s) > 0 ) {
        ret = m_SocketPool[socket.s];
    }
    return ret;
}

int PushSocketData(VPLSocket_t socket, _SocketData^ data)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    //bug 2353: Change PollingPool key generate rule to avoid random key duplicate, which might cause unable release of VPLSocket_Poll
    int ret = 0;
    if( m_SocketPool.size() > 0) {
        do{
            s_socketkeygen++;
            if(s_socketkeygen == INT_MAX)
                s_socketkeygen = 0;
            ret = s_socketkeygen;
        }
        while( m_SocketPool.count(ret) > 0 );
    }
    data->m_socketKey = ret;
    m_SocketPool.insert( std::pair<int,_SocketData^>( ret,data) );
    return ret;
}

//--------------------------------------------------------------------------
//_SocketData implement

String^ _SocketData::AddrToString(VPLNet_addr_t addr)
{
    String^ ret = L"";

    uint8_t a4 = (addr & 0xFF000000) >> 24;
    uint8_t a3 = (addr & 0xFF0000) >> 16;
    uint8_t a2 = (addr & 0xFF00) >> 8;
    uint8_t a1 = addr & 0xFF;

    ret = a1.ToString() + "." + 
        a2.ToString() + "." + 
        a3.ToString() + "." + 
        a4.ToString();

    return ret;
}

VPLNet_addr_t _SocketData::StringToAddr(String^ addr)
{
    VPLNet_addr_t ret = 0;

    std::wstring inaddr(addr->Data());

    std::vector<std::wstring> addrlist;

    int pos = inaddr.find(L".");
    while( pos != -1 ) {
        addrlist.push_back( inaddr.substr(0,pos));
        inaddr = inaddr.substr(pos+1, inaddr.length()-(pos+1));
        pos = inaddr.find(L".");
    }
    addrlist.push_back(inaddr);

    if(addrlist.size() == 4) {
        ret = _wtoi(addrlist.at(3).c_str()) * 0x1000000  +
            _wtoi(addrlist.at(2).c_str()) * 0x10000  +
            _wtoi(addrlist.at(1).c_str()) * 0x100 +
            _wtoi(addrlist.at(0).c_str());
    }

    return ret;
}

_SocketData::_SocketData(int type, bool isBlocking)
    :m_type(type),
    m_socket(nullptr),
    m_IsWrite(false),
    m_IsExcep(false),
    m_IsPollReading(false),
    m_IsPollWriting(false),
    m_IsClosed(false),
    m_IsBlocking(isBlocking),
    m_stage(SOCKET_STAGE_CREATE),
    m_hAcceptHandle(NULL),
    m_hUDPRecvHandle(NULL),
    m_localAddr(VPLNET_ADDR_ANY),
    m_MaxPandingClient(-1),
    m_LoadToBufferAction(nullptr),
    m_TotalBufferSize(0)
{
    ClearSocketOpt();
    ClearRecvFromAddr();

    VPLMutex_Init(&m_acceptMutex);
    VPLMutex_Init(&m_wFlagMutex);
	VPLMutex_Init(&m_wpFlagMutex);
	VPLMutex_Init(&m_eFlagMutex);
	VPLMutex_Init(&m_rpFlagMutex);
	VPLMutex_Init(&m_cFlagMutex);
    VPLMutex_Init(&m_dataBufferMutex);
    VPLMutex_Init(&m_connectedSocketsMutex);
    VPLMutex_Init(&m_pollingMutex);
    VPLMutex_Init(&m_sendMutex);

    //if type is UDP -> craete socket directly
    if( m_type == VPLSOCKET_DGRAM) {
        m_socket = ref new DatagramSocket();
        m_hUDPRecvHandle = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        ((DatagramSocket^)m_socket)->MessageReceived += 
            ref new TypedEventHandler<DatagramSocket^ , DatagramSocketMessageReceivedEventArgs^>(this, &_SocketData::OnMessageReceived);
        this->SetIsWrite(true);
    }
}

void _SocketData::OnConnection(StreamSocketListener^ listener, StreamSocketListenerConnectionReceivedEventArgs^ object) 
{
    //if we have connection limit & accepted queue has reached the limit, we close the connection
    MutexAutoLock lock(&m_connectedSocketsMutex);
    if( m_MaxPandingClient > 0 && m_ConnectedSockets.size() == m_MaxPandingClient ) {
        StreamSocket^ newClient = object->Socket;
        delete newClient;
    }
    else {
        //store connected socket
        m_ConnectedSockets.push(object->Socket);
        //get local IP address from client socket (StreamSocket)
        if(object->Socket == nullptr) {
            //bug 13335: avoid accessing nullptr
        }
        else {
            m_localAddr = _SocketData::StringToAddr(object->Socket->Information->LocalAddress->RawName);
        }
        //for Listener, it should be select by read set if there is any pending connection
    }

    //if blocking VPLSocket_Accept() -> release it 
    VPLMutex_Lock(&m_acceptMutex);
    if( m_hAcceptHandle != NULL ) {
        SetEvent(m_hAcceptHandle);
    }
    VPLMutex_Unlock(&m_acceptMutex);

    if( IsPollReading() ) {
        SetIsPollReading(false);
        SetPollingEvents(VPLSOCKET_POLL_RDNORM);
    }
}

void _SocketData::OnMessageReceived(DatagramSocket^ socket, DatagramSocketMessageReceivedEventArgs^ e)
{
    try {
        unsigned int availableBytes = e->GetDataReader()->UnconsumedBufferLength;
        if( availableBytes <= 0) {
            if( IsPollReading() ) {
                SetPollingEvents(VPLSOCKET_POLL_HUP);
            }
            goto end;
        }

        _SocketBuffer buffer;
        buffer.totalBufferLength = availableBytes;
        buffer.unconsumedBufferLength = availableBytes;
        buffer.buf = (unsigned char*)malloc(availableBytes);
        buffer.sourceAddr.addr = _SocketData::StringToAddr(e->RemoteAddress->RawName);
        buffer.sourceAddr.port = VPLNet_port_hton(_wtoi(e->RemotePort->Data()));
        //read data from stream 
        for(int i=0 ; i < availableBytes ; i++) {
            buffer.buf[i] = e->GetDataReader()->ReadByte();
        }
        //put to buffer queue
        VPLMutex_Lock(&m_dataBufferMutex);
        m_BufferQueue.push(buffer);
        int bufSize = m_BufferQueue.size();
        VPLMutex_Unlock(&m_dataBufferMutex);
        m_TotalBufferSize += buffer.totalBufferLength;
        if( IsPollReading() ) {
            SetIsPollReading(false);
            SetPollingEvents(VPLSOCKET_POLL_RDNORM);
        }
    }
    catch (Exception^ exception) {
        int rv = exceptionHandling(exception->HResult);
    }

end:
    if( m_hUDPRecvHandle != NULL) 
        SetEvent(m_hUDPRecvHandle);

}

bool _SocketData::IsWrite()
{
    return m_IsWrite;
}

bool _SocketData::IsExcep()
{
    return m_IsExcep;
}

bool _SocketData::IsPollReading()
{
    return m_IsPollReading;
}

bool _SocketData::IsPollWriting()
{
    return m_IsPollWriting;
}

bool _SocketData::IsBlocking() 
{
    return m_IsBlocking;
}

bool _SocketData::IsClosed() 
{
    return m_IsClosed;
}

void _SocketData::SetIsWrite(bool value)
{
    // Bug 5857: Using same mutex for all flags causes value setting interference,
    //           so give each flag a mutex.
    MutexAutoLock lock(&m_wFlagMutex);
    m_IsWrite = value;
}

void _SocketData::SetIsExcep(bool value)
{
    // Bug 5857: Using same mutex for all flags causes value setting interference,
    //           so give each flag a mutex.
    VPLMutex_Lock(&m_eFlagMutex);
    m_IsExcep = value;
    VPLMutex_Unlock(&m_eFlagMutex);
}

void _SocketData::SetIsPollReading(bool value)
{
    // Bug 5857: Using same mutex for all flags causes value setting interference,
    //           so give each flag a mutex.
    VPLMutex_Lock(&m_rpFlagMutex);
    m_IsPollReading = value;
    VPLMutex_Unlock(&m_rpFlagMutex);
}

void _SocketData::SetIsPollWriting(bool value)
{
    // Bug 5857: Using same mutex for all flags causes value setting interference,
    //           so give each flag a mutex.
    VPLMutex_Lock(&m_wpFlagMutex);
    m_IsPollWriting = value;
    VPLMutex_Unlock(&m_wpFlagMutex);
}

void _SocketData::SetIsClosed(bool value)
{
    // Bug 5857: Using same mutex for all flags causes value setting interference,
    //           so give each flag a mutex.
    VPLMutex_Lock(&m_cFlagMutex);
    m_IsClosed = value;
    VPLMutex_Unlock(&m_cFlagMutex);
}

void _SocketData::ResetPollFlags()
{
    SetIsWrite(false);
    SetIsExcep(false);
}

void _SocketData::CreateTCPListener()
{
    if( m_socket == nullptr) {
        m_socket = ref new StreamSocketListener();
        ((StreamSocketListener^)m_socket)->ConnectionReceived += 
            ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>(this, &_SocketData::OnConnection);
    }

    m_type = VPLSOCKET_STREAMLISTENER;
}

void _SocketData::CreateTCPClient()
{
    if( m_socket == nullptr)
        m_socket = ref new StreamSocket();

    if(m_SocketOpt.name == VPLSOCKET_TCP_NODELAY) {
        try{
            ((StreamSocket^)m_socket)->Control->NoDelay = (bool)m_SocketOpt.value;
        }
        catch (Exception^ exception) {
            int rv = exceptionHandling(exception->HResult);
        }
    }

    m_type = VPLSOCKET_STREAM;
}

void _SocketData::AcceptTCPClient(StreamSocket^ clientSock)
{
    m_socket = (Object^)clientSock;
    m_type = VPLSOCKET_STREAM;
    m_stage = SOCKET_STAGE_CONNECT;
    //Set writable for it is a client socket accept by listener 
    SetIsWrite(true);
}

int _SocketData::SetSocketOpt(int optName, int value)
{
    int rv = VPL_OK;

    if( optName != VPLSOCKET_TCP_NODELAY && optName != VPLSOCKET_TCP_SECURE)  {
        return VPL_OK;
    }

    if(m_socket == nullptr) {
        this->m_SocketOpt.name = optName;
        this->m_SocketOpt.value = value;
    }
    else {
        if( optName == VPLSOCKET_TCP_NODELAY ) {
            try{
                switch(this->m_type) {
                case VPLSOCKET_STREAM:
                    ((StreamSocket^)m_socket)->Control->NoDelay = (bool)value;
                    break;
                // Other Type of WinRT socket does NOT support TCP_NODELAY option, but for CCD logic, which WILL need other sockets
                // set to TCP_NODELAY, here we treat as VPL_OK and do no-op.
                case VPLSOCKET_DGRAM:
                case VPLSOCKET_STREAMLISTENER:
                default:
                    break;
                }
            }
            catch (Exception^ exception) {
                //StreamSocket throws exception 0x8000000e (A method was called at an unexpected time) if set Control option after connected
                if( exception->HResult == 0x8000000e ) {
                    rv = VPL_ERR_ISCONN;
                } else {
                    rv = exceptionHandling(exception->HResult);
                }
            }
        }
        else {
            // Currently not support other socket option on WinRT
            rv = VPL_ERR_OPNOTSUPPORTED;
        }
    }

    return rv;
}

int _SocketData::DoBind(String^ addr, String^ port)
{
    char* msgA = NULL;
    _VPL__wstring_to_utf8_alloc(addr->Data(),&msgA);
    LOG_TRACE("DoBind on socket: %d, to addr: %s",this->m_socketKey,msgA);
    if(msgA != NULL) free(msgA);

    int rv = VPL_OK;
    IAsyncAction^ bindAction = nullptr;

    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        LOG_TRACE("DoBind on socket: %d, CreateEventEx failed",this->m_socketKey);
        rv = VPL_ERR_FAIL;
        goto end;
    }

    if(m_type == VPLSOCKET_STREAMLISTENER) { //TCP listener
        if(addr->IsEmpty())
            bindAction = ((StreamSocketListener^)m_socket)->BindServiceNameAsync(port);
        else
            bindAction = ((StreamSocketListener^)m_socket)->BindEndpointAsync( ref new HostName(addr), port);

        if(bindAction->Status == AsyncStatus::Error) {
            LOG_TRACE("DoBind on socket: %d, AsyncStatus::Error",this->m_socketKey);
            rv = VPL_ERR_FAIL;
            goto end;
        }
    }
    else if (m_type == VPLSOCKET_DGRAM) { //UDP receiver
        if(addr->IsEmpty())
            bindAction = ((DatagramSocket^)m_socket)->BindServiceNameAsync(port);
        else
            bindAction = ((DatagramSocket^)m_socket)->BindEndpointAsync( ref new HostName(addr), port);

        if(bindAction->Status == AsyncStatus::Error) {
            LOG_TRACE("DoBind on socket: %d, AsyncStatus::Error",this->m_socketKey);
            rv =  VPL_ERR_FAIL;
            goto end;
        }
    }
    else {
        LOG_TRACE("DoBind on socket: %d, not support bind on this socket (StreamSocket)",this->m_socketKey);
        rv = VPL_ERR_OPNOTSUPPORTED;
        goto end;
    }

    {
        bindAction->Completed = ref new AsyncActionCompletedHandler(
            [this,&rv,&completedEvent] (IAsyncAction^ op, AsyncStatus statue) {
                try {
                    op->GetResults();
                    rv = VPL_OK;
                    LOG_TRACE("DoBind on socket: %d, finished",this->m_socketKey);
                }
                catch (Exception^ exception) {
                    rv = exceptionHandling(exception->HResult);
                    char* msgA = NULL;
                    _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
                    LOG_TRACE("DoBind on socket: %d, exception: %s",this->m_socketKey,msgA);
                    if(msgA != NULL) free(msgA);
                }
                SetEvent(completedEvent);
            }
        );
    
        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        CloseHandle(completedEvent);
    }

end:
    if( rv != VPL_OK) {
        SetIsExcep(true);
    }

    return rv;
}

int _SocketData::ConnectionQueueSize()
{
    int size = 0;
    VPLMutex_Lock(&m_connectedSocketsMutex);
    size = m_ConnectedSockets.size();
    VPLMutex_Unlock(&m_connectedSocketsMutex);
    return size;
}

void _SocketData::DoAccept()
{
    int size = ConnectionQueueSize();
    if( size == 0) {
        {
            //if no any connected socket in buffer, block any wait for connection
            MutexAutoLock lock(&m_acceptMutex);
            m_hAcceptHandle = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            ResetEvent(m_hAcceptHandle);
        }
        std::thread t(
            [this] {
                WaitForSingleObjectEx(m_hAcceptHandle ,INFINITE, TRUE);
            }
        );
        t.join();
        {
            MutexAutoLock lock(&m_acceptMutex);
            CloseHandle(m_hAcceptHandle);
            m_hAcceptHandle = NULL;
        }
    }
}

int _SocketData::DoConnect(Platform::String^ addr, Platform::String^ port, VPLTime_t timeout)
{
    char* msgA = NULL;
    _VPL__wstring_to_utf8_alloc(addr->Data(),&msgA);
    LOG_TRACE("DoConnect on socket: %d, to addr: %s",this->m_socketKey,msgA);
    if(msgA != NULL) free(msgA);

    int rv = VPL_OK;
    IAsyncAction^ connectAction = nullptr;
    try {
        if( m_type == VPLSOCKET_STREAM ) { //TCP connect
            if( m_SecureOpt.name == VPLSOCKET_TCP_SECURE && m_SecureOpt.value == TRUE)
                connectAction = ((StreamSocket^)m_socket)->ConnectAsync(ref new HostName(addr), port, SocketProtectionLevel::SslAllowNullEncryption);
            else
                connectAction = ((StreamSocket^)m_socket)->ConnectAsync(ref new HostName(addr), port, SocketProtectionLevel::PlainSocket);
        }
        else if(m_type == VPLSOCKET_DGRAM ) { //UDP connect
            connectAction = ((DatagramSocket^)m_socket)->ConnectAsync(ref new HostName(addr), port);
        }

        if(connectAction->Status == AsyncStatus::Error) {
            rv = VPL_ERR_FAIL;
            LOG_TRACE("DoConnect on socket: %d, AsyncStatus::Error",this->m_socketKey);
        }
    }
    catch (Exception^ exception) {
        if( exception->HResult == 0x8000000e) { //already connnect
            rv = VPL_ERR_ISCONN;
        } else {
            rv = exceptionHandling(exception->HResult);
        }
        char* msgA = NULL;
        _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
        LOG_TRACE("DoConnect on socket: %d, exception: %s",this->m_socketKey,msgA);
        if(msgA != NULL) free(msgA);
    }

    if( rv == VPL_OK ) {
        // 1. for timeout handle we need to create 2 events for waiting
        //    * One is for timeout
        //    * One is for Async API Completed callback
        HANDLE timeoutEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if( !timeoutEvent ) {
            LOG_TRACE("DoConnect on socket: %d, CreateEventEx failed",this->m_socketKey);
            return VPL_ERR_FAIL;
        }
        concurrency::event completedEvent;

        connectAction->Completed = ref new AsyncActionCompletedHandler(
            [this,connectAction,&rv,&completedEvent,&timeoutEvent] (IAsyncAction^ op, AsyncStatus statue) {
                try {
                    // Try getting all exceptions from the continuation chain above this point
                    op->GetResults();
                    rv = VPL_OK;
                    LOG_TRACE("DoConnect on socket: %d, finished",this->m_socketKey);
                }
                catch (Exception^ exception) {
                    rv = exceptionHandling(exception->HResult);
                    char* msgA = NULL;
                    _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
                    LOG_TRACE("DoConnect on socket: %d, exception: %s",this->m_socketKey,msgA);
                    if(msgA != NULL) free(msgA);
                }
                SetEvent(timeoutEvent);
                completedEvent.set();
            }
        );
        // 2. if WaitForSingleObjectEx() reach timeout, it will release lock before Completed callback
        WaitForSingleObjectEx(timeoutEvent , VPLTime_ToMillisec(timeout), TRUE);
        //handle timeout
        if(connectAction->Status == AsyncStatus::Started) {
            rv = VPL_ERR_TIMEOUT;
            try {
                connectAction->Cancel();
            }
            catch (Exception^ exception) {
                int rv = exceptionHandling(exception->HResult);
                String^ msg = exception->Message;
            }
        }
        // 3. to avoid closing event before Completed callback (Handle is set), 
        //    we use another event to wait until Completed callback finished
        completedEvent.wait();
        // 4. then we claose the timeout event
        CloseHandle(timeoutEvent);

        if( rv == VPL_OK) {
            m_stage = SOCKET_STAGE_CONNECT;
            SetIsWrite(true);
        }
        else {
            SetIsExcep(true);
        }

        //bug 11142: handle if socket polled before connect.
        this->SetPollingEvents(VPLSOCKET_POLL_OUT | VPLSOCKET_POLL_RDNORM);
    }

    return rv;
}

int _SocketData::DoConnectNowait(Platform::String^ addr, Platform::String^ port)
{
    int rv = VPL_ERR_BUSY;
    SetIsWrite(false);

    char* msgA = NULL;
    _VPL__wstring_to_utf8_alloc(addr->Data(),&msgA);
    LOG_TRACE("DoConnectNowait on socket: %d, to addr: %s",this->m_socketKey,msgA);
    if(msgA != NULL) free(msgA);

    IAsyncAction^ connectAction = nullptr;
    try {
        if( m_type == VPLSOCKET_STREAM ) { //TCP connect
            if( m_SecureOpt.name == VPLSOCKET_TCP_SECURE && m_SecureOpt.value == TRUE)
                connectAction = ((StreamSocket^)m_socket)->ConnectAsync(ref new HostName(addr), port, SocketProtectionLevel::SslAllowNullEncryption);
            else
                connectAction = ((StreamSocket^)m_socket)->ConnectAsync(ref new HostName(addr), port, SocketProtectionLevel::PlainSocket);
        }
        else if(m_type == VPLSOCKET_DGRAM ) { //UDP connect
            connectAction = ((DatagramSocket^)m_socket)->ConnectAsync(ref new HostName(addr), port);
        }

        if(connectAction->Status == AsyncStatus::Error) {
            LOG_TRACE("DoConnectNowait on socket: %d, AsyncStatus::Error",this->m_socketKey);
            rv = VPL_ERR_FAIL;
            goto end;
        }
    }
    catch (Exception^ exception) {
        if( exception->HResult == 0x8000000e) { //already connnect
            rv = VPL_ERR_ISCONN;
        } else {
            rv = exceptionHandling(exception->HResult);
        }

        char* msgA = NULL;
        _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
        LOG_TRACE("DoConnectNowait on socket: %d, exception: %s",this->m_socketKey,msgA);
        if(msgA != NULL) free(msgA);

        goto end;
    }

    if( !this->IsBlocking() ) {
        connectAction->Completed = ref new AsyncActionCompletedHandler(
            [this,connectAction,&rv] (IAsyncAction^ op, AsyncStatus statue) {
                try {
                    // Try getting all exceptions from the continuation chain above this point
                    op->GetResults();
                    rv = VPL_OK;
                    LOG_TRACE("DoConnectNowait on socket: %d, finished",this->m_socketKey);
                }
                catch (Exception^ exception) {
                    rv = exceptionHandling(exception->HResult);
                    char* msgA = NULL;
                    _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
                    LOG_TRACE("DoConnectNowait on socket: %d, exception: %s",this->m_socketKey,msgA);
                    if(msgA != NULL) free(msgA);
                }

                //bug 11142: handle if socket polled before connect.
                this->SetIsWrite(true);
                this->SetPollingEvents(VPLSOCKET_POLL_OUT | VPLSOCKET_POLL_RDNORM);
            }
        );
    }

end:
    return rv;
}

void _SocketData::InsertPollingKey(int pollingkey, uint16_t event)
{
    LOG_TRACE("add polling job for socket: %d, key: %d evnets:%d",this->m_socketKey, pollingkey, event);
    MutexAutoLock lock(&m_pollingMutex);
    m_pollingKeyEvent[pollingkey] = event;
}

void _SocketData::SetPollingEvents(uint16_t event)
{
    if (m_pollingKeyEvent.empty()) return;
    MutexAutoLock lock(&m_pollingMutex);
    std::map<int, uint16_t>::iterator it;
    if (event == VPLSOCKET_POLL_HUP) { //disconnected
        LOG_WARN("Socket:%d VPLSOCKET_POLL_HUP", this->m_socketKey);
        for (it = m_pollingKeyEvent.begin(); it != m_pollingKeyEvent.end(); ++it) {
            m_polledKeyRevent[it->first] = VPLSOCKET_POLL_HUP;
            SetPollEvent(it->first);
        }
        m_pollingKeyEvent.clear();
    } else {
        LOG_DEBUG("socket: %d, Event: %s %s",this->m_socketKey, (event & VPLSOCKET_POLL_OUT) ? "VPLSOCKET_POLL_OUT" : "", (event & VPLSOCKET_POLL_RDNORM) ? "VPLSOCKET_POLL_RDNORM" : "" );
        for (it = m_pollingKeyEvent.begin(); it != m_pollingKeyEvent.end() ; ) {
            if (it->second & event) {
                m_polledKeyRevent[it->first] = (it->second & event);
                LOG_DEBUG("key: %d Revent: %d", it->first, (it->second & event));
                SetPollEvent(it->first);
                std::map<int, uint16_t>::iterator tmp_it = it;
                ++it;
                m_pollingKeyEvent.erase(tmp_it);
            } else {
                ++it;
            }
        }
    }
}


void _SocketData::DoPoll(int pollingKey, uint16_t poll_events)
{
    InsertPollingKey(pollingKey, poll_events);

    /* Check for read set */
    if (poll_events & VPLSOCKET_POLL_RDNORM) {
        //read -> TCP listener
        if( m_type == VPLSOCKET_STREAMLISTENER ) {
            //for Listener, it should be select by read set if there is any pending connection
            MutexAutoLock lock(&m_connectedSocketsMutex);
            if( m_ConnectedSockets.size() > 0 ) {
                //selected -> release blocking
                SetPollingEvents(VPLSOCKET_POLL_RDNORM);
            }
            else {
                //do nothing, remain lock for polling
                SetIsPollReading(true);
            }
        }
        //read -> TCP Client
        else if ( m_type == VPLSOCKET_STREAM ) {
            //if there is data in buffer queue -> set read flag
            MutexAutoLock lock(&m_dataBufferMutex);
            if( m_BufferQueue.size() > 0 ) {
                LOG_TRACE("DoPoll on socket: %d, get from buffer",this->m_socketKey);
                //selected -> release blocking
                SetPollingEvents(VPLSOCKET_POLL_RDNORM);
            }
            else { //if buffer queue is empty -> load data to queue to check if there is any income data
                LOG_TRACE("DoPoll on socket: %d, buffer empty -> LoadToBuffer()",this->m_socketKey);
                LoadToBuffer();
            }
        }
        //read -> UPD client
        else {
            //if there is data in buffer queue -> set read flag
            MutexAutoLock lock(&m_dataBufferMutex);
            if( m_BufferQueue.size() > 0 ) {
                //selected -> release blocking
                SetPollingEvents(VPLSOCKET_POLL_RDNORM);
            }
            else { 
                //waiting for data coming to select
                SetIsPollReading(true);
            }
        }
    }

    // TODO: handle exceptfds (OOB)
    //if (poll_events & VPLSOCKET_POLL_RDPRI) {
    //}

    /* Check for write set */
    if (poll_events & VPLSOCKET_POLL_OUT) {
        // Bug 5857: set IsPollWriting to true first, avoid IsWrite flag not yet set by VPLSocket_Send() and cause polling waiting cannot be release.
        SetIsPollWriting(true);
        //write
        if( IsWrite() ) {
            LOG_TRACE("DoPoll on socket: %d, is writable",this->m_socketKey);
            //select -> release blocking
            SetPollingEvents(VPLSOCKET_POLL_OUT);
        }
        else {
            LOG_TRACE("DoPoll on socket: %d, NOT writable",this->m_socketKey);
        }
    }
}

uint16_t _SocketData::GetPollResult( int pollingKey, uint16_t poll_events)
{
    LOG_DEBUG("socket:%d m_bufferQ:%d m_Conn:%d", this->m_socketKey, m_BufferQueue.size(), m_ConnectedSockets.size());
    uint16_t revent = 0;
    static uint16_t VALID_EVENTS = VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_OUT | VPLSOCKET_POLL_RDPRI;
    std::map<int, uint16_t>::iterator it;

    MutexAutoLock lock(&m_pollingMutex);
    it = m_polledKeyRevent.find(pollingKey);
    if (it != m_polledKeyRevent.end()) {
        if (poll_events & ~VALID_EVENTS) {
            // directly return VPLSOCKET_POLL_EV_INVAL if poll_events is invalid value
            revent |= VPLSOCKET_POLL_EV_INVAL;
        } else if (it->second & ~VALID_EVENTS) {
            // return invalid events
            revent |= it->second;
        } else {
            // return events been polled
            revent |= (it->second & poll_events);
        }
        m_polledKeyRevent.erase(it);
    }

    return revent;
}

int _SocketData::SizeToReadFromBuffer()
{
    int size = 0;
    MutexAutoLock lock(&m_dataBufferMutex);
    size = m_BufferQueue.size();
    return size;
}

int _SocketData::LoadToBuffer(int recv_len)
{
    int rc = VPL_ERR_AGAIN;

    if( IsPollReading() ) {
        LOG_TRACE("LoadToBuffer on socket: %d, but last polling not finished, return VPL_ERR_AGAIN",this->m_socketKey);
        return rc;
    }
    SetIsPollReading(true);
    LOG_TRACE("LoadToBuffer on socket: %d, try load (%d) bytes",this->m_socketKey,recv_len);

    //initial istream 
    DataReader^ istream = ref new DataReader( ((StreamSocket^)m_socket)->InputStream );
    //
    // In default setting of DataReader (InputStreamOptions is None), the LoadAsync function callback will only be call when 
    // stream size received reaches to the argument ("wish to read" size in integer) caller passed to LoadAsync() function
    // To avoid this behavior, must set InputStreamOptions to InputStreamOptions::Partial
    //
    // Reference: http://msdn.microsoft.com/en-us/library/windows/apps/windows.storage.streams.inputstreamoptions
    //
    istream->InputStreamOptions = InputStreamOptions::Partial;

    try {
        m_LoadToBufferAction = istream->LoadAsync(recv_len);
    }
    catch (Exception^ exception) {
        if( exception->HResult == 0x80000013) {
            rc = VPL_ERR_CONNRESET;
        } else {
            rc = exceptionHandling(exception->HResult);
        }
        char* msgA = NULL;
        _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
        LOG_TRACE("LoadToBuffer on socket: %d, exception: %s",this->m_socketKey,msgA);
        if(msgA != NULL) free(msgA);
        
        goto failed;
    }

    if(m_LoadToBufferAction->Status == AsyncStatus::Error) {
        rc = VPL_ERR_FAIL;
        try{
            m_LoadToBufferAction->GetResults();
        }
        catch(Exception^ exception) {
            if( exception->HResult == 0x80000013) {
                rc = VPL_ERR_CONNRESET;
            } else {
                rc = exceptionHandling(exception->HResult);
            }

            char* msgA = NULL;
            _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
            LOG_TRACE("LoadToBuffer on socket: %d, exception: %s",this->m_socketKey,msgA);
            if(msgA != NULL) free(msgA);
        }
        goto failed;
    }

    {
        m_LoadToBufferAction->Completed = ref new AsyncOperationCompletedHandler<unsigned int>(
            [this,istream] (IAsyncOperation<unsigned int>^ op, AsyncStatus status) {
                unsigned int availableBytes = 0;
                try {
                    availableBytes = op->GetResults();
                }
                catch (Exception^ exception) {
                    int rv = exceptionHandling(exception->HResult);
                    char* msgA = NULL;
                    _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
                    LOG_TRACE("LoadToBuffer on socket: %d, exception: %s",this->m_socketKey,msgA);
                    if(msgA != NULL) free(msgA);

                    //set revent to POLL_HUP
                    //Detach istream to avoid underlying using broken
                    istream->DetachStream();
                    SetIsPollReading(false);
                    SetPollingEvents(VPLSOCKET_POLL_HUP);
                    return;
                }
                uint16_t pollingEvent;
                if( availableBytes < 0) {
                        // Load bytes < 0, socket is disconnected by remote peer abord by local host
                        pollingEvent = VPLSOCKET_POLL_HUP;
                        LOG_TRACE("LoadToBuffer on socket: %d, availableBytes < 0, state = %d",this->m_socketKey,m_LoadToBufferAction->Status);
                }
                else if( availableBytes == 0) {
                    //According to MSFT response, if availableBytes is 0 & LoasAsync Status = Completed
                    //it means the socket is closed
                    if( m_LoadToBufferAction->Status == AsyncStatus::Completed) {
                        // Load 0 bytes, socket is close gracefully
                        pollingEvent = VPLSOCKET_POLL_HUP;
                        SetIsExcep(true);
                        SetIsClosed(true);
                    }
                    LOG_TRACE("LoadToBuffer on socket: %d, availableBytes = 0, state = %d",this->m_socketKey,m_LoadToBufferAction->Status);
                }
                else {
                    _SocketBuffer buffer;
                    buffer.totalBufferLength = availableBytes;
                    buffer.unconsumedBufferLength = availableBytes;
                    buffer.buf = (unsigned char*)malloc(availableBytes);
                    //read data from stream 
                    for(int i=0 ; i<availableBytes ; i++) {
                        buffer.buf[i] = istream->ReadByte();
                    }

                    //put to buffer queue
                    VPLMutex_Lock(&m_dataBufferMutex);
                    m_BufferQueue.push(buffer);
                    VPLMutex_Unlock(&m_dataBufferMutex);
                    m_TotalBufferSize += buffer.totalBufferLength;
                    LOG_TRACE("LoadToBuffer on socket: %d, finished receive (%d) bytes",this->m_socketKey,availableBytes);
                    pollingEvent = VPLSOCKET_POLL_RDNORM;
                }  

                //Detach istream to avoid underlying using broken
                istream->DetachStream();
                SetIsPollReading(false);
                SetPollingEvents(pollingEvent);
            }
        );

        return rc;
    }
failed:
    //Detach istream to avoid underlying using broken
    istream->DetachStream();
    SetIsPollReading(false);
    SetPollingEvents(VPLSOCKET_POLL_HUP);

    return rc;
}

int _SocketData::DoSend(const void* msg, int len)
{
    MutexAutoLock lock(&m_sendMutex);
    
    int rv = VPL_OK;
    if( !this->IsBlocking() && !IsWrite() ) {
        LOG_TRACE("DoSend on socket: %d, but last send not finished, return VPL_ERR_AGAIN",this->m_socketKey);
        rv = VPL_ERR_AGAIN;
        //LOG_TRACE("DoSend on socket: %d, VPL_ERR_AGAIN after unlock",this->m_socketKey);
        goto end;
    }
    LOG_TRACE("DoSend on socket: %d, try to send (%d) bytes",this->m_socketKey,len);

    //if writable -> send data async
    SetIsWrite(false);

    {
        //initial DataWriter
        DataWriter^ ostream = nullptr;
        if( m_type == VPLSOCKET_STREAM ) 
            ostream = ref new DataWriter( ((StreamSocket^)m_socket)->OutputStream);
        else if( m_type == VPLSOCKET_DGRAM ) 
            ostream = ref new DataWriter( ((DatagramSocket^)m_socket)->OutputStream);

        unsigned char* msgStr = (unsigned char*)msg;
        //write data to DataWriter
        for(int i=0 ; i<len ; i++) {
            ostream->WriteByte( msgStr[i] );
        }

        DataWriterStoreOperation^ storeOP = nullptr;
        try{
            storeOP = ostream->StoreAsync();
        }
        catch(Exception^ exception) {
            rv = exceptionHandling(exception->HResult);
            ostream->DetachStream();
            SetIsWrite(true);
            if( IsPollWriting() ) {
                SetPollingEvents(VPLSOCKET_POLL_HUP);
                SetIsPollWriting(false);
            }

            char* msgA = NULL;
            _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
            LOG_TRACE("DoSend on socket: %d, exception: %s",this->m_socketKey,msgA);
            if(msgA != NULL) free(msgA);

            //rv = VPL_ERR_FAIL;
            goto end;
        }

        if( storeOP->Status == AsyncStatus::Error ) {
            rv = VPL_ERR_FAIL;
            try{
                storeOP->GetResults();
            }
            catch(Exception^ exception) {
                rv = exceptionHandling(exception->HResult);
                char* msgA = NULL;
                _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
                LOG_TRACE("DoSend on socket: %d, exception: %s",this->m_socketKey,msgA);
                if(msgA != NULL) free(msgA);
            }
            ostream->DetachStream();
            SetIsWrite(true);
            if( IsPollWriting() ) {
                SetPollingEvents(VPLSOCKET_POLL_HUP);
                SetIsPollWriting(false);
            }
            LOG_TRACE("DoSend on socket: %d, exception: AsyncStatus::Error",this->m_socketKey);
            goto end;
        }

        HANDLE completedEvent = NULL;
        if( this->IsBlocking() ){
            completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if( !completedEvent ) {
                ostream->DetachStream();
                SetIsWrite(true);
                if( IsPollWriting() ) {
                    SetPollingEvents(VPLSOCKET_POLL_OUT);
                    SetIsPollWriting(false);
                }
                LOG_TRACE("DoSend on socket: %d, CreateEventEx failed",this->m_socketKey);
                rv = VPL_ERR_FAIL;
                goto end;
            }
        }

        storeOP->Completed = ref new AsyncOperationCompletedHandler<unsigned int>(
            [this, ostream,&rv,len,&completedEvent] (IAsyncOperation<unsigned int>^ storeOperation, AsyncStatus state) {

                unsigned int wrote = 0;
                try {
                    wrote = storeOperation->GetResults();
                    LOG_TRACE("DoSend on socket: %d, finished, wrote (%d) bytes out",this->m_socketKey,wrote);
                }
                catch(Exception^ exception) {
                    rv = exceptionHandling(exception->HResult);
                    char* msgA = NULL;
                    _VPL__wstring_to_utf8_alloc(exception->Message->Data(),&msgA);
                    LOG_TRACE("DoSend on socket: %d, exception: %s",this->m_socketKey,msgA);
                    if(msgA != NULL) free(msgA);
                }

                ostream->DetachStream();
                SetIsWrite(true);
                // release if polling
                if( IsPollWriting() ) {
                    SetPollingEvents(VPLSOCKET_POLL_OUT);
                    SetIsPollWriting(false);
                }

                if( IsBlocking() ) {
                    rv = wrote;
                    SetEvent(completedEvent);
                }
            }
        );

        if( IsBlocking() ) {
            WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
            CloseHandle(completedEvent);
            SetIsWrite(true);
        }
        else {
            rv = len;
        }
    }
end:
    return rv;
}

int _SocketData::DoReceive(void* buf, int len)
{
    LOG_TRACE("DoReceive on socket: %d, try to receive (%d) bytes",this->m_socketKey,len);

    int rv = VPL_OK;
    bool isBufEmpty = false;
    rv = GetBuffer((unsigned char*)buf,len,isBufEmpty);
    if(!isBufEmpty) {
        LOG_TRACE("DoReceive on socket: %d, get data from buffer (%d) bytes",this->m_socketKey,rv);
        goto end;
    }
    //if non-blocking socket & buffer empty - loadAsync & return VPL_ERR_AGAIN
    if( !IsBlocking() ) {
        if( m_type == VPLSOCKET_STREAM ) {
            LOG_TRACE("DoReceive on socket: %d, buffer empty -> LoadToBuffer",this->m_socketKey);
            rv = LoadToBuffer(len);
        }
        else if( m_type == VPLSOCKET_DGRAM ) {
            //do nothing, just return VPL_ERR_AGAIN
            rv = VPL_ERR_AGAIN;
        }
    }
    //if blocking socket & buffer empty -> loadAsync & block until data received
    else {
        if( m_type == VPLSOCKET_STREAM ) {
            HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if( !completedEvent ) {
                return 0;
            }

            DataReaderLoadOperation^ locaAction = nullptr;
            DataReader^ istream = ref new DataReader( ((StreamSocket^)m_socket)->InputStream );
            //
            // In default setting of DataReader (InputStreamOptions is None), the LoadAsync function callback will only be call when 
            // stream size received reaches to the argument ("wish to read" size in integer) caller passed to LoadAsync() function
            // To avoid this behavior, must set InputStreamOptions to InputStreamOptions::Partial
            //
            // Reference: http://msdn.microsoft.com/en-us/library/windows/apps/windows.storage.streams.inputstreamoptions
            //
            istream->InputStreamOptions = InputStreamOptions::Partial;
            try {
                locaAction = istream->LoadAsync(len);
            }
            catch (Exception^ exception) {
                //Detach istream to avoid underlying using broken
                istream->DetachStream();
                SetIsPollReading(false);
                if( exception->HResult == 0x80000013) {
                    rv = VPL_ERR_CONNRESET;
                } else {
                    rv = exceptionHandling(exception->HResult);
                }
                goto end;
            }

            if(locaAction->Status == AsyncStatus::Error) {
                rv = VPL_ERR_FAIL;
                try{
                    locaAction->GetResults();
                }
                catch(Exception^ exception) {
                    rv = exceptionHandling(exception->HResult);
                }
                //Detach istream to avoid underlying using broken
                istream->DetachStream();
                SetIsPollReading(false);
                return rv;
            }
           
            locaAction->Completed = ref new AsyncOperationCompletedHandler<unsigned int>(
                [this,istream,locaAction,&buf,len,&rv,&completedEvent] (IAsyncOperation<unsigned int>^ op, AsyncStatus status) {
                    unsigned int availableBytes = 0;
                    try {
                        availableBytes = op->GetResults();
                    }
                    catch (Exception^ exception) {
                        if( exception->HResult == 0x80000013) {
                            rv = VPL_ERR_CONNRESET;
                        } else {
                            rv = exceptionHandling(exception->HResult);
                        }
                        
                        //Detach istream to avoid underlying using broken
                        istream->DetachStream();
                        SetIsPollReading(false);
                        SetPollingEvents(VPLSOCKET_POLL_HUP);
                        //release block
                        SetEvent(completedEvent);
                        return;
                    }

                    if( availableBytes < 0) {
                        rv = VPL_ERR_CONNRESET;
                    }
                    else if( availableBytes == 0) {
                        if( status == AsyncStatus::Completed) {
                            SetIsClosed(true);
                            rv = 0;
                        }
                    }
                    else {
                        unsigned char* buffer = (unsigned char*)buf;
                        //read data from stream 
                        for(int i=0 ; i < availableBytes ; i++) {
                            buffer[i] = istream->ReadByte();
                        }
                        rv = availableBytes;
                    }
                    //Detach istream to avoid underlying using broken
                    istream->DetachStream();
                    SetIsPollReading(false);
                    SetPollingEvents(VPLSOCKET_POLL_RDNORM);
                    //release block
                    SetEvent(completedEvent);
                }
            );

            WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
            CloseHandle(completedEvent);

            if( rv <= 0) {
                SetIsExcep(true);
            }
        }
        if( m_type == VPLSOCKET_DGRAM ) {
            WaitForSingleObjectEx(m_hUDPRecvHandle ,INFINITE, TRUE);
            rv = GetBuffer((unsigned char*)buf,len,isBufEmpty);
        }
    }
end:
    return rv;
}

int _SocketData::GetBuffer(unsigned char* buf, int len, bool& isEmpty)
{
    MutexAutoLock lock(&m_dataBufferMutex);
    int readByteLengh = 0;

    if(m_BufferQueue.size() == 0) {
        isEmpty = true;
    }
    else {
        isEmpty = false;
        if( m_type != VPLSOCKET_STREAMLISTENER ) {
            while(true) {
                _SocketBuffer &s_buffer = m_BufferQueue.front();
            
                if(m_type == VPLSOCKET_DGRAM) {
                    if( m_recvFromAddr.addr == -1 ) {
                        m_recvFromAddr = s_buffer.sourceAddr;
                    }
                    else if( m_recvFromAddr.addr != s_buffer.sourceAddr.addr ) {
                        break;
                    }
                }

                if(s_buffer.unconsumedBufferLength > len) {                
                    int startPos = s_buffer.totalBufferLength-s_buffer.unconsumedBufferLength;
                    for(int j=startPos ; j<(len+startPos) ; j++)
                        buf[j-startPos] = s_buffer.buf[j];

                    readByteLengh += len;
                    s_buffer.unconsumedBufferLength -= len;

                    len = 0;
                }
                else if (s_buffer.unconsumedBufferLength == len) {
                    int startPos = s_buffer.totalBufferLength-s_buffer.unconsumedBufferLength;
                    for(int j=startPos ; j<(len+startPos) ; j++)
                        buf[j-startPos] = s_buffer.buf[j];

                    free( s_buffer.buf );
                    m_BufferQueue.pop();

                    readByteLengh += len;
                    len = 0;
                }
                else {
                    int startPos = s_buffer.totalBufferLength-s_buffer.unconsumedBufferLength;
                    for(int j=startPos ; j<s_buffer.totalBufferLength ; j++)
                        buf[j-startPos] = s_buffer.buf[j];

                    readByteLengh += s_buffer.unconsumedBufferLength;
                    len -= s_buffer.unconsumedBufferLength;
                    free( s_buffer.buf );
                    m_BufferQueue.pop();
                }

                if(len == 0 || m_BufferQueue.size() == 0) break;
            }
        }

        if(m_type == VPLSOCKET_DGRAM && m_BufferQueue.size()) {
            ResetEvent(m_hUDPRecvHandle);
        }

        m_TotalBufferSize -= readByteLengh;
    }

    return readByteLengh;
}

StreamSocket^ _SocketData::GetClientBuffer()
{
    StreamSocket^ ret = nullptr;
    MutexAutoLock lock(&m_connectedSocketsMutex);
    if( m_ConnectedSockets.size() > 0 ) {
        ret = m_ConnectedSockets.front();
        m_ConnectedSockets.pop();
    }
    return ret;
}

VPLNet_port_t _SocketData::GetRemotePort()
{
    if( m_type == VPLSOCKET_STREAM ) {
        return VPLNet_port_hton(_wtoi(((StreamSocket^)m_socket)->Information->LocalPort->Data()));
    }
    else
        return 0;
}

VPLNet_addr_t _SocketData::GetRemoteAddr()
{
    if( m_type == VPLSOCKET_STREAM ) {
        return StringToAddr(((StreamSocket^)m_socket)->Information->LocalAddress->RawName);
    }
    else
        return 0;
}

_SocketData::~_SocketData()
{
    // if socket close while polling -> release poll lock
    SetPollingEvents(VPLSOCKET_POLL_HUP);

    VPLMutex_Unlock(&m_acceptMutex);
    VPLMutex_Destroy(&m_acceptMutex);
    VPLMutex_Unlock(&m_wFlagMutex);
    VPLMutex_Destroy(&m_wFlagMutex);
    VPLMutex_Unlock(&m_wpFlagMutex);
    VPLMutex_Destroy(&m_wpFlagMutex);
    VPLMutex_Unlock(&m_eFlagMutex);
    VPLMutex_Destroy(&m_eFlagMutex);
    VPLMutex_Unlock(&m_rpFlagMutex);
    VPLMutex_Destroy(&m_rpFlagMutex);
    VPLMutex_Unlock(&m_cFlagMutex);
    VPLMutex_Destroy(&m_cFlagMutex);
    VPLMutex_Unlock(&m_dataBufferMutex);
    VPLMutex_Destroy(&m_dataBufferMutex);
    VPLMutex_Unlock(&m_connectedSocketsMutex);
    VPLMutex_Destroy(&m_connectedSocketsMutex);
    VPLMutex_Unlock(&m_pollingMutex);
    VPLMutex_Destroy(&m_pollingMutex);
    VPLMutex_Unlock(&m_sendMutex);
    VPLMutex_Destroy(&m_sendMutex);

    //cancel async operations
    if( m_LoadToBufferAction != nullptr && m_LoadToBufferAction->Status == AsyncStatus::Started ) {
        m_LoadToBufferAction->Cancel();
    }
    //close Socket 
    // For apps written in C++, the Close() method will be called when using the delete keyword on the StreamSocket object
    // reference: http://msdn.microsoft.com/en-US/library/windows/apps/windows.networking.sockets.streamsocket.close
    if(m_socket != nullptr) {
        switch(m_type) {
        case VPLSOCKET_STREAM:
            delete ((StreamSocket^)m_socket);
            break;
        case VPLSOCKET_DGRAM:
            delete ((DatagramSocket^)m_socket);
            break;
        case VPLSOCKET_STREAMLISTENER:    
            delete ((StreamSocketListener^)m_socket);
            break;
        default:
            break;
        }
    }
}


//--------------------------------------------------------------------------
// VPLSocket APIs

int VPLSocket_Init(void)
{
    srand( time(NULL) );
    VPLSocket_Quit();
    return VPL_OK;
}

int VPLSocket_Quit(void)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));

    //clear socket pool
    if( m_SocketPool.size() > 0) {
        for (std::map<int,_SocketData^>::iterator it = m_SocketPool.begin(); it != m_SocketPool.end(); ++it) {
            _SocketData^ data = it->second;
            if( data != nullptr)
                delete data;
        }
    }
    m_SocketPool.clear();
    return VPL_OK;
}

VPLSocket_t VPLSocket_Create(int family, int type, VPL_BOOL nonblock)
{
    //init VPLSocket_t
    VPLSocket_t ret = VPLSOCKET_INVALID;
    _SocketData^ data = ref new _SocketData(type, !nonblock);
    //put data to map
    ret.s = PushSocketData(ret,data);
    return ret;
}

int VPLSocket_SetSockOpt(VPLSocket_t socket, int level, int optionName, const void* optionValue, unsigned int optionLen)
{
    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }
    if( optionValue == NULL) {
        return VPL_ERR_INVALID;
    }
    if( optionName == VPLSOCKET_TCP_SECURE) {
        if( data->m_type != VPLSOCKET_STREAM || data->m_stage >= SOCKET_STAGE_CONNECT )
            return VPL_ERR_INVALID;
    }

    return data->SetSocketOpt(optionName, *(int*)(optionValue));
}

int VPLSocket_GetSockOpt(VPLSocket_t socket, int level, int optionName, void* optionValue, unsigned int optionLen)
{
    int rv = VPL_OK;

    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }

    // In WinRT socket, only support VPLSOCKET_TCP_NODELAY option for StreamSocket class
    // StreamSocketListener  & DatagramSocket not support
    if( data->m_socket == nullptr ||
        data->m_type == VPLSOCKET_STREAMLISTENER ||
        data->m_type == VPLSOCKET_DGRAM) {
            rv = VPL_ERR_INVALID;
    }
    else {
        if( optionName == VPLSOCKET_TCP_NODELAY) {
            bool* value = (bool*)optionValue;
            *value = ((StreamSocket^)data->m_socket)->Control->NoDelay;
            optionLen = sizeof(optionValue);
        }
        else {
            // TODO: support all other VPLSocket options?
            // VPLSOCKET__SO_BROADCAST, VPLSOCKET__SO_DEBUG, VPLSOCKET__SO_ERROR, 
            // VPLSOCKET__SO_DONTROUTE, VPLSOCKET__SO_KEEPALIVE, VPLSOCKET__SO_LINGER,
            // VPLSOCKET__SO_OOBINLINE, VPLSOCKET__SO_RCVBUF, VPLSOCKET__SO_REUSEADDR,
            // VPLSOCKET__SO_SNDBUF, VPLSOCKET__IP_DONTFRAGMENT
            rv = VPL_ERR_NOOP;
        }
    }

    return rv;
}

int VPLSocket_Bind(VPLSocket_t socket, const VPLSocket_addr_t* in_addr, size_t addrSize)
{
    int rv = VPL_OK;

    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }
    //bind with NULL address should get VPL_ERR_INVALID
    //bind with same socket should get VPL_ERR_INVALID
    if ( in_addr == NULL || data->m_stage >= SOCKET_STAGE_BIND) {
        return VPL_ERR_INVALID;
    }

    //To bind to IP ADDR_ANY
    //We should pass "Empty String" to argument [localServiceName] of function BindEndpointAsync() or BindServiceNameAsync()
    //then system will select the local TCP port on which to bind.
    // reference: http://msdn.microsoft.com/en-US/library/windows/apps/windows.networking.sockets.streamsocketlistener.bindservicenameasync
    String^ addr = nullptr;
    if( in_addr->addr == VPLNET_ADDR_ANY || in_addr->addr == VPLNET_ADDR_INVALID)
        addr = "";
    else
        addr = _SocketData::AddrToString(in_addr->addr);
    //get port
    //To bind to a port which allocate by system
    //We should pass "Empty String" to argument [localServiceName] of function BindEndpointAsync() or BindServiceNameAsync()
    //then system will select the local TCP port on which to bind.
    // reference: http://msdn.microsoft.com/en-US/library/windows/apps/windows.networking.sockets.streamsocketlistener.bindservicenameasync
    // reference: http://msdn.microsoft.com/en-US/library/windows/apps/windows.networking.sockets.streamsocketlistener.bindendpointasync
    String^ port = nullptr;
    if(in_addr->port == VPLNET_PORT_ANY)
        port = "";
    else
        port = VPLNet_port_ntoh(in_addr->port).ToString();

    if( data->m_type == VPLSOCKET_STREAM || data->m_type == VPLSOCKET_STREAMLISTENER) { 
        //TCP socket -> regard as TCP Listener
        data->CreateTCPListener();
    }
    //bind
    rv = data->DoBind(addr,port);

    if(rv == VPL_OK) {
        data->m_stage = SOCKET_STAGE_BIND;
        data->m_localAddr = in_addr->addr;
    }

    return rv;
}

int VPLSocket_Listen(VPLSocket_t socket, int backlog)
{
    int rv = VPL_OK;

    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }
    if( data->m_type == VPLSOCKET_STREAM || data->m_type == VPLSOCKET_DGRAM) { 
        return VPL_ERR_OPNOTSUPPORTED;
    }
    if( data->m_stage < SOCKET_STAGE_BIND ) {
        return VPL_ERR_DESTADDRREQ;
    }

    data->CreateTCPListener();
    data->m_MaxPandingClient = backlog;
    data->m_stage = SOCKET_STAGE_LISTEN;

    return rv;
}

int VPLSocket_Accept(VPLSocket_t socket, VPLSocket_addr_t* in_addr, size_t addrSize, VPLSocket_t* connectedSocket)
{
    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }
    if( data->m_stage < SOCKET_STAGE_LISTEN ){
        return VPL_ERR_INVALID;
    }
    if ( connectedSocket == NULL ) {
        return VPL_ERR_INVALID;
    }
    if( data->m_type == VPLSOCKET_STREAM || data->m_type == VPLSOCKET_DGRAM) { 
        return VPL_ERR_OPNOTSUPPORTED;
    }

    *connectedSocket = VPLSOCKET_INVALID;

    if( data->m_type == VPLSOCKET_STREAM || data->m_type == VPLSOCKET_DGRAM) { 
        //TCP client socket & UDP socket not support
        return VPL_ERR_INVALID;
    }

    _SocketData^ clientData = nullptr;
    StreamSocket^ clientSock = nullptr;

    if( data->IsBlocking() ) {
        //do accept
        data->DoAccept();
    }
    else {
        if( data->ConnectionQueueSize() == 0) {
            goto noconn;
        }
    }

    //get connected StreamSocket
    clientSock = data->GetClientBuffer();
    if(clientSock == nullptr) {
        //bug 13335: avoid accessing nullptr
        LOG_WARN("A NULL socket handle is accepted by listen socket, return VPL_ERR_FAIL.");
        goto failed;
    }

    //non-blocking property should inherit from listen client
    clientData = ref new _SocketData(VPLSOCKET_STREAM, data->IsBlocking());
    clientData->AcceptTCPClient(clientSock);
    //push to map
    connectedSocket->s = PushSocketData(*connectedSocket,clientData);
    //assign return value of addr if wish
    if( in_addr != NULL) {
        in_addr->addr = clientData->GetRemoteAddr();
        in_addr->port = clientData->GetRemotePort();
    }

    {
        char* msgA = NULL;
        _VPL__wstring_to_utf8_alloc(((StreamSocket^)clientData->m_socket)->Information->RemoteAddress->RawName->Data(),&msgA);
        LOG_TRACE("VPLSocket_Accept: get socket: %d from addr: %s",connectedSocket->s,msgA);
        if(msgA != NULL) free(msgA);
    }

    return VPL_OK;
noconn:
    return VPL_ERR_AGAIN;
failed:
    return VPL_ERR_FAIL;
}

static int connect_with_timeouts(VPLSocket_t socket, const VPLSocket_addr_t* in_addr, size_t addrSize,
                                 VPLTime_t timeout_nonroutable, VPLTime_t timeout_routable)
{
    int rv = VPL_OK;
    VPLTime_t timeout = 0;
    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end, timeout_remaining;

    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }
    if ( in_addr == NULL ) {
        return VPL_ERR_INVALID;
    }
    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        return VPL_ERR_INVALID;
    }

    //regard as TCP client
    if( data->m_type == VPLSOCKET_STREAM ) {
        data->CreateTCPClient();
    }

    // For routable addresses, wait 5 seconds max for response.
    // For non-routable addresses, wait 1 second max for response.
    // Non-routable address ranges:
    // * 10.0.0.0 - 10.255.255.255
    // * 172.16.0.0.- 172.31.255.255
    // * 192.168.0.0 - 192.168.255.255
    // * 169.254.0.0 - 169.254.255.255
    if(!VPLNet_IsRoutableAddress(in_addr->addr)) {
        // Non-routable address.
        end = start + timeout_nonroutable;
    }
    else {
        // Routable address. May take longer.
        end = start + timeout_routable;
    }

    String^ addr = _SocketData::AddrToString(in_addr->addr);
    String^ port = VPLNet_port_ntoh(in_addr->port).ToString();

    do {
        timeout_remaining = VPLTime_DiffClamp(end, VPLTime_GetTimeStamp());
        rv = data->DoConnect(addr,port,timeout_remaining);
    } while((rv == VPL_ERR_TIMEOUT) && (VPLTime_GetTimeStamp() < end));

    return rv;
}

int VPLSocket_Connect(VPLSocket_t socket, const VPLSocket_addr_t* in_addr, size_t addrSize)
{
    return connect_with_timeouts(socket, in_addr, addrSize, VPLTime_FromSec(1), VPLTime_FromSec(5));
}

int VPLSocket_ConnectWithTimeout(VPLSocket_t socket, const VPLSocket_addr_t* in_addr, size_t addrSize, VPLTime_t timeout)
{
    return connect_with_timeouts(socket, in_addr, addrSize, timeout, timeout);
}

int VPLSocket_ConnectWithTimeouts(VPLSocket_t socket, const VPLSocket_addr_t* in_addr, size_t addrSize, VPLTime_t timeout_nonroutable, VPLTime_t timeout_routable)
{
    return connect_with_timeouts(socket, in_addr, addrSize, timeout_nonroutable, timeout_routable);
}

int _vplsocket_connect_HN(VPLSocket__t socket, const char* hostname, VPLNet_port_t port)
{
    int rv = VPL_OK;

    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        return VPL_ERR_BADF;
    }
    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        return VPL_ERR_INVALID;
    }

    //regard as TCP client
    if( data->m_type == VPLSOCKET_STREAM ) {
        data->CreateTCPClient();
    }

    // ANSI(char) to Unicode(WCHAR)
    int nIndex = MultiByteToWideChar(CP_ACP, 0, hostname, -1, NULL, 0);
    TCHAR *pUnicode = new TCHAR[nIndex + 1];
    MultiByteToWideChar(CP_ACP, 0, hostname, -1, pUnicode, nIndex);
    String^ addr = ref new String(pUnicode);
    delete pUnicode;

    // This function is for domain name socket connect
    // We don't know the IP address of @hostname
    // Instead of using VPLNet_GetAddr() to retrieve ip address of @hostname, temporary set timeout to 5 seconds
    return data->DoConnect(addr,VPLNet_port_ntoh(port).ToString(), VPLTime_FromSec(5));
}

int VPLSocket_Poll(VPLSocket_poll_t* sockets, int num_sockets, VPLTime_t timeout)
{
    int rv = VPL_OK;

    if(sockets == NULL) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    //set blocking time
    int plat_timeout;
    bool isBlocking = false;
    if ( timeout == VPL_TIMEOUT_NONE ) {
        isBlocking = true;
    } else {
        VPLTime_t temp = timeout / 1000;
        if (temp > INT_MAX) {
            plat_timeout = INT_MAX;
        } else {
            plat_timeout = (int)temp;
        }
    }

    //create event for this time polling
    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        rv = VPL_ERR_FAIL;
        goto end;
    }
    //insert the event to polling pool
    int pollingKey = AddPollingJob(completedEvent);

    //do poll for every sockets in poll list
    bool bFound = false;
    for (int i = 0; i < num_sockets; i++) {
        _SocketData^ data = GetSocketData(sockets[i].socket);
        if( data != nullptr) {
            LOG_DEBUG("Socket[%d]: %d  Event:%d key:%d", i, data->m_socketKey, sockets[i].events, pollingKey);
            data->DoPoll(pollingKey, sockets[i].events);
            bFound = true;
        }
    }
    if (!bFound) {
        // bug 4807: cannot find invalid socket to poll in sockets
        RemovePollingJob(pollingKey);
        rv = VPL_ERR_INVALID;
        goto end;
    }

    {
        //wait until any socket is selected
        std::thread t(
            [isBlocking,&completedEvent,plat_timeout,pollingKey] {
                if( IsPollingHandleAlive(pollingKey) ) {
                    try{
                        if(isBlocking) {
                            WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
                        }
                        else
                            WaitForSingleObjectEx(completedEvent, plat_timeout, TRUE);
                    }
                    catch(Exception^ e) {
                        //
                    }
                }
            }
        );
        t.join();
        RemovePollingJob(pollingKey);
    }

    //set revent for every sockets in poll list
    for (int i = 0; i < num_sockets; i++) {
        _SocketData^ data = GetSocketData(sockets[i].socket);
        if( data != nullptr) {
            sockets[i].revents = data->GetPollResult(pollingKey, sockets[i].events);
            LOG_DEBUG("Socket[%d]: %d  Event:%d Revent:%d key:%d", i, data->m_socketKey, sockets[i].events, sockets[i].revents, pollingKey);
        }
        if( sockets[i].revents ) {
            rv++;
        }
    }

end:
    return rv;
}

int VPLSocket_Send(VPLSocket_t socket, const void* msg, int len)
{
    int rv = VPL_OK;

    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        rv = VPL_ERR_BADF;
        goto end;
    }
    // by VPLTest socket test case, we should return VPL_OK if msg is NULL
    if(msg == NULL) {
        rv = VPL_OK;
        goto end;
    }
    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        rv = VPL_ERR_INVALID;
        goto end;
    }    
    //send with un-connected socket should get VPL_ERR_INVALID
    if( data->m_stage < SOCKET_STAGE_CONNECT) {
        rv = VPL_ERR_NOTCONN;
        goto end;
    }   
    if(data->IsClosed()) {
        rv = VPL_ERR_CONNRESET;
        goto end;
    }
    rv = data->DoSend(msg,len);

end:
    return rv;
}

int VPLSocket_SendTo(VPLSocket_t socket, const void* buf, int len, const VPLSocket_addr_t* addr, size_t addrSize)
{
    int rv = VPL_OK;

    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        rv = VPL_ERR_BADF;
        goto end;
    }
    // by VPLTest socket test case, we should return VPL_OK if data is NULL
    if(buf == NULL) {
        rv = VPL_OK;
        goto end;
    }
    if( addr == NULL ) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        rv = VPL_ERR_INVALID;
        goto end;
    } 
    if(data->IsClosed()) {
        rv = VPL_ERR_CONNRESET;
        goto end;
    }

    if( data->m_type == VPLSOCKET_STREAM ) {
        data->CreateTCPClient();

        rv = VPLSocket_Connect(socket,addr,addrSize);

        if( rv == VPL_OK || rv == VPL_ERR_ISCONN) { //if connect OK or already connect -> send data
            rv = VPLSocket_Send(socket,buf,len);
        }
    }
    else if( data->m_type == VPLSOCKET_DGRAM ) {
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if( !completedEvent ) {
            rv = VPL_ERR_FAIL;
            goto end;
        }

        String^ addrStr = _SocketData::AddrToString(addr->addr);
        String^ portStr = VPLNet_port_ntoh(addr->port).ToString();
        IAsyncOperation<IOutputStream^>^ getOStreamAction = nullptr;
        try {
            getOStreamAction = ((DatagramSocket^)data->m_socket)->GetOutputStreamAsync(ref new HostName(addrStr), portStr);
        }
        catch (Exception^ exception){
            rv = exceptionHandling(exception->HResult);
            goto end;
        }

        if( getOStreamAction->Status == AsyncStatus::Error ) {
            rv = VPL_ERR_FAIL;
            goto end;
        }

        getOStreamAction->Completed = ref new AsyncOperationCompletedHandler<IOutputStream^>(
            [data,buf,&len,&rv,&completedEvent] (IAsyncOperation<IOutputStream^>^ op, AsyncStatus status) {
                try {
                    DataWriter^ writer = ref new DataWriter(op->GetResults());
                    for(int i=0 ; i<len ; i++) {
                        writer->WriteByte( ((unsigned char*)buf)[i] );
                    }

                    writer->StoreAsync()->Completed = ref new AsyncOperationCompletedHandler<unsigned int>(
                        [writer,&rv,len,&completedEvent] (IAsyncOperation<unsigned int>^ storeOperation, AsyncStatus state) {
                            unsigned int wrote = 0;
                            try {
                                wrote = storeOperation->GetResults();
                            }
                            catch(Exception^ exception) {
                                rv = exceptionHandling(exception->HResult);
                            }

                            rv = wrote;
                            writer->DetachStream();
                            SetEvent(completedEvent);
                        }
                    );
                }
                catch(Exception^ exception) {
                    rv = exceptionHandling(exception->HResult);
                    SetEvent(completedEvent);
                }
            }
        );

        WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
        CloseHandle(completedEvent);
    }

end:
    return rv;
}

int VPLSocket_Recv(VPLSocket_t socket, void* buf, int len)
{
    int rv = VPL_OK;
    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        rv = VPL_ERR_BADF;
        goto end;
    }
    // by VPLTest socket test case, we should return VPL_ERR_AGAIN if buf is NULL
    if(buf == NULL) {
        rv = VPL_ERR_AGAIN;
        goto end;
    }
    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        rv = VPL_ERR_INVALID;
        goto end;
    }
    if(data->IsClosed()) {
        rv = 0;
        goto end;
    }

    rv = data->DoReceive(buf,len);

end:
    return rv;
}

int VPLSocket_RecvFrom(VPLSocket_t socket, void* buf, int len, VPLSocket_addr_t* senderAddr, size_t senderAddrSize)
{
    int rv = VPL_OK;
    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        rv = VPL_ERR_BADF;
        goto end;
    }
    // by VPLTest socket test case, we should return VPL_ERR_AGAIN if buf is NULL
    if(buf == NULL || senderAddr == NULL || data->m_stage == SOCKET_STAGE_RRCVFROM) {
        rv = VPL_ERR_AGAIN;
        goto end;
    }
    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        rv = VPL_ERR_INVALID;
        goto end;
    }    
    if(data->IsClosed()) {
        rv = 0;
        goto end;
    }

    data->m_stage = SOCKET_STAGE_RRCVFROM;
    rv = VPLSocket_Recv(socket,buf,len);

    if(rv > 0) {
        if( senderAddr != NULL) {
            if( data->m_type == VPLSOCKET_STREAM ) { //if TCP -> direct get the remote address
                senderAddr->addr = data->GetRemoteAddr();
                senderAddr->port = data->GetRemotePort();            
            }
            else if(data->m_type == VPLSOCKET_DGRAM ) { //if UDP -> get from buffer
                senderAddr->addr =  data->m_recvFromAddr.addr;
                senderAddr->port =  data->m_recvFromAddr.port;
                data->ClearRecvFromAddr();
            }
        }
    }
    data->m_stage = SOCKET_STAGE_RRCVFROM_FIN;

end:
    return rv;
}

int VPLSocket_Close(VPLSocket_t socket)
{
    int rv = VPL_OK;

    //close Socketdata
    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        rv = VPL_ERR_NOTSOCK;
    }
    else {
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        m_SocketPool.erase(socket.s);
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mutex));
        delete data;
        socket = VPLSOCKET_INVALID;
    }

    return rv;
}

int VPLSocket_Shutdown(VPLSocket_t socket, int how)
{
    int rv = VPL_OK;

    //close Socketdata
    _SocketData^ data = GetSocketData(socket);
    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID) || data == nullptr ) {
        rv = VPL_ERR_BADF;
        goto end;
    }
    if ( data->m_socket == nullptr ) {
        rv = VPL_ERR_NOTCONN;
        goto end;
    }

    switch(data->m_type) {
    case VPLSOCKET_STREAM:
        delete ((StreamSocket^)data->m_socket);
        break;
    case VPLSOCKET_DGRAM:
        delete ((DatagramSocket^)data->m_socket);
        break;
    case VPLSOCKET_STREAMLISTENER:    
        delete ((StreamSocketListener^)data->m_socket);
        break;
    default:
        break;
    }

end:
    return rv;
}

VPLNet_port_t VPLSocket_GetPort(VPLSocket_t socket)
{
    _SocketData^ data = GetSocketData(socket);
    VPLNet_port_t ret = 0;
    
    if (data == nullptr) {
        ret = VPLNET_PORT_INVALID;
        goto end;
    }

    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        ret = VPLNet_port_hton(_wtoi(((StreamSocketListener^)data->m_socket)->Information->LocalPort->Data()));
    }
    else if( data->m_type == VPLSOCKET_STREAM ) {
        ret = VPLNet_port_hton(_wtoi(((StreamSocket^)data->m_socket)->Information->LocalPort->Data()));
    }
    else {
        ret = VPLNet_port_hton(_wtoi(((DatagramSocket^)data->m_socket)->Information->LocalPort->Data()));
    }

end:
    return ret;
}

VPLNet_addr_t VPLSocket_GetAddr(VPLSocket_t socket)
{
    _SocketData^ data = GetSocketData(socket);
    VPLNet_addr_t ret = 0;

    if (data == nullptr) {
        ret = VPLNET_ADDR_INVALID;
        goto end;
    }    

    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        ret = data->m_localAddr;
    }
    else if( data->m_type == VPLSOCKET_STREAM ) {
        ret = _SocketData::StringToAddr(((StreamSocket^)data->m_socket)->Information->LocalAddress->RawName);
    }
    else {
        ret = _SocketData::StringToAddr(((DatagramSocket^)data->m_socket)->Information->LocalAddress->RawName);
    }

end:
    return ret;
}

int VPLSocket_SetKeepAlive(VPLSocket_t socket, int enable, int waitSec, int intervalSec, int count)
{
    return VPL_OK;
}

int VPLSocket_GetKeepAlive(VPLSocket_t socket, int* enable, int* waitSec, int* intervalSec, int* count)
{
    return VPL_OK;
}

VPLNet_port_t VPLSocket_GetPeerPort(VPLSocket_t socket)
{
    _SocketData^ data = GetSocketData(socket);
    VPLNet_port_t ret = VPLNET_PORT_INVALID;

    if (data == nullptr) {
        goto end;
    }

    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        goto end;
    }
    else if( data->m_type == VPLSOCKET_STREAM ) {
        StreamSocket^ s = (StreamSocket^)data->m_socket;
        //if Information::RemoteAddress is nullptr means socket is not yet connected
        if(s == nullptr || s->Information->RemoteAddress == nullptr) {
            goto end;
        }
        ret = ret = VPLNet_port_hton(_wtoi(s->Information->RemotePort->Data()));
    }
    else {
        DatagramSocket^ s = (DatagramSocket^)data->m_socket;
        //if Information::RemoteAddress is nullptr means socket is not yet connected
        if(s == nullptr || s->Information->RemoteAddress == nullptr) {
            goto end;
        }
        ret = ret = VPLNet_port_hton(_wtoi(s->Information->RemotePort->Data()));
    }

end:
    return ret;
}

VPLNet_addr_t VPLSocket_GetPeerAddr(VPLSocket_t socket)
{
    _SocketData^ data = GetSocketData(socket);
    VPLNet_addr_t ret = VPLNET_ADDR_INVALID;

    if (data == nullptr) {
        goto end;
    }    

    if( data->m_type == VPLSOCKET_STREAMLISTENER ) {
        goto end;
    }
    else if( data->m_type == VPLSOCKET_STREAM ) {
        StreamSocket^ s = (StreamSocket^)data->m_socket;
        //if Information::RemoteAddress is nullptr means socket is not yet connected
        if(s == nullptr || s->Information->RemoteAddress == nullptr) {
            goto end;
        }
        ret = _SocketData::StringToAddr(s->Information->RemoteAddress->RawName);
    }
    else {
        DatagramSocket^ s = (DatagramSocket^)data->m_socket;
        //if Information::RemoteAddress is nullptr means socket is not yet connected
        if(s == nullptr || s->Information->RemoteAddress == nullptr) {
            goto end;
        }
        ret = _SocketData::StringToAddr(s->Information->RemoteAddress->RawName);
    }

end:
    return ret;
}
