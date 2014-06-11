//
//  KeychainFileReader.m
//  ccd
//
//  Created by Jimmy Horng on 12/6/28.
//  Copyright (c) 2012年 acer Inc. All rights reserved.
//

#import "KeychainFileReader.h"
//#import "KeychainItemWrapper.h"
#include "vplex_shared_object.h"

@implementation KeychainFileReader

void IOS_CCDConfig_CheckCcdConf(const char* local_app_data_path)
{
    // Get 'actool' location
    const char* actoolLocation = VPLSharedObject_GetActoolLocation();
    
    NSString *appDataPath = [NSString stringWithUTF8String:local_app_data_path];
    
    // Read keychain
    unsigned int confLength = 0;
    void* confBytes = NULL;
    VPLSharedObject_GetData(actoolLocation, VPL_SHARED_CCD_CONF_ID, &confBytes, confLength);
    // Covert to NSData
    NSData *confdata;
    if (confBytes) {
        confdata = [NSData dataWithBytes:confBytes length:confLength];
    }
    NSString *confFilePath = [appDataPath stringByAppendingPathComponent:[NSString stringWithFormat:@"/conf/ccd.conf"]];
    
    if(confdata) {
        NSLog(@"Data found, writing to file");
        // Create conf directory if not exist
        NSString *path = [appDataPath stringByAppendingPathComponent:[NSString stringWithFormat:@"conf"]];
        NSError *error;
        if (![[NSFileManager defaultManager] fileExistsAtPath:path])	//Does directory already exist?
        {
            if (![[NSFileManager defaultManager] createDirectoryAtPath:path
                                           withIntermediateDirectories:NO
                                                            attributes:nil
                                                                 error:&error])
            {
                NSLog(@"Create directory error: %@", error);
            }
        }
        // Write data to file
        [confdata writeToFile:confFilePath atomically:YES];
    } else {
        NSLog(@"Data not found, deleting file");
        NSError *error;
        if ([[NSFileManager defaultManager] fileExistsAtPath:confFilePath])		//Does file exist?
        {
            if (![[NSFileManager defaultManager] removeItemAtPath:confFilePath error:&error])	//Delete it
            {
                NSLog(@"Delete file error: %@", error);
            }
        }
    }
}

@end
