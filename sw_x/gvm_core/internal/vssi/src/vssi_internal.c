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

/// vssi_internal.c
///
/// Virtual Storage internal APIs implementations

#include "vpl_th.h"
#include "vplex_assert.h"
#include "vpl_socket.h"
#include "vplex_socket.h"
#include "vplex_trace.h"

#include <stdlib.h>

#include "vssi.h"
#include "vssi_internal.h"
#include "vssi_vss.h"
#include "vssi_http.h"
#include "vssi_error.h"

extern VPLMutex_t vssi_mutex;

extern VSSI_SessionNode* user_session_list_head;
extern VSSI_VSConnectionNode* vs_connection_list_head;
extern VSSI_HTTPConnectionNode* http_connection_list_head;
extern VSSI_ConnectProxyState* proxy_connection_list_head;
extern VSSI_SecureTunnelState* secure_tunnel_list_head;

static int vssi_reuse_connections = 1; // reuse by default
int vssi_max_server_connections = 1;

extern void (*VSSI_NotifySocketChangeFn)(void);

int VSSI_InternalSetParameter(VSSI_Param id, int value)
{
    int rv = VSSI_SUCCESS;

    switch(id) {
    case VSSI_PARAM_INVALID:
    default:
        rv = VSSI_INVALID;
        break;
    case VSSI_PARAM_REUSE_CONNECTIONS:
        vssi_reuse_connections = value;
        break;
    case VSSI_PARAM_MAX_SERVER_CONNECTIONS:
        if(value <= 16 && value >= 1) {
            vssi_max_server_connections = value;
        }
        else {
            rv = VSSI_INVALID;
        }
    }

    return rv;
}

int VSSI_InternalGetParameter(VSSI_Param id, int* value_out)
{
    int rv = VSSI_SUCCESS;

    switch(id) {
    case VSSI_PARAM_INVALID:
    default:
        rv = VSSI_INVALID;
        break;
    case VSSI_PARAM_REUSE_CONNECTIONS:
        *value_out = vssi_reuse_connections;
        break;
    case VSSI_PARAM_MAX_SERVER_CONNECTIONS:
        *value_out = vssi_max_server_connections;
        break;
    }

    return rv;
}

void VSSI_FreeFileState(VSSI_FileState* filestate)
{
    // Remove from the containing object's file list
    VSSI_ObjectState* object = filestate->object;
    bool found = false;

    if(object == NULL) {
        // TODO: can this really happen?
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Filestate %p destroyed, but object backpointer is null",
                         filestate);
        goto free_exit;
    }

    // Remove from singly linked list
    VPLMutex_Lock(&vssi_mutex);
    if(object->files == filestate) {
        object->files = filestate->next;
        object->num_files--;
        found = true;
    }
    else {
        VSSI_FileState* prev_file = object->files;
        while(prev_file != NULL && prev_file->next != filestate) {
            prev_file = prev_file->next;
        }
        if(prev_file != NULL) {
            prev_file->next = filestate->next;
            object->num_files--;
            found = true;
        }
    }
    VPLMutex_Unlock(&vssi_mutex);

    if (!found) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Filestate %p not linked to object %p. Object has %d files remaining.",
                         filestate, object, object->num_files);
        goto free_exit;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Filestate %p for object %p destroyed. Object has %d files remaining.",
                      filestate, object, object->num_files);
 free_exit: 
    free(filestate);
}

void VSSI_FreeObject(VSSI_ObjectState* object)
{
    // Remove object from its session's list of objects.
    VSSI_UserSession* session = object->user_session;

    VPLMutex_Lock(&vssi_mutex);
    if(session->objects == object) {
        session->objects = object->next;
    }
    else {
        VSSI_ObjectState* prev_object = session->objects;
        while(prev_object != NULL && prev_object->next != object) {
            prev_object = prev_object->next;
        }
        if(prev_object != NULL) {
            prev_object->next = object->next;
        }
    }

    // Close and free all file handles held by object    
    while(object->files != NULL) {
        VSSI_FreeFileState(object->files);
    }

    VPLMutex_Unlock(&vssi_mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Object %p for session "FMTu64" destroyed. Session has%s objects.",
                      object, session->handle, session->objects ? "" : " no remaining");
    free(object);
}

/// Build up an object structure from the description-XML.
VSSI_ObjectState* VSSI_BuildObject(const char* obj_xml,
                                   VSSI_UserSession* session)
{
    VSSI_ObjectState* object = NULL;
    char* objtag_start = NULL;
    char* objtag_end = NULL;
    char* endobjtag = NULL;
    char* format_property = NULL;
    char* request_size_property = NULL;
    size_t request_size = 0;
    int format;
    int is_atomic = 0; // backwards compatability
    size_t object_body_len;
    char* object_body = NULL;
    int rc;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Building object from XML {%s}.", obj_xml);

    // Find object tags.
    objtag_start = strstr(obj_xml, "<object");
    objtag_end = strchr(objtag_start, '>');
    endobjtag = strstr(obj_xml, "</object>");

    if(objtag_start == NULL ||
       objtag_end == NULL ||
       endobjtag == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Object XML {%s} has no object tags.",
                         obj_xml);
        goto exit;
    }

    // Find object format in the objtag
    format_property = strstr(objtag_start, "format=\"");
    if(format_property == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Object XML {%s} has no format.",
                         obj_xml);
        goto exit;
    }
    format_property = strchr(format_property, '=');
    if(format_property == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Object XML {%s} has bad format property.",
                         obj_xml);
        goto exit;
    }
    format_property = strchr(format_property, '\"');
    if(format_property == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Object XML {%s} has bad format property.",
                         obj_xml);
        goto exit;
    }
    format_property++; // pass the quote.

    // Determine format.
    if(strncmp(format_property, "list", strlen("list")) == 0 ||
       strncmp(format_property, "content", strlen("content")) == 0) {
        // Content object. Must be HTTP access.
        format = VSSI_PROTO_HTTP;
    }
    else if(strncmp(format_property, "dataset", strlen("dataset")) == 0) {
        // Dataset object. Must be VSSP access.
        format = VSSI_PROTO_VS;
    }
    else if(strncmp(format_property, "atomic", strlen("atomic")) == 0) {
        // Legacy format for dataset. Needs legacy handling.
        format = VSSI_PROTO_VS;
        is_atomic = 1;
    }
    else {
        // Not a known format. Eject!
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Object XML {%s} has no valid format.",
                         obj_xml);
        goto exit;
    }

    // Find optional request size parameter.
    request_size_property = strstr(objtag_start, "requestSize");
    if(request_size_property != NULL) {
        // Get the requestSize value for the object.
        request_size_property = strchr(request_size_property, '=');
        if(request_size_property != NULL) {
            request_size_property = strchr(request_size_property, '\"');
            if(request_size_property != NULL) {
                request_size_property++;
                request_size = strtoul(request_size_property, NULL, 10);
            }
        }
    }

    object_body_len = endobjtag - objtag_end - 1;
    object_body = (char*)malloc(object_body_len + 1);
    if(object_body == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to allocate memory for new object.");
        goto exit;
    }
    strncpy(object_body, objtag_end + 1, object_body_len);
    object_body[object_body_len] = '\0';

    object = (VSSI_ObjectState*)calloc(sizeof(VSSI_ObjectState), 1);
    if(object == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to allocate memory for new object.");
        goto exit;
    }
    object->user_session = session;
    object->proto = format; // They're the same thing for now.
    object->request_size = request_size;

    switch(format) {
    case VSSI_PROTO_HTTP:
        rc = VSSI_BuildContentObject(object_body, &object);
        break;
    case VSSI_PROTO_VS:
        if(is_atomic) {
            // Backwards compatibility.
            rc = VSSI_BuildAtomicObject(object_body, &object);
        }
        else {
            rc = VSSI_BuildDatasetObject(object_body, &object);
        }
        break;
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Bad format: %d", format);
        rc = -1;
    }

    if(rc != 0) {
        free(object);
        object = NULL;
        goto exit;
    }

    VPLMutex_Lock(&vssi_mutex);
    object->next = session->objects;
    session->objects = object;
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Object %p for session "FMTu64" being added.",
                      object, session->handle);
    VPLMutex_Unlock(&vssi_mutex);

 exit:
    if(object_body) {
        free(object_body);
    }

    return object;
}

static int route_pref[] = {
    VSSI_ROUTE_DIRECT_INTERNAL,
    VSSI_ROUTE_DIRECT_EXTERNAL,
    VSSI_ROUTE_PROXY
};
#define MAX_ROUTE_PREF      (sizeof(route_pref)/sizeof(route_pref[0]))

VSSI_ObjectState* VSSI_BuildObject2(u64 user_id,
                                    u64 dataset_id,
                                    const VSSI_RouteInfo* route_info,
                                    VSSI_UserSession* session)
{
    VSSI_ObjectState* object = NULL;
    size_t object_size;
    int i, j, k;
    char* tmp;

    object_size = sizeof(VSSI_ObjectState) + 
        sizeof(VSSI_Route) * route_info->num_routes;
    for(i = 0; i < route_info->num_routes; i++) {
        object_size += strlen(route_info->routes[i].server) + 1;
    }

    object = (VSSI_ObjectState*)calloc(object_size, 1);
    if(object == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to allocate memory for new object.");
        goto exit;
    }

    object->proto = VSSI_PROTO_VS;
    object->user_session = session;
    object->access.vs.user_id = user_id;
    object->access.vs.dataset_id = dataset_id;
    object->access.vs.routes = (VSSI_Route*)(object + 1);
    object->access.vs.num_routes = route_info->num_routes;
    object->request_size = 0;
    tmp = (char*)(object->access.vs.routes + route_info->num_routes);
    // sort the routes into place in order of preference
    k = 0;
    for(i = 0 ; i < MAX_ROUTE_PREF ; i++ ) {
        for(j = 0; j < route_info->num_routes; j++) {
            // Skip routes of the wrong type
            if ( route_info->routes[j].type != route_pref[i] ) {
                continue;
            }
            // we found one, copy it into place
            memcpy(&object->access.vs.routes[k], &route_info->routes[j],
                   sizeof(VSSI_Route));
            strcpy(tmp, route_info->routes[j].server);
            object->access.vs.routes[k].server = tmp;
            tmp += strlen(tmp) + 1;
            k++;
        }
    }

    // What do we do with unknown route types? Toss them?
    if ( k == 0 ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "No valid route types specified for session "FMTu64,
                          session->handle);
        free(object);
        object = NULL;
        goto exit;
    }
    else if ( k < route_info->num_routes ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "%d valid route types of %d specified for session "
                          FMTu64, k, route_info->num_routes,
                          session->handle);
        object->access.vs.num_routes = k;
    }

    object->access.vs.cluster_id = object->access.vs.routes[0].cluster_id;

    VPLMutex_Lock(&vssi_mutex);
    object->next = session->objects;
    session->objects = object;
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Object %p for session "FMTu64" being added.",
                      object, session->handle);
    VPLMutex_Unlock(&vssi_mutex);

 exit:
    return object;
}


int VSSI_BuildDatasetObject(const char* body_xml,
                            VSSI_ObjectState** object)
{
    int rv = -1;
    char* uid_start;
    char* did_start;
    char* routes_start;
    char* route_start;
    char* route_body = NULL;
    char* field_end;
    int num_routes = 0;
    size_t object_size;
    int i;
    void* tmp;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Building dataset object %p size "FMTu_size_t" from XML {%s}.",
                        *object, sizeof(VSSI_ObjectState), body_xml);

    // Find all components
    uid_start = strstr(body_xml, "<uid>");
    if(uid_start == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find uid in {%s}",
                         body_xml);
        goto exit;
    }
    uid_start += strlen("<uid>");
    field_end = strstr(uid_start, "</uid>");
    if(field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find /uid in {%s}",
                         body_xml);
        goto exit;
    }

    did_start = strstr(body_xml, "<did>");
    if(did_start == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find did in {%s}",
                         body_xml);
        goto exit;
    }
    did_start += strlen("<did>");
    field_end = strstr(did_start, "</did>");
    if(field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find /did in {%s}",
                         body_xml);
        goto exit;
    }

    routes_start = strstr(body_xml, "<routes>");
    if(routes_start == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find routes in {%s}",
                         body_xml);
        goto exit;
    }
    field_end = strstr(routes_start, "</routes>");
    if(field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find routes in {%s}",
                         body_xml);
        goto exit;
    }

    (*object)->access.vs.user_id = strtoull(uid_start, 0, 16);
    (*object)->access.vs.dataset_id = strtoull(did_start, 0, 16);

    // Count routes
    route_start = strstr(routes_start, "<route>");
    while(route_start != NULL) {
        field_end = strstr(route_start, "</route>");
        if(field_end == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not find /route tag in {%s}",
                             route_start);
            goto exit;
        }
        num_routes++;
        route_start = strstr(field_end, "<route>");
    }

    // Allocate space for routes
    object_size = (sizeof(VSSI_ObjectState) +
                   num_routes * sizeof(VSSI_Route));
    tmp = realloc(*object, object_size);
    if(tmp == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not allocate space for %d routes",
                         num_routes);
        goto exit;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object was at %p size "FMTx_size_t", now %p size "FMTx_size_t".",
                        *object, sizeof(VSSI_ObjectState),
                        tmp, object_size);

    *object = (VSSI_ObjectState*)tmp;
    (*object)->access.vs.routes = (VSSI_Route*)(*object + 1);
    (*object)->access.vs.num_routes = num_routes;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object has %d routes starting at %p.",
                        (*object)->access.vs.num_routes,
                        (*object)->access.vs.routes);

    // For each component, add details.
    route_start = strstr(routes_start, "<route>");
    for(i = 0; i < num_routes; i++) {
        VSSI_ObjectState* new_object;
        size_t route_length;
        route_start += strlen("<route>");
        field_end = strstr(route_start, "</route>");
        route_length = field_end - route_start;

        tmp = realloc(route_body, route_length + 1);
        if(tmp == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not allocate space for route %d.",
                             i);
            goto exit;
        }
        route_body = (char*)tmp;
        strncpy(route_body, route_start, route_length);
        route_body[route_length] = '\0';

        new_object = *object;
        rv = VSSI_BuildDatasetRoute(route_body,
                                    &((*object)->access.vs.routes[i]),
                                    &(new_object), &object_size);
        if(rv != 0) {
            goto exit;
        }

        if(new_object != *object) {
            int j;
            new_object->access.vs.routes = (VSSI_Route*)(new_object + 1);
            for(j = 0; j < i; j++) {
                VSSI_RebaseDatasetRoute(&(new_object->access.vs.routes[j]),
                                        (char*)new_object - (char*)(*object));
            }
            *object = new_object;
        }

        route_start = strstr(field_end, "<route>");
    }

    // If any route has a nonzero cluster ID, use that cluster ID for the server cluster ID.
    (*object)->access.vs.cluster_id = 0;
    for(i = 0; i < num_routes; i++) {
        if((*object)->access.vs.routes[i].cluster_id != 0) {
            (*object)->access.vs.cluster_id = (*object)->access.vs.routes[i].cluster_id;
            break;
        }
    }

    rv = 0;
 exit:
    if(route_body) {
        free(route_body);
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object %p built.", *object);

    return rv;
}

int VSSI_BuildDatasetRoute(const char* route_xml,
                           VSSI_Route* route,
                           VSSI_ObjectState** object,
                           size_t* object_size)
{
    int rv = -1;
    char* routetype_start;
    size_t routetype_size;
    char* protocol_start;
    size_t protocol_size;
    char* server_start;
    size_t server_size;
    char* port_start;
    char* clusterid_start;
    char* field_end;
    void* tmp;
    size_t new_size;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Building route at %p for object %p size "FMTu_size_t" from XML {%s}.",
                        route, *object, *object_size, route_xml);

    // Find all the component fields.
    routetype_start = strstr(route_xml, "<routetype>");
    field_end = strstr(route_xml, "</routetype>");
    if(routetype_start == NULL || field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find routetype tags in {%s}.",
                         route_xml);
        goto exit;
    }
    routetype_start = routetype_start + strlen("<routetype>");
    routetype_size = field_end - routetype_start;

    protocol_start = strstr(route_xml, "<protocol>");
    field_end = strstr(route_xml, "</protocol>");
    if(protocol_start == NULL || field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find protocol tags in {%s}.",
                         route_xml);
        goto exit;
    }
    protocol_start = protocol_start + strlen("<protocol>");
    protocol_size = field_end - protocol_start;

    server_start = strstr(route_xml, "<server>");
    field_end = strstr(route_xml, "</server>");
    if(server_start == NULL || field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find server tags in {%s}.",
                         route_xml);
        goto exit;
    }
    server_start = server_start + strlen("<server>");
    server_size = field_end - server_start;

    port_start = strstr(route_xml, "<port>");
    field_end = strstr(route_xml, "</port>");
    if(port_start == NULL || field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find port tags in {%s}.",
                         route_xml);
        goto exit;
    }
    port_start = port_start + strlen("<port>");

    clusterid_start = strstr(route_xml, "<clusterid>");
    field_end = strstr(route_xml, "</clusterid>");
    if(clusterid_start == NULL || field_end == NULL) {
        // only an error for proxied routes. Detect later.
        clusterid_start = NULL;
    }
    else {
        clusterid_start = clusterid_start + strlen("<clusterid>");
    }

    // Expand object space for added data.
    // Each string gets an additional NULL terminator
    new_size = *object_size;
    new_size += server_size + 1;
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Need "FMTu_size_t" bytes for object ("FMTu_size_t"), server ("FMTu_size_t").",
                        new_size, *object_size, server_size);
    tmp = realloc(*object, new_size);
    if(tmp == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not allocate "FMTu_size_t" bytes of space for route details",
                         new_size);
        goto exit;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object was at %p size "FMTx_size_t", now %p size "FMTx_size_t".",
                        *object, *object_size,
                        tmp, new_size);
    if((char*)tmp != (char*)*object) {
        route = (VSSI_Route*)((char*)route +
                              ((char*)tmp - ((char*)*object)));
    }
    *object = (VSSI_ObjectState*)tmp;

    route->server = ((char*)(*object)) + *object_size;
    strncpy(route->server, server_start, server_size);
    route->server[server_size] = '\0';
    *object_size += server_size + 1;

    route->port = strtoul(port_start, 0, 10);

    if(clusterid_start != NULL) {
        route->cluster_id = strtoull(clusterid_start, 0, 16);
    }
    else {
        route->cluster_id = 0;
    }

    if(strncmp("direct", routetype_start, routetype_size) == 0) {
        route->type = VSSI_ROUTE_DIRECT_EXTERNAL;
    }
    else if(strncmp("proxy", routetype_start, routetype_size) == 0) {
        route->type = VSSI_ROUTE_PROXY;
        if(clusterid_start == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "No cluster ID found for proxy route in XML: {%s}",
                             route_xml);
            goto exit;
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unknown route type in XML: {%s}",
                         route_xml);
        goto exit;
    }

    if(strncmp("VSS", protocol_start, protocol_size) == 0) {
        route->proto = VSSI_PROTO_VS;
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Unknown protocol in XML: {%s}",
                         route_xml);
        goto exit;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Built route %p: server {%s} port %u type: (%d)%s proto: (%d)%s cluster ID:"FMTu64"",
                        route, route->server, route->port,
                        route->type,
                        VSSI_ROUTE_IS_DIRECT(route->type) ? "direct" : "proxy",
                        route->proto,
                        route->proto == VSSI_PROTO_VS ? "VSS" : "???",
                        route->cluster_id);
    rv = 0;
 exit:

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object %p built.", *object);

    return rv;
}

void VSSI_RebaseDatasetRoute(VSSI_Route* route,
                             size_t offset)
{
    route->server += offset;
}

int VSSI_BuildContentObject(const char* body_xml,
                            VSSI_ObjectState** object)
{
    int rv = -1;
    int num_components = 0;
    char* component_start;
    char* component_end;
    char* component_body = NULL;
    void* tmp;
    size_t object_size;
    int i;

    // Count the components.
    component_start = strstr(body_xml, "<component>");
    while(component_start != NULL) {
        component_end = strstr(component_start, "</component>");
        if(component_end == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not find end component tag in {%s}",
                             body_xml);
            goto exit;
        }
        num_components++;
        component_start = strstr(component_end, "<component>");
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Found %d components.", num_components);

    if(num_components == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find components in {%s}",
                         body_xml);
        goto exit;
    }

    // Allocate space for components.
    object_size = (sizeof(VSSI_ObjectState) +
                   num_components * sizeof(VSSI_HTTPComponent));
    tmp = realloc(*object, object_size);
    if(tmp == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not allocate space for %d components",
                         num_components);
        goto exit;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object was at %p size "FMTx_size_t", now %p size "FMTx_size_t".",
                        *object, sizeof(VSSI_ObjectState),
                        tmp, object_size);

    *object = (VSSI_ObjectState*)tmp;
    (*object)->access.http.components = (VSSI_HTTPComponent*)(*object + 1);
    (*object)->access.http.num_components = num_components;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object has %d components starting at %p.",
                        (*object)->access.http.num_components,
                        (*object)->access.http.components);

    // For each component, add details.
    component_start = strstr(body_xml, "<component>");
    for(i = 0; i < num_components; i++) {
        VSSI_ObjectState* new_object;
        size_t component_length;
        component_start += strlen("<component>");
        component_end = strstr(component_start, "</component>");
        component_length = component_end - component_start;

        tmp = realloc(component_body, component_length + 1);
        if(tmp == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not allocate space for component %d.",
                             i);
            goto exit;
        }
        component_body = (char*)tmp;
        strncpy(component_body, component_start, component_length);
        component_body[component_length] = '\0';

        new_object = *object;
        rv = VSSI_BuildContentComponent(component_body,
                                        &((*object)->access.http.components[i]),
                                        &(new_object), &object_size);
        if(rv != 0) {
            goto exit;
        }

        if(new_object != *object) {
            int j;
            new_object->access.http.components = (VSSI_HTTPComponent*)(new_object + 1);
            for(j = 0; j < i; j++) {
                VSSI_RebaseContentComponent(&(new_object->access.http.components[j]),
                                            (char*)new_object - (char*)(*object));
            }
            *object = new_object;
        }

        component_start = strstr(component_end, "<component>");
    }

    for(i = 0; i < num_components; i++) {
        VSSI_HTTPComponent* component = &((*object)->access.http.components[i]);

        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Built component %d of object %p at %p.",
                            i, *object, component);
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Built component {%s} at server {%s} IP/port "FMT_VPLNet_addr_t":%d uri {%s}",
                            component->name, component->server,
                            VAL_VPLNet_addr_t(component->server_inaddr.addr),
                            VPLNet_port_ntoh(component->server_inaddr.port),
                            component->uri);
    }

    rv = 0;
 exit:
    if(component_body) {
        free(component_body);
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object %p built.", *object);

    return rv;
}

int VSSI_BuildContentComponent(const char* component_xml,
                               VSSI_HTTPComponent* component,
                               VSSI_ObjectState** object,
                               size_t* object_size)
{
    int rv = -1;
    char* type_start;
    size_t type_size;
    char* server_start;
    size_t server_size;
    char* port_start;
    char* uri_start;
    size_t uri_size;
    char* field_end;
    void* tmp;
    size_t new_size;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Building component at %p for object %p size "FMTu_size_t" from XML {%s}.",
                        component, *object, *object_size, component_xml);

    // Find all the component fields.
    type_start = strstr(component_xml, "<type>");
    field_end = strstr(component_xml, "</type>");
    if(type_start == NULL || field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find type tags in {%s}.",
                         component_xml);
        goto exit;
    }
    type_start = type_start + strlen("<type>");
    type_size = field_end - type_start;

    server_start = strstr(component_xml, "<url>");
    field_end = strstr(component_xml, "</url>");
    if(server_start == NULL || field_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find url tags in {%s}.",
                         component_xml);
        goto exit;
    }
    server_start = strstr(server_start, "http://");
    if(server_start == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find {http://} in {%s}.",
                         component_xml);
        goto exit;
    }
    server_start += strlen("http://");
    // Port is optional.
    port_start = strchr(server_start, ':');
    if(port_start) {
        server_size = port_start - server_start;
        port_start++;
        uri_start = strchr(port_start, '/');
        if(uri_start == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not find uri in {%s}",
                             component_xml);
            goto exit;
        }
        // Keep leading slash.
    }
    else {
        field_end = strchr(server_start, '/');
        if(field_end == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not find uri in {%s}",
                             component_xml);
            goto exit;
        }
        server_size = field_end - server_start;
        uri_start = field_end;
        // Keep leading slash.
    }
    field_end = strchr(uri_start, '<');
    uri_size = field_end - uri_start;

    // Expand object space for added data.
    // Each string gets an additional NULL terminator
    new_size = *object_size;
    new_size += type_size + 1;
    new_size += server_size + 1;
    new_size += uri_size + 1;
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Need "FMTu_size_t" bytes for object ("FMTu_size_t"), type ("FMTu_size_t"), server ("FMTu_size_t") and uri ("FMTu_size_t").",
                        new_size, *object_size, type_size, server_size, uri_size);
    tmp = realloc(*object, new_size);
    if(tmp == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not allocate "FMTu_size_t" bytes of space for component details",
                         new_size);
        goto exit;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object was at %p size "FMTx_size_t", now %p size "FMTx_size_t".",
                        *object, *object_size,
                        tmp, new_size);
    if((char*)tmp != (char*)*object) {
        component = (VSSI_HTTPComponent*)((char*)component +
                                          ((char*)tmp - ((char*)*object)));
    }
    *object = (VSSI_ObjectState*)tmp;

    component->name = ((char*)(*object)) + *object_size;
    strncpy(component->name, type_start, type_size);
    component->name[type_size] = '\0';
    *object_size += type_size + 1;

    component->server = ((char*)(*object)) + *object_size;
    strncpy(component->server, server_start, server_size);
    component->server[server_size] = '\0';
    *object_size += server_size + 1;

    component->uri = ((char*)(*object)) + *object_size;
    strncpy(component->uri, uri_start, uri_size);
    component->uri[uri_size] = '\0';
    *object_size += uri_size + 1;

    component->server_inaddr.family = VPL_PF_INET;
    if(port_start) {
        component->server_inaddr.port = VPLNet_port_hton(strtoul(port_start, 0, 10));
    }
    else {
        component->server_inaddr.port = VPLNet_port_hton(HTTP_SERVER_PORT);
    }
    component->server_inaddr.addr = VPLNet_GetAddr(component->server);

    component->server_connection = NULL;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Built component %p {%s} at server {%s} IP/port "FMT_VPLNet_addr_t":%d uri {%s} from XML {%s}",
                        component, component->name, component->server,
                        VAL_VPLNet_addr_t(component->server_inaddr.addr),
                        VPLNet_port_ntoh(component->server_inaddr.port),
                        component->uri, component_xml);
    rv = 0;
 exit:

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object %p built.", *object);

    return rv;
}

void VSSI_RebaseContentComponent(VSSI_HTTPComponent* component,
                                 size_t offset)
{
    // Adjust each pointer by the offset.
    component->name += offset;
    component->server += offset;
    component->uri += offset;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Re-based by "FMTu_size_t" component %p {%s} at server {%s} IP/port "FMT_VPLNet_addr_t":%d uri {%s}",
                        offset, component, component->name, component->server,
                        VAL_VPLNet_addr_t(component->server_inaddr.addr),
                        VPLNet_port_ntoh(component->server_inaddr.port),
                        component->uri);
}

int VSSI_BuildAtomicObject(const char* body_xml,
                           VSSI_ObjectState** object)
{
    // This is a legacy format, soon to be deprecated.
    // Atomic object format means exactly one URL exists for the object.
    int rv = -1;
    char* server_start = NULL;
    char* server_end = NULL;
    size_t server_length;
    char* port_start = NULL;
    char* uid_start = NULL;
    char* did_start = NULL;
    VSSI_VSAccess* access;
    void * tmp;
    size_t object_size;
    
    // Find all the components.
    server_start = strstr(body_xml, "vss://");
    if(server_start == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find {vss://} in {%s}",
                         body_xml);
        goto exit;
    }
    server_start += strlen("vss://");
    // Port is optional.
    port_start = strchr(server_start, ':');
    if(port_start) {
        server_end = port_start;
        port_start++;
        uid_start = strchr(port_start, '/');
        if(uid_start == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not find uid in {%s}",
                             body_xml);
            goto exit;
        }
        uid_start++;
    }
    else {
        server_end = strchr(server_start, '/');
        if(server_end == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Could not find dataset identifiers in {%s}",
                             body_xml);
            goto exit;
        }
        uid_start = server_end + 1;
    }
    if(server_end == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find end of server name in {%s}",
                         body_xml);
        goto exit;
    }
    server_length = server_end - server_start;
    
    // DID starts after last slash.
    did_start = strchr(uid_start, '/');
    if(did_start == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Could not find did in {%s}",
                         body_xml);
        goto exit;
    }
    did_start++;
    while(strchr(did_start, '/') != NULL) {
        did_start = strchr(did_start, '/') + 1;
    }
    
    // Populate the access struct. We have one route.
    object_size = sizeof(VSSI_ObjectState) + sizeof(VSSI_Route) + server_length + 1;
    tmp = realloc(*object, object_size);
    if(tmp == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to allocate space for object routes.");
        goto exit;
    }
    *object = tmp;
    access = &((*object)->access.vs);
    access->routes = (VSSI_Route*)((*object) + 1);
    access->routes[0].server = (char*)(access->routes + 1);
    access->num_routes = 1;
    
    access->user_id = strtoull(uid_start, 0, 16);
    access->dataset_id = strtoull(did_start, 0, 16);
    
    strncpy(access->routes[0].server, server_start, server_length);
    access->routes[0].server[server_length] = '\0';
    if(port_start) {
        access->routes[0].port = strtoul(port_start, 0, 10);
    }
    else {
        access->routes[0].port = VSS_SERVER_PORT;
    }
    access->routes[0].type = VSSI_ROUTE_DIRECT_EXTERNAL;
    access->routes[0].proto = VSSI_PROTO_VS;
    access->routes[0].cluster_id = 0;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Built atomic object at %p "FMTu64":"FMTu64" with route server {%s} port %u from XML {%s}",
                        *object,
                        access->user_id, access->dataset_id,
                        access->routes[0].server,
                        access->routes[0].port,
                        body_xml);
    
    rv = 0;
 exit:
    
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Object %p built.", *object);
    
    return rv;
}

int VSSI_CheckObjectConflict(VSSI_ObjectState* object)
{
    int rv = VSSI_SUCCESS;

    if((object->mode & VSSI_FORCE) == 0) {
        if(object->version != object->new_version) {
            rv = VSSI_CONFLICT;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Object(%p): Version conflict ("FMTu64" vs "FMTu64") previously detected, not cleared.",
                             object, object->version, object->new_version);
        }
    }

    return rv;
}

