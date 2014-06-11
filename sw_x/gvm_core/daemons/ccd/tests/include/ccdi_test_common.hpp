//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __CCDI_TEST_COMMON_HPP__
#define __CCDI_TEST_COMMON_HPP__

#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <log.h>
#include <vpl_time.h>
#include <vplu_format.h>
#include <vpl_th.h>
#include <vpl_socket.h>
#include <vplex_error.h>

#define CCD_TEST_INSTANCE_ID  "CCD_TEST_INSTANCE_ID"

/// Reads environment variable CCD_TEST_INSTANCE_ID and sets the CCDI client
/// to connect to that instance of CCD.
int CCDITest_SetupCcdiTest();

#endif // include guard
