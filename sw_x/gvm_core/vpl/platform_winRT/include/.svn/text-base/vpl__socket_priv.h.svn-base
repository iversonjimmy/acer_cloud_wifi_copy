#include "../../include/vpl_socket.h"
#include "vpl_th.h"
//#include <ppltasks.h>
#include <queue>
#include <map>
#include <thread>
#include <algorithm>

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Concurrency;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;


#define RECEIVE_MAX_LENGTH 100*1024 //define max recv len to 100Kbytes


// Set socket to secure socket
// value: boolean
#define VPLSOCKET_TCP_SECURE     0x1111

int _vplsocket_connect_HN(VPLSocket__t socket, const char* hostname, VPLNet_port_t port);

ref class _SocketData;

enum SOCKET_STAGE {
    SOCKET_STAGE_CREATE = 0,
    SOCKET_STAGE_BIND = 1,
    SOCKET_STAGE_LISTEN = 2,
    SOCKET_STAGE_CONNECT = 3,
    SOCKET_STAGE_POLL = 4,
    SOCKET_STAGE_RRCVFROM = 5,
    SOCKET_STAGE_RRCVFROM_FIN = 6,
};

struct _SocketBuffer {
    int totalBufferLength;
    int unconsumedBufferLength;
    unsigned char* buf;
    VPLSocket_addr_t sourceAddr;
};

struct _SocketOpt {
    int name;
    int value;
};

_SocketData^ GetSocketData(VPLSocket_t socket);
int PushSocketData(VPLSocket_t socket, _SocketData^ data);

//---------------------------------------
//Class _SocketData
ref class _SocketData sealed
{
internal:
    int m_type;
    Platform::Object^ m_socket;
    SOCKET_STAGE m_stage;
    HANDLE m_hAcceptHandle;
    HANDLE m_hUDPRecvHandle;
    int m_MaxPandingClient;
    VPLNet_addr_t m_localAddr;
    VPLSocket_addr_t m_recvFromAddr;
    std::queue<StreamSocket^> m_ConnectedSockets;
    std::queue<_SocketBuffer> m_BufferQueue;
    std::map<int, uint16_t> m_pollingKeyEvent;
    std::map<int, uint16_t> m_polledKeyRevent;
    int m_socketKey;
    VPLMutex_t m_wFlagMutex;
    VPLMutex_t m_wpFlagMutex;
    VPLMutex_t m_eFlagMutex;
    VPLMutex_t m_rpFlagMutex;
    VPLMutex_t m_cFlagMutex;
    VPLMutex_t m_dataBufferMutex;
    VPLMutex_t m_connectedSocketsMutex;  // mutex of connected sockets
      VPLMutex_t m_acceptMutex;  // mutex of m_hAcceptHandle, locking order never prior than m_connectedSocketsMutex
    VPLMutex_t m_pollingMutex;
    VPLMutex_t m_sendMutex;

private:
    ~_SocketData();

public:
    _SocketData(int type, bool isBlocking);
    

    static Platform::String^ AddrToString(VPLNet_addr_t addr);
    static VPLNet_addr_t StringToAddr(String^ addr);

    void OnConnection(StreamSocketListener^ listener, StreamSocketListenerConnectionReceivedEventArgs^ object);
    void OnMessageReceived(DatagramSocket^ socket, DatagramSocketMessageReceivedEventArgs^ e);

    int SizeToReadFromBuffer();
    bool IsWrite();
    bool IsExcep();
    bool IsPollReading();
    bool IsPollWriting();
    bool IsBlocking();
    bool IsClosed();
    void SetIsWrite(bool value);
    void SetIsExcep(bool value);
    void SetIsPollReading(bool value);
    void SetIsPollWriting(bool value);
    void SetIsClosed(bool value);
    void ResetPollFlags();

    void CreateTCPListener();
    void CreateTCPClient();
    void AcceptTCPClient(StreamSocket^ clientSock);
    int SetSocketOpt(int optName, int value);

    StreamSocket^ GetClientBuffer();

    VPLNet_port_t GetRemotePort();
    VPLNet_addr_t GetRemoteAddr();

internal:
    int DoBind(Platform::String^ addr, Platform::String^ port);
    int ConnectionQueueSize();
    void DoAccept();
    int DoConnect(Platform::String^ addr, Platform::String^ port, VPLTime_t timeout);
    int DoConnectNowait(Platform::String^ addr, Platform::String^ port);
    void DoPoll(int pollingKey, uint16_t poll_events);
    int DoSend(const void* msg, int len);
    int DoReceive(void* buf, int len);

    uint16_t GetPollResult(int pollingKey, uint16_t poll_events);

    int GetBuffer(unsigned char* buf, int len, bool &isEmpty);
    void ClearRecvFromAddr() {
        m_recvFromAddr.addr = -1;
        m_recvFromAddr.port = -1;
    }
    void ClearSocketOpt() {
        m_SocketOpt.name = -1;
        m_SocketOpt.value = false;
    }

private:
    volatile bool m_IsWrite;
    volatile bool m_IsExcep;
    volatile bool m_IsPollReading;
    volatile bool m_IsPollWriting;
    volatile bool m_IsBlocking;
    volatile bool m_IsTCPPollReading;
    volatile bool m_IsClosed;
    _SocketOpt m_SecureOpt;
    _SocketOpt m_SocketOpt;
    DataReaderLoadOperation^ m_LoadToBufferAction;
    int m_TotalBufferSize;

private:
    int LoadToBuffer(int recv_len = RECEIVE_MAX_LENGTH);
    void InsertPollingKey(int pollingKey, uint16_t event);
    void SetPollingEvents(uint16_t event);
};

