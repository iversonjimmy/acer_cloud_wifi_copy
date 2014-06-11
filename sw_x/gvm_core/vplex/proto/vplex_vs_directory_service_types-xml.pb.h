// GENERATED FROM vplex_vs_directory_service_types.proto, DO NOT EDIT
#ifndef __vplex_vs_directory_service_types_xml_pb_h__
#define __vplex_vs_directory_service_types_xml_pb_h__

#include <vplex_xml_writer.h>
#include <ProtoXmlParseState.h>
#include "vplex_vs_directory_service_types.pb.h"

namespace vplex {
namespace vsDirectory {

class ParseStateAPIVersion : public ProtoXmlParseState
{
public:
    ParseStateAPIVersion(const char* outerTag, APIVersion* message);
};

class ParseStateError : public ProtoXmlParseState
{
public:
    ParseStateError(const char* outerTag, Error* message);
};

class ParseStateSessionInfo : public ProtoXmlParseState
{
public:
    ParseStateSessionInfo(const char* outerTag, SessionInfo* message);
};

class ParseStateETicketData : public ProtoXmlParseState
{
public:
    ParseStateETicketData(const char* outerTag, ETicketData* message);
};

class ParseStateLocalization : public ProtoXmlParseState
{
public:
    ParseStateLocalization(const char* outerTag, Localization* message);
};

class ParseStateTitleData : public ProtoXmlParseState
{
public:
    ParseStateTitleData(const char* outerTag, TitleData* message);
};

class ParseStateTitleDetail : public ProtoXmlParseState
{
public:
    ParseStateTitleDetail(const char* outerTag, TitleDetail* message);
};

class ParseStateContentDetail : public ProtoXmlParseState
{
public:
    ParseStateContentDetail(const char* outerTag, ContentDetail* message);
};

class ParseStateSaveData : public ProtoXmlParseState
{
public:
    ParseStateSaveData(const char* outerTag, SaveData* message);
};

class ParseStateTitleTicket : public ProtoXmlParseState
{
public:
    ParseStateTitleTicket(const char* outerTag, TitleTicket* message);
};

class ParseStateSubscription : public ProtoXmlParseState
{
public:
    ParseStateSubscription(const char* outerTag, Subscription* message);
};

class ParseStateSyncDirectory : public ProtoXmlParseState
{
public:
    ParseStateSyncDirectory(const char* outerTag, SyncDirectory* message);
};

class ParseStateDatasetData : public ProtoXmlParseState
{
public:
    ParseStateDatasetData(const char* outerTag, DatasetData* message);
};

class ParseStateDatasetDetail : public ProtoXmlParseState
{
public:
    ParseStateDatasetDetail(const char* outerTag, DatasetDetail* message);
};

class ParseStateStoredDataset : public ProtoXmlParseState
{
public:
    ParseStateStoredDataset(const char* outerTag, StoredDataset* message);
};

class ParseStateDeviceInfo : public ProtoXmlParseState
{
public:
    ParseStateDeviceInfo(const char* outerTag, DeviceInfo* message);
};

class ParseStateStorageAccessPort : public ProtoXmlParseState
{
public:
    ParseStateStorageAccessPort(const char* outerTag, StorageAccessPort* message);
};

class ParseStateStorageAccess : public ProtoXmlParseState
{
public:
    ParseStateStorageAccess(const char* outerTag, StorageAccess* message);
};

class ParseStateDeviceAccessTicket : public ProtoXmlParseState
{
public:
    ParseStateDeviceAccessTicket(const char* outerTag, DeviceAccessTicket* message);
};

class ParseStateUserStorage : public ProtoXmlParseState
{
public:
    ParseStateUserStorage(const char* outerTag, UserStorage* message);
};

class ParseStateUpdatedDataset : public ProtoXmlParseState
{
public:
    ParseStateUpdatedDataset(const char* outerTag, UpdatedDataset* message);
};

class ParseStateDatasetFilter : public ProtoXmlParseState
{
public:
    ParseStateDatasetFilter(const char* outerTag, DatasetFilter* message);
};

class ParseStateMssDetail : public ProtoXmlParseState
{
public:
    ParseStateMssDetail(const char* outerTag, MssDetail* message);
};

class ParseStateStorageUnitDetail : public ProtoXmlParseState
{
public:
    ParseStateStorageUnitDetail(const char* outerTag, StorageUnitDetail* message);
};

class ParseStateBrsDetail : public ProtoXmlParseState
{
public:
    ParseStateBrsDetail(const char* outerTag, BrsDetail* message);
};

class ParseStateBrsStorageUnitDetail : public ProtoXmlParseState
{
public:
    ParseStateBrsStorageUnitDetail(const char* outerTag, BrsStorageUnitDetail* message);
};

class ParseStateBackupStatus : public ProtoXmlParseState
{
public:
    ParseStateBackupStatus(const char* outerTag, BackupStatus* message);
};

class ParseStateGetSaveTicketsInput : public ProtoXmlParseState
{
public:
    ParseStateGetSaveTicketsInput(const char* outerTag, GetSaveTicketsInput* message);
};

class ParseStateGetSaveTicketsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetSaveTicketsOutput(const char* outerTag, GetSaveTicketsOutput* message);
};

class ParseStateGetSaveDataInput : public ProtoXmlParseState
{
public:
    ParseStateGetSaveDataInput(const char* outerTag, GetSaveDataInput* message);
};

class ParseStateGetSaveDataOutput : public ProtoXmlParseState
{
public:
    ParseStateGetSaveDataOutput(const char* outerTag, GetSaveDataOutput* message);
};

class ParseStateGetOwnedTitlesInput : public ProtoXmlParseState
{
public:
    ParseStateGetOwnedTitlesInput(const char* outerTag, GetOwnedTitlesInput* message);
};

class ParseStateGetOwnedTitlesOutput : public ProtoXmlParseState
{
public:
    ParseStateGetOwnedTitlesOutput(const char* outerTag, GetOwnedTitlesOutput* message);
};

class ParseStateGetTitlesInput : public ProtoXmlParseState
{
public:
    ParseStateGetTitlesInput(const char* outerTag, GetTitlesInput* message);
};

class ParseStateGetTitlesOutput : public ProtoXmlParseState
{
public:
    ParseStateGetTitlesOutput(const char* outerTag, GetTitlesOutput* message);
};

class ParseStateGetTitleDetailsInput : public ProtoXmlParseState
{
public:
    ParseStateGetTitleDetailsInput(const char* outerTag, GetTitleDetailsInput* message);
};

class ParseStateGetTitleDetailsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetTitleDetailsOutput(const char* outerTag, GetTitleDetailsOutput* message);
};

class ParseStateGetAttestationChallengeInput : public ProtoXmlParseState
{
public:
    ParseStateGetAttestationChallengeInput(const char* outerTag, GetAttestationChallengeInput* message);
};

class ParseStateGetAttestationChallengeOutput : public ProtoXmlParseState
{
public:
    ParseStateGetAttestationChallengeOutput(const char* outerTag, GetAttestationChallengeOutput* message);
};

class ParseStateAuthenticateDeviceInput : public ProtoXmlParseState
{
public:
    ParseStateAuthenticateDeviceInput(const char* outerTag, AuthenticateDeviceInput* message);
};

