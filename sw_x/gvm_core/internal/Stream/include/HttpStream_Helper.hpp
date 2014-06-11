#ifndef __HTTP_STREAM_HELPER_HPP__
#define __HTTP_STREAM_HELPER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplu_sstr.hpp>

#include <string>

class HttpStream;

/// For use as the content param for #SetCompleteResponse().
#define JSON_ERRMSG(msg_)  SSTR("{\"errMsg\":\"" << msg_ << "\"}")

namespace HttpStream_Helper {
    int AddStdRespHeaders(HttpStream *hs);
    int SetCompleteResponse(HttpStream *hs, int code);
    int SetCompleteResponse(HttpStream *hs, int code, const std::string &content, const std::string &contentType);
}

#endif // incl guard
