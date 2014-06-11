//
//  SanityAppDelegate.m
//  SanityTest
//
//  Created by wukon hsieh on 12/6/15.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#import "SanityAppDelegate.h"
#include "ccd_core.h"
#include "log.h"
#include "TestHelper.h"

#include "SyncConfigTest.hpp"

@implementation SanityAppDelegate

@synthesize window = _window;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    const char* test_path;
    NSString* home_dir;
    NSString* home_lib_dir;
    NSString* home_tmp_dir;

    /*
     * Redirect console logs to '%HOME/Document/console.log'.
     */
    [TestHelper redirectConsoleLogToDocumentFolder];

    /*
     * To clean up the credentials in the keychain.
     */
    [TestHelper clearCredentials];

    /*
     * Setup paths.
     */
    NSFileManager* file_manager = [NSFileManager defaultManager];
    home_dir = NSHomeDirectory();
    home_lib_dir = [home_dir stringByAppendingPathComponent:@"Library"];
    home_tmp_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    test_path = [home_tmp_dir UTF8String];
    
    /*
     * Clean up the data created by ccd last time.
     */
    //Remove '%HOME/Documents/My Cloud'
    [file_manager removeItemAtPath: [home_lib_dir stringByAppendingString: @"/Caches/My Cloud"]
                             error: Nil];

    //Remove '%HOME/tmp/cc'
    [file_manager removeItemAtPath: [home_tmp_dir stringByAppendingString: @"/cc"]
                             error: Nil];

    //Remove '%HOME/tmp/conf'
    [file_manager removeItemAtPath: [home_tmp_dir stringByAppendingString: @"/conf"]
                             error: Nil];

    //Remove '%HOME/tmp/logs'
    [file_manager removeItemAtPath: [home_tmp_dir stringByAppendingString: @"/logs"]
                             error: Nil];

    /*
     * Copy the ccd.conf from bundle into file system.
     */
    NSString* ccd_config = [NSString stringWithContentsOfFile: [[NSBundle mainBundle] pathForResource:@"ccd.conf" ofType:@""]
                                                     encoding: NSUTF8StringEncoding
                                                        error: Nil];
    
    [file_manager createDirectoryAtPath: [NSString stringWithFormat:@"%s/conf", test_path]
            withIntermediateDirectories: YES
                             attributes: Nil
                                  error: Nil];

    [file_manager createFileAtPath: [NSString stringWithFormat:@"%s/conf/ccd.conf", test_path]
                          contents: [ccd_config dataUsingEncoding:NSUTF8StringEncoding]
                        attributes: Nil];

    /*
     * Set the flag to make libccd ignore loading ccd.conf in keychain that created by actool.
     */
    setenv("IGNORE_ACTOOL_KEYCHAIN", "TRUE", 1);

    /*
     * Taking arguments.
     */
    NSString* test_arguments = [NSString stringWithContentsOfFile: [[NSBundle mainBundle] pathForResource:@"TestArguments" ofType:@""]
                                                        encoding: NSUTF8StringEncoding
                                                           error: Nil];
    NSArray *argument_array = [test_arguments componentsSeparatedByString:@"\n"];
    int argument_count = [argument_array count];
    argument_count++;
    
    const char* aregs[argument_count];
    aregs[0] = "SyncConfigTest";
    for (int i = 1; i < argument_count; i++) {
        NSString *argument_string = [NSString stringWithFormat:@"%@", [argument_array objectAtIndex:i-1]];
        aregs[i] = [argument_string cStringUsingEncoding:[NSString defaultCStringEncoding]];
        
        LOG_INFO("Taking argument: %s", aregs[i]);
    }

    /*
     * Run ccd sync test.
     */
    syncConfigTest(argument_count, aregs);

    printf("\nCLEAN EXIT\n");

    fflush(stdout);
    [TestHelper postLogFile:@"ios_SyncConfig.log"];

    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
