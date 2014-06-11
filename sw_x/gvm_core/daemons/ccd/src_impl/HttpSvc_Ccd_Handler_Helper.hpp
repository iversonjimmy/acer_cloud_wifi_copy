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

#ifndef __HTTPSVC_CCD_HANDLER_HELPER_HPP__
#define __HTTPSVC_CCD_HANDLER_HELPER_HPP__

#include "vpl_plat.h"
#include "vplex_util.hpp"

#include <map>
#include <string>

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        namespace Handler_Helper {
            int ForwardToServerCcd(HttpStream *hs);

            // Always sets hs->SetStatusCode before returning.
            // Returns negative if connection should be closed.
            int utilForwardLocalFile(HttpStream *hs,
                                     const std::string& absPathToForward,
                                     bool includeMimeTypeHeader,
                                     const std::string& mimeTypeHeader);

            class PhotoMimeMap {
            public:
                PhotoMimeMap();
                const std::string &GetMimeFromExt(const std::string &ext) const;
            private:
                std::map<std::string, std::string, case_insensitive_less> photoMimeMap;
            };
            extern PhotoMimeMap photoMimeMap;

            class FileMimeMap {
            public:
                FileMimeMap();
                const std::string &GetMimeFromExt(const std::string &ext) const;
            private:
                std::map<std::string, std::string, case_insensitive_less> fileMimeMap;
            };
            extern FileMimeMap fileMimeMap;
        }
    }
}

#endif // __HTTPSVC_CCD_HANDLER_HELPER_HPP__

