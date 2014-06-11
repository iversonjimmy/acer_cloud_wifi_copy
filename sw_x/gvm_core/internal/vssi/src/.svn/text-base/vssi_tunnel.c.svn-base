/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

/// vssi_tunnel.c
///
/// Virtual Storage secure tunnel API implementations

#include "vssi_internal.h"
#include "vssi_vss.h"
#include "vssi_error.h"

#include "vplex_trace.h"

extern VPLMutex_t vssi_mutex;
extern VSSI_SecureTunnelState* secure_tunnel_list_head;
extern void (*VSSI_NotifySocketChangeFn)(void);

void VSSI_InternalCompleteProxyTunnel(void* ctx, VSSI_Result result)
{
    VSSI_SecureTunnelState* tunnel = (VSSI_SecureTunnelState*)(ctx);
    VSSI_Callback client_callback = tunnel->client_callback;
    void* client_ctx = tunnel->client_ctx;
    tunnel->client_callback = NULL;

    // Set tunnel handle if successful. otherwise destroy tunnel state.
    if(result == VSSI_SUCCESS) {
        tunnel->authenticated = true;
        tunnel->recv_enable = true;
        *(tunnel->client_handle) = (VSSI_SecureTunnel)(tunnel);

        // Add secure tunnel to list of secure tunnels.
        VPLMutex_Lock(&vssi_mutex);
        tunnel->next = secure_tunnel_list_head;
        secure_tunnel_list_head = tunnel;
        VSSI_NotifySocketChangeFn();
        VPLMutex_Unlock(&vssi_mutex);
    }
    else {
        VPLMutex_Destroy(&(tunnel->mutex));
        free(tunnel);
    }

    // Call callback with result.
    if(client_callback) {
        (client_callback)(client_ctx, result);
    }
}

static size_t VSSI_InternalTunnelRecvBuffer(VSSI_SecureTunnelState* tunnel,
                                            void** buf_out)
{
    size_t rv = 0;
    *buf_out = NULL;

    if(tunnel->replylen == 0) {
        if(tunnel->incoming_reply == NULL) {
            tunnel->incoming_reply = (char*)calloc(VSS_HEADER_SIZE, 1);
            if(tunnel->incoming_reply == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed allocation for incoming response.");
                goto exit;
            }
            tunnel->reply_bufsize = VSS_HEADER_SIZE;
        }

        rv = VSS_HEADER_SIZE - tunnel->replylen_recvd;
        *buf_out = (tunnel->incoming_reply + tunnel->replylen_recvd);
    }
    else if(tunnel->replylen_recvd == VSS_HEADER_SIZE) {
        if(tunnel->reply_bufsize < tunnel->replylen) {
            // Increase response buffer as needed.
            void* tmp = realloc(tunnel->incoming_reply, tunnel->replylen);

            if(tmp == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed memory allocation on reading reply.");
                goto exit;
            }
            tunnel->incoming_reply = (char*)tmp;
            tunnel->reply_bufsize = tunnel->replylen;
        }
        rv = tunnel->replylen - tunnel->replylen_recvd;
        *buf_out = tunnel->incoming_reply + tunnel->replylen_recvd;
    }
    else {
        rv = tunnel->replylen - tunnel->replylen_recvd;
        *buf_out = tunnel->incoming_reply + tunnel->replylen_recvd;
    }

 exit:
    return rv;    
}

// Return 0: OK to continue receiving.
// Return 1: Stop receiving.
// Return -1: Stop receiving and handle socket disconnect.
static int VSSI_InternalTunnelHandleBuffer(VSSI_SecureTunnelState* tunnel, 
                                           int bufsize)
{
    int rv = 0;
    int rc;

    tunnel->replylen_recvd += bufsize;

    if(tunnel->replylen_recvd < VSS_HEADER_SIZE) {
        // Do nothing yet.
        return 0;
    }
    else if(tunnel->replylen_recvd == VSS_HEADER_SIZE) {
        // Verify header
        switch(vss_get_command(tunnel->incoming_reply)) {
        case VSS_TUNNEL_DATA:
        case VSS_TUNNEL_DATA_REPLY:
            rc = VSSI_VerifyHeader(tunnel->incoming_reply, tunnel->session,
                                   tunnel->sign_mode, tunnel->sign_type);
            break;
        default:
            rc = VSSI_VerifyHeader(tunnel->incoming_reply, tunnel->session,
                                   VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                   VSS_NEGOTIATE_SIGN_TYPE_SHA1);
            break;
        }

        if(rc == VSSI_SUCCESS) {
            // Set expected reply length.
            tunnel->replylen = VSS_HEADER_SIZE +
                vss_get_data_length(tunnel->incoming_reply);
        }
        else {
            // Invalid header.
            // Drop the connetion due to compromised data stream.
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Got invalid header (%d) on connection "FMT_VPLSocket_t".",
                             rc, VAL_VPLSocket_t(tunnel->connection));
            VPLTRACE_DUMP_BUF_ERR(TRACE_BVS, 0,
                                  tunnel->incoming_reply, VSS_HEADER_SIZE);
            rv = -1;
            goto exit;
        }
    }

    if(tunnel->replylen == tunnel->replylen_recvd) {
        // Verify the data.
        if(tunnel->incoming_reply != NULL) {
            switch(vss_get_command(tunnel->incoming_reply)) {
            case VSS_TUNNEL_DATA:
            case VSS_TUNNEL_DATA_REPLY:
                rc = VSSI_VerifyData(tunnel->incoming_reply,
                                     tunnel->replylen, tunnel->session,
                                     tunnel->sign_mode, tunnel->sign_type, tunnel->enc_type);
                break;
            default:
                rc = VSSI_VerifyData(tunnel->incoming_reply,
                                     tunnel->replylen, tunnel->session,
                                     VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                     VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                                     VSS_NEGOTIATE_ENCRYPT_TYPE_AES128);
                break;
            }
        }
        
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Incoming reply type %x has bogus data signature.",
                             vss_get_command(tunnel->incoming_reply));
            rv = -1;
            goto exit;
        }

        // Reset receive state.
        tunnel->replylen = 0;
        tunnel->replylen_recvd = 0;

        // Handle response
        switch(vss_get_command(tunnel->incoming_reply)) {
            
        case VSS_AUTHENTICATE_REPLY:
            // Must be non-authenticated to get this message.
            if(tunnel->authenticated) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Got proxy request reply while authenticated secure tunnel to "FMTu64".",
                                  tunnel->destination_device);
            }
            else {
                // Check the result.
                if(vss_get_status(tunnel->incoming_reply) == VSSI_SUCCESS) {
                    // Tunnel established.
                    tunnel->authenticated = true;

                    if(vss_get_data_length(tunnel->incoming_reply) > VSS_AUTHENTICATER_SIZE_EXT_VER_0) {
                        // assume v1 response
                        char* body = tunnel->incoming_reply + VSS_HEADER_SIZE;
                        tunnel->sign_mode = vss_authenticate_reply_get_signing_mode(body);
                        tunnel->sign_type = vss_authenticate_reply_get_sign_type(body);
                        tunnel->enc_type = vss_authenticate_reply_get_encrypt_type(body);
                        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                          "Secure tunnel "FMT_VPLSocket_t" to "FMTu64" security: %d.%d.%d.",
                                          VAL_VPLSocket_t(tunnel->connection),
                                          tunnel->destination_device, tunnel->sign_mode,
                                          tunnel->sign_type, tunnel->enc_type);                          
                    }
                    *(tunnel->client_handle) = (VSSI_SecureTunnel)(tunnel);
                    if(tunnel->client_callback) {
                        (tunnel->client_callback)(tunnel->client_ctx, VSSI_SUCCESS);
                        tunnel->client_callback = NULL;
                    }
                }
                else {
                    // Tunnel failed to authenticate.
                    tunnel->fail_reason = vss_get_status(tunnel->incoming_reply);                    
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Secure tunnel to "FMTu64" not authenticated. Code %d",
                                      tunnel->destination_device,
                                      tunnel->fail_reason);
                    rv = -1;
                }
            }
            break;

        case VSS_TUNNEL_DATA:
        case VSS_TUNNEL_DATA_REPLY: // Shouldn't see, but it's same as above.
            VPLMutex_Lock(&(tunnel->mutex));

            if(!tunnel->resetting) {
                char* data = tunnel->incoming_reply + VSS_HEADER_SIZE;
                VSSI_MsgBuffer* msg;
                // Add data to received queue.
                msg = (VSSI_MsgBuffer*)calloc(sizeof(VSSI_MsgBuffer) +
                                              vss_tunnel_data_get_length(data), 1);
                if(msg == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Unable to allocate space for received data on secure tunnel to "FMTu64".",
                                     tunnel->destination_device);
                    rv = -1;
                    VPLMutex_Unlock(&(tunnel->mutex));
                    goto exit;
                }
                memcpy((char*)(msg + 1), vss_tunnel_data_get_data(data),
                       vss_tunnel_data_get_length(data));
                msg->length = vss_tunnel_data_get_length(data);


                if(tunnel->recv_tail) {
                    tunnel->recv_tail->next = msg;
                    
                }
                else {
                    tunnel->recv_head = msg;
                }
                tunnel->recv_tail = msg;
                tunnel->recv_q_depth += msg->length;

                if(tunnel->recv_q_depth >= VSSI_TUNNEL_RECV_Q_MAX) {
                    // Pause receive until queue drains.
                    tunnel->recv_enable = false;
                    VSSI_NotifySocketChangeFn();
                }

                // Call any callbacks for waiting data.
                while(tunnel->recv_wait_top) {
                    VSSI_TunnelWaiter* waiter = tunnel->recv_wait_top;
                    tunnel->recv_wait_top = waiter->next;

                    (waiter->callback)(waiter->ctx, VSSI_SUCCESS);

                    free(waiter);
                }
            }

            VPLMutex_Unlock(&(tunnel->mutex));
            break;

        case VSS_TUNNEL_RESET:
        case VSS_TUNNEL_RESET_REPLY: // same thing
            VPLMutex_Lock(&(tunnel->mutex));

            tunnel->resetting = 0;
            tunnel->inactive_timeout = VPLTIME_INVALID;

            // Call callbacks for tunnel reset
            while(tunnel->reset_wait_top) {
                VSSI_TunnelWaiter* waiter = tunnel->reset_wait_top;
                tunnel->reset_wait_top = waiter->next;
                
                (waiter->callback)(waiter->ctx, VSSI_SUCCESS);
                
                free(waiter);
            }
            // Call callbacks for waiting to send
            while(tunnel->send_wait_top) {
                VSSI_TunnelWaiter* waiter = tunnel->send_wait_top;
                tunnel->send_wait_top = waiter->next;
                
                (waiter->callback)(waiter->ctx, VSSI_SUCCESS);
                
                free(waiter);
            }
            // Call callbacks for waiting to receive
            while(tunnel->recv_wait_top) {
                VSSI_TunnelWaiter* waiter = tunnel->recv_wait_top;
                tunnel->recv_wait_top = waiter->next;

                (waiter->callback)(waiter->ctx, VSSI_ABORTED/*VSSI_SUCCESS*/);

                free(waiter);
            }

            VPLMutex_Unlock(&(tunnel->mutex));
            break;

        case VSS_ERROR:
        default:
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Incoming reply of unexpected type 0x%x received on secure tunnel to "FMTu64".",
                             vss_get_command(tunnel->incoming_reply),
                             tunnel->destination_device);
            tunnel->fail_reason = vss_get_status(tunnel->incoming_reply);
            if(tunnel->fail_reason == VSSI_SUCCESS) {
                tunnel->fail_reason = VSSI_COMM;
            }
            rv = -1;
            break;
        }
    }

 exit:
    return rv;
}

void VSSI_InternalTunnelDoRecv(VSSI_SecureTunnelState* tunnel)
{
    int rc = 0;
    void* buf;
    size_t bufsize;
    
    // Determine how many bytes to receive.
    bufsize = VSSI_InternalTunnelRecvBuffer(tunnel, &buf);
    if(buf == NULL || bufsize == 0) {
        // Nothing to receive now.
        return;
    }
    
    // Receive them. Handle any socket errors.
    rc = VPLSocket_Recv(tunnel->connection, buf, bufsize);
    if(rc == 0) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Got socket closed for connection "FMT_VPLSocket_t".",
                          VAL_VPLSocket_t(tunnel->connection));
        rc = -1;
    }
    else if(rc < 0) {
        // handle expected socket errors
        switch(rc) {
        case VPL_ERR_AGAIN:
        case VPL_ERR_INTR:
            // These are OK. Try again later.
            rc = 1; // Stop, but no disconnect.
            break;
            
        default:
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Got socket error %d for connection "FMT_VPLSocket_t".",
                              rc, VAL_VPLSocket_t(tunnel->connection));
            rc = -1; // stop and disconnect
            break;
        }
    }
    else {
        // Handle received bytes.
        tunnel->last_active = VPLTime_GetTimeStamp();
        rc = VSSI_InternalTunnelHandleBuffer(tunnel, rc);
    }
 
   if(rc == -1) {
       // Close connection. Do not destroy.
       // Client will destroy later.
       VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                         "Closing tunnel connection "FMT_VPLSocket_t".",
                         VAL_VPLSocket_t(tunnel->connection));
       
       VPLMutex_Lock(&(tunnel->mutex));
       
       VPLSocket_Close(tunnel->connection);
       tunnel->connection = VPLSOCKET_INVALID;
       VSSI_NotifySocketChangeFn();
       
       // Call any callbacks for waiting data.
       VSSI_InternalClearTunnelNoLock(tunnel);
       
       VPLMutex_Unlock(&(tunnel->mutex));

       if(!tunnel->authenticated) {
           // Destroy tunnel later.
           tunnel->disconnected = true;
        }
    }
}

VSSI_Result VSSI_InternalTunnelRecv(VSSI_SecureTunnelState* tunnel,
                                    char* data,
                                    size_t len)
{
    VSSI_Result rv = 0;

    if(data == NULL || len == 0) {
        return VSSI_INVALID;
    }

    // Pull data from receive queue in order received.
    VPLMutex_Lock(&(tunnel->mutex));
    
    if(tunnel->recv_head == NULL) {
        if(VPLSocket_Equal(tunnel->connection, VPLSOCKET_INVALID)) {
            // Only returning comm error if there are no bytes to send.
            rv = VSSI_COMM;
        }
        else {
            rv = VSSI_AGAIN;
        }
    }
    else {
        // Return as much as is available, up to amount requested.
        while(tunnel->recv_head != NULL && ((size_t)(rv)) < len) {
            size_t transfer = tunnel->recv_head->length - tunnel->recv_so_far;
            if(transfer > len - rv) {
                transfer = len - rv;
            }

            memcpy(data + rv, tunnel->recv_head->msg + tunnel->recv_so_far,
                   transfer);
            tunnel->recv_so_far += transfer;
            tunnel->recv_q_depth -= transfer;
            rv += transfer;

            if(tunnel->recv_so_far == tunnel->recv_head->length) {
                VSSI_MsgBuffer* msg = tunnel->recv_head;
                if(tunnel->recv_tail == msg) {
                    tunnel->recv_tail = NULL;
                }
                tunnel->recv_head = msg->next;
                free(msg);
                tunnel->recv_so_far = 0;
            }
        }

        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Received %d/"FMTu_size_t" bytes requested from secure tunnel "FMT_VPLSocket_t".",
                            rv, len, VAL_VPLSocket_t(tunnel->connection));
    }
    
    if(tunnel->recv_q_depth < VSSI_TUNNEL_RECV_Q_MAX) {
        tunnel->recv_enable = true;
        VSSI_NotifySocketChangeFn();
    }

    VPLMutex_Unlock(&(tunnel->mutex));


    return rv;
}

void VSSI_InternalTunnelWaitForRecv(VSSI_SecureTunnelState* tunnel,
                                    void* ctx,
                                    VSSI_Callback callback)
{
    int do_callback = false;
    int rv = VSSI_SUCCESS;
    VSSI_TunnelWaiter* waiter;

    if(callback == NULL)  {
        return;
    }

    VPLMutex_Lock(&(tunnel->mutex));

    if(tunnel->recv_q_depth > 0) {
        do_callback = true;
    }
    else if(VPLSocket_Equal(tunnel->connection, VPLSOCKET_INVALID)) {
        rv = VSSI_COMM;
        do_callback = true;
    }
    else {
        waiter = (VSSI_TunnelWaiter*)calloc(sizeof(VSSI_TunnelWaiter), 1);
        if(waiter == NULL) {
            rv = VSSI_NOMEM;
            do_callback = true;
        }
        else {
            waiter->ctx = ctx;
            waiter->callback = callback;
            
            waiter->next = tunnel->recv_wait_top;
            tunnel->recv_wait_top = waiter;

            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Caller now waiting to receive from secure tunnel "FMT_VPLSocket_t".",
                                VAL_VPLSocket_t(tunnel->connection));
        }
    }

    VPLMutex_Unlock(&(tunnel->mutex));

    if(do_callback) {
        (callback)(ctx, rv);
    }
}

VSSI_Result VSSI_InternalTunnelSend(VSSI_SecureTunnelState* tunnel,
                                    const char* data,
                                    size_t len)
{
    VSSI_Result rv = VSSI_SUCCESS;

    if(data == NULL || len == 0) {
        return VSSI_INVALID;
    }
    if(VPLSocket_Equal(tunnel->connection, VPLSOCKET_INVALID)) {
        return VSSI_COMM;
    }

    // Add to the tunnel send queue
    VPLMutex_Lock(&(tunnel->mutex));

    if(tunnel->send_q_depth >= VSSI_TUNNEL_SEND_Q_MAX) {
        rv = VSSI_AGAIN;
    }
    else if(tunnel->resetting) {
        // Reset of tunnel pending. Reject send.
        rv = VSSI_AGAIN;        
    }
    else {
        // Create signed, encrypted data packet for queue.
        VSSI_MsgBuffer* msg = NULL;
        char header[VSS_HEADER_SIZE] = {0};
        char body[VSS_TUNNEL_DATA_BASE_SIZE] = {0};
        
        vss_set_command(header, VSS_TUNNEL_DATA);
        vss_set_xid(header, tunnel->next_xid++);
        vss_tunnel_data_set_length(body, len);
        msg = VSSI_PrepareMessage(header, body, VSS_TUNNEL_DATA_BASE_SIZE,
                                  data, len, NULL, 0);
        if(msg == NULL) {
            rv = VSSI_NOMEM;
            goto exit;
        }

        msg = VSSI_ComposeMessage(tunnel->session, 
                                  tunnel->sign_mode, tunnel->sign_type, tunnel->enc_type,
                                  msg);
        if(msg == NULL) {
            rv = VSSI_NOMEM;
            goto exit;
        }

        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Queuing message (%p) size "FMTu_size_t" (contains "FMTu_size_t" bytes) Secuity:%d.%d.%d for send on secure tunnel "FMT_VPLSocket_t" to device "FMTu64".",
                            msg, msg->length, len, tunnel->sign_mode, tunnel->sign_type, tunnel->enc_type,
                            VAL_VPLSocket_t(tunnel->connection), tunnel->destination_device);
        
        if(tunnel->send_head == NULL) {
            tunnel->send_head = msg;
        }
        else {
            tunnel->send_tail->next = msg;
        }
        tunnel->send_tail = msg;
        tunnel->send_q_depth += msg->length;
        if(!tunnel->send_enable) {
            tunnel->send_enable = true;
            VSSI_NotifySocketChangeFn();
        }
        rv = (int)(len);
    }

 exit:
    VPLMutex_Unlock(&(tunnel->mutex));

    return rv;
}

void VSSI_InternalTunnelDoSend(VSSI_SecureTunnelState* tunnel)
{
    int rc;

    VPLMutex_Lock(&(tunnel->mutex));
    
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Performing send on secure tunnel "FMT_VPLSocket_t".",
                        VAL_VPLSocket_t(tunnel->connection));

    if(tunnel->send_head != NULL) {
        rc = VPLSocket_Send(tunnel->connection,
                            tunnel->send_head->msg + tunnel->sent_so_far,
                            tunnel->send_head->length - tunnel->sent_so_far);
        if(rc > 0) {
            tunnel->sent_so_far += rc;
            tunnel->send_q_depth -= rc;
        }
        else {
            // Stop sending at first error.
            // Handle socket closed with receive.
            goto exit;
        }
        
        if(tunnel->sent_so_far == tunnel->send_head->length) {
            VSSI_MsgBuffer* msg = tunnel->send_head;

            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Done sending data block (%p) of size "FMTu_size_t" on secure tunnel "FMT_VPLSocket_t".",
                                msg, msg->length, VAL_VPLSocket_t(tunnel->connection));

            tunnel->send_head = msg->next;
            if(tunnel->send_tail == msg) {
                tunnel->send_tail = NULL;
            }
            free(msg);
            tunnel->sent_so_far = 0;
        }
    }

    if(tunnel->send_q_depth < VSSI_TUNNEL_SEND_Q_MAX) {
        // Call any callbacks for waiting send space.
        while(tunnel->send_wait_top) {
            VSSI_TunnelWaiter* waiter = tunnel->send_wait_top;
            tunnel->send_wait_top = waiter->next;
            
            (waiter->callback)(waiter->ctx, VSSI_SUCCESS);
            
            free(waiter);
        }
    }
    if(tunnel->send_head == NULL) {
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Pause sending on secure tunnel "FMT_VPLSocket_t".",
                            VAL_VPLSocket_t(tunnel->connection));
        tunnel->send_enable = false;
        VSSI_NotifySocketChangeFn();
    }   
    tunnel->last_active = VPLTime_GetTimeStamp();

 exit:
    VPLMutex_Unlock(&(tunnel->mutex));
}

void VSSI_InternalTunnelWaitForSend(VSSI_SecureTunnelState* tunnel,
                                    void* ctx,
                                    VSSI_Callback callback)
{
    int do_callback = false;
    int rv = VSSI_SUCCESS;
    VSSI_TunnelWaiter* waiter;

    if(callback == NULL)  {
        return;
    }

    VPLMutex_Lock(&(tunnel->mutex));

    if(VPLSocket_Equal(tunnel->connection, VPLSOCKET_INVALID)) {
        rv = VSSI_COMM;
        do_callback = true;
    }
    else if(!tunnel->resetting &&
       tunnel->send_q_depth < VSSI_TUNNEL_SEND_Q_MAX) {
        do_callback = true;
    }
    else {
        waiter = (VSSI_TunnelWaiter*)calloc(sizeof(VSSI_TunnelWaiter), 1);
        if(waiter == NULL) {
            rv = VSSI_NOMEM;
            do_callback = true;
        }
        else {
            waiter->ctx = ctx;
            waiter->callback = callback;
            
            waiter->next = tunnel->send_wait_top;
            tunnel->send_wait_top = waiter;
        }

        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Caller now waiting for space in send queue for secure tunnel "FMT_VPLSocket_t".",
                            VAL_VPLSocket_t(tunnel->connection));

    }

    VPLMutex_Unlock(&(tunnel->mutex));

    if(do_callback) {
        (callback)(ctx, rv);
    }
}

