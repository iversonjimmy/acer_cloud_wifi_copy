#ifndef __HTTPSVC_TSTOHSADAPTER_HPP__
#define __HTTPSVC_TSTOHSADAPTER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_thread.h>

#include <string>

#include <ts_server.hpp>

class HttpStream;

namespace HttpSvc {
    class TsToHsAdapter {
    public:
        TsToHsAdapter(TSServiceRequest_t *tssr);
        ~TsToHsAdapter();
        int Run();
    private:
        VPL_DISABLE_COPY_AND_ASSIGN(TsToHsAdapter);

        TSServiceRequest_t *tssr;
    };
} // namespace HttpSvc

#endif // incl guard
