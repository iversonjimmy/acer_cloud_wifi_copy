//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL__DYNAMIC_H__
#define __VPL__DYNAMIC_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int status;
    void* handle; //% handle for underlying functions
} VPLDL__LibHandle;

#ifdef __cplusplus
}
#endif

#endif // include guard
