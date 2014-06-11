#ifndef __HTTPSERVICE_HPP__
#define __HTTPSERVICE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_th.h>

namespace HttpSvc {
    namespace Ccd {
        class Server;
    }
}

namespace Ts2 {
class LocalInfo;
}

/// Responsible for functioning as a local HTTP server for receiving requests from
/// other applications on the localhost.
class HttpService
{
public:
    HttpService();
    ~HttpService();

    /// Configure stream service. Parameters take effect on #start().
    /// @param userId Service started for this user
    /// @param localDeviceId This device's ID.
    /// @param userServerAddr Interface address for client connections
    /// @param userServerPort TCP port to use when listening for connections
    void configure_service(u64 userId,
                           u64 localDeviceId,
                           VPLNet_addr_t userServerAddr,
                           VPLNet_port_t userServerPort,
                           Ts2::LocalInfo *localInfo);

    /// Start streaming service.
    /// @return 0 on successful start or already running.
    /// @return -1 on fail to start.
    int start(); 

    /// Stop streaming service.
    /// @return 0 on successful stop or not started.
    /// @return -1 on fail to stop.
    int stop(bool userLogout);

    /// Check if stream service is running.
    bool is_running() const;

    /// If a stream server (PSN) is unlinked, stop allowing streaming access
    /// to the server for streaming requests.
    /// @param deviceId Device ID of the stream server removed (unlinked).
    void remove_stream_server(u64 deviceId);

    /// If a stream server (PSN) is added or has a change, signal 
    /// update needed to HttpServices.
    /// @param deviceId Device ID of the stream server added or changed.
    /// @param force_drop_conns Force drop of all proxy connections.
    /// @note For now, stream server addresses are fetched direct from VSDS.
    ///       This will change eventually.
    void add_or_change_stream_server(u64 deviceId, bool force_drop_conns);

    /// Return port stream service is using to listen for client connections.
    /// Returns VPLNET_PORT_INVALID if not listening (service not started).
    VPLNet_port_t stream_listening_port() const;
    
    /// On init, update stream servers from CCD cache.
    /// @param force_drop_conns Force drop of all proxy connections.
    void update_stream_servers(bool force_drop_conns);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(HttpService);

    mutable VPLMutex_t mutex;

    u64 userId;
    u64 localDeviceId;
    VPLNet_addr_t userServerAddr;
    VPLNet_port_t userServerPort;
    Ts2::LocalInfo *localInfo;

    HttpSvc::Ccd::Server *server;
};

#endif // include guard
