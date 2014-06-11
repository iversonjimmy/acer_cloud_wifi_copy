#include <vpl_plat.h>
#include <vplex_time.h>
#include <string>
#include <log.h>
#include <stdlib.h>
#include <string>
#include <vpl_error.h>

#include <exif_util.h>
#include <exif_parser.h>
#include <time.h>

#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

NSString* searchDateTime(NSDictionary* metadata)
{
    NSDictionary* exif_dictionary = [metadata objectForKey:(NSString *)kCGImagePropertyExifDictionary];
    NSString* datetime = [exif_dictionary objectForKey:(NSString*)kCGImagePropertyExifDateTimeDigitized];
    if (datetime != Nil) {
        return datetime;
    }
    datetime = [exif_dictionary objectForKey:(NSString*)kCGImagePropertyExifDateTimeOriginal];
    if (datetime != Nil) {
        return datetime;
    }
    // No DateTime in EXIF, seach in TIFF instead
    NSDictionary* tiff_dictionary = [metadata objectForKey:(NSString *)kCGImagePropertyTIFFDictionary];
    datetime = [tiff_dictionary objectForKey:(NSString*)kCGImagePropertyTIFFDateTime];
    
    return datetime;
}

int EXIFGetImageTimestamp(const std::string& filename,
                          VPLTime_CalendarTime& dateTime,
                          VPLTime_t& timestamp)
{
    int rv = VPL_ERR_FAIL;
    NSString* filepath = [[NSString alloc] initWithBytes:filename.c_str() length:filename.size() encoding:[NSString defaultCStringEncoding]];
    NSURL* file_url = [NSURL fileURLWithPath:filepath];
    CGImageSourceRef image_source_ref = CGImageSourceCreateWithURL((__bridge CFURLRef)file_url, NULL);
    NSDictionary* metadata = (__bridge NSDictionary *) CGImageSourceCopyPropertiesAtIndex(image_source_ref, 0, NULL);
    NSString* datetime = searchDateTime(metadata);
    if (datetime != Nil) {
        const char* c_datetime = [datetime UTF8String];
        NSRegularExpression* regular_expression = [NSRegularExpression regularExpressionWithPattern:@"(\\d{1,}):(\\d{1,}):(\\d{1,}) (\\d{1,}):(\\d{1,}):(\\d{1,})"
                                                                                            options:NSRegularExpressionCaseInsensitive
                                                                                              error:Nil];
        NSRange range_of_first_match = [regular_expression rangeOfFirstMatchInString:datetime
                                                                          options:0
                                                                            range:NSMakeRange(0, [datetime length])];

        if (range_of_first_match.location == NSNotFound) {
            LOG_WARN("Got unexpected datetime - %s", c_datetime);
        }

        sscanf(c_datetime, "%u:%u:%u %u:%u:%u", &dateTime.year, &dateTime.month, &dateTime.day, &dateTime.hour, &dateTime.min, &dateTime.sec);
        dateTime.msec = 0;
        dateTime.usec = 0;

        {
            struct tm temp;
            temp.tm_year = dateTime.year - 1900;
            temp.tm_mon = dateTime.month - 1;
            temp.tm_mday = dateTime.day;
            temp.tm_hour = dateTime.hour;
            temp.tm_min = dateTime.min;
            temp.tm_sec = dateTime.sec;
            // Since the timestamp will be calculated back to local time, we can ignore the daylight saving time.
            temp.tm_isdst = 0;
            // The mktime() creates an UTC timestamp, recalculate it back to local time.
            // Since PicStream sends the timestamp to server without timezone data, the timestamp being created here should be the current local time not UTC time.
            timestamp = (VPLTime_t) (mktime(&temp) - timezone);
        }

        rv = VPL_OK;
    } else {
        LOG_WARN("No date time in exif info - %s", filename.c_str());
    }

    return rv;
}
