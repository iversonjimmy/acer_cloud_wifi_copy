#ifndef DX_REMOTE_HTTP_AGENT_H_
#define DX_REMOTE_HTTP_AGENT_H_

#include "IDxRemoteAgent.h"
#include "dx_remote_agent.pb.h"
#include "vplu_types.h"
#include <string>

class DxRemoteHTTPAgent : public IDxRemoteAgent
{
public:
    DxRemoteHTTPAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    ~DxRemoteHTTPAgent();
    int doAction();
protected:
    igware::dxshell::ErrorCode handler(igware::dxshell::HttpGetInput &httpInput, igware::dxshell::HttpGetOutput &httpOutput);
};

#endif
