/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include "vsTest_infra.hpp"

#include "vplex_ias.hpp"
#include "vplex_ias_service_types.pb.h"
#include "vplex_serialization.h"
#include "vplex_trace.h"
#include "vpl_th.h"

#include "csltypes.h"
#include "cslsha.h"

#include <sstream>
#include <stack>

using namespace std;

static string makeMessageId()
{
    stringstream stream;
    VPLTime_t currTimeMillis = VPLTIME_TO_MILLISEC(VPLTime_GetTime());
    stream << "vsTest-" << currTimeMillis;
    return string(stream.str());
}

template<class RequestT> void
setIasAbstractRequestFields(RequestT& request)
{
    request.mutable__inherited()->set_version("2.0");
    request.mutable__inherited()->set_country("US");
    request.mutable__inherited()->set_language("en");
    request.mutable__inherited()->set_region("US");
    request.mutable__inherited()->set_messageid(makeMessageId());
}

static VPLMutex_t infra_mutex;
static stack<VPLIas_ProxyHandle_t> iasproxies;

void vsTest_infra_init(void)
{
    VPLMutex_Init(&infra_mutex);
}

void vsTest_infra_destroy(void)
{
    while(!iasproxies.empty()) {
        VPLIas_ProxyHandle_t iasproxy  = iasproxies.top();
        iasproxies.pop();
        VPLIas_DestroyProxy(iasproxy);
    }
            
    VPLMutex_Destroy(&infra_mutex);
}

int userLogin(const std::string& ias_name, u16 port, 
              const std::string& user, const std::string& ns, 
              const std::string& pass,
              u64& uid, vplex::vsDirectory::SessionInfo& session,
              std::string* iasTicket)
{
    int rc, rv = 0;
    vplex::ias::LoginRequestType loginReq;
    vplex::ias::LoginResponseType loginRes;
    VPLIas_ProxyHandle_t iasproxy;

    // Testing VSDS login operation
    // Use the VPLIAS API to log in to infrastructure.
    // Relying on VPL unit testing to cover testing for login.
    
    setIasAbstractRequestFields(loginReq);
    loginReq.set_username(user);
    loginReq.set_namespace_(ns);
    loginReq.set_password(pass);

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), port, &iasproxy);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, 
                             "VPLIas_CreateProxy returned %d.", rv);
            rv++;
            VPLMutex_Unlock(&infra_mutex);
            goto fail;
        }
    }
    else {
        iasproxy = iasproxies.top();
        iasproxies.pop();
    }
    VPLMutex_Unlock(&infra_mutex);

    rc = VPLIas_Login(iasproxy, VPLTIME_FROM_SEC(30), loginReq, loginRes);

    VPLMutex_Lock(&infra_mutex);
    // Put proxy on stack for re-use
    iasproxies.push(iasproxy);
    VPLMutex_Unlock(&infra_mutex);

    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed login: %d", rc);
        rv++;
        goto fail;
    }
    else {
        VPLUser_SessionSecret_t session_secret;
        string service = "Virtual Storage";
        size_t encode_len = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(sizeof(session_secret));
        char secret_data_encoded[encode_len];
        VPLUser_ServiceTicket_t ticket_data;
        CSL_ShaContext context;

        session.set_sessionhandle(loginRes.sessionhandle());

        memcpy(session_secret, loginRes.sessionsecret().data(),
               VPL_USER_SESSION_SECRET_LENGTH);
        // Compute service ticket like so:
        // ticket = SHA1(base64(session_secret) + "Virtual Storage")
        VPL_EncodeBase64(session_secret, sizeof(session_secret),
                         secret_data_encoded, &encode_len, false, false);
        CSL_ResetSha(&context);
        CSL_InputSha(&context, secret_data_encoded, encode_len - 1); // remove terminating NULL
        CSL_InputSha(&context, service.data(), service.size());
        CSL_ResultSha(&context, (unsigned char*)ticket_data);
        session.set_serviceticket(ticket_data, VPL_USER_SERVICE_TICKET_LENGTH);
        uid = loginRes.userid();

        if(iasTicket) {
            service = "IAS";
            CSL_ResetSha(&context);
            CSL_InputSha(&context, secret_data_encoded, encode_len - 1); // remove terminating NULL
            CSL_InputSha(&context, service.data(), service.size());
            CSL_ResultSha(&context, (unsigned char*)ticket_data);

            iasTicket->assign(ticket_data, VPL_USER_SERVICE_TICKET_LENGTH);
        }
    }

 fail:
    return rv;
}

// bug 10397
// register this instance of vsTest as a new device
int registerAsDevice(const std::string& ias_name, u16 port, 
                     const std::string& user, const std::string& pass,
                     u64& testDeviceId, const char *inst_hw_info)
{
    int rv = 0;
    vplex::ias::RegisterVirtualDeviceRequestType registerRequest;
    vplex::ias::RegisterVirtualDeviceResponseType registerResponse;
    VPLIas_ProxyHandle_t iasproxy;

    setIasAbstractRequestFields(registerRequest);
    registerRequest.set_username(user);
    registerRequest.set_password(pass);
    registerRequest.set_hardwareinfo((inst_hw_info) ? inst_hw_info : "vstest hardware");
    registerRequest.set_devicename("vstest device");

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), port, &iasproxy);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, 
                             "VPLIas_CreateProxy returned %d.", rv);
            rv++;
            VPLMutex_Unlock(&infra_mutex);
            goto fail;
        }
    }
    else {
        iasproxy = iasproxies.top();
        iasproxies.pop();
    }
    VPLMutex_Unlock(&infra_mutex);

    rv = VPLIas_RegisterVirtualDevice(iasproxy, VPLTIME_FROM_SEC(30), registerRequest, registerResponse);

    VPLMutex_Lock(&infra_mutex);
    // Put proxy on stack for re-use
    iasproxies.push(iasproxy);
    VPLMutex_Unlock(&infra_mutex);

    if (rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, 
                         "VPLIas_RegisterVirtuaLDevice returned %d.", rv);
        goto fail;
    }
    
    testDeviceId = registerResponse._inherited().deviceid();

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Successfully registered as deviceId "FMTu64".", testDeviceId);

 fail:       
    return rv;
}

int getAnsLoginBlob(const std::string& ias_name, u16 port, 
                    u64 sessionHandle,
                    const std::string& iasTicket,
                    u64 deviceId,
                    std::string& ansSessionKey,
                    std::string& ansLoginBlob)
{
    int rv = 0;
    vplex::ias::GetSessionKeyRequestType iasInput;
    vplex::ias::GetSessionKeyResponseType iasOutput;
    VPLIas_ProxyHandle_t iasproxy;

    setIasAbstractRequestFields(iasInput);    
    iasInput.mutable__inherited()->set_sessionhandle(sessionHandle);
    iasInput.mutable__inherited()->set_serviceticket(iasTicket);
    iasInput.mutable__inherited()->set_serviceid("IAS");
    iasInput.mutable__inherited()->set_deviceid(deviceId);
    iasInput.set_type("ANS");

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), port, &iasproxy);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, 
                             "VPLIas_CreateProxy returned %d.", rv);
            rv++;
            VPLMutex_Unlock(&infra_mutex);
            goto fail;
        }
    }
    else {
        iasproxy = iasproxies.top();
        iasproxies.pop();
    }
    VPLMutex_Unlock(&infra_mutex);

    rv = VPLIas_GetSessionKey(iasproxy, VPLTIME_FROM_SEC(30), iasInput, iasOutput);

    VPLMutex_Lock(&infra_mutex);
    // Put proxy on stack for re-use
    iasproxies.push(iasproxy);
    VPLMutex_Unlock(&infra_mutex);

    if (rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, 
                         "VPLIas_GetSessionKey returned %d.", rv);
        goto fail;
    }
    
    ansSessionKey.assign(iasOutput.sessionkey());
    // The IAS GetSessionKey field "EncryptedSessionKey" is actually the ANS login blob.
    ansLoginBlob.assign(iasOutput.encryptedsessionkey());

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Successfully retrieved session keys.");

 fail:       
    return rv;
}