/// Use only to confirm if a session is validly registered.
VPL_BOOL VSSI_SessionRegistered(VSSI_UserSession* session)
{
    VPL_BOOL rv = VPL_FALSE;
    VSSI_SessionNode* node = NULL;

    VPLMutex_Lock(&vssi_mutex);

    for(node = user_session_list_head; node != NULL; node = node->next) {
        if(&(node->session) == session) {
            rv = VPL_TRUE;
            break;
        }
    }

    VPLMutex_Unlock(&vssi_mutex);

    return rv;
}

int connect_server(VPLSocket_t* socket,
                   const char* server, VPLNet_port_t port,
                   int reuse_addr)
{
    // TODO: Try each IP address for this server name once, then give up.
    // FORNOW: Try 3 times, looking-up IP address each time. Skip trial if IP is same as last trial.
    int rv = -1;
    VPLNet_addr_t addr;
    int tries = 3;
    VPLSocket_addr_t inaddr;
    inaddr.family = VPL_PF_INET;
    inaddr.port = VPLNet_port_hton(port);
    inaddr.addr = VPLNET_ADDR_INVALID;

    while(tries-- > 0) {
        addr = VPLNet_GetAddr(server);
        if(addr == inaddr.addr) {
            // Repeated address. Skip try.
            continue;
        }

        inaddr.addr = addr;

        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Connecting to server %s via address "FMT_VPLNet_addr_t":%u.",
                          server,
                          VAL_VPLNet_addr_t(inaddr.addr),
                          VPLNet_port_ntoh(inaddr.port));
        
        rv = reconnect_server(socket, &inaddr, reuse_addr, NULL,
                              4, 5);  // use these timeouts in VPLSocket_Connect*()
        if(rv != -1) {
            // connection made            
            break;
        }
    }

    return rv;
}

int reconnect_server(VPLSocket_t* socket, VPLSocket_addr_t* inaddr,
                     int reuse_addr, VPLSocket_addr_t* use_inaddr,
                     int timeout_nonroutable, int timeout_routable)
{
    int rv = -1;

    // If there is no open socket to this server, open a socket.
    if(VPLSocket_Equal(*socket, VPLSOCKET_INVALID)) {
        int yes = 1;
        int rc;
        VPLSocket_t sockfd = VPLSOCKET_INVALID;

        sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Created socket "FMT_VPLSocket_t" to connect to server "FMT_VPLNet_addr_t":%d.",
                          VAL_VPLSocket_t(sockfd),
                          VAL_VPLNet_addr_t(inaddr->addr),
                          VPLNet_port_ntoh(inaddr->port));
        if(VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to open socket to server "FMT_VPLNet_addr_t":%d.",
                             VAL_VPLNet_addr_t(inaddr->addr),
                             VPLNet_port_ntoh(inaddr->port));
            goto exit;
        }
#ifndef VPL_PLAT_IS_WINRT
        if(reuse_addr) {
            // Must use SO_REUSEADDR option. 
            rc = VPLSocket_SetSockOpt(sockfd, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                                      (void*)&yes, sizeof(yes));
            if(rc != VPL_OK) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Failed to set SO_REUSEADDR on socket "FMT_VPLSocket_t": %d.",
                                  VAL_VPLSocket_t(sockfd), rc);
                VPLSocket_Close(sockfd);
                sockfd = VPLSOCKET_INVALID;
                goto exit;
            }

            if(use_inaddr) {
                // Must use specific address.
                rc = VPLSocket_Bind(sockfd, use_inaddr, sizeof(VPLSocket_addr_t));
                if (rc != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Failed to bind socket. socket:"FMT_VPLSocket_t", error:%d",
                                     VAL_VPLSocket_t(sockfd), rc);
                    VPLSocket_Close(sockfd);
                    sockfd = VPLSOCKET_INVALID;
                    goto exit;
                }
            }
        }
#endif
        /* Set TCP no delay for performance reasons. */
        rc = VPLSocket_SetSockOpt(sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                                  (void*)&yes, sizeof(yes));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Failed to set TCP_NODELAY on socket "FMT_VPLSocket_t": %d.",
                              VAL_VPLSocket_t(sockfd), rc);
        }

        // Set TCP keep-alive on, probe after 30 seconds inactive, 
        // repeat every 10 seconds, fail if inactive for 60 seconds.
        rc = VPLSocket_SetKeepAlive(sockfd, true, 30, 10, 3);
        if(rc != VPL_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Failed to set TCP_KEEPALIVE on socket "FMT_VPLSocket_t": %d.",
                              VAL_VPLSocket_t(sockfd), rc);
        }

        rc = VPLSocket_ConnectWithTimeouts(sockfd, inaddr,
                                           sizeof(VPLSocket_addr_t),
                                           timeout_nonroutable >= 0 ? VPLTime_FromSec(timeout_nonroutable) : VPLTime_FromSec(1),
                                           timeout_routable >= 0 ? VPLTime_FromSec(timeout_routable) : VPLTime_FromSec(5));
        if (rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed (%d) to connect to server "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                             rc,
                             VAL_VPLNet_addr_t(inaddr->addr),
                             VPLNet_port_ntoh(inaddr->port),
                             VAL_VPLSocket_t(sockfd));
            VPLSocket_Close(sockfd);
            sockfd = VPLSOCKET_INVALID;
            *socket = VPLSOCKET_INVALID;
            goto exit;
        }

        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Opened socket "FMT_VPLSocket_t" to server "FMT_VPLNet_addr_t":%d.",
                          VAL_VPLSocket_t(sockfd),
                          VAL_VPLNet_addr_t(inaddr->addr),
                          VPLNet_port_ntoh(inaddr->port));
        *socket = sockfd;
        VSSI_NotifySocketChangeFn();
        rv = 1; // new connection made
    }
    else {
        rv = 0; // existing connection used
    }

 exit:
    return rv;
}

void VSSI_ConnectVSServer(VSSI_VSConnectionNode* server_connection, VSSI_Result result)
{
    int rc;
    // Always connect for first server connection.
    VSSI_VSConnection* connection = server_connection->connections;

    if(connection != NULL) {
        // Disconnect any connected socket.
        if(!VPLSocket_Equal(VPLSOCKET_INVALID, connection->conn_id)) {
            VPLSocket_Close(connection->conn_id);
            connection->conn_id = VPLSOCKET_INVALID;
            connection->connected = 0;
        }
    }
    else {
        // Create new connection for next attempt.
        // Make one sub-connection for starters.
        connection = (VSSI_VSConnection*)calloc(sizeof(VSSI_VSConnection), 1);
        if(connection == NULL)  {
            result = VSSI_NOMEM;
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Connect to server "FMTu64" fails: out of memory.",
                              server_connection->cluster_id);
            goto fail;
        }

        connection->conn_id = VPLSOCKET_INVALID;
        connection->last_active = VPLTime_GetTimeStamp();
        connection->inactive_timeout = VSSI_VS_CONNECTION_TIMEOUT;

        // Add to the server connections list.
        server_connection->connections = connection;
    }

    // Try routes until out of routes.
    while(server_connection->object->access.vs.num_routes > server_connection->route_id) {
        VSSI_Route* route = &(server_connection->object->access.vs.routes[server_connection->route_id]);
        server_connection->route_id++;

        switch(route->type) {
        case VSSI_ROUTE_DIRECT_INTERNAL:
        case VSSI_ROUTE_DIRECT_EXTERNAL:
        default: {
            rc = connect_server(&(connection->conn_id),
                                route->server, route->port, 0);
            if(rc != -1) {
                // Connected! Authenticate the connection.

                char header[VSS_HEADER_SIZE] = {0};
                char request[VSS_AUTHENTICATE_SIZE_EXT_VER_0] = {0};
                VSSI_MsgBuffer* msg = NULL;
                
                vss_set_command(header, VSS_AUTHENTICATE);
                vss_set_xid(header, 0);
                vss_authenticate_set_cluster_id(request, server_connection->cluster_id);
                
                msg = VSSI_PrepareMessage(header, request, VSS_AUTHENTICATE_SIZE_EXT_VER_0,
                                          NULL, 0, NULL, 0);
                if(msg == NULL) {
                    // Try next route... probably will end up failing.
                    server_connection->connect_state = CONNECTION_CONNECTING;
                    break;
                }

                msg = VSSI_ComposeMessage(server_connection->object->user_session,
                                          VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                          VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                                          VSS_NEGOTIATE_ENCRYPT_TYPE_AES128,
                                          msg);
                if(msg == NULL) {
                    // Try next route... probably will end up failing.
                    server_connection->connect_state = CONNECTION_CONNECTING;
                    break;
                }

                server_connection->type = route->type;
                server_connection->connect_state = CONNECTION_CONNECTED;
                connection->connected = 1;
                connection->cmds_outstanding++;

                VSSI_Sendall(connection->conn_id, msg->msg, msg->length);
                free(msg);
                
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Connected to server "FMTu64" on socket "FMT_VPLSocket_t".",
                                  server_connection->cluster_id, VAL_VPLSocket_t(connection->conn_id));

                // Will complete operation on authenticate reply.
                goto exit;
            }
        } break;

        case VSSI_ROUTE_PROXY: {
            // Need proxy connection.
            VSSI_ConnectProxyState* state =
                (VSSI_ConnectProxyState*)calloc(sizeof(VSSI_ConnectProxyState), 1);
            if(state == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Unable to allocate proxy connect state.");
                // Try next route... will probably fail.
                break;
            }

            state->session = server_connection->object->user_session;
            state->callback = VSSI_ConnectVSServerDone;
            state->ctx = server_connection;
            state->socket = &(connection->conn_id);
            state->conn_id = VPLSOCKET_INVALID;
            state->p2p_conn_id = VPLSOCKET_INVALID;
            state->p2p_flags = VSSI_PROXY_TRY_P2P;
            state->type = VSS_PROXY_CONNECT_TYPE_VSSP;
            state->destination_device = route->cluster_id;
            VSSI_VSConnectProxy(state, route->server, route->port);
            // Will complete operation on proxy-connect reply.
            goto exit;

        } break;
        } // switch(route->type)
    }

    if(server_connection->object->access.vs.num_routes <= server_connection->route_id) {
        // Out of routes.
        result = VSSI_COMM;
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Connect to server "FMTu64" fails: all routes failed.",
                          server_connection->cluster_id);
    fail:
        // Connection fails.
        server_connection->connect_state = CONNECTION_FAILED; // can't be used anymore
        connection->connected = 1; // giving up.

        VSSI_HandleLostConnection(server_connection, NULL, result);
    }

 exit:
    return;
}

void VSSI_ConnectVSServerDone(void* ctx, VSSI_Result result)
{
    VSSI_VSConnectionNode* connection = (VSSI_VSConnectionNode*)(ctx);
    int rc;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Connection %p result %d.", connection, result);

    if(result == VSSI_SUCCESS) {
        // Connected to correct destination.
        // Trigger negotiation for all send states waiting for connection.
        VSSI_SessionNode* session;
        VSSI_Route* route;
        VPLMutex_Lock(&vssi_mutex);

        VPLMutex_Lock(&(connection->send_mutex));

        // Set route-used information.
        route = &(connection->object->access.vs.routes[connection->route_id - 1]);
        connection->type = route->type;
        connection->inaddr.port = VPLSocket_GetPort(connection->connections->conn_id);
        connection->inaddr.addr = VPLSocket_GetAddr(connection->connections->conn_id);
        connection->connect_state = CONNECTION_ACTIVE;
        connection->connections->connected = 1;

        VPLMutex_Unlock(&(connection->send_mutex));

        for(session = user_session_list_head; session != NULL; session = session->next) {
            VSSI_SendState* send_state = session->session.send_states;
            while(send_state != NULL && send_state->server_connection != connection) {
                send_state = send_state->next;
            }

            if(send_state) {
                rc = VSSI_NegotiateSession(connection->object, send_state, NULL, 0);
                if(rc != VSSI_SUCCESS) {
                    // Failed to negotiate. Fail send state requests.
                    VSSI_ReplyPendingCommands(send_state, NULL, rc);
                }
            }
        }        

        VPLMutex_Unlock(&vssi_mutex);
    } 
    else {
        // Try next route.
        VPLMutex_Lock(&(connection->send_mutex));

        VSSI_ConnectVSServer(connection, result);

        VPLMutex_Unlock(&(connection->send_mutex));
    }
}

void VSSI_VSConnectProxy(VSSI_ConnectProxyState* state,
                         const char* server, VPLNet_port_t port)
{
    VPLNet_port_t p2p_port = VPLNET_PORT_INVALID;
    VPLSocket_t new_socket = VPLSOCKET_INVALID;
    VPLMutex_Lock(&vssi_mutex);

    // Keep session from ending by adding to connect count.
    state->session->connect_count++;
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Session "FMTu64" opening proxy connection.",
                      state->session->handle);

    // Add to list of pending proxy connections, but as an inactive proxy.
    state->last_active = VPLTime_GetTimeStamp();
    state->inactive_timeout = VSSI_PROXY_CONNECT_TIMEOUT;
    state->conn_id = VPLSOCKET_INVALID;
    state->next = proxy_connection_list_head;
    proxy_connection_list_head = state;

    VPLMutex_Unlock(&vssi_mutex);

    // Attempt to connect to proxy server. Use SO_REUSEADDR if P2P needed.
    {
        int rc = connect_server(&(new_socket),
                                server, port,
                                state->p2p_flags);

        if(rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to open connection for proxy to "FMTu64".",
                             state->destination_device);
            VSSI_HandleProxyDone(state, VSSI_COMM);
            return;
        }
    }
    
    // Start timeout clock after connection made.
    VPLMutex_Lock(&vssi_mutex);
    state->conn_id = new_socket;
    state->last_active = VPLTime_GetTimeStamp();
    VPLMutex_Unlock(&vssi_mutex);

    // Open P2P socket if P2P attempt should be made.
    // Send port ID of proxy socket to indicate P2P attempt.
    if(state->p2p_flags) {
        p2p_port = VPLNet_port_ntoh(VPLSocket_GetPort(state->conn_id));
    }

    // Send proxy request message.
    {
        VSSI_Result result = VSSI_SendProxyRequest(state->session,
                                                   state->destination_device,
                                                   state->type,
                                                   p2p_port,
                                                   state->conn_id);
        state->last_active = VPLTime_GetTimeStamp();
        state->inactive_timeout = VSSI_VS_PROXY_CONNECTION_TIMEOUT;
        if(result != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to send proxy request message for proxy to "FMTu64".",
                             state->destination_device);
            VSSI_HandleProxyDone(state, result);
            return;
        }
    }

    // begin monitoring the proxy connect socket
    VSSI_NotifySocketChangeFn();
}

void VSSI_HandleProxyDone(VSSI_ConnectProxyState* state, VSSI_Result result)
{
    VPLMutex_Lock(&vssi_mutex);

    // Remove state from list.
    if(state == proxy_connection_list_head) {
        proxy_connection_list_head = state->next;
    }
    else {
        VSSI_ConnectProxyState* prev = proxy_connection_list_head;
        while(prev != NULL && prev->next != state) {
            prev = prev->next;
        }
        if(prev != NULL) {
            prev->next = state->next;
        }
    }
    
    // no longer counts against session.
    state->session->connect_count--;

    VPLMutex_Unlock(&vssi_mutex);

    // Close sockets if open.
    if(!VPLSocket_Equal(state->conn_id, VPLSOCKET_INVALID)) {
        VPLSocket_Close(state->conn_id);
        state->conn_id = VPLSOCKET_INVALID;
    }
    if(!VPLSocket_Equal(state->p2p_conn_id, VPLSOCKET_INVALID)) {
        VPLSocket_Close(state->p2p_conn_id);
        state->p2p_conn_id = VPLSOCKET_INVALID;
    }
    if(!VPLSocket_Equal(state->p2p_punch_id, VPLSOCKET_INVALID)) {
        VPLSocket_Close(state->p2p_punch_id);
        state->p2p_punch_id = VPLSOCKET_INVALID;
    }
    if(!VPLSocket_Equal(state->p2p_listen_id, VPLSOCKET_INVALID)) {
        VPLSocket_Close(state->p2p_listen_id);
        state->p2p_listen_id = VPLSOCKET_INVALID;
    }

    // If proxy made, result is success. P2P may have failed or timed-out.
    if(state->p2p_flags & VSSI_PROXY_DO_P2P) {
        result = VSSI_SUCCESS;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Done with proxy connect attempt to device "FMTu64", result:%d.",
                      state->destination_device, result);

    // Call callback.
    if(state->callback) {
        (state->callback)(state->ctx, result);
    }

    if(state->incoming_reply) { free(state->incoming_reply); }
    free(state);

    // One way or another, a socket no longer needs service.
    VSSI_NotifySocketChangeFn();
}

void VSSI_ReceiveProxyResponse(VSSI_ConnectProxyState* state)
{
    int rc;
    char* data;

    // Receive incoming message
    if(state->replylen == 0) {
        // Receive header.

        if(state->incoming_reply == NULL) {
            // Allocate for header
            state->incoming_reply = (char*)calloc(VSS_HEADER_SIZE, 1);
            if(state->incoming_reply == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to allocate buffer for proxy request reply.");
                return;
            }
        }
        // Receive header as much as possible.
        if(state->so_far < VSS_HEADER_SIZE) {
            rc = VPLSocket_Recv(state->conn_id, state->incoming_reply + state->so_far, 
                                VSS_HEADER_SIZE - state->so_far);
            if(rc == 0) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Got socket closed for connection "FMT_VPLSocket_t".",
                                  VAL_VPLSocket_t(state->conn_id));
                VSSI_HandleProxyDone(state, VSSI_COMM);
                return;
            }
            else if(rc < 0) {
                // handle expected socket errors
                switch(rc) {
                case VPL_ERR_AGAIN:
                case VPL_ERR_INTR:
                    // These are OK. Try again later.
                    return;
                    break;
                    
                default:
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Got socket error %d for connection "FMT_VPLSocket_t".",
                                      rc, VAL_VPLSocket_t(state->conn_id));
                    VSSI_HandleProxyDone(state, VSSI_COMM);
                    return;
                    break;
                }
            }
            else {
                // Handle received bytes.
                state->so_far += rc;
            }
        }

        if(state->so_far == VSS_HEADER_SIZE) {
            void* tmp;

            // Verify if not an ERROR.
            if(vss_get_command(state->incoming_reply) == VSS_ERROR) {
                VSSI_HandleProxyDone(state, vss_get_status(state->incoming_reply));
                return;                
            }
            rc = VSSI_VerifyHeader(state->incoming_reply, state->session,
                                   VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                   VSS_NEGOTIATE_SIGN_TYPE_SHA1);
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Bad header signature on proxy response on socket "FMT_VPLSocket_t".",
                                  VAL_VPLSocket_t(state->conn_id));
                VSSI_HandleProxyDone(state, rc);
                return;
            }

            // Reallocate to receive body.
            tmp = realloc(state->incoming_reply,
                          VSS_HEADER_SIZE + 
                          vss_get_data_length(state->incoming_reply));
            if(tmp == NULL) {
                // Fail realloc. Try again later.
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to allocate buffer for proxy request reply.");
                return;
            }
            state->incoming_reply = (char*)tmp;

            // Set message length.
            state->replylen = VSS_HEADER_SIZE + 
                vss_get_data_length(state->incoming_reply);
        }
    }
    if(state->replylen > 0) {
        if(state->so_far < state->replylen) {
            // Receive body
            rc = VPLSocket_Recv(state->conn_id, state->incoming_reply + state->so_far, 
                                state->replylen - state->so_far);
            if(rc == 0) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Got socket closed for connection "FMT_VPLSocket_t".",
                                  VAL_VPLSocket_t(state->conn_id));
                VSSI_HandleProxyDone(state, VSSI_COMM);
                return;
            }
            else if(rc < 0) {
                // handle expected socket errors
                switch(rc) {
                case VPL_ERR_AGAIN:
                case VPL_ERR_INTR:
                    // These are OK. Try again later.
                    return;
                    break;
                    
                default:
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Got socket error %d for connection "FMT_VPLSocket_t".",
                                      rc, VAL_VPLSocket_t(state->conn_id));
                    VSSI_HandleProxyDone(state, VSSI_COMM);
                    return;
                    break;
                }
            }
            else {
                // Handle received bytes.
                state->so_far += rc;
            }
        }
        if(state->replylen == state->so_far) {
            rc = VSSI_VerifyData(state->incoming_reply, state->replylen,
                                 state->session,
                                 VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                 VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                                 VSS_NEGOTIATE_ENCRYPT_TYPE_AES128);
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Bad body signature on proxy response on socket "FMT_VPLSocket_t".",
                                  VAL_VPLSocket_t(state->conn_id));
                VSSI_HandleProxyDone(state, rc);
                return;
            }

            // If complete, proxy connection established.
            // If P2P connect requested, now try to complete P2P connection.
            data = state->incoming_reply + VSS_HEADER_SIZE;
            if(state->p2p_flags) {
                if(vss_proxy_request_reply_get_port(data) != 0) {
                    state->inaddr.addr = vss_proxy_request_reply_get_destination_ip(data);
                    state->inaddr.port = VPLNet_port_hton(vss_proxy_request_reply_get_port(data));
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "P2P after proxy to device "FMTu64" being attempted to "FMT_VPLNet_addr_t":%u.",
                                      state->destination_device,
                                      VAL_VPLNet_addr_t(state->inaddr.addr),
                                      VPLNet_port_ntoh(state->inaddr.port));
                }
                else {
                    // Give up on P2P. Other side not interested.
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "P2P after proxy not attempted. Device "FMTu64" not P2P aware.",
                                      state->destination_device);
                    state->p2p_flags = 0;
                }
            }

            // Assume proxy connection will be used.
            *(state->socket) = state->conn_id;
            state->conn_id = VPLSOCKET_INVALID; // Don't close.
            if(state->is_direct != NULL) {
                *(state->is_direct) = false;
            }

            if(!state->p2p_flags) {
                // Use proxy connection. Done.
                VSSI_HandleProxyDone(state, VSSI_SUCCESS);
            }
            else {
                // Need to try P2P connection.
                state->last_active = VPLTime_GetTimeStamp();
                state->inactive_timeout = VSSI_PROXY_P2P_CONN_TIMEOUT;
                state->p2p_flags |= VSSI_PROXY_DO_P2P;
                state->replylen = 0;
                state->so_far = 0;
                VSSI_ProxyToP2P(state);
            }
        }
    }
}

void VSSI_ProxyToP2P(VSSI_ConnectProxyState* state)
{
    int rc;
    int yes = 1;
    VPLSocket_addr_t origin_addr;

    origin_addr.addr = VPLSocket_GetAddr(*(state->socket));
    origin_addr.port = VPLSocket_GetPort(*(state->socket));
    
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Attempt to open P2P connection to "FMTu64" from "
                      FMT_VPLNet_addr_t":%u to "FMT_VPLNet_addr_t":%u",
                      state->destination_device,
                      VAL_VPLNet_addr_t(origin_addr.addr),
                      VPLNet_port_ntoh(origin_addr.port),
                      VAL_VPLNet_addr_t(state->inaddr.addr),
                      VPLNet_port_ntoh(state->inaddr.port));
    
    // Can't just reconnect_server() here.
    // Have to be more deliberate to get a P2P connection either by
    // connecting or by accepting a connection.
    
    // Open two sockets: one to listen and one to connect (punch firewalls)
    state->p2p_listen_id = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
    if(VPLSocket_Equal(state->p2p_listen_id, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open P2P socket.");
        goto exit;
    }    
    state->p2p_punch_id = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, true);
    if(VPLSocket_Equal(state->p2p_punch_id, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open P2P socket.");
        goto exit;
    }
    
#ifndef VPL_PLAT_IS_WINRT
    // Must use SO_REUSEADDR option for each. 
    rc = VPLSocket_SetSockOpt(state->p2p_listen_id, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                              (void*)&yes, sizeof(yes));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed (%d) to set SO_REUSEADDR for socket.",
                          rc);
        goto exit;
    }
    rc = VPLSocket_SetSockOpt(state->p2p_punch_id, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                              (void*)&yes, sizeof(yes));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed (%d) to set SO_REUSEADDR for socket.",
                          rc);
        goto exit;
    }
#endif
#ifdef IOS
    // Must use SO_REUSEPORT option for each.
    rc = VPLSocket_SetSockOpt(state->p2p_listen_id, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEPORT,
                              (void*)&yes, sizeof(yes));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed (%d) to set VPLSOCKET_SO_REUSEPORT for socket.",
                          rc);
        goto exit;
    }
    rc = VPLSocket_SetSockOpt(state->p2p_punch_id, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEPORT,
                              (void*)&yes, sizeof(yes));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed (%d) to set VPLSOCKET_SO_REUSEPORT for socket.",
                          rc);
        goto exit;
    }
#endif
    /* Set TCP no delay for performance reasons. */
    VPLSocket_SetSockOpt(state->p2p_listen_id, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                         (void*)&yes, sizeof(yes));
    VPLSocket_SetSockOpt(state->p2p_punch_id, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                         (void*)&yes, sizeof(yes));
    
    // Bind each to origin address
    rc = VPLSocket_Bind(state->p2p_listen_id, &origin_addr, sizeof(origin_addr));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to bind p2p socket to "FMT_VPLNet_addr_t":%u. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLNet_addr_t(origin_addr.addr),
                         VPLNet_port_ntoh(origin_addr.port),
                         VAL_VPLSocket_t(state->p2p_listen_id), rc);
        goto exit;
    }
    rc = VPLSocket_Bind(state->p2p_punch_id, &origin_addr, sizeof(origin_addr));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to bind p2p socket to "FMT_VPLNet_addr_t":%u. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLNet_addr_t(origin_addr.addr),
                         VPLNet_port_ntoh(origin_addr.port),
                         VAL_VPLSocket_t(state->p2p_punch_id), rc);
        goto exit;
    }
    
    // "Connect" to the server, punching the local firewall.
    rc = VPLSocket_ConnectNowait(state->p2p_punch_id, &state->inaddr, sizeof(state->inaddr));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Expected failure (%d) to connect to server "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                          rc,
                          VAL_VPLNet_addr_t(state->inaddr.addr),
                          VPLNet_port_ntoh(state->inaddr.port),
                          VAL_VPLSocket_t(state->p2p_punch_id));
        if(rc == VPL_ERR_BUSY || rc == VPL_ERR_AGAIN) {
            state->p2p_flags |= VSSI_PROXY_PUNCH_P2P;
        }
    }
    else {
        state->p2p_conn_id = state->p2p_punch_id;        
        state->p2p_flags = VSSI_PROXY_DO_P2P | VSSI_PROXY_CONN_P2P;
        state->p2p_punch_id = VPLSOCKET_INVALID;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Punch socket connected to server at "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                          VAL_VPLNet_addr_t(state->inaddr.addr),
                          VPLNet_port_ntoh(state->inaddr.port),
                          VAL_VPLSocket_t(state->p2p_conn_id));
        VSSI_ProxySendAuthP2P(state);
        goto exit;
    }
    
    // Listen on the socket.
    // expecting only a single connection
    rc = VPLSocket_Listen(state->p2p_listen_id, 1);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to listen on p2p socket: %d.", rc);
        goto exit;
    }
    
    state->p2p_flags |= VSSI_PROXY_LISTEN_P2P;

    VSSI_NotifySocketChangeFn();
    return;
 exit:
    VSSI_HandleProxyDone(state, VSSI_SUCCESS);
}

void VSSI_ProxyPunchP2P(VSSI_ConnectProxyState* state)
{
    // Check status of punch socket. 
    // "Accept" connection if succeeded.
    int so_err = 0;
    int rc;
    
    rc = VPLSocket_GetSockOpt(state->p2p_punch_id,
                              VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_ERROR,
                              &so_err, sizeof(so_err));
    if(rc != VPL_OK) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Get SO_ERROR for punch_sockfd failed:%d.",
                          rc); 
        so_err = -1;
    }
    
    if(so_err == 0) {
        state->p2p_conn_id = state->p2p_punch_id;        
        state->p2p_flags = VSSI_PROXY_DO_P2P | VSSI_PROXY_CONN_P2P;
        state->p2p_punch_id = VPLSOCKET_INVALID;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Punch socket connected to server at "FMT_VPLNet_addr_t":%d on socket "FMT_VPLSocket_t".",
                          VAL_VPLNet_addr_t(state->inaddr.addr),
                          VPLNet_port_ntoh(state->inaddr.port),
                          VAL_VPLSocket_t(state->p2p_conn_id));
        VSSI_ProxySendAuthP2P(state);
    }
    else {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Punch socket connected ends with error %d.",
                          so_err); 
        state->p2p_flags &= ~(VSSI_PROXY_PUNCH_P2P);
    }
    VSSI_NotifySocketChangeFn();
}

void VSSI_ProxyListenP2P(VSSI_ConnectProxyState* state)
{
    // Connection made. Accept it and move to connected state.
    VPLSocket_addr_t addr;
    int rc;

    rc = VPLSocket_Accept(state->p2p_listen_id, &addr, sizeof(addr),
                          &(state->p2p_conn_id));
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Client connection accept error. socket:"FMT_VPLSocket_t", error:%d", 
                         VAL_VPLSocket_t(state->p2p_listen_id), rc);
    }
    else {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Got connection from "FMT_VPLNet_addr_t":%u (Expected from "FMT_VPLNet_addr_t":%u) on socket "FMT_VPLSocket_t".",
                          VAL_VPLNet_addr_t(addr.addr), VPLNet_port_ntoh(addr.port),
                          VAL_VPLNet_addr_t(state->inaddr.addr), VPLNet_port_ntoh(state->inaddr.port),
                          VAL_VPLSocket_t(state->p2p_conn_id));
        state->last_active = VPLTime_GetTimeStamp();
        state->p2p_flags = VSSI_PROXY_DO_P2P | VSSI_PROXY_CONN_P2P;
        VSSI_ProxySendAuthP2P(state);
    }
    VSSI_NotifySocketChangeFn();
}

void VSSI_ProxySendAuthP2P(VSSI_ConnectProxyState* state)
{
    VSSI_MsgBuffer* msg = NULL;
    char header[VSS_HEADER_SIZE] = {0};
    char body[VSS_AUTHENTICATE_SIZE_EXT_VER_0] = {0};
    int rc;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Success opening P2P connection after proxy to device "FMTu64".",
                      state->destination_device);
    
    vss_set_command(header, VSS_AUTHENTICATE);
    vss_set_xid(header, 0);
    vss_authenticate_set_cluster_id(body, state->destination_device);
    
    msg = VSSI_PrepareMessage(header,
                              body, VSS_AUTHENTICATE_SIZE_EXT_VER_0, 
                              NULL, 0, NULL, 0);
    if(msg == NULL) {
        goto end;
    }
    
    msg = VSSI_ComposeMessage(state->session, 
                              VSS_NEGOTIATE_SIGNING_MODE_FULL,
                              VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                              VSS_NEGOTIATE_ENCRYPT_TYPE_AES128,
                              msg);
    if(msg == NULL) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed to allocate space for auth message on P2P connection to "FMTu64".",
                          state->destination_device);
        goto end;
    }

    // Send right away.
    rc = VSSI_Sendall(state->p2p_conn_id, msg->msg, msg->length);
    if(rc != msg->length) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Failed to send auth message on P2P connection to "FMTu64": %d.",
                          state->destination_device, rc);
        goto end;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Sent auth message on P2P connection to "FMTu64"",
                      state->destination_device);
    state->p2p_flags |= VSSI_PROXY_CONN_P2P;
    state->inactive_timeout = VSSI_PROXY_P2P_AUTH_TIMEOUT;
    state->last_active = VPLTime_GetTimeStamp();
    free(msg);

    return;
 end:
    if(msg) {
        free(msg);
    }
    // Immediately timeout.
    state->inactive_timeout = 0;
}

void VSSI_ProxyAuthP2P(VSSI_ConnectProxyState* state)
{
    int rc;

    if(state->incoming_reply == NULL) {
        // Allocate for header
        state->incoming_reply = (char*)calloc(VSS_HEADER_SIZE, 1);
        if(state->incoming_reply == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to allocate buffer for p2p auth request reply.");
            goto done;
        }
    }
    // Receive header as much as possible.
    if(state->so_far < VSS_HEADER_SIZE) {
        rc = VPLSocket_Recv(state->p2p_conn_id, state->incoming_reply + state->so_far, 
                            VSS_HEADER_SIZE - state->so_far);
        if(rc == 0) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Got socket closed for p2p connection "FMT_VPLSocket_t".",
                              VAL_VPLSocket_t(state->p2p_conn_id));
            goto done;
        }
        else if(rc < 0) {
            // handle expected socket errors
            switch(rc) {
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR:
                // These are OK. Try again later.
                goto exit;
                break;
                
            default:
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Got socket error %d for connection "FMT_VPLSocket_t".",
                                  rc, VAL_VPLSocket_t(state->conn_id));
                goto done;
                break;
            }
        }
        else {
            // Handle received bytes.
            state->so_far += rc;
        }
    }
    
    if(state->so_far == VSS_HEADER_SIZE) {
        rc = VSSI_VerifyHeader(state->incoming_reply, state->session,
                               VSS_NEGOTIATE_SIGNING_MODE_FULL,
                               VSS_NEGOTIATE_SIGN_TYPE_SHA1);
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Bad header signature on proxy response on socket "FMT_VPLSocket_t".",
                              VAL_VPLSocket_t(state->conn_id));
            goto done;
        }
        
        // There is no body to an auth response.
        
        if(vss_get_command(state->incoming_reply) != VSS_AUTHENTICATE_REPLY || 
           vss_get_status(state->incoming_reply) != VSSI_SUCCESS) {
            goto done;
        }
        
        // Successful authentication. Use P2P connection.
        state->conn_id = *(state->socket); // allow proxy to be closed
        *(state->socket) = state->p2p_conn_id;
        state->p2p_conn_id = VPLSOCKET_INVALID; // Don't close.
        if(state->is_direct != NULL) {
            *(state->is_direct) = true;
        }
        goto done;
    }

 exit:
    return;
 done:
    VSSI_HandleProxyDone(state, VSSI_SUCCESS);
}

VSSI_VSConnectionNode* VSSI_GetServerConnection(u64 cluster_id)
{
    VSSI_VSConnectionNode* rv = NULL;

    VPLMutex_Lock(&vssi_mutex);
    
    for(rv = vs_connection_list_head; rv != NULL; rv = rv->next) {
        if(rv->cluster_id == cluster_id &&
           rv->connect_state != CONNECTION_FAILED) {
            // found it
            break;
        }
    }

    if(rv == NULL) {
        int rc;

        // Need to make one.
        rv = (VSSI_VSConnectionNode*)calloc(sizeof(VSSI_VSConnectionNode), 1);
        if(rv == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed memory allocation on creating server connection node.");
            goto exit;
        }

        rc = VPLMutex_Init(&(rv->send_mutex));
        if(rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed mutex init (%d) for server connection.",
                             rc);
            goto dealloc;
        }

        rv->cluster_id = cluster_id;
        rv->connect_state = CONNECTION_DISCONNECTED;
        

        // Make one sub-connection for starters.
        rv->connections = (VSSI_VSConnection*)calloc(sizeof(VSSI_VSConnection), 1);
        if(rv->connections == NULL)  {
            goto dealloc;
        }

        rv->connections->conn_id = VPLSOCKET_INVALID;
        rv->connections->last_active = VPLTime_GetTimeStamp();
        rv->connections->inactive_timeout = VSSI_VS_CONNECTION_TIMEOUT;

        // Add to the connections list.
        rv->next = vs_connection_list_head;
        vs_connection_list_head = rv;
        
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Server connection struct made to server-cluster "FMTu64".",
                          rv->cluster_id);
    }

    rv->refcount++;

    goto exit;
 dealloc:
    free(rv);
    rv = NULL;
 exit:
    VPLMutex_Unlock(&vssi_mutex);

    return rv;
}

void VSSI_ReleaseServerConnection(VSSI_VSConnectionNode* connection)
{
    if(connection) {
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Release connection to "FMT_VPLNet_addr_t":%d.",
                            VAL_VPLNet_addr_t(connection->inaddr.addr),
                            VPLNet_port_ntoh(connection->inaddr.port));

        VPLMutex_Lock(&vssi_mutex);
        connection->refcount--;
        VPLMutex_Unlock(&vssi_mutex);
    }
}

void VSSI_DeleteServerConnection(VSSI_VSConnectionNode* connection)
{
    // Remove from list and delete.
    if(vs_connection_list_head == connection) {
        vs_connection_list_head = connection->next;
    }
    else {
        VSSI_VSConnectionNode* prev = vs_connection_list_head;
        while(prev->next != connection) {
            prev = prev->next;
        }
        prev->next = connection->next;
    }
    
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Destroy cluster "FMTu64" connection to "FMT_VPLNet_addr_t":%d.",
                      connection->cluster_id,
                      VAL_VPLNet_addr_t(connection->inaddr.addr),
                      VPLNet_port_ntoh(connection->inaddr.port));

    VPLMutex_Lock(&(connection->send_mutex));

    while(connection->connections) {
        VSSI_VSConnection* conn = connection->connections;
        connection->connections = conn->next;

        if(!VPLSocket_Equal(conn->conn_id, VPLSOCKET_INVALID)) {
            VPLSocket_Close(conn->conn_id);
            VSSI_NotifySocketChangeFn();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Closed socket "FMT_VPLSocket_t" connected to "FMT_VPLNet_addr_t":%d.",
                              VAL_VPLSocket_t(conn->conn_id),
                              VAL_VPLNet_addr_t(connection->inaddr.addr),
                              VPLNet_port_ntoh(connection->inaddr.port));
            conn->conn_id = VPLSOCKET_INVALID;
        }

        free(conn->incoming_reply);
        free(conn->send_msg);
        free(conn);
    }

    VPLMutex_Unlock(&(connection->send_mutex));
    VPLMutex_Destroy(&(connection->send_mutex));

    free(connection);
}

static VSSI_Result VSSI_ExpandCommandSlots(VSSI_SendState* send_state)
{
    VSSI_Result rv = VSSI_SUCCESS;

    // If all slots are full, increase the table.
    if(send_state->num_cmd_slots_active == send_state->num_cmd_slots) {
        void* tmp;
        VSSI_PendInfo** new_chunks;
        int i;

        VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                           "Reallocating pending commands table. Was %d slots, now %d.",
                           send_state->num_cmd_slots,
                           send_state->num_cmd_slots + PEND_CMD_GROW_CNT);

        tmp = realloc(send_state->pending_cmds,
                      sizeof(VSSI_PendInfo*) *
                      (send_state->num_cmd_slots + PEND_CMD_GROW_CNT +
                       send_state->num_chunks + 1));
        if(tmp == NULL) {
            // Failed to expand pending commands table.
            rv = VSSI_CMDLIMIT;
            goto fail;
        }
        send_state->pending_cmds = tmp;
        send_state->cmd_chunks = (VSSI_PendInfo**)&(send_state->pending_cmds[send_state->num_cmd_slots]);
        new_chunks = (VSSI_PendInfo**)&(send_state->pending_cmds[send_state->num_cmd_slots + PEND_CMD_GROW_CNT]);

        // Allocate new command chunk at new chunk slot.
        // Chunk slots come after pending command slots.
        new_chunks[send_state->num_chunks] =
            (VSSI_PendInfo*)calloc(sizeof(VSSI_PendInfo),
                                   PEND_CMD_GROW_CNT);
        if(new_chunks[send_state->num_chunks] == NULL) {
            // Failed to expand pending commands table.
            rv = VSSI_CMDLIMIT;
            goto fail;
        }

        // Move previous chunk pointers to end of allocated space.
        for(i = send_state->num_chunks; i > 0; i--) {
            new_chunks[i - 1] = send_state->cmd_chunks[i - 1];
        }
        send_state->cmd_chunks = new_chunks;

        // Clear new slots
        memset(&(send_state->pending_cmds[send_state->num_cmd_slots]), 0,
               sizeof(VSSI_PendInfo*) * PEND_CMD_GROW_CNT);

        // Put new info structs onto free stack.
        for(i = 0; i < PEND_CMD_GROW_CNT; i++) {
            send_state->cmd_chunks[send_state->num_chunks][i].next_free =
                send_state->free_stack;
            send_state->free_stack = &(send_state->cmd_chunks[send_state->num_chunks][i]);
        }

        // Set new table size
        send_state->num_cmd_slots += PEND_CMD_GROW_CNT;
        send_state->num_chunks++;
    }

 fail:
    return rv;
}

VSSI_Result VSSI_AddPendingCommand(VSSI_SendState* send_state,
                                   const VSSI_PendInfo* cmd_data,
                                   u32* xid_out)
{
    VSSI_Result rv = VSSI_SUCCESS;
    int slot;
    
    rv = VSSI_ExpandCommandSlots(send_state);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    // Select an XID for this command.
    *xid_out = send_state->seq_id++;
    // Do not reuse XID 0 after the first command.
    if(send_state->seq_id == 0) {
        send_state->seq_id++;
    }

    // Try to place at xid % num slots.
    // If that slot is taken, take the next open slot.
    // Will always find a slot thanks to expansion above.
    slot = *xid_out % send_state->num_cmd_slots;
    while(send_state->pending_cmds[slot] != NULL) {
        slot = (slot + 1) % send_state->num_cmd_slots;
    }
    send_state->pending_cmds[slot] = send_state->free_stack;
    send_state->free_stack = send_state->pending_cmds[slot]->next_free;
    memcpy(send_state->pending_cmds[slot], cmd_data, sizeof(VSSI_PendInfo));
    send_state->pending_cmds[slot]->xid = *xid_out;
    send_state->num_cmd_slots_active++;

 fail:
    return rv;
}

VSSI_Result VSSI_AddWaitingCommand(VSSI_SendState* send_state,
                                   const VSSI_PendInfo* cmd_data)
{
    VSSI_Result rv = VSSI_SUCCESS;
    VSSI_PendInfo* new_info = NULL;

    rv = VSSI_ExpandCommandSlots(send_state);
    if(rv != VSSI_SUCCESS) {
        goto fail;
    }

    new_info = send_state->free_stack;
    send_state->free_stack = new_info->next_free;
    send_state->num_cmd_slots_active++;

    memcpy(new_info, cmd_data, sizeof(VSSI_PendInfo));

    new_info->next = NULL;
    if(send_state->wait_head == NULL) {
        send_state->wait_head = new_info;
    }
    else {
        send_state->wait_tail->next = new_info;
    }
    send_state->wait_tail = new_info;

 fail:
    return rv;
}

VSSI_Result VSSI_WaitingCommandToPending(VSSI_SendState* send_state,
                                         VSSI_PendInfo* cmd_data,
                                         u32* xid_out)
{
    VSSI_Result rv = VSSI_SUCCESS;
    int slot;
    
    // Select an XID for this command.
    *xid_out = send_state->seq_id++;
    // Do not reuse XID 0 after the first command.
    if(send_state->seq_id == 0) {
        send_state->seq_id++;
    }

    // Try to place at xid % num slots.
    // If that slot is taken, take the next open slot.
    // Will always find a slot since the info struct is from the table in the first place.
    slot = *xid_out % send_state->num_cmd_slots;
    while(send_state->pending_cmds[slot] != NULL) {
        slot = (slot + 1) % send_state->num_cmd_slots;
    }
    send_state->pending_cmds[slot] = cmd_data;
    send_state->pending_cmds[slot]->xid = *xid_out;

    return rv;
}

int VSSI_GetCommandSlotUnlocked(VSSI_SendState* send_state, u32 xid)
{
    int start_pos;
    int rv;

    if(send_state->num_cmd_slots == 0) {
        // No table? Can't find anything.
        rv = -1;
        goto exit;
    }

    start_pos = xid % send_state->num_cmd_slots;

    if(send_state->pending_cmds[start_pos] != NULL &&
       send_state->pending_cmds[start_pos]->xid == xid) {
        // Found it in correct position.
        rv = start_pos;
    }
    else {
        for(rv = (start_pos + 1) % send_state->num_cmd_slots;
            rv != start_pos;
            rv = (rv + 1) % send_state->num_cmd_slots) {
            if(send_state->pending_cmds[rv] != NULL &&
               send_state->pending_cmds[rv]->xid == xid) {
                break;
            }
        }
        if(rv == start_pos) {
            // No such pending command. Done.
            rv = -1;
        }
    }

 exit:
    return rv;
}

VSSI_PendInfo* VSSI_GetVSCommandSlot(VSSI_SendState* send_state, u32 xid)
{
    // Find slot with matching XID.
    // Start at xid % num slots, and go forward from there.
    // Return error (-1) if no match is found.
    int slot;
    VSSI_PendInfo* rv = NULL;

    VPLMutex_Lock(&(send_state->send_mutex));
    slot = VSSI_GetCommandSlotUnlocked(send_state, xid);

    if(slot >= 0) {
        rv = send_state->pending_cmds[slot];
    }
    VPLMutex_Unlock(&(send_state->send_mutex));

    return rv;
}

int VSSI_ResolveCommand(VSSI_PendInfo* cmd_info,
                        VSSI_Result result)
{
    VSSI_ObjectState* object = NULL;
    VSSI_FileState* filestate = NULL;
    int rv = 0;

    //
    // For the case of OPEN FILE, need to clean up the file_state
    // structure that was allocated in the failure case.
    //
    switch(cmd_info->delete_filestate_when) {
    case DELETE_NEVER:
    default:
        break;
    case DELETE_ON_FAIL:
        if(result != VSSI_SUCCESS && result != VSSI_EXISTS) {
            filestate = cmd_info->file_state;
        }
        break;
    case DELETE_ALWAYS:
        filestate = cmd_info->file_state;
        break;
    }

    if(filestate) {
        VSSI_FreeFileState(filestate);
    }

    // If indicated, free specified object (after releasing mutex)
    switch(cmd_info->delete_obj_when) {
    case DELETE_NEVER:
    default:
        break;
    case DELETE_ON_FAIL:
        if(result != VSSI_SUCCESS && result != VSSI_EXISTS) {
            object = cmd_info->object;
        }
        break;
    case DELETE_ALWAYS:
        object = cmd_info->object;
        break;
    }

    if(object) {
        VSSI_FreeObject(object);
    }

    if(cmd_info->callback) {
        (cmd_info->callback)(cmd_info->ctx, result);
    }

    return rv;
}

void VSSI_ReplyPendingCommands(VSSI_SendState* send_state,
                               VSSI_VSConnection* connection,
                               VSSI_Result result)
{
    int i;
    VSSI_PendInfo* cmd;
    int negotiate_fail = 0;

    VPLMutex_Lock(&(send_state->send_mutex));

    for(i = 0; i < send_state->num_cmd_slots; i++) {
        // If pending, call the callback with context and result.
        if(send_state->pending_cmds[i] != NULL &&
           (connection == NULL ||
            send_state->pending_cmds[i]->connection == connection)) {
            cmd = send_state->pending_cmds[i];
            send_state->pending_cmds[i] = NULL;
            
            VPLMutex_Unlock(&(send_state->send_mutex));
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Clearing command %s callback, xid %u.",
                              cmd->callback ? "with" : "without",
                              cmd->xid);
            if(cmd->command == VSS_NEGOTIATE) {
                negotiate_fail = true;
            }
            else {
                VSSI_ResolveCommand(cmd, result);
            }
            VPLMutex_Lock(&(send_state->send_mutex));

            send_state->num_cmd_slots_active--;
            free(cmd->msg);
            memset(cmd, 0, sizeof(VSSI_PendInfo));
            cmd->next_free = send_state->free_stack;
            send_state->free_stack = cmd;
        }
    }

    if(connection == NULL || negotiate_fail) {
        // Also reply to commands waiting for session negotiation.
        while(send_state->wait_head != NULL) {
            cmd = send_state->wait_head;
            send_state->wait_head = cmd->next;
            
            VPLMutex_Unlock(&(send_state->send_mutex));
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Clearing command %s callback from wait queue.",
                              cmd->callback ? "with" : "without");
            VSSI_ResolveCommand(cmd, result);
            VPLMutex_Lock(&(send_state->send_mutex));
            send_state->num_cmd_slots_active--;
            free(cmd->msg);
            memset(cmd, 0, sizeof(VSSI_PendInfo));
            cmd->next_free = send_state->free_stack;
            send_state->free_stack = cmd;
        }
    }

    VPLMutex_Unlock(&(send_state->send_mutex));
}

VSSI_SendState* VSSI_GetSendState(VSSI_UserSession* user_session,
                                  u64 cluster_id)
{
    VSSI_SendState* rv;

    VPLMutex_Lock(&vssi_mutex);

    rv = user_session->send_states;
    while(rv != NULL && rv->cluster_id != cluster_id) {
        rv = rv->next;
    }
    if(rv == NULL) {
        rv = (VSSI_SendState*)calloc(sizeof(VSSI_SendState), 1);
        if(rv == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to allocate send context.");
            goto exit;
        }

        VPLMutex_Init(&(rv->send_mutex));

        rv->sign_mode = VSS_NEGOTIATE_SIGNING_MODE_FULL;
        rv->sign_type = VSS_NEGOTIATE_SIGN_TYPE_SHA1;
        rv->enc_type = VSS_NEGOTIATE_ENCRYPT_TYPE_AES128;
        rv->negotiated = NEGOTIATE_NEEDED;

        rv->cluster_id = cluster_id;
        rv->next = user_session->send_states;
        rv->session = user_session;
        user_session->send_states = rv;

        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Adding send state %p to session "FMTu64" for cluster "FMTu64".",
                          rv, user_session->handle, cluster_id);
    }

 exit:
    VPLMutex_Unlock(&vssi_mutex);

    return rv;
}

void VSSI_DestroySendState(VSSI_SendState* send_state)
{
    VPLMutex_Lock(&vssi_mutex);
    VSSI_DestroySendStateUnlocked(send_state);
    VPLMutex_Unlock(&vssi_mutex);
}

void VSSI_DestroySendStateUnlocked(VSSI_SendState* send_state)
{
    int i;
    VSSI_UserSession* user_session = send_state->session;
    // Find, remove from session.
    if(user_session->send_states == send_state) {
        user_session->send_states = send_state->next;
    }
    else {
        VSSI_SendState* prev_send_state = user_session->send_states;
        while(prev_send_state != NULL &&
              prev_send_state->next != send_state) {
            prev_send_state = prev_send_state->next;
        }
        if(prev_send_state) {
            prev_send_state->next = send_state->next;
        }
    }

    // Release connection reference so it might be destroyed after disconnect.
    if(send_state->server_connection) {
        VSSI_ReleaseServerConnection(send_state->server_connection);
    }

    // Destroy.
    VPLMutex_Lock(&(send_state->send_mutex));

    for(i = 0; i < send_state->num_chunks; i++) {
        free(send_state->cmd_chunks[i]);
    }
    if(send_state->pending_cmds) {
        free(send_state->pending_cmds);
    }

    VPLMutex_Unlock(&(send_state->send_mutex));
    VPLMutex_Destroy(&(send_state->send_mutex));

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Removed send state %p from session "FMTu64" for cluster "FMTu64".",
                      send_state, user_session->handle, send_state->cluster_id);
    free(send_state);    
}

void VSSI_HandleLostConnection(VSSI_VSConnectionNode* conn_node,
                               VSSI_VSConnection* connection,
                               VSSI_Result result)
{
    /// All pending commands (for all users) return @a result immediately.
    /// Pending commands are determined by XID.
    /// Connection ID is set to invalid.
    VSSI_SessionNode* session;

    // remove connection from conn_node so it can be deleted later.
    if(connection != NULL) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Closing socket "FMT_VPLSocket_t" to cluster "FMTu64".",
                          VAL_VPLSocket__t(connection->conn_id),
                          conn_node->cluster_id);

        if(conn_node->connections == connection) {
            conn_node->connections = connection->next;
        }
        else {
            VSSI_VSConnection* tmp;
            tmp = conn_node->connections;
            while(tmp != NULL && tmp->next != connection) {
                tmp = tmp->next;
            }
            if(tmp != NULL) {
                tmp->next = connection->next;
            }
        }

        // If connection was in-progress, it has failed.
        if(conn_node->connect_state == CONNECTION_CONNECTED) {
            VSSI_ConnectVSServerDone(conn_node, result);
        }
        // If no connections remain, server connection failed.
        // All associated send states must reset negotiation state.
        if(conn_node->connections == NULL) {
            conn_node->connect_state = CONNECTION_FAILED;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Cluster "FMTu64" connection lost.",
                              conn_node->cluster_id);
        }

        // Close socket.
        if(!VPLSocket_Equal(VPLSOCKET_INVALID, connection->conn_id)) {
            VPLSocket_Close(connection->conn_id);
            connection->conn_id = VPLSOCKET_INVALID;
        }
        // Free possible buffers
        free(connection->incoming_reply);
        free(connection->send_msg);
        // Delete connection
        free(connection);
    }

    VPLMutex_Lock(&vssi_mutex);

    // Find all send-states using this server.
    for(session = user_session_list_head; session != NULL; session = session->next) {
        VSSI_SendState* send_state = session->session.send_states;
        VSSI_SendState* prev_send_state = NULL;
        VSSI_ObjectState* object;

        // Find objects referring to this connection for notifications.
        // Notifications are implicitly canceled.
        // Call notify callback one last time saying so.
        for(object = session->session.objects; object != NULL; object = object->next) {
            if(object->notify_connection == connection) {
                object->notify_connection = NULL;
                if(object->notify_callback) {
                    (object->notify_callback)(object->notify_ctx, 
                                              VSSI_NOTIFY_DISCONNECTED_EVENT,
                                              NULL, 0);
                }
            }
        }
                
        while(send_state != NULL && send_state->server_connection != conn_node) {
            prev_send_state = send_state;
            send_state = send_state->next;
        }

        if(send_state) {
            // Reset negotiation state if send state lost.
            if(conn_node->connect_state == CONNECTION_FAILED) {
                send_state->sign_mode = VSS_NEGOTIATE_SIGNING_MODE_FULL;
                send_state->sign_type = VSS_NEGOTIATE_SIGN_TYPE_SHA1;
                send_state->enc_type = VSS_NEGOTIATE_ENCRYPT_TYPE_AES128;
                send_state->negotiated = NEGOTIATE_NEEDED;
            }

            // Reply all commands sent on the broken connection.
            VSSI_ReplyPendingCommands(send_state, connection, result);

            // If server connection lost last connection, destroy send state, too.
            if(conn_node->connections == NULL || 
               conn_node->connect_state == CONNECTION_FAILED) {
                // Reply to all commands that are left on this send_state
                VSSI_ReplyPendingCommands(send_state, NULL, result);
                if(prev_send_state == NULL) {
                    session->session.send_states = send_state->next;
                }
                else {
                    prev_send_state = send_state->next;
                }
                VSSI_DestroySendStateUnlocked(send_state);
            }
        }        
    }
    
    VPLMutex_Unlock(&vssi_mutex);
    
    VSSI_NotifySocketChangeFn();
}

/// Read in whatever data is ready for this connection's socket.
size_t VSSI_ReceiveVSBuffer(VSSI_VSConnection* connection,
                            void** buf_out)
{
    size_t rv = 0;
    *buf_out = NULL;

    if(connection->replylen == 0) {
        if(connection->incoming_reply == NULL) {
            connection->incoming_reply = (char*)calloc(VSS_HEADER_SIZE, 1);
            if(connection->incoming_reply == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed allocation for incoming response.");
                goto exit;
            }
        }

        rv = VSS_HEADER_SIZE - connection->replylen_recvd;
        *buf_out = (connection->incoming_reply + connection->replylen_recvd);
    }
    else if(connection->replylen_recvd == VSS_HEADER_SIZE) {
        if(connection->reply_bufsize < connection->replylen) {
            // Increase response buffer as needed.
            void* tmp = realloc(connection->incoming_reply, connection->replylen);

            if(tmp == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed memory allocation on reading reply.");
                goto exit;
            }
            connection->incoming_reply = (char*)tmp;
            connection->reply_bufsize = connection->replylen;
        }
        rv = connection->replylen - connection->replylen_recvd;
        *buf_out = connection->incoming_reply + connection->replylen_recvd;
    }
    else {
        rv = connection->replylen - connection->replylen_recvd;
        *buf_out = connection->incoming_reply + connection->replylen_recvd;
    }

 exit:
    return rv;
}

