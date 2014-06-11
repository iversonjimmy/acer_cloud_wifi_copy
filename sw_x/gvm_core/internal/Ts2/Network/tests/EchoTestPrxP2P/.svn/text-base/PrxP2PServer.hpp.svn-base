// PrxP2PServer.hpp for Ts2::Network testing.

#ifndef __PRXP2P_SERVER_HPP__
#define __PRXP2P_SERVER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <FrontEnd.hpp>
#include <Packet.hpp>
#include "LocalInfo_InfraHelper.hpp"
#include "ans_device.h"

#include <queue>
#include <string>

class PrxP2PServer {
public:

    PrxP2PServer(const std::string &username, const std::string &password);
    ~PrxP2PServer();

    u64 GetUserId() const;
    u64 GetDeviceId() const;
    u32 GetInstanceId() const;

    int Start();
    int Stop();
    int WaitStopped();

private:
    const std::string username;
    const std::string password;

    mutable VPLMutex_t mutex;

    static Ts2::LocalInfo *svrlocalInfo; // For testing purpose, we have only one server
    Ts2::LocalInfo *localInfo;
    Ts2::Network::FrontEnd *frontEnd;

    ans_client_t *ansClient;
    // Not much we could do for this static boolean
    // Since we don't care so much about ANS, we just make this as static.
    static bool ansLoginDone;

    static void recvdPacketHandler(Ts2::Packet *packet,
                                   void *context);
    void recvdPacketHandler(Ts2::Packet *packet);

    static VPL_BOOL connectionActive(ans_client_t *client, VPLNet_addr_t local_addr){return true;};
    static VPL_BOOL receiveNotification(ans_client_t *client, ans_unpacked_t *unpacked);
    static void receiveSleepInfo(ans_client_t *client, ans_unpacked_t *unpacked){};
    static void receiveDeviceState(ans_client_t *client, ans_unpacked_t *unpacked){};
    static void connectionDown(ans_client_t *client){};
    static void connectionClosed(ans_client_t *client){};
    static void setPingPacket(ans_client_t *client, char *packet, int length){};
    static void rejectCredentials(ans_client_t *client){};
    static void loginCompleted(ans_client_t *client);
    static void rejectSubscriptions(ans_client_t *client){};
    static void receiveResponse(ans_client_t *client, ans_unpacked_t *response){};
};

#endif // incl guard
