//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_PLAT_H__
#define __VPL_PLAT_H__

//============================================================================
/// @file
/// Virtual Platform Layer Run-Time Initialization and Shutdown
///
//% Including this header should also make available all of the standard
//% functions found in <stdarg.h>, <stddef.h>, <stdio.h>, <string.h>.
//% We include these other headers largely for consistency with the RVL IOP,
//% since the corresponding symbols get added all at once when it includes ioslibc.h.
//============================================================================

#include "vpl_types.h"
#include "vpl__plat.h" //% Platform specific definitions.
#include "vpl_error.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================

/// Perform any platform-specific initialization that VPL requires.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_IS_INIT #VPL_Init() has already been invoked.
/// @retval #VPL_ERR_FAIL Initialization failed.
int VPL_Init(void);

/// Perform any platform-specific cleanup that VPL requires.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_NOT_INIT #VPL_Init() must be invoked first.
/// @retval #VPL_ERR_FAIL Cleanup failed.
int VPL_Quit(void);

/// Returns #VPL_TRUE if VPL is initialized, otherwise #VPL_FALSE.
/// Does not change VPL state.
VPL_BOOL VPL_IsInit();

/// Retrieves a unique identifier for the underlying hardware.
/// @note To avoid leaking memory, you must call #VPL_ReleaseHwUuid() on the result when you
///     are finished with it.
/// @param[out] hwUuid_out Location to receive a pointer to the string.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID \a hwUuid_out was #NULL.
/// @retval #VPL_ERR_NOT_INIT #VPL_Init() must be invoked first.
/// @retval #VPL_ERR_NOMEM Out of memory.
int VPL_GetHwUuid(char** hwUuid_out);

/// Releases the memory allocated by #VPL_GetHwUuid().
/// @param[in] hwUuid The string to release. You can safely pass #NULL; nothing will happen.
void VPL_ReleaseHwUuid(char* hwUuid);

/// Returns the display name of the current user from the underlying operating system.
/// @note To avoid leaking memory, you must call #VPL_ReleaseOSUserName() on the result when you
///     are finished with it.
/// @note If the platform supports it, the user may change their name while the program is running.
///     Therefore, subsequent calls to this function are not guaranteed to return the same string.
/// @param[out] osUserName_out Location to receive a pointer to the string.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID \a osUserName_out was #NULL.
/// @retval #VPL_ERR_NOT_INIT #VPL_Init() must be invoked first.
/// @retval #VPL_ERR_NOMEM Out of memory.
int VPL_GetOSUserName(char** osUserName_out);

/// Releases the memory allocated by #VPL_GetOSUserName().
/// @param[in] osUserName The string to release. You can safely pass #NULL; nothing will happen.
void VPL_ReleaseOSUserName(char* osUserName);

#ifndef VPL_PLAT_IS_WINRT
/// Returns a unique identifier for the current user from the underlying operating system.
/// @note To avoid leaking memory, you must call #VPL_ReleaseOSUserId() on the result when you
///     are finished with it.
/// @param[out] osUserId_out Location to receive a pointer to the string.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_INVALID \a osUserId_out was #NULL.
/// @retval #VPL_ERR_NOT_INIT #VPL_Init() must be invoked first.
/// @retval #VPL_ERR_NOMEM Out of memory.
int VPL_GetOSUserId(char** osUserId_out);

/// Releases the memory allocated by #VPL_GetOSUserName().
/// @param[in] osUserId The string to release. You can safely pass #NULL; nothing will happen.
void VPL_ReleaseOSUserId(char* osUserId);
#endif

/// Returns a string identifying when VPL was built.
/// This will return a static buffer, and the value will not change while the program is running.
const char* VPL_GetBuildId(void);

/// Returns device info.
/// @note To avoid leaking memory, you must call #VPL_ReleaseDeviceInfo() on the result when you
///     are finished with it.
/// @param[out] manufacturer Location to receive a pointer to the string.
/// @param[out] model Location to receive a pointer to the string.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_FAIL Failed
int VPL_GetDeviceInfo(char** manufacturer, char** model);

/// Releases the memory allocated by #VPL_GetDeviceInfo().
/// @param[in] manufacturer The string to release. You can safely pass #NULL; nothing will happen.
/// @param[in] model        The string to release. You can safely pass #NULL; nothing will happen.
void VPL_ReleaseDeviceInfo(char* manufacturer, char* model);

/// Returns OS Version 
/// @note To avoid leaking memory, you must call #VPL_ReleaseOSVersion() on the result when you
///     are finished with it.
/// @param[out] osVersion_out Location to receive a pointer to the string.
/// @retval #VPL_OK Success.
/// @retval #VPL_ERR_FAIL Failed
int VPL_GetOSVersion(char** osVersion_out);

/// Releases the memory allocated by #VPL_GetOSVersion().
/// @param[in] osVersion The string to release. You can safely pass #NULL; nothing will happen.
void VPL_ReleaseOSVersion(char* osVersion);

#ifdef __cplusplus
}
#endif

#endif  // include guard
