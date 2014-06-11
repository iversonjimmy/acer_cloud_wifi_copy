/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

// TODO: this file should be renamed to .hpp

#ifndef __VPLEX_VS_DIRECTORY_HPP__
#define __VPLEX_VS_DIRECTORY_HPP__

//============================================================================
/// @file
/// The client API for the VSDS (Virtual Storage Directory Services) Web Service.
/// Please see @ref VirtualStorage.
//============================================================================

#include "vplex_plat.h"
#include "vplex_vs_directory_service_types.pb.h"
#include "vplex_vs_directory_types.h"
#include "vplex_user.h"
#include "vpl_socket.h"
#include "vpl_user.h"

// the value must be -30000 minus the corresponding error code in VsdsStatusCode.java
#define VPL_VS_DIRECTORY_ERR_INVALID_SESSION     -32202
#define VPL_VS_DIRECTORY_ERR_DUPLICATE_CLUSTERID -32230
#define VPL_VS_DIRECTORY_ERR_UNLINKED_SESSION    -32283
#define VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS  -32285
#define VPL_VS_DIRECTORY_ERR_NO_ASD_TO_REMOVE    -32288

// If this is ever disabled (for testing purposes), you will probably need to
// enable VPLEX_SOAP_SUPPORT_BROADON_AUTH in vplex_soap.cpp.
/// The connections will use HTTPS, so the requests and responses will be encrypted.
#define VPL_VS_DIRECTORY_USE_SSL  defined

/** @addtogroup VirtualStorage VPLex VsDirectory
  VPLVsDirectory APIs (part of the VPLex library) provide the platform with
  access to the Virtual Storage infrastructure.

  These APIs are all blocking operations. For uses where non-blocking operations
  are desired, a worker thread can be spawned to wait for the blocking operation
  to complete.
 */
///@{

//============================================================================
/// @addtogroup VirtualStorageStartupAndShutdown Startup and Shutdown
///@{


int VPLVsDirectory_CreateProxy(
                        const char* serverHostname,
                        u16 serverPort,
                        VPLVsDirectory_ProxyHandle_t* proxyHandle_out);
///< Prepares a new Virtual Storage Directory Services proxy.  No network activity occurs until
///< the proxy is used.
///<
///< @note To avoid race conditions, you should ensure that #VPLHttp2::Init() is
///<     called before calling this.
///< @note The \a serverHostname buffer is not copied; you must make sure that the memory stays
///<     valid until you are done with the proxy.  You are allowed to later modify the
///<     \a serverHostname buffer as long as you ensure (via a mutex for example) that
///<     nothing else is using the proxy at the same time.
///< @param[in] serverHostname Internet hostname for the server (buffer is not copied).
///< @param[in] serverPort Internet port for the server.
///< @param[out] proxyHandle_out The handle for the new server proxy.


int VPLVsDirectory_DestroyProxy(
                        VPLVsDirectory_ProxyHandle_t proxyHandle);
///< Reclaims resources used by the Virtual Storage Directory Service proxy.
///< @param[in] proxyHandle The handle for the server proxy to destroy.


///@}
//============================================================================
/// @addtogroup VirtualStorageDirectoryCalls Service Calls
///@{

int VPLVsDirectory_AddCameraDataset(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                    VPLTime_t timeout,
                                    const vplex::vsDirectory::AddCameraDatasetInput& in,
                                    vplex::vsDirectory::AddCameraDatasetOutput& out);

int VPLVsDirectory_AddCameraSubscription(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                         VPLTime_t timeout,
                                         const vplex::vsDirectory::AddCameraSubscriptionInput& in,
                                         vplex::vsDirectory::AddCameraSubscriptionOutput& out);

int VPLVsDirectory_AddDataSet(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddDataSetInput& in,
                              vplex::vsDirectory::AddDataSetOutput& out);
///< Create a new dataset owned by this user with the specified type and
///< user-specified name. The name must be unique for datasets owned by the user.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_DeleteDataSet(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::DeleteDataSetInput& in,
                                 vplex::vsDirectory::DeleteDataSetOutput& out);
///< Delete an existing dataset owned by this user.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_AddSubscriptions(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddSubscriptionsInput& in,
                              vplex::vsDirectory::AddSubscriptionsOutput& out);
