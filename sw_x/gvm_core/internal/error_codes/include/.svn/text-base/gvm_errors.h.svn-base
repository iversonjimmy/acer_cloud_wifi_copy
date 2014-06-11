//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __GVM_ERRORS_H__
#define __GVM_ERRORS_H__

//============================================================================
/// @file
/// Error codes emitted by the various client subsystems.
/// Please keep this file in-sync with
/// http://www.ctbg.acer.com/wiki/index.php/AcerCloud_Error_Codes
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

#define GVM_OK  0

//-----------------------------------------------------------------------------
// MGRD (obsolete)
//-----------------------------------------------------------------------------
//#define MGRD_OK  0
//
//#define MGRD_ERROR_PARAMETER        -13901
//#define MGRD_ERROR_FOPEN            -13903
//#define MGRD_ERROR_MKDIR            -13904
//#define MGRD_ERROR_MKNOD            -13905
//#define MGRD_ERROR_MODPROBE         -13906
//#define MGRD_ERROR_MOUNT            -13907
//#define MGRD_ERROR_MQ_OPEN          -13908
//#define MGRD_ERROR_NODE_TYPE        -13909
//#define MGRD_ERROR_PARSE            -13910
//#define MGRD_ERROR_SSCANF           -13911
//#define MGRD_ERROR_SETENV           -13912
//#define MGRD_ERROR_SHM_OPEN         -13913
//#define MGRD_ERROR_START_PROCESS    -13914
//#define MGRD_ERROR_SYMLINK          -13915
//#define MGRD_ERROR_UMOUNT           -13916
//#define MGRD_ERROR_XORG_NOT_READY   -13917
//#define MGRD_ERROR_REBOOT           -13918
//#define MGRD_ERROR_SHUTDOWN         -13919
//#define MGRD_ERROR_TITLE_RUNNING    -13920
//#define MGRD_ERROR_NO_TITLE         -13921
//#define MGRD_ERROR_LINK             -13922
//#define MGRD_ERROR_EXECUTE_CHROOT   -13923
//#define MGRD_ERROR_XORG_START       -13924
//#define MGRD_ERROR_DEBUG_LAUNCH     -13925
//#define MGRD_ERROR_SET_LIMIT        -13926
//#define MGRD_ERROR_DELETE_MODULE    -13927
//#define MGRD_ERROR_SOCKET           -13928
//#define MGRD_ERROR_INET_PTON        -13929
//#define MGRD_ERROR_IOCTL            -13930
//#define MGRD_ERROR_UNAME            -13931
//#define MGRD_ERROR_OPEN             -13932
//#define MGRD_ERROR_REALLOC          -13933
//#define MGRD_ERROR_INIT_MODULE      -13934
//#define MGRD_ERROR_READ_KLOG        -13935
//#define MGRD_ERROR_NO_HYPERVISOR    -13936
//#define MGRD_ERROR_FORMAT           -13937
//#define MGRD_ERROR_MOUNT_HANG       -13938
//#define MGRD_ERROR_UMOUNT_HANG      -13939
//#define MGRD_ERROR_INVALID_DAEMON   -13940
//#define MGRD_ERROR_CHOWN            -13941

//-----------------------------------------------------------------------------
// CCDI
//-----------------------------------------------------------------------------
#define CCDI_OK                             0

/// Bad function parameter (programmer error).
#define CCDI_ERROR_PARAMETER                -14001

/// @deprecated No longer generated.
//@{
//#define CCDI_ERROR_LAUNCH_TITLE             -14007
//#define CCDI_ERROR_QUERY_TYPE               -14012
//#define CCDI_ERROR_INVALID_SIZE             -14016
//#define CCDI_ERROR_ADD_CACHE_USER           -14017
//#define CCDI_ERROR_REMOVE_CACHE_USER        -14018
//#define CCDI_ERROR_SHUTDOWN_TITLE           -14019
//@}

/// Unspecified error using CCDI.  See the logs for more information.
#define CCDI_ERROR_FAIL                     -14020

/// @deprecated No longer generated.
//@{
//#define CCDI_ERROR_AGAIN                    -14021
//#define CCDI_ERROR_ADD_EVENT                -14022
//#define CCDI_ERROR_GET_EVENTS               -14023
//#define CCDI_ERROR_NO_CACHED_USERS          -14024
//#define CCDI_ERROR_LOGGED_IN                -14025
//@}

/// The CCDI Client process was unable to allocate memory.
#define CCDI_ERROR_OUT_OF_MEM               -14026

/// @deprecated No longer generated.
//@{
//#define CCDI_ERROR_NOT_GAME                 -14028
//#define CCDI_ERROR_COMMIT_SAVE_DATA         -14029
//@}

/// Response from CCD was not as expected; ensure that the versions of
/// CCDI and CCD are compatible.
#define CCDI_ERROR_BAD_RESPONSE             -14030

/// @deprecated No longer generated.
//@{
//#define CCDI_ERROR_GET_SESSION_DATA         -14031
//#define CCDI_ERROR_REMOVE_USER_TITLE        -14033
//#define CCDI_ERROR_FAKE_USER                -14047
//@}

/// Cannot omit the password because CCD does not have a saved password for the user.
#define CCDI_ERROR_NO_PASSWORD              -14048

/// @deprecated No longer generated.
//@{
//#define CCDI_ERROR_NO_EVENTS                -14049
//#define CCDI_ERROR_ADD_NOTIFICATION         -14050
//#define CCDI_ERROR_ACHIEVE_INDEX            -14051
//#define CCDI_ERROR_SORT_CRITERIA            -14052
//@}

/// Unspecified error at the RPC layer.  See the logs for more information.
#define CCDI_ERROR_RPC_FAILURE              -14090

//-----------------------------------------------------------------------------
// CCD
//-----------------------------------------------------------------------------
#define CCD_OK                      0

/// Bad function parameter (programmer error).
#define CCD_ERROR_PARAMETER         -14101

#define CCD_ERROR_HTTP_STATUS       -14105
#define CCD_ERROR_USER_NOT_FOUND    -14106


/// ::open(), fopen(), or fdopen() failed.
#define CCD_ERROR_FOPEN             -14107

#define CCD_ERROR_MKDIR             -14108
#define CCD_ERROR_OPENDIR           -14109

#define CCD_ERROR_USER_DATA_VERSION -14110
#define CCD_ERROR_PARSE_CONTENT     -14111
#define CCD_ERROR_SIGNATURE_INVALID -14112

#define CCD_ERROR_NOT_IMPLEMENTED   -14113

#define CCD_ERROR_MAIN_DATA_VERSION -14114

/// rename() failed.
#define CCD_ERROR_RENAME            -14115

/// mkstemp() failed.
#define CCD_ERROR_MKSTEMP           -14116

/// Requested player index is already in use, or the requested user is already signed-in locally.
#define CCD_ERROR_SIGNED_IN         -14117

#define CCD_ERROR_NOT_SIGNED_IN     -14118

/// Requested feature is not enabled for offline use; please connect to the infrastructure first.
#define CCD_ERROR_OFFLINE_MODE      -14119

/// Specified player index and user ID do not match.
#define CCD_ERROR_WRONG_USER_ID     -14121

/// @deprecated No longer generated.
//@{
#define CCD_ERROR_TITLE_NOT_FOUND   -14122
//@}

#define CCD_ERROR_NO_CACHED_USERS   -14123
#define CCD_ERROR_ES_FAIL           -14124
#define CCD_ERROR_ECDK_FAIL         -14125
#define CCD_ERROR_NOMEM             -14126
#define CCD_ERROR_CS_FAIL           -14127
#define CCD_ERROR_WRONG_STATE       -14128
#define CCD_ERROR_LOCK_FAILED       -14129
#define CCD_ERROR_SOCKET_RECV       -14130
#define CCD_ERROR_INTERNAL          -14131
#define CCD_ERROR_TITLE_EXISTS      -14132

