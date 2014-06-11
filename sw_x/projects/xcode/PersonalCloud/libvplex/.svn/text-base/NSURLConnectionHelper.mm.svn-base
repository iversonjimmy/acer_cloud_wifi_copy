//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#import "NSURLConnectionHelper.h"

#include "vpl_th.h"
#include "vplu_mutex_autolock.hpp"
#include "vplex_private.h"

// Private
@interface NSURLConnectionHelper() {
    int responseCode;
    NSMutableDictionary *headerContent;
    s64 expectedLength;
    u64 progress;
    u64 sendSize;

    VPLHttp2_RecvCb recvCb;
    void *recvCtx;
    VPLHttp2_ProgressCb recvProgCb;
    void *recvProgCtx;
    VPLHttp2 *vplHttp2Interface;
    VPLMutex_t mutex;

    VPLHttp2_ProgressCb sendProgCb;
    void *sendProgCtx;
}

@property (nonatomic, strong, readwrite) NSURLConnection* connection;
@property (nonatomic, strong, readwrite) NSMutableURLRequest* mutableURLRequest;
@property (strong, nonatomic) NSString *contentFilePath;
@property (strong, nonatomic) NSString *receiveFilePath;
@property (strong, nonatomic) NSCallbackInputStream *callbackInputStream;

- (void)endConnection;
- (NSString*) reformatDictionaryString:(NSDictionary*) inputDictionary;

@end

@implementation NSURLConnectionHelper

@synthesize isFinished, isDebugging, connectionURIString, connectionMethod, connectionBody, callbackInputStream, contentFilePath, receiveFilePath, connection, mutableURLRequest, timeoutInterval;
@synthesize responseData;

- (id) init
{
    self = [super init];
    
    if (self) {
        self.isFinished = NO;
        self.mutableURLRequest = [[NSMutableURLRequest alloc] init];
        self.timeoutInterval = 60;
        self.connectionBody = nil;
        self.callbackInputStream = nil;
        self.contentFilePath = nil;
        self.errorReturnCode = VPL_OK;
        self.responseData = [[NSMutableData alloc] init];
        VPLMutex_Init(&mutex);
    }
    
    return self;
}

- (void) dealloc
{
    VPLMutex_Destroy(&mutex);
}

// Set whole URI line.
- (void)setConnectionURIFromCString:(const char *)newConnectionURI
{
    self.connectionURIString = [NSString stringWithCString:newConnectionURI encoding:NSASCIIStringEncoding];
}

// Add a request header.
- (void)addRequestHeaderWithField:(const char *)field Value:(const char *)value
{
    NSString *valueString = [NSString stringWithCString:value encoding:NSASCIIStringEncoding];
    NSString *fieldString = [NSString stringWithCString:field encoding:NSASCIIStringEncoding];
    [self.mutableURLRequest setValue:valueString forHTTPHeaderField:fieldString];
}

// Setup request
- (void)setConnectionMethodFromCString:(const char *)specifiedConnectionMethod
{
    self.connectionMethod = [NSString stringWithCString:specifiedConnectionMethod encoding:NSASCIIStringEncoding];;
}

- (void)setConnectionBodyFromData:(const void *)newConnectionBody DataLength:(unsigned int)dataLength
{
    self.connectionBody = [NSData dataWithBytes:newConnectionBody length:dataLength];
}

- (void)setContentFilePathFromFilePath:(const char *)filePath
{
    self.contentFilePath = [NSString stringWithCString:filePath encoding:NSUTF8StringEncoding];
}

- (void)setReceiveFilePathFromFilePath:(const char *)filePath
{
    self.receiveFilePath = [NSString stringWithCString:filePath encoding:NSUTF8StringEncoding];
}

- (void)setCallbackInputStream:(NSCallbackInputStream *)inputStream SendSize:(u64) size
{
    self.callbackInputStream = inputStream;
    sendSize = size;
}

// Register callback functions
- (void)registerReceiveCallback:(VPLHttp2_RecvCb)callback Interface:(VPLHttp2 *)receiverInterface Context:(void *)receiverCbCtx
{
    recvCb = callback;
    recvCtx = receiverCbCtx;
    vplHttp2Interface = receiverInterface;
}

