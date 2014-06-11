// TcpDinServer.cpp for Ts2::Link testing.

#include "TcpDinServer.hpp"

#include <gvm_thread_utils.h>
#include <log.h>

#include <vplu_mutex_autolock.hpp>

#include <cassert>

TcpDinServer::TcpDinServer(const Ts2::Udi_t &localUdi, const Ts2::Udi_t &remoteUdi, VPLNet_port_t portSpec)
    : localUdi(localUdi), remoteUdi(remoteUdi), portSpec(portSpec),
      localInfo(NULL), listenSocket(VPLSOCKET_INVALID), acceptHelper(NULL), endPoint(NULL)
{
    VPLMutex_Init(&mutex);
    VPLCond_Init(&endPointReady_cond);
    VPLCond_Init(&qNonEmpty_cond);
}

TcpDinServer::~TcpDinServer()
{
    if (endPoint) {
        LOG_WARN("Server: EndPoint still exists");
        delete endPoint;
    }

    if (acceptHelper) {
        LOG_WARN("Server: AcceptHelper still exists");
        delete acceptHelper;
    }

    if (!VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID)) {
        LOG_WARN("Server: ListenSocket still open");
        VPLSocket_Close(listenSocket);
    }

    if (!packetQ.empty()) {
        LOG_WARN("Server: PacketQueue not empty (%d left)", packetQ.size());
        while (!packetQ.empty()) {
            Ts2::Packet *packet = packetQ.front();
            packetQ.pop();
            delete packet;
        }
    }

    VPLCond_Destroy(&endPointReady_cond);
    VPLCond_Destroy(&qNonEmpty_cond);
    VPLMutex_Destroy(&mutex);

    LOG_INFO("Server: Destroyed");
}

int TcpDinServer::Start()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    assert(!localInfo);
    localInfo = new Ts2::LocalInfo_FixedValues(localUdi.userId, localUdi.deviceId, 0);
    assert(localInfo);

    assert(VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID));
    listenSocket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, /*nonblock*/VPL_TRUE);
    assert(!VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID));
    LOG_INFO("Server: Created listen socket");

    VPLSocket_addr_t sin;
    sin.family = VPL_PF_INET;
    sin.addr = VPLNET_ADDR_LOOPBACK;
    sin.port = VPLConv_hton_u16(portSpec);
    err = VPLSocket_Bind(listenSocket, &sin, sizeof(sin));
    assert(!err);
    LOG_INFO("Server: Bound listen socket");

    err = VPLSocket_Listen(listenSocket, 10);
    assert(!err);
    LOG_INFO("Server: Listening..");

    err = Util_SpawnThread(threadMain, this, /*stackSize*/32*1024, /*isJoinable*/VPL_TRUE, &thread);
    assert(!err);
    LOG_INFO("Server: Spawned service thread");

    return err;
}

int TcpDinServer::Stop()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);

        err = VPLSocket_Close(listenSocket);
        assert(!err);
        listenSocket = VPLSOCKET_INVALID;
        LOG_INFO("Server: Closed ListenSocket");
    
        err = endPoint->Stop();
        assert(!err);
        LOG_INFO("Server: Sent Stop to EndPoint");
    }

    enqueue(NULL);  // NULL Packet is a sign for thread to exit.
    LOG_INFO("Server: Queued NULL packet, to signal thread to exit");

    return err;
}

int TcpDinServer::WaitStopped()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);

        err = endPoint->WaitStopped();
        assert(!err);
        LOG_INFO("Server: EndPoint stopped");

        delete endPoint;
        LOG_INFO("Server: Destroyed EndPoint");
        endPoint = NULL;
    }

    LOG_INFO("Server: Joining thread..");
    err = VPLDetachableThread_Join(&thread);
    assert(!err);
    LOG_INFO("Server: Joined thread");

    return err;
}

VPLNet_port_t TcpDinServer::GetPort() const
{
    MutexAutoLock lock(&mutex);
    assert(!VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID));
    return VPLNet_port_ntoh(VPLSocket_GetPort(listenSocket));
}

// class method
VPLTHREAD_FN_DECL TcpDinServer::threadMain(void *param)
{
    TcpDinServer *server = reinterpret_cast<TcpDinServer*>(param);
    server->threadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void TcpDinServer::threadMain()
{
    assert(!VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID));

    waitConnectionAttempt();
    acceptConnectionAttemptAsync();
    waitEndPointReady();
    {
        MutexAutoLock lock(&mutex);
        assert(acceptHelper);
        delete acceptHelper;
        acceptHelper = NULL;
    }

    processPackets();
}

// class method
void TcpDinServer::acceptedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType,
                                         u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                         Ts2::Link::AcceptHelper *acceptHelper, void *context)
{
    assert(context);
    TcpDinServer *server = reinterpret_cast<TcpDinServer*>(context);
    server->acceptedSocketHandler(socket, routeType,
                                  remoteUserId, remoteDeviceId, remoteInstanceId,
                                  acceptHelper);
}

void TcpDinServer::acceptedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType,
                                         u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                         Ts2::Link::AcceptHelper *acceptHelper)
{
    // AcceptedSocketHandler() could be called multiple times, to deliver possibly multiple accepted sockets (e.g., PRX/P2P case).
    // When all accepted sockets are delivered, there will be one more call, indicated by ROUTE_TYPE_INVALID.

    MutexAutoLock lock(&mutex);

    if (routeType != Ts2::Link::ROUTE_TYPE_INVALID) {
        LOG_INFO("Server: Received an accepted socket");
        assert(!endPoint);
        endPoint = new Ts2::Link::EndPoint(socket, remoteUserId, remoteDeviceId, remoteInstanceId, recvPacketHandler, this, Ts2::Link::ROUTE_TYPE_DIN, localInfo);
        assert(endPoint);
        int err = endPoint->Start();
        assert(!err);
    }
    else {
        LOG_INFO("Server: No more accepted sockets");
        VPLCond_Signal(&endPointReady_cond);
    }
}

// class method
void TcpDinServer::recvPacketHandler(Ts2::Packet *packet, void *context)
{
    assert(context);
    TcpDinServer *server = reinterpret_cast<TcpDinServer*>(context);
    server->recvPacketHandler(packet);
}

void TcpDinServer::recvPacketHandler(Ts2::Packet *packet)
{
    assert(packet);  // Received Packet can never be NULL.
    enqueue(packet);
}

void TcpDinServer::waitConnectionAttempt()
{
    VPLSocket_poll_t pollspec[1];
    pollspec[0].socket = listenSocket;
    pollspec[0].events = VPLSOCKET_POLL_RDNORM;
    int numSockets = VPLSocket_Poll(pollspec, 1, VPL_TIMEOUT_NONE);
    assert(numSockets == 1);
    assert(pollspec[0].revents == VPLSOCKET_POLL_RDNORM);
    LOG_INFO("Server: Detected connection attempt");
}

void TcpDinServer::acceptConnectionAttemptAsync()
{
    VPLSocket_t socket = VPLSOCKET_SET_INVALID;
    VPLSocket_addr_t addr;
    int err = VPLSocket_Accept(listenSocket, &addr, sizeof(addr), &socket);
    assert(!err);
    LOG_INFO("Server: Connection from "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t,
             VAL_VPLNet_addr_t(addr.addr), VPLNet_port_ntoh(addr.port));
    acceptHelper = new Ts2::Link::TcpDinAcceptHelper(acceptedSocketHandler, this, socket, localInfo);
    err = acceptHelper->Accept();
    assert(!err);
}

void TcpDinServer::waitEndPointReady()
{
    MutexAutoLock lock(&mutex);
    while (!endPoint) {
        int err = VPLCond_TimedWait(&endPointReady_cond, &mutex, VPL_TIMEOUT_NONE);
        assert(!err);
    }
    LOG_INFO("Server: EndPoint ready");
}

void TcpDinServer::processPackets()
{
    while (1) {
        Ts2::Packet *packet = NULL;
        {
            MutexAutoLock lock(&mutex);
            while (packetQ.size() == 0) {
                int err = VPLCond_TimedWait(&qNonEmpty_cond, &mutex, VPL_TIMEOUT_NONE);
                assert(!err);
                LOG_INFO("Server: Woke up from CondVar: packetQ.size=%d", packetQ.size());
            }

            packet = packetQ.front();
            packetQ.pop();
        }

        if (packet == NULL) {  // sign for thread to exit
            LOG_INFO("Server: Detected NULL packet - thread exiting");
            break;
        }

        // Each Packet object owns the memory used to store data.
        // Thus, it cannot be shared.
        // Make a copy, to use in the response Packet.
        size_t data_size = (u32)packet->GetDataSize();
        Ts2::PacketHdr_t header;
        packet->DupHdr(header);
        char *pkt_body = new char[data_size];
        memcpy(pkt_body, packet->pkt_body, data_size);
        Ts2::Packet *respPacket = new Ts2::Packet(header, (u8*)pkt_body, data_size, true);
        pkt_body = NULL;  // ownership transferred to Packet obj
        delete packet;

        int err = endPoint->Send(respPacket);
        assert(!err);
        LOG_INFO("Server: Sent packet");

        delete respPacket;
    }
}

void TcpDinServer::enqueue(Ts2::Packet *packet)
{
    MutexAutoLock lock(&mutex);
    packetQ.push(packet);
    VPLCond_Signal(&qNonEmpty_cond);
}

