#ifndef __HTTPSVC_CCD_HANDLER_HPP__
#define __HTTPSVC_CCD_HANDLER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

class vss_server;

class HttpStream;

namespace HttpSvc {
    namespace Sn {

        /*
          A Handler obj processes an HTTP request.
          The Handler class is essentially just an interface.
          The actual processing is coded in a subclass of Handler.
          There will be one Handler subclass for each service.

          A Handler obj is designed only for single-use;
          do not try to reuse, or modify code to allow reuse.
        */
        class Handler {
        public:
            Handler(HttpStream *hs) : hs(hs) {}

            virtual ~Handler() {}

            /* Run() processes the HTTP request that is encapsulated in the HttpStream obj.
               It is responsible for writing the response into the HttpStream obj
               and returning 0, even in cases were the request failed, e.g., 404.
               A non-0 return value is reserved for extreme cases,
               and will likely result in the caller dropping the connection with the App.
               A valid use case of returning non-0:
               HTTP response header (with Content-Length header) was already sent
               but the response body got truncated in the middle and got short.
             */
            virtual int Run() = 0;

            static void SetVssServer(vss_server *server);

            static bool IsVssServerNull();

        protected:
            HttpStream *const hs;

            static vss_server *vssServer;
        };
    } // namespace Sn
} // namespace HttpSvc

#endif // incl guard