class ParseStateAuthenticateDeviceOutput : public ProtoXmlParseState
{
public:
    ParseStateAuthenticateDeviceOutput(const char* outerTag, AuthenticateDeviceOutput* message);
};

class ParseStateGetOnlineTitleTicketInput : public ProtoXmlParseState
{
public:
    ParseStateGetOnlineTitleTicketInput(const char* outerTag, GetOnlineTitleTicketInput* message);
};

class ParseStateGetOnlineTitleTicketOutput : public ProtoXmlParseState
{
public:
    ParseStateGetOnlineTitleTicketOutput(const char* outerTag, GetOnlineTitleTicketOutput* message);
};

class ParseStateGetOfflineTitleTicketsInput : public ProtoXmlParseState
{
public:
    ParseStateGetOfflineTitleTicketsInput(const char* outerTag, GetOfflineTitleTicketsInput* message);
};

class ParseStateGetOfflineTitleTicketsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetOfflineTitleTicketsOutput(const char* outerTag, GetOfflineTitleTicketsOutput* message);
};

class ParseStateListOwnedDataSetsInput : public ProtoXmlParseState
{
public:
    ParseStateListOwnedDataSetsInput(const char* outerTag, ListOwnedDataSetsInput* message);
};

class ParseStateListOwnedDataSetsOutput : public ProtoXmlParseState
{
public:
    ParseStateListOwnedDataSetsOutput(const char* outerTag, ListOwnedDataSetsOutput* message);
};

class ParseStateGetDatasetDetailsInput : public ProtoXmlParseState
{
public:
    ParseStateGetDatasetDetailsInput(const char* outerTag, GetDatasetDetailsInput* message);
};

class ParseStateGetDatasetDetailsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetDatasetDetailsOutput(const char* outerTag, GetDatasetDetailsOutput* message);
};

class ParseStateAddDataSetInput : public ProtoXmlParseState
{
public:
    ParseStateAddDataSetInput(const char* outerTag, AddDataSetInput* message);
};

class ParseStateAddDataSetOutput : public ProtoXmlParseState
{
public:
    ParseStateAddDataSetOutput(const char* outerTag, AddDataSetOutput* message);
};

class ParseStateAddCameraDatasetInput : public ProtoXmlParseState
{
public:
    ParseStateAddCameraDatasetInput(const char* outerTag, AddCameraDatasetInput* message);
};

class ParseStateAddCameraDatasetOutput : public ProtoXmlParseState
{
public:
    ParseStateAddCameraDatasetOutput(const char* outerTag, AddCameraDatasetOutput* message);
};

class ParseStateDeleteDataSetInput : public ProtoXmlParseState
{
public:
    ParseStateDeleteDataSetInput(const char* outerTag, DeleteDataSetInput* message);
};

class ParseStateDeleteDataSetOutput : public ProtoXmlParseState
{
public:
    ParseStateDeleteDataSetOutput(const char* outerTag, DeleteDataSetOutput* message);
};

class ParseStateRenameDataSetInput : public ProtoXmlParseState
{
public:
    ParseStateRenameDataSetInput(const char* outerTag, RenameDataSetInput* message);
};

class ParseStateRenameDataSetOutput : public ProtoXmlParseState
{
public:
    ParseStateRenameDataSetOutput(const char* outerTag, RenameDataSetOutput* message);
};

class ParseStateSetDataSetCacheInput : public ProtoXmlParseState
{
public:
    ParseStateSetDataSetCacheInput(const char* outerTag, SetDataSetCacheInput* message);
};

class ParseStateSetDataSetCacheOutput : public ProtoXmlParseState
{
public:
    ParseStateSetDataSetCacheOutput(const char* outerTag, SetDataSetCacheOutput* message);
};

class ParseStateRemoveDeviceFromSubscriptionsInput : public ProtoXmlParseState
{
public:
    ParseStateRemoveDeviceFromSubscriptionsInput(const char* outerTag, RemoveDeviceFromSubscriptionsInput* message);
};

class ParseStateRemoveDeviceFromSubscriptionsOutput : public ProtoXmlParseState
{
public:
    ParseStateRemoveDeviceFromSubscriptionsOutput(const char* outerTag, RemoveDeviceFromSubscriptionsOutput* message);
};

class ParseStateListSubscriptionsInput : public ProtoXmlParseState
{
public:
    ParseStateListSubscriptionsInput(const char* outerTag, ListSubscriptionsInput* message);
};

class ParseStateListSubscriptionsOutput : public ProtoXmlParseState
{
public:
    ParseStateListSubscriptionsOutput(const char* outerTag, ListSubscriptionsOutput* message);
};

class ParseStateAddSubscriptionsInput : public ProtoXmlParseState
{
public:
    ParseStateAddSubscriptionsInput(const char* outerTag, AddSubscriptionsInput* message);
};

class ParseStateAddSubscriptionsOutput : public ProtoXmlParseState
{
public:
    ParseStateAddSubscriptionsOutput(const char* outerTag, AddSubscriptionsOutput* message);
};

class ParseStateAddUserDatasetSubscriptionInput : public ProtoXmlParseState
{
public:
    ParseStateAddUserDatasetSubscriptionInput(const char* outerTag, AddUserDatasetSubscriptionInput* message);
};

class ParseStateAddUserDatasetSubscriptionOutput : public ProtoXmlParseState
{
public:
    ParseStateAddUserDatasetSubscriptionOutput(const char* outerTag, AddUserDatasetSubscriptionOutput* message);
};

class ParseStateAddCameraSubscriptionInput : public ProtoXmlParseState
{
public:
    ParseStateAddCameraSubscriptionInput(const char* outerTag, AddCameraSubscriptionInput* message);
};

class ParseStateAddCameraSubscriptionOutput : public ProtoXmlParseState
{
public:
    ParseStateAddCameraSubscriptionOutput(const char* outerTag, AddCameraSubscriptionOutput* message);
};

class ParseStateAddDatasetSubscriptionInput : public ProtoXmlParseState
{
public:
    ParseStateAddDatasetSubscriptionInput(const char* outerTag, AddDatasetSubscriptionInput* message);
};

class ParseStateAddDatasetSubscriptionOutput : public ProtoXmlParseState
{
public:
    ParseStateAddDatasetSubscriptionOutput(const char* outerTag, AddDatasetSubscriptionOutput* message);
};

class ParseStateDeleteSubscriptionsInput : public ProtoXmlParseState
{
public:
    ParseStateDeleteSubscriptionsInput(const char* outerTag, DeleteSubscriptionsInput* message);
};

class ParseStateDeleteSubscriptionsOutput : public ProtoXmlParseState
{
public:
    ParseStateDeleteSubscriptionsOutput(const char* outerTag, DeleteSubscriptionsOutput* message);
};

class ParseStateUpdateSubscriptionFilterInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateSubscriptionFilterInput(const char* outerTag, UpdateSubscriptionFilterInput* message);
};

class ParseStateUpdateSubscriptionFilterOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateSubscriptionFilterOutput(const char* outerTag, UpdateSubscriptionFilterOutput* message);
};

class ParseStateUpdateSubscriptionLimitsInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateSubscriptionLimitsInput(const char* outerTag, UpdateSubscriptionLimitsInput* message);
};

class ParseStateUpdateSubscriptionLimitsOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateSubscriptionLimitsOutput(const char* outerTag, UpdateSubscriptionLimitsOutput* message);
};

class ParseStateGetSubscriptionDetailsForDeviceInput : public ProtoXmlParseState
{
public:
    ParseStateGetSubscriptionDetailsForDeviceInput(const char* outerTag, GetSubscriptionDetailsForDeviceInput* message);
};

class ParseStateGetSubscriptionDetailsForDeviceOutput : public ProtoXmlParseState
{
public:
    ParseStateGetSubscriptionDetailsForDeviceOutput(const char* outerTag, GetSubscriptionDetailsForDeviceOutput* message);
};

class ParseStateGetCloudInfoInput : public ProtoXmlParseState
{
public:
    ParseStateGetCloudInfoInput(const char* outerTag, GetCloudInfoInput* message);
};

class ParseStateGetCloudInfoOutput : public ProtoXmlParseState
{
public:
    ParseStateGetCloudInfoOutput(const char* outerTag, GetCloudInfoOutput* message);
};

class ParseStateGetSubscribedDatasetsInput : public ProtoXmlParseState
{
public:
    ParseStateGetSubscribedDatasetsInput(const char* outerTag, GetSubscribedDatasetsInput* message);
};

class ParseStateGetSubscribedDatasetsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetSubscribedDatasetsOutput(const char* outerTag, GetSubscribedDatasetsOutput* message);
};

class ParseStateGetSubscriptionDetailsInput : public ProtoXmlParseState
{
public:
    ParseStateGetSubscriptionDetailsInput(const char* outerTag, GetSubscriptionDetailsInput* message);
};

class ParseStateGetSubscriptionDetailsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetSubscriptionDetailsOutput(const char* outerTag, GetSubscriptionDetailsOutput* message);
};

class ParseStateLinkDeviceInput : public ProtoXmlParseState
{
public:
    ParseStateLinkDeviceInput(const char* outerTag, LinkDeviceInput* message);
};

class ParseStateLinkDeviceOutput : public ProtoXmlParseState
{
public:
    ParseStateLinkDeviceOutput(const char* outerTag, LinkDeviceOutput* message);
};

class ParseStateUnlinkDeviceInput : public ProtoXmlParseState
{
public:
    ParseStateUnlinkDeviceInput(const char* outerTag, UnlinkDeviceInput* message);
};

class ParseStateUnlinkDeviceOutput : public ProtoXmlParseState
{
public:
    ParseStateUnlinkDeviceOutput(const char* outerTag, UnlinkDeviceOutput* message);
};

class ParseStateSetDeviceNameInput : public ProtoXmlParseState
{
public:
    ParseStateSetDeviceNameInput(const char* outerTag, SetDeviceNameInput* message);
};

class ParseStateSetDeviceNameOutput : public ProtoXmlParseState
{
public:
    ParseStateSetDeviceNameOutput(const char* outerTag, SetDeviceNameOutput* message);
};

class ParseStateUpdateDeviceInfoInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDeviceInfoInput(const char* outerTag, UpdateDeviceInfoInput* message);
};

class ParseStateUpdateDeviceInfoOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDeviceInfoOutput(const char* outerTag, UpdateDeviceInfoOutput* message);
};

class ParseStateGetDeviceLinkStateInput : public ProtoXmlParseState
{
public:
    ParseStateGetDeviceLinkStateInput(const char* outerTag, GetDeviceLinkStateInput* message);
};

class ParseStateGetDeviceLinkStateOutput : public ProtoXmlParseState
{
public:
    ParseStateGetDeviceLinkStateOutput(const char* outerTag, GetDeviceLinkStateOutput* message);
};

class ParseStateGetDeviceNameInput : public ProtoXmlParseState
{
public:
    ParseStateGetDeviceNameInput(const char* outerTag, GetDeviceNameInput* message);
};

class ParseStateGetDeviceNameOutput : public ProtoXmlParseState
{
public:
    ParseStateGetDeviceNameOutput(const char* outerTag, GetDeviceNameOutput* message);
};

class ParseStateGetLinkedDevicesInput : public ProtoXmlParseState
{
public:
    ParseStateGetLinkedDevicesInput(const char* outerTag, GetLinkedDevicesInput* message);
};

class ParseStateGetLinkedDevicesOutput : public ProtoXmlParseState
{
public:
    ParseStateGetLinkedDevicesOutput(const char* outerTag, GetLinkedDevicesOutput* message);
};

class ParseStateGetLoginSessionInput : public ProtoXmlParseState
{
public:
    ParseStateGetLoginSessionInput(const char* outerTag, GetLoginSessionInput* message);
};

class ParseStateGetLoginSessionOutput : public ProtoXmlParseState
{
public:
    ParseStateGetLoginSessionOutput(const char* outerTag, GetLoginSessionOutput* message);
};

class ParseStateCreatePersonalStorageNodeInput : public ProtoXmlParseState
{
public:
    ParseStateCreatePersonalStorageNodeInput(const char* outerTag, CreatePersonalStorageNodeInput* message);
};

class ParseStateCreatePersonalStorageNodeOutput : public ProtoXmlParseState
{
public:
    ParseStateCreatePersonalStorageNodeOutput(const char* outerTag, CreatePersonalStorageNodeOutput* message);
};

class ParseStateGetAsyncNoticeServerInput : public ProtoXmlParseState
{
public:
    ParseStateGetAsyncNoticeServerInput(const char* outerTag, GetAsyncNoticeServerInput* message);
};

class ParseStateGetAsyncNoticeServerOutput : public ProtoXmlParseState
{
public:
    ParseStateGetAsyncNoticeServerOutput(const char* outerTag, GetAsyncNoticeServerOutput* message);
};

class ParseStateUpdateStorageNodeConnectionInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateStorageNodeConnectionInput(const char* outerTag, UpdateStorageNodeConnectionInput* message);
};

class ParseStateUpdateStorageNodeConnectionOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateStorageNodeConnectionOutput(const char* outerTag, UpdateStorageNodeConnectionOutput* message);
};

class ParseStateUpdateStorageNodeFeaturesInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateStorageNodeFeaturesInput(const char* outerTag, UpdateStorageNodeFeaturesInput* message);
};

class ParseStateUpdateStorageNodeFeaturesOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateStorageNodeFeaturesOutput(const char* outerTag, UpdateStorageNodeFeaturesOutput* message);
};

class ParseStateGetPSNDatasetLocationInput : public ProtoXmlParseState
{
public:
    ParseStateGetPSNDatasetLocationInput(const char* outerTag, GetPSNDatasetLocationInput* message);
};

class ParseStateGetPSNDatasetLocationOutput : public ProtoXmlParseState
{
public:
    ParseStateGetPSNDatasetLocationOutput(const char* outerTag, GetPSNDatasetLocationOutput* message);
};

