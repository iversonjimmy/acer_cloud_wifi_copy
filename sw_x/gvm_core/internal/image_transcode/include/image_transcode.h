#ifndef __IMAGE_TRANSCODE__
#define __IMAGE_TRANSCODE__

#include <stddef.h> // for size_t
#include <vpl_types.h>
#include <vplu_types.h>

#define IMAGE_TRANSCODING_OK 0
#define IMAGE_TRANSCODING_NOT_INITIALIZED -1
#define IMAGE_TRANSCODING_ALREADY_INITIALIZED -2
#define IMAGE_TRANSCODING_BAD_PARAMETER -3
#define IMAGE_TRANSCODING_BAD_HANDLE -4
#define IMAGE_TRANSCODING_NOT_SUPPORT -5
#define IMAGE_TRANSCODING_FAILED -6

#define INVALID_IMAGE_TRANSCODE_HANDLE -1

enum ImageTranscode_ImageType {
    ImageType_JPG,
    ImageType_PNG,
    ImageType_TIFF,
    ImageType_BMP,
    ImageType_GIF,
    ImageType_Original
};

typedef u32 ImageTranscode_handle_t;

typedef struct t_TranscodingData {
    size_t width;
    size_t height;
    u32 type;
    unsigned char* image;
    size_t image_len;
} TranscodingData;


/// Callback for async operations
typedef void (*ImageTranscode_AsyncCallback)(ImageTranscode_handle_t handle, int return_code, void* args);

///// Callback for reading data from dataset (for cloudnode)
//typedef int (*ImageTranscode_ReadDataCallback)(void* ctx, void* buffer, size_t size, size_t offset);
//
///// Callback for reading data from dataset completed (for cloudnode)
//typedef int (*ImageTranscode_ReadDataCompletedCallback)(void* ctx);

/// Callback for reading data from dataset and write to a temp file (for cloudnode)
typedef int (*ImageTranscode_GenerateTempFileCallback)(void* ctx, const char* path);

/// To perform the platform-specific initialization that image transcoding requires.
/// @note This function should only be called once unless a matching ImageTranscode_Shutdown() be called.
/// @note This function is not thread safe. You must ensure that only one thread calls it.
/// @note Need to call ImageTranscode_Shutdown() to release the resources once this function be called.
/// @retval #IMAGE_TRANSCODING_OK Success.
/// @retval #IMAGE_TRANSCODING_ALREADY_INITIALIZED #ImageTranscode_Init() has already been invoked.
int ImageTranscode_Init();

/// Photo transcoding function
int ImageTranscode_Transcode(const char* filepath,
                             size_t filepath_len,
                             const unsigned char* image,
                             size_t image_len,
                             ImageTranscode_ImageType type,
                             size_t width,
                             size_t height,
                             ImageTranscode_handle_t* out_handle);

/// Photo async transcoding function
int ImageTranscode_AsyncTranscode(const char* filepath,
                                  size_t filepath_len,
                                  const unsigned char* image,
                                  size_t image_len,
                                  ImageTranscode_ImageType type,
                                  size_t width,
                                  size_t height,
                                  ImageTranscode_AsyncCallback callback,
                                  void* callback_args,
                                  ImageTranscode_handle_t* out_handle);

/// Photo async transcoding function - for cloudnode
int ImageTranscode_AsyncTranscode_cloudnode(ImageTranscode_GenerateTempFileCallback gen_tempfile_callback,
                                            void* gen_tempfile_callbackctx,
                                            const char* file_ext,
                                            size_t file_ext_len,
                                            ImageTranscode_ImageType type,
                                            size_t width,
                                            size_t height,
                                            ImageTranscode_AsyncCallback callback,
                                            void* callback_args,
                                            ImageTranscode_handle_t* out_handle);

/// To get the content length after transcoding.
int ImageTranscode_GetContentLength(ImageTranscode_handle_t handle, size_t& length);

/// To get the image size after transcoding.
int ImageTranscode_GetImageSize(ImageTranscode_handle_t handle, size_t& width, size_t& height);

/// To read the data content after transcoding.
int ImageTranscode_Read(ImageTranscode_handle_t handle, void* buf, size_t len, size_t offset);

/// To check the handle is valid or not.
bool ImageTranscode_IsValidHandle(ImageTranscode_handle_t handle);

/// To destroy the handle.
int ImageTranscode_DestroyHandle(ImageTranscode_handle_t handle);

/// To perform the platform-specific cleanup that image transcoding requires.
/// @note This call will shutdown the library and invalidate any currently valid handles.
/// @note This function is not thread safe. You must ensure that only one thread calls it.
/// @retval #IMAGE_TRANSCODING_OK Success.
/// @retval #IMAGE_TRANSCODING_NOT_INITIALIZED #ImageTranscode_Init() must be invoked first.
int ImageTranscode_Shutdown();

#endif
