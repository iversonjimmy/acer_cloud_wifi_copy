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

/**
 * vssi.c
 *
 * Virtual Storage Server Interface: API Implementation
 */

#include "vpl_socket.h"
#include "vpl_net.h"
#include "vpl_th.h"
#include "vplex_trace.h"
#include "vplex_assert.h"

#include "vssi.h"
#include "vssi_error.h"
#include "vssi_vss.h"
#include "vssi_http.h"
#include "vssi_internal.h"

#include "cslsha.h"

#include <stdlib.h>

VPLMutex_t vssi_mutex = VPLMUTEX_SET_UNDEF;
bool vssi_init = false;
u64 vssi_device_id = 0;

VSSI_SessionNode* user_session_list_head = NULL;
VSSI_VSConnectionNode* vs_connection_list_head = NULL;
VSSI_HTTPConnectionNode* http_connection_list_head = NULL;
VSSI_ConnectProxyState* proxy_connection_list_head = NULL;
VSSI_SecureTunnelState* secure_tunnel_list_head = NULL;

static u32 get_unique_value(u32 seed);

void (*VSSI_NotifySocketChangeFn)(void) = NULL;

VSSI_Result VSSI_Init(u64 device_id,
                      void (*notify_change_fn)(void))
{
    VSSI_Result rv = VSSI_INIT;
    int rc;

    if(vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI already initialized. Not doing it twice.");
        rv = VSSI_SUCCESS;
        goto exit;
    }

    rc = VSSI_InternalInit();
    if(rc != VSSI_SUCCESS) {
        rv = rc;
        goto exit;
    }

    if(VPLMutex_Init(&vssi_mutex) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create mutex.");
        goto exit;
    }

    VSSI_NotifySocketChangeFn = notify_change_fn;
    vssi_device_id = device_id;

    vssi_init = true;
    rv = VSSI_SUCCESS;
 exit:
    return rv;
}

void VSSI_Cleanup(void)
{
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to cleanup non-initialized VSSI.");
        return;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                 "Cleaning up VSSI...");

    VPLMutex_Lock(&vssi_mutex);

    // clear the user and server session lists
    while(user_session_list_head != NULL) {
        VSSI_SessionNode* node = user_session_list_head;
        user_session_list_head = node->next;

        while(node->session.send_states != NULL) {
            VSSI_SendState* send_state = node->session.send_states;
            VSSI_ReplyPendingCommands(send_state, NULL, VSSI_ABORTED);
            VSSI_DestroySendStateUnlocked(send_state);
        }

        // Destroy all objects for this session.
        while(node->session.objects != NULL) {
            VSSI_ObjectState* object = node->session.objects;
            node->session.objects = object->next;
            VSSI_FreeObject(object);
        }

        free(node);
    }

    while(vs_connection_list_head != NULL) {
        VSSI_VSConnectionNode* node = vs_connection_list_head;
        vs_connection_list_head = node->next;

        VPLMutex_Lock(&(node->send_mutex));

        while(node->connections != NULL) {
            VSSI_VSConnection* connection = node->connections;
            node->connections = connection->next;
            if(!VPLSocket_Equal(connection->conn_id, VPLSOCKET_INVALID)) {
                VPLSocket_Close(connection->conn_id);
                connection->conn_id = VPLSOCKET_INVALID;
            }
            free(connection->incoming_reply);
            free(connection->send_msg);
            free(connection);
        }

        VPLMutex_Unlock(&(node->send_mutex));
        VPLMutex_Destroy(&(node->send_mutex));

        free(node);
    }

    while(http_connection_list_head != NULL) {
        VSSI_HTTPConnectionNode* node = http_connection_list_head;
        http_connection_list_head = node->next;

        free(node);
    }

    while(proxy_connection_list_head != NULL) {
        VSSI_ConnectProxyState* node = proxy_connection_list_head;
        proxy_connection_list_head = node->next;

        free(node);
    }

    while(secure_tunnel_list_head != NULL) {
        VSSI_SecureTunnelState* node = secure_tunnel_list_head;
        secure_tunnel_list_head = node->next;

        VSSI_InternalDestroyTunnel(node);
    }

    vssi_init = false;

    VPLMutex_Unlock(&vssi_mutex);
    VPLMutex_Destroy(&vssi_mutex);

    VSSI_InternalCleanup();

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Cleaning up VSSI...Done.");
}

int VSSI_SetParameter(VSSI_Param id, int value)
{
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        return VSSI_INIT;
    }

    return VSSI_InternalSetParameter(id, value);
}

int VSSI_GetParameter(VSSI_Param id, int* value_out)
{
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        return VSSI_INIT;
    }

    return VSSI_InternalGetParameter(id, value_out);
}

void VSSI_ForEachSocket(void (*fn)(VPLSocket_t, int, int, void*),
                        void* ctx)
{
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to process events on non-initialized VSSI.");
        return;
    }

    VSSI_InternalForEachSocket(fn, ctx);
}

void VSSI_HandleSockets(int (*recv_ready)(VPLSocket_t, void*),
                        int (*send_ready)(VPLSocket_t, void*),
                        void* ctx)
{
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to process events on non-initialized VSSI.");
        return;
    }

    VSSI_InternalHandleSockets(recv_ready, send_ready, ctx);
}

VPLTime_t VSSI_HandleSocketsTimeout(void)
{
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to get timeout on non-initialized VSSI.");
        return VPLTIME_INVALID;
    }

    return VSSI_InternalHandleSocketsTimeout();
}

void VSSI_NetworkDown(void)
{
    VSSI_InternalNetworkDown();
}

VSSI_Session VSSI_RegisterSession(u64 sessionHandle,
                                  const char* serviceTicket)
{
    VSSI_Session rv = NULL;
    VSSI_SessionNode* node = NULL;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        goto fail_init;
    }

    VPLMutex_Lock(&vssi_mutex);

    // Look for a session already registered with the same session handle.
    node = user_session_list_head;
    while(node != NULL &&
          node->session.handle != sessionHandle) {
        node = node->next;
    }

    // If found, increment refcount.
    // This may indicate misuse of sessions. print a warning as well.
    if(node != NULL) {
        node->session.refcount++;
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "VSSI session for login handle "FMTu64" now registered %d times. Check usage of VSSI sessions. VSSI sessions only need to be registered once.",
                          sessionHandle, node->session.refcount);
        rv = &(node->session);
    }
    else {
        // allocate a new session node.
        node =
            (VSSI_SessionNode*)calloc(sizeof(VSSI_SessionNode), 1);

        if(node != NULL) {
            rv = &(node->session);
            node->session.handle = sessionHandle;
            node->session.refcount = 1;
            memcpy(node->session.secret, serviceTicket, SERVICE_TICKET_LEN);

            // Compute encryption and signing keys:
            // Encryption: "Encryption Key <dev_id> <session_id>"
            // Signing:    "Signing Key <dev_id> <session_id>"
            {
                char text[128];
                snprintf(text, 128, "Encryption Key %016"PRIX64" %016"PRIX64,
                         vssi_device_id, sessionHandle);
                compute_hmac(text, strlen(text),
                             serviceTicket, node->session.encryptingKey,
                             CSL_AES_KEYSIZE_BYTES);
                         
            }
            {
                char text[128];
                snprintf(text, 128, "Signing Key %016"PRIX64" %016"PRIX64,
                         vssi_device_id, sessionHandle);
                compute_hmac(text, strlen(text),
                             serviceTicket, node->session.signingKey,
                             CSL_SHA1_DIGESTSIZE);                         
            }

            node->next = user_session_list_head;
            user_session_list_head = node;
        }
    }

    VPLMutex_Unlock(&vssi_mutex);
 fail_init:
    return rv;
}

VSSI_Result VSSI_EndSession(VSSI_Session session)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_UserSession* this_session = (VSSI_UserSession*)(session);
    VSSI_SessionNode* session_node;
    VSSI_SessionNode* prev_session_node;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        return VSSI_INIT;
    }

    VPLMutex_Lock(&vssi_mutex);

    if(!VSSI_SessionRegistered(this_session)) {
        rv = VSSI_NOTFOUND;
        goto exit;
    }

    if(this_session->connect_count > 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Session "FMTu64" has %d connections in progress. Can't end.",
                          this_session->handle, this_session->connect_count);
        rv = VSSI_OPENED;
        goto exit;
    }

    // Decrement refcount.
    // If refcount goes to 0, then clean up session.
    this_session->refcount--;
    if(this_session->refcount <= 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Session "FMTu64" ending.",
                            this_session->handle);

        // Abort any pending requests for this session's send states.
        while(this_session->send_states != NULL) {
            VSSI_SendState* send_state = this_session->send_states;
            VSSI_ReplyPendingCommands(send_state, NULL, VSSI_ABORTED);
            VSSI_DestroySendStateUnlocked(send_state);
        }

        // Destroy all objects for this session.
        while(this_session->objects != NULL) {
            VSSI_ObjectState* object = this_session->objects;
            this_session->objects = object->next;
            VSSI_FreeObject(object);
        }

        // Remove this user session from the list.
        session_node = user_session_list_head;
        prev_session_node = NULL;
        while(&(session_node->session) != this_session) {
            prev_session_node = session_node;
            session_node = session_node->next;
        }

        if(prev_session_node == NULL) {
            user_session_list_head = session_node->next;
        }
        else {
            prev_session_node->next = session_node->next;
        }
        free(session_node);
    }

 exit:
    VPLMutex_Unlock(&vssi_mutex);
    return rv;
}

u64 VSSI_GetVersion(VSSI_Object handle)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)(handle);

    return object->version;
}

u32 VSSI_GetObjectOptimalAccessSize(VSSI_Object handle)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)(handle);

    return object->request_size;

}

void VSSI_OpenProxiedConnection(VSSI_Session session,
                                const char* proxy_addr,
                                u16 proxy_port,
                                u64 destination_device,
                                u8 traffic_type,
                                int make_p2p,
                                VPLSocket_t* socket,
                                int* is_direct,
                                void* ctx,
                                VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_UserSession* this_session = (VSSI_UserSession*)session;
    *socket = VPLSOCKET_INVALID;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(!socket || !proxy_addr) {
        rv = VSSI_INVALID;
        goto fail;
    }

    // Attempt to make the proxy connection.
    {
        VSSI_ConnectProxyState* context = 
            (VSSI_ConnectProxyState*)calloc(sizeof(VSSI_ConnectProxyState), 1);
        if(context == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to allocate connect-proxy context");
            rv = VSSI_NOMEM;
            goto fail;
        }
        else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Requesting proxy connection to device "FMTu64" of type %d.",
                              destination_device, traffic_type);

            context->session = this_session;
            context->callback = callback;
            context->ctx = ctx;
            context->socket = socket;
            context->is_direct = is_direct;
            context->conn_id = VPLSOCKET_INVALID;
            context->p2p_listen_id = VPLSOCKET_INVALID;
            context->p2p_punch_id = VPLSOCKET_INVALID;
            context->p2p_conn_id = VPLSOCKET_INVALID;
            context->p2p_flags = make_p2p ? VSSI_PROXY_TRY_P2P : 0;
            context->type = traffic_type;
            context->destination_device = destination_device;
            
            VSSI_VSConnectProxy(context, proxy_addr, proxy_port);
            // VSSI_VSConnectProxy is now responsible for calling the callback.
        }
    }

    return;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }

}

static void OpenObject(VSSI_Session session,
                       const char* obj_xml,
                       u64 user_id,
                       u64 dataset_id,
                       const VSSI_RouteInfo* route_info,
                       bool use_xml,
                       u8 mode,
                       VSSI_Object* handle,
                       void* ctx,
                       VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_ObjectState* object = NULL;
    VSSI_UserSession* this_session = (VSSI_UserSession*)session;
    *handle = NULL;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Building a VSSI object for session "FMTu64".",
                      this_session->handle);

    if(use_xml) {
        object = VSSI_BuildObject(obj_xml, this_session);
    }
    else {
        object = VSSI_BuildObject2(user_id, dataset_id, route_info,
                                   this_session);
    }
    if(object == NULL) {
        rv = VSSI_NOMEM;
        goto fail;
    }

    object->mode = mode;
    object->files = NULL;
    object->num_files = 0;

    switch(object->proto) {
    case  VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.handle = handle;
        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_ON_FAIL;
        cmd_data.command = VSS_OPEN;

        VSSI_SendOpenCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        // Associate object with its server(s).
        rv = VSSI_HTTPGetServerConnections(object);
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to associate HTTP object with server(s).");
            goto fail_dealloc;
        }

        // Set object parameters and retval handle.
        object->version = 0;
        *handle = (VSSI_Object)(object);

        // Object is now open. Call callback if provided.
        if(callback) {
            (callback)(ctx, rv);
        }
        break;

    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unknown protocol for URL %d.",
                         object->proto);
        rv = VSSI_INVALID;
        goto fail_dealloc;
        break;
    }

    goto exit;
 fail_dealloc:
    // Destroy the failed object.
    VSSI_FreeObject(object);
 fail:
    // Call the callback.
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_OpenObject(VSSI_Session session,
                     const char* obj_xml,
                     u8 mode,
                     VSSI_Object* handle,
                     void* ctx,
                     VSSI_Callback callback)
{
    OpenObject(session, obj_xml, 0, 0, NULL, true, mode, handle, ctx, callback);
}

void VSSI_OpenObject2(VSSI_Session session,
                      u64 user_id,
                      u64 dataset_id,
                      const VSSI_RouteInfo* route_info,
                      u8 mode,
                      VSSI_Object* handle,
                      void* ctx,
                      VSSI_Callback callback)
{
    OpenObject(session, NULL, user_id, dataset_id, route_info, false,
        mode, handle, ctx, callback);
}

void VSSI_CloseObject(VSSI_Object handle,
                      void* ctx,
                      VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_ObjectState* object = (VSSI_ObjectState*)(handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if (object == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_ALWAYS;
        cmd_data.command = VSS_CLOSE;

        VSSI_SendCloseCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        // Nothing more needs to be done for HTTP-protocol objects.
        // Call callback without error. Clean-up the object locally.
        VSSI_FreeObject(object);
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

int VSSI_ClearConflict(VSSI_Object handle)
{
    int rv = VSSI_SUCCESS;
    VSSI_ObjectState* object = (VSSI_ObjectState*)(handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    // Clear conflict by accepting new version as current.
    object->version = object->new_version;

 fail:
    return rv;
}

void VSSI_StartSet(VSSI_Object handle,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_ObjectState* object = (VSSI_ObjectState*)(handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_START_SET;

        VSSI_SendStartSetCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Commit(VSSI_Object handle,
                 void* ctx,
                 VSSI_Callback callback)
{
    int rv = VSSI_SUCCESS;
    VSSI_ObjectState* object = (VSSI_ObjectState*)(handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_COMMIT;

        VSSI_SendCommitCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Erase(VSSI_Object handle,
                void* ctx,
                VSSI_Callback callback)
{
    int rv = VSSI_SUCCESS;
    VSSI_ObjectState* object = (VSSI_ObjectState*)(handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_ERASE;

        VSSI_SendEraseCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

static void Delete(VSSI_Session session,
                   const char* obj_xml,
                   u64 user_id,
                   u64 dataset_id,
                   const VSSI_RouteInfo* route_info,
                   bool use_xml,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_ObjectState* object = NULL;
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_UserSession* this_session = (VSSI_UserSession*)session;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                        "Building a VSSI object for session "FMTu64".",
                        this_session->handle);

    if ( use_xml ) {
        object = VSSI_BuildObject(obj_xml, this_session);
    }
    else {
        object = VSSI_BuildObject2(user_id, dataset_id, route_info,
            this_session);
    }
    if(object == NULL) {
        rv = VSSI_NOMEM;
        goto fail;
    }

    switch(object->proto) {
    case  VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_ALWAYS;
        cmd_data.command = VSS_DELETE;

        VSSI_SendDeleteCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail_dealloc;
        break;

    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unknown protocol for URL %d.",
                         object->proto);
        rv = VSSI_INVALID;
        goto fail_dealloc;
        break;
    }

    goto exit;
 fail_dealloc:
    // Destroy the failed object.
    VSSI_FreeObject(object);
 fail:
    // Call the callback.
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Delete(VSSI_Session session,
                 const char* obj_xml,
                 void* ctx,
                 VSSI_Callback callback)
{
    Delete(session, obj_xml, 0, 0, NULL, true, ctx, callback);
}

void VSSI_Delete2(VSSI_Session session,
                  u64 user_id,
                  u64 dataset_id,
                  const VSSI_RouteInfo* route_info,
                  void* ctx,
                  VSSI_Callback callback)
{
    Delete(session, NULL, user_id, dataset_id, route_info, false, ctx,
        callback);
}

void VSSI_Read(VSSI_Object handle,
               const char* name,
               u64 offset,
               u32* length,
               char* buf,
               void* ctx,
               VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    int rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_READ;
        cmd_data.length = length;
        cmd_data.buf = buf;

        VSSI_SendReadCommand(object, &cmd_data,
                             name, offset, *length);
    } break;

    case VSSI_PROTO_HTTP:
        rv = VSSI_HTTPRead(object, name,
                           offset, length, buf,
                           callback, ctx);
        if(rv != VSSI_SUCCESS) {
            goto fail;
        }
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_ReadTrash(VSSI_Object handle,
                    VSSI_TrashId id,
                    const char* name,
                    u64 offset,
                    u32* length,
                    char* buf,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    int rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_READ_TRASH;
        cmd_data.length = length;
        cmd_data.buf = buf;

        VSSI_SendReadTrashCommand(object, &cmd_data,
                                  id, name, offset, *length);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Write(VSSI_Object handle,
                const char* name,
                u64 offset,
                u32* length,
                const char* buf,
                void* ctx,
                VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.length = length;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_WRITE;

        VSSI_SendWriteCommand(object, &cmd_data,
                              name, offset, *length, buf);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_SetTimes(VSSI_Object handle,
                   const char* name,
                   VPLTime_t ctime,
                   VPLTime_t mtime,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_SET_SIZE;

        VSSI_SendSetTimesCommand(object, &cmd_data,
                                 name, ctime, mtime);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Truncate(VSSI_Object handle,
                   const char* name,
                   u64 length,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_SET_SIZE;

        VSSI_SendSetSizeCommand(object, &cmd_data,
                                name, length);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_OpenDir(VSSI_Object handle,
                  const char* name,
                  VSSI_Dir* dir,
                  void* ctx,
                  VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(dir == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_READ_DIR;
        cmd_data.object = object;
        cmd_data.directory = dir;
        *dir = NULL;

        VSSI_SendReadDirCommand(object, &cmd_data,
                                name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_OpenDir2(VSSI_Object handle,
                   const char* name,
                   VSSI_Dir2* dir,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(dir == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_READ_DIR2;
        cmd_data.object = object;
        cmd_data.directory = (VSSI_Dir *)dir;
        *dir = NULL;

        VSSI_SendReadDir2Command(object, &cmd_data, name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_OpenTrashcan(VSSI_Object handle,
                       VSSI_Trashcan* trashcan, 
                       void* ctx,
                       VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(trashcan == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_READ_TRASHCAN;
        cmd_data.object = object;
        cmd_data.trashcan = trashcan;
        *trashcan = NULL;

        VSSI_SendReadTrashcanCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_OpenTrashDir(VSSI_Object handle,
                       VSSI_TrashId id,
                       const char* name,
                       VSSI_Dir* dir,
                       void* ctx,
                       VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(dir == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_READ_TRASH_DIR;
        cmd_data.object = object;
        cmd_data.directory = dir;
        *dir = NULL;

        VSSI_SendReadTrashDirCommand(object, &cmd_data,
                                     id, name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Stat(VSSI_Object handle,
               const char* name,
               VSSI_Dirent** stats,
               void* ctx,
               VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(stats == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_STAT;
        cmd_data.object = object;
        cmd_data.stats = stats;
        *stats = NULL;

        VSSI_SendStatCommand(object, &cmd_data,
                             name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Stat2(VSSI_Object handle,
                const char* name,
                VSSI_Dirent2** stats,
                void* ctx,
                VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(stats == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_STAT2;
        cmd_data.object = object;
        cmd_data.stats2 = stats;
        *stats = NULL;

        VSSI_SendStat2Command(object, &cmd_data, name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_StatTrash(VSSI_Object handle,
                    VSSI_TrashId id,
                    const char* name,
                    VSSI_Dirent** stats,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(stats == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_STAT_TRASH;
        cmd_data.object = object;
        cmd_data.stats = stats;
        *stats = NULL;

        VSSI_SendStatTrashCommand(object, &cmd_data,
                                  id, name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

VSSI_Dirent* VSSI_ReadDir(VSSI_Dir dir)
{
    VSSI_DirectoryState* directory = (VSSI_DirectoryState*)(dir);
    VSSI_Dirent* rv = NULL;

    if(directory == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Directory is NULL. Call VSSI_OpenDir() first.");
    }
    else if(directory->version != 1) {
        // NOTE: We could actually handle this in a backward compat manner
        // if we wanted to.
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "readdir() on opendir2()");
    }
    else if(directory->offset >= directory->data_len) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Walked off end of directory.");
    }
    else {
        // Copy next entry into cur_entry and advance offset.
        char* entry = directory->raw_data + directory->offset;
        directory->cur_entry.size = vss_dirent_get_size(entry);
        directory->cur_entry.ctime = vss_dirent_get_ctime(entry);
        directory->cur_entry.mtime = vss_dirent_get_mtime(entry);
        directory->cur_entry.changeVer = vss_dirent_get_change_ver(entry);
        directory->cur_entry.isDir = vss_dirent_get_is_dir(entry);
        directory->cur_entry.signature = (const char*)vss_dirent_get_signature(entry);
        // name is NULL terminated from the server
        directory->cur_entry.name = (const char*)vss_dirent_get_name(entry);
        directory->cur_entry.metadata = (void*)&(directory->cur_metadata);
        directory->cur_metadata.dataSize = vss_dirent_get_meta_size(entry);
        directory->cur_metadata.curOffset = 0;
        directory->cur_metadata.data = vss_dirent_get_metadata(entry);
        directory->offset += (VSS_DIRENT_BASE_SIZE +
                              vss_dirent_get_name_len(entry) + 
                              vss_dirent_get_meta_size(entry));
        rv = &(directory->cur_entry);
    }

    return rv;
}

VSSI_Dirent2* VSSI_ReadDir2(VSSI_Dir2 dir)
{
    VSSI_DirectoryState* directory = (VSSI_DirectoryState*)(dir);
    VSSI_Dirent2* rv = NULL;

    if(directory == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Directory is NULL. Call VSSI_OpenDir() first.");
    }
    else if(directory->version != 2) {
        // NOTE: We could actually handle this in a forward compat manner
        // if we wanted to.
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "readdir2() on opendir()");
    }
    else if(directory->offset >= directory->data_len) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Walked off end of directory.");
    }
    else {
        // Copy next entry into cur_entry and advance offset.
        char* entry = directory->raw_data + directory->offset;
        directory->cur_entry2.size = vss_dirent2_get_size(entry);
        directory->cur_entry2.ctime = vss_dirent2_get_ctime(entry);
        directory->cur_entry2.mtime = vss_dirent2_get_mtime(entry);
        directory->cur_entry2.changeVer = vss_dirent2_get_change_ver(entry);
        directory->cur_entry2.isDir = vss_dirent2_get_is_dir(entry);
        directory->cur_entry2.attrs = vss_dirent2_get_attrs(entry);
        directory->cur_entry2.signature = (const char*)vss_dirent2_get_signature(entry);
        // name is NULL terminated from the server
        directory->cur_entry2.name = (const char*)vss_dirent2_get_name(entry);
        directory->cur_entry2.metadata = (void*)&(directory->cur_metadata);
        directory->cur_metadata.dataSize = vss_dirent2_get_meta_size(entry);
        directory->cur_metadata.curOffset = 0;
        directory->cur_metadata.data = vss_dirent2_get_metadata(entry);
        directory->offset += (VSS_DIRENT2_BASE_SIZE +
                              vss_dirent2_get_name_len(entry) + 
                              vss_dirent2_get_meta_size(entry));
        rv = &(directory->cur_entry2);
    }

    return rv;
}

void VSSI_RewindDir(VSSI_Dir dir)
{
    VSSI_DirectoryState* directory = (VSSI_DirectoryState*)(dir);

    if(directory == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Directory is NULL. Call VSSI_OpenDir() first.");
    }
    else {
        directory->offset = 0;
    }
}

void VSSI_RewindDir2(VSSI_Dir2 dir)
{
    VSSI_RewindDir((VSSI_Dir)dir);
}

void VSSI_CloseDir(VSSI_Dir dir)
{
    VSSI_DirectoryState* directory = (VSSI_DirectoryState*)(dir);

    if(directory == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Directory is NULL. Call VSSI_OpenDir() first.");
    }
    else {
        free(directory->raw_data);
        free(directory);
    }
}

void VSSI_CloseDir2(VSSI_Dir2 dir)
{
    VSSI_CloseDir((VSSI_Dir)dir);
}

VSSI_Metadata* VSSI_GetMetadataByType(VSSI_Dirent* dirent, u8 type)
{
    VSSI_Metadata* rv = NULL;

    if(dirent != NULL && dirent->metadata != NULL) {
        VSSI_DirentMetadataState* metaState = 
            (VSSI_DirentMetadataState*)(dirent->metadata);

        // Attempt to find the desired metadata type.
        size_t offset = metaState->curOffset;
        while((rv = VSSI_ReadMetadata(dirent)) != NULL) {
            if(rv->type == type) {
                break;
            }
        }
        // Restore metadata read offset.
        metaState->curOffset = offset;

        if(rv && rv->type != type) {
            rv = NULL;
        }
    }   

    return rv;
}

VSSI_Metadata* VSSI_ReadMetadata(VSSI_Dirent* dirent)
{
    VSSI_Metadata* rv = NULL;
    if(dirent != NULL && dirent->metadata != NULL) {
        VSSI_DirentMetadataState* metaState = 
            (VSSI_DirentMetadataState*)(dirent->metadata);

        // Return current entry and advance offset.
        if(metaState->curOffset < metaState->dataSize) {
            const char* entry = metaState->data + metaState->curOffset;
            rv = &(metaState->curEntry);
            rv->type = vss_dirent_metadata_get_type(entry);
            rv->length = vss_dirent_metadata_get_length(entry);
            rv->data = vss_dirent_metadata_get_value(entry);

            metaState->curOffset += VSS_DIRENT_META_BASE_SIZE + rv->length;
        }
    }

    return rv;
}

void VSSI_RewindMetadata(VSSI_Dirent* dirent)
{
    if(dirent != NULL && dirent->metadata != NULL) {
        VSSI_DirentMetadataState* metaState = 
            (VSSI_DirentMetadataState*)(dirent->metadata);
        
        metaState->curOffset = 0;
    }
}

VSSI_TrashRecord* VSSI_ReadTrashcan(VSSI_Trashcan trashcan)
{
    VSSI_TrashcanState* trash = (VSSI_TrashcanState*)(trashcan);
    VSSI_TrashRecord* rv = NULL;

    if(trashcan == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Trashcan is NULL. Call VSSI_OpenTrashcan() first.");
    }
    else if(trash->offset >= trash->data_len) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Walked off end of trashcan.");
    }
    else {
        // Copy next entry into cur_entry and advance offset.
        char* record = trash->raw_data + trash->offset;
        trash->cur_record.id.version = vss_trashrec_get_recver(record);
        trash->cur_record.id.index = vss_trashrec_get_recidx(record);
        trash->cur_record.ctime = vss_trashrec_get_ctime(record);
        trash->cur_record.mtime = vss_trashrec_get_mtime(record);
        trash->cur_record.dtime = vss_trashrec_get_dtime(record);
        trash->cur_record.isDir = vss_trashrec_get_is_dir(record);
        trash->cur_record.size = vss_trashrec_get_size(record);
        // name is NULL terminated from the server
        trash->cur_record.name = (const char*)vss_trashrec_get_name(record);
        trash->offset += (VSS_TRASHREC_BASE_SIZE +
                          vss_trashrec_get_name_len(record));
        rv = &(trash->cur_record);
    }

    return rv;
}

void VSSI_RewindTrashcan(VSSI_Trashcan trashcan)
{
    VSSI_TrashcanState* trash = (VSSI_TrashcanState*)(trashcan);

    if(trashcan == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Trashcan is NULL. Call VSSI_OpenDir() first.");
    }
    else {
        trash->offset = 0;
    }
}

void VSSI_CloseTrashcan(VSSI_Trashcan trashcan)
{
    VSSI_TrashcanState* trash = (VSSI_TrashcanState*)(trashcan);

    if(trashcan == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Trashcan is NULL. Call VSSI_OpenDir() first.");
    }
    else {
        free(trash->raw_data);
        free(trash);
    }
}

void VSSI_MkDir(VSSI_Object handle,
                const char* name,
                void* ctx,
                VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_MAKE_DIR;
        cmd_data.object = object;

        VSSI_SendMakeDirCommand(object, &cmd_data,
                                name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_MkDir2(VSSI_Object handle,
                 const char* name,
                 u32 attrs,
                 void* ctx,
                 VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_MAKE_DIR2;
        cmd_data.object = object;

        VSSI_SendMakeDir2Command(object, &cmd_data, name, attrs);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Chmod(VSSI_Object handle,
                const char* name,
                u32 attrs,
                u32 attrs_mask,
                void* ctx,
                VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_CHMOD;
        cmd_data.object = object;

        VSSI_SendChmodCommand(object, &cmd_data, name, attrs, attrs_mask);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}


void VSSI_Remove(VSSI_Object handle,
                 const char* name,
                 void* ctx,
                 VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_REMOVE;
        cmd_data.object = object;

        VSSI_SendRemoveCommand(object, &cmd_data,
                               name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_DeleteTrash(VSSI_Object handle,
                      VSSI_TrashId id,
                      void* ctx,
                      VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_DELETE_TRASH;
        cmd_data.object = object;

        VSSI_SendDeleteTrashCommand(object, &cmd_data,
                                    id);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_EmptyTrash(VSSI_Object handle,
                     void* ctx,
                     VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_EMPTY_TRASH;
        cmd_data.object = object;

        VSSI_SendEmptyTrashCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Rename(VSSI_Object handle,
                  const char* name,
                  const char* new_name,
                  void* ctx,
                  VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    // Make sure neither name nor new_name is a subpath of the other,
    // but allow "rename self".
    if((strncmp(name, new_name, strlen(new_name)) == 0 &&
        name[strlen(new_name)] == '/') ||
       (strncmp(new_name, name, strlen(name)) == 0 &&
        new_name[strlen(name)] == '/')) {
        rv = VSSI_INVALID;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_RENAME;
        cmd_data.object = object;

        VSSI_SendRenameCommand(object, &cmd_data, 
                               name, new_name);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Rename2(VSSI_Object handle,
                  const char* name,
                  const char* new_name,
                  u32 flags,
                  void* ctx,
                  VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    // Make sure neither name nor new_name is a subpath of the other,
    // but allow "rename self".
    if((strncmp(name, new_name, strlen(new_name)) == 0 &&
        name[strlen(new_name)] == '/') ||
       (strncmp(new_name, name, strlen(name)) == 0 &&
        new_name[strlen(name)] == '/')) {
        rv = VSSI_INVALID;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_RENAME2;
        cmd_data.object = object;

        VSSI_SendRename2Command(object, &cmd_data, 
                               name, new_name, flags);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_Copy(VSSI_Object handle,
               const char* source,
               const char* destination,
               void* ctx,
               VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    // Make sure neither source nor destination is a subpath of the other.
    if((strncmp(source, destination, strlen(destination)) == 0 &&
        (strlen(source) == strlen(destination) || 
         source[strlen(destination)] == '/')) ||
       (strncmp(destination, source, strlen(source)) == 0 &&
        (strlen(source) == strlen(destination) || 
         destination[strlen(source)] == '/'))) {
        rv = VSSI_INVALID;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_COPY;
        cmd_data.object = object;

        VSSI_SendCopyCommand(object, &cmd_data,
                             source, destination);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_CopyMatch(VSSI_Object handle,
                    u64 size,
                    const char* signature,
                    const char* destination,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_COPY_MATCH;
        cmd_data.object = object;

        VSSI_SendCopyMatchCommand(object, &cmd_data, 
                                  size, signature, destination);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_RestoreTrash(VSSI_Object handle,
                       VSSI_TrashId id,
                       const char* destination,
                       void* ctx,
                       VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_RESTORE_TRASH;
        cmd_data.object = object;

        VSSI_SendRestoreTrashCommand(object,&cmd_data,
                                     id, destination);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_SetMetadata(VSSI_Object handle,
                      const char* name,
                      u8 type,
                      u8 length,
                      const char* data,
                      void* ctx,
                      VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    rv = VSSI_CheckObjectConflict(object);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_SET_METADATA;

        VSSI_SendSetMetadataCommand(object, &cmd_data,
                                    name, type, length, data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_GetSpace(VSSI_Object handle,
                   u64* disk_size,
                   u64* dataset_size,
                   u64* avail_size,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(disk_size == NULL || dataset_size == NULL || avail_size == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_GET_SPACE;
        cmd_data.object = object;
        cmd_data.disk_size = disk_size;
        cmd_data.dataset_size = dataset_size;
        cmd_data.avail_size = avail_size;
        *disk_size = 0;
        *dataset_size = 0;
        *avail_size = 0;

        VSSI_SendGetSpaceCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_GetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_out,
                          void* ctx,
                          VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.notify_mask = mask_out;
        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_GET_NOTIFY;

        VSSI_SendGetNotifyCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_SetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_in_out,
                          void* notify_ctx,
                          VSSI_NotifyCallback notify_callback,
                          void* ctx,
                          VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_Result rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.notify_callback = notify_callback;
        cmd_data.notify_ctx = notify_ctx;
        cmd_data.notify_mask = mask_in_out;
        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.command = VSS_SET_NOTIFY;

        VSSI_SendSetNotifyCommand(object, &cmd_data, *mask_in_out);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "HTTP protocol not supported.");
        rv = VSSI_INVALID;
        goto fail;
        break;

    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_OpenFile(VSSI_Object handle,
                   const char* name,
                   u32 flags,
                   u32 attrs,
                   VSSI_File* file,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_ObjectState* object = (VSSI_ObjectState*)handle;
    VSSI_FileState* file_state = NULL;
    int rv = VSSI_SUCCESS;
    u32 temp;
        
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        file_state = (VSSI_FileState *)calloc(sizeof(VSSI_FileState), 1);
        if(file_state == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to allocate memory for new object.");
            rv = VSSI_NOSPACE;
            goto fail;
        }

        file_state->flags = flags;
        file_state->object = object;
        //
        // Construct a unique ID for this client handle (unique across all potential clients)
        // by using the vss_object handle returned by the server, which is at least unique
        // on the server for the particular dataset object, mixed together with the virtual
        // address of the file state object.  The "origin" is passed on all lock and file
        // IO requests to tie the requestor back to the originating Windows file handle.
        //
        file_state->origin = (u64) object->access.vs.proto_handle;
        temp = get_unique_value((u32) file_state);
        file_state->origin |= ((u64)temp) << 32;

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = file_state;
        cmd_data.delete_filestate_when = DELETE_ON_FAIL;
        cmd_data.file = file;
        cmd_data.command = VSS_OPEN_FILE;

        // Add file state to object (assume success for now)
        VPLMutex_Lock(&vssi_mutex);
        file_state->next = object->files;
        object->files = file_state;
        object->num_files++;
        VPLMutex_Unlock(&vssi_mutex);

        VSSI_SendOpenFileCommand(object, &cmd_data, name, flags, attrs);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_ReadFile(VSSI_File file,
                   u64 offset,
                   u32* length,
                   char* buf,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState *)file;
    VSSI_ObjectState* object = filestate->object;
    int rv = VSSI_SUCCESS;
        
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    // Check that file handle is opened for read
    if ((filestate->flags & VSSI_FILE_OPEN_READ) == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI_ReadFile not permitted to handle (flags %x)", filestate->flags);
        rv = VSSI_PERM;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.buf = buf;
        cmd_data.length = length;
        cmd_data.command = VSS_READ_FILE;

        VSSI_SendReadFileCommand(object, &cmd_data, offset, *length);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_WriteFile(VSSI_File file,
                    u64 offset,
                    u32* length,
                    const char* buf,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState *)file;
    VSSI_ObjectState* object = filestate->object;
    int rv = VSSI_SUCCESS;
        
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    // Check that file handle is opened for write
    if ((filestate->flags & VSSI_FILE_OPEN_WRITE) == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI_WriteFile to readOnly handle (flags %x)", filestate->flags);
        rv = VSSI_PERM;
        goto fail;
    }

    //
    // Limit write length to a reasonable maximum of 1MB.  Note that this
    // does not fail the write, but will return a count less than the
    // caller requested.  The caller must then write the additional data
    // in a separate call.
    //
    // Note that this limitation has to happen here, since it's too late
    // by the time it gets to the server side.
    //
    if(*length > (1<<20)) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "write file %p limited from %u to %u bytes.",
                          filestate, *length, (1<<20));
        *length = (1<<20);
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.length = length;
        cmd_data.command = VSS_WRITE_FILE;

        VSSI_SendWriteFileCommand(object, &cmd_data, offset, *length, buf);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_TruncateFile(VSSI_File file,
                       u64 offset,
                       void* ctx,
                       VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState *)file;
    VSSI_ObjectState* object = filestate->object;
    int rv = VSSI_SUCCESS;
        
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    // Check that file handle is opened for write
    if ((filestate->flags & VSSI_FILE_OPEN_WRITE) == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI_TruncateFile to readOnly handle (flags %x)", filestate->flags);
        rv = VSSI_PERM;
        goto fail;
    }


    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.command = VSS_TRUNCATE_FILE;

        VSSI_SendTruncateFileCommand(object, &cmd_data, offset);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_ChmodFile(VSSI_File file,
                    u32 attrs,
                    u32 attrs_mask,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState *)file;
    VSSI_ObjectState* object = filestate->object;
    int rv = VSSI_SUCCESS;
        
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.command = VSS_CHMOD_FILE;

        VSSI_SendChmodFileCommand(object, &cmd_data, attrs, attrs_mask);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_SetFileLockState(VSSI_File file,
                           VSSI_FileLockState lock_state,
                           void* ctx,
                           VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState*)file;
    VSSI_ObjectState* object = NULL;
    int rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if (filestate == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }
    object = filestate->object;
    if (object == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.command = VSS_SET_LOCK;

        VSSI_SendSetLockCommand(object, &cmd_data, lock_state);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_GetFileLockState(VSSI_File file,
                           VSSI_FileLockState* lock_state,
                           void* ctx,
                           VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState*)file;
    VSSI_ObjectState* object = NULL;
    int rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if (filestate == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }
    object = filestate->object;
    if (object == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.lock_state = lock_state;
        cmd_data.command = VSS_GET_LOCK;

        VSSI_SendGetLockCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_SetByteRangeLock(VSSI_File file,
                           VSSI_ByteRangeLock* br_lock,
                           u32 flags,
                           void* ctx,
                           VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState*)file;
    VSSI_ObjectState* object = NULL;
    int rv = VSSI_SUCCESS;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if (filestate == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }
    object = filestate->object;
    if (object == NULL) {
        rv = VSSI_INVALID;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.command = VSS_SET_LOCK_RANGE;

        // TODO: convert to pass file handle

        VSSI_SendSetLockRangeCommand(object, &cmd_data, br_lock, flags);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_ReleaseFile(VSSI_File file,
                      void* ctx,
                      VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState *)file;
    VSSI_ObjectState* object;
    int rv = VSSI_SUCCESS;
        
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(filestate == NULL) {
        rv = VSSI_BADOBJ;
        goto fail;
    }

    object = filestate->object;
    if(object == NULL) {
        rv = VSSI_BADOBJ;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.command = VSS_RELEASE_FILE;

        VSSI_SendReleaseFileCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

void VSSI_CloseFile(VSSI_File file,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_FileState* filestate = (VSSI_FileState *)file;
    VSSI_ObjectState* object;
    int rv = VSSI_SUCCESS;
        
    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv = VSSI_INIT;
        goto fail;
    }

    if(filestate == NULL) {
        rv = VSSI_BADOBJ;
        goto fail;
    }

    object = filestate->object;
    if(object == NULL) {
        rv = VSSI_BADOBJ;
        goto fail;
    }

    switch(object->proto) {
    case VSSI_PROTO_VS: {
        VSSI_PendInfo cmd_data;
        memset(&cmd_data, 0, sizeof(VSSI_PendInfo));

        cmd_data.callback = callback;
        cmd_data.ctx = ctx;
        cmd_data.object = object;
        cmd_data.delete_obj_when = DELETE_NEVER;
        cmd_data.file_state = filestate;
        cmd_data.delete_filestate_when = DELETE_NEVER;
        cmd_data.command = VSS_CLOSE_FILE;

        VSSI_SendCloseFileCommand(object, &cmd_data);
    } break;

    case VSSI_PROTO_HTTP:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "HTTP protocol not supported.");
    default:
        // unsupported.
        rv = VSSI_INVALID;
        goto fail;
        break;
    }

    goto exit;
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
 exit:
    return;
}

VSSI_ServerFileId VSSI_GetServerFileId(VSSI_File file)
{
    VSSI_FileState* filestate = (VSSI_FileState *)file;

    return filestate->server_handle;
}

void VSSI_SecureTunnelConnect(VSSI_Session session,
                              const char* server_name,
                              u16 server_port,
                              VSSI_SecureTunnelConnectType connection_type,
                              u64 destination_device,
                              VSSI_SecureTunnel* tunnel_handle,
                              void* ctx,
                              VSSI_Callback callback)
{
    int rv = VSSI_SUCCESS;
    int rc;
    VSSI_SecureTunnelState* tunnel;

    *tunnel_handle = NULL;

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        rv =  VSSI_INIT;
        goto fail;
    }

    // Create and initialize secure tunnel state.
    tunnel = (VSSI_SecureTunnelState*)calloc(sizeof(VSSI_SecureTunnelState), 1);
    if(tunnel == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed memory allocation on creating secure tunnel.");
        rv = VSSI_NOMEM;
        goto fail;
    }
    rc = VPLMutex_Init(&(tunnel->mutex));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed mutex initialization creating secure tunnel.");
        rv = rc;
        goto fail_dealloc;
    }
    tunnel->session = session;
    tunnel->destination_device = destination_device;
    tunnel->inactive_timeout = VSSI_TUNNEL_CONNECTION_TIMEOUT;
    tunnel->last_active = VPLTime_GetTimeStamp();
    tunnel->client_handle = tunnel_handle;
    tunnel->client_ctx = ctx;
    tunnel->client_callback = callback;
    tunnel->connection = VPLSOCKET_INVALID;
    tunnel->inaddr.addr = VPLNet_GetAddr(server_name);
    tunnel->inaddr.port = VPLNet_port_hton(server_port);
    tunnel->connect_type = connection_type;
    tunnel->sign_mode = VSS_NEGOTIATE_SIGNING_MODE_FULL;
    tunnel->sign_type = VSS_NEGOTIATE_SIGN_TYPE_SHA1;
    tunnel->enc_type = VSS_NEGOTIATE_ENCRYPT_TYPE_AES128;

    // Make appropriate type of tunnel connection.
    switch(connection_type) {
    case VSSI_SECURE_TUNNEL_DIRECT:
    case VSSI_SECURE_TUNNEL_DIRECT_INTERNAL:
        // Directly connect. 
        tunnel->connect_type = VSSI_SECURE_TUNNEL_DIRECT;
        rc = reconnect_server(&(tunnel->connection), &(tunnel->inaddr), 0, NULL,
                              4, 5);  // use these timeouts in VPLSocket_Connect*()
        if(rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to open direct connection to device "FMTu64" to address %s:%u",
                             destination_device,
                             server_name, server_port);
            rv = VSSI_COMM;
            goto fail_destroy;
        }
        else {
            // Send authentication message (put on send queue).            
            VSSI_MsgBuffer* msg = NULL;
            char header[VSS_HEADER_SIZE] = {0};
            char body[VSS_AUTHENTICATE_SIZE_EXT_VER_1] = {0};
            size_t body_size = VSS_AUTHENTICATE_SIZE_EXT_VER_1;

            vss_set_command(header, VSS_AUTHENTICATE);
            vss_set_xid(header, 0);
            vss_authenticate_set_cluster_id(body, tunnel->destination_device);
            if(connection_type == VSSI_SECURE_TUNNEL_DIRECT_INTERNAL &&
               !VPLNet_IsRoutableAddress(tunnel->inaddr.addr)) {
                vss_authenticate_set_ext_version(body, 1);
                vss_authenticate_set_signing_mode(body, VSS_NEGOTIATE_SIGNING_MODE_NONE);
                vss_authenticate_set_sign_type(body, VSS_NEGOTIATE_SIGN_TYPE_NONE);
                vss_authenticate_set_encrypt_type(body, VSS_NEGOTIATE_ENCRYPT_TYPE_NONE);
            }
            else {
                body_size = VSS_AUTHENTICATE_SIZE_EXT_VER_0;
            }
            msg = VSSI_PrepareMessage(header,
                                      body, body_size, 
                                      NULL, 0, NULL, 0);
            if(msg == NULL) {
                rv = VSSI_NOMEM;
                goto fail_destroy;
            }
            msg = VSSI_ComposeMessage(tunnel->session, 
                                      VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                      VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                                      VSS_NEGOTIATE_ENCRYPT_TYPE_AES128,
                                      msg);
            if(msg == NULL) {
                rv = VSSI_NOMEM;
                goto fail_destroy;
            }
            
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Sending AUTHENTICATE (%p size "FMTu_size_t") on secure tunnel "FMT_VPLSocket_t".",
                                msg, msg->length,
                                VAL_VPLSocket_t(tunnel->connection));

            VPLMutex_Lock(&(tunnel->mutex));
            tunnel->send_head = msg;
            tunnel->send_tail = msg;
            tunnel->send_q_depth += msg->length;
            tunnel->send_enable = true;
            VPLMutex_Unlock(&(tunnel->mutex));
            
            tunnel->last_active = VPLTime_GetTimeStamp();
            tunnel->recv_enable = true;

            tunnel->is_direct = true;

            // Receiving a response authenticates or fails connection,
            // calls callback.
            // Add to list of secure tunnels to await reply.
            VPLMutex_Lock(&vssi_mutex);
            tunnel->next = secure_tunnel_list_head;
            secure_tunnel_list_head = tunnel;
            VPLMutex_Unlock(&vssi_mutex);
            VSSI_NotifySocketChangeFn();
        }
        
        break;
    case VSSI_SECURE_TUNNEL_PROXY:
    case VSSI_SECURE_TUNNEL_PROXY_P2P:
        // Make a proxy connection. Provide callback to handle result.
        VSSI_OpenProxiedConnection(session, server_name, server_port,
                                   destination_device,
                                   VSS_PROXY_CONNECT_TYPE_SSTREAM,
                                   (connection_type == VSSI_SECURE_TUNNEL_PROXY_P2P) ? true : false,
                                   &(tunnel->connection), &(tunnel->is_direct),
                                   tunnel, VSSI_InternalCompleteProxyTunnel);
        break;

    case VSSI_SECURE_TUNNEL_INVALID:
    default:
        // Not a valid connection type. Call callback with error.
        rv = VSSI_INVALID;
        goto fail_destroy;
    }

    return;
 fail_destroy:
    VPLMutex_Destroy(&(tunnel->mutex));
 fail_dealloc:
    free(tunnel);
 fail:
    if(callback) {
        (callback)(ctx, rv);
    }
}

void VSSI_SecureTunnelDisconnect(VSSI_SecureTunnel tunnel_handle)
{
    VSSI_SecureTunnelState* tunnel = 
        (VSSI_SecureTunnelState*)(tunnel_handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        return;
    }

    VPLMutex_Lock(&vssi_mutex);
    // Destroy tunnel later.
    tunnel->disconnected = true;
    VSSI_InternalClearTunnel(tunnel);
    // Force update of active sockets.
    VSSI_NotifySocketChangeFn();
    VPLMutex_Unlock(&vssi_mutex);
}

int VSSI_SecureTunnelIsDirect(VSSI_SecureTunnel tunnel_handle)
{
    int rv = false;
    VSSI_SecureTunnelState* state = 
        (VSSI_SecureTunnelState*)(tunnel_handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        return VSSI_INIT;
    }
    
    if(!VPLSocket_Equal(state->connection, VPLSOCKET_INVALID)) {
        rv = state->is_direct;
    }

    return rv;
}

VSSI_Result VSSI_SecureTunnelSend(VSSI_SecureTunnel tunnel_handle,
                                  const char* data,
                                  size_t len)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_SecureTunnelState* state = 
        (VSSI_SecureTunnelState*)(tunnel_handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        return VSSI_INIT;
    }

    // Put data in send queue.
    rv = VSSI_InternalTunnelSend(state, data, len);

    return rv;
}

void VSSI_SecureTunnelWaitToSend(VSSI_SecureTunnel tunnel_handle,
                                 void* ctx,
                                 VSSI_Callback callback)
{
    VSSI_SecureTunnelState* state = 
        (VSSI_SecureTunnelState*)(tunnel_handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        if(callback) {
            (callback)(ctx, VSSI_INIT);
        }
        return;
    }

    // Set callback to be called when ready to send
    VSSI_InternalTunnelWaitForSend(state, ctx, callback);
}

VSSI_Result VSSI_SecureTunnelReceive(VSSI_SecureTunnel tunnel_handle,
                                     char* data,
                                     size_t len)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_SecureTunnelState* state = 
        (VSSI_SecureTunnelState*)(tunnel_handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        return VSSI_INIT;
    }

    // Pull data from receive queue until queue empty or len reached.
    rv = VSSI_InternalTunnelRecv(state, data, len);

    return rv;
}

void VSSI_SecureTunnelWaitToReceive(VSSI_SecureTunnel tunnel_handle,
                                    void* ctx,
                                    VSSI_Callback callback)
{
    VSSI_SecureTunnelState* state = 
        (VSSI_SecureTunnelState*)(tunnel_handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        if(callback) {
            (callback)(ctx, VSSI_INIT);
        }
        return;
    }

    // Set callback to be called when ready to receive
    VSSI_InternalTunnelWaitForRecv(state, ctx, callback);
}

void VSSI_SecureTunnelReset(VSSI_SecureTunnel tunnel_handle,
                                    void* ctx,
                                    VSSI_Callback callback)
{
    VSSI_SecureTunnelState* state = 
        (VSSI_SecureTunnelState*)(tunnel_handle);

    if(!vssi_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI is not initialized. Call VSSI_Init() first.");
        if(callback) {
            (callback)(ctx, VSSI_INIT);
        }
        return;
    }

    // Reset tunnel and set callback.
    VSSI_InternalTunnelReset(state, ctx, callback);
}

// Calculate value to ensure that origin is unique.
static u32 get_unique_value(u32 seed)
{
    CSL_ShaContext hashCtx;
    u32 hashVal[CSL_SHA1_DIGESTSIZE/sizeof(u32)];
    static u32 local_counter = 0;
    struct {
        u32 seed;
        u32 counter;
        VPLTime_t currTime;
    } hashInput;

    hashInput.seed = seed;
    hashInput.counter = local_counter++;
    hashInput.currTime = VPLTime_GetTimeStamp();

    CSL_ResetSha(&hashCtx);
    CSL_InputSha(&hashCtx, (u8 *)&hashInput, sizeof(hashInput));
    CSL_ResultSha(&hashCtx, (u8 *)&hashVal[0]);
    
    return hashVal[0];
}
