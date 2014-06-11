#ifndef __HTTPSVC_CCD_SOCKETAGENT_HPP__
#define __HTTPSVC_CCD_SOCKETAGENT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <string>

#include "HttpSvc_Ccd_Agent.hpp"
#include "HttpSvc_Utils.hpp"

namespace HttpSvc {
    namespace Ccd {
        class Server;

        class SocketAgent : public Agent {
        public:
            SocketAgent(Server *server, u64 userId, VPLSocket_t socket);
            ~SocketAgent();

            int Start();
            int AsyncStop(bool isUserLogout) { return AsyncStop(); }
            int AsyncStop();  // SocketAgent specific

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(SocketAgent);

            // args supplied in ctor
            VPLSocket_t socket;

            bool isSocketClosed;
            int closeSocket();

            Utils::ThreadState threadState;
            Utils::CancelState cancelState;
            std::string getStateStr() const;
            int getStateNum() const;

            VPLDetachableThreadHandle_t thread;
            void threadMain();
            static VPLTHREAD_FN_DECL threadMain(void* param);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
