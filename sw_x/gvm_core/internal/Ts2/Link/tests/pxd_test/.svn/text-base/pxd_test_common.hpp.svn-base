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

#ifndef PXD_TEST_COMMON_HPP_
#define PXD_TEST_COMMON_HPP_

#include <stdio.h>
#include <cstring>
#include <iostream>
#include <string>

#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vplex_file.h"
#include "vpl_conv.h"
#include "vpl_th.h"
#include "vplex_socket.h"

#include "pxd_client.h"
#include "../PxdTestInfraHelper/PxdTestInfraHelper.hpp"
#include "p2p.hpp"

/// These are Linux specific non-abstracted headers.
/// TODO: provide similar functionality in a platform-abstracted way.
#include <setjmp.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <fcntl.h>

#define LOG_INFO(...) VPLTRACE_LOG_INFO(TRACE_APP, 0, ##__VA_ARGS__);
#define LOG_ERROR(...) VPLTRACE_LOG_ERR(TRACE_APP, 0, ##__VA_ARGS__);

enum ANS_NotificationType_t {
  ANS_TYPE_SYNC_AGENT = 2,
  ANS_TYPE_PROXY = 3,
  ANS_TYPE_CLIENT_CONN = 4,
  ANS_TYPE_COMMUNITY = 5,
  ANS_TYPE_SERVER_WAKEUP = 6,
  ANS_TYPE_SESSION_EXPIRED = 11,
  ANS_TYPE_USER_DEVICE_MESSAGE = 12,
  ANS_TYPE_KEEPALIVE = 254
};

using namespace std;

void
hex_dump(const char *title, const char *contents, int length);


inline u64 stringTou64(const char * str)
{
    u64 res = 0;
    while (*str != '\0')
    {
        res *= 10 ;
        res += *str - '0';
        str++;
    }
    return res;
}


#endif /* PXD_TEST_COMMON_HPP_ */
