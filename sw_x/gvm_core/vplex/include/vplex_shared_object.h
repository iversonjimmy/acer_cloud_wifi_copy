//
//  vplex_shared_object.h
//  vplex
//
//  Created by Jimmy on 12/9/10.
//  Copyright (c) 2012 acer Inc. All rights reserved.
//

#ifndef __VPLEX_SHARED_OBJECT_H__
#define __VPLEX_SHARED_OBJECT_H__

//============================================================================
/// @file
//============================================================================

#include "vplex_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VPL_SHARED_CCD_CONF_ID "ccdconf"

/// Old shared credentials names.
/// These should only be used when migrating from pre-2.6.0 CCD.
/// For 2.6.0 and beyond, see vplex_shared_credential.hpp.
//@{
#define VPL_SHARED_USER_NAME_ID "username"
#define VPL_SHARED_USER_IASOUTPUT_ID "user_iasoutput"
#define VPL_SHARED_ANS_SESSION_KEY_ID "ans_session_key"
#define VPL_SHARED_ANS_LOGIN_BLOB_ID "ans_login_blob"

#define VPL_SHARED_DEVICE_ID "device_id"
#define VPL_SHARED_DEVICE_CREDSCLEAR_ID "device_creds_clear"
#define VPL_SHARED_DEVICE_CREDSECRET_ID "device_creds_secret"
#define VPL_SHARED_DEVICE_RENEWAL_TOKEN_ID "device_renewal_token"
//@}

/// Cross-application flag to indicate if the device has been linked.
/// This is used in 2.6.0 and beyond.
//@{
#define VPL_SHARED_IS_DEVICE_LINKED_ID "is_device_linked"
#define VPL_SHARED_IS_DEVICE_UNLINKING_ID "is_device_unlinking"

#define VPL_SHARED_DEVICE_NOT_LINKED "0"
#define VPL_SHARED_DEVICE_LINKED "1"
//@}

/**
 * Add data to the shared location with specified ID.
 * 
 * @param object_id
 * @param input_data
 * @param data_length
 * @return status code
 */
int VPLSharedObject_AddData(const char *shared_location, const char *object_id, const void *input_data, unsigned int data_length);
/**
 * Add string to the shared location with specified ID.
 * 
 * @param object_id
 * @param input_string
 * @return status code
 */
int VPLSharedObject_AddString(const char *shared_location, const char *object_id, const char *input_string);

/**
 * Delete object in the shared location.
 * 
 * @param object_id
 * @return status code
 */
int VPLSharedObject_DeleteObject(const char *shared_location, const char *object_id);

/**
 * Get data from shared location.
 * @note You must call #VPLSharedObject_FreeData when done with \a pData.
 * 
 * @param object_id
 * @param[out] pData Returns the data
 * @param[out] data_length Returns the length of the data
 */
void VPLSharedObject_GetData(const char *shared_location, const char *object_id, void **pData, unsigned int &data_length);
/// Release pData returned by #VPLSharedObject_GetData.
void VPLSharedObject_FreeData(void* pData);

/**
 * Get string from shared location.
 * @note You must call #VPLSharedObject_FreeString when done with \a pString.
 * 
 * @param object_id
 * @param[out] pString Returns the string
 */
void VPLSharedObject_GetString(const char *shared_location, const char *object_id, char **pString);
/// Release pString returned by #VPLSharedObject_GetString.
void VPLSharedObject_FreeString(char* pString);

const char* VPLSharedObject_GetActoolLocation();
const char* VPLSharedObject_GetCredentialsLocation();

#ifdef __cplusplus
}
#endif

#endif // include guard
