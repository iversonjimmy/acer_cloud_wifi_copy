//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#include <stdint.h>
#include <vplu.h>
#include <vpl_conv.h>
#include <vpl_net.h>
#include <vpl_th.h>
#include <vpl_user.h>
#include <vplex_assert.h>
#include <vplex_time.h>
#include <cslsha.h>
#include <aes.h>
#include <log.h>
#include <vpl_socket.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <csltypes.h>
#include <cslsha.h>
#include <aes.h>
#include <unistd.h>
#include <fcntl.h>

static void *  my_malloc(size_t);
static int     my_aes_SwEncrypt(u8 *, u8 *, u8 *, int, u8 *);
static int     my_aes_SwDecrypt(u8 *, u8 *, u8 *, int, u8 *);
static int     my_VPLSocket_Recv(VPLSocket_t, void *, int);
static int     my_VPLSocket_Poll(VPLSocket_poll_t *, int, VPLTime_t);

#define malloc          my_malloc
#define aes_SwEncrypt   my_aes_SwEncrypt
#define aes_SwDecrypt   my_aes_SwDecrypt
#define VPLSocket_Recv  my_VPLSocket_Recv
#define VPLSocket_Poll  my_VPLSocket_Poll

#include "pxd_test.h"
#include "pxd_event.h"
#include "pxd_log.cpp"
#include "pxd_mtwist.cpp"
#include "pxd_event.cpp"
#include "pxd_util.cpp"
#include "pxd_packet.cpp"

#undef malloc
#undef aes_SwEncrypt
#undef aes_SwDecrypt
#undef VPLSocket_Recv
#undef VPLSocket_Poll

#define streq(a, b)    (strcmp((a), (b)) == 0)

static char            data[]           = "Hello, world!";
static char            region[]         = "test region";
static char            instance_id[]    = "test instance";
static char            test_cluster[]   = "pxd.test.broadon.com";
static char            ans_dns[]        = "ans dns";
static char            pxd_dns[]        = "pxd dns";
static char            key[20]          = "This is a key";
static uint64_t        request_id       = 1024;
static uint64_t        user_id          = 2048;
static uint64_t        device_id        = 4096;
static uint16_t        version          = 1;
static char            challenge[16]    = "challenge!";
static char            address[4]       = "=ip";
static char            blob[256]        = "fake blob";

static char            list_address_1[15] = "list addr 1";
static int32_t         list_port_1        = 32768;
static char            list_address_2[15] = "list addr 2";
static int32_t         list_port_2        = 65536;
static int             malloc_countdown   = 0;
static int             receive_error      = 0;
static int             poll_error         = 0;
static int             poll_revents       = 0;
static int             poll_index         = 0;
static int             fail_encrypt;
static int             fail_decrypt;

static int             client_user        = 7000;
static int             client_device      = 7887;
static int             server_user        = 8000;
static int             server_device      = 8998;
static char            proxy_key[20]      = "proxy_key";
static char            client_handle[]    = "handle";
static char            service_id[]       = "service id";
static char            ticket[]           = "ticket";
static char            extra[]            = "extra field";
static char            client_instance[]  = "client instance";
static char            server_instance[]  = "server instance longer";

static pxd_address_t   address_list[2];

static pxd_crypto_t *  shared_crypto;
static pxd_io_t        shared_io;
static int64_t         shared_sequence;

static void
test_name(int type, const char *expected)
{
    if (!streq(pxd_packet_type(type), expected)) {
        log("pxd_packet_type(%d) failed\n", type);
        exit(1);
    }
}

static void
test_type_names(void)
{
    test_name(0, "unknown");

    test_name(Reject_pxd_credentials,   "Reject_pxd_credentials");
    test_name(Send_pxd_login,           "Send_pxd_login");
    test_name(Declare_server,           "Declare_server");
    test_name(Query_server_declaration, "Query_server_declaration");
    test_name(Send_server_declaration,  "Send_server_declaration");
    test_name(Start_connection_attempt, "Start_connection_attempt");
    test_name(Start_proxy_connection,   "Start_proxy_connection");
    test_name(Send_pxd_challenge,       "Send_pxd_challenge");
    test_name(Send_pxd_response,        "Send_pxd_response");
    test_name(Send_ccd_login,           "Send_ccd_login");
    test_name(Send_ccd_challenge,       "Send_ccd_challenge");
    test_name(Reject_ccd_credentials,   "Reject_ccd_credentials");
    test_name(Send_ccd_response,        "Send_ccd_response");
    test_name(Set_pxd_configuration,    "Set_pxd_configuration");

    test_name(Send_unicast,             "Send_unicast");
    test_name(Send_multicast,           "Send_multicast");
    test_name(Send_subscriptions,       "Send_subscriptions");
    test_name(Send_challenge,           "Send_challenge");
    test_name(Send_ping,                "Send_ping");
    test_name(Send_timed_ping,          "Send_timed_ping");
    test_name(Send_login_blob,          "Send_login_blob");
    test_name(Set_device_params,        "Set_device_params");
    test_name(Request_sleep_setup,      "Request_sleep_setup");
    test_name(Send_sleep_setup,         "Send_sleep_setup");
    test_name(Request_wakeup,           "Request_wakeup");
    test_name(Reject_credentials,       "Reject_credentials");
    test_name(Set_login_version,        "Set_login_version");
    test_name(Set_param_version,        "Set_param_version");
    test_name(Query_device_list,        "Query_device_list");
    test_name(Send_response,            "Send_response");
    test_name(Send_device_update,       "Send_device_update");
    test_name(Send_device_shutdown,     "Send_device_shutdown");
    test_name(Send_state_list,          "Send_state_list");
    test_name(Set_device_compat,        "Set_device_compat");
    test_name(Send_state_compat,        "Send_state_compat");
}

static void
test_random(void)
{
    pxd_crypto_t *  crypto;

    int  zeros;

    zeros = 0;

    crypto = pxd_create_crypto(key, sizeof(key), 123);

    for (int i = 0; i < 100; i++) {
        if (pxd_random(crypto) == 0) {
            zeros++;
        }
    }

    if (zeros > 1) {
        log("pxd_random produced %d zeros\n", zeros);
        exit(1);
    }

    pxd_free_crypto(&crypto);
}

static void
test_response(int response, const char *expected)
{
    if (!streq(pxd_response(response), expected)) {
        log("pxd_response(%d) failed\n", response);
        exit(1);
    }
}

static void
test_responses(void)
{
    test_response(DEVICE_ONLINE,        "online");
    test_response(DEVICE_SLEEPING,      "sleeping");
    test_response(DEVICE_OFFLINE,       "offline");
    test_response(QUERY_FAILED,         "query failed");
    test_response(pxd_op_successful,    "successful");
    test_response(0,                    "succeeded");
    test_response(pxd_timed_out,        "timed out");
    test_response(pxd_connection_lost,  "connection lost");
    test_response(pxd_op_failed,        "op failed");

    test_response(pxd_credentials_rejected,  "credentials rejected");
    test_response(pxd_quota_exceeded,        "quota exceeded");
}

static void
test_append(void)
{
    char      buffer[200];
    int       length;
    char *    end;
    char *    base;
    char      v8;
    uint16_t  v16;
    uint32_t  v32;
    uint64_t  v64;

    length = packed_string_length(data);
    end    = buffer + 200;
    base   = buffer;

    append_string(&base, end, data);

    if (base - buffer != length) {
        log("append_string sequence failed!\n");
        exit(1);
    }

    length = packed_bytes_length(sizeof(data));
    end    = buffer + 200;
    base   = buffer;

    append_bytes(&base, end, data, sizeof(data));

    if (base - buffer != length) {
        log("append_bytes sequence failed!\n");
        exit(1);
    }

    v8   = 0;
    v16  = 0;
    v32  = 0;
    v64  = 0;

    base = buffer;
    append_byte(&base, end, v8);

    if (base - buffer != sizeof(v8)) {
        log("append_byte failed!\n");
        exit(1);
    }

    base = buffer;
    append_short(&base, end, v16);

    if (base - buffer != sizeof(v16)) {
        log("append_short failed!\n");
        exit(1);
    }

    base = buffer;
    append_int(&base, end, v32);

    if (base - buffer != sizeof(v32)) {
        log("append_int failed!\n");
        exit(1);
    }

    base = buffer;
    append_long(&base, end, v64);

    if (base - buffer != sizeof(v64)) {
        log("append_long failed!\n");
        exit(1);
    }

    base = end;

    if (!append_byte(&base, end, v8)) {
        log("append_byte failed to detect a short buffer!\n");
        exit(1);
    }

    if (!append_short(&base, end, v16)) {
        log("append_short failed to detect a short buffer!\n");
        exit(1);
    }

    if (!append_int(&base, end, v32)) {
        log("append_int failed to detect a short buffer!\n");
        exit(1);
    }

    if (!append_long(&base, end, v64)) {
        log("append_long failed to detect a short buffer!\n");
        exit(1);
    }

    if (!append_bytes(&base, end, data, sizeof(data))) {
        log("append_bytes failed to detect a short buffer!\n");
        exit(1);
    }

    if (!append_string(&base, end, "hello")) {
        log("append_string failed to detect a short buffer!\n");
        exit(1);
    }
}

static int
memeq(const char *a, int a_length, const char *b, int b_length)
{
    int  pass;

    pass = a_length == b_length;

    if (pass) {
        pass = memcmp(a, b, a_length) == 0;
    }

    return pass;
}

#define check_fixed(a, b, s, t)                            \
            do {                                           \
                if ((a) != (b)) {                          \
                    log("field %s failed on type %d\n",    \
                        (s), (int) (t));                   \
                    exit(1);                               \
                }                                          \
            } while (0)

#define check_string(a, b, s, t)                           \
            do {                                           \
                if (!streq((a), (b))) {                    \
                    log("field %s failed on type %d\n",    \
                        (s), (int) (t));                   \
                    exit(1);                               \
                }                                          \
            } while (0)

#define check_bytes(a, a_len, b, b_len, s, t)              \
            do {                                           \
                if (!memeq((a), (a_len), (b), (b_len))) {  \
                    log("field %s failed on type %d\n",    \
                        (s), (int) (t));                   \
                    exit(1);                               \
                }                                          \
            } while (0)

static void
make_unpacked(pxd_unpacked_t *unpacked)
{
    int64_t  test_tag;
    int64_t  async_id;

    test_tag = 0x0102030405060708LL;
    async_id = 0x1102030405060718LL;

    memset(unpacked, 0, sizeof(*unpacked));

    unpacked->version      = version;
    unpacked->region       = region;
    unpacked->user_id      = user_id;
    unpacked->device_id    = device_id;
    unpacked->instance_id  = instance_id;
    unpacked->request_id   = request_id;
    unpacked->async_id     = async_id;
    unpacked->instance_id  = instance_id;
    unpacked->pxd_dns      = pxd_dns;
    unpacked->ans_dns      = ans_dns;
    unpacked->port         = 0x01020304;

    unpacked->challenge          = challenge;
    unpacked->challenge_length   = sizeof(challenge);

    unpacked->address            = address;
    unpacked->address_length     = sizeof(address);

    unpacked->blob               = blob;
    unpacked->blob_length        = sizeof(blob);
    unpacked->connection_tag     = 885551212;

    unpacked->proxy_retries      = 100;
    unpacked->proxy_wait         = 101;
    unpacked->idle_limit         = 102;
    unpacked->sync_io_timeout    = 103;
    unpacked->min_delay          = 104;
    unpacked->max_delay          = 105;
    unpacked->thread_retries     = 106;
    unpacked->max_packet_size    = 107;
    unpacked->max_encrypt        = 108;
    unpacked->partial_timeout    = 109;

    unpacked->address_count           = 2;
    unpacked->addresses               = address_list;
    unpacked->addresses[0].ip_address = list_address_1;
    unpacked->addresses[0].ip_length  = sizeof(list_address_1);
    unpacked->addresses[0].port       = list_port_1;
    unpacked->addresses[0].type       = 1;

    unpacked->addresses[1].ip_address = list_address_2;
    unpacked->addresses[1].ip_length  = sizeof(list_address_2);
    unpacked->addresses[1].port       = list_port_2;
    unpacked->addresses[1].type       = 1;

    shared_io.tag = unpacked->connection_tag;
}

static void
test_reject(int type)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;
    
    int64_t  test_tag;

    test_tag = 0x0102030405060708LL;

    memset(&input, 0, sizeof(input));
 
    input.type            = type;
    input.connection_tag  = test_tag;
    packet                = pxd_pack(&input, null, &error);
    packet->verbose       = true;
    output                = pxd_unpack(shared_crypto, packet, null, test_tag);

    if (pxd_get_type(packet) != input.type) {
        log("pxd_get_type(%d) = %d\n", input.type, pxd_get_type(packet));
        exit(1);
    }

    check_fixed(input.type,           output->type,           "type",  input.type);
    check_fixed(input.connection_tag, output->connection_tag, "tag",   input.type);

    if (!pxd_check_signature(shared_crypto, packet)) {
        log("signature check should pass on Reject_pxd_credentials\n");
        exit(1);
    }

    pxd_free_packet  (&packet);
    pxd_free_unpacked(&output);

    /*
     *  pxd_free_unpacked should be okay with a double call.
     */
    pxd_free_unpacked(&output);
}

static void
test_login(int type)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    uint64_t  sequence;
    int       length;

    make_unpacked(&input);
 
    input.type = type;
    packet     = pxd_pack(&input, null, &error);

    pxd_prep_packet(&shared_io, packet, &shared_sequence);

    if (!pxd_check_signature(shared_crypto, packet)) {
        log("signature failure on type %d\n", (int) input.type);
        exit(1);
    }

    shared_sequence--;
    output = pxd_unpack(shared_crypto, packet, &shared_sequence, input.connection_tag);

    if (pxd_get_type(packet) != type) {
        log("pxd_get_type(%d) = %d\n", type, pxd_get_type(packet));
        exit(1);
    }

    check_fixed(input.type,    output->type,    "type",     input.type);
    check_fixed(input.version, output->version, "version",  input.type);

    check_bytes
    (
        input.challenge,    input.challenge_length,
        output->challenge,  output->challenge_length,
        "challenge",        input.type
    );

    check_bytes
    (
        input.blob,    input.blob_length,
        output->blob,  output->blob_length,
        "blob",        input.type
    );

    if (type == Send_pxd_login) {
        check_string(input.pxd_dns, output->pxd_dns, "pxd dns", input.type);
    }

    /*
     *  pxd_unpack should complain about the sequence id...
     */
    pxd_free_unpacked(&output);
    output = pxd_unpack(shared_crypto, packet, &shared_sequence, input.connection_tag);

    if (output != null)  {
        log("pxd_unpack should complain about a bad sequence id.\n");
        exit(1);
    }

    packet->base[0]++;

    if (pxd_check_signature(shared_crypto, packet)) {
        log("pxd_check_signature accepted an altered packet.\n");
        exit(1);
    }

    packet->base[0]--;

    /*
     *  Check to make sure that a short length is detected.
     */
    length         = packet->length;
    packet->length = header_size + signature_size - 1;

    if (pxd_check_signature(shared_crypto, packet)) {
        log("pxd_check_signature accepted a short packet.\n");
        exit(1);
    }

    packet->length = length;

    /*
     *  pxd_prep_packet should be okay being called twice.
     */
    sequence = shared_sequence;
    pxd_prep_packet(&shared_io, packet, &shared_sequence);

    if (shared_sequence != sequence) {
        log("pxd_prep_packet didn't detect a double call\n");
        exit(1);
    }

    pxd_free_packet(&packet);
}

static void
test_challenge(int type)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = type;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    if (pxd_get_type(packet) != type) {
        log("pxd_get_type(%d) = %d\n", type, pxd_get_type(packet));
        exit(1);
    }

    check_fixed (input.type,    output->type,          "type",     input.type);
    check_fixed (input.version, output->version,       "version",  input.type);
    check_string(input.pxd_dns,       output->pxd_dns, "pxd dns",  input.type);

    check_fixed (input.connection_id, output->connection_id, "connection id",  input.type);

    check_bytes
    (
        input.challenge,    input.challenge_length,
        output->challenge,  output->challenge_length,
        "challenge",        input.type
    );

    check_fixed(input.connection_time, output->connection_time, "connection time",  input.type);

    check_bytes
    (
        input.address,    input.address_length,
        output->address,  output->address_length,
        "address",        input.type
    );

    check_fixed(input.port, output->port, "port",  input.type);

    pxd_free_packet(&packet);
    pxd_free_unpacked(&output);
}

static void
test_send_response(int type)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type         = type;
    input.response     = 42;
    input.out_sequence = 32;
    packet             = pxd_pack(&input, null, &error);
    output             = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    if (pxd_get_type(packet) != type) {
        log("pxd_get_type(%d) = %d\n", type, pxd_get_type(packet));
        exit(1);
    }

    check_fixed(input.type,     output->type,     "type",      input.type);
    check_fixed(input.response, output->response, "response",  input.type);

    check_fixed(input.out_sequence, output->out_sequence, "sequence",  input.type);

    pxd_free_packet(&packet);
    pxd_free_unpacked(&output);
}

static void
test_extended_response(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type            = Send_ccd_response;
    input.response        = 42;
    input.out_sequence    = 32;
    input.address         = list_address_1;
    input.address_length  = sizeof(list_address_1);
    input.port            = 10101;
    packet                = pxd_pack(&input, null, &error);
    output                = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    if (pxd_get_type(packet) != Send_ccd_response) {
        log("pxd_get_type(%d) = %d\n", Send_ccd_response, pxd_get_type(packet));
        exit(1);
    }

    check_fixed(input.type,     output->type,     "type",      input.type);
    check_fixed(input.response, output->response, "response",  input.type);
    check_fixed(input.port,     output->port,     "port",      input.type);

    check_fixed(input.out_sequence, output->out_sequence, "sequence",  input.type);
    check_string(input.address, output->address,  "address",  input.type);

    pxd_free_packet(&packet);
    pxd_free_unpacked(&output);
}

static void
check_addresses(pxd_unpacked_t *input, pxd_unpacked_t *output)
{
    int  pass;
    int  i;

    pass = input->address_count == output->address_count;

    if (!pass) {
        log("The address counts didn't match - %d vs %d\n",
            (int) input->address_count, (int) output->address_count);
        exit(1);
    }

    if (pass) {
        for (i = 0; i < input->address_count; i++) {
            check_bytes
            (
                input ->addresses[i].ip_address,
                input ->addresses[i].ip_length,
                output->addresses[i].ip_address,
                output->addresses[i].ip_length,
                "ip addresses",
                (int) input->type
            );

            check_fixed
            (
                input ->addresses[i].port,
                output->addresses[i].port,
                "ip port",
                (int) input->type
            );
        }
    }
}

static void
test_declare(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = Declare_server;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    if (pxd_get_type(packet) != input.type) {
        log("pxd_get_type(%d) = %d\n", input.type, pxd_get_type(packet));
        exit(1);
    }

    check_fixed (input.type,          output->type,          "type",      input.type);
    check_fixed (input.version,       output->version,       "version",   input.type);
    check_string(input.region,        output->region,        "region",    input.type);
    check_fixed (input.user_id,       output->user_id,       "user id",   input.type);
    check_fixed (input.device_id,     output->device_id,     "device id", input.type);
    check_string(input.instance_id,   output->instance_id,   "instance",  input.type);
    check_string(input.ans_dns,       output->ans_dns,       "ans dns",   input.type);
    check_string(input.pxd_dns,       output->pxd_dns,       "pxd dns",   input.type);
    check_fixed (input.address_count, output->address_count, "device id", input.type);

    check_addresses(&input, output);

    pxd_free_packet(&packet);
    pxd_free_unpacked(&output);
}

static void
test_query(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = Query_server_declaration;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    if (pxd_get_type(packet) != input.type) {
        log("pxd_get_type(%d) = %d\n", input.type, pxd_get_type(packet));
        exit(1);
    }

    check_fixed (input.type,        output->type,        "type",         input.type);
    check_fixed (input.version,     output->version,     "version",      input.type);
    check_string(input.region,      output->region,      "region",       input.type);
    check_fixed (input.user_id,     output->user_id,     "user id",      input.type);
    check_fixed (input.device_id,   output->device_id,   "device id",    input.type);
    check_string(input.instance_id, output->instance_id, "instance id",  input.type);

    pxd_free_packet(&packet);
    pxd_free_unpacked(&output);
}

static void
test_send(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = Send_server_declaration;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    check_fixed (input.type,        output->type,        "type",         input.type);
    check_fixed (input.version,     output->version,     "version",      input.type);
    check_string(input.region,      output->region,      "region",       input.type);
    check_fixed (input.user_id,     output->user_id,     "user id",      input.type);
    check_fixed (input.device_id,   output->device_id,   "device id",    input.type);
    check_string(input.instance_id, output->instance_id, "instance id",  input.type);
    check_string(input.ans_dns,     output->ans_dns,     "ans_dns",      input.type);
    check_string(input.pxd_dns,     output->pxd_dns,     "pxd_dns",      input.type);

    check_addresses(&input, output);

    /*
     *  Force a malloc error while we've got a packet.
     */
    pxd_free_unpacked(&output);
    malloc_countdown = 1;

    output = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    if (output != null) {
        log("pxd_unpack didn't trigger a malloc failure.\n");
        exit(1);
    }

    pxd_free_packet(&packet);
}

static void
test_start_proxy(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = Start_proxy_connection;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    check_fixed (input.type,        output->type,        "type",         input.type);
    check_fixed (input.version,     output->version,     "version",      input.type);
    check_fixed (input.user_id,     output->user_id,     "user id",      input.type);
    check_fixed (input.device_id,   output->device_id,   "device id",    input.type);
    check_string(input.instance_id, output->instance_id, "instance id",  input.type);
    check_fixed (input.request_id,  output->request_id,  "request id",   input.type);

    pxd_free_packet(&packet);
    pxd_free_unpacked(&output);
}

static void
test_start_connection(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = Start_connection_attempt;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    check_fixed (input.type,        output->type,        "type",         input.type);
    check_fixed (input.version,     output->version,     "version",      input.type);
    check_string(input.region,      output->region,      "region",       input.type);
    check_fixed (input.user_id,     output->user_id,     "user id",      input.type);
    check_fixed (input.device_id,   output->device_id,   "device id",    input.type);
    check_string(input.instance_id, output->instance_id, "instance id",  input.type);
    check_string(input.pxd_dns,     output->pxd_dns,     "pxd dns",      input.type);
    check_fixed (input.request_id,  output->request_id,  "request id",   input.type);

    check_addresses(&input, output);

    pxd_free_packet(&packet);
    pxd_free_unpacked(&output);
}

static void
test_send_declare(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = Send_server_declaration;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    check_fixed (input.type,        output->type,      "type",      input.type);
    check_fixed (input.version,     output->version,   "version",   input.type);
    check_fixed (input.response,    output->response,  "response",  input.type);

    check_addresses(&input, output);

    pxd_free_packet  (&packet);
    pxd_free_unpacked(&output);
}

static void
test_send_config(int send_extra)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    make_unpacked(&input);

    if (send_extra) {
        input.extra = 10;
    }
 
    input.type = Set_pxd_configuration;
    packet     = pxd_pack(&input, null, &error);
    output     = pxd_unpack(shared_crypto, packet, null, input.connection_tag);

    check_fixed (input.type,            output->type,            "type",            input.type);
    check_fixed (input.version,         output->version,         "version",         input.type);
    check_fixed (input.proxy_retries,   output->proxy_retries,   "proxy_retries",   input.type);
    check_fixed (input.proxy_wait,      output->proxy_wait,      "proxy_wait",      input.type);
    check_fixed (input.idle_limit,      output->idle_limit,      "idle limit",      input.type);
    check_fixed (input.sync_io_timeout, output->sync_io_timeout, "sync_io_timeout", input.type);
    check_fixed (input.min_delay,       output->min_delay,       "min_delay",       input.type);
    check_fixed (input.max_delay,       output->max_delay,       "max_delay",       input.type);
    check_fixed (input.thread_retries,  output->thread_retries,  "thread_retries",  input.type);
    check_fixed (input.max_packet_size, output->max_packet_size, "max_packet_size", input.type);
    check_fixed (input.max_encrypt,     output->max_encrypt,     "max_encrypt",     input.type);
    check_fixed (input.partial_timeout, output->partial_timeout, "partial_timeout", input.type);
    check_fixed (input.extra,           output->extra,           "extra",           input.type);

    pxd_free_packet  (&packet);
    pxd_free_unpacked(&output);
}

static void
test_invalid(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_error_t       error;

    make_unpacked(&input);
 
    input.type = -1;
    packet     = pxd_pack(&input, null, &error);

    if (packet != null) {
        log("packet type %d didn't fail.\n", (int) input.type);
        exit(1);
    }
}

static void
test_forced_fail(void)
{
    pxd_unpacked_t    input;
    pxd_packet_t *    packet;
    pxd_error_t       error;

    pxd_fail_pack = true;

    make_unpacked(&input);
 
    input.type = Send_pxd_challenge;
    packet     = pxd_pack(&input, null, &error);

    if (packet != null) {
        log("packet type %d didn't fail.\n", (int) input.type);
        exit(1);
    }

    pxd_fail_pack = false;
}

/*
 *  Try to create an over-sized packet.  The pxd_pack() routine
 *  should fail.
 */
static void
test_long_packet(void)
{
    char            ip_address[4];
    pxd_address_t   addresses[1000];
    pxd_error_t     error;
    pxd_unpacked_t  input;
    pxd_packet_t *  packet;

    memset(ip_address, 0, sizeof(ip_address));
    memset(&error,     0, sizeof(error));

    for (int i = 0; i < array_size(addresses); i++) {
        addresses[i].ip_address = ip_address;
        addresses[i].ip_length  = sizeof(ip_address);
        addresses[i].port       = 10;
        addresses[i].type       =  8;
    }

    make_unpacked(&input);

    input.type           = Send_server_declaration;
    input.addresses      = addresses;
    input.address_count  = array_size(addresses);

    packet = pxd_pack(&input, null, &error);

    if (packet != null || error.error != VPL_ERR_INVALID) {
        log("The long packet test failed.\n");
        exit(1);
    }
}

static void
io_kill(pxd_io_t *io, pxd_command_t command, int why)
{
    *(int *) io->opaque += 1;    

    if (command.type != Send_pxd_login) {
        log("Got a kill on packet type %d.\n", (int) command.type);
        exit(1);
    }
}

static void
make_busy(pxd_io_t *io)
{
    for (int i = 0; i < array_size(io->outstanding); i++) {
        io->outstanding[i].valid    = true;
        io->outstanding[i].async_id = i + 100;
        io->outstanding[i].type     = Send_pxd_login;
        io->outstanding[i].start    = VPLTime_GetTime() - 2 * io->time_limit;
    }
}

static void
test_queue_packet(void)
{
    pxd_io_t      io;
    pxd_packet_t  packet;
    pxd_error_t   error;
    volatile int  kill_count;

    int  pass;

    memset(&io, 0, sizeof(io));
    pxd_mutex_init(&io.mutex);
    pxd_create_event(&io.event);

    io.stop_now    = true;
    io.kill        = io_kill;
    io.opaque      = (void *) &kill_count;
    io.time_limit  = 2;
    kill_count     = 0;

    pass = pxd_queue_packet(&io, &packet, &error, "testing");

    if (pass || error.error != VPL_ERR_NOT_RUNNING) {
        log("pxd_queue_packet didn't fail on a stopped io.\n");
        exit(1);
    }

    make_busy(&io);
    memset(&packet, 0, sizeof(packet));

    packet.async_id = 5;
    packet.type     = Reject_ccd_credentials;

    error.error = 0;
    io.stop_now = false;

    pass = pxd_queue_packet(&io, &packet, &error, "testing");

    if (!pass) {
        log("pxd_queue_packet didn't succeed.\n");
        exit(1);
    }

    pxd_free_event(&io.event);

    if (kill_count != 1) {
        log("pxd_queue_packet didn't kill a command.\n");
        exit(1);
    }
}

static void
test_check_timeout(void)
{
    pxd_io_t  io;
    int       kill_count;

    memset(&io, 0, sizeof(io));
    pxd_mutex_init(&io.mutex);
    pxd_create_event(&io.event);

    io.kill        = io_kill;
    io.time_limit  = VPLTime_FromSec(2);
    io.opaque      = (void *) &kill_count;
    kill_count     = 0;

    io.outstanding[4].valid  = true;
    io.outstanding[4].start  = VPLTime_GetTime() - io.time_limit * 2;
    io.outstanding[4].type   = Send_pxd_login;

    io.outstanding[5].valid  = true;
    io.outstanding[5].start  = VPLTime_GetTime();
    io.outstanding[5].type   = Send_pxd_login;

    pxd_check_timeout(&io);

    if (kill_count != 1) {
        log("pxd_check_timeout didn't kill commands properly (%d).\n", kill_count);
        exit(1);
    }

    pxd_free_event(&io.event);
}

static void
test_packets(void)
{
    test_reject(Reject_pxd_credentials);
    test_reject(Reject_ccd_credentials);
    test_login(Send_pxd_login);
    test_login(Send_ccd_login);
    test_challenge(Send_pxd_challenge);
    test_challenge(Send_ccd_challenge);
    test_declare();
    test_query();
    test_send();
    test_start_connection();
    test_start_proxy();
    test_send_declare();
    test_send_response(Send_response);
    test_send_response(Send_pxd_response);
    test_send_response(Send_ccd_response);
    test_extended_response();
    test_send_config(false);
    test_send_config(true);
    test_invalid();
    test_forced_fail();
    test_long_packet();
}

static const char *
test_messages[] = {
    "hello!",
    "0123456789012345"
};

static void
test_message(pxd_crypto_t *crypto, const char *message, int test_malloc)
{
    int     pass;
    int     length;
    char    iv[aes_block_size];
    char *  ciphertext;
    int     cipherlength;
    char *  plaintext;
    int     plainlength;
    char *  failtext;
    int     faillength;

    length = strlen(message);
    memset(iv, 0, sizeof(iv));

    /*
     *  If asked, force some malloc failures in encryption.
     */
    if (test_malloc) {
        for (int i = 1; i <= 2; i++) {
            malloc_countdown = i;

            pass =
                pxd_encrypt
                (
                    crypto,
                    &ciphertext,
                    &cipherlength,
                    iv,
                    message,
                    length
                );

            if (pass) {
                log("pxd_encrypt didn't trigger a malloc failure\n");
                exit(1);
            }
        }
    }

    /*
     *  Okay, now try encrypting the message.
     */
    pass =
        pxd_encrypt
        (
            crypto,
            &ciphertext,
            &cipherlength,
            iv,
            message,
            length
        );

    if (!pass) {
        log("pxd_encrypt failed in test_message.\n");
        exit(1);
    }

    /*
     *  Okay, now force an error.
     */
    fail_encrypt = true;

    pass =
        pxd_encrypt
        (
            crypto,
            &failtext,
            &faillength,
            iv,
            message,
            length
        );

    if (pass) {
        log("pxd_encrypt didn't trigger an error.\n");
        exit(1);
    }

    /*
     *  If asked, force some malloc failures in decryption.
     */
    if (test_malloc) {
        for (int i = 1; i <= 2; i++) {
            malloc_countdown = i;

            pass =
                pxd_decrypt
                (
                    crypto,
                    &plaintext,
                    &plainlength,
                    ciphertext,
                    cipherlength
                );

            if (pass) {
                log("pxd_encrypt didn't trigger a malloc failure\n");
                exit(1);
            }
        }
    }

    /*
     *  Okay, now try decrypting the ciphertext.
     */
    pass =
        pxd_decrypt
        (
            crypto,
            &plaintext,
            &plainlength,
            ciphertext,
            cipherlength
        );

    if (!pass) {
        log("pxd_decrypt failed in test_message.\n");
        exit(1);
    }

    if (!memeq(plaintext, plainlength, message, length)) {
        log("The test message does match the output.\n");
        exit(1);
    }

    free(ciphertext);

    /*
     *  Now force an error.
     */
    fail_decrypt = true;

    pass =
        pxd_decrypt
        (
            crypto,
            &failtext,
            &faillength,
            ciphertext,
            cipherlength
        );

    if (pass) {
        log("pxd_decrypt failed to trigger an error.\n");
        exit(1);
    }

    free(plaintext);
}

static void
test_crypto(void)
{
    pxd_crypto_t *  crypto;

    char *   ciphertext;
    int      cipherlength;
    char *   plaintext;
    int      plainlength;
    int      pass;
    char     buffer[4 * aes_block_size];
    char     output[8 * aes_block_size];
    int16_t  length;
    int      result;

    malloc_countdown = 1;

    crypto = pxd_create_crypto(key, sizeof(key), 0);

    if (crypto != null || malloc_countdown > 0) {
        log("pxd_create_crypto didn't trigger a malloc failure\n");
        exit(1);
    }

    /*
     *  pxd_free_crypto should be okay with a null pointer.
     */
    pxd_free_crypto(&crypto);

    /*
     *  Now test a valid crypto structure.
     */
    crypto = pxd_create_crypto(key, sizeof(key), 201071823);

    if (pxd_encrypt(crypto, &ciphertext, &cipherlength, null, null, 0)) {
        log("pxd_encrypt didn't fail on a zero length.\n");
        exit(1);
    }

    if (pxd_encrypt(crypto, &ciphertext, &cipherlength, null, null, pxd_max_encrypt + 1)) {
        log("pxd_encrypt didn't fail on a long message.\n");
        exit(1);
    }

    cipherlength = 13;

    pass = 
        pxd_decrypt
        (
            crypto,
            &plaintext,
            &plainlength,
            ciphertext,
            cipherlength
        );

    if (pass) {
        log("pxd_decrypt didn't fail on a bad input length.\n");
        exit(1);
    }

    cipherlength = aes_block_size;

    pass = 
        pxd_decrypt
        (
            crypto,
            &plaintext,
            &plainlength,
            ciphertext,
            cipherlength
        );

    if (pass) {
        log("pxd_decrypt didn't fail on a short message.\n");
        exit(1);
    }

    for (int i = 0; i < array_size(test_messages); i++) {
        test_message(crypto, test_messages[i], i == 0);
    }

    /*
     *  Okay, test a corrupt message.
     */
    memset(buffer, 0, sizeof(buffer));
    memset(output, 0, sizeof(output));

    length = VPLConv_ntoh_u16(2000);
    memcpy(buffer, &length, sizeof(length));

    result = 
        aes_SwEncrypt
        (
            crypto->aes_key,
            (u8 *) output,
            (u8 *) buffer,
                   sizeof(buffer),
            (u8 *) output + aes_block_size
        );

    if (result < 0) {
        log("aes_SeEncrypt failed on the test message.\n");
        exit(1);
    }

    pass = 
        pxd_decrypt
        (
            crypto,
            &plaintext,
            &plainlength,
            output,
            aes_block_size + sizeof(buffer)
        );

    if (pass) {
        log("pxd_decrypt didn't fail on the test message.\n");
        exit(1);
    }

    pxd_free_crypto(&crypto);
}

static void
test_find_command(void)
{
    pxd_io_t        io;
    pxd_command_t   command;
    pxd_command_t   blank;
    pxd_unpacked_t  unpacked;

    int  found;

    memset(&io,       0, sizeof(io      ));
    memset(&blank,    0, sizeof(blank   ));
    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.async_id = 0;
    
    command = pxd_find_command(&io, &unpacked, &found);
    
    if (found || !memeq((char *) &command, sizeof(command), (char *) &blank, sizeof(blank))) {
        log("pxd_find_command failed with a negative id.\n");
        exit(1);
    }

    pxd_mutex_init(&io.mutex);

    io.outstanding[4].valid    = true;
    io.outstanding[4].async_id = 5;
    io.outstanding[4].type     = Send_pxd_challenge;
    unpacked.async_id          = io.outstanding[4].async_id;

    command = pxd_find_command(&io, &unpacked, &found);

    if (!found || command.type != Send_pxd_challenge) {
        log("pxd_find_command failed with a valid id!\n");
        exit(1);
    }

    VPLMutex_Destroy(&io.mutex);
}

static void
test_ans_packet(void)
{
    pxd_unpacked_t    input;
    pxd_unpacked_t *  output;
    pxd_error_t       error;

    char *  body;
    int     length;

    input.type             = Start_proxy_connection;
    input.user_id          = 801;
    input.device_id        = 802;
    input.instance_id      = instance_id;
    input.request_id       = 803;
    input.pxd_dns          = test_cluster;
    input.address_count    = 1;
    input.addresses        = (pxd_address_t *) malloc(sizeof(pxd_address_t));
    input.server_instance  = server_instance;

    input.addresses[0].ip_address = address;
    input.addresses[0].ip_length  = sizeof(address);
    input.addresses[0].port       = 10101;
    input.addresses[0].type       = 42;

    body = pxd_pack_ans(&input, &length);

    if (body != null) {
        log("pxd_pack_ans didn't detect an invalid type.\n");
        exit(1);
    }

    /*
     *  Force a malloc error.
     */
    malloc_countdown = 1;
    input.type = pxd_connect_request;
    body = pxd_pack_ans(&input, &length);

    if (body != null || malloc_countdown > 0) {
        log("pxd_unpack_ans didn't fail in malloc.\n");
        exit(1);
    }

    /*
     *  Force a pack error.
     */
    pxd_fail_pack = true;
    body = pxd_pack_ans(&input, &length);

    if (body != null) {
        log("pxd_fail_packet didn't trigger.\n");
        exit(1);
    }

    pxd_fail_pack = false;

    /*
     *  Create a connection request and try to unpack it.
     */
    input.type = pxd_connect_request;
    body       = pxd_pack_ans(&input, &length);
    output     = pxd_unpack_ans(body, length, &error);

    if (error.error != 0) {
        log("pxd_unpack_ans failed:  %s\n", error.message);
        exit(1);
    }

    /*
     *  Check that the results match.
     */
    check_fixed (input.type,            output->type,            "type",       input.type);
    check_fixed (input.user_id,         output->user_id,         "user id",    input.type);
    check_fixed (input.device_id,       output->device_id,       "device id",  input.type);
    check_fixed (input.request_id,      output->request_id,      "request id", input.type);
    check_string(input.pxd_dns,         output->pxd_dns,         "pxd dns",    input.type);
    check_string(input.server_instance, output->server_instance, "server",     input.type);
    check_string(input.instance_id,     output->instance_id,     "instance",   input.type);
    check_fixed (input.address_count,   output->address_count,   "device id",  input.type);

    check_addresses(&input, output);

    free(body);
    pxd_free_unpacked(&output);

    /*
     *  Okay, test the other message type:  a wakeup.
     */
    input.type = pxd_wakeup;
    body       = pxd_pack_ans(&input, &length);
    output     = pxd_unpack_ans(body, length, &error);

    check_fixed(input.type,          output->type,          "type",       input.type);
    check_string(input.instance_id,  output->instance_id,   "instance",   input.type);

    /*
     *  Okay, force a malloc error in unpack.
     */
    pxd_free_unpacked(&output);
    malloc_countdown = 1;
    output = pxd_unpack_ans(body, length, &error);

    if (output != null || error.error != VPL_ERR_NOMEM || malloc_countdown > 0) {
        log("pxd_unpack_ans didn't fail in malloc.\n");
        exit(1);
    }

    free(input.addresses);
    free(body);
}

