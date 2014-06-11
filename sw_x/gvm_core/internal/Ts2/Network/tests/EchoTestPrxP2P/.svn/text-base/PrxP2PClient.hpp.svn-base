// PrxP2PClient.hpp for Ts2::Network testing.

#ifndef __PRXP2P_CLIENT_HPP__
#define __PRXP2P_CLIENT_HPP__

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

class PrxP2PClient {
public:
    PrxP2PClient(const std::string &username, const std::string &password);
    ~PrxP2PClient();

    u64 GetUserId() const;
    u64 GetDeviceId() const;
    u32 GetInstanceId() const;

    int Start();
    int Enqueue(Ts2::Packet *packet);
    int Stop();
    int WaitStopped();

private:
    const std::string &username;
    const std::string &password;

    mutable VPLMutex_t mutex;

    Ts2::LocalInfo *localInfo;
    Ts2::Network::FrontEnd *frontEnd;

    static void recvdPacketHandler(Ts2::Packet *packet,
                                   void *context);
    void recvdPacketHandler(Ts2::Packet *packet);
};

#endif // incl guard
