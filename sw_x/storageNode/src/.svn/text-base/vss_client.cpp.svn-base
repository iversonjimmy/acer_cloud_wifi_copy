/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include "vss_client.hpp"

#include <iostream>

#include "vpl_conv.h"
#include "vplex_trace.h"

#include "vss_comm.h"

#include "vss_cmdproc.hpp"

using namespace std;

vss_client::vss_client(vss_server& server,
                       VPLSocket_t sockfd,
                       VPLNet_addr_t addr,
                       VPLNet_port_t port) :
    sockfd(sockfd), 
    sending(false),
    receiving(false),
    disconnected(false),
    new_connection(true),
    tasks_pending(0),
    addr(addr),
    port(port),
    device_id(0),
    server(server), 
    recv_cnt(0),
    send_cnt(0),
    cmd_cnt(0),
    incoming_req(NULL),
    reqlen(0),
    req_so_far(0),
    recv_error(false),
    sent_so_far(0),
    pending_cmds(0),
    inactive_timeout(VPLTIME_INVALID)
{
    VPLMutex_Init(&mutex);
    start_time = last_active = VPLTime_GetTimeStamp();  

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      FMT_VPLNet_addr_t":%u connected",
                      VAL_VPLNet_addr_t(addr), port); 
}

vss_client::~vss_client()
{
    disconnect();

    if(!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u socket "FMT_VPLSocket_t" disconnected. Closing socket.",
                          device_id, VAL_VPLNet_addr_t(addr), port, VAL_VPLSocket_t(sockfd));
        VPLSocket_Close(sockfd);
        sockfd = VPLSOCKET_INVALID;
    }
    
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      FMTu64" "FMT_VPLNet_addr_t":%u deleting.",
                      device_id, VAL_VPLNet_addr_t(addr), port);

    if(!send_queue.empty()) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u Send queue still had data.",
                          device_id, VAL_VPLNet_addr_t(addr), port);    
    }
    while(!send_queue.empty()) {
        pair<size_t, const char*> response = send_queue.front();
        free((void*)response.second);
        send_queue.pop();
    }
    
    delete incoming_req;

    VPLMutex_Destroy(&mutex);
}

int vss_client::start(VPLTime_t inactive_timeout)
{
    int rc = 0;

    VPLMutex_Lock(&mutex);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Client connection made using sockfd %d.",
                      VPLSocket_AsFd(sockfd));

    // Set TCP keep-alive on, probe after 30 seconds inactive, 
    // repeat every 3 seconds, fail if inactive for 60 seconds.
    rc = VPLSocket_SetKeepAlive(sockfd, true, 30, 3, 10);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Failed to set TCP_KEEPALIVE on socket "FMT_VPLSocket_t": %d.",
                          VAL_VPLSocket_t(sockfd), rc);
    }

    last_active = VPLTime_GetTimeStamp();
    receiving = true;
    server.changeNotice();
    // Not sending until there's something to send.

    this->inactive_timeout = inactive_timeout;

    VPLMutex_Unlock(&mutex);

    return 0;
}

bool vss_client::active()
{
    bool rv = false;

    // Consider active if connected and there are pending commands or
    // response data to send.
    if((sending || receiving) &&
       (req_so_far || pending_cmds || !send_queue.empty())) {
        rv = true;
    }
   
    VPLMutex_Lock(&mutex); 
    if(!objects.empty())
        rv = true;
    VPLMutex_Unlock(&mutex);

    return rv;
}

void vss_client::disconnect() 
{
    VPLMutex_Lock(&mutex);

    disconnected = true;

    sending = false;
    receiving = false;

    if(pending_cmds != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u %d pending commands.",
                          device_id, VAL_VPLNet_addr_t(addr), port, pending_cmds);    
    }

    // release any associated objects
    // Note: we're counting on nesting the mutex lock here
    // since this will trigger a call to disassociate_object()
    // This will also modify the list so we don't perform an
    // inc on the iterator but start it from the beginning.
    for( std::set<vss_object*>::iterator it = objects.begin();
            it != objects.end() ; it = objects.begin() ) {
       (*it)->set_notify_mask(0, this, 0); 
    }

    VPLMutex_Unlock(&mutex);

    server.changeNotice();
}

void vss_client::do_receive()
{
    int rc;
    void* buf;
    size_t bufsize;

    if(recv_error) {
        receiving = false;
        return;
    }

    // Determine how many bytes to receive.
    bufsize = receive_buffer(&buf);
    if(buf == NULL || bufsize <= 0) {
        // nothing to receive now
        return;
    }
    
    // Receive them. Handle any socket errors.
    rc = VPLSocket_Recv(sockfd, buf, bufsize);
    if(rc < 0) {
        if(rc == VPL_ERR_AGAIN) {
            // Temporary setback.
            rc = 0;
        }
        else {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              FMTu64" "FMT_VPLNet_addr_t":%u recv error %d.",
                              device_id, VAL_VPLNet_addr_t(addr), port, rc);
            goto recv_error;
        }
    }
    else if(rc == 0) { // Socket shutdown. Treat as "error".
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u client disconnect.",
                          device_id, VAL_VPLNet_addr_t(addr), port);
        goto recv_error;
    }
    
    // Handle received bytes.
    if(rc > 0) {
        recv_cnt += rc;
        handle_received(rc);
    }
    
    last_active = VPLTime_GetTimeStamp();

    return;
 recv_error:
    // Failed to receive data. Consider connection lost.
    delete incoming_req;
    incoming_req = NULL;
    disconnect();
}

size_t vss_client::receive_buffer(void** buf_out)
{
    // Determine how much data to receive.
    // Allocate space to receive it as needed.
    size_t rv = 0;
    *buf_out = NULL;
    
    if(recv_error) {
        goto exit;
    }

    if(incoming_req == NULL) {
        reqlen = 0;
        req_so_far = 0;
        incoming_req = new (nothrow) vss_req_proc_ctx(this);
        if(incoming_req == NULL) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              FMTu64" "FMT_VPLNet_addr_t":%u alloc failed.",
                              device_id, VAL_VPLNet_addr_t(addr), port);
            goto exit;
        }
    }

    if(reqlen == 0) {
        // If no command yet in progress, get command header.
        *buf_out = (void*)(incoming_req->header + req_so_far);
        rv = VSS_HEADER_SIZE - req_so_far;
    }
    else {
        if(incoming_req->body == NULL) {
            incoming_req->body = (char*)calloc(reqlen, 1);
            if(incoming_req->body == NULL) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  FMTu64" "FMT_VPLNet_addr_t":%u alloc failed.",
                                  device_id, VAL_VPLNet_addr_t(addr), port);
                goto exit;
            }
        }
        *buf_out = (void*)(incoming_req->body + (req_so_far - VSS_HEADER_SIZE));
        rv = reqlen - (req_so_far - VSS_HEADER_SIZE);
    }

 exit:
    return rv;
}

void vss_client::handle_received(int bufsize)
{
    // Accept the bytes received.
    req_so_far += bufsize;


    if(req_so_far < VSS_HEADER_SIZE) {
        // Do nothing yet.
    }
    else if(req_so_far == VSS_HEADER_SIZE) {
        // Record client device ID.
        if(device_id != vss_get_device_id(incoming_req->header)) {
            if(device_id == 0) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  FMTu64" "FMT_VPLNet_addr_t":%u client device identified.",
                                  vss_get_device_id(incoming_req->header), VAL_VPLNet_addr_t(addr), port);
            }
            else {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  FMTu64" "FMT_VPLNet_addr_t":%u client device identity changed from "FMTu64".",
                                  vss_get_device_id(incoming_req->header), VAL_VPLNet_addr_t(addr), port,
                                  device_id);
            }
            device_id = vss_get_device_id(incoming_req->header);
        }

        // Stop receive until header verified.
        receiving = false;
        server.changeNotice();
        VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                           FMTu64" "FMT_VPLNet_addr_t":%u Pause recv to verify header.",
                           device_id, VAL_VPLNet_addr_t(addr), port);        
        VPLMutex_Lock(&mutex);
        tasks_pending++;
        VPLMutex_Unlock(&mutex);
        server.addTask(verify_incoming_header_helper, this);
    }
    else if(reqlen > 0 && req_so_far == (reqlen  + VSS_HEADER_SIZE)) {
        VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                           FMTu64" "FMT_VPLNet_addr_t":%u Pause recv to handle request.",
                           device_id, VAL_VPLNet_addr_t(addr), port);

        VPLMutex_Lock(&mutex);
        vss_req_proc_ctx* req = incoming_req;
        // Stop receive until request processed.
        receiving = false;
        server.changeNotice();
        incoming_req = NULL;
        pending_cmds++;
        VPLMutex_Unlock(&mutex);

        server.notifyReqReceived(req);
    }
}

void verify_incoming_header_helper(void* vpclient)
{
    vss_client* pclient = (vss_client*)(vpclient);
    pclient->verify_incoming_header();
}

void vss_client::verify_incoming_header()
{
    int rc;
    bool verify_device = false;
    u64 user_id;
    u64 device_id = vss_get_device_id(incoming_req->header);

    VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                       FMTu64" "FMT_VPLNet_addr_t":%u Verifying header.",
                       device_id, VAL_VPLNet_addr_t(addr), port);        
        
    // Get session info for request.
    incoming_req->session = server.get_session(vss_get_handle(incoming_req->header));
    if(incoming_req->session == NULL) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u No session for handle "FMTu64".",
                          device_id, VAL_VPLNet_addr_t(addr), port, 
                          vss_get_handle(incoming_req->header));
        
        recv_error = true;
        // Attempt to send error message.
        char* resp = generate_error(VSSI_NOLOGIN);
        if(resp) {
            vss_set_xid(resp, vss_get_xid(incoming_req->header));
            vss_set_handle(resp, vss_get_handle(incoming_req->header));
            // Can't sign this response.
            VPLMutex_Lock(&mutex);
            pending_cmds++;
            VPLMutex_Unlock(&mutex);
            put_response(resp);
        }
        delete incoming_req;
        incoming_req = NULL;
        goto exit;
    }
    user_id = incoming_req->session->get_uid();
    
    // Verify the header as good.
    rc = incoming_req->session->verify_header(incoming_req->header, verify_device);
    if (rc != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u Header auth failed: %d.",
                          device_id, VAL_VPLNet_addr_t(addr), port, rc);
    }
    else if (verify_device && !server.isDeviceLinked(user_id, device_id)) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u connect from unlinked device: %d.",
                          device_id, VAL_VPLNet_addr_t(addr), port, rc);
        rc = VSSI_PERM;
    }

    if(rc != 0 ) {
        recv_error = true;
        // Attempt to send back an error message.
        char* resp = generate_error(rc);
        if(resp) {
            vss_set_xid(resp, vss_get_xid(incoming_req->header));
            vss_set_handle(resp, vss_get_handle(incoming_req->header));
            // Can't sign this response.
            VPLMutex_Lock(&mutex);
            pending_cmds++;
            VPLMutex_Unlock(&mutex);
            put_response(resp);
        }

        delete incoming_req;
        incoming_req = NULL;
        goto exit;
    }

    // Determine length of incoming request body.
    reqlen = vss_get_data_length(incoming_req->header);

    // Check that the request length is reasonable.
    if(reqlen > 2<<20) {
        // More than 2M is unreasonable.
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u request length %u not reasonable.",
                          device_id, VAL_VPLNet_addr_t(addr), port, reqlen);

        recv_error = true;
        // Attempt to send back an error message.
        char* resp = generate_error(VSSI_INVALID);
        if(resp) {
            vss_set_xid(resp, vss_get_xid(incoming_req->header));
            vss_set_handle(resp, vss_get_handle(incoming_req->header));
            // Can't sign this response.
            VPLMutex_Lock(&mutex);
            pending_cmds++;
            VPLMutex_Unlock(&mutex);
            put_response(resp);
        }

        delete incoming_req;
        incoming_req = NULL;
        goto exit;        
    }

    // If no body, begin request processing immediately.
    if(reqlen == 0) {
        vss_req_proc_ctx* req;

        VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                           FMTu64" "FMT_VPLNet_addr_t":%u Body-less request. Straight to processing.",
                           device_id, VAL_VPLNet_addr_t(addr), port);        

        VPLMutex_Lock(&mutex);
        pending_cmds++;
        req = incoming_req;
        incoming_req = NULL;
        VPLMutex_Unlock(&mutex);

        server.notifyReqReceived(req);
    }
    else {
        // Re-enable receive to get request body.
        VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                           FMTu64" "FMT_VPLNet_addr_t":%u Restart receive for request body [%u].",
                           device_id, VAL_VPLNet_addr_t(addr), port, reqlen);
        receiving = true;
        server.changeNotice();
    }

 exit:
    // Done with deferred task.
    VPLMutex_Lock(&mutex);
    tasks_pending--;
    VPLMutex_Unlock(&mutex);

    return;
}

void vss_client::verify_request(vss_req_proc_ctx*& context)
{
    VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                       FMTu64" "FMT_VPLNet_addr_t":%u Verifying request.",
                       device_id, VAL_VPLNet_addr_t(addr), port);        

    // Verify the body as good, if there's body data.
    int rc = context->session->validate_request_data(context->header,
                                                     context->body);
    if(rc != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMT_VPLNet_addr_t":%u Data auth failed.",
                          device_id, VAL_VPLNet_addr_t(addr), port);

        recv_error = true;
        // Attempt to send back an error message.
        char* resp = generate_error(VSSI_BADSIG);
        if(resp) {
            vss_set_xid(resp, vss_get_xid(context->header));
            vss_set_handle(resp, vss_get_handle(context->header));
            // Can't sign this response.
            put_response(resp);
        }
        delete context;
        context = NULL;

        goto exit;
    }

    new_connection = false;
        
    // Request now gets procesed by caller.
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        FMTu64" "FMT_VPLNet_addr_t":%u request %d xid %u processing.",
                        device_id, VAL_VPLNet_addr_t(addr), port,
                        vss_get_command(context->header),
                        vss_get_xid(context->header));
    cmd_cnt++;

    // Resume receive.
    VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                       FMTu64" "FMT_VPLNet_addr_t":%u Restart receive for next request.",
                       device_id, VAL_VPLNet_addr_t(addr), port);
    receiving = true;
    server.changeNotice();

 exit:
    return;
}

void vss_client::do_send()
{
    bool failed = false;
    VPLMutex_Lock(&mutex);
    
    // Send as much as possible of first message to the client.
    if(!send_queue.empty()) {
        pair<size_t, const char*> response = send_queue.front();
    
        VPLMutex_Unlock(&mutex);
        int rc = VPLSocket_Send(sockfd, response.second + sent_so_far,
                                response.first - sent_so_far);
        VPLMutex_Lock(&mutex);

        if(rc > 0) {
            sent_so_far += rc;
            send_cnt += rc;
            
            if(sent_so_far == response.first) {
                free((void*)(response.second));
                send_queue.pop();
                sent_so_far = 0;
            }
        }
        else if(rc == 0) {
            failed = true;
        }
        else {
            switch(rc) {
            case VPL_ERR_AGAIN:
                // temporary setback;
                break;

            default:
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  FMTu64" "FMT_VPLNet_addr_t":%u send failed: %d.",
                                  device_id, VAL_VPLNet_addr_t(addr), port, rc);
                failed = true;

            }
        }
    }
    
    // When out of replies to send, say so.
    if(send_queue.empty()) {
        sending = false;
        // If recv failed, disconnect now that the send queue is empty.
        if(recv_error) {
            failed = true;
        }
    }

    last_active = VPLTime_GetTimeStamp();
    VPLMutex_Unlock(&mutex);

    // Disconnect if connection failed.
    if(failed) {
        disconnect();
    }
}

void vss_client::put_response(const char* resp, bool proxy_reply, bool async_reply)
{
    VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                       FMTu64" "FMT_VPLNet_addr_t":%u put response %x xid %u.",
                       device_id, VAL_VPLNet_addr_t(addr), port,
                       vss_get_command(resp), vss_get_xid(resp));

    VPLMutex_Lock(&mutex);
    if(!proxy_reply && !async_reply) {
        pending_cmds--;
    }
    
    if(disconnected) {
        free((void*)resp);
    }
    else {
        pair<size_t, const char*> response = make_pair(vss_get_data_length(resp) + VSS_HEADER_SIZE, resp);
        
        send_queue.push(response);
        if(!sending) {
            sending = true;
            server.changeNotice();
        }
    }

    last_active = VPLTime_GetTimeStamp();
    VPLMutex_Unlock(&mutex);
}

bool vss_client::inactive()
{
    bool rv = false;

    // Handle receive error induced disconnect here.
    if(recv_error && send_queue.empty()) {
        disconnect();
    }

    VPLMutex_Lock(&mutex);

    // Can't be inactive if there are pending commands (references to self).
    if(tasks_pending == 0 && pending_cmds == 0 && objects.empty()) {
        if(disconnected) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              FMTu64" "FMT_VPLNet_addr_t":%u VSSI client disconnected.",
                              device_id, VAL_VPLNet_addr_t(addr), port);
            rv = true;
        }
        else if(send_queue.empty() &&
                VPLTime_GetTimeStamp() > last_active + inactive_timeout) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              FMTu64" "FMT_VPLNet_addr_t":%u VSSI client timeout after "FMT_VPLTime_t"us.",
                              device_id, VAL_VPLNet_addr_t(addr), port,
                              VPLTime_DiffClamp(VPLTime_GetTimeStamp(), last_active));
            rv = true;
        }
        else if(new_connection && req_so_far > 0 &&
                VPLTime_GetTimeStamp() > last_active + VPLTime_FromSec(10)) {
            // New connection received data but first request incomplete
            // for more than 10 second without data received? Consider this an
            // attack and drop connection.
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              FMTu64" "FMT_VPLNet_addr_t":%u VSSI client first request incomplete for "FMT_VPLTime_t"us. Drop connection assuming DOS attack.",
                              device_id, VAL_VPLNet_addr_t(addr), port,
                              VPLTime_DiffClamp(VPLTime_GetTimeStamp(), last_active));
            rv = true;
        }
    }

    VPLMutex_Unlock(&mutex);

    return rv;
}

void vss_client::associate_object(vss_object * object)
{
    VPLMutex_Lock(&mutex);
    objects.insert(object);
    VPLMutex_Unlock(&mutex);
}

void vss_client::disassociate_object(vss_object * object)
{
    VPLMutex_Lock(&mutex);
    objects.erase(object);
    VPLMutex_Unlock(&mutex);
}
