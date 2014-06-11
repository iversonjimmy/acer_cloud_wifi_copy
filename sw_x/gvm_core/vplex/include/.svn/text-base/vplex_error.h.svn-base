//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_ERROR_H__
#define __VPLEX_ERROR_H__

/// VPLex library error codes.
/// -9600 to -9699 for general VPLex errors.
/// -9700 to -9799 for community services specific errors.
/// -9800 to -9999 reserved for later use.
#define VPLEX_ERR_CODES \
    \
    /* Catch-all error code for an unexpected error within the HTTP client library. */ \
    ROW(-9601, VPL_ERR_HTTP_ENGINE) \
    \
    ROW(-9602, VPL_ERR_BUF_TOO_SMALL) \
    \
    /* A SOAP call got a response from the server that was not HTTP status 200. */ \
    ROW(-9603, VPL_ERR_UNABLE_TO_SERVICE) \
    \
    ROW(-9605, VPL_ERR_INVALID_SERVER_RESPONSE) \
    ROW(-9606, VPL_ERR_FAILED_ON_SERVER) \
    ROW(-9607, VPL_ERR_INVALID_LOGIN) \
    ROW(-9608, VPL_ERR_INVALID_SESSION) \
    ROW(-9609, VPL_ERR_INVALID_STATUS) \
    ROW(-9610, VPL_ERR_SSL) \
    ROW(-9611, VPL_ERR_SSL_DATE_INVALID) \
    ROW(-9612, VPL_ERR_RESPONSE_TRUNCATED) \
    ROW(-9613, VPL_ERR_IN_RECV_CALLBACK) \
    \
    ROW(-9701, VPL_ERR_EXISTS) \
    ROW(-9702, VPL_ERR_NOT_EXIST) \
    \
    /* DEPRECATED: no longer used. */ \
    ROW(-9703, VPL_ERR_FRIEND_REQUEST_PENDING) \
    \
    /* DEPRECATED: no longer used. */ \
    ROW(-9704, VPL_ERR_ALREADY_FRIEND) \


/// For implementation only. \hideinitializer
#define ROW(code, name) \
    name = code,

/// VPLex-specific error codes.
enum {
    VPLEX_ERR_CODES
};
#undef ROW

/// Remap positive server error codes to the range [-30000, -39999] without losing information.
/// Client conventions are that error codes must be less than 0.
/// For more information, please see "Error Codes by Subsystem" on the Wiki.
#define VPL_INFRA_ERR_TO_CLIENT_ERR(infraErr) (-30000 - (infraErr))
#define VPL_MAX_EXPECTED_INFRA_ERRCODE  9999

//-----------------------------------------------------------------------------
// TODO: Bug 10159
// These are from the GVM IPC library, which hasn't been properly merged into
// VPLex yet.
//-----------------------------------------------------------------------------
#define IPC_ERROR_OK                   0
#define IPC_ERROR_ACCESS               -15001
#define IPC_ERROR_EXISTS               -15002
#define IPC_ERROR_INVALID              -15004
#define IPC_ERROR_MAX                  -15005
#define IPC_ERROR_NOEXISTS             -15006
#define IPC_ERROR_UNKNOWN              -15009
#define IPC_ERROR_NOTREADY             -15010
#define IPC_ERROR_NO_LINK              -15024
#define IPC_ERROR_PARAMETER            -15025

#define IPC_ERROR_CONNECT              -15050
#define IPC_ERROR_DISCONNECT           -15051
#define IPC_ERROR_UPDATE               -15052
#define IPC_ERROR_STATREQ              -15053
#define IPC_ERROR_INIT                 -15054
#define IPC_ERROR_CAL                  -15055
#define IPC_ERROR_SEND                 -15056
#define IPC_ERROR_RECEIVE              -15057
#define IPC_ERROR_ACCEPT               -15058
#define IPC_ERROR_INTR                 -15059
#define IPC_ERROR_SRV                  -15060
#define IPC_ERROR_CLOSE                -15061

#define IPC_ERROR_SOCKET_ACCEPT        -15080
#define IPC_ERROR_SOCKET_BIND          -15081
#define IPC_ERROR_SOCKET_CLOSE         -15082
#define IPC_ERROR_SOCKET_CONNECT       -15083
#define IPC_ERROR_SOCKET_LISTEN        -15084
#define IPC_ERROR_SOCKET_OPEN          -15085
#define IPC_ERROR_SOCKET_NONBLOCK      -15086
#define IPC_ERROR_SOCKET_RECEIVE       -15087
#define IPC_ERROR_SOCKET_RECEIVE_FROM  -15088
#define IPC_ERROR_SOCKET_SEND          -15089
#define IPC_ERROR_SOCKET_SEND_TO       -15090
#define IPC_ERROR_SOCKET_PEERNAME      -15091
#define IPC_ERROR_SOCKET_SEND_FILE     -15092

/// @deprecated No longer generated.
// TODO: bug 14970: remove these in the next release
//@{
#define IPC_ERROR_SEM_ALREADY_WAIT     -15100
#define IPC_ERROR_SEM_CLOSE            -15101
#define IPC_ERROR_SEM_WAIT             -15102
#define IPC_ERROR_SEM_OPEN             -15103
#define IPC_ERROR_SEM_TRYWAIT          -15104
#define IPC_ERROR_SEM_POST             -15105

#define IPC_ERROR_MSGQ_CLOSE           -15120
#define IPC_ERROR_MSGQ_GETATTR         -15121
#define IPC_ERROR_MSGQ_OPEN            -15122
#define IPC_ERROR_MSGQ_READ            -15123
#define IPC_ERROR_MSGQ_NONBLOCK        -15124
#define IPC_ERROR_MSGQ_WRITE           -15125
#define IPC_ERROR_MSGQ_UNLINK          -15126

#define IPC_ERROR_SHM_CLOSE            -15140
#define IPC_ERROR_SHM_OPEN             -15142
#define IPC_ERROR_SHM_MMAP             -15143
#define IPC_ERROR_SHM_UNLINK           -15144

#define IPC_RTP_ERROR_RECV             -15152
#define IPC_RTP_ERROR_SEND             -15153
#define IPC_RTP_ERROR_DROP             -15154
//@}

#endif // include guard
