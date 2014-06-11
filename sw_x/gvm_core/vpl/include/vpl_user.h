//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_USER_H__
#define __VPL_USER_H__

//============================================================================
/// @file
/// Please see @ref VPLUser.
//============================================================================

#include "vpl_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLUser VPL User API
///@{

///
/// Globally unique identifier for a user.
///
typedef uint64_t VPLUser_Id_t;

/// printf format macro for #VPLUser_Id_t.
#define FMT_VPLUser_Id_t "%"PRIu64

/// Value of #VPLUser_Id_t reserved to indicate "no user".
//% The protobuf files don't have access to this definition, so they use [default = 0] instead.
#define VPLUSER_ID_NONE  0

///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
