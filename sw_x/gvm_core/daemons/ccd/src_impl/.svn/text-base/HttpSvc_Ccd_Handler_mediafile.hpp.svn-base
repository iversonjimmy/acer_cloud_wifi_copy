#ifndef __HTTPSVC_CCD_HANDLER_MEDIAFILE_HPP__
#define __HTTPSVC_CCD_HANDLER_MEDIAFILE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "HttpSvc_Ccd_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler_mediafile : public Handler {
        public:
            Handler_mediafile(HttpStream *hs);
            ~Handler_mediafile();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_mediafile);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
