//
//  NSDirectoryWrapper.m
//  vpl
//
//  Created by build on 13/3/12.
//
//

#import "NSDirectoryWrapper.h"

@implementation NSDirectoryWrapper

const char* NSDirectory_GetHomeDirectory()
{
    NSString *homeDirString = NSHomeDirectory();
    return [homeDirString cStringUsingEncoding:[NSString defaultCStringEncoding]];
}

@end
