// GENERATED FROM ccdi_rpc.proto, DO NOT EDIT

#include "ccdi_rpc-client.pb.h"
#include <string>
#include "ccdi_rpc.pb.h"
#include <rpc.pb.h>
#include <ProtoChannel.h>
#include <ProtoRpcClient.h>

using std::string;

ccd::CCDIServiceClient::CCDIServiceClient(ProtoChannel& channel)
    : _client(channel)
{}

void
ccd::CCDIServiceClient::EventsCreateQueue(const ccd::EventsCreateQueueInput& request, ccd::EventsCreateQueueOutput& response, RpcStatus& status)
{
    static const string method("EventsCreateQueue");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::EventsDestroyQueue(const ccd::EventsDestroyQueueInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("EventsDestroyQueue");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::EventsDequeue(const ccd::EventsDequeueInput& request, ccd::EventsDequeueOutput& response, RpcStatus& status)
{
    static const string method("EventsDequeue");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetSystemState(const ccd::GetSystemStateInput& request, ccd::GetSystemStateOutput& response, RpcStatus& status)
{
    static const string method("GetSystemState");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::Login(const ccd::LoginInput& request, ccd::LoginOutput& response, RpcStatus& status)
{
    static const string method("Login");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::Logout(const ccd::LogoutInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("Logout");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::InfraHttpRequest(const ccd::InfraHttpRequestInput& request, ccd::InfraHttpRequestOutput& response, RpcStatus& status)
{
    static const string method("InfraHttpRequest");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UpdateAppState(const ccd::UpdateAppStateInput& request, ccd::UpdateAppStateOutput& response, RpcStatus& status)
{
    static const string method("UpdateAppState");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UpdateSystemState(const ccd::UpdateSystemStateInput& request, ccd::UpdateSystemStateOutput& response, RpcStatus& status)
{
    static const string method("UpdateSystemState");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RegisterStorageNode(const ccd::RegisterStorageNodeInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("RegisterStorageNode");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UnregisterStorageNode(const ccd::UnregisterStorageNodeInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("UnregisterStorageNode");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UpdateStorageNode(const ccd::UpdateStorageNodeInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("UpdateStorageNode");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ReportLanDevices(const ccd::ReportLanDevicesInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("ReportLanDevices");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ListLanDevices(const ccd::ListLanDevicesInput& request, ccd::ListLanDevicesOutput& response, RpcStatus& status)
{
    static const string method("ListLanDevices");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ProbeLanDevices(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("ProbeLanDevices");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ListStorageNodeDatasets(const ccd::NoParamRequest& request, ccd::ListStorageNodeDatasetsOutput& response, RpcStatus& status)
{
    static const string method("ListStorageNodeDatasets");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::AddDataset(const ccd::AddDatasetInput& request, ccd::AddDatasetOutput& response, RpcStatus& status)
{
    static const string method("AddDataset");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::AddSyncSubscription(const ccd::AddSyncSubscriptionInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("AddSyncSubscription");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::DeleteDataset(const ccd::DeleteDatasetInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("DeleteDataset");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::DeleteSyncSubscriptions(const ccd::DeleteSyncSubscriptionsInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("DeleteSyncSubscriptions");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetDatasetDirectoryEntries(const ccd::GetDatasetDirectoryEntriesInput& request, ccd::GetDatasetDirectoryEntriesOutput& response, RpcStatus& status)
{
    static const string method("GetDatasetDirectoryEntries");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetInfraHttpInfo(const ccd::GetInfraHttpInfoInput& request, ccd::GetInfraHttpInfoOutput& response, RpcStatus& status)
{
    static const string method("GetInfraHttpInfo");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetLocalHttpInfo(const ccd::GetLocalHttpInfoInput& request, ccd::GetLocalHttpInfoOutput& response, RpcStatus& status)
{
    static const string method("GetLocalHttpInfo");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetPersonalCloudState(const ccd::GetPersonalCloudStateInput& request, ccd::GetPersonalCloudStateOutput& response, RpcStatus& status)
{
    static const string method("GetPersonalCloudState");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetSyncState(const ccd::GetSyncStateInput& request, ccd::GetSyncStateOutput& response, RpcStatus& status)
{
    static const string method("GetSyncState");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetSyncStateNotifications(const ccd::GetSyncStateNotificationsInput& request, ccd::GetSyncStateNotificationsOutput& response, RpcStatus& status)
{
    static const string method("GetSyncStateNotifications");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::LinkDevice(const ccd::LinkDeviceInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("LinkDevice");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ListLinkedDevices(const ccd::ListLinkedDevicesInput& request, ccd::ListLinkedDevicesOutput& response, RpcStatus& status)
{
    static const string method("ListLinkedDevices");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ListOwnedDatasets(const ccd::ListOwnedDatasetsInput& request, ccd::ListOwnedDatasetsOutput& response, RpcStatus& status)
{
    static const string method("ListOwnedDatasets");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ListSyncSubscriptions(const ccd::ListSyncSubscriptionsInput& request, ccd::ListSyncSubscriptionsOutput& response, RpcStatus& status)
{
    static const string method("ListSyncSubscriptions");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::OwnershipSync(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("OwnershipSync");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::PrivateMsaDataCommit(const ccd::PrivateMsaDataCommitInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("PrivateMsaDataCommit");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RemoteWakeup(const ccd::RemoteWakeupInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("RemoteWakeup");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RenameDataset(const ccd::RenameDatasetInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("RenameDataset");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SyncOnce(const ccd::SyncOnceInput& request, ccd::SyncOnceOutput& response, RpcStatus& status)
{
    static const string method("SyncOnce");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UnlinkDevice(const ccd::UnlinkDeviceInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("UnlinkDevice");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UpdateSyncSettings(const ccd::UpdateSyncSettingsInput& request, ccd::UpdateSyncSettingsOutput& response, RpcStatus& status)
{
    static const string method("UpdateSyncSettings");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UpdateSyncSubscription(const ccd::UpdateSyncSubscriptionInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("UpdateSyncSubscription");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ListUserStorage(const ccd::ListUserStorageInput& request, ccd::ListUserStorageOutput& response, RpcStatus& status)
{
    static const string method("ListUserStorage");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RemoteSwUpdateMessage(const ccd::RemoteSwUpdateMessageInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("RemoteSwUpdateMessage");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SWUpdateCheck(const ccd::SWUpdateCheckInput& request, ccd::SWUpdateCheckOutput& response, RpcStatus& status)
{
    static const string method("SWUpdateCheck");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SWUpdateBeginDownload(const ccd::SWUpdateBeginDownloadInput& request, ccd::SWUpdateBeginDownloadOutput& response, RpcStatus& status)
{
    static const string method("SWUpdateBeginDownload");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SWUpdateGetDownloadProgress(const ccd::SWUpdateGetDownloadProgressInput& request, ccd::SWUpdateGetDownloadProgressOutput& response, RpcStatus& status)
{
    static const string method("SWUpdateGetDownloadProgress");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SWUpdateEndDownload(const ccd::SWUpdateEndDownloadInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("SWUpdateEndDownload");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SWUpdateCancelDownload(const ccd::SWUpdateCancelDownloadInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("SWUpdateCancelDownload");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SWUpdateSetCcdVersion(const ccd::SWUpdateSetCcdVersionInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("SWUpdateSetCcdVersion");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSABeginCatalog(const ccd::BeginCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSABeginCatalog");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSACommitCatalog(const ccd::CommitCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSACommitCatalog");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSAEndCatalog(const ccd::EndCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSAEndCatalog");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSABeginMetadataTransaction");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSAUpdateMetadata(const ccd::UpdateMetadataInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSAUpdateMetadata");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSADeleteMetadata(const ccd::DeleteMetadataInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSADeleteMetadata");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSACommitMetadataTransaction(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSACommitMetadataTransaction");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSAGetMetadataSyncState(const ccd::NoParamRequest& request, media_metadata::GetMetadataSyncStateOutput& response, RpcStatus& status)
{
    static const string method("MSAGetMetadataSyncState");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSADeleteCollection(const ccd::DeleteCollectionInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSADeleteCollection");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSADeleteCatalog(const ccd::DeleteCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("MSADeleteCatalog");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSAListCollections(const ccd::NoParamRequest& request, media_metadata::ListCollectionsOutput& response, RpcStatus& status)
{
    static const string method("MSAListCollections");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& request, ccd::GetCollectionDetailsOutput& response, RpcStatus& status)
{
    static const string method("MSAGetCollectionDetails");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MSAGetContentURL(const ccd::MSAGetContentURLInput& request, ccd::MSAGetContentURLOutput& response, RpcStatus& status)
{
    static const string method("MSAGetContentURL");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::MCAQueryMetadataObjects(const ccd::MCAQueryMetadataObjectsInput& request, ccd::MCAQueryMetadataObjectsOutput& response, RpcStatus& status)
{
    static const string method("MCAQueryMetadataObjects");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::EnableInMemoryLogging(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("EnableInMemoryLogging");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::DisableInMemoryLogging(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("DisableInMemoryLogging");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::FlushInMemoryLogs(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("FlushInMemoryLogs");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RespondToPairingRequest(const ccd::RespondToPairingRequestInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("RespondToPairingRequest");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RequestPairing(const ccd::RequestPairingInput& request, ccd::RequestPairingOutput& response, RpcStatus& status)
{
    static const string method("RequestPairing");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RequestPairingPin(const ccd::RequestPairingPinInput& request, ccd::RequestPairingPinOutput& response, RpcStatus& status)
{
    static const string method("RequestPairingPin");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::GetPairingStatus(const ccd::GetPairingStatusInput& request, ccd::GetPairingStatusOutput& response, RpcStatus& status)
{
    static const string method("GetPairingStatus");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::QueryPicStreamObjects(const ccd::CCDIQueryPicStreamObjectsInput& request, ccd::CCDIQueryPicStreamObjectsOutput& response, RpcStatus& status)
{
    static const string method("QueryPicStreamObjects");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SharedFilesStoreFile(const ccd::SharedFilesStoreFileInput& request, ccd::SharedFilesStoreFileOutput& response, RpcStatus& status)
{
    static const string method("SharedFilesStoreFile");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SharedFilesShareFile(const ccd::SharedFilesShareFileInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("SharedFilesShareFile");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SharedFilesUnshareFile(const ccd::SharedFilesUnshareFileInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("SharedFilesUnshareFile");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SharedFilesDeleteSharedFile(const ccd::SharedFilesDeleteSharedFileInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("SharedFilesDeleteSharedFile");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::SharedFilesQuery(const ccd::SharedFilesQueryInput& request, ccd::SharedFilesQueryOutput& response, RpcStatus& status)
{
    static const string method("SharedFilesQuery");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::RegisterRemoteExecutable(const ccd::RegisterRemoteExecutableInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("RegisterRemoteExecutable");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::UnregisterRemoteExecutable(const ccd::UnregisterRemoteExecutableInput& request, ccd::NoParamResponse& response, RpcStatus& status)
{
    static const string method("UnregisterRemoteExecutable");
    _client.sendRpc(method, request, response, status);
}

void
ccd::CCDIServiceClient::ListRegisteredRemoteExecutables(const ccd::ListRegisteredRemoteExecutablesInput& request, ccd::ListRegisteredRemoteExecutablesOutput& response, RpcStatus& status)
{
    static const string method("ListRegisteredRemoteExecutables");
    _client.sendRpc(method, request, response, status);
}

