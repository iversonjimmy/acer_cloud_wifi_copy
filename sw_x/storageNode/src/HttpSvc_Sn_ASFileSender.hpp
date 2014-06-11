#ifndef __HTTPSVC_SN_ASFILESENDER_HPP__
#define __HTTPSVC_SN_ASFILESENDER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_file.h>
#include <vpl_fs.h>

#include <string>
#include <utility>
#include <vector>


class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class MediaFile;

        class ASFileSender {
        public:
            ASFileSender(MediaFile *mf, u64 mtime, const std::string& hash, HttpStream *hs);
            ~ASFileSender();
            int Send();

        private:
            MediaFile *mf;
            u64 originalModTime;
            std::string originalHash;
            bool tolerateFileModification;
            HttpStream *hs;
            std::vector<std::pair<std::string, std::string> > ranges;
            int processRangeSpec();
            int sendHttpHeaders();
            int sendWholeFile();

        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard

