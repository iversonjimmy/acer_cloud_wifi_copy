//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef VPL_ASSERT_H__
#define VPL_ASSERT_H__

//============================================================================
/// @file
/// Basic assertions for use by the VPL implementations.
//============================================================================

#include "vpl_plat.h"
#include "vplu_debug.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#   define VPLIMPL_FAILED_ASSERT(...)  ((void)0)
#   define VPLIMPL_ASSERT(exp)  ((void)0)
#else
#   define VPLIMPL_FAILED_ASSERT(...) \
        BEGIN_MULTI_STATEMENT_MACRO \
            VPL_REPORT_FATAL(__VA_ARGS__); \
            assert(0); \
        END_MULTI_STATEMENT_MACRO

#   define VPLIMPL_ASSERT(exp) \
        BEGIN_MULTI_STATEMENT_MACRO \
            if(!(exp)) { \
                VPLIMPL_FAILED_ASSERT("%s", #exp); \
            } \
        END_MULTI_STATEMENT_MACRO
#endif

#ifdef __cplusplus
}
#endif

#endif // include guard
