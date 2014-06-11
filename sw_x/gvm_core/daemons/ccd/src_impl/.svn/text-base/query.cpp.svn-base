//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "query.h"

#include "ccd_core.h"
#include "ias_query.hpp"
#include "nus_query.hpp"
#include "vsds_query.hpp"
#include "ans_connection.hpp"
#include "cache.h"
#include "ccd_storage.hpp"
#include "ccdi.hpp"
#include "ccdi_internal_defs.hpp"
#include "config.h"
#include "vplex_file.h"
#include "vplex_http2.hpp"
#include "vplu_sstr.hpp"
#include <memory>

using namespace std;
using namespace ccd;

void
Query_UpdateClusterId(s64 clusterId)
{
    char newHostname[HOSTNAME_MAX_LENGTH];
    CCD_GET_INFRA_CLUSTER_HOSTNAME(newHostname, clusterId);
    Query_UpdateIasHostname(newHostname);
    Query_UpdateNusHostname(newHostname);
    Query_UpdateVsdsHostname(newHostname);
}

void
Query_UpdateClusterId()
{
    char newHostname[HOSTNAME_MAX_LENGTH];
    CCD_GET_INFRA_CENTRAL_HOSTNAME(newHostname);
    Query_UpdateIasHostname(newHostname);
    Query_UpdateNusHostname(newHostname);
    Query_UpdateVsdsHostname(newHostname);
}

s32
Query_Init()
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    int vpl_rv;

    vpl_rv = VPLHttp2::Init();
    if (vpl_rv != VPL_OK) {
        LOG_ERROR("VPLHttp2::Init returned %d", vpl_rv);
    }
    
    char defaultHostname[HOSTNAME_MAX_LENGTH];
    CCD_GET_INFRA_CENTRAL_HOSTNAME(defaultHostname);

    rv = Query_InitIAS(defaultHostname, __ccdConfig.iasPort);
    if (rv != 0) {
        LOG_ERROR("Failure initializing IAS queries: %d", rv);
        goto out;
    }
    rv = Query_InitNUS(defaultHostname, __ccdConfig.nusPort);
    if (rv != 0) {
        LOG_ERROR("Failure initializing NUS queries: %d", rv);
        goto out;
    }
    rv = Query_InitVSDS(defaultHostname, __ccdConfig.vsdsContentPort);
    if (rv != 0) {
        LOG_ERROR("Failure initializing VSDS queries: %d", rv);
        goto out;
    }

out:
    return rv;
}


s32
Query_GetAnsLoginBlob(u64 sessionHandle, const std::string& iasTicket, std::string& ansSessionKey_out, std::string& ansLoginBlob_out, u32& instanceId_out)
{
    s32 rv;
    {
        u64 deviceId;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("ESCore_GetDeviceGuid failed: %d", rv);
            goto out;
        }
        vplex::ias::GetSessionKeyRequestType iasInput;
        Query_SetIasAbstractRequestFields(iasInput);
        iasInput.mutable__inherited()->set_sessionhandle(sessionHandle);
        iasInput.mutable__inherited()->set_serviceticket(iasTicket);
        iasInput.mutable__inherited()->set_serviceid("IAS");
        iasInput.mutable__inherited()->set_deviceid(deviceId);
        
        vplex::ias::StrAttributeType* sat = NULL;
        sat = iasInput.add_keyattributes();
        sat->set_attributename("DeviceType");
        sat->set_attributevalue(DEVICE_TYPE_CODE);
        
        sat = iasInput.add_keyattributes();
        sat->set_attributename("TitleId");
        sat->set_attributevalue(CCDGetTitleId());
        
        sat = iasInput.add_keyattributes();
        sat->set_attributename("CCDProtocolVersion");
        sat->set_attributevalue(CCD_PROTOCOL_VERSION);
        
        iasInput.set_type("ANS");

        LOG_DEBUG("Calling %s", "VPLIas_GetSessionKey");
        vplex::ias::GetSessionKeyResponseType iasOutput;
        rv = QUERY_IAS(VPLIas_GetSessionKey, iasInput, iasOutput);
        if (rv != VPL_OK) {
            LOG_ERROR("%s failed: %d", "VPLIas_GetSessionKey", rv);
            goto out;
        } else {
            LOG_DEBUG("%s success", "VPLIas_GetSessionKey");
        }
        ansSessionKey_out.assign(iasOutput.sessionkey());
        // The IAS GetSessionKey field "EncryptedSessionKey" is actually the ANS login blob.
        ansLoginBlob_out.assign(iasOutput.encryptedsessionkey());
        instanceId_out = iasOutput.instanceid();
    }
