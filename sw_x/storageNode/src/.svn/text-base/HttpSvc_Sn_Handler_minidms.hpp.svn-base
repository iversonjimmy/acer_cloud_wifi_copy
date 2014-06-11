#ifndef __HTTPSVC_SN_HANDLER_MINIDMS_HPP__
#define __HTTPSVC_SN_HANDLER_MINIDMS_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "sn_features.h"

#include <media_metadata_errors.hpp>
#include <media_metadata_types.pb.h>

#include "HttpSvc_Sn_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class Handler_minidms : public Handler {
        public:
            Handler_minidms(HttpStream *hs);
            ~Handler_minidms();
            int Run();

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_minidms);

            std::vector<std::string> uri_tokens;

            typedef int (Handler_minidms::*req_handler_fn)();

            int process_deviceinfo();

            class ObjJumpTable {
            public:
                ObjJumpTable();
                std::map<std::string, req_handler_fn> handlers;
            };
            static ObjJumpTable objJumpTable;

        };
    } // namespace Sn
} // namespace HttpSvc

#endif // __HTTPSVC_SN_HANDLER_MINIDMS_HPP__
