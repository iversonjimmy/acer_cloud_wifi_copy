#ifndef __HTTPSVC_SN_HANDLER_VCS_ARCHIVE_HPP__
#define __HTTPSVC_SN_HANDLER_VCS_ARCHIVE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <map>
#include <string>
#include <vector>
#include <set>

#include "dataset.hpp"

#include "HttpSvc_Sn_Handler.hpp"

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class Handler_vcs_archive : public Handler {
        public:
            Handler_vcs_archive(HttpStream *hs);
            ~Handler_vcs_archive();
            int Run();

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_vcs_archive);
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard
