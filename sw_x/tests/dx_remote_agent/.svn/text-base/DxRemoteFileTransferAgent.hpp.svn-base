#ifndef DX_REMOTE_FILE_TRANSFER_AGENT_H_
#define DX_REMOTE_FILE_TRANSFER_AGENT_H_

#include "IDxRemoteAgent.h"
#include "dx_remote_agent.pb.h"

class DxRemoteFileTransferAgent :  public IDxRemoteAgent
{
public:
    DxRemoteFileTransferAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    ~DxRemoteFileTransferAgent();
    int doAction();
    int SendFinalData() { return 0; };
protected:
    int PushFile(igware::dxshell::DxRemoteFileTransfer &myReq, igware::dxshell::DxRemoteFileTransfer &myRes);
    int GetFile(igware::dxshell::DxRemoteFileTransfer &myReq, igware::dxshell::DxRemoteFileTransfer &myRes);

    int SendProtoSize(uint32_t resSize);
    int SendProtoResponse(igware::dxshell::DxRemoteFileTransfer &myRes);
};

#endif
