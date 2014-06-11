#include "HttpSvc_Sn_MediaDsFile.hpp"

#include "dataset.hpp"
#include "vss_file.hpp"

#include <gvm_errors.h>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_fs.h>
#include <vplex_file.h>

#include <sstream>

HttpSvc::Sn::MediaDsFile::MediaDsFile(dataset *ds, const std::string &path)
    : ds(ds), path(path), file(NULL)
{
#ifdef CLOUDNODE
    if (this->path.empty()) {
        this->path.assign("/");
    }
    else if (this->path[0] != '/') {
        this->path.insert(0, "/");
    }
    // ELSE path already begins with '/'
#endif // CLOUDNODE

    LOG_INFO("MediaDsFile[%p]: Created for dataset[%p] (id "FMTu64"), path %s", this, ds, (ds ? ds->get_id().did : 0), this->path.c_str());
    if (!ds) {
        LOG_WARN("MediaDsFile[%p]: NULL dataset", this);
    }
}

HttpSvc::Sn::MediaDsFile::~MediaDsFile()
{
    Close();

    LOG_INFO("MediaDsFile[%p]: Destroyed", this);
}

int HttpSvc::Sn::MediaDsFile::Open()
{
    int err = 0;

#ifdef ENABLE_PHOTO_TRANSCODE
    if (isTranscoded()) {
        // nothing to do
        return 0;
    }
#endif // ENABLE_PHOTO_TRANSCODE

    assert(!isTranscoded());

    if (!ds) {
        LOG_ERROR("MediaDsFile[%p]: NULL dataset", this);
        return CCD_ERROR_NOT_INIT;
    }

    err = ds->open_file(path, /*version*/0, VSSI_FILE_OPEN_READ | VSSI_FILE_SHARE_READ, /*attrs*/0, file);
    if (err) {
        LOG_ERROR("Failed to open file: dataset "FMTu64", path %s, err %d", ds->get_id().did, path.c_str(), err);
        return err;
    }

    return err;
}

int HttpSvc::Sn::MediaDsFile::Close()
{
    int err = 0;

    if (!ds) {
        LOG_ERROR("MediaDsFile[%p]: NULL dataset", this);
        return CCD_ERROR_NOT_INIT;
    }

    if (file) {
        err = ds->close_file(file, /*vss_object*/NULL, /*origin*/0);
        file = NULL;
    }

    return err;
}

#ifdef ENABLE_PHOTO_TRANSCODE
static int closeVssFile(dataset *ds, vss_file *file)
{
    return ds->close_file(file, /*vss_object*/NULL, /*origin*/0);
}

static int closeThenDeleteVplFile(VPLFile_handle_t file, const char *path)
{
    VPLFile_Close(file);
    return VPLFile_Delete(path);
}

int HttpSvc::Sn::MediaDsFile::Transcode(size_t width, size_t height, ImageTranscode_ImageType imageType)
{
    int err = 0;

    if (!ds) {
        LOG_ERROR("MediaDsFile[%p]: NULL dataset", this);
        return CCD_ERROR_NOT_INIT;
    }

    // ImageTranscode_Transcode() requires path to source file; it cannot deal with a file in a dataset.
    // Thus, copy the file out of the dataset into a temporary file, transcode, and then delee.

    // If something goes wrong, reset transcodeHandle and report success (return 0), 
    // so that the original image will be returned.

    u64 filesize = 0;
    {
        VPLFS_stat_t stat;
        err = ds->stat_component(path, stat);
        if (err) {
            LOG_ERROR("Falied to stat file in dataset "FMTu64" path %s: err %d", ds->get_id().did, path.c_str(), err);
            return 0;  // report success to return original image
        }
        filesize = stat.size;
    }

    std::string tmppath;
    {
        std::string src_ext;
        size_t pos = path.find_last_of("/.");
        if (pos == path.npos || path[pos] == '/') {
            // Cannot determine source image type from source filename extension.
            return 0;  // report success to return original image
        }
        src_ext.assign(path, pos, path.npos);

        std::ostringstream oss;
        oss << getenv("HOME") << "/" << VPLTime_GetTimeStamp() << "." << src_ext;
        tmppath.assign(oss.str());
    }

    VPLFile_handle_t vplfile;
    vplfile = VPLFile_Open(tmppath.c_str(), VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0600);
    if (!VPLFile_IsValidHandle(vplfile)) {
        LOG_ERROR("Failed to open file %s", tmppath.c_str());
        return 0;  // report success to return original image
    }
    ON_BLOCK_EXIT(closeThenDeleteVplFile, vplfile, tmppath.c_str());

    vss_file *vssfile = NULL;
    err = ds->open_file(path, 0, VSSI_FILE_OPEN_READ | VSSI_FILE_SHARE_READ, 0, vssfile);
    if (err) {
        LOG_ERROR("Failed to open file in dataset "FMTu64" path %s: err %d", ds->get_id().did, path.c_str(), err);
        return 0;  // report success to return original image
    }
    ON_BLOCK_EXIT(closeVssFile, ds, vssfile);

    u64 offset = 0;
    while (offset < filesize) {
        char buf[4096];
        u32 length = sizeof(buf);
        err = ds->read_file(vssfile, /*vss_object*/NULL, /*origin*/0, offset, length, buf);
        if (err) {
            LOG_ERROR("Failed to read from file %s: err %d", path.c_str(), err);
            return 0;  // report success to return original image
        }
        offset += length;

        ssize_t bytes = VPLFile_Write(vplfile, buf, length);
        if (bytes < 0) {
            LOG_ERROR("Failed to write to file %s: err %d", tmppath.c_str(), bytes);
            return 0;  // report success to return original image
        }
        if ((u32)bytes < length) {
            LOG_ERROR("Partial write to file %s: wrote "FMT_ssize_t", expected "FMTu32, tmppath.c_str(), bytes, length);
            return 0;  // report success to return original image
        }
    }

    err = ImageTranscode_Transcode(tmppath.c_str(), tmppath.size(),
                                   /*image*/NULL, /*image_len*/0,
                                   imageType,
                                   width, height,
                                   &transcodeHandle);
    if (err) {
        LOG_ERROR("Failed to transcode image: err %d", err);
        if (transcodeHandle != INVALID_IMAGE_TRANSCODE_HANDLE) {
            ImageTranscode_DestroyHandle(transcodeHandle);
            transcodeHandle = INVALID_IMAGE_TRANSCODE_HANDLE;
            assert(!isTranscoded());
        }
    }

    return err;
}
#endif // ENABLE_PHOTO_TRANSCODE

s64 HttpSvc::Sn::MediaDsFile::GetSize() const
{
#ifdef ENABLE_PHOTO_TRANSCODE
    if (isTranscoded()) {
        return getTranscodedSize();
    }
#endif // ENABLE_PHOTO_TRANSCODE

    assert(!isTranscoded());

    if (!ds) {
        LOG_ERROR("MediaDsFile[%p]: NULL dataset", this);
        return CCD_ERROR_NOT_INIT;
    }

    VPLFS_stat_t stat;
    int err = ds->stat_component(path, stat);
    if (err) {
        LOG_ERROR("Failed to stat file: dataset "FMTu64", path %s, err %d", ds->get_id().did, path.c_str(), err);
        return err;
    }

    return stat.size;
}

ssize_t HttpSvc::Sn::MediaDsFile::ReadAt(u64 offset, char *buf, size_t bufsize)
{
#ifdef ENABLE_PHOTO_TRANSCODE
    if (isTranscoded()) {
        return readTranscodedAt(offset, buf, bufsize);
    }
#endif // ENABLE_PHOTO_TRANSCODE

    assert(!isTranscoded());

    if (!ds) {
        LOG_ERROR("MediaDsFile[%p]: NULL dataset", this);
        return CCD_ERROR_NOT_INIT;
    }

    u32 size = bufsize;
    int err = ds->read_file(file, /*vss_object*/NULL, /*origin*/0, offset, size, buf);
    if (err) {
        LOG_ERROR("Failed to read from file: dataset "FMTu64", path %s, err %d", ds->get_id().did, path.c_str(), err);
        return err;
    }

    return size;
}

