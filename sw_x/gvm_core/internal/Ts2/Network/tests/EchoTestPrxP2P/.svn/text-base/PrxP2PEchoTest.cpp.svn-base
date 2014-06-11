// PrxP2PEchoTest.cpp for Ts2::Network testing.

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include <getopt.h>

#include <vpl_thread.h>
#include <vplex_http2.hpp>

#include <log.h>

#include <Packet.hpp>

#include "PrxP2PServer.hpp"
#include "PrxP2PClient.hpp"

const u32 ServerVtunnelId = 10000;
const u32 ClientVtunnelId = 20000;

std::string username;
std::string password;

static u64 targetSvrUid = 0;
static u64 targetSvrDid = 0;
static std::string targetSvrIid = "";

static int delayStop = 5;

enum TEST_MODE {
    TEST_MODE_NONE = 0,
    TEST_MODE_CLIENT,
    TEST_MODE_SERVER,
    TEST_MODE_BOTH
} testMode = TEST_MODE_BOTH;

inline u64 stringTou64(const char * str)
{
    u64 res = 0;
    while (*str != '\0')
    {
        res *= 10 ;
        res += *str - '0';
        str++;
    }
    return res;
}

static void usage(int argc, char* argv[])
{
    printf("Usage: %s [options]\n", argv[0]);
    printf(" -u --username USERNAME     User name\n");
    printf(" -p --password PASSWORD     User password\n");
    printf(" -m --mode MODE             Mode\n");
    printf("    MODE:\n");
    printf("        client              Run program as client\n");
    printf("        server              Run program as server\n");
    printf("        all                 Run program as client and server\n");
    printf(" -t --target U,D,I          Required for client mode\n");
    printf(" -d --delay SEC             Seconds for delaying stop\n");
}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"mode", required_argument, 0, 'm'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"target", required_argument, 0, 't'},
        {"delay", required_argument, 0, 'd'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "m:u:p:t:d:",
                            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 'm':
        {
            std::string mode = optarg;
            if(mode == "client") testMode = TEST_MODE_CLIENT;
            else if(mode == "server") testMode = TEST_MODE_SERVER;
            else testMode = TEST_MODE_BOTH;
            break;
        }
        case 'u':
            username = optarg;
            break;

        case 'p':
            password = optarg;
            break;
        case 't':
        {
            char * pch;
            pch = strtok(optarg, ",");
            if(pch != NULL) {
                targetSvrUid = stringTou64(pch);
            }
            pch = strtok(NULL, ",");
            if(pch != NULL) {
                targetSvrDid = stringTou64(pch);
            }
            pch = strtok(NULL, ",");
            if(pch != NULL) {
                targetSvrIid = pch;
            }

            break;
        }
        case 'd':
            delayStop = atoi(optarg);
            break;
        default:
            usage(argc, argv);
            rv = -1;
            break;
        }
    }

    if(testMode == TEST_MODE_CLIENT &&
        (targetSvrUid == 0 ||
         targetSvrDid == 0 ||
         targetSvrIid == "")) {
        usage(argc, argv);
        rv = -1;
    }

    return rv;
}

int main(int argc, char *argv[])
{
    int err = 0;
    PrxP2PServer *server = NULL;
    PrxP2PClient *client = NULL;

    if (parse_args(argc, argv) != 0) {
        exit(0);
    }

    LOGInit("PrxP2PEchoTest", NULL);
    LOGSetMax(0); // No limit
    VPL_Init();
    VPLHttp2::Init();

    if(testMode & TEST_MODE_SERVER) {
        server = new PrxP2PServer(username, password);
        err = server->Start();
        assert(!err);
        targetSvrUid = server->GetUserId();
        targetSvrDid = server->GetDeviceId();
        targetSvrIid = server->GetInstanceId();
    }
    if(testMode & TEST_MODE_CLIENT) {
        client = new PrxP2PClient(username, password);
        err = client->Start();
        assert(!err);

        Ts2::Udi_t clientUdi, serverUdi;
        Compose_Udi_helper(clientUdi, client->GetUserId(), client->GetDeviceId(), "0");
        Compose_Udi_helper(serverUdi, targetSvrUid, targetSvrDid, "0");

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
            VPLThread_Sleep(VPLTime_FromMillisec(2000));
        }

        VPLThread_Sleep(VPLTime_FromSec(delayStop));

        err = client->Stop();
        assert(!err);
        client->WaitStopped();
        assert(!err);
        delete client;
    }

    if(testMode & TEST_MODE_SERVER) {

        cout << "SvrUserId: " << targetSvrUid <<
                ",SvrDeviceId: " << targetSvrDid <<
                ",SvrInstanceId: " << targetSvrIid << endl;

        VPLThread_Sleep(VPLTime_FromSec(delayStop));

        err = server->Stop();
        assert(!err);
        err = server->WaitStopped();
        assert(!err);
        delete server;
    }
}

