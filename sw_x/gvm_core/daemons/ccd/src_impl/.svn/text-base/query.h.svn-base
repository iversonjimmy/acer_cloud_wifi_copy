//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// TODO: rename to query.hpp

#ifndef __QUERY_H__
#define __QUERY_H__

#include "cache.h"
#include "ccdi_rpc.pb.h"
#include "ans_device.h"
#include "config.h"
#include <sstream> // for std::ostringstream

//--------------------

/// Gets the fully qualified domain name (FQDN) for the specified infra cluster.
#define CCD_GET_INFRA_CLUSTER_HOSTNAME(fqdn_, clusterId_) \
        snprintf(fqdn_, ARRAY_SIZE_IN_BYTES(fqdn_), "www-c"FMTs64".%s", clusterId_, __ccdConfig.infraDomain)

/// Gets the fully qualified domain name (FQDN) for the central infra.
#define CCD_GET_INFRA_CENTRAL_HOSTNAME(fqdn_) \
        snprintf(fqdn_, ARRAY_SIZE_IN_BYTES(fqdn_), "www.%s", __ccdConfig.infraDomain)

/// Gets the fully qualified domain name (FQDN) for the ANS server in the specified infra cluster.
#define CCD_GET_ANS_HOSTNAME(fqdn_, clusterId_) \
        snprintf(fqdn_, ARRAY_SIZE_IN_BYTES(fqdn_), "ans-c"FMTs64".%s", clusterId_, __ccdConfig.infraDomain)

/// Gets the fully qualified domain name (FQDN) for the sleep keep-alive server with the specified
/// local name.
#define CCD_GET_SLEEP_SERVER_HOSTNAME(fqdn_, local_name_) \
        snprintf(fqdn_, ARRAY_SIZE_IN_BYTES(fqdn_), "%s.%s", local_name_, __ccdConfig.infraDomain)

s32 Query_GetUrlPrefix(std::ostringstream& result_out,
        ccd::InfraHttpService_t service,
        bool secure,
        const CachePlayer* optionalUser);

s32 Query_GetUrlPrefix(std::ostringstream& result_out,
                       ccd::LocalHttpService_t service);

s32 Query_Init(void);

s32 Query_Quit(void);

/// Call this when activating a user to use the user's cluster in infra host names.
//% This assumes (CCD_MAX_USERS == 1)
void Query_UpdateClusterId(s64 clusterId);

/// Call this when deactivating a user to restore the default infra host names.
//% This assumes (CCD_MAX_USERS == 1)
void Query_UpdateClusterId();

//--------------------

s32 Query_ChangePassword(VPLUser_Id_t userId, const std::string& oldPassword,
        const std::string& newPassword, std::string& weakToken_out);

s32 Query_GetAnsLoginBlob(u64 sessionHandle, const std::string& iasTicket,
                          std::string& ansSessionKey_out, std::string& ansLoginBlob_out, u32& instanceId_out);

s32 Query_Login(vplex::ias::LoginResponseType& iasOutput_out,
                           const char* userName, const char* password,
                           const char* pairingToken, const VPL_BOOL* eulaAgreed);

s32 Query_GenerateServiceTickets(ccd::UserSession &session);

s32 Query_Logout(const ccd::UserSession& session);
s32 Query_Logout(u64 sessionHandle, const std::string& iasTicket);

s32 Query_GetPxdLoginBlob(u64 sessionHandle, const std::string& iasTicket,
                          const std::string& ansLoginBlob,
                          std::string& pxdSessionKey_out, std::string& pxdLoginBlob_out, u32& instanceId_out);

/// Get the CCD-to-CCD credentials for the tunnel layer (TS) to authenticate us to a remote CCD.
s32 Query_GetCCDLoginBlob(u64 sessionHandle, const std::string& iasTicket,
                          u64 serverUserId, u64 serverDeviceId, u32 serverInstanceId,
                          const std::string& ansLoginBlob,
                          std::string& ccdSessionKey_out, std::string& ccdLoginBlob_out, u32& instanceId);

/// Get the key that allows CCD to authenticate an incoming tunnel layer (TS) connection.
s32 Query_GetCCDServerKey(u64 sessionHandle, const std::string& iasTicket,
        VPLUser_Id_t userId, std::string& ccdServerKey_out);

#endif // include guard
