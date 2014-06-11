/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef __VSSI_COMMON_HPP__
#define __VSSI_COMMON_HPP__

#include "vpl_th.h"
#include "vssi_types.h"
#if defined(WIN32)
#include "pipe_win.hpp"
#else
#include "vplex_vs_directory.h"
#endif

#define VSSI_THREAD_STACK_SIZE 32768
VPLThread_return_t vssi_select_thread(VPLThread_arg_t arg);
void wakeup_vssi_select_thread(void);
int vssi_select_thread_init(void);

/// Pre-test setup with no specific user session.
int do_vssi_setup(u64 deviceId);

/// Post-test cleanup, no user session to clean-up.
void do_vssi_cleanup(void);

#endif
