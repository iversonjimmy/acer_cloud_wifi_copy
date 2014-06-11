/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#ifndef IS_TS
#define IS_TS
#endif

#include <ts_client.hpp>
#include <ts_server.hpp>
#include <ts_internal.hpp>
#include <getopt.h>
#include "LocalInfo_FixedValues.hpp"

using namespace std;

#define TEST_USER_ID 0x111111111ULL
#define CLIENT_DEVICE_ID 0x222222222ULL
#define SERVER_DEVICE_ID 0x333333333ULL

static u64  client_device_id = CLIENT_DEVICE_ID;
static u64  server_device_id = SERVER_DEVICE_ID;
static int  add_delay   = 0;
static bool ignore_conn = false;

static void usage(int argc, char* argv[])
{
    // Dump original command

    // Print usage message
    LOG_INFO("Usage: %s [options]\n", argv[0]);
    LOG_INFO("Options:");
    LOG_INFO(" -v --verbose               Raise verbosity one level (may repeat 3 times)");
    LOG_INFO(" -t --terse                 Lower verbosity (make terse) one level (may repeat 2 times or cancel a -v flag)");
    LOG_INFO(" -y --delay                 add delay (induce backpressure)");
    LOG_INFO(" -d --device                Device ID for this instance");
    LOG_INFO(" -p --server_port           Port of target device");
    LOG_INFO(" -o --drop_open             Drop the first open request");
    LOG_INFO(" -c --drop_close            Drop the first close request");
    LOG_INFO(" -i --ignore_conn           Have server ignore a connection");
}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"terse", no_argument, 0, 't'},
        {"device", required_argument, 0, 'd'},
        {"server_port", required_argument, 0, 'p'},
        {"delay", required_argument, 0, 'y'},
        {"drop_open", no_argument, 0, 'o'},
        {"drop_close", no_argument, 0, 'c'},
        {"ignore_conn", no_argument, 0, 'i'},
        {0,0,0,0}
    };

    for (;;) {
        int option_index = 0;
        
        int c = getopt_long(argc, argv, "vtd:p:y:oci", 
                            long_options, &option_index);

        if (c == -1) {
            break;
        }
        switch (c) {
        case 'v':
            break;
        case 't':
            break;
        case 'd':
            server_device_id = (u64)atoi(optarg);
            break;
        case 'i':
            ignore_conn = true;
            break;
        case 'y':
            add_delay = atoi(optarg);
            break;

        default:
            usage(argc, argv);
            rv = -1;
            break;
        }
    }

    return rv;
}

static void echo_svc_handler(TSServiceRequest_t& request)
{
    TSError_t err;
    string error_msg;
    char buf[1024];
    int i;

    LOG_INFO("svc_handle: %p client: "FMTu64" device: "FMTu64" ioh: %u",
            request.service_handle, request.client_user_id, 
            request.client_device_id, (u32) request.io_handle);

    for (i = 0; ; i++) {
        size_t buf_len = sizeof(buf);

        err = TS::TSS_Read(request.io_handle, buf, buf_len, error_msg);
        if (err != TS_OK) {
            if (err != TS_ERR_CLOSED) {
                LOG_ERROR("TSS_Read() %d:%s", err, error_msg.c_str());
            }
            break;
        }
        buf[buf_len] = 0;
        // LOG_INFO("TSS_Read(): %s", buf);

        err = TS::TSS_Write(request.io_handle, buf, buf_len, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TSS_Write() %d:%s", err, error_msg.c_str());
            break;
        }

        if (add_delay) {
            VPLThread_Sleep(VPLTime_FromSec(add_delay));
        }
    }

    LOG_INFO("svc_handle: %p client: "FMTu64" device: "FMTu64" ioh: %u - exiting",
            request.service_handle, request.client_user_id, 
            request.client_device_id, (u32) request.io_handle);
}

extern s32 getRouteInfo(u64 userId, u64 deviceId, TsRouteInfo *routeInfo);

s32 getRouteInfo(u64 userId, u64 deviceId, TsRouteInfo *routeInfo)
{
    s32 rv = 0;

    return rv;
}

int main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    TSError_t err;
    string error_msg;
    TSServiceHandle_t svc_handle;
    TSServiceHandle_t svc_handle2;
    VPLNet_port_t serverPort;
    int portNum;
    Ts2::LocalInfo* svc_localInfo = NULL;
    Ts2::LocalInfo* cli_localInfo = NULL;

    LOGInit("ApiTest", NULL);
    LOGSetMax(0); // No limit

    if (parse_args(argc, argv) != 0) {
        goto exit;
    }

    rv = VPL_Init();
    if (rv != VPL_OK) {
        LOG_ERROR("VPL_Init() failed - %d", rv);
        goto exit;
    }

    // Initialize Ts2::LocalInfo for server and client
    {
        Ts2::LocalInfo_FixedValues *_localInfo = new Ts2::LocalInfo_FixedValues(TEST_USER_ID, server_device_id, /*instanceId*/0);
        assert(_localInfo);
        svc_localInfo = _localInfo;
    }
    // Initialize the server library
    err = TS::TS_ServerInit(TEST_USER_ID, server_device_id, /*instanceId*/0, svc_localInfo, error_msg);
    if (err != TS_OK) {
        LOG_ERROR("TS_ServerInit() %d:%s", err, error_msg.c_str());
        goto exit;
    }

    err = TS::TS_GetServerLocalPort(portNum);
    if (err != TS_OK) {
        LOG_ERROR("TS_GetServerLocalPort ret %d", err);
        goto exit;
    }
    serverPort = (VPLNet_port_t)portNum;
    LOG_INFO("Server port is %d", serverPort);

    // Initialize Ts2::LocalInfo object for client
    {
        Ts2::LocalInfo_FixedValues *_localInfo = new Ts2::LocalInfo_FixedValues(TEST_USER_ID, client_device_id, /*instanceId*/0);
        assert(_localInfo);
        _localInfo->SetServerTcpDinAddrPort(TEST_USER_ID, server_device_id, /*instanceId*/0,
                                            VPLNET_ADDR_LOOPBACK, serverPort);
        cli_localInfo = _localInfo;
    }
    // Initialize the client library
    err = TS::TS_Init(TEST_USER_ID, client_device_id, /*instanceId*/0, getRouteInfo, 0, 0, 0, 0, cli_localInfo, error_msg);
    if (err != TS_OK) {
        LOG_ERROR("TS_Init() %d:%s", err, error_msg.c_str());
        goto exit;
    }

    // register echo service handler
    {
        TSServiceParms_t parms;

        parms.service_names.push_back("echo"); 
        parms.service_names.push_back("bark"); 
        parms.service_names.push_back("meow"); 
        parms.protocol_name = "echo"; 
        parms.service_handler = echo_svc_handler;

        err = TS::TS_RegisterService(parms, svc_handle, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_RegisterService() %d:%s", err, error_msg.c_str());
            goto exit;
        }
        LOG_INFO("svc_handle %p", svc_handle);
    }

    // register another service handler
    {
        TSServiceParms_t parms;

        parms.service_names.push_back("blort"); 
        parms.service_names.push_back("barf"); 
        parms.service_names.push_back("blat12345678901234567890"); 
        parms.service_names.push_back("meow"); 
        parms.protocol_name = "http"; 
        parms.service_handler = echo_svc_handler;

        // Duplicate name should fail with EEXIST
        err = TS::TS_RegisterService(parms, svc_handle2, error_msg);
        if (err != TS_ERR_EXISTS) {
            LOG_ERROR("TS_RegisterService() %d:%s expected %d", err, error_msg.c_str(), TS_ERR_EXISTS);
            goto exit;
        }
    }

    // register another service handler
    {
        TSServiceParms_t parms;

        parms.service_names.push_back("blort"); 
        parms.service_names.push_back("barf"); 
        parms.service_names.push_back("blat12345678901234567890"); 
        parms.service_names.push_back("mooooo"); 
        parms.service_names.push_back("hiss"); 
        parms.protocol_name = "http"; 
        parms.service_handler = echo_svc_handler;

        // This should succeed
        err = TS::TS_RegisterService(parms, svc_handle2, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_RegisterService() %d:%s", err, error_msg.c_str());
            goto exit;
        }
        LOG_INFO("svc_handle %p", svc_handle2);
    }

    // Now check that we can find the various services
    {
        TSServiceHandle_t my_handle;
        string svc_name;

        svc_name.assign("barf");
        err = TS_ServiceLookup(svc_name, my_handle, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_ServiceLookup() %s returns %d:%s", svc_name.c_str(), err, error_msg.c_str());
            goto exit;
        }
        if (my_handle != svc_handle2) {
            LOG_ERROR("TS_ServiceLookup() %s returns wrong handle %p:%p", svc_name.c_str(),
                my_handle, svc_handle2);
            goto exit;
        }
        svc_name.assign("blat12345678901234567890"); 
        err = TS_ServiceLookup(svc_name, my_handle, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_ServiceLookup() %s returns %d:%s", svc_name.c_str(), err, error_msg.c_str());
            goto exit;
        }
        if (my_handle != svc_handle2) {
            LOG_ERROR("TS_ServiceLookup() %s returns wrong handle %p:%p", svc_name.c_str(),
                my_handle, svc_handle2);
            goto exit;
        }
        // Check for short match
        svc_name.assign("blat1234567890123456789"); 
        err = TS_ServiceLookup(svc_name, my_handle, error_msg);
        if (err != TS_ERR_NO_SERVICE) {
            LOG_ERROR("TS_ServiceLookup() %s returns %d:%s expected %d",
                svc_name.c_str(), err, error_msg.c_str(), TS_ERR_NO_SERVICE);
            goto exit;
        }
        // Check for long match
        svc_name.assign("barfo"); 
        err = TS_ServiceLookup(svc_name, my_handle, error_msg);
        if (err != TS_ERR_NO_SERVICE) {
            LOG_ERROR("TS_ServiceLookup() %s returns %d:%s expected %d",
                svc_name.c_str(), err, error_msg.c_str(), TS_ERR_NO_SERVICE);
            goto exit;
        }
        // Check for case-sensitivity
        svc_name.assign("Meow"); 
        err = TS_ServiceLookup(svc_name, my_handle, error_msg);
        if (err != TS_ERR_NO_SERVICE) {
            LOG_ERROR("TS_ServiceLookup() %s returns %d:%s expected %d",
                svc_name.c_str(), err, error_msg.c_str(), TS_ERR_NO_SERVICE);
            goto exit;
        }
        // Check for null string
        svc_name.clear();
        err = TS_ServiceLookup(svc_name, my_handle, error_msg);
        if (err != TS_ERR_NO_SERVICE) {
            LOG_ERROR("TS_ServiceLookup() %s returns %d:%s expected %d",
                svc_name.c_str(), err, error_msg.c_str(), TS_ERR_NO_SERVICE);
            goto exit;
        }
    }

    // Try to open a connection
    {
        TSOpenParms_t parms;
        TSIOHandle_t ioh;
        char buf[1024];
        size_t len;

        parms.user_id = TEST_USER_ID;
        parms.device_id = server_device_id;
        parms.instance_id = 0;
        parms.service_name.assign("meow");
        parms.flags = 0;
        parms.timeout = VPLTime_FromSec(10);

        err = TS::TS_Open(parms, ioh, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_Open() %s returns %d:%s", parms.service_name.c_str(), err, error_msg.c_str());
            goto exit;
        }
        len = sizeof(buf);
        memset(buf, 0x42, len);
        err = TS::TS_Write(ioh, buf, len, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_Write() returns %d:%s", err, error_msg.c_str());
            goto exit;
        }
        err = TS::TS_Read(ioh, buf, len, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_Read() returns %d:%s", err, error_msg.c_str());
            goto exit;
        }
        err = TS::TS_Write(ioh, buf, len, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_Write() returns %d:%s", err, error_msg.c_str());
            goto exit;
        }
        err = TS::TS_Close(ioh, error_msg);
        if (err != TS_OK) {
            LOG_ERROR("TS_Close() returns %d:%s", err, error_msg.c_str());
            goto exit;
        }
    }

    err = TS::TS_DeregisterService(svc_handle, error_msg);
    if (err != TS_OK) {
        LOG_ERROR("TS_DeregisterService() %d:%s", err, error_msg.c_str());
        goto exit;
    }

    err = TS::TS_DeregisterService(svc_handle2, error_msg);
    if (err != TS_OK) {
        LOG_ERROR("TS_DeregisterService() %d:%s", err, error_msg.c_str());
        goto exit;
    }

    TS::TS_Shutdown();

    TS::TS_ServerShutdown();

exit:
    return (rv) ? 1 : 0;
}
