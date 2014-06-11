#ifndef __HTTPSVC_CCD_HANDLER_CLOUDDOC_HPP__
#define __HTTPSVC_CCD_HANDLER_CLOUDDOC_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <map>
#include <string>
#include <vector>

#include "HttpSvc_Ccd_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler_clouddoc : public Handler {
        public:
            Handler_clouddoc(HttpStream *hs);
            ~Handler_clouddoc();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_clouddoc);

            std::vector<std::string> uri_tokens;

            typedef int (Handler_clouddoc::*clouddoc_handler_fn)();

            int run_async();
            int run_dir();
            int run_file();
            int run_filemetadata();
            int run_preview();
            int run_conflict();
            int run_copyback();

            class ObjJumpTable {
            public:
                ObjJumpTable();
                std::map<std::string, clouddoc_handler_fn> handlers;
            };
            static ObjJumpTable objJumpTable;

            int run_async_delete();
            int run_async_get();
            int run_async_post();

            class AsyncMethodJumpTable {
            public:
                AsyncMethodJumpTable();
                std::map<std::string, clouddoc_handler_fn> handlers;
            };
            static AsyncMethodJumpTable asyncMethodJumpTable;

            int run_file_get();
            int run_file_post();
            int run_file_delete();

            class FileMethodJumpTable {
            public:
                FileMethodJumpTable();
                std::map<std::string, clouddoc_handler_fn> handlers;
            };
            static FileMethodJumpTable fileMethodJumpTable;

            // find "Cloud Doc" datasetId
            int getDatasetId(u64 &datasetId);

            // get server host:port
            int getServer(std::string &host_port);

            int checkQueryPresent(const std::string &name);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
