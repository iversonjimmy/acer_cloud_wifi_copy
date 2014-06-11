/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

#ifndef __TS_TEST_HPP__
#define __TS_TEST_HPP__

#include "vplu_types.h"
#include <string>

extern const char* TSTEST_STR;

int tstest_commands(int argc, const char* argv[]);

struct TSTestOpenParams {
    u64          user_id;
    u64          device_id;
    std::string  service_name;
    std::string  credentials;
    u64          flags;
    u64          timeout; // usec

    TSTestOpenParams () :
        user_id(0), device_id(0), flags(0), timeout(0) {}
};


struct TSTestParameters {
    TSTestOpenParams tsOpenParms;
    s32 testId;
    s32 logEnableLevel;
    u32 xfer_cnt;
    u32 xfer_size;
    u32 nTestIterations;
    u32 nClients;
    s32 client_write_delay;
    s32 server_read_delay;

    TSTestParameters (s32 logLevel) :
        testId(0),
        logEnableLevel(logLevel),
        xfer_cnt(1),
        xfer_size(1024),
        nTestIterations(1),
        nClients(1),
        client_write_delay(0),
        server_read_delay(0)    {}
};

struct TSTestResult {
    s32          return_value;
    std::string  error_msg;

    TSTestResult () : return_value(0) {}
};

int runTsTest(const TSTestParameters& test, TSTestResult& result);

bool isInitDone();

#endif // include guard
