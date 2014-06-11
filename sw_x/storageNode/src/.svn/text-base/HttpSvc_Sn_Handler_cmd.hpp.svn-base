#ifndef __HTTPSVC_SN_HANDLER_CMD_HPP__
#define __HTTPSVC_SN_HANDLER_CMD_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "HttpSvc_Sn_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class Handler_cmd : public Handler {
        public:
            Handler_cmd(HttpStream *hs);
            ~Handler_cmd();
            int Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_cmd);
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard
