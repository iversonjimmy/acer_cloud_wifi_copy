//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <vpl_plat.h>
#include "VcsTestSession.hpp"

#include "VcsTestUtils.hpp"

#include "cslsha.h"
#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include "gvm_misc_utils.h"
#include "protobuf_file_reader.hpp"
#include "protobuf_file_writer.hpp"
#include "scopeguard.hpp"
#include "vpl_fs.h"
#include "vplex_ias.hpp"
#include "vplex_serialization.h"
#include "vplex_user.h"
#include "vplex_vs_directory.h"

#include <iostream>
#include <sstream>
#include <string>

#include "log.h"

static std::string makeMessageId()
{
    std::stringstream stream;
    VPLTime_t currTimeMillis = VPLTIME_TO_MILLISEC(VPLTime_GetTime());
    stream << "SCWTest-" << currTimeMillis;
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

//------------------------------------------------------------
// code from linkedDevices.cpp
// code from vsTest_infra.cpp (with minor modifications)
static int userLogin(const std::string& ias_name,
                     u16 port,
                     const std::string& user,
                     const std::string& ns,
                     const std::string& pass,
                     u64& clusterId_out,
                     u64& uid_out,
                     u64& sessionHandle_out,
                     std::string& serviceTicket_out)
{
    int rc, rv = 0;
    VPLIas_ProxyHandle_t iasproxy;
    vplex::ias::LoginRequestType loginReq;
    vplex::ias::LoginResponseType loginRes;
    sessionHandle_out = 0;
    serviceTicket_out.clear();

    // Testing VSDS login operation
    // Use the VPLIAS API to log in to infrastructure.
    // Relying on VPL unit testing to cover testing for login.
    rc = VPLIas_CreateProxy(ias_name.c_str(), port, &iasproxy);
    if (rc != 0) {
        LOG_ERROR("VPLIas_CreateProxy returned %d.", rv);
        rv=rc;
        goto fail;
    }

    setIasAbstractRequestFields(loginReq);
    loginReq.set_username(user);
    loginReq.set_namespace_(ns);
    loginReq.set_password(pass);
    rc = VPLIas_Login(iasproxy, VPLTIME_FROM_SEC(30), loginReq, loginRes);
    if(rc != 0) {
        LOG_ERROR("FAIL: Failed login: %d", rc);
        rv = rc;
        goto fail;
    }
    else {
        VPLUser_SessionSecret_t session_secret;
        std::string service = "Virtual Storage";
        const size_t const_encode_len = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(sizeof(session_secret));
        char secret_data_encoded[const_encode_len];
        size_t encode_len = const_encode_len;
        VPLUser_ServiceTicket_t ticket_data;
        CSL_ShaContext context;

        sessionHandle_out = loginRes.sessionhandle();

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
        serviceTicket_out.assign(ticket_data, VPL_USER_SERVICE_TICKET_LENGTH);
        uid_out = loginRes.userid();
        clusterId_out = loginRes.storageclusterid();
    }

 fail:
    rc = VPLIas_DestroyProxy(iasproxy);
    if(rc != 0) {
        LOG_ERROR("FAIL: Failed to destroy IAS proxy: %d", rc);
        if(rv == 0) {
            rv=rc;
        }
    }

    return rv;
}

// COPY: This function is identical to the one in sw_x/tests/dxshell/vcs_test.cpp
int vcsGetTestSession(const std::string& domain,
                      int domainPort,
                      const std::string& ns,
                      const std::string& user,
                      const std::string& password,
                      VcsTestSession& vcsTestSession_out)
{
    vcsTestSession_out.clear();

    u64 userId_out = 0;
    u64 sessionHandle_out = 0;
    u64 clusterId_out = 0;
    std::string sessionServiceTicket_out;
    // See CCD_GET_INFRA_CENTRAL_HOSTNAME in sw_x/gvm_core/daemons/ccd/src_impl/query.h
    std::string ias_url = std::string("www.")+domain;
    std::string vcs_url;
    int rv = 0;
    int rc;

    rc = userLogin(ias_url,
                   domainPort,
                   user,
                   ns,  // Very likely "acer"
                   password,
                   clusterId_out,
                   userId_out,
                   sessionHandle_out,
                   sessionServiceTicket_out);
    if(rc != 0) {
        LOG_ERROR("userLogin failed (%s, %s, %s): %d",
                  ias_url.c_str(), user.c_str(), password.c_str(),
                  rc);
        rv = rc;
        return rv;
    }

    {   // Figure out vcs_url
        char clusterIdBuf[64];
        snprintf(clusterIdBuf, ARRAY_SIZE_IN_BYTES(clusterIdBuf), FMTs64, clusterId_out);
        vcs_url = std::string("www-c")+
                  std::string(clusterIdBuf)+
                  std::string(".")+
                  domain;
    }

    vcsTestSession_out.userId = userId_out;
    vcsTestSession_out.username = user;
    vcsTestSession_out.sessionHandle = sessionHandle_out;
    vcsTestSession_out.sessionServiceTicket = sessionServiceTicket_out;

    {  // Encode the session into base64
        char* ticket64;
        rc = Util_EncodeBase64(sessionServiceTicket_out.data(),
                               sessionServiceTicket_out.size(),
                               &ticket64, NULL, VPL_FALSE, VPL_FALSE);
        if (rc < 0) {
            LOG_ERROR("error %d when generating service ticket", rv);
            rv = rc;
            return rv;
        }
        ON_BLOCK_EXIT(free, ticket64);
        vcsTestSession_out.sessionEncodedBase64.assign(ticket64);
    }

    vcsTestSession_out.serverHostname.assign(vcs_url);

    // Always use http secure.  For now, assuming https is always used.
    vcsTestSession_out.urlPrefix.assign("https://");
    vcsTestSession_out.urlPrefix.append(vcs_url);
    vcsTestSession_out.urlPrefix.append(":443");

    return rv;
}

// COPY: This function is identical to the one in sw_x/tests/dxshell/vcs_test.cpp
// read by readVcsTestSessionFile
int saveVcsTestSessionFile(const std::string& sessionFilepath,
                           const VcsTestSession& session)
{
    int rv = 0;
    int rc = VPLFile_Delete(sessionFilepath.c_str());
    if(rc != 0) {
        LOG_WARN("VPLFile_Delete %s:%d, File may not exist.  Not critical, continue.",
                 sessionFilepath.c_str(), rc);
    }
    rc = Util_CreatePath(sessionFilepath.c_str(), false);
    if(rc != 0){
        LOG_ERROR("Util_CreatePath %s:%d", sessionFilepath.c_str(), rc);
        return rc;
    }
    {
        ProtobufFileWriter writer;
        rv = writer.open(sessionFilepath.c_str(), (VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR));
        if (rv != 0) {
            LOG_ERROR("Failed to open \"%s\" for writing: %d", sessionFilepath.c_str(), rv);
            return rv;
        }
        google::protobuf::io::CodedOutputStream tempStream(writer.getOutputStream());

        tempStream.WriteVarint64(session.userId);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session userId:"FMTu64, session.userId);
            return -1;
        }

        tempStream.WriteVarint32(session.username.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session username prepend size:"FMTu_size_t, session.username.size());
            return -2;
        }
        tempStream.WriteString(session.username);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session username:%s", session.username.c_str());
            return -3;
        }

        tempStream.WriteVarint32(session.sessionEncodedBase64.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session sessionEncodedBase64 prepend size:"FMTu_size_t, session.sessionEncodedBase64.size());
            return -4;
        }
        tempStream.WriteString(session.sessionEncodedBase64);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session sessionEncodedBase64:%s", session.sessionEncodedBase64.c_str());
            return -5;
        }

        tempStream.WriteVarint32(session.urlPrefix.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session urlPrefix prepend size:"FMTu_size_t, session.urlPrefix.size());
            return -6;
        }
        tempStream.WriteString(session.urlPrefix);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session urlPrefix:%s", session.urlPrefix.c_str());
            return -7;
        }

        tempStream.WriteVarint64(session.sessionHandle);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs sessionHandle");
            return -8;
        }

        tempStream.WriteVarint32(session.sessionServiceTicket.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session prepend sessionServiceTicket size:"FMTu_size_t, session.sessionServiceTicket.size());
            return -9;
        }
        tempStream.WriteString(session.sessionServiceTicket);
        if(tempStream.HadError()) {
            LOG_ERROR("Write sessionServiceTicket failed");
            return -10;
        }

        tempStream.WriteVarint32(session.serverHostname.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session prepend serverHostname size:"FMTu_size_t, session.serverHostname.size());
            return -11;
        }
        tempStream.WriteString(session.serverHostname);
        if(tempStream.HadError()) {
            LOG_ERROR("Write serverHostname failed");
            return -12;
        }
    }
    LOG_ALWAYS("Saved session to %s", sessionFilepath.c_str());
    return rv;
}

// COPY: This function is identical to the one in sw_x/tests/dxshell/vcs_test.cpp
// See saveVcsTestSessionFile for what this function is reading.
int readVcsTestSessionFile(const std::string& sessionFilepath,
                           bool printLog,
                           VcsTestSession& session_out)
{
    int rc;
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(sessionFilepath.c_str(), &statBuf);
    if(rc != 0) {
        LOG_ERROR("Stat of %s failed:%d", sessionFilepath.c_str(), rc);
        return rc;
    }
    if(statBuf.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("Is not file:%s", sessionFilepath.c_str());
        return -20;
    }
    {
        ProtobufFileReader reader;
        rc = reader.open(sessionFilepath.c_str(), true);
        if(rc != 0) {
            LOG_ERROR("Could not open %s:%d", sessionFilepath.c_str(), rc);
            return rc;
        }

        u32 prependLength;
        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());

        if(!tempStream.ReadVarint64(&session_out.userId)) {
            LOG_ERROR("Failed to read userId");
            return -1;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend username");
            return -2;
        }
        if(!tempStream.ReadString(&session_out.username, prependLength)) {
            LOG_ERROR("Failed to read username");
            return -3;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend sessionEncodedBase64");
            return -4;
        }
        if(!tempStream.ReadString(&session_out.sessionEncodedBase64, prependLength)) {
            LOG_ERROR("Failed to read sessionEncodedBase64");
            return -5;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend urlPrefix");
            return -6;
        }
        if(!tempStream.ReadString(&session_out.urlPrefix, prependLength)) {
            LOG_ERROR("Failed to read urlPrefix");
            return -7;
        }

        if(!tempStream.ReadVarint64(&session_out.sessionHandle)) {
            LOG_ERROR("Failed to read sessionHandle");
            return -8;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend sessionServiceTicket");
            return -9;
        }
        if(!tempStream.ReadString(&session_out.sessionServiceTicket, prependLength)) {
            LOG_ERROR("Failed to read sessionServiceTicket");
            return -10;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend serverHostname");
            return -11;
        }
        if(!tempStream.ReadString(&session_out.serverHostname, prependLength)) {
            LOG_ERROR("Failed to read serverHostname");
            return -12;
        }
    }

    if(printLog) {
        LOG_ALWAYS("Read session state from %s\n"
                   "         username:%s\n"
                   "         userId:"FMTu64"\n"
                   "         serverHostname:%s\n"
                   "         urlPrefix:%s\n"
                   "         sessionHandle:"FMTs64"\n"
                   "         Session file created on "FMTu64,
                   sessionFilepath.c_str(),
                   session_out.username.c_str(),
                   session_out.userId,
                   session_out.serverHostname.c_str(),
                   session_out.urlPrefix.c_str(),
                   session_out.sessionHandle,
                   (u64)statBuf.ctime);
    }
    return 0;
}

