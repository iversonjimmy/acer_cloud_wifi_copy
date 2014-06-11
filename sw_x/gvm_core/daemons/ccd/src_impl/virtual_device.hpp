//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VIRTUAL_DEVICE_HPP__
#define __VIRTUAL_DEVICE_HPP__

#include "ccdi.hpp"
#if CCD_USE_SHARED_CREDENTIALS
#include "vplex_shared_credential.hpp"
#endif

/*
 * Return the device ID.
 * 0 is returned if the device has no valid ID assigned.
 */
u64 VirtualDevice_GetDeviceId();

/*
 * Check whether or not there is device credential in local storage.
 *
 * Return values:
 *  =1 : Device credential found.
 *  =0 : Device credential not found.
 */
int VirtualDevice_HasCredentials();

#if CCD_USE_SHARED_CREDENTIALS
/*
 * Try to copy the shared device credential to local storage and then load it.
 *
 * Return values:
 *  =0 : success
 *  <0 : error
 */
int VirtualDevice_LoadSharedCredentials(const vplex::sharedCredential::DeviceCredential& deviceCredential);
#endif

/*
 * Try to load the device credential from local storage.
 * If there is not, it will return CCD_ERROR_CREDENTIALS_MISSING.
 * In this case, the caller should call VirtualDevice_Register() to obtain a device credential.
 *
 * Return values:
 *  =0 : success
 *  <0 : error
 */
int VirtualDevice_LoadCredentials();

/*
 * Register the device.
 * This will also obtain a device credential.
 * The caller is still responsible for calling VirtualDevice_LoadCredentials() 
 * to load the credential.
 */
int VirtualDevice_Register(const std::string &user_name, const char* password, const char* pairingToken);

/*
 * This function returns the device cert from the clear credentials.
 * This is needed by sw_update to import tickets used to decrypt software
 * updates.
 */
int VirtualDevice_GetDeviceCert(std::string& device_cert);

/// Delete any local device credential files, so that they will be fetched from the shared
/// credentials location later.
void VirtualDevice_ClearLocalDeviceCredentials();

#endif // include guard
