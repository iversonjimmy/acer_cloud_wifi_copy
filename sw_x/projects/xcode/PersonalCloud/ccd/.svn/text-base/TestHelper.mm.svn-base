//
//  TestHelper.m
//  ccd
//
//  Created by Jimmy on 12/10/12.
//
//

#import "TestHelper.h"
#include "vplex_shared_object.h"
#include "vplex_shared_credential.hpp"

@implementation TestHelper

+ (void) clearCredentials
{
    // Get 'credentials' location
    const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
    VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_IS_DEVICE_LINKED_ID);
    
    DeleteCredential(VPL_USER_CREDENTIAL);
    DeleteCredential(VPL_DEVICE_CREDENTIAL);
}

+ (void) redirectConsoleLogToDocumentFolder
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                         NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *logPath = [documentsDirectory stringByAppendingPathComponent:@"console.log"];
    freopen([logPath cStringUsingEncoding:NSASCIIStringEncoding],"w+",stdout);
    freopen([logPath cStringUsingEncoding:NSASCIIStringEncoding],"w+",stderr);
}

+ (void)postLogFile:(NSString *)filename
{
    NSString *urlContent = [NSString stringWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"LogReceiveURL" ofType:@""]
                                                     encoding:NSUTF8StringEncoding
                                                        error:NULL];
    NSString *urlString = urlContent;
    NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];
    [request setURL:[NSURL URLWithString:urlString]];
    [request setHTTPMethod:@"POST"];
    NSString *boundary = @"---";
    NSString *contentType = [NSString stringWithFormat:@"multipart/form-data; boundary=%@",boundary];
    [request addValue:contentType forHTTPHeaderField: @"Content-Type"];
    NSMutableData *postbody = [NSMutableData data];
    [postbody appendData:[[NSString stringWithFormat:@"\r\n--%@\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
    [postbody appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"logfile\"; filename=\"%@\"\r\n", filename] dataUsingEncoding:NSUTF8StringEncoding]];
    [postbody appendData:[[NSString stringWithFormat:@"Content-Type: application/octet-stream\r\n\r\n"] dataUsingEncoding:NSUTF8StringEncoding]];
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *logPath = [documentsDirectory stringByAppendingPathComponent:@"console.log"];
    NSData *logFileData = [NSData dataWithContentsOfFile:logPath];
    
    [postbody appendData:[NSData dataWithData:logFileData]];
    [postbody appendData:[[NSString stringWithFormat:@"\r\n--%@--\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
    [request setHTTPBody:postbody];
    
    NSError *err = nil;
    NSHTTPURLResponse * httpResponse;
    int retryCount = 0;
    
    while (retryCount != 3) {
        [NSURLConnection sendSynchronousRequest:request returningResponse:&httpResponse error:&err];
        if (err) {
            NSLog(@"NSURL request failed with error: %@", err);
            retryCount++;
        } else {
            if (httpResponse.statusCode != 200) {
                NSLog(@"HTTP request failed with code: %d", httpResponse.statusCode);
                retryCount++;
            } else {
                break;
            }
        }
    }
}


@end
