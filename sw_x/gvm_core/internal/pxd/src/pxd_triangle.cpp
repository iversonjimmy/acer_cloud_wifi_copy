#define pxd_test

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <vpl_socket.h>
#include <pxd_test.h>
#include <pxd_client.h>
#include <pxd_log.h>
#include <ans_device.h>
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

static pxd_open_t *     server_info        = null;
static pxd_client_t *   server;
static pxd_address_t *  server_address     = null;
static volatile int     connect_finished   = false;
static volatile int     connect_passed     = false;
static const char *     client_message     = "from the client";
static const char *     server_message     = "from the server";

static char *    ans_host;
static char *    pxd_host;
static char *    lookup_host        = null;
static uint64_t  handle             = 12345678;
static char      service_id[]       = "service id";
static char      ticket[]           = "ticket";
static char      client_key [20]    = "client key";
static char      server_key [20]    = "server key";
static char      blob_key   [20]    = "blob key";
static char      session_key[20]    = "session key";
static char      saved_address[4]   = "ip!";
static char      saved_length       = 4;
static char      ip_address[4]      = "ips";
static int       incoming_logins    = 0;
static int       ans_login_done     = false;
static int       test_pxd_rejection = false;
static int       rejections         = 0;

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
            error("A socket send failed:  %d.\n", count);
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
log_address(const char *type, const pxd_address_t *address)
{
    log("I received the external %s address %d.%d.%d.%d:%d\n",
              type,
        (int) address->ip_address[0],
        (int) address->ip_address[1],
        (int) address->ip_address[2],
        (int) address->ip_address[3],
        (int) address->port);
}

static void
supply_external_client(pxd_cb_data_t *data)
{
    log_address("client", data->addresses);
}

static void
supply_external_server(pxd_cb_data_t *data)
{
    pxd_free_address(&server_address);
    server_address = pxd_copy_address(data->addresses);
    log_address("server", server_address);
}

static void
connect_done(pxd_cb_data_t *data)
{
    log("A pxd_connect request completed.\n");

    if (data->result != pxd_op_successful) {
        error("The connect failed:  %s (%d).\n",
            pxd_string(data->result), (int) data->result);
        connect_finished = true;
        return;
    }

    if (data->addresses == null || data->address_count != 1) {
        error("The connect callback didn't get any addresses.\n");
        connect_finished = true;
        exit(1);
    }

    read_message (data->socket, server_message);
    write_message(data->socket, client_message);
    log(" === The client side succeeded!\n");
    connect_passed    = true;
    connect_finished  = true;
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

    log("got credentials for user " FMTs64 ", device " FMTs64 ", instance %s at %s\n",
        data->user_id,
        data->device_id,
        data->instance_id,
        data->pxd_dns);

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
    if (test_pxd_rejection) {
        rejections++;
    } else {
        error("The pxd credentials were rejected.\n");
        exit(1);
    }
}

static pxd_callback_t  pxd_client_callbacks =
    {
        supply_local,
        supply_external_client,
        connect_done,
        lookup_done,
        incoming_request,
        incoming_login,
        reject_ccd_creds,
        reject_pxd_creds
    };

static pxd_callback_t  pxd_server_callbacks =
    {
        supply_local,
        supply_external_server,
        connect_done,
        lookup_done,
        incoming_request,
        incoming_login,
        reject_ccd_creds,
        reject_pxd_creds
    };

static VPL_BOOL
connectionActive(ans_client_t *client, VPLNet_addr_t local_addr)
{
    return true;
}

static VPL_BOOL
receiveNotification(ans_client_t *client, ans_unpacked_t *ans_unpacked)
{
    pxd_error_t       error;
    pxd_unpacked_t *  pxd_unpacked;
    pxd_declare_t     declare;
    pxd_receive_t     receive;

    log("test: Received a notification.\n");

    pxd_unpacked = pxd_unpack_ans((char *) ans_unpacked->notification + 1, ans_unpacked->notificationLength - 1, &error);

    if (pxd_unpacked == null) {
        error("I could not unpack the ANS notification.\n");
        exit(1);
    }

    switch (pxd_unpacked->type) {
    case pxd_connect_request:
        memset(&receive, 0, sizeof(receive));

        /*
         *  Set up the receive structure.  The first byte is removed
         *  by CCD, which we are emulating.
         */
        receive.client             = server;
        receive.buffer             = (char *) ans_unpacked->notification + 1;
        receive.buffer_length      = ans_unpacked->notificationLength - 1;
        receive.server_key         = blob_key;
        receive.server_key_length  = sizeof(blob_key);

        pxd_receive(&receive, &error);

        if (error.error != 0) {
            error("pxd_receive returned \"%s\" (%s, %d).\n",
                error.message, pxd_string(error.error), (int) error.error);
            exit(1);
        }

        break;

    case pxd_wakeup:
        log("I am calling pxd_declare.\n");

        /*
         *  Declare the server CCD to the PXD demon.
         */
        memset(&declare,    0, sizeof(declare   ));
        memset(&ip_address, 0, sizeof(ip_address));

        declare.ans_dns                  = ans_host;
        declare.pxd_dns                  = pxd_host;
        declare.address_count            = 1;
        declare.addresses[0].ip_address  = ip_address;  // not used
        declare.addresses[0].ip_length   = sizeof(ip_address);
        declare.addresses[0].port        = 8;

        pxd_declare(server, &declare, &error);

        if (error.error != 0) {
            error("pxd_declare failed:  %s.\n", error.message);
            exit(1);
        }

        break;

    default:
        error("That message type (%d) is not valid!\n", (int) pxd_unpacked->type);
        exit(1);
    }

    pxd_free_unpacked(&pxd_unpacked);
    return true;
}

static void
receiveSleepInfo(ans_client_t *client, ans_unpacked_t *unpacked)
{
}

static void
receiveDeviceState(ans_client_t *client, ans_unpacked_t *unpacked)
{
}

static void
connectionDown(ans_client_t *client)
{
}

static void
connectionClosed(ans_client_t *client)
{
}

static void
setPingPacket(ans_client_t *client, char *packet, int length)
{
}

static void
rejectCredentials(ans_client_t *client)
{
}

static void
loginCompleted(ans_client_t *client)
{
    ans_login_done = true;
}

static void
rejectSubscriptions(ans_client_t *client)
{
}

static void
receiveResponse(ans_client_t *client, ans_unpacked_t *response)
{
}

static ans_callbacks_t  ans_callbacks =
    {
        connectionActive,
        receiveNotification,
        receiveSleepInfo,
        receiveDeviceState,
        connectionDown,
        connectionClosed,
        setPingPacket,
        rejectCredentials,
        loginCompleted,
        rejectSubscriptions,
        receiveResponse
    };

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

static int64_t
parse_id(char *id)
{
    char *   end;
    int64_t  result;

    end = null;

    result = strtoll(id, &end, 10);

    if (end != null && *end != 0) {
        error("id \"%s\" is not valid.\n", id);
        exit(1);
    }

    return result;
}

static pxd_client_t *
do_open
(
    pxd_open_t *  open,
    char *        user_id,
    char *        device_id,
    int           is_incoming
)
{
    pxd_client_t *  client;
    pxd_client_t *  rejected;
    pxd_error_t     error;
    char *          bytes;
    int             tries;

    open->credentials->id->user_id     = parse_id(user_id  );
    open->credentials->id->device_id   = parse_id(device_id);

    open->callback     = is_incoming ? &pxd_server_callbacks : &pxd_client_callbacks;
    open->is_incoming  = is_incoming;

    make_pxd_blob(open);

    client = pxd_open(open, &error);

    if (error.error != 0) {
        error("pxd_open failed:  %s\n", error.message);
        exit(1);
    }

    bytes = (char *) open->credentials->opaque;
    
    if (is_incoming) {
        bytes[open->credentials->opaque_length - 1]++;
    } else {
        bytes[0]++;
    }

    test_pxd_rejection  = true;
    rejections          = 0;
    rejected            = pxd_open(open, &error);

    if (error.error != 0) {
        error("pxd_open failed for the reject test:  %s\n", error.message);
        exit(1);
    }

    tries = 0;

    while (rejections == 0 && tries++ < 10) {
        sleep(1);
    }

    if (rejections == 0) {
        error("I did not receive a reject-credentials callback.\n");
        exit(1);
    }

    pxd_close(&rejected, true, &error);
    test_pxd_rejection = false;
    return client;
}

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

    if (connect_creds->opaque == null) {
        error("pxd_pack_blob failed:  %s\n", error.message);
        exit(1);
    }

    pxd_free_crypto(&crypto);
    return;
}

static char *
unmarshal(const char *string, int *length)
{
    const char *  input;

    char *  binary;
    int     output_length = strlen(string) / 2;
    int     i;
    int     value;

    if (string == NULL) {
        error("unmarshal a null string\n");
        exit(1);
    }

    binary = (char *) malloc(output_length);

    if (binary == NULL) {
        error("unmarshal malloc failed.\n");
        exit(1);
    }

    input = string;

    for (i = 0; i < output_length; i++) {
        sscanf(input, "%2x", &value);
        binary[i] = value;
        input += 2;
    }

    *length = output_length;
    return binary;
}

static ans_client_t *
open_ans(pxd_cred_t *credentials, char *ans_host, char *ans_blob, char *ans_key)
{
    ans_client_t *  result;
    ans_open_t      open;
    int             tries;

    memset(&open, 0, sizeof(open));

    open.clusterName   = ans_host;
    open.deviceType    = "default";
    open.application   = "pxd test";
    open.verbose       = true;
    open.callbacks     = &ans_callbacks;

    open.blob = unmarshal(ans_blob, &open.blobLength);
    open.key  = unmarshal(ans_key , &open.keyLength );

    result = ans_open(&open);

    tries = 15;

    while (!ans_login_done && tries-- > 0) {
        log("Waiting for a connection to the ANS server.\n");
        sleep(1);
    }

    if (!ans_login_done) {
        error("ans_open failed.\n");
        exit(1);
    }

    free((void *) open.blob);
    free((void *) open.key);
    return result;
}

/*
 *  Try to work around using the standard logging routines
 *  as much as possible.  ans_device is compiled to use
 *  them, unfortunately.
 */
extern "C" {
    extern void LOGInit(const char* processName, const char* root);
};


int
main(int argc, char **argv)
{
    char *  region           = (char *) getenv("region"         );

    char *  client_user      = (char *) getenv("client_user"    );
    char *  client_device    = (char *) getenv("client_device"  );

    char *  server_user      = (char *) getenv("server_user"    );
    char *  server_device    = (char *) getenv("server_device"  );
    char *  server_instance  = (char *) getenv("server_instance");
    char *  server_ans_blob  = (char *) getenv("ans_blob"       );
    char *  server_ans_key   = (char *) getenv("ans_key"        );

    //pxd_declare_t   declare;  // TODO:  remove

    pxd_open_t      open_client;
    pxd_id_t        client_id;
    pxd_cred_t      client_creds;
    pxd_client_t *  client;

    pxd_open_t      open_server;
    pxd_id_t        server_id;
    pxd_cred_t      server_creds;
    ans_client_t *  ans_client;

    pxd_cred_t      connect_creds;
    pxd_connect_t   connect;
    pxd_error_t     error;
    mtwist_t        mt;
    //int             lookup_result;
    int             tries;
    //int             lookup_tries;
    int             connect_tries;
    uint32_t        seed;

    LOGInit("pxd_triangle", null);

    pxd_host = (char *) getenv("pxd_host");
    ans_host = (char *) getenv("ans_host");

    if (pxd_host == null || ans_host == null || region == null) {
        printf("pxd_host, ans_host, and region must be set.\n");
        exit(1);
    }

    if (client_user == null || client_device == null) {
        printf("client_user and client_device must be set.\n");
        exit(1);
    }

    if (server_user == null || server_device == null || server_instance == null) {
        printf("server_user, server_device, and server_instance must be set.\n");
        exit(1);
    }

    if (server_ans_blob == null || server_ans_key == null) {
        printf("ans_blob and ans_key must be set.\n");
        exit(1);
    }

    seed = VPLTime_GetTime() ^ getpid();

    mtwist_init(&mt, seed);

    /*
     *  Set up the structures for opening a client connection.
     */
    memset(&open_client,  0, sizeof(open_client ));
    memset(&client_creds, 0, sizeof(client_creds));
    memset(&client_id,    0, sizeof(client_id   ));

    open_client.cluster_name                  = pxd_host;
    open_client.credentials                   = &client_creds;
    open_client.credentials->id               = &client_id;
    open_client.credentials->key              = client_key;
    open_client.credentials->key_length       = sizeof(client_key);
    open_client.credentials->id->region       = region;
    open_client.credentials->id->instance_id  = (char *) "CCD-client";
    open_client.credentials->key              = client_key;
    open_client.credentials->key_length       = sizeof(client_key);

    /*
     *  Now open the client CCD connection.
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
    open_server.credentials->id->instance_id  = server_instance;
    open_server.credentials->key              = server_key;
    open_server.credentials->key_length       = sizeof(server_key);

    server = do_open(&open_server, server_user, server_device, true );
    free(open_server.credentials->opaque);

    open_server.credentials->opaque = null;

    if (client == null || server == null) {
        error("A pxd_open operation failed.\n");
        exit(1);
    }

    ans_client = open_ans(open_server.credentials, ans_host, server_ans_blob, server_ans_key);

    VPLThread_Sleep(VPLTime_FromMillisec(1000));

    /*
     *  Declare the server CCD to the PXD demon.
     */
    /*memset(&declare,    0, sizeof(declare   ));
    memset(&ip_address, 0, sizeof(ip_address));

    declare.ans_dns                  = ans_host;
    declare.pxd_dns                  = pxd_host;
    declare.address_count            = 1;
    declare.addresses[0].ip_address  = ip_address;  // not used
    declare.addresses[0].ip_length   = sizeof(ip_address);
    declare.addresses[0].port        = 8;

    pxd_declare(server, &declare, &error);

    if (error.error != 0) {
        error("pxd_declare failed:  %s.\n", error.message);
        exit(1);
    }*/

    /*
     *  Now try a lookup of the server CCD on the CCD client side.
     *  The lookup should cause PXD to send a server wakeup message
     *  to the server side.
     */
    /*lookup_tries = 4;

    log("Starting the pxd_lookup attempts.\n");

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
            pxd_string(lookup_result), (int) lookup_result);
        exit(1);
    }*/

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
     *  Now add the address.  It's not used yet, so its content is irrelevant,
     *  and I might not even need to set it.
     */
    connect.address_count           = 1;
    connect.addresses[0].ip_address = saved_address;
    connect.addresses[0].ip_length  = saved_length;
    connect.addresses[0].port       = 7000;

    connect_tries = 3;

    do {
        connect_finished = false;
        log("calling pxd_connect\n");

        pxd_connect(client, &connect, &error);

        if (error.error != 0) {
            error("pxd_connect failed:  %s.\n", error.message);
            exit(1);
        }

        /*
         *  The connection setup might be done on the server, with
         *  some look. So start trying receive operations.  We don't
         *  have ANS providing synchronization, so we need to give
         *  it a few shots.
         */
        tries = 10;

        while ((!connect_finished || incoming_logins == 0) && tries-- > 0) {
            sleep(1);
        }

        if (!connect_finished) {
            error("The proxy connection operation didn't complete.\n");
            exit(1);
        }

        log(" === A connect operation completed.\n");
    } while (!connect_passed && connect_tries-- > 0);

    tries = 5;

    free(connect.creds->opaque);
    connect.creds->opaque = null;

    while (incoming_logins == 0 && tries-- > 0) {
        sleep(1);
    }

    /*
     *  At this point, we should have received the callback for the
     *  connection attempt and an incoming login.
     */
    if (!connect_passed || incoming_logins == 0) {
        error("The proxy connection operation didn't complete or didn't pass.\n");
        error("connect_finished:  %d, connect_passed %d, incoming_logins %d\n",
            (int) connect_finished, (int) connect_passed, (int) incoming_logins);
        exit(1);
    }

    log(" === The connect operation succeeded.\n");

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

    ans_close(ans_client, true);
    free(lookup_host);
    pxd_free_address(&server_address);

    sleep(1);   // Wait for the threads to exit (we hope)
    printf("The pxd triangle test passed.\n");
    return 0;
}
