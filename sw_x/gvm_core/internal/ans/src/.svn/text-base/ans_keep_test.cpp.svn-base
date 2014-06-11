//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

/*

        This program implements a simple test of the ANS device
    client keep-alive code.  It implements a minimal version of the
    ANS server, and creates ANS device clients via ans_open for
    testing.  In order to save testing time, this program sets very
    low keep-alive intervals.

        ans_keep_test runs a small set of tests.  Each test runs
    one device client connection with a given pattern of UDP and
    TCP keep-alive packets from the emulated server.  For example,
    the "udp_short" test echoes the first two UDP keep-alive packets
    it gets from the ANS client, and all the TCP keep-alive packets.
    The connection thus should fail in a computable amount of time,
    which the run_test() procedure checks.

        This program allows a simple emulation of dropped or delayed
    packets by providing a skip index and a skip count.  For example,
    the udp_skip_1 test reads five packets total from the device,
    and doesn't echo the third packet.  This skip feature tests the
    ability of the keep-alive code to recover properly from lost or
    delayed packets.

*/

#undef LOG_ALWAYS
#undef LOG_ERROR
#undef LOG_WARN
#undef LOG_INFO

#define LOG_ALWAYS(x, args...)  printf(x, ## args); printf("\n");
#define LOG_ERROR(x, args...)   printf(x, ## args); printf("\n");
#define LOG_WARN(x, args...)    printf(x, ## args); printf("\n");
#define LOG_INFO(x, args...)    printf(x, ## args); printf("\n");

#define log(x, args...)                         \
            do      {                           \
                char ts_buffer[500];            \
                                                \
                printf("%s  test: " x "\n" ,    \
                    utc_ts(ts_buffer, sizeof(ts_buffer)), ## args); \
            } while (0)

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>

#undef abs

#define abs(a) (((a) > 0) ? (a) : -(a))

#define ans_test

#include "ans_device.cpp"

#define tcp_echo     0
#define tcp_short    1
#define tcp_long     2
#define tcp_skip_1   3
#define tcp_skip_2   4

#define udp_echo     5
#define udp_long     6
#define udp_short    7
#define udp_skip_1   8
#define udp_skip_2   9
#define udp_skip_3  10
#define udp_skip_4  11

/*
 *  Define the parameters passed to a keep-alive
 *  thread.
 */
typedef struct {
    int             limit;          /* the maximum number of packets to receive     */
    int             skip_index;     /* the first packet to drop                     */
    int             skip_count;     /* the number of packets to drop                */
    crypto_t *      crypto;         /* the encryption configuration for TCP packets */
} input_t;

static volatile int  got_connection   = false;
static volatile int  ans_login_done   = false;

static int       tolerance            = 2;
static char      host_id[]            = "localhost";
static char      ping_packet[]        = "ping";
static char      key[20]              = "session key";
static char      blob[120]            = "fake blob";
static char      ip_address[]         = "ip address";
static uint64_t  user_id              = 100;
static uint64_t  device_id            = 200;
static uint64_t  server_generation    =   0;
static uint64_t  connection_tag       =   0;
static int       connection_counter   =   1;
static int       async_id             =   0;
static int64_t   sequence_id          =   0;
static int       server_length;
static int *     down_flag;
static int       udp_fd               =   -1;
static int       tcp_fd               =   -1;
static int32_t   ans_tcp_port;

static struct sockaddr_in  server_address;
static struct timespec     sleep_time = { 0, 5 * 1000 * 1000 };

/*
 *  Implement the ANS callbacks.
 *
 *  When a connnection becomes active, set the flag and reset the
 *  TCP sequence number.
 */
static int
t_connectionActive(ans_client_t *client, VPLNet_addr_t address)
{
    got_connection = true;
    sequence_id    = 0;

    return 1;
}

static void
t_reject_credentials(ans_client_t *client)
{
}

/*
 *  Handle a connection going down.  The test code can set
 *  a pointer for a failure flag.
 */
static void
t_connectionDown(ans_client_t *client)
{
    got_connection = false;

    if (down_flag) {
        *down_flag = true;
    }
}

static int
t_receiveNotification(ans_client_t *client, ans_unpacked_t *unpacked)
{
    return 1;
}

static void
t_receiveSleepInfo(ans_client_t *client, ans_unpacked_t *unpacked)
{
}

static void
t_receiveDeviceState(ans_client_t *client, ans_unpacked_t *unpacked)
{
}

static void
t_connectionClosed(ans_client_t *client)
{
    got_connection = false;
}

static void
t_set_ping_packet(ans_client_t *client, char *ping_packet, int ping_length)
{
}

static void
t_login_completed(ans_client_t *client)
{
    ans_login_done = true;
    log("login completed for client %p", client);
}

static void
t_reject_subscriptions(ans_client_t *client)
{
}

static void
t_receive_response(ans_client_t *client, ans_unpacked_t *response)
{
}

static ans_callbacks_t  callbacks =
    {
        t_connectionActive,
        t_receiveNotification,
        t_receiveSleepInfo,
        t_receiveDeviceState,
        t_connectionDown,
        t_connectionClosed,
        t_set_ping_packet,
        t_reject_credentials,
        t_login_completed,
        t_reject_subscriptions,
        t_receive_response
    };

#if 0
static int
append_byte(char **basep, char *end, char value)
{
    char *  base;

    base = *basep;

    if (end - base < sizeof(value)) {
        return true;
    }

    memcpy(base, &value, sizeof(value));

    base   += sizeof(value);
    *basep  = base;
    return false;
}
#endif

static int
append_short(char **basep, char *end, uint16_t value)
{
    char *  base;

    base = *basep;

    if (end - base < sizeof(value)) {
        return true;
    }

    value = VPLConv_ntoh_u16(value);
    memcpy(base, &value, sizeof(value));

    base   += sizeof(value);
    *basep  = base;
    return false;
}

static int
append_int  (char **basep, char *end, uint32_t value)
{
    char *  base;

    base = *basep;

    if (end - base < sizeof(value)) {
        return true;
    }

    value = VPLConv_ntoh_u32(value);
    memcpy(base, &value, sizeof(value));

    base   += sizeof(value);
    *basep  = base;
    return false;
}

static int
append_long(char **basep, char *end, uint64_t value)
{
    char *  base;

    base = *basep;

    if (end - base < sizeof(value)) {
        return true;
    }

    value = VPLConv_ntoh_u64(value);
    memcpy(base, &value, sizeof(value));

    base   += sizeof(value);
    *basep  = base;
    return false;
}

#if 0
static int
append_string(char **basep, char *end, const char *string)
{
    uint16_t  length;
    uint16_t  wire_length;
    char *    base;

    base = *basep;
    length = strlen(string);

    if (end - base < sizeof(length) + length) {
        return true;
    }

    wire_length = VPLConv_ntoh_u16(length);

    memcpy(base, &wire_length, sizeof(wire_length));
    base += sizeof(wire_length);

    memcpy(base, string, length);

    base   += length;
    *basep  = base;
    return false;
}
#endif

/*
 *  Sign a packet with the session key for the ANS client.
 */
static void
sign_packet(char *packet, int req_length)
{
    CSL_ShaContext  context;

    CSL_ResetSha (&context);
    CSL_InputSha (&context, key, sizeof(key));
    CSL_InputSha (&context, (unsigned char *) packet,  req_length - signature_size);
    CSL_ResultSha(&context, (unsigned char *) packet + req_length - signature_size);
}

/*
 *  Make a packet in ANS wire format.  This routine implements mostly
 *  types that the ANS device client doesn't need to create on its
 *  own.
 */
static void
make_packet
(
    int               req_type,
    crypto_t *        crypto,
    char **           output,
    int *             output_length,
    ans_unpacked_t *  unpacked
)
{
    char *    packet;
    uint16_t  length;
    short     req_length;
    char *    base;
    char *    end;
    uint64_t  connection;
    char      challenge[16];
    int       device_params;
    int64_t   server_time;
    int       fail;
    int32_t   external_port;

    /*
     *  Compute the size of the packet.
     */
    req_length     = header_size;
    connection     = connection_counter++;
    external_port  = 666;
    device_params  = 0; /* keep gcc happy */

    if (connection_tag != 0) {
        req_length += sizeof(uint16_t) + sizeof(uint64_t);
    }

    memset(challenge, 3, sizeof(challenge));

    switch (req_type) {
    case Set_device_params:
        device_params = 25;

        req_length += 3 * sizeof(uint16_t) + device_params * sizeof(uint32_t);
        req_length += sizeof(uint16_t) + strlen(ping_packet);
        req_length += sizeof(uint16_t) + strlen(host_id    );
        req_length += sizeof(uint16_t) + strlen(ip_address );

        break;

    case Send_challenge:
        req_length += sizeof(uint16_t) + sizeof(uint64_t);
        req_length += sizeof(uint16_t) + sizeof(challenge);
        break;

    case Send_timed_ping:
        req_length += sizeof(uint16_t) + 5 * sizeof(uint64_t);
        break;

    default:
        log("*** Unimplemented test type %s (%d)",
            packet_type(req_type), (int) req_type);
        exit(1);
    }

    /*
     *  If this packet isn't a Send_challenge, it'll be signed.
     */
    if (req_type != Send_challenge) {
        req_length += signature_size;
    }

    packet = (char *) malloc(req_length);

    memset(packet, 0, req_length);
    base = packet;
    end  = packet + req_length;
    fail = false;

    if (connection_tag != 0) {
        fail |= append_short(&base, end, server_tag_flag);
    }

    fail |= append_short(&base, end, ~req_length);
    fail |= append_short(&base, end,  req_length);
    fail |= append_short(&base, end,  req_type);
    fail |= append_long (&base, end,  user_id);
    fail |= append_long (&base, end,  async_id);
    fail |= append_long (&base, end,  sequence_id);

    if (connection_tag != 0) {
        fail |= append_long (&base, end, connection_tag);
    }

    switch (req_type) {
    case Send_challenge:
        fail |= append_short(&base, end, ans_challenge_version - 1);
        fail |= append_long (&base, end, connection);
        fail |= append_short(&base, end, sizeof(challenge));

        fail = fail || end - base < sizeof(challenge);

        if (!fail) {
            memcpy(base, challenge, sizeof(challenge));
            base += sizeof(challenge);
        }

        break;

    case Set_device_params:
        fail |= append_short(&base, end, ans_device_version);
        fail |= append_short(&base, end, device_params);

        fail |= append_int  (&base, end, ans_print_interval);
        fail |= append_int  (&base, end, ans_max_subs);
        fail |= append_int  (&base, end, ans_max_packet_size);
        fail |= append_int  (&base, end, ans_max_query);
        fail |= append_int  (&base, end, ans_partial_timeout);
        fail |= append_int  (&base, end, ans_min_delay);
        fail |= append_int  (&base, end, ans_max_delay);

        fail |= append_int  (&base, end, ans_base_interval);
        fail |= append_int  (&base, end, ans_retry_interval);
        fail |= append_int  (&base, end, ans_retry_count);      // 10

        fail |= append_int  (&base, end, ans_tcp_ka_time);
        fail |= append_int  (&base, end, ans_tcp_ka_interval);
        fail |= append_int  (&base, end, ans_tcp_ka_probes);

        fail |= append_long (&base, end, device_id);            // 14

        fail |= append_int  (&base, end, ans_ping_back);        // 16
        fail |= append_int  (&base, end, ans_ping_factor);

        server_generation++;

        fail |= append_long (&base, end, server_generation);    // 18

        server_time = VPLTime_GetTime() / 1000 + 4;

        fail |= append_long (&base, end, server_time);          // 20
        fail |= append_int  (&base, end, ans_default_keep_port);// 22
        fail |= append_int  (&base, end, ans_jitter);
        fail |= append_int  (&base, end, external_port);
        fail |= append_int  (&base, end, 0);                    // 25

        /*
         *  Append the number of strings.
         */
        fail |= append_short(&base, end, 3);

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
        length = ntohs(strlen(host_id));
        memcpy(base, &length, sizeof(length));
        base += sizeof(length);

        memcpy(base, &host_id[0], strlen(host_id));
        base += strlen(host_id);

        /*
         *  The last string is an IP address.
         */
        length = ntohs(strlen(ip_address));
        memcpy(base, &length, sizeof(length));
        base += sizeof(length);

        memcpy(base, &ip_address[0], strlen(ip_address));
        base += strlen(ip_address);
        break;

    case Send_timed_ping:
        fail |= append_short(&base, end, 5);
        fail |= append_long (&base, end, unpacked->sendTime);
        fail |= append_long (&base, end, unpacked->rtt);
        fail |= append_long (&base, end, unpacked->connection);
        fail |= append_long (&base, end, 0);
        fail |= append_long (&base, end, server_generation);
        break;
    }

    /*
     *  Add the signature.
     */
    if (req_type != Send_challenge) {
        sign_packet(packet, req_length);
        base += signature_size;
    }

    if (base != end) {
        log("*** Packet creation failed for type %s (%d).",
            packet_type(req_type), (int) req_type);
        exit(1);
    }

    log("made packet %d (%s, %d, si " FMTs64 ")",
        (int) async_id,
              packet_type(req_type),
        (int) req_type,
              sequence_id);

    async_id++;

    *output = packet;
    *output_length = req_length;
}

/*
 *  Send an ANS packet of the given type to the given client.
 */
static void
send_packet
(
    int               fd,
    int               req_type,
    crypto_t *        crypto,
    ans_unpacked_t *  unpacked
)
{
    int     sent_bytes;
    int     count;
    char *  packet;
    int     length;
    char *  base;

    make_packet(req_type, crypto, &packet, &length, unpacked);

    sent_bytes = 0;
    base       = packet;

    log("sending type %s, sequence %d, type %d",
              packet_type(req_type),
        (int) sequence_id,
        (int) req_type);

    while (sent_bytes < length) {
        count = write(fd, base, length - sent_bytes);

        if (count <= 0) {
            perror("write");
            log("*** Write failed");
            exit(1);
        }

        base       += count;
        sent_bytes += count;
    }

    free(packet);

    if (req_type != Send_challenge) {
        sequence_id++;
    }
}

/*
 *  Open a TCP server socket to receive connection requests from
 *  our clients.  We save the address and port number for the
 *  UDP keep-alive socket.
 */
static int
open_server(void)
{
    struct hostent *       address;
    struct sockaddr        local_name;
    socklen_t              local_length;
    struct sockaddr_in *   ip_name;
    struct sockaddr_in6 *  ipv6_name;

    void *  name;
    int     server_fd;
    int     error;

    address = gethostbyname(host_id);

    if (address == null) {
        perror("gethostbyname");
        log("*** No host");
        exit(1);
    }

    server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_fd < 0) {
        perror("socket");
        log("*** No socket");
        exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    memcpy(&server_address.sin_addr, address->h_addr, address->h_length);
    server_address.sin_family = AF_INET;
    server_length = sizeof(server_address);

    /*
     *  Try to bind a random port.
     */
    server_address.sin_port = htons(0);

    error = bind(server_fd, (struct sockaddr *) &server_address, server_length);

    if (error) {
        perror("bind");
        log("*** bind failed");
        close(server_fd);
        exit(1);
    }

    error = listen(server_fd, 4);

    if (error) {
        perror("listen");
        log("*** listen failed");
        close(server_fd);
        exit(1);
    }

    /*
     *  Okay, we got a live socket, so get and save the address
     *  and port.
     */
    local_length = sizeof(local_name);
    name         = malloc(local_length);

    memset(name, 0, local_length);

    error = getsockname(server_fd, (struct sockaddr *) name, &local_length);

    if (error < 0) {
        perror("getsockbyname");
        close(server_fd);
        exit(1);
    }

    memcpy(&local_name, name, sizeof(local_name));

    if (local_name.sa_family == PF_INET) {
        ip_name = (struct sockaddr_in *) name;
        ans_tcp_port = ntohs(ip_name->sin_port);
        ans_default_keep_port = ans_tcp_port;

        log("using IPv4port %d", (int) ans_tcp_port);
    } else if (local_name.sa_family == PF_INET6) {
        ipv6_name = (struct sockaddr_in6 *) name;
        ans_tcp_port = ntohs(ipv6_name->sin6_port);
        ans_default_keep_port = ans_tcp_port;

        log("using IPv6 port %d", (int) ans_tcp_port);
    }

    /*
     *  The global server_address variable is used to
     *  open the UDP keep-alive socket.
     */
    server_address.sin_port = ntohs(ans_tcp_port);
    free(name);
    return server_fd;
}

static int
open_keep(void)
{
    int  keep_fd;
    int  error;

    /*
     *  Try to get a UDP socket.
     */
    keep_fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (keep_fd < 0) {
        log("*** socket() failed");
        perror("socket");
        exit(1);
    }

    /*
     *  Now bind it to the server address.
     */
    error = bind(keep_fd, (struct sockaddr *) &server_address, server_length);

    if (error) {
        log("*** bind() failed");
        perror("bind");
        exit(1);
    }

    return keep_fd;
}

/*
 *  Poll on one fd for incoming I/O, waiting up to the
 *  given time period.
 */
static int
easy_poll(int fd, timespec *sleep_time)
{
    struct pollfd  poll_data;

    poll_data.fd       = fd;
    poll_data.events   = POLLIN;
    poll_data.revents  = 0;

    ppoll(&poll_data, 1, sleep_time, null);
    return poll_data.revents != 0;
}

/*
 *  Read a file descriptor asynchronously.
 */
static void
read_async(int fd, char *buffer, int length, int tries, const char *where)
{
    int       count;
    int       remaining;
    int       flags;
    int64_t   timeout;
    limits_t  limits;
    keep_t    fake_keep;

    memset(&fake_keep, 0, sizeof(fake_keep));
    mutex_init(&fake_keep.mutex);

    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    get_limits(&fake_keep, &limits);

    remaining = length;

    if (tries <= 0) {
        timeout  = limits.tcp_limit / VPLTime_FromSec(1);
        timeout  = (timeout + 2) * (int64_t) 1000 * 1000 * 1000;
        tries    = timeout / sleep_time.tv_nsec;
    }

    log("read_async:  %d tries\n", tries);

    do {
        count = read(fd, buffer, remaining);

        if (count > 0) {
            buffer    += count;
            remaining -= count;
        } else if (count == 0 || (count < 0 && errno == EAGAIN)) {
            easy_poll(fd, &sleep_time);
        } else if (count < 0) {
            perror("read_async");
            exit(1);
        }

        if (tries > 0 && --tries == 0) {
            log("read timeout:  %s\n", where);
            exit(1);
        }
    } while (remaining > 0 && ans_timeout_events == 0);

    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

/*
 *  Check whether an ANS packet is a Send_login_blob
 *  message.
 */
static int
is_login_blob(packet_t *packet)
{
    char *  base;
    char *  end;
    int     fail;

    base = packet->base;
    end  = base + packet->length;
    fail = false;

    /*
     *  Unpack part of the header.
     */
    unpack_u16(check_length);

    if (check_length == server_tag_flag) {
        unpack_u16(next);
        check_length = next;
    }

    unpack_u16(length);

    if (fail || check_length != (uint16_t) ~length) {
        log_warn_1("The check length doesn't match the length.");
        return VPL_FALSE;
    }

    unpack_u16(type);

    return type == Send_login_blob;
}

/*
 *  Try to receive a TCP packet.  Usually, it'll be a keep-
 *  alive, but we use this routine to receive the login blob
 *  as well.  If we can read a packet, echo it back unless
 *  the test configuration says to skip the echo.
 */
static int
receive_tcp_packet(int fd, int count, input_t *input)
{
    char *      buffer;
    char        data[500];
    int         remaining;
    uint16_t    wire_length;
    uint16_t    tag;
    int         total_length;
    int         skip;
    packet_t *  packet;
    int         has_tag;

    ans_unpacked_t *  unpacked;

    /*
     *  Try to read the check length field.
     */
    read_async(fd, (char *) &wire_length, sizeof(wire_length), 0, "first");

    if (ans_timeout_events != 0) {
        return false;
    }

    has_tag = wire_length == server_tag_flag;

    if (has_tag) {
        read_async(fd, (char *) &wire_length, sizeof(wire_length), 10, "read length");
    }

    if (ans_timeout_events != 0) {
        return false;
    }

    /*
     *  Get and check the length.
     */
    total_length = ntohs(~wire_length);

    if (total_length < header_size || total_length > sizeof(data)) {
        log("*** The tcp packet size is %d", total_length);
        exit(1);
    }

    if (has_tag) {
        tag = htons(server_tag_flag);
        memcpy(data,               &tag,         sizeof(tag));
        memcpy(data + sizeof(tag), &wire_length, sizeof(wire_length));

        buffer     = data + sizeof(tag) + sizeof(wire_length);
        remaining  = total_length - (sizeof(tag) + sizeof(wire_length));
    } else {
        memcpy(data, &wire_length, sizeof(wire_length));
        buffer     = data + sizeof(wire_length);
        remaining  = total_length - sizeof(wire_length);
    }

    /*
     *  Now try to read the rest of the packet.
     */
    read_async(fd, (char *) buffer, remaining, 10, "read body");

    if (ans_timeout_events != 0) {
        return false;
    }

    packet = alloc_packet(data, total_length);

    /*
     *  Unpack the packet and check the type.  If it's not
     *  a keep-alive ping, we have a problem.
     */
    unpacked = unpack(null, packet, null);

    if (unpacked == null) {
        log("*** unpack failed");
        exit(1);
    }

    /*
     *  If we've received the login blob, just return.
     */
    if (is_login_blob(packet)) {
        connection_tag = unpacked->tag;
        free_unpacked(&unpacked);
        packet->base = null;
        free_packet(&packet);
        log("=== got the login blob");
        return true;
    }

    log("received TCP ping " FMTs64, unpacked->sequenceId);

    if (unpacked->type != Send_timed_ping) {
        log("*** Got a TCP packet of type %s (%d)",
                  packet_type(unpacked->type),
            (int) unpacked->type);
        exit(1);
    }

    /*
     *  Check to see whether we're supposed to skip the echo part.
     */
    skip = count > input->skip_index && input->skip_count-- > 0;

    if (!skip) {
        send_packet(fd, Send_timed_ping, input->crypto, unpacked);
    } else {
        log("Skipping TCP echo at count %d.", count);
    }

    packet->base = null;

    free_packet  (&packet  );
    free_unpacked(&unpacked);
    return true;
}

/*
 *  Try to read a UDP keep-alive packet from the given fd.  If we get
 *  one, echo it back unless the test configuration says we're supposed
 *  to skip the echo.
 */
static int
receive_udp_packet(int fd, int count, input_t *input)
{
    char    buffer[500];
    int     length;
    int     pass;
    int     tries;
    int     received;

    struct sockaddr  from;
    socklen_t        from_length;

    from_length = sizeof(from  );
    length      = sizeof(buffer);
    pass        = false;

    memset(&from, 0, sizeof(from));

    received = recvfrom(fd, buffer, length, MSG_DONTWAIT, &from, &from_length);

    /*
     *  If we got a packet, then check the skip configuration.  Otherwise,
     *  try to echo it back to the process.  If we didn't get a packet, just
     *  return.  If the echo fails, declare a test failure.
     */
    if (received > 0 && count >= input->skip_index && input->skip_count-- > 0) {
        log("Skipping UDP echo at count %d.", count);
        pass = true;
    } else if (received > 0) {
        tries = 10;

        do {
            pass = sendto(fd, buffer, received, 0, &from, from_length);
            pass = pass > 0;

            if (!pass && errno == EAGAIN && tries > 0) {
                easy_poll(fd, &sleep_time);
            }
        } while (!pass && tries-- > 0);

        if (!pass) {
            log("*** sendto failed");
            perror("sendto");
            exit(1);
        } else {
            log("received and echoed UDP ping at count %d", count);
        }
    } else if (received < 0 && errno != EAGAIN) {
        log("*** recvfrom failed");
        perror("recvfrom");
        exit(1);
    }

    return pass;
}

/*
 *  Start a connection to an ANS client.  We need to send a challenge packet,
 *  then discard the login blob, and finally, configure the device.  The
 *  device configuration is required to inform the ANS client that the login
 *  was accepted.
 */
static int
start_connection(int server_fd, ans_client_t *client, crypto_t *crypto)
{
    struct sockaddr  client_address;

    socklen_t  client_length;
    int        fd;
    int        tries;
    int        done;

    memset(&client_address, 0, sizeof(client_address));
    client_length = 0;

    do {
        fd = accept(server_fd, &client_address, &client_length);

        if (fd < 0) {
            perror("accept");
            log("*** No client accepted");
            close(server_fd);
            exit(1);
        }

        sequence_id = 0;
        send_packet(fd, Send_challenge, crypto, null);
        tries = 0;

        while (!got_connection && tries++ < 10) {
            sleep(1);
        }

        if (got_connection) {
            tries   = 0;

            do {
                done = receive_tcp_packet(fd, 0, null);  // need to parse the login packet...

                if (!done) {
                    sleep(1);
                }
            } while (!done && tries++ < 10);

            if (!done) {
                log("*** I didn't receive a login blob");
                exit(1);
            }

            send_packet(fd, Set_device_params, crypto, null);
        }
    } while (!got_connection);

    return fd;
}

static void *
run_tcp(void *opaque)
{
    input_t *  input;
    int        count;
    int        received;

    input = (input_t *) opaque;
    count = 0;

    while (tcp_fd < 0) {
        nanosleep(&sleep_time, null);
    }

    while (count < input->limit && ans_timeout_events == 0) {
        received = receive_tcp_packet(tcp_fd, count, input);

        if (received) {
            count++;
        } else {
            easy_poll(tcp_fd, &sleep_time);
        }
    }

    log("run_tcp sent %d packets of %d", count, (int) input->limit);
    free(input);
    return null;
}

/*
 *  Run a UDP I/O thread.  We use the configuration to
 *  determine how many packets to receive.  The receive
 *  function checks which ones not to echo back to the
 *  ANS client.
 */
static void *
run_udp(void *opaque)
{
    input_t *  input;
    int        pass;
    int        count;

    input = (input_t *) opaque;
    count = 0;

    while (count < input->limit && ans_timeout_events == 0) {
        pass = receive_udp_packet(udp_fd, count, input);

        if (pass) {
            count++;
        } else {
            easy_poll(udp_fd, &sleep_time);
        }
    }

    log("run_udp sent %d packets of %d", count, (int) input->limit);
    free(input);
    return null;
}

/*
 *  Given a thread type, make a configuration structure for it
 *  and start a thread.
 */
static pthread_t
start(int test_type, input_t *format, int *timeout)
{
    void * (*func)(void *);

    input_t *  input;
    int        error;
    pthread_t  handle;
    int        udp_retry;
    int        udp_timeout;
    int        tcp_limit;
    int        tcp_cover;
    int        tcp_timeout;
    limits_t   limits;
    keep_t     fake_keep;

    memset(&fake_keep, 0, sizeof(fake_keep));
    mutex_init(&fake_keep.mutex);
    get_limits(&fake_keep, &limits);

    udp_retry   = limits.udp_limit / VPLTime_FromSec(1);
    tcp_limit   = limits.tcp_limit / VPLTime_FromSec(1);

    input       = (input_t *) malloc(sizeof(*input));
    *input      = *format;

    switch (test_type) {
    case tcp_echo:
        func          = run_tcp;
        input->limit  = 1000;
        tcp_cover     = (input->limit - 1) * ans_base_interval * ans_ping_factor;
        tcp_timeout   = tcp_cover + tcp_limit;
        *timeout      = tcp_timeout;
        break;

    case tcp_short:
        func          = run_tcp;
        input->limit  = 2;
        tcp_cover     = (input->limit - 1) * ans_base_interval * ans_ping_factor;
        tcp_timeout   = tcp_cover + tcp_limit;
        *timeout      = tcp_timeout;
        break;

    case tcp_long:
        func          = run_tcp;
        input->limit  = 6;
        tcp_cover     = (input->limit - 1) * ans_base_interval * ans_ping_factor;
        tcp_timeout   = tcp_cover + tcp_limit;
        *timeout      = tcp_timeout;
        break;

    case tcp_skip_1:
    case tcp_skip_2:
        func                = run_tcp;
        input->skip_index   = 2;
        input->skip_count   = test_type - tcp_skip_1 + 1;
        input->limit        = 6 + input->skip_count;
        tcp_cover           = (input->limit - 1) * ans_base_interval * ans_ping_factor;
        tcp_timeout         = tcp_cover + tcp_limit;
        *timeout            = tcp_timeout;
        break;

    case udp_echo:
        func          = run_udp;
        input->limit  = 1000;
        udp_timeout   = input->limit * ans_base_interval + udp_retry;
        *timeout      = udp_timeout;
        break;

    case udp_long:
        func          = run_udp;
        input->limit  = 6;
        udp_timeout   = input->limit * ans_base_interval + udp_retry;
        *timeout      = udp_timeout;
        break;

    case udp_short:
        func          = run_udp;
        input->limit  = 2;
        udp_timeout   = input->limit * ans_base_interval + udp_retry;
        *timeout      = udp_timeout;
        break;

    case udp_skip_1:
    case udp_skip_2:
    case udp_skip_3:
        func                = run_udp;
        input->skip_index   = 2;
        input->skip_count   = test_type - udp_skip_1 + 1;
        input->limit        = 4 + input->skip_count;
        udp_timeout         = (input->limit - input->skip_count) * ans_base_interval;
        udp_timeout        += (input->skip_count - 1) * ans_retry_interval + udp_retry;
        *timeout            = udp_timeout;
        break;

    case udp_skip_4:
        func                = run_udp;
        input->skip_index   = 2;
        input->skip_count   = test_type - udp_skip_1 + 1;
        input->limit        = 4 + input->skip_count;
        udp_timeout         = (input->skip_index * ans_base_interval) + udp_retry;
        *timeout            = udp_timeout;
        break;

    default:
        log("*** No such test type");
        exit(1);
    }

    error = pthread_create(&handle, null, func, input);

    if (error != 0) {
        perror("pthread_create");
        exit(1);
    }

    return handle;
}

static void
flush_udp(int fd)
{
    char  buffer[2048];
    int   length;

    struct sockaddr  from;
    socklen_t        from_length;

    while (easy_poll(fd, &sleep_time)) {
        memset(&from, 0, sizeof(from));

        from_length = sizeof(from);
        length      = sizeof(buffer);

        length = recvfrom(fd, buffer, length, MSG_DONTWAIT, &from, &from_length);
    }
}

#define clock_res()  ((int64_t) 1000 * 1000)

int64_t
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
run_test(const char *name, int tcp_type, int udp_type, int server_fd, int keep_fd)
{
    pthread_t       tcp_handle;
    pthread_t       udp_handle;
    ans_open_t      open;
    ans_client_t *  client;
    crypto_t *      crypto;
    int             done;
    int             error;
    input_t         format;
    int64_t         start_time;
    int64_t         total_time;
    int             tcp_timeout;
    int             udp_timeout;
    int             expected_time;
    const char *    where;

    log("=== Starting test:  %s (%d, %d)", name, udp_type, tcp_type);

    ans_timeout_events = 0;

    crypto     = create_crypto(key, sizeof(key));
    done       = false;
    down_flag  = &done;
    tcp_fd     = -1;
    udp_fd     = keep_fd;

    ans_max_error = 0;  /* reset the maximum poll wait error */

    memset(&format, 0, sizeof(format));
    memset(&open,   0, sizeof(open  ));

    format.crypto = crypto;

    /*
     *  Flush any old packets from the UDP keep-alive fd.
     */
    flush_udp(keep_fd);

    /*
     *  Start the keep-alive I/O threads so that they're reading
     *  and running.
     */
    tcp_handle = start(tcp_type, &format, &tcp_timeout);
    udp_handle = start(udp_type, &format, &udp_timeout);

    sleep(1);   // Hopefully, this is enough.

    /*
     *  Okay, start the ANS client and make the
     *  connection.
     */
    ans_login_done   = false;

    open.clusterName  = host_id;
    open.callbacks    = &callbacks;
    open.blob         = blob;
    open.blobLength   = sizeof(blob);
    open.key          = key;
    open.keyLength    = sizeof(key);
    open.deviceType   = "default";
    open.application  = "unit test";
    open.verbose      = true;
    open.server_tcp_port = ans_tcp_port;

    client = ans_open(&open);
    tcp_fd = start_connection(server_fd, client, crypto);
    log("=== connection made");

    /*
     *  Okay, get the start time!  Hopefully, it'll be close
     *  enough.
     */
    while (!ans_login_done) {
        nanosleep(&sleep_time, null);
    }

    log("=== ANS login completed");

    start_time     = get_monotonic();
    expected_time  = min(tcp_timeout, udp_timeout);
    where          = tcp_timeout < udp_timeout ? "TCP" : "UDP";

    /*
     *  Wait for the reader threads to finish.
     */
    error  = pthread_join(tcp_handle, null);
    error |= pthread_join(udp_handle, null);

    if (error) {
        log("*** pthread_join failed");
        exit(1);
    }

    /*
     *  We might need to wait a bit longer for the client to
     *  get to the timeout.
     */
    log("Wait for the connection failure");

    while
    (
        ans_timeout_events == 0
    &&  ((get_monotonic() - start_time) / clock_res()) < expected_time + 8
    ) {
        nanosleep(&sleep_time, null);
    }

    /*
     *  Okay, compute the time that the test required.
     */
    total_time = (get_monotonic() - start_time) / clock_res();

    log("=== Finished test:  %s (%d, %d), took %d seconds, expected %d seconds",
              name,
              udp_type,
              tcp_type,
        (int) total_time,
              expected_time);

    /*
     *  Shut down the ANS client and the TCP fd.
     */
    down_flag = null;
    ans_close(client, true);
    free_crypto(&crypto);
    close(tcp_fd);

    /*
     *  Now check the time required for the test.
     */
    if (abs(total_time - expected_time) > tolerance) {
        log("*** The test was expected to take %d seconds, but took %d.",
            (int) expected_time, (int) total_time);
        log("*** The maximum poll error is " FMTs64, ans_max_error);
        exit(1);
    }

    /*
     *  Also check the timeout type.
     */
    if (strcmp(ans_timeout_where, where) != 0) {
        log("*** The test was expected to cause a %s timeout, but "
            "got a %s timeout.",
            where,
            ans_timeout_where);
        exit(1);
    }

    tcp_fd = -1;
}

int
main(int argc, char **argv)
{
    int  server_fd;
    int  keep_fd;
    int  no_valgrind;

    ans_base_interval   = 4;
    ans_retry_count     = 3;
    ans_retry_interval  = 2;
    ans_ping_factor     = 1;
    ans_jitter          = 0;

    no_valgrind =
        argc == 2 && strcmp(argv[1], "--no-valgrind") == 0;

    if (no_valgrind) {
        tolerance = 1;
        log("=== Setting the tolerance to %d second%s.",
            (int) tolerance, (tolerance != 1) ? "s" : "");
    }

    /* tcp limit = (4 + 2 * 2) * 2 = 16 */
    /* udp retry = 4 + (2 * 2) = 8 */

    server_fd  = open_server();
    keep_fd    = open_keep();

    run_test("short tcp",   tcp_short,   udp_echo,    server_fd, keep_fd);
    run_test("long tcp",    tcp_long,    udp_echo,    server_fd, keep_fd);
    run_test("skip 1 tcp",  tcp_skip_1,  udp_echo,    server_fd, keep_fd);
    run_test("skip 2 tcp",  tcp_skip_2,  udp_echo,    server_fd, keep_fd);

    run_test("short udp",   tcp_echo,    udp_short,   server_fd, keep_fd);
    run_test("long udp",    tcp_echo,    udp_long,    server_fd, keep_fd);
    run_test("skip 1 udp",  tcp_echo,    udp_skip_1,  server_fd, keep_fd);
    run_test("skip 2 udp",  tcp_echo,    udp_skip_2,  server_fd, keep_fd);
    run_test("skip 3 udp",  tcp_echo,    udp_skip_3,  server_fd, keep_fd);
    run_test("skip 4 udp",  tcp_echo,    udp_skip_4,  server_fd, keep_fd);

    return 0;
}