class ParseStateUpdatePSNDatasetStatusInput : public ProtoXmlParseState
{
public:
    ParseStateUpdatePSNDatasetStatusInput(const char* outerTag, UpdatePSNDatasetStatusInput* message);
};

class ParseStateUpdatePSNDatasetStatusOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdatePSNDatasetStatusOutput(const char* outerTag, UpdatePSNDatasetStatusOutput* message);
};

class ParseStateAddUserStorageInput : public ProtoXmlParseState
{
public:
    ParseStateAddUserStorageInput(const char* outerTag, AddUserStorageInput* message);
};

class ParseStateAddUserStorageOutput : public ProtoXmlParseState
{
public:
    ParseStateAddUserStorageOutput(const char* outerTag, AddUserStorageOutput* message);
};

class ParseStateDeleteUserStorageInput : public ProtoXmlParseState
{
public:
    ParseStateDeleteUserStorageInput(const char* outerTag, DeleteUserStorageInput* message);
};

class ParseStateDeleteUserStorageOutput : public ProtoXmlParseState
{
public:
    ParseStateDeleteUserStorageOutput(const char* outerTag, DeleteUserStorageOutput* message);
};

class ParseStateChangeUserStorageNameInput : public ProtoXmlParseState
{
public:
    ParseStateChangeUserStorageNameInput(const char* outerTag, ChangeUserStorageNameInput* message);
};

class ParseStateChangeUserStorageNameOutput : public ProtoXmlParseState
{
public:
    ParseStateChangeUserStorageNameOutput(const char* outerTag, ChangeUserStorageNameOutput* message);
};

class ParseStateChangeUserStorageQuotaInput : public ProtoXmlParseState
{
public:
    ParseStateChangeUserStorageQuotaInput(const char* outerTag, ChangeUserStorageQuotaInput* message);
};

class ParseStateChangeUserStorageQuotaOutput : public ProtoXmlParseState
{
public:
    ParseStateChangeUserStorageQuotaOutput(const char* outerTag, ChangeUserStorageQuotaOutput* message);
};

class ParseStateListUserStorageInput : public ProtoXmlParseState
{
public:
    ParseStateListUserStorageInput(const char* outerTag, ListUserStorageInput* message);
};

class ParseStateListUserStorageOutput : public ProtoXmlParseState
{
public:
    ParseStateListUserStorageOutput(const char* outerTag, ListUserStorageOutput* message);
};

class ParseStateGetUserStorageAddressInput : public ProtoXmlParseState
{
public:
    ParseStateGetUserStorageAddressInput(const char* outerTag, GetUserStorageAddressInput* message);
};

class ParseStateUserStorageAddress : public ProtoXmlParseState
{
public:
    ParseStateUserStorageAddress(const char* outerTag, UserStorageAddress* message);
};

class ParseStateGetUserStorageAddressOutput : public ProtoXmlParseState
{
public:
    ParseStateGetUserStorageAddressOutput(const char* outerTag, GetUserStorageAddressOutput* message);
};

class ParseStateAssignUserDatacenterStorageInput : public ProtoXmlParseState
{
public:
    ParseStateAssignUserDatacenterStorageInput(const char* outerTag, AssignUserDatacenterStorageInput* message);
};

class ParseStateAssignUserDatacenterStorageOutput : public ProtoXmlParseState
{
public:
    ParseStateAssignUserDatacenterStorageOutput(const char* outerTag, AssignUserDatacenterStorageOutput* message);
};

class ParseStateGetStorageUnitForDatasetInput : public ProtoXmlParseState
{
public:
    ParseStateGetStorageUnitForDatasetInput(const char* outerTag, GetStorageUnitForDatasetInput* message);
};

class ParseStateGetStorageUnitForDatasetOutput : public ProtoXmlParseState
{
public:
    ParseStateGetStorageUnitForDatasetOutput(const char* outerTag, GetStorageUnitForDatasetOutput* message);
};

class ParseStateGetStoredDatasetsInput : public ProtoXmlParseState
{
public:
    ParseStateGetStoredDatasetsInput(const char* outerTag, GetStoredDatasetsInput* message);
};

class ParseStateGetStoredDatasetsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetStoredDatasetsOutput(const char* outerTag, GetStoredDatasetsOutput* message);
};

class ParseStateGetProxyConnectionForClusterInput : public ProtoXmlParseState
{
public:
    ParseStateGetProxyConnectionForClusterInput(const char* outerTag, GetProxyConnectionForClusterInput* message);
};

class ParseStateGetProxyConnectionForClusterOutput : public ProtoXmlParseState
{
public:
    ParseStateGetProxyConnectionForClusterOutput(const char* outerTag, GetProxyConnectionForClusterOutput* message);
};

class ParseStateSendMessageToPSNInput : public ProtoXmlParseState
{
public:
    ParseStateSendMessageToPSNInput(const char* outerTag, SendMessageToPSNInput* message);
};

class ParseStateSendMessageToPSNOutput : public ProtoXmlParseState
{
public:
    ParseStateSendMessageToPSNOutput(const char* outerTag, SendMessageToPSNOutput* message);
};

class ParseStateChangeStorageUnitForDatasetInput : public ProtoXmlParseState
{
public:
    ParseStateChangeStorageUnitForDatasetInput(const char* outerTag, ChangeStorageUnitForDatasetInput* message);
};

class ParseStateChangeStorageUnitForDatasetOutput : public ProtoXmlParseState
{
public:
    ParseStateChangeStorageUnitForDatasetOutput(const char* outerTag, ChangeStorageUnitForDatasetOutput* message);
};

class ParseStateCreateStorageClusterInput : public ProtoXmlParseState
{
public:
    ParseStateCreateStorageClusterInput(const char* outerTag, CreateStorageClusterInput* message);
};

class ParseStateCreateStorageClusterOutput : public ProtoXmlParseState
{
public:
    ParseStateCreateStorageClusterOutput(const char* outerTag, CreateStorageClusterOutput* message);
};

class ParseStateGetMssInstancesForClusterInput : public ProtoXmlParseState
{
public:
    ParseStateGetMssInstancesForClusterInput(const char* outerTag, GetMssInstancesForClusterInput* message);
};

class ParseStateGetMssInstancesForClusterOutput : public ProtoXmlParseState
{
public:
    ParseStateGetMssInstancesForClusterOutput(const char* outerTag, GetMssInstancesForClusterOutput* message);
};

class ParseStateGetStorageUnitsForClusterInput : public ProtoXmlParseState
{
public:
    ParseStateGetStorageUnitsForClusterInput(const char* outerTag, GetStorageUnitsForClusterInput* message);
};

class ParseStateGetStorageUnitsForClusterOutput : public ProtoXmlParseState
{
public:
    ParseStateGetStorageUnitsForClusterOutput(const char* outerTag, GetStorageUnitsForClusterOutput* message);
};

class ParseStateGetBrsInstancesForClusterInput : public ProtoXmlParseState
{
public:
    ParseStateGetBrsInstancesForClusterInput(const char* outerTag, GetBrsInstancesForClusterInput* message);
};

class ParseStateGetBrsInstancesForClusterOutput : public ProtoXmlParseState
{
public:
    ParseStateGetBrsInstancesForClusterOutput(const char* outerTag, GetBrsInstancesForClusterOutput* message);
};

