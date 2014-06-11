#ifndef _REMOTE_AGENT_HPP_
#define _REMOTE_AGENT_HPP_

#include <vplu_types.h>
#include <vplu_common.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <string>

// TODO: This is actually a client stub for talking to a Remote Agent.  It is not an actual
//   "Remote Agent".  Suggest renaming to something like "RemoteAgentClient" for clarity.
/// This is a client that will connect to a "Remote Agent" agent running at the specified
/// IP address and port.
class RemoteAgent{
public:
    RemoteAgent(VPLNet_addr_t ipaddr, u16 port);
    ~RemoteAgent();
    int checkSocketConnect(void);
    int send(int command, const std::string &input, std::string &output);
private:
    VPLNet_addr_t ipaddr;
    u16 port;
    VPL_DISABLE_COPY_AND_ASSIGN(RemoteAgent);

    VPLSocket_t socket;
    int sock_err;
    int connectAgentSocket(void);
    int sendAgentSocket(const void* buf, size_t len);
    int recvAgentSocket(void* buf, size_t len);
};

#endif // _REMOTE_AGENT_HPP_
