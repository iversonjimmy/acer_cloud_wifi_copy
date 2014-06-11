#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <vpl_socket.h>
#include <pxd_test.h>
#include <pxd_client.h>
#include <pxd_log.h>
#include <log.h>

#include "pxd_log.cpp"
#include "pxd_packet.cpp"
#include "pxd_mtwist.cpp"
#include "pxd_client.cpp"
#include "pxd_event.cpp"
#include "pxd_util.cpp"

#undef  null
#define null 0

#define streq(a, b)  (strcmp((a), (b)) == 0)

#define check_fixed(a, b, s, t)                            \
            do {                                           \
                if ((a) != (b)) {                          \
                    error("field %s failed on %s\n",       \
                        (s), (t));                         \
                    exit(1);                               \
                }                                          \
            } while (0)

#define check_string(a, b, s, t)                           \
            do {                                           \
                if (!streq((a), (b))) {                    \
                    error("field %s failed on %s\n",       \
                        (s), (t));                         \
                    exit(1);                               \
                }                                          \
            } while (0)

#define check_bytes(a, a_len, b, b_len, s, t)              \
            do {                                           \
                if (!memeq((a), (a_len), (b), (b_len))) {  \
                    error("field %s failed on %s\n",       \
                        (s), (int) (t));                   \
                    exit(1);                               \
                }                                          \
            } while (0)


static pxd_open_t *  server_info        = null;
static volatile int  connect_received   = false;
static const char *  client_message     = "from the client";
static const char *  server_message     = "from the server";
static char *        lookup_host        = null;
static uint64_t      handle             = 12345678;
static char          service_id[]       = "service id";
static char          ticket[]           = "ticket";
static char          client_key [20]    = "client key";
static char          server_key [20]    = "server key";
static char          blob_key   [20]    = "blob key";
static char          session_key[20]    = "session key";
static char          saved_address[4]   = "ip!";
static char          saved_length       = 4;
static int           saved_port         = 101010;
static int           incoming_logins    = 0;

static int64_t       client_user;
static int64_t       client_device;
static int64_t       server_user;
static int64_t       server_device;

static char  demon_key[20]     =
        {
            0x2F, 0x70, 0x54, 0x37,
            0x66, 0x99, 0x4E, 0x58,
            0x42, 0x41, 0x2B, 0xA5,
            0xF6, 0xEB, 0x90, 0x16,
            0xeb, 0x31, 0x90, 0x6B
        };

static void
write_message(VPLSocket_t socket, const char *message)
{
    const char *  buffer;

    int  count;
    int  remaining;

    buffer     = message;
    remaining  = strlen(message) + 1;

    do {
        count = VPLSocket_Send(socket, buffer, remaining);

        if (count == VPL_ERR_AGAIN) {
            VPLThread_Sleep(VPLTime_FromMillisec(200));
        } else if (count <= 0) {
            error("A socket receive failed:  %d.\n", count);
            exit(1);
        } else {
            buffer    += count;
            remaining -= count;
        }
    } while (remaining > 0);
}

static void
read_message(VPLSocket_t socket, const char *expected)
{
    char *  buffer;
    char *  base;
    int     count;
    int     remaining;

    remaining =          strlen(expected) + 1;
    buffer    = (char *) malloc(remaining + 1);
    base      =          buffer;

    /*
     *  Force null termination.
     */
    buffer[remaining] = 0;

    do {
        count = VPLSocket_Recv(socket, buffer, remaining);

        if (count == VPL_ERR_AGAIN) {
            VPLThread_Sleep(VPLTime_FromMillisec(20));
        } else if (count <= 0) {
            error("A socket receive failed:  %d.\n", count);
            exit(1);
        } else {
            buffer    += count;
            remaining -= count;
        }
    } while (remaining > 0);

    if (!streq(base, expected)) {
        error("The message is incorrect:  got \"%s\" (length %d), expected \"%s\" (length %d).\n",
            base,
            strlen(base),
            expected,
            strlen(expected));
        exit(1);
    }

    free(base);
}

static void
supply_local(pxd_cb_data_t *data)
{
}

static void
supply_external(pxd_cb_data_t *data)
{
}

static void
connect_done(pxd_cb_data_t *data)
{
    log("A pxd_connect request completed.\n");

    if (data->result != pxd_op_successful) {
        error("The connect failed:  %s (%d).\n",
            pxd_string(data->result), (int) data->result);
        exit(1);
    }

    read_message (data->socket, server_message);
    write_message(data->socket, client_message);
    log(" === The client side succeeded!\n");
    connect_received = true;
}

static void
lookup_done(pxd_cb_data_t *data)
{
    pxd_id_t *  id;

    if (data->result != pxd_op_successful) {
        error("A lookup failed:  %s (%d).\n",
                  pxd_string(data->result),
            (int) data->result);
        *(int *) data->op_opaque = data->result;
        return;
    }

    id = server_info->credentials->id;

    check_string(data->region,      id->region,       "region",    "lookup");
    check_fixed (data->user_id,     id->user_id,      "user id",   "lookup");
    check_fixed (data->device_id,   id->device_id,    "device id", "lookup");
    check_string(data->instance_id, id->instance_id,  "instance",  "lookup");

    log("got credentials for user " FMTs64 ", device " FMTs64 ", instance %s\n",
        data->user_id,
        data->device_id,
        data->instance_id);

    *(int *) data->op_opaque = data->result;
    free(lookup_host);
    lookup_host = strdup(data->pxd_dns);
}

static void
incoming_request(pxd_cb_data_t *data)
{
    log("The incoming request has been parsed.\n");
}

static void
incoming_login(pxd_cb_data_t *data)
{
    log("The incoming request succeeded.\n");
    write_message(data->socket, server_message);
    read_message (data->socket, client_message);
    log(" === The server side succeeded!\n");
    incoming_logins++;
}

static void
reject_ccd_creds(pxd_cb_data_t *data)
{
    error("The ccd credentials were rejected.\n");
    exit(1);
}

static void
reject_pxd_creds(pxd_cb_data_t *data)
{
    error("The pxd credentials were rejected.\n");
    exit(1);
}

static pxd_callback_t  callbacks =
    {
        supply_local,
        supply_external,
        connect_done,
        lookup_done,
        incoming_request,
        incoming_login,
        reject_ccd_creds,
        reject_pxd_creds
    };

/*
 *  Make credentials to give to a PXD demon.  CCD gets
 *  these from IAS.
 */
static void
make_pxd_blob(pxd_open_t *open)
{
    pxd_crypto_t *  crypto;
    pxd_blob_t      input;
    pxd_error_t     error;

    crypto = pxd_create_crypto(&demon_key, sizeof(demon_key), 1234);

    memset(&input, 0, sizeof(input));

    input.client_user         = open->credentials->id->user_id;
    input.client_device       = open->credentials->id->device_id;
    input.client_instance     = open->credentials->id->instance_id;
    input.create              = VPLTime_GetTime();
    input.key                 = (char *) open->credentials->key;
    input.key_length          = open->credentials->key_length;
    input.handle              = (char *) &handle;
    input.handle_length       = sizeof(handle);
    input.service_id          = service_id;
    input.service_id_length   = sizeof(service_id);
    input.ticket              = ticket;
    input.ticket_length       = sizeof(ticket);

    open->credentials->opaque =
        pxd_pack_demon_blob(&open->credentials->opaque_length, &input, crypto, &error);

    pxd_free_crypto(&crypto);
}

/*
 *  Open a client for a pxd demon.
 */
static pxd_client_t *
do_open
(
    pxd_open_t *  open,
    int64_t       user_id,
    int64_t       device_id,
    int           is_incoming
)
{
    pxd_client_t *  client;
    pxd_error_t     error;

    open->credentials->id->user_id     = user_id;
    open->credentials->id->device_id   = device_id;

    open->callback     = &callbacks;
    open->is_incoming  = is_incoming;

    make_pxd_blob(open);

    client = pxd_open(open, &error);

    if (error.error != 0) {
        error("pxd_open failed:  %s\n", error.message);
        exit(1);
    }

    return client;
}

/*
 *  Make credentials for connecting to a server CCD emulator.
 */
static void
make_ccd_creds(pxd_id_t *server_id, pxd_cred_t *connect_creds)
{
    pxd_crypto_t *  crypto;
    pxd_blob_t      input;
    pxd_error_t     error;

    crypto = pxd_create_crypto(&blob_key, sizeof(blob_key), 1234);

    memset(&input, 0, sizeof(input));

    input.client_user         = connect_creds->id->user_id;
    input.client_device       = connect_creds->id->device_id;
    input.client_instance     = connect_creds->id->instance_id;
    input.server_user         = server_id->user_id;
    input.server_device       = server_id->device_id;
    input.server_instance     = server_id->instance_id;
    input.create              = VPLTime_GetTime();
    input.key                 = session_key;
    input.key_length          = sizeof(session_key);
    input.handle              = (char *) &handle;
    input.handle_length       = sizeof(handle);
    input.service_id          = service_id;
    input.service_id_length   = sizeof(service_id);
    input.ticket              = ticket;
    input.ticket_length       = sizeof(ticket);

    connect_creds->opaque =
        pxd_pack_blob(&connect_creds->opaque_length, &input, crypto, &error);

    pxd_free_crypto(&crypto);
    return;
}

/*
 *  Create an ANS message like that sent from a PXD demon to
 *  a server CCD instance to inform it of a proxy request.
 */
static char *
make_ans
(
    pxd_cred_t *  client_creds,
    int *         length,
    char *        pxd_host,
    int64_t       request_id,
    char *        server_instance
)
{
    pxd_unpacked_t  unpacked;
    pxd_id_t *      id;
    pxd_address_t   address;

    id = client_creds->id;

    memset(&unpacked, 0, sizeof(unpacked));
    memset(&address,  0, sizeof(address ));

    unpacked.type            = pxd_connect_request;
    unpacked.user_id         = id->user_id;
    unpacked.device_id       = id->device_id;
    unpacked.instance_id     = id->instance_id;
    unpacked.request_id      = request_id;
    unpacked.pxd_dns         = pxd_host;
    unpacked.address_count   = 1;
    unpacked.server_instance = server_instance;
    unpacked.addresses       = &address;

    unpacked.address         = saved_address;
    unpacked.address_length  = saved_length;
    unpacked.port            = saved_port;

    return pxd_pack_ans(&unpacked, length);
}

static int64_t
generate_id(mtwist_t *mt)
{
    int64_t  result;

    result  = mtwist_rand(mt);
    result  = result << 31;
    result ^= mtwist_rand(mt);
    return result;
}

int
main(int argc, char **argv)
{
    char *  pxd_host  = (char *) getenv("pxd_host");
    char *  ans_host  = (char *) getenv("ans_host");
    char *  region    = (char *) getenv("region");

    pxd_open_t      open_client;
    pxd_id_t        client_id;
    pxd_cred_t      client_creds;
    pxd_client_t *  client;

    pxd_open_t      open_server;
    pxd_id_t        server_id;
    pxd_cred_t      server_creds;
    pxd_client_t *  server;

    pxd_cred_t      connect_creds;
    pxd_declare_t   declare;
    pxd_connect_t   connect;
    pxd_receive_t   receive;
    pxd_error_t     error;
    mtwist_t        mt;
    char            ip_address[4];
    int             lookup_result;
    int             tries;
    int             lookup_tries;
    int             receive_tries;
    uint32_t        seed;

    if (pxd_host == null || ans_host == null || region == null) {
        printf("pxd_host, ans_host, and region must be set.\n");
        exit(1);
    }

    seed = VPLTime_GetTime() ^ getpid();

    mtwist_init(&mt, seed);

    /*
     *  Generate some fake user and device ids.
     */
    client_user    = generate_id(&mt);
    server_user    = generate_id(&mt);
    client_device  = generate_id(&mt);
    server_device  = generate_id(&mt);

    /*
     *  Set up the structures for opening a client connection.
     */
    memset(&open_client,  0, sizeof(open_client ));
    memset(&client_creds, 0, sizeof(client_creds));
    memset(&client_id,    0, sizeof(client_id   ));

    open_client.cluster_name             = pxd_host;
    open_client.credentials              = &client_creds;
    open_client.credentials->id          = &client_id;
    open_client.credentials->key         = client_key;
    open_client.credentials->key_length  = sizeof(client_key);
    open_client.credentials->id->region  = region;
    open_client.credentials->id->instance_id  = (char *) "CCD-client";
    open_client.credentials->key              = client_key;
    open_client.credentials->key_length       = sizeof(client_key);

    /*
     *  Now open the client CCD emulator connection.
     */
    client = do_open(&open_client, client_user, client_device, false);
    free(open_client.credentials->opaque);
    open_client.credentials->opaque = null;

    /*
     *  Okay, open the server CCD connection.
     */
    server_info = &open_server;

    memset(&open_server,  0, sizeof(open_server ));
    memset(&server_creds, 0, sizeof(server_creds));
    memset(&server_id,    0, sizeof(server_id   ));

    open_server.cluster_name                  = pxd_host;
    open_server.credentials                   = &server_creds;
    open_server.credentials->id               = &server_id;
    open_server.credentials->key              = server_key;
    open_server.credentials->key_length       = sizeof(server_key);
    open_server.credentials->id->region       = region;
    open_server.credentials->id->instance_id  = (char *) "CCD-server";
    open_server.credentials->key              = server_key;
    open_server.credentials->key_length       = sizeof(server_key);

    server = do_open(&open_server, server_user, server_device, true );
    free(open_server.credentials->opaque);
    open_server.credentials->opaque = null;

    if (client == null || server == null) {
        error("A pxd_open operation failed.\n");
        exit(1);
    }

    /*
     *  Declare the server CCD to the PXD demon.
     */
    memset(&declare,    0, sizeof(declare   ));
    memset(&ip_address, 0, sizeof(ip_address));

    declare.ans_dns       = ans_host;
    declare.pxd_dns       = pxd_host;
    declare.address_count = 0;

    declare.addresses[0].ip_address = ip_address;
    declare.addresses[0].ip_length  = sizeof(ip_address);
    declare.addresses[0].port       = 8;

    pxd_declare(server, &declare, &error);

    if (error.error != 0) {
        error("pxd_declare failed:  %s.\n", error.message);
        exit(1);
    }

    VPLThread_Sleep(VPLTime_FromMillisec(200));

    /*
     *  Now do a lookup of the server CCD on the client CCD connection.
     */
    lookup_tries = 4;

    log("starting a lookup for user " FMTs64 ", device " FMTs64 ", instance %s\n",
        server_id.user_id,
        server_id.device_id,
        server_id.instance_id);

    do {
        lookup_result = -1;

        pxd_lookup(client, &server_id, &lookup_result, &error);

        if (error.error != 0) {
            error("pxd_lookup failed:  %s.\n", error.message);
            exit(1);
        }

        tries = 40;

        while (lookup_result < 0 && tries-- > 0) {
            VPLThread_Sleep(VPLTime_FromMillisec(100));
        }

        if (lookup_result != pxd_op_successful) {
            sleep(1);
        }
    } while (lookup_tries-- > 0 && lookup_result != pxd_op_successful);

    if (lookup_result != pxd_op_successful) {
        error("pxd_lookup failed:  %s (%d).\n",
            pxd_string(lookup_result),
            (int) lookup_result);
        exit(1);
    }

    /*
     *  Start the connection attempt.
     */
    memset(&connect,       0, sizeof(connect      ));
    memset(&connect_creds, 0, sizeof(connect_creds));

    connect.target                  = server_info->credentials->id;
    connect.creds                   = &connect_creds;
    connect.pxd_dns                 = lookup_host;
    connect.creds->id               = &client_id;
    connect.creds->key              = session_key;
    connect.creds->key_length       = sizeof(session_key);

    make_ccd_creds(connect.target, connect.creds);

    /*
     *  Now add the address.  It's not used yet by this test
     *  program, so its content is irrelevant, and I might not
     *  even need to set it.
     */
    connect.address_count = 1;

    connect.addresses[0].ip_address = saved_address;
    connect.addresses[0].ip_length  = saved_length;
    connect.addresses[0].port       = 7000;

    log("calling pxd_connect\n");

    pxd_connect(client, &connect, &error);
    free(connect.creds->opaque);
    connect.creds->opaque = null;

    if (error.error != 0) {
        error("pxd_connect failed:  %s.\n", error.message);
        exit(1);
    }

    /*
     *  Give the server some time to get the connection going.
     */
    sleep(1);

    /*
     *  The connection setup might be done on the server, with
     *  some look. So start trying receive operations.  We don't
     *  have ANS providing synchronization, so we need to give
     *  it a few shots.
     */
    receive_tries = 3;

    do {
        log("calling pxd_receive\n");
        memset(&receive, 0, sizeof(receive));

        receive.buffer =
            make_ans
            (
                &client_creds,
                &receive.buffer_length,
                pxd_host,
                connect.request_id,
                server_id.instance_id
            );

        receive.server_key         = blob_key;
        receive.server_key_length  = sizeof(blob_key);
        receive.client             = server;

        if (receive.buffer == null) {
            error("make_ans failed.\n");
            exit(1);
        }

        pxd_receive(&receive, &error);
        free(receive.buffer);
        receive.buffer = null;

        if (error.error != 0) {
            error("pxd_receive failed:  %s.\n", error.message);
            exit(1);
        }

        tries = 20;

        while (tries-- > 0 && (!connect_received || incoming_logins == 0)) {
            VPLThread_Sleep(VPLTime_FromMillisec(200));
        }
    } while (receive_tries-- > 0 && incoming_logins == 0);
 
    /*
     *  At this point, we should have received the callback for the
     *  connection attempt and an incoming login.
     */
    if (!connect_received || incoming_logins == 0) {
        error("The proxy connection operation didn't complete.\n");
        error("connect_receive:  %d, incoming_logins %d\n",
            (int) connect_received, (int) incoming_logins);
        exit(1);
    }

    log(" === The connect operation completed\n");

    pxd_close(&client, true, &error);

    if (error.error != 0) {
        error("The client close failed:  %s.\n", error.message);
        exit(1);
    }

    pxd_close(&server, true, &error);

    if (error.error != 0) {
        error("The server close failed:  %s.\n", error.message);
        exit(1);
    }

    free(lookup_host);
    sleep(1);   // Wait for the threads to exit (we hope)
    printf("The pxd circle test passed.\n");
    return 0;
}