static void
test_blob(void)
{
    pxd_blob_t    input;
    pxd_blob_t *  blob;
    pxd_error_t   error;
    char *        packed_blob;
    int           packed_length;
    char *        no_blob;
    int           no_length;
    
    blob = pxd_unpack_blob(null, 0, shared_crypto, &error);

    if (blob != null || error.error != VPL_ERR_INVALID) {
        log("pxd_unpack_blob didn't fail on an empty buffer.\n");
        exit(1);
    }

    input.client_user         = client_user;
    input.client_device       = client_device;
    input.server_user         = server_user;
    input.server_device       = server_device;
    input.create              = VPLTime_GetTime();
    input.key                 = proxy_key;
    input.key_length          = sizeof(proxy_key);
    input.handle              = client_handle;
    input.handle_length       = sizeof(client_handle);
    input.service_id          = service_id;
    input.service_id_length   = sizeof(service_id);
    input.ticket              = ticket;
    input.ticket_length       = sizeof(ticket);
    input.client_instance     = client_instance;
    input.server_instance     = server_instance;
    input.extra               = extra;
    input.extra_length        = strlen(extra) + 1;

    pxd_fail_pack = true;
    error.error = 0;

    packed_blob = pxd_pack_blob(&packed_length, &input, shared_crypto, &error);

    if (packed_blob != null || error.error == 0) {
        log("pxd_pack_blob didn't trigger a pack failure.\n");
        exit(1);
    }

    error.error    = 0;
    pxd_fail_pack  = false;

    packed_blob = pxd_pack_blob(&packed_length, &input, shared_crypto, &error);

    if (packed_blob == null || error.error != 0) {
        log("pxd_pack_blob failed on valid input.\n");
        exit(1);
    }

    blob = pxd_unpack_blob(packed_blob, packed_length, shared_crypto, &error);

    if (blob == null || error.error != 0) {
        log("pxd_unpack_blob failed on valid input:  %s\n", error.message);
        exit(1);
    }

    check_fixed (input.client_user,     blob->client_user,   "client user"  , 0);
    check_fixed (input.client_device,   blob->client_device, "client device", 0);
    check_fixed (input.server_user,     blob->server_user,   "server user"  , 0);
    check_fixed (input.server_device,   blob->server_device, "server_device", 0);
    check_fixed (input.create,          blob->create,        "create"       , 0);

    check_bytes
    (
        input.key, input.key_length,
        blob->key, blob->key_length,
        "blob key", 0
    );

    check_bytes
    (
        input.extra, input.extra_length,
        blob->extra, blob->extra_length,
        "blob extra", 0
    );

    check_string(input.handle,          blob->handle,          "handle"         , 0);
    check_string(input.service_id,      blob->service_id,      "server id"      , 0);
    check_string(input.ticket,          blob->ticket,          "ticket"         , 0);
    check_string(input.client_instance, blob->client_instance, "client instance", 0);
    check_string(input.server_instance, blob->server_instance, "server instance", 0);

    /*
     *  Free the blob.  A double free should be fine.
     */
    pxd_free_blob(&blob);
    pxd_free_blob(&blob);
    free(packed_blob);

    /*
     *  Clear the extra fields and see that the blob code still works.
     */
    input.extra_length = 0;

    packed_blob = pxd_pack_blob(&packed_length, &input, shared_crypto, &error);

    if (packed_blob == null || error.error != 0) {
        log("pxd_pack_blob failed on valid input.\n");
        exit(1);
    }

    blob = pxd_unpack_blob(packed_blob, packed_length, shared_crypto, &error);

    if (blob == null || error.error != 0) {
        log("pxd_unpack_blob failed on valid input:  %s\n", error.message);
        exit(1);
    }

    pxd_free_blob(&blob);

    /*
     *  Now test the error paths.
     */
    for (int i = 1; i <= 11; i++) {
        if (i < 10 ) {
            malloc_countdown = i;

            no_blob = pxd_pack_blob(&no_length, &input, shared_crypto, &error);

            if (no_blob != null || error.error == 0 || malloc_countdown > 0) {
                log("pxd_pack_blob didn't trigger a malloc failure (%d)\n", i);
                exit(1);
            }
        }

        malloc_countdown = i;

        blob = pxd_unpack_blob(packed_blob, packed_length, shared_crypto, &error);

        if (blob != null || error.error == 0 || malloc_countdown > 0) {
            log("pxd_unpack_blob didn't trigger a malloc failure (%d)\n", i);
            exit(1);
        }
    }

    (*packed_blob)++;
    pxd_blob_signatures = 0;

    blob = pxd_unpack_blob(packed_blob, packed_length, shared_crypto, &error);

    if (blob != null || error.error == 0 || pxd_blob_signatures == 0) {
        log("pxd_unpack_blob didn't detect a bad signature.\n");
        exit(1);
    }

    free(packed_blob);
}

static void
test_demon_blob(void)
{
    pxd_blob_t    input;
    pxd_blob_t *  blob;
    pxd_error_t   error;
    char *        packed_blob;
    int           packed_length;
    char *        no_blob;
    int           no_length;
    
    blob = pxd_unpack_demon_blob(null, 0, shared_crypto, &error);

    if (blob != null || error.error != VPL_ERR_INVALID) {
        log("pxd_unpack_demon_blob didn't fail on an empty buffer.\n");
        exit(1);
    }

    input.client_user         = client_user;
    input.client_device       = client_device;
    input.create              = VPLTime_GetTime();
    input.key                 = proxy_key;
    input.key_length          = sizeof(proxy_key);
    input.handle              = client_handle;
    input.handle_length       = sizeof(client_handle);
    input.service_id          = service_id;
    input.service_id_length   = sizeof(service_id);
    input.ticket              = ticket;
    input.ticket_length       = sizeof(ticket);
    input.client_instance     = client_instance;
    input.extra               = extra;
    input.extra_length        = strlen(extra) + 1;

    error.error = 0;
    pxd_fail_pack = true;

    packed_blob = pxd_pack_demon_blob(&packed_length, &input, shared_crypto, &error);

    if (blob != null || error.error == 0) {
        log("pxd_unpack_demon_blob didn't trigger a pack failure\n");
        exit(1);
    }

    pxd_fail_pack = false;
    error.error   = 0;

    packed_blob = pxd_pack_demon_blob(&packed_length, &input, shared_crypto, &error);

    if (packed_blob == null || error.error != 0) {
        log("pxd_pack_demon_blob failed:  %s\n", error.message);
        exit(1);
    }

    blob = pxd_unpack_demon_blob(packed_blob, packed_length, shared_crypto, &error);

    if (blob == null || error.error != 0) {
        log("pxd_unpack_demon_blob failed on valid input:  %s\n", error.message);
        exit(1);
    }

    check_fixed (input.client_user,     blob->client_user,   "client user"  , 0);
    check_fixed (input.client_device,   blob->client_device, "client device", 0);
    check_fixed (input.create,          blob->create,        "create"       , 0);

    check_bytes
    (
        input.key, input.key_length,
        blob->key, blob->key_length,
        "blob key", 0
    );

    check_bytes
    (
        input.extra, input.extra_length,
        blob->extra, blob->extra_length,
        "demon blob extra", 0
    );

    check_string(input.handle,          blob->handle,          "handle"         , 0);
    check_string(input.service_id,      blob->service_id,      "server id"      , 0);
    check_string(input.ticket,          blob->ticket,          "ticket"         , 0);
    check_string(input.client_instance, blob->client_instance, "client instance", 0);

    /*
     *  Free the blob.  A double free should be fine.
     */
    pxd_free_blob(&blob);
    pxd_free_blob(&blob);
    free(packed_blob);

    /*
     *  Now clear the extra field and see that the code still works.
     */
    input.extra_length = 0;

    packed_blob = pxd_pack_demon_blob(&packed_length, &input, shared_crypto, &error);

    if (packed_blob == null || error.error != 0) {
        log("pxd_pack_demon_blob failed:  %s\n", error.message);
        exit(1);
    }

    blob = pxd_unpack_demon_blob(packed_blob, packed_length, shared_crypto, &error);

    if (blob == null || error.error != 0) {
        log("pxd_unpack_demon_blob failed on valid input:  %s\n", error.message);
        exit(1);
    }

    pxd_free_blob(&blob);

    /*
     *  Now test the error paths.
     */
    for (int i = 1; i <= 10; i++) {
        if (i < 10 ) {
            malloc_countdown = i;

            no_blob = pxd_pack_demon_blob(&no_length, &input, shared_crypto, &error);

            if (no_blob != null || error.error == 0 || malloc_countdown > 0) {
                log("pxd_pack_blob didn't trigger a malloc failure (%d)\n", i);
                exit(1);
            }
        }

        malloc_countdown = i;

        blob = pxd_unpack_demon_blob(packed_blob, packed_length, shared_crypto, &error);

        if (blob != null || error.error == 0 || malloc_countdown > 0) {
            log("pxd_unpack_demon_blob didn't trigger a malloc failure (%d)\n", i);
            exit(1);
        }
    }

    (*packed_blob)++;
    pxd_demon_blob = 0;

    blob = pxd_unpack_demon_blob(packed_blob, packed_length, shared_crypto, &error);

    if (blob != null || error.error == 0 || pxd_blob_signatures == 0) {
        log("pxd_unpack_demon_blob didn't detect a bad signature.\n");
        exit(1);
    }

    free(packed_blob);
}

static void
test_unpack_helpers(void)
{
    char      buffer[200];
    char *    base;
    char *    end;
    char *    result;
    char *    output;
    int       length;
    int       error;
    uint16_t  packed_length;

    pxd_unpacked_t  unpacked;

    memset(buffer, 0, sizeof(buffer));

    length = unpack_bytes(buffer + 1, buffer, &result, "test short");

    if (length >= 0) {
        log("unpack_bytes didn't fail on a short buffer.\n");
        exit(1);
    }

    packed_length = VPLConv_ntoh_u16(4000);

    memcpy(buffer, &packed_length, sizeof(packed_length));

    length = unpack_bytes(buffer + sizeof(buffer), buffer, &result, "test short");

    if (length >= 0) {
        log("unpack_bytes didn't fail on a long count.\n");
        exit(1);
    }

    error  = false;
    base   = buffer;
    end    = buffer + sizeof(buffer);

    result = unpack_string(&base, end, &error);

    if (result != null) {
        log("unpack_string didn't fail on a long count.\n");
        exit(1);
    }

    memset(buffer, 0, sizeof(buffer));

    error = true;
    base  = buffer;
    end   = buffer + sizeof(buffer);

    result = unpack_string(&base, end, &error);

    if (result != null) {
        log("unpack_string didn't fail an error.\n");
        exit(1);
    }

    malloc_countdown = 1;
    memset(&unpacked, 0, sizeof(unpacked));

    error  = false;
    result = unpack_string(&base, end, &error);

    if (result != null || malloc_countdown != 0 || !error) {
        log("unpack_string didn't trigger a malloc error.\n");
        exit(1);
    }

    error  = false;
    result = unpack_string(&base, end, &error);

    if (result == null || strlen(result) != 0 || error) {
        log("unpack_string didn't work on the test buffer.\n");
        exit(1);
    }

    free(result);
    result = null;
    error  = false;

    malloc_countdown = 1;

    unpack_addresses(300, &base, end, &unpacked, &error);

    if (unpacked.addresses != null || malloc_countdown > 0 || !error) {
        log("unpack_addresses didn't trigger a malloc error.\n");
        exit(1);
    }

    error = false;
    base  = buffer;
    end   = buffer + 7;

    unpack_addresses(300, &base, end, &unpacked, &error);

    if (unpacked.addresses != null || !error) {
        log("unpack_addresses didn't fail on a short buffer.\n");
        exit(1);
    }

    /*
     *  Okay, send unpack_encrypted a bad count.
     */
    packed_length = ntohs(0);
    memcpy(buffer, &packed_length, sizeof(packed_length));

    output = buffer;
    length = 21;
   
    result = unpack_encrypted(null, buffer + 2, buffer, &output, &length, "test");

    if (result != null || output != null || length > 0) {
        log("unpack_encrypted didn't fail on a short buffer.\n");
        exit(1);
    }
}

static void
test_unpack(void)
{
    pxd_packet_t      packet;
    pxd_unpacked_t *  result;

    char      buffer[200];
    int64_t   sequence;
    uint64_t  tag;
    uint16_t  length;
    uint16_t  type;

    memset(buffer,  0, sizeof(buffer));
    memset(&packet, 0, sizeof(packet));

    sequence = 0;
    tag      = 0;

    /*
     *  Try a short buffer...
     */
    packet.length     = header_size - 1;
    pxd_short_packets = 0;

    if (pxd_unpack(null, &packet, null, 0) != null || pxd_short_packets == 0) {
        log("pxd_unpack didn't detect a short buffer.\n");
        exit(1);
    }

    /*
     *  Try a mismatched length and check length
     */
    pxd_check_length  = 0;
    packet.length     = header_size;
    packet.base       = buffer;
    length            = ntohs(~header_size);

    memcpy(buffer, &length, sizeof(length));

    if (pxd_unpack(null, &packet, null, tag) != null || pxd_check_length == 0) {
        log("pxd_unpack didn't detect a checksum mismatch.\n");
        exit(1);
    }

    /*
     *  A few routines check the length and check length...
     */
    if (pxd_get_type(&packet) != -1) {
        log("pxd_get_type didn't detect a checksum mismatch.\n");
        exit(1);
    }

    /*
     *  Now try a mismatched tag...
     */
    tag             = 1;
    pxd_tag_errors  = 0;
    length          = ntohs(header_size);
    memcpy(buffer + sizeof(length), &length, sizeof(length));

    if (pxd_unpack(null, &packet, null, tag) != null || pxd_tag_errors == 0) {
        log("pxd_unpack didn't detect a tag mismatch.\n");
        exit(1);
    }

    /*
     *  The packet should have an invalid type...
     */
    pxd_type_errors = 0;
    tag = 0;

    result = pxd_unpack(null, &packet, null, tag);

    if (result == null || result->type != Send_ping || pxd_type_errors == 0) {
        log("pxd_unpack didn't detect an invalid type.\n");
        exit(1);
    }

    pxd_free_unpacked(&result);

    /*
     *  Now force a length mismatch.
     */
    pxd_length_mismatches = 0;

    packet.length = header_size + 1;

    length = ntohs(~header_size);
    memcpy(buffer, &length, sizeof(length));

    length = ntohs(header_size);
    memcpy(buffer + sizeof(length), &length, sizeof(length));

    type = ntohs(Reject_ccd_credentials);
    memcpy(buffer + 2 * sizeof(uint16_t), &type, sizeof(type));

    result = pxd_unpack(null, &packet, null, tag);

    if (result != null || pxd_length_mismatches == 0) {
        log("pxd_unpack missed a length problem.\n");
        exit(1);
    }

    /*
     *  Now force a length mismatch.
     */
    pxd_packet_length = 0;
    packet.length     = header_size + 1;

    length = ntohs(~(header_size + 1));
    memcpy(buffer, &length, sizeof(length));

    length = ntohs(header_size + 1);
    memcpy(buffer + sizeof(length), &length, sizeof(length));

    result = pxd_unpack(null, &packet, null, tag);

    if (result != null || pxd_packet_length == 0) {
        log("pxd_unpack missed a packet-data length mismatch.\n");
        exit(1);
    }

    /*
     *  Now try a short buffer...
     */
    type = ntohs(Reject_ccd_credentials);
    memcpy(buffer + 2 * sizeof(uint16_t), &type, sizeof(type));

    result = pxd_unpack(null, &packet, null, tag);

    if (result != null) {
        log("pxd_unpack didn't fail on a short buffer.\n");
        exit(1);
    }
}

static void
test_output(void)
{
    pxd_io_t      io;
    pxd_packet_t  packet;

    memset(&io,     0, sizeof(io));
    memset(&packet, 0, sizeof(packet));

    pxd_mutex_init(&io.mutex);

    io.queue_head    = &packet;
    io.queue_tail    = &packet;
    io.ready_to_send = false;

    /*
     *  pxd_write_output should handle !ready_to_send.
     */
    pxd_write_output(&io);
}

typedef struct {
    pxd_io_t *  io;
    int         fd;
    int         result;
    int         done;
    pthread_t   handle;

    pxd_packet_t *  packet;
} read_t;

static void *
read_thread(void *opaque)
{
    read_t *  info;

    info = (read_t *) opaque;

    info->packet = pxd_read_packet(info->io);

    return null;
}

static void
fork_read(read_t *info)
{
    int   result;
    char  buffer;

    do {
        result = read(info->io->socket.fd, &buffer, sizeof(buffer));
    } while (result > 0);

    result = pthread_create(&info->handle, null, read_thread, info);

    if (result != 0) {
        perror("pthread_create");
        exit(1);
    }
}

static void
sync_write(read_t *info, const void *base, int length)
{
    const char *  buffer;
    int           remaining;
    int           tries;
    int           count;
    int           result;

    buffer     = (const char *) base;
    remaining  = length;
    tries      = 5;

    do {
        count = write(info->fd, buffer, remaining);

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

    result = pthread_join(info->handle, null);

    if (result != 0) {
        perror("pthread_join");
        exit(1);
    }
}

static int read_sizes[] = { 1, 2, 4, 5 };

static void
test_read_packet(void)
{
    pxd_io_t        io;
    pxd_event_t *   sockets;
    char            buffer[500];
    uint16_t        length;
    read_t          info;
    pxd_unpacked_t  unpacked;
    pxd_packet_t *  packet;
    pxd_error_t     error;
    char *          base;
    int             result;

    memset(&info, 0, sizeof(info));
    memset(&io,   0, sizeof(io  ));

    pxd_create_event(&io.event);
    pxd_create_event(&sockets );

    info.fd        = sockets->out_socket.fd;
    info.io        = &io;

    pxd_mutex_init(&io.mutex);
    io.socket      = sockets->socket;
    io.readable    = true;

    result = fcntl(io.socket.fd, F_SETFL, O_NONBLOCK);
    result = fcntl(info.fd,      F_SETFL, O_NONBLOCK);

    /*
     *  Send an over-size length.
     */
    length = ntohs(~32767);
    memcpy(&buffer[0], &length, sizeof(length));

    length = ntohs(32767);
    memcpy(&buffer[2], &length, sizeof(length));

    pxd_packet_size_errors  = 0;
    pxd_partial_timeout     = 4;

    fork_read(&info);

    sync_write(&info, buffer, 2 * sizeof(length));

    if (info.packet != null || pxd_packet_size_errors != 1) {
        error("pxd_read_packet accepted an invalid packet size.\n");
        exit(1);
    }

    /*
     *  Send a length-check-length mismatch.
     */
    pxd_size_mismatches = 0;

    length = ntohs(~80);
    memcpy(&buffer[0], &length, sizeof(length));

    length = ntohs(81);
    memcpy(&buffer[2], &length, sizeof(length));

    pxd_packet_size_errors  = 0;

    fork_read(&info);

    sync_write(&info, buffer, 2 * sizeof(length));

    if (info.packet != null || pxd_size_mismatches != 1) {
        error("pxd_read_packet accepted an invalid packet size.\n");
        exit(1);
    }

    /*
     *  Force some malloc failures.  First, make a valid
     *  packet.
     */
    memset(&unpacked, 0, sizeof(unpacked));

    unpacked.type      = Send_pxd_response;
    unpacked.response  = pxd_op_successful;

    packet = pxd_pack(&unpacked, null, &error);
    base   = packet->base;
    length = packet->length;

    for (int i = 1; i <= 2; i++) {
        malloc_countdown = i;

        fork_read(&info);

        sync_write(&info, base, length);

        if (info.packet != null || malloc_countdown > 0) {
            error("pxd_read_packet didn't trigger a malloc failure at %d.\n", (int) i);
            exit(1);
        }
    }

    /*
     *  Do a partial packet timeout after n bytes.
     */
    pxd_partial_timeout = 3;

    for (int i = 0; i < array_size(read_sizes); i++) {
        pxd_partial_timeouts = 0;
        fork_read(&info);

        sync_write(&info, base, read_sizes[i]);

        if (info.packet != null || pxd_partial_timeouts != 1) {
            error("pxd_read_packet failed.\n");
            exit(1);
        }
    }

    /*
     *  Okay, try various I/O errors.
     */
    poll_error = VPL_ERR_IO;

    fork_read(&info);

    sync_write(&info, base, length);

    if (info.packet != null) {
        error("pxd_read_packet didn't fail on an I/O error.\n");
        exit(1);
    }

    poll_revents = VPLSOCKET_POLL_ERR;

    fork_read(&info);

    sync_write(&info, base, length);

    if (info.packet != null) {
        error("pxd_read_packet didn't fail on an poll flags error.\n");
        exit(1);
    }

    /*
     *  read_packet should recover from EAGAIN.
     */
    receive_error = VPL_ERR_AGAIN;

    fork_read(&info);

    sync_write(&info, base, length);

    if (info.packet == null) {
        error("pxd_read_packet didn't recover from VPL_ERR_AGAIN.\n");
        exit(1);
    }

    pxd_free_packet(&info.packet);

    receive_error = VPL_ERR_IO;

    fork_read(&info);

    sync_write(&info, base, length);

    if (info.packet != null) {
        error("pxd_read_packet didn't fail on an I/O error.\n");
        exit(1);
    }

    /*
     *  Okay, set stop_now...
     */
    io.stop_now = true;
    fork_read(&info);

    sync_write(&info, base, length);

    if (info.packet != null) {
        error("pxd_read_packet failed.\n");
        exit(1);
    }

    pxd_free_packet(&packet);

    pxd_free_event(&io.event);
    pxd_free_event(&sockets );
}

int
main(int argc, char **argv, char **envp)
{
    VPLNet_addr_t  address;

    char      buffer[200];

    VPLTime_GetTime(); /* start the timer */

    printf("Starting the pxd packet test.\n");
    address = htonl(0x7f640003);
    pxd_convert_address(address, buffer, sizeof(buffer));
    
    if (strcmp(buffer, "127.100.0.3") != 0) {
        log("pxd_convert_address failed\n");
        exit(1);
    }

    shared_crypto = pxd_create_crypto(key, sizeof(key), 0x10101010);

    if (shared_crypto == null) {
        log("pxd_create_crypto failed\n");
        exit(1);
    }

    shared_io.crypto = shared_crypto;

    test_random        ();
    test_responses     ();
    test_type_names    ();
    test_append        ();
    test_packets       ();
    test_queue_packet  ();
    test_check_timeout ();
    test_crypto        ();
    test_find_command  ();
    test_ans_packet    ();
    test_blob          ();
    test_demon_blob    ();
    test_unpack_helpers();
    test_unpack        ();
    test_output        ();
    test_read_packet   ();

    if (pxd_lock_errors != 0) {
        log("The test had lock errors.\n");
        exit(1);
    }

    pxd_free_crypto(&shared_crypto);
    printf("The pxd packet test passed.\n");
    return 0;
}

static void *
my_malloc(size_t size)
{
    void *  result;

    if (--malloc_countdown == 0) {
        result = null;
    } else {
        result = malloc(size);
    }

    return result;
}

static int
my_aes_SwEncrypt(u8 *a1, u8 *a2, u8 *a3, int a4, u8 *a5)
{
    int  result;

    if (fail_encrypt) {
        result = -1;
        fail_encrypt = false;
    } else {
        result = aes_SwEncrypt(a1, a2, a3, a4, a5);
    }

    return result;
}

static int
my_aes_SwDecrypt(u8 *a1, u8 *a2, u8 *a3, int a4, u8 *a5)
{
    int  result;

    if (fail_decrypt) {
        result = -1;
        fail_decrypt = false;
    } else {
        result = aes_SwDecrypt(a1, a2, a3, a4, a5);
    }

    return result;
}

static int
my_VPLSocket_Recv(VPLSocket_t socket, void *buffer, int count)
{
    int  result;

    if (receive_error != 0) {
        result = receive_error;
        receive_error = 0;
    } else {
        result = VPLSocket_Recv(socket, buffer, count);
    }

    return result;
}

static int
my_VPLSocket_Poll(VPLSocket_poll_t *poll, int count, VPLTime_t timeout)
{
    int  result;

    if (poll_error != 0) {
        result     = poll_error;
        poll_error = 0;
    } else if (poll_revents != 0) {
        poll[poll_index].revents = poll_revents;

        result       = 0;
        poll_revents = 0;
    } else {
        result = VPLSocket_Poll(poll, count, timeout);
    }

    return result;
}
