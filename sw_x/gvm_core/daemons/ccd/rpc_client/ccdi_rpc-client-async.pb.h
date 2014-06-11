// GENERATED FROM ccdi_rpc.proto, DO NOT EDIT

#ifndef __ccdi_rpc_CLIENT_ASYNC_PB_H__
#define __ccdi_rpc_CLIENT_ASYNC_PB_H__

#include "ccdi_rpc-client.pb.h"
#include <rpc.pb.h>
#include <ProtoChannel.h>
#include <ProtoRpcClientAsync.h>

namespace ccd {

class CCDIServiceClientAsync : public proto_rpc::ServiceClientAsync
{
public:
    /// The #AsyncServiceClientState can safely be shared by multiple service clients.
    CCDIServiceClientAsync(
            proto_rpc::AsyncServiceClientState& asyncState,
            AcquireClientCallback acquireClient,
            ReleaseClientCallback releaseClient);

    /// Retrieve and process the response for a completed request.
    /// This method will dequeue the next available response and invoke the
    /// callback that was specified when the request was made.
    /// The callback will be invoked in this thread's context.
    /// If no results are ready, this will immediately return.
    int ProcessAsyncResponse();

    // ---------------------------------------
    // EventsCreateQueue
    // ---------------------------------------
    typedef void (*EventsCreateQueueCallback)(int requestId, void* param,
            ccd::EventsCreateQueueOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.EventsCreateQueue().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int EventsCreateQueueAsync(const ccd::EventsCreateQueueInput& request,
            EventsCreateQueueCallback callback, void* callbackParam);

    // ---------------------------------------
    // EventsDestroyQueue
    // ---------------------------------------
    typedef void (*EventsDestroyQueueCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.EventsDestroyQueue().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int EventsDestroyQueueAsync(const ccd::EventsDestroyQueueInput& request,
            EventsDestroyQueueCallback callback, void* callbackParam);

    // ---------------------------------------
    // EventsDequeue
    // ---------------------------------------
    typedef void (*EventsDequeueCallback)(int requestId, void* param,
            ccd::EventsDequeueOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.EventsDequeue().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int EventsDequeueAsync(const ccd::EventsDequeueInput& request,
            EventsDequeueCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetSystemState
    // ---------------------------------------
    typedef void (*GetSystemStateCallback)(int requestId, void* param,
            ccd::GetSystemStateOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetSystemState().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetSystemStateAsync(const ccd::GetSystemStateInput& request,
            GetSystemStateCallback callback, void* callbackParam);

    // ---------------------------------------
    // Login
    // ---------------------------------------
    typedef void (*LoginCallback)(int requestId, void* param,
            ccd::LoginOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.Login().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int LoginAsync(const ccd::LoginInput& request,
            LoginCallback callback, void* callbackParam);

    // ---------------------------------------
    // Logout
    // ---------------------------------------
    typedef void (*LogoutCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.Logout().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int LogoutAsync(const ccd::LogoutInput& request,
            LogoutCallback callback, void* callbackParam);

    // ---------------------------------------
    // InfraHttpRequest
    // ---------------------------------------
    typedef void (*InfraHttpRequestCallback)(int requestId, void* param,
            ccd::InfraHttpRequestOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.InfraHttpRequest().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int InfraHttpRequestAsync(const ccd::InfraHttpRequestInput& request,
            InfraHttpRequestCallback callback, void* callbackParam);

    // ---------------------------------------
    // UpdateAppState
    // ---------------------------------------
    typedef void (*UpdateAppStateCallback)(int requestId, void* param,
            ccd::UpdateAppStateOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UpdateAppState().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UpdateAppStateAsync(const ccd::UpdateAppStateInput& request,
            UpdateAppStateCallback callback, void* callbackParam);

    // ---------------------------------------
    // UpdateSystemState
    // ---------------------------------------
    typedef void (*UpdateSystemStateCallback)(int requestId, void* param,
            ccd::UpdateSystemStateOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UpdateSystemState().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UpdateSystemStateAsync(const ccd::UpdateSystemStateInput& request,
            UpdateSystemStateCallback callback, void* callbackParam);

    // ---------------------------------------
    // RegisterStorageNode
    // ---------------------------------------
    typedef void (*RegisterStorageNodeCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RegisterStorageNode().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RegisterStorageNodeAsync(const ccd::RegisterStorageNodeInput& request,
            RegisterStorageNodeCallback callback, void* callbackParam);

    // ---------------------------------------
    // UnregisterStorageNode
    // ---------------------------------------
    typedef void (*UnregisterStorageNodeCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UnregisterStorageNode().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UnregisterStorageNodeAsync(const ccd::UnregisterStorageNodeInput& request,
            UnregisterStorageNodeCallback callback, void* callbackParam);

    // ---------------------------------------
    // UpdateStorageNode
    // ---------------------------------------
    typedef void (*UpdateStorageNodeCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UpdateStorageNode().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UpdateStorageNodeAsync(const ccd::UpdateStorageNodeInput& request,
            UpdateStorageNodeCallback callback, void* callbackParam);

    // ---------------------------------------
    // ReportLanDevices
    // ---------------------------------------
    typedef void (*ReportLanDevicesCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ReportLanDevices().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ReportLanDevicesAsync(const ccd::ReportLanDevicesInput& request,
            ReportLanDevicesCallback callback, void* callbackParam);

    // ---------------------------------------
    // ListLanDevices
    // ---------------------------------------
    typedef void (*ListLanDevicesCallback)(int requestId, void* param,
            ccd::ListLanDevicesOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ListLanDevices().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ListLanDevicesAsync(const ccd::ListLanDevicesInput& request,
            ListLanDevicesCallback callback, void* callbackParam);

    // ---------------------------------------
    // ProbeLanDevices
    // ---------------------------------------
    typedef void (*ProbeLanDevicesCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ProbeLanDevices().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ProbeLanDevicesAsync(const ccd::NoParamRequest& request,
            ProbeLanDevicesCallback callback, void* callbackParam);

    // ---------------------------------------
    // ListStorageNodeDatasets
    // ---------------------------------------
    typedef void (*ListStorageNodeDatasetsCallback)(int requestId, void* param,
            ccd::ListStorageNodeDatasetsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ListStorageNodeDatasets().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ListStorageNodeDatasetsAsync(const ccd::NoParamRequest& request,
            ListStorageNodeDatasetsCallback callback, void* callbackParam);

    // ---------------------------------------
    // AddDataset
    // ---------------------------------------
    typedef void (*AddDatasetCallback)(int requestId, void* param,
            ccd::AddDatasetOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.AddDataset().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int AddDatasetAsync(const ccd::AddDatasetInput& request,
            AddDatasetCallback callback, void* callbackParam);

    // ---------------------------------------
    // AddSyncSubscription
    // ---------------------------------------
    typedef void (*AddSyncSubscriptionCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.AddSyncSubscription().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int AddSyncSubscriptionAsync(const ccd::AddSyncSubscriptionInput& request,
            AddSyncSubscriptionCallback callback, void* callbackParam);

    // ---------------------------------------
    // DeleteDataset
    // ---------------------------------------
    typedef void (*DeleteDatasetCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.DeleteDataset().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int DeleteDatasetAsync(const ccd::DeleteDatasetInput& request,
            DeleteDatasetCallback callback, void* callbackParam);

    // ---------------------------------------
    // DeleteSyncSubscriptions
    // ---------------------------------------
    typedef void (*DeleteSyncSubscriptionsCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.DeleteSyncSubscriptions().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int DeleteSyncSubscriptionsAsync(const ccd::DeleteSyncSubscriptionsInput& request,
            DeleteSyncSubscriptionsCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetDatasetDirectoryEntries
    // ---------------------------------------
    typedef void (*GetDatasetDirectoryEntriesCallback)(int requestId, void* param,
            ccd::GetDatasetDirectoryEntriesOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetDatasetDirectoryEntries().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetDatasetDirectoryEntriesAsync(const ccd::GetDatasetDirectoryEntriesInput& request,
            GetDatasetDirectoryEntriesCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetInfraHttpInfo
    // ---------------------------------------
    typedef void (*GetInfraHttpInfoCallback)(int requestId, void* param,
            ccd::GetInfraHttpInfoOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetInfraHttpInfo().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetInfraHttpInfoAsync(const ccd::GetInfraHttpInfoInput& request,
            GetInfraHttpInfoCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetLocalHttpInfo
    // ---------------------------------------
    typedef void (*GetLocalHttpInfoCallback)(int requestId, void* param,
            ccd::GetLocalHttpInfoOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetLocalHttpInfo().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetLocalHttpInfoAsync(const ccd::GetLocalHttpInfoInput& request,
            GetLocalHttpInfoCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetPersonalCloudState
    // ---------------------------------------
    typedef void (*GetPersonalCloudStateCallback)(int requestId, void* param,
            ccd::GetPersonalCloudStateOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetPersonalCloudState().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetPersonalCloudStateAsync(const ccd::GetPersonalCloudStateInput& request,
            GetPersonalCloudStateCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetSyncState
    // ---------------------------------------
    typedef void (*GetSyncStateCallback)(int requestId, void* param,
            ccd::GetSyncStateOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetSyncState().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetSyncStateAsync(const ccd::GetSyncStateInput& request,
            GetSyncStateCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetSyncStateNotifications
    // ---------------------------------------
    typedef void (*GetSyncStateNotificationsCallback)(int requestId, void* param,
            ccd::GetSyncStateNotificationsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetSyncStateNotifications().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetSyncStateNotificationsAsync(const ccd::GetSyncStateNotificationsInput& request,
            GetSyncStateNotificationsCallback callback, void* callbackParam);

    // ---------------------------------------
    // LinkDevice
    // ---------------------------------------
    typedef void (*LinkDeviceCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.LinkDevice().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int LinkDeviceAsync(const ccd::LinkDeviceInput& request,
            LinkDeviceCallback callback, void* callbackParam);

    // ---------------------------------------
    // ListLinkedDevices
    // ---------------------------------------
    typedef void (*ListLinkedDevicesCallback)(int requestId, void* param,
            ccd::ListLinkedDevicesOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ListLinkedDevices().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ListLinkedDevicesAsync(const ccd::ListLinkedDevicesInput& request,
            ListLinkedDevicesCallback callback, void* callbackParam);

    // ---------------------------------------
    // ListOwnedDatasets
    // ---------------------------------------
    typedef void (*ListOwnedDatasetsCallback)(int requestId, void* param,
            ccd::ListOwnedDatasetsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ListOwnedDatasets().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ListOwnedDatasetsAsync(const ccd::ListOwnedDatasetsInput& request,
            ListOwnedDatasetsCallback callback, void* callbackParam);

    // ---------------------------------------
    // ListSyncSubscriptions
    // ---------------------------------------
    typedef void (*ListSyncSubscriptionsCallback)(int requestId, void* param,
            ccd::ListSyncSubscriptionsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ListSyncSubscriptions().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ListSyncSubscriptionsAsync(const ccd::ListSyncSubscriptionsInput& request,
            ListSyncSubscriptionsCallback callback, void* callbackParam);

    // ---------------------------------------
    // OwnershipSync
    // ---------------------------------------
    typedef void (*OwnershipSyncCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.OwnershipSync().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int OwnershipSyncAsync(const ccd::NoParamRequest& request,
            OwnershipSyncCallback callback, void* callbackParam);

    // ---------------------------------------
    // PrivateMsaDataCommit
    // ---------------------------------------
    typedef void (*PrivateMsaDataCommitCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.PrivateMsaDataCommit().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int PrivateMsaDataCommitAsync(const ccd::PrivateMsaDataCommitInput& request,
            PrivateMsaDataCommitCallback callback, void* callbackParam);

    // ---------------------------------------
    // RemoteWakeup
    // ---------------------------------------
    typedef void (*RemoteWakeupCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RemoteWakeup().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RemoteWakeupAsync(const ccd::RemoteWakeupInput& request,
            RemoteWakeupCallback callback, void* callbackParam);

    // ---------------------------------------
    // RenameDataset
    // ---------------------------------------
    typedef void (*RenameDatasetCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RenameDataset().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RenameDatasetAsync(const ccd::RenameDatasetInput& request,
            RenameDatasetCallback callback, void* callbackParam);

    // ---------------------------------------
    // SyncOnce
    // ---------------------------------------
    typedef void (*SyncOnceCallback)(int requestId, void* param,
            ccd::SyncOnceOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SyncOnce().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SyncOnceAsync(const ccd::SyncOnceInput& request,
            SyncOnceCallback callback, void* callbackParam);

    // ---------------------------------------
    // UnlinkDevice
    // ---------------------------------------
    typedef void (*UnlinkDeviceCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UnlinkDevice().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UnlinkDeviceAsync(const ccd::UnlinkDeviceInput& request,
            UnlinkDeviceCallback callback, void* callbackParam);

    // ---------------------------------------
    // UpdateSyncSettings
    // ---------------------------------------
    typedef void (*UpdateSyncSettingsCallback)(int requestId, void* param,
            ccd::UpdateSyncSettingsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UpdateSyncSettings().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UpdateSyncSettingsAsync(const ccd::UpdateSyncSettingsInput& request,
            UpdateSyncSettingsCallback callback, void* callbackParam);

    // ---------------------------------------
    // UpdateSyncSubscription
    // ---------------------------------------
    typedef void (*UpdateSyncSubscriptionCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UpdateSyncSubscription().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UpdateSyncSubscriptionAsync(const ccd::UpdateSyncSubscriptionInput& request,
            UpdateSyncSubscriptionCallback callback, void* callbackParam);

    // ---------------------------------------
    // ListUserStorage
    // ---------------------------------------
    typedef void (*ListUserStorageCallback)(int requestId, void* param,
            ccd::ListUserStorageOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ListUserStorage().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ListUserStorageAsync(const ccd::ListUserStorageInput& request,
            ListUserStorageCallback callback, void* callbackParam);

    // ---------------------------------------
    // RemoteSwUpdateMessage
    // ---------------------------------------
    typedef void (*RemoteSwUpdateMessageCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RemoteSwUpdateMessage().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RemoteSwUpdateMessageAsync(const ccd::RemoteSwUpdateMessageInput& request,
            RemoteSwUpdateMessageCallback callback, void* callbackParam);

    // ---------------------------------------
    // SWUpdateCheck
    // ---------------------------------------
    typedef void (*SWUpdateCheckCallback)(int requestId, void* param,
            ccd::SWUpdateCheckOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SWUpdateCheck().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SWUpdateCheckAsync(const ccd::SWUpdateCheckInput& request,
            SWUpdateCheckCallback callback, void* callbackParam);

    // ---------------------------------------
    // SWUpdateBeginDownload
    // ---------------------------------------
    typedef void (*SWUpdateBeginDownloadCallback)(int requestId, void* param,
            ccd::SWUpdateBeginDownloadOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SWUpdateBeginDownload().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SWUpdateBeginDownloadAsync(const ccd::SWUpdateBeginDownloadInput& request,
            SWUpdateBeginDownloadCallback callback, void* callbackParam);

    // ---------------------------------------
    // SWUpdateGetDownloadProgress
    // ---------------------------------------
    typedef void (*SWUpdateGetDownloadProgressCallback)(int requestId, void* param,
            ccd::SWUpdateGetDownloadProgressOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SWUpdateGetDownloadProgress().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SWUpdateGetDownloadProgressAsync(const ccd::SWUpdateGetDownloadProgressInput& request,
            SWUpdateGetDownloadProgressCallback callback, void* callbackParam);

    // ---------------------------------------
    // SWUpdateEndDownload
    // ---------------------------------------
    typedef void (*SWUpdateEndDownloadCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SWUpdateEndDownload().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SWUpdateEndDownloadAsync(const ccd::SWUpdateEndDownloadInput& request,
            SWUpdateEndDownloadCallback callback, void* callbackParam);

    // ---------------------------------------
    // SWUpdateCancelDownload
    // ---------------------------------------
    typedef void (*SWUpdateCancelDownloadCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SWUpdateCancelDownload().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SWUpdateCancelDownloadAsync(const ccd::SWUpdateCancelDownloadInput& request,
            SWUpdateCancelDownloadCallback callback, void* callbackParam);

    // ---------------------------------------
    // SWUpdateSetCcdVersion
    // ---------------------------------------
    typedef void (*SWUpdateSetCcdVersionCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SWUpdateSetCcdVersion().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SWUpdateSetCcdVersionAsync(const ccd::SWUpdateSetCcdVersionInput& request,
            SWUpdateSetCcdVersionCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSABeginCatalog
    // ---------------------------------------
    typedef void (*MSABeginCatalogCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSABeginCatalog().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSABeginCatalogAsync(const ccd::BeginCatalogInput& request,
            MSABeginCatalogCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSACommitCatalog
    // ---------------------------------------
    typedef void (*MSACommitCatalogCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSACommitCatalog().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSACommitCatalogAsync(const ccd::CommitCatalogInput& request,
            MSACommitCatalogCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSAEndCatalog
    // ---------------------------------------
    typedef void (*MSAEndCatalogCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSAEndCatalog().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSAEndCatalogAsync(const ccd::EndCatalogInput& request,
            MSAEndCatalogCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSABeginMetadataTransaction
    // ---------------------------------------
    typedef void (*MSABeginMetadataTransactionCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSABeginMetadataTransaction().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSABeginMetadataTransactionAsync(const ccd::BeginMetadataTransactionInput& request,
            MSABeginMetadataTransactionCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSAUpdateMetadata
    // ---------------------------------------
    typedef void (*MSAUpdateMetadataCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSAUpdateMetadata().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSAUpdateMetadataAsync(const ccd::UpdateMetadataInput& request,
            MSAUpdateMetadataCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSADeleteMetadata
    // ---------------------------------------
    typedef void (*MSADeleteMetadataCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSADeleteMetadata().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSADeleteMetadataAsync(const ccd::DeleteMetadataInput& request,
            MSADeleteMetadataCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSACommitMetadataTransaction
    // ---------------------------------------
    typedef void (*MSACommitMetadataTransactionCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSACommitMetadataTransaction().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSACommitMetadataTransactionAsync(const ccd::NoParamRequest& request,
            MSACommitMetadataTransactionCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSAGetMetadataSyncState
    // ---------------------------------------
    typedef void (*MSAGetMetadataSyncStateCallback)(int requestId, void* param,
            media_metadata::GetMetadataSyncStateOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSAGetMetadataSyncState().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSAGetMetadataSyncStateAsync(const ccd::NoParamRequest& request,
            MSAGetMetadataSyncStateCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSADeleteCollection
    // ---------------------------------------
    typedef void (*MSADeleteCollectionCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSADeleteCollection().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSADeleteCollectionAsync(const ccd::DeleteCollectionInput& request,
            MSADeleteCollectionCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSADeleteCatalog
    // ---------------------------------------
    typedef void (*MSADeleteCatalogCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSADeleteCatalog().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSADeleteCatalogAsync(const ccd::DeleteCatalogInput& request,
            MSADeleteCatalogCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSAListCollections
    // ---------------------------------------
    typedef void (*MSAListCollectionsCallback)(int requestId, void* param,
            media_metadata::ListCollectionsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSAListCollections().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSAListCollectionsAsync(const ccd::NoParamRequest& request,
            MSAListCollectionsCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSAGetCollectionDetails
    // ---------------------------------------
    typedef void (*MSAGetCollectionDetailsCallback)(int requestId, void* param,
            ccd::GetCollectionDetailsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSAGetCollectionDetails().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSAGetCollectionDetailsAsync(const ccd::GetCollectionDetailsInput& request,
            MSAGetCollectionDetailsCallback callback, void* callbackParam);

    // ---------------------------------------
    // MSAGetContentURL
    // ---------------------------------------
    typedef void (*MSAGetContentURLCallback)(int requestId, void* param,
            ccd::MSAGetContentURLOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MSAGetContentURL().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MSAGetContentURLAsync(const ccd::MSAGetContentURLInput& request,
            MSAGetContentURLCallback callback, void* callbackParam);

    // ---------------------------------------
    // MCAQueryMetadataObjects
    // ---------------------------------------
    typedef void (*MCAQueryMetadataObjectsCallback)(int requestId, void* param,
            ccd::MCAQueryMetadataObjectsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.MCAQueryMetadataObjects().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int MCAQueryMetadataObjectsAsync(const ccd::MCAQueryMetadataObjectsInput& request,
            MCAQueryMetadataObjectsCallback callback, void* callbackParam);

    // ---------------------------------------
    // EnableInMemoryLogging
    // ---------------------------------------
    typedef void (*EnableInMemoryLoggingCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.EnableInMemoryLogging().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int EnableInMemoryLoggingAsync(const ccd::NoParamRequest& request,
            EnableInMemoryLoggingCallback callback, void* callbackParam);

    // ---------------------------------------
    // DisableInMemoryLogging
    // ---------------------------------------
    typedef void (*DisableInMemoryLoggingCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.DisableInMemoryLogging().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int DisableInMemoryLoggingAsync(const ccd::NoParamRequest& request,
            DisableInMemoryLoggingCallback callback, void* callbackParam);

    // ---------------------------------------
    // FlushInMemoryLogs
    // ---------------------------------------
    typedef void (*FlushInMemoryLogsCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.FlushInMemoryLogs().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int FlushInMemoryLogsAsync(const ccd::NoParamRequest& request,
            FlushInMemoryLogsCallback callback, void* callbackParam);

    // ---------------------------------------
    // RespondToPairingRequest
    // ---------------------------------------
    typedef void (*RespondToPairingRequestCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RespondToPairingRequest().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RespondToPairingRequestAsync(const ccd::RespondToPairingRequestInput& request,
            RespondToPairingRequestCallback callback, void* callbackParam);

    // ---------------------------------------
    // RequestPairing
    // ---------------------------------------
    typedef void (*RequestPairingCallback)(int requestId, void* param,
            ccd::RequestPairingOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RequestPairing().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RequestPairingAsync(const ccd::RequestPairingInput& request,
            RequestPairingCallback callback, void* callbackParam);

    // ---------------------------------------
    // RequestPairingPin
    // ---------------------------------------
    typedef void (*RequestPairingPinCallback)(int requestId, void* param,
            ccd::RequestPairingPinOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RequestPairingPin().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RequestPairingPinAsync(const ccd::RequestPairingPinInput& request,
            RequestPairingPinCallback callback, void* callbackParam);

    // ---------------------------------------
    // GetPairingStatus
    // ---------------------------------------
    typedef void (*GetPairingStatusCallback)(int requestId, void* param,
            ccd::GetPairingStatusOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.GetPairingStatus().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int GetPairingStatusAsync(const ccd::GetPairingStatusInput& request,
            GetPairingStatusCallback callback, void* callbackParam);

    // ---------------------------------------
    // QueryPicStreamObjects
    // ---------------------------------------
    typedef void (*QueryPicStreamObjectsCallback)(int requestId, void* param,
            ccd::CCDIQueryPicStreamObjectsOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.QueryPicStreamObjects().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int QueryPicStreamObjectsAsync(const ccd::CCDIQueryPicStreamObjectsInput& request,
            QueryPicStreamObjectsCallback callback, void* callbackParam);

    // ---------------------------------------
    // SharedFilesStoreFile
    // ---------------------------------------
    typedef void (*SharedFilesStoreFileCallback)(int requestId, void* param,
            ccd::SharedFilesStoreFileOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SharedFilesStoreFile().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SharedFilesStoreFileAsync(const ccd::SharedFilesStoreFileInput& request,
            SharedFilesStoreFileCallback callback, void* callbackParam);

    // ---------------------------------------
    // SharedFilesShareFile
    // ---------------------------------------
    typedef void (*SharedFilesShareFileCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SharedFilesShareFile().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SharedFilesShareFileAsync(const ccd::SharedFilesShareFileInput& request,
            SharedFilesShareFileCallback callback, void* callbackParam);

    // ---------------------------------------
    // SharedFilesUnshareFile
    // ---------------------------------------
    typedef void (*SharedFilesUnshareFileCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SharedFilesUnshareFile().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SharedFilesUnshareFileAsync(const ccd::SharedFilesUnshareFileInput& request,
            SharedFilesUnshareFileCallback callback, void* callbackParam);

    // ---------------------------------------
    // SharedFilesDeleteSharedFile
    // ---------------------------------------
    typedef void (*SharedFilesDeleteSharedFileCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SharedFilesDeleteSharedFile().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SharedFilesDeleteSharedFileAsync(const ccd::SharedFilesDeleteSharedFileInput& request,
            SharedFilesDeleteSharedFileCallback callback, void* callbackParam);

    // ---------------------------------------
    // SharedFilesQuery
    // ---------------------------------------
    typedef void (*SharedFilesQueryCallback)(int requestId, void* param,
            ccd::SharedFilesQueryOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.SharedFilesQuery().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int SharedFilesQueryAsync(const ccd::SharedFilesQueryInput& request,
            SharedFilesQueryCallback callback, void* callbackParam);

    // ---------------------------------------
    // RegisterRemoteExecutable
    // ---------------------------------------
    typedef void (*RegisterRemoteExecutableCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.RegisterRemoteExecutable().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int RegisterRemoteExecutableAsync(const ccd::RegisterRemoteExecutableInput& request,
            RegisterRemoteExecutableCallback callback, void* callbackParam);

    // ---------------------------------------
    // UnregisterRemoteExecutable
    // ---------------------------------------
    typedef void (*UnregisterRemoteExecutableCallback)(int requestId, void* param,
            ccd::NoParamResponse& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.UnregisterRemoteExecutable().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int UnregisterRemoteExecutableAsync(const ccd::UnregisterRemoteExecutableInput& request,
            UnregisterRemoteExecutableCallback callback, void* callbackParam);

    // ---------------------------------------
    // ListRegisteredRemoteExecutables
    // ---------------------------------------
    typedef void (*ListRegisteredRemoteExecutablesCallback)(int requestId, void* param,
            ccd::ListRegisteredRemoteExecutablesOutput& response, RpcStatus& status);

    /// Asynchronously call the RPC implemented by #CCDIService.ListRegisteredRemoteExecutables().
    /// @return positive requestId if the request was successfully queued,
    ///     or a negative error code if we couldn't queue the request.
    int ListRegisteredRemoteExecutablesAsync(const ccd::ListRegisteredRemoteExecutablesInput& request,
            ListRegisteredRemoteExecutablesCallback callback, void* callbackParam);

};

} // namespace ccd

#endif /* __ccdi_rpc_CLIENT_ASYNC_PB_H__ */
