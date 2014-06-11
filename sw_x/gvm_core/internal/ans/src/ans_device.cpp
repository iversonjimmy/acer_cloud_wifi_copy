//
//  Copyright 2011-2013 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

/*
 *  See the wiki page for the ANS messaging system for an understanding of
 *  what this module does.
 */
#include "ans_device.h"

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
#ifndef IOS
#include <malloc.h>
#endif
#include <string.h>
#include <stdio.h>

#ifdef ans_test
#include <ctype.h>
#endif

#define ANS_WRITE_TO_STDOUT 0

#undef  null
#define null 0

#define ans_device_version      2
#define ans_login_version       2
#define ans_challenge_version   2
#define server_flag_new_ping    1

#define server_tag_flag         ((uint16_t) -1)

#define Send_subscriptions      33
#define Request_sleep_setup     54
#define Send_sleep_setup        53
#define Send_challenge          27
#define Send_ping               35
#define Send_timed_ping         12
#define Send_login_blob         26
#define Send_device_shutdown    67
#define Set_device_params       37
#define Request_wakeup          34
#define Query_device_list       39
#define Reject_credentials      57
#define Set_login_version       62
#define Set_param_version       63
#define Send_state_list          1
#define Send_response            9
#define Send_device_update      SEND_DEVICE_UPDATE

#define ANS_DEFAULT_SERVER_TCP_PORT     443

#define Send_unicast             6
#define Send_multicast           7

#define open_magic        0x30211aff
#define header_size       30
#define sequence_offset   22
#define signature_size    CSL_SHA1_DIGESTSIZE
#define state_size         9
#define cleanup_not_set    0
#define cleanup_async      1
#define cleanup_wait       2
#define max_device_state  30  /* from AnsCommon.java */
#define date_format       "%04d/%02d/%02d %02d:%02d:%02d.%03d UTC"

typedef struct mtwist_s mtwist_t;

static void     mtwist_init(mtwist_t *, uint32_t);
static int32_t  mtwist_next(mtwist_t *, int32_t);

#undef  abs
#undef  min
#undef  max

#define abs(x)         ((x) >= 0 ? (x) : -(x))
#define min(a, b)      ((a) < (b) ? (a) : (b))
#define max(a, b)      ((a) > (b) ? (a) : (b))
#define array_size(x)  (sizeof(x) / sizeof((x)[0]))
#define invalid(x)     (VPLSocket_Equal((x), VPLSOCKET_INVALID))
#define valid(x)       (!invalid(x))

/*
 *  Define some macros to unpack values from a packet.  Integer
 *  values are sent in network byte order.
 */
#define unpack_u8(x)                                \
        uint8_t x;                                  \
                                                    \
        do {                                        \
            fail |= end - base < sizeof(x);         \
                                                    \
            if (!fail) {                            \
                memcpy(&x, base, sizeof(x));        \
                base += sizeof(x);                  \
            } else {                                \
                x = 0;                              \
            }                                       \
        } while (0)

#define unpack_u16(x)  unpack_u(uint16_t, VPLConv_ntoh_u16, x)
#define unpack_u32(x)  unpack_u(uint32_t, VPLConv_ntoh_u32, x)
#define unpack_u64(x)  unpack_u(uint64_t, VPLConv_ntoh_u64, x)

#define unpack_u(t, h, x)                           \
        t x;                                        \
                                                    \
        do {                                        \
            fail |= end - base < sizeof(x);         \
                                                    \
            if (!fail) {                            \
                memcpy(&x, base, sizeof(x));        \
                x = h(x);                           \
                base += sizeof(x);                  \
            } else {                                \
                x = 0;                              \
            }                                       \
        } while (0)

#define s(x)    s_s(x)
#define s_s(x)  #x

/*
 *  Unpack a 64-bit parameter from a packet.  This macro sets
 *  the parameter value and logs the change.
 */
#define unpack_param_64(x)                              \
        do {                                            \
            param_upper = -1;                           \
            param_lower = -1;                           \
                                                        \
            unpack_param(param_upper);                  \
            unpack_param(param_lower);                  \
                                                        \
            client->x =                                 \
                (((uint64_t) param_upper) << 32) |      \
                    param_lower;                        \
                                                        \
            log_info("set %-20s to " FMTu64,            \
                s(x), client->x);                       \
        } while (0)

/*
 *  Unpack a 32-bit parameter from a packet.  This macro sets
 *  the parameter and logs the change.
 */
#define unpack_param(x)                                 \
        do {                                            \
            int32_t value;                              \
                                                        \
            memcpy(&value, base, sizeof(value));        \
            value = VPLConv_ntoh_u32(value);            \
                                                        \
            if (value != -1) {                          \
                (x) = value;                            \
                log_info("set %-20s to %4d",            \
                        s(x), (int) value);             \
            }                                           \
                                                        \
            base += sizeof(value);                      \
        } while (0)

static char *utc_ts(char *, int);

/*
 *  Define some logging macros.  They prepend a useful timestamp,
 *  since the one from the logging routines contains the local
 *  time, but no time zone.
 */
