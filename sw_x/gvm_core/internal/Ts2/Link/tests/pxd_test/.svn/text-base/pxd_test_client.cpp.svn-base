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
#include "pxd_test_common.hpp"
#include "ans_device.h"

typedef struct  {
    VPLSocket_t socket;
    int result;
    VPLSem_t sem;
} pxd_test_context_t;

static string username = "sefflu-cn-1@acer.org";
static string password = "4rfv5tgb";
static string ans_name = "ans-c100.pc-int.igware.net";
static string pxd_name = "pxd-c100.pc-int.igware.net";
static string vsds_name = "www-c100.pc-int.igware.net";
static u16 vsds_port = 443;
static string ias_name = "www-c100.pc-int.igware.net";
static u16 ias_port = 443;
static u64 block_size = 1048576;
static u64 bs_count = 100;

static pxd_address_t ext_address;
static pxd_client_t* pxd_client = NULL;
static u64 server_device_id = 29403555548LL;

static bool ans_login_done = false;

static int perf_test(VPLSocket_t socket, u64 bs, u64 count);

static void usage(int argc, char* argv[])
{
    printf("Usage: %s [options]\n", argv[0]);
    printf(" -u --username   USERNAME         User name\n");
    printf(" -p --password   PASSWORD         User password\n");
    printf(" -b --block-size SIZE             Mode\n");
    printf(" -c --count      COUNT            Mode\n");
    printf(" -s --svr-dev-id ID               Mode\n");
}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"svr-dev-id", required_argument, 0, 's'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"vsds-name", required_argument, 0, 'd'},
        {"vsds-port", required_argument, 0, 'q'},
        {"ias-name", required_argument, 0, 'i'},
        {"ias-port", required_argument, 0, 'r'},
        {"block-size", required_argument, 0, 'b'},
        {"count", required_argument, 0, 'c'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "s:u:p:d:q:i:r:b:c:",
                            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            break;
        case 's':
            server_device_id = stringTou64(optarg);
            break;
        case 'u':
            username = optarg;
            break;

        case 'p':
            password = optarg;
            break;

        case 'd':
            vsds_name = optarg;
            break;

        case 'q':
            vsds_port = atoi(optarg);
            break;

        case 'i':
            ias_name = optarg;
            break;

        case 'r':
            ias_port = atoi(optarg);
            break;

        case 'b':
            block_size = stringTou64(optarg);
            break;

        case 'c':
            bs_count = stringTou64(optarg);
            break;

        default:
            usage(argc, argv);
            rv = -1;
            break;
        }
    }

    return rv;
}

static void connect_done(pxd_cb_data_t *cb_data) {
    LOG_INFO("connect_done: result "FMTu64, cb_data->result);
    pxd_test_context_t* test_context = (pxd_test_context_t*)cb_data->client_opaque;
    test_context->socket = cb_data->socket;
    test_context->result = cb_data->result;
    VPLSem_Post(&(test_context->sem));
}
static void incoming_login(pxd_cb_data_t *cb_data) {
    LOG_INFO("incoming_login: result "FMTu64, cb_data->result);
}
static void incoming_request(pxd_cb_data_t *cb_data) {
    LOG_INFO("incoming_request: result "FMTu64, cb_data->result);
}
static void lookup_done(pxd_cb_data_t *cb_data) {
    LOG_INFO("lookup_done: result "FMTu64, cb_data->result);
}
static void reject_ccd_creds(pxd_cb_data_t *cb_data) {
    LOG_INFO("reject_ccd_creds: result "FMTu64, cb_data->result);
    pxd_test_context_t* test_context = (pxd_test_context_t*)cb_data->client_opaque;
    VPLSem_Post(&(test_context->sem));
}
static void reject_pxd_creds(pxd_cb_data_t *cb_data) {
    LOG_INFO("reject_pxd_creds: result "FMTu64, cb_data->result);
}
static void supply_external(pxd_cb_data_t *cb_data) {
    LOG_INFO("supply_external: result "FMTu64, cb_data->result);
    pxd_test_context_t* test_context = (pxd_test_context_t*)cb_data->client_opaque;
    for (int i = 0; i < cb_data->address_count; i++) {
        char buf[32] = {0};
        u32 address = 0;
        for (int n = 3; n >= 0 ; n--) {
            address = address << 8 | cb_data->addresses[i].ip_address[n];
        }
        VPLNet_Ntop((VPLNet_addr_t*)&address, buf, 32);
        LOG_INFO("Address %d %s, port is %d", i, buf, cb_data->addresses[i].port);

        if(ext_address.ip_address == NULL) {
            ext_address.ip_address = new char[4];
        }
        memcpy(ext_address.ip_address, cb_data->addresses[i].ip_address, 4);
        ext_address.ip_length = cb_data->addresses[i].ip_length;
        ext_address.port = cb_data->addresses[i].port;
        ext_address.type = cb_data->addresses[i].type;
    }
    VPLSem_Post(&(test_context->sem));
}
static void supply_local(pxd_cb_data_t *cb_data) {
    LOG_INFO("supply_local: result "FMTu64, cb_data->result);
}

