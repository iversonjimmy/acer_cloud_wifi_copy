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

#include "vplex_vs_directory.h"
#include "vplex_vs_directory_priv.h"
#include "vplex_vs_directory_service_types-xml.pb.h"
#include "vplex_private.h"
#include "vplex_soap.h"
#include "vplex_mem_utils.h"
#include "vplex_serialization.h"
#include "vplex_socket.h"

#include <stdlib.h>

#ifdef VPL_VS_DIRECTORY_USE_SSL
static const VPLSoapProtocol PROTOCOL = VPL_SOAP_PROTO_HTTPS;
#define BROADON_AUTH_REQUESTER_NAME  ""
#define BROADON_AUTH_REQUESTER_SECRET  ""
#else
static const VPLSoapProtocol PROTOCOL = VPL_SOAP_PROTO_HTTP;
#define BROADON_AUTH_REQUESTER_NAME  "unitTest"
#define BROADON_AUTH_REQUESTER_SECRET  "8bfdc4e3-472d-42be-93a0-71d322883802"
#endif

#define SERVICE_NAME  "vsds"

#define WS_NAMESPACE  "urn:vsds.wsapi.broadon.com"

//--------------------------

int
VPLVsDirectory_CreateProxy(
        const char* serverHostname,
        u16 serverPort,
        VPLVsDirectory_ProxyHandle_t* proxyHandle_out)
{
    int rc;
    VPLVsDirectory_Proxy_t* newConnection;
    if (proxyHandle_out == NULL) {
        return VPL_ERR_INVALID;
    }
    proxyHandle_out->ptr = NULL;

    if (serverHostname == NULL) {
        return VPL_ERR_INVALID;
    }

    newConnection = (VPLVsDirectory_Proxy_t*)malloc(sizeof(VPLVsDirectory_Proxy_t));
    if(newConnection == NULL) {
        return VPL_ERR_NOMEM;
    }

    rc = VPLSoapProxy_Init(
            &(newConnection->soapProxy),
            PROTOCOL,
            serverHostname,
            serverPort,
            SERVICE_NAME,
            BROADON_AUTH_REQUESTER_NAME,
            BROADON_AUTH_REQUESTER_SECRET);
    if(rc != VPL_OK) {
        goto fail;
    }
    proxyHandle_out->ptr = newConnection;
    return VPL_OK;
fail:
    free(newConnection);
    return rc;
}

int
VPLVsDirectory_DestroyProxy(
        VPLVsDirectory_ProxyHandle_t proxyHandle)
{
    VPLVsDirectory_Proxy_t* proxy = getVsDirectoryProxyFromHandle(proxyHandle);
    int rc = VPLSoapProxy_Cleanup(&proxy->soapProxy);
    free(proxy);
    return rc;
}

void
VPLVsDirectory_priv_insertCommonSoap(VPLXmlWriter* writer, const char* operName)
{
    VPLSoapUtil_InsertCommonSoap2(writer, operName, WS_NAMESPACE);
}

static inline int
VPLVsDirectory_priv_VSDSErrToVPLErr(int vsdsErrCode)
{
    return VPLSoapUtil_InfraErrToVPLErr(vsdsErrCode);
}

int VPLVsDirectory_AddCameraDataset(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddCameraDatasetInput& in,
                              vplex::vsDirectory::AddCameraDatasetOutput& out)
{
    vplex::vsDirectory::AddCameraDatasetOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AddCameraDatasetInput,
            vplex::vsDirectory::AddCameraDatasetOutput,
            vplex::vsDirectory::ParseStateAddCameraDatasetOutput>(
                    "AddCameraDataset", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAddCameraDatasetInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_AddCameraSubscription(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddCameraSubscriptionInput& in,
                              vplex::vsDirectory::AddCameraSubscriptionOutput& out)
{
    vplex::vsDirectory::AddCameraSubscriptionOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AddCameraSubscriptionInput,
            vplex::vsDirectory::AddCameraSubscriptionOutput,
            vplex::vsDirectory::ParseStateAddCameraSubscriptionOutput>(
                    "AddCameraSubscription", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAddCameraSubscriptionInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_AddDataSet(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddDataSetInput& in,
                              vplex::vsDirectory::AddDataSetOutput& out)
{
    vplex::vsDirectory::AddDataSetOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AddDataSetInput,
            vplex::vsDirectory::AddDataSetOutput,
            vplex::vsDirectory::ParseStateAddDataSetOutput>(
                    "AddDataSet", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAddDataSetInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_DeleteDataSet(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::DeleteDataSetInput& in,
                                 vplex::vsDirectory::DeleteDataSetOutput& out)
{
    vplex::vsDirectory::DeleteDataSetOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::DeleteDataSetInput,
            vplex::vsDirectory::DeleteDataSetOutput,
            vplex::vsDirectory::ParseStateDeleteDataSetOutput>(
                    "DeleteDataSet", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeDeleteDataSetInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_AddSubscriptions(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddSubscriptionsInput& in,
                              vplex::vsDirectory::AddSubscriptionsOutput& out)
{
    vplex::vsDirectory::AddSubscriptionsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AddSubscriptionsInput,
            vplex::vsDirectory::AddSubscriptionsOutput,
            vplex::vsDirectory::ParseStateAddSubscriptionsOutput>(
                    "AddSubscriptions", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAddSubscriptionsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_AddDatasetSubscription(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddDatasetSubscriptionInput& in,
                              vplex::vsDirectory::AddDatasetSubscriptionOutput& out)
{
    vplex::vsDirectory::AddDatasetSubscriptionOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AddDatasetSubscriptionInput,
            vplex::vsDirectory::AddDatasetSubscriptionOutput,
            vplex::vsDirectory::ParseStateAddDatasetSubscriptionOutput>(
                    "AddDatasetSubscription", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAddDatasetSubscriptionInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_DeleteSubscriptions(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::DeleteSubscriptionsInput& in,
                              vplex::vsDirectory::DeleteSubscriptionsOutput& out)
{
    vplex::vsDirectory::DeleteSubscriptionsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::DeleteSubscriptionsInput,
            vplex::vsDirectory::DeleteSubscriptionsOutput,
            vplex::vsDirectory::ParseStateDeleteSubscriptionsOutput>(
                    "DeleteSubscriptions", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeDeleteSubscriptionsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_ListOwnedDataSets(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::ListOwnedDataSetsInput& in,
                              vplex::vsDirectory::ListOwnedDataSetsOutput& out)
{
    vplex::vsDirectory::ListOwnedDataSetsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::ListOwnedDataSetsInput,
            vplex::vsDirectory::ListOwnedDataSetsOutput,
            vplex::vsDirectory::ParseStateListOwnedDataSetsOutput>(
                    "ListOwnedDataSets", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeListOwnedDataSetsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_ListSubscriptions(
                              VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::ListSubscriptionsInput& in,
                              vplex::vsDirectory::ListSubscriptionsOutput& out)
{
    vplex::vsDirectory::ListSubscriptionsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::ListSubscriptionsInput,
            vplex::vsDirectory::ListSubscriptionsOutput,
            vplex::vsDirectory::ParseStateListSubscriptionsOutput>(
                    "ListSubscriptions", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeListSubscriptionsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_GetDatasetDetails(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetDatasetDetailsInput& in,
                        vplex::vsDirectory::GetDatasetDetailsOutput& out)
{
    vplex::vsDirectory::GetDatasetDetailsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetDatasetDetailsInput,
            vplex::vsDirectory::GetDatasetDetailsOutput,
            vplex::vsDirectory::ParseStateGetDatasetDetailsOutput>(
                    "GetDatasetDetails", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetDatasetDetailsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetSaveTickets(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetSaveTicketsInput& in,
                        vplex::vsDirectory::GetSaveTicketsOutput& out)
{
    vplex::vsDirectory::GetSaveTicketsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetSaveTicketsInput,
            vplex::vsDirectory::GetSaveTicketsOutput,
            vplex::vsDirectory::ParseStateGetSaveTicketsOutput>(
                    "GetSaveTickets", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetSaveTicketsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetSaveData(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetSaveDataInput& in,
                        vplex::vsDirectory::GetSaveDataOutput& out)
{
    vplex::vsDirectory::GetSaveDataOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetSaveDataInput,
            vplex::vsDirectory::GetSaveDataOutput,
            vplex::vsDirectory::ParseStateGetSaveDataOutput>(
                    "GetSaveData", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetSaveDataInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetSubscribedDatasets(
                       VPLVsDirectory_ProxyHandle_t proxyHandle,
                       VPLTime_t timeout,
                       const vplex::vsDirectory::GetSubscribedDatasetsInput& in,
                       vplex::vsDirectory::GetSubscribedDatasetsOutput& out)
{
    vplex::vsDirectory::GetSubscribedDatasetsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetSubscribedDatasetsInput,
            vplex::vsDirectory::GetSubscribedDatasetsOutput,
            vplex::vsDirectory::ParseStateGetSubscribedDatasetsOutput>(
                    "GetSubscribedDatasets", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetSubscribedDatasetsInput);
    if(rv != VPL_OK) {
        goto end;
    }
    if(result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetSubscriptionDetailsForDevice(
                       VPLVsDirectory_ProxyHandle_t proxyHandle,
                       VPLTime_t timeout,
                       const vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput& in,
                       vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput& out)
{
    vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput,
            vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput,
            vplex::vsDirectory::ParseStateGetSubscriptionDetailsForDeviceOutput>(
                    "GetSubscriptionDetailsForDevice", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetSubscriptionDetailsForDeviceInput);
    if(rv != VPL_OK) {
        goto end;
    }
    if(result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

 end:
    return rv;
}

int VPLVsDirectory_GetOwnedTitles(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetOwnedTitlesInput& in,
                        vplex::vsDirectory::GetOwnedTitlesOutput& out)
{
    vplex::vsDirectory::GetOwnedTitlesOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetOwnedTitlesInput,
            vplex::vsDirectory::GetOwnedTitlesOutput,
            vplex::vsDirectory::ParseStateGetOwnedTitlesOutput>(
                    "GetOwnedTitles", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetOwnedTitlesInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetTitles(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetTitlesInput& in,
                        vplex::vsDirectory::GetTitlesOutput& out)
{
    vplex::vsDirectory::GetTitlesOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetTitlesInput,
            vplex::vsDirectory::GetTitlesOutput,
            vplex::vsDirectory::ParseStateGetTitlesOutput>(
                    "GetTitles", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetTitlesInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetTitleDetails(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetTitleDetailsInput& in,
                        vplex::vsDirectory::GetTitleDetailsOutput& out)
{
    vplex::vsDirectory::GetTitleDetailsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetTitleDetailsInput,
            vplex::vsDirectory::GetTitleDetailsOutput,
            vplex::vsDirectory::ParseStateGetTitleDetailsOutput>(
                    "GetTitleDetails", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetTitleDetailsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetAttestationChallenge(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetAttestationChallengeInput& in,
                        vplex::vsDirectory::GetAttestationChallengeOutput& out)
{
    vplex::vsDirectory::GetAttestationChallengeOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetAttestationChallengeInput,
            vplex::vsDirectory::GetAttestationChallengeOutput,
            vplex::vsDirectory::ParseStateGetAttestationChallengeOutput>(
                    "GetAttestationChallenge", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetAttestationChallengeInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_AuthenticateDevice(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::AuthenticateDeviceInput& in,
                        vplex::vsDirectory::AuthenticateDeviceOutput& out)
{
    vplex::vsDirectory::AuthenticateDeviceOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AuthenticateDeviceInput,
            vplex::vsDirectory::AuthenticateDeviceOutput,
            vplex::vsDirectory::ParseStateAuthenticateDeviceOutput>(
                    "AuthenticateDevice", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAuthenticateDeviceInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_LinkDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::LinkDeviceInput& in,
                              vplex::vsDirectory::LinkDeviceOutput& out)
{
    vplex::vsDirectory::LinkDeviceOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::LinkDeviceInput,
            vplex::vsDirectory::LinkDeviceOutput,
            vplex::vsDirectory::ParseStateLinkDeviceOutput>(
                    "LinkDevice", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeLinkDeviceInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_UnlinkDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                VPLTime_t timeout,
                                const vplex::vsDirectory::UnlinkDeviceInput& in,
                                vplex::vsDirectory::UnlinkDeviceOutput& out)
{
    vplex::vsDirectory::UnlinkDeviceOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::UnlinkDeviceInput,
            vplex::vsDirectory::UnlinkDeviceOutput,
            vplex::vsDirectory::ParseStateUnlinkDeviceOutput>(
                    "UnlinkDevice", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeUnlinkDeviceInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetLinkedDevices(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                    VPLTime_t timeout,
                                    const vplex::vsDirectory::GetLinkedDevicesInput& in,
                                    vplex::vsDirectory::GetLinkedDevicesOutput& out)
{
    vplex::vsDirectory::GetLinkedDevicesOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetLinkedDevicesInput,
            vplex::vsDirectory::GetLinkedDevicesOutput,
            vplex::vsDirectory::ParseStateGetLinkedDevicesOutput>(
                    "GetLinkedDevices", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetLinkedDevicesInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_SetDeviceName(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::SetDeviceNameInput& in,
                                 vplex::vsDirectory::SetDeviceNameOutput& out)
{
    vplex::vsDirectory::SetDeviceNameOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::SetDeviceNameInput,
            vplex::vsDirectory::SetDeviceNameOutput,
            vplex::vsDirectory::ParseStateSetDeviceNameOutput>(
                    "SetDeviceName", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeSetDeviceNameInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetOnlineTitleTicket(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetOnlineTitleTicketInput& in,
                        vplex::vsDirectory::GetOnlineTitleTicketOutput& out)
{
    vplex::vsDirectory::GetOnlineTitleTicketOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetOnlineTitleTicketInput,
            vplex::vsDirectory::GetOnlineTitleTicketOutput,
            vplex::vsDirectory::ParseStateGetOnlineTitleTicketOutput>(
                    "GetOnlineTitleTicket", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetOnlineTitleTicketInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetOfflineTitleTickets(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetOfflineTitleTicketsInput& in,
                        vplex::vsDirectory::GetOfflineTitleTicketsOutput& out)
{
    vplex::vsDirectory::GetOfflineTitleTicketsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetOfflineTitleTicketsInput,
            vplex::vsDirectory::GetOfflineTitleTicketsOutput,
            vplex::vsDirectory::ParseStateGetOfflineTitleTicketsOutput>(
                    "GetOfflineTitleTickets", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetOfflineTitleTicketsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetLoginSession(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetLoginSessionInput& in,
                        vplex::vsDirectory::GetLoginSessionOutput& out)
{
    vplex::vsDirectory::GetLoginSessionOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetLoginSessionInput,
            vplex::vsDirectory::GetLoginSessionOutput,
            vplex::vsDirectory::ParseStateGetLoginSessionOutput>(
                    "GetLoginSession", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetLoginSessionInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_CreatePersonalStorageNode(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::CreatePersonalStorageNodeInput& in,
                        vplex::vsDirectory::CreatePersonalStorageNodeOutput& out)
{
    vplex::vsDirectory::CreatePersonalStorageNodeOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::CreatePersonalStorageNodeInput,
            vplex::vsDirectory::CreatePersonalStorageNodeOutput,
            vplex::vsDirectory::ParseStateCreatePersonalStorageNodeOutput>(
                    "CreatePersonalStorageNode", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeCreatePersonalStorageNodeInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        // This is not always a problem; rather than make multiple calls to infra, we always try to
        // create the storage node and simply ignore the DUPLICATE_CLUSTERID error (-32230).
        VPL_LIB_LOG_INFO(VPL_SG_VS, "returning %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_AddUserStorage(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::AddUserStorageInput& in,
                        vplex::vsDirectory::AddUserStorageOutput& out)
{
    vplex::vsDirectory::AddUserStorageOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AddUserStorageInput,
            vplex::vsDirectory::AddUserStorageOutput,
            vplex::vsDirectory::ParseStateAddUserStorageOutput>(
                    "AddUserStorage", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAddUserStorageInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_ListUserStorage(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::ListUserStorageInput& in,
                        vplex::vsDirectory::ListUserStorageOutput& out)
{
    vplex::vsDirectory::ListUserStorageOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::ListUserStorageInput,
            vplex::vsDirectory::ListUserStorageOutput,
            vplex::vsDirectory::ParseStateListUserStorageOutput>(
                    "ListUserStorage", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeListUserStorageInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_UpdateStorageNodeFeatures(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateStorageNodeFeaturesInput& in,
                        vplex::vsDirectory::UpdateStorageNodeFeaturesOutput& out)
{
    vplex::vsDirectory::UpdateStorageNodeFeaturesOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::UpdateStorageNodeFeaturesInput,
            vplex::vsDirectory::UpdateStorageNodeFeaturesOutput,
            vplex::vsDirectory::ParseStateUpdateStorageNodeFeaturesOutput>(
                    "UpdateStorageNodeFeatures", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeUpdateStorageNodeFeaturesInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_UpdateStorageNodeConnection(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateStorageNodeConnectionInput& in,
                        vplex::vsDirectory::UpdateStorageNodeConnectionOutput& out)
{
    vplex::vsDirectory::UpdateStorageNodeConnectionOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::UpdateStorageNodeConnectionInput,
            vplex::vsDirectory::UpdateStorageNodeConnectionOutput,
            vplex::vsDirectory::ParseStateUpdateStorageNodeConnectionOutput>(
                    "UpdateStorageNodeConnection", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeUpdateStorageNodeConnectionInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_UpdatePSNDatasetStatus(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdatePSNDatasetStatusInput& in,
                        vplex::vsDirectory::UpdatePSNDatasetStatusOutput& out)
{
    vplex::vsDirectory::UpdatePSNDatasetStatusOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::UpdatePSNDatasetStatusInput,
            vplex::vsDirectory::UpdatePSNDatasetStatusOutput,
            vplex::vsDirectory::ParseStateUpdatePSNDatasetStatusOutput>(
                    "UpdatePSNDatasetStatus", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeUpdatePSNDatasetStatusInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetPSNDatasetLocation(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetPSNDatasetLocationInput& in,
                        vplex::vsDirectory::GetPSNDatasetLocationOutput& out)
{
    vplex::vsDirectory::GetPSNDatasetLocationOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetPSNDatasetLocationInput,
            vplex::vsDirectory::GetPSNDatasetLocationOutput,
            vplex::vsDirectory::ParseStateGetPSNDatasetLocationOutput>(
                    "GetPSNDatasetLocation", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetPSNDatasetLocationInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_GetUserStorageAddress(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetUserStorageAddressInput& in,
                        vplex::vsDirectory::GetUserStorageAddressOutput& out)
{
    vplex::vsDirectory::GetUserStorageAddressOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetUserStorageAddressInput,
            vplex::vsDirectory::GetUserStorageAddressOutput,
            vplex::vsDirectory::ParseStateGetUserStorageAddressOutput>(
                    "GetUserStorageAddress", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetUserStorageAddressInput);
    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
end:
    return rv;
}


int VPLVsDirectory_GetAsyncNoticeServer(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::GetAsyncNoticeServerInput& in,
                        vplex::vsDirectory::GetAsyncNoticeServerOutput& out)
{
    vplex::vsDirectory::GetAsyncNoticeServerOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::GetAsyncNoticeServerInput,
            vplex::vsDirectory::GetAsyncNoticeServerOutput,
            vplex::vsDirectory::ParseStateGetAsyncNoticeServerOutput>(
                    "GetAsyncNoticeServer", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeGetAsyncNoticeServerInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_UpdateSubscriptionFilter(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateSubscriptionFilterInput& in,
                        vplex::vsDirectory::UpdateSubscriptionFilterOutput& out)
{
    vplex::vsDirectory::UpdateSubscriptionFilterOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::UpdateSubscriptionFilterInput,
            vplex::vsDirectory::UpdateSubscriptionFilterOutput,
            vplex::vsDirectory::ParseStateUpdateSubscriptionFilterOutput>(
                    "UpdateSubscriptionFilter", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeUpdateSubscriptionFilterInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_UpdateSubscriptionLimits(
                        VPLVsDirectory_ProxyHandle_t proxyHandle,
                        VPLTime_t timeout,
                        const vplex::vsDirectory::UpdateSubscriptionLimitsInput& in,
                        vplex::vsDirectory::UpdateSubscriptionLimitsOutput& out)
{
    vplex::vsDirectory::UpdateSubscriptionLimitsOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::UpdateSubscriptionLimitsInput,
            vplex::vsDirectory::UpdateSubscriptionLimitsOutput,
            vplex::vsDirectory::ParseStateUpdateSubscriptionLimitsOutput>(
                    "UpdateSubscriptionLimits", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeUpdateSubscriptionLimitsInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }

    out = result;

end:
    return rv;
}

int VPLVsDirectory_DeleteUserStorage(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::DeleteUserStorageInput& in,
                                 vplex::vsDirectory::DeleteUserStorageOutput& out)
{
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::DeleteUserStorageInput,
            vplex::vsDirectory::DeleteUserStorageOutput,
            vplex::vsDirectory::ParseStateDeleteUserStorageOutput>(
                    "DeleteUserStorage", proxyHandle, timeout, in, out,
                    vplex::vsDirectory::writeDeleteUserStorageInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (out.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", out.error().errorcode(),
                out.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(out.error().errorcode());
        goto end;
    }
 end:
    return rv;
}

int VPLVsDirectory_UpdateDeviceInfo(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                 VPLTime_t timeout,
                                 const vplex::vsDirectory::UpdateDeviceInfoInput& in,
                                 vplex::vsDirectory::UpdateDeviceInfoOutput& out)
{
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::UpdateDeviceInfoInput,
            vplex::vsDirectory::UpdateDeviceInfoOutput,
            vplex::vsDirectory::ParseStateUpdateDeviceInfoOutput>(
                    "UpdateDeviceInfo", proxyHandle, timeout, in, out,
                    vplex::vsDirectory::writeUpdateDeviceInfoInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (out.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", out.error().errorcode(),
                out.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(out.error().errorcode());
        goto end;
    }
 end:
    return rv;

}

int VPLVsDirectory_GetCloudInfo(VPLVsDirectory_ProxyHandle_t proxyHandle,
                                VPLTime_t timeout,
                                const vplex::vsDirectory::GetCloudInfoInput& in,
                                vplex::vsDirectory::GetCloudInfoOutput& out)
{
    vplex::vsDirectory::GetCloudInfoOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
    vplex::vsDirectory::GetCloudInfoInput,
    vplex::vsDirectory::GetCloudInfoOutput,
    vplex::vsDirectory::ParseStateGetCloudInfoOutput>(
                                                      "GetCloudInfo", proxyHandle, timeout, in, result,
                                                      vplex::vsDirectory::writeGetCloudInfoInput);
    
    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                        result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
    
end:
    return rv;
}

int VPLVsDirectory_AddDatasetArchiveStorageDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput& in,
                              vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput& out)
{
    vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput,
            vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput,
            vplex::vsDirectory::ParseStateAddDatasetArchiveStorageDeviceOutput>(
                    "AddDatasetArchiveStorageDevice", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeAddDatasetArchiveStorageDeviceInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}

int VPLVsDirectory_RemoveDatasetArchiveStorageDevice(VPLVsDirectory_ProxyHandle_t proxyHandle,
                              VPLTime_t timeout,
                              const vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput& in,
                              vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput& out)
{
    vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput result;
    int rv = VPLVsDirectory_priv_ProtoSoapCall<
            vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput,
            vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput,
            vplex::vsDirectory::ParseStateRemoveDatasetArchiveStorageDeviceOutput>(
                    "RemoveDatasetArchiveStorageDevice", proxyHandle, timeout, in, result,
                    vplex::vsDirectory::writeRemoveDatasetArchiveStorageDeviceInput);

    if (rv != VPL_OK) {
        goto end;
    }
    if (result.error().errorcode() != 0) {
        VPL_LIB_LOG_ERR(VPL_SG_VS, "error %d: %s", result.error().errorcode(),
                result.error().errordetail().c_str());
        rv = VPLVsDirectory_priv_VSDSErrToVPLErr(result.error().errorcode());
        goto end;
    }
    out = result;
 end:
    return rv;
}
