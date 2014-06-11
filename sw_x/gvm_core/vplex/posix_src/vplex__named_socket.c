//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

//============================================================================
/// @file
/// 
//============================================================================

#include "vplex_named_socket.h"
#include "vplex_ipc_socket.h"
#include "vplex_private.h"

#define DEFAULT_MAX_CONNECTIONS  64

s32
VPLNamedSocket_OpenAndActivate(VPLNamedSocket_t* socket, const char* uniqueName, const char* osUserId)
{
    UNUSED(osUserId);
    if ((socket == NULL) || (uniqueName == NULL)) {
        return VPL_ERR_INVALID;
    }
    socket->s.fd = -1;
    s32 rv;
    rv = IPCSocket_Open(&(socket->s), IPC_SOCKET_UNIX, 0);
    if (rv < 0) {
        VPL_LIB_LOG_ERR(VPL_SG_SOCKET, "Failed to open socket: %s", uniqueName);
        goto out;
    }

    rv = IPCSocket_Bind(&(socket->s), uniqueName, 0);
    if (rv < 0) {
        VPL_LIB_LOG_ERR(VPL_SG_SOCKET, "Failed to bind socket: %s", uniqueName);
        goto out;
    }

    rv = IPCSocket_Listen(&(socket->s), DEFAULT_MAX_CONNECTIONS);
    if (rv < 0) {
        VPL_LIB_LOG_ERR(VPL_SG_SOCKET, "Failed to listen on socket: %s", uniqueName);
        goto out;
    }
out:
    return rv;
}

s32
VPLNamedSocket_Reactivate(VPLNamedSocket_t* socket, const char* uniqueName, const char* osUserId)
{
    UNUSED(osUserId);
    if ((socket == NULL) || (uniqueName == NULL)) {
        return VPL_ERR_INVALID;
    }
    // Nothing more to do; this function only exists in the API because of how Windows
    // Named Pipes behave.
    return VPL_OK;
}

s32
VPLNamedSocket_Accept(VPLNamedSocket_t* listeningSocket,
        VPLNamedSocket_t* connectedSocket_out)
{
    if ((listeningSocket == NULL) || (connectedSocket_out == NULL)) {
        return VPL_ERR_INVALID;
    }
    memset(&(connectedSocket_out->s), 0, sizeof(IPCSocket));
    return IPCSocket_Accept(&(listeningSocket->s), &(connectedSocket_out->s),
            0 /* blocking, as per #VPLNamedSocket_Accept's doc */);
}

s32
VPLNamedSocket_Close(VPLNamedSocket_t* socket)
{
    if (socket == NULL) {
        return VPL_ERR_INVALID;
    }
    return IPCSocket_Close(&(socket->s));
}

s32
VPLNamedSocketClient_Close(VPLNamedSocketClient_t* socket)
{
    if (socket == NULL) {
        return VPL_ERR_INVALID;
    }
    return IPCSocket_Close(&(socket->c));
}

s32
VPLNamedSocketClient_Open(const char* uniqueName, VPLNamedSocketClient_t* connectedSocket_out)
{
    int rv = IPCSocket_Open(&(connectedSocket_out->c), IPC_SOCKET_UNIX, 0);
    if (rv < 0) {
        VPL_LIB_LOG_ERR(VPL_SG_SOCKET, "IPCSocket_Open returned %d", rv);
        goto out;
    }
    rv = IPCSocket_Connect(&(connectedSocket_out->c), (const s8*)uniqueName, 0);
    if (rv < 0) {
        VPL_LIB_LOG_INFO(VPL_SG_SOCKET, "IPCSocket_Connect returned %d", rv);
        IPCSocket_CloseAndLog(&(connectedSocket_out->c));
        goto out;
    }
out:
    return rv;
}

int
VPLNamedSocket_GetFd(const VPLNamedSocket_t* socket)
{
    return socket->s.fd;
}

int
VPLNamedSocketClient_GetFd(const VPLNamedSocketClient_t* socket)
{
    return socket->c.fd;
}
