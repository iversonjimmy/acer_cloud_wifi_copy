#ifndef DX_REMOTE_MSA_AGENT_H_
#define DX_REMOTE_MSA_AGENT_H_

#include "IDxRemoteAgent.h"
#include "dx_remote_agent.pb.h"

class DxRemoteMSAAgent : public IDxRemoteAgent
{
public:
    DxRemoteMSAAgent(VPLSocket_t skt, char *buf, uint32_t pktSize);
    ~DxRemoteMSAAgent();
    int doAction();
protected:
    void msaBeginCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaCommitCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaBeginMetadataTransaction(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaUpdateMetadata(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaDeleteMetadata(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaCommitMetadataTransaction(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaGetMetadataSyncState(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaDeleteCollection(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaDeleteCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaEndCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaListCollections(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
    void msaGetCollectionDetails(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes);
};

#endif
