// TcpDinClient.hpp for Ts2::Network testing.

#ifndef __TCP_DIN_CLIENT_HPP__
#define __TCP_DIN_CLIENT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <FrontEnd.hpp>
#include <Packet.hpp>
#ifndef TS2_NO_PXD
#include "LocalInfo_InfraHelper.hpp"
#else
#include "LocalInfo_FixedValues.hpp"
#endif

class TcpDinClient {
public:
#ifndef TS2_NO_PXD
    TcpDinClient(const std::string &username, const std::string &password, u64 serverDeviceId, VPLNet_addr_t serverAddr, VPLNet_port_t serverPort);
#else
    TcpDinClient(u64 clientUserId, u64 clientDeviceId, u32 clientInstanceId,
                 u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                 VPLNet_addr_t serverAddr, VPLNet_port_t serverPort);
#endif
    ~TcpDinClient();

    u64 GetUserId() const;
    u64 GetDeviceId() const;
    u32 GetInstanceId() const;

    int Start();
    int Enqueue(Ts2::Packet *packet);
    int Stop();
    int WaitStopped();

private:
#ifndef TS2_NO_PXD
    const std::string &username;
    const std::string &password;
    u64 serverDeviceId;
#else
    const u64 clientUserId;
    const u64 clientDeviceId;
    const u32 clientInstanceId;
    const u64 serverUserId;
    const u64 serverDeviceId;
    const u32 serverInstanceId;
#endif
    const VPLNet_addr_t serverAddr;
    const VPLNet_port_t serverPort;

    mutable VPLMutex_t mutex;

    Ts2::LocalInfo *localInfo;
    Ts2::Network::FrontEnd *frontEnd;

    static void recvdPacketHandler(Ts2::Packet *packet, 
                                   void *context);
    void recvdPacketHandler(Ts2::Packet *packet);
};

#endif // incl guard
