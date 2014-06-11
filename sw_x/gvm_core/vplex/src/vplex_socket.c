/*
 *                Copyright (C) 2008, BroadOn Communications Corp.
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of BroadOn Communications Corp.,
 *   and are protected by Federal copyright law. They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or in
 *   part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#include "vplex_socket.h"
#include "vplex_mem_utils.h"
#include "vplex_trace.h"

int VPLSocket_Read(VPLSocket_t sockfd, void* buf, int len, VPLTime_t timeout) 
{
    VPLTime_t startTime;
    VPLTime_t timeoutTime;
    int bytesRead;
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    startTime = VPLTime_GetTimeStamp();
    timeoutTime = startTime + timeout;
    if (timeoutTime < startTime) {
        // Wrap occurred, wait forever.
        timeoutTime = VPL_TIMEOUT_NONE;
    }
    bytesRead = 0;
    while (bytesRead < len) {
        int rc;
        VPLTime_t now = VPLTime_GetTimeStamp();
        VPLTime_t pollTimeout = (timeout == VPL_TIMEOUT_NONE) ? VPL_TIMEOUT_NONE
            : VPLTime_DiffClamp(timeoutTime, now);
        VPLSocket_poll_t psock;
        psock.socket = sockfd;
        psock.events = VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_RDPRI;
        rc = VPLSocket_Poll(&psock, 1, pollTimeout);
        if (rc < 0) {
            return rc;
        } else if (rc == 0) {
            return VPL_ERR_TIMEOUT;
        } else {
            // TODO: any reason to check the revents?
        }
        
        now = VPLTime_GetTimeStamp();
        rc = VPLSocket_Recv(sockfd, VPLPtr_AddSigned(buf, bytesRead), len - bytesRead);
        if (rc == 0) { // EOF reached on a socket - connection closed.
            // reporting number of bytes read.
            break;
        } else if(rc > 0) {
            bytesRead += rc;
            if ((bytesRead < len) && (now >= timeoutTime)) {
                return VPL_ERR_TIMEOUT;
            }
        } else {
          return rc;
        }
    }
    return bytesRead;
}

int VPLSocket_Write(VPLSocket_t sockfd, const void* buf, int len, VPLTime_t timeout)
{
    VPLTime_t startTime;
    VPLTime_t timeoutTime;
    int bytesWritten;
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    startTime = VPLTime_GetTimeStamp();
    timeoutTime = startTime + timeout;
    if (timeoutTime < startTime) {
        // Wrap occurred, wait forever.
        timeoutTime = VPL_TIMEOUT_NONE;
    }
    bytesWritten = 0;
    while (bytesWritten < len) {
        int rc;
        VPLTime_t now = VPLTime_GetTimeStamp();
        VPLTime_t pollTimeout = (timeout == VPL_TIMEOUT_NONE) ? VPL_TIMEOUT_NONE
            : VPLTime_DiffClamp(timeoutTime, now);
        VPLSocket_poll_t psock;
        psock.socket = sockfd;
        psock.events = VPLSOCKET_POLL_OUT;
        rc = VPLSocket_Poll(&psock, 1, pollTimeout);
        if (rc < 0) {
            return rc;
        } else if (rc == 0) {
            return VPL_ERR_TIMEOUT;
        } else {
            // TODO: any reason to check the revents?
        }

        now = VPLTime_GetTimeStamp();
        rc = VPLSocket_Send(sockfd, VPLPtr_AddSigned(buf, bytesWritten), len - bytesWritten);
        if(rc == 0) {
            // Note: a 0 return code from send() does not indicate EOF.  Instead
            //     it will happen when we've outpaced the receiver for too long.  However,
            //     we really don't expect this case, since we called poll first.
            break;
        } else if (rc > 0) {
            bytesWritten += rc;
            if ((bytesWritten < len) && (now >= timeoutTime)) {
                return VPL_ERR_TIMEOUT;
            }
        } else if (rc == VPL_ERR_AGAIN) {
            // Bug 2463: On WinRT, VPLSocket_Send() will return VPL_ERR_AGAIN if another thread 
            // is currently writing to the same socket.  A similar situation may also happen on
            // other platforms, so let's retry until timeout.
            // In our codebase, we can write to the same socket from multiple threads when we
            // use the private "eventFds"/command queue pattern (so that we can have a thread
            // poll/select on real sockets and also be awakened by other threads).
            if(now >= timeoutTime) {
                return VPL_ERR_TIMEOUT;
            }
            // Give the other thread a better chance to finish.
            VPLThread_Sleep(VPLTime_FromMillisec(5));
        } else {
            VPLTRACE_LOG_WARN(VPLTRACE_GRP_VPL, VPL_SG_SOCKET, "VPLSocket_Send failed: %d", rc);
            return rc;
        }
    }
    return bytesWritten;
}
