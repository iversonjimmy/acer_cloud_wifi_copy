#ifndef __HTTPSVC_SN_HANDLER_REMOTE_EXECUTABLE_HPP__
#define __HTTPSVC_SN_HANDLER_REMOTE_EXECUTABLE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_thread.h>

#include <list>
#include <string>
#include <vector>
#include <sstream>

#include "HttpSvc_Sn_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Sn {

        class Handler_rexe : public Handler {
        public:
            Handler_rexe(HttpStream *hs);
            ~Handler_rexe();
            int Run();

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_rexe);

            std::string app_key;
            std::string executable_name;
            std::string absolute_path;
            u64 minimal_version_num;

            std::stringstream stdout_buf_stream;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
            HANDLE read_child_stdout_handle;
#elif defined(LINUX) || defined(CLOUDNODE)
            int read_child_stdout_handle;
#endif // defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

            int helperThreadReturnValue;

            int parseRequest();
            int checkExecutable();
            int runExecutable();

            int writeToBuffer(const char* buf, u32 size);

            // been used to read standard output of launched process.
            static VPLTHREAD_FN_DECL helperThreadMain(void *param);
            void helperThreadMain();

        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard
