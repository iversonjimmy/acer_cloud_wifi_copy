// GENERATED FROM ccdi_rpc.proto, DO NOT EDIT

package igware.gvm.pb;

public class CcdiRpcClient {

    public static class CCDIServiceClient extends igware.protobuf.ProtoRpcClient {

        /**
         * Allows sending RPC requests to an instance of CCDIService.
         * Please see {@link igware.protobuf.ProtoRpcClient}.
         */
        public CCDIServiceClient(igware.protobuf.ProtoChannel channel, boolean throwOnAppError) {
            super(channel, throwOnAppError);
        }

        public int EventsCreateQueue(
                igware.gvm.pb.CcdiRpc.EventsCreateQueueInput request,
                igware.gvm.pb.CcdiRpc.EventsCreateQueueOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("EventsCreateQueue", request, responseBuilder);
        }

        public int EventsDestroyQueue(
                igware.gvm.pb.CcdiRpc.EventsDestroyQueueInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("EventsDestroyQueue", request, responseBuilder);
        }

        public int EventsDequeue(
                igware.gvm.pb.CcdiRpc.EventsDequeueInput request,
                igware.gvm.pb.CcdiRpc.EventsDequeueOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("EventsDequeue", request, responseBuilder);
        }

        public int GetSystemState(
                igware.gvm.pb.CcdiRpc.GetSystemStateInput request,
                igware.gvm.pb.CcdiRpc.GetSystemStateOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetSystemState", request, responseBuilder);
        }

        public int Login(
                igware.gvm.pb.CcdiRpc.LoginInput request,
                igware.gvm.pb.CcdiRpc.LoginOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("Login", request, responseBuilder);
        }

        public int Logout(
                igware.gvm.pb.CcdiRpc.LogoutInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("Logout", request, responseBuilder);
        }

        public int InfraHttpRequest(
                igware.gvm.pb.CcdiRpc.InfraHttpRequestInput request,
                igware.gvm.pb.CcdiRpc.InfraHttpRequestOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("InfraHttpRequest", request, responseBuilder);
        }

        public int UpdateAppState(
                igware.gvm.pb.CcdiRpc.UpdateAppStateInput request,
                igware.gvm.pb.CcdiRpc.UpdateAppStateOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UpdateAppState", request, responseBuilder);
        }

        public int UpdateSystemState(
                igware.gvm.pb.CcdiRpc.UpdateSystemStateInput request,
                igware.gvm.pb.CcdiRpc.UpdateSystemStateOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UpdateSystemState", request, responseBuilder);
        }

        public int RegisterStorageNode(
                igware.gvm.pb.CcdiRpc.RegisterStorageNodeInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RegisterStorageNode", request, responseBuilder);
        }

        public int UnregisterStorageNode(
                igware.gvm.pb.CcdiRpc.UnregisterStorageNodeInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UnregisterStorageNode", request, responseBuilder);
        }

        public int UpdateStorageNode(
                igware.gvm.pb.CcdiRpc.UpdateStorageNodeInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UpdateStorageNode", request, responseBuilder);
        }

        public int ReportLanDevices(
                igware.gvm.pb.CcdiRpc.ReportLanDevicesInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ReportLanDevices", request, responseBuilder);
        }

        public int ListLanDevices(
                igware.gvm.pb.CcdiRpc.ListLanDevicesInput request,
                igware.gvm.pb.CcdiRpc.ListLanDevicesOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ListLanDevices", request, responseBuilder);
        }

        public int ProbeLanDevices(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ProbeLanDevices", request, responseBuilder);
        }

        public int ListStorageNodeDatasets(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.gvm.pb.CcdiRpc.ListStorageNodeDatasetsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ListStorageNodeDatasets", request, responseBuilder);
        }

        public int AddDataset(
                igware.gvm.pb.CcdiRpc.AddDatasetInput request,
                igware.gvm.pb.CcdiRpc.AddDatasetOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("AddDataset", request, responseBuilder);
        }

        public int AddSyncSubscription(
                igware.gvm.pb.CcdiRpc.AddSyncSubscriptionInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("AddSyncSubscription", request, responseBuilder);
        }

