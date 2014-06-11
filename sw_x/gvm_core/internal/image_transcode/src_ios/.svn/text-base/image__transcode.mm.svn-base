#include "image_transcode.h"
#include "image__transcode.h"

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vpl_types.h>
#include <vplu_format.h>
#include <vpl_time.h>
#include <vpl_fs.h>
#include <vplex_file.h>
#include <log.h>

#include "ImageTranscodeHelper.h"

int ImageTranscode_init_private()
{
    return IMAGE_TRANSCODING_OK;
}

int ImageTranscode_transcode_private(std::string filepath,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    int rv = IMAGE_TRANSCODING_OK;
    NSData* image_data;
    unsigned char* data = NULL;

    if (type == ImageType_Original) {
        ImageTranscode_ImageType org_image_type = [ImageTranscodeHelper guessImageType:filepath.c_str()];
        if (org_image_type == ImageType_Original) {
            // Unknown image type.
            return IMAGE_TRANSCODING_FAILED;
        } else {
            type = org_image_type;
        }
    }
    image_data = [ImageTranscodeHelper resizeImage:filepath.c_str()
                                           toWidth:width
                                         andHeight:height
                                          toFormat:type];

    if (image_data == Nil) {
        return IMAGE_TRANSCODING_FAILED;
    }

    data = new unsigned char[[image_data length]];
    if (data == NULL) {
        return IMAGE_TRANSCODING_FAILED;
    }

    memcpy(data, [image_data bytes], [image_data length]);
    *out_image = data;
    out_image_len = [image_data length];

    return rv;
}

int ImageTranscode_transcode_private(const unsigned char* image,
                                     size_t image_len,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    // Deprecated. Not support by using ffmpeg.
    return IMAGE_TRANSCODING_FAILED;
}

int ImageTranscode_shutdown_private()
{
    return IMAGE_TRANSCODING_OK;
}
