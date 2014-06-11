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

#include <ctype.h>

#include "vpl_th.h"
#include "vpl_socket.h"
#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"

#include "vssi.h"
#include "vssi_http.h"
#include "vssi_error.h"

// Set to 1 to have all request and response headers logged.
#define DEBUG_HTTP_TRANSACTIONS 0

// Maximum in-flight requests to allow per server connection (socket).
#define HTTP_PIPELINE_MAX 4

extern void (*VSSI_NotifySocketChangeFn)(void);
extern VSSI_HTTPConnectionNode* http_connection_list_head;
extern VPLMutex_t vssi_mutex;

VSSI_Result VSSI_AddHTTPCommand(VSSI_HTTPConnectionNode* connection,
                                VSSI_PendingHTTPCommand* cmd);
int VSSI_SendHTTP(VSSI_HTTPConnection* connection,
                  VSSI_PendingHTTPCommand* cmd);
void VSSI_CompleteHTTPCommand(VSSI_HTTPConnection* connection,
                           int result);
void VSSI_ParseHTTPReply(VSSI_PendingHTTPCommand* cmd);

VSSI_Result VSSI_HTTPGetServerConnections(VSSI_ObjectState* object)
{
    VSSI_HTTPConnectionNode* connection = NULL;
    int i, j;
    VSSI_Result rv = VSSI_SUCCESS;

    // Associate each component with a server
    VPLMutex_Lock(&vssi_mutex);
    for(i = 0; i < object->access.http.num_components; i++) {
        // Find any existing connection to the indicated server and port.
        connection = http_connection_list_head;
        while(connection != NULL &&
              memcmp(&(connection->inaddr), &(object->access.http.components[i].server_inaddr), 
                     sizeof(connection->inaddr)) != 0) {
            connection = connection->next;
        }

        // If no existing server connection found, create a new one.
        if(connection == NULL) {
            int rc;

            connection = (VSSI_HTTPConnectionNode*)calloc(sizeof(VSSI_HTTPConnectionNode), 1);
            if(connection == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed memory allocation on creating server connection node.");
                rv = VSSI_NOMEM;
                goto exit;
            }
            
            rc = VPLMutex_Init(&(connection->send_mutex));
            if(rc != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed mutex init (%d) for server connection.",
                                 rc);
                free(connection);
                rv = VSSI_NOMEM; // well... what else could it be?
                goto exit;
            }
            for(j = 0; j < MAX_HTTP_CONNS; j++) {
                connection->connection[j].conn_id = VPLSOCKET_INVALID;
                connection->connection[j].last_active = VPLTime_GetTimeStamp();
                connection->connection[j].inactive_timeout = VSSI_HTTP_CONNECTION_TIMEOUT;
            }
            
            connection->next = NULL;
            memcpy(&(connection->inaddr), &(object->access.http.components[i].server_inaddr), 
                   sizeof(connection->inaddr));
            
            // Add to the connections list.
            connection->next = http_connection_list_head;
            http_connection_list_head = connection;
            
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                              "Server connection struct made to server "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t".",
                              VAL_VPLNet_addr_t(connection->inaddr.addr),
                              VPLNet_port_ntoh(connection->inaddr.port));
        }
        else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                              "Existing server connection struct found to server "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t".",
                              VAL_VPLNet_addr_t(connection->inaddr.addr),
                              VPLNet_port_ntoh(connection->inaddr.port));
        }
        object->access.http.components[i].server_connection = connection;
    }

 exit:
    VPLMutex_Unlock(&vssi_mutex);

    return rv;
}

VSSI_Result VSSI_HTTPRead(VSSI_ObjectState* object,
                          const char* name,
                          u64 offset, 
                          u32* length,
                          char* buf,
                          VSSI_Callback callback,
                          void* ctx)
{
    VSSI_Result rv = VSSI_SUCCESS;
    int i;
    VSSI_PendingHTTPCommand* cmd = NULL;

    // Confirm the object has the designated component. (else VSSI_ACCESS)
    for(i = 0; i < object->access.http.num_components; i++) {
        if(strcmp(object->access.http.components[i].name, name) == 0) {
            break;
        }
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                            "Component %s != %s at location %s/%s.",
                            name, object->access.http.components[i].name, 
                            object->access.http.components[i].server,
                            object->access.http.components[i].uri);
            
    }
    if(i == object->access.http.num_components) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "No such component %s for object %p",
                         name, object);
        rv = VSSI_ACCESS;
        goto exit;
    }
    
    // Allocate pending command state
    cmd = (VSSI_PendingHTTPCommand*)calloc(sizeof(VSSI_PendingHTTPCommand), 1);
    if(cmd == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "Could not allocate pending command state.");
        rv = VSSI_NOMEM;
        goto exit;
    }

    // Fill-in with details
    cmd->offset = offset;
    cmd->length = length;
    cmd->buf = buf;
    cmd->callback = callback;
    cmd->ctx = ctx;
    cmd->component = &(object->access.http.components[i]);

    // Queue to the component's server connection
    rv = VSSI_AddHTTPCommand(object->access.http.components[i].server_connection,
                             cmd);
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "Could not add cmd to pending queue.");
        goto dealloc;
    }

    // Attempt to send commands to server.
    VSSI_SendHTTPCommands(object->access.http.components[i].server_connection);

    goto exit;
 dealloc:
    free(cmd);
 exit:
    return rv;
}

VSSI_Result VSSI_AddHTTPCommand(VSSI_HTTPConnectionNode* connection,
                                VSSI_PendingHTTPCommand* cmd)
{
    VSSI_Result rv = VSSI_SUCCESS;

    VPLMutex_Lock(&(connection->send_mutex));

    cmd->next = NULL;
    if(connection->pending_tail) {
        connection->pending_tail->next = cmd;
    }
    else {
        connection->pending_head = cmd;
    }
    connection->pending_tail = cmd;

    VPLMutex_Unlock(&(connection->send_mutex));

    return rv;
}

void VSSI_SendHTTPCommands(VSSI_HTTPConnectionNode* connection)
{
    int i = 0;
    int rc = 0;
    VSSI_PendingHTTPCommand* cmd = NULL;

    if(connection->connection_lost) {
        goto fail;
    }

    VPLMutex_Lock(&(connection->send_mutex));

    // Send as many commands as each connection will allow.
    // Pipelining connections allow N; non-pipelining connections 1.
    while(connection->pending_head != NULL && i < MAX_HTTP_CONNS) {
        // Skip connection if not eligible to send now.
        if(connection->connection[i].stale_connection ||           
           connection->connection[i].sent_count >= HTTP_PIPELINE_MAX ||
           (!connection->connection[i].pipelining && 
            connection->connection[i].sent_count > 0)
           ) {
            i++;
        }
        else {
            cmd = connection->pending_head;

            // Send under mutex to preserve message boundaries.
            rc = VSSI_SendHTTP(&(connection->connection[i]), cmd);
            
            // Remove command from pending on success or fatal error.
            // Command may retry later on non-fatal error.
            if(rc <= 0) {
                connection->pending_head = cmd->next;
                if(connection->pending_head == NULL) {
                    connection->pending_tail = NULL;
                }
            }
            
            // Put command on sent queue on success.
            // Stop sending on any error.
            if(rc == 0) {
                cmd->next = NULL;
                if(connection->connection[i].sent_tail != NULL) {
                    connection->connection[i].sent_tail->next = cmd;
                }
                else {
                    connection->connection[i].sent_head = cmd;
                }
                connection->connection[i].sent_tail = cmd;
                connection->connection[i].sent_count++;
                cmd = NULL;
            }
            else {
                if(rc > 0) {
                    // non-fatal error. Leave in the pending queue.
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Got warning %d sending command. Waiting until later.", rc);
                    cmd = NULL;
                }
                else {
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Got Error %d sending command. Aborting command.", rc);
                }
                break;
            }
        }
    }

    VPLMutex_Unlock(&(connection->send_mutex));

    // On unrecoverable errors, call command's callback.
 fail:
    if(rc < 0 && cmd) {
        if(cmd->callback) {
            (cmd->callback)(cmd->ctx, rc);
        }
        free(cmd);
    }    
}

static const char VSSI_HTTPRequestTemplate[] = 
    "GET %s HTTP/1.1\r\n"
    "Host: %s:%u\r\n"
    "User-Agent: vssi/0.1\r\n"
    "Accept: */*\r\n"
    "Range: bytes="FMTu64"-"FMTu64"\r\n"
    "\r\n";
int VSSI_SendHTTP(VSSI_HTTPConnection* connection,
                  VSSI_PendingHTTPCommand* cmd)
{
    char *request = NULL;
    size_t len;
    int rv = 0;
    int rc;

    // Reconnect to the server as needed.
    connection->last_active = VPLTime_GetTimeStamp();
    reconnect_server(&(connection->conn_id), 
                     &(cmd->component->server_inaddr), 0, NULL,
                     -1, -1);  // use default timeouts in VPLSocket_Connect*()
    if(VPLSocket_Equal(connection->conn_id, VPLSOCKET_INVALID)) {
        // Fail to connect. Issue comm error immediately.
        rv = VSSI_COMM; // no retry - server unreachable
        goto fail;
    }

    // Compose HTTP request according to the active command.
    len = snprintf(NULL, 0, VSSI_HTTPRequestTemplate,
                   cmd->component->uri,
                   cmd->component->server,
                   VPLNet_port_ntoh(cmd->component->server_inaddr.port),
                   cmd->offset,
                   cmd->offset + 
                   (u64)(*(cmd->length) - 1));
    request = (char*)malloc(len + 1);
    if(request == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "Failed to allocate memory to send request.");
        rv = VSSI_NOMEM; // no retry - no memory
        goto fail;
    }
    
    snprintf(request, len + 1, VSSI_HTTPRequestTemplate,
             cmd->component->uri,
             cmd->component->server,
             VPLNet_port_ntoh(cmd->component->server_inaddr.port),
             cmd->offset,
             cmd->offset + 
             (u64)(*(cmd->length) - 1));
    
#if DEBUG_HTTP_TRANSACTIONS
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, 
                      "Sending HTTP request:\n%s",
                      request);
#endif

    // Send the HTTP request
    rc = VSSI_Sendall(connection->conn_id, request, len);
    if(rc != len) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "Failed to send HTTP request.");
        rv = VSSI_COMM; // will retry
    }

 fail:
    if(request) {
        free(request);
    }

    return rv;
}