        public int DeleteDataset(
                igware.gvm.pb.CcdiRpc.DeleteDatasetInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("DeleteDataset", request, responseBuilder);
        }

        public int DeleteSyncSubscriptions(
                igware.gvm.pb.CcdiRpc.DeleteSyncSubscriptionsInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("DeleteSyncSubscriptions", request, responseBuilder);
        }

        public int GetDatasetDirectoryEntries(
                igware.gvm.pb.CcdiRpc.GetDatasetDirectoryEntriesInput request,
                igware.gvm.pb.CcdiRpc.GetDatasetDirectoryEntriesOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetDatasetDirectoryEntries", request, responseBuilder);
        }

        public int GetInfraHttpInfo(
                igware.gvm.pb.CcdiRpc.GetInfraHttpInfoInput request,
                igware.gvm.pb.CcdiRpc.GetInfraHttpInfoOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetInfraHttpInfo", request, responseBuilder);
        }

        public int GetLocalHttpInfo(
                igware.gvm.pb.CcdiRpc.GetLocalHttpInfoInput request,
                igware.gvm.pb.CcdiRpc.GetLocalHttpInfoOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetLocalHttpInfo", request, responseBuilder);
        }

        public int GetPersonalCloudState(
                igware.gvm.pb.CcdiRpc.GetPersonalCloudStateInput request,
                igware.gvm.pb.CcdiRpc.GetPersonalCloudStateOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetPersonalCloudState", request, responseBuilder);
        }

        public int GetSyncState(
                igware.gvm.pb.CcdiRpc.GetSyncStateInput request,
                igware.gvm.pb.CcdiRpc.GetSyncStateOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetSyncState", request, responseBuilder);
        }

        public int GetSyncStateNotifications(
                igware.gvm.pb.CcdiRpc.GetSyncStateNotificationsInput request,
                igware.gvm.pb.CcdiRpc.GetSyncStateNotificationsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetSyncStateNotifications", request, responseBuilder);
        }

        public int LinkDevice(
                igware.gvm.pb.CcdiRpc.LinkDeviceInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("LinkDevice", request, responseBuilder);
        }

        public int ListLinkedDevices(
                igware.gvm.pb.CcdiRpc.ListLinkedDevicesInput request,
                igware.gvm.pb.CcdiRpc.ListLinkedDevicesOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ListLinkedDevices", request, responseBuilder);
        }

        public int ListOwnedDatasets(
                igware.gvm.pb.CcdiRpc.ListOwnedDatasetsInput request,
                igware.gvm.pb.CcdiRpc.ListOwnedDatasetsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ListOwnedDatasets", request, responseBuilder);
        }

        public int ListSyncSubscriptions(
                igware.gvm.pb.CcdiRpc.ListSyncSubscriptionsInput request,
                igware.gvm.pb.CcdiRpc.ListSyncSubscriptionsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ListSyncSubscriptions", request, responseBuilder);
        }

        public int OwnershipSync(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("OwnershipSync", request, responseBuilder);
        }

        public int PrivateMsaDataCommit(
                igware.gvm.pb.CcdiRpc.PrivateMsaDataCommitInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("PrivateMsaDataCommit", request, responseBuilder);
        }

        public int RemoteWakeup(
                igware.gvm.pb.CcdiRpc.RemoteWakeupInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RemoteWakeup", request, responseBuilder);
        }

        public int RenameDataset(
                igware.gvm.pb.CcdiRpc.RenameDatasetInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RenameDataset", request, responseBuilder);
        }

        public int SyncOnce(
                igware.gvm.pb.CcdiRpc.SyncOnceInput request,
                igware.gvm.pb.CcdiRpc.SyncOnceOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SyncOnce", request, responseBuilder);
        }

        public int UnlinkDevice(
                igware.gvm.pb.CcdiRpc.UnlinkDeviceInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UnlinkDevice", request, responseBuilder);
        }

        public int UpdateSyncSettings(
                igware.gvm.pb.CcdiRpc.UpdateSyncSettingsInput request,
                igware.gvm.pb.CcdiRpc.UpdateSyncSettingsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UpdateSyncSettings", request, responseBuilder);
        }

