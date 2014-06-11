//
//  Copyright 2011-2013 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <malloc.h>
#include <vplu.h>
#include <vpl_socket.h>
#include <cslsha.h>
#include <aes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define __LOG_H__

#undef  LOG_ALWAYS
#undef  LOG_ERROR
#undef  LOG_WARN
#undef  LOG_INFO
#undef  LOG_FUNC_ENTRY
#undef  ASSERT

#define LOG_ALWAYS(x, args...)  printf(x, ## args); printf("\n");
#define LOG_ERROR(x, args...)   printf(x, ## args); printf("\n");
#define LOG_WARN(x, args...)    printf(x, ## args); printf("\n");
#define LOG_INFO(x, args...)    printf(x, ## args); printf("\n");
#define LOG_FUNC_ENTRY(a)
#define ASSERT(A)
#define log(x, args...)         ts(); printf(x, ##args)

#define streq(a, b)  (strcmp((a), (b)) == 0)

#define send_normal         0
#define send_short          1
#define send_wrong_version  2
#define send_short_params   3
#define send_short_state    4

#define SERVER_TCP_PORT         443

#define ans_test
#define ans_test_hex
#define ans_send_ping

static void *my_malloc(size_t);
static int   my_aes_SwEncrypt(u8 *, u8 *, u8 *, int, u8 *);
static int   my_aes_SwDecrypt(u8 *, u8 *, u8 *, int, u8 *);
static int   my_VPLDetachableThread_Create(VPLDetachableThreadHandle_t*,
                     VPLDetachableThread_fn_t, void*, const VPLThread_attr_t*, const char*);
static int   my_VPLSocket_Recv(VPLSocket_t socket, void* buf, int len);

#define malloc          my_malloc
#define aes_SwEncrypt   my_aes_SwEncrypt
#define aes_SwDecrypt   my_aes_SwDecrypt
#define VPLSocket_Recv  my_VPLSocket_Recv

#define VPLDetachableThread_Create  my_VPLDetachableThread_Create

#include "ans_device.cpp"

static void  make_packet(ans_client_t *, int, crypto_t *, char **, int *, int);

static void
ts(void)
{
    char buffer[500];

    printf("%s  ", print_time(VPLTime_GetTime(), buffer, sizeof(buffer)));
}

#undef malloc
#undef VPLMutex_Lock
#undef VPLSocket_Create
#undef VPLSocket_Recv
#undef VPLNet_GetAddr
#undef aes_SwEncrypt
#undef aes_SwDecrypt
#undef VPLDetachableThread_Create

#define true  1
#define false 0

static uint64_t       req_user_id        = 0x77;
static uint64_t       req_async_id       = 1000;
static uint64_t       req_query_id       = 100001;
static uint64_t       req_sequenceId     = 0;
static uint64_t       req_deviceId       = 0x88088088;
static uint32_t       req_classes        = 3;
uint64_t              tag                = 0;
static char           req_sleep_packet[] = "sleep packet";
static char           req_wakeup_key[]   = "wakeup key";
static int32_t        req_sleep_port     = 145;
static int32_t        req_interval       = 31;
static const char     req_sleep_dns[]    = "machine.domain.net";
static const char     my_host_id[]       = "test-ans";
static int            received_count     = 0;
static char           fake_ip[]          = { 1, 2, 3, 4, 5 };
static volatile int   connection_counter = 1;
static int            got_connection     = false;
static int            login_active       = false;
static unsigned char  message[]          = "1234567";
static volatile int   down_count         = 0;
static int64_t        server_generation  = 101;
static volatile int   rejected_subs      = 0;
// TODO static int            enable_reader      = 0;
static int            device_param_flag  = 0;
static volatile int   challenge_flag     = 0;
static volatile int   force_malloc_error = 0;
static volatile int   malloc_countdown   = 0;
static volatile int   fail_thread_create = 0;
static volatile int   fail_receive       = 0;
static volatile int   fail_encrypt       = false;
static volatile int   fail_decrypt       = false;
static const char *   ping_packet        = "ping";
static const char *   test_param         = "long param";
static uint64_t       blob[3];
static char           key[20];
static char           macAddress[6]      = "12345";
static char           sleepPacket[12]    = "sleep";
static char           wakeupKey[8]       = "abcde";
static char           clusterName[]      = "ans.test.acer.com";

static int32_t  my_ans_print_interval;
static int32_t  my_ans_max_subs;
static int32_t  my_ans_max_packet_size;
static int32_t  my_ans_max_query;
static int32_t  my_ans_partial_timeout;
static int32_t  my_ans_min_delay;
static int32_t  my_ans_max_delay;
static int32_t  my_ans_jitter;

static int32_t  my_ans_base_interval;
static int32_t  my_ans_retry_interval;
static int32_t  my_ans_retry_count;
static int32_t  my_ans_ping_back;
static int32_t  my_ans_ping_factor;

static int32_t  my_ans_tcp_ka_time;
static int32_t  my_ans_tcp_ka_interval;
static int32_t  my_ans_tcp_ka_probes;

static int32_t  my_ans_keep_port;

static const char *test_strings[] =
    {
        "short",
        "01234567890ab",
        "01234567890abc",
        "01234567890abcd",
        "01234567890abcde",
        "01234567890abcdef",
        "01234567890abcdef0",
        "This is a longer text to encrypt",
        "a very long message to test the encryption of more than one block"
    };

static int
t_connectionActive(ans_client_t *client, VPLNet_addr_t address)
{
    got_connection = true;
    log("test: connection active\n");
    return 1;
}

static void
t_reject_credentials(ans_client_t *client)
{
}

static void
t_connectionDown(ans_client_t *client)
{
    log("test: connection down\n");
    down_count++;
    got_connection = false;
    login_active   = false;
}

static uint64_t
ntohll(uint64_t value)
{
    uint64_t lower;
    uint64_t upper;

    if (ntohl(1) == 1) {
        return value;
    }

    upper = value >> 32;
    lower = value & (unsigned) 0xffffffff;
    return ((uint64_t) ntohl(lower) << 32) | ntohl(upper);
}

static int
t_receiveNotification(ans_client_t *client, ans_unpacked_t *unpacked)
{
    if (unpacked->type == SEND_USER_NOTIFICATION) {
        if (unpacked->deviceId != req_deviceId) {
            log("test: *** bad device id\n");
            exit(1);
        }
    }

    if (unpacked->userId != req_user_id) {
        log("test: *** bad user id\n");
        exit(1);
    }

    received_count++;
    return 1;
}

static void
t_receiveSleepInfo(ans_client_t *client, ans_unpacked_t *unpacked)
{
    if (strcmp(unpacked->sleepPacket, req_sleep_packet) != 0) {
        log("test: *** bad sleep packet\n");
        exit(1);
    }

    if (unpacked->sleepPacketLength != strlen(req_sleep_packet) + 1) {
        log("test: *** bad sleep packet length\n");
        exit(1);
    }

    if (strcmp(unpacked->wakeupKey, req_wakeup_key) != 0) {
        log("test: *** bad wakeup key\n");
        exit(1);
    }

    if (unpacked->wakeupKeyLength != strlen(req_wakeup_key) + 1) {
        log("test: *** bad wakeup key length\n");
        exit(1);
    }

    if (unpacked->sleepPort != req_sleep_port) {
        log("test: *** bad sleep port\n");
        exit(1);
    }

    if (unpacked->sleepPacketInterval != req_interval) {
        log("test: *** bad sleep packet interval\n");
        exit(1);
    }

    if (strlen(unpacked->sleepDns) != strlen(req_sleep_dns)) {
        log("test: *** bad sleep ip length\n");
        exit(1);
    }

    if (strcmp(unpacked->sleepDns, req_sleep_dns) != 0) {
        log("test: *** bad sleep ip\n");
        exit(1);
    }

    log("test: got sleep packet \"%s\", wakeup key \"%s\"\n",
        unpacked->sleepPacket, unpacked->wakeupKey);
}

static void
t_receiveDeviceState(ans_client_t *client, ans_unpacked_t *unpacked)
{
    log("test: received device state information\n");

    if (unpacked->asyncId != req_query_id) {
        log("test: *** got %ld for the async id, expected %ld\n",
            (long) unpacked->asyncId, (long) req_query_id);
        exit(1);
    }
}

static void
t_connectionClosed(ans_client_t *client)
{
    log("test: connection closed\n");
    login_active = false;
    down_count++;
}

static void
t_set_ping_packet(ans_client_t *client, char *ping_packet, int ping_length)
{
}

static void
t_login_completed(ans_client_t *client)
{
    log("test: login completed for client %p\n", client);
    login_active = true;
}

static void
t_reject_subscriptions(ans_client_t *client)
{
    rejected_subs++;
}

static void
t_receive_response(ans_client_t *client, ans_unpacked_t *response)
{
}

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

static void
print_socket_name(int fd)
{
    int     result;
    char *  ip_address;
    int     ip_port;

    struct sockaddr        local_name;
    socklen_t              local_length;
    struct sockaddr_in *   ip_name;
    struct sockaddr_in6 *  ipv6_name;
    void *                 name;

    name         = malloc(sizeof(local_name));
    local_length = sizeof(local_name);
    ip_address   = NULL;

    memset(name, 0, sizeof(local_name));

    result = getsockname(fd, (struct sockaddr *) name, &local_length);

    memcpy(&local_name, name, sizeof(local_name));

    if (result < 0) {
        log("getsockname failed\n");
        fflush(stdout);
        perror("getsockname");
    } else if (local_name.sa_family == PF_INET) {
        ip_name     = (struct sockaddr_in *) name;
        ip_address  = ip_print((char *) &ip_name->sin_addr.s_addr, 4);
        ip_port     = ntohs(ip_name->sin_port);

        log("test: using IP address %s:%d\n",
            ip_address, (int) ip_port);
    } else if (local_name.sa_family == PF_INET6) {
        ipv6_name   = (struct sockaddr_in6 *) name;
        ip_address  = ip_print((char *) &ipv6_name->sin6_addr.s6_addr, 16);
        ip_port     = ntohs(ipv6_name->sin6_port);

        log("test: using IPv6 address %s:%d\n", ip_address, (int) ip_port);
    }

    free(ip_address);
    free(name);
}

static void
test_event(void)
{
    event_t           event;
    VPLSocket_poll_t  poll;
    int               i;
    int               delay;

    log("test: === Starting the event test.\n");

    /*
     *  Make an event structure.  That should work...
     */
    memset(&event, 0, sizeof(event));
    init_event(&event);
    mutex_init(&event.mutex);
    ans_event_deaths = 0;

    make_event(&event);

    if (event.dead || ans_event_deaths != 0) {
        log("test: *** make_event failed!\n");
        exit(1);
    }

    /*
     *  Report an I/O and see whether the event is rebuilt.
     */
    memset(&poll, 0, sizeof(poll));
    poll.revents = VPLSOCKET_POLL_ERR;

    check_event(&event, 0, &poll);

    if (event.dead || ans_event_deaths != 1) {
        log("test: *** check_event(error) failed!\n");
        exit(1);
    }

    /*
     *  Simulate an I/O error in ping_thread and see whether
     *  it's handled properly.
     */
    ans_force_event  = 1;
    ans_event_deaths = 0;

    ping_thread(&event, "test event");

    if (!event.dead || ans_event_deaths != 1) {
        log("test: *** ping_thread(error) failed!\n");
        exit(1);
    }

    /*
     *  Now see whether the check_event code rebuilds the
     *  event structure properly.
     */
    poll.revents = 0;
    check_event(&event, 0, &poll);
    event.backoff = 500;

    if (event.dead || ans_event_deaths != 2) {
        log("test: *** check_event on a dead event failed!\n");
        exit(1);
    }

    free_event(&event);

    for (i = 1; i < 7; i++) {
        ans_force_make  = i;
        event.backoff   = 0;

        make_event(&event);

        if (!event.dead|| event.backoff != 2 * event_min_delay) {
            log("test: *** make_event force failed at %d!\n", i);
            exit(1);
        }

        free_event(&event);
    }

    i = 0;

    while (event.backoff < event_max_delay && i++ < 100) {
        backoff(&event);
    }

    delay = backoff(&event);

    if (delay != event_max_delay) {
        log("test: *** backoff returned %d for the delay\n", delay);
        exit(1);
    }

    if (event.backoff != event_max_delay) {
        log("test: *** backoff set %d!\n", delay);
        exit(1);
    }

    /*
     *  Fake a failed receive operation to test the error handling.
     */
    fail_receive = 1;

    make_event(&event);
    ans_event_deaths = 0;
    poll.revents = VPLSOCKET_POLL_RDNORM;

    check_event(&event, 0, &poll);

    if (ans_event_deaths != 1) {
        log("test: *** check_event ignored a receive error!\n");
        exit(1);
    }

    VPLMutex_Destroy(&event.mutex);
}

static void
test_short_packet(ans_client_t *client, short type)
{
    packet_t *  packet;
    char *      body;
    int         length;

    make_packet(client, type, client->crypto, &body, &length, send_short);
    packet = alloc_packet(body, length);

    if (unpack(client, packet, null) != null) {
        log("test: *** unpack failed to detect a short packet, type %d!\n", (int) type);
        exit(1);
    }

    free_packet(&packet);
}

static void
test_unpack_malloc(ans_client_t *client, int type, int count)
{
    packet_t *  packet;
    char *      body;
    int         length;
    int         i;

    for (i = 1; i <= count; i++) {
        make_packet(client, type, client->crypto, &body, &length, 0);

        packet              = alloc_packet(body, length);
        force_malloc_error  = i;
        malloc_countdown    = 0;

        if (unpack(client, packet, null) != null) {
            log("test: *** unpack failed to detect a malloc error, type %d!\n", (int) type);
            exit(1);
        }

        if (force_malloc_error > 0) {
            log("test: *** unpack failed to trigger a malloc error, type %d, i %d!\n",
                (int) type, (int) i);
            exit(1);
        }

        free_packet(&packet);
    }
}

