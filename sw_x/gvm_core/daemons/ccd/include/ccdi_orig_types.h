//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __CCDI_CTYPES_H__
#define __CCDI_CTYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

/// Longest allowed filesystem path for any file in the cache.
/// This count includes the null-terminator.
/// Expand the limit to 1024 for all platforms except winRT (see bug 7095 for more info).
#if !defined(VPL_PLAT_IS_WINRT)
#define CCD_PATH_MAX_LENGTH  1024
#else
#define CCD_PATH_MAX_LENGTH  (259*4)
#endif

/// The cache root needs to be short enough that we have some space left over
/// to actually put our files.
/// This count includes the null-terminator.
#if !defined(VPL_PLAT_IS_WINRT)
#define LOCAL_APP_DATA_MAX_LENGTH  (CCD_PATH_MAX_LENGTH - 128)
#else
#define LOCAL_APP_DATA_MAX_LENGTH  (128*4)
#endif

// This is used by remotefile at stream_service.cpp (client) and strm_http.cpp (server)
#define RF_ERR_MSG_NODIR "parent directory doesn't exist"

#ifdef __cplusplus
}
#endif

#endif // include guard
