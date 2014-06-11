//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#include "PxdTestInfraHelper.hpp"

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

PxdTest::InfraHelper::InfraHelper(const std::string& _username,
                                  const std::string& _password,
                                  const std::string& _deviceInfo,
                                  const std::string& _ns,
                                  const std::string& _ans_name,
                                  const std::string& _pxd_name,
                                  const std::string& _vsds_name,
                                  const u16 _vsds_port,
                                  const std::string& _ias_name,
                                  const u16 _ias_port) :
    username(_username),
    password(_password),
    device_info(_deviceInfo),
    ns(_ns),
    ans_name(_ans_name),
    pxd_name(_pxd_name),
    vsds_name(_vsds_name),
    vsds_port(_vsds_port),
    ias_name(_ias_name),
    ias_port(_ias_port),
    userId(0),
    deviceId(0)
{
    VPLMutex_Init(&infra_mutex);
}

PxdTest::InfraHelper::~InfraHelper()
{
    while(!iasproxies.empty()) {
        VPLIas_ProxyHandle_t iasproxy  = iasproxies.top();
        iasproxies.pop();
        VPLIas_DestroyProxy(iasproxy);
    }

    while(!vsdsproxies.empty()) {
        VPLVsDirectory_ProxyHandle_t vsdsproxy = vsdsproxies.top();
        vsdsproxies.pop();
        VPLVsDirectory_DestroyProxy(vsdsproxy);
    }

    VPLMutex_Destroy(&infra_mutex);
}

static std::string makeMessageId()
{
    std::stringstream stream;
    VPLTime_t currTimeMillis = VPLTIME_TO_MILLISEC(VPLTime_GetTime());
    stream << "pxdCommon-" << currTimeMillis;
    return std::string(stream.str());
}

template<class RequestT>
static void
setIasAbstractRequestFields(RequestT& request)
{
    request.mutable__inherited()->set_version("2.0");
    request.mutable__inherited()->set_country("US");
    request.mutable__inherited()->set_language("en");
    request.mutable__inherited()->set_region("US");
    request.mutable__inherited()->set_messageid(makeMessageId());
}

int PxdTest::InfraHelper::userLogin()
{
    int rc, rv = 0;
    vplex::ias::LoginRequestType loginReq;
    vplex::ias::LoginResponseType loginRes;
    VPLIas_ProxyHandle_t iasproxy;

    // Testing VSDS login operation
    // Use the VPLIAS API to log in to infrastructure.
    // Relying on VPL unit testing to cover testing for login.

    setIasAbstractRequestFields(loginReq);
    loginReq.set_username(username);
    loginReq.set_namespace_(ns);
    loginReq.set_password(password);

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), ias_port, &iasproxy);
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
        char *secret_data_encoded = (char*)malloc(encode_len);
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
        userId = loginRes.userid();

        service = "IAS";
        CSL_ResetSha(&context);
        CSL_InputSha(&context, secret_data_encoded, encode_len - 1); // remove terminating NULL
        CSL_InputSha(&context, service.data(), service.size());
        CSL_ResultSha(&context, (unsigned char*)ticket_data);

        iasTicket.assign(ticket_data, VPL_USER_SERVICE_TICKET_LENGTH);

        free(secret_data_encoded);
    }

 fail:
    return rv;
}
int PxdTest::InfraHelper::registerAsDevice()
{
    int rv = 0;
    vplex::ias::RegisterVirtualDeviceRequestType registerRequest;
    vplex::ias::RegisterVirtualDeviceResponseType registerResponse;
    VPLIas_ProxyHandle_t iasproxy;

    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                      "DevoceInfo: %s",device_info.c_str());

    setIasAbstractRequestFields(registerRequest);
    registerRequest.set_username(username);
    registerRequest.set_password(password);
    registerRequest.set_hardwareinfo(device_info);
    registerRequest.set_devicename(device_info);

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), ias_port, &iasproxy);
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

    deviceId = registerResponse._inherited().deviceid();

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Successfully registered as deviceId "FMTu64".", deviceId);

 fail:
    return rv;
}

int PxdTest::InfraHelper::linkDevice()
{
    int rv = 0;
    int rc = 0;
    VPLVsDirectory_ProxyHandle_t vsdsproxy;
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::LinkDeviceInput linkReq;
    vplex::vsDirectory::LinkDeviceOutput linkResp;

    vplex::vsDirectory::UnlinkDeviceInput unlinkReq;
    vplex::vsDirectory::UnlinkDeviceOutput unlinkResp;
    std::vector<vplex::vsDirectory::DatasetDetail>::iterator dataset_it;

    // Unlink this device.
    req_session = unlinkReq.mutable_session();
    *req_session = session;
    unlinkReq.set_userid(userId);
    unlinkReq.set_deviceid(deviceId);

    VPLMutex_Lock(&infra_mutex);
    if(vsdsproxies.empty()) {
        rv = VPLVsDirectory_CreateProxy(vsds_name.c_str(), vsds_port, &vsdsproxy);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "VPLIas_CreateProxy returned %d.", rv);
            rv++;
            VPLMutex_Unlock(&infra_mutex);
            goto fail;
        }
    } else {
        vsdsproxy = vsdsproxies.top();
        vsdsproxies.pop();
    }
    VPLMutex_Unlock(&infra_mutex);

    rc = VPLVsDirectory_UnlinkDevice(vsdsproxy, VPLTIME_FROM_SEC(30),
                                     unlinkReq, unlinkResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "UnlinkDevice query returned %d, detail:%d:%s",
                         rc, unlinkResp.error().errorcode(),
                         unlinkResp.error().errordetail().c_str());
        rv++;
    }

    VPLThread_Sleep(VPLTime_FromSec(3));

    // Link device to the user.
    req_session = linkReq.mutable_session();
    *req_session = session;
    linkReq.set_userid(userId);
    linkReq.set_deviceid(deviceId);
    linkReq.set_hascamera(false);
    linkReq.set_isacer(true);
    linkReq.set_deviceclass("AndroidPhone");
    linkReq.set_devicename(device_info);

    rc = VPLVsDirectory_LinkDevice(vsdsproxy, VPLTIME_FROM_SEC(30),
                                   linkReq, linkResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "LinkDevice query returned %d, detail:%d:%s",
                         rc, linkResp.error().errorcode(),
                         linkResp.error().errordetail().c_str());
        rv++;
        goto fail;
    }

 fail:
    return rv;
}

int PxdTest::InfraHelper::ConnectInfra(u64& _userId, u64& _deviceId) {
    int rv = 0;
    rv = userLogin();
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,"userLogin() failed: %d", rv);
        goto fail;
    }

    rv = registerAsDevice();
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,"registerAsDevice() failed: %d", rv);
        goto fail;
    }

    rv = linkDevice();
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,"linkDevice() failed: %d", rv);
        goto fail;
    }
    _userId = userId;
    _deviceId = deviceId;
fail:
    return rv;
}

int PxdTest::InfraHelper::GetPxdLoginBlob(string inst_id,
                                          std::string ansLoginBlob,
                                          std::string& pxdSessionKey,
                                          std::string& pxdLoginBlob)
{
    int rv = 0;
    vplex::ias::GetSessionKeyRequestType iasInput;
    vplex::ias::GetSessionKeyResponseType iasOutput;
    VPLIas_ProxyHandle_t iasproxy;

    setIasAbstractRequestFields(iasInput);
    iasInput.mutable__inherited()->set_sessionhandle(session.sessionhandle());
    iasInput.mutable__inherited()->set_serviceticket(iasTicket);
    iasInput.mutable__inherited()->set_serviceid("IAS");
    iasInput.mutable__inherited()->set_deviceid(deviceId);
    iasInput.set_type("PXD");
    iasInput.set_encryptedsessionkey(ansLoginBlob);

    vplex::ias::StrAttributeType* sat = iasInput.add_keyattributes();
    sat = iasInput.add_keyattributes();
    sat->set_attributename("DeviceType");
    sat->set_attributevalue(DEVICE_TYPE_CODE);

    sat = iasInput.add_keyattributes();
    sat->set_attributename("TitleId");
    sat->set_attributevalue("0");

    sat = iasInput.add_keyattributes();
    sat->set_attributename("CCDProtocolVersion");
    sat->set_attributevalue(CCD_PROTOCOL_VERSION);

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), ias_port, &iasproxy);
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

    pxdSessionKey.assign(iasOutput.sessionkey());
    // The IAS GetSessionKey field "EncryptedSessionKey" is actually the ANS login blob.
    pxdLoginBlob.assign(iasOutput.encryptedsessionkey());

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Successfully retrieved session keys.");

 fail:
    return rv;
}

int PxdTest::InfraHelper::GetCCDLoginBlob(
                    string inst_id,
                    u64 server_user_id,
                    u64 server_device_id,
                    string server_inst_id,
                    std::string ansLoginBlob,
                    std::string& ccdSessionKey,
                    std::string& ccdLoginBlob)
{
    int rv = 0;
    vplex::ias::GetSessionKeyRequestType iasInput;
    vplex::ias::GetSessionKeyResponseType iasOutput;
    VPLIas_ProxyHandle_t iasproxy;

    // for convert
    std::stringstream ss;

    setIasAbstractRequestFields(iasInput);
    iasInput.mutable__inherited()->set_sessionhandle(session.sessionhandle());
    iasInput.mutable__inherited()->set_serviceticket(iasTicket);
    iasInput.mutable__inherited()->set_serviceid("IAS");
    iasInput.mutable__inherited()->set_deviceid(deviceId);
    iasInput.set_type("CCD");
    iasInput.set_encryptedsessionkey(ansLoginBlob);

    vplex::ias::StrAttributeType* sat = iasInput.add_keyattributes();
    sat->set_attributename("CCDServerDeviceId");
    ss << server_device_id;
    sat->set_attributevalue(ss.str());

    sat = iasInput.add_keyattributes();
    ss.str("");
    ss << server_user_id;
    sat->set_attributename("CCDServerUserId");
    sat->set_attributevalue(ss.str());

#if 0
    sat = iasInput.add_keyattributes();
    sat->set_attributename("CCDServerInstanceId");
    sat->set_attributevalue(server_inst_id);
#endif

    sat = iasInput.add_keyattributes();
    sat->set_attributename("DeviceType");
    sat->set_attributevalue(DEVICE_TYPE_CODE);

    sat = iasInput.add_keyattributes();
    sat->set_attributename("TitleId");
    sat->set_attributevalue("0");

    sat = iasInput.add_keyattributes();
    sat->set_attributename("CCDProtocolVersion");
    sat->set_attributevalue(CCD_PROTOCOL_VERSION);

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), ias_port, &iasproxy);
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

    ccdSessionKey.assign(iasOutput.sessionkey());
    // The IAS GetSessionKey field "EncryptedSessionKey" is actually the ANS login blob.
    ccdLoginBlob.assign(iasOutput.encryptedsessionkey());

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Successfully retrieved session keys.");

 fail:
    return rv;
}