        public int UpdateSyncSubscription(
                igware.gvm.pb.CcdiRpc.UpdateSyncSubscriptionInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UpdateSyncSubscription", request, responseBuilder);
        }

        public int ListUserStorage(
                igware.gvm.pb.CcdiRpc.ListUserStorageInput request,
                igware.gvm.pb.CcdiRpc.ListUserStorageOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ListUserStorage", request, responseBuilder);
        }

        public int RemoteSwUpdateMessage(
                igware.gvm.pb.CcdiRpc.RemoteSwUpdateMessageInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RemoteSwUpdateMessage", request, responseBuilder);
        }

        public int SWUpdateCheck(
                igware.gvm.pb.CcdiRpc.SWUpdateCheckInput request,
                igware.gvm.pb.CcdiRpc.SWUpdateCheckOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SWUpdateCheck", request, responseBuilder);
        }

        public int SWUpdateBeginDownload(
                igware.gvm.pb.CcdiRpc.SWUpdateBeginDownloadInput request,
                igware.gvm.pb.CcdiRpc.SWUpdateBeginDownloadOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SWUpdateBeginDownload", request, responseBuilder);
        }

        public int SWUpdateGetDownloadProgress(
                igware.gvm.pb.CcdiRpc.SWUpdateGetDownloadProgressInput request,
                igware.gvm.pb.CcdiRpc.SWUpdateGetDownloadProgressOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SWUpdateGetDownloadProgress", request, responseBuilder);
        }

        public int SWUpdateEndDownload(
                igware.gvm.pb.CcdiRpc.SWUpdateEndDownloadInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SWUpdateEndDownload", request, responseBuilder);
        }

        public int SWUpdateCancelDownload(
                igware.gvm.pb.CcdiRpc.SWUpdateCancelDownloadInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SWUpdateCancelDownload", request, responseBuilder);
        }

        public int SWUpdateSetCcdVersion(
                igware.gvm.pb.CcdiRpc.SWUpdateSetCcdVersionInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SWUpdateSetCcdVersion", request, responseBuilder);
        }

        public int MSABeginCatalog(
                igware.gvm.pb.CcdiRpc.BeginCatalogInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSABeginCatalog", request, responseBuilder);
        }

        public int MSACommitCatalog(
                igware.gvm.pb.CcdiRpc.CommitCatalogInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSACommitCatalog", request, responseBuilder);
        }

        public int MSAEndCatalog(
                igware.gvm.pb.CcdiRpc.EndCatalogInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSAEndCatalog", request, responseBuilder);
        }

        public int MSABeginMetadataTransaction(
                igware.gvm.pb.CcdiRpc.BeginMetadataTransactionInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSABeginMetadataTransaction", request, responseBuilder);
        }

        public int MSAUpdateMetadata(
                igware.gvm.pb.CcdiRpc.UpdateMetadataInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSAUpdateMetadata", request, responseBuilder);
        }

        public int MSADeleteMetadata(
                igware.gvm.pb.CcdiRpc.DeleteMetadataInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSADeleteMetadata", request, responseBuilder);
        }

        public int MSACommitMetadataTransaction(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSACommitMetadataTransaction", request, responseBuilder);
        }

        public int MSAGetMetadataSyncState(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.cloud.media_metadata.pb.MediaMetadata.GetMetadataSyncStateOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSAGetMetadataSyncState", request, responseBuilder);
        }

        public int MSADeleteCollection(
                igware.gvm.pb.CcdiRpc.DeleteCollectionInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSADeleteCollection", request, responseBuilder);
        }

        public int MSADeleteCatalog(
                igware.gvm.pb.CcdiRpc.DeleteCatalogInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSADeleteCatalog", request, responseBuilder);
        }

        public int MSAListCollections(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.cloud.media_metadata.pb.MediaMetadata.ListCollectionsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSAListCollections", request, responseBuilder);
        }

        public int MSAGetCollectionDetails(
                igware.gvm.pb.CcdiRpc.GetCollectionDetailsInput request,
                igware.gvm.pb.CcdiRpc.GetCollectionDetailsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSAGetCollectionDetails", request, responseBuilder);
        }

        public int MSAGetContentURL(
                igware.gvm.pb.CcdiRpc.MSAGetContentURLInput request,
                igware.gvm.pb.CcdiRpc.MSAGetContentURLOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MSAGetContentURL", request, responseBuilder);
        }

        public int MCAQueryMetadataObjects(
                igware.gvm.pb.CcdiRpc.MCAQueryMetadataObjectsInput request,
                igware.gvm.pb.CcdiRpc.MCAQueryMetadataObjectsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("MCAQueryMetadataObjects", request, responseBuilder);
        }

        public int EnableInMemoryLogging(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("EnableInMemoryLogging", request, responseBuilder);
        }

        public int DisableInMemoryLogging(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("DisableInMemoryLogging", request, responseBuilder);
        }

        public int FlushInMemoryLogs(
                igware.gvm.pb.CcdiRpc.NoParamRequest request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("FlushInMemoryLogs", request, responseBuilder);
        }

        public int RespondToPairingRequest(
                igware.gvm.pb.CcdiRpc.RespondToPairingRequestInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RespondToPairingRequest", request, responseBuilder);
        }

        public int RequestPairing(
                igware.gvm.pb.CcdiRpc.RequestPairingInput request,
                igware.gvm.pb.CcdiRpc.RequestPairingOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RequestPairing", request, responseBuilder);
        }

        public int RequestPairingPin(
                igware.gvm.pb.CcdiRpc.RequestPairingPinInput request,
                igware.gvm.pb.CcdiRpc.RequestPairingPinOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RequestPairingPin", request, responseBuilder);
        }

        public int GetPairingStatus(
                igware.gvm.pb.CcdiRpc.GetPairingStatusInput request,
                igware.gvm.pb.CcdiRpc.GetPairingStatusOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("GetPairingStatus", request, responseBuilder);
        }

        public int QueryPicStreamObjects(
                igware.gvm.pb.CcdiRpc.CCDIQueryPicStreamObjectsInput request,
                igware.gvm.pb.CcdiRpc.CCDIQueryPicStreamObjectsOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("QueryPicStreamObjects", request, responseBuilder);
        }

        public int SharedFilesStoreFile(
                igware.gvm.pb.CcdiRpc.SharedFilesStoreFileInput request,
                igware.gvm.pb.CcdiRpc.SharedFilesStoreFileOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SharedFilesStoreFile", request, responseBuilder);
        }

        public int SharedFilesShareFile(
                igware.gvm.pb.CcdiRpc.SharedFilesShareFileInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SharedFilesShareFile", request, responseBuilder);
        }

        public int SharedFilesUnshareFile(
                igware.gvm.pb.CcdiRpc.SharedFilesUnshareFileInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SharedFilesUnshareFile", request, responseBuilder);
        }

        public int SharedFilesDeleteSharedFile(
                igware.gvm.pb.CcdiRpc.SharedFilesDeleteSharedFileInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SharedFilesDeleteSharedFile", request, responseBuilder);
        }

        public int SharedFilesQuery(
                igware.gvm.pb.CcdiRpc.SharedFilesQueryInput request,
                igware.gvm.pb.CcdiRpc.SharedFilesQueryOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("SharedFilesQuery", request, responseBuilder);
        }

        public int RegisterRemoteExecutable(
                igware.gvm.pb.CcdiRpc.RegisterRemoteExecutableInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("RegisterRemoteExecutable", request, responseBuilder);
        }

        public int UnregisterRemoteExecutable(
                igware.gvm.pb.CcdiRpc.UnregisterRemoteExecutableInput request,
                igware.gvm.pb.CcdiRpc.NoParamResponse.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("UnregisterRemoteExecutable", request, responseBuilder);
        }

        public int ListRegisteredRemoteExecutables(
                igware.gvm.pb.CcdiRpc.ListRegisteredRemoteExecutablesInput request,
                igware.gvm.pb.CcdiRpc.ListRegisteredRemoteExecutablesOutput.Builder responseBuilder)
        throws igware.protobuf.ProtoRpcException {
            return sendRpc("ListRegisteredRemoteExecutables", request, responseBuilder);
        }

    }

}