int getOwnedTitles(VPLVsDirectory_ProxyHandle_t& proxy,
                   const vplex::vsDirectory::SessionInfo& session,
                   const vplex::vsDirectory::Localization& l10n,
                   std::vector<vplex::vsDirectory::TitleDetail>& ownedTitles)
{
    int rv = 0;
    int rc;

    vplex::vsDirectory::GetOwnedTitlesInput listReq;
    vplex::vsDirectory::GetOwnedTitlesOutput listResp;
    vplex::vsDirectory::SessionInfo* req_session = listReq.mutable_session();
    vplex::vsDirectory::Localization* req_l10n = listReq.mutable_l10n();
    
    *req_session = session;
    *req_l10n = l10n;
    
    rc = VPLVsDirectory_GetOwnedTitles(proxy, VPLTIME_FROM_SEC(30),
                                       listReq, listResp);
    
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "GetTitles query returned %d, detail:%d:%s",
                         rc, listResp.error().errorcode(),
                         listResp.error().errordetail().c_str());
        rv++;
    }
    else {
        // For all known titles, get info.
        vplex::vsDirectory::GetTitleDetailsInput detailReq;
        vplex::vsDirectory::GetTitleDetailsOutput detailResp;
        req_session = detailReq.mutable_session();
        req_l10n = detailReq.mutable_l10n();
        
        *req_session = session;
        *req_l10n = l10n;
        
        for(int i = 0; i < listResp.titledata_size(); i++) {
            detailReq.add_titleids(listResp.titledata(i).titleid());
        }
        
        rc = VPLVsDirectory_GetTitleDetails(proxy, VPLTIME_FROM_SEC(30),
                                            detailReq, detailResp);
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "GetTitleDetails query returned %d, detail:%d:%s",
                             rc, detailResp.error().errorcode(),
                             detailResp.error().errordetail().c_str());
            rv++;
        }
        else {
            for(int i = 0; i < detailResp.titledetails_size(); i++) {
                ownedTitles.push_back(detailResp.titledetails(i));
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "Found title %s, ID:%s",
                                    detailResp.titledetails(i).name().c_str(),
                                    detailResp.titledetails(i).titleid().c_str());
            }
        }
    }

    return rv;
}

int unsubscribeDataset(VPLVsDirectory_ProxyHandle_t& proxy,
                       const vplex::vsDirectory::SessionInfo& session,
                       u64 uid, u64 testDeviceId, u64 datasetId)
{
    vplex::vsDirectory::DeleteSubscriptionsInput req;
    vplex::vsDirectory::DeleteSubscriptionsOutput resp;
    vplex::vsDirectory::SessionInfo* req_session = req.mutable_session();
    int rv = 0;

    *req_session = session;
    req.set_userid(uid);
    req.set_deviceid(testDeviceId);
    req.add_datasetids(datasetId);

    rv = VPLVsDirectory_DeleteSubscriptions(proxy, VPLTIME_FROM_SEC(30),
                                            req, resp);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "DeleteSubscriptions query returned %d, detail:%d:%s",
                         rv, resp.error().errorcode(),
                         resp.error().errordetail().c_str());
    }

    return rv;
}

int updatePSNConnection(VPLVsDirectory_ProxyHandle_t& proxy,
                        u64 userId,
                        u64 clusterId,
                        const std::string& hostname,
                        u16 vssiPort,
                        u16 secureClearfiPort,
                        const vplex::vsDirectory::SessionInfo& session)
{
    int rv = 0;

    vplex::vsDirectory::UpdateStorageNodeConnectionInput query;
    vplex::vsDirectory::UpdateStorageNodeConnectionOutput response;
    vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();

    *query_session = session;
    query.set_userid(userId);
    query.set_clusterid(clusterId);
    query.set_reportedname(hostname);
    query.set_reportedport(vssiPort);
    query.set_reportedhttpport(0); // unused, but required.
    query.set_reportedclearfiport(0); // unused, but required
    query.set_reportedclearfisecureport(secureClearfiPort);
    query.set_accesshandle(session.sessionhandle());
    query.set_accessticket(session.serviceticket());

    rv = VPLVsDirectory_UpdateStorageNodeConnection(proxy,
                                                    VPLTIME_FROM_SEC(30),
                                                    query, response);
    if(rv != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "VPLVsDirectory_UpdateStorageNodeConnection() failed with code %d.", rv);
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Updated connection info for cluster "FMTx64", host %s, vssi_port %u, secure_clearfi_port %u",
                            clusterId, hostname.c_str(), vssiPort, secureClearfiPort);
    }

    return rv;
}
