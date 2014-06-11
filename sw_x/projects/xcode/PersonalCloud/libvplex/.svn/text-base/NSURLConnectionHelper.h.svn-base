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

#import <Foundation/Foundation.h>
#import <strings.h>
#include "vplex__http2.hpp"
#import "NSCallbackInputStream.h"

@interface NSURLConnectionHelper : NSObject
{
    NSCondition *waitForConnectionCondition;
    bool isFinished;
    bool isDebugging;
}

@property (nonatomic, assign) bool isFinished;
@property (nonatomic, assign) bool isDebugging;
@property (nonatomic, assign) int errorReturnCode;

@property (nonatomic, assign) NSInteger timeoutInterval;
@property (strong, nonatomic) NSString *connectionURIString;
@property (strong, nonatomic) NSString *connectionMethod;
@property (strong, nonatomic) NSData *connectionBody;

@property (strong, nonatomic) NSMutableData *responseData;

- (void)setConnectionURIFromCString:(const char *)newConnectionURI;
- (void)addRequestHeaderWithField:(const char *)field Value:(const char *)value;

- (void)setConnectionMethodFromCString:(const char *)specifiedConnectionMethod;
- (void)setConnectionBodyFromData:(const void *)newConnectionBody DataLength:(unsigned int)dataLength;
- (void)setContentFilePathFromFilePath:(const char *)filePath;
- (void)setReceiveFilePathFromFilePath:(const char *)filePath;
- (void)setCallbackInputStream:(NSCallbackInputStream *)inputStream SendSize:(u64) size;

- (void)registerReceiveCallback:(VPLHttp2_RecvCb)callback Interface:(VPLHttp2 *)receiverInterface Context:(void *)receiverCbCtx;
- (void)registerReceiveProgressCallback:(VPLHttp2_ProgressCb)callback Interface:(VPLHttp2 *)receiverInterface Context:(void *)receiverCbCtx;
- (void)registerSendProgressCallback:(VPLHttp2_ProgressCb)callback Interface:(VPLHttp2 *)receiverInterface Context:(void *)receiverCbCtx;

- (void)startConnection:(NSCondition *)condition;

- (int)getStatusCode;
- (const char *)findHeader:(NSString *) headerField;
- (void)cancelConnection;

@end
