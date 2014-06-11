// TcpDinEchoTest.cpp for Ts2::Network testing.

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include <vpl_thread.h>
#include <vplex_http2.hpp>

#include <log.h>

#include <Packet.hpp>

#include "TcpDinServer.hpp"
#include "TcpDinClient.hpp"

#ifdef TS2_NO_PXD
const u64 UserId = 100;
const u64 FirstServerDeviceId = 1000;
const u64 FirstClientDeviceId = 2000;
const u32 ServerInstanceId = 0;
const u32 ClientInstanceId = 0;
#endif
const u32 ServerVtunnelId = 10000;
const u32 ClientVtunnelId = 20000;

int main(int argc, char *argv[])
{
    int err = 0;

#ifndef TS2_NO_PXD
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " username password" << std::endl;
        exit(0);
    }
    std::string username(argv[1]);
    std::string password(argv[2]);
#else
#endif    

    LOGInit("TcpDinEchoTest", NULL);
    LOGSetMax(0); // No limit
    VPL_Init();
    VPLHttp2::Init();

#ifdef TS2_NO_PXD
    TcpDinServer *server = new TcpDinServer(UserId, FirstServerDeviceId, ServerInstanceId,
                                            UserId, FirstClientDeviceId, ClientInstanceId,
                                            VPLNET_PORT_ANY);
#else
    TcpDinServer *server = new TcpDinServer(username, password, VPLNET_PORT_ANY);
#endif
    err = server->Start();
    assert(!err);

#ifdef TS2_NO_PXD
    TcpDinClient *client = new TcpDinClient(UserId, FirstClientDeviceId, ClientInstanceId,
                                            UserId, FirstServerDeviceId, ServerInstanceId,
                                            VPLNET_ADDR_LOOPBACK, server->GetPort());
#else
    //VPLNet_addr_t serverAddr = VPLNet_GetAddr("10.0.20.146");
    VPLNet_addr_t serverAddr = VPLNET_ADDR_LOOPBACK;
    TcpDinClient *client = new TcpDinClient(username, password, server->GetDeviceId(), serverAddr, server->GetPort());
#endif
    err = client->Start();
    assert(!err);

    Ts2::Udi_t clientUdi, serverUdi;
    Compose_Udi_helper(clientUdi, client->GetUserId(), client->GetDeviceId(), "0");
    Compose_Udi_helper(serverUdi, server->GetUserId(), server->GetDeviceId(), "0");

    for (int i = 0; i < 10; i++) {
        std::ostringstream oss;
        for (int j = 0; j < 2+i; j++) {
            oss.put('0'+i);
        }
        oss.put('\0');
        std::string msg(oss.str());
        char *msgbuf = new char [msg.size()];
        memcpy(msgbuf, msg.data(), msg.size());
        Ts2::PacketData *packet = new Ts2::PacketData(clientUdi, ClientVtunnelId,
                                                      serverUdi, ServerVtunnelId,
                                                      0, 0, 
                                                      (const void*)msgbuf,
                                                      (const u32)msg.size(), true);
        msgbuf = NULL;  // ownership transferred to PacketData obj
        int err = client->Enqueue(packet);
        assert(!err);
        //VPLThread_Sleep(VPLTime_FromMillisec(100));
    }

    for (int i = 100; i<110; i++) {
        LOG_INFO("client send ACK msg, window size=%d, error=%d", 1024+i, i);
        Ts2::PacketAck *packet = new Ts2::PacketAck(clientUdi, ClientVtunnelId,
                                                    serverUdi, ServerVtunnelId,
                                                    0, 0, 1024+i, i);
        int err = client->Enqueue(packet);
        assert(!err);
        //VPLThread_Sleep(VPLTime_FromMillisec(100)); 
    }

    VPLThread_Sleep(VPLTime_FromSec(5));

    err = client->Stop();
    assert(!err);
    client->WaitStopped();
    assert(!err);
    delete client;

    VPLThread_Sleep(VPLTime_FromSec(2));

    err = server->Stop();
    assert(!err);
    err = server->WaitStopped();
    assert(!err);
    delete server;
}

