#ifndef __HTTPSVC_CCD_HANDLER_DS_HPP__
#define __HTTPSVC_CCD_HANDLER_DS_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "HttpSvc_Ccd_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler_ds : public Handler {
        public:
            Handler_ds(HttpStream *hs);
            ~Handler_ds();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_ds);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
