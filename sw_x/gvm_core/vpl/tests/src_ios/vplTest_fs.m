#import <Foundation/Foundation.h>

const char* getRootPath()
{
    NSString *home_dir = NSHomeDirectory();
    NSString *tmp_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    return [tmp_dir UTF8String];
}