class ParseStateGetBrsStorageUnitsForClusterInput : public ProtoXmlParseState
{
public:
    ParseStateGetBrsStorageUnitsForClusterInput(const char* outerTag, GetBrsStorageUnitsForClusterInput* message);
};

class ParseStateGetBrsStorageUnitsForClusterOutput : public ProtoXmlParseState
{
public:
    ParseStateGetBrsStorageUnitsForClusterOutput(const char* outerTag, GetBrsStorageUnitsForClusterOutput* message);
};

class ParseStateChangeStorageAssignmentsForDatasetInput : public ProtoXmlParseState
{
public:
    ParseStateChangeStorageAssignmentsForDatasetInput(const char* outerTag, ChangeStorageAssignmentsForDatasetInput* message);
};

class ParseStateChangeStorageAssignmentsForDatasetOutput : public ProtoXmlParseState
{
public:
    ParseStateChangeStorageAssignmentsForDatasetOutput(const char* outerTag, ChangeStorageAssignmentsForDatasetOutput* message);
};

class ParseStateUpdateDatasetStatusInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDatasetStatusInput(const char* outerTag, UpdateDatasetStatusInput* message);
};

class ParseStateUpdateDatasetStatusOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDatasetStatusOutput(const char* outerTag, UpdateDatasetStatusOutput* message);
};

class ParseStateUpdateDatasetBackupStatusInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDatasetBackupStatusInput(const char* outerTag, UpdateDatasetBackupStatusInput* message);
};

class ParseStateUpdateDatasetBackupStatusOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDatasetBackupStatusOutput(const char* outerTag, UpdateDatasetBackupStatusOutput* message);
};

class ParseStateUpdateDatasetArchiveStatusInput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDatasetArchiveStatusInput(const char* outerTag, UpdateDatasetArchiveStatusInput* message);
};

class ParseStateUpdateDatasetArchiveStatusOutput : public ProtoXmlParseState
{
public:
    ParseStateUpdateDatasetArchiveStatusOutput(const char* outerTag, UpdateDatasetArchiveStatusOutput* message);
};

class ParseStateGetDatasetStatusInput : public ProtoXmlParseState
{
public:
    ParseStateGetDatasetStatusInput(const char* outerTag, GetDatasetStatusInput* message);
};

class ParseStateGetDatasetStatusOutput : public ProtoXmlParseState
{
public:
    ParseStateGetDatasetStatusOutput(const char* outerTag, GetDatasetStatusOutput* message);
};

class ParseStateStoreDeviceEventInput : public ProtoXmlParseState
{
public:
    ParseStateStoreDeviceEventInput(const char* outerTag, StoreDeviceEventInput* message);
};

class ParseStateStoreDeviceEventOutput : public ProtoXmlParseState
{
public:
    ParseStateStoreDeviceEventOutput(const char* outerTag, StoreDeviceEventOutput* message);
};

class ParseStateEventInfo : public ProtoXmlParseState
{
public:
    ParseStateEventInfo(const char* outerTag, EventInfo* message);
};

class ParseStateGetLinkedDatasetStatusInput : public ProtoXmlParseState
{
public:
    ParseStateGetLinkedDatasetStatusInput(const char* outerTag, GetLinkedDatasetStatusInput* message);
};

class ParseStateGetLinkedDatasetStatusOutput : public ProtoXmlParseState
{
public:
    ParseStateGetLinkedDatasetStatusOutput(const char* outerTag, GetLinkedDatasetStatusOutput* message);
};

class ParseStateGetUserQuotaStatusInput : public ProtoXmlParseState
{
public:
    ParseStateGetUserQuotaStatusInput(const char* outerTag, GetUserQuotaStatusInput* message);
};

class ParseStateGetUserQuotaStatusOutput : public ProtoXmlParseState
{
public:
    ParseStateGetUserQuotaStatusOutput(const char* outerTag, GetUserQuotaStatusOutput* message);
};

class ParseStateGetDatasetsToBackupInput : public ProtoXmlParseState
{
public:
    ParseStateGetDatasetsToBackupInput(const char* outerTag, GetDatasetsToBackupInput* message);
};

class ParseStateGetDatasetsToBackupOutput : public ProtoXmlParseState
{
public:
    ParseStateGetDatasetsToBackupOutput(const char* outerTag, GetDatasetsToBackupOutput* message);
};

class ParseStateGetBRSHostNameInput : public ProtoXmlParseState
{
public:
    ParseStateGetBRSHostNameInput(const char* outerTag, GetBRSHostNameInput* message);
};

class ParseStateGetBRSHostNameOutput : public ProtoXmlParseState
{
public:
    ParseStateGetBRSHostNameOutput(const char* outerTag, GetBRSHostNameOutput* message);
};

class ParseStateGetBackupStorageUnitsForBrsInput : public ProtoXmlParseState
{
public:
    ParseStateGetBackupStorageUnitsForBrsInput(const char* outerTag, GetBackupStorageUnitsForBrsInput* message);
};

class ParseStateGetBackupStorageUnitsForBrsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetBackupStorageUnitsForBrsOutput(const char* outerTag, GetBackupStorageUnitsForBrsOutput* message);
};

class ParseStateGetUpdatedDatasetsInput : public ProtoXmlParseState
{
public:
    ParseStateGetUpdatedDatasetsInput(const char* outerTag, GetUpdatedDatasetsInput* message);
};

class ParseStateGetUpdatedDatasetsOutput : public ProtoXmlParseState
{
public:
    ParseStateGetUpdatedDatasetsOutput(const char* outerTag, GetUpdatedDatasetsOutput* message);
};

class ParseStateAddDatasetArchiveStorageDeviceInput : public ProtoXmlParseState
{
public:
    ParseStateAddDatasetArchiveStorageDeviceInput(const char* outerTag, AddDatasetArchiveStorageDeviceInput* message);
};

class ParseStateAddDatasetArchiveStorageDeviceOutput : public ProtoXmlParseState
{
public:
    ParseStateAddDatasetArchiveStorageDeviceOutput(const char* outerTag, AddDatasetArchiveStorageDeviceOutput* message);
};

class ParseStateRemoveDatasetArchiveStorageDeviceInput : public ProtoXmlParseState
{
public:
    ParseStateRemoveDatasetArchiveStorageDeviceInput(const char* outerTag, RemoveDatasetArchiveStorageDeviceInput* message);
};

class ParseStateRemoveDatasetArchiveStorageDeviceOutput : public ProtoXmlParseState
{
public:
    ParseStateRemoveDatasetArchiveStorageDeviceOutput(const char* outerTag, RemoveDatasetArchiveStorageDeviceOutput* message);
};

void RegisterHandlers_vplex_vs_directory_service_types(ProtoXmlParseStateImpl*);

