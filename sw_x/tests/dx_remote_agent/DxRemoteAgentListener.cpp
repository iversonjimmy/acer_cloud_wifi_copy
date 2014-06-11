#include "DxRemoteAgentListener.hpp"
#include "DxRemoteAgentManager.hpp"
#include "dx_remote_agent_util.h"
#include <vplex_http2.hpp>
#if defined(VPL_PLAT_IS_WINRT)
#include "dx_remote_agent_util_winrt.h"
#endif
#include "log.h"

#include <vpl_socket.h>
#include <vpl_thread.h>

DxRemoteAgentListener::DxRemoteAgentListener()
{
    VPL_Init();
    VPLHttp2::Init();
}

DxRemoteAgentListener::~DxRemoteAgentListener()
{
    VPLHttp2::Shutdown();
    VPL_Quit();
}

static VPLTHREAD_FN_DECL listenMain(VPLThread_arg_t arg)
{
    DxRemoteAgentListener *listener = (DxRemoteAgentListener *)arg;
    
    listener->Run();
    delete listener;

    return VPLTHREAD_RETURN_VALUE;
}

void DxRemoteAgentListener::RunDetached()
{
    int rc = 0;
    VPLThread_attr_t attrs;
    VPLThread_AttrInit(&attrs);
    VPLThread_AttrSetDetachState(&attrs, VPL_TRUE);

    rc = VPLDetachableThread_Create(NULL, listenMain, this, &attrs, "DxRemoteAgentListener");
    if(rc != VPL_OK) {
        LOG_ERROR("Listener thread create failed: (%d).",rc);
    }    
}

static VPLTHREAD_FN_DECL startManager(VPLThread_arg_t arg)
{
    DxRemoteAgentManager *mgr = (DxRemoteAgentManager *)arg;
    
    mgr->Run();
    delete mgr;

    return VPLTHREAD_RETURN_VALUE;    
}

int DxRemoteAgentListener::Run()
{
    VPLSocket_addr_t sock_addr;
    int yes = 1;
    VPLSocket_t socket = VPLSOCKET_INVALID;
    VPLSocket_t clientSocket;
    int rc = 0;
    int tryBind = 5;
    unsigned short bindPort = 0;

    VPLThread_attr_t attrs;
    VPLThread_AttrInit(&attrs);
    VPLThread_AttrSetDetachState(&attrs, VPL_TRUE);

    // Try up to 5 times to bind a port. On success, record the port used to agent config.
    do {
        socket = VPLSocket_Create( VPL_PF_INET, VPLSOCKET_STREAM, /*blocking socket*/0 );
        if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
            LOG_ERROR("Failed to create listening socket.");
            goto exit;
        }
        
        // Set desired socket options. No harm if failed.
#ifndef VPL_PLAT_IS_WINRT
        VPLSocket_SetSockOpt( socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR, (void*)&yes, sizeof(yes) );
#endif
        VPLSocket_SetSockOpt( socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, (void*)&yes, sizeof(yes) );
        
        bindPort = getFreePort();
        sock_addr.port = VPLNet_port_hton(bindPort);
        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;

        rc = VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr));
        if(rc != VPL_OK) {
            LOG_WARN("Failed to bind socket to "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t" (%d)",
                     VAL_VPLNet_addr_t(sock_addr.addr), VPLNet_port_ntoh(sock_addr.port), rc);
            bindPort = 0;
            VPLSocket_Close(socket);
            socket = VPLSOCKET_INVALID;
        }
        else {
            // learn the port used
            bindPort = VPLNet_port_ntoh(VPLSocket_GetPort(socket));
        }
    } while(--tryBind && VPLSocket_Equal(socket, VPLSOCKET_INVALID));
    if(bindPort == 0) {
        LOG_ERROR("Failed to bind socket. Agent quitting.");
        goto exit;
    }
    if((rc = recordPortUsed(bindPort)) != 0) {
        LOG_ERROR("Failed to record port used (%d). Agent quitting.", rc);
        goto exit;
    }

    rc = VPLSocket_Listen(socket, 10);
    if(rc != VPL_OK) {
        LOG_ERROR("Failed to listen on socket (%d)", rc);
        goto exit;
    }

    // TODO: Do these belong here, or in the platform-specific upper layer?
    #if defined(VPL_PLAT_IS_WINRT)
    create_picstream_path();
    #endif
    #if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    clean_cc();
    startccd("0");
    #endif

    LOG_ALWAYS("dx_remote_agent start to listen on port %d", bindPort);
    while((rc = VPLSocket_Accept(socket, NULL, 0, &clientSocket)) == VPL_OK)
    {
        DxRemoteAgentManager* mgr = new DxRemoteAgentManager(clientSocket);
        LOG_ALWAYS("Launching manager thread for socket "FMT_VPLSocket_t".",
                VAL_VPLSocket_t(clientSocket));
        rc = VPLDetachableThread_Create(NULL, startManager, mgr, &attrs, "DxRemoteAgentManager");
        if(rc != VPL_OK) {
            LOG_ERROR("Manager thread for socket "FMT_VPLSocket_t" create failed: (%d).",
                      VAL_VPLSocket_t(clientSocket), rc);
            delete mgr;
            VPLSocket_Close(clientSocket);
        }
    }
    LOG_ALWAYS("Listening ended on VPLSocket_Accept() return (%d).", rc);

 exit:
    if(!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(socket);
    }
    VPLThread_AttrDestroy(&attrs);

    return rc;
}
