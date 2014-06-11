#include "HttpSvc_Sn_MediaFsFile.hpp"

#include <log.h>

#include <vpl_fs.h>

#include <cassert>

HttpSvc::Sn::MediaFsFile::MediaFsFile(const std::string &path)
    : path(path), file(VPLFILE_INVALID_HANDLE)
{
    LOG_INFO("MediaFsFile[%p]: Created for path %s", this, path.c_str());
}

HttpSvc::Sn::MediaFsFile::~MediaFsFile()
{
    Close();

    LOG_INFO("MediaFsFile[%p]: Destroyed", this);
}

int HttpSvc::Sn::MediaFsFile::Open()
{
    int err = 0;

#ifdef ENABLE_PHOTO_TRANSCODE
    if (isTranscoded()) {
        // nothing to do
        return 0;
    }
#endif // ENABLE_PHOTO_TRANSCODE

    assert(!isTranscoded());

    file = VPLFile_Open(path.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(file)) {
        LOG_ERROR("Failed to open file: path %s", path.c_str());
        return -1;
    }

    return err;
}

int HttpSvc::Sn::MediaFsFile::Close()
{
    int err = 0;

    if (VPLFile_IsValidHandle(file)) {
        err = VPLFile_Close(file);
        file = VPLFILE_INVALID_HANDLE;
    }

    return err;
}

#ifdef ENABLE_PHOTO_TRANSCODE
int HttpSvc::Sn::MediaFsFile::Transcode(size_t width, size_t height, ImageTranscode_ImageType imageType)
{
    int err = 0;

    // If something goes wrong, reset transcodeHandle and report success (return 0), 
    // so that the original image will be returned.

    err = ImageTranscode_Transcode(path.c_str(), path.size(),
                                   /*image*/NULL, /*image_len*/0,
                                   imageType,
                                   width, height,
                                   &transcodeHandle);
    if (err) {
        LOG_ERROR("Failed to transcode image: path %s, err %d", path.c_str(), err);
        if (transcodeHandle != INVALID_IMAGE_TRANSCODE_HANDLE) {
            ImageTranscode_DestroyHandle(transcodeHandle);
            transcodeHandle = INVALID_IMAGE_TRANSCODE_HANDLE;
            assert(!isTranscoded());
        }
    }

    return err;
}
#endif // ENABLE_PHOTO_TRANSCODE

s64 HttpSvc::Sn::MediaFsFile::GetSize() const
{
#ifdef ENABLE_PHOTO_TRANSCODE
    if (isTranscoded()) {
        return getTranscodedSize();
    }
#endif // ENABLE_PHOTO_TRANSCODE

    assert(!isTranscoded());

    VPLFS_stat_t stat;
    int err = VPLFS_Stat(path.c_str(), &stat);
    if (err) {
        LOG_ERROR("Failed to stat file: path %s, err %d", path.c_str(), err);
        return err;
    }

    return stat.size;
}

ssize_t HttpSvc::Sn::MediaFsFile::ReadAt(u64 offset, char *buf, size_t bufsize)
{
#ifdef ENABLE_PHOTO_TRANSCODE
    if (isTranscoded()) {
        return readTranscodedAt(offset, buf, bufsize);
    }
#endif // ENABLE_PHOTO_TRANSCODE

    assert(!isTranscoded());

    ssize_t nbytes = VPLFile_ReadAt(file, buf, bufsize, offset);
    if (nbytes < 0) {
        LOG_ERROR("Failed to read from file: path %s, err %d", path.c_str(), nbytes);
        return nbytes;
    }

    return nbytes;
}