out:
    return rv;
}

s32
Query_Login(vplex::ias::LoginResponseType& iasOutput_out,
            const char* userName, const char* password,
            const char* pairingToken, const VPL_BOOL* eulaAgreed)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    
    if (userName == NULL) {
        LOG_ERROR("NULL userName");
        rv = CCD_ERROR_PARAMETER;
        goto out;
    }
    if (password == NULL && pairingToken == NULL) {
        LOG_ERROR("Must provide one of password or pairing token");
        rv = CCD_ERROR_PARAMETER;
        goto out;
    }

    {
        u64 deviceId;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("ESCore_GetDeviceGuid failed: %d", rv);
            goto out;
        }
        vplex::ias::LoginRequestType iasInput;
        Query_SetIasAbstractRequestFields(iasInput);
        iasInput.set_username(userName);
        if (password != NULL) {
            iasInput.set_password(password);
        }
        if (pairingToken != NULL) {
            iasInput.set_pairingtoken(pairingToken);
        }
        if (eulaAgreed != NULL) {
            iasInput.set_aceulaagreed(*eulaAgreed);
        }
        iasInput.set_namespace_(VPL_DEFAULT_USER_NAMESPACE);
        iasInput.mutable__inherited()->set_deviceid(deviceId);

        LOG_DEBUG("Calling VPLIas_Login");
        rv = QUERY_IAS(VPLIas_Login, iasInput, iasOutput_out);
        if (rv != VPL_OK) {
            LOG_ERROR("VPLIas_Login returned %d", rv);
            goto out;
        } else {
            LOG_DEBUG("VPLIas_Login success");
        }
    }
    
out:
    return rv;
}

s32 Query_GenerateServiceTickets(ccd::UserSession &session)
{
    s32 rv;
    // Base64-encode the secret for purposes of generating the service tickets.
    char* secret64 = NULL;
    size_t secret64len = 0;
    Util_EncodeBase64(session.session_secret().c_str(), session.session_secret().size(),
                      &secret64, &secret64len, VPL_FALSE, VPL_FALSE);
    ON_BLOCK_EXIT(free, secret64);
    {
        UCFBuffer ticket;
        memset(&ticket, 0, sizeof(ticket));
        rv = Util_GetServiceTicket(&ticket, "Virtual Storage", secret64, secret64len);
        if (rv < 0) {
            goto out;
        }
        session.set_vs_ticket(ticket.data, ticket.size);
        session.set_rf_ticket(ticket.data, ticket.size);  // for backwards compatibility, same ticket as vs
        free(ticket.data);
    }
    {
        UCFBuffer ticket;
        memset(&ticket, 0, sizeof(ticket));
        rv = Util_GetServiceTicket(&ticket, "Community Services", secret64, secret64len);
        if (rv < 0) {
            goto out;
        }
        session.set_cs_ticket(ticket.data, ticket.size);
        free(ticket.data);
    }
    {
        UCFBuffer ticket;
        memset(&ticket, 0, sizeof(ticket));
        rv = Util_GetServiceTicket(&ticket, "IAS", secret64, secret64len);
        if (rv < 0) {
            goto out;
        }
        session.set_ias_ticket(ticket.data, ticket.size);
        free(ticket.data);
    }
    {
        UCFBuffer ticket;
        memset(&ticket, 0, sizeof(ticket));
        rv = Util_GetServiceTicket(&ticket, "OPS", secret64, secret64len);
        if (rv < 0) {
            goto out;
        }
        session.set_ops_ticket(ticket.data, ticket.size);
        free(ticket.data);
    }
    {
        UCFBuffer ticket;
        memset(&ticket, 0, sizeof(ticket));
        rv = Util_GetServiceTicket(&ticket, "ECommerce", secret64, secret64len);
        if (rv < 0) {
            goto out;
        }
        session.set_ec_ticket(ticket.data, ticket.size);
        free(ticket.data);
    }
    
out:
    return rv;
}

s32 Query_Logout(const ccd::UserSession& session)
{
    return Query_Logout(session.session_handle(), session.ias_ticket());
}

s32
Query_Logout(u64 sessionHandle, const std::string& iasTicket)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;

    {
        vplex::ias::LogoutRequestType iasInput;
        Query_SetIasAbstractRequestFields(iasInput);
        iasInput.mutable__inherited()->set_sessionhandle(sessionHandle);
        iasInput.mutable__inherited()->set_serviceticket(iasTicket);
        iasInput.mutable__inherited()->set_serviceid("IAS");
        vplex::ias::LogoutResponseType iasOutput;
        LOG_DEBUG("Calling VPLIas_Logout");
        rv = QUERY_IAS(VPLIas_Logout, iasInput, iasOutput);
        if (rv != VPL_OK) {
            LOG_WARN("%s failed: %d", "VPLIas_Logout", rv);
        } else {
            LOG_DEBUG("%s success", "VPLIas_Logout");
        }
    }
    return rv;
}

s32
Query_ChangePassword(
        VPLUser_Id_t userId,
        const std::string& oldPassword,
        const std::string& newPassword,
        std::string& weakToken_out)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;

    UNUSED(userId);
    UNUSED(oldPassword);
    UNUSED(newPassword);
    UNUSED(weakToken_out);
    rv = CCD_ERROR_NOT_IMPLEMENTED;
    LOG_ERROR("Ability to change password from the client has been disabled!");

    return rv;
}

s32
Query_Quit(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv = CCD_OK;

    rv = Query_QuitVSDS();
    if(rv != 0) {
        LOG_ERROR("%s failed: %d", "Query_QuitVSDS", rv);
    }

    rv = Query_QuitNUS();
    if(rv != 0) {
        LOG_ERROR("%s failed: %d", "Query_QuitNUS", rv);
    }

    rv = Query_QuitIAS();
    if(rv != 0) {
        LOG_ERROR("%s failed: %d", "Query_QuitIAS", rv);
    }

    return rv;
}

s32
Query_GetUrlPrefix(std::ostringstream& result_out,
        InfraHttpService_t service,
        bool secure,
        const CachePlayer* optionalUser)
{
    bool useCluster;
    s64 clusterId = -1;

    switch (service) {
    case INFRA_HTTP_SERVICE_VCS:
    case INFRA_HTTP_SERVICE_OPS:
        if (optionalUser == NULL) {
            useCluster = false;
        } else {
            useCluster = true;
            clusterId = optionalUser->cluster_id();
        }
        break;
    case INFRA_HTTP_SERVICE_OPS_CENTRAL:
        useCluster = false;
        service = INFRA_HTTP_SERVICE_OPS;
        break;
    case INFRA_HTTP_SERVICE_OPS_REGIONAL:
        if (optionalUser == NULL) {
            LOG_ERROR("Must specify a user for regional");
            return CCD_ERROR_PARAMETER;
        }
        useCluster = true;
        clusterId = optionalUser->cluster_id();
        service = INFRA_HTTP_SERVICE_OPS;
        break;
    default:
        LOG_ERROR("Unknown service: %d", service);
        return CCD_ERROR_PARAMETER;
    }

    char hostname[HOSTNAME_MAX_LENGTH];
    if (useCluster) {
        CCD_GET_INFRA_CLUSTER_HOSTNAME(hostname, clusterId);
    } else {
        CCD_GET_INFRA_CENTRAL_HOSTNAME(hostname);
    }
    if (secure) {
        result_out << "https://" << hostname << ":443";
    } else {
        result_out << "http://" << hostname << ":80";
    }
    return VPL_OK;
}

s32 Query_GetUrlPrefix(std::ostringstream& result_out,
                       LocalHttpService_t service)
{
    VPLNet_port_t port = 80;
    switch (service) {
    case LOCAL_HTTP_SERVICE_REMOTE_FILES:
        port = LocalServers_GetHttpService().stream_listening_port();
        if (port == VPLNET_PORT_INVALID) {
            LOG_ERROR("No port assigned for streaming");
            return CCD_ERROR_STREAM_SERVICE;
        }
        break;
    default:
        LOG_ERROR("Unknown service: %d", service);
        return CCD_ERROR_PARAMETER;
    }

    result_out << "http://127.0.0.1:" << port;
    return VPL_OK;
}

s32
Query_GetPxdLoginBlob(u64 sessionHandle, const std::string& iasTicket,
                      const std::string& ansLoginBlob,
                      std::string& pxdSessionKey_out, std::string& pxdLoginBlob_out, u32& instanceId_out)
{
    s32 rv;
    {
        u64 deviceId;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("ESCore_GetDeviceGuid failed: %d", rv);
            goto out;
        }
        vplex::ias::GetSessionKeyRequestType iasInput;
        Query_SetIasAbstractRequestFields(iasInput);
        iasInput.mutable__inherited()->set_sessionhandle(sessionHandle);
        iasInput.mutable__inherited()->set_serviceticket(iasTicket);
        iasInput.mutable__inherited()->set_serviceid("IAS");
        iasInput.mutable__inherited()->set_deviceid(deviceId);
        iasInput.set_type("PXD");
        iasInput.set_encryptedsessionkey(ansLoginBlob);

        vplex::ias::StrAttributeType* sat = NULL;
        sat = iasInput.add_keyattributes();
        sat->set_attributename("DeviceType");
        sat->set_attributevalue(DEVICE_TYPE_CODE);

        sat = iasInput.add_keyattributes();
        sat->set_attributename("TitleId");
        sat->set_attributevalue(CCDGetTitleId());

        sat = iasInput.add_keyattributes();
        sat->set_attributename("CCDProtocolVersion");
        sat->set_attributevalue(CCD_PROTOCOL_VERSION);

        LOG_DEBUG("Calling %s", "VPLIas_GetSessionKey");
        vplex::ias::GetSessionKeyResponseType iasOutput;
        rv = QUERY_IAS(VPLIas_GetSessionKey, iasInput, iasOutput);
        if (rv != VPL_OK) {
            LOG_ERROR("%s failed: %d", "VPLIas_GetSessionKey", rv);
            goto out;
        } else {
            LOG_DEBUG("%s success", "VPLIas_GetSessionKey");
        }
        pxdSessionKey_out.assign(iasOutput.sessionkey());
        // The IAS GetSessionKey field "EncryptedSessionKey" is actually the PXD login blob.
        pxdLoginBlob_out.assign(iasOutput.encryptedsessionkey());
        instanceId_out = iasOutput.instanceid();
    }
out:
    return rv;
}

s32
Query_GetCCDLoginBlob(u64 sessionHandle, const std::string& iasTicket,
                      u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                      const std::string& ansLoginBlob, 
                      std::string& ccdSessionKey_out, std::string& ccdLoginBlob_out, u32& instanceId_out)
{
    s32 rv;
    {
        u64 deviceId;
        vplex::ias::StrAttributeType* sat = NULL;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("ESCore_GetDeviceGuid failed: %d", rv);
            goto out;
        }
        vplex::ias::GetSessionKeyRequestType iasInput;
        Query_SetIasAbstractRequestFields(iasInput);
        iasInput.mutable__inherited()->set_sessionhandle(sessionHandle);
        iasInput.mutable__inherited()->set_serviceticket(iasTicket);
        iasInput.mutable__inherited()->set_serviceid("IAS");
        iasInput.mutable__inherited()->set_deviceid(deviceId);
        iasInput.set_type("CCD");
        iasInput.set_encryptedsessionkey(ansLoginBlob);

        sat = iasInput.add_keyattributes();
        sat->set_attributename("CCDServerDeviceId");
        sat->set_attributevalue(SSTR(serverDeviceId));

        sat = iasInput.add_keyattributes();
        sat->set_attributename("CCDServerUserId");
        sat->set_attributevalue(SSTR(serverUserId));

#if 0
        // not needed for 2.7
        // http://www.ctbg.acer.com/wiki/index.php/IAS_Enhancement_for_Supporting_Multiple_Instance_ID
        sat = iasInput.add_keyattributes();
        sat->set_attributename("CCDServerInstanceId");
        sat->set_attributevalue(serverInstanceId);
#endif

        sat = iasInput.add_keyattributes();
        sat->set_attributename("DeviceType");
        sat->set_attributevalue(DEVICE_TYPE_CODE);

        sat = iasInput.add_keyattributes();
        sat->set_attributename("TitleId");
        sat->set_attributevalue(CCDGetTitleId());

        sat = iasInput.add_keyattributes();
        sat->set_attributename("CCDProtocolVersion");
        sat->set_attributevalue(CCD_PROTOCOL_VERSION);

        LOG_DEBUG("Calling %s", "VPLIas_GetSessionKey");
        vplex::ias::GetSessionKeyResponseType iasOutput;
        rv = QUERY_IAS(VPLIas_GetSessionKey, iasInput, iasOutput);
        if (rv != VPL_OK) {
            LOG_ERROR("%s failed: %d", "VPLIas_GetSessionKey", rv);
            goto out;
        } else {
            LOG_DEBUG("%s success", "VPLIas_GetSessionKey");
        }
        ccdSessionKey_out.assign(iasOutput.sessionkey());
        // The IAS GetSessionKey field "EncryptedSessionKey" is actually the CCD-to-CCD login blob.
        ccdLoginBlob_out.assign(iasOutput.encryptedsessionkey());
        instanceId_out = iasOutput.instanceid();
    }
out:
    return rv;
}

s32
Query_GetCCDServerKey(u64 sessionHandle, const std::string& iasTicket,
        VPLUser_Id_t userId, std::string& ccdServerKey_out)
{
    s32 rv;
    {
        u64 deviceId;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("ESCore_GetDeviceGuid failed: %d", rv);
            goto out;
        }
        vplex::ias::GetServerKeyRequestType iasInput;
        Query_SetIasAbstractRequestFields(iasInput);
        iasInput.mutable__inherited()->set_sessionhandle(sessionHandle);
        iasInput.mutable__inherited()->set_serviceticket(iasTicket);
        iasInput.mutable__inherited()->set_serviceid("IAS");
        iasInput.mutable__inherited()->set_deviceid(deviceId);
        iasInput.set_userid(userId);

        LOG_DEBUG("Calling %s", "VPLIas_GetServerKey");
        vplex::ias::GetServerKeyResponseType iasOutput;
        rv = QUERY_IAS(VPLIas_GetServerKey, iasInput, iasOutput);
        if (rv != VPL_OK) {
            LOG_ERROR("%s failed: %d", "VPLIas_GetServerKey", rv);
            goto out;
        } else {
            LOG_DEBUG("%s success", "VPLIas_GetServerKey");
        }
        ccdServerKey_out.assign(iasOutput.serverkey());
    }
out:
    return rv;
}
