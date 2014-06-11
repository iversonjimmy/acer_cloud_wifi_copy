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

#ifndef HTTPSVC_CCD_HANDLER_SHARE_HPP_04_24_2014_
#define HTTPSVC_CCD_HANDLER_SHARE_HPP_04_24_2014_

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
        class Handler_share : public Handler {
        public:
            Handler_share(HttpStream *hs);
            ~Handler_share();
        protected:
            int _Run();
        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Handler_share);
            // General dispatch function
            int get_share_file();
            // Serves preview (smallest) resolution photos
            int get_share_file_handleThumbnailRes(u64 compId, u64 feature);
            // Serves shared resolution.
            int get_share_file_handleSharedRes(u64 compId, u64 feature);

            int utilForwardLocalFile(const std::string& absPathToForward);
            int downloadFromVcsServer(u64 compId,
                                      u64 revision,
                                      u64 datasetId,
                                      const std::string& datasetRelPath,
                                      const VcsCategory& category,
                                      const std::string& absDestPath);
        };
    } // namespace Ccd
} // namespace HttpSvc


#endif // HTTPSVC_CCD_HANDLER_SHARE_HPP_04_24_2014_
