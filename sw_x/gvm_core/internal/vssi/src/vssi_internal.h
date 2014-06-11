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

/// @file
/// Virtual Storage internal APIs and definitions.
/// This header is for use within VSSI only. Users of the library should
/// only include vssi.h, vssi_types.h and vssi_error.h.

#ifndef VSSI_INTERNAL_H_
#define VSSI_INTERNAL_H_

#include "vpl_th.h"
#include "vpl_socket.h"

#include "vssi_types.h"
#include "vss_comm.h"

#include "csltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

 /// Forward declarations
struct _VSSI_ObjectState;
struct _VSSI_SendState;
struct _VSSI_PendInfo;
struct _VSSI_FileState;

/// A VSSI user session context
typedef struct {
    u64  handle;             /// Session handle
#if (SERVICE_TICKET_LEN != CSL_SHA1_DIGESTSIZE)
#  error "Service ticket length != CSL SHA1 digest size!"
#endif
    char secret[SERVICE_TICKET_LEN]; /// Session service ticket (hashing key v0)
    char signingKey[CSL_SHA1_DIGESTSIZE]; /// Session signing key (v1)
    char encryptingKey[CSL_AES_KEYSIZE_BYTES]; /// Session encryption key (v1)
    int refcount;            /// # times registered - # times ended
    struct _VSSI_SendState* send_states; // Send states using this session to servers by server ID.
    struct _VSSI_ObjectState* objects; // Open objects on this session.
    int connect_count; // number of proxy connections in-progress
} VSSI_UserSession;

/// User session node for list of active user sessions
typedef struct _VSSI_SessionNode {
    struct _VSSI_SessionNode* next;
    VSSI_UserSession session;
} VSSI_SessionNode;

///
/// Proxy connection context
///
typedef struct _VSSI_ConnectProxyState {
    struct _VSSI_ConnectProxyState* next; /// For internal list while processing
    VSSI_UserSession* session; /// User session requesting proxy connect
    VSSI_Callback callback;  /// Callback when done
    void* ctx;               /// Context for callback
    VPLSocket_t* socket;     /// Return for open socket on success
    int* is_direct;          /// Return for direct vs. proxy connection made
    VPLSocket_t conn_id;     /// Connection for attempt in-progress
    VPLSocket_t p2p_listen_id; /// P2P listen socket
    VPLSocket_t p2p_punch_id; /// P2P punch (connect) attempt socket
    VPLSocket_t p2p_conn_id; /// Successful P2P connection when valid
    int p2p_flags;           /// Try to make a P2P connection after proxy
#define VSSI_PROXY_TRY_P2P   (1<<0) /// Requested to try P2P connection
#define VSSI_PROXY_DO_P2P    (1<<1) /// Do P2P connect attempt
#define VSSI_PROXY_PUNCH_P2P (1<<2) /// connecting (hole-punching) for P2P
#define VSSI_PROXY_LISTEN_P2P (1<<3) /// listening for P2P connection
#define VSSI_PROXY_CONN_P2P  (1<<4) /// Made P2P connection
    u8 type;                 /// Type of proxy connection to make
    u64 destination_device;  /// Ultimate destination
    VPLSocket_addr_t inaddr; /// P2P address when attempting P2P connection.
    VPLTime_t last_active;   /// Last time this connection was active.
    VPLTime_t inactive_timeout; /// Amount of time until connection considered inactive.
#define VSSI_PROXY_CONNECT_TIMEOUT VPLTime_FromSec(30) /// timeout to establish connection
#define VSSI_PROXY_P2P_CONN_TIMEOUT VPLTime_FromSec(5) /// Timeout to make P2P connection
#define VSSI_PROXY_P2P_AUTH_TIMEOUT VPLTime_FromSec(3) /// Timeout to authenticate P2P connection
    char* incoming_reply;    /// proxy request reply when receiving
    size_t replylen;         /// Length of reply when known
    size_t so_far;           /// reply received so far
} VSSI_ConnectProxyState;

