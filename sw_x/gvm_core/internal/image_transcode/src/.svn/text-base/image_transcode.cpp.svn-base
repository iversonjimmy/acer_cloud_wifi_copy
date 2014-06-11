#include "image_transcode.h"
#include "image__transcode.h"
#include <vpl_types.h>
#include <vplu_types.h>
#include <vpl_error.h>
#include <vpl_th.h>
#include <vplex_file.h>
#include <vpl_fs.h>
#include <log.h>
#include <string>
#include <map>

static VPLTHREAD_FN_DECL transcode_function(void* param);

static bool initialized = false;
static ImageTranscode_handle_t next_handle = 0;
static VPLMutex_t next_handle_mutex;
static std::map<ImageTranscode_handle_t, TranscodingData> transcodingDatas;
static VPLMutex_t transcoding_data_mutex;


typedef struct t_TranscodeTicket {
    ImageTranscode_handle_t handle;
    std::string filepath;
    const unsigned char* image;
    size_t image_len;
    ImageTranscode_GenerateTempFileCallback gen_tempfile_callback;
    void* gen_tempfile_callback_ctx;
    std::string file_ext;
    ImageTranscode_ImageType type;
    size_t width;
    size_t height;
    ImageTranscode_AsyncCallback callback;
    void* callback_args;
} TranscodeTicket;

static std::string generate_temp_filename(const char* folder_path, const char* ext_name)
{
    std::string filename;
    char c_timestamp[64];
    VPLTime_t timestamp = VPLTime_GetTimeStamp();
    sprintf(c_timestamp, FMT_VPLTime_t, timestamp);

    filename.assign(folder_path);
    filename.append(c_timestamp);
    filename.append(".");
    filename.append(ext_name);
    return filename;
}

// To initial something in this function
int ImageTranscode_Init()
{
    int rv = IMAGE_TRANSCODING_OK;
    LOG_INFO("Initialize ImageTranscode");
    if (initialized) {
        rv = IMAGE_TRANSCODING_ALREADY_INITIALIZED;
    } else {
        rv = ImageTranscode_init_private();
        if (rv == IMAGE_TRANSCODING_OK) {
            initialized = true;
        }
    }

    VPLMutex_Init(&next_handle_mutex);
    VPLMutex_Init(&transcoding_data_mutex);

    if (rv == IMAGE_TRANSCODING_OK) {
        LOG_INFO("ImageTranscode is initialized.");
    } else if (rv == IMAGE_TRANSCODING_ALREADY_INITIALIZED) {
        LOG_WARN("ImageTranscode already initialized.");
    } else {
        LOG_ERROR("Failed to initialize ImageTranscode, rv = %d", rv);
    }

    return rv;
}

// Photo transcoding function
int ImageTranscode_Transcode(const char* filepath,
                             size_t filepath_len,
                             const unsigned char* image,
                             size_t image_len,
                             ImageTranscode_ImageType type,
                             size_t width,
                             size_t height,
                             ImageTranscode_handle_t* out_handle)
{
    int rv;
    unsigned char* out_image;
    size_t out_image_len;

    if (!initialized) {
        rv = ImageTranscode_init_private();
        if (rv != IMAGE_TRANSCODING_OK) {
            return rv;
        }
    }

    if (filepath != NULL && filepath_len > 0) {
        std::string path(filepath);
        rv = ImageTranscode_transcode_private(path,
                                              type,
                                              width,
                                              height,
                                              &out_image,
                                              out_image_len);

    } else if (image != NULL && image_len > 0) {
        rv = ImageTranscode_transcode_private(image,
                                              image_len,
                                              type,
                                              width,
                                              height,
                                              &out_image,
                                              out_image_len);

    } else {
        return VPL_ERR_INVALID;
    }

    if (rv == IMAGE_TRANSCODING_OK) {
        TranscodingData transcodingData;
        transcodingData.image_len = out_image_len;
        transcodingData.image = out_image;
        transcodingData.type = type;
        transcodingData.width = 0; // TODO: To get the width from transcoded image.
        transcodingData.height = 0; // TODO: To get the height from transcoded image.

        VPLMutex_Lock(&next_handle_mutex);
        *out_handle = next_handle++;
        VPLMutex_Unlock(&next_handle_mutex);

        VPLMutex_Lock(&transcoding_data_mutex);
        transcodingDatas[*out_handle] = transcodingData;
        VPLMutex_Unlock(&transcoding_data_mutex);
    } else {
        *out_handle = INVALID_IMAGE_TRANSCODE_HANDLE;
    }

    return rv;
}

// Photo async transcoding function
int ImageTranscode_AsyncTranscode(const char* filepath,
                                  size_t filepath_len,
                                  const unsigned char* image,
                                  size_t image_len,
                                  ImageTranscode_ImageType type,
                                  size_t width,
                                  size_t height,
                                  ImageTranscode_AsyncCallback callback,
                                  void* callback_args,
                                  ImageTranscode_handle_t* out_handle)
{
    int rv;
    TranscodeTicket* ticket = NULL;
    VPLThread_attr_t thread_attributes;
    VPLDetachableThreadHandle_t thread_handle;

    if (!initialized) {
        rv = ImageTranscode_init_private();
        if (rv != IMAGE_TRANSCODING_OK) {
            return rv;
        }
    }

    if (filepath != NULL && filepath_len > 0) {
        ticket = new TranscodeTicket();
        ticket->filepath = std::string(filepath);
        ticket->image = NULL;
        ticket->image_len = 0;
        ticket->gen_tempfile_callback = NULL;
        ticket->gen_tempfile_callback_ctx = NULL;
        ticket->type = type;
        ticket->width = width;
        ticket->height = height;
        ticket->callback = callback;
        ticket->callback_args = callback_args;

    } else if (image != NULL && image_len > 0) {
        ticket = new TranscodeTicket();
        ticket->image = image;
        ticket->image_len = image_len;
        ticket->gen_tempfile_callback = NULL;
        ticket->gen_tempfile_callback_ctx = NULL;
        ticket->type = type;
        ticket->width = width;
        ticket->height = height;
        ticket->callback = callback;
        ticket->callback_args = callback_args;

    } else {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    TranscodingData transcodingData;
    transcodingData.width = 0;
    transcodingData.height = 0;
    transcodingData.type = type;
    transcodingData.image = NULL;
    transcodingData.image_len = 0;

    VPLMutex_Lock(&next_handle_mutex);
    *out_handle = next_handle++;
    VPLMutex_Unlock(&next_handle_mutex);

    VPLMutex_Lock(&transcoding_data_mutex);
    // Record the transcoding data with handle
    transcodingDatas[*out_handle] = transcodingData;
    VPLMutex_Unlock(&transcoding_data_mutex);

    // Set the handle in transcoding ticket
    ticket->handle = *out_handle;

    rv = VPLThread_AttrInit(&thread_attributes);
    if (rv < 0) {
        LOG_ERROR("VPLThread_AttrInit returned %d", rv);
        goto out;
    }

    rv = VPLThread_AttrSetDetachState(&thread_attributes, VPL_TRUE);
    if (rv < 0) {
        LOG_ERROR("VPLThread_AttrSetDetachState returned %d", rv);
        goto out;
    }

    rv = VPLDetachableThread_Create(&thread_handle, transcode_function, ticket, &thread_attributes, NULL);
    if (rv < 0) {
        LOG_ERROR("VPLDetachableThread_Create returned %d", rv);
    }

 out:
    VPLThread_AttrDestroy(&thread_attributes);
 end:
    return rv;
}

int ImageTranscode_AsyncTranscode_cloudnode(ImageTranscode_GenerateTempFileCallback gen_tempfile_callback,
                                            void* gen_tempfile_callback_ctx,
                                            const char* file_ext,
                                            size_t file_ext_len,
                                            ImageTranscode_ImageType type,
                                            size_t width,
                                            size_t height,
                                            ImageTranscode_AsyncCallback callback,
                                            void* callback_args,
                                            ImageTranscode_handle_t* out_handle)
{
    int rv;
    TranscodeTicket* ticket = NULL;
    VPLThread_attr_t thread_attributes;
    VPLDetachableThreadHandle_t thread_handle;

    if (!initialized) {
        rv = ImageTranscode_init_private();
        if (rv != IMAGE_TRANSCODING_OK) {
            return rv;
        }
    }

    if (gen_tempfile_callback && file_ext != NULL && file_ext_len > 0) {
        ticket = new TranscodeTicket();
        ticket->image = NULL;
        ticket->image_len = 0;
        ticket->gen_tempfile_callback = gen_tempfile_callback;
        ticket->gen_tempfile_callback_ctx = gen_tempfile_callback_ctx;
        ticket->file_ext = std::string(file_ext);
        ticket->type = type;
        ticket->width = width;
        ticket->height = height;
        ticket->callback = callback;
        ticket->callback_args = callback_args;

    } else {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    TranscodingData transcodingData;
    transcodingData.width = 0;
    transcodingData.height = 0;
    transcodingData.type = type;
    transcodingData.image = NULL;
    transcodingData.image_len = 0;

    VPLMutex_Lock(&next_handle_mutex);
    *out_handle = next_handle++;
    VPLMutex_Unlock(&next_handle_mutex);

    VPLMutex_Lock(&transcoding_data_mutex);
    // Record the transcoding data with handle
    transcodingDatas[*out_handle] = transcodingData;
    VPLMutex_Unlock(&transcoding_data_mutex);

    // Set the handle in transcoding ticket
    ticket->handle = *out_handle;

    rv = VPLThread_AttrInit(&thread_attributes);
    if (rv < 0) {
        LOG_ERROR("VPLThread_AttrInit returned %d", rv);
        goto out;
    }

    rv = VPLThread_AttrSetDetachState(&thread_attributes, VPL_TRUE);
    if (rv < 0) {
        LOG_ERROR("VPLThread_AttrSetDetachState returned %d", rv);
        goto out;
    }

    rv = VPLDetachableThread_Create(&thread_handle, transcode_function, ticket, &thread_attributes, NULL);
    if (rv < 0) {
        LOG_ERROR("VPLDetachableThread_Create returned %d", rv);
    }

 out:
    VPLThread_AttrDestroy(&thread_attributes);
 end:
    return rv;
}

// To get the content length after transcoding.
int ImageTranscode_GetContentLength(ImageTranscode_handle_t handle, size_t& length)
{
    int rv = IMAGE_TRANSCODING_BAD_HANDLE;
    VPLMutex_Lock(&transcoding_data_mutex);
    std::map<ImageTranscode_handle_t, TranscodingData>::iterator it = transcodingDatas.find(handle);
    if (it != transcodingDatas.end()) {
        length = it->second.image_len;
        rv = IMAGE_TRANSCODING_OK;
    }

    VPLMutex_Unlock(&transcoding_data_mutex);
    return rv;
}

// To get the image size after transcoding.
int ImageTranscode_GetImageSize(ImageTranscode_handle_t handle, size_t& width, size_t& height)
{
    int rv = IMAGE_TRANSCODING_BAD_HANDLE;
    VPLMutex_Lock(&transcoding_data_mutex);
    std::map<ImageTranscode_handle_t, TranscodingData>::iterator it = transcodingDatas.find(handle);
    if (it != transcodingDatas.end()) {
        width = it->second.width;
        height = it->second.height;
        rv = IMAGE_TRANSCODING_OK;
    }

    VPLMutex_Unlock(&transcoding_data_mutex);
    return rv;
}

// To read the data content after transcoding.
int ImageTranscode_Read(ImageTranscode_handle_t handle, void* buf, size_t len, size_t offset)
{
    int rv = IMAGE_TRANSCODING_BAD_HANDLE;
    VPLMutex_Lock(&transcoding_data_mutex);
    std::map<ImageTranscode_handle_t, TranscodingData>::iterator it = transcodingDatas.find(handle);
    if (it != transcodingDatas.end()) {
        if ((offset + len) > it->second.image_len) {
            len = it->second.image_len - offset; // remain size
        }

        memcpy((void*)buf, it->second.image + offset, len);
        rv = len;
    }

    VPLMutex_Unlock(&transcoding_data_mutex);
    return rv;
}

// To check the handle is valid or not.
bool ImageTranscode_IsValidHandle(ImageTranscode_handle_t handle)
{
    bool is_valid = false;
    VPLMutex_Lock(&transcoding_data_mutex);
    std::map<ImageTranscode_handle_t, TranscodingData>::iterator it = transcodingDatas.find(handle);
    if (it != transcodingDatas.end()) {
        is_valid = true;
    }

    VPLMutex_Unlock(&transcoding_data_mutex);
    return is_valid;
}

// To destroy the handle.
int ImageTranscode_DestroyHandle(ImageTranscode_handle_t handle)
{
    int rv = IMAGE_TRANSCODING_BAD_HANDLE;
    VPLMutex_Lock(&transcoding_data_mutex);
    std::map<ImageTranscode_handle_t, TranscodingData>::iterator it = transcodingDatas.find(handle);
    if (it != transcodingDatas.end()) {
        if (it->second.image) {
            delete it->second.image;
        }
        transcodingDatas.erase(it);
        rv = IMAGE_TRANSCODING_OK;
    }

    VPLMutex_Unlock(&transcoding_data_mutex);
    return rv;
}

int ImageTranscode_Shutdown()
{
    int rv = IMAGE_TRANSCODING_OK;
    std::map<ImageTranscode_handle_t, TranscodingData>::iterator it;
    LOG_INFO("Shutdown ImageTranscode");
    if (initialized) {
        rv = ImageTranscode_shutdown_private();
        if (rv == IMAGE_TRANSCODING_OK) {
            initialized = false;
        }
    } else {
        rv = IMAGE_TRANSCODING_NOT_INITIALIZED;
    }

    VPLMutex_Lock(&transcoding_data_mutex);
    for (it = transcodingDatas.begin(); it != transcodingDatas.end(); it++) {
        if (it->second.image) {
            delete it->second.image;
        }
    }
    transcodingDatas.clear();

    if (rv == IMAGE_TRANSCODING_OK) {
        LOG_INFO("ImageTranscode is shutdown");
    } else if (rv == IMAGE_TRANSCODING_NOT_INITIALIZED) {
        LOG_WARN("ImageTranscode has not been initialized");
    } else {
        LOG_ERROR("Failed to shutdown ImageTranscode, rv = %d", rv);
    }

    VPLMutex_Unlock(&transcoding_data_mutex);
    VPLMutex_Destroy(&transcoding_data_mutex);
    VPLMutex_Destroy(&next_handle_mutex);
    return rv;
}

static VPLTHREAD_FN_DECL transcode_function(void* param)
{
    int rv = IMAGE_TRANSCODING_OK;
    unsigned char* out_image;
    size_t out_image_len;
    TranscodeTicket* ticket = (TranscodeTicket*)param;
    std::map<ImageTranscode_handle_t, TranscodingData>::iterator it;
    if (ticket->gen_tempfile_callback != NULL && ticket->file_ext.size() > 0) {
        std::string temp_source_filename;

        // To generate source image temp file path.
        {
            const char* home_path = getenv("HOME");
            temp_source_filename = generate_temp_filename(home_path, ticket->file_ext.c_str());
        }

        // Write into temp file
        VPLMutex_Lock(&transcoding_data_mutex);
        it = transcodingDatas.find(ticket->handle);
        if (it != transcodingDatas.end()) {
            rv = ticket->gen_tempfile_callback(ticket->gen_tempfile_callback_ctx, temp_source_filename.c_str());
        } else {
            LOG_WARN("Image transcoding handle(%d) has been destroyed.", ticket->handle);
            goto handle_unavailable;
        }
        VPLMutex_Unlock(&transcoding_data_mutex);

        if (rv != VPL_OK) {
            LOG_ERROR("Can not generate temp file \"%s\", rv = %d.", temp_source_filename.c_str(), rv);
            goto put_into_transcoding_data;;
        }

        // call ImageTranscode_transcode_private()
        rv = ImageTranscode_transcode_private(temp_source_filename,
                                              ticket->type,
                                              ticket->width,
                                              ticket->height,
                                              &out_image,
                                              out_image_len);

        // Delete source temp file
        {
            int rc = VPLFile_Delete(temp_source_filename.c_str());
            if (rc != VPL_OK) {
                LOG_WARN("Can not delete the temp file \"%s\", rc = %d.", temp_source_filename.c_str(), rc);
            }
        }

    } else if (ticket->filepath.size() > 0) {
        rv = ImageTranscode_transcode_private(ticket->filepath,
                                              ticket->type,
                                              ticket->width,
                                              ticket->height,
                                              &out_image,
                                              out_image_len);

    } else if (ticket->image != NULL && ticket->image_len > 0) {
        rv = ImageTranscode_transcode_private(ticket->image,
                                              ticket->image_len,
                                              ticket->type,
                                              ticket->width,
                                              ticket->height,
                                              &out_image,
                                              out_image_len);

    } else {
        // Should not happened.
        LOG_ERROR("No filepath and image binary data.");
        rv = IMAGE_TRANSCODING_FAILED;
    }

put_into_transcoding_data:
    VPLMutex_Lock(&transcoding_data_mutex);
    it = transcodingDatas.find(ticket->handle);
    if (it != transcodingDatas.end()) {
        if (rv == IMAGE_TRANSCODING_OK) {
            it->second.image = out_image;
            it->second.image_len = out_image_len;
        } else {
            LOG_ERROR("Failed to transcode, rv = %d", rv);
        }
        ticket->callback(ticket->handle, rv, ticket->callback_args);
    }
handle_unavailable:
    delete ticket;
    VPLMutex_Unlock(&transcoding_data_mutex);
    return VPLTHREAD_RETURN_VALUE;
}