int PxdTest::InfraHelper::GetCCDServerKey(string inst_id, std::string& ccdServerKey)
{
    int rv = 0;
    vplex::ias::GetServerKeyRequestType iasInput;
    vplex::ias::GetServerKeyResponseType iasOutput;
    VPLIas_ProxyHandle_t iasproxy;

    setIasAbstractRequestFields(iasInput);
    iasInput.mutable__inherited()->set_sessionhandle(session.sessionhandle());
    iasInput.mutable__inherited()->set_serviceticket(iasTicket);
    iasInput.mutable__inherited()->set_serviceid("IAS");
    iasInput.mutable__inherited()->set_deviceid(deviceId);
    iasInput.set_userid(userId);

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), ias_port, &iasproxy);
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

    rv = VPLIas_GetServerKey(iasproxy, VPLTIME_FROM_SEC(30), iasInput, iasOutput);

    VPLMutex_Lock(&infra_mutex);
    // Put proxy on stack for re-use
    iasproxies.push(iasproxy);
    VPLMutex_Unlock(&infra_mutex);

    if (rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "VPLIas_GetServerKey returned %d.", rv);
        goto fail;
    }

    ccdServerKey.assign(iasOutput.serverkey());

    {
        char* buff = NULL;
        size_t buffersize = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(ccdServerKey.size());
        buff = (char*)malloc(buffersize);
        VPL_EncodeBase64(ccdServerKey.data(), ccdServerKey.size(),
                buff, &buffersize, /*addNewLines*/VPL_FALSE, /*urlSafe*/VPL_FALSE);

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "buff: %s", buff);
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Successfully retrieved server keys.");

 fail:
    return rv;
}

int PxdTest::InfraHelper::GetAnsLoginBlob(
                    std::string& ansSessionKey,
                    std::string& ansLoginBlob)
{
    int rv = 0;
    vplex::ias::GetSessionKeyRequestType iasInput;
    vplex::ias::GetSessionKeyResponseType iasOutput;
    VPLIas_ProxyHandle_t iasproxy;

    setIasAbstractRequestFields(iasInput);
    iasInput.mutable__inherited()->set_sessionhandle(session.sessionhandle());
    iasInput.mutable__inherited()->set_serviceticket(iasTicket);
    iasInput.mutable__inherited()->set_serviceid("IAS");
    iasInput.mutable__inherited()->set_deviceid(deviceId);
    iasInput.set_type("ANS");

    VPLMutex_Lock(&infra_mutex);
    // Grab a proxy. Create one if needed.
    if(iasproxies.empty()) {
        rv = VPLIas_CreateProxy(ias_name.c_str(), ias_port, &iasproxy);
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

int PxdTest::InfraHelper::GetUserStorage(u64 device_id,
                                         vplex::vsDirectory::UserStorage &userStorage)
{
    int rv = 0;
    int rc = 0;
    VPLVsDirectory_ProxyHandle_t vsdsproxy;
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::ListUserStorageInput req;
    vplex::vsDirectory::ListUserStorageOutput resp;

    VPLMutex_Lock(&infra_mutex);
    if(vsdsproxies.empty()) {
        rv = VPLVsDirectory_CreateProxy(vsds_name.c_str(), vsds_port, &vsdsproxy);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "VPLIas_CreateProxy returned %d.", rv);
            rv++;
            VPLMutex_Unlock(&infra_mutex);
            goto fail;
        }
    } else {
        vsdsproxy = vsdsproxies.top();
        vsdsproxies.pop();
    }
    VPLMutex_Unlock(&infra_mutex);

    req_session = req.mutable_session();
    *req_session = session;
    req.set_userid(userId);
    req.set_deviceid(deviceId);

    rc = VPLVsDirectory_ListUserStorage(vsdsproxy, VPLTIME_FROM_SEC(30),
                                        req, resp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ListUserStorage query returned %d, detail:%d:%s",
                         rc, resp.error().errorcode(),
                         resp.error().errordetail().c_str());
        rv++;
        goto fail;
    }

    if (resp.storageassignments_size() != 1) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ListUserStorage did not find deviceId "FMTu64, deviceId);
        rv++;
        goto fail;
    }

    userStorage = resp.storageassignments(0);

 fail:
    return rv;
}

const std::string& PxdTest::InfraHelper::GetPxdSvrName() const
{
    return pxd_name;
}

const std::string& PxdTest::InfraHelper::GetAnsSvrName() const
{
    return ans_name;
}
