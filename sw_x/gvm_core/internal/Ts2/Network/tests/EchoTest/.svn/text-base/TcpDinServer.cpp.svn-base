// TcpDinServer.cpp for Ts2::Network testing.

#include "TcpDinServer.hpp"

#include <gvm_thread_utils.h>
#include <log.h>

#include <vplu_mutex_autolock.hpp>

#include <cassert>

#ifdef TS2_NO_PXD
TcpDinServer::TcpDinServer(u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                 u64 clientUserId, u64 clientDeviceId, u32 clientInstanceId,
                 VPLNet_port_t portSpec)
    : serverUserId(serverUserId), serverDeviceId(serverDeviceId), serverInstanceId(serverInstanceId),
      clientUserId(clientUserId), clientDeviceId(clientDeviceId), clientInstanceId(clientInstanceId),
      portSpec(portSpec),
      localInfo(NULL), frontEnd(NULL)
{
    VPLMutex_Init(&mutex);
}
#else
TcpDinServer::TcpDinServer(const std::string &username, const std::string &password, VPLNet_port_t portSpec)
    : username(username), password(password), portSpec(portSpec),
      localInfo(NULL), frontEnd(NULL)
{
    VPLMutex_Init(&mutex);
}
#endif

TcpDinServer::~TcpDinServer()
{
    assert(!frontEnd);

    VPLMutex_Destroy(&mutex);

    LOG_INFO("Server: Destroyed");
}

u64 TcpDinServer::GetUserId() const
{
    return localInfo->GetUserId();
}

u64 TcpDinServer::GetDeviceId() const
{
    return localInfo->GetDeviceId();
}

u32 TcpDinServer::GetInstanceId() const
{
    return localInfo->GetInstanceId();
}

int TcpDinServer::Start()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    assert(!localInfo);
#ifndef TS2_NO_PXD
    {
        Ts2::LocalInfo_InfraHelper *_localInfo = new Ts2::LocalInfo_InfraHelper(username, password, "Network::EchoTest::Server");
        assert(_localInfo);
        localInfo = _localInfo;
        LOG_INFO("Server: deviceId is "FMTu64, localInfo->GetDeviceId());
    }
#else
    {
        Ts2::LocalInfo_FixedValues *_localInfo = new Ts2::LocalInfo_FixedValues(serverUserId, serverDeviceId, serverInstanceId);
        assert(_localInfo);
        localInfo = _localInfo;
    }
#endif
    assert(localInfo);

    assert(!frontEnd);
    frontEnd = new Ts2::Network::FrontEnd(recvdPacketHandler, this, VPLNET_ADDR_ANY, VPLNET_PORT_ANY, localInfo);
    assert(frontEnd);

    err = frontEnd->Start();
    assert(!err);

    LOG_INFO("Server: Start succeeded on Network::FrontEnd obj");

    return err;
}

int TcpDinServer::Stop()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        assert(frontEnd);
        err = frontEnd->Stop();
        assert(!err);
        LOG_INFO("Server: Sent Stop to Network::FrontEnd");
    }

    return err;
}

int TcpDinServer::WaitStopped()
{
    int err = 0;

    Ts2::Network::FrontEnd *_frontEnd = NULL;
    {
        MutexAutoLock lock(&mutex);
        _frontEnd = frontEnd;
        frontEnd = NULL;
    }
    assert(_frontEnd);
    err = _frontEnd->WaitStopped();
    assert(!err);
    LOG_INFO("Server: FrontEnd stopped");

    delete _frontEnd;
    LOG_INFO("Server: Destroyed FrontEnd");

    return err;
}

VPLNet_port_t TcpDinServer::GetPort() const
{
    MutexAutoLock lock(&mutex);
    assert(frontEnd);
    return frontEnd->GetTcpDinListenerPort();
}

// class method
void TcpDinServer::recvdPacketHandler(Ts2::Packet *packet, void *context)
{
    assert(context);
    TcpDinServer *server = reinterpret_cast<TcpDinServer*>(context);
    server->recvdPacketHandler(packet);
}

void TcpDinServer::recvdPacketHandler(Ts2::Packet *packet)
{
    assert(packet);  // Received Packet can never be NULL.

    // Each Packet object owns the memory used to store data.
    // Thus, it cannot be shared.
    // Make a copy, to use in the response Packet.
    size_t data_size = (u32)packet->GetDataSize();
    Ts2::PacketHdr_t header;
    packet->DupHdr(header);
    {
        Ts2::Udi_t tmp_udi = header.src_udi;
        header.src_udi = header.dst_udi;
        header.dst_udi = tmp_udi;
    }
    char *pkt_body = new char[data_size];
    memcpy(pkt_body, packet->pkt_body, data_size);
    Ts2::Packet *respPacket = new Ts2::Packet(header, (u8*)pkt_body, data_size, true);
    pkt_body = NULL;  // ownership transferred to Packet obj
    delete packet;

    {
        MutexAutoLock lock(&mutex);
        assert(frontEnd);
        int err = frontEnd->Enqueue(respPacket);
        assert(!err);
        LOG_INFO("Server: Enqueued packet");
    }
}

