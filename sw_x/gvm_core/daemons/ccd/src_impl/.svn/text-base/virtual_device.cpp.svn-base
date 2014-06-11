//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "virtual_device.hpp"

#include "log.h"
#include "vpl_th.h"
#include "vpl_conv.h"
#include "vplex_math.h"
#include "vplex_file.h"
#include "vplex_strings.h"
#include "scopeguard.hpp"

#include "ccd_core.h"
#include "ccd_storage.hpp"
#include "query.h"
#include "ias_query.hpp"
#include "nus_query.hpp"
#include "system_query.hpp"
#include "DeviceStateCache.hpp"
#include "EventManagerPb.hpp"

#include "vsds_query.hpp"
#include "tmdviewer.h"
#include "escore.h"

#include "cslsha.h"

//#define DEBUG_DUMP_UPDATE

#include <sstream>
#include <algorithm>

using namespace ccd;
using namespace std;

#define MAX_HWUUID_LENGTH 255

static inline
int Util_WriteFileFromString(const char* filePath, string str)
{
    return Util_WriteFile(filePath, str.data(), str.size());
}

int VirtualDevice_Register(const std::string &user_name, const char* password, const char* pairingToken)
{
    int rv = 0;
    vplex::ias::RegisterVirtualDeviceResponseType registerResponse;
    {
        // Note that ESCore_GetDeviceGuid may not be valid at this point.
        vplex::ias::RegisterVirtualDeviceRequestType registerRequest;
        Query_SetIasAbstractRequestFields(registerRequest);
        registerRequest.set_username(user_name);
        if (password != NULL) {
            registerRequest.set_password(password);
        } else {
			registerRequest.set_pairingtoken(pairingToken);
        }
        {
#ifndef VPL_PLAT_IS_WINRT
            const char* osUserId = CCDGetOsUserId();
            ASSERT_NOT_NULL(osUserId);
#endif
            char* hwUuid;
            rv = VPL_GetHwUuid(&hwUuid);
            if (rv < 0) {
                LOG_ERROR("%s failed: %d", "VPL_GetHwUuid", rv);
                goto done_register_device;
            }
            ON_BLOCK_EXIT(VPL_ReleaseHwUuid, hwUuid);

            // We don't want to hash the hardware UUID, since it can help us identify Acer devices
            // and detect abuse.

            // TODO: We may want to hash the osUserId unless we can think of a reason why it is
            //   useful for the infrastructure to have it verbatim.

            stringstream hwInfoStream;

#ifndef VPL_PLAT_IS_WINRT
            hwInfoStream << "V1_" << DEVICE_NAME_PREFIX << '#' << osUserId << '#' << __ccdConfig.testInstanceNum << '@' << hwUuid;
#else
            hwInfoStream << "V1_" << DEVICE_NAME_PREFIX << '#' << __ccdConfig.testInstanceNum << '@' << hwUuid;
#endif

            // Bug 4667: If the string is longer than 255 bytes, 
            // then generate the hash of hwUuid instead
            if (hwInfoStream.str().length() > MAX_HWUUID_LENGTH) {
                std::string hwUuidHash;
                rv = Util_CalcHash(hwUuid, /*out*/ hwUuidHash);
                if (rv != 0) {
                    goto done_register_device;
                }
                // Cleanup 
                hwInfoStream.str("");
                hwInfoStream.clear();
                // Assign V2 hw info
#ifndef VPL_PLAT_IS_WINRT
                std::string osUserIdHash;
                rv = Util_CalcHash(osUserId, /*out*/ osUserIdHash);
                if (rv != 0) {
                    goto done_register_device;
                }
                hwInfoStream << "V2_" << DEVICE_NAME_PREFIX << '#' << osUserIdHash << '#' << __ccdConfig.testInstanceNum << '@' << hwUuidHash;
#else
                hwInfoStream << "V2_" << DEVICE_NAME_PREFIX << '#' << __ccdConfig.testInstanceNum << '@' << hwUuidHash;
#endif
            }
            LOG_INFO("hw info: %s", hwInfoStream.str().c_str());
            registerRequest.set_hardwareinfo(hwInfoStream.str());
        }
        {
            stringstream stream;
            stream << DEVICE_NAME_PREFIX << VPLMath_Rand();
            registerRequest.set_devicename(stream.str());
        }
        rv = QUERY_IAS(VPLIas_RegisterVirtualDevice, registerRequest, registerResponse);
        if (rv != CCD_OK) {
            goto done_register_device;
        }
    }
    {
        char path[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForDeviceId(sizeof(path), path);
        u64 packedDeviceId = VPLConv_hton_u64(registerResponse._inherited().deviceid());
        rv = Util_WriteFile(path, &packedDeviceId, sizeof(packedDeviceId));
        if (rv < 0) {
            LOG_ERROR("Failed to write to \"%s\"", path);
            goto done_register_device;
        }
    }
    {
        vplex::ias::RenewVirtualDeviceCredentialsRequestType renewRequest;
        Query_SetIasAbstractRequestFields(renewRequest);
        renewRequest.mutable__inherited()->set_deviceid(registerResponse._inherited().deviceid());
        renewRequest.set_renewaltoken(registerResponse.renewaltoken());
        vplex::ias::RenewVirtualDeviceCredentialsResponseType renewResponse;
        rv = QUERY_IAS(VPLIas_RenewVirtualDeviceCredentials, renewRequest, renewResponse);
        if (rv != CCD_OK) {
            goto done_register_device;
        }
        
        {
            char path[CCD_PATH_MAX_LENGTH];
            
            DiskCache::getPathForDeviceCredsClear(sizeof(path), path);
            Util_WriteFileFromString(path, renewResponse.cleardevicecredentials());
            if (rv < 0) {
                LOG_ERROR("Failed to write to \"%s\"", path);
                goto done_register_device;
            }
            LOG_INFO("Successfully wrote \"%s\"", path);
            
            DiskCache::getPathForDeviceCredsSecret(sizeof(path), path);
            Util_WriteFileFromString(path, renewResponse.secretdevicecredentials());
            if (rv < 0) {
                LOG_ERROR("Failed to write to \"%s\"", path);
                goto done_register_device;
            }
            LOG_INFO("Successfully wrote \"%s\"", path);
            DiskCache::getPathForNewRenewalToken(sizeof(path), path);
            rv = Util_WriteFileFromString(path, renewResponse.renewaltoken());
            if (rv < 0) {
                LOG_ERROR("Failed to write to \"%s\"", path);
                goto done_register_device;
            }

            // Cleanup any old incorrect token (this is technically optional since it won't be read).
            DiskCache::getPathForOldRenewalToken(sizeof(path), path);
            (void)VPLFile_Delete(path);

#if CCD_USE_SHARED_CREDENTIALS
            // Set shared device credentials
            vplex::sharedCredential::DeviceCredential deviceCredential;
            deviceCredential.set_device_id(registerResponse._inherited().deviceid());
            deviceCredential.set_creds_clear(renewResponse.cleardevicecredentials());
            deviceCredential.set_creds_secret(renewResponse.secretdevicecredentials());
            deviceCredential.set_renewal_token(renewResponse.renewaltoken());
            // Write the device credential to the shared location
            rv = WriteDeviceCredential(deviceCredential);
            if (rv < 0) {
                LOG_ERROR("Failed to write device credential to shared location: %d", rv);
                goto done_register_device;
            }
#endif

            DiskCache::getRootPathForDeviceCreds(sizeof(path), path);
            
            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
            ccdiEvent->mutable_device_cred_change()->set_change_type(ccd::DEVICE_CRED_CHANGE_TYPE_WRITE);
            ccdiEvent->mutable_device_cred_change()->set_local_file_root_path(path);
            EventManagerPb_AddEvent(ccdiEvent);
        }
    }
 done_register_device:
    return rv;
}

// These buffers need to remain valid for as long as ESCore is being used, since ESCore
// does not make its own copies.
static void* tempBufClear = NULL;
static void* tempBufSecret = NULL;

int VirtualDevice_HasCredentials()
{
    char path[CCD_PATH_MAX_LENGTH];

    DiskCache::getPathForDeviceId(sizeof(path), path);
    if (VPLFile_CheckAccess(path, VPLFILE_CHECKACCESS_READ) != VPL_OK) {
        return 0;
    }
    DiskCache::getPathForDeviceCredsClear(sizeof(path), path);
    if (VPLFile_CheckAccess(path, VPLFILE_CHECKACCESS_READ) != VPL_OK) {
        return 0;
    }
    DiskCache::getPathForDeviceCredsSecret(sizeof(path), path);
    if (VPLFile_CheckAccess(path, VPLFILE_CHECKACCESS_READ) != VPL_OK) {
        return 0;
    }
    return 1;
}

void VirtualDevice_ClearLocalDeviceCredentials()
{
    // Note we ignore the return value of VPLFile_Delete(), as we want to delete as much of the files as possible.
    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForDeviceId(sizeof(path), path);
    (void)VPLFile_Delete(path);
    DiskCache::getPathForDeviceCredsClear(sizeof(path), path);
    (void)VPLFile_Delete(path);
    DiskCache::getPathForDeviceCredsSecret(sizeof(path), path);
    (void)VPLFile_Delete(path);
    DiskCache::getPathForOldRenewalToken(sizeof(path), path);
    (void)VPLFile_Delete(path);
    DiskCache::getPathForNewRenewalToken(sizeof(path), path);
    (void)VPLFile_Delete(path);
}

/// Delete any device credential files, so that they will be fetched from infra later.
static void VirtualDevice_RemoveCredentials()
{
    VirtualDevice_ClearLocalDeviceCredentials();
#if CCD_USE_SHARED_CREDENTIALS
    {
        // Delete shared device credential
        DeleteCredential(VPL_DEVICE_CREDENTIAL);
    }
#endif
    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getRootPathForDeviceCreds(sizeof(path), path);
    
    ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
    ccdiEvent->mutable_device_cred_change()->set_change_type(ccd::DEVICE_CRED_CHANGE_TYPE_DELETE);
    ccdiEvent->mutable_device_cred_change()->set_local_file_root_path(path);
    EventManagerPb_AddEvent(ccdiEvent);
}

static u64 deviceId = 0;

u64 VirtualDevice_GetDeviceId()
{
    return deviceId;
}

// The following are hard coded default credentials
static bool ec_is_init = 0;
static u8 cred_default[] = {
    0x01, 0x00,                 // version 1
    0x00, 0x00,                 // padding
    0x00, 0x00, 0x00, 0x00,     // pub type RSA4096
    0x00, 0x01, 0x00, 0x01,     // root_kpub_exp          
    // platform common key
    0xa2, 0xc8, 0x80, 0x41, 0xe2, 0x36, 0x73, 0x4b, 0x43, 0x6e, 0x76, 0x4b,
    0xcb, 0x83, 0x0f, 0x61,
    // rsa 4096 cert
    0xe5, 0x05, 0xce, 0xec, 0x5f, 0xf7, 0x61, 0xa7,
    0x8a, 0x38, 0xfc, 0x8c, 0x6e, 0x1e, 0x21, 0x53,
    0x39, 0x8c, 0x3c, 0x44, 0xd0, 0x04, 0xc8, 0x27,
    0xfa, 0xac, 0xbd, 0x06, 0x05, 0xe6, 0x88, 0x98,
    0x76, 0x32, 0xb3, 0x48, 0x7d, 0x96, 0x08, 0x74,
    0xab, 0xe4, 0xff, 0x3b, 0x2d, 0xbd, 0x20, 0x6d,
    0xed, 0xd0, 0x06, 0xd5, 0x03, 0x26, 0xd0, 0x1b,
    0x11, 0xba, 0x80, 0x4f, 0x15, 0x82, 0xb1, 0xd9,
    0x85, 0xc3, 0x97, 0xbd, 0x81, 0x30, 0x72, 0xd2,
    0x0e, 0x16, 0x18, 0x03, 0x0c, 0xc6, 0x75, 0xc3,
    0xf4, 0x7d, 0xb6, 0xf6, 0x91, 0x19, 0x49, 0xf8,
    0xab, 0x06, 0x2c, 0x5d, 0xf5, 0x19, 0xe4, 0x85,
    0x1f, 0x70, 0x94, 0xa6, 0x40, 0x0c, 0x88, 0x1a,
    0x3d, 0xdf, 0x8f, 0x61, 0x56, 0xc8, 0x88, 0xd1,
    0x05, 0x1e, 0xff, 0x5e, 0xda, 0x57, 0xff, 0x6e,
    0x80, 0x1f, 0x28, 0x6a, 0x89, 0x62, 0xad, 0x73,
    0x25, 0xd5, 0xdd, 0x8e, 0x6b, 0x5b, 0x5f, 0x84,
    0x4b, 0xf9, 0x58, 0xfa, 0xff, 0x60, 0xd7, 0x05,
    0xa4, 0x15, 0x22, 0x14, 0x0e, 0xea, 0xe6, 0x74,
    0x60, 0x30, 0x32, 0x75, 0x45, 0xeb, 0xfe, 0x99,
    0x58, 0xf8, 0x5c, 0x41, 0xce, 0xbb, 0xbd, 0xa7,
    0x8d, 0x22, 0x38, 0x61, 0x1b, 0x9d, 0x02, 0x3e,
    0x16, 0xf4, 0x86, 0x80, 0x87, 0xc9, 0xb2, 0xb2,
    0x67, 0xce, 0xbb, 0x71, 0xf5, 0x72, 0xc9, 0x56,
    0x95, 0x47, 0xd8, 0x43, 0xd8, 0x2a, 0x3a, 0x60,
    0x66, 0x7b, 0x3b, 0x2a, 0xd7, 0x65, 0x9c, 0xee,
    0x97, 0x67, 0x9b, 0xe3, 0x8f, 0x40, 0xd3, 0x5d,
    0xeb, 0xca, 0x81, 0x71, 0x9f, 0xbb, 0x93, 0x37,
    0x6b, 0x9f, 0x08, 0xc0, 0x31, 0x00, 0xe9, 0x86,
    0x1b, 0x34, 0x38, 0xc7, 0x65, 0xe2, 0xe6, 0xa5,
    0xe6, 0xea, 0x48, 0x04, 0x40, 0x3a, 0x21, 0xeb,
    0xce, 0xeb, 0x5d, 0x14, 0x73, 0x98, 0xda, 0x32,
    0x10, 0x62, 0x82, 0x1f, 0xab, 0x5b, 0x1a, 0x22,
    0xb8, 0x6e, 0x9f, 0x76, 0x74, 0x30, 0x33, 0x3a,
    0x51, 0x42, 0xe2, 0x9e, 0xf8, 0x26, 0x19, 0x18,
    0x05, 0xd6, 0xc2, 0x70, 0x45, 0x08, 0x8d, 0xdf,
    0xc9, 0x00, 0xa3, 0x48, 0xfb, 0xba, 0x10, 0xcd,
    0xbd, 0xfe, 0x57, 0xa2, 0x97, 0x89, 0x6b, 0x4a,
    0x62, 0xd2, 0xf5, 0xcc, 0x04, 0x95, 0x4f, 0xe8,
    0x01, 0x41, 0x29, 0xe6, 0x02, 0x0e, 0x98, 0x05,
    0x3f, 0x61, 0x8f, 0x19, 0xc2, 0x49, 0x8a, 0x3e,
    0x1e, 0xc6, 0xff, 0xc8, 0x23, 0xc3, 0xbe, 0x43,
    0x4a, 0x36, 0x2a, 0x32, 0xc5, 0x97, 0x9b, 0xfc,
    0x14, 0x30, 0xac, 0x04, 0xfb, 0xd6, 0x23, 0x86,
    0xd3, 0xae, 0xf2, 0xae, 0xef, 0x1f, 0x53, 0x1d,
    0x95, 0x9f, 0x78, 0xf8, 0x9f, 0xc8, 0xb8, 0x90,
    0x79, 0x00, 0x20, 0xb2, 0xa5, 0x73, 0xcb, 0x28,
    0x8d, 0xf6, 0xaa, 0xbd, 0x01, 0x74, 0x5b, 0x83,
    0x49, 0x63, 0xa4, 0x6a, 0x2f, 0xe3, 0x40, 0x82,
    0x7d, 0xa1, 0xd0, 0x62, 0xcd, 0xf1, 0x23, 0xb4,
    0x5b, 0xab, 0x35, 0x15, 0x18, 0xb7, 0x7a, 0x95,
    0xd3, 0x56, 0x67, 0xb9, 0xe9, 0x25, 0xe0, 0x11,
    0xd8, 0x20, 0x74, 0xeb, 0xc5, 0x28, 0xd9, 0x7e,
    0x6e, 0x58, 0x18, 0x52, 0xb8, 0xbe, 0x52, 0xbc,
    0xaf, 0xb5, 0xff, 0xfa, 0xfe, 0xca, 0xf1, 0x1c,
    0x67, 0x41, 0x43, 0xa8, 0x03, 0x59, 0x42, 0x8f,
    0x03, 0x85, 0x66, 0xb6, 0x6b, 0x91, 0x8d, 0x59,
    0xc1, 0xcc, 0x54, 0xa4, 0x3a, 0x9e, 0xb6, 0xbe,
    0x5e, 0xb6, 0x02, 0x90, 0x15, 0x5f, 0xe4, 0xd6,
    0x67, 0x40, 0x41, 0xa3, 0x18, 0x90, 0x1c, 0x34,
    0xcb, 0xbd, 0xb3, 0xb1, 0x1c, 0x28, 0xd4, 0xdd,
    0x94, 0x56, 0x6f, 0x16, 0x44, 0x51, 0x5d, 0x1d,
    0x33, 0x24, 0x67, 0xcf, 0x9d, 0x5e, 0xde, 0x27,
    0x3d, 0xcc, 0xc1, 0xb4, 0xd3, 0xa5, 0x02, 0xe1,
};

#if CCD_USE_SHARED_CREDENTIALS
int VirtualDevice_LoadSharedCredentials(const vplex::sharedCredential::DeviceCredential& deviceCredential)
{
    char path[CCD_PATH_MAX_LENGTH];
    {
        if(deviceCredential.has_device_id()){
            LOG_INFO("Shared device credentials found, writing to file");
            
            DiskCache::getPathForDeviceId(sizeof(path), path);
            u64 packedDeviceId = VPLConv_hton_u64(deviceCredential.device_id());
            int rv = Util_WriteFile(path, &packedDeviceId, sizeof(packedDeviceId));
            if (rv < 0) {
                LOG_ERROR("Failed to write to \"%s\"", path);
                return rv;
            }
            LOG_INFO("Successfully wrote \"%s\"", path);
            
            DiskCache::getPathForNewRenewalToken(sizeof(path), path);
            rv = Util_WriteFileFromString(path, deviceCredential.renewal_token());
            if (rv < 0) {
                LOG_ERROR("Failed to write to \"%s\"", path);
                return rv;
            }
            LOG_INFO("Successfully wrote \"%s\"", path);
            
            DiskCache::getPathForDeviceCredsClear(sizeof(path), path);
            rv = Util_WriteFileFromString(path, deviceCredential.creds_clear());
            if (rv < 0) {
                LOG_ERROR("Failed to write to \"%s\"", path);
                return rv;
            }
            LOG_INFO("Successfully wrote \"%s\"", path);
            
            DiskCache::getPathForDeviceCredsSecret(sizeof(path), path);
            rv = Util_WriteFileFromString(path, deviceCredential.creds_secret());
            if (rv < 0) {
                LOG_ERROR("Failed to write to \"%s\"", path);
                return rv;
            }
            LOG_INFO("Successfully wrote \"%s\"", path);
            
            rv = VirtualDevice_LoadCredentials();
            if (rv < 0) {
                LOG_ERROR("%s failed: %d", "VirtualDevice_LoadCredentials", rv);
                return rv;
            }
        }
    }
    return VPL_OK;
}
#endif

int VirtualDevice_LoadCredentials()
{
    int rv = 0;
    void* newClearBuf = NULL;
    void* newSecretBuf = NULL;
    u32 readLenClear = -1;
    u32 readLenSecret = -1;
    char path[CCD_PATH_MAX_LENGTH];

    // Load the default credentials
    if ( !ec_is_init ) {
        rv = ESCore_Init(cred_default, sizeof(cred_default));
        if ( rv < 0 ) {
            LOG_ERROR("ESCore_Init failed with error code %d", rv);
            return rv;
        }
        ec_is_init = 1;
    }

    if (!VirtualDevice_HasCredentials()) {
        LOG_INFO("No device credentials yet");
        return CCD_ERROR_CREDENTIALS_MISSING;
    }

    // load deviceId from file and cache it
    {
        DiskCache::getPathForDeviceId(sizeof(path), path);
        u64 *packedDeviceId;
        rv = Util_ReadFile(path, (void**)&packedDeviceId, sizeof(*packedDeviceId));
        if (rv < 0) {
            LOG_ERROR("Failed to read from deviceId: %d", rv);
            goto fail_cred_load;
        }
        deviceId = VPLConv_ntoh_u64(*packedDeviceId);
        free(packedDeviceId);
    }

    DiskCache::getPathForDeviceCredsClear(sizeof(path), path);
    rv = Util_ReadFile(path, &newClearBuf, 0);
    if (rv < 0) {
        LOG_ERROR("Failed to read dev_cred_clear: %d", rv);
        goto fail_cred_load;
    }
    LOG_INFO("Read %d bytes from dev_cred_clear", rv);
    readLenClear = INT_TO_U32(rv);

    DiskCache::getPathForDeviceCredsSecret(sizeof(path), path);
    rv = Util_ReadFile(path, &newSecretBuf, 0);
    if (rv < 0) {
        LOG_ERROR("Failed to read dev_cred_secret: %d", rv);
        goto fail_cred_load;
    }
    LOG_INFO("Read %d bytes from dev_cred_secret", rv);
    readLenSecret = INT_TO_U32(rv);

    LOG_INFO("Using downloaded credentials");
    rv = ESCore_LoadCredentials(newSecretBuf, readLenSecret, newClearBuf, readLenClear, ESCORE_INIT_MODE_NORMAL);
    if (rv < 0) {
        LOG_ERROR("Failed to load existing device credentials: %d", rv);
        goto fail_cred_load;
    } else {
        free(tempBufClear);
        tempBufClear = newClearBuf;
        free(tempBufSecret);
        tempBufSecret = newSecretBuf;
        LOG_INFO("ESCore started; device credentials loaded successfully for %016"PRIx64" ("FMTu64")",
                deviceId, deviceId);
        DeviceStateCache_SetLocalDeviceId(deviceId);
    }
    goto cred_load_done;

 fail_cred_load:
    free(newClearBuf);
    free(newSecretBuf);
    VirtualDevice_RemoveCredentials();

 cred_load_done:

    return rv;
}

int VirtualDevice_GetDeviceCert(std::string& device_cert)
{
    // HACK: location of devcert within clear credential
    device_cert.assign((char*)tempBufClear+76, 384);

    return 0;
}

#if 0
int VirtualDevice_GetRenewalToken(std::string& device_token_out)
{
    int rv = CCD_ERROR_CREDENTIALS_MISSING;
    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForNewRenewalToken(sizeof(path), path);
    void* devTokenBuf = NULL;
    int bytesRead = Util_ReadFile(path, &devTokenBuf, 0);
    if (bytesRead < 0) {
        LOG_ERROR("Failed to read renewal token: %d", bytesRead);
        goto out;
    }
    ON_BLOCK_EXIT(free, devTokenBuf);
    if (bytesRead < 1) {
        LOG_ERROR("Renewal token was 0-length");
    }
    device_token_out.assign((char*)devTokenBuf, bytesRead);
    rv = 0;
out:
    return rv;
}
#endif