#define log_info(format, ...)                                                         \
            do {                                                                      \
                char log_buffer[255];                                                 \
                                                                                      \
                LOG_INFO("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                   \
            } while (0)

#define log_warn(format, ...)                                                         \
            do {                                                                      \
                char log_buffer[255];                                                 \
                                                                                      \
                LOG_WARN("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                   \
            } while (0)

#define log_error(format, ...)                                                         \
            do {                                                                       \
                char log_buffer[255];                                                  \
                                                                                       \
                LOG_ERROR("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                    \
            } while (0)

#define log_always(format, ...)                                                         \
            do {                                                                        \
                char log_buffer[255];                                                   \
                                                                                        \
                LOG_ALWAYS("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                     \
            } while (0)

#define log_info_1(format)                                                             \
            do {                                                                       \
                char log_buffer[255];                                                  \
                                                                                       \
                LOG_INFO("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer))); \
            } while (0)

#define log_warn_1(format)                                                             \
            do {                                                                       \
                char log_buffer[255];                                                  \
                                                                                       \
                LOG_WARN("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer))); \
            } while (0)

#define log_error_1(format)                                                             \
            do {                                                                        \
                char log_buffer[255];                                                   \
                                                                                        \
                LOG_ERROR("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer))); \
            } while (0)

#define log_always_1(format, ...)                                                         \
            do {                                                                          \
                char log_buffer[255];                                                     \
                                                                                          \
                LOG_ALWAYS("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer)));  \
            } while (0)

#ifdef ans_test
#define log_test(format, ...)                                                           \
            do {                                                                        \
                char log_buffer[255];                                                   \
                                                                                        \
                LOG_ALWAYS("%s  ans:  " format, utc_ts(log_buffer, sizeof(log_buffer)), \
                    ##__VA_ARGS__);                                                     \
            } while (0)
#else
#define log_test(format, ...)
#endif

static const char *op_results[] =
        {
            "0",
            "online",
            "sleeping",
            "offline",
            "query failed",
            "successful",
            "op failed",
            "user unavailable",
            "device unavailable",
            "timed out",
            "send failed",
            "device started",
            "invalid device id given",
            "operation rejected"
        };

static int32_t  ans_default_keep_port  =    80;
static int32_t  ans_jitter             =   200;   /* milliseconds */

static int      event_min_delay      =  1;
static int      event_max_delay      = 10;

/*
 *  These parameters can be set by the server.
 */
static int32_t           ans_max_subs         =    0;        // not yet known...
static int32_t           ans_max_query        =    0;
static int32_t           ans_max_packet_size  = 8192;
static int32_t           ans_partial_timeout  =   20;
static int32_t           ans_min_delay        =    4;
static int32_t           ans_max_delay        =   10 * 60;   // seconds

static volatile int32_t  ans_print_interval   =  120;        // seconds
static volatile int32_t  ans_retry_count      =    0;
static volatile int32_t  ans_base_interval    =    0;
static volatile int32_t  ans_retry_interval   =    0;
static volatile int32_t  ans_ping_back        =    0;
static volatile int32_t  ans_ping_factor      =    1;

#if defined(ANDROID) || defined(IOS) || defined(VPL_PLAT_IS_WINRT)
static int32_t  ans_tcp_ka_time      =  180;
static int32_t  ans_tcp_ka_interval  =    5;
static int32_t  ans_tcp_ka_probes    =    6;
#else
static int32_t  ans_tcp_ka_time      =   30;
static int32_t  ans_tcp_ka_interval  =    3;
static int32_t  ans_tcp_ka_probes    =   10;
#endif

static int32_t  ans_max_encrypt      =  256;

static int      ans_reject_count     = 0;        /* for testing */
static int      ans_fail_pack        = 0;        /* for testing */
static int      ans_print_count      = 0;        /* for testing */
static int      ans_packets_out      = 0;        /* for testing */
static int      ans_pings_out        = 0;        /* for testing */
static int      ans_shutdowns        = 0;        /* for testing */
static int      ans_write_limit      = 100000;   /* for testing */
static int      ans_partial_timeouts = 0;        /* for testing */
static int      ans_invalid_closes   = 0;        /* for testing */
static int      ans_fast_timeout     = 0;        /* for testing */
static int      ans_fast_packets     = 0;        /* for testing */
static int      ans_timeout_events   = 0;        /* for testing */
static int      ans_keep_packets_out = 0;        /* for testing */
static int      ans_keep_packets_in  = 0;        /* for testing */
static int      ans_keep_connection  = 0;        /* for testing */
static int      ans_no_output        = 0;        /* for testing */
static int      ans_timeout_closes   = 0;        /* for testing */
static int      ans_backwards_time   = 0;        /* for testing */
static int64_t  ans_connect_errors   = 0;        /* for testing */
static int64_t  ans_force_fail       = 0;        /* for testing */
static int64_t  ans_force_time       = 0;        /* for testing */
static int64_t  ans_force_socket     = 0;        /* for testing */
static int64_t  ans_force_change     = 0;        /* for testing */
static int64_t  ans_force_make       = 0;        /* for testing */
static int64_t  ans_force_event      = 0;        /* for testing */
static int      ans_bad_keep_sizes   = 0;        /* for testing */
static int      ans_keep_errors      = 0;        /* for testing */
static int      ans_event_deaths     = 0;        /* for testing */
static int      ans_back_loops       = 0;        /* for testing */
static int      ans_lock_errors      = 0;        /* for testing */
static int      ans_forced_closes    = 0;        /* for testing */
static int      ans_short_packets    = 0;        /* for testing */
static int64_t  ans_max_error        = 0;        /* for testing */

static const char * ans_timeout_where;           /* for testing */

typedef struct {
    int         type;
    int         count;
    void *      data;
    uint64_t    async_id;
    uint64_t    user_id;
    uint64_t    device_id;
    uint64_t    tag;
    int         ioac;
    int         version;
} describe_t;

#define clear_describe(describe)  \
            do { memset((describe), 0, sizeof(describe_t)); } while (0)

/*
 *  Define a type for passing around a buffer for an outgoing packet.
 */
typedef struct packet_s  packet_t;

struct packet_s {
    char *        base;       /* the base address of the packet           */
    char *        buffer;     /* the I/O pointer when writing the packet  */
    int           length;     /* bytes                                    */
    int           prepared;   /* boolean                                  */
    packet_t *    next;       /* forward link for the queue               */
    uint64_t      sequence;   /* the sequence id                          */
    describe_t    describe;
    const char *  where;      /* for debugging                            */
};

/*
 *  Define the block size and the control structure for encryption.  We
 *  use AES.
 */
#define aes_block_size 16

typedef struct {
    u8 *             sha1_key;
    u8               aes_key[aes_block_size];
    unsigned int     sha1_length;
} crypto_t;

/*
 *  Define the structure for controlling a channel for event notification.
 *  Each channel has a socket for sending an event notification, a socket
 *  for receiving notifications, and a server socket for making the TCP/IP
 *  connection that is used.
 *
 *  An "event" is signaled by writing one byte to the outgoing socket.
 */
typedef struct {
    VPLSocket_t   socket;           /* the socket for listening         */
    VPLSocket_t   server_socket;    /* the server socket for connecting */
    VPLSocket_t   out_socket;       /* the socket for sending an event  */
    volatile int  dead;             /* a flag for a dead event channel  */
    volatile int  backoff;          /* the current retry time           */
    VPLMutex_t    mutex;
    volatile int  pending_event;    /* for debugging */
} event_t;

/*
 *  Define the control structure for a keep-alive thread and its
 *  processing.
 */
typedef struct {
    ans_client_t *    client;              /* a pointer back to the client */
    VPLSocket_t       socket;              /* the UDP socket for I/O       */
    volatile int      stop;
    volatile int      done;

    /*
     *  The keep-alive thread keeps an estimate of the round-trip time for
     *  a UDP ping.
     */
#define buckets  20
    VPLTime_t         ping_when[buckets];  /* the receive time             */
    int64_t           ping_rtt [buckets];  /* past ping round trip times   */
    int64_t           ping_id  [buckets];  /* the sequence id for the rtts */
    int64_t           rtt_estimate;        /* the current rtt estimate     */

    int64_t           sort_area[buckets];  /* for get_median               */
#undef buckets

    int64_t           time_to_millis;
    int64_t           out_sequence;
    int64_t           in_sequence;
    int               packet_length;

    packet_t *        udp_head;         /* the head of the output queue */
    packet_t *        udp_tail;

    /*
     *  These fields are used to communicate with the run_device
     *  thread.  The client design wiki page, q.v., has an explanation
     *  of the algorithm used.
     */
    VPLMutex_t        mutex;
    volatile int32_t  live_socket_id;
    volatile int32_t  dead_socket_id;
    volatile int      declared_dead;
    volatile int      keep_foreground;
    volatile int64_t  last_tcp_receive;
    int64_t           last_udp_receive;
    int64_t           last_udp_send;
    int64_t           udp_in;
    int64_t           udp_blocked;
    volatile int64_t  deadline;
    int               received_udp;
    int               warned;
    event_t           event;
    int               i;
#define history 16
    VPLTime_t         sends[history];
    int64_t           id   [history];
    const char *      who  [history];
#undef history

    volatile int      keeping;  /* for testing */
    const char *      where;
} keep_t;

/*
 *  We use a Mersenne twister to generate pseudo-random numbers as needed.
 *  This structure defines an instance of a Mersenne twister.
 */
struct mtwist_s {
    int       index;
    int       inited;
    uint32_t  state[623];
};

/*
 *  ans_client_t
 *
 *     This structure is the base state structure for an instance of the
 *  ans device client.  The thread is passed this structure as its argument.
 */
struct client_s {
    ans_callbacks_t  callback_table;
    char *           cluster;          /* the DNS name of the server host    */
    char *           key;              /* the MAC key and length             */
    int              key_length;
    char *           blob;             /* the connection blob from AnsCommon */
    int              blob_length;
    char *           device_type;
    char *           application;
    char *           host_name;
    char *           external_ip;
    int              external_count;
    int              external_port;
    mtwist_t         mt;
    int              login_inited;
    volatile int     is_foreground;
    volatile int     open;
    volatile int     stop_now;         /* a flag to tell the thread to stop  */
    volatile int     cleanup;          /* describes how to clean up at exit  */
    int              verbose;          /* turns on more debugging output     */
    volatile int     ready_to_send;
    volatile int     login_completed;
    int              delay;
    int64_t          out_sequence;
    uint64_t         packets_in;
    uint64_t         device_id;
    uint64_t         connection;
    uint64_t         server_flags;
    uint64_t         generation;
    uint64_t         server_time;
    uint64_t         tag;
    volatile int     network_changed;
    volatile int     force_close;
    volatile int     rejected;
    char *           challenge;
    int              challenge_length;
    int32_t          keep_port;
    crypto_t *       crypto;
    VPLSocket_t      socket;
    VPLMutex_t       mutex;  ///< Protects this structure.
    VPLMutex_t       mode_mutex;
    VPLNet_addr_t    server_address;
    int              subscription_count;
    uint64_t *       subscriptions;
    keep_t           keep;
    int              keep_started;
    int32_t          next_socket_id;
    event_t          event;
    int              enable_tcp_only;
    int32_t          server_tcp_port;   

    VPLDetachableThreadHandle_t  device_handle;
    VPLDetachableThreadHandle_t  keep_handle;

    volatile VPLTime_t      interval;
    volatile VPLNet_addr_t  server_ip;
    packet_t * volatile     tcp_head;
    packet_t * volatile     tcp_tail;
};

/*
 *  This structure holds the user-level keep-alive parameters.
 */
typedef struct {
    int64_t  base_delay;        /* the UDP packet interval in normal mode  */
    int64_t  retry_delay;       /* the UDP packet interval in retry mode   */
    int64_t  udp_limit;         /* the maximum time to wait for a UDP ping */
    int64_t  tcp_limit;         /* the maximum time to wait for a TCP ping */
    int      retry_count;
    int      factor;
    int      valid;
} limits_t;

static packet_t *pack(ans_client_t *, describe_t *);

#ifdef ans_test
static void
print_data(const char *title, int64_t *data, int count)
{
    char    buffer[4096];
    char *  current;
    int     remaining;
    int     added;

    current    = buffer;
    remaining  = sizeof(buffer) - 1;

    added = snprintf(current, remaining, "%s = { " FMTs64, title, data[0]);

    current   += added;
    remaining -= added;

    for (int i = 1; i < count; i++) {
        added = snprintf(current, remaining, ", " FMTs64, data[i]);

        current   += added;
        remaining -= added;
    }

    snprintf(current, remaining, " };");

    log_test("%s", buffer);
}
#else
#define print_data(t, d, c)
#endif

/*
 *  Convert a given time to a UTC text string.
 */
static char *
print_time(VPLTime_t time, char *buffer, int size)
{
    VPLTime_CalendarTime_t  vplex_time;

    VPLTime_ConvertToCalendarTimeUniversal(time, &vplex_time);

    snprintf(buffer, size, date_format,
        vplex_time.year,
        vplex_time.month,
        vplex_time.day,
        vplex_time.hour,
        vplex_time.min,
        vplex_time.sec,
        vplex_time.msec);

    /*
     *  Make sure there's a null termination.
     */
    buffer[size - 1] = 0;
    return buffer;
}

/*
 *  Create a UTC timestamp using the current time.
 */
static char *
utc_ts(char *buffer, int size)
{
    return print_time(VPLTime_GetTime(), buffer, size);
}

/*
 *  Convert a response code from the server into a string.
 */
static const char *
response_string(uint64_t response)
{
    const char *  result;

    if (response > 0 && response < array_size(op_results)) {
        result = op_results[response];
    } else {
        result = "unknown";
    }

    return result;
}

#ifdef ans_test_hex
static void
make_printable(const char *contents, char *printable, int length)
{
    memcpy(printable, contents, length);

    for (int c = 0; c < length; c++) {
        if
        (
           !(isprint(printable[c]) && !isspace(printable[c]))
        && printable[c] != ' '
        ) {
            printable[c] = '.'; 
        }
    }

    printable[length] = 0;
}

#define per_line  4

static void
hex_dump(const char *title, const char *contents, int length)
{
    int      i;
    int      stride;
    int      count;
    int32_t  data1;
    int32_t  data2;
    int32_t  data3;
    int32_t  data4;
    int      start;
    char     printable[per_line * sizeof(data1) + 1];

    log_info("dump %s (%d bytes)", title, length);

    count  = sizeof(data1);
    stride = per_line * count;

    for (i = 0; i < length / stride; i++) {
        memcpy(&data1, contents + (i * stride) + 0 * count, count);
        memcpy(&data2, contents + (i * stride) + 1 * count, count);
        memcpy(&data3, contents + (i * stride) + 2 * count, count);
        memcpy(&data4, contents + (i * stride) + 3 * count, count);

        make_printable(contents + i * stride, printable, per_line * sizeof(data1));

        log_info("  0x%8.8x  0x%8.8x  0x%8.8x  0x%8.8x    %s",
            data1, data2, data3, data4, printable);
    }

    start = (i * stride) / count;

    for (i = start; i < length / count; i++) {
        memcpy(&data1, contents + (i * count), count);
        make_printable(contents + i * count, printable, count);
        log_info("  0x%8.8x    %s", data1, printable);
    }

    start = i * count;

    for (i = start; i < length; i++) {
        make_printable(contents + i, printable, 1);
        log_info("  0x%2.2x   %s", contents[i] & 0xff, printable);
    }
}
#endif

static void
mutex_init(VPLMutex_t *mutex)
{
    int  result;

    result = VPLMutex_Init(mutex);

    if (result != VPL_OK) {
        log_error("VPLMutex_Init failed: %d", result);
        ans_lock_errors++;
    }
}

static void
mutex_lock(VPLMutex_t *mutex)
{
    int  result;

    result = VPLMutex_Lock(mutex);

    /*
     *  We have nothing useful to do if we can't get the lock.  We should
     *  abort the program, most likely.
     */
    if (result != VPL_OK) {
        ans_lock_errors++;
    }
}

static void
mutex_unlock(VPLMutex_t *mutex)
{
    VPLMutex_Unlock(mutex);
}

/*
 *  Close a socket, if it's valid, and record that we closed it.
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
 *  Send an event to a thread.
 */
static void
ping_thread(event_t *event, const char *where)
{
    char  buffer;
    int   count;

    buffer  = 0;
    count   = 1;

    mutex_lock(&event->mutex);
    event->pending_event++;    /* for testing */

    if (valid(event->out_socket)) {
        count = VPLSocket_Send(event->out_socket, &buffer, 1);
    }

    if (count <= 0 || --ans_force_event == 0) {
        close_socket(&event->out_socket);
        ans_event_deaths++;
        event->dead = true;
        log_error("%s event error:  %d", where, count);
    }

    mutex_unlock(&event->mutex);
}

/*
 *  Ping the device thread.
 */
static void
ping_device(ans_client_t *client)
{
    ping_thread(&client->event, "device");
}

/*
 *  Ping the keep-alive thread.
 */
static void
ping_keep_alive(ans_client_t *client)
{
    ping_thread(&client->keep.event, "keep-alive");
}

/*
 *  Initialize an event structure.  Only a few fields need to be
 *  set.
 */
static void
init_event(event_t *event)
{
    event->server_socket = VPLSOCKET_INVALID;
    event->socket        = VPLSOCKET_INVALID;
    event->out_socket    = VPLSOCKET_INVALID;
    event->dead          = false;
}

/*
 *  Release any resources allocated to an event.   Currently,
 *  that's just the sockets.
 */
static void
free_event(event_t *event)
{
    mutex_lock(&event->mutex);

    close_socket(&event->server_socket);
    close_socket(&event->socket);
    close_socket(&event->out_socket);

    event->dead = true;
    mutex_unlock(&event->mutex);
}

/*
 *  Compute an exponentional back-off time for event creation
 *  retry.  An upper limit is imposed on the wait time.
 */
static int
backoff(event_t *event)
{
    int  result;

    result         = max(event->backoff, event_min_delay);
    result         = min(result,         event_max_delay);
    event->backoff = min(2 * result,     event_max_delay);

    return result;
}

/*
 *  Try to make a loop-back connection for sending events.
 */
static void
make_event(event_t *event)
{
    VPLSocket_addr_t  address;

    int  result;

    init_event(event);

    mutex_lock(&event->mutex);
    event->dead = true;

    /*
     *  First make a server socket to which we can connect.
     */
    event->server_socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_FALSE);

    if (invalid(event->server_socket) || --ans_force_make == 0) {
        mutex_unlock(&event->mutex);
        log_error_1("event server_socket creation failure");
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        return;
    }

    /*
     *  Now bind the socket to an address.  We use port 0, the wildcard port.
     */
    memset(&address, 0, sizeof(address));

    address.family = VPL_PF_INET;
    address.addr   = VPLNET_ADDR_LOOPBACK;

    result = VPLSocket_Bind(event->server_socket, &address, sizeof(address));

    if (result != VPL_OK || --ans_force_make == 0) {
        mutex_unlock(&event->mutex);
        log_error("event bind failure %d", result);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        free_event(event);
        return;
    }

    /*
     *  Make it possible to receive connections on the socket.
     */
    result = VPLSocket_Listen(event->server_socket, 10);

    if (result != VPL_OK || --ans_force_make == 0) {
        mutex_unlock(&event->mutex);
        log_error("event listen failure %d", result);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        free_event(event);
        return;
    }

    /*
     *  Get the address of the server socket.  We used a wildcard port,
     *  so now we need to determine the actual target.
     */
    address.addr = VPLSocket_GetAddr(event->server_socket);
    address.port = VPLSocket_GetPort(event->server_socket);

    /*
     *  Now create a socket and connect to the server socket.
     */
    event->out_socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);

    if (invalid(event->out_socket) || --ans_force_make == 0) {
        mutex_unlock(&event->mutex);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        log_error_1("event out_socket creation failure");
        free_event(event);
        return;
    }

    result = VPLSocket_Connect(event->out_socket, &address, sizeof(address));

    if (result != VPL_OK || --ans_force_make == 0) {
        mutex_unlock(&event->mutex);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        log_error("event connection failure %d", result);
        free_event(event);
        return;
    }

    /*
     *  Accept the incoming connection.
     */
    result = VPLSocket_Accept(event->server_socket, &address, sizeof(address), &event->socket);

    if (result != VPL_OK || --ans_force_make == 0) {
        mutex_unlock(&event->mutex);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        log_error("event accept failure %d", result);
        free_event(event);
        return;
    }

    event->dead    = false;
    event->backoff = event_min_delay;

    mutex_unlock(&event->mutex);
}

/*
 *  Poll to see whether an event has occurred.  Also, check
 *  for socket failures.
 */
static void
check_event(event_t *event, int result, VPLSocket_poll_t *poll)
{
    char  buffer[80];
    int   count;

    event->pending_event = 0;   /* for debugging only */

    if (poll->revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP)) {
        event->dead = true;
        log_error_1("event socket failure on poll");
    } else if (poll->revents & VPLSOCKET_POLL_RDNORM) {
        count = VPLSocket_Recv(event->socket, buffer, sizeof(buffer));

        if (count < 0) {
            event->dead = true;
            log_error_1("event socket failure on Recv");
        }
    }

    /*
     *  If our event socket has died, make a new one if possible.
     */
    if (event->dead) {
        ans_event_deaths++;
        mutex_lock(&event->mutex);
        free_event(event);
        make_event(event);
        mutex_unlock(&event->mutex);
    }
}

/*
 *  Wait for an event to be posted.  A zero or negative time
 *  limit is translated to a 2 second wait.
 */
static void
event_wait(event_t *event, int64_t wait_limit)
{
    uint64_t  timeout;
    int       result;

    VPLSocket_poll_t  poll[1];

    timeout         = wait_limit > 0 ? wait_limit : VPLTime_FromSec(2000000);
    poll[0].socket  = event->socket;
    poll[0].events  = VPLSOCKET_POLL_RDNORM;
    poll[0].revents = 0;

    result = VPLSocket_Poll(poll, array_size(poll), timeout);

    check_event(event, result, &poll[0]);
}

/*
 *  Encrypt a message via AES given an IV, a byte count, and a length.
 *  This routine does any needed padding to the cipher block length.
 */
static VPL_BOOL
ans_encrypt
(
    ans_client_t *  client,
    char **         ciphertext,
    int *           cipher_length,
    char *          iv,
    const char *    message_in,
    int             message_length
)
{
    u8 *  padded_text;
    u8 *  output;
    int   padded_length;
    int   output_length;
    u16   length;
    int   result;
    int   pad_count;

    *ciphertext    = null;
    *cipher_length = 0;

    if (message_length <= 0 || message_length > ans_max_encrypt) {
        return VPL_FALSE;
    }

    /*
     *  We are not using any of the auto-padding schemes, so compute
     *  the bytes necessary to round the plaintext (a two-byte count
     *  plus the message_in) up to the aes_block_size.  We will fill
     *  the padding area with zeros.  Also, compute the output size,
     *  which includes the IV.
     */
    padded_length = sizeof(length) + message_length;

    if (padded_length % aes_block_size != 0) {
        pad_count = aes_block_size - (padded_length % aes_block_size);
    } else {
        pad_count = 0;
    }

    padded_length += pad_count;
    output_length  = padded_length + aes_block_size;

    /*
     *  Allocate the buffers.
     */
    output         = (u8 *) malloc(output_length);
    padded_text    = (u8 *) malloc(padded_length);

    if (output == null || padded_text == null) {
        log_warn_1("malloc failure in ans_encrypt");
        free(output);
        free(padded_text);
        return VPL_FALSE;
    }

    /*
     *  Save the IV to the output area.
     */
    memcpy(output, iv, aes_block_size);

    /*
     *  Move the length and plain text into the padded area, then zero the
     *  rest of the buffer to keep valgrind happy.
     */
    length = VPLConv_ntoh_u16(message_length);
    memcpy(padded_text,                 &length,     sizeof(length));
    memcpy(padded_text + sizeof(length), message_in, message_length);
    memset(padded_text + sizeof(length) + message_length, 0, pad_count);

    /*
     *  Run the actual encryption algorithm (finally).  Once that's
     *  done, free the padded text.
     */
    result = aes_SwEncrypt(client->crypto->aes_key, (u8 *) iv, padded_text,
                padded_length, output + aes_block_size);

    free(padded_text);

    if (result < 0) {
        free(output);
        log_error("aes_SwEncrypt failed:  %d", (int) result);
        return VPL_FALSE;
    }

    *ciphertext    = (char *) output;
    *cipher_length = output_length;

    return VPL_TRUE;
}

/*
 *  Decrypt a message from the server and discard any padding.
 */
static VPL_BOOL
ans_decrypt
(
    ans_client_t *  client,
    char **         plaintext,
    int *           plain_length,
    const char *    message_in,
    int             message_length
)
{
    u8 *  message;
    u8 *  work;
    u8 *  iv;
    u8 *  ciphertext;
    u8 *  text;
    u16   length;
    int   result;
    int   cipher_length;

    message = (u8 *) message_in;

    *plaintext    = null;
    *plain_length = 0;

    /*
     *  Check that the message is feasible.  It must be a multiple
     *  of the AES block size in length.
     */
    if (message_length % aes_block_size != 0) {
        return VPL_FALSE;
    }

    /*
     *  It must contain the IV and a message, so it must be more
     *  than one block size in length.
     */
    if (message_length <= aes_block_size) {
        return VPL_FALSE;
    }

    /*
     *  Now compute the addresses of the IV and the actual ciphertext.
     */
    iv            = message;
    ciphertext    = message + aes_block_size;
    cipher_length = message_length - aes_block_size;

    work = (u8 *) malloc(cipher_length);

    if (work == null) {
        log_error_1("malloc failed in ans_decrypt");
        return VPL_FALSE;
    }

    /*
     *  Decrypt the ciphertext.
     */
    result = aes_SwDecrypt(client->crypto->aes_key, iv, ciphertext,
            cipher_length, work);

    if (result < 0) {
        free(work);
        log_error("aes_SwDecrypt failed:  %d", (int) result);
        return VPL_FALSE;
    }

    /*
     *  Get the actual length of the message from the packet.
     */
    memcpy(&length, work, sizeof(length));
    length = VPLConv_ntoh_u16(length);

    /*
     *  Check that the length is feasible.
     */
    if (length <= 0 || length > cipher_length - sizeof(length)) {
        free(work);
        log_error("got message length %d", (int) length);
        return VPL_FALSE;
    }

    /*
     *  Get some space to hold the result, then copy the plaintext
     *  into it.
     */
    text = (u8 *) malloc(length);

    if (text == null) {
        free(work);
        log_error_1("malloc failed in ans_decrypt (text)");
        return VPL_FALSE;
    }

    memcpy(text, work + 2, length);

    *plaintext    = (char *) text;
    *plain_length = length;

    free(work);
    return VPL_TRUE;
}

/*
 *  Create a packet structure.  Set the data and length fields,
 *  if given.
 */
static packet_t *
alloc_packet(char *data, int length)
{
    packet_t *  packet;

    packet = (packet_t *) malloc(sizeof(*packet));

    if (packet == null) {
        return null;
    }

    memset(packet, 0, sizeof(*packet));

    packet->base     = data;
    packet->buffer   = data;
    packet->length   = length;
    packet->prepared = VPL_FALSE;
    packet->next     = null;
    packet->where    = "control";

    return packet;
}

/*
 *  Free a packet structure and any buffers attached to it.
 */
static void
free_packet(packet_t **packet)
{
    if (*packet != null) {
        free((*packet)->base);
        free((*packet)->describe.data);
        free(*packet);
        *packet = null;
    }
}

/*
 *  Convert a packet type to a string.
 */
static const char *
packet_type(int type)
{
    const char *  result;

    switch(type) {
    case Send_unicast:
        result = "Send_unicast";
        break;

    case Send_multicast:
        result = "Send_multicast";
        break;

    case Send_subscriptions:
        result = "Send_subscriptions";
        break;

    case Send_challenge:
        result = "Send_challenge";
        break;

    case Send_ping:
        result = "Send_ping";
        break;

    case Send_timed_ping:
        result = "Send_timed_ping";
        break;

    case Send_login_blob:
        result = "Send_login_blob";
        break;

    case Set_device_params:
        result = "Set_device_params";
        break;

    case Request_sleep_setup:
        result = "Request_sleep_setup";
        break;

    case Send_sleep_setup:
        result = "Send_sleep_setup";
        break;

    case Request_wakeup:
        result = "Request_wakeup";
        break;

    case Reject_credentials:
        result = "Reject_credentials";
        break;

    case Set_login_version:
        result = "Set_login_version";
        break;

    case Set_param_version:
        result = "Set_param_version";
        break;

    case Query_device_list:
        result = "Query_device_list";
        break;

    case Send_response:
        result = "Send_response";
        break;

    case Send_device_update:
        result = "Send_device_update";
        break;

    case Send_device_shutdown:
        result = "Send_device_shutdown";
        break;

    case Send_state_list:
        result = "Send_state_list";
        break;

    default:
        result = "unknown";
        break;
    }

    return result;
}

/*
 *  This routine removes the packet at the head of the output
 *  queue, frees the memory associated with it, and then returns
 *  the next available packet, or null, if the queue is empty.
 *
 *  It is called when the thread finishes writing a packet or when
 *  some sort of failure requires flushing the queue.
 */
static packet_t *
advance_queue(ans_client_t *client, int success, packet_t *current)
{
    packet_t *  packet;
    packet_t *  next;

    packet            = client->tcp_head;
    client->tcp_head  = packet->next;
    next              = packet->next;

    ASSERT(current == null || current == packet);

    /*
     *  Clear the tail pointer if the queue is empty now.
     */
    if (client->tcp_head == null) {
        client->tcp_tail = null;
    }

    if (packet != null && success) {
        ans_packets_out++;  // for testing

        if (packet->describe.type == Send_ping) {
            ans_pings_out++;    // for testing
        }
    }

    free_packet(&packet);

    return next;
}

/*
 *  Discard any packets queued to the TCP socket.  This routine
 *  is called when a socket is shut down or fails.
 */
static void
discard_tcp_queue(ans_client_t *client)
{
    mutex_lock(&client->mutex);

    while (client->tcp_head != null) {
        advance_queue(client, VPL_FALSE, null);
    }

    mutex_unlock(&client->mutex);
}

/*
 *  Add the sequence number to a packet and sign it if that
 *  hasn't been done already.  We also increment the outgoing
 *  sequence id here.
 */
static void
prep_packet(ans_client_t *client, packet_t *packet, int64_t *sequence)
{
    uint64_t  sequence_id;

    CSL_ShaContext    context;
    unsigned char *   buffer;
    unsigned char *   signature_area;

    if (packet->prepared) {
        return;
    }

    packet->sequence = *sequence;
    sequence_id = VPLConv_ntoh_u64(*sequence);
    (*sequence)++;

    /*
     *  TODO remove
     */
    if (client->tag != 0) {
        memcpy(packet->buffer + sequence_offset + 2, &sequence_id, sizeof(sequence_id));
    } else {
        memcpy(packet->buffer + sequence_offset, &sequence_id, sizeof(sequence_id));
    }

    if (client->verbose && packet->describe.type != Send_timed_ping) {
        log_info("assign packet %p (%s, %d) sequence id " FMTu64,
            packet,
            packet_type(packet->describe.type),
            packet->describe.type,
            packet->sequence);
    }

    buffer          = (unsigned char *) packet->buffer;
    signature_area  = buffer + packet->length - signature_size;

    CSL_ResetSha (&context);
    CSL_InputSha (&context, client->crypto->sha1_key, client->crypto->sha1_length);
    CSL_InputSha (&context, buffer, packet->length - signature_size);
    CSL_ResultSha(&context, signature_area);

    packet->prepared = VPL_TRUE;
}

/*
 *  Create the basic wire-format for a packet if that hasn't
 *  been done.  The signature and sequence number are added later.
 */
static int
pack_queued(ans_client_t *client, packet_t *packet)
{
    packet_t *  sendable;

    if (packet->buffer == null) {
        packet->describe.tag = client->tag;

        sendable = pack(client, &packet->describe);

        if (sendable == null) {
            return false;
        }

        sendable->describe = packet->describe;
        sendable->next     = packet->next;
        sendable->where    = packet->where;

        *packet = *sendable;
        free(sendable);
    }

    return true;
}

/*
 *  Try to do some asynchronous output, if there's anything
 *  on the queue.
 */
static void
write_output(ans_client_t *client)
{
    int  count;
    int  done;
    int  pass;

    packet_t *  packet;

    mutex_lock(&client->mutex);

    packet = client->tcp_head;

    if (packet == null || client->crypto == null) {
        ans_no_output++;
        mutex_unlock(&client->mutex);
        return;
    }

    /*
     *  Try to make a packet ready to send.  It will fail
     *  on a malloc failure.  Sleep a bit if we get a malloc
     *  error, to avoid a spin loop.
     */
    pass = pack_queued(client, packet);

    if (!pass) {
        mutex_unlock(&client->mutex);
        VPLThread_Sleep(VPLTime_FromMillisec(200));
        return;
    }

    /*
     *  If this packet hasn't had the sequence number inserted and
     *  this signature generated, do that now.
     */
    prep_packet(client, packet, &client->out_sequence);
    mutex_unlock(&client->mutex);

    /*
     *  Okay, write until the socket is full or we're out of packets.
     */
    do {
        count = packet->length;

        if (ans_write_limit != 0 && count > ans_write_limit) { // for testing
            count = ans_write_limit;
        }

        count = VPLSocket_Send(client->socket, packet->buffer, count);
        done  = count <= 0;

        if (done) {
            break;
        }

        /*
         *  If the write succeeded in sending some data, we have some work
         *  to do.
         */
        packet->length -= count;
        packet->buffer += count;

        /*
         *  Check whether we've written the entire packet.  If so,
         *  print some logging information and advance the queue.
         */
        if (packet->length == 0) {
            if (packet->describe.type != Send_timed_ping) {
                log_info("sent packet %p (%s, %d), id " FMTu64 ", sequence " FMTu64 " (%s)",
                    packet,
                    packet_type(packet->describe.type),
                    packet->describe.type,
                    packet->describe.async_id,
                    packet->sequence,
                    packet->where);
            }

#ifdef ans_test
            if (packet->describe.type == Send_timed_ping) {
                log_info("sent TCP ping " FMTu64 " for client %p", packet->sequence, client);
            }
#endif

            /*
             *  Get the next packet from the queue, if there is one.
             */
            mutex_lock(&client->mutex);
            packet = advance_queue(client, VPL_TRUE, packet);
            done   = packet == null;

            if (done) {
                mutex_unlock(&client->mutex);
                break;
            }

            /*
             *  If there's more to write, pack the buffer and add
             *  the sequence number, if we can.
             */
            pass = pack_queued(client, packet);

            if (pass) {
                prep_packet(client, packet, &client->out_sequence);
            }

            mutex_unlock(&client->mutex);

            /*
             *  If we hit a malloc failure in pack_queued, we're done
             */
            done = !pass;
        }
    } while (!done);

    return;
}

/*
 *  Sleep for a given period of time, or until an event
 *  occurs.  While sleeping, we need to write output to
 *  the socket, if there's anything queued, so we don't
 *  actually get much rest.
 */
static void
sleep_seconds(ans_client_t *client, int seconds)
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
    failed          = invalid(client->socket);

    do {
        poll[0].socket   = client->event.socket;
        poll[0].events   = VPLSOCKET_POLL_RDNORM;

        poll[1].socket   = client->socket;
        poll[1].events   = 0;

        /*
         *  If there's anything queued for output, and the TCP socket
         *  is valid, be sure to poll on it.
         */
        if (!failed && client->tcp_head != null) {
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

        check_event(&client->event, result, &poll[0]);

        failed = poll_count > 1 && (poll[1].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP));

        /*
         *  Try to compute the wall-clock time that we've waited, in case the
         *  device sleeps.  If time seems to move backward, just give up
         *  and return.
         */
        current_time = VPLTime_GetTime();

        if (current_time >= start_time && --ans_force_time != 0) {
            new_time_waited  = current_time - start_time;

            /*
             *  Check for overflow...  Exit the wait loop if there's
             *  a problem.
             */
            if (new_time_waited < 0) {
                log_info("wait time overflow:  " FMTu64 " to " FMTu64,
                    (uint64_t) start_time, (uint64_t) current_time);
                time_waited = requested_wait;
            } else {
                time_waited = new_time_waited;
            }
        } else {
            log_info("time backed up:  " FMTu64 " to " FMTu64,
                (uint64_t) start_time, (uint64_t) current_time);
            ans_backwards_time++;
            time_waited  = requested_wait;
        }

        /*
         *  Write any output we can, if there's anything queued.
         */
        if (client->tcp_head != null && valid(client->socket)) {
            write_output(client);
        }
    } while
    (
        !client->stop_now
    &&  !client->network_changed
    &&  time_waited < requested_wait
    );
}

/*
 *  Set the kernel TCP/IP keep-alive parameters, if the platform supports
 *  it.
 */
static int
ans_set_keep_alive(VPLSocket_t sock)
{
    int  result;

#ifdef IOS
   result = VPL_OK;
#else
    if
    (
        ans_tcp_ka_time > 0
    &&  ans_tcp_ka_interval > 0
    &&  ans_tcp_ka_probes > 0
    ) {
        result = VPLSocket_SetKeepAlive(sock, VPL_TRUE,
                ans_tcp_ka_time, ans_tcp_ka_interval, ans_tcp_ka_probes);
    } else {
        result = VPLSocket_SetKeepAlive(sock, VPL_FALSE, 0, 0, 0);
    }
#endif

    return result;
}

static const char * digits = "0123456789";

/*
 *  Convert a network address into a hex string.
 */
static void
convert_address(VPLNet_addr_t address, char *buffer, int max_length)
{
    char *  current;
    int     i;
    int     element;
    int     force;

    address = VPLConv_ntoh_u32(address);
    current = buffer;

    for (i = 0; i < 4; i++) {
        element  = address >> (24 - i * 8);
        element &= 0xff;
        force    = false;

        if (element >= 100) {
            *current++ = digits[element / 100];
            element %= 100;
            force    = true;
        }

        if (element >= 10 || force) {
            *current++ = digits[element / 10];
            element %= 10;
        }

        *current++ = digits[element];

        if (i != 3) {
            *current++ = '.';
        }
    }

    *current = 0;
}

/*
 *  deprep_tcp_queue - mark all packets as needed serial numbers
 *
 *  When a connection breaks, any packets that have serial numbers
 *  need to be given new ones, so mark them unprepared.  The caller
 *  must hold the client mutex.
 */
static void
deprep_tcp_queue(ans_client_t *client)
{
    packet_t *  current;

    current = client->tcp_head;

    while (current != null) {
        current->prepared = false;
        current = current->next;
    }
}

/*
 *  Open a TCP connection to an ANS server.  Don't return until the
 *  connection is valid or we're told to shut down.
 */
static VPLSocket_t
sync_open_socket(ans_client_t *client)
{
    int   delay;
    int   error;
    int   result;
    char  server_quad[256];
    char  local_quad [256];
    int   jitter;

    VPLSocket_addr_t  server_address;
    VPLNet_addr_t     net_address;
    VPLSocket_t       socket;
    VPLNet_addr_t     local_address;
    VPLNet_port_t     local_port;

    log_always("starting a connection attempt to %s:%d",
        client->cluster, client->server_tcp_port);

    socket = VPLSOCKET_INVALID;
    delay  = ans_min_delay;

    server_quad[0] = 0;
    local_quad [0] = 0;

    /*
     *  Keep looping until we're asked to shut down or we manage
     *  to connect.
     */
    while (!client->stop_now && invalid(socket)) {
        if (client->verbose && delay < ans_max_delay) {
            log_info("I am attempting to connect to %s:%d",
                client->cluster, client->server_tcp_port);
        }

        /*
         *  Try to create a TCP socket.
         */
        socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);

        if (invalid(socket) || --ans_force_socket == 0) {
            if (client->verbose && delay < ans_max_delay) {
                log_warn_1("I failed to create a socket.");
            }

            close_socket(&socket);
        }

        /*
         *  If we got a valid socket, get the network address of an ANS
         *  server.
         */
        if (valid(socket)) {
            net_address = VPLNet_GetAddr(client->cluster);

            if (net_address == VPLNET_ADDR_INVALID || --ans_force_socket == 0) {
                if (client->verbose && delay < ans_max_delay) {
                    log_warn_1("I failed to get an address for the server.");
                }

                close_socket(&socket);
            } else {
                convert_address(net_address, server_quad, sizeof(server_quad));
            }
        }

        /*
         *  Set keep-alive now that we're ready to connect.
         */
        if (valid(socket)) {
            result = ans_set_keep_alive(socket);

            if (result != VPL_OK || --ans_force_socket == 0) {
                log_warn("I failed to set the TCP keep-alive values: %d", result);
                close_socket(&socket);
            }
        }

        /*
         *  Okay, we have a socket and an address, so try to connect.
         */
        if (valid(socket)) {
            client->server_ip     = net_address;
            server_address.addr   = net_address;
            server_address.port   = VPLConv_ntoh_u16(client->server_tcp_port);
            server_address.family = VPL_PF_INET;
            client->force_close   = false;    /* no point in dropping a new connection */

            error = VPLSocket_Connect(socket, &server_address, sizeof(server_address));

            if (error != VPL_OK || --ans_force_socket == 0) {
                if (client->verbose && delay < ans_max_delay) {
                    log_warn("The connect attempt to %s(%s):%d failed:  %d (%s)",
                        client->cluster, server_quad, client->server_tcp_port, error,
                        error == VPL_ERR_TIMEOUT ?  "timeout" : "unknown");
                }

                close_socket(&socket);
            }
        }

        /*
         *  If the connect succeeded, declare success.
         */
        if (valid(socket)) {
            mutex_lock(&client->keep.mutex);
            client->server_address       = net_address;
            client->keep.declared_dead   = false;
            client->keep.live_socket_id  = -1;
            deprep_tcp_queue(client);
            mutex_unlock(&client->keep.mutex);

            local_address = VPLSocket_GetAddr(socket);
            local_port    = VPLSocket_GetPort(socket);

            convert_address(local_address, local_quad, sizeof(local_quad));

            log_info("I am connected to %s(%s):%d via %s:%d",
                client->cluster,
                server_quad,
                client->server_tcp_port,
                local_quad,
                VPLConv_ntoh_u16(local_port));
        }

        /*
         *  Okay, if we didn't manage to connect, delay as appropriate,
         *  then try to connect again.
         */
        if (!valid(socket)) {
            ans_connect_errors++;

            /*
             *  If the network setup changes, try to connect right away.
             *  Otherwise, use the normal backoff algorithm.
             */
            if (client->network_changed || ans_force_change) {
                client->network_changed  = VPL_FALSE;
                ans_force_change         = false;
                delay                    = ans_min_delay;
            } else {
                if (client->mt.inited) {
                    jitter = delay / 4;
                    jitter  = mtwist_next(&client->mt, jitter);
                    delay  += jitter;
                }

                if (client->verbose && delay < ans_max_delay) {
                    log_info("I will try again in %ds.", delay);
                }

                sleep_seconds(client, delay);
                delay = MIN(delay * 2, ans_max_delay);
            }
        }
    }

    return socket;
}

/*
 *  Check whether a raw packet is a Reject_credentials packet.  These
 *  packets aren't signed, because sometimes there's no valid session key.
 */
static int
is_blob_rejection(packet_t *packet)
{
    char *  base;
    char *  end;
    int     fail;
    u16     tag_flag;

    if (packet->length < 2 * sizeof(uint16_t)) {
        ans_short_packets++;
        log_warn("The packet was too short (%d)", (int) packet->length);
        return VPL_FALSE;
    }

    base = packet->base;
    end  = base + packet->length;
    fail = false;

    memcpy(&tag_flag, base, sizeof(u16));
    
    if (tag_flag == server_tag_flag) {
        base += sizeof(u16);
    }

    /*
     *  Unpack the header.  This code is duplicated in unpack().
     */
    unpack_u16(check_length);
    unpack_u16(length);

    if (fail || check_length != (uint16_t) ~length) {
        log_warn_1("The check length doesn't match the length.");
        return VPL_FALSE;
    }

    unpack_u16(type);

    return type == Reject_credentials;
}

/*
 *  Validate a signature on a packet.
 */
static int
check_signature(crypto_t *crypto, packet_t *packet)
{
    unsigned char *   received;
    unsigned char *   buffer;
    unsigned char     expected[CSL_SHA1_DIGESTSIZE];
    unsigned int      length;
    CSL_ShaContext    context;

    /*
     *  Credential rejection packets aren't signed.  Check carefully
     *  for such a packet.
     */
    if (is_blob_rejection(packet)) {
        return VPL_TRUE;
    }

    length  = signature_size;
    buffer  = (unsigned char *) packet->buffer;

    /*
     *  Compute the signature.
     */
    CSL_ResetSha (&context);
    CSL_InputSha (&context, crypto->sha1_key, crypto->sha1_length);
    CSL_InputSha (&context, buffer, packet->length - signature_size);
    CSL_ResultSha(&context, expected);

    /*
     *  Compute the address of the signature inside the packet.
     */
    received = buffer + packet->length - signature_size;

    /*
     *  Check the expected signature and what we got.
     */
    return memcmp(expected, received, signature_size) == 0;
}

/*
 *  Encrypt an outgoing field given a count and a length.  This routine
 *  generates the IV.
 */
static void
encrypt_field
(
    ans_client_t *  client,
    char **         encrypted,
    int *           encrypted_count,
    char *          plaintext,
    int             plaintext_count
)
{
    char     buffer[aes_block_size];
    char *   iv;
    int32_t  random[aes_block_size / 4];
    int      i;

    *encrypted = null;
    *encrypted_count = 0;

    iv = &buffer[0];
    memset(buffer, 0, sizeof(buffer));

    for (i = 0; i < array_size(random); i++) {
        random[i] = mtwist_next(&client->mt, -1);
    }

    memcpy(iv, random, min(sizeof(random), sizeof(buffer)));

    ans_encrypt(client, encrypted, encrypted_count, iv, plaintext,
        plaintext_count);

    return;
}

/*
 *  Convert a packet into wire format.  The serial number and signature are
 *  added later.
 */
static packet_t *
pack(ans_client_t *client, describe_t *describe)
{
    char *      body;
    char *      base;
    char *      encrypted;
    void *      data;
    int         encrypted_count;
    uint16_t    length;
    short       req_length;
    uint16_t    type;
    uint16_t    version;
    uint64_t    user_id;
    uint64_t    device_id;
    uint64_t    sequence_id;
    uint64_t    connection;
    uint64_t    async_id;
    uint64_t    tag;
    uint32_t    packed_ioac;
    short       sent_count;
    int         i;
    int         count;
    packet_t *  packet;
    short       ioac;
    int         type_length;
    int         app_length;
    int32_t     class_mask;

    const uint64_t *  devices;
    const uint64_t *  ping_data;
    uint64_t          ping_array[5];

    req_length       = header_size + signature_size;
    user_id          = VPLConv_ntoh_u64(describe->user_id);
    device_id        = VPLConv_ntoh_u64(describe->device_id);
    data             = describe->data;
    sequence_id      = 0;    /* the true sequence id is added later */
    encrypted        = null;
    ioac             = describe->ioac;
    count            = describe->count;
    type_length      = 0;
    app_length       = 0;
    encrypted_count  = 0;

    if (describe->tag != 0) {
        req_length += sizeof(uint64_t) + sizeof(uint16_t);
    }

    /*
     *  Compute the total packet length.
     */
    switch (describe->type) {
    case Send_subscriptions:
        req_length += sizeof(sent_count) + count;
        count /= sizeof(uint64_t);
        break;

    case Send_login_blob:
        type_length = strlen(client->device_type);
        app_length  = strlen(client->application);

        req_length += sizeof(version);
        req_length += sizeof(client->connection);
        req_length += sizeof(sent_count) + count;
        req_length += sizeof(sent_count) + client->challenge_length;
        req_length += sizeof(sent_count) + type_length;
        req_length += sizeof(sent_count) + app_length;
        break;

    case Send_unicast:
        req_length += sizeof(device_id);
        req_length += sizeof(sent_count) + count;
        break;

    case Send_multicast:
        req_length += sizeof(class_mask);
        req_length += sizeof(sent_count) + count;
        break;

    case Send_ping:
    case Send_device_shutdown:
        break;

    case Send_timed_ping:
        if (count != 5 * sizeof(uint64_t)) {
            log_error_1("The count is wrong for a timed ping.");
            return null;
        }

        req_length += sizeof(uint16_t) + count;
        break;

    case Set_login_version:
    case Set_param_version:
        req_length += 2;
        break;

    case Request_wakeup:
        req_length += sizeof(device_id);
        break;

    case Request_sleep_setup:
        if (count <= 0 || data == null) {
            log_error_1("The mac address wasn't specified.");
            return null;
        }

        encrypt_field(client, &encrypted, &encrypted_count,
            (char *) data, count);

        if (encrypted == null) {
            log_error_1("The mac address encryption call failed.");
            return null;
        }

        req_length += sizeof(uint32_t);    // ioac type
        req_length += sizeof(sent_count) + encrypted_count;
        break;

    case Query_device_list:
        req_length += sizeof(length);
        req_length += count;
        count      /= sizeof(device_id);

        if (count <= 0) {
            log_error("The device list length (%d) is invalid.",
                (int) count);
            return null;
        }

        if (count > ans_max_query) {
            log_error("The device list (%d entries) is too long.",
                (int) count);
            return null;
        }

        break;

    default:
        log_error("pack() got an unexpected packet type %d", describe->type);
        return null;
    }

    if (req_length > ans_max_packet_size) {
        log_error("That packet size (%d) is too large",
            (int) req_length);
        return null;
    }

    /*
     *  Allocate space for the packet.
     */
    body = (char *) malloc(req_length);

    if (body == null) {
        log_error_1("malloc failed during pack");
        return null;
    }

    /*
     *  Okay, build the header.  The sequence number is inserted later.
     */
    memset(body, 0, req_length);
    base = body;

    if (describe->tag != 0) {
        length = VPLConv_ntoh_u16(server_tag_flag);
        memcpy(base, &length, sizeof(length));
        base += sizeof(length);
    }

    length = VPLConv_ntoh_u16(~req_length);
    memcpy(base, &length, sizeof(length));
    base += sizeof(length);

    length = VPLConv_ntoh_u16(req_length);
    memcpy(base, &length, sizeof(length));
    base += sizeof(length);

    type = VPLConv_ntoh_u16(describe->type);
    memcpy(base, &type, sizeof(type));
    base += sizeof(type);

    memcpy(base, &user_id, sizeof(user_id));
    base += sizeof(user_id);

    async_id = VPLConv_ntoh_u64(describe->async_id);
    memcpy(base, &async_id, sizeof(async_id));
    base += sizeof(async_id);

    memcpy(base, &sequence_id, sizeof(sequence_id));
    base += sizeof(sequence_id);

    if (describe->tag != 0) {
        tag = VPLConv_ntoh_u64(describe->tag);
        memcpy(base, &tag, sizeof(tag));
        base += sizeof(tag);
    }

    /*
     *  Now append the variable fields.
     */
    switch (describe->type) {
    case Send_subscriptions:
        sent_count = VPLConv_ntoh_u16(count);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        devices = (const uint64_t *) (data);

        for (i = 0; i < count; i++) {
            device_id = VPLConv_ntoh_u64(devices[i]);
            memcpy(base, &device_id, sizeof(device_id));
            base += sizeof(device_id);
        }

        for (i = 0; i < count; i++) {
            log_info("send subscriptions[%d] = " FMTu64,
                i, devices[i]);
        }

        break;

    case Set_login_version:
    case Set_param_version:
        version = (short) describe->version;
        version = VPLConv_ntoh_u16(version);
        memcpy(base, &version, sizeof(version));
        base += sizeof(version);
        break;

    case Request_sleep_setup:
        packed_ioac = VPLConv_ntoh_u32(ioac);
        memcpy(base, &packed_ioac, sizeof(packed_ioac));
        base += sizeof(packed_ioac);

        sent_count = VPLConv_ntoh_u16(encrypted_count);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        memcpy(base, encrypted, encrypted_count);
        base += encrypted_count;

        free(encrypted);
        break;

    case Send_login_blob:
        /*
         *  Pack the connection id.
         */
        version = VPLConv_ntoh_u16(ans_login_version);
        memcpy(base, &version, sizeof(version));
        base += sizeof(version);

        connection = VPLConv_ntoh_u64(client->connection);
        memcpy(base, &connection, sizeof(connection));
        base += sizeof(connection);

        /*
         *  Pack the challenge.
         */
        sent_count = VPLConv_ntoh_u16(client->challenge_length);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        memcpy(base, client->challenge, client->challenge_length);
        base += client->challenge_length;

        /*
         *  Pack the blob.
         */
        sent_count = VPLConv_ntoh_u16(count);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        memcpy(base, data, count);
        base += count;

        sent_count = VPLConv_ntoh_u16(type_length);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        memcpy(base, client->device_type, type_length);
        base += type_length;

        sent_count = VPLConv_ntoh_u16(app_length);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        memcpy(base, client->application, app_length);
        base += app_length;

        break;

    case Request_wakeup:
        memcpy(base, &device_id, sizeof(device_id));
        base += sizeof(device_id);
        break;

    case Query_device_list:
        devices = static_cast<const uint64_t *>(data);
        length  = VPLConv_ntoh_u16(count);

        memcpy(base, &length, sizeof(length));
        base += sizeof(length);

        for (i = 0; i < count; i++) {
            if (client->verbose) {
                log_info("query device " FMTu64, devices[i]);
            }

            device_id = VPLConv_ntoh_u64(devices[i]);
            memcpy(base, &device_id, sizeof(device_id));
            base += sizeof(device_id);
        }

        break;

    case Send_unicast:
        memcpy(base, &device_id, sizeof(device_id));
        base += sizeof(device_id);

        sent_count = VPLConv_ntoh_u16(count);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        memcpy(base, data, count);
        base += count;
        break;

    case Send_multicast:
        class_mask = VPLConv_ntoh_u32(-1);  // not used
        memcpy(base, &class_mask, sizeof(class_mask));
        base += sizeof(class_mask);

        sent_count = VPLConv_ntoh_u16(count);
        memcpy(base, &sent_count, sizeof(sent_count));
        base += sizeof(sent_count);

        memcpy(base, data, count);
        base += count;
        break;

    case Send_timed_ping:
        length = VPLConv_ntoh_u16(count / sizeof(uint64_t));
        memcpy(base, &length, sizeof(length));
        base += sizeof(length);

        ping_data = (uint64_t *) data;

        ping_array[0] = VPLConv_ntoh_u64(ping_data[0]);
        ping_array[1] = VPLConv_ntoh_u64(ping_data[1]);
        ping_array[2] = VPLConv_ntoh_u64(ping_data[2]);
        ping_array[3] = VPLConv_ntoh_u64(ping_data[3]);
        ping_array[4] = VPLConv_ntoh_u64(ping_data[4]);

        memcpy(base, ping_array, count);
        base += count;
        break;
    }

    /*
     *  Add the signature size to our offset.  The signature itself is
     *  inserted later.
     */
    memset(base, 0, signature_size);
    base += signature_size;

    /*
     *  Check that the packet is properly formed.
     */
    if (base - body != req_length || ans_fail_pack) {
        free(body);
        log_error("The packet size (type %s, %d) doesn't match the expected size.",
            packet_type(describe->type), describe->type);
        return null;
    }

    /*
     *  Okay, we got this far, so allocate the control data for the packet.
     */
    packet = alloc_packet(body, req_length);

    if (packet == null) {
        log_error_1("The packet malloc failed.");
        free(body);
        return null;
    }

    packet->describe.type     = describe->type;
    packet->describe.async_id = describe->async_id;
    return packet;
}

/*
 *  Queue an output packet for the TCP socket.  If the client is being
 *  stopped, discard the packet instead.  Ping the device thread when
 *  done, so that it checks the output queue.
 */
static int
queue_packet(ans_client_t *client, packet_t *packet, const char *where)
{
    packet->next   = null;
    packet->where  = where;

    mutex_lock(&client->mutex);

    if (client->stop_now) {
        mutex_unlock(&client->mutex);
        free_packet(&packet);
        return false;
    }

    if (client->tcp_head == null) {
        client->tcp_head = packet;
        client->tcp_tail = packet;
    } else {
        client->tcp_tail->next = packet;
        client->tcp_tail = packet;
    }

    mutex_unlock(&client->mutex);
    ping_device(client);
    return VPL_TRUE;
}

/*
 *  Unpack a variable-length byte array from a packet.
 *  The first two bytes specify the length.
 */
static int
unpack_bytes(char *end, char *base, char **array, const char *name)
{
    int  fail;

    if (end - base < sizeof(uint16_t)) {
        log_error("The packet has no room for the %s (length)", name);
        return -1;
    }

    fail = false;
    unpack_u16(length);

    if (end - base < length || fail) {
        log_error("The packet has no room for the %s", name);
        return -1;
    }

    *array = base;
    return length;
}

/*
 *  Unpack an encrypted field from a packet.  The first two bytes specify
 *  the encrypted length.  The encrypted string contains the length of the
 *  actual field value.  The return value is the address of the byte just
 *  beyond the end of the ciphertext.  It is used to continue parsing the
 *  packet, which can contain more data.
 */
static char *
unpack_encrypted
(
    ans_client_t *  client,
    char *          end,
    char *          base,
    char **         array,
    int *           array_size,
    const char *    name
)
{
    int     success;
    char *  ciphertext;
    int     ciphertext_count;

    ciphertext_count = unpack_bytes(end, base, &ciphertext, name);

    if (ciphertext_count <= 0) {
        return null;
    }

    success = ans_decrypt(client, array,
                 array_size, ciphertext, ciphertext_count);

    if (!success) {
        log_error("ans_decrypt failed on the %s field", name);
        return null;
    }

    return ciphertext + ciphertext_count;
}

#define device_state(response)  \
            ((response) > max_device_state ? DEVICE_SLEEPING : (response))

/*
 *  Free an unpacked packet control structure and any allocate memory
 *  areas related to it.
 */
static void
free_unpacked(ans_unpacked_t **unpacked)
{
    if (*unpacked != null) {
        free((*unpacked)->sleepDns);
        free((*unpacked)->deviceList);
        free((*unpacked)->deviceStates);
        free((*unpacked)->deviceTimes);
        free((*unpacked)->plainSleep);
        free((*unpacked)->plainKey);

        if
        (
            (*unpacked)->type == SEND_SLEEP_SETUP
        ||  (*unpacked)->type == Send_sleep_setup
        ) {
            free((*unpacked)->sleepPacket);
            free((*unpacked)->wakeupKey);
            free((*unpacked)->macAddress);
        }

        free(*unpacked);
        *unpacked = null;
    }
}

/*
 *  Convert a device state to a string.
 */
static const char *
state_to_string(char state)
{
    const char *  result;

    switch (state)
    {
    case DEVICE_ONLINE:
        result = "online";
        break;

    case DEVICE_SLEEPING:
        result = "sleeping";
        break;

    case DEVICE_OFFLINE:
        result = "offline";
        break;

    case QUERY_FAILED:
        result = "query failed";
        break;

    default:
        result = "unknown";
        break;
    }

    return result;
}

static void
log_device_state
(
    ans_client_t *  client,
    uint64_t        deviceId,
    char            state,
    uint64_t        sleepTime
)
{
    log_info("device " FMTu64 ":  %s (%d) @ " FMTu64 ", local device " FMTu64,
        deviceId, state_to_string(state), state, sleepTime, client->device_id);
}

/*
 *  Send a message to the ANS server.  This routine saves the packet
 *  description values, which likely reside on the stack.
 */
static int
send_message(ans_client_t *client, describe_t *describe)
{
    packet_t *  packet;

    packet = alloc_packet(null, 0);

    if (packet == null) {
        return VPL_FALSE;
    }

    packet->describe = *describe;

    if (describe->data != null) {
        packet->describe.data = malloc(describe->count);

        if (packet->describe.data == null) {
            log_error_1("malloc failed in send_message");
            free(packet);
            return VPL_FALSE;
        }

        memcpy(packet->describe.data, describe->data, describe->count);
    }

    return queue_packet(client, packet, "send_message");
}

/*
 *  Generate a keep-alive packet.  The sequence number is
 *  inserted later, as is the signature.  This routine is
 *  called both for UDP and TCP keep-alive packets.
 */
static packet_t *
make_keep_packet(keep_t *keep)
{
    int         type;
    int         count;
    packet_t *  packet;
    uint64_t    data[5];
    describe_t  describe;

    type     = Send_timed_ping;
    count    = sizeof(data);
    data[0]  = VPLTime_GetTime();
    data[1]  = keep->rtt_estimate / keep->time_to_millis;
    data[2]  = keep->client->connection;
    data[3]  = keep->client->interval / VPLTime_FromSec(1);
    data[4]  = keep->client->generation;

    clear_describe(&describe);

    describe.type  = type;
    describe.count = count;
    describe.data  = data;
    describe.tag   = keep->client->tag;

    packet = pack(null, &describe);

    return packet;  /* might be null */
}

/*
 *  Send a TCP ping to the ANS server.  The sequence number and signature
 *  are by the TCP queue handling.
 */
static int
send_tcp_ping(keep_t *keep, const char *where)
{
    packet_t *  packet;

    packet = make_keep_packet(keep);

    if (packet == null) {
        return false;
    }

    log_test("queueing a TCP ping for client %p at %s", keep->client, where);

    queue_packet(keep->client, packet, where);
    return true;
}

/*
 *  Send the subscriptions for a client to the ANS server.  The
 *  subscriptions are set by CCD, but are saved when a socket fails.
 *  When a new connection is established, the subscriptions are
 *  reinstated.
 */
static void
send_subscriptions(ans_client_t *client)
{
    int  bytes;
    int  count;

    uint64_t *  subscriptions;
    describe_t  describe;

    mutex_lock(&client->mutex);

    if (client->subscriptions == null) {
        mutex_unlock(&client->mutex);
        log_info_1("This client has no device subscriptions");
        return;
    }

    /*
     *  If the subscriptions count is too large, tell the client.  The limit
     *  can change at any time.
     */
    if (client->subscription_count > ans_max_subs) {
        mutex_unlock(&client->mutex);
        free(client->subscriptions);
        client->subscriptions = null;
        log_error("The subscription list is too large (%d).",
            (int) client->subscription_count);
        client->callback_table.rejectSubscriptions(client);
        return;
    }

    /*
     *  Make a copy of the subscriptions.  This copy is used when the packet
     *  is created.  We release the lock on the subscriptions before doing
     *  that step.
     */
    count = client->subscription_count;
    bytes = count * sizeof(subscriptions[0]);
    subscriptions = (uint64_t *) malloc(bytes);

    if (subscriptions == null) {
        mutex_unlock(&client->mutex);
        log_error_1("send_subscriptions:  malloc failed");
        return;
    }

    memcpy(subscriptions, client->subscriptions, bytes);
    mutex_unlock(&client->mutex);

    /*
     *  Queue the packet and try to write any output that's needed.
     */
    clear_describe(&describe);
    describe.type  = Send_subscriptions;
    describe.data  = subscriptions;
    describe.count = bytes;

    send_message(client, &describe);

    write_output(client);
    free(subscriptions);
    return;
}

/*
 *  Do the processing required when a login to the ANS server is completed.
 */
static void
declare_online(ans_client_t *client)
{
    if (!client->login_completed) {
        log_info("login completed for client %p", client);
        client->login_completed = VPL_TRUE;
        client->callback_table.loginCompleted(client);

        /*
         *  Send the subscription list, if there is one.
         */
        send_subscriptions(client);

        /*
         *  Tell the keep-alive code about our new socket.
         */
        mutex_lock(&client->keep.mutex);
        client->keep.live_socket_id   = client->next_socket_id++;
        client->keep.dead_socket_id   = -1;
        client->keep.declared_dead    = false;
        mutex_unlock(&client->keep.mutex);
        ping_keep_alive(client);

        if (client->interval > 0) {
            send_tcp_ping(&client->keep, "declare_online");
        }
    }
}

static void
print_external(ans_client_t *client)
{
    char    dotted_quad[512];
    char *  buffer;
    int     remaining;
    int     count;

    buffer     = dotted_quad;
    remaining  = sizeof(dotted_quad) - 1;

    for (int i = 0; i < client->external_count; i++) {
        if (i != 0 && remaining > 0) {
            remaining--;
            *buffer++ = '.';
        }

        count = snprintf(buffer, remaining, "%d",
            client->external_ip[i] & 0xff);

        if (count <= 0) {
            break;
        }

        buffer    += count;
        remaining -= count;
    }

    dotted_quad[sizeof(dotted_quad) - 1] = 0;

    log_info("This client's external address is %s:%d",
        dotted_quad,
        client->external_port);
}

/*
 *  Unpack a packet from wire format.  The caller should
 *  validate the signature, if there is one.
 */
static ans_unpacked_t *
unpack(ans_client_t * client, packet_t *packet, int64_t *expectedId)
{
    int       remaining;
    int       i;
    uint64_t  device_state;
    char *    base;
    char *    end;
    char *    notification;
    char *    sleep_packet;
    int       sleep_length;
    char *    wakeup_key;
    int       wakeup_length;
    char *    ping_packet;
    char *    host_name;
    int       ping_length;
    int       host_length;
    char *    ip_name;
    int       ip_length;
    char *    challenge;
    char *    mac_address;
    int       mac_count;
    char *    bytes;
    int       bytes_length;
    char *    string;
    char *    byte_param;
    int       string_length;
    uint16_t  challenge_count;
    uint64_t  challenge_time;
    uint64_t  tag;
    uint32_t  param_upper;
    uint32_t  param_lower;
    char      time_buffer[200];
    uint64_t  seed;
    uint32_t  time;
    uint16_t  tag_flag;
    int       fail;
    int       has_tag;

    describe_t  describe;

    ans_unpacked_t *        result;
    VPLTime_CalendarTime_t  vplex_time;

    /*
     *  Create a control structure for the packet.
     */
    result = (ans_unpacked_t *) malloc(sizeof(*result));

    if (result == null) {
        log_error_1("malloc failed during unpack.");
        return null;
    }

    memset(result, 0, sizeof(*result));

    result->deviceList   = null;
    result->deviceStates = null;
    result->deviceTimes  = null;
    result->sleepDns     = null;

    /*
     *  Point the control structure at the actual data from the network.
     */
    base = packet->buffer;
    end  = base + packet->length;
    fail = false;

    /*
     *  Make sure that it's long enough to be valid.
     */
    if (packet->length < header_size) {
        log_error("I got a bad packet:  "
            "packet->length (%d) < header_size (%d)",
            packet->length, header_size);
        free_unpacked(&result);
        return null;
    }

    /*
     *  Unpack the header.
     */
    memcpy(&tag_flag, base, sizeof(tag_flag));

    has_tag = tag_flag == server_tag_flag;  // TODO remove

    if (has_tag) {
        base += sizeof(tag_flag);
    }
        
    unpack_u16(check_length);
    unpack_u16(length);

    if ((uint16_t) ~check_length != length) {
        log_error("unpack got a check size mismatch:  %d vs %d",
            ~check_length, length);
        free_unpacked(&result);
        return null;
    }

    unpack_u16(type);
    unpack_u64(user_id);
    unpack_u64(async_id);
    unpack_u64(sequence_id);

    if (has_tag) {
        memcpy(&tag, base, sizeof(tag));
        tag = VPLConv_ntoh_u64(tag);
        base += sizeof(tag);
    } else {
        tag = 0;
    }

    /*
     *  Set the fields we pass to the user or have parsed.
     */
    result->type          = type;
    result->userId        = user_id;
    result->asyncId       = async_id;
    result->sleepDns      = null;
    result->deviceStates  = null;
    result->notification  = null;
    result->tag           = tag;

    /*
     *  Check that the sequence number is correct, if this packet
     *  type has a sequence number.  If the sequence number is okay,
     *  increment the counter.
     */
    if (expectedId != null) {
        if (sequence_id != *expectedId) {
            log_error("I was expecting sequence number " FMTu64
                ", but got " FMTu64 ".", *expectedId, sequence_id);
            free_unpacked(&result);
            return null;
        }

        (*expectedId)++;
    }

    /*
     *  Check whether the type is one that we can decode.
     */
    switch (type) {
    case Send_unicast:
    case Send_multicast:
    case Set_device_params:
    case Send_ping:
    case Send_challenge:
    case Reject_credentials:
    case Send_state_list:
    case Send_sleep_setup:
    case Send_device_update:
    case Send_response:
        if (client == null) {
            free_unpacked(&result);
            return null;
        }

        break;

    case Send_login_blob:
    case Send_timed_ping:
        break;

    default:
        log_error("I got an unknown packet type %d - version mismatch?",
            (int) type);
        result->type = Send_ping;
        return result;
    }

    /*
     *  Start unpacking the type-specific fields.  The message buffer for
     *  a notification is unpacked later.
     */
    switch (type) {
    case Send_unicast:
        if (end - base < sizeof(uint64_t) + signature_size) {
            log_error_1("The packet doesn't contain a device id.");
            free_unpacked(&result);
            return null;
        }

        unpack_u64(device_id);
        result->userId   = user_id;
        result->deviceId = device_id;
        break;

    case Send_multicast:
        if (end - base < sizeof(uint32_t) + signature_size) {
            log_error_1("The packet doesn't contain a class mask.");
            free_unpacked(&result);
            return null;
        }

        unpack_u32(classes); /* not used anymore */
        result->userId = user_id;
        break;

    case Send_sleep_setup:
        if (end - base < sizeof(uint32_t) + signature_size) {
            log_error_1("no ioac type");
            free_unpacked(&result);
            return null;
        }

        /*
         *  Unpack the ioac type.
         */
        unpack_u32(ioac_type);

        /*
         *  Unpack the mac address.
         */
        base = unpack_encrypted(client, end, base, &mac_address,
                    &mac_count, "mac address");

        if (base == null) {
            free_unpacked(&result);
            return null;
        }

        result->ioacType         = ioac_type;
        result->macAddress       = mac_address;
        result->macAddressLength = mac_count;

        /*
         *  Unpack the sleep packet.
         */
        base = unpack_encrypted(client, end, base, &sleep_packet,
                    &sleep_length, "sleep packet");

        if (base == null) {
            free_unpacked(&result);
            return null;
        }

        result->sleepPacket = sleep_packet;
        result->sleepPacketLength = sleep_length;

        /*
         *  Now get the wakeup key.
         */
        base = unpack_encrypted(client, end, base, &wakeup_key,
                    &wakeup_length, "wakeup key");

        if (base == null) {
            free_unpacked(&result);
            return null;
        }

        result->wakeupKey        = wakeup_key;
        result->wakeupKeyLength  = wakeup_length;

        /*
         *  The last variable-length field is the ip name.
         */
        ip_length = unpack_bytes(end, base, &ip_name, "sleep ip");

        if (ip_length <= 0) {
            free_unpacked(&result);
            return null;
        }

        base = ip_name + ip_length;
        result->sleepDns = (char *) malloc(ip_length + 1);

        if (result->sleepDns == null) {
            log_error_1("The malloc for the DNS name failed.");
            free_unpacked(&result);
            return null;
        }

        memcpy(result->sleepDns, ip_name, ip_length);
        result->sleepDns[ip_length] = 0;

        /*
         *  Finally, get the port and sleep interval.
         */
        unpack_u32(sleep_setup_port);
        result->sleepPort = sleep_setup_port;

        unpack_u32(sleep_setup_interval);
        result->sleepPacketInterval = sleep_setup_interval;

        break;

    case Send_challenge:
        if (end - base < sizeof(uint64_t) + 2 * sizeof(uint16_t)) {
            log_error_1("The challenge packet is incomplete.");
            free_unpacked(&result);
            return null;
        }

        /*
         *  Reset the server flags to the default.
         */
        client->server_flags = server_flag_new_ping;

        unpack_u16(challenge_type);

        if (challenge_type == ans_challenge_version) {
            unpack_u16(count);
            challenge_count = count;

            if (end - base < count * sizeof(uint64_t) || count < 2) {
                log_error_1("The challenge parameters are incomplete.");
                free_unpacked(&result);
                return null;
            }
        } else {
            challenge_count = 0;    /* keep gcc happy */
        }

        unpack_u64(connection);

        if (challenge_type == ans_challenge_version) {
            unpack_u64(time);
            challenge_time = time;

            VPLTime_ConvertToCalendarTimeUniversal
            (
                VPLTime_FromMillisec(challenge_time), &vplex_time
            );

            log_always("got server time " date_format,
                vplex_time.year,
                vplex_time.month,
                vplex_time.day,
                vplex_time.hour,
                vplex_time.min,
                vplex_time.sec,
                vplex_time.msec);

            challenge_count -= 2;

            if (challenge_count > 0) {
                unpack_u64(server_flags);
                client->server_flags = server_flags;
                log_always("got server flags 0x" FMTx64, server_flags);
                challenge_count--;
            }

            while (challenge_count-- > 0) {
                unpack_u64(challenge_drop);
            }
        }

        client->connection = connection;
        log_always("This socket is ANS connection " FMTu64, connection);

        free(client->challenge);
        client->challenge = null;

        client->challenge_length = unpack_bytes(end, base, &challenge, "challenge");

        if (client->challenge_length <= 0) {
            free_unpacked(&result);
            return null;
        }

        client->challenge = (char *) malloc(client->challenge_length);

        if (client->challenge == null) {
            log_error_1("The malloc for the challenge buffer failed.");
            free_unpacked(&result);
            return null;
        }

        memcpy(client->challenge, challenge, client->challenge_length);
        base = challenge + client->challenge_length;

        /*
         *  TODO remove
         */
        if (result->tag == 0) {
            result->tag = VPLTime_GetTime() / VPLTime_FromMillisec(1);
        }

        break;

    case Set_device_params:
        /*
         *  The packet must have a version, a parameter count, and a string
         *  count, each of which are shorts.
         */
        if (end - base < 3 * sizeof(uint16_t) + signature_size) {
            log_error_1("The device configuration is incomplete "
                "-- version mismatch?");
            free_unpacked(&result);
            return null;
        }

        unpack_u16(version);

        if (version != ans_device_version) {
            log_info("device parameters type mismatch %d vs %d",
                (int) version, (int) ans_device_version);

            clear_describe(&describe);
            describe.type    = Set_param_version;
            describe.version = ans_device_version;
            send_message(client, &describe);
            write_output(client);

            result->type = Send_ping;
            return result;
        }

        unpack_u16(param_count);

        if (param_count < 24) {
            log_warn("short device parameter count:  %d", (int) param_count);
            result->type = Send_ping;
            return result;
        }

        /*
         *  The packet has a reasonable parameter count, but it must have
         *  have a string count after them.
         */
        if (end - base < param_count * sizeof(int32_t) + sizeof(uint16_t)) {
            log_warn_1("short device parameter packet (string)");
            result->type = Send_ping;
            return result;
        }

        /*
         *  Unpack all the parameters.
         */
        unpack_param(ans_print_interval);
        unpack_param(ans_max_subs);
        unpack_param(ans_max_packet_size);
        unpack_param(ans_max_query);
        unpack_param(ans_partial_timeout);
        unpack_param(ans_min_delay);
        unpack_param(ans_max_delay);

        mutex_lock(&client->keep.mutex);
        unpack_param(ans_base_interval);
        unpack_param(ans_retry_interval);
        unpack_param(ans_retry_count);
        mutex_unlock(&client->keep.mutex);

        unpack_param(ans_tcp_ka_time);
        unpack_param(ans_tcp_ka_interval);
        unpack_param(ans_tcp_ka_probes);

        /*
         *  Apply some sanity rules to the delay parameters.
         */
        ans_min_delay = MAX(   1, ans_min_delay);
        ans_min_delay = MIN(  60, ans_min_delay);

        ans_max_delay = MAX(  10, ans_max_delay);
        ans_max_delay = MIN(3600, ans_max_delay);

        /*
         *  Clear the newer parameters.
         */
        client->server_time  = 0;
        client->device_id    = 0;
        client->generation   = 0;

        /*
         *  If this package has the device id, unpack it.
         */
        unpack_param_64(device_id);

        /*
         *  Unpack more software keep-alive parameters.
         */
        unpack_param(ans_ping_back);
        unpack_param(ans_ping_factor);

        /*
         *  Unpack the server generation id and the server time
         *  when the packet was created.
         */
        unpack_param_64(generation);
        unpack_param_64(server_time);
        client->server_time *= 1000;

        unpack_param(client->keep_port);
        unpack_param(ans_jitter);
        unpack_param(client->external_port);

        param_count -= 24;

        if (param_count > 0) {
            unpack_param(client->enable_tcp_only);
            param_count--;
        }

        /*
         *  Skip past any parameters that are beyond our knowledge...
         */
        while (param_count-- > 0) {
           base += sizeof(int32_t);
        }

        /*
         *  Now get the number of strings.
         */
        unpack_u16(string_count);

        if (string_count < 3) {
            log_warn_1("short string count");
            result->type = Send_ping;
            return result;
        }

        /*
         *  Pull the UDP ping packet from the configuration.
         */
        ping_length = unpack_bytes(end, base, &ping_packet, "ping packet");

        if (ping_length < 0) {
            log_warn_1("short ping packet");
            result->type = Send_ping;
            return result;
        }

        base = ping_packet + ping_length;

        free(client->host_name);
        client->host_name = null;

        /*
         *  If there's a host name in the packet, pull it out and save it.
         */
        host_length = unpack_bytes(end, base, &host_name, "server DNS");

        if (host_length > 0) {
            client->host_name = (char *) malloc(host_length + 1);

            if (client->host_name != null) {
                memcpy(client->host_name, host_name, host_length);
                client->host_name[host_length] = 0;
            }

            base = host_name + host_length;
        }

        log_always("The server is %s, generation " FMTu64 ", cluster %s",
            client->host_name != null ? client->host_name : "<unknown>",
            client->generation,
            client->cluster);

        log_always("The server time is %s",
            print_time(client->server_time, time_buffer, sizeof(time_buffer)));

        log_always("Connection " FMTu64 " is for device " FMTu64,
            client->connection,
            client->device_id);

        /*
         *  Try to use a slightly better seed now that we have more
         *  data.
         */
        if (!client->login_inited) {
            time  = VPLTime_GetTime();
            seed  = client->device_id  ^ (client->device_id  >> 15);
            seed ^= client->connection ^ (client->generation <<  9);
            seed ^= time               ^ (time               << 16);
            seed ^= seed >> 32;

            client->login_inited = true;
            mtwist_init(&client->mt, seed);
        }

        bytes_length = unpack_bytes(end, base, &bytes, "external address");

        free(client->external_ip);
        client->external_ip = null;

        if (bytes_length > 0) {
            client->external_ip     = (char *) malloc(bytes_length);
            client->external_count  = bytes_length;

            if (client->external_ip != null) {
                memcpy(client->external_ip, bytes, bytes_length);
            }

            print_external(client);

            base = bytes + bytes_length;
        }

        string_count -= 3;

        /*
         *  Skip past any strings beyond our knowledge.
         */
        while (string_count-- > 0) {
            string_length = unpack_bytes(end, base, &string, "extra string");

            if (string_length < 0) {
                log_warn_1("short extra string");
                result->type = Send_ping;
                return result;
            }

            base = string + string_length;
        }

        /*
         *  Now set various values based on the parameters from the packet.
         */
        ans_set_keep_alive(client->socket);
        ping_keep_alive(client);

        client->callback_table.setPingPacket(client, ping_packet, ping_length);

        /*
         *  Tell CCD that login is successful, and dequeue any dependent
         *  server requests.
         */
        declare_online(client);
        break;

    case Send_state_list:
        if (end - base < sizeof(uint16_t) + signature_size) {
            log_error_1("The packet doesn't contain the list length.");
            free_unpacked(&result);
            return null;
        }

        unpack_u16(list_length);

        result->deviceCount = list_length;

        result->deviceList   =
            (uint64_t *) malloc(list_length * sizeof(device_id));

        result->deviceStates =
            (char *) malloc(list_length);

        result->deviceTimes  =
            (uint64_t *) malloc(list_length * sizeof(device_state));

        if
        (
            result->deviceList   == null
        ||  result->deviceStates == null
        ||  result->deviceTimes  == null
        ) {
            log_error_1("The mallocs for the device information failed.");
            free_unpacked(&result);
            return null;
        }

        if (end - base < list_length * state_size) {
            log_error_1("The packet doesn't contain the results.");
            free_unpacked(&result);
            return null;
        }

        for (i = 0; i < list_length; i++) {
            unpack_u64(device_id);
            unpack_u64(state);
            unpack_u8 (wakeup_requested);

            result->deviceList[i]   = device_id;
            result->deviceStates[i] = device_state(state);

            if
            (
                result->deviceStates[i] != DEVICE_SLEEPING
            ||  result->deviceTimes[i] == DEVICE_SLEEPING
            ) {
                result->deviceTimes[i] = 0;
            }
        }

        break;

    case Send_device_update:
        if (end - base < 2 * sizeof(uint64_t) + signature_size) {
            log_error_1("The device information is not present.");
            free_unpacked(&result);
            return null;
        }

        unpack_u64(update_device);
        unpack_u64(device_state);
        result->deviceId = update_device;
        result->newDeviceState = device_state(device_state);

        if
        (
            result->newDeviceState == DEVICE_SLEEPING
        &&  device_state != DEVICE_SLEEPING
        ) {
            result->newDeviceTime = device_state;
        } else {
            result->newDeviceTime = 0;
        }

        log_device_state(client, result->deviceId, result->newDeviceState,
            result->newDeviceTime);
        break;

    case Send_ping:
        if (client->verbose) {
            log_info("received a ping at " FMTu64, VPLTime_GetTime());
        }

        break;

    case Send_login_blob:
        unpack_u16(login_version);
        unpack_u64(connection_id);

        length = unpack_bytes(end, base, &byte_param, "challenge");
        base   = byte_param + length;

        length = unpack_bytes(end, base, &byte_param, "message");
        base   = byte_param + length;

        length = unpack_bytes(end, base, &byte_param, "device type");
        base   = byte_param + length;

        length = unpack_bytes(end, base, &byte_param, "application");
        base   = byte_param + length;
        break;

    case Send_timed_ping:
        if (end - base < sizeof(uint16_t) + 4 * sizeof(uint64_t) + signature_size) {
            log_error_1("The ping parameters are not present.");
            free_unpacked(&result);
            return null;
        }

        unpack_u16(ping_count);

        if (end - base < ping_count * sizeof(uint64_t)) {
            log_error_1("The ping parameters don't match the count.");
            free_unpacked(&result);
            return null;
        }

        if (ping_count < 4) {
            log_error("The ping parameter count (%d) is not valid.", ping_count);
            free_unpacked(&result);
            return null;
        }

        unpack_u64(send_time);
        unpack_u64(rtt);
        unpack_u64(ping_connection);
        unpack_u64(back_time);

        ping_count -= 4;

        while (ping_count-- > 0) {
            unpack_u64(discard);
        }

        result->sequenceId = sequence_id;
        result->sendTime   = send_time;
        result->rtt        = rtt;
        result->connection = ping_connection;

#ifdef ans_test
        if (client != null) {
            log_info("received TCP ping " FMTs64, result->sequenceId);
        }
#endif

        break;

    case Send_response:
        if (end - base < sizeof(uint64_t) + signature_size) {
            log_error_1("The response is not present.");
            free_unpacked(&result);
            return null;
        }

        unpack_u64(op_result);
        log_info("op id " FMTu64 " returned \"%s\" (" FMTu64 ")",
            result->asyncId, response_string(op_result), op_result);
        result->response = op_result;
        break;
    }

    /*
     *  If this package has a notification in it, unpack the message contents.
     */
    switch (type) {
    case Send_unicast:
    case Send_multicast:
        notification = null;

        result->notificationLength =
            unpack_bytes(end, base, &notification, "message");

        if (result->notificationLength < 0) {
            free_unpacked(&result);
            return null;
        }

        base = notification + result->notificationLength;
        result->notification = (char *) notification;
        break;

    default:
        break;
    }

    /*
     *  Check that all that remains in the packet is the signature, if there
     *  is a signature.
     */
    switch (type) {
    case Send_challenge:        /* challenges are unsigned */
        remaining = end - base;
        break;

    case Reject_credentials:    /* as are rejections.      */
        remaining = end - base;
        client->rejected = VPL_TRUE;
        ans_reject_count++;
        break;

    default:
        remaining = end - base - signature_size;
        break;
    }

    if (remaining != 0) {
        free_unpacked(&result);
        log_error("The packet (%s, %d) is the wrong size - leftover %d bytes"
            " - version mismatch?",
                  packet_type(type),
            (int) type,
            (int) remaining);
        return null;
    }

    /*
     *  If we get any kind of valid packet, reset the TCP timeout clock.
     */
    if (client != null) {
        client->keep.last_tcp_receive = VPLTime_GetTime();
    }

    /*
     *  Log TCP packets except for keep-alives.  The keep-alives
     *  are too frequent, and would keep the disk from spinning
     *  down, due to the constant output.
     */
    if (client != null) {
        client->packets_in++;

        if (result->type != Send_timed_ping) {
            log_info("got packet " FMTu64
                " (%s, %d, %d bytes) for device " FMTu64 ".",
                      async_id,
                      packet_type(result->type),
                (int) result->type,
                (int) packet->length,
                      client->device_id);
        }
    }

    return result;
}

#if defined(ans_send_ping)
static VPL_BOOL
send_ping(ans_client_t *client)
{
    packet_t *  packet;
    int         success;
    describe_t  describe;

    clear_describe(&describe);
    describe.type = Send_ping;
    describe.tag  = client->tag;

    packet = pack(client, &describe);

    if (packet == null) {
        return VPL_FALSE;
    }

    success = queue_packet(client, packet, "old ping");
    return success;
}
#endif

/*
 *  Read from the TCP socket.  Convert EOF to an error to
 *  close the socket.  Ignore VPL_ERR_AGAIN and VPL_ERR_INTR
 *  error returns.
 */
static int
do_read(ans_client_t *client, void *buffer, int bytes)
{
    int  count;

    count = VPLSocket_Recv(client->socket, buffer, bytes);

    if (count < 0 && count != VPL_ERR_AGAIN && count != VPL_ERR_INTR) {
        log_warn("I/O failed with count %d", (int) count);
    } else if (count == 0) {
        count = -1; // force an error on when the server closes the socket
    } else if (count < 0) {
        count = 0;
    }

    return count;
}

/*
 *  Check whether the keep-alive socket is dead.
 */
static int
check_keep(ans_client_t *client)
{
    int       result;

    mutex_lock(&client->keep.mutex);

    result =
            client->keep.declared_dead
        &&  client->keep.dead_socket_id == client->next_socket_id - 1;

    mutex_unlock(&client->keep.mutex);
    return result;
}

/*
 *  Compute the timeout for a poll on the TCP and event
 *  sockets.  We need to obey the partial packet timeout
 *  limit and the print interval.
 */
static uint64_t
compute_timeout
(
    uint64_t  last_ping,
    uint64_t  last_print,
    int       doing_timeout,
    uint64_t  start_time
)
{
    uint64_t  current_time;
    uint64_t  wait_time;
    uint64_t  interval;
    uint64_t  time_waited;

    current_time = VPLTime_GetTime();
    current_time = max(current_time, start_time);
    wait_time    = VPLTime_FromSec(200000);

    if (ans_print_interval > 0 && current_time >= last_print) {
        interval    = VPLTime_FromSec(ans_print_interval);
        time_waited = current_time - last_print;

        if (time_waited < interval) {
            wait_time = min(wait_time, interval - time_waited);
        } else {
            wait_time = 0;
        }
    }

    if (doing_timeout && ans_partial_timeout > 0) {
        interval    = VPLTime_FromSec(ans_partial_timeout);
        time_waited = current_time - start_time;

        if (time_waited < interval) {
            wait_time = min(wait_time, interval - time_waited);
        } else {
            wait_time = 0;
        }
    }

    /*
     *  Wait times less than 1 ms don't seem to work well on Linux.
     *  They seem to lead to a zero wait time.  100 ms should be okay.
     */
    wait_time = max(wait_time, VPLTime_FromMillisec(100));
    return wait_time;
}

/*
 *  Load the current keep-alive parameters as programmed
 *  by the server.  Convert the time units to VPLTime.
 */
static void
get_limits(keep_t *keep, limits_t *limits)
{
    mutex_lock(&keep->mutex);
    limits->base_delay  = ans_base_interval;
    limits->retry_delay = ans_retry_interval;
    limits->retry_count = ans_retry_count;
    limits->factor      = max(0, ans_ping_factor);
    mutex_unlock(&keep->mutex);

    limits->valid =
            limits->base_delay  > 0
        &&  limits->retry_delay > 0
        &&  limits->retry_count > 0;

    limits->tcp_limit  = 0;   // set the default
    limits->udp_limit  = 0;   // set the default

    /*
     *  Compute the actual delay and timeout limits.  We can't control the
     *  TCP retry schedule, so just make a guess as to what might work.
     */
    if (limits->valid) {
        limits->base_delay   = VPLTime_FromSec(limits->base_delay);
        limits->retry_delay  = VPLTime_FromSec(limits->retry_delay);
        limits->udp_limit    = (limits->retry_count - 1) * limits->retry_delay;

        if (limits->factor > 0) {
            limits->tcp_limit = (limits->base_delay + limits->udp_limit) * (limits->factor + 1);
        }
    }

    log_test("got limits:  %s", limits->valid ? "valid" : "invalid");
}

static void
dump_sends(keep_t *keep, limits_t *limits, const char *why)
{
    int   current;
    int   index;
    int   previous;
    char  time_buffer[512];

    log_warn("    %s (at " FMTs64 ")",
        why,
        VPLTime_GetTime());
    log_warn_1("    UDP pings sent:");
    log_warn_1("           Where           Sequence       When              Interval                When");

    for (current = 0; current < array_size(keep->sends); current++) {
        index  = current + keep->i;
        index %= array_size(keep->sends);

        if (index > 0) {
            previous = index - 1;
        } else {
            previous = array_size(keep->sends) - 1;
        }

        if (keep->sends[index] != 0) {
            if (current != 0 && keep->sends[previous] != 0) {
                log_warn("        %-16.16s   %8d  " FMTs64 "  %8d millis    %s",
                          keep->who  [index],
                    (int) keep->id   [index],
                          keep->sends[index],
                    (int) ((keep->sends[index] - keep->sends[previous]) / VPLTime_FromMillisec(1)),
                          print_time(keep->sends[index], time_buffer, sizeof(time_buffer)));
            } else {
                log_warn("        %-16.16s   %8d  " FMTs64 "                     %s",
                          keep->who  [index],
                    (int) keep->id   [index],
                          keep->sends[index],
                          print_time(keep->sends[index], time_buffer, sizeof(time_buffer)));
            }
        }
    }

    log_warn("    base " FMTs64 ", retry " FMTs64 ", retries %d",
        limits->base_delay,
        limits->retry_delay,
        limits->retry_count);
    log_warn("    TCP limit " FMTs64 ", rtt estimate " FMTs64,
        limits->tcp_limit,
        keep->rtt_estimate);
}

static void
dump_receives(keep_t *keep)
{
    int   i;
    char  time_buffer[512];

    log_warn_1("    UDP pings received:");
    log_warn_1("       Sequence        When            RTT              When");

    for (i = 0; i < array_size(keep->ping_rtt); i++) {
        if (keep->ping_when[i] != 0) {
            log_warn("       %8d  " FMTs64 "   %6d   %s",
                (int) keep->ping_id  [i],
                      keep->ping_when[i],
                (int) keep->ping_rtt [i],
                      print_time(keep->ping_when[i], time_buffer, sizeof(time_buffer)));
        }
    }

    log_warn("    Last TCP ping or reset:  " FMTu64 " (%s)",
        keep->last_tcp_receive,
        print_time(keep->last_tcp_receive, time_buffer, sizeof(time_buffer)));
}

/*
 *  Loop doing I/O on the TCP socket.  This routine is passed a buffer
 *  and a count, and returns only when the buffer is full, or some sort
 *  of error or shutdown occurs.
 */
static int
io_loop(ans_client_t *client, void *buffer_in, int expected, int reading_initial)
{
    int        count;
    int        total;
    VPLTime_t  timeout;
    VPLTime_t  last_ping;
    VPLTime_t  last_print;
    VPLTime_t  start_time;
    VPLTime_t  current_time;
    VPLTime_t  print_interval;
    limits_t   limits;
    VPLTime_t  partial_timeout;
    int        failed;
    int        result;
    int        do_timeout;
    char *     buffer;

    VPLSocket_poll_t  poll[2];

    buffer          = (char *) buffer_in;
    total           = 0;
    timeout         = VPLTime_FromSec(1);
    start_time      = VPLTime_GetTime();
    last_ping       = start_time;
    last_print      = start_time;
    partial_timeout = VPLTime_FromSec(ans_partial_timeout);

    do {
        do_timeout      = (!(reading_initial && total == 0) || !client->login_completed);
        timeout         = compute_timeout(last_ping, last_print, do_timeout, start_time);

        poll[0].socket  = client->socket;
        poll[0].events  = VPLSOCKET_POLL_RDNORM;
        poll[0].revents = 0;

        poll[1].socket  = client->event.socket;
        poll[1].events  = VPLSOCKET_POLL_RDNORM;
        poll[1].revents = 0;

        /*
         *  Check whether we have a packet to send, and whether
         *  it's valid to try to send one.  If we've received a
         *  packet, the server is ready to receive them. Otherwise,
         *  maybe not.
         */
        if
        (
            client->tcp_head != null
        &&  client->crypto != null
        &&  client->ready_to_send
        ) {
            poll[0].events |= VPLSOCKET_POLL_OUT;
        }

        /*
         *  Do the actual poll, then check the event structure and
         *  the output situation.
         */
        result = VPLSocket_Poll(poll, array_size(poll), timeout);

        check_event(&client->event, result, &poll[1]);

        if (poll[0].revents & VPLSOCKET_POLL_OUT) {
            write_output(client);
        }

        /*
         *  Check whether the poll operation failed, and quit if it did.
         */
        failed =
            (
                result != VPL_ERR_AGAIN
            &&  result != VPL_ERR_INTR
            &&  result < 0
            );

        if (failed) {
            log_info("poll() failed with error code %d", (int) result);
            total = -1;
            break;
        }

        /*
         *  Check whether a timeout has been declared.
         */
        if (client->keep.declared_dead) {
            failed = check_keep(client);

            if (failed) {
                ans_timeout_closes++;
                log_info("keep-alive declared a %s "
                    "timeout for device " FMTu64,
                    client->keep.where,
                    client->device_id);
                total = -1;
                break;
            }
        }

        /*
         *  Check for I/O errors on the TCP socket.
         */
        if (poll[0].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP)) {
            log_info("failed on poll flags 0x%x", (int) poll[0].revents);
            get_limits(&client->keep, &limits);
            dump_sends(&client->keep, &limits, "The TCP socket failed");
            dump_receives(&client->keep);
            total = -1;
            break;
        }

        /*
         *  Now check various timers.
         */
        current_time = VPLTime_GetTime();
        current_time = max(current_time, start_time);

        if (ans_print_interval > 0) {
            print_interval = VPLTime_FromSec(ans_print_interval);
        } else {
            print_interval = 0;
        }

        if (print_interval > 0 && current_time - last_print > print_interval) {
            last_print = current_time;
            ans_print_count++;
            log_info_1("I am waiting for data.");
        }

        if
        (
            partial_timeout > 0
        &&  current_time - start_time > partial_timeout
        &&  do_timeout
        ) {
            log_warn_1("The packet timeout limit was reached.");
            ans_partial_timeouts++;
            total = -1;
            break;
        }

        /*
         *  Read when the poll says there's input.
         */
        if (poll[0].revents & VPLSOCKET_POLL_RDNORM) {
            count = do_read(client, buffer, expected - total);

            if (count < 0) {
                total = -1;
                break;
            }

            buffer += count;
            total  += count;
        }

        /*
         *  If the upper layers have indicated that we should drop
         *  the connection, exit this loop and flag an error.
         */
        if (client->force_close) {
            total = -1;
            break;
        }
    } while (total < expected && !client->stop_now);

    return total;
}

/*
 *  Read a TCP packet from the network.  This routine returns
 *  only when a packet has been read or an error or shutdown
 *  has occurred.
 */
static packet_t *
read_packet(ans_client_t *client)
{
    packet_t *  packet;

    char *  body;
    char *  base;
    u16     packet_length;
    u16     check_length;
    u16     net_flag;
    u16     net_check;
    u16     net_length;
    int     remaining;
    int     result;
    int     has_tag;

    /*
     *  First, read the packet length and check length,
     *  which are the first four bytes on the network.
     *  The check length comes first.
     */
    result = io_loop(client, &packet_length, sizeof(packet_length), VPL_TRUE);

    if (client->stop_now || client->force_close) {
        return null;
    }

    if (result != sizeof(packet_length)) {
        return null;
    }

    /*
     *  Get the check length or the tag flag.  It's the first two bytes of
     *  the packet.
     */
    check_length = VPLConv_ntoh_u16(packet_length);
    has_tag      = check_length == server_tag_flag;

    /*
     *  If we got a tag, read the check size.
     */
    if (has_tag) {
        result = io_loop(client, &packet_length, sizeof(packet_length), VPL_TRUE);

        if (client->stop_now || client->force_close) {
            return null;
        }

        if (result != sizeof(packet_length)) {
            return null;
        }

        check_length = ~VPLConv_ntoh_u16(packet_length);
    } else {
        check_length = ~check_length;
    }

    /*
     *  Read the length field.  It's next.
     */
    result = io_loop(client, &packet_length, sizeof(packet_length), VPL_FALSE);

    if (client->stop_now || client->force_close) {
        return null;
    }

    if (result != sizeof(packet_length)) {
        return null;
    }

    /*
     *  Convert the packet length to host format, and
     *  start checking.
     */
    packet_length = VPLConv_ntoh_u16(packet_length);

    if
    (
        packet_length < (has_tag ? header_size + 10 : header_size)
    ||  packet_length > ans_max_packet_size
    ) {
        log_error("I failed on the packet size sanity check (%d).",
            (int) packet_length);
        return null;
    }

    if (check_length != packet_length) {
        log_error("The lengths didn't match (%d vs %d).",
            (int) packet_length, (int) check_length);
        return null;
    }

    /*
     *  Okay, read the rest of the packet.
     */
    body = (char *) malloc(packet_length);

    if (body == null) {
        log_error_1("malloc failed during read_packet");
        return null;
    }

    /*
     *  Okay, put the packet together.  We need to put
     *  the length and the check length back into it,
     *  in proper network form, so that we can compute
     *  the correct signature and check it against
     *  what we received.
     */
    remaining   = check_length - 2 * sizeof(uint16_t);

    if (has_tag) {
        remaining -= sizeof(uint16_t);
    }

    net_check   = VPLConv_ntoh_u16(~check_length   );
    net_length  = VPLConv_ntoh_u16( check_length   );
    net_flag    = VPLConv_ntoh_u16( server_tag_flag);

    if (!has_tag) {
        memcpy(&body[0],                    &net_check,  sizeof(u16));
        memcpy(&body[sizeof(check_length)], &net_length, sizeof(u16));

        base = &body[2 * sizeof(u16)];
    } else {
        memcpy(&body[0],               &net_flag,   sizeof(u16));
        memcpy(&body[    sizeof(u16)], &net_check,  sizeof(u16));
        memcpy(&body[2 * sizeof(u16)], &net_length, sizeof(u16));

        base = &body[3 * sizeof(u16)];
    }

    result = io_loop(client, base, remaining, VPL_FALSE);

    if (result != remaining) {
        log_error("I failed trying to read the body:  "
            "%d bytes received, expected %d.",
            result, remaining);
        free(body);
        return null;
    }

    packet = alloc_packet(body, packet_length);

    if (packet == null){
        log_error_1("I failed to malloc the packet structure.");
        free(body);
        return null;
    }

    return packet;
}

/*
 *  Initialize a cryptography structure using the given key.
 */
static crypto_t *
create_crypto(const void *key, int length)
{
    crypto_t *  crypto;

    int  aes_length;

    crypto = (crypto_t *) malloc(sizeof(*crypto));

    if (crypto == null) {
        return null;
    }

    memset(crypto, 0, sizeof(*crypto));

    crypto->sha1_key    = (u8 *) key;
    crypto->sha1_length = length;

    /*
     *  Create a valid AES key.
     */
    memset(crypto->aes_key, 0, aes_block_size);
    aes_length = aes_block_size > length ? length : aes_block_size;
    memcpy(crypto->aes_key, crypto->sha1_key, aes_length);

    return crypto;
}

static void
free_crypto(crypto_t **crypto)
{
    if (*crypto == null) {
        return;
    }

    free(*crypto);
    *crypto = null;
}

/*
 * Process a packet received from the ANS instance. Here
 * we check for a correct sequence number and signature.
 */
static int
process_packet(ans_client_t *client, packet_t *packet, int64_t *sequence)
{
    ans_unpacked_t *unpacked;
    int  print;

    if (!check_signature(client->crypto, packet)) {
        log_warn_1("The packet signature is invalid");
        return VPL_FALSE;
    }

    /*
     *  Now remove the length field from the packet and
     *  unpack it.  The unpack routine checks and advances
     *  the sequence id.
     */
    unpacked = unpack(client, packet, sequence);

    if (unpacked == null) {
        return VPL_FALSE;
    }

    if
    (
        client->tag != 0
    &&  client->tag != unpacked->tag
    &&  unpacked->type != Reject_credentials
    ) {
        log_info("got a tag mismatch:  " FMTs64 " vs " FMTs64,
            client->tag,
            unpacked->tag);
        free_unpacked(&unpacked);
        return VPL_FALSE;
    }

    /*
     *  Log information based on the packet type.
     */
    switch (unpacked->type) {
    case Send_unicast:
        log_info("received notification (asyncId=" FMTu64
            ") for user "FMT_VPLUser_Id_t,
            unpacked->asyncId, unpacked->userId);
        break;

    case Send_multicast:
        log_info("received multicast (asyncId=" FMTu64
            ") for user "FMT_VPLUser_Id_t,
            unpacked->asyncId, unpacked->userId);
        break;

    case Send_sleep_setup:
        log_info("received a requested sleep configuration (asyncId="FMTu64")", unpacked->asyncId);
        break;

    case Send_state_list:
        log_info("received a device state list (asyncId="FMTu64")", unpacked->asyncId);
        break;

    case Send_device_update:
        log_info("received a device update (asyncId="FMTu64")", unpacked->asyncId);
        break;

    case Reject_credentials:
        log_info("received a credentials rejection (asyncId="FMTu64")", unpacked->asyncId);
        break;
    }

    print = true;

    /*
     *  Invoke the type-specific callback, if any.
     */
    switch (unpacked->type) {
    case Send_unicast:
        unpacked->type = SEND_USER_NOTIFICATION;
        client->callback_table.receiveNotification(client, unpacked);
        break;

    case Send_multicast:
        unpacked->type = SEND_USER_MULTICAST;
        client->callback_table.receiveNotification(client, unpacked);
        break;

    case Send_sleep_setup:
        unpacked->type = SEND_SLEEP_SETUP;
        client->callback_table.receiveSleepInfo(client, unpacked);
        break;

    case Send_state_list:
        unpacked->type = SEND_STATE_LIST;
        client->callback_table.receiveDeviceState(client, unpacked);
        break;

    case Send_device_update:
        unpacked->type = SEND_DEVICE_UPDATE;
        client->callback_table.receiveDeviceState(client, unpacked);
        break;

    case Send_response:
        unpacked->type = SEND_RESPONSE;
        client->callback_table.receiveResponse(client, unpacked);
        break;

    case Reject_credentials:
        client->callback_table.rejectCredentials(client);
        break;

    default:
        print = false;
        break;
    }

    if (print) {
        log_info_1("callback completed");
    }

    free_unpacked(&unpacked);
    return VPL_TRUE;
}

/*
 *  Handle all processing that's required when a socket fails or is shut
 *  down.
 */
static void
declare_down(ans_client_t *client, const char *where)
{
    limits_t  limits;

    /*
     *  Print where we failed and the host name.
     */
    log_warn("declare_down:  %s.", where);
    log_warn("device " FMTu64 ", cluster %s, host %s, connection " FMTu64,
        client->device_id,
        client->cluster,
        client->host_name != null ? client->host_name : "unknown",
        client->connection);

    /*
     *  For debugging, this information can be useful.
     */
    if (!client->stop_now && !client->keep.declared_dead) {
        log_warn_1("Keep-Alive Status:");
        get_limits(&client->keep, &limits);
        dump_sends(&client->keep, &limits, "The connection is down");
        dump_receives(&client->keep);
    }

    free(client->host_name);
    free(client->external_ip);

    client->host_name         = null;
    client->external_ip       = null;
    client->server_time       = 0;
    client->device_id         = 0;
    client->generation        = 0;
    client->keep.udp_in       = 0;
    client->keep.udp_blocked  = false;
    client->keep_port         = ans_default_keep_port;

    /*
     *  Close the socket, if it's open.
     */
    close_socket(&client->socket);

    /*
     *  Tell the keep-alive thread that the socket is dead.
     */
    mutex_lock(&client->keep.mutex);
    client->keep.live_socket_id = -1;
    mutex_unlock(&client->keep.mutex);

    /*
     *  Delete the outgoing queue.
     */
    discard_tcp_queue(client);

    /*
     *  Tell the client that the connection is down.
     */
    client->callback_table.connectionDown(client);

    if (!client->stop_now) {
        /*
         *  Check the network changed flag, and set a very
         *  short wait if the network configuration has changed.
         */
        if (client->network_changed) {
            client->network_changed = VPL_FALSE;
            client->delay = ans_min_delay;
        } else  {
            client->delay = MIN(client->delay * 2, ans_max_delay);
        }

        if (client->verbose) {
            log_info("I will try again in %ds.", client->delay);
        }

        /*
         *  Delay before attempting to connect.
         */
        sleep_seconds(client, client->delay);
    }

    client->connection      = 0;
    client->out_sequence    = 0;
    client->rejected        = VPL_FALSE;
    client->ready_to_send   = VPL_FALSE;
    client->login_completed = VPL_FALSE;
    client->force_close     = VPL_FALSE;
}

/*
 *  Send a login blob to the ANS server.
 */
static int
send_blob(ans_client_t *client)
{
    packet_t *  packet;
    describe_t  describe;

    /*
     *  Create the packet.
     */
    clear_describe(&describe);

    describe.type   = Send_login_blob;
    describe.data   = client->blob;
    describe.count  = client->blob_length;
    describe.tag    = client->tag;

    packet = pack(client, &describe);

    if (packet == null) {
        return VPL_FALSE;
    }

    /*
     *  Put the blob packet at the head of the queue.
     */
    mutex_lock(&client->mutex);

    packet->next = client->tcp_head;
    client->tcp_head = packet;

    if (packet->next == null) {
        client->tcp_tail = packet;
    }

    mutex_unlock(&client->mutex);

    /*
     *  Sending the blob will make the connection active on the
     *  server end, so now set ready-to-send.
     */
    client->ready_to_send = VPL_TRUE;
    write_output(client);
    return VPL_TRUE;
}

/*
 *  Run a straight insertion sort and get the median as a
 *  robust estimator of the mean.
 */
static int64_t
get_median(int64_t *data, int count)
{
    int64_t  value;
    int64_t  median;
    int      index;
    int      j;

    if (count < 2) {
        return data[0];
    }

    print_data("input ", data, count);

    for (int i = 1; i < count; i++) {
        value = data[i];

        for (j = i; j > 0 && value < data[j - 1]; j--) {
            data[j] = data[j - 1];
        }

        data[j] = value;
    }

    print_data("sorted", data, count);

    index = count / 2;

    if (count & 1) {
        median = data[index];
    } else {
        median = (data[index - 1] + data[index]) / 2;
    }

    log_test("median = " FMTu64, median);
    return median;
}

/*
 *  Compute the mean absolute deviation as a
 *  robust estimator of the standard deviation.
 */
static int64_t
get_mad(int64_t *data, int count, int64_t median)
{
    int64_t  mad;

    for (int i = 0; i < count; i ++) {
        data[i] = abs(data[i] - median);
    }

    mad = get_median(data, count);
    log_test("mad    = " FMTu64, mad);
    return mad;
}

/*
 *  Compute a rough estimate of the round-trip time for a
 *  UDP ping and response.  This estimate is used to modify
 *  the timeout period.
 *
 *  The response times are kept in the ping_time history field.
 */
static void
estimate_rtt(keep_t *keep)
{
    int       pings_recorded;
    int       i;
    int64_t   median;
    int64_t   mad;
    int64_t   result;

    pings_recorded  = 0;
    median          = 0;
    mad             = 0;

    /*
     *  Sum up the response times and number of responses received.
     */
    for (i = 0; i < array_size(keep->ping_rtt); i++) {
        if (keep->ping_rtt[i] != 0) {
           keep->sort_area[pings_recorded] = keep->ping_rtt[i];
           pings_recorded++;
        }
    }

    /*
     *  Compute the median and the MAD.  We estimate an
     *  acceptable rtt as
     *     median + 3 * MAD
     *  but set a floor at 20 ms.  A scheduling slice
     *  might be that long.
     *
     *  If we don't have any data yet, we use 500 ms as
     *  a guess.
     */
    if (pings_recorded > 0) {
        median    = get_median(keep->sort_area, pings_recorded);
        mad       = get_mad   (keep->sort_area, pings_recorded, median);
        result    = median + 3 * mad;
        result    = max(result, VPLTime_FromMillisec(20));
    } else {
        result = 500 * keep->time_to_millis;
    }

    log_test("rtt estimate " FMTu64 " ms, median "  FMTu64 " us, "
        "mad " FMTu64 " us",
        result / keep->time_to_millis, median, mad);

    keep->rtt_estimate = result;
    return;
}

/*
 *  Parse a UDP keep-alive packet.  Verify the signature
 *  and check the sequence number, eliminating any
 *  duplicates.
 */
static int
process_udp_keep_alive
(
    keep_t *  keep,
    char *    body,
    int       buffer_size
)
{
    ans_unpacked_t *  unpacked;

    packet_t *  packet;
    int         bucket;
    int64_t     current_time;
    int64_t     rtt;
    int         pass;
    uint16_t    size;

    if (keep->client->crypto == null) {
        return false;
    }

    if (buffer_size < header_size + signature_size) {
        ans_bad_keep_sizes++;
        ans_keep_errors++;
        log_test("The buffer size (%d) is invalid!", (int) buffer_size);
        return false;
    }

    /*
     *  First, pull the check size out of the packet and validate it.
     */
    memcpy(&size, body, sizeof(uint16_t));

    size = VPLConv_ntoh_u16(size);

    if (size == server_tag_flag) {
        memcpy(&size, body + 2, sizeof(uint16_t));
        size = VPLConv_ntoh_u16(size);
    }

    size ^= -1;

    if (size > buffer_size || size < header_size + signature_size) {
        ans_bad_keep_sizes++;
        ans_keep_errors++;
        log_test("The transmitted size (%d) is invalid!", (int) size);
        return false;
    }

    packet = alloc_packet(body, size);

    if (packet == null) {
        ans_keep_errors++;
        log_warn_1("malloc failed during keep-alive parsing");
        return false;
    }

    /*
     *  Validate the signature.
     */
    if (!check_signature(keep->client->crypto, packet)) {
        ans_keep_errors++;
        packet->base = null;
        free_packet(&packet);
        return false;
    }

    /*
     *  Unpack the buffer, if possible.
     */
    unpacked = unpack(null, packet, null);

    if (unpacked == null) {
        ans_keep_errors++;
        packet->base = null;
        free_packet(&packet);
        return false;
    }

    /*
     *  Check that the type makes sense.
     */
    if (unpacked->type != Send_timed_ping) {
        ans_keep_errors++;
        packet->base = null;
        free_packet(&packet);
        free_unpacked(&unpacked);
        return false;
    }

    /*
     *  Make sure that the packet was intended for this client.
     */
    if (unpacked->connection != keep->client->connection) {
        ans_keep_connection++;
        packet->base = null;
        free_packet(&packet);
        free_unpacked(&unpacked);
        return false;
    }

    pass = false;

    /*
     *  Update the round trip array if the packet seems valid and is not a
     *  repeat.  Save the packet time and the sequence number for future
     *  use.
     */
    if (unpacked->sequenceId > keep->in_sequence) {
        bucket = unpacked->sequenceId % array_size(keep->ping_id);

        if (unpacked->sequenceId > keep->ping_id[bucket]) {
            current_time             = VPLTime_GetTime();
            keep->ping_when[bucket]  = current_time;
            keep->ping_id  [bucket]  = unpacked->sequenceId;
            keep->in_sequence        = unpacked->sequenceId;

            /*
             *  Check for the clock moving backwards.  If the time is
             *  reasonable, compute the rtt, but clamp the maximum rtt
             *  to 2 seconds.
             */
            if (current_time >= unpacked->sendTime) {
                rtt = max(1, current_time - unpacked->sendTime);
                keep->ping_rtt[bucket] = min(rtt, VPLTime_FromSec(2));
            }

            log_test("received UDP ping " FMTs64 " at " FMTu64
                ", rtt " FMTs64 " micros",
                unpacked->sequenceId,
                current_time,
                current_time - unpacked->sendTime);

            pass = true;

            estimate_rtt(keep);

            keep->last_udp_receive  = VPLTime_GetTime();
            keep->received_udp      = true;
            keep->udp_blocked       = false;

            keep->udp_in++;
            ans_keep_packets_in++;  /* for testing */
        }
    }

    if (!pass) {
        log_test("The keep-alive sequence id (" FMTs64 ") was invalid.",
            unpacked->sequenceId);
        ans_keep_errors++;
    }

    packet->base = null;
    free_packet(&packet);
    free_unpacked(&unpacked);
    return pass;
}

/*
 *  Drop all the packets in the UDP output queue.  This routine
 *  is invoked when a socket fails or when shutting down.
 */
static void
clear_keep_output(keep_t *keep)
{
    packet_t *  next;
    packet_t *  current;

    next = keep->udp_head;

    while (next != null) {
        current = next;
        next    = next->next;

        free_packet(&current);
    }

    keep->udp_head = null;
    keep->udp_tail = null;
}

static VPLSocket_t
make_udp_socket(ans_client_t *client)
{
    return VPLSocket_Create(VPL_PF_INET, VPLSOCKET_DGRAM, VPL_TRUE);
}

/*
 *  Poll the UDP keep-alive socket for input and output, as appropriate.
 *  The caller specifies the maximum time to wait.
 */
static int
poll_keep_io(keep_t *keep, int64_t time_limit, const char *where)
{
    VPLSocket_poll_t  poll[2];
    VPLSocket_addr_t  address;

    int64_t     start_time;
    int64_t     current_time;
    int64_t     time_error;
    int         result;
    packet_t *  packet;
    char        buffer[200];
    int         received;
    int         i;

    time_limit = max(time_limit, VPLTime_FromMillisec(50));
    time_limit = min(time_limit, VPLTime_FromSec(5));

    received         = false;
    poll[0].socket   = keep->socket;
    poll[0].events   = VPLSOCKET_POLL_RDNORM;
    poll[0].revents  = 0;

    poll[1].socket   = keep->event.socket;
    poll[1].events   = VPLSOCKET_POLL_RDNORM;
    poll[1].revents  = 0;

    /*
     *  Check whether we have a packet to send, and
     *  whether it's valid to try to send one.  If
     *  we've received a packet, the server is ready
     *  to receive them. Otherwise, maybe not.
     */
    if
    (
        keep->udp_head       != null
    &&  keep->client->crypto != null
    &&  keep->client->ready_to_send
    ) {
        poll[0].events |= VPLSOCKET_POLL_OUT;
    }

    log_test("start a keep poll at " FMTu64 ", delay " FMTs64
        ", events 0x%x",
        VPLTime_GetTime(), time_limit, (unsigned) poll[0].events);

    start_time    = VPLTime_GetTime();
    result        = VPLSocket_Poll(poll, array_size(poll), time_limit);
    current_time  = VPLTime_GetTime();
    time_error    = (current_time - start_time) - time_limit;
    ans_max_error = max(ans_max_error, time_error);

    log_test("finished a keep poll at " FMTu64 " with revents 0x%x from 0x%x, "
        "event 0x%x, delay error " FMTs64 ", at %s",
                   current_time,
        (unsigned) poll[0].revents,
        (unsigned) poll[0].events,
        (unsigned) poll[1].revents,
                   time_error,
                   where);

    check_event(&keep->event, result, &poll[1]);

    /*
     *  If we're going into background mode, ditch all the
     *  outgoing packets.
     */
    if (keep->client->interval != 0) {
        poll[0].revents &= ~VPLSOCKET_POLL_OUT;
        clear_keep_output(keep);
    }

    /*
     *  Perform output, if needed and possible.
     */
    if (poll[0].revents & VPLSOCKET_POLL_OUT) {
        packet         = keep->udp_head;
        address.family = VPL_PF_INET;
        address.addr   = keep->client->server_ip;
        address.port   = VPLNet_port_ntoh(keep->client->keep_port);

        log_test("sending UDP ping " FMTs64 " for client %p to port %d at " FMTs64,
            packet->sequence, keep->client, (int) keep->client->keep_port, VPLTime_GetTime());

        result =
            VPLSocket_SendTo
            (
                keep->socket,
                packet->buffer,
                packet->length,
                &address,
                sizeof(address)
            );

        /*
         *  Check whether the send succeeded, and process
         *  accordingly.
         */
        if (result == packet->length) {
            ans_keep_packets_out++;
            keep->udp_head = packet->next;

            if (keep->udp_head == null) {
                keep->udp_tail = null;
            }

            i              = (keep->i++) % array_size(keep->sends);
            current_time   = VPLTime_GetTime();
            keep->sends[i] = current_time;
            keep->id   [i] = packet->sequence;
            keep->who  [i] = packet->where;
            keep->warned   = false;

            if (!keep->udp_blocked) {
                keep->deadline = keep->sends[i] + keep->rtt_estimate;
            }

            estimate_rtt(keep);

            log_test("poll_keep_io:  blocked %d, deadline " FMTs64 " (%s)",
                (int) keep->udp_blocked,
                      keep->deadline,
                      print_time(current_time, buffer, sizeof(buffer)));

            free_packet(&packet);
        } else if (result == VPL_ERR_UNREACH) {
#ifdef IOS
            /*
             *  On iOS, after we close the network interface,
             *  VPLSocket_SendTo() returns VPL_ERR_UNREACH,
             *  even though this socket is for UDP.
             */
            keep->client->ready_to_send = VPL_FALSE;
#endif
            VPLThread_Sleep(VPLTime_FromMillisec(100));
        } else {
            if (!keep->warned) {
                log_warn("VPLSocket_SendTo returned %d", result);
                keep->warned = true;
            }

            VPLThread_Sleep(VPLTime_FromMillisec(100));
        }
    }

    /*
     *  Try to read any available input.  Process any packet that's received.
     */
    if ((poll[0].revents & VPLSOCKET_POLL_RDNORM) && keep->packet_length != 0) {
        result =
            VPLSocket_RecvFrom
            (
                keep->socket,
                buffer,
                sizeof(buffer),
                &address,
                sizeof(address)
            );

        if (result > 0) {
            received = process_udp_keep_alive(keep, buffer, sizeof(buffer));
        } else {
            log_warn("keep-alive RecvFrom returned %d, size "FMTu_size_t,
                result, sizeof(buffer));

            close_socket(&keep->socket);
            keep->socket = make_udp_socket(keep->client);
        }
    }

    return received;
}

/*
 *  Make a UDP ping packet and add it to the output queue.
 */
static int
send_udp_ping(keep_t *keep, const char *where)
{
    packet_t *  packet;

    /*
     *  If the crytography key isn't available, we can't send.
     */
    if (keep->client->crypto == null) {
        return false;
    }

    log_test("creating UDP ping " FMTs64 " at %s", keep->out_sequence, where);

    packet = make_keep_packet(keep);

    if (packet == null) {
        return false;
    }

    prep_packet(keep->client, packet, &keep->out_sequence);

    keep->packet_length = packet->length;
    packet->next        = null;
    packet->where       = where;

    /*
     *  Apply the lock and add the packet to the queue.
     */
    mutex_lock(&keep->mutex);

    if (keep->udp_head == null) {
        keep->udp_head = packet;
        keep->udp_tail = packet;
    } else {
        keep->udp_tail->next = packet;
        keep->udp_tail       = packet;
    }

    mutex_unlock(&keep->mutex);
    return true;
}

/*
 *  Declare a user-level keep-alive timeout on a socket,
 *  if the socket is still live.  Ping the device thread
 *  when we're done so that it shuts down the TCP socket.
 */
static void
declare_timeout(keep_t *keep, int32_t my_socket, const char *where)
{
    limits_t  limits;

    mutex_lock(&keep->mutex);

    if (keep->live_socket_id == my_socket) {
        keep->live_socket_id  = -1;
        keep->dead_socket_id  = my_socket;
        keep->declared_dead   = true;
        keep->where           = where;

        ans_timeout_events++;
        ans_timeout_where = where;

        clear_keep_output(keep);
        log_info("I am declaring a %s timeout for client %p",
            where, keep->client);

        get_limits(keep, &limits);
        dump_sends(keep, &limits, "Timeout Status:");
        dump_receives(keep);
    }

    mutex_unlock(&keep->mutex);
    ping_device(keep->client);
}

/*
 *  Compute the time to send the next UDP packet.  Mostly we
 *  just introduce jitter if the random number generator has
 *  been initialized.
 */
static int64_t
next(keep_t *keep, int64_t send_time, int64_t delay, uint64_t current_time)
{
    int64_t  next_send_time;
    int64_t  jitter;

    next_send_time = send_time + delay;

    /*
     *  If the packet already is late, just send one now.
     *  Otherwise, subtract some jitter time.
     */
    if (next_send_time < current_time) {
        next_send_time = current_time;
        log_test("next_send_time " FMTs64 " = " FMTs64,
            next_send_time, current_time);
    } else if (keep->client->mt.inited && ans_jitter > 1) {
        jitter          = mtwist_next(&keep->client->mt, ans_jitter);
        jitter         *= keep->time_to_millis;
        next_send_time -= jitter;

        log_test("next_send_time " FMTs64 " = " FMTs64 " + "
            FMTu64 " - " FMTs64,
            next_send_time, send_time, delay, jitter);
    }

    return next_send_time;
}

/*
 *  Compute the time to wait in the UDP poll routine.  We
 *  need to obey the deadline for entering the next phase
 *  and the next time to send a packet.
 */
static int64_t
compute_wait
(
    keep_t *    keep,
    limits_t *  limits,
    int64_t     current_time,
    int64_t     next_send_time
)
{
    int64_t  result;
    int64_t  tcp_wait;
    int64_t  udp_wait;
    int64_t  deadline;

    deadline  = keep->deadline;
    tcp_wait  = keep->last_tcp_receive + limits->tcp_limit - current_time;
    udp_wait  = min(deadline, next_send_time) - current_time;
    result    = min(udp_wait, tcp_wait);
    result    = max(result, 0);

    log_test("compute_wait:  udp_wait " FMTs64 " = min(dl " FMTs64 ", ns " FMTs64 ") - ct " FMTs64,
        udp_wait, deadline, next_send_time, current_time);
    log_test("compute_wait:  tcp_wait " FMTs64 " = " FMTs64 " - " FMTs64 " + " FMTs64,
        tcp_wait, keep->last_tcp_receive, limits->tcp_limit, current_time);
    log_test("compute_wait:  wait " FMTs64 ", GetTime " FMTs64, result, VPLTime_GetTime());
    return result;
}

static int
check_tcp_timeout(keep_t *keep, int32_t my_socket, int64_t current_time, limits_t *limits, const char *where)
{
    int64_t  tcp_limit;
    int      result;

    tcp_limit = limits->tcp_limit;

    if (tcp_limit > 0) {
        tcp_limit += keep->rtt_estimate;
    }

    result = tcp_limit > 0 && current_time - keep->last_tcp_receive > tcp_limit;

    log_test("check_tcp_timeout:  time " FMTs64 ", last " FMTs64 ", c - last " FMTs64 ", limit " FMTs64 " -> %s, at %s",
        current_time,
        keep->last_tcp_receive,
        current_time - keep->last_tcp_receive,
        tcp_limit,
        result ? "true" : "false",
        where);

    return result;
}

static int
wait_for_send(keep_t *keep, int my_socket, int64_t start_time, int64_t next_send_time)
{
    int64_t   current_time;
    int       tcp_timeout;
    int64_t   wait_time;
    int       received;
    limits_t  limits;

    get_limits(keep, &limits);

    current_time  = start_time;
    tcp_timeout   = check_tcp_timeout(keep, my_socket, current_time, &limits, "wait_for_send");

    while
    (
        my_socket == keep->live_socket_id
    &&  current_time < next_send_time
    &&  current_time < keep->deadline
    &&  keep->client->is_foreground
    &&  !tcp_timeout
    ) {
        wait_time    = compute_wait(keep, &limits, current_time, next_send_time);

        log_test("wait_for_send:  loop at " FMTs64 ", deadline " FMTs64 ", ns - ct " FMTs64,
            current_time, keep->deadline, next_send_time - current_time);

        received     = poll_keep_io(keep, wait_time, "wait_for_send");
        current_time = VPLTime_GetTime();
        tcp_timeout  = check_tcp_timeout(keep, my_socket, current_time, &limits, "wait_for_send loop");

        /*
         *  If we've received a UDP packet, update the deadline to
         *  past the next send time.  The poll_keep_io set the
         *  deadline to the expected receive time of the packet to
         *  expedite lost packet detection, so now we need to make
         *  a new best guess.
         */
        if (keep->received_udp) {
            keep->deadline = next_send_time + limits.base_delay;
        }
    }

    log_test("wait_for_send:  exit at " FMTs64 ", deadline " FMTs64 ", ns - ct " FMTs64,
        current_time, keep->deadline, next_send_time - current_time);
    return tcp_timeout;
}

static int
run_fast_keep(keep_t *keep, int my_socket, limits_t *limits)
{
    int64_t  current_time;
    int64_t  next_send_time;
    int64_t  deadline;
    int      udp_timeout;
    int      tcp_timeout;
    int      received;
    int      retries_sent;
    int64_t  wait_time;
#ifdef ans_test
    char     buffer[500];
#endif

    /*
     *  We only get this far if keep-alive is enabled.
     *
     *  Prepare to handle the case where pings aren't being received
     *  quickly enough, or at all.
     */
    current_time    = VPLTime_GetTime();
    deadline        = current_time + limits->udp_limit + keep->rtt_estimate;
    next_send_time  = next(keep, current_time, limits->retry_delay, current_time);

    log_test("run_fast_keep:  client %p at " FMTs64 ", d - ct " FMTs64,
        keep->client, current_time, deadline - current_time);
    log_test("next_send_time    " FMTs64, next_send_time);
    log_test("last_udp_send     " FMTs64, keep->last_udp_send);
    log_test("last_udp_receive  " FMTu64, keep->last_udp_receive);
    log_test("deadline          " FMTs64, deadline);

    send_udp_ping(keep, "start retries");
    ans_fast_timeout++;
    ans_fast_packets++;

    retries_sent  = 1;
    udp_timeout   = false;
    tcp_timeout   = false;

    /*
     *  While we need to run in the fast retry state:
     */
    while (my_socket == keep->live_socket_id && keep->client->is_foreground) {
        received = false;

        /*
         *  Wait until it's time to send the next packet, or the
         *  socket has been shut down.
         */
        do {
            wait_time     = max(min(next_send_time, deadline) - current_time, 0);
            received      = poll_keep_io(keep, wait_time, "run_fast_keep");
            current_time  = VPLTime_GetTime();

            if (current_time >= next_send_time) {
                break;
            }

            /*
             *  Exit the fast loop if a keep-alive has arrived.
             *  reached.
             */
            if (received) {
                break;
            }

            tcp_timeout = check_tcp_timeout(keep, my_socket, current_time, limits, "run_fast_keep inner loop");
        }
        while
        (
            my_socket == keep->live_socket_id
        &&  next_send_time > current_time
        &&  keep->client->is_foreground
        &&  current_time < deadline
        &&  !tcp_timeout
        );

        /*
         *  If the state has changed, or the TCP timeout has
         *  been reached, we're done.
         */
        if (!keep->client->is_foreground || tcp_timeout) {
            break;
        }

        /*
         *  If we got a packet, we're done.
         */
        if (received) {
            break;
        }

        /*
         *  Check the deadline against the current time.  Stop the
         *  retry loop if we've reached the original deadline and
         *  we've managed to queue all the retry packets.  Otherwise,
         *  set the deadline to some time past the expected receive
         *  time, or sometime in the future, at least.
         */
        if (current_time >= deadline) {
            udp_timeout = retries_sent >= limits->retry_count;

            if (keep->client->enable_tcp_only && udp_timeout && keep->udp_in == 0) {
                keep->udp_blocked = true;
                udp_timeout       = false;
                break;
            } else if (udp_timeout) {
                log_test("fast loop gives up:  current_time " FMTs64 ", deadline " FMTs64
                    ", ct - dl " FMTs64 " (%s)",
                    current_time, deadline, current_time - deadline,
                    print_time(deadline, buffer, sizeof(buffer)));
                break;
            } else {
                deadline = max(current_time, next_send_time) + keep->rtt_estimate;
                log_test("fast loop update deadline:  " FMTs64, deadline);
            }
        }

        /*
         *  Now check whether it's time to send the next keep-alive.  If so,
         *  updated the deadline as needed, and send the packet.
         */
        if (next_send_time <= current_time && retries_sent < limits->retry_count) {
            deadline       = max(current_time + keep->rtt_estimate, deadline);
            next_send_time = next(keep, next_send_time, limits->retry_delay, current_time);

            send_udp_ping(keep, "send retry");
            send_tcp_ping(keep, "send retry");
            ans_fast_packets++;
            retries_sent++;

            log_test("queued a retry UDP ping, deadline " FMTs64,
                deadline);
        }

        tcp_timeout = check_tcp_timeout(keep, my_socket, current_time, limits, "run_fast_keep outer loop");

        if (tcp_timeout) {
            log_test("TCP timeout reached in the fast loop");
            break;
        }
    }

    if (tcp_timeout) {
        declare_timeout(keep, my_socket, "TCP");
    }

    log_test("run_fast_keep:  exit at " FMTs64, VPLTime_GetTime());
    return udp_timeout;
}

/*
 *  Loop handling keep-alive for one TCP connection.  Exit
 *  when the connection fails or a shutdown is requested.
 */
static void
keep_one_socket(keep_t *keep)
{
    int64_t    current_time;
    limits_t   limits;
    int32_t    my_socket;
    int64_t    next_send_time;
    int        countdown;
    int        udp_timeout;
    int        tcp_timeout;
    int        blocked;

    /*
     *  Find the socket id of the current socket.  Return if
     *  there's no connection to the ANS server.
     */
    my_socket = keep->live_socket_id;

    if (my_socket < 0) {
        return;
    }

    /*
     *  Get the timeout parameters.  If they're not configured as
     *  active, just exit.
     */
    get_limits(keep, &limits);

    if (!limits.valid) {
        event_wait(&keep->event, 0);
        return;
    }

    if (!keep->client->is_foreground || keep->client->interval != 0) {
        return;
    }

    keep->keeping = true;

    estimate_rtt(keep);

    /*
     *  Compute the various deadline and send times for
     *  starting our loop.
     */
    current_time            = VPLTime_GetTime();
    keep->last_tcp_receive  = current_time;
    keep->last_udp_receive  = 0;
    keep->last_udp_send     = 0;
    keep->received_udp      = false;
    next_send_time          = next(keep, current_time, limits.base_delay, current_time);
    keep->deadline          = current_time + limits.base_delay;
    tcp_timeout             = false;
    countdown               = limits.factor;

    /*
     *  Send the first ping.
     */
    send_udp_ping(keep, "start keep-alive");

    /*
     *  Send a TCP ping if requested.
     */
    if (--countdown == 0) {
        send_tcp_ping(keep, "keep_one start");
        countdown = limits.factor;
    }

    log_test("current_time      " FMTu64, current_time);
    log_test("next_send_time    " FMTs64, next_send_time);
    log_test("deadline          " FMTs64, keep->deadline);
    log_test("base delay        " FMTs64, limits.base_delay);
    log_test("retry delay       " FMTs64, limits.retry_delay);
    log_test("retry count       " FMTs64, (int64_t) limits.retry_count);
    log_test("factor            " FMTs64, (int64_t) limits.factor);

    /*
     *  While the socket still is live and we're in the foreground.
     */
    while (my_socket == keep->live_socket_id && keep->client->is_foreground) {
        log_test("entering the keep-alive slow loop for %p", keep->client);

        /*
         *  This loops handle the normal case with limits enabled,
         *  where the socket is valid and pings from the server are
         *  arriving in a timely fashion.
         */
        do {
            blocked       = keep->udp_blocked;
            current_time  = VPLTime_GetTime();
            tcp_timeout   = wait_for_send(keep, my_socket, current_time, next_send_time);

            if (tcp_timeout) {
                declare_timeout(keep, my_socket, "TCP");
                break;
            }

            current_time = VPLTime_GetTime();

            if (!keep->received_udp && !keep->udp_blocked) {
                log_test("break to the fast loop at " FMTs64 ", deadline " FMTs64,
                    current_time, keep->deadline);
                break;
            }

            log_test("keep_one_socket:  done with wait_for_send, ct " FMTs64 ", ns " FMTs64,
                current_time,
                next_send_time);

            /*
             *  Send a keep-alive packet, and schedule the next one.
             *  In addition, refetch the limits, in case they've changed.
             */
            if (current_time >= next_send_time) {
                keep->received_udp   = false;
                keep->last_udp_send  = current_time;
                next_send_time       = next(keep, next_send_time, limits.base_delay, current_time);
                keep->deadline       = current_time + limits.base_delay;  // guess
                send_udp_ping(keep, "send normal");

                log_test("keep_one_socket:  send UDP, ct " FMTs64 ", ns " FMTs64 ", deadline " FMTs64,
                    current_time, next_send_time, keep->deadline);

                if (--countdown == 0) {
                    send_tcp_ping(keep, "keep_one slow");
                    countdown = limits.factor;
                }
            }

            get_limits(keep, &limits);
        } while (my_socket == keep->live_socket_id && limits.valid && keep->client->is_foreground);

        /*
         *  If keep-alive is disabled, go back to the main wait loop.
         */
        if (!limits.valid) {
            log_test("The server stopped keep-alive at " FMTs64, current_time);
            break;
        }

        /*
         *  If my socket is dead or we've switched to background mode, give up.
         */
        if (my_socket != keep->live_socket_id || !keep->client->is_foreground) {
            break;
        }

        udp_timeout = run_fast_keep(keep, my_socket, &limits);

        log_test("leaving the keep-alive fast loop for client %p at " FMTu64,
            keep->client, current_time);
        log_test("current_time      " FMTs64, current_time);
        log_test("next_send_time    " FMTs64, next_send_time);
        log_test("last_udp_receive  " FMTs64, keep->last_udp_receive);
        log_test("deadline          " FMTs64, keep->deadline);

        /*
         *  If we have transitioned to the udp_blocked state, reset the
         *  the deadline to prevent wait_for_send from exiting prematurely.
         */
        if (!blocked && keep->udp_blocked) {
            log_test("keep_one_socket:  advanced to the blocked state");
            keep->deadline  = next_send_time + limits.base_delay;
        }

        /*
         *  If the deadline has passed, declare the socket dead.
         *  The declare_timeout checks the validity of the my_socket
         *  value atomically.
         */
        if (udp_timeout) {
            log_warn("UDP timeout:  current_time " FMTs64 ", last_udp_receive " FMTu64,
                VPLTime_GetTime(), keep->last_udp_receive);
            declare_timeout(keep, my_socket, "UDP");
            break;
        }

        get_limits(keep, &limits);

        current_time    = VPLTime_GetTime();
        next_send_time  = current_time + limits.base_delay;
    }

    log_test("leaving keep_one_socket for %p", keep->client);
    keep->keeping = false;
    clear_keep_output(keep);
}

/*
 *  This routine implements the main thread processing for
 *  keep-alive.
 */
static VPLTHREAD_FN_DECL
run_keep_alive(void *config)
{
    keep_t *   keep;

    keep = (keep_t *) config;

    log_always_1("keep-alive thread started");

    /*
     *  Loop until the client is shutting down.
     */
    while (!keep->stop) {
        keep->keep_foreground = keep->client->is_foreground;

        /*
         *  Wait until there's a connection.
         */
        while
        (
            !keep->stop
        &&  (keep->live_socket_id < 0 || !keep->client->is_foreground)
        ) {
            event_wait(&keep->event, 0);
        }

        /*
         *  Peform keep-alive for the current socket, if appropriate.
         */
        if
        (
            !keep->stop
        &&  keep->client->is_foreground
        &&  keep->keep_foreground
        ) {
            keep_one_socket(keep);
        }
    }

    clear_keep_output(keep);
    log_always_1("keep-alive thread done");
    keep->done = true;

    return VPLTHREAD_RETURN_VALUE;
}

/*
 *  Tell the keep-alive thread to shut down.  Then wait for it
 *  to exit, if asked.
 */
static void
stop_keep_alive(ans_client_t *client, int wait)
{
    client->keep.stop = true;  // might have been set in ans_close
    client->keep.live_socket_id = -1;
    ping_keep_alive(client);

    while (wait && !client->keep.done) {
        ping_keep_alive(client);
        VPLThread_Sleep(VPLTime_FromMillisec(100));
    }
}

/*
 *  Free any allocated memory associated with the given ans_client
 *  structure.  This routine is idempotent.
 */
static void
free_client_malloc(ans_client_t *client)
{
    free(client->cluster);
    free(client->key);
    free(client->blob);
    free(client->subscriptions);
    free(client->challenge);
    free(client->device_type);
    free(client->application);
    free(client->host_name);
    free(client->external_ip);

    client->cluster        = null;
    client->key            = null;
    client->blob           = null;
    client->subscriptions  = null;
    client->challenge      = null;
    client->device_type    = null;
    client->application    = null;
    client->host_name      = null;
    client->external_ip    = null;

    free_crypto(&client->crypto);
}

/*
 *  Destroy any VPL locks associated with a client.
 */
static void
destroy_locks(ans_client_t *client)
{
    VPLMutex_Destroy(&client->mutex);
    VPLMutex_Destroy(&client->mode_mutex);
    VPLMutex_Destroy(&client->keep.mutex);
    VPLMutex_Destroy(&client->event.mutex);
    VPLMutex_Destroy(&client->keep.event.mutex);
}

/*
 * Implements the ANS device client thread.
 */
static VPLTHREAD_FN_DECL
run_device(void *config)
{
    packet_t *        packet;
    ans_client_t *    client;
    ans_unpacked_t *  unpacked;
    VPLNet_addr_t     local_ip;
    describe_t        describe;

    int       socket_failed;
    int       success;
    int       result;
    long      packets;
    int64_t   sequence;
    int64_t   unused;

    client = (ans_client_t *) config;
    packet = null;

    client->socket  = VPLSOCKET_INVALID;
    client->delay   = ans_min_delay;

    log_always("client %p has started, device type %s",
        client, client->device_type);

    /*
     *  Loop until asked to stop.
     *  1) Get a socket
     *  2) Wait for a key.
     *  3) Create the MAC instance
     *  4) Loop reading packets until an error * occurs
     */
    while (!client->stop_now) {
        client->socket = sync_open_socket(client);

        if (client->stop_now) {
            break;
        }

        /*
         *  Tell the client that we have a connection.
         */
        local_ip = VPLSocket_GetAddr(client->socket);
        client->callback_table.connectionActive(client, local_ip);

        /*
         * Get the challenge.
         */
        packet = read_packet(client);

        if (client->stop_now) {
            break;
        }

        if (packet == null) {
            declare_down(client, "I failed to read the challenge");
            continue;
        }

        /*
         *  Read the challenge packet for the login sequence.
         */
        unused   = 0;
        unpacked = unpack(client, packet, &unused);

        free_packet(&packet);

        if (unpacked == null || unpacked->type != Send_challenge) {
            declare_down(client, "The challenge packet was invalid");
            continue;
        }

        client->tag = unpacked->tag;
        log_info("Setting the connection tag to " FMTu64, client->tag);

        free_unpacked(&unpacked);

        /*
         * Set up encryption, if possible.
         */
        if (client->crypto == null) {
            client->crypto = create_crypto(client->key, client->key_length);
        }

        if (client->crypto == null) {
            declare_down(client, "The MAC setup failed");
            continue;
        }

        success = send_blob(client);

        if (!success) {
            declare_down(client, "The blob send failed");
            continue;
        }

        /*
         * Set the sequence number.
         */
        sequence = 0;

        /*
         * Loop reading packets until error occurs or we are requested
         * to stop.
         */
        packets = 0;
        socket_failed = false;

        while (!socket_failed && !client->rejected && !client->stop_now) {
            packet = read_packet(client);
            socket_failed = (packet == null);

            if (!socket_failed) {
                socket_failed = !process_packet(client, packet, &sequence);
                packets++;
                free_packet(&packet);
            }
        }

        /*
         *  Reset the retry delay if we got some packets through.
         */
        if (packets > 3 && client->login_completed) {
            client->delay = ans_min_delay;
        }

        /*
         * Do some cleanup if we're not exiting.
         */
        if (client->force_close) {
            ans_forced_closes++;
            declare_down(client, "The connection was closed by request");
        } else if (!client->stop_now) {
            declare_down(client, "The device socket failed");
        }
    }

    /*
     *  Free any packet structure we might have.
     */
    free_packet(&packet);
    log_always("client %p is stopping", client);

    /*
     *  Send a shutdown message if we can.
     */
    if (valid(client->socket)) {
        clear_describe(&describe);
        describe.type = Send_device_shutdown;
        send_message(client, &describe);
        sleep_seconds(client, 1);
    }

    /*
     *  Shut down the socket and clear the queues.
     */
    declare_down(client, "This thread is stopping");

    // Must never reach here until ans_close() is called.
    ASSERT(client->stop_now);

    /*
     *  Stop the keep-alive thread and wait for it to exit.
     */
    stop_keep_alive(client, true);

    result = VPLDetachableThread_Join(&client->keep_handle);

    if (result != VPL_OK) {
        log_error("The join with the keep-alive thread failed: %d", result);
        /* leak memory */
    }

    /*
     *  Free any remaining packets.
     */
    discard_tcp_queue(client);

    /*
     *  Free the event sockets and then release memory.
     */
    free_event(&client->event);
    free_event(&client->keep.event);
    free_client_malloc(client);

    while (client->cleanup == cleanup_not_set) {
        log_info_1("I am waiting for the cleanup sign.");
        VPLThread_Sleep(VPLTime_FromSec(1));
    }

    destroy_locks(client);

    /*
     *  Tell the client that we're done.
     */
    client->callback_table.connectionClosed(client);

    if (client->cleanup == cleanup_async) {
        ans_shutdowns++;    // for testing
        free(client);
    }

    log_info_1("The device thread is exiting.");
    return VPLTHREAD_RETURN_VALUE;
}

/*
 *  Do any needed cleanup when an ans_open attempt fails.
 */
static void
fail_open(ans_client_t *client, int made_locks)
{
    /*
     *  The keep-alive thread might be running.
     */
    if (client->keep_started) {
        stop_keep_alive(client, true);
        VPLDetachableThread_Join(&client->keep_handle);
    }

    if (made_locks) {
        destroy_locks(client);
    }

    free_client_malloc(client);
    free(client);
    return;
}

/*
 *  Initialize all the locks associated with a given
 *  client instance.
 */
static void
make_locks(ans_client_t *client)
{
    mutex_init(&client->mutex);
    mutex_init(&client->mode_mutex);
    mutex_init(&client->keep.mutex);
    mutex_init(&client->event.mutex);
    mutex_init(&client->keep.event.mutex);
}

/*
 *  Starts a process that connects to an ANS server and
 *  performs the TCP I/O.
 *
 *  The ANS library will attempt to keep a connection open
 *  until the device client actively closes the connection.
 */
ans_client_t *
ans_open(const ans_open_t *input)
{
    ans_client_t *  client;
    int32_t         seed;
    int64_t         time;

    int  i;
    int  result;
    int  app_length;
    int  type_length;
    int  made_locks;

    const char *  device_type;

    log_info("ans_open(%s)", input->clusterName);

    client = (ans_client_t*) malloc(sizeof(*client));

    if (client == null) {
        log_error_1("The client malloc failed.");
        return null;
    }

    memset(client, 0, sizeof(*client));

    made_locks = false;

#if defined(ANDROID)
    device_type = "android";
#elif defined(IOS)
    device_type = "ios";
#elif defined(VPL_PLAT_IS_WINRT)
    device_type = "winrt";
#else
    device_type = "pc";
#endif

    //type_length = strlen(input->deviceType) + 1;
    type_length = strlen(device_type) + 1;
    app_length  = strlen(input->application) + 1;

    client->cluster        = (char *)  malloc(strlen(input->clusterName) + 1);
    client->blob           = (char *)  malloc(input->blobLength);
    client->blob_length    =           input->blobLength;
    client->key            = (char *)  malloc(input->keyLength);
    client->key_length     =           input->keyLength;
    client->challenge      =           null;
    client->server_address =           VPLNET_ADDR_INVALID;
    client->device_type    = (char *)  malloc(type_length);
    client->application    = (char *)  malloc(app_length);
    client->callback_table =           *input->callbacks;
    client->verbose        =           input->verbose;
    client->cleanup        =           cleanup_not_set;
    client->stop_now       =           VPL_FALSE;
    client->out_sequence   =           0;
    client->packets_in     =           0;
    client->crypto         =           null;
    client->tcp_head       =           null;
    client->subscriptions  =           null;
    client->host_name      =           null;
    client->ready_to_send  =           VPL_FALSE;
    client->is_foreground  =           VPL_TRUE;
    client->mt.inited      =           VPL_FALSE;
    client->keep_port      =           ans_default_keep_port;
    client->server_tcp_port =          (input->server_tcp_port == 0)? ANS_DEFAULT_SERVER_TCP_PORT : input->server_tcp_port;

    /*
     *  Check whether any of the mallocs failed.
     */
    if
    (
        client->cluster     == null
    ||  client->key         == null
    ||  client->blob        == null
    ||  client->device_type == null
    ||  client->application == null
    ) {
        log_error_1("ans_open failed in malloc");
        fail_open(client, made_locks);
        return null;
    }

    time = VPLTime_GetTime();
    seed = time ^ (time << 11) ^ (time >> 19);
    mtwist_init(&client->mt, seed);

    strcpy(client->cluster,     input->clusterName);
    memcpy(client->key,         input->key,         client->key_length);
    memcpy(client->blob,        input->blob,        client->blob_length);
    memcpy(client->device_type, device_type,        type_length);
    memcpy(client->application, input->application, app_length);
    //memcpy(client->device_type, input->deviceType,  type_length);

    memset(&client->socket, -1, sizeof(client->socket));
    init_event(&client->event);

    client->keep.client           = client;
    client->keep.live_socket_id   = -1;
    client->keep.dead_socket_id   = -1;
    client->keep.declared_dead    = false;
    client->keep.done             = false;
    client->keep.in_sequence      = -1;
    client->keep.time_to_millis   = VPLTime_FromMillisec(1);
    client->next_socket_id        = 1;

    init_event(&client->keep.event);

    for (i = 0; i < array_size(client->keep.ping_id); i++) {
        client->keep.ping_id[i] = -1;
    }

    client->keep.socket = make_udp_socket(client);

    if (invalid(client->keep.socket) || --ans_force_fail == 0) {
        log_error_1("VPLSocket_Create(keep-alive) failed");
        fail_open(client, made_locks);
        return null;
    }

    /*
     *  Initialize the mutexes.
     */
    make_locks(client);
    made_locks = true;

    make_event(&client->event);
    make_event(&client->keep.event);

    /*
     *  Start the keep-alive thread.
     */
    result = VPLDetachableThread_Create(&client->keep_handle, &run_keep_alive,
                 &client->keep, null, "ans");

    if (result != VPL_OK) {
        log_error("The keep-alive thread creation failed: %d", result);
        fail_open(client, made_locks);
        return null;
    }

    client->keep_started = true;

    /*
     *  Start the device thread.
     */
    result = VPLDetachableThread_Create(&client->device_handle, &run_device,
                 client, null, "ans");

    if (result != VPL_OK) {
        log_error("The device thread creation failed: %d", result);
        fail_open(client, made_locks);
        return null;
    }

    client->open = open_magic;
    log_always("client %p is now live for %s", client, input->clusterName);
    return client;
}

VPL_BOOL
ans_setSubscriptions
(
    ans_client_t *    client,
    uint64_t          asyncId,
    const uint64_t *  devices,
    int               count
)
{
    int         bytes;
    uint64_t *  subscriptions;
    int         defer;
    int         result;
    describe_t  describe;

    if (count < 0 || (count > ans_max_subs && ans_max_subs > 0)) {
        log_error("The subscription count (%d of %d) is invalid.",
            (int) count, ans_max_subs);
        client->callback_table.rejectSubscriptions(client);
        return VPL_FALSE;
    }

    log_always("setting %d subscription%s for client %p, device " FMTu64,
        count, count == 1 ? "" : "s", client, client->device_id);

    /*
     *  Save a copy of the subscriptions.
     */
    bytes = count * sizeof(devices[0]);
    subscriptions = (uint64_t *) malloc(bytes);

    if (subscriptions == null) {
        log_error_1("failed subscriptions malloc");
        return VPL_FALSE;
    }

    memcpy(subscriptions, devices, bytes);

    for (int i = 0; i < count; i++) {
        log_info("declare subscriptions[%d] = " FMTu64,
            i, subscriptions[i]);
    }

    /*
     *  Install the new subscriptions and check whether we
     *  can send them to the server now.
     */
    mutex_lock(&client->mutex);

    free(client->subscriptions);

    client->subscriptions      = subscriptions;
    client->subscription_count = count;

    defer = ans_max_subs <= 0;

    mutex_unlock(&client->mutex);

    /*
     *  Check the defer flag and send the subscriptions
     *  if the connection is ready.
     */
    if (defer) {
        result = VPL_TRUE;
    } else {
        clear_describe(&describe);
        describe.type     = Send_subscriptions;
        describe.data     = (void *) devices;
        describe.count    = bytes;
        describe.async_id = asyncId;
        result = send_message(client, &describe);
    }

    return result;
}

VPL_BOOL
ans_requestWakeup(ans_client_t *client, uint64_t device)
{
    describe_t  describe;

    log_always("requesting a wakeup via client %p", client);
    clear_describe(&describe);
    describe.type      = Request_wakeup;
    describe.device_id = device;
    return send_message(client, &describe);
}

VPL_BOOL
ans_requestDeviceState
(
    ans_client_t *    client,
    uint64_t          async_id,
    const uint64_t *  devices,
    int               count
)
{
    describe_t  describe;

    if (ans_max_query <= 0) {
        log_info_1("ans_requestDeviceState:  deferred");
        return VPL_FALSE;
    }

    if
    (
        devices == null
    ||  count <= 0
    ||  count > ans_max_query
    ) {
        log_error("That device list is invalid (%d).", (int) count);
        return VPL_FALSE;
    }

    log_always("requesting device state via client %p", client);

    clear_describe(&describe);
    describe.type  = Query_device_list;
    describe.data  = (void *) devices;
    describe.count = count * sizeof(devices[0]);
    return send_message(client, &describe);
}

VPL_BOOL
ans_requestSleepSetup
(
    ans_client_t *  client,
    uint64_t        async_id,
    int             ioac_type,
    const void *    macAddress,
    int             macLength
)
{
    describe_t  describe;

    if (macAddress == null || macLength > 100) {
        log_error_1("That MAC address is invalid.");
        return VPL_FALSE;
    }

    log_always("requesting a sleep setup via client %p", client);

    clear_describe(&describe);
    describe.type      = Request_sleep_setup;
    describe.data      = (void *) macAddress;
    describe.count     = macLength;
    describe.ioac      = ioac_type;
    describe.async_id  = async_id;  /* force a response */

    return send_message(client, &describe);
}

VPL_BOOL
ans_sendUnicast
(
    ans_client_t *  client,
    uint64_t        user_id,
    uint64_t        device_id,
    const void *    message,
    int             count,
    uint64_t        async_id
)
{
    describe_t  describe;

    clear_describe(&describe);

    describe.type       = Send_unicast;
    describe.user_id    = user_id;
    describe.device_id  = device_id;
    describe.data       = (void *) message;
    describe.count      = count;
    describe.async_id   = async_id | 1;  /* force a response */

    return send_message(client, &describe);
}

VPL_BOOL
ans_sendMulticast
(
    ans_client_t *  client,
    uint64_t        user_id,
    const void *    message,
    int             count,
    uint64_t        async_id
)
{
    describe_t  describe;

    clear_describe(&describe);

    describe.type       = Send_multicast;
    describe.user_id    = user_id;
    describe.data       = (void *) message;
    describe.count      = count;
    describe.async_id   = async_id;

    return send_message(client, &describe);
}

VPL_BOOL
ans_setForeground(ans_client_t *client, VPL_BOOL isForeground, uint64_t interval)
{
    int  tries;

    if (client->open != open_magic) {
        log_error_1("ans_setForeground was called on an invalid client");
        return VPL_FALSE;
    }

    log_always("ans_setForeground(client %p, %s)", client,
        isForeground ? "foreground" : "background");

    mutex_lock(&client->mode_mutex);

    /*
     *  Requests that don't change the mode are errors.
     */
    if (client->is_foreground && isForeground) {
        mutex_unlock(&client->mode_mutex);
        return VPL_FALSE;
    }

    if (!client->is_foreground && !isForeground) {
        mutex_unlock(&client->mode_mutex);
        return VPL_FALSE;
    }

    /*
     *  Okay, set the new mode.
     */
    client->is_foreground = isForeground;

    if (isForeground) {
        client->interval = 0;
        ping_device(client);
        ping_keep_alive(client);

        /*
         *  Wait until the keep-alive thread has recognized the new
         *  state, or until we run out of time.
         */
        tries = 0;

        while (!client->keep.keep_foreground && tries++ < 50) {
            VPLThread_Sleep(VPLTime_FromMillisec(100));
        }
    } else {
        client->interval = interval;

        client->keep.last_tcp_receive = VPLTime_GetTime();

        ping_device(client);
        ping_keep_alive(client);
        send_tcp_ping(&client->keep, "set foreground start");

        /*
         *  Wait until the keep-alive thread has recognized the new
         *  state, or until we run out of time.
         */
        tries = 0;

        while (client->keep.keep_foreground && tries++ < 50) {
            ping_keep_alive(client);
            VPLThread_Sleep(VPLTime_FromMillisec(100));
        }

        /*
         *  Send another ping to account for the time we spent waiting for
         *  keep-alive to stop.  This packet is mostly redundant.
         */
        send_tcp_ping(&client->keep, "set foreground end");
    }

    log_test("leaving ans_setForeground");
    mutex_unlock(&client->mode_mutex);
    return VPL_TRUE;
}

/*
 *  This routine is invoked periodically when the program is in
 *  the background state, typically on a phone or other mobile
 *  device.  It handles user-level keep-alive, which is done via
 *  TCP/IP while the process is in the background.  The operating
 *  system sets the frequency at which this routine is invoked.
 *  The client program communicates this interval via the
 *  ans_setForeground routine when the device enters background
 *  mode.
 */
VPL_BOOL
ans_background(ans_client_t *client)
{
    int64_t  current_time;
    int64_t  time_limit;
    int64_t  interval;
    int64_t  deadline;
    int32_t  my_socket;
    int64_t  background_limit;
    int      done;

    if (client->open != open_magic) {
        log_error_1("ans_background was called on an invalid client");
        return false;
    }

    if (client->is_foreground) {
        log_error_1("ans_background was called on a foreground client");
        return false;
    }

    my_socket = client->keep.live_socket_id;

    if (my_socket == -1) {
        log_info_1("ans_background has no live socket");
        return true;
    }

    /*
     *  Send the next ping packet right away.  Maybe it'll get
     *  back before we're done here.
     */
    send_tcp_ping(&client->keep, "background");

    /*
     *  Ping the device client to make sure it checks for I/O,
     *  then wait to give it a chance to do some worh.
     */
    ping_device(client);
    VPLThread_Sleep(VPLTime_FromSec(1));

    /*
     *  Okay, now check the timeout limit.  If it's expired, declare
     *  a timeout and give the device thread a chance to do its job.
     */
    do {
        background_limit = VPLTime_FromSec(max(1, ans_ping_back));
        current_time     = VPLTime_GetTime();
        interval         = client->interval;
        time_limit       = interval + background_limit;
        deadline         = client->keep.last_tcp_receive + time_limit;

        /*
         *  check whether we have a timeout.
         */
        done = current_time >= deadline && interval > 0 && time_limit > 0;

        if (done) {
            log_info("background timeout:  time " FMTs64 ", time_limit " FMTs64
                ", interval " FMTs64 ", deadline " FMTs64,
                current_time,
                time_limit,
                (int64_t) client->interval,
                deadline);
            declare_timeout(&client->keep, my_socket, "background");
            VPLThread_Sleep(VPLTime_FromSec(1));
        }

        /*
         *  If the deadline is far in the future, we're done.
         */
        if (!done) {
            done = deadline - current_time > background_limit;
        }

        /*
         *  If we have more work to do, wait a second and then check the
         *  status again.
         */
        if (!done) {
            ans_back_loops++;
            VPLThread_Sleep(VPLTime_FromSec(1));
        }
    } while (!done);

    return true;
}

VPL_BOOL
ans_setVerbose(ans_client_t *client, VPL_BOOL verbose)
{
    if (client->open != open_magic) {
        log_error_1("ans_setVerbose was called on an invalid client");
        return false;
    }

    client->verbose = verbose;
    return true;
}

VPL_BOOL
ans_onNetworkConnected(ans_client_t *client)
{
    log_always("declaring a network status change for client %p", client);
    client->network_changed = VPL_TRUE;
    ping_device(client);
    return VPL_TRUE;
}

VPL_BOOL
ans_declareNetworkChange(ans_client_t *client, VPL_BOOL dropConnection)
{
    log_always("declaring a network status change for client %p", client);
    client->network_changed = VPL_TRUE;
    client->force_close     = dropConnection;
    ping_device(client);
    return VPL_TRUE;
}

void
ans_close(ans_client_t *client, VPL_BOOL waitForShutdown)
{
    int  result;

    if (client->open != open_magic) {
        ans_invalid_closes++;
        log_error_1("ans_close was called on an invalid client");
        return;
    }

    log_always("ans_close(%p, %s, %s)", client, client->cluster,
        waitForShutdown ? "wait" : "async");

    client->open = 0;
    stop_keep_alive(client, false);

    if (!waitForShutdown) {
        VPLDetachableThread_Detach(&client->device_handle);

        client->cleanup  = cleanup_async;
        client->stop_now = VPL_TRUE;

        ping_device(client);
    } else {
        client->cleanup  = cleanup_wait;
        client->stop_now = VPL_TRUE;

        ping_device(client);

        result = VPLDetachableThread_Join(&client->device_handle);

        if (result != VPL_OK) {
            log_error("The join with the device thread failed: %d", result);
            /* leak memory */
        } else {
            free(client);
        }
    }

    log_info_1("ans_close done");
}

#undef  size
#define size(mt)  (sizeof(mt->state) / sizeof(mt->state[0]))

/*
 *  Implement the basic Mersenne Twister pseudo-random number
 *  generator.
 */
static void
mtwist_init(mtwist_t *mt, uint32_t seed)
{
    mt->index    = 0;
    mt->state[0] = seed;

    for (int i = 1; i < size(mt); i++) {
        mt->state[i] = (1812433253 * (mt->state[i-1]) ^ (mt->state[i-1] >> 30)) + i;
    }

    mt->inited = true;
}

/*
 *  Generate an array of untempered random numbers.
 */
static void
mtwist_generate(mtwist_t *mt)
{
    uint32_t  data;

    for (int i = 0; i < size(mt); i++) {
        data          = mt->state[i] & 0x80000000;
        data         |= mt->state[(i +   1) % size(mt)] & 0x7fffffff;
        mt->state[i]  = mt->state[(i + 397) % size(mt)] ^ (data >> 1);

        if (data & 1) {
            mt->state[i] ^= 0x9908b0df;
        }
    }
}

/*
 *  Generate the next random number for the twister.  The limit parameter,
 *  if positive, specifies the interval for the random number, i.e., the
 *  result will be in the range [0, limit).  A negative limit value
 *  disables this feature.
 */
static int32_t
mtwist_next(mtwist_t *mt, int32_t limit)
{
    uint32_t  next;

    if (limit == 1 || limit == 0) {
        return 0;
    }

    /*
     *  Generate an array of untempered values if the array
     *  has been used.
     */
    if (mt->index == 0) {
        mtwist_generate(mt);
    }

    /*
     *  Now temper the next value.
     */
    next  = mt->state[mt->index];

    next ^= (next >> 11);
    next ^= (next <<  7) & 0x9d2c5680;
    next ^= (next << 15) & 0xefc60000;
    next ^= (next >> 18);

    /*
     *  Increment the index to the next random number and wrap if
     *  needed.
     */
    mt->index++;

    if (mt->index >= size(mt)) {
        mt->index = 0;
    }

    if (limit > 1) {
        next &= 0x7fffffff; /* make next non-negative */
        next %= limit;
    }

    return next;
}
