// TcpDinClient.cpp for Ts2::Link testing.

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

TcpDinClient::TcpDinClient(const Ts2::Udi_t &localUdi, const Ts2::Udi_t &remoteUdi, VPLNet_port_t remotePort)
    : localUdi(localUdi), remoteUdi(remoteUdi), remotePort(remotePort),
      localInfo(NULL), connHelper(NULL), endPoint(NULL)
{
    VPLMutex_Init(&mutex);
    VPLCond_Init(&endPointReady_cond);
}

TcpDinClient::~TcpDinClient()
{
    if (endPoint) {
        LOG_WARN("EndPoint still exists");
        delete endPoint;
    }
    if (connHelper) {
        LOG_WARN("ConnHelper still exists");
        delete connHelper;
    }

    VPLCond_Destroy(&endPointReady_cond);
    VPLMutex_Destroy(&mutex);

    LOG_INFO("Client: Destroyed");
}

int TcpDinClient::Connect()
{
    MutexAutoLock lock(&mutex);

    int err = 0;

    assert(!localInfo);
    localInfo = new Ts2::LocalInfo_FixedValues(localUdi.userId, localUdi.deviceId, /*instanceId*/0);
    assert(localInfo);
    localInfo->SetServerTcpDinAddrPort(remoteUdi.userId, remoteUdi.deviceId, /*instanceId*/0,
                                       VPLNET_ADDR_LOOPBACK, remotePort);

    assert(connHelper == NULL);
    connHelper = new Ts2::Link::TcpDinConnHelper(remoteUdi.userId, remoteUdi.deviceId, /*instanceId*/0, connectedSocketHandler, this, localInfo);
    err = connHelper->Connect();
    assert(!err);
    LOG_INFO("Client: Connecting..");

    return err;
}

int TcpDinClient::WaitConnected()
{
    MutexAutoLock lock(&mutex);
    while (endPoint == NULL) {
        int err = VPLCond_TimedWait(&endPointReady_cond, &mutex, VPL_TIMEOUT_NONE);
        assert(!err);
    }
    LOG_INFO("Client: Connected");

    if (connHelper) {
        delete connHelper;
        LOG_INFO("Client: Destroyed ConnHelper");
        connHelper = NULL;
    }

    return 0;
}

int TcpDinClient::Send(Ts2::Packet *packet)
{
    int err = endPoint->Send(packet);
    assert(!err);
    LOG_INFO("Client: Sent Packet");
    return err;
}

int TcpDinClient::Disconnect()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = endPoint->Stop();
    assert(!err);
    LOG_INFO("Client: Sent Stop to EndPoint");

    return err;
}

int TcpDinClient::WaitDisconnected()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    err = endPoint->WaitStopped();
    assert(!err);
    LOG_INFO("Client: EndPoint stopped");

    delete endPoint;
    LOG_INFO("Client: Destroyed EndPoint");
    endPoint = NULL;

    return err;
}

// class method
void TcpDinClient::connectedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType, Ts2::Link::ConnectHelper *connectHelper, void *context)
{
    assert(context);
    TcpDinClient *client = reinterpret_cast<TcpDinClient*>(context);
    client->connectedSocketHandler(socket, routeType, connectHelper);
}

void TcpDinClient::connectedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType, Ts2::Link::ConnectHelper *connectHelper)
{
    // ConnectedSocketHandler() could be called multiple times, to deliver possibly multiple connected sockets (e.g., PRX/P2P case).
    // When all connected sockets are delivered, there will be one more call, indicated by ROUTE_TYPE_INVALID.

    MutexAutoLock lock(&mutex);

    if (routeType != Ts2::Link::ROUTE_TYPE_INVALID) {
        LOG_INFO("Client: Received a connected socket");
        assert(endPoint == NULL);
        endPoint = new Ts2::Link::EndPoint(socket, remoteUdi.userId, remoteUdi.deviceId, /*instanceId*/0, recvdPacketHandler, this, Ts2::Link::ROUTE_TYPE_DIN, localInfo);
        int err = endPoint->Start();
        assert(!err);
    }
    else {
        LOG_INFO("Client: No more connected sockets");
        VPLCond_Signal(&endPointReady_cond);
    }
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

    // Assume data is NUL-terminated printable string.
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
