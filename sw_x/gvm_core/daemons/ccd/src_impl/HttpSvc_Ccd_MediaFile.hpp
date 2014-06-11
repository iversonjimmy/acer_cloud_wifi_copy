#ifndef __HTTPSVC_CCD_MEDIAFILE_HPP__
#define __HTTPSVC_CCD_MEDIAFILE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_file.h>
#include "ccd_features.h"

#include <string>

#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
#include <image_transcode.h>
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE

namespace HttpSvc {
    namespace Ccd {
        // Abstraction of media file (possibly transcoded) to return.
        // Note to Maintainer: hide all product/platform specific code in this class.
        class MediaFile {
        public:
            MediaFile();
            virtual ~MediaFile();
            virtual int Open() = 0;
            virtual int Close() = 0;
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
            virtual int Transcode(size_t width, size_t height, ImageTranscode_ImageType imageType) = 0;
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
            virtual s64 GetSize() const = 0;
            virtual ssize_t ReadAt(u64 offset, char *buf, size_t bufsize) = 0;

            static MediaFile *Create(const std::string &location);

        protected:
            bool isTranscoded() const;
#ifdef ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
            ImageTranscode_handle_t transcodeHandle;
            s64 getTranscodedSize() const;
            ssize_t readTranscodedAt(u64 offset, char *buf, size_t bufsize);
#endif // ENABLE_CLIENTSIDE_PHOTO_TRANSCODE
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard

