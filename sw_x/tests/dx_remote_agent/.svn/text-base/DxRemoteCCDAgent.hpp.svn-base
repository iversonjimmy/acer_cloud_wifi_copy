#ifndef DX_REMOTE_CCD_AGENT_H_
#define DX_REMOTE_CCD_AGENT_H_

#include "IDxRemoteAgent.h"
#include "dx_remote_agent.pb.h"
#include "PrepareProtoBufSizeStream.hpp"

class DxRemoteCCDAgent : public IDxRemoteAgent
{
public:
    DxRemoteCCDAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    ~DxRemoteCCDAgent();
    int doAction();
    int SendFinalData() { return 0; };
protected:
    int handleProtoBuf(char *requestBuf, int size);
};

#endif
