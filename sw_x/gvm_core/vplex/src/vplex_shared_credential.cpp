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

#include <vpl_time.h>
#include "vplex_shared_credential.hpp"
#include "vplex_shared_object.h"
#include "scopeguard.hpp"
#include "vplex_private.h"
#include "vpl_conv.h"

static int ReadSharedCredentialToString(const char *credentialId, std::string &credentialString);
static int WriteSharedCredentialFromString(const char *credentialId, const std::string *credentialString);

int GetUserCredential(vplex::sharedCredential::UserCredential& userCredential)
{
    int rv = VPL_OK;
    std::string userCredentialString;
    rv = ReadSharedCredentialToString(VPL_USER_CREDENTIAL, userCredentialString);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_INFO(VPL_SG_IAS,"User credential not found");
        return rv;
    }
    if(!userCredential.ParsePartialFromString(userCredentialString)) {
        rv = VPL_ERR_FAULT;
        VPL_LIB_LOG_ERR(VPL_SG_IAS,"Failed to parse user credential from string: %s", userCredentialString.c_str());
    }
    
    return rv;
}

int GetDeviceCredential(vplex::sharedCredential::DeviceCredential& deviceCredential)
{
    int rv = VPL_OK;
    std::string deviceCredentialString;
    rv = ReadSharedCredentialToString(VPL_DEVICE_CREDENTIAL, deviceCredentialString);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_INFO(VPL_SG_IAS,"Device credential not found");
        return rv;
    }
    if(!deviceCredential.ParsePartialFromString(deviceCredentialString)) {
        rv = VPL_ERR_FAULT;
        VPL_LIB_LOG_ERR(VPL_SG_IAS,"Failed to parse device credential from string: %s", deviceCredentialString.c_str());
    }
    
    return rv;
}

int WriteUserCredential(const vplex::sharedCredential::UserCredential& userCredential)
{
    int rv = VPL_OK;
    std::string userCredentialString;
    userCredential.SerializeToString(&userCredentialString);
    rv = WriteSharedCredentialFromString(VPL_USER_CREDENTIAL, &userCredentialString);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS,"Writing user credential error: %d from string: %s", rv, userCredentialString.c_str());
    }
    
    return rv;
}

int WriteDeviceCredential(const vplex::sharedCredential::DeviceCredential& deviceCredential)
{
    int rv = VPL_OK;
    std::string deviceCredentialString;
    deviceCredential.SerializeToString(&deviceCredentialString);
    rv = WriteSharedCredentialFromString(VPL_DEVICE_CREDENTIAL, &deviceCredentialString);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_IAS,"Writing device credential error: %d from string: %s", rv, deviceCredentialString.c_str());
    }
    
    return rv;
}

int DeleteCredential(const char *credentialID)
{
    const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
    // Delete shared credentials
    return VPLSharedObject_DeleteObject(credentialsLocation, credentialID);
}

// Migrate shared credentials written from CCD version prior to 2.6.0
void MigrateCredential()
{
    const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
    
    // Migrate shared user credentials
    char* userName = NULL;
    VPLSharedObject_GetString(credentialsLocation, VPL_SHARED_USER_NAME_ID, &userName);
    ON_BLOCK_EXIT(VPLSharedObject_FreeString, userName);
    
    unsigned int sharedIASOutputLength = 0;
    void* sharedIASOutputPointer = NULL;
    VPLSharedObject_GetData(credentialsLocation, VPL_SHARED_USER_IASOUTPUT_ID, &sharedIASOutputPointer, sharedIASOutputLength);
    ON_BLOCK_EXIT(VPLSharedObject_FreeData, sharedIASOutputPointer);
    
    if ((userName != NULL) && (sharedIASOutputPointer != NULL)) {
        VPL_LIB_LOG_INFO(VPL_SG_IAS,"Old shared user credentials found, trying to migrate");
        vplex::sharedCredential::UserCredential userCredential;
        
        userCredential.set_user_name(userName);
        userCredential.set_ias_output(sharedIASOutputPointer, sharedIASOutputLength);
        WriteUserCredential(userCredential);
        
        // Cleanup old user credentials
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_USER_NAME_ID);
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_USER_IASOUTPUT_ID);
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_ANS_SESSION_KEY_ID);
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_ANS_LOGIN_BLOB_ID);
    }
    
    // Migrate shared device credentials
    void* sharedDeviceIdData = NULL;
    unsigned int deviceIdLength = 0;
    VPLSharedObject_GetData(credentialsLocation, VPL_SHARED_DEVICE_ID, &sharedDeviceIdData, deviceIdLength);
    ON_BLOCK_EXIT(VPLSharedObject_FreeData, sharedDeviceIdData);
    
    if (sharedDeviceIdData !=  NULL) {
        VPL_LIB_LOG_INFO(VPL_SG_IAS,"Old shared device credentials found, trying to migrate");
        vplex::sharedCredential::DeviceCredential deviceCredential;
        
        u64 packedSharedDeviceId = *((u64*)sharedDeviceIdData);
        u64 sharedDeviceId = VPLConv_ntoh_u64(packedSharedDeviceId);
        deviceCredential.set_device_id(sharedDeviceId);
        
        void* sharedDeviceCredsClearPointer = NULL;
        unsigned int deviceCredsClearLength = 0;
        VPLSharedObject_GetData(credentialsLocation, VPL_SHARED_DEVICE_CREDSCLEAR_ID, &sharedDeviceCredsClearPointer, deviceCredsClearLength);
        ON_BLOCK_EXIT(VPLSharedObject_FreeData, sharedDeviceCredsClearPointer);
        std::string sharedDeviceCredsClear((char*)sharedDeviceCredsClearPointer, deviceCredsClearLength);
        deviceCredential.set_creds_clear(sharedDeviceCredsClear);
        
        void* sharedDeviceCredsSecretPointer =  NULL;
        unsigned int deviceCredsSecretLength = 0;
        VPLSharedObject_GetData(credentialsLocation, VPL_SHARED_DEVICE_CREDSECRET_ID, &sharedDeviceCredsSecretPointer, deviceCredsSecretLength);
        ON_BLOCK_EXIT(VPLSharedObject_FreeData, sharedDeviceCredsSecretPointer);
        std::string sharedDeviceCredsSecret((char*)sharedDeviceCredsSecretPointer, deviceCredsSecretLength);
        deviceCredential.set_creds_secret(sharedDeviceCredsSecret);
        
        char* sharedDeviceRenewalToken = NULL;
        VPLSharedObject_GetString(credentialsLocation, VPL_SHARED_DEVICE_RENEWAL_TOKEN_ID, &sharedDeviceRenewalToken);
        ON_BLOCK_EXIT(VPLSharedObject_FreeString, sharedDeviceRenewalToken);
        deviceCredential.set_renewal_token(sharedDeviceRenewalToken);
        
        WriteDeviceCredential(deviceCredential);
        
        // Cleanup old device credentials
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_DEVICE_ID);
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_DEVICE_CREDSCLEAR_ID);
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_DEVICE_CREDSECRET_ID);
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_DEVICE_RENEWAL_TOKEN_ID);
    }
}

//------- Private functions
static int ReadSharedCredentialToString(const char *credentialId, std::string &credentialString)
{
    const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
    
    // Check user credential.
    unsigned int credLength = 0;
    void* credPointer = NULL;
    VPLSharedObject_GetData(credentialsLocation, credentialId, &credPointer, credLength);
    ON_BLOCK_EXIT(VPLSharedObject_FreeData, credPointer);
    if (credPointer == NULL) {
        return VPL_ERR_NOENT;
    }
    
    credentialString.assign((char*)credPointer, credLength);
    
    return VPL_OK;
}

static int WriteSharedCredentialFromString(const char *credentialId, const std::string *credentialString)
{
    const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
    return VPLSharedObject_AddData(credentialsLocation, credentialId, credentialString->data(), credentialString->size());
}