///
/// HTTP object structure
///

struct _VSSI_HTTPComponent; /// Forward declaration

/// Single pending command. Will be sent once a connection is free.
typedef struct _VSSI_PendingHTTPCommand {
    struct _VSSI_PendingHTTPCommand* next;

    u64 offset; /// Read offset
    u32* length; /// Pointer to length to read
    char* buf; /// Buffer to fill on read reply

    VSSI_Callback callback; /// Callback to call
    void* ctx; /// Callback context for this command.
    VSSI_Result result;

    struct _VSSI_HTTPComponent* component; /// Component being read

    char* partial_line; // partial line in-progress
    size_t partial_line_len;
    char* response_line; // Response line received
    char** headers; // Array of headers received
    size_t num_headers; // number of complete headers received

    int collect_body; // Flag to indicate time to collect reply body data

    // fields read from headers
    int chunked;// If true, the response data is in chunked format.
    size_t reply_length; // reply body length
    int conn_closed; // Reply had "Connect: close" header.

    // If a chunked reply, state of the chunked-data receive
    int chunk_length;
    int chunk_so_far;

    // If a non-chunked reply, the reply received so far.
    size_t reply_so_far; // reply body bytes received
} VSSI_PendingHTTPCommand;

/// Context for an HTTP server connection, possibly used by many user sessions.
typedef struct {
    VPLSocket_t conn_id;     /// Connection handle (socket)

    // Don't reuse this connection until socket has been closed and reopened.
    int stale_connection;

    // Pipelining is allowed on this connection.
    int pipelining;

    /// Currently active command. NULL when connection is idle.
    /// Received data is for this command.
    VSSI_PendingHTTPCommand* active_cmd;
    /// Queue of sent commands awaiting reply.
    VSSI_PendingHTTPCommand* sent_head;
    VSSI_PendingHTTPCommand* sent_tail;
    int sent_count;

    VPLTime_t last_active;
    VPLTime_t inactive_timeout;
#define VSSI_HTTP_CONNECTION_TIMEOUT VPLTime_FromSec(30)
} VSSI_HTTPConnection;

/// HTTP server connection node for list of active server sessions
#define MAX_HTTP_CONNS 4
typedef struct _VSSI_HTTPConnectionNode {
    struct _VSSI_HTTPConnectionNode* next;

    /// mutex for adding pending commands, selecting connection for send
    VPLMutex_t send_mutex;

    VPLSocket_addr_t inaddr; /// server IP address and port

    VSSI_HTTPConnection connection[MAX_HTTP_CONNS]; // sockets to server

    int connection_lost;

    VSSI_PendingHTTPCommand* pending_head; // Commands waiting to be sent
    VSSI_PendingHTTPCommand* pending_tail;
} VSSI_HTTPConnectionNode;

typedef struct _VSSI_HTTPComponent{
    char* name; /// Type of object-file

    char* server; /// Server name, for later IP lookup
    VPLSocket_addr_t server_inaddr; // Server IP and port
    char* uri;        /// URI for the object-file

    VSSI_HTTPConnectionNode* server_connection; /// Active server connection
} VSSI_HTTPComponent;

typedef struct {
    VSSI_HTTPComponent* components; /// List of object components
    int num_components;
} VSSI_HTTPAccess;

///
/// VS protocol object structure
///

/// Generic message.
typedef struct _VSSI_MsgBuffer {
    struct _VSSI_MsgBuffer* next;
    size_t length;
    char msg[];
} VSSI_MsgBuffer;


/// Context for a VS server connection, possibly used by many user sessions.
typedef struct _VSSI_VSConnection {
    VPLSocket_t conn_id;     /// Connection handle (socket)
    int connected;           /// False when connection being made (proxy/P2P or additional sockets).
                             /// True once connection made or given up.

    int enable_send;
    // Current message being sent. 
    VSSI_MsgBuffer* send_msg;
    size_t sent_so_far;

    char* incoming_reply;    /// Pointer to buffer of incoming reply
    size_t reply_bufsize;    /// Current length of incoming reply buffer
    size_t replylen;         /// Length of reply to expect, when receiving
    size_t replylen_recvd;   /// Length of reply received so far
    VSSI_UserSession* cur_session; /// Session for incoming reply
    struct _VSSI_SendState* cur_send_state; /// send-state for incoming reply

    VPLTime_t last_active;
    VPLTime_t inactive_timeout;
#define VSSI_VS_CONNECTION_TIMEOUT VPLTime_FromSec(15)
#define VSSI_VS_PROXY_CONNECTION_TIMEOUT VPLTime_FromSec(300 - 15) // 4m 45s.

    int cmds_outstanding;    /// Number of commands outstanding on this socket.
    int timeout_suspended;   /// If nonzero, count of objects depending on connection for notifications.
                             /// Connection cannot timeout while nonzero.
    struct _VSSI_VSConnection* next;
} VSSI_VSConnection;

/// VS server connection node for list of active server sessions
typedef struct _VSSI_VSConnectionNode {
    struct _VSSI_VSConnectionNode* next;

    VPLMutex_t send_mutex;   /// mutex for sending commands on this connection

    /// This server's cluster ID (server ID).
    u64 cluster_id;

    /// IP address, port, and route type when connected.
    VPLSocket_addr_t inaddr;
    int type;
    enum {
        CONNECTION_DISCONNECTED,
        CONNECTION_CONNECTING,
        CONNECTION_CONNECTED,
        CONNECTION_ACTIVE,
        CONNECTION_FAILED
    } connect_state;

    /// Object (routes and session) invoking connection, and current route being tried.
    int route_id;
    struct _VSSI_ObjectState* object;

    int refcount;

    /// Queue of messages pending send on any connection.
    struct _VSSI_PendInfo* send_head;
    struct _VSSI_PendInfo* send_tail;

    VSSI_VSConnection* connections; // Sockets to server
} VSSI_VSConnectionNode;

enum {
    DELETE_NEVER,
    DELETE_ON_FAIL,
    DELETE_ALWAYS
};

/// State for pending commands.
typedef struct _VSSI_PendInfo {
    struct _VSSI_PendInfo* next_free; // pointer to next free struct, when free.
    struct _VSSI_PendInfo* next; // pointer to next pending when waiting to send.
    u32 xid; /// XID of the command.
    u8 command; /// type of command 
    VSSI_MsgBuffer* msg; // prepared message to send, if waiting for connection
    VSSI_VSConnection* connection; /// Server connection that sent this command.

    struct _VSSI_ObjectState* object; /// Object for the command.
    int delete_obj_when; // Delete object condition.
    struct _VSSI_FileState* file_state; /// File handle state for the command
    int delete_filestate_when; // Delete filestate condition.
    u32* length; /// Pointer to length read/written (for read/read_diff/write)
    u64* offset; /// Pointer to offset (for read_diff only)
    char* buf; /// Buffer to fill on read reply (for read/read_diff)
    VSSI_Object* handle; /// Handle returned on open success (for open only)
    VSSI_Dir* directory; /// pointer to open dir on success (open dir only)
    VSSI_Trashcan* trashcan; /// pointer to open trashcan on success (open trashcan only)
    VSSI_File* file; /// Handle returned on open file success (for open file only)
    VSSI_Dirent** stats; /// pointer to file stats pointer (stat only)
    VSSI_Dirent2** stats2; /// pointer to file stats pointer (stat2 only)
    u64* disk_size; /// pointer to disk size (for getspace only)
    u64* dataset_size; /// pointer to dataset size (for getspace only)
    u64* avail_size; /// pointer to available size (for getspace only)
    VSSI_NotifyCallback notify_callback; /// Callback for notifications when setting events (for notify only)
    void* notify_ctx; /// Context for notification when setting events (for notify only)
    VSSI_NotifyMask* notify_mask; /// Location for notify events active return (for notify only)
    VSSI_FileLockState* lock_state; /// Location for file lock state return (for filelocking only)
    VSSI_Callback callback; /// Callback to call
    void* ctx; /// Callback context for this pending command.
} VSSI_PendInfo;

