#ifndef __HTTPSVC_HSTOVCSTRANSLATOR_HPP__
#define __HTTPSVC_HSTOVCSTRANSLATOR_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_http2.hpp>

#include <map>
#include <string>
#include <vector>

class HttpStream;

namespace HttpSvc {
        class HsToVcsTranslator {
        public:
            HsToVcsTranslator(HttpStream *hs, u64 datasetId_in);
            HsToVcsTranslator(HttpStream *hs, const std::string datasetId_in);
            ~HsToVcsTranslator();
            int Run();
            
            s32 sendCb(VPLHttp2 *http, char *buf, u32 size);
        private:
            enum DatasetCategory {
                DATASET_CATEGORY_NOTES = 1
            };
            
            VPL_DISABLE_COPY_AND_ASSIGN(HsToVcsTranslator);
            
            HttpStream *const hs;
            u64 datasetId;
            std::string operationPattern;
            std::string parameterPattern;
            std::string servicePrefix;
            std::string categoryName;
            bool hasNoparameters;
            DatasetCategory datasetCategory;
            
            std::vector<std::string> uri_tokens;
            
            typedef int (HsToVcsTranslator::*vcs_translator_fn)();
            
            int run_rf();
            
            class ServiceJumpTable {
            public:
                ServiceJumpTable();
                std::map<std::string, vcs_translator_fn> handlers;
            };
            static ServiceJumpTable serviceJumpTable;
            
            int run_dir();
            int run_file();
            int run_filemetadata();
            
            class ObjJumpTable {
            public:
                ObjJumpTable();
                std::map<std::string, vcs_translator_fn> handlers;
            };
            static ObjJumpTable objJumpTable;
            
            int run_dir_get();
            int run_dir_post();
            int run_dir_delete();
            
            class DirMethodJumpTable {
            public:
                DirMethodJumpTable();
                std::map<std::string, vcs_translator_fn> handlers;
            };
            static DirMethodJumpTable dirMethodJumpTable;
            
            int run_file_get();
            int run_file_post();
            int run_file_delete();
            
            class FileMethodJumpTable {
            public:
                FileMethodJumpTable();
                std::map<std::string, vcs_translator_fn> handlers;
            };
            static FileMethodJumpTable fileMethodJumpTable;
            
            // get server host:port
            int getServer(std::string &host_port);
            
            int checkQueryPresent(const std::string &name);
            
            void getParentPath(const std::string &path, std::string &parentDir);
            
            int removeReqHeaderFromStream();
            
            int getCompId(const std::string &pathString, u64 *compId_out);
        };
} // namespace HttpSvc

#endif // incl guard
