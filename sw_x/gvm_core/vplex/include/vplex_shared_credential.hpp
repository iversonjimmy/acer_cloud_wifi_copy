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

#ifndef __VPLEX_SHARED_CREDENTIAL_H__
#define __VPLEX_SHARED_CREDENTIAL_H__

#include "vplex_plat.h"

#include "vplex_shared_credential.pb.h"
#include <string>

#define VPL_USER_CREDENTIAL "user_credential"
#define VPL_DEVICE_CREDENTIAL "device_credential"

/// Get user credentials from shared location.
/// Parse the content into \a userCredential_out.
int GetUserCredential(vplex::sharedCredential::UserCredential& userCredential_out);

/// Get device credentials from shared location.
/// Parse the content into \a deviceCredential_out.
int GetDeviceCredential(vplex::sharedCredential::DeviceCredential& deviceCredential_out);

/// Write local user credentials into shared location.
int WriteUserCredential(const vplex::sharedCredential::UserCredential& userCredential);

/// Write local device credentials into shared location.
int WriteDeviceCredential(const vplex::sharedCredential::DeviceCredential& deviceCredential);

/// Delete a shared credential.
int DeleteCredential(const char* credentialID);

/// Migrate shared credentials written from CCD version prior to 2.6.0.
void MigrateCredential();

#endif // include guard
