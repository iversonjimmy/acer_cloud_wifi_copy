// TcpDinServer.hpp for Ts2::Network testing.

#ifndef __TCP_DIN_SERVER_HPP__
#define __TCP_DIN_SERVER_HPP__

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

#include <queue>
#include <string>

class TcpDinServer {
public:
#ifdef TS2_NO_PXD
    TcpDinServer(u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                 u64 clientUserId, u64 clientDeviceId, u32 clientInstanceId,
                 VPLNet_port_t portSpec);
#else
    TcpDinServer(const std::string &username, const std::string &password, VPLNet_port_t portSpec);
#endif // TS2_NO_PXD

    ~TcpDinServer();

    u64 GetUserId() const;
    u64 GetDeviceId() const;
    u32 GetInstanceId() const;

    int Start();
    int Stop();
    int WaitStopped();

    VPLNet_port_t GetPort() const;

private:
#ifndef TS2_NO_PXD
    const std::string username;
    const std::string password;
#else
    const u64 serverUserId;
    const u64 serverDeviceId;
    const u32 serverInstanceId;
    const u64 clientUserId;
    const u64 clientDeviceId;
    const u32 clientInstanceId;
#endif
    const VPLNet_port_t portSpec;

    mutable VPLMutex_t mutex;

    Ts2::LocalInfo *localInfo;
    Ts2::Network::FrontEnd *frontEnd;

    static void recvdPacketHandler(Ts2::Packet *packet,
                                   void *context);
    void recvdPacketHandler(Ts2::Packet *packet);
};

#endif // incl guard
