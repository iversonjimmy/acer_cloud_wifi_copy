#ifndef __HTTPSVC_CCD_LISTENAGENT_HPP__
#define __HTTPSVC_CCD_LISTENAGENT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <string>

#include "HttpSvc_Ccd_Agent.hpp"
#include "HttpSvc_AtomicCounter.hpp"
#include "HttpSvc_Utils.hpp"

namespace HttpSvc {
    namespace Ccd {
        class Server;

        class ListenAgent : public Agent {
        public:
            ListenAgent(Server *server, u64 userId, VPLNet_addr_t addrSpec, VPLNet_port_t portSpec);
            ~ListenAgent();

            // Start a thread to listen.
            int Start();

            // Instruct the thread to exit.
            // Note the function does not wait for the thread to exit but only instruct.
            int AsyncStop(bool isUserLogout) { return AsyncStop(); }
            int AsyncStop();

            bool IsRunning() const { return threadState == Utils::ThreadState_Running; }

            // Get port number for service.
            // This is the port used by Apps to send requests.
            VPLNet_port_t GetServicePort() const;

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(ListenAgent);

            // args supplied in ctor
            const VPLNet_addr_t addrSpec;
            const VPLNet_port_t portSpec;

            enum SocketState {
                SocketState_NoSocket = 0,
                SocketState_Listening,
            };
            SocketState socketState;
            const std::string &getSocketStateStr() const;
            Utils::ThreadState threadState;
            Utils::CancelState cancelState;
            std::string getStateStr() const;
            int getStateNum() const;

            VPLSocket_t svcSocket;
            int openSocket(VPLNet_addr_t addr, VPLNet_port_t port);
            int closeSocket();

            VPLDetachableThreadHandle_t main_thread;
            static VPLTHREAD_FN_DECL thread_main(void* param);
            void thread_main();

            int handleNewConnection(VPLSocket_t socket);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