static void
test_short_challenge(ans_client_t *client)
{
    packet_t *  packet;
    char *      body;
    int         length;
    int64_t     expected;

    expected = 0;
    make_packet(client, Send_challenge, client->crypto, &body, &length, 0);
    packet = alloc_packet(body, header_size + sizeof(uint64_t) + 2 * sizeof(uint16_t));

    if (unpack(client, packet, &expected) != null) {
        log("test: *** unpack failed to detect a short challenge packet!\n");
        exit(1);
    }

    free_packet(&packet);
}

static void
test_short_multicast(ans_client_t *client)
{
    packet_t *  packet;
    char *      body;
    int         length;
    int64_t     expected;
    uint16_t    sent_count;

    expected = 0;
    make_packet(client, Send_multicast, client->crypto, &body, &length, 0);
    sent_count = ntohs(-1);
    memcpy(body + header_size + sizeof(uint32_t), &sent_count, sizeof(sent_count));
    packet = alloc_packet(body, length);

    if (unpack(client, packet, &expected) != null) {
        log("test: *** unpack failed to detect a bad multicast length!\n");
        exit(1);
    }

    free_packet(&packet);
}

static void
test_short_state(ans_client_t *client)
{
    packet_t *  packet;
    char *      body;
    int         length;
    int64_t     expected;

    expected = 0;
    make_packet(client, Send_state_list, client->crypto, &body, &length, send_short_state);
    packet = alloc_packet(body, length);

    if (unpack(client, packet, &expected) != null) {
        log("test: *** unpack failed to detect a truncated state list!\n");
        exit(1);
    }

    free_packet(&packet);
}

static void
test_hex_dump(void)
{
    char  buffer[per_line * sizeof(int32_t) + sizeof(int32_t) + 3];

    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i;
    }

    hex_dump("test buffer", buffer, sizeof(buffer));
}

static void
test_long_packet(ans_client_t *client, short type)
{
    packet_t *  packet;
    char *      body;
    int         length;
    int64_t     expected;
    char *      faulty;

    make_packet(client, type, client->crypto, &body, &length, 0);
    faulty = (char *) malloc(length + 1);
    memset(faulty, 0, length + 1);
    memcpy(faulty, body, length);
    free(body);

    packet = alloc_packet(faulty, length + 1);

    expected = 0;

    if (unpack(client, packet, &expected) != null) {
        log("test: *** unpack failed to detect a long packet, type %d!\n", (int) type);
        exit(1);
    }

    free_packet(&packet);
}

static void
test_pack_unpack(void)
{
    uint16_t    length;
    uint16_t    packed_length;
    int64_t     expected;
    packet_t *  packet;
    describe_t  describe;

    uint64_t    ping_data[5]   = { 0, 1, 2, 3, 4 };
    char        buffer[4 * sizeof(uint16_t)];

    log("test: === Testing pack and unpack.\n");

    length = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));

    packed_length = ~length;
    packed_length = VPLConv_ntoh_u16(packed_length);

    memcpy(&buffer[0], &packed_length, sizeof(packed_length));

    packed_length = length;
    packed_length = VPLConv_ntoh_u16(packed_length);

    memcpy(&buffer[sizeof(uint16_t)], &packed_length, sizeof(packed_length));

    packet = alloc_packet(buffer, length);

    if (unpack(null, packet, &expected) != null) {
        log("test: *** unpack failed to detect a bad length!\n");
        exit(1);
    }

    packet->base = null; /* the buffer is one the stack */
    free_packet(&packet);

    /*
     *  pack() always sets the sequence id to zero, since the actual value
     *  is inserted when the packet is going to be sent.
     */
    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);

    packet   = pack(null, &describe);
    expected = 1;

    if (unpack(null, packet, &expected) != null) {
        log("test: *** unpack failed to detect a bad sequence id!\n");
        exit(1);
    }

    free_packet(&packet);

    clear_describe(&describe);
    describe.type = Send_ping;

    packet   = pack(null, &describe);
    expected = 0;

    if (unpack(null, packet, &expected) != null) {
        log("test: *** unpack failed to detect a null client!\n");
        exit(1);
    }

    free_packet(&packet);

    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);
    ans_fail_pack  = true;

    packet = pack(null, &describe);

    if (packet != null) {
        log("test: *** unpack ignored ans_fail_packet!\n");
        exit(1);
    }

    ans_fail_pack = false;
}

#define pack_uint32(mine)                           \
        do {                                        \
            uint32_t  local;                        \
                                                    \
            local = ntohl(mine);                    \
            memcpy(base, &local, sizeof(local));    \
            base += sizeof(local);                  \
        } while (0)

#define pack_param(mine, ans)                       \
        do {                                        \
            uint32_t  local;                        \
                                                    \
            mine  = ans + 1;                        \
            local = ntohl(mine);                    \
            memcpy(base, &local, sizeof(local));    \
            base += sizeof(local);                  \
        } while (0)

#define pack_u16(value)                             \
        do {                                        \
            uint16_t  local;                        \
                                                    \
            local = ntohs(value);                   \
            memcpy(base, &local, sizeof(local));    \
            base += sizeof(local);                  \
        } while (0)


static void
sign_packet(char *packet, int req_length)
{
    CSL_ShaContext  context;

    CSL_ResetSha (&context);
    CSL_InputSha (&context, key, sizeof(key));
    CSL_InputSha (&context, (unsigned char *) packet,  req_length - signature_size);
    CSL_ResultSha(&context, (unsigned char *) packet + req_length - signature_size);
}

#define packInt(value)                                \
            do {                                      \
                int32_t  local;                       \
                                                      \
                local = htonl(value);                 \
                memcpy(base, &local, sizeof(local));  \
                base += sizeof(local);                \
            } while (0)

#define packBytes(value, length)                      \
            do {                                      \
                uint16_t  local;                      \
                                                      \
                local = htons(length);                \
                memcpy(base, &local, sizeof(local));  \
                base += sizeof(local);                \
                memcpy(base, (value), length);        \
                base += length;                       \
            } while (0)

#define packEncrypted(value, length)                         \
            do {                                             \
                uint16_t  local;                             \
                char *    cipher_text;                       \
                int       cipher_length;                     \
                                                             \
                encrypt_field(client, &cipher_text,          \
                    &cipher_length, value, length);          \
                                                             \
                local = htons(cipher_length);                \
                memcpy(base, &local, sizeof(local));         \
                base += sizeof(local);                       \
                memcpy(base, cipher_text, cipher_length);    \
                base += cipher_length;                       \
                free(cipher_text);                           \
            } while (0)

static int
get_encrypted_length(ans_client_t *client, char *text, int length)
{
    char *  cipher_text;
    int     cipher_length;

    encrypt_field(client, &cipher_text, &cipher_length, text, length);
    free(cipher_text);
    return cipher_length;
}

static void
make_packet
(
    ans_client_t *  client,
    int             req_type,
    crypto_t *      crypto,
    char **         output,
    int *           output_length,
    int             test_flags
)
{
    char *    packet;
    uint16_t  check_length;
    uint16_t  length;
    uint16_t  sent_count;
    short     req_length;
    char *    base;
    uint16_t  type;
    uint16_t  version;
    uint64_t  deviceId;
    uint64_t  userId;
    uint64_t  async_id;
    uint64_t  sequenceId;
    uint64_t  device_state;
    char      wakeup_requested;
    uint32_t  classes;
    uint32_t  param_upper;
    uint32_t  param_lower;
    uint16_t  message_length;
    uint16_t  challenge_length;
    uint64_t  connection;
    uint64_t  param;
    char      challenge[16];
    int       device_params;
    int       i;
    int32_t   fake_param;
    int32_t   my_fake_param;
    int64_t   server_time;
    char      filling;
    int       has_tag;
    uint16_t  tag_flag;

    uint64_t  devices[] = { 10101, 20202, 30303 };

    /*
     *  Compute the size of the packet.
     */
    req_length     = header_size;
    connection     = connection_counter++;
    device_params  = 0; /* keep gcc happy */
    has_tag        = tag != 0;

    if (has_tag) {
        req_length += sizeof(uint16_t) + sizeof(uint64_t);
    }

    /*
     *  TODO remove
     */

    memset(challenge, 3, sizeof(challenge));

    switch (req_type) {
    case Send_unicast:
        req_length += 8 + 2 + sizeof(message);
        break;

    case Send_multicast:
        req_length += 4 + 2 + sizeof(message);
        break;

    case Set_device_params:
        if (test_flags == send_wrong_version) {
            req_length += 3 * sizeof(uint16_t);
        } else if (test_flags == send_short_params) {
            req_length += 3 * sizeof(uint16_t);
        } else {
            device_param_flag++;
            device_params = 25;

            req_length += 3 * sizeof(uint16_t) + device_params * sizeof(my_ans_print_interval);
            req_length += sizeof(uint16_t) + strlen(ping_packet);
            req_length += sizeof(uint16_t) + strlen(my_host_id);
            req_length += sizeof(uint16_t) + sizeof(fake_ip);

            if (device_param_flag & 1) {
                req_length += sizeof(my_ans_print_interval);
                req_length += sizeof(uint16_t) + strlen(test_param);

                device_params++;
            }
        }

        break;

    case Send_challenge:
        challenge_flag++;

        if (challenge_flag & 1) {
            req_length += 2 * sizeof(uint16_t) + 4 * sizeof(uint64_t);
            req_length += sizeof(uint16_t) + sizeof(challenge);
        } else {
            req_length += sizeof(uint16_t) + sizeof(uint64_t);
            req_length += sizeof(uint16_t) + sizeof(challenge);
        }

        break;

    case Send_state_list:
        if (test_flags == send_short_state) {
            req_length   += 2 * sizeof(uint16_t);
        } else {
            req_length   += sizeof(uint16_t);
            req_length   += array_size(devices) * (1 + 8 + 8);
            req_async_id  = req_query_id;
        }

        break;

    case Send_ping:
        break;

    case Send_sleep_setup:
        req_length += 4 + 4 + 4 + 
                      2 + get_encrypted_length(client, macAddress, sizeof(macAddress))  +
                      2 + get_encrypted_length(client, sleepPacket, sizeof(sleepPacket)) +
                      2 + get_encrypted_length(client, wakeupKey, sizeof(wakeupKey))   +
                      2 + strlen(clusterName);
        break;

    case Send_device_update:
    case Send_timed_ping:
    case Send_response:
        if (test_flags != send_short && test_flags != send_short_state) {
            log("test: *** Unimplemented test type %s (%d)\n",
                packet_type(req_type), (int) req_type);
            exit(1);
        }

        break;

    default:
        log("test: *** Unimplemented test type %s (%d)\n",
            packet_type(req_type), (int) req_type);
        exit(1);
    }

    /*
     *  If we've been asked to send a short packet, ignore the size
     *  we've computed.
     */
    if (test_flags == send_short) {
        req_length = header_size + sizeof(filling);
    }

    if (req_type != Send_challenge) {
        req_length += signature_size;
    }

    packet = (char *) malloc(req_length);

    memset(packet, 0, req_length);
    base = packet;

    if (has_tag) {
        tag_flag = server_tag_flag;
        memcpy(base, &tag_flag, sizeof(tag_flag));
        base += sizeof(tag_flag);
    }

    check_length = ntohs(~req_length);
    memcpy(base, &check_length, sizeof(check_length));
    base += sizeof(check_length);

    length = ntohs(req_length);
    memcpy(base, &length, sizeof(length));
    base += sizeof(length);

    type = ntohs(req_type);
    memcpy(base, &type, sizeof(type));
    base += sizeof(type);

    userId = ntohll(req_user_id);
    memcpy(base, &userId, sizeof(userId));
    base += sizeof(userId);

    async_id = ntohll(req_async_id);
    memcpy(base, &async_id, sizeof(async_id));
    base += sizeof(async_id);
    req_async_id++;

    sequenceId = ntohll(req_sequenceId);
    memcpy(base, &sequenceId, sizeof(sequenceId));
    base += sizeof(sequenceId);

    if (has_tag) {
        memcpy(base, &tag, sizeof(tag));
        base += sizeof(tag);
    }

    /*
     *  If we've been asked to send a short packet, just
     *  append an int32 and a signature and we're done.
     */
    if (test_flags == send_short) {
        filling = 0;

        memcpy(base, &filling, sizeof(filling));
        base += sizeof(filling);

        if (req_type != Send_challenge) {
            sign_packet(packet, req_length);
        }

        *output = packet;
        *output_length = req_length;
        return;
    }

    switch (req_type) {
    case Send_unicast:
        deviceId = ntohll(req_deviceId);
        memcpy(base, &deviceId, sizeof(deviceId));
        base += sizeof(deviceId);
        break;

    case Send_multicast:
        classes = ntohl(req_classes);
        memcpy(base, &classes, sizeof(classes));
        base += sizeof(classes);
        break;

    case Send_challenge:
        if (challenge_flag & 1) {
            version = ntohs(ans_challenge_version);
        } else {
            version = ntohs(ans_challenge_version - 1);
        }

        memcpy(base, &version, sizeof(version));
        base += sizeof(version);

        if (challenge_flag & 1) {
            sent_count = ntohs(4);
            memcpy(base, &sent_count, sizeof(sent_count));
            base += sizeof(sent_count);
        }

        connection = ntohll(connection);
        memcpy(base, &connection, sizeof(connection));
        base += sizeof(connection);

        if (challenge_flag & 1) {
            param = (uint64_t) time(null) * 1000;
            param = ntohll(param);
            memcpy(base, &param, sizeof(param));
            base += sizeof(param);

            param = server_flag_new_ping;
            param = ntohll(param);
            memcpy(base, &param, sizeof(param));
            base += sizeof(param);

            param = -1;
            param = ntohll(param);
            memcpy(base, &param, sizeof(param));
            base += sizeof(param);
        }

        challenge_length = ntohs(sizeof(challenge));
        memcpy(base, &challenge_length, sizeof(challenge_length));
        base += sizeof(challenge_length);

        memcpy(base, challenge, sizeof(challenge));
        base += sizeof(challenge);
        break;

    case Set_device_params:
        if (test_flags == send_short) {
            classes = 0;
            memcpy(base, &classes, sizeof(classes));
            base += sizeof(classes);
            break;
        } 
        
        if (test_flags == send_wrong_version) {
            version = ntohs(ans_device_version + 1);

            memcpy(base, &version, sizeof(version));
            base += sizeof(version);

            memcpy(base, &version, sizeof(version));
            base += sizeof(version);

            memcpy(base, &version, sizeof(version));
            base += sizeof(version);
            break;
        }
        
        if (test_flags == send_short_params) {
            version = ntohs(ans_device_version);

            memcpy(base, &version, sizeof(version));
            base += sizeof(version);

            sent_count = ntohs(0);

            memcpy(base, &sent_count, sizeof(sent_count));
            base += sizeof(sent_count);

            memcpy(base, &sent_count, sizeof(sent_count));
            base += sizeof(sent_count);
            break;
        }

        version = ntohs(ans_device_version);
        memcpy(base, &version, sizeof(version));
        base += sizeof(version);

        pack_u16(device_params);
        pack_param(my_ans_print_interval,  ans_print_interval);
        pack_param(my_ans_max_subs,        ans_max_subs);
        pack_param(my_ans_max_packet_size, ans_max_packet_size);
        pack_param(my_ans_max_query,       ans_max_query);
        pack_param(my_ans_partial_timeout, ans_partial_timeout);
        pack_param(my_ans_min_delay,       ans_min_delay);
        pack_param(my_ans_max_delay,       ans_max_delay);

        pack_param(my_ans_base_interval,   ans_base_interval);
        pack_param(my_ans_retry_interval,  ans_retry_interval);
        pack_param(my_ans_retry_count,     ans_retry_count);

        pack_param(my_ans_tcp_ka_time,     ans_tcp_ka_time);
        pack_param(my_ans_tcp_ka_interval, ans_tcp_ka_interval);
        pack_param(my_ans_tcp_ka_probes,   ans_tcp_ka_probes);

        param_upper = req_deviceId >> 32;
        param_lower = req_deviceId;

        pack_uint32(param_upper);       // 14
        pack_uint32(param_lower);

        pack_param(my_ans_ping_back, ans_ping_back);
        pack_param(my_ans_ping_factor, ans_ping_factor);

        server_generation++;

        param_upper = server_generation >> 32;  // 16
        param_lower = server_generation;

        pack_uint32(param_upper);       // 18
        pack_uint32(param_lower);

        server_time = VPLTime_GetTime() / 1000 + 4;

        param_upper = server_time >> 32;
        param_lower = server_time;

        pack_uint32(param_upper);       // 20
        pack_uint32(param_lower);       // 21
        pack_param(my_ans_keep_port, ans_default_keep_port);
                                        // 22
        pack_param(my_ans_jitter, ans_jitter);
        pack_uint32(444);               // 24 - external port
        pack_uint32(0);                 // 24 - enable_tcp_only

        /*
         *  If we're testing the extended parameters code, pack an extra parameter.
         */
        if (device_param_flag & 1) {
            fake_param = 64;
            pack_param(my_fake_param, fake_param);
        }

        /*
         *  Pack the string parameters.
         */
        if (device_param_flag & 1) {
            pack_u16(4);
        } else {
            pack_u16(3);
        }

        /*
         *  First is the ping packet.
         */
        length = ntohs(strlen(ping_packet));
        memcpy(base, &length, sizeof(length));
        base += sizeof(length);

        memcpy(base, &ping_packet[0], strlen(ping_packet));
        base += strlen(ping_packet);

        /*
         *  Next is the host DNS name.
         */
        length = ntohs(strlen(my_host_id));
        memcpy(base, &length, sizeof(length));
        base += sizeof(length);

        memcpy(base, &my_host_id[0], strlen(my_host_id));
        base += strlen(my_host_id);

        /*
         *  Now the external address.
         */
        length = ntohs(sizeof(fake_ip));
        memcpy(base, &length, sizeof(length));
        base += sizeof(length);

        memcpy(base, &fake_ip[0], sizeof(fake_ip));
        base += sizeof(fake_ip);

        /*
         *  If we're testing the extended parameter handlings, add an extra string.
         */
        if (device_param_flag & 1) {
            length = ntohs(strlen(test_param));
            memcpy(base, &length, sizeof(length));
            base += sizeof(length);

            memcpy(base, &test_param[0], strlen(test_param));
            base += strlen(test_param);
        }

        break;

    case Send_state_list:
        sent_count = ntohs(array_size(devices));

        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        if (test_flags == send_short_state) {
            memcpy(base, &sent_count, sizeof(sent_count));
            base += sizeof(sent_count);
            break;
        }

        for (i = 0; i < array_size(devices); i++) {
            deviceId = ntohll(devices[i]);
            memcpy(base, &deviceId, sizeof(deviceId));
            base += sizeof(deviceId);

            device_state = ntohll(1);
            memcpy(base, &device_state, sizeof(device_state));
            base += sizeof(device_state);

            wakeup_requested = 1;
            memcpy(base, &wakeup_requested, sizeof(wakeup_requested));
            base += sizeof(wakeup_requested);
        }

        break;

    case Send_sleep_setup:
        packInt(1);     // sleep type
        packEncrypted(macAddress,  sizeof(macAddress));
        packEncrypted(sleepPacket, sizeof(sleepPacket));
        packEncrypted(wakeupKey,   sizeof(wakeupKey));
        packBytes    (clusterName, strlen(clusterName));
        packInt(1024);  // sleep port
        packInt(   5);  // sleep interval
        break;
    }

    /*
     *  Pack the message itself, if present.
     */
    switch (req_type) {
    case Send_unicast:
    case Send_multicast:
        message_length = ntohs(sizeof(message));
        memcpy(base, &message_length, sizeof(message_length));
        base += sizeof(message_length);

        memcpy(base, message, sizeof(message));
        base += sizeof(message);
        break;

    }

    /*
     *  Add the signature.
     */
    if (req_type != Send_challenge) {
        sign_packet(packet, req_length);
        base += signature_size;
    }

    if (base - packet != req_length) {
        log("test: *** Packet creation failed for type %d.\n", (int) req_type);
        exit(1);
    }

    *output = packet;
    *output_length = req_length;
}

static void
send_packet
(
    ans_client_t *  client,
    int             fd,
    int             req_type,
    crypto_t *      crypto,
    int             do_tests,
    int             test_flags
)
{
    int     sent_bytes;
    int     count;
    char *  packet;
    int     length;
    char *  base;

    packet_t *  test_packet;

    make_packet(client, req_type, crypto, &packet, &length, test_flags);

    /*
     *  Make a quick test of check_signature and unpack.
     */
    if (do_tests) {
        packet[0]++;
        test_packet = alloc_packet(packet, length);

        if (check_signature(crypto, test_packet)) {
            log("test: *** check_signature failed to detect a corrupt packet!\n");
            exit(1);
        }

        if (unpack(client, test_packet, 0) != null) {
            log("test: *** unpack failed to detect a bad check length!\n");
            exit(1);
        }

        test_packet->buffer = null;
        test_packet->base   = null;
        free_packet(&test_packet);

        packet[0]--;
    }

    sent_bytes = 0;
    base       = packet;

    log("test: sending sequence %d, type %d\n", (int) req_sequenceId,
        (int) req_type);

    while (sent_bytes < length) {
        count = write(fd, base, length - sent_bytes);

        if (count <= 0) {
            perror("write");
            log("test: *** Write failed\n");
            exit(1);
        }

        base       += count;
        sent_bytes += count;
    }

    free(packet);

    if (req_type != Send_challenge) {
        req_sequenceId++;
    }
}

static void
read_bytes(int fd)
{
    char buffer[2];
    int  count;
    int  flags;

    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    do {
        count = read(fd, buffer, sizeof(buffer));
    } while (count > 0);

    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

static void
no_data_test(ans_client_t *client, int type)
{
    describe_t  describe;

    clear_describe(&describe);
    describe.type = type;

    if (pack(client, &describe) != null) {
        log("test: *** pack(%s) should have failed!\n",
            packet_type(type));
        exit(1);
    }
}

static void
test_unpack_bytes(ans_client_t *client)
{
    char    buffer[80];
    char *  array;
    int     size;
    short   length;

    memset(buffer, 0, sizeof(buffer));
    array = null;

    if (unpack_bytes(buffer + 1, buffer, &array, "test buffer 1") >= 0) {
        log("test: *** unpack_bytes didn't complain about a short buffer!\n");
        exit(1);
    }

    length = ntohs(array_size(buffer) - sizeof(length) + 1);
    memcpy(buffer, &length, sizeof(length));

    if (unpack_bytes(buffer + array_size(buffer), buffer, &array, "test buffer 2") >= 0) {
        log("test: *** unpack_bytes didn't complain about an oversized count!\n");
        exit(1);
    }

    if (unpack_encrypted(client, buffer + 1, buffer, &array, &size, "test field 1") != null) {
        log("test: *** unpack_encrypted didn't complain about a short buffer!\n");
        exit(1);
    }

    length = ntohs(23);
    memcpy(buffer, &length, sizeof(length));

    if (unpack_encrypted(client, buffer + sizeof(buffer), buffer, &array, &size, "test field 2") != null) {
        log("test: *** unpack_encrypted didn't complain about a short buffer!\n");
        exit(1);
    }
}

static void
packet_test(ans_client_t *client)
{
    packet_t *  packet;
    describe_t  describe;

    log("test: === Starting the packet test.\n");

    /*
     *  Try making a couple of packet types that require data,
     *  without specifying any data.
     */
    no_data_test(client, Send_timed_ping);
    no_data_test(client, Request_sleep_setup);

    /*
     *  unpack_bytes should get a bit of a test.
     */
    test_unpack_bytes(client);

    /*
     *  Try packing a very long notification.
     */
    clear_describe(&describe);
    describe.type  = Send_unicast;
    describe.count = 10000;
    describe.data  = malloc(describe.count);
    memset(describe.data, 0, describe.count);

    if (pack(client, &describe) != null) {
        log("test: *** pack didn't detect a long notification.\n");
        exit(1);
    }

    free(describe.data);

    /*
     *  Try packing an invalid type.
     */
    clear_describe(&describe);
    describe.type = 9999;

    packet = pack(client, &describe);

    if (packet != NULL) {
        log("test: *** pack(invalid) should fail!\n");
        exit(1);
    }
}

#if 0
static VPLTHREAD_FN_DECL
reader(void *data)
{
   int   fd;
   int   result;
   char  buffer[80];

   fd = (int) data;

   do {
       result = read(fd, buffer, sizeof(buffer));
   } while (result > 0);    // not thread-safe

    log("test: A reader exited\n");
    return VPLTHREAD_RETURN_VALUE;
}

static void
start_reader(int fd)
{
    int  rc;

    if (!enable_reader) {
        return;
    }

    VPLDetachableThreadHandle_t thread;

    rc = VPLDetachableThread_Create(&thread, reader, (void *) fd, NULL, "ans");

    if (rc == 0) {
        VPLDetachableThread_Detach(&thread);
    }
}
#endif

static void
read_data(int fd, void *buffer, int length)
{
    char *  base;
    int     count;
    int     remaining;

    base       = (char *) buffer;
    remaining  = length;

    while (remaining > 0) {
        count = read(fd, base, remaining);

        if (count == 0 || (count < 0 && (errno == EAGAIN))) {
            VPLThread_Sleep(VPLTime_FromMillisec(100));
        } else if (count < 0) {
            perror("receive_login read:  ");
            exit(1);
        } else {
            remaining -= count;
            base      += count;
        }
    }
}

static void
receive_login(int fd, crypto_t *crypto)
{
    int       has_tag;
    uint16_t  tag_flag;
    uint16_t  check_length;
    char *    body;
    int       length;
    char *    base;

    ans_unpacked_t *  unpacked;
    packet_t          packet;

    read_data(fd, &tag_flag, sizeof(tag_flag));

    has_tag = (uint16_t) htons(tag_flag) == server_tag_flag;

    if (!has_tag) {
        check_length = tag_flag;
    } else {
        read_data(fd, &check_length, sizeof(check_length));
    }

    length = htons(~check_length);
    body   = (char *) malloc(length);
    base   = body;

    if (has_tag) {
        memcpy(base, &tag_flag, sizeof(tag_flag));
        base += sizeof(tag_flag);
    }

    memcpy(base, &check_length, sizeof(tag_flag));
    base += sizeof(check_length);

    read_data(fd, base, length - 4);

    memset(&packet, 0, sizeof(packet));

    packet.base    = body;
    packet.buffer  = body;
    packet.length  = length;

    unpacked = unpack(null, &packet, null);

    tag = VPLConv_ntoh_u64(unpacked->tag);

    free_unpacked(&unpacked);
    free(body);
}

static int
start_connection(int server_fd, crypto_t *crypto, int get_tag)
{
    struct sockaddr  client_address;

    socklen_t  client_length;
    int        fd;
    int        tries;

    memset(&client_address, 0, sizeof(client_address));
    client_length = 0;
    tag = 0;

    do {
        fd = accept(server_fd, &client_address, &client_length);

        if (fd < 0) {
            perror("accept");
            log("test: *** No client accepted\n");
            close(server_fd);
            exit(1);
        }

        log("accepted fd %d\n", (int) fd);

        req_sequenceId = 0;
        send_packet(NULL, fd, Send_challenge, crypto, false, 0);

        if (get_tag) {
            receive_login(fd, crypto);
        }

        tries = 0;

        while (!got_connection && tries++ < 10) {
            sleep(1);
        }
    } while (!got_connection);

    return fd;
}

static void
test_misc(ans_client_t *client)
{
    uint64_t    devices[]      = { 10101, 20202, 30303 };
    uint64_t    users[]        = { 11111, 22222, 33333 };
    uint64_t    current_time;
    int64_t     next_send_time;
    int64_t     delay;
    int64_t     expected;
    int32_t     saved_subs;
    int         huge_count;
    uint64_t *  huge;
    int32_t     saved_time;
    int         result;
    int         verbose;
    describe_t  describe;
    packet_t *  packet;
    char *      body;
    packet_t    short_packet;
    int         short_size;
    int         pass;

    log("test: === Running the miscellaneous tests.\n");

    /*
     *  Test the case where the current time is past the next
     *  send time.
     */
    delay          = VPLTime_FromSec(5);
    current_time   = delay * 10;
    next_send_time = current_time - 6 * delay;
    expected       = current_time;

    client->mt.inited = false;

    if (next(&client->keep, next_send_time, delay, current_time) != expected) {
        log("test: *** next() failed\n");
        exit(1);
    }

    client->mt.inited = true;

    saved_time      = ans_tcp_ka_time;
    ans_tcp_ka_time = 0;

    result = ans_set_keep_alive(client->socket);

    if (result != VPL_OK) {
        log("test: *** ans_set_keep_alive 1 failed:  %d\n", (int) result);
        exit(1);
    }

    ans_tcp_ka_time = saved_time;

    result = ans_set_keep_alive(client->socket);

    if (result != VPL_OK) {
        log("test: *** ans_set_keep_alive 2 failed:  %d\n", (int) result);
        exit(1);
    }

    if (ans_setSubscriptions(client, -1, users, -1)) {
        log("test: *** send subscriptions worked with a negative count!\n");
        exit(1);
    }

    /*
     *  Make sure ans_setForeground makes some sense.
     */
    if (!ans_setForeground(client, false, 1000)) {
        log("test: *** ans_setForeground failed!\n");
        exit(1);
    }

    if (client->is_foreground) {
        log("test: *** ans_setForeground failed to set the state!\n");
        exit(1);
    }

    if (!ans_setForeground(client, true, 2000)) {
        log("test: *** ans_setForeground failed!\n");
        exit(1);
    }

    if (!client->is_foreground) {
        log("test: *** ans_setForeground failed to set the state!\n");
        exit(1);
    }

    verbose = false;

    if (!ans_setVerbose(client, verbose) || client->verbose != verbose) {
        log("test: *** ans_setVerbose failed to set the mode!\n");
        exit(1);
    }

    if (!ans_setVerbose(client, true) || !client->verbose) {
        log("test: *** ans_setVerbose didn't restore the mode!\n");
        exit(1);
    }

    /*
     *  Okay, try a huge subscription list.
     */
    saved_subs    = ans_max_subs;
    rejected_subs =    0;
    huge_count    = 1000;
    ans_max_subs  =   32;

    huge = (uint64_t *) malloc(huge_count * sizeof(uint64_t));

    memset(huge, 0, huge_count * sizeof(uint64_t));

    if (ans_setSubscriptions(client, -1, huge, huge_count)) {
        log("test: *** send subscriptions worked with a huge count!\n");
        exit(1);
    }

    free(huge);

    if (rejected_subs != 1) {
        log("test: *** The rejectSubscriptions callback didn't occur!\n");
        exit(1);
    }

    ans_max_subs = saved_subs;

    if (!ans_setSubscriptions(client, -1, users, array_size(users))) {
        log("test: *** send subscriptions failed\n");
        exit(1);
    }

    if (received_count > 0 && !ans_requestDeviceState(client, req_query_id, devices, array_size(devices))) {
        log("test: *** request device state failed\n");
        exit(1);
    }

    if (!ans_requestWakeup(client, devices[0])) {
        log("test: *** request wakeup failed\n");
        exit(1);
    }

    if (ans_requestDeviceState(client, 0, devices, 0)) {
        log("test: *** ans_requestDeviceState failed the zero count test\n");
        exit(1);
    }

    if (received_count > 0 && ans_requestDeviceState(client, 0, devices, ans_max_query + 1)) {
        log("test: *** ans_requestDeviceState failed the long count test\n");
        exit(1);
    }

    clear_describe(&describe);
    describe.type  = Query_device_list;
    describe.data  = devices;
    describe.count = 0;

    if (pack(client, &describe) != null) {
        log("test: *** pack(Query_device_list) failed the zero count test\n");
        exit(1);
    }

    clear_describe(&describe);
    describe.type  = Query_device_list;
    describe.data  = devices;
    describe.count = ans_max_subs + 1;

    if (pack(client, &describe) != null) {
        log("test: *** pack(Query_device_list) failed the long subscription test\n");
        exit(1);
    }

    clear_describe(&describe);
    describe.type       = Request_wakeup;
    describe.device_id  = devices[0];

    packet = pack(client, &describe);

    if (packet == null) {
        log("test: *** pack(Request_wakeup) failed\n");
        exit(1);
    }

    free_packet(&packet);

    if (ans_background(client)) {
        log("test: *** ans_background(client) didn't detect a foreground client!\n");
        exit(1);
    }

    clear_describe(&describe);
    describe.type = -1;

    if (pack(client, &describe)) {
        log("test: *** pack(-1) worked!\n");
        exit(1);
    }

    /*
     *  Send a short packet to is_blob_rejection.
     */
    ans_short_packets = 0;

    short_size = 2 * sizeof(uint16_t) - 1;
    body       = (char *) malloc(short_size);

    short_packet.buffer = body;
    short_packet.base   = body;
    short_packet.length = short_size;

    pass = !is_blob_rejection(&short_packet);

    if (!pass || ans_short_packets != 1) {
        log("test: *** is_blob_rejection failed on a short packet\n"); 
        exit(1);
    }

    free(body);
}

static void
test_malloc_errors(ans_open_t *input)
{
    int  i;
    int  pass;

    uint64_t      ping_data[2] = {  4,  5 };
    char          mac[6]       = {  1,  2,  3,  4,  5,  6 };
    describe_t    describe;
    ans_client_t  null_client;
    packet_t *    packet;
    int64_t       sequence;
    char *        body;
    int           length;

    memset(&null_client, 0, sizeof(null_client));

    null_client.crypto               = create_crypto(key, sizeof(key));
    null_client.keep.client          = &null_client;
    null_client.keep.rtt_estimate    = 0;
    null_client.keep.time_to_millis  = VPLTime_FromMillisec(1);

    log("test: === Starting the malloc failure test.\n");

    /*
     *  send_message should fail quickly on malloc failures.
     */
    for (i = 1; i <= 2; i++) {
        clear_describe(&describe);
        describe.type  = Send_timed_ping;
        describe.data  = ping_data;
        describe.count = sizeof(ping_data);

        force_malloc_error = i;
        pass = send_message(&null_client, &describe);

        if (pass || force_malloc_error != 0) {
            log("test: *** send_message didn't trigger a malloc failure.\n");
            exit(1);
        }
    }

    for (i = 1; i <= 2; i++) {
        force_malloc_error = i;

        if (send_ping(&null_client) || force_malloc_error != 0) {
            log("test: *** send_ping didn't trigger a malloc failure.\n"
                ", round %d.\n", i);
            exit(1);
        }
    }

    force_malloc_error = 1;

    if (create_crypto(null, 0) != null || force_malloc_error != 0) {
        log("test: *** create_crypto didn't trigger a malloc failure.\n");
        exit(1);
    }

    force_malloc_error = 1;

    if (ans_setSubscriptions(&null_client, 0, null, 1)|| force_malloc_error != 0) {
        log("test: *** ans_setSubscriptions didn't trigger a malloc failure.\n");
        exit(1);
    }

    for (int i = 1; i <= 6; i++) {
        force_malloc_error = i;

        if (ans_open(input) != null || force_malloc_error > 0) {
            log("test: *** ans_open didn't trigger a malloc failure.\n");
            exit(1);
        }
    }

    force_malloc_error = 1;

    if (send_udp_ping(&null_client.keep, "test") || force_malloc_error > 0) {
        log("test: *** send_udp_ping didn't trigger a malloc failure.\n");
        exit(1);
    }

    force_malloc_error = 1;

    if (send_tcp_ping(&null_client.keep, "test malloc") || force_malloc_error > 0) {
        log("test: *** send_tcp_ping didn't trigger a malloc failure.\n");
        exit(1);
    }

    clear_describe(&describe);
    describe.type  = Send_ping;

    sequence = 0;

    packet = pack(&null_client, &describe);
    prep_packet(&null_client, packet, &sequence);

    force_malloc_error = 1;

    if
    (
        process_packet(&null_client, packet, &sequence) != null
    ||  force_malloc_error > 0
    ) {
        log("test: *** process_packet didn't trigger a malloc failure.\n");
        exit(1);
    }

    free_packet(&packet);

    /*
     *  Force a malloc failure in write_output.  This test also checks
     *  pack_queued.
     */
    packet = pack(&null_client, &describe);
    free(packet->buffer);

    packet->buffer        = null;
    packet->base          = null;
    packet->next          = null;
    null_client.tcp_head  = packet;
    null_client.tcp_tail  = packet;

    mutex_init(&null_client.mutex);

    force_malloc_error = 1;

    write_output(&null_client);

    if (force_malloc_error > 0) {
        log("test: *** write_output didn't trigger a malloc failure.\n");
        exit(1);
    }

    free_packet(&packet);

    /*
     *  Force a failure in encryption when packing a sleep setup request.
     */
    clear_describe(&describe);
    describe.type  = Request_sleep_setup;
    describe.data  = mac;
    describe.count = sizeof(mac);

    force_malloc_error = 1;

    if (pack(&null_client, &describe) != null || force_malloc_error > 0) {
        log("test: *** pack(Request_sleep_setup) didn't trigger a malloc error!");
        exit(1);
    }

    /*
     *  Force a malloc error in Send_state_list processing.
     */
    make_packet(&null_client, Send_state_list, null_client.crypto, &body, &length, 0);
    packet = alloc_packet(body, length);

    force_malloc_error = 2;
    sequence = 0;

    if (unpack(&null_client, packet, &sequence) != null || force_malloc_error > 0) {
        log("test: *** unpack(Send_state_list) didn't trigger a malloc error!");
        exit(1);
    }

    free_packet(&packet);

    test_unpack_malloc(&null_client, Send_sleep_setup, 8);

    VPLMutex_Destroy(&null_client.mutex);
    free_crypto(&null_client.crypto);
    /* read_packet 2 */
}

static void
test_short_packets(void)
{
    ans_client_t  null_client;

    memset(&null_client, 0, sizeof(null_client));

    null_client.crypto               = create_crypto(key, sizeof(key));
    null_client.keep.client          = &null_client;
    null_client.keep.rtt_estimate    = 0;
    null_client.keep.time_to_millis  = VPLTime_FromMillisec(1);

    test_short_packet(&null_client, Send_unicast);
    test_short_packet(&null_client, Send_multicast);
    test_short_packet(&null_client, Send_sleep_setup);
    test_short_packet(&null_client, Send_challenge);
    test_short_packet(&null_client, Send_state_list);
    test_short_packet(&null_client, Send_device_update);
    test_short_packet(&null_client, Send_timed_ping);
    test_short_packet(&null_client, Send_response);

    free_crypto(&null_client.crypto);
}

static void
test_sync_open(int server_fd, ans_open_t *input, crypto_t *crypto)
{
    ans_client_t *  client;

    int32_t  saved_min;
    int32_t  saved_max;
    int      loops;

    log("test: === Starting the sync_open_socket test.\n");

    saved_min     = ans_min_delay;
    saved_max     = ans_max_delay;
    ans_min_delay = 0;
    ans_max_delay = 1;

    for (int i = 1; i <= 4; i++) {
        ans_connect_errors  = 0;
        ans_force_socket    = i;
        ans_force_change    = i == 1;

        client = ans_open(input);
        loops  = 0;

        while (ans_connect_errors == 0 && loops++ < 10) {
            sleep(1);
        }

        if (ans_connect_errors == 0) {
            log("test: *** sync_open_socket didn't fail!\n");
            exit(1);
        }

        ans_close(client, VPL_TRUE);
    }

    if (ans_force_socket > 0) {
        log("test: *** ans_force_socket is still set\n");
        exit(1);
    }

    ans_min_delay = saved_min;
    ans_max_delay = saved_max;
    log("test: === The sync_open_socket test passed.\n");
}

static void
flush_accept_queue(int server_fd)
{
    struct sockaddr  client_address;
    struct pollfd    pollfd;
    socklen_t        client_length;

    int  fd;

    sleep(1);

    pollfd.fd       = server_fd;
    pollfd.events   = POLLIN;
    pollfd.revents  = 0;
    client_length   = sizeof(client_address);

    memset(&client_address, 0, sizeof(client_address));

    do {
        poll(&pollfd, 1, 10);

        if (pollfd.revents & POLLIN) {
            fd = accept(server_fd, &client_address, &client_length);
            close(fd);
        }
    } while (pollfd.revents != 0);
}

static void
test_slow_login(int server_fd, ans_open_t *input, crypto_t *crypto)
{
    struct sockaddr  client_address;
    socklen_t        client_length;
    ans_client_t *   client;

    int  fd;
    int  save;

    log("test: === Starting the slow login test.\n");
    save = ans_partial_timeout;

    ans_partial_timeout  = 3;
    ans_partial_timeouts = 0;

    flush_accept_queue(server_fd);
    client = ans_open(input);

    client_length = sizeof(client_address);

    fd = accept(server_fd, &client_address, &client_length);

    if (fd < 0) {
        perror("accept");
        log("test: *** No client accepted in test_slow_login\n");
        close(server_fd);
        exit(1);
    }

    sleep(ans_partial_timeout + 2);

    if (ans_partial_timeouts != 1) {
        log("test: *** test_slow_login didn't get a partial packet timeout.\n");
        close(server_fd);
        exit(1);
    }

    ans_close(client, true);
    ans_partial_timeout = save;;
    log("test: === The slow login test passed.\n");
}

static void
test_run_device(int server_fd, ans_open_t *input, crypto_t *crypto)
{
    ans_client_t *  client;

    int32_t  saved_min;
    int32_t  saved_max;
    int      loops;
    int      countdown;

    log("test: === Starting the run_device test.\n");

    saved_min     = ans_min_delay;
    saved_max     = ans_max_delay;
    ans_min_delay = 0;
    ans_max_delay = 0;
    countdown     = 7;

    flush_accept_queue(server_fd);

    client = ans_open(input);

    do {
        loops = 0;

        while (invalid(client->socket) && loops++ < 3) {
            sleep(1);
        }

        if (invalid(client->socket)) {
            log("test: *** The run_device thread didn't create a socket\n");
            exit(1);
        }

        free_crypto(&client->crypto);

        force_malloc_error = countdown--;

        start_connection(server_fd, crypto, false);

        loops = 0;

        while (force_malloc_error > 0 && loops++ < 3) {
            sleep(1);
        }

        if (force_malloc_error > 0) {
            log("test: *** The malloc countdown failed.\n");
            exit(1);
        }
    } while (countdown > 0);

    ans_close(client, true);

    flush_accept_queue(server_fd);

    client = ans_open(input);
    force_malloc_error = 7;
    start_connection(server_fd, crypto, false);
    sleep(1);
    ans_close(client, true);

    flush_accept_queue(server_fd);
    ans_min_delay = saved_min;
    ans_max_delay = saved_max;
    tag = 0;
    log("test: === The run_device test passed.\n");
}

static void
test_keep_alive(ans_client_t *client)
{
    uint64_t  ping_data[5]   = { 0, 1, 2, 3, 4 };
    int64_t   sequence_id;
    int       pass;
    uint16_t  sent_count;
    int       i;
    int       short_count;

    describe_t  describe;
    packet_t *  result;

    log("test: === Starting the keep-alive coverage test.\n");

    /*
     *  Set up the ping data.
     */
    ping_data[0]  = VPLTime_GetTime();
    ping_data[1]  = client->keep.rtt_estimate;
    ping_data[2]  = client->connection;

    /*
     *  Try creating and parsing a valid keep-alive packet.
     */
    estimate_rtt(&client->keep);

    clear_describe(&describe);

    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);
    describe.tag   = client->tag;

    result = pack(client, &describe);

    if (result == null) {
        log("test: *** pack(Send_timed_ping) failed\n");
        exit(1);
    }

    prep_packet(client, result, &client->keep.out_sequence);

    /*
     *  prep_packet should handle a prepared packet...
     */
    sequence_id = client->keep.out_sequence;
    prep_packet(client, result, &client->keep.out_sequence);

    if (sequence_id != client->keep.out_sequence) {
        log("test: *** prep_packet should be idempotent.");
        exit(1);
    }

    pass = process_udp_keep_alive(&client->keep, result->base, result->length);

    if (!pass) {
        log("test: *** process_udp_keep_alive failed on a valid packet:  1\n");
        exit(1);
    }

    free_packet(&result);

    /*
     *  Okay, try a duplicate sequence id.
     */
    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);
    describe.tag   = client->tag;

    result = pack(client, &describe);

    client->keep.out_sequence--;
    prep_packet(client, result, &client->keep.out_sequence);

    pass = process_udp_keep_alive(&client->keep, result->base, result->length);

    if (pass) {
        log("test: *** process_udp_keep_alive failed to detect a duplicate\n");
        exit(1);
    }

    free_packet(&result);

    /*
     *  Okay, try a bad signature.
     */
    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);
    describe.tag   = client->tag;

    result = pack(client, &describe);

    prep_packet(client, result, &client->keep.out_sequence);

    result->base[result->length - 2]++;

    pass = process_udp_keep_alive(&client->keep, result->base, result->length);

    if (pass) {
        log("test: *** process_udp_keep_alive failed to detect a bad signature\n");
        exit(1);
    }

    /*
     *  While we have a bad signature, try process_packet.
     */
    sequence_id = 0;

    if (process_packet(client, result, &sequence_id) != null) {
        log("test: *** process_packet failed to detect a bad signature\n");
        exit(1);
    }

    free_packet(&result);

    /*
     *  Now try a packet with a count greater than the buffer size.
     */
    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);
    describe.tag   = client->tag;

    result = pack(client, &describe);

    if (result == null) {
        log("test: *** pack(Send_timed_ping) failed\n");
        exit(1);
    }

    prep_packet(client, result, &client->keep.out_sequence);
    sent_count = (result->length + 1) ^ -1;
    sent_count = VPLConv_ntoh_u16(sent_count);
    memcpy(result->base, &sent_count, sizeof(sent_count));
    ans_bad_keep_sizes = 0;

    pass = process_udp_keep_alive(&client->keep, result->base, result->length);

    if (pass || ans_bad_keep_sizes != 1) {
        log("test: *** process_udp_keep_alive didn't detect a long packet.\n");
        exit(1);
    }

    /*
     *  Now try processing a packet that's too short.
     */
    short_count = header_size + signature_size - 1;
    pass        = process_udp_keep_alive(&client->keep, result->base, short_count);

    if (pass) {
        log("test: *** process_udp_keep_alive didn't detect a short packet.\n");
        exit(1);
    }

    free_packet(&result);

    /*
     *  process_udp_keep_alive should detect an invalid connection id.
     */
    ping_data[2]++;
    ans_keep_connection = 0;

    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);
    describe.tag   = client->tag;

    result = pack(client, &describe);

    prep_packet(client, result, &client->keep.out_sequence);

    pass = process_udp_keep_alive(&client->keep, result->base, result->length);

    if (pass || ans_keep_connection == 0) {
        log("test: *** process_udp_keep_alive failed to detect a bad connection id\n");
        exit(1);
    }

    free_packet(&result);

    /*
     *  Set the connection back, and the parsing should pass.
     */
    ping_data[2]--;
    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = ping_data;
    describe.count = sizeof(ping_data);
    describe.tag   = client->tag;

    result = pack(client, &describe);

    sequence_id = client->keep.in_sequence + 1;
    prep_packet(client, result, &sequence_id);

    pass = process_udp_keep_alive(&client->keep, result->base, result->length);

    if (!pass) {
        log("test: *** process_udp_keep_alive failed on a valid packet:  2\n");
        exit(1);
    }

    free_packet(&result);

    /*
     *  Now test sending an invalid packet type to the keep-alive handler.
     */
    clear_describe(&describe);
    describe.type    = Set_login_version;
    describe.version = 3;
    describe.tag   = client->tag;

    result = pack(client, &describe);

    if (result == null) {
        log("test: *** pack(Set_login_version) failed!\n");
        exit(1);
    }

    sequence_id = client->keep.in_sequence + 1;
    prep_packet(client, result, &sequence_id);

    if (process_udp_keep_alive(&client->keep, result->base, result->length)) {
        log("test: *** process_udp_keep_alive didn't detect a bad type.\n");
        exit(1);
    }

    free_packet(&result);

    for (i = 1; i <= 2; i++) {
        clear_describe(&describe);
        describe.type  = Send_timed_ping;
        describe.data  = ping_data;
        describe.count = sizeof(ping_data);
        describe.tag   = client->tag;

        result = pack(client, &describe);

        prep_packet(client, result, &client->keep.out_sequence);

        force_malloc_error = i;

        if (process_udp_keep_alive(&client->keep, result->base, result->length)) {
            log("test: process_udp_keep_alive didn't fail in malloc at iteration %d\n",
                (int) i);
            exit(1);
        }

        if (force_malloc_error > 0) {
            log("test: force_malloc_error didn't trigger at iteration %d.\n", i);
            exit(1);
        }

        free_packet(&result);
    }

    free_packet(&result);
    log("test: === The keep-alive coverage test passed.\n");
}

static void
cover_locks(void)
{
    mutex_lock(null);

    if (ans_lock_errors != 1) {
        log("test: *** mutex_lock didn't complain about a null lock!\n");
        exit(1);
    }

    mutex_init(null);

    if (ans_lock_errors != 2) {
        log("test: *** mutex_init didn't complain about a null lock!\n");
        exit(1);
    }

    ans_lock_errors = 0;
}

/*
 *  Do some tests using a fake client.  There's no thread running,
 *  so some things are easier.
 */
static void
test_fake(void)
{
    ans_client_t *    fake_client;
    ans_callbacks_t   callbacks;
    uint64_t          devices[]      = { 10101, 20202, 30303 };
    packet_t *        packet;
    packet_t *        output;
    ans_unpacked_t *  unpacked;
    describe_t        describe;
    const char *      body           = "hello!";
    event_t           event;

    int64_t   start_time;
    int       pass;
    int32_t   saved_query;
    limits_t  limits;
    int       i;
    int       count;
    int64_t   sequence;

    log("test: === Starting the fake client tests.\n");

    /*
     *  Make a fake device client for a number of small tests.
     */
    fake_client = (ans_client_t *) malloc(sizeof(*fake_client));

    memset(fake_client, 0, sizeof(*fake_client));
    memset(&limits,     0, sizeof(limits));

    make_locks(fake_client);

    init_event(&fake_client->keep.event);
    make_event(&fake_client->keep.event);

    callbacks.connectionActive       = t_connectionActive;
    callbacks.receiveNotification    = t_receiveNotification;
    callbacks.receiveSleepInfo       = t_receiveSleepInfo;
    callbacks.receiveDeviceState     = t_receiveDeviceState;
    callbacks.connectionDown         = t_connectionDown;
    callbacks.connectionClosed       = t_connectionClosed;
    callbacks.setPingPacket          = t_set_ping_packet;
    callbacks.loginCompleted         = t_login_completed;
    callbacks.rejectCredentials      = t_reject_credentials;
    callbacks.rejectSubscriptions    = t_reject_subscriptions;
    callbacks.receiveResponse        = t_receive_response;
    fake_client->callback_table      = callbacks;

    fake_client->keep.who  [0]  = "test 1";
    fake_client->keep.id   [0]  = 101;
    fake_client->keep.sends[0]  = VPLTime_GetTimeStamp() + 1000;

    fake_client->keep.who  [1]  = "test 2";
    fake_client->keep.id   [1]  = 102;
    fake_client->keep.sends[1]  = VPLTime_GetTimeStamp() + 1234;

    dump_sends   (&fake_client->keep, &limits, "test");
    dump_receives(&fake_client->keep);

    fake_client->keep.ping_when[2]  = VPLTime_GetTime();
    fake_client->keep.ping_rtt [2]  = 500;
    fake_client->keep.ping_id  [2]  = 101;

    fake_client->keep.ping_when [3]  = VPLTime_GetTime() + 12345;
    fake_client->keep.ping_rtt [3]  = 700;
    fake_client->keep.ping_id  [3]  = 102;

    fake_client->keep.last_tcp_receive     = VPLTime_GetTime();

    dump_receives(&fake_client->keep);

    /*
     *  First, check the deadline computation.
     */
    memset(&limits, 0, sizeof(limits));
    limits.base_delay  = 1000;
    limits.retry_count =    5;

    /*
     *  Now, make sure that ans_close detects an invalid client.
     */
    fake_client->open         = ~open_magic;
    fake_client->socket       = VPLSOCKET_INVALID;
    fake_client->keep.client  = fake_client;
    ans_invalid_closes        = 0;

    ans_close(fake_client, false);

    if (ans_invalid_closes != 1) {
        log("test: *** ans_close(invalid) did not fail\n");
        exit(1);
    }

    /*
     *  ans_background should detect a bad client, too.
     */
    if (ans_background(fake_client)) {
        log("test: *** ans_background(invalid) did not fail\n");
        exit(1);
    }

    /*
     *  ans_setForeground should detect a bad client.
     */
    if (ans_setForeground(fake_client, false, 0)) {
        log("test: *** ans_setForeground failed to detect a bad client!\n");
        exit(1);
    }

    /*
     *  ans_setVerbose does the same.
     */
    if (ans_setVerbose(fake_client, false)) {
        log("test: *** ans_setVerbose failed to detect a bad client!\n");
        exit(1);
    }

    /*
     *  Okay, make the client valid, in background mode, and pretend that
     *  it just got a ping.  It should return success without killing the
     *  socket.
     */
    ans_timeout_events        = 0;
    ans_ping_back             = 5;
    fake_client->open         = open_magic;
    fake_client->interval     = VPLTime_FromSec(30);

    fake_client->keep.last_tcp_receive  = VPLTime_GetTime();
    fake_client->keep.time_to_millis    = VPLTime_FromMillisec(1);

    if (!ans_background(fake_client)) {
        log("test: *** ans_background failed on a valid client 1\n");
        exit(1);
    }

    if (ans_timeout_events != 0) {
        log("test: *** ans_background declared a timeout!\n");
        exit(1);
    }

    if (ans_back_loops != 0) {
        log("test: *** ans_background waited with a current ping!\n");
        exit(1);
    }

    /*
     *  send_udp_ping should fail without a cryptography key configured.
     */
    if (send_udp_ping(&fake_client->keep, "test")) {
        log("test: *** send_udp_ping didn't detect a missing key.\n");
        exit(1);
    }

    /*
     *  Now try timing out a client.
     */
    fake_client->keep.last_tcp_receive  = VPLTime_GetTime();
    fake_client->keep.last_tcp_receive -= fake_client->interval;
    ans_back_loops = 0;

    if (!ans_background(fake_client)) {
        log("test: *** ans_background failed on a valid client 2\n");
        exit(1);
    }

    if (ans_timeout_events != 1) {
        log("test: *** ans_background(invalid) failed to declare a timeout.\n");
        exit(1);
    }

    if (ans_back_loops < 2) {
        log("test: *** ans_background didn't wait long enough (%d vs %d)!\n",
            (int) ans_back_loops, (int) ans_ping_back);
        exit(1);
    }

    fake_client->keep.live_socket_id = -1;

    if (!ans_background(fake_client)) {
        log("test: *** ans_background failed on a dead socket\n");
        exit(1);
    }

    declare_down(fake_client, "test");
    ans_ping_back  = 0;

    /*
     *  Now try send_subscriptions on a huge subscription list.
     */
    fake_client->subscriptions       = (uint64_t *) malloc(128);
    fake_client->subscription_count  = 1000;

    ans_max_subs   = 32;
    rejected_subs  = 0;

    send_subscriptions(fake_client);

    if (rejected_subs != 1 || fake_client->subscriptions != null) {
        log("test: *** send_subscriptions failed to detect a bad count!\n");
        exit(1);
    }

    /*
     *  Try write_output with a null queue.
     */
    ans_no_output = 0;
    write_output(fake_client);

    if (ans_no_output != 1) {
        log("test: *** write_output with a null queue failed!\n");
        exit(1);
    }

    /*
     *  Force a simulated backwards time change in sleep_seconds.  First,
     *  get the event structure into shape.
     */
    free_event(&fake_client->event);
    make_event(&fake_client->event);

    start_time         = VPLTime_GetTimeStamp();
    ans_force_time     = 1;
    ans_backwards_time = 0;

    sleep_seconds(fake_client, 1);

    if (ans_backwards_time != 1) {
        log("test: *** The ans_backwards_time count is wrong.\n");
        exit(1);
    }

    /*
     *  Force ans_requestDeviceState to defer an operations.
     */
    saved_query   = ans_max_query;
    ans_max_query = 0;

    free(fake_client->subscriptions);
    fake_client->subscriptions = null;
    pass = ans_requestDeviceState(fake_client, 0, devices, array_size(devices));

    ans_max_query = saved_query;

    if (pass || fake_client->subscriptions != null) {
        log("test: *** ans_requestDeviceState failed to defer!\n");
        exit(1);
    }

    fake_client->keep.client = fake_client;

    /*
     *  process_udp_keep_alive should bail very early with a null crypto
     *  structure.
     */
    process_udp_keep_alive(&fake_client->keep, null, 0);

    /*
     *  Do some minimal checking of the messaging packets.
     */
    memset(&describe, 0, sizeof(describe));

    fake_client->crypto = create_crypto(key, sizeof(key));
    describe.type       = Send_unicast;
    describe.count      = strlen(body);
    describe.data       = (void *) body;
    describe.async_id   =  108010;
    describe.user_id    =  describe.async_id + 1;
    describe.device_id  =  describe.async_id + 2;

    packet = pack(fake_client, &describe);

    prep_packet(fake_client, packet, &fake_client->out_sequence);

    unpacked = unpack(fake_client, packet, null);

    if (unpacked->userId != describe.user_id) {
        log("test: *** The unicast user id is incorrect!\n");
        exit(1);
    }

    if (unpacked->deviceId != describe.device_id) {
        log("test: *** The unicast device id is incorrect!\n");
        exit(1);
    }

    if (unpacked->notificationLength != describe.count) {
        log("test: *** The unicast count is incorrect!\n");
        exit(1);
    }

    if (strncmp((char *) unpacked->notification, (char *) describe.data, describe.count) != 0) {
        log("test: *** The unicast body is incorrect!\n");
        exit(1);
    }

    /*
     *  Create a multiple-entry keep-alive output queue and delete it.
     *  valgrind will complain if we miss any free() operations.
     */
    for (i = 0; i < 3; i++) {
        send_udp_ping(&fake_client->keep, "test");
    }

    clear_keep_output(&fake_client->keep);

    free_packet(&packet);
    free_unpacked(&unpacked);

    /*
     *  Now check a multicast.
     */
    describe.type = Send_multicast;
    packet = pack(fake_client, &describe);

    prep_packet(fake_client, packet, &fake_client->out_sequence);

    unpacked = unpack(fake_client, packet, null);

    if (unpacked->userId != describe.user_id) {
        log("test: *** The multicast user id is incorrect!\n");
        exit(1);
    }

    if (unpacked->notificationLength != describe.count) {
        log("test: *** The multicast count is incorrect!\n");
        exit(1);
    }

    if (strncmp((char *) unpacked->notification, (char *) describe.data, describe.count) != 0) {
        log("test: *** The multicast body is incorrect!\n");
        exit(1);
    }

    free_packet(&packet);
    free_unpacked(&unpacked);

    free_crypto(&fake_client->crypto);

    /*
     *  send_subscriptions should be okay with a malloc failure.
     */
    fake_client->subscriptions = devices;
    fake_client->subscription_count = 3;
    force_malloc_error = 1;

    send_subscriptions(fake_client);

    if (force_malloc_error != 0) {
        log("test: *** send_subscriptions didn't trigger a malloc error\n");
        exit(1);
    }

    /*
     *  Create some packets for discard_tcp_queue to free.
     */
    for (int i = 0; i < 3; i++) {
        clear_describe(&describe);
        describe.type  = Send_ping;

        packet = pack(fake_client, &describe);

        if (packet == null) {
            log("test: *** pack(Send_ping) failed.\n");
            exit(1);
        }

        queue_packet(fake_client, packet, "test");

        if (fake_client->tcp_tail != packet) {
            log("test: *** queue_packet failed.\n");
            exit(1);
        }
    }

    fake_client->is_foreground = false;

    if (ans_setForeground(fake_client, false, 10000)) {
        log("test: *** ans_setForeground should detect a null state change.\n");
        exit(1);
    }

    fake_client->is_foreground = true;

    if (ans_setForeground(fake_client, true, 10000)) {
        log("test: *** ans_setForeground should detect a null state change.\n");
        exit(1);
    }

    fake_client->stop_now = true;

    /*
     *  valgrind will tell us whether this worked.
     */
    discard_tcp_queue(fake_client);

    /*
     *  Do some test of the I/O error handling.  Create a crypto
     *  structure and then build a packet to send.
     */
    fake_client->crypto   = create_crypto(key, sizeof(key));
    fake_client->stop_now = false;

    make_event(&fake_client->event);

    describe.type = Send_ping;
    packet   = pack(fake_client, &describe);
    sequence = 0;
    prep_packet(fake_client, packet, &sequence);

    /*
     *  Create an event structure so that we can steal its sockets.
     */
    memset(&event, 0, sizeof(event));
    init_event(&event);
    mutex_init(&event.mutex);

    for (i = 1; i <= 3; i++) {
        make_event(&event);
        fake_client->socket = event.socket;
        count = VPLSocket_Send(event.out_socket, packet->base, packet->length);

        if (count != packet->length) {
            log("test: *** VPLSocket_Send failed.\n");
            exit(1);
        }

        fail_receive = i;

        output = read_packet(fake_client);

        if (output != null || fail_receive > 0) {
            log("test: *** read_packet didn't trigger an I/O error.");
            exit(1);
        }

        free_event(&event);
    }

    free_packet(&packet);

    /*
     *  Try setting the subscriptions.  The operation should be deferred.
     */
    ans_max_subs = 0;
    fake_client->subscriptions = null;

    if (!ans_setSubscriptions(fake_client, 0, devices, array_size(devices))) {
        log("test: *** ans_setSubscriptions failed for the defer test!\n");
        exit(1);
    }

    free(fake_client->subscriptions);
    fake_client->subscriptions = null;

    /*
     *  Start keep_one_socket with an invalid socket id.  It should handle
     *  that properly.
     */
    fake_client->keep.live_socket_id = -1;

    keep_one_socket(&fake_client->keep);

    fake_client->keep.live_socket_id = 100000;
    fake_client->is_foreground       = false;
    fake_client->interval            = 0;

    /*
     *  Keep-alive also should handle a non-matching socket id.
     */
    keep_one_socket(&fake_client->keep);

    /*
     *  Run through the random number generator to check it for valgrind
     *  problems.
     */
    mtwist_init(&fake_client->mt, 32);

    for (i = 0; i < 1000; i ++) {
        mtwist_next(&fake_client->mt, 100);
    }

    free_crypto(&fake_client->crypto);
    destroy_locks(fake_client);
    free(fake_client);
}

static void
test_deprep()
{
    ans_client_t  client;
    packet_t      packets[10];
    int           i;

    memset(packets, 0, sizeof(packets));
    memset(&client, 0, sizeof(client));

    for (i = 0; i < array_size(packets); i++) {
        packets[i].prepared = true;

        if (i < array_size(packets) - 1) {
            packets[i].next = &packets[i + 1];
        } else {
            packets[i].next = null;
        }
    }

    packets[2].prepared = true; /* mark one as unprepared for testing */

    client.tcp_head = &packets[0];
    client.tcp_tail = &packets[array_size(packets) - 1];

    deprep_tcp_queue(&client);

    for (i = 0; i < array_size(packets); i++) {
        if (packets[i].prepared) {
            log("test: *** deprep_tcp_queue failed.\n");
            exit(1);
        }
    }
}

static void
test_encryption(ans_client_t *client)
{
    int     success;
    char *  ciphertext;
    int     cipher_length;
    char *  plaintext;
    int     plain_length;
    char    iv[aes_block_size];
    int     j;
    u8      message[2 * aes_block_size];
    short   length;
    int     error;

    log("test: === Running the encryption tests.\n");

    for (int i = 0; i < array_size(test_strings); i++) {
        /*
         *  Make a new IV for each test case.
         */
        for (j = 0; j < array_size(iv); j++) {
            iv[j] = i * 16 + j + 31;
        }

        /*
         *  Test some invalid lengths.
         */
        success = ans_encrypt(client, &ciphertext, &cipher_length,
                        iv, test_strings[i], 0);

        if (success) {
            log("test: *** ans_encrypt worked with a zero length\n");
            exit(1);
        }

        success = ans_encrypt(client, &ciphertext, &cipher_length,
                        iv, test_strings[i], ans_max_encrypt + 1);

        if (success) {
            log("test: *** ans_encrypt worked with bad length\n");
            exit(1);
        }

        /*
         *  The first time through, check how well ans_encrypt handles
         *  malloc failures and encryption failures.
         */
        if (i == 0) {
            for (j = 1; j <= 2; j++) {
                force_malloc_error = j;
                ciphertext    = &iv[0];
                cipher_length = 20;

                success = ans_encrypt(client, &ciphertext, &cipher_length,
                                iv, test_strings[i], strlen(test_strings[i]) + 1);

                if (force_malloc_error > 0) {
                    log("test: *** ans_encrypt didn't trigger a malloc error\n");
                    exit(1);
                }

                if (success) {
                    log("test: *** ans_encrypt didn't fail on a malloc error\n");
                    exit(1);
                }

                if (ciphertext != null || cipher_length != 0) {
                    log("test: *** ans_encrypt didn't clear its outputs\n");
                    exit(1);
                }
            }

            fail_encrypt = true;

            success = ans_encrypt(client, &ciphertext, &cipher_length,
                            iv, test_strings[i], strlen(test_strings[i]) + 1);

            if (success) {
                log("test: *** fail_encrypt didn't work\n");
                exit(1);
            }

            if (ciphertext != null || cipher_length != 0) {
                log("test: *** ans_encrypt didn't clear its outputs\n");
                exit(1);
            }
        }

        /*
         *  Okay, now try a real encryption.
         */
        success = ans_encrypt(client, &ciphertext, &cipher_length,
                        iv, test_strings[i], strlen(test_strings[i]) + 1);

        if (!success) {
            log("test: *** ans_encrypt failed on round %d\n", i);
            exit(1);
        }

        if (cipher_length == strlen(test_strings[i])) {
            log("test: *** ans_encrypt produced a strange size on round %d\n", i);
            exit(1);
        }

        /*
         *  On the first round, check that ans_decrypt can handle malloc
         *  failures and decryption failures.
         */
        if (i == 0) {
            for (j = 1; j <= 2; j++) {
                force_malloc_error = j;
                plaintext     = &iv[0];
                plain_length  = 20;

                success = ans_decrypt(client, &plaintext, &plain_length,
                                ciphertext, cipher_length);

                if (force_malloc_error > 0) {
                    log("test: *** ans_decrypt didn't trigger a malloc error\n");
                    exit(1);
                }

                if (success) {
                    log("test: *** ans_decrypt didn't fail on a malloc error\n");
                    exit(1);
                }

                if (plaintext != null || plain_length != 0) {
                    log("test: *** ans_decrypt didn't clear its outputs\n");
                    exit(1);
                }
            }

            fail_decrypt = true;

            success = ans_decrypt(client, &plaintext, &plain_length,
                            ciphertext, cipher_length);

            if (success) {
                log("test: *** force_crypt didn't work\n");
                exit(1);
            }

            if (plaintext != null || plain_length != 0) {
                log("test: *** ans_decrypt didn't clear its outputs\n");
                exit(1);
            }
        }

        /*
         *  We should be able to decrypt what we encypted.
         */
        success = ans_decrypt(client, &plaintext, &plain_length,
                        ciphertext, cipher_length);

        if (!success) {
            log("test: *** ans_decrypt failed on round %d\n", i);
            exit(1);
        }

        if (plain_length != strlen(test_strings[i]) + 1) {
            log("test: *** ans_decrypt returned the wrong size on round %d\n", i);
            exit(1);
        }

        if (strcmp(plaintext, test_strings[i]) != 0) {
            log("test: *** The output didn't match on round %d\n", i);
            exit(1);
        }

        free(plaintext);
        plaintext = null;

        /*
         *  Try another invalid size test.
         */
        success = ans_decrypt(client, &plaintext, &plain_length,
                        ciphertext, cipher_length - 1);

        if (success) {
            log("test: *** ans_decrypt didn't detect a bad size:  round %d\n", i);
            exit(1);
        }

        free(ciphertext);
    }

    success = ans_decrypt(client, &plaintext, &plain_length,
                    iv, aes_block_size);

    if (success) {
        log("test: *** ans_decrypt didn't detect a short message\n");
        exit(1);
    }

    /*
     *  Send a message with an invalid count.
     */
    memset(message, 0, sizeof(message));

    length = ntohs(30000);
    memcpy(message + aes_block_size, &length, sizeof(length));

    error = 
        aes_SwEncrypt
        (
            (u8 *) client->key,
            message,
            message + aes_block_size,
            aes_block_size,
            message + aes_block_size
        );

    if (error) {
        log("test: *** aes_SwEncrypt failed!\n");
        exit(1);
    }

    success = ans_decrypt(client, &plaintext, &plain_length, (char *) message, sizeof(message));

    if (success) {
        log("test: *** ans_decrypt didn't detect a bad length\n");
        exit(1);
    }
}

static void
check_name(int type, const char *name)
{
    if (packet_type(type) == null || !streq(name, packet_type(type))) {
        log("test: *** The name of packet type %d is incorrect\n", type);
        exit(1);
    }
}

static void
check_net_address(uint32_t address, const char *expected)
{
    char  buffer[200];

    convert_address(VPLConv_ntoh_u32(address), buffer, sizeof(buffer));

    if (!streq(buffer, expected)) {
        log("test: *** address 0x%x didn't match %s, got %s\n",
            address, expected, buffer);
        exit(1);
    }
}

static void
check_state_string(char state, const char *expected)
{
    if (!streq(state_to_string(state), expected)) {
        log("test: *** state 0x%x didn't match %s, got %s\n",
            state, expected, state_to_string(state));
        exit(1);
    }
}

static void
cover_misc(ans_client_t *client)
{
    uint32_t  complex;

    log("test: === Starting the miscellaneous coverage test.\n");

    check_name(Set_login_version,    "Set_login_version");
    check_name(Send_state_list,      "Send_state_list");
    check_name(Set_param_version,    "Set_param_version");
    check_name(Send_device_shutdown, "Send_device_shutdown");
    check_name(Send_timed_ping,      "Send_timed_ping");

    check_name(-1, "unknown");

    check_net_address(1 << 24, "1.0.0.0");
    check_net_address(1 << 16, "0.1.0.0");
    check_net_address(1 <<  8, "0.0.1.0");
    check_net_address(1 <<  0, "0.0.0.1");
    check_net_address(      0, "0.0.0.0");
    check_net_address(     -1, "255.255.255.255");

    check_state_string(QUERY_FAILED, "query failed");
    check_state_string(-1, "unknown");

    complex = (100 << 24) + (201 << 16) + (220 << 8) + 255;
    check_net_address(complex, "100.201.220.255");

    complex = (10 << 24) + (21 << 16) + (5 << 8) + 55;
    check_net_address(complex, "10.21.5.55");

    if (!streq(response_string(1001), "unknown")) {
        log("*** bad response_string result: %s vs %s\n",
            response_string(1001), "unknown");
        exit(1);
    }

    sleep_seconds(null, 0);
}

static void
test_foreground(ans_client_t *client, int server_fd, crypto_t *crypto)
{
    int  tries;
    int  client_fd;

    log("test: === Starting the foreground-background test.\n");

    tries      = 0;
    client_fd  = start_connection(server_fd, crypto, true);

    send_packet(client, client_fd, Set_device_params, crypto, false, 0);

    /*
     *  Wait for keep-alive to start.
     */
    while
    (
        (client->keep.live_socket_id == -1 || !client->keep.keeping)
    &&  tries++ < 10
    ) {
        sleep(1);
    }

    if (client->keep.live_socket_id == -1 || !client->keep.keeping) {
        log("test: The client didn't enter the foreground state\n");
        exit(1);
    }

    /*
     *  Move to background mode.
     */
    ans_setForeground(client, false, (long) 1 * 1000 * 1000 * 1000);
    tries = 0;

    while (client->keep.keeping && tries++ < 10) {
        sleep(1);
    }

    if (client->keep.keeping) {
        log("test: The client didn't enter the background state\n");
        exit(1);
    }

    /*
     *  Try to move back to the foreground state.
     */
    ans_setForeground(client, true, 0);

    tries = 0;

    while (!client->keep.keeping && tries++ < 10) {
        sleep(1);
    }

    if (!client->keep.keeping) {
        log("test: The client didn't enter the foreground state.\n");
        exit(1);
    }

    tag = 0;
}

static void
test_network_change(ans_client_t *client, int server_fd, crypto_t *crypto)
{
    int  tries;
    int  client_fd;
    int  start_down_count;

    log("test: === Starting the ans_declareNetworkChange test.\n");

    ans_declareNetworkChange(client, true);

    tries      = 0;
    client_fd  = start_connection(server_fd, crypto, true);

    send_packet(client, client_fd, Set_device_params, crypto, false, 0);

    /* 
     *  Wait for the connection to hit the live state.
     */
    while
    (
        (client->keep.live_socket_id == -1 || !client->keep.keeping)
    &&  tries++ < 10
    ) {
        sleep(1);
    }

    if (client->keep.live_socket_id == -1 || !client->keep.keeping) {
        log("test: The client connection didn't start.\n");
        exit(1);
    }

    start_down_count  = down_count;
    ans_forced_closes  = 0;

    ans_declareNetworkChange(client, true);
    log("test: declared the network down\n");

    /*
     *  Now wait for the connection to drop.
     */
    tries = 0;

    while (down_count == start_down_count && tries++ < 5) {
        sleep(1);
    }

    if (down_count == start_down_count) {
        log("test: The client ignored the network change.\n");
        exit(1);
    }

    sleep(1);

    if (ans_forced_closes != 1) {
        log("test: The forced drop count is wrong (%d).\n", ans_forced_closes);
        exit(1);
    }

    tag = 0;
    log("test: The ans_declareNetworkChange test passed.\n");
}

static int64_t array1[] = {  1,  2,  3,  4,  5, 16     };
static int64_t array2[] = {  6,  2,  9,  8,  5, 16     };
static int64_t array3[] = {  6,  5,  4,  3,  2,  1     };
static int64_t array4[] = {  4,  8, 16, 32, 31, 37, 51 };
static int64_t array5[] = { 51, 37, 32, 31, 16,  4,  8 };

static void
check_array(const char *title, int64_t *data, int count)
{
    for (int i = 0; i < count - 1; i++) {
        if (data[i] > data[i + 1]) {
            log("*** The sort for %s failed at %d\n", title, i);
            exit(1);
        }
    }
}

/*
 *  Test the sorting and media code a bit.
 *  TODO:  save the correct answers and check the results.
 */
static void
test_sort(int64_t *data, int count, const char *title)
{
    int64_t  median;
    int64_t  mad;

    log("Test %s\n", title);

    median = get_median(data, count);
    check_array("sorted", data, count);

    mad  = get_mad(data, count, median);
    check_array("sorted", data, count);
}

static void
test_get_median(void)
{
    test_sort(array1, array_size(array1), "array1");
    test_sort(array2, array_size(array2), "array2");
    test_sort(array3, array_size(array3), "array3");
    test_sort(array4, array_size(array4), "array4");
    test_sort(array5, array_size(array5), "array5");
}

