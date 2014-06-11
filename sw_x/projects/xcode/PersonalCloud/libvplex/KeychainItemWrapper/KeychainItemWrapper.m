#import "KeychainItemWrapper.h"
#import <Security/Security.h>

#if ! __has_feature(objc_arc)
#error THIS CODE MUST BE COMPILED WITH ARC ENABLED!
#endif

/*
 
 These are the default constants and their respective types,
 available for the kSecClassGenericPassword Keychain Item class:
 
 kSecAttrAccessGroup			-		CFStringRef
 kSecAttrCreationDate		-		CFDateRef
 kSecAttrModificationDate    -		CFDateRef
 kSecAttrDescription			-		CFStringRef
 kSecAttrComment				-		CFStringRef
 kSecAttrCreator				-		CFNumberRef
 kSecAttrType                -		CFNumberRef
 kSecAttrLabel				-		CFStringRef
 kSecAttrIsInvisible			-		CFBooleanRef
 kSecAttrIsNegative			-		CFBooleanRef
 kSecAttrAccount				-		CFStringRef
 kSecAttrService				-		CFStringRef
 kSecAttrGeneric				-		CFDataRef
 
 See the header file Security/SecItem.h for more details.
 
 */

@interface KeychainItemWrapper (PrivateMethods)

- (NSMutableDictionary*)getKeychainItemDataWithIdentifier: (NSString *)identifier status:(OSStatus *) status;
- (NSMutableDictionary *)createKeychainItem:(NSString *)identifier;
/*
 The decision behind the following two methods (secItemFormatToDictionary and dictionaryToSecItemFormat) was
 to encapsulate the transition between what the detail view controller was expecting (NSString *) and what the
 Keychain API expects as a validly constructed container class.
 */
- (NSMutableDictionary *)secItemFormatToDictionary:(NSDictionary *)dictionaryToConvert status:(OSStatus *) status;
- (NSMutableDictionary *)dictionaryToSecItemFormat:(NSDictionary *)dictionaryToConvert;

// Updates the item in the keychain, or adds it if it doesn't exist.
- (bool)writeToKeychain:(NSMutableDictionary *) keychainItemData status:(OSStatus *) status;

@end

@implementation KeychainItemWrapper
{
    //    NSMutableDictionary *keychainItemData;		// The actual keychain item data backing store.
    NSMutableDictionary *genericPasswordQuery;	// A placeholder for the generic keychain item query used to locate the item.
    NSString *currentAccessGroup;
}

- (id)initWithAccessGroup:(NSString *) accessGroup
{
    if (self = [super init])
    {
        // Begin Keychain search setup. The genericPasswordQuery leverages the special user
        // defined attribute kSecAttrGeneric to distinguish itself between other generic Keychain
        // items which may be included by the same application.
        genericPasswordQuery = [[NSMutableDictionary alloc] init];
        
		[genericPasswordQuery setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
        
		// The keychain access group attribute determines if this item can be shared
		// amongst multiple apps whose code signing entitlements contain the same keychain access group.
		if (accessGroup != nil)
		{
            currentAccessGroup = accessGroup;
#if TARGET_IPHONE_SIMULATOR
			// Ignore the access group if running on the iPhone simulator.
			// 
			// Apps that are built for the simulator aren't signed, so there's no keychain access group
			// for the simulator to check. This means that all apps can see all keychain items when run
			// on the simulator.
			//
			// If a SecItem contains an access group attribute, SecItemAdd and SecItemUpdate on the
			// simulator will return -25243 (errSecNoAccessForItem).
#else			
			[genericPasswordQuery setObject:accessGroup forKey:(__bridge id)kSecAttrAccessGroup];
#endif
		}
    }
	return self;
}

- (id)getObjectWithIdentifier: (NSString *)identifier status:(OSStatus *) status
{
    OSStatus getItemStatus = noErr;
    NSMutableDictionary *keychainItemData = [self getKeychainItemDataWithIdentifier:identifier status:&getItemStatus];
    status = &getItemStatus;
    
	return [keychainItemData objectForKey:(__bridge id)kSecAttrService];
}

- (NSMutableDictionary*)getKeychainItemDataWithIdentifier: (NSString *)identifier status:(OSStatus *) status
{
    
    [genericPasswordQuery setObject:identifier forKey:(__bridge id)kSecAttrGeneric];
    
    // Use the proper search constants, return only the attributes of the first match.
    [genericPasswordQuery setObject:(__bridge id)kSecMatchLimitOne forKey:(__bridge id)kSecMatchLimit];
    [genericPasswordQuery setObject:(__bridge id)kCFBooleanTrue forKey:(__bridge id)kSecReturnAttributes];
    
    NSDictionary *tempQuery = [NSDictionary dictionaryWithDictionary:genericPasswordQuery];
    
    CFMutableDictionaryRef outDictionary = NULL;
    
    NSMutableDictionary *keychainItemData;
    
    if (!SecItemCopyMatching((__bridge CFDictionaryRef)tempQuery, (CFTypeRef *)&outDictionary) == noErr)
    {
        // Can't find the keychain item, return NULL.
        return nil;
    }
    else
    {
        OSStatus formatStatus = noErr;
        // load the saved data from Keychain.
        keychainItemData = [self secItemFormatToDictionary:(__bridge NSDictionary *)outDictionary status:&formatStatus];
        status = &formatStatus;
    }
    if(outDictionary) CFRelease(outDictionary);
    
	return keychainItemData;
}

- (bool)setObject:(id)inObject forIdentifier:(NSString *)identifier status:(OSStatus *) status
{
    if (inObject == nil) return NO;
    
    OSStatus operationStatus = noErr;
    id currentObject = [self getObjectWithIdentifier:identifier status:&operationStatus];
    if (operationStatus != noErr && operationStatus != errSecItemNotFound) {
        status = &operationStatus;
        return NO;
    }
    
    if (![currentObject isEqual:inObject])
    {
        NSMutableDictionary *keychainItemData = [self getKeychainItemDataWithIdentifier:identifier status:&operationStatus];
        if (operationStatus != noErr && operationStatus != errSecItemNotFound) {
            status = &operationStatus;
            return NO;
        }
        
        if (keychainItemData == nil) {
            keychainItemData = [self createKeychainItem:identifier];
        }
        
        [keychainItemData setObject:inObject forKey:(__bridge id)kSecAttrService];
        return [self writeToKeychain:keychainItemData status:&*status];
    }
    
    return YES;
}

- (NSMutableDictionary *)createKeychainItem:(NSString *)identifier
{
    NSMutableDictionary *createdKeychainItemData = [[NSMutableDictionary alloc] init];
    
    // Default attributes for keychain item.
    [createdKeychainItemData setObject:@"" forKey:(__bridge id)kSecAttrAccount];
    [createdKeychainItemData setObject:@"" forKey:(__bridge id)kSecAttrLabel];
    [createdKeychainItemData setObject:@"" forKey:(__bridge id)kSecAttrDescription];
    
    // Default data for keychain item.
    [createdKeychainItemData setObject:@"" forKey:(__bridge id)kSecValueData];
    
    // Add the generic attribute and the keychain access group.
    [createdKeychainItemData setObject:identifier forKey:(__bridge id)kSecAttrGeneric];
    // Set the accessible to kSecAttrAccessibleAfterFirstUnlock, not rely on the system default.
    [createdKeychainItemData setObject:(__bridge id)kSecAttrAccessibleAlwaysThisDeviceOnly forKey:(__bridge id)kSecAttrAccessible];
    if (currentAccessGroup != nil)
    {
#if TARGET_IPHONE_SIMULATOR
        // Ignore the access group if running on the iPhone simulator.
        // 
        // Apps that are built for the simulator aren't signed, so there's no keychain access group
        // for the simulator to check. This means that all apps can see all keychain items when run
        // on the simulator.
        //
        // If a SecItem contains an access group attribute, SecItemAdd and SecItemUpdate on the
        // simulator will return -25243 (errSecNoAccessForItem).
#else			
        [createdKeychainItemData setObject:currentAccessGroup forKey:(__bridge id)kSecAttrAccessGroup];
#endif
    }
    
    return createdKeychainItemData;
}

- (bool)resetKeychainItem:(NSString *)identifier status:(OSStatus *) status
{
    OSStatus operationStatus = noErr;
    NSMutableDictionary *keychainItemData = [self getKeychainItemDataWithIdentifier:identifier status:&operationStatus];
    if (operationStatus != noErr) {
        if (operationStatus == errSecItemNotFound) {
            // Item does not exist, no need to delete it
            return YES;
        } else {
            status = &operationStatus;
            return NO;
        }
    }
    
	operationStatus = noErr;
    if (keychainItemData)
    {
        NSMutableDictionary *tempDictionary = [self dictionaryToSecItemFormat:keychainItemData];
		operationStatus = SecItemDelete((__bridge CFDictionaryRef)tempDictionary);
        if (operationStatus != noErr && operationStatus != errSecItemNotFound) {
            NSLog(@"Problem deleting current dictionary. Identifier: %@", identifier);
            status = &operationStatus;
            return NO;
        }
    }
    
    return YES;
}

- (NSMutableDictionary *)dictionaryToSecItemFormat:(NSDictionary *)dictionaryToConvert
{
    // The assumption is that this method will be called with a properly populated dictionary
    // containing all the right key/value pairs for a SecItem.
    
    // Create a dictionary to return populated with the attributes and data.
    NSMutableDictionary *returnDictionary = [NSMutableDictionary dictionaryWithDictionary:dictionaryToConvert];
    
    // Add the Generic Password keychain item class attribute.
    [returnDictionary setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
    
    // Convert the NSString to NSData to meet the requirements for the value type kSecValueData.
	// This is where to store sensitive data that should be encrypted.
    NSString *passwordString = [dictionaryToConvert objectForKey:(__bridge id)kSecValueData];
    [returnDictionary setObject:[passwordString dataUsingEncoding:NSUTF8StringEncoding] forKey:(__bridge id)kSecValueData];
    
    return returnDictionary;
}

- (NSMutableDictionary *)secItemFormatToDictionary:(NSDictionary *)dictionaryToConvert status:(OSStatus *) status
{
    // The assumption is that this method will be called with a properly populated dictionary
    // containing all the right key/value pairs for the UI element.
    
    // Create a dictionary to return populated with the attributes and data.
    NSMutableDictionary *returnDictionary = [NSMutableDictionary dictionaryWithDictionary:dictionaryToConvert];
    
    // Add the proper search key and class attribute.
    [returnDictionary setObject:(__bridge id)kCFBooleanTrue forKey:(__bridge id)kSecReturnData];
    [returnDictionary setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
    
    // Acquire the password data from the attributes.
    CFDataRef passwordData = NULL;
    *status = SecItemCopyMatching((__bridge CFDictionaryRef)returnDictionary, (CFTypeRef *)&passwordData);
    if ( *status == noErr)
    {
        // Remove the search, class, and identifier key/value, we don't need them anymore.
        [returnDictionary removeObjectForKey:(__bridge id)kSecReturnData];
        
        // Add the password to the dictionary, converting from NSData to NSString.
        NSString *password = [[NSString alloc] initWithBytes:[(__bridge NSData *)passwordData bytes] length:[(__bridge NSData *)passwordData length] 
                                                    encoding:NSUTF8StringEncoding];
        [returnDictionary setObject:password forKey:(__bridge id)kSecValueData];
    }
    else
    {
        NSLog(@"secItemFormatToDictionary: system error with status: %ld", *status);
        return nil;
    }
	if(passwordData) CFRelease(passwordData);
    
	return returnDictionary;
}

- (bool)writeToKeychain:(NSMutableDictionary *)keychainItemData status:(OSStatus *) status
{
    CFDictionaryRef attributes = NULL;
    NSMutableDictionary *updateItem = nil;
    
    bool isSuccess = YES;
    
    if (SecItemCopyMatching((__bridge CFDictionaryRef)genericPasswordQuery, (CFTypeRef *)&attributes) == noErr)
    {
        // First we need the attributes from the Keychain.
        updateItem = [NSMutableDictionary dictionaryWithDictionary:(__bridge NSDictionary *)attributes];
        // Second we need to add the appropriate search key/values.
        [updateItem setObject:[genericPasswordQuery objectForKey:(__bridge id)kSecClass] forKey:(__bridge id)kSecClass];
        
        // Lastly, we need to set up the updated attribute list being careful to remove the class.
        NSMutableDictionary *tempCheck = [self dictionaryToSecItemFormat:keychainItemData];
        [tempCheck removeObjectForKey:(__bridge id)kSecClass];
		
#if TARGET_IPHONE_SIMULATOR
		// Remove the access group if running on the iPhone simulator.
		// 
		// Apps that are built for the simulator aren't signed, so there's no keychain access group
		// for the simulator to check. This means that all apps can see all keychain items when run
		// on the simulator.
		//
		// If a SecItem contains an access group attribute, SecItemAdd and SecItemUpdate on the
		// simulator will return -25243 (errSecNoAccessForItem).
		//
		// The access group attribute will be included in items returned by SecItemCopyMatching,
		// which is why we need to remove it before updating the item.
		[tempCheck removeObjectForKey:(__bridge id)kSecAttrAccessGroup];
#endif
        // An implicit assumption is that you can only update a single item at a time.
		
        *status = SecItemUpdate((__bridge CFDictionaryRef)updateItem, (__bridge CFDictionaryRef)tempCheck);
        if (*status  != noErr) {
            NSLog(@"Couldn't update the Keychain Item.");
            isSuccess = NO;
        }
    }
    else
    {
        // No previous item found; add the new one.
        *status  = SecItemAdd((__bridge CFDictionaryRef)[self dictionaryToSecItemFormat:keychainItemData], NULL);
        if (*status  != noErr) {
            NSLog(@"Couldn't add the Keychain Item.");
            isSuccess = NO;
        }
    }
	
	if(attributes) CFRelease(attributes);
    
    return isSuccess;
}

@end
