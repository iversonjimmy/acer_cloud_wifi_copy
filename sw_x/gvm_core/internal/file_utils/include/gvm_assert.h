//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef _GVM_ASSERT_H_
#define _GVM_ASSERT_H_

#include "vplex_assert.h"
#include "log.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define GVM_ASSERT_FAILED(...) \
    BEGIN_MULTI_STATEMENT_MACRO \
    LOG_CRITICAL(__VA_ARGS__); \
    FAILED_ASSERT(__VA_ARGS__); \
    END_MULTI_STATEMENT_MACRO

#ifdef  __cplusplus
}
#endif

#endif // include guard