/// Pending commands context for a given user session and server connection
#define PEND_CMD_GROW_CNT 16
typedef struct _VSSI_SendState {
    u64 cluster_id; /// Server cluster ID.

    VSSI_UserSession* session; /// parent user session

    VPLMutex_t send_mutex;   /// mutex for req_id and pending cmd table

    u32 seq_id;    /// Session sequence ID for outgoing commands

    u8 sign_mode;
    u8 sign_type;
    u8 enc_type;
    enum {
        NEGOTIATE_NEEDED,  // need to start negotiation
        NEGOTIATE_PENDING, // negotiation underway
        NEGOTIATE_DONE     // negotiation done
    } negotiated;
    VSSI_VSConnectionNode* server_connection;

    /// Queue of messages waiting to be sent after negotiation done.
    VSSI_PendInfo* wait_head;
    VSSI_PendInfo* wait_tail;

    /// Table of pending cmds, dynamically allocated.
    int num_cmd_slots; // Number of current command slots.
    int num_cmd_slots_active; // Number of active command slots now.
    VSSI_PendInfo* free_stack; // stack of free pending command structs
    VSSI_PendInfo** pending_cmds; // active commands
    VSSI_PendInfo** cmd_chunks; // allocated chunks of commands
    int num_chunks; // number of allocated chunks of any size

    struct _VSSI_SendState* next;
} VSSI_SendState;

/// Object access protocol types
enum {
    VSSI_PROTO_INVALID = 0, /// Invalid protocol (uninitialized object?)
    VSSI_PROTO_VS,          /// Virtual storage service protocol (VSSP)
    VSSI_PROTO_HTTP,        /// HTTP protocol, for use with CDNs
};

typedef struct {
    /// Dataset identifiers
    u64 cluster_id; /// host server(s) cluster ID
    u64 user_id;   
    u64 dataset_id;

    /// VS Handle when object is open
    u32 proto_handle;

    /// List of available routes to use.
    VSSI_Route* routes;
    int num_routes;
} VSSI_VSAccess;

/// An instantiated VSSI object
typedef struct _VSSI_ObjectState {
    VSSI_UserSession* user_session; /// User session context for this object.

    int proto;           /// Object access protocol (format).
    union {
        VSSI_VSAccess vs;     /// for dataset object format
        VSSI_HTTPAccess http; /// for content object format
    } access; /// Access info, keyed by #proto.

    int mode;          /// Object open mode
    u64 version;       /// Current object version
    u64 new_version;   /// When in conflict, actual object version
    u32 request_size;  /// Desired request size, from object description

    /// Callback and context for async notifications received for this object.
    VSSI_NotifyCallback notify_callback;
    void* notify_ctx;
    VSSI_VSConnection* notify_connection;
    struct _VSSI_FileState* files;  /// List of open file handles
    u32 num_files;                  /// Number of open file handles

    struct _VSSI_ObjectState* next;
} VSSI_ObjectState;

/// VSSI Client instantiation of file handle
typedef struct _VSSI_FileState {
    VSSI_ObjectState *object;   /// Back pointer to containing object
    u32 flags;          /// File open mode bits
    u64 origin;         /// Unique origin ID
    u32 server_handle;  /// File handle returned by the server

    struct _VSSI_FileState *next;   /// Single linked list pointer
} VSSI_FileState;

/// State when attempting to establish a server connection.
typedef struct {
    int num_tries; // Number of times to try before quit.
    int next_route; // Next route to try.
    VSSI_ObjectState* object; // Object state to use
    VSSI_Callback callback; /// Original callback
    void* ctx; /// Original context
    VSSI_Object* handle; // handle for open command
    u8 mode; // mode for open command
    u8 deleteNotOpen; // Action to take after connect. true:delete, false:open
} VSSI_ConnectAndActState;

/// State for callback waiting for a ready condition (send or receive).
typedef struct _VSSI_TunnelWaiter {
    struct _VSSI_TunnelWaiter* next;
    
    void* ctx;
    VSSI_Callback callback;
} VSSI_TunnelWaiter;

/// State for secure tunnel connections.
typedef struct _VSSI_SecureTunnelState {
    struct _VSSI_SecureTunnelState* next; // Next in list

    /// Mutex for this tunnel state.
    VPLMutex_t mutex;

    /// User session context for tunnel
    VSSI_UserSession* session; 

    /// Destination of this secure tunnel
    u64 destination_device;

    /// State when establishing secure tunnel
    VPLTime_t inactive_timeout;
    VPLTime_t last_active;
#define VSSI_TUNNEL_CONNECTION_TIMEOUT VPLTime_FromSec(5)
    int authenticated;
    VSSI_SecureTunnel* client_handle; // Set when tunnel created.
    void* client_ctx; // For client callback when tunnel created or failed.
    VSSI_Callback client_callback; // Client callback when tunnel created or failed.
    VSSI_Result fail_reason;

    /// When connected, connected socket
    VPLSocket_t connection; /// Connected socket
    int recv_enable; // Send on socket enabled
    int send_enable; // Receive on socket enabled
    int disconnected;
#define VSSI_TUNNEL_RECV_Q_MAX (1024 * 1024) // 1MB
#define VSSI_TUNNEL_SEND_Q_MAX (1024 * 1024) // 1MB

    VPLSocket_addr_t inaddr; /// Server IP address and port (direct conenction only)
    VSSI_SecureTunnelConnectType connect_type; /// Type of connection made.
    int is_direct; // When making proxy connection, return code for proxy vs. p2p connection made.

    /// When connected, send queue.
    VSSI_MsgBuffer* send_head;
    VSSI_MsgBuffer* send_tail;
    size_t sent_so_far; // bytes sent of head buffer
    size_t send_q_depth; // bytes in entire send queue
    u32 next_xid; // increment per message sent (for encryption XID)

    // When connected, callbacks waiting for space to send
    VSSI_TunnelWaiter* send_wait_top;

    /// When connected, receive queue.
    VSSI_MsgBuffer* recv_head;
    VSSI_MsgBuffer* recv_tail;
    size_t recv_so_far; // bytes received of head buffer
    size_t recv_q_depth; // bytes in entire receive queue

    // When connected, callbacks waiting for data received
    VSSI_TunnelWaiter* recv_wait_top;

    // When connected, callbacks waiting for tunnel reset
    VSSI_TunnelWaiter* reset_wait_top;
    int resetting;
#define VSSI_TUNNEL_RESET_TIMEOUT VPLTime_FromSec(10)

    // While receiving data, receive state.
    char* incoming_reply;    /// Pointer to buffer of incoming reply
    size_t reply_bufsize;    /// Current length of incoming reply buffer
    size_t replylen;         /// Length of reply to expect, when receiving
    size_t replylen_recvd;   /// Length of reply received so far

    // Security settings for tunnel data
    u8 sign_mode;
    u8 sign_type;
    u8 enc_type;

} VSSI_SecureTunnelState;

/// A directory entry's metadata and read state.
typedef struct {
    size_t dataSize;
    size_t curOffset;
    VSSI_Metadata curEntry;
    const char* data;
} VSSI_DirentMetadataState;

/// A directory struct, for open/read/close/etc directory.
typedef struct {
    int version; // 1 or 2
    char* raw_data; // raw data from server
    size_t data_len; // length of data from server
    size_t offset; // offset to start of next entry
    VSSI_Dirent cur_entry; // entry that would be shown to client
    VSSI_Dirent2 cur_entry2; // entry that would be shown to client
    VSSI_DirentMetadataState cur_metadata; // visible entry's metadata shown to the client.
} VSSI_DirectoryState;

/// A trashcan struct, for open/read/close/etc trashcan.
typedef struct {
    char* raw_data; // raw data from server
    size_t data_len; // length of data from server
    size_t offset; // offset to start of next record
    VSSI_TrashRecord cur_record; // record that would be shown to client
} VSSI_TrashcanState;

/// Build an object
VSSI_ObjectState* VSSI_BuildObject2(u64 user_id,
                                    u64 dataset_id,
                                    const VSSI_RouteInfo* route_info,
                                    VSSI_UserSession* session);
VSSI_ObjectState* VSSI_BuildObject(const char* obj_xml,
                                   VSSI_UserSession* session);
int VSSI_BuildDatasetObject(const char* body_xml,
                            VSSI_ObjectState** object);
int VSSI_BuildDatasetRoute(const char* route_xml,
                           VSSI_Route* route,
                           VSSI_ObjectState** object,
                           size_t* object_size);
void VSSI_RebaseDatasetRoute(VSSI_Route* route,
                             size_t offset);
int VSSI_BuildContentObject(const char* body_xml,
                            VSSI_ObjectState** object);
int VSSI_BuildContentComponent(const char* component_xml,
                               VSSI_HTTPComponent* component,
                               VSSI_ObjectState** object,
                               size_t* object_size);
void VSSI_RebaseContentComponent(VSSI_HTTPComponent* component,
                                 size_t offset);
int VSSI_BuildAtomicObject(const char* body_xml,
                           VSSI_ObjectState** object);

/// Check if an object is in the conflict state.
int VSSI_CheckObjectConflict(VSSI_ObjectState* object);

/// Free a previously built object.
void VSSI_FreeObject(VSSI_ObjectState* object);

/// Free a previously built filestate.
void VSSI_FreeFileState(VSSI_FileState* filestate);

/// Confirm that a session is currently registered.
VPL_BOOL VSSI_SessionRegistered(VSSI_UserSession* session);

/// Make a proxy connection
void VSSI_VSConnectProxy(VSSI_ConnectProxyState* state,
                         const char* server, VPLNet_port_t port);
/// Receive proxy connect (auth, success) response
void VSSI_ReceiveProxyResponse(VSSI_ConnectProxyState* state);
/// Begin P2P connect attempt after proxy success.
void VSSI_ProxyToP2P(VSSI_ConnectProxyState* state);
/// Check P2P punch attempt.
void VSSI_ProxyPunchP2P(VSSI_ConnectProxyState* state);
/// Listen for P2P connection from server.
void VSSI_ProxyListenP2P(VSSI_ConnectProxyState* state);
/// Send Authenticate message to server.
void VSSI_ProxySendAuthP2P(VSSI_ConnectProxyState* state);
/// Authenticate P2P connection made with server. May end P2P success or fail.
void VSSI_ProxyAuthP2P(VSSI_ConnectProxyState* state);
/// Proxy connect done. Will call client callback with result.
void VSSI_HandleProxyDone(VSSI_ConnectProxyState* state, VSSI_Result result);

/// Reconnect a socket to a server
/// @param socket Pointer to socket on success
/// @param inaddr Address of server to connect to
/// @param reuse_addr If nonzero, set SO_REUSEADDR option for socket.
/// @param use_inaddr If not NULL, bind this address for the socket.
/// @param timeout_nonroutable Timeout to use when connecting to a nonroutable address.  <0 means to use default of 1 sec.
/// @param timeout_routable Timeout to use when connecting to a routable address.  <0 means to use default of 5 sec.
/// @return  1 on success requiring new connection to be made, with @a socket set to a valid value.
///          0 on success, with @a socket set to a valid value.
///         -1 on fail to connect, with @a socket set to invalid value.
int reconnect_server(VPLSocket_t* socket,
                     VPLSocket_addr_t* inaddr,
                     int reuse_addr,
                     VPLSocket_addr_t* use_inaddr,
                     int timeout_nonroutable,
                     int timeout_routable);
int connect_server(VPLSocket_t* socket,
                   const char* server, VPLNet_port_t port,
                   int reuse_addr);

/// Establish connection to a VS server using a route from the object provided.
void VSSI_ConnectVSServer(VSSI_VSConnectionNode* server_connection, VSSI_Result result);
void VSSI_ConnectVSServerDone(void* ctx, VSSI_Result result);

void VSSI_HandleLostConnection(VSSI_VSConnectionNode* conn_node,
                               VSSI_VSConnection* connection,
                               VSSI_Result result);

/// Add details for a pending command to the pending commands table.
/// Private fields will be filled in on the table copy.
/// Returns VSSI_SUCCESS on success, -VSSI_NOMEM on failure.
/// @a xid_out is set to the XID used on success.
VSSI_Result VSSI_AddPendingCommand(VSSI_SendState* send_state,
                                   const VSSI_PendInfo* cmd_data,
                                   u32* xid_out);

/// Add a pending command to the wait queue.
VSSI_Result VSSI_AddWaitingCommand(VSSI_SendState* send_state,
                                   const VSSI_PendInfo* cmd_data);

/// Place command from wait queue into pending commands table.
VSSI_Result VSSI_WaitingCommandToPending(VSSI_SendState* send_state,
                                         VSSI_PendInfo* cmd_data,
                                         u32* xid_out);

/// Get the pending command slot for a given XID.
/// Returns pointer to slot to use, NULL if not possible.
VSSI_PendInfo* VSSI_GetVSCommandSlot(VSSI_SendState* send_state,
                                     u32 xid);
/// Get the command slot index for a given XID. Assume lock held.
int VSSI_GetCommandSlotUnlocked(VSSI_SendState* send_state, u32 xid);

/// Update the connection used to send a command.
void VSSI_UpdateCommandConnection(VSSI_SendState* send_state, u32 xid,
                                  VSSI_VSConnection* connection);

/// Resolve an outstanding command, if still outstanding.
/// @return 0 on command resolved, -1 on no command to resolve.
int VSSI_ResolveCommand(VSSI_PendInfo* command,
                        VSSI_Result result);

/// Reply to all pending commands for a send state with a given result.
/// Use mainly when pending commands will not get a normal reply,
/// such as when connection is lost.
void VSSI_ReplyPendingCommands(VSSI_SendState* send_state,
                               VSSI_VSConnection* connection,
                               VSSI_Result result);

/// Get the send state for a connection and session.
/// Create it if it doesn't exist.
VSSI_SendState* VSSI_GetSendState(VSSI_UserSession* user_session,
                                  u64 cluster_id);

/// Destroy a send state when done with it.
void VSSI_DestroySendState(VSSI_SendState* send_state);
/// Same, holding vssi_mutex.
void VSSI_DestroySendStateUnlocked(VSSI_SendState* send_state);

/// For each active socket, call @a fn with the socket.
/// @param fn function taking a VPLSocket_t and provided to call.
/// @param ctx Pointer to some context for the callback.
/// @note Caller may use this to collect the set of active sockets from VSSI.
void VSSI_InternalForEachSocket(void (*fn)(VPLSocket_t, int, int, void*),
                                void* ctx);

/// For each active socket, check @a read_ready to see if the socket is ready
/// for receive, and if so, handle any receive activity on the socket.
/// @param recv_ready function taking a VPLSocket_t that will return true when
///        the socket is ready for receive.
/// @param ctx Pointer to some context for the callback.
void VSSI_InternalHandleSockets(int (*recv_ready)(VPLSocket_t, void*),
                                int (*send_ready)(VPLSocket_t, void*),
                                void* ctx);
size_t VSSI_ReceiveVSBuffer(VSSI_VSConnection* connection,
                            void** buf_out);
int VSSI_HandleVSBuffer(VSSI_VSConnectionNode* conn_node,
                        VSSI_VSConnection* connection,
                        int bufsize);
void VSSI_ReceiveVSResponse(VSSI_VSConnectionNode* conn_node,
                            VSSI_VSConnection* connection);
void VSSI_SendVSRequest(VSSI_VSConnectionNode* conn_node,
                        VSSI_VSConnection* connection);

/// Get the maximum amount of time to wait for a socket to be ready before
/// handling sockets. If this much time passes without socket ready, call
/// #VSSI_HandleSockets() anyway to handle any timeout events.
/// @return Amount of time to wait for socket ready.
///         VPL_TIMEOUT_NONE if no timeout required.
VPLTime_t VSSI_InternalHandleSocketsTimeout(void);

/// Close all open sockets.
/// Terminate all outstanding requests with VSSI_COMM error.
/// Mark all connections as unusable.
void VSSI_InternalNetworkDown(void);

/// Get an existing connection node for the provided route.
/// If necessary, create the connection node.
/// Connection node returned increases reference count.
VSSI_VSConnectionNode* VSSI_GetServerConnection(u64 cluster_id);
/// Release a connection node.
void VSSI_ReleaseServerConnection(VSSI_VSConnectionNode* connection);
/// If a connection node has no references, close connections and delete node.
void VSSI_DeleteServerConnection(VSSI_VSConnectionNode* connection);

/// Send all of a buffer via given socket.
int VSSI_Sendall(VPLSocket_t sockfd, const void* msg, int len);

/// Callback to complete secure tunnel via proxy connection.
void VSSI_InternalCompleteProxyTunnel(void* ctx, VSSI_Result result);

/// Receive from secure tunnel
/// Receive from queue
VSSI_Result VSSI_InternalTunnelRecv(VSSI_SecureTunnelState* tunnel,
                                    char* data,
                                    size_t len);
/// Receive from network
void VSSI_InternalTunnelDoRecv(VSSI_SecureTunnelState* tunnel);

/// Wait for secure tunnel receive ready.
void VSSI_InternalTunnelWaitForRecv(VSSI_SecureTunnelState* tunnel,
                                    void* ctx,
                                    VSSI_Callback callback);

/// Send via secure tunnel
/// Queue to send
VSSI_Result VSSI_InternalTunnelSend(VSSI_SecureTunnelState* tunnel,
                                    const char* data,
                                    size_t len);
/// Actually send
void VSSI_InternalTunnelDoSend(VSSI_SecureTunnelState* tunnel);

/// Wait for secure tunnel send ready.
void VSSI_InternalTunnelWaitForSend(VSSI_SecureTunnelState* tunnel,
                                    void* ctx,
                                    VSSI_Callback callback);

/// Reset secure tunnel.
void VSSI_InternalTunnelReset(VSSI_SecureTunnelState* tunnel,
                              void* ctx,
                              VSSI_Callback callback);


/// Clear all callbacks for a  secure tunnel, as prep for destroying the tunnel.
/// Assumes vssi_mutex is locked.
void VSSI_InternalClearTunnel(VSSI_SecureTunnelState* tunnel);
void VSSI_InternalClearTunnelNoLock(VSSI_SecureTunnelState* tunnel);

/// Destroy a secure tunnel.
/// Assumes vssi_mutex is locked.
void VSSI_InternalDestroyTunnel(VSSI_SecureTunnelState* tunnel);

/// Perform internal init of VSSI.
/// Returns result of internal init.
VSSI_Result VSSI_InternalInit(void);

/// Perform internal cleanup of VSSI
void VSSI_InternalCleanup(void);

int VSSI_InternalSetParameter(VSSI_Param id, int value);
int VSSI_InternalGetParameter(VSSI_Param id, int* value_out);


#ifdef __cplusplus
}
#endif

#endif // include guard
