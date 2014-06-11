#ifndef __HTTPSVC_SN_HANDLER_RF_HPP__
#define __HTTPSVC_SN_HANDLER_RF_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <map>
#include <string>
#include <vector>
#include <set>

#include "dataset.hpp"

#include "HttpSvc_Sn_Handler.hpp"
#include "rf_search_manager.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class Handler_rf : public Handler {
        public:
            Handler_rf(HttpStream *hs);
            ~Handler_rf();
            int Run();

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_rf);

            std::vector<std::string> uri_tokens;
            u64 datasetId;
            dataset *ds;
            std::string path;
            std::string search_func;

            // Map representing the query args of a uri as shown in the example below.
            // www.example.com?queryArg=1&queryArg=2&queryArg=3
            std::map<std::string, std::string> uriQueryArgs;

            typedef int (Handler_rf::*subhandler_func)();

            int handle_dir();
            int handle_file();
            int handle_filemetadata();
            int handle_whitelist();
            int handle_search();
            int handle_dirmetadata();

            int handle_dir_DELETE();
            int handle_dir_GET();
            int handle_dir_POST();
            int handle_dir_PUT();

            int handle_file_DELETE();
            int handle_file_GET();
            int handle_file_POST();

            int handle_filemetadata_GET();
            int handle_filemetadata_PUT();

            int handle_whitelist_GET();

            int handle_dir_POST_copy(const std::string &copyFrom);
            int handle_dir_POST_move(const std::string &moveFrom);
            int handle_file_POST_copy(const std::string &copyFrom);
            int handle_file_POST_move(const std::string &moveFrom);
            int handle_file_POST_upload();
            int handle_search_POST_begin();
            int handle_search_POST_get();
            int handle_search_POST_end();

            int handle_dirmetadata_GET();

            class JumpTableMap {
            public:
                JumpTableMap(void (*initializer)(std::map<std::string, subhandler_func> &)) {
                    initializer(m);
                }
                typedef std::map<std::string, subhandler_func>::const_iterator const_iterator;
                const_iterator find(const std::string &key) const {
                    return m.find(key);
                }
                const_iterator end() const {
                    return m.end();
                }
            protected:
                std::map<std::string, subhandler_func> m;
            };

            static void initJumpTableMap_byObj(std::map<std::string, subhandler_func> &m);
            static void initJumpTableMap_dir_byMethod(std::map<std::string, subhandler_func> &m);
            static void initJumpTableMap_file_byMethod(std::map<std::string, subhandler_func> &m);
            static void initJumpTableMap_filemetadata_byMethod(std::map<std::string, subhandler_func> &m);
            static void initJumpTableMap_whitelist_byMethod(std::map<std::string, subhandler_func> &m);
            static void initJumpTableMap_dirmetadata_byMethod(std::map<std::string, subhandler_func> &m);

            static const JumpTableMap jumpTableMap_byObj;
            static const JumpTableMap jumpTableMap_dir_byMethod;
            static const JumpTableMap jumpTableMap_file_byMethod;
            static const JumpTableMap jumpTableMap_filemetadata_byMethod;
            static const JumpTableMap jumpTableMap_whitelist_byMethod;
            static const JumpTableMap jumpTableMap_dirmetadata_byMethod;
        };
    } // namespace Sn
} // namespace HttpSvc

namespace HttpSvc {
    namespace Sn {
        namespace Handler_rf_Helper
        {   // These variables are defined in HttpSvc_Sn_Dispatcher.cpp

            class AccessControlLocker {
            public:
                AccessControlLocker() {
                    VPLMutex_Lock(&mutex);
                }
            
                ~AccessControlLocker() {
                    VPLMutex_Unlock(&mutex);
                }
                static VPLMutex_t mutex;
            };
            extern std::set<std::string> blackList;
            extern std::set<std::string> whiteList;
            extern std::set<std::string> userWhiteList;
            extern std::multimap<std::string,ccd::RemoteFileAccessControlDirSpec> acDirs;
            int updateRemoteFileAccessControlDir(dataset *ds,
                                                const ccd::RemoteFileAccessControlDirSpec &dir, 
                                                bool add);
            int checkAccessControlList(dataset *ds, const std::string &path);

            void winPathToRemoteFilePath(const std::string& inputWinPath,
                                         std::string& rfPath_out);
            bool isShortcut(const std::string& path, VPLFS_file_type_t type);

            // Initialized by sw_x/storageNode/src/vss_server.cpp :: vss_server::start()
            // Same lifetime as vss_server.
            extern RemoteFileSearchManager* rfSearchManager;
        }
    }
}

#endif // incl guard
