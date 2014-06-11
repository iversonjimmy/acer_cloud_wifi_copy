#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <sys/time.h>
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

#undef min
#define min(a, b) ((a) < (b) ? (a) : (b))

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

static volatile int       server_socket      = -1;
static volatile int       client_socket      = -1;

static volatile int       ans_login_done     = false;
static volatile int       pxd_login_done     = false;

static char  demon_key[20]     =
        {
            0x2F, 0x70, 0x54, 0x37,
            0x66, 0x99, 0x4E, 0x58,
            0x42, 0x41, 0x2B, 0xA5,
            0xF6, 0xEB, 0x90, 0x16,
            0xeb, 0x31, 0x90, 0x6B
        };

static struct timespec     sleep_time = { 0, 5 * 1000 * 1000 };

static void
supply_local(pxd_cb_data_t *data)
{
}

static void
supply_external_client(pxd_cb_data_t *data)
{
}

static void
supply_external_server(pxd_cb_data_t *data)
{
    pxd_free_address(&server_address);
    server_address = pxd_copy_address(data->addresses);
    pxd_login_done = true;
}

static void
connect_done(pxd_cb_data_t *data)
{
    connect_finished = true;

    if (data->result != pxd_op_successful) {
        error("The pxd_connect request failed:  %s (%d).\n",
            pxd_string(data->result), (int) data->result);
        exit(1);
    }

    if (data->addresses == null || data->address_count != 1) {
        error("The pxd_connect callback didn't get any addresses.\n");
        exit(1);
    }

    client_socket = data->socket.fd;
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

    *(int *) data->op_opaque = data->result;
    free(lookup_host);
    lookup_host = strdup(data->pxd_dns);
}

static void
incoming_request(pxd_cb_data_t *data)
{
}

static void
incoming_login(pxd_cb_data_t *data)
{
    server_socket = data->socket.fd;
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
    pxd_error_t     error;
    char *          bytes;

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
        error("unmarshal can't handle a null string.\n");
        exit(1);
    }

    binary = (char *) malloc(output_length);

    if (binary == NULL) {
        error("malloc failed in unmarshal.\n");
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

    tries = 1000;

    while (!ans_login_done && tries-- > 0) {
        nanosleep(&sleep_time, null);
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

#define clock_res()  ((int64_t) 1000 * 1000)

static int64_t
get_monotonic(void)
{
    struct timeval  time_value;

    int64_t  result;

    gettimeofday(&time_value, null);

    result  = (int64_t) time_value.tv_sec * clock_res();
    result += time_value.tv_usec;

    return result;
}

static void
poll_init(struct pollfd *poll_data)
{
    memset(poll_data, 0, 2 * sizeof(*poll_data));

    poll_data[0].fd      = server_socket;
    poll_data[0].events  = POLLIN;

    poll_data[1].fd      = client_socket;
    poll_data[1].events  = POLLOUT;
}

static void
run_oneway(int stream_length, int buffer_size)
{
    struct pollfd  poll_data[2];

    char *   buffer;
    int      bytes_read;
    int      bytes_written;
    int      result;
    int      count;
    int      write_limit;
    int64_t  start_time;
    int64_t  total_time;
    int64_t  print_time;
    int64_t  current;
    double   bandwidth;
    double   total_seconds;
    double   total_kb;
    int      fd;

    fd     = server_socket;
    result = fcntl(fd, F_SETFL, O_NONBLOCK);

    if (result < 0) {
        printf(" *** failed in fcntl on fd %d\n", fd);
        perror("server fd:  fcntl");
        exit(1);
    }

    fd     = client_socket;
    result = fcntl(fd, F_SETFL, O_NONBLOCK);

    if (result != 0) {
        printf(" *** failed in fcntl on fd %d\n", fd);
        perror("client fd:  fcntl");
        exit(1);
    }

    bytes_read     = 0;
    bytes_written  = 0;
    print_time     = VPLTime_GetTime();
    buffer         = (char *) malloc(buffer_size);

    poll_init(poll_data);
    memset(buffer, 0, buffer_size);

    start_time = get_monotonic();

    do {
        result = poll(poll_data, 2, 1000);

        if (result < 0 && errno != EINTR && errno != EAGAIN) {
            perror("poll");
            exit(1);
        }

        if (poll_data[0].revents & POLLIN) {
            do {
                count = read(server_socket, buffer, buffer_size);
            } while (count < 0 && errno == EAGAIN);

            if (count < 0) {
                perror("oneway read");
                exit(1);
            }

            if (count == 0) {
            }

            bytes_read += count;

            current = VPLTime_GetTime();

            if (current - print_time > VPLTime_FromSec(4)) {
                print_time = current;
                printf(" === At %d bytes\n", bytes_read);
            }
        }

        if (poll_data[1].revents & POLLOUT) {
            do {
                write_limit  = min(buffer_size, stream_length - bytes_written);
                count        = write(client_socket, buffer, write_limit);
            } while (count < 0 && errno == EAGAIN);

            if (count < 0) {
                perror("oneway write");
                exit(1);
            }

            bytes_written += count;

            if (bytes_written >= stream_length) {
                poll_data[1].events = 0;
            }
        }
    } while (bytes_read < stream_length);

    total_time     = get_monotonic() - start_time;

    total_seconds  = (double) total_time    / clock_res();
    total_kb       = (double) stream_length / 1024;
    bandwidth      =          total_kb      / total_seconds;

    printf(" === The bandwidth was %6.3lf kbps\n", bandwidth);
    free(buffer);
}

static void
run_twoway(int stream_length, int buffer_size)
{
}

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

    //pxd_declare_t   declare;

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
    int64_t         start;
    int64_t         setup_time;
    int             stream_length;
    char *          end;

    LOGInit("pxd_bw", null);

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

    stream_length = 32 * 1024;

    if (argc > 1) {
        stream_length = strtoll(argv[1], &end, 10);

        if (stream_length < 0 || *end != 0) {
            printf("\"%s\" is not a valid length.\n", argv[1]);
            exit(1);
        }
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
    server_info = &open_server;

    /*
     *  Set up the structures for opening the server CCD connection.
     */
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

    /*
     *  Open the server's ANS connection.
     */
    ans_client = open_ans(open_server.credentials, ans_host, server_ans_blob, server_ans_key);

    /*
     *  Okay, open the server CCD connection.
     */
    server = do_open(&open_server, server_user, server_device, true );
    free(open_server.credentials->opaque);

    open_server.credentials->opaque = null;

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
     *  Wait for the server ANS and PXD opens to complete.
     */
    while (!pxd_login_done || !ans_login_done) {
        sleep(1);
    }

    /*
     *  Get the start time.
     */
    log_info(" === Starting the timer");
    start = get_monotonic();

    /*
     *  Now open the client CCD connection.
     */
    client = do_open(&open_client, client_user, client_device, false);
    free(open_client.credentials->opaque);
    open_client.credentials->opaque = null;

    /*
     *  Make sure the PXD opens worked.
     */
    if (client == null || server == null) {
        error("A pxd_open operation failed.\n");
        exit(1);
    }

    /*
     *  Now try a lookup of the server CCD on the CCD client side.
     *  The lookup should cause PXD to send a server wakeup message
     *  to the server side.
     */
    /*lookup_tries = 4;

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
            nanosleep(&sleep_time, null);
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
        tries = 1000;

        while (!connect_finished && tries-- > 0) {
            nanosleep(&sleep_time, null);
        }

        if (!connect_finished) {
            error("The proxy connection operation didn't complete.\n");
            exit(1);
        }
    } while (!connect_finished && connect_tries-- > 0);

    tries = 100;

    free(connect.creds->opaque);
    connect.creds->opaque = null;

    while ((server_socket < 0 || client_socket < 0) && tries-- > 0) {
        nanosleep(&sleep_time, null);
    }

    if (server_socket < 0 || client_socket < 0) {
        error("The pxd_connect operation failed.");
        exit(1);
    }

    setup_time = get_monotonic() - start;

    printf(" === The pxd_connect operation required %5.3f seconds.\n",
        (double) setup_time / clock_res());

    run_oneway(stream_length, 8192);
    run_twoway(stream_length, 8192);

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

    sleep(2);
    printf("The pxd bandwidth test passed.\n");
    return 0;
}
