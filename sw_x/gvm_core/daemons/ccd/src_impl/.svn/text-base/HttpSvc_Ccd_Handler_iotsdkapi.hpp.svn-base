//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef HTTPSVC_CCD_HANDLER_IOTSDKAPI_HPP_
#define HTTPSVC_CCD_HANDLER_IOTSDKAPI_HPP_

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_util.hpp>

#include "HttpSvc_Ccd_Handler.hpp"
#include "vcs_defs.hpp"

#include <map>
#include <string>

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler_iotsdkapi : public Handler {
        public:
            Handler_iotsdkapi(HttpStream *hs);
            ~Handler_iotsdkapi();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_iotsdkapi);
            std::string apiVer;
            bool isIoStream;

            typedef int (Handler_iotsdkapi::*iotsdk_handler_fn)();

            class ApiHandlerTable { 
            public:
                ApiHandlerTable();
                std::map<std::string, iotsdk_handler_fn> handlers;
            }; 
            static ApiHandlerTable handlerTable;

            int do_local();
            int do_remote();
            int do_cloud();
        };
    } // namespace Ccd
} // namespace HttpSvc


#endif // HTTPSVC_CCD_HANDLER_IOTSDKAPI_HPP_
