// GENERATED FROM ccdi_rpc.proto, DO NOT EDIT

#ifndef __ccdi_rpc_CLIENT_PB_H__
#define __ccdi_rpc_CLIENT_PB_H__

#include "ccdi_rpc.pb.h"
#include <rpc.pb.h>
#include <ProtoChannel.h>
#include <ProtoRpcClient.h>

namespace ccd {

class CCDIServiceClient
{
public:
    CCDIServiceClient(ProtoChannel& channel);

    // Implemented by #CCDIService.EventsCreateQueue() 
    void EventsCreateQueue(const ccd::EventsCreateQueueInput& request, ccd::EventsCreateQueueOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.EventsDestroyQueue() 
    void EventsDestroyQueue(const ccd::EventsDestroyQueueInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.EventsDequeue() 
    void EventsDequeue(const ccd::EventsDequeueInput& request, ccd::EventsDequeueOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.GetSystemState() 
    void GetSystemState(const ccd::GetSystemStateInput& request, ccd::GetSystemStateOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.Login() 
    void Login(const ccd::LoginInput& request, ccd::LoginOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.Logout() 
    void Logout(const ccd::LogoutInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.InfraHttpRequest() 
    void InfraHttpRequest(const ccd::InfraHttpRequestInput& request, ccd::InfraHttpRequestOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.UpdateAppState() 
    void UpdateAppState(const ccd::UpdateAppStateInput& request, ccd::UpdateAppStateOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.UpdateSystemState() 
    void UpdateSystemState(const ccd::UpdateSystemStateInput& request, ccd::UpdateSystemStateOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.RegisterStorageNode() 
    void RegisterStorageNode(const ccd::RegisterStorageNodeInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.UnregisterStorageNode() 
    void UnregisterStorageNode(const ccd::UnregisterStorageNodeInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.UpdateStorageNode() 
    void UpdateStorageNode(const ccd::UpdateStorageNodeInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.ReportLanDevices() 
    void ReportLanDevices(const ccd::ReportLanDevicesInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.ListLanDevices() 
    void ListLanDevices(const ccd::ListLanDevicesInput& request, ccd::ListLanDevicesOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.ProbeLanDevices() 
    void ProbeLanDevices(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.ListStorageNodeDatasets() 
    void ListStorageNodeDatasets(const ccd::NoParamRequest& request, ccd::ListStorageNodeDatasetsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.AddDataset() 
    void AddDataset(const ccd::AddDatasetInput& request, ccd::AddDatasetOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.AddSyncSubscription() 
    void AddSyncSubscription(const ccd::AddSyncSubscriptionInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.DeleteDataset() 
    void DeleteDataset(const ccd::DeleteDatasetInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.DeleteSyncSubscriptions() 
    void DeleteSyncSubscriptions(const ccd::DeleteSyncSubscriptionsInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.GetDatasetDirectoryEntries() 
    void GetDatasetDirectoryEntries(const ccd::GetDatasetDirectoryEntriesInput& request, ccd::GetDatasetDirectoryEntriesOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.GetInfraHttpInfo() 
    void GetInfraHttpInfo(const ccd::GetInfraHttpInfoInput& request, ccd::GetInfraHttpInfoOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.GetLocalHttpInfo() 
    void GetLocalHttpInfo(const ccd::GetLocalHttpInfoInput& request, ccd::GetLocalHttpInfoOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.GetPersonalCloudState() 
    void GetPersonalCloudState(const ccd::GetPersonalCloudStateInput& request, ccd::GetPersonalCloudStateOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.GetSyncState() 
    void GetSyncState(const ccd::GetSyncStateInput& request, ccd::GetSyncStateOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.GetSyncStateNotifications() 
    void GetSyncStateNotifications(const ccd::GetSyncStateNotificationsInput& request, ccd::GetSyncStateNotificationsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.LinkDevice() 
    void LinkDevice(const ccd::LinkDeviceInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.ListLinkedDevices() 
    void ListLinkedDevices(const ccd::ListLinkedDevicesInput& request, ccd::ListLinkedDevicesOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.ListOwnedDatasets() 
    void ListOwnedDatasets(const ccd::ListOwnedDatasetsInput& request, ccd::ListOwnedDatasetsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.ListSyncSubscriptions() 
    void ListSyncSubscriptions(const ccd::ListSyncSubscriptionsInput& request, ccd::ListSyncSubscriptionsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.OwnershipSync() 
    void OwnershipSync(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.PrivateMsaDataCommit() 
    void PrivateMsaDataCommit(const ccd::PrivateMsaDataCommitInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.RemoteWakeup() 
    void RemoteWakeup(const ccd::RemoteWakeupInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.RenameDataset() 
    void RenameDataset(const ccd::RenameDatasetInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.SyncOnce() 
    void SyncOnce(const ccd::SyncOnceInput& request, ccd::SyncOnceOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.UnlinkDevice() 
    void UnlinkDevice(const ccd::UnlinkDeviceInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.UpdateSyncSettings() 
    void UpdateSyncSettings(const ccd::UpdateSyncSettingsInput& request, ccd::UpdateSyncSettingsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.UpdateSyncSubscription() 
    void UpdateSyncSubscription(const ccd::UpdateSyncSubscriptionInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.ListUserStorage() 
    void ListUserStorage(const ccd::ListUserStorageInput& request, ccd::ListUserStorageOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.RemoteSwUpdateMessage() 
    void RemoteSwUpdateMessage(const ccd::RemoteSwUpdateMessageInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.SWUpdateCheck() 
    void SWUpdateCheck(const ccd::SWUpdateCheckInput& request, ccd::SWUpdateCheckOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.SWUpdateBeginDownload() 
    void SWUpdateBeginDownload(const ccd::SWUpdateBeginDownloadInput& request, ccd::SWUpdateBeginDownloadOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.SWUpdateGetDownloadProgress() 
    void SWUpdateGetDownloadProgress(const ccd::SWUpdateGetDownloadProgressInput& request, ccd::SWUpdateGetDownloadProgressOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.SWUpdateEndDownload() 
    void SWUpdateEndDownload(const ccd::SWUpdateEndDownloadInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.SWUpdateCancelDownload() 
    void SWUpdateCancelDownload(const ccd::SWUpdateCancelDownloadInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.SWUpdateSetCcdVersion() 
    void SWUpdateSetCcdVersion(const ccd::SWUpdateSetCcdVersionInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSABeginCatalog() 
    void MSABeginCatalog(const ccd::BeginCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSACommitCatalog() 
    void MSACommitCatalog(const ccd::CommitCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSAEndCatalog() 
    void MSAEndCatalog(const ccd::EndCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSABeginMetadataTransaction() 
    void MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSAUpdateMetadata() 
    void MSAUpdateMetadata(const ccd::UpdateMetadataInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSADeleteMetadata() 
    void MSADeleteMetadata(const ccd::DeleteMetadataInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSACommitMetadataTransaction() 
    void MSACommitMetadataTransaction(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSAGetMetadataSyncState() 
    void MSAGetMetadataSyncState(const ccd::NoParamRequest& request, media_metadata::GetMetadataSyncStateOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.MSADeleteCollection() 
    void MSADeleteCollection(const ccd::DeleteCollectionInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSADeleteCatalog() 
    void MSADeleteCatalog(const ccd::DeleteCatalogInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.MSAListCollections() 
    void MSAListCollections(const ccd::NoParamRequest& request, media_metadata::ListCollectionsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.MSAGetCollectionDetails() 
    void MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& request, ccd::GetCollectionDetailsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.MSAGetContentURL() 
    void MSAGetContentURL(const ccd::MSAGetContentURLInput& request, ccd::MSAGetContentURLOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.MCAQueryMetadataObjects() 
    void MCAQueryMetadataObjects(const ccd::MCAQueryMetadataObjectsInput& request, ccd::MCAQueryMetadataObjectsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.EnableInMemoryLogging() 
    void EnableInMemoryLogging(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.DisableInMemoryLogging() 
    void DisableInMemoryLogging(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.FlushInMemoryLogs() 
    void FlushInMemoryLogs(const ccd::NoParamRequest& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.RespondToPairingRequest() 
    void RespondToPairingRequest(const ccd::RespondToPairingRequestInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.RequestPairing() 
    void RequestPairing(const ccd::RequestPairingInput& request, ccd::RequestPairingOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.RequestPairingPin() 
    void RequestPairingPin(const ccd::RequestPairingPinInput& request, ccd::RequestPairingPinOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.GetPairingStatus() 
    void GetPairingStatus(const ccd::GetPairingStatusInput& request, ccd::GetPairingStatusOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.QueryPicStreamObjects() 
    void QueryPicStreamObjects(const ccd::CCDIQueryPicStreamObjectsInput& request, ccd::CCDIQueryPicStreamObjectsOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.SharedFilesStoreFile() 
    void SharedFilesStoreFile(const ccd::SharedFilesStoreFileInput& request, ccd::SharedFilesStoreFileOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.SharedFilesShareFile() 
    void SharedFilesShareFile(const ccd::SharedFilesShareFileInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.SharedFilesUnshareFile() 
    void SharedFilesUnshareFile(const ccd::SharedFilesUnshareFileInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.SharedFilesDeleteSharedFile() 
    void SharedFilesDeleteSharedFile(const ccd::SharedFilesDeleteSharedFileInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.SharedFilesQuery() 
    void SharedFilesQuery(const ccd::SharedFilesQueryInput& request, ccd::SharedFilesQueryOutput& response, RpcStatus& status);

    // Implemented by #CCDIService.RegisterRemoteExecutable() 
    void RegisterRemoteExecutable(const ccd::RegisterRemoteExecutableInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.UnregisterRemoteExecutable() 
    void UnregisterRemoteExecutable(const ccd::UnregisterRemoteExecutableInput& request, ccd::NoParamResponse& response, RpcStatus& status);

    // Implemented by #CCDIService.ListRegisteredRemoteExecutables() 
    void ListRegisteredRemoteExecutables(const ccd::ListRegisteredRemoteExecutablesInput& request, ccd::ListRegisteredRemoteExecutablesOutput& response, RpcStatus& status);

private:
    ProtoRpcClient _client;
};

} // namespace ccd

#endif /* __ccdi_rpc_CLIENT_PB_H__ */