/// statvfs failed in an unexpected way; see logs for the errno.
#define CCD_ERROR_STATVFS           -14133

/// @deprecated No longer generated.
//@{
#define CCD_ERROR_NO_TICKET         -14134
//@}

#define CCD_ERROR_DISK_SERIALIZE    -14135

/// @deprecated No longer generated.
//@{
#define CCD_ERROR_FCLOSE            -14136
#define CCD_ERROR_CSAN              -14137
#define CCD_ERROR_CSAN_LOST         -14138
#define CCD_ERROR_PARSE_TITLES      -14139
#define CCD_ERROR_SHM               -14140
//@}

/// Component was not initialized.
#define CCD_ERROR_NOT_INIT          -14141

/// Component is already initialized.
#define CCD_ERROR_ALREADY_INIT      -14142

/// Unexpected server response.
#define CCD_ERROR_BAD_SERVER_RESPONSE -14143

/// @deprecated No longer generated.
//@{
#define CCD_ERROR_GSM_ALREADY_SHOWN  -14144
#define CCD_ERROR_REMOTE_NOT_JOINABLE -14145
#define CCD_ERROR_CANNOT_JOIN_LOCAL_USER -14146
//@}

/// Failure configuring connection to VSDS.
#define CCD_ERROR_VSDS_FAIL              -14147

/// The operation requires elevated privileges for the user.
#define CCD_ERROR_NEED_PRIVILEGE         -14148

/// The user's current password is required for the operation, but the password was incorrect or
/// missing.
#define CCD_ERROR_NEED_PASSWORD          -14149

/// The requested new password was rejected by the infrastructure because it didn't meet the
/// server policies.
#define CCD_ERROR_PASSWORD_NOT_ALLOWED   -14150

#define CCD_ERROR_SAVE_LOCATION_NOT_FOUND -14151

#define CCD_ERROR_OUT_OF_BVS_DEVICES     -14152

/// For the target platform/configuration, a required feature was disabled.
#define CCD_ERROR_FEATURE_DISABLED       -14153

/// An bad TMD was encountered.
#define CCD_ERROR_BAD_TMD                -14154

/// Attempting to create file descriptors with pipe() failed.
#define CCD_ERROR_PIPE_FAILED            -14155

/// Tried loading credentials but no such files found
#define CCD_ERROR_CREDENTIALS_MISSING    -14156

/// The requested dataset was not found for the user.
#define CCD_ERROR_DATASET_NOT_FOUND      -14157

/// The sync agent / dataset is not in active state 
#define CCD_ERROR_SYNC_DISABLED          -14158

#define CCD_ERROR_STREAM_SERVICE         -14159

#define CCD_ERROR_DSET_STREAMER          -14160

/// Unspecified error from the ANS device client.
#define CCD_ERROR_ANS_FAILED             -14161

/// The queue no longer exists - it may have been deleted because it became full
/// or CCD may have restarted.  Either way, events have probably been lost.
#define CCD_ERROR_NO_QUEUE               -14162

/// Component is not running.
#define CCD_ERROR_NOT_RUNNING            -14163

/// Component is already in the desired state (initialized, not initialized, started, or stopped).
#define CCD_ERROR_ALREADY                -14164

/// Unspecified error from the IOAC library.
#define CCD_ERROR_IOAC_LIB_FAIL          -14165
/// Deprecated version of #CCD_ERROR_IOAC_LIB_FAIL.
/// @deprecated
#define CCD_ERROR_WAKE_ON_WIFI_FAIL      -14165

/// There is another instance of CCD on the same machine already servicing the same named socket.
#define CCD_ERROR_OTHER_INSTANCE         -14166

/// (Windows-specific) Unspecified error using the Network List Manager (NLM).
#define CCD_ERROR_NLM                    -14167

/// (Windows-specific) Required .dll file was not found.
#define CCD_ERROR_MISSING_DLL            -14168

/// (Windows-specific) The .dll was found, but was missing a required function.
#define CCD_ERROR_WRONG_DLL              -14169

/// The requested operation requires the local device to be ONLINE.
#define CCD_ERROR_LOCAL_DEVICE_OFFLINE   -14170

/// Request was canceled because CCD is shutting down.
#define CCD_ERROR_SHUTTING_DOWN          -14171

/// Improper use of locking; indicates programmer error.
/// Possible causes are attempting to unlock a lock that isn't held by the current thread or
/// attempting to upgrade a read lock to a write lock.
#define CCD_ERROR_LOCK_USAGE             -14172

/// LocalAppData directory is not authorized for the current osUserId.
#define CCD_ERROR_UNAUTHORIZED_DATA_DIR  -14173

/// Generic error for not found
#define CCD_ERROR_NOT_FOUND              -14174

/// Attempted to add more than the maximum number of allowed paths.
#define CCD_ERROR_TOO_MANY_PATHS         -14175

/// The specified path was not found.
#define CCD_ERROR_PATH_NOT_FOUND         -14176

/// The specified app cannot be used
#define CCD_ERROR_APP_ID                 -14177

/// (Programmer error) Calling this function while holding a particular lock can result in deadlock.
#define CCD_ERROR_COULD_DEADLOCK         -14178

/// Generic error code for retry purpose
#define CCD_ERROR_TRANSIENT              -14179

/// There is no appropriate VCS Archive Storage Device.
#define CCD_ERROR_ARCHIVE_DEVICE_NOT_FOUND -14180

/// The corresponding VCS Archive Storage Device is offline.
#define CCD_ERROR_ARCHIVE_DEVICE_OFFLINE -14181

/// The dataset has been suspended by infra.
#define CCD_ERROR_DATASET_SUSPENDED      -14182

/// Cannot change syncbox sync setting while active.  You must disable it first.
#define CCD_ERROR_SYNCBOX_ACTIVE        -14183

/// Bad URL that cannot be handled by syncbox
#define CCD_ERROR_MALFORMED_URL         -14184

/// The local device must first be enabled as a StorageNode before functioning
/// as an Archive Storage Device.
#define CCD_ERROR_NOT_STORAGE_NODE      -14185

//-----------------------------------------------------------------------------
// Downloader (obsolete)
//-----------------------------------------------------------------------------
#define DOWNLOAD_ERR_NO_MEM          -14201
#define DOWNLOAD_ERR_FOPEN           -14202
#define DOWNLOAD_ERR_WRITE_FAILED    -14203
#define DOWNLOAD_ERR_CONTENT_HANDLE_NOT_FOUND  -14204
#define DOWNLOAD_ERR_MUTEX           -14205
#define DOWNLOAD_ERR_INVALID         -14206
#define DOWNLOAD_ERR_INIT            -14207
#define DOWNLOAD_ERR_ALREADY_RUNNING -14208
#define DOWNLOAD_ERR_NOT_RUNNING     -14209
/// An bad TMD was encountered.
#define DOWNLOAD_ERR_BAD_TMD         -14210
/// ContentDetail is missing (should always be retrieved from VSDS).
#define DOWNLOAD_ERR_CONTENT_DETAIL  -14211
/// Not a true error; just indicates that the requested item is not currently being tracked by the downloader.
#define DOWNLOAD_ERR_NOT_ACTIVE      -14212

//-----------------------------------------------------------------------------
// SWU (Software Update)
//-----------------------------------------------------------------------------

#define SWU_ERR_NOT_IMPL                -14320
#define SWU_ERR_FAIL                    -14321
#define SWU_ERR_NOT_INIT                -14322
#define SWU_ERR_ALREADY_INIT            -14323
#define SWU_ERR_NO_DEV_ID               -14324
#define SWU_ERR_NOT_FOUND               -14325
#define SWU_ERR_IN_PROGRESS             -14326
#define SWU_ERR_DOWNLOAD_STOPPED        -14327
#define SWU_ERR_DOWNLOAD_FAILED         -14328
#define SWU_ERR_NO_MEM                  -14329
#define SWU_ERR_DECRYPT_FAIL            -14330
#define SWU_ERR_BAD_TMD                 -14331
#define SWU_ERR_CANCELED                -14332
#define SWU_ERR_CCD_NOT_SET             -14333

//-----------------------------------------------------------------------------
// Remote software update 
//-----------------------------------------------------------------------------
#define REMOTE_SWU_ERR_INVALID_TARGET           -14340
#define REMOTE_SWU_ERR_TARGET_OFFLINE           -14341
#define REMOTE_SWU_ERR_TARGET_UP_TO_DATE        -14342
#define REMOTE_SWU_ERR_SEND_NOTIFICATION_FAIL   -14343

//-----------------------------------------------------------------------------
// NOTE: -14400 to -14499 is for Media Metadata
// See media_metadata_errors.hpp.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// IOAC
// -14500 to -14529
//-----------------------------------------------------------------------------
// Didn't set globalAccessDataPath to ccd.
#define IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH  -14501

// Failed to access disk cache.
#define IOAC_FAILED_TO_ACCESS_DISK_CACHE         -14502

// IOAC function been occupied by other windows user (ccd).
#define IOAC_FUNCTION_ALREADY_IN_USE             -14503

// There is no IOAC capable adapter.
#define IOAC_NO_HARDWARE                         -14504

//-----------------------------------------------------------------------------
// Remote File Search
// -14530 to -14549
//-----------------------------------------------------------------------------
#define RF_SEARCH_ERR_NOT_IMPLEMENTED              -14530  // Initially, there will be no implementation for the orbe.
#define RF_SEARCH_ERR_INVALID_REQUEST              -14531
#define RF_SEARCH_ERR_DATASETID_DOES_NOT_EXIST     -14532  // The datasetId provided does not exist.
#define RF_SEARCH_ERR_PATH_DOES_NOT_EXIST          -14533  // The <path> in the request does not exist in target device.
#define RF_SEARCH_ERR_FOLDER_ACCESS_DENIED         -14534  // Access denied by remote file module.
#define RF_SEARCH_ERR_FOLDER_NO_PERMISSION         -14535  // Access to <path> denied by the target device OS.
#define RF_SEARCH_ERR_MAX_ONGOING_SEARCHES         -14536  // Too many searches ongoing to satisfy this request. Wait and try again.
#define RF_SEARCH_ERR_SEARCH_QUEUE_ID_INVALID      -14537  // The searchQueueId provided is not recognized.
#define RF_SEARCH_ERR_START_INDEX_JUMPED_BACKWARDS -14538  // The startIndex provided was less than a startIndex previously received. (likely App bug).
#define RF_SEARCH_ERR_START_INDEX_JUMPED_FORWARDS  -14539  // The startIndex provided skips over entries never returned. (likely App bug).
#define RF_SEARCH_ERR_INTERNAL                     -14540

//-----------------------------------------------------------------------------
// -14550 to -14599 should be free, but check the Wiki:
// http://www.ctbg.acer.com/wiki/index.php/AcerCloud_Error_Codes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Tunnel Services
// -14600 to -14624
//-----------------------------------------------------------------------------
#define TS_ERR_NO_DEVICE        -14600
#define TS_ERR_NO_MEM           -14601
#define TS_ERR_BAD_TICKET       -14602
#define TS_ERR_COMM             -14603
#define TS_ERR_NO_ROUTE         -14604
#define TS_ERR_NO_PORT          -14605
#define TS_ERR_EXISTS           -14606
#define TS_ERR_INVALID          -14607
#define TS_ERR_NO_AUTH          -14608
#define TS_ERR_NO_SERVICE       -14609
#define TS_ERR_NO_USER          -14610
#define TS_ERR_NO_CONNECTION    -14611
#define TS_ERR_CLOSED           -14612
#define TS_ERR_NOT_INIT         -14613
#define TS_ERR_BAD_HANDLE       -14614
#define TS_ERR_BAD_SIGN         -14615
#define TS_ERR_EXT_TYPE         -14616
#define TS_ERR_CREATE_THRD      -14617
// New error codes starting TS2.
#define TS_ERR_WRONG_STATE      -14618
#define TS_ERR_INTERNAL         -14619
#define TS_ERR_TIMEOUT          -14620
#define TS_ERR_BAD_CRED         -14621
#define TS_ERR_INVALID_DEVICE   -14622

//-----------------------------------------------------------------------------
// -14625 to -14799 should be free, but check the Wiki:
// http://www.ctbg.acer.com/wiki/index.php/AcerCloud_Error_Codes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// SYNC_AGENT_ERR_
//-----------------------------------------------------------------------------
#define SYNC_AGENT_ERR_FAIL      -14801

/// The operation was canceled due to receiving a "stop" request.
#define SYNC_AGENT_ERR_STOPPING          -14802
#define SYNC_AGENT_ERR_NOT_IMPL          -14803
#define SYNC_AGENT_ERR_RESUME_CALLED_DURING_WAIT_FOR_PAUSE  -14804
#define SYNC_AGENT_STATUS_PAUSING        -14805

/// Invalid function argument.  (Programmer error.)
#define SYNC_AGENT_ERR_INVALID_ARG       -14808

#define SYNC_AGENT_ERR_NO_MEM            -14809

#define SYNC_AGENT_ERR_NEED_UPLOAD_SCAN  -14810

//-----------------------------------------------------------------------------

#define SYNC_CONFIG_ERR_NOT_FOUND               -14850
#define SYNC_CONFIG_ERR_ALREADY                 -14851
#define SYNC_CONFIG_STATUS_NO_AVAIL_THREAD      -14852
#define SYNC_CONFIG_ERR_SHUTDOWN                -14853
#define SYNC_CONFIG_ERR_BUSY_TASK               -14854
#define SYNC_CONFIG_ERR_REVISION_NOT_READY      -14855
#define SYNC_CONFIG_ERR_REVISION_IS_OBSOLETE    -14856

//-----------------------------------------------------------------------------
// SYNC_AGENT_DB_ERR_
//-----------------------------------------------------------------------------
#define SYNC_AGENT_DB_ERR_FAIL                  -14881
#define SYNC_AGENT_DB_ERR_ROW_NOT_FOUND         -14882
#define SYNC_AGENT_DB_CANNOT_OPEN               -14883
#define SYNC_AGENT_DB_FUTURE_SCHEMA             -14884
#define SYNC_AGENT_DB_UNSUPPORTED_PREV_SCHEMA   -14885
#define SYNC_AGENT_DB_ERR_UNEXPECTED_DATA       -14886
#define SYNC_AGENT_DB_ERR_INTERNAL              -14887
#define SYNC_AGENT_DB_NOT_EXIST_TO_OPEN         -14888  // db doesn't exist but not allow to open-create db

//-----------------------------------------------------------------------------
// VCS_CACHE
//-----------------------------------------------------------------------------
#define VCS_CACHE_CANCELLED                     -14895
#define VCS_CACHE_NOT_FOUND                     -14896

//-----------------------------------------------------------------------------
// Utils
//-----------------------------------------------------------------------------
#define UTIL_ERR_NO_MEM  -14901
#define UTIL_ERR_MKDIR   -14902
#define UTIL_ERR_FOPEN   -14903
#define UTIL_ERR_FREAD   -14904
#define UTIL_ERR_STAT    -14905

/// Bad function parameter (programmer error).
#define UTIL_ERR_INVALID -14906

/// The requested file or buffer was too large.
#define UTIL_ERR_TOO_BIG -14907

#define UTIL_ERR_FWRITE  -14908

#define UTIL_NOT_INIT    -14910

#define DB_UTIL_ERR_INTERNAL                    -14920
#define DB_UTIL_ERR_ROW_NOT_FOUND               -14921

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // include guard
