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

#import <Foundation/Foundation.h>

#include "vplex__http2.hpp"

@interface NSCallbackInputStream : NSInputStream <NSStreamDelegate>

- (id)initWithHttp2SendCb:(VPLHttp2_SendCb)http2SendCb SendCtx:(void *)sendCtx VPLHttp2Interface:(VPLHttp2 *)interface;

@end
