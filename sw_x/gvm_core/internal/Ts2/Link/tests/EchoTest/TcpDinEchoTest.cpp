// This test program depends on use of tmpPacket.

#include <cassert>
#include <sstream>
#include <string>

#include <vpl_thread.h>

#include <log.h>

#include <Packet.hpp>

#include "TcpDinServer.hpp"
#include "TcpDinClient.hpp"

const u64 UserId = 100;
const u64 FirstServerDeviceId = 1000;
const u64 FirstClientDeviceId = 2000;
const std::string ServerInstanceId = "0";
const std::string ClientInstanceId = "0";
const u32 ServerVtunnelId = 10000;
const u32 ClientVtunnelId = 20000;

int main(int argc, char *argv[])
{
    int err = 0;

    LOGInit("TcpDinEchoTest", NULL);
    LOGSetMax(0); // No limit
    VPL_Init();

    Ts2::Udi_t clientUdi, serverUdi;
    Compose_Udi_helper(clientUdi, UserId, FirstClientDeviceId, ClientInstanceId.c_str());
    Compose_Udi_helper(serverUdi, UserId, FirstServerDeviceId, ServerInstanceId.c_str());

    TcpDinServer *server = new TcpDinServer(serverUdi, clientUdi, VPLNET_PORT_ANY);
    err = server->Start();
    assert(!err);

    TcpDinClient *client = new TcpDinClient(clientUdi, serverUdi, server->GetPort());
    err = client->Connect();
    assert(!err);
    err = client->WaitConnected();
    assert(!err);

    for (int i = 0; i < 10; i++) {
        std::ostringstream oss;
        for (int j = 0; j < 2+i; j++) {
            oss.put('0'+i);
        }
        oss.put('\0');
        std::string msg(oss.str());
        char *msgbuf = new char [msg.size()];
        memcpy(msgbuf, msg.data(), msg.size());
        LOG_INFO("client send msg, len=%d: %s", msg.size(), msgbuf);
        Ts2::PacketData *packet = new Ts2::PacketData(clientUdi, ClientVtunnelId,
                                                      serverUdi, ServerVtunnelId,
                                                      0, 0, 
                                                      (const void*)msgbuf,
                                                      (const u32)msg.size(), true);
        msgbuf = NULL;  // ownership transferred to PacketData obj
        int err = client->Send(packet);
        assert(!err);
        delete packet;
        //VPLThread_Sleep(VPLTime_FromMillisec(100));
    }

    for (int i = 100; i<110; i++) {
        LOG_INFO("client send ACK msg, window size=%d, error=%d", 1024+i, i);
        Ts2::PacketAck *packet = new Ts2::PacketAck(clientUdi, ClientVtunnelId,
                                                    serverUdi, ServerVtunnelId,
                                                    0, 0, 1024+i, i);
        int err = client->Send(packet);
        assert(!err);
        delete packet;
        //VPLThread_Sleep(VPLTime_FromMillisec(100)); 
    }

    VPLThread_Sleep(VPLTime_FromSec(2));

    err = client->Disconnect();
    assert(!err);
    client->WaitDisconnected();
    assert(!err);
    delete client;

    VPLThread_Sleep(VPLTime_FromSec(2));

    err = server->Stop();
    assert(!err);
    err = server->WaitStopped();
    assert(!err);
    delete server;
}

