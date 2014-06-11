#ifndef __HTTPSVC_SN_DISPATCHER_HPP__
#define __HTTPSVC_SN_DISPATCHER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <map>
#include <new>
#include <string>

class HttpStream;

namespace HttpSvc {
    namespace Sn {
        class Dispatcher {
        public:
            static int Dispatch(HttpStream *hs);
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard
