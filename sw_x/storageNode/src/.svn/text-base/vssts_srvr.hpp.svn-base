/*
 *  Copyright 2014 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

/// Tunnel Service Wrapper Class
/// This takes over the existing strm_http class.


#ifndef __VSSTS_SRVR_HPP__
#define __VSSTS_SRVR_HPP__


#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"

#define IS_TS
#include "ts_client.hpp"
#include "ts_server.hpp"
#include "gvm_errors.h"

#include "vss_server.hpp"
#include "vss_session.hpp"

using namespace std;


TSError_t vssts_srvr_start(vss_server* old_vss_server,
                           vss_session* session,
                           string& error_msg);

void vssts_srvr_stop(void);

void vssts_srvr_put_response(u32 vssts_id, char* response);


#endif // include guard
