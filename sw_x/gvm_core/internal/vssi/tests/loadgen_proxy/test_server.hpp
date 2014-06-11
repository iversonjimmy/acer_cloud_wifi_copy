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

#ifndef __TEST_SERVER_HPP__
#define __TEST_SERVER_HPP__

/// @file
/// Test server class

class test_server;

#include "vplu_types.h"
#include "vplu_common.h"

#include "vplex_vs_directory.h"

#include <string>

#include "ans_device.h"

class test_server
{
public:
    test_server();
    ~test_server();

    // Start server. Use when server first connects.
    int start();

    // Stop server.
    void stop();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(test_server);

    ans_client_t* ans_client;
    ans_open_t ans_input;
    std::string ans_name;
    std::string ansLoginBlob; 
    std::string ansSessionKey;
};

#endif // include guard