// Receive buffer for all HTTP replies
// Due to program structure, only one receive operation is active at a time.
// (select loop by library user must be in a single thread)
#define HTTP_BUF_SIZE 16384
static char http_inbuf[HTTP_BUF_SIZE];
void VSSI_ReceiveHTTPResponse(VSSI_HTTPConnectionNode* node, int id)
{
    int rc = 0;
    size_t beginline = 0;
    size_t linelen = 0;
    int i;
    VSSI_PendingHTTPCommand* cmd = NULL;
    void* tmp;
                
    // Receive chunks of data until nothing more to receive
    do {
        // Receive a chunk of data
        if(beginline == rc) {
            rc = VPLSocket_Recv(node->connection[id].conn_id,
                                http_inbuf, HTTP_BUF_SIZE);
            if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
                // Nothing more for now.
                VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                    "Nothing to receive now.");
               
                break;
            }
            else if(rc <= 0) {
                // Socket closed or had some other error.
                if(rc == 0) {
                    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                      "Got socket closed for connection "FMT_VPLSocket_t".",
                                      VAL_VPLSocket_t(node->connection[id].conn_id));
                }
                else {
                    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                      "Got socket error %d for connection "FMT_VPLSocket_t".",
                                      rc, VAL_VPLSocket_t(node->connection[id].conn_id));
                }
                // Complete the current command (if any) with a comm error.
                VSSI_HTTPCommError(node, id, VSSI_COMM);
                break;
            }

            beginline = 0;
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Received %d bytes.", rc);
        }

        VPLMutex_Lock(&(node->send_mutex));
        // Advance active command if necessary.
        cmd = node->connection[id].active_cmd;
        if(cmd == NULL) {
            cmd = node->connection[id].sent_head;
            if(cmd != NULL) {
                node->connection[id].active_cmd = cmd;
                node->connection[id].sent_head = cmd->next;
                if(node->connection[id].sent_head == NULL) {
                    node->connection[id].sent_tail = NULL;
                }
            }
        }
        VPLMutex_Unlock(&(node->send_mutex));
            
        if(cmd == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Received %u bytes of data on socket "FMT_VPLSocket_t" without an active command.",
                             rc, VAL_VPLSocket_t(node->connection[id].conn_id));
            // Drop the bytes on the floor. Keep going.
            beginline = rc;
        }
        else {
            if(!cmd->collect_body) {
                // Parse the buffer into the response.
                // Break the input into lines on \n characters. May also get \r\n.
                // Empty line indicates end of headers.

                while(beginline < rc) {
                    linelen = 0;
                    for(i = beginline; i < rc; i++) {
                        linelen++;
                        if(http_inbuf[i] == '\n') {
                            break;
                        }
                    }
                                                            
                    // Add line fragment to partial line. Append if needed.
                    tmp = realloc(cmd->partial_line, 
                                  cmd->partial_line_len + linelen);
                    if(tmp == NULL) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                                         "Failed to allocate buffer for reply line.");
                        // Command reply receive fails.
                        VSSI_HTTPCommError(node, id, VSSI_NOMEM);
                    }
                    cmd->partial_line = (char*)tmp;
                    memcpy(cmd->partial_line + cmd->partial_line_len,
                           http_inbuf + beginline, linelen);
                    cmd->partial_line_len += linelen;
                    beginline += linelen;
                    
                    if(cmd->partial_line[cmd->partial_line_len - 1] != '\n') {
                        // Not a complete line. Get more data.
                        break;
                    }
                    
                    // Strip newline characters from the end
                    while(cmd->partial_line_len > 0 &&
                          (cmd->partial_line[cmd->partial_line_len - 1] == '\n' ||
                           cmd->partial_line[cmd->partial_line_len - 1] == '\r')) {
                        cmd->partial_line[cmd->partial_line_len - 1] = '\0';
                        cmd->partial_line_len--;
                    }
                    
                    if(cmd->response_line == NULL) {
                        // Collect response line
                        cmd->response_line = cmd->partial_line;
                        cmd->partial_line = NULL;
                        cmd->partial_line_len = 0;
                    }
                    else {
                        // Check for end of headers
                        if(cmd->partial_line_len == 0) {
                            // Blank line after headers indicates end of headers.
                            free(cmd->partial_line);
                            cmd->partial_line = NULL;
                            cmd->collect_body = 1;
                            
                            // Parse the response for reply body details
                            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                                "Got complete reply on socket "FMT_VPLSocket_t".",
                                                VAL_VPLSocket_t(node->connection[id].conn_id));
                            VSSI_ParseHTTPReply(cmd);
                            break; // Continue with collecting body data.
                        }
                        else {
                            // Collect header. Expand header array by 1.
                            tmp = realloc(cmd->headers, 
                                          sizeof(char*) * (cmd->num_headers + 1));
                            if(tmp == NULL) {
                                VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                                                 "Failed to collect header "FMTu_size_t" of reply.",
                                                 cmd->num_headers + 1);
                                // Command reply receive fails.
                                VSSI_HTTPCommError(node, id, VSSI_NOMEM);
                            }
                            cmd->headers = (char**)tmp;
                            cmd->headers[cmd->num_headers] = cmd->partial_line;
                            cmd->num_headers++;
                            cmd->partial_line = NULL;
                            cmd->partial_line_len = 0;
                        }
                    }
                }
            }

            if(cmd->collect_body) {
                while(cmd->chunked && beginline < rc) {
                    // Need to absorb the chunks.
                    if(cmd->chunk_length == 0) {
                        // Get a chunk header
                        linelen = 0;
                        for(i = beginline; i < rc; i++) {
                            linelen++;
                            if(http_inbuf[i] == '\n') {
                                break;
                            }
                        }
                                
                        // Add line fragment to partial line. Append if needed.
                        tmp = realloc(cmd->partial_line, 
                                      cmd->partial_line_len + linelen);
                        if(tmp == NULL) {
                            VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                                             "Failed to allocate buffer for reply line.");
                            // Command reply receive fails.
                            VSSI_HTTPCommError(node, id, VSSI_NOMEM);
                        }
                        cmd->partial_line = (char*)tmp;
                        memcpy(cmd->partial_line + cmd->partial_line_len,
                               http_inbuf + beginline, linelen);
                        cmd->partial_line_len += linelen;
                        beginline += linelen;
                        
                        if(cmd->partial_line[cmd->partial_line_len - 1] != '\n') {
                            // Not a complete line. Get more data.
                            break;
                        }
                        
                        // Strip newline characters from the end
                        while(cmd->partial_line_len > 0 &&
                              (cmd->partial_line[cmd->partial_line_len - 1] == '\n' ||
                               cmd->partial_line[cmd->partial_line_len - 1] == '\r')) {
                            cmd->partial_line[cmd->partial_line_len - 1] = '\0';
                            cmd->partial_line_len--;
                        }
                        
                        if(cmd->partial_line_len == 0) {
                            // end of chunks on blank line.
                            cmd->chunked = 0;
                            break;
                        }
                        
                        // Get chunk size, a hex value starting the line.
                        cmd->chunk_length = strtoull(cmd->partial_line, 0, 16);
                        free(cmd->partial_line);
                        cmd->partial_line = NULL;
                        cmd->partial_line_len = 0;
                    }

                    if(cmd->chunk_length != 0) {
                        // absorb up-to chunk_length bytes, plus CRLF
                        if(rc - beginline >= 2 + cmd->chunk_length - cmd->chunk_so_far) {
                            beginline += 2 + cmd->chunk_length - cmd->chunk_so_far;
                            cmd->chunk_length = 0;
                            cmd->chunk_so_far = 0;
                        }
                        else {
                            cmd->chunk_so_far += rc - beginline;
                            beginline = rc;
                        }
                    }
                }
                
                if(!cmd->chunked) {
                    // Accept received bytes up through reply length.
                    // Copy received bytes up through space in receive buffer.
                    // Throw away bytes received in excess of initial request.
                    size_t accept_len = rc - beginline;
                    size_t copy_len = rc - beginline;
                    if(*(cmd->length) > cmd->reply_so_far) {
                        // Don't overfill buffer.
                        if(copy_len > *(cmd->length) - cmd->reply_so_far) {
                            copy_len = *(cmd->length) - cmd->reply_so_far;
                        }
                        // No more bytes than expected.
                        if(copy_len > cmd->reply_length - cmd->reply_so_far) {
                            copy_len = cmd->reply_length - cmd->reply_so_far;
                        }

                        if(copy_len > 0) {
                            memcpy(cmd->buf + cmd->reply_so_far,
                                   http_inbuf + beginline,
                                   copy_len);
                            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                                "Copied "FMTu_size_t" bytes into receive buffer Have "FMTu_size_t"/%d bytes of reply.",
                                                copy_len, cmd->reply_so_far + copy_len, *(cmd->length));
                        }
                    }
                    if(cmd->reply_length > cmd->reply_so_far) {
                        // Watch for excess bytes.
                        if(accept_len > cmd->reply_length - cmd->reply_so_far) {
                            accept_len = cmd->reply_length - cmd->reply_so_far;
                        }
                        cmd->reply_so_far += accept_len;
                        beginline += accept_len;
                        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                            "Absorbed "FMTu_size_t" bytes of reply. Received "FMTu_size_t"/"FMTu_size_t" bytes.",
                                            accept_len, cmd->reply_so_far,
                                            cmd->reply_length);
                    }
                }

                // Done if all bytes desired are received.
                if(!cmd->chunked &&
                   cmd->reply_so_far == cmd->reply_length) {
                    // Complete the command.
                    if(cmd->reply_length <= *(cmd->length)) {
                        *(cmd->length) = cmd->reply_length;
                    }
                    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                        "Completed command reply. "FMTu_size_t"bytes remaining",
                                        rc - beginline);
                    if(cmd->conn_closed) {
                        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                            "Trashing "FMTu_size_t" bytes on closed connection.",
                                            rc - beginline);
                        beginline = rc;
                    }
                    VSSI_CompleteHTTPCommand(&(node->connection[id]), 
                                             cmd->result);
                }
            } // Collecting body data.
        } // Add received to a reply.
    } while(rc > 0);
}

void VSSI_HTTPCommError(VSSI_HTTPConnectionNode* node, int id, int result)
{
    // Close the socket.
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Closing socket "FMT_VPLSocket_t" due to command error %d",
                      VAL_VPLSocket_t(node->connection[id].conn_id), result);

    // Complete the current command (if any) with a comm error.
    VSSI_CompleteHTTPCommand(&(node->connection[id]), result);

    VPLMutex_Lock(&(node->send_mutex));

    // Move all send commands back to the pending queue.
    if(node->connection[id].sent_head != NULL) {
        node->connection[id].sent_tail->next = node->pending_head;
        if(node->pending_tail == NULL) {
            node->pending_tail = node->connection[id].sent_tail;
        }
        node->pending_head = node->connection[id].sent_head;
        node->connection[id].sent_head = NULL;
        node->connection[id].sent_tail = NULL;
        node->connection[id].sent_count = 0;
    }

    // Close the connection and clear stale flag.
    VPLSocket_Close(node->connection[id].conn_id);        
    node->connection[id].conn_id = VPLSOCKET_INVALID;
    node->connection[id].stale_connection = 0;

    VPLMutex_Unlock(&(node->send_mutex));

    VSSI_NotifySocketChangeFn();
}

void VSSI_ParseHTTPReply(VSSI_PendingHTTPCommand* cmd)
{
    int i;
    char* val_str;
    int value;

    // (debug) Log the reply header.
#if DEBUG_HTTP_TRANSACTIONS
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Reply header information:");
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "\t%s", cmd->response_line);
    for(i = 0; i < cmd->num_headers; i++) {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "\t%s", cmd->headers[i]);
    }
#endif

    // Determine reply result.
    // Response line must start "HTTP/1.1 " followed by result code
    if(cmd->response_line &&
       (val_str = strchr(cmd->response_line, ' ')) != NULL) {
        val_str++;
        value = atoi(val_str);

        switch(value) {
        case 200: // Total success... short file?
        case 206:
            cmd->result = VSSI_SUCCESS;
            break;
        case 416: // Out-of-range. Treat as EOF.
            cmd->result = VSSI_EOF;
            break;
        case 400: // File not found. That means it's an access error.
        default:
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "HTTP Access error on code %d.", value);
            cmd->result = VSSI_ACCESS;
            break;

        }
    }
    else {
        // No response line? Assume not found error (like 400 reply).
        cmd->result = VSSI_ACCESS;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP Access error on no response line.");
    }
    
    // Parse haders for interesting information

    for(i = 0; i < cmd->num_headers; i++) {

        // Determine if the response has chunked transfer encoding.
        if(strstr(cmd->headers[i], "Transfer-Encoding:")) {
            val_str = strchr(cmd->headers[i], ':');
            val_str++;
            if(strstr(val_str, "chunked")) {
                cmd->chunked = true;
            }
        }

        // Determine body length for command reply.
        // Just need the "Content-Length" header value. Find it.
        // Lack of header leaves reply_length at 0.
        else if(strstr(cmd->headers[i], "Content-Length:")) {
            val_str = strchr(cmd->headers[i], ':');
            val_str++;
            while(val_str[0] != '\0' && isspace(val_str[0])) val_str++;
            cmd->reply_length = atoi(val_str);
        }

        // Determine if connection gets closed after this reply.
        if(strstr(cmd->headers[i], "Connection:")) {
            val_str = strchr(cmd->headers[i], ':');
            val_str++;
            while(val_str[0] != '\0' && isspace(val_str[0])) val_str++;
            if(strcmp(val_str, "close") == 0) {
                cmd->conn_closed = 1;
            }
        }
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Parsed reply. Got result %d, chunked: %d, "FMTu_size_t" bytes of body, close:%d.",
                      cmd->result, cmd->chunked, cmd->reply_length, cmd->conn_closed);
}

void VSSI_CompleteHTTPCommand(VSSI_HTTPConnection* connection,
                              int result)
{
    VSSI_PendingHTTPCommand* cmd = connection->active_cmd;

    if(cmd) {
        // Remove active cmd from connection.
        VPLMutex_Lock(&(cmd->component->server_connection->send_mutex));
        connection->active_cmd = NULL;
        connection->sent_count--;

        // If connection got closed, mark connection as stale.
        if(cmd->conn_closed) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Marking socket "FMT_VPLSocket_t" stale due to Connection: close header.",
                              VAL_VPLSocket_t(connection->conn_id));
            connection->stale_connection = 1;
            connection->pipelining = 0;
        }
        // If not closed, pipelining is go!
        else {
            connection->pipelining = 1;
        }

        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Completing command from socket "FMT_VPLSocket_t" with result %d.",
                          VAL_VPLSocket_t(connection->conn_id), result);              
        VPLMutex_Unlock(&(cmd->component->server_connection->send_mutex));


        // Call callback if needed
        if(cmd->callback) {
            (cmd->callback)(cmd->ctx, result);
        }

#if DEBUG_HTTP_TRANSACTIONS
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Completed response: %s.", cmd->headers[0]);
#endif

        // Free the command structure
        if(cmd->partial_line) free(cmd->partial_line);
        if(cmd->response_line) free(cmd->response_line);
        if(cmd->headers) {
            int i;
            for(i = 0; i < cmd->num_headers; i++) {
                if(cmd->headers[i]) {
                    free(cmd->headers[i]);
                }
            }
            free(cmd->headers);
        }
        free(cmd);
    }
}
