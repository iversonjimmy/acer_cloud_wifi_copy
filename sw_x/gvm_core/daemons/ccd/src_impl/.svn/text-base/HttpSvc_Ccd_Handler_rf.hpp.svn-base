#ifndef __HTTPSVC_CCD_HANDLER_RF_HPP__
#define __HTTPSVC_CCD_HANDLER_RF_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_th.h>

#include "HttpSvc_Ccd_Handler.hpp"

#include <map>
#include <string>
#include <vector>

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class AsyncAgent;

        class Handler_rf : public Handler {
        public:
            Handler_rf(HttpStream *hs);
            ~Handler_rf();
            static void SetAsyncAgent(AsyncAgent *asyncAgent);

        protected:
            int _Run();

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_rf);

            std::vector<std::string> uri_tokens;

            typedef int (Handler_rf::*req_handler_fn)();

            int process_async();
            int process_dataset();
            int process_device();
            int process_dir();
            int process_file();
            int process_filemetadata();
            int process_whitelist();
            int process_search();
            int process_dirmetadata();

            class ObjJumpTable {
            public:
                ObjJumpTable();
                std::map<std::string, req_handler_fn> handlers;
            };
            static ObjJumpTable objJumpTable;

            int process_async_delete();
            int process_async_get();
            int process_async_post();

            class AsyncMethodJumpTable {
            public:
                AsyncMethodJumpTable();
                std::map<std::string, req_handler_fn> handlers;
            };
            static AsyncMethodJumpTable asyncMethodJumpTable;

            // used only in case of /rf/async
            class AsyncLock {
            public:
                AsyncLock();
                ~AsyncLock();
                VPLMutex_t mutex;
            };
            static AsyncLock asyncLock;  // lock for the shared asyncAgent data member below
            static AsyncAgent *asyncAgent_shared;
            AsyncAgent *asyncAgent;  // this is for each instance of Handler_rf
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
