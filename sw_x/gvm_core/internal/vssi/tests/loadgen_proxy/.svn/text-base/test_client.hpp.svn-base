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

#ifndef __TEST_CLIENT_HPP__
#define __TEST_CLIENT_HPP__

/// @file
/// Test client class

class test_client;

#include "vplu_types.h"
#include "vplu_common.h"
#include "vpl_socket.h"
#include "vpl_net.h"
#include "vpl_th.h"
#include "vpl_time.h"

#include <string>

class test_client
{
public:
    test_client(const char* header, const char* request);
    ~test_client();

    // Start client. Use when client first connects.
    int start();

    // Used by start() to launch activity thread.
    void launch();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(test_client);

    // Communication socket for the client
    VPLSocket_t socket;

    u8 version;
    u32 xid;
    u64 device_id;
    u64 handle;
    u32 proxy_cookie;
    u32 client_ip;
    u16 client_port;
    u16 server_port;
    std::string server_addr;
    u8 proxy_type;
    
    int fail_line;
    int fail_code;
};

// Activity task thread routine.
VPLTHREAD_FN_DECL test_client_main(VPLThread_arg_t vpclient);

#endif // include guard
