#ifndef __HTTPSVC_CCD_HANDLER_MINIDMS_HPP__
#define __HTTPSVC_CCD_HANDLER_MINIDMS_HPP__

#include <vplu_types.h>
#include <vplu_common.h>

#include "HttpSvc_Ccd_Handler.hpp"

#include <string>
#include <vector>
#include <map>
#include <cJSON2.h>

#include "JsonHelper.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler_minidms : public Handler {
        public:
            Handler_minidms(HttpStream *hs);
            ~Handler_minidms();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_minidms);

            std::vector<std::string> uri_tokens;

            typedef int (Handler_minidms::*req_handler_fn)();

            int process_protocolinfo();
            int process_pin();
            int process_pin_get();
            int process_pin_put();
            int process_pin_delete();
            int process_deviceinfo();

            class ObjJumpTable {
            public:
                ObjJumpTable();
                std::map<std::string, req_handler_fn> handlers;
            };
            static ObjJumpTable objJumpTable;

            class PINMethodJumpTable {
            public:
                PINMethodJumpTable();
                std::map<std::string, req_handler_fn> handlers;
            };
            static PINMethodJumpTable pinMethodJumpTable;
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
