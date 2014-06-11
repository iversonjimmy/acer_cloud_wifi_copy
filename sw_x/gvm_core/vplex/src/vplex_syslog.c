//
//               Copyright (C) 2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and  are protected by Federal copyright law. They may not be disclosed
//  to  third  parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//
//

#include "vplex_syslog.h"

#ifdef VPL_SYSLOG_ENABLED

#include "vpl_socket.h"
#include "vpl_net.h"
#include "vplex_socket.h"

static VPLSocket_t syslog_socket = VPLSOCKET_SET_INVALID;

int VPLSyslog_SetServer(char const* hostname) {
    int rv;
    VPLSocket_addr_t sock_addr;

    if(VPLSocket_Equal(syslog_socket, VPLSOCKET_INVALID)) {
        // Ensure socket library is ready (it's okay if this was already called previously).
        (IGNORE_RESULT) VPLSocket_Init();
    }
    else {
        // Replacing existing syslog server connection.
        (IGNORE_RESULT) VPLSocket_Close(syslog_socket);
    }

    // Creating a BLOCKING socket for syslogs.
    syslog_socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_DGRAM, 0);
    if(VPLSocket_Equal(syslog_socket, VPLSOCKET_INVALID)) {
        return VPL_ERR_SOCKET;
    }

    sock_addr.family = VPL_PF_INET;
    sock_addr.addr = VPLNet_GetAddr(hostname);
    sock_addr.port = VPLNet_port_hton(VPLSYSLOG_PORT);

    if(sock_addr.addr == VPLNET_ADDR_INVALID) {
        return VPL_ERR_HOSTNAME;
    }

    if ((rv = VPLSocket_Connect(syslog_socket, &sock_addr, sizeof(sock_addr))) != VPL_OK) {

        VPLSocket_Close(syslog_socket);
        syslog_socket = VPLSOCKET_INVALID;
        return VPL_ERR_NOTCONN;
    }

    return VPL_OK;
}

int VPLSyslog(char const* msg, size_t msglen)
{
    int rv = VPL_OK;

    if(!VPLSocket_Equal(syslog_socket, VPLSOCKET_INVALID)) {
        char buf[VPLSYSLOG_MAX_MSG_LEN];

        // non-standard syslog format.  the TP field is skipped.
        snprintf(buf, sizeof(buf), "<%d>%s", VPLSYSLOG_PRIORITY, msg);

        if((rv = VPLSocket_Write(syslog_socket, buf,
                                 (int)msglen, VPLTIME_FROM_SEC(10))) >= 0) {
            rv = VPL_OK;
        }
    }

    return rv;
}

#endif
