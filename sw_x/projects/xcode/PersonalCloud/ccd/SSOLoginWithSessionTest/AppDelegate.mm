//
//  AppDelegate.m
//  SSOLoginWithSessionTest
//
//  Created by Jimmy on 12/10/12.
//
//

#import "AppDelegate.h"
#include "ccd_core.h"
#include "log.h"
#include "test_single_sign_on.h"
#include "TestHelper.h"

#include <string>
#include <ccdi.hpp>

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    int rv;
    const char* test_path;
    NSFileManager* file_manager;
    NSString* home_dir;
    NSString* home_tmp_dir;
    
    NSString* ccd_config;
    
    NSString* test_account;
    
    /*
     * Redirect console logs to '%HOME/Document/console.log'.
     */
    [TestHelper redirectConsoleLogToDocumentFolder];
    
    file_manager = [NSFileManager defaultManager];
    home_dir = NSHomeDirectory();
    home_tmp_dir = [home_dir stringByAppendingPathComponent:@"tmp"];
    test_path = [home_tmp_dir UTF8String];
    
    ccd_config = [NSString stringWithContentsOfFile: [[NSBundle mainBundle] pathForResource:@"ccd.conf" ofType:@""]
                                           encoding: NSUTF8StringEncoding
                                              error: Nil];
    
    test_account = [NSString stringWithContentsOfFile: [[NSBundle mainBundle] pathForResource:@"CCDTestAccount" ofType:@""]
                                             encoding: NSUTF8StringEncoding
                                                error: Nil];
    
    /*
     * Clean up the data created by ccd last time.
     */
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
     * Start CCD before running ccd tests.
     */
    rv = CCDStart("ccd", test_path, NULL, "0");
    if (rv != 0) {
        LOG_ERROR("CCDStart failed!");
        return YES;
    }
    
    /*
     * Test to login user with password.
     */
    rv = test_login_with_credential([test_account UTF8String]);
    if (rv == CCD_OK) {
        /*
         * Test logout after login with shared credentials
         */
        test_logout();
    }

    
    printf("\nCLEAN EXIT\n");
    
    fflush(stdout);
    [TestHelper postLogFile:@"ios_ccdSSOTest_session.log"];
    
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
