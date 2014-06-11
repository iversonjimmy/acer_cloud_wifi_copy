#ifndef __HTTPSVC_CCD_HANDLER_MM_HPP__
#define __HTTPSVC_CCD_HANDLER_MM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_util.hpp>

#include "HttpSvc_Ccd_Handler.hpp"

#include <map>
#include <string>

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler_mm : public Handler {
        public:
            Handler_mm(HttpStream *hs);
            ~Handler_mm();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_mm);

            bool trySatisfyFromCache(HttpStream *hs);
            int prepareRequest(HttpStream *hs);
            int getFromStorageServer(HttpStream *hs);
            int getFromVcsServer(HttpStream *hs,
                                 const std::string& destPath,
                                 std::string& destFilename_out,
                                 u64& compId_out,
                                 u64& revisionId_out);
            /// Always sets hs->SetStatusCode before returning.
            /// Returns negative if connection should be closed.
            int forwardDownloadedVcsFile(HttpStream *hs,
                                         const std::string& pathToForward);
            bool isLocalReservedContent(const std::string& uri);
            int tryToSendLocalReservedContent(HttpStream *hs,
                                              bool isHead,
                                              bool& is_replied_out);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
