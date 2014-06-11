
#include "vplex_shared_object.h"
#include "vplex_trace.h"
#import "SystemInfo.h"
#import "KeychainItemWrapper.h"
#include "vpl_lazy_init.h"
#include "vplex_private.h"

#define VPL_SHARED_ACTOOL_LOCATION "actool"
#define VPL_SHARED_CREDENTIALS_LOCATION "credentials"

static VPLLazyInitMutex_t s_mutex = VPLLAZYINITMUTEX_INIT;

int VPLSharedObject_InitKeychainItemWrapper(KeychainItemWrapper **keychainItemWrapper, const char *shared_location)
{
    if(shared_location == NULL) {
        return VPL_ERR_INVALID;
    }
    // Initial keychain
    *keychainItemWrapper = [[KeychainItemWrapper alloc] initWithAccessGroup:[NSString stringWithFormat:@"%@.%@", [SystemInfo bundleSeedID], [NSString stringWithCString:shared_location encoding:NSASCIIStringEncoding]]];
    if (!keychainItemWrapper) {
        return VPL_ERR_FAIL;
    }
    return VPL_OK;
}

int VPLSharedObject_AddData(const char *shared_location, const char *object_id, const void *input_data, unsigned int data_length)
{
    int rv = VPL_OK;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    {
        KeychainItemWrapper *keychainItemWrapper;
        rv = VPLSharedObject_InitKeychainItemWrapper(&keychainItemWrapper, shared_location);
        if (rv != VPL_OK) {
            goto end;
        }
        
        if (!keychainItemWrapper) {
            rv = VPL_ERR_NOT_INIT;
            goto end;
        }
        if(object_id == NULL || input_data == NULL) {
            rv = VPL_ERR_INVALID;
            goto end;
        }
        
        OSStatus status;
        bool isSuccess = [keychainItemWrapper setObject:[NSData dataWithBytes:input_data length:data_length] forIdentifier:[NSString stringWithCString:object_id encoding:NSASCIIStringEncoding] status:&status];
        
        if (!isSuccess) {
            if (status != noErr) {
                VPL_LIB_LOG_ERR(VPL_SG_IAS,"failed with OS status: %ld location: %s object: %s", status, shared_location, object_id);
            }
            rv = VPL_ERR_ACCESS;
        }
    }
end:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mutex));
    return rv;
}

int VPLSharedObject_AddString(const char *shared_location, const char *object_id, const char *input_string)
{
    int rv = VPL_OK;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    {
        KeychainItemWrapper *keychainItemWrapper;

        rv = VPLSharedObject_InitKeychainItemWrapper(&keychainItemWrapper, shared_location);
        if (rv != VPL_OK) {
            goto end;
        }
        
        if (!keychainItemWrapper) {
            rv = VPL_ERR_NOT_INIT;
            goto end;
        }
        if(object_id == NULL || input_string == NULL) {
            rv = VPL_ERR_INVALID;
            goto end;
        }
        
        OSStatus status;
        bool isSuccess = [keychainItemWrapper setObject:[NSString stringWithCString:input_string encoding:NSASCIIStringEncoding] forIdentifier:[NSString stringWithCString:object_id encoding:NSASCIIStringEncoding] status:&status];
        
        if (!isSuccess) {
            if (status != noErr) {
                VPL_LIB_LOG_ERR(VPL_SG_IAS,"failed with OS status: %ld location: %s object: %s", status, shared_location, object_id);
            }
            rv = VPL_ERR_ACCESS;
        }
    }
end:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mutex));
    return rv;
}

int VPLSharedObject_DeleteObject(const char *shared_location, const char *object_id)
{
    int rv = VPL_OK;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    {
        KeychainItemWrapper *keychainItemWrapper;
        rv = VPLSharedObject_InitKeychainItemWrapper(&keychainItemWrapper, shared_location);
        if (rv != VPL_OK) {
            goto end;
        }
        
        if (!keychainItemWrapper) {
            rv = VPL_ERR_NOT_INIT;
            goto end;
        }
        if(object_id == NULL) {
            rv = VPL_ERR_INVALID;
            goto end;
        }
        
        OSStatus status;
        bool isSuccess = [keychainItemWrapper resetKeychainItem:[NSString stringWithCString:object_id encoding:NSASCIIStringEncoding] status:&status];
        
        if (!isSuccess) {
            if (status != noErr && status != errSecItemNotFound) {
                VPL_LIB_LOG_ERR(VPL_SG_IAS,"failed with OS status: %ld location: %s object: %s", status, shared_location, object_id);
            }
            rv = VPL_ERR_ACCESS;
        }
    }
end:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mutex));
    return rv;
}

void VPLSharedObject_GetData(const char *shared_location, const char *object_id, void **pData, unsigned int &data_length)
{
    int rv = VPL_OK;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    {
        KeychainItemWrapper *keychainItemWrapper;
        rv = VPLSharedObject_InitKeychainItemWrapper(&keychainItemWrapper, shared_location);
        if (rv != VPL_OK) {
            goto end;
        }
        
        if (!keychainItemWrapper) {
            goto end;
        }
        OSStatus status;
        NSData *sharedData = (NSData *)[keychainItemWrapper getObjectWithIdentifier:[NSString stringWithCString:object_id encoding:NSASCIIStringEncoding] status:&status];
        if (!sharedData) {
            if (status != noErr && status != errSecItemNotFound) {
                VPL_LIB_LOG_ERR(VPL_SG_IAS,"failed with OS status: %ld location: %s object: %s", status, shared_location, object_id);
            }
            goto end;
        }
        data_length = [sharedData length];
        *pData = (void*)[sharedData bytes];
    }
end:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mutex));
}

void VPLSharedObject_FreeData(void* pData)
{
    // iOS has autorelease. No need of doing it manually.
    return;
}

void VPLSharedObject_GetString(const char *shared_location, const char *object_id, char **pString)
{
    int rv = VPL_OK;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    
    {
        KeychainItemWrapper *keychainItemWrapper;
        rv = VPLSharedObject_InitKeychainItemWrapper(&keychainItemWrapper, shared_location);
        if (rv != VPL_OK) {
            goto end;
        }
        
        if (!keychainItemWrapper) {
            goto end;
        }
        
        OSStatus status;
        NSString *sharedString = (NSString *)[keychainItemWrapper getObjectWithIdentifier:[NSString stringWithCString:object_id encoding:NSASCIIStringEncoding] status:&status];
        if (!sharedString) {
            if (status != noErr && status != errSecItemNotFound) {
                VPL_LIB_LOG_ERR(VPL_SG_IAS,"failed with OS status: %ld location: %s object: %s", status, shared_location, object_id);
            }
            goto end;
        }
        
        *pString = (char*)[sharedString cStringUsingEncoding: NSASCIIStringEncoding];
    }
end:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_mutex));
}

void VPLSharedObject_FreeString(char* pString)
{
    // iOS has autorelease. No need of doing it manually.
    return;
}

const char* VPLSharedObject_GetActoolLocation()
{
    return VPL_SHARED_ACTOOL_LOCATION;
}
const char* VPLSharedObject_GetCredentialsLocation()
{
    return VPL_SHARED_CREDENTIALS_LOCATION;
}