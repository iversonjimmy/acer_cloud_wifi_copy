/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD TECHNOLOGY INC.
 *
 */

#include "test_server.hpp"

#include "vplex_trace.h"
#include "vplex_vs_directory.h"
#include "vplex_ans_message.pb.h"
#include "vsTest_infra.hpp"
#include "loadgen_proxy.hpp"

#include "test_client.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

#include "vss_comm.h"

using namespace std;

extern string infra_name;
extern u16 infra_port;
extern u64 uid;
extern u64 testDeviceId;
extern vplex::vsDirectory::SessionInfo session;
extern string iasTicket;
extern string psnServiceTicket;

char encryption_key[CSL_AES_KEYSIZE_BYTES];
char signing_key[CSL_SHA1_DIGESTSIZE];

test_server::test_server()
{
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Created test server.");
    ans_name = infra_name;
    ans_name.replace(0, 3, "ans"); // www to ans.

    // compute encryption, signing keys for device-specific service ticket
    string text;
    {
        stringstream stream;
        stream << "Encryption Key " 
               << hex << uppercase <<setfill('0') << setw(16) << testDeviceId << " "
               << hex << uppercase <<setfill('0') << setw(16) << session.sessionhandle();
        text = stream.str();
    }
    compute_hmac((char*)(text.data()), text.size(),
                 psnServiceTicket.data(),
                 encryption_key, CSL_AES_KEYSIZE_BYTES);
    
    {
        stringstream stream;
        stream << "Signing Key " 
               << hex << uppercase <<setfill('0') << setw(16) << testDeviceId << " "
               << hex << uppercase <<setfill('0') << setw(16) << session.sessionhandle();
        text = stream.str();
    }
    compute_hmac((char*)(text.data()), text.size(),
                 psnServiceTicket.data(),
                 signing_key, CSL_SHA1_DIGESTSIZE);

    
}

test_server::~test_server()
{
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Destroyed test server.");
}

static VPL_BOOL connectionActiveCb(ans_client_t* client, VPLNet_addr_t localAddr)
{
    return VPL_TRUE;
}

static
VPL_BOOL processProxyNotification(const char* proxy_request)
{
    // Verify, decrypt the request.
    size_t data_length = vss_get_data_length(proxy_request);
    char* request = new char[data_length];
    decrypt_data(request, proxy_request + VSS_HEADER_SIZE,
                 data_length, proxy_request, encryption_key);

    // Start test_client for proxy connection.
    test_client* client = new test_client(proxy_request, request);

    if(client->start() != 0) {
        delete client;
    }
    delete request;

    return VPL_TRUE;
}

static VPL_BOOL receiveNotificationCb(ans_client_t* client, ans_unpacked_t* notification)
{
    u8 type = ((u8*)notification->notification)[0];

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "See notification of type %d.", type);

    if(type == vplex::ans::ANS_TYPE_PROXY) {
        return processProxyNotification(((const char*)notification->notification) + 1);
    }
    return VPL_TRUE;
}

static void receiveSleepInfoCb(ans_client_t *client, ans_unpacked_t *notification)
{}

static void receiveDeviceStateCb(ans_client_t *client, ans_unpacked_t *notification)
{}

static void connectionDownCb(ans_client_t* client)
{}

static void connectionClosedCb(ans_client_t* client)
{}

static void setPingPacketCb(ans_client_t *client, char *pingPacket, int pingLength)
{}

static void rejectCredentialsCb(ans_client_t *client)
{}

static void loginCompletedCb(ans_client_t *client) 
{}

static void rejectSubscriptionsCb(ans_client_t *client)
{}

static void receiveResponseCb(ans_client_t *client, ans_unpacked_t *response)
{
}

static const ans_callbacks_t callbacks = {
    connectionActiveCb,
    receiveNotificationCb,
    receiveSleepInfoCb,
    receiveDeviceStateCb,
    connectionDownCb,
    connectionClosedCb,
    setPingPacketCb,
    rejectCredentialsCb,
    loginCompletedCb,
    rejectSubscriptionsCb,
    receiveResponseCb
};

int test_server::start()
{
    int rv = getAnsLoginBlob(infra_name, infra_port,
                             session.sessionhandle(), iasTicket, testDeviceId,
                             ansSessionKey, ansLoginBlob);
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "TC_RESULT = %s ;;; TC_NAME = Get_ANS_Login_Blob",
                        rv ? "FAIL" : "PASS");
    if(rv) {
        goto exit;
    }

    ans_input.clusterName = ans_name.c_str();
    ans_input.callbacks   = &callbacks;
    ans_input.blob        = ansLoginBlob.data();
    ans_input.blobLength  = static_cast<int>(ansLoginBlob.size());
    ans_input.key         = ansSessionKey.data();
    ans_input.keyLength   = static_cast<int>(ansSessionKey.size());
    ans_input.deviceType  = "default";
    ans_input.application = "test";
    ans_input.verbose     = VPL_TRUE;

    ans_client = ans_open(&ans_input);

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Start test server activity.");
 exit:
    return rv;
}

void test_server::stop()
{
    if(ans_client) {
        ans_close(ans_client, VPL_TRUE);
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Stop test server activity.");
}
