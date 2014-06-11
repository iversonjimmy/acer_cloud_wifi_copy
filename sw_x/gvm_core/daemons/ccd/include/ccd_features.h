//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __CCD_FEATURES_H__
#define __CCD_FEATURES_H__

//============================================================================
/// @file
/// This header consolidates compile-time configuration choices for CCD.
//============================================================================

#include <vplex_plat.h>
#include <gvm_configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------
// Compile-time daemon configuration settings
// TODO: These may belong in the makefile defs.


// CCD_USE_IPC_SOCKET:
// - If true, CCD runs a VPLNamedSocket server to accept ProtoRPC CCDI requests.
// .
// CCD_USE_PROTORPC:
// - If true, CCDI requests are received in serialized ProtoRPC format and are dispatched via the
//   ProtoRPC layer (see #CCDIService).
// - If false, apps link with the CCD implementation directly and do not use the ProtoRPC layer.
// .
#if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
  // On iOS and WinRT, IPC is prohibited, so each app is linked to its own copy of CCD.
# define CCD_USE_IPC_SOCKET  0
# define CCD_USE_PROTORPC  0
#elif defined(ANDROID)
  // On Android, we use AIDL (instead of VPLNamedSocket) for IPC.
# define CCD_USE_IPC_SOCKET  0
# define CCD_USE_PROTORPC  1
#else
# define CCD_USE_IPC_SOCKET  1
# define CCD_USE_PROTORPC  1
#endif

// This is an artifact from the original version of CCD.
// Much of the current codebase assumes only 1 logged-in user per process.
#define CCD_MAX_USERS  1

#if defined(ANDROID) || defined(IOS) || defined(VPL_PLAT_IS_WINRT)
#  define CCD_ENABLE_STORAGE_NODE  0
#  define CCD_ENABLE_MEDIA_SERVER_AGENT  0
#else
#  define CCD_ENABLE_STORAGE_NODE  1
#  define CCD_ENABLE_MEDIA_SERVER_AGENT  1
#endif

#if defined(CLOUDNODE)
#define CCD_ENABLE_DOC_SAVE_N_GO 0
#else
#define CCD_ENABLE_DOC_SAVE_N_GO 1
#endif

#if defined(ANDROID) || defined(IOS) || defined(VPL_PLAT_IS_WINRT) || defined(CLOUDNODE)
#define CCD_ENABLE_SYNCDOWN_CLOUDDOC 0
#else
#define CCD_ENABLE_SYNCDOWN_CLOUDDOC 1
#endif

#define CCD_ENABLE_SYNCDOWN_PICSTREAM 1

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC || CCD_ENABLE_SYNCDOWN_PICSTREAM
#define CCD_ENABLE_SYNCDOWN 1
#else
#define CCD_ENABLE_SYNCDOWN 0
#endif

#if defined(CLOUDNODE)
#define CCD_ENABLE_SYNCUP_PICSTREAM 0
#else
#define CCD_ENABLE_SYNCUP_PICSTREAM 1
#endif

#if CCD_ENABLE_SYNCUP_PICSTREAM
#define CCD_ENABLE_SYNCUP 1
#else
#define CCD_ENABLE_SYNCUP 0
#endif

#if defined(CLOUDNODE)
#  define CCD_ENABLE_MEDIA_METADATA_DOWNLOAD  0
#else
#  define CCD_ENABLE_MEDIA_METADATA_DOWNLOAD  1
#endif

#define CCD_ENABLE_MEDIA_CLIENT CCD_ENABLE_MEDIA_METADATA_DOWNLOAD

#if defined(VPL_PLAT_IS_WIN_DESKTOP_MODE) || defined(CLOUDNODE)
#  define CCD_SYNC_CONFIGS_ALWAYS_ACTIVE  1
#else
#  define CCD_SYNC_CONFIGS_ALWAYS_ACTIVE  0
#endif

#if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
#  define CCD_ALLOW_BACKGROUND_SYNC  0
#  define CCD_USE_SHARED_CREDENTIALS 1
#else
#  define CCD_ALLOW_BACKGROUND_SYNC  1
#  define CCD_USE_SHARED_CREDENTIALS 0
#endif

#if defined(ANDROID) || defined(IOS) || defined(VPL_PLAT_IS_WINRT)
// For 2.6, leave this feature deactivated, which means that by default
// photo thumbnails will continue to be synced on all devices
#define CCD_MOBILE_DEVICE_CONSERVE_STORAGE 0
#else
#define CCD_MOBILE_DEVICE_CONSERVE_STORAGE 0
#endif

#define CCD_USE_MEDIA_STREAM_SERVICE  1
#define CCD_USE_ANS  1

#define CCD_ENABLE_USER_SUMMARIES 1

#if defined(VPL_PLAT_IS_WIN_DESKTOP_MODE)
#  define CCD_ENABLE_IOAC  1
#  define CCD_IS_WINDOWS_SERVICE  0
#else
#  define CCD_ENABLE_IOAC  0
#endif

// NOTE: Many places in the code assume that CCD_PROTOCOL_VERSION can be parsed as a decimal integer!
#define CCD_PROTOCOL_VERSION   "5"

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
#define ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
#elif defined(IOS)
#define ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
#elif defined(ANDROID)
#define ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
#endif

#define CCD_ENABLE_IOT_SDK_HTTP_API         0

//------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"
#endif

#endif // include guard
