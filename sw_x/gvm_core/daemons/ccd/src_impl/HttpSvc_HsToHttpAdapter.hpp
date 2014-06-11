#ifndef __HTTPSVC_HSTOHTTPADAPTER_HPP__
#define __HTTPSVC_HSTOHTTPADAPTER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vplex_http2.hpp>

#include <map>
#include <string>

class HttpStream;

namespace HttpSvc {
    class HsToHttpAdapter {
    public:
        HsToHttpAdapter(HttpStream *hs);
        ~HsToHttpAdapter();
        int Run();

        s32 recvCb(VPLHttp2 *http, const char *buf, u32 size);
        s32 sendCb(VPLHttp2 *http, char *buf, u32 size);

    private:
        VPL_DISABLE_COPY_AND_ASSIGN(HsToHttpAdapter);

        HttpStream *const hs;
        bool runCalled;
        bool bailout;  // true iff threads should exit asap

        VPLHttp2 http;
        bool recvCb_firsttime;

        typedef int (HsToHttpAdapter::*handler_fn)();

        int run_delete();
        int run_get();
        int run_post();
        int run_put();

        class MethodJumpTable {
        public:
            MethodJumpTable();
            std::map<std::string, handler_fn> handlers;
        };
        static MethodJumpTable methodJumpTable;

        int copyReqHeader();
        int copyRespHeader();
        int tryCopyReqHeader(const std::string &name);
        int tryCopyRespHeader(const std::string &name);

        int removeReqHeaderFromStream();

        bool send_resp_in_chunks;  // Used in GET method.
    };
} // namespace HttpSvc

#endif // incl guard
