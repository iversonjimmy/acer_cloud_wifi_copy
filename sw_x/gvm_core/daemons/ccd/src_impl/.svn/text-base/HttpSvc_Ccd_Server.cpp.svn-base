#include "HttpSvc_Ccd_Server.hpp"

#include "HttpSvc_Ccd_Agent.hpp"
#include "HttpSvc_Ccd_AsyncAgent.hpp"
#include "HttpSvc_Ccd_Handler_rf.hpp"
#include "HttpSvc_Ccd_ListenAgent.hpp"
#include "HttpSvc_Ccd_SocketAgent.hpp"
#include "HttpSvc_Utils.hpp"

#include "cache.h"
#include "ccd_core.h"
#include "ccd_features.h"
#include "config.h"
#include "RouteManager.hpp"
#include "virtual_device.hpp"

#include <log.h>
#include <LocalInfo.hpp>
#include <ts_client.hpp>
#include <ts_ext_server.hpp>

#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

#include <cassert>

namespace HttpSvc {
namespace Ccd {

static s32 getRouteInfo(u64 userId, u64 deviceId, TsRouteInfo *routeInfo)
{
    return RouteManager::Instance().getRouteInfo(userId, deviceId, (RouteInfo*)routeInfo);
}

} // namespace Ccd
} // namespace HttpSvc

HttpSvc::Ccd::Server::Server(u64 userId, Ts2::LocalInfo *localInfo)
    : userId(userId), localInfo(localInfo), asyncAgent(NULL), listenAgent(NULL)
{
    VPLMutex_Init(&mutex);

    LOG_INFO("Server[%p]: Created for user["FMTu64"]", this, userId);
}

HttpSvc::Ccd::Server::~Server()
{
    if (RefCounter > 0) {
        LOG_WARN("Server[%p]: RefCount %d", this, int(RefCounter));
    }

    VPLMutex_Destroy(&mutex);

    LOG_INFO("Server[%p]: Destroyed", this);
}

extern void ts_shim_set_enableTs(int _enableTs);

int HttpSvc::Ccd::Server::Start(VPLNet_addr_t serviceAddrSpec, VPLNet_port_t servicePortSpec)
{
    int err = 0;

    {
        // TEMPORARY during 2.6-2.7
        ts_shim_set_enableTs(__ccdConfig.enableTs);

        // New code for 2.7
        u64 deviceId = VirtualDevice_GetDeviceId();
        u32 instanceId = localInfo->GetInstanceId();
        std::string errmsg;

        LOG_INFO("TS_Init for UDI <"FMTu64","FMTu64","FMTu32">", userId, deviceId, instanceId);
        TSError_t tserr = TS_Init(
                                  userId, deviceId, instanceId,
                                  getRouteInfo,
                                  VPLTime_FromSec(__ccdConfig.dinConnIdleTimeout),
                                  VPLTime_FromSec(__ccdConfig.p2pConnIdleTimeout),
                                  VPLTime_FromSec(__ccdConfig.prxConnIdleTimeout),
                                  __ccdConfig.clearfiMode,
                                  localInfo,
                                  errmsg);
        if (tserr != TS_OK) {
            LOG_ERROR("Server[%p]: TS_Init failed: err %d, msg %s", this, tserr, errmsg.c_str());
            err = (int)tserr;
            goto end;
        }
        if (__ccdConfig.enableTs & CONFIG_ENABLE_TS_INIT_TS_EXT) {
            tserr = TSExtServer_Init(errmsg);
            if (tserr != TS_OK) {
                LOG_ERROR("Server[%p]: TSExtClient_Init failed: err %d, msg %s", this, tserr, errmsg.c_str());
                err = (int)tserr;
                goto end;
            }
        }
    }

    {
        AsyncAgent *_asyncAgent = NULL;
        {
            MutexAutoLock lock(&mutex);
            if (!asyncAgent) {
                asyncAgent = Utils::CopyPtr(new (std::nothrow) AsyncAgent(this, userId));  // REFCOUNT(AsyncAgent,InitialCopy)
                if (!asyncAgent) {
                    LOG_ERROR("Server[%p]: No memory to create AsyncAgent obj", this);
                    err = CCD_ERROR_NOMEM;
                    goto end;
                }
                AddAgent(asyncAgent);
                // make a copy so we can call Start() without holding the mutex
                _asyncAgent = Utils::CopyPtr(asyncAgent);  // REFCOUNT(AsyncAgent,ToCallStart)
            }
            if (_asyncAgent) {
                Handler_rf::SetAsyncAgent(_asyncAgent);
                _asyncAgent->Start();
                Utils::DestroyPtr(_asyncAgent);  // REFCOUNT(AsyncAgent,ToCallStart)
            }
        }
    }

    {
        ListenAgent *_listenAgent = NULL;
        {
            MutexAutoLock lock(&mutex);
            if (!listenAgent) {
                listenAgent = Utils::CopyPtr(new (std::nothrow) ListenAgent(this, userId, serviceAddrSpec, servicePortSpec));  // REFCOUNT(ListenAgent,InitialCopy)
                if (!listenAgent) {
                    LOG_ERROR("Server[%p]: No memory to create ListenAgent obj", this);
                    err = CCD_ERROR_NOMEM;
                    goto end;
                }
                AddAgent(listenAgent);
                // make a copy so we can call Start() without holding the mutex
                _listenAgent = Utils::CopyPtr(listenAgent);  // REFCOUNT(ListenAgent,ToCallStart)
            }
        }
        if (_listenAgent) {
            _listenAgent->Start();
            Utils::DestroyPtr(_listenAgent);  // REFCOUNT(ListenAgent,ToCallStart)
        }
    }

 end:
    if (err)
        AsyncStop(false);

    return err;
}

int HttpSvc::Ccd::Server::AsyncStop(bool isUserLogout)
{
    Handler_rf::SetAsyncAgent(NULL);

    {
        VPLMutex_Lock(&mutex);

        Utils::DestroyPtr(asyncAgent);  // REFCOUNT(AsyncAgent,InitialCopy)
        asyncAgent = NULL;
        Utils::DestroyPtr(listenAgent);  // REFCOUNT(ListenAgent,InitialCopy)
        listenAgent = NULL;

        while (!agents.empty()) {
            std::set<Agent*>::iterator it = agents.begin();
            Agent *_agent = *it;  // Transferred reference count.
            agents.erase(it);
            VPLMutex_Unlock(&mutex);
            _agent->AsyncStop(isUserLogout);
            Utils::DestroyPtr(_agent);  // REFCOUNT(Agent,CopyInSetOfAgents)
            VPLMutex_Lock(&mutex);
        }
        VPLMutex_Unlock(&mutex);
    }

    if (__ccdConfig.enableTs & CONFIG_ENABLE_TS_INIT_TS_EXT) {
        TSExtServer_Stop();
    }
    TS_Shutdown();

    return 0;
}

VPLNet_port_t HttpSvc::Ccd::Server::GetServicePort() const
{
    ListenAgent *_listenAgent = NULL;
    {
        MutexAutoLock lock(&mutex);
        if (!listenAgent) {
            return VPLNET_PORT_INVALID;
        }
        // Make a copy so we can call GetServicePort() without holding the mutex.
        _listenAgent = Utils::CopyPtr(listenAgent);  // REFCOUNT(ListenAgent,ToCallGetServicePort)
    }
    assert(_listenAgent != NULL);

    VPLNet_port_t port = _listenAgent->GetServicePort();
    Utils::DestroyPtr(_listenAgent);  // REFCOUNT(ListenAgent,ToCallGetServicePort)
    return port;
}

int HttpSvc::Ccd::Server::AddAgent(Agent *agent)
{
    if (agent == NULL) {
        LOG_WARN("Server[%p]: Null Agent", this);
        return CCD_ERROR_PARAMETER;
    }

    MutexAutoLock lock(&mutex);

    agents.insert(Utils::CopyPtr(agent));  // REFCOUNT(Agent,CopyInSetOfAgents)

    return 0;
}

int HttpSvc::Ccd::Server::DropAgent(Agent *agent)
{
    if (agent == NULL) {
        LOG_WARN("Server[%p]: Null Agent", this);
        return CCD_ERROR_PARAMETER;
    }

    MutexAutoLock lock(&mutex);

    std::set<Agent*>::iterator it = agents.find(agent);
    if (it != agents.end()) {
        Agent *_agent = *it;
        agents.erase(it);
        Utils::DestroyPtr(_agent);  // REFCOUNT(Agent,CopyInSetOfAgents)
    }

    return 0;
}