int
main(int argc, char *argv[], char *env[])
{
    ans_client_t *     client;
    ans_callbacks_t    callbacks;
    const char *       hostname = "localhost";
    struct hostent *   address;
    struct sockaddr_in server_address;
    struct sockaddr    client_address;
    ans_open_t         input;
    crypto_t *         crypto;
    describe_t         describe;

    int      server_fd;
    int      error;
    int      server_length;
    int      client_fd;
    int      connections;
    int      total_pings;
    int      loops;
    int      print_count;
    int      success;
    int      shutdowns;
    int16_t  partial;
    int      round;
    int      i;
    int      result;
    int      expected;
    int32_t  ans_tcp_port;

    setlinebuf(stdout);
    log("test: === I am starting the device client test.\n");

    ans_tcp_port        = 2048;
    ans_print_interval  =    1;
    ans_base_interval   =   20;
    ans_retry_interval  =   20;
    ans_retry_count     =    2;
    total_pings         =    0;
    ans_partial_timeout =    2;

    memset(blob, 0, sizeof(blob));
    memset(key,  0, sizeof(key));

    crypto  = create_crypto(key, sizeof(key));
    address = gethostbyname(hostname);

    if (address == null) {
        perror("gethostbyname");
        log("test: *** No host\n");
        exit(1);
    }

    /*
     *  Run some small tests.
     */
    test_hex_dump();
    test_get_median();
    test_event();
    test_pack_unpack();

    /*
     *  Open a server socket to emulate an ANS server.
     */
    server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_fd < 0) {
        perror("socket");
        log("test: *** No socket\n");
        exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    memcpy(&server_address.sin_addr, address->h_addr, address->h_length);
    server_address.sin_family = AF_INET;
    server_length = sizeof(server_address);

    /*
     *  Try a few different ports to get one that we can use.
     */
    loops = 30;

    do {
        server_address.sin_port = htons(ans_tcp_port);
        error = bind(server_fd, (struct sockaddr *) &server_address, server_length);

        if (error) {
            ans_tcp_port++;
        }
    } while (error && loops-- > 0);

    if (error) {
        perror("bind");
        log("test: *** bind failed\n");
        close(server_fd);
        exit(1);
    }

    error = listen(server_fd, 4);

    if (error) {
        perror("listen");
        log("test: *** listen failed\n");
        close(server_fd);
        exit(1);
    }

    print_socket_name(server_fd);

    /*
     *  Now set up structures so that we can open a connection
     *  to our fake server.
     */
    callbacks.connectionActive       = t_connectionActive;
    callbacks.receiveNotification    = t_receiveNotification;
    callbacks.receiveSleepInfo       = t_receiveSleepInfo;
    callbacks.receiveDeviceState     = t_receiveDeviceState;
    callbacks.connectionDown         = t_connectionDown;
    callbacks.connectionClosed       = t_connectionClosed;
    callbacks.setPingPacket          = t_set_ping_packet;
    callbacks.loginCompleted         = t_login_completed;
    callbacks.rejectCredentials      = t_reject_credentials;
    callbacks.rejectSubscriptions    = t_reject_subscriptions;
    callbacks.receiveResponse        = t_receive_response;

    input.clusterName  = hostname;
    input.callbacks    = &callbacks;
    input.blob         = blob;
    input.blobLength   = sizeof(blob);
    input.key          = key;
    input.keyLength    = sizeof(key);
    input.deviceType   = "default";
    input.application  = "unit test";
    input.verbose      = true;
    input.server_tcp_port = ans_tcp_port;

    cover_locks();
    test_fake();
    test_deprep();
    test_malloc_errors(&input);
    test_short_packets();
    test_run_device(server_fd, &input, crypto);
    test_sync_open (server_fd, &input, crypto);
    test_slow_login(server_fd, &input, crypto);

    ans_max_subs = 32;

    for (round = 0; round < 2; round++) {
        log("test: === Starting round %d.\n", round);

        /*
         *  Try forcing failures in ans_open.
         */
        ans_force_fail = 1;
        client = ans_open(&input);

        if (client != null) {
            log("test: *** No forced ans_open failure occurred!\n");
            close(server_fd);
            exit(1);
        }

        /*
         *  Now force thread creation failures.
         */
        for (i = 1; i <= 2 && round == 0; i++) {
            fail_thread_create = i;
            client = ans_open(&input);

            if (client != null || fail_thread_create > 0) {
                log("test: *** No forced thread creation failure occurred!\n");
                close(server_fd);
                exit(1);
            }
        }

        flush_accept_queue(server_fd);

        /*
         *  Now open a usable client.
         */
        ans_min_delay    = 2;
        ans_force_socket = 1;
        ans_force_change = round == 0;
        ans_ping_factor  = -1;
        client           = ans_open(&input);

        if (client == null) {
            log("test: *** No client\n");
            close(server_fd);
            exit(1);
        }

        if (round == 0) {
            packet_test(client);
            cover_misc(client);

            test_short_challenge(client);
            test_short_challenge(client);
            test_short_multicast(client);

            test_long_packet(client,  Send_unicast);

            test_short_state(client);
        } else {
            ans_write_limit = 4;
        }

        memset(&client_address, 0, sizeof(client_address));

        /*
         *  Run each client through three connections.
         */
        for (connections = 0; connections < 3; connections++) {
            log("test: === Starting connection %d for round %d.\n",
                connections, round);

            if (force_malloc_error > 0 || malloc_countdown > 0) {
                log("test: *** The malloc failure testing code is active!\n");
                exit(1);
            }

            login_active = false;
            client_fd    = start_connection(server_fd, crypto, true);

            print_socket_name(client_fd);
            send_packet(client, client_fd, Set_device_params, crypto, false, 0);

            /*
             *  Test sending a some bad device parameter packets, and a valid
             *  device state list.
             */
            send_packet(client, client_fd, Set_device_params, crypto, false, send_wrong_version);
            send_packet(client, client_fd, Set_device_params, crypto, false, send_short_params);
            send_packet(client, client_fd, Send_state_list,   crypto, false, 0);

            received_count = 0;
            expected = 0;

            success  = send_ping(client);
            success &= send_ping(client);
            success &= send_ping(client);
            total_pings += 3;

            if (!success) {
                log("test: *** send_ping failed\n");
                exit(1);
            }

            send_packet(client, client_fd, Send_ping, crypto, round == 0, 0);

            /*
             *  Test the packet queue.
             */
            clear_describe(&describe);
            describe.type = Send_ping;
            send_message(client, &describe);
            send_message(client, &describe);
            send_message(client, &describe);
            send_message(client, &describe);
            total_pings += 4;

            /*
             *  Okay, do some tests that depend on the login being active
             *  and the encryption being available.
             */
            if (round == 0 && connections == 0) {
                /*
                 *  If a packet has been received, we need to fake a
                 *  device configuration packet from the ANS server.
                 */
                loops = 0;

                while (client->crypto == null && loops++ < 10) {
                    sleep(1);
                }

                if (client->crypto == null) {
                    log("test: *** Client %p should have a crypto structure.\n",
                        client);
                    exit(1);
                }

                loops = 0;

                while (!login_active && loops++ < 10) {
                    sleep(1);
                }

                if (!login_active) {
                    log("test: *** The login attempt failed.\n");
                    exit(1);
                }

                if (client->generation != server_generation) {
                    log("test: *** The client generation is incorrect.\n");
                    exit(1);
                }

                test_misc(client);
                test_keep_alive(client);
            }

            read_bytes(client_fd);

            for (i = 0; i < 6; i++) {
                send_packet(client, client_fd, Send_unicast, crypto, false, 0);
                send_packet(client, client_fd, Send_multicast, crypto, false, 0);
                expected += 2;
                read_bytes(client_fd);
            }

            loops = 0;

            while (received_count != expected && loops++ < 20) {
                log("test: waiting for messages\n");
                sleep(1);
            }

            loops = 0;

            while (!login_active && loops++ < 5) {
                sleep(1);
            }

            if (!login_active) {
                log("test: *** The login didn't complete.\n");
                exit(1);
            }

            /*
             *  Okay, check that the device parameters actually were set.
             */
            if (connections == 0) {
                if
                (
                    ans_print_interval  != my_ans_print_interval
                ||  ans_max_subs        != my_ans_max_subs
                ||  ans_max_packet_size != my_ans_max_packet_size
                ||  ans_min_delay       != my_ans_min_delay
                ||  ans_max_delay       != my_ans_max_delay
                ||  ans_base_interval   != my_ans_base_interval
                ||  ans_retry_interval  != my_ans_retry_interval
                ||  ans_retry_count     != my_ans_retry_count
                ||  ans_tcp_ka_time     != my_ans_tcp_ka_time
                ||  ans_tcp_ka_interval != my_ans_tcp_ka_interval
                ||  ans_tcp_ka_probes   != my_ans_tcp_ka_probes
                ||  ans_jitter          != my_ans_jitter
                // ||  ans_keep_port       != my_ans_keep_port
                ) {
                    log("test: *** The device configuration failed on round %d.\n",
                        (int) connections);
                    close(server_fd);
                    exit(1);
                }
            } else {
                if
                (
                    ans_print_interval  != my_ans_print_interval
                ||  ans_max_subs        != my_ans_max_subs
                ||  ans_max_packet_size != my_ans_max_packet_size
                ||  ans_min_delay       != my_ans_min_delay
                ||  ans_max_delay       != my_ans_max_delay
                ) {
                    log("test: *** The device configuration failed on round %d.\n",
                        (int) connections);
                    close(server_fd);
                    exit(1);
                }
            }

            if (received_count != expected) {
                log("test: *** Failed to receive messages\n");
                close(server_fd);
                exit(1);
            }

            if (round == 0 && connections == 0) {
                test_encryption(client);
            }

            /*
             *  Test an incomplete Set_device_params packet.
             */
            if (round == 0 && connections == 0) {
                send_packet(client, client_fd, Set_device_params, crypto, false, send_short);
                sleep(2);   /* give the packet some time to be processed */
            }

            tag = 0;
            close(client_fd); /* won't work with a reader... */
            ans_onNetworkConnected(client);
        }

        ans_close(client, VPL_TRUE);
        log("test: === Finishing round %d.\n", round);
    }

    /*
     *  Test the print interval in read_loop.
     */
    log("test: === Starting the print interval test.\n");
    ans_print_interval = 1;

    input.clusterName      = hostname;
    input.callbacks        = &callbacks;
    input.blob             = blob;
    input.blobLength       = sizeof(blob);
    input.key              = key;
    input.keyLength        = sizeof(key);
    input.deviceType       = "default";
    input.application      = "unit test";
    input.verbose          = true;
    input.server_tcp_port  = ans_tcp_port;

    flush_accept_queue(server_fd);

    client    = ans_open(&input);
    client_fd = start_connection(server_fd, crypto, true);

    /*
     *  Wait for the connection setup to be done, then set
     *  the print interval to one second.
     */
    sleep(1);
    read_bytes(client_fd);
    ans_print_interval = 1;
    ping_device(client);

    /*
     *  Wait for the print interval change to be acknowledged,
     *  and then wait for some messages to occur.
     */
    sleep(1);
    print_count = ans_print_count;
    sleep(3 * ans_print_interval + 1);

    ans_print_interval = 10;

    /*
     *  Wait a bit to see whether the log message is printed.
     */
    if (print_count == ans_print_count) {
        log("test: *** No data-wait log messages were printed.\n");
        exit(1);
    }

    /*
     *  Test the shutdown code a bit.  Tell the thread to shut down,
     *  but wait to set the shutdown type.
     */
    log("test: === Starting the async run_device shutdown test.\n");
    client->stop_now = true;
    ping_device(client);

    while (client->cluster != null) {
        sleep(1);
    }

    sleep(1);
    client->cleanup = cleanup_wait;
    result = VPLDetachableThread_Join(&client->device_handle);

    if (result != VPL_OK) {
        log("test: *** The join failed.\n");
        exit(1);
    }

    free(client);
    tag = 0;

    /*
     *  Test the user-level keep-alive.
     */
    log("test: === Starting the user-level keep-alive test.\n");

    input.clusterName      = hostname;
    input.blob             = blob;
    input.blobLength       = sizeof(blob);
    input.key              = key;
    input.keyLength        = sizeof(key);
    input.deviceType       = "default";
    input.application      = "unit test";
    input.verbose          = true;
    input.server_tcp_port  = ans_tcp_port;

    flush_accept_queue(server_fd);

    client = ans_open(&input);
    start_connection(server_fd, crypto, true);

    /*
     *  We might need to wait a bit for the keep-alive thread.
     */
    loops = 0;

    while (!client->keep.keeping && loops++ < 20) {
        sleep(1);
    }

    if (!client->keep.keeping) {
        log("test: The keep-alive won't start\n");
        sleep(1);
    }

    /*
     *  See whether async shutdown works.
     */
    log("test: === Testing the asynchronous shutdown code.\n");
    shutdowns = ans_shutdowns;
    ans_close(client, VPL_FALSE);
    loops = 0;

    while (shutdowns == ans_shutdowns && loops++ < 10) {
        log("test: waiting for the shutdown to complete\n");
        sleep(1);
    }

    if (shutdowns == ans_shutdowns) {
        log("test: *** The async shutdown was too slow or failed.\n");
        exit(1);
    }

    tag = 0;

    log("test: === Testing the partial packet code.\n");

    flush_accept_queue(server_fd);

    ans_partial_timeouts   = 0;

    input.clusterName      = hostname;
    input.blob             = blob;
    input.blobLength       = sizeof(blob);
    input.key              = key;
    input.keyLength        = sizeof(key);
    input.deviceType       = "default";
    input.application      = "unit test";
    input.verbose          = true;
    input.server_tcp_port  = ans_tcp_port;

    /*
     *  First, open the connection.
     */
    flush_accept_queue(server_fd);

    client    = ans_open(&input);
    client_fd = start_connection(server_fd, crypto, true);

    /*
     *  Send the Set_device_params packet to complete the login.
     */
    send_packet(client, client_fd, Set_device_params, crypto, false, 0);

    sleep(ans_partial_timeout + 3);

    if (ans_partial_timeouts != 0) {
        log("test: *** A size read timed out with no data\n");
        exit(1);
    }

    /*
     *  Okay, send part of a packet.
     */
    log("test: writing the partial packet\n");
    partial = ntohs(~36);
    int count = write(client_fd, &partial, sizeof(partial));

    if (count != sizeof(partial)) {
        log("test: *** The partial buffer write failed.\n");
        exit(1);
    }

    sleep(ans_partial_timeout * 2);

    if (ans_partial_timeouts != 1) {
        log("test: *** A partial packet read failed to time out\n");
        exit(1);
    }

    ans_close(client, VPL_TRUE);
    tag = 0;

    log("test: === Testing the application-level keep-alive code.\n");

    ans_base_interval    =    3;
    ans_retry_interval   =    1;
    ans_retry_count      =    2;
    ans_print_interval   =   -1;

    ans_fast_timeout     =    0;
    ans_fast_packets     =    0;
    ans_timeout_events   =    0;
    ans_timeout_closes   =    0;
    ans_keep_packets_out =    0;
    ans_jitter           =  500;

    flush_accept_queue(server_fd);

    client    = ans_open(&input);
    client_fd = start_connection(server_fd, crypto, true);

    send_packet(client, client_fd, Set_device_params, crypto, false, 0);
    read_bytes(client_fd);
    sleep(1);
    ans_print_interval = 0;

    sleep(ans_retry_interval * ans_retry_count + 2);

    if
    (
        ans_fast_timeout   != 1
    ||  ans_timeout_events != 1
    ||  ans_timeout_closes != 1
    ||  ans_fast_packets     < ans_retry_count
    ||  ans_keep_packets_out < ans_retry_count + 1
    ) {
        log("test: *** Round 1:  The timeout code didn't work properly.\n");
        log("test: fast timeouts:     %2d\n", ans_fast_timeout);
        log("test: timeout events:    %2d\n", ans_timeout_events);
        log("test: timeout closes:    %2d\n", ans_timeout_closes);
        log("test: fast packets:      %2d\n", ans_fast_packets);
        log("test: keep packets out:  %2d\n", ans_keep_packets_out);
        log("test: ans_retry_count:   %2d\n", ans_retry_count);
        exit(1);
    }

    tag = 0;

    /*
     *  Okay, try another round.  Make sure that the timeout is
     *  obeyed.
     */
    log("test: === Starting the second timeout test.\n");
    ans_base_interval    =    3;
    ans_retry_interval   =    1;
    ans_retry_count      =    2;
    ans_print_interval   =   -1;

    client_fd = start_connection(server_fd, crypto, true);
    send_packet(client, client_fd, Set_device_params, crypto, false, 0);
    sleep(1);
    read_bytes(client_fd);
    ans_print_interval = 0;

    sleep(ans_retry_interval * ans_retry_count + 2);

    if
    (
        ans_fast_timeout   != 2
    ||  ans_timeout_events != 2
    ||  ans_timeout_closes != 2
    ||  ans_fast_packets     < 2 * ans_retry_count
    ||  ans_keep_packets_out < 2 * (ans_retry_count + 1)
    ) {
        log("test: *** Round 2:  The timeout code didn't work properly.\n");
        log("test: fast timeouts:     %2d\n", ans_fast_timeout);
        log("test: timeout events:    %2d\n", ans_timeout_events);
        log("test: timeout closes:    %2d\n", ans_timeout_closes);
        log("test: fast packets:      %2d\n", ans_fast_packets);
        log("test: keep packets out:  %2d\n", ans_keep_packets_out);
        log("test: ans_retry_count:   %2d\n", ans_retry_count);
        exit(1);
    }

    test_foreground(client, server_fd, crypto);
    test_network_change(client, server_fd, crypto);

    ans_close(client, true);
    close(server_fd);
    free_crypto(&crypto);

    if (ans_lock_errors != 0) {
        log("test: *** %d locking errors were reported!\n",
            (int) ans_lock_errors);
        exit(1);
    }

    tag = 0;
    log("test: Device client test passed\n");
    return 0;
}

/*
To-do:
overload the following routines:
    VPLSocket_Recv
    VPLSocket_RecvFrom
    VPLSocket_SendTo
    VPLSocket_Poll
    VPLDetachableThread_Join

unpack
    very short packet
    bad type
    short for each type > header
    notification with inaccurate length...
    packet with leftover bytes
    read a bad packet length
    fail a body read
    packet with a bad signature
    fake a crypto creation failure
*/

#undef VPLAssert_Failed

void
VPLAssert_Failed(char const* file, char const* func, int line, char const* formatMsg, ...)
{
}

static void *
my_malloc(size_t count)
{
    void *  result;

    if (--force_malloc_error == 0) {
        if (malloc_countdown > 0) {
            force_malloc_error = --malloc_countdown;
        }

        result = null;
    } else {
        result = malloc(count);
    }

    return result;
}

static int
my_aes_SwEncrypt(u8 *key, u8 *iv, u8 *text, int length, u8 *destination)
{
    int  result;

    if (fail_encrypt) {
        fail_encrypt = false;
        return -1;
    }

    result = aes_SwEncrypt(key, iv, text, length, destination);
    return result;
}

static int
my_aes_SwDecrypt(u8 *key, u8 *iv, u8 *text, int length, u8 *destination)
{
    int  result;

    if (fail_decrypt) {
        fail_decrypt = false;
        return -1;
    }

    result = aes_SwDecrypt(key, iv, text, length, destination);
    return result;
}

static int
my_VPLDetachableThread_Create(VPLDetachableThreadHandle_t* threadHandle_out,
                     VPLDetachableThread_fn_t startRoutine,
                     void* startArg,
                     const VPLThread_attr_t* attrs,
                     const char* threadName)
{
    if (--fail_thread_create != 0) {
        return VPLDetachableThread_Create(threadHandle_out, startRoutine, startArg, attrs, threadName);
    } else {
        return VPL_ERR_AGAIN;
    }
}

static int
my_VPLSocket_Recv(VPLSocket_t socket, void* buf, int len)
{
    if (--fail_receive == 0) {
        return VPL_ERR_NOTCONN;
    } else {
        return VPLSocket_Recv(socket, buf, len);
    }
}
