//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __BASE_H__
#define __BASE_H__

#include "ccd_features.h"

#include "vpl_th.h"
#include "vplex_ipc_socket.h"
#include "vplex_named_socket.h"

#include "ccdi_orig_types.h"

#include "conf.h"
#include "escore.h"
#include "gvm_assert.h"
#include "gvm_errors.h"
#include "gvm_file_utils.hpp"
#include "gvm_misc_utils.h"
#include "gvm_thread_utils.h"
#include "log.h"
#include "scopeguard.hpp"
#include "ucf.h"

#include <time.h>
#include <ctype.h>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WIN32)
#if !defined(IOS)
#include <sys/reboot.h>
#endif
#include <signal.h>
#include <semaphore.h>
#endif

#define FMT_DeviceId  FMTu64

/// mode_t value for open(), required when O_CREAT flag is specified.
#define CCD_NEW_FILE_MODE  (VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR)

#define HOSTNAME_MAX_LENGTH         256

#define USERGROUP_MAX_LENGTH        256

#define MAX_CONNECTIONS             64

#define MAIN_LOOP_THREAD_STACK_SIZE       UTIL_DEFAULT_THREAD_STACK_SIZE

#define TAGEDIT_PATH_MAX_LENGTH     (CONF_MAX_STR_LENGTH+1)

#endif // include guard
