//
//  Copyright 2010-2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef CC_SDK__CCDI_HPP__
#define CC_SDK__CCDI_HPP__

//============================================================================
/// @file
/// APIs for interacting with the Cloud Client.
//============================================================================

#include <vpl_types.h>

#include <ccd_features.h>
#include <ccdi_rpc.pb.h>
#include <gvm_errors.h>

typedef int32_t CCDIError;

namespace ccdi {
namespace client {

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIAddDataset(const ccd::AddDatasetInput& request,
        ccd::AddDatasetOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIAddSyncSubscription(const ccd::AddSyncSubscriptionInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIDeleteDataset(const ccd::DeleteDatasetInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIDeleteSyncSubscriptions(const ccd::DeleteSyncSubscriptionsInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIEventsCreateQueue(const ccd::EventsCreateQueueInput& request,
        ccd::EventsCreateQueueOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIEventsDestroyQueue(const ccd::EventsDestroyQueueInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIEventsDequeue(const ccd::EventsDequeueInput& request,
        ccd::EventsDequeueOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetDatasetDirectoryEntries(const ccd::GetDatasetDirectoryEntriesInput& request,
        ccd::GetDatasetDirectoryEntriesOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetInfraHttpInfo(const ccd::GetInfraHttpInfoInput& request,
        ccd::GetInfraHttpInfoOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetLocalHttpInfo(const ccd::GetLocalHttpInfoInput& request,
        ccd::GetLocalHttpInfoOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetPersonalCloudState(const ccd::GetPersonalCloudStateInput& request,
        ccd::GetPersonalCloudStateOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetSyncState(const ccd::GetSyncStateInput& request,
        ccd::GetSyncStateOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetSyncStateNotifications(const ccd::GetSyncStateNotificationsInput& request,
        ccd::GetSyncStateNotificationsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetSystemState(const ccd::GetSystemStateInput& request,
        ccd::GetSystemStateOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIInfraHttpRequest(const ccd::InfraHttpRequestInput& request,
        ccd::InfraHttpRequestOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSABeginCatalog(const ccd::BeginCatalogInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSACommitCatalog(const ccd::CommitCatalogInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSAEndCatalog(const ccd::EndCatalogInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSAUpdateMetadata(const ccd::UpdateMetadataInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSADeleteMetadata(const ccd::DeleteMetadataInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSACommitMetadataTransaction();

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSAGetMetadataSyncState(media_metadata::GetMetadataSyncStateOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSADeleteCollection(const ccd::DeleteCollectionInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSADeleteCatalog(const ccd::DeleteCatalogInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSAListCollections(media_metadata::ListCollectionsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& request,
                                      ccd::GetCollectionDetailsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMSAGetContentURL(const ccd::MSAGetContentURLInput& request,
        ccd::MSAGetContentURLOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIMCAQueryMetadataObjects(const ccd::MCAQueryMetadataObjectsInput& request,
        ccd::MCAQueryMetadataObjectsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDILinkDevice(const ccd::LinkDeviceInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIListLinkedDevices(const ccd::ListLinkedDevicesInput& request,
        ccd::ListLinkedDevicesOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIListOwnedDatasets(const ccd::ListOwnedDatasetsInput& request,
        ccd::ListOwnedDatasetsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIListSyncSubscriptions(const ccd::ListSyncSubscriptionsInput& request,
        ccd::ListSyncSubscriptionsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDILogin(const ccd::LoginInput& request,
        ccd::LoginOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDILogout(const ccd::LogoutInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIOwnershipSync();

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIPrivateMsaDataCommit(const ccd::PrivateMsaDataCommitInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRegisterStorageNode(const ccd::RegisterStorageNodeInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIReportLanDevices(const ccd::ReportLanDevicesInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISyncOnce(const ccd::SyncOnceInput& request, ccd::SyncOnceOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIListLanDevices(const ccd::ListLanDevicesInput& request, ccd::ListLanDevicesOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIProbeLanDevices();

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIListStorageNodeDatasets(ccd::ListStorageNodeDatasetsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRemoteWakeup(const ccd::RemoteWakeupInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRenameDataset(const ccd::RenameDatasetInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIListUserStorage(const ccd::ListUserStorageInput& request,
        ccd::ListUserStorageOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISWUpdateBeginDownload(const ccd::SWUpdateBeginDownloadInput& request,
        ccd::SWUpdateBeginDownloadOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISWUpdateCheck(const ccd::SWUpdateCheckInput& request,
        ccd::SWUpdateCheckOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISWUpdateEndDownload(const ccd::SWUpdateEndDownloadInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISWUpdateCancelDownload(const ccd::SWUpdateCancelDownloadInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISWUpdateGetDownloadProgress(const ccd::SWUpdateGetDownloadProgressInput& request,
        ccd::SWUpdateGetDownloadProgressOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISWUpdateSetCcdVersion(
        const ccd::SWUpdateSetCcdVersionInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUnlinkDevice(const ccd::UnlinkDeviceInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUnregisterStorageNode(const ccd::UnregisterStorageNodeInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUpdateAppState(const ccd::UpdateAppStateInput& request,
        ccd::UpdateAppStateOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUpdateStorageNode(const ccd::UpdateStorageNodeInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUpdateSyncSettings(const ccd::UpdateSyncSettingsInput& request,
        ccd::UpdateSyncSettingsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUpdateSyncSubscription(const ccd::UpdateSyncSubscriptionInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUpdateSystemState(const ccd::UpdateSystemStateInput& request,
        ccd::UpdateSystemStateOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRemoteSwUpdateMessage(const ccd::RemoteSwUpdateMessageInput& request);

#if (defined LINUX || defined __CLOUDNODE__) && !defined LINUX_EMB
/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIEnableInMemoryLogging();

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIDisableInMemoryLogging();
#endif
/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIFlushInMemoryLogs();

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRespondToPairingRequest(const ccd::RespondToPairingRequestInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRequestPairing(const ccd::RequestPairingInput& request,
        ccd::RequestPairingOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRequestPairingPin(const ccd::RequestPairingPinInput& request,
        ccd::RequestPairingPinOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIGetPairingStatus(const ccd::GetPairingStatusInput& request,
        ccd::GetPairingStatusOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIQueryPicStreamObjects(const ccd::CCDIQueryPicStreamObjectsInput& request,
    ccd::CCDIQueryPicStreamObjectsOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISharedFilesStoreFile(const ccd::SharedFilesStoreFileInput& request,
    ccd::SharedFilesStoreFileOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISharedFilesShareFile(const ccd::SharedFilesShareFileInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISharedFilesUnshareFile(const ccd::SharedFilesUnshareFileInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISharedFilesDeleteSharedFile(const ccd::SharedFilesDeleteSharedFileInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDISharedFilesQuery(const ccd::SharedFilesQueryInput& request,
    ccd::SharedFilesQueryOutput& response);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIRegisterRemoteExecutable(const ccd::RegisterRemoteExecutableInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIUnregisterRemoteExecutable(const ccd::UnregisterRemoteExecutableInput& request);

/// Please refer to ccdi_rpc.proto for parameter documentation.
CCDIError CCDIListRegisteredRemoteExecutables(const ccd::ListRegisteredRemoteExecutablesInput& request,
        ccd::ListRegisteredRemoteExecutablesOutput& response);

} // namespace client
} // namespace ccdi

// TODO: temporary; update the samples to use the namespaces, then remove this.
using namespace ccdi::client;

#endif // include guard
