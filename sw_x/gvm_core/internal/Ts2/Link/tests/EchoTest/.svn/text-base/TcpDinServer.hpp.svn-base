// TcpDinServer.hpp for Ts2::Link testing.

#ifndef __TCP_DIN_SERVER_HPP__
#define __TCP_DIN_SERVER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <EndPoint.hpp>
#include <TcpDinAcceptHelper.hpp>
#include <Packet.hpp>
#include "LocalInfo_FixedValues.hpp"

#include <queue>
#include <string>

class TcpDinServer {
public:
    TcpDinServer(const Ts2::Udi_t &localUdi, const Ts2::Udi_t &remoteUdi, VPLNet_port_t portSpec);
    ~TcpDinServer();

    int Start();
    int Stop();
    int WaitStopped();

    VPLNet_port_t GetPort() const;

private:
    const Ts2::Udi_t localUdi;
    const Ts2::Udi_t remoteUdi;
    const VPLNet_port_t portSpec;

    mutable VPLMutex_t mutex;
    mutable VPLCond_t endPointReady_cond;
    mutable VPLCond_t qNonEmpty_cond;

    Ts2::LocalInfo_FixedValues *localInfo;
    VPLSocket_t listenSocket;
    Ts2::Link::TcpDinAcceptHelper *acceptHelper;
    Ts2::Link::EndPoint *endPoint;

    VPLDetachableThreadHandle_t thread;
    static VPLTHREAD_FN_DECL threadMain(void *param);
    void threadMain();

    static void acceptedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType,
                                      u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                      Ts2::Link::AcceptHelper *acceptHelper,
                                      void *context);
    void acceptedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType,
                               u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                               Ts2::Link::AcceptHelper *acceptHelper);

    static void recvPacketHandler(Ts2::Packet *packet,
                                  void *context);
    void recvPacketHandler(Ts2::Packet *packet);

    void waitConnectionAttempt();
    void acceptConnectionAttemptAsync();
    void waitEndPointReady();
    void processPackets();

    std::queue<Ts2::Packet*> packetQ;
    void enqueue(Ts2::Packet *packet);

};

#endif // incl guard
