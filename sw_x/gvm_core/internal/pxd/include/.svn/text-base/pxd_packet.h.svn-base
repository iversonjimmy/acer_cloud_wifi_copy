//
//  Copyright 2011-2013 Acer Cloud Technology
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#ifndef PXD_PACKET_H
#define PXD_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Packet Types
 *
 *      These must match PxdCommon.
 */
#define Reject_pxd_credentials       200
#define Send_pxd_login               201
#define Declare_server               202
#define Query_server_declaration     203
#define Send_server_declaration      204
#define Start_connection_attempt     205
#define Start_proxy_connection       206
#define Send_pxd_challenge           208
#define Send_pxd_response            209
#define Send_ccd_login               210
#define Send_ccd_challenge           211
#define Reject_ccd_credentials       212
#define Send_ccd_response            213
#define Set_pxd_configuration        226

/*
 *  Types for ANS messages sent to a server CCD instance.
 *  These values are coded into PxdCommon.java, so a change
 *  here means a change there.
 */
#define pxd_connect_request  1
#define pxd_wakeup           2

/*
 *  Values for the device states.  These must match PxdCommon.
 */
#define device_online    1
#define device_sleeping  2
#define device_offline   3
#define query_failed     4
#define op_successful    5
#define timed_out        9
#define connection_lost 14
#define op_failed       15

/*
 *  Cryptography is done using a pxd_crypto_t.  The key should be 20
 *  bytes in length. Shorter keys will be padded; longer keys will be
 *  truncated.  The seed is used to initialize a random number generator.
 *  Its value is not particularly critical.  The time in milliseconds
 *  should be close enough.
 */
typedef struct pxd_packet_s  pxd_packet_t;
typedef struct pxd_crypto_s  pxd_crypto_t;

typedef struct {
    int             from_unpack;
    int             type;
    int             count;
    void *          data;
    uint64_t        async_id;
    uint64_t        user_id;
    uint64_t        device_id;
    uint64_t        connection_id;
    uint64_t        connection_time;
    uint64_t        connection_tag;
    int             ioac_type;
    uint16_t        version;
    const char *    application;
    const char *    device_type;
    char *          host_name;

    uint64_t *      device_list;
    char *          device_states;
    uint64_t *      device_times;
    int             device_count;

    int32_t         sleep_port;
    int             sleep_packet_interval;

    uint64_t        rtt;
    uint64_t        out_sequence;
    uint64_t        response;
    uint64_t        send_time;
    uint64_t        new_device_state;
    uint64_t        new_device_time;
    uint64_t        request_id;

    char *          challenge;
    int             challenge_length;

    char *          blob;
    int             blob_length;

    char *          address;    /* an ip address */
    int             address_length;
    int32_t         port;       /* and its port  */

    char *          notification;
    int             notification_length;

    char *          mac_address;
    int             mac_address_length;

    char *          sleep_packet;
    int             sleep_packet_length;

    char *          sleep_dns;
    int             sleep_dns_length;

    char *          wakeup_key;
    int             wakeup_key_length;

    char *          region;
    char *          instance_id;
    char *          ans_dns;
    char *          pxd_dns;
    char *          server_instance;

    int             verbose;

    uint16_t        address_count;  // the length of the address array
    pxd_address_t * addresses;      // an array of addresses

    pxd_packet_t *  packet;         // for sync_io and friends
    pxd_blob_t *    unpacked_blob;  // for pxd_login

    /*
     *  Reconfigurable device parameters
     */
    uint32_t        proxy_retries;
    uint32_t        proxy_wait;
    uint32_t        idle_limit;
    uint32_t        sync_io_timeout;
    uint32_t        min_delay;
    uint32_t        max_delay;
    uint32_t        thread_retries;
    uint32_t        max_packet_size;
    uint32_t        max_encrypt;
    uint32_t        partial_timeout;
    uint32_t        reject_limit;
    uint32_t        extra;  /* for testing */
} pxd_unpacked_t;

/*
 *  Define a type for passing around a buffer, which is just a pointer
 *  and a length
 */
typedef struct pxd_io_s      pxd_io_t;

typedef void (*pxd_packet_cb_t)(pxd_io_t *, pxd_packet_t *, int);

struct pxd_packet_s {
    char *      base;
    char *      buffer;
    int         length;
    int         type;
    int         prepared;
    int         verbose;
    int64_t     tag;
    int64_t     sequence_id;
    int64_t     async_id;

    pxd_packet_cb_t  callback;
    pxd_io_t *       io;
    pxd_blob_t *     blob;
    pxd_packet_t *   next;
    const char *     where;
    void *           opaque;
};

typedef struct {
    void *   opaque;
    int64_t  async_id;
    int      type;
    int      valid;
    int64_t  start;
} pxd_command_t;

#define outstanding_limit 20

typedef void (*pxd_kill_t)(pxd_io_t *, pxd_command_t, int);

struct pxd_io_s {
    void *          opaque; /* for the client */
    VPLSocket_t     socket;
    int             verbose;
    int             stop_now;
    int             readable;
    int             ready_to_send;
    int64_t         time_limit;
    pxd_event_t *   event;
    pxd_crypto_t *  crypto;
    VPLMutex_t      mutex;

    pxd_packet_t *  queue_head;
    pxd_packet_t *  queue_tail;
    int64_t         idle_limit;
    int64_t         last_active;
    int64_t         tag;
    int64_t         out_sequence;
    int64_t         connection;
    int             timeouts_count;
    pxd_kill_t      kill;
    pxd_command_t   outstanding[outstanding_limit];
    pxd_command_t   timeouts   [outstanding_limit];
    char            host[128];
};

/*
 *  Exported routines 
 */
extern const char *      pxd_response         (uint64_t                                 );
extern const char *      pxd_packet_type      (int                                      );
extern const char *      pxd_state            (char                                     );
extern void              pxd_update_param     (int,            int *,     const char *  );
extern void              pxd_configure_packet (pxd_client_t *, pxd_unpacked_t *         );
extern pxd_crypto_t *    pxd_create_crypto    (const void *,   int,       uint32_t      );
extern void              pxd_free_crypto      (pxd_crypto_t **                          );
extern pxd_packet_t *    pxd_alloc_packet     (char *,         int,       void *        );
extern void              pxd_free_packet      (pxd_packet_t **                          );
extern void              pxd_prep_sync        (pxd_packet_t *, int64_t *, pxd_crypto_t *);
extern pxd_packet_t *    pxd_read_packet      (pxd_io_t *                               );
extern void              pxd_write_output     (pxd_io_t *                               );
extern pxd_packet_t *    pxd_pack             (pxd_unpacked_t *, void *,   pxd_error_t *);
extern uint32_t          pxd_random           (pxd_crypto_t *);
extern int               pxd_packet_limit     (void);
extern int               pxd_queue_packet     (pxd_io_t *, pxd_packet_t *, pxd_error_t *, const char *);
extern int               pxd_prequeue_packet  (pxd_io_t *, pxd_packet_t *, pxd_error_t *, const char *);
extern int               pxd_check_signature  (pxd_crypto_t *, pxd_packet_t *           );
extern void              pxd_kill_all         (pxd_io_t *,     int                      );
extern pxd_command_t     pxd_find_command     (pxd_io_t *,     pxd_unpacked_t *, int *  );
extern pxd_unpacked_t *  pxd_unpack           (pxd_crypto_t *, pxd_packet_t *, int64_t *, uint64_t);
extern pxd_unpacked_t *  pxd_unpack_ans       (char *,         int,        pxd_error_t *);
extern int               pxd_get_type         (pxd_packet_t *);
extern void              pxd_free_unpacked    (pxd_unpacked_t **                        );
extern void              pxd_reset_queue      (pxd_io_t *);
extern pxd_unpacked_t *  pxd_alloc_unpacked   (void      );
extern pxd_blob_t *      pxd_unpack_blob      (char *, int, pxd_crypto_t *, pxd_error_t *);
extern void              pxd_free_blob        (pxd_blob_t **);

extern int               pxd_encrypt          (pxd_crypto_t*, char**, int*, char*, const char*, int);
extern int               pxd_decrypt          (pxd_crypto_t*, char**, int*, const char*, int);
extern int               pxd_is_signed        (int);
extern void              pxd_sign_packet      (pxd_crypto_t*, char*, int);
extern void              pxd_prep_packet      (pxd_io_t*, pxd_packet_t*, int64_t*);
extern char *            pxd_pack_ans         (pxd_unpacked_t*, int*);
extern char *            pxd_pack_blob        (int*, pxd_blob_t*, pxd_crypto_t*, pxd_error_t*);
extern char *            pxd_pack_demon_blob  (int*, pxd_blob_t*, pxd_crypto_t*, pxd_error_t*);
extern pxd_blob_t *      pxd_unpack_demon_blob(char*, int, pxd_crypto_t*, pxd_error_t*);

#ifdef __cplusplus
}
#endif

#endif /* PXD_PACKET_H */
