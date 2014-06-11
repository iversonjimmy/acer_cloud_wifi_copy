#include "HttpSvc_Ccd_Agent.hpp"

#include "HttpSvc_Ccd_Server.hpp"
#include "HttpSvc_Utils.hpp"

#include <log.h>

#include <vpl_th.h>
#include <vplu_mutex_autolock.hpp>

HttpSvc::Ccd::Agent::Agent(Server *server, u64 userId)
    : server(Utils::CopyPtr(server)), userId(userId)  // REFCOUNT(Server,BackRef)
{
    VPLMutex_Init(&mutex);
}

HttpSvc::Ccd::Agent::~Agent()
{
    if (RefCounter > 0) {
        LOG_CRITICAL("Agent[%p]: RefCount %d", this, int(RefCounter));
    }

    VPLMutex_Destroy(&mutex);

    Utils::DestroyPtr(server);  // REFCOUNT(Server,BackRef)
}
