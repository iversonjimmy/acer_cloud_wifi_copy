#ifndef DX_REMOTE_TS_TEST_AGENT_H_
#define DX_REMOTE_TS_TEST_AGENT_H_

#include "IDxRemoteAgent.h"
#include "dx_remote_agent.pb.h"

class DxRemoteTSTestAgent : public IDxRemoteAgent
{
public:
    DxRemoteTSTestAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    ~DxRemoteTSTestAgent();
    int doAction();
};

#endif