vplex::vsDirectory::DatasetType parseDatasetType(const std::string& data, ProtoXmlParseStateImpl* state);
vplex::vsDirectory::RouteType parseRouteType(const std::string& data, ProtoXmlParseStateImpl* state);
vplex::vsDirectory::ProtocolType parseProtocolType(const std::string& data, ProtoXmlParseStateImpl* state);
vplex::vsDirectory::PortType parsePortType(const std::string& data, ProtoXmlParseStateImpl* state);
vplex::vsDirectory::SubscriptionRole parseSubscriptionRole(const std::string& data, ProtoXmlParseStateImpl* state);
const char* writeDatasetType(vplex::vsDirectory::DatasetType val);
const char* writeRouteType(vplex::vsDirectory::RouteType val);
const char* writeProtocolType(vplex::vsDirectory::ProtocolType val);
const char* writePortType(vplex::vsDirectory::PortType val);
const char* writeSubscriptionRole(vplex::vsDirectory::SubscriptionRole val);

void writeAPIVersion(VPLXmlWriter* writer, const APIVersion& message);
void writeError(VPLXmlWriter* writer, const Error& message);
void writeSessionInfo(VPLXmlWriter* writer, const SessionInfo& message);
void writeETicketData(VPLXmlWriter* writer, const ETicketData& message);
void writeLocalization(VPLXmlWriter* writer, const Localization& message);
void writeTitleData(VPLXmlWriter* writer, const TitleData& message);
void writeTitleDetail(VPLXmlWriter* writer, const TitleDetail& message);
void writeContentDetail(VPLXmlWriter* writer, const ContentDetail& message);
void writeSaveData(VPLXmlWriter* writer, const SaveData& message);
void writeTitleTicket(VPLXmlWriter* writer, const TitleTicket& message);
void writeSubscription(VPLXmlWriter* writer, const Subscription& message);
void writeSyncDirectory(VPLXmlWriter* writer, const SyncDirectory& message);
void writeDatasetData(VPLXmlWriter* writer, const DatasetData& message);
void writeDatasetDetail(VPLXmlWriter* writer, const DatasetDetail& message);
void writeStoredDataset(VPLXmlWriter* writer, const StoredDataset& message);
void writeDeviceInfo(VPLXmlWriter* writer, const DeviceInfo& message);
void writeStorageAccessPort(VPLXmlWriter* writer, const StorageAccessPort& message);
void writeStorageAccess(VPLXmlWriter* writer, const StorageAccess& message);
void writeDeviceAccessTicket(VPLXmlWriter* writer, const DeviceAccessTicket& message);
void writeUserStorage(VPLXmlWriter* writer, const UserStorage& message);
void writeUpdatedDataset(VPLXmlWriter* writer, const UpdatedDataset& message);
void writeDatasetFilter(VPLXmlWriter* writer, const DatasetFilter& message);
void writeMssDetail(VPLXmlWriter* writer, const MssDetail& message);
void writeStorageUnitDetail(VPLXmlWriter* writer, const StorageUnitDetail& message);
void writeBrsDetail(VPLXmlWriter* writer, const BrsDetail& message);
void writeBrsStorageUnitDetail(VPLXmlWriter* writer, const BrsStorageUnitDetail& message);
void writeBackupStatus(VPLXmlWriter* writer, const BackupStatus& message);
void writeGetSaveTicketsInput(VPLXmlWriter* writer, const GetSaveTicketsInput& message);
void writeGetSaveTicketsOutput(VPLXmlWriter* writer, const GetSaveTicketsOutput& message);
void writeGetSaveDataInput(VPLXmlWriter* writer, const GetSaveDataInput& message);
void writeGetSaveDataOutput(VPLXmlWriter* writer, const GetSaveDataOutput& message);
void writeGetOwnedTitlesInput(VPLXmlWriter* writer, const GetOwnedTitlesInput& message);
void writeGetOwnedTitlesOutput(VPLXmlWriter* writer, const GetOwnedTitlesOutput& message);
void writeGetTitlesInput(VPLXmlWriter* writer, const GetTitlesInput& message);
void writeGetTitlesOutput(VPLXmlWriter* writer, const GetTitlesOutput& message);
void writeGetTitleDetailsInput(VPLXmlWriter* writer, const GetTitleDetailsInput& message);
void writeGetTitleDetailsOutput(VPLXmlWriter* writer, const GetTitleDetailsOutput& message);
void writeGetAttestationChallengeInput(VPLXmlWriter* writer, const GetAttestationChallengeInput& message);
void writeGetAttestationChallengeOutput(VPLXmlWriter* writer, const GetAttestationChallengeOutput& message);
void writeAuthenticateDeviceInput(VPLXmlWriter* writer, const AuthenticateDeviceInput& message);
void writeAuthenticateDeviceOutput(VPLXmlWriter* writer, const AuthenticateDeviceOutput& message);
void writeGetOnlineTitleTicketInput(VPLXmlWriter* writer, const GetOnlineTitleTicketInput& message);
void writeGetOnlineTitleTicketOutput(VPLXmlWriter* writer, const GetOnlineTitleTicketOutput& message);
void writeGetOfflineTitleTicketsInput(VPLXmlWriter* writer, const GetOfflineTitleTicketsInput& message);
void writeGetOfflineTitleTicketsOutput(VPLXmlWriter* writer, const GetOfflineTitleTicketsOutput& message);
void writeListOwnedDataSetsInput(VPLXmlWriter* writer, const ListOwnedDataSetsInput& message);
void writeListOwnedDataSetsOutput(VPLXmlWriter* writer, const ListOwnedDataSetsOutput& message);
void writeGetDatasetDetailsInput(VPLXmlWriter* writer, const GetDatasetDetailsInput& message);
void writeGetDatasetDetailsOutput(VPLXmlWriter* writer, const GetDatasetDetailsOutput& message);
void writeAddDataSetInput(VPLXmlWriter* writer, const AddDataSetInput& message);
void writeAddDataSetOutput(VPLXmlWriter* writer, const AddDataSetOutput& message);
void writeAddCameraDatasetInput(VPLXmlWriter* writer, const AddCameraDatasetInput& message);
void writeAddCameraDatasetOutput(VPLXmlWriter* writer, const AddCameraDatasetOutput& message);
void writeDeleteDataSetInput(VPLXmlWriter* writer, const DeleteDataSetInput& message);
void writeDeleteDataSetOutput(VPLXmlWriter* writer, const DeleteDataSetOutput& message);
void writeRenameDataSetInput(VPLXmlWriter* writer, const RenameDataSetInput& message);
void writeRenameDataSetOutput(VPLXmlWriter* writer, const RenameDataSetOutput& message);
void writeSetDataSetCacheInput(VPLXmlWriter* writer, const SetDataSetCacheInput& message);
void writeSetDataSetCacheOutput(VPLXmlWriter* writer, const SetDataSetCacheOutput& message);
void writeRemoveDeviceFromSubscriptionsInput(VPLXmlWriter* writer, const RemoveDeviceFromSubscriptionsInput& message);
void writeRemoveDeviceFromSubscriptionsOutput(VPLXmlWriter* writer, const RemoveDeviceFromSubscriptionsOutput& message);
void writeListSubscriptionsInput(VPLXmlWriter* writer, const ListSubscriptionsInput& message);
void writeListSubscriptionsOutput(VPLXmlWriter* writer, const ListSubscriptionsOutput& message);
void writeAddSubscriptionsInput(VPLXmlWriter* writer, const AddSubscriptionsInput& message);
void writeAddSubscriptionsOutput(VPLXmlWriter* writer, const AddSubscriptionsOutput& message);
void writeAddUserDatasetSubscriptionInput(VPLXmlWriter* writer, const AddUserDatasetSubscriptionInput& message);
void writeAddUserDatasetSubscriptionOutput(VPLXmlWriter* writer, const AddUserDatasetSubscriptionOutput& message);
void writeAddCameraSubscriptionInput(VPLXmlWriter* writer, const AddCameraSubscriptionInput& message);
void writeAddCameraSubscriptionOutput(VPLXmlWriter* writer, const AddCameraSubscriptionOutput& message);
void writeAddDatasetSubscriptionInput(VPLXmlWriter* writer, const AddDatasetSubscriptionInput& message);
void writeAddDatasetSubscriptionOutput(VPLXmlWriter* writer, const AddDatasetSubscriptionOutput& message);
void writeDeleteSubscriptionsInput(VPLXmlWriter* writer, const DeleteSubscriptionsInput& message);
void writeDeleteSubscriptionsOutput(VPLXmlWriter* writer, const DeleteSubscriptionsOutput& message);
void writeUpdateSubscriptionFilterInput(VPLXmlWriter* writer, const UpdateSubscriptionFilterInput& message);
void writeUpdateSubscriptionFilterOutput(VPLXmlWriter* writer, const UpdateSubscriptionFilterOutput& message);
void writeUpdateSubscriptionLimitsInput(VPLXmlWriter* writer, const UpdateSubscriptionLimitsInput& message);
void writeUpdateSubscriptionLimitsOutput(VPLXmlWriter* writer, const UpdateSubscriptionLimitsOutput& message);
void writeGetSubscriptionDetailsForDeviceInput(VPLXmlWriter* writer, const GetSubscriptionDetailsForDeviceInput& message);
void writeGetSubscriptionDetailsForDeviceOutput(VPLXmlWriter* writer, const GetSubscriptionDetailsForDeviceOutput& message);
void writeGetCloudInfoInput(VPLXmlWriter* writer, const GetCloudInfoInput& message);
void writeGetCloudInfoOutput(VPLXmlWriter* writer, const GetCloudInfoOutput& message);
void writeGetSubscribedDatasetsInput(VPLXmlWriter* writer, const GetSubscribedDatasetsInput& message);
void writeGetSubscribedDatasetsOutput(VPLXmlWriter* writer, const GetSubscribedDatasetsOutput& message);
void writeGetSubscriptionDetailsInput(VPLXmlWriter* writer, const GetSubscriptionDetailsInput& message);
void writeGetSubscriptionDetailsOutput(VPLXmlWriter* writer, const GetSubscriptionDetailsOutput& message);
void writeLinkDeviceInput(VPLXmlWriter* writer, const LinkDeviceInput& message);
void writeLinkDeviceOutput(VPLXmlWriter* writer, const LinkDeviceOutput& message);
void writeUnlinkDeviceInput(VPLXmlWriter* writer, const UnlinkDeviceInput& message);
void writeUnlinkDeviceOutput(VPLXmlWriter* writer, const UnlinkDeviceOutput& message);
void writeSetDeviceNameInput(VPLXmlWriter* writer, const SetDeviceNameInput& message);
void writeSetDeviceNameOutput(VPLXmlWriter* writer, const SetDeviceNameOutput& message);
void writeUpdateDeviceInfoInput(VPLXmlWriter* writer, const UpdateDeviceInfoInput& message);
void writeUpdateDeviceInfoOutput(VPLXmlWriter* writer, const UpdateDeviceInfoOutput& message);
void writeGetDeviceLinkStateInput(VPLXmlWriter* writer, const GetDeviceLinkStateInput& message);
void writeGetDeviceLinkStateOutput(VPLXmlWriter* writer, const GetDeviceLinkStateOutput& message);
void writeGetDeviceNameInput(VPLXmlWriter* writer, const GetDeviceNameInput& message);
void writeGetDeviceNameOutput(VPLXmlWriter* writer, const GetDeviceNameOutput& message);
void writeGetLinkedDevicesInput(VPLXmlWriter* writer, const GetLinkedDevicesInput& message);
void writeGetLinkedDevicesOutput(VPLXmlWriter* writer, const GetLinkedDevicesOutput& message);
void writeGetLoginSessionInput(VPLXmlWriter* writer, const GetLoginSessionInput& message);
void writeGetLoginSessionOutput(VPLXmlWriter* writer, const GetLoginSessionOutput& message);
void writeCreatePersonalStorageNodeInput(VPLXmlWriter* writer, const CreatePersonalStorageNodeInput& message);
void writeCreatePersonalStorageNodeOutput(VPLXmlWriter* writer, const CreatePersonalStorageNodeOutput& message);
void writeGetAsyncNoticeServerInput(VPLXmlWriter* writer, const GetAsyncNoticeServerInput& message);
void writeGetAsyncNoticeServerOutput(VPLXmlWriter* writer, const GetAsyncNoticeServerOutput& message);
void writeUpdateStorageNodeConnectionInput(VPLXmlWriter* writer, const UpdateStorageNodeConnectionInput& message);
void writeUpdateStorageNodeConnectionOutput(VPLXmlWriter* writer, const UpdateStorageNodeConnectionOutput& message);
void writeUpdateStorageNodeFeaturesInput(VPLXmlWriter* writer, const UpdateStorageNodeFeaturesInput& message);
void writeUpdateStorageNodeFeaturesOutput(VPLXmlWriter* writer, const UpdateStorageNodeFeaturesOutput& message);
void writeGetPSNDatasetLocationInput(VPLXmlWriter* writer, const GetPSNDatasetLocationInput& message);
void writeGetPSNDatasetLocationOutput(VPLXmlWriter* writer, const GetPSNDatasetLocationOutput& message);
void writeUpdatePSNDatasetStatusInput(VPLXmlWriter* writer, const UpdatePSNDatasetStatusInput& message);
void writeUpdatePSNDatasetStatusOutput(VPLXmlWriter* writer, const UpdatePSNDatasetStatusOutput& message);
void writeAddUserStorageInput(VPLXmlWriter* writer, const AddUserStorageInput& message);
void writeAddUserStorageOutput(VPLXmlWriter* writer, const AddUserStorageOutput& message);
void writeDeleteUserStorageInput(VPLXmlWriter* writer, const DeleteUserStorageInput& message);
void writeDeleteUserStorageOutput(VPLXmlWriter* writer, const DeleteUserStorageOutput& message);
void writeChangeUserStorageNameInput(VPLXmlWriter* writer, const ChangeUserStorageNameInput& message);
void writeChangeUserStorageNameOutput(VPLXmlWriter* writer, const ChangeUserStorageNameOutput& message);
void writeChangeUserStorageQuotaInput(VPLXmlWriter* writer, const ChangeUserStorageQuotaInput& message);
void writeChangeUserStorageQuotaOutput(VPLXmlWriter* writer, const ChangeUserStorageQuotaOutput& message);
void writeListUserStorageInput(VPLXmlWriter* writer, const ListUserStorageInput& message);
void writeListUserStorageOutput(VPLXmlWriter* writer, const ListUserStorageOutput& message);
void writeGetUserStorageAddressInput(VPLXmlWriter* writer, const GetUserStorageAddressInput& message);
void writeUserStorageAddress(VPLXmlWriter* writer, const UserStorageAddress& message);
void writeGetUserStorageAddressOutput(VPLXmlWriter* writer, const GetUserStorageAddressOutput& message);
void writeAssignUserDatacenterStorageInput(VPLXmlWriter* writer, const AssignUserDatacenterStorageInput& message);
void writeAssignUserDatacenterStorageOutput(VPLXmlWriter* writer, const AssignUserDatacenterStorageOutput& message);
void writeGetStorageUnitForDatasetInput(VPLXmlWriter* writer, const GetStorageUnitForDatasetInput& message);
void writeGetStorageUnitForDatasetOutput(VPLXmlWriter* writer, const GetStorageUnitForDatasetOutput& message);
void writeGetStoredDatasetsInput(VPLXmlWriter* writer, const GetStoredDatasetsInput& message);
void writeGetStoredDatasetsOutput(VPLXmlWriter* writer, const GetStoredDatasetsOutput& message);
void writeGetProxyConnectionForClusterInput(VPLXmlWriter* writer, const GetProxyConnectionForClusterInput& message);
void writeGetProxyConnectionForClusterOutput(VPLXmlWriter* writer, const GetProxyConnectionForClusterOutput& message);
void writeSendMessageToPSNInput(VPLXmlWriter* writer, const SendMessageToPSNInput& message);
void writeSendMessageToPSNOutput(VPLXmlWriter* writer, const SendMessageToPSNOutput& message);
void writeChangeStorageUnitForDatasetInput(VPLXmlWriter* writer, const ChangeStorageUnitForDatasetInput& message);
void writeChangeStorageUnitForDatasetOutput(VPLXmlWriter* writer, const ChangeStorageUnitForDatasetOutput& message);
void writeCreateStorageClusterInput(VPLXmlWriter* writer, const CreateStorageClusterInput& message);
void writeCreateStorageClusterOutput(VPLXmlWriter* writer, const CreateStorageClusterOutput& message);
void writeGetMssInstancesForClusterInput(VPLXmlWriter* writer, const GetMssInstancesForClusterInput& message);
void writeGetMssInstancesForClusterOutput(VPLXmlWriter* writer, const GetMssInstancesForClusterOutput& message);
void writeGetStorageUnitsForClusterInput(VPLXmlWriter* writer, const GetStorageUnitsForClusterInput& message);
void writeGetStorageUnitsForClusterOutput(VPLXmlWriter* writer, const GetStorageUnitsForClusterOutput& message);
void writeGetBrsInstancesForClusterInput(VPLXmlWriter* writer, const GetBrsInstancesForClusterInput& message);
void writeGetBrsInstancesForClusterOutput(VPLXmlWriter* writer, const GetBrsInstancesForClusterOutput& message);
void writeGetBrsStorageUnitsForClusterInput(VPLXmlWriter* writer, const GetBrsStorageUnitsForClusterInput& message);
void writeGetBrsStorageUnitsForClusterOutput(VPLXmlWriter* writer, const GetBrsStorageUnitsForClusterOutput& message);
void writeChangeStorageAssignmentsForDatasetInput(VPLXmlWriter* writer, const ChangeStorageAssignmentsForDatasetInput& message);
void writeChangeStorageAssignmentsForDatasetOutput(VPLXmlWriter* writer, const ChangeStorageAssignmentsForDatasetOutput& message);
void writeUpdateDatasetStatusInput(VPLXmlWriter* writer, const UpdateDatasetStatusInput& message);
void writeUpdateDatasetStatusOutput(VPLXmlWriter* writer, const UpdateDatasetStatusOutput& message);
void writeUpdateDatasetBackupStatusInput(VPLXmlWriter* writer, const UpdateDatasetBackupStatusInput& message);
void writeUpdateDatasetBackupStatusOutput(VPLXmlWriter* writer, const UpdateDatasetBackupStatusOutput& message);
void writeUpdateDatasetArchiveStatusInput(VPLXmlWriter* writer, const UpdateDatasetArchiveStatusInput& message);
void writeUpdateDatasetArchiveStatusOutput(VPLXmlWriter* writer, const UpdateDatasetArchiveStatusOutput& message);
void writeGetDatasetStatusInput(VPLXmlWriter* writer, const GetDatasetStatusInput& message);
void writeGetDatasetStatusOutput(VPLXmlWriter* writer, const GetDatasetStatusOutput& message);
void writeStoreDeviceEventInput(VPLXmlWriter* writer, const StoreDeviceEventInput& message);
void writeStoreDeviceEventOutput(VPLXmlWriter* writer, const StoreDeviceEventOutput& message);
void writeEventInfo(VPLXmlWriter* writer, const EventInfo& message);
void writeGetLinkedDatasetStatusInput(VPLXmlWriter* writer, const GetLinkedDatasetStatusInput& message);
void writeGetLinkedDatasetStatusOutput(VPLXmlWriter* writer, const GetLinkedDatasetStatusOutput& message);
void writeGetUserQuotaStatusInput(VPLXmlWriter* writer, const GetUserQuotaStatusInput& message);
void writeGetUserQuotaStatusOutput(VPLXmlWriter* writer, const GetUserQuotaStatusOutput& message);
void writeGetDatasetsToBackupInput(VPLXmlWriter* writer, const GetDatasetsToBackupInput& message);
void writeGetDatasetsToBackupOutput(VPLXmlWriter* writer, const GetDatasetsToBackupOutput& message);
void writeGetBRSHostNameInput(VPLXmlWriter* writer, const GetBRSHostNameInput& message);
void writeGetBRSHostNameOutput(VPLXmlWriter* writer, const GetBRSHostNameOutput& message);
void writeGetBackupStorageUnitsForBrsInput(VPLXmlWriter* writer, const GetBackupStorageUnitsForBrsInput& message);
void writeGetBackupStorageUnitsForBrsOutput(VPLXmlWriter* writer, const GetBackupStorageUnitsForBrsOutput& message);
void writeGetUpdatedDatasetsInput(VPLXmlWriter* writer, const GetUpdatedDatasetsInput& message);
void writeGetUpdatedDatasetsOutput(VPLXmlWriter* writer, const GetUpdatedDatasetsOutput& message);
void writeAddDatasetArchiveStorageDeviceInput(VPLXmlWriter* writer, const AddDatasetArchiveStorageDeviceInput& message);
void writeAddDatasetArchiveStorageDeviceOutput(VPLXmlWriter* writer, const AddDatasetArchiveStorageDeviceOutput& message);
void writeRemoveDatasetArchiveStorageDeviceInput(VPLXmlWriter* writer, const RemoveDatasetArchiveStorageDeviceInput& message);
void writeRemoveDatasetArchiveStorageDeviceOutput(VPLXmlWriter* writer, const RemoveDatasetArchiveStorageDeviceOutput& message);

} // namespace vplex
} // namespace vsDirectory
#endif /* __vplex_vs_directory_service_types_xml_pb_h__ */
