#import <Foundation/Foundation.h>

const char* server_url;
const char* branch; 
const char* product; 

const char* getRootPath()
{
    NSString *home_dir = NSHomeDirectory();
    NSString *tmp_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    return [tmp_dir UTF8String];
}

void initEnv(){
    NSString* filePath = @"vplexTest";
    NSString* fileRoot = [[NSBundle mainBundle] 
               pathForResource:filePath ofType:@".conf"];

    printf("\n---initEnv---\n");

    @try {
        // read everything from text
        NSString* fileContents = 
            [NSString stringWithContentsOfFile:fileRoot 
            encoding:NSUTF8StringEncoding error:nil];

        // first, separate by new line
        NSArray* allLinedStrings = 
            [fileContents componentsSeparatedByCharactersInSet:
            [NSCharacterSet newlineCharacterSet]];

        NSString* str_server_url = [allLinedStrings objectAtIndex:0];
        NSString* str_branch = [allLinedStrings objectAtIndex:1];
        NSString* str_product = [allLinedStrings objectAtIndex:2];

        if( [str_server_url length] != 0 ) {
            server_url = [str_server_url UTF8String];
        }else{
            server_url = "ccd-http-test.ctbg.acer.com";
        }

        if( [str_branch length] != 0 ) {
            branch = [str_branch UTF8String];
        }else{
            branch = "DEV";
        }

        if( [str_product length] != 0 ) {
            product = [str_product UTF8String];
        }else{
            product = "ios";
        }
    }

    @catch ( NSException *e ) {
        server_url = "ccd-http-test.ctbg.acer.com";
        branch = "DEV";
        product = "ios";
    }

}
