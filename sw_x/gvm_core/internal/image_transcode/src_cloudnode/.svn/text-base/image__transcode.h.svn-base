#ifndef __IMAGE__TRANSCODE__
#define __IMAGE__TRANSCODE__

#include <vpl_types.h>
#include <vplu_types.h>
#include <vpl_error.h>
#include <string>

/// The initialization functions depends on the used library.
int ImageTranscode_init_private();

/// The implementation of ImageTranscode_Transcode(), and it's platform dependent.
/// param[IN] filepath: The path of photo to be transcoded.
/// param[IN] type: The image type of output
/// param[IN] width: The width of output image
/// param[IN] height: The height of output image
/// param[OUT] out_image: Binary of output image. (Need to call 'delete' on after use.)
/// param[OUT] out_image_len: Binary length of of output image.
int ImageTranscode_transcode_private(std::string filepath,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len);

/// The implementation of ImageTranscode_Transcode(), and it's platform dependent.
/// param[IN] image: The binary of original photo.
/// param[IN] image_len: The length of original photo.
/// param[IN] type: The image type of output
/// param[IN] width: The width of output image
/// param[IN] height: The height of output image
/// param[OUT] out_image: Binary of output image. (Need to call 'delete' on after use.)
/// param[OUT] out_image_len: Binary length of of output image.
int ImageTranscode_transcode_private(const unsigned char* image,
                                     size_t image_len,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len);

/// The shutdown functions depends on the used library.
int ImageTranscode_shutdown_private();

#endif
