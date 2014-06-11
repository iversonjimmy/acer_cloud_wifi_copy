#include "HttpService.hpp"

#include "HttpSvc_Ccd_Server.hpp"
#include "HttpSvc_Utils.hpp"

#include <log.h>
#include <ts_client.hpp>

#include <vplex_assert.h>
#include <vplu_mutex_autolock.hpp>

HttpService::HttpService() :
    userId(0),
    userServerAddr(VPLNET_ADDR_LOOPBACK),
    userServerPort(VPLNET_PORT_ANY),
    localInfo(NULL),
    server(NULL)
{
    VPLMutex_Init(&mutex);
}

HttpService::~HttpService()
{
    // HttpService client code should have called stop() before destroying it.
    ASSERT(server == NULL);

    VPLMutex_Destroy(&mutex);
}

void HttpService::configure_service(u64 userId,
                                    u64 localDeviceId,
                                    VPLNet_addr_t userServerAddr,
                                    VPLNet_port_t userServerPort,
                                    Ts2::LocalInfo *localInfo)
{
    this->userId         = userId;
    this->localDeviceId  = localDeviceId;
    this->userServerAddr = userServerAddr;
    this->userServerPort = userServerPort;
    this->localInfo      = localInfo;
}

bool HttpService::is_running() const
{
    MutexAutoLock lock(&mutex);

    return server != NULL;
}

int HttpService::start()
{
    int err = 0;

    HttpSvc::Ccd::Server *_server = NULL;
    {
        MutexAutoLock lock(&mutex);
        if (!server) {
            server = HttpSvc::Utils::CopyPtr(new HttpSvc::Ccd::Server(userId, localInfo));  // REFCOUNT(Server,InitialCopy)
            // make a copy so we can call Start() without holding the mutex
            _server = HttpSvc::Utils::CopyPtr(server);  // REFCOUNT(Server,ToCallStart)
        }
    }
    if (_server) {
        err = _server->Start(userServerAddr, userServerPort);
        HttpSvc::Utils::DestroyPtr(_server);  // REFCOUNT(Server,ToCallStart)
    }

    return err;
}

int HttpService::stop(bool userLogout)
{
    int err = 0;

    HttpSvc::Ccd::Server *_server = NULL;
    {
        MutexAutoLock lock(&mutex);
        if (server) {
            _server = server;  // transfer reference count
            server = NULL;
        }
    }
    if (_server) {
        err = _server->AsyncStop(userLogout);
        HttpSvc::Utils::DestroyPtr(_server);  // REFCOUNT(Server,InitialCopy)
    }
 
   return err;
} 

VPLNet_port_t HttpService::stream_listening_port() const
{
    MutexAutoLock lock(&mutex);

    if (!server) {
        return VPLNET_PORT_INVALID;
    }

    return server->GetServicePort();
}

void HttpService::remove_stream_server(u64 deviceId)
{
    TS_RemoveServerInfo(deviceId);
}

void HttpService::add_or_change_stream_server(u64 deviceId, bool force_drop_conns)
{
    if (userId) {
        TS_UpdateServerInfo(userId, deviceId, force_drop_conns);
    }
}

void HttpService::update_stream_servers(bool force_drop_conns)
{
    if (userId) {
        TS_RefreshServerInfo(userId, force_drop_conns);
    }
}