- (void)registerReceiveProgressCallback:(VPLHttp2_ProgressCb)callback Interface:(VPLHttp2 *)receiverInterface Context:(void *)receiverCbCtx
{
    recvProgCb = callback;
    recvProgCtx = receiverCbCtx;
    vplHttp2Interface = receiverInterface;
}

- (void)registerSendProgressCallback:(VPLHttp2_ProgressCb)callback Interface:(VPLHttp2 *)receiverInterface Context:(void *)receiverCbCtx
{
    sendProgCb = callback;
    sendProgCtx = receiverCbCtx;
    vplHttp2Interface = receiverInterface;
}

// Get HTTP status code.
- (int)getStatusCode
{
    return responseCode;
}

// Find response header value.
- (const char *)findHeader:(NSString *) headerField
{
    if (!headerField) {
        return nil;
    }
    return [[headerContent valueForKey:[headerField lowercaseString]] cStringUsingEncoding:[NSString defaultCStringEncoding]];
}

// Abort transfer.
- (void)cancelConnection
{
    if (self.connection) {
        [self.connection cancel];
        self.errorReturnCode = VPL_ERR_CANCELED;
        [self endConnection];
    }
}

// Start the connection
- (void)startConnection:(NSCondition *)condition
{
    waitForConnectionCondition = condition;
    
    [self.mutableURLRequest setURL:[NSURL URLWithString:self.connectionURIString]];
    
    [self.mutableURLRequest setHTTPMethod:self.connectionMethod];
    
    [self.mutableURLRequest setTimeoutInterval: self.timeoutInterval];
    // Reformat and log the header
    if (self.isDebugging) {
        NSString *headerString = [self reformatDictionaryString:[self.mutableURLRequest allHTTPHeaderFields]];
        VPL_LogHttpBuffer("send header", [headerString cStringUsingEncoding:[NSString defaultCStringEncoding]], [headerString length]);
    }
    
    if (self.callbackInputStream != nil) {
        if ([self.connectionMethod isEqualToString:@"GET"] || [self.connectionMethod isEqualToString:@"DELETE"]) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Requesting %s with callbackInputStream being set", [self.connectionMethod cStringUsingEncoding:NSUTF8StringEncoding]);
        } else {
            VPL_LIB_LOG_INFO(VPL_SG_HTTP, "send data from callbackInputStream, sendSize:%llu", sendSize);
            [self.mutableURLRequest addValue:[NSString stringWithFormat:@"%llu", sendSize] forHTTPHeaderField:@"Content-Length"];
            [self.mutableURLRequest setHTTPBodyStream:self.callbackInputStream];
        }
    } else if (self.connectionBody != nil) {
        if ([self.connectionMethod isEqualToString:@"GET"] || [self.connectionMethod isEqualToString:@"DELETE"]) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Requesting %s with connectionBody being set", [self.connectionMethod cStringUsingEncoding:NSUTF8StringEncoding]);
        } else {
            if (self.isDebugging) {
                VPL_LogHttpBuffer("send body", [self.connectionBody bytes], [self.connectionBody length]);
            }
            [self.mutableURLRequest setHTTPBody:self.connectionBody];
        }
    } else if (self.contentFilePath != nil) {
        if ([self.connectionMethod isEqualToString:@"GET"] || [self.connectionMethod isEqualToString:@"DELETE"]) {
            VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Requesting %s with contentFilePath being set", [self.connectionMethod cStringUsingEncoding:NSUTF8StringEncoding]);
        } else {
            NSInputStream *stream = [[NSInputStream alloc] initWithFileAtPath:self.contentFilePath];
            NSError *error = nil;
            NSDictionary *fileAttributes = [[NSFileManager defaultManager] attributesOfItemAtPath:self.contentFilePath error:&error];
            if (error) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "Error on accessing file: %s Error(%d): %s", [self.contentFilePath cStringUsingEncoding:[NSString defaultCStringEncoding]], [error code], [[error localizedDescription] UTF8String]);
                self.errorReturnCode = VPL_ERR_BADF;
                [self endConnection];
                return;
            }
            
            NSNumber *fileSizeNumber = [fileAttributes objectForKey:NSFileSize];
            if (self.isDebugging) {
                VPL_LIB_LOG_INFO(VPL_SG_HTTP, "send file path: %s", [self.contentFilePath cStringUsingEncoding:[NSString defaultCStringEncoding]]);
            }
            [self.mutableURLRequest addValue: [fileSizeNumber stringValue] forHTTPHeaderField:@"Content-Length"];
            [self.mutableURLRequest setHTTPBodyStream:stream];
        }
    }
    
    self.connection = [[NSURLConnection alloc] initWithRequest:self.mutableURLRequest delegate:self startImmediately:NO];
    
    // keep in a runloop and wait for NSURLConnection delegate events
    [self.connection scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [self.connection start];

    [[NSRunLoop currentRunLoop]  runUntilDate:[NSDate distantFuture]];
}

//------------ NSURLConnection delegate
- (void)connection:(NSURLConnection *)theConnection didReceiveResponse:(NSURLResponse *)response
{
    NSHTTPURLResponse * httpResponse;
    if(theConnection != self.connection){
        VPL_LIB_LOG_ERR(VPL_SG_HTTP, "theConnection != self.connection");
    } else {
        NSDictionary *headers;
        NSEnumerator *headers_enumerator;
        id header_field;
        
        httpResponse = (NSHTTPURLResponse *) response;
        assert( [httpResponse isKindOfClass:[NSHTTPURLResponse class]] );
        
        responseCode = httpResponse.statusCode;
        VPL_LIB_LOG_INFO(VPL_SG_HTTP, "@Response status code: %d", httpResponse.statusCode);

        // Make query headers case insensitive.
        headers = [httpResponse allHeaderFields];
        // Reformat and log the header
        if (self.isDebugging) {
            NSString *headerString = [self reformatDictionaryString:headers];
            VPL_LogHttpBuffer("receive header", [headerString cStringUsingEncoding:[NSString defaultCStringEncoding]], [headerString length]);
        }
        
        headers_enumerator = [headers keyEnumerator];
        // As per the documentation, didReceiveResponse can be called multiple times.  We rely on
        // ARC to release the previous instance.
        headerContent = [[NSMutableDictionary alloc] init];
        while ((header_field = [headers_enumerator nextObject])) {
            [headerContent setObject:[headers valueForKey:header_field]
                              forKey:[header_field lowercaseString]];
        }

        expectedLength = -1;

        // To get expectedLength from Http Header - 'Content-Length'.
        // Note that this can be different than NSURLResponse.expectedContentLength in some cases;
        // for more information, see http://bugs.ctbg.acer.com/show_bug.cgi?id=8331#c24
        if ([headerContent objectForKey: @"content-length"] != nil) {
            NSString* header_value = [headerContent objectForKey: @"content-length"];
            s64 content_length = [header_value longLongValue];
            if (content_length != LLONG_MAX && content_length >= 0) {
                VPL_LIB_LOG_INFO(VPL_SG_HTTP, "@Got expectedLength from Content-Length: %lld", content_length);
                expectedLength = content_length;
            } else {
                VPL_LIB_LOG_WARN(VPL_SG_HTTP, "@Got unexpected Content-Length in header: %s", [header_value UTF8String]);
            }
        }

        // To get expectedLength from NSURLResponse.expectedContentLength if there is no Http Header - 'Content-Length'.
        if (expectedLength == -1) {
            VPL_LIB_LOG_INFO(VPL_SG_HTTP, "@Got expectedLength from expectedContentLength: %lld", httpResponse.expectedContentLength);
            expectedLength = httpResponse.expectedContentLength;
        }
        
        // Prepare the destination file.
        // There will still be an empty file when the request didn't received any data.
        if (self.receiveFilePath != nil) {
            // Ensure there is an empty destination file for data writing.
            NSFileManager *fileMgr = [NSFileManager defaultManager];
            if ([fileMgr fileExistsAtPath:self.receiveFilePath]) {
                [fileMgr removeItemAtPath:self.receiveFilePath error:nil];
            }
            [fileMgr createFileAtPath:self.receiveFilePath contents:[NSData data] attributes:nil];
        }

        // As per the documentation, we should reset progress each time this is called.
        progress = 0;
    };
}

- (void)connection:(NSURLConnection *)theConnection didFailWithError:(NSError *)error
{
    VPL_LIB_LOG_INFO(VPL_SG_HTTP, "Connection failed with error(%d): %s", [error code], [[error localizedDescription] UTF8String]);
    switch ([error code]) {
        case NSURLErrorTimedOut:
            self.errorReturnCode = VPL_ERR_TIMEOUT;
            break;
        case NSURLErrorCannotFindHost:
            self.errorReturnCode = VPL_ERR_UNREACH;
            break;
        case NSURLErrorCannotConnectToHost:
            self.errorReturnCode = VPL_ERR_CONNREFUSED;
            break;
        case NSURLErrorCannotWriteToFile || NSURLErrorDownloadDecodingFailedMidStream || NSURLErrorDownloadDecodingFailedToComplete:
            self.errorReturnCode = VPL_ERR_IO;
            break;
        case NSURLErrorServerCertificateHasBadDate:
            self.errorReturnCode = VPL_ERR_SSL_DATE_INVALID;
            break;
        case NSURLErrorServerCertificateUntrusted:
            self.errorReturnCode = VPL_ERR_SSL;
            break;
        default:
            self.errorReturnCode = VPL_ERR_HTTP_ENGINE;
            break;
    }
    [self endConnection];
}

- (void)connection:(NSURLConnection *)theConnection didReceiveData:(NSData *)data
{
    progress += [data length];
    if (self.isDebugging) {
        VPL_LogHttpBuffer("receive body", [data bytes], [data length]);
    }

    if (recvProgCb) {
        recvProgCb(vplHttp2Interface, recvProgCtx, expectedLength, progress);
    }

    if (recvCb) {
        @autoreleasepool {
            s32 recvCbRv = recvCb(vplHttp2Interface, recvCtx, (const char*)[data bytes], [data length]);
            if (recvCbRv != [data length]) {
                [self cancelConnection];
                self.errorReturnCode = VPL_ERR_IN_RECV_CALLBACK;
            }
        }
    } else {
        if (self.receiveFilePath != nil) {
            //Directly append the received data to file.
            NSFileHandle *receiveFileHandle = [NSFileHandle fileHandleForUpdatingAtPath:self.receiveFilePath];

            if (!receiveFileHandle) {
                VPL_LIB_LOG_ERR(VPL_SG_HTTP, "@Could not write to file : %s", [self.receiveFilePath UTF8String]);
                self.errorReturnCode = VPL_ERR_BADF;
                return;
            }
            
            [receiveFileHandle seekToEndOfFile];
            [receiveFileHandle writeData:data];
            
            [receiveFileHandle closeFile];
        } else {
            [self.responseData appendData:data];
        }
    }
}

- (void)connectionDidFinishLoading:(NSURLConnection *)theConnection
{
    if (expectedLength >= 0) {
        if (progress < expectedLength) {
            VPL_LIB_LOG_WARN(VPL_SG_HTTP, "Unexpected Truncation, Content-Length: %lld, received: %lld", expectedLength, progress);
            self.errorReturnCode = VPL_ERR_RESPONSE_TRUNCATED;
        } else if (progress > expectedLength) {
            VPL_LIB_LOG_WARN(VPL_SG_HTTP, "Received more data than expected, Content-Length: %lld, received: %lld", expectedLength, progress);
            self.errorReturnCode = VPL_ERR_INVALID_SERVER_RESPONSE;
        }
    }
    [self endConnection];
}

- (void)connection:(NSURLConnection *)theConnection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite
{
    if (sendProgCb) {
        sendProgCb(vplHttp2Interface, sendProgCtx, totalBytesExpectedToWrite, totalBytesWritten);
    }
}

- (void)endConnection
{
    MutexAutoLock lock(&mutex);
    [waitForConnectionCondition lock];
    if (!self.isFinished) {
        self.contentFilePath = nil;
        self.receiveFilePath = nil;
        self.isFinished = YES;
        [waitForConnectionCondition signal];
        CFRunLoopStop(CFRunLoopGetCurrent());
    }
    [waitForConnectionCondition unlock];
}

- (NSString*) reformatDictionaryString:(NSDictionary*) inputDictionary
{
    NSMutableString *outputString = [[NSMutableString alloc] init];
    for (NSString *key in inputDictionary) {
        [outputString appendString:[NSString stringWithFormat:@"%@: %@\n", key, [inputDictionary objectForKey:key]]];
    }
    return outputString;
}

@end
