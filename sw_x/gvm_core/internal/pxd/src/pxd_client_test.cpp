//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF  Acer Cloud Technology.
//

#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <vplu.h>
#include <vpl_conv.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_user.h>
#include <vplex_assert.h>
#include <vplex_time.h>

#define streq(a, b)     (strcmp((a), (b)) == 0)
#define memeq(a, b, c)  (memcmp((a), (b), (c)) == 0)

static void *  my_malloc(int);
static int     my_VPLDetachableThread_Create(VPLDetachableThreadHandle_t *,
                    VPLTHREAD_FN_DECL (*routine)(void *),
                    void *                    opaque1,
                    const VPLThread_attr_t *  opaque2,
                    const char *              where);
static VPLSocket_t my_VPLSocket_Create(int a1, int a2, int a3);
static char *      my_strdup(const char *);

#define malloc                      my_malloc
#define VPLDetachableThread_Create  my_VPLDetachableThread_Create
#define VPLSocket_Create            my_VPLSocket_Create
#define strdup                      my_strdup

#define pxd_test

#include "pxd_test.h"
#include "pxd_log.cpp"
#include "pxd_packet.cpp"
#include "pxd_mtwist.cpp"
#include "pxd_client.cpp"
#include "pxd_event.cpp"
#include "pxd_util.cpp"

#undef malloc
#undef VPLDetachableThread_Create
#undef VPLSocket_Create
#undef my_strdup

#define cb_started  1
#define cb_invoked  2
#define cb_done     3
#define cb_idle     4

typedef struct {
    int      fd;
    int      type;
    int      sequence;
    int      async_id;
    int      response;
    int      challenge_length;
    int      bad_blob;
    int64_t  connection_id;
    int64_t  tag;

    pxd_address_t *  address;
    pxd_crypto_t *   crypto;
    char         *   challenge;
} send_t;

typedef struct {
    int      fd;
    int      type;
    int      challenge_length;
    int64_t  connection_id;
    int64_t  sequence;
    int64_t  tag;

    pxd_crypto_t *  crypto;
    pxd_client_t *  client;
    char         *  challenge;
} receive_t;

static char  test_region[10]      = "america";
static char  test_instance_id[]   = "test-instance";
static char  mismatch_instance[]  = "no-such-instance";
static char  test_opaque[10]      = "opaque";
static char  test_key[10]         = "key";
static char  test_ip_address[15]  = "ip address";
static int   malloc_countdown     = 0;
static int   fail_create          = false;
static int   thread_countdown     = 0;
static char  pxd_dns[]            = "pxd-c100.test.acer.com";
static char  ans_dns[]            = "ans-c100.test.acer.com";
static char  message[]            = "test message";
static char  cluster_name[]       = "localhost";
static char  no_such_host[]       = "no_such_host";
static int   routine_opaque;
static int   callback_opaque;
static int   server_fd;
static int   server_port;
static int   out_sequence_id;
static char  blob[80];
static char  key [20];
static char  challenge[16];
static char  ip_address[]         = "ip!";
static int   shared_time          = 0;
static char  shared_challenge[16] = "challenge";
static char  lookup_region[]      = "look-america";
static int   lookup_user          = 801;
static int   lookup_device        = 802;
static char  lookup_instance_id[] = "look-instance";
static char  lookup_ans[]         = "look-ans";
static char  lookup_pxd[]         = "look-pxd";
static char  lookup_ip1[]         = "ip1";
static char  lookup_ip2[]         = "ip2";

static void *receive_opaque       = null;
static int   client_user          = 7000;
static int   client_device        = 7887;
static int   server_user          = 8000;
static int   server_device        = 8998;
static int   incoming_login_count =    0;
static int   pxd_rejections       =    0;
static int   ccd_rejections       =    0;
static int   expect_address       = false;

static int   connect_state        = cb_idle;
static int   receive_state        = cb_idle;
static int   lookup_state         = cb_idle;
static int   open_state           = cb_idle;

static char  blob_key[20]         = "blob_key";
static char  proxy_key[20]        = "proxy_key";
static char  handle[]             = "handle";
static char  service_id[]         = "service id";
static char  ticket[]             = "ticket";
static char  client_instance[]    = "client instance";
static char  server_instance[]    = "server side";
static char  client_region[]      = "client region";
static char  server_region[]      = "server region";

static pxd_address_t  lookup_addresses[] =
            {
                { lookup_ip1, 4, 101, 1 },
                { lookup_ip2, 4, 102, 2 }
            };

static char *  proxy_blob;
static int     proxy_length;
static char *  bad_blob;
static int     bad_length;
static char *  connect_blob;
static int     connect_length;
static char    connect_key[20]       = "connect key";

static pxd_crypto_t *  shared_crypto;
static pxd_crypto_t *  connect_crypto;
static pxd_crypto_t *  proxy_crypto;
static pxd_crypto_t *  blob_crypto;
static pxd_id_t *      incoming_id;

static int  expect_incoming_fail = false;

static char *
ip_print(char *address, int length)
{
    char    field[5];
    char *  result;
    int     i;
    int     reserve;

    reserve   = length * 4 + 1;
    result    = (char *) malloc(reserve);
    result[0] = '\0';

    for (i = 0; i < length; i++) {
        snprintf(field, sizeof(field), "%d", (unsigned char) address[i]);
        strncat(result, field, reserve);

        if (i < length - 1) {
            strncat(result, ".", reserve);
        }
    }

    return result;
}

static int
print_socket_name(int fd, const char *who)
{
    int     result;
    char *  ip_address;
    int     ip_port;

    struct sockaddr        local_name;
    socklen_t              local_length;
    struct sockaddr_in  *  ip_name;
    struct sockaddr_in6 *  ipv6_name;
    void *                 name;

    name         = malloc(sizeof(local_name));
    local_length = sizeof(local_name);
    ip_address   = NULL;

    memset(name, 0, sizeof(local_name));

    result = getsockname(fd, (struct sockaddr *) name, &local_length);

    memcpy(&local_name, name, sizeof(local_name));

    if (result < 0) {
        error("getsockname failed\n");
        fflush(stdout);
        perror("getsockname");
        exit(1);
    } else if (local_name.sa_family == PF_INET) {
        ip_name     = (struct sockaddr_in *) name;
        ip_address  = ip_print((char *) &ip_name->sin_addr.s_addr, 4);
        ip_port     = ntohs(ip_name->sin_port);

        log("using IP address %s:%d for %s\n", ip_address, (int) ip_port, who);
    } else if (local_name.sa_family == PF_INET6) {
        ipv6_name   = (struct sockaddr_in6 *) name;
        ip_address  = ip_print((char *) &ipv6_name->sin6_addr.s6_addr, 16);
        ip_port     = ntohs(ipv6_name->sin6_port);

        log("using IPv6 address %s:%d for %s\n", ip_address, (int) ip_port, who);
    } else {
        error("getsockname returned an unknown family\n");
        exit(1);
    }

    free(ip_address);
    free(name);
    return ip_port;
}

static void
check_string(const char *a, const char *b, const char *where)
{
    if (!streq(a, b)) {
        error("check_string failed:  %s vs %s:  %s\n",
            a, b, where);
        exit(1);
    }
}

static void
check_fixed(int64_t a, int64_t b, const char *where)
{
    if (a != b) {
        error("check_fixed failed:  " FMTs64 " vs " FMTs64 ":  %s\n",
            a, b, where);
        exit(1);
    }
}

static void
check_pointer(void *a, void *b, const char *where)
{
    if (a != b) {
        error("check_pointer failed:  %p vs %p:  %s\n",
            a, b, where);
        exit(1);
    }
}

static char *
make_blob(int *length, char *key, int key_length)
{
    pxd_blob_t   input;
    pxd_error_t  error;
    char *       packed_blob;

    memset(&input, 0, sizeof(input));

    input.client_user         = client_user;
    input.client_device       = client_device;
    input.client_instance     = client_instance;
    input.server_user         = server_user;
    input.server_device       = server_device;
    input.server_instance     = server_instance;
    input.create              = VPLTime_GetTime();
    input.key                 = key;
    input.key_length          = key_length;
    input.handle              = handle;
    input.handle_length       = sizeof(handle);
    input.service_id          = service_id;
    input.service_id_length   = sizeof(service_id);
    input.ticket              = ticket;
    input.ticket_length       = sizeof(ticket);

    error.error = 0;

    packed_blob = pxd_pack_blob(length, &input, blob_crypto, &error);

    if (packed_blob == null || error.error != 0) {
        error("pxd_pack_blob failed:  %s\n", error.message);
        exit(1);
    }

    return packed_blob;
}

static pxd_id_t *
make_id(char *region, char *instance_id)
{
    pxd_id_t *  id;

    id = (pxd_id_t *) malloc(sizeof(*id));

    memset(id, 0, sizeof(*id));

    id->region       = test_region;
    id->instance_id  = test_instance_id;
    return id;
}

static void
fill_creds(pxd_cred_t *creds, pxd_id_t *id)
{
    creds->id             = id;
    creds->opaque         = test_opaque;
    creds->key            = test_key;
    creds->opaque_length  = sizeof(test_opaque);
    creds->key_length     = sizeof(test_key);
}

#define fake_host "fake_host"

static void
test_sync_open(void)
{
    pxd_client_t  client;
    VPLSocket_t   socket;

    pxd_max_delay = 1;

    memset(&client, 0, sizeof(client));

    client.is_proxy  = true;
    client.cluster   = (char *) malloc(strlen(fake_host) + 1);

    strcpy(client.cluster, fake_host);
    pxd_create_event(&client.io.event);
    VPLMutex_Init(&client.io.mutex);

    fail_create = true;

    socket = sync_open_socket(&client);

    if (valid(socket)) {
        error("sync_open_socket didn't fail.\n");
        exit(1);
    }

    fail_create = false;
    free(client.cluster);
    pxd_free_event(&client.io.event);
}

static void
check_callback_state(int *state, const char *where)
{
    switch (*state) {
    case cb_started:
        *state = cb_invoked;
        break;

    case cb_invoked:
        error("I got a second %s callback.\n", where);
        exit(1);

    case cb_done:
        error("I got a %s callback when done.\n", where);
        exit(1);

    case cb_idle:
        error("I got a %s callback when idle.\n", where);
        exit(1);

    default:
        break;
    }
}

static void
check_connect_state(void)
{
    check_callback_state(&connect_state, "pxd_connect");
}

static void
check_receive_state(void)
{
    check_callback_state(&receive_state, "pxd_receive");
}

static void
check_lookup_state(void)
{
    check_callback_state(&lookup_state, "pxd_lookup");
}

/*
 *  Implement the callback routines for the PXD client
 *  library.
 */
static void
supply_local_cb(pxd_cb_data_t *data)
{
}

static void
supply_external_cb(pxd_cb_data_t *data)
{
}

static void
connect_done_cb(pxd_cb_data_t *data)
{
    check_connect_state();

    log("connect cb invoked\n"); 

    if (data != null && data->op_opaque != null) {
        if (data->result == 0) {
            error("The connect callback got a zero status.");
            exit(1);
        }

        if (expect_address && data->result == pxd_op_successful) {
            if (data->address_count != 1) {
                error("The connect callback didn't get an external address.\n");
                exit(1);
            }

            check_string(data->addresses[0].ip_address,  lookup_addresses[0].ip_address, "ip"  );
            check_fixed (data->addresses[0].port,        lookup_addresses[0].port,       "port");
        }

        *(int *) data->op_opaque = data->result;
    }
}

static void
incoming_request_cb(pxd_cb_data_t *data)
{
    check_receive_state();

    if (data != null) {
        check_pointer(data->op_opaque,                receive_opaque,                 "opaque");
        check_fixed  (data->address_count,            1,                              "count" );
        check_string (data->addresses[0].ip_address,  lookup_addresses[0].ip_address, "ip"    );
        check_fixed  (data->addresses[0].ip_length,   lookup_addresses[0].ip_length,  "length");
        check_fixed  (data->addresses[0].port,        lookup_addresses[0].port,       "port"  );
        check_fixed  (data->addresses[0].type,        lookup_addresses[0].type,       "type"  );
    }
}

static void
incoming_login_cb(pxd_cb_data_t *data)
{
    if  (data != null) {
        incoming_login_count++;

        if (data->result == 0) {
            error("pxd_login returned a zero status code");
            exit(1);
        }

        check_pointer(data->op_opaque,             receive_opaque,  "receive_opaque" );
        check_fixed  (data->blob->server_user,     server_user,     "server user"    );
        check_fixed  (data->blob->server_device,   server_device,   "server device"  );
        check_string (data->blob->server_instance, server_instance, "server instance");
        check_fixed  (data->blob->client_user,     client_user,     "client user"    );
        check_fixed  (data->blob->client_device,   client_device,   "client device"  );
        check_string (data->blob->client_instance, client_instance, "client instance");
        check_string (data->blob->handle,          handle,          "handle"         );
        check_string (data->blob->ticket,          ticket,          "ticket"         );
        check_string (data->blob->service_id,      service_id,      "server id"      );

        if (data->addresses == null || data->address_count != 1) {
            error("The incoming_login callback didn't get the external address.");
            exit(1);
        }
    }
}

static void
reject_ccd_cb(pxd_cb_data_t *data)
{
    check_connect_state();
    ccd_rejections++;

    if (data->result == 0) {
        error("The reject_ccd callback got a zero status.");
        exit(1);
    }

    *(int *) data->op_opaque = Reject_ccd_credentials;

    if (data->result != pxd_op_failed) {
        error("A login rejection returned %d.\n", (int) data->result);
        exit(1);
    }
}

static void
reject_pxd_cb(pxd_cb_data_t *data)
{
    check_callback_state(&open_state, "reject_pxd");
    pxd_rejections++;
}

static void
lookup_cb(pxd_cb_data_t *data)
{
    pxd_address_t *  addresses;

    check_lookup_state();

    if (data->op_opaque != null) {
        if (data->result != pxd_op_successful) {
            error("A lookup failed!\n");
            exit(1);
        }

        addresses = data->addresses;

        check_string(data->region,        lookup_region,      "lookup cb region"  );
        check_fixed (data->user_id,       lookup_user,        "lookup cb user"    );
        check_fixed (data->device_id,     lookup_device,      "lookup cb device"  );
        check_string(data->instance_id,   lookup_instance_id, "lookup cb instance");
        check_string(data->ans_dns,       lookup_ans,         "lookup cb ans"     );
        check_string(data->pxd_dns,       lookup_pxd,         "lookup cb pxd"     );

        check_fixed (data->address_count, array_size(lookup_addresses), "lookup cb count");

        for (int i = 0; i < array_size(lookup_addresses); i++) {
            check_string(addresses[i].ip_address,  lookup_addresses[i].ip_address, "ip"    );
            check_fixed (addresses[i].ip_length,   lookup_addresses[i].ip_length,  "length");
            check_fixed (addresses[i].port,        lookup_addresses[i].port,       "port"  );
            check_fixed (addresses[i].type,        lookup_addresses[i].type,       "type"  );
        }

        *(int *) data->op_opaque = true;
        log("A lookup completed.\n");
    }
}

static pxd_callback_t callbacks =
    {
        supply_local_cb,
        supply_external_cb,
        connect_done_cb,
        lookup_cb,
        incoming_request_cb,
        incoming_login_cb,
        reject_ccd_cb,
        reject_pxd_cb
    };

static void
fill_open(pxd_open_t *open)
{
    pxd_cred_t *    credentials;
    pxd_id_t *      id;

    credentials = (pxd_cred_t *) malloc(sizeof(*credentials));
    id = (pxd_id_t *) malloc(sizeof(*id));

    memset(open, 0, sizeof(*open));
    memset(credentials, 0, sizeof(*credentials));
    memset(id, 0, sizeof(*id));

    open->credentials      = credentials;
    open->credentials->id  = id;

    id->region       = test_region;
    id->user_id      = 101;
    id->device_id    = 222;
    id->instance_id  = test_instance_id;

    credentials->id             = id;
    credentials->opaque         = blob;
    credentials->opaque_length  = sizeof(blob);
    credentials->key            = key;
    credentials->key_length     = sizeof(key);

    open->cluster_name = cluster_name;
    open->credentials  = credentials;
    open->is_incoming  = true;
    open->callback     = &callbacks;
    open->opaque       = &callback_opaque;
}

static void
flush_accept_queue(void)
{
    struct sockaddr  client_address;
    socklen_t        client_length;

    int  client_fd;
    int  flags;
    int  count;

    memset(&client_address, 0, sizeof(client_address));
    client_length = sizeof(client_address);

    flags  = fcntl(server_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    count  = 0;

    fcntl(server_fd, F_SETFL, flags);

    do {
        client_fd = accept(server_fd, &client_address, &client_length);

        if (client_fd >= 0) {
            close(client_fd);
            count++;
        }
    } while (client_fd >= 0);

    fcntl(server_fd, F_SETFL, flags & ~O_NONBLOCK);

    log(" === flushed %d connections\n", count);
}

static void
write_buffer(int fd, const char *base, int count)
{
    const char *  buffer;
    int           remaining;
    int           tries;

    buffer     = base;
    remaining  = count;
    tries      = 5;

    do {
        count = write(fd, buffer, remaining);

        if (count == 0) {
            tries--;
            sleep(1);
        } else if (count < 0) {
            error("A write operation failed:  %d.\n", count);
            exit(1);
        } else {
            buffer    += count;
            remaining -= count;
        }
    } while (remaining > 0 && tries > 0);

    if (remaining != 0) {
        error("A write operation timed out.\n");
        exit(1);
    }
}

static void
wait_lots(void)
{
    int  counter = 0;

    while (counter-- > 0) {
        sleep(2);
    }
}

static void
free_open(pxd_open_t *open)
{
    free(open->credentials->id);
    open->credentials->id = null;

    free(open->credentials);
    open->credentials = null;
}

static void
fork_proxy(pxd_client_t *client, pthread_t *handle)
{
    int   result;

    result = pthread_create(handle, null, run_proxy, client);

    if (result != 0) {
        perror("pthread_create");
        exit(1);
    }
}

static int
start_connection(void)
{
    struct sockaddr  client_address;

    socklen_t  client_length;
    int        client_fd;

    memset(&client_address, 0, sizeof(client_address));

    client_length    = 0;
    out_sequence_id  = 0;

    client_fd = accept(server_fd, &client_address, &client_length);

    if (client_fd < 0) {
        perror("accept");
        error("No client accepted\n");
        exit(1);
    }

    return client_fd;
}

static void
join_proxy(pxd_client_t *client, pthread_t *handle, int client_fd)
{
    int  result;

    wait_lots();
    close(client_fd);

    do {
        sleep(1);
    } while (pxd_down_events == 0);

    pxd_mutex_lock(&client->io.mutex);
    client->io.stop_now = true;
    pxd_ping(client->io.event, "run proxy test");
    pxd_mutex_unlock(&client->io.mutex);

    flush_accept_queue();

    result = pthread_join(*handle, null);

    if (result != 0) {
        perror("pthread_join");
        exit(1);
    }

    if (pxd_down_events == 0) {
        error("No socket down events!\n");
        exit(1);
    }
}

static void
proxy_kill(pxd_io_t *io, pxd_command_t command, int status)
{
}

static void
test_run_proxy_path(int countdown)
{
    pxd_client_t *  client;
    pthread_t       handle;
    pxd_open_t      open;
    int             client_fd;
    pxd_unpacked_t  unpacked;
    pxd_error_t     error;
    pxd_packet_t *  packet;
    int             bad_challenge;
    int             bad_login_send;
    int             bad_mac_setup;
    int             bad_crypto_setup;

    pxd_max_delay       = 1;
    pxd_down_events     = 0;
    pxd_bad_challenges  = 0;

    fill_open(&open);

    client = (pxd_client_t *) malloc(sizeof(*client));

    memset(client, 0, sizeof(*client));

    client->is_proxy       = true;
    client->cluster        = (char *) malloc(strlen(open.cluster_name) + 1);
    client->creds          = pxd_copy_creds(open.credentials);
    client->callback       = callbacks;
    client->opaque         = &callback_opaque;
    client->io.kill        = proxy_kill;

    strcpy(client->cluster, open.cluster_name);
    pxd_create_event(&client->io.event);
    VPLMutex_Init(&client->io.mutex);

    fork_proxy(client, &handle);
    sleep(1);
    client->is_detached = true;

    pxd_down_events  = 0;
    client_fd        = start_connection();
    sleep(1);

    if (countdown-- <= 0) {
        join_proxy(client, &handle, client_fd);
        free_open(&open);
        return;
    }

    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.type             = Send_pxd_challenge;
    unpacked.challenge        = challenge;
    unpacked.challenge_length = sizeof(challenge);
    unpacked.pxd_dns          = open.cluster_name;
    unpacked.connection_time  = ++shared_time;
    unpacked.address          = ip_address;
    unpacked.address_length   = sizeof(ip_address);
    unpacked.port             = 42;
    unpacked.connection_tag   = 0;

    packet = pxd_pack(&unpacked, null, &error);

    bad_challenge = countdown-- == 0;

    if (bad_challenge) {
        malloc_countdown = 3;
    }

    bad_mac_setup = countdown-- == 0;

    if (bad_mac_setup) {
        malloc_countdown = 5;
    }

    bad_crypto_setup = countdown-- == 0;

    if (bad_crypto_setup) {
        malloc_countdown = 7;
    }

    bad_login_send = countdown-- == 0;

    if (bad_login_send) {
        malloc_countdown = 8;
    }

    write_buffer(client_fd, packet->base, packet->length);

    if (countdown < 0) {
        join_proxy(client, &handle, client_fd);

        if (malloc_countdown > 0) {
            error("run_proxy didn't trigger a malloc failure.\n");
            exit(1);
        }

        if (bad_challenge && pxd_bad_challenges == 0) {
            error("run_proxy didn't record a bad challenge.\n");
            exit(1);
        } else if (bad_mac_setup && pxd_bad_mac_setups == 0) {
            error("run_proxy didn't record a mac setup failure.\n");
            exit(1);
        } else if (bad_crypto_setup && pxd_bad_crypto_setups == 0) {
            error("run_proxy didn't record a crypto setup failure.\n");
            exit(1);
        } else if (bad_login_send && pxd_bad_login_sends == 0) {
            error("run_proxy didn't record a login send failure.\n");
            exit(1);
        } 
        
        if (malloc_countdown > 0) {
            error("run_proxy didn't record a malloc failure.\n");
            exit(1);
        }

        free_open(&open);
        pxd_free_packet(&packet);
        return;
    }

    free_open(&open);
    pxd_free_packet(&packet);
}

static void
test_run_proxy(void)
{
    for (int i = 0; i <= 4; i++) {
        test_run_proxy_path(i);
    }
}

static void
test_copy(void)
{
    pxd_cred_t *     creds;
    pxd_cred_t *     new_creds;
    pxd_address_t *  address;
    pxd_address_t *  new_address;
    pxd_id_t *       id;
    pxd_id_t *       new_id;
    pxd_cred_t       cred_area;
    pxd_address_t    address_area;
    int              i;

    id               = make_id(test_region, test_instance_id);
    id->user_id      = 810;
    id->device_id    = 799;

    new_id = pxd_copy_id(id);

    if
    (
        id->user_id   != new_id->user_id
    ||  id->device_id != new_id->device_id
    ||  !streq(id->region,      new_id->region)
    ||  !streq(id->instance_id, new_id->instance_id)
    ) {
        error("pxd_copy_id failed\n");
        exit(1);
    }

    pxd_free_id(&new_id);

    creds = &cred_area;
    fill_creds(creds, id);

    for (i = 1; i <= 6; i++) {
        malloc_countdown = i;
        new_creds = pxd_copy_creds(creds);

        if (new_creds != null) {
            error("pxd_copy_creds didn't report a malloc failure at %d.\n", i);
            exit(1);
        }

        pxd_free_creds(&new_creds);
    }

    new_creds = pxd_copy_creds(creds);
    new_id    = new_creds->id;

    if
    (
        id->user_id   != new_id->user_id
    ||  id->device_id != new_id->device_id
    ||  !streq(id->region,      new_id->region)
    ||  !streq(id->instance_id, new_id->instance_id)
    ) {
        error("pxd_copy_creds failed on the id\n");
        exit(1);
    }

    if
    (
        creds->opaque_length != new_creds->opaque_length
    ||  creds->key_length    != new_creds->key_length
    ||  !memeq(creds->opaque, new_creds->opaque, creds->opaque_length)
    ||  !memeq(creds->key,    new_creds->key,    creds->key_length)
    ) {
        error("pxd_copy_creds failed\n");
        exit(1);
    }

    pxd_free_creds(&new_creds);
    free(id);

    address = &address_area;

    address->ip_address = test_ip_address;
    address->ip_length  = sizeof(test_ip_address);
    address->port       = 100;
    address->type       = 222;

    for (i = 1; i <= 2; i++) {
        malloc_countdown = i;
        new_address = pxd_copy_address(address);

        if (new_address != null) {
            error("pxd_copy_address didn't report a malloc failure at %d.\n", i);
            exit(1);
        }

        pxd_free_address(&new_address);
    }

    new_address = pxd_copy_address(address);

    pxd_free_address(&new_address);
}

static void
test_verify(void)
{
    pxd_verify_t  verify;
    pxd_error_t   error;

    verify.opaque         = connect_blob;
    verify.opaque_length  = connect_length;
    verify.server_key     = blob_key;
    verify.server_length  = sizeof(blob_key);

    if (!pxd_verify(&verify, &error)) {
        error("pxd_verify failed on a valid blob.");
        exit(1);
    }

    verify.server_key     = proxy_key;
    verify.server_length  = sizeof(proxy_key);

    if (pxd_verify(&verify, &error)) {
        error("pxd_verify didn't detect a bad blob.");
        exit(1);
    }

    malloc_countdown = 1;

    if (pxd_verify(&verify, &error) || malloc_countdown > 0) {
        error("pxd_verify didn't fail in malloc.");
        exit(1);
    }

    if (error.error != VPL_ERR_NOMEM) {
        error("pxd_verify didn't report a malloc error.");
        exit(1);
    }
}

static void
make_busy(pxd_client_t *client)
{
    for (int i = 0; i < array_size(client->io.outstanding); i++) {
        client->io.outstanding[i].valid     = true;
        client->io.outstanding[i].async_id  = i + 100;
        client->io.outstanding[i].start     = VPLTime_GetTime() + 5000;
    }
}

static void
make_idle(pxd_client_t *client)
{

    for (int i = 0; i < array_size(client->io.outstanding); i++) {
        client->io.outstanding[i].valid = false;
    }
}

static void
test_open_malloc(void)
{
    pxd_client_t *  client;
    pxd_open_t      open;
    pxd_error_t     error;

    fill_open(&open);

    for (int i = 1; i <= 11; i++) {
        malloc_countdown = i;
        client = pxd_open(&open, &error);

        if (client != null) {
            error("pxd_open didn't fail in malloc testing, at %d\n", i);
            exit(1);
        }

        if (malloc_countdown > 0) {
            error("The malloc trigger failed for pxd_open, at %d\n", i);
            exit(1);
        }
    }

    free_open(&open);
}

static void
test_misc_malloc(void)
{
    int  pass;

    malloc_countdown = 1;

    pass = !send_start_proxy(null);

    if (!pass || malloc_countdown > 0) {
        error("The malloc trigger failed for send_start_proxy\n");
        exit(1);
    }
}
static void
test_open_close(void)
{
    pxd_client_t *  client;
    pxd_open_t      open;
    pxd_error_t     error;
    int             tries;

    fill_open(&open);
    pxd_min_delay    =  1;
    pxd_max_delay    = 10;
    tries            = 10;
    pxd_force_socket =  3;   // TODO force a connection failure.

    client = pxd_open(&open, &error);

    while (pxd_force_socket > 0 && tries-- > 0) {
        sleep(1);
    }

    if (pxd_force_socket > 0) {
        error("pxd_force_socket didn't trigger.\n");
        exit(1);
    }

    pxd_close(&client, true, &error);

    if (client != null) {
        error("pxd_close didn't clear the client (test_open_close)!\n");
        exit(1);
    }

    thread_countdown = 1;
    client = pxd_open(&open, &error);

    if (client != null || thread_countdown > 0) {
        error("thread_countdown didn't trigger in pxd_open!\n");
        exit(1);
    }

    /*
     *  Now test thread shutdown.
     */
    client = pxd_open(&open, &error);

    pxd_stop_connection(client, &error, true);

    if (error.error != 0) {
        error("pxd_stop_connection failed:  %s\n", error.message);
        exit(1);
    }

    error.error = 1;

    pxd_close(&client, true, &error);

    if (error.error != 0) {
        error("The close after the hand-off test failed!\n");
        exit(1);
    }

    /*
     *  Two closes should be fine.
     */
    error.error = 1;

    pxd_close(&client, true, &error);

    if (error.error != 0) {
        error("The re-close failed!\n");
        exit(1);
    }

    free_open(&open);
}

static void
test_declare(void)
{
    pxd_client_t *    client;
    pxd_unpacked_t *  unpacked;
    pxd_open_t        open;
    pxd_declare_t     declare;
    pxd_error_t       error;

    fill_open(&open);
    open.cluster_name = no_such_host;
    memset(&declare, 0, sizeof(declare));

    declare.ans_dns        = ans_dns;
    declare.pxd_dns        = pxd_dns;
    declare.address_count  = 1;

    declare.addresses[0].ip_address = test_ip_address;
    declare.addresses[0].ip_length  = strlen(test_ip_address);
    declare.addresses[0].port       = 1234;
    declare.addresses[0].type       = 0;

    open.is_incoming = false;

    client = pxd_open(&open, &error);

    pxd_declare(client, &declare, &error);

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_declare failed to detect a bad client!\n");
        exit(1);
    }

    pxd_close(&client, true, &error);

    /*
     *  Now test a valid client.
     */
    open.is_incoming = true;

    client = pxd_open(&open, &error);
    sleep(1);   /* give the thread some time to start */
    pxd_stop_connection(client, &error, true);
    free_open(&open);

    client->io.tag = 0;

    /*
     *  Check for busy table problems.
     */
    make_busy(client);

    pxd_declare(client, &declare, &error);

    if (error.error != VPL_ERR_BUSY) {
        error("pxd_declare didn't fail with a busy table!\n");
        exit(1);
    }

    make_idle(client);

    pxd_mutex_lock  (&client->io.mutex);
    // TODO pxd_free_packet (&client->declare);
    pxd_mutex_unlock(&client->io.mutex);

    pxd_declare(client, &declare, &error);

    if (error.error != 0) {
        error("pxd_declare failed!\n");
        exit(1);
    }

    unpacked = pxd_unpack(client->io.crypto, client->io.queue_head, null, 0);

    if (unpacked == null) {
        error("pxd_unpack(Declare_server) failed!\n");
        exit(1);
    }

    pxd_free_unpacked(&unpacked);

    /*
     *  Discard any packets in the output queue.
     */
    discard_tcp_queue(client);

    /*
     *  Test for malloc error handling.
     */
    for (int i = 1; i <= 3; i++) {
        malloc_countdown = i;

        pxd_declare(client, &declare, &error);

        if (error.error != VPL_ERR_NOMEM || malloc_countdown > 0) {
            error("pxd_declare should have failed in malloc! (%d)\n", i);
            exit(1);
        }
    }

    /*
     *  Force a connection start failure.
     */
    pxd_stop_connection(client, &error, true);

    thread_countdown = 1;

    pxd_declare(client, &declare, &error);

    if (error.error == 0 || thread_countdown > 0) {
        error("pxd_declare should have failed in pxd_start_connection!\n");
        exit(1);
    }

    pxd_close(&client, true, &error);
}

static void
test_receive(void)
{
    pxd_id_t *        id;
    pxd_cred_t        creds;
    pxd_client_t      client;
    pxd_client_t *    proxy;
    pxd_open_t        open;
    pxd_error_t       error;
    pxd_unpacked_t    unpacked;
    pxd_unpacked_t *  blob_unpacked;
    pxd_receive_t     receive;
    pxd_crypto_t *    crypto;
    pxd_address_t     address;

    int     tries;

    pxd_min_delay  = 1;
    pxd_max_delay  = 1;

    memset(&client,   0, sizeof(client  ));
    memset(&error,    0, sizeof(error   ));
    memset(&receive,  0, sizeof(receive ));
    memset(&receive,  0, sizeof(receive ));
    memset(&address,  0, sizeof(address ));
    memset(&unpacked, 0, sizeof(unpacked));

    id               = make_id(test_region, test_instance_id);
    id->user_id      = 810;
    id->device_id    = 799;

    fill_creds(&creds, id);

    address.ip_address        = ip_address;
    address.ip_length         = sizeof(ip_address);
    address.port              = 65;
    client.external_address   = &address;

    creds.opaque = connect_blob;

    receive.buffer            = message;
    receive.buffer_length     = sizeof(message);
    receive.server_key        = blob_key;
    receive.server_key_length = sizeof(blob_key);
    receive.client            = null;
    receive.opaque            = &error;
    receive_opaque            = &error;

    error.error = 0;

    pxd_receive(&receive, &error);

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_receive didn't detect a null client\n");
        exit(1);
    }

    error.error    = 0;
    receive.client = &client;

    pxd_receive(&receive, &error);

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_receive didn't detect an invalid client\n");
        exit(1);
    }

    error.error         = 0;
    client.open         = open_magic;
    client.creds        = &creds;
    client.cluster      = cluster_name;
    client.instance_id  = server_instance;
    client.is_incoming  = false;
    client.callback     = callbacks;

    pxd_receive(&receive, &error);

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_receive failed to detect an outgoing client!\n");
        exit(1);
    }

    client.is_incoming = true;

    pxd_receive(&receive, &error);

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_receive failed to detect an invalid message\n");
        exit(1);
    }

    unpacked.type             = pxd_connect_request;
    unpacked.user_id          = client_user;
    unpacked.device_id        = client_device;
    unpacked.request_id       = 803;
    unpacked.instance_id      = client_instance;
    unpacked.pxd_dns          = no_such_host;
    unpacked.server_instance  = client.instance_id;
    unpacked.address_count    = 1;
    unpacked.addresses        = lookup_addresses;

    receive.buffer = pxd_pack_ans(&unpacked, &receive.buffer_length);

    for (int i = 1; i <= 4; i++) {
        malloc_countdown = i;
        error.error = 0;

        pxd_receive(&receive, &error);

        if (error.error == 0 || malloc_countdown > 0) {
            error("pxd_receive should have failed in malloc! (%d)\n", i);
            exit(1);
        }
    }

    free(receive.buffer);
    unpacked.server_instance  = mismatch_instance;
    receive.buffer            = pxd_pack_ans(&unpacked, &receive.buffer_length);
    pxd_wrong_instance        = 0;

    pxd_receive(&receive, &error);

    if (error.error != 0 || pxd_wrong_instance != 1) {
        error("pxd_receive failed to detect an instance mismatch.\n");
        exit(1);
    }

    free(receive.buffer);
    unpacked.server_instance  = client.instance_id;
    receive.buffer            = pxd_pack_ans(&unpacked, &receive.buffer_length);
    pxd_proxy_shutdowns       = 0;

    log("Starting the receive thread shutdown test.\n");
    receive_state = cb_started;

    pxd_receive(&receive, &error);

    if (error.error != 0) {
        error("pxd_receive failed on a valid message\n");
        exit(1);
    }

    tries = 10 + pxd_idle_limit;

    while (pxd_proxy_shutdowns == 0 && tries-- > 0) {
        sleep(1);
    }

    if (pxd_proxy_shutdowns == 0 || receive_state != cb_invoked) {
        error("The proxy client didn't complete.\n");
        exit(1);
    }

    receive_state = cb_done;
    fill_open(&open);

    blob_unpacked  = pxd_unpack_ans(receive.buffer, receive.buffer_length, &error);

    if (error.error != 0) {
        error("pxd_unpack_ans failed:  %s.\n", error.message);
        exit(1);
    }

    /*
     *  Now force an error in the extra setup code for the proxy
     *  clients.
     */
    for (int i = 1; i <= 12; i++) {
        crypto = pxd_create_crypto(creds.key, creds.key_length, 13);
        malloc_countdown = i;

        proxy = pxd_do_open(&open, &error, blob_unpacked, crypto);

        if (proxy != null || malloc_countdown > 0) {
            error("pxd_do_open should have failed in malloc! (%d)\n", i);
            exit(1);
        }
    }

    pxd_free_unpacked(&blob_unpacked);
    free(receive.buffer);
    free_open(&open);
    sleep(1);   // Hopefully, the thread will stop completely for valgrind...
    receive_opaque = null;
    free(id);
}

static void
test_lookup(void)
{
    pxd_client_t *   client;
    pxd_open_t       open;
    pxd_id_t *       id;
    pxd_declare_t *  declare;
    pxd_error_t      error;

    fill_open(&open);

    id               = make_id(test_region, test_instance_id);
    id->user_id      = 810;
    id->device_id    = 799;

    memset(&declare, 0, sizeof(declare));

    /*
     *  Okay, try testing pxd_lookup.
     */
    client = pxd_open(&open, &error);

    lookup_state = cb_started;
    pxd_lookup(client, id, null, &error);

    if (error.error != 0) {
        error("pxd_lookup failed!\n");
        exit(1);
    }

    pxd_close(&client, true, &error);

    if (lookup_state != cb_invoked) {
        error("The pxd_lookup callback didn't happen!\n");
        exit(1);
    }

    lookup_state = cb_done;
    client = pxd_open(&open, &error);
    free_open(&open);

    /*
     *  Test for malloc error handling and queue shutdown.
     */
    pxd_stop_connection(client, &error, false);

    for (int i = 1; i <= 3; i++) {
        malloc_countdown = i;

        pxd_lookup(client, id, null, &error);

        if (error.error != VPL_ERR_NOMEM || malloc_countdown > 0) {
            error("pxd_lookup should have failed in malloc (%d)!\n", i);
            exit(1);
        }
    }

    pxd_force_queue = true;

    pxd_lookup(client, id, null, &error);

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_lookup should have failed in pxd_queue_packet.\n");
        exit(1);
    }

    pxd_force_queue = false;

    free(id);

    /*
     *  Force a connection start failure.
     */
    pxd_stop_connection(client, &error, true);
    thread_countdown = 1;

    pxd_lookup(client, id, null, &error);

    if (error.error != VPL_ERR_INVALID || thread_countdown > 0) {
        error("pxd_lookup should have failed in pxd_start_connection!\n");
        exit(1);
    }

    pxd_close(&client, false, &error);
}

static void
test_connect(void)
{
    pxd_client_t *   client;
    pxd_open_t       open;
    pxd_id_t *       id;
    pxd_error_t      error;
    pxd_connect_t    connect;
    pxd_cred_t       creds;

    expect_address   = false;

    id               = make_id(test_region, test_instance_id);
    id->user_id      = 810;
    id->device_id    = 799;

    fill_open(&open);

    open.is_incoming = false;

    creds.id             = id;
    creds.opaque         = test_opaque;
    creds.key            = test_key;
    creds.opaque_length  = sizeof(test_opaque);
    creds.key_length     = sizeof(test_key);

    /*
     *  Okay, try testing pxd_lookup.
     */
    client = pxd_open(&open, &error);

    memset(&connect, 0, sizeof(connect));
 
    connect.target         = id;
    connect.creds          = &creds;
    connect.opaque         = &routine_opaque;
    connect.address_count  = 1;

    connect.addresses[0].ip_address  = test_ip_address;
    connect.addresses[0].ip_length   = strlen(test_ip_address);
    connect.addresses[0].port        = 42;
    connect.addresses[0].type        = 40;

    for (int i = 1; i <= 11; i++) {
        malloc_countdown = i;
        pxd_connect(client, &connect, &error);

        if (error.error != VPL_ERR_NOMEM) {
            error("pxd_connect didn't report a malloc failure at %d\n", i);

            if (error.error != 0) {
                error("pxd_connect returned %s\n", error.message);
            }

            exit(1);
        }
    }

    /*
     *  Force a connection start failure.  First, stop the
     *  thread.
     */
    error.error = 0;
    pxd_stop_connection(client, &error, true);

    if (error.error != 0) {
        error("pxd_stop_connection failed.\n");
        exit(1);
    }

    /*
     *  Force a thread error.  Also, try setting the PXD
     *  DNS name in the connect structure.
     */
    connect.pxd_dns = cluster_name;
    thread_countdown = 1;

    pxd_connect(client, &connect, &error);

    if (error.error != VPL_ERR_INVALID || thread_countdown > 0) {
        error("pxd_declare should have failed in pxd_start_connection!\n");
        exit(1);
    }

    pxd_force_queue = true;
    pxd_connect(client, &connect, &error);
    pxd_force_queue = false;

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_connect should have failed in pxd_queue_packet.\n");
        exit(1);
    }

    pxd_connect(client, &connect, &error);

    if (error.error != 0) {
        error("pxd_connect failed!\n");
        exit(1);
    }

    pxd_connect(client, &connect, &error);

    if (error.error != VPL_ERR_BUSY) {
        error("pxd_connect failed to detect a busy client!\n");
        exit(1);
    }

    pxd_free_crypto(&client->blob_crypto);
    pxd_free_id    (&client->connect_target);

    make_busy(client);

    pxd_connect(client, &connect, &error);

    if (error.error != VPL_ERR_BUSY) {
        error("pxd_connect failed to detect a full queue!\n");
        exit(1);
    }

    make_idle(client);

    pxd_close(&client, true, &error);

    if (client != null) {
        error("pxd_close didn't clear the client!\n");
        exit(1);
    }

    open.is_incoming = true;
    client = pxd_open(&open, &error);

    if (error.error != 0) {
        error("pxd_open for incoming failed!\n");
        exit(1);
    }

    pxd_connect(client, &connect, &error);

    if (error.error != VPL_ERR_INVALID) {
        error("pxd_connect should fail on an incoming client.\n");
        exit(1);
    }

    pxd_close(&client, true, &error);

    if (error.error != 0) {
        error("pxd_close on incoming failed.\n");
        exit(1);
    }

    free_open(&open);
    free(id);
}

static int
start_server(void)
{
    struct hostent *    address;
    struct sockaddr_in  server_address;

    int  server_length;
    int  error;
    int  port;

    address = gethostbyname(cluster_name);

    if (address == null) {
        perror("gethostbyname");
        error("No host\n");
        exit(1);
    }

    server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_fd < 0) {
        perror("socket");
        error("No socket\n");
        exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    memcpy(&server_address.sin_addr, address->h_addr, address->h_length);

    server_address.sin_family = AF_INET;
    server_address.sin_port   = htons(0);

    server_length = sizeof(server_address);

    /*
     *  Try to get a port.
     */
    error = bind(server_fd, (struct sockaddr *) &server_address, server_length);

    if (error) {
        perror("bind");
        error("bind failed\n");
        exit(1);
    }

    error = listen(server_fd, 4);

    if (error) {
        perror("listen");
        error("listen failed\n");
        exit(1);
    }

    port = print_socket_name(server_fd, "server");
    return port;
}

static void
read_buffer(int fd, char *base, int count)
{
    char *  buffer;
    int     remaining;
    int     tries;

    buffer    = base;
    remaining = count;
    tries     = 10;

    do {
        count = read(fd, buffer, remaining);

        if (count == 0 || (count < 0 && errno == EAGAIN)) {
            tries--;
            sleep(1);
        } else if (count < 0) {
            error("A socket receive failed:  %d, %d.\n", count, (int) errno);
            perror("read");
            exit(1);
        } else {
            buffer    += count;
            remaining -= count;
        }
    } while (remaining > 0 && tries > 0);

    if (remaining != 0) {
        error("A read_buffer operation timed out.\n");
        exit(1);
    }
}

static int
send_packet(send_t *send)
{
    pxd_unpacked_t  unpacked;
    pxd_packet_t *  packet;
    pxd_error_t     error;

    int  is_signed;

    is_signed = false;

    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.type            = send->type;
    unpacked.out_sequence    = send->sequence;
    unpacked.async_id        = send->async_id;
    unpacked.connection_tag  = send->tag;

    switch (send->type) {
    case Send_pxd_challenge:
    case Send_ccd_challenge:
        unpacked.challenge        = challenge;
        unpacked.challenge_length = sizeof(challenge);
        unpacked.pxd_dns          = cluster_name;
        unpacked.connection_time  = ++shared_time;
        unpacked.address          = ip_address;
        unpacked.address_length   = sizeof(ip_address);
        unpacked.port             = 42;
        unpacked.connection_tag   = 0;
        send->tag                 = unpacked.connection_time;
        break;

    case Send_ccd_login:
        if (send->challenge != null) {
            unpacked.challenge        = send->challenge;
            unpacked.challenge_length = send->challenge_length;
            unpacked.connection_id    = send->connection_id;
        } else {
            unpacked.challenge        = challenge;
            unpacked.challenge_length = sizeof(challenge);
        }

        if (send->bad_blob) {
            unpacked.blob             = bad_blob;
            unpacked.blob_length      = bad_length;
        } else {
            unpacked.blob             = proxy_blob;
            unpacked.blob_length      = proxy_length;
        }

        is_signed                 = true;
        break;

    case Send_server_declaration:
        unpacked.region        = lookup_region;
        unpacked.user_id       = lookup_user;
        unpacked.device_id     = lookup_device;
        unpacked.instance_id   = lookup_instance_id;
        unpacked.ans_dns       = lookup_ans;
        unpacked.pxd_dns       = lookup_pxd;
        unpacked.address_count = array_size(lookup_addresses);
        unpacked.addresses     = lookup_addresses;
        is_signed              = true;
        break;

    case Reject_ccd_credentials:
    case Reject_pxd_credentials:
        is_signed = false;
        break;

    case Send_pxd_response:
    case Send_ccd_response:
        unpacked.response = send->response;
        is_signed         = true;

        if (send->address != null) {
            unpacked.address         = send->address->ip_address;
            unpacked.address_length  = send->address->ip_length;
            unpacked.port            = send->address->port;
        }

        break;

    case Set_pxd_configuration:
        unpacked.proxy_retries   = pxd_proxy_retries   + 1;
        unpacked.proxy_wait      = pxd_proxy_wait      + 1;
        unpacked.idle_limit      = pxd_idle_limit      + 1;
        unpacked.sync_io_timeout = pxd_sync_io_timeout + 1;
        unpacked.min_delay       = pxd_min_delay;
        unpacked.max_delay       = pxd_max_delay;
        unpacked.thread_retries  = pxd_thread_retries  + 1;
        unpacked.max_packet_size = pxd_max_packet_size + 1;
        unpacked.max_encrypt     = pxd_max_encrypt     + 1;
        unpacked.partial_timeout = pxd_partial_timeout + 1;
        unpacked.reject_limit    = pxd_reject_limit    + 1;
        is_signed                = true;
        break;

    default:
        error("send_packet doesn't support type %s (%d).\n",
            pxd_packet_type(send->type),
            send->type);
        exit(1);
    }

    packet = pxd_pack(&unpacked, null, &error);

    if (error.error != 0) {
        error("I couldn't make a packet of type %d.\n", send->type);
        exit(1);
    }

    if (is_signed) {
        pxd_sign_packet(send->crypto, packet->base, packet->length);
    }

    write_buffer(send->fd, packet->base, packet->length);

    log("made packet %s (%d), async id %d, sequence %d\n",
              pxd_packet_type(unpacked.type),
        (int) unpacked.type,
        (int) unpacked.async_id,
        (int) unpacked.out_sequence);

    pxd_free_packet(&packet);
    return true;
}

static char *
copy_bytes(char *source, int length)
{
    char *  result;

    result = (char *) malloc(length);
    memcpy(result, source, length);
    return result;
}

static int
receive_packet(receive_t *receive)
{
    char      buffer[2000];
    uint16_t  packet_size;
    int       async_id;
    int       remaining;
    int       fd;

    pxd_packet_t      packet;
    pxd_unpacked_t *  unpacked;
    pxd_client_t *    client;

    fd     = receive->fd;
    client = receive->client;

    /*
     *  Okay, read the check size from the socket.
     */
    read_buffer(fd, buffer, sizeof(uint16_t));

    memcpy(&packet_size, buffer, sizeof(packet_size));
    packet_size = VPLConv_ntoh_u16(~packet_size);

    if (packet_size > sizeof(buffer) - sizeof(uint16_t)) {
        error("read(%d) returned a check size of %d\n", fd, packet_size);
        exit(1);
    }

    /*
     *  Now that we know the packet size, we can read the rest of the
     *  packet.
     */
    remaining = packet_size - sizeof(uint16_t);

    read_buffer(fd, buffer + (packet_size - remaining), remaining);

    memset(&packet, 0, sizeof(packet));

    packet.base   = buffer;
    packet.length = packet_size;

    unpacked = pxd_unpack(receive->crypto, &packet, &receive->sequence, receive->tag);

    if (unpacked == null) {
        error("unpack failed in receive_packet\n");
        exit(1);
    }

    if (unpacked->type != receive->type) {
        error("received the wrong type:  %s (%d) vs %s (%d)\n",
                  pxd_packet_type(receive->type),
                  receive->type,
                  pxd_packet_type(unpacked->type),
            (int) unpacked->type);
        exit(1);
    }

    async_id = unpacked->async_id;

    log("received packet %s (%d), async id %d\n",
        pxd_packet_type(unpacked->type), (int) unpacked->type, async_id);

    switch (unpacked->type) {
    case Declare_server:
        check_string(unpacked->region,        client->region,      "Declare_server region");
        check_string(unpacked->instance_id,   client->instance_id, "Declare_server instance");
        check_string(unpacked->ans_dns,       cluster_name,        "Declare_server ANS");
        check_string(unpacked->pxd_dns,       cluster_name,        "Declare_server PXD");
        check_fixed (unpacked->user_id,       client->user_id,     "Declare_server user");
        check_fixed (unpacked->device_id,     client->device_id,   "Declare_server device");
        check_fixed (unpacked->address_count, 1,                   "address_count");
        break;

    case Query_server_declaration:
        check_string(unpacked->region,        lookup_region,      "Query_server region");
        check_fixed (unpacked->user_id,       lookup_user,        "Query_server user");
        check_fixed (unpacked->device_id,     lookup_device,      "Query_server device");
        check_string(unpacked->instance_id,   lookup_instance_id, "Query_server instance");
        break;

    case Send_ccd_challenge:
        log("The ccd tag is " FMTs64 ".\n", unpacked->connection_time);
        receive->tag = unpacked->connection_time;

        receive->challenge =
            copy_bytes(unpacked->challenge, unpacked->challenge_length);

        receive->connection_id     = unpacked->connection_id;
        receive->challenge_length  = unpacked->challenge_length;
        break;
    }

    pxd_free_unpacked(&unpacked);
    return async_id;
}

static void
declare_live(pxd_client_t *client)
{
    pxd_declare_t  declare;
    pxd_error_t    error;

    memset(&declare, 0, sizeof(declare));

    declare.ans_dns        = cluster_name;
    declare.pxd_dns        = cluster_name;
    declare.address_count  = 1;

    declare.addresses[0].ip_address = test_ip_address;
    declare.addresses[0].ip_length  = strlen(test_ip_address);
    declare.addresses[0].port       = 1234;
    declare.addresses[0].type       = 0;

    pxd_declare(client, &declare, &error);

    if (error.error != 0) {
        error("live pxd_declare failed:  %s\n", error.message);
        exit(1);
    }
}

static void
query_live(pxd_client_t *client, int *done)
{
    pxd_id_t     id;
    pxd_error_t  error;

    id.region       = lookup_region;
    id.user_id      = lookup_user;
    id.device_id    = lookup_device;
    id.instance_id  = lookup_instance_id;

    lookup_state = cb_started;

    pxd_lookup(client, &id, done, &error);

    if (error.error != 0) {
        error("query_live:  pxd_lookup failed:  %s\n", error.message);
        exit(1);
    }
}

static void
test_mt(void)
{
    mtwist_t  mtwist;

    mtwist_init(&mtwist, 0x1020304);

    for (int i = 0; i < 1000; i++) {
        mtwist_next(&mtwist, 513);
    }
}

static void
login_callback(pxd_cb_data_t *data)
{
    *(int *) data->op_opaque = true;
}

static void
test_login(void)
{
    pxd_login_t  login;
    pxd_error_t  error;
    pxd_cred_t   creds;
    pxd_id_t *   id;

    int  done;
    int  tries;

    done = 0;

    id = make_id(test_region, test_instance_id);

    id->user_id      = 810;
    id->device_id    = 799;

    fill_creds(&creds, id);

    memset(&login, 0, sizeof(login));
    memset(&error, 0, sizeof(error));   // keep GCC happy

    login.socket             = VPLSOCKET_INVALID;
    login.callback           =   login_callback;
    login.opaque             = &done;
    login.is_incoming        = false;
    login.credentials        = &creds;
    login.server_key         = blob_key;
    login.server_key_length  = sizeof(blob_key);
    login.server_id          = id;

    for (int i = 1; i <= 2; i++) {
        malloc_countdown  = i;
        error.error       = 0;
        login.is_incoming = false;

        pxd_login(&login, &error);

        if (error.error != VPL_ERR_NOMEM || malloc_countdown > 0) {
            error("An outgoing pxd_login failed to trigger a malloc failure.\n");
            exit(1);
        }

        malloc_countdown  = i;
        error.error       = 0;
        login.is_incoming = true;

        pxd_login(&login, &error);

        if (error.error != VPL_ERR_NOMEM || malloc_countdown > 0) {
            error("An incoming pxd_login failed to trigger a malloc failure.\n");
            exit(1);
        }
    }

    thread_countdown = 1;
    error.error = 0;

    pxd_login(&login, &error);

    if (error.error == 0 || thread_countdown > 0) {
        error("An incoming pxd_login failed to trigger a thread failure.\n");
        exit(1);
    }

    error.error = 1;

    pxd_login(&login, &error);

    if (error.error != 0) {
        error("pxd_login failed:  %s\n", error.message);
        exit(1);
    }

    tries = 4;

    while (!done && tries-- > 0) {
        sleep(1);
    }

    if (!done) {
        error("pxd_login didn't invoke the callback\n");
        exit(1);
    }

    free(id);
}

static void
test_mutex(void)
{
    VPLMutex_t  mutex;

    if (pxd_lock_errors != 0) {
        error("The test had lock errors before the mutex tests.\n");
        exit(1);
    }

    memset(&mutex, 0, sizeof(mutex));

    pxd_mutex_lock(&mutex);

    if (pxd_lock_errors != 1) {
        error("pxd_mutex_lock failed to detect a bad mutex\n");
        exit(1);
    }

    pxd_mutex_init(null);

    if (pxd_lock_errors != 2) {
        error("pxd_mutex_init failed to detect a valid mutex\n");
        exit(1);
    }

    pxd_lock_errors = 0;
}

static void
test_send_login(void)
{
    pxd_open_t      open;
    pxd_client_t *  client;
    pxd_error_t     error;
    pxd_unpacked_t  unpacked;

    int  pass;

    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.challenge        = shared_challenge;
    unpacked.challenge_length = sizeof(shared_challenge);

    fill_open(&open);

    client = pxd_open(&open, &error);
    free_open(&open);

    if (error.error != 0) {
        error("pxd_open failed in test_send_login\n");
        exit(1);
    }

    for (int i = 1; i <= 2; i++) {
        malloc_countdown = i;

        pass = send_login(client, Send_pxd_login, &unpacked);

        if (pass || malloc_countdown > 0) {
            error("send_login malloc failed to trigger at %d\n", i);
            exit(1);
        }
    }

    pxd_force_queue = true;

    pass = send_login(client, Send_pxd_login, &unpacked);

    if (pass) {
        error("send_login failed to trigger an enqueue error\n");
        exit(1);
    }

    pxd_force_queue = false;
    pxd_close(&client, true, &error);
}

static void
test_send_declare_retry(void)
{
#if 0   // TODO
    pxd_client_t   client;

    char *  buffer;
    int     pass;

    memset(&client, 0, sizeof(client));

    buffer = (char *) malloc(sizeof(buffer));

    client.declare = pxd_alloc_packet(buffer, sizeof(buffer), null);

    pxd_mutex_init(&client.io.mutex);

    pxd_force_queue = true;

    pass = send_declare_retry(&client);
    pxd_force_queue = false;
#endif
}

static void
test_start_stop_connection(void)
{
    pxd_client_t  client;
    pxd_error_t   error;

    memset(&error,  0, sizeof(error));

    memset(&client, 0, sizeof(client));
    make_locks(&client);

    client.thread_done  = false;
    client.io.stop_now  = true;

    pxd_start_connection(&client, &error);

    if (error.error != VPL_ERR_TIMEOUT) {
        error("pxd_start_connection didn't time out.\n");
        exit(1);
    }

    error.error = 0;
    pxd_create_event(&client.io.event);

    pxd_stop_connection(&client, &error, true);

    if (error.error != VPL_ERR_TIMEOUT) {
        error("pxd_stop_connection didn't time out.\n");
        exit(1);
    }

    pxd_free_event(&client.io.event);
}

static void
test_sleep(void)
{
    pxd_client_t  client;

    memset(&client, 0, sizeof(client));

    client.io.socket = VPLSOCKET_INVALID;
    pxd_mutex_init(&client.io.mutex);
    pxd_create_event(&client.io.event);

    sleep_seconds(&client, 0);   // at least shouldn't dump core

    pxd_force_time      = 1;
    pxd_backwards_time  = 0;

    sleep_seconds(&client, 1);
    pxd_free_event(&client.io.event);

    if (pxd_backwards_time != 1) {
        error("Time only went forward.\n");
        exit(1);
    }
}

static void
test_send_ccd_challenge(void)
{
    pxd_client_t *  client;
    pxd_open_t      open;
    pxd_error_t     error;

    int  pass;

    fill_open(&open);

    client = pxd_open(&open, &error);

    free_open(&open);

    pxd_stop_connection(client, &error, true);

    if (error.error != 0) {
        error("pxd_stop_connection failed in the ccd challenge test.\n");
        exit(1);
    }

    client->io.stop_now = false;

    for (int i = 1; i <= 2; i++) {
        malloc_countdown = i;

        pass = send_ccd_challenge(client);

        if (pass || malloc_countdown > 0) {
            error("send_ccd_challenge didn't trigger a malloc failure at %d.\n", i);
            exit(1);
        }
    }

    client->io.stop_now = true;
    pass = send_ccd_challenge(client);

    if (pass) {
        error("send_ccd_challenge didn't trigger a queue failure.\n");
        exit(1);
    }

    client->io.stop_now = false;

    pass = send_ccd_challenge(client);

    if (!pass) {
        error("send_ccd_challenge failed.\n");
        exit(1);
    }

    client->io.stop_now = true;
    pxd_close(&client, true, &error);
}

static void
test_process_packet(void)
{
    pxd_client_t *  client;
    pxd_open_t      open;
    pxd_error_t     error;
    pxd_unpacked_t  unpacked;
    pxd_packet_t *  packet;
    pxd_cred_t *    creds;
    int64_t         sequence;

    char  challenge[7];
    int   pass;
    int   packed_length;

    fill_open(&open);

    client = pxd_open(&open, &error);
    free_open(&open);

    pxd_stop_connection(client, &error, true);

    if (error.error != 0) {
        error("test_process_packet can't stop the thread.\n");
        exit(1);
    }

    creds = client->creds;
    client->io.crypto = pxd_create_crypto(creds->key, creds->key_length, 13);

    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.type     = Send_pxd_response;
    unpacked.response = pxd_op_failed;
    unpacked.async_id = 3;

    packet = pxd_pack(&unpacked, client->io.crypto, &error);
    pxd_prep_packet(&client->io, packet, &client->io.out_sequence);

    pxd_process_unpack = 0;

    pass = process_packet(client, packet, &client->io.out_sequence);

    if (pass || pxd_process_unpack == 0) {
        error("process_packet didn't detect a bad sequence.\n");
        exit(1);
    }

    /*
     *  process_packet should complain about a missing command entry.
     */
    client->io.out_sequence--;
    pxd_not_found = 0;

    pass = process_packet(client, packet, &client->io.out_sequence);

    if (pxd_not_found == 0) {
        error("process_packet didn't notice a missing command entry.\n");
        exit(1);
    }

    /*
     *  Now fake an invalid command...
     */
    client->io.out_sequence--;
    pxd_bad_commands = 0;
    client->io.outstanding[0].valid    = true;
    client->io.outstanding[0].type     = Send_pxd_response;
    client->io.outstanding[0].async_id = unpacked.async_id;

    pass = process_packet(client, packet, &client->io.out_sequence);

    if (pass || pxd_bad_commands == 0) {
        error("process_packet didn't notice an invalid command.\n");
        exit(1);
    }

    /*
     *  Corrupt the packet so that the signature isn't valid.
     */
    pxd_bad_signatures = 0;
    client->io.out_sequence--;
    packet->base[sequence_offset + 8]++;

    pass = process_packet(client, packet, &client->io.out_sequence);

    if (pass || pxd_bad_signatures == 0) {
        error("process_packet didn't detect a bad signature.\n");
        exit(1);
    }

    pxd_free_packet(&packet);

    /*
     *  Force a malloc failure during crypto setup for a login.
     */

    pxd_free_crypto(&client->io.crypto);

    client->connect_login  = true;
    client->io.crypto      = pxd_create_crypto(test_key, sizeof(test_key), 32);
    pxd_login_crypto       = 0;
    client->connect_creds  = creds;

    memset(challenge, 0, sizeof(challenge));

    unpacked.type              = Send_ccd_challenge;
    unpacked.connection_id     = 101;
    unpacked.challenge         = challenge;
    unpacked.challenge_length  = sizeof(challenge);
    unpacked.pxd_dns           = pxd_dns;
    unpacked.connection_time   = 32;
    unpacked.address           = test_ip_address;
    unpacked.address_length    = sizeof(test_ip_address);
    unpacked.port              = 42;

    packet = pxd_pack(&unpacked, client->io.crypto, &error);
    pxd_prep_packet(&client->io, packet, &client->io.out_sequence);

    malloc_countdown = 1;

    pass = !process_packet(client, packet, &sequence);

    if (!pass || pxd_login_crypto != 1 || sequence != 0) {
        error("process_packet didn't trigger a crypto malloc failure.\n");
        exit(1);
    }
    
    pxd_free_packet(&packet);

    client->connect_creds = null;

    /*
     *  Now fail in the process_login call.
     */
    unpacked.type         = Send_ccd_login;
    unpacked.blob         = make_blob(&packed_length, proxy_key, sizeof(proxy_key));
    unpacked.blob_length  = packed_length;

    packet = pxd_pack(&unpacked, client->io.crypto, &error);
    pxd_prep_packet(&client->io, packet, &client->io.out_sequence);

    pxd_bad_blobs    = 0;
    sequence         = client->io.out_sequence - 1;

    pass = process_packet(client, packet, &sequence);

    if (!pass || pxd_bad_blobs != 1) {
        error("process_packet didn't trigger a process_login failure.\n");
        exit(1);
    }
    
    pxd_free_packet(&packet);
    free(unpacked.blob);

    pxd_close(&client, true, &error);
}

static void
test_live(int reject_creds)
{
    pxd_error_t     error;
    pxd_open_t      open;
    pxd_client_t *  client;
    send_t          send;
    receive_t       receive;

    int  client_fd;
    int  done;
    int  async_id;
    int  out;
    int  in;

    volatile int  tries;

    log("Starting the test_live socket open test.\n");

    sleep(2);   //  Wait for other clients to exit or try to connect
    flush_accept_queue();

    pxd_min_delay        =  1;
    pxd_max_delay        = 10;
    pxd_declare_failures =  0;
    pxd_force_socket     =  1;
    pxd_force_change     = true;

    /*
     *  Create a client.
     */
    fill_open(&open);

    client = pxd_open(&open, &error);

    free_open(&open);

    if (error.error != 0) {
        error("test_live:  pxd_open failed:  %s\n", error.message);
        exit(1);
    }

    /*
     *  Wait for the socket errors to happen.
     */
    tries = 4;

    while (pxd_force_socket > 0 && tries-- > 0) {
        sleep(1);
    }

    if (pxd_force_socket > 0) {
        error("test_live:  pxd_force_socket won't trigger in open.\n");
        exit(1);
    }

    sleep(1);   // Wait a bit for the reconnect try.

    /*
     * Now accept the connection from the client.
     */
    client_fd = start_connection();
    out       = 0;
    in        = 0;

    /*
     *  Send the challenge.
     */
    memset(&send, 0, sizeof(send));

    send.fd       = client_fd;
    send.type     = Send_pxd_challenge;
    send.sequence = out++;
    send.crypto   = shared_crypto;

    send_packet(&send);

    /*
     *  Receive the login packet and send the response.
     */
    memset(&receive, 0, sizeof(receive));

    receive.fd        = client_fd;
    receive.crypto    = shared_crypto;
    receive.type      = Send_pxd_login;
    receive.sequence  = in++;
    receive.tag       = shared_time;
    receive.client    = client;

    async_id = receive_packet(&receive);

    if (reject_creds) {
        pxd_rejections = 0;
        open_state     = cb_started;
        send.type      = Reject_pxd_credentials;
        send.sequence  = out++;
        send.async_id  = async_id;
        send.response  = pxd_op_successful;

        send_packet(&send);

        tries = 2;

        while (pxd_rejections == 0 && tries-- > 0) {
            sleep(1);
        }

        if (pxd_rejections == 0) {
            error("The client didn't report a rejection.\n");
            exit(1);
        }

        if (open_state != cb_invoked) {
            error("The callback state is incorrect.\n");
            exit(1);
        }

        open_state = cb_done;

        pxd_close(&client, true, &error);
        pxd_force_socket = -1;  /* make sure feature this is disabled */
        return;
    }

    send.type     = Send_pxd_response;
    send.sequence = out++;
    send.async_id = async_id;
    send.response = pxd_op_successful;

    send_packet(&send);

    /*
     *  Try sending a configuration packet while we're at it.
     */
    send.type     = Set_pxd_configuration;
    send.sequence = out++;

    send_packet(&send);

    /*
     *  Try declaring a configuration.
     */
    declare_live(client);

    receive.type      = Declare_server;
    receive.sequence  = in++;

    async_id = receive_packet(&receive);

    send.type     = Send_pxd_response;
    send.sequence = out++;
    send.async_id = async_id;

    send_packet(&send);

    /*
     *  Now try a lookup...
     */
    done = 0;
    query_live(client, &done);

    receive.type      = Query_server_declaration;
    receive.sequence  = in++;

    async_id = receive_packet(&receive);

    send.type     = Send_server_declaration;
    send.sequence = out++;
    send.async_id = async_id;

    send_packet(&send);

    /*
     *  The declaration should complete and invoked the callback.
     */
    tries = 4;

    while (!done && tries-- > 0) {
        sleep(1);
    }

    if (!done || lookup_state != cb_invoked) {
        error("test_live:  query_live failed\n");
        exit(1);
    }

    lookup_state = cb_done;

    if (pxd_declare_failures != 0) {
        error("test_live:  The server declaration failed.\n");
        exit(1);
    }

    sleep(1);   // Wait a bit for the reconnect try.

    /*
     *  Try failing a server declaration.
     */
    declare_live(client);

    receive.type      = Declare_server;
    receive.sequence  = in++;

    async_id = receive_packet(&receive);

    send.type     = Send_pxd_response;
    send.sequence = out++;
    send.async_id = async_id;
    send.response = pxd_op_failed;

    send_packet(&send);

    tries = 4;

    while (pxd_declare_failures == 0 && tries-- > 0) {
        sleep(1);
    }

    if (pxd_declare_failures == 0) {
        error("test_live:  The declaration didn't fail\n");
        exit(1);
    }

    /*
     *  Break the connection.
     */
    log("Starting the test_live connection break test.\n");
    flush_accept_queue();
    pxd_force_socket = 2;
    close(client_fd);

    /*
     *  Wait for the socket errors to happen.
     */
    tries = 4;

    while (pxd_force_socket > 0 && tries-- > 0) {
        sleep(1);
    }

    if (pxd_force_socket > 0) {
        error("test_live:  pxd_force_socket won't trigger in reconnect.\n");
        exit(1);
    }

    /*
     *  Okay, the client should try to reconnect.
     */
    log("test_live:  Waiting for a reconnect.\n");
    client_fd = start_connection();
    out       = 0;
    in        = 0;

    /*
     *  Send the challenge.
     */
    memset(&send, 0, sizeof(send));

    send.fd       = client_fd;
    send.type     = Send_pxd_challenge;
    send.sequence = out++;
    send.crypto   = shared_crypto;

    send_packet(&send);

    /*
     *  Receive the login packet and send the response.
     */
    receive.fd        = client_fd;
    receive.type      = Send_pxd_login;
    receive.sequence  = in++;
    receive.tag       = shared_time;

    async_id = receive_packet(&receive);

    send.type     = Send_pxd_response;
    send.sequence = out++;
    send.async_id = async_id;
    send.response = pxd_op_successful;

    send_packet(&send);

    pxd_close(&client, true, &error);

    if (error.error != 0) {
        error("test_live:  pxd_close failed:  %s\n", error.message);
        exit(1);
    }

    log("Finished test_live.\n");
    pxd_force_socket = -1;  /* make sure feature this is disabled */
}

static int
login_client(pxd_client_t *client)
{
    send_t     send;
    receive_t  receive;

    int  client_fd;
    int  in;
    int  out;
    int  async_id;

    client_fd = start_connection();
    out       = 0;
    in        = 0;

    /*
     *  Send the challenge.
     */
    memset(&send, 0, sizeof(send));

    send.fd       = client_fd;
    send.type     = Send_pxd_challenge;
    send.sequence = out++;
    send.crypto   = shared_crypto;

    send_packet(&send);

    /*
     *  Receive the login packet and send the response.
     */
    memset(&receive, 0, sizeof(receive));

    receive.fd        = client_fd;
    receive.type      = Send_pxd_login;
    receive.sequence  = in++;
    receive.tag       = shared_time;
    receive.client    = client;
    receive.crypto    = shared_crypto;

    async_id = receive_packet(&receive);

    send.type     = Send_pxd_response;
    send.sequence = out++;
    send.async_id = async_id;
    send.response = pxd_op_successful;

    send_packet(&send);
    return client_fd;
}

static void
fill_connect(pxd_connect_t *connect, volatile void *opaque)
{
    pxd_id_t *  id;

    memset(connect, 0, sizeof(*connect));

    id               = make_id(test_region, test_instance_id);
    id->user_id      = 810;
    id->device_id    = 799;
    connect->target  = pxd_copy_id(id);
    connect->opaque  = (void *) opaque;
    connect->creds   = (pxd_cred_t *) malloc(sizeof(*connect->creds));
    connect->pxd_dns = cluster_name;

    connect->creds->id             = connect->target;
    connect->creds->key            = connect_key;
    connect->creds->opaque         = connect_blob;
    connect->creds->opaque_length  = connect_length;
    connect->creds->key_length     = sizeof(connect_key);

    connect->address_count = 1;

    memcpy(&connect->addresses[0], &lookup_addresses[0], sizeof(connect->addresses[0]));
    free(id);
}

static void
free_connect(pxd_connect_t *connect)
{
    pxd_free_id(&connect->target);
    free(connect->creds);
    connect->creds = null;
}

static void
test_live_connect(void)
{
    pxd_error_t     error;
    pxd_open_t      open;
    pxd_client_t *  client;
    pxd_connect_t   connect;
    send_t          send;
    receive_t       receive;
    volatile int    result;
    volatile int    login_done;
    volatile int    rejected;
    volatile int    failed;

    int  client_fd;
    int  in;
    int  out;
    int  tries;
    int  async_id;
    int  status;

    log("Starting test_live_connect.\n");
    flush_accept_queue();

    /*
     *  Create a client and do the login.
     */
    fill_open(&open);
    open.cluster_name = cluster_name;
    open.is_incoming  = false;

    client = pxd_open(&open, &error);

    client_fd = login_client(client);
    in        = 1;
    out       = 2;

    /*
     *  Okay, try failing a connection attempt.
     */
    fill_connect(&connect, &result);
    result = -1;

    connect_state = cb_started;

    pxd_connect(client, &connect, &error);

    memset(&receive, 0, sizeof(receive));

    receive.fd        = client_fd;
    receive.type      = Start_connection_attempt;
    receive.sequence  = in++;
    receive.tag       = shared_time;
    receive.client    = client;
    receive.crypto    = shared_crypto;

    async_id = receive_packet(&receive);

    memset(&send, 0, sizeof(send));

    send.tag      = shared_time;
    send.type     = Send_pxd_response;
    send.fd       = client_fd;
    send.sequence = out++;
    send.async_id = async_id;
    send.response = pxd_timed_out;
    send.crypto   = shared_crypto;

    send_packet(&send);

    tries = 3;

    while (result < 0 && tries-- > 0) {
        sleep(1);
    }

    if (result < 0) {
        error("The pxd_connect callback didn't happen.\n");
        exit(1);
    }

    if (connect_state != cb_invoked) {
        error("The pxd_connect callback didn't advance the state.\n");
        exit(1);
    }

    connect_state = cb_done;

    if (result != pxd_timed_out && result != pxd_op_failed) {
        error("The pxd_connect callback got the wrong result.\n");
        exit(1);
    }

    sleep(1);

    /*
     *  Okay, try succeeding.
     */
    expect_address = true;
    connect.opaque = (void *) &login_done;

    connect_state = cb_started;

    pxd_connect(client, &connect, &error);

    client_fd  = login_client(client);
    result     = -1;
    login_done = -1;
    in         =  1;
    out        =  2;

    receive.type      = Start_connection_attempt;
    receive.sequence  = in++;
    receive.fd        = client_fd;
    receive.tag       = shared_time;

    async_id = receive_packet(&receive);

    /*
     *  Send the pxd response.
     */
    send.fd           = client_fd;
    send.type         = Send_pxd_response;
    send.response     = pxd_op_successful;
    send.sequence     = out++;
    send.tag       = shared_time;

    send_packet(&send);

    in  = 0; /* Start the new sequence ids */
    out = 0;

    send.type      = Send_ccd_challenge;
    send.async_id  = 0;
    send.sequence  = out++;

    send_packet(&send);

    receive.type      = Send_ccd_login;
    receive.sequence  = in++;
    receive.crypto    = connect_crypto;
    receive.client    = client;
    receive.tag       = shared_time;

    async_id = receive_packet(&receive);

    send.type      = Send_ccd_response;
    send.async_id  = async_id;
    send.sequence  = out++;
    send.response  = pxd_op_successful;
    send.address   = &lookup_addresses[0];
    send.crypto    = connect_crypto;

    send_packet(&send);

    send.address   = null;

    tries = 4;

    while (login_done < 0 && tries-- > 0) {
        sleep(1);
    }

    if (login_done != pxd_op_successful) {
        error("A successful connect returned %d.\n", (int) login_done);
        exit(1);
    }

    if (connect_state != cb_invoked) {
        error("The pxd_connect callback didn't advance the state.\n");
        exit(1);
    }

    pxd_close(&client, true, &error);
    expect_address = false;

    connect_state = cb_done;
    sleep(1);
    flush_accept_queue();

    /*
     *  Now try rejecting the credentials.
     */
    client = pxd_open(&open, &error);

    client_fd  = login_client(client);
    in         =  1;
    out        =  2;
    rejected   = -1;

    connect.opaque    = (void *) &rejected;
    connect_state     = cb_started;

    pxd_connect(client, &connect, &error);

    receive.type      = Start_connection_attempt;
    receive.sequence  = in++;
    receive.fd        = client_fd;
    receive.tag       = shared_time;
    receive.crypto    = shared_crypto;

    async_id = receive_packet(&receive);

    in  = 0; /* Start the new sequence ids */
    out = 0;

    send.type      = Send_ccd_challenge;
    send.async_id  = 0;
    send.sequence  = out++;
    send.fd        = client_fd;

    send_packet(&send);

    receive.type      = Send_ccd_login;
    receive.sequence  = in++;
    receive.crypto    = connect_crypto;
    receive.client    = client;
    receive.tag       = shared_time;

    async_id = receive_packet(&receive);

    send.type      = Reject_ccd_credentials;
    send.sequence  = out++;
    send.async_id  = async_id;
    send.crypto    = connect_crypto;

    send_packet(&send);

    tries = 4;

    while (rejected < 0 && tries-- > 0) {
        sleep(1);
    }

    if (rejected != Reject_ccd_credentials) {
        error("A rejected connection returned %d.\n", (int) rejected);
        exit(1);
    }

    if (connect_state != cb_invoked) {
        error("The pxd_connect callback didn't advance the state.\n");
        exit(1);
    }

    connect_state = cb_done;
    sleep(1);

    pxd_close(&client, true, &error);

    /*
     *  Now send a failure result.
     */
    client = pxd_open(&open, &error);

    client_fd  = login_client(client);
    in         =  1;
    out        =  2;
    failed     = -1;

    connect_state     = cb_started;
    connect.opaque    = (void *) &failed;

    pxd_connect(client, &connect, &error);

    receive.type      = Start_connection_attempt;
    receive.sequence  = in++;
    receive.fd        = client_fd;
    receive.tag       = shared_time;
    receive.crypto    = shared_crypto;

    async_id = receive_packet(&receive);

    in  = 0; /* Start the new sequence ids */
    out = 0;

    send.type      = Send_ccd_challenge;
    send.async_id  = 0;
    send.sequence  = out++;
    send.fd        = client_fd;

    send_packet(&send);

    receive.type      = Send_ccd_login;
    receive.sequence  = in++;
    receive.crypto    = connect_crypto;
    receive.client    = client;
    receive.tag       = shared_time;

    async_id = receive_packet(&receive);

    send.type      = Send_ccd_response;
    send.response  = pxd_op_failed;
    send.sequence  = out++;
    send.async_id  = async_id;
    send.crypto    = connect_crypto;

    send_packet(&send);

    tries = 4;

    while (failed < 0 && tries-- > 0) {
        sleep(1);
    }

    if (failed != send.response) {
        error("A failed connection returned %d.\n", (int) failed);
        exit(1);
    }

    if (connect_state != cb_invoked) {
        error("The pxd_connect callback didn't advance the state.\n");
        exit(1);
    }

    connect_state = cb_done;
    sleep(1);

    pxd_close(&client, true, &error);

    /*
     *  Try letting a timeout occur.
     */
    pxd_idle_limit      = 5;
    pxd_close_pending  = 0;

    client = pxd_open(&open, &error);

    client_fd  = login_client(client);
    in         =  1;
    out        =  2;
    failed     = -1;

    failed          = false;
    connect.opaque  = (void *) &status;
    connect_state   = cb_started;

    pxd_connect(client, &connect, &error);

    receive.type      = Start_connection_attempt;
    receive.sequence  = in++;
    receive.fd        = client_fd;
    receive.tag       = shared_time;
    receive.crypto    = shared_crypto;

    sleep(pxd_idle_limit + 2);

    if
    (
        connect_state != cb_invoked
    || status < 0
    || status == pxd_op_successful
    || pxd_close_pending != 1
    ) {
        error("The pxd_connect timeout test failed:  %d, %d\n",
            (int) status, (int) connect_state);
        exit(1);
    }

    connect_state = cb_done;
    sleep(1);
    pxd_close(&client, true, &error);

    free_connect(&connect);
    free_open(&open);
}

static void
fill_server_open(pxd_open_t *open)
{
    pxd_cred_t *    credentials;
    pxd_id_t *      id;

    credentials = (pxd_cred_t *) malloc(sizeof(*credentials));
    id = (pxd_id_t *) malloc(sizeof(*id));

    memset(open, 0, sizeof(*open));
    memset(credentials, 0, sizeof(*credentials));
    memset(id, 0, sizeof(*id));

    open->credentials      = credentials;
    open->credentials->id  = id;

    id->region       = test_region;
    id->user_id      = server_user;
    id->device_id    = server_device;
    id->instance_id  = server_instance;

    credentials->id             = id;
    credentials->opaque         = blob;
    credentials->opaque_length  = sizeof(blob);
    credentials->key            = key;
    credentials->key_length     = sizeof(key);

    open->cluster_name = cluster_name;
    open->credentials  = credentials;
    open->is_incoming  = true;
    open->callback     = &callbacks;
    open->opaque       = &callback_opaque;
}

static void
test_live_incoming(int fail)
{
    pxd_client_t *  client;
    pxd_open_t      open;
    pxd_error_t     error;
    receive_t       receive;
    send_t          send;
    pxd_unpacked_t  unpacked;
    pxd_receive_t   ans_receive;

    int     in;
    int     out;
    int     client_fd;
    int     async_id;
    int     tries;

    log("Starting test_live_incoming.\n");

    /*
     *  Create an incoming client and log it onto the "server"
     */
    fill_server_open(&open);
    open.is_incoming = true;

    client    = pxd_open(&open, &error);
    client_fd = login_client(client);

    /*
     *  Okay, send the ANS message...
     */
    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.type             = pxd_connect_request;
    unpacked.user_id          = 801;
    unpacked.device_id        = 802;
    unpacked.request_id       = 803;
    unpacked.instance_id      = open.credentials->id->instance_id;
    unpacked.pxd_dns          = cluster_name;
    unpacked.server_instance  = client->instance_id;
    unpacked.address_count    = 1;
    unpacked.addresses        = lookup_addresses;

    memset(&receive, 0, sizeof(receive));

    ans_receive.buffer            = pxd_pack_ans(&unpacked, &ans_receive.buffer_length);
    ans_receive.server_key        = blob_key;
    ans_receive.server_key_length = sizeof(blob_key);
    ans_receive.client            = client;
    ans_receive.opaque            = &unpacked;
    receive_opaque                = &unpacked;

    receive_state = cb_started;

    pxd_receive(&ans_receive, &error);

    if (error.error != 0) {
        error("The client rejected the message:  %s.\n", error.message);
        exit(1);
    }

    free(ans_receive.buffer);
    free_open(&open);

    /*
     *  The ANS message has arrived.  The proxy client
     *  should connect.
     */
    client_fd = login_client(null);
    in        =  1;
    out       =  2;

    memset(&send,    0, sizeof(send   ));
    memset(&receive, 0, sizeof(receive));

    send.fd          = client_fd;
    send.sequence    = out;
    send.crypto      = shared_crypto;
    send.tag         = shared_time;
    receive.fd       = client_fd;
    receive.sequence = in;
    receive.crypto   = shared_crypto;
    receive.tag      = shared_time;
    receive.type     = Start_proxy_connection;

    /*
     *  Okay, get the start request and send a response.
     */
    async_id = receive_packet(&receive);

    free(receive.challenge);

    receive.challenge    = null;
    pxd_failed_proxy     = 0;
    pxd_proxy_shutdowns  = 0;

    send.type      = Send_pxd_response;
    send.response  = fail ? pxd_op_failed : pxd_op_successful;
    send.async_id  = async_id;

    send_packet(&send);

    if (fail) {
        tries = 4;

        while ((pxd_failed_proxy == 0 || pxd_proxy_shutdowns == 0) && tries-- > 0) {
            sleep(1);
        }

        if (pxd_failed_proxy == 0 || pxd_proxy_shutdowns == 0) {
            error("The proxy connection attempt didn't fail properly.\n");
            exit(1);
        }

        pxd_close(&client, true, &error);
        return;
    }

    /*
     *  Now comes the CCD login process.
     */
    send.crypto      = proxy_crypto;
    send.sequence    = 0;
    receive.crypto   = proxy_crypto;
    receive.sequence = 0;
    receive.tag      = 0;

    /*
     *  Receive the challenge.
     */
    receive.type = Send_ccd_challenge;
    async_id     = receive_packet(&receive);

    free(receive.challenge);
    receive.challenge = null;

    /*
     *  Now we should send a CCD login packet.
     */
    incoming_login_count = 0;

    send.type     = Send_ccd_login;
    send.async_id = async_id;
    send.tag      = receive.tag;

    send_packet(&send);

    /*
     *  Receive a response.
     */
    receive.type  = Send_ccd_response;

    async_id = receive_packet(&receive);

    if (async_id != send.async_id) {
        error("The client send the wrong async id:  %d.\n", (int) async_id);
        exit(1);
    }

    tries = 10;

    while (incoming_login_count == 0 && tries-- > 0) {
        sleep(1);
    }

    if (incoming_login_count == 0) {
        error("The login callback didn't occur.\n");
        exit(1);
    }

    if (receive_state != cb_invoked) {
        error("The callback didn't occur.\n");
        exit(1);
    }

    receive_state   = cb_idle;
    receive_opaque  = null;

    close(client_fd);
    pxd_close(&client, true, &error);
    sleep(1);
}

static void
test_live_reject(void)
{
    pxd_client_t *  client;
    pxd_open_t      open;
    pxd_error_t     error;
    receive_t       receive;
    send_t          send;
    pxd_unpacked_t  unpacked;
    pxd_receive_t   ans_receive;

    int     in;
    int     out;
    int     client_fd;
    int     async_id;

    log("Starting test_live_reject.\n");

    /*
     *  Create an incoming client and log it onto the "server"
     */
    fill_server_open(&open);
    open.is_incoming = true;

    client    = pxd_open(&open, &error);
    client_fd = login_client(client);

    /*
     *  Okay, send the ANS message...
     */
    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.type             = pxd_connect_request;
    unpacked.user_id          = 801;
    unpacked.device_id        = 802;
    unpacked.request_id       = 803;
    unpacked.instance_id      = open.credentials->id->instance_id;
    unpacked.pxd_dns          = cluster_name;
    unpacked.server_instance  = client->instance_id;
    unpacked.address_count    = 1;
    unpacked.addresses        = lookup_addresses;

    memset(&receive, 0, sizeof(receive));

    ans_receive.buffer            = pxd_pack_ans(&unpacked, &ans_receive.buffer_length);
    ans_receive.server_key        = blob_key;
    ans_receive.server_key_length = sizeof(blob_key);
    ans_receive.client            = client;
    ans_receive.opaque            = &unpacked;
    receive_opaque                = &unpacked;

    receive_state = cb_started;

    pxd_receive(&ans_receive, &error);

    if (error.error != 0) {
        error("The client rejected the message:  %s.\n", error.message);
        exit(1);
    }

    free(ans_receive.buffer);
    free_open(&open);

    /*
     *  The ANS message has arrived.  The proxy client
     *  should connect.
     */
    client_fd = login_client(null);
    in        =  1;
    out       =  2;

    memset(&send,    0, sizeof(send   ));
    memset(&receive, 0, sizeof(receive));

    send.fd          = client_fd;
    send.sequence    = out;
    send.crypto      = shared_crypto;
    send.tag         = shared_time;
    receive.fd       = client_fd;
    receive.sequence = in;
    receive.crypto   = shared_crypto;
    receive.tag      = shared_time;
    receive.type     = Start_proxy_connection;

    /*
     *  Okay, get the start request and send a response.
     */
    async_id = receive_packet(&receive);

    free(receive.challenge);

    receive.challenge    = null;
    pxd_failed_proxy     = 0;
    pxd_proxy_shutdowns  = 0;

    send.type      = Send_pxd_response;
    send.response  = pxd_op_successful;
    send.async_id  = async_id;

    send_packet(&send);

    /*
     *  Now comes the CCD login process.
     */
    send.crypto      = proxy_crypto;
    send.sequence    = 0;
    receive.crypto   = proxy_crypto;
    receive.sequence = 0;
    receive.tag      = 0;

    /*
     *  Receive the challenge.
     */
    receive.type = Send_ccd_challenge;
    async_id     = receive_packet(&receive);

    free(receive.challenge);
    receive.challenge = null;

    /*
     *  Now we should send a CCD login packet.
     */
    incoming_login_count = 0;

    send.type     = Send_ccd_login;
    send.async_id = async_id;
    send.bad_blob = true;
    send.tag      = receive.tag;

    printf("async id is %d\n", (int) async_id);
    send_packet(&send);

    /*
     *  Receive a response.
     */
    receive.type  = Reject_ccd_credentials;
    async_id      = receive_packet(&receive);

    if (async_id != send.async_id) {
        error("The client send the wrong async id:  expected %d, got %d.\n",
            (int) async_id,
            (int) send.async_id);
        exit(1);
    }

    close(client_fd);
    pxd_close(&client, true, &error);
    sleep(1);
}

static void
test_process_login(void)
{
    pxd_client_t    client;
    pxd_unpacked_t  unpacked;
    pxd_blob_t      input;
    pxd_cb_data_t   data;
    pxd_blob_t *    blob;
    pxd_error_t     error;

    char *  blob_buffer;
    int     blob_buffer_length;
    int     pass;

    memset(&client,   0, sizeof(client  ));
    memset(&unpacked, 0, sizeof(unpacked));
    memset(&input,    0, sizeof(input   ));
    memset(&data,     0, sizeof(data    ));
    memset(&error,    0, sizeof(error   ));

    /*
     *  First make a blob.
     */
    memset(&input, 0, sizeof(input));

    input.client_user         = client_user;
    input.client_device       = client_device;
    input.client_instance     = client_instance;
    input.server_user         = server_user;
    input.server_device       = server_device;
    input.server_instance     = server_instance;
    input.create              = VPLTime_GetTime();
    input.key                 = proxy_key;
    input.key_length          = sizeof(proxy_key);
    input.handle              = handle;
    input.handle_length       = sizeof(handle);
    input.service_id          = service_id;
    input.service_id_length   = sizeof(service_id);
    input.ticket              = ticket;
    input.ticket_length       = sizeof(ticket);

    blob_buffer = pxd_pack_blob(&blob_buffer_length, &input, blob_crypto, &error);

    if (blob_buffer == null) {
        error("The blob pack operation failed.\n");
        exit(1);
    }

    /*
     *  Make sure the blob is okay.
     */
    blob = pxd_unpack_blob(blob_buffer, blob_buffer_length, blob_crypto, &error);

    if (blob == null) {
        error("The blob unpack operation failed.\n");
        exit(1);
    }

    pxd_free_blob(&blob);

    /*
     *  Now try testing for malloc issues.
     */
    client.blob_crypto    = blob_crypto;
    client.user_id        = server_user;
    client.device_id      = server_device;
    client.instance_id    = server_instance;

    unpacked.blob         = blob_buffer;
    unpacked.blob_length  = blob_buffer_length;


    for (int i = 1; i <= 12; i++) {
        malloc_countdown = i;

        pass = process_login(&client, &unpacked, &data);

        if (pass || malloc_countdown != 0) {
            error("process_login didn't trigger a malloc error (%d).\n", i);
            exit(1);
        }
    }

    /*
     *  Now try a non-matching blob...
     */
    free(blob_buffer);
    input.server_user++;
    blob_buffer = pxd_pack_blob(&blob_buffer_length, &input, blob_crypto, &error);

    unpacked.blob        = blob_buffer;
    unpacked.blob_length = blob_buffer_length;

    pass = process_login(&client, &unpacked, &data);

    if (pass) {
        error("process_login didn't detect a non-matching user id.\n");
        exit(1);
    }

    free(blob_buffer);
}

static void
test_string(void)
{
    if (!streq(pxd_response(pxd_timed_out), pxd_string(pxd_timed_out))) {
        error("pxd_string failed its minimal sanity check.\n");
        exit(1);
    }
}

static int  outgoing_fd;
static int  incoming_fd;

static void
outgoing_done(pxd_cb_data_t *data)
{
    if (data->result != pxd_op_successful) {
        error("The outgoing connection thread failed.\n");
        exit(1);
    }

    if (data->socket.fd != outgoing_fd) {
        error("The outgoing fd is wrong.\n");
        exit(1);
    }

    *(int *) data->op_opaque = true;
}

static void
incoming_done(pxd_cb_data_t *data)
{
    if (!expect_incoming_fail) {
        if (data->result != pxd_op_successful) {
            error("The incoming connection op failed.\n");
            exit(1);
        }

        if (data->socket.fd != incoming_fd) {
            error("The incoming fd is wrong.\n");
            exit(1);
        }
        
        if (incoming_id != null) {
            if (data->blob->client_user != incoming_id->user_id) {
                error("The incoming user id is wrong.\n");
                exit(1);
            }
            
            if (data->blob->client_device != incoming_id->device_id) {
                error("The incoming device id is wrong.\n");
                exit(1);
            }
            
            if (!streq(data->blob->client_instance, incoming_id->instance_id)) {
                error("The incoming instance id is wrong.\n");
                exit(1);
            }
        }
    } else if (data->result == pxd_op_successful) {
        error("The incoming connection op succeeded.\n");
        exit(1);
    }

    *(int *) data->op_opaque = true;
}

static void
test_live_login(void)
{
    pxd_event_t *  event;
    pxd_error_t    error;
    pxd_login_t    login;
    pxd_id_t       client_id;
    pxd_id_t       server_id;
    pxd_cred_t     creds;

    char *  packed_blob;
    int     packed_length;
    int     incoming_flag;
    int     outgoing_flag;
    int     tries;

    log("Starting the pxd_login test.\n");

    /*
     *  Create an event structure and use its connection.
     */
    pxd_create_event(&event);

    /*
     *  Now make a blob.  Normally, IAS would provide this.
     */
    packed_blob            = make_blob(&packed_length, proxy_key, sizeof(proxy_key));

    client_id.region       = client_region;
    client_id.user_id      = client_user;
    client_id.device_id    = client_device;
    client_id.instance_id  = client_instance;

    server_id.region       = server_region;
    server_id.user_id      = server_user;
    server_id.device_id    = server_device;
    server_id.instance_id  = server_instance;

    creds.id               = &client_id;
    creds.opaque           = packed_blob;
    creds.opaque_length    = packed_length;
    creds.key              = proxy_key;
    creds.key_length       = sizeof(proxy_key);

    memset(&login, 0, sizeof(login));

    login.credentials        = &creds;
    login.server_id          = &server_id;
    login.server_key         = blob_key;
    login.server_key_length  = sizeof(blob_key);

    incoming_id = &client_id;

    /*
     *  Start the incoming login process.
     */
    incoming_flag          = false;
    login.socket           = event->socket;
    incoming_fd            = login.socket.fd;
    login.is_incoming      = true;
    login.opaque           = &incoming_flag;
    login.callback         = incoming_done;

    pxd_login(&login, &error);

    if (error.error != 0) {
        log("pxd_login(incoming) failed:  %s\n", error.message);
        exit(1);
    }

    /*
     * Okay, start the outgoing login process.
     */
    outgoing_flag          = false;
    login.socket           = event->out_socket;
    outgoing_fd            = login.socket.fd;
    login.is_incoming      = false;
    login.opaque           = &outgoing_flag;
    login.callback         = outgoing_done;

    pxd_login(&login, &error);

    if (error.error != 0) {
        log("pxd_login(outgoing) failed:  %s\n", error.message);
        exit(1);
    }

    tries = 4;

    while ((!incoming_flag || !outgoing_flag) && tries-- > 0) {
        sleep(1);
    }

    if (!incoming_flag || !outgoing_flag) {
        error("The connection attempt did not complete.\n");
        exit(1);
    }

    log("The pxd_login test passed.\n");

    pxd_free_event(&event);
    free(packed_blob);
    incoming_id = null;
}

static void
fail_wait(int *done, const char *where, pxd_event_t *event)
{
    int  wait_count;

    log("fail_wait for %s\n", where);
    wait_count = 4;

    while (wait_count-- > 0 && !*done) {
        sleep(1);
    }

    if (!*done) {
        error("pxd_login didn't complete at %s\n", where);
        exit(1);
    }

    if (malloc_countdown > 0) {
        error("pxd_login didn't trigger a malloc failure at %s\n", where);
        exit(1);
    }

    pxd_free_event(&event);
}

/*
 *  Test the error paths in pxd_login, the incoming login path.
 */
static void
test_incoming_login_path(int where)
{
    pxd_event_t *  event;
    pxd_error_t    error;
    pxd_login_t    login;
    pxd_id_t       client_id;
    pxd_id_t       server_id;
    pxd_cred_t     creds;
    send_t         send;
    receive_t      receive;

    char *  packed_blob;
    int     packed_length;
    int     incoming_flag;
    int     async_id;
    int     tries;
    int     result;
    int     fix_proxy_blob;

    const char *  message;

    log("Starting the outgoing pxd_login test at %d.\n", where);

    fix_proxy_blob    = false;
    pxd_reject_limit  = 1;
    message           = "test error";

    /*
     *  Create an event structure and use its connection.
     */
    pxd_create_event(&event);

    /*
     *  Now make a blob.
     */
    packed_blob            = make_blob(&packed_length, proxy_key, sizeof(proxy_key));

    client_id.region       = client_region;
    client_id.user_id      = client_user;
    client_id.device_id    = client_device;
    client_id.instance_id  = client_instance;

    server_id.region       = server_region;
    server_id.user_id      = server_user;
    server_id.device_id    = server_device;
    server_id.instance_id  = server_instance;

    creds.id               = &client_id;
    creds.opaque           = packed_blob;
    creds.opaque_length    = packed_length;
    creds.key              = proxy_key;
    creds.key_length       = sizeof(proxy_key);

    memset(&login,    0, sizeof(login  ));
    memset(&send,     0, sizeof(send   ));
    memset(&receive,  0, sizeof(receive));

    login.credentials        = &creds;
    login.server_id          = &server_id;
    login.server_key         = blob_key;
    login.server_key_length  = sizeof(blob_key);

    /*
     *  Start the incoming login process.
     */
    incoming_flag          = false;
    login.socket           = event->socket;
    incoming_fd            = event->socket.fd;
    login.is_incoming      = true;
    login.opaque           = &incoming_flag;
    login.callback         = incoming_done;

    result = fcntl(incoming_fd, F_SETFL, O_NONBLOCK);

    if (result == -1) {
        perror("test_incoming_login_path fcntl");
        exit(1);
    }

    pxd_login(&login, &error);

    free(packed_blob);

    if (error.error != 0) {
        log("pxd_login(incoming) failed:  %s\n", error.message);
        exit(1);
    }

    /*
     *  Now comes the CCD login process.
     */
    send.crypto      = proxy_crypto;
    send.sequence    = 0;
    send.fd          = event->out_socket.fd;

    receive.fd       = event->out_socket.fd;
    receive.crypto   = proxy_crypto;
    receive.sequence = 0;
    receive.tag      = 0;

    /*
     *  Receive the challenge.
     */
    receive.type = Send_ccd_challenge;
    async_id     = receive_packet(&receive);

    if (where-- <= 0) {
        free(receive.challenge);
        close(receive.fd);
        fail_wait(&incoming_flag, "incoming socket close", event);
        return;
    }

    /*
     *  Now we should send a CCD login packet.
     */
    incoming_login_count = 0;

    if (where-- == 0) {
        free(receive.challenge);
        sleep(pxd_sync_io_timeout + 2);
        fail_wait(&incoming_flag, "read timeout", event);
        return;
    }

    if (where-- == 0) {
        message = "invalid challenge";
        receive.challenge[0]++;
    }

    if (where-- == 0) {
        message = "invalid connection id";
        receive.connection_id++;
    }

    if (where-- == 0) {
        message = "malloc failure 1";
        malloc_countdown = 3;
    }

    if (where-- == 0) {
        message = "malloc failure 2";
        malloc_countdown = 4;
    }

    if (where-- == 0) {
        message = "malloc failure 3";
        malloc_countdown = 6;
    }

    if (where-- == 0) {
        message = "malloc failure 4";
        malloc_countdown = 5;
    }

    if (where-- == 0) {
        message = "invalid blob";
        proxy_blob[0]++;
        fix_proxy_blob = true;
    }

    if (where-- == 0) {
        message = "incorrect packet type";
        send.type          = Reject_ccd_credentials;
    } else {
        send.type          = Send_ccd_login;
    }

    send.async_id          = async_id;
    send.tag               = receive.tag;
    send.challenge         = receive.challenge;
    send.connection_id     = receive.connection_id;
    send.challenge_length  = receive.challenge_length;

    send_packet(&send);

    free(send.challenge);

    if (where <= 0) {
        if (fix_proxy_blob) {
            proxy_blob[0]--;
        }

        fail_wait(&incoming_flag, message, event);
        return;
    }

    send.challenge     = null;
    receive.challenge  = null;

    /*
     *  Receive a response.
     */
    receive.type  = Send_ccd_response;

    async_id = receive_packet(&receive);

    if (async_id != send.async_id) {
        error("The client send the wrong async id:  %d.\n", (int) async_id);
        exit(1);
    }

    tries = 10;

    while (!incoming_flag && tries-- > 0) {
        sleep(1);
    }

    log("The incoming pxd_login test passed.\n");

    pxd_free_event(&event);
}

static void
test_incoming_login(void)
{
    expect_incoming_fail = true;

    for (int i = 0; i <= 8; i++) {
        test_incoming_login_path(i);
    }

    expect_incoming_fail = false;
}

static void
test_check_ccd_login(void)
{
    pxd_unpacked_t  unpacked;
    pxd_write_t     write_info;
    pxd_login_t     info;
    pxd_id_t        id;
    int             result;
    pxd_blob_t      blob_input;
    pxd_error_t     error;

    pxd_wrong_id = 0;

    memset(&id,         0, sizeof(id        ));
    memset(&info,       0, sizeof(info      ));
    memset(&blob_input, 0, sizeof(blob_input));
    memset(&unpacked,   0, sizeof(unpacked  ));

    memset(write_info.challenge, 2, sizeof(write_info.challenge));

    blob_input.client_user        = 42;
    blob_input.client_device      = 33;
    blob_input.client_instance    = test_instance_id;
    blob_input.server_user        = 42;
    blob_input.server_device      = 33;
    blob_input.server_instance    = test_instance_id;
    blob_input.key                = test_key;
    blob_input.key_length         = sizeof(test_key);
    blob_input.handle             = handle;
    blob_input.handle_length      = sizeof(handle);
    blob_input.service_id         = handle;
    blob_input.service_id_length  = sizeof(handle);
    blob_input.ticket             = ticket;
    blob_input.ticket_length      = sizeof(ticket);

    write_info.connection_id      = 42;
    info.server_id                = &id;
    info.server_key               = connect_key;
    info.server_key_length        = sizeof(connect_key);
    id.user_id                    = blob_input.server_user;
    id.device_id                  = blob_input.server_device;
    id.instance_id                = mismatch_instance;

    unpacked.connection_id        = write_info.connection_id;
    unpacked.challenge            = (char *) write_info.challenge;
    unpacked.challenge_length     = sizeof(write_info.challenge);
    unpacked.blob                 = pxd_pack_blob(&unpacked.blob_length, &blob_input, connect_crypto, &error);

    result = check_ccd_login(&info, &unpacked, &write_info);

    if (result == pxd_op_successful || pxd_wrong_id == 0) {
        error("check_ccd_login failed to detect an id mismatch.\n");
        exit(1);
    }

    free(unpacked.blob);
}

static void
test_outgoing_login_path(int where)
{
    pxd_event_t *  event;
    pxd_error_t    error;
    pxd_login_t    login;
    pxd_id_t       client_id;
    pxd_id_t       server_id;
    pxd_cred_t     creds;
    send_t         send;
    receive_t      receive;
    uint16_t       buffer;

    char *  packed_blob;
    int     packed_length;
    int     login_flag;
    int     async_id;
    int     out;
    int     in;
    int     tries;
    int     result;
    int     fail_send;
    int     send_bad_type;

    const char * message;

    log("Starting the outgoing pxd_login test at %d.\n", where);

    message = "testing error";

    /*
     *  Create an event structure and use its connection.
     */
    pxd_create_event(&event);

    /*
     *  Now make a blob.
     */
    packed_blob            = make_blob(&packed_length, proxy_key, sizeof(proxy_key));

    client_id.region       = client_region;
    client_id.user_id      = client_user;
    client_id.device_id    = client_device;
    client_id.instance_id  = client_instance;

    server_id.region       = server_region;
    server_id.user_id      = server_user;
    server_id.device_id    = server_device;
    server_id.instance_id  = server_instance;

    creds.id               = &client_id;
    creds.opaque           = packed_blob;
    creds.opaque_length    = packed_length;
    creds.key              = proxy_key;
    creds.key_length       = sizeof(proxy_key);

    memset(&login,    0, sizeof(login  ));
    memset(&send,     0, sizeof(send   ));
    memset(&receive,  0, sizeof(receive));

    login.credentials        = &creds;
    login.server_id          = &server_id;
    login.server_key         = blob_key;
    login.server_key_length  = sizeof(blob_key);

    /*
     *  Start the incoming login process.
     */
    login_flag             = false;
    login.socket           = event->socket;
    incoming_fd            = login.socket.fd;
    login.is_incoming      = false;
    login.opaque           = &login_flag;
    login.callback         = incoming_done;

    result = fcntl(incoming_fd, F_SETFL, O_NONBLOCK);

    if (result == -1) {
        perror("test_incoming_login_path fcntl");
        exit(1);
    }

    pxd_login(&login, &error);

    if (error.error != 0) {
        log("pxd_login(incoming) failed:  %s\n", error.message);
        exit(1);
    }

    free(packed_blob);

    in  = 0; /* Start the new sequence ids */
    out = 0;

    if (where-- <= 0) {
        sleep(pxd_sync_io_timeout + 2);
        fail_wait(&login_flag, "no Send_ccd_challenge", event);
        return;
    }

    send_bad_type = where-- == 0;

    if (send_bad_type) {
        message = "send bad type";
        send.type  = Send_pxd_response;
    } else {
        send.type  = Send_ccd_challenge;
    }

    send.async_id  = 0;
    send.sequence  = out++;
    send.fd        = event->out_socket.fd;

    if (where-- == 0) {
        buffer = ntohs(~32767);
        write_buffer(send.fd, (char *) &buffer, sizeof(buffer));
        fail_wait(&login_flag, "invalid packet size", event);
        return;
    } 
    
    if (where-- == 0) {
        buffer = ntohs(~128);
        write_buffer(send.fd, (char *) &buffer, sizeof(buffer));

        buffer = ntohs(128);
        write_buffer(send.fd, (char *) &buffer, sizeof(buffer));

        sleep(pxd_sync_io_timeout + 2);
        fail_wait(&login_flag, "failure for body", event);
        return;
    }

    fail_send = where-- == 0;

    if (fail_send) {
        message = "Send_ccd_login malloc";
        malloc_countdown = 8;
    }

    send_packet(&send);

    if (fail_send || send_bad_type) {
        fail_wait(&login_flag, message, event);
        return;
    }

    wait_lots();

    receive.fd        = event->out_socket.fd;
    receive.type      = Send_ccd_login;
    receive.sequence  = in++;
    receive.crypto    = connect_crypto;
    receive.client    = null;
    receive.tag       = shared_time;

    async_id = receive_packet(&receive);

    wait_lots();

    if (where-- == 0) {
        sleep(pxd_sync_io_timeout + 2);
        fail_wait(&login_flag, "no Send_ccd_response", event);
        return;
    }

    if (where-- == 0) {
        message = "send Reject_ccd_credentials";
        send.type  = Reject_ccd_credentials;
    } else if (where-- == 0) {
        message = "send invalid packet type";
        send.type  = Send_ccd_login;
    } else {
        send.type  = Send_ccd_response;
    }

    send.async_id  = async_id;
    send.sequence  = out++;
    send.response  = pxd_op_successful;
    send.crypto    = connect_crypto;

    send_packet(&send);

    if (where <= 0) {
        fail_wait(&login_flag, message, event);
        return;
    }

    wait_lots();

    tries = 4;

    while (!login_flag && tries-- > 0) {
        sleep(1);
    }

    log("The outgoing pxd_login test passed.\n");

    pxd_free_event(&event);
}

static void
test_outgoing_login(void)
{
    expect_incoming_fail = true;

    for (int i = 0; i <= 7; i++) {
        test_outgoing_login_path(i);
        log("Finished the outgoing login path test at %d\n", (int) i);
    }

    expect_incoming_fail = false;
}

int
main(int argc, char **argv, char **envp)
{
    struct sigaction  action;

    printf("Starting the pxd client test.\n");

    /*
     *  Create a server socket for accepting connections from PXD clients.
     */
    server_port   = start_server();
    pxd_tcp_port  = server_port;

    memset(&action, 0, sizeof(action));

    action.sa_handler = SIG_IGN;

    sigaction(SIGPIPE, &action, null);

    /*
     *  Set up some crypto structures.
     */
    shared_crypto   = pxd_create_crypto(key,         sizeof(key),         1234);
    connect_crypto  = pxd_create_crypto(connect_key, sizeof(connect_key), 1234);
    proxy_crypto    = pxd_create_crypto(proxy_key,   sizeof(proxy_key),   1234);
    blob_crypto     = pxd_create_crypto(blob_key,    sizeof(blob_key),    1234);

    connect_blob    = make_blob(&connect_length, connect_key, sizeof(connect_key));
    proxy_blob      = make_blob(&proxy_length,   proxy_key,   sizeof(proxy_key  ));
    bad_blob        = make_blob(&bad_length,     proxy_key,   sizeof(proxy_key  ));

    (*(char *) bad_blob)++;

    pxd_verbose     = true;
    pxd_write_limit = 10;

    test_verify();
    test_check_ccd_login();
    test_sync_open();
    test_run_proxy();
    test_copy();
    test_open_malloc();
    test_misc_malloc();
    test_open_close();
    test_declare();
    test_receive();
    test_lookup();
    test_connect();
    test_mt();
    test_login();
    test_mutex();
    test_send_login();
    test_send_declare_retry();
    test_start_stop_connection();
    test_sleep();
    test_send_ccd_challenge();
    test_process_packet();
    test_process_login();
    test_string();

    test_live(false);
    test_live(true);
    test_live_incoming(false);
    test_live_incoming(true);
    test_live_login();
    test_live_reject();
    test_live_connect();

    test_incoming_login();
    test_outgoing_login();

    pxd_free_crypto(&shared_crypto);
    pxd_free_crypto(&connect_crypto);
    pxd_free_crypto(&proxy_crypto);
    pxd_free_crypto(&blob_crypto);

    free(connect_blob);
    free(proxy_blob  );
    free(bad_blob    );

    if (pxd_lock_errors != 0) {
        error("The test had lock errors.\n");
        exit(1);
    }

    printf("The pxd client test passed.\n");
    return 0;
}

static void *
my_malloc(int size)
{
    void *  result;

    if (--malloc_countdown == 0) {
        log("simulating a malloc failure\n");
        result = null;
    } else {
        result = malloc(size);
    }

    return result;
}

static int
my_VPLDetachableThread_Create
(
    VPLDetachableThreadHandle_t *  a1,
    VPLDetachableThread_fn_t       a2,
    void *                         a3,
    const VPLThread_attr_t*        a4,
    const char *                   a5)
{
    int  result;

    if (--thread_countdown == 0) {
        result = VPL_ERR_INVALID;
    } else {
        result = VPLDetachableThread_Create(a1, a2, a3, a4, a5);
    }

    return result;
}

static VPLSocket_t
my_VPLSocket_Create(int a1, int a2, int a3)
{
    if (fail_create) {
        return VPLSOCKET_INVALID;
    } else {
        return VPLSocket_Create(a1, a2, a3);
    }
}

static char *
my_strdup(const char *input)
{
    char *  result;

    result = (char *) my_malloc(strlen(input) + 1);

    if (result != null) {
        memcpy(result, input, strlen(input) + 1);
    }

    return result;
}
