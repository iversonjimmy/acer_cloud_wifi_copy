// PrxP2PServer.cpp for Ts2::Network testing.

#include "PrxP2PServer.hpp"

#include <gvm_thread_utils.h>
#include <log.h>

#include <vplu_mutex_autolock.hpp>

#include <cassert>

bool PrxP2PServer::ansLoginDone = false;

Ts2::LocalInfo* PrxP2PServer::svrlocalInfo = NULL;

PrxP2PServer::PrxP2PServer(const std::string &username, const std::string &password)
    : username(username), password(password),
      localInfo(NULL), frontEnd(NULL), ansClient(NULL)
{
    VPLMutex_Init(&mutex);
}

PrxP2PServer::~PrxP2PServer()
{
    assert(!frontEnd);

    VPLMutex_Destroy(&mutex);

    LOG_INFO("Server: Destroyed");
}

u64 PrxP2PServer::GetUserId() const
{
    return localInfo->GetUserId();
}

u64 PrxP2PServer::GetDeviceId() const
{
    return localInfo->GetDeviceId();
}

u32 PrxP2PServer::GetInstanceId() const
{
    return localInfo->GetInstanceId();
}

int PrxP2PServer::Start()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    assert(!localInfo);
    {
        Ts2::LocalInfo_InfraHelper *_localInfo = new Ts2::LocalInfo_InfraHelper(username, password, "Network::EchoTest::Server");
        assert(_localInfo);
        localInfo = _localInfo;
        svrlocalInfo = localInfo;
    }
    assert(localInfo);

    static ans_callbacks_t  ansCallbacks =
        {
            connectionActive,
            receiveNotification,
            receiveSleepInfo,
            receiveDeviceState,
            connectionDown,
            connectionClosed,
            setPingPacket,
            rejectCredentials,
            loginCompleted,
            rejectSubscriptions,
            receiveResponse
        };

    // ANS init
    // get ans blob and session key
    std::string ansSessionKey;
    std::string ansLoginBlob;
    ans_open_t ansInput;
    static_cast<Ts2::LocalInfo_InfraHelper*>(localInfo)->GetAnsSessionKey(ansSessionKey, ansLoginBlob);

    ansInput.clusterName   = localInfo->GetAnsSvrName().c_str();
    ansInput.deviceType    = "default";
    ansInput.application   = (char*)"0";  // FIXME
    ansInput.verbose       = true;
    ansInput.callbacks     = &ansCallbacks;

    ansInput.blob = (void*)ansLoginBlob.data();
    ansInput.blobLength  = static_cast<int>(ansLoginBlob.size());
    ansInput.key  = (void*)ansSessionKey.data();
    ansInput.keyLength   = static_cast<int>(ansSessionKey.size());

    ansClient = ans_open(&ansInput);

    // wait for login done
    LOG_INFO("Server: Wait for ANS login done");
    {
        int retry = 15;
        while (!PrxP2PServer::ansLoginDone) {
            VPLThread_Sleep(VPLTime_FromSec(1));
            if (retry-- < 0) {
                err = -1;
            }
        }
    }
    LOG_INFO("Server: ANS login done");
    assert(!err);

    assert(!frontEnd);
    frontEnd = new Ts2::Network::FrontEnd(recvdPacketHandler, this, VPLNET_ADDR_ANY, VPLNET_PORT_ANY, localInfo);
    assert(frontEnd);

    err = frontEnd->Start();
    assert(!err);

    LOG_INFO("Server: Start succeeded on Network::FrontEnd obj");

    return err;
}

int PrxP2PServer::Stop()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        assert(frontEnd);
        err = frontEnd->Stop();
        assert(!err);
        LOG_INFO("Server: Sent Stop to Network::FrontEnd");
        if(ansClient != NULL) {
            ans_close(ansClient, VPL_FALSE);
            ansClient = NULL;
        }
    }

    return err;
}

int PrxP2PServer::WaitStopped()
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

// class method
void PrxP2PServer::recvdPacketHandler(Ts2::Packet *packet, void *context)
{
    assert(context);
    PrxP2PServer *server = reinterpret_cast<PrxP2PServer*>(context);
    server->recvdPacketHandler(packet);
}

void PrxP2PServer::recvdPacketHandler(Ts2::Packet *packet)
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

VPL_BOOL PrxP2PServer::receiveNotification(ans_client_t *client, ans_unpacked_t *unpacked)
{
    if (unpacked->notificationLength < 1) {
        LOG_ERROR("Ignoring empty notification");
        return VPL_TRUE;
    }

    u8 type = ((u8*)unpacked->notification)[0];

    LOG_INFO("Server: notification type: "FMTu8, type);
    const void* payload = VPLPtr_AddUnsigned(unpacked->notification, 1);
    u32 payloadLen = unpacked->notificationLength - 1;

    if (type == 4/* ANS_TYPE_CLIENT_CONN*/) {

        LOG_INFO("Server: client is trying to connect");
        PrxP2PServer::svrlocalInfo->AnsNotifyIncomingClient((char*)payload, payloadLen);
    }

    return true;
}

void PrxP2PServer::loginCompleted(ans_client_t *client)
{
    // No call context could be referenced
    PrxP2PServer::ansLoginDone = true;
}
