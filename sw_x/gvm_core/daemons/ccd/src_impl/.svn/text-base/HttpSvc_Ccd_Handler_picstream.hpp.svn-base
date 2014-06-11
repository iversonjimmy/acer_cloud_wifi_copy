#ifndef __HTTPSVC_CCD_HANDLER_PICSTREAM_HPP__
#define __HTTPSVC_CCD_HANDLER_PICSTREAM_HPP__

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
        class Handler_picstream : public Handler {
        public:
            Handler_picstream(HttpStream *hs);
            ~Handler_picstream();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_picstream);

            std::vector<std::string> uri_tokens;

            typedef int (Handler_picstream::*picstream_handler_fn)();

            int run_file();
            int run_fileinfo();

            class ObjJumpTable {
            public:
                ObjJumpTable();
                std::map<std::string, picstream_handler_fn> handlers;
            };
            static ObjJumpTable objJumpTable;

            int run_file_get();
            int run_file_delete();

            class FileMethodJumpTable {
            public:
                FileMethodJumpTable();
                std::map<std::string, picstream_handler_fn> handlers;
            };
            static FileMethodJumpTable fileMethodJumpTable;

            // find "PicStream" datasetId
            int getDatasetId(u64 &datasetId);

            // get server host:port
            int getServer(std::string &host_port);

            int checkQueryPresent(const std::string &name);

            int file2httpstream(std::string filePath, HttpStream *hs);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
