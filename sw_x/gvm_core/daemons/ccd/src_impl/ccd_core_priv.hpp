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

#ifndef __CCD_CORE_PRIV_HPP__
#define __CCD_CORE_PRIV_HPP__

#include "vpl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Marks the beginning of a login / local user activation.
/// Blocks until it is safe to perform the operation.
/// Prevents any other login, logout or shutdown from occurring until #CCDPrivEndLogin is called.
/// @note If this succeeds, you must call #CCDPrivEndLogin() later.
/// @return CCD_ERROR_SHUTTING_DOWN If CCD is shutting down and the login should be canceled.
int CCDPrivBeginLogin();
void CCDPrivEndLogin();

/// Marks the beginning of a logout.
/// Blocks until it is safe to perform logout.
/// Prevents any other login, logout or shutdown from occurring until #CCDPrivEndLogout is called.
/// @note If this succeeds, you must call #CCDPrivEndLogout() later.
/// @return CCD_ERROR_SHUTTING_DOWN If CCD is shutting down and the logout should be canceled.
/// @return CCD_ERROR_ALREADY If CCD is logging out user and returnIfLogoutInProgress is 'true'.
int CCDPrivBeginLogout(bool returnIfLogoutInProgress);
void CCDPrivEndLogout();

/// Marks the beginning of functionality that starts or stops modules.
/// Blocks until it is safe to start/stop modules.
/// Prevents any other login, logout or shutdown from occurring until #CCDPrivEndOther is called.
/// @note If this succeeds, you must call #CCDPrivEndOther() later.
/// @return CCD_ERROR_SHUTTING_DOWN If CCD is shutting down and the start/stop logic should be skipped.
int CCDPrivBeginOther();
void CCDPrivEndOther();

#ifdef __cplusplus
}
#endif

#endif // include guard
