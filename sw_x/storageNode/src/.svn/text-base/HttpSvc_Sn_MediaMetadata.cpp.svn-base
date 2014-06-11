#include "HttpSvc_Sn_MediaMetadata.hpp"

#include "vss_server.hpp"

#include <gvm_errors.h>
#include <log.h>

HttpSvc::Sn::MediaMetadata::MediaMetadata(vss_server *vssServer, const std::string &uri, const std::string &objectId)
    : vssServer(vssServer), uri(uri), objectId(objectId),
      cached(false)
{
    if (uri.empty() && objectId.empty()) {
        LOG_WARN("Both uri and objectId are empty");
    }
}

// TEMPORARY FOR 2.7.1
HttpSvc::Sn::MediaMetadata::MediaMetadata(vss_server *vssServer, const std::string &uri, const std::string &objectId, const std::string &collectionId)
    : vssServer(vssServer), uri(uri), objectId(objectId), collectionId(collectionId),
      cached(false)
{
    if (uri.empty() && objectId.empty()) {
        LOG_WARN("Both uri and objectId are empty");
    }
    if (collectionId.empty()) {
        LOG_WARN("CollectionId is empty");
    }
}

HttpSvc::Sn::MediaMetadata::~MediaMetadata()
{
}

int HttpSvc::Sn::MediaMetadata::get() const
{
    int err = 0;

    if (vssServer == NULL) {
        LOG_ERROR("VSS server not found");
        return CCD_ERROR_NOT_INIT;
    }

    vss_server::msaGetObjectMetadataFunc_t msaGetObjectMetadata = vssServer->getMsaGetObjectMetadataCb();
    if (msaGetObjectMetadata == NULL) {
        LOG_ERROR("No route to media metadata access function");
        return CCD_ERROR_NOT_INIT;
    }

    media_metadata::GetObjectMetadataInput request;
    if (!uri.empty())
        request.set_url(uri);
    else if (!objectId.empty())
        request.set_object_id(objectId);
    else {
        LOG_ERROR("Both uri and objectId are empty");
        return CCD_ERROR_PARAMETER;
    }
    err = msaGetObjectMetadata(request, collectionId, metadata, vssServer->getMsaCallbackContext());
    if (err) {
        LOG_ERROR("Metadata not found: err %d", err);
        return CCD_ERROR_NOT_FOUND;
    }

    return err;
}

media_metadata::MediaType_t HttpSvc::Sn::MediaMetadata::GetMediaType() const
{
    if (!cached) {
        get();
        cached = true;
    }
    return metadata.media_type();
}

const std::string &HttpSvc::Sn::MediaMetadata::GetContentPath() const
{
    if (!cached) {
        get();
        cached = true;
    }
    return metadata.absolute_path();
}

const std::string &HttpSvc::Sn::MediaMetadata::GetThumbnailPath() const
{
    if (!cached) {
        get();
        cached = true;
    }
    return metadata.thumbnail();
}

const std::string &HttpSvc::Sn::MediaMetadata::GetFileFormat() const
{
    if (!cached) {
        get();
        cached = true;
    }
    return metadata.file_format();
}
