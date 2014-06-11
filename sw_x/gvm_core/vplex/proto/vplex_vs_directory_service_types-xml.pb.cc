// GENERATED FROM vplex_vs_directory_service_types.proto, DO NOT EDIT

#include "vplex_vs_directory_service_types-xml.pb.h"

#include <string.h>

#include "vplex_common_types-xml.pb.h"
#include <ProtoXmlParseStateImpl.h>

static void processOpenAPIVersion(vplex::vsDirectory::APIVersion* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAPIVersionCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAPIVersion(vplex::vsDirectory::APIVersion* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAPIVersionCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenError(vplex::vsDirectory::Error* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenErrorCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseError(vplex::vsDirectory::Error* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseErrorCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSessionInfo(vplex::vsDirectory::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSessionInfo(vplex::vsDirectory::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenETicketData(vplex::vsDirectory::ETicketData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenETicketDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseETicketData(vplex::vsDirectory::ETicketData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseETicketDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLocalization(vplex::vsDirectory::Localization* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLocalizationCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLocalization(vplex::vsDirectory::Localization* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLocalizationCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleData(vplex::vsDirectory::TitleData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleData(vplex::vsDirectory::TitleData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleDetail(vplex::vsDirectory::TitleDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleDetail(vplex::vsDirectory::TitleDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenContentDetail(vplex::vsDirectory::ContentDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenContentDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseContentDetail(vplex::vsDirectory::ContentDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseContentDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSaveData(vplex::vsDirectory::SaveData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSaveDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSaveData(vplex::vsDirectory::SaveData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSaveDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleTicket(vplex::vsDirectory::TitleTicket* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenTitleTicketCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleTicket(vplex::vsDirectory::TitleTicket* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseTitleTicketCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSubscription(vplex::vsDirectory::Subscription* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSubscriptionCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSubscription(vplex::vsDirectory::Subscription* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSubscriptionCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSyncDirectory(vplex::vsDirectory::SyncDirectory* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSyncDirectoryCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSyncDirectory(vplex::vsDirectory::SyncDirectory* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSyncDirectoryCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDatasetData(vplex::vsDirectory::DatasetData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDatasetDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDatasetData(vplex::vsDirectory::DatasetData* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDatasetDataCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDatasetDetail(vplex::vsDirectory::DatasetDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDatasetDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDatasetDetail(vplex::vsDirectory::DatasetDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDatasetDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStoredDataset(vplex::vsDirectory::StoredDataset* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStoredDatasetCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStoredDataset(vplex::vsDirectory::StoredDataset* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStoredDatasetCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeviceInfo(vplex::vsDirectory::DeviceInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeviceInfoCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeviceInfo(vplex::vsDirectory::DeviceInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeviceInfoCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStorageAccessPort(vplex::vsDirectory::StorageAccessPort* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStorageAccessPortCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStorageAccessPort(vplex::vsDirectory::StorageAccessPort* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStorageAccessPortCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStorageAccess(vplex::vsDirectory::StorageAccess* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStorageAccessCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStorageAccess(vplex::vsDirectory::StorageAccess* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStorageAccessCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeviceAccessTicket(vplex::vsDirectory::DeviceAccessTicket* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeviceAccessTicketCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeviceAccessTicket(vplex::vsDirectory::DeviceAccessTicket* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeviceAccessTicketCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUserStorage(vplex::vsDirectory::UserStorage* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUserStorageCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUserStorage(vplex::vsDirectory::UserStorage* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUserStorageCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdatedDataset(vplex::vsDirectory::UpdatedDataset* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdatedDatasetCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdatedDataset(vplex::vsDirectory::UpdatedDataset* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdatedDatasetCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDatasetFilter(vplex::vsDirectory::DatasetFilter* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDatasetFilterCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDatasetFilter(vplex::vsDirectory::DatasetFilter* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDatasetFilterCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenMssDetail(vplex::vsDirectory::MssDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenMssDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseMssDetail(vplex::vsDirectory::MssDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseMssDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStorageUnitDetail(vplex::vsDirectory::StorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStorageUnitDetail(vplex::vsDirectory::StorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenBrsDetail(vplex::vsDirectory::BrsDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenBrsDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseBrsDetail(vplex::vsDirectory::BrsDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseBrsDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenBrsStorageUnitDetail(vplex::vsDirectory::BrsStorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenBrsStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseBrsStorageUnitDetail(vplex::vsDirectory::BrsStorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseBrsStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenBackupStatus(vplex::vsDirectory::BackupStatus* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenBackupStatusCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseBackupStatus(vplex::vsDirectory::BackupStatus* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseBackupStatusCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveTicketsInput(vplex::vsDirectory::GetSaveTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveTicketsInput(vplex::vsDirectory::GetSaveTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveTicketsOutput(vplex::vsDirectory::GetSaveTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveTicketsOutput(vplex::vsDirectory::GetSaveTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveDataInput(vplex::vsDirectory::GetSaveDataInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveDataInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveDataInput(vplex::vsDirectory::GetSaveDataInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveDataInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveDataOutput(vplex::vsDirectory::GetSaveDataOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSaveDataOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveDataOutput(vplex::vsDirectory::GetSaveDataOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSaveDataOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOwnedTitlesInput(vplex::vsDirectory::GetOwnedTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOwnedTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOwnedTitlesInput(vplex::vsDirectory::GetOwnedTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOwnedTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOwnedTitlesOutput(vplex::vsDirectory::GetOwnedTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOwnedTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOwnedTitlesOutput(vplex::vsDirectory::GetOwnedTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOwnedTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitlesInput(vplex::vsDirectory::GetTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitlesInput(vplex::vsDirectory::GetTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitlesOutput(vplex::vsDirectory::GetTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitlesOutput(vplex::vsDirectory::GetTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitleDetailsInput(vplex::vsDirectory::GetTitleDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitleDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitleDetailsInput(vplex::vsDirectory::GetTitleDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitleDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitleDetailsOutput(vplex::vsDirectory::GetTitleDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetTitleDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitleDetailsOutput(vplex::vsDirectory::GetTitleDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetTitleDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAttestationChallengeInput(vplex::vsDirectory::GetAttestationChallengeInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAttestationChallengeInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAttestationChallengeInput(vplex::vsDirectory::GetAttestationChallengeInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAttestationChallengeInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAttestationChallengeOutput(vplex::vsDirectory::GetAttestationChallengeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAttestationChallengeOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAttestationChallengeOutput(vplex::vsDirectory::GetAttestationChallengeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAttestationChallengeOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAuthenticateDeviceInput(vplex::vsDirectory::AuthenticateDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAuthenticateDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAuthenticateDeviceInput(vplex::vsDirectory::AuthenticateDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAuthenticateDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAuthenticateDeviceOutput(vplex::vsDirectory::AuthenticateDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAuthenticateDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAuthenticateDeviceOutput(vplex::vsDirectory::AuthenticateDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAuthenticateDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOnlineTitleTicketInput(vplex::vsDirectory::GetOnlineTitleTicketInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOnlineTitleTicketInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOnlineTitleTicketInput(vplex::vsDirectory::GetOnlineTitleTicketInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOnlineTitleTicketInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOnlineTitleTicketOutput(vplex::vsDirectory::GetOnlineTitleTicketOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOnlineTitleTicketOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOnlineTitleTicketOutput(vplex::vsDirectory::GetOnlineTitleTicketOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOnlineTitleTicketOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOfflineTitleTicketsInput(vplex::vsDirectory::GetOfflineTitleTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOfflineTitleTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOfflineTitleTicketsInput(vplex::vsDirectory::GetOfflineTitleTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOfflineTitleTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOfflineTitleTicketsOutput(vplex::vsDirectory::GetOfflineTitleTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetOfflineTitleTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOfflineTitleTicketsOutput(vplex::vsDirectory::GetOfflineTitleTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetOfflineTitleTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListOwnedDataSetsInput(vplex::vsDirectory::ListOwnedDataSetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListOwnedDataSetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListOwnedDataSetsInput(vplex::vsDirectory::ListOwnedDataSetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListOwnedDataSetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListOwnedDataSetsOutput(vplex::vsDirectory::ListOwnedDataSetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListOwnedDataSetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListOwnedDataSetsOutput(vplex::vsDirectory::ListOwnedDataSetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListOwnedDataSetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetDetailsInput(vplex::vsDirectory::GetDatasetDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetDetailsInput(vplex::vsDirectory::GetDatasetDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetDetailsOutput(vplex::vsDirectory::GetDatasetDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetDetailsOutput(vplex::vsDirectory::GetDatasetDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDataSetInput(vplex::vsDirectory::AddDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDataSetInput(vplex::vsDirectory::AddDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDataSetOutput(vplex::vsDirectory::AddDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDataSetOutput(vplex::vsDirectory::AddDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraDatasetInput(vplex::vsDirectory::AddCameraDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraDatasetInput(vplex::vsDirectory::AddCameraDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraDatasetOutput(vplex::vsDirectory::AddCameraDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraDatasetOutput(vplex::vsDirectory::AddCameraDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteDataSetInput(vplex::vsDirectory::DeleteDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteDataSetInput(vplex::vsDirectory::DeleteDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteDataSetOutput(vplex::vsDirectory::DeleteDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteDataSetOutput(vplex::vsDirectory::DeleteDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenameDataSetInput(vplex::vsDirectory::RenameDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenameDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenameDataSetInput(vplex::vsDirectory::RenameDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenameDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenameDataSetOutput(vplex::vsDirectory::RenameDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRenameDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenameDataSetOutput(vplex::vsDirectory::RenameDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRenameDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDataSetCacheInput(vplex::vsDirectory::SetDataSetCacheInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDataSetCacheInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDataSetCacheInput(vplex::vsDirectory::SetDataSetCacheInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDataSetCacheInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDataSetCacheOutput(vplex::vsDirectory::SetDataSetCacheOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDataSetCacheOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDataSetCacheOutput(vplex::vsDirectory::SetDataSetCacheOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDataSetCacheOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDeviceFromSubscriptionsInput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDeviceFromSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDeviceFromSubscriptionsInput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDeviceFromSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDeviceFromSubscriptionsOutput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDeviceFromSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDeviceFromSubscriptionsOutput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDeviceFromSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListSubscriptionsInput(vplex::vsDirectory::ListSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListSubscriptionsInput(vplex::vsDirectory::ListSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListSubscriptionsOutput(vplex::vsDirectory::ListSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListSubscriptionsOutput(vplex::vsDirectory::ListSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddSubscriptionsInput(vplex::vsDirectory::AddSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddSubscriptionsInput(vplex::vsDirectory::AddSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddSubscriptionsOutput(vplex::vsDirectory::AddSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddSubscriptionsOutput(vplex::vsDirectory::AddSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserDatasetSubscriptionInput(vplex::vsDirectory::AddUserDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserDatasetSubscriptionInput(vplex::vsDirectory::AddUserDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserDatasetSubscriptionOutput(vplex::vsDirectory::AddUserDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserDatasetSubscriptionOutput(vplex::vsDirectory::AddUserDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraSubscriptionInput(vplex::vsDirectory::AddCameraSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraSubscriptionInput(vplex::vsDirectory::AddCameraSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraSubscriptionOutput(vplex::vsDirectory::AddCameraSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddCameraSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraSubscriptionOutput(vplex::vsDirectory::AddCameraSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddCameraSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetSubscriptionInput(vplex::vsDirectory::AddDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetSubscriptionInput(vplex::vsDirectory::AddDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetSubscriptionOutput(vplex::vsDirectory::AddDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetSubscriptionOutput(vplex::vsDirectory::AddDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteSubscriptionsInput(vplex::vsDirectory::DeleteSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteSubscriptionsInput(vplex::vsDirectory::DeleteSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteSubscriptionsOutput(vplex::vsDirectory::DeleteSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteSubscriptionsOutput(vplex::vsDirectory::DeleteSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionFilterInput(vplex::vsDirectory::UpdateSubscriptionFilterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionFilterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionFilterInput(vplex::vsDirectory::UpdateSubscriptionFilterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionFilterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionFilterOutput(vplex::vsDirectory::UpdateSubscriptionFilterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionFilterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionFilterOutput(vplex::vsDirectory::UpdateSubscriptionFilterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionFilterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionLimitsInput(vplex::vsDirectory::UpdateSubscriptionLimitsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionLimitsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionLimitsInput(vplex::vsDirectory::UpdateSubscriptionLimitsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionLimitsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionLimitsOutput(vplex::vsDirectory::UpdateSubscriptionLimitsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateSubscriptionLimitsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionLimitsOutput(vplex::vsDirectory::UpdateSubscriptionLimitsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateSubscriptionLimitsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsForDeviceInput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsForDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsForDeviceInput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsForDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsForDeviceOutput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsForDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsForDeviceOutput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsForDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetCloudInfoInput(vplex::vsDirectory::GetCloudInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetCloudInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetCloudInfoInput(vplex::vsDirectory::GetCloudInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetCloudInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetCloudInfoOutput(vplex::vsDirectory::GetCloudInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetCloudInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetCloudInfoOutput(vplex::vsDirectory::GetCloudInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetCloudInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscribedDatasetsInput(vplex::vsDirectory::GetSubscribedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscribedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscribedDatasetsInput(vplex::vsDirectory::GetSubscribedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscribedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscribedDatasetsOutput(vplex::vsDirectory::GetSubscribedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscribedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscribedDatasetsOutput(vplex::vsDirectory::GetSubscribedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscribedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsInput(vplex::vsDirectory::GetSubscriptionDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsInput(vplex::vsDirectory::GetSubscriptionDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsOutput(vplex::vsDirectory::GetSubscriptionDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetSubscriptionDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsOutput(vplex::vsDirectory::GetSubscriptionDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetSubscriptionDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLinkDeviceInput(vplex::vsDirectory::LinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLinkDeviceInput(vplex::vsDirectory::LinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLinkDeviceOutput(vplex::vsDirectory::LinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenLinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLinkDeviceOutput(vplex::vsDirectory::LinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseLinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUnlinkDeviceInput(vplex::vsDirectory::UnlinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUnlinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUnlinkDeviceInput(vplex::vsDirectory::UnlinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUnlinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUnlinkDeviceOutput(vplex::vsDirectory::UnlinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUnlinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUnlinkDeviceOutput(vplex::vsDirectory::UnlinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUnlinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDeviceNameInput(vplex::vsDirectory::SetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDeviceNameInput(vplex::vsDirectory::SetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDeviceNameOutput(vplex::vsDirectory::SetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDeviceNameOutput(vplex::vsDirectory::SetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDeviceInfoInput(vplex::vsDirectory::UpdateDeviceInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDeviceInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDeviceInfoInput(vplex::vsDirectory::UpdateDeviceInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDeviceInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDeviceInfoOutput(vplex::vsDirectory::UpdateDeviceInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDeviceInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDeviceInfoOutput(vplex::vsDirectory::UpdateDeviceInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDeviceInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceLinkStateInput(vplex::vsDirectory::GetDeviceLinkStateInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceLinkStateInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceLinkStateInput(vplex::vsDirectory::GetDeviceLinkStateInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceLinkStateInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceLinkStateOutput(vplex::vsDirectory::GetDeviceLinkStateOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceLinkStateOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceLinkStateOutput(vplex::vsDirectory::GetDeviceLinkStateOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceLinkStateOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceNameInput(vplex::vsDirectory::GetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceNameInput(vplex::vsDirectory::GetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceNameOutput(vplex::vsDirectory::GetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceNameOutput(vplex::vsDirectory::GetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDevicesInput(vplex::vsDirectory::GetLinkedDevicesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDevicesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDevicesInput(vplex::vsDirectory::GetLinkedDevicesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDevicesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDevicesOutput(vplex::vsDirectory::GetLinkedDevicesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDevicesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDevicesOutput(vplex::vsDirectory::GetLinkedDevicesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDevicesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLoginSessionInput(vplex::vsDirectory::GetLoginSessionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLoginSessionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLoginSessionInput(vplex::vsDirectory::GetLoginSessionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLoginSessionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLoginSessionOutput(vplex::vsDirectory::GetLoginSessionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLoginSessionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLoginSessionOutput(vplex::vsDirectory::GetLoginSessionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLoginSessionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreatePersonalStorageNodeInput(vplex::vsDirectory::CreatePersonalStorageNodeInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreatePersonalStorageNodeInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreatePersonalStorageNodeInput(vplex::vsDirectory::CreatePersonalStorageNodeInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreatePersonalStorageNodeInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreatePersonalStorageNodeOutput(vplex::vsDirectory::CreatePersonalStorageNodeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreatePersonalStorageNodeOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreatePersonalStorageNodeOutput(vplex::vsDirectory::CreatePersonalStorageNodeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreatePersonalStorageNodeOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAsyncNoticeServerInput(vplex::vsDirectory::GetAsyncNoticeServerInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAsyncNoticeServerInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAsyncNoticeServerInput(vplex::vsDirectory::GetAsyncNoticeServerInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAsyncNoticeServerInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAsyncNoticeServerOutput(vplex::vsDirectory::GetAsyncNoticeServerOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetAsyncNoticeServerOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAsyncNoticeServerOutput(vplex::vsDirectory::GetAsyncNoticeServerOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetAsyncNoticeServerOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeConnectionInput(vplex::vsDirectory::UpdateStorageNodeConnectionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeConnectionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeConnectionInput(vplex::vsDirectory::UpdateStorageNodeConnectionInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeConnectionInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeConnectionOutput(vplex::vsDirectory::UpdateStorageNodeConnectionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeConnectionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeConnectionOutput(vplex::vsDirectory::UpdateStorageNodeConnectionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeConnectionOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeFeaturesInput(vplex::vsDirectory::UpdateStorageNodeFeaturesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeFeaturesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeFeaturesInput(vplex::vsDirectory::UpdateStorageNodeFeaturesInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeFeaturesInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeFeaturesOutput(vplex::vsDirectory::UpdateStorageNodeFeaturesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateStorageNodeFeaturesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeFeaturesOutput(vplex::vsDirectory::UpdateStorageNodeFeaturesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateStorageNodeFeaturesOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPSNDatasetLocationInput(vplex::vsDirectory::GetPSNDatasetLocationInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPSNDatasetLocationInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPSNDatasetLocationInput(vplex::vsDirectory::GetPSNDatasetLocationInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPSNDatasetLocationInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPSNDatasetLocationOutput(vplex::vsDirectory::GetPSNDatasetLocationOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetPSNDatasetLocationOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPSNDatasetLocationOutput(vplex::vsDirectory::GetPSNDatasetLocationOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetPSNDatasetLocationOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdatePSNDatasetStatusInput(vplex::vsDirectory::UpdatePSNDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdatePSNDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdatePSNDatasetStatusInput(vplex::vsDirectory::UpdatePSNDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdatePSNDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdatePSNDatasetStatusOutput(vplex::vsDirectory::UpdatePSNDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdatePSNDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdatePSNDatasetStatusOutput(vplex::vsDirectory::UpdatePSNDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdatePSNDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserStorageInput(vplex::vsDirectory::AddUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserStorageInput(vplex::vsDirectory::AddUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserStorageOutput(vplex::vsDirectory::AddUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserStorageOutput(vplex::vsDirectory::AddUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteUserStorageInput(vplex::vsDirectory::DeleteUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteUserStorageInput(vplex::vsDirectory::DeleteUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteUserStorageOutput(vplex::vsDirectory::DeleteUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenDeleteUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteUserStorageOutput(vplex::vsDirectory::DeleteUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseDeleteUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageNameInput(vplex::vsDirectory::ChangeUserStorageNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageNameInput(vplex::vsDirectory::ChangeUserStorageNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageNameOutput(vplex::vsDirectory::ChangeUserStorageNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageNameOutput(vplex::vsDirectory::ChangeUserStorageNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageQuotaInput(vplex::vsDirectory::ChangeUserStorageQuotaInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageQuotaInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageQuotaInput(vplex::vsDirectory::ChangeUserStorageQuotaInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageQuotaInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageQuotaOutput(vplex::vsDirectory::ChangeUserStorageQuotaOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeUserStorageQuotaOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageQuotaOutput(vplex::vsDirectory::ChangeUserStorageQuotaOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeUserStorageQuotaOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListUserStorageInput(vplex::vsDirectory::ListUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListUserStorageInput(vplex::vsDirectory::ListUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListUserStorageOutput(vplex::vsDirectory::ListUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenListUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListUserStorageOutput(vplex::vsDirectory::ListUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseListUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserStorageAddressInput(vplex::vsDirectory::GetUserStorageAddressInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserStorageAddressInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserStorageAddressInput(vplex::vsDirectory::GetUserStorageAddressInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserStorageAddressInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUserStorageAddress(vplex::vsDirectory::UserStorageAddress* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUserStorageAddressCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUserStorageAddress(vplex::vsDirectory::UserStorageAddress* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUserStorageAddressCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserStorageAddressOutput(vplex::vsDirectory::GetUserStorageAddressOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserStorageAddressOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserStorageAddressOutput(vplex::vsDirectory::GetUserStorageAddressOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserStorageAddressOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAssignUserDatacenterStorageInput(vplex::vsDirectory::AssignUserDatacenterStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAssignUserDatacenterStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAssignUserDatacenterStorageInput(vplex::vsDirectory::AssignUserDatacenterStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAssignUserDatacenterStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAssignUserDatacenterStorageOutput(vplex::vsDirectory::AssignUserDatacenterStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAssignUserDatacenterStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAssignUserDatacenterStorageOutput(vplex::vsDirectory::AssignUserDatacenterStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAssignUserDatacenterStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitForDatasetInput(vplex::vsDirectory::GetStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitForDatasetInput(vplex::vsDirectory::GetStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitForDatasetOutput(vplex::vsDirectory::GetStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitForDatasetOutput(vplex::vsDirectory::GetStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStoredDatasetsInput(vplex::vsDirectory::GetStoredDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStoredDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStoredDatasetsInput(vplex::vsDirectory::GetStoredDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStoredDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStoredDatasetsOutput(vplex::vsDirectory::GetStoredDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStoredDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStoredDatasetsOutput(vplex::vsDirectory::GetStoredDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStoredDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetProxyConnectionForClusterInput(vplex::vsDirectory::GetProxyConnectionForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetProxyConnectionForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetProxyConnectionForClusterInput(vplex::vsDirectory::GetProxyConnectionForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetProxyConnectionForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetProxyConnectionForClusterOutput(vplex::vsDirectory::GetProxyConnectionForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetProxyConnectionForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetProxyConnectionForClusterOutput(vplex::vsDirectory::GetProxyConnectionForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetProxyConnectionForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSendMessageToPSNInput(vplex::vsDirectory::SendMessageToPSNInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSendMessageToPSNInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSendMessageToPSNInput(vplex::vsDirectory::SendMessageToPSNInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSendMessageToPSNInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSendMessageToPSNOutput(vplex::vsDirectory::SendMessageToPSNOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenSendMessageToPSNOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSendMessageToPSNOutput(vplex::vsDirectory::SendMessageToPSNOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseSendMessageToPSNOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageUnitForDatasetInput(vplex::vsDirectory::ChangeStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageUnitForDatasetInput(vplex::vsDirectory::ChangeStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageUnitForDatasetOutput(vplex::vsDirectory::ChangeStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageUnitForDatasetOutput(vplex::vsDirectory::ChangeStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreateStorageClusterInput(vplex::vsDirectory::CreateStorageClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreateStorageClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreateStorageClusterInput(vplex::vsDirectory::CreateStorageClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreateStorageClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreateStorageClusterOutput(vplex::vsDirectory::CreateStorageClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenCreateStorageClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreateStorageClusterOutput(vplex::vsDirectory::CreateStorageClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseCreateStorageClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetMssInstancesForClusterInput(vplex::vsDirectory::GetMssInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetMssInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetMssInstancesForClusterInput(vplex::vsDirectory::GetMssInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetMssInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetMssInstancesForClusterOutput(vplex::vsDirectory::GetMssInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetMssInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetMssInstancesForClusterOutput(vplex::vsDirectory::GetMssInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetMssInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitsForClusterInput(vplex::vsDirectory::GetStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitsForClusterInput(vplex::vsDirectory::GetStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitsForClusterOutput(vplex::vsDirectory::GetStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitsForClusterOutput(vplex::vsDirectory::GetStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsInstancesForClusterInput(vplex::vsDirectory::GetBrsInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsInstancesForClusterInput(vplex::vsDirectory::GetBrsInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsInstancesForClusterOutput(vplex::vsDirectory::GetBrsInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsInstancesForClusterOutput(vplex::vsDirectory::GetBrsInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsStorageUnitsForClusterInput(vplex::vsDirectory::GetBrsStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsStorageUnitsForClusterInput(vplex::vsDirectory::GetBrsStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsStorageUnitsForClusterOutput(vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBrsStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsStorageUnitsForClusterOutput(vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBrsStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageAssignmentsForDatasetInput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageAssignmentsForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageAssignmentsForDatasetInput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageAssignmentsForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageAssignmentsForDatasetOutput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenChangeStorageAssignmentsForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageAssignmentsForDatasetOutput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseChangeStorageAssignmentsForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetStatusInput(vplex::vsDirectory::UpdateDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetStatusInput(vplex::vsDirectory::UpdateDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetStatusOutput(vplex::vsDirectory::UpdateDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetStatusOutput(vplex::vsDirectory::UpdateDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetBackupStatusInput(vplex::vsDirectory::UpdateDatasetBackupStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetBackupStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetBackupStatusInput(vplex::vsDirectory::UpdateDatasetBackupStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetBackupStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetBackupStatusOutput(vplex::vsDirectory::UpdateDatasetBackupStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetBackupStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetBackupStatusOutput(vplex::vsDirectory::UpdateDatasetBackupStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetBackupStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetArchiveStatusInput(vplex::vsDirectory::UpdateDatasetArchiveStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetArchiveStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetArchiveStatusInput(vplex::vsDirectory::UpdateDatasetArchiveStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetArchiveStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetArchiveStatusOutput(vplex::vsDirectory::UpdateDatasetArchiveStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenUpdateDatasetArchiveStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetArchiveStatusOutput(vplex::vsDirectory::UpdateDatasetArchiveStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseUpdateDatasetArchiveStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetStatusInput(vplex::vsDirectory::GetDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetStatusInput(vplex::vsDirectory::GetDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetStatusOutput(vplex::vsDirectory::GetDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetStatusOutput(vplex::vsDirectory::GetDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStoreDeviceEventInput(vplex::vsDirectory::StoreDeviceEventInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStoreDeviceEventInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStoreDeviceEventInput(vplex::vsDirectory::StoreDeviceEventInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStoreDeviceEventInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStoreDeviceEventOutput(vplex::vsDirectory::StoreDeviceEventOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenStoreDeviceEventOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStoreDeviceEventOutput(vplex::vsDirectory::StoreDeviceEventOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseStoreDeviceEventOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenEventInfo(vplex::vsDirectory::EventInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenEventInfoCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseEventInfo(vplex::vsDirectory::EventInfo* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseEventInfoCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDatasetStatusInput(vplex::vsDirectory::GetLinkedDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDatasetStatusInput(vplex::vsDirectory::GetLinkedDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDatasetStatusOutput(vplex::vsDirectory::GetLinkedDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetLinkedDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDatasetStatusOutput(vplex::vsDirectory::GetLinkedDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetLinkedDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserQuotaStatusInput(vplex::vsDirectory::GetUserQuotaStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserQuotaStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserQuotaStatusInput(vplex::vsDirectory::GetUserQuotaStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserQuotaStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserQuotaStatusOutput(vplex::vsDirectory::GetUserQuotaStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUserQuotaStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserQuotaStatusOutput(vplex::vsDirectory::GetUserQuotaStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUserQuotaStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetsToBackupInput(vplex::vsDirectory::GetDatasetsToBackupInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetsToBackupInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetsToBackupInput(vplex::vsDirectory::GetDatasetsToBackupInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetsToBackupInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetsToBackupOutput(vplex::vsDirectory::GetDatasetsToBackupOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetDatasetsToBackupOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetsToBackupOutput(vplex::vsDirectory::GetDatasetsToBackupOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetDatasetsToBackupOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBRSHostNameInput(vplex::vsDirectory::GetBRSHostNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBRSHostNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBRSHostNameInput(vplex::vsDirectory::GetBRSHostNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBRSHostNameInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBRSHostNameOutput(vplex::vsDirectory::GetBRSHostNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBRSHostNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBRSHostNameOutput(vplex::vsDirectory::GetBRSHostNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBRSHostNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBackupStorageUnitsForBrsInput(vplex::vsDirectory::GetBackupStorageUnitsForBrsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBackupStorageUnitsForBrsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBackupStorageUnitsForBrsInput(vplex::vsDirectory::GetBackupStorageUnitsForBrsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBackupStorageUnitsForBrsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBackupStorageUnitsForBrsOutput(vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetBackupStorageUnitsForBrsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBackupStorageUnitsForBrsOutput(vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetBackupStorageUnitsForBrsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUpdatedDatasetsInput(vplex::vsDirectory::GetUpdatedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUpdatedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUpdatedDatasetsInput(vplex::vsDirectory::GetUpdatedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUpdatedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUpdatedDatasetsOutput(vplex::vsDirectory::GetUpdatedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenGetUpdatedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUpdatedDatasetsOutput(vplex::vsDirectory::GetUpdatedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseGetUpdatedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetArchiveStorageDeviceInput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetArchiveStorageDeviceInput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenAddDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseAddDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDatasetArchiveStorageDeviceInput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDatasetArchiveStorageDeviceInput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processOpenRemoveDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state);
static void processCloseRemoveDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state);

vplex::vsDirectory::ParseStateAPIVersion::ParseStateAPIVersion(const char* outerTag, APIVersion* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateError::ParseStateError(const char* outerTag, Error* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSessionInfo::ParseStateSessionInfo(const char* outerTag, SessionInfo* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateETicketData::ParseStateETicketData(const char* outerTag, ETicketData* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateLocalization::ParseStateLocalization(const char* outerTag, Localization* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateTitleData::ParseStateTitleData(const char* outerTag, TitleData* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateTitleDetail::ParseStateTitleDetail(const char* outerTag, TitleDetail* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateContentDetail::ParseStateContentDetail(const char* outerTag, ContentDetail* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSaveData::ParseStateSaveData(const char* outerTag, SaveData* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateTitleTicket::ParseStateTitleTicket(const char* outerTag, TitleTicket* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSubscription::ParseStateSubscription(const char* outerTag, Subscription* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSyncDirectory::ParseStateSyncDirectory(const char* outerTag, SyncDirectory* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDatasetData::ParseStateDatasetData(const char* outerTag, DatasetData* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDatasetDetail::ParseStateDatasetDetail(const char* outerTag, DatasetDetail* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateStoredDataset::ParseStateStoredDataset(const char* outerTag, StoredDataset* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeviceInfo::ParseStateDeviceInfo(const char* outerTag, DeviceInfo* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateStorageAccessPort::ParseStateStorageAccessPort(const char* outerTag, StorageAccessPort* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateStorageAccess::ParseStateStorageAccess(const char* outerTag, StorageAccess* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeviceAccessTicket::ParseStateDeviceAccessTicket(const char* outerTag, DeviceAccessTicket* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUserStorage::ParseStateUserStorage(const char* outerTag, UserStorage* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdatedDataset::ParseStateUpdatedDataset(const char* outerTag, UpdatedDataset* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDatasetFilter::ParseStateDatasetFilter(const char* outerTag, DatasetFilter* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateMssDetail::ParseStateMssDetail(const char* outerTag, MssDetail* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateStorageUnitDetail::ParseStateStorageUnitDetail(const char* outerTag, StorageUnitDetail* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateBrsDetail::ParseStateBrsDetail(const char* outerTag, BrsDetail* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateBrsStorageUnitDetail::ParseStateBrsStorageUnitDetail(const char* outerTag, BrsStorageUnitDetail* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateBackupStatus::ParseStateBackupStatus(const char* outerTag, BackupStatus* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSaveTicketsInput::ParseStateGetSaveTicketsInput(const char* outerTag, GetSaveTicketsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSaveTicketsOutput::ParseStateGetSaveTicketsOutput(const char* outerTag, GetSaveTicketsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSaveDataInput::ParseStateGetSaveDataInput(const char* outerTag, GetSaveDataInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSaveDataOutput::ParseStateGetSaveDataOutput(const char* outerTag, GetSaveDataOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetOwnedTitlesInput::ParseStateGetOwnedTitlesInput(const char* outerTag, GetOwnedTitlesInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetOwnedTitlesOutput::ParseStateGetOwnedTitlesOutput(const char* outerTag, GetOwnedTitlesOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetTitlesInput::ParseStateGetTitlesInput(const char* outerTag, GetTitlesInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetTitlesOutput::ParseStateGetTitlesOutput(const char* outerTag, GetTitlesOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetTitleDetailsInput::ParseStateGetTitleDetailsInput(const char* outerTag, GetTitleDetailsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetTitleDetailsOutput::ParseStateGetTitleDetailsOutput(const char* outerTag, GetTitleDetailsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetAttestationChallengeInput::ParseStateGetAttestationChallengeInput(const char* outerTag, GetAttestationChallengeInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetAttestationChallengeOutput::ParseStateGetAttestationChallengeOutput(const char* outerTag, GetAttestationChallengeOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAuthenticateDeviceInput::ParseStateAuthenticateDeviceInput(const char* outerTag, AuthenticateDeviceInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAuthenticateDeviceOutput::ParseStateAuthenticateDeviceOutput(const char* outerTag, AuthenticateDeviceOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetOnlineTitleTicketInput::ParseStateGetOnlineTitleTicketInput(const char* outerTag, GetOnlineTitleTicketInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetOnlineTitleTicketOutput::ParseStateGetOnlineTitleTicketOutput(const char* outerTag, GetOnlineTitleTicketOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetOfflineTitleTicketsInput::ParseStateGetOfflineTitleTicketsInput(const char* outerTag, GetOfflineTitleTicketsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetOfflineTitleTicketsOutput::ParseStateGetOfflineTitleTicketsOutput(const char* outerTag, GetOfflineTitleTicketsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateListOwnedDataSetsInput::ParseStateListOwnedDataSetsInput(const char* outerTag, ListOwnedDataSetsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateListOwnedDataSetsOutput::ParseStateListOwnedDataSetsOutput(const char* outerTag, ListOwnedDataSetsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDatasetDetailsInput::ParseStateGetDatasetDetailsInput(const char* outerTag, GetDatasetDetailsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDatasetDetailsOutput::ParseStateGetDatasetDetailsOutput(const char* outerTag, GetDatasetDetailsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddDataSetInput::ParseStateAddDataSetInput(const char* outerTag, AddDataSetInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddDataSetOutput::ParseStateAddDataSetOutput(const char* outerTag, AddDataSetOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddCameraDatasetInput::ParseStateAddCameraDatasetInput(const char* outerTag, AddCameraDatasetInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddCameraDatasetOutput::ParseStateAddCameraDatasetOutput(const char* outerTag, AddCameraDatasetOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeleteDataSetInput::ParseStateDeleteDataSetInput(const char* outerTag, DeleteDataSetInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeleteDataSetOutput::ParseStateDeleteDataSetOutput(const char* outerTag, DeleteDataSetOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateRenameDataSetInput::ParseStateRenameDataSetInput(const char* outerTag, RenameDataSetInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateRenameDataSetOutput::ParseStateRenameDataSetOutput(const char* outerTag, RenameDataSetOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSetDataSetCacheInput::ParseStateSetDataSetCacheInput(const char* outerTag, SetDataSetCacheInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSetDataSetCacheOutput::ParseStateSetDataSetCacheOutput(const char* outerTag, SetDataSetCacheOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateRemoveDeviceFromSubscriptionsInput::ParseStateRemoveDeviceFromSubscriptionsInput(const char* outerTag, RemoveDeviceFromSubscriptionsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateRemoveDeviceFromSubscriptionsOutput::ParseStateRemoveDeviceFromSubscriptionsOutput(const char* outerTag, RemoveDeviceFromSubscriptionsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateListSubscriptionsInput::ParseStateListSubscriptionsInput(const char* outerTag, ListSubscriptionsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateListSubscriptionsOutput::ParseStateListSubscriptionsOutput(const char* outerTag, ListSubscriptionsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddSubscriptionsInput::ParseStateAddSubscriptionsInput(const char* outerTag, AddSubscriptionsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddSubscriptionsOutput::ParseStateAddSubscriptionsOutput(const char* outerTag, AddSubscriptionsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddUserDatasetSubscriptionInput::ParseStateAddUserDatasetSubscriptionInput(const char* outerTag, AddUserDatasetSubscriptionInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddUserDatasetSubscriptionOutput::ParseStateAddUserDatasetSubscriptionOutput(const char* outerTag, AddUserDatasetSubscriptionOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddCameraSubscriptionInput::ParseStateAddCameraSubscriptionInput(const char* outerTag, AddCameraSubscriptionInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddCameraSubscriptionOutput::ParseStateAddCameraSubscriptionOutput(const char* outerTag, AddCameraSubscriptionOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddDatasetSubscriptionInput::ParseStateAddDatasetSubscriptionInput(const char* outerTag, AddDatasetSubscriptionInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddDatasetSubscriptionOutput::ParseStateAddDatasetSubscriptionOutput(const char* outerTag, AddDatasetSubscriptionOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeleteSubscriptionsInput::ParseStateDeleteSubscriptionsInput(const char* outerTag, DeleteSubscriptionsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeleteSubscriptionsOutput::ParseStateDeleteSubscriptionsOutput(const char* outerTag, DeleteSubscriptionsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateSubscriptionFilterInput::ParseStateUpdateSubscriptionFilterInput(const char* outerTag, UpdateSubscriptionFilterInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateSubscriptionFilterOutput::ParseStateUpdateSubscriptionFilterOutput(const char* outerTag, UpdateSubscriptionFilterOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateSubscriptionLimitsInput::ParseStateUpdateSubscriptionLimitsInput(const char* outerTag, UpdateSubscriptionLimitsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateSubscriptionLimitsOutput::ParseStateUpdateSubscriptionLimitsOutput(const char* outerTag, UpdateSubscriptionLimitsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSubscriptionDetailsForDeviceInput::ParseStateGetSubscriptionDetailsForDeviceInput(const char* outerTag, GetSubscriptionDetailsForDeviceInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSubscriptionDetailsForDeviceOutput::ParseStateGetSubscriptionDetailsForDeviceOutput(const char* outerTag, GetSubscriptionDetailsForDeviceOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetCloudInfoInput::ParseStateGetCloudInfoInput(const char* outerTag, GetCloudInfoInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetCloudInfoOutput::ParseStateGetCloudInfoOutput(const char* outerTag, GetCloudInfoOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSubscribedDatasetsInput::ParseStateGetSubscribedDatasetsInput(const char* outerTag, GetSubscribedDatasetsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSubscribedDatasetsOutput::ParseStateGetSubscribedDatasetsOutput(const char* outerTag, GetSubscribedDatasetsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSubscriptionDetailsInput::ParseStateGetSubscriptionDetailsInput(const char* outerTag, GetSubscriptionDetailsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetSubscriptionDetailsOutput::ParseStateGetSubscriptionDetailsOutput(const char* outerTag, GetSubscriptionDetailsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateLinkDeviceInput::ParseStateLinkDeviceInput(const char* outerTag, LinkDeviceInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateLinkDeviceOutput::ParseStateLinkDeviceOutput(const char* outerTag, LinkDeviceOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUnlinkDeviceInput::ParseStateUnlinkDeviceInput(const char* outerTag, UnlinkDeviceInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUnlinkDeviceOutput::ParseStateUnlinkDeviceOutput(const char* outerTag, UnlinkDeviceOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSetDeviceNameInput::ParseStateSetDeviceNameInput(const char* outerTag, SetDeviceNameInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSetDeviceNameOutput::ParseStateSetDeviceNameOutput(const char* outerTag, SetDeviceNameOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDeviceInfoInput::ParseStateUpdateDeviceInfoInput(const char* outerTag, UpdateDeviceInfoInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDeviceInfoOutput::ParseStateUpdateDeviceInfoOutput(const char* outerTag, UpdateDeviceInfoOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDeviceLinkStateInput::ParseStateGetDeviceLinkStateInput(const char* outerTag, GetDeviceLinkStateInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDeviceLinkStateOutput::ParseStateGetDeviceLinkStateOutput(const char* outerTag, GetDeviceLinkStateOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDeviceNameInput::ParseStateGetDeviceNameInput(const char* outerTag, GetDeviceNameInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDeviceNameOutput::ParseStateGetDeviceNameOutput(const char* outerTag, GetDeviceNameOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetLinkedDevicesInput::ParseStateGetLinkedDevicesInput(const char* outerTag, GetLinkedDevicesInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetLinkedDevicesOutput::ParseStateGetLinkedDevicesOutput(const char* outerTag, GetLinkedDevicesOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetLoginSessionInput::ParseStateGetLoginSessionInput(const char* outerTag, GetLoginSessionInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetLoginSessionOutput::ParseStateGetLoginSessionOutput(const char* outerTag, GetLoginSessionOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateCreatePersonalStorageNodeInput::ParseStateCreatePersonalStorageNodeInput(const char* outerTag, CreatePersonalStorageNodeInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateCreatePersonalStorageNodeOutput::ParseStateCreatePersonalStorageNodeOutput(const char* outerTag, CreatePersonalStorageNodeOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetAsyncNoticeServerInput::ParseStateGetAsyncNoticeServerInput(const char* outerTag, GetAsyncNoticeServerInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetAsyncNoticeServerOutput::ParseStateGetAsyncNoticeServerOutput(const char* outerTag, GetAsyncNoticeServerOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateStorageNodeConnectionInput::ParseStateUpdateStorageNodeConnectionInput(const char* outerTag, UpdateStorageNodeConnectionInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateStorageNodeConnectionOutput::ParseStateUpdateStorageNodeConnectionOutput(const char* outerTag, UpdateStorageNodeConnectionOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateStorageNodeFeaturesInput::ParseStateUpdateStorageNodeFeaturesInput(const char* outerTag, UpdateStorageNodeFeaturesInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateStorageNodeFeaturesOutput::ParseStateUpdateStorageNodeFeaturesOutput(const char* outerTag, UpdateStorageNodeFeaturesOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetPSNDatasetLocationInput::ParseStateGetPSNDatasetLocationInput(const char* outerTag, GetPSNDatasetLocationInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetPSNDatasetLocationOutput::ParseStateGetPSNDatasetLocationOutput(const char* outerTag, GetPSNDatasetLocationOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdatePSNDatasetStatusInput::ParseStateUpdatePSNDatasetStatusInput(const char* outerTag, UpdatePSNDatasetStatusInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdatePSNDatasetStatusOutput::ParseStateUpdatePSNDatasetStatusOutput(const char* outerTag, UpdatePSNDatasetStatusOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddUserStorageInput::ParseStateAddUserStorageInput(const char* outerTag, AddUserStorageInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddUserStorageOutput::ParseStateAddUserStorageOutput(const char* outerTag, AddUserStorageOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeleteUserStorageInput::ParseStateDeleteUserStorageInput(const char* outerTag, DeleteUserStorageInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateDeleteUserStorageOutput::ParseStateDeleteUserStorageOutput(const char* outerTag, DeleteUserStorageOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeUserStorageNameInput::ParseStateChangeUserStorageNameInput(const char* outerTag, ChangeUserStorageNameInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeUserStorageNameOutput::ParseStateChangeUserStorageNameOutput(const char* outerTag, ChangeUserStorageNameOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeUserStorageQuotaInput::ParseStateChangeUserStorageQuotaInput(const char* outerTag, ChangeUserStorageQuotaInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeUserStorageQuotaOutput::ParseStateChangeUserStorageQuotaOutput(const char* outerTag, ChangeUserStorageQuotaOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateListUserStorageInput::ParseStateListUserStorageInput(const char* outerTag, ListUserStorageInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateListUserStorageOutput::ParseStateListUserStorageOutput(const char* outerTag, ListUserStorageOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetUserStorageAddressInput::ParseStateGetUserStorageAddressInput(const char* outerTag, GetUserStorageAddressInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUserStorageAddress::ParseStateUserStorageAddress(const char* outerTag, UserStorageAddress* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetUserStorageAddressOutput::ParseStateGetUserStorageAddressOutput(const char* outerTag, GetUserStorageAddressOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAssignUserDatacenterStorageInput::ParseStateAssignUserDatacenterStorageInput(const char* outerTag, AssignUserDatacenterStorageInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAssignUserDatacenterStorageOutput::ParseStateAssignUserDatacenterStorageOutput(const char* outerTag, AssignUserDatacenterStorageOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetStorageUnitForDatasetInput::ParseStateGetStorageUnitForDatasetInput(const char* outerTag, GetStorageUnitForDatasetInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetStorageUnitForDatasetOutput::ParseStateGetStorageUnitForDatasetOutput(const char* outerTag, GetStorageUnitForDatasetOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetStoredDatasetsInput::ParseStateGetStoredDatasetsInput(const char* outerTag, GetStoredDatasetsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetStoredDatasetsOutput::ParseStateGetStoredDatasetsOutput(const char* outerTag, GetStoredDatasetsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetProxyConnectionForClusterInput::ParseStateGetProxyConnectionForClusterInput(const char* outerTag, GetProxyConnectionForClusterInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetProxyConnectionForClusterOutput::ParseStateGetProxyConnectionForClusterOutput(const char* outerTag, GetProxyConnectionForClusterOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSendMessageToPSNInput::ParseStateSendMessageToPSNInput(const char* outerTag, SendMessageToPSNInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateSendMessageToPSNOutput::ParseStateSendMessageToPSNOutput(const char* outerTag, SendMessageToPSNOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeStorageUnitForDatasetInput::ParseStateChangeStorageUnitForDatasetInput(const char* outerTag, ChangeStorageUnitForDatasetInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeStorageUnitForDatasetOutput::ParseStateChangeStorageUnitForDatasetOutput(const char* outerTag, ChangeStorageUnitForDatasetOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateCreateStorageClusterInput::ParseStateCreateStorageClusterInput(const char* outerTag, CreateStorageClusterInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateCreateStorageClusterOutput::ParseStateCreateStorageClusterOutput(const char* outerTag, CreateStorageClusterOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetMssInstancesForClusterInput::ParseStateGetMssInstancesForClusterInput(const char* outerTag, GetMssInstancesForClusterInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetMssInstancesForClusterOutput::ParseStateGetMssInstancesForClusterOutput(const char* outerTag, GetMssInstancesForClusterOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetStorageUnitsForClusterInput::ParseStateGetStorageUnitsForClusterInput(const char* outerTag, GetStorageUnitsForClusterInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetStorageUnitsForClusterOutput::ParseStateGetStorageUnitsForClusterOutput(const char* outerTag, GetStorageUnitsForClusterOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBrsInstancesForClusterInput::ParseStateGetBrsInstancesForClusterInput(const char* outerTag, GetBrsInstancesForClusterInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBrsInstancesForClusterOutput::ParseStateGetBrsInstancesForClusterOutput(const char* outerTag, GetBrsInstancesForClusterOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBrsStorageUnitsForClusterInput::ParseStateGetBrsStorageUnitsForClusterInput(const char* outerTag, GetBrsStorageUnitsForClusterInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBrsStorageUnitsForClusterOutput::ParseStateGetBrsStorageUnitsForClusterOutput(const char* outerTag, GetBrsStorageUnitsForClusterOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeStorageAssignmentsForDatasetInput::ParseStateChangeStorageAssignmentsForDatasetInput(const char* outerTag, ChangeStorageAssignmentsForDatasetInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateChangeStorageAssignmentsForDatasetOutput::ParseStateChangeStorageAssignmentsForDatasetOutput(const char* outerTag, ChangeStorageAssignmentsForDatasetOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDatasetStatusInput::ParseStateUpdateDatasetStatusInput(const char* outerTag, UpdateDatasetStatusInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDatasetStatusOutput::ParseStateUpdateDatasetStatusOutput(const char* outerTag, UpdateDatasetStatusOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDatasetBackupStatusInput::ParseStateUpdateDatasetBackupStatusInput(const char* outerTag, UpdateDatasetBackupStatusInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDatasetBackupStatusOutput::ParseStateUpdateDatasetBackupStatusOutput(const char* outerTag, UpdateDatasetBackupStatusOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDatasetArchiveStatusInput::ParseStateUpdateDatasetArchiveStatusInput(const char* outerTag, UpdateDatasetArchiveStatusInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateUpdateDatasetArchiveStatusOutput::ParseStateUpdateDatasetArchiveStatusOutput(const char* outerTag, UpdateDatasetArchiveStatusOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDatasetStatusInput::ParseStateGetDatasetStatusInput(const char* outerTag, GetDatasetStatusInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDatasetStatusOutput::ParseStateGetDatasetStatusOutput(const char* outerTag, GetDatasetStatusOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateStoreDeviceEventInput::ParseStateStoreDeviceEventInput(const char* outerTag, StoreDeviceEventInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateStoreDeviceEventOutput::ParseStateStoreDeviceEventOutput(const char* outerTag, StoreDeviceEventOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateEventInfo::ParseStateEventInfo(const char* outerTag, EventInfo* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetLinkedDatasetStatusInput::ParseStateGetLinkedDatasetStatusInput(const char* outerTag, GetLinkedDatasetStatusInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetLinkedDatasetStatusOutput::ParseStateGetLinkedDatasetStatusOutput(const char* outerTag, GetLinkedDatasetStatusOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetUserQuotaStatusInput::ParseStateGetUserQuotaStatusInput(const char* outerTag, GetUserQuotaStatusInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetUserQuotaStatusOutput::ParseStateGetUserQuotaStatusOutput(const char* outerTag, GetUserQuotaStatusOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDatasetsToBackupInput::ParseStateGetDatasetsToBackupInput(const char* outerTag, GetDatasetsToBackupInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetDatasetsToBackupOutput::ParseStateGetDatasetsToBackupOutput(const char* outerTag, GetDatasetsToBackupOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBRSHostNameInput::ParseStateGetBRSHostNameInput(const char* outerTag, GetBRSHostNameInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBRSHostNameOutput::ParseStateGetBRSHostNameOutput(const char* outerTag, GetBRSHostNameOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBackupStorageUnitsForBrsInput::ParseStateGetBackupStorageUnitsForBrsInput(const char* outerTag, GetBackupStorageUnitsForBrsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetBackupStorageUnitsForBrsOutput::ParseStateGetBackupStorageUnitsForBrsOutput(const char* outerTag, GetBackupStorageUnitsForBrsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetUpdatedDatasetsInput::ParseStateGetUpdatedDatasetsInput(const char* outerTag, GetUpdatedDatasetsInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateGetUpdatedDatasetsOutput::ParseStateGetUpdatedDatasetsOutput(const char* outerTag, GetUpdatedDatasetsOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddDatasetArchiveStorageDeviceInput::ParseStateAddDatasetArchiveStorageDeviceInput(const char* outerTag, AddDatasetArchiveStorageDeviceInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateAddDatasetArchiveStorageDeviceOutput::ParseStateAddDatasetArchiveStorageDeviceOutput(const char* outerTag, AddDatasetArchiveStorageDeviceOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateRemoveDatasetArchiveStorageDeviceInput::ParseStateRemoveDatasetArchiveStorageDeviceInput(const char* outerTag, RemoveDatasetArchiveStorageDeviceInput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

vplex::vsDirectory::ParseStateRemoveDatasetArchiveStorageDeviceOutput::ParseStateRemoveDatasetArchiveStorageDeviceOutput(const char* outerTag, RemoveDatasetArchiveStorageDeviceOutput* message)
    : ProtoXmlParseState(outerTag, message)
{
    RegisterHandlers_vplex_vs_directory_service_types(impl);
}

void
vplex::vsDirectory::RegisterHandlers_vplex_vs_directory_service_types(ProtoXmlParseStateImpl* impl)
{
    if (impl->openHandlers.find("vplex.vsDirectory.APIVersion") == impl->openHandlers.end()) {
        impl->openHandlers["vplex.vsDirectory.APIVersion"] = processOpenAPIVersionCb;
        impl->closeHandlers["vplex.vsDirectory.APIVersion"] = processCloseAPIVersionCb;
        impl->openHandlers["vplex.vsDirectory.Error"] = processOpenErrorCb;
        impl->closeHandlers["vplex.vsDirectory.Error"] = processCloseErrorCb;
        impl->openHandlers["vplex.vsDirectory.SessionInfo"] = processOpenSessionInfoCb;
        impl->closeHandlers["vplex.vsDirectory.SessionInfo"] = processCloseSessionInfoCb;
        impl->openHandlers["vplex.vsDirectory.ETicketData"] = processOpenETicketDataCb;
        impl->closeHandlers["vplex.vsDirectory.ETicketData"] = processCloseETicketDataCb;
        impl->openHandlers["vplex.vsDirectory.Localization"] = processOpenLocalizationCb;
        impl->closeHandlers["vplex.vsDirectory.Localization"] = processCloseLocalizationCb;
        impl->openHandlers["vplex.vsDirectory.TitleData"] = processOpenTitleDataCb;
        impl->closeHandlers["vplex.vsDirectory.TitleData"] = processCloseTitleDataCb;
        impl->openHandlers["vplex.vsDirectory.TitleDetail"] = processOpenTitleDetailCb;
        impl->closeHandlers["vplex.vsDirectory.TitleDetail"] = processCloseTitleDetailCb;
        impl->openHandlers["vplex.vsDirectory.ContentDetail"] = processOpenContentDetailCb;
        impl->closeHandlers["vplex.vsDirectory.ContentDetail"] = processCloseContentDetailCb;
        impl->openHandlers["vplex.vsDirectory.SaveData"] = processOpenSaveDataCb;
        impl->closeHandlers["vplex.vsDirectory.SaveData"] = processCloseSaveDataCb;
        impl->openHandlers["vplex.vsDirectory.TitleTicket"] = processOpenTitleTicketCb;
        impl->closeHandlers["vplex.vsDirectory.TitleTicket"] = processCloseTitleTicketCb;
        impl->openHandlers["vplex.vsDirectory.Subscription"] = processOpenSubscriptionCb;
        impl->closeHandlers["vplex.vsDirectory.Subscription"] = processCloseSubscriptionCb;
        impl->openHandlers["vplex.vsDirectory.SyncDirectory"] = processOpenSyncDirectoryCb;
        impl->closeHandlers["vplex.vsDirectory.SyncDirectory"] = processCloseSyncDirectoryCb;
        impl->openHandlers["vplex.vsDirectory.DatasetData"] = processOpenDatasetDataCb;
        impl->closeHandlers["vplex.vsDirectory.DatasetData"] = processCloseDatasetDataCb;
        impl->openHandlers["vplex.vsDirectory.DatasetDetail"] = processOpenDatasetDetailCb;
        impl->closeHandlers["vplex.vsDirectory.DatasetDetail"] = processCloseDatasetDetailCb;
        impl->openHandlers["vplex.vsDirectory.StoredDataset"] = processOpenStoredDatasetCb;
        impl->closeHandlers["vplex.vsDirectory.StoredDataset"] = processCloseStoredDatasetCb;
        impl->openHandlers["vplex.vsDirectory.DeviceInfo"] = processOpenDeviceInfoCb;
        impl->closeHandlers["vplex.vsDirectory.DeviceInfo"] = processCloseDeviceInfoCb;
        impl->openHandlers["vplex.vsDirectory.StorageAccessPort"] = processOpenStorageAccessPortCb;
        impl->closeHandlers["vplex.vsDirectory.StorageAccessPort"] = processCloseStorageAccessPortCb;
        impl->openHandlers["vplex.vsDirectory.StorageAccess"] = processOpenStorageAccessCb;
        impl->closeHandlers["vplex.vsDirectory.StorageAccess"] = processCloseStorageAccessCb;
        impl->openHandlers["vplex.vsDirectory.DeviceAccessTicket"] = processOpenDeviceAccessTicketCb;
        impl->closeHandlers["vplex.vsDirectory.DeviceAccessTicket"] = processCloseDeviceAccessTicketCb;
        impl->openHandlers["vplex.vsDirectory.UserStorage"] = processOpenUserStorageCb;
        impl->closeHandlers["vplex.vsDirectory.UserStorage"] = processCloseUserStorageCb;
        impl->openHandlers["vplex.vsDirectory.UpdatedDataset"] = processOpenUpdatedDatasetCb;
        impl->closeHandlers["vplex.vsDirectory.UpdatedDataset"] = processCloseUpdatedDatasetCb;
        impl->openHandlers["vplex.vsDirectory.DatasetFilter"] = processOpenDatasetFilterCb;
        impl->closeHandlers["vplex.vsDirectory.DatasetFilter"] = processCloseDatasetFilterCb;
        impl->openHandlers["vplex.vsDirectory.MssDetail"] = processOpenMssDetailCb;
        impl->closeHandlers["vplex.vsDirectory.MssDetail"] = processCloseMssDetailCb;
        impl->openHandlers["vplex.vsDirectory.StorageUnitDetail"] = processOpenStorageUnitDetailCb;
        impl->closeHandlers["vplex.vsDirectory.StorageUnitDetail"] = processCloseStorageUnitDetailCb;
        impl->openHandlers["vplex.vsDirectory.BrsDetail"] = processOpenBrsDetailCb;
        impl->closeHandlers["vplex.vsDirectory.BrsDetail"] = processCloseBrsDetailCb;
        impl->openHandlers["vplex.vsDirectory.BrsStorageUnitDetail"] = processOpenBrsStorageUnitDetailCb;
        impl->closeHandlers["vplex.vsDirectory.BrsStorageUnitDetail"] = processCloseBrsStorageUnitDetailCb;
        impl->openHandlers["vplex.vsDirectory.BackupStatus"] = processOpenBackupStatusCb;
        impl->closeHandlers["vplex.vsDirectory.BackupStatus"] = processCloseBackupStatusCb;
        impl->openHandlers["vplex.vsDirectory.GetSaveTicketsInput"] = processOpenGetSaveTicketsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSaveTicketsInput"] = processCloseGetSaveTicketsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetSaveTicketsOutput"] = processOpenGetSaveTicketsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSaveTicketsOutput"] = processCloseGetSaveTicketsOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetSaveDataInput"] = processOpenGetSaveDataInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSaveDataInput"] = processCloseGetSaveDataInputCb;
        impl->openHandlers["vplex.vsDirectory.GetSaveDataOutput"] = processOpenGetSaveDataOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSaveDataOutput"] = processCloseGetSaveDataOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetOwnedTitlesInput"] = processOpenGetOwnedTitlesInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetOwnedTitlesInput"] = processCloseGetOwnedTitlesInputCb;
        impl->openHandlers["vplex.vsDirectory.GetOwnedTitlesOutput"] = processOpenGetOwnedTitlesOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetOwnedTitlesOutput"] = processCloseGetOwnedTitlesOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetTitlesInput"] = processOpenGetTitlesInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetTitlesInput"] = processCloseGetTitlesInputCb;
        impl->openHandlers["vplex.vsDirectory.GetTitlesOutput"] = processOpenGetTitlesOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetTitlesOutput"] = processCloseGetTitlesOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetTitleDetailsInput"] = processOpenGetTitleDetailsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetTitleDetailsInput"] = processCloseGetTitleDetailsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetTitleDetailsOutput"] = processOpenGetTitleDetailsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetTitleDetailsOutput"] = processCloseGetTitleDetailsOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetAttestationChallengeInput"] = processOpenGetAttestationChallengeInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetAttestationChallengeInput"] = processCloseGetAttestationChallengeInputCb;
        impl->openHandlers["vplex.vsDirectory.GetAttestationChallengeOutput"] = processOpenGetAttestationChallengeOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetAttestationChallengeOutput"] = processCloseGetAttestationChallengeOutputCb;
        impl->openHandlers["vplex.vsDirectory.AuthenticateDeviceInput"] = processOpenAuthenticateDeviceInputCb;
        impl->closeHandlers["vplex.vsDirectory.AuthenticateDeviceInput"] = processCloseAuthenticateDeviceInputCb;
        impl->openHandlers["vplex.vsDirectory.AuthenticateDeviceOutput"] = processOpenAuthenticateDeviceOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AuthenticateDeviceOutput"] = processCloseAuthenticateDeviceOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetOnlineTitleTicketInput"] = processOpenGetOnlineTitleTicketInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetOnlineTitleTicketInput"] = processCloseGetOnlineTitleTicketInputCb;
        impl->openHandlers["vplex.vsDirectory.GetOnlineTitleTicketOutput"] = processOpenGetOnlineTitleTicketOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetOnlineTitleTicketOutput"] = processCloseGetOnlineTitleTicketOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetOfflineTitleTicketsInput"] = processOpenGetOfflineTitleTicketsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetOfflineTitleTicketsInput"] = processCloseGetOfflineTitleTicketsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetOfflineTitleTicketsOutput"] = processOpenGetOfflineTitleTicketsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetOfflineTitleTicketsOutput"] = processCloseGetOfflineTitleTicketsOutputCb;
        impl->openHandlers["vplex.vsDirectory.ListOwnedDataSetsInput"] = processOpenListOwnedDataSetsInputCb;
        impl->closeHandlers["vplex.vsDirectory.ListOwnedDataSetsInput"] = processCloseListOwnedDataSetsInputCb;
        impl->openHandlers["vplex.vsDirectory.ListOwnedDataSetsOutput"] = processOpenListOwnedDataSetsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.ListOwnedDataSetsOutput"] = processCloseListOwnedDataSetsOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetDatasetDetailsInput"] = processOpenGetDatasetDetailsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDatasetDetailsInput"] = processCloseGetDatasetDetailsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetDatasetDetailsOutput"] = processOpenGetDatasetDetailsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDatasetDetailsOutput"] = processCloseGetDatasetDetailsOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddDataSetInput"] = processOpenAddDataSetInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddDataSetInput"] = processCloseAddDataSetInputCb;
        impl->openHandlers["vplex.vsDirectory.AddDataSetOutput"] = processOpenAddDataSetOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddDataSetOutput"] = processCloseAddDataSetOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddCameraDatasetInput"] = processOpenAddCameraDatasetInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddCameraDatasetInput"] = processCloseAddCameraDatasetInputCb;
        impl->openHandlers["vplex.vsDirectory.AddCameraDatasetOutput"] = processOpenAddCameraDatasetOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddCameraDatasetOutput"] = processCloseAddCameraDatasetOutputCb;
        impl->openHandlers["vplex.vsDirectory.DeleteDataSetInput"] = processOpenDeleteDataSetInputCb;
        impl->closeHandlers["vplex.vsDirectory.DeleteDataSetInput"] = processCloseDeleteDataSetInputCb;
        impl->openHandlers["vplex.vsDirectory.DeleteDataSetOutput"] = processOpenDeleteDataSetOutputCb;
        impl->closeHandlers["vplex.vsDirectory.DeleteDataSetOutput"] = processCloseDeleteDataSetOutputCb;
        impl->openHandlers["vplex.vsDirectory.RenameDataSetInput"] = processOpenRenameDataSetInputCb;
        impl->closeHandlers["vplex.vsDirectory.RenameDataSetInput"] = processCloseRenameDataSetInputCb;
        impl->openHandlers["vplex.vsDirectory.RenameDataSetOutput"] = processOpenRenameDataSetOutputCb;
        impl->closeHandlers["vplex.vsDirectory.RenameDataSetOutput"] = processCloseRenameDataSetOutputCb;
        impl->openHandlers["vplex.vsDirectory.SetDataSetCacheInput"] = processOpenSetDataSetCacheInputCb;
        impl->closeHandlers["vplex.vsDirectory.SetDataSetCacheInput"] = processCloseSetDataSetCacheInputCb;
        impl->openHandlers["vplex.vsDirectory.SetDataSetCacheOutput"] = processOpenSetDataSetCacheOutputCb;
        impl->closeHandlers["vplex.vsDirectory.SetDataSetCacheOutput"] = processCloseSetDataSetCacheOutputCb;
        impl->openHandlers["vplex.vsDirectory.RemoveDeviceFromSubscriptionsInput"] = processOpenRemoveDeviceFromSubscriptionsInputCb;
        impl->closeHandlers["vplex.vsDirectory.RemoveDeviceFromSubscriptionsInput"] = processCloseRemoveDeviceFromSubscriptionsInputCb;
        impl->openHandlers["vplex.vsDirectory.RemoveDeviceFromSubscriptionsOutput"] = processOpenRemoveDeviceFromSubscriptionsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.RemoveDeviceFromSubscriptionsOutput"] = processCloseRemoveDeviceFromSubscriptionsOutputCb;
        impl->openHandlers["vplex.vsDirectory.ListSubscriptionsInput"] = processOpenListSubscriptionsInputCb;
        impl->closeHandlers["vplex.vsDirectory.ListSubscriptionsInput"] = processCloseListSubscriptionsInputCb;
        impl->openHandlers["vplex.vsDirectory.ListSubscriptionsOutput"] = processOpenListSubscriptionsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.ListSubscriptionsOutput"] = processCloseListSubscriptionsOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddSubscriptionsInput"] = processOpenAddSubscriptionsInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddSubscriptionsInput"] = processCloseAddSubscriptionsInputCb;
        impl->openHandlers["vplex.vsDirectory.AddSubscriptionsOutput"] = processOpenAddSubscriptionsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddSubscriptionsOutput"] = processCloseAddSubscriptionsOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddUserDatasetSubscriptionInput"] = processOpenAddUserDatasetSubscriptionInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddUserDatasetSubscriptionInput"] = processCloseAddUserDatasetSubscriptionInputCb;
        impl->openHandlers["vplex.vsDirectory.AddUserDatasetSubscriptionOutput"] = processOpenAddUserDatasetSubscriptionOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddUserDatasetSubscriptionOutput"] = processCloseAddUserDatasetSubscriptionOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddCameraSubscriptionInput"] = processOpenAddCameraSubscriptionInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddCameraSubscriptionInput"] = processCloseAddCameraSubscriptionInputCb;
        impl->openHandlers["vplex.vsDirectory.AddCameraSubscriptionOutput"] = processOpenAddCameraSubscriptionOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddCameraSubscriptionOutput"] = processCloseAddCameraSubscriptionOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddDatasetSubscriptionInput"] = processOpenAddDatasetSubscriptionInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddDatasetSubscriptionInput"] = processCloseAddDatasetSubscriptionInputCb;
        impl->openHandlers["vplex.vsDirectory.AddDatasetSubscriptionOutput"] = processOpenAddDatasetSubscriptionOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddDatasetSubscriptionOutput"] = processCloseAddDatasetSubscriptionOutputCb;
        impl->openHandlers["vplex.vsDirectory.DeleteSubscriptionsInput"] = processOpenDeleteSubscriptionsInputCb;
        impl->closeHandlers["vplex.vsDirectory.DeleteSubscriptionsInput"] = processCloseDeleteSubscriptionsInputCb;
        impl->openHandlers["vplex.vsDirectory.DeleteSubscriptionsOutput"] = processOpenDeleteSubscriptionsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.DeleteSubscriptionsOutput"] = processCloseDeleteSubscriptionsOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateSubscriptionFilterInput"] = processOpenUpdateSubscriptionFilterInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateSubscriptionFilterInput"] = processCloseUpdateSubscriptionFilterInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateSubscriptionFilterOutput"] = processOpenUpdateSubscriptionFilterOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateSubscriptionFilterOutput"] = processCloseUpdateSubscriptionFilterOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateSubscriptionLimitsInput"] = processOpenUpdateSubscriptionLimitsInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateSubscriptionLimitsInput"] = processCloseUpdateSubscriptionLimitsInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateSubscriptionLimitsOutput"] = processOpenUpdateSubscriptionLimitsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateSubscriptionLimitsOutput"] = processCloseUpdateSubscriptionLimitsOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetSubscriptionDetailsForDeviceInput"] = processOpenGetSubscriptionDetailsForDeviceInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSubscriptionDetailsForDeviceInput"] = processCloseGetSubscriptionDetailsForDeviceInputCb;
        impl->openHandlers["vplex.vsDirectory.GetSubscriptionDetailsForDeviceOutput"] = processOpenGetSubscriptionDetailsForDeviceOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSubscriptionDetailsForDeviceOutput"] = processCloseGetSubscriptionDetailsForDeviceOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetCloudInfoInput"] = processOpenGetCloudInfoInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetCloudInfoInput"] = processCloseGetCloudInfoInputCb;
        impl->openHandlers["vplex.vsDirectory.GetCloudInfoOutput"] = processOpenGetCloudInfoOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetCloudInfoOutput"] = processCloseGetCloudInfoOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetSubscribedDatasetsInput"] = processOpenGetSubscribedDatasetsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSubscribedDatasetsInput"] = processCloseGetSubscribedDatasetsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetSubscribedDatasetsOutput"] = processOpenGetSubscribedDatasetsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSubscribedDatasetsOutput"] = processCloseGetSubscribedDatasetsOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetSubscriptionDetailsInput"] = processOpenGetSubscriptionDetailsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSubscriptionDetailsInput"] = processCloseGetSubscriptionDetailsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetSubscriptionDetailsOutput"] = processOpenGetSubscriptionDetailsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetSubscriptionDetailsOutput"] = processCloseGetSubscriptionDetailsOutputCb;
        impl->openHandlers["vplex.vsDirectory.LinkDeviceInput"] = processOpenLinkDeviceInputCb;
        impl->closeHandlers["vplex.vsDirectory.LinkDeviceInput"] = processCloseLinkDeviceInputCb;
        impl->openHandlers["vplex.vsDirectory.LinkDeviceOutput"] = processOpenLinkDeviceOutputCb;
        impl->closeHandlers["vplex.vsDirectory.LinkDeviceOutput"] = processCloseLinkDeviceOutputCb;
        impl->openHandlers["vplex.vsDirectory.UnlinkDeviceInput"] = processOpenUnlinkDeviceInputCb;
        impl->closeHandlers["vplex.vsDirectory.UnlinkDeviceInput"] = processCloseUnlinkDeviceInputCb;
        impl->openHandlers["vplex.vsDirectory.UnlinkDeviceOutput"] = processOpenUnlinkDeviceOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UnlinkDeviceOutput"] = processCloseUnlinkDeviceOutputCb;
        impl->openHandlers["vplex.vsDirectory.SetDeviceNameInput"] = processOpenSetDeviceNameInputCb;
        impl->closeHandlers["vplex.vsDirectory.SetDeviceNameInput"] = processCloseSetDeviceNameInputCb;
        impl->openHandlers["vplex.vsDirectory.SetDeviceNameOutput"] = processOpenSetDeviceNameOutputCb;
        impl->closeHandlers["vplex.vsDirectory.SetDeviceNameOutput"] = processCloseSetDeviceNameOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDeviceInfoInput"] = processOpenUpdateDeviceInfoInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDeviceInfoInput"] = processCloseUpdateDeviceInfoInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDeviceInfoOutput"] = processOpenUpdateDeviceInfoOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDeviceInfoOutput"] = processCloseUpdateDeviceInfoOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetDeviceLinkStateInput"] = processOpenGetDeviceLinkStateInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDeviceLinkStateInput"] = processCloseGetDeviceLinkStateInputCb;
        impl->openHandlers["vplex.vsDirectory.GetDeviceLinkStateOutput"] = processOpenGetDeviceLinkStateOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDeviceLinkStateOutput"] = processCloseGetDeviceLinkStateOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetDeviceNameInput"] = processOpenGetDeviceNameInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDeviceNameInput"] = processCloseGetDeviceNameInputCb;
        impl->openHandlers["vplex.vsDirectory.GetDeviceNameOutput"] = processOpenGetDeviceNameOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDeviceNameOutput"] = processCloseGetDeviceNameOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetLinkedDevicesInput"] = processOpenGetLinkedDevicesInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetLinkedDevicesInput"] = processCloseGetLinkedDevicesInputCb;
        impl->openHandlers["vplex.vsDirectory.GetLinkedDevicesOutput"] = processOpenGetLinkedDevicesOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetLinkedDevicesOutput"] = processCloseGetLinkedDevicesOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetLoginSessionInput"] = processOpenGetLoginSessionInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetLoginSessionInput"] = processCloseGetLoginSessionInputCb;
        impl->openHandlers["vplex.vsDirectory.GetLoginSessionOutput"] = processOpenGetLoginSessionOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetLoginSessionOutput"] = processCloseGetLoginSessionOutputCb;
        impl->openHandlers["vplex.vsDirectory.CreatePersonalStorageNodeInput"] = processOpenCreatePersonalStorageNodeInputCb;
        impl->closeHandlers["vplex.vsDirectory.CreatePersonalStorageNodeInput"] = processCloseCreatePersonalStorageNodeInputCb;
        impl->openHandlers["vplex.vsDirectory.CreatePersonalStorageNodeOutput"] = processOpenCreatePersonalStorageNodeOutputCb;
        impl->closeHandlers["vplex.vsDirectory.CreatePersonalStorageNodeOutput"] = processCloseCreatePersonalStorageNodeOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetAsyncNoticeServerInput"] = processOpenGetAsyncNoticeServerInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetAsyncNoticeServerInput"] = processCloseGetAsyncNoticeServerInputCb;
        impl->openHandlers["vplex.vsDirectory.GetAsyncNoticeServerOutput"] = processOpenGetAsyncNoticeServerOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetAsyncNoticeServerOutput"] = processCloseGetAsyncNoticeServerOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateStorageNodeConnectionInput"] = processOpenUpdateStorageNodeConnectionInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateStorageNodeConnectionInput"] = processCloseUpdateStorageNodeConnectionInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateStorageNodeConnectionOutput"] = processOpenUpdateStorageNodeConnectionOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateStorageNodeConnectionOutput"] = processCloseUpdateStorageNodeConnectionOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateStorageNodeFeaturesInput"] = processOpenUpdateStorageNodeFeaturesInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateStorageNodeFeaturesInput"] = processCloseUpdateStorageNodeFeaturesInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateStorageNodeFeaturesOutput"] = processOpenUpdateStorageNodeFeaturesOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateStorageNodeFeaturesOutput"] = processCloseUpdateStorageNodeFeaturesOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetPSNDatasetLocationInput"] = processOpenGetPSNDatasetLocationInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetPSNDatasetLocationInput"] = processCloseGetPSNDatasetLocationInputCb;
        impl->openHandlers["vplex.vsDirectory.GetPSNDatasetLocationOutput"] = processOpenGetPSNDatasetLocationOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetPSNDatasetLocationOutput"] = processCloseGetPSNDatasetLocationOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdatePSNDatasetStatusInput"] = processOpenUpdatePSNDatasetStatusInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdatePSNDatasetStatusInput"] = processCloseUpdatePSNDatasetStatusInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdatePSNDatasetStatusOutput"] = processOpenUpdatePSNDatasetStatusOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdatePSNDatasetStatusOutput"] = processCloseUpdatePSNDatasetStatusOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddUserStorageInput"] = processOpenAddUserStorageInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddUserStorageInput"] = processCloseAddUserStorageInputCb;
        impl->openHandlers["vplex.vsDirectory.AddUserStorageOutput"] = processOpenAddUserStorageOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddUserStorageOutput"] = processCloseAddUserStorageOutputCb;
        impl->openHandlers["vplex.vsDirectory.DeleteUserStorageInput"] = processOpenDeleteUserStorageInputCb;
        impl->closeHandlers["vplex.vsDirectory.DeleteUserStorageInput"] = processCloseDeleteUserStorageInputCb;
        impl->openHandlers["vplex.vsDirectory.DeleteUserStorageOutput"] = processOpenDeleteUserStorageOutputCb;
        impl->closeHandlers["vplex.vsDirectory.DeleteUserStorageOutput"] = processCloseDeleteUserStorageOutputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeUserStorageNameInput"] = processOpenChangeUserStorageNameInputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeUserStorageNameInput"] = processCloseChangeUserStorageNameInputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeUserStorageNameOutput"] = processOpenChangeUserStorageNameOutputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeUserStorageNameOutput"] = processCloseChangeUserStorageNameOutputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeUserStorageQuotaInput"] = processOpenChangeUserStorageQuotaInputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeUserStorageQuotaInput"] = processCloseChangeUserStorageQuotaInputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeUserStorageQuotaOutput"] = processOpenChangeUserStorageQuotaOutputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeUserStorageQuotaOutput"] = processCloseChangeUserStorageQuotaOutputCb;
        impl->openHandlers["vplex.vsDirectory.ListUserStorageInput"] = processOpenListUserStorageInputCb;
        impl->closeHandlers["vplex.vsDirectory.ListUserStorageInput"] = processCloseListUserStorageInputCb;
        impl->openHandlers["vplex.vsDirectory.ListUserStorageOutput"] = processOpenListUserStorageOutputCb;
        impl->closeHandlers["vplex.vsDirectory.ListUserStorageOutput"] = processCloseListUserStorageOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetUserStorageAddressInput"] = processOpenGetUserStorageAddressInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetUserStorageAddressInput"] = processCloseGetUserStorageAddressInputCb;
        impl->openHandlers["vplex.vsDirectory.UserStorageAddress"] = processOpenUserStorageAddressCb;
        impl->closeHandlers["vplex.vsDirectory.UserStorageAddress"] = processCloseUserStorageAddressCb;
        impl->openHandlers["vplex.vsDirectory.GetUserStorageAddressOutput"] = processOpenGetUserStorageAddressOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetUserStorageAddressOutput"] = processCloseGetUserStorageAddressOutputCb;
        impl->openHandlers["vplex.vsDirectory.AssignUserDatacenterStorageInput"] = processOpenAssignUserDatacenterStorageInputCb;
        impl->closeHandlers["vplex.vsDirectory.AssignUserDatacenterStorageInput"] = processCloseAssignUserDatacenterStorageInputCb;
        impl->openHandlers["vplex.vsDirectory.AssignUserDatacenterStorageOutput"] = processOpenAssignUserDatacenterStorageOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AssignUserDatacenterStorageOutput"] = processCloseAssignUserDatacenterStorageOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetStorageUnitForDatasetInput"] = processOpenGetStorageUnitForDatasetInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetStorageUnitForDatasetInput"] = processCloseGetStorageUnitForDatasetInputCb;
        impl->openHandlers["vplex.vsDirectory.GetStorageUnitForDatasetOutput"] = processOpenGetStorageUnitForDatasetOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetStorageUnitForDatasetOutput"] = processCloseGetStorageUnitForDatasetOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetStoredDatasetsInput"] = processOpenGetStoredDatasetsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetStoredDatasetsInput"] = processCloseGetStoredDatasetsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetStoredDatasetsOutput"] = processOpenGetStoredDatasetsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetStoredDatasetsOutput"] = processCloseGetStoredDatasetsOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetProxyConnectionForClusterInput"] = processOpenGetProxyConnectionForClusterInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetProxyConnectionForClusterInput"] = processCloseGetProxyConnectionForClusterInputCb;
        impl->openHandlers["vplex.vsDirectory.GetProxyConnectionForClusterOutput"] = processOpenGetProxyConnectionForClusterOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetProxyConnectionForClusterOutput"] = processCloseGetProxyConnectionForClusterOutputCb;
        impl->openHandlers["vplex.vsDirectory.SendMessageToPSNInput"] = processOpenSendMessageToPSNInputCb;
        impl->closeHandlers["vplex.vsDirectory.SendMessageToPSNInput"] = processCloseSendMessageToPSNInputCb;
        impl->openHandlers["vplex.vsDirectory.SendMessageToPSNOutput"] = processOpenSendMessageToPSNOutputCb;
        impl->closeHandlers["vplex.vsDirectory.SendMessageToPSNOutput"] = processCloseSendMessageToPSNOutputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeStorageUnitForDatasetInput"] = processOpenChangeStorageUnitForDatasetInputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeStorageUnitForDatasetInput"] = processCloseChangeStorageUnitForDatasetInputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeStorageUnitForDatasetOutput"] = processOpenChangeStorageUnitForDatasetOutputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeStorageUnitForDatasetOutput"] = processCloseChangeStorageUnitForDatasetOutputCb;
        impl->openHandlers["vplex.vsDirectory.CreateStorageClusterInput"] = processOpenCreateStorageClusterInputCb;
        impl->closeHandlers["vplex.vsDirectory.CreateStorageClusterInput"] = processCloseCreateStorageClusterInputCb;
        impl->openHandlers["vplex.vsDirectory.CreateStorageClusterOutput"] = processOpenCreateStorageClusterOutputCb;
        impl->closeHandlers["vplex.vsDirectory.CreateStorageClusterOutput"] = processCloseCreateStorageClusterOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetMssInstancesForClusterInput"] = processOpenGetMssInstancesForClusterInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetMssInstancesForClusterInput"] = processCloseGetMssInstancesForClusterInputCb;
        impl->openHandlers["vplex.vsDirectory.GetMssInstancesForClusterOutput"] = processOpenGetMssInstancesForClusterOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetMssInstancesForClusterOutput"] = processCloseGetMssInstancesForClusterOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetStorageUnitsForClusterInput"] = processOpenGetStorageUnitsForClusterInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetStorageUnitsForClusterInput"] = processCloseGetStorageUnitsForClusterInputCb;
        impl->openHandlers["vplex.vsDirectory.GetStorageUnitsForClusterOutput"] = processOpenGetStorageUnitsForClusterOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetStorageUnitsForClusterOutput"] = processCloseGetStorageUnitsForClusterOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetBrsInstancesForClusterInput"] = processOpenGetBrsInstancesForClusterInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBrsInstancesForClusterInput"] = processCloseGetBrsInstancesForClusterInputCb;
        impl->openHandlers["vplex.vsDirectory.GetBrsInstancesForClusterOutput"] = processOpenGetBrsInstancesForClusterOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBrsInstancesForClusterOutput"] = processCloseGetBrsInstancesForClusterOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetBrsStorageUnitsForClusterInput"] = processOpenGetBrsStorageUnitsForClusterInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBrsStorageUnitsForClusterInput"] = processCloseGetBrsStorageUnitsForClusterInputCb;
        impl->openHandlers["vplex.vsDirectory.GetBrsStorageUnitsForClusterOutput"] = processOpenGetBrsStorageUnitsForClusterOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBrsStorageUnitsForClusterOutput"] = processCloseGetBrsStorageUnitsForClusterOutputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeStorageAssignmentsForDatasetInput"] = processOpenChangeStorageAssignmentsForDatasetInputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeStorageAssignmentsForDatasetInput"] = processCloseChangeStorageAssignmentsForDatasetInputCb;
        impl->openHandlers["vplex.vsDirectory.ChangeStorageAssignmentsForDatasetOutput"] = processOpenChangeStorageAssignmentsForDatasetOutputCb;
        impl->closeHandlers["vplex.vsDirectory.ChangeStorageAssignmentsForDatasetOutput"] = processCloseChangeStorageAssignmentsForDatasetOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDatasetStatusInput"] = processOpenUpdateDatasetStatusInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDatasetStatusInput"] = processCloseUpdateDatasetStatusInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDatasetStatusOutput"] = processOpenUpdateDatasetStatusOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDatasetStatusOutput"] = processCloseUpdateDatasetStatusOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDatasetBackupStatusInput"] = processOpenUpdateDatasetBackupStatusInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDatasetBackupStatusInput"] = processCloseUpdateDatasetBackupStatusInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDatasetBackupStatusOutput"] = processOpenUpdateDatasetBackupStatusOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDatasetBackupStatusOutput"] = processCloseUpdateDatasetBackupStatusOutputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDatasetArchiveStatusInput"] = processOpenUpdateDatasetArchiveStatusInputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDatasetArchiveStatusInput"] = processCloseUpdateDatasetArchiveStatusInputCb;
        impl->openHandlers["vplex.vsDirectory.UpdateDatasetArchiveStatusOutput"] = processOpenUpdateDatasetArchiveStatusOutputCb;
        impl->closeHandlers["vplex.vsDirectory.UpdateDatasetArchiveStatusOutput"] = processCloseUpdateDatasetArchiveStatusOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetDatasetStatusInput"] = processOpenGetDatasetStatusInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDatasetStatusInput"] = processCloseGetDatasetStatusInputCb;
        impl->openHandlers["vplex.vsDirectory.GetDatasetStatusOutput"] = processOpenGetDatasetStatusOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDatasetStatusOutput"] = processCloseGetDatasetStatusOutputCb;
        impl->openHandlers["vplex.vsDirectory.StoreDeviceEventInput"] = processOpenStoreDeviceEventInputCb;
        impl->closeHandlers["vplex.vsDirectory.StoreDeviceEventInput"] = processCloseStoreDeviceEventInputCb;
        impl->openHandlers["vplex.vsDirectory.StoreDeviceEventOutput"] = processOpenStoreDeviceEventOutputCb;
        impl->closeHandlers["vplex.vsDirectory.StoreDeviceEventOutput"] = processCloseStoreDeviceEventOutputCb;
        impl->openHandlers["vplex.vsDirectory.EventInfo"] = processOpenEventInfoCb;
        impl->closeHandlers["vplex.vsDirectory.EventInfo"] = processCloseEventInfoCb;
        impl->openHandlers["vplex.vsDirectory.GetLinkedDatasetStatusInput"] = processOpenGetLinkedDatasetStatusInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetLinkedDatasetStatusInput"] = processCloseGetLinkedDatasetStatusInputCb;
        impl->openHandlers["vplex.vsDirectory.GetLinkedDatasetStatusOutput"] = processOpenGetLinkedDatasetStatusOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetLinkedDatasetStatusOutput"] = processCloseGetLinkedDatasetStatusOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetUserQuotaStatusInput"] = processOpenGetUserQuotaStatusInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetUserQuotaStatusInput"] = processCloseGetUserQuotaStatusInputCb;
        impl->openHandlers["vplex.vsDirectory.GetUserQuotaStatusOutput"] = processOpenGetUserQuotaStatusOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetUserQuotaStatusOutput"] = processCloseGetUserQuotaStatusOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetDatasetsToBackupInput"] = processOpenGetDatasetsToBackupInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDatasetsToBackupInput"] = processCloseGetDatasetsToBackupInputCb;
        impl->openHandlers["vplex.vsDirectory.GetDatasetsToBackupOutput"] = processOpenGetDatasetsToBackupOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetDatasetsToBackupOutput"] = processCloseGetDatasetsToBackupOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetBRSHostNameInput"] = processOpenGetBRSHostNameInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBRSHostNameInput"] = processCloseGetBRSHostNameInputCb;
        impl->openHandlers["vplex.vsDirectory.GetBRSHostNameOutput"] = processOpenGetBRSHostNameOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBRSHostNameOutput"] = processCloseGetBRSHostNameOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetBackupStorageUnitsForBrsInput"] = processOpenGetBackupStorageUnitsForBrsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBackupStorageUnitsForBrsInput"] = processCloseGetBackupStorageUnitsForBrsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetBackupStorageUnitsForBrsOutput"] = processOpenGetBackupStorageUnitsForBrsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetBackupStorageUnitsForBrsOutput"] = processCloseGetBackupStorageUnitsForBrsOutputCb;
        impl->openHandlers["vplex.vsDirectory.GetUpdatedDatasetsInput"] = processOpenGetUpdatedDatasetsInputCb;
        impl->closeHandlers["vplex.vsDirectory.GetUpdatedDatasetsInput"] = processCloseGetUpdatedDatasetsInputCb;
        impl->openHandlers["vplex.vsDirectory.GetUpdatedDatasetsOutput"] = processOpenGetUpdatedDatasetsOutputCb;
        impl->closeHandlers["vplex.vsDirectory.GetUpdatedDatasetsOutput"] = processCloseGetUpdatedDatasetsOutputCb;
        impl->openHandlers["vplex.vsDirectory.AddDatasetArchiveStorageDeviceInput"] = processOpenAddDatasetArchiveStorageDeviceInputCb;
        impl->closeHandlers["vplex.vsDirectory.AddDatasetArchiveStorageDeviceInput"] = processCloseAddDatasetArchiveStorageDeviceInputCb;
        impl->openHandlers["vplex.vsDirectory.AddDatasetArchiveStorageDeviceOutput"] = processOpenAddDatasetArchiveStorageDeviceOutputCb;
        impl->closeHandlers["vplex.vsDirectory.AddDatasetArchiveStorageDeviceOutput"] = processCloseAddDatasetArchiveStorageDeviceOutputCb;
        impl->openHandlers["vplex.vsDirectory.RemoveDatasetArchiveStorageDeviceInput"] = processOpenRemoveDatasetArchiveStorageDeviceInputCb;
        impl->closeHandlers["vplex.vsDirectory.RemoveDatasetArchiveStorageDeviceInput"] = processCloseRemoveDatasetArchiveStorageDeviceInputCb;
        impl->openHandlers["vplex.vsDirectory.RemoveDatasetArchiveStorageDeviceOutput"] = processOpenRemoveDatasetArchiveStorageDeviceOutputCb;
        impl->closeHandlers["vplex.vsDirectory.RemoveDatasetArchiveStorageDeviceOutput"] = processCloseRemoveDatasetArchiveStorageDeviceOutputCb;
        vplex::common::RegisterHandlers_vplex_common_types(impl);
    }
}

vplex::vsDirectory::DatasetType
vplex::vsDirectory::parseDatasetType(const std::string& data, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DatasetType out;
    if (data.compare("USER") == 0) {
        out = vplex::vsDirectory::USER;
    } else if (data.compare("CAMERA") == 0) {
        out = vplex::vsDirectory::CAMERA;
    } else if (data.compare("CLEAR_FI") == 0) {
        out = vplex::vsDirectory::CLEAR_FI;
    } else if (data.compare("CR_UP") == 0) {
        out = vplex::vsDirectory::CR_UP;
    } else if (data.compare("CR_DOWN") == 0) {
        out = vplex::vsDirectory::CR_DOWN;
    } else if (data.compare("PIM") == 0) {
        out = vplex::vsDirectory::PIM;
    } else if (data.compare("CACHE") == 0) {
        out = vplex::vsDirectory::CACHE;
    } else if (data.compare("PIM_CONTACTS") == 0) {
        out = vplex::vsDirectory::PIM_CONTACTS;
    } else if (data.compare("PIM_EVENTS") == 0) {
        out = vplex::vsDirectory::PIM_EVENTS;
    } else if (data.compare("PIM_NOTES") == 0) {
        out = vplex::vsDirectory::PIM_NOTES;
    } else if (data.compare("PIM_TASKS") == 0) {
        out = vplex::vsDirectory::PIM_TASKS;
    } else if (data.compare("PIM_FAVORITES") == 0) {
        out = vplex::vsDirectory::PIM_FAVORITES;
    } else if (data.compare("MEDIA") == 0) {
        out = vplex::vsDirectory::MEDIA;
    } else if (data.compare("MEDIA_METADATA") == 0) {
        out = vplex::vsDirectory::MEDIA_METADATA;
    } else if (data.compare("FS") == 0) {
        out = vplex::vsDirectory::FS;
    } else if (data.compare("VIRT_DRIVE") == 0) {
        out = vplex::vsDirectory::VIRT_DRIVE;
    } else if (data.compare("CLEARFI_MEDIA") == 0) {
        out = vplex::vsDirectory::CLEARFI_MEDIA;
    } else if (data.compare("USER_CONTENT_METADATA") == 0) {
        out = vplex::vsDirectory::USER_CONTENT_METADATA;
    } else if (data.compare("SYNCBOX") == 0) {
        out = vplex::vsDirectory::SYNCBOX;
    } else if (data.compare("SBM") == 0) {
        out = vplex::vsDirectory::SBM;
    } else if (data.compare("SWM") == 0) {
        out = vplex::vsDirectory::SWM;
    } else {
        state->error = "Invalid vplex.vsDirectory.DatasetType: ";
        state->error.append(data);
        out = vplex::vsDirectory::USER;
    }
    return out;
}

vplex::vsDirectory::RouteType
vplex::vsDirectory::parseRouteType(const std::string& data, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RouteType out;
    if (data.compare("INVALID_ROUTE") == 0) {
        out = vplex::vsDirectory::INVALID_ROUTE;
    } else if (data.compare("DIRECT_INTERNAL") == 0) {
        out = vplex::vsDirectory::DIRECT_INTERNAL;
    } else if (data.compare("DIRECT_EXTERNAL") == 0) {
        out = vplex::vsDirectory::DIRECT_EXTERNAL;
    } else if (data.compare("PROXY") == 0) {
        out = vplex::vsDirectory::PROXY;
    } else {
        state->error = "Invalid vplex.vsDirectory.RouteType: ";
        state->error.append(data);
        out = vplex::vsDirectory::INVALID_ROUTE;
    }
    return out;
}

vplex::vsDirectory::ProtocolType
vplex::vsDirectory::parseProtocolType(const std::string& data, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ProtocolType out;
    if (data.compare("INVALID_PROTOCOL") == 0) {
        out = vplex::vsDirectory::INVALID_PROTOCOL;
    } else if (data.compare("VS") == 0) {
        out = vplex::vsDirectory::VS;
    } else {
        state->error = "Invalid vplex.vsDirectory.ProtocolType: ";
        state->error.append(data);
        out = vplex::vsDirectory::INVALID_PROTOCOL;
    }
    return out;
}

vplex::vsDirectory::PortType
vplex::vsDirectory::parsePortType(const std::string& data, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::PortType out;
    if (data.compare("INVALID_PORT") == 0) {
        out = vplex::vsDirectory::INVALID_PORT;
    } else if (data.compare("PORT_VSSI") == 0) {
        out = vplex::vsDirectory::PORT_VSSI;
    } else if (data.compare("PORT_HTTP") == 0) {
        out = vplex::vsDirectory::PORT_HTTP;
    } else if (data.compare("PORT_CLEARFI") == 0) {
        out = vplex::vsDirectory::PORT_CLEARFI;
    } else if (data.compare("PORT_CLEARFI_SECURE") == 0) {
        out = vplex::vsDirectory::PORT_CLEARFI_SECURE;
    } else {
        state->error = "Invalid vplex.vsDirectory.PortType: ";
        state->error.append(data);
        out = vplex::vsDirectory::INVALID_PORT;
    }
    return out;
}

vplex::vsDirectory::SubscriptionRole
vplex::vsDirectory::parseSubscriptionRole(const std::string& data, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SubscriptionRole out;
    if (data.compare("GENERAL") == 0) {
        out = vplex::vsDirectory::GENERAL;
    } else if (data.compare("PRODUCER") == 0) {
        out = vplex::vsDirectory::PRODUCER;
    } else if (data.compare("CONSUMER") == 0) {
        out = vplex::vsDirectory::CONSUMER;
    } else if (data.compare("CLEARFI_SERVER") == 0) {
        out = vplex::vsDirectory::CLEARFI_SERVER;
    } else if (data.compare("CLEARFI_CLIENT") == 0) {
        out = vplex::vsDirectory::CLEARFI_CLIENT;
    } else if (data.compare("WRITER") == 0) {
        out = vplex::vsDirectory::WRITER;
    } else if (data.compare("READER") == 0) {
        out = vplex::vsDirectory::READER;
    } else {
        state->error = "Invalid vplex.vsDirectory.SubscriptionRole: ";
        state->error.append(data);
        out = vplex::vsDirectory::GENERAL;
    }
    return out;
}

void
processOpenAPIVersion(vplex::vsDirectory::APIVersion* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenAPIVersionCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::APIVersion* msg = static_cast<vplex::vsDirectory::APIVersion*>(state->protoStack.top());
    processOpenAPIVersion(msg, tag, state);
}

void
processCloseAPIVersion(vplex::vsDirectory::APIVersion* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAPIVersionCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::APIVersion* msg = static_cast<vplex::vsDirectory::APIVersion*>(state->protoStack.top());
    processCloseAPIVersion(msg, tag, state);
}

void
processOpenError(vplex::vsDirectory::Error* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenErrorCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::Error* msg = static_cast<vplex::vsDirectory::Error*>(state->protoStack.top());
    processOpenError(msg, tag, state);
}

void
processCloseError(vplex::vsDirectory::Error* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "errorCode") == 0) {
        if (msg->has_errorcode()) {
            state->error = "Duplicate of non-repeated field errorCode";
        } else {
            msg->set_errorcode(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "errorDetail") == 0) {
        if (msg->has_errordetail()) {
            state->error = "Duplicate of non-repeated field errorDetail";
        } else {
            msg->set_errordetail(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseErrorCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::Error* msg = static_cast<vplex::vsDirectory::Error*>(state->protoStack.top());
    processCloseError(msg, tag, state);
}

void
processOpenSessionInfo(vplex::vsDirectory::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SessionInfo* msg = static_cast<vplex::vsDirectory::SessionInfo*>(state->protoStack.top());
    processOpenSessionInfo(msg, tag, state);
}

void
processCloseSessionInfo(vplex::vsDirectory::SessionInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "sessionHandle") == 0) {
        if (msg->has_sessionhandle()) {
            state->error = "Duplicate of non-repeated field sessionHandle";
        } else {
            msg->set_sessionhandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "serviceTicket") == 0) {
        if (msg->has_serviceticket()) {
            state->error = "Duplicate of non-repeated field serviceTicket";
        } else {
            msg->set_serviceticket(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseSessionInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SessionInfo* msg = static_cast<vplex::vsDirectory::SessionInfo*>(state->protoStack.top());
    processCloseSessionInfo(msg, tag, state);
}

void
processOpenETicketData(vplex::vsDirectory::ETicketData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenETicketDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ETicketData* msg = static_cast<vplex::vsDirectory::ETicketData*>(state->protoStack.top());
    processOpenETicketData(msg, tag, state);
}

void
processCloseETicketData(vplex::vsDirectory::ETicketData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "eTicket") == 0) {
        if (msg->has_eticket()) {
            state->error = "Duplicate of non-repeated field eTicket";
        } else {
            msg->set_eticket(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "certificate") == 0) {
        msg->add_certificate(parseBytes(state->lastData, state));
    } else {
        (void)msg;
    }
}
void
processCloseETicketDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ETicketData* msg = static_cast<vplex::vsDirectory::ETicketData*>(state->protoStack.top());
    processCloseETicketData(msg, tag, state);
}

void
processOpenLocalization(vplex::vsDirectory::Localization* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenLocalizationCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::Localization* msg = static_cast<vplex::vsDirectory::Localization*>(state->protoStack.top());
    processOpenLocalization(msg, tag, state);
}

void
processCloseLocalization(vplex::vsDirectory::Localization* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "language") == 0) {
        if (msg->has_language()) {
            state->error = "Duplicate of non-repeated field language";
        } else {
            msg->set_language(state->lastData);
        }
    } else if (strcmp(tag, "country") == 0) {
        if (msg->has_country()) {
            state->error = "Duplicate of non-repeated field country";
        } else {
            msg->set_country(state->lastData);
        }
    } else if (strcmp(tag, "region") == 0) {
        if (msg->has_region()) {
            state->error = "Duplicate of non-repeated field region";
        } else {
            msg->set_region(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseLocalizationCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::Localization* msg = static_cast<vplex::vsDirectory::Localization*>(state->protoStack.top());
    processCloseLocalization(msg, tag, state);
}

void
processOpenTitleData(vplex::vsDirectory::TitleData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenTitleDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::TitleData* msg = static_cast<vplex::vsDirectory::TitleData*>(state->protoStack.top());
    processOpenTitleData(msg, tag, state);
}

void
processCloseTitleData(vplex::vsDirectory::TitleData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "titleId") == 0) {
        if (msg->has_titleid()) {
            state->error = "Duplicate of non-repeated field titleId";
        } else {
            msg->set_titleid(state->lastData);
        }
    } else if (strcmp(tag, "detailHash") == 0) {
        if (msg->has_detailhash()) {
            state->error = "Duplicate of non-repeated field detailHash";
        } else {
            msg->set_detailhash(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "ticketVersion") == 0) {
        if (msg->has_ticketversion()) {
            state->error = "Duplicate of non-repeated field ticketVersion";
        } else {
            msg->set_ticketversion(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "useOnlineETicket") == 0) {
        if (msg->has_useonlineeticket()) {
            state->error = "Duplicate of non-repeated field useOnlineETicket";
        } else {
            msg->set_useonlineeticket(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "useOfflineETicket") == 0) {
        if (msg->has_useofflineeticket()) {
            state->error = "Duplicate of non-repeated field useOfflineETicket";
        } else {
            msg->set_useofflineeticket(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseTitleDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::TitleData* msg = static_cast<vplex::vsDirectory::TitleData*>(state->protoStack.top());
    processCloseTitleData(msg, tag, state);
}

void
processOpenTitleDetail(vplex::vsDirectory::TitleDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "contents") == 0) {
        vplex::vsDirectory::ContentDetail* proto = msg->add_contents();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else if (strcmp(tag, "contentRating") == 0) {
        if (msg->has_contentrating()) {
            state->error = "Duplicate of non-repeated field contentRating";
        } else {
            vplex::common::ContentRating* proto = msg->mutable_contentrating();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "altContentRating") == 0) {
        if (msg->has_altcontentrating()) {
            state->error = "Duplicate of non-repeated field altContentRating";
        } else {
            vplex::common::ContentRating* proto = msg->mutable_altcontentrating();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenTitleDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::TitleDetail* msg = static_cast<vplex::vsDirectory::TitleDetail*>(state->protoStack.top());
    processOpenTitleDetail(msg, tag, state);
}

void
processCloseTitleDetail(vplex::vsDirectory::TitleDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "titleId") == 0) {
        if (msg->has_titleid()) {
            state->error = "Duplicate of non-repeated field titleId";
        } else {
            msg->set_titleid(state->lastData);
        }
    } else if (strcmp(tag, "titleVersion") == 0) {
        if (msg->has_titleversion()) {
            state->error = "Duplicate of non-repeated field titleVersion";
        } else {
            msg->set_titleversion(state->lastData);
        }
    } else if (strcmp(tag, "tmdUrl") == 0) {
        if (msg->has_tmdurl()) {
            state->error = "Duplicate of non-repeated field tmdUrl";
        } else {
            msg->set_tmdurl(state->lastData);
        }
    } else if (strcmp(tag, "name") == 0) {
        if (msg->has_name()) {
            state->error = "Duplicate of non-repeated field name";
        } else {
            msg->set_name(state->lastData);
        }
    } else if (strcmp(tag, "iconUrl") == 0) {
        if (msg->has_iconurl()) {
            state->error = "Duplicate of non-repeated field iconUrl";
        } else {
            msg->set_iconurl(state->lastData);
        }
    } else if (strcmp(tag, "imageUrl") == 0) {
        if (msg->has_imageurl()) {
            state->error = "Duplicate of non-repeated field imageUrl";
        } else {
            msg->set_imageurl(state->lastData);
        }
    } else if (strcmp(tag, "publisher") == 0) {
        if (msg->has_publisher()) {
            state->error = "Duplicate of non-repeated field publisher";
        } else {
            msg->set_publisher(state->lastData);
        }
    } else if (strcmp(tag, "genre") == 0) {
        if (msg->has_genre()) {
            state->error = "Duplicate of non-repeated field genre";
        } else {
            msg->set_genre(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseTitleDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::TitleDetail* msg = static_cast<vplex::vsDirectory::TitleDetail*>(state->protoStack.top());
    processCloseTitleDetail(msg, tag, state);
}

void
processOpenContentDetail(vplex::vsDirectory::ContentDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenContentDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ContentDetail* msg = static_cast<vplex::vsDirectory::ContentDetail*>(state->protoStack.top());
    processOpenContentDetail(msg, tag, state);
}

void
processCloseContentDetail(vplex::vsDirectory::ContentDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "contentId") == 0) {
        if (msg->has_contentid()) {
            state->error = "Duplicate of non-repeated field contentId";
        } else {
            msg->set_contentid(state->lastData);
        }
    } else if (strcmp(tag, "contentLocation") == 0) {
        if (msg->has_contentlocation()) {
            state->error = "Duplicate of non-repeated field contentLocation";
        } else {
            msg->set_contentlocation(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseContentDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ContentDetail* msg = static_cast<vplex::vsDirectory::ContentDetail*>(state->protoStack.top());
    processCloseContentDetail(msg, tag, state);
}

void
processOpenSaveData(vplex::vsDirectory::SaveData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenSaveDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SaveData* msg = static_cast<vplex::vsDirectory::SaveData*>(state->protoStack.top());
    processOpenSaveData(msg, tag, state);
}

void
processCloseSaveData(vplex::vsDirectory::SaveData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "titleId") == 0) {
        if (msg->has_titleid()) {
            state->error = "Duplicate of non-repeated field titleId";
        } else {
            msg->set_titleid(state->lastData);
        }
    } else if (strcmp(tag, "saveLocation") == 0) {
        if (msg->has_savelocation()) {
            state->error = "Duplicate of non-repeated field saveLocation";
        } else {
            msg->set_savelocation(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseSaveDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SaveData* msg = static_cast<vplex::vsDirectory::SaveData*>(state->protoStack.top());
    processCloseSaveData(msg, tag, state);
}

void
processOpenTitleTicket(vplex::vsDirectory::TitleTicket* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "eTicket") == 0) {
        if (msg->has_eticket()) {
            state->error = "Duplicate of non-repeated field eTicket";
        } else {
            vplex::vsDirectory::ETicketData* proto = msg->mutable_eticket();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenTitleTicketCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::TitleTicket* msg = static_cast<vplex::vsDirectory::TitleTicket*>(state->protoStack.top());
    processOpenTitleTicket(msg, tag, state);
}

void
processCloseTitleTicket(vplex::vsDirectory::TitleTicket* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "titleId") == 0) {
        if (msg->has_titleid()) {
            state->error = "Duplicate of non-repeated field titleId";
        } else {
            msg->set_titleid(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseTitleTicketCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::TitleTicket* msg = static_cast<vplex::vsDirectory::TitleTicket*>(state->protoStack.top());
    processCloseTitleTicket(msg, tag, state);
}

void
processOpenSubscription(vplex::vsDirectory::Subscription* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenSubscriptionCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::Subscription* msg = static_cast<vplex::vsDirectory::Subscription*>(state->protoStack.top());
    processOpenSubscription(msg, tag, state);
}

void
processCloseSubscription(vplex::vsDirectory::Subscription* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "filter") == 0) {
        if (msg->has_filter()) {
            state->error = "Duplicate of non-repeated field filter";
        } else {
            msg->set_filter(state->lastData);
        }
    } else if (strcmp(tag, "deviceRoot") == 0) {
        if (msg->has_deviceroot()) {
            state->error = "Duplicate of non-repeated field deviceRoot";
        } else {
            msg->set_deviceroot(state->lastData);
        }
    } else if (strcmp(tag, "datasetRoot") == 0) {
        if (msg->has_datasetroot()) {
            state->error = "Duplicate of non-repeated field datasetRoot";
        } else {
            msg->set_datasetroot(state->lastData);
        }
    } else if (strcmp(tag, "uploadOk") == 0) {
        if (msg->has_uploadok()) {
            state->error = "Duplicate of non-repeated field uploadOk";
        } else {
            msg->set_uploadok(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "downloadOk") == 0) {
        if (msg->has_downloadok()) {
            state->error = "Duplicate of non-repeated field downloadOk";
        } else {
            msg->set_downloadok(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "uploadDeleteOk") == 0) {
        if (msg->has_uploaddeleteok()) {
            state->error = "Duplicate of non-repeated field uploadDeleteOk";
        } else {
            msg->set_uploaddeleteok(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "downloadDeleteOk") == 0) {
        if (msg->has_downloaddeleteok()) {
            state->error = "Duplicate of non-repeated field downloadDeleteOk";
        } else {
            msg->set_downloaddeleteok(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetLocation") == 0) {
        if (msg->has_datasetlocation()) {
            state->error = "Duplicate of non-repeated field datasetLocation";
        } else {
            msg->set_datasetlocation(state->lastData);
        }
    } else if (strcmp(tag, "contentType") == 0) {
        if (msg->has_contenttype()) {
            state->error = "Duplicate of non-repeated field contentType";
        } else {
            msg->set_contenttype(state->lastData);
        }
    } else if (strcmp(tag, "createdFor") == 0) {
        if (msg->has_createdfor()) {
            state->error = "Duplicate of non-repeated field createdFor";
        } else {
            msg->set_createdfor(state->lastData);
        }
    } else if (strcmp(tag, "maxSize") == 0) {
        if (msg->has_maxsize()) {
            state->error = "Duplicate of non-repeated field maxSize";
        } else {
            msg->set_maxsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "maxFiles") == 0) {
        if (msg->has_maxfiles()) {
            state->error = "Duplicate of non-repeated field maxFiles";
        } else {
            msg->set_maxfiles((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "creationTime") == 0) {
        if (msg->has_creationtime()) {
            state->error = "Duplicate of non-repeated field creationTime";
        } else {
            msg->set_creationtime((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseSubscriptionCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::Subscription* msg = static_cast<vplex::vsDirectory::Subscription*>(state->protoStack.top());
    processCloseSubscription(msg, tag, state);
}

void
processOpenSyncDirectory(vplex::vsDirectory::SyncDirectory* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenSyncDirectoryCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SyncDirectory* msg = static_cast<vplex::vsDirectory::SyncDirectory*>(state->protoStack.top());
    processOpenSyncDirectory(msg, tag, state);
}

void
processCloseSyncDirectory(vplex::vsDirectory::SyncDirectory* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "localPath") == 0) {
        if (msg->has_localpath()) {
            state->error = "Duplicate of non-repeated field localPath";
        } else {
            msg->set_localpath(state->lastData);
        }
    } else if (strcmp(tag, "serverPath") == 0) {
        if (msg->has_serverpath()) {
            state->error = "Duplicate of non-repeated field serverPath";
        } else {
            msg->set_serverpath(state->lastData);
        }
    } else if (strcmp(tag, "privateFlag") == 0) {
        if (msg->has_privateflag()) {
            state->error = "Duplicate of non-repeated field privateFlag";
        } else {
            msg->set_privateflag(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseSyncDirectoryCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SyncDirectory* msg = static_cast<vplex::vsDirectory::SyncDirectory*>(state->protoStack.top());
    processCloseSyncDirectory(msg, tag, state);
}

void
processOpenDatasetData(vplex::vsDirectory::DatasetData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenDatasetDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DatasetData* msg = static_cast<vplex::vsDirectory::DatasetData*>(state->protoStack.top());
    processOpenDatasetData(msg, tag, state);
}

void
processCloseDatasetData(vplex::vsDirectory::DatasetData* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "detailHash") == 0) {
        if (msg->has_detailhash()) {
            state->error = "Duplicate of non-repeated field detailHash";
        } else {
            msg->set_detailhash(parseInt32(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseDatasetDataCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DatasetData* msg = static_cast<vplex::vsDirectory::DatasetData*>(state->protoStack.top());
    processCloseDatasetData(msg, tag, state);
}

void
processOpenDatasetDetail(vplex::vsDirectory::DatasetDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenDatasetDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DatasetDetail* msg = static_cast<vplex::vsDirectory::DatasetDetail*>(state->protoStack.top());
    processOpenDatasetDetail(msg, tag, state);
}

void
processCloseDatasetDetail(vplex::vsDirectory::DatasetDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "contentType") == 0) {
        if (msg->has_contenttype()) {
            state->error = "Duplicate of non-repeated field contentType";
        } else {
            msg->set_contenttype(state->lastData);
        }
    } else if (strcmp(tag, "createdFor") == 0) {
        if (msg->has_createdfor()) {
            state->error = "Duplicate of non-repeated field createdFor";
        } else {
            msg->set_createdfor(state->lastData);
        }
    } else if (strcmp(tag, "externalId") == 0) {
        if (msg->has_externalid()) {
            state->error = "Duplicate of non-repeated field externalId";
        } else {
            msg->set_externalid(state->lastData);
        }
    } else if (strcmp(tag, "lastUpdated") == 0) {
        if (msg->has_lastupdated()) {
            state->error = "Duplicate of non-repeated field lastUpdated";
        } else {
            msg->set_lastupdated((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterName") == 0) {
        if (msg->has_storageclustername()) {
            state->error = "Duplicate of non-repeated field storageClusterName";
        } else {
            msg->set_storageclustername(state->lastData);
        }
    } else if (strcmp(tag, "storageClusterHostName") == 0) {
        if (msg->has_storageclusterhostname()) {
            state->error = "Duplicate of non-repeated field storageClusterHostName";
        } else {
            msg->set_storageclusterhostname(state->lastData);
        }
    } else if (strcmp(tag, "storageClusterPort") == 0) {
        if (msg->has_storageclusterport()) {
            state->error = "Duplicate of non-repeated field storageClusterPort";
        } else {
            msg->set_storageclusterport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetLocation") == 0) {
        if (msg->has_datasetlocation()) {
            state->error = "Duplicate of non-repeated field datasetLocation";
        } else {
            msg->set_datasetlocation(state->lastData);
        }
    } else if (strcmp(tag, "sizeOnDisk") == 0) {
        if (msg->has_sizeondisk()) {
            state->error = "Duplicate of non-repeated field sizeOnDisk";
        } else {
            msg->set_sizeondisk((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetType") == 0) {
        if (msg->has_datasettype()) {
            state->error = "Duplicate of non-repeated field datasetType";
        } else {
            msg->set_datasettype(vplex::vsDirectory::parseDatasetType(state->lastData, state));
        }
    } else if (strcmp(tag, "linkedTo") == 0) {
        if (msg->has_linkedto()) {
            state->error = "Duplicate of non-repeated field linkedTo";
        } else {
            msg->set_linkedto((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "suspendedFlag") == 0) {
        if (msg->has_suspendedflag()) {
            state->error = "Duplicate of non-repeated field suspendedFlag";
        } else {
            msg->set_suspendedflag(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryStorageId") == 0) {
        if (msg->has_primarystorageid()) {
            state->error = "Duplicate of non-repeated field primaryStorageId";
        } else {
            msg->set_primarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deleteDataAfter") == 0) {
        if (msg->has_deletedataafter()) {
            state->error = "Duplicate of non-repeated field deleteDataAfter";
        } else {
            msg->set_deletedataafter((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "archiveStorageDeviceId") == 0) {
        msg->add_archivestoragedeviceid((u64)parseInt64(state->lastData, state));
    } else if (strcmp(tag, "displayName") == 0) {
        if (msg->has_displayname()) {
            state->error = "Duplicate of non-repeated field displayName";
        } else {
            msg->set_displayname(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseDatasetDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DatasetDetail* msg = static_cast<vplex::vsDirectory::DatasetDetail*>(state->protoStack.top());
    processCloseDatasetDetail(msg, tag, state);
}

void
processOpenStoredDataset(vplex::vsDirectory::StoredDataset* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenStoredDatasetCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StoredDataset* msg = static_cast<vplex::vsDirectory::StoredDataset*>(state->protoStack.top());
    processOpenStoredDataset(msg, tag, state);
}

void
processCloseStoredDataset(vplex::vsDirectory::StoredDataset* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetType") == 0) {
        if (msg->has_datasettype()) {
            state->error = "Duplicate of non-repeated field datasetType";
        } else {
            msg->set_datasettype(vplex::vsDirectory::parseDatasetType(state->lastData, state));
        }
    } else if (strcmp(tag, "dataRetentionTime") == 0) {
        if (msg->has_dataretentiontime()) {
            state->error = "Duplicate of non-repeated field dataRetentionTime";
        } else {
            msg->set_dataretentiontime((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryStorageId") == 0) {
        if (msg->has_primarystorageid()) {
            state->error = "Duplicate of non-repeated field primaryStorageId";
        } else {
            msg->set_primarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryStorageId") == 0) {
        if (msg->has_secondarystorageid()) {
            state->error = "Duplicate of non-repeated field secondaryStorageId";
        } else {
            msg->set_secondarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseStoredDatasetCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StoredDataset* msg = static_cast<vplex::vsDirectory::StoredDataset*>(state->protoStack.top());
    processCloseStoredDataset(msg, tag, state);
}

void
processOpenDeviceInfo(vplex::vsDirectory::DeviceInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenDeviceInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeviceInfo* msg = static_cast<vplex::vsDirectory::DeviceInfo*>(state->protoStack.top());
    processOpenDeviceInfo(msg, tag, state);
}

void
processCloseDeviceInfo(vplex::vsDirectory::DeviceInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceClass") == 0) {
        if (msg->has_deviceclass()) {
            state->error = "Duplicate of non-repeated field deviceClass";
        } else {
            msg->set_deviceclass(state->lastData);
        }
    } else if (strcmp(tag, "deviceName") == 0) {
        if (msg->has_devicename()) {
            state->error = "Duplicate of non-repeated field deviceName";
        } else {
            msg->set_devicename(state->lastData);
        }
    } else if (strcmp(tag, "isAcer") == 0) {
        if (msg->has_isacer()) {
            state->error = "Duplicate of non-repeated field isAcer";
        } else {
            msg->set_isacer(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "hasCamera") == 0) {
        if (msg->has_hascamera()) {
            state->error = "Duplicate of non-repeated field hasCamera";
        } else {
            msg->set_hascamera(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "osVersion") == 0) {
        if (msg->has_osversion()) {
            state->error = "Duplicate of non-repeated field osVersion";
        } else {
            msg->set_osversion(state->lastData);
        }
    } else if (strcmp(tag, "protocolVersion") == 0) {
        if (msg->has_protocolversion()) {
            state->error = "Duplicate of non-repeated field protocolVersion";
        } else {
            msg->set_protocolversion(state->lastData);
        }
    } else if (strcmp(tag, "isVirtDrive") == 0) {
        if (msg->has_isvirtdrive()) {
            state->error = "Duplicate of non-repeated field isVirtDrive";
        } else {
            msg->set_isvirtdrive(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "isMediaServer") == 0) {
        if (msg->has_ismediaserver()) {
            state->error = "Duplicate of non-repeated field isMediaServer";
        } else {
            msg->set_ismediaserver(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMediaServerCapable") == 0) {
        if (msg->has_featuremediaservercapable()) {
            state->error = "Duplicate of non-repeated field featureMediaServerCapable";
        } else {
            msg->set_featuremediaservercapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureVirtDriveCapable") == 0) {
        if (msg->has_featurevirtdrivecapable()) {
            state->error = "Duplicate of non-repeated field featureVirtDriveCapable";
        } else {
            msg->set_featurevirtdrivecapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureRemoteFileAccessCapable") == 0) {
        if (msg->has_featureremotefileaccesscapable()) {
            state->error = "Duplicate of non-repeated field featureRemoteFileAccessCapable";
        } else {
            msg->set_featureremotefileaccesscapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureFSDatasetTypeCapable") == 0) {
        if (msg->has_featurefsdatasettypecapable()) {
            state->error = "Duplicate of non-repeated field featureFSDatasetTypeCapable";
        } else {
            msg->set_featurefsdatasettypecapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "modelNumber") == 0) {
        if (msg->has_modelnumber()) {
            state->error = "Duplicate of non-repeated field modelNumber";
        } else {
            msg->set_modelnumber(state->lastData);
        }
    } else if (strcmp(tag, "buildInfo") == 0) {
        if (msg->has_buildinfo()) {
            state->error = "Duplicate of non-repeated field buildInfo";
        } else {
            msg->set_buildinfo(state->lastData);
        }
    } else if (strcmp(tag, "featureVirtSyncCapable") == 0) {
        if (msg->has_featurevirtsynccapable()) {
            state->error = "Duplicate of non-repeated field featureVirtSyncCapable";
        } else {
            msg->set_featurevirtsynccapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMyStorageServerCapable") == 0) {
        if (msg->has_featuremystorageservercapable()) {
            state->error = "Duplicate of non-repeated field featureMyStorageServerCapable";
        } else {
            msg->set_featuremystorageservercapable(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseDeviceInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeviceInfo* msg = static_cast<vplex::vsDirectory::DeviceInfo*>(state->protoStack.top());
    processCloseDeviceInfo(msg, tag, state);
}

void
processOpenStorageAccessPort(vplex::vsDirectory::StorageAccessPort* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenStorageAccessPortCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StorageAccessPort* msg = static_cast<vplex::vsDirectory::StorageAccessPort*>(state->protoStack.top());
    processOpenStorageAccessPort(msg, tag, state);
}

void
processCloseStorageAccessPort(vplex::vsDirectory::StorageAccessPort* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "portType") == 0) {
        if (msg->has_porttype()) {
            state->error = "Duplicate of non-repeated field portType";
        } else {
            msg->set_porttype(vplex::vsDirectory::parsePortType(state->lastData, state));
        }
    } else if (strcmp(tag, "port") == 0) {
        if (msg->has_port()) {
            state->error = "Duplicate of non-repeated field port";
        } else {
            msg->set_port(parseInt32(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseStorageAccessPortCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StorageAccessPort* msg = static_cast<vplex::vsDirectory::StorageAccessPort*>(state->protoStack.top());
    processCloseStorageAccessPort(msg, tag, state);
}

void
processOpenStorageAccess(vplex::vsDirectory::StorageAccess* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "ports") == 0) {
        vplex::vsDirectory::StorageAccessPort* proto = msg->add_ports();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenStorageAccessCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StorageAccess* msg = static_cast<vplex::vsDirectory::StorageAccess*>(state->protoStack.top());
    processOpenStorageAccess(msg, tag, state);
}

void
processCloseStorageAccess(vplex::vsDirectory::StorageAccess* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "routeType") == 0) {
        if (msg->has_routetype()) {
            state->error = "Duplicate of non-repeated field routeType";
        } else {
            msg->set_routetype(vplex::vsDirectory::parseRouteType(state->lastData, state));
        }
    } else if (strcmp(tag, "protocol") == 0) {
        if (msg->has_protocol()) {
            state->error = "Duplicate of non-repeated field protocol";
        } else {
            msg->set_protocol(vplex::vsDirectory::parseProtocolType(state->lastData, state));
        }
    } else if (strcmp(tag, "server") == 0) {
        if (msg->has_server()) {
            state->error = "Duplicate of non-repeated field server";
        } else {
            msg->set_server(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseStorageAccessCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StorageAccess* msg = static_cast<vplex::vsDirectory::StorageAccess*>(state->protoStack.top());
    processCloseStorageAccess(msg, tag, state);
}

void
processOpenDeviceAccessTicket(vplex::vsDirectory::DeviceAccessTicket* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenDeviceAccessTicketCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeviceAccessTicket* msg = static_cast<vplex::vsDirectory::DeviceAccessTicket*>(state->protoStack.top());
    processOpenDeviceAccessTicket(msg, tag, state);
}

void
processCloseDeviceAccessTicket(vplex::vsDirectory::DeviceAccessTicket* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "accessTicket") == 0) {
        if (msg->has_accessticket()) {
            state->error = "Duplicate of non-repeated field accessTicket";
        } else {
            msg->set_accessticket(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseDeviceAccessTicketCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeviceAccessTicket* msg = static_cast<vplex::vsDirectory::DeviceAccessTicket*>(state->protoStack.top());
    processCloseDeviceAccessTicket(msg, tag, state);
}

void
processOpenUserStorage(vplex::vsDirectory::UserStorage* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "storageAccess") == 0) {
        vplex::vsDirectory::StorageAccess* proto = msg->add_storageaccess();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenUserStorageCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UserStorage* msg = static_cast<vplex::vsDirectory::UserStorage*>(state->protoStack.top());
    processOpenUserStorage(msg, tag, state);
}

void
processCloseUserStorage(vplex::vsDirectory::UserStorage* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageName") == 0) {
        if (msg->has_storagename()) {
            state->error = "Duplicate of non-repeated field storageName";
        } else {
            msg->set_storagename(state->lastData);
        }
    } else if (strcmp(tag, "storageType") == 0) {
        if (msg->has_storagetype()) {
            state->error = "Duplicate of non-repeated field storageType";
        } else {
            msg->set_storagetype(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "usageLimit") == 0) {
        if (msg->has_usagelimit()) {
            state->error = "Duplicate of non-repeated field usageLimit";
        } else {
            msg->set_usagelimit((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "isVirtDrive") == 0) {
        if (msg->has_isvirtdrive()) {
            state->error = "Duplicate of non-repeated field isVirtDrive";
        } else {
            msg->set_isvirtdrive(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "isMediaServer") == 0) {
        if (msg->has_ismediaserver()) {
            state->error = "Duplicate of non-repeated field isMediaServer";
        } else {
            msg->set_ismediaserver(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "accessHandle") == 0) {
        if (msg->has_accesshandle()) {
            state->error = "Duplicate of non-repeated field accessHandle";
        } else {
            msg->set_accesshandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "accessTicket") == 0) {
        if (msg->has_accessticket()) {
            state->error = "Duplicate of non-repeated field accessTicket";
        } else {
            msg->set_accessticket(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMediaServerEnabled") == 0) {
        if (msg->has_featuremediaserverenabled()) {
            state->error = "Duplicate of non-repeated field featureMediaServerEnabled";
        } else {
            msg->set_featuremediaserverenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureVirtDriveEnabled") == 0) {
        if (msg->has_featurevirtdriveenabled()) {
            state->error = "Duplicate of non-repeated field featureVirtDriveEnabled";
        } else {
            msg->set_featurevirtdriveenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureRemoteFileAccessEnabled") == 0) {
        if (msg->has_featureremotefileaccessenabled()) {
            state->error = "Duplicate of non-repeated field featureRemoteFileAccessEnabled";
        } else {
            msg->set_featureremotefileaccessenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureFSDatasetTypeEnabled") == 0) {
        if (msg->has_featurefsdatasettypeenabled()) {
            state->error = "Duplicate of non-repeated field featureFSDatasetTypeEnabled";
        } else {
            msg->set_featurefsdatasettypeenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "devSpecAccessTicket") == 0) {
        if (msg->has_devspecaccessticket()) {
            state->error = "Duplicate of non-repeated field devSpecAccessTicket";
        } else {
            msg->set_devspecaccessticket(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "featureCloudDocEnabled") == 0) {
        if (msg->has_featureclouddocenabled()) {
            state->error = "Duplicate of non-repeated field featureCloudDocEnabled";
        } else {
            msg->set_featureclouddocenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureVirtSyncEnabled") == 0) {
        if (msg->has_featurevirtsyncenabled()) {
            state->error = "Duplicate of non-repeated field featureVirtSyncEnabled";
        } else {
            msg->set_featurevirtsyncenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMyStorageServerEnabled") == 0) {
        if (msg->has_featuremystorageserverenabled()) {
            state->error = "Duplicate of non-repeated field featureMyStorageServerEnabled";
        } else {
            msg->set_featuremystorageserverenabled(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseUserStorageCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UserStorage* msg = static_cast<vplex::vsDirectory::UserStorage*>(state->protoStack.top());
    processCloseUserStorage(msg, tag, state);
}

void
processOpenUpdatedDataset(vplex::vsDirectory::UpdatedDataset* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenUpdatedDatasetCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdatedDataset* msg = static_cast<vplex::vsDirectory::UpdatedDataset*>(state->protoStack.top());
    processOpenUpdatedDataset(msg, tag, state);
}

void
processCloseUpdatedDataset(vplex::vsDirectory::UpdatedDataset* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetType") == 0) {
        if (msg->has_datasettype()) {
            state->error = "Duplicate of non-repeated field datasetType";
        } else {
            msg->set_datasettype(vplex::vsDirectory::parseDatasetType(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "lastUpdated") == 0) {
        if (msg->has_lastupdated()) {
            state->error = "Duplicate of non-repeated field lastUpdated";
        } else {
            msg->set_lastupdated((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "destDatasetId") == 0) {
        if (msg->has_destdatasetid()) {
            state->error = "Duplicate of non-repeated field destDatasetId";
        } else {
            msg->set_destdatasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryVersion") == 0) {
        if (msg->has_primaryversion()) {
            state->error = "Duplicate of non-repeated field primaryVersion";
        } else {
            msg->set_primaryversion((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdatedDatasetCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdatedDataset* msg = static_cast<vplex::vsDirectory::UpdatedDataset*>(state->protoStack.top());
    processCloseUpdatedDataset(msg, tag, state);
}

void
processOpenDatasetFilter(vplex::vsDirectory::DatasetFilter* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenDatasetFilterCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DatasetFilter* msg = static_cast<vplex::vsDirectory::DatasetFilter*>(state->protoStack.top());
    processOpenDatasetFilter(msg, tag, state);
}

void
processCloseDatasetFilter(vplex::vsDirectory::DatasetFilter* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "name") == 0) {
        if (msg->has_name()) {
            state->error = "Duplicate of non-repeated field name";
        } else {
            msg->set_name(state->lastData);
        }
    } else if (strcmp(tag, "value") == 0) {
        if (msg->has_value()) {
            state->error = "Duplicate of non-repeated field value";
        } else {
            msg->set_value(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseDatasetFilterCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DatasetFilter* msg = static_cast<vplex::vsDirectory::DatasetFilter*>(state->protoStack.top());
    processCloseDatasetFilter(msg, tag, state);
}

void
processOpenMssDetail(vplex::vsDirectory::MssDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenMssDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::MssDetail* msg = static_cast<vplex::vsDirectory::MssDetail*>(state->protoStack.top());
    processOpenMssDetail(msg, tag, state);
}

void
processCloseMssDetail(vplex::vsDirectory::MssDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "mssId") == 0) {
        if (msg->has_mssid()) {
            state->error = "Duplicate of non-repeated field mssId";
        } else {
            msg->set_mssid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "mssName") == 0) {
        if (msg->has_mssname()) {
            state->error = "Duplicate of non-repeated field mssName";
        } else {
            msg->set_mssname(state->lastData);
        }
    } else if (strcmp(tag, "inactiveFlag") == 0) {
        if (msg->has_inactiveflag()) {
            state->error = "Duplicate of non-repeated field inactiveFlag";
        } else {
            msg->set_inactiveflag(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseMssDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::MssDetail* msg = static_cast<vplex::vsDirectory::MssDetail*>(state->protoStack.top());
    processCloseMssDetail(msg, tag, state);
}

void
processOpenStorageUnitDetail(vplex::vsDirectory::StorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StorageUnitDetail* msg = static_cast<vplex::vsDirectory::StorageUnitDetail*>(state->protoStack.top());
    processOpenStorageUnitDetail(msg, tag, state);
}

void
processCloseStorageUnitDetail(vplex::vsDirectory::StorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "storageId") == 0) {
        if (msg->has_storageid()) {
            state->error = "Duplicate of non-repeated field storageId";
        } else {
            msg->set_storageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "mssIds") == 0) {
        msg->add_mssids((u64)parseInt64(state->lastData, state));
    } else if (strcmp(tag, "inactiveFlag") == 0) {
        if (msg->has_inactiveflag()) {
            state->error = "Duplicate of non-repeated field inactiveFlag";
        } else {
            msg->set_inactiveflag(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StorageUnitDetail* msg = static_cast<vplex::vsDirectory::StorageUnitDetail*>(state->protoStack.top());
    processCloseStorageUnitDetail(msg, tag, state);
}

void
processOpenBrsDetail(vplex::vsDirectory::BrsDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenBrsDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::BrsDetail* msg = static_cast<vplex::vsDirectory::BrsDetail*>(state->protoStack.top());
    processOpenBrsDetail(msg, tag, state);
}

void
processCloseBrsDetail(vplex::vsDirectory::BrsDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "brsId") == 0) {
        if (msg->has_brsid()) {
            state->error = "Duplicate of non-repeated field brsId";
        } else {
            msg->set_brsid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "brsName") == 0) {
        if (msg->has_brsname()) {
            state->error = "Duplicate of non-repeated field brsName";
        } else {
            msg->set_brsname(state->lastData);
        }
    } else if (strcmp(tag, "inactiveFlag") == 0) {
        if (msg->has_inactiveflag()) {
            state->error = "Duplicate of non-repeated field inactiveFlag";
        } else {
            msg->set_inactiveflag(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseBrsDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::BrsDetail* msg = static_cast<vplex::vsDirectory::BrsDetail*>(state->protoStack.top());
    processCloseBrsDetail(msg, tag, state);
}

void
processOpenBrsStorageUnitDetail(vplex::vsDirectory::BrsStorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenBrsStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::BrsStorageUnitDetail* msg = static_cast<vplex::vsDirectory::BrsStorageUnitDetail*>(state->protoStack.top());
    processOpenBrsStorageUnitDetail(msg, tag, state);
}

void
processCloseBrsStorageUnitDetail(vplex::vsDirectory::BrsStorageUnitDetail* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "brsStorageId") == 0) {
        if (msg->has_brsstorageid()) {
            state->error = "Duplicate of non-repeated field brsStorageId";
        } else {
            msg->set_brsstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "brsId") == 0) {
        if (msg->has_brsid()) {
            state->error = "Duplicate of non-repeated field brsId";
        } else {
            msg->set_brsid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "inactiveFlag") == 0) {
        if (msg->has_inactiveflag()) {
            state->error = "Duplicate of non-repeated field inactiveFlag";
        } else {
            msg->set_inactiveflag(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseBrsStorageUnitDetailCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::BrsStorageUnitDetail* msg = static_cast<vplex::vsDirectory::BrsStorageUnitDetail*>(state->protoStack.top());
    processCloseBrsStorageUnitDetail(msg, tag, state);
}

void
processOpenBackupStatus(vplex::vsDirectory::BackupStatus* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenBackupStatusCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::BackupStatus* msg = static_cast<vplex::vsDirectory::BackupStatus*>(state->protoStack.top());
    processOpenBackupStatus(msg, tag, state);
}

void
processCloseBackupStatus(vplex::vsDirectory::BackupStatus* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "lastBackupTime") == 0) {
        if (msg->has_lastbackuptime()) {
            state->error = "Duplicate of non-repeated field lastBackupTime";
        } else {
            msg->set_lastbackuptime((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "lastBackupVersion") == 0) {
        if (msg->has_lastbackupversion()) {
            state->error = "Duplicate of non-repeated field lastBackupVersion";
        } else {
            msg->set_lastbackupversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "lastArchiveTime") == 0) {
        if (msg->has_lastarchivetime()) {
            state->error = "Duplicate of non-repeated field lastArchiveTime";
        } else {
            msg->set_lastarchivetime((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "lastArchiveVersion") == 0) {
        if (msg->has_lastarchiveversion()) {
            state->error = "Duplicate of non-repeated field lastArchiveVersion";
        } else {
            msg->set_lastarchiveversion((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseBackupStatusCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::BackupStatus* msg = static_cast<vplex::vsDirectory::BackupStatus*>(state->protoStack.top());
    processCloseBackupStatus(msg, tag, state);
}

void
processOpenGetSaveTicketsInput(vplex::vsDirectory::GetSaveTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetSaveTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveTicketsInput* msg = static_cast<vplex::vsDirectory::GetSaveTicketsInput*>(state->protoStack.top());
    processOpenGetSaveTicketsInput(msg, tag, state);
}

void
processCloseGetSaveTicketsInput(vplex::vsDirectory::GetSaveTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "certificate") == 0) {
        if (msg->has_certificate()) {
            state->error = "Duplicate of non-repeated field certificate";
        } else {
            msg->set_certificate(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetSaveTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveTicketsInput* msg = static_cast<vplex::vsDirectory::GetSaveTicketsInput*>(state->protoStack.top());
    processCloseGetSaveTicketsInput(msg, tag, state);
}

void
processOpenGetSaveTicketsOutput(vplex::vsDirectory::GetSaveTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "encryptionTicket") == 0) {
        if (msg->has_encryptionticket()) {
            state->error = "Duplicate of non-repeated field encryptionTicket";
        } else {
            vplex::vsDirectory::ETicketData* proto = msg->mutable_encryptionticket();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "signingTicket") == 0) {
        if (msg->has_signingticket()) {
            state->error = "Duplicate of non-repeated field signingTicket";
        } else {
            vplex::vsDirectory::ETicketData* proto = msg->mutable_signingticket();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetSaveTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveTicketsOutput* msg = static_cast<vplex::vsDirectory::GetSaveTicketsOutput*>(state->protoStack.top());
    processOpenGetSaveTicketsOutput(msg, tag, state);
}

void
processCloseGetSaveTicketsOutput(vplex::vsDirectory::GetSaveTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetSaveTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveTicketsOutput* msg = static_cast<vplex::vsDirectory::GetSaveTicketsOutput*>(state->protoStack.top());
    processCloseGetSaveTicketsOutput(msg, tag, state);
}

void
processOpenGetSaveDataInput(vplex::vsDirectory::GetSaveDataInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetSaveDataInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveDataInput* msg = static_cast<vplex::vsDirectory::GetSaveDataInput*>(state->protoStack.top());
    processOpenGetSaveDataInput(msg, tag, state);
}

void
processCloseGetSaveDataInput(vplex::vsDirectory::GetSaveDataInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "titleIds") == 0) {
        msg->add_titleids(state->lastData);
    } else {
        (void)msg;
    }
}
void
processCloseGetSaveDataInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveDataInput* msg = static_cast<vplex::vsDirectory::GetSaveDataInput*>(state->protoStack.top());
    processCloseGetSaveDataInput(msg, tag, state);
}

void
processOpenGetSaveDataOutput(vplex::vsDirectory::GetSaveDataOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "data") == 0) {
        vplex::vsDirectory::SaveData* proto = msg->add_data();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSaveDataOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveDataOutput* msg = static_cast<vplex::vsDirectory::GetSaveDataOutput*>(state->protoStack.top());
    processOpenGetSaveDataOutput(msg, tag, state);
}

void
processCloseGetSaveDataOutput(vplex::vsDirectory::GetSaveDataOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetSaveDataOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSaveDataOutput* msg = static_cast<vplex::vsDirectory::GetSaveDataOutput*>(state->protoStack.top());
    processCloseGetSaveDataOutput(msg, tag, state);
}

void
processOpenGetOwnedTitlesInput(vplex::vsDirectory::GetOwnedTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "l10n") == 0) {
        if (msg->has_l10n()) {
            state->error = "Duplicate of non-repeated field l10n";
        } else {
            vplex::vsDirectory::Localization* proto = msg->mutable_l10n();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetOwnedTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOwnedTitlesInput* msg = static_cast<vplex::vsDirectory::GetOwnedTitlesInput*>(state->protoStack.top());
    processOpenGetOwnedTitlesInput(msg, tag, state);
}

void
processCloseGetOwnedTitlesInput(vplex::vsDirectory::GetOwnedTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetOwnedTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOwnedTitlesInput* msg = static_cast<vplex::vsDirectory::GetOwnedTitlesInput*>(state->protoStack.top());
    processCloseGetOwnedTitlesInput(msg, tag, state);
}

void
processOpenGetOwnedTitlesOutput(vplex::vsDirectory::GetOwnedTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "titleData") == 0) {
        vplex::vsDirectory::TitleData* proto = msg->add_titledata();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetOwnedTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOwnedTitlesOutput* msg = static_cast<vplex::vsDirectory::GetOwnedTitlesOutput*>(state->protoStack.top());
    processOpenGetOwnedTitlesOutput(msg, tag, state);
}

void
processCloseGetOwnedTitlesOutput(vplex::vsDirectory::GetOwnedTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetOwnedTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOwnedTitlesOutput* msg = static_cast<vplex::vsDirectory::GetOwnedTitlesOutput*>(state->protoStack.top());
    processCloseGetOwnedTitlesOutput(msg, tag, state);
}

void
processOpenGetTitlesInput(vplex::vsDirectory::GetTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "l10n") == 0) {
        if (msg->has_l10n()) {
            state->error = "Duplicate of non-repeated field l10n";
        } else {
            vplex::vsDirectory::Localization* proto = msg->mutable_l10n();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitlesInput* msg = static_cast<vplex::vsDirectory::GetTitlesInput*>(state->protoStack.top());
    processOpenGetTitlesInput(msg, tag, state);
}

void
processCloseGetTitlesInput(vplex::vsDirectory::GetTitlesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "titleIds") == 0) {
        msg->add_titleids(state->lastData);
    } else {
        (void)msg;
    }
}
void
processCloseGetTitlesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitlesInput* msg = static_cast<vplex::vsDirectory::GetTitlesInput*>(state->protoStack.top());
    processCloseGetTitlesInput(msg, tag, state);
}

void
processOpenGetTitlesOutput(vplex::vsDirectory::GetTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "titleData") == 0) {
        vplex::vsDirectory::TitleData* proto = msg->add_titledata();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitlesOutput* msg = static_cast<vplex::vsDirectory::GetTitlesOutput*>(state->protoStack.top());
    processOpenGetTitlesOutput(msg, tag, state);
}

void
processCloseGetTitlesOutput(vplex::vsDirectory::GetTitlesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetTitlesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitlesOutput* msg = static_cast<vplex::vsDirectory::GetTitlesOutput*>(state->protoStack.top());
    processCloseGetTitlesOutput(msg, tag, state);
}

void
processOpenGetTitleDetailsInput(vplex::vsDirectory::GetTitleDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "l10n") == 0) {
        if (msg->has_l10n()) {
            state->error = "Duplicate of non-repeated field l10n";
        } else {
            vplex::vsDirectory::Localization* proto = msg->mutable_l10n();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetTitleDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitleDetailsInput* msg = static_cast<vplex::vsDirectory::GetTitleDetailsInput*>(state->protoStack.top());
    processOpenGetTitleDetailsInput(msg, tag, state);
}

void
processCloseGetTitleDetailsInput(vplex::vsDirectory::GetTitleDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "titleIds") == 0) {
        msg->add_titleids(state->lastData);
    } else {
        (void)msg;
    }
}
void
processCloseGetTitleDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitleDetailsInput* msg = static_cast<vplex::vsDirectory::GetTitleDetailsInput*>(state->protoStack.top());
    processCloseGetTitleDetailsInput(msg, tag, state);
}

void
processOpenGetTitleDetailsOutput(vplex::vsDirectory::GetTitleDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "titleDetails") == 0) {
        vplex::vsDirectory::TitleDetail* proto = msg->add_titledetails();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetTitleDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitleDetailsOutput* msg = static_cast<vplex::vsDirectory::GetTitleDetailsOutput*>(state->protoStack.top());
    processOpenGetTitleDetailsOutput(msg, tag, state);
}

void
processCloseGetTitleDetailsOutput(vplex::vsDirectory::GetTitleDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetTitleDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetTitleDetailsOutput* msg = static_cast<vplex::vsDirectory::GetTitleDetailsOutput*>(state->protoStack.top());
    processCloseGetTitleDetailsOutput(msg, tag, state);
}

void
processOpenGetAttestationChallengeInput(vplex::vsDirectory::GetAttestationChallengeInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetAttestationChallengeInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAttestationChallengeInput* msg = static_cast<vplex::vsDirectory::GetAttestationChallengeInput*>(state->protoStack.top());
    processOpenGetAttestationChallengeInput(msg, tag, state);
}

void
processCloseGetAttestationChallengeInput(vplex::vsDirectory::GetAttestationChallengeInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetAttestationChallengeInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAttestationChallengeInput* msg = static_cast<vplex::vsDirectory::GetAttestationChallengeInput*>(state->protoStack.top());
    processCloseGetAttestationChallengeInput(msg, tag, state);
}

void
processOpenGetAttestationChallengeOutput(vplex::vsDirectory::GetAttestationChallengeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetAttestationChallengeOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAttestationChallengeOutput* msg = static_cast<vplex::vsDirectory::GetAttestationChallengeOutput*>(state->protoStack.top());
    processOpenGetAttestationChallengeOutput(msg, tag, state);
}

void
processCloseGetAttestationChallengeOutput(vplex::vsDirectory::GetAttestationChallengeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "challenge") == 0) {
        if (msg->has_challenge()) {
            state->error = "Duplicate of non-repeated field challenge";
        } else {
            msg->set_challenge(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "challengeTmd") == 0) {
        if (msg->has_challengetmd()) {
            state->error = "Duplicate of non-repeated field challengeTmd";
        } else {
            msg->set_challengetmd(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetAttestationChallengeOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAttestationChallengeOutput* msg = static_cast<vplex::vsDirectory::GetAttestationChallengeOutput*>(state->protoStack.top());
    processCloseGetAttestationChallengeOutput(msg, tag, state);
}

void
processOpenAuthenticateDeviceInput(vplex::vsDirectory::AuthenticateDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAuthenticateDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AuthenticateDeviceInput* msg = static_cast<vplex::vsDirectory::AuthenticateDeviceInput*>(state->protoStack.top());
    processOpenAuthenticateDeviceInput(msg, tag, state);
}

void
processCloseAuthenticateDeviceInput(vplex::vsDirectory::AuthenticateDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "challengeResponse") == 0) {
        if (msg->has_challengeresponse()) {
            state->error = "Duplicate of non-repeated field challengeResponse";
        } else {
            msg->set_challengeresponse(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceCertificate") == 0) {
        if (msg->has_devicecertificate()) {
            state->error = "Duplicate of non-repeated field deviceCertificate";
        } else {
            msg->set_devicecertificate(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseAuthenticateDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AuthenticateDeviceInput* msg = static_cast<vplex::vsDirectory::AuthenticateDeviceInput*>(state->protoStack.top());
    processCloseAuthenticateDeviceInput(msg, tag, state);
}

void
processOpenAuthenticateDeviceOutput(vplex::vsDirectory::AuthenticateDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAuthenticateDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AuthenticateDeviceOutput* msg = static_cast<vplex::vsDirectory::AuthenticateDeviceOutput*>(state->protoStack.top());
    processOpenAuthenticateDeviceOutput(msg, tag, state);
}

void
processCloseAuthenticateDeviceOutput(vplex::vsDirectory::AuthenticateDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAuthenticateDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AuthenticateDeviceOutput* msg = static_cast<vplex::vsDirectory::AuthenticateDeviceOutput*>(state->protoStack.top());
    processCloseAuthenticateDeviceOutput(msg, tag, state);
}

void
processOpenGetOnlineTitleTicketInput(vplex::vsDirectory::GetOnlineTitleTicketInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetOnlineTitleTicketInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOnlineTitleTicketInput* msg = static_cast<vplex::vsDirectory::GetOnlineTitleTicketInput*>(state->protoStack.top());
    processOpenGetOnlineTitleTicketInput(msg, tag, state);
}

void
processCloseGetOnlineTitleTicketInput(vplex::vsDirectory::GetOnlineTitleTicketInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceCertificate") == 0) {
        if (msg->has_devicecertificate()) {
            state->error = "Duplicate of non-repeated field deviceCertificate";
        } else {
            msg->set_devicecertificate(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "titleId") == 0) {
        if (msg->has_titleid()) {
            state->error = "Duplicate of non-repeated field titleId";
        } else {
            msg->set_titleid(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetOnlineTitleTicketInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOnlineTitleTicketInput* msg = static_cast<vplex::vsDirectory::GetOnlineTitleTicketInput*>(state->protoStack.top());
    processCloseGetOnlineTitleTicketInput(msg, tag, state);
}

void
processOpenGetOnlineTitleTicketOutput(vplex::vsDirectory::GetOnlineTitleTicketOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "eTicket") == 0) {
        if (msg->has_eticket()) {
            state->error = "Duplicate of non-repeated field eTicket";
        } else {
            vplex::vsDirectory::ETicketData* proto = msg->mutable_eticket();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetOnlineTitleTicketOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOnlineTitleTicketOutput* msg = static_cast<vplex::vsDirectory::GetOnlineTitleTicketOutput*>(state->protoStack.top());
    processOpenGetOnlineTitleTicketOutput(msg, tag, state);
}

void
processCloseGetOnlineTitleTicketOutput(vplex::vsDirectory::GetOnlineTitleTicketOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetOnlineTitleTicketOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOnlineTitleTicketOutput* msg = static_cast<vplex::vsDirectory::GetOnlineTitleTicketOutput*>(state->protoStack.top());
    processCloseGetOnlineTitleTicketOutput(msg, tag, state);
}

void
processOpenGetOfflineTitleTicketsInput(vplex::vsDirectory::GetOfflineTitleTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetOfflineTitleTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOfflineTitleTicketsInput* msg = static_cast<vplex::vsDirectory::GetOfflineTitleTicketsInput*>(state->protoStack.top());
    processOpenGetOfflineTitleTicketsInput(msg, tag, state);
}

void
processCloseGetOfflineTitleTicketsInput(vplex::vsDirectory::GetOfflineTitleTicketsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceCertificate") == 0) {
        if (msg->has_devicecertificate()) {
            state->error = "Duplicate of non-repeated field deviceCertificate";
        } else {
            msg->set_devicecertificate(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "titleIds") == 0) {
        msg->add_titleids(state->lastData);
    } else {
        (void)msg;
    }
}
void
processCloseGetOfflineTitleTicketsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOfflineTitleTicketsInput* msg = static_cast<vplex::vsDirectory::GetOfflineTitleTicketsInput*>(state->protoStack.top());
    processCloseGetOfflineTitleTicketsInput(msg, tag, state);
}

void
processOpenGetOfflineTitleTicketsOutput(vplex::vsDirectory::GetOfflineTitleTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "titleTickets") == 0) {
        vplex::vsDirectory::TitleTicket* proto = msg->add_titletickets();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetOfflineTitleTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOfflineTitleTicketsOutput* msg = static_cast<vplex::vsDirectory::GetOfflineTitleTicketsOutput*>(state->protoStack.top());
    processOpenGetOfflineTitleTicketsOutput(msg, tag, state);
}

void
processCloseGetOfflineTitleTicketsOutput(vplex::vsDirectory::GetOfflineTitleTicketsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetOfflineTitleTicketsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetOfflineTitleTicketsOutput* msg = static_cast<vplex::vsDirectory::GetOfflineTitleTicketsOutput*>(state->protoStack.top());
    processCloseGetOfflineTitleTicketsOutput(msg, tag, state);
}

void
processOpenListOwnedDataSetsInput(vplex::vsDirectory::ListOwnedDataSetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenListOwnedDataSetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListOwnedDataSetsInput* msg = static_cast<vplex::vsDirectory::ListOwnedDataSetsInput*>(state->protoStack.top());
    processOpenListOwnedDataSetsInput(msg, tag, state);
}

void
processCloseListOwnedDataSetsInput(vplex::vsDirectory::ListOwnedDataSetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseListOwnedDataSetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListOwnedDataSetsInput* msg = static_cast<vplex::vsDirectory::ListOwnedDataSetsInput*>(state->protoStack.top());
    processCloseListOwnedDataSetsInput(msg, tag, state);
}

void
processOpenListOwnedDataSetsOutput(vplex::vsDirectory::ListOwnedDataSetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "datasets") == 0) {
        vplex::vsDirectory::DatasetDetail* proto = msg->add_datasets();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenListOwnedDataSetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListOwnedDataSetsOutput* msg = static_cast<vplex::vsDirectory::ListOwnedDataSetsOutput*>(state->protoStack.top());
    processOpenListOwnedDataSetsOutput(msg, tag, state);
}

void
processCloseListOwnedDataSetsOutput(vplex::vsDirectory::ListOwnedDataSetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseListOwnedDataSetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListOwnedDataSetsOutput* msg = static_cast<vplex::vsDirectory::ListOwnedDataSetsOutput*>(state->protoStack.top());
    processCloseListOwnedDataSetsOutput(msg, tag, state);
}

void
processOpenGetDatasetDetailsInput(vplex::vsDirectory::GetDatasetDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetDatasetDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetDetailsInput* msg = static_cast<vplex::vsDirectory::GetDatasetDetailsInput*>(state->protoStack.top());
    processOpenGetDatasetDetailsInput(msg, tag, state);
}

void
processCloseGetDatasetDetailsInput(vplex::vsDirectory::GetDatasetDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDatasetDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetDetailsInput* msg = static_cast<vplex::vsDirectory::GetDatasetDetailsInput*>(state->protoStack.top());
    processCloseGetDatasetDetailsInput(msg, tag, state);
}

void
processOpenGetDatasetDetailsOutput(vplex::vsDirectory::GetDatasetDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "datasetDetail") == 0) {
        if (msg->has_datasetdetail()) {
            state->error = "Duplicate of non-repeated field datasetDetail";
        } else {
            vplex::vsDirectory::DatasetDetail* proto = msg->mutable_datasetdetail();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetDatasetDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetDetailsOutput* msg = static_cast<vplex::vsDirectory::GetDatasetDetailsOutput*>(state->protoStack.top());
    processOpenGetDatasetDetailsOutput(msg, tag, state);
}

void
processCloseGetDatasetDetailsOutput(vplex::vsDirectory::GetDatasetDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetDatasetDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetDetailsOutput* msg = static_cast<vplex::vsDirectory::GetDatasetDetailsOutput*>(state->protoStack.top());
    processCloseGetDatasetDetailsOutput(msg, tag, state);
}

void
processOpenAddDataSetInput(vplex::vsDirectory::AddDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDataSetInput* msg = static_cast<vplex::vsDirectory::AddDataSetInput*>(state->protoStack.top());
    processOpenAddDataSetInput(msg, tag, state);
}

void
processCloseAddDataSetInput(vplex::vsDirectory::AddDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "datasetTypeId") == 0) {
        if (msg->has_datasettypeid()) {
            state->error = "Duplicate of non-repeated field datasetTypeId";
        } else {
            msg->set_datasettypeid(vplex::vsDirectory::parseDatasetType(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDataSetInput* msg = static_cast<vplex::vsDirectory::AddDataSetInput*>(state->protoStack.top());
    processCloseAddDataSetInput(msg, tag, state);
}

void
processOpenAddDataSetOutput(vplex::vsDirectory::AddDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDataSetOutput* msg = static_cast<vplex::vsDirectory::AddDataSetOutput*>(state->protoStack.top());
    processOpenAddDataSetOutput(msg, tag, state);
}

void
processCloseAddDataSetOutput(vplex::vsDirectory::AddDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDataSetOutput* msg = static_cast<vplex::vsDirectory::AddDataSetOutput*>(state->protoStack.top());
    processCloseAddDataSetOutput(msg, tag, state);
}

void
processOpenAddCameraDatasetInput(vplex::vsDirectory::AddCameraDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddCameraDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraDatasetInput* msg = static_cast<vplex::vsDirectory::AddCameraDatasetInput*>(state->protoStack.top());
    processOpenAddCameraDatasetInput(msg, tag, state);
}

void
processCloseAddCameraDatasetInput(vplex::vsDirectory::AddCameraDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "createdFor") == 0) {
        if (msg->has_createdfor()) {
            state->error = "Duplicate of non-repeated field createdFor";
        } else {
            msg->set_createdfor(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddCameraDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraDatasetInput* msg = static_cast<vplex::vsDirectory::AddCameraDatasetInput*>(state->protoStack.top());
    processCloseAddCameraDatasetInput(msg, tag, state);
}

void
processOpenAddCameraDatasetOutput(vplex::vsDirectory::AddCameraDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddCameraDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraDatasetOutput* msg = static_cast<vplex::vsDirectory::AddCameraDatasetOutput*>(state->protoStack.top());
    processOpenAddCameraDatasetOutput(msg, tag, state);
}

void
processCloseAddCameraDatasetOutput(vplex::vsDirectory::AddCameraDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddCameraDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraDatasetOutput* msg = static_cast<vplex::vsDirectory::AddCameraDatasetOutput*>(state->protoStack.top());
    processCloseAddCameraDatasetOutput(msg, tag, state);
}

void
processOpenDeleteDataSetInput(vplex::vsDirectory::DeleteDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenDeleteDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteDataSetInput* msg = static_cast<vplex::vsDirectory::DeleteDataSetInput*>(state->protoStack.top());
    processOpenDeleteDataSetInput(msg, tag, state);
}

void
processCloseDeleteDataSetInput(vplex::vsDirectory::DeleteDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseDeleteDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteDataSetInput* msg = static_cast<vplex::vsDirectory::DeleteDataSetInput*>(state->protoStack.top());
    processCloseDeleteDataSetInput(msg, tag, state);
}

void
processOpenDeleteDataSetOutput(vplex::vsDirectory::DeleteDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenDeleteDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteDataSetOutput* msg = static_cast<vplex::vsDirectory::DeleteDataSetOutput*>(state->protoStack.top());
    processOpenDeleteDataSetOutput(msg, tag, state);
}

void
processCloseDeleteDataSetOutput(vplex::vsDirectory::DeleteDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseDeleteDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteDataSetOutput* msg = static_cast<vplex::vsDirectory::DeleteDataSetOutput*>(state->protoStack.top());
    processCloseDeleteDataSetOutput(msg, tag, state);
}

void
processOpenRenameDataSetInput(vplex::vsDirectory::RenameDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenRenameDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RenameDataSetInput* msg = static_cast<vplex::vsDirectory::RenameDataSetInput*>(state->protoStack.top());
    processOpenRenameDataSetInput(msg, tag, state);
}

void
processCloseRenameDataSetInput(vplex::vsDirectory::RenameDataSetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "datasetNameNew") == 0) {
        if (msg->has_datasetnamenew()) {
            state->error = "Duplicate of non-repeated field datasetNameNew";
        } else {
            msg->set_datasetnamenew(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseRenameDataSetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RenameDataSetInput* msg = static_cast<vplex::vsDirectory::RenameDataSetInput*>(state->protoStack.top());
    processCloseRenameDataSetInput(msg, tag, state);
}

void
processOpenRenameDataSetOutput(vplex::vsDirectory::RenameDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenRenameDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RenameDataSetOutput* msg = static_cast<vplex::vsDirectory::RenameDataSetOutput*>(state->protoStack.top());
    processOpenRenameDataSetOutput(msg, tag, state);
}

void
processCloseRenameDataSetOutput(vplex::vsDirectory::RenameDataSetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseRenameDataSetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RenameDataSetOutput* msg = static_cast<vplex::vsDirectory::RenameDataSetOutput*>(state->protoStack.top());
    processCloseRenameDataSetOutput(msg, tag, state);
}

void
processOpenSetDataSetCacheInput(vplex::vsDirectory::SetDataSetCacheInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenSetDataSetCacheInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDataSetCacheInput* msg = static_cast<vplex::vsDirectory::SetDataSetCacheInput*>(state->protoStack.top());
    processOpenSetDataSetCacheInput(msg, tag, state);
}

void
processCloseSetDataSetCacheInput(vplex::vsDirectory::SetDataSetCacheInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "cacheDatasetId") == 0) {
        if (msg->has_cachedatasetid()) {
            state->error = "Duplicate of non-repeated field cacheDatasetId";
        } else {
            msg->set_cachedatasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseSetDataSetCacheInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDataSetCacheInput* msg = static_cast<vplex::vsDirectory::SetDataSetCacheInput*>(state->protoStack.top());
    processCloseSetDataSetCacheInput(msg, tag, state);
}

void
processOpenSetDataSetCacheOutput(vplex::vsDirectory::SetDataSetCacheOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenSetDataSetCacheOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDataSetCacheOutput* msg = static_cast<vplex::vsDirectory::SetDataSetCacheOutput*>(state->protoStack.top());
    processOpenSetDataSetCacheOutput(msg, tag, state);
}

void
processCloseSetDataSetCacheOutput(vplex::vsDirectory::SetDataSetCacheOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseSetDataSetCacheOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDataSetCacheOutput* msg = static_cast<vplex::vsDirectory::SetDataSetCacheOutput*>(state->protoStack.top());
    processCloseSetDataSetCacheOutput(msg, tag, state);
}

void
processOpenRemoveDeviceFromSubscriptionsInput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenRemoveDeviceFromSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput* msg = static_cast<vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput*>(state->protoStack.top());
    processOpenRemoveDeviceFromSubscriptionsInput(msg, tag, state);
}

void
processCloseRemoveDeviceFromSubscriptionsInput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseRemoveDeviceFromSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput* msg = static_cast<vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput*>(state->protoStack.top());
    processCloseRemoveDeviceFromSubscriptionsInput(msg, tag, state);
}

void
processOpenRemoveDeviceFromSubscriptionsOutput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenRemoveDeviceFromSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput*>(state->protoStack.top());
    processOpenRemoveDeviceFromSubscriptionsOutput(msg, tag, state);
}

void
processCloseRemoveDeviceFromSubscriptionsOutput(vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseRemoveDeviceFromSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput*>(state->protoStack.top());
    processCloseRemoveDeviceFromSubscriptionsOutput(msg, tag, state);
}

void
processOpenListSubscriptionsInput(vplex::vsDirectory::ListSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenListSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListSubscriptionsInput* msg = static_cast<vplex::vsDirectory::ListSubscriptionsInput*>(state->protoStack.top());
    processOpenListSubscriptionsInput(msg, tag, state);
}

void
processCloseListSubscriptionsInput(vplex::vsDirectory::ListSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseListSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListSubscriptionsInput* msg = static_cast<vplex::vsDirectory::ListSubscriptionsInput*>(state->protoStack.top());
    processCloseListSubscriptionsInput(msg, tag, state);
}

void
processOpenListSubscriptionsOutput(vplex::vsDirectory::ListSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "subscriptions") == 0) {
        vplex::vsDirectory::Subscription* proto = msg->add_subscriptions();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenListSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::ListSubscriptionsOutput*>(state->protoStack.top());
    processOpenListSubscriptionsOutput(msg, tag, state);
}

void
processCloseListSubscriptionsOutput(vplex::vsDirectory::ListSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseListSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::ListSubscriptionsOutput*>(state->protoStack.top());
    processCloseListSubscriptionsOutput(msg, tag, state);
}

void
processOpenAddSubscriptionsInput(vplex::vsDirectory::AddSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "subscriptions") == 0) {
        vplex::vsDirectory::Subscription* proto = msg->add_subscriptions();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenAddSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddSubscriptionsInput* msg = static_cast<vplex::vsDirectory::AddSubscriptionsInput*>(state->protoStack.top());
    processOpenAddSubscriptionsInput(msg, tag, state);
}

void
processCloseAddSubscriptionsInput(vplex::vsDirectory::AddSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddSubscriptionsInput* msg = static_cast<vplex::vsDirectory::AddSubscriptionsInput*>(state->protoStack.top());
    processCloseAddSubscriptionsInput(msg, tag, state);
}

void
processOpenAddSubscriptionsOutput(vplex::vsDirectory::AddSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::AddSubscriptionsOutput*>(state->protoStack.top());
    processOpenAddSubscriptionsOutput(msg, tag, state);
}

void
processCloseAddSubscriptionsOutput(vplex::vsDirectory::AddSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAddSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::AddSubscriptionsOutput*>(state->protoStack.top());
    processCloseAddSubscriptionsOutput(msg, tag, state);
}

void
processOpenAddUserDatasetSubscriptionInput(vplex::vsDirectory::AddUserDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddUserDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserDatasetSubscriptionInput* msg = static_cast<vplex::vsDirectory::AddUserDatasetSubscriptionInput*>(state->protoStack.top());
    processOpenAddUserDatasetSubscriptionInput(msg, tag, state);
}

void
processCloseAddUserDatasetSubscriptionInput(vplex::vsDirectory::AddUserDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceRoot") == 0) {
        if (msg->has_deviceroot()) {
            state->error = "Duplicate of non-repeated field deviceRoot";
        } else {
            msg->set_deviceroot(state->lastData);
        }
    } else if (strcmp(tag, "filter") == 0) {
        if (msg->has_filter()) {
            state->error = "Duplicate of non-repeated field filter";
        } else {
            msg->set_filter(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddUserDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserDatasetSubscriptionInput* msg = static_cast<vplex::vsDirectory::AddUserDatasetSubscriptionInput*>(state->protoStack.top());
    processCloseAddUserDatasetSubscriptionInput(msg, tag, state);
}

void
processOpenAddUserDatasetSubscriptionOutput(vplex::vsDirectory::AddUserDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddUserDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserDatasetSubscriptionOutput* msg = static_cast<vplex::vsDirectory::AddUserDatasetSubscriptionOutput*>(state->protoStack.top());
    processOpenAddUserDatasetSubscriptionOutput(msg, tag, state);
}

void
processCloseAddUserDatasetSubscriptionOutput(vplex::vsDirectory::AddUserDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAddUserDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserDatasetSubscriptionOutput* msg = static_cast<vplex::vsDirectory::AddUserDatasetSubscriptionOutput*>(state->protoStack.top());
    processCloseAddUserDatasetSubscriptionOutput(msg, tag, state);
}

void
processOpenAddCameraSubscriptionInput(vplex::vsDirectory::AddCameraSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddCameraSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraSubscriptionInput* msg = static_cast<vplex::vsDirectory::AddCameraSubscriptionInput*>(state->protoStack.top());
    processOpenAddCameraSubscriptionInput(msg, tag, state);
}

void
processCloseAddCameraSubscriptionInput(vplex::vsDirectory::AddCameraSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceRoot") == 0) {
        if (msg->has_deviceroot()) {
            state->error = "Duplicate of non-repeated field deviceRoot";
        } else {
            msg->set_deviceroot(state->lastData);
        }
    } else if (strcmp(tag, "filter") == 0) {
        if (msg->has_filter()) {
            state->error = "Duplicate of non-repeated field filter";
        } else {
            msg->set_filter(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddCameraSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraSubscriptionInput* msg = static_cast<vplex::vsDirectory::AddCameraSubscriptionInput*>(state->protoStack.top());
    processCloseAddCameraSubscriptionInput(msg, tag, state);
}

void
processOpenAddCameraSubscriptionOutput(vplex::vsDirectory::AddCameraSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddCameraSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraSubscriptionOutput* msg = static_cast<vplex::vsDirectory::AddCameraSubscriptionOutput*>(state->protoStack.top());
    processOpenAddCameraSubscriptionOutput(msg, tag, state);
}

void
processCloseAddCameraSubscriptionOutput(vplex::vsDirectory::AddCameraSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAddCameraSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddCameraSubscriptionOutput* msg = static_cast<vplex::vsDirectory::AddCameraSubscriptionOutput*>(state->protoStack.top());
    processCloseAddCameraSubscriptionOutput(msg, tag, state);
}

void
processOpenAddDatasetSubscriptionInput(vplex::vsDirectory::AddDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetSubscriptionInput* msg = static_cast<vplex::vsDirectory::AddDatasetSubscriptionInput*>(state->protoStack.top());
    processOpenAddDatasetSubscriptionInput(msg, tag, state);
}

void
processCloseAddDatasetSubscriptionInput(vplex::vsDirectory::AddDatasetSubscriptionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetType") == 0) {
        if (msg->has_datasettype()) {
            state->error = "Duplicate of non-repeated field datasetType";
        } else {
            msg->set_datasettype(vplex::vsDirectory::parseDatasetType(state->lastData, state));
        }
    } else if (strcmp(tag, "role") == 0) {
        if (msg->has_role()) {
            state->error = "Duplicate of non-repeated field role";
        } else {
            msg->set_role(vplex::vsDirectory::parseSubscriptionRole(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceRoot") == 0) {
        if (msg->has_deviceroot()) {
            state->error = "Duplicate of non-repeated field deviceRoot";
        } else {
            msg->set_deviceroot(state->lastData);
        }
    } else if (strcmp(tag, "filter") == 0) {
        if (msg->has_filter()) {
            state->error = "Duplicate of non-repeated field filter";
        } else {
            msg->set_filter(state->lastData);
        }
    } else if (strcmp(tag, "maxSize") == 0) {
        if (msg->has_maxsize()) {
            state->error = "Duplicate of non-repeated field maxSize";
        } else {
            msg->set_maxsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "maxFiles") == 0) {
        if (msg->has_maxfiles()) {
            state->error = "Duplicate of non-repeated field maxFiles";
        } else {
            msg->set_maxfiles((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddDatasetSubscriptionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetSubscriptionInput* msg = static_cast<vplex::vsDirectory::AddDatasetSubscriptionInput*>(state->protoStack.top());
    processCloseAddDatasetSubscriptionInput(msg, tag, state);
}

void
processOpenAddDatasetSubscriptionOutput(vplex::vsDirectory::AddDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetSubscriptionOutput* msg = static_cast<vplex::vsDirectory::AddDatasetSubscriptionOutput*>(state->protoStack.top());
    processOpenAddDatasetSubscriptionOutput(msg, tag, state);
}

void
processCloseAddDatasetSubscriptionOutput(vplex::vsDirectory::AddDatasetSubscriptionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAddDatasetSubscriptionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetSubscriptionOutput* msg = static_cast<vplex::vsDirectory::AddDatasetSubscriptionOutput*>(state->protoStack.top());
    processCloseAddDatasetSubscriptionOutput(msg, tag, state);
}

void
processOpenDeleteSubscriptionsInput(vplex::vsDirectory::DeleteSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenDeleteSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteSubscriptionsInput* msg = static_cast<vplex::vsDirectory::DeleteSubscriptionsInput*>(state->protoStack.top());
    processOpenDeleteSubscriptionsInput(msg, tag, state);
}

void
processCloseDeleteSubscriptionsInput(vplex::vsDirectory::DeleteSubscriptionsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetNames") == 0) {
        msg->add_datasetnames(state->lastData);
    } else if (strcmp(tag, "datasetIds") == 0) {
        msg->add_datasetids((u64)parseInt64(state->lastData, state));
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseDeleteSubscriptionsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteSubscriptionsInput* msg = static_cast<vplex::vsDirectory::DeleteSubscriptionsInput*>(state->protoStack.top());
    processCloseDeleteSubscriptionsInput(msg, tag, state);
}

void
processOpenDeleteSubscriptionsOutput(vplex::vsDirectory::DeleteSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenDeleteSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::DeleteSubscriptionsOutput*>(state->protoStack.top());
    processOpenDeleteSubscriptionsOutput(msg, tag, state);
}

void
processCloseDeleteSubscriptionsOutput(vplex::vsDirectory::DeleteSubscriptionsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseDeleteSubscriptionsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteSubscriptionsOutput* msg = static_cast<vplex::vsDirectory::DeleteSubscriptionsOutput*>(state->protoStack.top());
    processCloseDeleteSubscriptionsOutput(msg, tag, state);
}

void
processOpenUpdateSubscriptionFilterInput(vplex::vsDirectory::UpdateSubscriptionFilterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateSubscriptionFilterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionFilterInput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionFilterInput*>(state->protoStack.top());
    processOpenUpdateSubscriptionFilterInput(msg, tag, state);
}

void
processCloseUpdateSubscriptionFilterInput(vplex::vsDirectory::UpdateSubscriptionFilterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "filter") == 0) {
        if (msg->has_filter()) {
            state->error = "Duplicate of non-repeated field filter";
        } else {
            msg->set_filter(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateSubscriptionFilterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionFilterInput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionFilterInput*>(state->protoStack.top());
    processCloseUpdateSubscriptionFilterInput(msg, tag, state);
}

void
processOpenUpdateSubscriptionFilterOutput(vplex::vsDirectory::UpdateSubscriptionFilterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateSubscriptionFilterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionFilterOutput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionFilterOutput*>(state->protoStack.top());
    processOpenUpdateSubscriptionFilterOutput(msg, tag, state);
}

void
processCloseUpdateSubscriptionFilterOutput(vplex::vsDirectory::UpdateSubscriptionFilterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateSubscriptionFilterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionFilterOutput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionFilterOutput*>(state->protoStack.top());
    processCloseUpdateSubscriptionFilterOutput(msg, tag, state);
}

void
processOpenUpdateSubscriptionLimitsInput(vplex::vsDirectory::UpdateSubscriptionLimitsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateSubscriptionLimitsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionLimitsInput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionLimitsInput*>(state->protoStack.top());
    processOpenUpdateSubscriptionLimitsInput(msg, tag, state);
}

void
processCloseUpdateSubscriptionLimitsInput(vplex::vsDirectory::UpdateSubscriptionLimitsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetName") == 0) {
        if (msg->has_datasetname()) {
            state->error = "Duplicate of non-repeated field datasetName";
        } else {
            msg->set_datasetname(state->lastData);
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "filter") == 0) {
        if (msg->has_filter()) {
            state->error = "Duplicate of non-repeated field filter";
        } else {
            msg->set_filter(state->lastData);
        }
    } else if (strcmp(tag, "maxSize") == 0) {
        if (msg->has_maxsize()) {
            state->error = "Duplicate of non-repeated field maxSize";
        } else {
            msg->set_maxsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "maxFiles") == 0) {
        if (msg->has_maxfiles()) {
            state->error = "Duplicate of non-repeated field maxFiles";
        } else {
            msg->set_maxfiles((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateSubscriptionLimitsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionLimitsInput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionLimitsInput*>(state->protoStack.top());
    processCloseUpdateSubscriptionLimitsInput(msg, tag, state);
}

void
processOpenUpdateSubscriptionLimitsOutput(vplex::vsDirectory::UpdateSubscriptionLimitsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateSubscriptionLimitsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionLimitsOutput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionLimitsOutput*>(state->protoStack.top());
    processOpenUpdateSubscriptionLimitsOutput(msg, tag, state);
}

void
processCloseUpdateSubscriptionLimitsOutput(vplex::vsDirectory::UpdateSubscriptionLimitsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateSubscriptionLimitsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateSubscriptionLimitsOutput* msg = static_cast<vplex::vsDirectory::UpdateSubscriptionLimitsOutput*>(state->protoStack.top());
    processCloseUpdateSubscriptionLimitsOutput(msg, tag, state);
}

void
processOpenGetSubscriptionDetailsForDeviceInput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetSubscriptionDetailsForDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput*>(state->protoStack.top());
    processOpenGetSubscriptionDetailsForDeviceInput(msg, tag, state);
}

void
processCloseGetSubscriptionDetailsForDeviceInput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetSubscriptionDetailsForDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput*>(state->protoStack.top());
    processCloseGetSubscriptionDetailsForDeviceInput(msg, tag, state);
}

void
processOpenGetSubscriptionDetailsForDeviceOutput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "subscriptions") == 0) {
        vplex::vsDirectory::Subscription* proto = msg->add_subscriptions();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSubscriptionDetailsForDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput*>(state->protoStack.top());
    processOpenGetSubscriptionDetailsForDeviceOutput(msg, tag, state);
}

void
processCloseGetSubscriptionDetailsForDeviceOutput(vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetSubscriptionDetailsForDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput*>(state->protoStack.top());
    processCloseGetSubscriptionDetailsForDeviceOutput(msg, tag, state);
}

void
processOpenGetCloudInfoInput(vplex::vsDirectory::GetCloudInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetCloudInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetCloudInfoInput* msg = static_cast<vplex::vsDirectory::GetCloudInfoInput*>(state->protoStack.top());
    processOpenGetCloudInfoInput(msg, tag, state);
}

void
processCloseGetCloudInfoInput(vplex::vsDirectory::GetCloudInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetCloudInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetCloudInfoInput* msg = static_cast<vplex::vsDirectory::GetCloudInfoInput*>(state->protoStack.top());
    processCloseGetCloudInfoInput(msg, tag, state);
}

void
processOpenGetCloudInfoOutput(vplex::vsDirectory::GetCloudInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "devices") == 0) {
        vplex::vsDirectory::DeviceInfo* proto = msg->add_devices();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else if (strcmp(tag, "datasets") == 0) {
        vplex::vsDirectory::DatasetDetail* proto = msg->add_datasets();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else if (strcmp(tag, "subscriptions") == 0) {
        vplex::vsDirectory::Subscription* proto = msg->add_subscriptions();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else if (strcmp(tag, "storageAssignments") == 0) {
        vplex::vsDirectory::UserStorage* proto = msg->add_storageassignments();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetCloudInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetCloudInfoOutput* msg = static_cast<vplex::vsDirectory::GetCloudInfoOutput*>(state->protoStack.top());
    processOpenGetCloudInfoOutput(msg, tag, state);
}

void
processCloseGetCloudInfoOutput(vplex::vsDirectory::GetCloudInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetCloudInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetCloudInfoOutput* msg = static_cast<vplex::vsDirectory::GetCloudInfoOutput*>(state->protoStack.top());
    processCloseGetCloudInfoOutput(msg, tag, state);
}

void
processOpenGetSubscribedDatasetsInput(vplex::vsDirectory::GetSubscribedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "l10n") == 0) {
        if (msg->has_l10n()) {
            state->error = "Duplicate of non-repeated field l10n";
        } else {
            vplex::vsDirectory::Localization* proto = msg->mutable_l10n();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetSubscribedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscribedDatasetsInput* msg = static_cast<vplex::vsDirectory::GetSubscribedDatasetsInput*>(state->protoStack.top());
    processOpenGetSubscribedDatasetsInput(msg, tag, state);
}

void
processCloseGetSubscribedDatasetsInput(vplex::vsDirectory::GetSubscribedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetSubscribedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscribedDatasetsInput* msg = static_cast<vplex::vsDirectory::GetSubscribedDatasetsInput*>(state->protoStack.top());
    processCloseGetSubscribedDatasetsInput(msg, tag, state);
}

void
processOpenGetSubscribedDatasetsOutput(vplex::vsDirectory::GetSubscribedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "titleData") == 0) {
        vplex::vsDirectory::TitleData* proto = msg->add_titledata();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else if (strcmp(tag, "datasetData") == 0) {
        vplex::vsDirectory::DatasetData* proto = msg->add_datasetdata();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSubscribedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscribedDatasetsOutput* msg = static_cast<vplex::vsDirectory::GetSubscribedDatasetsOutput*>(state->protoStack.top());
    processOpenGetSubscribedDatasetsOutput(msg, tag, state);
}

void
processCloseGetSubscribedDatasetsOutput(vplex::vsDirectory::GetSubscribedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetSubscribedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscribedDatasetsOutput* msg = static_cast<vplex::vsDirectory::GetSubscribedDatasetsOutput*>(state->protoStack.top());
    processCloseGetSubscribedDatasetsOutput(msg, tag, state);
}

void
processOpenGetSubscriptionDetailsInput(vplex::vsDirectory::GetSubscriptionDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetSubscriptionDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsInput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsInput*>(state->protoStack.top());
    processOpenGetSubscriptionDetailsInput(msg, tag, state);
}

void
processCloseGetSubscriptionDetailsInput(vplex::vsDirectory::GetSubscriptionDetailsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetIds") == 0) {
        msg->add_datasetids((u64)parseInt64(state->lastData, state));
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetSubscriptionDetailsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsInput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsInput*>(state->protoStack.top());
    processCloseGetSubscriptionDetailsInput(msg, tag, state);
}

void
processOpenGetSubscriptionDetailsOutput(vplex::vsDirectory::GetSubscriptionDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "subscriptions") == 0) {
        vplex::vsDirectory::Subscription* proto = msg->add_subscriptions();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetSubscriptionDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsOutput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsOutput*>(state->protoStack.top());
    processOpenGetSubscriptionDetailsOutput(msg, tag, state);
}

void
processCloseGetSubscriptionDetailsOutput(vplex::vsDirectory::GetSubscriptionDetailsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetSubscriptionDetailsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetSubscriptionDetailsOutput* msg = static_cast<vplex::vsDirectory::GetSubscriptionDetailsOutput*>(state->protoStack.top());
    processCloseGetSubscriptionDetailsOutput(msg, tag, state);
}

void
processOpenLinkDeviceInput(vplex::vsDirectory::LinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenLinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::LinkDeviceInput* msg = static_cast<vplex::vsDirectory::LinkDeviceInput*>(state->protoStack.top());
    processOpenLinkDeviceInput(msg, tag, state);
}

void
processCloseLinkDeviceInput(vplex::vsDirectory::LinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceClass") == 0) {
        if (msg->has_deviceclass()) {
            state->error = "Duplicate of non-repeated field deviceClass";
        } else {
            msg->set_deviceclass(state->lastData);
        }
    } else if (strcmp(tag, "deviceName") == 0) {
        if (msg->has_devicename()) {
            state->error = "Duplicate of non-repeated field deviceName";
        } else {
            msg->set_devicename(state->lastData);
        }
    } else if (strcmp(tag, "isAcer") == 0) {
        if (msg->has_isacer()) {
            state->error = "Duplicate of non-repeated field isAcer";
        } else {
            msg->set_isacer(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "hasCamera") == 0) {
        if (msg->has_hascamera()) {
            state->error = "Duplicate of non-repeated field hasCamera";
        } else {
            msg->set_hascamera(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "osVersion") == 0) {
        if (msg->has_osversion()) {
            state->error = "Duplicate of non-repeated field osVersion";
        } else {
            msg->set_osversion(state->lastData);
        }
    } else if (strcmp(tag, "protocolVersion") == 0) {
        if (msg->has_protocolversion()) {
            state->error = "Duplicate of non-repeated field protocolVersion";
        } else {
            msg->set_protocolversion(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "modelNumber") == 0) {
        if (msg->has_modelnumber()) {
            state->error = "Duplicate of non-repeated field modelNumber";
        } else {
            msg->set_modelnumber(state->lastData);
        }
    } else if (strcmp(tag, "buildInfo") == 0) {
        if (msg->has_buildinfo()) {
            state->error = "Duplicate of non-repeated field buildInfo";
        } else {
            msg->set_buildinfo(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseLinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::LinkDeviceInput* msg = static_cast<vplex::vsDirectory::LinkDeviceInput*>(state->protoStack.top());
    processCloseLinkDeviceInput(msg, tag, state);
}

void
processOpenLinkDeviceOutput(vplex::vsDirectory::LinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenLinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::LinkDeviceOutput* msg = static_cast<vplex::vsDirectory::LinkDeviceOutput*>(state->protoStack.top());
    processOpenLinkDeviceOutput(msg, tag, state);
}

void
processCloseLinkDeviceOutput(vplex::vsDirectory::LinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseLinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::LinkDeviceOutput* msg = static_cast<vplex::vsDirectory::LinkDeviceOutput*>(state->protoStack.top());
    processCloseLinkDeviceOutput(msg, tag, state);
}

void
processOpenUnlinkDeviceInput(vplex::vsDirectory::UnlinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUnlinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UnlinkDeviceInput* msg = static_cast<vplex::vsDirectory::UnlinkDeviceInput*>(state->protoStack.top());
    processOpenUnlinkDeviceInput(msg, tag, state);
}

void
processCloseUnlinkDeviceInput(vplex::vsDirectory::UnlinkDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUnlinkDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UnlinkDeviceInput* msg = static_cast<vplex::vsDirectory::UnlinkDeviceInput*>(state->protoStack.top());
    processCloseUnlinkDeviceInput(msg, tag, state);
}

void
processOpenUnlinkDeviceOutput(vplex::vsDirectory::UnlinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUnlinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UnlinkDeviceOutput* msg = static_cast<vplex::vsDirectory::UnlinkDeviceOutput*>(state->protoStack.top());
    processOpenUnlinkDeviceOutput(msg, tag, state);
}

void
processCloseUnlinkDeviceOutput(vplex::vsDirectory::UnlinkDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUnlinkDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UnlinkDeviceOutput* msg = static_cast<vplex::vsDirectory::UnlinkDeviceOutput*>(state->protoStack.top());
    processCloseUnlinkDeviceOutput(msg, tag, state);
}

void
processOpenSetDeviceNameInput(vplex::vsDirectory::SetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenSetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDeviceNameInput* msg = static_cast<vplex::vsDirectory::SetDeviceNameInput*>(state->protoStack.top());
    processOpenSetDeviceNameInput(msg, tag, state);
}

void
processCloseSetDeviceNameInput(vplex::vsDirectory::SetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceName") == 0) {
        if (msg->has_devicename()) {
            state->error = "Duplicate of non-repeated field deviceName";
        } else {
            msg->set_devicename(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseSetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDeviceNameInput* msg = static_cast<vplex::vsDirectory::SetDeviceNameInput*>(state->protoStack.top());
    processCloseSetDeviceNameInput(msg, tag, state);
}

void
processOpenSetDeviceNameOutput(vplex::vsDirectory::SetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenSetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDeviceNameOutput* msg = static_cast<vplex::vsDirectory::SetDeviceNameOutput*>(state->protoStack.top());
    processOpenSetDeviceNameOutput(msg, tag, state);
}

void
processCloseSetDeviceNameOutput(vplex::vsDirectory::SetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseSetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SetDeviceNameOutput* msg = static_cast<vplex::vsDirectory::SetDeviceNameOutput*>(state->protoStack.top());
    processCloseSetDeviceNameOutput(msg, tag, state);
}

void
processOpenUpdateDeviceInfoInput(vplex::vsDirectory::UpdateDeviceInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateDeviceInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDeviceInfoInput* msg = static_cast<vplex::vsDirectory::UpdateDeviceInfoInput*>(state->protoStack.top());
    processOpenUpdateDeviceInfoInput(msg, tag, state);
}

void
processCloseUpdateDeviceInfoInput(vplex::vsDirectory::UpdateDeviceInfoInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceName") == 0) {
        if (msg->has_devicename()) {
            state->error = "Duplicate of non-repeated field deviceName";
        } else {
            msg->set_devicename(state->lastData);
        }
    } else if (strcmp(tag, "osVersion") == 0) {
        if (msg->has_osversion()) {
            state->error = "Duplicate of non-repeated field osVersion";
        } else {
            msg->set_osversion(state->lastData);
        }
    } else if (strcmp(tag, "protocolVersion") == 0) {
        if (msg->has_protocolversion()) {
            state->error = "Duplicate of non-repeated field protocolVersion";
        } else {
            msg->set_protocolversion(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "modelNumber") == 0) {
        if (msg->has_modelnumber()) {
            state->error = "Duplicate of non-repeated field modelNumber";
        } else {
            msg->set_modelnumber(state->lastData);
        }
    } else if (strcmp(tag, "buildInfo") == 0) {
        if (msg->has_buildinfo()) {
            state->error = "Duplicate of non-repeated field buildInfo";
        } else {
            msg->set_buildinfo(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateDeviceInfoInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDeviceInfoInput* msg = static_cast<vplex::vsDirectory::UpdateDeviceInfoInput*>(state->protoStack.top());
    processCloseUpdateDeviceInfoInput(msg, tag, state);
}

void
processOpenUpdateDeviceInfoOutput(vplex::vsDirectory::UpdateDeviceInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateDeviceInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDeviceInfoOutput* msg = static_cast<vplex::vsDirectory::UpdateDeviceInfoOutput*>(state->protoStack.top());
    processOpenUpdateDeviceInfoOutput(msg, tag, state);
}

void
processCloseUpdateDeviceInfoOutput(vplex::vsDirectory::UpdateDeviceInfoOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateDeviceInfoOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDeviceInfoOutput* msg = static_cast<vplex::vsDirectory::UpdateDeviceInfoOutput*>(state->protoStack.top());
    processCloseUpdateDeviceInfoOutput(msg, tag, state);
}

void
processOpenGetDeviceLinkStateInput(vplex::vsDirectory::GetDeviceLinkStateInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetDeviceLinkStateInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceLinkStateInput* msg = static_cast<vplex::vsDirectory::GetDeviceLinkStateInput*>(state->protoStack.top());
    processOpenGetDeviceLinkStateInput(msg, tag, state);
}

void
processCloseGetDeviceLinkStateInput(vplex::vsDirectory::GetDeviceLinkStateInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDeviceLinkStateInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceLinkStateInput* msg = static_cast<vplex::vsDirectory::GetDeviceLinkStateInput*>(state->protoStack.top());
    processCloseGetDeviceLinkStateInput(msg, tag, state);
}

void
processOpenGetDeviceLinkStateOutput(vplex::vsDirectory::GetDeviceLinkStateOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetDeviceLinkStateOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceLinkStateOutput* msg = static_cast<vplex::vsDirectory::GetDeviceLinkStateOutput*>(state->protoStack.top());
    processOpenGetDeviceLinkStateOutput(msg, tag, state);
}

void
processCloseGetDeviceLinkStateOutput(vplex::vsDirectory::GetDeviceLinkStateOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "linked") == 0) {
        if (msg->has_linked()) {
            state->error = "Duplicate of non-repeated field linked";
        } else {
            msg->set_linked(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDeviceLinkStateOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceLinkStateOutput* msg = static_cast<vplex::vsDirectory::GetDeviceLinkStateOutput*>(state->protoStack.top());
    processCloseGetDeviceLinkStateOutput(msg, tag, state);
}

void
processOpenGetDeviceNameInput(vplex::vsDirectory::GetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceNameInput* msg = static_cast<vplex::vsDirectory::GetDeviceNameInput*>(state->protoStack.top());
    processOpenGetDeviceNameInput(msg, tag, state);
}

void
processCloseGetDeviceNameInput(vplex::vsDirectory::GetDeviceNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDeviceNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceNameInput* msg = static_cast<vplex::vsDirectory::GetDeviceNameInput*>(state->protoStack.top());
    processCloseGetDeviceNameInput(msg, tag, state);
}

void
processOpenGetDeviceNameOutput(vplex::vsDirectory::GetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceNameOutput* msg = static_cast<vplex::vsDirectory::GetDeviceNameOutput*>(state->protoStack.top());
    processOpenGetDeviceNameOutput(msg, tag, state);
}

void
processCloseGetDeviceNameOutput(vplex::vsDirectory::GetDeviceNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "deviceName") == 0) {
        if (msg->has_devicename()) {
            state->error = "Duplicate of non-repeated field deviceName";
        } else {
            msg->set_devicename(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDeviceNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDeviceNameOutput* msg = static_cast<vplex::vsDirectory::GetDeviceNameOutput*>(state->protoStack.top());
    processCloseGetDeviceNameOutput(msg, tag, state);
}

void
processOpenGetLinkedDevicesInput(vplex::vsDirectory::GetLinkedDevicesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetLinkedDevicesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDevicesInput* msg = static_cast<vplex::vsDirectory::GetLinkedDevicesInput*>(state->protoStack.top());
    processOpenGetLinkedDevicesInput(msg, tag, state);
}

void
processCloseGetLinkedDevicesInput(vplex::vsDirectory::GetLinkedDevicesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetLinkedDevicesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDevicesInput* msg = static_cast<vplex::vsDirectory::GetLinkedDevicesInput*>(state->protoStack.top());
    processCloseGetLinkedDevicesInput(msg, tag, state);
}

void
processOpenGetLinkedDevicesOutput(vplex::vsDirectory::GetLinkedDevicesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "devices") == 0) {
        vplex::vsDirectory::DeviceInfo* proto = msg->add_devices();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetLinkedDevicesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDevicesOutput* msg = static_cast<vplex::vsDirectory::GetLinkedDevicesOutput*>(state->protoStack.top());
    processOpenGetLinkedDevicesOutput(msg, tag, state);
}

void
processCloseGetLinkedDevicesOutput(vplex::vsDirectory::GetLinkedDevicesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetLinkedDevicesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDevicesOutput* msg = static_cast<vplex::vsDirectory::GetLinkedDevicesOutput*>(state->protoStack.top());
    processCloseGetLinkedDevicesOutput(msg, tag, state);
}

void
processOpenGetLoginSessionInput(vplex::vsDirectory::GetLoginSessionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetLoginSessionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLoginSessionInput* msg = static_cast<vplex::vsDirectory::GetLoginSessionInput*>(state->protoStack.top());
    processOpenGetLoginSessionInput(msg, tag, state);
}

void
processCloseGetLoginSessionInput(vplex::vsDirectory::GetLoginSessionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "sessionHandle") == 0) {
        if (msg->has_sessionhandle()) {
            state->error = "Duplicate of non-repeated field sessionHandle";
        } else {
            msg->set_sessionhandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetLoginSessionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLoginSessionInput* msg = static_cast<vplex::vsDirectory::GetLoginSessionInput*>(state->protoStack.top());
    processCloseGetLoginSessionInput(msg, tag, state);
}

void
processOpenGetLoginSessionOutput(vplex::vsDirectory::GetLoginSessionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetLoginSessionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLoginSessionOutput* msg = static_cast<vplex::vsDirectory::GetLoginSessionOutput*>(state->protoStack.top());
    processOpenGetLoginSessionOutput(msg, tag, state);
}

void
processCloseGetLoginSessionOutput(vplex::vsDirectory::GetLoginSessionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "serviceTicket") == 0) {
        if (msg->has_serviceticket()) {
            state->error = "Duplicate of non-repeated field serviceTicket";
        } else {
            msg->set_serviceticket(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetLoginSessionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLoginSessionOutput* msg = static_cast<vplex::vsDirectory::GetLoginSessionOutput*>(state->protoStack.top());
    processCloseGetLoginSessionOutput(msg, tag, state);
}

void
processOpenCreatePersonalStorageNodeInput(vplex::vsDirectory::CreatePersonalStorageNodeInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenCreatePersonalStorageNodeInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreatePersonalStorageNodeInput* msg = static_cast<vplex::vsDirectory::CreatePersonalStorageNodeInput*>(state->protoStack.top());
    processOpenCreatePersonalStorageNodeInput(msg, tag, state);
}

void
processCloseCreatePersonalStorageNodeInput(vplex::vsDirectory::CreatePersonalStorageNodeInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterName") == 0) {
        if (msg->has_clustername()) {
            state->error = "Duplicate of non-repeated field clusterName";
        } else {
            msg->set_clustername(state->lastData);
        }
    } else if (strcmp(tag, "virtDriveCapable") == 0) {
        if (msg->has_virtdrivecapable()) {
            state->error = "Duplicate of non-repeated field virtDriveCapable";
        } else {
            msg->set_virtdrivecapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "mediaServerCapable") == 0) {
        if (msg->has_mediaservercapable()) {
            state->error = "Duplicate of non-repeated field mediaServerCapable";
        } else {
            msg->set_mediaservercapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMediaServerCapable") == 0) {
        if (msg->has_featuremediaservercapable()) {
            state->error = "Duplicate of non-repeated field featureMediaServerCapable";
        } else {
            msg->set_featuremediaservercapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureVirtDriveCapable") == 0) {
        if (msg->has_featurevirtdrivecapable()) {
            state->error = "Duplicate of non-repeated field featureVirtDriveCapable";
        } else {
            msg->set_featurevirtdrivecapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureRemoteFileAccessCapable") == 0) {
        if (msg->has_featureremotefileaccesscapable()) {
            state->error = "Duplicate of non-repeated field featureRemoteFileAccessCapable";
        } else {
            msg->set_featureremotefileaccesscapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureFSDatasetTypeCapable") == 0) {
        if (msg->has_featurefsdatasettypecapable()) {
            state->error = "Duplicate of non-repeated field featureFSDatasetTypeCapable";
        } else {
            msg->set_featurefsdatasettypecapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "featureVirtSyncCapable") == 0) {
        if (msg->has_featurevirtsynccapable()) {
            state->error = "Duplicate of non-repeated field featureVirtSyncCapable";
        } else {
            msg->set_featurevirtsynccapable(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMyStorageServerCapable") == 0) {
        if (msg->has_featuremystorageservercapable()) {
            state->error = "Duplicate of non-repeated field featureMyStorageServerCapable";
        } else {
            msg->set_featuremystorageservercapable(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseCreatePersonalStorageNodeInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreatePersonalStorageNodeInput* msg = static_cast<vplex::vsDirectory::CreatePersonalStorageNodeInput*>(state->protoStack.top());
    processCloseCreatePersonalStorageNodeInput(msg, tag, state);
}

void
processOpenCreatePersonalStorageNodeOutput(vplex::vsDirectory::CreatePersonalStorageNodeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenCreatePersonalStorageNodeOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreatePersonalStorageNodeOutput* msg = static_cast<vplex::vsDirectory::CreatePersonalStorageNodeOutput*>(state->protoStack.top());
    processOpenCreatePersonalStorageNodeOutput(msg, tag, state);
}

void
processCloseCreatePersonalStorageNodeOutput(vplex::vsDirectory::CreatePersonalStorageNodeOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseCreatePersonalStorageNodeOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreatePersonalStorageNodeOutput* msg = static_cast<vplex::vsDirectory::CreatePersonalStorageNodeOutput*>(state->protoStack.top());
    processCloseCreatePersonalStorageNodeOutput(msg, tag, state);
}

void
processOpenGetAsyncNoticeServerInput(vplex::vsDirectory::GetAsyncNoticeServerInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetAsyncNoticeServerInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAsyncNoticeServerInput* msg = static_cast<vplex::vsDirectory::GetAsyncNoticeServerInput*>(state->protoStack.top());
    processOpenGetAsyncNoticeServerInput(msg, tag, state);
}

void
processCloseGetAsyncNoticeServerInput(vplex::vsDirectory::GetAsyncNoticeServerInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetAsyncNoticeServerInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAsyncNoticeServerInput* msg = static_cast<vplex::vsDirectory::GetAsyncNoticeServerInput*>(state->protoStack.top());
    processCloseGetAsyncNoticeServerInput(msg, tag, state);
}

void
processOpenGetAsyncNoticeServerOutput(vplex::vsDirectory::GetAsyncNoticeServerOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetAsyncNoticeServerOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAsyncNoticeServerOutput* msg = static_cast<vplex::vsDirectory::GetAsyncNoticeServerOutput*>(state->protoStack.top());
    processOpenGetAsyncNoticeServerOutput(msg, tag, state);
}

void
processCloseGetAsyncNoticeServerOutput(vplex::vsDirectory::GetAsyncNoticeServerOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "address") == 0) {
        if (msg->has_address()) {
            state->error = "Duplicate of non-repeated field address";
        } else {
            msg->set_address(state->lastData);
        }
    } else if (strcmp(tag, "port") == 0) {
        if (msg->has_port()) {
            state->error = "Duplicate of non-repeated field port";
        } else {
            msg->set_port(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetAsyncNoticeServerOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetAsyncNoticeServerOutput* msg = static_cast<vplex::vsDirectory::GetAsyncNoticeServerOutput*>(state->protoStack.top());
    processCloseGetAsyncNoticeServerOutput(msg, tag, state);
}

void
processOpenUpdateStorageNodeConnectionInput(vplex::vsDirectory::UpdateStorageNodeConnectionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "accessTickets") == 0) {
        vplex::vsDirectory::DeviceAccessTicket* proto = msg->add_accesstickets();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenUpdateStorageNodeConnectionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeConnectionInput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeConnectionInput*>(state->protoStack.top());
    processOpenUpdateStorageNodeConnectionInput(msg, tag, state);
}

void
processCloseUpdateStorageNodeConnectionInput(vplex::vsDirectory::UpdateStorageNodeConnectionInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "reportedName") == 0) {
        if (msg->has_reportedname()) {
            state->error = "Duplicate of non-repeated field reportedName";
        } else {
            msg->set_reportedname(state->lastData);
        }
    } else if (strcmp(tag, "reportedPort") == 0) {
        if (msg->has_reportedport()) {
            state->error = "Duplicate of non-repeated field reportedPort";
        } else {
            msg->set_reportedport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "reportedHTTPPort") == 0) {
        if (msg->has_reportedhttpport()) {
            state->error = "Duplicate of non-repeated field reportedHTTPPort";
        } else {
            msg->set_reportedhttpport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "proxyClusterId") == 0) {
        if (msg->has_proxyclusterid()) {
            state->error = "Duplicate of non-repeated field proxyClusterId";
        } else {
            msg->set_proxyclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "proxyConnectionCookie") == 0) {
        if (msg->has_proxyconnectioncookie()) {
            state->error = "Duplicate of non-repeated field proxyConnectionCookie";
        } else {
            msg->set_proxyconnectioncookie(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "reportedClearFiPort") == 0) {
        if (msg->has_reportedclearfiport()) {
            state->error = "Duplicate of non-repeated field reportedClearFiPort";
        } else {
            msg->set_reportedclearfiport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "reportedClearFiSecurePort") == 0) {
        if (msg->has_reportedclearfisecureport()) {
            state->error = "Duplicate of non-repeated field reportedClearFiSecurePort";
        } else {
            msg->set_reportedclearfisecureport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "accessHandle") == 0) {
        if (msg->has_accesshandle()) {
            state->error = "Duplicate of non-repeated field accessHandle";
        } else {
            msg->set_accesshandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "accessTicket") == 0) {
        if (msg->has_accessticket()) {
            state->error = "Duplicate of non-repeated field accessTicket";
        } else {
            msg->set_accessticket(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateStorageNodeConnectionInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeConnectionInput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeConnectionInput*>(state->protoStack.top());
    processCloseUpdateStorageNodeConnectionInput(msg, tag, state);
}

void
processOpenUpdateStorageNodeConnectionOutput(vplex::vsDirectory::UpdateStorageNodeConnectionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateStorageNodeConnectionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeConnectionOutput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeConnectionOutput*>(state->protoStack.top());
    processOpenUpdateStorageNodeConnectionOutput(msg, tag, state);
}

void
processCloseUpdateStorageNodeConnectionOutput(vplex::vsDirectory::UpdateStorageNodeConnectionOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateStorageNodeConnectionOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeConnectionOutput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeConnectionOutput*>(state->protoStack.top());
    processCloseUpdateStorageNodeConnectionOutput(msg, tag, state);
}

void
processOpenUpdateStorageNodeFeaturesInput(vplex::vsDirectory::UpdateStorageNodeFeaturesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateStorageNodeFeaturesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeFeaturesInput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeFeaturesInput*>(state->protoStack.top());
    processOpenUpdateStorageNodeFeaturesInput(msg, tag, state);
}

void
processCloseUpdateStorageNodeFeaturesInput(vplex::vsDirectory::UpdateStorageNodeFeaturesInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "isVirtDrive") == 0) {
        if (msg->has_isvirtdrive()) {
            state->error = "Duplicate of non-repeated field isVirtDrive";
        } else {
            msg->set_isvirtdrive(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "isMediaServer") == 0) {
        if (msg->has_ismediaserver()) {
            state->error = "Duplicate of non-repeated field isMediaServer";
        } else {
            msg->set_ismediaserver(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMediaServerEnabled") == 0) {
        if (msg->has_featuremediaserverenabled()) {
            state->error = "Duplicate of non-repeated field featureMediaServerEnabled";
        } else {
            msg->set_featuremediaserverenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureVirtDriveEnabled") == 0) {
        if (msg->has_featurevirtdriveenabled()) {
            state->error = "Duplicate of non-repeated field featureVirtDriveEnabled";
        } else {
            msg->set_featurevirtdriveenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureRemoteFileAccessEnabled") == 0) {
        if (msg->has_featureremotefileaccessenabled()) {
            state->error = "Duplicate of non-repeated field featureRemoteFileAccessEnabled";
        } else {
            msg->set_featureremotefileaccessenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureFSDatasetTypeEnabled") == 0) {
        if (msg->has_featurefsdatasettypeenabled()) {
            state->error = "Duplicate of non-repeated field featureFSDatasetTypeEnabled";
        } else {
            msg->set_featurefsdatasettypeenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "featureVirtSyncEnabled") == 0) {
        if (msg->has_featurevirtsyncenabled()) {
            state->error = "Duplicate of non-repeated field featureVirtSyncEnabled";
        } else {
            msg->set_featurevirtsyncenabled(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "featureMyStorageServerEnabled") == 0) {
        if (msg->has_featuremystorageserverenabled()) {
            state->error = "Duplicate of non-repeated field featureMyStorageServerEnabled";
        } else {
            msg->set_featuremystorageserverenabled(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateStorageNodeFeaturesInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeFeaturesInput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeFeaturesInput*>(state->protoStack.top());
    processCloseUpdateStorageNodeFeaturesInput(msg, tag, state);
}

void
processOpenUpdateStorageNodeFeaturesOutput(vplex::vsDirectory::UpdateStorageNodeFeaturesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateStorageNodeFeaturesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeFeaturesOutput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeFeaturesOutput*>(state->protoStack.top());
    processOpenUpdateStorageNodeFeaturesOutput(msg, tag, state);
}

void
processCloseUpdateStorageNodeFeaturesOutput(vplex::vsDirectory::UpdateStorageNodeFeaturesOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateStorageNodeFeaturesOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateStorageNodeFeaturesOutput* msg = static_cast<vplex::vsDirectory::UpdateStorageNodeFeaturesOutput*>(state->protoStack.top());
    processCloseUpdateStorageNodeFeaturesOutput(msg, tag, state);
}

void
processOpenGetPSNDatasetLocationInput(vplex::vsDirectory::GetPSNDatasetLocationInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetPSNDatasetLocationInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetPSNDatasetLocationInput* msg = static_cast<vplex::vsDirectory::GetPSNDatasetLocationInput*>(state->protoStack.top());
    processOpenGetPSNDatasetLocationInput(msg, tag, state);
}

void
processCloseGetPSNDatasetLocationInput(vplex::vsDirectory::GetPSNDatasetLocationInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetUserId") == 0) {
        if (msg->has_datasetuserid()) {
            state->error = "Duplicate of non-repeated field datasetUserId";
        } else {
            msg->set_datasetuserid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetPSNDatasetLocationInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetPSNDatasetLocationInput* msg = static_cast<vplex::vsDirectory::GetPSNDatasetLocationInput*>(state->protoStack.top());
    processCloseGetPSNDatasetLocationInput(msg, tag, state);
}

void
processOpenGetPSNDatasetLocationOutput(vplex::vsDirectory::GetPSNDatasetLocationOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetPSNDatasetLocationOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetPSNDatasetLocationOutput* msg = static_cast<vplex::vsDirectory::GetPSNDatasetLocationOutput*>(state->protoStack.top());
    processOpenGetPSNDatasetLocationOutput(msg, tag, state);
}

void
processCloseGetPSNDatasetLocationOutput(vplex::vsDirectory::GetPSNDatasetLocationOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetPSNDatasetLocationOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetPSNDatasetLocationOutput* msg = static_cast<vplex::vsDirectory::GetPSNDatasetLocationOutput*>(state->protoStack.top());
    processCloseGetPSNDatasetLocationOutput(msg, tag, state);
}

void
processOpenUpdatePSNDatasetStatusInput(vplex::vsDirectory::UpdatePSNDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdatePSNDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdatePSNDatasetStatusInput* msg = static_cast<vplex::vsDirectory::UpdatePSNDatasetStatusInput*>(state->protoStack.top());
    processOpenUpdatePSNDatasetStatusInput(msg, tag, state);
}

void
processCloseUpdatePSNDatasetStatusInput(vplex::vsDirectory::UpdatePSNDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetUserId") == 0) {
        if (msg->has_datasetuserid()) {
            state->error = "Duplicate of non-repeated field datasetUserId";
        } else {
            msg->set_datasetuserid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetSize") == 0) {
        if (msg->has_datasetsize()) {
            state->error = "Duplicate of non-repeated field datasetSize";
        } else {
            msg->set_datasetsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetVersion") == 0) {
        if (msg->has_datasetversion()) {
            state->error = "Duplicate of non-repeated field datasetVersion";
        } else {
            msg->set_datasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdatePSNDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdatePSNDatasetStatusInput* msg = static_cast<vplex::vsDirectory::UpdatePSNDatasetStatusInput*>(state->protoStack.top());
    processCloseUpdatePSNDatasetStatusInput(msg, tag, state);
}

void
processOpenUpdatePSNDatasetStatusOutput(vplex::vsDirectory::UpdatePSNDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdatePSNDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdatePSNDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::UpdatePSNDatasetStatusOutput*>(state->protoStack.top());
    processOpenUpdatePSNDatasetStatusOutput(msg, tag, state);
}

void
processCloseUpdatePSNDatasetStatusOutput(vplex::vsDirectory::UpdatePSNDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdatePSNDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdatePSNDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::UpdatePSNDatasetStatusOutput*>(state->protoStack.top());
    processCloseUpdatePSNDatasetStatusOutput(msg, tag, state);
}

void
processOpenAddUserStorageInput(vplex::vsDirectory::AddUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserStorageInput* msg = static_cast<vplex::vsDirectory::AddUserStorageInput*>(state->protoStack.top());
    processOpenAddUserStorageInput(msg, tag, state);
}

void
processCloseAddUserStorageInput(vplex::vsDirectory::AddUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageName") == 0) {
        if (msg->has_storagename()) {
            state->error = "Duplicate of non-repeated field storageName";
        } else {
            msg->set_storagename(state->lastData);
        }
    } else if (strcmp(tag, "usageLimit") == 0) {
        if (msg->has_usagelimit()) {
            state->error = "Duplicate of non-repeated field usageLimit";
        } else {
            msg->set_usagelimit((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserStorageInput* msg = static_cast<vplex::vsDirectory::AddUserStorageInput*>(state->protoStack.top());
    processCloseAddUserStorageInput(msg, tag, state);
}

void
processOpenAddUserStorageOutput(vplex::vsDirectory::AddUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserStorageOutput* msg = static_cast<vplex::vsDirectory::AddUserStorageOutput*>(state->protoStack.top());
    processOpenAddUserStorageOutput(msg, tag, state);
}

void
processCloseAddUserStorageOutput(vplex::vsDirectory::AddUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAddUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddUserStorageOutput* msg = static_cast<vplex::vsDirectory::AddUserStorageOutput*>(state->protoStack.top());
    processCloseAddUserStorageOutput(msg, tag, state);
}

void
processOpenDeleteUserStorageInput(vplex::vsDirectory::DeleteUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenDeleteUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteUserStorageInput* msg = static_cast<vplex::vsDirectory::DeleteUserStorageInput*>(state->protoStack.top());
    processOpenDeleteUserStorageInput(msg, tag, state);
}

void
processCloseDeleteUserStorageInput(vplex::vsDirectory::DeleteUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseDeleteUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteUserStorageInput* msg = static_cast<vplex::vsDirectory::DeleteUserStorageInput*>(state->protoStack.top());
    processCloseDeleteUserStorageInput(msg, tag, state);
}

void
processOpenDeleteUserStorageOutput(vplex::vsDirectory::DeleteUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenDeleteUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteUserStorageOutput* msg = static_cast<vplex::vsDirectory::DeleteUserStorageOutput*>(state->protoStack.top());
    processOpenDeleteUserStorageOutput(msg, tag, state);
}

void
processCloseDeleteUserStorageOutput(vplex::vsDirectory::DeleteUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseDeleteUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::DeleteUserStorageOutput* msg = static_cast<vplex::vsDirectory::DeleteUserStorageOutput*>(state->protoStack.top());
    processCloseDeleteUserStorageOutput(msg, tag, state);
}

void
processOpenChangeUserStorageNameInput(vplex::vsDirectory::ChangeUserStorageNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenChangeUserStorageNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageNameInput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageNameInput*>(state->protoStack.top());
    processOpenChangeUserStorageNameInput(msg, tag, state);
}

void
processCloseChangeUserStorageNameInput(vplex::vsDirectory::ChangeUserStorageNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "newStorageName") == 0) {
        if (msg->has_newstoragename()) {
            state->error = "Duplicate of non-repeated field newStorageName";
        } else {
            msg->set_newstoragename(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseChangeUserStorageNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageNameInput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageNameInput*>(state->protoStack.top());
    processCloseChangeUserStorageNameInput(msg, tag, state);
}

void
processOpenChangeUserStorageNameOutput(vplex::vsDirectory::ChangeUserStorageNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenChangeUserStorageNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageNameOutput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageNameOutput*>(state->protoStack.top());
    processOpenChangeUserStorageNameOutput(msg, tag, state);
}

void
processCloseChangeUserStorageNameOutput(vplex::vsDirectory::ChangeUserStorageNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseChangeUserStorageNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageNameOutput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageNameOutput*>(state->protoStack.top());
    processCloseChangeUserStorageNameOutput(msg, tag, state);
}

void
processOpenChangeUserStorageQuotaInput(vplex::vsDirectory::ChangeUserStorageQuotaInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenChangeUserStorageQuotaInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageQuotaInput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageQuotaInput*>(state->protoStack.top());
    processOpenChangeUserStorageQuotaInput(msg, tag, state);
}

void
processCloseChangeUserStorageQuotaInput(vplex::vsDirectory::ChangeUserStorageQuotaInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "newLimit") == 0) {
        if (msg->has_newlimit()) {
            state->error = "Duplicate of non-repeated field newLimit";
        } else {
            msg->set_newlimit((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseChangeUserStorageQuotaInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageQuotaInput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageQuotaInput*>(state->protoStack.top());
    processCloseChangeUserStorageQuotaInput(msg, tag, state);
}

void
processOpenChangeUserStorageQuotaOutput(vplex::vsDirectory::ChangeUserStorageQuotaOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenChangeUserStorageQuotaOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageQuotaOutput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageQuotaOutput*>(state->protoStack.top());
    processOpenChangeUserStorageQuotaOutput(msg, tag, state);
}

void
processCloseChangeUserStorageQuotaOutput(vplex::vsDirectory::ChangeUserStorageQuotaOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseChangeUserStorageQuotaOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeUserStorageQuotaOutput* msg = static_cast<vplex::vsDirectory::ChangeUserStorageQuotaOutput*>(state->protoStack.top());
    processCloseChangeUserStorageQuotaOutput(msg, tag, state);
}

void
processOpenListUserStorageInput(vplex::vsDirectory::ListUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenListUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListUserStorageInput* msg = static_cast<vplex::vsDirectory::ListUserStorageInput*>(state->protoStack.top());
    processOpenListUserStorageInput(msg, tag, state);
}

void
processCloseListUserStorageInput(vplex::vsDirectory::ListUserStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseListUserStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListUserStorageInput* msg = static_cast<vplex::vsDirectory::ListUserStorageInput*>(state->protoStack.top());
    processCloseListUserStorageInput(msg, tag, state);
}

void
processOpenListUserStorageOutput(vplex::vsDirectory::ListUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "storageAssignments") == 0) {
        vplex::vsDirectory::UserStorage* proto = msg->add_storageassignments();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenListUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListUserStorageOutput* msg = static_cast<vplex::vsDirectory::ListUserStorageOutput*>(state->protoStack.top());
    processOpenListUserStorageOutput(msg, tag, state);
}

void
processCloseListUserStorageOutput(vplex::vsDirectory::ListUserStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseListUserStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ListUserStorageOutput* msg = static_cast<vplex::vsDirectory::ListUserStorageOutput*>(state->protoStack.top());
    processCloseListUserStorageOutput(msg, tag, state);
}

void
processOpenGetUserStorageAddressInput(vplex::vsDirectory::GetUserStorageAddressInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetUserStorageAddressInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserStorageAddressInput* msg = static_cast<vplex::vsDirectory::GetUserStorageAddressInput*>(state->protoStack.top());
    processOpenGetUserStorageAddressInput(msg, tag, state);
}

void
processCloseGetUserStorageAddressInput(vplex::vsDirectory::GetUserStorageAddressInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetUserStorageAddressInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserStorageAddressInput* msg = static_cast<vplex::vsDirectory::GetUserStorageAddressInput*>(state->protoStack.top());
    processCloseGetUserStorageAddressInput(msg, tag, state);
}

void
processOpenUserStorageAddress(vplex::vsDirectory::UserStorageAddress* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenUserStorageAddressCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UserStorageAddress* msg = static_cast<vplex::vsDirectory::UserStorageAddress*>(state->protoStack.top());
    processOpenUserStorageAddress(msg, tag, state);
}

void
processCloseUserStorageAddress(vplex::vsDirectory::UserStorageAddress* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "direct_address") == 0) {
        if (msg->has_direct_address()) {
            state->error = "Duplicate of non-repeated field direct_address";
        } else {
            msg->set_direct_address(state->lastData);
        }
    } else if (strcmp(tag, "direct_port") == 0) {
        if (msg->has_direct_port()) {
            state->error = "Duplicate of non-repeated field direct_port";
        } else {
            msg->set_direct_port(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "proxy_address") == 0) {
        if (msg->has_proxy_address()) {
            state->error = "Duplicate of non-repeated field proxy_address";
        } else {
            msg->set_proxy_address(state->lastData);
        }
    } else if (strcmp(tag, "proxy_port") == 0) {
        if (msg->has_proxy_port()) {
            state->error = "Duplicate of non-repeated field proxy_port";
        } else {
            msg->set_proxy_port(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "internal_direct_address") == 0) {
        if (msg->has_internal_direct_address()) {
            state->error = "Duplicate of non-repeated field internal_direct_address";
        } else {
            msg->set_internal_direct_address(state->lastData);
        }
    } else if (strcmp(tag, "direct_secure_port") == 0) {
        if (msg->has_direct_secure_port()) {
            state->error = "Duplicate of non-repeated field direct_secure_port";
        } else {
            msg->set_direct_secure_port(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "access_handle") == 0) {
        if (msg->has_access_handle()) {
            state->error = "Duplicate of non-repeated field access_handle";
        } else {
            msg->set_access_handle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "access_ticket") == 0) {
        if (msg->has_access_ticket()) {
            state->error = "Duplicate of non-repeated field access_ticket";
        } else {
            msg->set_access_ticket(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseUserStorageAddressCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UserStorageAddress* msg = static_cast<vplex::vsDirectory::UserStorageAddress*>(state->protoStack.top());
    processCloseUserStorageAddress(msg, tag, state);
}

void
processOpenGetUserStorageAddressOutput(vplex::vsDirectory::GetUserStorageAddressOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetUserStorageAddressOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserStorageAddressOutput* msg = static_cast<vplex::vsDirectory::GetUserStorageAddressOutput*>(state->protoStack.top());
    processOpenGetUserStorageAddressOutput(msg, tag, state);
}

void
processCloseGetUserStorageAddressOutput(vplex::vsDirectory::GetUserStorageAddressOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "directAddress") == 0) {
        if (msg->has_directaddress()) {
            state->error = "Duplicate of non-repeated field directAddress";
        } else {
            msg->set_directaddress(state->lastData);
        }
    } else if (strcmp(tag, "directPort") == 0) {
        if (msg->has_directport()) {
            state->error = "Duplicate of non-repeated field directPort";
        } else {
            msg->set_directport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "proxyAddress") == 0) {
        if (msg->has_proxyaddress()) {
            state->error = "Duplicate of non-repeated field proxyAddress";
        } else {
            msg->set_proxyaddress(state->lastData);
        }
    } else if (strcmp(tag, "proxyPort") == 0) {
        if (msg->has_proxyport()) {
            state->error = "Duplicate of non-repeated field proxyPort";
        } else {
            msg->set_proxyport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "internalDirectAddress") == 0) {
        if (msg->has_internaldirectaddress()) {
            state->error = "Duplicate of non-repeated field internalDirectAddress";
        } else {
            msg->set_internaldirectaddress(state->lastData);
        }
    } else if (strcmp(tag, "directSecurePort") == 0) {
        if (msg->has_directsecureport()) {
            state->error = "Duplicate of non-repeated field directSecurePort";
        } else {
            msg->set_directsecureport(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "accessHandle") == 0) {
        if (msg->has_accesshandle()) {
            state->error = "Duplicate of non-repeated field accessHandle";
        } else {
            msg->set_accesshandle((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "accessTicket") == 0) {
        if (msg->has_accessticket()) {
            state->error = "Duplicate of non-repeated field accessTicket";
        } else {
            msg->set_accessticket(parseBytes(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetUserStorageAddressOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserStorageAddressOutput* msg = static_cast<vplex::vsDirectory::GetUserStorageAddressOutput*>(state->protoStack.top());
    processCloseGetUserStorageAddressOutput(msg, tag, state);
}

void
processOpenAssignUserDatacenterStorageInput(vplex::vsDirectory::AssignUserDatacenterStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenAssignUserDatacenterStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AssignUserDatacenterStorageInput* msg = static_cast<vplex::vsDirectory::AssignUserDatacenterStorageInput*>(state->protoStack.top());
    processOpenAssignUserDatacenterStorageInput(msg, tag, state);
}

void
processCloseAssignUserDatacenterStorageInput(vplex::vsDirectory::AssignUserDatacenterStorageInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "region") == 0) {
        if (msg->has_region()) {
            state->error = "Duplicate of non-repeated field region";
        } else {
            msg->set_region(state->lastData);
        }
    } else if (strcmp(tag, "usageLimit") == 0) {
        if (msg->has_usagelimit()) {
            state->error = "Duplicate of non-repeated field usageLimit";
        } else {
            msg->set_usagelimit((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryStorageId") == 0) {
        if (msg->has_primarystorageid()) {
            state->error = "Duplicate of non-repeated field primaryStorageId";
        } else {
            msg->set_primarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryStorageId") == 0) {
        if (msg->has_secondarystorageid()) {
            state->error = "Duplicate of non-repeated field secondaryStorageId";
        } else {
            msg->set_secondarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAssignUserDatacenterStorageInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AssignUserDatacenterStorageInput* msg = static_cast<vplex::vsDirectory::AssignUserDatacenterStorageInput*>(state->protoStack.top());
    processCloseAssignUserDatacenterStorageInput(msg, tag, state);
}

void
processOpenAssignUserDatacenterStorageOutput(vplex::vsDirectory::AssignUserDatacenterStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "storageAssignment") == 0) {
        if (msg->has_storageassignment()) {
            state->error = "Duplicate of non-repeated field storageAssignment";
        } else {
            vplex::vsDirectory::UserStorage* proto = msg->mutable_storageassignment();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAssignUserDatacenterStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AssignUserDatacenterStorageOutput* msg = static_cast<vplex::vsDirectory::AssignUserDatacenterStorageOutput*>(state->protoStack.top());
    processOpenAssignUserDatacenterStorageOutput(msg, tag, state);
}

void
processCloseAssignUserDatacenterStorageOutput(vplex::vsDirectory::AssignUserDatacenterStorageOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAssignUserDatacenterStorageOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AssignUserDatacenterStorageOutput* msg = static_cast<vplex::vsDirectory::AssignUserDatacenterStorageOutput*>(state->protoStack.top());
    processCloseAssignUserDatacenterStorageOutput(msg, tag, state);
}

void
processOpenGetStorageUnitForDatasetInput(vplex::vsDirectory::GetStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitForDatasetInput* msg = static_cast<vplex::vsDirectory::GetStorageUnitForDatasetInput*>(state->protoStack.top());
    processOpenGetStorageUnitForDatasetInput(msg, tag, state);
}

void
processCloseGetStorageUnitForDatasetInput(vplex::vsDirectory::GetStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitForDatasetInput* msg = static_cast<vplex::vsDirectory::GetStorageUnitForDatasetInput*>(state->protoStack.top());
    processCloseGetStorageUnitForDatasetInput(msg, tag, state);
}

void
processOpenGetStorageUnitForDatasetOutput(vplex::vsDirectory::GetStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitForDatasetOutput* msg = static_cast<vplex::vsDirectory::GetStorageUnitForDatasetOutput*>(state->protoStack.top());
    processOpenGetStorageUnitForDatasetOutput(msg, tag, state);
}

void
processCloseGetStorageUnitForDatasetOutput(vplex::vsDirectory::GetStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "storageClusterId") == 0) {
        if (msg->has_storageclusterid()) {
            state->error = "Duplicate of non-repeated field storageClusterId";
        } else {
            msg->set_storageclusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryStorageId") == 0) {
        if (msg->has_primarystorageid()) {
            state->error = "Duplicate of non-repeated field primaryStorageId";
        } else {
            msg->set_primarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryStorageId") == 0) {
        if (msg->has_secondarystorageid()) {
            state->error = "Duplicate of non-repeated field secondaryStorageId";
        } else {
            msg->set_secondarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitForDatasetOutput* msg = static_cast<vplex::vsDirectory::GetStorageUnitForDatasetOutput*>(state->protoStack.top());
    processCloseGetStorageUnitForDatasetOutput(msg, tag, state);
}

void
processOpenGetStoredDatasetsInput(vplex::vsDirectory::GetStoredDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetStoredDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStoredDatasetsInput* msg = static_cast<vplex::vsDirectory::GetStoredDatasetsInput*>(state->protoStack.top());
    processOpenGetStoredDatasetsInput(msg, tag, state);
}

void
processCloseGetStoredDatasetsInput(vplex::vsDirectory::GetStoredDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageId") == 0) {
        if (msg->has_storageid()) {
            state->error = "Duplicate of non-repeated field storageId";
        } else {
            msg->set_storageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetStoredDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStoredDatasetsInput* msg = static_cast<vplex::vsDirectory::GetStoredDatasetsInput*>(state->protoStack.top());
    processCloseGetStoredDatasetsInput(msg, tag, state);
}

void
processOpenGetStoredDatasetsOutput(vplex::vsDirectory::GetStoredDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "datasets") == 0) {
        vplex::vsDirectory::StoredDataset* proto = msg->add_datasets();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetStoredDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStoredDatasetsOutput* msg = static_cast<vplex::vsDirectory::GetStoredDatasetsOutput*>(state->protoStack.top());
    processOpenGetStoredDatasetsOutput(msg, tag, state);
}

void
processCloseGetStoredDatasetsOutput(vplex::vsDirectory::GetStoredDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetStoredDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStoredDatasetsOutput* msg = static_cast<vplex::vsDirectory::GetStoredDatasetsOutput*>(state->protoStack.top());
    processCloseGetStoredDatasetsOutput(msg, tag, state);
}

void
processOpenGetProxyConnectionForClusterInput(vplex::vsDirectory::GetProxyConnectionForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetProxyConnectionForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetProxyConnectionForClusterInput* msg = static_cast<vplex::vsDirectory::GetProxyConnectionForClusterInput*>(state->protoStack.top());
    processOpenGetProxyConnectionForClusterInput(msg, tag, state);
}

void
processCloseGetProxyConnectionForClusterInput(vplex::vsDirectory::GetProxyConnectionForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetProxyConnectionForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetProxyConnectionForClusterInput* msg = static_cast<vplex::vsDirectory::GetProxyConnectionForClusterInput*>(state->protoStack.top());
    processCloseGetProxyConnectionForClusterInput(msg, tag, state);
}

void
processOpenGetProxyConnectionForClusterOutput(vplex::vsDirectory::GetProxyConnectionForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetProxyConnectionForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetProxyConnectionForClusterOutput* msg = static_cast<vplex::vsDirectory::GetProxyConnectionForClusterOutput*>(state->protoStack.top());
    processOpenGetProxyConnectionForClusterOutput(msg, tag, state);
}

void
processCloseGetProxyConnectionForClusterOutput(vplex::vsDirectory::GetProxyConnectionForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "cookie") == 0) {
        if (msg->has_cookie()) {
            state->error = "Duplicate of non-repeated field cookie";
        } else {
            msg->set_cookie((u32)parseInt32(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetProxyConnectionForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetProxyConnectionForClusterOutput* msg = static_cast<vplex::vsDirectory::GetProxyConnectionForClusterOutput*>(state->protoStack.top());
    processCloseGetProxyConnectionForClusterOutput(msg, tag, state);
}

void
processOpenSendMessageToPSNInput(vplex::vsDirectory::SendMessageToPSNInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenSendMessageToPSNInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SendMessageToPSNInput* msg = static_cast<vplex::vsDirectory::SendMessageToPSNInput*>(state->protoStack.top());
    processOpenSendMessageToPSNInput(msg, tag, state);
}

void
processCloseSendMessageToPSNInput(vplex::vsDirectory::SendMessageToPSNInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "message") == 0) {
        if (msg->has_message()) {
            state->error = "Duplicate of non-repeated field message";
        } else {
            msg->set_message(parseBytes(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseSendMessageToPSNInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SendMessageToPSNInput* msg = static_cast<vplex::vsDirectory::SendMessageToPSNInput*>(state->protoStack.top());
    processCloseSendMessageToPSNInput(msg, tag, state);
}

void
processOpenSendMessageToPSNOutput(vplex::vsDirectory::SendMessageToPSNOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenSendMessageToPSNOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SendMessageToPSNOutput* msg = static_cast<vplex::vsDirectory::SendMessageToPSNOutput*>(state->protoStack.top());
    processOpenSendMessageToPSNOutput(msg, tag, state);
}

void
processCloseSendMessageToPSNOutput(vplex::vsDirectory::SendMessageToPSNOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseSendMessageToPSNOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::SendMessageToPSNOutput* msg = static_cast<vplex::vsDirectory::SendMessageToPSNOutput*>(state->protoStack.top());
    processCloseSendMessageToPSNOutput(msg, tag, state);
}

void
processOpenChangeStorageUnitForDatasetInput(vplex::vsDirectory::ChangeStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenChangeStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageUnitForDatasetInput* msg = static_cast<vplex::vsDirectory::ChangeStorageUnitForDatasetInput*>(state->protoStack.top());
    processOpenChangeStorageUnitForDatasetInput(msg, tag, state);
}

void
processCloseChangeStorageUnitForDatasetInput(vplex::vsDirectory::ChangeStorageUnitForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "currentStorageId") == 0) {
        if (msg->has_currentstorageid()) {
            state->error = "Duplicate of non-repeated field currentStorageId";
        } else {
            msg->set_currentstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "newStorageId") == 0) {
        if (msg->has_newstorageid()) {
            state->error = "Duplicate of non-repeated field newStorageId";
        } else {
            msg->set_newstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseChangeStorageUnitForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageUnitForDatasetInput* msg = static_cast<vplex::vsDirectory::ChangeStorageUnitForDatasetInput*>(state->protoStack.top());
    processCloseChangeStorageUnitForDatasetInput(msg, tag, state);
}

void
processOpenChangeStorageUnitForDatasetOutput(vplex::vsDirectory::ChangeStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenChangeStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageUnitForDatasetOutput* msg = static_cast<vplex::vsDirectory::ChangeStorageUnitForDatasetOutput*>(state->protoStack.top());
    processOpenChangeStorageUnitForDatasetOutput(msg, tag, state);
}

void
processCloseChangeStorageUnitForDatasetOutput(vplex::vsDirectory::ChangeStorageUnitForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "storageId") == 0) {
        if (msg->has_storageid()) {
            state->error = "Duplicate of non-repeated field storageId";
        } else {
            msg->set_storageid((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseChangeStorageUnitForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageUnitForDatasetOutput* msg = static_cast<vplex::vsDirectory::ChangeStorageUnitForDatasetOutput*>(state->protoStack.top());
    processCloseChangeStorageUnitForDatasetOutput(msg, tag, state);
}

void
processOpenCreateStorageClusterInput(vplex::vsDirectory::CreateStorageClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenCreateStorageClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreateStorageClusterInput* msg = static_cast<vplex::vsDirectory::CreateStorageClusterInput*>(state->protoStack.top());
    processOpenCreateStorageClusterInput(msg, tag, state);
}

void
processCloseCreateStorageClusterInput(vplex::vsDirectory::CreateStorageClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterName") == 0) {
        if (msg->has_clustername()) {
            state->error = "Duplicate of non-repeated field clusterName";
        } else {
            msg->set_clustername(state->lastData);
        }
    } else if (strcmp(tag, "clusterType") == 0) {
        if (msg->has_clustertype()) {
            state->error = "Duplicate of non-repeated field clusterType";
        } else {
            msg->set_clustertype(parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "region") == 0) {
        if (msg->has_region()) {
            state->error = "Duplicate of non-repeated field region";
        } else {
            msg->set_region(state->lastData);
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseCreateStorageClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreateStorageClusterInput* msg = static_cast<vplex::vsDirectory::CreateStorageClusterInput*>(state->protoStack.top());
    processCloseCreateStorageClusterInput(msg, tag, state);
}

void
processOpenCreateStorageClusterOutput(vplex::vsDirectory::CreateStorageClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenCreateStorageClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreateStorageClusterOutput* msg = static_cast<vplex::vsDirectory::CreateStorageClusterOutput*>(state->protoStack.top());
    processOpenCreateStorageClusterOutput(msg, tag, state);
}

void
processCloseCreateStorageClusterOutput(vplex::vsDirectory::CreateStorageClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseCreateStorageClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::CreateStorageClusterOutput* msg = static_cast<vplex::vsDirectory::CreateStorageClusterOutput*>(state->protoStack.top());
    processCloseCreateStorageClusterOutput(msg, tag, state);
}

void
processOpenGetMssInstancesForClusterInput(vplex::vsDirectory::GetMssInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetMssInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetMssInstancesForClusterInput* msg = static_cast<vplex::vsDirectory::GetMssInstancesForClusterInput*>(state->protoStack.top());
    processOpenGetMssInstancesForClusterInput(msg, tag, state);
}

void
processCloseGetMssInstancesForClusterInput(vplex::vsDirectory::GetMssInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetMssInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetMssInstancesForClusterInput* msg = static_cast<vplex::vsDirectory::GetMssInstancesForClusterInput*>(state->protoStack.top());
    processCloseGetMssInstancesForClusterInput(msg, tag, state);
}

void
processOpenGetMssInstancesForClusterOutput(vplex::vsDirectory::GetMssInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "mssInstances") == 0) {
        vplex::vsDirectory::MssDetail* proto = msg->add_mssinstances();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetMssInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetMssInstancesForClusterOutput* msg = static_cast<vplex::vsDirectory::GetMssInstancesForClusterOutput*>(state->protoStack.top());
    processOpenGetMssInstancesForClusterOutput(msg, tag, state);
}

void
processCloseGetMssInstancesForClusterOutput(vplex::vsDirectory::GetMssInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetMssInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetMssInstancesForClusterOutput* msg = static_cast<vplex::vsDirectory::GetMssInstancesForClusterOutput*>(state->protoStack.top());
    processCloseGetMssInstancesForClusterOutput(msg, tag, state);
}

void
processOpenGetStorageUnitsForClusterInput(vplex::vsDirectory::GetStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitsForClusterInput* msg = static_cast<vplex::vsDirectory::GetStorageUnitsForClusterInput*>(state->protoStack.top());
    processOpenGetStorageUnitsForClusterInput(msg, tag, state);
}

void
processCloseGetStorageUnitsForClusterInput(vplex::vsDirectory::GetStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitsForClusterInput* msg = static_cast<vplex::vsDirectory::GetStorageUnitsForClusterInput*>(state->protoStack.top());
    processCloseGetStorageUnitsForClusterInput(msg, tag, state);
}

void
processOpenGetStorageUnitsForClusterOutput(vplex::vsDirectory::GetStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "storageUnits") == 0) {
        vplex::vsDirectory::StorageUnitDetail* proto = msg->add_storageunits();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitsForClusterOutput* msg = static_cast<vplex::vsDirectory::GetStorageUnitsForClusterOutput*>(state->protoStack.top());
    processOpenGetStorageUnitsForClusterOutput(msg, tag, state);
}

void
processCloseGetStorageUnitsForClusterOutput(vplex::vsDirectory::GetStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetStorageUnitsForClusterOutput* msg = static_cast<vplex::vsDirectory::GetStorageUnitsForClusterOutput*>(state->protoStack.top());
    processCloseGetStorageUnitsForClusterOutput(msg, tag, state);
}

void
processOpenGetBrsInstancesForClusterInput(vplex::vsDirectory::GetBrsInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetBrsInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsInstancesForClusterInput* msg = static_cast<vplex::vsDirectory::GetBrsInstancesForClusterInput*>(state->protoStack.top());
    processOpenGetBrsInstancesForClusterInput(msg, tag, state);
}

void
processCloseGetBrsInstancesForClusterInput(vplex::vsDirectory::GetBrsInstancesForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetBrsInstancesForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsInstancesForClusterInput* msg = static_cast<vplex::vsDirectory::GetBrsInstancesForClusterInput*>(state->protoStack.top());
    processCloseGetBrsInstancesForClusterInput(msg, tag, state);
}

void
processOpenGetBrsInstancesForClusterOutput(vplex::vsDirectory::GetBrsInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "brsInstances") == 0) {
        vplex::vsDirectory::BrsDetail* proto = msg->add_brsinstances();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetBrsInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsInstancesForClusterOutput* msg = static_cast<vplex::vsDirectory::GetBrsInstancesForClusterOutput*>(state->protoStack.top());
    processOpenGetBrsInstancesForClusterOutput(msg, tag, state);
}

void
processCloseGetBrsInstancesForClusterOutput(vplex::vsDirectory::GetBrsInstancesForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetBrsInstancesForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsInstancesForClusterOutput* msg = static_cast<vplex::vsDirectory::GetBrsInstancesForClusterOutput*>(state->protoStack.top());
    processCloseGetBrsInstancesForClusterOutput(msg, tag, state);
}

void
processOpenGetBrsStorageUnitsForClusterInput(vplex::vsDirectory::GetBrsStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetBrsStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsStorageUnitsForClusterInput* msg = static_cast<vplex::vsDirectory::GetBrsStorageUnitsForClusterInput*>(state->protoStack.top());
    processOpenGetBrsStorageUnitsForClusterInput(msg, tag, state);
}

void
processCloseGetBrsStorageUnitsForClusterInput(vplex::vsDirectory::GetBrsStorageUnitsForClusterInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetBrsStorageUnitsForClusterInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsStorageUnitsForClusterInput* msg = static_cast<vplex::vsDirectory::GetBrsStorageUnitsForClusterInput*>(state->protoStack.top());
    processCloseGetBrsStorageUnitsForClusterInput(msg, tag, state);
}

void
processOpenGetBrsStorageUnitsForClusterOutput(vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "brsStorageUnits") == 0) {
        vplex::vsDirectory::BrsStorageUnitDetail* proto = msg->add_brsstorageunits();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetBrsStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput* msg = static_cast<vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput*>(state->protoStack.top());
    processOpenGetBrsStorageUnitsForClusterOutput(msg, tag, state);
}

void
processCloseGetBrsStorageUnitsForClusterOutput(vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetBrsStorageUnitsForClusterOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput* msg = static_cast<vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput*>(state->protoStack.top());
    processCloseGetBrsStorageUnitsForClusterOutput(msg, tag, state);
}

void
processOpenChangeStorageAssignmentsForDatasetInput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenChangeStorageAssignmentsForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput* msg = static_cast<vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput*>(state->protoStack.top());
    processOpenChangeStorageAssignmentsForDatasetInput(msg, tag, state);
}

void
processCloseChangeStorageAssignmentsForDatasetInput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryStorageId") == 0) {
        if (msg->has_primarystorageid()) {
            state->error = "Duplicate of non-repeated field primaryStorageId";
        } else {
            msg->set_primarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryStorageId") == 0) {
        if (msg->has_secondarystorageid()) {
            state->error = "Duplicate of non-repeated field secondaryStorageId";
        } else {
            msg->set_secondarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseChangeStorageAssignmentsForDatasetInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput* msg = static_cast<vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput*>(state->protoStack.top());
    processCloseChangeStorageAssignmentsForDatasetInput(msg, tag, state);
}

void
processOpenChangeStorageAssignmentsForDatasetOutput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenChangeStorageAssignmentsForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput* msg = static_cast<vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput*>(state->protoStack.top());
    processOpenChangeStorageAssignmentsForDatasetOutput(msg, tag, state);
}

void
processCloseChangeStorageAssignmentsForDatasetOutput(vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseChangeStorageAssignmentsForDatasetOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput* msg = static_cast<vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput*>(state->protoStack.top());
    processCloseChangeStorageAssignmentsForDatasetOutput(msg, tag, state);
}

void
processOpenUpdateDatasetStatusInput(vplex::vsDirectory::UpdateDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenUpdateDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetStatusInput* msg = static_cast<vplex::vsDirectory::UpdateDatasetStatusInput*>(state->protoStack.top());
    processOpenUpdateDatasetStatusInput(msg, tag, state);
}

void
processCloseUpdateDatasetStatusInput(vplex::vsDirectory::UpdateDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "storageId") == 0) {
        if (msg->has_storageid()) {
            state->error = "Duplicate of non-repeated field storageId";
        } else {
            msg->set_storageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetSize") == 0) {
        if (msg->has_datasetsize()) {
            state->error = "Duplicate of non-repeated field datasetSize";
        } else {
            msg->set_datasetsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetVersion") == 0) {
        if (msg->has_datasetversion()) {
            state->error = "Duplicate of non-repeated field datasetVersion";
        } else {
            msg->set_datasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "ansNotificationOff") == 0) {
        if (msg->has_ansnotificationoff()) {
            state->error = "Duplicate of non-repeated field ansNotificationOff";
        } else {
            msg->set_ansnotificationoff(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetStatusInput* msg = static_cast<vplex::vsDirectory::UpdateDatasetStatusInput*>(state->protoStack.top());
    processCloseUpdateDatasetStatusInput(msg, tag, state);
}

void
processOpenUpdateDatasetStatusOutput(vplex::vsDirectory::UpdateDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::UpdateDatasetStatusOutput*>(state->protoStack.top());
    processOpenUpdateDatasetStatusOutput(msg, tag, state);
}

void
processCloseUpdateDatasetStatusOutput(vplex::vsDirectory::UpdateDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::UpdateDatasetStatusOutput*>(state->protoStack.top());
    processCloseUpdateDatasetStatusOutput(msg, tag, state);
}

void
processOpenUpdateDatasetBackupStatusInput(vplex::vsDirectory::UpdateDatasetBackupStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenUpdateDatasetBackupStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetBackupStatusInput* msg = static_cast<vplex::vsDirectory::UpdateDatasetBackupStatusInput*>(state->protoStack.top());
    processOpenUpdateDatasetBackupStatusInput(msg, tag, state);
}

void
processCloseUpdateDatasetBackupStatusInput(vplex::vsDirectory::UpdateDatasetBackupStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetVersion") == 0) {
        if (msg->has_datasetversion()) {
            state->error = "Duplicate of non-repeated field datasetVersion";
        } else {
            msg->set_datasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateDatasetBackupStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetBackupStatusInput* msg = static_cast<vplex::vsDirectory::UpdateDatasetBackupStatusInput*>(state->protoStack.top());
    processCloseUpdateDatasetBackupStatusInput(msg, tag, state);
}

void
processOpenUpdateDatasetBackupStatusOutput(vplex::vsDirectory::UpdateDatasetBackupStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateDatasetBackupStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetBackupStatusOutput* msg = static_cast<vplex::vsDirectory::UpdateDatasetBackupStatusOutput*>(state->protoStack.top());
    processOpenUpdateDatasetBackupStatusOutput(msg, tag, state);
}

void
processCloseUpdateDatasetBackupStatusOutput(vplex::vsDirectory::UpdateDatasetBackupStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateDatasetBackupStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetBackupStatusOutput* msg = static_cast<vplex::vsDirectory::UpdateDatasetBackupStatusOutput*>(state->protoStack.top());
    processCloseUpdateDatasetBackupStatusOutput(msg, tag, state);
}

void
processOpenUpdateDatasetArchiveStatusInput(vplex::vsDirectory::UpdateDatasetArchiveStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenUpdateDatasetArchiveStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetArchiveStatusInput* msg = static_cast<vplex::vsDirectory::UpdateDatasetArchiveStatusInput*>(state->protoStack.top());
    processOpenUpdateDatasetArchiveStatusInput(msg, tag, state);
}

void
processCloseUpdateDatasetArchiveStatusInput(vplex::vsDirectory::UpdateDatasetArchiveStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetVersion") == 0) {
        if (msg->has_datasetversion()) {
            state->error = "Duplicate of non-repeated field datasetVersion";
        } else {
            msg->set_datasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseUpdateDatasetArchiveStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetArchiveStatusInput* msg = static_cast<vplex::vsDirectory::UpdateDatasetArchiveStatusInput*>(state->protoStack.top());
    processCloseUpdateDatasetArchiveStatusInput(msg, tag, state);
}

void
processOpenUpdateDatasetArchiveStatusOutput(vplex::vsDirectory::UpdateDatasetArchiveStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenUpdateDatasetArchiveStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetArchiveStatusOutput* msg = static_cast<vplex::vsDirectory::UpdateDatasetArchiveStatusOutput*>(state->protoStack.top());
    processOpenUpdateDatasetArchiveStatusOutput(msg, tag, state);
}

void
processCloseUpdateDatasetArchiveStatusOutput(vplex::vsDirectory::UpdateDatasetArchiveStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseUpdateDatasetArchiveStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::UpdateDatasetArchiveStatusOutput* msg = static_cast<vplex::vsDirectory::UpdateDatasetArchiveStatusOutput*>(state->protoStack.top());
    processCloseUpdateDatasetArchiveStatusOutput(msg, tag, state);
}

void
processOpenGetDatasetStatusInput(vplex::vsDirectory::GetDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetStatusInput* msg = static_cast<vplex::vsDirectory::GetDatasetStatusInput*>(state->protoStack.top());
    processOpenGetDatasetStatusInput(msg, tag, state);
}

void
processCloseGetDatasetStatusInput(vplex::vsDirectory::GetDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetStatusInput* msg = static_cast<vplex::vsDirectory::GetDatasetStatusInput*>(state->protoStack.top());
    processCloseGetDatasetStatusInput(msg, tag, state);
}

void
processOpenGetDatasetStatusOutput(vplex::vsDirectory::GetDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::GetDatasetStatusOutput*>(state->protoStack.top());
    processOpenGetDatasetStatusOutput(msg, tag, state);
}

void
processCloseGetDatasetStatusOutput(vplex::vsDirectory::GetDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryStorageId") == 0) {
        if (msg->has_primarystorageid()) {
            state->error = "Duplicate of non-repeated field primaryStorageId";
        } else {
            msg->set_primarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryDatasetSize") == 0) {
        if (msg->has_primarydatasetsize()) {
            state->error = "Duplicate of non-repeated field primaryDatasetSize";
        } else {
            msg->set_primarydatasetsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryDatasetVersion") == 0) {
        if (msg->has_primarydatasetversion()) {
            state->error = "Duplicate of non-repeated field primaryDatasetVersion";
        } else {
            msg->set_primarydatasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryStorageId") == 0) {
        if (msg->has_secondarystorageid()) {
            state->error = "Duplicate of non-repeated field secondaryStorageId";
        } else {
            msg->set_secondarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryDatasetSize") == 0) {
        if (msg->has_secondarydatasetsize()) {
            state->error = "Duplicate of non-repeated field secondaryDatasetSize";
        } else {
            msg->set_secondarydatasetsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryDatasetVersion") == 0) {
        if (msg->has_secondarydatasetversion()) {
            state->error = "Duplicate of non-repeated field secondaryDatasetVersion";
        } else {
            msg->set_secondarydatasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetType") == 0) {
        if (msg->has_datasettype()) {
            state->error = "Duplicate of non-repeated field datasetType";
        } else {
            msg->set_datasettype(vplex::vsDirectory::parseDatasetType(state->lastData, state));
        }
    } else if (strcmp(tag, "deleteDataAfter") == 0) {
        if (msg->has_deletedataafter()) {
            state->error = "Duplicate of non-repeated field deleteDataAfter";
        } else {
            msg->set_deletedataafter((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupDatasetVersion") == 0) {
        if (msg->has_backupdatasetversion()) {
            state->error = "Duplicate of non-repeated field backupDatasetVersion";
        } else {
            msg->set_backupdatasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "archiveDatasetVersion") == 0) {
        if (msg->has_archivedatasetversion()) {
            state->error = "Duplicate of non-repeated field archiveDatasetVersion";
        } else {
            msg->set_archivedatasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "suspendedFlag") == 0) {
        if (msg->has_suspendedflag()) {
            state->error = "Duplicate of non-repeated field suspendedFlag";
        } else {
            msg->set_suspendedflag(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::GetDatasetStatusOutput*>(state->protoStack.top());
    processCloseGetDatasetStatusOutput(msg, tag, state);
}

void
processOpenStoreDeviceEventInput(vplex::vsDirectory::StoreDeviceEventInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "eventInfos") == 0) {
        vplex::vsDirectory::EventInfo* proto = msg->add_eventinfos();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenStoreDeviceEventInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StoreDeviceEventInput* msg = static_cast<vplex::vsDirectory::StoreDeviceEventInput*>(state->protoStack.top());
    processOpenStoreDeviceEventInput(msg, tag, state);
}

void
processCloseStoreDeviceEventInput(vplex::vsDirectory::StoreDeviceEventInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "deviceId") == 0) {
        if (msg->has_deviceid()) {
            state->error = "Duplicate of non-repeated field deviceId";
        } else {
            msg->set_deviceid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseStoreDeviceEventInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StoreDeviceEventInput* msg = static_cast<vplex::vsDirectory::StoreDeviceEventInput*>(state->protoStack.top());
    processCloseStoreDeviceEventInput(msg, tag, state);
}

void
processOpenStoreDeviceEventOutput(vplex::vsDirectory::StoreDeviceEventOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenStoreDeviceEventOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StoreDeviceEventOutput* msg = static_cast<vplex::vsDirectory::StoreDeviceEventOutput*>(state->protoStack.top());
    processOpenStoreDeviceEventOutput(msg, tag, state);
}

void
processCloseStoreDeviceEventOutput(vplex::vsDirectory::StoreDeviceEventOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "successCount") == 0) {
        if (msg->has_successcount()) {
            state->error = "Duplicate of non-repeated field successCount";
        } else {
            msg->set_successcount((u32)parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "errorCount") == 0) {
        if (msg->has_errorcount()) {
            state->error = "Duplicate of non-repeated field errorCount";
        } else {
            msg->set_errorcount((u32)parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "nextReportTime") == 0) {
        if (msg->has_nextreporttime()) {
            state->error = "Duplicate of non-repeated field nextReportTime";
        } else {
            msg->set_nextreporttime((u32)parseInt32(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseStoreDeviceEventOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::StoreDeviceEventOutput* msg = static_cast<vplex::vsDirectory::StoreDeviceEventOutput*>(state->protoStack.top());
    processCloseStoreDeviceEventOutput(msg, tag, state);
}

void
processOpenEventInfo(vplex::vsDirectory::EventInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenEventInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::EventInfo* msg = static_cast<vplex::vsDirectory::EventInfo*>(state->protoStack.top());
    processOpenEventInfo(msg, tag, state);
}

void
processCloseEventInfo(vplex::vsDirectory::EventInfo* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "appId") == 0) {
        if (msg->has_appid()) {
            state->error = "Duplicate of non-repeated field appId";
        } else {
            msg->set_appid(state->lastData);
        }
    } else if (strcmp(tag, "eventId") == 0) {
        if (msg->has_eventid()) {
            state->error = "Duplicate of non-repeated field eventId";
        } else {
            msg->set_eventid(state->lastData);
        }
    } else if (strcmp(tag, "startTime") == 0) {
        if (msg->has_starttime()) {
            state->error = "Duplicate of non-repeated field startTime";
        } else {
            msg->set_starttime((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "endTime") == 0) {
        if (msg->has_endtime()) {
            state->error = "Duplicate of non-repeated field endTime";
        } else {
            msg->set_endtime((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "eventCount") == 0) {
        if (msg->has_eventcount()) {
            state->error = "Duplicate of non-repeated field eventCount";
        } else {
            msg->set_eventcount((u32)parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "limitReached") == 0) {
        if (msg->has_limitreached()) {
            state->error = "Duplicate of non-repeated field limitReached";
        } else {
            msg->set_limitreached(parseBool(state->lastData, state));
        }
    } else if (strcmp(tag, "eventInfo") == 0) {
        if (msg->has_eventinfo()) {
            state->error = "Duplicate of non-repeated field eventInfo";
        } else {
            msg->set_eventinfo(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseEventInfoCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::EventInfo* msg = static_cast<vplex::vsDirectory::EventInfo*>(state->protoStack.top());
    processCloseEventInfo(msg, tag, state);
}

void
processOpenGetLinkedDatasetStatusInput(vplex::vsDirectory::GetLinkedDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetLinkedDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDatasetStatusInput* msg = static_cast<vplex::vsDirectory::GetLinkedDatasetStatusInput*>(state->protoStack.top());
    processOpenGetLinkedDatasetStatusInput(msg, tag, state);
}

void
processCloseGetLinkedDatasetStatusInput(vplex::vsDirectory::GetLinkedDatasetStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetLinkedDatasetStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDatasetStatusInput* msg = static_cast<vplex::vsDirectory::GetLinkedDatasetStatusInput*>(state->protoStack.top());
    processCloseGetLinkedDatasetStatusInput(msg, tag, state);
}

void
processOpenGetLinkedDatasetStatusOutput(vplex::vsDirectory::GetLinkedDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetLinkedDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::GetLinkedDatasetStatusOutput*>(state->protoStack.top());
    processOpenGetLinkedDatasetStatusOutput(msg, tag, state);
}

void
processCloseGetLinkedDatasetStatusOutput(vplex::vsDirectory::GetLinkedDatasetStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "linkedDatasetId") == 0) {
        if (msg->has_linkeddatasetid()) {
            state->error = "Duplicate of non-repeated field linkedDatasetId";
        } else {
            msg->set_linkeddatasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryStorageId") == 0) {
        if (msg->has_primarystorageid()) {
            state->error = "Duplicate of non-repeated field primaryStorageId";
        } else {
            msg->set_primarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryDatasetSize") == 0) {
        if (msg->has_primarydatasetsize()) {
            state->error = "Duplicate of non-repeated field primaryDatasetSize";
        } else {
            msg->set_primarydatasetsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "primaryDatasetVersion") == 0) {
        if (msg->has_primarydatasetversion()) {
            state->error = "Duplicate of non-repeated field primaryDatasetVersion";
        } else {
            msg->set_primarydatasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryStorageId") == 0) {
        if (msg->has_secondarystorageid()) {
            state->error = "Duplicate of non-repeated field secondaryStorageId";
        } else {
            msg->set_secondarystorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryDatasetSize") == 0) {
        if (msg->has_secondarydatasetsize()) {
            state->error = "Duplicate of non-repeated field secondaryDatasetSize";
        } else {
            msg->set_secondarydatasetsize((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "secondaryDatasetVersion") == 0) {
        if (msg->has_secondarydatasetversion()) {
            state->error = "Duplicate of non-repeated field secondaryDatasetVersion";
        } else {
            msg->set_secondarydatasetversion((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetType") == 0) {
        if (msg->has_datasettype()) {
            state->error = "Duplicate of non-repeated field datasetType";
        } else {
            msg->set_datasettype(vplex::vsDirectory::parseDatasetType(state->lastData, state));
        }
    } else if (strcmp(tag, "suspendedFlag") == 0) {
        if (msg->has_suspendedflag()) {
            state->error = "Duplicate of non-repeated field suspendedFlag";
        } else {
            msg->set_suspendedflag(parseBool(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetLinkedDatasetStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetLinkedDatasetStatusOutput* msg = static_cast<vplex::vsDirectory::GetLinkedDatasetStatusOutput*>(state->protoStack.top());
    processCloseGetLinkedDatasetStatusOutput(msg, tag, state);
}

void
processOpenGetUserQuotaStatusInput(vplex::vsDirectory::GetUserQuotaStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetUserQuotaStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserQuotaStatusInput* msg = static_cast<vplex::vsDirectory::GetUserQuotaStatusInput*>(state->protoStack.top());
    processOpenGetUserQuotaStatusInput(msg, tag, state);
}

void
processCloseGetUserQuotaStatusInput(vplex::vsDirectory::GetUserQuotaStatusInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetUserQuotaStatusInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserQuotaStatusInput* msg = static_cast<vplex::vsDirectory::GetUserQuotaStatusInput*>(state->protoStack.top());
    processCloseGetUserQuotaStatusInput(msg, tag, state);
}

void
processOpenGetUserQuotaStatusOutput(vplex::vsDirectory::GetUserQuotaStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetUserQuotaStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserQuotaStatusOutput* msg = static_cast<vplex::vsDirectory::GetUserQuotaStatusOutput*>(state->protoStack.top());
    processOpenGetUserQuotaStatusOutput(msg, tag, state);
}

void
processCloseGetUserQuotaStatusOutput(vplex::vsDirectory::GetUserQuotaStatusOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "quotaLimit") == 0) {
        if (msg->has_quotalimit()) {
            state->error = "Duplicate of non-repeated field quotaLimit";
        } else {
            msg->set_quotalimit((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "currentUsage") == 0) {
        if (msg->has_currentusage()) {
            state->error = "Duplicate of non-repeated field currentUsage";
        } else {
            msg->set_currentusage((u64)parseInt64(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetUserQuotaStatusOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUserQuotaStatusOutput* msg = static_cast<vplex::vsDirectory::GetUserQuotaStatusOutput*>(state->protoStack.top());
    processCloseGetUserQuotaStatusOutput(msg, tag, state);
}

void
processOpenGetDatasetsToBackupInput(vplex::vsDirectory::GetDatasetsToBackupInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetDatasetsToBackupInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetsToBackupInput* msg = static_cast<vplex::vsDirectory::GetDatasetsToBackupInput*>(state->protoStack.top());
    processOpenGetDatasetsToBackupInput(msg, tag, state);
}

void
processCloseGetDatasetsToBackupInput(vplex::vsDirectory::GetDatasetsToBackupInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "count") == 0) {
        if (msg->has_count()) {
            state->error = "Duplicate of non-repeated field count";
        } else {
            msg->set_count((u32)parseInt32(state->lastData, state));
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetDatasetsToBackupInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetsToBackupInput* msg = static_cast<vplex::vsDirectory::GetDatasetsToBackupInput*>(state->protoStack.top());
    processCloseGetDatasetsToBackupInput(msg, tag, state);
}

void
processOpenGetDatasetsToBackupOutput(vplex::vsDirectory::GetDatasetsToBackupOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "datasetsToBackup") == 0) {
        vplex::vsDirectory::BackupStatus* proto = msg->add_datasetstobackup();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetDatasetsToBackupOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetsToBackupOutput* msg = static_cast<vplex::vsDirectory::GetDatasetsToBackupOutput*>(state->protoStack.top());
    processOpenGetDatasetsToBackupOutput(msg, tag, state);
}

void
processCloseGetDatasetsToBackupOutput(vplex::vsDirectory::GetDatasetsToBackupOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetDatasetsToBackupOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetDatasetsToBackupOutput* msg = static_cast<vplex::vsDirectory::GetDatasetsToBackupOutput*>(state->protoStack.top());
    processCloseGetDatasetsToBackupOutput(msg, tag, state);
}

void
processOpenGetBRSHostNameInput(vplex::vsDirectory::GetBRSHostNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetBRSHostNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBRSHostNameInput* msg = static_cast<vplex::vsDirectory::GetBRSHostNameInput*>(state->protoStack.top());
    processOpenGetBRSHostNameInput(msg, tag, state);
}

void
processCloseGetBRSHostNameInput(vplex::vsDirectory::GetBRSHostNameInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "backupStorageId") == 0) {
        if (msg->has_backupstorageid()) {
            state->error = "Duplicate of non-repeated field backupStorageId";
        } else {
            msg->set_backupstorageid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetBRSHostNameInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBRSHostNameInput* msg = static_cast<vplex::vsDirectory::GetBRSHostNameInput*>(state->protoStack.top());
    processCloseGetBRSHostNameInput(msg, tag, state);
}

void
processOpenGetBRSHostNameOutput(vplex::vsDirectory::GetBRSHostNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetBRSHostNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBRSHostNameOutput* msg = static_cast<vplex::vsDirectory::GetBRSHostNameOutput*>(state->protoStack.top());
    processOpenGetBRSHostNameOutput(msg, tag, state);
}

void
processCloseGetBRSHostNameOutput(vplex::vsDirectory::GetBRSHostNameOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "hostName") == 0) {
        if (msg->has_hostname()) {
            state->error = "Duplicate of non-repeated field hostName";
        } else {
            msg->set_hostname(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetBRSHostNameOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBRSHostNameOutput* msg = static_cast<vplex::vsDirectory::GetBRSHostNameOutput*>(state->protoStack.top());
    processCloseGetBRSHostNameOutput(msg, tag, state);
}

void
processOpenGetBackupStorageUnitsForBrsInput(vplex::vsDirectory::GetBackupStorageUnitsForBrsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processOpenGetBackupStorageUnitsForBrsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBackupStorageUnitsForBrsInput* msg = static_cast<vplex::vsDirectory::GetBackupStorageUnitsForBrsInput*>(state->protoStack.top());
    processOpenGetBackupStorageUnitsForBrsInput(msg, tag, state);
}

void
processCloseGetBackupStorageUnitsForBrsInput(vplex::vsDirectory::GetBackupStorageUnitsForBrsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "brsId") == 0) {
        if (msg->has_brsid()) {
            state->error = "Duplicate of non-repeated field brsId";
        } else {
            msg->set_brsid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetBackupStorageUnitsForBrsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBackupStorageUnitsForBrsInput* msg = static_cast<vplex::vsDirectory::GetBackupStorageUnitsForBrsInput*>(state->protoStack.top());
    processCloseGetBackupStorageUnitsForBrsInput(msg, tag, state);
}

void
processOpenGetBackupStorageUnitsForBrsOutput(vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenGetBackupStorageUnitsForBrsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput* msg = static_cast<vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput*>(state->protoStack.top());
    processOpenGetBackupStorageUnitsForBrsOutput(msg, tag, state);
}

void
processCloseGetBackupStorageUnitsForBrsOutput(vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "backupStorageIds") == 0) {
        msg->add_backupstorageids((u64)parseInt64(state->lastData, state));
    } else {
        (void)msg;
    }
}
void
processCloseGetBackupStorageUnitsForBrsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput* msg = static_cast<vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput*>(state->protoStack.top());
    processCloseGetBackupStorageUnitsForBrsOutput(msg, tag, state);
}

void
processOpenGetUpdatedDatasetsInput(vplex::vsDirectory::GetUpdatedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "filters") == 0) {
        vplex::vsDirectory::DatasetFilter* proto = msg->add_filters();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetUpdatedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUpdatedDatasetsInput* msg = static_cast<vplex::vsDirectory::GetUpdatedDatasetsInput*>(state->protoStack.top());
    processOpenGetUpdatedDatasetsInput(msg, tag, state);
}

void
processCloseGetUpdatedDatasetsInput(vplex::vsDirectory::GetUpdatedDatasetsInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else if (strcmp(tag, "clusterId") == 0) {
        if (msg->has_clusterid()) {
            state->error = "Duplicate of non-repeated field clusterId";
        } else {
            msg->set_clusterid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "startTime") == 0) {
        if (msg->has_starttime()) {
            state->error = "Duplicate of non-repeated field startTime";
        } else {
            msg->set_starttime((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "endTime") == 0) {
        if (msg->has_endtime()) {
            state->error = "Duplicate of non-repeated field endTime";
        } else {
            msg->set_endtime((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "count") == 0) {
        if (msg->has_count()) {
            state->error = "Duplicate of non-repeated field count";
        } else {
            msg->set_count((u32)parseInt32(state->lastData, state));
        }
    } else {
        (void)msg;
    }
}
void
processCloseGetUpdatedDatasetsInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUpdatedDatasetsInput* msg = static_cast<vplex::vsDirectory::GetUpdatedDatasetsInput*>(state->protoStack.top());
    processCloseGetUpdatedDatasetsInput(msg, tag, state);
}

void
processOpenGetUpdatedDatasetsOutput(vplex::vsDirectory::GetUpdatedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else if (strcmp(tag, "datasets") == 0) {
        vplex::vsDirectory::UpdatedDataset* proto = msg->add_datasets();
        state->protoStack.push(proto);
        state->protoStartDepth.push(state->tagDepth);
    } else {
        (void)msg;
    }
}
void
processOpenGetUpdatedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUpdatedDatasetsOutput* msg = static_cast<vplex::vsDirectory::GetUpdatedDatasetsOutput*>(state->protoStack.top());
    processOpenGetUpdatedDatasetsOutput(msg, tag, state);
}

void
processCloseGetUpdatedDatasetsOutput(vplex::vsDirectory::GetUpdatedDatasetsOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseGetUpdatedDatasetsOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::GetUpdatedDatasetsOutput* msg = static_cast<vplex::vsDirectory::GetUpdatedDatasetsOutput*>(state->protoStack.top());
    processCloseGetUpdatedDatasetsOutput(msg, tag, state);
}

void
processOpenAddDatasetArchiveStorageDeviceInput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput* msg = static_cast<vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput*>(state->protoStack.top());
    processOpenAddDatasetArchiveStorageDeviceInput(msg, tag, state);
}

void
processCloseAddDatasetArchiveStorageDeviceInput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "archiveStorageDeviceId") == 0) {
        msg->add_archivestoragedeviceid((u64)parseInt64(state->lastData, state));
    } else if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseAddDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput* msg = static_cast<vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput*>(state->protoStack.top());
    processCloseAddDatasetArchiveStorageDeviceInput(msg, tag, state);
}

void
processOpenAddDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenAddDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput* msg = static_cast<vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput*>(state->protoStack.top());
    processOpenAddDatasetArchiveStorageDeviceOutput(msg, tag, state);
}

void
processCloseAddDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseAddDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput* msg = static_cast<vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput*>(state->protoStack.top());
    processCloseAddDatasetArchiveStorageDeviceOutput(msg, tag, state);
}

void
processOpenRemoveDatasetArchiveStorageDeviceInput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "session") == 0) {
        if (msg->has_session()) {
            state->error = "Duplicate of non-repeated field session";
        } else {
            vplex::vsDirectory::SessionInfo* proto = msg->mutable_session();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenRemoveDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput* msg = static_cast<vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput*>(state->protoStack.top());
    processOpenRemoveDatasetArchiveStorageDeviceInput(msg, tag, state);
}

void
processCloseRemoveDatasetArchiveStorageDeviceInput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "datasetId") == 0) {
        if (msg->has_datasetid()) {
            state->error = "Duplicate of non-repeated field datasetId";
        } else {
            msg->set_datasetid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "archiveStorageDeviceId") == 0) {
        msg->add_archivestoragedeviceid((u64)parseInt64(state->lastData, state));
    } else if (strcmp(tag, "userId") == 0) {
        if (msg->has_userid()) {
            state->error = "Duplicate of non-repeated field userId";
        } else {
            msg->set_userid((u64)parseInt64(state->lastData, state));
        }
    } else if (strcmp(tag, "version") == 0) {
        if (msg->has_version()) {
            state->error = "Duplicate of non-repeated field version";
        } else {
            msg->set_version(state->lastData);
        }
    } else {
        (void)msg;
    }
}
void
processCloseRemoveDatasetArchiveStorageDeviceInputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput* msg = static_cast<vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput*>(state->protoStack.top());
    processCloseRemoveDatasetArchiveStorageDeviceInput(msg, tag, state);
}

void
processOpenRemoveDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    if (strcmp(tag, "error") == 0) {
        if (msg->has_error()) {
            state->error = "Duplicate of non-repeated field error";
        } else {
            vplex::vsDirectory::Error* proto = msg->mutable_error();
            state->protoStack.push(proto);
            state->protoStartDepth.push(state->tagDepth);
        }
    } else {
        (void)msg;
    }
}
void
processOpenRemoveDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput* msg = static_cast<vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput*>(state->protoStack.top());
    processOpenRemoveDatasetArchiveStorageDeviceOutput(msg, tag, state);
}

void
processCloseRemoveDatasetArchiveStorageDeviceOutput(vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput* msg, const char* tag, ProtoXmlParseStateImpl* state)
{
    {
        (void)msg;
    }
}
void
processCloseRemoveDatasetArchiveStorageDeviceOutputCb(const char* tag, ProtoXmlParseStateImpl* state)
{
    vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput* msg = static_cast<vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput*>(state->protoStack.top());
    processCloseRemoveDatasetArchiveStorageDeviceOutput(msg, tag, state);
}

const char*
vplex::vsDirectory::writeDatasetType(vplex::vsDirectory::DatasetType val)
{
    const char* out;
    if (val == vplex::vsDirectory::USER) {
        out = "USER";
    } else if (val == vplex::vsDirectory::CAMERA) {
        out = "CAMERA";
    } else if (val == vplex::vsDirectory::CLEAR_FI) {
        out = "CLEAR_FI";
    } else if (val == vplex::vsDirectory::CR_UP) {
        out = "CR_UP";
    } else if (val == vplex::vsDirectory::CR_DOWN) {
        out = "CR_DOWN";
    } else if (val == vplex::vsDirectory::PIM) {
        out = "PIM";
    } else if (val == vplex::vsDirectory::CACHE) {
        out = "CACHE";
    } else if (val == vplex::vsDirectory::PIM_CONTACTS) {
        out = "PIM_CONTACTS";
    } else if (val == vplex::vsDirectory::PIM_EVENTS) {
        out = "PIM_EVENTS";
    } else if (val == vplex::vsDirectory::PIM_NOTES) {
        out = "PIM_NOTES";
    } else if (val == vplex::vsDirectory::PIM_TASKS) {
        out = "PIM_TASKS";
    } else if (val == vplex::vsDirectory::PIM_FAVORITES) {
        out = "PIM_FAVORITES";
    } else if (val == vplex::vsDirectory::MEDIA) {
        out = "MEDIA";
    } else if (val == vplex::vsDirectory::MEDIA_METADATA) {
        out = "MEDIA_METADATA";
    } else if (val == vplex::vsDirectory::FS) {
        out = "FS";
    } else if (val == vplex::vsDirectory::VIRT_DRIVE) {
        out = "VIRT_DRIVE";
    } else if (val == vplex::vsDirectory::CLEARFI_MEDIA) {
        out = "CLEARFI_MEDIA";
    } else if (val == vplex::vsDirectory::USER_CONTENT_METADATA) {
        out = "USER_CONTENT_METADATA";
    } else if (val == vplex::vsDirectory::SYNCBOX) {
        out = "SYNCBOX";
    } else if (val == vplex::vsDirectory::SBM) {
        out = "SBM";
    } else if (val == vplex::vsDirectory::SWM) {
        out = "SWM";
    } else {
        out = "";
    }
    return out;
}

const char*
vplex::vsDirectory::writeRouteType(vplex::vsDirectory::RouteType val)
{
    const char* out;
    if (val == vplex::vsDirectory::INVALID_ROUTE) {
        out = "INVALID_ROUTE";
    } else if (val == vplex::vsDirectory::DIRECT_INTERNAL) {
        out = "DIRECT_INTERNAL";
    } else if (val == vplex::vsDirectory::DIRECT_EXTERNAL) {
        out = "DIRECT_EXTERNAL";
    } else if (val == vplex::vsDirectory::PROXY) {
        out = "PROXY";
    } else {
        out = "";
    }
    return out;
}

const char*
vplex::vsDirectory::writeProtocolType(vplex::vsDirectory::ProtocolType val)
{
    const char* out;
    if (val == vplex::vsDirectory::INVALID_PROTOCOL) {
        out = "INVALID_PROTOCOL";
    } else if (val == vplex::vsDirectory::VS) {
        out = "VS";
    } else {
        out = "";
    }
    return out;
}

const char*
vplex::vsDirectory::writePortType(vplex::vsDirectory::PortType val)
{
    const char* out;
    if (val == vplex::vsDirectory::INVALID_PORT) {
        out = "INVALID_PORT";
    } else if (val == vplex::vsDirectory::PORT_VSSI) {
        out = "PORT_VSSI";
    } else if (val == vplex::vsDirectory::PORT_HTTP) {
        out = "PORT_HTTP";
    } else if (val == vplex::vsDirectory::PORT_CLEARFI) {
        out = "PORT_CLEARFI";
    } else if (val == vplex::vsDirectory::PORT_CLEARFI_SECURE) {
        out = "PORT_CLEARFI_SECURE";
    } else {
        out = "";
    }
    return out;
}

const char*
vplex::vsDirectory::writeSubscriptionRole(vplex::vsDirectory::SubscriptionRole val)
{
    const char* out;
    if (val == vplex::vsDirectory::GENERAL) {
        out = "GENERAL";
    } else if (val == vplex::vsDirectory::PRODUCER) {
        out = "PRODUCER";
    } else if (val == vplex::vsDirectory::CONSUMER) {
        out = "CONSUMER";
    } else if (val == vplex::vsDirectory::CLEARFI_SERVER) {
        out = "CLEARFI_SERVER";
    } else if (val == vplex::vsDirectory::CLEARFI_CLIENT) {
        out = "CLEARFI_CLIENT";
    } else if (val == vplex::vsDirectory::WRITER) {
        out = "WRITER";
    } else if (val == vplex::vsDirectory::READER) {
        out = "READER";
    } else {
        out = "";
    }
    return out;
}

void
vplex::vsDirectory::writeAPIVersion(VPLXmlWriter* writer, const vplex::vsDirectory::APIVersion& message)
{
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeError(VPLXmlWriter* writer, const vplex::vsDirectory::Error& message)
{
    if (message.has_errorcode()) {
        char val[15];
        sprintf(val, FMTs32, message.errorcode());
        VPLXmlWriter_InsertSimpleElement(writer, "errorCode", val);
    }
    if (message.has_errordetail()) {
        VPLXmlWriter_InsertSimpleElement(writer, "errorDetail", message.errordetail().c_str());
    }
}

void
vplex::vsDirectory::writeSessionInfo(VPLXmlWriter* writer, const vplex::vsDirectory::SessionInfo& message)
{
    if (message.has_sessionhandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.sessionhandle());
        VPLXmlWriter_InsertSimpleElement(writer, "sessionHandle", val);
    }
    if (message.has_serviceticket()) {
        std::string val = writeBytes(message.serviceticket());
        VPLXmlWriter_InsertSimpleElement(writer, "serviceTicket", val.c_str());
    }
}

void
vplex::vsDirectory::writeETicketData(VPLXmlWriter* writer, const vplex::vsDirectory::ETicketData& message)
{
    if (message.has_eticket()) {
        std::string val = writeBytes(message.eticket());
        VPLXmlWriter_InsertSimpleElement(writer, "eTicket", val.c_str());
    }
    for (int i = 0; i < message.certificate_size(); i++) {
        std::string val = writeBytes(message.certificate(i));
        VPLXmlWriter_InsertSimpleElement(writer, "certificate", val.c_str());
    }
}

void
vplex::vsDirectory::writeLocalization(VPLXmlWriter* writer, const vplex::vsDirectory::Localization& message)
{
    if (message.has_language()) {
        VPLXmlWriter_InsertSimpleElement(writer, "language", message.language().c_str());
    }
    if (message.has_country()) {
        VPLXmlWriter_InsertSimpleElement(writer, "country", message.country().c_str());
    }
    if (message.has_region()) {
        VPLXmlWriter_InsertSimpleElement(writer, "region", message.region().c_str());
    }
}

void
vplex::vsDirectory::writeTitleData(VPLXmlWriter* writer, const vplex::vsDirectory::TitleData& message)
{
    if (message.has_titleid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleId", message.titleid().c_str());
    }
    if (message.has_detailhash()) {
        char val[15];
        sprintf(val, FMTs32, message.detailhash());
        VPLXmlWriter_InsertSimpleElement(writer, "detailHash", val);
    }
    if (message.has_ticketversion()) {
        char val[15];
        sprintf(val, FMTs32, message.ticketversion());
        VPLXmlWriter_InsertSimpleElement(writer, "ticketVersion", val);
    }
    if (message.has_useonlineeticket()) {
        const char* val = (message.useonlineeticket() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "useOnlineETicket", val);
    }
    if (message.has_useofflineeticket()) {
        const char* val = (message.useofflineeticket() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "useOfflineETicket", val);
    }
}

void
vplex::vsDirectory::writeTitleDetail(VPLXmlWriter* writer, const vplex::vsDirectory::TitleDetail& message)
{
    if (message.has_titleid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleId", message.titleid().c_str());
    }
    if (message.has_titleversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleVersion", message.titleversion().c_str());
    }
    if (message.has_tmdurl()) {
        VPLXmlWriter_InsertSimpleElement(writer, "tmdUrl", message.tmdurl().c_str());
    }
    for (int i = 0; i < message.contents_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "contents", 0);
        vplex::vsDirectory::writeContentDetail(writer, message.contents(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_name()) {
        VPLXmlWriter_InsertSimpleElement(writer, "name", message.name().c_str());
    }
    if (message.has_iconurl()) {
        VPLXmlWriter_InsertSimpleElement(writer, "iconUrl", message.iconurl().c_str());
    }
    if (message.has_imageurl()) {
        VPLXmlWriter_InsertSimpleElement(writer, "imageUrl", message.imageurl().c_str());
    }
    if (message.has_publisher()) {
        VPLXmlWriter_InsertSimpleElement(writer, "publisher", message.publisher().c_str());
    }
    if (message.has_genre()) {
        VPLXmlWriter_InsertSimpleElement(writer, "genre", message.genre().c_str());
    }
    if (message.has_contentrating()) {
        VPLXmlWriter_OpenTagV(writer, "contentRating", 0);
        vplex::common::writeContentRating(writer, message.contentrating());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_altcontentrating()) {
        VPLXmlWriter_OpenTagV(writer, "altContentRating", 0);
        vplex::common::writeContentRating(writer, message.altcontentrating());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeContentDetail(VPLXmlWriter* writer, const vplex::vsDirectory::ContentDetail& message)
{
    if (message.has_contentid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "contentId", message.contentid().c_str());
    }
    if (message.has_contentlocation()) {
        VPLXmlWriter_InsertSimpleElement(writer, "contentLocation", message.contentlocation().c_str());
    }
}

void
vplex::vsDirectory::writeSaveData(VPLXmlWriter* writer, const vplex::vsDirectory::SaveData& message)
{
    if (message.has_titleid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleId", message.titleid().c_str());
    }
    if (message.has_savelocation()) {
        VPLXmlWriter_InsertSimpleElement(writer, "saveLocation", message.savelocation().c_str());
    }
}

void
vplex::vsDirectory::writeTitleTicket(VPLXmlWriter* writer, const vplex::vsDirectory::TitleTicket& message)
{
    if (message.has_titleid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleId", message.titleid().c_str());
    }
    if (message.has_eticket()) {
        VPLXmlWriter_OpenTagV(writer, "eTicket", 0);
        vplex::vsDirectory::writeETicketData(writer, message.eticket());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeSubscription(VPLXmlWriter* writer, const vplex::vsDirectory::Subscription& message)
{
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_filter()) {
        VPLXmlWriter_InsertSimpleElement(writer, "filter", message.filter().c_str());
    }
    if (message.has_deviceroot()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceRoot", message.deviceroot().c_str());
    }
    if (message.has_datasetroot()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetRoot", message.datasetroot().c_str());
    }
    if (message.has_uploadok()) {
        const char* val = (message.uploadok() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "uploadOk", val);
    }
    if (message.has_downloadok()) {
        const char* val = (message.downloadok() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "downloadOk", val);
    }
    if (message.has_uploaddeleteok()) {
        const char* val = (message.uploaddeleteok() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "uploadDeleteOk", val);
    }
    if (message.has_downloaddeleteok()) {
        const char* val = (message.downloaddeleteok() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "downloadDeleteOk", val);
    }
    if (message.has_datasetlocation()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetLocation", message.datasetlocation().c_str());
    }
    if (message.has_contenttype()) {
        VPLXmlWriter_InsertSimpleElement(writer, "contentType", message.contenttype().c_str());
    }
    if (message.has_createdfor()) {
        VPLXmlWriter_InsertSimpleElement(writer, "createdFor", message.createdfor().c_str());
    }
    if (message.has_maxsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.maxsize());
        VPLXmlWriter_InsertSimpleElement(writer, "maxSize", val);
    }
    if (message.has_maxfiles()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.maxfiles());
        VPLXmlWriter_InsertSimpleElement(writer, "maxFiles", val);
    }
    if (message.has_creationtime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.creationtime());
        VPLXmlWriter_InsertSimpleElement(writer, "creationTime", val);
    }
}

void
vplex::vsDirectory::writeSyncDirectory(VPLXmlWriter* writer, const vplex::vsDirectory::SyncDirectory& message)
{
    if (message.has_localpath()) {
        VPLXmlWriter_InsertSimpleElement(writer, "localPath", message.localpath().c_str());
    }
    if (message.has_serverpath()) {
        VPLXmlWriter_InsertSimpleElement(writer, "serverPath", message.serverpath().c_str());
    }
    if (message.has_privateflag()) {
        const char* val = (message.privateflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "privateFlag", val);
    }
}

void
vplex::vsDirectory::writeDatasetData(VPLXmlWriter* writer, const vplex::vsDirectory::DatasetData& message)
{
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_detailhash()) {
        char val[15];
        sprintf(val, FMTs32, message.detailhash());
        VPLXmlWriter_InsertSimpleElement(writer, "detailHash", val);
    }
}

void
vplex::vsDirectory::writeDatasetDetail(VPLXmlWriter* writer, const vplex::vsDirectory::DatasetDetail& message)
{
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_contenttype()) {
        VPLXmlWriter_InsertSimpleElement(writer, "contentType", message.contenttype().c_str());
    }
    if (message.has_createdfor()) {
        VPLXmlWriter_InsertSimpleElement(writer, "createdFor", message.createdfor().c_str());
    }
    if (message.has_externalid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "externalId", message.externalid().c_str());
    }
    if (message.has_lastupdated()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.lastupdated());
        VPLXmlWriter_InsertSimpleElement(writer, "lastUpdated", val);
    }
    if (message.has_storageclustername()) {
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterName", message.storageclustername().c_str());
    }
    if (message.has_storageclusterhostname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterHostName", message.storageclusterhostname().c_str());
    }
    if (message.has_storageclusterport()) {
        char val[15];
        sprintf(val, FMTs32, message.storageclusterport());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterPort", val);
    }
    if (message.has_datasetlocation()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetLocation", message.datasetlocation().c_str());
    }
    if (message.has_sizeondisk()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.sizeondisk());
        VPLXmlWriter_InsertSimpleElement(writer, "sizeOnDisk", val);
    }
    if (message.has_datasettype()) {
        const char* val = vplex::vsDirectory::writeDatasetType(message.datasettype());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetType", val);
    }
    if (message.has_linkedto()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.linkedto());
        VPLXmlWriter_InsertSimpleElement(writer, "linkedTo", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_suspendedflag()) {
        const char* val = (message.suspendedflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "suspendedFlag", val);
    }
    if (message.has_primarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryStorageId", val);
    }
    if (message.has_deletedataafter()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deletedataafter());
        VPLXmlWriter_InsertSimpleElement(writer, "deleteDataAfter", val);
    }
    for (int i = 0; i < message.archivestoragedeviceid_size(); i++) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.archivestoragedeviceid(i));
        VPLXmlWriter_InsertSimpleElement(writer, "archiveStorageDeviceId", val);
    }
    if (message.has_displayname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "displayName", message.displayname().c_str());
    }
}

void
vplex::vsDirectory::writeStoredDataset(VPLXmlWriter* writer, const vplex::vsDirectory::StoredDataset& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_datasettype()) {
        const char* val = vplex::vsDirectory::writeDatasetType(message.datasettype());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetType", val);
    }
    if (message.has_dataretentiontime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.dataretentiontime());
        VPLXmlWriter_InsertSimpleElement(writer, "dataRetentionTime", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_primarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryStorageId", val);
    }
    if (message.has_secondarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryStorageId", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
}

void
vplex::vsDirectory::writeDeviceInfo(VPLXmlWriter* writer, const vplex::vsDirectory::DeviceInfo& message)
{
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_deviceclass()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceClass", message.deviceclass().c_str());
    }
    if (message.has_devicename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceName", message.devicename().c_str());
    }
    if (message.has_isacer()) {
        const char* val = (message.isacer() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isAcer", val);
    }
    if (message.has_hascamera()) {
        const char* val = (message.hascamera() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "hasCamera", val);
    }
    if (message.has_osversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "osVersion", message.osversion().c_str());
    }
    if (message.has_protocolversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "protocolVersion", message.protocolversion().c_str());
    }
    if (message.has_isvirtdrive()) {
        const char* val = (message.isvirtdrive() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isVirtDrive", val);
    }
    if (message.has_ismediaserver()) {
        const char* val = (message.ismediaserver() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isMediaServer", val);
    }
    if (message.has_featuremediaservercapable()) {
        const char* val = (message.featuremediaservercapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMediaServerCapable", val);
    }
    if (message.has_featurevirtdrivecapable()) {
        const char* val = (message.featurevirtdrivecapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtDriveCapable", val);
    }
    if (message.has_featureremotefileaccesscapable()) {
        const char* val = (message.featureremotefileaccesscapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureRemoteFileAccessCapable", val);
    }
    if (message.has_featurefsdatasettypecapable()) {
        const char* val = (message.featurefsdatasettypecapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureFSDatasetTypeCapable", val);
    }
    if (message.has_modelnumber()) {
        VPLXmlWriter_InsertSimpleElement(writer, "modelNumber", message.modelnumber().c_str());
    }
    if (message.has_buildinfo()) {
        VPLXmlWriter_InsertSimpleElement(writer, "buildInfo", message.buildinfo().c_str());
    }
    if (message.has_featurevirtsynccapable()) {
        const char* val = (message.featurevirtsynccapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtSyncCapable", val);
    }
    if (message.has_featuremystorageservercapable()) {
        const char* val = (message.featuremystorageservercapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMyStorageServerCapable", val);
    }
}

void
vplex::vsDirectory::writeStorageAccessPort(VPLXmlWriter* writer, const vplex::vsDirectory::StorageAccessPort& message)
{
    if (message.has_porttype()) {
        const char* val = vplex::vsDirectory::writePortType(message.porttype());
        VPLXmlWriter_InsertSimpleElement(writer, "portType", val);
    }
    if (message.has_port()) {
        char val[15];
        sprintf(val, FMTs32, message.port());
        VPLXmlWriter_InsertSimpleElement(writer, "port", val);
    }
}

void
vplex::vsDirectory::writeStorageAccess(VPLXmlWriter* writer, const vplex::vsDirectory::StorageAccess& message)
{
    if (message.has_routetype()) {
        const char* val = vplex::vsDirectory::writeRouteType(message.routetype());
        VPLXmlWriter_InsertSimpleElement(writer, "routeType", val);
    }
    if (message.has_protocol()) {
        const char* val = vplex::vsDirectory::writeProtocolType(message.protocol());
        VPLXmlWriter_InsertSimpleElement(writer, "protocol", val);
    }
    if (message.has_server()) {
        VPLXmlWriter_InsertSimpleElement(writer, "server", message.server().c_str());
    }
    for (int i = 0; i < message.ports_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "ports", 0);
        vplex::vsDirectory::writeStorageAccessPort(writer, message.ports(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeDeviceAccessTicket(VPLXmlWriter* writer, const vplex::vsDirectory::DeviceAccessTicket& message)
{
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_accessticket()) {
        std::string val = writeBytes(message.accessticket());
        VPLXmlWriter_InsertSimpleElement(writer, "accessTicket", val.c_str());
    }
}

void
vplex::vsDirectory::writeUserStorage(VPLXmlWriter* writer, const vplex::vsDirectory::UserStorage& message)
{
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_storagename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "storageName", message.storagename().c_str());
    }
    if (message.has_storagetype()) {
        char val[15];
        sprintf(val, FMTs32, message.storagetype());
        VPLXmlWriter_InsertSimpleElement(writer, "storageType", val);
    }
    if (message.has_usagelimit()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.usagelimit());
        VPLXmlWriter_InsertSimpleElement(writer, "usageLimit", val);
    }
    if (message.has_isvirtdrive()) {
        const char* val = (message.isvirtdrive() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isVirtDrive", val);
    }
    if (message.has_ismediaserver()) {
        const char* val = (message.ismediaserver() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isMediaServer", val);
    }
    if (message.has_accesshandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.accesshandle());
        VPLXmlWriter_InsertSimpleElement(writer, "accessHandle", val);
    }
    if (message.has_accessticket()) {
        std::string val = writeBytes(message.accessticket());
        VPLXmlWriter_InsertSimpleElement(writer, "accessTicket", val.c_str());
    }
    for (int i = 0; i < message.storageaccess_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "storageAccess", 0);
        vplex::vsDirectory::writeStorageAccess(writer, message.storageaccess(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_featuremediaserverenabled()) {
        const char* val = (message.featuremediaserverenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMediaServerEnabled", val);
    }
    if (message.has_featurevirtdriveenabled()) {
        const char* val = (message.featurevirtdriveenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtDriveEnabled", val);
    }
    if (message.has_featureremotefileaccessenabled()) {
        const char* val = (message.featureremotefileaccessenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureRemoteFileAccessEnabled", val);
    }
    if (message.has_featurefsdatasettypeenabled()) {
        const char* val = (message.featurefsdatasettypeenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureFSDatasetTypeEnabled", val);
    }
    if (message.has_devspecaccessticket()) {
        std::string val = writeBytes(message.devspecaccessticket());
        VPLXmlWriter_InsertSimpleElement(writer, "devSpecAccessTicket", val.c_str());
    }
    if (message.has_featureclouddocenabled()) {
        const char* val = (message.featureclouddocenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureCloudDocEnabled", val);
    }
    if (message.has_featurevirtsyncenabled()) {
        const char* val = (message.featurevirtsyncenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtSyncEnabled", val);
    }
    if (message.has_featuremystorageserverenabled()) {
        const char* val = (message.featuremystorageserverenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMyStorageServerEnabled", val);
    }
}

void
vplex::vsDirectory::writeUpdatedDataset(VPLXmlWriter* writer, const vplex::vsDirectory::UpdatedDataset& message)
{
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasettype()) {
        const char* val = vplex::vsDirectory::writeDatasetType(message.datasettype());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetType", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_lastupdated()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.lastupdated());
        VPLXmlWriter_InsertSimpleElement(writer, "lastUpdated", val);
    }
    if (message.has_destdatasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.destdatasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "destDatasetId", val);
    }
    if (message.has_primaryversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primaryversion());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryVersion", val);
    }
}

void
vplex::vsDirectory::writeDatasetFilter(VPLXmlWriter* writer, const vplex::vsDirectory::DatasetFilter& message)
{
    if (message.has_name()) {
        VPLXmlWriter_InsertSimpleElement(writer, "name", message.name().c_str());
    }
    if (message.has_value()) {
        VPLXmlWriter_InsertSimpleElement(writer, "value", message.value().c_str());
    }
}

void
vplex::vsDirectory::writeMssDetail(VPLXmlWriter* writer, const vplex::vsDirectory::MssDetail& message)
{
    if (message.has_mssid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.mssid());
        VPLXmlWriter_InsertSimpleElement(writer, "mssId", val);
    }
    if (message.has_mssname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "mssName", message.mssname().c_str());
    }
    if (message.has_inactiveflag()) {
        const char* val = (message.inactiveflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "inactiveFlag", val);
    }
}

void
vplex::vsDirectory::writeStorageUnitDetail(VPLXmlWriter* writer, const vplex::vsDirectory::StorageUnitDetail& message)
{
    if (message.has_storageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageId", val);
    }
    for (int i = 0; i < message.mssids_size(); i++) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.mssids(i));
        VPLXmlWriter_InsertSimpleElement(writer, "mssIds", val);
    }
    if (message.has_inactiveflag()) {
        const char* val = (message.inactiveflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "inactiveFlag", val);
    }
}

void
vplex::vsDirectory::writeBrsDetail(VPLXmlWriter* writer, const vplex::vsDirectory::BrsDetail& message)
{
    if (message.has_brsid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.brsid());
        VPLXmlWriter_InsertSimpleElement(writer, "brsId", val);
    }
    if (message.has_brsname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "brsName", message.brsname().c_str());
    }
    if (message.has_inactiveflag()) {
        const char* val = (message.inactiveflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "inactiveFlag", val);
    }
}

void
vplex::vsDirectory::writeBrsStorageUnitDetail(VPLXmlWriter* writer, const vplex::vsDirectory::BrsStorageUnitDetail& message)
{
    if (message.has_brsstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.brsstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "brsStorageId", val);
    }
    if (message.has_brsid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.brsid());
        VPLXmlWriter_InsertSimpleElement(writer, "brsId", val);
    }
    if (message.has_inactiveflag()) {
        const char* val = (message.inactiveflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "inactiveFlag", val);
    }
}

void
vplex::vsDirectory::writeBackupStatus(VPLXmlWriter* writer, const vplex::vsDirectory::BackupStatus& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_lastbackuptime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.lastbackuptime());
        VPLXmlWriter_InsertSimpleElement(writer, "lastBackupTime", val);
    }
    if (message.has_lastbackupversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.lastbackupversion());
        VPLXmlWriter_InsertSimpleElement(writer, "lastBackupVersion", val);
    }
    if (message.has_lastarchivetime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.lastarchivetime());
        VPLXmlWriter_InsertSimpleElement(writer, "lastArchiveTime", val);
    }
    if (message.has_lastarchiveversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.lastarchiveversion());
        VPLXmlWriter_InsertSimpleElement(writer, "lastArchiveVersion", val);
    }
}

void
vplex::vsDirectory::writeGetSaveTicketsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSaveTicketsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_certificate()) {
        std::string val = writeBytes(message.certificate());
        VPLXmlWriter_InsertSimpleElement(writer, "certificate", val.c_str());
    }
}

void
vplex::vsDirectory::writeGetSaveTicketsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSaveTicketsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_encryptionticket()) {
        VPLXmlWriter_OpenTagV(writer, "encryptionTicket", 0);
        vplex::vsDirectory::writeETicketData(writer, message.encryptionticket());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_signingticket()) {
        VPLXmlWriter_OpenTagV(writer, "signingTicket", 0);
        vplex::vsDirectory::writeETicketData(writer, message.signingticket());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetSaveDataInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSaveDataInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titleids_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleIds", message.titleids(i).c_str());
    }
}

void
vplex::vsDirectory::writeGetSaveDataOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSaveDataOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.data_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "data", 0);
        vplex::vsDirectory::writeSaveData(writer, message.data(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetOwnedTitlesInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetOwnedTitlesInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_l10n()) {
        VPLXmlWriter_OpenTagV(writer, "l10n", 0);
        vplex::vsDirectory::writeLocalization(writer, message.l10n());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetOwnedTitlesOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetOwnedTitlesOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titledata_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "titleData", 0);
        vplex::vsDirectory::writeTitleData(writer, message.titledata(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetTitlesInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetTitlesInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_l10n()) {
        VPLXmlWriter_OpenTagV(writer, "l10n", 0);
        vplex::vsDirectory::writeLocalization(writer, message.l10n());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titleids_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleIds", message.titleids(i).c_str());
    }
}

void
vplex::vsDirectory::writeGetTitlesOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetTitlesOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titledata_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "titleData", 0);
        vplex::vsDirectory::writeTitleData(writer, message.titledata(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetTitleDetailsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetTitleDetailsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_l10n()) {
        VPLXmlWriter_OpenTagV(writer, "l10n", 0);
        vplex::vsDirectory::writeLocalization(writer, message.l10n());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titleids_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleIds", message.titleids(i).c_str());
    }
}

void
vplex::vsDirectory::writeGetTitleDetailsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetTitleDetailsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titledetails_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "titleDetails", 0);
        vplex::vsDirectory::writeTitleDetail(writer, message.titledetails(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetAttestationChallengeInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetAttestationChallengeInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
}

void
vplex::vsDirectory::writeGetAttestationChallengeOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetAttestationChallengeOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_challenge()) {
        std::string val = writeBytes(message.challenge());
        VPLXmlWriter_InsertSimpleElement(writer, "challenge", val.c_str());
    }
    if (message.has_challengetmd()) {
        std::string val = writeBytes(message.challengetmd());
        VPLXmlWriter_InsertSimpleElement(writer, "challengeTmd", val.c_str());
    }
}

void
vplex::vsDirectory::writeAuthenticateDeviceInput(VPLXmlWriter* writer, const vplex::vsDirectory::AuthenticateDeviceInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_challengeresponse()) {
        std::string val = writeBytes(message.challengeresponse());
        VPLXmlWriter_InsertSimpleElement(writer, "challengeResponse", val.c_str());
    }
    if (message.has_devicecertificate()) {
        std::string val = writeBytes(message.devicecertificate());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceCertificate", val.c_str());
    }
}

void
vplex::vsDirectory::writeAuthenticateDeviceOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AuthenticateDeviceOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetOnlineTitleTicketInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetOnlineTitleTicketInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_devicecertificate()) {
        std::string val = writeBytes(message.devicecertificate());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceCertificate", val.c_str());
    }
    if (message.has_titleid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleId", message.titleid().c_str());
    }
}

void
vplex::vsDirectory::writeGetOnlineTitleTicketOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetOnlineTitleTicketOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_eticket()) {
        VPLXmlWriter_OpenTagV(writer, "eTicket", 0);
        vplex::vsDirectory::writeETicketData(writer, message.eticket());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetOfflineTitleTicketsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetOfflineTitleTicketsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_devicecertificate()) {
        std::string val = writeBytes(message.devicecertificate());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceCertificate", val.c_str());
    }
    for (int i = 0; i < message.titleids_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "titleIds", message.titleids(i).c_str());
    }
}

void
vplex::vsDirectory::writeGetOfflineTitleTicketsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetOfflineTitleTicketsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titletickets_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "titleTickets", 0);
        vplex::vsDirectory::writeTitleTicket(writer, message.titletickets(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeListOwnedDataSetsInput(VPLXmlWriter* writer, const vplex::vsDirectory::ListOwnedDataSetsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeListOwnedDataSetsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::ListOwnedDataSetsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.datasets_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "datasets", 0);
        vplex::vsDirectory::writeDatasetDetail(writer, message.datasets(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetDatasetDetailsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDatasetDetailsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetDatasetDetailsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDatasetDetailsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_datasetdetail()) {
        VPLXmlWriter_OpenTagV(writer, "datasetDetail", 0);
        vplex::vsDirectory::writeDatasetDetail(writer, message.datasetdetail());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeAddDataSetInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddDataSetInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_datasettypeid()) {
        const char* val = vplex::vsDirectory::writeDatasetType(message.datasettypeid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetTypeId", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddDataSetOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddDataSetOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
}

void
vplex::vsDirectory::writeAddCameraDatasetInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddCameraDatasetInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_createdfor()) {
        VPLXmlWriter_InsertSimpleElement(writer, "createdFor", message.createdfor().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddCameraDatasetOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddCameraDatasetOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
}

void
vplex::vsDirectory::writeDeleteDataSetInput(VPLXmlWriter* writer, const vplex::vsDirectory::DeleteDataSetInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeDeleteDataSetOutput(VPLXmlWriter* writer, const vplex::vsDirectory::DeleteDataSetOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeRenameDataSetInput(VPLXmlWriter* writer, const vplex::vsDirectory::RenameDataSetInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_datasetnamenew()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetNameNew", message.datasetnamenew().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeRenameDataSetOutput(VPLXmlWriter* writer, const vplex::vsDirectory::RenameDataSetOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeSetDataSetCacheInput(VPLXmlWriter* writer, const vplex::vsDirectory::SetDataSetCacheInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_cachedatasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.cachedatasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "cacheDatasetId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeSetDataSetCacheOutput(VPLXmlWriter* writer, const vplex::vsDirectory::SetDataSetCacheOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeRemoveDeviceFromSubscriptionsInput(VPLXmlWriter* writer, const vplex::vsDirectory::RemoveDeviceFromSubscriptionsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeRemoveDeviceFromSubscriptionsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::RemoveDeviceFromSubscriptionsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeListSubscriptionsInput(VPLXmlWriter* writer, const vplex::vsDirectory::ListSubscriptionsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeListSubscriptionsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::ListSubscriptionsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.subscriptions_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "subscriptions", 0);
        vplex::vsDirectory::writeSubscription(writer, message.subscriptions(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeAddSubscriptionsInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddSubscriptionsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    for (int i = 0; i < message.subscriptions_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "subscriptions", 0);
        vplex::vsDirectory::writeSubscription(writer, message.subscriptions(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddSubscriptionsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddSubscriptionsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeAddUserDatasetSubscriptionInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddUserDatasetSubscriptionInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_deviceroot()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceRoot", message.deviceroot().c_str());
    }
    if (message.has_filter()) {
        VPLXmlWriter_InsertSimpleElement(writer, "filter", message.filter().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddUserDatasetSubscriptionOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddUserDatasetSubscriptionOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeAddCameraSubscriptionInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddCameraSubscriptionInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_deviceroot()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceRoot", message.deviceroot().c_str());
    }
    if (message.has_filter()) {
        VPLXmlWriter_InsertSimpleElement(writer, "filter", message.filter().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddCameraSubscriptionOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddCameraSubscriptionOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeAddDatasetSubscriptionInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddDatasetSubscriptionInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_datasettype()) {
        const char* val = vplex::vsDirectory::writeDatasetType(message.datasettype());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetType", val);
    }
    if (message.has_role()) {
        const char* val = vplex::vsDirectory::writeSubscriptionRole(message.role());
        VPLXmlWriter_InsertSimpleElement(writer, "role", val);
    }
    if (message.has_deviceroot()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceRoot", message.deviceroot().c_str());
    }
    if (message.has_filter()) {
        VPLXmlWriter_InsertSimpleElement(writer, "filter", message.filter().c_str());
    }
    if (message.has_maxsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.maxsize());
        VPLXmlWriter_InsertSimpleElement(writer, "maxSize", val);
    }
    if (message.has_maxfiles()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.maxfiles());
        VPLXmlWriter_InsertSimpleElement(writer, "maxFiles", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddDatasetSubscriptionOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddDatasetSubscriptionOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeDeleteSubscriptionsInput(VPLXmlWriter* writer, const vplex::vsDirectory::DeleteSubscriptionsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    for (int i = 0; i < message.datasetnames_size(); i++) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetNames", message.datasetnames(i).c_str());
    }
    for (int i = 0; i < message.datasetids_size(); i++) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetids(i));
        VPLXmlWriter_InsertSimpleElement(writer, "datasetIds", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeDeleteSubscriptionsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::DeleteSubscriptionsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUpdateSubscriptionFilterInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateSubscriptionFilterInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_filter()) {
        VPLXmlWriter_InsertSimpleElement(writer, "filter", message.filter().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUpdateSubscriptionFilterOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateSubscriptionFilterOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUpdateSubscriptionLimitsInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateSubscriptionLimitsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_datasetname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "datasetName", message.datasetname().c_str());
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_filter()) {
        VPLXmlWriter_InsertSimpleElement(writer, "filter", message.filter().c_str());
    }
    if (message.has_maxsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.maxsize());
        VPLXmlWriter_InsertSimpleElement(writer, "maxSize", val);
    }
    if (message.has_maxfiles()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.maxfiles());
        VPLXmlWriter_InsertSimpleElement(writer, "maxFiles", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUpdateSubscriptionLimitsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateSubscriptionLimitsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetSubscriptionDetailsForDeviceInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetSubscriptionDetailsForDeviceOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.subscriptions_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "subscriptions", 0);
        vplex::vsDirectory::writeSubscription(writer, message.subscriptions(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetCloudInfoInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetCloudInfoInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetCloudInfoOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetCloudInfoOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.devices_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "devices", 0);
        vplex::vsDirectory::writeDeviceInfo(writer, message.devices(i));
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.datasets_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "datasets", 0);
        vplex::vsDirectory::writeDatasetDetail(writer, message.datasets(i));
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.subscriptions_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "subscriptions", 0);
        vplex::vsDirectory::writeSubscription(writer, message.subscriptions(i));
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.storageassignments_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "storageAssignments", 0);
        vplex::vsDirectory::writeUserStorage(writer, message.storageassignments(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetSubscribedDatasetsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSubscribedDatasetsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_l10n()) {
        VPLXmlWriter_OpenTagV(writer, "l10n", 0);
        vplex::vsDirectory::writeLocalization(writer, message.l10n());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetSubscribedDatasetsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSubscribedDatasetsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.titledata_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "titleData", 0);
        vplex::vsDirectory::writeTitleData(writer, message.titledata(i));
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.datasetdata_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "datasetData", 0);
        vplex::vsDirectory::writeDatasetData(writer, message.datasetdata(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetSubscriptionDetailsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSubscriptionDetailsInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    for (int i = 0; i < message.datasetids_size(); i++) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetids(i));
        VPLXmlWriter_InsertSimpleElement(writer, "datasetIds", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetSubscriptionDetailsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetSubscriptionDetailsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.subscriptions_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "subscriptions", 0);
        vplex::vsDirectory::writeSubscription(writer, message.subscriptions(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeLinkDeviceInput(VPLXmlWriter* writer, const vplex::vsDirectory::LinkDeviceInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_deviceclass()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceClass", message.deviceclass().c_str());
    }
    if (message.has_devicename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceName", message.devicename().c_str());
    }
    if (message.has_isacer()) {
        const char* val = (message.isacer() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isAcer", val);
    }
    if (message.has_hascamera()) {
        const char* val = (message.hascamera() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "hasCamera", val);
    }
    if (message.has_osversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "osVersion", message.osversion().c_str());
    }
    if (message.has_protocolversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "protocolVersion", message.protocolversion().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
    if (message.has_modelnumber()) {
        VPLXmlWriter_InsertSimpleElement(writer, "modelNumber", message.modelnumber().c_str());
    }
    if (message.has_buildinfo()) {
        VPLXmlWriter_InsertSimpleElement(writer, "buildInfo", message.buildinfo().c_str());
    }
}

void
vplex::vsDirectory::writeLinkDeviceOutput(VPLXmlWriter* writer, const vplex::vsDirectory::LinkDeviceOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUnlinkDeviceInput(VPLXmlWriter* writer, const vplex::vsDirectory::UnlinkDeviceInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUnlinkDeviceOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UnlinkDeviceOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeSetDeviceNameInput(VPLXmlWriter* writer, const vplex::vsDirectory::SetDeviceNameInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_devicename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceName", message.devicename().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeSetDeviceNameOutput(VPLXmlWriter* writer, const vplex::vsDirectory::SetDeviceNameOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUpdateDeviceInfoInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDeviceInfoInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_devicename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceName", message.devicename().c_str());
    }
    if (message.has_osversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "osVersion", message.osversion().c_str());
    }
    if (message.has_protocolversion()) {
        VPLXmlWriter_InsertSimpleElement(writer, "protocolVersion", message.protocolversion().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
    if (message.has_modelnumber()) {
        VPLXmlWriter_InsertSimpleElement(writer, "modelNumber", message.modelnumber().c_str());
    }
    if (message.has_buildinfo()) {
        VPLXmlWriter_InsertSimpleElement(writer, "buildInfo", message.buildinfo().c_str());
    }
}

void
vplex::vsDirectory::writeUpdateDeviceInfoOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDeviceInfoOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetDeviceLinkStateInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDeviceLinkStateInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetDeviceLinkStateOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDeviceLinkStateOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_linked()) {
        const char* val = (message.linked() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "linked", val);
    }
}

void
vplex::vsDirectory::writeGetDeviceNameInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDeviceNameInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetDeviceNameOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDeviceNameOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_devicename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "deviceName", message.devicename().c_str());
    }
}

void
vplex::vsDirectory::writeGetLinkedDevicesInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetLinkedDevicesInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetLinkedDevicesOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetLinkedDevicesOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.devices_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "devices", 0);
        vplex::vsDirectory::writeDeviceInfo(writer, message.devices(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetLoginSessionInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetLoginSessionInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_sessionhandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.sessionhandle());
        VPLXmlWriter_InsertSimpleElement(writer, "sessionHandle", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetLoginSessionOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetLoginSessionOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_serviceticket()) {
        std::string val = writeBytes(message.serviceticket());
        VPLXmlWriter_InsertSimpleElement(writer, "serviceTicket", val.c_str());
    }
}

void
vplex::vsDirectory::writeCreatePersonalStorageNodeInput(VPLXmlWriter* writer, const vplex::vsDirectory::CreatePersonalStorageNodeInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_clustername()) {
        VPLXmlWriter_InsertSimpleElement(writer, "clusterName", message.clustername().c_str());
    }
    if (message.has_virtdrivecapable()) {
        const char* val = (message.virtdrivecapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "virtDriveCapable", val);
    }
    if (message.has_mediaservercapable()) {
        const char* val = (message.mediaservercapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "mediaServerCapable", val);
    }
    if (message.has_featuremediaservercapable()) {
        const char* val = (message.featuremediaservercapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMediaServerCapable", val);
    }
    if (message.has_featurevirtdrivecapable()) {
        const char* val = (message.featurevirtdrivecapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtDriveCapable", val);
    }
    if (message.has_featureremotefileaccesscapable()) {
        const char* val = (message.featureremotefileaccesscapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureRemoteFileAccessCapable", val);
    }
    if (message.has_featurefsdatasettypecapable()) {
        const char* val = (message.featurefsdatasettypecapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureFSDatasetTypeCapable", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
    if (message.has_featurevirtsynccapable()) {
        const char* val = (message.featurevirtsynccapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtSyncCapable", val);
    }
    if (message.has_featuremystorageservercapable()) {
        const char* val = (message.featuremystorageservercapable() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMyStorageServerCapable", val);
    }
}

void
vplex::vsDirectory::writeCreatePersonalStorageNodeOutput(VPLXmlWriter* writer, const vplex::vsDirectory::CreatePersonalStorageNodeOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetAsyncNoticeServerInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetAsyncNoticeServerInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetAsyncNoticeServerOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetAsyncNoticeServerOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_address()) {
        VPLXmlWriter_InsertSimpleElement(writer, "address", message.address().c_str());
    }
    if (message.has_port()) {
        char val[15];
        sprintf(val, FMTs32, message.port());
        VPLXmlWriter_InsertSimpleElement(writer, "port", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
}

void
vplex::vsDirectory::writeUpdateStorageNodeConnectionInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateStorageNodeConnectionInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_reportedname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "reportedName", message.reportedname().c_str());
    }
    if (message.has_reportedport()) {
        char val[15];
        sprintf(val, FMTs32, message.reportedport());
        VPLXmlWriter_InsertSimpleElement(writer, "reportedPort", val);
    }
    if (message.has_reportedhttpport()) {
        char val[15];
        sprintf(val, FMTs32, message.reportedhttpport());
        VPLXmlWriter_InsertSimpleElement(writer, "reportedHTTPPort", val);
    }
    if (message.has_proxyclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.proxyclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "proxyClusterId", val);
    }
    if (message.has_proxyconnectioncookie()) {
        char val[15];
        sprintf(val, FMTs32, message.proxyconnectioncookie());
        VPLXmlWriter_InsertSimpleElement(writer, "proxyConnectionCookie", val);
    }
    if (message.has_reportedclearfiport()) {
        char val[15];
        sprintf(val, FMTs32, message.reportedclearfiport());
        VPLXmlWriter_InsertSimpleElement(writer, "reportedClearFiPort", val);
    }
    if (message.has_reportedclearfisecureport()) {
        char val[15];
        sprintf(val, FMTs32, message.reportedclearfisecureport());
        VPLXmlWriter_InsertSimpleElement(writer, "reportedClearFiSecurePort", val);
    }
    if (message.has_accesshandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.accesshandle());
        VPLXmlWriter_InsertSimpleElement(writer, "accessHandle", val);
    }
    if (message.has_accessticket()) {
        std::string val = writeBytes(message.accessticket());
        VPLXmlWriter_InsertSimpleElement(writer, "accessTicket", val.c_str());
    }
    for (int i = 0; i < message.accesstickets_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "accessTickets", 0);
        vplex::vsDirectory::writeDeviceAccessTicket(writer, message.accesstickets(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUpdateStorageNodeConnectionOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateStorageNodeConnectionOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUpdateStorageNodeFeaturesInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateStorageNodeFeaturesInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_isvirtdrive()) {
        const char* val = (message.isvirtdrive() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isVirtDrive", val);
    }
    if (message.has_ismediaserver()) {
        const char* val = (message.ismediaserver() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "isMediaServer", val);
    }
    if (message.has_featuremediaserverenabled()) {
        const char* val = (message.featuremediaserverenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMediaServerEnabled", val);
    }
    if (message.has_featurevirtdriveenabled()) {
        const char* val = (message.featurevirtdriveenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtDriveEnabled", val);
    }
    if (message.has_featureremotefileaccessenabled()) {
        const char* val = (message.featureremotefileaccessenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureRemoteFileAccessEnabled", val);
    }
    if (message.has_featurefsdatasettypeenabled()) {
        const char* val = (message.featurefsdatasettypeenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureFSDatasetTypeEnabled", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
    if (message.has_featurevirtsyncenabled()) {
        const char* val = (message.featurevirtsyncenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureVirtSyncEnabled", val);
    }
    if (message.has_featuremystorageserverenabled()) {
        const char* val = (message.featuremystorageserverenabled() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "featureMyStorageServerEnabled", val);
    }
}

void
vplex::vsDirectory::writeUpdateStorageNodeFeaturesOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateStorageNodeFeaturesOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetPSNDatasetLocationInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetPSNDatasetLocationInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetuserid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetuserid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetUserId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetPSNDatasetLocationOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetPSNDatasetLocationOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
}

void
vplex::vsDirectory::writeUpdatePSNDatasetStatusInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdatePSNDatasetStatusInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetuserid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetuserid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetUserId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_datasetsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetsize());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetSize", val);
    }
    if (message.has_datasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetVersion", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUpdatePSNDatasetStatusOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdatePSNDatasetStatusOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeAddUserStorageInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddUserStorageInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_storagename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "storageName", message.storagename().c_str());
    }
    if (message.has_usagelimit()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.usagelimit());
        VPLXmlWriter_InsertSimpleElement(writer, "usageLimit", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddUserStorageOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddUserStorageOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeDeleteUserStorageInput(VPLXmlWriter* writer, const vplex::vsDirectory::DeleteUserStorageInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeDeleteUserStorageOutput(VPLXmlWriter* writer, const vplex::vsDirectory::DeleteUserStorageOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeChangeUserStorageNameInput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeUserStorageNameInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_newstoragename()) {
        VPLXmlWriter_InsertSimpleElement(writer, "newStorageName", message.newstoragename().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeChangeUserStorageNameOutput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeUserStorageNameOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeChangeUserStorageQuotaInput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeUserStorageQuotaInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_newlimit()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.newlimit());
        VPLXmlWriter_InsertSimpleElement(writer, "newLimit", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeChangeUserStorageQuotaOutput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeUserStorageQuotaOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeListUserStorageInput(VPLXmlWriter* writer, const vplex::vsDirectory::ListUserStorageInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeListUserStorageOutput(VPLXmlWriter* writer, const vplex::vsDirectory::ListUserStorageOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.storageassignments_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "storageAssignments", 0);
        vplex::vsDirectory::writeUserStorage(writer, message.storageassignments(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetUserStorageAddressInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetUserStorageAddressInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUserStorageAddress(VPLXmlWriter* writer, const vplex::vsDirectory::UserStorageAddress& message)
{
    if (message.has_direct_address()) {
        VPLXmlWriter_InsertSimpleElement(writer, "direct_address", message.direct_address().c_str());
    }
    if (message.has_direct_port()) {
        char val[15];
        sprintf(val, FMTs32, message.direct_port());
        VPLXmlWriter_InsertSimpleElement(writer, "direct_port", val);
    }
    if (message.has_proxy_address()) {
        VPLXmlWriter_InsertSimpleElement(writer, "proxy_address", message.proxy_address().c_str());
    }
    if (message.has_proxy_port()) {
        char val[15];
        sprintf(val, FMTs32, message.proxy_port());
        VPLXmlWriter_InsertSimpleElement(writer, "proxy_port", val);
    }
    if (message.has_internal_direct_address()) {
        VPLXmlWriter_InsertSimpleElement(writer, "internal_direct_address", message.internal_direct_address().c_str());
    }
    if (message.has_direct_secure_port()) {
        char val[15];
        sprintf(val, FMTs32, message.direct_secure_port());
        VPLXmlWriter_InsertSimpleElement(writer, "direct_secure_port", val);
    }
    if (message.has_access_handle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.access_handle());
        VPLXmlWriter_InsertSimpleElement(writer, "access_handle", val);
    }
    if (message.has_access_ticket()) {
        std::string val = writeBytes(message.access_ticket());
        VPLXmlWriter_InsertSimpleElement(writer, "access_ticket", val.c_str());
    }
}

void
vplex::vsDirectory::writeGetUserStorageAddressOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetUserStorageAddressOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_directaddress()) {
        VPLXmlWriter_InsertSimpleElement(writer, "directAddress", message.directaddress().c_str());
    }
    if (message.has_directport()) {
        char val[15];
        sprintf(val, FMTs32, message.directport());
        VPLXmlWriter_InsertSimpleElement(writer, "directPort", val);
    }
    if (message.has_proxyaddress()) {
        VPLXmlWriter_InsertSimpleElement(writer, "proxyAddress", message.proxyaddress().c_str());
    }
    if (message.has_proxyport()) {
        char val[15];
        sprintf(val, FMTs32, message.proxyport());
        VPLXmlWriter_InsertSimpleElement(writer, "proxyPort", val);
    }
    if (message.has_internaldirectaddress()) {
        VPLXmlWriter_InsertSimpleElement(writer, "internalDirectAddress", message.internaldirectaddress().c_str());
    }
    if (message.has_directsecureport()) {
        char val[15];
        sprintf(val, FMTs32, message.directsecureport());
        VPLXmlWriter_InsertSimpleElement(writer, "directSecurePort", val);
    }
    if (message.has_accesshandle()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.accesshandle());
        VPLXmlWriter_InsertSimpleElement(writer, "accessHandle", val);
    }
    if (message.has_accessticket()) {
        std::string val = writeBytes(message.accessticket());
        VPLXmlWriter_InsertSimpleElement(writer, "accessTicket", val.c_str());
    }
}

void
vplex::vsDirectory::writeAssignUserDatacenterStorageInput(VPLXmlWriter* writer, const vplex::vsDirectory::AssignUserDatacenterStorageInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_region()) {
        VPLXmlWriter_InsertSimpleElement(writer, "region", message.region().c_str());
    }
    if (message.has_usagelimit()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.usagelimit());
        VPLXmlWriter_InsertSimpleElement(writer, "usageLimit", val);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_primarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryStorageId", val);
    }
    if (message.has_secondarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryStorageId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAssignUserDatacenterStorageOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AssignUserDatacenterStorageOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_storageassignment()) {
        VPLXmlWriter_OpenTagV(writer, "storageAssignment", 0);
        vplex::vsDirectory::writeUserStorage(writer, message.storageassignment());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetStorageUnitForDatasetInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetStorageUnitForDatasetInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetStorageUnitForDatasetOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetStorageUnitForDatasetOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_storageclusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageclusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageClusterId", val);
    }
    if (message.has_primarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryStorageId", val);
    }
    if (message.has_secondarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryStorageId", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
}

void
vplex::vsDirectory::writeGetStoredDatasetsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetStoredDatasetsInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_storageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetStoredDatasetsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetStoredDatasetsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.datasets_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "datasets", 0);
        vplex::vsDirectory::writeStoredDataset(writer, message.datasets(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetProxyConnectionForClusterInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetProxyConnectionForClusterInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetProxyConnectionForClusterOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetProxyConnectionForClusterOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_cookie()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.cookie());
        VPLXmlWriter_InsertSimpleElement(writer, "cookie", val);
    }
}

void
vplex::vsDirectory::writeSendMessageToPSNInput(VPLXmlWriter* writer, const vplex::vsDirectory::SendMessageToPSNInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_message()) {
        std::string val = writeBytes(message.message());
        VPLXmlWriter_InsertSimpleElement(writer, "message", val.c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeSendMessageToPSNOutput(VPLXmlWriter* writer, const vplex::vsDirectory::SendMessageToPSNOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeChangeStorageUnitForDatasetInput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeStorageUnitForDatasetInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_currentstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.currentstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "currentStorageId", val);
    }
    if (message.has_newstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.newstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "newStorageId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeChangeStorageUnitForDatasetOutput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeStorageUnitForDatasetOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_storageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageId", val);
    }
}

void
vplex::vsDirectory::writeCreateStorageClusterInput(VPLXmlWriter* writer, const vplex::vsDirectory::CreateStorageClusterInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_clustername()) {
        VPLXmlWriter_InsertSimpleElement(writer, "clusterName", message.clustername().c_str());
    }
    if (message.has_clustertype()) {
        char val[15];
        sprintf(val, FMTs32, message.clustertype());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterType", val);
    }
    if (message.has_region()) {
        VPLXmlWriter_InsertSimpleElement(writer, "region", message.region().c_str());
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeCreateStorageClusterOutput(VPLXmlWriter* writer, const vplex::vsDirectory::CreateStorageClusterOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetMssInstancesForClusterInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetMssInstancesForClusterInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetMssInstancesForClusterOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetMssInstancesForClusterOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.mssinstances_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "mssInstances", 0);
        vplex::vsDirectory::writeMssDetail(writer, message.mssinstances(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetStorageUnitsForClusterInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetStorageUnitsForClusterInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetStorageUnitsForClusterOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetStorageUnitsForClusterOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.storageunits_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "storageUnits", 0);
        vplex::vsDirectory::writeStorageUnitDetail(writer, message.storageunits(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetBrsInstancesForClusterInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBrsInstancesForClusterInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetBrsInstancesForClusterOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBrsInstancesForClusterOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.brsinstances_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "brsInstances", 0);
        vplex::vsDirectory::writeBrsDetail(writer, message.brsinstances(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetBrsStorageUnitsForClusterInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBrsStorageUnitsForClusterInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetBrsStorageUnitsForClusterOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBrsStorageUnitsForClusterOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.brsstorageunits_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "brsStorageUnits", 0);
        vplex::vsDirectory::writeBrsStorageUnitDetail(writer, message.brsstorageunits(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeChangeStorageAssignmentsForDatasetInput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeStorageAssignmentsForDatasetInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_primarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryStorageId", val);
    }
    if (message.has_secondarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryStorageId", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeChangeStorageAssignmentsForDatasetOutput(VPLXmlWriter* writer, const vplex::vsDirectory::ChangeStorageAssignmentsForDatasetOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUpdateDatasetStatusInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDatasetStatusInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_storageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.storageid());
        VPLXmlWriter_InsertSimpleElement(writer, "storageId", val);
    }
    if (message.has_datasetsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetsize());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetSize", val);
    }
    if (message.has_datasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetVersion", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
    if (message.has_ansnotificationoff()) {
        const char* val = (message.ansnotificationoff() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "ansNotificationOff", val);
    }
}

void
vplex::vsDirectory::writeUpdateDatasetStatusOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDatasetStatusOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUpdateDatasetBackupStatusInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDatasetBackupStatusInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
    if (message.has_datasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetVersion", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUpdateDatasetBackupStatusOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDatasetBackupStatusOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeUpdateDatasetArchiveStatusInput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDatasetArchiveStatusInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
    if (message.has_datasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetVersion", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeUpdateDatasetArchiveStatusOutput(VPLXmlWriter* writer, const vplex::vsDirectory::UpdateDatasetArchiveStatusOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetDatasetStatusInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDatasetStatusInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetDatasetStatusOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDatasetStatusOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_primarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryStorageId", val);
    }
    if (message.has_primarydatasetsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarydatasetsize());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryDatasetSize", val);
    }
    if (message.has_primarydatasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarydatasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryDatasetVersion", val);
    }
    if (message.has_secondarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryStorageId", val);
    }
    if (message.has_secondarydatasetsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarydatasetsize());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryDatasetSize", val);
    }
    if (message.has_secondarydatasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarydatasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryDatasetVersion", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
    if (message.has_datasettype()) {
        const char* val = vplex::vsDirectory::writeDatasetType(message.datasettype());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetType", val);
    }
    if (message.has_deletedataafter()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deletedataafter());
        VPLXmlWriter_InsertSimpleElement(writer, "deleteDataAfter", val);
    }
    if (message.has_backupdatasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupdatasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "backupDatasetVersion", val);
    }
    if (message.has_archivedatasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.archivedatasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "archiveDatasetVersion", val);
    }
    if (message.has_suspendedflag()) {
        const char* val = (message.suspendedflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "suspendedFlag", val);
    }
}

void
vplex::vsDirectory::writeStoreDeviceEventInput(VPLXmlWriter* writer, const vplex::vsDirectory::StoreDeviceEventInput& message)
{
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_deviceid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.deviceid());
        VPLXmlWriter_InsertSimpleElement(writer, "deviceId", val);
    }
    for (int i = 0; i < message.eventinfos_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "eventInfos", 0);
        vplex::vsDirectory::writeEventInfo(writer, message.eventinfos(i));
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeStoreDeviceEventOutput(VPLXmlWriter* writer, const vplex::vsDirectory::StoreDeviceEventOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_successcount()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.successcount());
        VPLXmlWriter_InsertSimpleElement(writer, "successCount", val);
    }
    if (message.has_errorcount()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.errorcount());
        VPLXmlWriter_InsertSimpleElement(writer, "errorCount", val);
    }
    if (message.has_nextreporttime()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.nextreporttime());
        VPLXmlWriter_InsertSimpleElement(writer, "nextReportTime", val);
    }
}

void
vplex::vsDirectory::writeEventInfo(VPLXmlWriter* writer, const vplex::vsDirectory::EventInfo& message)
{
    if (message.has_appid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "appId", message.appid().c_str());
    }
    if (message.has_eventid()) {
        VPLXmlWriter_InsertSimpleElement(writer, "eventId", message.eventid().c_str());
    }
    if (message.has_starttime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.starttime());
        VPLXmlWriter_InsertSimpleElement(writer, "startTime", val);
    }
    if (message.has_endtime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.endtime());
        VPLXmlWriter_InsertSimpleElement(writer, "endTime", val);
    }
    if (message.has_eventcount()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.eventcount());
        VPLXmlWriter_InsertSimpleElement(writer, "eventCount", val);
    }
    if (message.has_limitreached()) {
        const char* val = (message.limitreached() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "limitReached", val);
    }
    if (message.has_eventinfo()) {
        VPLXmlWriter_InsertSimpleElement(writer, "eventInfo", message.eventinfo().c_str());
    }
}

void
vplex::vsDirectory::writeGetLinkedDatasetStatusInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetLinkedDatasetStatusInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetLinkedDatasetStatusOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetLinkedDatasetStatusOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_linkeddatasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.linkeddatasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "linkedDatasetId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_primarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryStorageId", val);
    }
    if (message.has_primarydatasetsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarydatasetsize());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryDatasetSize", val);
    }
    if (message.has_primarydatasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.primarydatasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "primaryDatasetVersion", val);
    }
    if (message.has_secondarystorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarystorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryStorageId", val);
    }
    if (message.has_secondarydatasetsize()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarydatasetsize());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryDatasetSize", val);
    }
    if (message.has_secondarydatasetversion()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.secondarydatasetversion());
        VPLXmlWriter_InsertSimpleElement(writer, "secondaryDatasetVersion", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
    if (message.has_datasettype()) {
        const char* val = vplex::vsDirectory::writeDatasetType(message.datasettype());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetType", val);
    }
    if (message.has_suspendedflag()) {
        const char* val = (message.suspendedflag() ? "true" : "false");
        VPLXmlWriter_InsertSimpleElement(writer, "suspendedFlag", val);
    }
}

void
vplex::vsDirectory::writeGetUserQuotaStatusInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetUserQuotaStatusInput& message)
{
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetUserQuotaStatusOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetUserQuotaStatusOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_quotalimit()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.quotalimit());
        VPLXmlWriter_InsertSimpleElement(writer, "quotaLimit", val);
    }
    if (message.has_currentusage()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.currentusage());
        VPLXmlWriter_InsertSimpleElement(writer, "currentUsage", val);
    }
}

void
vplex::vsDirectory::writeGetDatasetsToBackupInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDatasetsToBackupInput& message)
{
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    if (message.has_count()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.count());
        VPLXmlWriter_InsertSimpleElement(writer, "count", val);
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetDatasetsToBackupOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetDatasetsToBackupOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.datasetstobackup_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "datasetsToBackup", 0);
        vplex::vsDirectory::writeBackupStatus(writer, message.datasetstobackup(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetBRSHostNameInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBRSHostNameInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_backupstorageid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageid());
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetBRSHostNameOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBRSHostNameOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_hostname()) {
        VPLXmlWriter_InsertSimpleElement(writer, "hostName", message.hostname().c_str());
    }
}

void
vplex::vsDirectory::writeGetBackupStorageUnitsForBrsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBackupStorageUnitsForBrsInput& message)
{
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_brsid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.brsid());
        VPLXmlWriter_InsertSimpleElement(writer, "brsId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeGetBackupStorageUnitsForBrsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetBackupStorageUnitsForBrsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.backupstorageids_size(); i++) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.backupstorageids(i));
        VPLXmlWriter_InsertSimpleElement(writer, "backupStorageIds", val);
    }
}

void
vplex::vsDirectory::writeGetUpdatedDatasetsInput(VPLXmlWriter* writer, const vplex::vsDirectory::GetUpdatedDatasetsInput& message)
{
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
    if (message.has_clusterid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.clusterid());
        VPLXmlWriter_InsertSimpleElement(writer, "clusterId", val);
    }
    if (message.has_starttime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.starttime());
        VPLXmlWriter_InsertSimpleElement(writer, "startTime", val);
    }
    if (message.has_endtime()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.endtime());
        VPLXmlWriter_InsertSimpleElement(writer, "endTime", val);
    }
    if (message.has_count()) {
        char val[15];
        sprintf(val, FMTs32, (s32)message.count());
        VPLXmlWriter_InsertSimpleElement(writer, "count", val);
    }
    for (int i = 0; i < message.filters_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "filters", 0);
        vplex::vsDirectory::writeDatasetFilter(writer, message.filters(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeGetUpdatedDatasetsOutput(VPLXmlWriter* writer, const vplex::vsDirectory::GetUpdatedDatasetsOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
    for (int i = 0; i < message.datasets_size(); i++) {
        VPLXmlWriter_OpenTagV(writer, "datasets", 0);
        vplex::vsDirectory::writeUpdatedDataset(writer, message.datasets(i));
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeAddDatasetArchiveStorageDeviceInput(VPLXmlWriter* writer, const vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput& message)
{
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    for (int i = 0; i < message.archivestoragedeviceid_size(); i++) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.archivestoragedeviceid(i));
        VPLXmlWriter_InsertSimpleElement(writer, "archiveStorageDeviceId", val);
    }
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeAddDatasetArchiveStorageDeviceOutput(VPLXmlWriter* writer, const vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

void
vplex::vsDirectory::writeRemoveDatasetArchiveStorageDeviceInput(VPLXmlWriter* writer, const vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput& message)
{
    if (message.has_datasetid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.datasetid());
        VPLXmlWriter_InsertSimpleElement(writer, "datasetId", val);
    }
    for (int i = 0; i < message.archivestoragedeviceid_size(); i++) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.archivestoragedeviceid(i));
        VPLXmlWriter_InsertSimpleElement(writer, "archiveStorageDeviceId", val);
    }
    if (message.has_session()) {
        VPLXmlWriter_OpenTagV(writer, "session", 0);
        vplex::vsDirectory::writeSessionInfo(writer, message.session());
        VPLXmlWriter_CloseTag(writer);
    }
    if (message.has_userid()) {
        char val[30];
        sprintf(val, FMTs64, (s64)message.userid());
        VPLXmlWriter_InsertSimpleElement(writer, "userId", val);
    }
    if (message.has_version()) {
        VPLXmlWriter_InsertSimpleElement(writer, "version", message.version().c_str());
    }
}

void
vplex::vsDirectory::writeRemoveDatasetArchiveStorageDeviceOutput(VPLXmlWriter* writer, const vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput& message)
{
    if (message.has_error()) {
        VPLXmlWriter_OpenTagV(writer, "error", 0);
        vplex::vsDirectory::writeError(writer, message.error());
        VPLXmlWriter_CloseTag(writer);
    }
}

