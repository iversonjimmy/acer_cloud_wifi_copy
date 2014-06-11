//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#import "NSCallbackInputStream.h"

#import <objc/runtime.h>

@implementation NSCallbackInputStream
{
    NSStreamStatus streamStatus;
    VPLHttp2_SendCb sendCb;
    void *sendCbCtx;
    VPLHttp2 *httpInterface;
    BOOL bytesAvailable;
    
    
    id <NSStreamDelegate> delegate;
    
   	CFReadStreamClientCallBack copiedCallback;
    CFStreamClientContext copiedContext;
    CFOptionFlags requestedEvents;
}

// We want to rewrite the _scheduleInCFRunLoop, _unscheduleFromCFRunLoop and _setCFClientFlags but don't want to directly access the undocumented functions.
// Using resolveInstanceMethod to redirect the function calls to the functions that we created(which is the same function name without the underscope).
// This can help to let the NSURLRequest to successfully using this stream object for the setHTTPBodyStream.
+ (BOOL) resolveInstanceMethod:(SEL) selector
{
    NSString * name = NSStringFromSelector(selector);
    
    if ( [name hasPrefix:@"_"] )
    {
        name = [name substringFromIndex:1];
        SEL aSelector = NSSelectorFromString(name);
        Method method = class_getInstanceMethod(self, aSelector);
        
        if ( method )
        {
            class_addMethod(self,
                            selector,
                            method_getImplementation(method),
                            method_getTypeEncoding(method));
            return YES;
        }
    }
    return [super resolveInstanceMethod:selector];
}

- (id <NSStreamDelegate> )delegate {
    return delegate;
}

- (void)setDelegate:(id<NSStreamDelegate>)aDelegate {
    if (aDelegate == nil) {
        delegate = self;
    }
    else {
        delegate = aDelegate;
    }
}

- (id)initWithHttp2SendCb:(VPLHttp2_SendCb)http2SendCb SendCtx:(void *)sendCtx VPLHttp2Interface:(VPLHttp2 *)interface;
{
    self = [super init];
    if (self) {
        // Initialization code here.
        streamStatus = NSStreamStatusNotOpen;
        sendCb = http2SendCb;
        sendCbCtx = sendCtx;
        httpInterface = interface;
        bytesAvailable = YES;
        
        [self setDelegate:self];
    }
    
    return self;
}

#pragma mark - NSStream subclass overrides

- (void)open {
    streamStatus = NSStreamStatusOpen;
}

- (void)close {
    streamStatus = NSStreamStatusClosed;
}

- (void) scheduleInCFRunLoop:(CFRunLoopRef) runLoop
                     forMode:(CFStringRef) mode
{
    // Don't need to schedule in a runloop
}

- (void) unscheduleFromCFRunLoop:(CFRunLoopRef) runLoop
                         forMode:(CFStringRef) mode
{
    // Don't need to schedule in a runloop
}

- (void) scheduleInRunLoop:(NSRunLoop *) aRunLoop
                   forMode:(NSString *) mode
{
    [self scheduleInRunLoop:aRunLoop forMode:mode];
}

- (void) removeFromRunLoop:(NSRunLoop *) aRunLoop
                   forMode:(NSString *) mode
{
    [self removeFromRunLoop:aRunLoop forMode:mode];
}

- (NSStreamStatus)streamStatus {
    return streamStatus;
}

- (NSError *)streamError {
    return nil;
}


#pragma mark - NSInputStream subclass overrides

- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len {
    int receivedSize = 0;
    if (sendCb) {
        receivedSize = sendCb(httpInterface, sendCbCtx, (char*)buffer, len);
    }

    if (receivedSize > 0) {
        bytesAvailable = YES;
    } else {
        bytesAvailable = NO;
    }
    
    if (CFReadStreamGetStatus((CFReadStreamRef)self) == kCFStreamStatusOpen) {
        double delayInSeconds = 0;
        dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, delayInSeconds * NSEC_PER_SEC);
        dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
            if (copiedCallback && (requestedEvents & kCFStreamEventHasBytesAvailable)) {
                copiedCallback((__bridge CFReadStreamRef)self, kCFStreamEventHasBytesAvailable, &copiedContext);
            }
        });
    }
    
    return receivedSize;
}

- (BOOL)getBuffer:(uint8_t **)buffer length:(NSUInteger *)len {
    // No buffer for retrieving. return NO.
    return NO;
}

- (BOOL)hasBytesAvailable {
    return bytesAvailable;
}

- (BOOL) setCFClientFlags:(CFOptionFlags)inFlags
                 callback:(CFReadStreamClientCallBack)inCallback
                  context:(CFStreamClientContext *)inContext
{
	if (inCallback != NULL) {
        requestedEvents = inFlags;
        copiedCallback = inCallback;
        memcpy(&copiedContext, inContext, sizeof(CFStreamClientContext));
        
        if (copiedContext.info && copiedContext.retain) {
            copiedContext.retain(copiedContext.info);
        }
    }
    else {
        requestedEvents = kCFStreamEventNone;
        copiedCallback = NULL;
        if (copiedContext.info && copiedContext.release) {
            copiedContext.release(copiedContext.info);
        }
        
        memset(&copiedContext, 0, sizeof(CFStreamClientContext));
    }
    
    return YES;
}

@end
