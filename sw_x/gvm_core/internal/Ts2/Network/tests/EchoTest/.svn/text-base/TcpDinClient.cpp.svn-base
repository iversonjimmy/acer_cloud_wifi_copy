// TcpDinClient.cpp for Ts2::Network testing.

#include "TcpDinClient.hpp"

#include <log.h>

#include <vplu_mutex_autolock.hpp>

#include <cassert>

#include <iostream>
#include <iomanip>
#include <sstream>

static void hexdump(const char *bytes, size_t nbytes)
{
    std::ostringstream oss;
    oss << std::endl;
    for (size_t i = 0; i < nbytes; i++) {
        if (i % 16 == 0) {
            oss << std::setfill('0') << std::setw(4) << std::hex << i << ": ";
        }
        oss << std::setfill('0') << std::setw(2) << std::hex << ((u32)bytes[i] & 0xff) << " ";
        if (i % 16 == 15) {
            oss << std::endl;
        }
    }
    oss << std::endl;
    std::string line(oss.str());
    LOG_INFO("%s", line.c_str());
}

#ifndef TS2_NO_PXD
TcpDinClient::TcpDinClient(const std::string &username, const std::string &password, 
                           u64 serverDeviceId, VPLNet_addr_t serverAddr, VPLNet_port_t serverPort)
    : username(username), password(password), serverDeviceId(serverDeviceId), serverAddr(serverAddr), serverPort(serverPort),
      localInfo(NULL), frontEnd(NULL)
{
    VPLMutex_Init(&mutex);
}
#else
TcpDinClient::TcpDinClient(u64 clientUserId, u64 clientDeviceId, u32 clientInstanceId,
                 u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                 VPLNet_addr_t serverAddr, VPLNet_port_t serverPort)
    : clientUserId(clientUserId), clientDeviceId(clientDeviceId), clientInstanceId(clientInstanceId),
      serverUserId(serverUserId), serverDeviceId(serverDeviceId), serverInstanceId(serverInstanceId),
      serverAddr(serverAddr), serverPort(serverPort),
      localInfo(NULL), frontEnd(NULL)
{
    VPLMutex_Init(&mutex);
}
#endif

TcpDinClient::~TcpDinClient()
{
    assert(!frontEnd);

    VPLMutex_Destroy(&mutex);

    LOG_INFO("Client: Destroyed");
}

u64 TcpDinClient::GetUserId() const
{
    return localInfo->GetUserId();
}

u64 TcpDinClient::GetDeviceId() const
{
    return localInfo->GetDeviceId();
}

u32 TcpDinClient::GetInstanceId() const
{
    return localInfo->GetInstanceId();
}

int TcpDinClient::Start()
{
    MutexAutoLock lock(&mutex);

    int err = 0;

    assert(!localInfo);
#ifndef TS2_NO_PXD
    {
        Ts2::LocalInfo_InfraHelper *_localInfo = new Ts2::LocalInfo_InfraHelper(username, password, "Network::EchoTest::Client");
        assert(_localInfo);
        _localInfo->SetServerTcpDinAddrPort(_localInfo->GetUserId(), serverDeviceId, /*instanceId*/0,
                                            serverAddr, serverPort);
        localInfo = _localInfo;
    }
#else
    {
        Ts2::LocalInfo_FixedValues *_localInfo = new Ts2::LocalInfo_FixedValues(clientUserId, clientDeviceId, clientInstanceId);
        assert(_localInfo);
        _localInfo->SetServerTcpDinAddrPort(serverUserId, serverDeviceId, serverInstanceId,
                                            serverAddr, serverPort);
        localInfo = _localInfo;
    }
#endif
    assert(localInfo);

    assert(!frontEnd);
    frontEnd = new Ts2::Network::FrontEnd(recvdPacketHandler, this, localInfo);
    assert(frontEnd);

    err = frontEnd->Start();
    assert(!err);

    LOG_INFO("Client: Start succeeded on Network::FrontEnd obj");

    return err;
}

int TcpDinClient::Enqueue(Ts2::Packet *packet)
{
    assert(frontEnd);
    int err = frontEnd->Enqueue(packet);
    assert(!err);
    return err;
}

int TcpDinClient::Stop()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = frontEnd->Stop();
    assert(!err);
    LOG_INFO("Client: Sent Stop to Network::FrontEnd obj");

    return err;
}

int TcpDinClient::WaitStopped()
{
    int err = 0;

    assert(frontEnd);
    err = frontEnd->WaitStopped();
    assert(!err);
    delete frontEnd;
    frontEnd = NULL;

    return err;
}

// class method
void TcpDinClient::recvdPacketHandler(Ts2::Packet *packet, void *context)
{
    assert(context);
    TcpDinClient *client = reinterpret_cast<TcpDinClient*>(context);
    client->recvdPacketHandler(packet);
}

void TcpDinClient::recvdPacketHandler(Ts2::Packet *packet)
{
    assert(packet);

    switch (packet->GetPktType()) {
    case Ts2::TS_PKT_ACK:
        {
            s32 error = (s32)*packet->pkt_body;
            LOG_INFO("Client: ACK received, window size="FMTu64", error="FMTs32, packet->GetWindowSize(), error);
        }
        hexdump((char*)packet->pkt_body, sizeof(s32));
        break;
    case Ts2::TS_PKT_DATA:
        {
            Ts2::PacketData* dataPkt = (Ts2::PacketData*)packet;
            LOG_INFO("Client: DATA received, len = %d: %s", (int)dataPkt->GetDataLen(), dataPkt->GetData());
            hexdump((char*)dataPkt->GetData(), dataPkt->GetDataLen());
        }
        break;
    default:
        LOG_ERROR("Echo server returns wrong type of packet, %d", packet->GetPktType());
        break;
    }
}
