//
//  SystemInfo.m
//  vplex
//
//  Created by Jimmy on 12/9/10.
//  Copyright (c) 2012 acer Inc. All rights reserved.
//

#import "SystemInfo.h"

@implementation SystemInfo

+ (NSString *)bundleSeedID {
    NSDictionary *query = [NSDictionary dictionaryWithObjectsAndKeys:
                           (__bridge id)kSecClassGenericPassword, kSecClass,
                           @"bundleSeedID1", kSecAttrAccount,
                           @"", kSecAttrService,
                           (id)kCFBooleanTrue, kSecReturnAttributes,
                           nil];
    CFDictionaryRef result = nil;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, (CFTypeRef *)&result);
    if (status == errSecItemNotFound)
    {
        NSMutableDictionary *queryForAdd = [[NSMutableDictionary alloc] initWithDictionary:query];
        [queryForAdd setObject:(__bridge id)kSecAttrAccessibleAlways forKey:(__bridge id)kSecAttrAccessible];
        status = SecItemAdd((__bridge CFDictionaryRef)queryForAdd, (CFTypeRef *)&result);
    }
    if (status != errSecSuccess)
        return nil;
    NSString *accessGroup = [(__bridge NSDictionary *)result objectForKey:(__bridge id)kSecAttrAccessGroup];
    NSArray *components = [accessGroup componentsSeparatedByString:@"."];
    NSString *bundleSeedID = [[components objectEnumerator] nextObject];
    CFRelease(result);
    return bundleSeedID;
}

@end
