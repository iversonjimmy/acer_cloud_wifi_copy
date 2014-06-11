// GENERATED FROM ccdi_rpc.proto, DO NOT EDIT

#include "ccdi_rpc-server.pb.h"
#include <memory>
#include <string>
#include "ccdi_rpc.pb.h"
#include <rpc.pb.h>
#include <ProtoChannel.h>
#include <ProtoRpcServer.h>

using std::string;

bool
ccd::CCDIService::handleRpc(ProtoChannel& _channel,
                             DebugMsgCallback debugCallback,
                             DebugRequestMsgCallback debugRequestCallback,
                             DebugResponseMsgCallback debugResponseCallback)
{
    bool success = true;
    RpcRequestHeader header;
    RpcStatus status;
    std::auto_ptr<ProtoMessage> response;
    do {
        if (_channel.inputStreamError() || _channel.outputStreamError()) {
            break;
        }
        if (!_channel.extractMessage(header)) {
            if (!_channel.inputStreamError()) {
                status.set_status(RpcStatus::HEADER_ERROR);
                string error("Error in request header: ");
                error.append(header.InitializationErrorString());
                status.set_errordetail(error);
            }
            break;
        }
        debugLogInvoke(debugCallback, header.methodname());
        if (header.methodname().compare("EventsCreateQueue") == 0) {
            ccd::EventsCreateQueueInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::EventsCreateQueueOutput* typedResponse = new ccd::EventsCreateQueueOutput();
            response.reset(typedResponse);
            status.set_appstatus(EventsCreateQueue(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("EventsDestroyQueue") == 0) {
            ccd::EventsDestroyQueueInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(EventsDestroyQueue(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("EventsDequeue") == 0) {
            ccd::EventsDequeueInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::EventsDequeueOutput* typedResponse = new ccd::EventsDequeueOutput();
            response.reset(typedResponse);
            status.set_appstatus(EventsDequeue(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetSystemState") == 0) {
            ccd::GetSystemStateInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetSystemStateOutput* typedResponse = new ccd::GetSystemStateOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetSystemState(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("Login") == 0) {
            ccd::LoginInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::LoginOutput* typedResponse = new ccd::LoginOutput();
            response.reset(typedResponse);
            status.set_appstatus(Login(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("Logout") == 0) {
            ccd::LogoutInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(Logout(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("InfraHttpRequest") == 0) {
            ccd::InfraHttpRequestInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::InfraHttpRequestOutput* typedResponse = new ccd::InfraHttpRequestOutput();
            response.reset(typedResponse);
            status.set_appstatus(InfraHttpRequest(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UpdateAppState") == 0) {
            ccd::UpdateAppStateInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::UpdateAppStateOutput* typedResponse = new ccd::UpdateAppStateOutput();
            response.reset(typedResponse);
            status.set_appstatus(UpdateAppState(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UpdateSystemState") == 0) {
            ccd::UpdateSystemStateInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::UpdateSystemStateOutput* typedResponse = new ccd::UpdateSystemStateOutput();
            response.reset(typedResponse);
            status.set_appstatus(UpdateSystemState(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RegisterStorageNode") == 0) {
            ccd::RegisterStorageNodeInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(RegisterStorageNode(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UnregisterStorageNode") == 0) {
            ccd::UnregisterStorageNodeInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(UnregisterStorageNode(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UpdateStorageNode") == 0) {
            ccd::UpdateStorageNodeInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(UpdateStorageNode(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ReportLanDevices") == 0) {
            ccd::ReportLanDevicesInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(ReportLanDevices(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ListLanDevices") == 0) {
            ccd::ListLanDevicesInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::ListLanDevicesOutput* typedResponse = new ccd::ListLanDevicesOutput();
            response.reset(typedResponse);
            status.set_appstatus(ListLanDevices(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ProbeLanDevices") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(ProbeLanDevices(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ListStorageNodeDatasets") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::ListStorageNodeDatasetsOutput* typedResponse = new ccd::ListStorageNodeDatasetsOutput();
            response.reset(typedResponse);
            status.set_appstatus(ListStorageNodeDatasets(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("AddDataset") == 0) {
            ccd::AddDatasetInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::AddDatasetOutput* typedResponse = new ccd::AddDatasetOutput();
            response.reset(typedResponse);
            status.set_appstatus(AddDataset(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("AddSyncSubscription") == 0) {
            ccd::AddSyncSubscriptionInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(AddSyncSubscription(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("DeleteDataset") == 0) {
            ccd::DeleteDatasetInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(DeleteDataset(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("DeleteSyncSubscriptions") == 0) {
            ccd::DeleteSyncSubscriptionsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(DeleteSyncSubscriptions(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetDatasetDirectoryEntries") == 0) {
            ccd::GetDatasetDirectoryEntriesInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetDatasetDirectoryEntriesOutput* typedResponse = new ccd::GetDatasetDirectoryEntriesOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetDatasetDirectoryEntries(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetInfraHttpInfo") == 0) {
            ccd::GetInfraHttpInfoInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetInfraHttpInfoOutput* typedResponse = new ccd::GetInfraHttpInfoOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetInfraHttpInfo(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetLocalHttpInfo") == 0) {
            ccd::GetLocalHttpInfoInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetLocalHttpInfoOutput* typedResponse = new ccd::GetLocalHttpInfoOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetLocalHttpInfo(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetPersonalCloudState") == 0) {
            ccd::GetPersonalCloudStateInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetPersonalCloudStateOutput* typedResponse = new ccd::GetPersonalCloudStateOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetPersonalCloudState(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetSyncState") == 0) {
            ccd::GetSyncStateInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetSyncStateOutput* typedResponse = new ccd::GetSyncStateOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetSyncState(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetSyncStateNotifications") == 0) {
            ccd::GetSyncStateNotificationsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetSyncStateNotificationsOutput* typedResponse = new ccd::GetSyncStateNotificationsOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetSyncStateNotifications(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("LinkDevice") == 0) {
            ccd::LinkDeviceInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(LinkDevice(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ListLinkedDevices") == 0) {
            ccd::ListLinkedDevicesInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::ListLinkedDevicesOutput* typedResponse = new ccd::ListLinkedDevicesOutput();
            response.reset(typedResponse);
            status.set_appstatus(ListLinkedDevices(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ListOwnedDatasets") == 0) {
            ccd::ListOwnedDatasetsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::ListOwnedDatasetsOutput* typedResponse = new ccd::ListOwnedDatasetsOutput();
            response.reset(typedResponse);
            status.set_appstatus(ListOwnedDatasets(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ListSyncSubscriptions") == 0) {
            ccd::ListSyncSubscriptionsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::ListSyncSubscriptionsOutput* typedResponse = new ccd::ListSyncSubscriptionsOutput();
            response.reset(typedResponse);
            status.set_appstatus(ListSyncSubscriptions(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("OwnershipSync") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(OwnershipSync(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("PrivateMsaDataCommit") == 0) {
            ccd::PrivateMsaDataCommitInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(PrivateMsaDataCommit(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RemoteWakeup") == 0) {
            ccd::RemoteWakeupInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(RemoteWakeup(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RenameDataset") == 0) {
            ccd::RenameDatasetInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(RenameDataset(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SyncOnce") == 0) {
            ccd::SyncOnceInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::SyncOnceOutput* typedResponse = new ccd::SyncOnceOutput();
            response.reset(typedResponse);
            status.set_appstatus(SyncOnce(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UnlinkDevice") == 0) {
            ccd::UnlinkDeviceInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(UnlinkDevice(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UpdateSyncSettings") == 0) {
            ccd::UpdateSyncSettingsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::UpdateSyncSettingsOutput* typedResponse = new ccd::UpdateSyncSettingsOutput();
            response.reset(typedResponse);
            status.set_appstatus(UpdateSyncSettings(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UpdateSyncSubscription") == 0) {
            ccd::UpdateSyncSubscriptionInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(UpdateSyncSubscription(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ListUserStorage") == 0) {
            ccd::ListUserStorageInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::ListUserStorageOutput* typedResponse = new ccd::ListUserStorageOutput();
            response.reset(typedResponse);
            status.set_appstatus(ListUserStorage(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RemoteSwUpdateMessage") == 0) {
            ccd::RemoteSwUpdateMessageInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(RemoteSwUpdateMessage(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SWUpdateCheck") == 0) {
            ccd::SWUpdateCheckInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::SWUpdateCheckOutput* typedResponse = new ccd::SWUpdateCheckOutput();
            response.reset(typedResponse);
            status.set_appstatus(SWUpdateCheck(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SWUpdateBeginDownload") == 0) {
            ccd::SWUpdateBeginDownloadInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::SWUpdateBeginDownloadOutput* typedResponse = new ccd::SWUpdateBeginDownloadOutput();
            response.reset(typedResponse);
            status.set_appstatus(SWUpdateBeginDownload(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SWUpdateGetDownloadProgress") == 0) {
            ccd::SWUpdateGetDownloadProgressInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::SWUpdateGetDownloadProgressOutput* typedResponse = new ccd::SWUpdateGetDownloadProgressOutput();
            response.reset(typedResponse);
            status.set_appstatus(SWUpdateGetDownloadProgress(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SWUpdateEndDownload") == 0) {
            ccd::SWUpdateEndDownloadInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(SWUpdateEndDownload(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SWUpdateCancelDownload") == 0) {
            ccd::SWUpdateCancelDownloadInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(SWUpdateCancelDownload(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SWUpdateSetCcdVersion") == 0) {
            ccd::SWUpdateSetCcdVersionInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(SWUpdateSetCcdVersion(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSABeginCatalog") == 0) {
            ccd::BeginCatalogInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSABeginCatalog(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSACommitCatalog") == 0) {
            ccd::CommitCatalogInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSACommitCatalog(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSAEndCatalog") == 0) {
            ccd::EndCatalogInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSAEndCatalog(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSABeginMetadataTransaction") == 0) {
            ccd::BeginMetadataTransactionInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSABeginMetadataTransaction(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSAUpdateMetadata") == 0) {
            ccd::UpdateMetadataInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSAUpdateMetadata(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSADeleteMetadata") == 0) {
            ccd::DeleteMetadataInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSADeleteMetadata(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSACommitMetadataTransaction") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSACommitMetadataTransaction(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSAGetMetadataSyncState") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            media_metadata::GetMetadataSyncStateOutput* typedResponse = new media_metadata::GetMetadataSyncStateOutput();
            response.reset(typedResponse);
            status.set_appstatus(MSAGetMetadataSyncState(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSADeleteCollection") == 0) {
            ccd::DeleteCollectionInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSADeleteCollection(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSADeleteCatalog") == 0) {
            ccd::DeleteCatalogInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(MSADeleteCatalog(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSAListCollections") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            media_metadata::ListCollectionsOutput* typedResponse = new media_metadata::ListCollectionsOutput();
            response.reset(typedResponse);
            status.set_appstatus(MSAListCollections(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSAGetCollectionDetails") == 0) {
            ccd::GetCollectionDetailsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetCollectionDetailsOutput* typedResponse = new ccd::GetCollectionDetailsOutput();
            response.reset(typedResponse);
            status.set_appstatus(MSAGetCollectionDetails(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MSAGetContentURL") == 0) {
            ccd::MSAGetContentURLInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::MSAGetContentURLOutput* typedResponse = new ccd::MSAGetContentURLOutput();
            response.reset(typedResponse);
            status.set_appstatus(MSAGetContentURL(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("MCAQueryMetadataObjects") == 0) {
            ccd::MCAQueryMetadataObjectsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::MCAQueryMetadataObjectsOutput* typedResponse = new ccd::MCAQueryMetadataObjectsOutput();
            response.reset(typedResponse);
            status.set_appstatus(MCAQueryMetadataObjects(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        }
		#if (defined(LINUX) || defined(__CLOUDNODE__)) && !defined(LINUX_EMB) 
		else if (header.methodname().compare("EnableInMemoryLogging") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(EnableInMemoryLogging(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("DisableInMemoryLogging") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(DisableInMemoryLogging(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        }
		#endif 
		else if (header.methodname().compare("FlushInMemoryLogs") == 0) {
            ccd::NoParamRequest request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(FlushInMemoryLogs(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RespondToPairingRequest") == 0) {
            ccd::RespondToPairingRequestInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(RespondToPairingRequest(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RequestPairing") == 0) {
            ccd::RequestPairingInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::RequestPairingOutput* typedResponse = new ccd::RequestPairingOutput();
            response.reset(typedResponse);
            status.set_appstatus(RequestPairing(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RequestPairingPin") == 0) {
            ccd::RequestPairingPinInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::RequestPairingPinOutput* typedResponse = new ccd::RequestPairingPinOutput();
            response.reset(typedResponse);
            status.set_appstatus(RequestPairingPin(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("GetPairingStatus") == 0) {
            ccd::GetPairingStatusInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::GetPairingStatusOutput* typedResponse = new ccd::GetPairingStatusOutput();
            response.reset(typedResponse);
            status.set_appstatus(GetPairingStatus(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("QueryPicStreamObjects") == 0) {
            ccd::CCDIQueryPicStreamObjectsInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::CCDIQueryPicStreamObjectsOutput* typedResponse = new ccd::CCDIQueryPicStreamObjectsOutput();
            response.reset(typedResponse);
            status.set_appstatus(QueryPicStreamObjects(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SharedFilesStoreFile") == 0) {
            ccd::SharedFilesStoreFileInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::SharedFilesStoreFileOutput* typedResponse = new ccd::SharedFilesStoreFileOutput();
            response.reset(typedResponse);
            status.set_appstatus(SharedFilesStoreFile(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SharedFilesShareFile") == 0) {
            ccd::SharedFilesShareFileInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(SharedFilesShareFile(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SharedFilesUnshareFile") == 0) {
            ccd::SharedFilesUnshareFileInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(SharedFilesUnshareFile(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SharedFilesDeleteSharedFile") == 0) {
            ccd::SharedFilesDeleteSharedFileInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(SharedFilesDeleteSharedFile(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("SharedFilesQuery") == 0) {
            ccd::SharedFilesQueryInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::SharedFilesQueryOutput* typedResponse = new ccd::SharedFilesQueryOutput();
            response.reset(typedResponse);
            status.set_appstatus(SharedFilesQuery(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("RegisterRemoteExecutable") == 0) {
            ccd::RegisterRemoteExecutableInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(RegisterRemoteExecutable(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("UnregisterRemoteExecutable") == 0) {
            ccd::UnregisterRemoteExecutableInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::NoParamResponse* typedResponse = new ccd::NoParamResponse();
            response.reset(typedResponse);
            status.set_appstatus(UnregisterRemoteExecutable(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else if (header.methodname().compare("ListRegisteredRemoteExecutables") == 0) {
            ccd::ListRegisteredRemoteExecutablesInput request;
            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }
            ccd::ListRegisteredRemoteExecutablesOutput* typedResponse = new ccd::ListRegisteredRemoteExecutablesOutput();
            response.reset(typedResponse);
            status.set_appstatus(ListRegisteredRemoteExecutables(request, *typedResponse));
            bottomBoilerplate(*response, status);
            break;
        } else {
            status.set_status(RpcStatus::UNKNOWN_METHOD);
            string error("Unrecognized method: ");
            error.append(header.methodname());
            status.set_errordetail(error);
            break;
        }
    } while (false);

    if (status.has_status()) {
        if (!_channel.writeMessage(status, debugCallback)) {
            success = false;
        } else {
            debugLogResponse(debugResponseCallback, header.methodname(), status, response.get());
            if ((status.status() == RpcStatus::OK) && (status.appstatus() >= 0)) {
                if (!_channel.writeMessage(*response, debugCallback)) {
                    success = false;
                }
            }
        }
        if (success) {
            success = _channel.flushOutputStream();
        }
    }

    return success;
}

