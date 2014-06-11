#ifndef __HTTPSVC_SN_MEDIA_FILE_SENDER_HPP__
#define __HTTPSVC_SN_MEDIA_FILE_SENDER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <string>
#include <utility>
#include <vector>

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class MediaFile;

        class MediaFileSender {
        public:
            MediaFileSender(MediaFile *mf, const std::string &mediaType, HttpStream *hs);
            ~MediaFileSender();
            int Send();

        private:
            MediaFile *mf;
            std::string mediaType;
            HttpStream *hs;

            std::string location;  // need to be init;d

            std::vector<std::pair<std::string, std::string> > ranges;

            int processRangeSpec();
            int sendHttpHeaders();
            int sendWholeFile();
            int sendSingleRange();
            int sendMultipleRanges();
#if 0
#endif
            void generateBoundaryString(std::string &boundaryString);
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard

