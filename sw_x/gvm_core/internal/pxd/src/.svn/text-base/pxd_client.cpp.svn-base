//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology
//

#include <vplu.h>
#include <vpl_conv.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_user.h>
#include <vplex_assert.h>
#include <vplex_time.h>
#include <cslsha.h>
#include <aes.h>
#include <log.h>
#include <stdlib.h>

#ifndef IOS
#include <malloc.h>
#endif

#include <string.h>
#include <stdio.h>

#undef  true
#undef  false
#undef  null
#undef  min
#undef  max

#define  true    1
#define  false   0
#define  null    0

#define abs(x)         ((x) >= 0 ? (x) : -(x))
#define array_size(x)  (sizeof(x) / sizeof((x)[0]))
#define min(a, b)      ((a) < (b) ? (a) : (b))
#define max(a, b)      ((a) > (b) ? (a) : (b))
#define invalid(x)     (VPLSocket_Equal((x), VPLSOCKET_INVALID))
#define valid(x)       (!invalid(x))

#include <pxd_client.h>
#include <pxd_log.h>
#include <pxd_mtwist.h>
#include <pxd_util.h>
#include <pxd_event.h>
#include <pxd_packet.h>
#include <vpl_lazy_init.h>

#define open_magic 0x71720301

struct pxd_client_s {
    char *          cluster;
    char *          current_server;
    char *          region;
    int64_t         user_id;
    int64_t         device_id;
    char *          instance_id;
    pxd_cred_t *    creds;
    int             is_incoming;
    pxd_callback_t  callback;
    void *          opaque;
    int             network_changed;
    int64_t         next_async;
    mtwist_t        mt;
    int32_t         open;
    int             configured;
    int             delay;
    int             thread_done;
    int             close_requested;
    int             connect_login;
    void *          connect_opaque;
    VPLMutex_t      thread_lock;    // used when controlling the thread
    pxd_io_t        io;
    int             is_connect;
    pxd_cred_t *    connect_creds;
    pxd_id_t *      connect_target;

    pxd_address_t * external_address;

    int             is_proxy;
    uint64_t        proxy_user;
    uint64_t        proxy_device;
    char *          proxy_instance;
    uint64_t        proxy_request;
    uint64_t        proxy_tag;
    pxd_crypto_t *  blob_crypto;
    volatile int    is_detached;

    VPLDetachableThreadHandle_t  device_handle;
    volatile VPLNet_addr_t       server_ip;
};

typedef struct {
    VPLSocket_t     socket;
    int64_t         tag;
    int64_t         sequence;
    pxd_crypto_t *  crypto;
    int64_t         expected;
    pxd_packet_t *  packet;
    int64_t         id;
    int             result;
} pxd_read_t;

typedef struct {
    VPLSocket_t  socket;
    int64_t      tag;
    int64_t      sequence;
    void *       opaque;

    pxd_crypto_t *    crypto;
    pxd_unpacked_t    unpacked;
    pxd_cred_t *      creds;
    uint32_t          challenge[4];
    int64_t           connection_id;
    int64_t           id;
} pxd_write_t;

/*
 *  Configurable Parameters
 */
static int  pxd_proxy_retries     =   3;
static int  pxd_proxy_wait        =  10;        /* seconds */
static int  pxd_idle_limit        =  20;        /* seconds */
static int  pxd_sync_io_timeout   =   6;        /* seconds */
static int  pxd_min_delay         =   2;        /* seconds */
static int  pxd_max_delay         =  10 * 60;   /* seconds */
static int  pxd_thread_retries    =  10;
static int  pxd_reject_limit      =   5;        /* seconds */

/*
 *  Other values
 */
static int  pxd_tcp_port          = 16956;
static int  pxd_outgoing_id       =   1;

static char pxd_invalid_dns[]     = "pxd_invalid_cluster";

static int  pxd_connect_errors    =   0;  /* for testing */
static int  pxd_shutdowns         =   0;  /* for testing */
static int  pxd_proxy_shutdowns   =   0;  /* for testing */
static int  pxd_proxy_failures    =   0;  /* for testing */
static int  pxd_force_socket      =   0;  /* for testing */
static int  pxd_force_change      =   0;  /* for testing */
static int  pxd_force_time        =   0;  /* for testing */
static int  pxd_backwards_time    =   0;  /* for testing */
static int  pxd_declare_failures  =   0;  /* for testing */
static int  pxd_bad_signatures    =   0;  /* for testing */
static int  pxd_bad_blobs         =   0;  /* for testing */
static int  pxd_process_unpack    =   0;  /* for testing */
static int  pxd_not_found         =   0;  /* for testing */
static int  pxd_bad_commands      =   0;  /* for testing */
static int  pxd_login_crypto      =   0;  /* for testing */
static int  pxd_down_events       =   0;  /* for testing */
static int  pxd_bad_challenges    =   0;  /* for testing */
static int  pxd_bad_login_sends   =   0;  /* for testing */
static int  pxd_bad_crypto_setups =   0;  /* for testing */
static int  pxd_bad_mac_setups    =   0;  /* for testing */
static int  pxd_wrong_id          =   0;  /* for testing */
static int  pxd_failed_proxy      =   0;  /* for testing */
static int  pxd_close_pending     =   0;  /* for testing */
static int  pxd_wrong_instance    =   0;  /* for testing */
static int  pxd_verbose           = true; /* for testing */

/*
 *  Define our request id generation variables.  We use this
 *  to generate ids for pxd_login operations, too.
 */
static int64_t  pxd_connect_id      =  1;
static VPLLazyInitMutex_t pxd_connect_lock = VPLLAZYINITMUTEX_INIT;

/*
 *  Generate a unique request id for a pxd_connect attempt.
 *  This id needs to be unique per:
 *    <client CCD user id, client CCD device id>
 *  tuple, so it needs to be a global (CCD-wide) lock and serial number.
 *
 *  I suppose I could generate a random 64-bit integer, too, but I don't
 *  have a particularly convincing way of initializing a random number
 *  generator for each pxd_client thread, either.
 */
static int64_t
pxd_next_id(void)
{
    VPLMutex_t *  lock;
    int64_t       result;

    lock = VPLLazyInitMutex_GetMutex(&pxd_connect_lock);

    pxd_mutex_lock(lock);
    result = pxd_connect_id++;
    pxd_mutex_unlock(lock);

    return result;
}

/*
 *  Set the "reuse address" flag in the socket, if possible.
 *  The tunnel code tries to use the socket address for
 *  further connections.
 */
static int
set_reuse(VPLSocket_t socket)
{
    int  result;
    int  value;

    value  = 1;
    result = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET,
                VPLSOCKET_SO_REUSEADDR, &value, sizeof(value));

#ifdef IOS
    if(result != VPL_OK) {
        return result;
    }

    result = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET,
                VPLSOCKET_SO_REUSEPORT, &value, sizeof(value));

#endif

    return result;
}

/*
 *  Discard the packets in the outgoing TCP queue.
 */
static void
discard_tcp_queue(pxd_client_t *client)
{
    pxd_packet_t *  packet;
    pxd_packet_t *  next;

    pxd_mutex_lock(&client->io.mutex);
    packet = client->io.queue_head;

    while (packet != null) {
        next = packet->next;
        pxd_free_packet(&packet);
        packet = next;
    }

    client->io.queue_head = null;
    client->io.queue_tail = null;

    pxd_mutex_unlock(&client->io.mutex);
}

static void
free_memory(pxd_client_t **clientp)
{
    pxd_client_t *  client;

    client = *clientp;

    pxd_free_creds  (&client->creds           );
    pxd_free_creds  (&client->connect_creds   );
    pxd_free_crypto (&client->blob_crypto     );
    pxd_free_id     (&client->connect_target  );
    pxd_free_event  (&client->io.event        );
    pxd_free_crypto (&client->io.crypto       );
    pxd_free_address(&client->external_address);

    if (client->io.queue_head != null) {
        discard_tcp_queue(client);
    }

    free(client->cluster       );
    free(client->region        );
    free(client->instance_id   );
    free(client->proxy_instance);

    free(client);

    *clientp = null;
}

static void
make_locks(pxd_client_t *client)
{
    pxd_mutex_init(&client->io.mutex);
    pxd_mutex_init(&client->thread_lock);
}

static void
destroy_locks(pxd_client_t *client)
{
    VPLMutex_Destroy(&client->io.mutex);
    VPLMutex_Destroy(&client->thread_lock);
}

static void
fail_open(pxd_client_t *client, int made_locks)
{
    if (made_locks) {
        destroy_locks(client);
    }

    free_memory(&client);
}

/*
 *  Close a socket in an idempotent way.
 */
static void
close_socket(VPLSocket_t *socket)
{
    if (valid(*socket)) {
        VPLSocket_Close(*socket);
        *socket = VPLSOCKET_INVALID;
    }
}

/*
 *  Sleep for a given period of time, or until an event
 *  occurs.  While sleeping, we need to write output to
 *  the socket, if there's anything queued, so we don't
 *  actually get much rest.
 */
static void
sleep_seconds(pxd_client_t *client, int seconds)
{
    VPLTime_t  start_time;
    VPLTime_t  current_time;
    VPLTime_t  timeout;
    int64_t    new_time_waited;
    int64_t    requested_wait;
    int64_t    time_waited;
    int        result;
    int        poll_count;
    int        failed;

    VPLSocket_poll_t  poll[2];

    if (seconds <= 0) {
        return;
    }

    /*
     *  Get the starting time.  We use the actual time of day, in
     *  case the device sleeps.  That means we need to watch for
     *  time going backwards, though.
     */
    start_time      = VPLTime_GetTime();
    time_waited     = 0;
    requested_wait  = VPLTime_FromSec(seconds);
    failed          = invalid(client->io.socket);

    do {
        poll[0].socket   = pxd_socket(client->io.event);
        poll[0].events   = VPLSOCKET_POLL_RDNORM;

        poll[1].socket   = client->io.socket;
        poll[1].events   = 0;

        /*
         *  If there's anything queued for output, and the TCP socket
         *  is valid, be sure to poll on it.
         */
        if (!failed && client->io.queue_head != null && client->io.ready_to_send) {
            poll[1].events = VPLSOCKET_POLL_OUT;
            poll_count = 2;
        } else {
            poll[1].events = 0;
            poll_count = 1;
        }

        poll[0].revents  = 0;
        poll[1].revents  = 0;

        timeout = requested_wait - time_waited;
        result  = VPLSocket_Poll(poll, poll_count, timeout);

        pxd_check_event(client->io.event, result, &poll[0]);

        failed = poll_count > 1 && (poll[1].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP));

        /*
         *  Try to compute the wall-clock time that we've waited, in case the
         *  device sleeps.  If time seems to move backward, just give up
         *  and return.
         */
        current_time = VPLTime_GetTime();

        if (current_time >= start_time && --pxd_force_time != 0) {
            new_time_waited  = current_time - start_time;

            /*
             *  Check for overflow...  Exit the wait loop if there's
             *  a problem.
             */
            if (new_time_waited < 0) {
                log_info("wait time overflow:  " FMTu64 " to " FMTu64,
                    (uint64_t) start_time,
                    (uint64_t) current_time);
                time_waited = requested_wait;
            } else {
                time_waited = new_time_waited;
            }
        } else {
            log_info("time backed up:  " FMTu64 " to " FMTu64,
                (uint64_t) start_time, (uint64_t) current_time);
            pxd_backwards_time++;
            time_waited  = requested_wait;
        }

        /*
         *  Write any output we can, if there's anything queued.
         */
        pxd_write_output(&client->io);
    } while
    (
        !client->io.stop_now
    &&  !client->network_changed
    &&  time_waited < requested_wait
    );
}

/*
 *  Open a TCP connection to a PXD server.  Don't return until the
 *  connection is valid or we're told to shut down.
 */
static VPLSocket_t
sync_open_socket(pxd_client_t *client)
{
    int   delay;
    int   error;
    char  dotted_quad[256];
    int   jitter;
    int   tries;
    int   limit;

    VPLSocket_addr_t  server_address;
    VPLNet_addr_t     net_address;
    VPLSocket_t       socket;

    log_always("client %p is starting a connection attempt to %s:%d",
        client, client->cluster, pxd_tcp_port);

    socket       = VPLSOCKET_INVALID;
    net_address  = VPLNET_ADDR_INVALID;
    delay        = pxd_min_delay;
    limit        = client->is_proxy ? pxd_proxy_retries : 0;
    tries        = 0;

    dotted_quad[0] = 0;

    /*
     *  Keep looping until we're asked to shut down or we manage
     *  to connect.
     */
    while (!client->io.stop_now && invalid(socket)) {
        if (limit > 0 && tries++ >= limit) {
            pxd_proxy_failures++;
            client->io.stop_now = true;
            log_info("client %p, request " FMTs64 " cannot connect for a proxy attempt",
                client, client->proxy_request);
            return VPLSOCKET_INVALID;
        }

        if (client->io.verbose && delay < pxd_max_delay) {
            log_info("client %p is attempting to connect to %s:%d",
                client, client->cluster, pxd_tcp_port);
        }

        /*
         *  Try to create a TCP socket.
         */
        socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);

        if (invalid(socket) || --pxd_force_socket == 0) {
            if (client->io.verbose && delay < pxd_max_delay) {
                log_warn("client %p failed to create a socket.", client);
            }

            close_socket(&socket);
        }

        set_reuse(socket);

        /*
         *  If we got a valid socket, get the network address of a PXD
         *  server.
         */
        if (valid(socket)) {
            net_address = VPLNet_GetAddr(client->cluster);

            if (net_address == VPLNET_ADDR_INVALID || --pxd_force_socket == 0) {
                if (client->io.verbose && delay < pxd_max_delay) {
                    log_warn("client %p failed to get an address for the server.",
                        client);
                }

                close_socket(&socket);
            } else {
                pxd_convert_address(net_address, dotted_quad, sizeof(dotted_quad));
            }
        }

        /*
         *  Okay, we have a socket and an address, so try to connect.
         */
        if (valid(socket)) {
            client->server_ip      = net_address;
            server_address.addr    = net_address;
            server_address.port    = VPLConv_ntoh_u16(pxd_tcp_port);
            server_address.family  = VPL_PF_INET;

            error = VPLSocket_Connect(socket, &server_address, sizeof(server_address));

            if (error != VPL_OK || --pxd_force_socket == 0) {
                if (client->io.verbose && delay < pxd_max_delay) {
                    log_warn("client %p:  The connection attempt to %s(%s):%d failed:  %d (%s)",
                        client, client->cluster, dotted_quad, pxd_tcp_port, error,
                        error == VPL_ERR_TIMEOUT ?  "timeout" : "unknown");
                }

                close_socket(&socket);
            }
        }

        /*
         *  If the connect succeeded, declare success.
         */
        if (valid(socket)) {
            client->io.readable = true;
            log_info("client %p is connected to %s(%s):%d",
                client, client->cluster, dotted_quad, pxd_tcp_port);
        }

        /*
         *  Okay, if we didn't manage to connect, delay as appropriate,
         *  then try to connect again.
         */
        if (!valid(socket)) {
            pxd_connect_errors++;

            /*
             *  If the network setup changes, try to connect right away.
             *  Otherwise, use the normal backoff algorithm.
             */
            if (client->network_changed || pxd_force_change) {
                client->network_changed  = VPL_FALSE;
                pxd_force_change         = false;
                delay                    = pxd_min_delay;
            } else {
                if (client->mt.inited) {
                    jitter  = delay / 4;
                    jitter  = mtwist_next(&client->mt, jitter);
                    delay  += jitter;
                }

                if (client->io.verbose && delay < pxd_max_delay) {
                    log_info("client %p will try again in %ds.", client, delay);
                }

                sleep_seconds(client, delay);
                delay = MIN(delay * 2, pxd_max_delay);
            }
        }
    }

    return socket;
}

/*
 *  deprep_tcp_queue - mark all packets as needed serial numbers
 *
 *  When a connection breaks, any packets that have serial numbers
 *  need to be given new ones, so mark them unprepared.  The caller
 *  must hold the client mutex.
 */
static void
deprep_tcp_queue(pxd_client_t *client)
{
    pxd_packet_t *  current;

    pxd_mutex_lock(&client->io.mutex);

    current = client->io.queue_head;

    while (current != null) {
        current->prepared = false;
        current = current->next;
    }

    pxd_mutex_unlock(&client->io.mutex);
}

/*
 *  Do the processing for a command that has been killed,
 *  likely due to a timeout or a lost connection.
 */
static void
pxd_kill(pxd_io_t *io, pxd_command_t command, int result)
{
    pxd_client_t *  client;
    pxd_cb_data_t   data;

    client = (pxd_client_t *) io->opaque;

    memset(&data, 0, sizeof(data));

    data.client_opaque  = client->opaque;
    data.op_opaque      = command.opaque;
    data.result         = result;

    if (command.type == Query_server_declaration) {
        client->callback.lookup_done(&data);
    }
}

/*
 *  Declare the TCP connection to the server
 *  down, and prepare for the reconnect.
 */
static void
declare_down(pxd_client_t *client, const char *where)
{
    pxd_down_events++;

    log_warn("connection " FMTs64 "@%s:  %s (device " FMTu64 ", cluster %s)",
        client->io.connection,
        client->io.host,
        where,
        client->device_id,
        client->cluster);

    close_socket(&client->io.socket);
    deprep_tcp_queue(client);
    pxd_kill_all(&client->io, pxd_connection_lost);

    client->io.tag            = 0;
    client->io.timeouts_count = 0;
    client->io.out_sequence   = 0;
    client->io.ready_to_send  = false;
    client->io.connection     = 0;
    client->configured        = false;

    strncpy(client->io.host, "<unconnected>", sizeof(client->io.host));

    if (!client->io.stop_now) {
        /*
         *  Check the network changed flag, and set a very
         *  short wait if the network configuration has changed.
         */
        client->delay = MIN(client->delay * 2, pxd_max_delay);

        if (client->io.verbose) {
            log_info("client %p will try again in %ds.", client, client->delay);
        }

        /*
         *  Delay before attempting to connect.
         */
        sleep_seconds(client, client->delay);
    }
}

/*
 *  Tell the client thread to shut down, if it's running, and
 *  close any connection to the server
 */
void
pxd_stop_connection(pxd_client_t *client, pxd_error_t *error, int sync)
{
    int  tries;
    int  pass;

    tries = 0;

    pxd_mutex_lock(&client->thread_lock);
    pxd_mutex_lock(&client->io.mutex);

    if (!client->io.stop_now) {
        VPLDetachableThread_Detach(&client->device_handle);
    }

    client->io.stop_now = true;
    pxd_ping(client->io.event, "stop_connection");

    if (!sync) {
        pxd_mutex_unlock(&client->io.mutex);
        pxd_mutex_unlock(&client->thread_lock);
        return;
    }

    do {
        pxd_mutex_unlock(&client->io.mutex);
        VPLThread_Sleep(VPLTime_FromMillisec(200));
        pxd_mutex_lock(&client->io.mutex);
    } while (tries++ < pxd_thread_retries && !client->thread_done);

    pass = client->thread_done;

    if (!pass) {
        error->error    = VPL_ERR_TIMEOUT;
        error->message  = "The service thread wouldn't stop.";
    }

    pxd_mutex_unlock(&client->thread_lock);
    pxd_mutex_unlock(&client->io.mutex);
}

/*
 *  Do the processing that's needed after a proxy connection
 *  has been established.  This code does the socket hand-off
 *  to the tunnel services, and tells the PXD client thread
 *  that it's time to exit.
 */
static void
pxd_login_cb(pxd_io_t *io, pxd_packet_t *packet, int sent)
{
    pxd_cb_data_t   data;
    pxd_client_t *  client;

    if (sent) {
        client = (pxd_client_t *) io->opaque;
        log_info("connection " FMTs64 "@%s completed a server-side ccd login for user "
            FMTs64 ", device " FMTs64 ", instance %s",
            client->io.connection,
            client->io.host,
            client->user_id,
            client->device_id,
            client->instance_id);

        memset(&data, 0, sizeof(data));

        data.op_opaque      = client->opaque;
        data.blob           = packet->blob;
        data.socket         = client->io.socket;
        data.result         = pxd_op_successful;
        data.addresses      = client->external_address;
        data.address_count  = 1;
        client->io.socket   = VPLSOCKET_INVALID;
        client->io.stop_now = true; // tell the thread to exit

        client->callback.incoming_login(&data);
    }

    pxd_free_blob(&packet->blob);
}

/*
 *  Do some initialization of a packet request structure.  Set the
 *  tag field and generate a unique operation id.
 */
static void
setup_unpacked(pxd_client_t *client, pxd_unpacked_t *unpacked)
{
    pxd_mutex_lock(&client->io.mutex);

    unpacked->connection_tag  = client->io.tag;
    unpacked->async_id        = client->next_async++;

    pxd_mutex_unlock(&client->io.mutex);
}

/*
 *  Send a packet to the PXD server of the given type.
 */
static int
send_command
(
    pxd_client_t *    client,
    pxd_unpacked_t *  unpacked,
    int               prequeue,
    pxd_blob_t *      blob,
    pxd_error_t *     error,
    const char *      where
)
{
    pxd_packet_t *  packet;

    int      success;

    /*
     *  Some packet types need to be prepped here a bit.
     */
    switch (unpacked->type)
    {
    case Send_ccd_challenge:
    case Send_ccd_response:
    case Send_ccd_login:
    case Reject_ccd_credentials:
        break;

    default:
        setup_unpacked(client, unpacked);
    }

    /*
     *  Get the wire format.
     */
    packet = pxd_pack(unpacked, null, error);

    pxd_free_unpacked(&unpacked);

    if (packet == null) {
        log_warn("client %p:  send_command (%s) failed in pack:  %s",
            client,
            where,
            error->message);
        return false;
    }

    packet->verbose = pxd_verbose | client->io.verbose;

    /*
     *  If there's a blob, this command is completing a
     *  proxy login, so set the callback appropriately.
     */
    if (blob != null) {
        packet->callback = pxd_login_cb;
        packet->blob     = blob;
        packet->io       = &client->io;
    }

    /*
     *  The Send_pxd_login packet needs to go to the front of the queue.
     *  Some others might, too.
     */
    if (prequeue) {
        success = pxd_prequeue_packet(&client->io, packet, error, where);
    } else {
        success = pxd_queue_packet   (&client->io, packet, error, where);
    }

    if (!success) {
        pxd_free_packet(&packet);
        log_warn("client %p:  send_command (%s) failed to enqueue:  %s",
            client,
            where,
            error->message);
    }

    return success;
}

/*
 *  Send a login packet of the given type, either Send_pxd_login
 *  or Send_ccd_login.
 */
static int
send_login(pxd_client_t *client, int type, pxd_unpacked_t *input)
{
    pxd_unpacked_t *  unpacked;
    pxd_error_t       error;

    unpacked = pxd_alloc_unpacked();

    if (unpacked == null) {
        log_warn_1("send_login failed in alloc");
        return false;
    }

    unpacked->type             =          type;
    unpacked->connection_tag   =          client->io.tag;
    unpacked->challenge        =          input->challenge;
    unpacked->challenge_length =          input->challenge_length;
    unpacked->blob             = (char *) client->creds->opaque;
    unpacked->blob_length      =          client->creds->opaque_length;
    unpacked->pxd_dns          =          client->cluster;

    return send_command(client, unpacked, true, null, &error, "send_login");
}

/*
 *  Do some client cleanup after a connect attempt fails.
 */
static void
connect_cleanup(pxd_client_t *client)
{
    log_info("client %p is cleaning up after a failed connection attempt.",
        client);
    pxd_mutex_lock(&client->io.mutex);
    pxd_free_creds(&client->connect_creds);
    pxd_free_id   (&client->connect_target);

    client->is_connect      = false;
    client->connect_login   = false;
    client->connect_opaque  = null;

    pxd_mutex_unlock(&client->io.mutex);
}

/*
 *  Build and send the CCD challenge packet for a proxy connection
 *  attempt.
 */
static int
send_ccd_challenge(pxd_client_t *client)
{
    uint32_t          challenge[4];
    pxd_unpacked_t *  unpacked;
    VPLNet_addr_t     client_address;
    VPLNet_port_t     client_port;
    pxd_error_t       error;

    /*
     *  Get the values we need for the packet.
     */
    client_address = VPLSocket_GetPeerAddr(client->io.socket);
    client_port    = VPLSocket_GetPeerPort(client->io.socket);
    client_port    = VPLConv_ntoh_u16(client_port);

    for (int i = 0; i < array_size(challenge); i++) {
        challenge[i] = mtwist_rand(&client->mt);
    }

    /*
     *  Okay, build the packet.
     */
    unpacked = pxd_alloc_unpacked();

    if (unpacked == null) {
        return false;
    }

    unpacked->type             =          Send_ccd_challenge;
    unpacked->challenge        = (char *) challenge;
    unpacked->pxd_dns          =          client->cluster;
    unpacked->challenge_length =          sizeof(challenge);
    unpacked->connection_time  =          VPLTime_GetTime() / VPLTime_FromMillisec(1);
    unpacked->connection_id    =          1;
    unpacked->address          = (char *) &client_address;
    unpacked->address_length   =          sizeof(client_address);
    unpacked->port             =          client_port;

    /*
     *  Save the connection tag.  We can set it only after this
     *  packet has been sent, so do that in process_packet.
     */
    client->proxy_tag = unpacked->connection_time;

    return send_command(client, unpacked, false, null, &error, "send_ccd_challenge");
}

static int
send_start_proxy(pxd_client_t *client)
{
    pxd_unpacked_t *  unpacked;
    pxd_error_t       error;

    int  pass;

    unpacked = pxd_alloc_unpacked();

    if (unpacked == null) {
        return false;
    }

    unpacked->type        = Start_proxy_connection;
    unpacked->user_id     = client->proxy_user;
    unpacked->device_id   = client->proxy_device;
    unpacked->instance_id = client->proxy_instance;
    unpacked->request_id  = client->proxy_request;

    pass = send_command(client, unpacked, false, null, &error, "send_start_proxy");
    free(client->proxy_instance);
    client->proxy_instance = null;
    return pass;
}

/*
 *  Send a Send_ccd_response packet needed for a proxy operation.
 */
static int
send_ccd_response
(
    pxd_client_t *    client,
    pxd_unpacked_t *  input,
    pxd_blob_t *      blob,
    int64_t           response
)
{
    pxd_unpacked_t *  unpacked;
    pxd_error_t       error;

    unpacked = pxd_alloc_unpacked();

    unpacked->type            = Send_ccd_response;
    unpacked->response        = response;
    unpacked->address         = input->address;
    unpacked->address_length  = input->address_length;
    unpacked->port            = input->port;

    return send_command(client, unpacked, false, blob, &error, "send_ccd_response");
}

static int
send_reject_ccd(pxd_client_t *client, int64_t async_id)
{
    pxd_unpacked_t *  unpacked;
    pxd_error_t       error;

    unpacked = pxd_alloc_unpacked();

    unpacked->type      = Reject_ccd_credentials;
    unpacked->async_id  = async_id;

    return send_command(client, unpacked, false, null, &error, "send_reject_ccd");
}

/*
 *  Check a login request for a proxy operation.  We have an incoming
 *  login packet from a CCD instance.
 */
static int
process_login(pxd_client_t *client, pxd_unpacked_t *unpacked, pxd_cb_data_t *data)
{
    pxd_blob_t *  blob;
    pxd_error_t   error;
    uint32_t      seed;

    blob = pxd_unpack_blob(unpacked->blob, unpacked->blob_length, client->blob_crypto, &error);

    if (blob == null) {
        log_info("connection " FMTs64 "@%s:  The blob is invalid:  %s",
            client->io.connection,
            client->io.host,
            error.message);
        pxd_bad_blobs++;
        return false;
    }

    if
    (
        blob->server_user   != client->user_id
    ||  blob->server_device != client->device_id
    ||  strcmp(blob->server_instance, client->instance_id) != 0
    ) {
        log_info("connection " FMTs64 "@%s:  The user, device, or instance id was invalid.",
            client->io.connection,
            client->io.host);
        pxd_free_blob(&blob);
        return false;
    }

    seed = mtwist_rand(&client->mt);

    pxd_free_crypto(&client->io.crypto);
    client->io.crypto = pxd_create_crypto(blob->key, blob->key_length, seed);

    if (client->io.crypto == null) {
        log_warn_1("malloc failed in process_login");
        pxd_free_blob(&blob);
        return false;
    }

    data->blob = blob;
    return true;
}

static void
pxd_configure_client(pxd_client_t *client, pxd_unpacked_t *unpacked)
{
    pxd_update_param(unpacked->proxy_retries  , &pxd_proxy_retries  , "proxy retries"    );
    pxd_update_param(unpacked->proxy_wait     , &pxd_proxy_wait     , "proxy wait"       );
    pxd_update_param(unpacked->idle_limit     , &pxd_idle_limit     , "idle limit"       );
    pxd_update_param(unpacked->sync_io_timeout, &pxd_sync_io_timeout, "sync I/O limit"   );
    pxd_update_param(unpacked->min_delay      , &pxd_min_delay      , "min connect delay");
    pxd_update_param(unpacked->max_delay      , &pxd_max_delay      , "max connect delay");
    pxd_update_param(unpacked->thread_retries , &pxd_thread_retries , "thread retries"   );
    pxd_update_param(unpacked->reject_limit   , &pxd_reject_limit   , "reject limit"     );
}

static void
pxd_supply_local(pxd_client_t *client)
{
    VPLNet_addr_t  ip_address;
    int            port;
    pxd_cb_data_t  data;
    pxd_address_t  address;

    ip_address  = VPLSocket_GetAddr(client->io.socket);
    port        = VPLSocket_GetPort(client->io.socket);
    port        = VPLConv_ntoh_u16(port);

    memset(&data, 0, sizeof(data));

    data.client_opaque  = client->opaque;
    data.addresses      = &address;
    data.address_count  = 1;
    address.ip_address  = (char *) &ip_address;
    address.ip_length   = sizeof(ip_address);
    address.port        = port;
    address.type        = 0;

    client->callback.supply_local(&data);
}

/*
 *  Process a packet received from the PXD server.  The packet might
 *  be forwarded by the PXD server from a CCD instance to us.
 */
static int
process_packet(pxd_client_t *client, pxd_packet_t *packet, int64_t *sequence)
{
    pxd_unpacked_t *  unpacked;
    pxd_cb_data_t     data;
    pxd_command_t     command;
    pxd_cred_t *      creds;
    pxd_address_t     address;

    int       found;
    int       pass;
    int       type;
    uint32_t  seed;

    type = pxd_get_type(packet);

    /*
     *  If we are a client CCD doing a CCD login sequence, then
     *  we must treat Send_ccd_challenge packets specially.  We
     *  need to switch to the new encryption key and restart the
     *  sequence number.
     */
    if (client->connect_login && type == Send_ccd_challenge) {
        pxd_free_crypto(&client->io.crypto);

        *sequence          = 0;
        client->io.tag     = 0;
        creds              = client->connect_creds;
        seed               = mtwist_rand(&client->mt) ^ VPLTime_GetTime();
        client->io.crypto  = pxd_create_crypto(creds->key, creds->key_length, seed);

        /*
         *  We can't tolerate an allocation failure here and keep the
         *  connection.
         */
        if (client->io.crypto == null) {
            pxd_login_crypto++;
            log_warn_1("malloc failed during ccd login.");
            return false;
        }
    }

    /*
     *  If we're doing a CCD login, we need to switch to the new tag for this
     *  packet stream.
     */
    if (type == Send_ccd_login) {
        client->io.tag = client->proxy_tag;
    }

    /*
     *  Unpack the packet.
     */
    unpacked = pxd_unpack(client->io.crypto, packet, sequence, client->io.tag);

    if (unpacked == null) {
        log_test("connection " FMTs64 "@%s:  unpack failed",
            client->io.connection,
            client->io.host);
        pxd_process_unpack++;
        return false;
    }

    log_test("connection " FMTs64 "@%s:  processing packet type %s (%d), sequence " FMTu64,
              client->io.connection,
              client->io.host,
              pxd_packet_type(type),
        (int) type,
              unpacked->out_sequence);

    /*
     *  If this packet is an incoming login, try to unpack the blob.  If the
     *  blob or signature is bad, reject the login and send a notice back to
     *  the caller.  Note that a bad signature likely indicates that the packet
     *  was corrupted in transit.  It's possible that the CCD client is using
     *  the wrong key, but statistically, a corrupted packet had better be a
     *  lot more likely.
     *
     *  If this packet is not an incoming login, check the signature if there
     *  is one.  If we got a blob, we now have the new cryptography key in
     *  action.
     */
    memset(&data, 0, sizeof(data));

    if (type == Send_ccd_login) {
        pass = process_login(client, unpacked, &data);

        if (!pass || !pxd_check_signature(client->io.crypto, packet)) {
            client->io.readable    = false;
            client->io.idle_limit  = VPLTime_FromSec(pxd_reject_limit);

            send_reject_ccd(client, unpacked->async_id);
            pxd_free_unpacked(&unpacked);
            return true;
        }
    } else if (!pxd_check_signature(client->io.crypto, packet)) {
        pxd_bad_signatures++;
        type = pxd_get_type(packet);

        log_warn("connection " FMTs64 "@%s:  The packet signature is invalid:  type %s (%d)",
            client->io.connection, client->io.host,
            pxd_packet_type(type), (int) type);

        pxd_free_unpacked(&unpacked);
        return false;
    }

    /*
     *  Get the command entry from the table, if we're expecting one.
     *  Incoming challenge packets are spontaneous, so there's no
     *  command.  Outgoing challenges will be in the table.  In that
     *  case, we're a server CCD instance.
     */
    memset(&command, 0, sizeof(command));
    command.type = unpacked->type;

    if (unpacked->type == Send_ccd_challenge && client->connect_login) {
        found = true;
    } else if (unpacked->type == Set_pxd_configuration) {
        found = true;
    } else {
        command = pxd_find_command(&client->io, unpacked, &found);
    }

    /*
     *  If we don't find the outgoing request corresponding to this packet,
     *  just return.
     */
    if (!found) {
        pxd_not_found++;
        log_warn("connection " FMTs64 "@%s:  packet type %s (%d), op id " FMTs64 " was not expected!",
                  client->io.connection, client->io.host,
                  pxd_packet_type(unpacked->type),
            (int) unpacked->type,
                  unpacked->async_id);

        pxd_free_unpacked(&unpacked);
        pxd_free_blob    (&data.blob);
        return true;
    }

    log_info("connection " FMTs64 "@%s received  %s (%d), op id " FMTs64,
              client->io.connection,
              client->io.host,
              pxd_packet_type(unpacked->type),
        (int) unpacked->type,
              unpacked->async_id);


    data.client_opaque  = client->opaque;
    data.op_opaque      = command.opaque;

    /*
     *  Send_pxd_response packets can indicate the failure
     *  of a command.  Other packets types correspond to
     *  success.
     */
    if
    (
        unpacked->type == Send_pxd_response
    ||  unpacked->type == Send_ccd_response
    ) {
        data.result = unpacked->response;
    } else {
        data.result = pxd_op_successful;
    }

    pass = true;

    /*
     *  Switch on the command type.  Usually, this thread is
     *  receiving a response to an operation that it started,
     *  so the command type is not the incoming packet.
     *  Challenge packets are spontanous, so they are an
     *  exception.
     */
    switch (command.type) {
    /*
     *  We are doing a CCD login, either from the CCD server or
     *  CCD client side.  If we're on the client CCD side, this
     *  thread is processing a pxd_connect operation, so send the
     *  login packet now that we have received the challenge.
     *
     *  For the CCD server side, we just received the Send_ccd_login
     *  packet and processed it successfully by calling process_login,
     *  so here we just need to send a positive response.
     */
    case Send_ccd_challenge:
        if (client->connect_login) {
            pxd_free_creds(&client->creds);
            client->io.tag           = unpacked->connection_time;
            client->io.out_sequence  = 0;
            client->creds            = client->connect_creds;
            client->connect_creds    = null;

            pass = send_login(client, Send_ccd_login, unpacked);
        } else {
            client->io.readable = false;
            client->io.tag      = client->proxy_tag;

            if (client->external_address != null) {
                unpacked->address        = client->external_address->ip_address;
                unpacked->address_length = client->external_address->ip_length;
                unpacked->port           = client->external_address->port;
            }

            pass = send_ccd_response(client, unpacked, data.blob, pxd_op_successful);
        }

        break;

    /*
     *  A login to a PXD server has completed.  If this client
     *  is doing a proxy login, start processing it.  Otherwise,
     *  this client was started by CCD and we need to pass the
     *  external address back.
     */
    case Send_pxd_login:
        if (unpacked->type == Reject_pxd_credentials) {
            data.result = pxd_op_failed;

            if (!client->is_proxy) {
                client->callback.reject_pxd_creds(&data);
            }

            pass = false;
        }

        if (pass && client->is_proxy) {
            pass = send_start_proxy(client);
        } else if (pass) {
            pxd_supply_local(client);

            data.addresses     = client->external_address;
            data.address_count = 1;

            client->callback.supply_external(&data);
        }

        break;

    /*
     *  A CCD login for a pxd_connect operation has completed.  Generally,
     *  the server should reject the credentials or send a successful
     *  response, but check the result code anyway.
     */
    case Send_ccd_login:
        data.op_opaque = client->connect_opaque;
        client->connect_opaque = null;  // record that the callback is done.

        if (unpacked->type == Reject_ccd_credentials) {
            log_info("connection " FMTs64 "@%s:  My CCD credentials were rejected.",
                client->io.connection, client->io.host);
            data.result = pxd_op_failed;
            connect_cleanup(client);
            pxd_free_crypto(&client->io.crypto);
            client->callback.reject_ccd_creds(&data);
        } else if (data.result != pxd_op_successful) {
            log_info("connection " FMTs64 "@%s:  Failed:  \"%s\".",
                client->io.connection, client->io.host, pxd_response(data.result));
            connect_cleanup(client);
            data.result = pxd_op_failed;
            pxd_free_crypto(&client->io.crypto);
            client->callback.connect_done(&data);
        } else {
            log_info("connection " FMTs64 "@%s completed a client-side CCD login for user "
                FMTs64 ", device " FMTs64 ", instance %s",
                client->io.connection,
                client->io.host,
                client->user_id,
                client->device_id,
                client->instance_id);

            memset(&address, 0, sizeof(address));

            address.ip_address   = unpacked->address;
            address.ip_length    = unpacked->address_length;
            address.port         = unpacked->port;

            data.socket          = client->io.socket;
            data.addresses       = address.ip_address == null ? null : &address;
            data.address_count   = address.ip_address == null ? 0    : 1;

            client->io.socket    = VPLSOCKET_INVALID;

            client->callback.connect_done(&data);
        }

        client->io.stop_now = true;    // tell the thread to stop
        break;

    case Declare_server:
        if (data.result != pxd_op_successful) {
            log_warn("The server declaration failed:  %s (%d).",
                pxd_response(data.result), (int) data.result);
            pxd_declare_failures++;
        }

        break;

    case Query_server_declaration:
        data.region         = unpacked->region;
        data.user_id        = unpacked->user_id;
        data.device_id      = unpacked->device_id;
        data.instance_id    = unpacked->instance_id;
        data.ans_dns        = unpacked->ans_dns;
        data.pxd_dns        = unpacked->pxd_dns;
        data.address_count  = unpacked->address_count;
        data.addresses      = unpacked->addresses;

        client->callback.lookup_done(&data);
        break;

    case Set_pxd_configuration:
        client->configured = true;
        pxd_configure_client(client, unpacked);
        pxd_configure_packet(client, unpacked);
        break;

    /*
     *  A pxd_connect operation has completed at the PXD
     *  server level.  If it failed, do the pxd_connect
     *  cleanup.  Otherwise, wait for a CCD challenge.
     */
    case Start_connection_attempt:
        if (data.result != pxd_op_successful) {
            log_info("connection " FMTs64 "@%s:  "
                "The connection attempt returned \"%s\" (%d).",
                      client->io.connection,
                      client->io.host,
                      pxd_response(data.result),
                (int) data.result);

            client->callback.connect_done(&data);

            connect_cleanup(client);
            pxd_mutex_lock(&client->io.mutex);
            client->io.stop_now = true;
            pxd_mutex_unlock(&client->io.mutex);
        }

        break;

    /*
     *  The CCD server side has completed a proxy connection
     *  at the PXD server level.  Send the CCD challenge
     *  packet!
     */
    case Start_proxy_connection:
        pass = data.result == pxd_op_successful;

        if (pass) {
            pxd_free_creds (&client->creds);
            pxd_free_crypto(&client->io.crypto);

            client->io.out_sequence  = 0;
            client->io.tag           = 0;
            *sequence                = 0;

            seed = mtwist_rand(&client->mt) ^ VPLTime_GetTime();

            send_ccd_challenge(client);
        } else {
            pxd_failed_proxy++;
            client->io.stop_now = true;
        }

        break;

    default:
        pxd_bad_commands++;
        log_error("connection " FMTs64 "@%s:  Packet type %s (%d) isn't a command.",
                  client->io.connection,
                  client->io.host,
                  pxd_packet_type(unpacked->type),
            (int) unpacked->type);
        pass = false;
        break;
    }

    pxd_free_unpacked(&unpacked);
    return pass;
}

/*
 *  Save the adddress that the PXD server sees for this socket.  The
 *  server puts it into the challenge packet for us.
 */
static int
save_external_address(pxd_client_t *client, pxd_unpacked_t *unpacked)
{
    pxd_address_t  address;

    memset(&address, 0, sizeof(address));

    address.ip_address = unpacked->address;
    address.ip_length  = unpacked->address_length;
    address.port       = unpacked->port;

    pxd_free_address(&client->external_address);
    client->external_address = pxd_copy_address(&address);

    return client->external_address != null;
}

/*
 *  Log the challenge packet.  Save logging information from the
 *  challenge packet, too.
 */
static void
log_challenge(pxd_client_t *client, pxd_unpacked_t *unpacked)
{
    char  buffer[512];

    /*
     *  Save the connection id and the host id for logging.
     */
    client->io.connection = unpacked->connection_id;
    strncpy(client->io.host, unpacked->pxd_dns, sizeof(client->io.host));
    client->io.host[sizeof(client->io.host) - 1] = 0;

    log_info("connection " FMTs64 "@%s started   client %p, "
        "at server time %s, request id " FMTs64,
        client->io.connection,
        client->io.host,
        client,
        pxd_print_time(buffer, sizeof(buffer), VPLTime_FromMillisec(unpacked->connection_time)),
        client->proxy_request);
}

/*
 * Implements the PXD device client thread.
 */
static VPLTHREAD_FN_DECL
run_proxy(void *config)
{
    pxd_packet_t *    packet;
    pxd_client_t *    client;
    pxd_unpacked_t *  unpacked;
    pxd_cred_t *      creds;
    pxd_cb_data_t     data;

    int       socket_failed;
    int       success;
    int32_t   seed;
    long      packets;
    int64_t   sequence;
    int64_t   unused;
    int64_t   tries;
    int64_t   wait_time;
    int       verbose;

    client = (pxd_client_t *) config;
    packet = null;
    seed   = VPLTime_GetTime();
    seed   = seed ^ (seed << 11) ^ (seed >> 19);

    client->io.socket  = VPLSOCKET_INVALID;
    client->delay      = pxd_min_delay;

    unpacked = null;
    packet   = null;

    mtwist_init(&client->mt, seed);
    memset(&data, 0, sizeof(data));

    if (client->is_proxy) {
        wait_time  = VPLTime_FromMillisec(200);
        tries      = VPLTime_FromSec(pxd_proxy_wait) / wait_time;
        tries      = max(tries, 4);

        while (tries-- > 0 && !client->is_detached) {
            VPLThread_Sleep(wait_time);
        }
    }

    if (client->proxy_request != 0) {
        log_always("client %p is processing request " FMTs64,
            client, client->proxy_request);
    }

    /*
     *  Loop until asked to stop.
     *  1) Get a socket
     *  2) Wait for a key.
     *  3) Create the MAC instance
     *  4) Loop reading packets until an error * occurs
     */
    while (!client->io.stop_now) {
        pxd_free_unpacked(&unpacked);   // clean up from any previous iteration
        pxd_free_packet  (&packet  );

        client->io.socket = sync_open_socket(client);

        if (client->io.stop_now) {
            break;
        }

        client->io.last_active = VPLTime_GetTime();

        /*
         * Read the challenge packet.
         */
        packet = pxd_read_packet(&client->io);

        if (client->io.stop_now) {
            break;
        }

        if (packet == null) {
            declare_down(client, "I failed to read the challenge");
            continue;
        }

        /*
         *  Unpack the challenge packet.
         */
        unused   = 0;
        unpacked = pxd_unpack(client->io.crypto, packet, &unused, client->io.tag);

        if (unpacked == null || unpacked->type != Send_pxd_challenge) {
            pxd_bad_challenges++;
            declare_down(client, "The challenge packet was invalid");
            continue;
        }

        log_challenge(client, unpacked);

        client->io.tag = unpacked->connection_time;

        success = save_external_address(client, unpacked);

        if (!success) {
            pxd_bad_mac_setups++;
            declare_down(client, "I could not save the external address");
            continue;
        }

        /*
         * Set up encryption, if possible.
         */
        if (client->io.crypto == null) {
            creds = client->creds;
            client->io.crypto = pxd_create_crypto(creds->key, creds->key_length, seed);
        }

        if (client->io.crypto == null) {
            pxd_bad_crypto_setups++;
            declare_down(client, "The MAC setup failed");
            continue;
        }

        client->io.ready_to_send = true;

        success = send_login(client, Send_pxd_login, unpacked);

        pxd_free_unpacked(&unpacked);
        pxd_free_packet(&packet);

        if (!success) {
            pxd_bad_login_sends++;
            declare_down(client, "The blob send failed");
            continue;
        }

        /*
         * Set the sequence number.
         */
        sequence = 1;

        /*
         * Loop reading packets until error occurs or we are requested
         * to stop.
         */
        packets = 0;
        socket_failed = false;

        while (!socket_failed && !client->io.stop_now) {
            packet = pxd_read_packet(&client->io);
            socket_failed = (packet == null);

            if (!socket_failed) {
                socket_failed = !process_packet(client, packet, &sequence);
                packets++;
                pxd_free_packet(&packet);
            }
        }

        if (client->is_proxy) {
            pxd_proxy_failures++;
            client->io.stop_now = true;
        }

        /*
         *  If we're doing a pxd_connect operation and have reached the
         *  point of sending the login packet, we can't retry anymore.
         */
        if (client->is_connect && client->connect_creds == null) {
            client->io.stop_now = true;
        }

        /*
         *  Reset the retry delay if we got some packets through.
         */
        if (packets > 3 && client->configured) {
            client->delay = pxd_min_delay;
        }

        /*
         * Do some cleanup if we're not exiting.
         */
        if (!client->io.stop_now) {
            declare_down(client, "The device socket failed");
        }
    }

    ASSERT(client->io.stop_now);

    /*
     *  Do the connect callback if it's still pending.
     */
    if (client->connect_opaque) {
        pxd_close_pending++;
        memset(&data, 0, sizeof(data));
        data.client_opaque = client->opaque;
        data.op_opaque     = client->connect_opaque;
        data.result        = pxd_op_failed;

        client->connect_opaque = null;
        client->callback.connect_done(&data);
    }

    /*
     *  Free any packet structure we might have.
     */
    pxd_free_packet(&packet);
    pxd_free_unpacked(&unpacked);   // clean up from any previous iteration

    /*
     *  Shut down the socket and clear the queues.
     */
    declare_down(client, "The service thread is stopping");
    verbose = client->io.verbose;

    /*
     *  Free any remaining packets.
     */
    discard_tcp_queue(client);

    pxd_mutex_lock(&client->io.mutex);
    client->thread_done = true;

    if (client->is_proxy) {
        pxd_proxy_shutdowns++;
    }

    if (!client->close_requested && !client->is_proxy) {
        pxd_mutex_unlock(&client->io.mutex);
    } else {
        destroy_locks(client);
        free_memory(&client);
    }

    pxd_shutdowns++;

    return VPLTHREAD_RETURN_VALUE;
}

/*
 *  Save all the information needed for a proxy operation when
 *  creating a new client for that operation.
 */
static int
setup_proxy
(
    pxd_client_t *client, pxd_unpacked_t *proxy_data, pxd_error_t *error
)
{
    client->proxy_user      = proxy_data->user_id;
    client->proxy_device    = proxy_data->device_id;
    client->proxy_request   = proxy_data->request_id;
    client->proxy_instance  = strdup(proxy_data->instance_id);

    if (client->proxy_instance == null) {
        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc failed for the proxy instance";
        return false;
    }

    return true;
}

/*
 *  Create an instance of the PXD device client.  Some such clients
 *  will be created in response to ANS messages, to perform proxy
 *  connections.
 */
static pxd_client_t *
pxd_do_open
(
    pxd_open_t *      open,
    pxd_error_t *     error,
    pxd_unpacked_t *  proxy_data,
    pxd_crypto_t *    blob_crypto
)
{
    pxd_client_t *  client;
    pxd_id_t *      id;

    int  made_locks;
    int  result;
    int  pass;

    clear_error(error);
    made_locks = false;

    client = (pxd_client_t*) malloc(sizeof(*client));

    if (client == null) {
        log_error_1("The client malloc failed.");
        error->error   = VPL_ERR_NOMEM;
        error->message = "The client malloc failed";
        pxd_free_crypto(&blob_crypto);
        return null;
    }

    id = open->credentials->id;

    memset(client, 0, sizeof(*client));

    client->user_id        =          id->user_id;
    client->device_id      =          id->device_id;
    client->creds          =          pxd_copy_creds(open->credentials);
    client->is_incoming    =          open->is_incoming;
    client->next_async     =          1;
    client->callback       =          *open->callback;
    client->opaque         =          open->opaque;
    client->io.time_limit  =          VPLTime_FromSec(6);
    client->io.kill        =          pxd_kill;
    client->io.opaque      = (void *) client;
    client->cluster        =          strdup(open->cluster_name);
    client->region         =          strdup(id->region);
    client->instance_id    =          strdup(id->instance_id);
    client->io.tag         =          0;
    client->io.verbose     =          pxd_verbose;
    client->blob_crypto    =          blob_crypto;
    client->io.idle_limit  =          VPLTime_FromSec(pxd_idle_limit);

    strncpy(client->io.host, "<unconnected>", sizeof(client->io.host));
    pxd_create_event(&client->io.event);

    /*
     *  Check whether any of the mallocs failed.
     */
    if
    (
        client->cluster     == null
    ||  client->region      == null
    ||  client->instance_id == null
    ||  client->creds       == null
    ||  client->io.event    == null
    ) {
        log_error("client %p failed in malloc", client);
        fail_open(client, made_locks);
        error->error   = VPL_ERR_NOMEM;
        error->message = "malloc failed.";
        return null;
    }

    /*
     *  Initialize the mutexes.
     */
    make_locks(client);
    made_locks = true;

    client->is_proxy = proxy_data != null;

    if (client->is_proxy) {
        pass = setup_proxy(client, proxy_data, error);

        if (!pass) {
            log_error("client %p proxy setup failed:  %s.", client, error->message);
            fail_open(client, made_locks);
            return null;
        }
    }

    /*
     *  Start the device thread.
     */
    result = VPLDetachableThread_Create(&client->device_handle, &run_proxy,
                 client, null, "proxy");

    if (result != VPL_OK) {
        log_error("client %p thread creation failed: %d", client, result);
        fail_open(client, made_locks);
        return null;
    }

    client->open = open_magic;  // Mark the client as open.

    if (client->is_proxy) {
        VPLDetachableThread_Detach(&client->device_handle);
        client->is_detached = true;
    }

    return client;
}

pxd_client_t *
pxd_open(pxd_open_t *open, pxd_error_t *error)
{
    return pxd_do_open(open, error, null, null);
}

/*
 *  Shut down a PXD client.
 */
void
pxd_close(pxd_client_t **clientp, int is_sync, pxd_error_t *error)
{
    pxd_client_t *  client;

    VPLDetachableThreadHandle_t  device_handle;

    clear_error(error);
    client = *clientp;

    if (client == null) {
        return;
    }

    pxd_mutex_lock(&client->thread_lock);
    device_handle = client->device_handle;

    if (!is_sync) {
        VPLDetachableThread_Detach(&client->device_handle);
    }

    pxd_mutex_lock(&client->io.mutex);

    client->io.stop_now      = true;
    client->close_requested  = true;
    pxd_ping(client->io.event, "pxd_close");

    if (client->thread_done) {
        destroy_locks(client);
        free_memory(&client);
    } else {
        pxd_mutex_unlock(&client->io.mutex);
    }

    if (is_sync) {
        VPLDetachableThread_Join(&device_handle);
    }

    *clientp = null;
}

/*
 *  Restart a connection to the PXD server on a client if the
 *  client has gone idle and closed its connection and stopped
 *  its thread.
 */
static void
pxd_start_connection(pxd_client_t *client, pxd_error_t *error)
{
    int  restart_thread;
    int  tries;
    int  pass;

    VPLDetachableThreadHandle_t  device_handle;

    pass = true;

    client->io.last_active = VPLTime_GetTime();

    pxd_mutex_lock(&client->thread_lock);
    pxd_mutex_lock(&client->io.mutex);
    restart_thread = client->io.stop_now;

    if (restart_thread && !client->thread_done) {
        tries = 0;

        do {
            pxd_mutex_unlock(&client->io.mutex);
            VPLThread_Sleep(VPLTime_FromMillisec(200));
            pxd_mutex_lock(&client->io.mutex);
        } while (tries++ < pxd_thread_retries && !client->thread_done);

        pass = client->thread_done;

        if (!pass) {
            error->error    = VPL_ERR_TIMEOUT;
            error->message  = "The service thread wouldn't stop.";
        }
    }

    if (restart_thread && client->thread_done) {
        client->io.stop_now = false;
        client->thread_done = false;

        error->error = VPLDetachableThread_Create(&device_handle, &run_proxy,
                     client, null, "proxy");

        if (error->error != 0) {
            error->message      = "I failed to start the service thread.";
            client->thread_done = true;
        } else {
            VPLDetachableThread_Detach(&client->device_handle);
            client->device_handle = device_handle;
        }

    }

    pxd_mutex_unlock(&client->io.mutex);
    pxd_mutex_unlock(&client->thread_lock);
}

void
pxd_declare
(
    pxd_client_t *   client,
    pxd_declare_t *  declare,
    pxd_error_t *    error
)
{
    pxd_unpacked_t *  unpacked;
    pxd_packet_t *    packet;

    int  success;

    clear_error(error);

    if (!client->is_incoming) {
        error->error   = VPL_ERR_INVALID;
        error->message = "pxd_declare must be called on a incoming (server) client";
        return;
    }

    unpacked = pxd_alloc_unpacked();

    if (unpacked == null) {
        error->error   = VPL_ERR_NOMEM;
        error->message = "malloc failed in pxd_alloc_unpacked";
        return;
    }

    setup_unpacked(client, unpacked);

    unpacked->type            = Declare_server;
    unpacked->region          = client->region;
    unpacked->user_id         = client->user_id;
    unpacked->device_id       = client->device_id;
    unpacked->instance_id     = client->instance_id;
    unpacked->ans_dns         = declare->ans_dns;
    unpacked->pxd_dns         = declare->pxd_dns;
    unpacked->address_count   = declare->address_count;
    unpacked->addresses       = &declare->addresses[0];
    unpacked->connection_tag  = client->io.tag;

    packet = pxd_pack(unpacked, null, error);

    if (packet == null) {
        pxd_free_unpacked(&unpacked);
        log_warn("pxd_declare failed in pxd_pack:  %s", error->message);
        return;
    }

    pxd_free_unpacked(&unpacked);

    pxd_start_connection(client, error);

    if (error->error != 0) {
        log_warn("client %p:  pxd_declare failed in pxd_start_connection:  %s",
            client, error->message);
        pxd_free_packet(&packet);
        return;
    }

    success = pxd_queue_packet(&client->io, packet, error, "pxd_declare");

    if (!success) {
        log_warn("pxd_declare failed in pxd_queue_packet:  %s", error->message);
        pxd_free_packet(&packet);
    }

    return;
}

void
pxd_lookup(pxd_client_t *client, pxd_id_t *id, void * opaque, pxd_error_t *error)
{
    pxd_unpacked_t *  unpacked;
    pxd_packet_t *    packet;

    int  success;

    clear_error(error);

    pxd_start_connection(client, error);

    if (error->error != 0) {
        return;
    }

    unpacked = pxd_alloc_unpacked();

    if (unpacked == null) {
        error->error   = VPL_ERR_NOMEM;
        error->message = "malloc failed in pxd_alloc_unpacked";
        return;
    }

    setup_unpacked(client, unpacked);

    unpacked->type            = Query_server_declaration;
    unpacked->region          = id->region;
    unpacked->user_id         = id->user_id;
    unpacked->device_id       = id->device_id;
    unpacked->instance_id     = id->instance_id;

    packet = pxd_pack(unpacked, opaque, error);
    pxd_free_unpacked(&unpacked);

    if (packet == null) {
        log_warn("pxd_declare failed in pxd_pack:  %s", error->message);
        return;
    }

    success = pxd_queue_packet(&client->io, packet, error, "pxd_lookup");

    if (!success) {
        log_warn("pxd_declare failed in pxd_queue_packet:  %s", error->message);
        pxd_free_packet(&packet);
    }
}

void
pxd_connect(pxd_client_t *client, pxd_connect_t *connect, pxd_error_t *error)
{
    pxd_cred_t *      creds;
    pxd_id_t *        id;
    pxd_unpacked_t *  unpacked;
    pxd_packet_t *    packet;

    char *   pxd_dns;
    int64_t  request_id;
    int      free;

    clear_error(error);

    if (client->is_incoming) {
        error->error   = VPL_ERR_INVALID;
        error->message = "The pxd client was created for incoming requests.";
        return;
    }

    pxd_start_connection(client, error);

    if (error->error != 0) {
        return;
    }

    creds    = pxd_copy_creds(connect->creds);
    id       = pxd_copy_id   (connect->target);
    pxd_dns  = connect->pxd_dns != null ? connect->pxd_dns : client->cluster;

    if (creds == null || id == null) {
        pxd_free_creds(&creds);
        pxd_free_id   (&id);
        error->error   = VPL_ERR_NOMEM;
        error->message = "malloc failed in pxd_connect (copy)!";
        return;
    }

    pxd_mutex_lock(&client->io.mutex);

    free  = client->connect_creds == null && client->connect_target == null;
    free &= !client->connect_login;

    /*
     *  If this client is available for a connection attempt, update
     *  the state while we have the appropriate lock.  Also, generate
     *  a request id.
     */
    if (free) {
        request_id              = pxd_next_id();
        client->connect_creds   = creds;
        client->connect_target  = id;
        client->connect_login   = true;
        client->connect_opaque  = connect->opaque;
    }

    pxd_mutex_unlock(&client->io.mutex);

    if (!free) {
        error->error   = VPL_ERR_BUSY;
        error->message = "A connection attempt already was in progress!";
        pxd_free_creds(&creds);
        pxd_free_id   (&id);
        return;
    }

    unpacked = pxd_alloc_unpacked();

    if (unpacked == null) {
        error->error   = VPL_ERR_NOMEM;
        error->message = "malloc failed in pxd_connect (unpacked)!";
        connect_cleanup(client);
        return;
    }

    /*
     *  Okay, tell the PXD server to start trying to set up a proxy
     *  connection.
     */
    setup_unpacked(client, unpacked);

    unpacked->type           = Start_connection_attempt;
    unpacked->region         = id->region;
    unpacked->user_id        = id->user_id;
    unpacked->device_id      = id->device_id;
    unpacked->instance_id    = id->instance_id;
    unpacked->pxd_dns        = pxd_dns;
    unpacked->request_id     = request_id;
    unpacked->address_count  = connect->address_count;
    unpacked->addresses      = connect->addresses;
    unpacked->connection_tag = client->io.tag;

    packet = pxd_pack(unpacked, connect->opaque, error);
    pxd_free_unpacked(&unpacked);

    if (error->error != 0) {
        connect_cleanup(client);
        return;
    }

    pxd_queue_packet(&client->io, packet, error, "pxd_connect");

    if (error->error != 0) {
        pxd_free_packet(&packet);
        connect_cleanup(client);
    }

    connect->request_id  = request_id;
    client->is_connect   = true;
    return;
}

/*
 *  Perform synchronous I/O.  This routine is used when processing
 *  a pxd_login operation.
 */
static int
sync_io(VPLSocket_t socket, char *base, int length, int is_write, int64_t id)
{
    char *  buffer;
    int     remaining;
    int     tries;
    int     wait;
    int     result;
    int     count;

    wait      = 100;    // milliseconds
    buffer    = base;
    remaining = length;
    tries     = (pxd_sync_io_timeout * 1000) / wait;
    result    = pxd_op_successful;

    do {
        if (is_write) {
            count = VPLSocket_Send(socket, buffer, remaining);
        } else {
            count = VPLSocket_Recv(socket, buffer, remaining);
        }

        if (count == VPL_ERR_AGAIN || count == VPL_ERR_INTR) {
            tries--;

            if (tries == 0) {
                log_warn("login " FMTs64 ":  sync_io timed out", id);
                result = pxd_timed_out;
            } else {
                VPLThread_Sleep(VPLTime_FromMillisec(wait));
            }
        } else if (count < 0) {
            tries  = 0;
            result = pxd_op_failed;
            log_warn("login " FMTs64 ":  sync_io got an I/O error", id);
        } else if (count == 0) {
            tries  = 0;
            result = pxd_connection_lost;
            log_warn("login " FMTs64 ":  The socket was closed", id);
        } else {
            buffer    += count;
            remaining -= count;
        }
    } while (remaining > 0 && tries > 0);

    return result;
}

/*
 *  Read a packet synchronously from a socket.  This routine
 *  is used for pxd_login processing.
 */
static pxd_unpacked_t *
sync_read_packet(pxd_read_t *info)
{
    char      size_buffer[sizeof(uint16_t)];
    char *    buffer;
    uint16_t  packet_size;
    int       remaining;
    int       result;

    pxd_packet_t      packet;
    pxd_unpacked_t *  unpacked;

    info->result = pxd_op_failed;

    /*
     *  Okay, read the check size from the socket.
     */
    result = sync_io(info->socket, size_buffer, sizeof(size_buffer), false, info->id);

    if (result != pxd_op_successful) {
        log_warn("login " FMTs64 ":  sync_read_packet failed in I/O:  %d",
            info->id, (int) result);
        info->result = result;
        return null;
    }

    memcpy(&packet_size, size_buffer, sizeof(packet_size));
    packet_size = VPLConv_ntoh_u16(~packet_size);

    if (packet_size > pxd_packet_limit()) {
        log_warn("login " FMTs64 ":  sync_read_packet got a check size of %d", info->id, packet_size);
        return null;
    }

    /*
     *  Now that we know the packet size, we can read the rest of the
     *  packet.
     */
    buffer = (char *) malloc(packet_size);

    if (buffer == null) {
        log_warn("login " FMTs64 ":  malloc failed for a sync_read_packet buffer", info->id);
        return null;
    }

    remaining = packet_size - sizeof(size_buffer);
    memcpy(buffer, size_buffer, sizeof(size_buffer));

    result = sync_io(info->socket, buffer + sizeof(size_buffer), remaining, false, info->id);

    if (result != pxd_op_successful) {
        log_warn("login " FMTs64 ":  sync_read_packet failed in I/O", info->id);
        info->result = result;
        free(buffer);
        return null;
    }

    memset(&packet, 0, sizeof(packet));

    packet.base   = buffer;
    packet.length = packet_size;

    unpacked = pxd_unpack(info->crypto, &packet, &info->sequence, info->tag);

    if (unpacked == null) {
        log_warn("login " FMTs64 ":  sync_read_packet failed in pxd_unpack", info->id);
        free(buffer);
        return null;
    }

    unpacked->packet = pxd_alloc_packet(buffer, packet_size, null);

    if (unpacked->packet == null) {
        log_warn("login " FMTs64 ":  sync_read_packet failed in malloc", info->id);
        pxd_free_unpacked(&unpacked);
        free(buffer);
        return null;
    }

    if
    (
        info->expected == 0
    &&  unpacked->type != Reject_ccd_credentials
    &&  unpacked->type != Send_ccd_response
    ) {
        log_warn("login " FMTs64 ":  sync_read_packet got %s (%d) in response to a login",
                  info->id,
                  pxd_packet_type(unpacked->type),
            (int) unpacked->type);

        pxd_free_unpacked(&unpacked);
    } else if (info->expected != 0 && unpacked->type != info->expected) {
        log_warn("sync_read_packet expected %s (%d), got %s (%d)",
                  pxd_packet_type(info->expected),
            (int) info->expected,
                  pxd_packet_type(unpacked->type),
            (int) unpacked->type);

        pxd_free_unpacked(&unpacked);
    }

    if (unpacked != null) {
        info->result = pxd_op_successful;
        log_info("login " FMTs64 ":  got a packet of type %s (%d)",
                  info->id,
                  pxd_packet_type(unpacked->type),
            (int) unpacked->type);
    }

    return unpacked;
}

/*
 *  Synchronously write a packet to a socket for a pxd_login
 *  operation.
 */
static int
sync_write_packet(pxd_write_t *info)
{
    pxd_packet_t *  packet;
    pxd_error_t     error;
    int             result;

    packet = pxd_pack(&info->unpacked, info->opaque, &error);

    if (packet == null) {
        return pxd_op_failed;
    }

    pxd_prep_sync(packet, &info->sequence, info->crypto);

    result = sync_io(info->socket, packet->base, packet->length, true, info->id);

    if (result == pxd_op_successful) {
        log_info("login " FMTs64 ":  sent a packet of type %s (%d)",
                  info->id,
                  pxd_packet_type(info->unpacked.type),
            (int) info->unpacked.type);
    }

    pxd_free_packet(&packet);

    return result;
}

static void
free_info(pxd_login_t **infop)
{
    pxd_login_t *  info;

    info = *infop;

    if (info != null) {
        free(info->server_key);
        pxd_free_creds(&info->credentials);
        pxd_free_id   (&info->server_id  );
        free(info);
    }

    *infop = null;
}

static void
cleanup_login
(
    pxd_cb_data_t *  data,
    pxd_read_t *     read_info,
    pxd_write_t *    write_info,
    uint64_t         result
)
{
    pxd_free_crypto(&read_info->crypto);
    pxd_free_packet(&read_info->packet);

    write_info->crypto  = null;
    data->result        = result;
}

static int
send_challenge(pxd_write_t *info)
{
    VPLNet_addr_t   address;
    int             port;
    mtwist_t        mt;
    uint32_t        seed;

    address  = VPLSocket_GetPeerAddr(info->socket);
    port     = VPLSocket_GetPeerPort(info->socket);
    port     = VPLConv_ntoh_u16(port);
    seed     = VPLTime_GetTime() ^ (VPLTime_GetTime() >> 32);
    seed    ^= pxd_outgoing_id;
    seed    ^= (int) info;

    mtwist_init(&mt, seed);

    for (int i = 0; i < array_size(info->challenge); i++) {
        info->challenge[i] = mtwist_rand(&mt);
    }

    info->connection_id = pxd_outgoing_id++;

    info->unpacked.type             = Send_ccd_challenge;
    info->unpacked.connection_id    = info->connection_id;
    info->unpacked.challenge        = (char *) info->challenge;
    info->unpacked.challenge_length = sizeof(info->challenge);
    info->unpacked.pxd_dns          = pxd_invalid_dns;
    info->unpacked.connection_time  = VPLTime_GetTime() / VPLTime_FromMillisec(1);
    info->unpacked.address          = (char *) &address;
    info->unpacked.address_length   = sizeof(address);
    info->unpacked.port             = port;

    return sync_write_packet(info);
}

static int
send_result(pxd_write_t *info, int response)
{
    info->unpacked.type           = Send_ccd_response;
    info->unpacked.response       = response;
    info->unpacked.connection_tag = info->tag;

    return sync_write_packet(info);
}

static int
send_ccd_login(pxd_write_t *write_info, pxd_unpacked_t *challenge)
{
    write_info->unpacked.type              = Send_ccd_login;
    write_info->unpacked.connection_tag    = write_info->tag;
    write_info->unpacked.connection_id     = challenge->connection_id;
    write_info->unpacked.challenge         = challenge->challenge;
    write_info->unpacked.challenge_length  = challenge->challenge_length;
    write_info->unpacked.blob              = (char *) write_info->creds->opaque;
    write_info->unpacked.blob_length       = write_info->creds->opaque_length;

    return sync_write_packet(write_info);
}

/*
 *  Run an outgoing login from a CCD client to a CCD server.
 */
static void
run_outgoing(pxd_login_t *info, pxd_cb_data_t *data)
{
    pxd_read_t        read_info;
    pxd_write_t       write_info;
    pxd_cred_t *      creds;
    uint32_t          seed;
    pxd_unpacked_t *  unpacked;
    pxd_unpacked_t    input;
    uint64_t          result;

    /*
     *  Set up the information for doing reads.
     */
    creds       = info->credentials;
    seed        = VPLTime_GetTime() ^ (VPLTime_GetTime() >> 32);
    seed       ^= (int) info;

    memset(&read_info, 0, sizeof(read_info));

    read_info.socket    = info->socket;
    read_info.crypto    = pxd_create_crypto(creds->key, creds->key_length, seed);
    read_info.tag       = 0;
    read_info.id        = info->login_id;

    /*
     *  Now set up the information for doing writes.
     */
    memset(&input,      0, sizeof(input     ));
    memset(&write_info, 0, sizeof(write_info));

    write_info.socket   = info->socket;
    write_info.tag      = 0;
    write_info.crypto   = read_info.crypto;
    write_info.creds    = creds;
    write_info.id       = info->login_id;

    /*
     *  Read the challenge.
     */
    read_info.expected  = Send_ccd_challenge;

    unpacked = sync_read_packet(&read_info);

    if (unpacked == null) {
        cleanup_login(data, &read_info, &write_info, read_info.result);
        return;
    }

    /*
     *  Update the tag now that we have one from the server side.
     */
    log_info("login " FMTs64 ":  received tag " FMTu64,
        info->login_id,
        unpacked->connection_time);
    read_info .tag = unpacked->connection_time;
    write_info.tag = unpacked->connection_time;

    result = send_ccd_login(&write_info, unpacked);

    pxd_free_unpacked(&unpacked);

    if (result != pxd_op_successful) {
        cleanup_login(data, &read_info, &write_info, result);
        return;
    }

    read_info.expected = 0;

    unpacked = sync_read_packet(&read_info);

    if (unpacked == null) {
        result = read_info.result;
    } else if (unpacked->type == Reject_ccd_credentials) {
        log_warn("login " FMTs64 ":  The server CCD rejected the credentials", info->login_id);
        result = pxd_credentials_rejected;
    }  else {
        result = unpacked->response;
    }

    data->result = result;
    info->callback(data);
    info->callback = null;  // signal that we've done the callback
    cleanup_login(data, &read_info, &write_info, result);
    pxd_free_unpacked(&unpacked);
}

static int
check_ccd_login
(
    pxd_login_t *     info,
    pxd_unpacked_t *  unpacked,
    pxd_write_t *     write_info
)
{
    pxd_crypto_t *  crypto;
    pxd_blob_t *    blob;
    pxd_error_t     error;

    int       length;
    uint32_t  seed;
    int       pass;

    length = sizeof(write_info->challenge);

    if
    (
        unpacked->connection_id    != write_info->connection_id
    ||  unpacked->challenge_length != length
    ||  memcmp(write_info->challenge, unpacked->challenge, length) != 0
    ) {
        log_info("login " FMTs64 ":  The challenge or the connection id was invalid.",
            info->login_id);
        return pxd_op_failed;
    }

    crypto = pxd_create_crypto(info->server_key, info->server_key_length, 0);
    blob   = pxd_unpack_blob(unpacked->blob, unpacked->blob_length, crypto, &error);

    pxd_free_crypto(&crypto);

    if (blob == null) {
        log_info("login " FMTs64 ":  The ccd blob was invalid.", info->login_id);
        pxd_bad_blobs++;
        return pxd_credentials_rejected;
    }

    if
    (
        blob->server_user   != info->server_id->user_id
    ||  blob->server_device != info->server_id->device_id
    ||  strcmp(blob->server_instance, info->server_id->instance_id) != 0
    ) {
        pxd_wrong_id++;
        log_info("login " FMTs64 ":  The ccd user, device, or instance id was invalid.",
            info->login_id);
        pxd_free_blob(&blob);
        return pxd_credentials_rejected;
    }

    seed  = VPLTime_GetTime() ^ (VPLTime_GetTime() >> 32);
    seed ^= (int) blob;

    write_info->crypto = pxd_create_crypto(blob->key, blob->key_length, seed);
    unpacked->unpacked_blob = blob;

    pass = pxd_check_signature(write_info->crypto, unpacked->packet);
    return pass ? pxd_op_successful : pxd_op_failed;
}

static void
run_incoming(pxd_login_t *info, pxd_cb_data_t *data)
{
    pxd_read_t        read_info;
    pxd_write_t       write_info;
    pxd_cred_t *      creds;
    uint32_t          seed;
    int               result;
    pxd_unpacked_t *  unpacked;

    /*
     *  Set up the information for doing reads.
     */
    creds       = info->credentials;
    seed        = VPLTime_GetTime() ^ (VPLTime_GetTime() >> 32);

    memset(&read_info, 0, sizeof(read_info));

    read_info.socket    = info->socket;
    read_info.sequence  = 0;
    read_info.tag       = 0;
    read_info.id        = info->login_id;

    /*
     *  Now set up the information for doing writes.
     */

    memset(&write_info, 0, sizeof(write_info));

    write_info.socket    = info->socket;
    write_info.tag       = 0;
    write_info.sequence  = 0;
    write_info.id        = info->login_id;

    /*
     *  Okay, send the challenge.
     */
    result = send_challenge(&write_info);

    if (result != pxd_op_successful) {
        cleanup_login(data, &read_info, &write_info, result);
        return;
    }

    /*
     *  Update the tag now that we have set the tag.
     */
    read_info .tag       = write_info.unpacked.connection_time;
    write_info.tag       = write_info.unpacked.connection_time;
    read_info .expected  = Send_ccd_login;

    unpacked = sync_read_packet(&read_info);

    if (unpacked == null) {
        cleanup_login(data, &read_info, &write_info, read_info.result);
        return;
    }

    result = check_ccd_login(info, unpacked, &write_info);

    read_info.crypto = write_info.crypto;

    if (result == pxd_credentials_rejected) {
        write_info.unpacked.type = Reject_ccd_credentials;
        sync_write_packet(&write_info); 
        VPLThread_Sleep(VPLTime_FromSec(pxd_reject_limit));
        pxd_free_unpacked(&unpacked);
        cleanup_login(data, &read_info, &write_info, result);
        return;
    } else if (result != pxd_op_successful) {
        pxd_free_unpacked(&unpacked);
        cleanup_login(data, &read_info, &write_info, result);
        return;
    }

    data->blob   = unpacked->unpacked_blob;
    result       = send_result(&write_info, pxd_op_successful);
    data->result = result;

    info->callback(data);
    info->callback = null;  // signal that we've done the callback
    cleanup_login(data, &read_info, &write_info, result);
    pxd_free_unpacked(&unpacked);
}

static VPLTHREAD_FN_DECL
pxd_run_login(void *info_in)
{
    pxd_login_t *  info;
    pxd_cb_data_t  data;
    
    info = (pxd_login_t *) info_in;

    memset(&data, 0, sizeof(data));

    data.op_opaque  = info->opaque;
    data.result     = pxd_op_successful;
    data.socket     = info->socket;

    if (info->is_incoming) {
        run_incoming(info, &data);
    } else {
        run_outgoing(info, &data);
    }

    if (info->callback != null) {
        info->callback(&data);
    }

    if (data.result != pxd_op_successful) {
        log_info("login " FMTs64 ":  failed with code \"%s\" (%d)",
            info->login_id, pxd_string(data.result), (int) data.result);
        close_socket(&data.socket);
    }

    free_info(&info);
    return VPLTHREAD_RETURN_VALUE;
}

/*
 *  pxd_login creates a new thread to handle the incoming
 *  request.  This design allows us to use synchronous I/O,
 *  which is easier, at the cost of some efficiency.
 */
void
pxd_login(pxd_login_t *info, pxd_error_t *error)
{
    pxd_login_t *  copied_info;

    VPLDetachableThreadHandle_t  handle;

    int  result;

    clear_error(error);

    copied_info = (pxd_login_t *) malloc(sizeof(*copied_info));

    if (copied_info == null) {
        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc failed for the login information";
        return;
    }

    memset(copied_info, 0, sizeof(*copied_info));

    if (info->is_incoming) {
        copied_info->server_key   = malloc(info->server_key_length);
        copied_info->server_id  = pxd_copy_id(info->server_id);

        if (copied_info->server_key == null || copied_info->server_id == null) {
            free_info(&copied_info);

            error->error    = VPL_ERR_NOMEM;
            error->message  = "malloc failed for the blob key";
            return;
        }

        memcpy(copied_info->server_key, info->server_key, info->server_key_length);
        copied_info->server_key_length = info->server_key_length;
    } else {
        copied_info->credentials = pxd_copy_creds(info->credentials);

        if (copied_info->credentials == null) {
            free_info(&copied_info);

            error->error    = VPL_ERR_NOMEM;
            error->message  = "malloc failed for the credentials";
            return;
        }
    }

    copied_info->socket       = info->socket;
    copied_info->opaque       = info->opaque;
    copied_info->callback     = info->callback;
    copied_info->is_incoming  = info->is_incoming;
    copied_info->login_id     = info->login_id;

    /*
     *  Assign an id if the caller doesn't.
     */
    if (copied_info->login_id == 0) {
        copied_info->login_id = pxd_next_id();
        info->login_id        = copied_info->login_id;
    }

    result = VPLDetachableThread_Create(&handle, &pxd_run_login,
                 (void *) copied_info, null, "login thread");

    if (result != VPL_OK) {
        log_error("The thread creation for pxd_login failed: %d", result);
        free_info(&copied_info);

        error->error    = result;
        error->message  = "The thread creation failed in pxd_login";
        return;
    }

    VPLDetachableThread_Detach(&handle);
}

/*
 *  Process an ANS message from a PXD server
 */
void
pxd_receive(pxd_receive_t *receive, pxd_error_t *error)
{
    pxd_client_t   *  client;
    pxd_unpacked_t *  unpacked;
    pxd_client_t   *  proxy;
    pxd_open_t        open;
    pxd_cb_data_t     data;
    pxd_crypto_t *    crypto;
    uint32_t          seed;

    clear_error(error);

    client = receive->client;

    if (client == null || client->open != open_magic) {
        error->error    = VPL_ERR_INVALID;
        error->message  = "The PXD client is null or not open";
        return;
    }

    if (!client->is_incoming) {
        error->error    = VPL_ERR_INVALID;
        error->message  = "The PXD client should be in incoming mode";
        return;
    }

    unpacked = pxd_unpack_ans(receive->buffer, receive->buffer_length, error);

    if (error->error != 0) {
        pxd_wrong_instance++;
        pxd_free_unpacked(&unpacked);
        return;
    }

    /*
     *  If this message is directed at another instance, just return.
     */
    if (strcmp(unpacked->server_instance, client->instance_id) != 0) {
        pxd_wrong_instance++;
        pxd_free_unpacked(&unpacked);
        return;
    }

    seed   = mtwist_rand(&client->mt);
    crypto = pxd_create_crypto(receive->server_key, receive->server_key_length, seed);

    memset(&data, 0, sizeof(data));

    data.op_opaque     = receive->opaque;
    data.address_count = unpacked->address_count;
    data.addresses     = unpacked->addresses;

    client->callback.incoming_request(&data);

    open.cluster_name  = unpacked->pxd_dns;
    open.credentials   = client->creds;
    open.is_incoming   = true;
    open.callback      = &client->callback;
    open.opaque        = receive->opaque;

    proxy = pxd_do_open(&open, error, unpacked, crypto);

    pxd_free_unpacked(&unpacked);
}

int
pxd_verify(pxd_verify_t *verify, pxd_error_t *error)
{
    pxd_crypto_t *  crypto;
    pxd_blob_t   *  blob;

    int  result;

    clear_error(error);

    crypto = pxd_create_crypto(verify->server_key, verify->server_length, 0);

    if (crypto == null) {
        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc failed when verifying a set of credentials";
        return false;
    }

    blob   = pxd_unpack_blob((char *) verify->opaque, verify->opaque_length, crypto, error);
    result = blob != null;

    pxd_free_crypto(&crypto);
    pxd_free_blob  (&blob);
    return result;
}

const char *
pxd_string(uint64_t response)
{
    return pxd_response(response);
}

pxd_address_t *
pxd_copy_address(pxd_address_t *address)
{
    pxd_address_t *  new_address;

    new_address = (pxd_address_t *) malloc(sizeof(*new_address));

    if (new_address == null) {
        return null;
    }

    memset(new_address, 0, sizeof(*new_address));

    new_address->ip_address = (char *) malloc(address->ip_length);

    if (new_address->ip_address == null) {
        pxd_free_address(&new_address);
        return null;
    }

    new_address->ip_length = address->ip_length;
    new_address->port      = address->port;
    new_address->type      = address->type;

    memcpy(new_address->ip_address, address->ip_address, address->ip_length);

    return new_address;
}

pxd_cred_t *
pxd_copy_creds(pxd_cred_t *creds)
{
    pxd_cred_t *  new_creds;

    new_creds = (pxd_cred_t *) malloc(sizeof(*creds));

    if (new_creds == null) {
        return null;
    }

    memset(new_creds, 0, sizeof(*new_creds));

    new_creds->id = pxd_copy_id(creds->id);

    if (new_creds->id == null) {
        pxd_free_creds(&new_creds);
        return null;
    }

    new_creds->opaque = malloc(creds->opaque_length);
    new_creds->key    = malloc(creds->key_length);

    if (new_creds->opaque == null || new_creds->key == null) {
        pxd_free_creds(&new_creds);
        return null;
    }

    new_creds->opaque_length = creds->opaque_length;
    new_creds->key_length    = creds->key_length;

    memcpy(new_creds->opaque, creds->opaque, creds->opaque_length);
    memcpy(new_creds->key,    creds->key,    creds->key_length);

    return new_creds;
}

pxd_id_t *
pxd_copy_id(pxd_id_t *id)
{
    pxd_id_t *  new_id;

    new_id = (pxd_id_t *) malloc(sizeof(*new_id));

    if (new_id == null) {
        return null;
    }

    memset(new_id, 0, sizeof(*new_id));

    new_id->region       = strdup(id->region);
    new_id->instance_id  = strdup(id->instance_id);

    if (new_id->region == null || new_id->instance_id == null) {
        pxd_free_id(&new_id);
        return null;
    }

    new_id->user_id    = id->user_id;
    new_id->device_id  = id->device_id;

    return new_id;
}

void
pxd_free_id(pxd_id_t **idp)
{
    pxd_id_t *  id;

    id = *idp;

    if (id == null) {
        return;
    }

    free(id->region);
    free(id->instance_id);

    id->region      = null;
    id->instance_id = null;
    *idp            = null;

    free(id);
}

void
pxd_free_creds(pxd_cred_t **credsp)
{
    pxd_cred_t *  creds;

    creds = *credsp;

    if (creds == null) {
        return;
    }

    pxd_free_id(&creds->id);

    free(creds->opaque);
    free(creds->key);

    creds->opaque = null;
    creds->key    = null;
    *credsp       = null;

    free(creds);
}

void
pxd_free_address(pxd_address_t **addressp)
{
    pxd_address_t *  address;

    address = *addressp;

    if (address == null) {
        return;
    }

    free(address->ip_address);
    address->ip_address = null;

    free(address);
    *addressp = null;
}
