/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF 
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

#ifndef __WRAPPER_GLUE_HPP__
#define __WRAPPER_GLUE_HPP__

#include "vpl_types.h"
#include "vssi.h"

namespace vssts_wrapper {
int do_vssi_setup(u64 app_id);
void do_vssi_cleanup(void);
void wakeup_vssi_poll_thread(void);
void do_vssi_open_object2(u64 user_id,
                          u64 dataset_id,
                          u8 mode,
                          VSSI_Object* handle,
                          void* ctx,
                          VSSI_Callback callback);
void do_vssi_delete2(u64 user_id,
                     u64 dataset_id,
                     void* ctx,
                     VSSI_Callback callback);
bool dataset_is_new_vssi(u64 user_id, u64 dataset_id);
}

#endif // include guard
