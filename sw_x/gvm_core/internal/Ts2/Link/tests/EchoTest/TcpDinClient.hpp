// TcpDinClient.hpp for Ts2::Link testing.

#ifndef __TCP_DIN_CLIENT_HPP__
#define __TCP_DIN_CLIENT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <EndPoint.hpp>
#include <TcpDinConnHelper.hpp>
#include <Packet.hpp>
#include "LocalInfo_FixedValues.hpp"

class TcpDinClient {
public:
    TcpDinClient(const Ts2::Udi_t &localUdi, const Ts2::Udi_t &remoteUdi, VPLNet_port_t remotePort);
    ~TcpDinClient();

    int Connect();
    int WaitConnected();
    int Send(Ts2::Packet *packet);
    int Disconnect();
    int WaitDisconnected();

private:
    const Ts2::Udi_t localUdi;
    const Ts2::Udi_t remoteUdi;
    const VPLNet_port_t remotePort;

    mutable VPLMutex_t mutex;
    mutable VPLCond_t endPointReady_cond;

    Ts2::LocalInfo_FixedValues *localInfo;
    Ts2::Link::TcpDinConnHelper *connHelper;
    Ts2::Link::EndPoint *endPoint;

    static void connectedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType,
                                       Ts2::Link::ConnectHelper *connectHelper, 
                                       void *context);
    void connectedSocketHandler(VPLSocket_t socket, Ts2::Link::RouteType routeType,
                                Ts2::Link::ConnectHelper *connectHelper);

    static void recvdPacketHandler(Ts2::Packet *packet, 
                                   void *context);
    void recvdPacketHandler(Ts2::Packet *packet);
};

#endif // incl guard
