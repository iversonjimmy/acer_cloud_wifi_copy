#include "HttpSvc_Ccd_MediaFile.hpp"

#include "HttpSvc_Ccd_MediaFsFile.hpp"

#include <gvm_errors.h>
#include <log.h>

#include <vpl_conv.h>
#include <vplex_http_util.hpp>

#include <cassert>
#include <vector>

HttpSvc::Ccd::MediaFile::MediaFile()
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
    : transcodeHandle(INVALID_IMAGE_TRANSCODE_HANDLE)
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
{
}

HttpSvc::Ccd::MediaFile::~MediaFile()
{
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
    if (transcodeHandle != INVALID_IMAGE_TRANSCODE_HANDLE) {
        ImageTranscode_DestroyHandle(transcodeHandle);
    }
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
}

bool HttpSvc::Ccd::MediaFile::isTranscoded() const
{
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
    return transcodeHandle != INVALID_IMAGE_TRANSCODE_HANDLE;
#else
    return false;
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
}

#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
s64 HttpSvc::Ccd::MediaFile::getTranscodedSize() const
{
    assert(isTranscoded());

    size_t size;
    int err = ImageTranscode_GetContentLength(transcodeHandle, size);
    if (err) {
        LOG_ERROR("Failed to get transcoded image size: err %d", err);
        return err;
    }

    return size;
}
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE

#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
ssize_t HttpSvc::Ccd::MediaFile::readTranscodedAt(u64 offset, char *buf, size_t bufsize)
{
    assert(isTranscoded());

    int nbytes = ImageTranscode_Read(transcodeHandle, buf, bufsize, (size_t)offset);
    if (nbytes < 0) {
        LOG_ERROR("Failed to read transcoded image: err %d", nbytes);
    }

    return nbytes;
}
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE

HttpSvc::Ccd::MediaFile *HttpSvc::Ccd::MediaFile::Create(const std::string &location)
{
    // "location" is always a path in the device filesystem.
    return new (std::nothrow) HttpSvc::Ccd::MediaFsFile(location);
}