static const int DEFAULT_VCS_GET_SESSION_PORT = 443;

int vcsGetDatasetIdFromName(const VcsTestSession& vcsTestSession,
                            const std::string& datasetName,
                            u64& datasetId_out)
{
    // TODO: will this work if we don't link the device?
    int rv = 0;
    int rc;
    datasetId_out = 0;

    // Create VSDS proxy.
    VPLVsDirectory_ProxyHandle_t vsds_proxyHandle;
    rc = VPLVsDirectory_CreateProxy(vcsTestSession.serverHostname.c_str(),
                                    DEFAULT_VCS_GET_SESSION_PORT,
                                    &vsds_proxyHandle);
    if(rc != 0) {
        LOG_ERROR("VPLVsDirectory_CreateProxy:%d", rc);
        return rc;
    }

    // Get user's datasets.
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::ListOwnedDataSetsInput listDatasetReq;
    vplex::vsDirectory::ListOwnedDataSetsOutput listDatasetResp;

    req_session = listDatasetReq.mutable_session();
    req_session->set_sessionhandle(vcsTestSession.sessionHandle);
    req_session->set_serviceticket(vcsTestSession.sessionServiceTicket);
    listDatasetReq.set_userid(vcsTestSession.userId);
    // http://intwww/wiki/index.php/VSDS#ListOwnedDataSets
    // version needs to be "3.0" or greater for "Media MetaData VCS" dataset.
    listDatasetReq.set_version("3.0");
    //listDatasetReq.set_deviceclass("GVM");
    rc = VPLVsDirectory_ListOwnedDataSets(vsds_proxyHandle, VPLTIME_FROM_SEC(30),
                                          listDatasetReq, listDatasetResp);
    if(rc != 0) {
        LOG_ERROR("FAIL:ListOwnedDatasets query returned %d, detail:%d:%s",
                  rc, listDatasetResp.error().errorcode(),
                  listDatasetResp.error().errordetail().c_str());
        rv = rc;
        goto exit;
    }

    // Find the wanted datasetId
    LOG_INFO("User %s:"FMTu64" has %d datasets.",
             vcsTestSession.username.c_str(),
             vcsTestSession.userId,
             listDatasetResp.datasets_size());
    for(int i = 0; i < listDatasetResp.datasets_size(); i++) {
        LOG_INFO("Found dataset %s, ID:"FMTu64" stored at cluster %s, reachable at %s:%u.",
                 listDatasetResp.datasets(i).datasetname().c_str(),
                 listDatasetResp.datasets(i).datasetid(),
                 listDatasetResp.datasets(i).storageclustername().c_str(),
                 listDatasetResp.datasets(i).storageclusterhostname().c_str(),
                 listDatasetResp.datasets(i).storageclusterport());
        // Remember the dataset for HTTP testing.
        if(listDatasetResp.datasets(i).datasetname()==datasetName) {
            datasetId_out = listDatasetResp.datasets(i).datasetid();
            break;
        }
    }

    if(datasetId_out==0) {
        LOG_ERROR("No datasetId found.");
        rv = CCD_ERROR_DATASET_NOT_FOUND;
    }

 exit:
    rc = VPLVsDirectory_DestroyProxy(vsds_proxyHandle);
    if(rc != 0) {
        LOG_ERROR("VPLVsDirectory_DestroyProxy:%d.  Continuing.", rc);
    }

    return rv;
}

void getVcsSessionFromVcsTestSession(const VcsTestSession& vcsTestSession,
                                     VcsSession& vcsSession_out)
{
    vcsSession_out.userId = vcsTestSession.userId;
    // TODO: Bug 11568: Set deviceId here?
    // vcsSession_out.deviceId = ?;
    vcsSession_out.urlPrefix = vcsTestSession.urlPrefix;
    vcsSession_out.sessionHandle = vcsTestSession.sessionHandle;
    vcsSession_out.sessionServiceTicket = vcsTestSession.sessionServiceTicket;
}
