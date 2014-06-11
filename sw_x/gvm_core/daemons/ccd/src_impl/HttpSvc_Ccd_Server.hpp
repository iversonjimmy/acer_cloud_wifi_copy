#ifndef __HTTPSVC_CCD_SERVER_HPP__
#define __HTTPSVC_CCD_SERVER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_th.h>

#include <set>

#include "HttpSvc_AtomicCounter.hpp"

namespace Ts2 {
class LocalInfo;
}

namespace HttpSvc {
    namespace Ccd {
        class Agent;
        class AsyncAgent;
        class ListenAgent;

        class Server {
        public:
            Server(u64 userId, Ts2::LocalInfo *localInfo);
            ~Server();

            // Start the server, listening at given addr:port
            int Start(VPLNet_addr_t serviceAddrSpec, VPLNet_port_t servicePortSpec);

            // Instruct the server to stop.
            // Note that it does not wait for the server to actually stop.
            int AsyncStop(bool isUserLogout);

            VPLNet_port_t GetServicePort() const;

            // Add/Drop Agent obj.
            // Caller must guarantee that the ptr and the Agent obj at the end of the ptr are valid during the call.
            int AddAgent(Agent *agent);
            int DropAgent(Agent *agent);

            mutable AtomicCounter RefCounter;

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(Server);

            mutable VPLMutex_t mutex;

            const u64 userId;
            Ts2::LocalInfo *const localInfo;

            std::set<Agent*> agents;
            AsyncAgent *asyncAgent;  // convenience ptr
            ListenAgent *listenAgent;  // convenience ptr
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard
