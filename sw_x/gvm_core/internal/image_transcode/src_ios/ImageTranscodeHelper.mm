#import "ImageTranscodeHelper.h"

#include <vplu_format.h>

#import <UIKit/UIKit.h>
#include <log.h>
#include <string>

@implementation ImageTranscodeHelper

+ (ImageTranscode_ImageType)guessImageType:(const char*)path {
    uint8_t c;
    NSData* data = [[NSFileManager defaultManager] contentsAtPath: [NSString stringWithUTF8String:path]];

    [data getBytes:&c length:1];
    switch (c) {
        case 0xFF:
            return ImageType_JPG;
        case 0x89:
            return ImageType_PNG;
        case 0x42:
            return ImageType_BMP;
        case 0x49:
        case 0x4D:
            return ImageType_TIFF;
        case 0x47:
            return ImageType_GIF;
    }
    return ImageType_Original;
}

+ (NSData *)resizeImage:(const char*)path
                toWidth:(unsigned int)width
              andHeight:(unsigned int)height
               toFormat:(int)type
{
    NSString* str_path;
    UIImage* image;
    UIImage* new_image;
    CGSize size;

    str_path = [NSString stringWithCString:path encoding:NSUTF8StringEncoding];
    if (str_path == Nil) {
        LOG_ERROR("Failed to create NSString.");
        return Nil;
    }

    image = [UIImage imageWithContentsOfFile:str_path];
    if (image == Nil) {
        LOG_ERROR("Failed to open such image: %s", path);
        return Nil;
    }

    {
        CGSize org_size = [image size];
        size_t org_width = 0;
        size_t org_height = 0;
        double r1;
        double r2;

        org_size = [image size];
        org_width = org_size.width;
        org_height = org_size.height;

        if (org_width == 0 || org_height == 0) {
            LOG_ERROR("Failed to get original image size, org_width = "FMTu_size_t", org_height = "FMTu_size_t"", org_width, org_height);
            return Nil;
        }

        if (width >= org_width && height >= org_height) {
            size = CGSizeMake(org_width, org_height);

        } else {
            r1 = (double)org_height / org_width;
            r2 = (double)height / width;

            // org_width > org_height
            if (r1 < 1) {
                if (r1 <= r2) {
                    size = CGSizeMake(width, width * r1);
                } else {
                    size = CGSizeMake(height / r1, height);
                }

                // org_width <= org_height
            } else {
                if (r1 >= r2) {
                    size = CGSizeMake(height / r1, height);
                } else {
                    size = CGSizeMake(width, width * r1);
                }
            }
        }
    }

    UIGraphicsBeginImageContextWithOptions(CGSizeMake(size.width,
                                                      size.height), NO, 0);
    [image drawInRect:CGRectMake(0.0, 0.0, size.width, size.height)];
    new_image = UIGraphicsGetImageFromCurrentImageContext();
    if (new_image == Nil) {
        LOG_ERROR("Failed to get image from context.");
        return Nil;
    }

    UIGraphicsEndImageContext();

    switch(type){
        case ImageType_JPG:
            return UIImageJPEGRepresentation(new_image, 1.0);

        case ImageType_PNG:
            return UIImagePNGRepresentation(new_image);

        case ImageType_TIFF:
            LOG_ERROR("iOS does not support such TIFF.");
            break;

        case ImageType_BMP:
            LOG_ERROR("iOS does not support such BMP.");
            break;

        case ImageType_GIF:
            LOG_ERROR("iOS does not support such GIF.");
            break;

        case ImageType_Original:
        default:
            LOG_ERROR("Unknown type - %d", type);
            break;
    }

    return Nil;
}

@end
