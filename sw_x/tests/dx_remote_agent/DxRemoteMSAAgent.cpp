#include "DxRemoteMSAAgent.hpp"
#include "log.h"
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#else
#include <ccdi.hpp>
#endif

#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#else
DxRemoteMSAAgent::DxRemoteMSAAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) : IDxRemoteAgent(skt, buf, pktSize)
{
}

DxRemoteMSAAgent::~DxRemoteMSAAgent()
{
}

int DxRemoteMSAAgent::doAction()
{
    int rv = 0;

    igware::dxshell::DxRemoteMSA cliReq, cliRes;
    cliReq.ParseFromArray(recvBuf, recvBufSize);
    igware::dxshell::DxRemoteMSA_Function myfunc = cliReq.func();
    switch (myfunc)
    {
    case igware::dxshell::DxRemoteMSA_Function_MSABeginCatalog:
        LOG_INFO("DxRemoteMSA_Function_MSABeginCatalog");
        msaBeginCatalog(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSADeleteCatalog:
        LOG_INFO("DxRemoteMSA_Function_MSADeleteCatalog");
        msaDeleteCatalog(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSAEndCatalog:
        LOG_INFO("DxRemoteMSA_Function_MSAEndCatalog");
        msaEndCatalog(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSACommitCatalog:
        LOG_INFO("DxRemoteMSA_Function_MSACommitCatalog");
        msaCommitCatalog(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSABeginMetadataTransaction:
        LOG_INFO("DxRemoteMSA_Function_MSABeginMetadataTransaction");
        msaBeginMetadataTransaction(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSAUpdateMetadata:
        LOG_INFO("DxRemoteMSA_Function_MSAUpdateMetadata");
        msaUpdateMetadata(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSADeleteMetadata:
        LOG_INFO("DxRemoteMSA_Function_MSADeleteMetadata");
        msaDeleteMetadata(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSACommitMetadataTransaction:
        LOG_INFO("DxRemoteMSA_Function_MSACommitMetadataTransaction");
        msaCommitMetadataTransaction(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSAGetMetadataSyncState:
        LOG_INFO("DxRemoteMSA_Function_MSAGetMetadataSyncState");
        msaGetMetadataSyncState(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSADeleteCollection:
        LOG_INFO("DxRemoteMSA_Function_MSADeleteCollection");
        msaDeleteCollection(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSAListCollections:
        LOG_INFO("DxRemoteMSA_Function_MSAListCollections");
        msaListCollections(cliReq, cliRes);
        break;
    case igware::dxshell::DxRemoteMSA_Function_MSAGetCollectionDetails:
        LOG_INFO("DxRemoteMSA_Function_MSAGetCollectionDetails");
        msaGetCollectionDetails(cliReq, cliRes);
        break;
    default:
        cliRes.set_func(myfunc);
        break;
    }

    LOG_INFO("Serialize Response for func: %d", cliRes.func());
    LOG_INFO("Serialize Response for func_return: %d", cliRes.func_return());
    response = cliRes.SerializeAsString();

    return rv;
}

void DxRemoteMSAAgent::msaBeginCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::BeginCatalogInput input;
    input.ParseFromString(myReq.msa_input());
    rv = CCDIMSABeginCatalog(input);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaCommitCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::CommitCatalogInput ccInput;
    ccInput.ParseFromString(myReq.msa_input());
    rv = CCDIMSACommitCatalog(ccInput);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaDeleteCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::DeleteCatalogInput deleteCatalogInput;
    deleteCatalogInput.ParseFromString(myReq.msa_input());
    rv = CCDIMSADeleteCatalog(deleteCatalogInput);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaEndCatalog(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::EndCatalogInput endCatalogInput;
    endCatalogInput.ParseFromString(myReq.msa_input());
    rv = CCDIMSAEndCatalog(endCatalogInput);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaBeginMetadataTransaction(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::BeginMetadataTransactionInput input;
    input.ParseFromString(myReq.msa_input());
    rv = CCDIMSABeginMetadataTransaction(input);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaUpdateMetadata(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::UpdateMetadataInput input;
    input.ParseFromString(myReq.msa_input());
    rv = CCDIMSAUpdateMetadata(input);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaDeleteMetadata(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::DeleteMetadataInput input;
    input.ParseFromString(myReq.msa_input());
    rv = CCDIMSADeleteMetadata(input);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaCommitMetadataTransaction(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = CCDIMSACommitMetadataTransaction();
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}

void DxRemoteMSAAgent::msaGetMetadataSyncState(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    media_metadata::GetMetadataSyncStateOutput getMetaSyncStatOutput;
    rv = CCDIMSAGetMetadataSyncState(getMetaSyncStatOutput);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
    myRes.set_msa_output(getMetaSyncStatOutput.SerializeAsString());
}

void DxRemoteMSAAgent::msaListCollections(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    media_metadata::ListCollectionsOutput listColOutput;
    rv = CCDIMSAListCollections(listColOutput);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
    myRes.set_msa_output(listColOutput.SerializeAsString());
}

void DxRemoteMSAAgent::msaGetCollectionDetails(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::GetCollectionDetailsInput getCollectionInput;
    ccd::GetCollectionDetailsOutput getCollectionOutput;
    getCollectionInput.ParseFromString(myReq.msa_input());
    rv = CCDIMSAGetCollectionDetails(getCollectionInput, getCollectionOutput);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
    myRes.set_msa_output(getCollectionOutput.SerializeAsString());
}

void DxRemoteMSAAgent::msaDeleteCollection(igware::dxshell::DxRemoteMSA &myReq, igware::dxshell::DxRemoteMSA &myRes)
{
    int rv = 0;
    ccd::DeleteCollectionInput deleteCollectionInput;
    deleteCollectionInput.ParseFromString(myReq.msa_input());
    rv = CCDIMSADeleteCollection(deleteCollectionInput);
    myRes.set_func(myReq.func());
    myRes.set_func_return(rv);
}
#endif
