#ifndef __HTTPSVC_CCD_DISPATCHER_HPP__
#define __HTTPSVC_CCD_DISPATCHER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <map>
#include <new>
#include <string>

class HttpStream;

namespace HttpSvc {
    namespace Ccd {
        class Handler;

        class Dispatcher {
        public:
            static int Dispatch(HttpStream *hs);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
