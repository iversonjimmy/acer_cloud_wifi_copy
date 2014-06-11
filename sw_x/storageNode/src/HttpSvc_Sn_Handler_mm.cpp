#include "HttpSvc_Sn_Handler_mm.hpp"

#include "HttpSvc_Sn_MediaFile.hpp"
#include "HttpSvc_Sn_MediaFileSender.hpp"
#include "HttpSvc_Sn_MediaMetadata.hpp"

#include "vss_server.hpp"

#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>
#include <util_mime.hpp>

#include <vplex_http_util.hpp>

#include <new>
#include <sstream>
#include <string>
#include <vector>

HttpSvc::Sn::Handler_mm::Handler_mm(HttpStream *hs)
    : Handler(hs), mediaMetadata(NULL), mediafile(NULL)
{
    LOG_INFO("Handler_mm[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Sn::Handler_mm::~Handler_mm()
{
    if (mediaMetadata) {
        delete mediaMetadata;
    }
    LOG_INFO("Handler_mm[%p]: Destroyed", this);
}

int HttpSvc::Sn::Handler_mm::getMediaFileInfo()
{
    int err = 0;

    // URI looks like
    // /mm/00000006d7cb7f87/c/Mjg0MzMzMzllYjJlNWU5MmQ2OTg1MzQ5NThiYWUwODg5MzZiMGZmZA==/Mjg0MzMzMzllYjJlNWU5MmQ2OTg1MzQ5NThiYWUwODg5MzZiMGZmZA==.wma
    // /mm/00000006d7cb7f87/t/MDk5M2RjZDdjMWRjM2YxMjVmOGEzYTU3YjM1MjQ1MWI3MDMwZmRmMw==/MDk5M2RjZDdjMWRjM2YxMjVmOGEzYTU3YjM1MjQ1MWI3MDMwZmRmMw==/jpg
    // Note the third token tells whether it's for the content or the thumbnail.

    std::vector<std::string> uri_tokens;
    VPLHttp_SplitUri(hs->GetUri(), uri_tokens);
    if (uri_tokens[2] != "c" && uri_tokens[2] != "t") {
        LOG_ERROR("Handler_mm[%p]: Unknown type marker: uri %s, marker %s", this, hs->GetUri().c_str(), uri_tokens[2].c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unknown type marker '" << uri_tokens[2] << "'\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return CCD_ERROR_PARAMETER;
    }
    bool isContentReq = uri_tokens[2] == "c";

    // TEMPORARY FOR 2.7.1
    std::string collectionId;
    {
        err = hs->GetReqHeader("x-ac-collectionId", collectionId);
        if (!err) {
            err = VPLHttp_DecodeUri(collectionId, collectionId);
        }
        if (err) {
            collectionId.clear();
            err = 0;  // reset error
        }
    }

    mediaMetadata = new (std::nothrow) MediaMetadata(vssServer, hs->GetUri(), /*objectId*/"", collectionId);
    if (!mediaMetadata) {
        LOG_ERROR("Handler_mm[%p]: Not enough memory", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return CCD_ERROR_NOMEM;
    }

    switch (mediaMetadata->GetMediaType()) {
    case media_metadata::MEDIA_MUSIC_TRACK:
        if (mediaMetadata->GetContentPath().empty()) {
            LOG_INFO("Handler_mm[%p]: Music track has no content", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"No content\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
            return CCD_ERROR_NOT_FOUND;
        }
        location = mediaMetadata->GetContentPath();
        LOG_INFO("Handler_mm[%p]: Found music track content at \"%s\"", this, location.c_str());
        break;

    case media_metadata::MEDIA_MUSIC_ALBUM:
        if (mediaMetadata->GetThumbnailPath().empty()) {
            LOG_INFO("Handler_mm[%p]: Music album has no thumbnail", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"No thumbnail\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
            return CCD_ERROR_NOT_FOUND;
        }
        location = mediaMetadata->GetThumbnailPath();
        LOG_INFO("Handler_mm[%p]: Found music album thumbnail at \"%s\"", this, location.c_str());
        break;

    case media_metadata::MEDIA_VIDEO:
        if (isContentReq) {
            if (mediaMetadata->GetContentPath().empty()) {
                LOG_INFO("Handler_mm[%p]: Video has no content", this);
                std::ostringstream oss;
                oss << "{\"errMsg\":\"No content\"}";
                HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
                return CCD_ERROR_NOT_FOUND;
            }
            location = mediaMetadata->GetContentPath();
            LOG_INFO("Handler_mm[%p]: Found video content at \"%s\"", this, location.c_str());
        }
        else {
            if (mediaMetadata->GetThumbnailPath().empty()) {
                LOG_INFO("Handler_mm[%p]: Video has no thumbnail", this);
                std::ostringstream oss;
                oss << "{\"errMsg\":\"No thumbnail\"}";
                HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
                return CCD_ERROR_NOT_FOUND;
            }
            location = mediaMetadata->GetThumbnailPath();
            LOG_INFO("Handler_mm[%p]: Found video thumbnail at \"%s\"", this, location.c_str());
        }
        break;

    case media_metadata::MEDIA_PHOTO:
        if (isContentReq) {
            if (mediaMetadata->GetContentPath().empty()) {
                LOG_INFO("Handler_mm[%p]: Photo has no content", this);
                std::ostringstream oss;
                oss << "{\"errMsg\":\"No content\"}";
                HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
                return CCD_ERROR_NOT_FOUND;
            }
            location = mediaMetadata->GetContentPath();
            LOG_INFO("Handler_mm[%p]: Found photo content at \"%s\"", this, location.c_str());
        }
        else {
            if (mediaMetadata->GetThumbnailPath().empty()) {
                LOG_INFO("Handler_mm[%p]: Photo has no thumbnail", this);
                std::ostringstream oss;
                oss << "{\"errMsg\":\"No thumbnail\"}";
                HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
                return CCD_ERROR_NOT_FOUND;
            }
            location = mediaMetadata->GetThumbnailPath();
            LOG_INFO("Handler_mm[%p]: Found photo thumbnail at \"%s\"", this, location.c_str());
        }
        break;

    case media_metadata::MEDIA_PHOTO_ALBUM:
        if (mediaMetadata->GetThumbnailPath().empty()) {
            LOG_INFO("Handler_mm[%p]: Photo album has no thumbnail", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"No thumbnail\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
            return CCD_ERROR_NOT_FOUND;
        }
        location = mediaMetadata->GetThumbnailPath();
        LOG_INFO("Handler_mm[%p]: Found photo album thumbnail at \"%s\"", this, location.c_str());
        break;

    case media_metadata::MEDIA_NONE:
    default:
        LOG_ERROR("Handler_mm[%p]: Unknown/unexpected media type %d", this, mediaMetadata->GetMediaType());
        std::stringstream oss;
        oss << "{\"errMsg\":\"Unknown/unexpected media type " << mediaMetadata->GetMediaType() << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return CCD_ERROR_PARAMETER;
    }

    return err;
}

int HttpSvc::Sn::Handler_mm::findTranscodeParams()
{
    int err = 0;

#ifdef ENABLE_PHOTO_TRANSCODE
    std::string dimension;
    err = hs->GetReqHeader("act_xcode_dimension", dimension);
    if (err) {
        // act_xcode_dimension not found; nothing more to do
        return 0;
    }

    size_t comma = dimension.find(',');
    if (comma == dimension.npos) {
        // dimension in unexpected format
        LOG_WARN("Handler_mm[%p]: Ignoring malformed dimension %s", this, dimension.c_str());
        return 0;
    }

    std::string width(dimension, 0, comma);
    std::string height(dimension, comma + 1);
    VPLHttp_Trim(width);
    VPLHttp_Trim(height);
    transcodeParams.width = atoi(width.c_str());
    transcodeParams.height = atoi(height.c_str());

    transcodeParams.doTranscode = true;

    LOG_INFO("Handler_mm[%p]: Transcode image size "FMTu_size_t" x "FMTu_size_t" (%s)",
             this,
             transcodeParams.width,
             transcodeParams.height,
             dimension.c_str());

    std::string fmt;
    err = hs->GetReqHeader("act_xcode_fmt", fmt);
    if (err) {
        // act_xcode_fmt not found; nothing more to do
        return 0;
    }

    VPLHttp_Trim(fmt);
    std::transform(fmt.begin(), fmt.end(), fmt.begin(), (int(*)(int))toupper);
    if (fmt == "JPG") {
        transcodeParams.imageType = ImageType_JPG;
    } else if (fmt == "PNG") {
        transcodeParams.imageType = ImageType_PNG;
    } else if (fmt == "TIFF") {
        transcodeParams.imageType = ImageType_TIFF;
    } else if (fmt == "BMP") {
        transcodeParams.imageType = ImageType_BMP;
    } else {
        LOG_ERROR("Handler_mm[%p]: Unsupported image type %s", this, fmt.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unsupported image type " << fmt << ".\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 415, oss.str(), "application/json");
        return -1;  // FIXME
    }

    LOG_INFO("Handler_mm[%p]: Transcode image to type %s", this, fmt.c_str());
#endif

    return err;
}

int HttpSvc::Sn::Handler_mm::transcode()
{
    int err = 0;

#ifdef ENABLE_PHOTO_TRANSCODE
    err = findTranscodeParams();
    if (err) {
        // HTTP response set by findTranscodeParams()
        goto end;
    }

    if (transcodeParams.doTranscode) {
        err = mediafile->Transcode(transcodeParams.width, transcodeParams.height, transcodeParams.imageType);
        if (err) {
            LOG_ERROR("Handler_mm[%p]: Failed to transcode; err %d", this, err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to transcode\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 415, oss.str(), "application/json");
            goto end;
        }
    }
 end:
#endif // ENABLE_PHOTO_TRANSCODE

    return err;
}

void HttpSvc::Sn::Handler_mm::guessMediaType(std::string &mediaType)
{
    mediaType.clear();

    if (transcodeParams.imageType != ImageType_Original) {
        std::string fmt;
        int err = hs->GetReqHeader("act_xcode_fmt", fmt);
        if (err) {
            // should never happened.
            LOG_WARN("Handler_mm[%p]: transcodeParams.imageType != ImageType_Original but there is no http header \"act_xcode_fmt\"", this);
        } else {
            VPLHttp_Trim(fmt);
            mediaType = Util_FindPhotoMediaTypeFromExtension(fmt);
        }

        if (mediaType.empty()) {
            mediaType.assign("image/unknown");
        }
        return;
    }

    size_t last_dot = location.find_last_of("./");
    if ((last_dot != location.npos) && (location[last_dot] == '.') && (location.size() - last_dot > 1)) {
        std::string extension(location, last_dot+1);
        // use file extension to guess the media type
        switch (mediaMetadata->GetMediaType()) {
        case media_metadata::MEDIA_MUSIC_TRACK:
            mediaType = Util_FindAudioMediaTypeFromExtension(extension);
            break;
        case media_metadata::MEDIA_MUSIC_ALBUM:
        case media_metadata::MEDIA_PHOTO_ALBUM:
        case media_metadata::MEDIA_PHOTO:
            mediaType = Util_FindPhotoMediaTypeFromExtension(extension);
            break;
        case media_metadata::MEDIA_VIDEO:
            mediaType = Util_FindVideoMediaTypeFromExtension(extension);
            break;
        case media_metadata::MEDIA_NONE:
        default:
            ;
        }
    }

    if (mediaType.empty()) {
        switch (mediaMetadata->GetMediaType()) {
        case media_metadata::MEDIA_MUSIC_TRACK:
            mediaType.assign("audio/unknown");
            break;
        case media_metadata::MEDIA_MUSIC_ALBUM:
        case media_metadata::MEDIA_PHOTO_ALBUM:
        case media_metadata::MEDIA_PHOTO:
            mediaType.assign("image/unknown");
            break;
        case media_metadata::MEDIA_VIDEO:
            mediaType.assign("video/unknown");
            break;
        case media_metadata::MEDIA_NONE:
        default:
            ;
        }
    }
}

int HttpSvc::Sn::Handler_mm::sendResponse()
{
    // IMPORTANT:
    // Returning an error from this function will cause HttpSvc::Sn::Dispatcher to treat the request as having failed.
    // If we were able to set an HTTP response, return 0.

    std::string mediaType;
    guessMediaType(mediaType);

    MediaFileSender *mfs = new (std::nothrow) MediaFileSender(mediafile, mediaType, hs);
    if (!mfs) {
        LOG_ERROR("Handler_mm[%p]: Not enough memory", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // Forget the error because we were able to set an HTTP response.
    }

    int err = mfs->Send();

    delete mfs;

    return err;
}

int HttpSvc::Sn::Handler_mm::Run()
{
    int err = 0;

    LOG_INFO("Handler_mm[%p]: Run", this);

    err = getMediaFileInfo();
    if (err) {
        // HTTP response set by getMediaFileInfo().
        err = 0;  // reset error
        goto end;
    }

    mediafile = MediaFile::Create(vssServer, location);
    if (!mediafile) {
        LOG_ERROR("Handler_mm[%p]: Not enough memory", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        err = 0;  // reset error
        goto end;
    }

    err = transcode();
    if (err) {
        // HTTP response set by transcode().
        err = 0;  // reset error
        goto end;
    }

    err = sendResponse();
    if (err) {
        // We won't reset the error.
        // This is because we might have already sent the response header,
        // in which case, the only recourse is to drop the connection by returning a non-zero value.
        goto end;
    }

 end:
    if (mediafile) {
        delete mediafile;
        mediafile = NULL;
    }

    return err;
}

