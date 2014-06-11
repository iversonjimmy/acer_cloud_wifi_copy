//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLU_VERSION_H__
#define __VPLU_VERSION_H__

//============================================================================
/// @file
/// VPL utility (VPLU) macros for defining the build version.
//============================================================================

#include "vplu_common.h"

/// Constructs the module version string for the specified module.
#define MODULE_VERSION_STRING(name)  name##_MODULE_NAME" " \
    VPL_STRING(name##_MAJOR)"."VPL_STRING(name##_MINOR)"."VPL_STRING(name##_PATCH)" " \
    __DATE__" "__TIME__

#ifdef BUILD_HOST_NAME
#   define BUILD_HOST_NAME_STR  VPL_STRING(BUILD_HOST_NAME)
#else 
#   define BUILD_HOST_NAME_STR  "?"
#endif

#ifdef _DEBUG
#   ifdef NDEBUG
#       error "Conflicting definitions for NDEBUG and _DEBUG"
#   else
#       define VPLU_BUILD_TYPE_STRING  "DEBUG"
#   endif
#else
#   ifdef NDEBUG
#       define VPLU_BUILD_TYPE_STRING  "NON-DEBUG"
#   else
#       define VPLU_BUILD_TYPE_STRING  "[type unspecified]"
#   endif
#endif

#define VPLU_BUILD_TIME_STRING  __DATE__" "__TIME__

#endif // include guard
