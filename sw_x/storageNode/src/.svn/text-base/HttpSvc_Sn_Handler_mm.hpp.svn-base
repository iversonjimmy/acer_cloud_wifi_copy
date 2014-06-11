#ifndef __HTTPSVC_SN_HANDLER_MM_HPP__
#define __HTTPSVC_SN_HANDLER_MM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <string>

#include "sn_features.h"

#ifdef ENABLE_PHOTO_TRANSCODE
#include <image_transcode.h>
#endif
#include <media_metadata_errors.hpp>
#include <media_metadata_types.pb.h>

#include "HttpSvc_Sn_Handler.hpp"
//#include "HttpSvc_Sn_MediaFile.hpp"
//#include "HttpSvc_Sn_MediaMetadata.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class MediaFile;
        class MediaMetadata;

        class Handler_mm : public Handler {
        public:
            Handler_mm(HttpStream *hs);
            ~Handler_mm();
            int Run();

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_mm);

            MediaMetadata *mediaMetadata;
            std::string location;
            MediaFile *mediafile;

#ifdef ENABLE_PHOTO_TRANSCODE
            struct TranscodeParams {
                bool doTranscode;
                size_t width;
                size_t height;
                ImageTranscode_ImageType imageType;
                TranscodeParams() : doTranscode(false), width(0), height(0), imageType(ImageType_Original) {}
            };
            TranscodeParams transcodeParams;
#endif

            int getMediaFileInfo();
            int transcode();
            int sendResponse();

            int findTranscodeParams();
            void guessMediaType(std::string &mediaType);
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard
