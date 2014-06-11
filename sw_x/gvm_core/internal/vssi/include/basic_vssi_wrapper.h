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

#ifndef BASIC_VSSI_WRAPPER_H_07_16_2013
#define BASIC_VSSI_WRAPPER_H_07_16_2013

#include "vplu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Helper wrapper to service sockets used by the VSSI API.

/// Initializes VSSI.  Starts a thread that services the VSSI API sockets.
/// There is no need to call VSSI_Init() when using this wrapper.
int vssi_wrap_start(u64 device_id);

/// Shuts down VSSI.  Stops the thread that services the VSSI API sockets.
/// There is no need to call VSSI_Cleanup() when using this wrapper.
void vssi_wrap_stop(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BASIC_VSSI_WRAPPER_H_07_16_2013
