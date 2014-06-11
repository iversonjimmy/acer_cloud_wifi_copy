#ifndef I_DX_REMOTE_AGENT_H_
#define I_DX_REMOTE_AGENT_H_

#include "dx_remote_agent.h"
#include <vpl_socket.h>
#include <string>

class IDxRemoteAgent
{
public:
    IDxRemoteAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    virtual ~IDxRemoteAgent();
    virtual int doAction() = 0;
    virtual int SendFinalData();
protected:
    VPLSocket_t clienttcpsocket;
    char *recvBuf;
    uint32_t recvBufSize;

    std::string response;
    uint32_t resSize;
    uint32_t sentSize;
};

#endif