///< Add one or more subscriptions to the user's datasets for a given device.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_AddDatasetSubscription(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddDatasetSubscriptionInput& in,
                              vplex::vsDirectory::AddDatasetSubscriptionOutput& out);
///< Add one dataset subscriptions to the user's datasets for a given device.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_DeleteSubscriptions(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::DeleteSubscriptionsInput& in,
                              vplex::vsDirectory::DeleteSubscriptionsOutput& out);
///< Delete one or more of the user's subscriptions for a given device.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_ListOwnedDataSets(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::ListOwnedDataSetsInput& in,
                              vplex::vsDirectory::ListOwnedDataSetsOutput& out);
///< List all datasets owned by the user. If deviceClass isn't specified, return
///< only datasets supported by the deviceClass.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_ListSubscriptions(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::ListSubscriptionsInput& in,
                              vplex::vsDirectory::ListSubscriptionsOutput& out);
///< Get the user's subscriptions for a given device.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetDatasetDetails(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetDatasetDetailsInput& in,
                        vplex::vsDirectory::GetDatasetDetailsOutput& out);
///< Get details about a given dataset.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetSaveTickets(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetSaveTicketsInput& in,
                        vplex::vsDirectory::GetSaveTicketsOutput& out);
///< Get a user's save state encryption and signing (publishing) eTickets.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetSaveData(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetSaveDataInput& in,
                        vplex::vsDirectory::GetSaveDataOutput& out);
///< Get the user's save state location information for a given title.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetSubscribedDatasets(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetSubscribedDatasetsInput& in,
                        vplex::vsDirectory::GetSubscribedDatasetsOutput& out);

int VPLVsDirectory_GetSubscriptionDetailsForDevice(
                       VPLVsDirectory_ProxyHandle_t proxyHandle,
                       VPLTime_t timeout,
                       const vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput& in,
                       vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput& out);

int VPLVsDirectory_GetOwnedTitles(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetOwnedTitlesInput& in,
                        vplex::vsDirectory::GetOwnedTitlesOutput& out);
///< Get the summary data of all titles owned by a single user.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetTitles(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetTitlesInput& in,
                        vplex::vsDirectory::GetTitlesOutput& out);
///< Get summary data for a list of titles.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetTitleDetails(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetTitleDetailsInput& in,
                        vplex::vsDirectory::GetTitleDetailsOutput& out);
///< Get detailed, user-independent detailed title information for a list of
///< titles, including the title's TMD and  content location information.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetAttestationChallenge(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetAttestationChallengeInput& in,
                        vplex::vsDirectory::GetAttestationChallengeOutput& out);
///< Get a device attestation challenge.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_AuthenticateDevice(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::AuthenticateDeviceInput& in,
                        vplex::vsDirectory::AuthenticateDeviceOutput& out);
///< Authenticate this device using the answer from the attestation challenge.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_LinkDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::LinkDeviceInput& in,
                              vplex::vsDirectory::LinkDeviceOutput& out);
///< Link a specific device to a user so it may be assigned subscriptions.
///< A user may have any number of linked devices, and a device may be linked
///< to any number of users.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_UnlinkDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                VPLTime_t timeout,
                                const vplex::vsDirectory::UnlinkDeviceInput& in,
                                vplex::vsDirectory::UnlinkDeviceOutput& out);
///< Remove the link between device and user. All subscriptions by the user
///< will also be deleted.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetLinkedDevices(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                    VPLTime_t timeout,
                                    const vplex::vsDirectory::GetLinkedDevicesInput& in,
                                    vplex::vsDirectory::GetLinkedDevicesOutput& out);
///< Get a list of all devices linked to the user via LinkDevice
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_SetDeviceName(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::SetDeviceNameInput& in,
                                 vplex::vsDirectory::SetDeviceNameOutput& out);
///< Set the name of a specific device of a user
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetOnlineTitleTicket(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetOnlineTitleTicketInput& in,
                        vplex::vsDirectory::GetOnlineTitleTicketOutput& out);