void VSSI_InternalTunnelReset(VSSI_SecureTunnelState* tunnel,
                              void* ctx,
                              VSSI_Callback callback)
{
    int do_callback = false;
    int rv = VSSI_SUCCESS;
    VSSI_TunnelWaiter* waiter;
    VSSI_MsgBuffer* msg = NULL;
    char header[VSS_HEADER_SIZE] = {0};
    
    VPLMutex_Lock(&(tunnel->mutex));

    if(VPLSocket_Equal(tunnel->connection, VPLSOCKET_INVALID)) {
        rv = VSSI_COMM;
        do_callback = true;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Reset attempted on INVALID socket");
        goto exit;
    }

    waiter = (VSSI_TunnelWaiter*)calloc(sizeof(VSSI_TunnelWaiter), 1);
    if(waiter == NULL) {
        rv = VSSI_NOMEM;
        do_callback = true;
    }
    else {
        vss_set_command(header, VSS_TUNNEL_RESET);
        vss_set_xid(header, tunnel->next_xid++);
        msg = VSSI_PrepareMessage(header, NULL, 0, NULL, 0, NULL, 0);
        if(msg == NULL) {
            rv = VSSI_NOMEM;
            do_callback = true;
            goto exit;
        }

        msg = VSSI_ComposeMessage(tunnel->session, 
                                  VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                  VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                                  VSS_NEGOTIATE_ENCRYPT_TYPE_AES128,
                                  msg);
        if(msg == NULL) {
            rv = VSSI_NOMEM;
            do_callback = true;
            goto exit;
        }

        waiter->ctx = ctx;
        waiter->callback = callback;
        
        waiter->next = tunnel->reset_wait_top;
        tunnel->reset_wait_top = waiter;
        tunnel->resetting = 1;
        tunnel->inactive_timeout = VSSI_TUNNEL_RESET_TIMEOUT;

        // Purge all received data.
        while(tunnel->recv_head) {
            VSSI_MsgBuffer* msg = tunnel->recv_head;
            tunnel->recv_head = msg->next;
            free(msg);
        }
        tunnel->recv_tail = NULL;
        tunnel->recv_q_depth = 0;
        tunnel->recv_so_far = 0;
        if(!tunnel->recv_enable) {
            tunnel->recv_enable = true;
            VSSI_NotifySocketChangeFn();
        }
        // Purge any send data not yet started.
        if(tunnel->send_head) {
            if(tunnel->sent_so_far == 0) {
                while(tunnel->send_head) {
                    VSSI_MsgBuffer* msg = tunnel->send_head;
                    tunnel->send_head = msg->next;
                    tunnel->send_q_depth -= msg->length;
                    free(msg);
                }
            }
            else {
                while(tunnel->send_head->next) {
                    VSSI_MsgBuffer* msg = tunnel->send_head->next;
                    tunnel->send_head->next = msg->next;
                    tunnel->send_q_depth -= msg->length;
                    free(msg);
                }
            }
            tunnel->send_tail = tunnel->send_head; // empty or one message
        }
        
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Queuing tunnel reset message (%p) length "FMTu_size_t" for send on secure tunnel "FMT_VPLSocket_t".",
                            msg, msg->length,
                            VAL_VPLSocket_t(tunnel->connection));
        
        if(tunnel->send_head == NULL) {
            tunnel->send_head = msg;
        }
        else {
            tunnel->send_tail->next = msg;
        }
        tunnel->send_tail = msg;
        tunnel->send_q_depth += msg->length;
        if(!tunnel->send_enable) {
            tunnel->send_enable = true;
            VSSI_NotifySocketChangeFn();
        }
        
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Caller now waiting for tunnel reset for secure tunnel "FMT_VPLSocket_t".",
                            VAL_VPLSocket_t(tunnel->connection));
    }

 exit:
    VPLMutex_Unlock(&(tunnel->mutex));
    
    if(do_callback && callback) {
        (callback)(ctx, rv);
    }
}

void VSSI_InternalClearTunnel(VSSI_SecureTunnelState* tunnel)
{
    VPLMutex_Lock(&(tunnel->mutex));
    VSSI_InternalClearTunnelNoLock(tunnel);
    VPLMutex_Unlock(&(tunnel->mutex));
}

void VSSI_InternalClearTunnelNoLock(VSSI_SecureTunnelState* tunnel)
{
    VSSI_TunnelWaiter* waiter;

    // Call all send/receive/reset waiters with VSSI_COMM
    while(tunnel->send_wait_top) {
        waiter = tunnel->send_wait_top;
        tunnel->send_wait_top = waiter->next;
        
        (waiter->callback)(waiter->ctx, VSSI_COMM);
        
        free(waiter);
    }
    while(tunnel->recv_wait_top) {
        waiter = tunnel->recv_wait_top;
        tunnel->recv_wait_top = waiter->next;
        
        (waiter->callback)(waiter->ctx, VSSI_COMM);
        
        free(waiter);
    }
    while(tunnel->reset_wait_top) {
        waiter = tunnel->reset_wait_top;
        tunnel->reset_wait_top = waiter->next;
        
        (waiter->callback)(waiter->ctx, VSSI_COMM);
        
        free(waiter);
    }

    // If not authenticated, call client callback with error.
    if(!(tunnel->authenticated) && tunnel->client_callback) {
        if(tunnel->fail_reason == VSSI_SUCCESS) {
            tunnel->fail_reason = VSSI_COMM;
        }
        (tunnel->client_callback)(tunnel->client_ctx, tunnel->fail_reason);
        tunnel->client_callback = NULL;
    }
}

void VSSI_InternalDestroyTunnel(VSSI_SecureTunnelState* tunnel)
{
    VSSI_MsgBuffer* msg;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Destroying tunnel connection "FMT_VPLSocket_t".",
                      VAL_VPLSocket_t(tunnel->connection));
    
    VPLMutex_Lock(&(tunnel->mutex));

    // Call all send/receive/reset waiters with VSSI_COMM
    VSSI_InternalClearTunnelNoLock(tunnel);

    // Clear send/receive queues, freeing queued data
    while(tunnel->send_head) {
        msg = tunnel->send_head;
        tunnel->send_head = msg->next;
        free(msg);
    }
    while(tunnel->recv_head) {
        msg = tunnel->recv_head;
        tunnel->recv_head = msg->next;
        free(msg);
    }

    if(tunnel->incoming_reply) {
        free(tunnel->incoming_reply);
    }

    // Close connection if open
    if(!VPLSocket_Equal(tunnel->connection, VPLSOCKET_INVALID)) {
        VPLSocket_Close(tunnel->connection);
        tunnel->connection = VPLSOCKET_INVALID;
    }

    VPLMutex_Unlock(&(tunnel->mutex));

    // Destroy mutex
    VPLMutex_Destroy(&(tunnel->mutex));

    // Remove tunnel from list.
    if(secure_tunnel_list_head == tunnel) {
        secure_tunnel_list_head = tunnel->next;
    }
    else {
        VSSI_SecureTunnelState* prev = secure_tunnel_list_head;
        while(prev != NULL && prev->next != tunnel) {
            prev = prev->next;
        }
        if(prev != NULL) {
            prev->next = tunnel->next;
        }
    }

    // Free state
    free(tunnel);

    VSSI_NotifySocketChangeFn();
}
