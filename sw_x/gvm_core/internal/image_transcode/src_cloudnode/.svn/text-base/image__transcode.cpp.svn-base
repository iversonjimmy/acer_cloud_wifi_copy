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
    std::string temp_filename;
    VPLFile_handle_t handle;
    bool has_scaled_photo = false;
    std::string return_photo_path;

    // Check the input file path
    {
        VPLFS_stat_t state;
        rv = VPLFS_Stat(filepath.c_str(), &state);
        if (rv != VPL_OK) {
            LOG_ERROR("Failed to read file from path: %s", filepath.c_str());
            return rv;
        }
    }

    // Use ffmpeg to generate scaled photo
    {
        const char* home_path = getenv("HOME");
        std::ostringstream oss;
        std::string command;

        switch(type) {
            case ImageType_JPG:
                temp_filename = generate_temp_filename(home_path, "jpg");
                break;
            case ImageType_PNG:
                temp_filename = generate_temp_filename(home_path, "png");
                break;
            case ImageType_TIFF:
                temp_filename = generate_temp_filename(home_path, "tiff");
                break;
            case ImageType_BMP:
                temp_filename = generate_temp_filename(home_path, "bmp");
                break;
            case ImageType_GIF:
                temp_filename = generate_temp_filename(home_path, "gif");
                break;
            case ImageType_Original:
            default:
                temp_filename = generate_temp_filename(home_path, "jpg");
                break;
        }

        /*
         * The ffmpeg command format:
         *     - /usr/bin/ffmpeg -i %SOURCE_FILEPATH% -vf "scale=min(%WIDTH%\,(min(%HEIGHT%\,ih)*iw)/ih):min((min(%WIDTH%\,iw)*ih)/iw\,%HEIGHT%)" -sws_flags bilinear %TARGET_FILEPATH%
         */
        oss << "/usr/bin/ffmpeg -i " << filepath << " -vf "
            << "\"scale=min(" << width << "\\,(min(" << height << "\\,ih)*iw)/ih):min((min(" << width << "\\,iw)*ih)/iw\\," << height << ")\""
            << " -sws_flags bilinear " << temp_filename;

        command = oss.str();

        LOG_DEBUG("Use ffmpeg to scale image, command: %s", command.c_str());
        rv = system(command.c_str());
        LOG_DEBUG("ffmpeg scaling completed.");

        if (rv < 0) {
            LOG_WARN("(%d) Failed to use ffmpeg to scale image \"%s\", return original photo.", rv, filepath.c_str());
            return_photo_path = filepath;
        } else {
            // Check the output file
            VPLFS_stat_t state;
            rv = VPLFS_Stat(temp_filename.c_str(), &state);
            if (rv == VPL_OK) {
                has_scaled_photo = true;
                return_photo_path = temp_filename;
            } else {
                LOG_WARN("Can not find the output file: \"%s\", return original photo.", temp_filename.c_str());
                return_photo_path = filepath;
            }
        }
    }

    // Read the file into buffer
    {
        unsigned char* buf = NULL;
        u32 data_length = 0;
        u32 read_bytes = 0;
        handle = VPLFile_Open(return_photo_path.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if ( !VPLFile_IsValidHandle(handle) ) {
            LOG_ERROR("Failed to open the scaled photo: %s", return_photo_path.c_str());

            // Error handling is to return original photo.
            if (return_photo_path == temp_filename) {
                handle = VPLFile_Open(filepath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
                if ( !VPLFile_IsValidHandle(handle) ) {
                    LOG_ERROR("Can not open original photo: %s", filepath.c_str());
                    rv = IMAGE_TRANSCODING_FAILED;
                    goto failed_to_open_scaled_photo;
                }
            } else {
                rv = IMAGE_TRANSCODING_FAILED;
                goto failed_to_open_scaled_photo;
            }
        }

        data_length = (u32)VPLFile_Seek(handle, 0, VPLFILE_SEEK_END);
        VPLFile_Seek(handle, 0, VPLFILE_SEEK_SET);
        if (data_length > 0) {
            buf = new (std::nothrow) unsigned char[data_length];
            if (buf == NULL) {
                LOG_ERROR("No memory.");
                goto failed_to_open_scaled_photo;
            }

            while (read_bytes < data_length) {
                ssize_t r = VPLFile_Read(handle, (void*)(buf + read_bytes), data_length - read_bytes);
                if (r == 0) {
                    break; // EOF (would happen if the file shrinks after we determine its size)
                }

                if (r < 0) {
                    LOG_ERROR("Got error while reading scaled photo (path = %s, rv = "FMTd_ssize_t")", filepath.c_str(), r);
                    delete buf;
                    goto failed_to_open_scaled_photo;
                }

                read_bytes += r;
            }

            if (read_bytes != data_length) {
                LOG_WARN("read_bytes != data_length (read_bytes:"FMTu32", data_length:"FMTu32")", read_bytes, data_length);
                data_length = read_bytes;
            }

            *out_image = buf;
            out_image_len = data_length;
        }
        else {
            *out_image = NULL;
            out_image_len = 0;
        }
    }

failed_to_open_scaled_photo:
    if (VPLFile_IsValidHandle(handle)) {
        VPLFile_Close(handle);
    }

    // Delete the temp file.
    if (has_scaled_photo) {
        int rc = VPLFile_Delete(temp_filename.c_str());
        if (rc != VPL_OK) {
            LOG_WARN("Can not delete the temp file: %s, rc = %d", temp_filename.c_str(), rc);
        }
    }

    return IMAGE_TRANSCODING_OK;
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

#if 0
int ImageTranscode_transcode_private(std::string filepath,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    try {
        unsigned char* buffer = NULL;
        Magick::Blob blob;
        Magick::Image image(filepath);
        Magick::Geometry geometry(width, height);
        try {
            image.read(filepath);
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while reading image from file(%s) - %s", filepath.c_str(), warning.what());
        }
        try {
            image.sample(geometry);
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while transcoding - %s", warning.what());
        }
        try {
            image.write(&blob);
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while writing data to Blob - %s", warning.what());
        }

        buffer = new unsigned char[blob.length()];
        memcpy(buffer, blob.data(), blob.length());

        *out_image = buffer;
        out_image_len = blob.length();
        return IMAGE_TRANSCODING_OK;
    } catch (Magick::Exception &error) {
        LOG_ERROR("Failed to transcode, Magick::Exception - %s", error.what());
    } catch (std::exception &error) {
        LOG_ERROR("Failed to transcode, Exception - %s", error.what());
    } catch (...) {
        LOG_ERROR("Failed to transcode, Exception - Uknown Error.");
    }
    return IMAGE_TRANSCODING_FAILED;
}

int ImageTranscode_transcode_private(const unsigned char* image,
                                     size_t image_len,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    try {
        unsigned char* buffer = NULL;
        Magick::Blob in_blob;
        Magick::Blob out_blob;
        Magick::Image magick_image;
        Magick::Geometry geometry(width, height);

        try {
            in_blob = Magick::Blob(image, image_len);
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while creating Blob from binary - %s", warning.what());
        }
        try {
            magick_image.read(in_blob);
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while reading data from Blob - %s", warning.what());
        }
        try {
            switch(type) {
                case ImageType_JPG:
                    magick_image.magick("JPEG");
                    break;
                case ImageType_PNG:
                    magick_image.magick("PNG");
                    break;
                case ImageType_TIFF:
                    magick_image.magick("TIFF");
                    break;
                case ImageType_BMP:
                    magick_image.magick("BMP");
                    break;
                case ImageType_GIF:
                    magick_image.magick("GIF");
                    break;
                case ImageType_Original:
                default:
                    break;
            }
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while setting image type - %s", warning.what());
        }
        try {
            magick_image.sample(geometry);
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while transcoding - %s", warning.what());
        }
        try {
            magick_image.write(&out_blob);
        } catch (Magick::Warning &warning) {
            LOG_WARN("Got warning while writing data to Blob - %s", warning.what());
        }

        buffer = new unsigned char[out_blob.length()];
        memcpy(buffer, out_blob.data(), out_blob.length());

        *out_image = buffer;
        out_image_len = out_blob.length();
        return IMAGE_TRANSCODING_OK;
    } catch (Magick::Exception &error) {
        LOG_ERROR("Failed to transcode, Magick::Exception - %s", error.what());
    } catch (std::exception &error) {
        LOG_ERROR("Failed to transcode, Exception - %s", error.what());
    } catch (...) {
        LOG_ERROR("Failed to transcode, Exception - Uknown Error.");
    }
    return IMAGE_TRANSCODING_FAILED;
}
#endif

int ImageTranscode_shutdown_private()
{
    return IMAGE_TRANSCODING_OK;
}
