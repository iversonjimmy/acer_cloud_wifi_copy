#ifndef __HTTPSVC_SN_MEDIA_METADATA_HPP__
#define __HTTPSVC_SN_MEDIA_METADATA_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <string>

#include <media_metadata_types.pb.h>

class vss_server;

namespace HttpSvc {
    namespace Sn {
        class MediaMetadata {
        public:
            MediaMetadata(vss_server *vssServer, const std::string &uri, const std::string &objectId);
            MediaMetadata(vss_server *vssServer, const std::string &uri, const std::string &objectId, const std::string &collectionId);  // TEMPORARY FOR 2.7.1
            ~MediaMetadata();
            media_metadata::MediaType_t GetMediaType() const;
            const std::string &GetContentPath() const;
            const std::string &GetThumbnailPath() const;
            const std::string &GetFileFormat() const;
        private:
            vss_server *vssServer;
            std::string uri;
            std::string objectId;
            std::string collectionId;

            int get() const;
            mutable bool cached;
            mutable media_metadata::GetObjectMetadataOutput metadata;
        };
    } // namespace Sn
} // namespace HttpSvc
#endif // incl guard
