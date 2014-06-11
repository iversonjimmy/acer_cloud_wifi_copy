#ifndef __HTTPSVC_CCD_AGENT_HPP__
#define __HTTPSVC_CCD_AGENT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_th.h>

#include "HttpSvc_AtomicCounter.hpp"

namespace HttpSvc {
    namespace Ccd {
        class Server;

        /*
          An Agent obj is an obj with a thread.
          The thread is spawned when Start() is called.
          Calling AsyncStop() will instruct the obj to have the thread exit at the earliest possible time.
          It is important to note that the call does not wait for the thread to exit.
        */
        class Agent {
        public:
            Agent(Server *server, u64 userId);
            virtual ~Agent();

            // Spawn a thread and start work.
            virtual int Start() = 0;

            // Instruct the thread to exit.
            virtual int AsyncStop(bool isUserLogout) = 0;

            mutable AtomicCounter RefCounter;

        protected:
            mutable VPLMutex_t mutex;

            Server *const server;
            const u64 userId;
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
