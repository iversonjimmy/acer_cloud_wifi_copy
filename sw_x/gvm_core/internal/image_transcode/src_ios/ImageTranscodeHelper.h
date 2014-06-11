#import <Foundation/Foundation.h>

#include "image_transcode.h"

@interface ImageTranscodeHelper : NSObject

+ (ImageTranscode_ImageType)guessImageType:(const char*)path;

+ (NSData *)resizeImage:(const char*)path
                toWidth:(unsigned int)width
              andHeight:(unsigned int)height
               toFormat:(int)type;
@end
