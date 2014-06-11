#ifndef DX_REMOTE_AGENT_MANAGER_H_
#define DX_REMOTE_AGENT_MANAGER_H_

#include "dx_remote_agent.h"
#include <vpl_socket.h>
#include <vpl_thread.h>
#include "dx_remote_agent.pb.h"

#include "DxRemoteCCDAgent.hpp"
#include "DxRemoteHTTPAgent.hpp"
#include "DxRemoteQueryDeviceAgent.hpp"
#include "DxRemoteOSAgent.hpp"
#include "DxRemoteMSAAgent.hpp"
#include "DxRemoteFileTransferAgent.hpp"
#include "DxRemoteTSTestAgent.hpp"

#include <memory>

class DxRemoteAgentManager
{
public:
    DxRemoteAgentManager(VPLSocket_t socket);
    ~DxRemoteAgentManager();
    void Run();
    friend class IDxRemoteAgent;
    friend class DxRemoteCCDAgent;
    friend class DxRemoteHTTPAgent;
    friend class DxRemoteOSAgent;
    friend class DxRemoteMSAAgent;
    friend class DxRemoteQueryDeviceAgent;
protected:
    int RunAgent(igware::dxshell::RequestType reqType);
    VPLSocket_t socket;
private:
    /// Get a protobuf RPC packet from the socket. Packet returned includes the length descriptors.
    /// @return Length of packet, or 0 on failure. The failure is logged internally.
    /// Caller is reponsible for delete[] of packet received.
    int GetProtoRPCPacket(char*& packet);

    /// Get a protobuf message length from the socket. raw_bytes will contain the original byte series.
    /// @return Protobuf length received from the socket, or error code on failure.
    int GetProtoLength(std::string& raw_bytes);

    /// Get a blob packet prefixed with a 4-byte length from the socket. 
    /// @return Length of packet, or 0 on failure. The failure is logged internally.
    /// Caller is reponsible for delete[] of packet received.
    int GetBlobPacket(char*&packet);
};

#endif
