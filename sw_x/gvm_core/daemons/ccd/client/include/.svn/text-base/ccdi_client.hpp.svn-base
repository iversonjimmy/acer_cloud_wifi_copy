//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCDI_CLIENT_HPP__
#define __CCDI_CLIENT_HPP__

//============================================================================
/// @file
/// Extensions to ccdi.hpp for using the API as an IPC client.
//============================================================================

namespace ccdi {
namespace client {

/// Override the CCD testInstanceNum to connect to (for testing & debugging purposes).
/// @note No lock is used, so make sure that you don't use CCDI concurrently with calling this!
void CCDIClient_SetTestInstanceNum(int testInstanceNum);

/// Override the CCD osUserId to connect to.
/// This is currently only for the CCD Monitor Windows Service (since it is responsible for managing
/// separate instances of CCD for each Windows user).
/// @note No lock is used, so make sure that you don't use CCDI concurrently with calling this!
void CCDIClient_SetOsUserId(const char* osUserId);
 
} // namespace client
} // namespace ccdi

#endif // include guard
