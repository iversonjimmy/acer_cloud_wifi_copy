//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "ccdi_client_base.h"

#include "ccdi.hpp"
#include "ccdi_client.hpp"
#include "ccdi_client_named_socket.hpp"
#include "ccdi_client_tcp.hpp"
#include "ccdi_rpc-client.pb.h"
#include "FileProtoChannel.h"
#include "vpl_socket.h"
#include "vplex_named_socket.h"
#include "vplex_socket.h"
#include "build_info.h"

#ifdef _WIN32
#include <io.h>
#endif

#include <string>

using namespace ccd;

namespace ccdi {
namespace client {

//-----------------------
// Shared across all threads
//-----------------------
static bool s_useTcpSockAddr = false;

// local named-socket:
static char ccdiSocketName[CCDI_PROTOBUF_SOCKET_NAME_MAX_LENGTH] = "";
int s_ccdSockTestInstanceNum = 0; // This should be static, but making it static apparently triggers a
    // compiler bug in our mingw build where the value is initialized to be non-zero.
static std::string s_ccdSockOsUserId;
static VPLThread__once_t init_socketname_control = VPLTHREAD_ONCE_INIT;

// TCP socket:
static VPLSocket_addr_t s_tcpSockAddr;

//-----------------------
// Thread-specific
//-----------------------
static VPL_THREAD_LOCAL VPLNamedSocketClient_t tl_clientNamedSock;
static VPL_THREAD_LOCAL VPLSocket_t tl_clientTcpSock;
static VPL_THREAD_LOCAL VPL_BOOL tl_clientSockConnected = VPL_FALSE;
static VPL_THREAD_LOCAL FileProtoChannel* tl_channel = NULL;
static VPL_THREAD_LOCAL VPLSocket_addr_t tl_tcpSockAddr;
static VPL_THREAD_LOCAL VPL_BOOL tl_useTcpSockAddr = VPL_FALSE;
//-----------------------

static void privUpdateSocketName(void)
{
    if (s_ccdSockOsUserId.empty()) {
        char* user_id;
        int rv = VPL_GetOSUserId(&user_id);
        if (rv < 0) {
            LOG_ERROR("Failed to get user id, rv = %d", rv);
            return;
        }
        s_ccdSockOsUserId.assign(user_id);
        VPL_ReleaseOSUserId(user_id);
    }
    VPL_snprintf(ccdiSocketName, ARRAY_SIZE_IN_BYTES(ccdiSocketName),
            CCDI_PROTOBUF_SOCKET_NAME_FMT, s_ccdSockTestInstanceNum, s_ccdSockOsUserId.c_str());
    LOG_INFO("Now using socket name \"%s\"", ccdiSocketName);
}

static void
privCcdiClientInit(void)
{
    int rv;
    rv = VPL_Init();
    if ((rv != VPL_OK) && (rv != VPL_ERR_IS_INIT)) {
        LOG_ERROR("%s failed: %d", "VPL_Init", rv);
        return;
    }
    LOG_INFO("BUILD: %s", BUILD_INFO);
    privUpdateSocketName();
}

static int
init_protobufSockName()
{
    return VPLThread_Once(&init_socketname_control, privCcdiClientInit);
}

void CCDIClient_SetTestInstanceNum(int testInstanceNum)
{
    LOG_INFO("CCDIClient_SetTestInstanceNum(%d)", testInstanceNum);
    s_ccdSockTestInstanceNum = testInstanceNum;
    s_useTcpSockAddr = false;
    if (ccdiSocketName[0] == '\0') {
        init_protobufSockName();
    } else {
        privUpdateSocketName();
    }
}

void CCDIClient_SetOsUserId(const char* osUserId)
{
    LOG_INFO("CCDIClient_SetOsUserId(%s)", osUserId);
    s_ccdSockOsUserId.assign(osUserId);
    s_useTcpSockAddr = false;
    if (ccdiSocketName[0] == '\0') {
        init_protobufSockName();
    } else {
        privUpdateSocketName();
    }
}

void CCDIClient_SetRemoteSocket(const VPLSocket_addr_t& sockAddr)
{
    LOG_INFO("CCDIClient_SetRemoteSocket("FMT_VPLNet_addr_t":"FMT_VPLNet_port_t")",
            VAL_VPLNet_addr_t(sockAddr.addr), VPLNet_port_ntoh(sockAddr.port));
    s_tcpSockAddr = sockAddr;
    s_useTcpSockAddr = true;
}

void CCDIClient_SetRemoteSocketForThread(const VPLSocket_addr_t& sockAddr)
{
    LOG_INFO("CCDIClient_SetRemoteSocketForThread("FMT_VPLNet_addr_t":"FMT_VPLNet_port_t")",
            VAL_VPLNet_addr_t(sockAddr.addr), VPLNet_port_ntoh(sockAddr.port));
    tl_tcpSockAddr = sockAddr;
    tl_useTcpSockAddr = VPL_TRUE;
}

static void
_releaseClientCommon()
{
    delete tl_channel;
    tl_channel = NULL;
    
    if (tl_clientSockConnected) {
        if (tl_useTcpSockAddr || s_useTcpSockAddr) {
            int rv = VPLSocket_Close(tl_clientTcpSock);
            if (rv < 0) {
                LOG_ERROR("VPLSocket_Close returned %d", rv);
                // log, but don't abort
            }
        } else {
            int rv = VPLNamedSocketClient_Close(&tl_clientNamedSock);
            if (rv < 0) {
                LOG_ERROR("VPLNamedSocketClient_Close returned %d", rv);
                // log, but don't abort
            }
        }
        tl_clientSockConnected = VPL_FALSE;
    }
}

static void
releaseSyncServiceClient(CCDIServiceClient* serviceClient)
{
    delete serviceClient;
    _releaseClientCommon();
}

static int
ccdiNewTcpSocket(VPLSocket_t* sock)
{
    int rv;
    const VPLSocket_addr_t& sockAddr = tl_useTcpSockAddr ? tl_tcpSockAddr : s_tcpSockAddr;
    *sock = VPLSocket_Create(sockAddr.family, VPLSOCKET_STREAM, VPL_FALSE);
    if (VPLSocket_Equal(*sock, VPLSOCKET_INVALID)) {
        LOG_ERROR("Failed to create the socket");
        return CCDI_ERROR_FAIL;
    }

    rv = VPLSocket_ConnectWithTimeout(*sock, &sockAddr, sizeof(sockAddr),
                                      VPLTime_FromSec(30));
    if (rv < 0) {
        LOG_ERROR("VPLSocket_ConnectWithTimeout(30s) failed: %d", rv);
    }

    return rv;
}

int
CCDIClient_OpenNamedSocket(VPLNamedSocketClient_t* sock_out)
{
    int rv = init_protobufSockName();
    if (rv < 0) {
        LOG_ERROR("Failed to init the socket name, rv = %d", rv);
        return rv;
    }
    return VPLNamedSocketClient_Open(ccdiSocketName, sock_out);
}

static const u8 DX_REQUEST_CODE_PROTORPC = 1;

static int
_acquireClientCommon()
{
	LOG_ALWAYS("enter acquireClientCommon"); // jimmy
    int rv;
	LOG_ALWAYS("s_useTcpSockAddr: %d", s_useTcpSockAddr);
	/*
	try{
		LOG_ALWAYS("tl_useTcpSockAddr: %d", tl_useTcpSockAddr);
		throw "tl_useTcpSockAddr error";
	}
	catch(...)
	{
		LOG_ALWAYS("tl_useTcpSockAddr is error");
	}
	*/
	LOG_ALWAYS("after rv"); // jimmy
    if (tl_useTcpSockAddr || s_useTcpSockAddr) {
    //if (s_useTcpSockAddr) {
		LOG_ALWAYS("enter _acquireClientCommon if"); // jimmy
        if (!tl_clientSockConnected) {
            rv = ccdiNewTcpSocket(&tl_clientTcpSock);
            if (rv < 0) {
                goto out;
            }
            tl_clientSockConnected = VPL_TRUE;
        }
        if (tl_channel == NULL) {
            tl_channel = new FileProtoChannel(VPLSocket_AsFd(tl_clientTcpSock),
                    false /* Underlying fd will be closed in #_releaseClientCommon(). */,
                    true /* it's a TCP socket */);
            {
                u8 reqType = VPLConv_hton_u8(DX_REQUEST_CODE_PROTORPC);
                rv = VPLSocket_Write(tl_clientTcpSock, &reqType, sizeof(reqType), VPLTIME_FROM_SEC(6));
                if (rv != sizeof(reqType)) {
                    LOG_WARN("VPLSocket_Write failed: %d", rv);
                    goto out;
                }
            }
        }
    } else {
		LOG_ALWAYS("enter _acquireClientCommon else"); // jimmy
        if (!tl_clientSockConnected) {
			LOG_ALWAYS(" ! tl_clientSockConnected"); // jimmy
            rv = CCDIClient_OpenNamedSocket(&tl_clientNamedSock);
            if (rv < 0) {
                goto out;
            }
            tl_clientSockConnected = VPL_TRUE;
        }
        if (tl_channel == NULL) {
			LOG_ALWAYS("tl_channel == NULL"); // jimmy
            tl_channel = new FileProtoChannel(VPLNamedSocketClient_GetFd(&tl_clientNamedSock),
                    false /* Underlying fd will be closed in #_releaseClientCommon(). */,
                    false /* On Windows, named pipes are actually files, not sockets */);
        }
    }
	LOG_ALWAYS("_acquireClientCommon end"); // jimmy
    rv = CCDI_OK; 
out:
    return rv;
}

static int
acquireSyncServiceClient(CCDIServiceClient*& serviceClient_out)
{
	LOG_ALWAYS("enter acquireSyncServiceClient");
    int rv = _acquireClientCommon();
	LOG_ALWAYS("after _acquireClientCommon");
    if (rv >= 0) {
        serviceClient_out = new CCDIServiceClient(*tl_channel);
    }
    return rv;
}

#define COMMON_TOP_SYNC() \
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG); \
    try { \
		LOG_ALWAYS("enter COMMON_TOP_SYNC()"); \
        int sync_rv; \
        RpcStatus status; \
        CCDIServiceClient* serviceClient; \
		LOG_ALWAYS("before acquireSyncServiceClient"); \
        sync_rv = acquireSyncServiceClient(serviceClient); \
		LOG_ALWAYS("after acquireSyncServiceClient"); \
        if (sync_rv < 0) { \
			LOG_ALWAYS("enter sync_rv"); \
            if (sync_rv == VPL_ERR_NAMED_SOCKET_NOT_EXIST) { \
                LOG_ALWAYS("sync_rv == VPL_ERR_NAMED_SOCKET_NOT_EXIST."); \
                LOG_INFO("The CCD process doesn't appear to be running."); \
            } else { \
                LOG_ALWAYS("sync_rv != VPL_ERR_NAMED_SOCKET_NOT_EXIST."); \
                LOG_ERROR("Failed to connect to CCD: %d", sync_rv); \
            } \
            return sync_rv; \
        } \

#define COMMON_BOILERPLATE_SYNC(call__, request__, response__) \
    BEGIN_MULTI_STATEMENT_MACRO \
        serviceClient->call__(request__, response__, status); \
        if (status.status() != RpcStatus::OK) { \
            LOG_ERROR("RPC layer returned %d: %s", status.status(), status.errordetail().c_str()); \
            releaseSyncServiceClient(serviceClient); \
            return CCDI_ERROR_RPC_FAILURE; \
        } \
    END_MULTI_STATEMENT_MACRO

#define COMMON_BOTTOM_SYNC() \
        releaseSyncServiceClient(serviceClient); \
        if (status.appstatus() < 0) { \
            LOG_INFO("CCD returned %d (%s)", status.appstatus(), status.errordetail().c_str()); \
        } else { \
            LOG_INFO("CCD returned %d", status.appstatus()); \
        } \
        return status.appstatus(); \
    } catch (std::bad_alloc& e) { \
        LOG_ERROR("allocation failed: %s", e.what()); \
        return CCDI_ERROR_OUT_OF_MEM; \
    } catch (std::exception& e) { \
        LOG_ERROR("Caught unexpected exception: %s", e.what()); \
        return CCDI_ERROR_FAIL; \
    }

//-------------------------------------------------------------

CCDIError
CCDIAddDataset(const ccd::AddDatasetInput& request,
               ccd::AddDatasetOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(AddDataset, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIAddSyncSubscription(
        const ccd::AddSyncSubscriptionInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(AddSyncSubscription, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIDeleteDataset(
        const ccd::DeleteDatasetInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(DeleteDataset, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIDeleteSyncSubscriptions(
        const ccd::DeleteSyncSubscriptionsInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(DeleteSyncSubscriptions, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIEventsCreateQueue(
        const ccd::EventsCreateQueueInput& request,
        ccd::EventsCreateQueueOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(EventsCreateQueue, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIEventsDestroyQueue(
        const ccd::EventsDestroyQueueInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(EventsDestroyQueue, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIEventsDequeue(
        const ccd::EventsDequeueInput& request,
        ccd::EventsDequeueOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(EventsDequeue, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIGetDatasetDirectoryEntries(
            const ccd::GetDatasetDirectoryEntriesInput& request,
            ccd::GetDatasetDirectoryEntriesOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(GetDatasetDirectoryEntries, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIGetInfraHttpInfo(
        const ccd::GetInfraHttpInfoInput& request,
        ccd::GetInfraHttpInfoOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(GetInfraHttpInfo, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIGetLocalHttpInfo(
        const ccd::GetLocalHttpInfoInput& request,
        ccd::GetLocalHttpInfoOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(GetLocalHttpInfo, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIGetPersonalCloudState(
        const ccd::GetPersonalCloudStateInput& request,
        ccd::GetPersonalCloudStateOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(GetPersonalCloudState, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIGetSyncState(
        const ccd::GetSyncStateInput& request,
        ccd::GetSyncStateOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(GetSyncState, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIGetSyncStateNotifications(
        const ccd::GetSyncStateNotificationsInput& request,
        ccd::GetSyncStateNotificationsOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(GetSyncStateNotifications, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIGetSystemState(
        const GetSystemStateInput& request,
        GetSystemStateOutput& response)
{
	LOG_ALWAYS("enter CCDIGetSystemState"); // jimmy
	/*
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG); 
    try { 
        int sync_rv; 
        RpcStatus status; 
        CCDIServiceClient* serviceClient; 
        sync_rv = acquireSyncServiceClient(serviceClient); 
        if (sync_rv < 0) { 
            if (sync_rv == VPL_ERR_NAMED_SOCKET_NOT_EXIST) { 
                LOG_INFO("The CCD process doesn't appear to be running."); 
            } else { 
                LOG_ERROR("Failed to connect to CCD: %d", sync_rv); 
            } 
            return sync_rv;
        } 
    */
    COMMON_TOP_SYNC();
	LOG_ALWAYS("after COMMON_TOP_SYNC"); // jimmy
    COMMON_BOILERPLATE_SYNC(GetSystemState, request, response);
	LOG_ALWAYS("after COMMON_BOILERPLATE_SYNC"); // jimmy
    COMMON_BOTTOM_SYNC();
	LOG_ALWAYS("after COMMON_BOTTOM_SYNC"); // jimmy
}

CCDIError
CCDIInfraHttpRequest(
        const InfraHttpRequestInput& request,
        InfraHttpRequestOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(InfraHttpRequest, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSABeginCatalog(
        const ccd::BeginCatalogInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSABeginCatalog, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSACommitCatalog(
        const ccd::CommitCatalogInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSACommitCatalog, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSAEndCatalog(
        const ccd::EndCatalogInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSAEndCatalog, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSABeginMetadataTransaction(
        const ccd::BeginMetadataTransactionInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSABeginMetadataTransaction, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSAUpdateMetadata(
        const ccd::UpdateMetadataInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSAUpdateMetadata, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSADeleteMetadata(
        const ccd::DeleteMetadataInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSADeleteMetadata, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSACommitMetadataTransaction()
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSACommitMetadataTransaction, dummyRequest, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSAGetMetadataSyncState(
        media_metadata::GetMetadataSyncStateOutput& response)
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    COMMON_BOILERPLATE_SYNC(MSAGetMetadataSyncState, dummyRequest, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSADeleteCollection(
        const ccd::DeleteCollectionInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSADeleteCollection, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError CCDIMSADeleteCatalog(
        const ccd::DeleteCatalogInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(MSADeleteCatalog, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIMSAListCollections(
        media_metadata::ListCollectionsOutput& response)
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    COMMON_BOILERPLATE_SYNC(MSAListCollections, dummyRequest, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIMSAGetCollectionDetails(
        const ccd::GetCollectionDetailsInput& request,
        ccd::GetCollectionDetailsOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(MSAGetCollectionDetails, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIMSAGetContentURL(
        const ccd::MSAGetContentURLInput& request,
        ccd::MSAGetContentURLOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(MSAGetContentURL, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIMCAQueryMetadataObjects(
        const ccd::MCAQueryMetadataObjectsInput& request,
        ccd::MCAQueryMetadataObjectsOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(MCAQueryMetadataObjects, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDILinkDevice(
        const ccd::LinkDeviceInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(LinkDevice, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIListLinkedDevices(
        const ccd::ListLinkedDevicesInput& request,
        ccd::ListLinkedDevicesOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(ListLinkedDevices, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIListOwnedDatasets(
        const ccd::ListOwnedDatasetsInput& request,
        ccd::ListOwnedDatasetsOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(ListOwnedDatasets, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIListSyncSubscriptions(
        const ccd::ListSyncSubscriptionsInput& request,
        ccd::ListSyncSubscriptionsOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(ListSyncSubscriptions, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDILogin(
        const LoginInput& request,
        LoginOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(Login, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDILogout(
        const LogoutInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(Logout, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIOwnershipSync()
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(OwnershipSync, dummyRequest, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIPrivateMsaDataCommit(
        const ccd::PrivateMsaDataCommitInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(PrivateMsaDataCommit, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISyncOnce(
        const ccd::SyncOnceInput& request,
        ccd::SyncOnceOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(SyncOnce, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIRegisterStorageNode(
        const ccd::RegisterStorageNodeInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(RegisterStorageNode, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIReportLanDevices(
        const ccd::ReportLanDevicesInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(ReportLanDevices, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIListLanDevices(
        const ccd::ListLanDevicesInput& request,
        ccd::ListLanDevicesOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(ListLanDevices, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIProbeLanDevices()
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(ProbeLanDevices, dummyRequest, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIListStorageNodeDatasets(
        ccd::ListStorageNodeDatasetsOutput& response)
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    COMMON_BOILERPLATE_SYNC(ListStorageNodeDatasets, dummyRequest, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIRemoteWakeup(
        const ccd::RemoteWakeupInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(RemoteWakeup, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIRenameDataset(
        const ccd::RenameDatasetInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(RenameDataset, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIListUserStorage(
        const ccd::ListUserStorageInput& request,
        ccd::ListUserStorageOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(ListUserStorage, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISWUpdateBeginDownload(
        const ccd::SWUpdateBeginDownloadInput& request,
        ccd::SWUpdateBeginDownloadOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(SWUpdateBeginDownload, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISWUpdateCheck(
        const ccd::SWUpdateCheckInput& request,
        ccd::SWUpdateCheckOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(SWUpdateCheck, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISWUpdateEndDownload(
        const ccd::SWUpdateEndDownloadInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(SWUpdateEndDownload, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISWUpdateCancelDownload(
        const ccd::SWUpdateCancelDownloadInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(SWUpdateCancelDownload, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISWUpdateGetDownloadProgress(
        const ccd::SWUpdateGetDownloadProgressInput& request,
        ccd::SWUpdateGetDownloadProgressOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(SWUpdateGetDownloadProgress, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISWUpdateSetCcdVersion(const ccd::SWUpdateSetCcdVersionInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(SWUpdateSetCcdVersion, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUnlinkDevice(
        const ccd::UnlinkDeviceInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(UnlinkDevice, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUnregisterStorageNode(
        const ccd::UnregisterStorageNodeInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(UnregisterStorageNode, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUpdateAppState(
        const ccd::UpdateAppStateInput& request,
        ccd::UpdateAppStateOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(UpdateAppState, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUpdateStorageNode(
        const ccd::UpdateStorageNodeInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(UpdateStorageNode, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUpdateSyncSettings(
        const ccd::UpdateSyncSettingsInput& request,
        ccd::UpdateSyncSettingsOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(UpdateSyncSettings, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUpdateSyncSubscription(
        const ccd::UpdateSyncSubscriptionInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(UpdateSyncSubscription, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUpdateSystemState(
        const UpdateSystemStateInput& request,
        UpdateSystemStateOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(UpdateSystemState, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIRemoteSwUpdateMessage(
        const ccd::RemoteSwUpdateMessageInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(RemoteSwUpdateMessage, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}
#if (defined LINUX || defined __CLOUDNODE__) && !defined LINUX_EMB
CCDIError
CCDIEnableInMemoryLogging()
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(EnableInMemoryLogging, dummyRequest, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIDisableInMemoryLogging()
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(DisableInMemoryLogging, dummyRequest, dummyResponse);
    COMMON_BOTTOM_SYNC();
}
#endif
CCDIError
CCDIFlushInMemoryLogs()
{
    COMMON_TOP_SYNC();
    NoParamRequest dummyRequest;
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(FlushInMemoryLogs, dummyRequest, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIRespondToPairingRequest(
    const ccd::RespondToPairingRequestInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(RespondToPairingRequest, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError 
CCDIRequestPairing(
    const ccd::RequestPairingInput& request,
    ccd::RequestPairingOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(RequestPairing, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError 
CCDIRequestPairingPin(
    const ccd::RequestPairingPinInput& request,
    ccd::RequestPairingPinOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(RequestPairingPin, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError 
CCDIGetPairingStatus(
    const ccd::GetPairingStatusInput& request,
    ccd::GetPairingStatusOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(GetPairingStatus, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError 
CCDIQueryPicStreamObjects(
    const ccd::CCDIQueryPicStreamObjectsInput& request,
    ccd::CCDIQueryPicStreamObjectsOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(QueryPicStreamObjects, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISharedFilesStoreFile(
    const ccd::SharedFilesStoreFileInput& request,
    ccd::SharedFilesStoreFileOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(SharedFilesStoreFile, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISharedFilesShareFile(
    const ccd::SharedFilesShareFileInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(SharedFilesShareFile, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISharedFilesUnshareFile(
    const ccd::SharedFilesUnshareFileInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(SharedFilesUnshareFile, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISharedFilesDeleteSharedFile(
    const ccd::SharedFilesDeleteSharedFileInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(SharedFilesDeleteSharedFile, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDISharedFilesQuery(
    const ccd::SharedFilesQueryInput& request,
    ccd::SharedFilesQueryOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(SharedFilesQuery, request, response);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIRegisterRemoteExecutable(
    const ccd::RegisterRemoteExecutableInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(RegisterRemoteExecutable, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIUnregisterRemoteExecutable(
    const ccd::UnregisterRemoteExecutableInput& request)
{
    COMMON_TOP_SYNC();
    NoParamResponse dummyResponse;
    COMMON_BOILERPLATE_SYNC(UnregisterRemoteExecutable, request, dummyResponse);
    COMMON_BOTTOM_SYNC();
}

CCDIError
CCDIListRegisteredRemoteExecutables(
    const ccd::ListRegisteredRemoteExecutablesInput& request,
    ccd::ListRegisteredRemoteExecutablesOutput& response)
{
    COMMON_TOP_SYNC();
    COMMON_BOILERPLATE_SYNC(ListRegisteredRemoteExecutables, request, response);
    COMMON_BOTTOM_SYNC();
}

} // namespace ccdi
} // namespace client
