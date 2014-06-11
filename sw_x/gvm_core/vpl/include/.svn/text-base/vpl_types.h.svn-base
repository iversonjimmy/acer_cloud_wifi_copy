//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_TYPES_H__
#define __VPL_TYPES_H__

//============================================================================
/// @file
/// Virtual Platform Layer definition of basic integer types.
///
/// Unless \c VPL_NO_C99_TYPES is defined, including this
/// file should result in similar behavior as if
/// <stdint.h>, and <inttypes.h> had been included.
///
/// @note If compiling as C++, make sure to include this <b>before</b> any
///     standard C99 headers, otherwise the STDC limit macros will not be defined and
///     compilation errors can result.
//% iGware developers must always make sure that vpl_types.h gets included before any standard
//% headers.  The following alternative is only intended for outside developers:
///     Alternatively, you can define preprocessor macros as part of your command-line:
///     - __STDC_LIMIT_MACROS=1
///     - __STDC_CONSTANT_MACROS=1
///     - __STDC_FORMAT_MACROS=1
///     .
///
//============================================================================

// Platform specific definitions.
#include "vpl__types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Represents a boolean value.
typedef int VPL_BOOL;
//% TODO: typedef int VPL_Bool_t;

/// Represents "true", for use with #VPL_BOOL.
#define VPL_TRUE  1

/// Represents "false", for use with #VPL_BOOL.
#define VPL_FALSE  0

/// Null pointer.
#ifdef __cplusplus
#   define VPL_NULL  0
#else
#   define VPL_NULL  ((void*)0)
#endif

//----------------------------------------------------------------------------
// printf and scanf format macros for common types that are missing them.
// These can be used just like the definitions from C99's inttypes.h.

#define VPL_PRIu_off_t  VPL_PRI_PREFIX_off_t"u"
#define VPL_PRIx_off_t  VPL_PRI_PREFIX_off_t"x"
#define VPL_SCNu_off_t  VPL_SCN_PREFIX_off_t"u"
#define VPL_SCNx_off_t  VPL_SCN_PREFIX_off_t"x"

#define VPL_PRIu_size_t  VPL_PRI_PREFIX_size_t"u"
#define VPL_PRIx_size_t  VPL_PRI_PREFIX_size_t"x"
#define VPL_SCNu_size_t  VPL_SCN_PREFIX_size_t"u"
#define VPL_SCNx_size_t  VPL_SCN_PREFIX_size_t"x"

#define VPL_PRId_ssize_t  VPL_PRI_PREFIX_ssize_t"d"
#define VPL_PRIx_ssize_t  VPL_PRI_PREFIX_ssize_t"x"
#define VPL_SCNd_ssize_t  VPL_SCN_PREFIX_ssize_t"d"
#define VPL_SCNx_ssize_t  VPL_SCN_PREFIX_ssize_t"x"

//----------------------------------------------------------------------------

#ifdef  __cplusplus
}
#endif

#endif // include guard
