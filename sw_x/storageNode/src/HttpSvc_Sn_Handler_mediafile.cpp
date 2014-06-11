#include "HttpSvc_Sn_Handler_mediafile.hpp"
#include "HttpSvc_Sn_MediaMetadata.hpp"

#include "vss_server.hpp"

#include <cJSON2.h>
#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>
#include <media_metadata_errors.hpp>
#include <media_metadata_types.pb.h>

#include <scopeguard.hpp>
#include <vplex_http_util.hpp>

#include <sstream>

HttpSvc::Sn::Handler_mediafile::Handler_mediafile(HttpStream *hs)
    : Handler(hs), mediaMetadata(NULL)
{
}

HttpSvc::Sn::Handler_mediafile::~Handler_mediafile()
{
    if (mediaMetadata)
        delete mediaMetadata;
}

int HttpSvc::Sn::Handler_mediafile::Run()
{
    int err = 0;

    err = parseRequest();
    if (err) {
        // parseRequest() already set HTTP response
        return 0;  // reset error
    }

    err = getMediaMetadata();
    if (err) {
        // getMediaMetadata() already set HTTP response
        return 0;  // reset error
    }

    err = runTagEditor();
    if (err) {
        // runTagEditor() already set HTTP response
        return 0;  // reset error
    }

    return err;
}

int HttpSvc::Sn::Handler_mediafile::parseRequest()
{
    int err = 0;

    // URI looks like /mediafile/tag/<deviceId>/<objectId>
    const std::string &uri = hs->GetUri();

    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() != 4) {
        LOG_ERROR("Handler_mediafile[%p]: Unexpected number of segments; uri %s", this, uri.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected number of segments; uri " << uri << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return CCD_ERROR_PARAMETER;
    }
    objectId.assign(uri_tokens[3]);

    char buf[4096];
    char *reqBody = NULL;
    {
        ssize_t bytes = hs->Read(buf, sizeof(buf) - 1);  // subtract 1 for EOL
        if (bytes < 0) {
            LOG_ERROR("Handler_mediafile[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to read from HttpStream\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
            return 0;
        }
        buf[bytes] = '\0';
        char *boundary = strstr(buf, "\r\n\r\n");  // find header-body boundary
        if (!boundary) {
            LOG_ERROR("Handler_mediafile[%p]: Failed to find header-body boundary in request", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to find header-body boundary in request\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;
        }
        reqBody = boundary + 4;
    }

    cJSON2 *json = cJSON2_Parse(reqBody);
    if (!json) {
        LOG_ERROR("Handler_mediafile[%p]: Failed to parse JSON in request body %s", this, reqBody);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to parse JSON in request body\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json);

    for (int i = 0; i < cJSON2_GetArraySize(json); i++) {
        cJSON2 *item = cJSON2_GetArrayItem(json, i);
        if (item->type != cJSON2_String) {
            LOG_WARN("Handler_mediafile[%p]: Ignored non-string value", this);
            continue;
        }
        tags.push_back(std::make_pair(item->string, item->valuestring));
    }

    return err;
}

int HttpSvc::Sn::Handler_mediafile::getMediaMetadata()
{
    int err = 0;

    mediaMetadata = new (std::nothrow) MediaMetadata(vssServer, "", objectId);
    if (!mediaMetadata) {
        LOG_ERROR("Handler_mediafile[%p]: Not enough memory", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return CCD_ERROR_NOMEM;
    }

    switch (mediaMetadata->GetMediaType()) {
    case media_metadata::MEDIA_MUSIC_TRACK:
    case media_metadata::MEDIA_PHOTO:
    case media_metadata::MEDIA_VIDEO:
        if (mediaMetadata->GetContentPath().empty()) {
            LOG_ERROR("Handler_mediafile[%p]: No content; object %s", this, objectId.c_str());
            std::ostringstream oss;
            oss << "{\"errMsg\":\"No content; object " << objectId << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
            return CCD_ERROR_NOT_FOUND;
        }
        location = mediaMetadata->GetContentPath();
        type = mediaMetadata->GetFileFormat();
        break;
    case media_metadata::MEDIA_MUSIC_ALBUM:
    case media_metadata::MEDIA_PHOTO_ALBUM:
    case media_metadata::MEDIA_NONE:
    default:
        LOG_ERROR("Handler_mediafile[%p]: Unexpected media type: object %s, type %d", this, objectId.c_str(), mediaMetadata->GetMediaType());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected media type: object " << objectId << ", type" << mediaMetadata->GetMediaType() << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return CCD_ERROR_PARAMETER;
    }

    LOG_INFO("Handler_mediafile[%p]: Handling objectId %s, type %s, location %s", this, objectId.c_str(), type.c_str(), location.c_str());

    return err;
}

int HttpSvc::Sn::Handler_mediafile::runTagEditor()
{
    int err = 0;

    std::string tagEditorPath = vssServer->getTagEditProgramPath();
    if (tagEditorPath.empty()) {
        LOG_ERROR("Handler_mediafile[%p]: Invalid path to Tag Editor", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Invalid path to Tag Editor\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return CCD_ERROR_NOT_INIT;
    }

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    err = _VPLFS_CheckTrustedExecutable(tagEditorPath);
    if (err == VPL_ERR_NOENT) {
        LOG_ERROR("Handler_mediafile[%p]: Tag Editor not found; path %s", this, tagEditorPath.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Tag Editor not found\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return err;
    }
    if (err) {
        LOG_ERROR("Handler_mediafile[%p]: Tag Editor not trusted; path %s, err %d", this, tagEditorPath.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Tag Editor not trusted\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return err;
    }
#endif // defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    char *vplOsUserId = NULL;
#if defined(WIN32)
    _VPL__GetUserSidStr(&vplOsUserId);
#endif // defined(WIN32)
    if (vplOsUserId == NULL) {
        err = VPL_GetOSUserId(&vplOsUserId);
        if (err < 0) {
            LOG_ERROR("Handler_mediafile[%p]: Failed to determine OS userId: err %d", this, err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to determine OS userId\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
            return err;
        }
    }
    ON_BLOCK_EXIT(VPL_ReleaseOSUserId, vplOsUserId);

    std::string cmdline;
    {
        std::ostringstream oss;
        oss << tagEditorPath
            << " --osuserid " << vplOsUserId
            << " --type " << type
            << " --file \"" << location << "\"";
        std::list<std::pair<std::string, std::string> >::const_iterator it;
        for (it = tags.begin(); it != tags.end(); it++) {
            oss << " --" << it->first << " " << it->second;
        }
        cmdline = oss.str();
    }
    LOG_INFO("Handler_mediafile[%p]: cmdline %s", this, cmdline.c_str());

#if defined(WIN32)
    wchar_t *wcmdline = NULL;
    err = _VPL__utf8_to_wstring(cmdline.c_str(), &wcmdline);
    if (err) {
        LOG_ERROR("Handler_mediafile[%p]: Failed to convert to wchar_t: cmdline: %s, err %d", this, cmdline.c_str(), err);
        HttpStream_Helper::SetCompleteResponse(hs, 500);
        return err;
    }
    ON_BLOCK_EXIT(free, wcmdline);

    err = _wsystem(wcmdline);
    // TODO: need to make sure cmdline ran
#else
    err = system(cmdline.c_str());
    // TODO: need to make sure cmdline ran, and also separate exit code from others
#endif

    if (err) {
        LOG_ERROR("Handler_mediafile[%p]: Tag Editor failed: err %d", this, err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"ERR_TAGEDITOR(" << err << ")\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return err;
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return err;
}
