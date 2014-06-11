#ifndef __HTTPSVC_CCD_HANDLER_REMOTE_EXECUTABLE_HPP__
#define __HTTPSVC_CCD_HANDLER_REMOTE_EXECUTABLE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "HttpSvc_Ccd_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler_rexe : public Handler {
        public:
            Handler_rexe(HttpStream *hs);
            ~Handler_rexe();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_rexe);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
