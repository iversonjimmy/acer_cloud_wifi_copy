#ifndef DX_REMOTE_QUERY_DEVICE_AGENT_H_
#define DX_REMOTE_QUERY_DEVICE_AGENT_H_

#include "IDxRemoteAgent.h"
#include <string>

class DxRemoteQueryDeviceAgent : public IDxRemoteAgent
{
public:
    DxRemoteQueryDeviceAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    ~DxRemoteQueryDeviceAgent();
    int doAction();
protected:
    std::string GetDeviceName();
    std::string GetDeviceClass();
    std::string GetOSVersion();
    bool GetIsAcer();
    bool GetHasCamera();
};

#endif
