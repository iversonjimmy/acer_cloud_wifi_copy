// GENERATED FROM ccdi_rpc.proto, DO NOT EDIT

#ifndef __ccdi_rpc_SERVER_PB_H__
#define __ccdi_rpc_SERVER_PB_H__

#include "ccdi_rpc.pb.h"
#include <ProtoChannel.h>
#include <ProtoRpc.h>

namespace ccd {

class CCDIService
{
public:
    CCDIService() {}
    virtual ~CCDIService(void) {}
    bool handleRpc(ProtoChannel& channel,
                   DebugMsgCallback debugCallback = NULL,
                   DebugRequestMsgCallback debugRequestCallback = NULL,
                   DebugResponseMsgCallback debugResponseCallback = NULL);

protected:
    virtual google::protobuf::int32 EventsCreateQueue(const ccd::EventsCreateQueueInput& request, ccd::EventsCreateQueueOutput& response) = 0;
    virtual google::protobuf::int32 EventsDestroyQueue(const ccd::EventsDestroyQueueInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 EventsDequeue(const ccd::EventsDequeueInput& request, ccd::EventsDequeueOutput& response) = 0;
    virtual google::protobuf::int32 GetSystemState(const ccd::GetSystemStateInput& request, ccd::GetSystemStateOutput& response) = 0;
    virtual google::protobuf::int32 Login(const ccd::LoginInput& request, ccd::LoginOutput& response) = 0;
    virtual google::protobuf::int32 Logout(const ccd::LogoutInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 InfraHttpRequest(const ccd::InfraHttpRequestInput& request, ccd::InfraHttpRequestOutput& response) = 0;
    virtual google::protobuf::int32 UpdateAppState(const ccd::UpdateAppStateInput& request, ccd::UpdateAppStateOutput& response) = 0;
    virtual google::protobuf::int32 UpdateSystemState(const ccd::UpdateSystemStateInput& request, ccd::UpdateSystemStateOutput& response) = 0;
    virtual google::protobuf::int32 RegisterStorageNode(const ccd::RegisterStorageNodeInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 UnregisterStorageNode(const ccd::UnregisterStorageNodeInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 UpdateStorageNode(const ccd::UpdateStorageNodeInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 ReportLanDevices(const ccd::ReportLanDevicesInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 ListLanDevices(const ccd::ListLanDevicesInput& request, ccd::ListLanDevicesOutput& response) = 0;
    virtual google::protobuf::int32 ProbeLanDevices(const ccd::NoParamRequest& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 ListStorageNodeDatasets(const ccd::NoParamRequest& request, ccd::ListStorageNodeDatasetsOutput& response) = 0;
    virtual google::protobuf::int32 AddDataset(const ccd::AddDatasetInput& request, ccd::AddDatasetOutput& response) = 0;
    virtual google::protobuf::int32 AddSyncSubscription(const ccd::AddSyncSubscriptionInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 DeleteDataset(const ccd::DeleteDatasetInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 DeleteSyncSubscriptions(const ccd::DeleteSyncSubscriptionsInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 GetDatasetDirectoryEntries(const ccd::GetDatasetDirectoryEntriesInput& request, ccd::GetDatasetDirectoryEntriesOutput& response) = 0;
    virtual google::protobuf::int32 GetInfraHttpInfo(const ccd::GetInfraHttpInfoInput& request, ccd::GetInfraHttpInfoOutput& response) = 0;
    virtual google::protobuf::int32 GetLocalHttpInfo(const ccd::GetLocalHttpInfoInput& request, ccd::GetLocalHttpInfoOutput& response) = 0;
    virtual google::protobuf::int32 GetPersonalCloudState(const ccd::GetPersonalCloudStateInput& request, ccd::GetPersonalCloudStateOutput& response) = 0;
    virtual google::protobuf::int32 GetSyncState(const ccd::GetSyncStateInput& request, ccd::GetSyncStateOutput& response) = 0;
    virtual google::protobuf::int32 GetSyncStateNotifications(const ccd::GetSyncStateNotificationsInput& request, ccd::GetSyncStateNotificationsOutput& response) = 0;
    virtual google::protobuf::int32 LinkDevice(const ccd::LinkDeviceInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 ListLinkedDevices(const ccd::ListLinkedDevicesInput& request, ccd::ListLinkedDevicesOutput& response) = 0;
    virtual google::protobuf::int32 ListOwnedDatasets(const ccd::ListOwnedDatasetsInput& request, ccd::ListOwnedDatasetsOutput& response) = 0;
    virtual google::protobuf::int32 ListSyncSubscriptions(const ccd::ListSyncSubscriptionsInput& request, ccd::ListSyncSubscriptionsOutput& response) = 0;
    virtual google::protobuf::int32 OwnershipSync(const ccd::NoParamRequest& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 PrivateMsaDataCommit(const ccd::PrivateMsaDataCommitInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 RemoteWakeup(const ccd::RemoteWakeupInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 RenameDataset(const ccd::RenameDatasetInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 SyncOnce(const ccd::SyncOnceInput& request, ccd::SyncOnceOutput& response) = 0;
    virtual google::protobuf::int32 UnlinkDevice(const ccd::UnlinkDeviceInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 UpdateSyncSettings(const ccd::UpdateSyncSettingsInput& request, ccd::UpdateSyncSettingsOutput& response) = 0;
    virtual google::protobuf::int32 UpdateSyncSubscription(const ccd::UpdateSyncSubscriptionInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 ListUserStorage(const ccd::ListUserStorageInput& request, ccd::ListUserStorageOutput& response) = 0;
    virtual google::protobuf::int32 RemoteSwUpdateMessage(const ccd::RemoteSwUpdateMessageInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 SWUpdateCheck(const ccd::SWUpdateCheckInput& request, ccd::SWUpdateCheckOutput& response) = 0;
    virtual google::protobuf::int32 SWUpdateBeginDownload(const ccd::SWUpdateBeginDownloadInput& request, ccd::SWUpdateBeginDownloadOutput& response) = 0;
    virtual google::protobuf::int32 SWUpdateGetDownloadProgress(const ccd::SWUpdateGetDownloadProgressInput& request, ccd::SWUpdateGetDownloadProgressOutput& response) = 0;
    virtual google::protobuf::int32 SWUpdateEndDownload(const ccd::SWUpdateEndDownloadInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 SWUpdateCancelDownload(const ccd::SWUpdateCancelDownloadInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 SWUpdateSetCcdVersion(const ccd::SWUpdateSetCcdVersionInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSABeginCatalog(const ccd::BeginCatalogInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSACommitCatalog(const ccd::CommitCatalogInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSAEndCatalog(const ccd::EndCatalogInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSAUpdateMetadata(const ccd::UpdateMetadataInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSADeleteMetadata(const ccd::DeleteMetadataInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSACommitMetadataTransaction(const ccd::NoParamRequest& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSAGetMetadataSyncState(const ccd::NoParamRequest& request, media_metadata::GetMetadataSyncStateOutput& response) = 0;
    virtual google::protobuf::int32 MSADeleteCollection(const ccd::DeleteCollectionInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSADeleteCatalog(const ccd::DeleteCatalogInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 MSAListCollections(const ccd::NoParamRequest& request, media_metadata::ListCollectionsOutput& response) = 0;
    virtual google::protobuf::int32 MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& request, ccd::GetCollectionDetailsOutput& response) = 0;
    virtual google::protobuf::int32 MSAGetContentURL(const ccd::MSAGetContentURLInput& request, ccd::MSAGetContentURLOutput& response) = 0;
    virtual google::protobuf::int32 MCAQueryMetadataObjects(const ccd::MCAQueryMetadataObjectsInput& request, ccd::MCAQueryMetadataObjectsOutput& response) = 0;
    #if (defined(LINUX) || defined(__CLOUDNODE__)) && !defined(LINUX_EMB)
	virtual google::protobuf::int32 EnableInMemoryLogging(const ccd::NoParamRequest& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 DisableInMemoryLogging(const ccd::NoParamRequest& request, ccd::NoParamResponse& response) = 0;
	#endif
    virtual google::protobuf::int32 FlushInMemoryLogs(const ccd::NoParamRequest& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 RespondToPairingRequest(const ccd::RespondToPairingRequestInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 RequestPairing(const ccd::RequestPairingInput& request, ccd::RequestPairingOutput& response) = 0;
    virtual google::protobuf::int32 RequestPairingPin(const ccd::RequestPairingPinInput& request, ccd::RequestPairingPinOutput& response) = 0;
    virtual google::protobuf::int32 GetPairingStatus(const ccd::GetPairingStatusInput& request, ccd::GetPairingStatusOutput& response) = 0;
    virtual google::protobuf::int32 QueryPicStreamObjects(const ccd::CCDIQueryPicStreamObjectsInput& request, ccd::CCDIQueryPicStreamObjectsOutput& response) = 0;
    virtual google::protobuf::int32 SharedFilesStoreFile(const ccd::SharedFilesStoreFileInput& request, ccd::SharedFilesStoreFileOutput& response) = 0;
    virtual google::protobuf::int32 SharedFilesShareFile(const ccd::SharedFilesShareFileInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 SharedFilesUnshareFile(const ccd::SharedFilesUnshareFileInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 SharedFilesDeleteSharedFile(const ccd::SharedFilesDeleteSharedFileInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 SharedFilesQuery(const ccd::SharedFilesQueryInput& request, ccd::SharedFilesQueryOutput& response) = 0;
    virtual google::protobuf::int32 RegisterRemoteExecutable(const ccd::RegisterRemoteExecutableInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 UnregisterRemoteExecutable(const ccd::UnregisterRemoteExecutableInput& request, ccd::NoParamResponse& response) = 0;
    virtual google::protobuf::int32 ListRegisteredRemoteExecutables(const ccd::ListRegisteredRemoteExecutablesInput& request, ccd::ListRegisteredRemoteExecutablesOutput& response) = 0;
};

} // namespace ccd

#endif /* __ccdi_rpc_SERVER_PB_H__ */
