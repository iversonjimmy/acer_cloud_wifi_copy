#import <Foundation/Foundation.h>
#import "CCLibSanityTest.h"

const char* getHomePath()
{
    NSFileManager *file_manager = [NSFileManager defaultManager];
    NSString *home_dir = NSHomeDirectory();
    return [home_dir UTF8String];
}

const char* getTestPath()
{
    NSFileManager *file_manager = [NSFileManager defaultManager];
    NSString *home_dir = NSHomeDirectory();
    NSString *test_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    return [test_dir UTF8String];
}
