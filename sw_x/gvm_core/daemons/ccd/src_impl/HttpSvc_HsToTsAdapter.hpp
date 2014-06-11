#ifndef __HTTPSVC_HSTOTSADAPTER_HPP__
#define __HTTPSVC_HSTOTSADAPTER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_thread.h>

#include <string>

#include "ccd_features.h"

#include <ts_client.hpp>

class HttpStream;

namespace HttpSvc {
    class HsToTsAdapter {
    public:
        HsToTsAdapter(HttpStream *hs);
        ~HsToTsAdapter();
        int Run();
    private:
        VPL_DISABLE_COPY_AND_ASSIGN(HsToTsAdapter);

        HttpStream *const hs;
        bool runCalled;
        int bailout;  // Error code that caused bailout to happen.
                      // If non-zero, threads should exit ASAP.
                      // If negative, propagate error so connection will be dropped.
        bool sendDone;
        TSIOHandle_t ts;

        static VPLTHREAD_FN_DECL helperThreadMain(void *param);
        void helperThreadMain();

        int completeWrite(const char *data, size_t datasize);

        static const std::string httpHeaderBodyBoundary;
    };
} // namespace HttpSvc

#endif // incl guard
