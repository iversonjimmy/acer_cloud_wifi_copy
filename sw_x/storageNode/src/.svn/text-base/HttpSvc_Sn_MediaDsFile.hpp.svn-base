#ifndef __HTTPSVC_SN_MEDIADSFILE_HPP__
#define __HTTPSVC_SN_MEDIADSFILE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <string>

#include "sn_features.h"

#ifdef ENABLE_PHOTO_TRANSCODE
#include <image_transcode.h>
#endif

#include "HttpSvc_Sn_MediaFile.hpp"

class dataset;
class vss_file;

namespace HttpSvc {
    namespace Sn {
        class MediaDsFile : public MediaFile {
        public:
            MediaDsFile(dataset *ds, const std::string &location);
            ~MediaDsFile();
            int Open();
            int Close();
#ifdef ENABLE_PHOTO_TRANSCODE
            int Transcode(size_t width, size_t height, ImageTranscode_ImageType imageType);
#endif // ENABLE_PHOTO_TRANSCODE
            s64 GetSize() const;
            ssize_t ReadAt(u64 offset, char *buf, size_t bufsize);

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(MediaDsFile);

            dataset *ds;
            std::string path;

            vss_file *file;
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard

