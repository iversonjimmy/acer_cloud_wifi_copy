//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#ifndef PXD_CLIENT_H
#define PXD_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  This file defines the interface for interacting with the PD (Proxy
 *  Daemon) services.
 */
typedef struct pxd_client_s pxd_client_t;

/*
 *  All routines return error messages through this structure.
 */
typedef struct {
    int           error;
    const char *  message;
} pxd_error_t;

/*
 *  The proxy daemon provides naming services.  This structure is used to
 *  identify a CCD instance to which a client wants to create a tunnel, or
 *  the CCD instance that is making a request for a tunnel connection.
 */
typedef struct {
    char *   region;
    int64_t  user_id;
    int64_t  device_id;
    char *   instance_id;
} pxd_id_t;

typedef struct {
    pxd_id_t *  id;
    void *      opaque;             /* from IAS               */
    int         opaque_length;
    void *      key;                /* from IAS               */
    int         key_length;
} pxd_cred_t;

typedef struct {
    void *    opaque;
    int       opaque_length;
    void *    server_key;
    int       server_length;
} pxd_verify_t;

/*
 *  A specific CCD instance typically will have at least two addresses:  a
 *  local address for its local network, and a global address that likely is
 *  addressed through a NAT firewall.  This structure is used to return a
 *  single address to the pd client.
 */
typedef struct {
    char *  ip_address;
    int     ip_length;
    int     port;   // in native byte order
    char    type;
} pxd_address_t;

/*
 *  Now define a structure for returning blob data.
 */
typedef struct {
    int16_t   version;

    int64_t   client_user;
    int64_t   client_device;
    char *    client_instance;

    int64_t   server_user;
    int64_t   server_device;
    char *    server_instance;

    int64_t   create;
    char *    key;
    int       key_length;
    char *    handle;
    int       handle_length;
    char *    service_id;
    int       service_id_length;
    char *    ticket;
    int       ticket_length;
    char *    extra;          /* for testing */
    int       extra_length;   /* for testing */
} pxd_blob_t;

/*
 *  Define the data structures for the callback that occurs when a
 *  CCD instance receives an incoming proxy connection request.
 */
typedef struct {
    void *           client_opaque;  /* from the pxd_open call      */
    void *           op_opaque;      /* from the operation          */
    uint64_t         result;         /* a result code               */
    char *           region;         /* the server region           */
    uint64_t         user_id;        /* identifies the CCD instance */
    uint64_t         device_id;      /* identifies the CCD instance */
    char *           instance_id;    /* the server instance id      */
    char *           ans_dns;        /* the server's ANS DNS        */
    char *           pxd_dns;        /* the server's PXD DNS        */
    int              address_count;  /* the number of addresses     */
    pxd_address_t *  addresses;      /* an address list             */
    VPLSocket_t      socket;         /* the data socket             */
    pxd_blob_t *     blob;           /* information on a client CCD */
} pxd_cb_data_t;

typedef void (*pxd_cb_t)(pxd_cb_data_t *);

/*
 *      Define the callbacks that a CCD instance can receive.  Each
 *  callback completes only part of the pxd_cb_data_t, as described
 *  below.
 *
 *  supply_local
 *          The supply_local callback returns the TCP/IP address
 *      and port number as seen by the local end of the socket.
 *
 *      Returned fields:
 *          client_opaque
 *          addresses
 *          address_count
 *
 *          The address_count currently always is one.
 *
 *  supply_external
 *          The supply_external callback returns the TCP/IP address
 *      and port numbers as seen by the server end of the socket.  The
 *      server end can be the PXD demon or a server CCD instance.
 *
 *      Returned fields:
 *          client_opaque
 *          addresses
 *          address_count
 *
 *  connect_done
 *      Returned fields:
 *          client_opaque
 *          op_opaque
 *          result
 *          socket
 *          addresses
 *          address_count
 *
 *          The addresses array currently always has either zero or
 *      one entry. If the count is one, the addresses array contains
 *      the IP address and port of the server side socket for this
 *      proxy connection, as seen by the PXD server.  The addresses
 *      field will be null if malloc failed when trying to save the
 *      external address.  In this case, the address_count field will
 *      be zero.
 *
 *  lookup_done
 *      Returned fields:
 *          client_opaque
 *          op_opaque
 *          result
 *          region
 *          user_id
 *          device_id
 *          instance_id
 *          ans_dns
 *          pxd_dns
 *          address_count
 *          addresses
 *
 *  incoming_request
 *      Returned fields:
 *          op_opaque
 *          address_count
 *          addresses
 *
 *  incoming_login
 *      Returned fields:
 *          op_opaque
 *          socket
 *          addresses
 *          address_count
 *          blob (only for CCD-server-side operations)
 *
 *              The addresses array will contain the IP address and port as
 *              seen by the PXD server.  The address_count always will be 1.
 *
 *  reject_ccd_creds
 *      Returned fields:
 *          client_opaque
 *          op_opaque
 *          result
 *
 *  reject_pxd_creds
 *      Returned fields:
 *          client_opaque
 *          op_opaque
 *          
 */
typedef struct {
    pxd_cb_t supply_local;          /* for pxd_open              */
    pxd_cb_t supply_external;       /* for pxd_open              */
    pxd_cb_t connect_done;          /* for pxd_connect           */
    pxd_cb_t lookup_done;           /* for pxd_lookup            */
    pxd_cb_t incoming_request;      /* for pxd_receive           */
    pxd_cb_t incoming_login;        /* for pxd_receive logins    */
    pxd_cb_t reject_ccd_creds;      /* for pxd_connect           */
    pxd_cb_t reject_pxd_creds;      /* for PXD server operations */
} pxd_callback_t;

/*
 *  This struct defines the parameters passed to pxd_open, which is
 *  invoked to try to connect to a PD instance.
 */
typedef struct {
    char *            cluster_name; /* DNS name               */
    pxd_cred_t *      credentials;  /* from IAS               */
    int               is_incoming;  /* boolean                */
    pxd_callback_t *  callback;     /* for incoming requests  */
    void *            opaque;       /* passed to the callback */
} pxd_open_t;

/*
 *  Define the parameters to pxd_connect.
 */
typedef struct {
    pxd_id_t *     target;
    pxd_cred_t *   creds;
    char *         pxd_dns;
    void *         opaque;
    int64_t        request_id;    /* output for debugging */
    int            address_count;
    pxd_address_t  addresses[1];  /* variably sized       */
} pxd_connect_t;

/*
 *  pxd_receive also takes a lot of arguments.
 */
typedef struct {
    char         *   buffer;
    int              buffer_length;
    char *           server_key;          /* the CCD server key */
    int              server_key_length;
    pxd_client_t *   client;
    void *           opaque;
} pxd_receive_t;

/*
 *  This structure is used to pass parameters to pxd_login.
 *
 *  The following fields are passed to the callback:
 *
 *     op_opaque
 *     result
 *     socket
 *     unpacked blob (server side)
 */
typedef struct {
    VPLSocket_t    socket;
    void *         opaque;
    pxd_cb_t       callback;
    int            is_incoming;
    pxd_cred_t *   credentials;       /* used only on the client side */
    pxd_id_t *     server_id;
    void *         server_key;        /* used only on the server side */
    int            server_key_length;
    int64_t        login_id;          /* for logging */
} pxd_login_t;

/*
 *  This structure is used to return the registration information for
 *  a given CCD instance.
 */
typedef struct {
    char *         ans_dns;
    char *         pxd_dns;
    int            address_count;  /* the size of the addresses array */
    pxd_address_t  addresses[1];   /* variably-sized                  */
} pxd_declare_t;

/*
 *  Define the various return values.
 */
#define pxd_device_online          1
#define pxd_device_sleeping        2
#define pxd_device_offline         3
#define pxd_query_failed           4
#define pxd_op_successful          5
#define pxd_op_failed              6
#define pxd_timed_out              9
#define pxd_connection_lost       14
#define pxd_credentials_rejected  15
#define pxd_quota_exceeded        16

/*
 *  Service Routines
 *
 *         All major service routines are passed a pointer to a pxd_error_t
 *     structure so that they can return an error code and message in case
 *     of failure.
 *
 *  pxd_open
 *
 *         pxd_open is invoked to connect to an instance of the PD server. It
 *     returns a handle if successful.  A pd client handle can be either
 *     incoming or outgoing.  An incoming client handles incoming tunnel
 *     requests.  An outgoing client can be used for proxy connections.  Either
 *     type can be used for name lookup.
 *
 *  pxd_close
 *
 *         pxd_close is invoked to release the resources associated with a
 *     PD handle.
 *
 *  pxd_lookup
 *
 *         pxd_lookup returns the registration instance for a given CCD
 *     server instance.  The information includes IP addresses and ports,
 *     and the ANS and DNS server addresses.
 *
 *  pxd_connect
 *
 *         pxd_connect is used to create a proxy connection to a CCD instance.
 *     Once pxd_connect has completed successfully, the pxd_socket routine can
 *     be invoked to obtain the socket for the proxy connection.  Any bytes
 *     written to the socket will be transferred remote end uninterpreted.
 *     Likewise, any bytes written at the remote end can be read from the
 *     socket.  No further calls to pxd_lookup can be made using the given
 *     handle.
 *
 *         pxd_connect returns the connected socket to use to communicate with
 *     the remote end of the proxy connection.  It will return an error if
 *     pxd_connect has not completed successfully for this handle.
 *
 *  pxd_login
 *
 *         When a server CCD instances accepts a connection from a proxy client,
 *     it invoke pxd_login to perform the PXD login sequence on the connection.
 *     If the login attempt success, pxd_login invokes the callback given in
 *     the pxd_open call.  Otherwise, it puts an error code and message into
 *     the error structure passed to it.
 */
extern pxd_client_t *  pxd_open(pxd_open_t   *,                    pxd_error_t *);

extern void  pxd_close  (pxd_client_t  **, int,                    pxd_error_t *);
extern void  pxd_lookup (pxd_client_t  *,  pxd_id_t *,    void *,  pxd_error_t *);
extern void  pxd_connect(pxd_client_t  *,  pxd_connect_t *,        pxd_error_t *);
extern void  pxd_login  (pxd_login_t   *,                          pxd_error_t *);
extern void  pxd_declare(pxd_client_t  *,  pxd_declare_t *,        pxd_error_t *);
extern void  pxd_receive(pxd_receive_t *,                          pxd_error_t *);
extern int   pxd_verify (pxd_verify_t  *,                          pxd_error_t *);

/*
 *  pxd_string
 *
 *      pxd_string converts a PXD result code into a printable string.
 */
extern const char *  pxd_string(uint64_t);

/*
 *  Utility Routines
 *
 *  pxd_copy_creds
 *
 *         pxd_copy_cred is used to make a deep copy of a credentials structure.
 *
 *  pxd_copy_address
 *
 *         pxd_copy_address is used to make a deep copy of a address structure.
 *
 *  pxd_copy_id
 *
 *         pxd_copy_id is used to make a deep copy of an id structure.
 */
extern pxd_cred_t *     pxd_copy_creds  (pxd_cred_t    *);
extern pxd_address_t *  pxd_copy_address(pxd_address_t *);
extern pxd_id_t *       pxd_copy_id     (pxd_id_t      *);

/*
 *  These routines undo the copy routine's work.
 */
extern void  pxd_free_creds  (pxd_cred_t    **);
extern void  pxd_free_address(pxd_address_t **);
extern void  pxd_free_id     (pxd_id_t      **);

/*
 *  Declare this to get stupid compiler warnings to go away.
 */
extern void  pxd_stop_connection(pxd_client_t*, pxd_error_t*, int);

#ifdef __cplusplus
}
#endif

#endif /* PXD_CLIENT_H */