///< Get the one-time use online title eTicket for a given title after
///< completing device attestation.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetOfflineTitleTickets(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetOfflineTitleTicketsInput& in,
                        vplex::vsDirectory::GetOfflineTitleTicketsOutput& out);
///< Get the permanent offline title eTickets for a list of titles.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetLoginSession(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetLoginSessionInput& in,
                        vplex::vsDirectory::GetLoginSessionOutput& out);
///< Get the login session information for given session.
///< Used only by the StorageNode so it can authenticate user requests.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetAsyncNoticeServer(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetAsyncNoticeServerInput& in,
                        vplex::vsDirectory::GetAsyncNoticeServerOutput& out);
///< Get the server address for the Asynchronous Notification Server (ANS) the
///< Personal Storage Node should connect to. 
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_CreatePersonalStorageNode(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::CreatePersonalStorageNodeInput& in,
                        vplex::vsDirectory::CreatePersonalStorageNodeOutput& out);
///< Attempt to create a new personal storage node
///< Used only by the StorageNode on install so it is recognized by the
///< infrastructure.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_AddUserStorage(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::AddUserStorageInput& in,
                        vplex::vsDirectory::AddUserStorageOutput& out);
///< Add a storage cluster/user link so the user may create datasets there.
///< Used only by the StorageNode when a user is granted storage access.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_ListUserStorage(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::ListUserStorageInput& in,
                        vplex::vsDirectory::ListUserStorageOutput& out);
///< List storage clusters linked to a user.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_DeleteUserStorage(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::DeleteUserStorageInput& in,
                                 vplex::vsDirectory::DeleteUserStorageOutput& out);

int VPLVsDirectory_UpdateStorageNodeFeatures(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateStorageNodeFeaturesInput& in,
                        vplex::vsDirectory::UpdateStorageNodeFeaturesOutput& out);
///< Update the media server, virt drive enable bits of the storage node.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_UpdateStorageNodeConnection(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateStorageNodeConnectionInput& in,
                        vplex::vsDirectory::UpdateStorageNodeConnectionOutput& out);
///< Update the direct-connect hostname/IP and port for a storage node.
///< Used only by the StorageNode to keep the infrastructure updated.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.


int VPLVsDirectory_GetPSNDatasetLocation(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetPSNDatasetLocationInput& in,
                        vplex::vsDirectory::GetPSNDatasetLocationOutput& out);
///< Get the storage units and cluster ID for a given PSN-hosted dataset.
///< Used only by the StorageNode to verify it is the host of a dataset.
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetUserStorageAddress(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetUserStorageAddressInput& in,
                        vplex::vsDirectory::GetUserStorageAddressOutput& out);

int VPLVsDirectory_UpdatePSNDatasetStatus(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdatePSNDatasetStatusInput& in,
                        vplex::vsDirectory::UpdatePSNDatasetStatusOutput& out);
///< Update the current status for a PSN-hosted dataset at storage. 
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_UpdateSubscriptionFilter(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateSubscriptionFilterInput& in,
                        vplex::vsDirectory::UpdateSubscriptionFilterOutput& out);

int VPLVsDirectory_UpdateSubscriptionLimits(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateSubscriptionLimitsInput& in,
                        vplex::vsDirectory::UpdateSubscriptionLimitsOutput& out);

int VPLVsDirectory_UpdateDeviceInfo(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::UpdateDeviceInfoInput& in,
                                 vplex::vsDirectory::UpdateDeviceInfoOutput& out);

int VPLVsDirectory_AddDatasetArchiveStorageDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput& in,
                                 vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput& out);

int VPLVsDirectory_RemoveDatasetArchiveStorageDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput& in,
                                 vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput& out);

///< Update device information
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

int VPLVsDirectory_GetCloudInfo(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                VPLTime_t timeout,
                                const vplex::vsDirectory::GetCloudInfoInput& in,
                                vplex::vsDirectory::GetCloudInfoOutput& out);
///< Get all info from user's cloud.
///< Info includes: GetLinkedDevices, GetSubscriptionDetailsForDevice,
///<                ListOwnedDataSets, ListUserStorage, GetUserStorageAddress
///< @return #VPL_OK if all query was successful, otherwise the first error
///< encountered.

///@}

#endif // include guard
