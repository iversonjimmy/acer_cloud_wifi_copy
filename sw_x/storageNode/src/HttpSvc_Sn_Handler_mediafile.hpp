#ifndef __HTTPSVC_SN_HANDLER_MEDIAFILE_HPP__
#define __HTTPSVC_SN_HANDLER_MEDIAFILE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "HttpSvc_Sn_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class MediaMetadata;

        class Handler_mediafile : public Handler {
        public:
            Handler_mediafile(HttpStream *hs);
            ~Handler_mediafile();
            int Run();

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_mediafile);

            std::vector<std::string> uri_tokens;
            std::string objectId;
            std::list<std::pair<std::string, std::string> > tags;

            MediaMetadata *mediaMetadata;
            std::string location;
            std::string type;

            int parseRequest();
            int getMediaMetadata();
            int runTagEditor();
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard
