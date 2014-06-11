/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */
#ifndef __VPLEX_USER_H__
#define __VPLEX_USER_H__

/// @file
/// Types related to user sessions that must remain private to VPLex.

#include "vpl_types.h"

/// Maximum password length allowed by our infrastructure.
#define VPLUSER_MAX_PASSWORD_LEN 32

/// Login session handle: A 64-bit value unique over current open sessions.
typedef uint64_t VPLUser_SessionHandle_t;
#define SCN_VPLUser_SessionHandle_t SCNu64

#define VPL_USER_SESSION_SECRET_LENGTH 20
/// @note This buffer is not null-terminated.
typedef uint8_t VPLUser_SessionSecret_t[VPL_USER_SESSION_SECRET_LENGTH];

#define VPL_USER_SERVICE_TICKET_LENGTH 20
/// Credentials for accessing infrastructure services; 160-bit for a given session.
/// @note This buffer is not null-terminated.
typedef char VPLUser_ServiceTicket_t[VPL_USER_SERVICE_TICKET_LENGTH];

#endif // include guard