// Return 0: OK to continue receiving.
// Return 1: Stop receiving.
// Return -1: Stop receiving and handle socket disconnect.
int VSSI_HandleVSBuffer(VSSI_VSConnectionNode* conn_node,
                        VSSI_VSConnection* connection,
                        int bufsize)
{
    int rc;
    int rv = 0;

    connection->replylen_recvd += bufsize;

    if(connection->replylen_recvd < VSS_HEADER_SIZE) {
        // Do nothing yet.
        return 0;
    }
    else if(connection->replylen_recvd == VSS_HEADER_SIZE) {
        // Find security settings from send state used.
        VSSI_SessionNode* session = user_session_list_head;
        
        while(session != NULL && 
              session->session.handle != vss_get_handle(connection->incoming_reply)) {
            session = session->next;
        }
        if(session == NULL) {
            // Invalid header for unknown session.
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Got header with unknown session "FMTu64" on connection "FMT_VPLSocket_t".",
                             vss_get_handle(connection->incoming_reply),
                             VAL_VPLSocket_t(connection->conn_id));
            rv = -1;
            goto exit;
        }
        connection->cur_session = &(session->session);
        
        connection->cur_send_state = VSSI_GetSendState(connection->cur_session, conn_node->cluster_id);
        if(connection->cur_send_state == NULL) {
            // Invalid header for unknown send state.
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Got header with session "FMTu64" cluste "FMTu64" and no send state on connection "FMT_VPLSocket_t".",
                             vss_get_handle(connection->incoming_reply),
                             conn_node->cluster_id,
                             VAL_VPLSocket_t(connection->conn_id));
            rv = -1;
            goto exit;
        }

        if(vss_get_command(connection->incoming_reply) == VSS_ERROR) {
            connection->replylen = VSS_HEADER_SIZE;
        }
        else {
            rc = VSSI_VerifyHeader(connection->incoming_reply, connection->cur_session,
                                   connection->cur_send_state->sign_mode, 
                                   connection->cur_send_state->sign_type);
            if(rc == VSSI_SUCCESS) {
                // Set expected reply length.
                connection->replylen = VSS_HEADER_SIZE +
                    vss_get_data_length(connection->incoming_reply);
            }
            else {
                // Invalid header.
                // Drop the connection due to compromised data stream.
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Got invalid header (%d) on connection "FMT_VPLSocket_t".",
                                 rc, VAL_VPLSocket_t(connection->conn_id));
                rv = -1;
                goto exit;
            }
        }
    }
    
    if(connection->replylen == connection->replylen_recvd) {
        int slot;
        VSSI_PendInfo* cmd_info;

        if(connection->replylen > VSS_HEADER_SIZE) {
            // Verify the data.
            rc = VSSI_VerifyData(connection->incoming_reply, connection->replylen,
                                 connection->cur_session,
                                 connection->cur_send_state->sign_mode,
                                 connection->cur_send_state->sign_type,
                                 connection->cur_send_state->enc_type);
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Incoming reply type %x has bogus data signature.",
                                 vss_get_command(connection->incoming_reply));
                rv = -1;
                goto reply_done;
            }
        }

        // Handle requests that don't have a current command.
        if(conn_node->connect_state == CONNECTION_CONNECTED) {
            // Reply must be an AUTHENTICATE response.
            VSSI_ConnectVSServerDone(conn_node, vss_get_status(connection->incoming_reply));
            goto reply_done;

        }
        if(connection->cur_send_state->negotiated == NEGOTIATE_PENDING) {
            // Reply must be to NEGOTIATE for this send state session.
            VSSI_NegotiateSessionDone(connection->cur_session,
                                      connection->cur_send_state,
                                      conn_node,
                                      connection->incoming_reply);
            goto reply_done;
        }
        if(vss_get_command(connection->incoming_reply) == VSS_NOTIFICATION) {  
            // SPECIAL: Call notification callback for object referenced in body.
            // Object will refer to this connection (in case same handle came from multiple servers).
            VSSI_ObjectState* object;
            char* reply_data = connection->incoming_reply + VSS_HEADER_SIZE;
            for(object = connection->cur_session->objects; object != NULL; object = object->next) {
                if(object->access.vs.proto_handle == vss_notification_get_handle(reply_data) &&
                   object->notify_connection == connection) {
                    break;
                }
            }
            if(object && object->notify_callback) {
                (object->notify_callback)(object->notify_ctx, 
                                          vss_notification_get_mask(reply_data),
                                          vss_notification_get_data(reply_data),
                                          vss_notification_get_length(reply_data));
            }
            // Balance cmds_outstanding count, since notification is
            // spontaneous from the server side, not a reply to a
            // previous command.
            connection->cmds_outstanding++;
            goto reply_done;
        }

        // Find send state to handle response.
        cmd_info = VSSI_GetVSCommandSlot(connection->cur_send_state,
                                         vss_get_xid(connection->incoming_reply));
        if(cmd_info == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Incoming reply type %x xid %d for session "FMTu64" server "FMTu64" not expected.",
                             vss_get_command(connection->incoming_reply),
                             vss_get_xid(connection->incoming_reply),
                             vss_get_handle(connection->incoming_reply),
                             conn_node->cluster_id);
            goto reply_done;
        }

        switch(vss_get_command(connection->incoming_reply)) {
        case VSS_ERROR:
            rc = VSSI_HandleErrorReply(connection->incoming_reply);
            break;
            
        case VSS_OPEN_REPLY:
            rc = VSSI_HandleOpenReply(connection->incoming_reply,
                                      cmd_info->object);
            // Set object handle on success.
            if(rc == VSSI_SUCCESS) {
                *(cmd_info->handle) = (VSSI_Object)(cmd_info->object);
            }
            break;
            
        case VSS_CLOSE_REPLY:
            rc = VSSI_HandleCloseReply(connection->incoming_reply,
                                       cmd_info->object);
            break;
            
            // Status-only responses
        case VSS_START_SET_REPLY:
        case VSS_COMMIT_REPLY:
        case VSS_ERASE_REPLY:
        case VSS_MAKE_REF_REPLY:
        case VSS_SET_METADATA_REPLY:
        case VSS_SET_SIZE_REPLY:
        case VSS_SET_TIMES_REPLY:
        case VSS_REMOVE_REPLY:
        case VSS_RENAME_REPLY:
        case VSS_RENAME2_REPLY:
        case VSS_COPY_REPLY:
        case VSS_COPY_MATCH_REPLY:
        case VSS_EMPTY_TRASH_REPLY:
        case VSS_DELETE_TRASH_REPLY:
        case VSS_RESTORE_TRASH_REPLY:
        case VSS_MAKE_DIR_REPLY:
        case VSS_MAKE_DIR2_REPLY:
        case VSS_CHMOD_REPLY:
        case VSS_RELEASE_FILE_REPLY:
            rc = VSSI_HandleVersionReply(connection->incoming_reply,
                                         cmd_info->object);
            break;
            
        case VSS_DELETE_REPLY:
            rc = VSSI_HandleDeleteReply(connection->incoming_reply);
            break;
            
        case VSS_READ_REPLY:
            rc = VSSI_HandleReadReply(connection->incoming_reply,
                                      cmd_info->object,
                                      cmd_info->length,
                                      cmd_info->buf);
            break;
            
        case VSS_READ_TRASH_REPLY:
            rc = VSSI_HandleReadReply(connection->incoming_reply,
                                      cmd_info->object,
                                      cmd_info->length,
                                      cmd_info->buf);
            break;
            
        case VSS_WRITE_REPLY:
            rc = VSSI_HandleWriteReply(connection->incoming_reply,
                                       cmd_info->object,
                                       cmd_info->length);
            break;
            
        case VSS_READ_DIR_REPLY:
        case VSS_READ_TRASH_DIR_REPLY:
            rc = VSSI_HandleReadDirReply(connection->incoming_reply,
                                         cmd_info->object,
                                         cmd_info->directory, 1);
            break;
            
        case VSS_READ_DIR2_REPLY:
            rc = VSSI_HandleReadDirReply(connection->incoming_reply,
                                         cmd_info->object,
                                         cmd_info->directory, 2);
            break;
            
        case VSS_STAT_REPLY:
        case VSS_STAT_TRASH_REPLY:
            rc = VSSI_HandleStatReply(connection->incoming_reply,
                                      cmd_info->object,
                                      cmd_info->stats);
            break;
            
        case VSS_STAT2_REPLY:
            rc = VSSI_HandleStat2Reply(connection->incoming_reply,
                                       cmd_info->object,
                                       cmd_info->stats2);
            break;
            
        case VSS_READ_TRASHCAN_REPLY:
            rc = VSSI_HandleReadTrashcanReply(connection->incoming_reply,
                                              cmd_info->object,
                                              cmd_info->trashcan);
            break;
            
        case VSS_GET_SPACE_REPLY:
            rc = VSSI_HandleGetSpaceReply(connection->incoming_reply,
                                          cmd_info->object,
                                          cmd_info->disk_size,
                                          cmd_info->dataset_size,
                                          cmd_info->avail_size);
            break;

        case VSS_OPEN_FILE_REPLY:
            rc = VSSI_HandleOpenFileReply(connection->incoming_reply,
                                          cmd_info->file_state);
            // Return client file handle on success.
            // Note that VSSI_EXISTS is a special case only returned
            // by CREATE_ALWAYS mode when the file pre-exists.  It
            // is still successful, but conveys more information to
            // the caller.
            if(rc == VSSI_SUCCESS || rc == VSSI_EXISTS) {
                *(cmd_info->file) = (VSSI_File)(cmd_info->file_state);
            }
            break;
                    
        case VSS_CLOSE_FILE_REPLY:
            rc = VSSI_HandleCloseFileReply(connection->incoming_reply,
                                           cmd_info->object,
                                           &(cmd_info->delete_filestate_when));
            break;
            
        case VSS_READ_FILE_REPLY:
            rc = VSSI_HandleReadFileReply(connection->incoming_reply,
                                          cmd_info->length,
                                          cmd_info->buf);
            break;
            
        case VSS_WRITE_FILE_REPLY:
            rc = VSSI_HandleWriteFileReply(connection->incoming_reply,
                                           cmd_info->length);
            break;
            
        case VSS_TRUNCATE_FILE_REPLY:
            rc = VSSI_HandleTruncateFileReply(connection->incoming_reply);
            break;
            
        case VSS_CHMOD_FILE_REPLY:
            rc = VSSI_HandleChmodFileReply(connection->incoming_reply);
            break;
            
        case VSS_SET_LOCK_REPLY:
            rc = VSSI_HandleSetFileLockReply(connection->incoming_reply);
            break;
            
        case VSS_GET_LOCK_REPLY:
            rc = VSSI_HandleGetFileLockReply(connection->incoming_reply,
                                             cmd_info->lock_state);
            break;
            
        case VSS_SET_LOCK_RANGE_REPLY:
            rc = VSSI_HandleSetLockRangeReply(connection->incoming_reply);
            break;
            
        case VSS_GET_NOTIFY_REPLY: {
            VSSI_NotifyMask mask = 0;
            rc = VSSI_HandleGetNotifyReply(connection->incoming_reply, &mask);

            if(mask == 0) {
                // No events will end notifications. Stop holding connection open.
                if(cmd_info->object->notify_connection != NULL) {
                    cmd_info->object->notify_callback = NULL;
                    cmd_info->object->notify_ctx = NULL;
                    cmd_info->object->notify_connection->timeout_suspended--;
                    cmd_info->object->notify_connection = NULL;
                }
            }
            // else notification callback and related connection must be already set.

            if(cmd_info->notify_mask) {
                *(cmd_info->notify_mask) = mask;
            }

        } break;

        case VSS_SET_NOTIFY_REPLY: {
            VSSI_NotifyMask mask = 0;
            
            rc = VSSI_HandleGetNotifyReply(connection->incoming_reply, &mask);

            if(cmd_info->notify_mask == 0) {
                // No events will end notifications. Stop holding connection open.
                if(cmd_info->object->notify_connection != NULL) {
                    cmd_info->object->notify_callback = NULL;
                    cmd_info->object->notify_ctx = NULL;
                    cmd_info->object->notify_connection->timeout_suspended--;
                    cmd_info->object->notify_connection = NULL;
                }
            }
            else {
                // At least one event enabled.
                // Set callback and link object to connection.
                // Keep connection from timing out.
                cmd_info->object->notify_callback = cmd_info->notify_callback;
                cmd_info->object->notify_ctx = cmd_info->notify_ctx;
                if(cmd_info->object->notify_connection != NULL) {
                    cmd_info->object->notify_connection->timeout_suspended--;
                }
                cmd_info->object->notify_connection = connection;                
                connection->timeout_suspended++;
            }
        } break;

        default:
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Received reply of unknown type %u.",
                             vss_get_command(connection->incoming_reply));
            rc = VSSI_INVALID;
            break;
        }
        
        // Done with this command.
        VPLMutex_Lock(&(connection->cur_send_state->send_mutex));
        slot = VSSI_GetCommandSlotUnlocked(connection->cur_send_state,
                                           vss_get_xid(connection->incoming_reply));
        connection->cur_send_state->pending_cmds[slot] = NULL;
        VPLMutex_Unlock(&(connection->cur_send_state->send_mutex));            
        
        VSSI_ResolveCommand(cmd_info, rc);
        
        VPLMutex_Lock(&(connection->cur_send_state->send_mutex));
        connection->cur_send_state->num_cmd_slots_active--;
        memset(cmd_info, 0, sizeof(VSSI_PendInfo));
        cmd_info->next_free = connection->cur_send_state->free_stack;
        connection->cur_send_state->free_stack = cmd_info;
        VPLMutex_Unlock(&(connection->cur_send_state->send_mutex));            

    reply_done:
        connection->replylen = 0;
        connection->replylen_recvd = 0;
        connection->cur_session = NULL;
        connection->cur_send_state = NULL;

        if(rv == 0) {
            connection->cmds_outstanding--;
        }
    }
    
 exit:
    return rv;
}

void VSSI_ReceiveVSResponse(VSSI_VSConnectionNode* conn_node,
                            VSSI_VSConnection* connection)
{
    int rc = 0;
    void* buf;
    size_t bufsize;

    // Determine how many bytes to receive.
    bufsize = VSSI_ReceiveVSBuffer(connection, &buf);
    if(buf == NULL || bufsize == 0) {
        // Nothing to receive now.
        return;
    }
    
    // Receive them. Handle any socket errors.
    rc = VPLSocket_Recv(connection->conn_id, buf, bufsize);
    if(rc == 0) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Got socket closed for connection "FMT_VPLSocket_t".",
                          VAL_VPLSocket_t(connection->conn_id));
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
                              rc, VAL_VPLSocket_t(connection->conn_id));
            rc = -1; // stop and disconnect
            break;
        }
    }
    else {
        // Handle received bytes.
        rc = VSSI_HandleVSBuffer(conn_node, connection, rc);
    }
    
    if(rc == -1) {
        VSSI_HandleLostConnection(conn_node, connection, VSSI_COMM);
    }
}

void VSSI_SendVSRequest(VSSI_VSConnectionNode* conn_node,
                        VSSI_VSConnection* connection)
{
    int rc = 0;

    // Send as much of the head request as possible. 
    // When no more requests pending, disable send and notify change.

    if(connection->send_msg == NULL) {
        goto exit;
    }
    
    rc = VPLSocket_Send(connection->conn_id,
                        connection->send_msg->msg + connection->sent_so_far,
                        connection->send_msg->length - connection->sent_so_far);
    if(rc > 0) {
        connection->sent_so_far += rc;
    }
    else {
        // Stop sending at first error.
        // Handle socket closed with receive.
        goto exit;
    }
    
    if(connection->sent_so_far == connection->send_msg->length) {
        connection->sent_so_far = 0;
        free(connection->send_msg);

        if(conn_node->send_head == NULL) {
            connection->send_msg = NULL;
        }
        else {
            // Extract message from head of send queue.
            connection->send_msg = conn_node->send_head->msg;
            conn_node->send_head->msg = NULL;
            // Set connection sending message.
            conn_node->send_head->connection = connection;
            // Pop from send queue.
            conn_node->send_head = conn_node->send_head->next;
            if(conn_node->send_head == NULL) {
                conn_node->send_tail = NULL;
            }
            connection->cmds_outstanding++;
        }
    }

 exit:
    if(connection->send_msg == NULL) {
        // Stop checking for send if nothing left to send.
        connection->enable_send = 0;
        VSSI_NotifySocketChangeFn();
    }
}

void VSSI_InternalForEachSocket(void (*fn)(VPLSocket_t, int, int, void*),
                                void* ctx)
{
    VSSI_VSConnectionNode *vs_node;
    VSSI_VSConnection *connection;
    VSSI_HTTPConnectionNode *http_node;
    VSSI_ConnectProxyState *proxy_node;
    VSSI_SecureTunnelState *tunnel_node;
    int i;

    VPLMutex_Lock(&vssi_mutex);

    // For each VS server
    for(vs_node = vs_connection_list_head; vs_node != NULL; vs_node = vs_node->next) {
        // For each open socket to the server
        connection = vs_node->connections;
        while(connection != NULL) {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, connection->conn_id)) {
                // Call the callback.
                (fn)(connection->conn_id, true, connection->enable_send, ctx);
            }
            connection = connection->next;
        }
    }

    // For each HTTP server
    for(http_node = http_connection_list_head; http_node != NULL; http_node = http_node->next) {
        // For each open socket to the server
        for(i = 0; i < MAX_HTTP_CONNS; i++) {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, http_node->connection[i].conn_id)) {
                // Call the callback.
                (fn)(http_node->connection[i].conn_id, true, false, ctx);
            }
        }
    }

    // For each proxy connection
    for(proxy_node = proxy_connection_list_head; proxy_node != NULL; proxy_node = proxy_node->next) {
        // If making P2P connection, call callback for P2P sockets.
        // Else, call callback for proxy connection.
        if(proxy_node->p2p_flags & VSSI_PROXY_DO_P2P) {
            if((proxy_node->p2p_flags & VSSI_PROXY_PUNCH_P2P) &&
               !VPLSocket_Equal(VPLSOCKET_INVALID, proxy_node->p2p_punch_id)) {
                (fn)(proxy_node->p2p_punch_id, false, true, ctx);
            }
            if((proxy_node->p2p_flags & VSSI_PROXY_LISTEN_P2P) &&
               !VPLSocket_Equal(VPLSOCKET_INVALID, proxy_node->p2p_listen_id)) {
                (fn)(proxy_node->p2p_listen_id, true, false, ctx);
            }
            if((proxy_node->p2p_flags & VSSI_PROXY_CONN_P2P) &&
               !VPLSocket_Equal(VPLSOCKET_INVALID, proxy_node->p2p_conn_id)) {
                (fn)(proxy_node->p2p_conn_id, true, false, ctx);
            }
        }
        else {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, proxy_node->conn_id)) {
                (fn)(proxy_node->conn_id, true, false, ctx);
            }
        }
    }

    // For each tunnel connection
    for(tunnel_node = secure_tunnel_list_head; tunnel_node != NULL; tunnel_node = tunnel_node->next) {
        if(!VPLSocket_Equal(VPLSOCKET_INVALID, tunnel_node->connection)) {
            // Call the callback.
            (fn)(tunnel_node->connection, 
                 tunnel_node->recv_enable, tunnel_node->send_enable,
                 ctx);
        }
    }

    VPLMutex_Unlock(&vssi_mutex);
}

void VSSI_InternalHandleSockets(int (*recv_ready)(VPLSocket_t, void*),
                                int (*send_ready)(VPLSocket_t, void*),
                                void* ctx)
{
    VSSI_VSConnectionNode *vs_node;
    VSSI_HTTPConnectionNode *http_node;
    VSSI_ConnectProxyState *proxy_node;
    VSSI_SecureTunnelState *tunnel_node;
    int i;
    VPLTime_t now = VPLTime_GetTimeStamp();

    VPLMutex_Lock(&vssi_mutex);

    // For each VS server
    for(vs_node = vs_connection_list_head; vs_node != NULL;) {
        VSSI_VSConnectionNode *vs_temp = vs_node;
        VSSI_VSConnection* conn = NULL;
        vs_node = vs_temp->next;
        
        if(vs_temp->connect_state < CONNECTION_CONNECTED) {
            // Don't check. Not connected yet.
            continue;
        }

        VPLMutex_Lock(&(vs_temp->send_mutex));

        // For each open socket to the server:
        for(conn = vs_temp->connections; conn != NULL;) {
            VSSI_VSConnection* tmp = conn;
            conn = conn->next;

            // Ignore any currently unconnected connections, as this walk does not expect that.
            if(tmp->connected == 0){
                continue;
            }
            else if(VPLSocket_Equal(VPLSOCKET_INVALID, tmp->conn_id)) {
                // Connection disconnected. Delete it.
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "VSSP connection "
                                      FMT_VPLSocket_t" to "FMT_VPLNet_addr_t
                                      ":%d disconnected.",
                                      VAL_VPLSocket_t(tmp->conn_id),
                                      VAL_VPLNet_addr_t(vs_temp->inaddr.addr),
                                      VPLNet_port_ntoh(vs_temp->inaddr.port));
                VSSI_HandleLostConnection(vs_temp, tmp, VSSI_COMM);
            }
            else {
                // Check if socket is active
                int recv_now = (recv_ready)(tmp->conn_id, ctx);
                int send_now = (send_ready)(tmp->conn_id, ctx);
                if(send_now) {
                    // Handle send activity for the socket.
                    tmp->last_active = VPLTime_GetTimeStamp();
                    VSSI_SendVSRequest(vs_temp, tmp);
                }
                if(recv_now) {
                    // Handle recv activity for the socket.
                    tmp->last_active = VPLTime_GetTimeStamp();
                    VSSI_ReceiveVSResponse(vs_temp, tmp);
                }
                
                // Check if socket should be timed out and closed.                
                if(!(send_now || recv_now) &&
                   !VPLSocket_Equal(VPLSOCKET_INVALID, tmp->conn_id) &&
                   !tmp->timeout_suspended &&
                   tmp->cmds_outstanding == 0 &&
                   tmp->last_active +
                   tmp->inactive_timeout < now) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "VSSP connection "
                                      FMT_VPLSocket_t" to "FMT_VPLNet_addr_t
                                      ":%d timed-out inactive. "FMT_VPLTime_t
                                      "us since last activity.",
                                      VAL_VPLSocket_t(tmp->conn_id),
                                      VAL_VPLNet_addr_t(vs_temp->inaddr.addr),
                                      VPLNet_port_ntoh(vs_temp->inaddr.port),
                                      VPLTime_DiffAbs(now, tmp->last_active));
                    VSSI_HandleLostConnection(vs_temp, tmp, VSSI_TIMEOUT);
                }
            }
        }

        VPLMutex_Unlock(&(vs_temp->send_mutex));

        // Sweep out unused connections.
        if(vs_temp->refcount == 0 && vs_temp->connections == NULL) {
            VSSI_DeleteServerConnection(vs_temp);
        }
    }

    // For each HTTP server
    for(http_node = http_connection_list_head; http_node != NULL; http_node = http_node->next) {
        // For each open socket to the server
        for(i = 0; i < MAX_HTTP_CONNS; i++) {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, http_node->connection[i].conn_id)) {
                // Check if socket is active
                if((recv_ready)(http_node->connection[i].conn_id, ctx)) {
                    // Handle read activity for the socket.
                    VSSI_ReceiveHTTPResponse(http_node, i);
                    http_node->connection[i].last_active = VPLTime_GetTimeStamp();
                }

                // Check if socket should be timed out and closed.
                else if(http_node->connection[i].last_active +
                        http_node->connection[i].inactive_timeout < now) {
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Timing out HTTP connection "
                                      FMT_VPLSocket_t" to "FMT_VPLNet_addr_t
                                      ":%d due to timeout. "FMT_VPLTime_t
                                      "us since last activity.",
                                      VAL_VPLSocket_t(http_node->connection[i].conn_id),
                                      VAL_VPLNet_addr_t(http_node->inaddr.addr),
                                      VPLNet_port_ntoh(http_node->inaddr.port),
                                      VPLTime_DiffAbs(now, http_node->connection[i].last_active));
                    VSSI_HTTPCommError(http_node, i, VSSI_TIMEOUT);
                }
            }
        }

        // Attempt to send further commands to the server, if any.
        VSSI_SendHTTPCommands(http_node);
    }

    // For each proxy connection
    for(proxy_node = proxy_connection_list_head; proxy_node != NULL; ) {
        VSSI_ConnectProxyState *temp = proxy_node->next;
        
        if(proxy_node->p2p_flags & VSSI_PROXY_DO_P2P) {
            // Check if P2P attempt should time-out.
            if(proxy_node->last_active + 
               proxy_node->inactive_timeout < now) {
                VSSI_HandleProxyDone(proxy_node, VSSI_TIMEOUT);
                // proxy node goes away
            }
            else {
                if(proxy_node->p2p_flags & VSSI_PROXY_PUNCH_P2P) {
                    if((send_ready)(proxy_node->p2p_punch_id, ctx)) {
                        // May have a connection on the punch socket or
                        // need to retry.
                        VSSI_ProxyPunchP2P(proxy_node);
                    }
                }
                if(proxy_node->p2p_flags & VSSI_PROXY_LISTEN_P2P) {
                    if((recv_ready)(proxy_node->p2p_listen_id, ctx)) {
                        // May have a connection on the listening socket.
                        VSSI_ProxyListenP2P(proxy_node);
                    }
                }
                if(proxy_node->p2p_flags & VSSI_PROXY_CONN_P2P) {
                    if((recv_ready)(proxy_node->p2p_conn_id, ctx)) {
                        // Collecting authentication on the connected socket.
                        VSSI_ProxyAuthP2P(proxy_node);
                        // proxy node may go away
                    }
                }
            }
        }
        else {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, proxy_node->conn_id)) {
                // Check if socket is active
                if((recv_ready)(proxy_node->conn_id, ctx)) {
                    // Handle read activity for the socket.
                    VSSI_ReceiveProxyResponse(proxy_node);
                    // proxy node may go away as a result.
                }
                
                // Check if socket should be timed out and closed.
                else if(proxy_node->last_active +
                        proxy_node->inactive_timeout < now) {
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Timing out proxy connection "
                                      FMT_VPLSocket_t" to "FMT_VPLNet_addr_t
                                      ":%d due to timeout. "FMT_VPLTime_t
                                      "us since last activity.",
                                      VAL_VPLSocket_t(proxy_node->conn_id),
                                      VAL_VPLNet_addr_t(proxy_node->inaddr.addr),
                                      VPLNet_port_ntoh(proxy_node->inaddr.port),
                                      VPLTime_DiffAbs(now, proxy_node->last_active));
                    VSSI_HandleProxyDone(proxy_node, VSSI_TIMEOUT);
                }
            }
        }
        proxy_node = temp;
    }

    // For each secure tunnel connection
    for(tunnel_node = secure_tunnel_list_head; tunnel_node != NULL; ) {
        VSSI_SecureTunnelState *temp = tunnel_node->next;
        
        if(!VPLSocket_Equal(VPLSOCKET_INVALID, tunnel_node->connection)) {
            // Check if socket is active
            if((send_ready)(tunnel_node->connection, ctx)) {
                // Handle send activity for the socket.
                VSSI_InternalTunnelDoSend(tunnel_node);
            }
            if((recv_ready)(tunnel_node->connection, ctx)) {
                // Handle recv activity for the socket.
                VSSI_InternalTunnelDoRecv(tunnel_node);
            }
            
            // Check if socket should be timed out and closed.
            // Only applies to tunnels that are still being established or reset.
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, tunnel_node->connection) &&
               ((tunnel_node->connect_type == VSSI_SECURE_TUNNEL_DIRECT &&
                !tunnel_node->authenticated) ||
                (tunnel_node->resetting)) &&
               tunnel_node->last_active + tunnel_node->inactive_timeout < now) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Secure tunnel connection "
                                  FMT_VPLSocket_t" to "FMT_VPLNet_addr_t
                                  ":%d timed-out %s. "FMT_VPLTime_t
                                  "us since last activity.",
                                  VAL_VPLSocket_t(tunnel_node->connection),
                                  VAL_VPLNet_addr_t(tunnel_node->inaddr.addr),
                                  VPLNet_port_ntoh(tunnel_node->inaddr.port),
                                  (tunnel_node->resetting) ? "resetting" : "authenticating",
                                  VPLTime_DiffAbs(now, tunnel_node->last_active));
                tunnel_node->disconnected = true;
            }
        }

        // Sweep out unused tunnels.
        if(tunnel_node->disconnected) {
            VSSI_InternalDestroyTunnel(tunnel_node);
        }

        tunnel_node = temp;
    }

    VPLMutex_Unlock(&vssi_mutex);
}

VPLTime_t VSSI_InternalHandleSocketsTimeout(void)
{
    VPLTime_t rv = VPL_TIMEOUT_NONE;
    VPLTime_t timeout;
    VPLTime_t now = VPLTime_GetTimeStamp();
    VSSI_VSConnectionNode *vs_node;
    VSSI_VSConnection *connection;
    VSSI_HTTPConnectionNode *http_node;
    VSSI_ConnectProxyState *proxy_node;
    VSSI_SecureTunnelState *tunnel_node;
    int i;

    VPLMutex_Lock(&vssi_mutex);

    // For each VS server
    for(vs_node = vs_connection_list_head;
        vs_node != NULL;
        vs_node = vs_node->next) {

        VPLMutex_Lock(&vs_node->send_mutex);

        // For each idle open socket to the server
        for(connection = vs_node->connections; connection != NULL; connection = connection->next) {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, connection->conn_id) &&
               !connection->timeout_suspended &&
               connection->cmds_outstanding == 0) {
                // Determine amount of time until a timeout occurs.
                timeout = VPLTime_DiffClamp(connection->last_active +
                                            connection->inactive_timeout, now);
                // Set new return value if less than existing.
                if(rv == VPL_TIMEOUT_NONE || timeout < rv) {
                    rv = timeout;
                }
            }
        }

        VPLMutex_Unlock(&vs_node->send_mutex);
    }

    // For each HTTP server
    for(http_node = http_connection_list_head; http_node != NULL; http_node = http_node->next) {
        // For each open socket to the server
        for(i = 0; i < MAX_HTTP_CONNS; i++) {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, http_node->connection[i].conn_id)) {
                // Determine amount of time until a timeout occurs.
                timeout = VPLTime_DiffClamp(http_node->connection[i].last_active +
                                            http_node->connection[i].inactive_timeout, now);
                // Set new return value if less than existing.
                if(rv == VPL_TIMEOUT_NONE || timeout < rv) {
                    rv = timeout;
                }
            }
        }
    }

    // For each proxy connection
    for(proxy_node = proxy_connection_list_head;
        proxy_node != NULL;
        proxy_node = proxy_node->next) {

        if(proxy_node->p2p_flags & VSSI_PROXY_DO_P2P) {
            // Determine amount of time until a timeout occurs.
            timeout = VPLTime_DiffClamp(proxy_node->last_active +
                                        proxy_node->inactive_timeout, now);
            // Set new return value if less than existing.
            if(rv == VPL_TIMEOUT_NONE || timeout < rv) {
                rv = timeout;
            }
        }
    }

    // For each tunnel connection
    for(tunnel_node = secure_tunnel_list_head;
        tunnel_node != NULL;
        tunnel_node = tunnel_node->next) {

        if(!VPLSocket_Equal(VPLSOCKET_INVALID, tunnel_node->connection) &&
           ((tunnel_node->connect_type == VSSI_SECURE_TUNNEL_DIRECT &&
             !tunnel_node->authenticated) ||
            (tunnel_node->resetting))) {
            // Determine amount of time until a timeout occurs.
            timeout = VPLTime_DiffClamp(tunnel_node->last_active +
                                        tunnel_node->inactive_timeout, now);
            // Set new return value if less than existing.
            if(rv == VPL_TIMEOUT_NONE || timeout < rv) {
                rv = timeout;
            }
        }

        if(tunnel_node->disconnected) {
            // Handle disconnected tunnels immediately.
            rv = 0;
        }
    }
    
    VPLMutex_Unlock(&vssi_mutex);

    return rv;
}

void VSSI_InternalNetworkDown(void)
{
    VSSI_VSConnectionNode *vs_node;
    VSSI_VSConnection *connection;
    VSSI_HTTPConnectionNode *http_node;
    VSSI_ConnectProxyState *proxy_node;
    VSSI_SecureTunnelState *tunnel_node;
    int i;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Shutting down all connections.");

    VPLMutex_Lock(&vssi_mutex);

    // For each VS server
    for(vs_node = vs_connection_list_head;
        vs_node != NULL;
        vs_node = vs_node->next) {

        VPLMutex_Lock(&(vs_node->send_mutex));

        /// Close all open sockets.
        /// Terminate all outstanding requests with VSSI_COMM error.
        while(vs_node->connections != NULL) {
            connection = vs_node->connections;
            vs_node->connections = connection->next;

            if(!VPLSocket_Equal(VPLSOCKET_INVALID, connection->conn_id)) {
                VSSI_HandleLostConnection(vs_node, connection, VSSI_NET_DOWN);
            }
        }

        /// Mark all connections as unusable.
        vs_node->connect_state = CONNECTION_FAILED;

        VPLMutex_Unlock(&(vs_node->send_mutex));
    }

    // For each HTTP server
    for(http_node = http_connection_list_head; http_node != NULL; http_node = http_node->next) {
        // For each open socket to the server
        for(i = 0; i < MAX_HTTP_CONNS; i++) {
            if(!VPLSocket_Equal(VPLSOCKET_INVALID, http_node->connection[i].conn_id)) {
                VSSI_HTTPCommError(http_node, i, VSSI_NET_DOWN);
            }
        }

        /// Mark all connections as unusable.
        http_node->connection_lost = 1;
    }

    // For each proxy connection
    for(proxy_node = proxy_connection_list_head; proxy_node != NULL; ) {
        VSSI_ConnectProxyState *temp = proxy_node->next;

        if(!VPLSocket_Equal(VPLSOCKET_INVALID, proxy_node->conn_id)) {
            VSSI_HandleProxyDone(proxy_node, VSSI_NET_DOWN);
        }

        proxy_node = temp;
    }

    // For each secure tunnel connection
    for(tunnel_node = secure_tunnel_list_head;
        tunnel_node != NULL; 
        tunnel_node = tunnel_node->next) {

        if(!VPLSocket_Equal(VPLSOCKET_INVALID, tunnel_node->connection)) {
            // Close connection. Leave tunnel state in-tact.
            VPLSocket_Close(tunnel_node->connection);
            tunnel_node->connection = VPLSOCKET_INVALID;
        }
    }

    VPLMutex_Unlock(&vssi_mutex);
}

int VSSI_Sendall(VPLSocket_t sockfd, const void* msg, int len)
{
    int rv = 0;
    int rc;

    do {
        rc = VPLSocket_Send(sockfd, (char*)msg + rv,
                            len - rv);
        if(rc > 0) {
            rv += rc;
        }
        else if(rc == VPL_ERR_AGAIN) {
            // Wait up to 5 seconds for ready to send, then give up.
            VPLSocket_poll_t psock;
            psock.socket = sockfd;
            psock.events = VPLSOCKET_POLL_OUT;
            rc = VPLSocket_Poll(&psock, 1, VPLTIME_FROM_SEC(5));
            if(rc < 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Encountered error %d waiting for socket ready to send.",
                                 rc);
                rv = rc;
                break;
            }
            if(psock.revents != VPLSOCKET_POLL_OUT) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Encountered unexpected event %x waiting for socket ready to send.",
                                 psock.revents);
                rv = VPL_ERR_FAIL;
                break;
            }
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Encountered error %d on attempt to send.",
                             rc);
            rv = rc;
            break;
        }
    } while(rv < len);

    return rv;
}

VSSI_Result VSSI_InternalInit(void)
{
    int rv = VSSI_SUCCESS;

    return rv;
}

void VSSI_InternalCleanup(void)
{

}