static pxd_callback_t pxd_callbacks =
    {
        supply_local,
        supply_external,
        connect_done,
        lookup_done,
        incoming_request,
        incoming_login,
        reject_ccd_creds,
        reject_pxd_creds
    };

// ANS callbacks
static VPL_BOOL connectionActive(ans_client_t *client, VPLNet_addr_t local_addr)
{
    LOG_INFO("connectionActive");
    return true;
}

static VPL_BOOL receiveNotification(ans_client_t *client, ans_unpacked_t *unpacked)
{
    LOG_INFO("receiveNotification");
    return true;
}

static void receiveSleepInfo(ans_client_t *client, ans_unpacked_t *unpacked)
{
    LOG_INFO("receiveSleepInfo");
}

static void receiveDeviceState(ans_client_t *client, ans_unpacked_t *unpacked)
{
    LOG_INFO("receiveDeviceState");
}

static void connectionDown(ans_client_t *client)
{
    LOG_INFO("connectionDown");
}

static void connectionClosed(ans_client_t *client)
{
    LOG_INFO("connectionClosed");
}

static void setPingPacket(ans_client_t *client, char *packet, int length)
{
    LOG_INFO("setPingPacket");
}

static void rejectCredentials(ans_client_t *client)
{
    LOG_INFO("rejectCredentials");
}

static void loginCompleted(ans_client_t *client)
{
    LOG_INFO("loginCompleted");
    // It seems there is no user callback data could be used.
    ans_login_done = true;
}

static void rejectSubscriptions(ans_client_t *client)
{
    LOG_INFO("rejectSubscriptions");
}

static void receiveResponse(ans_client_t *client, ans_unpacked_t *response)
{
    LOG_INFO("receiveResponse");
}
// End of ANS callbacks

static ans_callbacks_t  ans_callbacks =
    {
        connectionActive,
        receiveNotification,
        receiveSleepInfo,
        receiveDeviceState,
        connectionDown,
        connectionClosed,
        setPingPacket,
        rejectCredentials,
        loginCompleted,
        rejectSubscriptions,
        receiveResponse
    };

int main(int argc, char* argv[]) {

    int rc = 0;
    int rv = 0;

    PxdTest::InfraHelper infraHelper(username, password, "pxd_test_client");

    pxd_error_t err;
    pxd_open_t open;
    pxd_id_t id;
    pxd_cred_t cred;
    pxd_id_t target_id;
    pxd_connect_t connect;
    pxd_cred_t ccd_cred;

    // ans variables
    ans_client_t *  ans_client = NULL;
    ans_open_t      ans_input;

    u64 user_id = 0;
    u64 device_id = 0;
    char inst_id[] = "0";
    char region[] = "US";

    string pxdLoginBlob = "";
    string pxdSessionKey = "";

    string ccdLoginBlob = "";
    string ccdSessionKey = "";
    string ansLoginBlob = "";
    string ansSessionKey = "";

    pxd_test_context_t test_context;

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if((rc = VPLSem_Init(&(test_context.sem), 1, 0)) != VPL_OK) {
        LOG_ERROR("Failed to create semaphore.");
        rv = rc;
        goto exit;
    }

    test_context.result = -1;

    // Init
    VPL_Init();

    // connect to infra
    rc = infraHelper.ConnectInfra(user_id, device_id);
    if (rc != 0) {
        LOG_ERROR("ConnectInfra failed: %d", rc);
        rv = rc;
        goto exit;
    }

    // ANS init
    // get ans blob and session key
    infraHelper.GetAnsLoginBlob(ansSessionKey, ansLoginBlob);

    ans_input.clusterName   = ans_name.c_str();
    ans_input.deviceType    = "default";
    ans_input.application   = inst_id;
    ans_input.verbose       = true;
    ans_input.callbacks     = &ans_callbacks;

    ans_input.blob = (void*)ansLoginBlob.data();
    ans_input.blobLength  = static_cast<int>(ansLoginBlob.size());
    ans_input.key  = (void*)ansSessionKey.data();
    ans_input.keyLength   = static_cast<int>(ansSessionKey.size());

    ans_client = ans_open(&ans_input);

    // wait for login done
    LOG_INFO("Wait for ANS login done");
    {
        int retry = 15;
        while (!ans_login_done) {
            VPLThread_Sleep(VPLTime_FromSec(1));
            if (retry-- < 0) {
                goto exit;
            }
        }
    }
    LOG_INFO("ANS login done");

    // get pxd blob and session key
    rc = infraHelper.GetPxdLoginBlob(inst_id, ansLoginBlob, pxdSessionKey, pxdLoginBlob);
    if(rc != 0) {
        LOG_ERROR("getPxdLoginBlob failed: %d", rc);
        rv = rc;
        goto exit;
    }

    hex_dump("pxdSessionKey - ", pxdSessionKey.data(), pxdSessionKey.size());
    hex_dump("pxdLoginBlob - ", pxdLoginBlob.data(), pxdLoginBlob.size());

    // pxd_open
    LOG_INFO("ID is "FMTu64" Device id "FMTu64" Inst id %s",
              user_id, device_id, inst_id);
    // TODO ask JonB to change type
    open.cluster_name = (char*)pxd_name.c_str();
    open.is_incoming = false;
    open.opaque = &test_context;

    open.credentials = &cred;
    open.credentials->id = &id;
    open.credentials->id->device_id = device_id;
    open.credentials->id->user_id = user_id;
    open.credentials->id->instance_id = inst_id;
    open.credentials->id->region = region;
    // TODO ask JonB to change type
    open.credentials->key = (void*)pxdSessionKey.data();
    open.credentials->key_length  = static_cast<int>(pxdSessionKey.size());
    // TODO ask JonB to change type
    open.credentials->opaque = (void*)pxdLoginBlob.data();
    open.credentials->opaque_length = static_cast<int>(pxdLoginBlob.size());

    open.callback = &pxd_callbacks;

    pxd_client = pxd_open(&open, &err);
    if(pxd_client == NULL) {
        LOG_ERROR("pxd_open failed");
        goto exit;
    }

    VPLSem_Wait(&(test_context.sem));

    target_id.region = region;
    target_id.user_id = user_id;
    target_id.device_id = server_device_id;
    target_id.instance_id = inst_id;

    // get ccd blob and session key
    rc = infraHelper.GetCCDLoginBlob(inst_id, user_id, server_device_id, inst_id, ansLoginBlob,
                    ccdSessionKey, ccdLoginBlob);
    if(rc != 0) {
        LOG_ERROR("getCCDLoginBlob failed: %d", rc);
        rv = rc;
        goto exit;
    }

    hex_dump("ccdSessionKey - ", ccdSessionKey.data(), ccdSessionKey.size());
    hex_dump("ccdLoginBlob - ", ccdLoginBlob.data(), ccdLoginBlob.size());

    ccd_cred.id = &id;
    // TODO ask JonB to change type
    ccd_cred.key = (void*)ccdSessionKey.data();
    ccd_cred.key_length  = static_cast<int>(ccdSessionKey.size());
    // TODO ask JonB to change type
    ccd_cred.opaque = (void*)ccdLoginBlob.data();
    ccd_cred.opaque_length = static_cast<int>(ccdLoginBlob.size());

    // Try  EXD and PRX
    LOG_INFO("pxd_connect");

    memset(&connect, 0, sizeof(connect));

    connect.creds = &ccd_cred;
    connect.target = &target_id;
    connect.pxd_dns = (char*)pxd_name.c_str();
    connect.address_count = 1;
    connect.addresses[0].ip_address = ext_address.ip_address;
    connect.addresses[0].ip_length  = ext_address.ip_length;
    connect.addresses[0].port = ext_address.port;
    connect.addresses[0].type = ext_address.type;
    LOG_INFO("do pxd_connect");
    pxd_connect(pxd_client, &connect, &err);
    if(err.error != 0) {
        LOG_ERROR("pxd_connect failed %s", err.message);
        goto exit;
    }
    VPLSem_Wait(&(test_context.sem));
    LOG_INFO("pxd_connect done, connected to server.");
    {
        int yes = 1;
        int rt = VPLSocket_SetSockOpt( test_context.socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, (void*)&yes, sizeof(yes) );
        ASSERT(rt == 0);
    }
    perf_test(test_context.socket, block_size, bs_count);

exit:
    if(rv != 0) {
        LOG_ERROR("Main failed %d", rv);
    } else {
        LOG_INFO("Main exit");
    }

    VPLSem_Destroy(&(test_context.sem));
    return 0;
}

int perf_test(VPLSocket_t socket, u64 bs, u64 count)
{
    VPLTime_t begin, end;
    VPLTime_t it_begin, it_end;
    u64 it_duration = 0;
    u64 total_duration = 0;
    u64 total_size = 0;
    u64 block_bits = bs*8;
    char* buf = (char*)malloc(bs);

    begin = VPLTime_GetTimeStamp();
    for(u64 i = 0; i < count; i++) {
        it_begin = VPLTime_GetTimeStamp();
        VPLSocket_Read(socket, buf, bs, VPL_TIMEOUT_NONE);
        total_size += bs;
        it_end = VPLTime_GetTimeStamp();
        it_duration = VPLTime_ToMillisec(VPLTime_DiffAbs(it_begin, it_end));
        if(it_duration == 0) it_duration = 1;
        total_duration += it_duration;
        LOG_INFO(" Size: "FMTu64" Bytes\t\t bps: "FMTu64"\t\t Avg bps: "FMTu64,
                total_size, (u64)(((double)block_bits/(double)it_duration)*1000.0F),
                (u64)(((double)total_size*8/(double)total_duration)*1000.0F));
    }
    end = VPLTime_GetTimeStamp();

    free(buf);
    return 0;
}

